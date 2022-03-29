#ifndef _TCORE_CALC_
#define _TCORE_CALC_

#include "tetrisCore.h"

typedef struct {
    int targetX;
    int targetRot;
    int score;
    int inherScore;
    int hold;
    int slide;
} moveset;

typedef struct {
    moveset move;
    BoardObject *gridResult;
} moveResult;

typedef struct {
    int *grid;
    int h;
    unsigned pieces; // Just current pieces, queue should be the same if the depth is the same
    int levelsLeft;  // No. of levels left to evaluate
    unsigned bag;    // In case we are in post queue free search
    int inherScore; // TODO: Inherited score needs to be linked downwards so every child node knows its inherited score from arrival.
    void *next; // Linked list structure
} cacheEntry;

typedef struct {
    cacheEntry **cells;
    unsigned len;
} cacheMap;

// Idea: assign a score to each cache entry, so when future searches look through again, it immediately knows the score for each associated cache entry.
// If the grid, pieces, depth, bag, and inherited score are the same

static moveset search(BoardObject *self, int *queue, int levelsLeft, int hold, int iScore);
static moveResult *makePlay(BoardObject *board, int x, int r, int slide, int p);
static moveset *searchSub(BoardObject *board, int *queue, int levelsLeft, int hold, int x, int r, int slide, int iScore);

static int calcScore(BoardObject *board);
static int calcInherScore(BoardObject *board, lcReturn lc);

static int hashGameState(cacheEntry ent, cacheMap *map);
static int cacheGameState(BoardObject *board, int piece1, int piece2, int depthMarker, unsigned bag, int inherScore, cacheMap *map);

PyObject *Board_search(BoardObject *self, PyObject *args);
static cacheMap * genCacheMap(unsigned len);
static void clearCache(cacheMap *cMap);
static void destroyCacheEntry(cacheEntry *ent);
void dumpState(moveResult *res);

void Search_init();

#endif