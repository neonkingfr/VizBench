#ifndef __LOOPER_H__
#define __LOOPER_H__

#define NUM_PARAMS 2
#define NUM_INPUTS 1
#define MAX_LOOPS 8
#define MAX_FRAMES 1024

class Looper {

public:

	Looper(int w, int h);
	~Looper();

	void processFrame24Bit(IplImage* fi);

	IplImage *m_frameImage;

	bool _enableXOR;
	bool _border;
	bool _recborder;

	int _currentLoop;
	bool _blackout;
	bool _autoNext;
	VFrameLoop _loop[MAX_LOOPS];
	int _recording[MAX_LOOPS];  // 1==recording, 2=overlaying
	int _playing[MAX_LOOPS];    // 1==playing loop, 2=live input
	// int _numPlaying;
	bool _movesmooth;
	int _moveamount;
	bool _flipv;
	bool _fliph;
	int is_showing(int n) {
		return ( _playing[n] > 0 );
	};
	int num_showing() {
		int nshowing = 0;
		for ( int n=0; n<MAX_LOOPS; n++ ) {
			if ( is_showing(n) ) {
				nshowing++;
			}
		}
		return nshowing;
	}


	int ffWidth;			// width of FreeFrame frame in pixels
	int ffHeight;		// height of FreeFrame frame in pixels
	CvSize ffSize;

	CvPoint _pos[MAX_LOOPS];
	CvPoint _targetpos[MAX_LOOPS];
	CvSize _sz[MAX_LOOPS];
	CvSize _targetsz[MAX_LOOPS];
	double _sizeFactor[MAX_LOOPS];
	IplImage *_szImage[MAX_LOOPS];
	CvSize _quarterSize;
	IplImage *_quarterImage;
	IplImage *_tmpImage;

	int validLoopnum(int loopnum);
	VFrame* createVFrame(IplImage* i);
	void setRecord(bool onoff);
	void setPlay(int loopnum, bool onoff);
	void setBlackout(bool onoff);

	void _setRecordOverlay(bool onoff);
	void _setQuadrants(bool onoff);
	void _setRandomFrame(int loopnum);
	void _restart(int loopnum);
	void _restartrandom(int loopnum);
	void _restartsave(int loopnum);
	void _restartrestore(int loopnum);
	void _freeze(int loopnum,bool freeze);
	void _truncate(int loopnum);
	void _copy24bit(VideoPixel24bit* dest, VideoPixel24bit* src);
	void _randomizeSize(int loopnum, int playing, double aspect);
	void _fullDisplay();
	void _allLive(bool onoff);
	void _quadrantDisplay();
	void debugsize();
	void _changePosSize(int n, CvPoint pt, CvSize sz, bool boundit = TRUE);
	void _nextLoop();
	void _towardtarget(int n, int *nfull);
	void _releaseAndAllocate(int n);
	void _boundPosSize(int *x, int *y, int *w, int *h);
	void _setautoNext(bool v);
	void _setmovesmooth(bool v);
	void _setplayfactor(int loopnum, double x);
	void _resetplayfactor(int loopnum);
	void _set_fliph(bool onoff);
	void _set_flipv(bool onoff);

	void randompositions(double aspect);
	void randomposition(double aspect);
	void morewindows();
	void lesswindows();
	void setwindows(int n);
	void togglemovesmooth();
	void togglexor();
	void toggleautonext();
	void toggleborder();
	void setinterp(bool b);

	IplImage* createImage24ofSize(CvSize sz);

	// void processosc(std::string addr, const char* types, int nargs, osc::ReceivedMessage::const_iterator arg); 
};
						
int	initialise();								
int	deInitialise();							

#endif
