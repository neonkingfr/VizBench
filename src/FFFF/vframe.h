#ifndef __VFRAME_H__
#define __VFRAME_H__

#include "opencv/cv.h"
#if 0
#include "BaseTsd.h"
#include "windef.h"
#include "winbase.h"
#include "winnt.h"
#endif
#include <mmsystem.h>

typedef struct VFrame {
	DWORD time;
	unsigned int framenum;
	// WORD bitDepth;		// enumerated indicator of bit depth of video 
						// 0 = 16 bit 5-6-5   1 = 24bit packed   2 = 32bit
	
	IplImage* image;
	// LPVOID bits;
	struct VFrame* next;
	struct VFrame* prev;
} VFrame;

class VFrameLoop {

public:
	int nframes;
	float framerate;
	float pos_within_frame;
	DWORD time_of_loop_start;
	VFrame* first;	// start of circular list
	VFrame* nexttoshow;
	VFrame* start;  // loop starts here
	VFrame* end;
	float startpos;
	float endpos;
	int reverse;
	float savedpos;
	int frozen;

	VFrameLoop() {
		clearme();
	};

	void clearme() {
		nframes = 0;
		first = NULL;
		nexttoshow = NULL;
		start = NULL;
		end = NULL;
		startpos = 0.0;
		endpos = 1.0;
		savedpos = 0.0;
		framerate = 1.0;
		reverse = 0;
		pos_within_frame = 0.0;
		time_of_loop_start = timeGetTime();
		frozen = 0;
	};

	VFrame* addVFrame(IplImage* i) {

		VFrame* f = _allocateVFrame(i);
		f->framenum = nframes;
		// NosuchDebug("AddVFrame framenum=%d",f->framenum);		f->time = timeGetTime();
		if ( first == NULL ) {
			first = f;
			f->next = f;
			f->prev = f;
		} else {
			f->next = first;
			f->prev = first->prev;
			f->prev->next = f;
			f->next->prev = f;
		}
		nframes++;
		return(f);
	};

	VFrame* replaceVFrame(IplImage* i) {
		if ( nexttoshow == NULL ) {
			NosuchDebug("Hey, replaceVFrame found nexttoshow==NULL?");
			return NULL;
		} else {
			cvReleaseImage(&(nexttoshow->image));
			nexttoshow->image = cvCloneImage(i);
			return nexttoshow;
		}
	};

	void freeandclear() {
		if ( first != NULL ) {
			_freeme();
		}
		clearme();
	};

	void setfreeze(int freeze) {
		frozen = freeze;
	};

	void setStartPos(float pos) {
		if ( reverse ) {
			endpos = pos;
		} else {
			startpos = pos;
		}
		posChanged();
	}

	void setEndPos(float pos) {
		if ( reverse ) {
			startpos = pos;
		} else {
			endpos = pos;
		}
		posChanged();
	}

	void posChanged() {
		if ( startpos > endpos ) {
			NosuchDebug("startpos > endpos, switching them");
			float t = startpos;
			startpos = endpos;
			endpos = t;
			reverse = 1 - reverse;
		}
		start = frameOfPos(startpos);
		end = frameOfPos(endpos);
		advanceToStart();
	}

	void setReverse(int onoff) {
		reverse = onoff;
	}

	VFrame* frameOfPos(float pos) {
		int fnum = int(nframes * pos) % nframes;
		VFrame* f = first;
		for ( int n=0; n<fnum; n++ ) {
			f = f->next;
		}
		return f;
	}

	void setNextToShow(VFrame* f) {
		nexttoshow = f;
		pos_within_frame = 0.0;
	}

	void advanceToStart() {
		if ( start != NULL ) {
			nexttoshow = start;
		} else if ( first != NULL ) {
			nexttoshow = first;
		} else {
			NosuchDebug("No loop to start!?");
			nexttoshow = NULL;
		}
		pos_within_frame = 0.0;
		time_of_loop_start = timeGetTime();
	}

	void advanceToEnd() {
		if ( end != NULL ) {
			nexttoshow = end;
		} else if ( first != NULL && first->prev != NULL ) {
			nexttoshow = first->prev;
		} else {
			NosuchDebug("Unexpected, error in advanceToEnd");
		}
		pos_within_frame = 0.0;
		time_of_loop_start = timeGetTime();
	}

	void resetLoop() {
		nexttoshow = first;
		pos_within_frame = 0.0;
		time_of_loop_start = timeGetTime();
	}

	int advance() {
		if ( nexttoshow == NULL ) {
			// NosuchDebug("HMMMM - VFrame::advance finds nexttoshow=NULL?");
			resetLoop();
			return 1;
		}
		if ( frozen == 0 ) {
			pos_within_frame += framerate;
		}
		// NosuchDebug("pos_with_frame = %lf  num=%d",pos_within_frame,(nexttoshow==NULL?-1:nexttoshow->framenum));
		while ( pos_within_frame >= 1.0 ) {
			// NosuchDebug("advancing!");
			if ( nexttoshow == NULL ) {
				NosuchDebug("Hey! advance changed nexttoshow to NULL!?");
				resetLoop();
				return 1;
			}
			if ( reverse ) {
				nexttoshow = nexttoshow->prev;
				// NosuchDebug("Reverse A");
			} else {
				nexttoshow = nexttoshow->next;
				// NosuchDebug("advancing to next A");
			}
			if ( !reverse && nexttoshow == end && end != NULL ) {
				// NosuchDebug("advancing to start (from end)");
				advanceToStart();
			} else if ( reverse && nexttoshow == start && start != NULL ) {
				// NosuchDebug("advancing to end (from start)");
				advanceToEnd();
			} else if ( nexttoshow == first ) {
				// NosuchDebug("advancing, now at first");
				if ( reverse ) {
					advanceToEnd();
					// NosuchDebug("Reverse advance to end");
				} else {
					// NosuchDebug("Normal advance to start=%ld",(long)start);
					advanceToStart();
				}
			}
			pos_within_frame -= 1.0;
		}
		return 0;
	}
private:

	VFrame* _allocateVFrame(IplImage* i) {
		VFrame* vf = (VFrame*) calloc(1,sizeof(VFrame));
		vf->time = 0;
		vf->image = cvCloneImage(i);
		vf->next = NULL;
		vf->prev = NULL;
		return vf;
	};

	void _freeVFrame(VFrame* vf) {
		// Reverse whatever _allocateVFrame does
		cvReleaseImage(&(vf->image));
		free(vf);
	};

	void _freeme() {
		while ( first != NULL ) {
			VFrame* vf = first;
			// Remove it from the circular list
			if ( vf->next != vf ) {
				first = vf->next;
				first->prev = vf->prev;
				vf->prev->next = first;
			} else {
				first = NULL;
			}
			_freeVFrame(vf);
		}
	};
};

#endif