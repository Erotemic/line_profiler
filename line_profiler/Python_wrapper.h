// Compatibility layer over `Python.h`.

#ifndef LINE_PROFILER_PYTHON_WRAPPER_H
#define LINE_PROFILER_PYTHON_WRAPPER_H

// Opt into the stable ABI starting at Python 3.8
#ifndef Py_LIMITED_API
#  define Py_LIMITED_API 0x03080000
#endif

#include "Python.h"

// Under the stable ABI the concrete frame / code structs are opaque. Python
// 3.8's limited headers do not expose the forward declarations, so provide
// aliases in that case only. Newer versions ship the typedefs even under the
// limited API, so avoid redefining them to prevent conflicts.
#if defined(Py_LIMITED_API) && PY_VERSION_HEX < 0x03090000
typedef PyObject PyFrameObject;
typedef PyObject PyCodeObject;
#endif

// Backport of Python 3.9 caller hooks

#ifndef PyObject_CallOneArg
#   define PyObject_CallOneArg(func, arg) \
        PyObject_CallFunctionObjArgs(func, arg, NULL)
#endif
#ifndef PyObject_CallMethodOneArg
#   define PyObject_CallMethodOneArg(obj, name, arg) \
        PyObject_CallMethodObjArgs(obj, name, arg, NULL)
#endif
#ifndef PyObject_CallNoArgs
#   define PyObject_CallNoArgs(func) \
        PyObject_CallFunctionObjArgs(func, NULL)
#endif
#ifndef PyObject_CallMethodNoArgs
#   define PyObject_CallMethodNoArgs(obj, name) \
        PyObject_CallMethodObjArgs(obj, name, NULL)
#endif

#ifndef PyTrace_CALL
#   define PyTrace_CALL 0
#   define PyTrace_EXCEPTION 1
#   define PyTrace_LINE 2
#   define PyTrace_RETURN 3
#   define PyTrace_OPCODE 4
#   define PyTrace_C_CALL 5
#   define PyTrace_C_EXCEPTION 6
#   define PyTrace_C_RETURN 7
#endif

#ifndef Py_tracefunc
typedef int (*Py_tracefunc)(PyObject *, PyFrameObject *, int, PyObject *);
#endif

#ifndef PyEval_SetTrace
PyAPI_FUNC(void) PyEval_SetTrace(Py_tracefunc func, PyObject *arg);
#endif

#ifndef PyCode_Addr2Line
PyAPI_FUNC(int) PyCode_Addr2Line(PyCodeObject *co, int byte_offset);
#endif

#if PY_VERSION_HEX < 0x030900a5  // 3.9.0a5
#   define PyThreadState_GetInterpreter(tstate) \
        ((tstate)->interp)
#endif

#if PY_VERSION_HEX < 0x030900b1  // 3.9.0b1
    /*
     * Notes:
     *     While 3.9.0a1 already has `PyFrame_GetCode()`, it doesn't
     *     INCREF the code object until 0b1 (PR #19773), so override
     *     that for consistency.
     */
#   define PyFrame_GetCode(x) PyFrame_GetCode_backport(x)
    inline PyCodeObject *PyFrame_GetCode_backport(PyFrameObject *frame)
    {
        PyObject *code_obj = PyObject_GetAttrString((PyObject *)frame, "f_code");
        if (code_obj == NULL) {
            return NULL;
        }
        return (PyCodeObject *)code_obj;
    }
#endif

#if PY_VERSION_HEX < 0x030b00b1  // 3.11.0b1
    /*
     * Notes:
     *     Since 3.11.0a7 (PR #31888) `co_code` has been made a
     *     descriptor, so:
     *     - This already creates a NewRef, so don't INCREF in that
     *       case; and
     *     - `code->co_code` will not work.
     */
    inline PyObject *PyCode_GetCode(PyCodeObject *code)
    {
        if (code == NULL) return NULL;
        return PyObject_GetAttrString((PyObject *)code, "co_code");
    }
#endif

#if PY_VERSION_HEX < 0x030d00a1  // 3.13.0a1
    inline PyObject *PyImport_AddModuleRef(const char *name)
    {
        PyObject *mod = NULL, *name_str = NULL;
        name_str = PyUnicode_FromString(name);
        if (name_str == NULL) goto cleanup;
        mod = PyImport_AddModuleObject(name_str);
        Py_XINCREF(mod);
    cleanup:
        Py_XDECREF(name_str);
        return mod;
    }
#endif

#endif // LINE_PROFILER_PYTHON_WRAPPER_H
