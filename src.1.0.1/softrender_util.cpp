#include "stdafx.h"
#include "mmgr/mmgr.h"
void printDebug(const char*str , ...)
{
	
	static char buf[512];
	va_list		argptr;
	va_start(argptr,str );
	vsprintf(buf,str,argptr);
	va_end(argptr);

	OutputDebugStringA(buf);
}