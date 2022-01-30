#ifndef TETRISCORE_H_
#define TETRISCORE_H_

#include <Python.h>

typedef struct {
    PyObject_HEAD
    int ** grid;
    int w;
    int h;
    int * ridge;
    int * sat;
    int holes;
} BoardObject;

static PyTypeObject BoardType;
static int testPeaks(BoardObject *b);
int Board_calcScore(BoardObject *board);

BoardObject* Board_genHypoBoard(int p, int x, int r, BoardObject *b);

#endif