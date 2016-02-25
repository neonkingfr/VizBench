#include "Vizlet.h"
#include "VizPuddle.h"

static CFFGLPluginInfo PluginInfo ( 
	VizPuddle::CreateInstance,	// Create method
	"V018",		// Plugin unique ID
	"VizPuddle",	// Plugin name	
	1,			// API major version number
	000,		// API minor version number	
	1,			// Plugin major version number
	000,		// Plugin minor version number
	FF_EFFECT,	// Plugin type
	"VizPuddle: a sample visual synth",	// description
	"by Tim Thompson - me@timthompson.com" 			// About
);

std::string vizlet_name() { return "VizPuddle"; }
CFFGLPluginInfo& vizlet_plugininfo() { return PluginInfo; }
void vizlet_setdll(std::string dll) { }

VizPuddle::VizPuddle() : Vizlet() {

	DEBUGPRINT(("VizPuddle is being created and initialized! this=%ld",(long)this));

	const char* regionnames[] = { "pad1", "pad2", "pad3", "pad4", NULL };
	const char* buttonnames[] = {	"UL1", "UL2", "UL3", "UR1", "UR2", "UR3",
									"LL1", "LL2", "LL3", "LR1", "LR2", "LR3",
									NULL
									};

	const char* s;

	for (int i = 0; (s=regionnames[i])!=NULL; i++ ) {
		_region[s] = new Region(s);
	}
	for (int i = 0; (s = buttonnames[i]) != NULL; i++) {
		_button[s] = new Button(s);
	}
	_autoloadparams = true;
}

VizPuddle::~VizPuddle() {
}

DWORD __stdcall VizPuddle::CreateInstance(CFreeFrameGLPlugin **ppInstance) {
	*ppInstance = new VizPuddle();
	return (*ppInstance != NULL)? FF_SUCCESS : FF_FAIL;
}

void VizPuddle::_trackCursorsPerRegion(VizCursor* c, int downdragup, Region* r) {
	// Keep track of cursors per-region
	switch (downdragup) {
	case CURSOR_DOWN:
		r->m_cursors.insert(c);
		DEBUGPRINT1(("track region cursors DOWN ncursors=%d",r->m_cursors.size()));
		break;
	case CURSOR_DRAG:
		// Make sure cursor is in our list.
		{
			auto fc = r->m_cursors.find(c);
			if (fc != r->m_cursors.end()) {
				VizCursor* vc = *fc;
				NosuchAssert(vc == c);
				DEBUGPRINT1(("track region cursors DRAG ncursors=%d", r->m_cursors.size()));
			}
			else {
				DEBUGPRINT1(("track region cursors got CURSOR_DRAG without a matching cursor in m_cursors"));
			}
		}
		break;
	case CURSOR_UP:
		r->m_cursors.erase(c);
		DEBUGPRINT1(("track region cursors UP ncursors=%d", r->m_cursors.size()));
		break;
	}
}

VizPuddle::Region*
VizPuddle::_findRegion(VizCursor* c) {
	int sid = c->sid;
	for (const auto &pair : _region) {
		Region* r = pair.second;
		if (sid >= r->sid_min && sid <= r->sid_max) {
			return r;
		}
	}
	return NULL;
}

void VizPuddle::processCursor(VizCursor* c, int downdragup) {
	// NO OpenGL calls here
	DEBUGPRINT1(("VizPuddle::processCursor! downdragup=%d c=%.4f %.4f",downdragup,c->pos.x,c->pos.y));
	// palette()->processCursor(c,downdragup);
	if (downdragup == CURSOR_DOWN) {
#ifdef WHEN_ON_SPACE_PALETTE
		int sid = c->sid;
		std::string pipeline = "";
		// See if it's a button
		for (const auto &pair : _button ) {
			if (sid >= pair.second->sid_min && sid <= pair.second->sid_max) {
				pipeline = pair.second->pipeline;
				break;
			}
		}
		if (pipeline != "") {
			LoadPipeline(pipeline);
		}
#endif
	}

	// Look through the regions to find the one that matches the cursor's sid
	Region* r = _findRegion(c);
	if ( r ) {
		_trackCursorsPerRegion(c, downdragup, r);
		_cursorSprite(c, downdragup, r);
		_cursorMidi(c, downdragup, r);
	}
	// For the moment, we only pay attention to the first region that matches the sid.
	// This might change (just remove the break, here).
}

std::string VizPuddle::processJson(std::string meth, cJSON *json, const char *id) {
	// NO OpenGL calls here

	DEBUGPRINT1(("VizPuddle::processJson puddle=%ld meth=%s",(long)this,meth.c_str()));
	if (meth == "apis") {
		std::string apis = "";
		for (const auto &pair : _region) {
			std::string nm = pair.first;
				apis += "set_" + nm + "_sidrange(sidrange);";
				apis += "set_" + nm + "_sprite(paramfile);";
				apis += "set_" + nm + "_midi(paramfile);";
				apis += "set_" + nm + "_looping(onoff);";
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
		DEBUGPRINT(("VizPuddle.testapi called!"));
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
			dump += sep + NosuchSnprintf("{\"method\":\"set_%s_looping\",\"params\":{\"onoff\":\"%d\"}}", pair.first.c_str(), pair.second->m_looping);
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

		std::string result;

		if (meth == "set_" + nm + "_sidrange") { result = _set_sidrange(r, json, id); }
		if (meth == "get_" + nm + "_sidrange") { result = jsonStringResult(NosuchSnprintf("%d-%d", r->sid_min, r->sid_max), id); }

		if (meth == "set_" + nm + "_sprite") { result = _set_region_spriteparams(r, json, id); }
		if (meth == "get_" + nm + "_sprite") { result = jsonStringResult(r->spriteparamfile, id); }

		if (meth == "set_" + nm + "_midi") { result = _set_region_midiparams(r, json, id); }
		if (meth == "get_" + nm + "_midi") { result = jsonStringResult(r->midiparamfile, id); }

		if (meth == "set_" + nm + "_looping") { result = _set_looping(r, json, id); }
		if (meth == "get_" + nm + "_looping") { result = jsonIntResult(r->m_looping, id); }

		if (result != "") {
			const char *p = result.c_str();
			if (p[0] != '{') {
				DEBUGPRINT(("Bad result in meth=%s result=%s", meth.c_str(), p));
			}
			return result;
		}
	}

	// Here we go through all the button names and look for their set/get methods
	for (const auto &pair : _button) {
		std::string nm = pair.first;
		Button* b = pair.second;

		std::string result;

		if (meth == "set_" + nm + "_sidrange") { result = _set_sidrange(b, json, id); }
		if (meth == "get_" + nm + "_sidrange") { result = jsonStringResult(NosuchSnprintf("%d-%d", b->sid_min, b->sid_max), id); }

		if (meth == "set_" + nm + "_pipeline") { result = _set_button_pipeline(b, json, id); }
		if (meth == "get_" + nm + "_pipeline") { result = jsonStringResult(b->pipeline, id); }

		if (result != "") {
			const char *p = result.c_str();
			if (p[0] != '{') {
				DEBUGPRINT(("Bad result in meth=%s result=%s", meth.c_str(), p));
			}
			return result;
		}
	}

	// PARAMETER "autoloadparams"
	if (meth == "set_autoloadparams") {
		_autoloadparams = jsonNeedBool(json, "onoff", false);
		return jsonOK(id);
	}
	if (meth == "get_autoloadparams") {
		return jsonIntResult(_autoloadparams ? 1 : 0, id);
	}

	throw NosuchException("VizPuddle - Unrecognized method '%s'",meth.c_str());
}

void
VizPuddle::_reloadParams(Region* r) {
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

bool VizPuddle::processDraw() {
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

int VizPuddle::_channelOf(VizCursor* c, MidiVizParams* mp) {
	return mp->channel;
}

int _interpolate(double f, int minval, int maxval) {
	// The (dval+1) in the next expression is intended to make the
	// 0 to 1.0 range of f map evenly to the entire value range,
	// otherwise the maxval would only be generated for exactly 1.0.
	int dval = maxval - minval;
	int r = (int)(f * (dval+1));
	if (r > dval) {
		r = dval;
	}
	return minval + r;
}

int VizPuddle::_pitchOf(VizCursor* c, MidiVizParams* mp) {

	int p = _interpolate(c->pos.x, mp->pitchmin, mp->pitchmax);
	if (mp->scale_on) {
		std::string scale = mp->scale.get();
		Scale s = Scale::Scales[scale];
		p = s.closestTo(p);
	}
	return p;
}

int VizPuddle::_velocityOf(VizCursor* c) {
	return 100;
}

click_t VizPuddle::_durationOf(VizCursor* c) {
	return SchedulerClicksPerBeat()/2;
}

click_t VizPuddle::_quantOf(VizCursor* c) {
	double f = 1.0;
	// Slower/larger quantization toward the bottom.
#define TIMEFRETS_3
#ifdef TIMEFRETS_4
	if (c->pos.y < 0.25) {
		f = 1.0;
	}
	else if (c->pos.y < 0.5) {
		f = 0.5;
	}
	else if (c->pos.y < 0.75) {
		f = 0.25;
	}
	else {
		f = 0.125;
	}
#endif
#ifdef TIMEFRETS_3
	if (c->pos.y < 0.33) {
		f = 0.5;
	}
	else if (c->pos.y < 0.66) {
		f = 0.25;
	}
	else {
		f = 0.125;
	}
#endif
	click_t q = (click_t)(f * SchedulerClicksPerBeat());  // round?
	return q;
}

click_t VizPuddle::_quantizeToNext(click_t tm, click_t q) {
	return tm - (tm%q) + q;
}

click_t
VizPuddle::_loopClicks(Region* r) {
	return (r->m_looping) ? r->midiparams->loopclicks.get() : 0;
}

void VizPuddle::_cursorMidi(VizCursor* c, int downdragup, Region* r) {

	if (r->midiparams == NULL) {
		return;
	}

	if (r->midiparams->arpeggiate.get()) {
		_genArpeggiatedMidi(c, downdragup, r);
	}
	else {
		_genNormalMidi(c, downdragup, r);
	}

	//  DEBUGPRINT(("Controllers are forcibly ignored"));
	_genControlMidi(c, downdragup, r);

	// Go through cursors, compute average depth for controllervalue
	return;
}

void
VizPuddle::_queueRegionMidiPhrase(MidiPhrase* ph, click_t tm, int cursorid, Region* r) {
	NosuchAssert(r);
	QueueMidiPhrase(ph, tm, cursorid, r->m_looping, r->midiparams);
}

void
VizPuddle::_queueRegionMidiMsg(MidiMsg* m, click_t tm, int cursorid, Region* r) {
	NosuchAssert(r);
	QueueMidiMsg(m, tm, cursorid, r->m_looping, r->midiparams);
}

void VizPuddle::_genArpeggiatedMidi(VizCursor* c, int downdragup, Region* r) {

	MidiVizParams* mp = r->midiparams;
	MidiPhrase *ph = new MidiPhrase();
	int ch = _channelOf(c,mp);
	int pitch = _pitchOf(c,mp);
	int vel = _velocityOf(c);
	click_t dur = _durationOf(c);
	click_t quant = _quantOf(c);
	click_t now = SchedulerCurrentClick();
	int outport = mp->port.get();

	MidiNoteOn *noteon = MidiNoteOn::make(ch, pitch, vel);
	MidiNoteOff *noteoff = MidiNoteOff::make(ch, pitch, 0);
	ph->insert(noteon, 0);
	ph->insert(noteoff, dur);
	ph->SetInputPort(MIDI_PORT_OF_GENERATED_STUFF);
	ph->SetOutputPort(outport);
	click_t tm = _quantizeToNext(now, quant);

	_queueRegionMidiPhrase(ph, tm, c->cursorid, r);
}

void VizPuddle::_genControlMidi(VizCursor* c, int downdragup, Region* r) {

	MidiVizParams* mp = r->midiparams;

	if (mp->depthctlnum == 0) {
		DEBUGPRINT(("depthctlnum value is 0, no controller"));
		return;
	}

	if (downdragup == CURSOR_UP && r->m_cursors.size() == 0) {
		_genControllerReset(c, r);
		return;
	}

	// The value of z we want here is the average of all the cursors in the
	// region we're in.
	double z = 0.0;

	double totz = 0.0;
	int nz = 0;
	for (const auto &key : r->m_cursors) {
		VizCursor* c = key;
		totz += c->pos.z;
		nz++;
	}
	double avgz = totz / nz;

	int newv = _interpolate(avgz, mp->depthctlmin.get(), mp->depthctlmax.get());
	newv = BoundValue(newv, 0, 127);
	DEBUGPRINT1(("sid=%d raw newv=%d", c->sid, newv));
	if (r->m_controllerval >= 0) {    // 
		// on subsequent times, smooth the new value with the old one
		int dv = (newv - r->m_controllerval);
		int newdv = dv / (mp->depthsmooth + 1);
		if (newdv == 0) {
			newdv = (dv > 0 ? 1 : -1);
		}
		newv = r->m_controllerval + newdv;
		DEBUGPRINT1(("sid=%d oldval=%d newdv=%d newv=%d",c->sid,r->m_controllerval,newdv,newv));
	}
	else {
		DEBUGPRINT1(("sid=%d first val=%d", c->sid, newv));
	}

	int outport = mp->port.get();
	int ch = _channelOf(c,mp);
	click_t now = SchedulerCurrentClick();

	MidiController* ctl = MidiController::make(ch, mp->depthctlnum, newv);
	ctl->SetOutputPort(outport);

	r->m_controllerval = newv;

	_queueRegionMidiMsg(ctl, now, c->cursorid, r);

	DEBUGPRINT1(("Queued sid=%d updated controllerval=%d",c->sid,c->m_controllerval));
	return;
}

// Reset controller value back to the minimum of its desired range (depthctlmin)
void
VizPuddle::_genControllerReset(VizCursor* c, Region* r) {

	MidiVizParams* mp = r->midiparams;

	int ch = _channelOf(c,mp);
	int outport = mp->port.get();
	click_t now = SchedulerCurrentClick();

	r->m_controllerval = (int)(mp->depthctlmin);
	MidiController* ctl = MidiController::make(ch, mp->depthctlnum, r->m_controllerval);
	ctl->SetOutputPort(outport);

	_queueRegionMidiMsg(ctl, now, c->cursorid, r);
}

int _outputPort(MidiVizParams* mp) {
	int outport = mp->port.get();
	if (outport >= MAX_MIDI_PORTS) {
		throw NosuchException("port value (%d) is too large!?", outport);
	}
	return outport;
}

void
VizPuddle::_queueNoteonWithNoteoffPending(VizCursor* c, Region* r) {

	MidiVizParams* mp = r->midiparams;
	int ch = _channelOf(c,mp);
	int pitch = _pitchOf(c, mp);
	int vel = _velocityOf(c);
	int outport = _outputPort(mp);

	click_t now = SchedulerCurrentClick();
	click_t quant = _quantOf(c);
	click_t nowquant = _quantizeToNext(now, quant);

	MidiNoteOn *noteon = MidiNoteOn::make(ch, pitch, vel);
	noteon->SetInputPort(MIDI_PORT_OF_GENERATED_STUFF);
	noteon->SetOutputPort(outport);

	MidiNoteOff *noteoff = MidiNoteOff::make(ch, pitch, 0);
	noteoff->SetInputPort(MIDI_PORT_OF_GENERATED_STUFF);
	noteoff->SetOutputPort(outport);

	DEBUGPRINT1(("QUEUE noteon/noteoff cursor=%ld noteon=%ld noteoff=%ld", (long)c,(long)(noteon),(long)(noteoff)));

	// Send the noteon, but don't sent the noteoff until we get a CURSOR_UP

	click_t loopclicks = _loopClicks(r);
	_queueRegionMidiMsg(noteon, nowquant, c->cursorid, r);
	c->m_pending_noteoff = noteoff;
	c->m_noteon_click = nowquant;
	c->m_noteon_loopclicks = loopclicks;
	c->m_noteon_depth = c->pos.z;

	DEBUGPRINT1(("     QUEUE noteon at click=%d", nowquant));

}

void
VizPuddle::_finishNote(VizCursor* c, click_t noteoff_click, int outport, Region* r) {
	DEBUGPRINT1(("FINISHNOTE, using pending noteoff type=%s",c->m_pending_noteoff->MidiTypeName()));
	_queueRegionMidiMsg(c->m_pending_noteoff, noteoff_click, c->cursorid, r);
}

void VizPuddle::_genNormalMidi(VizCursor* c, int downdragup, Region* r) {

	// In "NormalMidi" mode, the noteon/noteoff's are generated separately, so that
	// if you hold a cursor down, the note will stay on until you release the cursor
	// or move to a cursor position assigned to a different pitch.

	MidiVizParams* mp = r->midiparams;
	bool looping = r->m_looping;

	int pitch = _pitchOf(c, mp);
	int outport = _outputPort(mp);

	click_t now = SchedulerCurrentClick();
	click_t quant = _quantOf(c);
	click_t nowquant = _quantizeToNext(now, quant);

	if (downdragup == CURSOR_DOWN) {
		if (c->m_pending_noteoff) {
			DEBUGPRINT(("Hey! m_pending_noteoff isn't NULL for CURSOR_DOWN? c=%ld noteoff=%ld",(long)c,(long)(c->m_pending_noteoff)));
		}
		_queueNoteonWithNoteoffPending(c, r);
	}

	else if (downdragup == CURSOR_DRAG) {
		if (c->m_pending_noteoff == NULL) {
			DEBUGPRINT(("No pending_noteoff in CURSOR_DRAG!?"));
			// but keep going, to generate noteon
		}
		else {
			int dpitch = (pitch - c->m_pending_noteoff->Pitch());
			double dz = abs(c->pos.z - c->m_noteon_depth);
			if (mp->depthretrigger_on == 0) {
				dz = 0.0;
			}
			if (dpitch != 0 || dz > mp->depthretrigger_thresh.get()) {
				// When the new pitch is different, terminate the current note.
				// Schedule the noteoff to make sure it happens after the noteon,
				// which might not have been sent yet.
				_finishNote(c, c->m_noteon_click + 1, outport, r);

				// And then generate a new noteon with a pending noteoff
				_queueNoteonWithNoteoffPending(c, r);
			}
		}
	}

	else if (downdragup == CURSOR_UP) {

		if (c->m_pending_noteoff == NULL) {
			DEBUGPRINT(("Hey, no m_pending_noteoff for CURSOR_UP!?"));
		}
		else {
			// If noteoff is not quantized, it may preceded noteon, resulting in stuck note
			DEBUGPRINT1(("CURSOR_UP is calling finishNote !!"));
			_finishNote(c, nowquant, outport, r);
			c->m_pending_noteoff = NULL;
		}
#if 0
		if (mp->depthctlnum.get() > 0) {
			c->m_controllerval = (int)(mp->depthctlmin);
			MidiController* ctl = MidiController::make(ch, mp->depthctlnum, c->m_controllerval);
			_queueRegionMidiMsg(ctl, now, c->cursorid, r);
			DEBUGPRINT1(("Queued ctl val=%d", c->m_controllerval));
			return;
		}
#endif
	}
	else {
		DEBUGPRINT(("Invalid value for downdragup = %d !?", downdragup));
	}
}

void VizPuddle::_cursorSprite(VizCursor* c, int downdragup, Region* r) {

	SpriteVizParams* sp = r->spriteparams;
	if (sp == NULL) {
		return;
	}
	DEBUGPRINT1(("cursorSprite! sid=%d xyz = %.5f %.5f %.5f", c->sid, c->pos.x, c->pos.y, c->pos.z));

	NosuchPos pos = c->pos;
	double movedir = 0.0;
	bool randpos = false;
	if (randpos) {
		pos.x = pos.y = 0.5;
		// pos.z = (m->Velocity()*m->Velocity()) / (128.0*128.0);
		movedir = (double)(rand() % 360);
		sp->movedir.set(movedir);
	}
	else {
		movedir = sp->movedir.get();
	}

	DEBUGPRINT2(("_cursorSprite spriteparams=%ld shape=%s", (long)sp, sp->shape.get().c_str()));
	makeAndAddVizSprite(sp, pos);
}

std::string
VizPuddle::_set_region_spriteparams(Region* r, cJSON* json, const char* id)
{
	std::string file = jsonNeedString(json, "paramfile", "");
	if (file == "") {
		return jsonError(-32000, "Bad file value", id);
	}
	if (_loadSpriteVizParamsFile(file, r)) {
		return jsonOK(id);
	}
	else {
		// To make it easier to create new spriteparams files,
		// a non-existent file is now not an error.
		// return jsonError(-32000, "Unable to load spriteparams file", id);
		return jsonOK(id);
	}
}

std::string
VizPuddle::_set_looping(VizPuddle::Region* r, cJSON* json, const char* id)
{
	bool b = jsonNeedBool(json, "onoff", false);
	r->m_looping = b;
	if (!r->m_looping) {
		// Clear everything scheduled for this region
		ScheduleClear();
	}
	DEBUGPRINT(("Looping for pad %s is set to %d",r->name.c_str(),b));
	return jsonOK(id);
}

std::string
VizPuddle::_set_sidrange(SidRangeable* b, cJSON* json, const char* id)
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
VizPuddle::_set_button_pipeline(Button* b, cJSON* json, const char* id)
{
	std::string pipeline = jsonNeedString(json, "pipeline", "");
	if (pipeline == "") {
		return jsonError(-32000, "Bad pipeline value", id);
	}
	b->pipeline = pipeline;
	return jsonOK(id);
}

bool VizPuddle::_loadSpriteVizParamsFile(std::string fname, VizPuddle::Region* r) {
	r->lastspritefilecheck = time(0);
	r->lastspritefileupdate = time(0);
	SpriteVizParams* p = readSpriteVizParams(fname);
	if (!p) {
		// A non-existent params file is now okay
		// r->spriteparamfile = "";
		r->spriteparams = NULL;
		return false;
	}
	r->spriteparamfile = fname;
	r->spriteparams = p;
	return true;
}

std::string
VizPuddle::_set_region_midiparams(Region* r, cJSON* json, const char* id)
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

bool VizPuddle::_loadMidiVizParamsFile(std::string fname, VizPuddle::Region* r) {
	r->lastmidifilecheck = time(0);
	r->lastmidifileupdate = time(0);
	MidiVizParams* p = readMidiVizParams(fname);
	if (!p) {
		r->midiparamfile = "";
		r->midiparams = NULL;
		return false;
	}
	r->midiparamfile = fname;
	r->midiparams = p;
	return true;
}