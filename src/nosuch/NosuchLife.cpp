/*
	Copyright (c) 2011-2013 Tim Thompson <me@timthompson.com>

	Permission is hereby granted, free of charge, to any person obtaining
	a copy of this software and associated documentation files
	(the "Software"), to deal in the Software without restriction,
	including without limitation the rights to use, copy, modify, merge,
	publish, distribute, sublicense, and/or sell copies of the Software,
	and to permit persons to whom the Software is furnished to do so,
	subject to the following conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	Any person wishing to distribute modifications to the Software is
	requested to send the modifications to the original developer so that
	they can be incorporated into the canonical version.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
	IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
	ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
	CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
	WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "Vizlet.h"
#include "NosuchLife.h"
#include "NosuchGraphics.h"

NosuchLife::NosuchLife(LifeListener& listener, int nrows, int ncols) : m_listener(listener) {

	m_nrows = nrows;
	m_ncols = ncols;
	m_wrap = true;

	int ncells = m_nrows * m_ncols;
	m_cells = new LifeCell[ncells];
	m_nextcells = new LifeCell[ncells];
}

void
NosuchLife::Gen() {

	for (int r = 0; r < m_nrows; r++) {
		int rstart = r * m_ncols;
		for (int c = 0; c < m_ncols; c++) {

			LifeCell& cell_ul = Cell(r - 1, c - 1);
			LifeCell& cell_um = Cell(r - 1, c);
			LifeCell& cell_ur = Cell(r - 1, c + 1);

			LifeCell& cell_ml = Cell(r, c - 1);
			LifeCell& cell_mm = Cell(r, c);
			LifeCell& cell_mr = Cell(r, c + 1);

			LifeCell& cell_ll = Cell(r + 1, c - 1);
			LifeCell& cell_lm = Cell(r + 1, c);
			LifeCell& cell_lr = Cell(r + 1, c + 1);

			int tot = cell_ul.val() + cell_um.val() + cell_ur.val()
				+ cell_ml.val() + cell_mr.val()
				+ cell_ll.val() + cell_lm.val() + cell_lr.val();

			int v = cell_mm.val();  // The middle (i.e. current) cell

			LifeCell& nextcell = NextCell(r, c);

			if (v == 1 ) {
				if (tot < 2 || tot > 3) {
					// dies due to under- or over-population
					nextcell.setVal(false);
					m_listener.onCellDeath(r, c, nextcell);
				} else {
					nextcell.setVal(true);  // didn't change
					m_listener.onCellSurvive(r, c, nextcell);
				}
			}
			else {
				if (tot == 3) {
					// born
					nextcell.setVal(true);
					m_listener.onCellBirth(r, c, nextcell,
						cell_ul,cell_um,cell_ur,
						cell_ml,cell_mm,cell_mr,
						cell_ll,cell_lm,cell_lr);
				}
				else {
					nextcell.setVal(false);  // didn't change
				}
			}
		}
	}
	LifeCell* t = m_cells;
	m_cells = m_nextcells;
	m_nextcells = t;
}

void
NosuchLife::Draw() {
	for (int r = 0; r < m_nrows; r++) {
		for (int c = 0; c < m_ncols; c++) {
			LifeCell& cell = Cell(r, c);
			if (cell.val()) {
				m_listener.onCellDraw(r, c, cell);
			}
		}
	}
}