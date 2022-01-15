import time
import numpy as np

grid = np.zeros(shape=(10, 20), dtype=np.uint8)

start = time.time()

for i in range(160):
    cp = np.copy(grid)

print(time.time() - start)