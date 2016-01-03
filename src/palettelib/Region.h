#ifndef _REGION_H
#define _REGION_H

class NosuchLoop;
class NosuchScheduler;
class MMTT_SharedMemHeader;
class SpriteList;
class Palette;

#define DEFAULT_SHAPE "square"
#define MAX_REGION_ID 22

#if 0
class RegionParams : public Params {
public:
	RegionParams() {
#include "RegionParams_init.h"
	}

	static std::vector<std::string> behaviourTypes;
	static std::vector<std::string> movedirTypes;
	static std::vector<std::string> rotangdirTypes;
	static std::vector<std::string> mirrorTypes;
	static std::vector<std::string> controllerstyleTypes;
	static std::vector<std::string> shapeTypes;

	static void Initialize();

	std::string JsonString(std::string pre, std::string indent, std::string post) {
		char* names[] = {
#include "RegionParams_list.h"
			NULL
		};
		return JsonList(names,pre,indent,post);
	}

	void Set(std::string nm, std::string val) {
		bool stringval = false;

#define SET_DBL_PARAM(name) else if ( nm == #name ) name = string2double(val)
#define SET_INT_PARAM(name) else if ( nm == #name ) name = string2int(val)
#define SET_BOOL_PARAM(name) else if ( nm == #name ) name = string2bool(val)
#define SET_STR_PARAM(name) else if ( nm == #name ) (name = val),(stringval=true)

		if ( false ) { }
#include "RegionParams_set.h"

		// To abide by the limits for each value, we rely on the code in Increment()
		if ( ! stringval ) {
			Increment(nm,0.0);
		}
	}
	void Increment(std::string nm, double amount) {

#define INC_DBL_PARAM(name,mn,mx) else if (nm==#name)name=adjust(name,amount,mn,mx)
#define INC_INT_PARAM(name,mn,mx) else if (nm==#name)name=adjust(name,amount,mn,mx)
#define INC_STR_PARAM(name,vals) else if (nm==#name)name=adjust(name,amount,vals)
#define INC_BOOL_PARAM(name) else if (nm==#name)name=adjust(name,amount)
#define INC_NO_PARAM(name) else if (nm==#name)name=name

		if ( false ) { }
#include "RegionParams_increment.h"
	}
	void Toggle(std::string nm) {
		// Just the boolean values
#define TOGGLE_PARAM(name) else if ( nm == #name ) name = ! name
		if ( false ) { }
#include "RegionParams_toggle.h"
		else { NosuchDebug("No Toggle implemented for %s",nm.c_str()); }
	}
	std::string Get(std::string nm) {

#define GET_DBL_PARAM(name) else if(nm==#name)return DoubleString(name)
#define GET_INT_PARAM(name) else if(nm==#name)return IntString(name)
#define GET_BOOL_PARAM(name) else if(nm==#name)return BoolString(name)
#define GET_STR_PARAM(name) else if(nm==#name)return name

		if ( false ) { }
#include "RegionParams_get.h"
		return "";
	}

#include "RegionParams_declare.h"

	bool IsSpriteParam(std::string nm) {

#define IS_SPRITE_PARAM(name) if( nm == #name ) { return true; }

#include "RegionParams_issprite.h"
		return false;
	}
};

class RegionOverrides : public Params {
public:
	RegionOverrides() {
#define OVERRIDE_INIT(name) name = false;

#include "RegionOverrides_init.h"
	}
	std::string JsonString(std::string pre, std::string indent, std::string post) {
		char* names[] = {
#include "RegionOverrides_list.h"
			NULL
		};
		return JsonList(names,pre,indent,post);
	}
	void Set(std::string nm, std::string override) {
		return Set(nm,string2bool(override));
	}
	void Set(std::string nm, bool override) {

#define SET_OVERRIDE(name) else if (nm == #name ) name = override

		if ( false ) { }
#include "RegionOverrides_set.h"
	}
	std::string Get(std::string nm) {
		return GetBool(nm) ? "true" : "false";
	}
	bool GetBool(std::string nm) {

#define GET_BOOL(name) else if (nm == #name ) return name

		if ( false ) { }
#include "RegionOverrides_get.h"
		return false;
	}
#include "RegionOverrides_declare.h"
};
#endif

class Region {

public:
	Region(Palette* p, int id);
	~Region();

	SpriteVizParams* params;

	// 0 is a NULL region id - the first region is id=1.
	int id;
	std::string name;
	Palette* palette;
	typedef enum {
		UNKNOWN,
		SURFACE,
		BUTTON,
	} region_type;
	region_type type;
	int sid_low;
	int sid_high;

	void SetTypeAndSid(Region::region_type t, int sid_low, int sid_high);

	void initSound();
	void initParams();
	static private std::string shapeOfRegion(int id);
	static private double hueOfRegion(int id);
	void touchCursor(int sidnum, std::string sidsource, int now);
	Cursor* getCursor(int sidnum, std::string sidsource);
	Cursor* setCursor(int sidnum, std::string sidsource, int now, NosuchVector pos, double depth, double area, OutlineMem* om);
	void addrandom();
	double getMoveDir(std::string movedir);
	void buttonDown(std::string name);
	void buttonUp(std::string name);
	void cursorDown(Cursor* c);
	void cursorDrag(Cursor* c);
	void cursorUp(Cursor* c);
	void checkCursorUp(int milli);
	void instantiateSprite(Cursor* c, bool throttle);
	double spriteMoveDir(Cursor* c);
	int LoopId();
	void setNotesDisabled(bool disabled) {
		NosuchDebug("setNotesDisabled = %s",disabled?"true":"false");
		_disableNotes = disabled;
	}
	bool NotesDisabled() { return _disableNotes; }
	// these need to be thread-safe
	void draw();
	void advanceTo(int tm);
	void hitSprites();
	void getCursors(std::list<Cursor*>& cursors);

#if 0
	void accumulateSpritesForCursor(Cursor* c);
	void spritelist_lock_read();
	void spritelist_lock_write();
	void spritelist_unlock();
#endif
	bool cursorlist_lock_read();
	bool cursorlist_lock_write();
	void cursorlist_unlock();

	// ParamList* params() { return _params; }
	int channel() { return _channel; }
	NosuchLoop* loop() {
		if ( _loop == NULL ) {
			NosuchDebug("Hey, _loop is NULL?");
		}
		NosuchAssert(_loop);
		return _loop;
	}
	std::list<Cursor*>& cursors() { return _cursors; }

	void init_loop();

	NosuchScheduler* scheduler();

	double AverageCursorDepth();
	double MaxCursorDepth();
	size_t NumCursors() { return _cursors.size(); }
	// int NumberScheduled(click_t minclicks, click_t maxclicks, std::string sid);

	void OutputNotificationMidiMsg(MidiMsg* mm, int sidnum);

	void UpdateSound();

	bool isButton() { return type == BUTTON; };
	bool isSurface() { return type == SURFACE; };
	bool buttonTooDeep(Cursor* c);
	bool Chording() { return _chording; }
	bool Chording(bool b) { _chording = b; return _chording; }
	bool Looping() { return _looping; }
	bool Looping(bool b) { _looping = b; return _looping; }

private:

	NosuchLoop* _loop;
	bool _looping;
	bool _chording;
	GraphicBehaviour* _graphicBehaviour;
	MusicBehaviour* _musicBehaviour;
	NoteBehaviour* _noteBehaviour;

	std::list<Cursor*> _cursors;

	int _latestNoteTime;
	bool _disableNotes;

	int _lastScheduled;
	int _channel;  // -1 means we need to find a channel for the current sound
	// ParamList* _params;

	pthread_mutex_t _region_mutex;
#if 0
	pthread_rwlock_t spritelist_rwlock;
#endif
	pthread_rwlock_t cursorlist_rwlock;

	// Access to these lists need to be thread-safe
	// std::list<Sprite*> sprites;
	SpriteList* spritelist;

	// int m_id;
	int r;
	int g;
	int b;
	int numalive;
	int onoff;
	int debugcount;

	int last_tm;
	int leftover_tm;
	int fire_period;
	// This can be adjusted to ignore things close to the edges of each area, to ignore spurious events
	double x_min;
	double y_min;
	double x_max;
	double y_max;
};

#endif
