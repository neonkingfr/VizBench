#ifndef _MUSIC_BEHAVIOUR_H
#define _MUSIC_BEHAVIOUR_H

class MusicBehaviour : public Behaviour {

public:
	MusicBehaviour(Region* a);

	static void tonic_next(Palette* palette);
	static void tonic_reset(Palette* palette);
	static void tonic_set(Palette* palette, int t);

	void buttonDown(std::string bn);
	void buttonUp(std::string bn);
	void cursorDown(VizCursor* c);
	void cursorDrag(VizCursor* c);
	void cursorUp(VizCursor* c);
	void advanceTo(int tm);
	std::string name();

	bool isMyButton(std::string bn);

	int CurrentClick() {
		return paletteHost()->CurrentClick();
	}
	NosuchScheduler* scheduler() {
		return paletteHost()->scheduler();
	}

protected:
	int quantizeToNext(int clicks, int q);
	int VizCursor2Pitch(VizCursor* c);
	int VizCursor2Quant(VizCursor* c);
	void doNewNote(VizCursor* c);
	void doController(int ch, int ctrlr, int val, int sidnum, bool smooth);
	void doNewZController(VizCursor* c, int val, bool smooth);
	void setRegionSound(int rid, std::string nm);
	void changeSoundSet(int selected);
	int nextSoundSet();
	int prevSoundSet();
	int randSoundSet();
};


#endif