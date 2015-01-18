#ifndef _VIZPARAMS_H
#define _VIZPARAMS_H

#include <vector>
#include <map>
#include "NosuchException.h"

std::string JsonAllValues();
std::string JsonAllTypes();

#define IntString(x) NosuchSnprintf("%d",x)
#define DoubleString(x) NosuchSnprintf("%f",x)
#define BoolString(x) NosuchSnprintf("%s",x?"on":"off")

int string2int(std::string s);
float string2double(std::string s);
bool string2bool(std::string s);

class DoubleParam {
public:
	DoubleParam() { enabled = false; }
	double get() { return value; }
	void set(double v) { value = v; enabled = true; }
	void unset() { enabled = false; }
	operator double() { return value; }
	bool isset() { return enabled; }
private:
	double value;
	bool enabled;
};

class BoolParam {
public:
	BoolParam() { enabled = false; }
	bool get() { return value; }
	void set(bool v) { value = v; enabled = true; }
	operator bool() { return value; }
	bool isset() { return enabled; }
private:
	bool value;
	bool enabled;
};

class StringParam {
public:
	StringParam() { enabled = false; }
	std::string get() { return value; }
	void set(std::string v) { value = v; enabled = true; }
	operator std::string() { return value; }
	bool isset() { return enabled; }
private:
	std::string value;
	bool enabled;
};

class IntParam {
public:
	IntParam() { enabled = false; }
	int get() { return value; }
	void set(int v) { value = v; enabled = true; }
	operator int() { return value; }
	bool isset() { return enabled; }
private:
	int value;
	bool enabled;
};

class VizParams {

public:

	static std::vector<std::string> mirrorTypes;
	static std::vector<std::string> shapeTypes;
	static std::vector<std::string> controllerTypes;
	static std::vector<std::string> behaviourTypes;
	static std::vector<std::string> movedirTypes;
	static std::vector<std::string> rotangdirTypes;
	static std::vector<std::string> logicTypes;

	static bool Initialized;
	static void Initialize() {
	
		if ( Initialized ) {
			return;
		}

		Initialized = true;

		shapeTypes.push_back("nothing");  // make sure that sprite 0 is nothing
		shapeTypes.push_back("line");
		shapeTypes.push_back("triangle");
		shapeTypes.push_back("square");
		shapeTypes.push_back("arc");
		shapeTypes.push_back("circle");
		shapeTypes.push_back("outline");
	
		mirrorTypes.push_back("default");
		mirrorTypes.push_back("vertical");
		mirrorTypes.push_back("horizontal");

		logicTypes.push_back("none");
		logicTypes.push_back("vertical");
		logicTypes.push_back("horizontal");
		logicTypes.push_back("outline");

		behaviourTypes.push_back("default");
		behaviourTypes.push_back("museum");
		behaviourTypes.push_back("STEIM");
		behaviourTypes.push_back("casual");
		behaviourTypes.push_back("burn");

		controllerTypes.push_back("modulationonly");
		controllerTypes.push_back("allcontrollers");
		controllerTypes.push_back("pitchYZ");

		movedirTypes.push_back("cursor");
		movedirTypes.push_back("left");
		movedirTypes.push_back("right");
		movedirTypes.push_back("up");
		movedirTypes.push_back("down");
		movedirTypes.push_back("random");

		rotangdirTypes.push_back("right");
		rotangdirTypes.push_back("left");
		rotangdirTypes.push_back("random");
	}

	virtual std::string GetAsString(std::string) = 0;
	virtual std::string GetType(std::string) = 0;
	virtual std::string MinValue(std::string) = 0;
	virtual std::string MaxValue(std::string) = 0;

	double adjust(double v, double amount, double vmin, double vmax) {
		v += amount*(vmax-vmin);
		if ( v < vmin )
			v = vmin;
		else if ( v > vmax )
			v = vmax;
		return v;
	}
	int adjust(int v, double amount, int vmin, int vmax) {
		int incamount = (int)(amount*(vmax-vmin));
		if ( incamount == 0 ) {
			incamount = (amount>0.0) ? 1 : -1;
		}
		v = (int)(v + incamount);
		if ( v < vmin )
			v = vmin;
		else if ( v > vmax )
			v = vmax;
		return v;
	}
	bool adjust(bool v, double amount) {
		if ( amount > 0.0 ) {
			return true;
		}
		if ( amount < 0.0 ) {
			return false;
		}
		// if amount is 0.0, no change.
		return v;
	}
	std::string adjust(std::string v, double amount, std::vector<std::string>& vals) {
		// Find the existing value
		int existing = -1;
		int sz = vals.size();
		if ( sz == 0 ) {
			throw NosuchException("vals array is empty!?");
		}
		for ( int ei=0; ei<sz; ei++ ) {
			if ( v == vals[ei] ) {
				existing = ei;
				break;
			}
		}
		if ( existing < 0 ) {
			existing = 0;
		}
		// Return the next or previous value in the list
		int i = existing + ((amount>0.0)?1:-1);
		if ( i < 0 ) {
			i = sz-1;
		}
		if ( i >= sz ) {
			i = 0;
		}
		return vals[i];
	}
	std::string _JsonListOfValues(char* names[]) {
		std::string s = "{";
		std::string sep = "";
		for ( char** nm=names; *nm; nm++ ) {
			s += (sep + "\"" + *nm + "\": \"" + GetAsString(*nm) + "\"");
			sep = ",";
		}
		s += "}";
		return s;
	}
	std::string _JsonListOfTypes(char* names[]) {

		std::string s = "{ ";
		std::string sep = "";
		int n = 0;
		for ( char** pnm=names; *pnm; pnm++ ) {
			char* nm = *pnm;
			std::string t = GetType(nm);
			std::string mn = MinValue(nm);
			std::string mx = MaxValue(nm);
			s += (sep + "\"" + nm + "\": ");
			s += "{ \"type\": \"" + t + "\", \"min\": \"" + mn + "\", \"max\": \"" + mx + "\" }";
			sep = ",";
		}
		s += "}";
		return s;
	}
};

#endif