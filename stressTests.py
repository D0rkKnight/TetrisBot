import time
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

# for x in range(10):
#     board.set(1, x, 10)
#     print(board.get(x, 10))

# print(board)
# b2 = board.copy()
# print(b2.w)
# print(b2)

# Try filling some memory
# bs = []
# for i in range(1000):
#     bs.append(tetrisCore.Board())
#     print(i)

# bs[500].get(0, 11)

# test ridges
for i in range(board.w):
    board.setRidge(1, i)

for i in range(board.h):
    board.setSat(5, i)

b2 = board.copy()

# for i in range(board.w):
#     print(b2.getRidge(i))

# for i in range(board.h):
#     print(b2.getSat(i))

print("All instructions executed!")