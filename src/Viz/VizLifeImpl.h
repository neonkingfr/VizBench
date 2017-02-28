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

#ifndef NOSUCHLIFE_H
#define NOSUCHLIFE_H

class LifeCell {
public:
	LifeCell() {
		m_on = false;
		m_data = NULL;
	}
	int val() { return (m_on == true) ? 1 : 0; }
	void* data() { return m_data; }
	void setVal(bool onoff) { m_on = onoff; }
private:
	friend class VizLifeImpl;  // so it can use setData
	void setData(void* d) { m_data = d; }
	bool m_on;
	void* m_data;
};

class LifeListener {
public:
	virtual ~LifeListener() {}
	virtual void onCellDraw(int r, int c, LifeCell& cell) = 0;
	virtual void onCellBirth(int r, int c, LifeCell& cell,
		LifeCell& cell_ul, LifeCell& cell_um, LifeCell& cell_ur,
		LifeCell& cell_ml, LifeCell& cell_mm, LifeCell& cell_mr,
		LifeCell& cell_ll, LifeCell& cell_lm, LifeCell& cell_lr ) = 0;
	virtual void onCellSurvive(int r, int c, LifeCell& cell) = 0;
	virtual void onCellDeath(int r, int c, LifeCell& cell) = 0;
};

class VizLifeImpl {
public:
	VizLifeImpl(LifeListener& listener, int rows, int cols);
	void Draw();
	void Gen();
	LifeCell& Cell(int r, int c) {
		return _getcell(r, c, m_cells);
	}
	void setData(int row, int col, void* d) {
		// Set it in BOTH cell grids
		_getcell(row, col, m_cells).setData(d);
		_getcell(row, col, m_nextcells).setData(d);
	}
	void setWrap(bool w) { m_wrap = w; }
	bool getWrap() { return m_wrap; }
private:
	LifeCell& NextCell(int r, int c) {
		return _getcell(r, c, m_nextcells);
	}
	LifeCell& _getcell(int r, int c, LifeCell* cells) {
		// Handle requests for row and col just beyond borders
		if (r == -1) {
			if (!m_wrap) {
				return _zero;
			}
			r = m_nrows - 1;
		}
		else if (r == m_nrows) {
			if (!m_wrap) {
				return _zero;
			}
			r = 0;
		}
		if (c == -1) {
			if (!m_wrap) {
				return _zero;
			}
			c = m_ncols - 1;
		}
		else if (c == m_ncols) {
			if (!m_wrap) {
				return _zero;
			}
			c = 0;
		}
		return cells[r *m_ncols + c];
	}
	LifeListener& m_listener;
	LifeCell* m_cells;
	LifeCell* m_nextcells;
	LifeCell _zero;
	int m_nrows;
	int m_ncols;
	bool m_wrap;
};

#endif
