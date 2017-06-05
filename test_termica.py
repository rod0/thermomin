# import the necessary packages

import time
import timeit
import cv2
import math
from scipy import ndimage
import numpy as np
from skimage import exposure, img_as_ubyte
from skimage.morphology import disk
from skimage.filters import rank

fifo = open('/var/run/mlx9062x.sock', 'r')
irbase=np.zeros((4, 16))
step=0.5

def get_overlay():
    ir_raw = fifo.read()
    ir_trimmed = ir_raw[0:256]
    ir = np.frombuffer(ir_trimmed, dtype=np.float32)
    ir2 = ir.reshape(16,4)[::, ::]
    irt= ir2.transpose()
    for x in range (0,4):
	for y in range (0,16):
		if irbase[x,y] == 0: 
			irbase[x,y] = irt[x,y]
		elif irt[x,y] < irbase[x,y] -step: 
			irbase[x,y]-=step	
		elif irt[x,y] > irbase[x,y] +step:
			irbase[x,y]+=step
		else:
			irbase[x,y]=irt[x,y]

    ir3 = exposure.rescale_intensity(irbase, in_range=(30,35))
    ir5 = img_as_ubyte(ir3)
    #print(ir)
    return ir5

time.sleep(0.01)
cv2.namedWindow("VIDEO", cv2.WND_PROP_FULLSCREEN)
cv2.setWindowProperty("VIDEO", cv2.WND_PROP_FULLSCREEN, cv2.WINDOW_FULLSCREEN)

delay = 0.0666666


while 1:
    end=0
    startTime = timeit.default_timer()
    fifoir = get_overlay()
    fifores = cv2.blur(fifoir,(2,2))
    fifores2 = cv2.resize(fifores, (256, 16), interpolation=cv2.INTER_CUBIC)
    #img_fifo = cv2.resize(fifo_resize1, (480, 120), interpolation=cv2.INTER_NEAREST)
    img_fifo = cv2.resize(fifores2, (800, 480), interpolation=cv2.INTER_CUBIC)

    fifo_color = cv2.applyColorMap(img_fifo, cv2.COLORMAP_HOT)
    cv2.imshow('VIDEO', fifo_color)
    waitTime=(delay-(timeit.default_timer()-startTime))*1000
    #print (waitTime)
    #if waitTime>0:
    #	time.sleep(waitTime)

    #print (time.time() - startTime)
    #while (time.time() - startTime < delay):
    if waitTime<1:
    	waitTime=1
    key = cv2.waitKey(int(waitTime)) & 0xFF
    # if the `q` key was pressed, break from the loop
    if key == ord("q"):
        #end=1
	break
    #if end == 1:
    #	break

cv2.destroyAllWindows()
fifo.close()
