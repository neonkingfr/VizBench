#ifndef _MIDIBEHAVIOUR_H
#define _MIDIBEHAVIOUR_H

class Channel;

class MidiBehaviour : public Behaviour {

public:
	MidiBehaviour(Channel* c);

	Channel* channel() { return _channel; }
	int channelnum() { return _channel->id; }  // 0-15
	std::string name();
	void NoteOn(int pitch,int velocity);

private:
	Channel* _channel;
};

#endif
