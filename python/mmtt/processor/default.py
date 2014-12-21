import time
import sys
import inspect
import os
import mmtt.builtin
from threading import Thread, Lock

from random import randint,random

from nosuch.midiutil import *
from nosuch.midipypm import *

from mmtt.builtin import debug, next_event
from OpenGL.GL import *

from traceback import format_exc

BackgroundThread = None
MidiStarted = False

class Background(Thread):

	def __init__(self):
		Thread.__init__(self)
		debug("Background __init__")

	def run(self):
		debug("Background run start")
		while True:
			e = next_event()
			if e != None:
				if e["type"] != "CursorDrag":
					debug("Background event type="+e["type"]+" region="+e["region"])
				dt = 0.001
			else:
				dt = 0.01
			# time.sleep(dt)

class Processor:

	def __init__(self,passthru=True):
		debug("Processor.__init__ time=%f" % (time.time()) )

	def processOpenGL(self):
		# debug("processOpenGL in Processor!")
		return self.draw()

	def quicknote(self):
		p = Phrase()

		nt = NoteOn(pitch=64, channel=1, velocity=100)
		p.append(SequencedMidiMsg(nt,clocks=0))
		nt = NoteOff(pitch=64, channel=1, velocity=0)
		p.append(SequencedMidiMsg(nt,clocks=192))

		# nt = NoteOn(pitch=68, channel=1, velocity=100)
		# p.append(SequencedMidiMsg(nt,clocks=96))
		# nt = NoteOff(pitch=68, channel=1, velocity=0)
		# p.append(SequencedMidiMsg(nt,clocks=97))

		# nt = NoteOn(pitch=68, channel=1, velocity=100)
		# p.append(SequencedMidiMsg(nt,clocks=48))
		# nt = NoteOff(pitch=68, channel=1, velocity=0)
		# p.append(SequencedMidiMsg(nt,clocks=48))

		# Schedule the notes on MIDI output
		for n in p:
			self.o.schedule(n)

	def draw(self):
		# global BackgroundThread
		# if BackgroundThread == None:
		# 	BackgroundThread = Background()
		# 	BackgroundThread.start()

		global MidiStarted
		if not MidiStarted:
			MidiStarted = True
			Midi.startup()
			m = MidiPypmHardware()
			outname = "loopMIDI Port 1"
			# Open MIDI output named
			try:
				# if outname is None, it opens default output
				self.o = m.get_output(outname)
				self.o.open()
			except:
				print "Error opening output: %s, exception: %s" % (outname,format_exc())
				# Midi.shutdown()
				# sys.exit(1)

			self.quicknote()

		return ""

		# plugins (classes that extend Processor) should define this
		# debug("draw in Processor!")
		while True:
			e = next_event()
			if e == None:
				break
			debug("Processor.draw2 type="+e["type"]+" region="+e["region"])
		return ""

class DefaultProcessor(Processor):

	def __init__(self):
		debug("DefaultProcessor.__init__ time=%f" % (time.time()) )
		Processor.__init__(self,passthru=True)

	def draw(self):
		debug("draw in DefaultProcessor!")
		return "foo2"
