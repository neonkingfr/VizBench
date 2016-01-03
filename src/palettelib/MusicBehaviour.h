#ifndef _MUSIC_BEHAVIOUR_H
#define _MUSIC_BEHAVIOUR_H

class MusicBehaviour : public Behaviour {

public:
	MusicBehaviour(Region* a);
	static void initialize();
	static MusicBehaviour* makeBehaviour(std::string type, Region* a);

	static void tonic_change(Palette* palette);
	static void tonic_reset(Palette* palette);

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

	// void advanceTo(int tm);

	static bool initialized;
	static std::vector<std::string> behaviourTypes;
	static std::vector<std::string> controllerTypes;

	int CurrentClick() {
		return VizServer::GetServer()->SchedulerCurrentClick();
	}

protected:
	int quantizeToNext(int clicks, int q);
	int Cursor2Pitch(Cursor* c);
	int Cursor2Quant(Cursor* c);
	void doNewNote(Cursor* c);
	void doController(int ch, int ctrlr, int val, int sidnum, bool smooth);
	void doNewZController(Cursor* c, int val, bool smooth);
	void doNewPitchBend(double val, bool up, bool smooth);
	void setRegionSound(int rid, std::string nm);
	void changeSoundSet(int selected);
	int nextSoundSet();
	int prevSoundSet();
	int randSoundSet();
};

class MusicBehaviourDefault : public MusicBehaviour {
	
public:
	MusicBehaviourDefault(Region* a);
	virtual bool isSelectorDown();
	virtual void buttonDown(std::string bn);
	virtual void buttonUp(std::string bn);
	virtual void cursorDown(Cursor* c);
	virtual void cursorDrag(Cursor* c);
	virtual void cursorUp(Cursor* c);
	virtual void cursorDownWithSelector(Cursor* c);
	virtual void cursorDragWithSelector(Cursor* c);
	virtual void cursorUpWithSelector(Cursor* c);
	virtual void advanceTo(int tm);
	virtual std::string name() { return "default"; }
};

class MusicBehaviourMuseum : public MusicBehaviourDefault {
	
public:
	MusicBehaviourMuseum(Region* a);
	bool isSelectorDown();
	void buttonDown(std::string bn);
	void buttonUp(std::string bn);
	std::string name() { return "museum"; }
	void cursorDownWithSelector(Cursor *c);
};

class MusicBehaviourSTEIM : public MusicBehaviourDefault {
	
public:
	MusicBehaviourSTEIM(Region* a);
	virtual bool isSelectorDown();
	virtual void buttonDown(std::string bn);
	virtual void buttonUp(std::string bn);
	virtual void cursorDown(Cursor* c);
	virtual void cursorDrag(Cursor* c);
	virtual void cursorUp(Cursor* c);
	virtual void cursorDownWithSelector(Cursor* c);
	virtual void cursorDragWithSelector(Cursor* c);
	virtual void cursorUpWithSelector(Cursor* c);
	virtual void advanceTo(int tm);
	virtual std::string name() { return "STEIM"; }
};

class MusicBehaviourCasual : public MusicBehaviourDefault {
	
public:
	MusicBehaviourCasual(Region* a);
	virtual bool isMyButton(std::string bn);
	virtual bool isSelectorDown();
	virtual void buttonDown(std::string bn);
	virtual void buttonUp(std::string bn);
	virtual void cursorDown(Cursor* c);
	virtual void cursorDrag(Cursor* c);
	virtual void cursorUp(Cursor* c);
	virtual void cursorDownWithSelector(Cursor* c);
	virtual void cursorDragWithSelector(Cursor* c);
	virtual void cursorUpWithSelector(Cursor* c);
	virtual void advanceTo(int tm);
	virtual std::string name() { return "casual"; }
};

class MusicBehaviourBurn : public MusicBehaviourDefault {
	
public:
	MusicBehaviourBurn(Region* a);
	virtual bool isMyButton(std::string bn);
	virtual bool isSelectorDown();
	virtual void buttonDown(std::string bn);
	virtual void buttonUp(std::string bn);
	virtual void cursorDown(Cursor* c);
	virtual void cursorDrag(Cursor* c);
	virtual void cursorUp(Cursor* c);
	virtual void cursorDownWithSelector(Cursor* c);
	virtual void cursorDragWithSelector(Cursor* c);
	virtual void cursorUpWithSelector(Cursor* c);
	virtual void advanceTo(int tm);
	virtual std::string name() { return "burn"; }
};

class MusicBehaviour2013 : public MusicBehaviourDefault {
	
public:
	MusicBehaviour2013(Region* a);
	virtual bool isMyButton(std::string bn);
	virtual bool isSelectorDown();
	virtual void buttonDown(std::string bn);
	virtual void buttonUp(std::string bn);
	virtual void cursorDown(Cursor* c);
	virtual void cursorDrag(Cursor* c);
	virtual void cursorUp(Cursor* c);
	virtual void cursorDownWithSelector(Cursor* c);
	virtual void cursorDragWithSelector(Cursor* c);
	virtual void cursorUpWithSelector(Cursor* c);
	virtual void advanceTo(int tm);
	virtual std::string name() { return "2013"; }
};

#endif