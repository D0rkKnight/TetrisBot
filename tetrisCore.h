#ifndef TETRISCORE_H_
#define TETRISCORE_H_

#include <Python.h>

typedef struct {
    PyObject_HEAD
    unsigned * grid;
    int w;
    int h;
    int * ridge; // Counts from 0
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
void printBitstringAsString(size_t const size, void const * const ptr);
static int getPieceBit(int p, int r, int x, int y);

static void setBitUnchecked(BoardObject *board, int v, int x, int y);
static int getBitUnchecked(BoardObject *board, int x, int y);

int Board_calcScore(BoardObject *board);

genBoardReturn * Board_genHypoBoard(int p, int x, int r, int slide, BoardObject *b);
int slideBoard(BoardObject *hb, int p, int r, int x, int y, int slide);
void printBoard(BoardObject *b);

unsigned tetrisCore_getBag();

#endif