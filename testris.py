import time
import tracemalloc
import tetrisCore
import yep

tracemalloc.start(25)
tetrisCore.init()

board = tetrisCore.Board()
# board2 = board.copy()

# res = board.search([1, 2], 4)
# print(res[0])
# print(res[1])
# print(res[2])

# Board print test
# for x in range(10):
#     for y in range(20):
#         board2.set((x+y)%2, x, y)

# print(board)
# print(board2)

yep.start("d3search.prof")

tetrisCore.setBag([0, 5, 6])
board.search([1, 2, 3, 4], 4)

yep.stop()

print("All instructions executed!")

snapshot = tracemalloc.take_snapshot()
top_stats = snapshot.statistics('lineno')

print("\n[ Top 10 ]")
for stat in top_stats[:10]:
    print(stat)

print("\nProgram finished")
