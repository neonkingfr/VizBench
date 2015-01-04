import sys
from jsonrpc import dorpc

def perchan_set(port,chan,name,value):
	dorpc(port,"perchan_set","{\"channel\": \""+str(chan)+"\", \"name\": \""+str(name)+"\", \"value\": \""+str(value)+"\"}")

def linnset(port,setnum):
	for n in range(1,5):
		perchan_set(port,n,"loopnotes",222)

if __name__ == '__main__':
	port = 4449
	setnum = 1
	if len(sys.argv) > 1:
		setnum = int(sys.argv[1])

	print("LinnLoop setting setnum = ",setnum)
	try:
		linnset(port,setnum)
	except:
		print("Unable to linnset!")

