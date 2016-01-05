#ifndef _SPRITEVIZPARAMS_H
#define _SPRITEVIZPARAMS_H
#include "VizParams.h"
#include "NosuchJSON.h"

extern char* SpriteVizParamsNames[];

#define SpriteVizParamsNames_INIT \
	"alphafinal",\
	"alphainitial",\
	"alphatime",\
	"aspect",\
	"bounce",\
	"filled",\
	"gravity",\
	"huefillfinal",\
	"huefillinitial",\
	"huefilltime",\
	"huefinal",\
	"hueinitial",\
	"huetime",\
	"lifetime",\
	"luminance",\
	"mass",\
	"mirror",\
	"movedir",\
	"movedirrandom",\
	"movefollowcursor",\
	"noisevertex",\
	"nsprites",\
	"pitchmax",\
	"pitchmin",\
	"pitchwrap",\
	"placement",\
	"rotanginit",\
	"rotangspeed",\
	"rotdirrandom",\
	"saturation",\
	"shape",\
	"sizefinal",\
	"sizeinitial",\
	"sizetime",\
	"speedinitial",\
	"thickness",\
	"zable",\
	NULL

class SpriteVizParams : public VizParams {
public:
	SpriteVizParams() {
		loadDefaults();
	}
	char **ListOfNames() { return SpriteVizParamsNames; }
	// std::string JsonListOfValues() { return _JsonListOfValues(SpriteVizParamsNames); }
	// std::string JsonListOfParams() { return _JsonListOfParams(SpriteVizParamsNames); }
	std::string JsonListOfStringValues(std::string type) { return _JsonListOfStringValues(type); }
	void loadJson(cJSON* json) {
		cJSON* j;
		j = cJSON_GetObjectItem(json,"alphafinal");
		if (j) { alphafinal.set(j); }
		j = cJSON_GetObjectItem(json,"alphainitial");
		if (j) { alphainitial.set(j); }
		j = cJSON_GetObjectItem(json,"alphatime");
		if (j) { alphatime.set(j); }
		j = cJSON_GetObjectItem(json,"aspect");
		if (j) { aspect.set(j); }
		j = cJSON_GetObjectItem(json,"bounce");
		if (j) { bounce.set(j); }
		j = cJSON_GetObjectItem(json,"filled");
		if (j) { filled.set(j); }
		j = cJSON_GetObjectItem(json,"gravity");
		if (j) { gravity.set(j); }
		j = cJSON_GetObjectItem(json,"huefillfinal");
		if (j) { huefillfinal.set(j); }
		j = cJSON_GetObjectItem(json,"huefillinitial");
		if (j) { huefillinitial.set(j); }
		j = cJSON_GetObjectItem(json,"huefilltime");
		if (j) { huefilltime.set(j); }
		j = cJSON_GetObjectItem(json,"huefinal");
		if (j) { huefinal.set(j); }
		j = cJSON_GetObjectItem(json,"hueinitial");
		if (j) { hueinitial.set(j); }
		j = cJSON_GetObjectItem(json,"huetime");
		if (j) { huetime.set(j); }
		j = cJSON_GetObjectItem(json,"lifetime");
		if (j) { lifetime.set(j); }
		j = cJSON_GetObjectItem(json,"luminance");
		if (j) { luminance.set(j); }
		j = cJSON_GetObjectItem(json,"mass");
		if (j) { mass.set(j); }
		j = cJSON_GetObjectItem(json,"mirror");
		if (j) { mirror.set(j); }
		j = cJSON_GetObjectItem(json,"movedir");
		if (j) { movedir.set(j); }
		j = cJSON_GetObjectItem(json,"movedirrandom");
		if (j) { movedirrandom.set(j); }
		j = cJSON_GetObjectItem(json,"movefollowcursor");
		if (j) { movefollowcursor.set(j); }
		j = cJSON_GetObjectItem(json,"noisevertex");
		if (j) { noisevertex.set(j); }
		j = cJSON_GetObjectItem(json,"nsprites");
		if (j) { nsprites.set(j); }
		j = cJSON_GetObjectItem(json,"pitchmax");
		if (j) { pitchmax.set(j); }
		j = cJSON_GetObjectItem(json,"pitchmin");
		if (j) { pitchmin.set(j); }
		j = cJSON_GetObjectItem(json,"pitchwrap");
		if (j) { pitchwrap.set(j); }
		j = cJSON_GetObjectItem(json,"placement");
		if (j) { placement.set(j); }
		j = cJSON_GetObjectItem(json,"rotanginit");
		if (j) { rotanginit.set(j); }
		j = cJSON_GetObjectItem(json,"rotangspeed");
		if (j) { rotangspeed.set(j); }
		j = cJSON_GetObjectItem(json,"rotdirrandom");
		if (j) { rotdirrandom.set(j); }
		j = cJSON_GetObjectItem(json,"saturation");
		if (j) { saturation.set(j); }
		j = cJSON_GetObjectItem(json,"shape");
		if (j) { shape.set(j); }
		j = cJSON_GetObjectItem(json,"sizefinal");
		if (j) { sizefinal.set(j); }
		j = cJSON_GetObjectItem(json,"sizeinitial");
		if (j) { sizeinitial.set(j); }
		j = cJSON_GetObjectItem(json,"sizetime");
		if (j) { sizetime.set(j); }
		j = cJSON_GetObjectItem(json,"speedinitial");
		if (j) { speedinitial.set(j); }
		j = cJSON_GetObjectItem(json,"thickness");
		if (j) { thickness.set(j); }
		j = cJSON_GetObjectItem(json,"zable");
		if (j) { zable.set(j); }
	}
	void loadDefaults() {
		alphafinal.set(0.0);
		alphainitial.set(1.0);
		alphatime.set(2.0);
		aspect.set(1.0);
		bounce.set(false);
		filled.set(true);
		gravity.set(false);
		huefillfinal.set(0.0);
		huefillinitial.set(360.0);
		huefilltime.set(2.0);
		huefinal.set(360.0);
		hueinitial.set(0.0);
		huetime.set(2.0);
		lifetime.set(3.0);
		luminance.set(0.5);
		mass.set(0.5);
		mirror.set("none");
		movedir.set(0.0);
		movedirrandom.set(false);
		movefollowcursor.set(false);
		noisevertex.set(0.0);
		nsprites.set(1000);
		pitchmax.set(90);
		pitchmin.set(40);
		pitchwrap.set(true);
		placement.set("center");
		rotanginit.set(0.0);
		rotangspeed.set(0.0);
		rotdirrandom.set(false);
		saturation.set(1.0);
		shape.set("circle");
		sizefinal.set(0.5);
		sizeinitial.set(0.2);
		sizetime.set(2.0);
		speedinitial.set(0.0);
		thickness.set(3.0);
		zable.set(true);
	}
	void applyVizParamsFrom(SpriteVizParams* p) {
		if ( ! p ) { return; }
		if ( p->alphafinal.isset() ) { this->alphafinal.set(p->alphafinal.get()); }
		if ( p->alphainitial.isset() ) { this->alphainitial.set(p->alphainitial.get()); }
		if ( p->alphatime.isset() ) { this->alphatime.set(p->alphatime.get()); }
		if ( p->aspect.isset() ) { this->aspect.set(p->aspect.get()); }
		if ( p->bounce.isset() ) { this->bounce.set(p->bounce.get()); }
		if ( p->filled.isset() ) { this->filled.set(p->filled.get()); }
		if ( p->gravity.isset() ) { this->gravity.set(p->gravity.get()); }
		if ( p->huefillfinal.isset() ) { this->huefillfinal.set(p->huefillfinal.get()); }
		if ( p->huefillinitial.isset() ) { this->huefillinitial.set(p->huefillinitial.get()); }
		if ( p->huefilltime.isset() ) { this->huefilltime.set(p->huefilltime.get()); }
		if ( p->huefinal.isset() ) { this->huefinal.set(p->huefinal.get()); }
		if ( p->hueinitial.isset() ) { this->hueinitial.set(p->hueinitial.get()); }
		if ( p->huetime.isset() ) { this->huetime.set(p->huetime.get()); }
		if ( p->lifetime.isset() ) { this->lifetime.set(p->lifetime.get()); }
		if ( p->luminance.isset() ) { this->luminance.set(p->luminance.get()); }
		if ( p->mass.isset() ) { this->mass.set(p->mass.get()); }
		if ( p->mirror.isset() ) { this->mirror.set(p->mirror.get()); }
		if ( p->movedir.isset() ) { this->movedir.set(p->movedir.get()); }
		if ( p->movedirrandom.isset() ) { this->movedirrandom.set(p->movedirrandom.get()); }
		if ( p->movefollowcursor.isset() ) { this->movefollowcursor.set(p->movefollowcursor.get()); }
		if ( p->noisevertex.isset() ) { this->noisevertex.set(p->noisevertex.get()); }
		if ( p->nsprites.isset() ) { this->nsprites.set(p->nsprites.get()); }
		if ( p->pitchmax.isset() ) { this->pitchmax.set(p->pitchmax.get()); }
		if ( p->pitchmin.isset() ) { this->pitchmin.set(p->pitchmin.get()); }
		if ( p->pitchwrap.isset() ) { this->pitchwrap.set(p->pitchwrap.get()); }
		if ( p->placement.isset() ) { this->placement.set(p->placement.get()); }
		if ( p->rotanginit.isset() ) { this->rotanginit.set(p->rotanginit.get()); }
		if ( p->rotangspeed.isset() ) { this->rotangspeed.set(p->rotangspeed.get()); }
		if ( p->rotdirrandom.isset() ) { this->rotdirrandom.set(p->rotdirrandom.get()); }
		if ( p->saturation.isset() ) { this->saturation.set(p->saturation.get()); }
		if ( p->shape.isset() ) { this->shape.set(p->shape.get()); }
		if ( p->sizefinal.isset() ) { this->sizefinal.set(p->sizefinal.get()); }
		if ( p->sizeinitial.isset() ) { this->sizeinitial.set(p->sizeinitial.get()); }
		if ( p->sizetime.isset() ) { this->sizetime.set(p->sizetime.get()); }
		if ( p->speedinitial.isset() ) { this->speedinitial.set(p->speedinitial.get()); }
		if ( p->thickness.isset() ) { this->thickness.set(p->thickness.get()); }
		if ( p->zable.isset() ) { this->zable.set(p->zable.get()); }
	}
	void Set(std::string nm, std::string val) {
		bool stringval = false;
		if ( nm == "alphafinal" ) {
			alphafinal.set(string2double(val));
		} else if ( nm == "alphainitial" ) {
			alphainitial.set(string2double(val));
		} else if ( nm == "alphatime" ) {
			alphatime.set(string2double(val));
		} else if ( nm == "aspect" ) {
			aspect.set(string2double(val));
		} else if ( nm == "bounce" ) {
			bounce.set(string2bool(val));
		} else if ( nm == "filled" ) {
			filled.set(string2bool(val));
		} else if ( nm == "gravity" ) {
			gravity.set(string2bool(val));
		} else if ( nm == "huefillfinal" ) {
			huefillfinal.set(string2double(val));
		} else if ( nm == "huefillinitial" ) {
			huefillinitial.set(string2double(val));
		} else if ( nm == "huefilltime" ) {
			huefilltime.set(string2double(val));
		} else if ( nm == "huefinal" ) {
			huefinal.set(string2double(val));
		} else if ( nm == "hueinitial" ) {
			hueinitial.set(string2double(val));
		} else if ( nm == "huetime" ) {
			huetime.set(string2double(val));
		} else if ( nm == "lifetime" ) {
			lifetime.set(string2double(val));
		} else if ( nm == "luminance" ) {
			luminance.set(string2double(val));
		} else if ( nm == "mass" ) {
			mass.set(string2double(val));
		} else if ( nm == "mirror" ) {
			mirror.set(val);
			stringval = true;
		} else if ( nm == "movedir" ) {
			movedir.set(string2double(val));
		} else if ( nm == "movedirrandom" ) {
			movedirrandom.set(string2bool(val));
		} else if ( nm == "movefollowcursor" ) {
			movefollowcursor.set(string2bool(val));
		} else if ( nm == "noisevertex" ) {
			noisevertex.set(string2double(val));
		} else if ( nm == "nsprites" ) {
			nsprites.set(string2int(val));
		} else if ( nm == "pitchmax" ) {
			pitchmax.set(string2int(val));
		} else if ( nm == "pitchmin" ) {
			pitchmin.set(string2int(val));
		} else if ( nm == "pitchwrap" ) {
			pitchwrap.set(string2bool(val));
		} else if ( nm == "placement" ) {
			placement.set(val);
			stringval = true;
		} else if ( nm == "rotanginit" ) {
			rotanginit.set(string2double(val));
		} else if ( nm == "rotangspeed" ) {
			rotangspeed.set(string2double(val));
		} else if ( nm == "rotdirrandom" ) {
			rotdirrandom.set(string2bool(val));
		} else if ( nm == "saturation" ) {
			saturation.set(string2double(val));
		} else if ( nm == "shape" ) {
			shape.set(val);
			stringval = true;
		} else if ( nm == "sizefinal" ) {
			sizefinal.set(string2double(val));
		} else if ( nm == "sizeinitial" ) {
			sizeinitial.set(string2double(val));
		} else if ( nm == "sizetime" ) {
			sizetime.set(string2double(val));
		} else if ( nm == "speedinitial" ) {
			speedinitial.set(string2double(val));
		} else if ( nm == "thickness" ) {
			thickness.set(string2double(val));
		} else if ( nm == "zable" ) {
			zable.set(string2bool(val));
		}

		if ( ! stringval ) {
			Increment(nm,0.0); // abide by limits, using code in Increment
		}
	}
	void Increment(std::string nm, double amount) {
		if ( nm == "alphafinal" ) {
			alphafinal.set(adjust(alphafinal.get(),amount,0.0,1.0));
		} else if ( nm == "alphainitial" ) {
			alphainitial.set(adjust(alphainitial.get(),amount,0.0,1.0));
		} else if ( nm == "alphatime" ) {
			alphatime.set(adjust(alphatime.get(),amount,0.01,10.0));
		} else if ( nm == "aspect" ) {
			aspect.set(adjust(aspect.get(),amount,1.0,4.0));
		} else if ( nm == "bounce" ) {
			bounce.set(adjust(bounce.get(),amount));
		} else if ( nm == "filled" ) {
			filled.set(adjust(filled.get(),amount));
		} else if ( nm == "gravity" ) {
			gravity.set(adjust(gravity.get(),amount));
		} else if ( nm == "huefillfinal" ) {
			huefillfinal.set(adjust(huefillfinal.get(),amount,0.0,360.0));
		} else if ( nm == "huefillinitial" ) {
			huefillinitial.set(adjust(huefillinitial.get(),amount,0.0,360.0));
		} else if ( nm == "huefilltime" ) {
			huefilltime.set(adjust(huefilltime.get(),amount,0.01,10.0));
		} else if ( nm == "huefinal" ) {
			huefinal.set(adjust(huefinal.get(),amount,0.0,360.0));
		} else if ( nm == "hueinitial" ) {
			hueinitial.set(adjust(hueinitial.get(),amount,0.0,360.0));
		} else if ( nm == "huetime" ) {
			huetime.set(adjust(huetime.get(),amount,0.01,10.0));
		} else if ( nm == "lifetime" ) {
			lifetime.set(adjust(lifetime.get(),amount,0.01,10.0));
		} else if ( nm == "luminance" ) {
			luminance.set(adjust(luminance.get(),amount,0.0,1.0));
		} else if ( nm == "mass" ) {
			mass.set(adjust(mass.get(),amount,0.0,1.0));
		} else if ( nm == "mirror" ) {
			mirror.set(adjust(mirror.get(),amount,VizParams::StringVals["mirrorTypes"]));
		} else if ( nm == "movedir" ) {
			movedir.set(adjust(movedir.get(),amount,0.0,360.0));
		} else if ( nm == "movedirrandom" ) {
			movedirrandom.set(adjust(movedirrandom.get(),amount));
		} else if ( nm == "movefollowcursor" ) {
			movefollowcursor.set(adjust(movefollowcursor.get(),amount));
		} else if ( nm == "noisevertex" ) {
			noisevertex.set(adjust(noisevertex.get(),amount,0.0,1.0));
		} else if ( nm == "nsprites" ) {
			nsprites.set(adjust(nsprites.get(),amount,1,10000));
		} else if ( nm == "pitchmax" ) {
			pitchmax.set(adjust(pitchmax.get(),amount,0,127));
		} else if ( nm == "pitchmin" ) {
			pitchmin.set(adjust(pitchmin.get(),amount,0,127));
		} else if ( nm == "pitchwrap" ) {
			pitchwrap.set(adjust(pitchwrap.get(),amount));
		} else if ( nm == "placement" ) {
			placement.set(adjust(placement.get(),amount,VizParams::StringVals["placementTypes"]));
		} else if ( nm == "rotanginit" ) {
			rotanginit.set(adjust(rotanginit.get(),amount,0.0,360.0));
		} else if ( nm == "rotangspeed" ) {
			rotangspeed.set(adjust(rotangspeed.get(),amount,-360.0,360.0));
		} else if ( nm == "rotdirrandom" ) {
			rotdirrandom.set(adjust(rotdirrandom.get(),amount));
		} else if ( nm == "saturation" ) {
			saturation.set(adjust(saturation.get(),amount,0.0,1.0));
		} else if ( nm == "shape" ) {
			shape.set(adjust(shape.get(),amount,VizParams::StringVals["shapeTypes"]));
		} else if ( nm == "sizefinal" ) {
			sizefinal.set(adjust(sizefinal.get(),amount,0.0,10.0));
		} else if ( nm == "sizeinitial" ) {
			sizeinitial.set(adjust(sizeinitial.get(),amount,0.01,10.0));
		} else if ( nm == "sizetime" ) {
			sizetime.set(adjust(sizetime.get(),amount,0.01,10.0));
		} else if ( nm == "speedinitial" ) {
			speedinitial.set(adjust(speedinitial.get(),amount,0.0,1.0));
		} else if ( nm == "thickness" ) {
			thickness.set(adjust(thickness.get(),amount,0.01,10.0));
		} else if ( nm == "zable" ) {
			zable.set(adjust(zable.get(),amount));
		}

	}
	std::string DefaultValue(std::string nm) {
		if ( nm == "alphafinal" ) { return "0.0"; }
		if ( nm == "alphainitial" ) { return "1.0"; }
		if ( nm == "alphatime" ) { return "2.0"; }
		if ( nm == "aspect" ) { return "1.0"; }
		if ( nm == "bounce" ) { return "false"; }
		if ( nm == "filled" ) { return "true"; }
		if ( nm == "gravity" ) { return "false"; }
		if ( nm == "huefillfinal" ) { return "0.0"; }
		if ( nm == "huefillinitial" ) { return "360.0"; }
		if ( nm == "huefilltime" ) { return "2.0"; }
		if ( nm == "huefinal" ) { return "360.0"; }
		if ( nm == "hueinitial" ) { return "0.0"; }
		if ( nm == "huetime" ) { return "2.0"; }
		if ( nm == "lifetime" ) { return "3.0"; }
		if ( nm == "luminance" ) { return "0.5"; }
		if ( nm == "mass" ) { return "0.5"; }
		if ( nm == "mirror" ) { return "none"; }
		if ( nm == "movedir" ) { return "0.0"; }
		if ( nm == "movedirrandom" ) { return "false"; }
		if ( nm == "movefollowcursor" ) { return "false"; }
		if ( nm == "noisevertex" ) { return "0.0"; }
		if ( nm == "nsprites" ) { return "1000"; }
		if ( nm == "pitchmax" ) { return "90"; }
		if ( nm == "pitchmin" ) { return "40"; }
		if ( nm == "pitchwrap" ) { return "true"; }
		if ( nm == "placement" ) { return "center"; }
		if ( nm == "rotanginit" ) { return "0.0"; }
		if ( nm == "rotangspeed" ) { return "0.0"; }
		if ( nm == "rotdirrandom" ) { return "false"; }
		if ( nm == "saturation" ) { return "1.0"; }
		if ( nm == "shape" ) { return "circle"; }
		if ( nm == "sizefinal" ) { return "0.5"; }
		if ( nm == "sizeinitial" ) { return "0.2"; }
		if ( nm == "sizetime" ) { return "2.0"; }
		if ( nm == "speedinitial" ) { return "0.0"; }
		if ( nm == "thickness" ) { return "3.0"; }
		if ( nm == "zable" ) { return "true"; }
		return "";
	}
	std::string MinValue(std::string nm) {
		if ( nm == "alphafinal" ) { return "0.0"; }
		if ( nm == "alphainitial" ) { return "0.0"; }
		if ( nm == "alphatime" ) { return "0.01"; }
		if ( nm == "aspect" ) { return "1.0"; }
		if ( nm == "bounce" ) { return "false"; }
		if ( nm == "filled" ) { return "false"; }
		if ( nm == "gravity" ) { return "false"; }
		if ( nm == "huefillfinal" ) { return "0.0"; }
		if ( nm == "huefillinitial" ) { return "0.0"; }
		if ( nm == "huefilltime" ) { return "0.01"; }
		if ( nm == "huefinal" ) { return "0.0"; }
		if ( nm == "hueinitial" ) { return "0.0"; }
		if ( nm == "huetime" ) { return "0.01"; }
		if ( nm == "lifetime" ) { return "0.01"; }
		if ( nm == "luminance" ) { return "0.0"; }
		if ( nm == "mass" ) { return "0.0"; }
		if ( nm == "mirror" ) { return "mirrorTypes"; }
		if ( nm == "movedir" ) { return "0.0"; }
		if ( nm == "movedirrandom" ) { return "false"; }
		if ( nm == "movefollowcursor" ) { return "false"; }
		if ( nm == "noisevertex" ) { return "0.0"; }
		if ( nm == "nsprites" ) { return "1"; }
		if ( nm == "pitchmax" ) { return "0"; }
		if ( nm == "pitchmin" ) { return "0"; }
		if ( nm == "pitchwrap" ) { return "false"; }
		if ( nm == "placement" ) { return "placementTypes"; }
		if ( nm == "rotanginit" ) { return "0.0"; }
		if ( nm == "rotangspeed" ) { return "-360.0"; }
		if ( nm == "rotdirrandom" ) { return "false"; }
		if ( nm == "saturation" ) { return "0.0"; }
		if ( nm == "shape" ) { return "shapeTypes"; }
		if ( nm == "sizefinal" ) { return "0.0"; }
		if ( nm == "sizeinitial" ) { return "0.01"; }
		if ( nm == "sizetime" ) { return "0.01"; }
		if ( nm == "speedinitial" ) { return "0.0"; }
		if ( nm == "thickness" ) { return "0.01"; }
		if ( nm == "zable" ) { return "false"; }
		return "";
	}
	std::string MaxValue(std::string nm) {
		if ( nm == "alphafinal" ) { return "1.0"; }
		if ( nm == "alphainitial" ) { return "1.0"; }
		if ( nm == "alphatime" ) { return "10.0"; }
		if ( nm == "aspect" ) { return "4.0"; }
		if ( nm == "bounce" ) { return "true"; }
		if ( nm == "filled" ) { return "true"; }
		if ( nm == "gravity" ) { return "true"; }
		if ( nm == "huefillfinal" ) { return "360.0"; }
		if ( nm == "huefillinitial" ) { return "360.0"; }
		if ( nm == "huefilltime" ) { return "10.0"; }
		if ( nm == "huefinal" ) { return "360.0"; }
		if ( nm == "hueinitial" ) { return "360.0"; }
		if ( nm == "huetime" ) { return "10.0"; }
		if ( nm == "lifetime" ) { return "10.0"; }
		if ( nm == "luminance" ) { return "1.0"; }
		if ( nm == "mass" ) { return "1.0"; }
		if ( nm == "mirror" ) { return "mirrorTypes"; }
		if ( nm == "movedir" ) { return "360.0"; }
		if ( nm == "movedirrandom" ) { return "true"; }
		if ( nm == "movefollowcursor" ) { return "true"; }
		if ( nm == "noisevertex" ) { return "1.0"; }
		if ( nm == "nsprites" ) { return "10000"; }
		if ( nm == "pitchmax" ) { return "127"; }
		if ( nm == "pitchmin" ) { return "127"; }
		if ( nm == "pitchwrap" ) { return "true"; }
		if ( nm == "placement" ) { return "placementTypes"; }
		if ( nm == "rotanginit" ) { return "360.0"; }
		if ( nm == "rotangspeed" ) { return "360.0"; }
		if ( nm == "rotdirrandom" ) { return "true"; }
		if ( nm == "saturation" ) { return "1.0"; }
		if ( nm == "shape" ) { return "shapeTypes"; }
		if ( nm == "sizefinal" ) { return "10.0"; }
		if ( nm == "sizeinitial" ) { return "10.0"; }
		if ( nm == "sizetime" ) { return "10.0"; }
		if ( nm == "speedinitial" ) { return "1.0"; }
		if ( nm == "thickness" ) { return "10.0"; }
		if ( nm == "zable" ) { return "true"; }
		return "";
	}
	void Toggle(std::string nm) {
		bool stringval = false;
		if ( nm == "bounce" ) {
			bounce.set( ! bounce.get());
		}
		else if ( nm == "filled" ) {
			filled.set( ! filled.get());
		}
		else if ( nm == "gravity" ) {
			gravity.set( ! gravity.get());
		}
		else if ( nm == "movedirrandom" ) {
			movedirrandom.set( ! movedirrandom.get());
		}
		else if ( nm == "movefollowcursor" ) {
			movefollowcursor.set( ! movefollowcursor.get());
		}
		else if ( nm == "pitchwrap" ) {
			pitchwrap.set( ! pitchwrap.get());
		}
		else if ( nm == "rotdirrandom" ) {
			rotdirrandom.set( ! rotdirrandom.get());
		}
		else if ( nm == "zable" ) {
			zable.set( ! zable.get());
		}
	}
	std::string GetAsString(std::string nm) {
		if ( nm == "alphafinal" ) {
			return DoubleString(alphafinal.get());
		} else if ( nm == "alphainitial" ) {
			return DoubleString(alphainitial.get());
		} else if ( nm == "alphatime" ) {
			return DoubleString(alphatime.get());
		} else if ( nm == "aspect" ) {
			return DoubleString(aspect.get());
		} else if ( nm == "bounce" ) {
			return BoolString(bounce.get());
		} else if ( nm == "filled" ) {
			return BoolString(filled.get());
		} else if ( nm == "gravity" ) {
			return BoolString(gravity.get());
		} else if ( nm == "huefillfinal" ) {
			return DoubleString(huefillfinal.get());
		} else if ( nm == "huefillinitial" ) {
			return DoubleString(huefillinitial.get());
		} else if ( nm == "huefilltime" ) {
			return DoubleString(huefilltime.get());
		} else if ( nm == "huefinal" ) {
			return DoubleString(huefinal.get());
		} else if ( nm == "hueinitial" ) {
			return DoubleString(hueinitial.get());
		} else if ( nm == "huetime" ) {
			return DoubleString(huetime.get());
		} else if ( nm == "lifetime" ) {
			return DoubleString(lifetime.get());
		} else if ( nm == "luminance" ) {
			return DoubleString(luminance.get());
		} else if ( nm == "mass" ) {
			return DoubleString(mass.get());
		} else if ( nm == "mirror" ) {
			return mirror.get();
		} else if ( nm == "movedir" ) {
			return DoubleString(movedir.get());
		} else if ( nm == "movedirrandom" ) {
			return BoolString(movedirrandom.get());
		} else if ( nm == "movefollowcursor" ) {
			return BoolString(movefollowcursor.get());
		} else if ( nm == "noisevertex" ) {
			return DoubleString(noisevertex.get());
		} else if ( nm == "nsprites" ) {
			return IntString(nsprites.get());
		} else if ( nm == "pitchmax" ) {
			return IntString(pitchmax.get());
		} else if ( nm == "pitchmin" ) {
			return IntString(pitchmin.get());
		} else if ( nm == "pitchwrap" ) {
			return BoolString(pitchwrap.get());
		} else if ( nm == "placement" ) {
			return placement.get();
		} else if ( nm == "rotanginit" ) {
			return DoubleString(rotanginit.get());
		} else if ( nm == "rotangspeed" ) {
			return DoubleString(rotangspeed.get());
		} else if ( nm == "rotdirrandom" ) {
			return BoolString(rotdirrandom.get());
		} else if ( nm == "saturation" ) {
			return DoubleString(saturation.get());
		} else if ( nm == "shape" ) {
			return shape.get();
		} else if ( nm == "sizefinal" ) {
			return DoubleString(sizefinal.get());
		} else if ( nm == "sizeinitial" ) {
			return DoubleString(sizeinitial.get());
		} else if ( nm == "sizetime" ) {
			return DoubleString(sizetime.get());
		} else if ( nm == "speedinitial" ) {
			return DoubleString(speedinitial.get());
		} else if ( nm == "thickness" ) {
			return DoubleString(thickness.get());
		} else if ( nm == "zable" ) {
			return BoolString(zable.get());
		}
		return "";
	}
	std::string GetType(std::string nm) {
		if ( nm == "alphafinal" ) { return "double"; }
		if ( nm == "alphainitial" ) { return "double"; }
		if ( nm == "alphatime" ) { return "double"; }
		if ( nm == "aspect" ) { return "double"; }
		if ( nm == "bounce" ) { return "bool"; }
		if ( nm == "filled" ) { return "bool"; }
		if ( nm == "gravity" ) { return "bool"; }
		if ( nm == "huefillfinal" ) { return "double"; }
		if ( nm == "huefillinitial" ) { return "double"; }
		if ( nm == "huefilltime" ) { return "double"; }
		if ( nm == "huefinal" ) { return "double"; }
		if ( nm == "hueinitial" ) { return "double"; }
		if ( nm == "huetime" ) { return "double"; }
		if ( nm == "lifetime" ) { return "double"; }
		if ( nm == "luminance" ) { return "double"; }
		if ( nm == "mass" ) { return "double"; }
		if ( nm == "mirror" ) { return "string"; }
		if ( nm == "movedir" ) { return "double"; }
		if ( nm == "movedirrandom" ) { return "bool"; }
		if ( nm == "movefollowcursor" ) { return "bool"; }
		if ( nm == "noisevertex" ) { return "double"; }
		if ( nm == "nsprites" ) { return "int"; }
		if ( nm == "pitchmax" ) { return "int"; }
		if ( nm == "pitchmin" ) { return "int"; }
		if ( nm == "pitchwrap" ) { return "bool"; }
		if ( nm == "placement" ) { return "string"; }
		if ( nm == "rotanginit" ) { return "double"; }
		if ( nm == "rotangspeed" ) { return "double"; }
		if ( nm == "rotdirrandom" ) { return "bool"; }
		if ( nm == "saturation" ) { return "double"; }
		if ( nm == "shape" ) { return "string"; }
		if ( nm == "sizefinal" ) { return "double"; }
		if ( nm == "sizeinitial" ) { return "double"; }
		if ( nm == "sizetime" ) { return "double"; }
		if ( nm == "speedinitial" ) { return "double"; }
		if ( nm == "thickness" ) { return "double"; }
		if ( nm == "zable" ) { return "bool"; }
		return "";
	}

	DoubleParam alphafinal;
	DoubleParam alphainitial;
	DoubleParam alphatime;
	DoubleParam aspect;
	BoolParam bounce;
	BoolParam filled;
	BoolParam gravity;
	DoubleParam huefillfinal;
	DoubleParam huefillinitial;
	DoubleParam huefilltime;
	DoubleParam huefinal;
	DoubleParam hueinitial;
	DoubleParam huetime;
	DoubleParam lifetime;
	DoubleParam luminance;
	DoubleParam mass;
	StringParam mirror;
	DoubleParam movedir;
	BoolParam movedirrandom;
	BoolParam movefollowcursor;
	DoubleParam noisevertex;
	IntParam nsprites;
	IntParam pitchmax;
	IntParam pitchmin;
	BoolParam pitchwrap;
	StringParam placement;
	DoubleParam rotanginit;
	DoubleParam rotangspeed;
	BoolParam rotdirrandom;
	DoubleParam saturation;
	StringParam shape;
	DoubleParam sizefinal;
	DoubleParam sizeinitial;
	DoubleParam sizetime;
	DoubleParam speedinitial;
	DoubleParam thickness;
	BoolParam zable;
};

#endif
