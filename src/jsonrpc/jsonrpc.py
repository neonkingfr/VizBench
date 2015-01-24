# Utility to send JSON RPC messages

import requests
import json
import sys

verbose = False

def dorpc(port,meth,params):
	url = 'http://127.0.0.1:%d/api' % (port)
	id = '12345'
	data = '{ "jsonrpc": "2.0", "method": "'+meth+'", "params": '+params+', "id":"'+id+'" }\n'
	r = requests.post(url,data)
	print "r.text is ",r.text
	j = json.loads(r.text)
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
