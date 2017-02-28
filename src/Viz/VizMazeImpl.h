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

#ifndef NOSUCHMAZE_H
#define NOSUCHMAZE_H

// VizMazeImpl is really just a pixel map with simplified
// and deterministic physics, and behaviours attached to the pixels

class MazePoint {
public:
	MazePoint() : x(0), y(0) {}
	MazePoint(int x0, int y0) : x(x0), y(y0) {}
	int x;
	int y;
};

class MazeRowCol {
public:
	MazeRowCol() : r(0), c(0) {}
	MazeRowCol(int r0, int c0) : r(r0), c(c0) {}
	int r;
	int c;
};

class MazeCell {
public:
	MazeCell() {
		m_filled = false;
		m_data = NULL;
	}
	void* getData() { return m_data; }
	void setData(void* d) { m_data = d; }
	bool getFilled() { return m_filled; }
	void setFilled(bool onoff) { m_filled = onoff; }
private:
	friend class VizMazeImpl;
	bool m_filled;
	void* m_data;
};

class MazeBall {
public:
	MazeBall();
	MazeBall(MazePoint xy, MazePoint dxy, int born, int lifetime);
	void* getData() { return m_data; }
	void setData(void* d) { m_data = d; }
	MazePoint xy() { return m_xy; }

private:
	friend class VizMazeImpl;
	MazePoint m_xy;
	MazePoint m_dxy;	// delta per click
	int m_born;		// in clicks
	int m_age;		// in clicks
	int m_lifetime;	// in clicks
	bool m_alive;
	void* m_data;

	void reset(MazePoint xy, MazePoint dxy, int born, int lifetime);
};

class MazeListener {
public:
	virtual ~MazeListener() {}
	virtual void onCellHit(MazeCell& cell, MazeBall* b) = 0;
	virtual void onCellDraw(MazeCell& cell, MazeRowCol rc) = 0;
	virtual void onBallDraw(MazeBall* b, MazePoint xy) = 0;
};

class VizMazeImpl {
public:
	VizMazeImpl(MazeListener& listener, int nrows, int ncols, int cellxsize, int cellysize);
	void Draw();
	void setCellData(MazeRowCol rc, void* d) {
		getCell(rc).setData(d);
	}
	void* getCellData(MazeRowCol rc) {
		return getCell(rc).getData();
	}
	void addLine(MazeRowCol rc0, MazeRowCol rc1);
	MazeBall* addBall(MazePoint fxy, MazePoint dxy, int born, int lifetime);
	void moveBalls(int nclicks);
	void moveBallOneClick(MazeBall* b);
	bool isFilled(MazeRowCol rc);

	MazePoint xySize() { return MazePoint(m_cellxsize*m_ncols, m_cellysize*m_nrows); }
	MazePoint cellSize() { return MazePoint(m_cellxsize, m_cellysize); }
	MazeRowCol rowcolSize() { return MazeRowCol(m_nrows, m_ncols); }

	void killBall(MazeBall* b) {
		b->m_alive = false;
		DEBUGPRINT(("Killing ball!"));
	}

	MazeCell& getCell(MazePoint xy) {
		return getCell(getRowCol(xy));
	}

	MazeRowCol getRowCol(MazePoint xy) {
		int col = xy.x / m_cellxsize;
		int row = xy.y / m_cellysize;
		return MazeRowCol(row, col);
	}

	MazePoint getPoint(MazeRowCol rc) {
		return MazePoint( rc.c * m_cellxsize, rc.r * m_cellysize );
	}

	MazeCell& getCell(MazeRowCol rc) {

		// XXX - lots of room for optimization here, particularly
		// for loops that walk through the entire maze

		// Handle requests beyond the maze borders
		if (rc.c < 0) {
			return _zero;
		}
		else if (rc.c >= m_ncols) {
			return _zero;
		}
		if (rc.r < 0) {
			return _zero;
		}
		else if (rc.r >= m_nrows) {
			return _zero;
		}
		return m_cells[rc.r * m_ncols + rc.c];
	}

private:
	MazeListener& m_listener;
	MazeCell* m_cells;
	MazeCell _zero;

	// int m_xsize;
	// int m_ysize;

	int m_nrows;
	int m_ncols;

	int m_cellxsize;
	int m_cellysize;

	std::list<MazeBall*> m_balls;
};

#endif
