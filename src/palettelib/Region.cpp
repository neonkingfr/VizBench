#include  <stdlib.h>
#include <vector>
#include <cstdlib> // for srand, rand

#include "PaletteAll.h"

#define DEFAULT_LOOPING false

void RegionParams::Initialize() {

}

Region::Region(Palette* p, int i) {

	spritelist = new SpriteList();

	id = i;
	name = "";
	SetTypeAndSid(UNKNOWN, 0, 0);

	_loop = NULL;
	_disableNotes = false;

	palette = p;
	_lastScheduled = -1;
	_chording = false;
	_looping = DEFAULT_LOOPING;

	NosuchLockInit(&_region_mutex,"region");
	// spritelist_rwlock = PTHREAD_RWLOCK_INITIALIZER;
	cursorlist_rwlock = PTHREAD_RWLOCK_INITIALIZER;
	// int rc1 = pthread_rwlock_init(&spritelist_rwlock, NULL);
	int rc = pthread_rwlock_init(&cursorlist_rwlock, NULL);
	if ( rc ) {
		NosuchDebug("Failure on pthread_rwlock_init!? rc=%d",rc);
	}

	_latestNoteTime = 0;
	x_min = 0.00f;
	y_min = 0.00f;
	x_max = 1.0f;
	y_max = 1.0f;
	_channel = -1;

#if 0
	PaletteHost* ph = p->paletteHost();
	_graphicBehaviour = ph->makeGraphicBehaviour(this);
	_musicBehaviour = ph->makeMusicBehaviour(this);
#endif

	_noteBehaviour = new NoteBehaviourDefault(this);

	initParams();
}

void
Region::init_loop()
{
	if ( isSurface() ) {
		int loopid = LOOPID_BASE + id;
		_loop = new NosuchLoop(palette->paletteHost(),loopid,params.looplength,params.loopfade);
		palette->paletteHost()->AddLoop(_loop);
	} else {
		_loop = NULL;
	}
}

void
Region::SetTypeAndSid(Region::region_type t, int sid_low_, int sid_high_) {
	type = t;
	sid_low = sid_low_;
	sid_high = sid_high_;
}

int
Region::LoopId() {
	return _loop->id();
}

Region::~Region() {
	NosuchDebug("Region DESTRUCTOR!");
	delete _graphicBehaviour;
	delete _musicBehaviour;
	delete _noteBehaviour;
	// Hmmm, the _loop pointer is also
	// given to NosuchLooper::AddLoop, so watch out
	delete _loop;
}

void Region::initParams() {
	numalive = 0;
	debugcount = 0;
	last_tm = 0;
	leftover_tm = 0;
	// fire_period = 10;  // milliseconds
	fire_period = 1;  // milliseconds
	onoff = 0;
}

void
Region::initSound() {
	if ( isSurface() ) {
		std::string snd = SoundBank[0][0][0];
		NosuchAssert(snd!="");
		params.sound = snd;
		UpdateSound();
	}
}

void
Region::UpdateSound() {

	std::string sound = params.sound;

	std::map<std::string,Sound>::iterator it = Sounds.find(sound);
	if ( it == Sounds.end() ) {
		NosuchDebug("Hey, Updatesound found invalid sound, region=%d sound=%s",id,sound.c_str());
		return;
	}

	NosuchDebug(1,"Region::UpdateSound region=%d sound=%s",
		id,params.sound.c_str());

	int ch = palette->findSoundChannel(sound,id);
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
		scheduler()->ANO(_channel);
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
	NosuchDebug(1,"   progchange=%s", msg->DebugString().c_str());
	int loopid;
	if ( _loop == NULL )
		loopid = -1;
	else
		loopid = _loop->id();
	scheduler()->SendMidiMsg(msg,loopid);
	Sleep(150);  // Lame attempt to avoid Alchemy bug when it gets lots of Program Change messages
}

#define BUTTON_TOO_DEEP 0.15

bool Region::buttonTooDeep(Cursor* c) {
	return ( c->curr_depth > BUTTON_TOO_DEEP );
}

std::string Region::shapeOfRegion(int id) {
	// shape 0 is "nothing", so avoid 0
	int i = 1 + id % (RegionParams::shapeTypes.size()-1);
	std::string s = RegionParams::shapeTypes[i];
	s = DEFAULT_SHAPE;
	return s;
}

double Region::hueOfRegion(int id) {
	switch (id % 7) {
	case 0: return 0;
	case 1: return 50;
	case 2: return 100;
	case 3: return 150;
	case 4: return 200;
	case 5: return 250;
	case 6:
	default: return 300;
	}
}

void Region::touchCursor(int sidnum, std::string sidsource, int millinow) {
	Cursor* c = getCursor(sidnum, sidsource);
	if ( c != NULL ) {
		c->touch(millinow);
	}
}

Cursor* Region::getCursor(int sidnum, std::string sidsource) {
	Cursor* retc = NULL;

	if ( ! cursorlist_lock_read() ) {
		NosuchDebug("Region::getCursor returns NULL, unable to lock cursorlist");
		return NULL;
	}
	for ( std::list<Cursor*>::iterator i = _cursors.begin(); i!=_cursors.end(); i++ ) {
		Cursor* c = *i;
		NosuchAssert(c);
		if (c->sidnum() == sidnum && c->sidsource() == sidsource) {
			retc = c;
			break;
		}
	}
	cursorlist_unlock();

	// if ( retc == NULL ) {
	// NosuchDebug("DIDN'T FIND getCursor for sid=%s",sid.c_str());
	// }
	return retc;
}

double Region::AverageCursorDepth() {
	double totval = 0;
	int totnum = 0;
	if ( ! cursorlist_lock_read() ) {
		NosuchDebug("Region::AverageCursorDepth returns -1, unable to lock cursorlist");
		return -1;
	}
	for ( std::list<Cursor*>::iterator i = _cursors.begin(); i!=_cursors.end(); i++ ) {
		Cursor* c = *i;
		NosuchAssert(c);
		totval += c->curr_depth;
		totnum++;
	}
	cursorlist_unlock();
	if ( totnum == 0 ) {
		return -1;
	} else {
		return totval / totnum;
	}
}

double Region::MaxCursorDepth() {
	double maxval = 0;
	if ( ! cursorlist_lock_read() ) {
		NosuchDebug("Region::MaxCursorDepth returns -1, unable to lock cursorlist");
		return -1;
	}
	for ( std::list<Cursor*>::iterator i = _cursors.begin(); i!=_cursors.end(); i++ ) {
		Cursor* c = *i;
		NosuchAssert(c);
		double d = c->curr_depth;
		if ( d > maxval )
			maxval = d;
	}
	cursorlist_unlock();
	return maxval;
}

Cursor*
Region::setCursor(int sidnum, std::string sidsource, int millinow, NosuchVector pos, double depth, double area, OutlineMem* om) {

	if ( pos.x < x_min || pos.x > x_max || pos.y < y_min || pos.y > y_max ) {
		NosuchDebug("Ignoring out-of-bounds cursor pos=%f,%f,%f\n",pos.x,pos.y);
		return NULL;
	}

	if ( isButton() ) {
		// double buttonMinArea = 0.6f;
		double buttonMinArea = params.minbuttonarea;
		double buttonMaxArea = 1.3f;
		if ( area < buttonMinArea ) {
			NosuchDebug(2,"Ignoring setCursor for button, area %f too small",area);
			return NULL;
		}
		if ( area > buttonMaxArea ) {
			NosuchDebug(2,"Ignoring setCursor for button, area %f too big",area);
			return NULL;
		}
	}
	
	Cursor* c = getCursor(sidnum,sidsource);
	if ( c != NULL ) {
		c->settargetpos(pos);
		c->set_target_depth(depth);
		c->setarea(area);
		c->setoutline(om);
		cursorDrag(c);
	} else {
		c = new Cursor(palette, sidnum, sidsource, this, pos, depth,area);
		c->setoutline(om);

		if ( cursorlist_lock_write() ) {
			_cursors.push_back(c);
			cursorlist_unlock();

			cursorDown(c);
		} else {
			NosuchDebug("Region::setCursor unable to lock cursorlist");
		}
	}
	c->touch(millinow);
	return c;
}

void Region::addrandom() {
	double x = ((double)rand())/RAND_MAX;
	double y = ((double)rand())/RAND_MAX;
	double depth = ((double)rand())/RAND_MAX;
	depth /= 3.0;  // fake input shouldn't be very deep
	NosuchVector pos = NosuchVector(x,y);
	int sidnum = 2001;
	// std::string sid = sidString(sidnum,"");
	Cursor* c = new Cursor(palette, sidnum, "random", this, pos, depth, 1.0f);
	NosuchDebug("Addrandom creating new Cursor");
	cursorDown(c);
	cursorUp(c);
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

double scale_z(PaletteHost* ph, double z) {
	// We want the z value to be scaled exponentially toward 1.0,
	// i.e. raw z of .5 should result in a scale_z value of .75
	Palette* p = ph->palette();
	double expz = 1.0f - pow((1.0-z),p->params.zexponential);
	// NosuchDebug("scale_z z=%f expz=%f",z,expz);
	return expz * p->params.zmultiply;
}

void Region::buttonDown(std::string name) {
	palette->buttonDown(name);
	if ( _graphicBehaviour->isMyButton(name) ) {
		NosuchDebug("calling buttonDown name=%s of graphicBehaviour = %s",name.c_str(),_graphicBehaviour->name().c_str());
		_graphicBehaviour->buttonDown(name);
	}
	if ( _musicBehaviour->isMyButton(name) ) {
		NosuchDebug("calling buttonDown name=%s of musicBehaviour = %s",name.c_str(),_musicBehaviour->name().c_str());
		_musicBehaviour->buttonDown(name);
	}
}

void Region::buttonUp(std::string name) {
	palette->buttonUp(name);
	if ( _graphicBehaviour->isMyButton(name) ) {
		NosuchDebug("calling buttonUp of graphicBehaviour = %s",_graphicBehaviour->name().c_str());
		_graphicBehaviour->buttonUp(name);
	}
	if ( _musicBehaviour->isMyButton(name) ) {
		_musicBehaviour->buttonUp(name);
	}
}

void Region::cursorDown(Cursor* c) {
	if ( isButton() ) {
		NosuchDebug(1,"Region cursorDown depth=%f",c->curr_depth);
		if ( buttonTooDeep(c) ) {
			NosuchDebug("Ignoring cursor_down for button, too deep! (%.4f)",c->curr_depth);
		} else {
			NosuchDebug("REGION::BUTTONDOWN %s now=%.3f sid=%d/%s area=%.4f",
				name.c_str(),Palette::now/1000.0f,c->sidnum(),c->sidsource().c_str(),c->area());
			if ( c->area() < 0.0 ) {
				NosuchDebug("HEY!!!! area is negative!???");
			}
			buttonDown(name);
		}
	} else if ( Palette::selector_check && _graphicBehaviour->isSelectorDown() ) {
		NosuchDebug(1,"REGION::CURSORDOWN WITH GRAPHIC SELECTOR %s",c->DebugBrief().c_str());
		_graphicBehaviour->cursorDownWithSelector(c);
	} else if ( Palette::selector_check && _musicBehaviour->isSelectorDown() ) {
		NosuchDebug(1,"REGION::CURSORDOWN WITH MUSIC SELECTOR %s",c->DebugBrief().c_str());
		_musicBehaviour->cursorDownWithSelector(c);
	} else {
		NosuchDebug(2,"REGION::CURSORDOWN %s",c->DebugBrief().c_str());
		NosuchVector pos = c->curr_pos;
		palette->cursorDown(c);
		// NosuchDebug("cursorDown checking isSelectorDown() = %d",_graphicBehaviour->isSelectorDown());
		_graphicBehaviour->cursorDown(c);
		_musicBehaviour->cursorDown(c);
		palette->SetMostRecentCursorDown(c);
	}
}

void Region::cursorDrag(Cursor* c) {
	if ( isButton() ) {
		// Buttons aren't dragged
		std::string bn = name;
		if ( palette->isButtonDown(bn) != true ) {
			// This usually happens when the cursorDown is "too deep"
			NosuchDebug(2,"Hmmm, got cursorDrag for button=%s when _buttonDown=false?",bn.c_str());
		}
	} else if ( Palette::selector_check && _graphicBehaviour->isSelectorDown() ) {
		NosuchDebug(2,"REGION::CURSORDRAG WITH GRAPHIC SELECTOR %s",c->DebugBrief().c_str());
		_graphicBehaviour->cursorDragWithSelector(c);
	} else if ( Palette::selector_check && _musicBehaviour->isSelectorDown() ) {
		NosuchDebug(2,"REGION::CURSORDRAG WITH MUSIC SELECTOR %s",c->DebugBrief().c_str());
		_musicBehaviour->cursorDragWithSelector(c);
	} else {
		NosuchDebug(2,"REGION::CURSORDRAG %s",c->DebugBrief().c_str());
		palette->cursorDrag(c);
		_graphicBehaviour->cursorDrag(c);
		_musicBehaviour->cursorDrag(c);
	}
}

void Region::cursorUp(Cursor* c) {
	if ( c == palette->MostRecentCursorDown() ) {
		palette->SetMostRecentCursorDown(NULL);
	}
	if ( isButton() ) {
		if ( buttonTooDeep(c) ) {
			NosuchDebug("Ignoring button up, too deep!");
		} else if ( palette->isButtonDown(name) != true ) {
			NosuchDebug("Ignoring button up, isButtonDown wasn't true!");
		} else {
			NosuchDebug("REGION::BUTTONUP %s %s",name.c_str(),c->DebugBrief().c_str());
			buttonUp(name);
		}
	} else if ( Palette::selector_check && _graphicBehaviour->isSelectorDown() ) {
		NosuchDebug(1,"REGION::CURSORUP WITH GRAPHIC SELECTOR %s",c->DebugBrief().c_str());
		_graphicBehaviour->cursorUpWithSelector(c);
	} else if ( Palette::selector_check && _musicBehaviour->isSelectorDown() ) {
		NosuchDebug(1,"REGION::CURSORUP WITH MUSIC SELECTOR %s",c->DebugBrief().c_str());
		_musicBehaviour->cursorUpWithSelector(c);
	} else {
		NosuchVector pos = c->curr_pos;
		palette->cursorUp(c);
		NosuchDebug(2,"REGION::CURSORUP %s",c->DebugBrief().c_str());
		_graphicBehaviour->cursorUp(c);
		_musicBehaviour->cursorUp(c);
	}
}

void Region::checkCursorUp(int millinow) {
	
	if ( ! cursorlist_lock_write() ) {
		NosuchDebug("Region::checkCursorUp, unable to lock cursorlist");
		return;
	}
	for ( std::list<Cursor*>::iterator i = _cursors.begin(); i!=_cursors.end(); ) {
		Cursor* c = *i;
		NosuchAssert(c);
		int dt = millinow - c->touched();
		if (dt > 4) {
			// NosuchDebug("checkCursorUp, dt>4 = %d",dt);
			cursorUp(c);
			i = _cursors.erase(i);
			// XXX - should c be deleted?
			delete c;
		} else {
			i++;
		}
	}
	cursorlist_unlock();
}

double
Region::spriteMoveDir(Cursor* c)
{
	double dir;
	if ( c != NULL && params.movedir == "cursor" ) {
		dir = c->curr_degrees;
		// not sure why I have to reverse it - the cursor values are probably reversed
		dir -= 90.0;
		if ( dir < 0.0 ) {
			dir += 360.0;
		}
	} else {
		dir = getMoveDir(params.movedir);
	}
	return dir;
}

void
Region::instantiateSprite(Cursor* c, bool throttle) {

	std::string shape = params.shape;

	int tm = Palette::now;
	int dt = tm - c->last_instantiate();

	if ( throttle && (dt < SPRITE_THROTTLE_MS_PER_CURSOR ) ) {
		// NosuchDebug("THROTTLE is avoiding making a new sprite at tm=%d",tm);
		return;
	}

	Sprite* s = NULL;
	if ( shape == "square" ) {
		s = new SpriteSquare();
	} else if ( shape == "triangle" ) {
		s = new SpriteTriangle();
	} else if ( shape == "circle" ) {
		s = new SpriteCircle();
	} else if ( shape == "outline" ) {
		OutlineMem* om = c->outline();
		if ( om ) {
			SpriteOutline* so = new SpriteOutline();
			MMTT_SharedMemHeader* hdr = palette->paletteHost()->outlines_memory();
			so->setOutline(c->outline(),hdr);
			s = (Sprite*) so;
		}
	} else if ( shape == "curve" ) {
		s = new SpriteCurve();
	} else if ( shape == "line" ) {
		s = new SpriteLine();
	} else if ( shape == "arc" ) {
		NosuchDebug(1,"Sprite arc need to be finished!\n");
		s = new SpriteSquare();
	} else if ( shape == "nothing" ) {
		//
	} else {
		throw NosuchException("Unrecognized type of shape: %s",shape.c_str());
	}

	if ( s ) {
		s->params.initValues(this);
		s->initState(c->sidnum(),c->sidsource(),c->curr_pos,spriteMoveDir(c),c->curr_depth);
		c->set_last_instantiate(tm);
		// NosuchDebug("instantiateSprite c->curr_degrees=%.4lf direction=%.4lf",c->curr_degrees,s->state.direction);
		if ( params.shape == "curve" ) {
			c->curr_behaviour = "accumulate";
			s->startAccumulate(c);
		}
		spritelist->add(s,params.nsprites);
	}
}

void Region::hitSprites() {
	spritelist->hit();
}

#if 0
void Region::accumulateSpritesForCursor(Cursor* c) {
	spritelist_lock_write();
	for ( std::list<Sprite*>::iterator i = sprites.begin(); i!=sprites.end(); i++) {
		Sprite* s = *i;
		NosuchAssert(s);
		if ( s->state.sidnum == c->sidnum() && s->state.sidsource == c->sidsource() ) {
			s->accumulate(c);
		}
	}
	spritelist_unlock();
}
#endif

bool Region::cursorlist_lock_read() {
#ifndef DO_OSC_AND_HTTP_IN_MAIN_THREAD
	int e = pthread_rwlock_rdlock(&cursorlist_rwlock);
	if ( e != 0 ) {
		NosuchDebug("cursorlist_rwlock for read failed!? e=%d",e);
		return false;
	}
	NosuchDebug(2,"cursorlist_rwlock for read succeeded");
#endif
	return true;
}

bool Region::cursorlist_lock_write() {
#ifndef DO_OSC_AND_HTTP_IN_MAIN_THREAD
	int e = pthread_rwlock_wrlock(&cursorlist_rwlock);
	if ( e != 0 ) {
		NosuchDebug("cursorlist_rwlock for write failed!? e=%d",e);
		return false;
	}
	NosuchDebug(2,"cursorlist_rwlock for write succeeded");
#endif
	return true;
}

void Region::cursorlist_unlock() {
#ifndef DO_OSC_AND_HTTP_IN_MAIN_THREAD
	int e = pthread_rwlock_unlock(&cursorlist_rwlock);
	if ( e != 0 ) {
		NosuchDebug("cursorlist_rwlock unlock failed!? e=%d",e);
		return;
	}
	NosuchDebug(2,"cursorlist_rwlock unlock succeeded");
#endif
}

void Region::draw() {

	spritelist->draw();
#if 0
	spritelist_lock_read();
	for ( std::list<Sprite*>::iterator i = sprites.begin(); i!=sprites.end(); i++ ) {
		Sprite* s = *i;
		NosuchAssert(s);
		s->draw(b,this);
	}
	spritelist_unlock();
#endif
}

void
Region::getCursors(std::list<Cursor*>& cursors ) {
	for ( std::list<Cursor*>::iterator i = _cursors.begin(); i!=_cursors.end(); i++ ) {
		Cursor* c = *i;
		NosuchAssert(c);
		cursors.push_back(c);
	}
}

void Region::advanceTo(int tm) {
	
	spritelist->advanceTo(tm);
#if 0
	spritelist_lock_write();

	for ( std::list<Sprite*>::iterator i = sprites.begin(); i!=sprites.end(); ) {
		Sprite* s = *i;
		NosuchAssert(s);
		s->advanceTo(tm);
		if ( s->state.killme ) {
			i = sprites.erase(i);
			// NosuchDebug("Should be deleting Sprite s=%d",(int)s);
			delete s;
		} else {
			i++;
		}
	}

	spritelist_unlock();
#endif
	
	if ( last_tm > 0 && isSurface() ) {
		int dt = leftover_tm + tm - last_tm;
		if ( dt > fire_period ) {
			// NosuchDebug("Region %d calling behave->periodicFire now=%d",this->id,Palette::now);
			if ( Palette::selector_check && (_graphicBehaviour->isSelectorDown() || _musicBehaviour->isSelectorDown()) ) {
				NosuchDebug(2,"NOT calling behaviour->advanceTo because Selector is down");
			} else {
				_graphicBehaviour->advanceTo(tm);
				_musicBehaviour->advanceTo(tm);
			}
			dt -= fire_period;
		}
		leftover_tm = dt % fire_period;
	}
	last_tm = tm;
}

NosuchScheduler* Region::scheduler() { return palette->paletteHost()->scheduler(); }

#if 0
int Region::NumberScheduled(click_t minclicks, click_t maxclicks, std::string sid) {
	return palette->paletteHost()->NumberScheduled(minclicks,maxclicks,sid);
}
#endif

void Region::OutputNotificationMidiMsg(MidiMsg* mm, int sidnum) {
	// This is intended to be used to generate graphics from MIDI output.
	// By being attached to all output, it will reflect looped notes as well as direct input.

	// NosuchDebug(1,"Region::OutputNotificationMidiMsg is disabled...");
	// _noteBehaviour->gotMidiMsg(mm,sid);
}