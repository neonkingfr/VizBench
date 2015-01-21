#include "NosuchDebug.h"
#include "NosuchJson.h"
#include "VizParams.h"
#include "AllVizParams.h"

bool VizParams::Initialized = false;

std::map<std::string, StringList> VizParams::StringVals;

int
string2int(std::string s) {
	return atoi(s.c_str());
}

float
string2double(std::string s) {
	return (float)atof(s.c_str());
}

bool
string2bool(std::string s) {
	if (s == "") {
		DEBUGPRINT(("Unexpected empty value in string2bool!?"));
		return false;
	}
	if (s == "true" || s[0] == '1' || s == "on") return true;
	if (s == "false" || s[0] == '0' || s == "off") return false;
	return false;
}