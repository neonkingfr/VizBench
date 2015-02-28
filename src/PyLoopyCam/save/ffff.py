from nosuch.oscutil import *

import requests
import json
import sys
import threading

class FFparam:
	def __init__(self, name):
		self.name = name
		self.current_val = None
		self.default_val = None
		self.paramthread = None
		
	def set_current(self, v):
		self.current_val = v
		
	def set_default(self, v):
		self.default_val = v
		
class FFplugin:
	def __init__(self, name, i):
		self.name = name
		self.param = {}
		self.plugin_index = i
		self.paramindex = 0
		self.type = "ff"
		
	def num_params(self):
		return len(self.param)
		
	def get_param(self, name):
		if not name in self.param:
			self.param[name] = FFparam(name)
			self.param[name].index = self.paramindex
			self.paramindex += 1
		return self.param[name]
	
	def get_param_byindex(self,n):
		for name in self.param:
			if self.param[name].index == n:
				return self.param[name]
		return None
			
	def set_current_param(self, name, val):
		p = self.get_param(name)
		p.set_current(val)

	def set_default_param(self, name, val):
		p = self.get_param(name)
		p.set_default(val)

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
		self.type = "ffgl"
		
class FF10plugin(FFplugin):
	def __init__(self, name, i):
		FFplugin.__init__(self, name, i)
		self.type = "ff10"
		
class Ffff(Thread):

	def __init__(self, host, sendport):

		self.lock = threading.Lock()
		
		self.is_registered = False
		self.time_registered = 0
		self.last_retry = 0.0
		self.retry_limit = 4.0

		Thread.__init__(self)
		self.host = host
		self.sendport = sendport
		self.toffff = None
		# self.toffff = OscRecipient(self.host, self.sendport, proto="tcp")

		# populate the list of available plugins
		self.ff10_list = {}
		self.ff10_list["None"] = FF10plugin("None", 0)
		pluginlist = self.dorpc("ffff.ff10plugins")
		for nm in pluginlist:
			ff = FF10plugin(nm, len(self.ff10_list))
			params = self.dorpc("ffff.ff10paramlist","{\"plugin\":\""+nm+"\"}")
			ff.create_params_from_list(params)
			self.ff10_list[nm] = ff

		print "POPULATED ff10_list len = ",len(self.ff10_list)

		pluginlist = self.dorpc("ffff.ffglplugins")
		self.ffgl_list = {}
		self.ffgl_list["None"] = FFGLplugin("None", 0)
		for nm in pluginlist:
			ff = FFGLplugin(nm, len(self.ffgl_list))
			params = self.dorpc("ffff.ffglparamlist","{\"plugin\":\""+nm+"\"}")
			ff.create_params_from_list(params)
			self.ffgl_list[nm] = ff
		print "POPULATED ffgl_list len = ",len(self.ffgl_list)

		self.clearpipeline()


		

	def sendosc(self,addr,args):
		# print "SENDOSC addr=",addr," args=",args
		pass

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

	def dorpc(self,meth,params={}):
		url = 'http://127.0.0.1:%d/api' % (self.sendport)
		id = '12345'
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

	def ff10_nplugins(self):
		return len(self.ff10plugins)

	def ffgl_nplugins(self):
		return len(self.ffglplugins)

	def send_plugin_params(self, plugin):
		# print "Sending plugin prameters! plugin=",plugin.name
		args = []
		for nm in plugin.param:
			p = plugin.param[nm]
			args.append(nm + "=" + p.current_val)
		self.send("/ffff/" + plugin.type + "/" + plugin.name, args)
		
	def send_plugin_param(self, plugin, p):
		# print "Sending plugin prameter! plugin=",plugin.name," param=",p.name
		args = []
		args.append(p.name + "=" + p.current_val)
		self.send("/ffff/" + plugin.type + "/" + plugin.name, args)
		
	def set_ui(self, ui):
		self.ui = ui
		
	def find_ff10_byindex(self, i):
		for nm in self.ff10plugins:
			ff = self.ff10plugins[nm]
			if ff.plugin_index == i:
				return ff
		return None
		
	def find_ffgl_byindex(self, i):
		for nm in self.ffglplugins:
			ff = self.ffglplugins[nm]
			if ff.plugin_index == i:
				return ff
		return None
		
	def get_ff10(self, nm):
		if nm in self.ff10plugins:
			ff = self.ff10plugins[nm]
		else:
			ff = FF10plugin(nm, len(self.ff10plugins))
			if nm == "None":
				params = []
			else:
				params = self.dorpc("ffff.ff10paramlist","{\"plugin\":\""+nm+"\"}")
			ff.create_params_from_list(params)
			self.ff10plugins[nm] = ff
		return ff

	def get_ffgl(self, nm):
		if nm in self.ffglplugins:
			ffgl = self.ffglplugins[nm]
		else:
			ffgl = FFGLplugin(nm, len(self.ffglplugins))
			if nm == "None":
				params = []
			else:
				params = self.dorpc("ffff.ffglparamlist","{\"plugin\":\""+nm+"\"}")
			ffgl.create_params_from_list(params)
			self.ffglplugins[nm] = ffgl;
			print "get_ffgl nffglplugins = ",self.ffgl_nplugins()
		return ffgl

	def clearpipeline(self):
		print "CLEARPIPELINE called!"
		self.ff10_pipeline = []
		self.ffgl_pipeline = []
		self.sendffff("clearpipeline")

	def change_preff(self, index, nm):
		# print "CHANGE_PREFF index=",index," nm=",nm
		if self.toffff:
			self.sendosc("/ffff/preff/%d" % (index), [nm])

	def change_postff(self, index, nm):
		# print "CHANGE_POSTFF index=",index," nm=",nm
		self.sendosc("/ffff/postff/%d" % (index), [nm])

	def change_ffgl(self, index, nm):
		if index == 3:
			print "HEY!!! BB Unexpected 3!"
		# print "CHANGE_FFGL index=",index," nm=",nm
		self.sendosc("/ffff/postffgl/%d" % (index), [nm])

	def run(self):
		# someday this may be needed
		while True:
			self.send("/.ping", [])
			time.sleep(2.0)
		return

	def refresh(self, retry=True):
		print "FFFF refresh!  NEEDS WORK"
		try:
			self.ui.set_status("")
			self.ui.lcd_refresh()
			
		except:
			print "Error in FFFF refresh: %s" % format_exc()

	def record(self, onoff):
		self.sendvalue("/looper/record", onoff)

	def recordoverlay(self, onoff):
		self.sendvalue("/looper/recordoverlay", onoff)

	def restart(self, loopnum):
		self.sendvalue("/looper/restart", loopnum)

	def restartrandom(self, loopnum):
		self.sendvalue("/looper/restartrandom", loopnum)

	def restartsave(self, loopnum):
		self.sendvalue("/looper/restartsave", loopnum)

	def restartrestore(self, loopnum):
		self.sendvalue("/looper/restartrestore", loopnum)

	def freeze(self, loopnum):
		self.sendvalue("/looper/freeze", loopnum)

	def unfreeze(self, loopnum):
		self.sendvalue("/looper/unfreeze", loopnum)

	def sendvalue(self, path, value=None):
		if value != None:
			self.send(path, [value])
		else:
			self.send(path, [])

	def send(self, addr, msg=None, retry=True):
		if msg == None:
			msg = []
		self.lock.acquire()
		# print "FFFF.SEND  addr=", addr, " msg=", msg
		self.lock.release()

	def sendlooper(self, addr, msg=None, retry=True):
		self.sendapi("Viz10LoopyCam."+addr, msg, retry)

	def sendffff(self, addr, msg=None, retry=True):
		self.sendapi("ffff."+addr, msg, retry)

	def sendapi(self, addr, msg=None, retry=True):
		if msg == None:
			msg = {}
		self.lock.acquire()
		print "FFFF.SENDAPI  addr=", addr
		if msg == None:
			r = self.dorpc(addr)
		else:
			r = self.dorpc(addr,msg)
		self.lock.release()

	def read_preset(self,fname,panel):
		f = open(fname, "rb")
		plugin = None
		print "FFFF.READ_PRESET fname=",fname
		self.clearpipeline()
		for line in f.readlines():
			print "line=",line
			# NOTE: conversion from unicode to str here
			line = str(string.replace(line, "\n", ""))
			words = [x.strip() for x in line.split(":")]
			mode = words[0]
			if mode == "pre":
				n = int(words[1])
				name = words[2]
				if name != "None":
					plugin = self.get_ff10(name,create=True)
				else:
					plugin = None
				panel.butt["pre"][n].set_text(name)
				self.change_preff(n, name)
			elif mode == "post":
				n = int(words[1])
				name = words[2]
				if name != "None":
					plugin = self.get_ff10(name,create=True)
				else:
					plugin = None
				panel.butt["post"][n].set_text(name)
				self.change_postff(n, name)
			elif mode == "ffgl":
				n = int(words[1])
				name = words[2]
				if name != "None":
					plugin = self.get_ffgl(name,create=True)
				else:
					plugin = None
				panel.butt["ffgl"][n].set_text(name)
				self.change_ffgl(n, name)
			elif mode == "param":
				paramname = words[1]
				paramval = words[2]
				p = plugin.param[paramname]
				print "PARAM p=",p," paramval=",paramval
				panel.set_param_text_and_send(plugin, p, paramval)
		f.close()

