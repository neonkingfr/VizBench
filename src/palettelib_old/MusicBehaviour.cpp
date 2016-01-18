#include <pthread.h>
#include <iostream>
#include <fstream>

#include  <io.h>
#include  <stdlib.h>

#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>

#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <vector>
#include <string>
#include <iostream>

#include <cstdlib> // for srand, rand

#include "PaletteAll.h"
#include "Scale.h"

extern int bn_to_selected(std::string bn);
extern int burn_bn_to_selected(std::string bn);

std::vector<std::string> MusicBehaviour::behaviourTypes;
std::vector<std::string> MusicBehaviour::controllerTypes;
// std::vector<std::string> MusicBehaviour::soundTypes;

bool MusicBehaviour::initialized = false;

static bool DoPitchBend = false;

static int ChordSet[8][4] = {
	{ 2,-1 },
	{ 3,-1 },
	{ 4,-1 },
	{ 5,-1 },
	{ 6,-1 },
	{ 7,-1 },
	{ 8,-1 },
	{ 9,-1 },
};
static int CurrentChord = 0;

MusicBehaviour::MusicBehaviour(Region* a) : Behaviour(a) {
	NosuchDebug(2,"MusicBehaviour CONSTRUCTOR! setting CurrentSoundSet to 0!");
	CurrentSoundSet = 0;
}
void MusicBehaviour::initialize() {

	if ( initialized )
		return;
	initialized = true;

	behaviourTypes.push_back("default");
	behaviourTypes.push_back("museum");
	behaviourTypes.push_back("STEIM");
	behaviourTypes.push_back("casual");
	behaviourTypes.push_back("burn");

	controllerTypes.push_back("modulationonly");
	controllerTypes.push_back("allcontrollers");
	controllerTypes.push_back("pitchYZ");

	CurrentChord = 3;

	// soundTypes.push_back("lead1");
	// soundTypes.push_back("lead2");
	// soundTypes.push_back("lead3");
}

MusicBehaviour* MusicBehaviour::makeBehaviour(std::string type, Region* a) {
	if ( type == "default" ) {
		return new MusicBehaviourDefault(a);
	}
	if ( type == "museum" ) {
		return new MusicBehaviourMuseum(a);
	}
	if ( type == "STEIM" ) {
		return new MusicBehaviourSTEIM(a);
	}
	if ( type == "casual" ) {
		return new MusicBehaviourCasual(a);
	}
	if ( type == "burn" ) {
		return new MusicBehaviourBurn(a);
	}
	if ( type == "2013" ) {
		return new MusicBehaviour2013(a);
	}
	NosuchDebug("Unrecognized music behaviour name: %s, assuming default",type.c_str());
	return new MusicBehaviourDefault(a);
}

///////////////////////////////////////////
/////////////// MusicBehaviourDefault
///////////////////////////////////////////

MusicBehaviourDefault::MusicBehaviourDefault(Region* a) : MusicBehaviour(a) {
}

bool MusicBehaviourDefault::isSelectorDown() {
	return (
		isButtonDown("LL1")  // graphics select
		|| isButtonDown("LL2")     // visual effect select
		|| isButtonDown("UL1")  // tonic select
		|| isButtonDown("UL2")  // scale select
		|| isButtonDown("LR1")  // ano on/off
		|| isButtonDown("LR2")  // tempo inc/dec
		|| isButtonDown("LR3")  // looping on/off
		|| isButtonDown("UR1")  // sound select
		|| isButtonDown("UR2")  // sound layout select
		|| isButtonDown("UR3")  // chording
		);
}

int MusicBehaviour::nextSoundSet() {
	int ss = (CurrentSoundSet+1)%8;
	NosuchDebug("nextSoundSet called, this=%d _current=%d new=%d",this,CurrentSoundSet,ss);
	return ss;
}
int MusicBehaviour::prevSoundSet() {
	int ss = (CurrentSoundSet-1+8)%8;
	NosuchDebug("prevSoundSet called, this=%d _current=%d new=%d",this,CurrentSoundSet,ss);
	return ss;
}

int MusicBehaviour::randSoundSet() {
	return rand() % 8;
}

void MusicBehaviour::changeSoundSet(int selected) {
	palette()->changeSoundSet(selected);
}

int MusicBehaviour::Cursor2Pitch(Cursor* c) {
	int mn;
	int mx;

	if ( region()->params.fullrange ) {
		mn = 10;
		mx = 120;
	} else {
		mn = region()->params.pitchmin;
		mx = region()->params.pitchmax;
	}
	int dp = mx - mn;
	if ( dp < 0 )
		dp = -dp;
	double x = c->curr_pos.x;
	int p = mn + (int)(dp * x);
	NosuchDebug(2,"Cursor2Pitch x=%.4f pitch=%d",x,p);
	return p;
}

int MusicBehaviour::Cursor2Quant(Cursor* c) {

	if ( ! palette()->params.doquantize ) {
		return 1;
	}

	int q;
	double y = -1;
	Region* r = region();

	if ( r->params.quantfixed ) {
		q = QuarterNoteClicks/4;
	} else {
		y = c->curr_pos.y;
		if ( y < r->params.timefret1y ) {
			q = (int) (QuarterNoteClicks * r->params.timefret1q);
			// NosuchDebug("fret1  y=%.3f q=%d",y,q);
		} else if ( y < r->params.timefret2y ) {
			q = (int) (QuarterNoteClicks * r->params.timefret2q);
			// NosuchDebug("fret2  y=%.3f q=%d",y,q);
		} else if ( y < r->params.timefret3y ) {
			q = (int) (QuarterNoteClicks * r->params.timefret3q);
			// NosuchDebug("fret3  y=%.3f q=%d",y,q);
		} else if ( y < r->params.timefret4y ) {
			q = (int) (QuarterNoteClicks * r->params.timefret4q);
			// NosuchDebug("fret4  y=%.3f q=%d",y,q);
		} else {
			q = 1;
			// NosuchDebug("fret5  y=%.3f q=%d",y,q);
		}
		// NosuchDebug("y=%.3f q=%d timefret1234 = %.3f %.3f %.3f %.3f", y,q, r->params.timefret1y, r->params.timefret2y, r->params.timefret3y, r->params.timefret4y);
	}
	double qfactor = r->params.quantfactor;
	q = (int)(qfactor*q);
	if ( q < 1 )
		q = 1;
	NosuchDebug(2,"CURSOR2QUANT y=%.4f q=%d",y,q);
	return q;
}

void
MusicBehaviour::setRegionSound(int rid, std::string nm)
{
	palette()->setRegionSound(rid,nm);
}

void MusicBehaviourDefault::cursorDownWithSelector(Cursor *c) {
	if ( palette()->isButtonDown("LR1") ) {
		NosuchDebug("Clearing loop for region=%d",region()->id);
		region()->loop()->Clear();
		return;
	}
	if ( palette()->isButtonDown("LR2") ) {
		int selected = SelectionNumber(c);
		int clkpersec = NosuchScheduler::GetClicksPerSecond();
		switch (selected) {
		case 0: clkpersec -= 10; break;
		case 1: clkpersec += 10; break;
		case 2: clkpersec = 192*1/2; break;
		case 3: clkpersec = 192*2/3; break;
		case 4: clkpersec = 192*3/4; break;
		case 5: clkpersec = 192*1; break;
		case 6: clkpersec = 192*4/3; break;
		case 7: clkpersec = 192*3/2; break;
		}
		NosuchScheduler::SetClicksPerSecond(clkpersec);
		NosuchDebug("ClicksPerSecond is now %d",clkpersec);
		return;
	}
	if ( palette()->isButtonDown("UR3") ) {
		int selected = SelectionNumber(c);
		if ( palette()->isShifted() ) {
			NosuchDebug("SELECTING Chord %d",selected);
			CurrentChord = selected;
			return;
		}
		bool b = c->isRightSide();
		region()->Chording(b);
		NosuchDebug(2,"SETTING CHORDING=%d for region=%d",b,region()->id);
		return;
	}
	if ( palette()->isButtonDown("LR3") ) {
		if ( c->isRightSide() ) {
			// Turn looping on
			if ( region()->Looping() ) {
				// If we're already looping, make the fade out longer
				double fade = region()->params.loopfade;
				fade = 1.0f - ((1.0f-fade)/2.0f);
				if ( fade >= 0.89 ) {
					fade = 1.0;
				}
				region()->params.loopfade = fade;
				NosuchDebug("Loop Fade value is now %.4f",fade);
			} else {
				region()->Looping(true);
				region()->params.loopfade = DEFAULT_LOOPFADE;
			}
		} else {
			// Turn looping off
			if ( ! region()->Looping() ) {
				// If we're already off, clear the existing loop
				region()->loop()->Clear();
			} else {
				// Turn off looping (but leave the existing loop)
				region()->Looping(false);
			}
		}
		NosuchDebug("Region=%d looping=%d loopfade=%.4f",
			region()->id,region()->Looping(),region()->params.loopfade);
		return;
	}
	if ( palette()->isButtonDown("UR1") ) {
		std::string sound = region()->params.sound;
		NosuchDebug(2,"SOUND BUTTON DOWN! existing sound=%s",sound.c_str());
		if ( sound == "UNSET" ) {
			region()->params.sound = "pads";
		}
		int direction = ( c->isRightSide() ? 1 : -1 );
		std::string ns = Sound::nextSoundValue(direction, sound);
		NosuchDebug(1,"    new sound=%s",ns.c_str());
		region()->params.sound = ns;
		region()->UpdateSound();
		return;
	}
	if ( palette()->isButtonDown("UL2") ) {
		std::string musicscale = palette()->params.musicscale;
		int selected = SelectionNumber(c);
		NosuchDebug("SCALE BUTTON DOWN! existing musicscale=%s selected=%d",
			musicscale.c_str(),selected);
		std::string s = "";
		switch (selected) {
		case 0: s = "fifths"; break;
		case 1: s = "octaves"; break;
		case 2: s = "raga1"; break;
		case 3: s = "raga2"; break;
		case 4: s = "raga3"; break;
		case 5: s = "arabian"; break;
		case 6: s = "newage"; break;
		case 7: s = "chromatic"; break;
		default:
			NosuchDebug("HEY! Selection Number %d unexpected! Assuming 0",selected);
			s = "raga1";
			break;
		}
		NosuchDebug("NEW SCALE = %s",s.c_str());
		palette()->params.musicscale = s;
		palette()->ClearAllLoops(false);  // don't turn looping off (debatable)
		scheduler()->ANO();
		return;
	}
	if ( palette()->isButtonDown("UL1") ) {

		int tonic = palette()->params.tonic;
		int selected = SelectionNumber(c);
		int k = 0;
		switch (selected) {
		case 0: k = -7; break;
		case 1: k = 7; break;
		case 2: k = -5; break;
		case 3: k = -3; break;
		case 4: k = 3; break;
		case 5: k = 5; break;
		case 6: k = -1; break;
		case 7: k = 1; break;
		default:
			NosuchDebug("HEY! Selection Number %d unexpected! Assuming 0",selected);
			k = 0;
			break;
		}
		tonic += k;
		palette()->params.tonic = tonic;
		NosuchDebug("TONIC inc/dec=%d is now %d",k,tonic);
		GlobalPitchOffset = tonic;

		return;
	}
	if ( palette()->isButtonDown("UR2") ) {

		int selected = SelectionNumber(c);
		if ( palette()->isShifted() ) {
			NosuchDebug("SELECTING SOUND BANK %d",selected);
			palette()->soundBank(selected);
			return;
		}
		changeSoundSet(selected);
		return;
	}
}

void MusicBehaviourDefault::cursorDragWithSelector(Cursor *c) {
}

void MusicBehaviourDefault::cursorUpWithSelector(Cursor *c) {
}

void MusicBehaviourDefault::cursorDown(Cursor* c) {
	doNewNote(c);
	// XXX - should probably do controller here if z > zmin
}

void MusicBehaviourDefault::cursorDrag(Cursor* c) {
	double dist = c->curr_pos.sub(c->previous_musicpos()).mag();
	double ddist = c->curr_depth - c->last_depth();
	if ( ddist < 0 )
		ddist = -ddist;
	NosuchDebug(1,"cursorDrag dist=%.4f ddist=%.4f c->d=%.3f c->prevd=%.3f",
		dist,ddist,c->curr_depth,c->last_depth());
	if ( dist >= region()->params.minmove || ddist >= region()->params.minmovedepth ) {
		NosuchDebug(2,"MUSIC::CURSORDRAG dist=%.3f",dist);
		doNewNote(c);
	}
	double z = region()->MaxCursorDepth();   // was: c->depth();
	double zmin = region()->params.controllerzmin;
	double zmax = region()->params.controllerzmax;

	std::string cstyle = region()->params.controllerstyle;
	if ( cstyle == "modulationonly" ) {
		if ( z > zmin ) {
			double zz = (z>zmax)?zmax:z;
			double dz = (zz-zmin) / (zmax-zmin);
			int cval = (int)(dz*128.0);
			if ( cval > 127 )
				cval = 127;
			doNewZController(c,cval,true);
		}
	} else if ( cstyle == "allcontrollers" ) {
		// doNewXController(c,cval,true);
		int ch = region()->params.controllerchan;
		int xctrl = region()->params.xcontroller;
		int yctrl = region()->params.ycontroller;
		int zctrl = region()->params.zcontroller;

		double zz = (z>zmax)?zmax:z;
		double dz = zz / zmax;
		int zval = (int)(dz*128.0);

		NosuchVector v = c->curr_pos;
		int xval = (int)(v.x*128.0) % 128;
		int yval = (int)(v.y*128.0) % 128;

		NosuchDebug("ALLCONTROLLERS drag x=%.3f y=%.3f z=%.3f",v.x,v.y,z);
		NosuchDebug("ALLCONTROLLERS vals x=%d y=%d z=%d",xval,yval,zval);
		doController(ch,xctrl,xval,c->sidnum(),true);
		doController(ch,yctrl,yval,c->sidnum(),true);
		doController(ch,zctrl,zval,c->sidnum(),true);
	} else if ( cstyle == "pitchYZ" ) {
		// doNewXController(c,cval,true);
		// int ch = region()->params.controllerchan;
		int ch = region()->channel();
		int yctrl = region()->params.ycontroller;
		int zctrl = region()->params.zcontroller;

		double zz = (z>zmax)?zmax:z;
		double dz = zz / zmax;
		int zval = (int)(dz*128.0);

		NosuchVector v = c->curr_pos;
		int yval = (int)(v.y*128.0) % 128;

		NosuchDebug(1,"pitchYZ drag y=%.3f z=%.3f",v.y,z);
		NosuchDebug(1,"pitchYZ vals y=%d z=%d",yval,zval);
		doController(ch,yctrl,yval,c->sidnum(),true);
		doController(ch,zctrl,zval,c->sidnum(),true);
	} else {
		NosuchDebug("UNRECOGNIZED value for controllerstyle: %s",cstyle.c_str());
	}
}

void MusicBehaviourDefault::cursorUp(Cursor* c) {

	std::string cstyle = region()->params.controllerstyle;

	NosuchDebug(2,"MusicBehaviourDefault::cursorUp ! c->sid=%d",c->sidnum());

	if ( DoPitchBend ) {
		region()->setNotesDisabled(false);
	}

	if ( cstyle == "allcontrollers" ) {
		int ch = region()->params.controllerchan;
		int zctrl = region()->params.zcontroller;
		doController(ch,zctrl,0,c->sidnum(),false);  // no smoothing
		return;
	}
	if ( cstyle == "pitchYZ" ) {
		// int ch = region()->params.controllerchan;
		int ch = region()->channel();
		int zctrl = region()->params.zcontroller;
		int yctrl = region()->params.ycontroller;
		doController(ch,zctrl,0,c->sidnum(),false);  // no smoothing
		doController(ch,yctrl,0,c->sidnum(),false);  // no smoothing
		// continue on to do note
	}


	NosuchAssert(cstyle=="modulationonly"||cstyle=="pitchYZ");

	std::vector<int>& pitches = c->lastpitches();
	if ( pitches.size() == 0 ) {
		return;
	}
	int ch = c->lastchannel();
	int clk = c->lastclick();
	for ( size_t i=0; i < pitches.size(); i++ ) {
		int pitch = pitches[i];
		if ( pitch >= 0 ) {
			NosuchDebug(1,"cursorUp sending Noteoff for pitch=%d",pitch);
			// Normally, the getLastClick() will be quantized into the future,
			// so make sure we don't try to play the noteoff before that.
			int cc = paletteHost()->CurrentClick();
			if ( clk < cc ) {
				clk = cc;
			}
			scheduler()->IncomingNoteOff(clk,ch,pitch,0,c->sidnum());
		}
	}
	c->clear_last_note();

	// This was an attempt to try to do without the lastpitches stuff.  Failed.
	// I think it was because I didn't do the logic described above, where
	// it makes sure the noteoffs don't get played before the actual note
	// scheduler()->SendNoteOffsForNowPlaying(c->sid());

	// Reset the modulation controller to 0
	doNewZController(c,0,false);  // no smoothing

	// doNewPitchBend(0.0f, true, smooth) {
}

int MusicBehaviour::quantizeToNext(int clicks, int quant) {
	return clicks - (clicks%quant) + quant;
}

void MusicBehaviour::doController(int ch, int ctrlr, int val, int sidnum, bool smooth) {
	if ( DoControllers ) {
		MidiMsg* m = MidiController::make(ch,ctrlr,val);
		scheduler()->SendControllerMsg(m,sidnum,smooth);
		// Don't delete m, SendControllerMsg takes ownership of it
	}
}

void MusicBehaviour::doNewZController(Cursor* c, int val, bool smooth) {
	doController(region()->channel(),0x01,val,c->sidnum(),smooth);
}

void MusicBehaviour::doNewPitchBend(double depth, bool up, bool smooth) {
	if ( DoPitchBend == false ) {
		return;
	}
	double pb_depth_min = 0.05f;
	double pb_depth_max = 0.10f;
	if ( depth < pb_depth_min ) {
		depth = pb_depth_min;
	}
	if ( depth > pb_depth_max ) {
		depth = pb_depth_max;
	}
	double bend = (depth - pb_depth_min) / ( pb_depth_max - pb_depth_min );
	if ( up == false ) {
		bend = -bend;
	}
	// At this point, bend is -1.0 to 1.0, and we want it to be 0.0 to 1.0
	bend = (bend + 1.0f) / 2.0f;
	Cursor* c = palette()->MostRecentCursorDown();
	if ( c == NULL ) {
		NosuchDebug("doNewPitchBend, MostRecentCursor is NULL?");
		return;
	}
	Region* r = c->region();
	int ch = r->channel();

	if ( bend == 0.5 ) {
		NosuchDebug(1,"calling r->disableNotes false");
		r->setNotesDisabled(false);
	} else {
		NosuchDebug(1,"calling r->disableNotes true");
		r->setNotesDisabled(true);
	}
	int val = 8192;
	val += (int)(((bend-0.5f)*2.0f) * 8191);
#if 0
	if ( bend > 0.5f ) {
		val += (int)(((bend-0.5f)*2.0f) * 8191);
	} else if ( bend < 0.5f ) {
		val -= (int)(((bend-0.5f)*2.0f) * 8192);
	}
#endif

	NosuchDebug(1,"doNewPitchBend, depth=%f  bend=%f val=%d recent ch=%d",depth,bend,val,ch);
	 MidiMsg* m = MidiPitchBend::make(r->channel(),val);
	 scheduler()->SendPitchBendMsg(m,c->sidnum(),smooth);
	// Don't delete m, SendControllerMsg takes ownership of it
}

void MusicBehaviour::doNewNote(Cursor* c) {
	Region* r = c->region();
	if ( DoPitchBend && r->NotesDisabled() ) {
		NosuchDebug("doNewNote is NOT adding a note due to NotesDisabled");
		return;
	}
	if ( r->params.controllerstyle == "allcontrollers" ) {
		NosuchDebug("doNewNote is NOT adding a note due to allcontrollers mode");
		return;
	}
	if ( NosuchDebugMidiNotes ) {
		NosuchDebug("NEWNOTE! region id=%d  chan=%d",r->id,r->channel());
	}
	if ( region()->channel() < 0 ) {
		// May never happen, or maybe just on the first call?
		NosuchDebug("Is this code in doNewNote ever reached!?");
		region()->UpdateSound();
	}

	int ch = region()->channel();
	if ( ch <= 0 ) {
		NosuchDebug("NOT SENDING MIDI NOTE!  ch=%d",ch);
		return;
	}

	int clk = CurrentClick();

	int qnt = Cursor2Quant(c);
	int qclk = quantizeToNext(clk,qnt);

	// Should the scheduler be locked, here?

#if 0
	int nsched = region()->NumberScheduled(-1,-1,c->sid());

	NosuchDebug(1,"NEW NOTE clk=%d qclk=%d y=%.4f q=%d nsched=%d",clk,qclk,c->curr_pos.y,qnt,nsched);
#endif

	std::string sn = palette()->params.musicscale;
	Scale& scl = Scale::Scales[sn];
	int pitch = Cursor2Pitch(c);

	// XXX There should really be a "isdrums" or "dontscale" value
	// rather than depending on the actual soundset name, I think
	std::string ssval = region()->params.sound;
	int npitch = pitch;
	if ( ssval != "Beatbox" ) {
		npitch = scl.closestTo(pitch);
	}
	if ( NosuchDebugMidiNotes ) {
		NosuchDebug(1,"DONEWNOTE, scale=%s origpitch=%d newpitch=%d",
			sn.c_str(),pitch,npitch);
	}

	if ( npitch < 0 ) {
		throw NosuchException("Unable to find closest pitch to %d in scale=%s",pitch,sn.c_str());
	}

	if ( region()->params.arpeggio ) {
		NosuchDebug("Doing Arpeggio in doNewNote");
	// if ( palette()->isButtonDown("LL3") ) {
		// We want to "arpeggiate", which means we want the closest note to
		// the last note played by this cursor
		int lastnote = c->last_pitch();
		if ( lastnote >= 0 && npitch != lastnote ) {
			NosuchDebug(1,"pitch=%d  npitch=%d   lastnote=%d",pitch,npitch,lastnote);
			int dir = (npitch>lastnote)?1:-1;
			int nextnote = npitch;
			for ( int apitch=lastnote+dir; ; apitch+=dir ) {
				if ( apitch == npitch ) {
					// Oh well, the new pitch was already the closest
					break;
				}
				int ap = scl.closestTo(apitch);
				if ( ap != lastnote ) {
					nextnote = ap;
					break;
				}
			}
			NosuchDebug(1,"    nextnote=%d",nextnote);
			npitch = nextnote;
		}
	}

	int velocity = 127;  // Velocity should be based on something else
	MidiMsg* m1 = MidiNoteOn::make(ch,npitch,velocity);

	// c->add_last_note(qclk,ch,npitch);

	if ( ssval != "Beatbox" && region()->Chording() && CurrentChord != 0 ) {
		// NosuchDebug(1,"DONEWNOTE CHORDING!  Chord=%d first m1=%s",CurrentChord,m1->DebugString().c_str());

		int dp;
		int base = npitch;
		MidiMsg *currentm = m1;
		for ( int i=0; (dp=ChordSet[CurrentChord][i]) > 0; i++ ) {
			int p1 = base + dp;
			int p2 = scl.closestTo(p1);
			NosuchDebug(2,"chordpitch p1=%s p2=%s",ReadableMidiPitch(p1),ReadableMidiPitch(p2));
			MidiMsg* m2 = MidiNoteOn::make(ch,p2,velocity);
			currentm->next = m2;
			currentm = m2;
		}
		// NosuchDebug(1,"CHORD MidiMsg = %s",m1->DebugString().c_str());
	}

	scheduler()->IncomingMidiMsg(m1,qclk,c->sidnum());

	c->clear_last_note();
	c->add_last_note(qclk,m1);

	c->set_previous_musicpos(c->curr_pos);
	c->set_last_depth(c->curr_depth);
}

void MusicBehaviourDefault::advanceTo(int tm) {
	// NosuchDebug("Music::periodicFire called");
}

void MusicBehaviourDefault::buttonDown(std::string bn) {
	if ( bn=="LL3" || bn=="LL2" || bn=="LL1" ) {
		if ( palette()->isShiftDown() ) {
			palette()->setShifted(true);
		}
	} else if ( bn=="UL1") {
		// TONIC
		static int last_tonic = -1;
		if ( last_tonic>0 && (Palette::now - last_tonic)<1500 ) {
			tonic_reset(palette());
		}
		last_tonic = Palette::now;
	} else if ( bn=="UL2" ) {
		; // This is SCALE
	} else if ( bn=="UL3" ) {
		// This is the SHIFT button
	} else if ( bn=="LR1" ) {
		// If we're holding down the Looping button, then ANO will 
		// clear all loops and turn looping off
		if ( palette()->isButtonDown("LR3") ) {
			NosuchDebug("CLEARING ALL LOOPS and turning looping OFF!");
			palette()->ClearAllLoops(true);
		}
		scheduler()->ANO();
	} else if ( bn=="LR2" ) {
	} else if ( bn=="LR3" ) {
		// see above
		if ( palette()->isButtonDown("LR1") ) {
			NosuchDebug("CLEARING ALL LOOPS and turning looping OFF!");
			palette()->ClearAllLoops(true);
		}
		scheduler()->ANO();
	} else if ( bn=="UR1" ) {
	} else if ( bn=="UR2" ) {
		if ( palette()->isShiftDown() ) {
			palette()->setShifted(true);
		}
	} else if ( bn=="UR3" ) {
		// CHORDING
		if ( palette()->isShiftDown() ) {
			palette()->setShifted(true);
		}
	} else {
		NosuchDebug("Button %d doesn't control anything in MusicBehaviourDefault",bn);
	}
}

void MusicBehaviourDefault::buttonUp(std::string bn) {
	NosuchDebug(1,"MusicBehaviourDefault::buttonUp bn=%d",bn);
	if ( bn != "UL3" ) {
		palette()->setShifted(false);
	}
}

MusicBehaviourMuseum::MusicBehaviourMuseum(Region* a) : MusicBehaviourDefault(a) {
}

bool MusicBehaviourMuseum::isSelectorDown() {
	return (
		isButtonDown("UR3")
		);
}

void MusicBehaviourMuseum::buttonUp(std::string bn) {
	NosuchDebug(1,"MusicBehaviourMuseum::buttonUp bn=%d",bn);
	if ( bn=="LR3" ) {
		NosuchDebug("Looping OFF");
		palette()->SetAllLooping(false,-1);
	} else if ( bn=="UR3" ) {
		if ( ! palette()->isButtonUsed(bn) ) {
			NosuchDebug("SETTING A RANDOM SOUNDSET");
			int newset = CurrentSoundSet;
			while ( newset == CurrentSoundSet ) {
				newset = randSoundSet();
			}
			changeSoundSet(newset);
		}
	}
}

void
MusicBehaviour::tonic_reset(Palette* palette) {
	NosuchDebug("TONIC reset!!");
	palette->paletteHost()->scheduler()->ANO();
	palette->params.tonic = 0;
	GlobalPitchOffset = 0;
}

void
MusicBehaviour::tonic_change(Palette* palette) {

	palette->paletteHost()->scheduler()->ANO();

	int tonic = palette->params.tonic;
	switch(tonic){
	case 0: tonic = 5; break;
	case 5: tonic = 3; break;
	case 3: tonic = -4; break;
	case -4: tonic = 0; break;
	default: tonic = 0; break;
	}
	palette->params.tonic = tonic;
	NosuchDebug(1,"TONIC is now %d",tonic);
	GlobalPitchOffset = tonic;
}

void MusicBehaviourMuseum::buttonDown(std::string bn) {
	NosuchDebug("MUSEUM MUSIC buttonDown bn=%d",bn);
	palette()->setButtonUsed(bn,false);
	if ( bn=="LR1" ) {
		NosuchDebug("CLEAR ALL");
		palette()->SetAllLooping(false,DEFAULT_LOOPFADE);
		palette()->ClearAllLoops(true);
		scheduler()->ANO();
	} else if ( bn=="LR2" ) {
		NosuchDebug(1,"Change tonic");
		tonic_change(palette());
	} else if ( bn=="LR3" ) {
		NosuchDebug("Looping ON");
		palette()->SetAllLooping(true,DEFAULT_LOOPFADE);
	} else if ( bn=="UR1" ) {
		NosuchDebug(1,"PREV soundset");
		changeSoundSet(prevSoundSet());
	} else if ( bn=="UR2" ) {
		NosuchDebug(1,"NEXT soundset");
		changeSoundSet(nextSoundSet());
	} else if ( bn=="UR3" ) {
		NosuchDebug(1,"RAND soundset (but wait for button up)");
	}
}

void MusicBehaviourMuseum::cursorDownWithSelector(Cursor *c) {
	if ( palette()->isButtonDown("UR3") ) {

		int selected = SelectionNumber(c);
		NosuchDebug("SELECTING SOUND LAYOUT %d",selected);

		changeSoundSet(selected);

		palette()->setButtonUsed("UR3",true);
		return;
	}
}

//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------

MusicBehaviourSTEIM::MusicBehaviourSTEIM(Region* a) : MusicBehaviourDefault(a) {
}

bool MusicBehaviourSTEIM::isSelectorDown() {
	return (
		isButtonDown("LL2")     // Misc select
		|| isButtonDown("UL1")  // scale select
		|| isButtonDown("UL2")  // tonic select
		|| isButtonDown("UL3")  // range select
		|| isButtonDown("LR1")  // ano on/off
		|| isButtonDown("LR2")  // tempo inc/dec
		|| isButtonDown("LR3")  // looping on/off per region
		|| isButtonDown("UR3")  // sound select
		);
}

void MusicBehaviourSTEIM::cursorDownWithSelector(Cursor *c) {

	if ( palette()->isButtonDown("LR1") ) {
		NosuchDebug("Clearing loop for region=%d",region()->id);
		region()->loop()->Clear();
		return;
	}
	if ( palette()->isButtonDown("LR2") ) {
		int selected = SelectionNumber(c);
		int clkpersec = NosuchScheduler::GetClicksPerSecond();
		switch (selected) {
		case 0: clkpersec -= 10; break;
		case 1: clkpersec += 10; break;
		case 2: clkpersec = 192*1/2; break;
		case 3: clkpersec = 192*2/3; break;
		case 4: clkpersec = 192*3/4; break;
		case 5: clkpersec = 192*1; break;
		case 6: clkpersec = 192*4/3; break;
		case 7: clkpersec = 192*3/2; break;
		}
		NosuchScheduler::SetClicksPerSecond(clkpersec);
		NosuchDebug("ClicksPerSecond is now %d",clkpersec);
		return;
	}
	if ( palette()->isButtonDown("LR3") ) {
		NosuchDebug("STEIM looping selector!");
		if ( c->isRightSide() ) {
			// Turn looping on
			if ( region()->Looping() ) {
				NosuchDebug("NOT making Loop Fade any longer");
#if 0
				// If we're already looping, make the fade out longer
				double fade = region()->params.loopfade;
				fade = 1.0f - ((1.0f-fade)/2.0f);
				if ( fade >= 0.89 ) {
					fade = 1.0;
				}
				region()->params.loopfade = fade;
				NosuchDebug("Loop Fade value is now %.4f",fade);
#endif
			} else {
				region()->Looping(true);
				region()->params.loopfade = DEFAULT_LOOPFADE;
			}
		} else {
			// Turn looping off
			if ( ! region()->Looping() ) {
				// If we're already off, clear the existing loop
				region()->loop()->Clear();
			} else {
				// Turn off looping (but leave the existing loop)
				region()->Looping(false);
			}
		}
		NosuchDebug("Region=%d looping=%d loopfade=%.4f",
			region()->id,region()->Looping(),region()->params.loopfade);
		return;
	}
	if ( palette()->isButtonDown("UL3") ) {
		if ( c->isRightSide() ) {
			NosuchDebug("Should be EXPANDING range for region %d",c->region()->id);
		} else {
			NosuchDebug("Should be REDUCING range for region %d",c->region()->id);
		}
		return;
	}
	if ( palette()->isButtonDown("UL1") ) {
		int selected = SelectionNumber(c);
		NosuchDebug("SCALE BUTTON DOWN! existing scale=%s selected=%d",
			palette()->params.musicscale.c_str(),selected);
		std::string s = "";
		switch (selected) {
		case 0: s = "fifths"; break;
		case 1: s = "octaves"; break;
		case 2: s = "raga1"; break;
		case 3: s = "raga2"; break;
		case 4: s = "raga3"; break;
		case 5: s = "arabian"; break;
		case 6: s = "newage"; break;
		case 7: s = "chromatic"; break;
		default:
			NosuchDebug("HEY! Selection Number %d unexpected! Assuming 0",selected);
			s = "raga1";
			break;
		}
		NosuchDebug("NEW SCALE = %s",s.c_str());
		palette()->params.musicscale = s;
		palette()->ClearAllLoops(false);  // don't turn looping off (debatable)
		scheduler()->ANO();
		return;
	}
	if ( palette()->isButtonDown("UL2") ) {
		int tonic = palette()->params.tonic;
		int selected = SelectionNumber(c);
		int k = 0;
		switch (selected) {
		case 0: k = -7; break;
		case 1: k = 7; break;
		case 2: k = -5; break;
		case 3: k = -3; break;
		case 4: k = 3; break;
		case 5: k = 5; break;
		case 6: k = -1; break;
		case 7: k = 1; break;
		default:
			NosuchDebug("HEY! Selection Number %d unexpected! Assuming 0",selected);
			k = 0;
			break;
		}
		tonic += k;
		palette()->params.tonic = tonic;
		NosuchDebug("TONIC inc/dec=%d is now %d",k,tonic);
		GlobalPitchOffset = tonic;
		palette()->setButtonUsed("UL2",true);
		return;
	}
	if ( palette()->isButtonDown("UR3") ) {
		int selected = SelectionNumber(c);
		changeSoundSet(selected);
		palette()->setButtonUsed("UR3",true);
		return;
	}
	if ( palette()->isButtonDown("LL2") ) {
		int selected = SelectionNumber(c);
		NosuchDebug("MISC select = %d",selected);
		switch (selected) {
		case 0:
			NosuchDebug("Arpeggio off!");
			palette()->SetAllArpeggio(false);
			break;
		case 1:
			NosuchDebug("Arpeggio on!");
			palette()->SetAllArpeggio(true);
			break;
		case 2:
			NosuchDebug("QUANTIZE OFF!");
			palette()->params.doquantize = false;
			break;
		case 3:
			NosuchDebug("QUANTIZE ON!");
			palette()->params.doquantize = true;
			break;
		case 4:
			NosuchDebug("minmove is set to 0.0  (DISABLED!)");
			// palette()->params.minmove = 0.0;
			break;
		case 5:
			NosuchDebug("minmove is set to 0.05  (DISABLED)");
			// palette()->params.minmove = 0.05f;
			break;
		case 6:
			NosuchDebug("FULL RANGE OFF!");
			palette()->SetAllFullRange(false);
			break;
		case 7:
			NosuchDebug("FULL RANGE ON!");
			palette()->SetAllFullRange(true);
			break;
		}
		return;
	}
}

void MusicBehaviourSTEIM::cursorDragWithSelector(Cursor *c) {
}

void MusicBehaviourSTEIM::cursorUpWithSelector(Cursor *c) {
}

void MusicBehaviourSTEIM::cursorDown(Cursor* c) {
	doNewNote(c);
	// XXX - should probably do controller here if z > zmin
}

void MusicBehaviourSTEIM::cursorDrag(Cursor* c) {
	double dist = c->curr_pos.sub(c->previous_musicpos()).mag();
	double ddist = c->curr_depth - c->last_depth();
	if ( ddist < 0 )
		ddist = -ddist;
	NosuchDebug(1,"cursorDrag dist=%.4f ddist=%.4f c->d=%.3f c->prevd=%.3f",
		dist,ddist,c->curr_depth,c->last_depth());
	if ( dist >= region()->params.minmove || ddist >= region()->params.minmovedepth ) {
		NosuchDebug(2,"MUSIC::CURSORDRAG dist=%.3f",dist);
		doNewNote(c);
	}
	double z = region()->MaxCursorDepth();   // was: c->depth();
	double zmin = region()->params.controllerzmin;
	double zmax = region()->params.controllerzmax;

	std::string cstyle = region()->params.controllerstyle;
	if ( cstyle == "modulationonly" ) {
		if ( z > zmin ) {
			double zz = (z>zmax)?zmax:z;
			double dz = (zz-zmin) / (zmax-zmin);
			int cval = (int)(dz*128.0);
			if ( cval > 127 )
				cval = 127;
			doNewZController(c,cval,true);
		}
	} else if ( cstyle == "allcontrollers" ) {
		// doNewXController(c,cval,true);
		int ch = region()->params.controllerchan;
		int xctrl = region()->params.xcontroller;
		int yctrl = region()->params.ycontroller;
		int zctrl = region()->params.zcontroller;

		double zz = (z>zmax)?zmax:z;
		double dz = zz / zmax;
		int zval = (int)(dz*128.0);

		NosuchVector v = c->curr_pos;
		int xval = (int)(v.x*128.0) % 128;
		int yval = (int)(v.y*128.0) % 128;

		NosuchDebug("ALLCONTROLLERS drag x=%.3f y=%.3f z=%.3f",v.x,v.y,z);
		NosuchDebug("ALLCONTROLLERS vals x=%d y=%d z=%d",xval,yval,zval);
		doController(ch,xctrl,xval,c->sidnum(),true);
		doController(ch,yctrl,yval,c->sidnum(),true);
		doController(ch,zctrl,zval,c->sidnum(),true);
	} else if ( cstyle == "pitchYZ" ) {
		// doNewXController(c,cval,true);
		// int ch = region()->params.controllerchan;
		int ch = region()->channel();
		int yctrl = region()->params.ycontroller;
		int zctrl = region()->params.zcontroller;

		double zz = (z>zmax)?zmax:z;
		double dz = zz / zmax;
		int zval = (int)(dz*128.0);

		NosuchVector v = c->curr_pos;
		int yval = (int)(v.y*128.0) % 128;

		NosuchDebug(1,"pitchYZ drag y=%.3f z=%.3f",v.y,z);
		NosuchDebug(1,"pitchYZ vals y=%d z=%d",yval,zval);
		doController(ch,yctrl,yval,c->sidnum(),true);
		doController(ch,zctrl,zval,c->sidnum(),true);
	} else {
		NosuchDebug("UNRECOGNIZED value for controllerstyle: %s",cstyle.c_str());
	}
}

void MusicBehaviourSTEIM::cursorUp(Cursor* c) {

	std::string cstyle = region()->params.controllerstyle;

	NosuchDebug(2,"MusicBehaviourSTEIM::cursorUp ! c->sid=%d",c->sidnum());

	if ( DoPitchBend ) {
		region()->setNotesDisabled(false);
	}

	if ( cstyle == "allcontrollers" ) {
		int ch = region()->params.controllerchan;
		int zctrl = region()->params.zcontroller;
		doController(ch,zctrl,0,c->sidnum(),false);  // no smoothing
		return;
	}
	if ( cstyle == "pitchYZ" ) {
		// int ch = region()->params.controllerchan;
		int ch = region()->channel();
		int zctrl = region()->params.zcontroller;
		int yctrl = region()->params.ycontroller;
		doController(ch,zctrl,0,c->sidnum(),false);  // no smoothing
		doController(ch,yctrl,0,c->sidnum(),false);  // no smoothing
		// continue on, to do note
	}

	NosuchAssert(cstyle=="modulationonly"||cstyle=="pitchYZ");

	std::vector<int>& pitches = c->lastpitches();
	if ( pitches.size() == 0 ) {
		return;
	}
	int ch = c->lastchannel();
	int clk = c->lastclick();
	for ( size_t i=0; i < pitches.size(); i++ ) {
		int pitch = pitches[i];
		if ( pitch >= 0 ) {
			NosuchDebug(1,"cursorUp sending Noteoff for pitch=%d",pitch);
			// Normally, the getLastClick() will be quantized into the future,
			// so make sure we don't try to play the noteoff before that.
			int cc = paletteHost()->CurrentClick();
			if ( clk < cc ) {
				clk = cc;
			}
			scheduler()->IncomingNoteOff(clk,ch,pitch,0,c->sidnum());
		}
	}
	c->clear_last_note();

	// This was an attempt to try to do without the lastpitches stuff.  Failed.
	// I think it was because I didn't do the logic described above, where
	// it makes sure the noteoffs don't get played before the actual note
	// scheduler()->SendNoteOffsForNowPlaying(c->sidnum());

	// Reset the modulation controller to 0
	doNewZController(c,0,false);  // no smoothing

	// doNewPitchBend(0.0f, true, smooth) {
}

void MusicBehaviourSTEIM::advanceTo(int tm) {
	// NosuchDebug("Music::periodicFire called");
}

void MusicBehaviourSTEIM::buttonDown(std::string bn) {
	palette()->setButtonUsed(bn,false);
	if ( bn=="LL3" ) {
#ifdef TEMP_UNQUANTIZE
		NosuchDebug("doquantize is set to false");
		palette()->params.doquantize = false;
#endif
	} else if ( bn=="LL2" ) {
		// visual effects
	} else if ( bn=="LL1" ) {
		// graphics
	} else if ( bn=="UL1" ) {
		// This is SCALE
	} else if ( bn=="UL2" ) {
		// TONIC
		static int last_tonic = -1;
		if ( last_tonic>0 && (Palette::now - last_tonic)<1500 ) {
			tonic_reset(palette());
		}
		last_tonic = Palette::now;
	} else if ( bn=="UL3" ) {
		// This is the RANGE button
		static int last_range = -1;
		if ( last_range>0 && (Palette::now - last_range)<1500 ) {
			NosuchDebug("RANGE reset!!");
		}
		last_range = Palette::now;
	} else if ( bn=="LR1" ) {

		// If we're holding down the Looping button, then ANO will ) {

		// clear all loops and turn looping off
		if ( palette()->isButtonDown("LR3") ) {
			NosuchDebug("CLEARING ALL LOOPS and turning looping OFF!");
			palette()->ClearAllLoops(true);
		}
		scheduler()->ANO();
	} else if ( bn=="LR2" ) {
	} else if ( bn=="LR3" ) {

#ifdef TEMP_LOOPING
		{
		double fade;
		fade = region()->params.loopfade;
		NosuchDebug("Looping ON, fade=%.4f",fade);
		palette()->SetAllLooping(true,fade);
		}
#endif
	} else if ( bn=="UR1" ) {
	} else if ( bn=="UR2" ) {
	} else if ( bn=="UR3" ) {
	} else {
		NosuchDebug("Button %d doesn't control anything in MusicBehaviourSTEIM",bn);
	}
}

void MusicBehaviourSTEIM::buttonUp(std::string bn) {

	NosuchDebug(1,"MusicBehaviourSTEIM::buttonUp bn=%d",bn);
	if ( bn=="LR3" ) {
#ifdef TEMP_LOOPING
		NosuchDebug("Looping OFF");
		palette()->SetAllLooping(false,-1);
#endif
	} else if ( bn=="UL2" ) {
		if ( ! palette()->isButtonUsed(bn) ) {
			NosuchDebug("Advancing tonic");
			tonic_change(palette());
		}
	} else if ( bn=="LL3" ) {
#ifdef TEMP_UNQUANTIZE
		{
		static int last_range = -1;

		if ( last_range>0 && (Palette::now - last_range)<1500 ) {
			NosuchDebug("doquantize is set to 0.0");
			palette()->params.doquantize = false;
		} else {
			NosuchDebug("doquantize is set back to 1.0");
			palette()->params.doquantize = true;
		}
		last_range = Palette::now;
		}
#endif
	} else if ( bn=="UR3" ) {
		if ( ! palette()->isButtonUsed(bn) ) {
			NosuchDebug("SETTING A RANDOM SOUNDSET");
			int newset = CurrentSoundSet;
			while ( newset == CurrentSoundSet ) {
				newset = randSoundSet();
			}
			changeSoundSet(newset);
		}
	}
}

//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------

MusicBehaviourCasual::MusicBehaviourCasual(Region* a) : MusicBehaviourDefault(a) {
}

bool MusicBehaviourCasual::isMyButton(std::string bn) {
	if ( bn == "LL1" || bn == "LL2" || bn == "LL3"
		|| bn == "UL1" || bn == "UL2" || bn == "UL3" ) {
		return true;
	} else {
		return false;
	}
}

bool MusicBehaviourCasual::isSelectorDown() {
	return (
		isButtonDown("UL3")     // music select
		);
}

void MusicBehaviourCasual::cursorDownWithSelector(Cursor *c) {

	NosuchDebug(1,"MusicBehaviourCasual, cursorDownWithSelector");

	if ( palette()->isButtonDown("UL3") ) {
		int selected = SelectionNumber(c);
		NosuchDebug(1,"MusicBehaviourCasual, UL3 selected=%d",selected);
		changeSoundSet(selected);
		palette()->setButtonUsed("UL3",true);
	}
}

void MusicBehaviourCasual::cursorDragWithSelector(Cursor *c) {
}

void MusicBehaviourCasual::cursorUpWithSelector(Cursor *c) {
}

void MusicBehaviourCasual::cursorDown(Cursor* c) {
	static int last_tonicchange = 0;
	int dt = Palette::now - last_tonicchange;
	int change = (int)region()->params.tonicchange;
	if ( change == 0 || dt > change ) {
		NosuchDebug(1,"PERIODIC CHANGING of TONIC!");
		tonic_change(palette());
		last_tonicchange = Palette::now;
	}
	doNewNote(c);
	// XXX - should probably do controller here if z > zmin
}

void MusicBehaviourCasual::cursorDrag(Cursor* c) {
	double dist = c->curr_pos.sub(c->previous_musicpos()).mag();
	double ddist = c->curr_depth - c->last_depth();
	if ( ddist < 0 )
		ddist = -ddist;
	NosuchDebug(1,"cursorDrag dist=%.4f ddist=%.4f c->d=%.3f c->prevd=%.3f",
		dist,ddist,c->curr_depth,c->last_depth());
	if ( dist >= region()->params.minmove || ddist >= region()->params.minmovedepth ) {
		NosuchDebug(2,"MUSIC::CURSORDRAG dist=%.3f",dist);
		doNewNote(c);
	}
	double z = region()->MaxCursorDepth();   // was: c->depth();
	double zmin = region()->params.controllerzmin;
	double zmax = region()->params.controllerzmax;

	std::string cstyle = region()->params.controllerstyle;
	if ( cstyle == "modulationonly" ) {
		if ( z > zmin ) {
			double zz = (z>zmax)?zmax:z;
			double dz = (zz-zmin) / (zmax-zmin);
			int cval = (int)(dz*128.0);
			if ( cval > 127 )
				cval = 127;
			doNewZController(c,cval,true);
		}
	} else if ( cstyle == "allcontrollers" ) {
		// doNewXController(c,cval,true);
		int ch = region()->params.controllerchan;
		int xctrl = region()->params.xcontroller;
		int yctrl = region()->params.ycontroller;
		int zctrl = region()->params.zcontroller;

		double zz = (z>zmax)?zmax:z;
		double dz = zz / zmax;
		int zval = (int)(dz*128.0);

		NosuchVector v = c->curr_pos;
		int xval = (int)(v.x*128.0) % 128;
		int yval = (int)(v.y*128.0) % 128;

		NosuchDebug("ALLCONTROLLERS drag x=%.3f y=%.3f z=%.3f",v.x,v.y,z);
		NosuchDebug("ALLCONTROLLERS vals x=%d y=%d z=%d",xval,yval,zval);
		doController(ch,xctrl,xval,c->sidnum(),true);
		doController(ch,yctrl,yval,c->sidnum(),true);
		doController(ch,zctrl,zval,c->sidnum(),true);
	} else if ( cstyle == "pitchYZ" ) {
		// doNewXController(c,cval,true);
		// int ch = region()->params.controllerchan;
		int ch = region()->channel();
		int yctrl = region()->params.ycontroller;
		int zctrl = region()->params.zcontroller;

		double zz = (z>zmax)?zmax:z;
		double dz = zz / zmax;
		int zval = (int)(dz*128.0);

		NosuchVector v = c->curr_pos;
		int yval = (int)(v.y*128.0) % 128;

		NosuchDebug(1,"pitchYZ drag y=%.3f z=%.3f",v.y,z);
		NosuchDebug(1,"pitchYZ vals y=%d z=%d",yval,zval);
		doController(ch,yctrl,yval,c->sidnum(),true);
		doController(ch,zctrl,zval,c->sidnum(),true);
	} else {
		NosuchDebug("UNRECOGNIZED value for controllerstyle: %s",cstyle.c_str());
	}
}

void MusicBehaviourCasual::cursorUp(Cursor* c) {

	std::string cstyle = region()->params.controllerstyle;

	NosuchDebug(2,"MusicBehaviourCasual::cursorUp ! c->sid=%d",c->sidnum());

	if ( DoPitchBend ) {
		region()->setNotesDisabled(false);
	}

	if ( cstyle == "allcontrollers" ) {
		int ch = region()->params.controllerchan;
		int zctrl = region()->params.zcontroller;
		doController(ch,zctrl,0,c->sidnum(),false);  // no smoothing
		return;
	}
	if ( cstyle == "pitchYZ" ) {
		// int ch = region()->params.controllerchan;
		int ch = region()->channel();
		int zctrl = region()->params.zcontroller;
		int yctrl = region()->params.ycontroller;
		doController(ch,zctrl,0,c->sidnum(),false);  // no smoothing
		doController(ch,yctrl,0,c->sidnum(),false);  // no smoothing
		// continue on, to do note
	}

	NosuchAssert(cstyle=="modulationonly"||cstyle=="pitchYZ");

	std::vector<int>& pitches = c->lastpitches();
	if ( pitches.size() == 0 ) {
		return;
	}
	int ch = c->lastchannel();
	int clk = c->lastclick();
	for ( size_t i=0; i < pitches.size(); i++ ) {
		int pitch = pitches[i];
		if ( pitch >= 0 ) {
			NosuchDebug(1,"cursorUp sending Noteoff for pitch=%d",pitch);
			// Normally, the getLastClick() will be quantized into the future,
			// so make sure we don't try to play the noteoff before that.
			int cc = paletteHost()->CurrentClick();
			if ( clk < cc ) {
				clk = cc;
			}
			scheduler()->IncomingNoteOff(clk,ch,pitch,0,c->sidnum());
		}
	}
	c->clear_last_note();

	// This was an attempt to try to do without the lastpitches stuff.  Failed.
	// I think it was because I didn't do the logic described above, where
	// it makes sure the noteoffs don't get played before the actual note
	// scheduler()->SendNoteOffsForNowPlaying(c->sidnum());

	// Reset the modulation controller to 0
	doNewZController(c,0,false);  // no smoothing

	// doNewPitchBend(0.0f, true, smooth) {
}

void MusicBehaviourCasual::advanceTo(int tm) {
	// NosuchDebug("Music::periodicFire called");
}

void MusicBehaviourCasual::buttonDown(std::string bn) {

	// ALL the button actions are delayed until they're released
	palette()->setButtonUsed(bn,false);

#if 0
	if ( bn == "UL3" ) {
		// wait until button up
		palette()->setButtonUsed(bn,false);
	} else {
		if ( palette()->isButtonDown("UL3") ) {
			NosuchDebug("SHIFTED OPERATION on Button = %s",bn.c_str());
			palette()->setButtonUsed("UL3",true);
			if ( bn == "UL1" || bn == "UL2" ) {
				NosuchDebug("CLEARING ALL LOOPS AND SENDING ANO!");
				palette()->ClearAllLoops(true);
				scheduler()->ANO();
				tonic_reset(palette());
			} else if ( bn == "LL2" ) {
				static int last_tonic = -1;
				if ( last_tonic>0 && (Palette::now - last_tonic)<1500 ) {
					tonic_reset(palette());
				} else {
					tonic_change(palette());
				}
				last_tonic = Palette::now;
			} else if ( bn == "LL3" ) {
				palette()->SetAllLooping(true,DEFAULT_LOOPFADE);
			} else if ( bn == "LL1" ) {
				palette()->SetAllLooping(false,DEFAULT_LOOPFADE);
				// palette()->ClearAllLoops(true);
				// scheduler()->ANO();
			}
		} else {
			int selected = bn_to_selected(bn);
			changeSoundSet(selected);
		}
	}
#endif
}

void MusicBehaviourCasual::buttonUp(std::string bn) {

	NosuchDebug(1,"MusicBehaviourCasual::buttonUp bn=%d",bn);
#if 0
	if ( bn != "UL3" && bn != "UL2" && bn != "UL1" ) {
		return;
	}
#endif
	if ( ! palette()->isButtonUsed(bn) ) {

		if ( palette()->isButtonDown("LL2") ) {
			NosuchDebug("LL2 SHIFTED OPERATION on Button = %s",bn.c_str());
			palette()->setButtonUsed("LL2",true);
			if ( bn == "LL3" ) {
				NosuchDebug("CLEARING ALL LOOPS AND SENDING ANO!");
				palette()->ClearAllLoops(true);
				scheduler()->ANO();
				tonic_reset(palette());
			} else if ( bn == "LL1" ) {
				static int last_tonic = -1;
				if ( last_tonic>0 && (Palette::now - last_tonic)<1500 ) {
					tonic_reset(palette());
				} else {
					tonic_change(palette());
				}
				last_tonic = Palette::now;
			}
		} else if ( palette()->isButtonDown("UL1") ) {
			NosuchDebug("UL1 SHIFTED OPERATION on Button = %s",bn.c_str());
			palette()->setButtonUsed("UL1",true);
			if ( bn == "UL3" ) {
				palette()->SetAllLooping(true,DEFAULT_LOOPFADE);
			} else if ( bn == "UL2" ) {
				palette()->SetAllLooping(false,DEFAULT_LOOPFADE);
				// palette()->ClearAllLoops(true);
				// scheduler()->ANO();
			}
		} else {
			int selected = bn_to_selected(bn);
			changeSoundSet(selected);
		}
	}
}

//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------

MusicBehaviourBurn::MusicBehaviourBurn(Region* a) : MusicBehaviourDefault(a) {
}

bool MusicBehaviourBurn::isMyButton(std::string bn) {
	return true;
}

bool MusicBehaviourBurn::isSelectorDown() {
	return false;
}

void MusicBehaviourBurn::cursorDownWithSelector(Cursor *c) {
	NosuchDebug("MusicBehaviourBurn, cursorDownWithSelector!?");
}

void MusicBehaviourBurn::cursorDragWithSelector(Cursor *c) {
	NosuchDebug("MusicBehaviourBurn, cursorDragWithSelector!?");
}

void MusicBehaviourBurn::cursorUpWithSelector(Cursor *c) {
	NosuchDebug("MusicBehaviourBurn, cursorUpWithSelector!?");
}

void MusicBehaviourBurn::cursorDown(Cursor* c) {
	static int last_tonicchange = 0;
	int dt = Palette::now - last_tonicchange;
	int change = (int)region()->params.tonicchange;
	if ( change == 0 || dt > change ) {
		NosuchDebug(1,"PERIODIC CHANGING of TONIC!");
		tonic_change(palette());
		last_tonicchange = Palette::now;
	}
	doNewNote(c);
	// XXX - should probably do controller here if z > zmin
}

void MusicBehaviourBurn::cursorDrag(Cursor* c) {
	double dist = c->curr_pos.sub(c->previous_musicpos()).mag();
	double ddist = c->curr_depth - c->last_depth();
	if ( ddist < 0 )
		ddist = -ddist;
	NosuchDebug(1,"cursorDrag dist=%.4f ddist=%.4f c->d=%.3f c->prevd=%.3f",
		dist,ddist,c->curr_depth,c->last_depth());
	if ( dist >= region()->params.minmove || ddist >= region()->params.minmovedepth ) {
		NosuchDebug(2,"MUSIC::CURSORDRAG dist=%.3f",dist);
		doNewNote(c);
	}
	double z = region()->MaxCursorDepth();   // was: c->depth();
	double zmin = region()->params.controllerzmin;
	double zmax = region()->params.controllerzmax;

	std::string cstyle = region()->params.controllerstyle;
	if ( cstyle == "modulationonly" ) {
		if ( z > zmin ) {
			double zz = (z>zmax)?zmax:z;
			double dz = (zz-zmin) / (zmax-zmin);
			int cval = (int)(dz*128.0);
			if ( cval > 127 )
				cval = 127;
			doNewZController(c,cval,true);
		}
	} else if ( cstyle == "allcontrollers" ) {
		// doNewXController(c,cval,true);
		int ch = region()->params.controllerchan;
		int xctrl = region()->params.xcontroller;
		int yctrl = region()->params.ycontroller;
		int zctrl = region()->params.zcontroller;

		double zz = (z>zmax)?zmax:z;
		double dz = zz / zmax;
		int zval = (int)(dz*128.0);

		NosuchVector v = c->curr_pos;
		int xval = (int)(v.x*128.0) % 128;
		int yval = (int)(v.y*128.0) % 128;

		NosuchDebug("ALLCONTROLLERS drag x=%.3f y=%.3f z=%.3f",v.x,v.y,z);
		NosuchDebug("ALLCONTROLLERS vals x=%d y=%d z=%d",xval,yval,zval);
		doController(ch,xctrl,xval,c->sidnum(),true);
		doController(ch,yctrl,yval,c->sidnum(),true);
		doController(ch,zctrl,zval,c->sidnum(),true);
	} else if ( cstyle == "pitchYZ" ) {
		// doNewXController(c,cval,true);
		// int ch = region()->params.controllerchan;
		int ch = region()->channel();
		int yctrl = region()->params.ycontroller;
		int zctrl = region()->params.zcontroller;

		double zz = (z>zmax)?zmax:z;
		double dz = zz / zmax;
		int zval = (int)(dz*128.0);

		NosuchVector v = c->curr_pos;
		int yval = (int)(v.y*128.0) % 128;

		NosuchDebug(1,"pitchYZ drag y=%.3f z=%.3f",v.y,z);
		NosuchDebug(1,"pitchYZ vals y=%d z=%d",yval,zval);
		doController(ch,yctrl,yval,c->sidnum(),true);
		doController(ch,zctrl,zval,c->sidnum(),true);
	} else {
		NosuchDebug("UNRECOGNIZED value for controllerstyle: %s",cstyle.c_str());
	}
}

void MusicBehaviourBurn::cursorUp(Cursor* c) {

	std::string cstyle = region()->params.controllerstyle;

	if ( DoPitchBend ) {
		region()->setNotesDisabled(false);
	}

	if ( cstyle == "allcontrollers" ) {
		int ch = region()->params.controllerchan;
		int zctrl = region()->params.zcontroller;
		doController(ch,zctrl,0,c->sidnum(),false);  // no smoothing
		return;
	}
	if ( cstyle == "pitchYZ" ) {
		// int ch = region()->params.controllerchan;
		int ch = region()->channel();
		int zctrl = region()->params.zcontroller;
		int yctrl = region()->params.ycontroller;
		doController(ch,zctrl,0,c->sidnum(),false);  // no smoothing
		doController(ch,yctrl,0,c->sidnum(),false);  // no smoothing
		// continue on, to do note
	}

	NosuchAssert(cstyle=="modulationonly"||cstyle=="pitchYZ");

	std::vector<int>& pitches = c->lastpitches();
	if ( pitches.size() == 0 ) {
		return;
	}
	int ch = c->lastchannel();
	int clk = c->lastclick();
	for ( size_t i=0; i < pitches.size(); i++ ) {
		int pitch = pitches[i];
		if ( pitch >= 0 ) {
			NosuchDebug(1,"cursorUp sending Noteoff for pitch=%d",pitch);
			// Normally, the getLastClick() will be quantized into the future,
			// so make sure we don't try to play the noteoff before that.
			int cc = paletteHost()->CurrentClick();
			if ( clk < cc ) {
				clk = cc;
			}
			scheduler()->IncomingNoteOff(clk,ch,pitch,0,c->sidnum());
		}
	}
	c->clear_last_note();

	// This was an attempt to try to do without the lastpitches stuff.  Failed.
	// I think it was because I didn't do the logic described above, where
	// it makes sure the noteoffs don't get played before the actual note
	// scheduler()->SendNoteOffsForNowPlaying(c->sidnum());

	// Reset the modulation controller to 0
	doNewZController(c,0,false);  // no smoothing

	// doNewPitchBend(0.0f, true, smooth) {
}

void MusicBehaviourBurn::advanceTo(int tm) {
	// NosuchDebug("Music::periodicFire called");
}

void MusicBehaviourBurn::buttonDown(std::string bn) {
	int selected = burn_bn_to_selected(bn);
	NosuchDebug("MusicBehaviourBurn::buttonDown selected=%d",selected);
	changeSoundSet(selected);
}

void MusicBehaviourBurn::buttonUp(std::string bn) {
}

//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------

MusicBehaviour2013::MusicBehaviour2013(Region* a) : MusicBehaviourDefault(a) {
}

bool MusicBehaviour2013::isMyButton(std::string bn) {
	return true;
}

bool MusicBehaviour2013::isSelectorDown() {
	return false;
}

void MusicBehaviour2013::cursorDownWithSelector(Cursor *c) {
	NosuchDebug("MusicBehaviour2013, cursorDownWithSelector!?");
}

void MusicBehaviour2013::cursorDragWithSelector(Cursor *c) {
	NosuchDebug("MusicBehaviour2013, cursorDragWithSelector!?");
}

void MusicBehaviour2013::cursorUpWithSelector(Cursor *c) {
	NosuchDebug("MusicBehaviour2013, cursorUpWithSelector!?");
}

void MusicBehaviour2013::cursorDown(Cursor* c) {
	static int last_tonicchange = 0;
	int dt = Palette::now - last_tonicchange;
	int change = (int)region()->params.tonicchange;
	if ( change == 0 || dt > change ) {
		NosuchDebug(1,"PERIODIC CHANGING of TONIC!");
		tonic_change(palette());
		last_tonicchange = Palette::now;
	}
	// NosuchDebug("cursorDown calling doNewNote");
	doNewNote(c);
	// XXX - should probably do controller here if z > zmin
}

void MusicBehaviour2013::cursorDrag(Cursor* c) {
	double dist = c->curr_pos.sub(c->previous_musicpos()).mag();
	double ddist = c->curr_depth - c->last_depth();
	if ( ddist < 0 )
		ddist = -ddist;
	// NosuchDebug(1,"cursorDrag dist=%.4f ddist=%.4f c->d=%.3f c->prevd=%.3f",
	// 	dist,ddist,c->curr_depth,c->last_depth());
	double mm = region()->params.minmove;
	double mmd = region()->params.minmovedepth;
	// NosuchDebug("cursorDrag, dist=%.4f  mm=%.4f  mmd=%.4f",dist,mm,mmd);
	if ( dist >= mm || ddist >= mmd ) {
		NosuchDebug(2,"MUSIC::CURSORDRAG dist=%.3f",dist);
		doNewNote(c);
	}
	double z = region()->MaxCursorDepth();   // was: c->depth();
	double zmin = region()->params.controllerzmin;
	double zmax = region()->params.controllerzmax;

	std::string cstyle = region()->params.controllerstyle;
	if ( cstyle == "modulationonly" ) {
		if ( z > zmin ) {
			double zz = (z>zmax)?zmax:z;
			double dz = (zz-zmin) / (zmax-zmin);
			int cval = (int)(dz*128.0);
			if ( cval > 127 )
				cval = 127;
			doNewZController(c,cval,true);
		}
	} else if ( cstyle == "allcontrollers" ) {
		// doNewXController(c,cval,true);
		int ch = region()->params.controllerchan;
		int xctrl = region()->params.xcontroller;
		int yctrl = region()->params.ycontroller;
		int zctrl = region()->params.zcontroller;

		double zz = (z>zmax)?zmax:z;
		double dz = zz / zmax;
		int zval = (int)(dz*128.0);

		NosuchVector v = c->curr_pos;
		int xval = (int)(v.x*128.0) % 128;
		int yval = (int)(v.y*128.0) % 128;

		NosuchDebug("ALLCONTROLLERS drag x=%.3f y=%.3f z=%.3f",v.x,v.y,z);
		NosuchDebug("ALLCONTROLLERS vals x=%d y=%d z=%d",xval,yval,zval);
		doController(ch,xctrl,xval,c->sidnum(),true);
		doController(ch,yctrl,yval,c->sidnum(),true);
		doController(ch,zctrl,zval,c->sidnum(),true);
	} else if ( cstyle == "pitchYZ" ) {
		// doNewXController(c,cval,true);
		// int ch = region()->params.controllerchan;
		int ch = region()->channel();
		int yctrl = region()->params.ycontroller;
		int zctrl = region()->params.zcontroller;

		double zz = (z>zmax)?zmax:z;
		double dz = zz / zmax;
		int zval = (int)(dz*128.0);

		NosuchVector v = c->curr_pos;
		int yval = (int)(v.y*128.0) % 128;

		NosuchDebug(1,"pitchYZ drag y=%.3f z=%.3f",v.y,z);
		NosuchDebug(1,"pitchYZ vals y=%d z=%d",yval,zval);
		doController(ch,yctrl,yval,c->sidnum(),true);
		doController(ch,zctrl,zval,c->sidnum(),true);
	} else {
		NosuchDebug("UNRECOGNIZED value for controllerstyle: %s",cstyle.c_str());
	}
}

void MusicBehaviour2013::cursorUp(Cursor* c) {

	std::string cstyle = region()->params.controllerstyle;

	if ( DoPitchBend ) {
		region()->setNotesDisabled(false);
	}

	if ( cstyle == "allcontrollers" ) {
		int ch = region()->params.controllerchan;
		int zctrl = region()->params.zcontroller;
		doController(ch,zctrl,0,c->sidnum(),false);  // no smoothing
		return;
	}
	if ( cstyle == "pitchYZ" ) {
		// int ch = region()->params.controllerchan;
		int ch = region()->channel();
		int zctrl = region()->params.zcontroller;
		int yctrl = region()->params.ycontroller;
		doController(ch,zctrl,0,c->sidnum(),false);  // no smoothing
		doController(ch,yctrl,0,c->sidnum(),false);  // no smoothing
		// continue on, to do note
	}

	NosuchAssert(cstyle=="modulationonly"||cstyle=="pitchYZ");

	std::vector<int>& pitches = c->lastpitches();
	if ( pitches.size() == 0 ) {
		return;
	}
	int ch = c->lastchannel();
	int clk = c->lastclick();
	for ( size_t i=0; i < pitches.size(); i++ ) {
		int pitch = pitches[i];
		if ( pitch >= 0 ) {
			NosuchDebug(1,"cursorUp sending Noteoff for pitch=%d",pitch);
			// Normally, the getLastClick() will be quantized into the future,
			// so make sure we don't try to play the noteoff before that.
			int cc = paletteHost()->CurrentClick();
			if ( clk < cc ) {
				clk = cc;
			}
			scheduler()->IncomingNoteOff(clk,ch,pitch,0,c->sidnum());
		}
	}
	c->clear_last_note();

	// This was an attempt to try to do without the lastpitches stuff.  Failed.
	// I think it was because I didn't do the logic described above, where
	// it makes sure the noteoffs don't get played before the actual note
	// scheduler()->SendNoteOffsForNowPlaying(c->sidnum());

	// Reset the modulation controller to 0
	doNewZController(c,0,false);  // no smoothing

	// doNewPitchBend(0.0f, true, smooth) {
}

void MusicBehaviour2013::advanceTo(int tm) {
	// NosuchDebug("Music::periodicFire called");
}

void MusicBehaviour2013::buttonDown(std::string bn) {
	int selected = burn_bn_to_selected(bn);
	NosuchDebug("MusicBehaviour2013::buttonDown selected=%d",selected);
	changeSoundSet(selected);
}

void MusicBehaviour2013::buttonUp(std::string bn) {
}

//----------------------------------------------------------------------------------