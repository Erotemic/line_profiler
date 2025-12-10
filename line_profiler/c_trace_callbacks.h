#ifndef LINE_PROFILER_C_TRACE_CALLBACKS_H
#define LINE_PROFILER_C_TRACE_CALLBACKS_H

#include "Python_wrapper.h"

typedef struct TraceCallback
{
    /* Store the Python-level trace callable and its argument.  With the
     * stable ABI we cannot rely on CPython implementation details such
     * as `PyThreadState.c_tracefunc`, so we only keep the high-level
     * Python objects here. */
    PyObject *trace_callable;
} TraceCallback;

TraceCallback *alloc_callback();
void free_callback(TraceCallback *callback);
void populate_callback(TraceCallback *callback);
void restore_callback(TraceCallback *callback);
int call_callback(
    PyObject *disabler,
    TraceCallback *callback,
    PyFrameObject *py_frame,
    int what,
    PyObject *arg
);
void set_local_trace(PyObject *manager, PyFrameObject *py_frame);
Py_uintptr_t monitoring_restart_version();

#endif // LINE_PROFILER_C_TRACE_CALLBACKS_H
