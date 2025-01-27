# This script generates the source code and Visual Studio project
# for a new Vizlet plugin.  The VizletTemplate directory contains the 
# basis for it.  If the -v argument is 10, then Vizlet10Template is used

import sys
import os
import re
import hashlib
import argparse

def generate(vizletname,vizletid,pathin,pathout):

	fin = open(pathin,"r")
	fout = open(pathout,"w")

	print "Generating "+pathout

	lines = fin.readlines()

	for ln in lines:
		ln = ln.replace(VizletTemplate,vizletname)
		ln = ln.replace("VZID",vizletid)
		fout.write(ln)

	fout.close()
	fin.close()

if __name__ != "__main__":
	print "This code needs to be invoked as a main program."
	sys.exit(1)

parser = argparse.ArgumentParser("Generate a new FFGL plugin project")
parser.add_argument("name", help="FFGL plugin name")
parser.add_argument("-i", "--id", help="4-character FFGL plugin ID")
parser.add_argument("-f", "--force",
		help="Force overwriting of an existing project",
		action="store_true")
parser.add_argument("-b", "--builddir", help="Visual Studio build directory")
parser.add_argument("-v", "--viztype", help="Either 10 or GL")
parser.add_argument("-x", "--vizbenchx",
		help="Put the result in VizBenchX",
		action="store_true")

args = parser.parse_args()

nm = args.name
id = args.id
force = args.force
builddir = args.builddir
viztype = args.viztype
vizbenchx = args.vizbenchx

if not viztype:
	viztype = "GL"

VizletTemplate = "Vizlet" + viztype + "Template"

if not builddir:
	builddir = "build/vs2010_or_vs2013"

if not nm[0].isupper():
	print("The plugin name needs to start with an uppercase letter!")
	sys.exit(1)

vizbench = os.getenv("VIZBENCH")
if not vizbench:
	print("VIZBENCH environment variable isn't set!")
	sys.exit(1)

os.chdir(vizbench)

projdir = os.path.join(vizbench,builddir,VizletTemplate)
srcdir = os.path.join(vizbench,"src","plugins",VizletTemplate)
blddir = os.path.join(vizbench,builddir)

# Make sure various directories exist

if not os.path.isdir(blddir):
	print "Error: "+blddir+" directory doesn't exist!"
	sys.exit(1)

if not os.path.isdir(projdir):
	print "Error: "+projdir+" doesn't exist!"
	sys.exit(1)

if not os.path.isdir(srcdir):
	print "Error: "+srcdir+" doesn't exist!"
	sys.exit(1)

if not os.path.isdir("src"):
	print "Error: src directory doesn't exist!"
	sys.exit(1)

# Make sure we're in the correct directory (i.e. has blddir and src subdirs)

if args.vizbenchx:
	toprojdir = os.path.join("../VizBenchX",builddir,nm)
	tosrcdir = os.path.join("../VizBenchX/src/plugins",nm)
else:
	toprojdir = os.path.join(blddir,nm)
	tosrcdir = os.path.join("src/plugins",nm)

if force == False and (os.path.exists(toprojdir) or os.path.exists(tosrcdir)):
	print "That vizlet already exists (i.e. either "+ \
			toprojdir+" or "+tosrcdir+" already exists)"
	sys.exit(1)

if not id:
	id = "V%03d" % (int(hashlib.sha1(nm).hexdigest(), 16) % (10**3))
	print "Generated 4-character FFGL plugin ID is '"+id+"'"
else:
	id = id

if len(id) != 4:
	print("The plugin id (%s) needs to be exactly 4 characters long!"%id)
	sys.exit(1)

for dir in [toprojdir, tosrcdir]:
	if not os.path.exists(dir):
		os.mkdir(dir)
		if not os.path.isdir(dir):
			print "Unable to make directory: ",dir
			sys.exit(1)

print "========== Generating FFGL plugin:%s id:%s" % (nm,id)

generate(nm, id, projdir+"/FFPlugins.def", toprojdir+"/FFPlugins.def")
generate(nm, id, projdir+"/"+VizletTemplate+".vcxproj", toprojdir+"/"+nm+".vcxproj")
generate(nm, id, projdir+"/"+VizletTemplate+".vcxproj.filters", toprojdir+"/"+nm+".vcxproj.filters")

generate(nm, id, srcdir+"/"+VizletTemplate+".cpp", tosrcdir+"/"+nm+".cpp")
generate(nm, id, srcdir+"/"+VizletTemplate+".h", tosrcdir+"/"+nm+".h")

sys.exit(0)
