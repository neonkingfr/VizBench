# This renames debug.txt to a timestamped name, and creates a fresh debug.txt

import time
import os
import socket
from subprocess import call

hostname = socket.gethostname()

root = os.getenv("VIZBENCH")
os.chdir(os.path.join(root,"log"))

fname = hostname + "_" + time.strftime("viz_20%y_%m_%d_%H_%M_%S.debug")

try:
	os.rename("viz.debug",fname)
except:
	pass
try:
	f = open("viz.debug","w")
	f.close()
except:
	pass


