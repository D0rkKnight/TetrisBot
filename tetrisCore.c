#define PY_SSIZE_T_CLEAN

#include <Python.h>
#include <stdio.h>
#include <stdbool.h>
#include <structmember.h>
#include "tetrisCore.h"

// PIECE GENERATION ---------------------------
int *pieceData[7][4];
int *pieceMarginData[7][4];
int *pieceFillData[7][4];
enum piece{I, J, L, O, S, T, Z};
int pieceDims[7] = {4, 3, 3, 2, 3, 3, 3};

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

    // for (int i=0; i<len; i++) {
    //     printf("%d", mem[i]);
    // } 

    // printf("\n");

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

        //printf("%d", bMem[x]);
    }

    //printf("\n");

    pieceMarginData[p][r] = bMem;
    pieceFillData[p][r] = tMem;
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
        }
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

    return PyLong_FromLong(pieceData[p][0][x * pieceDims[p] + y]);
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

static int getBitUnchecked(BoardObject *board, int x, int y) {
    return board->grid[y][x];
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

    // Copy hole count
    cp->holes = self->holes;

    return (PyObject *) cp;
}

static int lineclear(BoardObject *b) {
    // Indexed buffer to hold lines for swaps
    int **clearedBuffer = PyMem_Malloc(sizeof(int)*b->h);
    int clearAddIndex = 0;
    int clearPullIndex = 0;
    int addLine = 0;

    int *lineArr = PyMem_Malloc(sizeof(int)*b->h);
    int lineArrSize = 0;

    for (int r=0; r<b->h; r++) {
        if (b->sat[r] >= b->w) {
            clearedBuffer[clearAddIndex] = b->grid[r];
            clearAddIndex ++;

            lineArr[lineArrSize] = r;
            lineArrSize++;
        } else {
            b->grid[addLine] = b->grid[r];
            addLine ++;
        }
    }

    if (clearAddIndex == 0) return 0;

    // Fill cleared lines back in as empty
    for (int r=0; r<clearAddIndex; r++) {
        int *row = clearedBuffer[r];
        memset(row, 0, sizeof(int)*b->w); // Clear row out
        b->grid[addLine+r] = row;
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

    // Return number of lineclears
    return clearAddIndex;
}

static BoardObject* genHypoBoard(int p, int x, int r, BoardObject *b) {
    int pDims = pieceDims[p];
    int *pData = pieceData[p][r];
    
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
            int cell = pData[lx*pDims + ly];

            if (cell > 0) {
                setBitUnchecked(hb, cell, x+lx, y+ly); // Unchecked for sp e e d.
                hb->sat[y+ly] ++;
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

    int clears = lineclear(hb);

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

static int testPits(BoardObject *b) {
    int o = 0;

    //printBoard(b);

    for(int i=1; i<b->w; i++){
        int prevAlt = b->ridge[i-1];
        int nextAlt = b->ridge[i];

        int diff = nextAlt-prevAlt;
        if (diff<0) diff*=-1;

        //printf("advance: %d, ridge: %d\n", diff, b->ridge[i]);

        o += diff;
    }
    return o;
}

static int testHoles(BoardObject *b) {
    return b->holes;
}

static PyObject *
Board_search(BoardObject *self, PyObject *args) {
    PyObject *queueObj;
    int hold1;
    if (!PyArg_ParseTuple(args, "Oi", &queueObj, &hold1))
        return NULL;
    if (!PyList_Check(queueObj))
        return NULL;

    int qLen = PyList_Size(queueObj);
    printf("Queue size: %d\n", qLen);
    printf("Hold: %d\n", hold1);

    // Construct initial data (hold branching will occur within the recursive search)
    int *queue = PyMem_Malloc(sizeof(int) * qLen);
    for (int i=0; i<qLen; i++) {
        queue[i] = PyLong_AsLong(PyList_GetItem(queueObj, i));
    }

    // Perform recursive search
    moveset move = search(self, queue, qLen, hold1);
    PyObject *toHold = Py_False;
    if (move.hold) toHold = Py_True;

    // Format output
    PyObject *o = PyTuple_New(3);
    PyTuple_SetItem(o, 0, PyLong_FromLong(move.targetX));
    PyTuple_SetItem(o, 1, PyLong_FromLong(move.targetRot));
    PyTuple_SetItem(o, 2, toHold);

    PyMem_Free(queue);
    return o;
}

static void destroy(moveResult *res) {
    if (res != NULL) PyMem_Free(res->gridResult);
    PyMem_Free(res);
}

// Recursive function has a f1-f2-f1-f2 branch structure
static moveset
search(BoardObject *board, int *queue1, int levelsLeft, int hold1) {
    // Generate an alt queue (with the hold)
    int qLen = levelsLeft;
    int *queue2 = PyMem_Malloc(sizeof(int) * qLen);
    queue2 = memcpy(queue2, queue1, sizeof(int) * qLen);
    queue2[0] = hold1;
    int hold2 = queue1[0];

    // Make a play
    moveResult *bestRes = NULL;
    for (int x=-2; x<board->w; x++) {
        for (int r=0; r<4; r++) {
            // Pull score
            moveResult *res1 = searchSub(board, queue1, levelsLeft-1, hold1, x, r);
            moveResult *res2 = searchSub(board, queue2, levelsLeft-1, hold2, x, r);
            if (res2 != NULL) res2->move.hold = true; // Specifying that this option entails a hold

            if (res1 == NULL && res2 == NULL) continue;

            // Selecting the better choice and deallocating the other
            moveResult *choice = res1;
            if (res1 == NULL || (res2 != NULL && res2->move.score > res1->move.score)) {
                choice=res2;
                destroy(res1);
            } else {
                destroy(res2);
            }

            // Compare choice against the current best choice
            if (bestRes == NULL || choice->move.score > bestRes->move.score) {
                destroy(bestRes);
                bestRes = choice;
            } else {
                destroy(choice);
            }
        }
    }

    moveset o = {.score = -100000};
    if (bestRes != NULL) o = bestRes->move;

    return o;
}

static moveResult *searchSub(BoardObject *board, int *queue, int levelsLeft, int hold, int x, int r) {
    // Pull score
    moveResult *res = makePlay(board, x, r, queue[0]);
    if (res == NULL) return NULL;

    // Stop branching if last level
    if (levelsLeft == 0) return res;

    // Shorten queue
    int *newQueue = PyMem_Malloc(sizeof(int)*levelsLeft);
    for (int i=0; i<levelsLeft; i++) newQueue[i] = queue[i+1];

    // Adopt the scoring of all child outcomes
    int futureScore = search(res->gridResult, newQueue, levelsLeft, hold).score;
    res->move.score = futureScore;

    return res;
}

static moveResult *makePlay(BoardObject *board, int x, int r, int p) {
    // Pull score
    BoardObject *results = genHypoBoard(p, x, r, board);
    if (results == NULL) return NULL;

    //printBoard(results);

    int score = calcScore(results);

    moveResult *o = PyMem_Malloc(sizeof(moveResult));
    moveset move = {.score = score, .targetRot = r, .targetX = x};
    o->move = move;
    o->gridResult = results;

    return o;
}

static int calcScore(BoardObject *board) {
    int s = 0;
    s += testPeaks(board) * -1;
    s += testPits(board) * -1;
    s += testHoles(board) * -10;

    return s;
}

// DEPRECATED
// static moveResult *
// positionPiece(BoardObject *self, int p) {

//     // printBoard(self);
//     // printf("piece: %d\n", p);

//     moveResult *bestRes = NULL;
//     // Iterate through available pieces
//     for (int x=-2; x<self->w; x++) {
//         for (int r=0; r<4; r++) {
//             // Pull score
//             moveResult *res = makePlay(self, x, r, p);
//             if (res == NULL) continue;

//             if (bestRes == NULL || res->move.score > bestRes->move.score) {
//                 PyMem_Free(bestRes);
//                 bestRes = res;
//             } else {
//                 PyMem_Free(res);
//             }

//             // printf("pos: %d, rot: %d, score: %d, peak: %d, pit %d, hole %d\n", x, r, score, peakScore, pitScore, holeScore);
//             // for(int i=0; i<results->w; i++) printf("%d", results->ridge[i]);
//             // printf("\n");
//         }
//     }

//     if (bestRes != NULL) {
//         // printf("base ridge: \n");
//         // for(int i=0; i<self->w; i++) printf("%d ", self->ridge[i]);
//         // printf("\n x: %d, piece no.: %d\n", bestX, p);
//         // printBoard(self);
//         // printf("new ridge: \n");
//         // for(int i=0; i<self->w; i++) printf("%d ", bestBoard->ridge[i]);
//         // printf("\n");
//         // printBoard(bestBoard);

//         // printf("HOLES: %d\n", testHoles(bestBoard));

//         // printf("\n\n\nBoard comparison below\n");
//         // printBoard(bestBoard);
//         // BoardObject *lcHB = (BoardObject *) Board_copy(bestBoard, NULL);
//         // lineclear(lcHB);
//         // printBoard(lcHB);
//     }

//     // printf("x: %d, r: %d\n", bestX, bestRot);
//     return bestRes;
// }



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
    {"search", (PyCFunction) Board_search, METH_VARARGS, ""
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