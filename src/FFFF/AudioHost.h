#ifndef _AUDIOHOST_H
#define _AUDIOHOST_H

#include "NosuchJSON.h"

#include <string>

class AudioHost {
public:
	AudioHost(std::string hosttype, cJSON* config);
	~AudioHost();

	bool Start();
	bool Execute();
	bool Stop();
	bool Recording(bool onoff);
	bool Playing(bool onoff);

private:
	bool _sendOscOnOff(std::string cmd, bool onoff);
	std::string m_hosttype;
	std::string m_executable;
	std::string m_patch;
	int m_oscport;

};

#endif
