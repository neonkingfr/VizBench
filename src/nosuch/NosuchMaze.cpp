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
#include "NosuchMaze.h"
#include "NosuchGraphics.h"

MazeBall::MazeBall() {
	reset(MazePoint(0, 0), MazePoint(0, 0), 0, 0);
}
MazeBall::MazeBall(MazePoint fxy, MazePoint dfxy, int born, int lifetime) {
	reset(fxy, dfxy, born, lifetime);
}

void
MazeBall::reset(MazePoint xy, MazePoint dxy, int born, int lifetime) {
	m_xy = xy;
	m_dxy = dxy;
	m_born = born;
	m_age = 0;
	m_lifetime = lifetime;
	m_alive = true;
	m_data = NULL;
}

NosuchMaze::NosuchMaze(MazeListener& listener, int nrows, int ncols, int cellxsize, int cellysize) : m_listener(listener) {

	m_nrows = nrows;
	m_ncols = ncols;
	m_cellxsize = cellxsize;
	m_cellysize = cellysize;
	// m_xsize = ncols * cellxsize;
	// m_ysize = nrows * cellysize;

	int ncells = ncols * nrows;
	m_cells = new MazeCell[ncells];
	m_balls.clear();
}

void
NosuchMaze::Draw() {
	MazeCell* cell = m_cells;
	for (int r = 0; r < m_nrows; r++) {
		for (int c = 0; c < m_ncols; c++) {
			if (cell->getFilled()) {
				m_listener.onCellDraw( *cell, MazeRowCol(r,c) );
			}
			cell++;
		}
	}
	for (std::list<MazeBall*>::iterator it = m_balls.begin(); it != m_balls.end(); it++) {
		MazeBall* b = *it;
		m_listener.onBallDraw(b,b->m_xy);
	}
}

void
NosuchMaze::moveBalls(int nclicks) {
	DEBUGPRINT1(("moveBalls start"));

	while (nclicks-- > 0) {
		for (std::list<MazeBall*>::iterator it = m_balls.begin(); it != m_balls.end();) {
			MazeBall* b = *it;
			if (b->m_alive) {
				moveBallOneClick(b);	// This may result in ball being killed
			}

			if (!b->m_alive) {
				it = m_balls.erase(it);
			}
			else {
				it++;
			}
		}
	}
	DEBUGPRINT1(("moveBalls start"));
}

#define signof(x) ((x > 0) ? 1 : ((x < 0) ? -1 : 0))

bool
NosuchMaze::isFilled(MazeRowCol rc) {
	MazeCell&  cell = getCell(rc);
	return cell.getFilled();
}

void
NosuchMaze::moveBallOneClick(MazeBall* b) {

	DEBUGPRINT(("\n================= moveBallOneClick start"));

	b->m_age += 1;
	if ( b->m_age > (b->m_born + b->m_lifetime) ) {
		killBall(b);
		return;
	}

	MazePoint oldxy = b->m_xy;
	MazePoint deltaxy = b->m_dxy;
	MazePoint newxy(oldxy.x + deltaxy.x, oldxy.y + deltaxy.y);
	MazeRowCol& oldrc = getRowCol(oldxy);
	MazeRowCol& newrc = getRowCol(newxy);
	int drow = newrc.r - oldrc.r;
	int dcol = newrc.c - oldrc.c;

	bool reversex = false;
	bool reversey = false;
	bool swapdxy = false;
	bool keepoldxy = false;

	MazePoint maxxy = xySize();
	if (newxy.x >= maxxy.x || newxy.x < 0 || newxy.y >= maxxy.y || newxy.y < 0) {
		killBall(b);
		return;
	}

	DEBUGPRINT(("oldrc = %d,%d   newrc = %d,%d  m_xy = %d,%d  m_dxy = %d,%d",oldrc.r,oldrc.c,newrc.r,newrc.c,b->m_xy.x,b->m_xy.y,b->m_dxy.x,b->m_dxy.y));

	if (drow==0 && dcol==0) {
		// It hasn't moved to a different cell
		DEBUGPRINT(("    didn't move"));
		goto getout;
	}

	// Verify important assumption here, requires that the m_dxy value
	// is never greater than the cellsize, so things can only move
	// a maximum of one cell at a time.
	NosuchAssert(drow == -1 || drow == 0 || drow == 1);
	NosuchAssert(dcol == -1 || dcol == 0 || dcol == 1);

	MazeCell& oldcell = getCell(oldrc);
	MazeCell& newcell = getCell(newrc);
	bool newfilled = newcell.getFilled();
	bool oldfilled = oldcell.getFilled();

	DEBUGPRINT(("******** oldcell=%d,%d(%d) newcell=%d,%d(%d)",
		oldrc.r, oldrc.c, oldfilled,
		newrc.r, newrc.c, newfilled));

	if (oldfilled) {
		// If our previous location was filled (i.e. is a wall),
		// it's because we bounced against it on our last move.
		// The direction was changed when it bounced, so this time
		// we just keep going
		DEBUGPRINT(("We hit a wall last time, right??"));
		goto getout;
	}


	if (drow == 0 || dcol == 0) {
		// It's headed directly left/right/up/down,
		if (newfilled) {
			// bounce it back
			if (drow == 0) {
				// Hit a vertical wall.  Look at cells above and below the oldcell
				// to see if it's hitting an above-diagonal or below-diagonal.
				// Make sure to take into account whether we're hitting
				// a wall from the left or right
				bool belowfilled = getCell(MazeRowCol(oldrc.r-1, oldrc.c)).getFilled();
				bool abovefilled = getCell(MazeRowCol(oldrc.r+1, oldrc.c)).getFilled();
				bool fromright = (dcol < 0);
				// If both are filled or not-filled, it's like a straight vertical wall 
				if ((abovefilled && belowfilled) || (!abovefilled && !belowfilled) ) {
					reversex = true;
				}
				else if (abovefilled) {
					// It's diagonal above, bounce it as if it's 45-degrees
					if (fromright) {
						swapdxy = true;
					}
					else { // fromleft
						swapdxy = true;
						reversex = reversey = true;
					}
				}
				else {
					NosuchAssert(belowfilled);
					// It's diagonal below, bounce it as if it's 45-degrees
					if (fromright) {
						swapdxy = true;
						reversex = reversey = true;
					}
					else { // fromleft
						swapdxy = true;
					}
				}
			} else { // dcol == 0
				// Hit a horizontal wall.  Look at cells left and right of the oldcell
				// to see if it's hitting an left-diagonal or right-diagonal.
				// Make sure to take into account whether we're hitting
				// a wall from above or below
				bool leftfilled = getCell(MazeRowCol(oldrc.r, oldrc.c-1)).getFilled();
				bool rightfilled = getCell(MazeRowCol(oldrc.r, oldrc.c+1)).getFilled();
				bool fromabove = (dcol < 0);

		// XXX - if this remains correct, merge these two if's
				if (leftfilled && rightfilled) {
					// If both are filled, it's like a straight horizontal wall 
					// that we're hitting head-on (from above or below)
					reversey = true;
				} else if (!leftfilled && !rightfilled) {
					// If neither is filled, it's like a straight horizontal wall 
					// that we're hitting at an angle
					reversey = true;
				}
				else if (leftfilled) {
					// It's diagonal left, bounce it as if it's 45-degrees
					if (fromabove) {
						swapdxy = true;
						reversex = reversey = true;
					}
					else { // frombelow
						swapdxy = true;
					}
				}
				else {
					NosuchAssert(rightfilled);
					// It's diagonal right, bounce it as if it's 45-degrees
					if (fromabove) {
						swapdxy = true;
					}
					else { // frombelow
						swapdxy = true;
						reversex = reversey = true;
					}
				}
			}
			// Keeping the oldxy value unchanged isn't accurate for bounces,
			// but is simple and probably accurate enough.
			keepoldxy = true;
			DEBUGPRINT(("    left/right/up/down filled!"));
		}
		goto getout;
	}

	// At this point, we know that abs(drow) and abs(dcol) are both 1,
	// i.e. the new cell can be in one of the four diagonal directions.
	// We look around that new cell to see if whether there are
	// horizontal or vertical walls, or corners.

	bool hwall;
	bool vwall;
	if (drow == 1 && dcol == 1) {
		vwall = isFilled(MazeRowCol(newrc.r - 1, newrc.c));
		hwall = isFilled(MazeRowCol(newrc.r, newrc.c - 1));
	}
	else if (drow == 1 && dcol == -1) {
		vwall = isFilled(MazeRowCol(newrc.r - 1, newrc.c));
		hwall = isFilled(MazeRowCol(newrc.r, newrc.c + 1));
	}
	else if (drow == -1 && dcol == -1) {
		vwall = isFilled(MazeRowCol(newrc.r + 1, newrc.c));
		hwall = isFilled(MazeRowCol(newrc.r, newrc.c + 1));
	}
	else if (drow == -1 && dcol == 1) {
		vwall = isFilled(MazeRowCol(newrc.r + 1, newrc.c));
		hwall = isFilled(MazeRowCol(newrc.r, newrc.c - 1));
	}

	if ( newfilled ) {
		// BOUNCE!  Figure out what we've hit
		if ( (!hwall && !vwall) || (hwall && vwall) ) {
			// outside or inside corner

// TRY THIS
			// reversex = reversey = true;
			swapdxy = true;
			keepoldxy = true;
		} else if (hwall) {
			// horizontal wall
			reversey = true;
			keepoldxy = true;
		} else { // vwall
			// vertical wall
			reversex = true;
			keepoldxy = true;
		}
	} else {
		// Check the two adjacent cells
		if (hwall && vwall) {
			// bounce it back as if there's a line between them,

// TRY THIS
			// reversex = reversey = true;
			swapdxy = true;
			keepoldxy = true;
		}
	}

getout:
	if (reversex) {
		b->m_dxy.x = -(b->m_dxy.x);
	}
	if (reversey) {
		b->m_dxy.y = -(b->m_dxy.y);
	}
	if (swapdxy) {
		int t = b->m_dxy.y;
		b->m_dxy.y = b->m_dxy.x;
		b->m_dxy.x = t;
	}

	// TRY THIS
	b->m_xy = newxy;
#if 0
	if (keepoldxy) {
		b->m_xy = oldxy;
	}
	else {
		b->m_xy = newxy;
	}
#endif

	if (reversex || reversey || swapdxy) {
		MazeRowCol rc = getRowCol(b->m_xy);
		DEBUGPRINT(("    =================================================================== "
			"postbounce m_xy=%d,%d  m_dxy=%d,%d  rc=%d,%d", b->m_xy.x, b->m_xy.y, b->m_dxy.x, b->m_dxy.y, rc.r, rc.c));
	}
}

MazeBall*
NosuchMaze::addBall(MazePoint fxy, MazePoint dxy, int born, int lifetime) {
	NosuchAssert(dxy.x <= m_cellxsize && dxy.y <= m_cellysize);
	MazeBall* b = new MazeBall( fxy, dxy, born, lifetime);
	m_balls.push_back(b);
	return b;
}

#define sign(x) (((x)>=0.0)?1:-1)

void
NosuchMaze::addLine(MazeRowCol rc0, MazeRowCol rc1) {

#if 0
	// THIS SHOULD BE THE ALGORITHM TO USE - all integer
	// It's from this page: http://tech-algorithm.com/articles/drawing-line-using-bresenham-algorithm/
	public void line(int x, int y, int x2, int y2, int color) {
		int w = x2 - x;
		int h = y2 - y;
		int dx1 = 0, dy1 = 0, dx2 = 0, dy2 = 0;
		if (w<0) dx1 = -1; else if (w>0) dx1 = 1;
		if (h<0) dy1 = -1; else if (h>0) dy1 = 1;
		if (w<0) dx2 = -1; else if (w>0) dx2 = 1;
		int longest = Math.abs(w);
		int shortest = Math.abs(h);
		if (!(longest>shortest)) {
			longest = Math.abs(h);
			shortest = Math.abs(w);
			if (h<0) dy2 = -1; else if (h>0) dy2 = 1;
			dx2 = 0;
		}
		int numerator = longest >> 1;
		for (int i = 0; i <= longest; i++) {
			putpixel(x, y, color);
			numerator += shortest;
			if (!(numerator<longest)) {
				numerator -= longest;
				x += dx1;
				y += dy1;
			}
			else {
				x += dx2;
				y += dy2;
			}
		}
	}
#endif
	int x0 = rc0.c;
	int y0 = rc0.r;
	int x1 = rc1.c;
	int y1 = rc1.r;

	double deltax = x1 - x0;
	double deltay = y1 - y0;
	int signy = sign(deltay);
	int signx = sign(deltax);
	double error = 0.0;
	if (deltax == 0) {
		// vertical line
		for (int y = y0; y != y1; y += signy) {
			getCell(MazeRowCol(y,x0)).setFilled(true);
		}
		return;
	}
	double deltaerr = abs(deltay / deltax); // Assume deltax != 0 (line is not vertical),
	// note that this division needs to be done in a way that preserves the fractional part
	int y = y0;
	for (int x = x0; x != x1; x += signx) {
		getCell(MazeRowCol(y, x)).setFilled(true);
		error = error + deltaerr;
		while (error >= 0.5) {
			getCell(MazeRowCol(y, x)).setFilled(true);
			y = y + sign(y1 - y0);
			error = error - 1.0;
		}
	}
}

static int round(double number)
{
    double d = number < 0.0f ? ceil(number - 0.5f) : floor(number + 0.5f);
	return (int)d;
}

#if 0
static MazeRowCol
NosuchMaze::xy2rc(double x, double y) {
	int c1 = round( (x * m_ncols) - 0.5);
	int r1 = round((y * m_nrows) - 0.5);
	return MazeRowCol(r1, c1);
}
#endif

#if 0
	c = integer(x*$.cols / ($.x1 - $.x0))
	my = m["y"]
	y = my - $.y0
	r = integer(y*$.rows / ($.y1 - $.y0))
	# print("xy2rc x0y0x1y1=", $.x0, $.y0, $.x1, $.y1, "   m=", m, "  x1=", $.x1, " x0=", $.x0, "  initial c=", c)
	if (c >= $.cols)
		c = $.cols - 1
	else if (c < 0)
	c = 0
	if (r >= $.rows)
		r = $.rows - 1
	else if (r < 0)
	r = 0
	dx = ($.x1 - $.x0) / $.cols
	dy = ($.y1 - $.y0) / $.rows
	relx = mx - ($.x0 + c * dx)
	rely = my - ($.y0 + r * dy)
	smallerdx = dx - 2
	smallerdy = dy - 2
	relx = relx / float(smallerdx)
	rely = rely / float(smallerdy)
	if (relx < 0.0) relx = 0.0;
	if (rely < 0.0) rely = 0.0
	if (relx > 1.0) relx = 1.0
	if (rely > 1.0) rely = 1.0
}
#endif