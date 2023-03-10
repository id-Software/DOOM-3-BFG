/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2014 Robert Beckebans

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

#include "RenderCommon.h"
#include "Model_local.h"

#include "../idlib/geometry/DrawVert_intrinsics.h"

/*
====================
R_TracePointCullStatic
====================
*/
static void R_TracePointCullStatic( byte* cullBits, byte& totalOr, const float radius, const idPlane* planes, const idDrawVert* verts, const int numVerts )
{
	assert_16_byte_aligned( cullBits );
	assert_16_byte_aligned( verts );

#if defined(USE_INTRINSICS_SSE)
	idODSStreamedArray< idDrawVert, 16, SBT_DOUBLE, 4 > vertsODS( verts, numVerts );

	const __m128 vector_float_radius	= _mm_splat_ps( _mm_load_ss( &radius ), 0 );
	const __m128 vector_float_zero		= { 0.0f, 0.0f, 0.0f, 0.0f };
	const __m128i vector_int_mask0		= _mm_set1_epi32( 1 << 0 );
	const __m128i vector_int_mask1		= _mm_set1_epi32( 1 << 1 );
	const __m128i vector_int_mask2		= _mm_set1_epi32( 1 << 2 );
	const __m128i vector_int_mask3		= _mm_set1_epi32( 1 << 3 );
	const __m128i vector_int_mask4		= _mm_set1_epi32( 1 << 4 );
	const __m128i vector_int_mask5		= _mm_set1_epi32( 1 << 5 );
	const __m128i vector_int_mask6		= _mm_set1_epi32( 1 << 6 );
	const __m128i vector_int_mask7		= _mm_set1_epi32( 1 << 7 );

	const __m128 p0 = _mm_loadu_ps( planes[0].ToFloatPtr() );
	const __m128 p1 = _mm_loadu_ps( planes[1].ToFloatPtr() );
	const __m128 p2 = _mm_loadu_ps( planes[2].ToFloatPtr() );
	const __m128 p3 = _mm_loadu_ps( planes[3].ToFloatPtr() );

	const __m128 p0X = _mm_splat_ps( p0, 0 );
	const __m128 p0Y = _mm_splat_ps( p0, 1 );
	const __m128 p0Z = _mm_splat_ps( p0, 2 );
	const __m128 p0W = _mm_splat_ps( p0, 3 );

	const __m128 p1X = _mm_splat_ps( p1, 0 );
	const __m128 p1Y = _mm_splat_ps( p1, 1 );
	const __m128 p1Z = _mm_splat_ps( p1, 2 );
	const __m128 p1W = _mm_splat_ps( p1, 3 );

	const __m128 p2X = _mm_splat_ps( p2, 0 );
	const __m128 p2Y = _mm_splat_ps( p2, 1 );
	const __m128 p2Z = _mm_splat_ps( p2, 2 );
	const __m128 p2W = _mm_splat_ps( p2, 3 );

	const __m128 p3X = _mm_splat_ps( p3, 0 );
	const __m128 p3Y = _mm_splat_ps( p3, 1 );
	const __m128 p3Z = _mm_splat_ps( p3, 2 );
	const __m128 p3W = _mm_splat_ps( p3, 3 );

	__m128i vecTotalOrInt = _mm_set_epi32( 0, 0, 0, 0 );

	for( int i = 0; i < numVerts; )
	{

		const int nextNumVerts = vertsODS.FetchNextBatch() - 4;

		for( ; i <= nextNumVerts; i += 4 )
		{
			const __m128 v0 = _mm_load_ps( vertsODS[i + 0].xyz.ToFloatPtr() );
			const __m128 v1 = _mm_load_ps( vertsODS[i + 1].xyz.ToFloatPtr() );
			const __m128 v2 = _mm_load_ps( vertsODS[i + 2].xyz.ToFloatPtr() );
			const __m128 v3 = _mm_load_ps( vertsODS[i + 3].xyz.ToFloatPtr() );

			const __m128 r0 = _mm_unpacklo_ps( v0, v2 );	// v0.x, v2.x, v0.z, v2.z
			const __m128 r1 = _mm_unpackhi_ps( v0, v2 );	// v0.y, v2.y, v0.w, v2.w
			const __m128 r2 = _mm_unpacklo_ps( v1, v3 );	// v1.x, v3.x, v1.z, v3.z
			const __m128 r3 = _mm_unpackhi_ps( v1, v3 );	// v1.y, v3.y, v1.w, v3.w

			const __m128 vX = _mm_unpacklo_ps( r0, r2 );	// v0.x, v1.x, v2.x, v3.x
			const __m128 vY = _mm_unpackhi_ps( r0, r2 );	// v0.y, v1.y, v2.y, v3.y
			const __m128 vZ = _mm_unpacklo_ps( r1, r3 );	// v0.z, v1.z, v2.z, v3.z

			const __m128 d0 = _mm_madd_ps( vX, p0X, _mm_madd_ps( vY, p0Y, _mm_madd_ps( vZ, p0Z, p0W ) ) );
			const __m128 d1 = _mm_madd_ps( vX, p1X, _mm_madd_ps( vY, p1Y, _mm_madd_ps( vZ, p1Z, p1W ) ) );
			const __m128 d2 = _mm_madd_ps( vX, p2X, _mm_madd_ps( vY, p2Y, _mm_madd_ps( vZ, p2Z, p2W ) ) );
			const __m128 d3 = _mm_madd_ps( vX, p3X, _mm_madd_ps( vY, p3Y, _mm_madd_ps( vZ, p3Z, p3W ) ) );

			const __m128 t0 = _mm_add_ps( d0, vector_float_radius );
			const __m128 t1 = _mm_add_ps( d1, vector_float_radius );
			const __m128 t2 = _mm_add_ps( d2, vector_float_radius );
			const __m128 t3 = _mm_add_ps( d3, vector_float_radius );

			const __m128 t4 = _mm_sub_ps( d0, vector_float_radius );
			const __m128 t5 = _mm_sub_ps( d1, vector_float_radius );
			const __m128 t6 = _mm_sub_ps( d2, vector_float_radius );
			const __m128 t7 = _mm_sub_ps( d3, vector_float_radius );

			__m128i c0 = __m128c( _mm_cmpgt_ps( t0, vector_float_zero ) );
			__m128i c1 = __m128c( _mm_cmpgt_ps( t1, vector_float_zero ) );
			__m128i c2 = __m128c( _mm_cmpgt_ps( t2, vector_float_zero ) );
			__m128i c3 = __m128c( _mm_cmpgt_ps( t3, vector_float_zero ) );

			__m128i c4 = __m128c( _mm_cmplt_ps( t4, vector_float_zero ) );
			__m128i c5 = __m128c( _mm_cmplt_ps( t5, vector_float_zero ) );
			__m128i c6 = __m128c( _mm_cmplt_ps( t6, vector_float_zero ) );
			__m128i c7 = __m128c( _mm_cmplt_ps( t7, vector_float_zero ) );

			c0 = _mm_and_si128( c0, vector_int_mask0 );
			c1 = _mm_and_si128( c1, vector_int_mask1 );
			c2 = _mm_and_si128( c2, vector_int_mask2 );
			c3 = _mm_and_si128( c3, vector_int_mask3 );

			c4 = _mm_and_si128( c4, vector_int_mask4 );
			c5 = _mm_and_si128( c5, vector_int_mask5 );
			c6 = _mm_and_si128( c6, vector_int_mask6 );
			c7 = _mm_and_si128( c7, vector_int_mask7 );

			c0 = _mm_or_si128( c0, c1 );
			c2 = _mm_or_si128( c2, c3 );
			c4 = _mm_or_si128( c4, c5 );
			c6 = _mm_or_si128( c6, c7 );

			c0 = _mm_or_si128( c0, c2 );
			c4 = _mm_or_si128( c4, c6 );
			c0 = _mm_or_si128( c0, c4 );

			vecTotalOrInt = _mm_or_si128( vecTotalOrInt, c0 );

			__m128i s0 = _mm_packs_epi32( c0, c0 );
			__m128i b0 = _mm_packus_epi16( s0, s0 );

			*( unsigned int* )&cullBits[i] = _mm_cvtsi128_si32( b0 );
		}
	}

	vecTotalOrInt = _mm_or_si128( vecTotalOrInt, _mm_shuffle_epi32( vecTotalOrInt, _MM_SHUFFLE( 1, 0, 3, 2 ) ) );
	vecTotalOrInt = _mm_or_si128( vecTotalOrInt, _mm_shuffle_epi32( vecTotalOrInt, _MM_SHUFFLE( 2, 3, 0, 1 ) ) );

	__m128i vecTotalOrShort = _mm_packs_epi32( vecTotalOrInt, vecTotalOrInt );
	__m128i vecTotalOrByte = _mm_packus_epi16( vecTotalOrShort, vecTotalOrShort );

	totalOr = ( byte ) _mm_cvtsi128_si32( vecTotalOrByte );

#else

	idODSStreamedArray< idDrawVert, 16, SBT_DOUBLE, 1 > vertsODS( verts, numVerts );

	byte tOr = 0;
	for( int i = 0; i < numVerts; )
	{

		const int nextNumVerts = vertsODS.FetchNextBatch() - 1;

		for( ; i <= nextNumVerts; i++ )
		{
			const idVec3& v = vertsODS[i].xyz;

			const float d0 = planes[0].Distance( v );
			const float d1 = planes[1].Distance( v );
			const float d2 = planes[2].Distance( v );
			const float d3 = planes[3].Distance( v );

			const float t0 = d0 + radius;
			const float t1 = d1 + radius;
			const float t2 = d2 + radius;
			const float t3 = d3 + radius;

			const float s0 = d0 - radius;
			const float s1 = d1 - radius;
			const float s2 = d2 - radius;
			const float s3 = d3 - radius;

			byte bits;
			bits  = IEEE_FLT_SIGNBITSET( t0 ) << 0;
			bits |= IEEE_FLT_SIGNBITSET( t1 ) << 1;
			bits |= IEEE_FLT_SIGNBITSET( t2 ) << 2;
			bits |= IEEE_FLT_SIGNBITSET( t3 ) << 3;

			bits |= IEEE_FLT_SIGNBITSET( s0 ) << 4;
			bits |= IEEE_FLT_SIGNBITSET( s1 ) << 5;
			bits |= IEEE_FLT_SIGNBITSET( s2 ) << 6;
			bits |= IEEE_FLT_SIGNBITSET( s3 ) << 7;

			bits ^= 0x0F;		// flip lower four bits

			tOr |= bits;
			cullBits[i] = bits;
		}
	}

	totalOr = tOr;

#endif
}

/*
====================
R_TracePointCullSkinned
====================
*/
static void R_TracePointCullSkinned( byte* cullBits, byte& totalOr, const float radius, const idPlane* planes, const idDrawVert* verts, const int numVerts, const idJointMat* joints )
{
	assert_16_byte_aligned( cullBits );
	assert_16_byte_aligned( verts );

#if defined(USE_INTRINSICS_SSE)
	idODSStreamedArray< idDrawVert, 16, SBT_DOUBLE, 4 > vertsODS( verts, numVerts );

	const __m128 vector_float_radius	= _mm_splat_ps( _mm_load_ss( &radius ), 0 );
	const __m128 vector_float_zero		= { 0.0f, 0.0f, 0.0f, 0.0f };
	const __m128i vector_int_mask0		= _mm_set1_epi32( 1 << 0 );
	const __m128i vector_int_mask1		= _mm_set1_epi32( 1 << 1 );
	const __m128i vector_int_mask2		= _mm_set1_epi32( 1 << 2 );
	const __m128i vector_int_mask3		= _mm_set1_epi32( 1 << 3 );
	const __m128i vector_int_mask4		= _mm_set1_epi32( 1 << 4 );
	const __m128i vector_int_mask5		= _mm_set1_epi32( 1 << 5 );
	const __m128i vector_int_mask6		= _mm_set1_epi32( 1 << 6 );
	const __m128i vector_int_mask7		= _mm_set1_epi32( 1 << 7 );

	const __m128 p0 = _mm_loadu_ps( planes[0].ToFloatPtr() );
	const __m128 p1 = _mm_loadu_ps( planes[1].ToFloatPtr() );
	const __m128 p2 = _mm_loadu_ps( planes[2].ToFloatPtr() );
	const __m128 p3 = _mm_loadu_ps( planes[3].ToFloatPtr() );

	const __m128 p0X = _mm_splat_ps( p0, 0 );
	const __m128 p0Y = _mm_splat_ps( p0, 1 );
	const __m128 p0Z = _mm_splat_ps( p0, 2 );
	const __m128 p0W = _mm_splat_ps( p0, 3 );

	const __m128 p1X = _mm_splat_ps( p1, 0 );
	const __m128 p1Y = _mm_splat_ps( p1, 1 );
	const __m128 p1Z = _mm_splat_ps( p1, 2 );
	const __m128 p1W = _mm_splat_ps( p1, 3 );

	const __m128 p2X = _mm_splat_ps( p2, 0 );
	const __m128 p2Y = _mm_splat_ps( p2, 1 );
	const __m128 p2Z = _mm_splat_ps( p2, 2 );
	const __m128 p2W = _mm_splat_ps( p2, 3 );

	const __m128 p3X = _mm_splat_ps( p3, 0 );
	const __m128 p3Y = _mm_splat_ps( p3, 1 );
	const __m128 p3Z = _mm_splat_ps( p3, 2 );
	const __m128 p3W = _mm_splat_ps( p3, 3 );

	__m128i vecTotalOrInt = _mm_set_epi32( 0, 0, 0, 0 );

	for( int i = 0; i < numVerts; )
	{

		const int nextNumVerts = vertsODS.FetchNextBatch() - 4;

		for( ; i <= nextNumVerts; i += 4 )
		{
			const __m128 v0 = LoadSkinnedDrawVertPosition( vertsODS[i + 0], joints );
			const __m128 v1 = LoadSkinnedDrawVertPosition( vertsODS[i + 1], joints );
			const __m128 v2 = LoadSkinnedDrawVertPosition( vertsODS[i + 2], joints );
			const __m128 v3 = LoadSkinnedDrawVertPosition( vertsODS[i + 3], joints );

			const __m128 r0 = _mm_unpacklo_ps( v0, v2 );	// v0.x, v2.x, v0.z, v2.z
			const __m128 r1 = _mm_unpackhi_ps( v0, v2 );	// v0.y, v2.y, v0.w, v2.w
			const __m128 r2 = _mm_unpacklo_ps( v1, v3 );	// v1.x, v3.x, v1.z, v3.z
			const __m128 r3 = _mm_unpackhi_ps( v1, v3 );	// v1.y, v3.y, v1.w, v3.w

			const __m128 vX = _mm_unpacklo_ps( r0, r2 );	// v0.x, v1.x, v2.x, v3.x
			const __m128 vY = _mm_unpackhi_ps( r0, r2 );	// v0.y, v1.y, v2.y, v3.y
			const __m128 vZ = _mm_unpacklo_ps( r1, r3 );	// v0.z, v1.z, v2.z, v3.z

			const __m128 d0 = _mm_madd_ps( vX, p0X, _mm_madd_ps( vY, p0Y, _mm_madd_ps( vZ, p0Z, p0W ) ) );
			const __m128 d1 = _mm_madd_ps( vX, p1X, _mm_madd_ps( vY, p1Y, _mm_madd_ps( vZ, p1Z, p1W ) ) );
			const __m128 d2 = _mm_madd_ps( vX, p2X, _mm_madd_ps( vY, p2Y, _mm_madd_ps( vZ, p2Z, p2W ) ) );
			const __m128 d3 = _mm_madd_ps( vX, p3X, _mm_madd_ps( vY, p3Y, _mm_madd_ps( vZ, p3Z, p3W ) ) );

			const __m128 t0 = _mm_add_ps( d0, vector_float_radius );
			const __m128 t1 = _mm_add_ps( d1, vector_float_radius );
			const __m128 t2 = _mm_add_ps( d2, vector_float_radius );
			const __m128 t3 = _mm_add_ps( d3, vector_float_radius );

			const __m128 t4 = _mm_sub_ps( d0, vector_float_radius );
			const __m128 t5 = _mm_sub_ps( d1, vector_float_radius );
			const __m128 t6 = _mm_sub_ps( d2, vector_float_radius );
			const __m128 t7 = _mm_sub_ps( d3, vector_float_radius );

			__m128i c0 = __m128c( _mm_cmpgt_ps( t0, vector_float_zero ) );
			__m128i c1 = __m128c( _mm_cmpgt_ps( t1, vector_float_zero ) );
			__m128i c2 = __m128c( _mm_cmpgt_ps( t2, vector_float_zero ) );
			__m128i c3 = __m128c( _mm_cmpgt_ps( t3, vector_float_zero ) );

			__m128i c4 = __m128c( _mm_cmplt_ps( t4, vector_float_zero ) );
			__m128i c5 = __m128c( _mm_cmplt_ps( t5, vector_float_zero ) );
			__m128i c6 = __m128c( _mm_cmplt_ps( t6, vector_float_zero ) );
			__m128i c7 = __m128c( _mm_cmplt_ps( t7, vector_float_zero ) );

			c0 = _mm_and_si128( c0, vector_int_mask0 );
			c1 = _mm_and_si128( c1, vector_int_mask1 );
			c2 = _mm_and_si128( c2, vector_int_mask2 );
			c3 = _mm_and_si128( c3, vector_int_mask3 );

			c4 = _mm_and_si128( c4, vector_int_mask4 );
			c5 = _mm_and_si128( c5, vector_int_mask5 );
			c6 = _mm_and_si128( c6, vector_int_mask6 );
			c7 = _mm_and_si128( c7, vector_int_mask7 );

			c0 = _mm_or_si128( c0, c1 );
			c2 = _mm_or_si128( c2, c3 );
			c4 = _mm_or_si128( c4, c5 );
			c6 = _mm_or_si128( c6, c7 );

			c0 = _mm_or_si128( c0, c2 );
			c4 = _mm_or_si128( c4, c6 );
			c0 = _mm_or_si128( c0, c4 );

			vecTotalOrInt = _mm_or_si128( vecTotalOrInt, c0 );

			__m128i s0 = _mm_packs_epi32( c0, c0 );
			__m128i b0 = _mm_packus_epi16( s0, s0 );

			*( unsigned int* )&cullBits[i] = _mm_cvtsi128_si32( b0 );
		}
	}

	vecTotalOrInt = _mm_or_si128( vecTotalOrInt, _mm_shuffle_epi32( vecTotalOrInt, _MM_SHUFFLE( 1, 0, 3, 2 ) ) );
	vecTotalOrInt = _mm_or_si128( vecTotalOrInt, _mm_shuffle_epi32( vecTotalOrInt, _MM_SHUFFLE( 2, 3, 0, 1 ) ) );

	__m128i vecTotalOrShort = _mm_packs_epi32( vecTotalOrInt, vecTotalOrInt );
	__m128i vecTotalOrByte = _mm_packus_epi16( vecTotalOrShort, vecTotalOrShort );

	totalOr = ( byte ) _mm_cvtsi128_si32( vecTotalOrByte );

#else

	idODSStreamedArray< idDrawVert, 16, SBT_DOUBLE, 1 > vertsODS( verts, numVerts );

	byte tOr = 0;
	for( int i = 0; i < numVerts; )
	{

		const int nextNumVerts = vertsODS.FetchNextBatch() - 1;

		for( ; i <= nextNumVerts; i++ )
		{
			const idVec3 v = Scalar_LoadSkinnedDrawVertPosition( vertsODS[i], joints );

			const float d0 = planes[0].Distance( v );
			const float d1 = planes[1].Distance( v );
			const float d2 = planes[2].Distance( v );
			const float d3 = planes[3].Distance( v );

			const float t0 = d0 + radius;
			const float t1 = d1 + radius;
			const float t2 = d2 + radius;
			const float t3 = d3 + radius;

			const float s0 = d0 - radius;
			const float s1 = d1 - radius;
			const float s2 = d2 - radius;
			const float s3 = d3 - radius;

			byte bits;
			bits  = IEEE_FLT_SIGNBITSET( t0 ) << 0;
			bits |= IEEE_FLT_SIGNBITSET( t1 ) << 1;
			bits |= IEEE_FLT_SIGNBITSET( t2 ) << 2;
			bits |= IEEE_FLT_SIGNBITSET( t3 ) << 3;

			bits |= IEEE_FLT_SIGNBITSET( s0 ) << 4;
			bits |= IEEE_FLT_SIGNBITSET( s1 ) << 5;
			bits |= IEEE_FLT_SIGNBITSET( s2 ) << 6;
			bits |= IEEE_FLT_SIGNBITSET( s3 ) << 7;

			bits ^= 0x0F;		// flip lower four bits

			tOr |= bits;
			cullBits[i] = bits;
		}
	}

	totalOr = tOr;

#endif
}

/*
====================
R_LineIntersectsTriangleExpandedWithCircle

The triangle is expanded in the plane with a circle of the given radius.
====================
*/
static bool R_LineIntersectsTriangleExpandedWithCircle( localTrace_t& hit, const idVec3& start, const idVec3& end, const float circleRadius, const idVec3& triVert0, const idVec3& triVert1, const idVec3& triVert2 )
{
	const idPlane plane( triVert0, triVert1, triVert2 );

	const float planeDistStart = plane.Distance( start );
	const float planeDistEnd = plane.Distance( end );

	if( planeDistStart < 0.0f )
	{
		return false;		// starts past the triangle
	}

	if( planeDistEnd > 0.0f )
	{
		return false;		// finishes in front of the triangle
	}

	const float planeDelta = planeDistStart - planeDistEnd;

	if( planeDelta < idMath::FLT_SMALLEST_NON_DENORMAL )
	{
		return false;		// coming at the triangle from behind or parallel
	}

	const float fraction = planeDistStart / planeDelta;

	if( fraction < 0.0f )
	{
		return false;		// shouldn't happen
	}

	if( fraction >= hit.fraction )
	{
		return false;		// have already hit something closer
	}

	// find the exact point of impact with the plane
	const idVec3 point = start + fraction * ( end - start );

	// see if the point is within the three edges
	// if radius > 0 the triangle is expanded with a circle in the triangle plane

	const float radiusSqr = circleRadius * circleRadius;

	const idVec3 dir0 = triVert0 - point;
	const idVec3 dir1 = triVert1 - point;

	const idVec3 cross0 = dir0.Cross( dir1 );
	float d0 = plane.Normal() * cross0;
	if( d0 > 0.0f )
	{
		if( radiusSqr <= 0.0f )
		{
			return false;
		}
		idVec3 edge = triVert0 - triVert1;
		const float edgeLengthSqr = edge.LengthSqr();
		if( cross0.LengthSqr() > edgeLengthSqr * radiusSqr )
		{
			return false;
		}
		d0 = edge * dir0;
		if( d0 < 0.0f )
		{
			edge = triVert0 - triVert2;
			d0 = edge * dir0;
			if( d0 < 0.0f )
			{
				if( dir0.LengthSqr() > radiusSqr )
				{
					return false;
				}
			}
		}
		else if( d0 > edgeLengthSqr )
		{
			edge = triVert1 - triVert2;
			d0 = edge * dir1;
			if( d0 < 0.0f )
			{
				if( dir1.LengthSqr() > radiusSqr )
				{
					return false;
				}
			}
		}
	}

	const idVec3 dir2 = triVert2 - point;

	const idVec3 cross1 = dir1.Cross( dir2 );
	float d1 = plane.Normal() * cross1;
	if( d1 > 0.0f )
	{
		if( radiusSqr <= 0.0f )
		{
			return false;
		}
		idVec3 edge = triVert1 - triVert2;
		const float edgeLengthSqr = edge.LengthSqr();
		if( cross1.LengthSqr() > edgeLengthSqr * radiusSqr )
		{
			return false;
		}
		d1 = edge * dir1;
		if( d1 < 0.0f )
		{
			edge = triVert1 - triVert0;
			d1 = edge * dir1;
			if( d1 < 0.0f )
			{
				if( dir1.LengthSqr() > radiusSqr )
				{
					return false;
				}
			}
		}
		else if( d1 > edgeLengthSqr )
		{
			edge = triVert2 - triVert0;
			d1 = edge * dir2;
			if( d1 < 0.0f )
			{
				if( dir2.LengthSqr() > radiusSqr )
				{
					return false;
				}
			}
		}
	}

	const idVec3 cross2 = dir2.Cross( dir0 );
	float d2 = plane.Normal() * cross2;
	if( d2 > 0.0f )
	{
		if( radiusSqr <= 0.0f )
		{
			return false;
		}
		idVec3 edge = triVert2 - triVert0;
		const float edgeLengthSqr = edge.LengthSqr();
		if( cross2.LengthSqr() > edgeLengthSqr * radiusSqr )
		{
			return false;
		}
		d2 = edge * dir2;
		if( d2 < 0.0f )
		{
			edge = triVert2 - triVert1;
			d2 = edge * dir2;
			if( d2 < 0.0f )
			{
				if( dir2.LengthSqr() > radiusSqr )
				{
					return false;
				}
			}
		}
		else if( d2 > edgeLengthSqr )
		{
			edge = triVert0 - triVert1;
			d2 = edge * dir0;
			if( d2 < 0.0f )
			{
				if( dir0.LengthSqr() > radiusSqr )
				{
					return false;
				}
			}
		}
	}

	// we hit this triangle
	hit.fraction = fraction;
	hit.normal = plane.Normal();
	hit.point = point;

	return true;
}

/*
====================
R_LocalTrace
====================
*/
localTrace_t R_LocalTrace( const idVec3& start, const idVec3& end, const float radius, const srfTriangles_t* tri )
{
	localTrace_t hit;
	hit.fraction = 1.0f;

	ALIGNTYPE16 idPlane planes[4];
	// create two planes orthogonal to each other that intersect along the trace
	idVec3 startDir = end - start;
	startDir.Normalize();
	startDir.NormalVectors( planes[0].Normal(), planes[1].Normal() );
	planes[0][3] = - start * planes[0].Normal();
	planes[1][3] = - start * planes[1].Normal();
	// create front and end planes so the trace is on the positive sides of both
	planes[2] = startDir;
	planes[2][3] = - start * planes[2].Normal();
	planes[3] = -startDir;
	planes[3][3] = - end * planes[3].Normal();

	// catagorize each point against the four planes
	byte* cullBits = ( byte* ) _alloca16( ALIGN( tri->numVerts, 4 ) );	// round up to a multiple of 4 for SIMD
	byte totalOr = 0;

	// RB: added check wether GPU skinning is available at all
	const idJointMat* joints = ( tri->staticModelWithJoints != NULL && r_useGPUSkinning.GetBool() ) ? tri->staticModelWithJoints->jointsInverted : NULL;
	// RB end

	if( joints != NULL )
	{
		R_TracePointCullSkinned( cullBits, totalOr, radius, planes, tri->verts, tri->numVerts, joints );
	}
	else
	{
		R_TracePointCullStatic( cullBits, totalOr, radius, planes, tri->verts, tri->numVerts );
	}

	// if we don't have points on both sides of both the ray planes, no intersection
	if( ( totalOr ^ ( totalOr >> 4 ) ) & 3 )
	{
		return hit;
	}

	// if we don't have any points between front and end, no intersection
	if( ( totalOr ^ ( totalOr >> 1 ) ) & 4 )
	{
		return hit;
	}

	// start streaming the indexes
	idODSStreamedArray< triIndex_t, 256, SBT_QUAD, 3 > indexesODS( tri->indexes, tri->numIndexes );

	for( int i = 0; i < tri->numIndexes; )
	{

		const int nextNumIndexes = indexesODS.FetchNextBatch() - 3;

		for( ; i <= nextNumIndexes; i += 3 )
		{
			const int i0 = indexesODS[i + 0];
			const int i1 = indexesODS[i + 1];
			const int i2 = indexesODS[i + 2];

			// get sidedness info for the triangle
			const byte triOr = cullBits[i0] | cullBits[i1] | cullBits[i2];

			// if we don't have points on both sides of both the ray planes, no intersection
			if( likely( ( triOr ^ ( triOr >> 4 ) ) & 3 ) )
			{
				continue;
			}

			// if we don't have any points between front and end, no intersection
			if( unlikely( ( triOr ^ ( triOr >> 1 ) ) & 4 ) )
			{
				continue;
			}

			const idVec3 triVert0 = idDrawVert::GetSkinnedDrawVertPosition( idODSObject< idDrawVert > ( & tri->verts[i0] ), joints );
			const idVec3 triVert1 = idDrawVert::GetSkinnedDrawVertPosition( idODSObject< idDrawVert > ( & tri->verts[i1] ), joints );
			const idVec3 triVert2 = idDrawVert::GetSkinnedDrawVertPosition( idODSObject< idDrawVert > ( & tri->verts[i2] ), joints );

			if( R_LineIntersectsTriangleExpandedWithCircle( hit, start, end, radius, triVert0, triVert1, triVert2 ) )
			{
				hit.indexes[0] = i0;
				hit.indexes[1] = i1;
				hit.indexes[2] = i2;
			}
		}
	}

	return hit;
}
