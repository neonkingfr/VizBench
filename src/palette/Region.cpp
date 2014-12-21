#include  <stdlib.h>
#include <vector>
#include <cstdlib> // for srand, rand

#include "VizServer.h"
#include "PaletteAll.h"

Region::Region(Palette* p, int i) {

	spritelist = new VizSpriteList();
	DEBUGPRINT1(("Region constructor i=%d spritelist=%ld",i,(long)spritelist));

	id = i;
	name = "";
	setTypeAndSid(UNKNOWN, 0, 0);

	_disableNotes = false;

	_palette = p;
	_lastScheduled = -1;
	_chording = false;

	NosuchLockInit(&_region_mutex,"region");

	_latestNoteTime = 0;
	x_min = 0.000f;  // Used to be 0.001f, not sure why
	y_min = 0.000f;  // Used to be 0.001f, not sure why
	x_max = 1.000f;  // Used to be 0.999f, not sure why
	y_max = 1.000f;  // Used to be 0.999f, not sure why
	_channel = -1;

	_regionParams = new AllVizParams(true);  // loads defaults
	// _regionSpecificParams = NULL;

	PaletteHost* ph = p->paletteHost();
	// _graphicBehaviour = ph->makeGraphicBehaviour(this);
	_graphicBehaviour = new GraphicBehaviour(this);
	_musicBehaviour = new MusicBehaviour(this);

	numalive = 0;
	debugcount = 0;
	last_tm = 0;
	leftover_tm = 0;
	// fire_period = 10;  // milliseconds
	fire_period = 1;  // milliseconds
	onoff = 0;
}

PaletteHost* Region::palettehost() {
	return palette()->paletteHost();
}

Vizlet* Region::vizlet() {
	return palette()->paletteHost()->vizlet();
}

void Region::resetRegionParams() {
	_regionParams = new AllVizParams(true);  // loads defaults
}

void
Region::setTypeAndSid(Region::region_type t, int sid_low_, int sid_high_) {
	type = t;
	sid_low = sid_low_;
	sid_high = sid_high_;
}

#if 0
AllVizParams*
Region::LoadParams(std::string pathname,AllVizParams* p) {
	std::string err;
	DEBUGPRINT(("LoadParams pathname=%s",pathname.c_str()));
	cJSON* j = jsonReadFile(pathname,err);
	if ( ! j ) {
		throw NosuchException("Unable to open file (name=%s, err=%s)",pathname.c_str(),err.c_str());
	}
	p->loadJson(j);
	jsonFree(j);
	return p;
}
#endif


void
Region::setButtonPatch(std::string newpatchname) {
	_patch = newpatchname;
}

Region::~Region() {
	NosuchDebug("Region DESTRUCTOR!");
	delete _graphicBehaviour;
	delete _musicBehaviour;
}

void
Region::initSound() {
	if ( isRegion() ) {
		std::string snd = SoundBank[0][0][0];
		NosuchAssert(snd!="");
		_regionParams->sound.set(snd);
		UpdateSound();
	}
}

void
Region::UpdateSound() {

	std::string sound = _regionParams->sound.get();

	std::map<std::string,Sound>::iterator it = Sounds.find(sound);
	if ( it == Sounds.end() ) {
		NosuchDebug("Hey, Updatesound found invalid sound, region=%d sound=%s",id,sound.c_str());
		return;
	}

	NosuchDebug(1,"Region::UpdateSound region=%d sound=%s",
		id,_regionParams->sound.get().c_str());

	int ch = palette()->findSoundChannel(sound,id);
	if ( ch < 0 ) {
		NosuchDebug("Region::UpdateSound Unable to find channel for sound=%s, using existing channel 1",sound.c_str());
		ch = 1;
	}
	if ( ch != _channel ) {
		NosuchDebug(1,"Existing channel for region %d is %d, new channel needs to be %d",
			id,_channel,ch);
		// Tempting to send ANO for the old channel, but the whole point
		// of the dynamic channel stuff is to avoid the need to cut off
		// old sounds when changing to new ones.
		_channel = ch;
		// Send ANO on the new channel, to terminate
		// anything currently playing there.
		vizserver()->ANO(_channel);
	} else {
		NosuchDebug(1,"Existing channel for region %d is %d",id,_channel);
	}
	MidiProgramChange* msg = Sound::ProgramChangeMsg(_channel,sound);
	if ( msg == NULL ) {
		NosuchDebug("HEY!! ProgramChangeMsg returned null for sound=%s",sound.c_str());
		return;
	}
	NosuchDebug("CHANGE rgn=%d sound=%s ch=%d",
		id,sound.c_str(),ch);
	NosuchDebug(1,"   progchange=%d", msg->Value());
	vizserver()->SendMidiMsg(msg);
	Sleep(150);  // Lame attempt to avoid Alchemy bug when it gets lots of Program Change messages
}

#define BUTTON_TOO_DEEP 0.15

bool Region::buttonTooDeep(VizCursor* c) {
	return ( c->depth() > BUTTON_TOO_DEEP );
}

void Region::touchVizCursor(int sidnum, std::string sidsource, int millinow) {
	VizCursor* c = getVizCursor(sidnum, sidsource);
	if ( c != NULL ) {
		c->touch(millinow);
	}
}

VizCursor* Region::getVizCursor(int sidnum, std::string sidsource) {
	VizCursor* retc = NULL;

	for ( std::list<VizCursor*>::iterator i = _palettecursors.begin(); i!=_palettecursors.end(); i++ ) {
		VizCursor* c = *i;
		NosuchAssert(c);
		if (c->sid == sidnum && c->source == sidsource) {
			retc = c;
			break;
		}
	}
	return retc;
}

double Region::AverageVizCursorDepth() {
	double totval = 0;
	int totnum = 0;
	for ( std::list<VizCursor*>::iterator i = _palettecursors.begin(); i!=_palettecursors.end(); i++ ) {
		VizCursor* c = *i;
		NosuchAssert(c);
		totval += c->depth();
		totnum++;
	}
	if ( totnum == 0 ) {
		return -1;
	} else {
		return totval / totnum;
	}
}

double Region::MaxVizCursorDepth() {
	double maxval = 0;
	for ( std::list<VizCursor*>::iterator i = _palettecursors.begin(); i!=_palettecursors.end(); i++ ) {
		VizCursor* c = *i;
		NosuchAssert(c);
		double d = c->depth();
		if ( d > maxval )
			maxval = d;
	}
	return maxval;
}

double Region::getMoveDir(std::string movedirtype) {
	if ( movedirtype == "left" ) {
		return 180.0f;
	}
	if ( movedirtype == "right" ) {
		return 0.0f;
	}
	if ( movedirtype == "up" ) {
		return 90.0f;
	}
	if ( movedirtype == "down" ) {
		return 270.0f;
	}
	if ( movedirtype == "random" ) {
		double f = ((double)(rand()))/ RAND_MAX;
		return f * 360.0f;
	}
	throw NosuchException("Unrecognized movedirtype value %s",movedirtype.c_str());
}

void Region::processCursor(VizCursor* c, int downdragup) {
	if ( isButton() ) {
		DEBUGPRINT(("Region::processCursor BUTTON!"));
		switch ( downdragup ) {
		case CURSOR_DOWN: buttonDown(); break;
		case CURSOR_UP: buttonUp(); break;
		}
	} else {
		DEBUGPRINT(("Region::processCursor SURFACE!"));
		switch ( downdragup ) {
		case CURSOR_DOWN: cursorDown(c); break;
		case CURSOR_DRAG: cursorDrag(c); break;
		case CURSOR_UP: cursorUp(c); break;
		}
	}
}

void Region::buttonDown() {
	palette()->buttonDown(name);
	if ( _graphicBehaviour->isMyButton(name) ) {
		_graphicBehaviour->buttonDown(name);
	}
	if ( _musicBehaviour->isMyButton(name) ) {
		_musicBehaviour->buttonDown(name);
	}
}

void Region::buttonUp() {
	palette()->buttonUp(name);
	if ( _graphicBehaviour->isMyButton(name) ) {
		_graphicBehaviour->buttonUp(name);
	}
	if ( _musicBehaviour->isMyButton(name) ) {
		_musicBehaviour->buttonUp(name);
	}
}

void Region::cursorDown(VizCursor* c) {
	if ( isButton() ) {
		NosuchDebug(1,"Region cursorDown depth=%f",c->depth());
		if ( buttonTooDeep(c) ) {
			NosuchDebug("Ignoring cursor_down for button, too deep! (%.4f)",c->depth());
		} else {
			NosuchDebug("REGION::BUTTONDOWN %s now=%.3f sid=%d/%s area=%.4f",
				name.c_str(),Palette::now/1000.0f,c->sid,c->source.c_str(),c->area);
			if ( c->area < 0.0 ) {
				NosuchDebug("HEY!!!! area is negative!???");
			}
			buttonDown();
		}
	} else {
		_graphicBehaviour->cursorDown(c);
		_musicBehaviour->cursorDown(c);
		palette()->SetMostRecentVizCursorDown(c);
	}
}

void Region::cursorDrag(VizCursor* c) {
	if ( isButton() ) {
		// Buttons aren't dragged
		std::string bn = name;
		if ( palette()->isButtonDown(bn) != true ) {
			// This usually happens when the cursorDown is "too deep"
			NosuchDebug(2,"Hmmm, got cursorDrag for button=%s when _buttonDown=false?",bn.c_str());
		}
	} else {
		_graphicBehaviour->cursorDrag(c);
		_musicBehaviour->cursorDrag(c);
	}
}

void Region::cursorUp(VizCursor* c) {
	if ( c == palette()->MostRecentVizCursorDown() ) {
		palette()->SetMostRecentVizCursorDown(NULL);
	}
	if ( isButton() ) {
		if ( buttonTooDeep(c) ) {
			NosuchDebug("Ignoring button up, too deep!");
		} else if ( palette()->isButtonDown(name) != true ) {
			NosuchDebug("Ignoring button up, isButtonDown wasn't true!");
		} else {
			buttonUp();
		}
	} else {
		_graphicBehaviour->cursorUp(c);
		_musicBehaviour->cursorUp(c);
	}
}

double
Region::spriteMoveDir(VizCursor* c)
{
	double dir;
	if ( c != NULL && _regionParams->movefollowcursor.get() ) {
		dir = c->curr_degrees;
		// not sure why I have to reverse it - the cursor values are probably reversed
		dir -= 90.0;
		if ( dir < 0.0 ) {
			dir += 360.0;
		}
	} else {
		dir = _regionParams->movedir.get();
	}
	return dir;
}

void Region::AddVizSprite(VizSprite* s) {
	spritelist->add(s,_regionParams->nsprites);
}

void Region::hitPaletteSprites() {
	spritelist->hit();
}

void Region::draw(PaletteHost* b) {
	// DEBUGPRINT(("Region draw i=%d spritelist=%ld size=%d",this->id,(long)spritelist,spritelist->size()));
	spritelist->draw(&(palette()->paletteHost()->vizlet()->graphics));
}

void Region::advanceTo(int tm) {
	
	// DEBUGPRINT(("Region advanceTo i=%d spritelist=%ld size=%d",this->id,(long)spritelist,spritelist->size()));
	spritelist->advanceTo(tm);
	
	if ( last_tm > 0 && isRegion() ) {
		int dt = leftover_tm + tm - last_tm;
		if ( dt > fire_period ) {
			// NosuchDebug("Region %d calling behave->periodicFire now=%d",this->id,Palette::now);
			_graphicBehaviour->advanceTo(tm);
			_musicBehaviour->advanceTo(tm);
			dt -= fire_period;
		}
		leftover_tm = dt % fire_period;
	}
	last_tm = tm;
}

VizServer* Region::vizserver() {
	return palette()->paletteHost()->vizserver();
}