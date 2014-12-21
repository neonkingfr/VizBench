#ifndef _GRAPHIC_BEHAVIOUR_H
#define _GRAPHIC_BEHAVIOUR_H

class Palette;
class Region;
class Param;

class GraphicBehaviour : public Behaviour {

public:
	GraphicBehaviour(Region* a);
	void buttonDown(std::string bn);
	void buttonUp(std::string bn);
	void cursorDown(VizCursor* c);
	void cursorDrag(VizCursor* c);
	void cursorUp(VizCursor* c);
	void advanceTo(int tm);
	std::string name() { return "default"; }
	bool isMyButton(std::string bn);
};

#endif