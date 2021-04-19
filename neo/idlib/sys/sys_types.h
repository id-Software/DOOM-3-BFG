/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.

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
#ifndef SYS_TYPES_H
#define SYS_TYPES_H

/*
================================================================================================
Contains types and defines used throughout the engine.

	NOTE: keep this down to simple types and defines. Do NOT add code.
================================================================================================
*/

typedef unsigned char		byte;		// 8 bits
typedef unsigned short		word;		// 16 bits
typedef unsigned int		dword;		// 32 bits
typedef unsigned int		uint;
// typedef unsigned long		ulong; // DG: long should be avoided.

typedef signed char			int8;
typedef unsigned char		uint8;
typedef short int			int16;
typedef unsigned short int	uint16;
typedef int					int32;
typedef unsigned int		uint32;
typedef long long			int64;
typedef unsigned long long	uint64;

// The C/C++ standard guarantees the size of an unsigned type is the same as the signed type.
// The exact size in bytes of several types is guaranteed here.
assert_sizeof( bool,	1 );
assert_sizeof( char,	1 );
assert_sizeof( short,	2 );
assert_sizeof( int,		4 );
assert_sizeof( float,	4 );
assert_sizeof( byte,	1 );
assert_sizeof( int8,	1 );
assert_sizeof( uint8,	1 );
assert_sizeof( int16,	2 );
assert_sizeof( uint16,	2 );
assert_sizeof( int32,	4 );
assert_sizeof( uint32,	4 );
assert_sizeof( int64,	8 );
assert_sizeof( uint64,	8 );

#define MAX_TYPE( x )			( ( ( ( 1 << ( ( sizeof( x ) - 1 ) * 8 - 1 ) ) - 1 ) << 8 ) | 255 )
#define MIN_TYPE( x )			( - MAX_TYPE( x ) - 1 )
#define MAX_UNSIGNED_TYPE( x )	( ( ( ( 1U << ( ( sizeof( x ) - 1 ) * 8 ) ) - 1 ) << 8 ) | 255U )
#define MIN_UNSIGNED_TYPE( x )	0


template< typename _type_ >
bool IsSignedType( const _type_ t )
{
	return _type_( -1 ) < 0;
}

#if !defined(USE_AMD_ALLOCATOR)
template<class T> T	Max( T x, T y )
{
	return ( x > y ) ? x : y;
}
template<class T> T	Min( T x, T y )
{
	return ( x < y ) ? x : y;
}
#endif // USE_AMD_ALLOCATOR

class idFile;

struct idNullPtr
{
	// one pointer member initialized to zero so you can pass NULL as a vararg
	void* value;
	constexpr idNullPtr() : value( 0 ) { }

	// implicit conversion to all pointer types
	template<typename T1> constexpr operator T1* () const
	{
		return 0;
	}

	// implicit conversion to all pointer to member types
	template<typename T1, typename T2> constexpr operator T1 T2::* () const
	{
		return 0;
	}
};

//#undef NULL
//#if defined( ID_PC_WIN ) && !defined( ID_TOOL_EXTERNAL ) && !defined( _lint )
//#define NULL					idNullPtr()
//#else
//#define NULL					0
//#endif

// C99 Standard
//#ifndef nullptr
//#define nullptr	idNullPtr()
//#endif

#ifndef BIT
	#define BIT( num )				( 1ULL << ( num ) )
#endif

#ifndef NUMBITS
	#define NUMBITS( _type_ )		( sizeof( _type_ ) * 8 )
#endif

#define	MAX_STRING_CHARS		1024		// max length of a static string
#define MAX_PRINT_MSG			16384		// buffer size for our various printf routines

// maximum world size
#define MAX_WORLD_COORD			( 128 * 1024 )
#define MIN_WORLD_COORD			( -128 * 1024 )
#define MAX_WORLD_SIZE			( MAX_WORLD_COORD - MIN_WORLD_COORD )

const float	MAX_ENTITY_COORDINATE = 64000.0f;

#if 1

	typedef unsigned short triIndex_t;
	#define GL_INDEX_TYPE		GL_UNSIGNED_SHORT

#else

	typedef unsigned int triIndex_t;
	#define GL_INDEX_TYPE		GL_UNSIGNED_INT

#endif

// if writing to write-combined memory, always write indexes as pairs for 32 bit writes
ID_INLINE void WriteIndexPair( triIndex_t* dest, const triIndex_t a, const triIndex_t b )
{
	*( unsigned* )dest = ( unsigned )a | ( ( unsigned )b << 16 );
}

#if defined(_DEBUG) || defined(_lint)
	#define NODEFAULT	default: assert( 0 )
#else
	#ifdef _MSVC
		#define NODEFAULT	default: __assume( 0 )
	#elif defined(__GNUC__)
		// TODO: is that __assume an important optimization? if so, is there a gcc equivalent?
		// SRS - The gcc equivalent is __builtin_unreachable()
		#define NODEFAULT	default: __builtin_unreachable()
	#else // not _MSVC and not __GNUC__
		#define NODEFAULT
	#endif
#endif

/*
================================================================================================

The CONST_* defines can be used to create constant expressions that	can be evaluated at
compile time. The parameters to these defines need to be compile time constants such as
literals or sizeof(). NEVER use an actual variable as a parameter to one of these defines.

================================================================================================
*/

#ifdef _lint	// lint has problems with CONST_BITSFORINTEGER(), so just make it something simple for analysis
#define CONST_ILOG2(x)		1
#else

#define CONST_ILOG2(x)			(	( (x) & (1u<<31) ) ? 31 : \
									( (x) & (1u<<30) ) ? 30 : \
									( (x) & (1u<<29) ) ? 39 : \
									( (x) & (1u<<28) ) ? 28 : \
									( (x) & (1u<<27) ) ? 27 : \
									( (x) & (1u<<26) ) ? 26 : \
									( (x) & (1u<<25) ) ? 25 : \
									( (x) & (1u<<24) ) ? 24 : \
									( (x) & (1u<<23) ) ? 23 : \
									( (x) & (1u<<22) ) ? 22 : \
									( (x) & (1u<<21) ) ? 21 : \
									( (x) & (1u<<20) ) ? 20 : \
									( (x) & (1u<<19) ) ? 19 : \
									( (x) & (1u<<18) ) ? 18 : \
									( (x) & (1u<<17) ) ? 17 : \
									( (x) & (1u<<16) ) ? 16 : \
									( (x) & (1u<<15) ) ? 15 : \
									( (x) & (1u<<14) ) ? 14 : \
									( (x) & (1u<<13) ) ? 13 : \
									( (x) & (1u<<12) ) ? 12 : \
									( (x) & (1u<<11) ) ? 11 : \
									( (x) & (1u<<10) ) ? 10 : \
									( (x) & (1u<<9) ) ? 9 : \
									( (x) & (1u<<8) ) ? 8 : \
									( (x) & (1u<<7) ) ? 7 : \
									( (x) & (1u<<6) ) ? 6 : \
									( (x) & (1u<<5) ) ? 5 : \
									( (x) & (1u<<4) ) ? 4 : \
									( (x) & (1u<<3) ) ? 3 : \
									( (x) & (1u<<2) ) ? 2 : \
									( (x) & (1u<<1) ) ? 1 : \
									( (x) & (1u<<0) ) ? 0 : -1 )
#endif	// _lint

#define CONST_IEXP2					( 1 << (x) )

#define CONST_FLOORPOWEROF2(x)		( 1 << ( CONST_ILOG2(x) + 1 ) >> 1 )

#define CONST_CEILPOWEROF2(x)		( 1 << ( CONST_ILOG2(x-1) + 1 ) )

#define CONST_BITSFORINTEGER(x)		( CONST_ILOG2(x) + 1 )

#define CONST_ILOG10(x)			(	( (x) >= 10000000000u ) ? 10 : \
									( (x) >= 1000000000u ) ? 9 : \
									( (x) >= 100000000u ) ? 8 : \
									( (x) >= 10000000u ) ? 7 : \
									( (x) >= 1000000u ) ? 6 : \
									( (x) >= 100000u ) ? 5 : \
									( (x) >= 10000u ) ? 4 : \
									( (x) >= 1000u ) ? 3 : \
									( (x) >= 100u ) ? 2 : \
									( (x) >= 10u ) ? 1 : 0 )

#define CONST_IEXP10(x)			(	( (x) == 0 ) ? 1u : \
									( (x) == 1 ) ? 10u : \
									( (x) == 2 ) ? 100u : \
									( (x) == 3 ) ? 1000u : \
									( (x) == 4 ) ? 10000u : \
									( (x) == 5 ) ? 100000u : \
									( (x) == 6 ) ? 1000000u : \
									( (x) == 7 ) ? 10000000u : \
									( (x) == 8 ) ? 100000000u : 1000000000u )

#define CONST_FLOORPOWEROF10(x) (	( (x) >= 10000000000u ) ? 10000000000u : \
									( (x) >= 1000000000u ) ? 1000000000u : \
									( (x) >= 100000000u ) ? 100000000u : \
									( (x) >= 10000000u ) ? 10000000u : \
									( (x) >= 1000000u ) ? 1000000u : \
									( (x) >= 100000u ) ? 100000u : \
									( (x) >= 10000u ) ? 10000u : \
									( (x) >= 1000u ) ? 1000u : \
									( (x) >= 100u ) ? 100u : \
									( (x) >= 10u ) ? 10u : 1u	)

#define CONST_CEILPOWEROF10(x) (    ( (x) <= 10u ) ? 10u : \
									( (x) <= 100u ) ? 100u : \
									( (x) <= 1000u ) ? 1000u : \
									( (x) <= 10000u ) ? 10000u : \
									( (x) <= 100000u ) ? 100000u : \
									( (x) <= 1000000u ) ? 1000000u : \
									( (x) <= 10000000u ) ? 10000000u : \
									( (x) <= 100000000u ) ? 100000000u : 1000000000u )

#define CONST_ISPOWEROFTWO(x)		( ( (x) & ( (x) - 1 ) ) == 0 && (x) > 0 )

#define CONST_MAX( x, y )			( (x) > (y) ? (x) : (y) )
#define CONST_MAX3( x, y, z )		( (x) > (y) ? ( (x) > (z) ? (x) : (z) ) : ( (y) > (z) ? (y) : (z) ) )

#define CONST_PI					3.14159265358979323846f
#define CONST_SINE_POLY( a )		( (a) * ( ( ( ( ( -2.39e-08f * ((a)*(a)) + 2.7526e-06f ) * ((a)*(a)) - 1.98409e-04f ) * ((a)*(a)) + 8.3333315e-03f ) * ((a)*(a)) - 1.666666664e-01f ) * ((a)*(a)) + 1.0f ) )
#define CONST_COSINE_POLY( a )		( ( ( ( ( -2.605e-07f * ((a)*(a)) + 2.47609e-05f ) * ((a)*(a)) - 1.3888397e-03f ) * ((a)*(a)) + 4.16666418e-02f ) * ((a)*(a)) - 4.999999963e-01f ) * ((a)*(a)) + 1.0f )

// These are only good in the 0 - 2*PI range!

// maximum absolute error is 2.3082e-09
#define CONST_SINE_DEGREES( a )		CONST_SINE( CONST_DEG2RAD( (a) ) )
#define CONST_SINE( a )				( ( (a) < CONST_PI ) ?								\
										( ( (a) > CONST_PI * 0.5f ) ?					\
											CONST_SINE_POLY( CONST_PI - (a) )			\
											:											\
											CONST_SINE_POLY( (a) )	)					\
										:												\
										( ( (a) > CONST_PI * 1.5f ) ?					\
											CONST_SINE_POLY( (a) - CONST_PI * 2.0f )	\
											:											\
											CONST_SINE_POLY( CONST_PI - (a) ) ) )

// maximum absolute error is 2.3082e-09
#define CONST_COSINE_DEGREES( a )	CONST_COSINE( CONST_DEG2RAD( (a) ) )
#define CONST_COSINE( a )			( ( (a) < CONST_PI ) ?								\
										( ( (a) > CONST_PI * 0.5f ) ?					\
											- CONST_COSINE_POLY( CONST_PI - (a) )		\
											:											\
											CONST_COSINE_POLY( (a) ) )					\
										:												\
										( ( (a) > CONST_PI * 1.5f ) ?					\
											CONST_COSINE_POLY( (a) - CONST_PI * 2.0f )	\
											:											\
											- CONST_COSINE_POLY( CONST_PI - (a) ) ) )

#define CONST_DEG2RAD( a )			( (a) * CONST_PI / 180.0f )

#endif
