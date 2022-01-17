#define PY_SSIZE_T_CLEAN

#include <Python.h>
#include <stdio.h>
#include <structmember.h>

int main() {
    printf("Hello World!");
}

typedef struct {
    PyObject_HEAD
    int ** grid;
    int w;
    int h;
} BoardObject;

static void
Board_dealloc(BoardObject *self) 
{
    // Dealloc every row on the grid
    int **grid = self->grid;
    for (int i = 0; i<self->h; i++) {
        free(grid[i]);
    }
    free(grid);

    Py_TYPE(self)->tp_free((PyObject *) self);
}

static PyObject *
Board_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    // Allocate space for object
    BoardObject *self;
    self = (BoardObject *) type->tp_alloc(type, 0);

    // Dunno how to read input, 10x20 grid for now
    int w = 10;
    int h = 20;

    if (self != NULL) {
        self->grid = malloc(h * sizeof(int));
        for (int y = 0; y < h; y ++) {
            int *row = calloc(w, sizeof(int));
            self->grid[y] = row;
        }

        self->w = w;
        self->h = h;
    }
    return (PyObject *) self;
}

static int
Board_init(BoardObject *self, PyObject *args, PyObject *kwds) 
{
    // Don't do anything I guess
    printf("Initiallizing a board! \n");

    return 0; // 0 means everything is ok
}

static PyMemberDef Board_members[] = {
    {"w", T_INT, offsetof(BoardObject, w), 0,
     "width"},
    {"h", T_INT, offsetof(BoardObject, h), 0,
     "height"},
    {NULL}
};

static PyObject *
Board_getHeight(BoardObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *hObj = PyLong_FromLong(self->h);
    return hObj;
}

static PyObject *
Board_asString(BoardObject *self, PyObject *Py_UNUSED(ignored))
{   
    int cells = self->w * self->h;
    int chars = cells + self->h + 1; // Cells + newlines + null term
    char *out = malloc(sizeof(char) * chars);
    for (int r = 0; r<self->h; r++) {
        int *row = self->grid[r];
        int rowOffset = r * (self->w + 1);

        for (int c = 0; c < self->w; c++) {
            // Pull character
            int val = self->grid[self->h - r - 1][c]; // Build from top
            int i = rowOffset + c;
            char oc = '_';

            if (val > 0)
                oc = 'O';

            out[i] = oc;
        }

        out[rowOffset + self->w] = '\n';
    }

    out[chars-1] = '\0'; // Terminate string

    return PyUnicode_FromString(out);
}

static PyMethodDef Board_methods[] = {
    {"getHeight", (PyCFunction) Board_getHeight, METH_NOARGS,
     "Returns height"
    },
    {"asString", (PyCFunction) Board_asString, METH_NOARGS,
     "Returns string that can be printed to represent the board"
    },
    {NULL}
};

static PyTypeObject BoardType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "tetrisCore.Board",
    .tp_doc = "Board for Tetris",
    .tp_basicsize = sizeof(BoardObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = Board_new,
    .tp_init = (initproc) Board_init,
    .tp_dealloc = (destructor) Board_dealloc,
    .tp_members = Board_members,
    .tp_methods = Board_methods
};

static PyObject *
tetrisCore_printBoard(PyObject *self, PyObject *args) {
    PyObject *gridO = malloc(sizeof(PyObject));
    int **grid;

    if (!PyArg_ParseTuple(args, "O", gridO))
        return NULL;

    printf("PyObject");

    printf("stuff: %d\n", gridO);

    grid = (int **) PyCapsule_GetPointer(gridO, "grid_pointer");

    //printf("Grid discovered with first elelemnt %d\n", grid[0][0]);

    Py_RETURN_NONE;
}

// C compiles linearly.

static PyMethodDef coreMethods[] = {
    {"printBoard", tetrisCore_printBoard, METH_VARARGS, "Prints a c type tetris board to console"},
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
PyInit_tetrisCore(void) 
{
    PyObject *m;
    if (PyType_Ready(&BoardType) < 0)
        return NULL;

    m = PyModule_Create(&coreModule);
    if (m == NULL)
        return NULL;
    
    Py_INCREF(&BoardType); // Dunno why this needs a reference but ok
    if (PyModule_AddObject(m, "Board", (PyObject *) &BoardType) < 0) {
        Py_DECREF(&BoardType);
        Py_DECREF(m);
        return NULL;
    }

    return m;
}