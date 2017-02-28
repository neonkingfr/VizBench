#include "VizException.h"
#include "VizOsc.h"

std::string
OscMessageString(const osc::ReceivedMessage& m)
{
    const char *addr = m.AddressPattern();
    const char *types = m.TypeTags();
	if ( types == NULL ) {
		types = "";
	}

	std::string result = VizSnprintf("{ \"address\":\"%s\", \"args\":[",addr);
    osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
	std::string sep = "";
    for ( const char *p=types; *p!='\0'; p++ ) {
		result += sep;
        switch (*p) {
        case 'i':
			result += VizSnprintf("%d",(arg++)->AsInt32());
            break;
        case 'f':
			result += VizSnprintf("%lf",(arg++)->AsFloat());
            break;
        case 's':
			result += VizSnprintf("\"%s\"",(arg++)->AsString());
            break;
        case 'b':
            arg++;
			result += VizSnprintf("\"blob?\"");
            break;
        }
		sep = ", ";
    }
	result += "] }";
	return result;
}

void
DebugOscMessage(std::string prefix, const osc::ReceivedMessage& m)
{
	DEBUGPRINT(("%s: %s ",prefix==""?"OSC message":prefix.c_str(),OscMessageString(m).c_str()));
}

int
ArgAsInt32(const osc::ReceivedMessage& m, unsigned int n)
{
    osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
	const char *types = m.TypeTags();
	if ( n >= strlen(types) )  {
		DebugOscMessage("ArgAsInt32 ",m);
		throw VizException("Attempt to get argument n=%d, but not that many arguments on addr=%s\n",n,m.AddressPattern());
	}
	if ( types[n] != 'i' ) {
		DebugOscMessage("ArgAsInt32 ",m);
		throw VizException("Expected argument n=%d to be an int(i), but it is (%c)\n",n,types[n]);
	}
	for ( unsigned i=0; i<n; i++ )
		arg++;
    return arg->AsInt32();
}

float
ArgAsFloat(const osc::ReceivedMessage& m, unsigned int n)
{
    osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
	const char *types = m.TypeTags();
	if ( n >= strlen(types) )  {
		DebugOscMessage("ArgAsFloat ",m);
		throw VizException("Attempt to get argument n=%d, but not that many arguments on addr=%s\n",n,m.AddressPattern());
	}
	if ( types[n] != 'f' ) {
		DebugOscMessage("ArgAsFloat ",m);
		throw VizException("Expected argument n=%d to be a double(f), but it is (%c)\n",n,types[n]);
	}
	for ( unsigned i=0; i<n; i++ )
		arg++;
    return arg->AsFloat();
}

std::string
ArgAsString(const osc::ReceivedMessage& m, unsigned n)
{
    osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
	const char *types = m.TypeTags();
	if ( n < 0 || n >= strlen(types) )  {
		DebugOscMessage("ArgAsString ",m);
		throw VizException("Attempt to get argument n=%d, but not that many arguments on addr=%s\n",n,m.AddressPattern());
	}
	if ( types[n] != 's' ) {
		DebugOscMessage("ArgAsString ",m);
		throw VizException("Expected argument n=%d to be a string(s), but it is (%c)\n",n,types[n]);
	}
	for ( unsigned i=0; i<n; i++ )
		arg++;
	return std::string(arg->AsString());
}
