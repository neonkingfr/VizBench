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

// Palette* Palette::_singleton = NULL;

const std::string Palette::configSuffix = ".plt";
const std::string Palette::configSeparator = "\\";
int Palette::lastsprite = 0;
int Palette::now = 0;

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
			_regions[i]->regionParams().Set(nm,val);
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
		r->name.c_str(),r->regionParams()->sound.get().c_str(),nm.c_str());
	r->regionParams()->sound.set(nm);
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

Patch*
Palette::NewPatchNamed(std::string nm) {
	Patch* p = new Patch(nm);
	_patches.push_back(p);
	return p;
}

Patch*
Palette::GetPatchNamed(std::string nm) {
	Patch* p = new Patch(nm);
	for ( unsigned int i=0; i<_patches.size(); i++ ) {
		if ( nm == _patches[i]->name() ) {
			return _patches[i];
		}
	}
	return NULL;
}

void
Palette::applyPatch(std::string patchname) {

	Patch* patch = GetPatchNamed(patchname);

	if ( patch == NULL ) {
		throw NosuchException("Unable to find patch named: %s",patchname.c_str());
	}

	for ( size_t i=0; i < _regions.size(); i++ ) {

		Region* r = _regions[i];
		std::string parampath = patch->getRegionParamPath(r->name);

		if ( parampath == "" ) {
			continue;
		}

		r->resetRegionParams();

		std::vector<std::string> files = NosuchSplitOnString(parampath,",");
		for ( size_t i=0; i<files.size(); i++ ) {
			// Read region-specific params for the patch
			AllVizParams* rparams = vizlet()->getAllVizParams(vizlet()->VizParamPath(files[i]));
			if ( rparams == NULL ) {
				DEBUGPRINT(("Unable to retrieve params file: %s",files[i].c_str()));
			} else {
				r->regionParams()->applyVizParamsFrom(rparams);
			}
			// Should rparams be freed?
		}
	}

	std::string effects = patch->getEffects();
	LoadEffectSet(effects);
}

Region*
Palette::NewRegionNamed(std::string nm, int sid_low, int sid_high) {
	Region* r = newRegionNamed(nm);
	NosuchAssert(r);
	r->setTypeAndSid(Region::SURFACE,sid_low,sid_high);
	return r;
}

Region*
Palette::NewButtonNamed(std::string nm, int sid_low, int sid_high, std::string patch) {
	Region* r = newRegionNamed(nm);
	NosuchAssert(r);
	r->setTypeAndSid(Region::BUTTON,sid_low,sid_high);
	r->setButtonPatch(patch);
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

#if 0
EffectSet buttonEffectSet[NUM_EFFECT_SETS] = {
	EffectSet(1,0,0,0,0,0,0,0,0,0,0,0,0),   // 0 - all effects off
	EffectSet(1,1,0,0,0,0,0,0,0,1,0,1,0),   // 1 - twisted, wave warp, edge detect, mirror
	EffectSet(0,0,0,0,0,0,0,0,1,1,0,0,1),   // 2 - iterate, edge detection, trails   GOOD
	EffectSet(1,0,0,1,0,0,0,1,0,0,0,0,1),   // 3 - twisted, blur, displace, trails GOOD?
	EffectSet(1,1,0,0,0,0,0,0,0,0,0,0,0),   // 4 - twisted, wave warp
	EffectSet(1,0,0,0,0,0,1,0,0,1,0,0,1),   // 5 - twisted, blur, posterize, edge detect, trails
	EffectSet(0,0,0,0,1,0,0,0,0,0,0,0,0),   // 6 - goo
	EffectSet(0,0,0,0,0,1,0,0,0,1,0,0,0),   // 7 - fragment, edge
	EffectSet(1,0,0,1,0,0,0,0,1,0,1,1,1),   // 8 - twisted, blur, iterate, blendoscope, mirror, trails
	EffectSet(1,0,1,0,0,0,0,0,0,0,0,0,1),   // 9 - twisted, kaleidoscope, trails  (TRY WITHOUT KALIED!)
	EffectSet(0,0,0,1,0,1,0,0,0,1,0,0,0),   // 10 - blur, fragment, edge detect
	EffectSet(0,0,0,0,1,0,0,1,0,0,0,0,1),   // 11 - goo, trails
};
#endif

Palette::Palette(PaletteHost* b) {

	// _highest_nonbutton_region_id = 4;
	NosuchLockInit(&_palette_mutex,"palette");
	musicscale = "newage";
	currentConfig = "";
	_paletteHost = b;
	_shifted = false;
	_soundbank = 0;
	_recentVizCursor = NULL;

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
}

#if 0
void
Palette::init_loops() {
	for ( size_t i=0; i < _regions.size(); i++ ) {
		NosuchDebug(1,"Initializing loop r=%d",i);
		_regions[i]->init_loop();
	}
}
#endif

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
Palette::SetAllArpeggio(bool arp) {
	NosuchDebug("Palette::SetAllArpeggio arp=%d",arp);
	for ( size_t i=0; i < _regions.size(); i++ ) {
		Region* r = _regions[i];
		if ( r!=NULL && r->isRegion() ) {
			r->regionParams()->arpeggio.set(arp);
		}
	}
}

void
Palette::SetAllFullRange(bool full) {
	NosuchDebug("Palette::SetAllFullRange full=%d",full);
	for ( size_t i=0; i < _regions.size(); i++ ) {
		Region* r = _regions[i];
		if ( r!=NULL && r->isRegion() ) {
			r->regionParams()->fullrange.set(full);
		}
	}
}

void
Palette::processCursor(VizCursor* c, int downdragup) {
	Region* r = RegionForSid(c->sid);
	c->region = r;
	if ( r ) {
		if ( r->isButton() ) {
			switch (downdragup) {
			case CURSOR_DOWN: r->buttonDown(); break;
			case CURSOR_UP: r->buttonUp(); break;
			}
		} else {
			switch (downdragup) {
			case CURSOR_DOWN: r->cursorDown(c); break;
			case CURSOR_DRAG: r->cursorDrag(c); break;
			case CURSOR_UP: r->cursorUp(c); break;
			}
		}
	} else {
		NosuchErrorOutput("Palette::processCursor Unable to find region (A) for sid=%d/%s",c->sid,c->source.c_str());
	}
}

void Palette::buttonDown(std::string bn) {
	_buttonDown[bn] = true;
}

void Palette::buttonUp(std::string bn) {
	_buttonDown[bn] = false;
}

static void writestr(std::ofstream& out, std::string s) {
	const char* p = s.c_str();
	out.write(p,s.size());
}

void Palette::randConfig() {

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

#if 0
std::string Palette::_loadConfig(std::ifstream &f) {
	
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
		_paletteHost->defaultParams()->Set(key,val);
		// params.Set(key,val);
	}

	if ( clearit ) {
		NosuchDebug("Hmmm, is something also needed here?");
	}

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
			region->regionParams()->Set(key,val);
		}
	}

	return "";
}
#endif

void Palette::hitPaletteSprites() {
	LockPalette();
	for ( size_t i=0; i<_regions.size(); i++ ) {
		Region* r = _regions[i];
		r->hitPaletteSprites();
	}
	UnlockPalette();
}

void Palette::advanceTo(int tm) {

	NosuchDebug(1,"===================== Palette::advanceTo tm=%d setting now",tm);
	now = tm;
	LockPalette();
	for ( size_t i=0; i<_regions.size(); i++ ) {
		Region* region = _regions[i];
		region->advanceTo(now);
	}
	UnlockPalette();
}

// public float random(int n) {
// return app.random(n);
// }

int Palette::draw() {

	// pthread_t thr = pthread_self ();
	// NosuchDebug("Palette::draw start thr=%d,%d",(int)(thr.p),thr.x);

	for ( size_t i=0; i<_regions.size(); i++ ) {
		Region* region = _regions[i];
		region->draw(_paletteHost);
	}

	return 0;
}

#include "NosuchColor.h"

static void
normalize(NosuchVector* v)
{
	v->x = (v->x * 2.0) - 1.0;
	v->y = (v->y * 2.0) - 1.0;
}

MidiMsg*
Palette::schedNewNoteInMilliseconds(int sidnum,int ch,int milli,int pitch) {
	double clickspermilli = _paletteHost->vizserver()->GetClicksPerSecond() / 1000.0;
	int clicks = (int)(0.5 + milli * clickspermilli);
	MidiMsg *m = schedNewNoteInClicks(sidnum,ch,clicks,pitch);
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

	_paletteHost->vizserver()->IncomingMidiMsg(m1,clicks,this);
	NosuchDebug(1,"schedNewNoteInClicks end sid=%d",sidnum);

	return m1;
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

void Palette::SetMostRecentVizCursorDown(VizCursor* c) {
	// NosuchDebug(2,"Setting MostRecentVizCursor to %s",c==NULL?"NULL":c->DebugString().c_str());
	_recentVizCursor = c;
}

std::string Palette::ConfigNormalizeSuffix(std::string name) {
	int suffindex = name.length()-configSuffix.length();
	if ( suffindex <= 0 || name.substr(suffindex) != configSuffix ) {
		name += configSuffix;
	}
	return name;
}