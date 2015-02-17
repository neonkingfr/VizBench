#include "Vizlet.h"
#include "VizBox2d.h"

static CFFGLPluginInfo PluginInfo ( 
	VizBox2d::CreateInstance,	// Create method
	"VZBX",		// Plugin unique ID
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

VizBox2d::VizBox2d() : Vizlet() {
	m_params = defaultParams();
	m_params->shape.set("circle");
	m_params->speedinitial.set(0.1);
	m_params->nsprites.set(5000);
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
	DEBUGPRINT(("bodydef position set %.4f %.4f",so->m_state.pos.x, so->m_state.pos.y));
	// bodydef.type = b2_dynamicBody;
	bodydef.position.Set((float32)(so->m_state.pos.x), (float32)(so->m_state.pos.y));

	b2Body* b = m_world->CreateBody(&bodydef);
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
		m_bodies.push_back(b);
	}
	if ( key == 32 || key == 82 ) {   // spacebar or 'r'
		float32 x = (rand() % 1000) / 1000.0f;
		float32 y = (rand() % 1000) / 1000.0f;
		b2Body* b = _makeDynamicBody(b2Vec2(x,y));
		m_bodies.push_back(b);
	}
	if ( key == 266 || key == 65 ) {
		// Snapshot current VizSpriteOutlines
		int noutlines = 0;
		int newest_framenum = -1;
		// We assume the sprite list is sorted with most recent ones at the beginning
		VizSpriteList* sl = GetVizSpriteList();
		sl->lock_read();
		for ( std::list<VizSprite*>::iterator i = sl->m_sprites.begin(); i!=sl->m_sprites.end(); i++ ) {
			VizSprite* s = *i;
			NosuchAssert(s);
			VizSpriteOutline* so = (VizSpriteOutline*)s;
			if ( so ) {
				if ( newest_framenum < 0 ) {
					newest_framenum = so->m_framenum;
				}
				if ( so->m_framenum != newest_framenum ) {
					break;
				}
				noutlines++;
				DEBUGPRINT(("Should be snapshotting outine at %.4f,%.4f with npoints=%d frame=%d",so->m_state.pos.x,so->m_state.pos.y,so->Npoints(),so->m_framenum));
				OutlineToBody(so);
			}
		}
		DEBUGPRINT(("Should be snapshotting %d outines, framenum=%d",noutlines,FrameNum()));
		sl->unlock();
	}
	if ( key == 70 || key == 71 ) {   // 'f' or 'g'
#define OLDSTYLE
#ifdef OLDSTYLE
		for (std::vector<b2Body*>::const_iterator iter=m_bodies.begin();iter!=m_bodies.end();++iter) {
			b2Body* b = *iter;
#else
		for ( b2Body* b: m_bodies ) {
#endif
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
}

std::string VizBox2d::processJson(std::string meth, cJSON *json, const char *id) {
	// NO OpenGL calls here

	if (meth == "apis") {
		return jsonStringResult("randomize;push", id);
	}

	if (meth == "randomize") {
		// Add some random bodies
		for (int n = 0; n < 10; n++) {
			float32 x = (rand() % 1000) / 1000.0f;
			float32 y = (rand() % 1000) / 1000.0f;
			b2Body* b = _makeDynamicBody(b2Vec2(x, y));
			m_bodies.push_back(b);
		}
		return jsonOK(id);
	}
	if (meth == "push") {
#ifdef OLDSTYLE
		for (std::vector<b2Body*>::const_iterator iter=m_bodies.begin();iter!=m_bodies.end();++iter) {
			b2Body* b = *iter;
#else
		for ( b2Body* b: m_bodies ) {
#endif
			float32 fx = (rand() % 1000 - 500) / 10000000.0f;
			float32 fy = (rand() % 1000 - 500) / 10000000.0f;
			b->ApplyLinearImpulse(b2Vec2(fx,fy),b2Vec2(0.0f,0.0f),true);
		}
		return jsonOK(id);
	}

	throw NosuchException("VizBox2d - Unrecognized method '%s'",meth.c_str());
}

void VizBox2d::processMidiInput(MidiMsg* m) {
	// NO OpenGL calls here
}

void VizBox2d::processMidiOutput(MidiMsg* m) {
	// NO OpenGL calls here
}

bool VizBox2d::processDraw() {

	box2d_step();
	glColor4f(1.0,1.0,0.0,0.5);
	glLineWidth((GLfloat)1.0f);
	glEnableClientState(GL_VERTEX_ARRAY);
	m_world->DrawDebugData();
	glDisableClientState(GL_VERTEX_ARRAY);

	DrawVizSprites();

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

#include <Box2D/Box2D.h>

// #include <stdio.h>

// This is a simple example of building and running a simulation
// using Box2D. Here we create a large ground box and a small dynamic
// box.
// There are no graphics for this example. Box2D is meant to be used
// with your rendering engine in your game engine.
void VizBox2d::box2d_setup()
{
	// m_gravity = b2Vec2(0.0f, -1.0f);
	m_gravity = b2Vec2(0.0f, 0.0f);
	m_world = new b2World(m_gravity);

	// Define the ground body.
	b2BodyDef groundBodyDef;
	groundBodyDef.position.Set(0.5f, 0.05f);

	b2Body* groundBody = m_world->CreateBody(&groundBodyDef);

	b2PolygonShape groundBox;
	groundBox.SetAsBox(0.5f, 0.01f);
	groundBody->CreateFixture(&groundBox, 1.0f);

#if 0
	// Add a couple of balls to the simulation
	b2Body* b = _makeDynamicBody(b2Vec2(0.40f, 0.9f));
	m_bodies.push_back(b);
	b = _makeDynamicBody(b2Vec2(0.5f, 0.8f));
	m_bodies.push_back(b);
#endif

	// Prepare for simulation. Typically we use a time step of 1/60 of a
	// second (60Hz) and 10 iterations. This provides a high quality simulation
	// in most game scenarios.
	m_timeStep = 1.0f / 60.0f;
	m_velocityIterations = 6;
	m_positionIterations = 2;
	
	// _body->ApplyLinearImpulse(b2Vec2(0.0f,0.2f),b2Vec2(0.0f,0.0f),true);
	// _body->ApplyAngularImpulse(1.0f,true);

	//in constructor, usually
	m_world->SetDebugDraw( &m_debugdraw );

	//somewhere appropriate
	m_debugdraw.SetFlags(b2Draw::e_shapeBit
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
	b = m_world->CreateBody(&bodyDef);

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
	m_world->Step(m_timeStep, m_velocityIterations, m_positionIterations);
}