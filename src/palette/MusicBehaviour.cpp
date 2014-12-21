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

static bool DoPitchBend = false;

MusicBehaviour::MusicBehaviour(Region* r) : Behaviour(r->palette(),r) {
	NosuchDebug(2,"MusicBehaviour CONSTRUCTOR! setting CurrentSoundSet to 0!");
	CurrentSoundSet = 0;
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

int MusicBehaviour::VizCursor2Pitch(VizCursor* c) {
	int mn;
	int mx;

	if ( regionParams()->fullrange.get() ) {
		mn = 10;
		mx = 120;
	} else {
		mn = regionParams()->pitchmin.get();
		mx = regionParams()->pitchmax.get();
	}
	int dp = mx - mn;
	if ( dp < 0 )
		dp = -dp;
	double x = c->pos.x;
	int p = mn + (int)(dp * x);
	NosuchDebug(2,"VizCursor2Pitch x=%.4f pitch=%d",x,p);
	return p;
}

int MusicBehaviour::VizCursor2Quant(VizCursor* c) {

	Region* r = region();
	AllVizParams* p = r->regionParams();

	if ( ! p->doquantize.get() ) {
		return 1;
	}

	int q;
	double y = -1;

	if ( p->quantfixed.get() ) {
		q = QuarterNoteClicks/4;
	} else {
		y = c->pos.y;
		if ( y < p->timefret1y.get() ) {
			q = (int) (QuarterNoteClicks * p->timefret1q.get());
		} else if ( y < p->timefret2y ) {
			q = (int) (QuarterNoteClicks * p->timefret2q.get());
		} else if ( y < p->timefret3y ) {
			q = (int) (QuarterNoteClicks * p->timefret3q.get());
		} else if ( y < p->timefret4y ) {
			q = (int) (QuarterNoteClicks * p->timefret4q.get());
		} else {
			q = 1;
		}
	}
	double qfactor = p->quantfactor.get();
	q = (int)(qfactor*q);
	if ( q < 1 )
		q = 1;
	NosuchDebug(1,"CURSOR2QUANT y=%.4f q=%d",y,q);
	return q;
}

void
MusicBehaviour::setRegionSound(int rid, std::string nm)
{
	palette()->setRegionSound(rid,nm);
}

int MusicBehaviour::quantizeToNext(int clicks, int quant) {
	return clicks - (clicks%quant) + quant;
}

void MusicBehaviour::doController(int ch, int ctrlr, int val, int sidnum, bool smooth) {
	MidiMsg* m = MidiController::make(ch,ctrlr,val);
	vizserver()->SendControllerMsg(m,this,smooth);
	// Don't delete m, SendControllerMsg takes ownership of it
}

void MusicBehaviour::doNewZController(VizCursor* c, int val, bool smooth) {
	doController(region()->channel(),0x01,val,c->sid,smooth);
}

void MusicBehaviour::doNewNote(VizCursor* c) {

	Region* r = c->region;
	AllVizParams* params = region()->regionParams();

	if ( DoPitchBend && r->NotesDisabled() ) {
		NosuchDebug("doNewNote is NOT adding a note due to NotesDisabled");
		return;
	}
	if ( params->controllerstyle.get() == "allcontrollers" ) {
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

	int qnt = VizCursor2Quant(c);
	int qclk = quantizeToNext(clk,qnt);

	// Should the scheduler be locked, here?

	std::string sn = palette()->musicscale;
	Scale& scl = Scale::Scales[sn];
	int pitch = VizCursor2Pitch(c);

	// XXX There should really be a "isdrums" or "dontscale" value
	// rather than depending on the actual soundset name, I think
	std::string ssval = params->sound.get();
	int npitch = pitch;
	if ( ssval != "Beatbox" ) {
		npitch = scl.closestTo(pitch);
	}
	if ( NosuchDebugMidiNotes ) {
		NosuchDebug("     DONEWNOTE, scale=%s origpitch=%d newpitch=%d frame=%d click=%d",
			sn.c_str(),pitch,npitch,paletteHost()->frame,paletteHost()->CurrentClick());
	}

	if ( npitch < 0 ) {
		throw NosuchException("Unable to find closest pitch to %d in scale=%s",pitch,sn.c_str());
	}

	if ( params->arpeggio.get() ) {
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

	vizserver()->IncomingMidiMsg(m1,qclk,this);

	c->clear_last_note();
	c->add_last_note(qclk,m1);

	c->set_previous_musicpos(c->pos);
	c->set_last_depth(c->depth());
}

void
MusicBehaviour::tonic_reset(Palette* palette) {
	NosuchDebug("TONIC reset!!");
	palette->paletteHost()->vizserver()->ANO();
	AllVizParams* params = palette->paletteHost()->defaultParams();
	palette->tonic = 0;
	palette->paletteHost()->vizserver()->SetGlobalPitchOffset(0);
}

void
MusicBehaviour::tonic_set(Palette* palette, int t) {
	NosuchDebug("TONIC SET for palette tonic=%d",t);
	AllVizParams* params = palette->paletteHost()->defaultParams();
	palette->tonic = t;
	palette->paletteHost()->vizserver()->SetGlobalPitchOffset(t);
}

void
MusicBehaviour::tonic_next(Palette* palette) {

	PaletteHost* phost = palette->paletteHost();

	phost->vizserver()->ANO();

	AllVizParams* params = phost->defaultParams();
	int tonic = palette->tonic;
	switch(tonic){
	case 0: tonic = 5; break;
	case 5: tonic = 3; break;
	case 3: tonic = -4; break;
	case -4: tonic = 0; break;
	default: tonic = 0; break;
	}
	palette->tonic = tonic;

	int nhosts = phost->_tonicchangeclienthost.size();
	if ( nhosts > 0 ) {
		NosuchDebug(1,"Sending tonicchange %d to %d other hosts",tonic,nhosts);
	}
	for ( int i=0; i<nhosts; i++ ) {
	    char buffer[1024];
		NosuchDebug(1,"Sending tonicchange %d OSC message to %d @ %s",
			tonic,
			phost->_tonicchangeclientport[i],
			phost->_tonicchangeclienthost[i].c_str());
	    osc::OutboundPacketStream p( buffer, sizeof(buffer) );
	    p << osc::BeginMessage( "/palette/tonicchange" ) << tonic << osc::EndMessage;
	    SendToUDPServer(phost->_tonicchangeclienthost[i],phost->_tonicchangeclientport[i],p.Data(),(int)p.Size());
	}
}

bool MusicBehaviour::isMyButton(std::string bn) {
	return true;
}

void MusicBehaviour::cursorDown(VizCursor* c) {
	doNewNote(c);
}

void MusicBehaviour::cursorDrag(VizCursor* c) {
	double dist = c->pos.sub(c->previous_musicpos()).mag();
	double ddist = c->depth() - c->last_depth();
	if ( ddist < 0 )
		ddist = -ddist;
	// NosuchDebug(1,"cursorDrag dist=%.4f ddist=%.4f c->d=%.3f c->prevd=%.3f",
	// 	dist,ddist,c->depth(),c->last_depth());

	AllVizParams* params = regionParams();

	double mm = params->minmove.get();
	double mmd = params->minmovedepth.get();
	// NosuchDebug("cursorDrag, dist=%.4f  mm=%.4f  mmd=%.4f",dist,mm,mmd);
	if ( dist >= mm || ddist >= mmd ) {
		if ( NosuchDebugMidiNotes ) {
			DEBUGPRINT1(("MUSIC::CURSORDRAG dist=%.3f doing doNewNote!",dist));
		}
		doNewNote(c);
	}
	double z = region()->MaxVizCursorDepth();   // was: c->depth();
	double zmin = params->controllerzmin.get();
	double zmax = params->controllerzmax.get();

	std::string cstyle = params->controllerstyle.get();
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
		int ch = params->controllerchan.get();
		int xctrl = params->xcontroller.get();
		int yctrl = params->ycontroller.get();
		int zctrl = params->zcontroller.get();

		double zz = (z>zmax)?zmax:z;
		double dz = zz / zmax;
		int zval = (int)(dz*128.0);

		NosuchVector v = c->pos;
		int xval = (int)(v.x*128.0) % 128;
		int yval = (int)(v.y*128.0) % 128;

		NosuchDebug("ALLCONTROLLERS drag x=%.3f y=%.3f z=%.3f",v.x,v.y,z);
		NosuchDebug("ALLCONTROLLERS vals x=%d y=%d z=%d",xval,yval,zval);
		doController(ch,xctrl,xval,c->sid,true);
		doController(ch,yctrl,yval,c->sid,true);
		doController(ch,zctrl,zval,c->sid,true);
	} else if ( cstyle == "pitchYZ" ) {
		// doNewXController(c,cval,true);
		// int ch = params->controllerchan.get();
		int ch = region()->channel();
		int yctrl = params->ycontroller.get();
		int zctrl = params->zcontroller.get();

		double zz = (z>zmax)?zmax:z;
		double dz = zz / zmax;
		int zval = (int)(dz*128.0);

		NosuchVector v = c->pos;
		int yval = (int)(v.y*128.0) % 128;

		NosuchDebug(1,"pitchYZ drag y=%.3f z=%.3f",v.y,z);
		NosuchDebug(1,"pitchYZ vals y=%d z=%d",yval,zval);
		doController(ch,yctrl,yval,c->sid,true);
		doController(ch,zctrl,zval,c->sid,true);
	} else {
		NosuchDebug("UNRECOGNIZED value for controllerstyle: %s",cstyle.c_str());
	}
}

void MusicBehaviour::cursorUp(VizCursor* c) {

	// DEBUGPRINT(("MusicBehaviour::cursorup! sid=%d",c->sid));
	AllVizParams* params = regionParams();
	std::string cstyle = params->controllerstyle.get();

	if ( DoPitchBend ) {
		region()->setNotesDisabled(false);
	}

	if ( cstyle == "allcontrollers" ) {
		int ch = params->controllerchan.get();
		int zctrl = params->zcontroller.get();
		doController(ch,zctrl,0,c->sid,false);  // no smoothing
		return;
	}
	if ( cstyle == "pitchYZ" ) {
		// int ch = params->controllerchan.get();
		int ch = region()->channel();
		int zctrl = params->zcontroller.get();
		int yctrl = params->ycontroller.get();
		doController(ch,zctrl,0,c->sid,false);  // no smoothing
		doController(ch,yctrl,0,c->sid,false);  // no smoothing
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
			if ( NosuchDebugMidiNotes ) {
				NosuchDebug("cursorUp C sending Noteoff for pitch=%d",pitch);
			}
			// Normally, the getLastClick() will be quantized into the future,
			// so make sure we don't try to play the noteoff before that.
			int cc = paletteHost()->CurrentClick();
			if ( clk < cc ) {
				clk = cc;
			}
			vizserver()->IncomingNoteOff(clk,ch,pitch,0,this);
		}
	}
	c->clear_last_note();

	// This was an attempt to try to do without the lastpitches stuff.  Failed.
	// I think it was because I didn't do the logic described above, where
	// it makes sure the noteoffs don't get played before the actual note
	// vizserver()->SendNoteOffsForNowPlaying(c->sid);

	// Reset the modulation controller to 0
	doNewZController(c,0,false);  // no smoothing
}

void MusicBehaviour::advanceTo(int tm) {
	// NosuchDebug("MusicBehaviour::advanceTo called tm=%d",tm);
	if ( palette()->paletteHost()->_do_tonicchange ) {
		static int last_tonicchange = 0;
		int dt = Palette::now - last_tonicchange;
		int change = (int)region()->regionParams()->tonicchange.get();
		if ( change == 0 || dt > change ) {
			tonic_next(palette());
			last_tonicchange = Palette::now;
		}
	}
}

void MusicBehaviour::buttonDown(std::string bn) {
	int selected = bn_to_selected(bn);
	NosuchDebug(1,"MusicBehaviour::buttonDown selected=%d",selected);
	changeSoundSet(selected);
}

void MusicBehaviour::buttonUp(std::string bn) {
}

//----------------------------------------------------------------------------------