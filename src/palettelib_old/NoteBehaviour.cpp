#include "PaletteAll.h"

bool NoteBehaviour::initialized = false;

NoteBehaviour::NoteBehaviour(Region* r) {
	_region = r;
	_palette = _region->palette;
	_paletteHost = _palette->paletteHost();
}

Palette* NoteBehaviour::palette() {
	NosuchAssert(_palette);
	return _palette;
}

PaletteHost* NoteBehaviour::paletteHost() {
	NosuchAssert(_paletteHost);
	return _paletteHost;
}

Region* NoteBehaviour::region() {
	NosuchAssert(_region);
	return _region;
}

void NoteBehaviour::initialize() {
	if ( initialized )
		return;
	initialized = true;
}

NoteBehaviourDefault::NoteBehaviourDefault(Region* r) : NoteBehaviour(r) {
}

void NoteBehaviourDefault::gotMidiMsg(MidiMsg* mm, std::string sid) {
	if ( mm->MidiType() == MIDI_NOTE_ON ) {
		// NosuchDebug(1,"NOTE BEHAVIOUR!  sid=%s midi msg = %s",sid.c_str(),mm->DebugString().c_str());
	}
}
