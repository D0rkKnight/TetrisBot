import time
import tetrisCore

tetrisCore.init()

board = tetrisCore.Board()
board2 = board.copy()

# res = board.search([1, 2], 4)
# print(res[0])
# print(res[1])
# print(res[2])

# Board print test
for x in range(10):
    for y in range(20):
        board2.set((x+y)%2, x, y)

print(board)
print(board2)


print("All instructions executed!")