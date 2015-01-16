#include <pthread.h>

#include <iostream>
#include <sstream>
#include <fstream>
#include <strstream>
#include <cstdlib> // for srand, rand
#include <ctime>   // for time
#include <sys/stat.h>

#include "UT_SharedMem.h"
#include "PaletteAll.h"
#include "Scale.h"
#include "osc/OscOutboundPacketStream.h"
#include "NosuchJSON.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
//  Plugin information
////////////////////////////////////////////////////////////////////////////////////////////////////

bool PaletteHost::StaticInitialized = false;

int PaletteHost::CurrentClick() {
	return _vizserver->CurrentClick();
}

std::string PaletteHost::ParamsFileName(std::string name) {
	return NosuchFullPath("..\\config\\params\\"+name);
}

void PaletteHost::LoadEffectSet(std::string effects) {
	NosuchDebug(1,"PaletteHost::LoadEffectSet effects=%s",effects.c_str());
	std::vector<std::string> efflist = NosuchSplitOnString(effects,",");
	for ( unsigned int a=0; a<_alleffects.size(); a++ ) {
		std::string e = _alleffects[a];
		bool found = false;
		for ( unsigned int i=0; i<efflist.size(); i++ ) {
			if ( e == efflist[i] ) {
				found = true;
				break;
			}
		}
		if ( found ) {
			EnableEffect(e,true);
		} else {
			EnableEffect(e,false);
		}
	}
}

void PaletteHost::StaticInitialization()
{
	if ( StaticInitialized ) {
		return;
	}
	StaticInitialized = true;

	srand( Pt_Time() );

	// Default debugging stuff
	NosuchDebugLevel = 0;   // 0=minimal messages, 1=more, 2=extreme
	NosuchDebugTimeTag = true;
	NosuchDebugThread = true;
	NosuchDebugToConsole = true;
	NosuchDebugToLog = true;
	// NosuchAppName = "Palette";
#ifdef DEBUG_TO_BUFFER
	NosuchDebugToBuffer = true;
#endif
	NosuchDebugAutoFlush = true;
	NosuchDebugLogFile = NosuchFullPath("palette.debug");
	
	NosuchDebug(1,"=== PaletteHost Static Initialization!");
}

PaletteHost* RealPaletteHost = NULL;

AllVizParams* PaletteHost::defaultParams() {
	return vizlet()->defaultParams();
}

AllVizParams* PaletteHost::regionParamsOf(std::string name) {
	Region* r = _palette->GetRegionNamed(name);
	return r->regionParams();
}

PaletteHost::PaletteHost(Vizlet* v)
{
	_vizlet = v;
	frame = 0;
	_vizserver = VizServer::GetServer();

	NosuchDebug(1,"=== PaletteHost is being constructed.");

	_configpath = ManifoldPath("config/palette.json");


	_do_tonicchange = false;
	_tonicchangeclients = "";

	_dotest = FALSE;
	_textEraseTime = 0;
	// _sharedmem_outlines = NULL;
	// _sharedmem_last_attempt = 0;

	NosuchLockInit(&palette_mutex,"palette");

	m_filled = false;
	m_stroked = false;

	disabled = false;
	disable_on_exception = false;

	NosuchErrorPopup = PaletteHost::ErrorPopup; // so that errors in LoadGlobalDefaults will pop up.

	_palette = new Palette(this);

	initStuff1();
}

Vizlet* PaletteHost::vizlet() {
	return _vizlet;
}

bool
PaletteHost::initStuff1() {

	LoadGlobalDefaults();

	if ( disabled ) {
		NosuchDebug("Hey!, LoadGlobalDefaults disabled plugin!");
		return false;
	}

	if ( _do_errorpopup ) {
		NosuchErrorPopup = PaletteHost::ErrorPopup;
	} else {
		NosuchErrorPopup = NULL;
	}

	// openSharedMemOutlines();

	initialized = false;

	width = 1.0f;
	height = 1.0f;

	// Don't do any OpenGL calls here, it isn't initialized yet.
	return true;
}

PaletteHost::~PaletteHost()
{
	NosuchDebug(1,"PaletteHost destructor called");
}

void PaletteHost::ErrorPopup(const char* msg) {
		MessageBoxA(NULL,msg,"Palette",MB_OK);
}

void
PaletteHost::LoadGlobalDefaults()
{
	// These are default values, which can be overridden by the config file.

	_do_errorpopup = true;
	// _patchFile = "default.mnf";

	_vizserver_osc_host = "127.0.0.1";  // This should always be the case
	_vizserver_osc_port = DEFAULT_VIZSERVER_OSC_PORT;

	_pyffle_osc_host = "127.0.0.1";  // This should always be the case
	_pyffle_osc_port = DEFAULT_PYFFLE_PORT;

	// Config file can override those values
	std::ifstream f;
	std::string fname = _configpath;

	f.open(fname.c_str());
	if ( ! f.good() ) {
		std::string err = NosuchSnprintf("No Palette config file (%s), disabling Freeframe plugin!\n",fname.c_str());
		disabled = true;
		NosuchErrorOutput("%s",err.c_str());  // avoid re-interpreting %'s and \\'s in name
		return;
	}

	NosuchDebug("Loading config=%s\n",fname.c_str());
	std::string line;
	std::string jstr;
	while ( getline(f,line) ) {
		// Delete anything after a # (i.e. comments)
		std::string::size_type pound = line.find("#");
		if ( pound != line.npos ) {
			line = line.substr(0,pound);
		}
		if ( line.find_last_not_of(" \t\n") == line.npos ) {
			NosuchDebug(1,"Ignoring blank/comment line=%s\n",line.c_str());
			continue;
		}
		jstr += line;
	}
	f.close();

	if ( ! LoadPaletteConfig(jstr) ) {
		std::string err = NosuchSnprintf("Unable to parse config file (%s), disabling Freeframe plugin!\n",fname.c_str());
		NosuchErrorOutput("%s",err.c_str());  // avoid re-interpreting %'s and \\'s in name
		disabled = true;
		return;
	}
}

static cJSON *
getNumber(cJSON *json,char *name)
{
	cJSON *j = cJSON_GetObjectItem(json,name);
	if ( j && j->type == cJSON_Number )
		return j;
	return NULL;
}

static cJSON *
getString(cJSON *json,char *name)
{
	cJSON *j = cJSON_GetObjectItem(json,name);
	if ( j && j->type == cJSON_String )
		return j;
	return NULL;
}

static cJSON *
getArray(cJSON *json,char *name)
{
	cJSON *j = cJSON_GetObjectItem(json,name);
	if ( j && j->type == cJSON_Array )
		return j;
	return NULL;
}

static cJSON *
getObject(cJSON *json,char *name)
{
	cJSON *j = cJSON_GetObjectItem(json,name);
	if ( j && j->type == cJSON_Object )
		return j;
	return NULL;
}

bool
PaletteHost::LoadPaletteConfig(std::string jstr)
{
	DEBUGPRINT1(("PaletteHost::LoadPaletteConfig!!"));
	cJSON *json = cJSON_Parse(jstr.c_str());
	if ( ! json ) {
		NosuchDebug("Unable to parse json for config!?  json= %s\n",jstr.c_str());
		return false;
	}

	cJSON *j;

	if ( (j=getNumber(json,"sharedmem")) != NULL ) {
		_do_sharedmem = (j->valueint != 0);
	}
	if ( (j=getString(json,"sharedmemname")) != NULL ) {
		_sharedmemname = std::string(j->valuestring);
	}
	if ( (j=getString(json,"midiinput")) != NULL ) {
		_midi_input_list = std::string(j->valuestring);
	}
	if ( (j=getString(json,"midioutput")) != NULL ) {
		_midi_output_list = std::string(j->valuestring);
	}
	if ( (j=getNumber(json,"tonicchange")) != NULL ) {
		_do_tonicchange = (j->valueint != 0);
	}
	if ( (j=getString(json,"tonicchangeclients")) != NULL ) {
		_tonicchangeclients = std::string(j->valuestring);
		std::vector<std::string> clientlist = NosuchSplitOnAnyChar(_tonicchangeclients,",");
		_tonicchangeclienthost.clear();
		_tonicchangeclientport.clear();
		for ( unsigned int i=0; i<clientlist.size(); i++ ) {
			int atpos = clientlist[i].find("@");
			if ( atpos > 0 ) {
				_tonicchangeclientport.push_back(atoi(clientlist[i].substr(0,atpos).c_str()));
				_tonicchangeclienthost.push_back(clientlist[i].substr(atpos+1).c_str());
			}
		}
	}
	if ( (j=getNumber(json,"errorpopup")) != NULL ) {
		_do_errorpopup = (j->valueint != 0);
	}
	if ( (j=getNumber(json,"vizserver_osc_port")) != NULL ) {
		_vizserver_osc_port = j->valueint;
	}
	if ( (j=getString(json,"midiconfig")) != NULL || (j=getString(json,"synthconfig")) != NULL ) {
		_midiconfigFile = std::string(j->valuestring);
	}
	if ( (j=getString(json,"alleffects")) != NULL ) {
		_alleffects = NosuchSplitOnString(j->valuestring,",");
	}
	if ( (j=getString(json,"defaultpatch")) != NULL ) {
		_defaultPatch = std::string(j->valuestring);
	}
	if ( (j=getString(json,"patch")) != NULL ) {
		// _patchFile = std::string(j->valuestring);
		NosuchDebug("ALERT: 'patch' value in config is no longer used?");
	}
	if ( (j=getNumber(json,"debugtoconsole")) != NULL ) {
		NosuchDebugToConsole = j->valueint?TRUE:FALSE;
	}
	if ( (j=getNumber(json,"debugtolog")) != NULL ) {
		bool b = j->valueint?TRUE:FALSE;
		// If we're turning debugtolog off, put a final
		// message out so we know that!
		if ( NosuchDebugToLog && !b ) {
			NosuchDebug("ALERT: NosuchDebugToLog is being set to false!");
		}
		NosuchDebugToLog = b;
	}
	if ( (j=getNumber(json,"debugautoflush")) != NULL ) {
		NosuchDebugAutoFlush = j->valueint?TRUE:FALSE;
	}
	if ( (j=getString(json,"debuglogfile")) != NULL ) {
		NosuchDebugSetLogDirFile(NosuchDebugLogDir,std::string(j->valuestring));
	}
	if ( (j=getObject(json,"regions")) != NULL ) {
		for ( cJSON* c=j->child; c != NULL; c=c->next ) {
			// Even though the value is normally an integer,
			// I'm making it a string so that in the future
			// the sid range can be specified with a string
			// like "211000-211032" (rather than the current
			// hardcoded value of 32, below)
			if ( c->type != cJSON_Object ) {
				NosuchDebug("Hey, value in 'regions' isn't an object?");
			} else {
				std::string name = c->string;
				cJSON* sidobj;
				if ( (sidobj=getString(c,"sid")) == NULL ) {
					NosuchDebug("Hey, no 'sid' value in regions?");
				} else {
					int sidnum = atoi(sidobj->valuestring);
					Region* rgn;
					rgn = _palette->NewRegionNamed(name,sidnum,sidnum+32);
					NosuchDebug("New Region name=%s sidnum=%d rid=%d",name.c_str(),sidnum,rgn->id);
				}
			}
		}
	}
	if ( (j=getObject(json,"patches")) != NULL ) {
		for ( cJSON* c=j->child; c != NULL; c=c->next ) {
			if ( c->type != cJSON_Object ) {
				NosuchDebug("Hey, value in 'patches' isn't an object?");
			} else {
				std::string patchname = c->string;
				Patch* patch = _palette->NewPatchNamed(patchname);
				for ( cJSON* c2=c->child; c2 != NULL; c2=c2->next ) {
					std::string name2 = c2->string;
					if ( name2 == "regions" ) {
						if ( c2->type != cJSON_Object ) {
							NosuchDebug("Hey, regions value in 'patches' isn't an object?");
						}
						for ( cJSON* c3=c2->child; c3 != NULL; c3=c3->next ) {
							std::string regionname = c3->string;
							if ( c3->type != cJSON_String ) {
								NosuchDebug("Hey, region value in %s patch isn't a string?",patchname.c_str());
							} else {
								patch->setRegionParamPath(regionname, c3->valuestring);
							}
						}
					} else if ( name2 == "labelpos" ) {
						if ( c2->type != cJSON_Array ) {
							NosuchDebug("Hey, labelpos value in 'patches' isn't an array?");
						}
					} else if ( name2 == "label" ) {
						if ( c2->type != cJSON_String ) {
							NosuchDebug("Hey, label value in %s patch isn't a string?",patchname.c_str());
						}
					} else if ( name2 == "effects" ) {
						if ( c2->type != cJSON_String ) {
							NosuchDebug("Hey, effects value in %s patch isn't a string?",patchname.c_str());
						} else {
							patch->setEffects(c2->valuestring);
						}
					} else if ( name2 == "channels" ) {
						NosuchDebug("The 'channels' value in %s patch needs work",patchname.c_str());
					} else {
						NosuchDebug("Unrecognized value '%s' in patch?",name2.c_str());
					}
				}
			}
		}
	}
	if ( (j=getObject(json,"buttons")) != NULL ) {
		for ( cJSON* c=j->child; c != NULL; c=c->next ) {
			if ( c->type != cJSON_Object ) {
				NosuchDebug("Hey, value in 'sids' isn't an object?");
			} else {
				std::string name = c->string;
				cJSON* sid = getString(c,"sid");
				NosuchAssert(sid);
				cJSON* patch = getString(c,"patch");
				NosuchAssert(patch);
				int sidnum = atoi(sid->valuestring);
				std::string patchname = patch->valuestring;
				Region* rgn;
				rgn = _palette->NewButtonNamed(name,sidnum,sidnum+32,patchname);
				NosuchDebug("New Button Region name=%s sidnum=%d rid=%d",
					name.c_str(),sidnum,rgn->id);
			}
		}
	}
	return true;
}

int
PaletteHost::EnableEffect(std::string effect, bool enabled)
{
    char buffer[1024];
    osc::OutboundPacketStream p( buffer, sizeof(buffer) );
	const char *meth = enabled ? "ffff.enable" : "ffff.disable";
	std::string jsonstr = NosuchSnprintf("{ \"instance\":\"%s\" }",effect.c_str());
    p << osc::BeginMessage("/api") << meth << jsonstr.c_str() << osc::EndMessage;
    return SendToVizServer(p);
}

int PaletteHost::SendToVizServer(osc::OutboundPacketStream& p) {
	NosuchDebug(1,"SendToVizServer host=%s port=%d",_vizserver_osc_host.c_str(),_vizserver_osc_port);
    return SendToUDPServer(_vizserver_osc_host,_vizserver_osc_port,p.Data(),(int)p.Size());
}

void
PaletteHost::ShowText(std::string text, int x, int y, int timeout) {
    char buffer[1024];
    osc::OutboundPacketStream p( buffer, sizeof(buffer) );
    p << osc::BeginMessage( "/set_text" ) << text.c_str() << osc::EndMessage;
    SendToUDPServer(_pyffle_osc_host,_pyffle_osc_port,p.Data(),(int)p.Size());
    p.Clear();
    p << osc::BeginMessage( "/set_pos" ) << x << y << osc::EndMessage;
    SendToUDPServer(_pyffle_osc_host,_pyffle_osc_port,p.Data(),(int)p.Size());
	if ( timeout > 0 ) {
		_textEraseTime = Pt_Time() + timeout;
	}
}

void
PaletteHost::CheckTimeouts(int millinow) {
	// This is currently called from the network thread.
	if ( _textEraseTime > 0 && millinow > _textEraseTime ) {
		NosuchDebug(1,"Erasing Text!");
		_textEraseTime = 0;
		ShowText("",0,0,0);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//  Methods
////////////////////////////////////////////////////////////////////////////////////////////////////

void PaletteHost::rect(double x, double y, double w, double h) {
	// if ( w != 2.0f || h != 2.0f ) {
	// 	NosuchDebug("Drawing rect xy = %.3f %.3f  wh = %.3f %.3f",x,y,w,h);
	// }
	quad(x,y, x+w,y,  x+w,y+h,  x,y+h);
}
void PaletteHost::fill(NosuchColor c, double alpha) {
	m_filled = true;
	m_fill_color = c;
	m_fill_alpha = alpha;
}
void PaletteHost::stroke(NosuchColor c, double alpha) {
	// glColor4d(c.r()/255.0f, c.g()/255.0f, c.b()/255.0f, alpha);
	m_stroked = true;
	m_stroke_color = c;
	m_stroke_alpha = alpha;
}
void PaletteHost::noStroke() {
	m_stroked = false;
}
void PaletteHost::noFill() {
	m_filled = false;
}
void PaletteHost::background(int b) {
	NosuchDebug("PaletteHost::background!");
}
void PaletteHost::strokeWeight(double w) {
	glLineWidth((GLfloat)w);
}
void PaletteHost::rotate(double degrees) {
	glRotated(-degrees,0.0f,0.0f,1.0f);
}
void PaletteHost::translate(double x, double y) {
	glTranslated(x,y,0.0f);
}
void PaletteHost::scale(double x, double y) {
	glScaled(x,y,1.0f);
	// NosuchDebug("SCALE xy= %f %f",x,y);
}
void PaletteHost::quad(double x0, double y0, double x1, double y1, double x2, double y2, double x3, double y3) {
	NosuchDebug(2,"   Drawing quad = %.3f %.3f, %.3f %.3f, %.3f %.3f, %.3f %.3f",x0,y0,x1,y1,x2,y2,x3,y3);
	if ( m_filled ) {
		glBegin(GL_QUADS);
		NosuchColor c = m_fill_color;
		glColor4d(c.r()/255.0f, c.g()/255.0f, c.b()/255.0f, m_fill_alpha);
		glVertex2d( x0, y0); 
		glVertex2d( x1, y1); 
		glVertex2d( x2, y2); 
		glVertex2d( x3, y3); 
		glEnd();
	}
	if ( m_stroked ) {
		NosuchColor c = m_stroke_color;
		glColor4d(c.r()/255.0f, c.g()/255.0f, c.b()/255.0f, m_stroke_alpha);
		glBegin(GL_LINE_LOOP); 
		glVertex2d( x0, y0); 
		glVertex2d( x1, y1); 
		glVertex2d( x2, y2); 
		glVertex2d( x3, y3); 
		glEnd();
	}
	if ( ! m_filled && ! m_stroked ) {
		NosuchDebug("Hey, quad() called when both m_filled and m_stroked are off!?");
	}
}
void PaletteHost::triangle(double x0, double y0, double x1, double y1, double x2, double y2) {
	NosuchDebug(2,"Drawing triangle xy0=%.3f,%.3f xy1=%.3f,%.3f xy2=%.3f,%.3f",x0,y0,x1,y1,x2,y2);
	if ( m_filled ) {
		NosuchColor c = m_fill_color;
		glColor4d(c.r()/255.0f, c.g()/255.0f, c.b()/255.0f, m_fill_alpha);
		NosuchDebug(2,"   fill_color=%d %d %d alpha=%.3f",c.r(),c.g(),c.b(),m_fill_alpha);
		glBegin(GL_TRIANGLE_STRIP); 
		glVertex3d( x0, y0, 0.0f );
		glVertex3d( x1, y1, 0.0f );
		glVertex3d( x2, y2, 0.0f );
		glEnd();
	}
	if ( m_stroked ) {
		NosuchColor c = m_stroke_color;
		glColor4d(c.r()/255.0f, c.g()/255.0f, c.b()/255.0f, m_stroke_alpha);
		NosuchDebug(2,"   stroke_color=%d %d %d alpha=%.3f",c.r(),c.g(),c.b(),m_stroke_alpha);
		glBegin(GL_LINE_LOOP); 
		glVertex2d( x0, y0); 
		glVertex2d( x1, y1);
		glVertex2d( x2, y2);
		glEnd();
	}
	if ( ! m_filled && ! m_stroked ) {
		NosuchDebug("Hey, triangle() called when both m_filled and m_stroked are off!?");
	}
}

void PaletteHost::line(double x0, double y0, double x1, double y1) {
	// NosuchDebug("Drawing line xy0=%.3f,%.3f xy1=%.3f,%.3f",x0,y0,x1,y1);
	if ( m_stroked ) {
		NosuchColor c = m_stroke_color;
		glColor4d(c.r()/255.0f, c.g()/255.0f, c.b()/255.0f, m_stroke_alpha);
		// NosuchDebug(2,"   stroke_color=%d %d %d alpha=%.3f",c.r(),c.g(),c.b(),m_stroke_alpha);
		glBegin(GL_LINES); 
		glVertex2d( x0, y0); 
		glVertex2d( x1, y1);
		glEnd();
	} else {
		NosuchDebug("Hey, line() called when m_stroked is off!?");
	}
}

static double degree2radian(double deg) {
	return 2.0f * (double)M_PI * deg / 360.0f;
}

void PaletteHost::ellipse(double x0, double y0, double w, double h) {
	NosuchDebug(2,"Drawing ellipse xy0=%.3f,%.3f wh=%.3f,%.3f",x0,y0,w,h);
	if ( m_filled ) {
		NosuchColor c = m_fill_color;
		glColor4d(c.r()/255.0f, c.g()/255.0f, c.b()/255.0f, m_fill_alpha);
		NosuchDebug(2,"   fill_color=%d %d %d alpha=%.3f",c.r(),c.g(),c.b(),m_fill_alpha);
		glBegin(GL_TRIANGLE_FAN);
		double radius = w;
		glVertex2d(x0, y0);
		for ( double degree=0.0f; degree <= 360.0f; degree+=5.0f ) {
			glVertex2d(x0 + sin(degree2radian(degree)) * radius, y0 + cos(degree2radian(degree)) * radius);
		}
		glEnd();
	}
	if ( m_stroked ) {
		NosuchColor c = m_stroke_color;
		glColor4d(c.r()/255.0f, c.g()/255.0f, c.b()/255.0f, m_stroke_alpha);
		NosuchDebug(2,"   stroke_color=%d %d %d alpha=%.3f",c.r(),c.g(),c.b(),m_stroke_alpha);
		glBegin(GL_LINE_LOOP);
		double radius = w;
		for ( double degree=0.0f; degree <= 360.0f; degree+=5.0f ) {
			glVertex2d(x0 + sin(degree2radian(degree)) * radius, y0 + cos(degree2radian(degree)) * radius);
		}
		glEnd();
	}

	if ( ! m_filled && ! m_stroked ) {
		NosuchDebug("Hey, ellipse() called when both m_filled and m_stroked are off!?");
	}
}

void PaletteHost::polygon(PointMem* points, int npoints) {
	if ( m_filled ) {
		NosuchColor c = m_fill_color;
		glColor4d(c.r()/255.0f, c.g()/255.0f, c.b()/255.0f, m_fill_alpha);
		glBegin(GL_TRIANGLE_FAN);
		glVertex2d(0.0, 0.0);
		for ( int pn=0; pn<npoints; pn++ ) {
			PointMem* p = &points[pn];
			glVertex2d(p->x,p->y);
		}
		glEnd();
	}
	if ( m_stroked ) {
		NosuchColor c = m_stroke_color;
		glColor4d(c.r()/255.0f, c.g()/255.0f, c.b()/255.0f, m_stroke_alpha);
		glBegin(GL_LINE_LOOP);
		for ( int pn=0; pn<npoints; pn++ ) {
			PointMem* p = &points[pn];
			glVertex2d(p->x,p->y);
		}
		glEnd();
	}

	if ( ! m_filled && ! m_stroked ) {
		NosuchDebug("Hey, ellipse() called when both m_filled and m_stroked are off!?");
	}
}

void PaletteHost::popMatrix() {
	glPopMatrix();
}

void PaletteHost::pushMatrix() {
	glPushMatrix();
}

#define RANDONE (((double)rand())/RAND_MAX)
#define RANDB ((((double)rand())/RAND_MAX)*2.0f-1.0f)

void
PaletteHost::test_draw()
{
	for ( int i=0; i<1000; i++ ) {
		glColor4d(RANDONE,RANDONE,RANDONE,RANDONE);
		glBegin(GL_QUADS);
		glVertex2d(RANDB,RANDB);
		glVertex2d(RANDB,RANDB);
		glVertex2d(RANDB,RANDB);
		glVertex2d(RANDB,RANDB);
		glVertex2d(RANDB,RANDB);
		glEnd();
	}
}

static bool
istrue(std::string s)
{
	return(s == "true" || s == "True" || s == "1");
}

void PaletteHost::initMidiConfig() {
	std::string midipath = NosuchFullPath("..\\config\\midi\\"+_midiconfigFile);
	if ( ! ends_with(midipath,".mnf") ) {
		midipath += ".mnf";
	}
	NosuchDebug("Reading midiconfig from %s",midipath.c_str());
	std::ifstream f(midipath.c_str(), std::ifstream::in | std::ifstream::binary);
	if ( ! f.is_open() ) {
		NosuchErrorOutput("Unable to open MIDI config file: %s",midipath.c_str());
	} else {
		readMidiConfig(f);
		f.close();
	}
	DEBUGPRINT1(("After reading PaletteHost configuration from %s",midipath.c_str()));
}

// Return everything after the '=' (and whitespace)
std::string
everything_after_char(std::string line, char lookfor = '=')
{
	const char *p = line.c_str();
	const char *q = strchr(p,lookfor);
	if ( q == NULL ) {
		NosuchDebug("Invalid format (no =): %s",p);
		return "";
	}
	q++;
	while ( *q != 0 && isspace(*q) ) {
		q++;
	}
	size_t len = strlen(q);
	if ( q[len-1] == '\r' ) {
		len--;
	}
	return std::string(q,len);
}

void PaletteHost::readMidiConfig(std::ifstream& f) {

	std::string line;
	std::string current_synth = "";
	int current_soundset = -1;
	std::string current_soundbank_name;
	int current_soundbank = -1;

	while (!std::getline(f,line,'\n').eof()) {
		std::vector<std::string> words = NosuchSplitOnAnyChar(line," \t\r=");
		if ( words.size() == 0 ) {
			continue;
		}
		std::string word0 = words[0];
		if ( word0 == "" || word0[0] == '#' ) {
			continue;
		}
		std::string word1 = (words.size()>1) ? words[1] : "";
		if ( word0 == "channel" ) {
			if ( words.size() < 3 ) {
				NosuchDebug("Invalid format (too short): %s",line.c_str()); 
				continue;
			}
			int chan = atoi(word1.c_str());
			Synths[chan] = words[2];
		} else if ( word0 == "include" ) {
			if ( words.size() < 2 ) {
				NosuchDebug("Invalid format (too short): %s",line.c_str()); 
				continue;
			}
			// as a hack to eliminate the need for an '=' on the
			// include line, we take everything after the 'e'
			// (last character in "include").
			std::string basename = everything_after_char(line,'e');
			std::string filename = ParamsFileName(basename);
			std::ifstream incf(filename.c_str(), std::ifstream::in | std::ifstream::binary);
			if ( ! incf.is_open() ) {
				if ( basename.find_first_of("local") == 0 ) {
					// This is not necessarily an error
					NosuchDebug("Local include file not defined: %s",filename.c_str());
				} else {
					NosuchErrorOutput("Unable to open include file: %s",filename.c_str());
				}
			} else {
				readMidiConfig(incf);
				incf.close();
			}
		} else if ( word0 == "sid" ) {
			NosuchErrorOutput("sid directive in readMidiConfig is no longer used!");
		} else if ( word0 == "debug" ) {
			if ( words.size() < 3 ) {
				NosuchDebug("Invalid format (too short): %s",line.c_str()); 
				continue;
			}
			if ( word1 == "level" ) {
				NosuchDebugLevel = atoi(everything_after_char(line).c_str());
			} else if ( word1 == "tologfile" ) {
				NosuchDebugToLog = istrue(everything_after_char(line));
			} else if ( word1 == "toconsole" ) {
				NosuchDebugToConsole = istrue(everything_after_char(line));
			} else if ( word1 == "autoflush" ) {
				NosuchDebugAutoFlush = istrue(everything_after_char(line));
			}
		} else if ( word0 == "python" ) {
			NosuchDebug("python in config file is ignored");
		} else if ( word0 == "midi" ) {
				NosuchDebug("midi keyword in *.mnf files is no longer used");
				continue;
		} else if ( word0 == "synth" ) {
			if ( words.size() < 2 ) {
				NosuchDebug("Invalid format (too short): %s",line.c_str()); 
				continue;
			}
			current_synth = word1;
		} else if ( word0 == "program" ) {
			if ( current_synth == "" ) {
				NosuchDebug("Invalid format (program seen before synth): %s",line.c_str()); 
				continue;
			}
			if ( words.size() < 2 ) {
				NosuchDebug("Invalid format (too short): %s",line.c_str()); 
				continue;
			}
			int pnum = atoi(word1.c_str());
			std::string progname = everything_after_char(line);
			NosuchDebug(1,"Program %d = ((%s))",pnum,progname.c_str());
			Sounds[progname] = Sound(current_synth,pnum);
		} else if ( word0 == "soundbank" ) {
			if ( words.size() < 3 ) {
				NosuchDebug("Invalid format (too short): %s",line.c_str()); 
				continue;
			}
			current_soundbank = atoi(word1.c_str());
			NosuchAssert(current_soundbank >=0 && current_soundbank < NUM_SOUNDBANKS);
			current_soundbank_name = words[2];
		} else if ( word0 == "soundset" ) {
			if ( words.size() < 2 ) {
				NosuchDebug("Invalid format (too short): %s",line.c_str()); 
				continue;
			}
			current_soundset = atoi(word1.c_str());
			NosuchAssert(current_soundset >=0 && current_soundset < NUM_SOUNDSETS);
		} else if ( word0 == "sound" ) {
			if ( words.size() < 3 ) {
				NosuchDebug("Invalid format (too short): %s",line.c_str()); 
				continue;
			}
			int soundnum = atoi(word1.c_str());
			NosuchAssert(soundnum >=0 && soundnum < NUM_SOUNDS_IN_SET);
			std::string soundname = everything_after_char(line);
			SoundBank[current_soundbank][current_soundset][soundnum] = soundname;
		} else {
			NosuchDebug("Invalid format (unrecognized first word): %s",line.c_str()); 
			continue;
		}
	}
}

bool PaletteHost::initStuff2() {

	NosuchDebug(1,"PaletteHost::initStuff2 starts");

	// test_stuff();

	bool r = true;
	try {
		// static initializations
		Scale::initialize();
		Sound::initialize();
	
		// Not static, config gets read here.
		initMidiConfig();

		_palette->initRegionSounds();

		_palette->now = Pt_Time();

	} catch (NosuchException& e) {
		NosuchDebug("NosuchException: %s",e.message());
		r = false;
	} catch (...) {
		// Does this really work?  Not sure
		NosuchDebug("Some other kind of exception occured!?");
		r = false;
	}
	NosuchDebug(2,"PaletteHost::initStuff2 returns %s\n",r?"true":"false");
	return r;
}

#if 0
void
PaletteHost::VizCursorDownNotification(VizCursor* c) {
	Region* r = c->region();
	NosuchAssert(r);
	if ( ! r->Looping() ) {
		return;
	}
}

void
PaletteHost::VizCursorLoopNotification(NosuchVizCursorMotion* cm, NosuchLoop* lp) {
	int clk = lp->click();
	SchedEvent* ev = new SchedEvent(cm,clk,lp->id());
	int nn = lp->AddLoopEvent(ev);
}
#endif

#ifdef SAVE_THIS_FOR_processMIDI_IMPLEMENTATION
void
PaletteHost::OutputNotificationMidiMsg(MidiMsg* mm, int sidnum) {
	// The sid can be a TUIO session ID or a loop id
	if ( sidnum < 0 ) {
		NosuchDebug(1,"OutputNotificationMidiMsg ignoring sidnum=%d",sidnum);
		return;
	}
	Region* r = _palette->RegionForSid(sidnum);
	if ( r == NULL ) {
		NosuchDebug(1,"OutputNotificationMidiMsg no region for sid=%d?",sidnum);
		return;
	}
	if ( r->type != Region::SURFACE ) {
		NosuchDebug(1,"OutputNotificationMidiMsg region for sid=%d region?",sidnum);
		return;
	}
	NosuchAssert(r);

	if ( isSidVizCursor(sidnum) ) {
		// If the output is coming from a cursor, then we see if looping is on

		NosuchDebug(2,"PaletteHost OutputNotificationMidiMsg for VizCursor, sid=%d",sidnum);
		r->OutputNotificationMidiMsg(mm,sidnum);
	}
}
#endif

bool PaletteHost::hostProcessDraw()
{
	frame++;

	if ( ! initialized ) {
		NosuchDebug(1,"PaletteHostProcessOpenGL calling initStuff2()");
		if ( ! initStuff2() ) {
			NosuchDebug("PaletteHost::initStuff2 failed, disabling plugin!");
			disabled = true;
			return false;
		}
		initialized = true;
		RealPaletteHost = this;
	}

	// Draw line just to show we're alive
	glColor4d(0.0, 0.0, 1.0, 1.0);
	glBegin(GL_LINES); 
	glVertex2d( 0.1, 0.1); 
	glVertex2d( 0.9, 0.9); 
	glEnd(); 

	bool gotexception = false;
	try {
		CATCH_NULL_POINTERS;

		int tm = _palette->now;
		int begintm = _palette->now;
		int endtm = Pt_Time();
		NosuchDebug(2,"ProcessOpenGL tm=%d endtm=%d dt=%d",tm,endtm,(endtm-tm));

		glDisable(GL_TEXTURE_2D); 
		glEnable(GL_BLEND); 
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 
		glLineWidth((GLfloat)3.0f);

		int ndt = 1;
		int n;
		for ( n=1; n<=ndt; n++ ) {
			tm = (int)(begintm + 0.5 + n * ((double)(endtm-begintm)/(double)ndt));
			if ( tm > endtm ) {
				tm = endtm;
			}
			int r = _palette->draw();

			if ( _dotest ) {
				test_draw();
			}
			// _palette->advanceTo(tm);
			if ( r > 0 ) {
				NosuchDebug("Palette::draw returned failure? (r=%d)\n",r);
				gotexception = true;
				break;
			}
		}
	} catch (NosuchException& e ) {
		NosuchDebug("NosuchException in Palette::draw : %s",e.message());
		gotexception = true;
	} catch (...) {
		NosuchDebug("UNKNOWN Exception in Palette::draw!");
		gotexception = true;
	}

	if ( gotexception && disable_on_exception ) {
		NosuchDebug("DISABLING PaletteHost due to exception!!!!!");
		disabled = true;
	}

	return true;
}

bool has_invalid_char(const char *nm)
{
	for ( const char *p=nm; *p!='\0'; p++ ) {
		if ( ! isalnum(*p) )
			return true;
	}
	return false;
}

#if 0
std::string PaletteHost::jsonDoubleResult(double r, const char *id) {
	return NosuchSnprintf("{ \"jsonrpc\": \"2.0\", \"result\": %f, \"id\": \"%s\" }",r,id);
}

std::string PaletteHost::jsonIntResult(int r, const char *id) {
	return NosuchSnprintf("{ \"jsonrpc\": \"2.0\", \"result\": %d, \"id\": \"%s\" }\r\n",r,id);
}

std::string PaletteHost::jsonStringResult(std::string r, const char *id) {
	return NosuchSnprintf("{ \"jsonrpc\": \"2.0\", \"result\": \"%s\", \"id\": \"%s\" }\r\n",r.c_str(),id);
}
#endif

std::string PaletteHost::jsonMethError(std::string e, const char *id) {
	return jsonError(-32602, e,id);
}

std::string PaletteHost::jsonError(int code, std::string e, const char* id) {
	return NosuchSnprintf("{ \"jsonrpc\": \"2.0\", \"error\": {\"code\": %d, \"message\": \"%s\" }, \"id\":\"%s\" }\r\n",code,e.c_str(),id);
}

std::string PaletteHost::jsonConfigResult(std::string name, const char *id) {

	// Remove the filename suffix on the config name
	int suffindex = name.length() - Palette::configSuffix.length();
	if ( suffindex > 0 && name.substr(suffindex) == Palette::configSuffix ) {
		name = name.substr(0,name.length()-Palette::configSuffix.length());
	}
	return jsonStringResult(name,id);
}

std::string needString(std::string meth,cJSON *params,std::string nm) {

	cJSON *c = cJSON_GetObjectItem(params,nm.c_str());
	if ( ! c ) {
		throw NosuchException("Missing %s argument on %s method",nm.c_str(),meth.c_str());
	}
	if ( c->type != cJSON_String ) {
		throw NosuchException("Unexpected type for %s argument to %s method, expecting string",nm.c_str(),meth.c_str());
	}
	return c->valuestring;
}

std::string needRegionOrMidiNum(std::string meth, cJSON *params, int* idnum) {
	cJSON *cregion = cJSON_GetObjectItem(params,"region");
	cJSON *cmidi = cJSON_GetObjectItem(params,"midi");
	if ( !cregion && !cmidi ) {
		throw NosuchException("Missing region or midi argument on %s method",meth.c_str());
	}
	std::string idtype;
	cJSON *c;
	if ( cregion ) {
		c = cregion;
		idtype = "region";
	} else {
		c = cmidi;
		idtype = "midi";
	}
	if ( c->type != cJSON_Number ) {
		throw NosuchException("Unexpected type for %s argument to %s method, expecting number",idtype.c_str(),meth.c_str());
	}
	*idnum = c->valueint;
	return idtype;
}

int needInt(std::string meth,cJSON *params,std::string nm) {
	cJSON *c = cJSON_GetObjectItem(params,nm.c_str());
	if ( ! c ) {
		throw NosuchException("Missing %s argument on %s method",nm.c_str(),meth.c_str());
	}
	if ( c->type != cJSON_Number ) {
		throw NosuchException("Unexpected type for %s argument to %s method, expecting number",nm.c_str(),meth.c_str());
	}
	return c->valueint;
}

double needDouble(std::string meth,cJSON *params,std::string nm) {
	cJSON *c = cJSON_GetObjectItem(params,nm.c_str());
	if ( ! c ) {
		throw NosuchException("Missing %s argument on %s method",nm.c_str(),meth.c_str());
	}
	if ( c->type != cJSON_Number ) {
		throw NosuchException("Unexpected type for %s argument to %s method, expecting double",nm.c_str(),meth.c_str());
	}
	return (double)(c->valuedouble);
}

void needParams(std::string meth, cJSON* params) {
	if(params==NULL) {
		throw NosuchException("No parameters on %s method?",meth.c_str());
	}
}

std::string PaletteHost::ExecuteJson(std::string meth, cJSON *params, const char *id) {

	static std::string errstr;  // So errstr.c_str() stays around, but I'm not sure that's now needed

	if ( meth == "debug_tail" ) {
#if 0
		cJSON *c_amount = cJSON_GetObjectItem(params,"amount");
		if ( ! c_amount ) {
			return error_json(-32000,"Missing amount argument",id);
		}
		if ( c_amount->type != cJSON_String ) {
			return error_json(-32000,"Expecting string type in amount argument to get",id);
		}
#endif
#ifdef DEBUG_DUMP_BUFFER
		std::string s = NosuchDebugDumpBuffer();
		std::string s2 = NosuchEscapeHtml(s);
		std::string result = 
			"{\"jsonrpc\": \"2.0\", \"result\": \""
			+ s2
			+ "\", \"id\": \""
			+ id
			+ "\"}";
		return(result);
#else
		return error_json(-32000,"DEBUG_DUMP_BUFFER not defined",id);
#endif
	}
	if ( meth == "_echo" || meth == "echo" ) {
		cJSON *c_value = cJSON_GetObjectItem(params,"value");
		if ( ! c_value ) {
			return error_json(-32000,"Missing value argument",id);
		}
		if ( c_value->type != cJSON_String ) {
			return error_json(-32000,"Expecting string type in value argument to echo",id);
		}
		return jsonStringResult(c_value->valuestring,id);
	}
	if (meth == "ANO") {
		vizserver()->ANO();
		return jsonIntResult(0,id);
	}
	if (meth == "clear_all") {
		vizserver()->ANO();
		return jsonIntResult(0,id);
	}
	if (meth == "range_full") {
		palette()->SetAllFullRange(true);
		return jsonIntResult(0,id);
	}
	if (meth == "range_normal") {
		palette()->SetAllFullRange(false);
		return jsonIntResult(0,id);
	}
	if (meth == "quantize_on") {
		NosuchDebug("doquantize is set to true");
		defaultParams()->doquantize.set(true);
		return jsonIntResult(0,id);
	}
	if (meth == "quantize_off") {
		NosuchDebug("doquantize is set to false");
		defaultParams()->doquantize.set(false);
		return jsonIntResult(0,id);
	}
	if (meth == "minmove_zero") {
		NosuchDebug("minmove is set to 0.0");
		for ( size_t i=0; i<palette()->_regions.size(); i++ ) {
			Region* region = palette()->_regions[i];
			region->regionParams()->minmove.set(0.0);
			region->regionParams()->minmovedepth.set(0.0);
		}
		return jsonIntResult(0,id);
	}
	if (meth == "minmove_default") {
		NosuchDebug("minmove is set to 0.05");
		for ( size_t i=0; i<palette()->_regions.size(); i++ ) {
			Region* region = palette()->_regions[i];
			region->regionParams()->minmove.set(0.025);
			region->regionParams()->minmovedepth.set(0.025);
		}
		return jsonIntResult(0,id);
	}
	if (meth == "tempo_slow") {
		vizserver()->SetTempoFactor(2.0);
		return jsonIntResult(0,id);
	}
	if (meth == "tempo_slowest") {
		vizserver()->SetTempoFactor(4.0);
		return jsonIntResult(0,id);
	}
	if (meth == "tempo_normal") {
		vizserver()->SetTempoFactor(1.0);
		return jsonIntResult(0,id);
	}
	if (meth == "tempo_fast") {
		vizserver()->SetTempoFactor(0.5);
		return jsonIntResult(0,id);
	}
	if (meth == "tempo_fastest") {
		vizserver()->SetTempoFactor(0.25);
		return jsonIntResult(0,id);
	}
	if (meth == "tonic_change") {
		MusicBehaviour::tonic_next(_palette);
		return jsonIntResult(0,id);
	}
	if (meth == "tonic_reset") {
		MusicBehaviour::tonic_reset(_palette);
		return jsonIntResult(0,id);
	}
#if 0
	if (meth == "config_load") {
		std::string name = needString(meth,params,"name");
		int r = needInt(meth,params,"region");
		if ( r != -1 ) {
			throw NosuchException("Can't handle config for for non-global region (%d)",r);
		}
		std::string err = _palette->ConfigLoad(name);
		if ( err != "" ) {
			throw NosuchException("Error in ConfigLoad for name=%s, err=%s",name.c_str(),err.c_str());
		}
		return jsonConfigResult(name,id);
	}
#endif
	if (meth == "button") {
		needParams(meth,params);
		std::string name = needString(meth,params,"name");
		Region* r = _palette->GetRegionNamed(name);
		if ( r == NULL ) {
			throw NosuchException("button, unable to find region: %s",name.c_str());
		}
		_palette->buttonDown(name);
		_palette->buttonUp(name);
		return jsonIntResult(0,id);
	}
	if (meth == "set") {
		needParams(meth,params);
		std::string name = needString(meth,params,"name");
		std::string val = needString(meth,params,"value");
		int idnum;
		std::string idtype = needRegionOrMidiNum(meth,params, &idnum);
		NosuchDebug("SET name=%s val=%s idnum=%d",name.c_str(),val.c_str(),idnum);

		std::string s;

		if ( idnum == MAGIC_VAL_FOR_PALETTE_PARAMS ) {
				defaultParams()->Set(name,val);
				s = defaultParams()->GetAsString(name);
		} else if ( idtype == "region" ) {
			int r = idnum;
			if ( r >= 0 && r < (int)_palette->_regions.size() ) {
				_palette->_regions[r]->regionParams()->Set(name,val);
				s = _palette->_regions[r]->regionParams()->GetAsString(name);
			} else {
				throw NosuchException("increment/decrement method - bad %s parameter: %d",idtype.c_str(),r);
			}
		} else {
			throw NosuchException("increment/decrement method - bad idtype: %d",idtype.c_str());
		}
		return jsonStringResult(s,id);
	}

	if (meth == "increment" || meth == "decrement" ) {
		needParams(meth,params);
		std::string name = needString(meth,params,"name");
		double amount = needDouble(meth,params,"amount");
		// int r = needInt(meth,params,"region");

		int idnum;
		std::string idtype = needRegionOrMidiNum(meth,params, &idnum);

		amount = amount * ((meth == "decrement") ? -1 : 1);

		std::string s;

		if ( idnum == MAGIC_VAL_FOR_PALETTE_PARAMS ) {
				defaultParams()->Increment(name,amount);
				s = defaultParams()->GetAsString(name);
		} else if ( idtype == "region" ) {
			int r = idnum;
			if ( r >= 0 && r < (int)_palette->_regions.size() ) {
				_palette->_regions[r]->regionParams()->Increment(name,amount);
				s = _palette->_regions[r]->regionParams()->GetAsString(name);
			} else {
				throw NosuchException("increment/decrement method - bad %s parameter: %d",idtype.c_str(),r);
			}
		} else {
			throw NosuchException("increment/decrement method - bad idtype: %d",idtype.c_str());
		}
		return jsonStringResult(s,id);
	}
	if (meth == "toggle" ) {
		needParams(meth,params);
		std::string name = needString(meth,params,"name");
		// int r = needInt(meth,params,"region");
		int idnum;
		std::string idtype = needRegionOrMidiNum(meth,params, &idnum);

		std::string s;

		if ( idnum == MAGIC_VAL_FOR_PALETTE_PARAMS ) {
			defaultParams()->Toggle(name);
			s = defaultParams()->GetAsString(name);
		} else if ( idtype == "region" ) {
			int r = idnum;
			if ( r >= 0 && r < (int)_palette->_regions.size() ) {
				_palette->_regions[r]->regionParams()->Toggle(name);
				s = _palette->_regions[r]->regionParams()->GetAsString(name);
			} else {
				throw NosuchException("toggle method - bad %s parameter: %d",idtype.c_str(),r);
			}
		} else {
			throw NosuchException("increment/decrement method - bad idtype: %d",idtype.c_str());
		}
		return jsonStringResult(s,id);
	}
	if (meth == "get") {
		needParams(meth,params);
		std::string name = needString(meth,params,"name");

		// int r = needInt(meth,params,"region");
		int idnum;
		std::string idtype = needRegionOrMidiNum(meth,params, &idnum);
		
		std::string s;
		// Really should have a different idtype for palette params...
		if ( idnum == MAGIC_VAL_FOR_PALETTE_PARAMS ) {
				s = defaultParams()->GetAsString(name);
		} else if ( idtype == "region" ) {
			int r = idnum;
			if ( r >= 0 && r < (int)_palette->_regions.size() ) {
				s = _palette->_regions[r]->regionParams()->GetAsString(name);
				NosuchDebug(1,"JSON executed, get of r=%d name=%s s=%s",r,name.c_str(),s.c_str());
			} else {
				throw NosuchException("get method - bad %s parameter: %d",idtype.c_str(),r);
			}
		} else {
			throw NosuchException("increment/decrement method - bad idtype: %d",idtype.c_str());
		}
		return jsonStringResult(s,id);
	}

	errstr = NosuchSnprintf("Unrecognized method name - %s",meth.c_str());
	return error_json(-32000,errstr.c_str(),id);
}

bool
PaletteHost::checkAddrPattern(const char *addr, char *patt)
{
	return ( strncmp(addr,patt,strlen(patt)) == 0 );
}

static double expandfactor = 1.0f;

static void
xyz_adjust(bool switchyz, double& x, double& y, double& z) {
	if ( switchyz ) {
		double t = y;
		y = z;
		z = t;
		z = 1.0 - z;
	}
	// The values we get from the Palette may not go all the way to
	// 0.0 or 1.0, so we can expand
	// the range a bit so people can draw all the way to the edges.
	if ( expandfactor != 1.0f ) {
		NosuchDebug("expandfactor!=1 xyz_adjust orig %.3f %.3f %.3f",x,y,z);
		x = ((x - 0.5f) * expandfactor) + 0.5f;
		y = ((y - 0.5f) * expandfactor) + 0.5f;
		NosuchDebug("    adjusted %.3f %.3f %.3f",x,y,z);
	}
	if (x < 0.0)
		x = 0.0f;
	else if (x > 1.0)
		x = 1.0f;
	if (y < 0.0)
		y = 0.0f;
	else if (y > 1.0)
		y = 1.0f;
}

#if 0
std::string
sidString(int sidnum, const char* source)
{
	if ( strcmp(source,"") == 0 ) {
		return NosuchSnprintf("%d",sidnum);
	} else {
		return NosuchSnprintf("%d/%s",sidnum,source);  // The source has an @ in it already
	}
}
#endif

#ifdef SAVE_FOR_processCURSOR_IMPLEMENTATION
void
PaletteHost::TouchVizCursorSid(int sidnum, const char* sidsource, int millinow)
{
	// std::string sid = sidString(sidnum,source);
	Region* r = _palette->RegionForSid(sidnum);
	if ( r ) {
		r->touchVizCursor(sidnum, sidsource, millinow);
	} else {
		NosuchErrorOutput("Unable to find region (A) for sid=%d/%s",sidnum,sidsource);
	}
}
VizCursor*
PaletteHost::SetVizCursorSid(int sidnum, const char* sidsource, int millinow, NosuchVector point, double depth, double tuio_f, OutlineMem* om)
{
	// std::string sid = sidString(sidnum,source);
	Region* r = _palette->RegionForSid(sidnum);
	if ( r ) {
		return r->setVizCursor(sidnum, sidsource, millinow, point, depth, tuio_f, om);
	} else {
		NosuchErrorOutput("Unable to find region (B) for sid=%d/%s",sidnum,sidsource);
		return NULL;
	}
}

#endif

void PaletteHost::processOsc( const char *source, const osc::ReceivedMessage& m) {
	static int Nprocessed = 0;
	try{
		// DebugOscMessage("ProcessOscMessage ",m);
	    const char *types = m.TypeTags();
		const char *addr = m.AddressPattern();
		int millinow = Pt_Time();
		Nprocessed++;
		NosuchDebug(1,"ProcessOscMessage source=%s millinow=%d addr=%s",
			source==NULL?"NULL?":source,millinow,addr);

		if (checkAddrPattern(addr,"/palette/tonicchange")) {
			int t = ArgAsInt32(m,0);
			NosuchDebug(1,"RECEIVED /palette/tonicchange message! t=%d",t);
			MusicBehaviour::tonic_set(palette(),t);
			return;
		}
		NosuchDebug("PaletteOscInput - NO HANDLER FOR addr=%s",m.AddressPattern());
	} catch( osc::Exception& e ){
		// any parsing errors such as unexpected argument types, or 
		// missing arguments get thrown as exceptions.
		NosuchDebug("ProcessOscMessage error while parsing message: %s : %s",m.AddressPattern(),e.what());
	} catch (NosuchException& e) {
		NosuchDebug("ProcessOscMessage, NosuchException: %s",e.message());
	} catch (...) {
		// This doesn't seem to work - it doesn't seem to catch other exceptions...
		NosuchDebug("ProcessOscMessage, some other kind of exception occured during !?");
	}
}