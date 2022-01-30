#include <stdio.h>
#include <stdbool.h>
#include "tetrisCore_Search.h"

PyObject *
Board_search(BoardObject *self, PyObject *args) {
    PyObject *queueObj;
    int hold1;
    if (!PyArg_ParseTuple(args, "Oi", &queueObj, &hold1))
        return NULL;
    if (!PyList_Check(queueObj))
        return NULL;

    int qLen = PyList_Size(queueObj);

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
    // Decref attached board
    if (res != NULL) {
        Py_DECREF(res->gridResult);
        PyMem_Free(res);
    }
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
    moveset bestMove = {.score=INT_MIN};
    for (int x=-2; x<board->w; x++) {
        for (int r=0; r<4; r++) {
            // Pull score
            moveset *m1 = searchSub(board, queue1, levelsLeft-1, hold1, x, r);
            moveset *m2 = searchSub(board, queue2, levelsLeft-1, hold2, x, r);
            
            if (m2 != NULL) m2->hold = true; // Specifying that this option entails a hold

            if (m1 == NULL && m2 == NULL) continue;

            // Selecting the better choice and deallocating the other
            moveset *choice = m1;
            if (m1 == NULL || (m2 != NULL && m2->score > m1->score)) {
                choice=m2;
            }

            // Compare choice against the current best choice
            if (choice->score > bestMove.score) {
                bestMove = *choice;
            }

            PyMem_Free(m1);
            PyMem_Free(m2);
        }
    }

    // Dealloc
    PyMem_Free(queue2);

    return bestMove;
}

static moveset *searchSub(BoardObject *board, int *queue, int levelsLeft, int hold, int x, int r) {
    // Pull score
    moveResult *res = makePlay(board, x, r, queue[0]);
    if (res == NULL) return NULL;

    moveset *o = PyMem_Malloc(sizeof(moveset));
    *o = res->move;

    if (levelsLeft > 0) {
        // Shorten queue
        int *newQueue = PyMem_Malloc(sizeof(int)*levelsLeft);
        for (int i=0; i<levelsLeft; i++) newQueue[i] = queue[i+1];

        // Adopt the scoring of all child outcomes
        int futureScore = search(res->gridResult, newQueue, levelsLeft, hold).score;
        res->move.score = futureScore;

        PyMem_Free(newQueue); // Done w/ this
    }

    destroy(res);
    return o;
}

static moveResult *makePlay(BoardObject *board, int x, int r, int p) {
    // Pull score
    BoardObject *results = Board_genHypoBoard(p, x, r, board);
    // BoardObject *results = (BoardObject *) Board_copy(board, NULL);
    if (results == NULL) return NULL;

    int score = Board_calcScore(results);

    moveResult *o = PyMem_Malloc(sizeof(moveResult));
    moveset move = {.score = score, .targetRot = r, .targetX = x};
    o->move = move;
    o->gridResult = results;

    return o;
}