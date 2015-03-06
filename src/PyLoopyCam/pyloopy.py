"""
Copyright (c) 2015, Tim Thompson
All rights reserved.	

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

-  Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

-  Redistributions in binary form must reproduce the above copyright notice, 
   this list of conditions and the following disclaimer in the documentation 
   and/or other materials provided with the distribution.

-  Neither the name of Tim Thompson, nosuch.com, nor the names of
   any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
"""

import sys
import time
import traceback
import thread
import threading
import copy
import asyncore
import asynchat
import socket
import sys
import re
import xml.dom.minidom as xmldom
import string
import pygame.pypm
import os.path
import os, pygame
import pickle

from os.path import isdir, isfile, isabs, abspath
from urllib import quote, unquote
from threading import *
from ctypes import *
from time import sleep
from Queue import Queue, Empty
from xml.sax import saxutils
from xml.dom import Node
from traceback import format_exc 
from dircache import listdir
from pygame.locals import *
from thread import *

from ffff import *

global debug
debug = False
global debugosc
debugosc = False
global debugosc2
debugosc2 = False

class NthEventServer(Thread):
	"""
	Provides an event stream that can serve multiple listeners
	track of what fingers are currently down, smoothing drag motion, etc.
	"""

	oneServer = None

	def __init__(self):

		Thread.__init__(self)
		self.setDaemon(True)

		NthEventServer.oneServer = self

		print "NthEventServer.oneServer = ", NthEventServer.oneServer
		self.dispenser = PushedEventDispenser()

		self.throttle = 0.005
		self.throttle = 0.0

		self.inputs = {}
		self.outputs = {}

		self.cv = threading.Condition()
		self.events = {}
		self.firstevent = 0
		self.nextevent = 0
		self.osc_recipients = {"music":[], "graphic":[]}

		self.start()
		self.too_old_seconds = 30.0
		self.event_inputs = {}
		self.forward_inputs = {}
		self.forward_finger = None

		self.tm0 = time.time()
		self.osc_count = 0

	def send_osc(self, o, apptype):
		(msg_addr, msg_data) = o
		if msg_addr == "":
			print "No msg_addr value in send_osc?"
			return
		now = time.time()
		self.osc_count += 1
		if now - self.tm0 > 1.0:
			print "OSC Per second = ", self.osc_count
			self.osc_count = 0
			self.tm0 = now
		msg_addr = str(msg_addr)
		b = createBinaryMsg(msg_addr, msg_data)
		# print "createBinary msg_addr=",msg_addr," msg_data=",msg_data

		print("SHOULD BE sending %s OSC=%s" % (apptype, o.__str__()))

		# r.osc_socket.sendto(b, (r.osc_addr, r.osc_port))

def main():

	debug = True
	httpaddr = "127.0.0.1"
	httpport = 7777
	rootdir = None 

	print "SYS.ARGV len=", len(sys.argv)
	argn = len(sys.argv)
	if len(sys.argv) == 1:
		print "NO arguments..."
	else:
		argn = 1
		if sys.argv[argn] == "-d":
			debug = True
			print "Debug is True"
			argn += 1
		else:
			debug = False
	
		argn += 1

		for i in range(argn, len	(sys.argv)):
			a = sys.argv[i]
			print("a = ", a)
			if a.startswith("rootdir:"):
				rootdir = abspath(a[8:])
			elif a.startswith("httpaddr:"):
				httpaddr = a[9:]
			elif a.startswith("httpport:"):
				httpport = int(a[9:])


	try:
		import os
		position = (-800, 0)
		position = (600, 360)
		os.environ['SDL_VIDEO_WINDOW_POS'] = str(position[0]) + "," + str(position[1])

		pygame.init()

		width = 250
		height = 500
		flags = pygame.SRCALPHA

		from panel import NthControlPanel
		ui = NthControlPanel(width, height, flags)
		
		time.sleep(1.0)
		# pygame.event.set_grab(True)

		try:
			ffff = Ffff("localhost",80)
		except:
			print "EXCEPT caught in creating Ffff! Exception=", format_exc()
		
		# ffff.set_ui(ui)
		ui.set_ffff(ffff)
		ui.set_status("")
		ui.lcd_refresh()
		ui.send_all_values()
		
		try:
			ui.start()
		except:
			print "EXCEPT caught in ui.start Exception=", format_exc()
			
		try:
			ui.shutdown()
		except:
			print "EXCEPT caught in ui.shutdown!"
			
		print("NthServer has finished")

	except KeyboardInterrupt:
		print("KeyboardInterrupt received...\n");
		# server.shutdown_quick()
	except:
		s = format_exc()
		if not re.search(".*shutdown_quick.*", s):
			print("Exception while running myserver?\n");
			print(s)
		# server.shutdown_quick()


if __name__ == '__main__':
	main()
	# import cProfile
	# cProfile.run('main()')
