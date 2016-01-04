#include <pthread.h>
#include <iostream>
#include <fstream>

#include  <io.h>
#include  <stdlib.h>

#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>

#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <vector>
#include <string>
#include <iostream>

#include <cstdlib> // for srand, rand

#include "PaletteAll.h"

const double Palette::UNSET_DOUBLE = -999.0f;
const std::string Palette::UNSET_STRING = "UNSET";
Palette* Palette::_singleton = NULL;
const std::string Palette::configSuffix = ".plt";
const std::string Palette::configSeparator = "\\";
int Palette::lastsprite = 0;
int Palette::now = 0;

int Palette::CurrentMuseumGraphic = 0;
int Palette::NumMuseumGraphics = 0;
int Palette::CurrentMayGraphic = 0;
int Palette::NumMayGraphics = 0;
int Palette::CurrentBurnGraphic = 0;
int Palette::NumBurnGraphics = 0;

bool Palette::initialized = false;

// ParamList* Palette::globalParams = NULL;
// ParamList* Palette::globalDefaults = NULL;
// ParamList* Palette::globalRegionOverrideParams = NULL;
// ParamList* Palette::globalRegionOverrideDefaults = NULL;

bool Palette::isShiftDown() {
	return ( isButtonDown("UL3"));
}

static int ChannelLastChange[16] = {
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0
};

#if 0
void Palette::SetGlobalParam(std::string nm, std::string val) {
	params.Set(nm,val);
}

void Palette::SetRegionParam(int rid, std::string nm, std::string val) {
	if ( rid == MAGIC_VAL_FOR_OVERRIDE_PARAMS ) {
		for ( size_t i=0; i < _regions.size(); i++ ) {
			_regions[i]->params.Set(nm,val);
		}
	}
}
#endif

int Palette::setRegionSound(std::string region, std::string nm) {
	Region* r = GetRegionNamed(region);
	NosuchAssert(r);
	return setRegionSound(r,nm);
}

int Palette::setRegionSound(int rid, std::string nm) {
	Region* r = getRegion(rid);
	NosuchAssert(r);
	return setRegionSound(r,nm);
}

int Palette::setRegionSound(Region* r, std::string nm) {
	NosuchDebug(1,"setRegionSound region=%s existing==%s new=%s",
		r->name.c_str(),r->params.sound.c_str(),nm.c_str());
	r->params.sound = nm;
	r->UpdateSound();
	return r->channel();
}

void Palette::changeSoundSet(int selected) {
	int sb = soundBank();

	NosuchAssert(selected>=0 && selected<NUM_SOUNDSETS);
	NosuchAssert(sb>=0 && sb<=NUM_SOUNDSETS);

	if ( SoundBank[sb][selected][0] == "" ) {
		NosuchErrorOutput("No sounds in soundbank %d !?",sb);
		return;
	}

	setRegionSound(1,SoundBank[sb][selected][0]);
	setRegionSound(2,SoundBank[sb][selected][1]);
	setRegionSound(3,SoundBank[sb][selected][2]);
	setRegionSound(4,SoundBank[sb][selected][3]);
	NosuchDebug(1,"CHANGED SOUND SET to number %d",selected);
	CurrentSoundSet = selected;
}

Region*
Palette::NewSurfaceNamed(std::string nm, int sid_low, int sid_high) {
	Region* r = newRegionNamed(nm);
	NosuchAssert(r);
	r->SetTypeAndSid(Region::SURFACE,sid_low,sid_high);
	return r;
}

Region*
Palette::NewButtonNamed(std::string nm, int sid_low, int sid_high) {
	Region* r = newRegionNamed(nm);
	NosuchAssert(r);
	r->SetTypeAndSid(Region::BUTTON,sid_low,sid_high);
	return r;
}

Region*
Palette::GetRegionNamed(std::string nm) {
	for ( size_t i=0; i < _regions.size(); i++ ) {
		if ( _regions[i]->name == nm ) {
			return _regions[i];
		}
	}
	NosuchDebug("Hey!  Unable to find a Region named %s !?",nm.c_str());
	return NULL;
}

Region*
Palette::newRegionNamed(std::string nm) {

	Region* r;
	int rid = _regions.size();
	if ( rid == 0 ) {
		r = new Region(this,rid);
		NosuchDebug(1,"CREATING Root Region rid=%d",rid);
		_regions.push_back(r);
		r->name = "root";
		rid = 1;
	}
	r = new Region(this,rid);
	NosuchDebug(1,"CREATING Region rid=%d",rid);
	_regions.push_back(r);

	// NosuchDebug("NEW REGION NAMED nm=%s  rid=%d",nm.c_str(),rid);
	r->name = nm;
	return r;
}

Region*
Palette::RegionForSid(int sidnum) {
	for ( size_t i=0; i < _regions.size(); i++ ) {
		int sid_low = _regions[i]->sid_low;
		int sid_high = _regions[i]->sid_high;
		if ( sidnum >= sid_low && sidnum <= sid_high ) {
			return _regions[i];
		}
	}
	return NULL;
}
void Palette::UpdateSound(int regionid) {
	NosuchDebug("UpdateSound regionid=%d",regionid);
	if ( regionid == MAGIC_VAL_FOR_OVERRIDE_PARAMS ) {
		for ( size_t i=0; i < _regions.size(); i++ ) {
			_regions[i]->UpdateSound();
		}
	} else {
		_regions[regionid]->UpdateSound();
	}
}

int Palette::findSoundChannel(std::string nm, int regionid) {
	Sound& sound = Sounds[nm];
	std::string synthname = sound.synth();

	if ( synthname == "UNINITIALIZED" ) {
		NosuchDebug("HEY!, didn't find sound named %s in findSoundChannel!?",nm.c_str());
		return -1;
	}
	int patch = sound.patchnum();
	int oldestregion = -1;
	int oldestregiontime = -1;

	// look through all the channels and see if any are free (and
	// not the current channel)
	int existingchan;
	if ( regionid >= (int)_regions.size() ) {
		// shouldn't really happen except at startup?
		existingchan = -1;
	} else {
		existingchan = _regions[regionid]->channel();
	}

	NosuchDebug(1,"==== Palette::findSoundChannel start region=%d existingchan=%d nm=%s",
		regionid,existingchan,nm.c_str());

	int foundfreechannel = -1;
	// int foundchan = -1;
	int oldest_time = INT_MAX;
	int oldest_chan = -1;
	for ( int ch=1; ch<=16; ch++ ) {

		if ( Synths[ch] != synthname )
			continue;

		// Channel isn't currently used on any region - see which channel is the oldest
		NosuchDebug(1,"findSoundChannel region=%d, found synth=%s on channel ch=%d lastchange=%d",
			regionid, synthname.c_str(),ch,ChannelLastChange[ch]);
		if ( oldest_chan < 0 || ChannelLastChange[ch] < oldest_time ) {
			NosuchDebug(1,"   SETTING oldest_chan to %d",ch);
			oldest_chan = ch;
			oldest_time = ChannelLastChange[ch];
		}
		// return ch;
	}
	if ( oldest_chan < 0 ) {
		NosuchDebug("HEY!! findSoundChannel region=%d - resorting to existing channel %d for sound %s",
			regionid, existingchan,nm.c_str());
		ChannelLastChange[existingchan] = Palette::now;
		return existingchan;
	}
	NosuchDebug(1,"findSoundChannel region=%d - oldest_chan is %d for sound %s, setting lastchange to %d",
		regionid, oldest_chan,nm.c_str(),Palette::now);
	ChannelLastChange[oldest_chan] = Palette::now;
	return oldest_chan;
}

void Palette::initialize() {
	if ( initialized )
		return;

	initialized = true;

	NosuchDebug(1,"END OF INITIALIZING globalParams and others!\n");
}

// THIS IS USED FOR STEIM !!
char* MuseumGraphics[] = {
	"steim_quickshapes.plt",
	"steim_4shapes.plt",

	"square_dance.plt",
	"steim_triangles.plt",

	"line_dance.plt",
	"web_of_outlines.plt",

	"line1.plt",
	"web_of_filled.plt",

	"line1c.plt",
	"line1b.plt",
	"web_of_outlines.plt",
	"trapezoid_slow.plt",
	NULL
};
char* MayGraphics[] = {
	"steim_quickshapes.plt",
	"oily3.plt",     // "line_dance.plt",
	"square_dance.plt",
	"steim_triangles.plt",
	"steim_4shapes.plt",
	"oily3.plt",     // "steim_quickshapes.plt",
	"line1.plt",
	"line1b.plt",
	"line1c.plt",
	"web_of_filled.plt",
	"web_of_outlines.plt",
	"trapezoid_slow.plt",
	NULL
};
char* BurnGraphics[] = {
	"shape_dance2.plt",   // was "steim_quickshapes.plt",
	"oily3.plt",     // "line_dance.plt",
	"burncircles.plt",    // GOOD
	"burn1c.plt",         // GOOD
	"steim_4shapes.plt",
	"oily3b.plt",     // "steim_quickshapes.plt",
	"fuzz1.plt",
	"burncircles2.plt",    // was "square_dance.plt",
	"steim_triangles.plt",
	"web_of_filled.plt",  // GOOD
	"line1d.plt",         // GOOD
	"burn1c.plt",         // GOOD
	NULL
};

EffectSet buttonEffectSet[NUM_EFFECT_SETS] = {
	EffectSet(0,0,0,0,0,0,0,0,0,0,0,0,0),   // 0 - all effects off
	EffectSet(1,1,0,0,0,0,0,0,0,1,0,1,0),   // 1 - twisted, wave warp, edge detect, mirror
	EffectSet(0,0,0,0,0,0,0,0,0,1,0,0,0),   // 2 - edge detection   GOOD
	EffectSet(1,0,0,1,0,0,0,1,0,0,0,0,1),   // 3 - twisted, blur, displace, trails GOOD?
	EffectSet(1,1,0,0,0,0,0,0,0,0,0,0,0),   // 4 - twisted, wave warp
	EffectSet(1,0,0,1,0,0,1,0,0,1,0,0,1),   // 5 - twisted, blur, posterize, edge detect, trails
	EffectSet(0,0,0,0,0,1,1,0,0,1,0,0,1),   // 6 - fragment, posterize, edge detection, trails
	EffectSet(1,0,0,0,0,1,0,0,0,1,0,0,0),   // 7 - twisted, fragment, edge
	EffectSet(0,0,0,1,0,0,1,0,0,1,0,0,1),   // 8 - blur, posterize, edge detect, trails
	EffectSet(1,0,0,1,0,0,0,0,0,1,0,0,1),   // 9 - twisted, blur, edge detect, trails
	EffectSet(0,0,0,1,0,0,1,0,0,0,0,0,0),   // 10 - blur, posterize
	EffectSet(0,0,0,0,0,0,0,1,0,0,0,0,1),   // 11 - displace, trails
};

Palette::Palette(FreeFrameHost* b) {

	// _highest_nonbutton_region_id = 4;
	NosuchLockInit(&_palette_mutex,"palette");
	currentConfig = "";
	_freeframeHost = b;
	_shifted = false;
	_soundbank = 0;
	_recentCursor = NULL;

	// Assume 16 MIDI channels
	for ( int i=0; i<16; i++ ) {
		Channel* c = new Channel(this,i);
		_channels.push_back(c);
	}

	now = 0;  // Don't use Pt_Time(), it may not have been started yet
	NosuchDebug(1,"Palette constructor, setting now to %d",now);

	frames = 0;
	frames_last = now;
	clearButtonDownAndUsed();
	_effectSet = 0;

	char** pp = MuseumGraphics;
	NumMuseumGraphics = 0;
	while ( *pp++ != NULL ) { NumMuseumGraphics++; }
	CurrentMuseumGraphic = 0;

	pp = MayGraphics;
	NumMayGraphics = 0;
	while ( *pp++ != NULL ) { NumMayGraphics++; }
	CurrentMayGraphic = 0;

	pp = BurnGraphics;
	NumBurnGraphics = 0;
	while ( *pp++ != NULL ) { NumBurnGraphics++; }
	CurrentBurnGraphic = 0;

	if ( _singleton != NULL ) {
		NosuchDebug("HEY!  Palette is instantiated twice!?");
	}
	_singleton = this;
}

void Palette::LoadEffectSet(int eset) {
	_effectSet = eset;
	NosuchDebug(1, "Palette::LoadEffectSet eset=%d", eset);
	if (eset >= _freeframeHost->NumEffectSet()) {
		NosuchDebug("Hey!  eset=%d is >= NumEffectSet?", eset);
		return;
	}
	NosuchDebug("EFFECT SELECTION! set=%d", eset);
	EffectSet es = buttonEffectSet[eset];
	for (int e = 0; e<13; e++) {
		_freeframeHost->EnableEffect(e, es.effectOn[e]);
	}
}
void Palette::LoadRandomEffectSet() {
	int curr = CurrentEffectSet();
	int effset;
	NosuchDebug(2, "Current Effect Set =%d", curr);
	while ((effset = RandomEffectSet()) == curr) {
		// try again
	}
	NosuchDebug("RANDOM Visual effset=%d", effset);
	LoadEffectSet(effset);
}

void
Palette::init_loops() {
	for ( size_t i=0; i < _regions.size(); i++ ) {
		NosuchDebug(1,"Initializing loop r=%d",i);
		_regions[i]->init_loop();
	}
}

Palette::~Palette() {
	for ( size_t i=0; i < _regions.size(); i++ ) {
		if ( _regions[i] ) {
			delete _regions[i];
			_regions[i] = NULL;
		}
	}
}

void
Palette::initRegionSounds() {
	for ( size_t i=0; i < _regions.size(); i++ ) {
		_regions[i]->initSound();
	}
}

void
Palette::ClearAllLoops(bool loopingoff) {
	for ( size_t i=0; i < _regions.size(); i++ ) {
		Region* r = _regions[i];
		if ( r!=NULL && r->isSurface() ) {
			NosuchDebug(1,"Clearing loop of region=%d",r->id);
			r->loop()->Clear();
			if ( loopingoff ) {
				r->Looping(false);
			}
		}
	}
}

void
Palette::SetAllLooping(bool looping, double fade) {
	NosuchDebug("Palette::SetAllLooping looping=%d",looping);
	for ( size_t i=0; i < _regions.size(); i++ ) {
		Region* r = _regions[i];
		if ( r!=NULL && r->isSurface() ) {
			r->Looping(looping);
			if ( looping && fade >= 0.0f ) {
				r->params.loopfade = fade;
			}
		}
	}
}

void
Palette::SetAllArpeggio(bool arp) {
	NosuchDebug("Palette::SetAllArpeggio arp=%d",arp);
	for ( size_t i=0; i < _regions.size(); i++ ) {
		Region* r = _regions[i];
		if ( r!=NULL && r->isSurface() ) {
			r->params.arpeggio = arp;
		}
	}
}

void
Palette::SetAllFullRange(bool full) {
	NosuchDebug("Palette::SetAllFullRange full=%d",full);
	for ( size_t i=0; i < _regions.size(); i++ ) {
		Region* r = _regions[i];
		if ( r!=NULL && r->isSurface() ) {
			r->params.fullrange = full;
		}
	}
}

void Palette::buttonDown(std::string bn) {
	if ( _freeframeHost ) {
		_freeframeHost->_lastActivity = now;
	}
	_buttonDown[bn] = true;
	PyEvent* e = new ButtonDownPyEvent(bn);
	addPyEvent(e);
}

void Palette::buttonUp(std::string bn) {
	if ( _freeframeHost ) {
		_freeframeHost->_lastActivity = now;
	}
	_buttonDown[bn] = false;
	addPyEvent(new ButtonUpPyEvent(bn));
}

void Palette::cursorDown(Cursor* c) {
	if ( _freeframeHost ) {
		_freeframeHost->_lastActivity = now;
	}
	NosuchVector pos = c->curr_pos;
	addPyEvent(new CursorDownPyEvent(c->region()->name,c->sidnum(),c->sidsource(),
		pos.x,pos.y,c->curr_depth));
}

void Palette::cursorDrag(Cursor* c) {
	if ( _freeframeHost ) {
		_freeframeHost->_lastActivity = now;
	}
	NosuchVector pos = c->curr_pos;
	addPyEvent(new CursorDragPyEvent(c->region()->name,c->sidnum(),c->sidsource(),
		pos.x,pos.y,c->curr_depth));
}

void Palette::cursorUp(Cursor* c) {
	if ( _freeframeHost ) {
		_freeframeHost->_lastActivity = now;
	}
	NosuchVector pos = c->curr_pos;
	addPyEvent(new CursorUpPyEvent(c->region()->name,c->sidnum(),c->sidsource(),
		pos.x,pos.y));
}

void Palette::addPyEvent(PyEvent* e) {
	if ( ! _freeframeHost->python_events_disabled() ) {
		_pyevents.push_back(e);
	}
}

PyEvent* Palette::popPyEvent() {
	if ( _freeframeHost->python_events_disabled() ) {
		return NULL;
	}
	PyEvent* e;
	LockPalette();
	if ( _pyevents.size() == 0 )
		e = NULL;
	else {
		e = _pyevents.front();
		_pyevents.pop_front();
	}
	UnlockPalette();
	return e;
}

static void writestr(std::ofstream& out, std::string s) {
	const char* p = s.c_str();
	out.write(p,s.size());
}

void Palette::randConfig() {

}

void
Palette::ConfigSave(std::string name)
{
	std::string filename = ConfigPath(name);

	std::ofstream f(filename.c_str());
	if ( f.fail() ) {
		throw NosuchException("Unable to open config: %s",filename.c_str() );
	} else {
		saveConfig(f);
		f.close();
	}
}

void Palette::saveConfig(std::ofstream& out) {

	std::string tab = "\t";
	writestr(out, "{\n");

	writestr(out, params.JsonString("\"global\": {\n","\t","\n\t},\n") );

#if 0
	writestr(out, regionOverrideParams.JsonString("\"overrideparams\": {\n","\t","\n\t},\n") );
	writestr(out, regionOverrideFlags.JsonString("\"overrideflags\": {\n","\t","\n\t},\n") );

	writestr(out, channelOverrideParams.JsonString("\"channeloverrideparams\": {\n","\t","\n\t},\n") );
	writestr(out, channelOverrideFlags.JsonString("\"channeloverrideflags\": {\n","\t","\n\t},\n") );
#endif

	LockPalette();

	std::string sep;

	writestr(out, "\"regions\": [\n");
	sep = "";
	for ( size_t i=0; i<_regions.size(); i++ ) {
		Region* r = _regions[i];
		writestr(out,sep+NosuchSnprintf("\t{ \"id\": %d",i));
		writestr(out, r->regionSpecificParams.JsonString(",\n\t\"regionspecificparams\": {\n","\t\t","\n\t\t}\n\t}") );
		sep = ",\n";
	}
	writestr(out, "\n" + tab + "],\n");

	writestr(out, "\"channels\": [\n");
	sep = "";
	for ( size_t i=0; i<_channels.size(); i++ ) {
		Channel* c = _channels[i];
		writestr(out,sep+NosuchSnprintf("\t{ \"id\": %d",i));
		writestr(out, c->channelSpecificParams.JsonString(",\n\t\"channelspecificparams\": {\n","\t\t","\n\t\t}\n\t}") );
		sep = ",\n";
	}
	writestr(out, "\n" + tab + "]\n");

	writestr(out, "}\n");

	UnlockPalette();
}

static std::string debugJson(cJSON *j, int indent) {
	std::string s = std::string(indent,' ').c_str();
	switch (j->type) {
	case cJSON_False:
		s += NosuchSnprintf("%s = False\n",j->string);
		break;
	case cJSON_True:
		s += NosuchSnprintf("%s = True\n",j->string);
		break;
	case cJSON_NULL:
		s += NosuchSnprintf("%s = NULL\n",j->string);
		break;
	case cJSON_Number:
		s += NosuchSnprintf("%s = (number) %.3f\n",j->string,j->valuedouble);
		break;
	case cJSON_String:
		s += NosuchSnprintf("%s = (string) %s\n",j->string,j->valuestring);
		break;
	case cJSON_Array:
		s += NosuchSnprintf("%s = (array)\n",j->string);
		for ( cJSON* j2=j->child; j2!=NULL; j2=j2->next ) {
			for ( cJSON* j3=j2->child; j3!=NULL; j3=j3->next ) {
				s += debugJson(j3,indent+3);
			}
		}
		break;
	case cJSON_Object:
		s += NosuchSnprintf("%s = object\n",j->string==NULL?"NULL":j->string);
		for ( cJSON* j2=j->child; j2!=NULL; j2=j2->next ) {
			s += debugJson(j2,indent+3);
		}
		break;
	default:
		s += NosuchSnprintf("Unable to handle JSON type=%d in debugJSON?\n",j->type);
		break;
	}
	return s;
}

std::string jsonValueString(cJSON* j) {
	std::string val;

	switch (j->type) {
	case cJSON_Number:
		val = NosuchSnprintf("%f",j->valuedouble);
		break;
	case cJSON_String:
		val = j->valuestring;
		break;
	default:
		throw NosuchException("jsonValueString not prepared to handle type=%d",j->type);
	}
	return val;
}

std::string Palette::loadConfig(std::ifstream &f) {
	
	// try not to throw exceptions, locks get left locked

	bool clearit = false;

	// Read File Line By Line
	std::string line;
	std::string data = "";
	while (!std::getline(f,line,'\n').eof()) {
		// Print the content on the console
		data += line;
	}
	if ( clearit ) {
		NosuchDebug("Hmmm, is something needed here?");
		// globalParams->clear();
	}
	cJSON *json = cJSON_Parse(data.c_str());
	if ( json == NULL ) {
		return NosuchSnprintf("NULL return from cJSON_Parse for <<%s...>>",data.substr(0,30).c_str());
	}
	if ( json->type != cJSON_Object ) {
		return NosuchSnprintf("JSON file didn't contain an Object?");
	}
	if ( NosuchDebugLevel > 0 ) {
		NosuchDebug("JSON of Config file follows:\n%s\n",debugJson(json,0).c_str());
	}
	cJSON *j = cJSON_GetObjectItem(json,"global");
	if ( j == NULL ) {
		return NosuchSnprintf("JSON file didn't contain a global Object?");
	}

	cJSON *j2;
	for ( j2=j->child; j2!=NULL; j2=j2->next ) {
		std::string key = j2->string;
		std::string val = jsonValueString(j2) ;
		params.Set(key,val);
	}

	if ( clearit ) {
		NosuchDebug("Hmmm, is something also needed here?");
		// globalRegionOverrideParams->clear();
	}

#if 0
	j = cJSON_GetObjectItem(json,"overrideparams");
	if ( j == NULL ) {
		return NosuchSnprintf("JSON file didn't contain an overrideparams Object?");
	}
	for ( j2=j->child; j2!=NULL; j2=j2->next ) {
		std::string key = j2->string;
		std::string val = jsonValueString(j2) ;
		regionOverrideParams.Set(key,val);
	}
	j = cJSON_GetObjectItem(json,"overrideflags");
	if ( j == NULL ) {
		return NosuchSnprintf("JSON file didn't contain an overrideflags Object?");
	}
	for ( j2=j->child; j2!=NULL; j2=j2->next ) {
		std::string key = j2->string;
		std::string val = jsonValueString(j2) ;
		regionOverrideFlags.Set(key,val);
	}
#endif

	j = cJSON_GetObjectItem(json,"regions");
	if ( j == NULL ) {
		return NosuchSnprintf("JSON file didn't contain a regions Object?");
	}
	if ( j->type != cJSON_Array ) {
		return NosuchSnprintf("regions object in JSON isn't an array!?");
	}
	int nregions = cJSON_GetArraySize(j);
	for (int i = 0; i < nregions; i++) {
		cJSON* ja = cJSON_GetArrayItem(j,i);
		cJSON* j_id = cJSON_GetObjectItem(ja,"id");
		cJSON* j_prms = cJSON_GetObjectItem(ja,"regionspecificparams");
		std::string nm = "";
		switch(j_id->valueint) {
		case 1: nm = "LOWER"; break;
		case 2: nm = "LEFT"; break;
		case 3: nm = "RIGHT"; break;
		case 4: nm = "UPPER"; break;
		}
		if ( nm == "" ) {
			NosuchDebug(2,"Ignoring region id=%d in config",j_id->valueint);
			continue;
		}
		Region* region = GetRegionNamed(nm);
		if ( region == NULL ) {
			NosuchDebug("Ignoring region named %s!?",nm.c_str());
			continue;
		}
		cJSON* j4;
		for ( j4=j_prms->child; j4!=NULL; j4=j4->next ) {
			std::string key = j4->string;
			std::string val = jsonValueString(j4) ;
			region->regionSpecificParams.Set(key,val);
		}
	}

	j = cJSON_GetObjectItem(json,"channels");
	if ( j == NULL ) {
		// return NosuchSnprintf("JSON file didn't contain a channels Object?");
		NosuchDebug("WARNING: JSON file didn't contain a channels Object");
		return "";
	}
	if ( j->type != cJSON_Array ) {
		return NosuchSnprintf("channels object in JSON isn't an array!?");
	}
	int nchannels = cJSON_GetArraySize(j);
	for (int i = 0; i < nchannels; i++) {
		cJSON* ja = cJSON_GetArrayItem(j,i);
		cJSON* j_id = cJSON_GetObjectItem(ja,"id");
		cJSON* j_prms = cJSON_GetObjectItem(ja,"channelspecificparams");
		int ch = j_id->valueint;
		Channel* channel = _channels[ch];
		if ( channel == NULL ) {
			NosuchDebug("Ignoring channel number %d!?",ch);
			continue;
		}
		cJSON* j4;
		for ( j4=j_prms->child; j4!=NULL; j4=j4->next ) {
			std::string key = j4->string;
			std::string val = jsonValueString(j4) ;
			channel->channelSpecificParams.Set(key,val);
		}
	}

	j = cJSON_GetObjectItem(json,"channeloverrideparams");
	if ( j == NULL ) {
		return NosuchSnprintf("JSON file didn't contain an channeloverrideparams Object?");
	}
	for ( j2=j->child; j2!=NULL; j2=j2->next ) {
		std::string key = j2->string;
		std::string val = jsonValueString(j2) ;
		channelOverrideParams.Set(key,val);
	}

	j = cJSON_GetObjectItem(json,"channeloverrideflags");
	if ( j == NULL ) {
		return NosuchSnprintf("JSON file didn't contain an channeloverrideflags Object?");
	}
	for ( j2=j->child; j2!=NULL; j2=j2->next ) {
		std::string key = j2->string;
		std::string val = jsonValueString(j2) ;
		channelOverrideFlags.Set(key,val);
	}

	ResetChannelParams();
	return "";
}

void Palette::hitSprites() {
	LockPalette();
	for ( size_t i=0; i<_regions.size(); i++ ) {
		Region* r = _regions[i];
		r->hitSprites();
	}
	for ( size_t i=0; i<_channels.size(); i++ ) {
		Channel* c = _channels[i];
		c->hitSprites();
	}
	UnlockPalette();
}

void Palette::advanceTo(int tm) {

	NosuchDebug(1,"===================== Palette::advanceTo tm=%d setting now",tm);
	now = tm;
	bool addrandom = false;
	if ( idleattract > 0 && now > (lastsprite+idleattract) ){
		NosuchDebug("Idle, should be forcing a new sprite\n");
		lastsprite = now;
		addrandom = true;
	}
	LockPalette();
	for ( size_t i=0; i<_regions.size(); i++ ) {
		Region* region = _regions[i];
		region->advanceTo(now);
		if ( addrandom ) {
			region->addrandom();
		}
	}
	for ( size_t i=0; i<_channels.size(); i++ ) {
		Channel* c = _channels[i];
		c->advanceTo(now);
	}
	UnlockPalette();

	if (params.showfps) {
		frames++;
		// Every second, print out FPS
		if (now > (frames_last + 1000)) {
			NosuchDebug("FPS=%d  now=%d",frames,now);
			frames = 0;
			frames_last = now;
		}
	}
}

// public float random(int n) {
// return app.random(n);
// }

int Palette::draw() {

	// pthread_t thr = pthread_self ();
	// NosuchDebug("Palette::draw start thr=%d,%d",(int)(thr.p),thr.x);

	for ( size_t i=0; i<_regions.size(); i++ ) {
		Region* region = _regions[i];
		region->draw();
	}

	for ( size_t i=0; i<_channels.size(); i++ ) {
		Channel* c = _channels[i];
		c->draw();
	}

	return 0;
}

void
Palette::getCursors(std::list<Cursor*>& cursors ) {
	for ( size_t i=0; i<_regions.size(); i++ ) {
		Region* region = _regions[i];
		region->getCursors(cursors);
	}
}

#include "NosuchColor.h"

void
Palette::process_midi_input(int msg) {

	int millinow = Pt_Time();

	int status = Pm_MessageStatus(msg);
	int command = status & 0xf0;
	int chan = status & 0x0f;
	if ( command != MIDI_NOTE_ON ) {
		return;
	}
	Channel* c = _channels[chan];
	c->setMidiBehaviour(c->params.behaviour);

	int pitch = Pm_MessageData1(msg);
	int velocity = Pm_MessageData2(msg);
	c->gotNoteOn(pitch,velocity);
	// NosuchDebug("process_midi_input time=%8d  msg=%02x %02x %02x\n",
	// 	Pt_Time(), status, pitch, velocity);
}

void
Palette::process_cursors_from_buff(MMTT_SharedMemHeader* hdr) {

	int millinow = Pt_Time();

	buff_index b = hdr->buff_to_display;
	int ncursors = hdr->numoutlines[b];
	if ( ncursors == 0 ) {
		goto getout;
	}

	for ( int n=0; n<ncursors; n++ ) {
		OutlineMem* om = hdr->outline(b,n);
		Cursor* c = SetCursorSid(om->sid,
			"sharedmem", millinow, NosuchVector(om->x,om->y), om->z, 0.5,om);
#if 0
		if ( c == NULL ) {
			NosuchDebug("HEY!  c==NULL in process_cursors_from_buff?");
		} else {
			c->setoutline(om);
		}
#endif
	}

getout:
	CheckCursorUp(millinow);
}

static void
normalize(NosuchVector* v)
{
	v->x = (v->x * 2.0) - 1.0;
	v->y = (v->y * 2.0) - 1.0;
}

#if 0
void
Palette::make_sprites_along_outlines(MMTT_SharedMemHeader* hdr) {

	buff_index b = hdr->buff_to_display;
	int noutlines = hdr->numoutlines[b];
	if ( noutlines == 0 ) {
		return;
	}

	// NosuchDebug("Drawing outline from hdr=%lx buff=%d, noutlines=%d",(long)hdr,b,noutlines);

	for ( int n=0; n<noutlines; n++ ) {
		OutlineMem* om = hdr->outline(b,n);
		// std::string sidstr = sidString(om->sid,"outline");
		Region* r = getRegion(om->region);

		if ( ! r->params.outlinesprites )  {
			continue;
		}

		int npoints = om->npoints;
		// NosuchDebug("Drawing outline from buff=%d, outline=%d npoints=%d",b,n,npoints);
		if ( npoints > 20000 ) {
			NosuchDebug("Corruption in outline! h=%lx b=%d noutlines=%d n=%d",(long)hdr,b,noutlines,n);
			return;
		}
		int pn0 = om->index_of_firstpoint;


		int sparseness = npoints / r->params.outlinensprites ;
		if ( sparseness <= 0 )
			sparseness = 1;

		NosuchColor color1 = NosuchColor(r->params.huefillinitial,0.5,0.5);
		NosuchColor color2 = NosuchColor(r->params.hueinitial,0.5,0.5);

		NosuchVector vfirst;
		NosuchVector v0;
		NosuchVector v1;

		int pn;
		for ( pn=0; pn<npoints; pn++ ) {
			PointMem* p = hdr->point(b,pn+pn0);
			if ( r->params.outlinesprites==true && (pn % sparseness)==0 ) {
				Cursor* nc = new Cursor(this,om->sid,"sharedmem",
					r,NosuchVector(p->x,p->y),p->z,0.5);
				r->instantiateSprite(nc,false);
				delete nc;
			}
		}

		if ( pn > 1 ) {
			app->stroke(color2,1.0);
			app->strokeWeight(1.0);
			app->line(v0.x,v0.y,vfirst.x,vfirst.y);
		}
	}
	// NosuchDebug("End of draw_outlines");
}
#endif

void
Palette::schedSessionEnd(int sidnum) {
	NosuchDebug(2,"schedSessionEnd sid=%d",sidnum);
	NosuchScheduler* s = scheduler();
	s->IncomingSessionEnd(s->CurrentClick(),sidnum);
	return;
}

MidiMsg*
Palette::schedNewNoteInMilliseconds(int sidnum,int ch,int milli,int pitch) {
	int clicks = (int)(0.5 + milli * scheduler()->ClicksPerMillisecond);
	NosuchDebug(1,"schedNewNoteInMilliseconds milli=%d clicks=%d _currentclick=%d",milli,clicks,scheduler()->_currentclick);
	MidiMsg *m = schedNewNoteInClicks(sidnum,ch,clicks,pitch);
	NosuchDebug(1,"schedNewNoteInMilliseconds end");
	return m;
}

MidiMsg*
Palette::schedNewNoteInClicks(int sidnum,int ch,int clicks,int pitch) {
	NosuchDebug(1,"schedNewNoteInClicks start sid=%d",sidnum);
	if ( NosuchDebugMidiNotes ) {
		NosuchDebug("NEWNOTE! sid=%d  chan=%d",sidnum,ch);
	}
	if ( ch <= 0 ) {
		NosuchDebug("NOT SENDING MIDI NOTE!  ch=%d",ch);
		return NULL;
	}

	// Should the scheduler be locked, here?

	int velocity = 127;  // Velocity should be based on something else
	MidiMsg* m1 = MidiNoteOn::make(ch,pitch,velocity);
	NosuchDebug(1,"schedNewNoteInClicks mid sid=%d",sidnum);

	scheduler()->IncomingMidiMsg(m1,clicks,sidnum);
	NosuchDebug(1,"schedNewNoteInClicks end sid=%d",sidnum);

	return m1;
}

void Palette::checkCursorUp(int milli) {
	for ( size_t i=0; i < _regions.size(); i++ ) {
		_regions[i]->checkCursorUp(milli);
	}
}

Region* Palette::getRegion(int r) {
	// NosuchDebug("getRegion r=%d  _regions.size=%d",r, _regions.size());
	// NosuchAssert(r<_regions.size());

	if ( r < (int)_regions.size() ) {
		// NosuchDebug("Returning existing region r=%d region=%d",r,(int)_regions[r]);
		return _regions[r];
	}
	// create it (and any lower-numbered regions) if it doesn't exist

	NosuchDebug("IS THIS CODE USED ANYMORE?  Creating Region %d inside getRegion",r);
	LockPalette();
	for ( int rnum=_regions.size(); rnum <= r; rnum++ ) {
		NosuchDebug(1,"getRegion creating new Region, rnum=%d",rnum);
		Region* rp = new Region(this,rnum);
		// rp->initParams();
		NosuchDebug(1,"CREATING Region r=%d",rnum);
		_regions.push_back(rp);
	}
	UnlockPalette();

	// NosuchDebug("getRegion end _regions.size=%d",_regions.size());
	return _regions[r];
}

void Palette::SetMostRecentCursorDown(Cursor* c) {
	// NosuchDebug(2,"Setting MostRecentCursor to %s",c==NULL?"NULL":c->DebugString().c_str());
	_recentCursor = c;
}

std::string Palette::ConfigNormalizeSuffix(std::string name) {
	int suffindex = name.length()-configSuffix.length();
	if ( suffindex <= 0 || name.substr(suffindex) != configSuffix ) {
		name += configSuffix;
	}
	return name;
}

std::string Palette::ParamConfigDir() {
	return _freeframeHost->ParamConfigDir();
}

void Palette::ConfigFiles(std::vector<std::string>& files) {

	UINT counter(0);
	bool working(true);
	std::string buffer;
	std::string fileName[1000];

	WIN32_FIND_DATAA myimage;
	std::string lookfor = ParamConfigDir() + "\\*" + configSuffix;
	HANDLE myHandle=FindFirstFileA(lookfor.c_str(),&myimage);

	// NosuchDebug("ConfigFiles start");
	if(myHandle!=INVALID_HANDLE_VALUE) {
		buffer = std::string(myimage.cFileName);
		files.push_back(buffer);

		while (FindNextFileA(myHandle, &myimage) != 0) {
			std::string file = std::string(myimage.cFileName);
			// Ignore vim temp files
			if ( file.find('~') != file.npos ) {
				continue;
			}
			files.push_back(file);
		}
		DWORD dwError = GetLastError();
		FindClose(myHandle);

		if (dwError != ERROR_NO_MORE_FILES) {
			 NosuchDebug("FindNextFile error is %u.\n", dwError);
		}
	}
	if (files.size() == 0) {
		throw NosuchException("There are no config files matching %s !?",lookfor.c_str());
	}
}

std::string Palette::ConfigPath(std::string name) {
	std::string s = ParamConfigDir() + configSeparator
			+ ConfigNormalizeSuffix(name);
	return NosuchForwardSlash(s);
	// return s;
}

int Palette::ConfigIndex(std::vector<std::string> &files, std::string name) {
	ConfigFiles(files);
	name = ConfigNormalizeSuffix(name);
	for (unsigned int n = 0; n < files.size(); n++) {
		std::string s = files[n];
		if (name == s) {
			return n;
		}
	}
	return -1;
}

std::string Palette::ConfigNext(std::string name, int dir) {
	std::vector<std::string> files;
	ConfigFiles(files);
	int i = ConfigIndex(files, name);
	i += dir;
	if (i < 0) {
		i = 0;
	} else if (i > (int)(files.size() - 1)) {
		i = files.size() - 1;
	}
	return files[i];
}

std::string Palette::ConfigNew(std::string name) {
	size_t suffleng = configSuffix.length();
	std::string base = name;
	std::string suff = (suffleng>base.length()) ? "" : base.substr(base.length()-suffleng);
	NosuchDebug("ConfigNew, name=%s suff=%s",name.c_str(),suff.c_str());
	if (suff == configSuffix) {
		base = name.substr(0, name.length() - configSuffix.length());
	}
	NosuchDebug("ConfigNew, name=%s suff=%s base=%s",name.c_str(),suff.c_str(),base.c_str());
	// If there's a _, remove it and everything after
	int i = base.rfind('_');
	if (i > 0) {
		base = base.substr(0, i);
	}
	std::string cp = ConfigPath(base);
	struct _stat statbuff;
	if( _stat( cp.c_str(), &statbuff ) < 0 ) {
	    // file doesn't exist, so it's okay as a new name
		return base;
	}
	i = 0;
	while (true) {
		std::string newbase = NosuchSnprintf("%s_%d",base.c_str(),i);
		std::string fn = ConfigPath(newbase);
		if( _stat( fn.c_str(), &statbuff ) < 0 ) {
		    // file doesn't exist, so it's okay as a new name
			return newbase;
		}
		i++;
	}
}

std::string Palette::ConfigRand(std::string name) {
	std::vector<std::string> files;
	ConfigFiles(files);
	NosuchAssert(files.size()>0);
	if (files.size() == 1) {
		return files[0];
	}
	// Make sure we get a new one. name might be null
	int curri = (name == "") ? -1 : ConfigIndex(files,name);
	int randi;
	while (1) {
		randi = rand() % files.size();
		if (randi != curri) {
			break;
		}
	}
	return files[randi];
}

std::string Palette::ConfigLoadRand(std::string& name) {
	name = ConfigRand(currentConfig);
	return ConfigLoad(name);
}

std::string Palette::ConfigLoad(std::string name) {
	std::string filename = ConfigPath(name);
	NosuchDebug("ConfigLoad file=%s",filename.c_str());

	std::ifstream f(filename.c_str(), std::ifstream::in | std::ifstream::binary);
	if ( ! f.is_open() ) {
		return NosuchSnprintf("Unable to open config file: %s",filename.c_str());
	}
	DEBUGPRINT(("Palette::ConfigLoad - should there be a lock here?"));
	// _freeframeHost->lock_freeframeHost();
	std::string r = loadConfig(f);
	// _freeframeHost->unlock_freeframeHost();
	f.close();
	if ( r == "" ) {
		currentConfig = name;
		NosuchDebug(1,"ConfigLoad successful");
	} else {
		NosuchDebug("ConfigLoad of %s NOT successful!",name.c_str());
	}
	return r;
}

