#include "PaletteAll.h"

Behaviour::Behaviour(Palette* p,Region* r) {
	_palette = p;
	_region = r;
	_paletteHost = p->paletteHost();
	_vizserver = VizServer::GetServer();
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

AllVizParams* Behaviour::defaultParams() {
	return palette()->vizlet()->defaultParams();
}

AllVizParams* Behaviour::regionParams() {
	return region()->regionParams();
}

Vizlet* Behaviour::vizlet() {
	return palette()->vizlet();
}

std::list<VizCursor*>& Behaviour::cursors() {
	return region()->cursors();
}