#ifndef _BEHAVIOUR_H
#define _BEHAVIOUR_H

class Behaviour {

public:
	Behaviour(Palette* p, Region* r);
	Palette* palette();
	PaletteHost* paletteHost();
	Vizlet* vizlet();
	std::list<VizCursor*>& cursors();
	bool isButtonDown(std::string bn);
	virtual bool isMyButton(std::string bn) { return true; }
	VizServer* vizserver() { return _vizserver; }
	AllVizParams* defaultParams();
	AllVizParams* regionParams();
	Region* region() {
		NosuchAssert(_region);
		return _region;
	}


private:
	Region* _region;
	VizServer* _vizserver;
	Palette* _palette;
	PaletteHost* _paletteHost;
};

#endif