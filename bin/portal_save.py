#
# This is a web server that provides a browser-based interface to 
# Space Manifold
#

import cherrypy
import subprocess
import httplib, urllib
import time
import os.path

from subprocess import call, Popen
from cherrypy import tools
from cherrypy.lib.static import serve_file
from mako.template import Template
from mako.lookup import TemplateLookup
from traceback import format_exc

lookup = TemplateLookup(directories=['../html'])
rootdir = os.path.realpath("..")
print "ROOTDIR=",rootdir

class Mmtt:
	process = None
	exe = "../bin64/mmtt.exe"
	port = 4440
	# config = "pcx_0"
	# config = "kinect"
	config = "kinect2"

class FFFF:
	process = None
	exe = "FFFF.exe"
	port = 4448

class Vsthost:
	process = None
	exe = "vsthost.exe"
	port = 5555

def ExceptionStr():
	e = format_exc()
	print "exception!!!! = ",e
	return "<pre>Exception!  In Root!? "+e+"<pre>"

class Root:

	@cherrypy.expose
	def index(self):
		try:
			tmpl = lookup.get_template("index.html")
			return tmpl.render()
		except:
			return ExceptionStr()

class FFFFPage:

	@cherrypy.expose
	def index(self):
		try:
			tmpl = lookup.get_template("FFFF/index.html")
			return tmpl.render()
		except:
			return ExceptionStr()

	@staticmethod
	def pipelinelist():
		return "DummyPipelineOutput"
		# return jsonrpc(Mmtt.port,passmeth,params)

def jsonIntResult(val,id):
	return { "jsonrpc": "2.0", "result": val, "id": id }

def jsonError(code,message,id):
	return { "jsonrpc": "2.0", "error": {"code": code, "message": message }, "id": id }

def jsonOK(id):
	return jsonIntResult(0,id)

def jsonrpc(port,method,params):
	headers = {"Content-type": "application/json",
				"Accept": "text/plain"}
	serverurl = "127.0.0.1:%d" % port
	conn = httplib.HTTPConnection(serverurl)
	id = "12345"   # XXX - fixed value works, but should really increment

	# XXX - GROSS HACK - change all u'' strings to ''
	# Should probably do a more explicit conversion of params into
	# properly-formatted JSON
	params = str(params).replace("u'","'")
	params = params.replace("'","\"")

	body = '{"jsonrpc": "2.0", "method": "'+method+'", "params": '+str(params)+', "id": "'+id+'" }'
	# print "PORTAL jsonrpc body=",body
	try:
		conn.request("POST", "/api", body, headers)
		response = conn.getresponse()
		# print response.status, response.reason
		data = response.read()
		# print "data=",data
		conn.close()
		return data
	except:
		return jsonError(-32000,"Unable to connect: "+serverurl,id)

class ApiPage:

	def killsilent(self,process):
		# The thing may have been spawned by someone else, or
		# the one we have spawned may have been killed
		# externally, so it doesn't to much good to
		# try to kill it using process.process, we
		# need to look it up and kill it by the exe name.
		FNULL = open(os.devnull, 'w')
		taskname = os.path.basename(process.exe)
		call(["c:/windows/system32/taskkill", "/f","/im",taskname],
				stdout=FNULL, stderr=FNULL)
		FNULL.close()

	def minimize(self,title):
		nircmd = "c:/local/bin/nircmd.exe"
		print "CALLING nircmd title=",title
		call([nircmd, "win","min","stitle",title])

	@cherrypy.expose
	@cherrypy.tools.json_out()
	@cherrypy.tools.json_in()
	def default(self):
		input_json = cherrypy.request.json
		# print "input_json = ",input_json
		meth = input_json["method"]
		params = input_json["params"]
		# print "meth = ",meth
		# print "params = ",params
		id = "123"
		if meth == "mmtt.start":
			self.killsilent(Mmtt)
			Mmtt.process = Popen([Mmtt.exe, Mmtt.config])
			return jsonOK(id)
		elif meth.startswith("mmtt.passthru."):
			passmeth = meth[len("mmtt.passthru."):]
			return jsonrpc(Mmtt.port,passmeth,params)
		elif meth == "FFFF.start":
			self.killsilent(FFFF)
			FFFF.process = Popen([FFFF.exe])
			return jsonOK(id)
		elif meth.startswith("FFFF.passthru."):
			passmeth = meth[len("FFFF.passthru."):]
			return jsonrpc(FFFF.port,passmeth,params)
		elif meth == "vsthost.start":
			self.killsilent(Vsthost)
			Vsthost.process = Popen([Vsthost.exe,"-server"])

			# time.sleep(1.0)  # give it time, so minimize will work
			# self.minimize("vsthost")

			return jsonOK(id)
		elif meth.startswith("vsthost.passthru."):
			passmeth = meth[len("vsthost.passthru."):]
			return jsonrpc(Vsthost.port,passmeth,params)
		else:
			return jsonError(-32000,"Unrecognized method: "+meth,id)

root = Root()
root.api = ApiPage()
root.FFFF = FFFFPage()

conf = {
	'global': {
		'server.socket_host': '0.0.0.0',
		'server.socket_port': 80,
	},
	'/': {
		'tools.staticdir.on': True,
		'tools.staticdir.dir': os.path.join(rootdir,"html")
	},
}

if __name__ == '__main__':
    current_dir = os.path.dirname(os.path.abspath(__file__))
    # CherryPy always starts with app.root when trying to map request URIs
    # to objects, so we need to mount a request handler root. A request
    # to '/' will be mapped to HelloWorld().index().
    cherrypy.quickstart(root, config=conf)

