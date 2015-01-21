#ifndef _SPRITE_H
#define _SPRITE_H

#include "AllVizParams.h"
#include <list>
#include <pthread.h>
#include "NosuchGraphics.h"

class VizSprite;
struct OutlineMem;
class MMTT_SharedMemHeader;
class b2Body;

class VizSpriteList {

public:
	VizSpriteList();
	void lock_read();
	void lock_write();
	void unlock();
	void draw(NosuchGraphics* g);
	void advanceTo(int tm);
	void computeForces();
	void hit();
	void add(VizSprite* s, int limit);
	int size() { return sprites.size(); }

	std::list<VizSprite*> sprites;
	pthread_rwlock_t rwlock;

};

class VizSpriteState {
public:
	VizSpriteState() {
		visible = false;
		// direction = 0.0;
		hue = 0.0f;
		huefill = 0.0f;
		pos = NosuchPos(0.0f, 0.0f, 0.0f);
		size = 0.5;
		alpha = 1.0;

		speedX = 0.0;
		speedY = 0.0;
		mass = 1.0;
		forceX = 0.0;
		forceY = 0.0;

		born = 0;
		last_tm = 0;
		killme = false;
		rotangsofar = 0.0f;
		rotangspeed = 0.0f;
		stationary = false;
		handle = NULL;
		rotclockwise = true;
	}
	bool visible;
	double hue;
	double huefill;
	NosuchPos pos;
	double size;
	double alpha;

	// double direction;
	double speedX;
	double speedY;
	double mass;
	double forceX;
	double forceY;

	int born;
	int last_tm;
	bool killme;
	double rotangsofar;
	double rotangspeed;
	bool stationary;
	void* handle;
	bool rotclockwise;

	double getSpeed() { return sqrt(speedX*speedX + speedY*speedY); }
};

static double Grav = 1.0e-5;

class VizSprite {

public:

	VizSprite(AllVizParams* sp);
	virtual ~VizSprite();

	static VizSprite* makeVizSprite(AllVizParams* sp);
	void initVizSpriteState(int millinow, void* handle, NosuchPos& pos, double movedir);


	static double degree2radian(double deg);
	virtual void drawShape(NosuchGraphics* graphics, int xdir, int ydir) = 0;
	virtual bool fixedScale() { return false; }

	// Screen space is 2.0x2.0, while cursor space is 1.0x1.0
	void scaleCursorSpaceToScreenSpace(NosuchVector& pos) {
		state.pos.x *= 2.0f;
		state.pos.y *= 2.0f;
	}

	void draw(NosuchGraphics* graphics);
	void drawAt(NosuchGraphics* app, double x, double y, double w, double h, int xdir, int ydir);
	NosuchPos deltaInDirection(double dt, double dir, double speed);
	void advanceTo(int tm);

#ifdef CURVE_STUFF
	virtual void startAccumulate(Cursor* c) { };
	virtual void accumulate(Cursor* c) { }
#endif

	AllVizParams* params;
	VizSpriteState state;
	b2Body* body;
	int frame;

protected:
	double vertexNoise();

private:
	void draw(NosuchGraphics* app, double scaled_z);
};

class VizSpriteSquare : public VizSprite {

public:
	VizSpriteSquare(AllVizParams* sp);
	void drawShape(NosuchGraphics* app, int xdir, int ydir);

private:
	double noise_x0;
	double noise_y0;
	double noise_x1;
	double noise_y1;
	double noise_x2;
	double noise_y2;
	double noise_x3;
	double noise_y3;
};

class VizSpriteTriangle : public VizSprite {

public:
	VizSpriteTriangle(AllVizParams* sp);
	void drawShape(NosuchGraphics* app, int xdir, int ydir);

private:
	double noise_x0;
	double noise_y0;
	double noise_x1;
	double noise_y1;
	double noise_x2;
	double noise_y2;
};

class VizSpriteCircle : public VizSprite {

public:
	VizSpriteCircle(AllVizParams* sp);
	void drawShape(NosuchGraphics* app, int xdir, int ydir);
};

class VizSpriteOutline : public VizSprite {

public:
	VizSpriteOutline(AllVizParams* sp);
	~VizSpriteOutline();
	void drawShape(NosuchGraphics* app, int xdir, int ydir);
	void setOutline(OutlineMem* om, MMTT_SharedMemHeader* hdr);
	int Npoints() { return _npoints; }
	PointMem* Points() { return _points; }

private:
	int _npoints;
	PointMem* _points;
};

class VizSpriteLine : public VizSprite {

public:
	VizSpriteLine(AllVizParams* sp);
	void drawShape(NosuchGraphics* app, int xdir, int ydir);

private:
	double noise_x0;
	double noise_y0;
	double noise_x1;
	double noise_y1;
};

class VizSpriteNothing : public VizSprite {

public:
	VizSpriteNothing(AllVizParams* sp);
	void drawShape(NosuchGraphics* app, int xdir, int ydir);
};

#endif
