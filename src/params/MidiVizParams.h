#ifndef _MIDIVIZPARAMS_H
#define _MIDIVIZPARAMS_H

#include "NosuchJSON.h"

extern char* MidiVizParamsNames[];

#define MidiVizParamsNames_INIT \
	"channel",\
	"pitchmax",\
	"pitchmin",\
	"port",\
	NULL

class MidiVizParams : public VizParams {
public:
	MidiVizParams() {
		loadDefaults();
	}
	char **ListOfNames() { return MidiVizParamsNames; }
	// std::string JsonListOfValues() { return _JsonListOfValues(MidiVizParamsNames); }
	// std::string JsonListOfParams() { return _JsonListOfParams(MidiVizParamsNames); }
	std::string JsonListOfStringValues(std::string type) { return _JsonListOfStringValues(type); }
	void loadJson(cJSON* json) {
		cJSON* j;
		j = cJSON_GetObjectItem(json,"channel");
		if (j) { channel.set(j); }
		j = cJSON_GetObjectItem(json,"pitchmax");
		if (j) { pitchmax.set(j); }
		j = cJSON_GetObjectItem(json,"pitchmin");
		if (j) { pitchmin.set(j); }
		j = cJSON_GetObjectItem(json,"port");
		if (j) { port.set(j); }
	}
	void loadDefaults() {
		channel.set(1);
		pitchmax.set(100);
		pitchmin.set(60);
		port.set(1);
	}
	void applyVizParamsFrom(MidiVizParams* p) {
		if ( ! p ) { return; }
		if ( p->channel.isset() ) { this->channel.set(p->channel.get()); }
		if ( p->pitchmax.isset() ) { this->pitchmax.set(p->pitchmax.get()); }
		if ( p->pitchmin.isset() ) { this->pitchmin.set(p->pitchmin.get()); }
		if ( p->port.isset() ) { this->port.set(p->port.get()); }
	}
	void Set(std::string nm, std::string val) {
		bool stringval = false;
		if ( nm == "channel" ) {
			channel.set(string2int(val));
		} else if ( nm == "pitchmax" ) {
			pitchmax.set(string2int(val));
		} else if ( nm == "pitchmin" ) {
			pitchmin.set(string2int(val));
		} else if ( nm == "port" ) {
			port.set(string2int(val));
		}

		if ( ! stringval ) {
			Increment(nm,0.0); // abide by limits, using code in Increment
		}
	}
	void Increment(std::string nm, double amount) {
		if ( nm == "channel" ) {
			channel.set(adjust(channel.get(),amount,1,16));
		} else if ( nm == "pitchmax" ) {
			pitchmax.set(adjust(pitchmax.get(),amount,1,127));
		} else if ( nm == "pitchmin" ) {
			pitchmin.set(adjust(pitchmin.get(),amount,1,127));
		} else if ( nm == "port" ) {
			port.set(adjust(port.get(),amount,1,32));
		}

	}
	std::string DefaultValue(std::string nm) {
		if ( nm == "channel" ) { return "1"; }
		if ( nm == "pitchmax" ) { return "100"; }
		if ( nm == "pitchmin" ) { return "60"; }
		if ( nm == "port" ) { return "1"; }
		return "";
	}
	std::string MinValue(std::string nm) {
		if ( nm == "channel" ) { return "1"; }
		if ( nm == "pitchmax" ) { return "1"; }
		if ( nm == "pitchmin" ) { return "1"; }
		if ( nm == "port" ) { return "1"; }
		return "";
	}
	std::string MaxValue(std::string nm) {
		if ( nm == "channel" ) { return "16"; }
		if ( nm == "pitchmax" ) { return "127"; }
		if ( nm == "pitchmin" ) { return "127"; }
		if ( nm == "port" ) { return "32"; }
		return "";
	}
	void Toggle(std::string nm) {
		bool stringval = false;
	}
	std::string GetAsString(std::string nm) {
		if ( nm == "channel" ) {
			return IntString(channel.get());
		} else if ( nm == "pitchmax" ) {
			return IntString(pitchmax.get());
		} else if ( nm == "pitchmin" ) {
			return IntString(pitchmin.get());
		} else if ( nm == "port" ) {
			return IntString(port.get());
		}
		return "";
	}
	std::string GetType(std::string nm) {
		if ( nm == "channel" ) { return "int"; }
		if ( nm == "pitchmax" ) { return "int"; }
		if ( nm == "pitchmin" ) { return "int"; }
		if ( nm == "port" ) { return "int"; }
		return "";
	}

	IntParam channel;
	IntParam pitchmax;
	IntParam pitchmin;
	IntParam port;
};

#endif
