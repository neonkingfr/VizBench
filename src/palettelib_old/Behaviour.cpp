#include "PaletteAll.h"

Behaviour::Behaviour(Region* r) {
	_region = r;
	_palette = _region->palette;
	_paletteHost = _palette->paletteHost();
}

Palette* Behaviour::palette() {
	NosuchAssert(_palette);
	return _palette;
}

bool Behaviour::isButtonDown(std::string bn) {
	return palette()->isButtonDown(bn);
}

PaletteHost* Behaviour::paletteHost() {
	NosuchAssert(_paletteHost);
	return _paletteHost;
}

Region* Behaviour::region() {
	NosuchAssert(_region);
	return _region;
}

std::list<Cursor*>& Behaviour::cursors() {
	return region()->cursors();
}

int Behaviour::SelectionNumber(Cursor* c) {
	// This code assumes that the big Space Palette regions are as follows:
	// region 1 is the lower one, region 2 is the left one,
	// region 3 is the right one, and region 4 is the top one.
	switch (c->region()->id) {
	case 1: return c->isRightSide() ? 1 : 0;
	case 2: return c->isRightSide() ? 3 : 2;
	case 3: return c->isRightSide() ? 5 : 4;
	case 4: return c->isRightSide() ? 7 : 6;
	}
	return -1;
}

