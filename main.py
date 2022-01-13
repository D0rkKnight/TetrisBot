import win32gui
import win32con
import win32api
import numpy as np
from PIL import ImageGrab

import keyboard

import time

started = False
tilde = 'Sc029'
topMargin = 3
latency = 0.1 
hardDropLatency = 0.4 # Queue needs time to update, for example
lineclearLatency = 1
ARD = 0.17
ARS = 0.05
ARC = 0.1 # compensation

desktop = win32gui.GetDesktopWindow()
screenDims = (1920.0, 1080.0)
firstBlock = True
currentPiece = ''
hold = ''
currentBoard = None

# Build piece library
pieces = {}
baseRot =  {'I' : [[0, 1, 0, 0],
                  [0, 1, 0, 0],
                  [0, 1, 0, 0],
                  [0, 1, 0, 0]],
            'J' : [[1, 1, 0],
                  [0, 1, 0],
                  [0, 1, 0]],
            'L' : [[0, 1, 0],
                  [0, 1, 0],
                  [1, 1, 0]],
            'O' : [[1, 1],
                  [1, 1]],
            'S' : [[0, 1, 0],
                  [1, 1, 0],
                  [1, 0, 0]],
            'T' : [[0, 1, 0],
                  [1, 1, 0],
                  [0, 1, 0]],
            'Z' : [[1, 0, 0],
                  [1, 1, 0],
                  [0, 1, 0]]
        }
spawnOffset = {'I' : -1, 'J' : -1, 'L' : -1, 'O' : 0, 'S' : -1, 'T' : -1, 'Z' : -1}


def main():
    global currentPiece, currentBoard, hold

    while True:
        if keyboard.is_pressed('esc'):
            break
        
        global started
        if keyboard.is_pressed('`'):
            started = True

        if not started:
            continue

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

        # Check for misinputs
        if (currentBoard is not None) and not np.array_equal(grid, currentBoard):
            print("MISINPUT: Delta between the following boards. 1: detected board 2: expected board")
            printGrid(grid)
            printGrid(currentBoard)
            print()

        # Read Queue
        queue = readQueue(img, blockSize)

        # Place a block
        global firstBlock 
        if firstBlock:
            hardDrop()
            currentPiece = queue.pop(0)
            firstBlock = False
            continue
        
        if hold == '':
            holdPiece(currentPiece)
            currentPiece = queue.pop(0)
            continue

        # Position block
        # inputsMain = chainMove({'queue' : [currentPiece], 'hold' : hold}, grid)
        # inputsAlt = chainMove({'queue' : [hold], 'hold' : currentPiece}, grid)
        inputs = chainMove({'queue' : [currentPiece, queue[0]], 'hold' : hold}, grid)

        if inputs['piece'] != currentPiece: # Means it was swapped for a hold
            holdPiece(currentPiece)
            # inputs = inputsAlt

        move(inputs['delta'], inputs['rot'])
        hardDrop()

        if inputs['lineclears'] > 0:
            time.sleep(lineclearLatency)

        currentPiece = queue.pop(0)
        currentBoard = inputs['board']

        report(inputs)
        
    return

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
        if log:
            printGrid(square)
        
        selected = False
        
        for key in baseRot.keys():
            if (np.array_equal(square, baseRot[key])):
                pQueue.append(key)
                if log:
                    print(key)
                selected = True
                break
        
        if np.array_equal(square, [[0, 0, 0], [1, 1, 0], [1, 1, 0]]):
            pQueue.append('O')
            if log:
                print('O')
            selected = True

        if not selected: 
            pQueue.append('I')
            if log:
                print('I')
    return pQueue

def hardDrop():
    ping('Space')
    time.sleep(hardDropLatency)
    return

def holdPiece(piece):
    global hold
    o = hold
    hold = piece

    ping('c')
    return o

def move(delta, rot):
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
        

def ping(key):
    keyboard.press_and_release(key)
    time.sleep(latency)

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

            # Rotations as well
            for r in range(4):
                rotGrid = np.rot90(pGrid, r, axes=(1, 0))
                # printGrid(rotGrid)

                # Find lowest valid position
                y = board.shape[1] - rotGrid.shape[1] # Starts from the top
                conflict = hasConflict(rotGrid, (x, y), board)

                # Abort if current spot is taken 
                if (conflict):
                    continue

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
                nhOutcome['piece'] = newHand['queue'][0] # Update active piece so the receiver knows which choice led to here

                ohOutcome = chainMove(hand, hypoBoard)
                ohOutcome['piece'] = hand['queue'][0]


                # Boilerplate lifted from elsewhere
                if (nhOutcome['score'] > maxScore or ohOutcome['score'] > maxScore):
                    maxScore = max(maxScore, nhOutcome['score'], ohOutcome['score'])
                    bestSpot = x
                    bestRot = r
                    bestBoard = hypoBoard
                    blc = lc

        spawnPos = board.shape[0] / 2 - 1 + spawnOffset[nextMove]
        delta = bestSpot - spawnPos

        lineClears = 0
        if blc is not None:
            lineClears = blc['clears']
                
        return {'delta' : delta, 'rot' : bestRot, 'board' : bestBoard, 'score' : maxScore, 'lineclears' : lineClears, 'piece' : nextMove}
    else:
        # Means that we are at end of the queue, just score like normal
        # Just scoring, not actually updating the hold
        s1 = positionPiece(nextMove, board)
        s2 = positionPiece(hand['hold'], board)
        if  s1['score'] > s2['score']:
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
            rotGrid = np.rot90(pGrid, r, axes=(1, 0))
            # printGrid(rotGrid)

            # Find lowest valid position
            y = board.shape[1] - rotGrid.shape[1] # Starts from the top
            conflict = hasConflict(rotGrid, (x, y), board)

            # Abort if current spot is taken 
            if (conflict):
                # print('Conflict')
                # print('X at conflict: ' + str(x))
                # print('Y at conflict: ' + str(y))
                continue

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
            hypoBoard = lc['board']
            
            #printGrid(hypoBoard)
            #if (piece == 'I' and (r == 1 or r == 3)):
            #    print("Vertical I piece!")
            #    time.sleep(1000)

            # Generate ridge
            ridge = genRidge(hypoBoard)

            score = 0
            score += testPeaks(hypoBoard, ridge)
            score += testHoles(hypoBoard) * 5
            score += testPits(hypoBoard, ridge)
            score += stackBuilder(lc['clears']) * 5
            score += tetrisFinder(lc['clears']) * 40

            if score > maxScore:
                maxScore = score
                bestSpot = x
                bestRot = r
                bestBoard = hypoBoard
                blc = lc
    
    # Calculate deltas from the best spot
    spawnPos = board.shape[0] / 2 - 1 + spawnOffset[piece]
    delta = bestSpot - spawnPos

    lineClears = 0
    if blc is not None:
        lineClears = blc['clears']
    return {'delta' : delta, 'rot' : bestRot, 'board' : bestBoard, 'score' : maxScore, 'lineclears' : lineClears, 'piece' : piece}

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

    #if (linesCleared > 0):
        #print("Lines cleared: " + str(linesCleared))
         #printGrid(postClear)

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
    print('delta: '+str(data['delta']))
    print('rot: '+str(data['rot']))
    printGrid(data['board'])
    print('Piece: '+data['piece'])
    print('^ Board should be')
    print("Hold is: " +  hold)
    print('_'*20)

main()