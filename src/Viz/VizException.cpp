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


#include <exception>
#include <stdint.h>
#include <stdarg.h>
#include <string>

#include "VizUtil.h"
#include "VizException.h"

#define EXCEPTION_BUFFER_SIZE 8096
static char VizExceptionBuffer[EXCEPTION_BUFFER_SIZE];

VizException::VizException( const char *fmt, ...) {
	m_msg = VizExceptionBuffer;
	va_list args;

	va_start(args, fmt);
	vsnprintf_s(m_msg,EXCEPTION_BUFFER_SIZE,EXCEPTION_BUFFER_SIZE,fmt,args);

	size_t lng = strlen(m_msg);
	if ( lng > 0 && m_msg[lng-1] == '\n' )
		m_msg[lng-1] = '\0';

	DEBUGPRINT(("VizException: %s",m_msg));

	va_end(args);
}

#ifdef SEH_STUFF_ONLY_USABLE_WHEN_COMPILING_FOR_SEH
// the translator function 
void SEH_To_Cplusplus ( unsigned int u, EXCEPTION_POINTERS *exp ) { 
		int code = exp->ExceptionRecord->ExceptionCode;
		if ( code == EXCEPTION_ACCESS_VIOLATION ) {
			VizErrorOutput("NULL POINTER DEREFERENCE!! throwing VizException\n");
			throw VizException("NULL POINTER DEREFERENCE!! (VizException translated from SEH exception)");
		} else {
			throw VizException("VizException translated from SEH exception, code=%d",code);
		}

} 
#endif
