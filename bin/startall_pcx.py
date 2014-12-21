import os
import shutil
import urllib
import urllib2
import json
from subprocess import call, Popen
from time import sleep
from nosuch.oscutil import *

def killtask(nm):
	call(["c:/windows/system32/taskkill","/f","/im",nm])

def mmtt_action(host,port,meth):
	url = 'http://'+host+':'+port+'/dojo.txt'
	params = '{}'
	id = '12345'
	data = '{ "jsonrpc": "2.0", "method": "'+meth+'", "params": "'+params+'", "id":"'+id+'" }\n'
	req = urllib2.Request(url, data)
	response = urllib2.urlopen(req)
	r = response.read()
	j = json.loads(r)
	if "result" in j:
		return j["result"]
	else:
		print "No result in JSON response!?  r="+r
		return -1

manifold = os.getenv("VIZBENCH")
call(["c:/python27/python.exe",manifold+"/bin/killall.py"])

loopmidi = Popen(["/Program Files (x86)/Tobias Erichsen/loopMIDI/loopMIDI.exe"])
print "loopMIDI has been started."

mmtt_exe = "mmtt_depth.exe"
mmtt_depth0 = Popen([mmtt_exe,"pcx_0.json"])
mmtt_depth1 = Popen([mmtt_exe,"pcx_1.json"])

sleep(2)  # let them get running

nircmd = "c:/local/bin/nircmdc.exe"
call([nircmd,"win","setsize","title","MMTT (pcx_0)","10","10","800","600"])
call([nircmd,"win","setsize","title","MMTT (pcx_1)","850","10","800","600"])

# call([nircmd,"win","settopmost","title","MMTT","1"])
# call([nircmd,"win","max","title","MMTT"])

while True:
	if mmtt_action("127.0.0.1","4440","align_isdone") == 1 and mmtt_action("127.0.0.1","4441","align_isdone") == 1:
		break
	sleep(1)

dobidule = True
bexe = "C:\\Program Files\\Plogue\\Bidule\\PlogueBidule_x64.exe"
bdir = manifold + "\\patches\\bidule\\"
if dobidule:
	b1 = Popen([ bexe, bdir+"Palette_Absynth_final_side1.bidule"])
	b2 = Popen([ bexe, bdir+"Palette_Absynth_final_side2.bidule"])
	sleep(20)

arena = Popen(["C:\\Program Files (x86)\\Resolume Arena 4.1.7\\Arena.exe"])
 
global resolume
resolume = OscRecipient("127.0.0.1",7000)

# Activate the clips in Resolume.
# IMPORTANT!! The last clip activated MUST be layer1, so that the
# Osc enabling/disabling eof FFGL plugins works as intended.

print "Sending OSC to activate Resolume."
resolume.sendosc("/layer2/clip1/connect",[1])
resolume.sendosc("/layer1/clip1/connect",[1])

# Keep sending - Resolume might not be up yet
for i in range(12):
	sleep(2)
	resolume.sendosc("/layer2/clip1/connect",[1])
	resolume.sendosc("/layer1/clip1/connect",[1])


print "DONE!"
