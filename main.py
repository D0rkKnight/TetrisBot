import win32gui
import win32con
import win32api
import numpy as np
from PIL import ImageGrab
import collections
import tetrisSim

import keyboard

import time

started = False
tilde = 'Sc029'
topMargin = 3
latency = 0.1 
hardDropLatency = 0.4 # Queue needs time to update, for example
lineclearLatency = 1
holdLatency = 0.2
latencyOn = False
ARD = 0.17
ARS = 0.05
ARC = 0.1 # compensation

desktop = win32gui.GetDesktopWindow()
screenDims = (1920.0, 1080.0)
firstBlock = True
currentPiece = ''
hold = ''
currentBoard = None
logData = False

INTERNAL_SIM = 0
SCREEN_CAPTURE = 1
source = INTERNAL_SIM
hasInputLag = False

# Build piece library
baseRot = tetrisSim.baseRot
spawnOffset = tetrisSim.spawnOffset

MoveInput = collections.namedtuple('MoveInput', 'delta rot board score lineclears piece')

def main():
    global currentPiece, currentBoard, hold

    if source == INTERNAL_SIM:
        tetrisSim.init(calculate)

def calculate():
    global currentBoard, currentPiece

    if keyboard.is_pressed('esc'):
        return
    
    global started
    if keyboard.is_pressed('`'):
        started = True

    # Screengrab for positioning
    if keyboard.is_pressed('1'):
        img = pollScreen() 
        img.show()

    if not started:
        return
    
    grid = None; queue = None

    if source == SCREEN_CAPTURE:
        # Game code loop
        img = pollScreen()
        #img.show()

        # Grab grid contents
        xPad = 300
        blockStack = img.crop((xPad, 50, img.width-xPad, img.height-50))

        gridDims = (10, 20)
        grid = pixelsToGrid(blockStack, gridDims, 50)
        blockSize = blockStack.width/float(gridDims[0])

        # Flip and format grid
        grid = np.flip(grid, 1)
        grid = grid[:grid.shape[0], :grid.shape[1]-topMargin] # Trim the board as to not capture the falling piece

        # Read Queue
        queue = readQueue(img, blockSize)
    
    elif source == INTERNAL_SIM:
        grid = np.copy(tetrisSim.grid)
        queue = np.copy(tetrisSim.queue)

    # Check for misinputs
    if (currentBoard is not None) and not np.array_equal(grid, currentBoard):
        log("MISINPUT: Delta between the following boards. 1: detected board 2: expected board")
        logGrid(grid)
        logGrid(currentBoard)
        log()

    # Place a block
    global firstBlock 
    if firstBlock:
        hardDrop()
        currentPiece = queue[0]
        firstBlock = False
        return
    
    if hold == '':
        holdPiece(currentPiece)
        currentPiece = queue[0]
        return

    # Position block
    # inputs = chainMove({'queue' : [currentPiece, queue[0]], 'hold' : hold}, grid)
    inputs = chainMove({'queue' : [currentPiece], 'hold' : hold}, grid)

    if inputs.piece != currentPiece: # Means it was swapped for a hold
        log("Current piece before hold: " + str(currentPiece))
        log("Held piece before hold: " + str(hold))
        holdPiece(currentPiece)
        log("")
    
    # Print queue
    for i in range(len(queue)):
        log('Element ' + str(i) + ' of queue is ' + str(queue[i]))

    move(inputs.delta, inputs.rot)
    hardDrop()

    if inputs.lineclears > 0:
        wait(lineclearLatency)

    currentPiece = tetrisSim.currentPiece
    currentBoard = inputs.board

    report(inputs)

def pollScreen():
    # Capture entire playspace
    taskBarW = 65
    xPad = 0.25
    yPad = 0.15
    left = (screenDims[0] - taskBarW) * xPad + taskBarW
    right = (screenDims[0] - taskBarW) * (1-xPad) + taskBarW
    top = screenDims[1] * yPad
    bottom = screenDims[1] * (1-yPad)

    img = ImageGrab.grab((left, top, right, bottom))

    return img

def pixelsToGrid(img, gDims, cutoff):
    grid = np.zeros(shape=gDims, dtype=np.uint8)
    for x in range(gDims[0]):
        for y in range(gDims[1]):
            scrnCord = ((float(x) + 0.5) / gDims[0] * img.width, 
                        (float(y) + 0.5) / gDims[1] * img.height)
            pixel = img.getpixel(scrnCord)

            if (sum(pixel) > cutoff):
                grid[x, y] = 1
    return grid

def printGrid(grid):
    print()
    shp = grid.shape
    for y in range(shp[1]):
        y = grid.shape[1]-y- 1
        line = ''
        for x in range(shp[0]):
            o = " "
            if grid[x, y] > 0:
                o = "O"
            line += o
        print(line)

def readQueue(img, blockSize, log=False):
    # Grab queue contents
    upperLeft = (735, 145)
    padding = 20
    gQ = []
    for i in range(3):
        q = img.crop((upperLeft[0], upperLeft[1] + blockSize*2*i + padding * i, 
                    upperLeft[0] + blockSize*3, upperLeft[1] + blockSize*2*(i+1) + padding * i))
        qGrid = pixelsToGrid(q, (3, 2), 120)
        gQ.append(qGrid)
    
    # Extract piece type
    pQueue = []
    for g in gQ:
        square = np.hstack([g, np.zeros(shape=(g.shape[0], 1), dtype=np.uint8)])
        
        selected = False
        
        for key in baseRot.keys():
            if (np.array_equal(square, baseRot[key])):
                pQueue.append(key)
                selected = True
                break
        
        if np.array_equal(square, [[0, 0, 0], [1, 1, 0], [1, 1, 0]]):
            pQueue.append('O')
            selected = True

        if not selected: 
            pQueue.append('I')
    return pQueue

def hardDrop():
    if source == SCREEN_CAPTURE:
        ping('Space')
        
    elif source == INTERNAL_SIM:
        tetrisSim.hardDrop()
        tetrisSim.runSim()
    
    wait(hardDropLatency)
    return

def holdPiece(piece):
    global hold, currentPiece
    o = hold
    hold = piece
    currentPiece = o

    if source == SCREEN_CAPTURE:
        ping('c')
    elif source == INTERNAL_SIM:
        tetrisSim.holdPiece()
        tetrisSim.runSim()
    
    wait(holdLatency)
    
    return o

def move(delta, rot):
    if source == SCREEN_CAPTURE:
        if rot == 1:
            ping('up')
        if rot == 2:
            ping('up')
            ping('up')
        if rot == 3:
            ping('z')

        if delta != 0:
            key = 'right'
            if delta < 0:
                key = 'left'
            
            absDelta = abs(delta)
            while absDelta > 0:
                ping(key)
                absDelta -= 1
    
    elif source == INTERNAL_SIM:
        # Force game ticks
        if rot == 1:
            tetrisSim.rotateBy(1)
            tetrisSim.runSim()
        elif rot == 2:
            tetrisSim.rotateBy(2)
            tetrisSim.runSim()
        elif rot == 3:
            tetrisSim.rotateBy(-1)
            tetrisSim.runSim()

        if delta != 0:
            tetrisSim.shiftBy(delta)
            tetrisSim.runSim()
        

def ping(key):
    keyboard.press_and_release(key)
    wait(latency)

def chainMove(inHand, board):
    hand = {'queue' : inHand['queue'].copy(), 'hold' : inHand['hold']}
    nextMove = hand['queue'].pop(0)


    # Make an arbitrary move

    if len(hand['queue']) > 0:
        pGrid = baseRot[nextMove]
        pGrid = np.flip(pGrid, 1) # Flip piece as well, so it is in the 1st quadrant

        maxScore = -100000
        bestSpot = 0
        bestRot = 0
        bestBoard = None
        blc = None


        for x in range(-2, board.shape[0]):
            for r in range(4):
                lc = genHypoBoard(board, pGrid, x, r)
                if lc is None:
                    continue
                hypoBoard = lc['board']

                # Create alternate queue that has a hold swap
                newHold = hand['queue'][0]
                newQueue = []
                for i in range(len(hand['queue'])):
                    if i == 0:
                        newQueue.append(hand['hold'])
                    else:
                        newQueue.append(hand['queue'][i])
                newHand = {'queue' : newQueue, 'hold' : newHold}

                # Now that the board state is resolved, bind score outcome to child score outcome
                nhOutcome = chainMove(newHand, hypoBoard)
                nhOutcome.piece = newHand['queue'][0] # Update active piece so the receiver knows which choice led to here

                ohOutcome = chainMove(hand, hypoBoard)
                ohOutcome.piece = hand['queue'][0]


                # Boilerplate lifted from elsewhere
                if (nhOutcome.score > maxScore or ohOutcome.score > maxScore):
                    maxScore = max(maxScore, nhOutcome.score, ohOutcome.score)
                    bestSpot = x
                    bestRot = r
                    bestBoard = hypoBoard
                    blc = lc

        spawnPos = board.shape[0] // 2 - 1 + spawnOffset[nextMove]

        delta = bestSpot - spawnPos

        lineClears = 0
        if blc is not None:
            lineClears = blc['clears']
                
        return MoveInput(delta, bestRot, bestBoard, maxScore, lineClears, nextMove)
    else:
        # Means that we are at end of the queue, just score like normal
        # Just scoring, not actually updating the hold
        s1 = positionPiece(nextMove, board)
        s2 = positionPiece(hand['hold'], board)
        if  s1.score > s2.score:
            return s1
        return s2

def positionPiece(piece, board):
    pGrid = baseRot[piece]
    pGrid = np.flip(pGrid, 1) # Flip piece as well, so it is in the 1st quadrant

    # Try it in every location for validity
    maxScore = -100000
    bestSpot = 0
    bestRot = 0
    bestBoard = None
    blc = None
    for x in range(-2, board.shape[0]):

        # Rotations as well
        for r in range(4):
            lc = genHypoBoard(board, pGrid, x, r)
            if lc is None:
                continue
            hypoBoard = lc['board']

            # Generate ridge
            ridge = genRidge(hypoBoard)

            score = 0
            score += testPeaks(hypoBoard, ridge) * 1
            score += testHoles(hypoBoard) * 5
            score += testPits(hypoBoard, ridge)
            score += stackBuilder(lc['clears']) * 8
            score += tetrisFinder(lc['clears']) * 40

            if score > maxScore:
                maxScore = score
                bestSpot = x
                bestRot = r
                bestBoard = hypoBoard
                blc = lc
    
    # Calculate deltas from the best spot
    spawnPos = board.shape[0] // 2 - 1 + spawnOffset[piece]
    delta = bestSpot - spawnPos

    lineClears = 0
    if blc is not None:
        lineClears = blc['clears']

    return MoveInput(delta, bestRot, bestBoard, maxScore, lineClears, piece)

def genHypoBoard(board, pGrid, x, r):
    rotGrid = np.rot90(pGrid, r, axes=(1, 0))

    # Find lowest valid position
    y = board.shape[1] - rotGrid.shape[1] # Starts from the top
    conflict = hasConflict(rotGrid, (x, y), board)

    # Abort if current spot is taken 
    if (conflict):
        return None

    while not conflict:
        conflict = hasConflict(rotGrid, (x, y-1), board)

        if not conflict:
            y -= 1

    # Create hypothetical board
    hypoBoard = np.copy(board)
    for lx in range(rotGrid.shape[0]):
        for ly in range(rotGrid.shape[1]):
            if (rotGrid[lx, ly] > 0):
                hypoBoard[x+lx, y+ly] = rotGrid[lx, ly]

    lc = lineClear(hypoBoard)
    return lc

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

def lineClear(grid):
    postClear = np.zeros(shape=grid.shape, dtype=np.uint8)
    linesCleared = 0

    for y in range(grid.shape[1]):
        fullRow = True

        for x in range(grid.shape[0]):
            if grid[x, y] == 0:
                fullRow = False
            #fullRow = grid[x, y] > 0 and fullRow
        
        if fullRow:
            linesCleared += 1
        else:
            # put line into post clear grid
            for x in range(grid.shape[0]):
                postClear[x, y - linesCleared] = grid[x, y]

    return {'board' : postClear, 'clears' : linesCleared}

def genRidge(grid):
    o = []
    for col in grid:
        i = len(col)-1
        while col[i] == 0 and i > 0:
            i -= 1
    
        o.append(i)
    return o

# Evaluate score on the highest points
def testPeaks(grid, ridge):
    peak = max(ridge)
    return grid.shape[1] - peak

def testHoles(grid):
    holes = 0
    for col in grid:
        covered = False

        # Descend
        for i in range(len(col)):
            y = len(col) - i - 1
            if col[y] > 0:
                covered = True
            
            if covered and col[y] == 0:
                holes += 1
    
    o = -holes

    return o

def testPits(grid, ridge):
    o = 0

    for i in range(1, len(ridge)):
        prevAlt = ridge[i-1]
        nextAlt = ridge[i]

        o -= abs(nextAlt-prevAlt)

    return o

def stackBuilder(lineClears):
    if 0  < lineClears < 4:
        return -1
    return 0

def tetrisFinder(lineClears):
    if lineClears == 4:
        return 1
    return 0

def report(data):
    log('delta: '+str(data.delta))
    log('rot: '+str(data.rot))
    log(data.board)
    log('^ Board should be')
    log('Piece: '+str(data.piece))
    log("Hold is: " + str(hold))
    log('_'*20)

def log(str=""):
    if logData:
        print(str)

def logGrid(grid):
    if logData:
        printGrid(grid)

def wait(amt):
    if latencyOn:
        time.sleep(amt)

main()