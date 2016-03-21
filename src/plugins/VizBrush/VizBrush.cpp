#include "Vizlet.h"
#include "VizBrush.h"

static CFFGLPluginInfo PluginInfo ( 
	VizBrush::CreateInstance,	// Create method
	"V955",		// Plugin unique ID
	"VizBrush",	// Plugin name	
	1,			// API major version number
	000,		// API minor version number	
	1,			// Plugin major version number
	000,		// Plugin minor version number
	FF_EFFECT,	// Plugin type
	"VizBrush: a sample visual synth",	// description
	"by Tim Thompson - me@timthompson.com" 			// About
);

std::string vizlet_name() { return "VizBrush"; }
CFFGLPluginInfo& vizlet_plugininfo() { return PluginInfo; }
void vizlet_setdll(std::string dll) { }

VizBrush::VizBrush() : Vizlet() {
	DEBUGPRINT(("VizBrush is being created and initialized! this=%ld",(long)this));
	_region = new Region();
	_autoloadparams = true;
}

VizBrush::~VizBrush() {
}

DWORD __stdcall VizBrush::CreateInstance(CFreeFrameGLPlugin **ppInstance) {
	*ppInstance = new VizBrush();
	return (*ppInstance != NULL)? FF_SUCCESS : FF_FAIL;
}

void VizBrush::_trackCursorsPerRegion(VizCursor* c, int downdragup, Region* r) {
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

bool
VizBrush::_inRegion(VizCursor* c, Region* r) {
	return (c->sid >= r->sid_min && c->sid <= r->sid_max);
}

void VizBrush::processCursor(VizCursor* c, int downdragup) {
	// NO OpenGL calls here
	DEBUGPRINT1(("VizBrush::processCursor! downdragup=%d c=%.4f %.4f",downdragup,c->pos.x,c->pos.y));

	if ( _inRegion(c,_region) ) {
		_trackCursorsPerRegion(c, downdragup, _region);
		_cursorSprite(c, downdragup, _region);
		_cursorMidi(c, downdragup, _region);
	}
}

std::string VizBrush::processJson(std::string meth, cJSON *json, const char *id) {
	// NO OpenGL calls here

	DEBUGPRINT1(("VizBrush::processJson brush=%ld meth=%s",(long)this,meth.c_str()));

	if (meth == "apis") {
		std::string apis = "";
		// We don't show set_sidrange, because we don't want it to show up in any auto-generated UIs
		apis += "set_sprite(paramfile);";
		apis += "set_midi(paramfile);";
		apis += "set_looping(onoff);";
		apis += "set_autoloadparams(onoff);";
		apis += "testapi();";
		return jsonStringResult(apis, id);
	}

	if (meth == "testapi") {
		DEBUGPRINT(("VizBrush.testapi called!"));
		return jsonOK(id);
	}

	if (meth == "dump") {
		std::string dump = "[";

		dump += NosuchSnprintf("{\"method\":\"set_sprite\",\"params\":{\"paramfile\":\"%s\"}}", _region->spriteparamfile.c_str());
		dump += NosuchSnprintf(",{\"method\":\"set_midi\",\"params\":{\"paramfile\":\"%s\"}}", _region->midiparamfile.c_str());
		dump += NosuchSnprintf(",{\"method\":\"set_looping\",\"params\":{\"onoff\":\"%d\"}}", _region->m_looping);

		// Don't dump the sidrange value - it's set from FFF via API
		// dump += NosuchSnprintf(",{\"method\":\"set_sidrange\",\"params\":{\"sidrange\":\"%d-%d\"}}", _region->sid_min, _region->sid_max);

		dump += "]";
		return jsonStringResult(dump, id);
	}

	// Here we go through all the region names and look for their set/get methods
	std::string result;

	if (meth == "set_sidrange") {
		result = _set_sidrange(_region, json, id);
	}
	if (meth == "get_sidrange") {
		result = jsonStringResult(NosuchSnprintf("%d-%d", _region->sid_min, _region->sid_max), id);
	}

	if (meth == "set_sprite") {
		result = _set_region_spriteparams(_region, json, id);
	}
	if (meth == "get_sprite") {
		result = jsonStringResult(_region->spriteparamfile, id);
	}

	if (meth == "set_midi") { result = _set_region_midiparams(_region, json, id); }
	if (meth == "get_midi") { result = jsonStringResult(_region->midiparamfile, id); }

	if (meth == "set_looping") { result = _set_looping(_region, json, id); }
	if (meth == "get_looping") { result = jsonIntResult(_region->m_looping, id); }

	if (result != "") {
		const char *p = result.c_str();
		if (p[0] != '{') {
			DEBUGPRINT(("Bad result in meth=%s result=%s", meth.c_str(), p));
		}
		return result;
	}

	// PARAMETER "autoloadparams"
	if (meth == "set_autoloadparams") {
		_autoloadparams = jsonNeedBool(json, "onoff", false);
		return jsonOK(id);
	}
	if (meth == "get_autoloadparams") {
		return jsonIntResult(_autoloadparams ? 1 : 0, id);
	}

	throw NosuchException("VizBrush - Unrecognized method '%s'",meth.c_str());
}

void
VizBrush::_reloadParams(Region* r) {
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

bool VizBrush::processDraw() {
	// OpenGL calls here
	if (_autoloadparams) {
		_reloadParams(_region);
	}
	DrawVizSprites();
	return true;
}

int VizBrush::_channelOf(VizCursor* c, MidiVizParams* mp) {
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

int VizBrush::_pitchOf(VizCursor* c, MidiVizParams* mp) {

	int p = _interpolate(c->pos.x, mp->pitchmin, mp->pitchmax);
	if (mp->scale_on) {
		std::string scale = mp->scale.get();
		Scale s = Scale::Scales[scale];
		p = s.closestTo(p);
	}
	return p;
}

int VizBrush::_velocityOf(VizCursor* c) {
	return 100;
}

click_t VizBrush::_durationOf(VizCursor* c) {
	return SchedulerClicksPerBeat()/2;
}

click_t VizBrush::_quantOf(VizCursor* c) {
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

click_t VizBrush::_quantizeToNext(click_t tm, click_t q) {
	return tm - (tm%q) + q;
}

click_t
VizBrush::_loopClicks(Region* r) {
	return (r->m_looping) ? r->midiparams->loopclicks.get() : 0;
}

void VizBrush::_cursorMidi(VizCursor* c, int downdragup, Region* r) {

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
VizBrush::_queueRegionMidiPhrase(MidiPhrase* ph, click_t tm, int cursorid, Region* r) {
	NosuchAssert(r);
	QueueMidiPhrase(ph, tm, cursorid, r->m_looping, r->midiparams);
}

void
VizBrush::_queueRegionMidiMsg(MidiMsg* m, click_t tm, int cursorid, Region* r) {
	NosuchAssert(r);
	QueueMidiMsg(m, tm, cursorid, r->m_looping, r->midiparams);
}

void VizBrush::_genArpeggiatedMidi(VizCursor* c, int downdragup, Region* r) {

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

void VizBrush::_genControlMidi(VizCursor* c, int downdragup, Region* r) {

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
VizBrush::_genControllerReset(VizCursor* c, Region* r) {

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
VizBrush::_queueNoteonWithNoteoffPending(VizCursor* c, Region* r) {

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
VizBrush::_finishNote(VizCursor* c, click_t noteoff_click, int outport, Region* r) {
	DEBUGPRINT1(("FINISHNOTE, using pending noteoff type=%s",c->m_pending_noteoff->MidiTypeName()));
	_queueRegionMidiMsg(c->m_pending_noteoff, noteoff_click, c->cursorid, r);
}

void VizBrush::_genNormalMidi(VizCursor* c, int downdragup, Region* r) {

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

void VizBrush::_cursorSprite(VizCursor* c, int downdragup, Region* r) {

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
VizBrush::_set_region_spriteparams(Region* r, cJSON* json, const char* id)
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
VizBrush::_set_looping(VizBrush::Region* r, cJSON* json, const char* id)
{
	bool b = jsonNeedBool(json, "onoff", false);
	r->m_looping = b;
	if (!r->m_looping) {
		// Clear everything scheduled for this region
		ScheduleClear();
	}
	DEBUGPRINT1(("Looping is set to %d",b));
	return jsonOK(id);
}

std::string
VizBrush::_set_sidrange(SidRangeable* b, cJSON* json, const char* id)
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

bool VizBrush::_loadSpriteVizParamsFile(std::string fname, VizBrush::Region* r) {
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
VizBrush::_set_region_midiparams(Region* r, cJSON* json, const char* id)
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

bool VizBrush::_loadMidiVizParamsFile(std::string fname, VizBrush::Region* r) {
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
