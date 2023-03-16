/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2016 Robert Beckebans
Copyright (C) 2014-2016 Kot in Action Creative Artel

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
idRenderModelOverlay::idRenderModelOverlay
====================
*/
idRenderModelOverlay::idRenderModelOverlay() :
	firstOverlay( 0 ),
	nextOverlay( 0 ),
	firstDeferredOverlay( 0 ),
	nextDeferredOverlay( 0 ),
	numOverlayMaterials( 0 ),
	index( -1 ),
	demoSerialWrite( 0 ),
	demoSerialCurrent( 0 )
{
	memset( overlays, 0, sizeof( overlays ) );
}

/*
====================
idRenderModelOverlay::~idRenderModelOverlay
====================
*/
idRenderModelOverlay::~idRenderModelOverlay()
{
	for( unsigned int i = 0; i < MAX_OVERLAYS; i++ )
	{
		FreeOverlay( overlays[i] );
	}
}

/*
=================
idRenderModelOverlay::ReUse
=================
*/
void idRenderModelOverlay::ReUse()
{
	firstOverlay = 0;
	nextOverlay = 0;
	firstDeferredOverlay = 0;
	nextDeferredOverlay = 0;
	numOverlayMaterials = 0;
	demoSerialCurrent++;

	for( unsigned int i = 0; i < MAX_OVERLAYS; i++ )
	{
		FreeOverlay( overlays[i] );
	}
}

/*
====================
idRenderModelOverlay::FreeOverlay
====================
*/
void idRenderModelOverlay::FreeOverlay( overlay_t& overlay )
{
	if( overlay.verts != NULL )
	{
		Mem_Free( overlay.verts );
	}
	if( overlay.indexes != NULL )
	{
		Mem_Free( overlay.indexes );
	}
	memset( &overlay, 0, sizeof( overlay ) );
}

/*
====================
R_OverlayPointCullStatic
====================
*/
static void R_OverlayPointCullStatic( byte* cullBits, halfFloat_t* texCoordS, halfFloat_t* texCoordT, const idPlane* planes, const idDrawVert* verts, const int numVerts )
{
	assert_16_byte_aligned( cullBits );
	assert_16_byte_aligned( texCoordS );
	assert_16_byte_aligned( texCoordT );
	assert_16_byte_aligned( verts );

#if defined(USE_INTRINSICS_SSE)
	idODSStreamedArray< idDrawVert, 16, SBT_DOUBLE, 4 > vertsODS( verts, numVerts );

	const __m128 vector_float_zero	= { 0.0f, 0.0f, 0.0f, 0.0f };
	const __m128 vector_float_one	= { 1.0f, 1.0f, 1.0f, 1.0f };
	const __m128i vector_int_mask0	= _mm_set1_epi32( 1 << 0 );
	const __m128i vector_int_mask1	= _mm_set1_epi32( 1 << 1 );
	const __m128i vector_int_mask2	= _mm_set1_epi32( 1 << 2 );
	const __m128i vector_int_mask3	= _mm_set1_epi32( 1 << 3 );

	const __m128 p0 = _mm_loadu_ps( planes[0].ToFloatPtr() );
	const __m128 p1 = _mm_loadu_ps( planes[1].ToFloatPtr() );

	const __m128 p0X = _mm_splat_ps( p0, 0 );
	const __m128 p0Y = _mm_splat_ps( p0, 1 );
	const __m128 p0Z = _mm_splat_ps( p0, 2 );
	const __m128 p0W = _mm_splat_ps( p0, 3 );

	const __m128 p1X = _mm_splat_ps( p1, 0 );
	const __m128 p1Y = _mm_splat_ps( p1, 1 );
	const __m128 p1Z = _mm_splat_ps( p1, 2 );
	const __m128 p1W = _mm_splat_ps( p1, 3 );

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
			const __m128 d2 = _mm_sub_ps( vector_float_one, d0 );
			const __m128 d3 = _mm_sub_ps( vector_float_one, d1 );

			__m128i flt16S = FastF32toF16( __m128c( d0 ) );
			__m128i flt16T = FastF32toF16( __m128c( d1 ) );

			_mm_storel_epi64( ( __m128i* )&texCoordS[i], flt16S );
			_mm_storel_epi64( ( __m128i* )&texCoordT[i], flt16T );

			__m128i c0 = __m128c( _mm_cmplt_ps( d0, vector_float_zero ) );
			__m128i c1 = __m128c( _mm_cmplt_ps( d1, vector_float_zero ) );
			__m128i c2 = __m128c( _mm_cmplt_ps( d2, vector_float_zero ) );
			__m128i c3 = __m128c( _mm_cmplt_ps( d3, vector_float_zero ) );

			c0 = _mm_and_si128( c0, vector_int_mask0 );
			c1 = _mm_and_si128( c1, vector_int_mask1 );
			c2 = _mm_and_si128( c2, vector_int_mask2 );
			c3 = _mm_and_si128( c3, vector_int_mask3 );

			c0 = _mm_or_si128( c0, c1 );
			c2 = _mm_or_si128( c2, c3 );
			c0 = _mm_or_si128( c0, c2 );

			c0 = _mm_packs_epi32( c0, c0 );
			c0 = _mm_packus_epi16( c0, c0 );

			*( unsigned int* )&cullBits[i] = _mm_cvtsi128_si32( c0 );
		}
	}

#else

	idODSStreamedArray< idDrawVert, 16, SBT_DOUBLE, 1 > vertsODS( verts, numVerts );

	for( int i = 0; i < numVerts; )
	{

		const int nextNumVerts = vertsODS.FetchNextBatch() - 1;

		for( ; i <= nextNumVerts; i++ )
		{
			const idVec3& v = vertsODS[i].xyz;

			const float d0 = planes[0].Distance( v );
			const float d1 = planes[1].Distance( v );
			const float d2 = 1.0f - d0;
			const float d3 = 1.0f - d1;

			halfFloat_t s = Scalar_FastF32toF16( d0 );
			halfFloat_t t = Scalar_FastF32toF16( d1 );

			texCoordS[i] = s;
			texCoordT[i] = t;

			byte bits;
			bits  = IEEE_FLT_SIGNBITSET( d0 ) << 0;
			bits |= IEEE_FLT_SIGNBITSET( d1 ) << 1;
			bits |= IEEE_FLT_SIGNBITSET( d2 ) << 2;
			bits |= IEEE_FLT_SIGNBITSET( d3 ) << 3;

			cullBits[i] = bits;
		}
	}

#endif
}

/*
====================
R_OverlayPointCullSkinned
====================
*/
static void R_OverlayPointCullSkinned( byte* cullBits, halfFloat_t* texCoordS, halfFloat_t* texCoordT, const idPlane* planes, const idDrawVert* verts, const int numVerts, const idJointMat* joints )
{
	assert_16_byte_aligned( cullBits );
	assert_16_byte_aligned( texCoordS );
	assert_16_byte_aligned( texCoordT );
	assert_16_byte_aligned( verts );

#if defined(USE_INTRINSICS_SSE)
	idODSStreamedArray< idDrawVert, 16, SBT_DOUBLE, 4 > vertsODS( verts, numVerts );

	const __m128 vector_float_zero	= { 0.0f, 0.0f, 0.0f, 0.0f };
	const __m128 vector_float_one	= { 1.0f, 1.0f, 1.0f, 1.0f };
	const __m128i vector_int_mask0	= _mm_set1_epi32( 1 << 0 );
	const __m128i vector_int_mask1	= _mm_set1_epi32( 1 << 1 );
	const __m128i vector_int_mask2	= _mm_set1_epi32( 1 << 2 );
	const __m128i vector_int_mask3	= _mm_set1_epi32( 1 << 3 );

	const __m128 p0 = _mm_loadu_ps( planes[0].ToFloatPtr() );
	const __m128 p1 = _mm_loadu_ps( planes[1].ToFloatPtr() );

	const __m128 p0X = _mm_splat_ps( p0, 0 );
	const __m128 p0Y = _mm_splat_ps( p0, 1 );
	const __m128 p0Z = _mm_splat_ps( p0, 2 );
	const __m128 p0W = _mm_splat_ps( p0, 3 );

	const __m128 p1X = _mm_splat_ps( p1, 0 );
	const __m128 p1Y = _mm_splat_ps( p1, 1 );
	const __m128 p1Z = _mm_splat_ps( p1, 2 );
	const __m128 p1W = _mm_splat_ps( p1, 3 );

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
			const __m128 d2 = _mm_sub_ps( vector_float_one, d0 );
			const __m128 d3 = _mm_sub_ps( vector_float_one, d1 );

			__m128i flt16S = FastF32toF16( __m128c( d0 ) );
			__m128i flt16T = FastF32toF16( __m128c( d1 ) );

			_mm_storel_epi64( ( __m128i* )&texCoordS[i], flt16S );
			_mm_storel_epi64( ( __m128i* )&texCoordT[i], flt16T );

			__m128i c0 = __m128c( _mm_cmplt_ps( d0, vector_float_zero ) );
			__m128i c1 = __m128c( _mm_cmplt_ps( d1, vector_float_zero ) );
			__m128i c2 = __m128c( _mm_cmplt_ps( d2, vector_float_zero ) );
			__m128i c3 = __m128c( _mm_cmplt_ps( d3, vector_float_zero ) );

			c0 = _mm_and_si128( c0, vector_int_mask0 );
			c1 = _mm_and_si128( c1, vector_int_mask1 );
			c2 = _mm_and_si128( c2, vector_int_mask2 );
			c3 = _mm_and_si128( c3, vector_int_mask3 );

			c0 = _mm_or_si128( c0, c1 );
			c2 = _mm_or_si128( c2, c3 );
			c0 = _mm_or_si128( c0, c2 );

			c0 = _mm_packs_epi32( c0, c0 );
			c0 = _mm_packus_epi16( c0, c0 );

			*( unsigned int* )&cullBits[i] = _mm_cvtsi128_si32( c0 );
		}
	}

#else

	idODSStreamedArray< idDrawVert, 16, SBT_DOUBLE, 1 > vertsODS( verts, numVerts );

	for( int i = 0; i < numVerts; )
	{

		const int nextNumVerts = vertsODS.FetchNextBatch() - 1;

		for( ; i <= nextNumVerts; i++ )
		{
			const idVec3 transformed = Scalar_LoadSkinnedDrawVertPosition( vertsODS[i], joints );

			const float d0 = planes[0].Distance( transformed );
			const float d1 = planes[1].Distance( transformed );
			const float d2 = 1.0f - d0;
			const float d3 = 1.0f - d1;

			halfFloat_t s = Scalar_FastF32toF16( d0 );
			halfFloat_t t = Scalar_FastF32toF16( d1 );

			texCoordS[i] = s;
			texCoordT[i] = t;

			byte bits;
			bits  = IEEE_FLT_SIGNBITSET( d0 ) << 0;
			bits |= IEEE_FLT_SIGNBITSET( d1 ) << 1;
			bits |= IEEE_FLT_SIGNBITSET( d2 ) << 2;
			bits |= IEEE_FLT_SIGNBITSET( d3 ) << 3;

			cullBits[i] = bits;
		}
	}

#endif
}

/*
=====================
idRenderModelOverlay::CreateOverlay

This projects on both front and back sides to avoid seams
The material should be clamped, because entire triangles are added, some of which
may extend well past the 0.0 to 1.0 texture range
=====================
*/
void idRenderModelOverlay::CreateOverlay( const idRenderModel* model, const idPlane localTextureAxis[2], const idMaterial* material )
{
	// count up the maximum possible vertices and indexes per surface
	int maxVerts = 0;
	int maxIndexes = 0;
	for( int surfNum = 0; surfNum < model->NumSurfaces(); surfNum++ )
	{
		const modelSurface_t* surf = model->Surface( surfNum );
		if( surf->geometry->numVerts > maxVerts )
		{
			maxVerts = surf->geometry->numVerts;
		}
		if( surf->geometry->numIndexes > maxIndexes )
		{
			maxIndexes = surf->geometry->numIndexes;
		}
	}
	maxIndexes += 3 * 16 / sizeof( triIndex_t );	// to allow the index size to be a multiple of 16 bytes

	// make temporary buffers for the building process
	idTempArray< byte > cullBits( maxVerts );
	idTempArray< halfFloat_t > texCoordS( maxVerts );
	idTempArray< halfFloat_t > texCoordT( maxVerts );
	idTempArray< triIndex_t > vertexRemap( maxVerts );
	idTempArray< overlayVertex_t > overlayVerts( maxVerts );
	idTempArray< triIndex_t > overlayIndexes( maxIndexes );

	// pull out the triangles we need from the base surfaces
	for( int surfNum = 0; surfNum < model->NumBaseSurfaces(); surfNum++ )
	{
		const modelSurface_t* surf = model->Surface( surfNum );

		if( surf->geometry == NULL || surf->shader == NULL )
		{
			continue;
		}

		// some surfaces can explicitly disallow overlays
		if( !surf->shader->AllowOverlays() )
		{
			continue;
		}

		const srfTriangles_t* tri = surf->geometry;

		// try to cull the whole surface along the first texture axis
		const float d0 = tri->bounds.PlaneDistance( localTextureAxis[0] );
		if( d0 < 0.0f || d0 > 1.0f )
		{
			continue;
		}

		// try to cull the whole surface along the second texture axis
		const float d1 = tri->bounds.PlaneDistance( localTextureAxis[1] );
		if( d1 < 0.0f || d1 > 1.0f )
		{
			continue;
		}

		// RB: added check wether GPU skinning is available at all
		if( tri->staticModelWithJoints != NULL && r_useGPUSkinning.GetBool() )
		{
			R_OverlayPointCullSkinned( cullBits.Ptr(), texCoordS.Ptr(), texCoordT.Ptr(), localTextureAxis, tri->verts, tri->numVerts, tri->staticModelWithJoints->jointsInverted );
		}
		else
		{
			R_OverlayPointCullStatic( cullBits.Ptr(), texCoordS.Ptr(), texCoordT.Ptr(), localTextureAxis, tri->verts, tri->numVerts );
		}
		// RB end

		// start streaming the indexes
		idODSStreamedArray< triIndex_t, 256, SBT_QUAD, 3 > indexesODS( tri->indexes, tri->numIndexes );

		memset( vertexRemap.Ptr(), -1, vertexRemap.Size() );
		int numIndexes = 0;
		int numVerts = 0;
		int maxReferencedVertex = 0;

		// find triangles that need the overlay
		for( int i = 0; i < tri->numIndexes; )
		{

			const int nextNumIndexes = indexesODS.FetchNextBatch() - 3;

			for( ; i <= nextNumIndexes; i += 3 )
			{
				const int i0 = indexesODS[i + 0];
				const int i1 = indexesODS[i + 1];
				const int i2 = indexesODS[i + 2];

				// skip triangles completely off one side
				if( cullBits[i0] & cullBits[i1] & cullBits[i2] )
				{
					continue;
				}

				// we could do more precise triangle culling, like a light interaction does, but it's not worth it

				// keep this triangle
				for( int j = 0; j < 3; j++ )
				{
					int index = tri->indexes[i + j];
					if( vertexRemap[index] == ( triIndex_t ) - 1 )
					{
						vertexRemap[index] = numVerts;

						overlayVerts[numVerts].vertexNum = index;
						overlayVerts[numVerts].st[0] = texCoordS[index];
						overlayVerts[numVerts].st[1] = texCoordT[index];
						numVerts++;

						maxReferencedVertex = Max( maxReferencedVertex, index );
					}
					overlayIndexes[numIndexes] = vertexRemap[index];
					numIndexes++;
				}
			}
		}

		if( numIndexes == 0 )
		{
			continue;
		}

		// add degenerate triangles until the index size is a multiple of 16 bytes
		for( ; ( ( ( numIndexes * sizeof( triIndex_t ) ) & 15 ) != 0 ); numIndexes += 3 )
		{
			overlayIndexes[numIndexes + 0] = 0;
			overlayIndexes[numIndexes + 1] = 0;
			overlayIndexes[numIndexes + 2] = 0;
		}

		demoSerialCurrent++;

		// allocate a new overlay
		overlay_t& overlay = overlays[nextOverlay++ & ( MAX_OVERLAYS - 1 )];
		FreeOverlay( overlay );
		overlay.material = material;
		overlay.surfaceNum = surfNum;
		overlay.surfaceId = surf->id;
		overlay.numIndexes = numIndexes;
		overlay.indexes = ( triIndex_t* )Mem_Alloc( numIndexes * sizeof( overlay.indexes[0] ), TAG_MODEL );
		memcpy( overlay.indexes, overlayIndexes.Ptr(), numIndexes * sizeof( overlay.indexes[0] ) );
		overlay.numVerts = numVerts;
		overlay.verts = ( overlayVertex_t* )Mem_Alloc( numVerts * sizeof( overlay.verts[0] ), TAG_MODEL );
		memcpy( overlay.verts, overlayVerts.Ptr(), numVerts * sizeof( overlay.verts[0] ) );
		overlay.maxReferencedVertex = maxReferencedVertex;
		overlay.writtenToDemo = false;

		if( nextOverlay - firstOverlay > MAX_OVERLAYS )
		{
			firstOverlay = nextOverlay - MAX_OVERLAYS;
		}
	}
}

/*
====================
idRenderModelOverlay::CreateDeferredOverlays
====================
*/
void idRenderModelOverlay::CreateDeferredOverlays( const idRenderModel* model )
{
	for( unsigned int i = firstDeferredOverlay; i < nextDeferredOverlay; i++ )
	{
		const overlayProjectionParms_t& parms = deferredOverlays[i & ( MAX_DEFERRED_OVERLAYS - 1 )];
		if( parms.startTime > tr.viewDef->renderView.time[0] -  DEFFERED_OVERLAY_TIMEOUT )
		{
			CreateOverlay( model, parms.localTextureAxis, parms.material );
		}
	}
	firstDeferredOverlay = 0;
	nextDeferredOverlay = 0;
}

/*
====================
idRenderModelOverlay::AddDeferredOverlay
====================
*/
void idRenderModelOverlay::AddDeferredOverlay( const overlayProjectionParms_t& localParms )
{
	deferredOverlays[nextDeferredOverlay++ & ( MAX_DEFERRED_OVERLAYS - 1 )] = localParms;
	if( nextDeferredOverlay - firstDeferredOverlay > MAX_DEFERRED_OVERLAYS )
	{
		firstDeferredOverlay = nextDeferredOverlay - MAX_DEFERRED_OVERLAYS;
	}
}

/*
====================
R_CopyOverlaySurface
====================
*/
static void R_CopyOverlaySurface( idDrawVert* verts, int numVerts, triIndex_t* indexes, int numIndexes, const overlay_t* overlay, const idDrawVert* sourceVerts )
{
	assert_16_byte_aligned( &verts[numVerts] );
	assert_16_byte_aligned( &indexes[numIndexes] );
	assert_16_byte_aligned( overlay->verts );
	assert_16_byte_aligned( overlay->indexes );
	assert( ( ( overlay->numVerts * sizeof( idDrawVert ) ) & 15 ) == 0 );
	assert( ( ( overlay->numIndexes * sizeof( triIndex_t ) ) & 15 ) == 0 );

#if defined(USE_INTRINSICS_SSE)

	const __m128i vector_int_clear_last = _mm_set_epi32( 0, -1, -1, -1 );
	const __m128i vector_int_num_verts = _mm_shuffle_epi32( _mm_cvtsi32_si128( numVerts ), 0 );
	const __m128i vector_short_num_verts = _mm_packs_epi32( vector_int_num_verts, vector_int_num_verts );

	// copy vertices
	for( int i = 0; i < overlay->numVerts; i++ )
	{
		const overlayVertex_t& overlayVert = overlay->verts[i];
		const idDrawVert& srcVert = sourceVerts[overlayVert.vertexNum];
		idDrawVert& dstVert = verts[numVerts + i];

		__m128i v0 = _mm_load_si128( ( const __m128i* )( ( byte* )&srcVert +  0 ) );
		__m128i v1 = _mm_load_si128( ( const __m128i* )( ( byte* )&srcVert + 16 ) );
		__m128i st = _mm_cvtsi32_si128( *( unsigned int* )overlayVert.st );

		st = _mm_shuffle_epi32( st, _MM_SHUFFLE( 0, 1, 2, 3 ) );
		v0 = _mm_and_si128( v0, vector_int_clear_last );
		v0 = _mm_or_si128( v0, st );

		_mm_stream_si128( ( __m128i* )( ( byte* )&dstVert +  0 ), v0 );
		_mm_stream_si128( ( __m128i* )( ( byte* )&dstVert + 16 ), v1 );
	}

	// copy indexes
	assert( ( overlay->numIndexes & 7 ) == 0 );
	assert( sizeof( triIndex_t ) == 2 );
	for( int i = 0; i < overlay->numIndexes; i += 8 )
	{
		__m128i vi = _mm_load_si128( ( const __m128i* )&overlay->indexes[i] );

		vi = _mm_add_epi16( vi, vector_short_num_verts );

		_mm_stream_si128( ( __m128i* )&indexes[numIndexes + i], vi );
	}

	_mm_sfence();

#else

	// copy vertices
	for( int i = 0; i < overlay->numVerts; i++ )
	{
		const overlayVertex_t& overlayVert = overlay->verts[i];

		// NOTE: bad out-of-order write-combined write, SIMD code does the right thing
		verts[numVerts + i] = sourceVerts[overlayVert.vertexNum];

		// RB begin
		verts[numVerts + i].SetTexCoordNative( overlayVert.st[0], overlayVert.st[1] );
		// RB end
	}

	// copy indexes
	for( int i = 0; i < overlay->numIndexes; i += 2 )
	{
		assert( overlay->indexes[i + 0] < overlay->numVerts && overlay->indexes[i + 1] < overlay->numVerts );
		WriteIndexPair( &indexes[numIndexes + i], numVerts + overlay->indexes[i + 0], numVerts + overlay->indexes[i + 1] );
	}

#endif
}

/*
=====================
idRenderModelOverlay::GetNumOverlayDrawSurfs
=====================
*/
unsigned int idRenderModelOverlay::GetNumOverlayDrawSurfs()
{
	numOverlayMaterials = 0;

	for( unsigned int i = firstOverlay; i < nextOverlay; i++ )
	{
		const overlay_t& overlay = overlays[i & ( MAX_OVERLAYS - 1 )];

		unsigned int j = 0;
		for( ; j < numOverlayMaterials; j++ )
		{
			if( overlayMaterials[j] == overlay.material )
			{
				break;
			}
		}
		if( j >= numOverlayMaterials )
		{
			overlayMaterials[numOverlayMaterials++] = overlay.material;
		}
	}

	return numOverlayMaterials;
}

/*
====================
idRenderModelOverlay::CreateOverlayDrawSurf
====================
*/
drawSurf_t* idRenderModelOverlay::CreateOverlayDrawSurf( const viewEntity_t* space, const idRenderModel* baseModel, unsigned int index )
{
	if( index < 0 || index >= numOverlayMaterials )
	{
		return NULL;
	}

	// md5 models won't have any surfaces when r_showSkel is set
	if( baseModel == NULL || baseModel->IsDefaultModel() || baseModel->NumSurfaces() == 0 )
	{
		return NULL;
	}

	assert( baseModel->IsDynamicModel() == DM_STATIC );

	const idRenderModelStatic* staticModel = static_cast< const idRenderModelStatic* >( baseModel );

	const idMaterial* material = overlayMaterials[index];

	int maxVerts = 0;
	int maxIndexes = 0;
	for( unsigned int i = firstOverlay; i < nextOverlay; i++ )
	{
		const overlay_t& overlay = overlays[i & ( MAX_OVERLAYS - 1 )];
		if( overlay.material == material )
		{
			maxVerts += overlay.numVerts;
			maxIndexes += overlay.numIndexes;
		}
	}

	if( maxVerts == 0 || maxIndexes == 0 )
	{
		return NULL;
	}

	// create a new triangle surface in frame memory so it gets automatically disposed of
	srfTriangles_t* newTri = ( srfTriangles_t* )R_ClearedFrameAlloc( sizeof( *newTri ), FRAME_ALLOC_SURFACE_TRIANGLES );
	newTri->staticModelWithJoints = ( staticModel->jointsInverted != NULL ) ? const_cast< idRenderModelStatic* >( staticModel ) : NULL;	// allow GPU skinning

	newTri->ambientCache = vertexCache.AllocVertex( NULL, maxVerts );
	newTri->indexCache = vertexCache.AllocIndex( NULL, maxIndexes );

	idDrawVert* mappedVerts = ( idDrawVert* )vertexCache.MappedVertexBuffer( newTri->ambientCache );
	triIndex_t* mappedIndexes = ( triIndex_t* )vertexCache.MappedIndexBuffer( newTri->indexCache );

	int numVerts = 0;
	int numIndexes = 0;

	for( unsigned int i = firstOverlay; i < nextOverlay; i++ )
	{
		overlay_t& overlay = overlays[i & ( MAX_OVERLAYS - 1 )];

		if( overlay.numVerts == 0 )
		{
			if( i == firstOverlay )
			{
				firstOverlay++;
			}
			continue;
		}

		if( overlay.material != material )
		{
			continue;
		}

		// get the source model surface for this overlay surface
		const modelSurface_t* baseSurf = ( overlay.surfaceNum < staticModel->NumSurfaces() ) ? staticModel->Surface( overlay.surfaceNum ) : NULL;

		// if the surface ids no longer match
		if( baseSurf == NULL || baseSurf->id != overlay.surfaceId )
		{
			// find the surface with the correct id
			if( staticModel->FindSurfaceWithId( overlay.surfaceId, overlay.surfaceNum ) )
			{
				baseSurf = staticModel->Surface( overlay.surfaceNum );
			}
			else
			{
				// the surface with this id no longer exists
				FreeOverlay( overlay );
				if( i == firstOverlay )
				{
					firstOverlay++;
				}
				continue;
			}
		}

		// check for out of range vertex references
		const srfTriangles_t* baseTri = baseSurf->geometry;
		if( overlay.maxReferencedVertex >= baseTri->numVerts )
		{
			// This can happen when playing a demofile and a model has been changed since it was recorded, so just issue a warning and go on.
			common->Warning( "idRenderModelOverlay::CreateOverlayDrawSurf: overlay vertex out of range.  Model has probably changed since generating the overlay." );
			FreeOverlay( overlay );
			if( i == firstOverlay )
			{
				firstOverlay++;
			}
			continue;
		}

		// use SIMD optimized routine to copy the vertices and indices directly to write-combined memory
		R_CopyOverlaySurface( mappedVerts, numVerts, mappedIndexes, numIndexes, &overlay, baseTri->verts );

		numIndexes += overlay.numIndexes;
		numVerts += overlay.numVerts;
	}

	newTri->numVerts = numVerts;
	newTri->numIndexes = numIndexes;

	// create the drawsurf
	drawSurf_t* drawSurf = ( drawSurf_t* )R_FrameAlloc( sizeof( *drawSurf ), FRAME_ALLOC_DRAW_SURFACE );
	drawSurf->frontEndGeo = newTri;
	drawSurf->numIndexes = newTri->numIndexes;
	drawSurf->ambientCache = newTri->ambientCache;
	drawSurf->indexCache = newTri->indexCache;
	drawSurf->space = space;
	drawSurf->scissorRect = space->scissorRect;
	drawSurf->extraGLState = 0;

	R_SetupDrawSurfShader( drawSurf, material, &space->entityDef->parms );
	R_SetupDrawSurfJoints( drawSurf, newTri, NULL );

	return drawSurf;
}

/*
====================
idRenderModelOverlay::ReadFromDemoFile
====================
*/
void idRenderModelOverlay::ReadFromDemoFile( idDemoFile* f )
{
	f->ReadUnsignedInt( firstOverlay );
	f->ReadUnsignedInt( nextOverlay );

	for( unsigned int i = firstOverlay; i < nextOverlay; i++ )
	{
		overlay_t& overlay = overlays[ i & ( MAX_OVERLAYS - 1 ) ];

		bool overlayWritten = false;
		f->ReadBool( overlayWritten );
		if( !overlayWritten )
		{
			continue;
		}

		f->ReadInt( overlay.surfaceNum );
		f->ReadInt( overlay.surfaceId );
		f->ReadInt( overlay.maxReferencedVertex );

		const char* matName = f->ReadHashString();
		overlay.material = matName[ 0 ] ? declManager->FindMaterial( matName ) : NULL;

		int numVerts = 0;
		int numIndices = 0;

		f->ReadInt( numVerts );

		if( numVerts > 0 )
		{
			if( overlay.numVerts != numVerts )
			{
				Mem_Free( overlay.verts );
				overlay.numVerts = numVerts;
				overlay.verts = ( overlayVertex_t* )Mem_Alloc( overlay.numVerts * sizeof( overlayVertex_t ), TAG_MODEL );
			}

			f->Read( overlay.verts, sizeof( overlayVertex_t ) * overlay.numVerts );
		}

		f->ReadInt( numIndices );

		if( numIndices > 0 )
		{
			if( overlay.numIndexes != numIndices )
			{
				Mem_Free( overlay.indexes );
				overlay.numIndexes = numIndices;
				overlay.indexes = ( triIndex_t* )Mem_Alloc( overlay.numIndexes * sizeof( triIndex_t ), TAG_MODEL );
			}

			f->Read( overlay.indexes, sizeof( triIndex_t ) * overlay.numIndexes );
		}
	}
}

/*
====================
idRenderModelOverlay::WriteToDemoFile
====================
*/
void idRenderModelOverlay::WriteToDemoFile( idDemoFile* f ) const
{
	f->WriteUnsignedInt( firstOverlay );
	f->WriteUnsignedInt( nextOverlay );

	for( unsigned int i = firstOverlay; i < nextOverlay; i++ )
	{
		const overlay_t& overlay = overlays[ i & ( MAX_OVERLAYS - 1 ) ];

		if( overlay.writtenToDemo )
		{
			f->WriteBool( false );
			continue;
		}

		f->WriteBool( true );
		f->WriteInt( overlay.surfaceNum );
		f->WriteInt( overlay.surfaceId );
		f->WriteInt( overlay.maxReferencedVertex );
		f->WriteHashString( overlay.material ? overlay.material->GetName() : "" );

		f->WriteInt( overlay.numVerts );
		for( int j = 0; j < overlay.numVerts; j++ )
		{
			f->Write( &overlay.verts[ j ], sizeof( overlayVertex_t ) );
		}

		f->WriteInt( overlay.numIndexes );
		for( int j = 0; j < overlay.numIndexes; j++ )
		{
			f->Write( &overlay.indexes[ j ], sizeof( triIndex_t ) );
		}

		// so it won't be written again
		overlay.writtenToDemo = true;
	}
}
