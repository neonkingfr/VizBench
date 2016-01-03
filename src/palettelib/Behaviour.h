#ifndef _BEHAVIOUR_H
#define _BEHAVIOUR_H

class Region;
class Palette;
class Cursor;

class Behaviour {

public:
	Behaviour(Region* r);
	Palette* palette();
	Region* region();
	std::list<Cursor*>& cursors();
	int SelectionNumber(Cursor* c);
	bool isButtonDown(std::string bn);
	virtual bool isMyButton(std::string bn) { return true; }

private:
	Region* _region;
	Palette* _palette;
};

#endif