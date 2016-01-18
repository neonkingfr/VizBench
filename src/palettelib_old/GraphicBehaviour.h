#ifndef _GRAPHIC_BEHAVIOUR_H
#define _GRAPHIC_BEHAVIOUR_H

class Palette;
class Region;
class Sprite;
class Param;
class Cursor;

class GraphicBehaviour : public Behaviour {

public:
	GraphicBehaviour(Region* a);
	static void initialize();
	static GraphicBehaviour* makeBehaviour(std::string type, Region* a);
	// abstract void fireSprite(Cursor c, ParamList params) throws Exception;

	virtual bool isSelectorDown() = 0;
	virtual void buttonDown(std::string bn) = 0;
	virtual void buttonUp(std::string bn) = 0;
	virtual void cursorDown(Cursor* c) = 0;
	virtual void cursorDrag(Cursor* c) = 0;
	virtual void cursorUp(Cursor* c) = 0;
	virtual void cursorDownWithSelector(Cursor* c) = 0;
	virtual void cursorDragWithSelector(Cursor* c) = 0;
	virtual void cursorUpWithSelector(Cursor* c) = 0;
	virtual void advanceTo(int tm) = 0;
	virtual std::string name() = 0;

	static bool initialized;
	static std::vector<std::string> behaviourTypes;
	static std::vector<std::string> movedirTypes;
	static std::vector<std::string> rotangdirTypes;
	static std::vector<std::string> mirrorTypes;
};

class GraphicBehaviourDefault : public GraphicBehaviour {
	
public:
	GraphicBehaviourDefault(Region* a);
	bool isSelectorDown();
	void buttonDown(std::string bn);
	void buttonUp(std::string bn);
	void cursorDown(Cursor* c);
	void cursorDrag(Cursor* c);
	void cursorUp(Cursor* c);
	void cursorDownWithSelector(Cursor* c);
	void cursorDragWithSelector(Cursor* c);
	void cursorUpWithSelector(Cursor* c);
	void advanceTo(int tm);
	std::string name() { return "default"; }
private:
	void loopCursorDown(Cursor* c);
};

class GraphicBehaviourMuseum : public GraphicBehaviourDefault {
	
public:
	GraphicBehaviourMuseum(Region* a);
	bool isSelectorDown();
	void buttonDown(std::string bn);
	void buttonUp(std::string bn);
	void cursorDownWithSelector(Cursor* c);
	void cursorDragWithSelector(Cursor* c);
	void cursorUpWithSelector(Cursor* c);
	std::string name() { return "museum"; }
private:
	void loopCursorDown(Cursor* c);
};

class GraphicBehaviourSTEIM : public GraphicBehaviourDefault {
	
public:
	GraphicBehaviourSTEIM(Region* a);
	bool isSelectorDown();
	void buttonDown(std::string bn);
	void buttonUp(std::string bn);
	void cursorDownWithSelector(Cursor* c);
	void cursorDragWithSelector(Cursor* c);
	void cursorUpWithSelector(Cursor* c);
	std::string name() { return "STEIM"; }
private:
	void loopCursorDown(Cursor* c);
};

class GraphicBehaviourCasual : public GraphicBehaviourDefault {
	
public:
	GraphicBehaviourCasual(Region* a);
	bool isMyButton(std::string bn);
	bool isSelectorDown();
	void buttonDown(std::string bn);
	void buttonUp(std::string bn);
	void cursorDownWithSelector(Cursor* c);
	void cursorDragWithSelector(Cursor* c);
	void cursorUpWithSelector(Cursor* c);
	std::string name() { return "casual"; }
private:
	void loopCursorDown(Cursor* c);
};

class GraphicBehaviourBurn : public GraphicBehaviourDefault {
	
public:
	GraphicBehaviourBurn(Region* a);
	bool isMyButton(std::string bn);
	bool isSelectorDown();
	void buttonDown(std::string bn);
	void buttonUp(std::string bn);
	void cursorDownWithSelector(Cursor* c);
	void cursorDragWithSelector(Cursor* c);
	void cursorUpWithSelector(Cursor* c);
	std::string name() { return "burn"; }
private:
	void loopCursorDown(Cursor* c);
};

class GraphicBehaviour2013 : public GraphicBehaviourDefault {
	
public:
	GraphicBehaviour2013(Region* a);
	bool isMyButton(std::string bn);
	bool isSelectorDown();
	void buttonDown(std::string bn);
	void buttonUp(std::string bn);
	void cursorDownWithSelector(Cursor* c);
	void cursorDragWithSelector(Cursor* c);
	void cursorUpWithSelector(Cursor* c);
	std::string name() { return "2013"; }
private:
	void loopCursorDown(Cursor* c);
};


#endif