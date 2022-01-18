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
} BoardObject;

static PyTypeObject BoardType;
static int testPeaks(BoardObject *b);

#endif