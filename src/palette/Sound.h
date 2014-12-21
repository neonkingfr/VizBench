#ifndef _SOUND_H
#define _SOUND_H

class MidiProgramChange;

#define NUM_SOUNDBANKS 8
#define NUM_SOUNDSETS 12
#define NUM_SOUNDS_IN_SET 4

extern std::string SoundBank[NUM_SOUNDBANKS][NUM_SOUNDSETS][NUM_SOUNDS_IN_SET];
extern int CurrentSoundSet;

class Sound {
public:
	Sound() { _synth = "UNINITIALIZED"; _patchnum = 0; }
	Sound(std::string synth, int patchnum);
	~Sound();
	static void initialize();
	std::string synth() { return _synth; }
	int patchnum() { return _patchnum; }
	static std::string nextSoundValue(int dir, std::string sound);
	static MidiProgramChange* ProgramChangeMsg(int ch, std::string sound);
private:
	std::string _synth;
	int _patchnum;   // program# on synth
	static bool initialized;
};

extern std::map<int,std::string> Synths;
extern std::map<std::string,Sound> Sounds;

#endif