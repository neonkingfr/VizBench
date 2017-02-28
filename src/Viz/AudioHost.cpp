#include "VizUtil.h"
#include "VizOsc.h"
#include "osc/OscOutboundPacketStream.h"
#include "AudioHost.h"

AudioHost::AudioHost(std::string hosttype, cJSON* config)
{
	if (hosttype != "bidule") {
		throw VizException("unsupported audiohost_type value: %s", hosttype.c_str());
	}
	m_hosttype = hosttype;
	m_executable = jsonNeedString(config, "executable", "");
	m_patch = jsonNeedString(config, "patch", "");
	m_oscport = jsonNeedInt(config, "oscport", 3210);
}

AudioHost::~AudioHost()
{
}

bool AudioHost::Start() {
	DEBUGPRINT(("AudioHost::Start()"));
	return Playing(true);
}

bool AudioHost::Execute() {
	STARTUPINFOA info = { 0 };
	PROCESS_INFORMATION processInfo;

	ZeroMemory(&info, sizeof(info));
	info.cb = sizeof(info);
	ZeroMemory(&processInfo, sizeof(processInfo));

	info.dwFlags = STARTF_USESHOWWINDOW;
	info.wShowWindow = TRUE;

	char* cmdline = _strdup(VizSnprintf("\"%s\" \"%s\"",m_executable.c_str(),VizConfigPath(m_patch).c_str()).c_str());
	DEBUGPRINT(("AudioHost::Start %s",m_executable.c_str()));
	if (CreateProcessA(m_executable.c_str(), cmdline, NULL, NULL, TRUE, 0, NULL, NULL, &info, &processInfo))
	{
		// We don't wait, we want it running separately
		// ::WaitForSingleObject(processInfo.hProcess, INFINITE);
		CloseHandle(processInfo.hProcess);
		CloseHandle(processInfo.hThread);
	}
	return false;
}

bool AudioHost::Stop() {
	DEBUGPRINT(("AudioHost::Stop()"));
	return false;
}

bool
AudioHost::Recording(bool onoff) {
	return _sendOscOnOff("/Audio_File_Recorder_0/Recording", onoff);
}

bool
AudioHost::Playing(bool onoff) {
	// For some reason/bug, my Plogue Bidule looks like it's on when
	// you start it, but I have to turn in off and back on in order to
	// get any audio.  
	if (onoff) {
		_sendOscOnOff("/play", 1);
		Sleep(100);  // milliseconds, too fast and bidule ignores it?
		_sendOscOnOff("/play", 0);
		Sleep(100);  // milliseconds
		return _sendOscOnOff("/play", 1);
	}
	else {
		return _sendOscOnOff("/play", 0);
	}
}

bool
AudioHost::_sendOscOnOff(std::string cmd, bool onoff) {
	char buffer[1024];
	osc::OutboundPacketStream p(buffer, sizeof(buffer));
	p << osc::BeginMessage(cmd.c_str()) << (onoff ? 1 : 0) << osc::EndMessage;
	return SendToUDPServer("127.0.0.1", m_oscport, p.Data(), (int)p.Size());
}