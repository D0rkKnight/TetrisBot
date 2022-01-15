# Tetris sim for testing and AI training purposes

import pygame
import numpy as np
import random
from pynput import keyboard
import collections
import time

stackSize = (10, 20)
grid = np.zeros(shape=stackSize, dtype=np.uint8)

I = 0; J = 1; L = 2; O = 3; S = 4; T = 5; Z = 6

baseRot =  [np.array([[0, 1, 0, 0],
                        [0, 1, 0, 0],
                        [0, 1, 0, 0],
                        [0, 1, 0, 0]]),
            np.array([[1, 1, 0],
                        [0, 1, 0],
                        [0, 1, 0]]),
            np.array([[0, 1, 0],
                        [0, 1, 0],
                        [1, 1, 0]]),
            np.array([[1, 1],
                        [1, 1]]),
            np.array([[0, 1, 0],
                        [1, 1, 0],
                        [1, 0, 0]]),
            np.array([[0, 1, 0],
                        [1, 1, 0],
                        [0, 1, 0]]),
            np.array([[1, 0, 0],
                        [1, 1, 0],
                        [0, 1, 0]])
]

rotVars = []

spawnOffset = [-1, -1, -1, 0, -1, -1, -1]

currentPiece = None
piecePos = None
pieceRot = 0
bag = []
queue = []
hold = ''
nextDelta = [0, 0]

hardDropQueued = False
lockQueued = False
holdQueued = False

forcePiece = None # Hijacking purposes

def init(cbI=None, cbL=None):
    initSim()

    if cbI is not None:
        cbI()

    # Hook input
    listener = keyboard.Listener(on_press=onPress)
    listener.start()

    pygame.init()

    screen = pygame.display.set_mode((600, 800))

    running = True
    while running:

        # Exit condition
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False

        screen.fill((50, 50, 50))

        # Run sim
        runSim()

        # Draw stack
        stackOff = (50, 50)
        squareSize = (30, 30)

        for x in range(grid.shape[0]):
            for y in range(grid.shape[1]):
                sy = stackOff[1] + (grid.shape[1]-y-1) * squareSize[1]
                sx = stackOff[0] + x * squareSize[0]

                cell = grid[x][y]
                color = (0, 0, 0)

                # Draw controlled piece too
                piece = getPGridFromID(currentPiece, pieceRot)

                px = x - piecePos[0]
                py = y - piecePos[1]

                if px >= 0 and px < piece.shape[0] and py >= 0 and py < piece.shape[1] and piece[px, py] > 0:
                    color = (255, 255, 255)
                

                if cell > 0:
                    color = (255, 255, 255)

                pygame.draw.rect(screen, color, (sx, sy, squareSize[0], squareSize[1]))

        # Draw queue
        queueAnch = (400, 50)
        gap = 10
        for i in range(len(queue)):
            qpAnch = (queueAnch[0], queueAnch[1] + i * squareSize[1]*4 + i*gap)
            piece = getPGridFromID(queue[i], 0)

            # Draw piece in queue
            for x in range(piece.shape[0]):
                for y in range(piece.shape[1]):
                    ly = piece.shape[1] - y - 1 # Flip the icon
                    if piece[x, ly] > 0:
                        sx = qpAnch[0] + x * squareSize[0]
                        sy = qpAnch[1] + y * squareSize[1]

                        pygame.draw.rect(screen, (255, 255, 255), (sx, sy, squareSize[0], squareSize[1]))

        # Draw hold
        if hold != '':
            holdAnch = (400, 500)
            holdPiece = getPGridFromID(hold, 0)

            for x in range(holdPiece.shape[0]):
                for y in range(holdPiece.shape[1]):
                    ly = holdPiece.shape[1] - y - 1 # Flip the icon
                    if holdPiece[x, ly] > 0:
                        sx = holdAnch[0] + x * squareSize[0]
                        sy = holdAnch[1] + y * squareSize[1]

                        pygame.draw.rect(screen, (255, 255, 255), (sx, sy, squareSize[0], squareSize[1]))

        pygame.display.update()

        if cbL is not None:
            cbL()

def initSim():
    global currentPiece, queue, piecePos
    currentPiece = drawPiece()
    piecePos = genPieceSpawn(currentPiece)

    regenerateQueue()

    # Flip pieces and initiallize piece rotations
    for i in range(len(baseRot)):
        bp = baseRot[i]
        bp = np.flip(bp, axis=1)
        baseRot[i] = bp

        o = []

        for r in range(4):
            variant = np.rot90(bp, r, axes=(1, 0))
            o.append(variant)
        
        rotVars.append(o)
    
    return

def runSim():
    global nextDelta, hardDropQueued, lockQueued, grid, hold, currentPiece, pieceRot, piecePos, holdQueued

    pGrid = getPGridFromID(currentPiece, pieceRot)

    if hardDropQueued:
        executeHardDrop(pGrid, grid, piecePos[0])
        hardDropQueued = False

    moveTo(pGrid, (piecePos[0]+nextDelta[0], piecePos[1]+nextDelta[1]))

    # Wipe deltas
    nextDelta = [0, 0]

    # Lock piece
    if lockQueued:
        lockQueued = False
        grid = lockPiece(pGrid, piecePos, grid)

    # Holds
    if holdQueued:
        if hold != '':
            o = hold
            hold = currentPiece
            currentPiece = o
        else:
            hold = currentPiece
            currentPiece = queue.pop(0)
            regenerateQueue()
        piecePos = genPieceSpawn(currentPiece)
        pieceRot = 0

        holdQueued = False

    return

def getPGridFromID(id, rot):
    pGrid = rotVars[id][rot]
    return pGrid

def executeHardDrop(pGrid, board, x):
    global lockQueued

    # Find lowest valid position
    y = board.shape[1] - pGrid.shape[1] # Starts from the top
    conflict = hasConflict(pGrid, (x, y), board)

    # Abort if current spot is taken 
    if (conflict):
        return None

    while not conflict:
        conflict = hasConflict(pGrid, (x, y-1), board)

        if not conflict:
            y -= 1
    
    nextDelta[1] = y - piecePos[1]
    lockQueued = True

def onPress(key):
    global hardDropQueued, pieceRot, holdQueued
    #print(key)

    if key == keyboard.Key.left:
        shiftBy(-1)
    if key == keyboard.Key.right:
        shiftBy(1)
    
    if key == keyboard.Key.space:
        hardDrop()
    
    if key == keyboard.Key.up:
        rotateBy(1)
    try:
        if key.char == 'z':
            rotateBy(-1)
        if key.char == 'c':
            holdQueued = True
    except AttributeError:
        # Don't do anything
        return

# Controls ---------

def shiftBy(amount):
    nextDelta[0] = amount

def hardDrop():
    global hardDropQueued
    hardDropQueued = True

def rotateBy(turns):
    global pieceRot
    pieceRot = (pieceRot + turns) % 4

def holdPiece():
    global holdQueued
    holdQueued = True

# -------------------

def moveTo(pGrid, newPos):
    global piecePos

    if not hasConflict(pGrid, newPos, grid):
        piecePos = newPos

def lockPiece(pGrid, pos, board):
    global piecePos, pieceRot, currentPiece

    for x in range(pGrid.shape[0]):
        for y in range(pGrid.shape[1]):
            if pGrid[x, y] > 0:
                board[pos[0] + x, pos[1] + y] = pGrid[x, y]
    
    currentPiece = queue.pop(0)
    regenerateQueue()
    piecePos = genPieceSpawn(currentPiece)

    pieceRot = 0

    lc = lineClear(board)

    return lc['board']

def drawPiece():
    global currentPiece, bag

    if len(bag) == 0:
        for i in range(7):
            bag.append(i)
        random.shuffle(bag)
    
    o = bag.pop(0)

    if forcePiece is not None:
        return forcePiece

    return o

def regenerateQueue():
    while len(queue) < 3:
        queue.append(drawPiece())

def genPieceSpawn(piece):
    return (grid.shape[0] // 2 - 1 + spawnOffset[piece], grid.shape[1] - baseRot[piece].shape[1])

def hasConflict(grid, pos, space):
    for x in range(grid.shape[0]):
        for y in range(grid.shape[1]):

            if (grid[x, y] > 0):
                 
                nx = pos[0] + x
                ny = pos[1] + y
                if (nx < 0 or nx >= space.shape[0]):
                    return True
                if (ny < 0 or ny >= space.shape[1]):
                    return True
                if (space[nx, ny] > 0):
                    return True

    return False

def lineClear(grid, sat=None):

    # Test to see if grid traversal is necessary
    if sat is not None:
        hasLC = False
        for i in sat:
            if i >= grid.shape[0]:
                hasLC = True
                break

        if not hasLC:
            return {'board' : grid, 'clears' : 0, 'lines' : []}

    postClear = np.zeros(shape=grid.shape, dtype=np.uint8)
    linesCleared = 0
    lines = []

    for y in range(grid.shape[1]):
        fullRow = True

        for x in range(grid.shape[0]):
            fullRow = grid[x, y] > 0 and fullRow
        
        if fullRow:
            linesCleared += 1
            lines.append(y)
        else:
            # put line into post clear grid
            for x in range(grid.shape[0]):
                postClear[x, y - linesCleared] = grid[x, y]

    return {'board' : postClear, 'clears' : linesCleared, 'lines' : lines}

if __name__ == '__main__':
    init(None)