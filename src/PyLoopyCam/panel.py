import sys
import os
import pygame
import random
import thread
import threading
# import string
 
# from traceback import format_exc

from nthdisplay import *
from nthdraw import *
from nosuch.pertelian import *
from ffff import *
from threading import Thread

NoLcd = True

NPRE = 3
NPOST = 3
NFFGL = 3

Ffff

try:
	import pygame
	from pygame.locals import *
except ImportError, err:
	print "%s failed to load module: %s" % (__file__, err)
	import sys
	sys.exit(1)

class NthPreset:
	def __init__(self, name):
		self.name = name
			
class NthValue:
	
	def __init__(self, panel, default=None):
		self.panel = panel
		self.default = default
		self.realvalue = default
		
	def getvalue(self):
		return self.realvalue
			
	def setvalue(self, v):
		self.realvalue = v
		
	def do(self, v):
		f = self._dofunc
		if f != None:
			return f(v)
		else:
			return ""
		
	def sendvalue(self):
		pass
		
	def send(self, addr, msg=None):
		if self.panel.ffff:
			self.panel.ffff.send(addr, msg)

	def sendlooper(self, addr, msg=None):
		if self.panel.ffff:
			self.panel.ffff.sendlooper(addr, msg)
	
class WindowsValue(NthValue):
	
	def __init__(self, panel):
		NthValue.__init__(self, panel, default=1)
		
	def display(self):
		return "%-2d" % self.getvalue()
	
	def inc(self):
		# if self.getvalue() == 1:
		a1 = self.panel.value["AllLive"]
		print "WINDOW INC AllLive = ",a1.getvalue()," isalllive=",self.panel.isalllive
		if self.panel.isalllive and self.getvalue() == 1:
			# artificially freeze first window to avoid bug in loopycam
			print "ARTIFICIAL FREEZE!"
			self.panel.isalllive = False
			self.panel.ffff.record(1)
			sleep(0.1)
			self.panel.ffff.record(0)
			
		if self.getvalue() < 8:
			self.setvalue(self.getvalue() + 1)
		self.pokeit()
		
	def dec(self):
		if self.getvalue() > 1:
			self.setvalue(self.getvalue() - 1)
		if self.getvalue() == 1:
			print "FORCING ALLLIVE!"
			a1 = self.panel.value["AllLive"]
			a1.inc()
			# self.panel.isalllive = True
		self.pokeit()
		
	def pokeit(self):
		self.send("/looper/setwindows", [self.getvalue()])
	
class RandPosValue(NthValue):
	
	def __init__(self, panel):
		NthValue.__init__(self, panel, default="")
		
	def display(self):
		return "1/All"
	
	def inc(self):
		self.sendlooper("randompositions")
		
	def dec(self):
		self.sendlooper("randomposition1")
		self.sendlooper("nextloop")
	
# class RandomPPValue(NthValue):
# 	
# 	def __init__(sel# f, panel):
# 		NthValue.__init__(self, panel, default="")
# 		
# 	def display(self):
# 		return "Param/All"
# 	
# 	def dec(self):
# 		self.panel.set_all_random_params_of_all_plugins(0, 0)
# 	
# 	def inc(self):
# 		self.panel.set_random_plugins(0, 0)
# 		
# class RandomPP2Value(NthValue):
# 	
# 	def __init__(self, panel):
# 		NthValue.__init__(self, panel, default="")
# 		
# 	def display(self):
# 		return "Param/Pre"
# 	
# 	def inc(self):
# 		pn = self.panel.value["PresetName"]
# 		pn.random_presetname()
# 		self.panel.load_preset()
# 		
# 	def dec(self):
# 		self.panel.set_all_random_params_of_all_plugins(0, 0)
	
class RandPrePostValue(NthValue):
	
	def __init__(self, panel):
		NthValue.__init__(self, panel, default="")
		
	def display(self):
		return "Pre/Post"
	
	def inc(self):
		self.panel.set_random_plugins(0, 0, type="post")
		
	def dec(self):
		self.panel.set_random_plugins(0, 0, type="pre")
		
class RandFfglAllValue(NthValue):
	
	def __init__(self, panel):
		NthValue.__init__(self, panel, default="")
		
	def display(self):
		return "Ffgl/All"
	
	def inc(self):
		self.panel.set_random_plugins(0, 0, type="all")
		
	def dec(self):
		self.panel.set_random_plugins(0, 0, type="ffgl")
	
class PlacementValue(NthValue):
	
	def __init__(self, panel):
		NthValue.__init__(self, panel, default="Full")
		
	def display(self):
		return "Quad/Full"
	
	def inc(self):
		self.setvalue("Full")
		self.sendlooper("fulldisplay")
		
	def dec(self):
		self.setvalue("Quadrant")
		self.sendlooper("quadrantdisplay")
	
class OnOffValue(NthValue):
	
	def __init__(self, panel, name, osc=None, default="On"):
		self.default = default
		NthValue.__init__(self, panel, default=default)
		self.name = name
		self.osc = osc
		
	def display(self):
		return "%3s" % (self.getvalue())
	
	def inc(self):
		self.changevalue("On")
		
	def dec(self):
		self.changevalue("Off")
			
	def changevalue(self,v):
		self.setvalue(v)
		self.sendvalue()
		
	def sendvalue(self):
		if self.osc:
			v = self.getvalue()
			if v == "Off":
				self.send(self.osc, [0])
			else:
				self.send(self.osc, [1])
			
	def toggle(self):
		if self.getvalue() == "On":
			self.dec()
		else:
			self.inc()
	
class FloatValue(NthValue):
	
	def __init__(self, panel, name, osc=None, default=0.5, min=0.0, max=1.0, delta=0.05):
		NthValue.__init__(self, panel, default=default)
		self.min = min
		self.delta = delta
		self.max = max
		self.name = name
		self.osc = osc
		if self.default > self.max:
			self.default = self.max
		if self.default > self.min:
			self.default = self.min
		
	def display(self):
		return "%.3f" % (self.getvalue())
	
	def inc(self):
		v = self.getvalue() + self.delta
		if v > self.max:
			v = self.max
		self.changevalue(v)
		
	def dec(self):
		v = self.getvalue() - self.delta
		if v < self.min:
			v = self.min
		self.changevalue(v)
	
	def changevalue(self, v):
		self.setvalue(v)
		self.sendvalue()
		
	def sendvalue(self):
		if self.osc:
			self.send(self.osc, [self.getvalue()])
		
class IntValue(FloatValue):
	
	def __init__(self, panel, name, osc=None, default=1, min=0, max=100, delta=1):
		self.default = default
		FloatValue.__init__(self, panel, name, osc=osc, default=default, min=min, max=max, delta=delta)
		
	def display(self):
		return "%3d" % (self.getvalue())
	
class NumAllValue(IntValue):
	def __init__(self, panel):
		IntValue.__init__(self, panel, "NumAll", min=0, max=3, delta=1, default=1)
		
	def getvalue(self):
		npre = self.panel.getnplugins("pre")
		npost = self.panel.getnplugins("post")
		nffgl = self.panel.getnplugins("ffgl")
		return min(npre, npost, nffgl)
		
	def setvalue(self, v):
		self.panel.setnplugins(v, "pre")
		self.panel.setnplugins(v, "post")
		if v == 0:
			self.panel.setnplugins(1, "ffgl")
			b = self.panel.butt["ffgl"][0]
			self.panel.set_curr_configmode_and_butt("ffgl", 0, b, updatedisplay=False)
			self.panel.set_curr_plugin("Shift RGB")
			plugin = self.panel.ffff.get_ffgl("Shift RGB")
			if not "Distance" in plugin.param:
					print "Hey, Distance not in plugin.param?"
			else:
				p = plugin.param["Distance"]
				self.panel.set_param_text_and_send(p, "0.0")
		else:
			self.panel.setnplugins(v, "ffgl")
		
class NumPreValue(IntValue):
	def __init__(self, panel):
		IntValue.__init__(self, panel, "NumPre", min=0, max=3, delta=1, default=1)
		
	def getvalue(self):
		return self.panel.getnplugins("pre")
		
	def setvalue(self, v):
		self.panel.setnplugins(v, "pre")
		
class NumPostValue(IntValue):
	def __init__(self, panel):
		IntValue.__init__(self, panel, "NumPost", min=0, max=3, delta=1, default=1)
		
	def getvalue(self):
		return self.panel.getnplugins("post")
		
	def setvalue(self, v):
		self.panel.setnplugins(v, "post")
		
class NumFfglValue(IntValue):
	def __init__(self, panel):
		IntValue.__init__(self, panel, "NumFfgl", min=1, max=3, delta=1, default=1)
		
	def getvalue(self):
		return self.panel.getnplugins("ffgl")
		
	def setvalue(self, v):
		self.panel.setnplugins(v, "ffgl")
		
class SmoothMoveValue(OnOffValue):
	def __init__(self, panel):
		OnOffValue.__init__(self, panel, "MvSmooth", osc="/looper/setsmooth")
		
class MoveAmountValue(IntValue):
	def __init__(self, panel):
		IntValue.__init__(self, panel, "MoveAmount", min=1, max=49, delta=3, default=10, osc="/looper/moveamount")
		
class PrmSpdValue(IntValue):
	def __init__(self, panel):
		IntValue.__init__(self, panel, "PrmSpd", min=1, max=100, delta=5, default=50)
	def setvalue(self, v):
		print "PrmSpdValue v=", v
		self.realvalue = v
		dtmin = 0.0005
		dtmax = 0.04
		self.panel.param_dt = dtmin + ((100 - v) / 100.0) * (dtmax - dtmin)
		
class PrmStpsValue(IntValue):
	def __init__(self, panel):
		IntValue.__init__(self, panel, "PrmStps", min=25, max=500, delta=25, default=200)
	def setvalue(self, v):
		print "PrmStpsValue v=", v
		self.realvalue = v
		self.panel.param_nsteps = v
		
class TrailValue(FloatValue):
	def __init__(self, panel):
		FloatValue.__init__(self, panel, "Trail", osc="/looper/trail", min=0.5, max=0.99, delta=0.02, default=0.75)
		
	def inc(self):
		FloatValue.inc(self)
		if self.realvalue > 0.5:
			t = self.panel.value["TrailOn"]
			t.inc()
		
	def dec(self):
		FloatValue.dec(self)
		if self.realvalue <= 0.5:
			t = self.panel.value["TrailOn"]
			t.dec()
		
class TrailOnValue(OnOffValue):
	def __init__(self, panel):
		OnOffValue.__init__(self, panel, "TrailOn", osc="/looper/dotrail", default="On")
		
class AllLiveValue(OnOffValue):
	def __init__(self, panel):
		OnOffValue.__init__(self, panel, "AllLive", osc="/looper/alllive", default="On")
		
	def inc(self):
		OnOffValue.inc(self)
		self.panel.isalllive = True
		
	def dec(self):
		OnOffValue.dec(self)
		self.panel.isalllive = False
		
class RecBorderValue(OnOffValue):
	def __init__(self, panel):
		OnOffValue.__init__(self, panel, "RBorder", osc="/looper/recborder", default="Off")
		
class ResizeInterpValue(OnOffValue):
	def __init__(self, panel):
		OnOffValue.__init__(self, panel, "ResizeInterp", osc="/looper/setinterp", default="Off")
		
class RevAffectAllValue(OnOffValue):
	def __init__(self, panel):
		OnOffValue.__init__(self, panel, "RevAffectAll", default="On")
		
class ReverseValue(OnOffValue):
	def __init__(self, panel):
		OnOffValue.__init__(self, panel, "Reverse", default="Off")
		
	def inc(self):
		pass
	
	def dec(self):
		pass
		
class BorderValue(OnOffValue):
	def __init__(self, panel):
		OnOffValue.__init__(self, panel, "Border", osc="/looper/border", default="Off")
		
class FlipHValue(OnOffValue):
	def __init__(self, panel):
		OnOffValue.__init__(self, panel, "FlipH", osc="/looper/fliph", default="On")
		
class FlipVValue(OnOffValue):
	def __init__(self, panel):
		OnOffValue.__init__(self, panel, "FlipV", osc="/looper/flipv", default="Off")
		
class XorValue(OnOffValue):
	def __init__(self, panel):
		OnOffValue.__init__(self, panel, "Xor", osc="/looper/xor", default="On")
		
class _GrabMouseValue(OnOffValue):
	def __init__(self, panel):
		OnOffValue.__init__(self, panel, "GrabMouse", default="Off")
		
	def inc(self):
		self.setvalue("On")
		pygame.event.set_grab(True)
		
	def dec(self):
		self.setvalue("Off")
		pygame.event.set_grab(False)
		
class AutoNextValue(OnOffValue):
	def __init__(self, panel):
		OnOffValue.__init__(self, panel, "AutoNxt", osc="/looper/autonext", default="On")
		
class BlackoutValue(OnOffValue):
	def __init__(self, panel):
		OnOffValue.__init__(self, panel, "Blackout", osc="/looper/blackout", default="Off")
		
class PresetSetValue(NthValue):
	
	def __init__(self, panel):
		NthValue.__init__(self, panel, default="techmild")
		self.files = ["techmild", "techmedium", "techwild", "converge", "orange", "tedx", "new", "good", "good_noswrl", "abstract", "basic", "subtle" ]
	
	def display(self):
		return "%s" % (self.getvalue())
	
	def reset(self):
		self.panel.value["PresetName"].reload_presetlist()
		
	def changeit(self, f):
		self.setvalue(f)
		self.panel.value["PresetName"].reload_presetlist()
		
	def inc(self):
		donext = False
		first = None
		done = False
		for f in self.files:
			if first == None:
				first = f
			if donext:
				self.changeit(f)
				done = True
				break
			if f == self.getvalue():
				donext = True
		if not done:
			self.changeit(first)
		
	def dec(self):
		prev = None
		usethis = None
		for f in self.files:
			last = f
			if f == self.getvalue():
				usethis = prev
			prev = f
		if usethis:
			self.changeit(usethis)
		else:
			self.changeit(last)
	
class PresetNameValue(NthValue):
	
	def __init__(self, panel):
		NthValue.__init__(self, panel, default="file1")
		self.files = []
		self.presetset = None
		
	def display(self):
		return "%s" % (self.getvalue())
	
	def random_presetname(self):
		n = len(self.files)
		if n == 0:
			rf = ""
		else:
			rf = self.files[random.randint(0, n - 1)]
		print "Random presetname = ", rf
		self.setvalue(rf)
		
	def reload_presetlist(self):
		presetset = self.panel.get_value_named("PresetSet")
		self.presetset = presetset
		dirname = "presets_%s" % presetset
		list = os.listdir(dirname)
		self.files = []
		for fn in list:
			if fn.endswith(".lpy"):
				fn = string.replace(fn, ".lpy", "")
				self.files.append(fn)
		if len(self.files) > 0:
			self.setvalue(self.files[0])
		else:
			self.setvalue("")
		
	def inc(self):
		donext = False
		for f in self.files:
			if donext:
				self.setvalue(f)
				break
			if f == self.getvalue():
				donext = True
		
	def dec(self):
		prev = None
		for f in self.files:
			if f == self.getvalue():
				break
			prev = f
		if prev:
			self.setvalue(prev)
	
# class PresetLoadValue(NthV# alue):
# 	
# 	def __init__(self,panel):
# 		NthValue.__init__(self,panel,default="")
# 		
# 	def display(self):
# 		return " Preset: Rand! Load!"
# 	
# 	def inc(self):
# 		self.panel.load_preset()
# 		
# 	def dec(self):
# 		pn = self.panel.value["PresetName"]
# 		pn.random_presetname()
# 		self.panel.load_preset()
	
class PresetSaveValue(NthValue):
	
	def __init__(self, panel):
		NthValue.__init__(self, panel, default="")
		self.last_saved = ""
		
	def display(self):
		return "%s" % self.last_saved.replace("presets_new/", "")
	
	def inc(self):
		self.savenew()
		
	def dec(self):
		self.savenew()
		
	def savenew(self):
		fname = self.panel.new_presetfile()
		self.last_saved = fname
		self.panel.write_preset(fname)
		self.panel.value["PresetName"].reload_presetlist()
		
class ParamThread(Thread):
	
	def __init__(self, panel, plugin, p, val):
		Thread.__init__(self)
		self.panel = panel
		self.plugin = plugin
		if plugin == None:
			print "Hey, ParamThread initialized with plugin=None?"
		self.param = p
		self.beginval = float(p.current_val)
		self.endval = float(val)
		self.stepnum = 0
		self.stopme = False
		if p.paramthread != None:
			t = p.paramthread
			t.stop()
			t.join()
		p.paramthread = self
		
	def stop(self):
		self.stopme = True
		
	def run(self):
		plugin1 = self.plugin
		if plugin1 == None:
			print "Hey, ParamThread called with self.plugin=None?"
			return
		while self.stopme == False and self.panel.param_nsteps > 0 and self.stepnum <= self.panel.param_nsteps:
			val = self.beginval + (self.stepnum * (self.endval - self.beginval)) / self.panel.param_nsteps
			if self.plugin == None:
				print "Hey, ParamThread.run sees plugin=None?  initial plugin=", plugin1.name
				break
			self.panel.set_param_text_and_send_now(self.plugin, self.param, "%f" % val)
			self.stepnum += 1
			# print "PARAMTHREAD sleeping!? dt=",self.panel.param_dt
			sleep(self.panel.param_dt)
		if self.param.paramthread != self:
			print "HEY!  run ended but paramthread != self!?"
		self.param.paramthread = None
		
class Actions():
	def __init__(self, panel):
		self.panel = panel
	def name(self):
		s = "%s" % self.__class__
		i = s.rfind(".")
		if i >= 0:
			s = s[i + 1:]
		i = s.find("Actions")
		if i >= 0:
			s = s[0:i]
		return s
	def lcd_refresh(self):
		self.panel.lcd_refresh()
	def ffff(self):
		return self.panel.ffff
	def send(self, msg, args=None):
		self.panel.ffff.send(msg, args)
	def sendlooper(self, meth, args=None):
		self.panel.ffff.sendlooper(meth, args)
	def value(self, nm):
		return self.panel.value[nm]
	def keydown(self, key):
		pass
	def keyup(self, key):
		pass
	def labels(self):
		return ["LoopyCam!", "", "", ""]
	
class TrailActions(Actions):
	def labels(self):
		t1 = self.value("Trail")
		t2 = self.value("TrailOn")
		x1 = self.value("Xor")
		if t2.getvalue() == "On":
			stat = "Trail=%.3f  XOR=%s" % (t1.getvalue(), x1.getvalue())
		else:
			stat = "Trail=Off  XOR=%s" % (x1.getvalue())
		return [None, stat, "/,* = ???,XOR", "-,+ = Less,More"]
	def keydown(self, key):
		t1 = self.value("Trail")
		t2 = self.value("TrailOn")
		x = self.value("Xor")
		if key == "+":
			t1.inc()
		elif key == "-":
			t1.dec()
		elif key == "/":
			print "NO ACTION YET"
		elif key == "*":
			x.toggle()
		self.lcd_refresh()
		
class CommonActions(Actions):
	def labels(self):
		a1 = self.value("AllLive")
		x = self.value("Xor")
		return [None, "Live=%s Xor=%s" % (a1.getvalue(), x.getvalue()), "/,* = Live,XOR", "-,+ = ???,???"]
	def keydown(self, key):
		a = self.value("AllLive")
		x = self.value("Xor")
		if key == "+":
			pass
		elif key == "-":
			pass
		elif key == "/":
			a.toggle()
		elif key == "*":
			x.toggle()
		self.lcd_refresh()
		
class PluginsActions(Actions):
	def labels(self):
		vpre = self.value("NumPre")
		vpost = self.value("NumPost")
		vffgl = self.value("NumFfgl")
		s = "Pre=%d Post=%d Ffgl=%d" % (vpre.getvalue(),vpost.getvalue(),vffgl.getvalue())
		return [None, s, "/,* = OnePre,OnePost", "-,+ = OneFF,OneAll"]
	def keydown(self, key):
		if key == "/":
			v = self.value("NumPre")
			v.setvalue(1)
			self.panel.set_random_plugins(0, 0, type="pre")
		elif key == "*":
			v = self.value("NumPost")
			v.setvalue(1)
			self.panel.set_random_plugins(0, 0, type="post")
		elif key == "-":
			v = self.value("NumFfgl")
			v.setvalue(1)
			self.panel.set_random_plugins(0, 0, type="ffgl")
		elif key == "+":
			v = self.value("NumAll")
			v.setvalue(1)
			self.panel.set_random_plugins(0, 0, type="all")
		
class RandomActions(Actions):
	def labels(self):
		v = self.value("PresetSet")
		vn = self.value("PresetName")
		s = "%s/%s" % (v.getvalue(),vn.getvalue())
		return [None, s,"/,* = Param,Preset", "-,+ = Pos1,PosAll"]
	def keydown(self, key):
		if key == "+":
			self.sendlooper("randompositions")
		elif key == "-":
			self.sendlooper("randomposition1")
			self.sendlooper("nextloop")
		elif key == "/":
			self.panel.set_all_random_params_of_all_plugins(0, 0)
		elif key == "*":
			# self.panel.set_random_plugins(0, 0)
			v = self.value("PresetName")
			v.random_presetname()
			self.panel.load_preset()
			self.lcd_refresh()
		
class PresetActions(Actions):
	def labels(self):
		v1 = self.value("PresetSet")
		v2 = self.value("PresetName")
		return [None, "%s/%s" % (v1.getvalue(), v2.getvalue()), "/,* = Prev,Next", "-,+ = Save,Load"]
	def keydown(self, key):
		v = self.value("PresetName")
		print "PRESETACTION!"
		if key == "+":
			self.panel.load_preset()
		elif key == "-":
			v = self.value("PresetSave")
			v.savenew()
		elif key == "/":
			v.dec()
		elif key == "*":
			v.inc()
		self.lcd_refresh()
		
class LayoutActions(Actions):
	def labels(self):
		w = self.value("Windows")
		return [None, "#Windows=%d"%(w.getvalue()), "/,* = Quad,Full", "-,+ = Windows--,++"]
	def keydown(self, key):
		if key == "/":
			self.sendlooper("quadrantdisplay")
			w = self.value("Windows")
			w.setvalue(4)
		elif key == "*":
			self.sendlooper("fulldisplay")
			w = self.value("Windows")
			w.setvalue(1)
		elif key == "-":
			w = self.value("Windows")
			w.dec()
		elif key == "+":
			w = self.value("Windows")
			w.inc()
		self.lcd_refresh()
			
		
class OptionActions(Actions):
	def labels(self):
		vn = self.panel.values[self.panel.optindex]
		v = self.panel.value[vn]
		return [None, "%s=%s" % (vn, v.display()), "/,* = Prev,Next", "-,+ = Less,More"]
	def keydown(self, key):
		vn = self.panel.values[self.panel.optindex]
		v = self.panel.value[vn]
		if key == "/":
			self.panel.optindex = (self.panel.optindex - 1) % len(self.panel.values)
		elif key == "*":
			self.panel.optindex = (self.panel.optindex + 1) % len(self.panel.values)
		elif key == "-":
			v.dec()
		elif key == "+":
			v.inc()
		self.lcd_refresh()
		
class SpeedActions(Actions):
	def labels(self):
		a = self.value("MoveAmount")
		s = "Move=%s" % a.getvalue()
		return [None, s, "/,* = Move Slow/Fast", "-,+ = Play Slow,Fast"]
	def keydown(self, key):
		if self.value("RevAffectAll").getvalue() == "On":
			doall = True
		else:
			r = random.randint(0, 7)
			doall = False
		a = self.value("MoveAmount")
		if key == "/":
			a.dec()
		elif key == "*":
			a.inc()
		elif key == "-":
			if doall:
				for n in range(8):
					self.send("/looper/playfactor", [n, 0.5])
			else:
				self.send("/looper/playfactor", [r, 0.5])
		elif key == "+":
			if doall:
				for n in range(8):
					self.send("/looper/playfactor", [n, 2.0])
			else:
				self.send("/looper/playfactor", [r, 2.0])
			
		self.lcd_refresh()
		
class MovementActions(Actions):
	def labels(self):
		rev = self.value("Reverse")
		m = self.value("MoveSmooth")
		line2 = "Smooth=%s Rev=%s" % (m.getvalue(), rev.getvalue())
		return [None, line2, "/,* = Smooth,Freeze", "-,+ = RevReset,Togg"]
	def keydown(self, key):
		rev = self.value("Reverse")
		m = self.value("MoveSmooth")
		if self.value("RevAffectAll").getvalue() == "On":
			doall = True
		else:
			r = random.randint(0, 7)
			doall = False
		if key == "/":
			m.toggle()
		elif key == "*":
			for n in range(8):
				self.send("/looper/freeze", [n])
		elif key == "+":
			if rev.getvalue() == "On":
				newv = 0
				rev.setvalue("Off")
			else:
				newv = 1
				rev.setvalue("On")
			if doall:
				for n in range(8):
					self.send("/looper/setreverse", [n, newv])
			else:
				self.send("/looper/setreverse", [r, newv])
		elif key == "-":
			for n in range(8):
				self.send("/looper/playfactorreset", [n])
				self.send("/looper/setreverse", [n, 0])
			rev.setvalue("Off")
			
		self.lcd_refresh()
		
	def keyup(self, key):
		if key == "*":
			for n in range(8):
				self.send("/looper/unfreeze", [n])
		
class ResetActions(Actions):
	def labels(self):
		v = self.value("PresetSet")
		s = "Presets=%s" % v.getvalue()
		return [None, s, "/,* = Blk,PresetSet ","-,+ = Plugins,All"]
	def keydown(self, key):
		if key == "-":
			v = self.value("NumAll")
			v.setvalue(0)
		elif key == "+":
			v = self.value("NumAll")
			v.setvalue(0)
			self.sendlooper("fulldisplay")
			w = self.value("Windows")
			w.setvalue(1)
			a1 = self.value("AllLive")
			a1.inc()
		elif key == "/":
			a = self.value("Blackout")
			a.toggle()
			print "BLACKOUT value=%s" % (a.getvalue())
			a.sendvalue()
			# self.send("/looper/blackout", [n])
		elif key == "old_/":
			a = self.value("AllLive")
			a.inc()
			self.panel.stop_all_params_of_all_plugins()
			print "Restarting loopycam"
			os.system("loopykill.bat")
			os.system("loopystart.bat")
			os.system("loopyactivate.bat")
			self.panel.ffff.hangup_and_reconnect()
			time.sleep(1.0)
			self.panel.ffff.refresh()
			self.panel.send_all_values()
		elif key == "*":
			v = self.value("PresetSet")
			v.inc()
			self.lcd_refresh()
		
class RestartActions(Actions):
	def labels(self):
		return [None, None, "/,* = Save,Restore", "-,+ = Rand,Beginning"]
	def keydown(self, key):
		if key == "-":
			for n in range(8):
				self.ffff().restartrandom(n)
				self.send("/looper/freeze", [n])
		elif key == "+":
			for n in range(8):
				self.ffff().restart(n)
				self.send("/looper/freeze", [n])
		elif key == "/":
			for n in range(8):
				self.ffff().restartsave(n)
		elif key == "*":
			for n in range(8):
				self.ffff().restartrestore(n)
				self.send("/looper/freeze", [n])
	def keyup(self,key):
		for n in range(8):
			self.send("/looper/unfreeze", [n])
		
class RecordingActions(Actions):
	def labels(self):
		a1 = self.value("AllLive")
		t = self.value("TrailOn")
		s = "Live=%s Trail=%s" % (a1.getvalue(),t.getvalue())
		return [None, s, "/,* = Live,Trail ", "-,+ = Rec,Overlay"]
	def keydown(self, key):
		if key == "-":
			self.ffff().record(1)
			self.recording = True
			self.panel.isalllive = False
		elif key == "+":
			self.ffff().recordoverlay(1)
			self.recording = True
			self.panel.isalllive = False
		elif key == "/":
			a1 = self.value("AllLive")
			a1.toggle()
		elif key == "*":
			t = self.value("TrailOn")
			t.toggle()
		self.lcd_refresh()
	def keyup(self, key):
		if key == "-":
			self.ffff().record(0)
			self.recording = False
		elif key == "+":
			self.ffff().recordoverlay(0)
			self.recording = False

class FingerData:
	def __init__(self,tm,x,y,id):
		self.time = tm
		self.x = x
		self.y = y
		self.id = id
	
class NthControlPanel():

	def __init__(self, width, height, flags):

		print "PANEL INIT start"
		self.recording = False
		self.lastpresetnum = 0
		self.init_rand()
		self.save_dir = "."
		self.ffff = None
		self.lcd_status = ""
		self.key_is_down = {}
		self.key_up_mode = {}
		self.param_dt = 0.005
		self.param_nsteps = 500
		self.lcdlines = ["", "", "", ""]
		self.last_xy_action = {}
		self.last_xy_time = 0.0
		self.finger_zero = 0

		self.presets = []
		self.curr_preset = 0
		self.value = {}
		self.values = []
		self.isalllive = True
		
		def addvalue(nm, v):
			self.value[nm] = v
			self.values.append(nm)
			
		# addvalue("Placement",PlacementValue(self))
		addvalue("AllLive", AllLiveValue(self))
		addvalue("AutoNext", AutoNextValue(self))
		addvalue("Blackout", BlackoutValue(self))
		addvalue("Border", BorderValue(self))
		addvalue("FlipH", FlipHValue(self))
		addvalue("FlipV", FlipVValue(self))
		addvalue("MoveSmooth", SmoothMoveValue(self))
		addvalue("MoveAmount", MoveAmountValue(self))
		addvalue("NumAll", NumAllValue(self))
		addvalue("NumFfgl", NumFfglValue(self))
		addvalue("NumPost", NumPostValue(self))
		addvalue("NumPre", NumPreValue(self))
		# addvalue("PresetLoad", PresetLoadValue(self))
		addvalue("PresetName", PresetNameValue(self))
		addvalue("PresetSave", PresetSaveValue(self))
		addvalue("PresetSet", PresetSetValue(self))
		addvalue("PrmStps", PrmStpsValue(self))
		addvalue("PrmSpd", PrmSpdValue(self))
		addvalue("RandPos", RandPosValue(self))
		# addvalue("RandomPP", RandomPPValue(self))
		# addvalue("RandomPP2", RandomPP2Value(self))
		addvalue("RandPrePost", RandPrePostValue(self))
		addvalue("RandFfglAll", RandFfglAllValue(self))
		addvalue("RecBorder", RecBorderValue(self))
		addvalue("ResizeInterp", ResizeInterpValue(self))
		addvalue("RevAffectAll", RevAffectAllValue(self))
		addvalue("Reverse", ReverseValue(self))
		addvalue("Trail", TrailValue(self))
		addvalue("TrailOn", TrailOnValue(self))
		addvalue("Windows", WindowsValue(self))
		addvalue("Xor", XorValue(self))
		addvalue("_GrabMouse", _GrabMouseValue(self))
		
		self.value["PresetName"].reload_presetlist()
		self.optindex = 0
		
		self.modes = {
					"7":"Plugins",
					"4":"Trail",
					"1":"Layout",
					"0":"Random",
					
					"9":"Reset",
					"6":"Speed",
					"3":"Preset",
					".":"Restart",
					
					"8":"Option",
					"5":"Movement",
					"2":"Recording",
					"000":"",
					}
		
		self.actions = {}
		for m in self.modes:
			aname = self.modes[m]
			classname = "%sActions" % aname
			self.actions[m] = globals()[classname](self)
			if self.actions[m] == None:
				print "NO Class!?  %s" % classname
		
		self.sticky_mode = "2"
		self.curr_mode = self.sticky_mode
		# self.curr_param = self.modes[self.curr_mode_index]["params"][0]
		
		# self.keyproc = NthKeyProcessor(self)
		self.scancode2key = {}
		self.scancode2key[69] = "NL"
		self.scancode2key[53] = "/"
		self.scancode2key[181] = "/"
		self.scancode2key[55] = "*"
		self.scancode2key[74] = "-"
		self.scancode2key[78] = "+"
		self.scancode2key[28] = "Enter"
		self.scancode2key[83] = "."
		self.scancode2key[11] = "000"
		self.scancode2key[82] = "0"
		self.scancode2key[79] = "1"
		self.scancode2key[80] = "2"
		self.scancode2key[81] = "3"
		self.scancode2key[75] = "4"
		self.scancode2key[76] = "5"
		self.scancode2key[77] = "6"
		self.scancode2key[71] = "7"
		self.scancode2key[72] = "8"
		self.scancode2key[73] = "9"
		
		for i in self.scancode2key:
			k = self.scancode2key[i]
			self.key_is_down[k] = False
			self.key_up_mode[k] = None
			
		self.keypad_butt = {}
		print "Creating pertelian()"
		self.pertelian = pertelian()
		self.pertelian.open()
		self.pertelian_num = -1
		for n in range(4):
			if self.pertelian.isopen(n):
				self.pertelian_num = n
				break
		if self.pertelian_num < 0:
			print "HEY, no Pertelian LCD is attached!"
		else:
			self.pertelian.backlight(self.pertelian_num, 1)
		self.curr_plugin = None
		self.nparams = 8
		self.debug = False
		self.curr_butt = None
		
		# self.curr_configmode = "pre"
		# self.curr_configmode_index = 0
		
		self.color = Color("black")
		self.bgcolor = Color(0xff, 0xcc, 0x66)
		NthDrawable.set_default_bgcolor(self.bgcolor)
		self.bgdarker = self.darker(self.bgcolor)
		self.highcolor = Color(255, 100, 100)
		self.alertcolor = Color(255, 0, 0)
		self.bgarrows = Color("white")

		self.screen = pygame.display.set_mode((width, height))

		self.dlist = NthDisplayList(self.screen, bgcolor=self.bgcolor)

		pygame.display.set_caption('LoopyCamPanel')

		self.save_primed = False

		self.width = width
		self.height = height

		self.next_control_num = 0

		# sz = (int(width*0.1),int(height*0.1))
		# b = NthButton(Rect((0,0),sz),"Hello")
		# self.add(b,offset=(10,10))
		# self.ui_mode_widget = b
		# b.set_callback(self.toggle_ui_mode,(0,0))

		self.linesz = height / 21

		pwidth = int(width * 0.333)

		sz = (pwidth, int(height))
		offset = (0, 0)
		self.config_pane(sz, offset)

		sz = (pwidth, int(height))
		offset = (pwidth, 0)
		self.param_pane(sz, offset)

		sz = (pwidth, int(height))
		offset = (pwidth * 2, 0)
		self.controller_pane(sz, offset)
		
		self.curr_butt = self.butt["pre"][0]
		self.curr_configmode = "pre"
		self.curr_configmode_index = 0
		
		self.lcd_clear()
		self.lcd_write(1, 2, "LoopyCam Starting")

		return
	
	def send_all_values(self):
		for nm in self.value:
			v = self.value[nm]
			# print "SENDING value nm=",nm," val=",v.getvalue()
			v.sendvalue()
	
	def get_value_named(self, nm):
		if not nm in self.value:
			print "HEY!  No value named nm=", nm
			return "BADVALUE"
		v = self.value[nm]
		return v.getvalue()
	
	def pad(self, s):
		b = "                    "
		return s + b[0:20 - len(s)]
		
	def lcd_refresh(self):
		# self.lcd_clear()
		# print "LCD_REFRESH time=",time.time()
		m = self.curraction()
		if m == None:
			print "NO curraction = ", self.curr_mode
			return
		# nw = self.value["Windows"].getvalue()
		lns = ["", "", "", ""]
		lines = m.labels()
		if lines == None or len(lines) != 4:
			print "Bad labels from class = ", m.name()
			return
		
		if lines[0] == None:
			lns[0] = self.pad("LoopyCam! %10s" % m.name())
		else:
			lns[0] = self.pad(lines[0])
			
		if lines[1] == None:
			lns[1] = self.pad("")
		else:
			lns[1] = self.pad(lines[1])
			
		if lines[2] == None:
			lns[2] = self.pad("")
		else:
			self.lcd_write(2, 0, self.pad(lines[2]))
			
		if lines[3] == None:
			lns[3] = self.pad("")
		else:
			self.lcd_write(3, 0, self.pad(lines[3]))
			
		for i in range(4):
			if self.lcdlines[i] != lns[i]:
				self.lcdlines[i] = lns[i]
				self.lcd_write(i, 0, lns[i])
		return
			
	def set_status(self, msg):
		self.lcd_status = msg
		# print "LCD set_status =",self.lcd_status
		
	def set_ffff(self, ffff):
		print "PANEL set_ffff!!"
		self.ffff = ffff
		# self.keyproc.set_ffff(ffff)

	def config_pane(self, sz, offset):

		self.butt = {}
		self.butt["pre"] = []
		self.butt["post"] = []
		self.butt["ffgl"] = []
		
		th = self.linesz
		b = NthButton(Rect((0, 0), (sz[0], th)), "Configuration")
		self.add(b, offset=offset)

		b = NthText(Rect((0, 0), (sz[0], th)), "FF Pre", border=False, bgcolor=self.bgdarker)
		self.add(b, offset=(0, th))

		for n in range(NPRE):
			y = (n + 2) * th
			self.tiny_buttons("pre", n, sz, th, y)
			
		b = NthText(Rect((0, 0), (sz[0], th)), "Loops", border=False, bgcolor=self.bgdarker)
		self.add(b, offset=(0, 5 * th))

		xoff = sz[0] * 0.1
		bsz = (sz[0] / 2, th)
		for n in range(8):
			y = (n + 6) * th

			b = NthText(Rect((0, 0), (xoff, th)), "%d" % n, border=False)
			self.add(b, offset=(0, y))

			b = NthButton(Rect((0, 0), bsz), "None")
			self.add(b, offset=(xoff, y))

		b = NthText(Rect((0, 0), (sz[0], th)), "FF Post", border=False, bgcolor=self.bgdarker)
		self.add(b, offset=(0, 14 * th))

		for n in range(NPOST):
			y = (n + 15) * th
			self.tiny_buttons("post", n, sz, th, y)

		b = NthText(Rect((0, 0), (sz[0], th)), "FFGL", border=False, bgcolor=self.bgdarker)
		self.add(b, offset=(0, 18 * th))

		for n in range(NFFGL):
			y = (n + 19) * th
			self.tiny_buttons("ffgl", n, sz, th, y)
			
	def tiny_buttons(self, mode, n, sz, th, y):
		
		xoff = sz[0] * 0.1
		b = NthText(Rect((0, 0), (xoff, th)), "%d" % n, border=False)
		self.add(b, offset=(0, y))

		bsz = (sz[0] / 2, th)
		b = NthButton(Rect((0, 0), bsz), "None")
		self.add(b, offset=(xoff, y))
		b.set_callback(self.plugin_set_current, (mode, n, b))
		
		self.butt[mode].append(b)
		
		b = NthButton(Rect((0, 0), (th, th)), "R")
		self.add(b, offset=(sz[0] - 3 * th, y))
		b.set_callback(self.set_random_plugin, (mode, n, b))
			
		b = NthButton(Rect((0, 0), (th, th)), "P")
		self.add(b, offset=(sz[0] - 2 * th, y))
		b.set_callback(self.set_random_params, (mode, n, b))

		b = NthButton(Rect((0, 0), (th, th)), "X")
		self.add(b, offset=(sz[0] - 1 * th, y))
		b.set_callback(self.set_none_plugin, (mode, n, b))
		
	def  set_preff(self, n, nm):
		if nm != self.butt["pre"][n].text:
			self.butt["pre"][n].set_text(nm)
			self.refresh()
			
	def set_postff(self, n, nm):
		if nm != self.butt["post"][n].text:
			self.butt["post"][n].set_text(nm)
			self.refresh()
			
	def set_postffgl(self, n, nm):
		if n > 2:
			print "IGNORING set_postffgl for n=", n
			return
		if nm != self.butt["ffgl"][n].text:
			self.butt["ffgl"][n].set_text(nm)
			self.refresh()
		
	def got_xy(self,x,y,id):
		tm = time.time()
		f = FingerData(tm,x,y,id)
		print "GOTXY! id=",id," xy=",x,y," time=",time.time()
		
		if tm > (self.last_xy_time+2.0):
			self.finger_zero = id
			print "RESET FINGER_ZERO TO ",id
			
		self.last_xy_time = tm
			
		if id < self.finger_zero:
			self.finger_zero = id
			
		# self.set_all_random_params_of_all_plugins(0, 0)
		# self.lcd_refresh()
		b = self.butt["ffgl"][0]

		# THIS IS THE THING THAT'S SLOWING IT DOWN!
		self.set_curr_configmode_and_butt("ffgl", 0, b, updatedisplay=False)
		p = self.curr_plugin
		# paramnum = ((id - self.finger_zero)*2) % p.num_params()
		paramnum = 0
		if not paramnum in self.last_xy_action:
			self.last_xy_action[paramnum] = tm
		if tm > (self.last_xy_action[paramnum]+0.05):
			print "XY ACTION! id=",id," xy=",x,y,"  PARAMNUM=",paramnum," id=",id
			param = p.get_param_byindex(paramnum)
			if param:
				self.set_param_text_and_send_now(p,param,"%lf"%x)
			self.last_xy_action[paramnum] = tm
			self.ffff.send_plugin_param(p, param)
		paramnum += 1
		if not paramnum in self.last_xy_action:
			self.last_xy_action[paramnum] = tm
		if tm > (self.last_xy_action[paramnum]+0.1):
			param = p.get_param_byindex(paramnum)
			if param:
				self.set_param_text_and_send_now(p,param,"%lf"%y)
				# self.ffff.send_plugin_param(p, param)
			self.last_xy_action[paramnum] = tm
		
		self.lcd_refresh()

	def set_random_params(self, d, ev, random_amount=1.0):
		self.plugin_set_current(d, ev)
		self.set_all_params(random_amount=random_amount, updatedisplay=False)
		self.lcd_refresh()
		
	def init_rand(self):
		self.randnums = []
		for n in range(100):
			self.randnums.append(0.0)
		self.randpos = 0
		self.randnums[self.randpos] = time.time()
			
	def start_next_rand(self):
		seed = self.randnums[self.randpos]
		# print "START_NEXT_RAND randpos=%d seed=%f" % (self.randpos,seed)
		random.seed(seed)
		self.randpos += 1
		if self.randpos >= len(self.randnums):
			self.randpos = 0
		self.randnums[self.randpos] = time.time()
		
	def previous_rand(self, d, ev):
		self.randpos -= 1
		if self.randpos < 0:
			self.randpos += len(self.randnums)
		self.randpos -= 1
		if self.randpos < 0:
			self.randpos += len(self.randnums)
		# print "PREV randomND!  randpos is now ",self.randpos
		
	def stop_all_params_of_all_plugins(self):
		print "STOP_ALL_PARAMS_OF_ALL_PLUGINS!"
		for i in range(0, self.ffff.ffgl_nplugins()):
			plugin = self.ffff.find_ffgl_byindex(i)
			self.stop_params_of_plugin(plugin)
		for i in range(0, self.ffff.ff_nplugins()):
			plugin = self.ffff.find_ff_byindex(i)
			self.stop_params_of_plugin(plugin)
			
	def stop_params_of_plugin(self,plugin):
		for nm in plugin.param:
			p = plugin.param[nm]
			t = p.paramthread
			if t:
				t.stop()
				t.join()
			
	def set_all_random_params_of_current(self, d1, d2, random_amount=1.0):
		self.start_next_rand()
		self.set_all_params(random_amount=random_amount)
		
	def set_all_random_params_of_all_plugins(self, d1, d2, random_amount=1.0):
		print "all_random_params, random_amount=", random_amount
		self.start_next_rand()
		lastb = None
		for n in range(NPRE):
			b = self.butt["pre"][n]
			b.set_bgcolor(self.bgcolor)
			if b.text != "None":
				self.set_curr_configmode_and_butt("pre", n, b, updatedisplay=False)
				self.set_all_params(random_amount=random_amount, updatedisplay=False)
				lastb = b
				
		for n in range(NPOST):
			b = self.butt["post"][n]
			b.set_bgcolor(self.bgcolor)
			if b.text != "None":
				self.set_curr_configmode_and_butt("post", n, b, updatedisplay=False)
				self.set_all_params(random_amount=random_amount, updatedisplay=False)
				lastb = b
				
		for n in range(NFFGL):
			b = self.butt["ffgl"][n]
			b.set_bgcolor(self.bgcolor)
			if b.text != "None":
				self.set_curr_configmode_and_butt("ffgl", n, b, updatedisplay=False)
				self.set_all_params(random_amount=random_amount, updatedisplay=False)
				lastb = b
				
		# we assume self.curr_plugin is set to the last randomized plugin
		if lastb != None:
			lastb.set_bgcolor(self.highcolor)

		# self.set_curr_configmode_and_butt(save_mode, save_mode_index, save_butt)

	def set_all_default_params(self, d, ev):
		self.set_all_params()
		
	def write_plugin(self, f, plugin):
		for nm in plugin.param:
			p = plugin.param[nm]
			f.write("param:%s:%s\n" % (nm, p.current_val))
			
	def preset_fullpath(self, dir, nm):
		return "presets_%s/%s.lpy" % (dir, nm)
		
	def new_presetfile(self):
		while True:
			nm = "%02d" % self.lastpresetnum
			fname = self.preset_fullpath("new", nm)
			if not os.path.exists(fname):
				print "NEW_PRESETFILE=", fname
				return fname
			self.lastpresetnum += 1
			
	def write_preset(self, fname):
		f = open(fname, "wb")
		for n in range(NPRE):
			b = self.butt["pre"][n]
			f.write("pre:%d:%s\n" % (n, b.text))
			self.write_plugin(f, self.ffff.get_ff(b.text))
		for n in range(NPOST):
			b = self.butt["post"][n]
			f.write("post:%d:%s\n" % (n, b.text))
			self.write_plugin(f, self.ffff.get_ff(b.text))
		for n in range(NFFGL):
			b = self.butt["ffgl"][n]
			f.write("ffgl:%d:%s\n" % (n, b.text))
			self.write_plugin(f, self.ffff.get_ffgl(b.text))
		f.close()
		
	def load_preset(self):
		set = self.get_value_named("PresetSet")
		fn = self.get_value_named("PresetName")
		fname = self.preset_fullpath(set, fn)
		try:
			self.read_preset(fname)
			print "FORCING ALLLIVE!"
		except:
			print "Error in loading fname=%s err=%s" % (fname,format_exc())
			
	def load_clear_preset(self):
		fname = self.preset_fullpath("basic", "clear")
		try:
			self.read_preset(fname)
		except:
			print "Unable to load fname=", fname
			
		# plugin = self.ffff.get_ffgl("Shift RGB")
		# if not "Distance" in plugin.param:
		#	print "Hey, Distance not in plugin.param?"
		# else:
		#	p = plugin.param["Distance"]
		#	self.set_param_text_and_send(p,"0.0")
		
	def read_preset(self, fname):
		f = open(fname, "rb")
		self.curr_plugin = None
		butt = None
		print "READ_PRESET fname=",fname
		for line in f.readlines():
			print "line=",line
			line = string.replace(line, "\n", "")
			words = line.split(":")
			# print "LINE=",line," words=",words
			mode = words[0]
			if mode == "pre":
				n = int(words[1])
				name = words[2]
				if name != "None":
					print "calling get_ff name=",name
					print "calling ffff=",self.ffff
					self.curr_plugin = self.ffff.get_ff(name,create=True)
					print "after, curr_plugin=",self.curr_plugin
				else:
					self.curr_plugin = None
				print "setting text to name=",name
				butt = self.butt["pre"][n]
				butt.set_text(name)
				print "calling change_preff n=",n," name=",name
				self.ffff.change_preff(n, name)
			elif mode == "post":
				n = int(words[1])
				name = words[2]
				if name != "None":
					self.curr_plugin = self.ffff.get_ff(name,create=True)
				else:
					self.curr_plugin = None
				butt = self.butt["post"][n]
				butt.set_text(name)
				self.ffff.change_postff(n, name)
			elif mode == "ffgl":
				n = int(words[1])
				name = words[2]
				if name != "None":
					self.curr_plugin = self.ffff.get_ffgl(name,create=True)
				else:
					self.curr_plugin = None
				butt = self.butt["ffgl"][n]
				butt.set_text(name)
				self.ffff.change_ffgl(n, name)
			elif mode == "param":
				paramname = words[1]
				paramval = words[2]
				print "TJT A curr_plugin=",self.curr_plugin
				p = self.curr_plugin.param[paramname]
				print "TJT B"
				self.set_param_text_and_send(p, paramval)
				print "TJT C"
		print "Last line"
		f.close()
		self.isalllive = True
		
	def getnplugins(self, plugtype):
		np = 0
		butt = self.butt
		for n in range(len(butt[plugtype])):
			if butt[plugtype][n].text != "None":
				np += 1
		return np
		
	def setnplugins(self, v, plugtype):
		nactive = 0
		butt = self.butt
		nbutt = len(butt[plugtype])
		for n in range(nbutt):
			if butt[plugtype][n].text != "None":
				nactive += 1
		for n in range(nbutt - 1, -1, -1):
			b = butt[plugtype][n]
			if b.text != "None" and nactive > v:
				nactive -= 1
				self.set_curr_configmode_and_butt(plugtype, n, b, updatedisplay=False)
				self.set_curr_plugin("None")
		for n in range(nbutt):
			b = butt[plugtype][n]
			if b.text == "None" and nactive < v:
				nactive += 1
				self.set_curr_configmode_and_butt(plugtype, n, b, updatedisplay=False)
				self.set_random_plugin(0, 0)
		self.refresh()
		
	def set_random_plugins(self, d, ev, type="all"):
		print "PANEL set_random_plugins"
		lastb = None
		self.start_next_rand()
		if type == "all" or type == "pre":
			for n in range(NPRE):
				b = self.butt["pre"][n]
				b.set_bgcolor(self.bgcolor)
				if b.text != "None":
					self.set_curr_configmode_and_butt("pre", n, b, updatedisplay=False)
					self.set_random_plugin(0, 0)
					lastb = b
		if type == "all" or type == "post":
			for n in range(NPOST):
				b = self.butt["post"][n]
				b.set_bgcolor(self.bgcolor)
				if b.text != "None":
					self.set_curr_configmode_and_butt("post", n, b, updatedisplay=False)
					self.set_random_plugin(0, 0)
					lastb = b
		if type == "all" or type == "ffgl":
			for n in range(NFFGL):
				b = self.butt["ffgl"][n]
				b.set_bgcolor(self.bgcolor)
				if b.text != "None":
					self.set_curr_configmode_and_butt("ffgl", n, b, updatedisplay=False)
					self.set_random_plugin(0, 0)
					lastb = b
		# self.set_curr_configmode_and_butt(save_mode, save_mode_index, save_butt)
		if lastb != None:
			lastb.set_bgcolor(self.highcolor)
		self.set_params_current()
		
	def set_none_plugin(self, d, ev):
		(mode, n, b) = d
		self.plugin_set_current(d, 0)
		self.curr_configmode = mode
		self.curr_configmode_index = n
		plugin = self.ffff.get_ff("None",create=True)
		self.curr_plugin = plugin
		self.set_params_current()
		self.set_curr_plugin(plugin.name)
		self.lcd_refresh()
		
	def set_random_plugin1(self, d, ev):
		self.start_next_rand()
		self.set_random_plugin(d, ev)
		
	def set_random_plugin(self, d, ev):
		print "set_random_plugin"
		if d == 0:
			self.curr_plugin_random()
		else:
			self.plugin_set_current(d, 0)
			self.curr_plugin_random()
			
	def curr_plugin_random(self):
		if self.curr_configmode == "ffgl":
			i = random.randint(1, self.ffff.ffgl_nplugins() - 1)
			plugin = self.ffff.find_ffgl_byindex(i)
		else:
			i = random.randint(1, self.ffff.ff_nplugins() - 1)
			plugin = self.ffff.find_ff_byindex(i)
		# print "   random plugin ff=", plugin.name
		if plugin.name == "None":
			print "Hey, random plugin = None?"
		self.curr_plugin = plugin
		self.set_params_current()
		self.set_curr_plugin(plugin.name)
		
	def set_all_params(self, random_amount=0.0, updatedisplay=True):
		if self.curr_plugin == None:
			print "NO CURRENT PLUGIN, set_all_params ignored"
			return
		plugin = self.curr_plugin
		for nm in plugin.param:
			p = plugin.param[nm]
			if random_amount > 0.0:
				v = (float(p.current_val) + random_amount*random.random()) % 1.0
				val = "%lf" % v
				# print "RANDOMIZED param=",p.name," amt=",random_amount," newv=",v
			else:
				val = p.default_val
			# print "SET_ALL_PARAMS plugin=",plugin.name," param=",p.name
			self.set_param_text_and_send(p, val)
		
		if updatedisplay:
			self.refresh()
		# self.ffff.send_plugin_params(self.curr_plugin)
		
	def set_one_param(self, index=0, value=0.0):
		if self.curr_plugin == None:
			print "NO CURRENT PLUGIN, set_one_param ignored"
			return
		plugin = self.curr_plugin
		for nm in plugin.param:
			p = plugin.param[nm]
			if p.index != index:
				continue
			self.set_param_text_and_send_now(plugin, p, value)
		
		# self.ffff.send_plugin_params(self.curr_plugin)
		
	def set_param_text_and_send_now(self, plugin, p, val):

		if plugin.name == "Glow":
			if p.name == "Floor":
				ceil = plugin.get_param("Ceiling")
				if float(val) > float(ceil.current_val):
					# print "Glow.Floor too high!"
					return
			elif p.name == "Ceiling":
				floor = plugin.get_param("Floor")
				if float(val) < float(floor.current_val):
					# print "Glow.Ceiling too low!"
					return
		# print "SET_PARAM_NOW plugin=",plugin.name," param=",p.name
		p.set_current(val)
		# print "Sending plugin=",plugin.name," param=",p.name," val=",val
		if plugin == None:
			print "Hmm, plugin==None in set_param_text_and_send_now?"
			return
		if self.curr_plugin != None and plugin.name == self.curr_plugin.name and p.index < len(self.butt_paramval):
			self.butt_paramval[p.index].set_text(val)
			# print "SETTING TEXT of button! p.index=",p.index," val=",val
		self.ffff.send_plugin_param(plugin, p)
			
	def set_param_text_and_send(self, p, val):
		print "SET_PARAM_TEXT_AND_SEND, val=",val,"  current=",p.current_val
		if self.param_dt <= 0.0 or self.param_nsteps <= 1:
			print "set_param B"
			self.set_param_text_and_send_now(self.curr_plugin, p, val)
			print "set_param C"
		else:
			print "set_param D"
			print "curr_plugin=",self.curr_plugin," p=",p," val=",val
			t = ParamThread(self, self.curr_plugin, p, val)
			t.start()
			print "set_param E"

	def set_params_current(self):
		
		for n in range(0, self.nparams):
			self.butt_paramname[n].visible = False
			self.butt_paramval[n].visible = False
			
		if self.curr_plugin != None:
			self.butt_plugin_name.set_text(self.curr_plugin.name)
			for nm in self.curr_plugin.param:
				p = self.curr_plugin.param[nm]
				i = p.index
				if i < len(self.butt_paramname):
					self.butt_paramname[i].set_text(nm)
					self.butt_paramval[i].set_text(p.current_val)
					self.butt_paramname[i].visible = True
					self.butt_paramval[i].visible = True

	def plugin_set_current(self, d, butt2, dorefresh=True):
		(mode, n, dummy) = d
		b = self.butt[mode][n]
		# print "FFPRE_CURRENT n=", n, " b=", b, " mode=", mode
		if self.curr_butt != None:
			self.curr_butt.set_bgcolor(self.bgcolor)  # reset current bgcolor
		self.set_curr_configmode_and_butt(mode, n, b)
		if dorefresh:
			self.refresh()
		
	def set_curr_configmode_and_butt(self, mode, n, b, updatedisplay=True):
		self.curr_configmode = mode 
		self.curr_butt = b
		self.curr_configmode_index = n
		if mode == "ffgl":
			self.curr_plugin = self.ffff.get_ffgl(b.text)
		else:
			self.curr_plugin = self.ffff.get_ff(b.text)
		# print "set_curr  PLUGIN =",self.curr_plugin.name
		if updatedisplay:
			b.set_bgcolor(self.highcolor)
			self.set_params_current()
		
	def param_pane(self, sz, offset):

		self.butt_paramname = []
		self.butt_paramval = []
		th = self.linesz
		bwidth = sz[0] / 3
		lmargin = sz[0] / 9
		
		b = NthRect(Rect((0, 0), (sz)))
		b.set_color(self.bgdarker)
		self.add(b, offset=offset)

		b = NthButton(Rect((0, 0), (sz[0], th)), "Parameters")
		self.add(b, offset=offset)

		b = NthButton(Rect((0, 0), (bwidth, th)), "Rand Params")
		self.add(b, offset=(offset[0] + lmargin, offset[1] + sz[1] - 6 * th))
		b.set_callback(self.set_all_random_params_of_current, 0)

		b = NthButton(Rect((0, 0), (bwidth, th)), "Defaults")
		self.add(b, offset=(offset[0] + lmargin * 2 + bwidth, offset[1] + sz[1] - 6 * th))
		b.set_callback(self.set_all_default_params, 0)

		b = NthButton(Rect((0, 0), (bwidth, th)), "Rand Params All")
		self.add(b, offset=(offset[0] + lmargin, offset[1] + sz[1] - 4 * th))
		b.set_callback(self.set_all_random_params_of_all_plugins, 0)

		b = NthButton(Rect((0, 0), (bwidth, th)), "Rand Plugin")
		self.add(b, offset=(offset[0] + lmargin * 2 + bwidth, offset[1] + sz[1] - 4 * th))
		b.set_callback(self.set_random_plugin1, 0)

		b = NthButton(Rect((0, 0), (bwidth, th)), "Previous Rand")
		self.add(b, offset=(offset[0] + lmargin, offset[1] + sz[1] - 2 * th))
		b.set_callback(self.previous_rand, 0)
		
		b = NthButton(Rect((0, 0), (bwidth, th)), "Rand Plugins All")
		self.add(b, offset=(offset[0] + lmargin * 2 + bwidth, offset[1] + sz[1] - 2 * th))
		b.set_callback(self.set_random_plugins, 0)

		y = offset[1] + 2 * th
		b = NthText(Rect((0, 0), (bwidth, th)), "Plugin", border=False)
		self.add(b, offset=(offset[0] + lmargin, y))
			
		b = NthButton(Rect((0, 0), (bwidth, th)), "")
		self.add(b, offset=(offset[0] + bwidth + lmargin * 2, y))
		self.butt_plugin_name = b
		b.set_callback(self.plugin_name_change, 0)
		
		for n in range(self.nparams):
			y = offset[1] + (n + 4) * (th * 1.1)
			pb = NthText(Rect((0, 0), (bwidth, th)), "Param%d" % n, border=False)
			self.add(pb, offset=(offset[0] + lmargin, y))
			self.butt_paramname.append(pb)
			pb.set_callback(self.paramname_down, (pb))
			
			vb = NthButton(Rect((0, 0), (bwidth, th)), "Val%d" % n)
			self.add(vb, offset=(offset[0] + bwidth + lmargin * 2, y))
			self.butt_paramval.append(vb)
			vb.set_callback(self.paramval_down, (pb, vb))
			
	def plugin_name_change(self, d, ev):
		if self.curr_plugin == None:
			self.curr_plugin = self.ffff.get_ff("None")
		if self.curr_configmode == "ffgl":
			plugin = self.ffff.get_ffgl(self.curr_plugin.name)
		else:
			plugin = self.ffff.get_ff(self.curr_plugin.name)
		if plugin == None:
			print "plugin_name_change, plugin=None for plugin.name=", self.curr_plugin.name
			return
		i = plugin.plugin_index
		# print "plugin name change, plugin index=", i, "  curr_configmode=", self.curr_configmode
		if ev.relpos[0] > 0.5:
			i += 1
		else:
			i -= 1
		if self.curr_configmode == "ffgl":
			i = i % self.ffff.ffgl_nplugins()
			# print "   new index=", i
			plugin = self.ffff.find_ffgl_byindex(i)
		else:
			i = i % self.ffff.ff_nplugins()
			# print "   new index=", i
			plugin = self.ffff.find_ff_byindex(i)
		# print "   new ff=", plugin.name
		self.curr_plugin = plugin
		self.set_params_current()
		self.set_curr_plugin(plugin.name)
		
	def set_curr_plugin(self, nm):
		i = self.curr_configmode_index
		m = self.curr_configmode
		if m == "pre":
			self.butt["pre"][i].set_text(nm)
			self.ffff.change_preff(i, nm)
		elif m == "post":
			self.butt["post"][i].set_text(nm)
			self.ffff.change_postff(i, nm)
		elif m == "ffgl":
			self.butt["ffgl"][i].set_text(nm)
			self.ffff.change_ffgl(i, nm)
		self.refresh()
		
	def paramname_down(self, d, ev):
		pb = d[0]
		print "param name =", pb.text
		
	def paramval_down(self, d, ev):
		pb = d[0]
		vb = d[1]
		plugin = self.curr_plugin
		if plugin == None:
			return
		if not pb.text in plugin.param:
			print "No such parameter (", pb.text, ") in plugin ", plugin.name, " ??"
			return
		p = plugin.param[pb.text]
		print "curr param val =", p.current_val
		v = float(vb.text)
		if ev.relpos[0] > 0.9:
			v += 0.2
		elif ev.relpos[0] > 0.8:
			v += 0.1
		elif ev.relpos[0] > 0.6:
			v += 0.05
		elif ev.relpos[0] > 0.5:
			v += 0.01
		elif ev.relpos[0] < 0.1:
			v -= 0.2
		elif ev.relpos[0] < 0.2:
			v -= 0.1
		elif ev.relpos[0] < 0.4:
			v -= 0.05
		elif ev.relpos[0] < 0.5:
			v -= 0.01
		else:
			print "SHOULD NOT GET HERE!?"
			
		if v > 1.0:
			v = 1.0
		elif v < 0.0:
			v = 0.0
		s = "%.5f" % v
		vb.set_text(s)
		# p.set_current(s)
		# self.ffff.send_plugin_param(self.curr_plugin, p)
		self.set_param_text_and_send(p, s)
		self.refresh()
		
	def one_letter(self, sz, offset, row, col):

		xmargin = 0
		ymargin = self.linesz * 2
		kwidth = sz[0] / 20
		kheight = kwidth * 2
		b = NthText(Rect((0, 0), (kwidth, kheight)), " ", border=False, bgcolor=self.bgdarker)
		self.add(b, offset=(offset[0] + xmargin + col * kwidth, offset[1] + ymargin + row * kheight))
		self.letters.append(b)
		
	def set_letter(self, row, col, letter):
		i = row * 20 + col
		self.letters[i].set_text(letter)

	def lcd_write(self, row, col, s):
		origcol = col
		if self.pertelian_num >= 0:
			self.pertelian.write(self.pertelian_num, s, col, row)
		for c in s:
			if col >= 20:
				print "HEY, WRITING LCD past 20 cols!  col=", origcol, " s=<<", s, ">>"
				return
			self.set_letter(row, col, c)
			col += 1
		
	def lcd_clear(self):
		if self.pertelian_num >= 0:
			self.pertelian.clear(self.pertelian_num)
			# self.pertelian.backlight(self.pertelian_num, 1)
		for row in range(0, 4):
				self.lcd_write(row, 0, "                    ")
		
	def one_key(self, sz, offset, keyname, row, col, height=1):
		xmargin = sz[0] / 10
		ymargin = self.linesz * 7
		kwidth = sz[0] / 5
		kheight = kwidth
		b = NthButton(Rect((0, 0), (kwidth, kheight * height)), keyname)
		self.add(b, offset=(offset[0] + xmargin + col * (kwidth * 1.1), offset[1] + ymargin + row * (kheight * 1.1)))
		b.set_callback(self.keypad_down, (keyname, b))
		b.set_callback_up(self.keypad_up, (keyname, b), strict=True)
		self.keypad_butt[keyname] = b
		
	def keydown(self, key):
		self.key_is_down[key] = True
		b = self.keypad_butt[key]
		b.set_bgcolor(self.highcolor)
		
		if key in self.modes:
			self.curr_mode = key
			self.lcd_refresh()
		else:
			m = self.curraction()
			m.keydown(key)
			self.key_up_mode[key] = m
		
	def curraction(self):
		return self.actions[self.curr_mode]
			
	def keyup(self, key):
		# print "KEYUP key=",key
		self.key_is_down[key] = False
		b = self.keypad_butt[key]
		b.set_bgcolor(self.bgcolor)
		# print "KEY ",key," UP"
		
		if key in self.modes:
			if self.key_is_down["Enter"]:
				print "STICKY!  staying in mode ", self.curr_mode
				self.sticky_mode = self.curr_mode
			else:
				self.curr_mode = self.sticky_mode
				self.lcd_refresh()
		else:
			if self.key_up_mode[key] != None:
				m = self.key_up_mode[key]
			else:
				m = self.curraction()
			self.key_up_mode[key] = None
			if m == None:
				print "No curraction value for ", self.curr_mode
				return
			m.keyup(key)
		
	def keypad_down(self, d, ev):
		self.keydown(d[0])
		
	def keypad_up(self, d):
		self.keyup(d[0])
		
	def controller_pane(self, sz, offset):

		self.letters = []
		
		b = NthRect(Rect((0, 0), (sz)))
		b.set_color(self.bgdarker)
		self.add(b, offset=offset)

		th = self.linesz
		b = NthButton(Rect((0, 0), (sz[0], th)), "Controller")
		self.add(b, offset=offset)

		b = NthButton(Rect((0, 0), (sz[0] / 2, th)), "Refresh")
		self.add(b, offset=(offset[0] + sz[0] / 4, offset[1] + sz[1] - 2 * th))
		b.set_callback(self.control_refresh, 0)
		
		for row in range(0, 4):
			for col in range(0, 20):
				self.one_letter(sz, offset, row, col)
				
		self.one_key(sz, offset, "+", 1, 3, height=2.1)
		self.one_key(sz, offset, "Enter", 3, 3, height=2.1)
		
		self.one_key(sz, offset, "NL", 0, 0)
		self.one_key(sz, offset, "/", 0, 1)
		self.one_key(sz, offset, "*", 0, 2)
		self.one_key(sz, offset, "-", 0, 3)
		
		self.one_key(sz, offset, "7", 1, 0)
		self.one_key(sz, offset, "8", 1, 1)
		self.one_key(sz, offset, "9", 1, 2)
		
		self.one_key(sz, offset, "4", 2, 0)
		self.one_key(sz, offset, "5", 2, 1)
		self.one_key(sz, offset, "6", 2, 2)
		
		self.one_key(sz, offset, "1", 3, 0)
		self.one_key(sz, offset, "2", 3, 1)
		self.one_key(sz, offset, "3", 3, 2)
		
		self.one_key(sz, offset, "0", 4, 0)
		self.one_key(sz, offset, "000", 4, 1)
		self.one_key(sz, offset, ".", 4, 2)
		
		self.lcd_clear()
		# self.lcd_write(1,5,"LoopyCam")
		
	def control_refresh(self, arg1, arg2):
		print "CONTROL refresh arg1=", arg1, " arg2=", arg2
		self.ffff.refresh()

	def refresh(self):
		# print "control panel REFRESH"
		self.dlist.refresh()
		self.lcd_refresh()

	def preset_save_down(self, b, button):
		if self.save_primed:
			self.save_primed = False
			color = self.bgcolor
		else:
			self.save_primed = True
			color = self.highcolor
		b.set_bgcolor(color)

	def add(self, item, offset=None):
		self.dlist.add(item, offset=offset)

	def darker(self, color):
		return (color[0] - 40, color[1] - 40, color[2] - 40)

	def top_pane_visible(self, vis):
		for b in self.topwidgets:
			b.visible = vis

	def message(self, msg):
		print "MSG: ", msg
		
	def shutdown(self):
		self.running = False
		self.lcd_clear()
		self.lcd_write(1, 2, "LoopyCam Stopped")

	def start(self):
		self.running = True
		self.clock = pygame.time.Clock()

		# Try to do 60 updates per second
		msperframe = 1000 / 60
		while self.running:
			t0 = pygame.time.get_ticks()
			self.running = self.handleEvents()
			self.dlist.paint()
			pygame.display.flip()
			t1 = pygame.time.get_ticks()
			dt = t1 - t0
			# if dt > (2*msperframe):
			#	print "Falling behind! dt=",dt
			pygame.time.wait(msperframe - dt)
		return

	def handleEvents(self):
		for event in pygame.event.get():
			# print "pygame event = ",event
			if event.type == QUIT:
				return False
			elif event.type == KEYDOWN:
				# if event.key == K_ESCAPE:
				# return False
				# print "KEYDOWN scancode=",event.scancode
				if event.scancode in self.scancode2key:
					self.keydown(self.scancode2key[event.scancode])
				else:
					print "UNHANDLED KEYDOWN event=", event
			elif event.type == KEYUP:
				if event.scancode in self.scancode2key:
					self.keyup(self.scancode2key[event.scancode])
				else:
					print "KEYUP event=", event
			elif event.type == MOUSEBUTTONDOWN or event.type == MOUSEBUTTONUP or event.type == MOUSEMOTION:
				self.dlist.mouse_event(event)
		return True

	def file_for(self, n, ptype):
		return os.path.join(self.save_dir, "preset_%s%02d.np" % (ptype, n))

	def save_file(self, fn, apptype):
		print "SAVE_FILE fn=", fn
		# f = open(fn,"wb")
		# f.close()

	def load_file (self, fn, apptype):
		self.message("LOAD_FILE fn=%s" % fn)
		return
		try:
			f = open(fn, "rb")
			for line in f.readlines():
				words = string.split(line)
				if len(words) != 3:
					print "Hey, not 3 words? line=", line
			f.close()
		except:
			self.message("Error in loading: %s" % format_exc())

	def button_delta(self, button):
		if button == 1:
			return - 1
		else:
			return 1

	def send(self, addr, msg=None):
		if self.ffff:
			self.ffff.send(addr, msg)

	def sendlooper(self, addr, msg=None):
		if self.ffff:
			self.ffff.sendlooper(addr, msg)
