// Copyright (C) 2004 Id Software, Inc.
//

#ifndef __MATH_SIMD_ALTIVEC_H__
#define __MATH_SIMD_ALTIVEC_H__

/*
===============================================================================

	AltiVec implementation of idSIMDProcessor

===============================================================================
*/

class idSIMD_AltiVec : public idSIMD_Generic {
#ifdef MACOS_X
public:
	virtual const char * VPCALL GetName( void ) const;

#endif
};

#endif /* !__MATH_SIMD_ALTIVEC_H__ */
