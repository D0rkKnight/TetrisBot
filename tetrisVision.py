
from PIL import ImageGrab
import win32gui
import win32con
import win32api
import tetrisSim
import numpy as np

desktop = win32gui.GetDesktopWindow()
screenDims = (1920.0, 1080.0)

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
        
        for key in tetrisSim.baseRot.keys():
            if (np.array_equal(square, tetrisSim.baseRot[key])):
                pQueue.append(key)
                selected = True
                break
        
        if np.array_equal(square, [[0, 0, 0], [1, 1, 0], [1, 1, 0]]):
            pQueue.append('O')
            selected = True

        if not selected: 
            pQueue.append('I')
    return pQueue
