import sys
import json
import re

if len(sys.argv) < 2:
	print "usage: %s {prefix}"
	sys.exit(1)

prefix = sys.argv[1]
fname = prefix + ".plt"

f = open(fname,"r")
lines = f.readlines()
f.close()

s = ""
for ln in lines:
	s += ln
jorig = json.loads(s)
if jorig == None:
	print "Unable to interpret "+fname+" as JSON!?"
	sys.exit(1)

print "Converting "+fname
newj = {}
for nm in jorig["overrideparams"]:
	if jorig["overrideflags"][nm] == "true":
		newj[nm] = jorig["overrideparams"][nm]

def reformat(j):
	for nm in j:
		s = j[nm]
		if re.match("[0-9]+\.[0-9]+",s):
			j[nm] = float(s)
		elif re.match("[0-9]+",s):
			j[nm] = int(s)
		elif s == "false" or s == "off":
			j[nm] = False
		elif s == "true" or s == "on":
			j[nm] = True

def writeitout(fname,j):
	f = open(fname,"w")
	reformat(j)
	f.write(json.dumps(j, sort_keys=True, indent=4, separators=(',', ': ')))
	f.close()

writeitout(prefix+"_o.json",newj)

for n in range(1,5):
	rp = jorig["regions"][n]["regionspecificparams"]
	writeitout("%s_%d.json" % (prefix,n), rp)
