#ifndef _VizLife_H
#define _VizLife_H

#include "NosuchLife.h"

class LifeCellData {
public:
	LifeCellData() { }
	int lifetime; // in generations
	float zdepth;
	int genborn;
	int row;
	int col;
	long seq;
};

class VizLife : public Vizlet, public LifeListener
{
public:
	VizLife();
	~VizLife();

	static DWORD __stdcall CreateInstance(CFreeFrameGLPlugin **ppInstance);

	std::string processJson(std::string meth, cJSON *jsonparams, const char *id);
	void processMidiInput(MidiMsg* m);
	void processMidiOutput(MidiMsg* m);
	void processCursor(VizCursor* c, int downdragup);
	void processKeystroke(int key, int downup);
	bool processDraw();
	void processDrawNote(MidiMsg* m);

	// LifeListener
	void onCellBirth(int r, int c, LifeCell& cell,
		LifeCell& cell_ul, LifeCell& cell_um, LifeCell& cell_ur,
		LifeCell& cell_ml, LifeCell& cell_mm, LifeCell& cell_mr,
		LifeCell& cell_ll, LifeCell& cell_lm, LifeCell& cell_lr);
	void onCellSurvive(int r, int c, LifeCell& cell);
	void onCellDeath(int r, int c, LifeCell& cell);
	void onCellDraw(int r, int c, LifeCell& cell);

	LifeCellData& getData(int r, int c);

private:

	void Age();
	void Clear();
	void Gen();
	void RandomAddEvery(int sparseness);
	int GenBornOf(int r, int c);
	void DoKey(int key);
	void cursor2Cell(VizCursor* c, int& row, int& col);
	float col2x(int col);
	float row2y(int row);
	float CellDepth(LifeCell& cell);
	void fakeCellBirth(int r, int c);
	void addCellSprite(int r, int c);
	void DeleteCellAndTouching(int r, int c);
	void SetSize(int rows, int cols);

	// Put private things here.
	AllVizParams* _params;
	AllVizParams* _cellparams;
	NosuchLife* _life;
	LifeCellData* _data;
	float _deepestdepth;
	LifeCell* _deepestcell;
	int _ncols;
	int _nrows;
	long _gen;
	int _savedkey;
	int _savedrow;
	int _savedcol;
	long _cellseq;
	bool _doage;
	bool _sprites;
	int _sparseness;
};

#endif
