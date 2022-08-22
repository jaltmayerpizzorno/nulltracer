#define PY_SSIZE_T_CLEAN    // programmers love obscure statements
#include <Python.h>
#include <frameobject.h>
#include <new>

struct NullTracerObject {
    PyObject_HEAD
    int count;
    PyObject* file_prefix;

    NullTracerObject() : count(0), file_prefix(nullptr) {}

    ~NullTracerObject() {
        Py_DecRef(file_prefix);
    }

    bool shouldTrace(PyObject* filename) {
        if (file_prefix == nullptr) {
            return true;
        }

        Py_ssize_t length;
        const char* prefix = PyUnicode_AsUTF8AndSize(file_prefix, &length);
        return strncmp(prefix, PyUnicode_AsUTF8(filename), length) == 0;
    }
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
    NullTracerObject* tracer = reinterpret_cast<NullTracerObject*>(self);
    ++tracer->count;

    if (what == PyTrace_CALL && !tracer->shouldTrace(frame->f_code->co_filename)) {
        frame->f_trace_lines = 0;   // disable tracing (for this frame)
    }
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

PyObject*
nulltracer_set_prefix(PyObject* self, PyObject* const* args, Py_ssize_t nargs) {
    if (nargs < 1) {
        PyErr_SetString(PyExc_Exception, "Missing argument(s)");
        return NULL;
    }
    if (!PyUnicode_Check(args[0])) {
        PyErr_SetString(PyExc_Exception, "Prefix is not a string.");
        return NULL;
    }
    if (_tracer == nullptr) {
        PyErr_SetString(PyExc_Exception, "No tracer active");
        return NULL;
    }

    Py_IncRef(args[0]);
    _tracer->file_prefix = args[0];

    Py_RETURN_NONE;
}

static PyMethodDef nulltracer_module_methods[] = {
    {"get_count",     (PyCFunction)nulltracer_get_count, METH_FASTCALL, "returns tracer call count"},
    {"set_prefix",     (PyCFunction)nulltracer_set_prefix, METH_FASTCALL, "sets tracer file prefix"},
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

    // PyObject_New doesn't call tp_new... not sure why.
    // _tracer = PyObject_New(NullTracerObject, &NullTracerType);
    _tracer = (NullTracerObject*)NullTracer_new(&NullTracerType, nullptr, nullptr);
    PyEval_SetTrace(Nulltracer_trace, (PyObject*)_tracer);

    return m;
}
