/************************************************
 *
 * This file is generated from 'MidiVizParams.list' by genparams.py
 *
 * DO NOT EDIT!
 *
 ************************************************/
#ifndef _MIDIVIZPARAMS_H
#define _MIDIVIZPARAMS_H
#include "VizParams.h"
#include "NosuchJSON.h"

extern char* MidiVizParamsNames[];

#define MidiVizParamsNames_INIT \
	"arpeggiate",\
	"channel",\
	"depthcontroller",\
	"duration",\
	"notelimit",\
	"pitchmax",\
	"pitchmin",\
	"port",\
	"scale",\
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
		j = cJSON_GetObjectItem(json,"arpeggiate");
		if (j) { arpeggiate.set(j); }
		j = cJSON_GetObjectItem(json,"channel");
		if (j) { channel.set(j); }
		j = cJSON_GetObjectItem(json,"depthcontroller");
		if (j) { depthcontroller.set(j); }
		j = cJSON_GetObjectItem(json,"duration");
		if (j) { duration.set(j); }
		j = cJSON_GetObjectItem(json,"notelimit");
		if (j) { notelimit.set(j); }
		j = cJSON_GetObjectItem(json,"pitchmax");
		if (j) { pitchmax.set(j); }
		j = cJSON_GetObjectItem(json,"pitchmin");
		if (j) { pitchmin.set(j); }
		j = cJSON_GetObjectItem(json,"port");
		if (j) { port.set(j); }
		j = cJSON_GetObjectItem(json,"scale");
		if (j) { scale.set(j); }
	}
	void loadDefaults() {
		arpeggiate.set(0);
		channel.set(1);
		depthcontroller.set(0);
		duration.set("hold");
		notelimit.set(4);
		pitchmax.set(100);
		pitchmin.set(60);
		port.set(0);
		scale.set("newage");
	}
	void applyVizParamsFrom(MidiVizParams* p) {
		if ( ! p ) { return; }
		if ( p->arpeggiate.isset() ) { this->arpeggiate.set(p->arpeggiate.get()); }
		if ( p->channel.isset() ) { this->channel.set(p->channel.get()); }
		if ( p->depthcontroller.isset() ) { this->depthcontroller.set(p->depthcontroller.get()); }
		if ( p->duration.isset() ) { this->duration.set(p->duration.get()); }
		if ( p->notelimit.isset() ) { this->notelimit.set(p->notelimit.get()); }
		if ( p->pitchmax.isset() ) { this->pitchmax.set(p->pitchmax.get()); }
		if ( p->pitchmin.isset() ) { this->pitchmin.set(p->pitchmin.get()); }
		if ( p->port.isset() ) { this->port.set(p->port.get()); }
		if ( p->scale.isset() ) { this->scale.set(p->scale.get()); }
	}
	void Set(std::string nm, std::string val) {
		bool stringval = false;
		if ( nm == "arpeggiate" ) {
			arpeggiate.set(string2int(val));
		} else if ( nm == "channel" ) {
			channel.set(string2int(val));
		} else if ( nm == "depthcontroller" ) {
			depthcontroller.set(string2int(val));
		} else if ( nm == "duration" ) {
			duration.set(val);
			stringval = true;
		} else if ( nm == "notelimit" ) {
			notelimit.set(string2int(val));
		} else if ( nm == "pitchmax" ) {
			pitchmax.set(string2int(val));
		} else if ( nm == "pitchmin" ) {
			pitchmin.set(string2int(val));
		} else if ( nm == "port" ) {
			port.set(string2int(val));
		} else if ( nm == "scale" ) {
			scale.set(val);
			stringval = true;
		}

		if ( ! stringval ) {
			Increment(nm,0.0); // abide by limits, using code in Increment
		}
	}
	void Increment(std::string nm, double amount) {
		if ( nm == "arpeggiate" ) {
			arpeggiate.set(adjust(arpeggiate.get(),amount,0,1));
		} else if ( nm == "channel" ) {
			channel.set(adjust(channel.get(),amount,1,16));
		} else if ( nm == "depthcontroller" ) {
			depthcontroller.set(adjust(depthcontroller.get(),amount,0,127));
		} else if ( nm == "duration" ) {
			duration.set(adjust(duration.get(),amount,VizParams::StringVals["durationTypes"]));
		} else if ( nm == "notelimit" ) {
			notelimit.set(adjust(notelimit.get(),amount,1,16));
		} else if ( nm == "pitchmax" ) {
			pitchmax.set(adjust(pitchmax.get(),amount,1,127));
		} else if ( nm == "pitchmin" ) {
			pitchmin.set(adjust(pitchmin.get(),amount,1,127));
		} else if ( nm == "port" ) {
			port.set(adjust(port.get(),amount,0,32));
		} else if ( nm == "scale" ) {
			scale.set(adjust(scale.get(),amount,VizParams::StringVals["scaleTypes"]));
		}

	}
	std::string DefaultValue(std::string nm) {
		if ( nm == "arpeggiate" ) { return "0"; }
		if ( nm == "channel" ) { return "1"; }
		if ( nm == "depthcontroller" ) { return "0"; }
		if ( nm == "duration" ) { return "hold"; }
		if ( nm == "notelimit" ) { return "4"; }
		if ( nm == "pitchmax" ) { return "100"; }
		if ( nm == "pitchmin" ) { return "60"; }
		if ( nm == "port" ) { return "0"; }
		if ( nm == "scale" ) { return "newage"; }
		return "";
	}
	std::string MinValue(std::string nm) {
		if ( nm == "arpeggiate" ) { return "0"; }
		if ( nm == "channel" ) { return "1"; }
		if ( nm == "depthcontroller" ) { return "0"; }
		if ( nm == "duration" ) { return "durationTypes"; }
		if ( nm == "notelimit" ) { return "1"; }
		if ( nm == "pitchmax" ) { return "1"; }
		if ( nm == "pitchmin" ) { return "1"; }
		if ( nm == "port" ) { return "0"; }
		if ( nm == "scale" ) { return "scaleTypes"; }
		return "";
	}
	std::string MaxValue(std::string nm) {
		if ( nm == "arpeggiate" ) { return "1"; }
		if ( nm == "channel" ) { return "16"; }
		if ( nm == "depthcontroller" ) { return "127"; }
		if ( nm == "duration" ) { return "durationTypes"; }
		if ( nm == "notelimit" ) { return "16"; }
		if ( nm == "pitchmax" ) { return "127"; }
		if ( nm == "pitchmin" ) { return "127"; }
		if ( nm == "port" ) { return "32"; }
		if ( nm == "scale" ) { return "scaleTypes"; }
		return "";
	}
	void Toggle(std::string nm) {
		bool stringval = false;
	}
	std::string GetAsString(std::string nm) {
		if ( nm == "arpeggiate" ) {
			return IntString(arpeggiate.get());
		} else if ( nm == "channel" ) {
			return IntString(channel.get());
		} else if ( nm == "depthcontroller" ) {
			return IntString(depthcontroller.get());
		} else if ( nm == "duration" ) {
			return duration.get();
		} else if ( nm == "notelimit" ) {
			return IntString(notelimit.get());
		} else if ( nm == "pitchmax" ) {
			return IntString(pitchmax.get());
		} else if ( nm == "pitchmin" ) {
			return IntString(pitchmin.get());
		} else if ( nm == "port" ) {
			return IntString(port.get());
		} else if ( nm == "scale" ) {
			return scale.get();
		}
		return "";
	}
	std::string GetType(std::string nm) {
		if ( nm == "arpeggiate" ) { return "int"; }
		if ( nm == "channel" ) { return "int"; }
		if ( nm == "depthcontroller" ) { return "int"; }
		if ( nm == "duration" ) { return "string"; }
		if ( nm == "notelimit" ) { return "int"; }
		if ( nm == "pitchmax" ) { return "int"; }
		if ( nm == "pitchmin" ) { return "int"; }
		if ( nm == "port" ) { return "int"; }
		if ( nm == "scale" ) { return "string"; }
		return "";
	}

	IntParam arpeggiate;
	IntParam channel;
	IntParam depthcontroller;
	StringParam duration;
	IntParam notelimit;
	IntParam pitchmax;
	IntParam pitchmin;
	IntParam port;
	StringParam scale;
};

#endif
