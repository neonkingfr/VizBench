#include <stdarg.h>
#include "NosuchUtil.h"
#include "NosuchException.h"
#include "Scale.h"

bool Scale::initialized = false;
std::vector<std::string> Scale::scaleTypes;
std::map<std::string,Scale> Scale::Scales;

void Scale::initialize() {
	if ( initialized )
		return;
	initialized = true;
	NosuchDebug(1,"SCALE::INITIALIZE for newage, etc");
	Scales["fifths"] = Scale("fifths",0,7,-1);

	/*
	Some scales from Joe Lasqo:
	[2.1] Raga Mayamalavagaula
	Starting point and tonic drone: both C
	Scale sequence: C - D♭ - E - F - G - A♭ - B - C
	[2.2] Raga Kalyani / Lydian Mode starting on 7th
	Tonic drone: C
	Starting point (ideally): B
	Scale sequence (ideal): B - C - D - E - F♯ - G - A - B
	Alternative sacle sequence: C - D - E - F♯ - G - A - B - C
	[2.3] Raga Shreeranjani
	Starting point and tonic drone: both C
	Scale sequence: C - D - E♭ - F - A - B♭ - C
	(Note there is no fifth).
	[2.4] Pakistani Qawwal Raga
	Tonic drone: E♭
	Starting point (ideally): D
	Scale sequence (ideal): D - E♭- E - G - A - B♭ - B - D
	Alternative scale sequence: E♭- E - G - A - B♭ - B - D - E♭
	*/

	Scales["raga1"] = Scale("raga1",0,1,4,5,7,8,11,-1);
	Scales["raga2"] = Scale("raga2",0,2,4,6,7,9,11,-1);
	Scales["raga3"] = Scale("raga3",0,2,3,5,9,10,-1);
	Scales["raga4"] = Scale("raga4",0,1,4,6,7,8,11,-1);

	Scales["arabian"] = Scale("arabian",0,1,4,5,7,8,10,-1);
	Scales["newage"] = Scale("newage",0,3,5,7,10,-1);
	Scales["ionian"] = Scale("ionian",0,2,4,5,7,9,11,-1);
	Scales["dorian"] = Scale("dorian",0,2,3,5,7,9,10,-1);
	Scales["phrygian"] = Scale("phrygian",0,1,3,5,7,8,10,-1);
	Scales["lydian"] = Scale("lydian",0,2,4,6,7,9,11,-1);
	Scales["mixolydian"] = Scale("mixolydian",0,2,4,5,7,9,10,-1);
	Scales["aeolian"] = Scale("aeolian",0,2,3,5,7,8,10,-1);
	Scales["locrian"] = Scale("locrian",0,1,3,5,6,8,10,-1);
	Scales["octaves"] = Scale("octaves",0,-1);
	Scales["harminor"] = Scale("harminor",0,2,3,5,7,8,11,-1);
	Scales["melminor"] = Scale("melminor",0,2,3,5,7,9,11,-1);
	Scales["chromatic"] = Scale("chromatic",0,1,2,3,4,5,6,7,8,9,10,11,-1);

	for ( std::map<std::string,Scale>::const_iterator i = Scales.begin();
			i != Scales.end(); ++i){

		std::string key = (*i).first;
		scaleTypes.push_back(key);
	}
}

Scale::Scale() {
	clear();
}

Scale::Scale(std::string nm, int n1, ...) {
	_name = nm;
	clear();
	va_list ap;
	va_start(ap, n1); 
	int cnt = 0;
	for ( int i=n1; i >= 0; i = va_arg(ap, int)) {
		if ( cnt++ >= 13 ) {
			NosuchDebug("Scale::Scale TOO MANY ITEMS in intialization of Scale=%s!?",_name.c_str());
			break;
		}
		// NosuchDebug("Scale::Scale nm=%s LOOP i=%d",nm.c_str(),i);
		NosuchAssert(i<=12);
		i = i % 12;
		while ( i <= 127 ) {
			// NosuchDebug("Scale::Scale nm=%s setting _has_notes[%d] to true",nm.c_str(),i);
			_has_note[i] = true;
			i += 12;
		}
	}
	va_end(ap);
}

int Scale::closestTo(int pitch) {
	int closestpitch = -1;
	int closestdelta= 9999;
	int sz = sizeof(_has_note);
	for ( int i=0; i<sz; i++ ) {
		if ( ! _has_note[i] )
			continue;
		int delta = pitch - i;
		if ( delta < 0 )
			delta = -delta;
		if ( delta < closestdelta ) {
			closestdelta = delta;
			closestpitch = i;
		}
	}
	return closestpitch;
}

void Scale::clear() {
	int sz = sizeof(_has_note);
	for ( int i=0; i<sz; i++ ) {
		_has_note[i] = false;
	}
}