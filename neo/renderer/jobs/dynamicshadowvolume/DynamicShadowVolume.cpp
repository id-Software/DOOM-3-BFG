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

#include "DynamicShadowVolume_local.h"

#include "../../../idlib/sys/sys_intrinsics.h"
#include "../../../idlib/geometry/DrawVert_intrinsics.h"

#if defined(USE_INTRINSICS)
static const __m128i vector_int_neg_one		= _mm_set_epi32( -1, -1, -1, -1 );
#endif

/*
=====================
TriangleFacing_SSE2
=====================
*/
#if defined(USE_INTRINSICS)
static ID_FORCE_INLINE __m128i TriangleFacing_SSE2(	const __m128& vert0X, const __m128& vert0Y, const __m128& vert0Z,
		const __m128& vert1X, const __m128& vert1Y, const __m128& vert1Z,
		const __m128& vert2X, const __m128& vert2Y, const __m128& vert2Z,
		const __m128& lightOriginX, const __m128& lightOriginY, const __m128& lightOriginZ )
{
	const __m128 sX = _mm_sub_ps( vert1X, vert0X );
	const __m128 sY = _mm_sub_ps( vert1Y, vert0Y );
	const __m128 sZ = _mm_sub_ps( vert1Z, vert0Z );
	
	const __m128 tX = _mm_sub_ps( vert2X, vert0X );
	const __m128 tY = _mm_sub_ps( vert2Y, vert0Y );
	const __m128 tZ = _mm_sub_ps( vert2Z, vert0Z );
	
	const __m128 normalX = _mm_nmsub_ps( tZ, sY, _mm_mul_ps( tY, sZ ) );
	const __m128 normalY = _mm_nmsub_ps( tX, sZ, _mm_mul_ps( tZ, sX ) );
	const __m128 normalZ = _mm_nmsub_ps( tY, sX, _mm_mul_ps( tX, sY ) );
	const __m128 normalW = _mm_madd_ps( normalX, vert0X, _mm_madd_ps( normalY, vert0Y, _mm_mul_ps( normalZ, vert0Z ) ) );
	
	const __m128 delta = _mm_nmsub_ps( lightOriginX, normalX, _mm_nmsub_ps( lightOriginY, normalY, _mm_nmsub_ps( lightOriginZ, normalZ, normalW ) ) );
	return _mm_castps_si128( _mm_cmplt_ps( delta, _mm_setzero_ps() ) );
}
#endif

/*
=====================
TriangleCulled

The clip space of the 'lightProject' is assumed to be in the range [0, 1].
=====================
*/
#if defined(USE_INTRINSICS)
static ID_FORCE_INLINE __m128i TriangleCulled_SSE2(	const __m128& vert0X, const __m128& vert0Y, const __m128& vert0Z,
		const __m128& vert1X, const __m128& vert1Y, const __m128& vert1Z,
		const __m128& vert2X, const __m128& vert2Y, const __m128& vert2Z,
		const __m128& lightProjectX, const __m128& lightProjectY, const __m128& lightProjectZ, const __m128& lightProjectW )
{

	const __m128 mvpX0 = _mm_splat_ps( lightProjectX, 0 );
	const __m128 mvpX1 = _mm_splat_ps( lightProjectX, 1 );
	const __m128 mvpX2 = _mm_splat_ps( lightProjectX, 2 );
	const __m128 mvpX3 = _mm_splat_ps( lightProjectX, 3 );
	
	const __m128 c0X = _mm_madd_ps( vert0X, mvpX0, _mm_madd_ps( vert0Y, mvpX1, _mm_madd_ps( vert0Z, mvpX2, mvpX3 ) ) );
	const __m128 c1X = _mm_madd_ps( vert1X, mvpX0, _mm_madd_ps( vert1Y, mvpX1, _mm_madd_ps( vert1Z, mvpX2, mvpX3 ) ) );
	const __m128 c2X = _mm_madd_ps( vert2X, mvpX0, _mm_madd_ps( vert2Y, mvpX1, _mm_madd_ps( vert2Z, mvpX2, mvpX3 ) ) );
	
	const __m128 mvpY0 = _mm_splat_ps( lightProjectY, 0 );
	const __m128 mvpY1 = _mm_splat_ps( lightProjectY, 1 );
	const __m128 mvpY2 = _mm_splat_ps( lightProjectY, 2 );
	const __m128 mvpY3 = _mm_splat_ps( lightProjectY, 3 );
	
	const __m128 c0Y = _mm_madd_ps( vert0X, mvpY0, _mm_madd_ps( vert0Y, mvpY1, _mm_madd_ps( vert0Z, mvpY2, mvpY3 ) ) );
	const __m128 c1Y = _mm_madd_ps( vert1X, mvpY0, _mm_madd_ps( vert1Y, mvpY1, _mm_madd_ps( vert1Z, mvpY2, mvpY3 ) ) );
	const __m128 c2Y = _mm_madd_ps( vert2X, mvpY0, _mm_madd_ps( vert2Y, mvpY1, _mm_madd_ps( vert2Z, mvpY2, mvpY3 ) ) );
	
	const __m128 mvpZ0 = _mm_splat_ps( lightProjectZ, 0 );
	const __m128 mvpZ1 = _mm_splat_ps( lightProjectZ, 1 );
	const __m128 mvpZ2 = _mm_splat_ps( lightProjectZ, 2 );
	const __m128 mvpZ3 = _mm_splat_ps( lightProjectZ, 3 );
	
	const __m128 c0Z = _mm_madd_ps( vert0X, mvpZ0, _mm_madd_ps( vert0Y, mvpZ1, _mm_madd_ps( vert0Z, mvpZ2, mvpZ3 ) ) );
	const __m128 c1Z = _mm_madd_ps( vert1X, mvpZ0, _mm_madd_ps( vert1Y, mvpZ1, _mm_madd_ps( vert1Z, mvpZ2, mvpZ3 ) ) );
	const __m128 c2Z = _mm_madd_ps( vert2X, mvpZ0, _mm_madd_ps( vert2Y, mvpZ1, _mm_madd_ps( vert2Z, mvpZ2, mvpZ3 ) ) );
	
	const __m128 mvpW0 = _mm_splat_ps( lightProjectW, 0 );
	const __m128 mvpW1 = _mm_splat_ps( lightProjectW, 1 );
	const __m128 mvpW2 = _mm_splat_ps( lightProjectW, 2 );
	const __m128 mvpW3 = _mm_splat_ps( lightProjectW, 3 );
	
	const __m128 c0W = _mm_madd_ps( vert0X, mvpW0, _mm_madd_ps( vert0Y, mvpW1, _mm_madd_ps( vert0Z, mvpW2, mvpW3 ) ) );
	const __m128 c1W = _mm_madd_ps( vert1X, mvpW0, _mm_madd_ps( vert1Y, mvpW1, _mm_madd_ps( vert1Z, mvpW2, mvpW3 ) ) );
	const __m128 c2W = _mm_madd_ps( vert2X, mvpW0, _mm_madd_ps( vert2Y, mvpW1, _mm_madd_ps( vert2Z, mvpW2, mvpW3 ) ) );
	
	const __m128 zero = _mm_setzero_ps();
	
	__m128 b0 = _mm_or_ps( _mm_or_ps( _mm_cmpgt_ps( c0X, zero ), _mm_cmpgt_ps( c1X, zero ) ), _mm_cmpgt_ps( c2X, zero ) );
	__m128 b1 = _mm_or_ps( _mm_or_ps( _mm_cmpgt_ps( c0Y, zero ), _mm_cmpgt_ps( c1Y, zero ) ), _mm_cmpgt_ps( c2Y, zero ) );
	__m128 b2 = _mm_or_ps( _mm_or_ps( _mm_cmpgt_ps( c0Z, zero ), _mm_cmpgt_ps( c1Z, zero ) ), _mm_cmpgt_ps( c2Z, zero ) );
	__m128 b3 = _mm_or_ps( _mm_or_ps( _mm_cmpgt_ps( c0W, c0X ), _mm_cmpgt_ps( c1W, c1X ) ), _mm_cmpgt_ps( c2W, c2X ) );
	__m128 b4 = _mm_or_ps( _mm_or_ps( _mm_cmpgt_ps( c0W, c0Y ), _mm_cmpgt_ps( c1W, c1Y ) ), _mm_cmpgt_ps( c2W, c2Y ) );
	__m128 b5 = _mm_or_ps( _mm_or_ps( _mm_cmpgt_ps( c0W, c0Z ), _mm_cmpgt_ps( c1W, c1Z ) ), _mm_cmpgt_ps( c2W, c2Z ) );
	
	b0 = _mm_and_ps( b0, b1 );
	b2 = _mm_and_ps( b2, b3 );
	b4 = _mm_and_ps( b4, b5 );
	b0 = _mm_and_ps( b0, b2 );
	b0 = _mm_and_ps( b0, b4 );
	
	return _mm_castps_si128( _mm_cmpeq_ps( b0, zero ) );
}

#else

/*
=====================
TriangleFacing

Returns 255 if the triangle is facing the light origin, otherwise returns 0.
=====================
*/
static byte TriangleFacing_Generic( const idVec3& v1, const idVec3& v2, const idVec3& v3, const idVec3& lightOrigin )
{
	const float sx = v2.x - v1.x;
	const float sy = v2.y - v1.y;
	const float sz = v2.z - v1.z;
	
	const float tx = v3.x - v1.x;
	const float ty = v3.y - v1.y;
	const float tz = v3.z - v1.z;
	
	const float normalX = ty * sz - tz * sy;
	const float normalY = tz * sx - tx * sz;
	const float normalZ = tx * sy - ty * sx;
	const float normalW = normalX * v1.x + normalY * v1.y + normalZ * v1.z;
	
	const float d = lightOrigin.x * normalX + lightOrigin.y * normalY + lightOrigin.z * normalZ - normalW;
	return ( d > 0.0f ) ? 255 : 0;
}

/*
=====================
TriangleCulled

Returns 255 if the triangle is culled to the light projection matrix, otherwise returns 0.
The clip space of the 'lightProject' is assumed to be in the range [0, 1].
=====================
*/
static byte TriangleCulled_Generic( const idVec3& v1, const idVec3& v2, const idVec3& v3, const idRenderMatrix& lightProject )
{
	// transform the triangle
	idVec4 c[3];
	for( int i = 0; i < 4; i++ )
	{
		c[0][i] = v1[0] * lightProject[i][0] + v1[1] * lightProject[i][1] + v1[2] * lightProject[i][2] + lightProject[i][3];
		c[1][i] = v2[0] * lightProject[i][0] + v2[1] * lightProject[i][1] + v2[2] * lightProject[i][2] + lightProject[i][3];
		c[2][i] = v3[0] * lightProject[i][0] + v3[1] * lightProject[i][1] + v3[2] * lightProject[i][2] + lightProject[i][3];
	}
	
	// calculate the culled bits
	int bits = 0;
	for( int i = 0; i < 3; i++ )
	{
		const float minW = 0.0f;
		const float maxW = c[i][3];
		
		if( c[i][0] > minW )
		{
			bits |= ( 1 << 0 );
		}
		if( c[i][0] < maxW )
		{
			bits |= ( 1 << 1 );
		}
		if( c[i][1] > minW )
		{
			bits |= ( 1 << 2 );
		}
		if( c[i][1] < maxW )
		{
			bits |= ( 1 << 3 );
		}
		if( c[i][2] > minW )
		{
			bits |= ( 1 << 4 );
		}
		if( c[i][2] < maxW )
		{
			bits |= ( 1 << 5 );
		}
	}
	
	// if any bits weren't set, the triangle is completely off one side of the frustum
	return ( bits != 63 ) ? 255 : 0;
}

#endif


/*
=====================
CalculateTriangleFacingCulledStatic
=====================
*/
static int CalculateTriangleFacingCulledStatic( byte* __restrict facing, byte* __restrict culled, const triIndex_t* __restrict indexes, int numIndexes,
		const idDrawVert* __restrict verts, const int numVerts,
		const idVec3& lightOrigin, const idVec3& viewOrigin,
		bool cullShadowTrianglesToLight, const idRenderMatrix& lightProject,
		bool* insideShadowVolume, const float radius )
{

	assert_spu_local_store( facing );
	assert_not_spu_local_store( indexes );
	assert_not_spu_local_store( verts );
	
	if( insideShadowVolume != NULL )
	{
		*insideShadowVolume = false;
	}
	
	// calculate the start, end, dir and length of the line from the view origin to the light origin
	const idVec3 lineStart = viewOrigin;
	const idVec3 lineEnd = lightOrigin;
	const idVec3 lineDelta = lineEnd - lineStart;
	const float lineLengthSqr = lineDelta.LengthSqr();
	const float lineLengthRcp = idMath::InvSqrt( lineLengthSqr );
	const idVec3 lineDir = lineDelta * lineLengthRcp;
	const float lineLength = lineLengthSqr * lineLengthRcp;
	
#if defined(USE_INTRINSICS)
	
	idODSStreamedIndexedArray< idDrawVert, triIndex_t, 32, SBT_QUAD, 4 * 3 > indexedVertsODS( verts, numVerts, indexes, numIndexes );
	
	const __m128 lightOriginX = _mm_splat_ps( _mm_load_ss( &lightOrigin.x ), 0 );
	const __m128 lightOriginY = _mm_splat_ps( _mm_load_ss( &lightOrigin.y ), 0 );
	const __m128 lightOriginZ = _mm_splat_ps( _mm_load_ss( &lightOrigin.z ), 0 );
	
	const __m128 lightProjectX = _mm_loadu_ps( lightProject[0] );
	const __m128 lightProjectY = _mm_loadu_ps( lightProject[1] );
	const __m128 lightProjectZ = _mm_loadu_ps( lightProject[2] );
	const __m128 lightProjectW = _mm_loadu_ps( lightProject[3] );
	
	const __m128i cullShadowTrianglesToLightMask = cullShadowTrianglesToLight ? vector_int_neg_one : vector_int_zero;
	
	__m128i numFrontFacing = _mm_setzero_si128();
	
	for( int i = 0, j = 0; i < numIndexes; )
	{
	
		const int batchStart = i;
		const int batchEnd = indexedVertsODS.FetchNextBatch();
		const int batchEnd4x = batchEnd - 4 * 3;
		const int indexStart = j;
		
		for( ; i <= batchEnd4x; i += 4 * 3, j += 4 )
		{
			const __m128 vertA0 = _mm_load_ps( indexedVertsODS[i + 0 * 3 + 0].xyz.ToFloatPtr() );
			const __m128 vertA1 = _mm_load_ps( indexedVertsODS[i + 0 * 3 + 1].xyz.ToFloatPtr() );
			const __m128 vertA2 = _mm_load_ps( indexedVertsODS[i + 0 * 3 + 2].xyz.ToFloatPtr() );
			
			const __m128 vertB0 = _mm_load_ps( indexedVertsODS[i + 1 * 3 + 0].xyz.ToFloatPtr() );
			const __m128 vertB1 = _mm_load_ps( indexedVertsODS[i + 1 * 3 + 1].xyz.ToFloatPtr() );
			const __m128 vertB2 = _mm_load_ps( indexedVertsODS[i + 1 * 3 + 2].xyz.ToFloatPtr() );
			
			const __m128 vertC0 = _mm_load_ps( indexedVertsODS[i + 2 * 3 + 0].xyz.ToFloatPtr() );
			const __m128 vertC1 = _mm_load_ps( indexedVertsODS[i + 2 * 3 + 1].xyz.ToFloatPtr() );
			const __m128 vertC2 = _mm_load_ps( indexedVertsODS[i + 2 * 3 + 2].xyz.ToFloatPtr() );
			
			const __m128 vertD0 = _mm_load_ps( indexedVertsODS[i + 3 * 3 + 0].xyz.ToFloatPtr() );
			const __m128 vertD1 = _mm_load_ps( indexedVertsODS[i + 3 * 3 + 1].xyz.ToFloatPtr() );
			const __m128 vertD2 = _mm_load_ps( indexedVertsODS[i + 3 * 3 + 2].xyz.ToFloatPtr() );
			
			const __m128 r0X = _mm_unpacklo_ps( vertA0, vertC0 );	// vertA0.x, vertC0.x, vertA0.z, vertC0.z
			const __m128 r0Y = _mm_unpackhi_ps( vertA0, vertC0 );	// vertA0.y, vertC0.y, vertA0.w, vertC0.w
			const __m128 r0Z = _mm_unpacklo_ps( vertB0, vertD0 );	// vertB0.x, vertD0.x, vertB0.z, vertD0.z
			const __m128 r0W = _mm_unpackhi_ps( vertB0, vertD0 );	// vertB0.y, vertD0.y, vertB0.w, vertD0.w
			
			const __m128 vert0X = _mm_unpacklo_ps( r0X, r0Z );		// vertA0.x, vertB0.x, vertC0.x, vertD0.x
			const __m128 vert0Y = _mm_unpackhi_ps( r0X, r0Z );		// vertA0.y, vertB0.y, vertC0.y, vertD0.y
			const __m128 vert0Z = _mm_unpacklo_ps( r0Y, r0W );		// vertA0.z, vertB0.z, vertC0.z, vertD0.z
			
			const __m128 r1X = _mm_unpacklo_ps( vertA1, vertC1 );	// vertA1.x, vertC1.x, vertA1.z, vertC1.z
			const __m128 r1Y = _mm_unpackhi_ps( vertA1, vertC1 );	// vertA1.y, vertC1.y, vertA1.w, vertC1.w
			const __m128 r1Z = _mm_unpacklo_ps( vertB1, vertD1 );	// vertB1.x, vertD1.x, vertB1.z, vertD1.z
			const __m128 r1W = _mm_unpackhi_ps( vertB1, vertD1 );	// vertB1.y, vertD1.y, vertB1.w, vertD1.w
			
			const __m128 vert1X = _mm_unpacklo_ps( r1X, r1Z );		// vertA1.x, vertB1.x, vertC1.x, vertD1.x
			const __m128 vert1Y = _mm_unpackhi_ps( r1X, r1Z );		// vertA1.y, vertB1.y, vertC1.y, vertD1.y
			const __m128 vert1Z = _mm_unpacklo_ps( r1Y, r1W );		// vertA1.z, vertB1.z, vertC1.z, vertD1.z
			
			const __m128 r2X = _mm_unpacklo_ps( vertA2, vertC2 );	// vertA2.x, vertC2.x, vertA2.z, vertC2.z
			const __m128 r2Y = _mm_unpackhi_ps( vertA2, vertC2 );	// vertA2.y, vertC2.y, vertA2.w, vertC2.w
			const __m128 r2Z = _mm_unpacklo_ps( vertB2, vertD2 );	// vertB2.x, vertD2.x, vertB2.z, vertD2.z
			const __m128 r2W = _mm_unpackhi_ps( vertB2, vertD2 );	// vertB2.y, vertD2.y, vertB2.w, vertD2.w
			
			const __m128 vert2X = _mm_unpacklo_ps( r2X, r2Z );		// vertA2.x, vertB2.x, vertC2.x, vertD2.x
			const __m128 vert2Y = _mm_unpackhi_ps( r2X, r2Z );		// vertA2.y, vertB2.y, vertC2.y, vertD2.y
			const __m128 vert2Z = _mm_unpacklo_ps( r2Y, r2W );		// vertA2.z, vertB2.z, vertC2.z, vertD2.z
			
			const __m128i triangleCulled = TriangleCulled_SSE2( vert0X, vert0Y, vert0Z, vert1X, vert1Y, vert1Z, vert2X, vert2Y, vert2Z, lightProjectX, lightProjectY, lightProjectZ, lightProjectW );
			
			__m128i triangleFacing = TriangleFacing_SSE2( vert0X, vert0Y, vert0Z, vert1X, vert1Y, vert1Z, vert2X, vert2Y, vert2Z, lightOriginX, lightOriginY, lightOriginZ );
			
			// optionally make triangles that are outside the light frustum facing so they do not contribute to the shadow volume
			triangleFacing = _mm_or_si128( triangleFacing, _mm_and_si128( triangleCulled, cullShadowTrianglesToLightMask ) );
			
			// store culled
			const __m128i culled_s = _mm_packs_epi32( triangleCulled, triangleCulled );
			const __m128i culled_b = _mm_packs_epi16( culled_s, culled_s );
			*( int* )&culled[j] = _mm_cvtsi128_si32( culled_b );
			
			// store facing
			const __m128i facing_s = _mm_packs_epi32( triangleFacing, triangleFacing );
			const __m128i facing_b = _mm_packs_epi16( facing_s, facing_s );
			*( int* )&facing[j] = _mm_cvtsi128_si32( facing_b );
			
			// count the number of facing triangles
			numFrontFacing = _mm_add_epi32( numFrontFacing, _mm_and_si128( triangleFacing, vector_int_one ) );
		}
		
		if( insideShadowVolume != NULL )
		{
			for( int k = batchStart, n = indexStart; k <= batchEnd - 3; k += 3, n++ )
			{
				if( !facing[n] )
				{
					if( R_LineIntersectsTriangleExpandedWithSphere( lineStart, lineEnd, lineDir, lineLength, radius, indexedVertsODS[k + 2].xyz, indexedVertsODS[k + 1].xyz, indexedVertsODS[k + 0].xyz ) )
					{
						*insideShadowVolume = true;
						insideShadowVolume = NULL;
						break;
					}
				}
			}
		}
	}
	
	numFrontFacing = _mm_add_epi32( numFrontFacing, _mm_shuffle_epi32( numFrontFacing, _MM_SHUFFLE( 1, 0, 3, 2 ) ) );
	numFrontFacing = _mm_add_epi32( numFrontFacing, _mm_shuffle_epi32( numFrontFacing, _MM_SHUFFLE( 2, 3, 0, 1 ) ) );
	
	return _mm_cvtsi128_si32( numFrontFacing );
	
#else
	
	idODSStreamedIndexedArray< idDrawVert, triIndex_t, 32, SBT_QUAD, 1 > indexedVertsODS( verts, numVerts, indexes, numIndexes );
	
	const byte cullShadowTrianglesToLightMask = cullShadowTrianglesToLight ? 255 : 0;
	
	int numFrontFacing = 0;
	
	for( int i = 0, j = 0; i < numIndexes; )
	{
	
		const int batchStart = i;
		const int batchEnd = indexedVertsODS.FetchNextBatch();
		const int indexStart = j;
	
		for( ; i <= batchEnd - 3; i += 3, j++ )
		{
			const idVec3& v1 = indexedVertsODS[i + 0].xyz;
			const idVec3& v2 = indexedVertsODS[i + 1].xyz;
			const idVec3& v3 = indexedVertsODS[i + 2].xyz;
	
			const byte triangleCulled = TriangleCulled_Generic( v1, v2, v3, lightProject );
	
			byte triangleFacing = TriangleFacing_Generic( v1, v2, v3, lightOrigin );
	
			// optionally make triangles that are outside the light frustum facing so they do not contribute to the shadow volume
			triangleFacing |= ( triangleCulled & cullShadowTrianglesToLightMask );
	
			culled[j] = triangleCulled;
			facing[j] = triangleFacing;
	
			// count the number of facing triangles
			numFrontFacing += ( triangleFacing & 1 );
		}
	
		if( insideShadowVolume != NULL )
		{
			for( int k = batchStart, n = indexStart; k <= batchEnd - 3; k += 3, n++ )
			{
				if( !facing[n] )
				{
					if( R_LineIntersectsTriangleExpandedWithSphere( lineStart, lineEnd, lineDir, lineLength, radius, indexedVertsODS[k + 2].xyz, indexedVertsODS[k + 1].xyz, indexedVertsODS[k + 0].xyz ) )
					{
						*insideShadowVolume = true;
						insideShadowVolume = NULL;
						break;
					}
				}
			}
		}
	}
	
	return numFrontFacing;
	
#endif
}

/*
=====================
CalculateTriangleFacingCulledSkinned
=====================
*/
static int CalculateTriangleFacingCulledSkinned( byte* __restrict facing, byte* __restrict culled, idVec4* __restrict tempVerts, const triIndex_t* __restrict indexes, int numIndexes,
		const idDrawVert* __restrict verts, const int numVerts, const idJointMat* __restrict joints,
		const idVec3& lightOrigin, const idVec3& viewOrigin,
		bool cullShadowTrianglesToLight, const idRenderMatrix& lightProject,
		bool* insideShadowVolume, const float radius )
{
	assert_spu_local_store( facing );
	assert_spu_local_store( joints );
	assert_not_spu_local_store( indexes );
	assert_not_spu_local_store( verts );
	
	if( insideShadowVolume != NULL )
	{
		*insideShadowVolume = false;
	}
	
	// calculate the start, end, dir and length of the line from the view origin to the light origin
	const idVec3 lineStart = viewOrigin;
	const idVec3 lineEnd = lightOrigin;
	const idVec3 lineDelta = lineEnd - lineStart;
	const float lineLengthSqr = lineDelta.LengthSqr();
	const float lineLengthRcp = idMath::InvSqrt( lineLengthSqr );
	const idVec3 lineDir = lineDelta * lineLengthRcp;
	const float lineLength = lineLengthSqr * lineLengthRcp;
	
#if defined(USE_INTRINSICS)
	
	idODSStreamedArray< idDrawVert, 32, SBT_DOUBLE, 1 > vertsODS( verts, numVerts );
	
	for( int i = 0; i < numVerts; )
	{
	
		const int nextNumVerts = vertsODS.FetchNextBatch() - 1;
		
		for( ; i <= nextNumVerts; i++ )
		{
			__m128 v = LoadSkinnedDrawVertPosition( vertsODS[i], joints );
			_mm_store_ps( tempVerts[i].ToFloatPtr(), v );
		}
	}
	
	idODSStreamedArray< triIndex_t, 256, SBT_QUAD, 4 * 3 > indexesODS( indexes, numIndexes );
	
	const __m128 lightOriginX = _mm_splat_ps( _mm_load_ss( &lightOrigin.x ), 0 );
	const __m128 lightOriginY = _mm_splat_ps( _mm_load_ss( &lightOrigin.y ), 0 );
	const __m128 lightOriginZ = _mm_splat_ps( _mm_load_ss( &lightOrigin.z ), 0 );
	
	const __m128 lightProjectX = _mm_loadu_ps( lightProject[0] );
	const __m128 lightProjectY = _mm_loadu_ps( lightProject[1] );
	const __m128 lightProjectZ = _mm_loadu_ps( lightProject[2] );
	const __m128 lightProjectW = _mm_loadu_ps( lightProject[3] );
	
	const __m128i cullShadowTrianglesToLightMask = cullShadowTrianglesToLight ? vector_int_neg_one : vector_int_zero;
	
	__m128i numFrontFacing = _mm_setzero_si128();
	
	for( int i = 0, j = 0; i < numIndexes; )
	{
	
		const int batchStart = i;
		const int batchEnd = indexesODS.FetchNextBatch();
		const int batchEnd4x = batchEnd - 4 * 3;
		const int indexStart = j;
		
		for( ; i <= batchEnd4x; i += 4 * 3, j += 4 )
		{
			const int indexA0 = indexesODS[( i + 0 * 3 + 0 )];
			const int indexA1 = indexesODS[( i + 0 * 3 + 1 )];
			const int indexA2 = indexesODS[( i + 0 * 3 + 2 )];
			
			const int indexB0 = indexesODS[( i + 1 * 3 + 0 )];
			const int indexB1 = indexesODS[( i + 1 * 3 + 1 )];
			const int indexB2 = indexesODS[( i + 1 * 3 + 2 )];
			
			const int indexC0 = indexesODS[( i + 2 * 3 + 0 )];
			const int indexC1 = indexesODS[( i + 2 * 3 + 1 )];
			const int indexC2 = indexesODS[( i + 2 * 3 + 2 )];
			
			const int indexD0 = indexesODS[( i + 3 * 3 + 0 )];
			const int indexD1 = indexesODS[( i + 3 * 3 + 1 )];
			const int indexD2 = indexesODS[( i + 3 * 3 + 2 )];
			
			const __m128 vertA0 = _mm_load_ps( tempVerts[indexA0].ToFloatPtr() );
			const __m128 vertA1 = _mm_load_ps( tempVerts[indexA1].ToFloatPtr() );
			const __m128 vertA2 = _mm_load_ps( tempVerts[indexA2].ToFloatPtr() );
			
			const __m128 vertB0 = _mm_load_ps( tempVerts[indexB0].ToFloatPtr() );
			const __m128 vertB1 = _mm_load_ps( tempVerts[indexB1].ToFloatPtr() );
			const __m128 vertB2 = _mm_load_ps( tempVerts[indexB2].ToFloatPtr() );
			
			const __m128 vertC0 = _mm_load_ps( tempVerts[indexC0].ToFloatPtr() );
			const __m128 vertC1 = _mm_load_ps( tempVerts[indexC1].ToFloatPtr() );
			const __m128 vertC2 = _mm_load_ps( tempVerts[indexC2].ToFloatPtr() );
			
			const __m128 vertD0 = _mm_load_ps( tempVerts[indexD0].ToFloatPtr() );
			const __m128 vertD1 = _mm_load_ps( tempVerts[indexD1].ToFloatPtr() );
			const __m128 vertD2 = _mm_load_ps( tempVerts[indexD2].ToFloatPtr() );
			
			const __m128 r0X = _mm_unpacklo_ps( vertA0, vertC0 );	// vertA0.x, vertC0.x, vertA0.z, vertC0.z
			const __m128 r0Y = _mm_unpackhi_ps( vertA0, vertC0 );	// vertA0.y, vertC0.y, vertA0.w, vertC0.w
			const __m128 r0Z = _mm_unpacklo_ps( vertB0, vertD0 );	// vertB0.x, vertD0.x, vertB0.z, vertD0.z
			const __m128 r0W = _mm_unpackhi_ps( vertB0, vertD0 );	// vertB0.y, vertD0.y, vertB0.w, vertD0.w
			
			const __m128 vert0X = _mm_unpacklo_ps( r0X, r0Z );		// vertA0.x, vertB0.x, vertC0.x, vertD0.x
			const __m128 vert0Y = _mm_unpackhi_ps( r0X, r0Z );		// vertA0.y, vertB0.y, vertC0.y, vertD0.y
			const __m128 vert0Z = _mm_unpacklo_ps( r0Y, r0W );		// vertA0.z, vertB0.z, vertC0.z, vertD0.z
			
			const __m128 r1X = _mm_unpacklo_ps( vertA1, vertC1 );	// vertA1.x, vertC1.x, vertA1.z, vertC1.z
			const __m128 r1Y = _mm_unpackhi_ps( vertA1, vertC1 );	// vertA1.y, vertC1.y, vertA1.w, vertC1.w
			const __m128 r1Z = _mm_unpacklo_ps( vertB1, vertD1 );	// vertB1.x, vertD1.x, vertB1.z, vertD1.z
			const __m128 r1W = _mm_unpackhi_ps( vertB1, vertD1 );	// vertB1.y, vertD1.y, vertB1.w, vertD1.w
			
			const __m128 vert1X = _mm_unpacklo_ps( r1X, r1Z );		// vertA1.x, vertB1.x, vertC1.x, vertD1.x
			const __m128 vert1Y = _mm_unpackhi_ps( r1X, r1Z );		// vertA1.y, vertB1.y, vertC1.y, vertD1.y
			const __m128 vert1Z = _mm_unpacklo_ps( r1Y, r1W );		// vertA1.z, vertB1.z, vertC1.z, vertD1.z
			
			const __m128 r2X = _mm_unpacklo_ps( vertA2, vertC2 );	// vertA2.x, vertC2.x, vertA2.z, vertC2.z
			const __m128 r2Y = _mm_unpackhi_ps( vertA2, vertC2 );	// vertA2.y, vertC2.y, vertA2.w, vertC2.w
			const __m128 r2Z = _mm_unpacklo_ps( vertB2, vertD2 );	// vertB2.x, vertD2.x, vertB2.z, vertD2.z
			const __m128 r2W = _mm_unpackhi_ps( vertB2, vertD2 );	// vertB2.y, vertD2.y, vertB2.w, vertD2.w
			
			const __m128 vert2X = _mm_unpacklo_ps( r2X, r2Z );		// vertA2.x, vertB2.x, vertC2.x, vertD2.x
			const __m128 vert2Y = _mm_unpackhi_ps( r2X, r2Z );		// vertA2.y, vertB2.y, vertC2.y, vertD2.y
			const __m128 vert2Z = _mm_unpacklo_ps( r2Y, r2W );		// vertA2.z, vertB2.z, vertC2.z, vertD2.z
			
			const __m128i triangleCulled = TriangleCulled_SSE2( vert0X, vert0Y, vert0Z, vert1X, vert1Y, vert1Z, vert2X, vert2Y, vert2Z, lightProjectX, lightProjectY, lightProjectZ, lightProjectW );
			
			__m128i triangleFacing = TriangleFacing_SSE2( vert0X, vert0Y, vert0Z, vert1X, vert1Y, vert1Z, vert2X, vert2Y, vert2Z, lightOriginX, lightOriginY, lightOriginZ );
			
			// optionally make triangles that are outside the light frustum facing so they do not contribute to the shadow volume
			triangleFacing = _mm_or_si128( triangleFacing, _mm_and_si128( triangleCulled, cullShadowTrianglesToLightMask ) );
			
			// store culled
			const __m128i culled_s = _mm_packs_epi32( triangleCulled, triangleCulled );
			const __m128i culled_b = _mm_packs_epi16( culled_s, culled_s );
			*( int* )&culled[j] = _mm_cvtsi128_si32( culled_b );
			
			// store facing
			const __m128i facing_s = _mm_packs_epi32( triangleFacing, triangleFacing );
			const __m128i facing_b = _mm_packs_epi16( facing_s, facing_s );
			*( int* )&facing[j] = _mm_cvtsi128_si32( facing_b );
			
			// count the number of facing triangles
			numFrontFacing = _mm_add_epi32( numFrontFacing, _mm_and_si128( triangleFacing, vector_int_one ) );
		}
		
		if( insideShadowVolume != NULL )
		{
			for( int k = batchStart, n = indexStart; k <= batchEnd - 3; k += 3, n++ )
			{
				if( !facing[n] )
				{
					const int i0 = indexesODS[k + 0];
					const int i1 = indexesODS[k + 1];
					const int i2 = indexesODS[k + 2];
					if( R_LineIntersectsTriangleExpandedWithSphere( lineStart, lineEnd, lineDir, lineLength, radius, tempVerts[i2].ToVec3(), tempVerts[i1].ToVec3(), tempVerts[i0].ToVec3() ) )
					{
						*insideShadowVolume = true;
						insideShadowVolume = NULL;
						break;
					}
				}
			}
		}
	}
	
	numFrontFacing = _mm_add_epi32( numFrontFacing, _mm_shuffle_epi32( numFrontFacing, _MM_SHUFFLE( 1, 0, 3, 2 ) ) );
	numFrontFacing = _mm_add_epi32( numFrontFacing, _mm_shuffle_epi32( numFrontFacing, _MM_SHUFFLE( 2, 3, 0, 1 ) ) );
	
	return _mm_cvtsi128_si32( numFrontFacing );
	
#else
	
	idODSStreamedArray< idDrawVert, 32, SBT_DOUBLE, 1 > vertsODS( verts, numVerts );
	
	for( int i = 0; i < numVerts; )
	{
	
		const int nextNumVerts = vertsODS.FetchNextBatch() - 1;
	
		for( ; i <= nextNumVerts; i++ )
		{
			tempVerts[i].ToVec3() = Scalar_LoadSkinnedDrawVertPosition( vertsODS[i], joints );
			tempVerts[i].w = 1.0f;
		}
	}
	
	idODSStreamedArray< triIndex_t, 256, SBT_QUAD, 1 > indexesODS( indexes, numIndexes );
	
	const byte cullShadowTrianglesToLightMask = cullShadowTrianglesToLight ? 255 : 0;
	
	int numFrontFacing = 0;
	
	for( int i = 0, j = 0; i < numIndexes; )
	{
	
		const int batchStart = i;
		const int batchEnd = indexesODS.FetchNextBatch();
		const int indexStart = j;
	
		for( ; i <= batchEnd - 3; i += 3, j++ )
		{
			const int i0 = indexesODS[i + 0];
			const int i1 = indexesODS[i + 1];
			const int i2 = indexesODS[i + 2];
	
			const idVec3& v1 = tempVerts[i0].ToVec3();
			const idVec3& v2 = tempVerts[i1].ToVec3();
			const idVec3& v3 = tempVerts[i2].ToVec3();
	
			const byte triangleCulled = TriangleCulled_Generic( v1, v2, v3, lightProject );
	
			byte triangleFacing = TriangleFacing_Generic( v1, v2, v3, lightOrigin );
	
			// optionally make triangles that are outside the light frustum facing so they do not contribute to the shadow volume
			triangleFacing |= ( triangleCulled & cullShadowTrianglesToLightMask );
	
			culled[j] = triangleCulled;
			facing[j] = triangleFacing;
	
			// count the number of facing triangles
			numFrontFacing += ( triangleFacing & 1 );
		}
	
		if( insideShadowVolume != NULL )
		{
			for( int k = batchStart, n = indexStart; k <= batchEnd - 3; k += 3, n++ )
			{
				if( !facing[n] )
				{
					const int i0 = indexesODS[k + 0];
					const int i1 = indexesODS[k + 1];
					const int i2 = indexesODS[k + 2];
					if( R_LineIntersectsTriangleExpandedWithSphere( lineStart, lineEnd, lineDir, lineLength, radius, tempVerts[i2].ToVec3(), tempVerts[i1].ToVec3(), tempVerts[i0].ToVec3() ) )
					{
						*insideShadowVolume = true;
						insideShadowVolume = NULL;
						break;
					}
				}
			}
		}
	}
	
	return numFrontFacing;
	
#endif
}

/*
============
StreamOut
============
*/
static void StreamOut( void* dst, const void* src, int numBytes )
{
	numBytes = ( numBytes + 15 ) & ~15;
	assert_16_byte_aligned( dst );
	assert_16_byte_aligned( src );
	
#if defined(USE_INTRINSICS)
	int i = 0;
	for( ; i + 128 <= numBytes; i += 128 )
	{
		__m128i d0 = _mm_load_si128( ( const __m128i* )( ( byte* )src + i + 0 * 16 ) );
		__m128i d1 = _mm_load_si128( ( const __m128i* )( ( byte* )src + i + 1 * 16 ) );
		__m128i d2 = _mm_load_si128( ( const __m128i* )( ( byte* )src + i + 2 * 16 ) );
		__m128i d3 = _mm_load_si128( ( const __m128i* )( ( byte* )src + i + 3 * 16 ) );
		__m128i d4 = _mm_load_si128( ( const __m128i* )( ( byte* )src + i + 4 * 16 ) );
		__m128i d5 = _mm_load_si128( ( const __m128i* )( ( byte* )src + i + 5 * 16 ) );
		__m128i d6 = _mm_load_si128( ( const __m128i* )( ( byte* )src + i + 6 * 16 ) );
		__m128i d7 = _mm_load_si128( ( const __m128i* )( ( byte* )src + i + 7 * 16 ) );
		_mm_stream_si128( ( __m128i* )( ( byte* )dst + i + 0 * 16 ), d0 );
		_mm_stream_si128( ( __m128i* )( ( byte* )dst + i + 1 * 16 ), d1 );
		_mm_stream_si128( ( __m128i* )( ( byte* )dst + i + 2 * 16 ), d2 );
		_mm_stream_si128( ( __m128i* )( ( byte* )dst + i + 3 * 16 ), d3 );
		_mm_stream_si128( ( __m128i* )( ( byte* )dst + i + 4 * 16 ), d4 );
		_mm_stream_si128( ( __m128i* )( ( byte* )dst + i + 5 * 16 ), d5 );
		_mm_stream_si128( ( __m128i* )( ( byte* )dst + i + 6 * 16 ), d6 );
		_mm_stream_si128( ( __m128i* )( ( byte* )dst + i + 7 * 16 ), d7 );
	}
	for( ; i + 16 <= numBytes; i += 16 )
	{
		__m128i d = _mm_load_si128( ( __m128i* )( ( byte* )src + i ) );
		_mm_stream_si128( ( __m128i* )( ( byte* )dst + i ), d );
	}
#else
	memcpy( dst, src, numBytes );
#endif
}

/*
============
R_CreateShadowVolumeTriangles
============
*/
static void R_CreateShadowVolumeTriangles( triIndex_t* __restrict shadowIndices, triIndex_t* __restrict indexBuffer, int& numShadowIndexesTotal,
		const byte* __restrict facing, const silEdge_t* __restrict silEdges, const int numSilEdges,
		const triIndex_t* __restrict indexes, const int numIndexes, const bool includeCaps )
{
	assert_spu_local_store( facing );
	assert_not_spu_local_store( shadowIndices );
	assert_not_spu_local_store( silEdges );
	assert_not_spu_local_store( indexes );
	
#if 1
	
	const int IN_BUFFER_SIZE = 64;
	const int OUT_BUFFER_SIZE = IN_BUFFER_SIZE * 8;			// each silhouette edge or cap triangle may create 6 indices (8 > 6)
	const int OUT_BUFFER_DEPTH = 4;							// quad buffer to allow overlapped output streaming
	const int OUT_BUFFER_MASK = ( OUT_BUFFER_SIZE * OUT_BUFFER_DEPTH - 1 );
	
	compile_time_assert( OUT_BUFFER_SIZE * OUT_BUFFER_DEPTH * sizeof( triIndex_t ) == OUTPUT_INDEX_BUFFER_SIZE );
	assert_16_byte_aligned( indexBuffer );
	
	int numShadowIndices = 0;
	int numStreamedIndices = 0;
	
	{
		idODSStreamedArray< silEdge_t, IN_BUFFER_SIZE, SBT_DOUBLE, 1 > silEdgesODS( silEdges, numSilEdges );
		
		for( int i = 0; i < numSilEdges; )
		{
		
			const int nextNumSilEdges = silEdgesODS.FetchNextBatch();
			
			// NOTE: we rely on FetchNextBatch() to wait for all previous DMAs to complete
			while( numShadowIndices - numStreamedIndices >= OUT_BUFFER_SIZE )
			{
				StreamOut( shadowIndices + numStreamedIndices, & indexBuffer[numStreamedIndices & OUT_BUFFER_MASK], OUT_BUFFER_SIZE * sizeof( triIndex_t ) );
				numStreamedIndices += OUT_BUFFER_SIZE;
			}
			
			for( ; i + 4 <= nextNumSilEdges; i += 4 )
			{
				const silEdge_t& sil0 = silEdgesODS[i + 0];
				const silEdge_t& sil1 = silEdgesODS[i + 1];
				const silEdge_t& sil2 = silEdgesODS[i + 2];
				const silEdge_t& sil3 = silEdgesODS[i + 3];
				
				{
					const byte f1a = facing[sil0.p1];
					const byte f2a = facing[sil0.p2];
					const byte ta = ( f1a ^ f2a ) & 6;
					const triIndex_t v1a = sil0.v1 << 1;
					const triIndex_t v2a = sil0.v2 << 1;
					
					WriteIndexPair( &indexBuffer[( numShadowIndices + 0 ) & OUT_BUFFER_MASK], v1a ^ ( 0 & 1 ), v2a ^ ( f1a & 1 ) );
					WriteIndexPair( &indexBuffer[( numShadowIndices + 2 ) & OUT_BUFFER_MASK], v2a ^ ( f2a & 1 ), v1a ^ ( f2a & 1 ) );
					WriteIndexPair( &indexBuffer[( numShadowIndices + 4 ) & OUT_BUFFER_MASK], v1a ^ ( f1a & 1 ), v2a ^ ( 1 & 1 ) );
					
					numShadowIndices += ta;
				}
				
				{
					const byte f1b = facing[sil1.p1];
					const byte f2b = facing[sil1.p2];
					const byte tb = ( f1b ^ f2b ) & 6;
					const triIndex_t v1b = sil1.v1 << 1;
					const triIndex_t v2b = sil1.v2 << 1;
					
					WriteIndexPair( &indexBuffer[( numShadowIndices + 0 ) & OUT_BUFFER_MASK], v1b ^ ( 0 & 1 ), v2b ^ ( f1b & 1 ) );
					WriteIndexPair( &indexBuffer[( numShadowIndices + 2 ) & OUT_BUFFER_MASK], v2b ^ ( f2b & 1 ), v1b ^ ( f2b & 1 ) );
					WriteIndexPair( &indexBuffer[( numShadowIndices + 4 ) & OUT_BUFFER_MASK], v1b ^ ( f1b & 1 ), v2b ^ ( 1 & 1 ) );
					
					numShadowIndices += tb;
				}
				
				{
					const byte f1c = facing[sil2.p1];
					const byte f2c = facing[sil2.p2];
					const byte tc = ( f1c ^ f2c ) & 6;
					const triIndex_t v1c = sil2.v1 << 1;
					const triIndex_t v2c = sil2.v2 << 1;
					
					WriteIndexPair( &indexBuffer[( numShadowIndices + 0 ) & OUT_BUFFER_MASK], v1c ^ ( 0 & 1 ), v2c ^ ( f1c & 1 ) );
					WriteIndexPair( &indexBuffer[( numShadowIndices + 2 ) & OUT_BUFFER_MASK], v2c ^ ( f2c & 1 ), v1c ^ ( f2c & 1 ) );
					WriteIndexPair( &indexBuffer[( numShadowIndices + 4 ) & OUT_BUFFER_MASK], v1c ^ ( f1c & 1 ), v2c ^ ( 1 & 1 ) );
					
					numShadowIndices += tc;
				}
				
				{
					const byte f1d = facing[sil3.p1];
					const byte f2d = facing[sil3.p2];
					const byte td = ( f1d ^ f2d ) & 6;
					const triIndex_t v1d = sil3.v1 << 1;
					const triIndex_t v2d = sil3.v2 << 1;
					
					WriteIndexPair( &indexBuffer[( numShadowIndices + 0 ) & OUT_BUFFER_MASK], v1d ^ ( 0 & 1 ), v2d ^ ( f1d & 1 ) );
					WriteIndexPair( &indexBuffer[( numShadowIndices + 2 ) & OUT_BUFFER_MASK], v2d ^ ( f2d & 1 ), v1d ^ ( f2d & 1 ) );
					WriteIndexPair( &indexBuffer[( numShadowIndices + 4 ) & OUT_BUFFER_MASK], v1d ^ ( f1d & 1 ), v2d ^ ( 1 & 1 ) );
					
					numShadowIndices += td;
				}
			}
			for( ; i + 1 <= nextNumSilEdges; i++ )
			{
				const silEdge_t& sil = silEdgesODS[i];
				
				const byte f1 = facing[sil.p1];
				const byte f2 = facing[sil.p2];
				const byte t = ( f1 ^ f2 ) & 6;
				const triIndex_t v1 = sil.v1 << 1;
				const triIndex_t v2 = sil.v2 << 1;
				
				WriteIndexPair( &indexBuffer[( numShadowIndices + 0 ) & OUT_BUFFER_MASK], v1 ^ ( 0 & 1 ), v2 ^ ( f1 & 1 ) );
				WriteIndexPair( &indexBuffer[( numShadowIndices + 2 ) & OUT_BUFFER_MASK], v2 ^ ( f2 & 1 ), v1 ^ ( f2 & 1 ) );
				WriteIndexPair( &indexBuffer[( numShadowIndices + 4 ) & OUT_BUFFER_MASK], v1 ^ ( f1 & 1 ), v2 ^ ( 1 & 1 ) );
				
				numShadowIndices += t;
			}
		}
	}
	
	if( includeCaps )
	{
		idODSStreamedArray< triIndex_t, IN_BUFFER_SIZE, SBT_QUAD, 1 > indexesODS( indexes, numIndexes );
		
		for( int i = 0, j = 0; i < numIndexes; )
		{
		
			const int nextNumIndexes = indexesODS.FetchNextBatch();
			
			// NOTE: we rely on FetchNextBatch() to wait for all previous DMAs to complete
			while( numShadowIndices - numStreamedIndices >= OUT_BUFFER_SIZE )
			{
				StreamOut( shadowIndices + numStreamedIndices, & indexBuffer[numStreamedIndices & OUT_BUFFER_MASK], OUT_BUFFER_SIZE * sizeof( triIndex_t ) );
				numStreamedIndices += OUT_BUFFER_SIZE;
			}
			
			for( ; i + 4 * 3 <= nextNumIndexes; i += 4 * 3, j += 4 )
			{
				const byte ta = ~facing[j + 0] & 6;
				const byte tb = ~facing[j + 1] & 6;
				const byte tc = ~facing[j + 2] & 6;
				const byte td = ~facing[j + 3] & 6;
				
				const triIndex_t i0a = indexesODS[i + 0 * 3 + 0] << 1;
				const triIndex_t i1a = indexesODS[i + 0 * 3 + 1] << 1;
				const triIndex_t i2a = indexesODS[i + 0 * 3 + 2] << 1;
				
				WriteIndexPair( &indexBuffer[( numShadowIndices + 0 ) & OUT_BUFFER_MASK], i2a + 0, i1a + 0 );
				WriteIndexPair( &indexBuffer[( numShadowIndices + 2 ) & OUT_BUFFER_MASK], i0a + 0, i0a + 1 );
				WriteIndexPair( &indexBuffer[( numShadowIndices + 4 ) & OUT_BUFFER_MASK], i1a + 1, i2a + 1 );
				
				numShadowIndices += ta;
				
				const triIndex_t i0b = indexesODS[i + 1 * 3 + 0] << 1;
				const triIndex_t i1b = indexesODS[i + 1 * 3 + 1] << 1;
				const triIndex_t i2b = indexesODS[i + 1 * 3 + 2] << 1;
				
				WriteIndexPair( &indexBuffer[( numShadowIndices + 0 ) & OUT_BUFFER_MASK], i2b + 0, i1b + 0 );
				WriteIndexPair( &indexBuffer[( numShadowIndices + 2 ) & OUT_BUFFER_MASK], i0b + 0, i0b + 1 );
				WriteIndexPair( &indexBuffer[( numShadowIndices + 4 ) & OUT_BUFFER_MASK], i1b + 1, i2b + 1 );
				
				numShadowIndices += tb;
				
				const triIndex_t i0c = indexesODS[i + 2 * 3 + 0] << 1;
				const triIndex_t i1c = indexesODS[i + 2 * 3 + 1] << 1;
				const triIndex_t i2c = indexesODS[i + 2 * 3 + 2] << 1;
				
				WriteIndexPair( &indexBuffer[( numShadowIndices + 0 ) & OUT_BUFFER_MASK], i2c + 0, i1c + 0 );
				WriteIndexPair( &indexBuffer[( numShadowIndices + 2 ) & OUT_BUFFER_MASK], i0c + 0, i0c + 1 );
				WriteIndexPair( &indexBuffer[( numShadowIndices + 4 ) & OUT_BUFFER_MASK], i1c + 1, i2c + 1 );
				
				numShadowIndices += tc;
				
				const triIndex_t i0d = indexesODS[i + 3 * 3 + 0] << 1;
				const triIndex_t i1d = indexesODS[i + 3 * 3 + 1] << 1;
				const triIndex_t i2d = indexesODS[i + 3 * 3 + 2] << 1;
				
				WriteIndexPair( &indexBuffer[( numShadowIndices + 0 ) & OUT_BUFFER_MASK], i2d + 0, i1d + 0 );
				WriteIndexPair( &indexBuffer[( numShadowIndices + 2 ) & OUT_BUFFER_MASK], i0d + 0, i0d + 1 );
				WriteIndexPair( &indexBuffer[( numShadowIndices + 4 ) & OUT_BUFFER_MASK], i1d + 1, i2d + 1 );
				
				numShadowIndices += td;
			}
			
			for( ; i + 3 <= nextNumIndexes; i += 3, j++ )
			{
				const byte t = ~facing[j] & 6;
				
				const triIndex_t i0 = indexesODS[i + 0] << 1;
				const triIndex_t i1 = indexesODS[i + 1] << 1;
				const triIndex_t i2 = indexesODS[i + 2] << 1;
				
				WriteIndexPair( &indexBuffer[( numShadowIndices + 0 ) & OUT_BUFFER_MASK], i2 + 0, i1 + 0 );
				WriteIndexPair( &indexBuffer[( numShadowIndices + 2 ) & OUT_BUFFER_MASK], i0 + 0, i0 + 1 );
				WriteIndexPair( &indexBuffer[( numShadowIndices + 4 ) & OUT_BUFFER_MASK], i1 + 1, i2 + 1 );
				
				numShadowIndices += t;
			}
		}
	}
	
	while( numShadowIndices - numStreamedIndices >= OUT_BUFFER_SIZE )
	{
		StreamOut( shadowIndices + numStreamedIndices, & indexBuffer[numStreamedIndices & OUT_BUFFER_MASK], OUT_BUFFER_SIZE * sizeof( triIndex_t ) );
		numStreamedIndices += OUT_BUFFER_SIZE;
	}
	if( numShadowIndices > numStreamedIndices )
	{
		assert( numShadowIndices - numStreamedIndices < OUT_BUFFER_SIZE );
		StreamOut( shadowIndices + numStreamedIndices, & indexBuffer[numStreamedIndices & OUT_BUFFER_MASK], ( numShadowIndices - numStreamedIndices ) * sizeof( triIndex_t ) );
	}
	
	numShadowIndexesTotal = numShadowIndices;
	
#if defined(USE_INTRINSICS)
	_mm_sfence();
#endif
	
#else	// NOTE: this code will not work on the SPU because it tries to write directly to the destination
	
	triIndex_t* shadowIndexPtr = shadowIndices;
	
	{
		idODSStreamedArray< silEdge_t, 128, SBT_DOUBLE, 1 > silEdgesODS( silEdges, numSilEdges );
	
		for( int i = 0; i < numSilEdges; )
		{
	
			const int nextNumSilEdges = silEdgesODS.FetchNextBatch() - 1;
	
			for( ; i <= nextNumSilEdges; i++ )
			{
				const silEdge_t& sil = silEdgesODS[i];
	
				const byte f1 = facing[sil.p1] & 1;
				const byte f2 = facing[sil.p2] & 1;
	
				if( ( f1 ^ f2 ) == 0 )
				{
					continue;
				}
	
				const triIndex_t v1 = sil.v1 << 1;
				const triIndex_t v2 = sil.v2 << 1;
	
				// set the two triangle winding orders based on facing
				// without using a poorly-predictable branch
#if 1
				// only write dwords to write combined memory
				WriteIndexPair( shadowIndexPtr + 0, v1 ^  0, v2 ^ f1 );
				WriteIndexPair( shadowIndexPtr + 2, v2 ^ f2, v1 ^ f2 );
				WriteIndexPair( shadowIndexPtr + 4, v1 ^ f1, v2 ^  1 );
#else
				shadowIndexPtr[0] == v1;
				shadowIndexPtr[1] == v2^ f1;
				shadowIndexPtr[2] == v2^ f2;
				shadowIndexPtr[3] == v1^ f2;
				shadowIndexPtr[4] == v1^ f1;
				shadowIndexPtr[5] == v2 ^ 1;
#endif
				shadowIndexPtr += 6;
			}
		}
	}
	
	if( includeCaps )
	{
		idODSStreamedArray< triIndex_t, 256, SBT_QUAD, 1 > indexesODS( indexes, numIndexes );
	
		for( int i = 0, j = 0; i < numIndexes; )
		{
	
			const int nextNumIndexes = indexesODS.FetchNextBatch() - 3;
	
			for( ; i <= nextNumIndexes; i += 3, j++ )
			{
				if( facing[j] )
				{
					continue;
				}
	
				const triIndex_t i0 = indexesODS[i + 0] << 1;
				const triIndex_t i1 = indexesODS[i + 1] << 1;
				const triIndex_t i2 = indexesODS[i + 2] << 1;
#if 1
				// only write dwords to write combined memory
				WriteIndexPair( shadowIndexPtr + 0, i2 + 0, i1 + 0 );
				WriteIndexPair( shadowIndexPtr + 2, i0 + 0, i0 + 1 );
				WriteIndexPair( shadowIndexPtr + 4, i1 + 1, i2 + 1 );
#else
				shadowIndexPtr[0] = i2;
				shadowIndexPtr[1] = i1;
				shadowIndexPtr[2] = i0;
				shadowIndexPtr[3] = i0 + 1;
				shadowIndexPtr[4] = i1 + 1;
				shadowIndexPtr[5] = i2 + 1;
#endif
				shadowIndexPtr += 6;
			}
		}
	}
	
	numShadowIndexesTotal = shadowIndexPtr - shadowIndices;
	
#endif
}

/*
=====================
R_CreateLightTriangles
=====================
*/
void R_CreateLightTriangles( triIndex_t* __restrict lightIndices, triIndex_t* __restrict indexBuffer, int& numLightIndicesTotal,
							 const byte* __restrict culled, const triIndex_t* __restrict indexes, const int numIndexes )
{
	assert_spu_local_store( culled );
	assert_not_spu_local_store( lightIndices );
	assert_not_spu_local_store( indexes );
	
#if 1
	
	const int IN_BUFFER_SIZE = 256;
	const int OUT_BUFFER_SIZE = IN_BUFFER_SIZE * 2;			// there are never more indices generated than the original indices
	const int OUT_BUFFER_DEPTH = 4;							// quad buffer to allow overlapped output streaming
	const int OUT_BUFFER_MASK = ( OUT_BUFFER_SIZE * OUT_BUFFER_DEPTH - 1 );
	
	compile_time_assert( OUT_BUFFER_SIZE * OUT_BUFFER_DEPTH * sizeof( triIndex_t ) == OUTPUT_INDEX_BUFFER_SIZE );
	assert_16_byte_aligned( indexBuffer );
	
	int numLightIndices = 0;
	int numStreamedIndices = 0;
	
	idODSStreamedArray< triIndex_t, IN_BUFFER_SIZE, SBT_QUAD, 1 > indexesODS( indexes, numIndexes );
	
	for( int i = 0, j = 0; i < numIndexes; )
	{
	
		const int nextNumIndexes = indexesODS.FetchNextBatch();
		
		// NOTE: we rely on FetchNextBatch() to wait for all previous DMAs to complete
		while( numLightIndices - numStreamedIndices >= OUT_BUFFER_SIZE )
		{
			StreamOut( lightIndices + numStreamedIndices, & indexBuffer[numStreamedIndices & OUT_BUFFER_MASK], OUT_BUFFER_SIZE * sizeof( triIndex_t ) );
			numStreamedIndices += OUT_BUFFER_SIZE;
		}
		
		for( ; i + 4 * 3 <= nextNumIndexes; i += 4 * 3, j += 4 )
		{
			const byte ta = ~culled[j + 0] & 3;
			const byte tb = ~culled[j + 1] & 3;
			const byte tc = ~culled[j + 2] & 3;
			const byte td = ~culled[j + 3] & 3;
			
			indexBuffer[( numLightIndices + 0 ) & OUT_BUFFER_MASK] = indexesODS[i + 0 * 3 + 0];
			indexBuffer[( numLightIndices + 1 ) & OUT_BUFFER_MASK] = indexesODS[i + 0 * 3 + 1];
			indexBuffer[( numLightIndices + 2 ) & OUT_BUFFER_MASK] = indexesODS[i + 0 * 3 + 2];
			
			numLightIndices += ta;
			
			indexBuffer[( numLightIndices + 0 ) & OUT_BUFFER_MASK] = indexesODS[i + 1 * 3 + 0];
			indexBuffer[( numLightIndices + 1 ) & OUT_BUFFER_MASK] = indexesODS[i + 1 * 3 + 1];
			indexBuffer[( numLightIndices + 2 ) & OUT_BUFFER_MASK] = indexesODS[i + 1 * 3 + 2];
			
			numLightIndices += tb;
			
			indexBuffer[( numLightIndices + 0 ) & OUT_BUFFER_MASK] = indexesODS[i + 2 * 3 + 0];
			indexBuffer[( numLightIndices + 1 ) & OUT_BUFFER_MASK] = indexesODS[i + 2 * 3 + 1];
			indexBuffer[( numLightIndices + 2 ) & OUT_BUFFER_MASK] = indexesODS[i + 2 * 3 + 2];
			
			numLightIndices += tc;
			
			indexBuffer[( numLightIndices + 0 ) & OUT_BUFFER_MASK] = indexesODS[i + 3 * 3 + 0];
			indexBuffer[( numLightIndices + 1 ) & OUT_BUFFER_MASK] = indexesODS[i + 3 * 3 + 1];
			indexBuffer[( numLightIndices + 2 ) & OUT_BUFFER_MASK] = indexesODS[i + 3 * 3 + 2];
			
			numLightIndices += td;
		}
		
		for( ; i + 3 <= nextNumIndexes; i += 3, j++ )
		{
			const byte t = ~culled[j] & 3;
			
			indexBuffer[( numLightIndices + 0 ) & OUT_BUFFER_MASK] = indexesODS[i + 0];
			indexBuffer[( numLightIndices + 1 ) & OUT_BUFFER_MASK] = indexesODS[i + 1];
			indexBuffer[( numLightIndices + 2 ) & OUT_BUFFER_MASK] = indexesODS[i + 2];
			
			numLightIndices += t;
		}
	}
	
	while( numLightIndices - numStreamedIndices >= OUT_BUFFER_SIZE )
	{
		StreamOut( lightIndices + numStreamedIndices, & indexBuffer[numStreamedIndices & OUT_BUFFER_MASK], OUT_BUFFER_SIZE * sizeof( triIndex_t ) );
		numStreamedIndices += OUT_BUFFER_SIZE;
	}
	if( numLightIndices > numStreamedIndices )
	{
		assert( numLightIndices - numStreamedIndices < OUT_BUFFER_SIZE );
		StreamOut( lightIndices + numStreamedIndices, & indexBuffer[numStreamedIndices & OUT_BUFFER_MASK], ( numLightIndices - numStreamedIndices ) * sizeof( triIndex_t ) );
	}
	
	numLightIndicesTotal = numLightIndices;
	
#if defined(USE_INTRINSICS)
	_mm_sfence();
#endif
	
#else	// NOTE: this code will not work on the SPU because it tries to write directly to the destination
	
	int numLightIndices = 0;
	
	idODSStreamedArray< triIndex_t, 256, SBT_QUAD, 1 > indexesODS( indexes, numIndexes );
	
	for( int i = 0, j = 0; i < numIndexes; )
	{
	
		const int nextNumIndexes = indexesODS.FetchNextBatch() - 3;
	
		for( ; i <= nextNumIndexes; i += 3, j++ )
		{
			if( culled[j] )
			{
				continue;
			}
	
			lightIndices[numLightIndices + 0] = indexesODS[i + 0];
			lightIndices[numLightIndices + 1] = indexesODS[i + 1];
			lightIndices[numLightIndices + 2] = indexesODS[i + 2];
	
			numLightIndices += 3;
		}
	}
	
	numLightIndicesTotal = numLightIndices;
	
#endif
}

/*
=====================
DynamicShadowVolumeJob

Creates shadow volume indices for a surface that intersects a light.
Optionally also creates new surface indices with just the triangles
inside the light volume. These indices will be unique for a given
light / surface combination.

The shadow volume indices are created using the original surface vertices.
However, the indices are setup to be used with a shadow volume vertex buffer
with all vertices duplicated where the even vertices have the same positions
as the surface vertices (at the near cap) and each odd vertex has the
same position as the previous even vertex but is projected to infinity
(the far cap) in the vertex program.
=====================
*/
void DynamicShadowVolumeJob( const dynamicShadowVolumeParms_t* parms )
{
	if( parms->tempFacing == NULL )
	{
		*const_cast< byte** >( &parms->tempFacing ) = ( byte* )_alloca16( TEMP_FACING( parms->numIndexes ) );
	}
	if( parms->tempCulled == NULL )
	{
		*const_cast< byte** >( &parms->tempCulled ) = ( byte* )_alloca16( TEMP_CULL( parms->numIndexes ) );
	}
	if( parms->tempVerts == NULL && parms->joints != NULL )
	{
		*const_cast< idVec4** >( &parms->tempVerts ) = ( idVec4* )_alloca16( TEMP_VERTS( parms->numVerts ) );
	}
	if( parms->indexBuffer == NULL )
	{
		*const_cast< triIndex_t** >( &parms->indexBuffer ) = ( triIndex_t* )_alloca16( OUTPUT_INDEX_BUFFER_SIZE );
	}
	
	assert( parms->joints == NULL || parms->numJoints > 0 );
	
	// Calculate the shadow depth bounds.
	float shadowZMin = parms->lightZMin;
	float shadowZMax = parms->lightZMax;
	if( parms->useShadowDepthBounds )
	{
		idRenderMatrix::DepthBoundsForShadowBounds( shadowZMin, shadowZMax, parms->triangleMVP, parms->triangleBounds, parms->localLightOrigin, true );
		shadowZMin = Max( shadowZMin, parms->lightZMin );
		shadowZMax = Min( shadowZMax, parms->lightZMax );
	}
	
	bool renderZFail = false;
	int numShadowIndices = 0;
	int numLightIndices = 0;
	
	// The shadow volume may be depth culled if either the shadow volume was culled to the view frustum or if the
	// depth range of the visible part of the shadow volume is outside the depth range of the light volume.
	if( shadowZMin < shadowZMax )
	{
	
		// Check if we need to render the shadow volume with Z-fail.
		bool* preciseInsideShadowVolume = NULL;
		// If the view is potentially inside the shadow volume bounds we may need to render with Z-fail.
		if( R_ViewPotentiallyInsideInfiniteShadowVolume( parms->triangleBounds, parms->localLightOrigin, parms->localViewOrigin, parms->zNear * INSIDE_SHADOW_VOLUME_EXTRA_STRETCH ) )
		{
			// Optionally perform a more precise test to see whether or not the view is inside the shadow volume.
			if( parms->useShadowPreciseInsideTest )
			{
				preciseInsideShadowVolume = & renderZFail;
			}
			else
			{
				renderZFail = true;
			}
		}
		
		// Calculate the facing of each triangle and cull each triangle to the light volume.
		// Optionally also calculate more precisely whether or not the view is inside the shadow volume.
		int numFrontFacing = 0;
		if( parms->joints != NULL )
		{
			numFrontFacing = CalculateTriangleFacingCulledSkinned( parms->tempFacing, parms->tempCulled, parms->tempVerts, parms->indexes, parms->numIndexes,
							 parms->verts, parms->numVerts, parms->joints,
							 parms->localLightOrigin, parms->localViewOrigin,
							 parms->cullShadowTrianglesToLight, parms->localLightProject,
							 preciseInsideShadowVolume, parms->zNear * INSIDE_SHADOW_VOLUME_EXTRA_STRETCH );
		}
		else
		{
			numFrontFacing = CalculateTriangleFacingCulledStatic( parms->tempFacing, parms->tempCulled, parms->indexes, parms->numIndexes,
							 parms->verts, parms->numVerts,
							 parms->localLightOrigin, parms->localViewOrigin,
							 parms->cullShadowTrianglesToLight, parms->localLightProject,
							 preciseInsideShadowVolume, parms->zNear * INSIDE_SHADOW_VOLUME_EXTRA_STRETCH );
		}
		
		// Create shadow volume indices.
		if( parms->shadowIndices != NULL )
		{
			const int numTriangles = parms->numIndexes / 3;
			
			// If there are any triangles facing away from the light.
			if( numTriangles - numFrontFacing > 0 )
			{
				// Set the "fake triangle" used by dangling edges to facing so a dangling edge will
				// make a silhouette if the triangle that uses the dangling edges is not facing.
				// Note that dangling edges outside the light frustum do not make silhouettes because
				// a triangle outside the light frustum is also set to facing just like the "fake triangle"
				// used by a dangling edge.
				parms->tempFacing[numTriangles] = 255;
				
				// Check if we can avoid rendering the shadow volume caps.
				bool renderShadowCaps = parms->forceShadowCaps || renderZFail;
				
				// Create new triangles along the silhouette planes and optionally add end-cap triangles on the model and on the distant projection.
				R_CreateShadowVolumeTriangles( parms->shadowIndices, parms->indexBuffer, numShadowIndices, parms->tempFacing,
											   parms->silEdges, parms->numSilEdges, parms->indexes, parms->numIndexes, renderShadowCaps );
											   
				assert( numShadowIndices <= parms->maxShadowIndices );
			}
		}
		
		// Create new indices with only the triangles that are inside the light volume.
		if( parms->lightIndices != NULL )
		{
			R_CreateLightTriangles( parms->lightIndices, parms->indexBuffer, numLightIndices, parms->tempCulled, parms->indexes, parms->numIndexes );
			
			assert( numLightIndices <= parms->maxLightIndices );
		}
	}
	
	// write out the number of shadow indices
	if( parms->numShadowIndices != NULL )
	{
		*parms->numShadowIndices = numShadowIndices;
	}
	// write out the number of light indices
	if( parms->numLightIndices != NULL )
	{
		*parms->numLightIndices = numLightIndices;
	}
	// write out whether or not the shadow volume needs to be rendered with Z-Fail
	if( parms->renderZFail != NULL )
	{
		*parms->renderZFail = renderZFail;
	}
	// write out the shadow depth bounds
	if( parms->shadowZMin != NULL )
	{
		*parms->shadowZMin = shadowZMin;
	}
	if( parms->shadowZMax != NULL )
	{
		*parms->shadowZMax = shadowZMax;
	}
	// write out the shadow volume state
	if( parms->shadowVolumeState != NULL )
	{
		*parms->shadowVolumeState = SHADOWVOLUME_DONE;
	}
}

REGISTER_PARALLEL_JOB( DynamicShadowVolumeJob, "DynamicShadowVolumeJob" );
