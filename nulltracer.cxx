#define PY_SSIZE_T_CLEAN    // programmers love obscure statements
#include <Python.h>
#include <frameobject.h>
#include <new>
#include <string>

struct NullTracerObject {
    PyObject_HEAD
    int count;
    std::string abs_prefix;
    std::string rel_prefix;

    NullTracerObject() : count(0) {}

    ~NullTracerObject() {}

    void setFilePrefix(const char* a_prefix, const char* r_prefix) {
        abs_prefix = a_prefix;
        if (r_prefix) rel_prefix = r_prefix;
        fprintf(stderr, "abs=%s, rel=%s\n", a_prefix, r_prefix ? r_prefix : "(none)");
    }

    bool shouldTrace(PyObject* filename) {
        // XXX could use PyUnicode_Tailmatch instead, and skip conversions...
        if (const char* u8name = PyUnicode_AsUTF8(filename)) {
            if (u8name[0] == '<' || u8name[0] == '\0' || strncmp("memory:", u8name, sizeof("memory:")-1)==0) {
                return false;
            }

            if (abs_prefix.length()) {
                if (u8name[0] == '/') {
                    return strncmp(abs_prefix.c_str(), u8name, abs_prefix.length()) == 0;
                }

                if (rel_prefix.length()) {
                    return strncmp(rel_prefix.c_str(), u8name, rel_prefix.length()) == 0;
                }
            }
        }
        return true;
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
//    fprintf(stderr, "Tracing %s\n", PyUnicode_AsUTF8(frame->f_code->co_filename));

    if (what == PyTrace_CALL) {
        if (!tracer->shouldTrace(frame->f_code->co_filename)) {
            frame->f_trace_lines = 0;   // disable tracing (for this frame)
        }
    }

    // XXX we could do (like coveragepy does), in case settrace(gettrace()) is done
    // XXX Py_INCREF(self);
    // XXX Py_XSETREF(frame->f_trace, self);

    return 0;   // -1 is error
}

static PyObject*
NullTracer_call(PyObject* self, PyObject* args, PyObject* kwargs) {
//    fprintf(stderr, "called(Python)\n");
    PyFrameObject* frame;
    PyObject* what;
    PyObject* arg;

    if (!PyArg_ParseTuple(args, "OOO", &frame, &what, &arg)) {
        return nullptr;
    }

    if (PyUnicode_CompareWithASCIIString(what, "call") == 0) {
        // for best speed, switch to C style tracing
        PyEval_SetTrace(Nulltracer_trace, self);
    }

    ++reinterpret_cast<NullTracerObject*>(self)->count;

    Py_INCREF(self);
    return self;
}

PyObject*
Nulltracer_get_count(PyObject* self, PyObject* const* args, Py_ssize_t nargs) {
    return PyLong_FromLong(reinterpret_cast<NullTracerObject*>(self)->count);
}

PyObject*
Nulltracer_set_prefix(PyObject* self, PyObject* const* args, Py_ssize_t nargs) {
    if (nargs < 1) {
        PyErr_SetString(PyExc_Exception, "Missing argument(s)");
        return NULL;
    }
    if (!PyUnicode_Check(args[0]) || nargs > 1 && !PyUnicode_Check(args[0])) {
        PyErr_SetString(PyExc_Exception, "Prefix is not a string.");
        return NULL;
    }

    const char* abs_prefix = PyUnicode_AsUTF8(args[0]);
    const char* rel_prefix = nargs > 1 ? PyUnicode_AsUTF8(args[1]) : nullptr;
    reinterpret_cast<NullTracerObject*>(self)->setFilePrefix(abs_prefix, rel_prefix);

    Py_RETURN_NONE;
}

static PyMethodDef NullTracer_methods[] = {
    {"get_count",   (PyCFunction)Nulltracer_get_count, METH_FASTCALL, "returns tracer call count"},
    {"set_prefix",  (PyCFunction)Nulltracer_set_prefix, METH_FASTCALL, "sets tracer file prefix"},
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

static PyMethodDef nulltracer_module_methods[] = {
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

    Py_INCREF(&NullTracerType);
    if (PyModule_AddObject(m, "nulltracer", (PyObject*)&NullTracerType) < 0) {
        Py_DECREF(&NullTracerType);
        Py_DECREF(m);
        return nullptr;
    }

    return m;
}
