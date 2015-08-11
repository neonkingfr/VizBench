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
	for (std::list<MazeBall*>::iterator it = m_balls.begin(); it != m_balls.end(); ) {
		MazeBall* b = *it;
		if (b->m_alive) {
			moveBall(b,nclicks);	// This may result in ball being killed
		}

		if (!b->m_alive) {
			it = m_balls.erase(it);
		}
		else {
			it++;
		}
	}
}

void
NosuchMaze::moveBall(MazeBall* b, int nclicks) {
	// DEBUGPRINT(("moveBall nclicks=%d", nclicks));
	while (nclicks-- > 0) {
		moveBallOneClick(b);
	}
}

#define signof(x) ((x > 0) ? 1 : ((x < 0) ? -1 : 0))

void
NosuchMaze::moveBallOneClick(MazeBall* b) {

	b->m_age += 1;
	if ( b->m_age > (b->m_born + b->m_lifetime) ) {
		killBall(b);
		return;
	}
	MazePoint oldxy = b->m_xy;
	MazePoint deltaxy = b->m_dxy;
	MazePoint newxy(oldxy.x + deltaxy.x, oldxy.y + deltaxy.y);

	MazePoint maxxy = xySize();

	if (newxy.x >= maxxy.x || newxy.x < 0 || newxy.y >= maxxy.y || newxy.y < 0) {
		killBall(b);
		return;
	}

	MazeRowCol& oldrc = getRowCol(b->m_xy);
	MazeRowCol& newrc = getRowCol(newxy);

	int drow = newrc.r - oldrc.r;
	int dcol = newrc.c - oldrc.c;

	if (drow==0 && dcol==0) {
		// It hasn't moved to a different cell
		return;
	}

	if (abs(drow) > 1 || abs(dcol) > 1) {
		DEBUGPRINT(("HEY!!  drow or dcol > 1?! drow=%d dcol=%d",drow,dcol));
		return;
	}

	MazeCell& newcell = getCell(newrc);
	MazeCell& oldcell = getCell(oldrc);

	if (newcell.getFilled()) {
		// We've moved into a wall, bounce the ball
		if (drow == 0) {
			// Bounce off a vertical wall
			b->m_dxy.x = -(b->m_dxy.x);
		}
		else if (dcol == 0) {
			// Bounce off a horizontal wall
			b->m_dxy.y = -(b->m_dxy.y);
		}
		else {
			// At this point, drow and dcol are both non-zero, either 1 or -1,
			// meaning that newcell is at a diagonal from the oldcell.
			MazeCell& diag1 = getCell(MazeRowCol(oldrc.r+drow,oldrc.c));
			MazeCell& diag2 = getCell(MazeRowCol(oldrc.r,oldrc.c+dcol));
			bool filled1 = diag1.getFilled();
			bool filled2 = diag2.getFilled();
			if ((filled1 && filled2) || (!filled1 && !filled2) ) {
				// We've hit an inside or outside corner, both directions are reversed
				b->m_dxy.x = -(b->m_dxy.x);
				b->m_dxy.y = -(b->m_dxy.y);
			}
			else if (filled1) {
				b->m_dxy.y = -(b->m_dxy.y);
			}
			else { // filled2 is true
				b->m_dxy.x = -(b->m_dxy.x);
			}
		}
	}
	b->m_xy = newxy;

#if 0
	# scale nfx, y to size of grid on screen
	nxy = $.fxy2xy(nfx, nfy)
		rc = $.grid.rxy2rc(nxy, 0)
		newrow = rc["row"]
		newcol = rc["col"]
		# print("r,c=", r, c, "  newrow,col=", rc)
		bouncetype = ""
		if ((r != newrow || c != newcol)) {
			if (r != newrow && c != newcol)
				iscorner = 1
			else
			iscorner = 0

			bouncedy = 0
			bouncedx = 0
			redrawbottomr = -2
			redrawbottomc = -2
			redrawrightr = -2
			redrawrightc = -2

			# Special cases for corners
			if (iscorner) {
				# print("ISCORNER");
				if (newrow < r && newcol < c) {
					# upper - left
					horiz = ($.hastop(r, c) && $.hastop(r, c - 1) && !$.hasright(r, c - 1))
						vert = ($.hasright(r, c - 1) && $.hasright(r - 1, c - 1) && !$.hastop(r, c))
						corn1 = ($.hastop(r, c) && $.hasright(r, c - 1))
						corn2 = ($.hasright(r - 1, c - 1) && $.hastop(r, c - 1))
						if (horiz) {
							bouncetype = "top"
						}
						else if (vert) {
							bouncetype = "left"
						}
						else if (corn1 || corn2) {
							$.hittop(r, c, cl)
								b->m_dfy = -b->m_dfy
								b->m_dfx = -b->m_dfx
								bouncedy = 1
								bouncedx = 1
								redrawbottomr = r - 1
								redrawbottomc = c
								redrawrightr = r
								redrawrightc = c - 1
						}
				}
				else if (newrow > r && newcol < c) {
					# lower - left
					horiz = ($.hastop(r + 1, c) && $.hastop(r + 1, c - 1) && !$.hasright(r, c - 1))
						vert = ($.hasright(r, c - 1) && $.hasright(r + 1, c - 1) && !$.hastop(r + 1, c))
						corn1 = ($.hastop(r + 1, c) && $.hasright(r, c - 1))
						corn2 = ($.hasright(r + 1, c - 1) && $.hastop(r + 1, c - 1))
						if (horiz) {
							bouncetype = "bottom"
						}
						else if (vert) {
							bouncetype = "left"
						}
						else if (corn1 || corn2) {
							$.hitbottom(r, c, cl)
								b->m_dfy = -b->m_dfy
								b->m_dfx = -b->m_dfx
								bouncedy = 1
								bouncedx = 1
								redrawbottomr = r
								redrawbottomc = c
								redrawrightr = r
								redrawrightc = c - 1
						}
				}
				else if (newrow < r && newcol > c) {
					# upper - right
					horiz = ($.hastop(r, c) && $.hastop(r, c + 1) && !$.hasright(r, c))
						vert = ($.hasright(r, c) && $.hasright(r - 1, c) && !$.hastop(r, c))
						corn1 = ($.hastop(r, c) && $.hasright(r, c))
						corn2 = ($.hasright(r - 1, c) && $.hastop(r, c + 1))
						if (horiz) {
							bouncetype = "top"
						}
						else if (vert) {
							bouncetype = "right"
						}
						else if (corn1 || corn2) {
							$.hittop(r, c, cl)
								b->m_dfy = -b->m_dfy
								b->m_dfx = -b->m_dfx
								bouncedy = 1
								bouncedx = 1
								redrawbottomr = r - 1
								redrawbottomc = c
								redrawrightr = r
								redrawrightc = c
						}
				}
				else if (newrow > r && newcol > c) {
					# lower - right
					horiz = ($.hastop(r + 1, c) && $.hastop(r + 1, c + 1) && !$.hasright(r, c))
						vert = ($.hasright(r, c) && $.hasright(r + 1, c) && !$.hastop(r + 1, c))
						corn1 = ($.hastop(r + 1, c) && $.hasright(r, c))
						corn2 = ($.hasright(r + 1, c) && $.hastop(r + 1, c + 1))
						if (horiz) {
							bouncetype = "bottom"
						}
						else if (vert) {
							bouncetype = "right"
						}
						else if (corn1 || corn2) {
							$.hittop(r, c, cl)
								b->m_dfy = -b->m_dfy
								b->m_dfx = -b->m_dfx
								bouncedy = 1
								bouncedx = 1
								redrawbottomr = r
								redrawbottomc = c
								redrawrightr = r
								redrawrightc = c
						}
				}
			}

			if (bouncetype == "") {
				# NORMAL bounce, off one side
				if (newrow < r) {
					# potential bounce off top
					if ($.hastop(r, c)) {
						bouncetype = "top"
					}
				}
				else if (newrow > r) {
					# potential bounce off bottom
					if ($.hasbottom(r, c)) {
						bouncetype = "bottom"
					}
				}
				else if (newcol < c) {
					# potential bounce off left
					if ($.hasleft(r, c)) {
						bouncetype = "left"
					}
				}
				else if (newcol > c) {
					# potential bounce off right
					if ($.hasright(r, c)) {
						bouncetype = "right"
					}
				}
			}

			if (bouncetype == "top") {
				$.hittop(r, c, cl)
					b->m_dfy = -b->m_dfy
					bouncedy = 1
					redrawbottomr = r - 1
					redrawbottomc = c
			}
			else if (bouncetype == "bottom") {
				$.hitbottom(r, c, cl)
					b->m_dfy = -b->m_dfy
					bouncedy = 1
					redrawbottomr = r
					redrawbottomc = c
			}
			else if (bouncetype == "left") {
				$.hitleft(r, c, cl)
					b->m_dfx = -b->m_dfx
					bouncedx = 1
					redrawrightr = r
					redrawrightc = c - 1
			}
			else if (bouncetype == "right") {
				$.hitright(r, c, cl)
					b->m_dfx = -b->m_dfx
					bouncedx = 1
					redrawrightr = r
					redrawrightc = c
			}


			if (bouncedx && bouncedy) {
				if (bouncedx)
					nfx = b->m_fx
					if (bouncedy)
						nfy = b->m_fy
			}
			else {
				if (bouncedx)
					nfx = b->m_fx
					if (bouncedy)
						nfy = b->m_fy;
			}

			nxy = $.fxy2xy(nfx, nfy)
				# print("BOUNCE, b=", b, "  nfxy=", nfx, nfy, "  nxy=", nxy["x"], nxy["y"])
				rc = $.grid.rxy2rc(nxy, 0)
				# Sanity check - if we bounce, we should
				# be in the same cell.
				if (!bouncedy) {
					if (newrow < 0) {
						print("!bouncedy, newrow<0")
							newcol = 0
					}
					b["row"] = newrow
				}
			if (!bouncedx) {
				if (newcol < 0) {
					print("!bouncedx, newcol<0=", newcol)
						newcol = 0
				}
				b["col"] = newcol
			}
			if (redrawrightr != -2) {
				task $.redrawright(redrawrightr,
					redrawrightc)
			}
			if (redrawbottomr != -2) {
				task $.redrawbottom(redrawbottomr,
					redrawbottomc)
			}
			# print("b is now ", b)
		}
	$.drawball(b, CLEAR)
		$.updateball(b, nfx, nfy, nxy["x"], nxy["y"])
		# print("MOVED ball b=", b)
		colorset($.colorindex[cl])
		if ($.debugmove>0){ print("drawing b=", b); }
	$.drawball(b, STORE)
#endif
}

MazeBall*
NosuchMaze::addBall(MazePoint fxy, MazePoint dxy, int born, int lifetime) {
	MazeBall* b = new MazeBall( fxy, dxy, born, lifetime);
	m_balls.push_back(b);
	return b;
}

void
NosuchMaze::addLine(MazeRowCol rc0, MazeRowCol rc1) {
	getCell(rc0).setFilled(true);
	int dir_c = ( rc1.c > rc0.c ) ? 1 : -1;
	int dir_r = ( rc1.r > rc0.r ) ? 1 : -1;
	int c = rc0.c;
	int r = rc0.r;
	while (c != rc1.c || r != rc1.r) {
		int dc = rc1.c - c;
		int dr = rc1.r - r;
		if ( abs(dc) >= abs(dr) ) {
			c += dir_c;
		}
		if ( abs(dr)>= abs(dc) ) {
			r += dir_r;
		}
		getCell(MazeRowCol(r,c)).setFilled(true);
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