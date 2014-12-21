from nosuch.midiutil import *
from nosuch.midifile import *
from nosuch.midipypm import *
from nosuch.oscutil import *
from nosuch.midiosc import *
from traceback import format_exc
from time import sleep
import sys

showalive = True
time0 = time.time()

Cursors = {}
CursorSeq = 0
Time0 = 0
Processors = []
MidiOutput = None
OscOutput = None
Debug = True

# The SID range for each region extends from its starting value (e.g. 11000)
SidRange = 100

# X and Y values are 0-1, normalized for the extent of each region.
# Z values, on the other hand, are absolute values, in meters.
# This value of Zmax is really only appropriate for the Intel/Creative camera.
# For the Kinect, the value should be larger.
Zmax = 0.2

def timetag():
	return "%8.3f "%(time.time()-Time0)

class RegionControl:

	def __init__(self,minsid,tag,xcontrol,ycontrol,zcontrol):
		self.minsid = minsid
		self.maxsid = self.minsid + SidRange
		self.tag = tag
		self.xcontrol = xcontrol
		self.ycontrol = ycontrol
		self.zcontrol = zcontrol

	def _applysTo(self,sid):
		return sid >= self.minsid and sid <= self.maxsid

	def f2val(self,f):
		i = int(f * 128.0) % 128
		# print "f=",f," i=",i
		return i

	def z2val(self,f):
		if f < 0.0:
			f = 0.0
		elif f > Zmax:
			f = Zmax
		nf = f / Zmax
		i = int(nf * 128.0) % 128
		# print "f=",f," nf=",nf," i=",i
		return i

	def sendControl(self,type,val):
		c = Controller(channel=1,controller=type,value=val)
		if MidiOutput:
			MidiOutput.schedule(c)
		if Debug:
			print "MIDI c="+c.__str__()

	def sendControllers(self,x,y,z):
		if self.xcontrol >= 0:
			self.sendControl(self.xcontrol,self.f2val(x))
		if self.ycontrol >= 0:
			self.sendControl(self.ycontrol,self.f2val(y))
		if self.ycontrol >= 0:
			self.sendControl(self.zcontrol,self.z2val(z))

	def cursorDown(self,sid,x,y,z):
		if not self._applysTo(sid):
			return
		self.sendControllers(x,y,z)

	def cursorDrag(self,sid,x,y,z):
		if not self._applysTo(sid):
			return
		self.sendControllers(x,y,z)

	def cursorUp(self,sid):
		if not self._applysTo(sid):
			return
		# do nothing

class RegionOsc:

	def __init__(self,minsid,addr):
		self.minsid = minsid
		self.maxsid = self.minsid + SidRange
		self.addr = addr

	def _applysTo(self,sid):
		return sid >= self.minsid and sid <= self.maxsid

	def f2val(self,f):
		i = int(f * 128.0) % 128
		# print "f=",f," i=",i
		return i

	def z2val(self,f):
		if f < 0.0:
			f = 0.0
		elif f > Zmax:
			f = Zmax
		nf = f / Zmax
		i = int(nf * 128.0) % 128
		# print "f=",f," nf=",nf," i=",i
		return i

	def sendValues(self,x,y,z):
		vals = [self.f2val(x),self.f2val(y),self.z2val(z)]
		if OscOutput:
			OscOutput.sendosc(self.addr,vals)
		if Debug:
			print "OSC addr=",self.addr," vals=",vals


	def cursorDown(self,sid,x,y,z):
		if not self._applysTo(sid):
			return
		self.sendValues(x,y,z)

	def cursorDrag(self,sid,x,y,z):
		if not self._applysTo(sid):
			return
		self.sendValues(x,y,z)

	def cursorUp(self,sid):
		if not self._applysTo(sid):
			return
		# do nothing

class RegionButton:

	def __init__(self,minsid,tag,control):
		self.minsid = minsid
		self.maxsid = self.minsid + SidRange
		self.tag = tag
		self.control = control

	def _applysTo(self,sid):
		return sid >= self.minsid and sid <= self.maxsid

	def sendControl(self,val):
		c = Controller(channel=1,controller=self.control,value=val)
		if MidiOutput:
			MidiOutput.schedule(c)
		if OscOutput:
			OscOutput.sendosc("/button",[self.tag,val])
		if Debug:
			print "MIDI c="+c.__str__()

	def cursorDown(self,sid,x,y,z):
		if not self._applysTo(sid):
			return
		if Debug:
			print "buttonDown tag=",self.tag," sid=",sid
		self.sendControl(127)

	def cursorDrag(self,sid,x,y,z):
		pass

	def cursorUp(self,sid):
		if not self._applysTo(sid):
			return
		if Debug:
			print "buttonUp tag=",self.tag," sid=",sid
		self.sendControl(0)

def mycallback(ev,d):
	global time0, CursorSeq, Cursors
	for m in ev.oscmsg:
		if showalive==False and len(m) >= 3 and ( m[2] == "alive" or m[2] == "fseq" ) :
			continue
		# print "IP=",ev.source_ip," Time=",time.time()-time0," MSG=",m
		addr = m[0]
		# alive = []
		if addr == "/tuio/25Dblb":
			cmd = m[2]
			if cmd == "alive":
				# print timetag()+"alive: ",m[2:]
				pass
			elif cmd == "set":
				# print timetag()+"set: ",m[2:]
				(sid,x,y,z) = m[3:7]
				dragging = sid in Cursors
				Cursors[sid] = (CursorSeq,x,y,z)
				for p in Processors:
					if dragging:
						p.cursorDrag(sid,x,y,z)
					else:
						p.cursorDown(sid,x,y,z)
			elif cmd == "fseq":
				# print timetag()+"fseq: ",m[2:]
				fseq = m[3]
				todelete = []
				for sid in Cursors:
					if Cursors[sid][0] != CursorSeq:
						todelete.append(sid)
				for sid in todelete:
					for p in Processors:
						p.cursorUp(sid)
					del Cursors[sid]
				CursorSeq += 1
			else:
				print "Unrecognized 25Dblb command: ",cmd
		else:
			print "Unrecognized addr: ",addr

if __name__ == '__main__':

	Time0 = time.time()

	Processors.append(RegionControl(11000,"LOWER",  16,17,18))
	Processors.append(RegionControl(12000,"LEFT",  16,17,18))
	Processors.append(RegionControl(13000,"RIGHT",  16,17,18))
	Processors.append(RegionControl(14000,"UPPER",  16,17,18))

	Processors.append(RegionOsc(11000,"/lower"))
	Processors.append(RegionOsc(12000,"/left"))
	Processors.append(RegionOsc(13000,"/right"))
	Processors.append(RegionOsc(14000,"/upper"))

	Processors.append(RegionButton(21000,"LL1",1))
	Processors.append(RegionButton(22000,"LL2",2))
	Processors.append(RegionButton(23000,"LL3",3))
	Processors.append(RegionButton(31000,"LR1",4))
	Processors.append(RegionButton(32000,"LR2",5))
	Processors.append(RegionButton(33000,"LR3",6))
	Processors.append(RegionButton(41000,"UL1",7))
	Processors.append(RegionButton(42000,"UL2",8))
	Processors.append(RegionButton(43000,"UL3",9))
	Processors.append(RegionButton(51000,"UR1",10))
	Processors.append(RegionButton(52000,"UR2",11))
	Processors.append(RegionButton(53000,"UR3",12))

	if len(sys.argv) < 3:
		print "Usage: oscproxy {input-port@input-addr} {output-port@output-addr} {MIDI-output-name}"
		sys.exit(1)

	input_name = sys.argv[1]
	output_name = sys.argv[2]
	midiout_name = sys.argv[3]

	if midiout_name == "" or midiout_name == "None":
		MidiOutput = None
	else:
		Midi.startup()
		m = MidiPypmHardware()
		try:
			o = m.get_output(midiout_name)
			o.open()
		except:
			print "Error opening MIDI output: %s, exception: %s" % (midiout_name,format_exc())
			sys.exit(1)
		MidiOutput = o

	input_port = re.compile(".*@").search(input_name).group()[:-1]
	input_host = re.compile("@.*").search(input_name).group()[1:]
	output_port = re.compile(".*@").search(output_name).group()[:-1]
	output_host = re.compile("@.*").search(output_name).group()[1:]

	print "input host=",input_host," port=",input_port
	print "output host=",output_host," port=",output_port
	oscmon = OscMonitor(input_host,input_port)
	oscmon.setcallback(mycallback,(output_host,output_port))

	OscOutput = OscRecipient(output_host,output_port)

	sleep(10000)
