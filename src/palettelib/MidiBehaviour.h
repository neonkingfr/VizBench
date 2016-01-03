#ifndef _MIDIBEHAVIOUR_H
#define _MIDIBEHAVIOUR_H

class MidiBehaviour {

public:
	MidiBehaviour(Channel* c);

	static MidiBehaviour* makeBehaviour(std::string type, Channel* c);

	Palette* palette();
	Channel* channel() { return _channel; }
	int channelnum() { return _channel->id; }  // 0-15
	virtual std::string name() = 0;
	virtual void NoteOn(int pitch,int velocity) = 0;

private:
	Channel* _channel;
	Palette* _palette;
};

class MidiBehaviourDefault : public MidiBehaviour {
public:
	MidiBehaviourDefault(Channel* c) : MidiBehaviour(c) { }
	std::string name() { return "default"; }
	void NoteOn(int pitch,int velocity);
};

class MidiBehaviourHorizontal : public MidiBehaviour {
public:
	MidiBehaviourHorizontal(Channel* c) : MidiBehaviour(c) { }
	std::string name() { return "horizontal"; }
	void NoteOn(int pitch,int velocity);
};

class MidiBehaviourVertical : public MidiBehaviour {
public:
	MidiBehaviourVertical(Channel* c) : MidiBehaviour(c) { }
	std::string name() { return "vertical"; }
	void NoteOn(int pitch,int velocity);
};

class MidiBehaviourOutline : public MidiBehaviour {
public:
	MidiBehaviourOutline(Channel* c) : MidiBehaviour(c) { }
	std::string name() { return "outline"; }
	void NoteOn(int pitch,int velocity);
};

#endif
