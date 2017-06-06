# Very simple test to know if the thermomin service is running
# Uses numpy $>pip install numpy
# Thermomin (c) Rodo Lillo 2017
from __future__ import print_function #compatible with python 2.7
import sys, time, numpy
fifo = open('/var/run/mlx9062x.sock', 'r')
while 1:
	ir = numpy.frombuffer(fifo.read()[0:256], dtype=numpy.float32)
	sys.stderr.write("\x1b[2J\x1b[H") #clear the terminal
	for j in range (0,64):
		print ('[{:3.2f}]'.format(ir[j]), end=' ')
		if j%4==0: print ('')
	time.sleep(0.1) #10fps
