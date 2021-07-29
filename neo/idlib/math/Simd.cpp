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

#include "precompiled.h"
#pragma hdrstop

#ifdef __APPLE__
	#include <mach/mach_time.h>
#endif

#include "Simd_Generic.h"
#include "Simd_SSE.h"

idSIMDProcessor*		processor = NULL;			// pointer to SIMD processor
idSIMDProcessor* 	generic = NULL;				// pointer to generic SIMD implementation
idSIMDProcessor* 	SIMDProcessor = NULL;

/*
================
idSIMD::Init
================
*/
void idSIMD::Init()
{
	generic = new( TAG_MATH ) idSIMD_Generic;
	generic->cpuid = CPUID_GENERIC;
	processor = NULL;
	SIMDProcessor = generic;
}

/*
============
idSIMD::InitProcessor
============
*/
void idSIMD::InitProcessor( const char* module, bool forceGeneric )
{
	cpuid_t cpuid;
	idSIMDProcessor* newProcessor;

	cpuid = idLib::sys->GetProcessorId();

	if( forceGeneric )
	{

		newProcessor = generic;

	}
	else
	{

		if( processor == NULL )
		{
#if defined(USE_INTRINSICS_SSE)
			if( ( cpuid & CPUID_MMX ) && ( cpuid & CPUID_SSE ) )
			{
				processor = new( TAG_MATH ) idSIMD_SSE;
			}
			else
#endif
			{
				processor = generic;
			}
			processor->cpuid = cpuid;
		}

		newProcessor = processor;
	}

	if( newProcessor != SIMDProcessor )
	{
		SIMDProcessor = newProcessor;
		idLib::common->Printf( "%s using %s for SIMD processing\n", module, SIMDProcessor->GetName() );
	}

	if( cpuid & CPUID_FTZ )
	{
		idLib::sys->FPU_SetFTZ( true );
		idLib::common->Printf( "enabled Flush-To-Zero mode\n" );
	}

	if( cpuid & CPUID_DAZ )
	{
		idLib::sys->FPU_SetDAZ( true );
		idLib::common->Printf( "enabled Denormals-Are-Zero mode\n" );
	}
}

/*
================
idSIMD::Shutdown
================
*/
void idSIMD::Shutdown()
{
	if( processor != generic )
	{
		delete processor;
	}
	delete generic;
	generic = NULL;
	processor = NULL;
	SIMDProcessor = NULL;
}


//===============================================================
//
// Test code
//
//===============================================================

#define COUNT		999			// data count (odd to catch edge cases)
#define BIG_COUNT	COUNT*5		// Some tests need a larger count
#define NUMTESTS	2048		// number of tests

#define RANDOM_SEED		1013904223L	//((int)idLib::sys->GetClockTicks())

idSIMDProcessor* p_simd;
idSIMDProcessor* p_generic;
int baseClocks = 0; // DG: use int instead of long for 64bit compatibility

#if defined(_MSC_VER) && defined(_M_IX86)
#define TIME_TYPE int

#pragma warning(disable : 4731)     // frame pointer register 'ebx' modified by inline assembly code

long saved_ebx = 0;

#define StartRecordTime( start )			\
	__asm mov saved_ebx, ebx				\
	__asm xor eax, eax						\
	__asm cpuid								\
	__asm rdtsc								\
	__asm mov start, eax					\
	__asm xor eax, eax						\
	__asm cpuid

#define StopRecordTime( end )				\
	__asm xor eax, eax						\
	__asm cpuid								\
	__asm rdtsc								\
	__asm mov end, eax						\
	__asm mov ebx, saved_ebx				\
	__asm xor eax, eax						\
	__asm cpuid

#elif defined(__APPLE__) // DG: versions for OSX and others from dhewm3

double ticksPerNanosecond;

#define TIME_TYPE uint64_t

#define StartRecordTime( start )			\
	start = mach_absolute_time();

#define StopRecordTime( end )				\
	end = mach_absolute_time();

#else // not _MSC_VER and _M_IX86 or __APPLE__
// FIXME: meaningful values/functions here for Linux?
#define TIME_TYPE int

#define StartRecordTime( start )			\
	start = 0;

#define StopRecordTime( end )				\
	end = 1;

#endif // DG end

#define GetBest( start, end, best )			\
	if ( !best || end - start < best ) {	\
		best = end - start;					\
	}


/*
============
PrintClocks
============
*/
void PrintClocks( const char* string, int dataCount, int clocks, int otherClocks = 0 )
{
	int i;

	idLib::common->Printf( string );
	for( i = idStr::LengthWithoutColors( string ); i < 48; i++ )
	{
		idLib::common->Printf( " " );
	}
	clocks -= baseClocks;
	if( otherClocks && clocks )
	{
		otherClocks -= baseClocks;
		float p = ( float )otherClocks / ( float )clocks;
		idLib::common->Printf( "c = %4d, clcks = %5d, %.1fX\n", dataCount, clocks, p );
	}
	else
	{
		idLib::common->Printf( "c = %4d, clcks = %5d\n", dataCount, clocks );
	}
}

/*
============
GetBaseClocks
============
*/
void GetBaseClocks()
{
	int i, start, end, bestClocks;

	bestClocks = 0;
	for( i = 0; i < NUMTESTS; i++ )
	{
		StartRecordTime( start );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
	}
	baseClocks = bestClocks;
}

/*
============
TestMinMax
============
*/
void TestMinMax()
{
	int i;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( float fsrc0[COUNT] );
	ALIGN16( idVec2 v2src0[COUNT] );
	ALIGN16( idVec3 v3src0[COUNT] );
	ALIGN16( idDrawVert drawVerts[COUNT] );
	ALIGN16( triIndex_t indexes[COUNT] );
	float min = 0.0f, max = 0.0f, min2 = 0.0f, max2 = 0.0f;
	idVec2 v2min, v2max, v2min2, v2max2;
	idVec3 vmin, vmax, vmin2, vmax2;
	const char* result;

	idRandom srnd( RANDOM_SEED );

	for( i = 0; i < COUNT; i++ )
	{
		fsrc0[i] = srnd.CRandomFloat() * 10.0f;
		v2src0[i][0] = srnd.CRandomFloat() * 10.0f;
		v2src0[i][1] = srnd.CRandomFloat() * 10.0f;
		v3src0[i][0] = srnd.CRandomFloat() * 10.0f;
		v3src0[i][1] = srnd.CRandomFloat() * 10.0f;
		v3src0[i][2] = srnd.CRandomFloat() * 10.0f;
		drawVerts[i].xyz = v3src0[i];
		indexes[i] = i;
	}

	idLib::common->Printf( "====================================\n" );

	bestClocksGeneric = 0;
	for( i = 0; i < NUMTESTS; i++ )
	{
		min = idMath::INFINITUM;
		max = -idMath::INFINITUM;
		StartRecordTime( start );
		p_generic->MinMax( min, max, fsrc0, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->MinMax( float[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for( i = 0; i < NUMTESTS; i++ )
	{
		StartRecordTime( start );
		p_simd->MinMax( min2, max2, fsrc0, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	result = ( min == min2 && max == max2 ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->MinMax( float[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );

	bestClocksGeneric = 0;
	for( i = 0; i < NUMTESTS; i++ )
	{
		StartRecordTime( start );
		p_generic->MinMax( v2min, v2max, v2src0, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->MinMax( idVec2[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for( i = 0; i < NUMTESTS; i++ )
	{
		StartRecordTime( start );
		p_simd->MinMax( v2min2, v2max2, v2src0, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	result = ( v2min == v2min2 && v2max == v2max2 ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->MinMax( idVec2[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );

	bestClocksGeneric = 0;
	for( i = 0; i < NUMTESTS; i++ )
	{
		StartRecordTime( start );
		p_generic->MinMax( vmin, vmax, v3src0, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->MinMax( idVec3[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for( i = 0; i < NUMTESTS; i++ )
	{
		StartRecordTime( start );
		p_simd->MinMax( vmin2, vmax2, v3src0, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	result = ( vmin == vmin2 && vmax == vmax2 ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->MinMax( idVec3[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );

	bestClocksGeneric = 0;
	for( i = 0; i < NUMTESTS; i++ )
	{
		StartRecordTime( start );
		p_generic->MinMax( vmin, vmax, drawVerts, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->MinMax( idDrawVert[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for( i = 0; i < NUMTESTS; i++ )
	{
		StartRecordTime( start );
		p_simd->MinMax( vmin2, vmax2, drawVerts, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	result = ( vmin == vmin2 && vmax == vmax2 ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->MinMax( idDrawVert[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );

	bestClocksGeneric = 0;
	for( i = 0; i < NUMTESTS; i++ )
	{
		StartRecordTime( start );
		p_generic->MinMax( vmin, vmax, drawVerts, indexes, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->MinMax( idDrawVert[], indexes[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for( i = 0; i < NUMTESTS; i++ )
	{
		StartRecordTime( start );
		p_simd->MinMax( vmin2, vmax2, drawVerts, indexes, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	result = ( vmin == vmin2 && vmax == vmax2 ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->MinMax( idDrawVert[], indexes[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestMemcpy
============
*/
void TestMemcpy()
{
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	int i;
	byte test0[BIG_COUNT];
	byte test1[BIG_COUNT];
	const char* result;

	idRandom random( RANDOM_SEED );
	for( i = 0; i < BIG_COUNT; i++ )
	{
		test0[i] = random.RandomInt( 255 );
	}

	idLib::common->Printf( "====================================\n" );

	bestClocksGeneric = 0;
	for( i = 0; i < NUMTESTS; i++ )
	{
		StartRecordTime( start );
		p_generic->Memcpy( test1, test0, BIG_COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->Memcpy()", BIG_COUNT, bestClocksGeneric );

	for( i = 0; i < BIG_COUNT; i++ )
	{
		test0[i] = random.RandomInt( 255 );
	}

	bestClocksSIMD = 0;
	for( i = 0; i < NUMTESTS; i++ )
	{
		StartRecordTime( start );
		p_simd->Memcpy( test1, test0, BIG_COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}
	for( i = 0; i < BIG_COUNT; i++ )
	{
		if( test1[i] != test0[i] )
		{
			break;
		}
	}
	result = ( i >= BIG_COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->Memcpy() %s", result ), BIG_COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestMemset
============
*/
void TestMemset()
{
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	int i, j;
	const char* result;
	byte test0[BIG_COUNT];

	idRandom random( RANDOM_SEED );
	j = 1 + random.RandomInt( 254 );

	idLib::common->Printf( "====================================\n" );

	bestClocksGeneric = 0;
	for( i = 0; i < NUMTESTS; i++ )
	{
		StartRecordTime( start );
		p_generic->Memset( test0, j, BIG_COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->Memset()", BIG_COUNT, bestClocksGeneric );

	j = 1 + random.RandomInt( 254 );

	bestClocksSIMD = 0;
	for( i = 0; i < NUMTESTS; i++ )
	{
		StartRecordTime( start );
		p_simd->Memset( test0, j, BIG_COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}
	for( i = 0; i < BIG_COUNT; i++ )
	{
		if( test0[i] != j )
		{
			break;
		}
	}
	result = ( i >= BIG_COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->Memset() %s", result ), BIG_COUNT, bestClocksSIMD, bestClocksGeneric );

	j = 0;

	bestClocksGeneric = 0;
	for( i = 0; i < NUMTESTS; i++ )
	{
		StartRecordTime( start );
		p_generic->Memset( test0, j, BIG_COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->Memset( 0 )", BIG_COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for( i = 0; i < NUMTESTS; i++ )
	{
		StartRecordTime( start );
		p_simd->Memset( test0, j, BIG_COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}
	for( i = 0; i < BIG_COUNT; i++ )
	{
		if( test0[i] != j )
		{
			break;
		}
	}
	result = ( i >= BIG_COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->Memset( 0 ) %s", result ), BIG_COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestBlendJoints
============
*/
void TestBlendJoints()
{
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	idTempArray< idJointQuat > baseJoints( COUNT );
	idTempArray< idJointQuat > joints1( COUNT );
	idTempArray< idJointQuat > joints2( COUNT );
	idTempArray< idJointQuat > blendJoints( COUNT );
	idTempArray< int > index( COUNT );
	float lerp = 0.3f;
	const char* result;

	idRandom srnd( RANDOM_SEED );

	for( i = 0; i < COUNT; i++ )
	{
		idAngles angles;
		angles[0] = srnd.CRandomFloat() * 180.0f;
		angles[1] = srnd.CRandomFloat() * 180.0f;
		angles[2] = srnd.CRandomFloat() * 180.0f;
		baseJoints[i].q = angles.ToQuat();
		baseJoints[i].t[0] = srnd.CRandomFloat() * 10.0f;
		baseJoints[i].t[1] = srnd.CRandomFloat() * 10.0f;
		baseJoints[i].t[2] = srnd.CRandomFloat() * 10.0f;
		baseJoints[i].w = 0.0f;
		angles[0] = srnd.CRandomFloat() * 180.0f;
		angles[1] = srnd.CRandomFloat() * 180.0f;
		angles[2] = srnd.CRandomFloat() * 180.0f;
		blendJoints[i].q = angles.ToQuat();
		blendJoints[i].t[0] = srnd.CRandomFloat() * 10.0f;
		blendJoints[i].t[1] = srnd.CRandomFloat() * 10.0f;
		blendJoints[i].t[2] = srnd.CRandomFloat() * 10.0f;
		blendJoints[i].w = 0.0f;
		index[i] = i;
	}

	bestClocksGeneric = 0;
	for( i = 0; i < NUMTESTS; i++ )
	{
		for( j = 0; j < COUNT; j++ )
		{
			joints1[j] = baseJoints[j];
		}
		StartRecordTime( start );
		p_generic->BlendJoints( joints1.Ptr(), blendJoints.Ptr(), lerp, index.Ptr(), COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->BlendJoints()", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for( i = 0; i < NUMTESTS; i++ )
	{
		for( j = 0; j < COUNT; j++ )
		{
			joints2[j] = baseJoints[j];
		}
		StartRecordTime( start );
		p_simd->BlendJoints( joints2.Ptr(), blendJoints.Ptr(), lerp, index.Ptr(), COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for( i = 0; i < COUNT; i++ )
	{
		if( !joints1[i].t.Compare( joints2[i].t, 1e-3f ) )
		{
			break;
		}
		if( !joints1[i].q.Compare( joints2[i].q, 1e-2f ) )
		{
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->BlendJoints() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestBlendJoints
============
*/
void TestBlendJointsFast()
{
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	idTempArray< idJointQuat > baseJoints( COUNT );
	idTempArray< idJointQuat > joints1( COUNT );
	idTempArray< idJointQuat > joints2( COUNT );
	idTempArray< idJointQuat > blendJoints( COUNT );
	idTempArray< int > index( COUNT );
	float lerp = 0.3f;
	const char* result;

	idRandom srnd( RANDOM_SEED );

	for( i = 0; i < COUNT; i++ )
	{
		idAngles angles;
		angles[0] = srnd.CRandomFloat() * 180.0f;
		angles[1] = srnd.CRandomFloat() * 180.0f;
		angles[2] = srnd.CRandomFloat() * 180.0f;
		baseJoints[i].q = angles.ToQuat();
		baseJoints[i].t[0] = srnd.CRandomFloat() * 10.0f;
		baseJoints[i].t[1] = srnd.CRandomFloat() * 10.0f;
		baseJoints[i].t[2] = srnd.CRandomFloat() * 10.0f;
		baseJoints[i].w = 0.0f;
		angles[0] = srnd.CRandomFloat() * 180.0f;
		angles[1] = srnd.CRandomFloat() * 180.0f;
		angles[2] = srnd.CRandomFloat() * 180.0f;
		blendJoints[i].q = angles.ToQuat();
		blendJoints[i].t[0] = srnd.CRandomFloat() * 10.0f;
		blendJoints[i].t[1] = srnd.CRandomFloat() * 10.0f;
		blendJoints[i].t[2] = srnd.CRandomFloat() * 10.0f;
		blendJoints[i].w = 0.0f;
		index[i] = i;
	}

	bestClocksGeneric = 0;
	for( i = 0; i < NUMTESTS; i++ )
	{
		for( j = 0; j < COUNT; j++ )
		{
			joints1[j] = baseJoints[j];
		}
		StartRecordTime( start );
		p_generic->BlendJointsFast( joints1.Ptr(), blendJoints.Ptr(), lerp, index.Ptr(), COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->BlendJointsFast()", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for( i = 0; i < NUMTESTS; i++ )
	{
		for( j = 0; j < COUNT; j++ )
		{
			joints2[j] = baseJoints[j];
		}
		StartRecordTime( start );
		p_simd->BlendJointsFast( joints2.Ptr(), blendJoints.Ptr(), lerp, index.Ptr(), COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for( i = 0; i < COUNT; i++ )
	{
		if( !joints1[i].t.Compare( joints2[i].t, 1e-3f ) )
		{
			break;
		}
		if( !joints1[i].q.Compare( joints2[i].q, 1e-2f ) )
		{
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->BlendJointsFast() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestConvertJointQuatsToJointMats
============
*/
void TestConvertJointQuatsToJointMats()
{
	int i;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	idTempArray< idJointQuat > baseJoints( COUNT );
	idTempArray< idJointMat > joints1( COUNT );
	idTempArray< idJointMat > joints2( COUNT );
	const char* result;

	idRandom srnd( RANDOM_SEED );

	for( i = 0; i < COUNT; i++ )
	{
		idAngles angles;
		angles[0] = srnd.CRandomFloat() * 180.0f;
		angles[1] = srnd.CRandomFloat() * 180.0f;
		angles[2] = srnd.CRandomFloat() * 180.0f;
		baseJoints[i].q = angles.ToQuat();
		baseJoints[i].t[0] = srnd.CRandomFloat() * 10.0f;
		baseJoints[i].t[1] = srnd.CRandomFloat() * 10.0f;
		baseJoints[i].t[2] = srnd.CRandomFloat() * 10.0f;
	}

	bestClocksGeneric = 0;
	for( i = 0; i < NUMTESTS; i++ )
	{
		StartRecordTime( start );
		p_generic->ConvertJointQuatsToJointMats( joints1.Ptr(), baseJoints.Ptr(), COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->ConvertJointQuatsToJointMats()", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for( i = 0; i < NUMTESTS; i++ )
	{
		StartRecordTime( start );
		p_simd->ConvertJointQuatsToJointMats( joints2.Ptr(), baseJoints.Ptr(), COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for( i = 0; i < COUNT; i++ )
	{
		if( !joints1[i].Compare( joints2[i], 1e-4f ) )
		{
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->ConvertJointQuatsToJointMats() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestConvertJointMatsToJointQuats
============
*/
void TestConvertJointMatsToJointQuats()
{
	int i;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	idTempArray< idJointMat > baseJoints( COUNT );
	idTempArray< idJointQuat > joints1( COUNT );
	idTempArray< idJointQuat > joints2( COUNT );
	const char* result;

	idRandom srnd( RANDOM_SEED );

	for( i = 0; i < COUNT; i++ )
	{
		idAngles angles;
		angles[0] = srnd.CRandomFloat() * 180.0f;
		angles[1] = srnd.CRandomFloat() * 180.0f;
		angles[2] = srnd.CRandomFloat() * 180.0f;
		baseJoints[i].SetRotation( angles.ToMat3() );
		idVec3 v;
		v[0] = srnd.CRandomFloat() * 10.0f;
		v[1] = srnd.CRandomFloat() * 10.0f;
		v[2] = srnd.CRandomFloat() * 10.0f;
		baseJoints[i].SetTranslation( v );
	}

	bestClocksGeneric = 0;
	for( i = 0; i < NUMTESTS; i++ )
	{
		StartRecordTime( start );
		p_generic->ConvertJointMatsToJointQuats( joints1.Ptr(), baseJoints.Ptr(), COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->ConvertJointMatsToJointQuats()", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for( i = 0; i < NUMTESTS; i++ )
	{
		StartRecordTime( start );
		p_simd->ConvertJointMatsToJointQuats( joints2.Ptr(), baseJoints.Ptr(), COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for( i = 0; i < COUNT; i++ )
	{
		if( !joints1[i].q.Compare( joints2[i].q, 1e-4f ) )
		{
			break;
		}
		if( !joints1[i].t.Compare( joints2[i].t, 1e-4f ) )
		{
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->ConvertJointMatsToJointQuats() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestTransformJoints
============
*/
void TestTransformJoints()
{
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	idTempArray< idJointMat > joints( COUNT + 1 );
	idTempArray< idJointMat > joints1( COUNT + 1 );
	idTempArray< idJointMat > joints2( COUNT + 1 );
	idTempArray< int > parents( COUNT + 1 );
	const char* result;

	idRandom srnd( RANDOM_SEED );

	for( i = 0; i <= COUNT; i++ )
	{
		idAngles angles;
		angles[0] = srnd.CRandomFloat() * 180.0f;
		angles[1] = srnd.CRandomFloat() * 180.0f;
		angles[2] = srnd.CRandomFloat() * 180.0f;
		joints[i].SetRotation( angles.ToMat3() );
		idVec3 v;
		v[0] = srnd.CRandomFloat() * 2.0f;
		v[1] = srnd.CRandomFloat() * 2.0f;
		v[2] = srnd.CRandomFloat() * 2.0f;
		joints[i].SetTranslation( v );
		parents[i] = i - 1;
	}

	bestClocksGeneric = 0;
	for( i = 0; i < NUMTESTS; i++ )
	{
		for( j = 0; j <= COUNT; j++ )
		{
			joints1[j] = joints[j];
		}
		StartRecordTime( start );
		p_generic->TransformJoints( joints1.Ptr(), parents.Ptr(), 1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->TransformJoints()", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for( i = 0; i < NUMTESTS; i++ )
	{
		for( j = 0; j <= COUNT; j++ )
		{
			joints2[j] = joints[j];
		}
		StartRecordTime( start );
		p_simd->TransformJoints( joints2.Ptr(), parents.Ptr(), 1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for( i = 1; i <= COUNT; i++ )
	{
		if( !joints1[i].Compare( joints2[i], 1e-3f ) )
		{
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->TransformJoints() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestUntransformJoints
============
*/
void TestUntransformJoints()
{
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	idTempArray< idJointMat > joints( COUNT + 1 );
	idTempArray< idJointMat > joints1( COUNT + 1 );
	idTempArray< idJointMat > joints2( COUNT + 1 );
	idTempArray< int > parents( COUNT + 1 );
	const char* result;

	idRandom srnd( RANDOM_SEED );

	for( i = 0; i <= COUNT; i++ )
	{
		idAngles angles;
		angles[0] = srnd.CRandomFloat() * 180.0f;
		angles[1] = srnd.CRandomFloat() * 180.0f;
		angles[2] = srnd.CRandomFloat() * 180.0f;
		joints[i].SetRotation( angles.ToMat3() );
		idVec3 v;
		v[0] = srnd.CRandomFloat() * 2.0f;
		v[1] = srnd.CRandomFloat() * 2.0f;
		v[2] = srnd.CRandomFloat() * 2.0f;
		joints[i].SetTranslation( v );
		parents[i] = i - 1;
	}

	bestClocksGeneric = 0;
	for( i = 0; i < NUMTESTS; i++ )
	{
		for( j = 0; j <= COUNT; j++ )
		{
			joints1[j] = joints[j];
		}
		StartRecordTime( start );
		p_generic->UntransformJoints( joints1.Ptr(), parents.Ptr(), 1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->UntransformJoints()", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for( i = 0; i < NUMTESTS; i++ )
	{
		for( j = 0; j <= COUNT; j++ )
		{
			joints2[j] = joints[j];
		}
		StartRecordTime( start );
		p_simd->UntransformJoints( joints2.Ptr(), parents.Ptr(), 1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for( i = 1; i <= COUNT; i++ )
	{
		if( !joints1[i].Compare( joints2[i], 1e-3f ) )
		{
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->UntransformJoints() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestMath
============
*/
void TestMath()
{
	int i;
	TIME_TYPE start, end, bestClocks;

	idLib::common->Printf( "====================================\n" );

	float tst = -1.0f;
	float tst2 = 1.0f;
	float testvar = 1.0f;
	idRandom rnd;

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for( i = 0; i < NUMTESTS; i++ )
	{
		StartRecordTime( start );
		tst = fabs( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "            fabs( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for( i = 0; i < NUMTESTS; i++ )
	{
		StartRecordTime( start );
		int tmp = * ( int* ) &tst;
		tmp &= 0x7FFFFFFF;
		tst = * ( float* ) &tmp;
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "    idMath::Fabs( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = 10.0f + 100.0f * rnd.RandomFloat();
	for( i = 0; i < NUMTESTS; i++ )
	{
		StartRecordTime( start );
		tst = sqrt( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst * 0.01f;
		tst = 10.0f + 100.0f * rnd.RandomFloat();
	}
	PrintClocks( "            sqrt( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.RandomFloat();
	for( i = 0; i < NUMTESTS; i++ )
	{
		StartRecordTime( start );
		tst = idMath::Sqrt( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.RandomFloat();
	}
	PrintClocks( "    idMath::Sqrt( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.RandomFloat();
	for( i = 0; i < NUMTESTS; i++ )
	{
		StartRecordTime( start );
		tst = idMath::Sqrt16( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.RandomFloat();
	}
	PrintClocks( "  idMath::Sqrt16( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for( i = 0; i < NUMTESTS; i++ )
	{
		StartRecordTime( start );
		tst = idMath::Sin( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "     idMath::Sin( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for( i = 0; i < NUMTESTS; i++ )
	{
		StartRecordTime( start );
		tst = idMath::Sin16( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "   idMath::Sin16( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for( i = 0; i < NUMTESTS; i++ )
	{
		StartRecordTime( start );
		tst = idMath::Cos( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "     idMath::Cos( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for( i = 0; i < NUMTESTS; i++ )
	{
		StartRecordTime( start );
		tst = idMath::Cos16( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "   idMath::Cos16( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for( i = 0; i < NUMTESTS; i++ )
	{
		StartRecordTime( start );
		idMath::SinCos( tst, tst, tst2 );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "  idMath::SinCos( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for( i = 0; i < NUMTESTS; i++ )
	{
		StartRecordTime( start );
		idMath::SinCos16( tst, tst, tst2 );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "idMath::SinCos16( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for( i = 0; i < NUMTESTS; i++ )
	{
		StartRecordTime( start );
		tst = idMath::Tan( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "     idMath::Tan( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for( i = 0; i < NUMTESTS; i++ )
	{
		StartRecordTime( start );
		tst = idMath::Tan16( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "   idMath::Tan16( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for( i = 0; i < NUMTESTS; i++ )
	{
		StartRecordTime( start );
		tst = idMath::ASin( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst * ( 1.0f / idMath::PI );
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "    idMath::ASin( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for( i = 0; i < NUMTESTS; i++ )
	{
		StartRecordTime( start );
		tst = idMath::ASin16( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst * ( 1.0f / idMath::PI );
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "  idMath::ASin16( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for( i = 0; i < NUMTESTS; i++ )
	{
		StartRecordTime( start );
		tst = idMath::ACos( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst * ( 1.0f / idMath::PI );
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "    idMath::ACos( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for( i = 0; i < NUMTESTS; i++ )
	{
		StartRecordTime( start );
		tst = idMath::ACos16( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst * ( 1.0f / idMath::PI );
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "  idMath::ACos16( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for( i = 0; i < NUMTESTS; i++ )
	{
		StartRecordTime( start );
		tst = idMath::ATan( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "    idMath::ATan( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for( i = 0; i < NUMTESTS; i++ )
	{
		StartRecordTime( start );
		tst = idMath::ATan16( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "  idMath::ATan16( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for( i = 0; i < NUMTESTS; i++ )
	{
		StartRecordTime( start );
		tst = idMath::Pow( 2.7f, tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst * 0.1f;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "    idMath::Pow( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for( i = 0; i < NUMTESTS; i++ )
	{
		StartRecordTime( start );
		tst = idMath::Pow16( 2.7f, tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst * 0.1f;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "  idMath::Pow16( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for( i = 0; i < NUMTESTS; i++ )
	{
		StartRecordTime( start );
		tst = idMath::Exp( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst * 0.1f;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "    idMath::Exp( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for( i = 0; i < NUMTESTS; i++ )
	{
		StartRecordTime( start );
		tst = idMath::Exp16( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst * 0.1f;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "  idMath::Exp16( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for( i = 0; i < NUMTESTS; i++ )
	{
		tst = fabs( tst ) + 1.0f;
		StartRecordTime( start );
		tst = idMath::Log( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "    idMath::Log( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for( i = 0; i < NUMTESTS; i++ )
	{
		tst = fabs( tst ) + 1.0f;
		StartRecordTime( start );
		tst = idMath::Log16( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "  idMath::Log16( tst )", 1, bestClocks );

	idLib::common->Printf( "testvar = %f\n", testvar );

	idMat3 resultMat3;
	idQuat fromQuat, toQuat, resultQuat;
	idCQuat cq;
	idAngles ang;

	fromQuat = idAngles( 30, 45, 0 ).ToQuat();
	toQuat = idAngles( 45, 0, 0 ).ToQuat();
	cq = idAngles( 30, 45, 0 ).ToQuat().ToCQuat();
	ang = idAngles( 30, 40, 50 );

	bestClocks = 0;
	for( i = 0; i < NUMTESTS; i++ )
	{
		StartRecordTime( start );
		resultMat3 = fromQuat.ToMat3();
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
	}
	PrintClocks( "       idQuat::ToMat3()", 1, bestClocks );

	bestClocks = 0;
	for( i = 0; i < NUMTESTS; i++ )
	{
		StartRecordTime( start );
		resultQuat.Slerp( fromQuat, toQuat, 0.3f );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
	}
	PrintClocks( "        idQuat::Slerp()", 1, bestClocks );

	bestClocks = 0;
	for( i = 0; i < NUMTESTS; i++ )
	{
		StartRecordTime( start );
		resultQuat = cq.ToQuat();
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
	}
	PrintClocks( "      idCQuat::ToQuat()", 1, bestClocks );

	bestClocks = 0;
	for( i = 0; i < NUMTESTS; i++ )
	{
		StartRecordTime( start );
		resultQuat = ang.ToQuat();
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
	}
	PrintClocks( "     idAngles::ToQuat()", 1, bestClocks );

	bestClocks = 0;
	for( i = 0; i < NUMTESTS; i++ )
	{
		StartRecordTime( start );
		resultMat3 = ang.ToMat3();
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
	}
	PrintClocks( "     idAngles::ToMat3()", 1, bestClocks );
}

/*
============
idSIMD::Test_f
============
*/
void idSIMD::Test_f( const idCmdArgs& args )
{

	// RB begin
#if defined(_WIN32)
	SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL );
#endif
	// RB end

	p_simd = processor;
	p_generic = generic;

	if( idStr::Length( args.Argv( 1 ) ) != 0 )
	{
		cpuid_t cpuid = idLib::sys->GetProcessorId();
		idStr argString = args.Args();

		argString.Replace( " ", "" );

#if defined(USE_INTRINSICS_SSE)
		if( idStr::Icmp( argString, "SSE" ) == 0 )
		{
			if( !( cpuid & CPUID_MMX ) || !( cpuid & CPUID_SSE ) )
			{
				common->Printf( "CPU does not support MMX & SSE\n" );
				return;
			}
			p_simd = new( TAG_MATH ) idSIMD_SSE;
		}
		else
#endif
		{
			common->Printf( "invalid argument, use: MMX, 3DNow, SSE, SSE2, SSE3, AltiVec\n" );
			return;
		}
	}

	idLib::common->SetRefreshOnPrint( true );

	idLib::common->Printf( "using %s for SIMD processing\n", p_simd->GetName() );

	GetBaseClocks();

	TestMath();
	TestMinMax();
	TestMemcpy();
	TestMemset();

	idLib::common->Printf( "====================================\n" );

	TestBlendJoints();
	TestBlendJointsFast();
	TestConvertJointQuatsToJointMats();
	TestConvertJointMatsToJointQuats();
	TestTransformJoints();
	TestUntransformJoints();

	idLib::common->Printf( "====================================\n" );

	idLib::common->SetRefreshOnPrint( false );

	if( p_simd != processor )
	{
		delete p_simd;
	}
	p_simd = NULL;
	p_generic = NULL;

	// RB begin
#if defined(_WIN32)
	SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_NORMAL );
#endif
	// RB end
}
