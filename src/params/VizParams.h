#ifndef _VIZPARAMS_H
#define _VIZPARAMS_H

#include <vector>
#include <map>
#include "VizException.h"
#include "VizJson.h"
#include "Scale.h"

#define IntString(x) VizSnprintf("%d",x)
#define DoubleString(x) VizSnprintf("%f",x)
#define BoolString(x) VizSnprintf("%s",x?"on":"off")

typedef std::vector<std::string> StringList;

int string2int(std::string s);
float string2double(std::string s);
bool string2bool(std::string s);

std::string VizParamsPath(std::string f,std::string paramtype);
std::string SpriteVizParamsPath(std::string f);
std::string MidiVizParamsPath(std::string f);
std::string PluginParamsPath(std::string f);
std::string pipelinePath(std::string name);
std::string pipesetPath(std::string name);

class DoubleParam {
public:
	DoubleParam() { enabled = false; value = 0.0;  }
	double get() { return value; }
	void set(double v) { value = v; enabled = true; }
	void set(cJSON* j) {
		if (j->type == cJSON_String) {
			set(atof(j->valuestring));
		}
		else if (j->type == cJSON_Number) {
			set(j->valuedouble);
		}
		else {
			throw VizException("Unable to set DoubleParam from j->type=%d", j->type);
		}
	}
	void unset() { enabled = false; }
	operator double() { return value; }
	bool isset() { return enabled; }
private:
	double value;
	bool enabled;
};

class BoolParam {
public:
	BoolParam() { enabled = false; value = false; }
	bool get() { return value; }
	void set(bool v) { value = v; enabled = true; }
	void set(cJSON* j) {
		switch (j->type) {
		case cJSON_True: set(true); break;
		case cJSON_False: set(false); break;
		case cJSON_String: set(jsonIsTrueValue(j->valuestring)); break;
		case cJSON_Number: set(j->valueint != 0); break;
		default:
			throw VizException("Unable to set BoolParam from j->type=%d", j->type);
		}
	}
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
	void set(cJSON* j) {
		if (j->type == cJSON_Number) {
			if ((j->valuedouble - j->valueint) != 0.0) {
				set(VizSnprintf("%f", j->valuedouble));
			}
			else {
				set(VizSnprintf("%d", j->valueint));
			}
		}
		else if (j->type == cJSON_String) {
			set(j->valuestring);
		}
		else {
			throw VizException("Unable to set StringParam from j->type=%d", j->type);
		}
	}
	operator std::string() { return value; }
	bool isset() { return enabled; }
private:
	std::string value;
	bool enabled;
};

class IntParam {
public:
	IntParam() { enabled = false; value = 0; }
	int get() { return value; }
	void set(int v) { value = v; enabled = true; }
	void set(cJSON* j) {
		if (j->type == cJSON_Number) {
			set(j->valueint);
		}
		else if (j->type == cJSON_String) {
			set(atoi(j->valuestring));
		}
		else {
			throw VizException("Unable to set IntParam from j->type=%d", j->type);
		}
	}
	operator int() { return value; }
	bool isset() { return enabled; }
private:
	int value;
	bool enabled;
};

class VizParams {

public:

	static std::map<std::string, StringList> StringVals;
	static void Initialize();
	static bool Initialized;

	virtual std::string GetAsString(std::string) = 0;
	virtual std::string GetType(std::string) = 0;
	virtual std::string MinValue(std::string) = 0;
	virtual std::string MaxValue(std::string) = 0;
	virtual std::string DefaultValue(std::string) = 0;

	double adjust(double v, double amount, double vmin, double vmax) {
		v += amount*(vmax - vmin);
		if (v < vmin)
			v = vmin;
		else if (v > vmax)
			v = vmax;
		return v;
	}
	int adjust(int v, double amount, int vmin, int vmax) {
		int incamount = (int)(amount*(vmax - vmin));
		if (incamount == 0) {
			incamount = (amount > 0.0) ? 1 : -1;
		}
		v = (int)(v + incamount);
		if (v < vmin)
			v = vmin;
		else if (v > vmax)
			v = vmax;
		return v;
	}
	bool adjust(bool v, double amount) {
		if (amount > 0.0) {
			return true;
		}
		if (amount < 0.0) {
			return false;
		}
		// if amount is 0.0, no change.
		return v;
	}
	std::string adjust(std::string v, double amount, std::vector<std::string>& vals) {
		// Find the existing value
		int existing = -1;
		int sz = vals.size();
		if (sz == 0) {
			throw VizException("vals array is empty!?");
		}
		for (int ei = 0; ei < sz; ei++) {
			if (v == vals[ei]) {
				existing = ei;
				break;
			}
		}
		if (existing < 0) {
			existing = 0;
		}
		// Return the next or previous value in the list
		int i = existing + ((amount > 0.0) ? 1 : -1);
		if (i < 0) {
			i = sz - 1;
		}
		if (i >= sz) {
			i = 0;
		}
		return vals[i];
	}
	// std::string JsonListOfValues() { return _JsonListOfValues(SpriteVizParamsNames); }
	// std::string JsonListOfParams() { return _JsonListOfParams(SpriteVizParamsNames); }
	virtual char **ListOfNames() = 0;

	std::string JsonListOfValues() {
		char **names = ListOfNames();
		std::string s = "{";
		std::string sep = "";
		for (char** nm = names; *nm; nm++) {
			s += (sep + "\"" + *nm + "\": \"" + GetAsString(*nm) + "\"");
			sep = ",";
		}
		s += "}";
		return s;
	}
	std::string _JsonListOfStringValues(std::string type) {
		StringList vals = StringVals[type];
		std::string s = "[";
		std::string sep = "";
		for (std::vector<std::string>::iterator it = vals.begin(); it != vals.end(); ++it) {
			s += (sep + "\"" + *it + "\"");
			sep = ",";
		}
		s += "]";
		return s;
	}
	std::string JsonListOfParams() {
		char **names = ListOfNames();
		std::string s = "{ ";
		std::string sep = "";
		int n = 0;
		for (char** pnm = names; *pnm; pnm++) {
			char* nm = *pnm;
			std::string t = GetType(nm);
			std::string mn = MinValue(nm);
			std::string mx = MaxValue(nm);
			std::string dflt = DefaultValue(nm);
			s += (sep + "\"" + nm + "\": ");
			s += "{ \"type\": \"" + t + "\", \"min\": \"" + mn + "\", \"max\": \"" + mx + "\", \"default\": \"" + dflt + "\" }";
			sep = ",";
		}
		s += "}";
		return s;
	}
};

#endif