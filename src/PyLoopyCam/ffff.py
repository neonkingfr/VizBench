from nosuch.oscutil import *

import requests
import json
import os
import sys
import threading

Verbose = False

class FFparam:
	def __init__(self, name):
		self.name = name
		self._current_val = None
		self._default_val = None
		self.paramthread = None
		
	def current_val(self):
		return self._current_val

	def default_val(self):
		return self._default_val

	def set_current(self, v):
		self._current_val = v
		
	def set_default(self, v):
		self._default_val = v
		
class FFplugin:
	def __init__(self, name, i):
		self.name = name
		self.param = {}
		self.plugin_index = i
		self.paramindex = 0
		self.type = ""
		
	def get_param(self, name):
		if not name in self.param:
			self.param[name] = FFparam(name)
			self.param[name].index = self.paramindex
			self.paramindex += 1
		return self.param[name]
	
	def create_params_from_list(self,params):
		for p in params:
			nm = p["name"]
			# print "CREATE PARAM nm=",nm," type=",type(nm)
			param = self.get_param(nm)
			dflt = p["default"]
			param.set_default(dflt)
			param.set_current(dflt)

class FFGLplugin(FFplugin):
	def __init__(self, name, i):
		FFplugin.__init__(self, name, i)
		self.type = "gl"
		
class FF10plugin(FFplugin):
	def __init__(self, name, i):
		FFplugin.__init__(self, name, i)
		self.type = "10"
		
class ParamThread(Thread):
	
	def __init__(self, ffff, plugin, p, val, dt, nsteps):
		Thread.__init__(self)
		self.ffff = ffff
		self.dt = dt
		self.nsteps = nsteps
		self.plugin = plugin
		if plugin == None:
			print "Hey, ParamThread initialized with plugin=None?"
		self.param = p
		self.beginval = float(p.current_val())
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
		plugin = self.plugin
		if plugin == None:
			print "Hey, ParamThread called with self.plugin=None?"
			return

		nsteps = self.nsteps

		while self.stopme == False and nsteps > 0 and self.stepnum <= nsteps:
			val = self.beginval + (self.stepnum * (self.endval - self.beginval)) / nsteps
			if plugin == None:
				print "Hey, ParamThread.run sees plugin=None?  initial plugin=", plugin.name
				break
			self.ffff.set_param_and_send_now(plugin, self.param, "%f" % val)
			self.stepnum += 1
			sleep(self.dt)
		if self.param.paramthread != self:
			print "HEY!  run ended but paramthread != self!?"
		self.param.paramthread = None
		# print "ParamThread done, plugin=",self.plugin.name
		
class Ffff(Thread):

	def __init__(self, host, sendport):

		self.lock = threading.Lock()
		
		self.lastpresetnum = 0
		self.is_registered = False
		self.time_registered = 0

		self.param_dt = 0.1
		self.param_nsteps = 5

		Thread.__init__(self)
		self.host = host
		self.sendport = sendport

		self.toffffosc = OscRecipient(self.host,3333)

		self.paramthreads = {}

		# populate the list of available plugins
		self.ff10_list = {}
		self.ff10_list["None"] = FF10plugin("None", 0)
		pluginlist = self.sendapi("ffff.ff10plugins")
		print "ff10plugins = ",pluginlist
		for nm in pluginlist:
			ff = FF10plugin(nm, len(self.ff10_list))
			params = self.sendapi("ffff.ff10paramlist","{\"plugin\":\""+nm+"\"}")
			ff.create_params_from_list(params)
			self.ff10_list[nm] = ff

		print "POPULATED ff10_list len = ",len(self.ff10_list)

		pluginlist = self.sendapi("ffff.ffglplugins")
		self.ffgl_list = {}
		self.ffgl_list["None"] = FFGLplugin("None", 0)
		for nm in pluginlist:
			ff = FFGLplugin(nm, len(self.ffgl_list))
			params = self.sendapi("ffff.ffglparamlist","{\"plugin\":\""+nm+"\"}")
			ff.create_params_from_list(params)
			self.ffgl_list[nm] = ff
		print "POPULATED ffgl_list len = ",len(self.ffgl_list)

		# self.clearpipeline()

	def sendosc(self,meth,args):
		# print "SENDOSC meth=",meth," args=",args
		self.toffffosc.sendosc("/api",[meth,args])

	# byteify - this is used to convert unicode to str in a json object
	def byteify(self,input):
		if isinstance(input, dict):
			return {self.byteify(key):self.byteify(value) for key,value in input.iteritems()}
		elif isinstance(input, list):
			return [self.byteify(element) for element in input]
		elif isinstance(input, unicode):
			return input.encode('utf-8')
		else:
			return input

	def _dorpc(self,meth,params={}):
		url = 'http://127.0.0.1:%d/api' % (self.sendport)
		id = '12345'
		if Verbose:
			print "DORPC meth=",meth," params=",params
		data = '{ "jsonrpc": "2.0", "method": "'+meth+'", "params": '+str(params)+', "id":"'+id+'" }\n'
		r = requests.post(url,data)
		# print "DORPC meth=",meth," r.text is ",r.text
		# NOTE: conversion from unicode to str here
		j = json.loads(r.text)
		j = self.byteify(j)
		if "error" in j:
			print "ERROR from method=%s err=%s" % (meth,j["error"]["message"])
			return None
		if not "result" in j:
			print "No result from method=%s" % (meth)
			return None
		return j["result"]

	def ff10_randplugin(self):
		# start at 1, since 0 is "None"
		i = random.randint(1, len(self.ff10_list) - 1)
		return self.ffff.find_ff10_byindex(i)

	def ffgl_randplugin(self):
		# start at 1, since 0 is "None"
		i = random.randint(1, len(self.ffgl_list) - 1)
		return self.find_ffgl_byindex(i)

	def send_plugin_param(self, plugin, p):
		api = "ff"+plugin.type+"paramset"
		# print "send_plugin_param "+plugin.name+" param="+p.name+" val="+str(p.current_val())
		jsonargs = "{\"instance\":\""+plugin.name+"\",\"param\":\""+p.name+"\",\"val\":\""+str(p.current_val())+"\"}"
		# print "============= Sending param with OSC api=",api," jsonargs=",jsonargs
		self.sendosc("ffff."+api,jsonargs)
		
	# def set_ui(self, ui):
	# 	self.ui = ui
		
	def find_ff10_byindex(self, i):
		for nm in self.ff10_list:
			ff = self.ff10_list[nm]
			if ff.plugin_index == i:
				return ff
		return None
		
	def find_ffgl_byindex(self, i):
		for nm in self.ffgl_list:
			ff = self.ffgl_list[nm]
			if ff.plugin_index == i:
				return ff
		return None
		
	def get_ff10(self, nm):
		if nm in self.ff10_list:
			ff = self.ff10_list[nm]
		else:
			ff = FF10plugin(nm, len(self.ff10_list))
			if nm == "None":
				params = []
			else:
				params = self.sendapi("ffff.ff10paramlist","{\"plugin\":\""+nm+"\"}")
			ff.create_params_from_list(params)
			self.ff10_list[nm] = ff
		return ff

	def get_ffgl(self, nm):
		if nm in self.ffgl_list:
			ff = self.ffgl_list[nm]
		else:
			ff = FFGLplugin(nm, len(self.ffgl_list))
			if nm == "None":
				params = []
			else:
				params = self.sendapi("ffff.ffglparamlist","{\"plugin\":\""+nm+"\"}")
			ff.create_params_from_list(params)
			self.ffgl_list[nm] = ff;
		return ff

	def ff10_pipeline(self):
		return self.sendapi("ffff.ff10pipeline")

	def ffgl_pipeline(self):
		return self.sendapi("ffff.ffglpipeline")

	def clearpipeline(self):
		print "CLEARPIPELINE paramthreads=",self.paramthreads
		for t in self.paramthreads:
			t.stop()
			t.join()
		self.paramthreads = {}
		self.sendapi("ffff.clearpipeline")

	def sendapi(self, addr, msg=None):
		if msg == None:
			msg = {}
		self.lock.acquire()
		if msg == None:
			r = self._dorpc(addr)
		else:
			r = self._dorpc(addr,msg)
		self.lock.release()
		return r

	def read_preset(self,preset_set,preset_nm):
		fname = self.preset_fullpath(preset_set, preset_nm)
		try:
			f = open(fname, "rb")
			print "FFFF.READ_PRESET fname=",fname
			self.read_preset_file(f)
			f.close()
		except:
			print "Error in loading fname=%s err=%s" % (fname,format_exc())

	def ff10_add_to_pipeline(self,plugin):
		nm = plugin.name
		instancenm = nm
		# print "ff10_add_to_pipeline nm=",nm," instancenm=",instancenm
		self.sendapi("ffff.ff10add", "{\"plugin\":\""+nm+"\",\"instance\":\""+instancenm+"\"}")

	def ffgl_add_to_pipeline(self,plugin):
		nm = plugin.name
		instancenm = nm
		self.sendapi("ffff.ffgladd", "{\"plugin\":\""+nm+"\",\"instance\":\""+instancenm+"\"}")

	def read_preset_file(self,f):
		plugin = None
		self.clearpipeline()
		loopyadded = False
		presetlines = f.readlines()

		# First, remove any plugins from the pipeline that aren't
		# in the new preset
		# XXX - this isn't done

		for line in presetlines:
			# print "line=",line
			# NOTE: conversion from unicode to str here
			line = str(string.replace(line, "\n", ""))
			words = [x.strip() for x in line.split(":")]
			mode = words[0]
			if mode == "pre":
				n = int(words[1])
				name = words[2]
				if name == "None":
					plugin = None
				else:
					plugin = self.get_ff10(name)
					if plugin:
						self.ff10_add_to_pipeline(plugin)
					else:
						print "UNABLE TO find/add ff10plugin - ",name
						return
			elif mode == "post":
				if not loopyadded:
					plugin = self.get_ff10("Viz10LoopyCam")
					if plugin:
						self.ff10_add_to_pipeline(plugin)
					else:
						print "UNABLE TO find/add Viz10LoopyCam plugin?"
						return
					loopyadded = True
				n = int(words[1])
				name = words[2]
				if name == "None":
					plugin = None
				else:
					plugin = self.get_ff10(name)
					if plugin:
						self.ff10_add_to_pipeline(plugin)
					else:
						print "UNABLE TO find/add ffplugin name=",name
						return
			elif mode == "ffgl":
				n = int(words[1])
				name = words[2]
				if name == "None":
					plugin = None
				else:
					plugin = self.get_ffgl(name)
					if plugin:
						self.ffgl_add_to_pipeline(plugin)
					else:
						print "UNABLE TO find/add ffplugin name=",name
						return
			elif mode == "param":
				paramname = words[1]
				paramval = words[2]
				if plugin:
					p = plugin.param[paramname]
					# print "PARAM p=",p," paramval=",paramval
					self.change_plugin_param_val(plugin, p, paramval)

	def write_plugin(self, f, plugin):
		for nm in plugin.param:
			p = plugin.param[nm]
			f.write("param:%s:%s\n" % (nm, p.current_val()))
			
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
			self.write_plugin(f, self.get_ff10(b.text))
		for n in range(NPOST):
			b = self.butt["post"][n]
			f.write("post:%d:%s\n" % (n, b.text))
			self.write_plugin(f, self.get_ff10(b.text))
		for n in range(NFFGL):
			b = self.butt["ffgl"][n]
			f.write("ffgl:%d:%s\n" % (n, b.text))
			self.write_plugin(f, self.get_ffgl10(b.text))
		f.close()


	def set_param_and_send_now(self, plugin, p, val):

		if plugin.name == "Glow":
			if p.name == "Floor":
				ceil = plugin.get_param("Ceiling")
				if float(val) > float(ceil.current_val()):
					# print "Glow.Floor too high!"
					return
			elif p.name == "Ceiling":
				floor = plugin.get_param("Floor")
				if float(val) < float(floor.current_val()):
					# print "Glow.Ceiling too low!"
					return
		if plugin == None:
			print "Hmm, plugin==None in set_param_and_send_now?"
			return
		p.set_current(val)
		self.send_plugin_param(plugin, p)
			
	def change_plugin_param_val(self, plugin, p, val):
		if self.param_dt <= 0.0 or self.param_nsteps <= 1:
			self.set_param_and_send_now(plugin, p, val)
		else:
			print "Spawning ParamThread p=",p.name," dt=",self.param_dt,"  nsteps=",self.param_nsteps
			t = ParamThread(self, plugin, p, val, self.param_dt, self.param_nsteps)
			t.start()
			self.paramthreads[t] = 1

