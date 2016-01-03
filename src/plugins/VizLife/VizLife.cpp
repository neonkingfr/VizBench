#include "Vizlet.h"
#include "VizLife.h"

static CFFGLPluginInfo PluginInfo ( 
	VizLife::CreateInstance,	// Create method
	"VZLF",		// Plugin unique ID
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

float
randdepth() {
	return 0.1f + (rand() % 80) / 100.0f;  // 0.1-0.9
}

VizLife::VizLife() : Vizlet(), LifeListener() {
	m_savedkey = 0;
	m_savedrow = m_savedcol = -1;
	m_params = defaultParams();
	m_cellparams = new SpriteVizParams();
	m_gen = 0;
	m_doage = false;
	m_cellseq = 0;
	m_life = NULL;
	m_sparseness = 3;
	m_sprites = true;
	SetSize( 64 , 64);
}

void
VizLife::SetSize(int rows, int cols) {

	m_nrows = rows;
	m_ncols = cols;

	// Should probably delete m_life if it's non-NULL
	m_life = new NosuchLife(*this,rows,cols);

	int ncells = rows * cols;
	m_data = new LifeCellData[ncells];
	for (int r = 0; r < rows; r++) {
		for (int c = 0; c < cols; c++) {
			int i = r * m_ncols + c;
			LifeCellData* d = &(m_data[i]);
			d->lifetime = 200;
			d->genborn = 0;
			d->row = r;
			d->col = c;
			d->zdepth = randdepth();
			m_life->setData(r, c, d);
		}
	}
}

LifeCellData&
VizLife::getData(int r, int c) {
	int i = r * m_ncols + c;
	return m_data[i];
}

VizLife::~VizLife() {
}

float VizLife::col2x(int col) {
	return float(col) / m_ncols;
}
float VizLife::row2y(int row) {
	return float(row) / m_nrows;
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
	data->genborn = m_gen;
	data->zdepth = avg_depth;
	data->seq = m_cellseq++;

	if (((data->seq) % 100) == 0) {
		addCellSprite(data->row, data->col);
	}
	// DEBUGPRINT(("cellBirth r=%d c=%d gen=%d genborn=%d", r, c, m_gen, oldest));
}


void
VizLife::addCellSprite(int r, int c)
{
	if (!m_sprites) {
		return;
	}
	m_cellparams->shape.set("circle");
	m_cellparams->filled.set(true);
	m_cellparams->sizeinitial.set(0.01);
	m_cellparams->sizefinal.set(0.3);
	m_cellparams->sizetime.set(1.0);
	m_cellparams->alphainitial.set(0.8);
	m_cellparams->alphafinal.set(0.1);
	m_cellparams->alphatime.set(1.0);

	float x = col2x(c);
	float y = row2y(r);
	NosuchPos pos = NosuchPos(x, y, 0.5);
	makeAndAddVizSprite(m_cellparams, pos);
}

void
VizLife::onCellSurvive(int row, int col, LifeCell& cell) {
	// DEBUGPRINT(("VizLife Survive r=%d c=%d", row, col));
	// getData(row, col).genborn = m_gen;
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
	int age = m_gen - d.genborn;
	float alpha = 1.0f - (float(age) / d.lifetime);

	if (d.zdepth > m_deepestdepth) {
		m_deepestdepth = d.zdepth;
		m_deepestcell = &cell;
	}

	float sz = (w / m_ncols) / 2.0f;

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
	m_life->Gen();
	m_gen++;
}

void
VizLife::Clear() {
	for (int r = 0; r < m_nrows; r++) {
		for (int c = 0; c < m_ncols; c++) {
			LifeCell& cell = m_life->Cell(r, c);
			cell.setVal(false);
		}
	}
}

void
VizLife::DeleteCellAndTouching(int r, int c) {
	LifeCell& cell = m_life->Cell(r, c);
	cell.setVal(false);
	LifeCell& cellcheck = m_life->Cell(r, c);
	// DEBUGPRINT(("Cell %d %d dying due to age, val=%d", r, c, cellcheck.val()));

	// If any of the surrounding cells are alive, recurse
	LifeCell& cell0 = m_life->Cell(r-1, c-1);
	if (cell0.val()) {
		DeleteCellAndTouching(r - 1, c - 1); }

	LifeCell& cell1 = m_life->Cell(r-1, c);
	if (cell1.val()) {
		DeleteCellAndTouching(r - 1, c ); }

	LifeCell& cell2 = m_life->Cell(r-1, c+1);
	if (cell2.val()) {
		DeleteCellAndTouching(r - 1, c + 1); }

	LifeCell& cell3 = m_life->Cell(r, c-1);
	if (cell3.val()) {
		DeleteCellAndTouching(r, c - 1); }

	LifeCell& cell4 = m_life->Cell(r, c+1);
	if (cell4.val()) {
		DeleteCellAndTouching(r, c + 1); }

	LifeCell& cell6 = m_life->Cell(r+1, c-1);
	if (cell6.val()) {
		DeleteCellAndTouching(r + 1, c - 1); }

	LifeCell& cell7 = m_life->Cell(r+1, c);
	if (cell7.val()) {
		DeleteCellAndTouching(r + 1, c ); }

	LifeCell& cell8 = m_life->Cell(r+1, c+1);
	if (cell8.val()) {
		DeleteCellAndTouching(r + 1, c + 1); }
}

void
VizLife::Age() {
	for (int r = 0; r < m_nrows; r++) {
		for (int c = 0; c < m_ncols; c++) {
			LifeCell& cell = m_life->Cell(r, c);
			if (!cell.val()) {
				continue;
			}
			LifeCellData& d = getData(r, c);
			int dg = m_gen - d.genborn;
			if (dg > d.lifetime) {
				DeleteCellAndTouching(r, c);
			}
		}
	}
}

void
VizLife::RandomAddEvery(int sparseness) {
	for (int r = 0; r < m_nrows; r++) {
		for (int c = 0; c < m_ncols; c++) {
			if ((rand() % sparseness) == 0) {
				fakeCellBirth(r, c);
			}
		}
	}
}
void VizLife::processKeystroke(int key, int downup) {
	if ( downup ) {
		m_savedkey = key;
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
		m_doage = !m_doage;
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
		m_life->setWrap( ! m_life->getWrap() );
		DEBUGPRINT(("Wrap is now %d", m_life->getWrap()));
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
	row = int((c->pos.y * m_nrows)+0.5f);
	row = limit(row, 0, m_nrows - 1);

	col = int((c->pos.x * m_ncols)+0.5f);
	col = limit(col, 0, m_ncols - 1);
}

void
VizLife::fakeCellBirth(int r, int c) {

	LifeCell& cell = m_life->Cell(r, c);

	cell.setVal(true);
	LifeCellData* data = (LifeCellData*) cell.data();
	if (data) {
		data->zdepth = randdepth();
	}

	LifeCell& cell_ul = m_life->Cell(r - 1, c - 1);
	LifeCell& cell_um = m_life->Cell(r - 1, c);
	LifeCell& cell_ur = m_life->Cell(r - 1, c + 1);

	LifeCell& cell_ml = m_life->Cell(r, c - 1);
	LifeCell& cell_mm = m_life->Cell(r, c);
	LifeCell& cell_mr = m_life->Cell(r, c + 1);

	LifeCell& cell_ll = m_life->Cell(r + 1, c - 1);
	LifeCell& cell_lm = m_life->Cell(r + 1, c);
	LifeCell& cell_lr = m_life->Cell(r + 1, c + 1);

	onCellBirth(r, c, cell,
		cell_ul, cell_um, cell_ur,
		cell_ml, cell_mm, cell_mr,
		cell_ll, cell_lm, cell_lr);
}

void VizLife::processCursor(VizCursor* c, int downdragup) {
	m_params->shape.set("square");
	m_params->filled.set(true);
	m_params->sizeinitial.set(1.0);
	m_params->sizefinal.set(0.1);
	m_params->sizetime.set(0.5);
	m_params->alphainitial.set(1.0);
	m_params->alphafinal.set(0.0);
	m_params->alphatime.set(0.5);

	// VizSprite* s = makeAndAddVizSprite(m_params, c->pos);
	// VizSpriteOutline* so = (VizSpriteOutline*)s;

	// NO OpenGL calls here
	double minarea = 0.05;
	if (c->area < 0.0 || c->area > minarea) {
		int row, col;
		cursor2Cell(c,row,col);
		if (downdragup == CURSOR_DOWN) {
			DEBUGPRINT(("fakeCellBirth (down) at row/col = %d / %d",m_savedrow,m_savedcol));
			fakeCellBirth(row, col);
			m_savedrow = row;
			m_savedcol = col;
		}
		else if (downdragup == CURSOR_DRAG) {
			while (m_savedrow>=0 && m_savedcol>= 0 && row != m_savedrow && col != m_savedcol) {
				int dr = row - m_savedrow;
				int dc = col - m_savedcol;
				if (dr == dc) {
					m_savedrow += (dr < 0 ? -1 : 1);
					m_savedcol += (dc < 0 ? -1 : 1);
				}
				else if (abs(dr) > abs(dc)) {
					m_savedrow += (dr < 0 ? -1 : 1);
				}
				else if (abs(dc) > abs(dr)) {
					m_savedcol += (dc < 0 ? -1 : 1);
				}
				else {
					DEBUGPRINT(("VizLife should not get here"));
					break;
				}
				DEBUGPRINT(("fakeCellBirth (drag) at row/col = %d / %d",m_savedrow,m_savedcol));
				fakeCellBirth(m_savedrow, m_savedcol);
			}
		}
	}

	if (downdragup == CURSOR_UP) {
		m_savedrow = -1;
		m_savedcol = -1;
	}
}

std::string VizLife::processJson(std::string meth, cJSON *json, const char *id) {

	// NO OpenGL calls here

	if (meth == "apis") {
		return jsonStringResult("randomize;clear;set_sparseness(amount);set_size(size);set_wrap(onoff);set_sprites(onoff)", id);
	}

	if (meth == "randomize") {
		RandomAddEvery(m_sparseness);
		return jsonOK(id);
	}

	if ( meth == "clear" ) {
		Clear();
		return jsonOK(id);
	}

	//  PARAMETER "wrap"
	if (meth == "set_wrap") {
		bool onoff = jsonNeedBool(json, "onoff", 1);
		m_life->setWrap(onoff);
		return jsonOK(id);
	}
	if (meth == "get_wrap") {
		std::string val = m_life->getWrap() ? "on" : "off";
		return jsonStringResult(val,id);
	}

	//  PARAMETER "sparseness"
	if (meth == "set_sparseness") {
		m_sparseness = jsonNeedInt(json, "amount", 1);
		return jsonOK(id);
	}
	if (meth == "get_sparseness") {
		std::string val = NosuchSnprintf("%d",m_sparseness);
		return jsonStringResult(val,id);
	}

	//  PARAMETER "sprites"
	if (meth == "set_sprites") {
		m_sprites = jsonNeedBool(json, "onoff", 1);
		return jsonOK(id);
	}
	if (meth == "get_sprites") {
		std::string val = m_sprites ? "on" : "off";
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
		std::string val = NosuchSnprintf("%dx%d", m_nrows, m_ncols);
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

	if (m_savedkey > 0) {
		DoKey(m_savedkey);
		m_savedkey = 0;
	}

	// OpenGL calls here
	DrawVizSprites();
	m_deepestcell = NULL;
	m_deepestdepth = 0.0;
	m_life->Draw();
#if 0
	if (m_deepestcell) {
		LifeCellData* data = (LifeCellData*) m_deepestcell->data();
		if (data) {
			addCellSprite(data->row, data->col);
		}
	}
#endif
	Gen();
	if (m_doage) {
		Age();
	}

	return true;
}

void VizLife::processDrawNote(MidiMsg* m) {
	// OpenGL calls here
}
