﻿# This script reads *VizParams.list files that define Vizlet parameters
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
		if len(ln) == 0:
			continue
		if ln[0] == '#':
			continue
		if ln[0] == ':':
			# These lines are used to define string values
			vals = ln.split(None,5)
			(name,typ,mn,mx,default,comment) = vals
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

## utility to make sure floating-point values are printed with a decimal point
## so function calls/etc get disambiguated between double and int.
def s2d(d):
	return "%f" % float(d);


def genparamheader(params,classname):

	uptype = classname.upper()
	tab = "\t"
	tab2 = "\t\t"
	tab3 = "\t\t\t"

	writeln("#ifndef _"+uptype+"_H")
	writeln("#define _"+uptype+"_H")

	writeln("#include \"VizParams.h\"")
	writeln("#include \"VizJSON.h\"")

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
	writeln(tab+classname+"() {")
	writeln(tab2+"loadDefaults();")
	writeln(tab+"}")

	writeln(tab+"char **ListOfNames() { return "+paramnames+"; }");
	writeln(tab+"// std::string JsonListOfValues() { return _JsonListOfValues("+paramnames+"); }");
	writeln(tab+"// std::string JsonListOfParams() { return _JsonListOfParams("+paramnames+"); }");
	writeln(tab+"std::string JsonListOfStringValues(std::string type) { return _JsonListOfStringValues(type); }");

	### Generate the method that loads JSON
	writeln(tab+"void loadJson(cJSON* json) {")
	writeln(tab2+"cJSON* j;")
	for p in params:
		name = p["name"]
		typ = p["type"]
		writeln(tab2+"j = cJSON_GetObjectItem(json,\""+name+"\");")
		writeln(tab2+"if (j) { "+name+".set(j); }")
	writeln(tab+"}")

	### Generate the method that loads default values
	writeln(tab+"void loadDefaults() {")
	for p in params:
		name = p["name"]
		typ = p["type"]
		defaultvalue = p["default"]
		if typ == "double":
			defaultvalue = s2d(defaultvalue)
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
			writeln(tab3+name+".set(adjust("+name+".get(),amount,"+s2d(mn)+","+s2d(mx)+"));")
		elif typ == "int":
			writeln(tab3+name+".set(adjust("+name+".get(),amount,"+mn+","+mx+"));")
		elif typ == "bool":
			writeln(tab3+name+".set(adjust("+name+".get(),amount));")
		elif typ == "string":
			vals = p["min"]
			if vals == "*":
				writeln(tab3+"// '*' means the value can be anything");
			else:
				writeln(tab3+name+".set(adjust("+name+".get(),amount,VizParams::StringVals[\""+vals+"\"]));")
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

# We expect this program to be invoked from the VizBench/bin directory
# so everything can be full paths without depending on environment variables

paramdir = "../src/params"
if not os.path.isdir(paramdir):
	print("No directory "+paramdir+" !?")
	sys.exit(1)

os.chdir(paramdir)

force = False
if len(sys.argv) > 2 and sys.argv[1] == "-f":
	force = True
	parambase = sys.argv[2]
else:
	parambase = sys.argv[1]

paramclass = parambase+"VizParams"
paramlist = parambase+"VizParams.list"
paramtouch = parambase+"VizParams.touch"
paramnames = parambase+"VizParamsNames"
file_h = parambase + "VizParams.h"
file_cpp = parambase + "VizParams.cpp"

changed = force or (modtime(paramlist) > modtime(paramtouch) ) or not os.path.exists(file_h) or not os.path.exists(file_cpp)

if not changed:
	print "No change in "+paramlist
	sys.exit(0)

do_not_edit = "/************************************************\n" \
			  " *\n" \
              " * This file is generated from '"+paramlist+"' by genparams.py\n" \
			  " *\n" \
			  " * DO NOT EDIT!\n" \
			  " *\n" \
			  " ************************************************/\n";

f = open(file_h,"w")
f.write(do_not_edit)
sys.stdout = f
params = readparams(paramlist)
genparamheader(params,paramclass)
f.close()

f = open(file_cpp,"w")
f.write(do_not_edit);
sys.stdout = f
genparamcpp(paramclass)
f.close()

def touch(filename):
	f = open(filename,"w")
	f.write("# This file exists to record the last build time\n");
	f.close()

touch(paramtouch)
