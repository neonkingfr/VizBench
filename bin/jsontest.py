# Utility to send JSON RPC messages

import urllib2
import json
import sys
from urllib2 import *
from traceback import format_exc

verbose = False

def jsonrpc(port,meth,params):
	url = 'http://127.0.0.1:%d/dojo.txt' % (port)
	id = '12345'
	data = '{ "jsonrpc": "2.0", "method": "'+meth+'", "params": '+params+', "id":"'+id+'" }\n'
	if verbose:
		print "SENDING: ",data
	req = urllib2.Request(url, data)
	response = urllib2.urlopen(req)
	r = response.read()
	return json.loads(r)

tests = [
	{ "cmd": [ "list", "{\"type\":\"ffx\"}" ], "result": "error" },
	{ "cmd": [ "list", "{\"type\":\"ffgl\"}" ], "result": "list" },
	{ "cmd": [ "list", "{\"type\":\"ff\"}" ], "result": "list" },
	{ "cmd": [ "echo", "{\"value\":\"xyzzy\"}" ], "result": "xyzzy" },
	]

if __name__ == '__main__':

	for t in tests:
		c = t["cmd"]
		expected = t["result"]
		cmd = "%s %s" % (c[0],str(c[1]))
		try:
			j = jsonrpc(4448,c[0],c[1])
		except URLError:
			print "URL Error - is manifold running!?"
			break
		except:
			print "Unexpected exception: %s" % format_exc(2)
			break

		if ("error" in j):
			e = j["error"]["message"]
			if expected == "error":
				print "OK: ",cmd
			else:
				print "ERROR: ",cmd,"  UNEXPECTED ERROR: ",e
			continue
		if not ("result" in j):
			print "ERROR: ",cmd,"  NO RESULT: ",j
			continue
		r = j["result"]
		if expected == "dict" and type(r) == type({}):
			print "OK: ",cmd
			continue
		if expected == "list" and type(r) == type([]):
			print "OK: ",cmd
			continue
		if r == expected:
			print "OK: ",cmd
		else:
			print "ERROR: ",cmd,"  EXPECTED: ",expected,"  RESULT: ",r
