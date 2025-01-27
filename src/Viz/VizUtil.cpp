/*
	Copyright (c) 2011-2013 Tim Thompson <me@timthompson.com>

	Permission is hereby granted, free of charge, to any person obtaining
	a copy of this software and associated documentation files
	(the "Software"), to deal in the Software without restriction,
	including without limitation the rights to use, copy, modify, merge,
	publish, distribute, sublicense, and/or sell copies of the Software,
	and to permit persons to whom the Software is furnished to do so,
	subject to the following conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	Any person wishing to distribute modifications to the Software is
	requested to send the modifications to the original developer so that
	they can be incorporated into the canonical version.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
	IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
	ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
	CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
	WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <winsock.h>

#include <stdint.h>
#include "VizUtil.h"
#include "tstring.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <vector>
#include <iterator>
#include <pthread.h>
#include <locale>
#include <sstream>
#include <string>

using namespace std;

#define NOSUCHMAXSTRING 8096

#include <time.h>
#include <sys/types.h>
#include <sys/timeb.h>

void VizPrintTime(const char *prefix) {
    struct _timeb tstruct;
    _ftime( &tstruct ); // C4996
	int milli = tstruct.millitm;
	long secs = (long) tstruct.time;
	DEBUGPRINT(("%s: time= %ld.%03u\n",prefix,secs,milli));
}

bool VizFileExists(const std::string fname)
{
	std::ifstream f;
	f.open(fname.c_str());
	if (!f.good()) {
		return false;
	}
	else {
		f.close();
		return true;
	}
}

bool VizFileCopy(std::string frompath, std::string topath)
{
	FILE* ffrom = fopen(frompath.c_str(), "rb");
	FILE* fto = fopen(topath.c_str(), "wb");
	if (ffrom == NULL || fto == NULL) {
		return false;
	}
	int c;
	while ((c = fgetc(ffrom)) >= 0) {
		fputc(c, fto);
	}
	fclose(ffrom);
	fclose(fto);
	return true;
}

// loadPipeline(pipenum, fname, fpath, ppipeline->m_sidmin, ppipeline->m_sidmax);



std::string VizReplaceAll(std::string str, const std::string from, const std::string to) {
	size_t start_pos = 0;
	while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
	}
	return str;
}

std::vector<std::string> VizSplitOnAnyChar(std::string s, std::string sepchars)
{
	std::vector<std::string> result;
	const char *seps = sepchars.c_str();
	const char *str = s.c_str();

    while(1) {
        const char *begin = str;
		while( strchr(seps,*str) == NULL && *str) {
                str++;
		}
        result.push_back(std::string(begin, str));
		if(0 == *str) {
			break;
		}
		// skip one or more of the sepchars
		while( strchr(seps,*str) != NULL && *str ) {
			str++;
		}
    }
    return result;
}

std::vector<std::string> VizSplitOnString(const std::string& s, const std::string& delim, const bool keep_empty) {
	std::vector<std::string> result;
    if (delim.empty()) {
        result.push_back(s);
        return result;
    }
    std::string::const_iterator substart = s.begin(), subend;
    while (true) {
        subend = search(substart, s.end(), delim.begin(), delim.end());
		std::string temp(substart, subend);
        if (keep_empty || !temp.empty()) {
            result.push_back(temp);
        }
        if (subend == s.end()) {
            break;
        }
        substart = subend + delim.size();
    }
    return result;
}

static int VizNetworkInitialized = 0;

int
VizNetworkInit()
{
  if ( ! VizNetworkInitialized ) {
	int err;
	WSADATA wsaData;
	err = WSAStartup(MAKEWORD(1,1),&wsaData);
	// err = WSAStartup(MAKEWORD(2,0), &wsaData);

	if ( err ) {

		DEBUGPRINT(("VizNetworkInit failed!? err=%d",err));

		return 1;

	}
	VizNetworkInitialized = 1;
  }
  return 0;
}

bool
SendToUDPServer(std::string host, int serverport, const char *data, int leng)
{
    SOCKET s;
    struct sockaddr_in sin;
    int sin_len = sizeof(sin);
    int i;
    PHOSTENT phe;
    const char *serverhost = host.c_str();

    phe = gethostbyname(serverhost);
    if (phe == NULL) {
        DEBUGPRINT(("SendToNthServer: gethostbyname(host=%s) fails?",serverhost));
        return false;
    }
    s = socket(PF_INET, SOCK_DGRAM, 0);
    if ( s < 0 ) {
        DEBUGPRINT(("SendToNthServer: unable to create socket!?"));
        return false;
    }
    sin.sin_family = AF_INET;
    memcpy((struct sockaddr FAR *) &(sin.sin_addr),
           *(char **)phe->h_addr_list, phe->h_length);
    sin.sin_port = htons(serverport);

    i = sendto(s,data,leng,0,(LPSOCKADDR)&sin,sin_len);

    closesocket(s);
    return true;
}

bool
SendToSLIPServer(std::string shost, int port, const char *data, int leng)
{
    SOCKET lhSocket;
    SOCKADDR_IN lSockAddr;
    int lConnect;
    size_t lleng;

    const char *host = shost.c_str();

    lhSocket = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(lhSocket == INVALID_SOCKET)
    {
        DEBUGPRINT(("INVALID SOCKET!"));
        return false;
    }
    memset(&lSockAddr,0, sizeof(SOCKADDR_IN));
    lSockAddr.sin_family = AF_INET;
    lSockAddr.sin_port = htons(port);
    // inet_addr doesn't work on "localhost" ?
    if ( strcmp(host,"localhost") == 0 ) {
        host = "127.0.0.1";
    }
    lSockAddr.sin_addr.s_addr = inet_addr(host);
    lConnect = connect(lhSocket,(SOCKADDR *)&lSockAddr,sizeof(SOCKADDR_IN));
    if(lConnect != 0)
    {
        DEBUGPRINT(("connect() failed to %s:%d, err=%d  WSAerr=%d",host,port,lConnect,WSAGetLastError()));
        return false;
    }
    char *buff = (char*) malloc(leng*2+2);
    char *bp = buff;
    const char *dp = data;
    *bp++ = (char)SLIP_END;
    for ( int n=0; n<leng; n++ ) {
        if ( IS_SLIP_END(*dp) ) {
            *bp++ = (char)SLIP_ESC;
            *bp++ = (char)SLIP_END;
            DEBUGPRINT(("ESCAPING SLIP_END!\n"));
        } else if ( IS_SLIP_ESC(*dp) ) {
            *bp++ = (char)SLIP_ESC;
            *bp++ = (char)SLIP_ESC2;
            DEBUGPRINT(("ESCAPING SLIP_ESC!\n"));
        } else {
            *bp++ = *dp;
        }
        dp++;
    }
    *bp++ = (char)SLIP_END;
    size_t bleng = bp - buff;
    lleng = send(lhSocket,buff,(int)bleng,0);
    if(lleng < bleng)
    {
        DEBUGPRINT(("Send error, all bytes not sent\n"));
    }
    closesocket(lhSocket);
    return true;
}

#if 0
CvScalar
randomRGB() {
    float h = (float)((360.0 * rand()) / (float) RAND_MAX) ;
    return HLStoRGB(h,0.5,1.0);
}

CvScalar
HLStoRGB(float hue, float lum, float sat)
{
    float r;
    float g;
    float b;

    if ( sat == 0 ) {
        r = g = b = lum * 255;
    } else {
        float rm2;
        if ( lum <= 0.5 ) {
            rm2 = lum + lum * sat;
        } else {
            rm2 = lum + sat - lum * sat;
        }
        float rm1 = 2 * lum - rm2;
        r = ToRGB1(rm1, rm2, hue + 120);
        g = ToRGB1(rm1, rm2, hue);
        b = ToRGB1(rm1, rm2, hue - 120);
    }
    return CV_RGB(r,g,b);
}

void
RGBtoHLS(float r, float g, float b, float* hue, float* lum, float* sat)
{
    float minval = min(r, min(g, b));
    float maxval = max(r, max(g, b));
    float mdiff = maxval - minval;
    float msum = maxval + minval;
    *lum = msum / 510;
    if ( maxval == minval ) {
        *sat = 0;
        *hue = 0;
    } else {
        float rnorm = (maxval - r) / mdiff;
        float gnorm = (maxval - g) / mdiff;
        float bnorm = (maxval - b) / mdiff;
        if ( *lum <= .5 ) {
            *sat = mdiff/msum;
        } else {
            *sat = mdiff / (510 - msum);
        }

        // self._saturation = (self._luminance <= .5) ? (mdiff/msum) : (mdiff / (510 - msum));
        if ( r == maxval ) {
            *hue = 60 * (6 + bnorm - gnorm);
        } else if ( g == maxval ) {
            *hue = 60 * (2 + rnorm - bnorm);
        } else if ( b == maxval ) {
            *hue = 60 * (4 + gnorm - rnorm);
        }

        while ( *hue > 360.0 ) {
            *hue -= 360.0;
        }
        while ( *hue < 0.0 ) {
            *hue += 360.0;
        }
    }
}

float
ToRGB1(float rm1, float rm2, float rh)
{
    if ( rh > 360 ) {
        rh -= 360;
    } else if ( rh < 0 ) {
        rh += 360;
    }

    if ( rh < 60 ) {
        rm1 = rm1 + (rm2 - rm1) * rh / 60;
    } else if ( rh < 180 ) {
        rm1 = rm2;
    } else if ( rh < 240 ) {
        rm1 = rm1 + (rm2 - rm1) * (240 - rh) / 60;
    }
    return(rm1 * 255);
}
#endif

float
angleNormalize(float a)
{
    while ( a < 0.0 ) {
        a += PI2;
    }
    while ( a > PI2 ) {
        a -= PI2;
    }
    return a;
}

// base64 code below was found at http://stackoverflow.com/questions/342409/how-do-i-base64-encode-decode-in-c

#include <math.h>
#include <stdint.h>
#include <stdlib.h>

static char encoding_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                                'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                                'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                                'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                'w', 'x', 'y', 'z', '0', '1', '2', '3',
                                '4', '5', '6', '7', '8', '9', '+', '/'};
static char *decoding_table = NULL;
static int mod_table[] = {0, 2, 1};

char *base64_encode(const uint8_t *data, size_t input_length) {

    size_t output_length = (size_t) (4.0 * ceil((double) input_length / 3.0));

    char *encoded_data = (char*) malloc(output_length+1);
    if (encoded_data == NULL) return NULL;

    for (size_t i = 0, j = 0; i < input_length;) {

        uint32_t octet_a = i < input_length ? data[i++] : 0;
        uint32_t octet_b = i < input_length ? data[i++] : 0;
        uint32_t octet_c = i < input_length ? data[i++] : 0;

        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

        encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
    }

    for (int i = 0; i < mod_table[input_length % 3]; i++)
        encoded_data[output_length - 1 - i] = '=';

	encoded_data[output_length] = '\0';
    return encoded_data;
}

void build_decoding_table() {

    decoding_table = (char *) malloc(256);

    for (int i = 0; i < 0x40; i++)
        decoding_table[encoding_table[i]] = i;
}

char *base64_decode(const char *data,
                    size_t input_length,
                    size_t *output_length) {

    if (decoding_table == NULL) build_decoding_table();

    if (input_length % 4 != 0) return NULL;

    *output_length = input_length / 4 * 3;
    if (data[input_length - 1] == '=') (*output_length)--;
    if (data[input_length - 2] == '=') (*output_length)--;

    char *decoded_data = (char *) malloc(*output_length);
    if (decoded_data == NULL) return NULL;

    for (size_t i = 0, j = 0; i < input_length;) {

        uint32_t sextet_a = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
        uint32_t sextet_b = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
        uint32_t sextet_c = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
        uint32_t sextet_d = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];

        uint32_t triple = (sextet_a << 3 * 6)
                        + (sextet_b << 2 * 6)
                        + (sextet_c << 1 * 6)
                        + (sextet_d << 0 * 6);

        if (j < *output_length) decoded_data[j++] = (triple >> 2 * 8) & 0xFF;
        if (j < *output_length) decoded_data[j++] = (triple >> 1 * 8) & 0xFF;
        if (j < *output_length) decoded_data[j++] = (triple >> 0 * 8) & 0xFF;
    }

    return decoded_data;
}

void base64_cleanup() {
    free(decoding_table);
}

std::string error_json(int code, const char *msg, const char *id) {
	return VizSnprintf(
		"{\"jsonrpc\": \"2.0\", \"error\": {\"code\": %d, \"message\": \"%s\"}, \"id\": \"%s\"}",code,msg,id);
}

std::string ok_json(const char *id) {
	return VizSnprintf(
		"{\"jsonrpc\": \"2.0\", \"result\": 0, \"id\": \"%s\"}",id);
}

std::string VizToLower(std::string s) {
	// This could probably be a lot more efficient
	std::string r = s;
	for(size_t i=0; i<s.size(); i++) {
		r[i] = tolower(s[i]);
	}
	return r;
}

std::string VizToUpper(std::string s) {
	// This could probably be a lot more efficient
	std::string r = s;
	for(size_t i=0; i<s.size(); i++) {
		r[i] = toupper(s[i]);
	}
	return r;
}

bool
VizEndsWith(std::string s, std::string suff)
{
	unsigned int nchars = suff.size();
	std::string suffix = (s.size()>nchars) ? s.substr(s.size() - nchars).c_str() : "";
	return suff == suffix;
}

bool
VizBeginsWith(std::string s, std::string prefix)
{
	unsigned int presize = prefix.size();
	if (s.size() < presize) {
		return false;
	}
	std::string pre = s.substr(0, presize);
	return pre == prefix;
}

void VizLockInit(pthread_mutex_t* mutex, char *tag) {

	*mutex = PTHREAD_ERRORCHECK_MUTEX_INITIALIZER;
	// *mutex = PTHREAD_MUTEX_INITIALIZER;

	DEBUGPRINT1(("VizLockInit pthread=%d tag=%s mutex=%ld",(int)pthread_self().p,tag,(long)mutex));
}

void VizLock(pthread_mutex_t* mutex, char *tag) {
	int r = pthread_mutex_lock(mutex);
	if ( r == EDEADLK ) {
		VizErrorOutput("Hey! VizLock detects DEADLOCK!!  tag=%s",tag);
	} else if ( r != 0 ) {
		VizErrorOutput("Hey! pthread_mutex_lock tag=%s returned non-zero!  r=%d",tag,r);
	} else {
		DEBUGPRINT2(("VizLock worked!! tag=%s",tag));
	}
}

void VizUnlock(pthread_mutex_t* mutex, char *tag) {
	int r = pthread_mutex_unlock(mutex);
	if ( r != 0 ) {
		VizErrorOutput("Hey! pthread_mutex_unlock tag=%s returned non-zero!  r=%d",tag,r);
	} else {
		DEBUGPRINT2(("VizUnLock worked!! tag=%s",tag));
	}
}

int VizTryLock(pthread_mutex_t* mutex, char *tag) {
	int r = pthread_mutex_trylock(mutex);
	if ( r != 0 && r != EBUSY ) {
		VizErrorOutput("Hey! pthread_mutex_trylock tag=%s returned non-zero!  r=%d",tag,r);
	}
	return r;
}

std::string ToNarrow( const wchar_t *s, char dfault, const std::locale& loc ) {
  std::ostringstream stm;

  while( *s != L'\0' ) {
    stm << std::use_facet< std::ctype<wchar_t> >( loc ).narrow( *s++, dfault );
  }
  return stm.str();
}

std::wstring s2ws(const std::string& s)
{
    int len;
    int slength = (int)s.length() + 1;
    len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0); 
    std::wstring r(len, L'\0');
    MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, &r[0], len);
    return r;
}

std::string ws2s(const std::wstring& s)
{
    int len;
    int slength = (int)s.length() + 1;
	// First figure out the length of the result, to allocate enough space
    len = WideCharToMultiByte(CP_ACP, 0, s.c_str(), slength, 0, 0, 0, 0); 
	char* r = new char[len];  // includes the null character
    WideCharToMultiByte(CP_ACP, 0, s.c_str(), slength, r, len, 0, 0); 
	std::string rs = std::string(r);
	delete[] r;
    return rs;
}

DWORD VizMilli0;

void VizTimeInit() {
	VizMilli0 = timeGetTime();
}

double VizTimeElapsedInSeconds() {
	return ( (timeGetTime() - VizMilli0) / 1000.0 );
}
