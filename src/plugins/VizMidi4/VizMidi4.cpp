#include "Vizlet.h"
#include "VizMidi4.h"

static CFFGLPluginInfo PluginInfo ( 
	VizMidi4::CreateInstance,	// Create method
	"VZMD",		// Plugin unique ID
	"VizMidi4",	// Plugin name	
	1,			// API major version number
	000,		// API minor version number	
	1,			// Plugin major version number
	000,		// Plugin minor version number
	FF_EFFECT,	// Plugin type
	"VizMidi4: a sample visual synth",	// description
	"by Tim Thompson - me@timthompson.com" 			// About
);

std::string vizlet_name() { return "VizMidi4"; }
CFFGLPluginInfo& vizlet_plugininfo() { return PluginInfo; }

VizMidi4::VizMidi4() : Vizlet() {

	for (int n = 0; n < 4; n++) {
		m_sprite_on[n].channel = n + 1;
		_loadSpriteVizParamsFile(NosuchSnprintf("patch_1_%c_on","ABCD"[n]), m_sprite_on[n]);
		m_sprite_off[n].channel = n + 1;
		_loadSpriteVizParamsFile(NosuchSnprintf("patch_1_%c_off","ABCD"[n]), m_sprite_off[n]);
	}
	m_autoloadparams = true;
}

VizMidi4::~VizMidi4() {
	DEBUGPRINT1(("VizMidi4 DESTRUCTOR!!"));
}

DWORD __stdcall VizMidi4::CreateInstance(CFreeFrameGLPlugin **ppInstance) {
	*ppInstance = new VizMidi4();
	DEBUGPRINT1(("CreateInstance of VizMidi4 = %ld", (long)(*ppInstance)));
	return (*ppInstance != NULL)? FF_SUCCESS : FF_FAIL;
}

void VizMidi4::processCursor(VizCursor* c, int downdragup) {
	// NO OpenGL calls here
}

bool VizMidi4::_loadSpriteVizParamsFile(std::string fname, VizMidi4::paramsfile_info& spriteinfo) {
	spriteinfo.lastfilecheck = time(0);
	spriteinfo.lastfileupdate = time(0);
	SpriteVizParams* p = getSpriteVizParams(fname);
	if (!p) {
		spriteinfo.paramsfname = "";
		spriteinfo.params = NULL;
		return false;
	}
	spriteinfo.paramsfname = fname;
	spriteinfo.params = p;
	return true;
}

std::string
VizMidi4::_set_params_on(int n, cJSON* json, const char* id)
{
	return _set_params(m_sprite_on[n],json,id);
}

std::string
VizMidi4::_set_params_off(int n, cJSON* json, const char* id)
{
	return _set_params(m_sprite_off[n],json,id);
}

std::string
VizMidi4::_set_params(paramsfile_info& spriteinfo, cJSON* json, const char* id)
{
	std::string file = jsonNeedString(json, "paramfile", "");
	if (file == "") {
		return jsonError(-32000, "Bad file value", id);
	}
	if (_loadSpriteVizParamsFile(file, spriteinfo)) {
		return jsonOK(id);
	}
	else {
		return jsonError(-32000, "Unable to load spriteparams file", id);
	}
}

std::string VizMidi4::processJson(std::string meth, cJSON *json, const char *id) {
	// NO OpenGL calls here

	if (meth == "apis") {
		return jsonStringResult(
			"set_sprite_on_A(paramfile);"
			"set_sprite_on_B(paramfile);"
			"set_sprite_on_C(paramfile);"
			"set_sprite_on_D(paramfile);"
			"set_sprite_off_A(paramfile);"
			"set_sprite_off_B(paramfile);"
			"set_sprite_off_C(paramfile);"
			"set_sprite_off_D(paramfile);"
			"set_channel_A(channel);"
			"set_channel_B(channel);"
			"set_channel_C(channel);"
			"set_channel_D(channel);"
			"set_autoloadparams(onoff);"
			"testapi();"
			, id);
	}
	if (meth == "testapi") {
		DEBUGPRINT(("VizMidi4.testapi called!"));
		return jsonOK(id);
	}
	if (meth == "dump") {
		std::string dump =
			"["
			+NosuchSnprintf("{\"method\":\"set_sprite_on_A\",\"params\":{\"paramfile\":\"%s\"}}", m_sprite_on[0].paramsfname.c_str())
			+NosuchSnprintf(",{\"method\":\"set_sprite_on_B\",\"params\":{\"paramfile\":\"%s\"}}", m_sprite_on[1].paramsfname.c_str())
			+NosuchSnprintf(",{\"method\":\"set_sprite_on_C\",\"params\":{\"paramfile\":\"%s\"}}", m_sprite_on[2].paramsfname.c_str())
			+NosuchSnprintf(",{\"method\":\"set_sprite_on_D\",\"params\":{\"paramfile\":\"%s\"}}", m_sprite_on[3].paramsfname.c_str())
			+NosuchSnprintf(",{\"method\":\"set_sprite_off_A\",\"params\":{\"paramfile\":\"%s\"}}", m_sprite_off[0].paramsfname.c_str())
			+NosuchSnprintf(",{\"method\":\"set_sprite_off_B\",\"params\":{\"paramfile\":\"%s\"}}", m_sprite_off[1].paramsfname.c_str())
			+NosuchSnprintf(",{\"method\":\"set_sprite_off_C\",\"params\":{\"paramfile\":\"%s\"}}", m_sprite_off[2].paramsfname.c_str())
			+NosuchSnprintf(",{\"method\":\"set_sprite_off_D\",\"params\":{\"paramfile\":\"%s\"}}", m_sprite_off[3].paramsfname.c_str())
			+NosuchSnprintf(",{\"method\":\"set_channel_A\",\"params\":{\"channel\":%d}}", m_sprite_off[0].channel)
			+NosuchSnprintf(",{\"method\":\"set_channel_B\",\"params\":{\"channel\":%d}}", m_sprite_off[1].channel)
			+NosuchSnprintf(",{\"method\":\"set_channel_C\",\"params\":{\"channel\":%d}}", m_sprite_off[2].channel)
			+NosuchSnprintf(",{\"method\":\"set_channel_D\",\"params\":{\"channel\":%d}}", m_sprite_off[3].channel)
			+"]"
			;
		return jsonStringResult(dump,id);
	}
	if (meth == "restore") {
		std::string dump = jsonNeedString(json, "dump");
		ExecuteDump(dump);
		return jsonOK(id);
	}

	if (meth == "set_channel_A") { m_sprite_on[0].channel = jsonNeedInt(json, "channel"); return jsonOK(id); }
	if (meth == "get_channel_A") { return jsonIntResult(m_sprite_on[0].channel,id); }
	if (meth == "set_channel_B") { m_sprite_on[1].channel = jsonNeedInt(json, "channel"); return jsonOK(id); }
	if (meth == "get_channel_B") { return jsonIntResult(m_sprite_on[1].channel,id); }
	if (meth == "set_channel_C") { m_sprite_on[2].channel = jsonNeedInt(json, "channel"); return jsonOK(id); }
	if (meth == "get_channel_C") { return jsonIntResult(m_sprite_on[2].channel,id); }
	if (meth == "set_channel_D") { m_sprite_on[3].channel = jsonNeedInt(json, "channel"); return jsonOK(id); }
	if (meth == "get_channel_D") { return jsonIntResult(m_sprite_on[3].channel,id); }

	if (meth == "set_sprite_on_A") { return _set_params_on(0, json,id); }
	if (meth == "get_sprite_on_A") { return jsonStringResult(m_sprite_on[0].paramsfname, id); }
	if (meth == "set_sprite_on_B") { return _set_params_on(1, json,id); }
	if (meth == "get_sprite_on_B") { return jsonStringResult(m_sprite_on[1].paramsfname, id); }
	if (meth == "set_sprite_on_C") { return _set_params_on(2, json,id); }
	if (meth == "get_sprite_on_C") { return jsonStringResult(m_sprite_on[2].paramsfname, id); }
	if (meth == "set_sprite_on_D") { return _set_params_on(3, json,id); }
	if (meth == "get_sprite_on_D") { return jsonStringResult(m_sprite_on[3].paramsfname, id); }

	if (meth == "set_sprite_off_A") { return _set_params_off(0, json,id); }
	if (meth == "get_sprite_off_A") { return jsonStringResult(m_sprite_off[0].paramsfname, id); }
	if (meth == "set_sprite_off_B") { return _set_params_off(1, json,id); }
	if (meth == "get_sprite_off_B") { return jsonStringResult(m_sprite_off[1].paramsfname, id); }
	if (meth == "set_sprite_off_C") { return _set_params_off(2, json,id); }
	if (meth == "get_sprite_off_C") { return jsonStringResult(m_sprite_off[2].paramsfname, id); }
	if (meth == "set_sprite_off_D") { return _set_params_off(3, json,id); }
	if (meth == "get_sprite_off_D") { return jsonStringResult(m_sprite_off[3].paramsfname, id); }

	// PARAMETER "autoloadparams"
	if (meth == "set_autoloadparams") {
		m_autoloadparams = jsonNeedBool(json, "onoff", false);
		return jsonOK(id);
	}
	if (meth == "get_autoloadparams") {
		return jsonIntResult(m_autoloadparams?1:0, id);
	}

	throw NosuchException("VizMidi4 - Unrecognized method '%s'",meth.c_str());
}

void VizMidi4::processAdvanceClickTo(int click) {
	// DEBUGPRINT(("processAdvanceClickTo click=%d time=%ld", click, timeGetTime()));
}

void VizMidi4::processMidiInput(MidiMsg* m) {
	VizSprite* s;
	// NO OpenGL calls here
	switch (m->MidiType()) {
	case MIDI_CONTROL:
		// DEBUGPRINT(("processMidiInput control! ch=%d ctrl=%d val=%d", m->Channel(), m->Controller(), m->Value()));
#if 0
		// CC #127 messages on channel 16 will be used to change the visualization
		XXX - Aborted attempt to use ProcessJson from within a Vizlet
			to reload the pipeline

		if (m->Controller() == 127 && m->Channel() == 16) {
			int v = m->Value();
			DEBUGPRINT(("Setting visual to %d", v));
			std::string fullmethod = "ffff.loadpipeline";
			std::string p = NosuchSnprintf("{\"filename\":\"patch_%d\"}",v+1);
			cJSON* params = cJSON_Parse(p.c_str());
			// USING ProcessJson DOESN'T WORK!!  BECAUSE LOADING THE PIPELINE
			// WILL UNLOADE THIS VIZLET (unless I do something to prevent it)
			// const char* s = vizserver()->ProcessJson(fullmethod.c_str(), params, "12345");
		}
#endif
		break;
	case MIDI_NOTE_ON:
		// DEBUGPRINT(("NOTE_ON"));
		s = _midiVizNoteOnSprite(m);
		if (s) {
			s->m_data = new sprite_info(m->Channel(), m->Pitch());
		}
		break;
	case MIDI_NOTE_OFF:
		// DEBUGPRINT(("NOTE_OFF"));
		VizSpriteList* sl = GetVizSpriteList();
		sl->lock_read();
		for (std::list<VizSprite*>::iterator i = sl->m_sprites.begin(); i != sl->m_sprites.end(); i++) {
			VizSprite* s = *i;
			NosuchAssert(s);
			sprite_info* si = (sprite_info*)(s->m_data);
			if (si!=NULL && si->channel == m->Channel() && si->pitch == m->Pitch()) {
				s->m_data = NULL;
				int i = (si->channel-1) % 4;
				SpriteVizParams* p = m_sprite_off[i].params;

				// Save the *initial values - we don't want to overwrite those
				double save_alphainitial = s->m_params->alphainitial.get();
				double save_huefillinitial = s->m_params->huefillinitial.get();
				double save_hueinitial = s->m_params->hueinitial.get();
				double save_sizeinitial = s->m_params->sizeinitial.get();

				s->m_params->applyVizParamsFrom(p);

				// Don't use the *initial parameter values when initializing the state
				s->initVizSpriteState(GetTime(),Handle(),s->m_state.pos,s->m_params,false);

				// The initial alpha/size/hue/huefill values in the
				// parameters should be the current state, so the
				// transition is smoother

				s->m_params->alphainitial.set(s->m_state.alpha);
				s->m_params->sizeinitial.set(s->m_state.size);
				s->m_params->hueinitial.set(s->m_state.hue);
				s->m_params->huefillinitial.set(s->m_state.huefill);

				delete si;  // XXX - if NOTEOFF is not received, we'll have a memory leak
				break;
			}
		}
		sl->unlock();
		// DEBUGPRINT(("NOTE_OFF B"));
		break;
	} 
}

void VizMidi4::processMidiOutput(MidiMsg* m) {
	// NO OpenGL calls here
	processMidiInput(m);
}

void
VizMidi4::_reload_params(paramsfile_info& si) {
	SpriteVizParams* p;
	p = checkSpriteVizParamsAndLoadIfModifiedSince(si.paramsfname, si.lastfilecheck, si.lastfileupdate);
	if (p) {
		si.params = p;
	}
}

bool VizMidi4::processDraw() {
	// OpenGL calls here	

#if 0
	glColor4f(1.0,0.0,0.0,0.5);
	glLineWidth((GLfloat)2.0f);
	// Draw a rectangle just to show that we're alive.
	glBegin(GL_LINE_LOOP);
	glVertex3f(0.2f, 0.2f, 0.0f);	// Top Left
	glVertex3f(0.2f, 0.8f, 0.0f);	// Top Right
	glVertex3f(0.8f, 0.8f, 0.0f);	// Bottom Right
	glVertex3f(0.8f, 0.2f, 0.0f);	// Bottom Left
	glEnd();
#endif

	// XXX SHOULDN'T REALLY DO THIS SO OFTEN...
	if (m_autoloadparams) {
		static int cnt = 999;
		cnt = (cnt+1) % 4;	// cycle between the 4 
		_reload_params(m_sprite_on[cnt]);
		_reload_params(m_sprite_off[cnt]);
	}

	DrawVizSprites();
	return true;
}

NosuchPos posAlongLine(double amount, NosuchPos frompos, NosuchPos topos) {
	NosuchPos pos;
	pos.x = frompos.x + amount * (topos.x - frompos.x);
	pos.y = frompos.y + amount * (topos.y - frompos.y);
	pos.z = frompos.z + amount * (topos.z - frompos.z);
	return pos;
}

VizSprite* VizMidi4::_midiVizNoteOnSprite(MidiMsg* m) {

	int minpitch = 0;
	int maxpitch = 127;

	if ( m->Velocity() == 0 ) {
		DEBUGPRINT(("Vizlet1 sees noteon with 0 velocity, ignoring"));
		return NULL;
	}
		
	int midichan = 1 + (m->Channel()-1)%4;
	// DEBUGPRINT(("VizMidi4 midichan=%d", midichan));
	int foundi = -1;
	// See if any of the four slots are set to this channel
	for (int i = 0; i < 4; i++) {
		if (m_sprite_on[i].channel == midichan) {
			foundi = i;
			break;
			
		}
	}
	if (foundi < 0) {
		return NULL;
	}
	// DEBUGPRINT(("VizMidi4 foundi=%d", foundi));
	SpriteVizParams* params = m_sprite_on[foundi].params;
	std::string place = params->placement;
	// DEBUGPRINT(("NoteOnSprite this=%ld foundi=%d file=%s",(long)this,foundi,m_sprite_on[foundi].paramsfname.c_str()));

	int pitchmin = params->pitchmin.get();
	int pitchmax = params->pitchmax.get();
	bool pitchwrap = params->pitchwrap.get();

	int pitch = m->Pitch();
	if (pitch < pitchmin || pitch > pitchmax) {
		if (!pitchwrap) {
			return NULL;
		}
		int dp = pitchmax - pitchmin;
		while (pitch < pitchmin) {
			pitch += dp;
		}
		while (pitch > pitchmax) {
			pitch -= dp;
		}
	}
	double amount = double(pitch - pitchmin) / (pitchmax - pitchmin);
	if (amount < 0.0 || amount > 1.0) {
		DEBUGPRINT(("HEY! VizMidi4 amount isn't 0.0 to 1.0!?"));
		return NULL;
	}
	NosuchPos pos;
	NosuchPos linefrom;
	NosuchPos lineto;
	bool useline = true;

	if (place == "nowhere") {
		return NULL;
	}
	else if (place == "random") {
		pos.x = (rand() % 1000) / 1000.0;
		pos.y = (rand() % 1000) / 1000.0;
		useline = false;
	}
	else if (place == "bottom") {
		linefrom = NosuchPos(0.0, 0.1, 0.0);
		lineto = NosuchPos(1.0, 0.1, 0.0);
	}
	else if (place == "left") {
		linefrom = NosuchPos(0.1, 0.0, 0.0);
		lineto = NosuchPos(0.1, 1.0, 0.0);
	}
	else if (place == "right") {
		linefrom = NosuchPos(0.9, 0.0, 0.0);
		lineto = NosuchPos(0.9, 1.0, 0.0);
	}
	else if (place == "top") {
		linefrom = NosuchPos(0.0, 0.9, 0.0);
		lineto = NosuchPos(1.0, 0.9, 0.0);
	}
	else if (place == "center") {
		pos.x = 0.5;
		pos.y = 0.5;
		useline = false;
	}
	else {
		DEBUGPRINT(("Invalid placement value: %s",place.c_str()));
		pos.x = 0.5;
		pos.y = 0.5;
		useline = false;
	}

	if (useline) {
		pos = posAlongLine(amount, linefrom, lineto);
	}

	pos.z = (m->Velocity()*m->Velocity()) / (128.0*128.0);

	return makeAndAddVizSprite(params, pos);
}