"""
This module provides an interface to the Vsthost interface for MIDI I/O
"""

import sys
import time
import traceback
import thread
import threading
import copy
import string
import re

from threading import Thread,Lock
from math import sqrt
from ctypes import *
from time import sleep
from traceback import format_exc
from array import array

from nosuch.midiutil import *
from nosuch.oscutil import *
from nosuch.vsthost import *

class MidiVstHostHardware(MidiBaseHardware):

	def __init__(self,input_name=None,output_name=None):

		print "IN __INIT__, output_name = ",output_name
		if input_name == None:
			input_name = "5557@127.0.0.1"
		self.input_name = input_name

		if output_name == None:
			output_name = "5556@127.0.0.1"
		self.output_name = output_name

	def input_devices(self):
		return [self.input_name]

	def output_devices(self):
		return [self.input_name]

	def get_input(self,input_name=None):
		if input_name == None:
			input_name = self.input_name
		port = re.compile(".*@").search(input_name).group()[:-1]
		host = re.compile("@.*").search(input_name).group()[1:]
		return MidiVstHostHardwareInput(host,port)

	def get_output(self,output_name=None):
		if output_name == None:
			output_name = self.output_name
		print "IN GET_OUTPUT, output_name = ",output_name
		port = re.compile(".*@").search(output_name).group()[:-1]
		host = re.compile("@.*").search(output_name).group()[1:]
		return MidiVstHostHardwareOutput(host,port)

class MidiVstHostHardwareInput(MidiBaseHardwareInput):

	def __init__(self,inhost,inport):
		raise Exception, "MidiVstHostHardwareInput isn't finished"

	def open(self):
		if Midi.oneThread:
			Midi.oneThread._add_midiin(self)

	def close(self):
		if Midi.oneThread:
			Midi.oneThread._remove_midiin(self)

	def __str__(self):
		return 'MidiInput(name="debug")'

	def to_xml(self):
		return '<midi_input name="debug"/>'


class MidiVstHostHardwareOutput(MidiBaseHardwareOutput):

	def __init__(self,outhost,outport):
		self.vsthost = VstHost()

	def is_open(self):
		return True

	def open(self):
		pass

	def close(self):
		pass

	def write_msg(self,m):
		self.vsthost.write_midi(m)
		# o = m.to_osc()
		# b = createBinaryMsg(o[0],o[1])
		# r = self.recipient
		# r.osc_socket.sendto(b,(r.osc_addr,r.osc_port))

	def schedule(self,msg,time=None):
		Midi.schedule(self,msg,time)

	def __str__(self):
		return 'MidiOutput(name="debug")'

	def to_xml(self):
		return '<midi_output name="debug"/>'


"""
This is executed when module is loaded
"""
