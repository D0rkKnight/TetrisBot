import time
import numpy as np
import ctypes
import tetrisCore

grid = np.zeros(shape=(10, 20), dtype=np.uint8)

start = time.time()

for i in range(1600):
    cp = np.copy(grid)

print(time.time() - start)

tetrisCore.hook()
sum = tetrisCore.add(5, 4)
print(sum)

tetrisCore.system("dir")
print("All instructions executed!")