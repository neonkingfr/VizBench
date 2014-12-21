#include "NosuchDebug.h"
#include "NosuchUtil.h"
#include "ffutil.h"

#include "Vizlet.h"
#include "VizBox2d.h"
#include "NosuchOsc.h"

#include "VizSprite.h"
#include "VizServer.h"

static CFFGLPluginInfo PluginInfo ( 
	VizBox2d::CreateInstance,	// Create method
	"V607",		// Plugin unique ID
	"VizBox2d",	// Plugin name	
	1,			// API major version number
	000,		// API minor version number	
	1,			// Plugin major version number
	000,		// Plugin minor version number
	FF_EFFECT,	// Plugin type
	"VizBox2d: a sample visual synth",	// description
	"by Tim Thompson - me@timthompson.com" 			// About
);

std::string vizlet_name() { return "VizBox2d"; }
CFFGLPluginInfo& vizlet_plugininfo() { return PluginInfo; }
void vizlet_setdll(std::string dll) { }

VizBox2d::VizBox2d() : Vizlet() {
	_params = defaultParams();
	_params->shape.set("circle");
	_params->speedinitial.set(0.1);
	_params->nsprites.set(5000);
	box2d_setup();
}

VizBox2d::~VizBox2d() {
}

DWORD __stdcall VizBox2d::CreateInstance(CFreeFrameGLPlugin **ppInstance) {
	*ppInstance = new VizBox2d();
	return (*ppInstance != NULL)? FF_SUCCESS : FF_FAIL;
}

void VizBox2d::OutlineToBody(VizSpriteOutline* so) {

	int npoints = so->Npoints();
	PointMem* points = so->Points();

	// The outlines might include a repeat of the first point, as the last point, so eliminate that
	if ( points[0].x == points[npoints-1].x && points[0].y == points[npoints-1].y ) {
		DEBUGPRINT(("point 0 and npoints-1 are the same!"));
		npoints--;
	}
	if ( npoints < 2 ) {
		return;
	}

	b2Vec2* b2points = new b2Vec2[npoints];

	DEBUGPRINT(("b2Linearslop = %f",b2_linearSlop));

	b2Vec2 b2p0 = b2Vec2(points[0].x,points[0].y);
	b2p0 *= 0.5;   // Not sure why
	b2Vec2 b2p1;
	int b2npoints = 0;
	b2points[b2npoints++] = b2p0;
	int sparseness = npoints / 64;
	int sparsecount = 0;
	for ( int pn=1; pn<npoints; pn++ ) {
		if ( sparsecount++ < sparseness ) {
			continue;
		}
		sparsecount = 0;
		PointMem* p = &points[pn];
		b2p1 = b2Vec2(p->x,p->y);
		b2p1 *= 0.5;   // Not sure why
		if (b2DistanceSquared(b2p0, b2p1) <= (b2_linearSlop * b2_linearSlop)) {
			DEBUGPRINT(("Too close, ignoring"));
			continue;
		}
		b2points[b2npoints++] = b2p1;
		b2p0 = b2p1;
	}

#if 0
	for ( int n=0; n<b2npoints; n++ ) {
		DEBUGPRINT(("b2points[%d] = %.4f  %.4f",b2npoints,b2points[n].x,b2points[n].y));
	}
	DEBUGPRINT(("FINAL b2npoints = %d",b2npoints));
#endif

	b2ChainShape chain;
	chain.CreateLoop(b2points, b2npoints);

	b2FixtureDef fixtureDef;
	fixtureDef.shape = &chain;
	fixtureDef.density = 0.1f;
	fixtureDef.friction = 1.0f;
	fixtureDef.restitution = 1.0f;

	b2BodyDef bodydef;
	DEBUGPRINT(("bodydef position set %.4f %.4f",so->state.pos.x, so->state.pos.y));
	// bodydef.type = b2_dynamicBody;
	bodydef.position.Set((float32)(so->state.pos.x), (float32)(so->state.pos.y));

	b2Body* b = _world->CreateBody(&bodydef);
	b->CreateFixture(&fixtureDef);

	delete b2points;
}

void VizBox2d::processKeystroke(int key, int downup) {
	DEBUGPRINT(("Keystroke key=%d downup=%d",key,downup));
	if ( downup != KEYSTROKE_DOWN ) {
		return;
	}
	// space bar fires
	if ( key == 84 ) {  // 't'
		float32 x = (rand() % 1000) / 1000.0f;
		float32 y = (rand() % 1000) / 1000.0f;
		b2Body* b = _makeDynamicBody(b2Vec2(0.4f+0.2f*x,0.9f+0.1f*y));
		_bodies.push_back(b);
	}
	if ( key == 32 || key == 82 ) {   // spacebar or 'r'
		float32 x = (rand() % 1000) / 1000.0f;
		float32 y = (rand() % 1000) / 1000.0f;
		b2Body* b = _makeDynamicBody(b2Vec2(x,y));
		_bodies.push_back(b);
	}
	if ( key == 266 || key == 65 ) {
		// Snapshot current VizSpriteOutlines
		int noutlines = 0;
		int newest_frameseq = -1;
		// We assume the sprite list is sorted with most recent ones at the beginning
		VizSpriteList* sl = GetVizSpriteList();
		sl->lock_read();
		for ( std::list<VizSprite*>::iterator i = sl->sprites.begin(); i!=sl->sprites.end(); i++ ) {
			VizSprite* s = *i;
			NosuchAssert(s);
			VizSpriteOutline* so = (VizSpriteOutline*)s;
			if ( so ) {
				if ( newest_frameseq < 0 ) {
					newest_frameseq = so->frame;
				}
				if ( so->frame != newest_frameseq ) {
					break;
				}
				noutlines++;
				DEBUGPRINT(("Should be snapshotting outine at %.4f,%.4f with npoints=%d frame=%d",so->state.pos.x,so->state.pos.y,so->Npoints(),so->frame));
				OutlineToBody(so);
			}
		}
		DEBUGPRINT(("Should be snapshotting %d outines, frameseq=%d",noutlines,FrameSeq()));
		sl->unlock();
	}
	if ( key == 70 || key == 71 ) {   // 'f' or 'g'
		for ( b2Body* b: _bodies ) {
			float32 fx = (rand() % 1000 - 500) / 10000000.0f;
			float32 fy = (rand() % 1000 - 500) / 10000000.0f;
			if ( key == 71 ) {
				fx *= 4.0;
				fy *= 4.0;
			}
			b->ApplyLinearImpulse(b2Vec2(fx,fy),b2Vec2(0.0f,0.0f),true);
		}
	}
}

void VizBox2d::processCursor(VizCursor* c, int downdragup) {

	// NO OpenGL calls here

	if ( downdragup == CURSOR_UP ) {
		return;
	}

	OutlineMem* outline = c->outline;
	buff_index b = c->hdr->buff_to_display;
	
#define ONE_OUTLINE_SPRITE
#ifdef ONE_OUTLINE_SPRITE
	if ( c->outline != NULL ) {
		_params->shape.set("outline");
	} else {
		_params->shape.set("square");
	}
	_params->zable.set(false);			// The z value should NOT control size
	_params->filled.set(false);
	_params->sizeinitial.set(0.5);
	_params->sizefinal.set(0.5);
	_params->sizetime.set(2.0);
	_params->alphainitial.set(0.5);
	_params->alphafinal.set(0.0);
	_params->alphatime.set(3.0);
	VizSprite* s = makeAndAddVizSprite(_params, c->pos);
	DEBUGPRINT1(("Adding VizSprite at %.4f,%.4f with frame=%d",c->pos.x,c->pos.y,s->frame));
	VizSpriteOutline* so = (VizSpriteOutline*)s;
	if ( so ) {
		so->setOutline(c->outline,c->hdr);
	}
#endif

#ifdef SPRITES_FROM_OUTLINE
	if ( outline ) {
		// DEBUGPRINT(("processCursor! downdragup=%d outline->npoints=%d\n", downdragup,outline->npoints));
		int npoints = outline->npoints;
		// NosuchDebug("Drawing outline from buff=%d, outline=%d npoints=%d",b,n,npoints);
		if ( outline->npoints > 20000 ) {
			NosuchDebug("Corruption in Cursor outline!?  npoints=%d",outline->npoints);
			return;
		}
		int pn0 = outline->index_of_firstpoint;
		int sparseness = npoints/10;  // or something higher

		int pn;
		for ( pn=0; pn<npoints; pn++ ) {
			PointMem* p = c->hdr->point(b,pn+pn0);
			if ( (pn % sparseness)==0 ) {
				// NosuchDebug("POINT= %.4f %.4f",p->x,p->y);
				AllVizParams* rp = defaultParams();
				rp->shape.set("outline");
				rp->movedir.set(rand()%360);
				makeAndAddVizSprite(rp, NosuchPos(c->pos.x+p->x,c->pos.y+p->y,c->pos.z));
			}
		}
	}
#endif
}

std::string VizBox2d::processJson(std::string meth, cJSON *json, const char *id) {
	// NO OpenGL calls here
	throw NosuchException("VizBox2d - Unrecognized method '%s'",meth.c_str());
}

void VizBox2d::processMidiInput(MidiMsg* m) {
	// NO OpenGL calls here
	defaultMidiVizSprite(m);
}

void VizBox2d::processMidiOutput(MidiMsg* m) {
	// NO OpenGL calls here
	VizSprite* s = defaultMidiVizSprite(m);
	if ( s ) {
		// create and attach box2d body
		DEBUGPRINT(("Should be creating/attaching body"));

		// Define the dynamic body. We set its position and call the body factory.
		b2BodyDef bodyDef;
		bodyDef.type = b2_dynamicBody;
		bodyDef.position.Set(0.0f, 4.0f);

		b2Body* b = _world->CreateBody(&bodyDef);

		// Define another box shape for our dynamic body.
		b2PolygonShape shape;
		shape.SetAsBox(1.0f, 1.0f);

		// Define the dynamic body fixture.
		b2FixtureDef fixtureDef;
		fixtureDef.shape = &shape;

		// Set the box density to be non-zero, so it will be dynamic.
		fixtureDef.density = 0.1f;
		fixtureDef.friction = 1.0f;
		fixtureDef.restitution = 1.0f;

		// Add the shape to the body.
		b->CreateFixture(&fixtureDef);

		// Add the body to the VizSprite
		s->body = b;
	}
}

bool VizBox2d::processDraw() {

	DEBUGPRINT1(("VizBox2d::processDraw start"));

	box2d_step();

	glColor4f(1.0,1.0,0.0,0.5);
	glLineWidth((GLfloat)1.0f);

	// OpenGL calls here	
	glBegin(GL_LINE_LOOP);
	glVertex3f(0.2f, 0.2f, 0.0f);	// Top Left
	glVertex3f(0.2f, 0.8f, 0.0f);	// Top Right
	glVertex3f(0.8f, 0.8f, 0.0f);	// Bottom Right
	glVertex3f(0.8f, 0.2f, 0.0f);	// Bottom Left
	glEnd();

#ifdef TJT2
	float vertsCoords[] = {-0.1f, 0.1f, //V1
                            0.1f, 0.1f, //V2
                            0.0f, 0.0f //V3
                            };
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, vertsCoords);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glDisableClientState(GL_VERTEX_ARRAY);
#endif

	// _drawBody(_body);

	glEnableClientState(GL_VERTEX_ARRAY);
	_world->DrawDebugData();
	glDisableClientState(GL_VERTEX_ARRAY);

	DrawVizSprites();

	DEBUGPRINT1(("VizBox2d::processDraw end"));

	return true;
}

void VizBox2d::_drawBody(b2Body* b) {
	b2Vec2 position = b->GetPosition();
	b2Fixture* fixtures = b->GetFixtureList();

	glBegin(GL_LINE_LOOP);
	float x = position.x;
	float y = position.y;
	glVertex3f(x-0.2f, y+0.2f, 0.0f);	// Top Left
	glVertex3f(x+0.2f, y+0.2f, 0.0f);	// Top Right
	glVertex3f(x+0.2f, y-0.2f, 0.0f);	// Bottom Right
	glVertex3f(x-0.2f, y-0.2f, 0.0f);	// Bottom Left
	glEnd();
}

void VizBox2d::processDrawNote(MidiMsg* m) {
	// OpenGL calls here
}

#include <Box2D/Box2D.h>

// #include <stdio.h>

// This is a simple example of building and running a simulation
// using Box2D. Here we create a large ground box and a small dynamic
// box.
// There are no graphics for this example. Box2D is meant to be used
// with your rendering engine in your game engine.
void VizBox2d::box2d_setup()
{
	// _gravity = b2Vec2(0.0f, -1.0f);
	_gravity = b2Vec2(0.0f, 0.0f);
	_world = new b2World(_gravity);

	// Define the ground body.
	b2BodyDef groundBodyDef;
	groundBodyDef.position.Set(0.5f, 0.05f);

	b2Body* groundBody = _world->CreateBody(&groundBodyDef);

	b2PolygonShape groundBox;
	groundBox.SetAsBox(0.5f, 0.01f);
	groundBody->CreateFixture(&groundBox, 1.0f);

	// Add a couple of balls to the simulation
	b2Body* b = _makeDynamicBody(b2Vec2(0.40f, 0.9f));
	_bodies.push_back(b);
	b = _makeDynamicBody(b2Vec2(0.5f, 0.8f));
	_bodies.push_back(b);

	// Prepare for simulation. Typically we use a time step of 1/60 of a
	// second (60Hz) and 10 iterations. This provides a high quality simulation
	// in most game scenarios.
	_timeStep = 1.0f / 60.0f;
	_velocityIterations = 6;
	_positionIterations = 2;
	
	// _body->ApplyLinearImpulse(b2Vec2(0.0f,0.2f),b2Vec2(0.0f,0.0f),true);
	// _body->ApplyAngularImpulse(1.0f,true);

	//in constructor, usually
	_world->SetDebugDraw( &_debugdraw );

	//somewhere appropriate
	_debugdraw.SetFlags(b2Draw::e_shapeBit
						| b2Draw::e_jointBit
						// | b2Draw::e_aabbBit
						| b2Draw::e_pairBit
						// | b2Draw::e_centerOfMassBit
						);
}

b2Body*
VizBox2d::_makeDynamicBody(b2Vec2 pos) {
	// Define the dynamic body. We set its position and call the body factory.
	b2Body* b;
	b2BodyDef bodyDef;

	bodyDef.type = b2_dynamicBody;
	bodyDef.position = pos;
	b = _world->CreateBody(&bodyDef);

#define TJT_CIRCLE
#ifdef TJT_CIRCLE
	b2CircleShape shape;
	shape.m_radius = 0.02f;
#else
	b2PolygonShape shape;
	shape.SetAsBox(0.02f, 0.02f);
#endif

	// Define the dynamic body fixture.
	b2FixtureDef fixtureDef;
	fixtureDef.shape = &shape;

	// Set the box density to be non-zero, so it will be dynamic.
	fixtureDef.density = 0.1f;
	fixtureDef.friction = 1.0f;
	fixtureDef.restitution = 1.0f;

	// Add the shape to the body.
	b->CreateFixture(&fixtureDef);
	return b;
}

void VizBox2d::box2d_step()
{
	// Instruct the world to perform a single step of simulation.
	// It is generally best to keep the time step and iterations fixed.
	_world->Step(_timeStep, _velocityIterations, _positionIterations);
}