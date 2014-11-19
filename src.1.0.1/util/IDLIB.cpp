#include "stdafx.h"
#include "precompiled.h"

/*
===============================================================================

	Assertion

===============================================================================
*/
#include <stdio.h>
void AssertFailed( const char *file, int line, const char *expression )
{
	printf( "\n\nASSERTION FAILED!\n%s(%d): '%s'\n", file, line, expression );

	__asm int 0x03

}


idSys* idLib::sys=0;
idCommon *	idLib::common=0;
int	idLib::frameNumber;


void idCommon::Printf(const char* , ...)
{

}
void idCommon::FatalError( const char *fmt, ... ) 
{

}
void	idLib::Init( void )
{

}
void	idLib::ShutDown( void )
{

}