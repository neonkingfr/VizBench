#include  <stdlib.h>
#include <vector>
#include <cstdlib> // for srand, rand

#include "PaletteAll.h"

long nspritereadlocks = 0;
long nspritewritelocks = 0;

std::vector<std::string> ChannelParams::mirrorTypes;
std::vector<std::string> ChannelParams::movedirTypes;
std::vector<std::string> ChannelParams::rotangdirTypes;
std::vector<std::string> ChannelParams::shapeTypes;
std::vector<std::string> ChannelParams::behaviourTypes;

void ChannelParams::Initialize() {

	shapeTypes.push_back("nothing");  // make sure that sprite 0 is nothing
	shapeTypes.push_back("line");
	shapeTypes.push_back("triangle");
	shapeTypes.push_back("square");
	shapeTypes.push_back("arc");
	shapeTypes.push_back("circle");
	shapeTypes.push_back("outline");
	shapeTypes.push_back("curve");

	movedirTypes.push_back("cursor");
	movedirTypes.push_back("left");
	movedirTypes.push_back("right");
	movedirTypes.push_back("up");
	movedirTypes.push_back("down");
	movedirTypes.push_back("random");

	rotangdirTypes.push_back("right");
	rotangdirTypes.push_back("left");
	rotangdirTypes.push_back("random");

	mirrorTypes.push_back("none");
	mirrorTypes.push_back("vertical");
	mirrorTypes.push_back("horizontal");
	mirrorTypes.push_back("four");

	behaviourTypes.push_back("default");
	behaviourTypes.push_back("horizontal");
	behaviourTypes.push_back("vertical");
	behaviourTypes.push_back("outline");
}

Channel::Channel(Palette* p, int i) {

	NosuchAssert( i >= 0 && i <= 15);

	spritelist = new SpriteList();
	id = i;

	_midiBehaviour = NULL;
	_midiBehaviourName = "";

	palette = p;
}

void
Channel::setMidiBehaviour(std::string b) {
	if ( b == _midiBehaviourName ) {
		return;
	}
	if ( _midiBehaviour != NULL ) {
		delete _midiBehaviour;
	}
	_midiBehaviourName = b;
	_midiBehaviour = ph->makeMidiBehaviour(b,this);
}

Channel::~Channel() {
	NosuchDebug("Channel DESTRUCTOR!");
	delete _midiBehaviour;
}

std::string Channel::shapeOfChannel(int id) {
	// shape 0 is "nothing", so avoid 0
	int i = 1 + id % (ChannelParams::shapeTypes.size()-1);
	std::string s = ChannelParams::shapeTypes[i];
	s = DEFAULT_SHAPE;
	return s;
}

double Channel::hueOfChannel(int id) {
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

#if 0
void Channel::touchCursor(int sidnum, std::string sidsource, int millinow) {
	Cursor* c = getCursor(sidnum, sidsource);
	if ( c != NULL ) {
		c->touch(millinow);
	}
}

Cursor* Channel::getCursor(int sidnum, std::string sidsource) {
	Cursor* retc = NULL;

	if ( ! cursorlist_lock_read() ) {
		NosuchDebug("Channel::getCursor returns NULL, unable to lock cursorlist");
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

double Channel::AverageCursorDepth() {
	double totval = 0;
	int totnum = 0;
	if ( ! cursorlist_lock_read() ) {
		NosuchDebug("Channel::AverageCursorDepth returns -1, unable to lock cursorlist");
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

double Channel::MaxCursorDepth() {
	double maxval = 0;
	if ( ! cursorlist_lock_read() ) {
		NosuchDebug("Channel::MaxCursorDepth returns -1, unable to lock cursorlist");
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
Channel::setCursor(int sidnum, std::string sidsource, int millinow, NosuchVector pos, double depth, double area, OutlineMem* om) {

	if ( pos.x < x_min || pos.x > x_max || pos.y < y_min || pos.y > y_max ) {
		NosuchDebug(2,"Ignoring out-of-bounds cursor pos=%f,%f,%f\n",pos.x,pos.y);
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
			NosuchDebug("Channel::setCursor unable to lock cursorlist");
		}
	}
	c->touch(millinow);
	return c;
}

void Channel::addrandom() {
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
#endif

double Channel::getMoveDir(std::string movedirtype) {
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
	if ( movedirtype == "cursor" ) {
		// random, not sure what it should be
		double f = ((double)(rand()))/ RAND_MAX;
		return f * 360.0f;
	}
	throw NosuchException("Unrecognized movedirtype value %s",movedirtype.c_str());
}

#if 0
double Channel::scale_z(double z) {
	// We want the z value to be scaled exponentially toward 1.0,
	// i.e. raw z of .5 should result in a scale_z value of .75
	double expz = 1.0f - pow((1.0-z),palette->params.zexponential);
	// NosuchDebug("scale_z z=%f expz=%f",z,expz);
	return expz * palette->params.zmultiply;
}
#endif

void
Channel::instantiateOutlines(int pitch) {

	std::list<Cursor*> cursors;
	palette->getCursors(cursors);
	// NosuchDebug("Channel::instantiateOutlines  # cursors = %d",cursors.size());
	int outlinenum = 0;
#define MAX_OUTLINES 100
	for ( std::list<Cursor*>::iterator i = cursors.begin(); i!=cursors.end(); i++ ) {
		if ( outlinenum++ >= MAX_OUTLINES ) {
			NosuchDebug("Too many outlines in instantiateOutlines!");
			break;
		}
		Cursor* c = *i;
		NosuchAssert(c);
		OutlineMem* om = c->outline();
		if ( !om ) {
			NosuchDebug("No c->outline() in instantiateOutline?");
			continue;
		}

		int sid = (channelnum()*MAX_OUTLINES*128) + (outlinenum*128) + pitch;

		SpriteOutline* so = new SpriteOutline();
		MMTT_SharedMemHeader* hdr = palette->paletteHost()->outlines_memory();
		so->setOutline(c->outline(),hdr);
		Sprite* s = (Sprite*) so;
		s->params.initValues(this);
		double movedir = getMoveDir(params.movedir);
		s->initState(sid,"midi",c->curr_pos,movedir,c->curr_depth);
		addSpriteToList(s);
	}
}


void
Channel::instantiateSprite(int pitch, int velocity, NosuchVector pos, double depth) {

	std::string shape = params.shape;
	Sprite* s = NULL;

	if ( shape == "square" ) {
		s = new SpriteSquare();
	} else if ( shape == "triangle" ) {
		s = new SpriteTriangle();
	} else if ( shape == "circle" ) {
		s = new SpriteCircle();
	} else if ( shape == "outline" ) {
		NosuchDebug("Hmmm?  instantiateSprite called with shape==outline?");
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
		double movedir = getMoveDir(params.movedir);
		int sid = channelnum() * 100000 + pitch;
		s->initState(sid,"midi",pos,movedir,depth);
		addSpriteToList(s);
	}
}

void
Channel::addSpriteToList(Sprite* s)
{
	spritelist->add(s,params.nsprites);
}

void Channel::spritelist_lock_read() {
	spritelist->lock_read();
}

void Channel::spritelist_lock_write() {
	spritelist->lock_write();
}

void Channel::spritelist_unlock() {
	spritelist->unlock();
}

void Channel::hitSprites() {
	spritelist->hit();
}

void Channel::draw() {
	spritelist->draw();
}

void Channel::gotNoteOn(int pitch, int velocity) {
	_midiBehaviour->NoteOn(pitch,velocity);
}

void Channel::advanceTo(int tm) {
	spritelist->advanceTo(tm);
}