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

std::vector<std::string> GraphicBehaviour::mirrorTypes;
std::vector<std::string> GraphicBehaviour::behaviourTypes;
std::vector<std::string> GraphicBehaviour::movedirTypes;
std::vector<std::string> GraphicBehaviour::rotangdirTypes;

bool GraphicBehaviour::initialized = false;

// ======================================================

GraphicBehaviour::GraphicBehaviour(Region* a) : Behaviour(a) {
}

void GraphicBehaviour::initialize() {

	if ( initialized )
		return;
	initialized = true;

	behaviourTypes.push_back("default");
	behaviourTypes.push_back("museum");
	behaviourTypes.push_back("STEIM");
	behaviourTypes.push_back("casual");
	behaviourTypes.push_back("burn");
	// behaviourTypes.push_back("draw");

	movedirTypes.push_back("cursor");
	movedirTypes.push_back("left");
	movedirTypes.push_back("right");
	movedirTypes.push_back("up");
	movedirTypes.push_back("down");
	movedirTypes.push_back("random");

	rotangdirTypes.push_back("right");
	rotangdirTypes.push_back("left");
	rotangdirTypes.push_back("random");

	mirrorTypes.push_back("none");
	mirrorTypes.push_back("vertical");
	mirrorTypes.push_back("horizontal");
	mirrorTypes.push_back("four");
}

GraphicBehaviour* GraphicBehaviour::makeBehaviour(std::string type, Region* a) {
	if ( type == "default" ) {
		return new GraphicBehaviourDefault(a);
	} else if ( type == "museum" ) {
		return new GraphicBehaviourMuseum(a);
	} else if ( type == "STEIM" ) {
		return new GraphicBehaviourSTEIM(a);
	} else if ( type == "casual" ) {
		return new GraphicBehaviourCasual(a);
	} else if ( type == "burn" ) {
		return new GraphicBehaviourBurn(a);
	} else if ( type == "2013" ) {
		return new GraphicBehaviour2013(a);
	}
	NosuchErrorOutput("Unrecognized graphic behaviour name: %s",type.c_str());
	return new GraphicBehaviourDefault(a);
}

// ======================================================

GraphicBehaviourDefault::GraphicBehaviourDefault(Region* r) : GraphicBehaviour(r) {
}

bool GraphicBehaviourDefault::isSelectorDown() {
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
void GraphicBehaviourDefault::cursorDownWithSelector(Cursor* c) {
	if ( palette()->isButtonDown("LL1") ) {
		int selected = SelectionNumber(c);
		std::string fn;
		switch (selected) {
		case 0: fn = "final_square1.plt"; break;
		case 1: fn = "web_of_lines.plt"; break;
		case 2: fn = "dreamsquares.plt"; break;
		case 3: fn = "dreamcircle2.plt"; break;
		case 4: fn = "nye1.plt"; break;
		case 5: fn = "slow_thick_bubbles.plt"; break;
		case 6: fn = "dreamlines.plt"; break;
		case 7: fn = "dreamvaried.plt"; break;
		}
		NosuchDebug("GRAPHIC SELECTION! selected=%d fn=%s",selected,fn.c_str());
		palette()->ConfigLoad(fn);
		palette()->setButtonUsed("LL1",true);
		return;
	}
	if ( palette()->isButtonDown("LL2") ) {
		int selected = SelectionNumber(c);
		NosuchDebug(2,"EFFECT SELECTION! selected=%d",selected);
		palette()->LoadEffectSet(selected);
		palette()->setButtonUsed("LL2",true);
		return;
	}
}

void GraphicBehaviourDefault::cursorDragWithSelector(Cursor* c) {
}

void GraphicBehaviourDefault::cursorUpWithSelector(Cursor* c) {
}

void GraphicBehaviourDefault::buttonDown(std::string bn) {
	
	int effset = -1;

	if ( bn == "LL1" || bn == "LL2" ) {
		NosuchDebug(1,"Setting Button Used to false for bn=%d");
		palette()->setButtonUsed(bn,false);
	}
}

void GraphicBehaviourDefault::buttonUp(std::string bn) {

	if ( bn=="LL2" ) {
		if ( ! palette()->isButtonUsed(bn) ) {
			int neweffset;
			do {
				neweffset = rand() % 8; // random visual effect from 0-7
			} while (neweffset == palette()->CurrentEffectSet());
			NosuchDebug("RANDOM EFFECT SELECTION! effset=%d",neweffset);
			palette()->LoadEffectSet(neweffset);
		}
	} else if ( bn == "LL1" ) {
		if ( ! palette()->isButtonUsed(bn) ) {
			NosuchDebug("RANDOM CONFIG SELECTION!");
			std::string name;
			palette()->ConfigLoadRand(name);
		}
	}
}
	
void GraphicBehaviourDefault::cursorDown(Cursor* c) {
	std::string shape = region()->params.shape;
	// NosuchDebug("cursorDown gl_frame=%d instantiating",paletteHost()->gl_frame);
	if ( region()->params.cursorsprites ) {
		region()->instantiateSprite(c,false);
	}
	if ( LoopCursors ) {
		paletteHost()->CursorDownNotification(c);
	}
}

void GraphicBehaviourDefault::cursorDrag(Cursor* c) {
}

void GraphicBehaviourDefault::cursorUp(Cursor* c) {
#if 0
	std::string behave = c->region()->graphicbahaviour;
	if ( c->params.behaviour == "accumulate" ) {
		c->curr_depth = 0.0f;
		region()->accumulateSpritesForCursor(c);
	}
#endif
}

void GraphicBehaviourDefault::advanceTo(int tm) {
	// NosuchDebug("GraphicBehaviourDefault::periodicFire called now=%d",Palette::now);
	// int now = Palette::now;

	if ( ! region()->cursorlist_lock_read() ) {
		NosuchDebug("Graphic->advanceTo returns, unable to lock cursorlist");
		return;
	}

	bool gotexception = false;
	try {
		CATCH_NULL_POINTERS;
		for ( std::list<Cursor*>::iterator i = region()->cursors().begin(); i!=region()->cursors().end(); i++ ) {
			Cursor* c = *i;

			NosuchAssert(c);

			c->advanceTo(tm);

			if ( ! region()->params.cursorsprites ) {
				continue;
			}

			std::string behave = c->curr_behaviour;
			if ( behave == "" || behave == "instantiate" ) {
				// NosuchDebug("advanceTo gl_frame=%d instantiating",paletteHost()->gl_frame);
				region()->instantiateSprite(c,true);
			} else if ( behave == "move" ) {
				NosuchDebug(2,"periodFire cursor move NOT IMPLEMENTED!");
#if 0
			} else if ( behave == "accumulate" ) {
				NosuchDebug(1,"periodFire cursor accumulate!");
				// region()->accumulateSprite(c);
				region()->accumulateSpritesForCursor(c);
#endif
			}
		}
	} catch (NosuchException& e ) {
		NosuchDebug("NosuchException in GraphicBehaviourDefault::advanceto : %s",e.message());
		gotexception = true;
	} catch (...) {
		NosuchDebug("UNKNOWN Exception in GraphicBehaviourDefault::advanceto!");
		gotexception = true;
	}

	region()->cursorlist_unlock();
}

// ======================================================

bool GraphicBehaviourMuseum::isSelectorDown() {
	return (
		isButtonDown("LL1")  // graphics select
		|| isButtonDown("UL3")     // visual effect select
		);
}
GraphicBehaviourMuseum::GraphicBehaviourMuseum(Region* r) : GraphicBehaviourDefault(r) {
}

void GraphicBehaviourMuseum::cursorDownWithSelector(Cursor* c) {
	int selected = SelectionNumber(c);

	// Select a specific graphic
	if ( palette()->isButtonDown("LL1") ) {
		palette()->CurrentMuseumGraphic = selected % palette()->NumMuseumGraphics;
		NosuchDebug("SELECTED GRAPHIC! n=%d",palette()->CurrentMuseumGraphic);
		palette()->ConfigLoad(MuseumGraphics[palette()->CurrentMuseumGraphic]);
	}

	// Select a specific visual
	if ( palette()->isButtonDown("UL3") ) {
		int effset = selected % paletteHost()->NumEffectSet();
		NosuchDebug("SELECTED VISUAL! effset=%d",effset);
		palette()->LoadEffectSet(effset);
	}

}

void GraphicBehaviourMuseum::cursorDragWithSelector(Cursor* c) {
}

void GraphicBehaviourMuseum::cursorUpWithSelector(Cursor* c) {
}

void GraphicBehaviourMuseum::buttonDown(std::string bn) {
	
	NosuchDebug("MUSEUM buttonDown bn=%d",bn);
	int effset = -1;
	int ngraphics = palette()->NumMuseumGraphics;
	int neffects = paletteHost()->NumEffectSet();

	if ( bn=="LL1" ) {
		NosuchDebug(1,"Random Graphic ngraphics=%d",ngraphics);
		palette()->ConfigLoadRandom();
	} else if ( bn=="LL2" ) {
		palette()->CurrentMuseumGraphic = (palette()->CurrentMuseumGraphic+ngraphics-1) % ngraphics;
		NosuchDebug("PREV GRAPHIC! n=%d",palette()->CurrentMuseumGraphic);
		palette()->ConfigLoad(MuseumGraphics[palette()->CurrentMuseumGraphic]);
	} else if ( bn=="LL3" ) {
		palette()->CurrentMuseumGraphic = (palette()->CurrentMuseumGraphic+1) % ngraphics;
		NosuchDebug("NEXT GRAPHIC! n=%d",palette()->CurrentMuseumGraphic);
		palette()->ConfigLoad(MuseumGraphics[palette()->CurrentMuseumGraphic]);
	} else if ( bn=="UL1" ) {
		effset = palette()->PrevEffectSet();
		NosuchDebug("PREV Visual effset=%d",effset);
		palette()->LoadEffectSet(effset);
	} else if ( bn=="UL2" ) {
		effset = palette()->NextEffectSet();
		NosuchDebug("NEXT Visual effset=%d",effset);
		palette()->LoadEffectSet(effset);
	} else if ( bn=="UL3" ) {
		palette()->LoadRandomEffectSet();
	}
}

void GraphicBehaviourMuseum::buttonUp(std::string bn) {

}
	
// ======================================================
// ======================================================

bool GraphicBehaviourSTEIM::isSelectorDown() {
	return (
		isButtonDown("UR1")  // visual effect select
		|| isButtonDown("UR2")     // graphic select
		);
}
GraphicBehaviourSTEIM::GraphicBehaviourSTEIM(Region* r) : GraphicBehaviourDefault(r) {
	NosuchDebug("Creating GraphicBehaviourSTEIM!");
}

void GraphicBehaviourSTEIM::cursorDownWithSelector(Cursor* c) {
	int selected = SelectionNumber(c);

	// Select a specific graphic
	if ( palette()->isButtonDown("UR2") ) {
		palette()->CurrentMuseumGraphic = selected % palette()->NumMuseumGraphics;
		NosuchDebug("SELECTED GRAPHIC! n=%d",palette()->CurrentMuseumGraphic);
		palette()->ConfigLoad(MuseumGraphics[palette()->CurrentMuseumGraphic]);
		palette()->setButtonUsed("UR2",true);
	}

	// Select a specific visual
	if ( palette()->isButtonDown("UR1") ) {
		int effset = selected % paletteHost()->NumEffectSet();
		NosuchDebug("SELECTED VISUAL! effset=%d",effset);
		palette()->LoadEffectSet(effset);
		palette()->setButtonUsed("UR1",true);
	}
}

void GraphicBehaviourSTEIM::cursorDragWithSelector(Cursor* c) {
}

void GraphicBehaviourSTEIM::cursorUpWithSelector(Cursor* c) {
	NosuchDebug(2,"Cursor Up with Selector!");
}

void GraphicBehaviourSTEIM::buttonDown(std::string bn) {
	NosuchDebug(1,"GraphicBehaviourSTEIM::buttonDown bn=%d",bn);
}

void GraphicBehaviourSTEIM::buttonUp(std::string bn) {
	NosuchDebug(1,"GraphicBehaviourSTEIM::buttonUp bn=%d",bn);
	if ( ! palette()->isButtonUsed(bn) ) {
		if ( bn == "UR1" ) {
			NosuchDebug("GraphicBehaviourSTEIM::buttonUp doing random viz effect");
			palette()->LoadRandomEffectSet();
		} else if ( bn == "UR2" ) {
			NosuchDebug("GraphicBehaviourSTEIM::buttonUp doing random graphic");
			std::string name;
			palette()->ConfigLoadRand(name);
		}
	}
}
	
// ======================================================
// ======================================================

bool GraphicBehaviourCasual::isSelectorDown() {
	return (
		isButtonDown("UR3")  // visual/graphic select
		);
}
bool GraphicBehaviourCasual::isMyButton(std::string bn) {
	if ( bn=="LR1"
		|| bn=="LR2"
		|| bn=="LR3"
		|| bn=="UR1"
		|| bn=="UR2"
		|| bn=="UR3" ) {
		return true;
	} else {
		return false;
	}
}

GraphicBehaviourCasual::GraphicBehaviourCasual(Region* r) : GraphicBehaviourDefault(r) {
}

void GraphicBehaviourCasual::cursorDownWithSelector(Cursor* c) {
	int selected = SelectionNumber(c);

	NosuchDebug(1,"GraphicBehaviourCasual, cursorDownWithSelector, selected=%d",selected);

	if ( palette()->isButtonDown("UR3") ) {
		int selected = SelectionNumber(c);
		NosuchDebug(1,"GraphicBehaviourCasual, UR3, selected=%d",selected);

		palette()->CurrentMayGraphic = selected % palette()->NumMayGraphics;
		NosuchDebug("SELECTED GRAPHIC! n=%d",palette()->CurrentMayGraphic);
		palette()->ConfigLoad(MayGraphics[palette()->CurrentMayGraphic]);

		int effset = selected % paletteHost()->NumEffectSet();
		NosuchDebug("SELECTED VISUAL! effset=%d",effset);
		palette()->LoadEffectSet(effset);

		palette()->setButtonUsed("UR3",true);
	}
}

void GraphicBehaviourCasual::cursorDragWithSelector(Cursor* c) {
}

void GraphicBehaviourCasual::cursorUpWithSelector(Cursor* c) {
	NosuchDebug(2,"Cursor Up with Selector!");
}

int bn_to_selected(std::string bn) {
	if ( bn == "LL1" ) return 0;
	if ( bn == "LL2" ) return 1;
	if ( bn == "LL3" ) return 2;
	if ( bn == "UL1" ) return 3;
	if ( bn == "UL2" ) return 4;
	if ( bn == "UL3" ) return 5;
	if ( bn == "LR1" ) return 0;
	if ( bn == "LR2" ) return 1;
	if ( bn == "LR3" ) return 2;
	if ( bn == "UR1" ) return 3;
	if ( bn == "UR2" ) return 4;
	if ( bn == "UR3" ) return 5;
	return 0;
}

void
graphic_and_effect_select(PaletteHost* manifold, Palette* palette, int selected)
{
	NosuchDebug(1,"   Should be settting viz/graphic to selected=%d",selected);
	int effset = selected % manifold->NumEffectSet();
	palette->LoadEffectSet(effset);

	palette->CurrentMayGraphic = selected % palette->NumMayGraphics;
	NosuchDebug("SELECTED GRAPHIC! n=%d",palette->CurrentMayGraphic);
	palette->ConfigLoad(MayGraphics[palette->CurrentMayGraphic]);
}

typedef struct select_info {
	char *label;
	int x, y;
} select_info;

#ifdef THIS_IS_FOR_POOR_RICHARD_FONT
select_info burn_select[NUM_EFFECT_SETS] = {
	{ "Ribbon Dance", 295, 350 },
	{ "Mirrored Waves", 280, 350 },
	{ "Sacred Circles", 290, 350 },
	{ "Oozing Color", 310, 350 },
	{ "Faded Swirls", 315, 350 },
	{ "Organic Outlines", 260, 350 },
	{ "Fuzzy Logic", 330, 350 },
	{ "Perky Shapes", 305, 350 },
	{ "Time Ripples", 305, 350 },
	{ "Smooth Moves", 310, 350 },
	{ "Dancing Lines", 300, 350 },
	{ "Rippled Shadows", 270, 350 },
};
#endif

select_info burn_select[NUM_EFFECT_SETS] = {
	{ "Ribbon Dance", 255, 350 },
	{ "Mirrored Waves", 220, 350 },
	{ "Sacred Circles", 243, 350 },
	{ "Oozing Color", 265, 350 },
	{ "Faded Swirls", 261, 350 },
	{ "Organic Outlines", 210, 350 },
	{ "Fuzzy Logic", 290, 350 },
	{ "Perky Shapes", 255, 350 },
	{ "Time Ripples", 265, 350 },
	{ "Smooth Moves", 245, 350 },
	{ "Dancing Lines", 240, 350 },
	{ "Rippled Shadows", 200, 350 },
};

void
burn_graphic_and_effect_select(PaletteHost* manifold, Palette* palette, int selected, std::string bn)
{
	NosuchDebug(1,"   Should be settting viz/graphic to selected=%d",selected);
	int effset = selected % manifold->NumEffectSet();
	palette->LoadEffectSet(effset);

	palette->CurrentBurnGraphic = selected % palette->NumBurnGraphics;
	NosuchDebug("GRAPHIC SELECTION! n=%d",palette->CurrentBurnGraphic);
	palette->ConfigLoad(BurnGraphics[palette->CurrentBurnGraphic]);
	std::string t = NosuchSnprintf("selected=%d",selected);
	select_info* si = &burn_select[selected%NUM_EFFECT_SETS];
	manifold->ShowChoice(bn,si->label,si->x,si->y,1500);
}

void GraphicBehaviourCasual::buttonDown(std::string bn) {
	// NosuchDebug("GraphicBehaviourCasual::buttonDown bn=%d",bn);
	if ( bn == "UR3" ) {
		// wait until button up
		palette()->setButtonUsed(bn,false);
	} else {
		int selected = bn_to_selected(bn);
		graphic_and_effect_select(paletteHost(),palette(),selected);
	}
}

void GraphicBehaviourCasual::buttonUp(std::string bn) {
	// NosuchDebug("GraphicBehaviourCasual::buttonUp bn=%d",bn);
	if ( bn != "UR3" ) {
		return;
	}
	if ( ! palette()->isButtonUsed(bn) ) {
		int selected = bn_to_selected(bn);
		graphic_and_effect_select(paletteHost(),palette(),selected);
	} else {
		NosuchDebug(1,"UR3 was used");
	}
}
	
// ======================================================
// ======================================================

GraphicBehaviourBurn::GraphicBehaviourBurn(Region* r) : GraphicBehaviourDefault(r) {
}

bool GraphicBehaviourBurn::isSelectorDown() {
	// NO selectors
	return false;
}
bool GraphicBehaviourBurn::isMyButton(std::string bn) {
	if ( bn=="LR1" || bn=="LR2" || bn=="LR3"
		|| bn=="UR1" || bn=="UR2" || bn=="UR3"
		|| bn=="LL1" || bn=="LL2" || bn=="LL3"
		|| bn=="UL1" || bn=="UL2" || bn=="UL3"
		) {
		return true;
	} else {
		NosuchDebug("Hmmm, GraphicBehaviourBurn got unexpected bn=%s",bn.c_str());
		return false;
	}
}

void GraphicBehaviourBurn::cursorDownWithSelector(Cursor* c) {
	int selected = SelectionNumber(c);
	NosuchDebug("GraphicBehaviourBurn, cursorDownWithSelector!? selected=%d",selected);
}

void GraphicBehaviourBurn::cursorDragWithSelector(Cursor* c) {
	NosuchDebug("Cursor drag with Selector?!");
}

void GraphicBehaviourBurn::cursorUpWithSelector(Cursor* c) {
	NosuchDebug("Cursor Up with Selector?!");
}

int burn_bn_to_selected(std::string bn) {
	if ( bn == "LL1" ) return 0;
	if ( bn == "LL2" ) return 1;
	if ( bn == "LL3" ) return 2;
	if ( bn == "UL1" ) return 3;
	if ( bn == "UL2" ) return 4;
	if ( bn == "UL3" ) return 5;
	if ( bn == "LR1" ) return 6;
	if ( bn == "LR2" ) return 7;
	if ( bn == "LR3" ) return 8;
	if ( bn == "UR1" ) return 9;
	if ( bn == "UR2" ) return 10;
	if ( bn == "UR3" ) return 11;
	return 0;
}

void GraphicBehaviourBurn::buttonDown(std::string bn) {
	int selected = burn_bn_to_selected(bn);
	NosuchDebug("GraphicBehaviourBurn::buttonDown selected=%d",selected);
	burn_graphic_and_effect_select(paletteHost(),palette(),selected,bn);
}

void GraphicBehaviourBurn::buttonUp(std::string bn) {
	NosuchDebug(1,"GraphicBehaviourBurn::buttonUp bn=%d",bn);
}
	
// ======================================================
// ======================================================

GraphicBehaviour2013::GraphicBehaviour2013(Region* r) : GraphicBehaviourDefault(r) {
}

bool GraphicBehaviour2013::isSelectorDown() {
	// NO selectors
	return false;
}
bool GraphicBehaviour2013::isMyButton(std::string bn) {
	if ( bn=="LR1" || bn=="LR2" || bn=="LR3"
		|| bn=="UR1" || bn=="UR2" || bn=="UR3"
		|| bn=="LL1" || bn=="LL2" || bn=="LL3"
		|| bn=="UL1" || bn=="UL2" || bn=="UL3"
		) {
		return true;
	} else {
		NosuchDebug("Hmmm, GraphicBehaviour2013 got unexpected bn=%s",bn.c_str());
		return false;
	}
}

void GraphicBehaviour2013::cursorDownWithSelector(Cursor* c) {
	int selected = SelectionNumber(c);
	NosuchDebug("GraphicBehaviour2013, cursorDownWithSelector!? selected=%d",selected);
}

void GraphicBehaviour2013::cursorDragWithSelector(Cursor* c) {
	NosuchDebug("Cursor drag with Selector?!");
}

void GraphicBehaviour2013::cursorUpWithSelector(Cursor* c) {
	NosuchDebug("Cursor Up with Selector?!");
}

void GraphicBehaviour2013::buttonDown(std::string bn) {
	int selected = burn_bn_to_selected(bn);
	NosuchDebug("GraphicBehaviour2013::buttonDown selected=%d",selected);
	burn_graphic_and_effect_select(paletteHost(),palette(),selected,bn);
}

void GraphicBehaviour2013::buttonUp(std::string bn) {
	NosuchDebug(1,"GraphicBehaviour2013::buttonUp bn=%d",bn);
}
	
// ======================================================
