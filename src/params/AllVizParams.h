#ifndef _ALLVIZPARAMS_H
#define _ALLVIZPARAMS_H
#include "VizParams.h"
#include "NosuchJSON.h"

extern char* AllVizParamsNames[];

#define AllVizParamsNames_INIT \
	"alphafinal",\
	"alphainitial",\
	"alphatime",\
	"arpeggio",\
	"aspect",\
	"bounce",\
	"controllerchan",\
	"controllerstyle",\
	"controllerzmax",\
	"controllerzmin",\
	"doquantize",\
	"filled",\
	"fullrange",\
	"gravity",\
	"huefillfinal",\
	"huefillinitial",\
	"huefilltime",\
	"huefinal",\
	"hueinitial",\
	"huetime",\
	"hvpos",\
	"lifetime",\
	"luminance",\
	"mass",\
	"minmove",\
	"minmovedepth",\
	"mirror",\
	"movedir",\
	"movedirrandom",\
	"movefollowcursor",\
	"noisevertex",\
	"noteonlogic",\
	"nsprites",\
	"pitchfactor",\
	"pitchmax",\
	"pitchmin",\
	"pitchoffset",\
	"quantfactor",\
	"quantfixed",\
	"rotanginit",\
	"rotangspeed",\
	"rotauto",\
	"rotdirrandom",\
	"saturation",\
	"shape",\
	"sizefinal",\
	"sizeinitial",\
	"sizetime",\
	"sound",\
	"speedinitial",\
	"thickness",\
	"timefret1q",\
	"timefret1y",\
	"timefret2q",\
	"timefret2y",\
	"timefret3q",\
	"timefret3y",\
	"timefret4q",\
	"timefret4y",\
	"tonicchange",\
	"xcontroller",\
	"ycontroller",\
	"zable",\
	"zcontroller",\
	NULL

class AllVizParams : public VizParams {
public:
	AllVizParams(bool defaults) {
		if ( defaults ) {
			loadDefaults();
		} else {
			// Otherwise, all of the parameters are unset
		}
	}
	std::string JsonListOfValues() { return _JsonListOfValues(AllVizParamsNames); }
	std::string JsonListOfParams() { return _JsonListOfParams(AllVizParamsNames); }
	void loadJson(cJSON* json) {
		cJSON* j;
		j = cJSON_GetObjectItem(json,"alphafinal");
		if (j) { alphafinal.set(j->valuedouble); }
		j = cJSON_GetObjectItem(json,"alphainitial");
		if (j) { alphainitial.set(j->valuedouble); }
		j = cJSON_GetObjectItem(json,"alphatime");
		if (j) { alphatime.set(j->valuedouble); }
		j = cJSON_GetObjectItem(json,"arpeggio");
		if (j) { arpeggio.set(j->valueint!=0); }
		j = cJSON_GetObjectItem(json,"aspect");
		if (j) { aspect.set(j->valuedouble); }
		j = cJSON_GetObjectItem(json,"bounce");
		if (j) { bounce.set(j->valueint!=0); }
		j = cJSON_GetObjectItem(json,"controllerchan");
		if (j) { controllerchan.set(j->valueint); }
		j = cJSON_GetObjectItem(json,"controllerstyle");
		if (j) { controllerstyle.set(j->valuestring); }
		j = cJSON_GetObjectItem(json,"controllerzmax");
		if (j) { controllerzmax.set(j->valuedouble); }
		j = cJSON_GetObjectItem(json,"controllerzmin");
		if (j) { controllerzmin.set(j->valuedouble); }
		j = cJSON_GetObjectItem(json,"doquantize");
		if (j) { doquantize.set(j->valueint!=0); }
		j = cJSON_GetObjectItem(json,"filled");
		if (j) { filled.set(j->valueint!=0); }
		j = cJSON_GetObjectItem(json,"fullrange");
		if (j) { fullrange.set(j->valueint!=0); }
		j = cJSON_GetObjectItem(json,"gravity");
		if (j) { gravity.set(j->valueint!=0); }
		j = cJSON_GetObjectItem(json,"huefillfinal");
		if (j) { huefillfinal.set(j->valuedouble); }
		j = cJSON_GetObjectItem(json,"huefillinitial");
		if (j) { huefillinitial.set(j->valuedouble); }
		j = cJSON_GetObjectItem(json,"huefilltime");
		if (j) { huefilltime.set(j->valuedouble); }
		j = cJSON_GetObjectItem(json,"huefinal");
		if (j) { huefinal.set(j->valuedouble); }
		j = cJSON_GetObjectItem(json,"hueinitial");
		if (j) { hueinitial.set(j->valuedouble); }
		j = cJSON_GetObjectItem(json,"huetime");
		if (j) { huetime.set(j->valuedouble); }
		j = cJSON_GetObjectItem(json,"hvpos");
		if (j) { hvpos.set(j->valuedouble); }
		j = cJSON_GetObjectItem(json,"lifetime");
		if (j) { lifetime.set(j->valuedouble); }
		j = cJSON_GetObjectItem(json,"luminance");
		if (j) { luminance.set(j->valuedouble); }
		j = cJSON_GetObjectItem(json,"mass");
		if (j) { mass.set(j->valuedouble); }
		j = cJSON_GetObjectItem(json,"minmove");
		if (j) { minmove.set(j->valuedouble); }
		j = cJSON_GetObjectItem(json,"minmovedepth");
		if (j) { minmovedepth.set(j->valuedouble); }
		j = cJSON_GetObjectItem(json,"mirror");
		if (j) { mirror.set(j->valuestring); }
		j = cJSON_GetObjectItem(json,"movedir");
		if (j) { movedir.set(j->valuedouble); }
		j = cJSON_GetObjectItem(json,"movedirrandom");
		if (j) { movedirrandom.set(j->valueint!=0); }
		j = cJSON_GetObjectItem(json,"movefollowcursor");
		if (j) { movefollowcursor.set(j->valueint!=0); }
		j = cJSON_GetObjectItem(json,"noisevertex");
		if (j) { noisevertex.set(j->valuedouble); }
		j = cJSON_GetObjectItem(json,"noteonlogic");
		if (j) { noteonlogic.set(j->valuestring); }
		j = cJSON_GetObjectItem(json,"nsprites");
		if (j) { nsprites.set(j->valueint); }
		j = cJSON_GetObjectItem(json,"pitchfactor");
		if (j) { pitchfactor.set(j->valuedouble); }
		j = cJSON_GetObjectItem(json,"pitchmax");
		if (j) { pitchmax.set(j->valueint); }
		j = cJSON_GetObjectItem(json,"pitchmin");
		if (j) { pitchmin.set(j->valueint); }
		j = cJSON_GetObjectItem(json,"pitchoffset");
		if (j) { pitchoffset.set(j->valueint); }
		j = cJSON_GetObjectItem(json,"quantfactor");
		if (j) { quantfactor.set(j->valuedouble); }
		j = cJSON_GetObjectItem(json,"quantfixed");
		if (j) { quantfixed.set(j->valueint!=0); }
		j = cJSON_GetObjectItem(json,"rotanginit");
		if (j) { rotanginit.set(j->valuedouble); }
		j = cJSON_GetObjectItem(json,"rotangspeed");
		if (j) { rotangspeed.set(j->valuedouble); }
		j = cJSON_GetObjectItem(json,"rotauto");
		if (j) { rotauto.set(j->valueint!=0); }
		j = cJSON_GetObjectItem(json,"rotdirrandom");
		if (j) { rotdirrandom.set(j->valueint!=0); }
		j = cJSON_GetObjectItem(json,"saturation");
		if (j) { saturation.set(j->valuedouble); }
		j = cJSON_GetObjectItem(json,"shape");
		if (j) { shape.set(j->valuestring); }
		j = cJSON_GetObjectItem(json,"sizefinal");
		if (j) { sizefinal.set(j->valuedouble); }
		j = cJSON_GetObjectItem(json,"sizeinitial");
		if (j) { sizeinitial.set(j->valuedouble); }
		j = cJSON_GetObjectItem(json,"sizetime");
		if (j) { sizetime.set(j->valuedouble); }
		j = cJSON_GetObjectItem(json,"sound");
		if (j) { sound.set(j->valuestring); }
		j = cJSON_GetObjectItem(json,"speedinitial");
		if (j) { speedinitial.set(j->valuedouble); }
		j = cJSON_GetObjectItem(json,"thickness");
		if (j) { thickness.set(j->valuedouble); }
		j = cJSON_GetObjectItem(json,"timefret1q");
		if (j) { timefret1q.set(j->valuedouble); }
		j = cJSON_GetObjectItem(json,"timefret1y");
		if (j) { timefret1y.set(j->valuedouble); }
		j = cJSON_GetObjectItem(json,"timefret2q");
		if (j) { timefret2q.set(j->valuedouble); }
		j = cJSON_GetObjectItem(json,"timefret2y");
		if (j) { timefret2y.set(j->valuedouble); }
		j = cJSON_GetObjectItem(json,"timefret3q");
		if (j) { timefret3q.set(j->valuedouble); }
		j = cJSON_GetObjectItem(json,"timefret3y");
		if (j) { timefret3y.set(j->valuedouble); }
		j = cJSON_GetObjectItem(json,"timefret4q");
		if (j) { timefret4q.set(j->valuedouble); }
		j = cJSON_GetObjectItem(json,"timefret4y");
		if (j) { timefret4y.set(j->valuedouble); }
		j = cJSON_GetObjectItem(json,"tonicchange");
		if (j) { tonicchange.set(j->valueint); }
		j = cJSON_GetObjectItem(json,"xcontroller");
		if (j) { xcontroller.set(j->valueint); }
		j = cJSON_GetObjectItem(json,"ycontroller");
		if (j) { ycontroller.set(j->valueint); }
		j = cJSON_GetObjectItem(json,"zable");
		if (j) { zable.set(j->valueint!=0); }
		j = cJSON_GetObjectItem(json,"zcontroller");
		if (j) { zcontroller.set(j->valueint); }
	}
	void loadDefaults() {
		alphafinal.set(0.0);
		alphainitial.set(0.2);
		alphatime.set(2.0);
		arpeggio.set(false);
		aspect.set(1.0);
		bounce.set(false);
		controllerchan.set(1);
		controllerstyle.set("modulationonly");
		controllerzmax.set(0.3);
		controllerzmin.set(0.05);
		doquantize.set(true);
		filled.set(true);
		fullrange.set(false);
		gravity.set(false);
		huefillfinal.set(0.0);
		huefillinitial.set(360.0);
		huefilltime.set(5.0);
		huefinal.set(360.0);
		hueinitial.set(0.0);
		huetime.set(5.0);
		hvpos.set(0.5);
		lifetime.set(5.0);
		luminance.set(0.5);
		mass.set(0.5);
		minmove.set(0.025);
		minmovedepth.set(0.025);
		mirror.set("none");
		movedir.set(0.0);
		movedirrandom.set(false);
		movefollowcursor.set(false);
		noisevertex.set(0.0);
		noteonlogic.set("none");
		nsprites.set(1000);
		pitchfactor.set(1.0);
		pitchmax.set(80);
		pitchmin.set(33);
		pitchoffset.set(33);
		quantfactor.set(1.0);
		quantfixed.set(false);
		rotanginit.set(0.0);
		rotangspeed.set(0.0);
		rotauto.set(true);
		rotdirrandom.set(false);
		saturation.set(1.0);
		shape.set("circle");
		sizefinal.set(0.0);
		sizeinitial.set(1.0);
		sizetime.set(1.0);
		sound.set("Dummy");
		speedinitial.set(0.0);
		thickness.set(3.0);
		timefret1q.set(1.0);
		timefret1y.set(0.3);
		timefret2q.set(0.5);
		timefret2y.set(0.60);
		timefret3q.set(0.25);
		timefret3y.set(0.95);
		timefret4q.set(0.125);
		timefret4y.set(1.0);
		tonicchange.set(30*1000);
		xcontroller.set(3);
		ycontroller.set(2);
		zable.set(true);
		zcontroller.set(1);
	}
	void applyVizParamsFrom(AllVizParams* p) {
		if ( ! p ) { return; }
		if ( p->alphafinal.isset() ) { this->alphafinal.set(p->alphafinal.get()); }
		if ( p->alphainitial.isset() ) { this->alphainitial.set(p->alphainitial.get()); }
		if ( p->alphatime.isset() ) { this->alphatime.set(p->alphatime.get()); }
		if ( p->arpeggio.isset() ) { this->arpeggio.set(p->arpeggio.get()); }
		if ( p->aspect.isset() ) { this->aspect.set(p->aspect.get()); }
		if ( p->bounce.isset() ) { this->bounce.set(p->bounce.get()); }
		if ( p->controllerchan.isset() ) { this->controllerchan.set(p->controllerchan.get()); }
		if ( p->controllerstyle.isset() ) { this->controllerstyle.set(p->controllerstyle.get()); }
		if ( p->controllerzmax.isset() ) { this->controllerzmax.set(p->controllerzmax.get()); }
		if ( p->controllerzmin.isset() ) { this->controllerzmin.set(p->controllerzmin.get()); }
		if ( p->doquantize.isset() ) { this->doquantize.set(p->doquantize.get()); }
		if ( p->filled.isset() ) { this->filled.set(p->filled.get()); }
		if ( p->fullrange.isset() ) { this->fullrange.set(p->fullrange.get()); }
		if ( p->gravity.isset() ) { this->gravity.set(p->gravity.get()); }
		if ( p->huefillfinal.isset() ) { this->huefillfinal.set(p->huefillfinal.get()); }
		if ( p->huefillinitial.isset() ) { this->huefillinitial.set(p->huefillinitial.get()); }
		if ( p->huefilltime.isset() ) { this->huefilltime.set(p->huefilltime.get()); }
		if ( p->huefinal.isset() ) { this->huefinal.set(p->huefinal.get()); }
		if ( p->hueinitial.isset() ) { this->hueinitial.set(p->hueinitial.get()); }
		if ( p->huetime.isset() ) { this->huetime.set(p->huetime.get()); }
		if ( p->hvpos.isset() ) { this->hvpos.set(p->hvpos.get()); }
		if ( p->lifetime.isset() ) { this->lifetime.set(p->lifetime.get()); }
		if ( p->luminance.isset() ) { this->luminance.set(p->luminance.get()); }
		if ( p->mass.isset() ) { this->mass.set(p->mass.get()); }
		if ( p->minmove.isset() ) { this->minmove.set(p->minmove.get()); }
		if ( p->minmovedepth.isset() ) { this->minmovedepth.set(p->minmovedepth.get()); }
		if ( p->mirror.isset() ) { this->mirror.set(p->mirror.get()); }
		if ( p->movedir.isset() ) { this->movedir.set(p->movedir.get()); }
		if ( p->movedirrandom.isset() ) { this->movedirrandom.set(p->movedirrandom.get()); }
		if ( p->movefollowcursor.isset() ) { this->movefollowcursor.set(p->movefollowcursor.get()); }
		if ( p->noisevertex.isset() ) { this->noisevertex.set(p->noisevertex.get()); }
		if ( p->noteonlogic.isset() ) { this->noteonlogic.set(p->noteonlogic.get()); }
		if ( p->nsprites.isset() ) { this->nsprites.set(p->nsprites.get()); }
		if ( p->pitchfactor.isset() ) { this->pitchfactor.set(p->pitchfactor.get()); }
		if ( p->pitchmax.isset() ) { this->pitchmax.set(p->pitchmax.get()); }
		if ( p->pitchmin.isset() ) { this->pitchmin.set(p->pitchmin.get()); }
		if ( p->pitchoffset.isset() ) { this->pitchoffset.set(p->pitchoffset.get()); }
		if ( p->quantfactor.isset() ) { this->quantfactor.set(p->quantfactor.get()); }
		if ( p->quantfixed.isset() ) { this->quantfixed.set(p->quantfixed.get()); }
		if ( p->rotanginit.isset() ) { this->rotanginit.set(p->rotanginit.get()); }
		if ( p->rotangspeed.isset() ) { this->rotangspeed.set(p->rotangspeed.get()); }
		if ( p->rotauto.isset() ) { this->rotauto.set(p->rotauto.get()); }
		if ( p->rotdirrandom.isset() ) { this->rotdirrandom.set(p->rotdirrandom.get()); }
		if ( p->saturation.isset() ) { this->saturation.set(p->saturation.get()); }
		if ( p->shape.isset() ) { this->shape.set(p->shape.get()); }
		if ( p->sizefinal.isset() ) { this->sizefinal.set(p->sizefinal.get()); }
		if ( p->sizeinitial.isset() ) { this->sizeinitial.set(p->sizeinitial.get()); }
		if ( p->sizetime.isset() ) { this->sizetime.set(p->sizetime.get()); }
		if ( p->sound.isset() ) { this->sound.set(p->sound.get()); }
		if ( p->speedinitial.isset() ) { this->speedinitial.set(p->speedinitial.get()); }
		if ( p->thickness.isset() ) { this->thickness.set(p->thickness.get()); }
		if ( p->timefret1q.isset() ) { this->timefret1q.set(p->timefret1q.get()); }
		if ( p->timefret1y.isset() ) { this->timefret1y.set(p->timefret1y.get()); }
		if ( p->timefret2q.isset() ) { this->timefret2q.set(p->timefret2q.get()); }
		if ( p->timefret2y.isset() ) { this->timefret2y.set(p->timefret2y.get()); }
		if ( p->timefret3q.isset() ) { this->timefret3q.set(p->timefret3q.get()); }
		if ( p->timefret3y.isset() ) { this->timefret3y.set(p->timefret3y.get()); }
		if ( p->timefret4q.isset() ) { this->timefret4q.set(p->timefret4q.get()); }
		if ( p->timefret4y.isset() ) { this->timefret4y.set(p->timefret4y.get()); }
		if ( p->tonicchange.isset() ) { this->tonicchange.set(p->tonicchange.get()); }
		if ( p->xcontroller.isset() ) { this->xcontroller.set(p->xcontroller.get()); }
		if ( p->ycontroller.isset() ) { this->ycontroller.set(p->ycontroller.get()); }
		if ( p->zable.isset() ) { this->zable.set(p->zable.get()); }
		if ( p->zcontroller.isset() ) { this->zcontroller.set(p->zcontroller.get()); }
	}
	void Set(std::string nm, std::string val) {
		bool stringval = false;
		if ( nm == "alphafinal" ) {
			alphafinal.set(string2double(val));
		} else if ( nm == "alphainitial" ) {
			alphainitial.set(string2double(val));
		} else if ( nm == "alphatime" ) {
			alphatime.set(string2double(val));
		} else if ( nm == "arpeggio" ) {
			arpeggio.set(string2bool(val));
		} else if ( nm == "aspect" ) {
			aspect.set(string2double(val));
		} else if ( nm == "bounce" ) {
			bounce.set(string2bool(val));
		} else if ( nm == "controllerchan" ) {
			controllerchan.set(string2int(val));
		} else if ( nm == "controllerstyle" ) {
			controllerstyle.set(val);
			stringval = true;
		} else if ( nm == "controllerzmax" ) {
			controllerzmax.set(string2double(val));
		} else if ( nm == "controllerzmin" ) {
			controllerzmin.set(string2double(val));
		} else if ( nm == "doquantize" ) {
			doquantize.set(string2bool(val));
		} else if ( nm == "filled" ) {
			filled.set(string2bool(val));
		} else if ( nm == "fullrange" ) {
			fullrange.set(string2bool(val));
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
		} else if ( nm == "hvpos" ) {
			hvpos.set(string2double(val));
		} else if ( nm == "lifetime" ) {
			lifetime.set(string2double(val));
		} else if ( nm == "luminance" ) {
			luminance.set(string2double(val));
		} else if ( nm == "mass" ) {
			mass.set(string2double(val));
		} else if ( nm == "minmove" ) {
			minmove.set(string2double(val));
		} else if ( nm == "minmovedepth" ) {
			minmovedepth.set(string2double(val));
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
		} else if ( nm == "noteonlogic" ) {
			noteonlogic.set(val);
			stringval = true;
		} else if ( nm == "nsprites" ) {
			nsprites.set(string2int(val));
		} else if ( nm == "pitchfactor" ) {
			pitchfactor.set(string2double(val));
		} else if ( nm == "pitchmax" ) {
			pitchmax.set(string2int(val));
		} else if ( nm == "pitchmin" ) {
			pitchmin.set(string2int(val));
		} else if ( nm == "pitchoffset" ) {
			pitchoffset.set(string2int(val));
		} else if ( nm == "quantfactor" ) {
			quantfactor.set(string2double(val));
		} else if ( nm == "quantfixed" ) {
			quantfixed.set(string2bool(val));
		} else if ( nm == "rotanginit" ) {
			rotanginit.set(string2double(val));
		} else if ( nm == "rotangspeed" ) {
			rotangspeed.set(string2double(val));
		} else if ( nm == "rotauto" ) {
			rotauto.set(string2bool(val));
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
		} else if ( nm == "sound" ) {
			sound.set(val);
			stringval = true;
		} else if ( nm == "speedinitial" ) {
			speedinitial.set(string2double(val));
		} else if ( nm == "thickness" ) {
			thickness.set(string2double(val));
		} else if ( nm == "timefret1q" ) {
			timefret1q.set(string2double(val));
		} else if ( nm == "timefret1y" ) {
			timefret1y.set(string2double(val));
		} else if ( nm == "timefret2q" ) {
			timefret2q.set(string2double(val));
		} else if ( nm == "timefret2y" ) {
			timefret2y.set(string2double(val));
		} else if ( nm == "timefret3q" ) {
			timefret3q.set(string2double(val));
		} else if ( nm == "timefret3y" ) {
			timefret3y.set(string2double(val));
		} else if ( nm == "timefret4q" ) {
			timefret4q.set(string2double(val));
		} else if ( nm == "timefret4y" ) {
			timefret4y.set(string2double(val));
		} else if ( nm == "tonicchange" ) {
			tonicchange.set(string2int(val));
		} else if ( nm == "xcontroller" ) {
			xcontroller.set(string2int(val));
		} else if ( nm == "ycontroller" ) {
			ycontroller.set(string2int(val));
		} else if ( nm == "zable" ) {
			zable.set(string2bool(val));
		} else if ( nm == "zcontroller" ) {
			zcontroller.set(string2int(val));
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
		} else if ( nm == "arpeggio" ) {
			arpeggio.set(adjust(arpeggio.get(),amount));
		} else if ( nm == "aspect" ) {
			aspect.set(adjust(aspect.get(),amount,1.0,10.0));
		} else if ( nm == "bounce" ) {
			bounce.set(adjust(bounce.get(),amount));
		} else if ( nm == "controllerchan" ) {
			controllerchan.set(adjust(controllerchan.get(),amount,1,16));
		} else if ( nm == "controllerstyle" ) {
			controllerstyle.set(adjust(controllerstyle.get(),amount,VizParams::controllerTypes));
		} else if ( nm == "controllerzmax" ) {
			controllerzmax.set(adjust(controllerzmax.get(),amount,0.0,1.0));
		} else if ( nm == "controllerzmin" ) {
			controllerzmin.set(adjust(controllerzmin.get(),amount,0.0,1.0));
		} else if ( nm == "doquantize" ) {
			doquantize.set(adjust(doquantize.get(),amount));
		} else if ( nm == "filled" ) {
			filled.set(adjust(filled.get(),amount));
		} else if ( nm == "fullrange" ) {
			fullrange.set(adjust(fullrange.get(),amount));
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
		} else if ( nm == "hvpos" ) {
			hvpos.set(adjust(hvpos.get(),amount,0.0,1.0));
		} else if ( nm == "lifetime" ) {
			lifetime.set(adjust(lifetime.get(),amount,0.01,10.0));
		} else if ( nm == "luminance" ) {
			luminance.set(adjust(luminance.get(),amount,0.0,1.0));
		} else if ( nm == "mass" ) {
			mass.set(adjust(mass.get(),amount,0.0,1.0));
		} else if ( nm == "minmove" ) {
			minmove.set(adjust(minmove.get(),amount,0.0,0.3));
		} else if ( nm == "minmovedepth" ) {
			minmovedepth.set(adjust(minmovedepth.get(),amount,0.0,0.3));
		} else if ( nm == "mirror" ) {
			mirror.set(adjust(mirror.get(),amount,VizParams::mirrorTypes));
		} else if ( nm == "movedir" ) {
			movedir.set(adjust(movedir.get(),amount,0.0,360.0));
		} else if ( nm == "movedirrandom" ) {
			movedirrandom.set(adjust(movedirrandom.get(),amount));
		} else if ( nm == "movefollowcursor" ) {
			movefollowcursor.set(adjust(movefollowcursor.get(),amount));
		} else if ( nm == "noisevertex" ) {
			noisevertex.set(adjust(noisevertex.get(),amount,0.0,1.0));
		} else if ( nm == "noteonlogic" ) {
			noteonlogic.set(adjust(noteonlogic.get(),amount,VizParams::logicTypes));
		} else if ( nm == "nsprites" ) {
			nsprites.set(adjust(nsprites.get(),amount,1,10000));
		} else if ( nm == "pitchfactor" ) {
			pitchfactor.set(adjust(pitchfactor.get(),amount,0.1,10.0));
		} else if ( nm == "pitchmax" ) {
			pitchmax.set(adjust(pitchmax.get(),amount,0,127));
		} else if ( nm == "pitchmin" ) {
			pitchmin.set(adjust(pitchmin.get(),amount,0,127));
		} else if ( nm == "pitchoffset" ) {
			pitchoffset.set(adjust(pitchoffset.get(),amount,0,127));
		} else if ( nm == "quantfactor" ) {
			quantfactor.set(adjust(quantfactor.get(),amount,0.5,2.0));
		} else if ( nm == "quantfixed" ) {
			quantfixed.set(adjust(quantfixed.get(),amount));
		} else if ( nm == "rotanginit" ) {
			rotanginit.set(adjust(rotanginit.get(),amount,0.0,360.0));
		} else if ( nm == "rotangspeed" ) {
			rotangspeed.set(adjust(rotangspeed.get(),amount,-360.0,360.0));
		} else if ( nm == "rotauto" ) {
			rotauto.set(adjust(rotauto.get(),amount));
		} else if ( nm == "rotdirrandom" ) {
			rotdirrandom.set(adjust(rotdirrandom.get(),amount));
		} else if ( nm == "saturation" ) {
			saturation.set(adjust(saturation.get(),amount,0.0,1.0));
		} else if ( nm == "shape" ) {
			shape.set(adjust(shape.get(),amount,VizParams::shapeTypes));
		} else if ( nm == "sizefinal" ) {
			sizefinal.set(adjust(sizefinal.get(),amount,0.0,10.0));
		} else if ( nm == "sizeinitial" ) {
			sizeinitial.set(adjust(sizeinitial.get(),amount,0.01,10.0));
		} else if ( nm == "sizetime" ) {
			sizetime.set(adjust(sizetime.get(),amount,0.01,10.0));
		} else if ( nm == "sound" ) {
			// '*' means the value can be anything
		} else if ( nm == "speedinitial" ) {
			speedinitial.set(adjust(speedinitial.get(),amount,0.0,1.0));
		} else if ( nm == "thickness" ) {
			thickness.set(adjust(thickness.get(),amount,0.01,10.0));
		} else if ( nm == "timefret1q" ) {
			timefret1q.set(adjust(timefret1q.get(),amount,0.0,1.0));
		} else if ( nm == "timefret1y" ) {
			timefret1y.set(adjust(timefret1y.get(),amount,0.0,1.0));
		} else if ( nm == "timefret2q" ) {
			timefret2q.set(adjust(timefret2q.get(),amount,0.0,1.0));
		} else if ( nm == "timefret2y" ) {
			timefret2y.set(adjust(timefret2y.get(),amount,0.0,1.0));
		} else if ( nm == "timefret3q" ) {
			timefret3q.set(adjust(timefret3q.get(),amount,0.0,1.0));
		} else if ( nm == "timefret3y" ) {
			timefret3y.set(adjust(timefret3y.get(),amount,0.0,1.0));
		} else if ( nm == "timefret4q" ) {
			timefret4q.set(adjust(timefret4q.get(),amount,0.0,1.0));
		} else if ( nm == "timefret4y" ) {
			timefret4y.set(adjust(timefret4y.get(),amount,0.0,1.0));
		} else if ( nm == "tonicchange" ) {
			tonicchange.set(adjust(tonicchange.get(),amount,0,300*1000));
		} else if ( nm == "xcontroller" ) {
			xcontroller.set(adjust(xcontroller.get(),amount,1,127));
		} else if ( nm == "ycontroller" ) {
			ycontroller.set(adjust(ycontroller.get(),amount,1,127));
		} else if ( nm == "zable" ) {
			zable.set(adjust(zable.get(),amount));
		} else if ( nm == "zcontroller" ) {
			zcontroller.set(adjust(zcontroller.get(),amount,1,127));
		}

	}
	std::string DefaultValue(std::string nm) {
		if ( nm == "alphafinal" ) { return "0.0"; }
		if ( nm == "alphainitial" ) { return "0.2"; }
		if ( nm == "alphatime" ) { return "2.0"; }
		if ( nm == "arpeggio" ) { return "false"; }
		if ( nm == "aspect" ) { return "1.0"; }
		if ( nm == "bounce" ) { return "false"; }
		if ( nm == "controllerchan" ) { return "1"; }
		if ( nm == "controllerstyle" ) { return "modulationonly"; }
		if ( nm == "controllerzmax" ) { return "0.3"; }
		if ( nm == "controllerzmin" ) { return "0.05"; }
		if ( nm == "doquantize" ) { return "true"; }
		if ( nm == "filled" ) { return "true"; }
		if ( nm == "fullrange" ) { return "false"; }
		if ( nm == "gravity" ) { return "false"; }
		if ( nm == "huefillfinal" ) { return "0.0"; }
		if ( nm == "huefillinitial" ) { return "360.0"; }
		if ( nm == "huefilltime" ) { return "5.0"; }
		if ( nm == "huefinal" ) { return "360.0"; }
		if ( nm == "hueinitial" ) { return "0.0"; }
		if ( nm == "huetime" ) { return "5.0"; }
		if ( nm == "hvpos" ) { return "0.5"; }
		if ( nm == "lifetime" ) { return "5.0"; }
		if ( nm == "luminance" ) { return "0.5"; }
		if ( nm == "mass" ) { return "0.5"; }
		if ( nm == "minmove" ) { return "0.025"; }
		if ( nm == "minmovedepth" ) { return "0.025"; }
		if ( nm == "mirror" ) { return "none"; }
		if ( nm == "movedir" ) { return "0.0"; }
		if ( nm == "movedirrandom" ) { return "false"; }
		if ( nm == "movefollowcursor" ) { return "false"; }
		if ( nm == "noisevertex" ) { return "0.0"; }
		if ( nm == "noteonlogic" ) { return "none"; }
		if ( nm == "nsprites" ) { return "1000"; }
		if ( nm == "pitchfactor" ) { return "1.0"; }
		if ( nm == "pitchmax" ) { return "80"; }
		if ( nm == "pitchmin" ) { return "33"; }
		if ( nm == "pitchoffset" ) { return "33"; }
		if ( nm == "quantfactor" ) { return "1.0"; }
		if ( nm == "quantfixed" ) { return "false"; }
		if ( nm == "rotanginit" ) { return "0.0"; }
		if ( nm == "rotangspeed" ) { return "0.0"; }
		if ( nm == "rotauto" ) { return "true"; }
		if ( nm == "rotdirrandom" ) { return "false"; }
		if ( nm == "saturation" ) { return "1.0"; }
		if ( nm == "shape" ) { return "circle"; }
		if ( nm == "sizefinal" ) { return "0.0"; }
		if ( nm == "sizeinitial" ) { return "1.0"; }
		if ( nm == "sizetime" ) { return "1.0"; }
		if ( nm == "sound" ) { return "Dummy"; }
		if ( nm == "speedinitial" ) { return "0.0"; }
		if ( nm == "thickness" ) { return "3.0"; }
		if ( nm == "timefret1q" ) { return "1.0"; }
		if ( nm == "timefret1y" ) { return "0.3"; }
		if ( nm == "timefret2q" ) { return "0.5"; }
		if ( nm == "timefret2y" ) { return "0.60"; }
		if ( nm == "timefret3q" ) { return "0.25"; }
		if ( nm == "timefret3y" ) { return "0.95"; }
		if ( nm == "timefret4q" ) { return "0.125"; }
		if ( nm == "timefret4y" ) { return "1.0"; }
		if ( nm == "tonicchange" ) { return "30*1000"; }
		if ( nm == "xcontroller" ) { return "3"; }
		if ( nm == "ycontroller" ) { return "2"; }
		if ( nm == "zable" ) { return "true"; }
		if ( nm == "zcontroller" ) { return "1"; }
		return "";
	}
	std::string MinValue(std::string nm) {
		if ( nm == "alphafinal" ) { return "0.0"; }
		if ( nm == "alphainitial" ) { return "0.0"; }
		if ( nm == "alphatime" ) { return "0.01"; }
		if ( nm == "arpeggio" ) { return "false"; }
		if ( nm == "aspect" ) { return "1.0"; }
		if ( nm == "bounce" ) { return "false"; }
		if ( nm == "controllerchan" ) { return "1"; }
		if ( nm == "controllerstyle" ) { return "controllerTypes"; }
		if ( nm == "controllerzmax" ) { return "0.0"; }
		if ( nm == "controllerzmin" ) { return "0.0"; }
		if ( nm == "doquantize" ) { return "false"; }
		if ( nm == "filled" ) { return "false"; }
		if ( nm == "fullrange" ) { return "false"; }
		if ( nm == "gravity" ) { return "false"; }
		if ( nm == "huefillfinal" ) { return "0.0"; }
		if ( nm == "huefillinitial" ) { return "0.0"; }
		if ( nm == "huefilltime" ) { return "0.01"; }
		if ( nm == "huefinal" ) { return "0.0"; }
		if ( nm == "hueinitial" ) { return "0.0"; }
		if ( nm == "huetime" ) { return "0.01"; }
		if ( nm == "hvpos" ) { return "0.0"; }
		if ( nm == "lifetime" ) { return "0.01"; }
		if ( nm == "luminance" ) { return "0.0"; }
		if ( nm == "mass" ) { return "0.0"; }
		if ( nm == "minmove" ) { return "0.0"; }
		if ( nm == "minmovedepth" ) { return "0.0"; }
		if ( nm == "mirror" ) { return "mirrorTypes"; }
		if ( nm == "movedir" ) { return "0.0"; }
		if ( nm == "movedirrandom" ) { return "false"; }
		if ( nm == "movefollowcursor" ) { return "false"; }
		if ( nm == "noisevertex" ) { return "0.0"; }
		if ( nm == "noteonlogic" ) { return "logicTypes"; }
		if ( nm == "nsprites" ) { return "1"; }
		if ( nm == "pitchfactor" ) { return "0.1"; }
		if ( nm == "pitchmax" ) { return "0"; }
		if ( nm == "pitchmin" ) { return "0"; }
		if ( nm == "pitchoffset" ) { return "0"; }
		if ( nm == "quantfactor" ) { return "0.5"; }
		if ( nm == "quantfixed" ) { return "false"; }
		if ( nm == "rotanginit" ) { return "0.0"; }
		if ( nm == "rotangspeed" ) { return "-360.0"; }
		if ( nm == "rotauto" ) { return "false"; }
		if ( nm == "rotdirrandom" ) { return "false"; }
		if ( nm == "saturation" ) { return "0.0"; }
		if ( nm == "shape" ) { return "shapeTypes"; }
		if ( nm == "sizefinal" ) { return "0.0"; }
		if ( nm == "sizeinitial" ) { return "0.01"; }
		if ( nm == "sizetime" ) { return "0.01"; }
		if ( nm == "sound" ) { return "*"; }
		if ( nm == "speedinitial" ) { return "0.0"; }
		if ( nm == "thickness" ) { return "0.01"; }
		if ( nm == "timefret1q" ) { return "0.0"; }
		if ( nm == "timefret1y" ) { return "0.0"; }
		if ( nm == "timefret2q" ) { return "0.0"; }
		if ( nm == "timefret2y" ) { return "0.0"; }
		if ( nm == "timefret3q" ) { return "0.0"; }
		if ( nm == "timefret3y" ) { return "0.0"; }
		if ( nm == "timefret4q" ) { return "0.0"; }
		if ( nm == "timefret4y" ) { return "0.0"; }
		if ( nm == "tonicchange" ) { return "0"; }
		if ( nm == "xcontroller" ) { return "1"; }
		if ( nm == "ycontroller" ) { return "1"; }
		if ( nm == "zable" ) { return "false"; }
		if ( nm == "zcontroller" ) { return "1"; }
		return "";
	}
	std::string MaxValue(std::string nm) {
		if ( nm == "alphafinal" ) { return "1.0"; }
		if ( nm == "alphainitial" ) { return "1.0"; }
		if ( nm == "alphatime" ) { return "10.0"; }
		if ( nm == "arpeggio" ) { return "true"; }
		if ( nm == "aspect" ) { return "10.0"; }
		if ( nm == "bounce" ) { return "true"; }
		if ( nm == "controllerchan" ) { return "16"; }
		if ( nm == "controllerstyle" ) { return "controllerTypes"; }
		if ( nm == "controllerzmax" ) { return "1.0"; }
		if ( nm == "controllerzmin" ) { return "1.0"; }
		if ( nm == "doquantize" ) { return "true"; }
		if ( nm == "filled" ) { return "true"; }
		if ( nm == "fullrange" ) { return "true"; }
		if ( nm == "gravity" ) { return "true"; }
		if ( nm == "huefillfinal" ) { return "360.0"; }
		if ( nm == "huefillinitial" ) { return "360.0"; }
		if ( nm == "huefilltime" ) { return "10.0"; }
		if ( nm == "huefinal" ) { return "360.0"; }
		if ( nm == "hueinitial" ) { return "360.0"; }
		if ( nm == "huetime" ) { return "10.0"; }
		if ( nm == "hvpos" ) { return "1.0"; }
		if ( nm == "lifetime" ) { return "10.0"; }
		if ( nm == "luminance" ) { return "1.0"; }
		if ( nm == "mass" ) { return "1.0"; }
		if ( nm == "minmove" ) { return "0.3"; }
		if ( nm == "minmovedepth" ) { return "0.3"; }
		if ( nm == "mirror" ) { return "mirrorTypes"; }
		if ( nm == "movedir" ) { return "360.0"; }
		if ( nm == "movedirrandom" ) { return "true"; }
		if ( nm == "movefollowcursor" ) { return "true"; }
		if ( nm == "noisevertex" ) { return "1.0"; }
		if ( nm == "noteonlogic" ) { return "logicTypes"; }
		if ( nm == "nsprites" ) { return "10000"; }
		if ( nm == "pitchfactor" ) { return "10.0"; }
		if ( nm == "pitchmax" ) { return "127"; }
		if ( nm == "pitchmin" ) { return "127"; }
		if ( nm == "pitchoffset" ) { return "127"; }
		if ( nm == "quantfactor" ) { return "2.0"; }
		if ( nm == "quantfixed" ) { return "true"; }
		if ( nm == "rotanginit" ) { return "360.0"; }
		if ( nm == "rotangspeed" ) { return "360.0"; }
		if ( nm == "rotauto" ) { return "true"; }
		if ( nm == "rotdirrandom" ) { return "true"; }
		if ( nm == "saturation" ) { return "1.0"; }
		if ( nm == "shape" ) { return "shapeTypes"; }
		if ( nm == "sizefinal" ) { return "10.0"; }
		if ( nm == "sizeinitial" ) { return "10.0"; }
		if ( nm == "sizetime" ) { return "10.0"; }
		if ( nm == "sound" ) { return "*"; }
		if ( nm == "speedinitial" ) { return "1.0"; }
		if ( nm == "thickness" ) { return "10.0"; }
		if ( nm == "timefret1q" ) { return "1.0"; }
		if ( nm == "timefret1y" ) { return "1.0"; }
		if ( nm == "timefret2q" ) { return "1.0"; }
		if ( nm == "timefret2y" ) { return "1.0"; }
		if ( nm == "timefret3q" ) { return "1.0"; }
		if ( nm == "timefret3y" ) { return "1.0"; }
		if ( nm == "timefret4q" ) { return "1.0"; }
		if ( nm == "timefret4y" ) { return "1.0"; }
		if ( nm == "tonicchange" ) { return "300*1000"; }
		if ( nm == "xcontroller" ) { return "127"; }
		if ( nm == "ycontroller" ) { return "127"; }
		if ( nm == "zable" ) { return "true"; }
		if ( nm == "zcontroller" ) { return "127"; }
		return "";
	}
	void Toggle(std::string nm) {
		bool stringval = false;
		if ( nm == "arpeggio" ) {
			arpeggio.set( ! arpeggio.get());
		}
		else if ( nm == "bounce" ) {
			bounce.set( ! bounce.get());
		}
		else if ( nm == "doquantize" ) {
			doquantize.set( ! doquantize.get());
		}
		else if ( nm == "filled" ) {
			filled.set( ! filled.get());
		}
		else if ( nm == "fullrange" ) {
			fullrange.set( ! fullrange.get());
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
		else if ( nm == "quantfixed" ) {
			quantfixed.set( ! quantfixed.get());
		}
		else if ( nm == "rotauto" ) {
			rotauto.set( ! rotauto.get());
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
		} else if ( nm == "arpeggio" ) {
			return BoolString(arpeggio.get());
		} else if ( nm == "aspect" ) {
			return DoubleString(aspect.get());
		} else if ( nm == "bounce" ) {
			return BoolString(bounce.get());
		} else if ( nm == "controllerchan" ) {
			return IntString(controllerchan.get());
		} else if ( nm == "controllerstyle" ) {
			return controllerstyle.get();
		} else if ( nm == "controllerzmax" ) {
			return DoubleString(controllerzmax.get());
		} else if ( nm == "controllerzmin" ) {
			return DoubleString(controllerzmin.get());
		} else if ( nm == "doquantize" ) {
			return BoolString(doquantize.get());
		} else if ( nm == "filled" ) {
			return BoolString(filled.get());
		} else if ( nm == "fullrange" ) {
			return BoolString(fullrange.get());
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
		} else if ( nm == "hvpos" ) {
			return DoubleString(hvpos.get());
		} else if ( nm == "lifetime" ) {
			return DoubleString(lifetime.get());
		} else if ( nm == "luminance" ) {
			return DoubleString(luminance.get());
		} else if ( nm == "mass" ) {
			return DoubleString(mass.get());
		} else if ( nm == "minmove" ) {
			return DoubleString(minmove.get());
		} else if ( nm == "minmovedepth" ) {
			return DoubleString(minmovedepth.get());
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
		} else if ( nm == "noteonlogic" ) {
			return noteonlogic.get();
		} else if ( nm == "nsprites" ) {
			return IntString(nsprites.get());
		} else if ( nm == "pitchfactor" ) {
			return DoubleString(pitchfactor.get());
		} else if ( nm == "pitchmax" ) {
			return IntString(pitchmax.get());
		} else if ( nm == "pitchmin" ) {
			return IntString(pitchmin.get());
		} else if ( nm == "pitchoffset" ) {
			return IntString(pitchoffset.get());
		} else if ( nm == "quantfactor" ) {
			return DoubleString(quantfactor.get());
		} else if ( nm == "quantfixed" ) {
			return BoolString(quantfixed.get());
		} else if ( nm == "rotanginit" ) {
			return DoubleString(rotanginit.get());
		} else if ( nm == "rotangspeed" ) {
			return DoubleString(rotangspeed.get());
		} else if ( nm == "rotauto" ) {
			return BoolString(rotauto.get());
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
		} else if ( nm == "sound" ) {
			return sound.get();
		} else if ( nm == "speedinitial" ) {
			return DoubleString(speedinitial.get());
		} else if ( nm == "thickness" ) {
			return DoubleString(thickness.get());
		} else if ( nm == "timefret1q" ) {
			return DoubleString(timefret1q.get());
		} else if ( nm == "timefret1y" ) {
			return DoubleString(timefret1y.get());
		} else if ( nm == "timefret2q" ) {
			return DoubleString(timefret2q.get());
		} else if ( nm == "timefret2y" ) {
			return DoubleString(timefret2y.get());
		} else if ( nm == "timefret3q" ) {
			return DoubleString(timefret3q.get());
		} else if ( nm == "timefret3y" ) {
			return DoubleString(timefret3y.get());
		} else if ( nm == "timefret4q" ) {
			return DoubleString(timefret4q.get());
		} else if ( nm == "timefret4y" ) {
			return DoubleString(timefret4y.get());
		} else if ( nm == "tonicchange" ) {
			return IntString(tonicchange.get());
		} else if ( nm == "xcontroller" ) {
			return IntString(xcontroller.get());
		} else if ( nm == "ycontroller" ) {
			return IntString(ycontroller.get());
		} else if ( nm == "zable" ) {
			return BoolString(zable.get());
		} else if ( nm == "zcontroller" ) {
			return IntString(zcontroller.get());
		}
		return "";
	}
	std::string GetType(std::string nm) {
		if ( nm == "alphafinal" ) { return "double"; }
		if ( nm == "alphainitial" ) { return "double"; }
		if ( nm == "alphatime" ) { return "double"; }
		if ( nm == "arpeggio" ) { return "bool"; }
		if ( nm == "aspect" ) { return "double"; }
		if ( nm == "bounce" ) { return "bool"; }
		if ( nm == "controllerchan" ) { return "int"; }
		if ( nm == "controllerstyle" ) { return "string"; }
		if ( nm == "controllerzmax" ) { return "double"; }
		if ( nm == "controllerzmin" ) { return "double"; }
		if ( nm == "doquantize" ) { return "bool"; }
		if ( nm == "filled" ) { return "bool"; }
		if ( nm == "fullrange" ) { return "bool"; }
		if ( nm == "gravity" ) { return "bool"; }
		if ( nm == "huefillfinal" ) { return "double"; }
		if ( nm == "huefillinitial" ) { return "double"; }
		if ( nm == "huefilltime" ) { return "double"; }
		if ( nm == "huefinal" ) { return "double"; }
		if ( nm == "hueinitial" ) { return "double"; }
		if ( nm == "huetime" ) { return "double"; }
		if ( nm == "hvpos" ) { return "double"; }
		if ( nm == "lifetime" ) { return "double"; }
		if ( nm == "luminance" ) { return "double"; }
		if ( nm == "mass" ) { return "double"; }
		if ( nm == "minmove" ) { return "double"; }
		if ( nm == "minmovedepth" ) { return "double"; }
		if ( nm == "mirror" ) { return "string"; }
		if ( nm == "movedir" ) { return "double"; }
		if ( nm == "movedirrandom" ) { return "bool"; }
		if ( nm == "movefollowcursor" ) { return "bool"; }
		if ( nm == "noisevertex" ) { return "double"; }
		if ( nm == "noteonlogic" ) { return "string"; }
		if ( nm == "nsprites" ) { return "int"; }
		if ( nm == "pitchfactor" ) { return "double"; }
		if ( nm == "pitchmax" ) { return "int"; }
		if ( nm == "pitchmin" ) { return "int"; }
		if ( nm == "pitchoffset" ) { return "int"; }
		if ( nm == "quantfactor" ) { return "double"; }
		if ( nm == "quantfixed" ) { return "bool"; }
		if ( nm == "rotanginit" ) { return "double"; }
		if ( nm == "rotangspeed" ) { return "double"; }
		if ( nm == "rotauto" ) { return "bool"; }
		if ( nm == "rotdirrandom" ) { return "bool"; }
		if ( nm == "saturation" ) { return "double"; }
		if ( nm == "shape" ) { return "string"; }
		if ( nm == "sizefinal" ) { return "double"; }
		if ( nm == "sizeinitial" ) { return "double"; }
		if ( nm == "sizetime" ) { return "double"; }
		if ( nm == "sound" ) { return "string"; }
		if ( nm == "speedinitial" ) { return "double"; }
		if ( nm == "thickness" ) { return "double"; }
		if ( nm == "timefret1q" ) { return "double"; }
		if ( nm == "timefret1y" ) { return "double"; }
		if ( nm == "timefret2q" ) { return "double"; }
		if ( nm == "timefret2y" ) { return "double"; }
		if ( nm == "timefret3q" ) { return "double"; }
		if ( nm == "timefret3y" ) { return "double"; }
		if ( nm == "timefret4q" ) { return "double"; }
		if ( nm == "timefret4y" ) { return "double"; }
		if ( nm == "tonicchange" ) { return "int"; }
		if ( nm == "xcontroller" ) { return "int"; }
		if ( nm == "ycontroller" ) { return "int"; }
		if ( nm == "zable" ) { return "bool"; }
		if ( nm == "zcontroller" ) { return "int"; }
		return "";
	}

	DoubleParam alphafinal;
	DoubleParam alphainitial;
	DoubleParam alphatime;
	BoolParam arpeggio;
	DoubleParam aspect;
	BoolParam bounce;
	IntParam controllerchan;
	StringParam controllerstyle;
	DoubleParam controllerzmax;
	DoubleParam controllerzmin;
	BoolParam doquantize;
	BoolParam filled;
	BoolParam fullrange;
	BoolParam gravity;
	DoubleParam huefillfinal;
	DoubleParam huefillinitial;
	DoubleParam huefilltime;
	DoubleParam huefinal;
	DoubleParam hueinitial;
	DoubleParam huetime;
	DoubleParam hvpos;
	DoubleParam lifetime;
	DoubleParam luminance;
	DoubleParam mass;
	DoubleParam minmove;
	DoubleParam minmovedepth;
	StringParam mirror;
	DoubleParam movedir;
	BoolParam movedirrandom;
	BoolParam movefollowcursor;
	DoubleParam noisevertex;
	StringParam noteonlogic;
	IntParam nsprites;
	DoubleParam pitchfactor;
	IntParam pitchmax;
	IntParam pitchmin;
	IntParam pitchoffset;
	DoubleParam quantfactor;
	BoolParam quantfixed;
	DoubleParam rotanginit;
	DoubleParam rotangspeed;
	BoolParam rotauto;
	BoolParam rotdirrandom;
	DoubleParam saturation;
	StringParam shape;
	DoubleParam sizefinal;
	DoubleParam sizeinitial;
	DoubleParam sizetime;
	StringParam sound;
	DoubleParam speedinitial;
	DoubleParam thickness;
	DoubleParam timefret1q;
	DoubleParam timefret1y;
	DoubleParam timefret2q;
	DoubleParam timefret2y;
	DoubleParam timefret3q;
	DoubleParam timefret3y;
	DoubleParam timefret4q;
	DoubleParam timefret4y;
	IntParam tonicchange;
	IntParam xcontroller;
	IntParam ycontroller;
	BoolParam zable;
	IntParam zcontroller;
};

#endif
