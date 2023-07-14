/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2012 Robert Beckebans

This file is part of the Doom 3 BFG Edition GPL Source Code ("Doom 3 BFG Edition Source Code").

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#pragma hdrstop
#include "../../idlib/precompiled.h"

// DG: SDL_*.h somehow needs the following functions, so #undef those silly
//     "don't use" #defines from Str.h
#undef strcasecmp
#undef strncmp
#undef vsnprintf
// DG end

#include <SDL_cpuinfo.h>


#pragma warning(disable:4740)	// warning C4740: flow in or out of inline asm code suppresses global optimization
#pragma warning(disable:4731)	// warning C4731: 'XXX' : frame pointer register 'ebx' modified by inline assembly code

/*
==============================================================

	Clock ticks

==============================================================
*/

/*
================
Sys_GetClockTicks
================
*/
#if defined(_WIN32)
double Sys_GetClockTicks()
{
	// RB begin
#if defined(_MSC_VER)
	unsigned long lo, hi;

	__asm
	{
		push ebx
		xor eax, eax
		cpuid
		rdtsc
		mov lo, eax
		mov hi, edx
		pop ebx
	}
	return ( double ) lo + ( double ) 0xFFFFFFFF * hi;

#elif defined(__GNUC__) && defined( __i386__ )
	unsigned long lo, hi;

	__asm__ __volatile__(
		"push %%ebx\n"			\
		"xor %%eax,%%eax\n"		\
		"cpuid\n"					\
		"rdtsc\n"					\
		"mov %%eax,%0\n"			\
		"mov %%edx,%1\n"			\
		"pop %%ebx\n"
		: "=r"( lo ), "=r"( hi ) );
	return ( double ) lo + ( double ) 0xFFFFFFFF * hi;
#else
#error unsupported CPU
#endif
	// RB end
}
#endif

/*
================
Sys_ClockTicksPerSecond
================
*/
#if defined(_WIN32)
double Sys_ClockTicksPerSecond()
{
	static double ticks = 0;
#if 0

	if( !ticks )
	{
		LARGE_INTEGER li;
		QueryPerformanceFrequency( &li );
		ticks = li.QuadPart;
	}

#else

	if( !ticks )
	{
		HKEY hKey;
		LPBYTE ProcSpeed;
		DWORD buflen, ret;

		if( !RegOpenKeyEx( HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &hKey ) )
		{
			ProcSpeed = 0;
			buflen = sizeof( ProcSpeed );
			ret = RegQueryValueEx( hKey, "~MHz", NULL, NULL, ( LPBYTE ) &ProcSpeed, &buflen );
			// If we don't succeed, try some other spellings.
			if( ret != ERROR_SUCCESS )
			{
				ret = RegQueryValueEx( hKey, "~Mhz", NULL, NULL, ( LPBYTE ) &ProcSpeed, &buflen );
			}
			if( ret != ERROR_SUCCESS )
			{
				ret = RegQueryValueEx( hKey, "~mhz", NULL, NULL, ( LPBYTE ) &ProcSpeed, &buflen );
			}
			RegCloseKey( hKey );
			if( ret == ERROR_SUCCESS )
			{
				ticks = ( double )( ( unsigned long )ProcSpeed ) * 1000000;
			}
		}
	}

#endif
	return ticks;
}
#endif


/*
==============================================================

	CPU

==============================================================
*/



/*
========================
Sys_CPUCount

numLogicalCPUCores	- the number of logical CPU per core
numPhysicalCPUCores	- the total number of cores per package
numCPUPackages		- the total number of packages (physical processors)
========================
*/
#if defined(_WIN32)
void Sys_CPUCount( int& numLogicalCPUCores, int& numPhysicalCPUCores, int& numCPUPackages )
{
	numPhysicalCPUCores = 1;
	numLogicalCPUCores = SDL_GetCPUCount();
	numCPUPackages = 1;
}
#endif

/*
================
Sys_GetCPUId
================
*/
cpuid_t Sys_GetCPUId()
{
	int flags;

	// check for an AMD
	flags = CPUID_GENERIC;

	// check for Multi Media Extensions
	if( SDL_HasMMX() )
	{
		flags |= CPUID_MMX;
	}

	// check for 3DNow!
	if( SDL_Has3DNow() )
	{
		flags |= CPUID_3DNOW;
	}

	// check for Streaming SIMD Extensions
	if( SDL_HasSSE() )
	{
		flags |= CPUID_SSE | CPUID_FTZ;
	}

	// check for Streaming SIMD Extensions 2
	if( SDL_HasSSE2() )
	{
		flags |= CPUID_SSE2;
	}

	// check for Streaming SIMD Extensions 3 aka Prescott's New Instructions
#if 0 //SDL_VERSION_ATLEAST(2,0,0)
	if( SDL_HasSSE3() )
	{
		flags |= CPUID_SSE3;
	}
#endif

	/*
	// check for Hyper-Threading Technology
	if( HasHTT() )
	{
		flags |= CPUID_HTT;
	}

	// check for Conditional Move (CMOV) and fast floating point comparison (FCOMI) instructions
	if( HasCMOV() )
	{
		flags |= CPUID_CMOV;
	}

	// check for Denormals-Are-Zero mode
	if( HasDAZ() )
	{
		flags |= CPUID_DAZ;
	}
	*/

	return ( cpuid_t )flags;
}


/*
===============================================================================

	FPU

===============================================================================
*/

typedef struct bitFlag_s
{
	const char* name;
	int			bit;
} bitFlag_t;

static byte fpuState[128], *statePtr = fpuState;
static char fpuString[2048];
static bitFlag_t controlWordFlags[] =
{
	{ "Invalid operation", 0 },
	{ "Denormalized operand", 1 },
	{ "Divide-by-zero", 2 },
	{ "Numeric overflow", 3 },
	{ "Numeric underflow", 4 },
	{ "Inexact result (precision)", 5 },
	{ "Infinity control", 12 },
	{ "", 0 }
};
static const char* precisionControlField[] =
{
	"Single Precision (24-bits)",
	"Reserved",
	"Double Precision (53-bits)",
	"Double Extended Precision (64-bits)"
};
static const char* roundingControlField[] =
{
	"Round to nearest",
	"Round down",
	"Round up",
	"Round toward zero"
};
static bitFlag_t statusWordFlags[] =
{
	{ "Invalid operation", 0 },
	{ "Denormalized operand", 1 },
	{ "Divide-by-zero", 2 },
	{ "Numeric overflow", 3 },
	{ "Numeric underflow", 4 },
	{ "Inexact result (precision)", 5 },
	{ "Stack fault", 6 },
	{ "Error summary status", 7 },
	{ "FPU busy", 15 },
	{ "", 0 }
};

/*
===============
Sys_FPU_PrintStateFlags
===============
*/
int Sys_FPU_PrintStateFlags( char* buf, int bufsize, int ctrl, int stat, int tags, int inof, int inse, int opof, int opse )
{
	int i, length = 0;

	length += idStr::snPrintf( buf + length, bufsize - length,	"CTRL = %08x\n"
							   "STAT = %08x\n"
							   "TAGS = %08x\n"
							   "INOF = %08x\n"
							   "INSE = %08x\n"
							   "OPOF = %08x\n"
							   "OPSE = %08x\n"
							   "\n",
							   ctrl, stat, tags, inof, inse, opof, opse );

	length += idStr::snPrintf( buf + length, bufsize - length, "Control Word:\n" );
	for( i = 0; controlWordFlags[i].name[0]; i++ )
	{
		length += idStr::snPrintf( buf + length, bufsize - length, "  %-30s = %s\n", controlWordFlags[i].name, ( ctrl & ( 1 << controlWordFlags[i].bit ) ) ? "true" : "false" );
	}
	length += idStr::snPrintf( buf + length, bufsize - length, "  %-30s = %s\n", "Precision control", precisionControlField[( ctrl >> 8 ) & 3] );
	length += idStr::snPrintf( buf + length, bufsize - length, "  %-30s = %s\n", "Rounding control", roundingControlField[( ctrl >> 10 ) & 3] );

	length += idStr::snPrintf( buf + length, bufsize - length, "Status Word:\n" );
	for( i = 0; statusWordFlags[i].name[0]; i++ )
	{
		length += idStr::snPrintf( buf + length, bufsize - length, "  %-30s = %s\n", statusWordFlags[i].name, ( stat & ( 1 << statusWordFlags[i].bit ) ) ? "true" : "false" );
	}
	length += idStr::snPrintf( buf + length, bufsize - length, "  %-30s = %d%d%d%d\n", "Condition code", ( stat >> 8 ) & 1, ( stat >> 9 ) & 1, ( stat >> 10 ) & 1, ( stat >> 14 ) & 1 );
	length += idStr::snPrintf( buf + length, bufsize - length, "  %-30s = %d\n", "Top of stack pointer", ( stat >> 11 ) & 7 );

	return length;
}

/*
===============
Sys_FPU_StackIsEmpty
===============
*/
bool Sys_FPU_StackIsEmpty()
{
	// TODO
	return true;
}

/*
===============
Sys_FPU_ClearStack
===============
*/
void Sys_FPU_ClearStack()
{
	// TODO
}

/*
===============
Sys_FPU_GetState

  gets the FPU state without changing the state
===============
*/
const char* Sys_FPU_GetState()
{
	return "TODO Sys_FPU_GetState()";
}

/*
===============
Sys_FPU_EnableExceptions
===============
*/
void Sys_FPU_EnableExceptions( int exceptions )
{
	// TODO
}

/*
===============
Sys_FPU_SetPrecision
===============
*/
void Sys_FPU_SetPrecision( int precision )
{
	// TODO

	/*
	short precisionBitTable[4] = { 0, 1, 3, 0 };
	short precisionBits = precisionBitTable[precision & 3] << 8;
	short precisionMask = ~( ( 1 << 9 ) | ( 1 << 8 ) );

	__asm {
		mov			eax, statePtr
		mov			cx, precisionBits
		fnstcw		word ptr [eax]
		mov			bx, word ptr [eax]
		and			bx, precisionMask
		or			bx, cx
		mov			word ptr [eax], bx
		fldcw		word ptr [eax]
	}
	*/
}

/*
================
Sys_FPU_SetRounding
================
*/
void Sys_FPU_SetRounding( int rounding )
{
	// TODO

	/*
	short roundingBitTable[4] = { 0, 1, 2, 3 };
	short roundingBits = roundingBitTable[rounding & 3] << 10;
	short roundingMask = ~( ( 1 << 11 ) | ( 1 << 10 ) );

	__asm {
		mov			eax, statePtr
		mov			cx, roundingBits
		fnstcw		word ptr [eax]
		mov			bx, word ptr [eax]
		and			bx, roundingMask
		or			bx, cx
		mov			word ptr [eax], bx
		fldcw		word ptr [eax]
	}
	*/
}

/*
================
Sys_FPU_SetDAZ
================
*/
void Sys_FPU_SetDAZ( bool enable )
{
	/*
	DWORD dwData;

	_asm {
		movzx	ecx, byte ptr enable
		and		ecx, 1
		shl		ecx, 6
		STMXCSR	dword ptr dwData
		mov		eax, dwData
		and		eax, ~(1<<6)	// clear DAX bit
		or		eax, ecx		// set the DAZ bit
		mov		dwData, eax
		LDMXCSR	dword ptr dwData
	}
	*/
}

/*
================
Sys_FPU_SetFTZ
================
*/
void Sys_FPU_SetFTZ( bool enable )
{
	/*
	DWORD dwData;

	_asm {
		movzx	ecx, byte ptr enable
		and		ecx, 1
		shl		ecx, 15
		STMXCSR	dword ptr dwData
		mov		eax, dwData
		and		eax, ~(1<<15)	// clear FTZ bit
		or		eax, ecx		// set the FTZ bit
		mov		dwData, eax
		LDMXCSR	dword ptr dwData
	}
	*/
}
