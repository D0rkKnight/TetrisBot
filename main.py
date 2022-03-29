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

searchDepth = 4

firstBlock = True
logData = False

INTERNAL_SIM = 0
SCREEN_CAPTURE = 1
source = INTERNAL_SIM
hasInputLag = False
divergenceRatio = 1
divergeIter = 0

MoveInput = collections.namedtuple('MoveInput', 'delta rot board score iScore lineclears piece')

tetroMarginBuffer = []
tetroRidgeBuffer = []
lockstep = True
stepPermit = False


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

    # Press A to go forwards
    global stepPermit
    if lockstep:
        if keyboard.is_pressed('a'):
            stepPermit = True
        if not stepPermit:
               return
        stepPermit = False

    # # Screengrab for positioning
    # if keyboard.is_pressed('1'):
    #     img = tetrisVision.pollScreen() 
    #     img.show()

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

    searchQueue = [tetrisSim.currentPiece]
    for i in range(searchDepth-1):
        searchQueue.append(queue[i])

    # Position block
    
    start = time.time() # Speedtests
    
    # Print queue
    for i in range(len(queue)):
        log('Element ' + str(i) + ' of queue is ' + str(queue[i]))

    # Simple C algo override
    # Write ridge into board internal ridge
    for i in range(grid.w):
        grid.setRidge(ridge[i]+1, i)
    
    # Write holes too
    grid.holes = holes
    
    measure1 = time.time()
    moveset = grid.search(searchQueue, tetrisSim.hold)
    xDest = moveset[0]
    rot = moveset[1]
    willHold = moveset[2]
    slide = moveset[3]
    
    measure1 = time.time()-measure1
    
    measure2 = time.time()
    moveset = grid.search(searchQueue[0:len(searchQueue)-1], tetrisSim.hold)
    measure2 = time.time()-measure2
    
    global divergenceRatio, divergeIter
    if (measure2 != 0):
        divergeIter += 1
        divergenceRatio = (divergenceRatio*divergeIter + measure1/measure2)/(divergeIter + 1)
        print("Divergence: "+str(divergenceRatio))
    
    

    # Calculate necessary shift
    spawnPos = tetrisSim.genPieceSpawn(searchQueue[0])
    if (willHold):
        spawnPos = tetrisSim.genPieceSpawn(tetrisSim.hold)
        holdPiece()

    delta = xDest - spawnPos[0]
    move([delta, 0], rot)
    
    print("Slide: "+str(slide))
    print("Holes: "+str(grid.holes))
    
    if (slide == 0):
        hardDrop()
    else:
        softDrop()
        move([slide, 0], 0)
        hardDrop()

    #time.sleep(1)

    print(time.time()-start)
    print() # Cap debug listing

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

def softDrop():
    if source == SCREEN_CAPTURE:
        ping('down')
        
    elif source == INTERNAL_SIM:
        tetrisSim.softDrop()
        tetrisSim.runSim()
    
    wait(hardDropLatency) # whatever
    return

def holdPiece():
    if source == SCREEN_CAPTURE:
        ping('c')
    elif source == INTERNAL_SIM:
        tetrisSim.holdPiece()
        tetrisSim.runSim()
    
    wait(holdLatency)

def move(delta, rot):
    dx = delta[0]
    dy = delta[1]
    if source == SCREEN_CAPTURE:
        if rot == 1:
            ping('up')
        if rot == 2:
            ping('up')
            ping('up')
        if rot == 3:
            ping('z')

        if dx != 0:
            key = 'right'
            if dx < 0:
                key = 'left'
            
            absDelta = abs(dx)
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

        if dx != 0 or dy != 0:
            tetrisSim.shiftBy(dx, dy)
            tetrisSim.runSim()
        

def ping(key):
    keyboard.press_and_release(key)
    wait(latency)

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
    
    o = holes

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

def log(str=""):
    if logData:
        print(str)

def wait(amt):
    if latencyOn:
        time.sleep(amt)

main()