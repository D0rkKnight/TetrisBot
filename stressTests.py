import time
import numpy as np
import ctypes
import tetrisCore

tetrisCore.init()

board = tetrisCore.Board()

# Print out piece data
# for p in range(7):
#     d = tetrisCore.getPieceDim(p)
#     for y in range(d):
#         o = ''
#         for x in range(d):
#             o += str(tetrisCore.getPieceBit(p, x, d-y-1))
#         print(o)
#     print()

for x in range(10):
    board.set(1, x, 10)
    print(board.get(x, 10))

print(board)
b2 = board.copy()
print(b2.w)
print(b2)

print("All instructions executed!")