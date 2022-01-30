#ifndef TETRISCORE_H_
#define TETRISCORE_H_

#include <Python.h>

typedef struct {
    PyObject_HEAD
    int * grid;
    int w;
    int h;
    int * ridge;
    int * sat;
    int holes;
} BoardObject;

static PyTypeObject BoardType;
static int testPeaks(BoardObject *b);
static void installPieceBitstring(int p, int r, int *data);
static void printBitstringAsString(size_t const size, void const * const ptr);
static int getPieceBit(int p, int r, int x, int y);

static void setBitUnchecked(BoardObject *board, int v, int x, int y);
static int getBitUnchecked(BoardObject *board, int x, int y);

int Board_calcScore(BoardObject *board);

BoardObject* Board_genHypoBoard(int p, int x, int r, BoardObject *b);

#endif