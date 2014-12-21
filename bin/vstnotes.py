import time
import os
import random
import httplib, urllib
import time
from vsthost import VstHost,VstPlugin

from subprocess import call,Popen
from nosuch.oscutil import *
from time import sleep

V = VstHost()
p1 = 0   # assumed to be Absynth 
p2 = 1   # assumed to be FM8 

V.setchannel(p1,2)
V.setchannel(p2,3)

for n in range(10):
	pitch = random.randint(1,127)
	V.noteon(2,pitch,100)
	sleep(0.2)
	V.noteoff(2,pitch,100)

# V.jsonrpc("hide")

# while True:
# 	chan = 1
# 	pitch = random.randint(1,127)
# 	velocity = 100
# 	osctovst("/midi",["noteon",chan,pitch,velocity])
# 	# sleep(0.1)
# 	osctovst("/midi",["noteoff",chan,pitch,velocity])

# time.sleep(5)
# tovst("/clear",[])
# time.sleep(5)
# tovst("/stop",[])
