/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013 Robert Beckebans

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
#ifndef __SYS_INTRIINSICS_H__
#define __SYS_INTRIINSICS_H__

#if defined(USE_INTRINSICS)
#include <emmintrin.h>
#endif
/*
================================================================================================

	Scalar single precision floating-point intrinsics

================================================================================================
*/

ID_INLINE_EXTERN float __fmuls( float a, float b )
{
	return ( a * b );
}
ID_INLINE_EXTERN float __fmadds( float a, float b, float c )
{
	return ( a * b + c );
}
ID_INLINE_EXTERN float __fnmsubs( float a, float b, float c )
{
	return ( c - a * b );
}
ID_INLINE_EXTERN float __fsels( float a, float b, float c )
{
	return ( a >= 0.0f ) ? b : c;
}
ID_INLINE_EXTERN float __frcps( float x )
{
	return ( 1.0f / x );
}
ID_INLINE_EXTERN float __fdivs( float x, float y )
{
	return ( x / y );
}
ID_INLINE_EXTERN float __frsqrts( float x )
{
	return ( 1.0f / sqrtf( x ) );
}
ID_INLINE_EXTERN float __frcps16( float x )
{
	return ( 1.0f / x );
}
ID_INLINE_EXTERN float __fdivs16( float x, float y )
{
	return ( x / y );
}
ID_INLINE_EXTERN float __frsqrts16( float x )
{
	return ( 1.0f / sqrtf( x ) );
}
ID_INLINE_EXTERN float __frndz( float x )
{
	return ( float )( ( int )( x ) );
}

/*
================================================================================================

	Zero cache line and prefetch intrinsics

================================================================================================
*/

#if defined(USE_INTRINSICS)
// The code below assumes that a cache line is 64 bytes.
// We specify the cache line size as 128 here to make the code consistent with the consoles.
#define CACHE_LINE_SIZE						128

ID_FORCE_INLINE void Prefetch( const void* ptr, int offset )
{
//	const char * bytePtr = ( (const char *) ptr ) + offset;
//	_mm_prefetch( bytePtr +  0, _MM_HINT_NTA );
//	_mm_prefetch( bytePtr + 64, _MM_HINT_NTA );
}
ID_FORCE_INLINE void ZeroCacheLine( void* ptr, int offset )
{
	assert_128_byte_aligned( ptr );
	char* bytePtr = ( ( char* ) ptr ) + offset;
	__m128i zero = _mm_setzero_si128();
	_mm_store_si128( ( __m128i* )( bytePtr + 0 * 16 ), zero );
	_mm_store_si128( ( __m128i* )( bytePtr + 1 * 16 ), zero );
	_mm_store_si128( ( __m128i* )( bytePtr + 2 * 16 ), zero );
	_mm_store_si128( ( __m128i* )( bytePtr + 3 * 16 ), zero );
	_mm_store_si128( ( __m128i* )( bytePtr + 4 * 16 ), zero );
	_mm_store_si128( ( __m128i* )( bytePtr + 5 * 16 ), zero );
	_mm_store_si128( ( __m128i* )( bytePtr + 6 * 16 ), zero );
	_mm_store_si128( ( __m128i* )( bytePtr + 7 * 16 ), zero );
}
ID_FORCE_INLINE void FlushCacheLine( const void* ptr, int offset )
{
	const char* bytePtr = ( ( const char* ) ptr ) + offset;
	_mm_clflush( bytePtr +  0 );
	_mm_clflush( bytePtr + 64 );
}

/*
================================================
#endif
	Other
================================================
*/
#else

#define CACHE_LINE_SIZE						128

ID_INLINE void Prefetch( const void* ptr, int offset ) {}
ID_INLINE void ZeroCacheLine( void* ptr, int offset )
{
	byte* bytePtr = ( byte* )( ( ( ( uintptr_t )( ptr ) ) + ( offset ) ) & ~( CACHE_LINE_SIZE - 1 ) );
	memset( bytePtr, 0, CACHE_LINE_SIZE );
}
ID_INLINE void FlushCacheLine( const void* ptr, int offset ) {}

#endif

/*
================================================
	Block Clear Macros
================================================
*/

// number of additional elements that are potentially cleared when clearing whole cache lines at a time
ID_INLINE_EXTERN int CACHE_LINE_CLEAR_OVERFLOW_COUNT( int size )
{
	if( ( size & ( CACHE_LINE_SIZE - 1 ) ) == 0 )
	{
		return 0;
	}
	if( size > CACHE_LINE_SIZE )
	{
		return 1;
	}
	return ( CACHE_LINE_SIZE / ( size & ( CACHE_LINE_SIZE - 1 ) ) );
}

// if the pointer is not on a cache line boundary this assumes the cache line the pointer starts in was already cleared
// RB: changed UINT_PTR to uintptr_t
#define CACHE_LINE_CLEAR_BLOCK( ptr, size )																		\
	byte * startPtr = (byte *)( ( ( (uintptr_t) ( ptr ) ) + CACHE_LINE_SIZE - 1 ) & ~( CACHE_LINE_SIZE - 1 ) );	\
	byte * endPtr = (byte *)( ( (uintptr_t) ( ptr ) + ( size ) - 1 ) & ~( CACHE_LINE_SIZE - 1 ) );				\
	for ( ; startPtr <= endPtr; startPtr += CACHE_LINE_SIZE ) {													\
		ZeroCacheLine( startPtr, 0 );																			\
	}

#define CACHE_LINE_CLEAR_BLOCK_AND_FLUSH( ptr, size )															\
	byte * startPtr = (byte *)( ( ( (uintptr_t) ( ptr ) ) + CACHE_LINE_SIZE - 1 ) & ~( CACHE_LINE_SIZE - 1 ) );	\
	byte * endPtr = (byte *)( ( (uintptr_t) ( ptr ) + ( size ) - 1 ) & ~( CACHE_LINE_SIZE - 1 ) );				\
	for ( ; startPtr <= endPtr; startPtr += CACHE_LINE_SIZE ) {													\
		ZeroCacheLine( startPtr, 0 );																			\
		FlushCacheLine( startPtr, 0 );																			\
	}
// RB end

/*
================================================================================================

	Vector Intrinsics

================================================================================================
*/

#if defined(USE_INTRINSICS)

/*
================================================
	PC Windows
================================================
*/

#if !defined( R_SHUFFLE_D )
#define R_SHUFFLE_D( x, y, z, w )	(( (w) & 3 ) << 6 | ( (z) & 3 ) << 4 | ( (y) & 3 ) << 2 | ( (x) & 3 ))
#endif

// DG: _CRT_ALIGN seems to be MSVC specific, so provide implementation..
#ifndef _CRT_ALIGN
#if defined(__GNUC__) // also applies for clang
#define _CRT_ALIGN(x) __attribute__ ((__aligned__ (x)))
#elif defined(_MSC_VER) // also for MSVC, just to be sure
#define _CRT_ALIGN(x) __declspec(align(x))
#endif
#endif
// DG: make sure __declspec(intrin_type) is only used on MSVC (it's not available on GCC etc
#ifdef _MSC_VER
#define DECLSPEC_INTRINTYPE __declspec( intrin_type )
#else
#define DECLSPEC_INTRINTYPE
#endif
// DG end


// make the intrinsics "type unsafe"
typedef union DECLSPEC_INTRINTYPE _CRT_ALIGN( 16 ) __m128c
{
	__m128c() {}
	__m128c( __m128 f )
	{
		m128 = f;
	}
	__m128c( __m128i i )
	{
		m128i = i;
	}
	operator	__m128()
	{
		return m128;
	}
	operator	__m128i()
	{
		return m128i;
	}
	__m128		m128;
	__m128i		m128i;
} __m128c;

#define _mm_madd_ps( a, b, c )				_mm_add_ps( _mm_mul_ps( (a), (b) ), (c) )
#define _mm_nmsub_ps( a, b, c )				_mm_sub_ps( (c), _mm_mul_ps( (a), (b) ) )
#define _mm_splat_ps( x, i )				__m128c( _mm_shuffle_epi32( __m128c( x ), _MM_SHUFFLE( i, i, i, i ) ) )
#define _mm_perm_ps( x, perm )				__m128c( _mm_shuffle_epi32( __m128c( x ), perm ) )
#define _mm_sel_ps( a, b, c )  				_mm_or_ps( _mm_andnot_ps( __m128c( c ), a ), _mm_and_ps( __m128c( c ), b ) )
#define _mm_sel_si128( a, b, c )			_mm_or_si128( _mm_andnot_si128( __m128c( c ), a ), _mm_and_si128( __m128c( c ), b ) )
#define _mm_sld_ps( x, y, imm )				__m128c( _mm_or_si128( _mm_srli_si128( __m128c( x ), imm ), _mm_slli_si128( __m128c( y ), 16 - imm ) ) )
#define _mm_sld_si128( x, y, imm )			_mm_or_si128( _mm_srli_si128( x, imm ), _mm_slli_si128( y, 16 - imm ) )

ID_FORCE_INLINE_EXTERN __m128 _mm_msum3_ps( __m128 a, __m128 b )
{
	__m128 c = _mm_mul_ps( a, b );
	return _mm_add_ps( _mm_splat_ps( c, 0 ), _mm_add_ps( _mm_splat_ps( c, 1 ), _mm_splat_ps( c, 2 ) ) );
}

ID_FORCE_INLINE_EXTERN __m128 _mm_msum4_ps( __m128 a, __m128 b )
{
	__m128 c = _mm_mul_ps( a, b );
	c = _mm_add_ps( c, _mm_perm_ps( c, _MM_SHUFFLE( 1, 0, 3, 2 ) ) );
	c = _mm_add_ps( c, _mm_perm_ps( c, _MM_SHUFFLE( 2, 3, 0, 1 ) ) );
	return c;
}

#define _mm_shufmix_epi32( x, y, perm )		__m128c( _mm_shuffle_ps( __m128c( x ), __m128c( y ), perm ) )
#define _mm_loadh_epi64( x, address )		__m128c( _mm_loadh_pi( __m128c( x ), (__m64 *)address ) )
#define _mm_storeh_epi64( address, x )		_mm_storeh_pi( (__m64 *)address, __m128c( x ) )

// floating-point reciprocal with close to full precision
ID_FORCE_INLINE_EXTERN __m128 _mm_rcp32_ps( __m128 x )
{
	__m128 r = _mm_rcp_ps( x );		// _mm_rcp_ps() has 12 bits of precision
	r = _mm_sub_ps( _mm_add_ps( r, r ), _mm_mul_ps( _mm_mul_ps( x, r ), r ) );
	r = _mm_sub_ps( _mm_add_ps( r, r ), _mm_mul_ps( _mm_mul_ps( x, r ), r ) );
	return r;
}
// floating-point reciprocal with at least 16 bits precision
ID_FORCE_INLINE_EXTERN __m128 _mm_rcp16_ps( __m128 x )
{
	__m128 r = _mm_rcp_ps( x );		// _mm_rcp_ps() has 12 bits of precision
	r = _mm_sub_ps( _mm_add_ps( r, r ), _mm_mul_ps( _mm_mul_ps( x, r ), r ) );
	return r;
}
// floating-point divide with close to full precision
ID_FORCE_INLINE_EXTERN __m128 _mm_div32_ps( __m128 x, __m128 y )
{
	return _mm_mul_ps( x, _mm_rcp32_ps( y ) );
}
// floating-point divide with at least 16 bits precision
ID_FORCE_INLINE_EXTERN __m128 _mm_div16_ps( __m128 x, __m128 y )
{
	return _mm_mul_ps( x, _mm_rcp16_ps( y ) );
}
// load idBounds::GetMins()
#define _mm_loadu_bounds_0( bounds )		_mm_perm_ps( _mm_loadh_pi( _mm_load_ss( & bounds[0].x ), (__m64 *) & bounds[0].y ), _MM_SHUFFLE( 1, 3, 2, 0 ) )
// load idBounds::GetMaxs()
#define _mm_loadu_bounds_1( bounds )		_mm_perm_ps( _mm_loadh_pi( _mm_load_ss( & bounds[1].x ), (__m64 *) & bounds[1].y ), _MM_SHUFFLE( 1, 3, 2, 0 ) )

#endif // #if defined(USE_INTRINSICS)

#endif	// !__SYS_INTRIINSICS_H__
