import time
import os
import random
import httplib, urllib
import time
import nosuch.midiutil

from subprocess import call,Popen
from nosuch.oscutil import *
from time import sleep
from vsthost import VstHost,VstPlugin

from nosuch.midiutil import MidiVstHostHardware

V = VstHost()
v1 = V.getVstInstance(0)
v2 = V.getVstInstance(1)

chanA = 1
chanB = 2
v1.setchannel(chanA)
v2.setchannel(chanB)

# v1.setparameter(2,0.25)

m = MidiVstHostHardware()

for n in range(10):
	pitch = random.randint(1,127)
	if (n%2) == 0:
		chan = chanA
	else:
		chan = chanB
	V.noteon(chan,pitch,100)
	sleep(0.2)
	V.noteoff(chan,pitch,100)
