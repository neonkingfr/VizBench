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
	"depthctlmax",\
	"depthctlmin",\
	"depthctlnum",\
	"depthretrigger_on",\
	"depthretrigger_thresh",\
	"depthsmooth",\
	"duration",\
	"loopclicks",\
	"loopctrl",\
	"loopfade",\
	"notelimit",\
	"pitchmax",\
	"pitchmin",\
	"port",\
	"scale",\
	"scale_on",\
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
		j = cJSON_GetObjectItem(json,"depthctlmax");
		if (j) { depthctlmax.set(j); }
		j = cJSON_GetObjectItem(json,"depthctlmin");
		if (j) { depthctlmin.set(j); }
		j = cJSON_GetObjectItem(json,"depthctlnum");
		if (j) { depthctlnum.set(j); }
		j = cJSON_GetObjectItem(json,"depthretrigger_on");
		if (j) { depthretrigger_on.set(j); }
		j = cJSON_GetObjectItem(json,"depthretrigger_thresh");
		if (j) { depthretrigger_thresh.set(j); }
		j = cJSON_GetObjectItem(json,"depthsmooth");
		if (j) { depthsmooth.set(j); }
		j = cJSON_GetObjectItem(json,"duration");
		if (j) { duration.set(j); }
		j = cJSON_GetObjectItem(json,"loopclicks");
		if (j) { loopclicks.set(j); }
		j = cJSON_GetObjectItem(json,"loopctrl");
		if (j) { loopctrl.set(j); }
		j = cJSON_GetObjectItem(json,"loopfade");
		if (j) { loopfade.set(j); }
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
		j = cJSON_GetObjectItem(json,"scale_on");
		if (j) { scale_on.set(j); }
	}
	void loadDefaults() {
		arpeggiate.set(0);
		channel.set(1);
		depthctlmax.set(127);
		depthctlmin.set(0);
		depthctlnum.set(0);
		depthretrigger_on.set(1);
		depthretrigger_thresh.set(0.000000);
		depthsmooth.set(2);
		duration.set("hold");
		loopclicks.set(768);
		loopctrl.set(1);
		loopfade.set(0.500000);
		notelimit.set(4);
		pitchmax.set(100);
		pitchmin.set(60);
		port.set(0);
		scale.set("newage");
		scale_on.set(1);
	}
	void applyVizParamsFrom(MidiVizParams* p) {
		if ( ! p ) { return; }
		if ( p->arpeggiate.isset() ) { this->arpeggiate.set(p->arpeggiate.get()); }
		if ( p->channel.isset() ) { this->channel.set(p->channel.get()); }
		if ( p->depthctlmax.isset() ) { this->depthctlmax.set(p->depthctlmax.get()); }
		if ( p->depthctlmin.isset() ) { this->depthctlmin.set(p->depthctlmin.get()); }
		if ( p->depthctlnum.isset() ) { this->depthctlnum.set(p->depthctlnum.get()); }
		if ( p->depthretrigger_on.isset() ) { this->depthretrigger_on.set(p->depthretrigger_on.get()); }
		if ( p->depthretrigger_thresh.isset() ) { this->depthretrigger_thresh.set(p->depthretrigger_thresh.get()); }
		if ( p->depthsmooth.isset() ) { this->depthsmooth.set(p->depthsmooth.get()); }
		if ( p->duration.isset() ) { this->duration.set(p->duration.get()); }
		if ( p->loopclicks.isset() ) { this->loopclicks.set(p->loopclicks.get()); }
		if ( p->loopctrl.isset() ) { this->loopctrl.set(p->loopctrl.get()); }
		if ( p->loopfade.isset() ) { this->loopfade.set(p->loopfade.get()); }
		if ( p->notelimit.isset() ) { this->notelimit.set(p->notelimit.get()); }
		if ( p->pitchmax.isset() ) { this->pitchmax.set(p->pitchmax.get()); }
		if ( p->pitchmin.isset() ) { this->pitchmin.set(p->pitchmin.get()); }
		if ( p->port.isset() ) { this->port.set(p->port.get()); }
		if ( p->scale.isset() ) { this->scale.set(p->scale.get()); }
		if ( p->scale_on.isset() ) { this->scale_on.set(p->scale_on.get()); }
	}
	void Set(std::string nm, std::string val) {
		bool stringval = false;
		if ( nm == "arpeggiate" ) {
			arpeggiate.set(string2int(val));
		} else if ( nm == "channel" ) {
			channel.set(string2int(val));
		} else if ( nm == "depthctlmax" ) {
			depthctlmax.set(string2int(val));
		} else if ( nm == "depthctlmin" ) {
			depthctlmin.set(string2int(val));
		} else if ( nm == "depthctlnum" ) {
			depthctlnum.set(string2int(val));
		} else if ( nm == "depthretrigger_on" ) {
			depthretrigger_on.set(string2int(val));
		} else if ( nm == "depthretrigger_thresh" ) {
			depthretrigger_thresh.set(string2double(val));
		} else if ( nm == "depthsmooth" ) {
			depthsmooth.set(string2int(val));
		} else if ( nm == "duration" ) {
			duration.set(val);
			stringval = true;
		} else if ( nm == "loopclicks" ) {
			loopclicks.set(string2int(val));
		} else if ( nm == "loopctrl" ) {
			loopctrl.set(string2int(val));
		} else if ( nm == "loopfade" ) {
			loopfade.set(string2double(val));
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
		} else if ( nm == "scale_on" ) {
			scale_on.set(string2int(val));
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
		} else if ( nm == "depthctlmax" ) {
			depthctlmax.set(adjust(depthctlmax.get(),amount,0,127));
		} else if ( nm == "depthctlmin" ) {
			depthctlmin.set(adjust(depthctlmin.get(),amount,0,127));
		} else if ( nm == "depthctlnum" ) {
			depthctlnum.set(adjust(depthctlnum.get(),amount,0,127));
		} else if ( nm == "depthretrigger_on" ) {
			depthretrigger_on.set(adjust(depthretrigger_on.get(),amount,0,1));
		} else if ( nm == "depthretrigger_thresh" ) {
			depthretrigger_thresh.set(adjust(depthretrigger_thresh.get(),amount,0.000000,1.000000));
		} else if ( nm == "depthsmooth" ) {
			depthsmooth.set(adjust(depthsmooth.get(),amount,0,128));
		} else if ( nm == "duration" ) {
			duration.set(adjust(duration.get(),amount,VizParams::StringVals["durationTypes"]));
		} else if ( nm == "loopclicks" ) {
			loopclicks.set(adjust(loopclicks.get(),amount,1,3072));
		} else if ( nm == "loopctrl" ) {
			loopctrl.set(adjust(loopctrl.get(),amount,0,1));
		} else if ( nm == "loopfade" ) {
			loopfade.set(adjust(loopfade.get(),amount,0.000000,1.000000));
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
		} else if ( nm == "scale_on" ) {
			scale_on.set(adjust(scale_on.get(),amount,0,1));
		}

	}
	std::string DefaultValue(std::string nm) {
		if ( nm == "arpeggiate" ) { return "0"; }
		if ( nm == "channel" ) { return "1"; }
		if ( nm == "depthctlmax" ) { return "127"; }
		if ( nm == "depthctlmin" ) { return "0"; }
		if ( nm == "depthctlnum" ) { return "0"; }
		if ( nm == "depthretrigger_on" ) { return "1"; }
		if ( nm == "depthretrigger_thresh" ) { return "0"; }
		if ( nm == "depthsmooth" ) { return "2"; }
		if ( nm == "duration" ) { return "hold"; }
		if ( nm == "loopclicks" ) { return "768"; }
		if ( nm == "loopctrl" ) { return "1"; }
		if ( nm == "loopfade" ) { return "0.5"; }
		if ( nm == "notelimit" ) { return "4"; }
		if ( nm == "pitchmax" ) { return "100"; }
		if ( nm == "pitchmin" ) { return "60"; }
		if ( nm == "port" ) { return "0"; }
		if ( nm == "scale" ) { return "newage"; }
		if ( nm == "scale_on" ) { return "1"; }
		return "";
	}
	std::string MinValue(std::string nm) {
		if ( nm == "arpeggiate" ) { return "0"; }
		if ( nm == "channel" ) { return "1"; }
		if ( nm == "depthctlmax" ) { return "0"; }
		if ( nm == "depthctlmin" ) { return "0"; }
		if ( nm == "depthctlnum" ) { return "0"; }
		if ( nm == "depthretrigger_on" ) { return "0"; }
		if ( nm == "depthretrigger_thresh" ) { return "0"; }
		if ( nm == "depthsmooth" ) { return "0"; }
		if ( nm == "duration" ) { return "durationTypes"; }
		if ( nm == "loopclicks" ) { return "1"; }
		if ( nm == "loopctrl" ) { return "0"; }
		if ( nm == "loopfade" ) { return "0"; }
		if ( nm == "notelimit" ) { return "1"; }
		if ( nm == "pitchmax" ) { return "1"; }
		if ( nm == "pitchmin" ) { return "1"; }
		if ( nm == "port" ) { return "0"; }
		if ( nm == "scale" ) { return "scaleTypes"; }
		if ( nm == "scale_on" ) { return "0"; }
		return "";
	}
	std::string MaxValue(std::string nm) {
		if ( nm == "arpeggiate" ) { return "1"; }
		if ( nm == "channel" ) { return "16"; }
		if ( nm == "depthctlmax" ) { return "127"; }
		if ( nm == "depthctlmin" ) { return "127"; }
		if ( nm == "depthctlnum" ) { return "127"; }
		if ( nm == "depthretrigger_on" ) { return "1"; }
		if ( nm == "depthretrigger_thresh" ) { return "1"; }
		if ( nm == "depthsmooth" ) { return "128"; }
		if ( nm == "duration" ) { return "durationTypes"; }
		if ( nm == "loopclicks" ) { return "3072"; }
		if ( nm == "loopctrl" ) { return "1"; }
		if ( nm == "loopfade" ) { return "1.0"; }
		if ( nm == "notelimit" ) { return "16"; }
		if ( nm == "pitchmax" ) { return "127"; }
		if ( nm == "pitchmin" ) { return "127"; }
		if ( nm == "port" ) { return "32"; }
		if ( nm == "scale" ) { return "scaleTypes"; }
		if ( nm == "scale_on" ) { return "1"; }
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
		} else if ( nm == "depthctlmax" ) {
			return IntString(depthctlmax.get());
		} else if ( nm == "depthctlmin" ) {
			return IntString(depthctlmin.get());
		} else if ( nm == "depthctlnum" ) {
			return IntString(depthctlnum.get());
		} else if ( nm == "depthretrigger_on" ) {
			return IntString(depthretrigger_on.get());
		} else if ( nm == "depthretrigger_thresh" ) {
			return DoubleString(depthretrigger_thresh.get());
		} else if ( nm == "depthsmooth" ) {
			return IntString(depthsmooth.get());
		} else if ( nm == "duration" ) {
			return duration.get();
		} else if ( nm == "loopclicks" ) {
			return IntString(loopclicks.get());
		} else if ( nm == "loopctrl" ) {
			return IntString(loopctrl.get());
		} else if ( nm == "loopfade" ) {
			return DoubleString(loopfade.get());
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
		} else if ( nm == "scale_on" ) {
			return IntString(scale_on.get());
		}
		return "";
	}
	std::string GetType(std::string nm) {
		if ( nm == "arpeggiate" ) { return "int"; }
		if ( nm == "channel" ) { return "int"; }
		if ( nm == "depthctlmax" ) { return "int"; }
		if ( nm == "depthctlmin" ) { return "int"; }
		if ( nm == "depthctlnum" ) { return "int"; }
		if ( nm == "depthretrigger_on" ) { return "int"; }
		if ( nm == "depthretrigger_thresh" ) { return "double"; }
		if ( nm == "depthsmooth" ) { return "int"; }
		if ( nm == "duration" ) { return "string"; }
		if ( nm == "loopclicks" ) { return "int"; }
		if ( nm == "loopctrl" ) { return "int"; }
		if ( nm == "loopfade" ) { return "double"; }
		if ( nm == "notelimit" ) { return "int"; }
		if ( nm == "pitchmax" ) { return "int"; }
		if ( nm == "pitchmin" ) { return "int"; }
		if ( nm == "port" ) { return "int"; }
		if ( nm == "scale" ) { return "string"; }
		if ( nm == "scale_on" ) { return "int"; }
		return "";
	}

	IntParam arpeggiate;
	IntParam channel;
	IntParam depthctlmax;
	IntParam depthctlmin;
	IntParam depthctlnum;
	IntParam depthretrigger_on;
	DoubleParam depthretrigger_thresh;
	IntParam depthsmooth;
	StringParam duration;
	IntParam loopclicks;
	IntParam loopctrl;
	DoubleParam loopfade;
	IntParam notelimit;
	IntParam pitchmax;
	IntParam pitchmin;
	IntParam port;
	StringParam scale;
	IntParam scale_on;
};

#endif
