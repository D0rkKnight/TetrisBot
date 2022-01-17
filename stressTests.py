import time
import numpy as np
import ctypes
import tetrisCore

board = tetrisCore.Board()
print(board.getHeight())
print(board.asString())

print("All instructions executed!")