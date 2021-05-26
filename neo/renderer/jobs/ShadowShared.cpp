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

#include "../../idlib/ParallelJobList_JobHeaders.h"
#include "../../idlib/SoftwareCache.h"

#include "../../idlib/math/Vector.h"
#include "../../idlib/math/Matrix.h"
#include "../../idlib/math/Quat.h"
#include "../../idlib/math/Rotation.h"
#include "../../idlib/math/Plane.h"
#include "../../idlib/bv/Sphere.h"
#include "../../idlib/bv/Bounds.h"
#include "../../idlib/geometry/JointTransform.h"
#include "../../idlib/geometry/DrawVert.h"
#include "../../idlib/geometry/RenderMatrix.h"
#include "ShadowShared.h"


/*
======================
R_ViewPotentiallyInsideInfiniteShadowVolume

If we know that we are "off to the side" of an infinite shadow volume,
we can draw it without caps in Z-pass mode.
======================
*/
bool R_ViewPotentiallyInsideInfiniteShadowVolume( const idBounds& occluderBounds, const idVec3& localLight, const idVec3& localView, const float zNear )
{
	// Expand the bounds to account for the near clip plane, because the
	// view could be mathematically outside, but if the near clip plane
	// chops a volume edge then the Z-pass rendering would fail.
	const idBounds expandedBounds = occluderBounds.Expand( zNear );

	// If the view is inside the geometry bounding box then the view
	// is also inside the shadow projection.
	if( expandedBounds.ContainsPoint( localView ) )
	{
		return true;
	}

	// If the light is inside the geometry bounding box then the shadow is projected
	// in all directions and any view position is inside the infinte shadow projection.
	if( expandedBounds.ContainsPoint( localLight ) )
	{
		return true;
	}

	// If the line from localLight to localView intersects the geometry
	// bounding box then the view is inside the infinite shadow projection.
	if( expandedBounds.LineIntersection( localLight, localView ) )
	{
		return true;
	}

	// The view is definitely not inside the projected shadow.
	return false;
}

/*
====================
R_ShadowVolumeCullBits
====================
*/
static void R_ShadowVolumeCullBits( byte* cullBits, byte& totalOr, const float radius, const idPlane* planes, const idShadowVert* verts, const int numVerts )
{
	assert_16_byte_aligned( cullBits );
	assert_16_byte_aligned( verts );

#if defined(USE_INTRINSICS_SSE)
	idODSStreamedArray< idShadowVert, 16, SBT_DOUBLE, 4 > vertsODS( verts, numVerts );

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
			const __m128 v0 = _mm_load_ps( vertsODS[i + 0].xyzw.ToFloatPtr() );
			const __m128 v1 = _mm_load_ps( vertsODS[i + 1].xyzw.ToFloatPtr() );
			const __m128 v2 = _mm_load_ps( vertsODS[i + 2].xyzw.ToFloatPtr() );
			const __m128 v3 = _mm_load_ps( vertsODS[i + 3].xyzw.ToFloatPtr() );

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

	idODSStreamedArray< idShadowVert, 16, SBT_DOUBLE, 1 > vertsODS( verts, numVerts );

	byte tOr = 0;
	for( int i = 0; i < numVerts; )
	{

		const int nextNumVerts = vertsODS.FetchNextBatch() - 1;

		for( ; i <= nextNumVerts; i++ )
		{
			const idVec3& v = vertsODS[i].xyzw.ToVec3();

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
===================
R_SegmentToSegmentDistanceSquare
===================
*/
static float R_SegmentToSegmentDistanceSquare( const idVec3& start1, const idVec3& end1, const idVec3& start2, const idVec3& end2 )
{

	const idVec3 dir0 = start1 - start2;
	const idVec3 dir1 = end1 - start1;
	const idVec3 dir2 = end2 - start2;

	const float dotDir1Dir1 = dir1 * dir1;
	const float dotDir2Dir2 = dir2 * dir2;
	const float dotDir1Dir2 = dir1 * dir2;
	const float dotDir0Dir1 = dir0 * dir1;
	const float dotDir0Dir2 = dir0 * dir2;

	if( dotDir1Dir1 < idMath::FLT_SMALLEST_NON_DENORMAL || dotDir2Dir2 < idMath::FLT_SMALLEST_NON_DENORMAL )
	{
		// At least one of the lines is degenerate.
		// The returned length is correct only if both lines are degenerate otherwise the start point of
		// the degenerate line has to be projected onto the other line to calculate the shortest distance.
		// The degenerate case is not relevant here though.
		return ( start2 - start1 ).LengthSqr();
	}

	const float d = dotDir1Dir1 * dotDir2Dir2 - dotDir1Dir2 * dotDir1Dir2;

	if( d < idMath::FLT_SMALLEST_NON_DENORMAL )
	{
		// The lines are parallel.
		// The returned length is not correct.
		// The parallel case is not relevent here though.
		return ( start2 - start1 ).LengthSqr();
	}

	const float n = dotDir0Dir2 * dotDir1Dir2 - dotDir0Dir1 * dotDir2Dir2;

	const float t1 = n / d;
	const float t1c = idMath::ClampFloat( 0.0f, 1.0f, t1 );

	const float t2 = ( dotDir0Dir2 + dotDir1Dir2 * t1 ) / dotDir2Dir2;
	const float t2c = idMath::ClampFloat( 0.0f, 1.0f, t2 );

	const idVec3 closest1 = start1 + ( dir1 * t1c );
	const idVec3 closest2 = start2 + ( dir2 * t2c );

	const float distSqr = ( closest2 - closest1 ).LengthSqr();

	return distSqr;
}

/*
===================
R_LineIntersectsTriangleExpandedWithSphere
===================
*/
bool R_LineIntersectsTriangleExpandedWithSphere( const idVec3& lineStart, const idVec3& lineEnd, const idVec3& lineDir, const float lineLength,
		const float sphereRadius, const idVec3& triVert0, const idVec3& triVert1, const idVec3& triVert2 )
{
	// edge directions
	const idVec3 edge1 = triVert1 - triVert0;
	const idVec3 edge2 = triVert2 - triVert0;

	// calculate determinant
	const idVec3 tvec = lineStart - triVert0;
	const idVec3 pvec = lineDir.Cross( edge1 );
	const idVec3 qvec = tvec.Cross( edge2 );
	const float det = edge2 * pvec;

	// calculate UV parameters
	const float u = ( tvec * pvec ) * det;
	const float v = ( lineDir * qvec ) * det;

	// test if the line passes through the triangle
	if( u >= 0.0f && u <= det * det )
	{
		if( v >= 0.0f && u + v <= det * det )
		{
			// if determinant is near zero then the ray lies in the triangle plane
			if( idMath::Fabs( det ) > idMath::FLT_SMALLEST_NON_DENORMAL )
			{
				const float fraction = ( edge1 * qvec ) / det;
				if( fraction >= 0.0f && fraction <= lineLength )
				{
					return true;
				}
			}
		}
	}

	const float radiusSqr = sphereRadius * sphereRadius;

	if( R_SegmentToSegmentDistanceSquare( lineStart, lineEnd, triVert0, triVert1 ) < radiusSqr )
	{
		return true;
	}

	if( R_SegmentToSegmentDistanceSquare( lineStart, lineEnd, triVert1, triVert2 ) < radiusSqr )
	{
		return true;
	}

	if( R_SegmentToSegmentDistanceSquare( lineStart, lineEnd, triVert2, triVert0 ) < radiusSqr )
	{
		return true;
	}

	return false;
}

/*
===================
R_ViewInsideShadowVolume

If the light origin is visible from the view origin without intersecting any
shadow volume near cap triangles then the view is not inside the shadow volume.
The near cap triangles of the shadow volume are expanded to account for the near
clip plane, because the view could be mathematically outside, but if the
near clip plane chops a triangle then Z-pass rendering would fail.

This is expensive but if the CPU time is available this can avoid many
cases where the shadow volume would otherwise be rendered with Z-fail.
Rendering with Z-fail can be significantly slower even on today's hardware.
===================
*/
bool R_ViewInsideShadowVolume( byte* cullBits, const idShadowVert* verts, int numVerts, const triIndex_t* indexes, int numIndexes,
							   const idVec3& localLightOrigin, const idVec3& localViewOrigin, const float zNear )
{

	ALIGNTYPE16 idPlane planes[4];
	// create two planes orthogonal to each other that intersect along the trace
	idVec3 startDir = localLightOrigin - localViewOrigin;
	startDir.Normalize();
	startDir.NormalVectors( planes[0].Normal(), planes[1].Normal() );
	planes[0][3] = - localViewOrigin * planes[0].Normal();
	planes[1][3] = - localViewOrigin * planes[1].Normal();
	// create front and end planes so the trace is on the positive sides of both
	planes[2] = startDir;
	planes[2][3] = - localViewOrigin * planes[2].Normal();
	planes[3] = -startDir;
	planes[3][3] = - localLightOrigin * planes[3].Normal();

	// catagorize each point against the four planes
	byte totalOr = 0;

	R_ShadowVolumeCullBits( cullBits, totalOr, zNear, planes, verts, numVerts );

	// if we don't have points on both sides of both the ray planes, no intersection
	if( ( totalOr ^ ( totalOr >> 4 ) ) & 3 )
	{
		return false;
	}

	// if we don't have any points between front and end, no intersection
	if( ( totalOr ^ ( totalOr >> 1 ) ) & 4 )
	{
		return false;
	}

	// start streaming the indexes
	idODSStreamedArray< triIndex_t, 256, SBT_QUAD, 3 > indexesODS( indexes, numIndexes );

	// Calculate the start, end, dir and length of the line from the view origin to the light origin.
	const idVec3 lineStart = localViewOrigin;
	const idVec3 lineEnd = localLightOrigin;
	const idVec3 lineDelta = lineEnd - lineStart;
	const float lineLengthSqr = lineDelta.LengthSqr();
	const float lineLengthRcp = idMath::InvSqrt( lineLengthSqr );
	const idVec3 lineDir = lineDelta * lineLengthRcp;
	const float lineLength = lineLengthSqr * lineLengthRcp;

	for( int i = 0; i < numIndexes; )
	{

		const int nextNumIndexes = indexesODS.FetchNextBatch() - 3;

		for( ; i <= nextNumIndexes; i += 3 )
		{
			const int i0 = indexesODS[i + 0];
			const int i1 = indexesODS[i + 1];
			const int i2 = indexesODS[i + 2];

			// Get sidedness info for the triangle.
			const byte triOr = cullBits[i0] | cullBits[i1] | cullBits[i2];

			// If there are no points on both sides of both the ray planes, no intersection.
			if( likely( ( triOr ^ ( triOr >> 4 ) ) & 3 ) )
			{
				continue;
			}

			// If there are no points between front and end, no intersection.
			if( unlikely( ( triOr ^ ( triOr >> 1 ) ) & 4 ) )
			{
				continue;
			}

			const idODSObject< idVec4 > triVert0( & verts[i0].xyzw );
			const idODSObject< idVec4 > triVert1( & verts[i1].xyzw );
			const idODSObject< idVec4 > triVert2( & verts[i2].xyzw );

			// If the W of any of the coordinates is zero then the triangle is at or
			// stretches to infinity which means it is not part of the near cap.
			if( triVert0->w == 0.0f || triVert1->w == 0.0f || triVert2->w == 0.0f )
			{
				continue;
			}

			// Test against the expanded triangle to see if we hit the near cap.
			if( R_LineIntersectsTriangleExpandedWithSphere( lineStart, lineEnd, lineDir, lineLength, zNear,
					triVert2->ToVec3(), triVert1->ToVec3(), triVert0->ToVec3() ) )
			{
				return true;
			}
		}
	}

	return false;
}
