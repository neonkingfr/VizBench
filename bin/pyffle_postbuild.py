import shutil
import os
d = os.getenv("VIZBENCH")+"/ffglplugins"
print "postbuild.py: looking for Pyffle plugins to update in "+d
os.chdir(d)
default = "Pyffle_Default.dll"
if os.access("Pyffle_Default_debug.dll",os.R_OK):
	default = "Pyffle_Default_debug.dll"

print "Default Pyffle dll = "+default

for fname in os.listdir("."):
	if fname[0:14] == "Pyffle_Default":
		continue
	if fname[0:7] == "Pyffle_" and fname[-4:] == ".dll":
		name = fname[7:-4]
		print "Copying "+default+" to "+fname
		shutil.copyfile(default,fname);
