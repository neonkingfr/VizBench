#include "NosuchUtil.h"
#include <string>
#include <map>
#include <vector>
#include  <io.h>
#include <iostream>
#include <fstream>
#include "Sound.h"
#include "NosuchMidi.h"

std::map<int,std::string> Synths;  // maps channels to Synths

bool Sound::initialized = false;
std::map<std::string,Sound> Sounds;

int CurrentSoundSet = 0;

std::string SoundBank[NUM_SOUNDBANKS][NUM_SOUNDSETS][NUM_SOUNDS_IN_SET];

Sound::Sound(std::string synth, int patchnum) {
	_synth = synth;
	_patchnum = patchnum;
}

Sound::~Sound() {
}

void Sound::initialize() {
	if ( initialized )
		return;
	initialized = true;

}

std::string
Sound::nextSoundValue(int direction, std::string sound) {
	
	if ( sound == "UNSET" )
		sound = SoundBank[0][0][0];
	std::map<std::string,Sound>::iterator it = Sounds.find(sound);
	if ( it == Sounds.end() ) {
		NosuchDebug("Hey!  no sound=%s in Sound!?  returning same sound",sound.c_str());
		return sound;
	}
	int pnum = Sounds[sound].patchnum();
	std::string synth = Sounds[sound].synth();

	int npnum = pnum + direction;
	if ( npnum < 0 )
		npnum = 0;

	// Go through the sounds and find the same synth with the new patchnum
	it = Sounds.begin();
	NosuchDebug("Looking for sound for synth=%s patchnum=%d",synth.c_str(),npnum);
	for ( ; it!=Sounds.end(); it++ ) {
		std::string soundname = it->first;
		Sound s = it->second;
		if ( s.synth() == synth && s.patchnum() == npnum ) {
			NosuchDebug("Found sound!  name=%s",soundname.c_str());
			return soundname;
		}
	}
	NosuchErrorOutput("Unable to find next sound dir=%d sound=%s",direction,sound.c_str());
	return sound;

}

MidiProgramChange*
Sound::ProgramChangeMsg(int ch, std::string sound) {
	// Both the channel and patchnumber are 1-based,
	// but both are 0-based in the MIDI message is 0-based
	std::map<std::string,Sound>::iterator it = Sounds.find(sound);
	if ( it == Sounds.end() ) {
		NosuchDebug("Hey!  no sound=%s in Sound!?  returning NULL for ProgramChangeMsg",sound.c_str());
		return NULL;
	}
	int pnum = Sounds[sound].patchnum();
	MidiProgramChange* pc = MidiProgramChange::make(ch,pnum);
	return pc;
}