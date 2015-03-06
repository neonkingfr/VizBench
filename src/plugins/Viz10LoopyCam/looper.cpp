#define _CRT_RAND_S

#include <Windows.h>
#include <opencv/cv.h>

#include <iostream>
#include <sstream>
#include <string>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <time.h>

#include "NosuchDebug.h"
#include "NosuchOsc.h"
#include "videoutil.h"
#include "looper.h"

int CV_interp = CV_INTER_NN;  // or CV_INTER_LINEAR

Looper::Looper(int w, int h) {
	_flipset = false;
    ffWidth = w;
    ffHeight = h;
    ffSize = cvSize(w,h);
	DEBUGPRINT(("Looper::Looper CONSTRUCTOR!"));
    m_frameImage = createImage24ofSize(ffSize);

    for ( int n=0; n<MAX_LOOPS; n++ ) {
        VFrameLoop* fl = &_loop[n];
        fl->clearme();
        _recording[n] = 0;
        _playing[n] = 0;
    }
	// DEBUGPRINT(("Looper::Looper B"));
    srand((unsigned)time(NULL));
    _enableXOR = 1;
    _border = 0;
	_recborder = 0;
	_blackout = 0;
    _currentLoop = 0;
    _autoNext = 1;
    // _numPlaying = 1;
    _playing[0] = 2;
    _smooth = 1;
    _moveamount = 20;

    CvSize sz = ffSize;
    DEBUGPRINT(("Looper frame size = %d,%d",sz.width,sz.height));
    _tmpImage = createImage24ofSize(sz);
	DEBUGPRINT(("_tmpImage=%d", _tmpImage));

    for ( int n=0; n<MAX_LOOPS; n++ ) {
        // _sz[n] = cvSize(0,0);
        // _targetsz[n] = cvSize(0,0);
        _sz[n] = cvSize(w,h);
        _targetsz[n] = cvSize(w,h);
        _sizeFactor[n] = 1.0;
        _szImage[n] = NULL;
		_pos[n] = cvPoint(0,0);
		_targetpos[n] = cvPoint(0,0);

    }
	// DEBUGPRINT(("Looper::Looper C"));
    // for ( int n=0; n<MAX_LOOPS; n++ ) {
    //   _randomizeSize(n,0,1.0);
    // }
	_playing[0] = 2;
    _fullDisplay();
	// DEBUGPRINT(("Looper::Looper D"));
}

Looper::~Looper() {
	DEBUGPRINT(("Looper::Looper DESTRUCTOR, NO FREES?!"));
	cvReleaseImage(&m_frameImage);
}

#if 0
void Looper::debugsize() {
	if (_pos[0].x != 0) {
		DEBUGPRINT(("Found it too!"));
	}
	if (_sz[0].width == 40) {
		DEBUGPRINT(("Found it!"));
	}
	if (_targetpos[0].x != 0) {
		DEBUGPRINT(("Found it again!"));
	}
	if (_targetsz[0].width != 640) {
		DEBUGPRINT(("Found it again!"));
	}
	for (int n = 0; n < MAX_LOOPS; n++) {
		DEBUGPRINT(("DEBUGSZ n=%d w=%d h=%d  targetpos[0].x=%d", n, _sz[n].width, _sz[n].height, _targetpos[0].x));
	}
}
#endif

void
Looper::_quadrantDisplay()
{
    CvSize sz = ffSize;
	setwindows(4);
	int q = 0;
	for ( int n=0; n<MAX_LOOPS; n++ ) {
		if ( is_showing(n) ) {
			switch(q) {
			case 0:
				_changePosSize(n,cvPoint(sz.width/4,sz.height/4),cvSize(sz.width/2,sz.height/2));
				break;
			case 1:
				_changePosSize(n,cvPoint(3*sz.width/4,sz.height/4),cvSize(sz.width/2,sz.height/2));
				break;
			case 2:    
				_changePosSize(n,cvPoint(3*sz.width/4,3*sz.height/4),cvSize(sz.width/2,sz.height/2));
				break;
			case 3:   
				_changePosSize(n,cvPoint(sz.width/4,3*sz.height/4),cvSize(sz.width/2,sz.height/2));
			}
			q++;
		}
	}

#if 0
	int n;
	int nshown = 0;
	DEBUGPRINT(("QUAD start num_showing=%d\n",num_showing()));
    for ( n=0; n<MAX_LOOPS; n++ ) {
		if ( is_showing(n) ) {
			nshown++;
		} else if ( nshown < 4 ) {
            _playing[n] = 2;
			nshown++;
		} else {
			_playing[n] = 0;
		}
    }
	DEBUGPRINT(("QUAD end num_showing=%d\n",num_showing()));
#endif

}

void
Looper::_allLive(int onoff)
{
	int v = ( onoff ? 2 : 1 );
	for ( int n=0; n<MAX_LOOPS; n++ ) {
		if ( is_showing(n) ) {
			_playing[n] = 2;
		}
	}
}

void
Looper::_fullDisplay()
{
    CvSize sz = ffSize;
	// Make each loop other than the first a different size
    _changePosSize(0,cvPoint(0,0),cvSize(sz.width,sz.height),FALSE);
    for ( int n=1; n<MAX_LOOPS; n++ ) {
		int offset = 4;
        _changePosSize(n,cvPoint(sz.width/2,sz.height/2),cvSize(sz.width-offset*n,sz.height-offset*n),FALSE);
    }
    for ( int n=0; n<MAX_LOOPS; n++ ) {
        _playing[n] = 0;
    }
    _playing[0] = 2;
}

void
Looper::_randomizeSize(int n, int playing, double aspect)
{
    unsigned int    r1,r2,r3,r4;
    CvSize sz = ffSize;

    if ( rand_s(&r1)!=0 || rand_s(&r2)!=0 || rand_s(&r3)!=0 || rand_s(&r4)!=0 ) {
        DEBUGPRINT(("HEY!! rand_s failed!?"));
        r1 = 0;
        r2 = 0;
        r3 = UINT_MAX;
        r4 = UINT_MAX;
    }
    int minwh = 100;  // was 50
	int xyborder = 100;   // was 50
    int dx = (sz.width - 2 * xyborder);
    int dy = (sz.height - 2 * xyborder);

    int x = xyborder + (int)(dx * (double(r1) / double(UINT_MAX)));
    int y = xyborder + (int)(dy * (double(r2) / double(UINT_MAX)));
    int w;
    int h;
    if ( aspect == 0.0 ) {
        w = minwh + (int)((sz.width-minwh) * (double(r3) / double(UINT_MAX)));
        h = minwh + (int)((sz.height-minwh) * (double(r4) / double(UINT_MAX)));
    } else {
		if ( aspect < 0.0 ) {
			aspect = double(sz.height) / sz.width;
		}
        if ( (double(r3) / double(UINT_MAX)) > 0.5 ) {
            w = minwh + (int)((sz.width-minwh) * (double(r4) / double(UINT_MAX)));
            h = int(w * aspect);
        } else {
            h = minwh + (int)((sz.height-minwh) * (double(r4) / double(UINT_MAX)));
            w = int(h * aspect);
        }
    }

    _changePosSize(n,cvPoint(x,y),cvSize(w,h));
    _playing[n] = playing;
}

void
Looper::_changePosSize(int n, CvPoint newpt, CvSize newsz, bool boundit)
{
    CvSize sz = ffSize;
    int x = newpt.x;
    int y = newpt.y;
    int w = newsz.width;
    int h = newsz.height;

    // DEBUGPRINT(("Looper changePosSize for n=%d pos=%d,%d  size=%d,%d",n,x,y,w,h));

	// DEBUGPRINT(("changepos A"));
	if ( boundit ) {
		// DEBUGPRINT(("Before, aspect ratio is %lf",(double)w/h));
		double aspectbefore = (double)w/h;
		int wbefore = w;
		_boundPosSize(&x,&y,&w,&h);
		// DEBUGPRINT(("After 1, aspect ratio is %lf  pos=%d,%d size=%d,%d",(double)w/h,x,y,w,h));
		if ( ((double)w/h) < aspectbefore ) {
			h = (int)(w/aspectbefore);
		} else {
			w = (int)(h*aspectbefore);
		}
		// DEBUGPRINT(("After 2, aspect ratio is %lf  pos=%d,%d size=%d,%d",(double)w/h,x,y,w,h));
	} else {
		// DEBUGPRINT(("NOT BOUNDING resize xy=%d,%d wh=%d,%d\n",x,y,w,h));
	}

    _targetpos[n] = cvPoint(x,y);
    _targetsz[n] = cvSize(w,h);
    if ( _smooth == 0 ) {
        // DEBUGPRINT(("smooth is 0, jumping to %d,%d",_pos[n].x,_pos[n].y));
        _pos[n] = cvPoint(x,y);
        _sz[n] = cvSize(w,h);
		DEBUGPRINT(("smooth sz=%d,%d", w, h));
    }

	// DEBUGPRINT(("changepos C"));
    _releaseAndAllocate(n);
	// DEBUGPRINT(("changepos D"));
}

void
Looper::_boundPosSize(int *x, int *y, int *w, int *h)
{
    CvSize sz = ffSize;
	int margin = 0;
	int xymargin = 0;
	int origx = *x;
	int origy = *y;
	int origw = *w;
	int origh = *h;

    // bound the values
    if ( *w < margin ) *w = margin;
    if ( *h < margin ) *h = margin;
    if ( *w > (sz.width-margin) ) *w = sz.width - margin;
    if ( *h > (sz.height-margin) ) *h = sz.height - margin;

    if ( *x < xymargin ) *x = xymargin;
    if ( *y < xymargin ) *y = xymargin;
    if ( *x > (sz.width-xymargin) ) *x = sz.width - xymargin;
    if ( *y > (sz.height-xymargin) ) *y = sz.height - xymargin;

    // make sure it's within the display
    if ( (*x-*w/2) < 0 ) *w = *x*2;
    if ( (*y-*h/2) < 0 ) *h = *y*2;
    if ( (*x+*w/2) > sz.width ) *w = (sz.width-*x-1)*2;
    if ( (*y+*h/2) > sz.height) *h = (sz.height-*y-1)*2;
	if ( *w==0 || *h ==0 ) {
		DEBUGPRINT(("BOUNDIT xy=%d,%d  wh=%d,%d   origxy=%d,%d origwh=%d,%d\n",*x,*y,*w,*h,origx,origy,origw,origh));
	}
}

#define signof(x) (x<0?-1:1)

void
Looper::_towardtarget(int n, int *nfull)
{
    // int maxamount = 3;
    int d;
    int changed = FALSE;

    if ( _pos[n].x != _targetpos[n].x ) {
        d = (_targetpos[n].x - _pos[n].x);
        if ( abs(d) > _moveamount )
            d = signof(d) * _moveamount;
        // DEBUGPRINT(("towardtarget, adjusting x, d=%d  targetx is %d  x was %d  now %d",d,_targetpos[n].x,_pos[n].x,_pos[n].x + d));
        _pos[n].x += d;
        changed = TRUE;
    }
    if ( _pos[n].y != _targetpos[n].y ) {
        d = (_targetpos[n].y - _pos[n].y);
        if ( abs(d) > _moveamount )
            d = signof(d) * _moveamount;
        _pos[n].y += d;
        changed = TRUE;
    }

	if (_sz[n].width < 20) {
		DEBUGPRINT(("HEY! width < 20?"));
	}

    if ( _sz[n].width != _targetsz[n].width ) {
        d = (_targetsz[n].width - _sz[n].width);
        if ( abs(d) > _moveamount )
            d = signof(d) * _moveamount;
        _sz[n].width += d;
        changed = TRUE;
    }
    if ( _sz[n].height != _targetsz[n].height ) {
        d = (_targetsz[n].height - _sz[n].height);
        if ( abs(d) > _moveamount )
            d = signof(d) * _moveamount;
        _sz[n].height += d;
        changed = TRUE;
    }
	if ( changed ) {
		DEBUGPRINT(("changed n=%d _sz.width=%d height=%d",n,_sz[n].width,_sz[n].height));
        _boundPosSize(&(_pos[n].x),&(_pos[n].y),&(_sz[n].width),&(_sz[n].height));
	}
	// If there's more than one full-screen window, make them slightly different sizes
	if ( _sz[n].height == ffSize.height && _sz[n].width == ffSize.width ) {
			// DEBUGPRINT(("n=%d nfull=%d\n",n,*nfull));
			if ( *nfull > 0 ) {
				DEBUGPRINT((" reducing n=%d by %d, was wh=%d,%d\n",n,*nfull,_sz[n].width,_sz[n].height));
#if 0
				_sz[n].height -= *nfull;
				_sz[n].width -= *nfull;
				changed = TRUE;
#endif
				DEBUGPRINT((" reducing n=%d by %d, now wh=%d,%d\n",n,*nfull,_sz[n].width,_sz[n].height));
			}
			(*nfull)++;
			// DEBUGPRINT(("Incremented n=%d nfull=%d nfullptr=%ld\n",n,*nfull,(long)nfull));
	}
    if ( changed ) {
        _releaseAndAllocate(n);
    }
}

void
Looper::_releaseAndAllocate(int n)
{
	// DEBUGPRINT(("Pre release"));
    if ( _sz[n].width != 0 && _sz[n].height != 0 && _szImage[n] != NULL ) {
        // DEBUGPRINT(("ABOUT TO FREE IMAGE of n=%d!",n));
		IplImage* i = _szImage[n];
		if (i == NULL) {
			DEBUGPRINT(("HEY! _szImage[ %d ] = NULL?", n));
		}
        cvReleaseImage(&_szImage[n]);
    }

	// DEBUGPRINT(("Post release n=%d sz=%d,%d",n,_sz[n].width,_sz[n].height));
    // DEBUGPRINT(("Looper changing size/pos for n=%d pos=%d,%d  size=%d,%d",n,_pos[n].x,_pos[n].y,_sz[n].width,_sz[n].height));
    _szImage[n] = createImage24ofSize(_sz[n]);
	if (_szImage[n] == NULL) {
		DEBUGPRINT(("HEY!!!! unable to createImage24ofSize!?"));
	}
	// DEBUGPRINT(("created _szImage = %d,%d\n",_szImage[n]->width,_szImage[n]->height));
	// DEBUGPRINT(("Post create"));
}

IplImage*
Looper::createImage24ofSize(CvSize sz)
{
    // DEBUGPRINT(("createImage24ofSize called! sz=%d %d\n",sz.width,sz.height));
    if ( sz.width > 5000 || sz.width > 5000 ) {
        DEBUGPRINT(("Hey, createImage24ofSize detects bad size!?"));
    }
    return cvCreateImage( sz, IPL_DEPTH_8U, 3 );
}

void
Looper::randompositions(double aspect) {
    for ( int n=0; n<MAX_LOOPS; n++ ) {
        _randomizeSize(n,_playing[n],aspect);
    }
}

void
Looper::randomposition(double aspect) {
    _randomizeSize(_currentLoop,_playing[_currentLoop],aspect);
}

void
Looper::morewindows() {
	DEBUGPRINT(("MOREWINDOWS, num_showing = %d\n",num_showing()));
    if ( num_showing() < MAX_LOOPS ) {
		for ( int n=0; n<MAX_LOOPS; n++ ) {
			if ( ! is_showing(n) ) {
				_playing[n] = 2;
				break;
			}
		}
        // printf("numLoops = %d\n",_numLoops);
    }
	DEBUGPRINT(("MOREWINDOWS end, num_showing = %d\n",num_showing()));

}
void
Looper::_setautoNext(int v) {
    _autoNext = v;
}
void
Looper::_setsmooth(int v) {
    _smooth = v;
}
void
Looper::_setplayfactor(int loopnum, double x) {
    if ( validLoopnum(loopnum) ) {
        _loop[loopnum].framerate *= x;
    }
}
void
Looper::_resetplayfactor(int loopnum) {
    if ( validLoopnum(loopnum) ) {
        _loop[loopnum].framerate = 1.0;
    }
}

void
Looper::lesswindows() {
	if ( num_showing() > 1 ) {
		for ( int n=0; n<MAX_LOOPS; n++ ) {
			if ( is_showing(n) ) {
				_playing[n] = 0;
				break;
			}
		}
	}
    // printf("numLoops = %d\n",_numLoops);
}

void
Looper::setwindows(int n) {
	DEBUGPRINT(("SETWINDOWS n=%d\n",n));
	while ( n > num_showing() && num_showing() < MAX_LOOPS ) {
		morewindows();
	}
	while ( n < num_showing() && num_showing() > 1 ) {
		lesswindows();
	}
}

void
Looper::togglesmooth() {
    _smooth = 1 - _smooth;
}

void
Looper::setsmooth(int v) {
	_smooth = (v)?1:0;
}

void
Looper::togglexor() {
    _enableXOR = 1 - _enableXOR;
}

void
Looper::toggleautonext() {
    _autoNext = 1 - _autoNext;
}

void
Looper::toggleborder() {
    _border = 1 - _border;
}

void
Looper::setinterp(int interp)
{
	if ( interp ) {
		CV_interp = CV_INTER_LINEAR;
	} else {
		CV_interp = CV_INTER_NN; // This produces some artifacts, but increases the frame rate from 20 to 30
	}
}

void
Looper::_nextLoop()
{
	int limit = MAX_LOOPS;
	DEBUGPRINT(("NEXTLOOP _currentloop=%d\n",_currentLoop));
	for ( int n=0; n<limit; n++ ) {
		_currentLoop++;	
		if ( _currentLoop >= MAX_LOOPS )
			_currentLoop = 0;
		if ( is_showing(_currentLoop) ) {
			DEBUGPRINT(("    currentLoop is now %d",_currentLoop));
			return;
		}
	}
	DEBUGPRINT(("HEY, nextLoop didn't work?\n"));
	_currentLoop = 0;

}

void
Looper::setRecord(int onoff)
{
    if ( _autoNext != 0 && onoff != 0 ) {
        _nextLoop();
    }
    int loopnum = _currentLoop;
    VFrameLoop* fl = &_loop[loopnum];
    DEBUGPRINT(("_setRecord, loopnum=%d onoff=%d",loopnum,onoff));
    if ( onoff == 0 ) {
        _recording[loopnum] = 0;
        // Turn recording off, start it playing
        DEBUGPRINT(("LOOP recording off, nframes=%d",fl->nframes));
        if ( _playing[loopnum] == 1 ) {
            DEBUGPRINT(("Recording off, advancing to Start"));
            fl->advanceToStart();
        }
    } else {
        for ( int n=0; n<MAX_LOOPS; n++ ) {
            _recording[n] = 0;
        }
        _recording[loopnum] = 1 * onoff;
        // Turn recording on, clear existing loop
        fl->freeandclear();
        // Force playing on?
        _playing[loopnum] = 1;
		DEBUGPRINT(("Playing of loopnum=%d is on\n",loopnum));
    }
}

void
Looper::setBlackout(int onoff)
{
	_blackout = onoff;
}

void
Looper::_setRecordOverlay(int onoff)
{
    int loopnum = _currentLoop;
    VFrameLoop* fl = &_loop[loopnum];
    if ( ! is_showing(loopnum) ) {
        DEBUGPRINT(("Hey, loopnum=%d isn't playing, overlay not allowed"));
        _recording[loopnum] = 0;
    } else {
        // DEBUGPRINT(("_setRecordOverlay, loopnum=%d onoff=%d",loopnum,onoff));
        for ( int n=0; n<MAX_LOOPS; n++ ) {
            _recording[n] = 0;
        }
        _recording[loopnum] = 2 * onoff;
    }

}


int
Looper::validLoopnum(int loopnum)
{
    if ( loopnum < 0 || loopnum >= MAX_LOOPS ) {
        DEBUGPRINT(("Looper: got bad loopnum (%d) ?",loopnum));
        return 0;
    }
    return 1;
}

void
Looper::setPlay(int loopnum, int onofflive)
{
    DEBUGPRINT(("_setPlay called, loop=%d onoff=%d",loopnum,onofflive));
    if ( onofflive == 1 ) {
        VFrameLoop* fl = &_loop[loopnum];
        DEBUGPRINT(("setPlay, fl=%ld",(long)fl));
        fl->advanceToStart();
    }
    _playing[loopnum] = onofflive;
}

void
Looper::_restart(int loopnum)
{
    VFrameLoop* fl = NULL;

    fl = &_loop[loopnum];
    if ( fl->nframes == 0 ) {
        DEBUGPRINT(("No frames recorded in loop %d",loopnum));
        return;
    }
    fl->advanceToStart();
}

void
Looper::_freeze(int loopnum, int freeze)
{
	VFrameLoop* fl = NULL;

	fl = &_loop[loopnum];
	if ( fl->nframes == 0 ) {
		return;
	}
	fl->setfreeze(freeze);
}

void
Looper::_restartrandom(int loopnum)
{
	VFrameLoop* fl = NULL;

	fl = &_loop[loopnum];
	if ( fl->nframes == 0 ) {
		return;
	}
	unsigned int r;
	if ( rand_s(&r) != 0 ) {
			r = 0;
	}
	float pos = (float)r / (float)MAXINT;
	VFrame* f = fl->frameOfPos( pos);
	fl->setNextToShow(f);
}

void
Looper::_restartsave(int loopnum)
{
	VFrameLoop* fl = &_loop[loopnum];
	if ( fl != NULL && fl->nframes > 0 ) {
		VFrame* f = fl->nexttoshow;
		if ( f != NULL ) {
			fl->savedpos = (float)(f->framenum) / (float)fl->nframes;
		}
	}
}

void
Looper::_restartrestore(int loopnum)
{
	VFrameLoop* fl = &_loop[loopnum];
	if ( fl->nframes == 0 ) {
		return;
	}
	VFrame* f = fl->frameOfPos( fl->savedpos);
	fl->setNextToShow(f);
}


void
Looper::_truncate(int loopnum)
{
    VFrameLoop* fl = NULL;

    fl = &_loop[loopnum];
    VFrame *next = fl->nexttoshow;
    if ( next == NULL ) {
        DEBUGPRINT(("No frames recorded in loop %d",loopnum));
        return;
    }
    if ( next->framenum < 10 ) {
        DEBUGPRINT(("Can't truncate that small (framenum=%d)",next->framenum));
        return;
    }
    fl->end = next;
    fl->advanceToStart();
}


void
Looper::_copy24bit(VideoPixel24bit* dest, VideoPixel24bit* src) {
    for (int x = 0; x < ffWidth; x++) {
        for (int y = 0; y < ffHeight; y++) {
            dest->red = (BYTE) (src->red);
            dest->green = (BYTE) (src->green);
            dest->blue = (BYTE) (src->blue);
            dest++;
            src++;
        }
    }
}

void
imageborder(IplImage *img, CvSize sz, CvScalar color)
{
    int linetype = 8;
    int w = sz.width;
    int h = sz.height;
    int thick = 1;

	int z = 0;
    cvLine(img,cvPoint(z,z),cvPoint(w-1,z),color,thick,linetype,0);
    cvLine(img,cvPoint(w-1,z),cvPoint(w-1,h-1),color,thick,linetype,0);
    cvLine(img,cvPoint(w-1,h-1),cvPoint(z,h-1),color,thick,linetype,0);
    cvLine(img,cvPoint(z,h-1),cvPoint(z,z),color,thick,linetype,0);
}

void
imageborder_record(IplImage *img, CvSize sz)
{
    imageborder(img,sz,CV_RGB(255,0,0));
}

void
imageborder_normal(IplImage *img, CvSize sz)
{
    imageborder(img,sz,CV_RGB(100,100,100));
}

void
Looper::processFrame24Bit(IplImage* fi)
{
	// DEBUGPRINT(("process start\n"));
    VFrameLoop* fl = NULL;

	// DEBUGPRINT(("Looper::processFrame A"));
	if ( _blackout ) {
		cvZero(fi);
		return;
	}

    int whichrecording = -1;

    // DEBUGPRINT(("processFrame24Bit"));
    for (int n=0; n<MAX_LOOPS; n++) {
        if ( _recording[n] == 0 )
            continue;
		// DEBUGPRINT(("Recording is on for loop n=%d\n",n));
        whichrecording = n;
        fl = &_loop[n];
        // If recording (as opposed to overlaying), add a frame
        if ( _recording[n] == 1 ) {
            if ( fl->nframes >= MAX_FRAMES ) {
                DEBUGPRINT(("Too many frames recorded, turning off recording"));
                fl->advanceToStart();
                _recording[n] = 0;
            } else {
                VFrame* tf = fl->addVFrame(fi);
				// DEBUGPRINT(("Added frame %d to loop\n",tf->framenum));
				fl->setNextToShow(tf);
            }
        } else if ( _recording[n] == 2 ) {
            fl->replaceVFrame(fi);
        } else {
            DEBUGPRINT(("Unexpected value of _recording=%d for n=%d",_recording[n],n));
        }
    }
	// DEBUGPRINT(("process mid1\n"));
	// DEBUGPRINT(("Looper::processFrame B"));

    IplImage* i2 = NULL;

    // if any are showing the live image, save a copy of it
    bool savelive = FALSE;
    for (int n=0; n<MAX_LOOPS; n++) {
        if ( _playing[n] == 2 ) {
            savelive = TRUE;
        }
    }
	
#if 0
	int interp = CV_INTER_LINEAR;  // or CV_INTER_NN
	interp = CV_INTER_NN;  // This produces some artifacts, but increases the frame rate from 20 to 30
#endif

    if ( savelive ) {
        cvResize(fi, _tmpImage, CV_interp);
    }

	// THIS CALL IS A BOTTLENECK
    cvZero(fi);
    // cvConvertScale( fi, fi, 0.5, 0 );

    bool trynew = TRUE;

    IplImage *toimg;
    CvSize tosz;

    if ( _currentLoop < 0 || _currentLoop >= MAX_LOOPS )
        _currentLoop = 0;  // just in case

    int n = _currentLoop;
    int nplayed = 0;
    // Make sure _currentLoop is the last one drawn.
	int nfull = 0;
	int nshowing = num_showing();

	// DEBUGPRINT(("process mid2\n"));

    for (n++; nplayed<nshowing; n++ ) {
		// DEBUGPRINT(("process mid3a n=%d\n",n));
        if ( n >= MAX_LOOPS )
            n = 0;
 		// DEBUGPRINT(("LOOPTOP n=%d nplayed=%d is_showing=%d\n",n,nplayed,is_showing(n)));

		if ( ! is_showing(n) ) {
            continue;
		}
		nplayed++;

        if ( _smooth ) {
            _towardtarget(n,&nfull);
        }
		if ( _sz[n].height == 0 || _sz[n].width == 0 ) {
			DEBUGPRINT(("HEY! n=%d height/width=0?\n",n));
		}

        int w, h;
        int x, y;
        if ( trynew == FALSE ) {
        } else {
            w = _sz[n].width;
            h = _sz[n].height;
 			// the expression (+h/2-h) is so that
			// if h is odd, it will round down (as
			// opposed to using (-h/2) which would
			// round the resulting value up
			x = _pos[n].x + w/2 - w;
            y = _pos[n].y + h/2 - h; 

            if ( x < 0 )
                x = 0;
            if ( y < 0 )
                y = 0;
			if ( (x+w) > ffWidth ) {
				DEBUGPRINT(("Hey, x+w>ffWidth\n"));
				w = ffWidth - x;
			}
			if ( (y+h) > ffHeight ) {
				DEBUGPRINT(("Hey, y+h>ffHeight\n"));
				h = ffHeight - y;
			}

            toimg = _szImage[n];
            tosz = _sz[n];
        }

        if ( _playing[n] == 2 ) {
            // feed live image through
            cvResize(_tmpImage, toimg, CV_interp);
            if ( _border ) {
                imageborder_normal(toimg,tosz);
            }
        } else {
            fl = &_loop[n];
            if ( fl->nexttoshow == NULL ) {
				DEBUGPRINT(("HEY nexttoshow==NULL in process for loop n=%d\n",n));
                continue;
            }
			// DEBUGPRINT(("Should be playing frame =%d\n",fl->nexttoshow->framenum));
            cvResize(fl->nexttoshow->image, toimg, CV_interp);
            // DEBUGPRINT(("n=%d which=%d",n,whichrecording));
#define NORECORDBORDER 0
            if ( _recborder && (n == whichrecording) && (! NORECORDBORDER) ) {
                imageborder_record(toimg,tosz);
            } else if ( _border ) {
                imageborder_normal(toimg,tosz);
            }
            // cvXor(fl->nexttoshow->image,fi,fi,NULL);
        }
		// DEBUGPRINT(("process mid5 n=%d\n",n));

        cvSetImageROI(fi,cvRect(x,y,w,h));
		// DEBUGPRINT(("process mid5b n=%d\n",n));

		int realn = n;
        if ( _enableXOR ) {
			// DEBUGPRINT(("cvXOR n=%d \n",n));
			// DEBUGPRINT(("process mid5c n=%d\n",n));
			try {
				cvXor(toimg,fi,fi,NULL);
			} catch (...) {
				DEBUGPRINT(("Hey! exception in cvXor!\n"));
			}
			// DEBUGPRINT(("process mid5c2 n=%d\n",n));
        } else {
 			// DEBUGPRINT(("cvOR n=%d \n",n));
			// DEBUGPRINT(("process mid5d n=%d\n",n));
            cvOr(toimg,fi,fi,NULL);
            // cvCopy(toimg,fi,NULL);
        }
 		// DEBUGPRINT(("process mid5e n=%d\n",n));
		cvResetImageROI( fi );
		// DEBUGPRINT(("process mid6 n=%d\n",n));

    }

	// DEBUGPRINT(("Looper::processFrame C"));
	// DEBUGPRINT(("process mid7\n"));

    // CvPoint pt1 = {10,10};
    // CvPoint pt2 = {100,100};
    // cvRectangle(fi,pt1,pt2,CV_RGB(255,0,0),5 /*width*/,8,0);

    // Advance the nexttoshow value of the playing loops
    for (int n=0; n<MAX_LOOPS; n++) {
        if ( _playing[n] == 1 ) {
			int r = _loop[n].advance();
#if 0
            if ( r == 1 ) {
                DEBUGPRINT(("Turning loop n=%d off because advance returned 1"));
            }
#endif
        }
    }
}

int getAsInt32(int& nargs, const char* & types, osc::ReceivedMessage::const_iterator& arg, int default = 0)
{
	if ( nargs > 0 ) {
		nargs--;
		char t = *types++;
		if ( t != 'i' ) {
			DEBUGPRINT(("Expected integer, found %c !?\n",t));
			return default;
		}
		return (arg++)->AsInt32();
	} else {
		return default;
	}
}

float getAsFloat(int& nargs, const char* & types, osc::ReceivedMessage::const_iterator& arg, float default = 0.0)
{
	if ( nargs > 0 ) {
		nargs--;
		char t = *types++;
		if ( t != 'f' ) {
			DEBUGPRINT(("Expected float, found %c !?\n",t));
			return default;
		}
		return (arg++)->AsFloat();
	} else {
		return default;
	}
}

void
Looper::_set_fliph(bool onoff)
{
	DEBUGPRINT(("_set_fliph needs work"));
#if 0
	_fliph = onoff;
	post2flip->setparam("Horizontal",(float)onoff);
#endif
}

void
Looper::set_flipv(bool onoff)
{
	DEBUGPRINT(("_set_flipv needs work"));
#if 0
	_flipv = onoff;
	post2flip->setparam("Vertical",(float)onoff);
#endif
}
