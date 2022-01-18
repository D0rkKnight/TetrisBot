import numpy as np
import collections
import tetrisSim
import tetrisVision
import tetrisCore

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

searchDepth = 2

firstBlock = True
logData = False

INTERNAL_SIM = 0
SCREEN_CAPTURE = 1
source = INTERNAL_SIM
hasInputLag = False

MoveInput = collections.namedtuple('MoveInput', 'delta rot board score iScore lineclears piece')

tetroMarginBuffer = []
tetroRidgeBuffer = []

def main():
    if source == INTERNAL_SIM:
        tetrisSim.init(init, calculate)

def init():
        # Generate marginal buffers
    for piece in tetrisSim.rotVars:
        pieceMarginBuff = []
        pieceRidgeBuff = []
        for rot in piece:
            rotMarginBuff = []
            rotRidgeBuff = []
            for x in range(rot.shape[0]):

                # March upwards
                alt = -1 # buffer in distance, -1 means no cell in the column.

                for y in range(rot.shape[1]):
                    if rot[x, y] > 0:
                        alt = y
                        break
                
                rotMarginBuff.append(alt)


                # March downwards for ridge counting
                rAlt = -1
                for y in range(rot.shape[1]):
                    ly = rot.shape[1] - y - 1
                    if rot[x, ly] > 0:
                        rAlt = ly + 1
                        break
                
                rotRidgeBuff.append(rAlt)
            
            pieceMarginBuff.append(rotMarginBuff)
            pieceRidgeBuff.append(rotRidgeBuff)
        
        tetroMarginBuffer.append(pieceMarginBuff)
        tetroRidgeBuffer.append(pieceRidgeBuff)

def calculate():
    if keyboard.is_pressed('esc'):
        return
    
    global started
    if keyboard.is_pressed('`'):
        started = True

    # Screengrab for positioning
    if keyboard.is_pressed('1'):
        img = tetrisVision.pollScreen() 
        img.show()

    if not started:
        return
    
    grid = None; queue = None

    if source == SCREEN_CAPTURE:
        # Game code loop
        img = tetrisVision.pollScreen()
        #img.show()

        # Grab grid contents
        xPad = 300
        blockStack = img.crop((xPad, 50, img.width-xPad, img.height-50))

        gridDims = (10, 20)
        grid = tetrisVision.pixelsToGrid(blockStack, gridDims, 50)
        blockSize = blockStack.width/float(gridDims[0])

        # Flip and format grid
        grid = np.flip(grid, 1)
        grid = grid[:grid.shape[0], :grid.shape[1]-topMargin] # Trim the board as to not capture the falling piece

        print("NP grid incompatible with TetrisCore grid")
        time.sleep(1000)

        # Read Queue
        queue = tetrisVision.readQueue(img, blockSize)
    
    elif source == INTERNAL_SIM:
        grid = tetrisSim.grid.copy()
        queue = np.copy(tetrisSim.queue)

    

    # Check for misinputs
    # if (currentBoard is not None) and not np.array_equal(grid, currentBoard):
    #     log("MISINPUT: Delta between the following boards. 1: detected board 2: expected board")
    #     logGrid(grid)
    #     logGrid(currentBoard)
    #     log()

    # Place a block
    global firstBlock 
    if firstBlock:
        hardDrop()
        firstBlock = False
        return
    
    if tetrisSim.hold == '':
        holdPiece()
        return

    ridge = genRidge(grid)
    holes = findHoles(grid)
    sat = findLineSaturation(grid)

    searchQueue = [tetrisSim.currentPiece]
    for i in range(searchDepth-1):
        searchQueue.append(queue[i])

    # Position block
    
    start = time.time() # Speedtests
    
    # inputs = chainMove({'queue' : searchQueue, 'hold' : tetrisSim.hold}, 
    #       {'contents' : grid, 'ridge' : ridge, 'holes' : holes, 'saturation' : sat, 'iScore' : 0})
        
    # print(time.time() - start)

    # if inputs.piece != tetrisSim.currentPiece: # Means it was swapped for a hold
    #     log("Current piece before hold: " + str(tetrisSim.currentPiece))
    #     log("Held piece before hold: " + str(tetrisSim.hold))
    #     holdPiece()
    #     log("")
    
    # Print queue
    for i in range(len(queue)):
        log('Element ' + str(i) + ' of queue is ' + str(queue[i]))

    # Simple C algo override
    # Write ridge into board internal ridge
    for i in range(grid.w):
        grid.setRidge(ridge[i]+1, i)
    
    #print("\n\n\n current piece: "+str(tetrisSim.currentPiece))

    piece = searchQueue[0]
    xDest = grid.positionPiece(piece)

    # Calculate necessary shift
    spawnPos = tetrisSim.genPieceSpawn(piece)
    delta = xDest - spawnPos[0]

    #move(inputs.delta, inputs.rot)
    move(delta, 0)
    hardDrop()

    time.sleep(0.2)

    # if inputs.lineclears > 0:
    #     wait(lineclearLatency)

    # report(inputs)

def hardDrop():
    if source == SCREEN_CAPTURE:
        ping('Space')
        
    elif source == INTERNAL_SIM:
        tetrisSim.hardDrop()
        tetrisSim.runSim()
    
    wait(hardDropLatency)
    return

def holdPiece():
    if source == SCREEN_CAPTURE:
        ping('c')
    elif source == INTERNAL_SIM:
        tetrisSim.holdPiece()
        tetrisSim.runSim()
    
    wait(holdLatency)

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
        maxScore = -100000
        maxIScore = -100000
        bestSpot = 0
        bestRot = 0
        bestBoard = None
        blc = None

        for x in range(-2, board['contents'].w):
            for r in range(4):
                result = genHypoBoard(board, nextMove, x, r)
                if result is None:
                    continue
                hypoGrid = result['board']
                ridge = result['ridge']
                holes = board['holes'] + result['hole delta']
                nextBoard = {'contents' : hypoGrid, 'ridge' : ridge, 'holes' : holes, 'saturation' : result['saturation']
                            , 'iScore' : result['iScore']}

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
                nhOutcome = chainMove(newHand, nextBoard)
                ohOutcome = chainMove(hand, nextBoard)


                # Boilerplate lifted from elsewhere
                if nhOutcome.score + nhOutcome.iScore > maxScore + maxIScore:
                    maxScore = max(maxScore, nhOutcome.score)
                    maxIScore = max(maxIScore, nhOutcome.iScore)
                    bestSpot = x
                    bestRot = r
                    bestBoard = nextBoard
                    blc = result
                if ohOutcome.score + ohOutcome.iScore > maxScore + maxIScore:
                    maxScore = max(maxScore, ohOutcome.score)
                    maxIScore = max(maxIScore, ohOutcome.iScore)
                    bestSpot = x
                    bestRot = r
                    bestBoard = nextBoard
                    blc = result

        spawnPos = tetrisSim.genPieceSpawn(nextMove)
        delta = bestSpot - spawnPos[0]

        lineClears = 0
        if blc is not None:
            lineClears = blc['clears']
                
        return MoveInput(delta, bestRot, bestBoard, maxScore, maxIScore, lineClears, nextMove) 
        # Report that the move popped for this layer has this score

    else:
        # Means that we are at end of the queue, just score like normal
        # Just scoring, not actually updating the hold
        s1 = positionPiece(nextMove, board)
        s2 = positionPiece(hand['hold'], board)

        if  s1.score + s1.iScore > s2.score + s2.iScore:
            return s1
        return s2

def positionPiece(piece, board):

    # Try it in every location for validity
    maxScore = -100000
    maxIScore = -100000
    bestSpot = 0
    bestRot = 0
    bestBoard = None
    blc = None
    for x in range(-2, board['contents'].w):

        # Rotations as well
        for r in range(4):
            results = genHypoBoard(board, piece, x, r)
            if results is None:
                continue

            hypoBoard = results['board']

            # Generate ridge
            ridge = results['ridge']
            holes = results['hole delta'] + board['holes']

            score = 0
            score += testPeaks(ridge) * 0.5
            score += testHoles(holes) * -10
            score += testPits(ridge)
            
            score += tetrisFinder(results['clears']) * 40

            if score + results['iScore'] > maxScore + maxIScore:
                maxScore = score
                maxIScore = results['iScore']
                bestSpot = x
                bestRot = r
                bestBoard = {'contents' : hypoBoard, 'ridge' : ridge}
                blc = results
            
            
    
    # Calculate deltas from the best spot
    spawnPos = tetrisSim.genPieceSpawn(piece)
    delta = bestSpot - spawnPos[0]

    lineClears = 0
    if blc is not None:
        lineClears = blc['clears']
    
    

    return MoveInput(delta, bestRot, bestBoard, maxScore, maxIScore, lineClears, piece)

def genHypoBoard(board, piece, x, r):
    rotGrid = tetrisSim.rotVars[piece][r]
    bGrid = board['contents']
    ridge = board['ridge']
    sat = board['saturation']

    # Find lowest valid position
    # Estimate top of ridge
    y = -2
    conflict = False
    for i in range(rotGrid.shape[0]):
        col = x + i
        colMargin = tetroMarginBuffer[piece][r][i]
        if colMargin < 0: # Check that this column has cells at all
            continue

        if col < 0 or col >= bGrid.w:
            # Out of bounds!
            conflict = True
            break

        colTop = ridge[col] + 1 - colMargin

        y = max(y, colTop)
    
    # Abort if current spot is taken 
    if conflict or y > bGrid.h - rotGrid.shape[1]:
        return None

    # Create hypothetical board
    # Needed for future reading
    hypoBoard = bGrid.copy()
    newSat = np.copy(sat)
    for lx in range(rotGrid.shape[0]):
        for ly in range(rotGrid.shape[1]):
            if (rotGrid[lx, ly] > 0):
                hypoBoard.set(rotGrid[lx, ly], x+lx, y+ly)
                newSat[y+ly] += 1

    lc = tetrisSim.lineClear(hypoBoard, newSat) # TODO: This is causing a 10 ms slowdown


    # Counting holes
    holesMade = 0
    holesRemoved = 0

    # Update ridge
    newRidge = np.copy(board['ridge'])
    for i in range(rotGrid.shape[0]):
        ridgeDelta = tetroRidgeBuffer[piece][r][i]
        if ridgeDelta < 0:
            continue

        col = x + i

        # See if there is a gap in the old ridge and the new piece
        # Do before the new ridge is recalculated
        margin = tetroMarginBuffer[piece][r][i]
        holesMade += y - (newRidge[col] + 1) + margin

        # Update ridge since we already have the y value
        newRidge[col] = y + ridgeDelta - 1

    lines = lc['lines']
    for x in range(len(newRidge)):

        for y in range(len(lines)):
            i = len(lines) - y - 1
            if newRidge[x] >= lines[i]:
                newRidge[x] -= i + 1 # i being the number of line clears the ridge stack intersects with

                # Account for newly exposed gaps
                while newRidge[x] >= 0 and lc['board'].get(x, newRidge[x]) == 0:
                    newRidge[x] -= 1

                    # Every one of these means a hole has been cleared.
                    holesRemoved += 1

                break
    
    holeDelta = holesMade - holesRemoved

    lc['ridge'] = newRidge
    lc['hole delta'] = holeDelta
    lc['saturation'] = newSat

    iScore = board['iScore']
    iScore += stackBuilder(lc['clears']) * 7
    lc['iScore'] = iScore

    return lc

def npGridFromTCGrid(tcGrid):
    o = np.zeros(shape=(tcGrid.w, tcGrid.h), dtype=np.uint8)
    for x in range(tcGrid.w):
        for y in range(tcGrid.h):
            o[x][y] = tcGrid.get(x, y)
    
    return o

def genRidge(tcGrid):
    # Needs internal link
    grid = npGridFromTCGrid(tcGrid)

    o = []
    for col in grid:
        i = len(col)-1
        while col[i] == 0 and i >= 0:
            i -= 1
    
        o.append(i)
    return o

# Evaluate score on the highest points
def testPeaks(ridge):
    peak = max(ridge)
    return -peak

def findHoles(tcGrid):
    grid = npGridFromTCGrid(tcGrid)

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

def findLineSaturation(tcGrid):
    grid = npGridFromTCGrid(tcGrid)

    o = []
    for y in range(grid.shape[1]):
        i = 0
        for x in range(grid.shape[0]):
            if grid[x, y] > 0:
                i += 1
        o.append(i)
    
    return o

# Doesn't do anything ig
def testHoles(holes):
    return holes

def testPits(ridge):
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
    log("Hold is: " + str(tetrisSim.hold))
    log('_'*20)

def log(str=""):
    if logData:
        print(str)

def wait(amt):
    if latencyOn:
        time.sleep(amt)

main()