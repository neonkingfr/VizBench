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
		# print "SET_CURRENT_PARAM of FF or FFGL plugin=", self.name, "  name=", name, " val=", val

	def set_default_param(self, name, val):
		p = self.get_param(name)
		p.set_default(val)
		

	def create_params_from_list(self,params):
		print "CREATE_PARAMS for plugin=",self.name," params=",params
		for p in params:
			print "p=",p
			nm = p["name"]
			print "param nm=",nm
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

		self.ffplugins = {}
		self.ffglplugins = {}
		self.loops = {}
		self.lock = threading.Lock()

		self.ffplugins["None"] = FFplugin("None", len(self.ffplugins))
		self.ffglplugins["None"] = FFGLplugin("None", len(self.ffglplugins))
		
		self.is_registered = False
		self.time_registered = 0
		self.last_retry = 0.0
		self.retry_limit = 4.0

		Thread.__init__(self)
		self.host = host
		self.sendport = sendport
		self.toffff = None
		# self.toffff = OscRecipient(self.host, self.sendport, proto="tcp")

		

	def sendosc(self,addr):
		print "SENDOSC addr=",addr

	def dorpc(self,meth,params):
		url = 'http://127.0.0.1:%d/api' % (self.sendport)
		id = '12345'
		print "dorpc meth=",meth
		print "dorpc params=",params
		data = '{ "jsonrpc": "2.0", "method": "'+meth+'", "params": '+str(params)+', "id":"'+id+'" }\n'
		r = requests.post(url,data)
		# print "r.text is ",r.text
		j = json.loads(r.text)
		if "error" in j:
			print "ERROR from method=%s err=%s" % (meth,j["error"]["message"])
			return None
		if not "result" in j:
			print "No result from method=%s" % (meth)
			return None
		return j["result"]

	def ff_nplugins(self):
		return len(self.ffplugins)

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
		
	# def get_ff(self,nm):
	# 	if nm in self.ffplugins:
	# 		return self.ffplugins[nm]
	# 	return None
	
	def find_ff_byindex(self, i):
		for nm in self.ffplugins:
			ff = self.ffplugins[nm]
			if ff.plugin_index == i:
				return ff
		return None
		
	def find_ffgl_byindex(self, i):
		for nm in self.ffglplugins:
			ff = self.ffglplugins[nm]
			if ff.plugin_index == i:
				return ff
		return None
		
	def get_ff(self, nm, create=False):
		print "TJT get_ff A"
		if nm in self.ffplugins:
			print "Existing ffplugin - ",nm
			ff = self.ffplugins[nm]
		else:
			if create:
				print "Creating new Ffplugin - ",nm
				ff = FFplugin(nm, len(self.ffplugins))
				print "new ff=",ff
				params = self.dorpc("ffff.ff10paramlist","{\"plugin\":\""+nm+"\"}")
				print "params =",params
				ff.create_params_from_list(params)
				self.ffplugins[nm] = ff
				print "After setting ffplugins"
			else:
				ff = None
		return ff

	def get_ffgl(self, nm, create=False):
		if nm in self.ffglplugins:
			ffgl = self.ffglplugins[nm]
		else:
			if create:
				self.ffglplugins[nm] = FFGLplugin(nm, len(self.ffglplugins))
				ffgl = self.ffglplugins[nm]
			else:
				ffgl = None
		return ffgl

	def ffffcallback(self, ev, d):
		msg = ev.oscmsg
		# print "SELF.FFFF Callback! ev=",ev," d=",d," msg[0]=",msg[0]
		ffprefix = "/ffff/ff/"
		ffglprefix = "/ffff/ffgl/"
		ffdefaultsprefix = "/ffff/ffdefaults/"
		ffgldefaultsprefix = "/ffff/ffgldefaults/"
		preffprefix = "/ffff/preff/"
		postffprefix = "/ffff/postff/"
		postffglprefix = "/ffff/postffgl/"
		replyprefix = "/.reply"
		a = msg[0]
		args = msg[2:]
		if a.startswith(ffprefix):
			nm = a[len(ffprefix):]
			# print "nm=",nm
			ff = self.get_ff(nm, create=True)
			for n in range(len(args)):
				p = args[n]
				i = p.find("=")
				if i >= 0:
					ff.set_current_param(p[0:i], p[i + 1:])
		elif a.startswith(ffglprefix):
			nm = a[len(ffglprefix):]
			# print "nm=",nm
			ffgl = self.get_ffgl(nm, create=True)
			# print "GOT FFGLpregix, nm=", nm
			for n in range(len(args)):
				p = args[n]
				i = p.find("=")
				if i >= 0:
					ffgl.set_current_param(p[0:i], p[i + 1:])
		elif a.startswith(ffdefaultsprefix):
			nm = a[len(ffdefaultsprefix):]
			ff = self.get_ff(nm, create=True)
			for n in range(len(args)):
				p = args[n]
				i = p.find("=")
				if i >= 0:
					ff.set_default_param(p[0:i], p[i + 1:])
		elif a.startswith(ffgldefaultsprefix):
			nm = a[len(ffgldefaultsprefix):]
			ffgl = self.get_ffgl(nm, create=True)
			for n in range(len(args)):
				p = args[n]
				i = p.find("=")
				if i >= 0:
					ffgl.set_default_param(p[0:i], p[i + 1:])
		elif a.startswith(preffprefix):
			num = int(a[len(preffprefix):])
			nm = args[0]
			if not nm in self.ffplugins:
				if nm != "None":
					print "HEY, nm=", nm, " not in ffplugins?  a=", a
			else:
				ff = self.ffplugins[nm]
				self.ui.set_preff(num, nm)
		elif a.startswith(postffprefix):
			num = int(a[len(postffprefix):])
			nm = args[0]
			if not nm in self.ffplugins:
				if nm != "None":
					print "HEY, nm=", nm, " not in ffplugins?  a=", a
			else:
				ff = self.ffplugins[nm]
				self.ui.set_postff(num, nm)
		elif a.startswith(postffglprefix):
			num = int(a[len(postffglprefix):])
			nm = args[0]
			if not nm in self.ffglplugins:
				if nm != "None":
					print "HEY, nm=", nm, " not in ffglplugins?  a=", a
					for name in self.ffglplugins:
						print "  ffglplugin=%s\n" % name
			else:
				ff = self.ffglplugins[nm]
				self.ui.set_postffgl(num, nm)
		elif a.startswith(replyprefix):
			if args[0] == "/.register":
				print "REGISTERED!!"
				self.is_registered = True;
				self.time_registered = time.time()
			if args[0] == "/.list" and args[1] == "/ffff/ff":
				for n in range(2, len(args)):
					nm = args[n]
					# When FFFF tells us about a plugin, we send back
					# a request to list its parameters
					self.sendosc("/ffff/ff/" + nm)
					self.sendosc("/ffff/ffdefaults/" + nm)
			elif args[0] == "/.list" and args[1] == "/ffff/ffgl":
				for n in range(2, len(args)):
					nm = args[n]
					if nm == "Trails" or nm == "Flip":
						print "IGNORING plugin in .list: ", nm
						continue
					# When FFFF tells us about a plugin, we send back
					# a request to list its parameters
					# print "FFGLPLUGIN = ", nm
					self.sendosc("/ffff/ffgl/" + nm)
					self.sendosc("/ffff/ffgldefaults/" + nm)
			elif args[0] == "/.list" and args[1] == "/ffff/preff":
				for n in range(2, len(args)):
					self.sendosc("/ffff/preff/%d" % (n - 2))
			elif args[0] == "/.list" and args[1] == "/ffff/postff":
				for n in range(2, len(args)):
					self.sendosc("/ffff/postff/%d" % (n - 2))
			elif args[0] == "/.list" and args[1] == "/ffff/postffgl":
				for n in range(2, len(args)):
					if (n - 2) == 3:
						print "HEY!!! AA Unexpected 3!"
					self.sendosc("/ffff/postffgl/%d" % (n - 2))
					
	def change_preff(self, index, nm):
		print "CHANGE_PREFF index=",index," nm=",nm
		if self.toffff:
			self.sendosc("/ffff/preff/%d" % (index), [nm])

	def change_postff(self, index, nm):
		print "CHANGE_POSTFF index=",index," nm=",nm
		self.sendosc("/ffff/postff/%d" % (index), [nm])

	def change_ffgl(self, index, nm):
		if index == 3:
			print "HEY!!! BB Unexpected 3!"
		print "CHANGE_FFGL index=",index," nm=",nm
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
		print "FFFF.SEND  addr=", addr, " msg=", msg
		self.lock.release()

	def sendlooper(self, addr, msg=None, retry=True):
		if msg == None:
			msg = {}
		self.lock.acquire()
		addr = "Viz10LoopyCam."+addr
		print "FFFF.SENDLOOPER  addr=", addr
		if msg == None:
			r = self.dorpc(addr)
		else:
			r = self.dorpc(addr,msg)
		self.lock.release()
