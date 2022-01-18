#define PY_SSIZE_T_CLEAN

#include <Python.h>
#include <stdio.h>
#include <stdbool.h>
#include <structmember.h>
#include "tetrisCore.h"

int main() {
    printf("This main method does nothing but fulfill an stdio requirement.");
}

// PIECE GENERATION ---------------------------
int *pieceData[7];
int *pieceMarginData[7];
int *pieceFillData[7];
enum piece{I, J, L, O, S, T, Z};
int pieceDims[7] = {4, 3, 3, 2, 3, 3, 3};

static void installPiece(int p, int *data) {
    int dim = pieceDims[p];
    int len = dim * dim;
    int *mem = PyMem_Malloc(len*sizeof(int));

    for (int i=0; i<len; i++) {
        mem[i] = data[i];
    }

    pieceData[p] = mem;

    return;
}

static void calcPieceMargins(int p, int *data) {
    int dim = pieceDims[p];
    int *bMem = PyMem_Malloc(dim*sizeof(int));
    int *tMem = PyMem_Malloc(dim*sizeof(int));
    
    for (int x=0; x<dim; x++) {
        // March upwards until a cell is discovered
        int bottomDetected = 0;
        int bottom = -1;
        int lastCell = 0;
        for (int y = 0; y < dim; y++) {
            if (data[x * dim + y] > 0) {
                if (!bottomDetected)
                    bottom = y;
                bottomDetected = 1;

                lastCell = y;
            }
        }

        bMem[x] = bottom;
        tMem[x] = lastCell+1;

        printf("%d", lastCell+1);
    }

    printf("\n");

    pieceMarginData[p] = bMem;
    pieceFillData[p] = tMem;
}

PyObject *
tetrisCore_init(PyObject *self, PyObject *args) {
    // Initialize pieces (1st quadrant, x-y indexing)
    int i[] = {0, 0, 1, 0,
                 0, 0, 1, 0,
                 0, 0, 1, 0,
                 0, 0, 1, 0};
    int j[] = {0, 1, 1,
                   0, 1, 0,
                   0, 1, 0};
    int l[] = {0, 1, 0,
                   0, 1, 0,
                   0, 1, 1};
    int o[] = {1, 1,
                   1, 1};
    int s[] = {0, 1, 0,
                   0, 1, 1,
                   0, 0, 1};
    int t[] = {0, 1, 0,
                   0, 1, 1,
                   0, 1, 0};
    int z[] = {0, 0, 1,
                   0, 1, 1,
                   0, 1, 0};
    
    int *tp[7] = {i, j, l, o, s, t, z};
    for (int a=0; a<7; a++) {
        installPiece(a, tp[a]);
        calcPieceMargins(a, tp[a]);
    }

    // Do rotate variants in the future

    Py_RETURN_NONE;
}

PyObject *
tetrisCore_getPieceBit(PyObject *self, PyObject *args) {
    int p;
    int x;
    int y;

    if (!PyArg_ParseTuple(args, "iii", &p, &x, &y))
        return NULL;

    return PyLong_FromLong(pieceData[p][x * pieceDims[p] + y]);
}

PyObject *
tetrisCore_getPieceDim(PyObject *self, PyObject *args) {
    int p;

    if (!PyArg_ParseTuple(args, "i", &p))
        return NULL;

    return PyLong_FromLong(pieceDims[p]);
}

// BOARD --------------------------------------
static void
Board_dealloc(BoardObject *self) 
{
    // Dealloc every row on the grid
    int **grid = self->grid;
    for (int i = 0; i<self->h; i++) {
        PyObject_Free(grid[i]);
    }
    PyObject_Free(grid);
    PyObject_Free(self->ridge);
    PyObject_Free(self->sat);

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
        self->grid = PyObject_Malloc(h * sizeof(int *));
        for (int y = 0; y < h; y ++) {
            int *row = PyObject_Calloc(w, sizeof(int));
            self->grid[y] = row;
        }

        self->w = w;
        self->h = h;

        self->sat = PyObject_Calloc(h, sizeof(int));
        self->ridge = PyObject_Calloc(w, sizeof(int));
    }

    return (PyObject *) self;
}

static int
Board_init(BoardObject *self, PyObject *args, PyObject *kwds) 
{
    // Don't do anything I guess
    return 0; // 0 means everything is ok
}

static PyMemberDef Board_members[] = {
    {"w", T_INT, offsetof(BoardObject, w), 0,
     "width"},
    {"h", T_INT, offsetof(BoardObject, h), 0,
     "height"},
    {NULL}
};

static char *
boardToString(BoardObject *self) {
    int cells = self->w * self->h;
    int chars = cells + self->h + 1; // Cells + newlines + null term
    char *out = PyMem_Malloc(sizeof(char) * chars);

    for (int r = 0; r<self->h; r++) {
        int *row = self->grid[self->h - r - 1];
        int rowOffset = r * (self->w + 1);

        for (int c = 0; c < self->w; c++) {
            // Pull character
            int val = row[c]; // Build from top

            //printf("read val \n");

            int i = rowOffset + c;
            char oc = '_';

            if (val > 0)
                oc = 'O';

            out[i] = oc;
        }

        out[rowOffset + self->w] = '\n';
    }

    out[chars-1] = '\0'; // Terminate string

    return out;
}

static void
printBoard(BoardObject *b) {
    char *str = boardToString(b);
    printf(str);
    PyMem_Free(str);
}

static PyObject *
Board_asString(BoardObject *self, PyObject *Py_UNUSED(ignored))
{   
    return PyUnicode_FromString(boardToString(self));
}

static int inBounds(BoardObject *board, int x, int y) {
    if (x < 0 || x >= board->w || y < 0 || y >= board->h) {
        printf("Index out of bounds: %d, %d\n", x, y);
        return 0;
    }

    return 1;
}

static int getBit(BoardObject *board, int x, int y) {
    // TODO: Make this compiler optional in case it's slowing things down
    if (!inBounds(board, x, y)) 
        return -1;
    
    int *row = board->grid[y];
    int val = row[x];

    return val;
}
static void setBit(BoardObject *board, int v, int x, int y) {
    if (!inBounds(board, x, y))
        return;
    
    board->grid[y][x] = v;
    return;
}

static void setBitUnchecked(BoardObject *board, int v, int x, int y) {
    board->grid[y][x] = v;
    return;
}

static PyObject *
Board_get(BoardObject *self, PyObject *args) {
    int x, y;

    if (!PyArg_ParseTuple(args, "ii", &x, &y))
        return NULL;

    int o = getBit(self, x, y);
    return PyLong_FromLong(o);
}

static PyObject *
Board_set(BoardObject *self, PyObject *args) {
    int x, y, v;
    
    if (!PyArg_ParseTuple(args, "iii", &v, &x, &y))
        return NULL;

    setBit(self, v, x, y);
    Py_RETURN_NONE;
}

static PyObject *
Board_getRidge(BoardObject *self, PyObject *args) {
    int i;

    if (!PyArg_ParseTuple(args, "i", &i))
        return NULL;

    if (!inBounds(self, i, 0))
        return NULL;
    
    return PyLong_FromLong(self->ridge[i]);
}

static PyObject *
Board_setRidge(BoardObject *self, PyObject *args) {
    int v, i;

    if (!PyArg_ParseTuple(args, "ii", &v, &i))
        return NULL;
    
    if (!inBounds(self, i, 0))
        return NULL;

    self->ridge[i] = v;
    
    Py_RETURN_NONE;
}

static PyObject *
Board_getSat(BoardObject *self, PyObject *args) {
    int i;

    if (!PyArg_ParseTuple(args, "i", &i))
        return NULL;
    
    if (!inBounds(self, 0, i))
        return NULL;

    return PyLong_FromLong(self->sat[i]);
}

static PyObject *
Board_setSat(BoardObject *self, PyObject *args) {
    int v, i;

    if (!PyArg_ParseTuple(args, "ii", &v, &i))
        return NULL;
    
    if (!inBounds(self, 0, 1))
        return NULL;

    self->sat[i] = v;
    Py_RETURN_NONE;
}

// Perform a deep copy of the board
static PyObject *
Board_copy(BoardObject *self, PyObject *Py_UNUSED(ignored)) {
    BoardObject *cp;
    cp = (BoardObject *) PyObject_CallObject((PyObject *) &BoardType, NULL);

    // Copy grid
    for (int r=0; r<self->h; r++) {
        // Copy row
        memcpy(cp->grid[r], self->grid[r], self->w*sizeof(int));
    }

    // Copy ridge and saturation
    memcpy(cp->ridge, self->ridge, self->w*sizeof(int));
    memcpy(cp->sat, self->sat, self->h*sizeof(int));

    return (PyObject *) cp;
}



static BoardObject* genHypoBoard(int p, int x, BoardObject *b) {
    int pDims = pieceDims[p];
    int *pData = pieceData[p];
    
    // Perform the ridge march
    int y=-2;
    int conflict = 0;

    for(int i=0; i<pDims; i++) {
        int col = x+i;
        int bMargin = pieceMarginData[p][i];
        if (bMargin < 0) continue;

        if (col < 0 || col >= b->w) {
            conflict = 1;
            break;
        }

        int rTop = b->ridge[col] - bMargin; // todo: generate the ridge, note that it is shifted off by one
        if (rTop > y) y = rTop;
    }

    if (conflict || y > b->h - pDims) return NULL;

    // Create a hypothetical board
    BoardObject *hb = (BoardObject *) Board_copy(b, NULL);
    for (int lx=0; lx<pDims; lx++) {
        for (int ly=0; ly<pDims; ly++) {
            int cell = pData[lx*pDims + ly];

            if (cell > 0) {
                setBitUnchecked(hb, cell, x+lx, y+ly); // Unchecked for sp e e d.
            }
        }
    }

    int holesMade = 0;
    int holesRemoved = 0;

    // Update hypo board ridge
    for(int i=0; i<pDims; i++) {
        int rDelta = pieceFillData[p][i];
        if (rDelta < 0) continue;

        int col = x + i;

        // Calculate holes (unused rn)
        int margin = pieceMarginData[p][i];
        int holesMade = y - hb->ridge[col] + margin;

        // Update ridge altitude
        hb->ridge[col] = y + rDelta;
    }

    // Todo: read line clears for hole resolution
    
    return hb;
}

static int testPeaks(BoardObject *b) {
    int *r = b->ridge;
    int maxAlt = 0;
    for (int i=0; i<b->w; i++) {
        if (r[i] > maxAlt) maxAlt = r[i];
    }

    return maxAlt;
}

static PyObject *
Board_positionPiece(BoardObject *self, PyObject *args) {
    int p;

    if (!PyArg_ParseTuple(args, "i", &p))
        return NULL;

    int maxScore = -100000;
    int bestX = 0;
    BoardObject *bestBoard = NULL;
    // Iterate through available pieces
    for (int x=-2; x<self->w; x++) {
        // Only horizontal shifts for now

        // Pull score
        BoardObject *results = genHypoBoard(p, x, self);
        if (results == NULL) continue;
        
        int score = 0;
        score += testPeaks(results) * -1;

        if (score > maxScore) {
            maxScore = score;
            bestX = x;
            bestBoard = results;
        }

        printf("pos: %d, score: %d\n", x, score);
    }

    if (bestBoard != NULL) {
        printf("base ridge: \n");
        for(int i=0; i<self->w; i++) printf("%d ", self->ridge[i]);
        printf("\n x: %d, piece no.: %d\n", bestX, p);
        printBoard(self);
        printf("new ridge: \n");
        for(int i=0; i<self->w; i++) printf("%d ", bestBoard->ridge[i]);
        printf("\n");
        printBoard(bestBoard);
    }

    return PyLong_FromLong(bestX);
}



static PyMethodDef Board_methods[] = {
    {"get", (PyCFunction) Board_get, METH_VARARGS,
     "Retrieve the element at a specified position"
    },
    {"set", (PyCFunction) Board_set, METH_VARARGS,
     "Set the element at a specified position to a specific value"
    },
    {"copy", (PyCFunction) Board_copy, METH_NOARGS,
     "Performs a deep copy of the board"
    },
    {"getRidge", (PyCFunction) Board_getRidge, METH_VARARGS, ""
    },
    {"setRidge", (PyCFunction) Board_setRidge, METH_VARARGS, ""
    },
    {"getSat", (PyCFunction) Board_getSat, METH_VARARGS, ""
    },
    {"setSat", (PyCFunction) Board_setSat, METH_VARARGS, ""
    },
    {"positionPiece", (PyCFunction) Board_positionPiece, METH_VARARGS, ""
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
    .tp_methods = Board_methods,
    .tp_str = Board_asString
};

static PyMethodDef coreMethods[] = {
    {"init", tetrisCore_init, METH_VARARGS, "Initializes the Tetris Core."},
    {"getPieceBit", tetrisCore_getPieceBit, METH_VARARGS, "Retrieves a cell from a tetromino."},
    {"getPieceDim", tetrisCore_getPieceDim, METH_VARARGS, "Retrieves dimensions for a tetromino."},
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