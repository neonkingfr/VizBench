#include "Vizlet.h"
#include "VizMaze2.h"

static CFFGLPluginInfo PluginInfo ( 
	VizMaze2::CreateInstance,	// Create method
	"V868",		// Plugin unique ID
	"VizMaze2",	// Plugin name	
	1,			// API major version number
	000,		// API minor version number	
	1,			// Plugin major version number
	000,		// Plugin minor version number
	FF_EFFECT,	// Plugin type
	"VizMaze2: a sample vizlet",			// description
	"by Tim Thompson - me@timthompson.com" 		// About
);

std::string vizlet_name() { return "VizMaze2"; }
CFFGLPluginInfo& vizlet_plugininfo() { return PluginInfo; }

VizMaze2::VizMaze2() : Vizlet(), b2ContactListener() {
	channelMin = 1;
	channelMax = 4;
	pitchMin = 60;
	pitchMax = 100;
	box2d_setup();
}

VizMaze2::~VizMaze2() {
}

int
VizMaze2::channelOf(b2Vec2 v)
{
	int nchan = channelMax - channelMin;
	double dy = 1.0 / nchan;
	int chan = channelMin + int(floor(v.y / dy));
	return chan;
}

int
VizMaze2::pitchOf(b2Vec2 v)
{
	int nchan = pitchMax - pitchMin;
	double dx = 1.0 / nchan;
	int pitch = pitchMin + int(floor(v.x / dx));
	return pitch;
}

void VizMaze2::BeginContact(b2Contact* contact) {
	//check if fixture A was a ball
	b2Fixture* fA = contact->GetFixtureA();
	b2Fixture* fB = contact->GetFixtureB();
	b2Body* bodyA = fA->GetBody();
	b2Body* bodyB = fB->GetBody();

	void* dataA = bodyA->GetUserData();
	void* dataB = bodyB->GetUserData();
	b2Transform transformA = bodyA->GetTransform();
	b2Transform transformB = bodyB->GetTransform();

	b2CircleShape* circleShape;
	b2EdgeShape* edgeShape;
	b2Vec2 circlePos;
	b2Vec2 edgePos;
	b2Vec2 edgePos1;
	b2Vec2 edgePos2;
	bool intersected = false;
	try{
		circleShape = (b2CircleShape*)(fB->GetShape());
		circlePos = bodyB->GetWorldCenter();

		edgeShape = (b2EdgeShape*)(fA->GetShape());
		edgePos = bodyA->GetWorldCenter();
		edgePos1 = edgeShape->m_vertex1;
		edgePos2 = edgeShape->m_vertex2;

		b2Vec2 vertiPos;
		b2RayCastInput rayin;
		rayin.maxFraction = 1;
		float dx = edgePos1.x - edgePos2.x;
		b2Vec2 intersect;
		if (dx == 0.0) {
			// Edge is vertical, unless circle's 
			intersect.x = edgePos1.x;
			intersect.y = circlePos.y;
			intersected = true;
		}
		else {
			b2RayCastOutput rayout;
			b2Transform transform;
			transform.SetIdentity();
			rayin.p1 = b2Vec2(circlePos.x, 0.0f);
			rayin.p2 = b2Vec2(circlePos.x, 1.0f);
			intersected = edgeShape->RayCast(&rayout, rayin, transform, 0);
			if (intersected) {
				// This assumes that rayin length is 1.0
				intersect = b2Vec2(circlePos.x, rayout.fraction);
				if (intersect.y > 1.0 || intersect.y < 0.0) {
					DEBUGPRINT(("Bad Y!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"));
				}
				if (intersect.x > 1.0 || intersect.x < 0.0) {
					DEBUGPRINT(("Bad X!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"));
				}
			}
		}
		if (intersected) {
			int chan = channelOf(intersect);
			int pitch = pitchOf(intersect);
			DEBUGPRINT(("intersect = %f,%f  chan=%d pitch=%d", intersect.x, intersect.y,chan,pitch));
		}
	}
	catch (...) {
		circleShape = NULL;
		edgeShape = NULL;
	}
	if (circleShape == NULL && edgeShape == NULL) {
		DEBUGPRINT(("Unrecognized collision shapes!?"));
		return;
	}

	if (contact->IsTouching()) {
		// DEBUGPRINT(("TOUCHING!!"));
	}
	else {
		DEBUGPRINT(("NOT TOUCHING!?!"));
	}

#if 0
	if (bodyUserData)
		static_cast<Ball*>(bodyUserData)->startContact();
	}
	//check if fixture B was a ball
	if (bodyUserData) {
		static_cast<Ball*>(bodyUserData)->startContact();
	}
#endif

}
void VizMaze2::EndContact(b2Contact* contact) {
}

DWORD __stdcall VizMaze2::CreateInstance(CFreeFrameGLPlugin **ppInstance) {
	*ppInstance = new VizMaze2();
	return (*ppInstance != NULL)? FF_SUCCESS : FF_FAIL;
}

void VizMaze2::processKeystroke(int key, int downup) {
	DEBUGPRINT(("Keystroke key=%d downup=%d", key, downup));
	if (downup != KEYSTROKE_DOWN) {
		return;
	}
	// space bar fires
	if (key == 84) {  // 't'
		float32 x = (rand() % 1000) / 1000.0f;
		float32 y = (rand() % 1000) / 1000.0f;
		addBall(b2Vec2(0.4f + 0.2f*x, 0.9f + 0.1f*y));
	}
	if (key == 32 || key == 82) {   // spacebar or 'r'
		float32 x = (rand() % 1000) / 1000.0f;
		float32 y = (rand() % 1000) / 1000.0f;
		addBall(b2Vec2(x, y));
	}
	if (key == 65) {
		float32 x0 = (rand() % 1000) / 1000.0f;
		float32 y0 = (rand() % 1000) / 1000.0f;
		float32 x1 = (rand() % 1000) / 1000.0f;
		float32 y1 = (rand() % 1000) / 1000.0f;
		addWall(b2Vec2(x0, y0), b2Vec2(x1,y1));
	}
#if 0
	if (key == 266 || key == 65) {
		// Snapshot current VizSpriteOutlines
		int noutlines = 0;
		int newest_framenum = -1;
		// We assume the sprite list is sorted with most recent ones at the beginning
		VizSpriteList* sl = GetVizSpriteList();
		sl->lock_read();
		for (std::list<VizSprite*>::iterator i = sl->m_sprites.begin(); i != sl->m_sprites.end(); i++) {
			VizSprite* s = *i;
			VizAssert(s);
			VizSpriteOutline* so = (VizSpriteOutline*)s;
			if (so) {
				if (newest_framenum < 0) {
					newest_framenum = so->m_framenum;
				}
				if (so->m_framenum != newest_framenum) {
					break;
				}
				noutlines++;
				DEBUGPRINT(("Should be snapshotting outine at %.4f,%.4f with npoints=%d frame=%d", so->m_state.pos.x, so->m_state.pos.y, so->Npoints(), so->m_framenum));
				OutlineToBody(so);
			}
		}
		DEBUGPRINT(("Should be snapshotting %d outines, framenum=%d", noutlines, FrameNum()));
		sl->unlock();
	}
#endif

	if (key >= 64 && key <= 77 ) {   // 'f' or 'g' or 'd' or 'c' or 'b'
#define OLDSTYLE
#ifdef OLDSTYLE
		for (std::vector<b2Body*>::const_iterator iter = m_bodies.begin(); iter != m_bodies.end(); ++iter) {
			b2Body* b = *iter;
#else
		for (b2Body* b : m_bodies) {
#endif
			float32 fx = (rand() % 1000 - 500) / 1000000000.0f;
			float32 fy = (rand() % 1000 - 500) / 1000000000.0f;
			if (key == 71) {
				fx *= 4.0;
				fy *= 4.0;
				b->ApplyLinearImpulse(b2Vec2(fx, fy), b->GetWorldCenter(), true);
			}
			if (key == 68) {
				fx /= 10.0;
				fy /= 10.0;
				b->ApplyLinearImpulse(b2Vec2(fx, fy), b->GetWorldCenter(), true);
			}
			if (key == 67) {  // c
				fx = 0.000f;
				fy = 0.0000001f;
				b->ApplyLinearImpulse(b2Vec2(fx, fy), b->GetWorldCenter(), true);
			}
			if (key == 66) {  // b
				fx = 0.000f;
				fy = -0.0000001f;
				b->ApplyLinearImpulse(b2Vec2(fx, fy), b->GetWorldCenter(), true);
			}
		}
	}
}

void VizMaze2::processCursor(VizCursor* c, int downdragup) {
	// NO OpenGL calls here
}

std::string VizMaze2::processJson(std::string meth, cJSON *json, const char *id) {
	// NO OpenGL calls here
	throw VizException("VizMaze2 - Unrecognized method '%s'",meth.c_str());
}

void VizMaze2::processMidiInput(MidiMsg* m) {
	// NO OpenGL calls here
}

void VizMaze2::processMidiOutput(MidiMsg* m) {
	// NO OpenGL calls here
}

bool VizMaze2::processDraw() {
	// OpenGL calls here
	box2d_step();

	glColor4f(1.0,1.0,0.0,0.5);
	glLineWidth((GLfloat)1.0f);
	glEnableClientState(GL_VERTEX_ARRAY);
	m_world->DrawDebugData();
	glDisableClientState(GL_VERTEX_ARRAY);

	m_world->SetContactListener(this);

	return true;
}

void VizMaze2::processDrawNote(MidiMsg* m) {
	// OpenGL calls here
}

void VizMaze2::addWall( b2Vec2 v0, b2Vec2 v1) {
	b2BodyDef wallBodyDef;
	wallBodyDef.type = b2_staticBody;
	wallBodyDef.position.Set(0.0f, 0.0f);
	b2Body* wb = m_world->CreateBody(&wallBodyDef);

	b2EdgeShape shape;
	shape.Set(v0, v1);

	b2FixtureDef wallFixtureDef;
	wallFixtureDef.shape = &shape;
	wallFixtureDef.density = 1.0f;
	wallFixtureDef.friction = 0.0f;
	wallFixtureDef.restitution = 1.0f;
	wb->CreateFixture(&wallFixtureDef);
}

// This is a simple example of building and running a simulation
// using Box2D. Here we create a large ground box and a small dynamic
// box.
// There are no graphics for this example. Box2D is meant to be used
// with your rendering engine in your game engine.
void VizMaze2::box2d_setup()
{
	// m_gravity = b2Vec2(0.0f, -1.0f);
	m_gravity = b2Vec2(0.0f, 0.0f);
	m_world = new b2World(m_gravity);

	// Define the ground body.
	b2BodyDef groundBodyDef;
	b2PolygonShape groundBox;

	addWall(b2Vec2(0.0f,0.0f),b2Vec2(1.0f,0.0f));
	addWall(b2Vec2(1.0f,0.0f),b2Vec2(1.0f,1.0f));
	addWall(b2Vec2(1.0f,1.0f),b2Vec2(0.0f,1.0f));
	addWall(b2Vec2(0.0f,1.0f),b2Vec2(0.0f,0.0f));

	// Prepare for simulation. Typically we use a time step of 1/60 of a
	// second (60Hz) and 10 iterations. This provides a high quality simulation
	// in most game scenarios.
	// m_timeStep = 1.0f / 60.0f;
	m_timeStep = 1.0f / 1000.0f;
	m_velocityIterations = 10;
	m_positionIterations = 4;

	m_velocityIterations = 100;
	m_positionIterations = 40;

	// _body->ApplyLinearImpulse(b2Vec2(0.0f,0.2f),b2Vec2(0.0f,0.0f),true);
	// _body->ApplyAngularImpulse(1.0f,true);

	//in constructor, usually
	m_world->SetDebugDraw(&m_debugdraw);

	//somewhere appropriate
	m_debugdraw.SetFlags(b2Draw::e_shapeBit
		| b2Draw::e_jointBit
		// | b2Draw::e_aabbBit
		| b2Draw::e_pairBit
		// | b2Draw::e_centerOfMassBit
		);

	// FAKE setup of things to test things
	addWall( b2Vec2(0.2f, 0.2f),  b2Vec2(0.5f, 0.8f));

	// addBall(b2Vec2(0.4f, 0.8f)); // above line

	addBall(b2Vec2(0.4f, 0.2f)); // below line

	// float fx = 0.000f;
	// float fy = 0.0000001f;
	// b->ApplyLinearImpulse(b2Vec2(fx, fy), b2Vec2(0.0f, 0.0f), true);
}

b2Body*
VizMaze2::addBall(b2Vec2 pos) {
	// Define the dynamic body. We set its position and call the body factory.
	b2Body* b = _makeBall(pos);
	m_bodies.push_back(b);
	return b;
}

b2Body*
VizMaze2::_makeBall(b2Vec2 pos) {
	// Define the dynamic body. We set its position and call the body factory.
	b2Body* b;
	b2BodyDef bodyDef;

	bodyDef.type = b2_dynamicBody;
	bodyDef.position = pos;
	b = m_world->CreateBody(&bodyDef);

#define TJT_CIRCLE
#ifdef TJT_CIRCLE
	b2CircleShape shape;
	shape.m_radius = 0.01f;
#else
	b2PolygonShape shape;
	shape.SetAsBox(0.02f, 0.02f);
#endif

	// Define the dynamic body fixture.
	b2FixtureDef fixtureDef;
	fixtureDef.shape = &shape;

	// Set the box density to be non-zero, so it will be dynamic.
	fixtureDef.density = 0.0001f;
	fixtureDef.friction = 0.0f;
	fixtureDef.restitution = 1.0f;

	// Add the shape to the body.
	b->CreateFixture(&fixtureDef);

	MazeBallData* bd = new MazeBallData();
	b->SetUserData(bd);
	return b;
}

void VizMaze2::box2d_step()
{
	// Instruct the world to perform a single step of simulation.
	// It is generally best to keep the time step and iterations fixed.
	m_world->Step(m_timeStep, m_velocityIterations, m_positionIterations);
}
