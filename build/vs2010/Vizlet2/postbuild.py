import shutil
import os
import sys

t="Debug"
if len(sys.argv) > 1:
	t = sys.argv[1]

d = "../../bin/ffglplugins"
f1 = t + "/Vizlet2.dll"
f2 = d + "/Vizlet2.dll"
print "Copying "+ f1 + " to " + f2
shutil.copyfile(f1,f2)

d = "../../ffglplugins_for_resolume"
f1 = t + "/Vizlet2.dll"
f2 = d + "/Vizlet2.dll"
print "Copying "+ f1 + " to " + f2
shutil.copyfile(f1,f2)
