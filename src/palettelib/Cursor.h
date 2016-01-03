#ifndef _CURSOR_H
#define _CURSOR_H

// Don't instantiate a cursor's sprites more often than this number of milliseconds
#define SPRITE_THROTTLE_MS_PER_CURSOR 5

class Palette;
class Region;

class Cursor {

public:

	static bool initialized;
	static void initialize();

	Cursor(Palette* palette_, int sidnum, std::string sidsource, Region* region_, NosuchVector pos_, double depth_, double area_);
	~Cursor();
	Cursor* clone();
	double radian2degree(double r) {
		return r * 360.0 / (2.0 * (double)M_PI);
	}
	bool rotauto() { return _region->params.rotauto; }

	Region* region() { return _region; }
	Palette* palette() { return _palette; }
	int touched() { return _touched; }
	void touch(int millinow) { _touched = millinow; }
	// std::string sid() { return _sid; }
	std::string sidsource() { return _sidsource; }
	int sidnum() { return _sidnum; }
	double area() { return _area; }

	double target_depth() { return _target_depth; }
	void set_target_depth(double d) { _target_depth = d; }
	void setarea(double v) { _area = v; }
	void setoutline(OutlineMem* om) {
		_outline_mem = om;
	}
	OutlineMem* outline() { return _outline_mem; }

	void settargetpos(NosuchVector p) {
		// _prev_pos = _pos;
		_target_pos = p;
	}
	// NosuchVector previous_pos() { return _prev_pos; }

	// Manipulation of cursor-related things for graphics
	void advanceTo(int tm);

	double g_smootheddistance() { return _g_smoothed_distance; }
	double target_degrees() { return _target_degrees; }

	// Manipulation of cursor-related things for music

	// XXX - this last_pitches stuff is bogus.  A cursorUp should just
	// invoke an empty event and let the DoEventAndDelete handle the
	// automatic note-offs for everything currently playing.

	std::vector<int>& lastpitches() { return _last_pitches; }
	int lastchannel() { return _last_channel; }
	int lastclick() { return _last_click; }
	void add_last_note(int clk, MidiMsg* m) {
		// NosuchDebug(2,"ADD_LAST_NOTE clk=%d m=%s",clk,m->DebugString().c_str());
		_last_click = clk;
		_last_channel = m->Channel();
		_last_pitches.push_back(m->Pitch());
		MidiMsg* nextm = m->next;
		while ( nextm != NULL ) {
			_last_pitches.push_back(nextm->Pitch());
			nextm = nextm->next;
		}
	}
	int last_pitch() {
		if ( _last_pitches.size() == 0 ) {
			return -1;
		}
		return _last_pitches.front();
	}
	void clear_last_note() {
		NosuchDebug(2,"CLEAR_LAST_NOTE!");
		_last_click = -1;
		_last_channel = -1;
		_last_pitches.clear();
	}
	NosuchVector previous_musicpos() { return _previous_musicpos; }
	double last_depth() { return _last_depth; }
	void set_previous_musicpos(NosuchVector p) { _previous_musicpos = p; }
	void set_last_depth(double f) { _last_depth = f; }

	bool isRightSide() { return ( curr_pos.x >= 0.5 ); }

	std::string DebugString() {
		return NosuchSnprintf("Cursor sid=%d/%s current=%.3f,%.3f last=%.3f,%.3f target=%.3f,%.3f depth=%.3f target=%.3f",
			sidnum(),sidsource().c_str(), curr_pos.x,curr_pos.y, _last_pos.x,_last_pos.y,
			_target_pos.x,_target_pos.y,curr_depth,_target_depth);
	}
	std::string DebugBrief() {
		return NosuchSnprintf("Cursor sid=%d/%s pos=%.3f,%.3f depth=%.3f",
			sidnum(),sidsource().c_str(), curr_pos.x,curr_pos.y, curr_depth);
	}

	NosuchVector curr_pos;
	double curr_depth;
	double curr_speed;   // distance per second
	std::string curr_behaviour;
	double curr_degrees;

	void set_last_instantiate(int tm) { _last_instantiate = tm; }
	int last_instantiate() { return _last_instantiate; }

private:
	// General stuff
	OutlineMem* _outline_mem;
	int _start_time;
	int _last_tm;
	long _touched;   // milliseconds
	long _last_instantiate;
	Region* _region;
	Palette* _palette;
	long _lastalive;
	int _sidnum; // This is the raw sid, e.g. 4000
	std::string _sidsource; // The hostname, or "sharedmem"
	double _area;
	NosuchVector _last_pos;
	NosuchVector _target_pos;
	// NosuchVector _prev_pos;
	double _target_depth;
	double _last_depth;


	// Musical stuff
	std::vector<int> _last_pitches;
	int _last_channel;
	int _last_click;
	NosuchVector _previous_musicpos;

	// Graphical stuff
	// NosuchVector _g_smoothedpos;
	// NosuchVector _g_last_smoothedpos;
	double _g_smoothed_distance;
	double _target_degrees;
	double _smooth_degrees_factor;
	bool _g_firstdir;
};

#endif