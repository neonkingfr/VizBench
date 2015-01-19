# This script reads *VizParams.list files that define Vizlet parameters
# and generates .h files for them, making runtime access to them much faster.
# This allows new parameters to be added just by editing one file.

import sys
import os
import re

types={"bool":"BOOL","int":"INT","double":"DBL","string":"STR"}
realtypes={"bool":"bool","int":"int","double":"double","string":"std::string"}
paramtypes={"bool":"BoolParam","int":"IntParam","double":"DoubleParam","string":"StringParam"}

def readparams(listfile):
	try:
		f = open(listfile)
	except:
		print(sys.stderr,"Unable to open "+listfile)
		sys.exit(1)


	lines = f.readlines()

	params = []
	for ln in lines:
		if len(ln) > 0 and ln[0] == '#':
			continue
		vals = ln.split(None,5)
		(name,typ,mn,mx,default,comment) = vals
		params.append(
			{ "name": name, "type": typ, "min": mn, "max": mx, "default": default, "comment": comment }
			)

	params = sorted(params, key=lambda dct: dct['name'])
	return params

def writeln(line):
	sys.stdout.write(line+"\n")

def write(line):
	sys.stdout.write(line)

def genparamcpp(paramclass):
	writeln("#include \"VizParams.h\"")
	writeln("#include \""+paramclass+".h\"")
	writeln("char* "+paramclass+"Names[] = { "+paramclass+"Names_INIT };")

def genparamheader(params,classname):

	uptype = classname.upper()
	tab = "\t"
	tab2 = "\t\t"
	tab3 = "\t\t\t"

	writeln("#ifndef _"+uptype+"_H")
	writeln("#define _"+uptype+"_H")

	writeln("#include \"VizParams.h\"")
	writeln("#include \"NosuchJSON.h\"")

	writeln("")
	### Generate a declaration for the array of parameter names.
	### The actual storage for it needs to be declared in a non-header file.
	writeln("extern char* "+paramnames+"[];")
	writeln("")

	### Generate a macro which is all the parameter names, used to initialize that array
	writeln("#define "+paramnames+"_INIT \\")
	for p in params:
		name = p["name"]
		writeln(tab+"\"%s\",\\"%(name))
	writeln(tab+"NULL")
	writeln("")

	### Start the class
	writeln("class "+classname+" : public VizParams {")
	writeln("public:")

	### Generate the class constructor
	writeln(tab+classname+"(bool defaults) {")
	writeln(tab2+"if ( defaults ) {")
	writeln(tab3+"loadDefaults();")
	writeln(tab2+"} else {")
	writeln(tab3+"// Otherwise, all of the parameters are unset")
	writeln(tab2+"}")
	writeln(tab+"}")

	writeln(tab+"std::string JsonListOfValues() { return _JsonListOfValues("+paramnames+"); }");
	writeln(tab+"std::string JsonListOfParams() { return _JsonListOfParams("+paramnames+"); }");

	### Generate the method that loads JSON
	writeln(tab+"void loadJson(cJSON* json) {")
	writeln(tab2+"cJSON* j;")
	for p in params:
		name = p["name"]
		typ = p["type"]
		writeln(tab2+"j = cJSON_GetObjectItem(json,\""+name+"\");")
		if typ == "double":
			writeln(tab2+"if (j) { "+name+".set(j->valuedouble); }")
		elif typ == "int":
			writeln(tab2+"if (j) { "+name+".set(j->valueint); }")
		elif typ == "bool":
			writeln(tab2+"if (j) { "+name+".set(j->valueint!=0); }")
		elif typ == "string":
			writeln(tab2+"if (j) { "+name+".set(j->valuestring); }")
	writeln(tab+"}")

	### Generate the method that loads default values
	writeln(tab+"void loadDefaults() {")
	for p in params:
		name = p["name"]
		typ = p["type"]
		defaultvalue = p["default"]
		writeln(tab2+name+".set("+defaultvalue+");")
	writeln(tab+"}")

	### Generate the method that applies one params to another
	writeln(tab+"void applyVizParamsFrom("+classname+"* p) {")
	writeln(tab2+"if ( ! p ) { return; }");
	for p in params:
		name = p["name"]
		typ = p["type"]
		writeln(tab2+"if ( p->"+name+".isset() ) { this->"+name+".set(p->"+name+".get()); }");
	writeln(tab+"}")

	### Generate the Set method
	writeln(tab+"void Set(std::string nm, std::string val) {")
	writeln(tab2+"bool stringval = false;")
	estr = ""
	for p in params:
		name = p["name"]
		writeln(tab2+estr+"if ( nm == \""+name+"\" ) {")
		typ = p["type"]
		if typ == "double":
			writeln(tab3+name+".set(string2double(val));")
		elif typ == "int":
			writeln(tab3+name+".set(string2int(val));")
		elif typ == "bool":
			writeln(tab3+name+".set(string2bool(val));")
		elif typ == "string":
			writeln(tab3+name+".set(val);")
			writeln(tab3+"stringval = true;")
		estr = "} else "
	writeln(tab2+"}")
	writeln("")
	writeln(tab2+"if ( ! stringval ) {")
	writeln(tab3+"Increment(nm,0.0); // abide by limits, using code in Increment")
	writeln(tab2+"}")
	writeln(tab+"}")

	### Generate the Increment method
	writeln(tab+"void Increment(std::string nm, double amount) {")
	estr = ""
	for p in params:
		name = p["name"]
		typ = p["type"]
		mn = p["min"]
		mx = p["max"]
		writeln(tab2+estr+"if ( nm == \""+name+"\" ) {")
		if typ == "double":
			writeln(tab3+name+".set(adjust("+name+".get(),amount,"+mn+","+mx+"));")
		elif typ == "int":
			writeln(tab3+name+".set(adjust("+name+".get(),amount,"+mn+","+mx+"));")
		elif typ == "bool":
			writeln(tab3+name+".set(adjust("+name+".get(),amount));")
		elif typ == "string":
			vals = p["min"]
			if vals == "*":
				writeln(tab3+"// '*' means the value can be anything");
			else:
				writeln(tab3+name+".set(adjust("+name+".get(),amount,VizParams::"+vals+"));")
		estr = "} else "
	writeln(tab2+"}")
	writeln("")
	writeln(tab+"}")

	### Generate the DefaultValue method
	writeln(tab+"std::string DefaultValue(std::string nm) {")
	estr = ""
	for p in params:
		name = p["name"]
		typ = p["type"]
		default = p["default"]
		if default[0] != "\"":
			default = "\"" + default + "\""
		writeln(tab2+estr+"if ( nm == \""+name+"\" ) { return "+default+"; }");
	writeln(tab2+"return \"\";");
	writeln(tab+"}")

	### Generate the MinValue method
	writeln(tab+"std::string MinValue(std::string nm) {")
	estr = ""
	for p in params:
		name = p["name"]
		typ = p["type"]
		mn = p["min"]
		write(tab2+estr+"if ( nm == \""+name+"\" ) { ")
		if typ == "double":
			write("return \""+mn+"\";");
		elif typ == "int":
			write("return \""+mn+"\";");
		elif typ == "bool":
			write("return \"false\";");
		elif typ == "string":
			write("return \""+mn+"\";");
		writeln(" }");
	writeln(tab2+"return \"\";");
	writeln(tab+"}")

	### Generate the MaxValue method
	writeln(tab+"std::string MaxValue(std::string nm) {")
	estr = ""
	for p in params:
		name = p["name"]
		typ = p["type"]
		mx = p["max"]
		write(tab2+estr+"if ( nm == \""+name+"\" ) { ")
		if typ == "double":
			write("return \""+mx+"\";");
		elif typ == "int":
			write("return \""+mx+"\";");
		elif typ == "bool":
			write("return \"true\";");
		elif typ == "string":
			write("return \""+mx+"\";");
		writeln(" }");
	writeln(tab2+"return \"\";");
	writeln(tab+"}")

	### Generate the Toggle method
	writeln(tab+"void Toggle(std::string nm) {")
	writeln(tab2+"bool stringval = false;")
	estr = ""
	for p in params:
		name = p["name"]
		typ = p["type"]
		if typ == "bool":
			writeln(tab2+estr+"if ( nm == \""+name+"\" ) {")
			writeln(tab3+name+".set( ! "+name+".get());")
			writeln(tab2+"}")
			estr = "else "
	writeln(tab+"}")

	### Generate the Get method
	writeln(tab+"std::string GetAsString(std::string nm) {")
	estr = ""
	for p in params:
		name = p["name"]
		typ = p["type"]
		writeln(tab2+estr+"if ( nm == \""+name+"\" ) {")
		if typ == "double":
			writeln(tab3+"return DoubleString("+name+".get());")
		elif typ == "int":
			writeln(tab3+"return IntString("+name+".get());")
		elif typ == "bool":
			writeln(tab3+"return BoolString("+name+".get());")
		elif typ == "string":
			writeln(tab3+"return "+name+".get();")
		estr = "} else "
	writeln(tab2+"}")
	writeln(tab2+"return \"\";")
	writeln(tab+"}")

	### Generate the GetType method
	writeln(tab+"std::string GetType(std::string nm) {")
	for p in params:
		name = p["name"]
		typ = p["type"]
		writeln(tab2+"if ( nm == \""+name+"\" ) { return \""+typ+"\"; }")
	writeln(tab2+"return \"\";")
	writeln(tab+"}")

	### Generate the member declarations
	writeln("")
	for p in params:
		name = p["name"]
		typ = p["type"]
		paramtype = paramtypes[typ]
		writeln(tab+paramtype+" "+name+";")

	writeln("};")
	writeln("")
	writeln("#endif")

def modtime(file):
	try:
		return os.path.getmtime(file)
	except:
		return -1

if __name__ != "__main__":
	print "This code needs to be invoked as a main program."
	sys.exit(1)

if len(sys.argv) < 2:
	print("Usage: %s {paramlist}" % sys.argv[0])
	sys.exit(1)

manifold = os.getenv("VIZBENCH")
if not manifold:
	print("VIZBENCH environment variable isn't set!")
	sys.exit(1)

paramdir = manifold + "/src/params"
os.chdir(paramdir)

parambase = sys.argv[1]
paramclass = parambase+"VizParams"
paramlist = parambase+"VizParams.list"
paramtouch = parambase+"VizParams.touch"
paramnames = parambase+"VizParamsNames"

changed = (modtime(paramlist) > modtime(paramtouch) )
changed = True

if not changed:
	print "No change in "+paramlist
	sys.exit(0)

file = parambase + "VizParams.h"
f = open(file,"w")
sys.stdout = f
params = readparams(paramlist)
genparamheader(params,paramclass)
f.close()

file = parambase + "VizParams.cpp"
f = open(file,"w")
sys.stdout = f
genparamcpp(paramclass)
f.close()

def touch(file):
	f = open(file,"w")
	f.write("# This file exists to record the last build time\n");
	f.close()

touch(paramtouch)
