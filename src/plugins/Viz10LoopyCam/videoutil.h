#ifndef __VIDEOUTIL_H__
#define __VIDEOUTIL_H__

#include <mmsystem.h>

#define PARAM_DISPLAY_LEN 16

typedef struct VideoPixel24bitTag {
	BYTE blue;
	BYTE green;
	BYTE red;
} VideoPixel24bit;

typedef struct VideoPixel16bitTag {
	BYTE fb;
	BYTE sb;
} VideoPixel16bit;

typedef struct VideoPixel32bitTag {
	BYTE blue;
	BYTE green;
	BYTE red;
	BYTE alpha;
} VideoPixel32bit;

typedef struct VideoParamInfoTag {
	char *name;
	float defaultval;
	int type;
} VideoParamInfo;

typedef struct VideoParamDataTag {
	float value;
	char display[PARAM_DISPLAY_LEN];
} VideoParamData;

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
	double framerate;
	double pos_within_frame;
	DWORD time_of_loop_start;
	VFrame* first;	// start of circular list
	VFrame* nexttoshow;
	VFrame* start;  // loop starts here
	VFrame* end;
	double startpos;
	double endpos;
	int reverse;
	double savedpos;
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
			DEBUGPRINT(("Hey, replaceVFrame found nexttoshow==NULL?"));
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

	void setStartPos(double pos) {
		if ( reverse ) {
			endpos = pos;
		} else {
			startpos = pos;
		}
		posChanged();
	}

	void setEndPos(double pos) {
		if ( reverse ) {
			startpos = pos;
		} else {
			endpos = pos;
		}
		posChanged();
	}

	void posChanged() {
		if ( startpos > endpos ) {
			DEBUGPRINT(("startpos > endpos, switching them"));
			double t = startpos;
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

	VFrame* frameOfPos(double pos) {
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
			DEBUGPRINT(("No loop to start!?"));
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
			DEBUGPRINT(("Unexpected, error in advanceToEnd"));
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
			resetLoop();
			return 1;
		}
		if ( frozen == 0 ) {
			pos_within_frame += framerate;
		}
		while ( pos_within_frame >= 1.0 ) {
			if ( nexttoshow == NULL ) {
				DEBUGPRINT(("Hey! advance changed nexttoshow to NULL!?"));
				resetLoop();
				return 1;
			}
			if ( reverse ) {
				nexttoshow = nexttoshow->prev;
			} else {
				nexttoshow = nexttoshow->next;
			}
			if ( !reverse && nexttoshow == end && end != NULL ) {
				advanceToStart();
			} else if ( reverse && nexttoshow == start && start != NULL ) {
				advanceToEnd();
			} else if ( nexttoshow == first ) {
				if ( reverse ) {
					advanceToEnd();
				} else {
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