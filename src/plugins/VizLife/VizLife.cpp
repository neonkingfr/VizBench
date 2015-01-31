#include "NosuchDebug.h"
#include "NosuchUtil.h"
#include "ffutil.h"

#include "Vizlet.h"
#include "VizLife.h"
#include "NosuchOsc.h"

#include "VizSprite.h"
#include "NosuchLife.h"
#include "VizServer.h"

static CFFGLPluginInfo PluginInfo ( 
	VizLife::CreateInstance,	// Create method
	"V510",		// Plugin unique ID
	"VizLife",	// Plugin name	
	1,			// API major version number
	000,		// API minor version number	
	1,			// Plugin major version number
	000,		// Plugin minor version number
	FF_EFFECT,	// Plugin type
	"VizLife: experimenting with Conway's game of life",	// description
	"by Tim Thompson - me@timthompson.com" 			// About
);

std::string vizlet_name() { return "VizLife"; }
CFFGLPluginInfo& vizlet_plugininfo() { return PluginInfo; }
// void vizlet_setdll(std::string dll) { }

float
randdepth() {
	return 0.1f + (rand() % 80) / 100.0f;  // 0.1-0.9
}

VizLife::VizLife() : Vizlet(), LifeListener() {
	_savedkey = 0;
	_savedrow = _savedcol = -1;
	_params = defaultParams();
	_cellparams = new AllVizParams(true);
	_gen = 0;
	_doage = false;
	_cellseq = 0;
	_life = NULL;
	_sparseness = 3;
	_sprites = true;
	SetSize( 64 , 64);
}

void
VizLife::SetSize(int rows, int cols) {

	_nrows = rows;
	_ncols = cols;

	// Should probably delete _life if it's non-NULL
	_life = new NosuchLife(*this,rows,cols);

	int ncells = rows * cols;
	_data = new LifeCellData[ncells];
	for (int r = 0; r < rows; r++) {
		for (int c = 0; c < cols; c++) {
			int i = r * _ncols + c;
			LifeCellData* d = &(_data[i]);
			d->lifetime = 200;
			d->genborn = 0;
			d->row = r;
			d->col = c;
			d->zdepth = randdepth();
			_life->setData(r, c, d);
		}
	}
}

LifeCellData&
VizLife::getData(int r, int c) {
	int i = r * _ncols + c;
	return _data[i];
}

VizLife::~VizLife() {
}

float VizLife::col2x(int col) {
	return float(col) / _ncols;
}
float VizLife::row2y(int row) {
	return float(row) / _nrows;
}

float
VizLife::CellDepth(LifeCell& cell) {
	LifeCellData* d = (LifeCellData*) cell.data();
	if (d) {
		return d->zdepth;
	}
	else {
		return -1;
	}
}

void
VizLife::onCellBirth(int r, int c, LifeCell& cell,
LifeCell& cell_ul, LifeCell& cell_um, LifeCell& cell_ur,
LifeCell& cell_ml, LifeCell& cell_mm, LifeCell& cell_mr,
LifeCell& cell_ll, LifeCell& cell_lm, LifeCell& cell_lr)
{
	LifeCellData* data = (LifeCellData*)cell.data();
	// We want the genborn of the born cell to be the average of the active
	// cells around it.
	int oldest = INT_MAX;

	float tot = 0;
	int nactive = 0;
	float a;

	a = CellDepth(cell_ul); if (a >= 0) { tot += a; nactive++; }
	a = CellDepth(cell_um); if (a >= 0) { tot += a; nactive++; }
	a = CellDepth(cell_ur); if (a >= 0) { tot += a; nactive++; }

	a = CellDepth(cell_ml); if (a >= 0) { tot += a; nactive++; }
	a = CellDepth(cell_mr); if (a >= 0) { tot += a; nactive++; }

	a = CellDepth(cell_ll); if (a >= 0) { tot += a; nactive++; }
	a = CellDepth(cell_lm); if (a >= 0) { tot += a; nactive++; }
	a = CellDepth(cell_lr); if (a >= 0) { tot += a; nactive++; }

	float avg_depth;
	if (nactive == 0) {
		// shouldn't happen, but...
		avg_depth = randdepth();
	}
	else {
		avg_depth = tot / nactive;
	}
	data->genborn = _gen;
	data->zdepth = avg_depth;
	data->seq = _cellseq++;

	if (((data->seq) % 100) == 0) {
		addCellSprite(data->row, data->col);
	}
	// DEBUGPRINT(("cellBirth r=%d c=%d gen=%d genborn=%d", r, c, _gen, oldest));
}


void
VizLife::addCellSprite(int r, int c)
{
	if (!_sprites) {
		return;
	}
	_cellparams->shape.set("circle");
	_cellparams->filled.set(true);
	_cellparams->sizeinitial.set(0.01);
	_cellparams->sizefinal.set(0.3);
	_cellparams->sizetime.set(1.0);
	_cellparams->alphainitial.set(0.8);
	_cellparams->alphafinal.set(0.1);
	_cellparams->alphatime.set(1.0);

	float x = col2x(c);
	float y = row2y(r);
	NosuchPos pos = NosuchPos(x, y, 0.5);
	makeAndAddVizSprite(_cellparams, pos);
}

void
VizLife::onCellSurvive(int row, int col, LifeCell& cell) {
	// DEBUGPRINT(("VizLife Survive r=%d c=%d", row, col));
	// getData(row, col).genborn = _gen;
	// LifeCellData& d = getData(row, col);
	// d.genborn++;
}

void
VizLife::onCellDeath(int row, int col, LifeCell& cell) {
	// DEBUGPRINT(("VizLife Death r=%d c=%d", row, col));
}

void
VizLife::onCellDraw(int row, int col, LifeCell& cell)
{
	float w = 1.0f;
	float x = col2x(col);
	float y = row2y(row);

	LifeCellData& d = getData(row, col);
	int age = _gen - d.genborn;
	float alpha = 1.0f - (float(age) / d.lifetime);

	if (d.zdepth > _deepestdepth) {
		_deepestdepth = d.zdepth;
		_deepestcell = &cell;
	}

	float sz = (w / _ncols) / 2.0f;

	glColor4f(0.0, 1.0, 0.0, alpha);
	glLineWidth((GLfloat)3.0f);

	glBegin(GL_QUADS);
	// glBegin(GL_LINE_LOOP);

	glVertex3f(x , y , 0.0f);
	glVertex3f(x , y + 2*sz, 0.0f);
	glVertex3f(x + 2*sz, y + 2*sz, 0.0f);
	glVertex3f(x + 2*sz, y , 0.0f);
	glEnd();

}

DWORD __stdcall VizLife::CreateInstance(CFreeFrameGLPlugin **ppInstance) {
	*ppInstance = new VizLife();
	return (*ppInstance != NULL)? FF_SUCCESS : FF_FAIL;
}

void
VizLife::Gen() {
	_life->Gen();
	_gen++;
}

void
VizLife::Clear() {
	for (int r = 0; r < _nrows; r++) {
		for (int c = 0; c < _ncols; c++) {
			LifeCell& cell = _life->Cell(r, c);
			cell.setVal(false);
		}
	}
}

void
VizLife::DeleteCellAndTouching(int r, int c) {
	LifeCell& cell = _life->Cell(r, c);
	cell.setVal(false);
	LifeCell& cellcheck = _life->Cell(r, c);
	// DEBUGPRINT(("Cell %d %d dying due to age, val=%d", r, c, cellcheck.val()));

	// If any of the surrounding cells are alive, recurse
	LifeCell& cell0 = _life->Cell(r-1, c-1);
	if (cell0.val()) {
		DeleteCellAndTouching(r - 1, c - 1); }

	LifeCell& cell1 = _life->Cell(r-1, c);
	if (cell1.val()) {
		DeleteCellAndTouching(r - 1, c ); }

	LifeCell& cell2 = _life->Cell(r-1, c+1);
	if (cell2.val()) {
		DeleteCellAndTouching(r - 1, c + 1); }

	LifeCell& cell3 = _life->Cell(r, c-1);
	if (cell3.val()) {
		DeleteCellAndTouching(r, c - 1); }

	LifeCell& cell4 = _life->Cell(r, c+1);
	if (cell4.val()) {
		DeleteCellAndTouching(r, c + 1); }

	LifeCell& cell6 = _life->Cell(r+1, c-1);
	if (cell6.val()) {
		DeleteCellAndTouching(r + 1, c - 1); }

	LifeCell& cell7 = _life->Cell(r+1, c);
	if (cell7.val()) {
		DeleteCellAndTouching(r + 1, c ); }

	LifeCell& cell8 = _life->Cell(r+1, c+1);
	if (cell8.val()) {
		DeleteCellAndTouching(r + 1, c + 1); }
}

void
VizLife::Age() {
	for (int r = 0; r < _nrows; r++) {
		for (int c = 0; c < _ncols; c++) {
			LifeCell& cell = _life->Cell(r, c);
			if (!cell.val()) {
				continue;
			}
			LifeCellData& d = getData(r, c);
			int dg = _gen - d.genborn;
			if (dg > d.lifetime) {
				DeleteCellAndTouching(r, c);
			}
		}
	}
}

void
VizLife::RandomAddEvery(int sparseness) {
	for (int r = 0; r < _nrows; r++) {
		for (int c = 0; c < _ncols; c++) {
			LifeCell& cell = _life->Cell(r, c);
			if ((rand() % sparseness) == 0) {
				LifeCellData* data = (LifeCellData*) cell.data();
				fakeCellBirth(r, c, cell);
			}
		}
	}
}
void VizLife::processKeystroke(int key, int downup) {
	if ( downup ) {
		_savedkey = key;
	}
}

void VizLife::DoKey(int key) {
	switch (key) {
	case 71:  // g
		Gen();
		break;
	case 67:  // c
		Clear();
		break;
	case 65:  // a
		_doage = !_doage;
		break;
	case 82: // r
		RandomAddEvery(10);
		break;
	case 83: // s
		RandomAddEvery(20);
		break;
	case 84: // t
		RandomAddEvery(30);
		break;
	case 87: // w
		_life->setWrap( ! _life->getWrap() );
		DEBUGPRINT(("Wrap is now %d", _life->getWrap()));
		break;
	case 49: // 1
		Clear();
		SetSize(20, 20);
		break;
	case 50: // 2
		Clear();
		SetSize(100, 100);
		break;
	case 51: // 3
		Clear();
		SetSize(256, 256);
		break;
	default:
		DEBUGPRINT(("Unrecognized key=%d", key));
		break;
	}
}

int
limit(int v, int mn, int mx) {
	if (v < mn) {
		v = mn;
	}
	else if (v > mx) {
		v = mx;
	}
	return v;
}

void
VizLife::cursor2Cell(VizCursor* c, int& row, int& col) {
	row = int((c->pos.y * _nrows)+0.5f);
	row = limit(row, 0, _nrows - 1);

	col = int((c->pos.x * _ncols)+0.5f);
	col = limit(col, 0, _ncols - 1);
}

void
VizLife::fakeCellBirth(int r, int c, LifeCell& cell) {

	cell.setVal(true);
	LifeCellData* data = (LifeCellData*) cell.data();
	if (data) {
		data->zdepth = randdepth();
	}

	LifeCell& cell_ul = _life->Cell(r - 1, c - 1);
	LifeCell& cell_um = _life->Cell(r - 1, c);
	LifeCell& cell_ur = _life->Cell(r - 1, c + 1);

	LifeCell& cell_ml = _life->Cell(r, c - 1);
	LifeCell& cell_mm = _life->Cell(r, c);
	LifeCell& cell_mr = _life->Cell(r, c + 1);

	LifeCell& cell_ll = _life->Cell(r + 1, c - 1);
	LifeCell& cell_lm = _life->Cell(r + 1, c);
	LifeCell& cell_lr = _life->Cell(r + 1, c + 1);

	onCellBirth(r, c, cell,
		cell_ul, cell_um, cell_ur,
		cell_ml, cell_mm, cell_mr,
		cell_ll, cell_lm, cell_lr);
}

void VizLife::processCursor(VizCursor* c, int downdragup) {
#ifdef DRAWOUTLINE
	if ( c->outline != NULL ) {
		_params->shape.set("outline");
	} else {
		_params->shape.set("square");
	}
#else
	_params->shape.set("square");
#endif
	_params->filled.set(true);
	_params->sizeinitial.set(1.0);
	_params->sizefinal.set(0.1);
	_params->sizetime.set(0.5);
	_params->alphainitial.set(1.0);
	_params->alphafinal.set(0.0);
	_params->alphatime.set(0.5);
	VizSprite* s = makeAndAddVizSprite(_params, c->pos);
	VizSpriteOutline* so = (VizSpriteOutline*)s;
#ifdef DRAWOUTLINE
	if ( so != NULL && c->outline != NULL ) {
		so->setOutline(c->outline,c->hdr);
	}
#endif

	// NO OpenGL calls here
	double minarea = 0.05;
	if (c->area > minarea) {
		int row, col;
		cursor2Cell(c,row,col);
		LifeCell& cell = _life->Cell(row,col);
		if (downdragup == CURSOR_DOWN) {
			fakeCellBirth(row, col, cell);
			_savedrow = row;
			_savedcol = col;
		}
		else if (downdragup == CURSOR_DRAG) {
			while (_savedrow>=0 && _savedcol>= 0 && row != _savedrow && col != _savedcol) {
				int dr = row - _savedrow;
				int dc = col - _savedcol;
				if (dr == dc) {
					_savedrow += (dr < 0 ? -1 : 1);
					_savedcol += (dc < 0 ? -1 : 1);
				}
				else if (abs(dr) > abs(dc)) {
					_savedrow += (dr < 0 ? -1 : 1);
				}
				else if (abs(dc) > abs(dr)) {
					_savedcol += (dc < 0 ? -1 : 1);
				}
				else {
					DEBUGPRINT(("VizLife should not get here"));
					break;
				}
				LifeCell& cell = _life->Cell(_savedrow,_savedcol);
				fakeCellBirth(_savedrow, _savedcol, cell);
			}
		}
	}

	if (downdragup == CURSOR_UP) {
		_savedrow = -1;
		_savedcol = -1;
	}
}

std::string VizLife::processJson(std::string meth, cJSON *json, const char *id) {

	// NO OpenGL calls here

	if (meth == "apis") {
		return jsonStringResult("randomize;clear;set_sparseness(amount);set_size(size);set_wrap(onoff);set_sprites(onoff)", id);
	}

	if (meth == "randomize") {
		RandomAddEvery(_sparseness);
		return jsonOK(id);
	}

	if ( meth == "clear" ) {
		Clear();
		return jsonOK(id);
	}

	//  PARAMETER "wrap"
	if (meth == "set_wrap") {
		bool onoff = jsonNeedBool(json, "onoff", 1);
		_life->setWrap(onoff);
		return jsonOK(id);
	}
	if (meth == "get_wrap") {
		std::string val = _life->getWrap() ? "on" : "off";
		return jsonStringResult(val,id);
	}

	//  PARAMETER "sparseness"
	if (meth == "set_sparseness") {
		_sparseness = jsonNeedInt(json, "amount", 1);
		return jsonOK(id);
	}
	if (meth == "get_sparseness") {
		std::string val = NosuchSnprintf("%d",_sparseness);
		return jsonStringResult(val,id);
	}

	//  PARAMETER "sprites"
	if (meth == "set_sprites") {
		_sprites = jsonNeedBool(json, "onoff", 1);
		return jsonOK(id);
	}
	if (meth == "get_sprites") {
		std::string val = _sprites ? "on" : "off";
		return jsonStringResult(val,id);
	}

	//  PARAMETER "size"
	if (meth == "set_size") {
		std::string sz = jsonNeedString(json, "size", "20x20");
		int rows, cols;
		if (sscanf(sz.c_str(), "%dx%d", &rows, &cols) != 2) {
			return jsonError(-32000, "Unable to parse size value, expecting something like 20x20", id);
		}
		if (rows <= 0 || cols <= 0) {
			return jsonError(-32000, "row and col values should be non-zero and positive",id);
		}
		SetSize( rows , cols);
		return jsonOK(id);
	}
	if (meth == "get_size") {
		std::string val = NosuchSnprintf("%dx%d", _nrows, _ncols);
		return jsonStringResult(val, id);
	}

	throw NosuchException("VizLife - Unrecognized method '%s'",meth.c_str());
}

void VizLife::processMidiInput(MidiMsg* m) {
	// NO OpenGL calls here
}

void VizLife::processMidiOutput(MidiMsg* m) {
	// NO OpenGL calls here
}

bool VizLife::processDraw() {

	if (_savedkey > 0) {
		DoKey(_savedkey);
		_savedkey = 0;
	}

	// OpenGL calls here
	DrawVizSprites();
	_deepestcell = NULL;
	_deepestdepth = 0.0;
	_life->Draw();
#if 0
	if (_deepestcell) {
		LifeCellData* data = (LifeCellData*) _deepestcell->data();
		if (data) {
			addCellSprite(data->row, data->col);
		}
	}
#endif
	Gen();
	if (_doage) {
		Age();
	}

	return true;
}

void VizLife::processDrawNote(MidiMsg* m) {
	// OpenGL calls here
}
