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

def mmtt_action(meth):
	url = 'http://127.0.0.1:4444/dojo.txt'
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

mmtt_exe = "mmtt_depth.exe"
mmtt_exe = "mmtt_kinetic.exe"
mmtt_exe = "mmtt_pcx.exe"

mmtt_depth = Popen([mmtt_exe])
sleep(2)  # let it get running
while True:
	if mmtt_action("align_isdone") == 1:
		break
	sleep(1)

print "MMTT has finished aligning."

loopmidi = Popen(["/Program Files (x86)/Tobias Erichsen/loopMIDI/loopMIDI.exe"])

print "loopMIDI has been started."
sleep(1)

bidule = Popen([
 	"C:\\Program Files\\Plogue\\Bidule\\PlogueBidule_x64.exe",
 	manifold+"\\patches\\bidule\\Palette_Alchemy_Burn.bidule"])
 
sleep(25)
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

# call(["c:/local/bin/nircmd.exe","win","settopmost","title","MMTT","1"])
# call(["c:/local/bin/nircmd.exe","win","max","title","MMTT"])
call(["c:/local/bin/nircmd.exe","win","setsize","title","MMTT","900","100","1000","750"])

# Keep sending - Resolume might not be up yet
for i in range(12):
	sleep(2)
	resolume.sendosc("/layer2/clip1/connect",[1])
	resolume.sendosc("/layer1/clip1/connect",[1])


print "DONE!"
