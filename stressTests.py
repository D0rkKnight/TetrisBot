import time
import numpy as np
import ctypes
import tetrisCore

tetrisCore.init()

board = tetrisCore.Board()
print(board.getHeight())
print(board.asString())
for p in range(7):
    d = tetrisCore.getPieceDim(p)
    for y in range(d):
        o = ''
        for x in range(d):
            o += str(tetrisCore.getPieceBit(p, x, d-y-1))
        print(o)
    print()

print("All instructions executed!")