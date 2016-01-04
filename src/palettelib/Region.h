#ifndef _REGION_H
#define _REGION_H

class NosuchLoop;
class NosuchScheduler;
class MMTT_SharedMemHeader;
class SpriteList;
class Palette;

#define DEFAULT_SHAPE "square"
#define MAX_REGION_ID 22

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
