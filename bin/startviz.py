import os
import shutil
import urllib
import urllib2
import json
from subprocess import call, Popen
from time import sleep
from nosuch.oscutil import *

UseLoopMIDI = False

root = os.getenv("VIZBENCH")
print "root = ",root

def killtask(nm):
	call(["c:/windows/system32/taskkill","/f","/im",nm])

call(["c:/python27/python.exe",os.path.join(root,"bin","killall.py")])
call(["c:/python27/python.exe",os.path.join(root,"bin","debugcycle.py")])

if UseLoopMIDI:
	loopmidi = Popen(["/Program Files (x86)/Tobias Erichsen/loopMIDI/loopMIDI.exe"])
	print "loopMIDI has been started."
 	bidulepatch = "patches\\bidule\\viz_loopMIDI.bidule"
else:
	print "NOTE: loopMIDI has NOT been started, and LoopBe is used."
 	bidulepatch = "patches\\bidule\\viz.bidule"

bidulepatchpath = os.path.join(root,bidulepatch)

sleep(1)  # ???

bidule = Popen([
 	"C:\\Program Files\\Plogue\\Bidule\\PlogueBidule_x64.exe",
 	bidulepatchpath])
 
# Wait for Bidule to load
sleep(10)

### patches="c:\\local\\manifold\\bin\\config\\palette"
### shutil.copy(patches+"\\default_burn.mnf",patches+"\\default.mnf")

# XXX - there's a hardcoded path in tofile
fromfile=os.path.join(root,"patches\\resolume\\resolume_config.xml")
tmpfile=os.path.join(root,"patches\\resolume\\tmp_resolume_config.xml")
# replace %VIZBENCH% with the root
fin = open(fromfile)
ftmp = open(tmpfile,"w")
lines = fin.readlines()
for line in lines:
	line = line.replace("%VIZBENCH%",root)
	ftmp.write(line)
fin.close()
ftmp.close()

tofile="c:\\users\\tjt\\documents\\resolume avenue 4\\preferences\\config.xml"
shutil.copy(tmpfile,tofile)

arena = Popen(["C:\\Program Files (x86)\\Resolume Avenue 4.1.11\\Avenue.exe"])
 
## cd \local\python\nosuch_oscutil

global resolume
resolume = OscRecipient("127.0.0.1",7000)

# Activate the clips in Resolume.
# IMPORTANT!! The last clip activated MUST be layer1, so that the
# Osc enabling/disabling eof FFGL plugins works as intended.

sleep(12)

## print "Sending OSC to activate Resolume."
# resolume.sendosc("/layer2/clip1/connect",[1])
# resolume.sendosc("/layer1/clip1/connect",[1])

# Keep sending - Resolume might not be up yet
for i in range(5):
	sleep(2)
	resolume.sendosc("/layer2/clip1/connect",[1])
	resolume.sendosc("/layer1/clip1/connect",[1])

# call(["c:/local/bin/nircmd.exe","win","min","stitle","Plogue"])


print "DONE!"
