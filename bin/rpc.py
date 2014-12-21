import httplib, urllib
headers = {"Content-type": "application/json",
			"Accept": "text/plain"}
conn = httplib.HTTPConnection("127.0.0.1:4444")
body = '{"foo": "bar"}'
conn.request("POST", "/api", body, headers)
response = conn.getresponse()
print response.status, response.reason
data = response.read()
print "data=",data
conn.close()

