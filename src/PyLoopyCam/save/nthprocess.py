import re
import thread
import threading

from threading import Thread

import nosuch.fingerutil
import nosuch.midiutil

from nosuch.fingerutil import *
from nosuch.midiutil import *

class BaseEvent:
	def __init__(self):
		self.time = 0.0

class NthEventProcessor:

	def __init__(self,server):
		self.server = server
		self.last_finger_event = {}
		self.last_clock_time = -1

	def process(self,ev):

		print "NthEventProcessor ev=",ev
		server = self.server
		if isinstance(ev,FingerEvent):
			ev.time -= server.finger_timezero
			eventit = True

			i = ev.devindex
			if i <= 3:
				# The finger_order hack allows the caller
				# of the server to adjust the devindex values
				# returned for finger events
				ev.devindex = ord(server.finger_order[i])-ord("0")

			# Don't send finger drag events (for the same finger)
			# closer than throttle secs apart

			if isinstance(ev,FingerDrag):
				if ev.uid in self.last_finger_event:
					dt = ev.time - self.last_finger_event[ev.uid].time
					if dt < server.throttle:
						print "Drag too close in time!"
						eventit = False

			if eventit and server.forward_finger:
				f = server.forward_finger
				evtext = "%s" % ev
				m = SysEx(0xf0)
				m.append(0)
				m.append(0x7f)
				for c in evtext:
					m.append(ord(c))
				m.append(0xf7)
				f.schedule(m)

			if eventit:
				server.broadcast_event(ev)
				if len(server.osc_recipients) > 0:
					if server.do_frets:
						server._send_osc_string_event(ev)
					else:
						server._send_osc_event(ev)
				self.last_finger_event[ev.uid] = ev

		elif isinstance(ev,MidiEvent):
			# print "Got MidiEvent in _push_event ev.time=",ev.time," ev=",ev
			# ev.time -= server.midi_timezero
			# See if it's one we should forward or event
			m = ev.midimsg
			if isinstance(m,RealTime):
				if self.last_clock_time >= 0:
					dt = ev.time - self.last_clock_time
					print "dt=%f" % (dt)
				self.last_clock_time = ev.time
				return
			# Should this be more efficient than a loop?
			# There aren't very many...
			for i in server.forward_inputs:
				if i == m.device:
					for o in server.forward_inputs[i]:
						o.schedule(m)
			# If device matches one we're servicing events for...
			for i in server.event_inputs:
				if i == m.device:
					server.broadcast_event(ev)
					break
			if len(server.osc_recipients) > 0:
				server._send_mapped_midi_osc_event(ev)

		elif isinstance(ev,OscEvent):
			print "Got OscEvent in _push_event ev=",ev
			if not self.process_osc_registerclient(ev):
				ev.time -= server.osc_timezero
				server.broadcast_event(ev)
		else:
			print "Unknown event type in _push_event!? ev=",ev

	def process_osc_registerclient(self,ev):
		m = ev.oscmsg
		if m[0] == "/registerclient":
			if m[1] != ",si":
				print "Unexpected type signature for /registerclient?"
			else:
				self.server.add_osc_recipient(m[2],int(m[3]))
			return True
		elif m[0] == "/unregisterclient":
			if m[1] != ",si":
				print "Unexpected type signature for /unregisterclient?"
			else:
				self.server.remove_osc_recipient(m[2],int(m[3]))
			return True
		return False

	def send_osc(self,o):
		print "send_osc, o=%s" % (o.__str__())
		self.server.send_osc(o)

class TemplateEventProcessor(NthEventProcessor):

	def process(self,ev):

		if isinstance(ev,FingerEvent):
			pass

		elif isinstance(ev,MidiEvent):
			pass

		elif isinstance(ev,OscEvent):
			pass

class Cs32:

	def __init__(self,server):
		self.tocs32 = server.get_midiout("Cooper")

	def cs32_toggle_msg(self,row,slot,v):
		if slot < 0 or slot >= 16:
			raise Exception, "Invalid slot value (%d) passed to cs32_toggle_msg!?" % slot
		if row == 2:
			return Controller(controller=16+slot,value=v,channel=14)
		if row == 1:
			return Controller(controller=80+slot,value=v,channel=14)
		if row == 0:
			return Controller(controller=16+slot,value=v,channel=15)
		if row == 3:
			return Controller(controller=slot,value=v,channel=15)
		if row == 4:
			return Controller(controller=64+slot,value=v,channel=14)
		if row == 5:
			return Controller(controller=slot,value=v,channel=14)
		raise Exception, "Invalid row value passed to cs32_toggle_msg!?"

	def cs32_rowslot(self,m):
		cv = m.value
		ct = m.controller
		ch = m.channel
		if ch == 16 and ct >= 16 and ct < 32:
			return(6,ct-16)
		if ch == 16 and ct >= 0 and ct < 16:
			return(7,ct)
		if ch == 15 and ct >= 16 and ct < 32:
			return (0,ct-16)
		if ch == 14 and ct >= 80 and ct < 96:
			return (1,ct-80)
		if ch == 14 and ct >= 16 and ct < 32:
			return (2,ct-16)
		if ch == 15 and ct >= 0 and ct < 16:
			return (3,ct)
		if ch == 14 and ct >= 64 and ct < 80:
			return (4,ct-64)
		if ch == 14 and ct >= 0 and ct < 16:
			return (5,ct)
		return (-1,-1)

	def clear_cs32(self):
		self.setall_cs32(0)

	def flash_cs32(self):
		self.setall_cs32(1)
		self.setall_cs32(0)

	def setall_cs32(self,v):
		t0 = Midi.time_now()
		for slot in range(0,16):
			for row in range(0,6):
				m = self.cs32_toggle_msg(row,slot,v)
				self.tocs32.schedule(m,time=t0)

	def cs32_off(self,row,slot):
		m = self.cs32_toggle_msg(row,slot,0)
		self.tocs32.schedule(m)


class UnionEventProcessor(NthEventProcessor):

	def __init__(self,server):
		NthEventProcessor.__init__(self,server)

		self.throttle = 0.005
		self.throttle = 0.0
		self.devindex2pad = {0:-1,1:-1,2:-1,3:-1,4:-1,5:-1,6:-1,7:-1}
		self.nfingers = 0
		self.padmode = {}
		for n in range(0,8):
			self.padmode[n] = 0

		self.process_finger = self.process_finger_pad_pos

		self.tocs32 = server.get_midiout("Cooper")
		# print "TJT DEBUG tocs32=",self.tocs32
		# self.tocs32 = server.get_midiout("Yoke.*5")

		print "Sleeping for 7 seconds to let things initialize..."
		# otherwise, some of the cs32 toggles don't clear properly
		sleep(7.0)

		if not self.tocs32:
			print "Hey, can't find CS32 output!?"
		else:
			self.flash_cs32()

		print "UnionEventProcessor is activated..."

	def cs32_toggle_msg(self,row,slot,v):
		if slot < 0 or slot >= 16:
			raise Exception, "Invalid slot value (%d) passed to cs32_toggle_msg!?" % slot
		if row == 2:
			return Controller(controller=16+slot,value=v,channel=14)
		if row == 1:
			return Controller(controller=80+slot,value=v,channel=14)
		if row == 0:
			return Controller(controller=16+slot,value=v,channel=15)
		if row == 3:
			return Controller(controller=slot,value=v,channel=15)
		if row == 4:
			return Controller(controller=64+slot,value=v,channel=14)
		if row == 5:
			return Controller(controller=slot,value=v,channel=14)
		raise Exception, "Invalid row value passed to cs32_toggle_msg!?"

	def cs32_rowslot(self,m):
		cv = m.value
		ct = m.controller
		ch = m.channel
		if ch == 16 and ct >= 16 and ct < 32:
			return(6,ct-16)
		if ch == 16 and ct >= 0 and ct < 16:
			return(7,ct)
		if ch == 15 and ct >= 16 and ct < 32:
			return (0,ct-16)
		if ch == 14 and ct >= 80 and ct < 96:
			return (1,ct-80)
		if ch == 14 and ct >= 16 and ct < 32:
			return (2,ct-16)
		if ch == 15 and ct >= 0 and ct < 16:
			return (3,ct)
		if ch == 14 and ct >= 64 and ct < 80:
			return (4,ct-64)
		if ch == 14 and ct >= 0 and ct < 16:
			return (5,ct)
		return (-1,-1)

	def clear_cs32(self):
		self.setall_cs32(0)

	def flash_cs32(self):
		self.setall_cs32(1)
		self.setall_cs32(0)

	def setall_cs32(self,v):
		t0 = Midi.time_now()
		for slot in range(0,16):
			for row in range(0,6):
				m = self.cs32_toggle_msg(row,slot,v)
				self.tocs32.schedule(m,time=t0)

	def cs32_off(self,row,slot):
		m = self.cs32_toggle_msg(row,slot,0)
		self.tocs32.schedule(m)

	def process_finger_pad_pos(self,ev):
		pad = self.devindex2pad[ev.devindex]
		padmode = self.padmode[pad]

		# print "pad=",pad

		# PAD MODE 0 - control start/end points in loops
		if padmode == 0:
			loopnum = pad
			if isinstance(ev,FingerDrag) or isinstance(ev,FingerDown):
				if ev.y < 0.5:
					action = "/nth/action/setstart"
				else:
					action = "/nth/action/setend"
				self.send_osc((action,[loopnum,ev.x]))
		elif padmode == 1:
			if isinstance(ev,FingerDown):
				print "Pad ",pad," freeze start"
				self.send_osc(("/nth/action/freezestart",[pad]))
			elif isinstance(ev,FingerDrag):
				print "Pad ",pad," freeze drag xy=",ev.x,ev.y
				self.send_osc(("/nth/action/freezedrag",[pad,ev.x,ev.y]))
			elif isinstance(ev,FingerUp):
				print "Pad ",pad," freeze end"
				self.send_osc(("/nth/action/freezeend",[pad]))
		else:
			print "Unknown pademode: ",padmode

	def cs32_showpadmode(self,padnum):
		currmode = self.padmode[padnum]
		print "showpadmode padnum=",padnum," currmode=",currmode
		for slot in range(0,8):
			if slot == currmode:
				v = 1
			else:
				v = 0
			m = self.cs32_toggle_msg(3+padnum,slot,v)
			self.tocs32.schedule(m)
			

	def process(self,ev):

		print "Union Event Processor ev=",ev
		if isinstance(ev,FingerEvent):

			ev.time -= self.server.finger_timezero
			eventit = True

			i = ev.devindex
			if i <= 3:
				# The finger_order hack allows the caller
				# of the server to adjust the devindex values
				# returned for finger events
				ev.devindex = ord(self.server.finger_order[i])-ord("0")

			if not ev.devindex in self.devindex2pad:
				print "Unabled to process FingerEvent, devindex=",ev.devindex," not in devindex2pad!?"
				return

			if self.devindex2pad[ev.devindex] == -1:
				self.devindex2pad[ev.devindex] = self.nfingers
				self.nfingers += 1

			# Don't send finger drag events (for the same finger)
			# closer than throttle secs apart

			if isinstance(ev,FingerDrag):
				if ev.uid in self.last_finger_event:
					dt = ev.time - self.last_finger_event[ev.uid].time
					if dt < self.throttle:
						print "Drag too close in time!"
						eventit = False

			if eventit:
				self.process_finger(ev)
				self.last_finger_event[ev.uid] = ev


		elif isinstance(ev,MidiEvent):
			m = ev.midimsg
			ch = m.channel
			ct = m.controller

			# print "MidiEvent ch=",ch," ct=",ct," val=",m.value
			if not re.search("Cooper",m.device.name):
				print "Ignoring MIDI input from ",m.device.name
				return
				
			(row,slot) = self.cs32_rowslot(m)

			# rows 0,1,2 are the top three rows of buttons
			# rows 3,4,5 are the bottom three rows of buttons
			# row 6 is the top row of sliders
			# row 7 is the bottom row of sliders

			cv = m.value
			fcv = float(cv) / 127.0
			onoff = int(cv != 0)
			print "ROW,SLOT = ",row,slot,"  onoff=",onoff," fcv=",fcv
			if row < 0 or slot < 0:
				print "Ignoring non-CS32 toggle/slider msg m=",m
				return

			# Individual actions, top right side
			if row == 6 and slot == 0:
				self.send_osc(("/nth/action/spritesize",[fcv]))
			elif row == 6 and slot == 1:
				self.send_osc(("/nth/action/spriteang",[fcv]))
			elif row == 6 and slot == 2:
				self.send_osc(("/nth/action/trackcolor",[fcv]))
			elif row == 6 and slot == 3:
				self.send_osc(("/nth/action/trackmovesmooth",[fcv]))
			elif row == 6 and slot == 4:
				self.send_osc(("/nth/action/trackangsmooth",[fcv]))
			elif row == 6 or row == 7:
				pass # ignore all the other sliders on 6/7
			elif row == 0 and slot == 8:
				self.send_osc(("/nth/action/setlive",[onoff]))
			elif row == 0 and slot == 9:
				self.send_osc(("/nth/action/setxor",[onoff]))
			elif row == 1 and slot == 8:
				self.send_osc(("/nth/action/trackinit",[]))
				self.cs32_off(row,slot)
			elif row == 1 and slot == 9:
				self.send_osc(("/nth/action/trackautoinit",[onoff]))
			elif row == 1 and slot == 10:
				self.send_osc(("/nth/action/trackautoangle",[onoff]))
			elif row == 1 and slot == 11:
				self.send_osc(("/nth/action/trackrandcolor",[onoff]))
			elif row == 1 and slot == 12:
				self.send_osc(("/nth/action/trackshowlive",[onoff]))
			elif row == 1 and slot == 13:
				self.send_osc(("/nth/action/trackdebug",[]))
				self.cs32_off(row,slot)

			# Loop-specific things, top left side
			elif row==0 or row==1 or row==2:
				loopnum = row
				if slot == 0:
					self.send_osc(("/nth/action/record",[loopnum,onoff]))
				elif slot == 1:
					self.send_osc(("/nth/action/play",[loopnum,onoff]))
				elif slot == 2:
					self.send_osc(("/nth/action/restart",[loopnum]))
					self.cs32_off(row,slot)
					# send it back, to turn the LED off
					# m.value = 0
					# if self.tocs32:
					# 	self.tocs32.schedule(m)
				elif slot == 3:
					self.send_osc(("/nth/action/playfactor",[loopnum,2.0]))
					self.cs32_off(row,slot)
				elif slot == 4:
					self.send_osc(("/nth/action/playfactor",[loopnum,0.5]))
					self.cs32_off(row,slot)
				elif slot == 5:
					self.send_osc(("/nth/action/setreverse",[loopnum,onoff]))
			elif row==3 or row==4 or row==5:
				# Lower 3 rows set pad mode
				padnum = row - 3
				if slot >= 0 and slot < 8:
					self.padmode[padnum] = slot
					self.cs32_showpadmode(padnum)
			else:
				self.cs32_off(row,slot)

		elif isinstance(ev,OscEvent):
			if self.process_osc_registerclient(ev):
				print "New OSC listener, resetting"
				self.flash_cs32()
			else:
				print "Got unrecognized OSC message, broadcasting it"
				ev.time -= self.server.osc_timezero
				self.server.broadcast_event(ev)
			pass

class NthFretMonitor(Thread):

	def __init__(self,es):

		Thread.__init__(self)
		self.eventserver = es
		self.setDaemon(True)

	def run(self):
		while True:
			self.eventserver.fret_check()
			print "FRET MONITOR RUNNING!?"
			sleep(0.01)

class FreeFrameEventProcessor(NthEventProcessor):

	def process(self,ev):
		print "HI from FreeFrameEventProcessor!  ev=",ev

class PrintEventProcessor(NthEventProcessor):

	def process(self,ev):
		print "PrintEventProcessor: ",ev
		NthEventProcessor.process(self,ev)

#######################################################################3
# The code below was an experiment in using the iGesture pad as
# a strummable controller with "frets"
#######################################################################3

nfrets = 12
string_basepitch = {1:52,2:57,3:62,4:67,5:71,6:76}

class StringDownEvent(BaseEvent):

	def __init__(self,strnum,ev):
		BaseEvent.__init__(self)
		self.strnum = strnum
		self.prox = ev.prox
		self.x = ev.x
		self.y = ev.y
		fret = self._find_fret(ev.x)
		self.pitch = string_basepitch[strnum] + fret

	def _find_fret(self,x):
		dx = 1.0 / nfrets
		for i in range(nfrets):
			if x < (i+1)*dx:
				return i
		return nfrets-1

	def to_osc(self):
		return("/multitouch/fretboard/stringdown",[self.strnum,self.pitch,self.prox])

class StringDragEvent(BaseEvent):

	def __init__(self,strnum,ev,downev):
		BaseEvent.__init__(self)
		self.strnum = strnum
		self.pitch = downev.pitch
		self.prox = ev.prox
		self.x = ev.x
		self.y = ev.y
		self.dx = ev.x - downev.x
		self.dy = ev.y - downev.y

	def to_osc(self):
		return("/multitouch/fretboard/stringdrag",[self.strnum,self.pitch,self.prox,self.dx,self.dy])

class StringUpEvent(BaseEvent):

	def __init__(self,strnum):
		BaseEvent.__init__(self)
		self.strnum = strnum

	def to_osc(self):
		return("/multitouch/fretboard/stringup",[self.strnum])

class FretEventProcessor(NthEventProcessor):

	def __init__(self,server):

		NthEventProcessor.__init__(self,server)

		# print "Hi from FretEventProcessor init!   FRET STUFF NEEDS WORK!!!"
		self.do_frets = False
		self.string_isdown = {1:0,2:0,3:0,4:0,5:0,6:0}
		self.string_lastchange = {1:0,2:0,3:0,4:0,5:0,6:0}
		self.string_downevent = {1:None,2:None,3:None,4:None,5:None,6:None}
		self.fretmon = NthFretMonitor(self)
		self.fretmon.start()

	def process(self,ev):
		print "HI from FretEventProcessor!  ev=",ev

	def fret_check(self):
		# print "FRET_CHECK ",time.time()
		now = time.time()
		for strnum in range(1,7):
			if not self.string_isdown[strnum]:
				continue
			dt = now - self.string_lastchange[strnum]
			if dt > 0.1:
				# print "        String ",strnum," up (fret_check) ",time.time()
				self.string_isdown[strnum] = 0

				fev = StringUpEvent(strnum)
				self._send_osc_event(fev)

				self.string_lastchange[strnum] = now

	def _send_osc_string_event(self,ev):
		dy = 1.0 / 6.0
		if ev.y < dy:
			strnum = 1
		elif ev.y < 2.0*dy:
			strnum = 2
		elif ev.y < 3.0*dy:
			strnum = 3
		elif ev.y < 4.0*dy:
			strnum = 4
		elif ev.y < 5.0*dy:
			strnum = 5
		else:
			strnum = 6

		now = time.time()

		if isinstance(ev,FingerDrag) or isinstance(ev,FingerDown):
			if not self.string_isdown[strnum]:
				# The fret down is only acknowledged 
				# if nothing has changed recently (within
				# .1 seconds) or if the prox is fairly high
				if ev.prox > 0.5 or self.string_lastchange[strnum] < (now-0.1):
					self.string_isdown[strnum] = 1
					fev = StringDownEvent(strnum,ev)
					self.string_downevent[strnum] = fev
					self._send_osc_event(fev)
			else:
				# if the proximity gets low enough,
				# we consider it a finger-up event
				if ev.prox < 0.25:
					# print "        String",strnum,"up (prox)",now
					self.string_isdown[strnum] = 0
					fev = StringUpEvent(strnum)
					self._send_osc_event(fev)
				else:
					downev = self.string_downevent[strnum]
					fev = StringDragEvent(strnum,ev,downev)
					self._send_osc_event(fev)
		else:   # FingerUp
			# We ignore normal fingerup events because they are
			# sometimes spurious (fingerup and fingerdown will
			# come in quick succession).  Instead we rely on
			# string_check to detect when a string has been let up.
			pass

		self.string_lastchange[strnum] = now

