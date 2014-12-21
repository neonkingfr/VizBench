import time
import os
import random
import httplib, urllib
import time
import nosuch.midiutil

from vsthost import VstHost,VstPlugin

from subprocess import call,Popen
from nosuch.oscutil import *
from time import sleep

# wait for kill to finish
call(["c:/python27/python.exe","c:/local/manifold/bin/killvsthost.py"])

# vsthost gets put in the background
Popen(["vsthost.exe","-server"])

# give it some time to start up?
sleep(1.0)

V = VstHost()
v1 = V.createVstInstance("Absynth 5")
v2 = V.createVstInstance("FM8")

v1.setchannel(1)
v2.setchannel(2)

v1.setparameter(2,0.25)

for pitch in range(50,100,24):
	V.noteon(1,pitch,100)
	sleep(0.2)
	V.noteoff(1,pitch,100)

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
