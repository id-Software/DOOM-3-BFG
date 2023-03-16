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

#include "precompiled.h"
#pragma hdrstop

#include "RenderCommon.h"

/*
===========================================================================

idInteraction implementation

===========================================================================
*/

/*
================
R_CalcInteractionFacing

Determines which triangles of the surface are facing towards the light origin.

The facing array should be allocated with one extra index than
the number of surface triangles, which will be used to handle dangling
edge silhouettes.
================
*/
void R_CalcInteractionFacing( const idRenderEntityLocal* ent, const srfTriangles_t* tri, const idRenderLightLocal* light, srfCullInfo_t& cullInfo )
{
	SCOPED_PROFILE_EVENT( "R_CalcInteractionFacing" );

	if( cullInfo.facing != NULL )
	{
		return;
	}

	idVec3 localLightOrigin;
	R_GlobalPointToLocal( ent->modelMatrix, light->globalLightOrigin, localLightOrigin );

	const int numFaces = tri->numIndexes / 3;
	cullInfo.facing = ( byte* ) R_StaticAlloc( ( numFaces + 1 ) * sizeof( cullInfo.facing[0] ), TAG_RENDER_INTERACTION );

	// exact geometric cull against face
	for( int i = 0, face = 0; i < tri->numIndexes; i += 3, face++ )
	{
		const idDrawVert& v0 = tri->verts[tri->indexes[i + 0]];
		const idDrawVert& v1 = tri->verts[tri->indexes[i + 1]];
		const idDrawVert& v2 = tri->verts[tri->indexes[i + 2]];

		const idPlane plane( v0.xyz, v1.xyz, v2.xyz );
		const float d = plane.Distance( localLightOrigin );

		cullInfo.facing[face] = ( d >= 0.0f );
	}
	cullInfo.facing[numFaces] = 1;	// for dangling edges to reference
}

/*
=====================
R_CalcInteractionCullBits

We want to cull a little on the sloppy side, because the pre-clipping
of geometry to the lights in dmap will give many cases that are right
at the border. We throw things out on the border, because if any one
vertex is clearly inside, the entire triangle will be accepted.
=====================
*/
void R_CalcInteractionCullBits( const idRenderEntityLocal* ent, const srfTriangles_t* tri, const idRenderLightLocal* light, srfCullInfo_t& cullInfo )
{
	SCOPED_PROFILE_EVENT( "R_CalcInteractionCullBits" );

	if( cullInfo.cullBits != NULL )
	{
		return;
	}

	idPlane frustumPlanes[6];
	idRenderMatrix::GetFrustumPlanes( frustumPlanes, light->baseLightProject, true, true );

	int frontBits = 0;

	// cull the triangle surface bounding box
	for( int i = 0; i < 6; i++ )
	{
		R_GlobalPlaneToLocal( ent->modelMatrix, frustumPlanes[i], cullInfo.localClipPlanes[i] );

		// get front bits for the whole surface
		if( tri->bounds.PlaneDistance( cullInfo.localClipPlanes[i] ) >= LIGHT_CLIP_EPSILON )
		{
			frontBits |= 1 << i;
		}
	}

	// if the surface is completely inside the light frustum
	if( frontBits == ( ( 1 << 6 ) - 1 ) )
	{
		cullInfo.cullBits = LIGHT_CULL_ALL_FRONT;
		return;
	}

	cullInfo.cullBits = ( byte* ) R_StaticAlloc( tri->numVerts * sizeof( cullInfo.cullBits[0] ), TAG_RENDER_INTERACTION );
	memset( cullInfo.cullBits, 0, tri->numVerts * sizeof( cullInfo.cullBits[0] ) );

	for( int i = 0; i < 6; i++ )
	{
		// if completely infront of this clipping plane
		if( frontBits & ( 1 << i ) )
		{
			continue;
		}
		for( int j = 0; j < tri->numVerts; j++ )
		{
			float d = cullInfo.localClipPlanes[i].Distance( tri->verts[j].xyz );
			cullInfo.cullBits[j] |= ( d < LIGHT_CLIP_EPSILON ) << i;
		}
	}
}

/*
================
R_FreeInteractionCullInfo
================
*/
void R_FreeInteractionCullInfo( srfCullInfo_t& cullInfo )
{
	if( cullInfo.facing != NULL )
	{
		R_StaticFree( cullInfo.facing );
		cullInfo.facing = NULL;
	}
	if( cullInfo.cullBits != NULL )
	{
		if( cullInfo.cullBits != LIGHT_CULL_ALL_FRONT )
		{
			R_StaticFree( cullInfo.cullBits );
		}
		cullInfo.cullBits = NULL;
	}
}

/*
====================
R_CreateInteractionLightTris

This is only used for the static interaction case, dynamic interactions
just draw everything and let the GPU deal with it.

The resulting surface will be a subset of the original triangles,
it will never clip triangles, but it may cull on a per-triangle basis.
====================
*/
static srfTriangles_t* R_CreateInteractionLightTris( const idRenderEntityLocal* ent,
		const srfTriangles_t* tri, const idRenderLightLocal* light,
		const idMaterial* shader )
{

	SCOPED_PROFILE_EVENT( "R_CreateInteractionLightTris" );

	int			i;
	int			numIndexes;
	triIndex_t*	indexes;
	srfTriangles_t*	newTri;
	int			c_backfaced;
	int			c_distance;
	idBounds	bounds;
	bool		includeBackFaces;
	int			faceNum;

	c_backfaced = 0;
	c_distance = 0;

	numIndexes = 0;
	indexes = NULL;

	// it is debatable if non-shadowing lights should light back faces. we aren't at the moment
	// RB: now we do with r_useHalfLambert, so don't cull back faces if we have smooth shadowing enabled
	if( r_lightAllBackFaces.GetBool() || light->lightShader->LightEffectsBackSides()
			|| shader->ReceivesLightingOnBackSides() || ent->parms.noSelfShadow || ent->parms.noShadow || r_usePBR.GetBool() || r_useHalfLambertLighting.GetInteger() )
	{
		includeBackFaces = true;
	}
	else
	{
		includeBackFaces = false;
	}

	// allocate a new surface for the lit triangles
	newTri = R_AllocStaticTriSurf();

	// save a reference to the original surface
	newTri->ambientSurface = const_cast<srfTriangles_t*>( tri );

	// the light surface references the verts of the ambient surface
	newTri->numVerts = tri->numVerts;
	R_ReferenceStaticTriSurfVerts( newTri, tri );

	// calculate cull information
	srfCullInfo_t cullInfo = {};

	if( !includeBackFaces )
	{
		R_CalcInteractionFacing( ent, tri, light, cullInfo );
	}
	R_CalcInteractionCullBits( ent, tri, light, cullInfo );

	// if the surface is completely inside the light frustum
	if( cullInfo.cullBits == LIGHT_CULL_ALL_FRONT )
	{

		// if we aren't self shadowing, let back facing triangles get
		// through so the smooth shaded bump maps light all the way around
		if( includeBackFaces )
		{

			// the whole surface is lit so the light surface just references the indexes of the ambient surface
			newTri->indexes = tri->indexes;
			newTri->indexCache = tri->indexCache;
//			R_ReferenceStaticTriSurfIndexes( newTri, tri );

			numIndexes = tri->numIndexes;
			bounds = tri->bounds;

		}
		else
		{

			// the light tris indexes are going to be a subset of the original indexes so we generally
			// allocate too much memory here but we decrease the memory block when the number of indexes is known
			R_AllocStaticTriSurfIndexes( newTri, tri->numIndexes );

			// back face cull the individual triangles
			indexes = newTri->indexes;
			const byte* facing = cullInfo.facing;
			for( faceNum = i = 0; i < tri->numIndexes; i += 3, faceNum++ )
			{
				if( !facing[ faceNum ] )
				{
					c_backfaced++;
					continue;
				}
				indexes[numIndexes + 0] = tri->indexes[i + 0];
				indexes[numIndexes + 1] = tri->indexes[i + 1];
				indexes[numIndexes + 2] = tri->indexes[i + 2];
				numIndexes += 3;
			}

			// get bounds for the surface
			SIMDProcessor->MinMax( bounds[0], bounds[1], tri->verts, indexes, numIndexes );

			// decrease the size of the memory block to the size of the number of used indexes
			newTri->numIndexes = numIndexes;
			R_ResizeStaticTriSurfIndexes( newTri, numIndexes );
		}

	}
	else
	{

		// the light tris indexes are going to be a subset of the original indexes so we generally
		// allocate too much memory here but we decrease the memory block when the number of indexes is known
		R_AllocStaticTriSurfIndexes( newTri, tri->numIndexes );

		// cull individual triangles
		indexes = newTri->indexes;
		const byte* facing = cullInfo.facing;
		const byte* cullBits = cullInfo.cullBits;
		for( faceNum = i = 0; i < tri->numIndexes; i += 3, faceNum++ )
		{
			int i1, i2, i3;

			// if we aren't self shadowing, let back facing triangles get
			// through so the smooth shaded bump maps light all the way around
			if( !includeBackFaces )
			{
				// back face cull
				if( !facing[ faceNum ] )
				{
					c_backfaced++;
					continue;
				}
			}

			i1 = tri->indexes[i + 0];
			i2 = tri->indexes[i + 1];
			i3 = tri->indexes[i + 2];

			// fast cull outside the frustum
			// if all three points are off one plane side, it definately isn't visible
			if( cullBits[i1] & cullBits[i2] & cullBits[i3] )
			{
				c_distance++;
				continue;
			}

			// add to the list
			indexes[numIndexes + 0] = i1;
			indexes[numIndexes + 1] = i2;
			indexes[numIndexes + 2] = i3;
			numIndexes += 3;
		}

		// get bounds for the surface
		SIMDProcessor->MinMax( bounds[0], bounds[1], tri->verts, indexes, numIndexes );

		// decrease the size of the memory block to the size of the number of used indexes
		newTri->numIndexes = numIndexes;
		R_ResizeStaticTriSurfIndexes( newTri, numIndexes );
	}

	// free the cull information when it's no longer needed
	R_FreeInteractionCullInfo( cullInfo );

	if( !numIndexes )
	{
		R_FreeStaticTriSurf( newTri );
		return NULL;
	}

	newTri->numIndexes = numIndexes;

	newTri->bounds = bounds;

	return newTri;
}

/*
===============
idInteraction::idInteraction
===============
*/
idInteraction::idInteraction()
{
	numSurfaces				= 0;
	surfaces				= NULL;
	entityDef				= NULL;
	lightDef				= NULL;
	lightNext				= NULL;
	lightPrev				= NULL;
	entityNext				= NULL;
	entityPrev				= NULL;
	staticInteraction		= false;
}

/*
===============
idInteraction::AllocAndLink
===============
*/
idInteraction* idInteraction::AllocAndLink( idRenderEntityLocal* edef, idRenderLightLocal* ldef )
{
	if( edef == NULL || ldef == NULL )
	{
		common->Error( "idInteraction::AllocAndLink: NULL parm" );
		return NULL;
	}

	idRenderWorldLocal* renderWorld = edef->world;

	idInteraction* interaction = renderWorld->interactionAllocator.Alloc();

	// link and initialize
	interaction->lightDef = ldef;
	interaction->entityDef = edef;

	interaction->numSurfaces = -1;		// not checked yet
	interaction->surfaces = NULL;

	// link at the start of the entity's list
	interaction->lightNext = ldef->firstInteraction;
	interaction->lightPrev = NULL;
	ldef->firstInteraction = interaction;
	if( interaction->lightNext != NULL )
	{
		interaction->lightNext->lightPrev = interaction;
	}
	else
	{
		ldef->lastInteraction = interaction;
	}

	// link at the start of the light's list
	interaction->entityNext = edef->firstInteraction;
	interaction->entityPrev = NULL;
	edef->firstInteraction = interaction;
	if( interaction->entityNext != NULL )
	{
		interaction->entityNext->entityPrev = interaction;
	}
	else
	{
		edef->lastInteraction = interaction;
	}

	// update the interaction table
	if( renderWorld->interactionTable != NULL )
	{
		int index = ldef->index * renderWorld->interactionTableWidth + edef->index;
		if( renderWorld->interactionTable[index] != NULL )
		{
			common->Error( "idInteraction::AllocAndLink: non NULL table entry" );
		}
		renderWorld->interactionTable[ index ] = interaction;
	}

	return interaction;
}

/*
===============
idInteraction::FreeSurfaces

Frees the surfaces, but leaves the interaction linked in, so it
will be regenerated automatically
===============
*/
void idInteraction::FreeSurfaces()
{
	// anything regenerated is no longer an optimized static version
	this->staticInteraction = false;

	if( this->surfaces != NULL )
	{
		R_StaticFree( this->surfaces );
		this->surfaces = NULL;
	}
	this->numSurfaces = -1;
}

/*
===============
idInteraction::Unlink
===============
*/
void idInteraction::Unlink()
{

	// unlink from the entity's list
	if( this->entityPrev )
	{
		this->entityPrev->entityNext = this->entityNext;
	}
	else
	{
		this->entityDef->firstInteraction = this->entityNext;
	}
	if( this->entityNext )
	{
		this->entityNext->entityPrev = this->entityPrev;
	}
	else
	{
		this->entityDef->lastInteraction = this->entityPrev;
	}
	this->entityNext = this->entityPrev = NULL;

	// unlink from the light's list
	if( this->lightPrev )
	{
		this->lightPrev->lightNext = this->lightNext;
	}
	else
	{
		this->lightDef->firstInteraction = this->lightNext;
	}
	if( this->lightNext )
	{
		this->lightNext->lightPrev = this->lightPrev;
	}
	else
	{
		this->lightDef->lastInteraction = this->lightPrev;
	}
	this->lightNext = this->lightPrev = NULL;
}

/*
===============
idInteraction::UnlinkAndFree

Removes links and puts it back on the free list.
===============
*/
void idInteraction::UnlinkAndFree()
{
	// clear the table pointer
	idRenderWorldLocal* renderWorld = this->lightDef->world;
	// RB: added check for NULL
	if( renderWorld->interactionTable != NULL )
	{
		int index = this->lightDef->index * renderWorld->interactionTableWidth + this->entityDef->index;
		if( renderWorld->interactionTable[index] != this && renderWorld->interactionTable[index] != INTERACTION_EMPTY )
		{
			common->Error( "idInteraction::UnlinkAndFree: interactionTable wasn't set" );
		}
		renderWorld->interactionTable[index] = NULL;
	}
	// RB end

	Unlink();

	FreeSurfaces();

	// put it back on the free list
	renderWorld->interactionAllocator.Free( this );
}

/*
===============
idInteraction::MakeEmpty

Relinks the interaction at the end of both the light and entity chains
and adds the INTERACTION_EMPTY marker to the interactionTable.

It is necessary to keep the empty interaction so when entities or lights move
they can set all the interactionTable values to NULL.
===============
*/
void idInteraction::MakeEmpty()
{
	// an empty interaction has no surfaces
	numSurfaces = 0;

	Unlink();

	// relink at the end of the entity's list
	this->entityNext = NULL;
	this->entityPrev = this->entityDef->lastInteraction;
	this->entityDef->lastInteraction = this;
	if( this->entityPrev )
	{
		this->entityPrev->entityNext = this;
	}
	else
	{
		this->entityDef->firstInteraction = this;
	}

	// relink at the end of the light's list
	this->lightNext = NULL;
	this->lightPrev = this->lightDef->lastInteraction;
	this->lightDef->lastInteraction = this;
	if( this->lightPrev )
	{
		this->lightPrev->lightNext = this;
	}
	else
	{
		this->lightDef->firstInteraction = this;
	}

	// store the special marker in the interaction table
	const int interactionIndex = lightDef->index * entityDef->world->interactionTableWidth + entityDef->index;
	assert( entityDef->world->interactionTable[ interactionIndex ] == this );
	entityDef->world->interactionTable[ interactionIndex ] = INTERACTION_EMPTY;
}

/*
===============
idInteraction::HasShadows
===============
*/
bool idInteraction::HasShadows() const
{
	return !entityDef->parms.noShadow && lightDef->LightCastsShadows();
}

/*
======================
CreateStaticInteraction

Called by idRenderWorldLocal::GenerateAllInteractions
======================
*/
void idInteraction::CreateStaticInteraction( nvrhi::ICommandList* commandList )
{
	// note that it is a static interaction
	staticInteraction = true;
	const idRenderModel* model = entityDef->parms.hModel;
	if( model == NULL || model->NumSurfaces() <= 0 || model->IsDynamicModel() != DM_STATIC )
	{
		MakeEmpty();
		return;
	}

	const idBounds bounds = model->Bounds( &entityDef->parms );

	// if it doesn't contact the light frustum, none of the surfaces will
	if( R_CullModelBoundsToLight( lightDef, bounds, entityDef->modelRenderMatrix ) )
	{
		MakeEmpty();
		return;
	}

	//
	// create slots for each of the model's surfaces
	//
	numSurfaces = model->NumSurfaces();
	surfaces = ( surfaceInteraction_t* )R_ClearedStaticAlloc( sizeof( *surfaces ) * numSurfaces );

	bool interactionGenerated = false;

	// check each surface in the model
	for( int c = 0 ; c < model->NumSurfaces() ; c++ )
	{
		const modelSurface_t* surf = model->Surface( c );
		const srfTriangles_t* tri = surf->geometry;
		if( tri == NULL )
		{
			continue;
		}

		// determine the shader for this surface, possibly by skinning
		// Note that this will be wrong if customSkin/customShader are
		// changed after map load time without invalidating the interaction!
		const idMaterial* const shader = R_RemapShaderBySkin( surf->shader,
										 entityDef->parms.customSkin, entityDef->parms.customShader );
		if( shader == NULL )
		{
			continue;
		}

		// try to cull each surface
		if( R_CullModelBoundsToLight( lightDef, tri->bounds, entityDef->modelRenderMatrix ) )
		{
			continue;
		}

		surfaceInteraction_t* sint = &surfaces[c];

		// generate a set of indexes for the lit surfaces, culling away triangles that are
		// not at least partially inside the light
		if( shader->ReceivesLighting() )
		{
			srfTriangles_t* lightTris = R_CreateInteractionLightTris( entityDef, tri, lightDef, shader );
			if( lightTris != NULL )
			{
				// make a static index cache
				sint->numLightTrisIndexes = lightTris->numIndexes;
				sint->lightTrisIndexCache = vertexCache.AllocStaticIndex( lightTris->indexes, ALIGN( lightTris->numIndexes * sizeof( lightTris->indexes[0] ), INDEX_CACHE_ALIGN ), commandList );

				interactionGenerated = true;
				R_FreeStaticTriSurf( lightTris );
			}
		}
	}

	// if none of the surfaces generated anything, don't even bother checking?
	if( !interactionGenerated )
	{
		MakeEmpty();
	}
}

/*
===================
R_ShowInteractionMemory_f
===================
*/
void R_ShowInteractionMemory_f( const idCmdArgs& args )
{
	int entities = 0;
	int interactions = 0;
	int deferredInteractions = 0;
	int emptyInteractions = 0;
	int lightTris = 0;
	int lightTriIndexes = 0;
	int shadowTris = 0;
	int shadowTriIndexes = 0;
	int maxInteractionsForEntity = 0;
	int maxInteractionsForLight = 0;

	for( int i = 0; i < tr.primaryWorld->lightDefs.Num(); i++ )
	{
		idRenderLightLocal* light = tr.primaryWorld->lightDefs[i];
		if( light == NULL )
		{
			continue;
		}
		int numInteractionsForLight = 0;
		for( idInteraction* inter = light->firstInteraction; inter != NULL; inter = inter->lightNext )
		{
			if( !inter->IsEmpty() )
			{
				numInteractionsForLight++;
			}
		}
		if( numInteractionsForLight > maxInteractionsForLight )
		{
			maxInteractionsForLight = numInteractionsForLight;
		}
	}

	for( int i = 0; i < tr.primaryWorld->entityDefs.Num(); i++ )
	{
		idRenderEntityLocal*	def = tr.primaryWorld->entityDefs[i];
		if( def == NULL )
		{
			continue;
		}
		if( def->firstInteraction == NULL )
		{
			continue;
		}
		entities++;

		int numInteractionsForEntity = 0;
		for( idInteraction* inter = def->firstInteraction; inter != NULL; inter = inter->entityNext )
		{
			interactions++;

			if( !inter->IsEmpty() )
			{
				numInteractionsForEntity++;
			}

			if( inter->IsDeferred() )
			{
				deferredInteractions++;
				continue;
			}
			if( inter->IsEmpty() )
			{
				emptyInteractions++;
				continue;
			}

			for( int j = 0; j < inter->numSurfaces; j++ )
			{
				surfaceInteraction_t* srf = &inter->surfaces[j];

				if( srf->numLightTrisIndexes )
				{
					lightTris++;
					lightTriIndexes += srf->numLightTrisIndexes;
				}
			}
		}
		if( numInteractionsForEntity > maxInteractionsForEntity )
		{
			maxInteractionsForEntity = numInteractionsForEntity;
		}
	}

	common->Printf( "%i entities with %i total interactions\n", entities, interactions );
	common->Printf( "%i deferred interactions, %i empty interactions\n", deferredInteractions, emptyInteractions );
	common->Printf( "%5i indexes in %5i light tris\n", lightTriIndexes, lightTris );
	common->Printf( "%5i indexes in %5i shadow tris\n", shadowTriIndexes, shadowTris );
	common->Printf( "%i maxInteractionsForEntity\n", maxInteractionsForEntity );
	common->Printf( "%i maxInteractionsForLight\n", maxInteractionsForLight );
}
