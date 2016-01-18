#ifndef _NOTE_BEHAVIOUR_H
#define _NOTE_BEHAVIOUR_H

class MidiMsg;

class NoteBehaviour {

public:
	NoteBehaviour(Region* r);

	Palette* palette();
	Region* region();

	static void initialize();
	virtual void gotMidiMsg(MidiMsg* msg, std::string sid) = 0;
	virtual std::string name() = 0;

private:
	static bool initialized;
	Region* _region;
	Palette* _palette;
};

class NoteBehaviourDefault : public NoteBehaviour {
public:
	NoteBehaviourDefault(Region* r);
	void gotMidiMsg(MidiMsg* mm, std::string sid);
	std::string name() { return "default"; }
};

	
#endif