import time
import os
import random
import httplib, urllib
import time
import nosuch.midiutil

from subprocess import call,Popen
from nosuch.oscutil import *
from time import sleep

class VstHost:

	def __init__(self,httpport=5555,oscport=5556):
		self.oschost = "127.0.0.1"
		self.oscport = oscport
		self.httphost = "127.0.0.1"
		self.httpport = httpport
		self.oscr = OscRecipient(self.oschost,self.oscport)
		self.serverurl = "%s:%d" % (self.httphost,self.httpport)

	def osctovst(self,addr,args):
		# print "osctovst, addr=",addr," args=",args
		b = createBinaryMsg(addr,args)
		self.oscr.osc_socket.sendto(b, (self.oschost, self.oscport))

	def noteon(self,channel,pitch,velocity):
		self.osctovst("/midi/noteon",[channel,pitch,velocity])

	def noteoff(self,channel,pitch,velocity):
		self.osctovst("/midi/noteoff",[channel,pitch,velocity])

	def write_midi(self,m):
		(addr,args) = m.to_osc()
		self.osctovst(addr,args)

	def jsonrpc(self,method,params={}):
		headers = {"Content-type": "application/json",
					"Accept": "text/plain"}
		conn = httplib.HTTPConnection(self.serverurl)
		id = "12345"   # XXX - fixed value works, but should really increment

		# XXX - GROSS HACK - change all u'' strings to ''
		# Should probably do a more explicit conversion of params into
		# properly-formatted JSON
		params = str(params).replace("u'","'")
		params = params.replace("'","\"")

		body = '{"jsonrpc": "2.0", "method": "'+method+'", "params": '+str(params)+', "id": "'+id+'" }'
		try:
			conn.request("POST", "/api", body, headers)
			response = conn.getresponse()
			# print response.status, response.reason
			data = response.read()
			# print "data=",data
			conn.close()
			try:
				result = eval(data)
			except:
				return self.jsonError(-32000,"Unable to interpret result data: "+data,id)
			return result
		except:
			return self.jsonError(-32000,"Unable to connect: "+serverurl,id)

	def jsonError(self,code,message,id):
		return { "jsonrpc": "2.0", "error": {"code": code, "message": message }, "id": id }

	def getVstInstance(self,plugindex):
		return VstPlugin(self,plugindex)

	def createVstInstance(self,name):
		if not name.endswith(".dll"):
			name = name + ".dll"
		path = '\\local\\vstplugins\\' + name
		params = {"vst":path}
		r = self.jsonrpc("add",params)
		if "error" in r:
			print "Error adding vst=",name," error=",r["error"]["message"]
			return None
		else:
			return VstPlugin(self,r["result"])


class VstPlugin:

	def __init__(self,vsthost,plugindex):
		self.vsthost = vsthost
		self.plugindex = plugindex

	def jsonrpc(self,method,params={}):
		return self.vsthost.jsonrpc(method,params)

	def setparameter(self,paramindex,value):
		r = self.jsonrpc("pluginparam",{"plugindex":self.plugindex,"paramindex":paramindex,"value":value})
		return self.checkerror(r,"setparameter")

	def setchannel(self,channel):
		r = self.jsonrpc("midichannel",{"plugindex":self.plugindex,"channel":channel})
		return self.checkerror(r,"setchannel")

	def checkerror(self,r,method):
		if "error" in r:
			print "Error in ",method,": ",r["error"]["message"]
			return False
		return True
