#ifndef NSOSCINPUT_H
#define NSOSCINPUT_H

#define _USE_MATH_DEFINES

#include <list>

#include "osc/OscReceivedElements.h"
#include <math.h>
#include "VizUtil.h"

// #define NTHEVENTSERVER_PORT 1384
// Every so many milliseconds, we re-register with the Nth Server
#define NTHEVENTSERVER_REREGISTER_MILLISECONDS 3000

void DebugOscMessage(std::string prefix, const osc::ReceivedMessage& m);
std::string OscMessageString(const osc::ReceivedMessage& m);

#define RAD2DEG(r) ((r)*360.0/(2.0*M_PI))
#define PI2 ((float)(2.0*M_PI))

class VizOscListener {
public:
	virtual void processOsc( const char *source, const osc::ReceivedMessage& m) { }
};

class VizOscInput {

public:
	VizOscInput (VizOscListener* p);
	virtual ~VizOscInput ();

	void processOscBundle( const char *source, const osc::ReceivedBundle& b);

	void ProcessReceivedPacket(const char *source, osc::ReceivedPacket& rp) {
	    if( rp.IsBundle() ) {
	        processOscBundle( source, osc::ReceivedBundle(rp) );
		} else {
	        m_processor->processOsc( source, osc::ReceivedMessage(rp) );
		}
	}

	VizOscListener* m_processor;
};

#endif
