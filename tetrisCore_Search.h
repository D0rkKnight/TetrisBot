#ifndef _TCORE_CALC_
#define _TCORE_CALC_

#include "tetrisCore.h"

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

static moveset search(BoardObject *self, int *queue, int levelsLeft, int hold);
static moveResult *makePlay(BoardObject *board, int x, int r, int p);
static moveset *searchSub(BoardObject *board, int *queue, int levelsLeft, int hold, int x, int r);

PyObject *Board_search(BoardObject *self, PyObject *args);

#endif