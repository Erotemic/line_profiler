#include "c_trace_callbacks.h"

#define CYTHON_MODULE "line_profiler._line_profiler"
#define DISABLE_CALLBACK "disable_line_events"
#define RAISE_IN_CALL(func_name, xc, const_msg) \
    PyErr_SetString(xc, \
                    "in `" CYTHON_MODULE "." func_name "()`: " \
                    const_msg)

TraceCallback *alloc_callback()
{
    /* Heap-allocate a new `TraceCallback`. */
    TraceCallback *callback = (TraceCallback*)malloc(sizeof(TraceCallback));
    if (callback == NULL) RAISE_IN_CALL(
        // If we're here we have bigger fish to fry... but be nice and
        // raise an error explicitly anyway
        "alloc_callback",
        PyExc_MemoryError,
        "failed to allocate memory for storing the existing "
        "`sys` trace callback"
    );
    callback->trace_callable = NULL;
    return callback;
}

void free_callback(TraceCallback *callback)
{
    /* Free a heap-allocated `TraceCallback`. */
    if (callback != NULL) {
        Py_XDECREF(callback->trace_callable);
        free(callback);
    }
    return;
}

void populate_callback(TraceCallback *callback)
{
    /* Store the members `.c_tracefunc` and `.c_traceobj` of the
     * current thread on `callback`.
     */
    // Shouldn't happen, but just to be safe
    if (callback == NULL) return;
    // The limited API does not expose the C-level trace callback, so
    // mirror the behaviour of ``sys.gettrace()``.
    PyObject *sys_mod = PyImport_ImportModule("sys");
    if (sys_mod != NULL) {
        PyObject *getter = PyObject_GetAttrString(sys_mod, "gettrace");
        if (getter != NULL) {
            PyObject *callable = PyObject_CallNoArgs(getter);
            Py_XINCREF(callable);
            callback->trace_callable = callable;
            Py_DECREF(getter);
        }
        Py_DECREF(sys_mod);
    }
    return;
}

void nullify_callback(TraceCallback *callback)
{
    if (callback == NULL) return;
    // No need for NULL check with `Py_XDECREF()`
    Py_XDECREF(callback->trace_callable);
    callback->trace_callable = NULL;
    return;
}

void restore_callback(TraceCallback *callback)
{
    /* Use `PyEval_SetTrace()` to set the trace callback on the current
     * thread to be consistent with the `callback`, then nullify the
     * pointers on `callback`.
     */
    // Shouldn't happen, but just to be safe
    if (callback == NULL) return;
    if (callback->trace_callable != NULL && callback->trace_callable != Py_None) {
        PyObject *sys_mod = PyImport_ImportModule("sys");
        if (sys_mod != NULL) {
            PyObject *setter = PyObject_GetAttrString(sys_mod, "settrace");
            if (setter != NULL) {
                PyObject *res = PyObject_CallOneArg(setter, callback->trace_callable);
                Py_XDECREF(res);
                Py_DECREF(setter);
            }
            Py_DECREF(sys_mod);
        }
    }
    nullify_callback(callback);
    return;
}

inline int is_null_callback(TraceCallback *callback)
{
    return (
        callback == NULL
        || callback->trace_callable == NULL
        || callback->trace_callable == Py_None
    );
}

int call_callback(
    PyObject *disabler,
    TraceCallback *callback,
    PyFrameObject *py_frame,
    int what,
    PyObject *arg
)
{
    /* Call the cached trace callback `callback` where appropriate, and
     * in a "safe" way so that:
     * - If it alters the `sys` trace callback, or
     * - If it sets `.f_trace_lines` to false,
     * said alterations are reverted so as not to hinder profiling.
     *
     * Returns:
     *     - 0 if `callback` is `NULL` or has nullified members;
     *     - -1 if an error occurs (e.g. when the disabling of line
     *       events for the frame-local trace function failed);
     *     - The result of calling said callback otherwise.
     *
     * Side effects:
     *     - If the callback unsets the `sys` callback, the `sys`
     *       callback is preserved but `callback` itself is nullified.
     *       This is to comply with what Python usually does: if the
     *       trace callback errors out, `sys.settrace(None)` is called.
     *     - If a frame-local callback sets the `.f_trace_lines` to
     *       false, `.f_trace_lines` is reverted but `.f_trace` is
     *       wrapped/altered so that it no longer sees line events.
     *
     * Notes:
     *     It is tempting to assume said current callback value to be
     *     `{ python_trace_callback, <profiler> }`, but remember that
     *     our callback may very well be called via another callback,
     *     much like how we call the cached callback via
     *     `python_trace_callback()`.
     */
    TraceCallback before = {0}, after = {0};
    PyObject *f_trace = NULL;
    PyObject *f_trace_lines_obj = NULL;
    int f_trace_lines = 0;
    int result;

    if (is_null_callback(callback)) return 0;

    f_trace_lines_obj = PyObject_GetAttrString((PyObject *)py_frame, "f_trace_lines");
    if (f_trace_lines_obj != NULL) {
        f_trace_lines = PyObject_IsTrue(f_trace_lines_obj);
    }
    populate_callback(&before);
    if (callback->trace_callable != NULL && callback->trace_callable != Py_None) {
        PyObject *event = NULL;
        switch (what) {
            case PyTrace_CALL: event = PyUnicode_FromString("call"); break;
            case PyTrace_EXCEPTION: event = PyUnicode_FromString("exception"); break;
            case PyTrace_LINE: event = PyUnicode_FromString("line"); break;
            case PyTrace_RETURN: event = PyUnicode_FromString("return"); break;
#ifdef PyTrace_OPCODE
            case PyTrace_OPCODE: event = PyUnicode_FromString("opcode"); break;
#endif
            case PyTrace_C_CALL: event = PyUnicode_FromString("c_call"); break;
            case PyTrace_C_EXCEPTION: event = PyUnicode_FromString("c_exception"); break;
            case PyTrace_C_RETURN: event = PyUnicode_FromString("c_return"); break;
            default: event = PyUnicode_FromString("call"); break;
        }
        PyObject *call_result = PyObject_CallFunctionObjArgs(
            callback->trace_callable,
            (PyObject *)py_frame,
            event,
            arg,
            NULL);
        if (call_result == NULL) {
            result = -1;
        } else {
            Py_DECREF(call_result);
            result = 0;
        }
        Py_XDECREF(event);
    } else {
        result = 0;
    }

    // Check if the callback has unset itself; if so, nullify `callback`
    populate_callback(&after);
    if (is_null_callback(&after)) nullify_callback(callback);
    nullify_callback(&after);
    restore_callback(&before);

    // Check if a callback has disabled future line events for the
    // frame, and if so, revert the change while withholding future line
    // events from the callback
    Py_XDECREF(f_trace_lines_obj);
    f_trace_lines_obj = PyObject_GetAttrString((PyObject *)py_frame, "f_trace_lines");
    if (f_trace_lines_obj != NULL && !PyObject_IsTrue(f_trace_lines_obj) && f_trace_lines)
    {
        PyObject *current_f_trace = PyObject_GetAttrString((PyObject *)py_frame, "f_trace");
        PyObject *bool_obj = f_trace_lines ? Py_True : Py_False;
        PyObject_SetAttrString((PyObject *)py_frame, "f_trace_lines", bool_obj);
        if (current_f_trace != NULL && current_f_trace != Py_None)
        {
            f_trace = PyObject_CallOneArg(disabler, current_f_trace);
            if (f_trace == NULL)
            {
                result = -1;
                Py_XDECREF(current_f_trace);
                goto cleanup;
            }
            if (PyObject_SetAttrString(
                (PyObject *)py_frame, "f_trace", f_trace))
            {
                result = -1;
            }
        }
        Py_XDECREF(current_f_trace);
    }
cleanup:
    Py_XDECREF(f_trace_lines_obj);
    return result;
}

inline void set_local_trace(PyObject *manager, PyFrameObject *py_frame)
{
    /* Set the frame-local trace callable:
     * - If there isn't one already, set it to `manager`;
     * - Else, call manager.wrap_local_f_trace()` on `py_frame->f_trace`
     *   where appropriate, setting the frame-local trace callable.
     *
     * Notes:
     *     This function is necessary for side-stepping Cython's auto
     *     memory management, which causes the return value of
     *     `wrap_local_f_trace()` to trigger the "Casting temporary
     *     Python object to non-numeric non-Python type" error.
     */
    PyObject *method = NULL;
    PyObject *current_trace = NULL;
    if (manager == NULL || py_frame == NULL) goto cleanup;
    // No-op
    current_trace = PyObject_GetAttrString((PyObject *)py_frame, "f_trace");
    if (current_trace == manager) goto cleanup;
    // No local trace function to wrap, just assign `manager`
    if (current_trace == NULL || current_trace == Py_None)
    {
        PyObject_SetAttrString((PyObject *)py_frame, "f_trace", manager);
        goto cleanup;
    }
    // Wrap the trace function
    // (No need to raise another exception in case the call or the
    // `setattr()` failed, it's already raised in the call)
    method = PyUnicode_FromString("wrap_local_f_trace");
    PyObject_SetAttrString(
        (PyObject *)py_frame, "f_trace",
        PyObject_CallMethodOneArg(manager, method, current_trace));
cleanup:
    Py_XDECREF(method);
    Py_XDECREF(current_trace);
    return;
}

inline Py_uintptr_t monitoring_restart_version()
{
    // The stable ABI does not expose interpreter internals; return a
    // sentinel value indicating the version is unknown.
    return (Py_uintptr_t)0;
}
