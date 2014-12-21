# Utility to send JSON RPC messages

import urllib2
import json
import sys

verbose = False

def dorpc(port,meth,params):
	url = 'http://127.0.0.1:%d/api' % (port)
	id = '12345'
	data = '{ "jsonrpc": "2.0", "method": "'+meth+'", "params": '+params+', "id":"'+id+'" }\n'
	if verbose:
		print "SENDING: ",data
	req = urllib2.Request(url, data, {'Content-Type': 'application/json'} )
	response = urllib2.urlopen(req)
	r = response.read()
	j = json.loads(r)
	if "error" in j:
		print "ERROR: "+str(j["error"]["message"])
	elif "result" in j:
		print "RESULT: "+str(j["result"])
	else:
		print "No error or result in JSON response!?  r="+r

if __name__ == '__main__':

	if len(sys.argv) < 2:
		print "Usage: jsonrpc {port} {meth} [ {params} ]"
	else:
		port = int(sys.argv[1])
		meth = sys.argv[2]
		if len(sys.argv) < 4:
			params = '{}'
		else:
			params = sys.argv[3]
		dorpc(port,meth,params)
