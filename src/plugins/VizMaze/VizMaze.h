#ifndef _VizMaze_H
#define _VizMaze_H

#include "VizMazeImpl.h"

class VizMazeCellData {
public:
	VizMazeCellData() { }
	long seq;
};

class VizMaze : public Vizlet, public MazeListener
{
public:
	VizMaze();
	virtual ~VizMaze();

	static DWORD __stdcall CreateInstance(CFreeFrameGLPlugin **ppInstance);

	std::string processJson(std::string meth, cJSON *jsonparams, const char *id);
	void processMidiInput(MidiMsg* m);
	void processMidiOutput(MidiMsg* m);
	void processCursor(VizCursor* c, int downdragup);
	// void processAdvanceClickTo(int click);
	bool processDraw();

	void onCellHit(MazeCell& cell, MazeBall* b);
	void onCellDraw(MazeCell& cell, MazeRowCol rc);
	void onBallDraw(MazeBall* b, MazePoint xy);

private:
	void drawCell(MazeRowCol rc, int sizefactor = 0);
	void setSize(int nrows, int ncols, int cellxsize, int cellysize);
	void setup();
	MazeBall* addBall(MazePoint xy, MazePoint dxy, int born, int lifetime);
	void addBorder();
	MazeRowCol randRowCol();
	MazePoint randDelta();

	VizMazeCellData* m_celldata;
	VizMazeImpl* m_maze;
};

#endif
