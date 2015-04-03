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
	m_params = new AllVizParams(true);  // loads it with default values
	m_params->applyVizParamsFrom(sp);
	m_framenum = 0;
}


VizSprite::~VizSprite() {
}

double VizSprite::vertexNoise()
{
	if (m_params->noisevertex > 0.0f) {
		return m_params->noisevertex * RANDDOUBLE * ((rand() % 2) == 0 ? 1 : -1);
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
	if (m_params->zable && m_state.pos.z != CURSOR_Z_UNSET) {
		double zmultiply = 1.0f;
		double zexponential = 2.0f;
		double scaled_z = scale_z(m_state.pos.z, zexponential, zmultiply);
		draw(p, scaled_z);
	}
	else {
		draw(p, 0.5);
	}
}

void VizSprite::draw(NosuchGraphics* graphics, double scaled_z) {
	if (!m_state.visible) {
		DEBUGPRINT(("VizSprite.draw NOT DRAWING, !visible"));
		return;
	}
	// double hue = state.hueoffset + params->hueinitial;
	// double huefill = state.hueoffset + params->huefillinitial;

	NosuchColor color = NosuchColor(m_state.hue, m_params->luminance, m_params->saturation);
	NosuchColor colorfill = NosuchColor(m_state.huefill, m_params->luminance, m_params->saturation);

	if (m_state.alpha <= 0.0f || m_state.size <= 0.0) {
		m_state.killme = true;
		return;
	}

	if (m_params->filled) {
		graphics->fill(colorfill, m_state.alpha);
	}
	else {
		graphics->noFill();
	}
	graphics->stroke(color, m_state.alpha);
	if (m_state.size < 0.001f) {
		m_state.killme = true;
		return;
	}
	double thickness = m_params->thickness;
	graphics->strokeWeight(thickness);
	double aspect = m_params->aspect;

	// double scaled_z = region->scale_z(state.depth);

	double scalex = m_state.size * scaled_z;
	double scaley = m_state.size * scaled_z;

	scalex *= aspect;

	double x;
	double y;
	int xdir;
	int ydir;
	std::string mirror = m_params->mirror.get();
	if (mirror == "four") {
		x = m_state.pos.x;
		y = m_state.pos.y;
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
		x = m_state.pos.x;
		y = m_state.pos.y;
		xdir = 1;
		ydir = 1;
		drawAt(graphics, x, y, scalex, scaley, xdir, ydir);
		// y = (1.0f-state.pos.y) * appheight;
		ydir = -1;
		drawAt(graphics, x, 1.0 - y, scalex, scaley, xdir, ydir);
	}
	else if (mirror == "horizontal") {
		x = m_state.pos.x;
		y = m_state.pos.y;
		xdir = 1;
		ydir = 1;
		drawAt(graphics, x, y, scalex, scaley, xdir, ydir);
		xdir = -1;
		drawAt(graphics, 1.0 - x, y, scalex, scaley, xdir, ydir);
	}
	else {
		x = m_state.pos.x;
		y = m_state.pos.y;
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
	double degrees = m_params->rotanginit + m_state.rotangsofar;
	graphics->rotate(degrees);
	drawShape(graphics, xdir, ydir);
	graphics->popMatrix();
}

NosuchPos VizSprite::deltaInDirection(double dt, double dir, double speed) {
	NosuchPos delta((double)cos(degree2radian(dir)), (double)sin(degree2radian(dir)), 0.0f);
	delta = delta.normalize();
	delta = delta.mult(dt * speed);
	return delta;
}

static double
envelopeValue(double initial, double final, double duration, double born, double tm) {
	double dt = tm - born;
	double dur = duration;
	if (dt >= dur)
		return final;
	if (dt <= 0)
		return initial;
	return initial + (final - initial) * ((tm - born) / (dur));
}

void VizSprite::advanceTo(double tm) {

	m_state.alpha = envelopeValue(m_params->alphainitial, m_params->alphafinal, m_params->alphatime, m_state.born, tm);
	m_state.size = envelopeValue(m_params->sizeinitial, m_params->sizefinal, m_params->sizetime, m_state.born, tm);

	double age = (tm - m_state.born);
	double life = m_params->lifetime;
	if (m_params->lifetime >= 0.0 && age > m_params->lifetime) {
		m_state.killme = true;
	}
	double dt = (double)(tm - m_state.last_tm);
	m_state.last_tm = tm;

	if (!m_state.visible) {
		return;
	}

	m_state.hue = envelopeValue(m_params->hueinitial, m_params->huefinal, m_params->huetime, m_state.born, tm);
	m_state.huefill = envelopeValue(m_params->huefillinitial, m_params->huefillfinal, m_params->huefilltime, m_state.born, tm);

	// state.hueoffset = fmod((state.hueoffset + params->cyclehue), 360.0);

	if (m_state.stationary) {
		DEBUGPRINT2(("VizSprite %d is stationary", this));
		return;
	}

	if (m_params->rotangspeed != 0.0) {
		m_state.rotangsofar = fmod((m_state.rotangsofar + (m_state.rotclockwise ? 1 : -1) * (dt / 1000.0) * m_params->rotangspeed), 360.0);
	}

	if (m_params->gravity) {
		double mass = m_params->mass;
		if (mass == 0.0) {
			mass = 0.001;
		}
		m_state.speedX += dt * m_state.forceX / mass;
		m_state.speedY += dt * m_state.forceY / mass;
		if (_isnan(m_state.speedX)) {
			DEBUGPRINT(("Updated speedX to NAN! %f", m_state.speedX));
		}
		if (_isnan(m_state.speedY)) {
			DEBUGPRINT(("Updated speedY to NAN! %f", m_state.speedY));
		}
	}

	if (m_state.speedX != 0.0 || m_state.speedY != 0.0) {

		NosuchPos npos = m_state.pos;
		npos.x += dt * m_state.speedX;
		npos.y += dt * m_state.speedY;
		if (_isnan(npos.x)) {
			DEBUGPRINT(("Updated pos to NAN! %.4f %.4f", npos.x, npos.y));
		}
		else {
			// DEBUGPRINT(("Updated pos to %.4f %.4f",npos.x,npos.y));
		}

		if (m_params->bounce) {

			if (npos.x > 1.0f) {
				m_state.speedX = -m_state.speedX;
				npos.x += 2 * dt * m_state.speedX;
			}
			if (npos.x < 0.0f) {
				m_state.speedX = -m_state.speedX;
				npos.x += 2 * dt * m_state.speedX;
			}
			if (npos.y > 1.0f) {
				m_state.speedY = -m_state.speedY;
				npos.y += 2 * dt * m_state.speedX;
			}
			if (npos.y < 0.0f) {
				m_state.speedY = -m_state.speedY;
				npos.y += 2 * dt * m_state.speedX;
			}
		}
		else {
			if (npos.x > 1.0f || npos.x < 0.0f || npos.y > 1.0f || npos.y < 0.0f) {
				m_state.killme = true;
			}
		}

		m_state.pos = npos;
	}
}

VizSpriteList::VizSpriteList() {
	m_rwlock = PTHREAD_RWLOCK_INITIALIZER;
	int rc1 = pthread_rwlock_init(&m_rwlock, NULL);
	if (rc1) {
		DEBUGPRINT(("Failure on pthread_rwlock_init!? rc=%d", rc1));
	}
	DEBUGPRINT2(("rwlock has been initialized"));
}

void
VizSpriteList::lock_read() {
	int e = pthread_rwlock_rdlock(&m_rwlock);
	if (e != 0) {
		DEBUGPRINT(("rwlock for read failed!? e=%d", e));
		return;
	}
}

void
VizSpriteList::lock_write() {
	int e = pthread_rwlock_wrlock(&m_rwlock);
	if (e != 0) {
		DEBUGPRINT(("rwlock for write failed!? e=%d", e));
		return;
	}
}

void
VizSpriteList::unlock() {
	int e = pthread_rwlock_unlock(&m_rwlock);
	if (e != 0) {
		DEBUGPRINT(("rwlock unlock failed!? e=%d", e));
		return;
	}
}

void VizSpriteList::hit() {
	lock_write();
	for (std::list<VizSprite*>::iterator i = m_sprites.begin(); i != m_sprites.end(); i++) {
		VizSprite* s = *i;
		NosuchAssert(s);
		s->m_state.alpha = 1.0;
	}
	unlock();
}

void VizSpriteList::computeForces() {
	// We assume we're being called with lock_write() already done.
	bool anyGravity = false;
	for (std::list<VizSprite*>::iterator i1 = m_sprites.begin(); i1 != m_sprites.end(); i1++) {
		VizSprite* s1 = *i1;
		s1->m_state.forceX = s1->m_state.forceY = 0.0;
		if (s1->m_params->gravity) {
			anyGravity = true;
		}
	}
	if (!anyGravity) {
		return;
	}
	for (std::list<VizSprite*>::iterator i1 = m_sprites.begin(); i1 != m_sprites.end(); i1++) {
		VizSprite* s1 = *i1;
		NosuchAssert(s1);
		if (!s1->m_params->gravity) {
			continue;
		}
		for (std::list<VizSprite*>::iterator i2 = m_sprites.begin(); i2 != m_sprites.end(); i2++) {
			VizSprite* s2 = *i2;
			NosuchAssert(s2);
			if (s2 == s1) {   // this assumes the iterators use the same order
				break;
			}
			if (!s2->m_params->gravity) {
				continue;
			}
			double dx = s1->m_state.pos.x - s2->m_state.pos.x;
			double dy = s1->m_state.pos.y - s2->m_state.pos.y;
			double dist = sqrt(dx*dx + dy*dy);
#define MIN_DIST 0.001
			if (dist < MIN_DIST) {
				dist = MIN_DIST;
			}
			double EPS = 0.01;  // softening parameter (to avoid infinities)
			double f = (Grav * s1->m_params->mass * s2->m_params->mass) / (dist*dist + EPS*EPS);

			s1->m_state.forceX -= f * dx / dist;
			s1->m_state.forceY -= f * dy / dist;
			if (_isnan(s1->m_state.forceX)) {
				DEBUGPRINT(("Updated s1->forceX to NAN!"));
			}
			if (_isnan(s1->m_state.forceY)) {
				DEBUGPRINT(("Updated s1->forceY to NAN!"));
			}

			s2->m_state.forceX += f * dx / dist;
			s2->m_state.forceY += f * dy / dist;
			if (_isnan(s2->m_state.forceX)) {
				DEBUGPRINT(("Updated s2->forceX to NAN!"));
			}
			if (_isnan(s2->m_state.forceY)) {
				DEBUGPRINT(("Updated s2->forceY to NAN!"));
			}
		}
	}
}

void
VizSpriteList::add(VizSprite* s, int limit)
{
	lock_write();
	m_sprites.push_front(s);
	NosuchAssert(limit >= 1);
	while ((int)m_sprites.size() > limit) {
		VizSprite* ps = m_sprites.back();
		m_sprites.pop_back();
		delete ps;
	}
	s->m_state.visible = true;
	unlock();
}

void
VizSpriteList::draw(NosuchGraphics* b) {
	lock_read();
	for (std::list<VizSprite*>::iterator i = m_sprites.begin(); i != m_sprites.end(); i++) {
		VizSprite* s = *i;
		NosuchAssert(s);
		s->draw(b);
	}
	unlock();
}

void
VizSpriteList::advanceTo(double tm) {
	if (m_sprites.size() == 0) {
		return;
	}
	lock_write();
	computeForces();
	for (std::list<VizSprite*>::iterator i = m_sprites.begin(); i != m_sprites.end();) {
		VizSprite* s = *i;
		NosuchAssert(s);
		s->advanceTo(tm);
		if (s->m_state.killme) {
			i = m_sprites.erase(i);
			delete s;
		}
		else {
			i++;
		}
	}
	unlock();
}

VizSpriteSquare::VizSpriteSquare(AllVizParams* sp) : VizSprite(sp) {
	m_noise_x0 = vertexNoise();
	m_noise_y0 = vertexNoise();
	m_noise_x1 = vertexNoise();
	m_noise_y1 = vertexNoise();
	m_noise_x2 = vertexNoise();
	m_noise_y2 = vertexNoise();
	m_noise_x3 = vertexNoise();
	m_noise_y3 = vertexNoise();
}

void VizSpriteSquare::drawShape(NosuchGraphics* graphics, int xdir, int ydir) {
	double halfw = 0.2f;
	double halfh = 0.2f;

	double x0 = -halfw + m_noise_x0 * halfw;
	double y0 = -halfh + m_noise_y0 * halfh;
	double x1 = -halfw + m_noise_x1 * halfw;
	double y1 = halfh + m_noise_y1 * halfh;
	double x2 = halfw + m_noise_x2 * halfw;
	double y2 = halfh + m_noise_y2 * halfh;
	double x3 = halfw + m_noise_x3 * halfw;
	double y3 = -halfh + m_noise_y3 * halfh;
	DEBUGPRINT2(("drawing Square halfw=%.3f halfh=%.3f", halfw, halfh));
	graphics->quad(x0, y0, x1, y1, x2, y2, x3, y3);
}

VizSpriteTriangle::VizSpriteTriangle(AllVizParams* sp) : VizSprite(sp) {
	m_noise_x0 = vertexNoise();
	m_noise_y0 = vertexNoise();
	m_noise_x1 = vertexNoise();
	m_noise_y1 = vertexNoise();
	m_noise_x2 = vertexNoise();
	m_noise_y2 = vertexNoise();
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

	graphics->triangle(p1.x + m_noise_x0*sz, p1.y + m_noise_y0*sz,
		p2.x + m_noise_x1*sz, p2.y + m_noise_y1*sz,
		p3.x + m_noise_x2*sz, p3.y + m_noise_y2*sz, xdir, ydir);
}

VizSpriteLine::VizSpriteLine(AllVizParams* sp) : VizSprite(sp) {
	m_noise_x0 = vertexNoise();
	m_noise_y0 = vertexNoise();
	m_noise_x1 = vertexNoise();
	m_noise_y1 = vertexNoise();
}

void VizSpriteLine::drawShape(NosuchGraphics* graphics, int xdir, int ydir) {
	double halfw = 0.2f;
	double halfh = 0.2f;
	double x0 = -0.2f;
	double y0 = 0.0f;
	double x1 = 0.2f;
	double y1 = 0.0f;
	graphics->line(x0 + m_noise_x0, y0 + m_noise_y0, x1 + m_noise_x1, y1 + m_noise_y1);
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
	m_npoints = 0;
	m_points = NULL;
}

void
VizSpriteOutline::drawShape(NosuchGraphics* graphics, int xdir, int ydir) {
	// app->ellipse(0, 0, 0.2f, 0.2f);
	graphics->polygon(m_points, m_npoints, xdir, ydir);
	return;
}

void
VizSpriteOutline::setOutline(OutlineMem* om, MMTT_SharedMemHeader* hdr) {
	buff_index b = hdr->buff_to_display;
	PointMem* p = hdr->point(b, om->index_of_firstpoint);
	m_npoints = om->npoints;
	m_points = new PointMem[m_npoints];
	memcpy(m_points, p, m_npoints*sizeof(PointMem));
}

VizSpriteOutline::~VizSpriteOutline() {
	if (m_points) {
		delete m_points;
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

void VizSprite::initVizSpriteState(double tm, void* handle, NosuchPos& pos, double movedir) {

	// most of the state has been initialized in VizSpriteState constructor
	m_state.pos = pos;

	double rad = degree2radian(movedir);
	m_state.speedX = m_params->speedinitial * sin(rad);
	m_state.speedY = m_params->speedinitial * cos(rad);

	m_state.handle = handle;
	m_state.born = tm;
	m_state.last_tm = tm;
	m_state.hue = m_params->hueinitial;
	m_state.huefill = m_params->huefillinitial;
	m_state.alpha = m_params->alphainitial;
	m_state.size = m_params->sizeinitial;
	m_state.rotangspeed = m_params->rotangspeed;
	if (m_params->rotdirrandom.get() && ((rand() % 2) == 0)) {
		m_state.rotangspeed = -m_state.rotangspeed;
	}
}