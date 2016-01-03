#ifndef _VizBox2d_H
#define _VizBox2d_H

#include <Box2D/Box2D.h>
#include <Box2D/Common/b2Math.h>
#include "DebugDraw.h"

class VizBox2d : public Vizlet
{
public:
	VizBox2d();
	virtual ~VizBox2d();

	static DWORD __stdcall CreateInstance(CFreeFrameGLPlugin **ppInstance);

	std::string processJson(std::string meth, cJSON *jsonparams, const char *id);
	void processMidiInput(MidiMsg* m);
	void processMidiOutput(MidiMsg* m);
	void processCursor(VizCursor* c, int downdragup);
	void processKeystroke(int key, int downup);
	bool processDraw();
	void OutlineToBody(VizSpriteOutline* so);

private:
	// Put private things here.

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
	b2Body* _makeDynamicBody(b2Vec2 pos);

	SpriteVizParams* m_params;
};

#endif
