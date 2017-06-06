# Real time thermo camera. It uses OpenCV and python 2.7
# To install OpenCV in raspberry Jessie
# http://www.pyimagesearch.com/2016/04/18/install-guide-raspberry-pi-3-raspbian-jessie-opencv-3/
# Thermomin. Rodo Lillo 2017 

# import the necessary packages

import time
import timeit
import cv2
from scipy import ndimage
import numpy as np
from skimage import exposure, img_as_ubyte

fifo = open('/var/run/mlx9062x.sock', 'r')
irbase=np.zeros((4, 16))
step=0.5

def get_overlay():
    ir_raw = fifo.read()
    ir_trimmed = ir_raw[0:256]
    ir = np.frombuffer(ir_trimmed, dtype=np.float32)
    ir2 = ir.reshape(16,4)[::, ::]
    irt= ir2.transpose()
    # Morph the image allowing only to increase or decrease at $step increment
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

    ir3 = exposure.rescale_intensity(irbase, in_range=(30,35)) #min and max values of black and white
    ir5 = img_as_ubyte(ir3)
    #print(ir) #Test purposes
    return ir5

time.sleep(0.01)
cv2.namedWindow("VIDEO", cv2.WND_PROP_FULLSCREEN)
cv2.setWindowProperty("VIDEO", cv2.WND_PROP_FULLSCREEN, cv2.WINDOW_FULLSCREEN)

delay = 0.0666666 #15fps


while 1:
    startTime = timeit.default_timer()
    fifoir = get_overlay()
    # I use this combination of filters to create the image
    fifores = cv2.blur(fifoir,(2,2))
    fifores2 = cv2.resize(fifores, (256, 16), interpolation=cv2.INTER_CUBIC)
    img_fifo = cv2.resize(fifores2, (800, 480), interpolation=cv2.INTER_CUBIC)
    # Apply the colormap. cv2.COLORMAP_JET is also nice
    fifo_color = cv2.applyColorMap(img_fifo, cv2.COLORMAP_HOT)
    cv2.imshow('VIDEO', fifo_color)
    waitTime=(delay-(timeit.default_timer()-startTime))*1000 #avoid flickering
    if waitTime<1: #minimum value to keep the flow
    	waitTime=1
    key = cv2.waitKey(int(waitTime)) & 0xFF
    # if the `q` key was pressed, break from the loop
    if key == ord("q"):
	break

cv2.destroyAllWindows()
fifo.close()
