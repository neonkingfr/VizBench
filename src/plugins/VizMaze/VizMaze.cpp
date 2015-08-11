#include "Vizlet.h"
#include "VizMaze.h"

static CFFGLPluginInfo PluginInfo ( 
	VizMaze::CreateInstance,	// Create method
	"VZMZ",		// Plugin unique ID
	"VizMaze",	// Plugin name	
	1,			// API major version number
	000,		// API minor version number	
	1,			// Plugin major version number
	000,		// Plugin minor version number
	FF_EFFECT,	// Plugin type
	"VizMaze: a sample vizlet",			// description
	"by Tim Thompson - me@timthompson.com" 		// About
);

std::string vizlet_name() { return "VizMaze"; }
CFFGLPluginInfo& vizlet_plugininfo() { return PluginInfo; }

VizMaze::VizMaze() : Vizlet(), MazeListener() {
	m_maze = NULL;
	setSize(128, 128, 1000, 1000);

	m_maze->addLine(MazeRowCol(10, 10), MazeRowCol(100, 10));

	int lifetime = 10 * SchedulerClicksPerSecond();

	MazePoint xy = m_maze->getPoint(MazeRowCol(40, 51));
	MazePoint dxy(1000, 1000);

	addBall(xy, dxy, SchedulerCurrentClick(), lifetime);
}

void
VizMaze::setSize(int nrows, int ncols, int cellxsize, int cellysize) {

	// Should probably delete m_maze if it's non-NULL
	m_maze = new NosuchMaze(*this, nrows, ncols, cellxsize, cellysize);

	int ncells = nrows * ncols;
	m_celldata = new VizMazeCellData[ncells];
	VizMazeCellData* d = m_celldata;
	d->seq = 0;
	for (int r = 0; r < nrows; r++) {
		for (int c = 0; c < ncols; c++) {
			m_maze->setCellData(MazeRowCol(r,c),d);
			d++;
		}
	}
	addBorder();
}

MazeBall*
VizMaze::addBall(MazePoint xy, MazePoint dxy, int born, int lifetime) {
	return m_maze->addBall(xy, dxy, born, lifetime);
}

void
VizMaze::addBorder() {
	MazeRowCol rc = m_maze->rowcolSize();
	int r0 = 0;
	int c0 = 0;
	int r1 = rc.r - 1;
	int c1 = rc.c - 1;
	m_maze->addLine(MazeRowCol(r0, c0), MazeRowCol(r0, c1));
	m_maze->addLine(MazeRowCol(r0, c1), MazeRowCol(r1, c1));
	m_maze->addLine(MazeRowCol(r1, c1), MazeRowCol(r1, c0));
	m_maze->addLine(MazeRowCol(r1, c0), MazeRowCol(r0, c0));
}

VizMaze::~VizMaze() {
}

DWORD __stdcall VizMaze::CreateInstance(CFreeFrameGLPlugin **ppInstance) {
	*ppInstance = new VizMaze();
	return (*ppInstance != NULL)? FF_SUCCESS : FF_FAIL;
}

void VizMaze::processCursor(VizCursor* c, int downdragup) {
	// NO OpenGL calls here
}

void VizMaze::processAdvanceClickTo(int click) {
	static int lastclick = 0;
	int nclicks = click - lastclick;
	// DEBUGPRINT(("processAdvanceClickTo MAZE click=%d time=%ld nclicks=%d", click, timeGetTime(),nclicks));
	m_maze->moveBalls(nclicks);
	lastclick = click;
}

std::string VizMaze::processJson(std::string meth, cJSON *json, const char *id) {
	// NO OpenGL calls here
	throw NosuchException("VizMaze - Unrecognized method '%s'",meth.c_str());
}

void VizMaze::processMidiInput(MidiMsg* m) {
	// NO OpenGL calls here
}

void VizMaze::processMidiOutput(MidiMsg* m) {
	// NO OpenGL calls here
}

bool VizMaze::processDraw() {
	// OpenGL calls here
	DrawVizSprites();
	m_maze->Draw();
	return true;
}

void
VizMaze::onCellHit(MazeCell& cell, MazeBall* b) {
	DEBUGPRINT(("CELL HIT!  ball xy=%d,%d", b->xy().x, b->xy().y));
}

#if 0
VizMazeCellData*
VizMaze::getCellData(MazeRowCol rc) {
	return (VizMazeCellData*) (m_maze->getCellData(rc));
}
#endif

void
VizMaze::onCellDraw(MazeCell& cell, MazeRowCol rc)
{
	glColor4f(0.0, 1.0, 0.0, 1.0);  // green
	drawCell(rc);
}

void
VizMaze::onBallDraw(MazeBall* ball, MazePoint xy)
{
	glColor4f(1.0, 0.0, 0.0, 1.0);  // red
	drawCell(m_maze->getRowCol(xy));
}

void
VizMaze::drawCell(MazeRowCol rc)
{
	float alpha = 1.0f;
	MazePoint xysize = m_maze->xySize();
	MazeRowCol rcsize = m_maze->rowcolSize();
	MazePoint cellsize = m_maze->cellSize();

	float x_percol = (1.0f / rcsize.c);
	float y_perrow = (1.0f / rcsize.r);

	float x = rc.c * float(cellsize.x) / xysize.x;
	float y = rc.r * float(cellsize.y) / xysize.y;

	glBegin(GL_QUADS);
	glVertex3f(x, y, 0.0f);
	glVertex3f(x, y+y_perrow, 0.0f);
	glVertex3f(x+x_percol, y+y_perrow, 0.0f);
	glVertex3f(x+x_percol, y, 0.0f);
	glEnd();

}