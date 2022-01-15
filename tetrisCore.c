#define PY_SSIZE_T_CLEAN

#include <Python.h>
#include <stdio.h>

int main() {
    printf("Hello World!");
}

static PyObject *
tetrisCore_add(PyObject *self, PyObject *args) {
    int *l1 = malloc(sizeof(int));
    int *l2 = malloc(sizeof(int));
    int o;
    
    printf("before parse\n");
    if (!PyArg_ParseTuple(args, "ii", l1, l2))
        return NULL;
    printf("after parse\n");
    o = *l1 + *l2;

    return PyLong_FromLong(o);
}

// static PyObject *
// tetrisCore_

// C compiles linearly.

static PyMethodDef coreMethods[] = {
    {"add", tetrisCore_add, METH_VARARGS, "Add test"},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef coreModule = {
    PyModuleDef_HEAD_INIT,
    "tetrisCore",
    "Faster calculations for Tetris domination",
    -1,
    coreMethods
};

PyMODINIT_FUNC
PyInit_tetrisCore(void) {
    return PyModule_Create(&coreModule);
}