#ifndef _REGION_H
#define _REGION_H

class VizServer;

#include "NosuchGraphics.h"

class VizCursor;
class PaletteVizCursor;
class NosuchScheduler;
class MMTT_SharedMemHeader;
class Palette;
class PaletteHost;
class VizSpriteList;
class GraphicBehaviour;
class MusicBehaviour;
class NoteBehaviour;

#define DEFAULT_SHAPE "square"
#define MAX_REGION_ID 22

class Region {

public:
	Region(Palette* p, int id);
	~Region();

	// 0 is a NULL region id - the first region is id=1.
	int id;
	std::string name;
	Palette* palette() { return _palette; }
	typedef enum {
		UNKNOWN,
		SURFACE,
		BUTTON,
	} region_type;
	region_type type;
	int sid_low;
	int sid_high;

	void setTypeAndSid(Region::region_type t, int sid_low, int sid_high);
	void setButtonPatch(std::string patchname);
	AllVizParams* LoadParams(std::string fname);
	// void applyPatch(std::string patchname);

	void resetRegionParams();
	AllVizParams* regionParams() { return _regionParams; }
	// AllVizParams* regionSpecificParams() { return _regionSpecificParams; }

	void initSound();
	void touchVizCursor(int sidnum, std::string sidsource, int now);
	VizCursor* getVizCursor(int sidnum, std::string sidsource);
	void AddVizSprite(VizSprite* s);

	// VizCursor* setVizCursor(int sidnum, std::string sidsource, int now, NosuchPos pos, double depth, double area, OutlineMem* om, MMTT_SharedMemHeader* hdr);
	// void setVizCursorUp(int sid, std::string source);
	double AverageVizCursorDepth();
	double MaxVizCursorDepth();

	double getMoveDir(std::string movedir);

	void processCursor(VizCursor* c, int downdragup);
	void buttonDown();
	void buttonUp();
	void cursorDown(VizCursor* c);
	void cursorDrag(VizCursor* c);
	void cursorUp(VizCursor* c);

	double spriteMoveDir(VizCursor* c);
	void setNotesDisabled(bool disabled) {
		NosuchDebug("setNotesDisabled = %s",disabled?"true":"false");
		_disableNotes = disabled;
	}
	bool NotesDisabled() { return _disableNotes; }
	// these need to be thread-safe
	void draw(PaletteHost* b);
	void advanceTo(int tm);
	void hitPaletteSprites();

	int channel() { return _channel; }
	std::list<VizCursor*>& cursors() { return _palettecursors; }

	// NosuchScheduler* scheduler();
	VizServer* vizserver();
	PaletteHost* palettehost();
	Vizlet* vizlet();

	size_t NumVizCursors() { return _palettecursors.size(); }

	void UpdateSound();

	bool isButton() { return type == BUTTON; };
	bool isRegion() { return type == SURFACE; };
	bool buttonTooDeep(VizCursor* c);
	std::string patch() { return _patch; }

private:

	Palette* _palette;
	AllVizParams* _regionParams;
	// These are the values specific to this region
	// AllVizParams* _regionSpecificParams;

	std::string _patch;
	bool _chording;
	GraphicBehaviour* _graphicBehaviour;
	MusicBehaviour* _musicBehaviour;

	std::list<VizCursor*> _palettecursors;

	int _latestNoteTime;
	bool _disableNotes;

	int _lastScheduled;
	int _channel;  // -1 means we need to find a channel for the current sound

	pthread_mutex_t _region_mutex;

	// Access to these lists need to be thread-safe
	VizSpriteList* spritelist;

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
