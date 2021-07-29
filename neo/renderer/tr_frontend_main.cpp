/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2014 Robert Beckebans

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

/*
==========================================================================================

FRAME MEMORY ALLOCATION

==========================================================================================
*/

static const unsigned int FRAME_ALLOC_ALIGNMENT = 128;
static const unsigned int MAX_FRAME_MEMORY = 64 * 1024 * 1024;	// larger so that we can noclip on PC for dev purposes

idFrameData		smpFrameData[NUM_FRAME_DATA];
idFrameData* 	frameData;
unsigned int	smpFrame;

//#define TRACK_FRAME_ALLOCS

#if defined( TRACK_FRAME_ALLOCS )
	idSysInterlockedInteger frameAllocTypeCount[FRAME_ALLOC_MAX];
	int frameHighWaterTypeCount[FRAME_ALLOC_MAX];
#endif

/*
====================
R_ToggleSmpFrame
====================
*/
void R_ToggleSmpFrame()
{
	// update the highwater mark
	if( frameData->frameMemoryAllocated.GetValue() > frameData->highWaterAllocated )
	{
		frameData->highWaterAllocated = frameData->frameMemoryAllocated.GetValue();
#if defined( TRACK_FRAME_ALLOCS )
		frameData->highWaterUsed = frameData->frameMemoryUsed.GetValue();
		for( int i = 0; i < FRAME_ALLOC_MAX; i++ )
		{
			frameHighWaterTypeCount[i] = frameAllocTypeCount[i].GetValue();
		}
#endif
	}

	// switch to the next frame
	smpFrame++;
	frameData = &smpFrameData[smpFrame % NUM_FRAME_DATA];

	// reset the memory allocation

	// RB: 64 bit fixes, changed unsigned int to uintptr_t
	const uintptr_t bytesNeededForAlignment = FRAME_ALLOC_ALIGNMENT - ( ( uintptr_t )frameData->frameMemory & ( FRAME_ALLOC_ALIGNMENT - 1 ) );
	// RB end

	frameData->frameMemoryAllocated.SetValue( bytesNeededForAlignment );
	frameData->frameMemoryUsed.SetValue( 0 );

#if defined( TRACK_FRAME_ALLOCS )
	for( int i = 0; i < FRAME_ALLOC_MAX; i++ )
	{
		frameAllocTypeCount[i].SetValue( 0 );
	}
#endif

	// clear the command chain and make a RC_NOP command the only thing on the list
	frameData->cmdHead = frameData->cmdTail = ( emptyCommand_t* )R_FrameAlloc( sizeof( *frameData->cmdHead ), FRAME_ALLOC_DRAW_COMMAND );
	frameData->cmdHead->commandId = RC_NOP;
	frameData->cmdHead->next = NULL;
}

/*
=====================
R_ShutdownFrameData
=====================
*/
void R_ShutdownFrameData()
{
	frameData = NULL;
	for( int i = 0; i < NUM_FRAME_DATA; i++ )
	{
		Mem_Free16( smpFrameData[i].frameMemory );
		smpFrameData[i].frameMemory = NULL;
	}
}

/*
=====================
R_InitFrameData
=====================
*/
void R_InitFrameData()
{
	R_ShutdownFrameData();

	for( int i = 0; i < NUM_FRAME_DATA; i++ )
	{
		smpFrameData[i].frameMemory = ( byte* ) Mem_Alloc16( MAX_FRAME_MEMORY, TAG_RENDER );
	}

	// must be set before calling R_ToggleSmpFrame()
	frameData = &smpFrameData[ 0 ];

	R_ToggleSmpFrame();
}

/*
================
R_FrameAlloc

This data will be automatically freed when the
current frame's back end completes.

This should only be called by the front end.  The
back end shouldn't need to allocate memory.

All temporary data, like dynamic tesselations
and local spaces are allocated here.

All memory is cache-line-cleared for the best performance.
================
*/
void* R_FrameAlloc( int bytes, frameAllocType_t type )
{
#if defined( TRACK_FRAME_ALLOCS )
	frameData->frameMemoryUsed.Add( bytes );
	frameAllocTypeCount[type].Add( bytes );
#endif

	bytes = ( bytes + FRAME_ALLOC_ALIGNMENT - 1 ) & ~( FRAME_ALLOC_ALIGNMENT - 1 );

	// thread safe add
	int	end = frameData->frameMemoryAllocated.Add( bytes );
	if( end > MAX_FRAME_MEMORY )
	{
		idLib::Error( "R_FrameAlloc ran out of memory. bytes = %d, end = %d, highWaterAllocated = %d\n", bytes, end, frameData->highWaterAllocated );
	}

	byte* ptr = frameData->frameMemory + end - bytes;

	// cache line clear the memory
	for( int offset = 0; offset < bytes; offset += CACHE_LINE_SIZE )
	{
		ZeroCacheLine( ptr, offset );
	}

	return ptr;
}

/*
==================
R_ClearedFrameAlloc
==================
*/
void* R_ClearedFrameAlloc( int bytes, frameAllocType_t type )
{
	// NOTE: every allocation is cache line cleared
	return R_FrameAlloc( bytes, type );
}

/*
==========================================================================================

FONT-END STATIC MEMORY ALLOCATION

==========================================================================================
*/

/*
=================
R_StaticAlloc
=================
*/
void* R_StaticAlloc( int bytes, const memTag_t tag )
{
	tr.pc.c_alloc++;

	void* buf = Mem_Alloc( bytes, tag );

	// don't exit on failure on zero length allocations since the old code didn't
	if( buf == NULL && bytes != 0 )
	{
		common->FatalError( "R_StaticAlloc failed on %i bytes", bytes );
	}
	return buf;
}

/*
=================
R_ClearedStaticAlloc
=================
*/
void* R_ClearedStaticAlloc( int bytes )
{
	void* buf = R_StaticAlloc( bytes );
	memset( buf, 0, bytes );
	return buf;
}

/*
=================
R_StaticFree
=================
*/
void R_StaticFree( void* data )
{
	tr.pc.c_free++;
	Mem_Free( data );
}

/*
==========================================================================================

FONT-END RENDERING

==========================================================================================
*/

/*
=================
R_SortDrawSurfs
=================
*/
static void R_SortDrawSurfs( drawSurf_t** drawSurfs, const int numDrawSurfs )
{
#if 1

	uint64* indices = ( uint64* ) _alloca16( numDrawSurfs * sizeof( indices[0] ) );

	// sort the draw surfs based on:
	// 1. sort value (largest first)
	// 2. depth (smallest first)
	// 3. index (largest first)
	assert( numDrawSurfs <= 0xFFFF );
	for( int i = 0; i < numDrawSurfs; i++ )
	{
		float sort = SS_POST_PROCESS - drawSurfs[i]->sort;
		assert( sort >= 0.0f );

		uint64 dist = 0;
		if( drawSurfs[i]->frontEndGeo != NULL )
		{
			float min = 0.0f;
			float max = 1.0f;
			idRenderMatrix::DepthBoundsForBounds( min, max, drawSurfs[i]->space->mvp, drawSurfs[i]->frontEndGeo->bounds );
			dist = idMath::Ftoui16( min * 0xFFFF );
		}

		indices[i] = ( ( numDrawSurfs - i ) & 0xFFFF ) | ( dist << 16 ) | ( ( uint64 )( *( uint32* )&sort ) << 32 );
	}

	const int64 MAX_LEVELS = 128;
	int64 lo[MAX_LEVELS];
	int64 hi[MAX_LEVELS];

	// Keep the top of the stack in registers to avoid load-hit-stores.
	register int64 st_lo = 0;
	register int64 st_hi = numDrawSurfs - 1;
	register int64 level = 0;

	for( ; ; )
	{
		register int64 i = st_lo;
		register int64 j = st_hi;
		if( j - i >= 4 && level < MAX_LEVELS - 1 )
		{
			register uint64 pivot = indices[( i + j ) / 2];
			do
			{
				while( indices[i] > pivot )
				{
					i++;
				}
				while( indices[j] < pivot )
				{
					j--;
				}
				if( i > j )
				{
					break;
				}
				uint64 h = indices[i];
				indices[i] = indices[j];
				indices[j] = h;
			}
			while( ++i <= --j );

			// No need for these iterations because we are always sorting unique values.
			//while ( indices[j] == pivot && st_lo < j ) j--;
			//while ( indices[i] == pivot && i < st_hi ) i++;

			assert( level < MAX_LEVELS - 1 );
			lo[level] = i;
			hi[level] = st_hi;
			st_hi = j;
			level++;
		}
		else
		{
			for( ; i < j; j-- )
			{
				register int64 m = i;
				for( int64 k = i + 1; k <= j; k++ )
				{
					if( indices[k] < indices[m] )
					{
						m = k;
					}
				}
				uint64 h = indices[m];
				indices[m] = indices[j];
				indices[j] = h;
			}
			if( --level < 0 )
			{
				break;
			}
			st_lo = lo[level];
			st_hi = hi[level];
		}
	}

	drawSurf_t** newDrawSurfs = ( drawSurf_t** ) indices;
	for( int i = 0; i < numDrawSurfs; i++ )
	{
		newDrawSurfs[i] = drawSurfs[numDrawSurfs - ( indices[i] & 0xFFFF )];
	}
	memcpy( drawSurfs, newDrawSurfs, numDrawSurfs * sizeof( drawSurfs[0] ) );

#else

	struct local_t
	{
		static int R_QsortSurfaces( const void* a, const void* b )
		{
			const drawSurf_t* ea = *( drawSurf_t** )a;
			const drawSurf_t* eb = *( drawSurf_t** )b;
			if( ea->sort < eb->sort )
			{
				return -1;
			}
			if( ea->sort > eb->sort )
			{
				return 1;
			}
			return 0;
		}
	};

	// Add a sort offset so surfaces with equal sort orders still deterministically
	// draw in the order they were added, at least within a given model.
	float sorfOffset = 0.0f;
	for( int i = 0; i < numDrawSurfs; i++ )
	{
		drawSurf[i]->sort += sorfOffset;
		sorfOffset += 0.000001f;
	}

	// sort the drawsurfs
	qsort( drawSurfs, numDrawSurfs, sizeof( drawSurfs[0] ), local_t::R_QsortSurfaces );

#endif
}

// RB begin
static void R_SetupSplitFrustums( viewDef_t* viewDef )
{
	idVec3			planeOrigin;

	const float zNearStart = ( viewDef->renderView.cramZNear ) ? ( r_znear.GetFloat() * 0.25f ) : r_znear.GetFloat();
	float zFarEnd = 10000;

	float zNear = zNearStart;
	float zFar = zFarEnd;

	float lambda = r_shadowMapSplitWeight.GetFloat();
	float ratio = zFarEnd / zNearStart;

	for( int i = 0; i < 6; i++ )
	{
		tr.viewDef->frustumSplitDistances[i] = idMath::INFINITUM;
	}

	for( int i = 1; i <= ( r_shadowMapSplits.GetInteger() + 1 ) && i < MAX_FRUSTUMS; i++ )
	{
		float si = i / ( float )( r_shadowMapSplits.GetInteger() + 1 );

		if( i > FRUSTUM_CASCADE1 )
		{
			zNear = zFar - ( zFar * 0.005f );
		}

		zFar = 1.005f * lambda * ( zNearStart * powf( ratio, si ) ) + ( 1 - lambda ) * ( zNearStart + ( zFarEnd - zNearStart ) * si );

		if( i <= r_shadowMapSplits.GetInteger() )
		{
			tr.viewDef->frustumSplitDistances[i - 1] = zFar;
		}

		float projectionMatrix[16];
		R_SetupProjectionMatrix2( tr.viewDef, zNear, zFar, projectionMatrix );

		// setup render matrices for faster culling
		idRenderMatrix projectionRenderMatrix;
		idRenderMatrix::Transpose( *( idRenderMatrix* )projectionMatrix, projectionRenderMatrix );
		idRenderMatrix viewRenderMatrix;
		idRenderMatrix::Transpose( *( idRenderMatrix* )tr.viewDef->worldSpace.modelViewMatrix, viewRenderMatrix );
		idRenderMatrix::Multiply( projectionRenderMatrix, viewRenderMatrix, tr.viewDef->frustumMVPs[i] );

		// the planes of the view frustum are needed for portal visibility culling
		idRenderMatrix::GetFrustumPlanes( tr.viewDef->frustums[i], tr.viewDef->frustumMVPs[i], false, true );

		// the DOOM 3 frustum planes point outside the frustum
		for( int j = 0; j < 6; j++ )
		{
			tr.viewDef->frustums[i][j] = - tr.viewDef->frustums[i][j];
		}

		// remove the Z-near to avoid portals from being near clipped
		if( i == FRUSTUM_CASCADE1 )
		{
			tr.viewDef->frustums[i][4][3] -= r_znear.GetFloat();
		}
	}
}

class idSort_CompareEnvprobe : public idSort_Quick< RenderEnvprobeLocal*, idSort_CompareEnvprobe >
{
	idVec3	viewOrigin;

public:
	idSort_CompareEnvprobe( const idVec3& origin )
	{
		viewOrigin = origin;
	}

	int Compare( RenderEnvprobeLocal* const& a, RenderEnvprobeLocal* const& b ) const
	{
		float adist = ( viewOrigin - a->parms.origin ).LengthSqr();
		float bdist = ( viewOrigin - b->parms.origin ).LengthSqr();

		if( adist < bdist )
		{
			return -1;
		}

		if( adist > bdist )
		{
			return 1;
		}

		return 0;
	}
};

static void R_FindClosestEnvironmentProbes()
{
	// set safe defaults
	tr.viewDef->globalProbeBounds.Clear();

	tr.viewDef->irradianceImage = globalImages->defaultUACIrradianceCube;
	tr.viewDef->radianceImageBlends.Set( 1, 0, 0, 0 );
	for( int i = 0; i < 3; i++ )
	{
		tr.viewDef->radianceImages[i] = globalImages->defaultUACRadianceCube;
	}

	// early out
	if( tr.viewDef->areaNum == -1 || tr.viewDef->isSubview )
	{
		return;
	}

	idList<RenderEnvprobeLocal*, TAG_RENDER_ENVPROBE> viewEnvprobes;
	for( int i = 0; i < tr.primaryWorld->envprobeDefs.Num(); i++ )
	{
		RenderEnvprobeLocal* vProbe = tr.primaryWorld->envprobeDefs[i];
		if( vProbe )
		{
			// check for being closed off behind a door
			if( r_useLightAreaCulling.GetBool() && vProbe->areaNum != -1 && !tr.viewDef->connectedAreas[ vProbe->areaNum ] )
			{
				continue;
			}

			viewEnvprobes.AddUnique( vProbe );
		}
	}

	if( viewEnvprobes.Num() == 0 )
	{
		return;
	}

	idVec3 testOrigin = tr.viewDef->renderView.vieworg;

	// sort by distance
	// RB: each Doom 3 level has ~50 - 150 probes so this should be ok for each frame
	viewEnvprobes.SortWithTemplate( idSort_CompareEnvprobe( testOrigin ) );

	RenderEnvprobeLocal* nearest = viewEnvprobes[0];
	tr.viewDef->globalProbeBounds = nearest->globalProbeBounds;

	if( !nearest->irradianceImage->IsDefaulted() )
	{
		tr.viewDef->irradianceImage = nearest->irradianceImage;
	}

	// form a triangle of the 3 closest probes
	idVec3 verts[3];
	for( int i = 0; i < 3; i++ )
	{
		verts[i] = viewEnvprobes[0]->parms.origin;
	}

	for( int i = 0; i < viewEnvprobes.Num() && i < 3; i++ )
	{
		RenderEnvprobeLocal* vProbe = viewEnvprobes[i];

		verts[i] = vProbe->parms.origin;
	}

	idVec3 closest = R_ClosestPointPointTriangle( testOrigin, verts[0], verts[1], verts[2] );
	idVec3 bary;

	// find the barycentric coordinates
	float denom = idWinding::TriangleArea( verts[0], verts[1], verts[2] );
	if( denom == 0 )
	{
		// all points at same location
		bary.Set( 1, 0, 0 );
	}
	else
	{
		float	a, b, c;

		a = idWinding::TriangleArea( closest, verts[1], verts[2] ) / denom;
		b = idWinding::TriangleArea( closest, verts[2], verts[0] ) / denom;
		c = idWinding::TriangleArea( closest, verts[0], verts[1] ) / denom;

		bary.Set( a, b, c );
	}

	tr.viewDef->radianceImageBlends.Set( bary.x, bary.y, bary.z, 0.0f );

	for( int i = 0; i < viewEnvprobes.Num() && i < 3; i++ )
	{
		if( !viewEnvprobes[i]->radianceImage->IsDefaulted() )
		{
			tr.viewDef->radianceImages[i] = viewEnvprobes[i]->radianceImage;
		}
	}
}
// RB end

/*
================
R_RenderView

A view may be either the actual camera view,
a mirror / remote location, or a 3D view on a gui surface.

Parms will typically be allocated with R_FrameAlloc
================
*/
void R_RenderView( viewDef_t* parms )
{
	// save view in case we are a subview
	viewDef_t* oldView = tr.viewDef;

	tr.viewDef = parms;

	// setup the matrix for world space to eye space
	R_SetupViewMatrix( tr.viewDef );

	// we need to set the projection matrix before doing
	// portal-to-screen scissor calculations
	R_SetupProjectionMatrix( tr.viewDef );

	// RB: we need a unprojection matrix to calculate the vertex position based on the depth image value
	// for some post process shaders
	R_SetupUnprojection( tr.viewDef );
	// RB end

	// setup render matrices for faster culling
	idRenderMatrix::Transpose( *( idRenderMatrix* )tr.viewDef->projectionMatrix, tr.viewDef->projectionRenderMatrix );
	idRenderMatrix viewRenderMatrix;
	idRenderMatrix::Transpose( *( idRenderMatrix* )tr.viewDef->worldSpace.modelViewMatrix, viewRenderMatrix );
	idRenderMatrix::Multiply( tr.viewDef->projectionRenderMatrix, viewRenderMatrix, tr.viewDef->worldSpace.mvp );

	// the planes of the view frustum are needed for portal visibility culling
	idRenderMatrix::GetFrustumPlanes( tr.viewDef->frustums[FRUSTUM_PRIMARY], tr.viewDef->worldSpace.mvp, false, true );

	// the DOOM 3 frustum planes point outside the frustum
	for( int i = 0; i < 6; i++ )
	{
		tr.viewDef->frustums[FRUSTUM_PRIMARY][i] = - tr.viewDef->frustums[FRUSTUM_PRIMARY][i];
	}
	// remove the Z-near to avoid portals from being near clipped
	tr.viewDef->frustums[FRUSTUM_PRIMARY][4][3] -= r_znear.GetFloat();

	// RB begin
	R_SetupSplitFrustums( tr.viewDef );
	// RB end

	// identify all the visible portal areas, and create view lights and view entities
	// for all the the entityDefs and lightDefs that are in the visible portal areas
	static_cast<idRenderWorldLocal*>( parms->renderWorld )->FindViewLightsAndEntities();

	// wait for any shadow volume jobs from the previous frame to finish
	tr.frontEndJobList->Wait();

	// make sure that interactions exist for all light / entity combinations that are visible
	// add any pre-generated light shadows, and calculate the light shader values
	R_AddLights();

	// adds ambient surfaces and create any necessary interaction surfaces to add to the light lists
	R_AddModels();

	// build up the GUIs on world surfaces
	R_AddInGameGuis( tr.viewDef->drawSurfs, tr.viewDef->numDrawSurfs );

	// any viewLight that didn't have visible surfaces can have it's shadows removed
	R_OptimizeViewLightsList();

	// sort all the ambient surfaces for translucency ordering
	R_SortDrawSurfs( tr.viewDef->drawSurfs, tr.viewDef->numDrawSurfs );

	// generate any subviews (mirrors, cameras, etc) before adding this view
	if( R_GenerateSubViews( tr.viewDef->drawSurfs, tr.viewDef->numDrawSurfs ) )
	{
		// if we are debugging subviews, allow the skipping of the main view draw
		if( r_subviewOnly.GetBool() )
		{
			return;
		}
	}

	// RB: find closest environment probes so we can interpolate between them in the ambient shaders
	R_FindClosestEnvironmentProbes();

	// write everything needed to the demo file
	if( common->WriteDemo() )
	{
		static_cast<idRenderWorldLocal*>( parms->renderWorld )->WriteVisibleDefs( tr.viewDef );
	}

	// add the rendering commands for this viewDef
	R_AddDrawViewCmd( parms, false );

	// restore view in case we are a subview
	tr.viewDef = oldView;
}

/*
================
R_RenderPostProcess

Because R_RenderView may be called by subviews we have to make sure the post process
pass happens after the active view and its subviews is done rendering.
================
*/
void R_RenderPostProcess( viewDef_t* parms )
{
	viewDef_t* oldView = tr.viewDef;

	if( !( parms->renderView.rdflags & RDF_IRRADIANCE ) )
	{
		R_AddDrawPostProcess( parms );
	}

	tr.viewDef = oldView;
}
