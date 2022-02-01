#ifndef TETRISCORE_H_
#define TETRISCORE_H_

#include <Python.h>

typedef struct {
    PyObject_HEAD
    unsigned * grid;
    int w;
    int h;
    int * ridge;
    int holes;
} BoardObject;

typedef struct {
    int clears;
    int *clearLocs;
} lcReturn;

typedef struct {
    BoardObject *board;
    lcReturn lc;
} genBoardReturn;

static PyTypeObject BoardType;
static void installPieceBitstring(int p, int r, int *data);
static void printBitstringAsString(size_t const size, void const * const ptr);
static int getPieceBit(int p, int r, int x, int y);

static void setBitUnchecked(BoardObject *board, int v, int x, int y);
static int getBitUnchecked(BoardObject *board, int x, int y);

int Board_calcScore(BoardObject *board);

genBoardReturn * Board_genHypoBoard(int p, int x, int r, BoardObject *b);

#endif