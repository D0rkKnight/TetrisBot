import time
import tetrisCore

tetrisCore.init()

board = tetrisCore.Board()

res = board.search([1, 2], 4)
print(res[0])
print(res[1])
print(res[2])

print("All instructions executed!")