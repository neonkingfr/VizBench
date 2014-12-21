# Utility to test/configure manifold

import urllib2
import json
import sys
from urllib2 import *
from traceback import format_exc
from time import sleep

verbose = False

def manifold(meth,params):
	port = 4448
	url = 'http://127.0.0.1:%d/dojo.txt' % (port)
	id = '12345'
	data = '{ "jsonrpc": "2.0", "method": "'+meth+'", "params": '+params+', "id":"'+id+'" }\n'
	if verbose:
		print "SENDING: ",data
	req = urllib2.Request(url, data)
	try:
		response = urllib2.urlopen(req)
	except:
		print "JSON CONNECTION ERROR: ",format_exc(2)
		sys.exit(1)
	r = response.read()
	try:
		j = json.loads(r)
	except:
		print "JSON RESULT PARSE ERROR: ",r
		sys.exit(1)
	if "error" in j:
		print "ERROR: ",j["error"]["message"]
		sys.exit(1)
	if not "result" in j:
		print "ERROR: no result in rpc call - ",meth,params," j=",j
		sys.exit(1)
	return j["result"]

def listem():
	ff = manifold("fflist", "{\"type\":\"ff\"}")
	for f in ff:
		print "ff=",f
		params = manifold("ffparamlist", "{\"plugin\":\""+f+"\"}")
		for p in params:
			print "   param=",p["name"]," type=",p["type"]

	ffgl = manifold("ffgllist", "{\"type\":\"ffgl\"}")
	for f in ffgl:
		print "ffgl=",f
		params = manifold("ffglparamlist", "{\"plugin\":\""+f+"\"}")
		for p in params:
			# print "   param=",p["name"]," type=",p["type"]
			print "   param=",p

def addinstance(plugin,instance=None,autoenable=1):
	if instance == None:
		instance = plugin
	return manifold("add", "{\"plugin\":\""+plugin+"\", \"instance\":\""+instance+"\", \"autoenable\":"+str(autoenable)+" }")

def deleteinstance(instance):
	return manifold("delete", "{\"instance\":\""+instance+"\"}")

def enableinstance(instance):
	return manifold("enable", "{\"instance\":\""+instance+"\"}")

def disableinstance(instance):
	return manifold("disable", "{\"instance\":\""+instance+"\"}")

def setparam(instance,param,val):
	return manifold("set", "{\"instance\":\""+instance+"\", \"param\":\""+param+"\", \"val\":"+str(val)+"}")

def getparam(instance,param):
	return manifold("get", "{\"instance\":\""+instance+"\", \"param\":\""+param+"\"}")

def ffglparamlist(plugin):
	return manifold("ffglparamlist", "{\"plugin\":\""+plugin+"\"}")

def pipelinelist():
	return manifold("pipelinelist", "{}")

def defaultparams(plugin,inst):
	params = ffglparamlist(plugin)
	# print "defaultparams for plugin=",plugin," inst=",inst
	for p in params:
		if p["type"] == "standard":
			setparam(inst,p["name"],p["default"])
			# print "Set inst=",inst," param=",p["name"]," def=",p["default"]

def randparams(plugin,inst):
	params = ffglparamlist(plugin)
	print "randparams for plugin=",plugin," inst=",inst
	for p in params:
		# print "p=",p
		if p["type"] == "standard":
			# print "p=",p["name"]
			v = random.random()
			# print "   Setting %s to %f" % (p["name"],v)
			setparam(inst,p["name"],v)

def doit():
	list = [
#		"EmptyA",
#		"Pyffle_one",
#		"EmptyB",
# 		"Twisted",
  		"Wave Warp",
#  		"Kaleidoscope",
  		"Blur",
  		"Goo",
  		"Fragment",
  		"Posterize",
  		"Displace",
  		"Iterate",
  		"Edge Detection",
#  		"Bendoscope",
#  		"Mirror",
#  		"Trails",
		]

	# deleteinstance("Pyffle_one1")
	# addinstance("Pyffle_one","Pyffle_one1")
	# enableinstance("Pyffle_one1")

	# sys.exit(0)

	for i in list:
		deleteinstance(i+str(1))

	# the camera image comes in flipped - this correct it
	if True:
		print "SHould be adding Flip!"
		deleteinstance("flip1")
		addinstance("Flip","flip1")
		setparam("flip1","Horizontal",1.0)
		setparam("flip1","Vertical",1.0)

	deleteinstance("vizlet1a")
	addinstance("Vizlet1","vizlet1a")

	# deleteinstance("palette1")
	# addinstance("Palette","palette1")

	# deleteinstance("emptya1")
	# addinstance("EmptyA","emptya1")

	for i in list:
		addinstance(i,i+str(1))

	# for i in list:
	# 	enableinstance(i+str(1))

	print "pipeline=",pipelinelist()

	# sleep(2.0)

	# for i in list:
	# 	randparams(i,i+str(1))

	# print "Sleeping after rand"
	# sleep(2.0)

	# for i in list:
	# 	defaultparams(i,i+str(1))

	# print "Sleeping after defaults"
	# sleep(2.0)

	# for i in list:
	# 	disableinstance(i+str(1))

	while True:
		for i in list:
			if random.randint(0,1) == 1:
				enableinstance(i+str(1))
				randparams(i,i+str(1))
		sleep(0.1)
		for i in list:
			disableinstance(i+str(1))


	print "pipeline=",pipelinelist()

	# addinstance("Mirror","mirror1")

	# setparam("mirror1","X",0.39)

	# addinstance("Mirror","mirror2")

	# for n in range(20):
	# 	f = n / 100.0
	# 	setparam("mirror1","X",f)

	# deleteinstance("edge1")
	# addinstance("Edge Detection","edge1")

	# deleteinstance("post1")
	# addinstance("Posterize","post1")

	# setparam("flip1","Horizontal",0.0)
	# setparam("flip1","Vertical",0.0)

	# params = ffglparamlist("Posterize")
	# for p in params:
	# 	print "p=",p["name"]

	# print "Horiz A = ",getparam("flip1","Horizontal")
	# print "Vert B = ",getparam("flip1","Vertical")

	# setparam("flip1","Horizontal",1.0)
	# setparam("flip1","Vertical",1.0)

	# addinstance("Bendoscope",0)
	# setparam("Bendoscope","Divisions",0.0)
	# r = getparam("Bendoscope","Divisions")

	# listem()

	# print "CHANGING Divisions to 1"
	# setparam("Bendoscope","Divisions",1.0)
	# listem()

def printparams(plugin):
	params = ffglparamlist(plugin)
	for p in params:
		print p

if __name__ == '__main__':

	if len(sys.argv) == 1:
		doit()
		sys.exit(0)

	cmd = sys.argv[1]
	if cmd == "pipeline":
		list = pipelinelist()
		for p in list:
			print p
	elif cmd == "plugin":
		plugin = sys.argv[2]
		print "PARAMETERS OF ",plugin
		printparams(plugin)
	elif cmd == "test":
		setparam("flip1","Horizontal",1.0)
		setparam("flip1","Vertical",1.0)

