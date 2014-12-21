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

GraphicBehaviour::GraphicBehaviour(Region* r) : Behaviour(r->palette(),r) {
}

void GraphicBehaviour::cursorDown(VizCursor* c) {
	VizSprite* s = vizlet()->makeAndInitVizSprite(region()->regionParams(),c->pos);
	region()->AddVizSprite(s);
}

void GraphicBehaviour::cursorDrag(VizCursor* c) {
	VizSprite* s = vizlet()->makeAndInitVizSprite(region()->regionParams(),c->pos);
	region()->AddVizSprite(s);
}

void GraphicBehaviour::cursorUp(VizCursor* c) {
	DEBUGPRINT1(("GraphicBehaviour::cursorup! sid=%d",c->sid));
}

void GraphicBehaviour::advanceTo(int tm) {

	bool gotexception = false;
	try {
		CATCH_NULL_POINTERS;
		for ( std::list<VizCursor*>::iterator i = region()->cursors().begin(); i!=region()->cursors().end(); i++ ) {
			VizCursor* c = *i;

			NosuchAssert(c);
			// c->advanceTo(tm);
			DEBUGPRINT(("GraphicBehaviour adding sprite at %.4f %.4f",c->pos.x,c->pos.y));
			vizlet()->makeAndAddVizSprite(region()->regionParams(),c->pos);

		}
	} catch (NosuchException& e ) {
		NosuchDebug("NosuchException in GraphicBehaviour::advanceto : %s",e.message());
		gotexception = true;
	} catch (...) {
		NosuchDebug("UNKNOWN Exception in GraphicBehaviour::advanceto!");
		gotexception = true;
	}
}

typedef struct select_info {
	char *label;
	int x, y;
} select_info;

#if 0
void
graphic_and_effect_select(PaletteHost* phost, Palette* palette, Patch* patch)
{
	palette->LoadEffectSet(patch->getEffects());

#if 0
	palette->CurrentBurnGraphic = selected % palette->NumBurnGraphics;
	NosuchDebug(1,"GRAPHIC SELECTION! n=%d",palette->CurrentBurnGraphic);
	palette->ConfigLoad(BurnGraphics[palette->CurrentBurnGraphic]);
	std::string t = NosuchSnprintf("selected=%d",selected);
	select_info* si = &burn_select[selected%NUM_EFFECT_SETS];
	phost->ShowText(si->label,si->x,si->y,1000);
#endif
}
#endif

int bn_to_selected(std::string bn) {
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

bool GraphicBehaviour::isMyButton(std::string bn) {
	if ( bn=="LR1" || bn=="LR2" || bn=="LR3"
		|| bn=="UR1" || bn=="UR2" || bn=="UR3"
		|| bn=="LL1" || bn=="LL2" || bn=="LL3"
		|| bn=="UL1" || bn=="UL2" || bn=="UL3"
		) {
		return true;
	} else {
		NosuchDebug("Hmmm, GraphicBehaviour got unexpected bn=%s",bn.c_str());
		return false;
	}
}

void GraphicBehaviour::buttonDown(std::string bn) {
	int selected = bn_to_selected(bn);
	std::string p = region()->patch();
	Patch* patch = palette()->GetPatchNamed(p);
	palette()->LoadEffectSet(patch->getEffects());
}

void GraphicBehaviour::buttonUp(std::string bn) {
	NosuchDebug(1,"GraphicBehaviour::buttonUp bn=%d",bn);
}
	