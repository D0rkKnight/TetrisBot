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

typedef struct {
    int targetX;
    int targetRot;
    int score;
    int hold;
} moveset;

typedef struct {
    moveset move;
    BoardObject *gridResult;
} moveResult;

static PyTypeObject BoardType;
static int testPeaks(BoardObject *b);
static moveset search(BoardObject *self, int *queue, int levelsLeft, int hold);
static moveResult *positionPiece(BoardObject *self, int p);
static int calcScore(BoardObject *board);
static moveResult *makePlay(BoardObject *board, int x, int r, int p);
static moveResult *searchSub(BoardObject *board, int *queue, int levelsLeft, int hold, int x, int r);

#endif