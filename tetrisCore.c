#define PY_SSIZE_T_CLEAN

#include <Python.h>
#include <stdio.h>
#include <stdbool.h>
#include <structmember.h>
#include "tetrisCore.h"
#include "tetrisCore_Search.h"

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

// PIECE GENERATION ---------------------------
int *pieceData[7][4];
int *pieceMarginData[7][4];
int *pieceFillData[7][4];
enum piece{I, J, L, O, S, T, Z};
int pieceDims[7] = {4, 3, 3, 2, 3, 3, 3};
unsigned bag = 0U;

int pieceBitstrings[7][4];

static void installPiece(int p, int r, int *data) {
    int dim = pieceDims[p];
    int len = dim * dim;
    int *mem = PyMem_Malloc(len*sizeof(int));

    for (int i=0; i<len; i++) {
        int x = i/dim;
        int y = i%dim;
        for (int i=0; i<r; i++) {
            // Spin
            int t = x;
            x = y;
            y = -t;
        }

        // Reposition
        if (r%4==2 || r%4==3) x += dim-1;
        if (r%4==1 || r%4==2) y += dim-1;

        // This should work?
        mem[x*dim+y] = data[i];
    }

    pieceData[p][r] = mem;

    return;
}

static void calcPieceMargins(int p, int r, int *data) {
    int dim = pieceDims[p];
    int *bMem = PyMem_Malloc(dim*sizeof(int));
    int *tMem = PyMem_Malloc(dim*sizeof(int));
    
    for (int x=0; x<dim; x++) {
        // March upwards until a cell is discovered
        int bottomDetected = 0;
        int bottom = -1;
        int lastCell = -2;
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
    }

    pieceMarginData[p][r] = bMem;
    pieceFillData[p][r] = tMem;
}

static void installPieceBitstring(int p, int r, int *data) {
    int len = pieceDims[p] * pieceDims[p];
    unsigned o = 0;
    
    for (int i = 0; i<len; i++) {
        o = o | (data[i] << i);
    }

    pieceBitstrings[p][r] = o;
}

// Assumes little endian, from https://stackoverflow.com/questions/111928/is-there-a-printf-converter-to-print-in-binary-format
void printBitstringAsString(size_t const size, void const * const ptr) {
    unsigned char *b = (unsigned char *) ptr;
    unsigned char byte;
    int i, j;

    for (i = ((int) size)-1; i >= 0; i--) {
        for (j = 7; j >= 0; j--) {
            byte = (b[i] >> j) & 1U;
            printf("%u", byte);
        }
    }

    printf("\n");
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
        for (int r=0; r<4; r++) {
            installPiece(a, r, tp[a]);
            calcPieceMargins(a, r, pieceData[a][r]);
            installPieceBitstring(a, r, pieceData[a][r]);
        }
    }
    Search_init();

    Py_RETURN_NONE;
}

static int getPieceBit(int p, int r, int x, int y) {
    return pieceBitstrings[p][r] >> (x * pieceDims[p] + y) & 1U;
}

PyObject *
tetrisCore_getPieceDim(PyObject *self, PyObject *args) {
    int p;

    if (!PyArg_ParseTuple(args, "i", &p))
        return NULL;

    return PyLong_FromLong(pieceDims[p]);
}

PyObject *
tetrisCore_setBag(PyObject *self, PyObject *args) {
    PyObject *arr;

    if (!PyArg_ParseTuple(args, "O", &arr))
        return NULL;
    
    int len = (int) PyList_Size(arr);

    bag = 0;
    for (int i=0; i<len; i++) {
        bag |= 1U << PyLong_AsLong(PyList_GetItem(arr, i));
    }

    Py_RETURN_NONE;
}

unsigned tetrisCore_getBag() {
    return bag;
}

// BOARD --------------------------------------
static void
Board_dealloc(BoardObject *self) 
{
    // Grid is 1D array so direct deallocation is safe
    PyObject_Free(self->grid);
    PyObject_Free(self->ridge);

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
        self->grid = PyObject_Calloc(h, sizeof(int));
        self->w = w;
        self->h = h;

        self->ridge = PyObject_Calloc(w, sizeof(int));

        self->holes = 0;
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
    {"holes", T_INT, offsetof(BoardObject, holes), 0,
    "Number of holes"},
    {NULL}
};

static char *
boardToString(BoardObject *self) {
    int cells = self->w * self->h;
    int chars = cells + self->h + 1; // Cells + newlines + null term
    char *out = PyMem_Malloc(sizeof(char) * chars);

    for (int r = 0; r<self->h; r++) {
        int row = self->grid[self->h - r - 1];
        int rowOffset = r * (self->w + 1);

        for (int c = 0; c < self->w; c++) {
            // Pull character
            int val = row >> c & 1U; // Build from top

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

static void
printArr(int *arr, int l) {
    for (int i=0; i<l; i++) printf("%d", arr[i]);
    printf("\n");
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

    return getBitUnchecked(board, x, y);
}
static void setBit(BoardObject *board, int v, int x, int y) {
    if (!inBounds(board, x, y))
        return;
    
    setBitUnchecked(board, v, x, y);
    return;
}

static void setBitUnchecked(BoardObject *board, int v, int x, int y) {
    int row = board->grid[y];
    if (v > 0) row = row | (1U << x);
    else row = row & ~(1U << x);

    board->grid[y] = row;
    return;
}

static int getBitUnchecked(BoardObject *board, int x, int y) {
    int row = board->grid[y];
    int val = row >> x & 1U;

    return val;
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

// Perform a deep copy of the board
static PyObject *
Board_copy(BoardObject *self, PyObject *Py_UNUSED(ignored)) {
    BoardObject *cp;
    cp = (BoardObject *) PyObject_CallObject((PyObject *) &BoardType, NULL);

    // Copy grid
    memcpy(cp->grid, self->grid, self->h*sizeof(int));

    // Copy ridge and saturation
    memcpy(cp->ridge, self->ridge, self->w*sizeof(int));

    // Copy hole count
    cp->holes = self->holes;

    return (PyObject *) cp;
}

static lcReturn lineclear(BoardObject *b) {
    // Indexed buffer to hold lines for swaps
    int clearAddIndex = 0;
    int clearPullIndex = 0;
    int addLine = 0;

    int *lineArr = PyMem_Malloc(sizeof(int)*b->h);
    int lineArrSize = 0;

    const unsigned fullRow = ~(~0U << 10);

    for (int r=0; r<b->h; r++) {
        if (b->grid[r] == fullRow) {
            clearAddIndex ++;

            lineArr[lineArrSize] = r;
            lineArrSize++;
        } else {
            b->grid[addLine] = b->grid[r];
            addLine ++;
        }
    }

    if (clearAddIndex > 0) {
        // Fill cleared lines back in as empty
        for (int r=0; r<clearAddIndex; r++) {
            b->grid[addLine+r] = 0;
        }

        // Recalculate ridges and open up holes
        for(int x=0; x<b->w; x++) {
            for (int l=lineArrSize-1; l>=0; l--) {
                if (b->ridge[x] > lineArr[l]) {
                    b->ridge[x] -= l+1;

                    // Account for newly exposed gaps
                    while (b->ridge[x] > 0 && getBitUnchecked(b, x, b->ridge[x]-1) == 0) {
                        b->ridge[x] -= 1;

                        // Every one of these means a hole has been cleared.
                        b->holes --;
                    }

                    break;
                }
            }
        }
    }

    // Return number of lineclears
    lcReturn o = {.clears = clearAddIndex, .clearLocs = lineArr};
    return o;
}

genBoardReturn * Board_genHypoBoard(int p, int x, int r, BoardObject *b) {
    int pDims = pieceDims[p];
    
    // Perform the ridge march
    int y=-2;
    int conflict = 0;

    for(int i=0; i<pDims; i++) {
        int col = x+i;
        int bMargin = pieceMarginData[p][r][i];
        if (bMargin < 0) continue;

        if (col < 0 || col >= b->w) {
            conflict = 1;
            break;
        }

        int rTop = b->ridge[col] - bMargin;
        if (rTop > y) y = rTop;
    }

    if (conflict || y > b->h - pDims) return NULL;

    // Create a hypothetical board
    BoardObject *hb = (BoardObject *) Board_copy(b, NULL);

    for (int lx=0; lx<pDims; lx++) {
        for (int ly=0; ly<pDims; ly++) {
            int cell = getPieceBit(p, r, lx, ly);

            if (cell > 0) {
                setBitUnchecked(hb, cell, x+lx, y+ly); // Unchecked for sp e e d.
            }
        }
    }

    // Update hypo board ridge
    for(int i=0; i<pDims; i++) {
        int rDelta = pieceFillData[p][r][i];
        if (rDelta < 0) continue;

        int col = x + i;

        // Calculate holes
        int margin = pieceMarginData[p][r][i];
        int holesMade = y - hb->ridge[col] + margin;
        hb->holes+=holesMade;

        // Update ridge altitude
        hb->ridge[col] = y + rDelta;
    }

    lcReturn lc = lineclear(hb);

    genBoardReturn *o = PyMem_Malloc(sizeof(genBoardReturn));
    o->board = hb;
    o->lc = lc;

    return o;
}

static PyObject *
Board_searchWrapper(BoardObject *self, PyObject *args) {
    return Board_search(self, args);
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
    {"search", (PyCFunction) Board_searchWrapper, METH_VARARGS, ""
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
    {"getPieceDim", tetrisCore_getPieceDim, METH_VARARGS, "Retrieves dimensions for a tetromino."},
    {"setBag", tetrisCore_setBag, METH_VARARGS, "Sets bag state."},
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
    printf("Does this even work?\n");

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