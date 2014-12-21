#ifndef _RESOLUME_H
#define _RESOLUME_H

// represents on/off state of effects which are attached in series (in Resolume)
class EffectSet {
public:
	EffectSet(bool e0, bool e1, bool e2, bool e3, bool e4,
		bool e5, bool e6, bool e7 , bool e8,
		bool e9, bool e10, bool e11 , bool e12) {
		effectOn[0] = e0; effectOn[1] = e1; effectOn[2] = e2;
		effectOn[3] = e3; effectOn[4] = e4; effectOn[5] = e5;
		effectOn[6] = e6; effectOn[7] = e7; effectOn[8] = e8;
		effectOn[9] = e9; effectOn[10] = e10; effectOn[11] = e11;
		effectOn[12] = e12;
	}
	bool effectOn[13];
};

#define NUM_EFFECT_SETS 12
extern EffectSet buttonEffectSet[NUM_EFFECT_SETS];

#endif