#ifndef _TCORE_CALC_
#define _TCORE_CALC_

#include "tetrisCore.h"

typedef struct {
    int targetX;
    int targetRot;
    int score;
    int inherScore;
    int hold;
} moveset;

typedef struct {
    moveset move;
    BoardObject *gridResult;
} moveResult;

typedef struct {
    int *grid;
    int pieces; // Just current pieces, queue should be the same if the depth is the same
    int depth;  // For now, only cull if both boards are at the same depth. I think this can be optimized further.
    int bag;    // In case we are in post queue free search
    int inherScore; // TODO: Inherited score needs to be linked downwards so every child node knows its inherited score from arrival.
    cacheEntry *next; // Linked list structure
} cacheEntry;

// Idea: assign a score to each cache entry, so when future searches look through again, it immediately knows the score for each associated cache entry.
// If the grid, pieces, depth, bag, and inherited score are the same

static moveset search(BoardObject *self, int *queue, int levelsLeft, int hold);
static moveResult *makePlay(BoardObject *board, int x, int r, int p);
static moveset *searchSub(BoardObject *board, int *queue, int levelsLeft, int hold, int x, int r);

static int calcScore(BoardObject *board);
static int calcInherScore(BoardObject *board, lcReturn lc);

PyObject *Board_search(BoardObject *self, PyObject *args);

#endif