#ifndef _IDLIB
#define _IDLIB
//#define ID_INLINE inline
#include "cpuid.h"

#ifdef __GNUC__
#define id_attribute(x) __attribute__(x)
#else
#define id_attribute(x)  
#endif



enum cpuid_t
{
	CPUID_NONE							= 0x00000,
	CPUID_UNSUPPORTED					= 0x00001,	// unsupported (386/486)
	CPUID_GENERIC						= 0x00002,	// unrecognized processor
	CPUID_INTEL							= 0x00004,	// Intel
	CPUID_AMD							= 0x00008,	// AMD
	CPUID_MMX							= 0x00010,	// Multi Media Extensions
	CPUID_3DNOW							= 0x00020,	// 3DNow!
	CPUID_SSE							= 0x00040,	// Streaming SIMD Extensions
	CPUID_SSE2							= 0x00080,	// Streaming SIMD Extensions 2
	CPUID_SSE3							= 0x00100,	// Streaming SIMD Extentions 3 aka Prescott's New Instructions
	CPUID_ALTIVEC						= 0x00200,	// AltiVec
	CPUID_HTT							= 0x01000,	// Hyper-Threading Technology
	CPUID_CMOV							= 0x02000,	// Conditional Move (CMOV) and fast floating point comparison (FCOMI) instructions
	CPUID_FTZ							= 0x04000,	// Flush-To-Zero mode (denormal results are flushed to zero)
	CPUID_DAZ							= 0x08000	// Denormals-Are-Zero mode (denormal source operands are set to zero)
};

inline int get_doom_cpuid(int feature)//from MSDN's cpuid
{


	int id =CPUID_NONE;
	if (feature==0)
		return CPUID_NONE;

	if (feature&_CPU_FEATURE_MMX)
		id = CPUID_MMX;
	if (feature&_CPU_FEATURE_SSE)
		id|= CPUID_SSE;
	if (feature&_CPU_FEATURE_SSE2)
		id|=CPUID_SSE2;
	if (feature&_CPU_FEATURE_SSE3)
		id|=CPUID_SSE3;
	if (feature&_CPU_FEATURE_3DNOW)
		id|=CPUID_3DNOW;
	//	if (checks&_CPU_FEATURE_MMX)
	//		id|=CPUID_MMX;
	//	if (checks&_CPU_FEATURE_MMX)
	//		id|=CPUID_MMX;
	return id;
};
typedef enum {
	FPU_EXCEPTION_INVALID_OPERATION		= 1,
	FPU_EXCEPTION_DENORMALIZED_OPERAND	= 2,
	FPU_EXCEPTION_DIVIDE_BY_ZERO		= 4,
	FPU_EXCEPTION_NUMERIC_OVERFLOW		= 8,
	FPU_EXCEPTION_NUMERIC_UNDERFLOW		= 16,
	FPU_EXCEPTION_INEXACT_RESULT		= 32
} fpuExceptions_t;

typedef enum {
	FPU_PRECISION_SINGLE				= 0,
	FPU_PRECISION_DOUBLE				= 1,
	FPU_PRECISION_DOUBLE_EXTENDED		= 2
} fpuPrecision_t;

typedef enum {
	FPU_ROUNDING_TO_NEAREST				= 0,
	FPU_ROUNDING_DOWN					= 1,
	FPU_ROUNDING_UP						= 2,
	FPU_ROUNDING_TO_ZERO				= 3
} fpuRounding_t;

typedef enum {
	AXIS_SIDE,
	AXIS_FORWARD,
	AXIS_UP,
	AXIS_ROLL,
	AXIS_YAW,
	AXIS_PITCH,
	MAX_JOYSTICK_AXIS
} joystickAxis_t;

typedef enum {
	SE_NONE,				// evTime is still valid
	SE_KEY,					// evValue is a key code, evValue2 is the down flag
	SE_CHAR,				// evValue is an ascii char
	SE_MOUSE,				// evValue and evValue2 are reletive signed x / y moves
	SE_JOYSTICK_AXIS,		// evValue is an axis number and evValue2 is the current state (-127 to 127)
	SE_CONSOLE				// evPtr is a char*, from typing something at a non-game console
} sysEventType_t;

typedef enum {
	M_ACTION1,
	M_ACTION2,
	M_ACTION3,
	M_ACTION4,
	M_ACTION5,
	M_ACTION6,
	M_ACTION7,
	M_ACTION8,
	M_DELTAX,
	M_DELTAY,
	M_DELTAZ
} sys_mEvents;

typedef struct sysEvent_s {
	sysEventType_t	evType;
	int				evValue;
	int				evValue2;
	int				evPtrLength;		// bytes of data pointed to by evPtr, for journaling
	void *			evPtr;				// this must be manually freed if not NULL
} sysEvent_t;

typedef struct sysMemoryStats_s {
	int memoryLoad;
	int totalPhysical;
	int availPhysical;
	int totalPageFile;
	int availPageFile;
	int totalVirtual;
	int availVirtual;
	int availExtendedVirtual;
} sysMemoryStats_t;

typedef unsigned long address_t;

template<class type> class idList;		// for Sys_ListFiles
class idLib {
public:
	static class idSys *		sys;
	static class idCommon *		common;
//	static class idCVarSystem *	cvarSystem;
//	static class idFileSystem *	fileSystem;
	static int					frameNumber;

	static void					Init( void );
	static void					ShutDown( void );
};
class idSys
{
public: 
	idSys();
	~idSys();

	const char *	GetCallStackCurStr( int depth ) {return 0;};
	virtual bool			LockMemory( void *ptr, int bytes ) {return true;};
	virtual bool			UnlockMemory( void *ptr, int bytes ){return true;};
	virtual cpuid_t			GetProcessorId( void ) {return CPUID_SSE;};
	virtual const char *	GetProcessorString( void ) {return 0;};
	virtual const char *	FPU_GetState( void ) {return 0;};
	virtual void			FPU_SetFTZ( bool enable ) {};
	virtual void			FPU_SetDAZ( bool enable ) {};
};

class idCommon
{
public :
	idCommon();
	~idCommon();
	void FatalError( const char *fmt, ... ) ;
	void Printf(const char* , ...);
	void Warning(const char*, ...){};
	void Error(const char* ,...){};
};

#ifdef _DEBUG
void AssertFailed( const char *file, int line, const char *expression );
#undef assert
#define assert( X )		if ( X ) { } else AssertFailed( __FILE__, __LINE__, #X )
#endif

//inline void assert(bool init)
//{
//	__asm
//	{
//		int 3;
//	}
//};
/*
===============================================================================

	Types and defines used throughout the engine.

===============================================================================
*/

typedef unsigned char			byte;		// 8 bits
typedef unsigned short			word;		// 16 bits
typedef unsigned int			dword;		// 32 bits
typedef unsigned int			uint32;
typedef unsigned long			ulong;

typedef int						qhandle_t;



#ifndef NULL
#define NULL					0
#endif

#ifndef BIT
#define BIT( num )				( 1 << ( num ) )
#endif

#define	MAX_STRING_CHARS		1024		// max length of a string

// maximum world size
#define MAX_WORLD_COORD			( 128 * 1024 )
#define MIN_WORLD_COORD			( -128 * 1024 )
#define MAX_WORLD_SIZE			( MAX_WORLD_COORD - MIN_WORLD_COORD )



// math
#include "heap.h"
#include "Str.h"
#include "math/Simd.h"
#include "math/Math.h"
#include "math/Random.h"
#include "math/Vector.h"
#include "math/Matrix.h"
#include "math/Angles.h"
#include "math/Quat.h"
#include "math/Rotation.h"
#include "math/Plane.h"

#include "geometry/DrawVert.h"
#include "geometry/JointTransform.h"

#include "BTree.h"
#endif
