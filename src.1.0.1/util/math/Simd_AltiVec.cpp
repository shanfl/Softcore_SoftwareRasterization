// Copyright (C) 2004 Id Software, Inc.
//
#include "stdafx.h"
#include "../precompiled.h"
#pragma hdrstop

#include "Simd_Generic.h"
#include "Simd_AltiVec.h"

//===============================================================
//
//	AltiVec implementation of idSIMDProcessor
//
//===============================================================

#ifdef MACOS_X

/*
============
idSIMD_AltiVec::GetName
============
*/
const char *idSIMD_AltiVec::GetName( void ) const {
	return "AltiVec";
}

#endif /* MACOS_X */
