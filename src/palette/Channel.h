#ifndef _CHANNEL_H
#define _CHANNEL_H

class NosuchLoop;
class NosuchScheduler;
class MMTT_SharedMemHeader;
class MidiBehaviour;

class Channel {

public:
	Channel(Palette* p, int id);
	~Channel();

	// These current values are a mixure of Overridden parameters
	// (i.e. palette->channelOverrideParams) and channelSpecificParams.
	AllVizParams* params;

	int id;  // 0-15
	Palette* palette() { return _palette; }

	int channelnum() { return id; }  // 0-15
#ifdef LOOPSTUFF
	void init_loop();
#endif
	static private std::string shapeOfChannel(int id);
	static private double hueOfChannel(int id);
	double getMoveDir(std::string movedir);
	void addSpriteToList(VizSprite* s);
	void draw(PaletteHost* b);
	void advanceTo(int tm);
	void hitSprites();
	void gotNoteOn(int pitch, int velocity);
	void setMidiBehaviour(std::string b);

	void spritelist_lock_read();
	void spritelist_lock_write();
	void spritelist_unlock();

	// void instantiateSprite(int pitch, int velocity, NosuchVector pos, double depth);

#ifdef LOOPSTUFF
	bool Looping() { return _looping; }
	bool Looping(bool b) { _looping = b; return _looping; }
	int LoopId();
#endif
#ifdef OUTLINESTUFF
	void instantiateOutlines(int pitch);
#endif
	NosuchScheduler* scheduler();

private:

	Palette* _palette;
	NosuchLoop* _loop;
	bool _looping;
	MidiBehaviour* _midiBehaviour;
	std::string _midiBehaviourName;

	pthread_mutex_t _channel_mutex;
	// pthread_rwlock_t spritelist_rwlock;
	pthread_rwlock_t cursorlist_rwlock;

	// Access to these lists need to be thread-safe
	// std::list<Sprite*> sprites;
	// VizSpriteList* spritelist;

};

#endif