#include "PaletteAll.h"

MidiBehaviour::MidiBehaviour(Channel* c) {
	_channel = c;
	_palette = _channel->palette;
	_paletteHost = _palette->paletteHost();
}

Palette* MidiBehaviour::palette() {
	NosuchAssert(_palette);
	return _palette;
}

PaletteHost* MidiBehaviour::paletteHost() {
	NosuchAssert(_paletteHost);
	return _paletteHost;
}

MidiBehaviour* MidiBehaviour::makeBehaviour(std::string type, Channel* c) {
	// NosuchDebug("MidiBehaviour::makeBehaviour type=%s chan=%d",type.c_str(),c->id);
	if ( type == "default" ) {
		return new MidiBehaviourDefault(c);
	} else if ( type == "horizontal" ) {
		return new MidiBehaviourHorizontal(c);
	} else if ( type == "vertical" ) {
		return new MidiBehaviourVertical(c);
	} else if ( type == "outline" ) {
		return new MidiBehaviourOutline(c);
	}
	NosuchDebug("Unrecognized midi behaviour name: %s",type.c_str());
	return new MidiBehaviourDefault(c);
}

void
MidiBehaviourOutline::NoteOn(int pitch, int velocity) {
	channel()->instantiateOutlines(pitch);
}

void
MidiBehaviourDefault::NoteOn(int pitch, int velocity) {
	double x = 0.5;
	double y = 0.5;
	double z = velocity / 127.0;
	channel()->instantiateSprite(pitch,velocity,NosuchVector(x,y),z);
}

double
val_for_pitch(int pitch, Channel* c) {
	pitch -= (int)(c->params.pitchoffset);
	pitch = (int)(pitch * c->params.pitchfactor);
	return fmod(pitch/127.0,1.0);
}

void
MidiBehaviourHorizontal::NoteOn(int pitch, int velocity) {
	double hvpos = channel()->params.hvpos;
	double x = val_for_pitch(pitch,channel());
	double y = hvpos;
	double z = velocity / 127.0;
	channel()->instantiateSprite(pitch,velocity,NosuchVector(x,y),z);
}

void
MidiBehaviourVertical::NoteOn(int pitch, int velocity) {
	double hvpos = channel()->params.hvpos;
	double x = hvpos;
	double y = val_for_pitch(pitch,channel());
	double z = velocity / 127.0;
	channel()->instantiateSprite(pitch,velocity,NosuchVector(x,y),z);
}