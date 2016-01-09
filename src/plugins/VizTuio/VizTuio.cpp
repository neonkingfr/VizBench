#include "Vizlet.h"
#include "VizTuio.h"

static CFFGLPluginInfo PluginInfo ( 
	VizTuio::CreateInstance,	// Create method
	"V018",		// Plugin unique ID
	"VizTuio",	// Plugin name	
	1,			// API major version number
	000,		// API minor version number	
	1,			// Plugin major version number
	000,		// Plugin minor version number
	FF_EFFECT,	// Plugin type
	"VizTuio: a sample visual synth",	// description
	"by Tim Thompson - me@timthompson.com" 			// About
);

std::string vizlet_name() { return "VizTuio"; }
CFFGLPluginInfo& vizlet_plugininfo() { return PluginInfo; }
void vizlet_setdll(std::string dll) { }

VizTuio::VizTuio() : Vizlet() {

	DEBUGPRINT(("VizTuio is being created and initialized! this=%ld",(long)this));

	const char* regionnames[] = { "UPPER", "LOWER", "LEFT", "RIGHT", NULL };
	const char* buttonnames[] = {	"UL1", "UL2", "UL3", "UR1", "UR2", "UR3",
									"LL1", "LL2", "LL3", "LR1", "LR2", "LR3",
									NULL
									};

	const char* s;
	for (int i = 0; (s=regionnames[i])!=NULL; i++ ) {
		_region[s] = new Region(s);
		DEBUGPRINT(("============ NEW REGION viztuio=%ld _region[%s]=%ld",(long)this,s,(long)(_region[s])));
	}
	for (int i = 0; (s = buttonnames[i]) != NULL; i++) {
		_button[s] = new Button(s);
	}

	_freeframeHost = new ResolumeHost();
	_palette = new Palette(_freeframeHost);

	_autoloadparams = true;
}

VizTuio::~VizTuio() {
}

DWORD __stdcall VizTuio::CreateInstance(CFreeFrameGLPlugin **ppInstance) {
	*ppInstance = new VizTuio();
	return (*ppInstance != NULL)? FF_SUCCESS : FF_FAIL;
}

void VizTuio::processCursor(VizCursor* c, int downdragup) {
	// NO OpenGL calls here
	DEBUGPRINT1(("VizTuio::processCursor! downdragup=%d c=%.4f %.4f",downdragup,c->pos.x,c->pos.y));
	// palette()->processCursor(c,downdragup);
	if (downdragup == CURSOR_DOWN || downdragup == CURSOR_DRAG) {
		SpriteVizParams *sp = NULL;
		MidiVizParams *mp = NULL;
		// Look through the regions to find the one that matches the cursor's sid
		int sid = c->sid;
		for (const auto &pair : _region ) {
			if (sid >= pair.second->sid_min && sid <= pair.second->sid_max) {
				sp = pair.second->spriteparams;
				mp = pair.second->midiparams;
				break;
			}
		}
		if (sp) {
			_cursorSprite(c, sp);
		}
		if (mp) {
			_cursorMidi(c,mp);
		}
	}
}

std::string VizTuio::processJson(std::string meth, cJSON *json, const char *id) {
	// NO OpenGL calls here

	DEBUGPRINT1(("VizTuio::processJson viztuio=%ld meth=%s",(long)this,meth.c_str()));
	if (meth == "apis") {
		std::string apis = "";
		for (const auto &pair : _region) {
			std::string nm = pair.first;
				apis += "set_" + nm + "_sidrange(sidrange);";
				apis += "set_" + nm + "_sprite(paramfile);";
				apis += "set_" + nm + "_midi(paramfile);";
		}
		for (const auto &pair : _button) {
			std::string nm = pair.first;
				apis += "set_" + nm + "_sidrange(sidrange);";
				apis += "set_" + nm + "_pipeline(pipeline);";
		}
		apis += "set_autoloadparams(onoff);";
		apis += "testapi();";
		return jsonStringResult(apis, id);
	}

	if (meth == "testapi") {
		DEBUGPRINT(("VizTuio.testapi called!"));
		return jsonOK(id);
	}

	if (meth == "dump") {
		std::string dump = "[";
		std::string sep = "";
		for (const auto &pair : _region) {
			dump += sep + NosuchSnprintf("{\"method\":\"set_%s_sidrange\",\"params\":{\"sidrange\":\"%d-%d\"}}", pair.first.c_str(), pair.second->sid_min, pair.second->sid_max);
			sep = ",";
			dump += sep + NosuchSnprintf("{\"method\":\"set_%s_sprite\",\"params\":{\"paramfile\":\"%s\"}}", pair.first.c_str(), pair.second->spriteparamfile.c_str());
			dump += sep + NosuchSnprintf("{\"method\":\"set_%s_midi\",\"params\":{\"paramfile\":\"%s\"}}", pair.first.c_str(), pair.second->midiparamfile.c_str());
		}
		for (const auto &pair : _button) {
			dump += sep + NosuchSnprintf("{\"method\":\"set_%s_sidrange\",\"params\":{\"sidrange\":\"%d-%d\"}}", pair.first.c_str(), pair.second->sid_min, pair.second->sid_max);
			dump += sep + NosuchSnprintf("{\"method\":\"set_%s_pipeline\",\"params\":{\"pipeline\":\"%s\"}}", pair.first.c_str(), pair.second->pipeline.c_str());
		}
		dump += "]";
		return jsonStringResult(dump, id);
	}

	// Here we go through all the region names and look for their set/get methods
	for (const auto &pair : _region) {
		std::string nm = pair.first;
		Region* r = pair.second;

		if (meth == "set_" + nm + "_sidrange") { return _set_sidrange(r, json, id); }
		if (meth == "get_" + nm + "_sidrange") { return jsonStringResult(NosuchSnprintf("%d-%d", r->sid_min, r->sid_max), id); }

		if (meth == "set_" + nm + "_sprite") { return _set_region_spriteparams(r, json, id); }
		if (meth == "get_" + nm + "_sprite") { return jsonStringResult(r->spriteparamfile, id); }

		if (meth == "set_" + nm + "_midi") { return _set_region_midiparams(r, json, id); }
		if (meth == "get_" + nm + "_midi") { return jsonStringResult(r->midiparamfile, id); }
	}

	// Here we go through all the button names and look for their set/get methods
	for (const auto &pair : _button) {
		std::string nm = pair.first;
		Button* b = pair.second;

		if (meth == "set_" + nm + "_sidrange") { return _set_sidrange(b, json, id); }
		if (meth == "get_" + nm + "_sidrange") { return jsonStringResult(NosuchSnprintf("%d-%d", b->sid_min, b->sid_max), id); }

		if (meth == "set_" + nm + "_pipeline") { return _set_button_pipeline(b, json, id); }
		if (meth == "get_" + nm + "_pipeline") { return jsonStringResult(b->pipeline, id); }
	}

	// PARAMETER "autoloadparams"
	if (meth == "set_autoloadparams") {
		_autoloadparams = jsonNeedBool(json, "onoff", false);
		return jsonOK(id);
	}
	if (meth == "get_autoloadparams") {
		return jsonIntResult(_autoloadparams ? 1 : 0, id);
	}

	throw NosuchException("VizTuio - Unrecognized method '%s'",meth.c_str());
}

void
VizTuio::_reloadParams(Region* r) {
	SpriteVizParams* p;
	p = checkSpriteVizParamsAndLoadIfModifiedSince(r->spriteparamfile, r->lastspritefilecheck, r->lastspritefileupdate);
	if (p) {
		r->spriteparams = p;
	}
	MidiVizParams* m;
	m = checkMidiVizParamsAndLoadIfModifiedSince(r->midiparamfile, r->lastmidifilecheck, r->lastmidifileupdate);
	if (m) {
		r->midiparams = m;
	}
}

bool VizTuio::processDraw() {
	// OpenGL calls here
	if (_autoloadparams) {
		for (const auto &pair : _region ) {
			Region* r = pair.second;
			_reloadParams(r);
		}
	}
	DrawVizSprites();
	return true;
}

void VizTuio::processAdvanceTimeTo(int milli) {
	// DO NOT PUT DEBUGPRINT HERE!
	// DEBUGPRINT(("VizTuio::processAdvanceTimeTo milli=%d",milli));
	// CheckVizCursorUp(milli);
	palette()->AdvanceTo(milli);
}

int VizTuio::_channelOf(VizCursor* c) {
	return 1;
}

int VizTuio::_pitchOf(VizCursor* c) {
	return 80;
}

int VizTuio::_velocityOf(VizCursor* c) {
	return 100;
}

void VizTuio::_cursorMidi(VizCursor* c, MidiVizParams* p) {
	MidiPhrase *ph = new MidiPhrase();
	int ch = _channelOf(c);
	int pitch = _pitchOf(c);
	int vel = _velocityOf(c);
	click_t now = SchedulerCurrentClick();
	int bpm = 120;
	click_t clksperbeat = SchedulerClicksPerSecond() * 60 / bpm;
	click_t eighth = clksperbeat / 2;  // A beat is a quarter note, so this is an eighth note
	click_t dur = 4 * eighth;

	MidiNoteOn *noteon = MidiNoteOn::make(ch, pitch, vel);
	MidiNoteOff *noteoff = MidiNoteOff::make(ch, pitch, 0);
	ph->insert(noteon, 0);
	ph->insert(noteoff, dur);
	QueueMidiPhrase(ph, now);
}

void VizTuio::_cursorSprite(VizCursor* c, SpriteVizParams* spriteparams) {

	DEBUGPRINT1(("cursorSprite! sid=%d xyz = %.5f %.5f %.5f", c->sid, c->pos.x, c->pos.y, c->pos.z));

	NosuchPos pos = c->pos;
	double movedir = 0.0;
	bool randpos = false;
	if (randpos) {
		pos.x = pos.y = 0.5;
		// pos.z = (m->Velocity()*m->Velocity()) / (128.0*128.0);
		movedir = (double)(rand() % 360);
		spriteparams->movedir.set(movedir);
	}
	else {
		movedir = spriteparams->movedir.get();
	}

	DEBUGPRINT2(("_cursorSprite spriteparams=%ld shape=%s",(long)spriteparams,spriteparams->shape.get().c_str()));
	makeAndAddVizSprite(spriteparams, pos);
}

std::string
VizTuio::_set_region_spriteparams(Region* r, cJSON* json, const char* id)
{
	std::string file = jsonNeedString(json, "paramfile", "");
	if (file == "") {
		return jsonError(-32000, "Bad file value", id);
	}
	if (_loadSpriteVizParamsFile(file, r)) {
		return jsonOK(id);
	}
	else {
		return jsonError(-32000, "Unable to load spriteparams file", id);
	}
}

std::string
VizTuio::_set_sidrange(SidRangeable* b, cJSON* json, const char* id)
{
	std::string sidrange = jsonNeedString(json, "sidrange", "");
	if (sidrange == "") {
		return jsonError(-32000, "Bad sidrange value", id);
	}
	int sidmin;
	int sidmax;
	int n = sscanf(sidrange.c_str(), "%d-%d",&sidmin,&sidmax);
	if (n == 1) {
		b->sid_min = sidmin;
		b->sid_max = sidmin;
	}
	else if (n == 2) {
		b->sid_min = sidmin;
		b->sid_max = sidmax;
	}
	else {
		return jsonError(-32000, "Unable to interpret sidrange value: %s", sidrange.c_str());
	}
	return jsonOK(id);
}

std::string
VizTuio::_set_button_pipeline(Button* b, cJSON* json, const char* id)
{
	std::string pipeline = jsonNeedString(json, "pipeline", "");
	if (pipeline == "") {
		return jsonError(-32000, "Bad pipeline value", id);
	}
	b->pipeline = pipeline;
	return jsonOK(id);
}

bool VizTuio::_loadSpriteVizParamsFile(std::string fname, VizTuio::Region* r) {
	r->lastspritefilecheck = time(0);
	r->lastspritefileupdate = time(0);
	SpriteVizParams* p = getSpriteVizParams(fname);
	if (!p) {
		r->spriteparamfile = "";
		r->spriteparams = NULL;
		return false;
	}
	r->spriteparamfile = fname;
	r->spriteparams = p;
	return true;
}

std::string
VizTuio::_set_region_midiparams(Region* r, cJSON* json, const char* id)
{
	std::string file = jsonNeedString(json, "paramfile", "");
	if (file == "") {
		return jsonError(-32000, "Bad file value", id);
	}
	if (_loadMidiVizParamsFile(file, r)) {
		return jsonOK(id);
	}
	else {
		return jsonError(-32000, "Unable to load spriteparams file", id);
	}
}

bool VizTuio::_loadMidiVizParamsFile(std::string fname, VizTuio::Region* r) {
	r->lastmidifilecheck = time(0);
	r->lastmidifileupdate = time(0);
	MidiVizParams* p = getMidiVizParams(fname);
	if (!p) {
		r->midiparamfile = "";
		r->midiparams = NULL;
		return false;
	}
	r->midiparamfile = fname;
	r->midiparams = p;
	return true;
}