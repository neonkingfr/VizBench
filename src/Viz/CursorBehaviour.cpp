#include <string>
#include <set>
#include <ctime>
#include "VizMidi.h"
#include "VizJson.h"
#include "CursorBehaviour.h"
#include "Vizlet.h"

CursorBehaviour::CursorBehaviour(Vizlet* vizlet) {
	sid_min = 0;
	sid_max = MAX_SESSIONID;
	spriteparamfile = "default";
	midiparamfile = "default";
	spriteparams = NULL;
	midiparams = NULL;
	m_controllerval = 0;
	m_looping = false;
	m_vizlet = vizlet;
	m_autoloadparams = true;
}

void CursorBehaviour::_trackCursors(VizCursor* c, int downdragup) {
	// Keep track of cursors per-region
	switch (downdragup) {
	case CURSOR_DOWN:
		m_cursors.insert(c);
		DEBUGPRINT1(("track region cursors DOWN ncursors=%d",m_cursors.size()));
		break;
	case CURSOR_DRAG:
		// Make sure cursor is in our list.
		{
			auto fc = m_cursors.find(c);
			if (fc != m_cursors.end()) {
				const VizCursor* vc = *fc;
				VizAssert(vc == c);
				DEBUGPRINT1(("track region cursors DRAG ncursors=%d", m_cursors.size()));
			}
			else {
				DEBUGPRINT1(("track region cursors got CURSOR_DRAG without a matching cursor in m_cursors"));
			}
		}
		break;
	case CURSOR_UP:
		m_cursors.erase(c);
		DEBUGPRINT1(("track region cursors UP ncursors=%d", m_cursors.size()));
		break;
	}
}

bool CursorBehaviour::_inRegion(VizCursor* c) {
	return (c->sid >= sid_min && c->sid <= sid_max);
}

void CursorBehaviour::_reloadParams() {
	SpriteVizParams* p;
	if (!m_autoloadparams) {
		return;
	}
	p = m_vizlet->checkSpriteVizParamsAndLoadIfModifiedSince(spriteparamfile, lastspritefilecheck, lastspritefileupdate);
	if (p) {
		spriteparams = p;
	}
	MidiVizParams* m;
	m = m_vizlet->checkMidiVizParamsAndLoadIfModifiedSince(midiparamfile, lastmidifilecheck, lastmidifileupdate);
	if (m) {
		midiparams = m;
	}
}

std::string CursorBehaviour::_set_region_spriteparams(cJSON* json, const char* id) {
	std::string file = jsonNeedString(json, "paramfile", "");
	if (file == "") {
		return jsonError(-32000, "Bad file value", id);
	}
	if (_loadSpriteVizParamsFile(file)) {
		return jsonOK(id);
	}
	else {
		// To make it easier to create new spriteparams files,
		// a non-existent file is now not an error.
		// return jsonError(-32000, "Unable to load spriteparams file", id);
		return jsonOK(id);
	}
}

std::string CursorBehaviour::_set_looping(cJSON* json, const char* id)
{
	bool b = jsonNeedBool(json, "onoff", false);
	m_looping = b;
	if (!m_looping) {
		// Clear everything scheduled for this region
		m_vizlet->ScheduleClear();
	}
	DEBUGPRINT1(("Looping is set to %d", b));
	return jsonOK(id);
}

std::string CursorBehaviour::_set_sidrange(cJSON* json, const char* id)
{
	std::string sidrange = jsonNeedString(json, "sidrange", "");
	if (sidrange == "") {
		return jsonError(-32000, "Bad sidrange value", id);
	}
	int sidmin;
	int sidmax;
	int n = sscanf(sidrange.c_str(), "%d-%d", &sidmin, &sidmax);
	if (n == 1) {
		sid_min = sidmin;
		sid_max = sidmin;
	}
	else if (n == 2) {
		sid_min = sidmin;
		sid_max = sidmax;
	}
	else {
		return jsonError(-32000, "Unable to interpret sidrange value: %s", sidrange.c_str());
	}
	return jsonOK(id);
}

bool CursorBehaviour::_loadSpriteVizParamsFile(std::string fname) {
	lastspritefilecheck = time(0);
	lastspritefileupdate = time(0);
	SpriteVizParams* p = m_vizlet->readSpriteVizParams(fname, spriteparamfile);
	if (!p) {
		// A non-existent params file is now okay
		spriteparams = NULL;
		return false;
	}
	spriteparamfile = fname;
	spriteparams = p;
	return true;
}

std::string CursorBehaviour::_set_region_midiparams(cJSON* json, const char* id)
{
	std::string file = jsonNeedString(json, "paramfile", "");
	if (file == "") {
		return jsonError(-32000, "Bad file value", id);
	}
	if (_loadMidiVizParamsFile(file)) {
		return jsonOK(id);
	}
	else {
		return jsonError(-32000, "Unable to load spriteparams file", id);
	}
}

bool CursorBehaviour::_loadMidiVizParamsFile(std::string fname) {
	lastmidifilecheck = time(0);
	lastmidifileupdate = time(0);
	MidiVizParams* p = m_vizlet->readMidiVizParams(fname);
	if (!p) {
		midiparamfile = "";
		midiparams = NULL;
		return false;
	}
	midiparamfile = fname;
	midiparams = p;
	return true;
}

click_t CursorBehaviour::_loopClicks() {
	return (m_looping) ? midiparams->loopclicks.get() : 0;
}

void CursorBehaviour::_cursorMidi(VizCursor* c, int downdragup) {

	if (midiparams == NULL) {
		return;
	}

	if (midiparams->arpeggiate.get()) {
		_genArpeggiatedMidi(c, downdragup);
	}
	else {
		_genNormalMidi(c, downdragup);
	}

	//  DEBUGPRINT(("Controllers are forcibly ignored"));
	_genControlMidi(c, downdragup);

	// Go through cursors, compute average depth for controllervalue
	return;
}

void CursorBehaviour::_queueRegionMidiPhrase(MidiPhrase* ph, click_t clk, int cursorid) {
	DEBUGPRINT1(("queueRegionMidiPhrase clk=%ld",clk));
	m_vizlet->QueueMidiPhrase(ph, clk, cursorid, m_looping, midiparams);
}
int CursorBehaviour::_channelOf(VizCursor* c, MidiVizParams* mp) {
	return mp->channel;
}

int CursorBehaviour::_interpolate(double f, int minval, int maxval) {
	// The (dval+1) in the next expression is intended to make the
	// 0 to 1.0 range of f map evenly to the entire value range,
	// otherwise the maxval would only be generated for exactly 1.0.
	int dval = maxval - minval;
	int r = (int)(f * (dval + 1));
	if (r > dval) {
		r = dval;
	}
	return minval + r;
}

int CursorBehaviour::_pitchOf(VizCursor* c, MidiVizParams* mp) {

	int p = _interpolate(c->pos.x, mp->pitchmin, mp->pitchmax);
	if (mp->scale_on) {
		std::string scale = mp->scale.get();
		Scale s = Scale::Scales[scale];
		p = s.closestTo(p);
	}
	return p;
}

int CursorBehaviour::_velocityOf(VizCursor* c) {
	return 100;
}

click_t CursorBehaviour::_durationOf(VizCursor* c) {
	return m_vizlet->SchedulerClicksPerBeat() / 2;
}

click_t CursorBehaviour::_quantOf(VizCursor* c) {
	double f = 1.0;
	// Slower/larger quantization toward the bottom.
#define TIMEFRETS_4
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
	click_t q = (click_t)(f * m_vizlet->SchedulerClicksPerBeat());  // round?
	return q;
}

click_t CursorBehaviour::_quantizeToNext(click_t tm, click_t q) {
	return tm - (tm%q) + q;
}

void CursorBehaviour::_queueRegionMidiMsg(MidiMsg* m, click_t clk, int cursorid) {
	static click_t lastclk = -1;
	DEBUGPRINT1(("queueRegionMidiMsg clk=%ld type=%s",clk,m->MidiTypeName()));

	m_vizlet->QueueMidiMsg(m, clk, cursorid, m_looping, midiparams);

	DEBUGPRINT1(("_queueRegionMidiMsg  sid=%d  NumQueued=%d  NumScheduled=%d",
		cursorid,NumQueuedOfId(cursorid),NumScheduledOfId(cursorid) ));
	lastclk = clk;
}

void CursorBehaviour::_genArpeggiatedMidi(VizCursor* c, int downdragup) {

	DEBUGPRINT(("genArpeggiatedMidi"));
	MidiVizParams* mp = midiparams;
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

	_queueRegionMidiPhrase(ph, tm, c->cursorid);
}

click_t
CursorBehaviour::SchedulerCurrentClick() {
	return m_vizlet->SchedulerCurrentClick();
}

void CursorBehaviour::_genControlMidi(VizCursor* c, int downdragup) {

	MidiVizParams* mp = midiparams;

	if (mp->depthctlnum == 0) {
		DEBUGPRINT(("depthctlnum value is 0, no controller"));
		return;
	}

	if (downdragup == CURSOR_UP && m_cursors.size() == 0) {
		_genControllerReset(c);
		return;
	}

	// The value of z we want here is the average of all the cursors in the
	// region we're in.
	double z = 0.0;

	double totz = 0.0;
	int nz = 0;
	for (const auto &key : m_cursors) {
		const VizCursor* c = key;
		totz += c->pos.z;
		nz++;
	}
	double avgz = totz / nz;

	int newv = _interpolate(avgz, mp->depthctlmin.get(), mp->depthctlmax.get());
	newv = BoundValue(newv, 0, 127);
	DEBUGPRINT1(("sid=%d raw newv=%d", c->sid, newv));
	if (m_controllerval >= 0) {    // 
		// on subsequent times, smooth the new value with the old one
		int dv = (newv - m_controllerval);
		int newdv = dv / (mp->depthsmooth + 1);
		if (newdv == 0) {
			newdv = (dv > 0 ? 1 : -1);
		}
		newv = m_controllerval + newdv;
		DEBUGPRINT1(("sid=%d oldval=%d newdv=%d newv=%d",c->sid,m_controllerval,newdv,newv));
	}
	else {
		DEBUGPRINT1(("sid=%d first val=%d", c->sid, newv));
	}

	int outport = mp->port.get();
	int ch = _channelOf(c,mp);
	click_t now = SchedulerCurrentClick();

	MidiController* ctl = MidiController::make(ch, mp->depthctlnum, newv);
	ctl->SetOutputPort(outport);

	m_controllerval = newv;

	_queueRegionMidiMsg(ctl, now, c->cursorid);

	DEBUGPRINT1(("Queued sid=%d updated controllerval=%d",c->sid,c->m_controllerval));
	return;
}

// Reset controller value back to the minimum of its desired range (depthctlmin)
void CursorBehaviour::_genControllerReset(VizCursor* c) {

	MidiVizParams* mp = midiparams;

	int ch = _channelOf(c,mp);
	int outport = mp->port.get();
	click_t now = SchedulerCurrentClick();

	m_controllerval = (int)(mp->depthctlmin);
	MidiController* ctl = MidiController::make(ch, mp->depthctlnum, m_controllerval);
	ctl->SetOutputPort(outport);

	_queueRegionMidiMsg(ctl, now, c->cursorid);
}

int CursorBehaviour::_outputPort(MidiVizParams* mp) {
	int outport = mp->port.get();
	if (outport >= MAX_MIDI_PORTS) {
		throw VizException("port value (%d) is too large!?", outport);
	}
	return outport;
}

void CursorBehaviour::_queueNoteonWithNoteoffPending(VizCursor* c) {

	MidiVizParams* mp = midiparams;
	int ch = _channelOf(c,mp);
	int pitch = _pitchOf(c, mp);
	int vel = _velocityOf(c);
	int outport = _outputPort(mp);

	click_t now = SchedulerCurrentClick();
	click_t quant = _quantOf(c);
	click_t nowquant = _quantizeToNext(now, quant);

	// If there are any things queued for this cursor inbetween now and the time at which
	// we're going to schedule this note, remove them before queuing up anything more.
	m_vizlet->QueueRemoveBefore(c->cursorid, nowquant);

	MidiNoteOn *noteon = MidiNoteOn::make(ch, pitch, vel);
	noteon->SetInputPort(MIDI_PORT_OF_GENERATED_STUFF);
	noteon->SetOutputPort(outport);

	MidiNoteOff *noteoff = MidiNoteOff::make(ch, pitch, 0);
	noteoff->SetInputPort(MIDI_PORT_OF_GENERATED_STUFF);
	noteoff->SetOutputPort(outport);

	DEBUGPRINT1(("QUEUE noteon/noteoff cursor=%ld noteon=%ld noteoff=%ld", (long)c,(long)(noteon),(long)(noteoff)));

	// Send the noteon, but don't sent the noteoff until we get a CURSOR_UP

	click_t loopclicks = _loopClicks();
	_queueRegionMidiMsg(noteon, nowquant, c->cursorid);
	c->m_pending_noteoff = noteoff;
	c->m_noteon_click = nowquant;
	c->m_noteon_loopclicks = loopclicks;
	c->m_noteon_depth = c->pos.z;

	DEBUGPRINT1(("     QUEUE noteon at click=%d", nowquant));

}

void CursorBehaviour::_finishNote(VizCursor* c, click_t noteoff_click, int outport) {
	DEBUGPRINT1(("FINISHNOTE, using pending noteoff type=%s",c->m_pending_noteoff->MidiTypeName()));
	_queueRegionMidiMsg(c->m_pending_noteoff, noteoff_click, c->cursorid);
}

void CursorBehaviour::_genNormalMidi(VizCursor* c, int downdragup) {

	// In "NormalMidi" mode, the noteon/noteoff's are generated separately, so that
	// if you hold a cursor down, the note will stay on until you release the cursor
	// or move to a cursor position assigned to a different pitch.

	// DEBUGPRINT(("genNormalMidi"));

	MidiVizParams* mp = midiparams;
	bool looping = m_looping;

	int pitch = _pitchOf(c, mp);
	int outport = _outputPort(mp);

	click_t now = SchedulerCurrentClick();
	click_t quant = _quantOf(c);
	click_t nowquant = _quantizeToNext(now, quant);

	if (downdragup == CURSOR_DOWN) {
		if (c->m_pending_noteoff) {
			DEBUGPRINT(("Hey! m_pending_noteoff isn't NULL for CURSOR_DOWN? c=%ld noteoff=%ld",(long)c,(long)(c->m_pending_noteoff)));
		}
		_queueNoteonWithNoteoffPending(c);
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
				_finishNote(c, c->m_noteon_click + 1, outport);
				DEBUGPRINT1(("FINISH NOTE now=%d  nowquant=%d   noteon_click=%d",now,nowquant,c->m_noteon_click));

				// And then generate a new noteon with a pending noteoff
				_queueNoteonWithNoteoffPending(c);
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
			_finishNote(c, nowquant, outport);
			c->m_pending_noteoff = NULL;
		}
#if 0
		if (mp->depthctlnum.get() > 0) {
			c->m_controllerval = (int)(mp->depthctlmin);
			MidiController* ctl = MidiController::make(ch, mp->depthctlnum, c->m_controllerval);
			_queueRegionMidiMsg(ctl, now, c->cursorid);
			DEBUGPRINT1(("Queued ctl val=%d", c->m_controllerval));
			return;
		}
#endif
	}
	else {
		DEBUGPRINT(("Invalid value for downdragup = %d !?", downdragup));
	}
}

void CursorBehaviour::_cursorSprite(VizCursor* c, int downdragup) {

	SpriteVizParams* sp = spriteparams;
	if (sp == NULL) {
		return;
	}
	DEBUGPRINT1(("cursorSprite! sid=%d xyz = %.5f %.5f %.5f", c->sid, c->pos.x, c->pos.y, c->pos.z));

	VizPos pos = c->pos;
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
	m_vizlet->makeAndAddVizSprite(sp, pos);
}