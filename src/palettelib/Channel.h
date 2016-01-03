#ifndef _CHANNEL_H
#define _CHANNEL_H

class NosuchScheduler;
class MMTT_SharedMemHeader;
class MidiBehaviour;

#if 0
class ChannelParams : public Params {
public:
	ChannelParams() {
#include "ChannelParams_init.h"
	}

	static std::vector<std::string> movedirTypes;
	static std::vector<std::string> rotangdirTypes;
	static std::vector<std::string> mirrorTypes;
	static std::vector<std::string> controllerstyleTypes;
	static std::vector<std::string> shapeTypes;
	static std::vector<std::string> behaviourTypes;

	static void Initialize();

	std::string JsonString(std::string pre, std::string indent, std::string post) {
		char* names[] = {
#include "ChannelParams_list.h"
			NULL
		};
		return JsonList(names,pre,indent,post);
	}

	void Set(std::string nm, std::string val) {
		bool stringval = false;

#define SET_DBL_PARAM(name) else if ( nm == #name ) name = string2double(val)
#define SET_INT_PARAM(name) else if ( nm == #name ) name = string2int(val)
#define SET_BOOL_PARAM(name) else if ( nm == #name ) name = string2bool(val)
#define SET_STR_PARAM(name) else if ( nm == #name ) (name = val),(stringval=true)

		if ( false ) { }
#include "ChannelParams_set.h"

		// To abide by the limits for each value, we rely on the code in Increment()
		if ( ! stringval ) {
			Increment(nm,0.0);
		}
	}
	void Increment(std::string nm, double amount) {

#define INC_DBL_PARAM(name,mn,mx) else if (nm==#name)name=adjust(name,amount,mn,mx)
#define INC_INT_PARAM(name,mn,mx) else if (nm==#name)name=adjust(name,amount,mn,mx)
#define INC_STR_PARAM(name,vals) else if (nm==#name)name=adjust(name,amount,vals)
#define INC_BOOL_PARAM(name) else if (nm==#name)name=adjust(name,amount)
#define INC_NO_PARAM(name) else if (nm==#name)name=name

		if ( false ) { }
#include "ChannelParams_increment.h"
	}
	void Toggle(std::string nm) {
		// Just the boolean values
#define TOGGLE_PARAM(name) else if ( nm == #name ) name = ! name
		if ( false ) { }
#include "ChannelParams_toggle.h"
		else { NosuchDebug("No Toggle implemented for %s",nm.c_str()); }
	}
	std::string Get(std::string nm) {

#define GET_DBL_PARAM(name) else if(nm==#name)return DoubleString(name)
#define GET_INT_PARAM(name) else if(nm==#name)return IntString(name)
#define GET_BOOL_PARAM(name) else if(nm==#name)return BoolString(name)
#define GET_STR_PARAM(name) else if(nm==#name)return name

		if ( false ) { }
#include "ChannelParams_get.h"
		return "";
	}

#include "ChannelParams_declare.h"

	bool IsSpriteParam(std::string nm) {

#define IS_SPRITE_PARAM(name) if( nm == #name ) { return true; }

#include "ChannelParams_issprite.h"
		return false;
	}
};

class ChannelOverrides : public Params {
public:
	ChannelOverrides() {
#define OVERRIDE_INIT(name) name = false;

#include "ChannelOverrides_init.h"
	}
	std::string JsonString(std::string pre, std::string indent, std::string post) {
		char* names[] = {
#include "ChannelOverrides_list.h"
			NULL
		};
		return JsonList(names,pre,indent,post);
	}
	void Set(std::string nm, std::string override) {
		return Set(nm,string2bool(override));
	}
	void Set(std::string nm, bool override) {

#define SET_OVERRIDE(name) else if (nm == #name ) name = override

		if ( false ) { }
#include "ChannelOverrides_set.h"
	}
	std::string Get(std::string nm) {
		return GetBool(nm) ? "true" : "false";
	}
	bool GetBool(std::string nm) {

#define GET_BOOL(name) else if (nm == #name ) return name

		if ( false ) { }
#include "ChannelOverrides_get.h"
		return false;
	}
#include "ChannelOverrides_declare.h"
};
#endif

class Channel {

public:
	Channel(Palette* p, int id);
	~Channel();

#if 0
	// These current values are a mixure of Overridden parameters
	// (i.e. palette->channelOverrideParams) and channelSpecificParams.
	ChannelParams params;

	// These are the values specific to this channel
	ChannelParams channelSpecificParams;
#endif

	int id;  // 0-15
	Palette* palette;

	int channelnum() { return id; }  // 0-15

#if 0
	static private std::string shapeOfChannel(int id);
	static private double hueOfChannel(int id);
	double getMoveDir(std::string movedir);
	void addSpriteToList(Sprite* s);
#endif

	void draw();
	void advanceTo(int tm);
	void hitSprites();
	void gotNoteOn(int pitch, int velocity);
	void setMidiBehaviour(std::string b);

	void instantiateSprite(int pitch, int velocity, NosuchVector pos, double depth);
	void instantiateOutlines(int pitch);
	NosuchScheduler* scheduler();

#if 0
	// Reset the Channel's current parameters, combining:
	//  rp - the channel-specific parameter values
	//  op - the parameter values which override channel-specific values
	//  ro - the flags which say which parameters get overridden
	void ResetParams( ChannelParams& rp,
						ChannelParams& op, ChannelOverrides& ro) {

#define RESET_PARAM(name) params.##name = ro.##name ? op.##name : rp.##name

#include "Channel_reset.h"
	}
#endif

private:

	MidiBehaviour* _midiBehaviour;
	std::string _midiBehaviourName;

	// pthread_mutex_t _channel_mutex;
	// pthread_rwlock_t spritelist_rwlock;
	// pthread_rwlock_t cursorlist_rwlock;

	// Access to these lists need to be thread-safe
	// std::list<Sprite*> sprites;
	SpriteList* spritelist;
};

#endif