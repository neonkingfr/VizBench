#if 0
#ifndef _REGION_H
#define _REGION_H

#include "RegionParams.h"
#include <list>
#include <pthread.h>

class SpaceCursor;
class Palette;
class Vizlet;
class MidiMsg;

class Region {
	
public:

	Region(Vizlet* v, std::string nm,  int mn, int mx) {
		_vizlet = v;
		_name = nm;
		_sid_min = mn;
		_sid_max = mx;
		_params = NULL;
	}

	virtual Region::~Region() {
	}

	bool containsSid(int sid) {
		return ( sid >= _sid_min && sid < _sid_max );
	}

	std::string name() { return _name; }
	Vizlet* vizlet() { return _vizlet; }
	RegionParams* params() { return _params; }

	virtual void processMidi(MidiMsg* m, bool isinput) { }
	virtual void processCursor(SpaceCursor* c, int downdragup) { }

	void setRegionParams(std::string overname, std::string mainname);

protected:
	Vizlet* _vizlet;
	std::string _name;
	int _sid_min;
	int _sid_max;

private:
	RegionParams* _params;
};

#endif

#endif