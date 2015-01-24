/*
	Copyright (c) 2011-2013 Tim Thompson <me@timthompson.com>

	Permission is hereby granted, free of charge, to any person obtaining
	a copy of this software and associated documentation files
	(the "Software"), to deal in the Software without restriction,
	including without limitation the rights to use, copy, modify, merge,
	publish, distribute, sublicense, and/or sell copies of the Software,
	and to permit persons to whom the Software is furnished to do so,
	subject to the following conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	Any person wishing to distribute modifications to the Software is
	requested to send the modifications to the original developer so that
	they can be incorporated into the canonical version.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
	IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
	ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
	CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
	WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
	*/

#include <cstdlib> // for srand, rand

#include "NosuchUtil.h"
#include "NosuchGraphics.h"
#include "Vizlet.h"
#include "VizSprite.h"
#include "coxdeboor.h"

#define RANDDOUBLE (((double)rand())/RAND_MAX)

VizSprite::VizSprite(AllVizParams* sp) {
	// create a copy, since the params may change
	params = new AllVizParams(true);  // loads it with default values
	params->applyVizParamsFrom(sp);
	frame = 0;
}


VizSprite::~VizSprite() {
}

double VizSprite::vertexNoise()
{
	if (params->noisevertex > 0.0f) {
		return params->noisevertex * RANDDOUBLE * ((rand() % 2) == 0 ? 1 : -1);
	}
	else {
		return 0.0f;
	}
}

double VizSprite::degree2radian(double deg) {
	return 2.0f * (double)M_PI * deg / 360.0f;
}

double scale_z(double z, double zexp, double zmult) {
	// We want the z value to be scaled exponentially toward 1.0,
	// i.e. raw z of .5 should result in a scale_z value of .75
	double expz = 1.0f - pow((1.0 - z), zexp);
	return expz * zmult;
}

void VizSprite::draw(NosuchGraphics* p) {
	double scalehack = 0.5f;
	if (params->zable) {
		double zmultiply = 4.0f;
		double zexponential = 2.0f;
		double scaled_z = scale_z(state.pos.z, zexponential, zmultiply);
		draw(p, scaled_z*scalehack);
	}
	else {
		draw(p, 1.0*scalehack);
	}
}

void VizSprite::draw(NosuchGraphics* graphics, double scaled_z) {
	if (!state.visible) {
		DEBUGPRINT(("VizSprite.draw NOT DRAWING, !visible"));
		return;
	}
	// double hue = state.hueoffset + params->hueinitial;
	// double huefill = state.hueoffset + params->huefillinitial;

	NosuchColor color = NosuchColor(state.hue, params->luminance, params->saturation);
	NosuchColor colorfill = NosuchColor(state.huefill, params->luminance, params->saturation);

	if (state.alpha <= 0.0f || state.size <= 0.0) {
		state.killme = true;
		return;
	}

	if (params->filled) {
		graphics->fill(colorfill, state.alpha);
	}
	else {
		graphics->noFill();
	}
	graphics->stroke(color, state.alpha);
	if (state.size < 0.001f) {
		state.killme = true;
		return;
	}
	double thickness = params->thickness;
	graphics->strokeWeight(thickness);
	double aspect = params->aspect;

	// double scaled_z = region->scale_z(state.depth);

	double scalex = state.size * scaled_z;
	double scaley = state.size * scaled_z;

	scalex *= aspect;

	double x;
	double y;
	int xdir;
	int ydir;
	std::string mirror = params->mirror.get();
	if (mirror == "four") {
		x = state.pos.x;
		y = state.pos.y;
		xdir = 1;
		ydir = 1;
		drawAt(graphics, x, y, scalex, scaley, xdir, ydir);
		ydir = -1;
		drawAt(graphics, x, 1.0 - y, scalex, scaley, xdir, ydir);
		xdir = -1;
		drawAt(graphics, 1.0 - x, y, scalex, scaley, xdir, ydir);
		ydir = 1;
		drawAt(graphics, 1.0 - x, 1.0 - y, scalex, scaley, xdir, ydir);
	}
	else if (mirror == "vertical") {
		x = state.pos.x;
		y = state.pos.y;
		xdir = 1;
		ydir = 1;
		drawAt(graphics, x, y, scalex, scaley, xdir, ydir);
		// y = (1.0f-state.pos.y) * appheight;
		ydir = -1;
		drawAt(graphics, x, 1.0 - y, scalex, scaley, xdir, ydir);
	}
	else if (mirror == "horizontal") {
		x = state.pos.x;
		y = state.pos.y;
		xdir = 1;
		ydir = 1;
		drawAt(graphics, x, y, scalex, scaley, xdir, ydir);
		xdir = -1;
		drawAt(graphics, 1.0 - x, y, scalex, scaley, xdir, ydir);
	}
	else {
		x = state.pos.x;
		y = state.pos.y;
		xdir = 1;
		ydir = 1;
		drawAt(graphics, x, y, scalex, scaley, xdir, ydir);
	}
}

void VizSprite::drawAt(NosuchGraphics* graphics, double x, double y, double scalex, double scaley, int xdir, int ydir) {
	DEBUGPRINT1(("VizSprite::drawAt xy=   %.4lf   %.4lf", x, y));
	graphics->pushMatrix();
	graphics->translate(x, y);
	if (fixedScale()) {
		graphics->scale(1.0, 1.0);
	}
	else {
		graphics->scale(scalex, scaley);
	}
	double degrees = params->rotanginit + state.rotangsofar;
	graphics->rotate(degrees);
	drawShape(graphics, xdir, ydir);
	graphics->popMatrix();
}

NosuchPos VizSprite::deltaInDirection(double dt, double dir, double speed) {
	NosuchPos delta((double)cos(degree2radian(dir)), (double)sin(degree2radian(dir)), 0.0f);
	delta = delta.normalize();
	delta = delta.mult((dt / 1000.0f) * speed);
	return delta;
}

static double
envelopeValue(double initial, double final, double duration, double born, double now) {
	double dt = now - born;
	double dur = duration * 1000.0;
	if (dt >= dur)
		return final;
	if (dt <= 0)
		return initial;
	return initial + (final - initial) * ((now - born) / (dur));
}

void VizSprite::advanceTo(int now) {

	// _params->advanceTo(tm);
	state.alpha = envelopeValue(params->alphainitial, params->alphafinal, params->alphatime, state.born, now);
	state.size = envelopeValue(params->sizeinitial, params->sizefinal, params->sizetime, state.born, now);

	float age = (now - state.born) / 1000.0f;
	double life = params->lifetime;
	if (params->lifetime >= 0.0 && age > params->lifetime) {
		state.killme = true;
	}
	double dt = (double)(now - state.last_tm);
	state.last_tm = now;

	if (!state.visible) {
		return;
	}

	state.hue = envelopeValue(params->hueinitial, params->huefinal, params->huetime, state.born, now);
	state.huefill = envelopeValue(params->huefillinitial, params->huefillfinal, params->huefilltime, state.born, now);

	// state.hueoffset = fmod((state.hueoffset + params->cyclehue), 360.0);

	if (state.stationary) {
		DEBUGPRINT2(("VizSprite %d is stationary", this));
		return;
	}

	if (params->rotangspeed != 0.0) {
		state.rotangsofar = fmod((state.rotangsofar + (state.rotclockwise ? 1 : -1) * (dt / 1000.0) * params->rotangspeed), 360.0);
	}

	if (params->gravity) {
		state.speedX += dt * state.forceX / params->mass;
		state.speedY += dt * state.forceY / params->mass;
		if (_isnan(state.speedX)) {
			DEBUGPRINT(("Updated speedX to NAN! %f", state.speedX));
		}
		if (_isnan(state.speedY)) {
			DEBUGPRINT(("Updated speedY to NAN! %f", state.speedY));
		}
	}

	if (state.speedX != 0.0 || state.speedY != 0.0) {

		NosuchPos npos = state.pos;
		npos.x += dt * state.speedX / 1000.0;
		npos.y += dt * state.speedY / 1000.0;
		if (_isnan(npos.x)) {
			DEBUGPRINT(("Updated pos to NAN! %.4f %.4f", npos.x, npos.y));
		}
		else {
			// DEBUGPRINT(("Updated pos to %.4f %.4f",npos.x,npos.y));
		}

		if (params->bounce) {

			if (npos.x > 1.0f) {
				state.speedX = -state.speedX;
				npos.x += 2 * dt * state.speedX;
			}
			if (npos.x < 0.0f) {
				state.speedX = -state.speedX;
				npos.x += 2 * dt * state.speedX;
			}
			if (npos.y > 1.0f) {
				state.speedY = -state.speedY;
				npos.y += 2 * dt * state.speedX;
			}
			if (npos.y < 0.0f) {
				state.speedY = -state.speedY;
				npos.y += 2 * dt * state.speedX;
			}
		}
		else {
			if (npos.x > 1.0f || npos.x < 0.0f || npos.y > 1.0f || npos.y < 0.0f) {
				state.killme = true;
			}
		}

		state.pos = npos;
	}
}

VizSpriteList::VizSpriteList() {
	rwlock = PTHREAD_RWLOCK_INITIALIZER;
	int rc1 = pthread_rwlock_init(&rwlock, NULL);
	if (rc1) {
		DEBUGPRINT(("Failure on pthread_rwlock_init!? rc=%d", rc1));
	}
	DEBUGPRINT2(("rwlock has been initialized"));
}

void
VizSpriteList::lock_read() {
	int e = pthread_rwlock_rdlock(&rwlock);
	if (e != 0) {
		DEBUGPRINT(("rwlock for read failed!? e=%d", e));
		return;
	}
}

void
VizSpriteList::lock_write() {
	int e = pthread_rwlock_wrlock(&rwlock);
	if (e != 0) {
		DEBUGPRINT(("rwlock for write failed!? e=%d", e));
		return;
	}
}

void
VizSpriteList::unlock() {
	int e = pthread_rwlock_unlock(&rwlock);
	if (e != 0) {
		DEBUGPRINT(("rwlock unlock failed!? e=%d", e));
		return;
	}
}

void VizSpriteList::hit() {
	lock_write();
	for (std::list<VizSprite*>::iterator i = sprites.begin(); i != sprites.end(); i++) {
		VizSprite* s = *i;
		NosuchAssert(s);
		s->state.alpha = 1.0;
	}
	unlock();
}

void VizSpriteList::computeForces() {
	// We assume we're being called with lock_write() already done.
	bool anyGravity = false;
	for (std::list<VizSprite*>::iterator i1 = sprites.begin(); i1 != sprites.end(); i1++) {
		VizSprite* s1 = *i1;
		s1->state.forceX = s1->state.forceY = 0.0;
		if (s1->params->gravity) {
			anyGravity = true;
		}
	}
	if (!anyGravity) {
		return;
	}
	for (std::list<VizSprite*>::iterator i1 = sprites.begin(); i1 != sprites.end(); i1++) {
		VizSprite* s1 = *i1;
		NosuchAssert(s1);
		if (!s1->params->gravity) {
			continue;
		}
		for (std::list<VizSprite*>::iterator i2 = sprites.begin(); i2 != sprites.end(); i2++) {
			VizSprite* s2 = *i2;
			NosuchAssert(s2);
			if (s2 == s1) {   // this assumes the iterators use the same order
				break;
			}
			if (!s2->params->gravity) {
				continue;
			}
			double dx = s1->state.pos.x - s2->state.pos.x;
			double dy = s1->state.pos.y - s2->state.pos.y;
			double dist = sqrt(dx*dx + dy*dy);
#define MIN_DIST 0.001
			if (dist < MIN_DIST) {
				dist = MIN_DIST;
			}
			double EPS = 0.01;  // softening parameter (to avoid infinities)
			double f = (Grav * s1->params->mass * s2->params->mass) / (dist*dist + EPS*EPS);

			s1->state.forceX -= f * dx / dist;
			s1->state.forceY -= f * dy / dist;
			if (_isnan(s1->state.forceX)) {
				DEBUGPRINT(("Updated s1->forceX to NAN!"));
			}
			if (_isnan(s1->state.forceY)) {
				DEBUGPRINT(("Updated s1->forceY to NAN!"));
			}

			s2->state.forceX += f * dx / dist;
			s2->state.forceY += f * dy / dist;
			if (_isnan(s2->state.forceX)) {
				DEBUGPRINT(("Updated s2->forceX to NAN!"));
			}
			if (_isnan(s2->state.forceY)) {
				DEBUGPRINT(("Updated s2->forceY to NAN!"));
			}
		}
	}
}

void
VizSpriteList::add(VizSprite* s, int limit)
{
	lock_write();
	sprites.push_front(s);
	NosuchAssert(limit >= 1);
	while ((int)sprites.size() > limit) {
		VizSprite* ps = sprites.back();
		sprites.pop_back();
		delete ps;
	}
	s->state.visible = true;
	unlock();
}

void
VizSpriteList::draw(NosuchGraphics* b) {
	lock_read();
	for (std::list<VizSprite*>::iterator i = sprites.begin(); i != sprites.end(); i++) {
		VizSprite* s = *i;
		NosuchAssert(s);
		s->draw(b);
	}
	unlock();
}

void
VizSpriteList::advanceTo(int tm) {
	if (sprites.size() == 0) {
		return;
	}
	lock_write();
	computeForces();
	for (std::list<VizSprite*>::iterator i = sprites.begin(); i != sprites.end();) {
		VizSprite* s = *i;
		NosuchAssert(s);
		s->advanceTo(tm);
		if (s->state.killme) {
			i = sprites.erase(i);
			delete s;
		}
		else {
			i++;
		}
	}
	unlock();
}

VizSpriteSquare::VizSpriteSquare(AllVizParams* sp) : VizSprite(sp) {
	noise_x0 = vertexNoise();
	noise_y0 = vertexNoise();
	noise_x1 = vertexNoise();
	noise_y1 = vertexNoise();
	noise_x2 = vertexNoise();
	noise_y2 = vertexNoise();
	noise_x3 = vertexNoise();
	noise_y3 = vertexNoise();
}

void VizSpriteSquare::drawShape(NosuchGraphics* graphics, int xdir, int ydir) {
	double halfw = 0.2f;
	double halfh = 0.2f;

	double x0 = -halfw + noise_x0 * halfw;
	double y0 = -halfh + noise_y0 * halfh;
	double x1 = -halfw + noise_x1 * halfw;
	double y1 = halfh + noise_y1 * halfh;
	double x2 = halfw + noise_x2 * halfw;
	double y2 = halfh + noise_y2 * halfh;
	double x3 = halfw + noise_x3 * halfw;
	double y3 = -halfh + noise_y3 * halfh;
	DEBUGPRINT2(("drawing Square halfw=%.3f halfh=%.3f", halfw, halfh));
	graphics->quad(x0, y0, x1, y1, x2, y2, x3, y3);
}

VizSpriteTriangle::VizSpriteTriangle(AllVizParams* sp) : VizSprite(sp) {
	noise_x0 = vertexNoise();
	noise_y0 = vertexNoise();
	noise_x1 = vertexNoise();
	noise_y1 = vertexNoise();
	noise_x2 = vertexNoise();
	noise_y2 = vertexNoise();
}

void VizSpriteTriangle::drawShape(NosuchGraphics* graphics, int xdir, int ydir) {
	// double halfw = w / 2.0f;
	// double halfh = h / 2.0f;
	double sz = 0.2f;
	NosuchVector p1 = NosuchVector(sz, 0.0f);
	NosuchVector p2 = p1;
	p2 = p2.rotate(VizSprite::degree2radian(120));
	NosuchVector p3 = p1;
	p3 = p3.rotate(VizSprite::degree2radian(-120));

	graphics->triangle(p1.x + noise_x0*sz, p1.y + noise_y0*sz,
		p2.x + noise_x1*sz, p2.y + noise_y1*sz,
		p3.x + noise_x2*sz, p3.y + noise_y2*sz, xdir, ydir);
}

VizSpriteLine::VizSpriteLine(AllVizParams* sp) : VizSprite(sp) {
	noise_x0 = vertexNoise();
	noise_y0 = vertexNoise();
	noise_x1 = vertexNoise();
	noise_y1 = vertexNoise();
}

void VizSpriteLine::drawShape(NosuchGraphics* graphics, int xdir, int ydir) {
	double halfw = 0.2f;
	double halfh = 0.2f;
	double x0 = -0.2f;
	double y0 = 0.0f;
	double x1 = 0.2f;
	double y1 = 0.0f;
	graphics->line(x0 + noise_x0, y0 + noise_y0, x1 + noise_x1, y1 + noise_y1);
}

VizSpriteCircle::VizSpriteCircle(AllVizParams* sp) : VizSprite(sp) {
}

void VizSpriteCircle::drawShape(NosuchGraphics* graphics, int xdir, int ydir) {
	graphics->ellipse(0, 0, 0.2f, 0.2f);
}

VizSpriteNothing::VizSpriteNothing(AllVizParams* sp) : VizSprite(sp) {
}

void VizSpriteNothing::drawShape(NosuchGraphics* graphics, int xdir, int ydir) {
}

VizSpriteOutline::VizSpriteOutline(AllVizParams* sp) : VizSprite(sp) {
	_npoints = 0;
	_points = NULL;
}

void
VizSpriteOutline::drawShape(NosuchGraphics* graphics, int xdir, int ydir) {
	// app->ellipse(0, 0, 0.2f, 0.2f);
	graphics->polygon(_points, _npoints, xdir, ydir);
	return;
}

void
VizSpriteOutline::setOutline(OutlineMem* om, MMTT_SharedMemHeader* hdr) {
	buff_index b = hdr->buff_to_display;
	PointMem* p = hdr->point(b, om->index_of_firstpoint);
	_npoints = om->npoints;
	_points = new PointMem[_npoints];
	memcpy(_points, p, _npoints*sizeof(PointMem));
}

VizSpriteOutline::~VizSpriteOutline() {
	if (_points) {
		delete _points;
	}
}

VizSprite*
VizSprite::makeVizSprite(AllVizParams* sp)
{
	std::string shape = sp->shape;
	VizSprite* s = NULL;

	if (shape == "square") {
		s = new VizSpriteSquare(sp);
	}
	else if (shape == "triangle") {
		s = new VizSpriteTriangle(sp);
	}
	else if (shape == "circle") {
		s = new VizSpriteCircle(sp);
	}
	else if (shape == "outline") {
		s = new VizSpriteOutline(sp);
	}
	else if (shape == "line") {
		s = new VizSpriteLine(sp);
	}
	else if (shape == "arc") {
		DEBUGPRINT1(("VizSprite arc need to be finished!\n"));
		s = new VizSpriteSquare(sp);
	}
	else if (shape == "nothing") {
		s = new VizSpriteNothing(sp);
	}
	else {
		DEBUGPRINT(("makeVizSprite - Unrecognized type of shape: %s, assuming nothing", shape.c_str()));
		s = new VizSpriteNothing(sp);
	}
	return s;
}

int rotangdirOf(std::string s) {
	int dir = 1;
	if (s == "right") {
		dir = 1;
	}
	else if (s == "left") {
		dir = -1;
	}
	else if (s == "random") {
		dir = ((rand() % 2) == 0) ? 1 : -1;
	}
	else {
		DEBUGPRINT(("rotangdirOf: Bad value - %s, assuming random", s.c_str()));
		dir = ((rand() % 2) == 0) ? 1 : -1;
	}
	return dir;
}

void VizSprite::initVizSpriteState(int millinow, void* handle, NosuchPos& pos, double movedir) {

	// most of the state has been initialized in VizSpriteState constructor
	state.pos = pos;

	double rad = degree2radian(movedir);
	state.speedX = params->speedinitial * sin(rad);
	state.speedY = params->speedinitial * cos(rad);

	state.handle = handle;
	state.born = millinow;
	state.last_tm = millinow;
	state.hue = params->hueinitial;
	state.huefill = params->huefillinitial;
	state.alpha = params->alphainitial;
	state.size = params->sizeinitial;
	state.rotangspeed = params->rotangspeed;
	if (params->rotdirrandom.get() && ((rand() % 2) == 0)) {
		state.rotangspeed = -state.rotangspeed;
	}
}