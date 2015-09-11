#ifndef _VizMaze2_H
#define _VizMaze2_H

#include <Box2D/Box2D.h>
#include <Box2D/Common/b2Math.h>
#include "DebugDraw.h"

class MazeBallData {
public:
	MazeBallData() { }
	int color; // and whatever
};

class VizMaze2 : public Vizlet, public b2ContactListener
{
public:
	VizMaze2();
	virtual ~VizMaze2();

	static DWORD __stdcall CreateInstance(CFreeFrameGLPlugin **ppInstance);

	void processKeystroke(int key, int downup);
	std::string processJson(std::string meth, cJSON *jsonparams, const char *id);
	void processMidiInput(MidiMsg* m);
	void processMidiOutput(MidiMsg* m);
	void processCursor(VizCursor* c, int downdragup);
	bool processDraw();
	void processDrawNote(MidiMsg* m);

	void BeginContact(b2Contact* contact);
	void EndContact(b2Contact* contact);

private:
	b2World *m_world;
	b2Vec2 m_gravity;
	float32 m_timeStep;
	int32 m_velocityIterations;
	int32 m_positionIterations;
	std::vector<b2Body*> m_bodies;
	VizDebugDraw m_debugdraw;
	int m_vaoId;
	int m_vboId;

	void box2d_setup();
	void box2d_step();
	void _drawBody(b2Body* b);
	b2Body* _makeBall(b2Vec2 pos);

	void addWall(b2Vec2 v0, b2Vec2 v1);
	b2Body* addBall(b2Vec2 pos);
	int channelOf(b2Vec2 v);
	int pitchOf(b2Vec2 v);

	int channelMin;
	int channelMax;
	int pitchMin;
	int pitchMax;

};

#endif
