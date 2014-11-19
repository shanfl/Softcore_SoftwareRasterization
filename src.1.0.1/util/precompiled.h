// Copyright (C) 2004 Id Software, Inc.
//

#ifndef __PRECOMPILED_H__
#define __PRECOMPILED_H__



#include <winsock2.h>
#include <mmsystem.h>
#include <mmreg.h>

#define DIRECTINPUT_VERSION  0x0700
#define DIRECTSOUND_VERSION  0x0800


#pragma warning(disable : 4100)				// unreferenced formal parameter
#pragma warning(disable : 4244)				// conversion to smaller type, possible loss of data
#pragma warning(disable : 4714)				// function marked as __forceinline not inlined

#include <malloc.h>							// no malloc.h on mac or unix
#include <windows.h>						// for qgl.h
#undef FindText								// stupid namespace poluting Microsoft monkeys



//-----------------------------------------------------

#if !defined( _DEBUG ) && !defined( NDEBUG )
	// don't generate asserts
	#define NDEBUG
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <ctype.h>
#include <typeinfo>
#include <errno.h>
#include <math.h>

//-----------------------------------------------------

// non-portable system services

#define	BUILD_STRING					"win-x86"
#define BUILD_OS_ID						0
#define	CPUSTRING						"x86"
#define CPU_EASYARGS					1

#define ALIGN16( x )					__declspec(align(16)) x
#define PACKED

#define _alloca16( x )					((void *)((((int)_alloca( (x)+15 )) + 15) & ~15))

#define PATHSEPERATOR_STR				"\\"
#define PATHSEPERATOR_CHAR				'\\'

#define ID_INLINE						__forceinline
#define ID_STATIC_TEMPLATE				static

#define assertmem( x, y )				assert( _CrtIsValidPointer( x, y, true ) )

typedef int glIndex_t;
typedef struct dominantTri_s {
	glIndex_t					v2, v3;
	float						normalizationScale[3];
} dominantTri_t;


template< class type >
inline void idSwap( type &a, type &b ) {
	type c = a;
	a = b;
	b = c;
}

// We have expression parsing and evaluation code in multiple places:
// materials, sound shaders, and guis. We should unify them.
const int MAX_EXPRESSION_OPS = 4096;
const int MAX_EXPRESSION_REGISTERS = 4096;


#include "idlib.h"


#endif /* !__PRECOMPILED_H__ */
