#define PY_SSIZE_T_CLEAN    // programmers love obscure statements
#include <Python.h>
#include <new>

struct NullTracerObject {
    PyObject_HEAD
    int count;

    NullTracerObject() : count(0) {}
};

static PyObject*
NullTracer_new(PyTypeObject* type, PyObject* args, PyObject* kwargs) {
//    fprintf(stderr, "new\n");
    NullTracerObject* self = (NullTracerObject*) type->tp_alloc(type, 0);
    if (self != nullptr) {
        new (self) NullTracerObject;
    }

    return (PyObject*)self;
}

static void
NullTracer_dealloc(PyObject* self) {
//    fprintf(stderr, "dealloc\n");
    reinterpret_cast<NullTracerObject*>(self)->~NullTracerObject();
    Py_TYPE(self)->tp_free(self);
}


static int
Nulltracer_trace(PyObject* self, PyFrameObject *frame, int what, PyObject *arg_unused) {
//    fprintf(stderr, "called(C)\n");
    ++reinterpret_cast<NullTracerObject*>(self)->count;
    return 0;   // -1 is error
}

static PyObject*
NullTracer_call(PyObject* self, PyObject* args, PyObject* kwargs) {
//    fprintf(stderr, "called(Python)\n");
    ++reinterpret_cast<NullTracerObject*>(self)->count;
    Py_INCREF(self);
    return self;
}

static PyMethodDef NullTracer_methods[] = {
    {NULL}
};

static PyTypeObject NullTracerType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "nulltracer.nulltracer",
    .tp_basicsize = sizeof(NullTracerObject),
    .tp_dealloc = NullTracer_dealloc,
    .tp_call = NullTracer_call,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_doc = "Null tracer",
    .tp_methods = NullTracer_methods,
    .tp_new = NullTracer_new,
};

static NullTracerObject* _tracer = nullptr;

PyObject*
nulltracer_get_count(PyObject* self, PyObject* const* args, Py_ssize_t nargs) {
    return PyLong_FromLong(_tracer ? _tracer->count : 0);
}

static PyMethodDef nulltracer_module_methods[] = {
    {"get_count",     (PyCFunction)nulltracer_get_count, METH_FASTCALL, "returns tracer call count"},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef nulltracer_module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "nulltracer",
    .m_doc = NULL, // no documentation
    .m_size = -1,
    .m_methods = nulltracer_module_methods,
};


PyMODINIT_FUNC
PyInit_nulltracer() {
    if (PyType_Ready(&NullTracerType) < 0) {
        return nullptr;
    }

    PyObject* m = PyModule_Create(&nulltracer_module);
    if (m == nullptr) {
        return nullptr;
    }

#if 0
    Py_INCREF(&NullTracerType);
    if (PyModule_AddObject(m, "nulltracer", (PyObject*)&NullTracerType) < 0) {
        Py_DECREF(&NullTracerType);
        Py_DECREF(m);
        return nullptr;
    }
#endif

    _tracer = PyObject_New(NullTracerObject, &NullTracerType);
    PyEval_SetTrace(Nulltracer_trace, (PyObject*)_tracer);

    return m;
}
