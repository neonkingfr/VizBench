#include "PaletteAll.h"

MidiBehaviour::MidiBehaviour(Channel* c) : Behaviour(c->palette(),NULL) {
	_channel = c;
}

double
val_for_pitch(int pitch, Channel* c) {
	pitch -= (int)(c->params->pitchoffset.get());
	pitch = (int)(pitch * c->params->pitchfactor.get());
	return fmod(pitch/127.0,1.0);
}

void
MidiBehaviour::NoteOn(int pitch, int velocity) {
	AllVizParams* params = channel()->params;
	std::string logic = params->noteonlogic.get();
	if ( logic == "default" ) {
		double x = 0.5;
		double y = 0.5;
		double z = velocity / 127.0;
		NosuchPos pos(x,y,z);
		vizlet()->makeAndAddVizSprite(params,pos);
	} else if ( logic == "horizontal" ) {
		double hvpos = params->hvpos;
		double x = val_for_pitch(pitch,channel());
		double y = hvpos;
		double z = velocity / 127.0;
		NosuchPos pos(x,y,z);
		vizlet()->makeAndAddVizSprite(params,pos);
	} else if ( logic == "vertical" ) {
		double hvpos = params->hvpos.get();
		double x = hvpos;
		double y = val_for_pitch(pitch,channel());
		double z = velocity / 127.0;
		NosuchPos pos(x,y,z);
		vizlet()->makeAndAddVizSprite(params,pos);
	} else if ( logic == "outline" ) {
		// channel()->instantiateOutlines(pitch);
		DEBUGPRINT(("Unimplemented value for noteonlogic: %s",logic.c_str()));
	} else {
		DEBUGPRINT(("Unrecognized value for noteonlogic: %s",logic.c_str()));
	}
}