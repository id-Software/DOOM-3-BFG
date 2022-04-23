/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2014-2016 Robert Beckebans
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

#if defined( USE_NVRHI )
	#include <sys/DeviceManager.h>
	extern DeviceManager* deviceManager;
#endif

/*
===================
R_ListRenderLightDefs_f
===================
*/
void R_ListRenderLightDefs_f( const idCmdArgs& args )
{
	int			i;
	idRenderLightLocal*	ldef;

	if( !tr.primaryWorld )
	{
		return;
	}
	int active = 0;
	int	totalRef = 0;
	int	totalIntr = 0;

	for( i = 0; i < tr.primaryWorld->lightDefs.Num(); i++ )
	{
		ldef = tr.primaryWorld->lightDefs[i];
		if( !ldef )
		{
			common->Printf( "%4i: FREED\n", i );
			continue;
		}

		// count up the interactions
		int	iCount = 0;
		for( idInteraction* inter = ldef->firstInteraction; inter != NULL; inter = inter->lightNext )
		{
			iCount++;
		}
		totalIntr += iCount;

		// count up the references
		int	rCount = 0;
		for( areaReference_t* ref = ldef->references; ref; ref = ref->ownerNext )
		{
			rCount++;
		}
		totalRef += rCount;

		common->Printf( "%4i: %3i intr %2i refs %s\n", i, iCount, rCount, ldef->lightShader->GetName() );
		active++;
	}

	common->Printf( "%i lightDefs, %i interactions, %i areaRefs\n", active, totalIntr, totalRef );
}

/*
===================
R_ListRenderEntityDefs_f
===================
*/
void R_ListRenderEntityDefs_f( const idCmdArgs& args )
{
	int			i;
	idRenderEntityLocal*	mdef;

	if( !tr.primaryWorld )
	{
		return;
	}
	int active = 0;
	int	totalRef = 0;
	int	totalIntr = 0;

	for( i = 0; i < tr.primaryWorld->entityDefs.Num(); i++ )
	{
		mdef = tr.primaryWorld->entityDefs[i];
		if( !mdef )
		{
			common->Printf( "%4i: FREED\n", i );
			continue;
		}

		// count up the interactions
		int	iCount = 0;
		for( idInteraction* inter = mdef->firstInteraction; inter != NULL; inter = inter->entityNext )
		{
			iCount++;
		}
		totalIntr += iCount;

		// count up the references
		int	rCount = 0;
		for( areaReference_t* ref = mdef->entityRefs; ref; ref = ref->ownerNext )
		{
			rCount++;
		}
		totalRef += rCount;

		common->Printf( "%4i: %3i intr %2i refs %s\n", i, iCount, rCount, mdef->parms.hModel->Name() );
		active++;
	}

	common->Printf( "total active: %i\n", active );
}

/*
===================
idRenderWorldLocal::idRenderWorldLocal
===================
*/
idRenderWorldLocal::idRenderWorldLocal()
{
	mapName.Clear();
	mapTimeStamp = FILE_NOT_FOUND_TIMESTAMP;

	generateAllInteractionsCalled = false;

	areaNodes = NULL;
	numAreaNodes = 0;

	portalAreas = NULL;
	numPortalAreas = 0;

	doublePortals = NULL;
	numInterAreaPortals = 0;

	interactionTable = 0;
	interactionTableWidth = 0;
	interactionTableHeight = 0;

	for( int i = 0; i < decals.Num(); i++ )
	{
		decals[i].entityHandle = -1;
		decals[i].lastStartTime = 0;
		decals[i].decals = new( TAG_MODEL ) idRenderModelDecal();
		decals[i].decals->index = i;
	}

	for( int i = 0; i < overlays.Num(); i++ )
	{
		overlays[i].entityHandle = -1;
		overlays[i].lastStartTime = 0;
		overlays[i].overlays = new( TAG_MODEL ) idRenderModelOverlay();
		overlays[ i ].overlays->index = i;
	}
}

/*
===================
idRenderWorldLocal::~idRenderWorldLocal
===================
*/
idRenderWorldLocal::~idRenderWorldLocal()
{
	// free all the entityDefs, lightDefs, portals, etc
	FreeWorld();

	for( int i = 0; i < decals.Num(); i++ )
	{
		delete decals[i].decals;
	}

	for( int i = 0; i < overlays.Num(); i++ )
	{
		delete overlays[i].overlays;
	}

	// free up the debug lines, polys, and text
	RB_ClearDebugPolygons( 0 );
	RB_ClearDebugLines( 0 );
	RB_ClearDebugText( 0 );
}

/*
===================
ResizeInteractionTable
===================
*/
void idRenderWorldLocal::ResizeInteractionTable()
{
	// we overflowed the interaction table, so make it larger
	common->Printf( "idRenderWorldLocal::ResizeInteractionTable: overflowed interactionTable, resizing\n" );

	const int oldInteractionTableWidth = interactionTableWidth;
	const int oldIinteractionTableHeight = interactionTableHeight;
	idInteraction** oldInteractionTable = interactionTable;

	// build the interaction table
	// this will be dynamically resized if the entity / light counts grow too much
	interactionTableWidth = entityDefs.Num() + 100;
	interactionTableHeight = lightDefs.Num() + 100;
	const int	size =  interactionTableWidth * interactionTableHeight * sizeof( *interactionTable );
	interactionTable = ( idInteraction** )R_ClearedStaticAlloc( size );
	for( int l = 0; l < oldIinteractionTableHeight; l++ )
	{
		for( int e = 0; e < oldInteractionTableWidth; e++ )
		{
			interactionTable[ l * interactionTableWidth + e ] = oldInteractionTable[ l * oldInteractionTableWidth + e ];
		}
	}

	R_StaticFree( oldInteractionTable );
}

/*
===================
AddEntityDef
===================
*/
qhandle_t idRenderWorldLocal::AddEntityDef( const renderEntity_t* re )
{
	// try and reuse a free spot
	int entityHandle = entityDefs.FindNull();
	if( entityHandle == -1 )
	{
		entityHandle = entityDefs.Append( NULL );

		if( interactionTable && entityDefs.Num() > interactionTableWidth )
		{
			ResizeInteractionTable();
		}
	}

	UpdateEntityDef( entityHandle, re );

	return entityHandle;
}

/*
==============
UpdateEntityDef

Does not write to the demo file, which will only be updated for
visible entities
==============
*/
int c_callbackUpdate;

void idRenderWorldLocal::UpdateEntityDef( qhandle_t entityHandle, const renderEntity_t* re )
{
	if( r_skipUpdates.GetBool() )
	{
		return;
	}

	tr.pc.c_entityUpdates++;

	if( !re->hModel && !re->callback )
	{
		common->Error( "idRenderWorld::UpdateEntityDef: NULL hModel" );
	}

	// create new slots if needed
	if( entityHandle < 0 || entityHandle > LUDICROUS_INDEX )
	{
		common->Error( "idRenderWorld::UpdateEntityDef: index = %i", entityHandle );
	}
	while( entityHandle >= entityDefs.Num() )
	{
		entityDefs.Append( NULL );
	}

	idRenderEntityLocal*	def = entityDefs[entityHandle];
	if( def != NULL )
	{

		if( !re->forceUpdate )
		{

			// check for exact match (OPTIMIZE: check through pointers more)
			if( !re->joints && !re->callbackData && !def->dynamicModel && !memcmp( re, &def->parms, sizeof( *re ) ) )
			{
				return;
			}

			// if the only thing that changed was shaderparms, we can just leave things as they are
			// after updating parms

			// if we have a callback function and the bounds, origin, axis and model match,
			// then we can leave the references as they are
			if( re->callback )
			{

				bool axisMatch = ( re->axis == def->parms.axis );
				bool originMatch = ( re->origin == def->parms.origin );
				bool boundsMatch = ( re->bounds == def->localReferenceBounds );
				bool modelMatch = ( re->hModel == def->parms.hModel );

				if( boundsMatch && originMatch && axisMatch && modelMatch )
				{
					// only clear the dynamic model and interaction surfaces if they exist
					c_callbackUpdate++;
					R_ClearEntityDefDynamicModel( def );
					def->parms = *re;
					return;
				}
			}
		}

		// save any decals if the model is the same, allowing marks to move with entities
		if( def->parms.hModel == re->hModel )
		{
			R_FreeEntityDefDerivedData( def, true, true );
		}
		else
		{
			R_FreeEntityDefDerivedData( def, false, false );
		}
	}
	else
	{
		// creating a new one
		def = new( TAG_RENDER_ENTITY ) idRenderEntityLocal;
		entityDefs[entityHandle] = def;

		def->world = this;
		def->index = entityHandle;
	}

	def->parms = *re;

	def->lastModifiedFrameNum = tr.frameCount;
	def->archived = false;

	// optionally immediately issue any callbacks
	if( !r_useEntityCallbacks.GetBool() && def->parms.callback != NULL )
	{
		R_IssueEntityDefCallback( def );
	}

	// trigger entities don't need to get linked in and processed,
	// they only exist for editor use
	if( def->parms.hModel != NULL && !def->parms.hModel->ModelHasDrawingSurfaces() )
	{
		return;
	}

	// based on the model bounds, add references in each area
	// that may contain the updated surface
	R_CreateEntityRefs( def );
}

/*
===================
FreeEntityDef

Frees all references and lit surfaces from the model, and
NULL's out it's entry in the world list
===================
*/
void idRenderWorldLocal::FreeEntityDef( qhandle_t entityHandle )
{
	idRenderEntityLocal*	def;

	if( entityHandle < 0 || entityHandle >= entityDefs.Num() )
	{
		common->Printf( "idRenderWorld::FreeEntityDef: handle %i > %i\n", entityHandle, entityDefs.Num() );
		return;
	}

	def = entityDefs[entityHandle];
	if( !def )
	{
		common->Printf( "idRenderWorld::FreeEntityDef: handle %i is NULL\n", entityHandle );
		return;
	}

	R_FreeEntityDefDerivedData( def, false, false );

	if( common->WriteDemo() && def->archived )
	{
		WriteFreeEntity( entityHandle );
	}

	// if we are playing a demo, these will have been freed
	// in R_FreeEntityDefDerivedData(), otherwise the gui
	// object still exists in the game

	def->parms.gui[ 0 ] = NULL;
	def->parms.gui[ 1 ] = NULL;
	def->parms.gui[ 2 ] = NULL;

	delete def;
	entityDefs[ entityHandle ] = NULL;
}

/*
==================
GetRenderEntity
==================
*/
const renderEntity_t* idRenderWorldLocal::GetRenderEntity( qhandle_t entityHandle ) const
{
	idRenderEntityLocal*	def;

	if( entityHandle < 0 || entityHandle >= entityDefs.Num() )
	{
		common->Printf( "idRenderWorld::GetRenderEntity: invalid handle %i [0, %i]\n", entityHandle, entityDefs.Num() );
		return NULL;
	}

	def = entityDefs[entityHandle];
	if( !def )
	{
		common->Printf( "idRenderWorld::GetRenderEntity: handle %i is NULL\n", entityHandle );
		return NULL;
	}

	return &def->parms;
}

/*
==================
AddLightDef
==================
*/
qhandle_t idRenderWorldLocal::AddLightDef( const renderLight_t* rlight )
{
	// try and reuse a free spot
	int lightHandle = lightDefs.FindNull();

	if( lightHandle == -1 )
	{
		lightHandle = lightDefs.Append( NULL );
		if( interactionTable && lightDefs.Num() > interactionTableHeight )
		{
			ResizeInteractionTable();
		}
	}
	UpdateLightDef( lightHandle, rlight );

	return lightHandle;
}

/*
=================
UpdateLightDef

The generation of all the derived interaction data will
usually be deferred until it is visible in a scene

Does not write to the demo file, which will only be done for visible lights
=================
*/
void idRenderWorldLocal::UpdateLightDef( qhandle_t lightHandle, const renderLight_t* rlight )
{
	if( r_skipUpdates.GetBool() )
	{
		return;
	}

	tr.pc.c_lightUpdates++;

	// create new slots if needed
	if( lightHandle < 0 || lightHandle > LUDICROUS_INDEX )
	{
		common->Error( "idRenderWorld::UpdateLightDef: index = %i", lightHandle );
	}
	while( lightHandle >= lightDefs.Num() )
	{
		lightDefs.Append( NULL );
	}

	bool justUpdate = false;
	idRenderLightLocal* light = lightDefs[lightHandle];
	if( light )
	{
		// if the shape of the light stays the same, we don't need to dump
		// any of our derived data, because shader parms are calculated every frame
		if( rlight->axis == light->parms.axis && rlight->end == light->parms.end &&
				rlight->lightCenter == light->parms.lightCenter && rlight->lightRadius == light->parms.lightRadius &&
				rlight->noShadows == light->parms.noShadows && rlight->origin == light->parms.origin &&
				rlight->parallel == light->parms.parallel && rlight->pointLight == light->parms.pointLight &&
				rlight->right == light->parms.right && rlight->start == light->parms.start &&
				rlight->target == light->parms.target && rlight->up == light->parms.up &&
				rlight->shader == light->lightShader && rlight->prelightModel == light->parms.prelightModel )
		{
			justUpdate = true;
		}
		else
		{
			// if we are updating shadows, the prelight model is no longer valid
			light->lightHasMoved = true;
			R_FreeLightDefDerivedData( light );
		}
	}
	else
	{
		// create a new one
		light = new( TAG_RENDER_LIGHT ) idRenderLightLocal;
		lightDefs[lightHandle] = light;

		light->world = this;
		light->index = lightHandle;
	}

	light->parms = *rlight;
	light->lastModifiedFrameNum = tr.frameCount;
	if( common->WriteDemo() && light->archived )
	{
		WriteFreeLight( lightHandle );
		light->archived = false;
	}

	// new for BFG edition: force noShadows on spectrum lights so teleport spawns
	// don't cause such a slowdown.  Hell writing shouldn't be shadowed anyway...
	if( light->parms.shader && light->parms.shader->Spectrum() )
	{
		light->parms.noShadows = true;
	}

	if( light->lightHasMoved )
	{
		light->parms.prelightModel = NULL;
	}

	if( !justUpdate )
	{
		R_CreateLightRefs( light );
	}
}

/*
====================
FreeLightDef

Frees all references and lit surfaces from the light, and
NULL's out it's entry in the world list
====================
*/
void idRenderWorldLocal::FreeLightDef( qhandle_t lightHandle )
{
	idRenderLightLocal*	light;

	if( lightHandle < 0 || lightHandle >= lightDefs.Num() )
	{
		common->Printf( "idRenderWorld::FreeLightDef: invalid handle %i [0, %i]\n", lightHandle, lightDefs.Num() );
		return;
	}

	light = lightDefs[lightHandle];
	if( !light )
	{
		common->Printf( "idRenderWorld::FreeLightDef: handle %i is NULL\n", lightHandle );
		return;
	}

	R_FreeLightDefDerivedData( light );

	if( common->WriteDemo() && light->archived )
	{
		WriteFreeLight( lightHandle );
	}

	delete light;
	lightDefs[lightHandle] = NULL;
}

/*
==================
GetRenderLight
==================
*/
const renderLight_t* idRenderWorldLocal::GetRenderLight( qhandle_t lightHandle ) const
{
	idRenderLightLocal* def;

	if( lightHandle < 0 || lightHandle >= lightDefs.Num() )
	{
		common->Printf( "idRenderWorld::GetRenderLight: handle %i > %i\n", lightHandle, lightDefs.Num() );
		return NULL;
	}

	def = lightDefs[lightHandle];
	if( !def )
	{
		common->Printf( "idRenderWorld::GetRenderLight: handle %i is NULL\n", lightHandle );
		return NULL;
	}

	return &def->parms;
}


// RB begin
qhandle_t idRenderWorldLocal::AddEnvprobeDef( const renderEnvironmentProbe_t* ep )
{
	// try and reuse a free spot
	int envprobeHandle = envprobeDefs.FindNull();

	if( envprobeHandle == -1 )
	{
		envprobeHandle = envprobeDefs.Append( NULL );

		// TODO
		//if( interactionTable && envprobeDefs.Num() > interactionTableHeight )
		//{
		//	ResizeEnvprobeInteractionTable();
		//}
	}

	UpdateEnvprobeDef( envprobeHandle, ep );

	return envprobeHandle;
}

/*
=================
UpdateEnvprobeDef

The generation of all the derived interaction data will
usually be deferred until it is visible in a scene

Does not write to the demo file, which will only be done for visible lights
=================
*/
void idRenderWorldLocal::UpdateEnvprobeDef( qhandle_t envprobeHandle, const renderEnvironmentProbe_t* ep )
{
	if( r_skipUpdates.GetBool() )
	{
		return;
	}

	tr.pc.c_envprobeUpdates++;

	// create new slots if needed
	if( envprobeHandle < 0 || envprobeHandle > LUDICROUS_INDEX )
	{
		common->Error( "idRenderWorld::UpdateEnvprobeDef: index = %i", envprobeHandle );
	}
	while( envprobeHandle >= envprobeDefs.Num() )
	{
		envprobeDefs.Append( NULL );
	}

	bool justUpdate = false;
	RenderEnvprobeLocal* probe = envprobeDefs[envprobeHandle];
	if( probe )
	{
		// if the shape of the envprobe stays the same, we don't need to dump
		// any of our derived data, because shader parms are calculated every frame
		if( ep->origin == probe->parms.origin )
		{
			justUpdate = true;
		}
		else
		{
			probe->envprobeHasMoved = true;
			R_FreeEnvprobeDefDerivedData( probe );
		}
	}
	else
	{
		// create a new one
		probe = new( TAG_RENDER_LIGHT ) RenderEnvprobeLocal;
		envprobeDefs[envprobeHandle] = probe;

		probe->world = this;
		probe->index = envprobeHandle;
	}

	probe->parms = *ep;
	probe->lastModifiedFrameNum = tr.frameCount;
	if( common->WriteDemo() && probe->archived )
	{
		WriteFreeEnvprobe( envprobeHandle );
		probe->archived = false;
	}

	if( !justUpdate )
	{
		R_CreateEnvprobeRefs( probe );
	}
}

/*
====================
FreeEnvprobeDef

Frees all references and lit surfaces from the light, and
NULL's out it's entry in the world list
====================
*/
void idRenderWorldLocal::FreeEnvprobeDef( qhandle_t envprobeHandle )
{
	RenderEnvprobeLocal*	probe;

	if( envprobeHandle < 0 || envprobeHandle >= envprobeDefs.Num() )
	{
		common->Printf( "idRenderWorld::FreeEnvprobeDef: invalid handle %i [0, %i]\n", envprobeHandle, envprobeDefs.Num() );
		return;
	}

	probe = envprobeDefs[envprobeHandle];
	if( !probe )
	{
		common->Printf( "idRenderWorld::FreeEnvprobeDef: handle %i is NULL\n", envprobeHandle );
		return;
	}

	R_FreeEnvprobeDefDerivedData( probe );

	if( common->WriteDemo() && probe->archived )
	{
		WriteFreeEnvprobe( envprobeHandle );
	}

	delete probe;
	envprobeDefs[envprobeHandle] = NULL;
}

const renderEnvironmentProbe_t* idRenderWorldLocal::GetRenderEnvprobe( qhandle_t envprobeHandle ) const
{
	RenderEnvprobeLocal* def;

	if( envprobeHandle < 0 || envprobeHandle >= envprobeDefs.Num() )
	{
		common->Printf( "idRenderWorld::GetRenderEnvprobe: handle %i > %i\n", envprobeHandle, envprobeDefs.Num() );
		return NULL;
	}

	def = envprobeDefs[envprobeHandle];
	if( !def )
	{
		common->Printf( "idRenderWorld::GetRenderEnvprobe: handle %i is NULL\n", envprobeHandle );
		return NULL;
	}

	return &def->parms;
}
// RB end

/*
================
idRenderWorldLocal::ProjectDecalOntoWorld
================
*/
void idRenderWorldLocal::ProjectDecalOntoWorld( const idFixedWinding& winding, const idVec3& projectionOrigin, const bool parallel, const float fadeDepth, const idMaterial* material, const int startTime )
{
	decalProjectionParms_t globalParms;

	if( !idRenderModelDecal::CreateProjectionParms( globalParms, winding, projectionOrigin, parallel, fadeDepth, material, startTime ) )
	{
		return;
	}

	// get the world areas touched by the projection volume
	int areas[10];
	int numAreas = BoundsInAreas( globalParms.projectionBounds, areas, 10 );

	// check all areas for models
	for( int i = 0; i < numAreas; i++ )
	{

		const portalArea_t* area = &portalAreas[ areas[i] ];

		// check all models in this area
		for( const areaReference_t* ref = area->entityRefs.areaNext; ref != &area->entityRefs; ref = ref->areaNext )
		{
			idRenderEntityLocal* def = ref->entity;

			if( def->parms.noOverlays )
			{
				continue;
			}

			if( def->parms.customShader != NULL && !def->parms.customShader->AllowOverlays() )
			{
				continue;
			}

			// completely ignore any dynamic or callback models
			const idRenderModel* model = def->parms.hModel;
			if( def->parms.callback != NULL || model == NULL || model->IsDynamicModel() != DM_STATIC )
			{
				continue;
			}

			idBounds bounds;
			bounds.FromTransformedBounds( model->Bounds( &def->parms ), def->parms.origin, def->parms.axis );

			// if the model bounds do not overlap with the projection bounds
			decalProjectionParms_t localParms;
			if( !globalParms.projectionBounds.IntersectsBounds( bounds ) )
			{
				continue;
			}

			// transform the bounding planes, fade planes and texture axis into local space
			idRenderModelDecal::GlobalProjectionParmsToLocal( localParms, globalParms, def->parms.origin, def->parms.axis );
			localParms.force = ( def->parms.customShader != NULL );

			if( def->decals == NULL )
			{
				def->decals = AllocDecal( def->index, startTime );
			}
			def->decals->AddDeferredDecal( localParms );
			def->archived = false;
		}
	}
}

/*
====================
idRenderWorldLocal::ProjectDecal
====================
*/
void idRenderWorldLocal::ProjectDecal( qhandle_t entityHandle, const idFixedWinding& winding, const idVec3& projectionOrigin, const bool parallel, const float fadeDepth, const idMaterial* material, const int startTime )
{
	if( entityHandle < 0 || entityHandle >= entityDefs.Num() )
	{
		common->Error( "idRenderWorld::ProjectOverlay: index = %i", entityHandle );
		return;
	}

	idRenderEntityLocal*	def = entityDefs[ entityHandle ];
	if( def == NULL )
	{
		return;
	}

	const idRenderModel* model = def->parms.hModel;

	if( model == NULL || model->IsDynamicModel() != DM_STATIC || def->parms.callback != NULL )
	{
		return;
	}

	decalProjectionParms_t globalParms;
	if( !idRenderModelDecal::CreateProjectionParms( globalParms, winding, projectionOrigin, parallel, fadeDepth, material, startTime ) )
	{
		return;
	}

	idBounds bounds;
	bounds.FromTransformedBounds( model->Bounds( &def->parms ), def->parms.origin, def->parms.axis );

	// if the model bounds do not overlap with the projection bounds
	if( !globalParms.projectionBounds.IntersectsBounds( bounds ) )
	{
		return;
	}

	// transform the bounding planes, fade planes and texture axis into local space
	decalProjectionParms_t localParms;
	idRenderModelDecal::GlobalProjectionParmsToLocal( localParms, globalParms, def->parms.origin, def->parms.axis );
	localParms.force = ( def->parms.customShader != NULL );

	if( def->decals == NULL )
	{
		def->decals = AllocDecal( def->index, startTime );
	}
	def->decals->AddDeferredDecal( localParms );
	def->archived = false;
}

/*
====================
idRenderWorldLocal::ProjectOverlay
====================
*/
void idRenderWorldLocal::ProjectOverlay( qhandle_t entityHandle, const idPlane localTextureAxis[2], const idMaterial* material, const int startTime )
{
	if( entityHandle < 0 || entityHandle >= entityDefs.Num() )
	{
		common->Error( "idRenderWorld::ProjectOverlay: index = %i", entityHandle );
		return;
	}

	idRenderEntityLocal*	def = entityDefs[ entityHandle ];
	if( def == NULL )
	{
		return;
	}

	const idRenderModel* model = def->parms.hModel;
	if( model->IsDynamicModel() != DM_CACHED )  	// FIXME: probably should be MD5 only
	{
		return;
	}

	overlayProjectionParms_t localParms;
	localParms.localTextureAxis[0] = localTextureAxis[0];
	localParms.localTextureAxis[1] = localTextureAxis[1];
	localParms.material = material;
	localParms.startTime = startTime;

	if( def->overlays == NULL )
	{
		def->overlays = AllocOverlay( def->index, startTime );
	}
	def->overlays->AddDeferredOverlay( localParms );
	def->archived = false;
}

/*
====================
idRenderWorldLocal::AllocDecal
====================
*/
idRenderModelDecal* idRenderWorldLocal::AllocDecal( qhandle_t newEntityHandle, int startTime )
{
	int oldest = 0;
	int oldestTime = MAX_TYPE( oldestTime );
	for( int i = 0; i < decals.Num(); i++ )
	{
		if( decals[i].lastStartTime < oldestTime )
		{
			oldestTime = decals[i].lastStartTime;
			oldest = i;
		}
	}

	// remove any reference another model may still have to this decal
	if( decals[oldest].entityHandle >= 0 && decals[oldest].entityHandle < entityDefs.Num() )
	{
		idRenderEntityLocal*	def = entityDefs[decals[oldest].entityHandle];
		if( def != NULL && def->decals == decals[oldest].decals )
		{
			def->decals = NULL;
		}
	}

	decals[oldest].entityHandle = newEntityHandle;
	decals[oldest].lastStartTime = startTime;
	decals[oldest].decals->ReUse();
	if( common->WriteDemo() )
	{
		WriteFreeDecal( common->WriteDemo(), oldest );
	}
	return decals[oldest].decals;
}

/*
====================
idRenderWorldLocal::AllocOverlay
====================
*/
idRenderModelOverlay* idRenderWorldLocal::AllocOverlay( qhandle_t newEntityHandle, int startTime )
{
	int oldest = 0;
	int oldestTime = MAX_TYPE( oldestTime );
	for( int i = 0; i < overlays.Num(); i++ )
	{
		if( overlays[i].lastStartTime < oldestTime )
		{
			oldestTime = overlays[i].lastStartTime;
			oldest = i;
		}
	}

	// remove any reference another model may still have to this overlay
	if( overlays[oldest].entityHandle >= 0 && overlays[oldest].entityHandle < entityDefs.Num() )
	{
		idRenderEntityLocal*	def = entityDefs[overlays[oldest].entityHandle];
		if( def != NULL && def->overlays == overlays[oldest].overlays )
		{
			def->overlays = NULL;
		}
	}

	overlays[oldest].entityHandle = newEntityHandle;
	overlays[oldest].lastStartTime = startTime;
	overlays[oldest].overlays->ReUse();

	return overlays[oldest].overlays;
}

/*
====================
idRenderWorldLocal::RemoveDecals
====================
*/
void idRenderWorldLocal::RemoveDecals( qhandle_t entityHandle )
{
	if( entityHandle < 0 || entityHandle >= entityDefs.Num() )
	{
		common->Error( "idRenderWorld::ProjectOverlay: index = %i", entityHandle );
		return;
	}

	idRenderEntityLocal*	def = entityDefs[ entityHandle ];
	if( !def )
	{
		return;
	}

	R_FreeEntityDefDecals( def );
	R_FreeEntityDefOverlay( def );
}

/*
====================
idRenderWorldLocal::SetRenderView

Sets the current view so any calls to the render world will use the correct parms.
====================
*/
void idRenderWorldLocal::SetRenderView( const renderView_t* renderView )
{
	tr.primaryRenderView = *renderView;
}

/*
====================
idRenderWorldLocal::RenderScene

Draw a 3D view into a part of the window, then return
to 2D drawing.

Rendering a scene may require multiple views to be rendered
to handle mirrors,
====================
*/
void idRenderWorldLocal::RenderScene( const renderView_t* renderView )
{
	if( !tr.IsInitialized() )
	{
		return;
	}

	renderView_t copy = *renderView;

	// skip front end rendering work, which will result
	// in only gui drawing
	if( r_skipFrontEnd.GetBool() )
	{
		return;
	}

	SCOPED_PROFILE_EVENT( "RenderWorld::RenderScene" );

	if( renderView->fov_x <= 0 || renderView->fov_y <= 0 )
	{
		common->Error( "idRenderWorld::RenderScene: bad FOVs: %f, %f", renderView->fov_x, renderView->fov_y );
	}

	// close any gui drawing
	tr.guiModel->EmitFullScreen();
	tr.guiModel->Clear();

	int startTime = Sys_Microseconds();

	// setup view parms for the initial view
	viewDef_t* parms = ( viewDef_t* )R_ClearedFrameAlloc( sizeof( *parms ), FRAME_ALLOC_VIEW_DEF );
	parms->renderView = *renderView;
	parms->targetRender = nullptr;

	if( tr.takingScreenshot )
	{
		parms->renderView.forceUpdate = true;
	}

	int windowWidth = tr.GetWidth();
	int windowHeight = tr.GetHeight();

	if( parms->renderView.rdflags & RDF_IRRADIANCE )
	{
		windowWidth = ENVPROBE_CAPTURE_SIZE;
		windowHeight = ENVPROBE_CAPTURE_SIZE;

		tr.CropRenderSize( 0, 0, windowWidth, windowHeight, true );
		tr.GetCroppedViewport( &parms->viewport );
	}
	else
	{
		tr.PerformResolutionScaling( windowWidth, windowHeight );

		// screenFraction is just for quickly testing fill rate limitations
		if( r_screenFraction.GetInteger() != 100 )
		{
			windowWidth = ( windowWidth * r_screenFraction.GetInteger() ) / 100;
			windowHeight = ( windowHeight * r_screenFraction.GetInteger() ) / 100;
		}
		tr.CropRenderSize( windowWidth, windowHeight );
		tr.GetCroppedViewport( &parms->viewport );
	}

	// the scissor bounds may be shrunk in subviews even if
	// the viewport stays the same
	// this scissor range is local inside the viewport
	parms->scissor.x1 = 0;
	parms->scissor.y1 = 0;
	parms->scissor.x2 = parms->viewport.x2 - parms->viewport.x1;
	parms->scissor.y2 = parms->viewport.y2 - parms->viewport.y1;

	parms->isSubview = false;
	parms->isObliqueProjection = false;
	parms->initialViewAreaOrigin = renderView->vieworg;
	parms->renderWorld = this;

	// see if the view needs to reverse the culling sense in mirrors
	// or environment cube sides
	idVec3	cross;
	cross = parms->renderView.viewaxis[1].Cross( parms->renderView.viewaxis[2] );
	if( cross * parms->renderView.viewaxis[0] > 0 )
	{
		parms->isMirror = false;
	}
	else
	{
		parms->isMirror = true;
	}

	// save this world for use by some console commands
	tr.primaryWorld = this;
	tr.primaryRenderView = *renderView;
	tr.primaryView = parms;

	// rendering this view may cause other views to be rendered
	// for mirrors / portals / shadows / environment maps
	// this will also cause any necessary entities and lights to be
	// updated to the demo file
	R_RenderView( parms );

	// render any post processing after the view and all its subviews has been draw
	R_RenderPostProcess( parms );

	// now write delete commands for any modified-but-not-visible entities, and
	// add the renderView command to the demo
	if( common->WriteDemo() )
	{
		WriteRenderView( renderView );
	}

#if 0
	for( int i = 0; i < entityDefs.Num(); i++ )
	{
		idRenderEntityLocal*	def = entityDefs[i];
		if( !def )
		{
			continue;
		}
		if( def->parms.callback )
		{
			continue;
		}
		if( def->parms.hModel->IsDynamicModel() == DM_CONTINUOUS )
		{
		}
	}
#endif

	tr.UnCrop();

	int endTime = Sys_Microseconds();

	tr.pc.frontEndMicroSec += endTime - startTime;

	// prepare for any 2D drawing after this
	tr.guiModel->Clear();
}

/*
===================
idRenderWorldLocal::NumAreas
===================
*/
int idRenderWorldLocal::NumAreas() const
{
	return numPortalAreas;
}

/*
===================
idRenderWorldLocal::NumPortalsInArea
===================
*/
int idRenderWorldLocal::NumPortalsInArea( int areaNum )
{
	portalArea_t*	area;
	int				count;
	portal_t*		portal;

	if( areaNum >= numPortalAreas || areaNum < 0 )
	{
		common->Error( "idRenderWorld::NumPortalsInArea: bad areanum %i", areaNum );
	}
	area = &portalAreas[areaNum];

	count = 0;
	for( portal = area->portals; portal; portal = portal->next )
	{
		count++;
	}
	return count;
}

/*
===================
idRenderWorldLocal::GetPortal
===================
*/
exitPortal_t idRenderWorldLocal::GetPortal( int areaNum, int portalNum )
{
	portalArea_t*	area;
	int				count;
	portal_t*		portal;
	exitPortal_t	ret;

	if( areaNum > numPortalAreas )
	{
		common->Error( "idRenderWorld::GetPortal: areaNum > numAreas" );
	}
	area = &portalAreas[areaNum];

	count = 0;
	for( portal = area->portals; portal; portal = portal->next )
	{
		if( count == portalNum )
		{
			ret.areas[0] = areaNum;
			ret.areas[1] = portal->intoArea;
			ret.w = portal->w;
			ret.blockingBits = portal->doublePortal->blockingBits;
			ret.portalHandle = portal->doublePortal - doublePortals + 1;
			return ret;
		}
		count++;
	}

	common->Error( "idRenderWorld::GetPortal: portalNum > numPortals" );

	memset( &ret, 0, sizeof( ret ) );
	return ret;
}

/*
===================
RB: idRenderWorldLocal::AreaBounds
===================
*/
idBounds idRenderWorldLocal::AreaBounds( int areaNum ) const
{
	if( areaNum < 0 || areaNum > numPortalAreas )
	{
		common->Error( "idRenderWorld::GetPortal: areaNum > numAreas" );
	}

	portalArea_t* area = &portalAreas[areaNum];

	return area->globalBounds;
}

/*
===============
idRenderWorldLocal::PointInAreaNum

Will return -1 if the point is not in an area, otherwise
it will return 0 <= value < tr.world->numPortalAreas
===============
*/
int idRenderWorldLocal::PointInArea( const idVec3& point ) const
{
	areaNode_t*	node;
	int			nodeNum;
	float		d;

	node = areaNodes;
	if( !node )
	{
		return -1;
	}
	while( 1 )
	{
		d = point * node->plane.Normal() + node->plane[3];
		if( d > 0 )
		{
			nodeNum = node->children[0];
		}
		else
		{
			nodeNum = node->children[1];
		}
		if( nodeNum == 0 )
		{
			return -1;		// in solid
		}
		if( nodeNum < 0 )
		{
			nodeNum = -1 - nodeNum;
			if( nodeNum >= numPortalAreas )
			{
				common->Error( "idRenderWorld::PointInArea: area out of range" );
			}
			return nodeNum;
		}
		node = areaNodes + nodeNum;
	}

	return -1;
}

/*
===================
idRenderWorldLocal::BoundsInAreas_r
===================
*/
void idRenderWorldLocal::BoundsInAreas_r( int nodeNum, const idBounds& bounds, int* areas, int* numAreas, int maxAreas ) const
{
	int side, i;
	areaNode_t* node;

	do
	{
		if( nodeNum < 0 )
		{
			nodeNum = -1 - nodeNum;

			for( i = 0; i < ( *numAreas ); i++ )
			{
				if( areas[i] == nodeNum )
				{
					break;
				}
			}
			if( i >= ( *numAreas ) && ( *numAreas ) < maxAreas )
			{
				areas[( *numAreas )++] = nodeNum;
			}
			return;
		}

		node = areaNodes + nodeNum;

		side = bounds.PlaneSide( node->plane );
		if( side == PLANESIDE_FRONT )
		{
			nodeNum = node->children[0];
		}
		else if( side == PLANESIDE_BACK )
		{
			nodeNum = node->children[1];
		}
		else
		{
			if( node->children[1] != 0 )
			{
				BoundsInAreas_r( node->children[1], bounds, areas, numAreas, maxAreas );
				if( ( *numAreas ) >= maxAreas )
				{
					return;
				}
			}
			nodeNum = node->children[0];
		}
	}
	while( nodeNum != 0 );

	return;
}

/*
===================
idRenderWorldLocal::BoundsInAreas

  fills the *areas array with the number of the areas the bounds are in
  returns the total number of areas the bounds are in
===================
*/
int idRenderWorldLocal::BoundsInAreas( const idBounds& bounds, int* areas, int maxAreas ) const
{
	int numAreas = 0;

	assert( areas );
	assert( bounds[0][0] <= bounds[1][0] && bounds[0][1] <= bounds[1][1] && bounds[0][2] <= bounds[1][2] );
	assert( bounds[1][0] - bounds[0][0] < 1e4f && bounds[1][1] - bounds[0][1] < 1e4f && bounds[1][2] - bounds[0][2] < 1e4f );

	if( !areaNodes )
	{
		return numAreas;
	}
	BoundsInAreas_r( 0, bounds, areas, &numAreas, maxAreas );
	return numAreas;
}

/*
================
idRenderWorldLocal::GuiTrace

checks a ray trace against any gui surfaces in an entity, returning the
fraction location of the trace on the gui surface, or -1,-1 if no hit.
this doesn't do any occlusion testing, simply ignoring non-gui surfaces.
start / end are in global world coordinates.
================
*/
guiPoint_t idRenderWorldLocal::GuiTrace( qhandle_t entityHandle, const idVec3 start, const idVec3 end ) const
{
	guiPoint_t	pt;
	pt.x = pt.y = -1;
	pt.guiId = 0;

	if( ( entityHandle < 0 ) || ( entityHandle >= entityDefs.Num() ) )
	{
		common->Printf( "idRenderWorld::GuiTrace: invalid handle %i\n", entityHandle );
		return pt;
	}

	idRenderEntityLocal* def = entityDefs[entityHandle];
	if( def == NULL )
	{
		common->Printf( "idRenderWorld::GuiTrace: handle %i is NULL\n", entityHandle );
		return pt;
	}

	idRenderModel* model = def->parms.hModel;
	if( model == NULL || model->IsDynamicModel() != DM_STATIC || def->parms.callback != NULL )
	{
		return pt;
	}

	// transform the points into local space
	idVec3 localStart, localEnd;
	R_GlobalPointToLocal( def->modelMatrix, start, localStart );
	R_GlobalPointToLocal( def->modelMatrix, end, localEnd );

	for( int i = 0; i < model->NumSurfaces(); i++ )
	{
		const modelSurface_t* surf = model->Surface( i );

		const srfTriangles_t* tri = surf->geometry;
		if( tri == NULL )
		{
			continue;
		}

		const idMaterial* shader = R_RemapShaderBySkin( surf->shader, def->parms.customSkin, def->parms.customShader );
		if( shader == NULL )
		{
			continue;
		}
		// only trace against gui surfaces
		if( !shader->HasGui() )
		{
			continue;
		}

		localTrace_t local = R_LocalTrace( localStart, localEnd, 0.0f, tri );
		if( local.fraction < 1.0f )
		{
			idVec3 origin, axis[3];

			R_SurfaceToTextureAxis( tri, origin, axis );
			const idVec3 cursor = local.point - origin;

			float axisLen[2];
			axisLen[0] = axis[0].Length();
			axisLen[1] = axis[1].Length();

			pt.x = ( cursor * axis[0] ) / ( axisLen[0] * axisLen[0] );
			pt.y = ( cursor * axis[1] ) / ( axisLen[1] * axisLen[1] );
			pt.guiId = shader->GetEntityGui();

			return pt;
		}
	}

	return pt;
}

/*
===================
idRenderWorldLocal::ModelTrace
===================
*/
bool idRenderWorldLocal::ModelTrace( modelTrace_t& trace, qhandle_t entityHandle, const idVec3& start, const idVec3& end, const float radius ) const
{

	memset( &trace, 0, sizeof( trace ) );
	trace.fraction = 1.0f;
	trace.point = end;

	if( entityHandle < 0 || entityHandle >= entityDefs.Num() )
	{
		return false;
	}

	idRenderEntityLocal*	def = entityDefs[entityHandle];
	if( def == NULL )
	{
		return false;
	}

	renderEntity_t* refEnt = &def->parms;

	idRenderModel* model = R_EntityDefDynamicModel( def );
	if( model == NULL )
	{
		return false;
	}

	// transform the points into local space
	float modelMatrix[16];
	idVec3 localStart;
	idVec3 localEnd;
	R_AxisToModelMatrix( refEnt->axis, refEnt->origin, modelMatrix );
	R_GlobalPointToLocal( modelMatrix, start, localStart );
	R_GlobalPointToLocal( modelMatrix, end, localEnd );

	// if we have explicit collision surfaces, only collide against them
	// (FIXME, should probably have a parm to control this)
	bool collisionSurface = false;
	for( int i = 0; i < model->NumBaseSurfaces(); i++ )
	{
		const modelSurface_t* surf = model->Surface( i );

		const idMaterial* shader = R_RemapShaderBySkin( surf->shader, def->parms.customSkin, def->parms.customShader );

		if( shader->GetSurfaceFlags() & SURF_COLLISION )
		{
			collisionSurface = true;
			break;
		}
	}

	// only use baseSurfaces, not any overlays
	for( int i = 0; i < model->NumBaseSurfaces(); i++ )
	{
		const modelSurface_t* surf = model->Surface( i );

		const idMaterial* shader = R_RemapShaderBySkin( surf->shader, def->parms.customSkin, def->parms.customShader );

		if( surf->geometry == NULL || shader == NULL )
		{
			continue;
		}

		if( collisionSurface )
		{
			// only trace vs collision surfaces
			if( ( shader->GetSurfaceFlags() & SURF_COLLISION ) == 0 )
			{
				continue;
			}
		}
		else
		{
			// skip if not drawn or translucent
			if( !shader->IsDrawn() || ( shader->Coverage() != MC_OPAQUE && shader->Coverage() != MC_PERFORATED ) )
			{
				continue;
			}
		}

		localTrace_t localTrace = R_LocalTrace( localStart, localEnd, radius, surf->geometry );

		if( localTrace.fraction < trace.fraction )
		{
			trace.fraction = localTrace.fraction;
			R_LocalPointToGlobal( modelMatrix, localTrace.point, trace.point );
			trace.normal = localTrace.normal * refEnt->axis;
			trace.material = shader;
			trace.entity = &def->parms;
			trace.jointNumber = refEnt->hModel->NearestJoint( i, localTrace.indexes[0], localTrace.indexes[1], localTrace.indexes[2] );
		}
	}

	return ( trace.fraction < 1.0f );
}

/*
===================
idRenderWorldLocal::Trace
===================
*/
// FIXME: _D3XP added those.
const char* playerModelExcludeList[] =
{
	"models/md5/characters/player/d3xp_spplayer.md5mesh",
	"models/md5/characters/player/head/d3xp_head.md5mesh",
	"models/md5/weapons/pistol_world/worldpistol.md5mesh",
	NULL
};

const char* playerMaterialExcludeList[] =
{
	"muzzlesmokepuff",
	NULL
};

bool idRenderWorldLocal::Trace( modelTrace_t& trace, const idVec3& start, const idVec3& end, const float radius, bool skipDynamic, bool skipPlayer /*_D3XP*/ ) const
{
	trace.fraction = 1.0f;
	trace.point = end;

	// bounds for the whole trace
	idBounds traceBounds;
	traceBounds.Clear();
	traceBounds.AddPoint( start );
	traceBounds.AddPoint( end );

	// get the world areas the trace is in
	int areas[128];
	int numAreas = BoundsInAreas( traceBounds, areas, 128 );

	int numSurfaces = 0;

	// check all areas for models
	for( int i = 0; i < numAreas; i++ )
	{

		portalArea_t* area = &portalAreas[ areas[i] ];

		// check all models in this area
		for( areaReference_t* ref = area->entityRefs.areaNext; ref != &area->entityRefs; ref = ref->areaNext )
		{
			idRenderEntityLocal* def = ref->entity;

			idRenderModel* model = def->parms.hModel;
			if( model == NULL )
			{
				continue;
			}

			if( model->IsDynamicModel() != DM_STATIC )
			{
				if( skipDynamic )
				{
					continue;
				}

#if 1	/* _D3XP addition. could use a cleaner approach */
				if( skipPlayer )
				{
					bool exclude = false;
					for( int k = 0; playerModelExcludeList[k] != NULL; k++ )
					{
						if( idStr::Cmp( model->Name(), playerModelExcludeList[k] ) == 0 )
						{
							exclude = true;
							break;
						}
					}
					if( exclude )
					{
						continue;
					}
				}
#endif

				model = R_EntityDefDynamicModel( def );
				if( !model )
				{
					continue;	// can happen with particle systems, which don't instantiate without a valid view
				}
			}

			idBounds bounds;
			bounds.FromTransformedBounds( model->Bounds( &def->parms ), def->parms.origin, def->parms.axis );

			// if the model bounds do not overlap with the trace bounds
			if( !traceBounds.IntersectsBounds( bounds ) || !bounds.LineIntersection( start, trace.point ) )
			{
				continue;
			}

			// check all model surfaces
			for( int j = 0; j < model->NumSurfaces(); j++ )
			{
				const modelSurface_t* surf = model->Surface( j );

				const idMaterial* shader = R_RemapShaderBySkin( surf->shader, def->parms.customSkin, def->parms.customShader );

				// if no geometry or no shader
				if( surf->geometry == NULL || shader == NULL )
				{
					continue;
				}

#if 1 /* _D3XP addition. could use a cleaner approach */
				if( skipPlayer )
				{
					bool exclude = false;
					for( int k = 0; playerMaterialExcludeList[k] != NULL; k++ )
					{
						if( idStr::Cmp( shader->GetName(), playerMaterialExcludeList[k] ) == 0 )
						{
							exclude = true;
							break;
						}
					}
					if( exclude )
					{
						continue;
					}
				}
#endif

				const srfTriangles_t* tri = surf->geometry;

				bounds.FromTransformedBounds( tri->bounds, def->parms.origin, def->parms.axis );

				// if triangle bounds do not overlap with the trace bounds
				if( !traceBounds.IntersectsBounds( bounds ) || !bounds.LineIntersection( start, trace.point ) )
				{
					continue;
				}

				numSurfaces++;

				// transform the points into local space
				float modelMatrix[16];
				idVec3 localStart, localEnd;
				R_AxisToModelMatrix( def->parms.axis, def->parms.origin, modelMatrix );
				R_GlobalPointToLocal( modelMatrix, start, localStart );
				R_GlobalPointToLocal( modelMatrix, end, localEnd );

				localTrace_t localTrace = R_LocalTrace( localStart, localEnd, radius, surf->geometry );

				if( localTrace.fraction < trace.fraction )
				{
					trace.fraction = localTrace.fraction;
					R_LocalPointToGlobal( modelMatrix, localTrace.point, trace.point );
					trace.normal = localTrace.normal * def->parms.axis;
					trace.material = shader;
					trace.entity = &def->parms;
					trace.jointNumber = model->NearestJoint( j, localTrace.indexes[0], localTrace.indexes[1], localTrace.indexes[2] );

					traceBounds.Clear();
					traceBounds.AddPoint( start );
					traceBounds.AddPoint( start + trace.fraction * ( end - start ) );
				}
			}
		}
	}
	return ( trace.fraction < 1.0f );
}

/*
==================
idRenderWorldLocal::RecurseProcBSP
==================
*/
void idRenderWorldLocal::RecurseProcBSP_r( modelTrace_t* results, int parentNodeNum, int nodeNum, float p1f, float p2f, const idVec3& p1, const idVec3& p2 ) const
{
	float		t1, t2;
	float		frac;
	idVec3		mid;
	int			side;
	float		midf;
	areaNode_t* node;

	if( results->fraction <= p1f )
	{
		return;		// already hit something nearer
	}
	// empty leaf
	if( nodeNum < 0 )
	{
		return;
	}
	// if solid leaf node
	if( nodeNum == 0 )
	{
		if( parentNodeNum != -1 )
		{

			results->fraction = p1f;
			results->point = p1;
			node = &areaNodes[parentNodeNum];
			results->normal = node->plane.Normal();
			return;
		}
	}
	node = &areaNodes[nodeNum];

	// distance from plane for trace start and end
	t1 = node->plane.Normal() * p1 + node->plane[3];
	t2 = node->plane.Normal() * p2 + node->plane[3];

	if( t1 >= 0.0f && t2 >= 0.0f )
	{
		RecurseProcBSP_r( results, nodeNum, node->children[0], p1f, p2f, p1, p2 );
		return;
	}
	if( t1 < 0.0f && t2 < 0.0f )
	{
		RecurseProcBSP_r( results, nodeNum, node->children[1], p1f, p2f, p1, p2 );
		return;
	}
	side = t1 < t2;
	frac = t1 / ( t1 - t2 );
	midf = p1f + frac * ( p2f - p1f );
	mid[0] = p1[0] + frac * ( p2[0] - p1[0] );
	mid[1] = p1[1] + frac * ( p2[1] - p1[1] );
	mid[2] = p1[2] + frac * ( p2[2] - p1[2] );
	RecurseProcBSP_r( results, nodeNum, node->children[side], p1f, midf, p1, mid );
	RecurseProcBSP_r( results, nodeNum, node->children[side ^ 1], midf, p2f, mid, p2 );
}

/*
==================
idRenderWorldLocal::FastWorldTrace
==================
*/
bool idRenderWorldLocal::FastWorldTrace( modelTrace_t& results, const idVec3& start, const idVec3& end ) const
{
	memset( &results, 0, sizeof( modelTrace_t ) );
	results.fraction = 1.0f;
	if( areaNodes != NULL )
	{
		RecurseProcBSP_r( &results, -1, 0, 0.0f, 1.0f, start, end );
		return ( results.fraction < 1.0f );
	}
	return false;
}

/*
=================================================================================

CREATE MODEL REFS

=================================================================================
*/

/*
=================
idRenderWorldLocal::AddEntityRefToArea

This is called by R_PushVolumeIntoTree and also directly
for the world model references that are precalculated.
=================
*/
void idRenderWorldLocal::AddEntityRefToArea( idRenderEntityLocal* def, portalArea_t* area )
{
	areaReference_t*	ref;

	if( def == NULL )
	{
		common->Error( "idRenderWorldLocal::AddEntityRefToArea: NULL def" );
		return;
	}

	for( ref = def->entityRefs; ref != NULL; ref = ref->ownerNext )
	{
		if( ref->area == area )
		{
			return;
		}
	}

	ref = areaReferenceAllocator.Alloc();

	tr.pc.c_entityReferences++;

	ref->entity = def;

	// link to entityDef
	ref->ownerNext = def->entityRefs;
	def->entityRefs = ref;

	// link to end of area list
	ref->area = area;
	ref->areaNext = &area->entityRefs;
	ref->areaPrev = area->entityRefs.areaPrev;
	ref->areaNext->areaPrev = ref;
	ref->areaPrev->areaNext = ref;
}

/*
===================
idRenderWorldLocal::AddLightRefToArea
===================
*/
void idRenderWorldLocal::AddLightRefToArea( idRenderLightLocal* light, portalArea_t* area )
{
	areaReference_t*	lref;

	for( lref = light->references; lref != NULL; lref = lref->ownerNext )
	{
		if( lref->area == area )
		{
			return;
		}
	}

	// add a lightref to this area
	lref = areaReferenceAllocator.Alloc();
	lref->light = light;
	lref->area = area;
	lref->ownerNext = light->references;
	light->references = lref;
	tr.pc.c_lightReferences++;

	// doubly linked list so we can free them easily later
	area->lightRefs.areaNext->areaPrev = lref;
	lref->areaNext = area->lightRefs.areaNext;
	lref->areaPrev = &area->lightRefs;
	area->lightRefs.areaNext = lref;
}

// RB begin
void idRenderWorldLocal::AddEnvprobeRefToArea( RenderEnvprobeLocal* probe, portalArea_t* area )
{
	areaReference_t*	lref;

	for( lref = probe->references; lref != NULL; lref = lref->ownerNext )
	{
		if( lref->area == area )
		{
			return;
		}
	}

	// add a envproberef to this area
	lref = areaReferenceAllocator.Alloc();
	lref->envprobe = probe;
	lref->area = area;
	lref->ownerNext = probe->references;
	probe->references = lref;
	tr.pc.c_lightReferences++;

	// doubly linked list so we can free them easily later
	area->envprobeRefs.areaNext->areaPrev = lref;
	lref->areaNext = area->envprobeRefs.areaNext;
	lref->areaPrev = &area->envprobeRefs;
	area->envprobeRefs.areaNext = lref;
}
// RB end

/*
===================
idRenderWorldLocal::GenerateAllInteractions

Force the generation of all light / surface interactions at the start of a level
If this isn't called, they will all be dynamically generated
===================
*/
void idRenderWorldLocal::GenerateAllInteractions()
{
	if( !tr.IsInitialized() )
	{
		return;
	}

	int start = Sys_Milliseconds();

	generateAllInteractionsCalled = false;

	// let the interaction creation code know that it shouldn't
	// try and do any view specific optimizations
	tr.viewDef = NULL;

	// build the interaction table
	// this will be dynamically resized if the entity / light counts grow too much
	interactionTableWidth = entityDefs.Num() + 100;
	interactionTableHeight = lightDefs.Num() + 100;
	int	size =  interactionTableWidth * interactionTableHeight * sizeof( *interactionTable );
	interactionTable = ( idInteraction** )R_ClearedStaticAlloc( size );

#if defined( USE_NVRHI )
	tr.commandList->open();
#endif

	// iterate through all lights
	int	count = 0;
	for( int i = 0; i < this->lightDefs.Num(); i++ )
	{
		idRenderLightLocal*	ldef = this->lightDefs[i];
		if( ldef == NULL )
		{
			continue;
		}

		// check all areas the light touches
		for( areaReference_t* lref = ldef->references; lref; lref = lref->ownerNext )
		{
			portalArea_t* area = lref->area;

			// check all the models in this area
			for( areaReference_t* eref = area->entityRefs.areaNext; eref != &area->entityRefs; eref = eref->areaNext )
			{
				idRenderEntityLocal* 	edef = eref->entity;

				// scan the doubly linked lists, which may have several dozen entries
				idInteraction*	inter;

				// we could check either model refs or light refs for matches, but it is
				// assumed that there will be less lights in an area than models
				// so the entity chains should be somewhat shorter (they tend to be fairly close).
				for( inter = edef->firstInteraction; inter != NULL; inter = inter->entityNext )
				{
					if( inter->lightDef == ldef )
					{
						break;
					}
				}

				// if we already have an interaction, we don't need to do anything
				if( inter != NULL )
				{
					continue;
				}

				// make an interaction for this light / entity pair
				// and add a pointer to it in the table
				inter = idInteraction::AllocAndLink( edef, ldef );
				count++;

				// the interaction may create geometry
				inter->CreateStaticInteraction( tr.commandList );
			}
		}

		session->Pump();
	}

#if defined( USE_NVRHI )
	tr.commandList->close();
	deviceManager->GetDevice()->executeCommandList( tr.commandList );
#endif

	int end = Sys_Milliseconds();
	int	msec = end - start;

	common->Printf( "idRenderWorld::GenerateAllInteractions, msec = %i\n", msec );
	common->Printf( "interactionTable size: %i bytes\n", size );
	common->Printf( "%i interactions take %i bytes\n", count, count * sizeof( idInteraction ) );

	// entities flagged as noDynamicInteractions will no longer make any
	generateAllInteractionsCalled = true;
}

/*
===================
idRenderWorldLocal::FreeInteractions
===================
*/
void idRenderWorldLocal::FreeInteractions()
{
	int			i;
	idRenderEntityLocal*	def;

	for( i = 0; i < entityDefs.Num(); i++ )
	{
		def = entityDefs[i];
		if( !def )
		{
			continue;
		}
		// free all the interactions
		while( def->firstInteraction != NULL )
		{
			def->firstInteraction->UnlinkAndFree();
		}
	}
}

/*
==================
idRenderWorldLocal::PushFrustumIntoTree_r

Used for both light volumes and model volumes.

This does not clip the points by the planes, so some slop
occurs.

tr.viewCount should be bumped before calling, allowing it
to prevent double checking areas.

We might alternatively choose to do this with an area flow.
==================
*/
void idRenderWorldLocal::PushFrustumIntoTree_r( idRenderEntityLocal* def, idRenderLightLocal* light,
		const frustumCorners_t& corners, int nodeNum )
{
	if( nodeNum < 0 )
	{
		int areaNum = -1 - nodeNum;
		portalArea_t* area = &portalAreas[ areaNum ];
		if( area->viewCount == tr.viewCount )
		{
			return;	// already added a reference here
		}
		area->viewCount = tr.viewCount;

		if( def != NULL )
		{
			AddEntityRefToArea( def, area );
		}
		if( light != NULL )
		{
			AddLightRefToArea( light, area );
		}

		return;
	}

	areaNode_t* node = areaNodes + nodeNum;

	// if we know that all possible children nodes only touch an area
	// we have already marked, we can early out
	if( node->commonChildrenArea != CHILDREN_HAVE_MULTIPLE_AREAS && r_useNodeCommonChildren.GetBool() )
	{
		// note that we do NOT try to set a reference in this area
		// yet, because the test volume may yet wind up being in the
		// solid part, which would cause bounds slightly poked into
		// a wall to show up in the next room
		if( portalAreas[ node->commonChildrenArea ].viewCount == tr.viewCount )
		{
			return;
		}
	}

	// exact check all the corners against the node plane
	frustumCull_t cull = idRenderMatrix::CullFrustumCornersToPlane( corners, node->plane );

	if( cull != FRUSTUM_CULL_BACK )
	{
		nodeNum = node->children[0];
		if( nodeNum != 0 )  	// 0 = solid
		{
			PushFrustumIntoTree_r( def, light, corners, nodeNum );
		}
	}

	if( cull != FRUSTUM_CULL_FRONT )
	{
		nodeNum = node->children[1];
		if( nodeNum != 0 )  	// 0 = solid
		{
			PushFrustumIntoTree_r( def, light, corners, nodeNum );
		}
	}
}

/*
==============
idRenderWorldLocal::PushFrustumIntoTree
==============
*/
void idRenderWorldLocal::PushFrustumIntoTree( idRenderEntityLocal* def, idRenderLightLocal* light, const idRenderMatrix& frustumTransform, const idBounds& frustumBounds )
{
	if( areaNodes == NULL )
	{
		return;
	}

	// calculate the corners of the frustum in word space
	ALIGNTYPE16 frustumCorners_t corners;
	idRenderMatrix::GetFrustumCorners( corners, frustumTransform, frustumBounds );

	PushFrustumIntoTree_r( def, light, corners, 0 );
}


// RB begin
void idRenderWorldLocal::PushEnvprobeIntoTree_r( RenderEnvprobeLocal* probe, int nodeNum )
{
	if( nodeNum < 0 )
	{
		int areaNum = -1 - nodeNum;
		portalArea_t* area = &portalAreas[ areaNum ];
		if( area->viewCount == tr.viewCount )
		{
			return;	// already added a reference here
		}
		area->viewCount = tr.viewCount;

		if( probe != NULL )
		{
			AddEnvprobeRefToArea( probe, area );
		}

		return;
	}

	areaNode_t* node = areaNodes + nodeNum;

	// if we know that all possible children nodes only touch an area
	// we have already marked, we can early out
	if( node->commonChildrenArea != CHILDREN_HAVE_MULTIPLE_AREAS && r_useNodeCommonChildren.GetBool() )
	{
		// note that we do NOT try to set a reference in this area
		// yet, because the test volume may yet wind up being in the
		// solid part, which would cause bounds slightly poked into
		// a wall to show up in the next room
		if( portalAreas[ node->commonChildrenArea ].viewCount == tr.viewCount )
		{
			return;
		}
	}


	int cull = node->plane.Side( probe->parms.origin );

	if( cull != PLANESIDE_BACK )
	{
		nodeNum = node->children[0];
		if( nodeNum != 0 )  	// 0 = solid
		{
			PushEnvprobeIntoTree_r( probe, nodeNum );
		}
	}

	if( cull != PLANESIDE_FRONT )
	{
		nodeNum = node->children[1];
		if( nodeNum != 0 )  	// 0 = solid
		{
			PushEnvprobeIntoTree_r( probe, nodeNum );
		}
	}
}
// RB end

//===================================================================

/*
====================
idRenderWorldLocal::DebugClearLines
====================
*/
void idRenderWorldLocal::DebugClearLines( int time )
{
	RB_ClearDebugLines( time );
	RB_ClearDebugText( time );
}

/*
====================
idRenderWorldLocal::DebugLine
====================
*/
void idRenderWorldLocal::DebugLine( const idVec4& color, const idVec3& start, const idVec3& end, const int lifetime, const bool depthTest )
{
	RB_AddDebugLine( color, start, end, lifetime, depthTest );
}

/*
================
idRenderWorldLocal::DebugArrow
================
*/
void idRenderWorldLocal::DebugArrow( const idVec4& color, const idVec3& start, const idVec3& end, int size, const int lifetime )
{
	idVec3 forward, right, up, v1, v2;
	float a, s;
	int i;
	static float arrowCos[40];
	static float arrowSin[40];
	static int arrowStep;

	DebugLine( color, start, end, lifetime );

	if( r_debugArrowStep.GetInteger() <= 10 )
	{
		return;
	}
	// calculate sine and cosine when step size changes
	if( arrowStep != r_debugArrowStep.GetInteger() )
	{
		arrowStep = r_debugArrowStep.GetInteger();
		for( i = 0, a = 0; a < 360.0f; a += arrowStep, i++ )
		{
			arrowCos[i] = idMath::Cos16( DEG2RAD( a ) );
			arrowSin[i] = idMath::Sin16( DEG2RAD( a ) );
		}
		arrowCos[i] = arrowCos[0];
		arrowSin[i] = arrowSin[0];
	}
	// draw a nice arrow
	forward = end - start;
	forward.Normalize();
	forward.NormalVectors( right, up );
	for( i = 0, a = 0; a < 360.0f; a += arrowStep, i++ )
	{
		s = 0.5f * size * arrowCos[i];
		v1 = end - size * forward;
		v1 = v1 + s * right;
		s = 0.5f * size * arrowSin[i];
		v1 = v1 + s * up;

		s = 0.5f * size * arrowCos[i + 1];
		v2 = end - size * forward;
		v2 = v2 + s * right;
		s = 0.5f * size * arrowSin[i + 1];
		v2 = v2 + s * up;

		DebugLine( color, v1, end, lifetime );
		DebugLine( color, v1, v2, lifetime );
	}
}

/*
====================
idRenderWorldLocal::DebugWinding
====================
*/
void idRenderWorldLocal::DebugWinding( const idVec4& color, const idWinding& w, const idVec3& origin, const idMat3& axis, const int lifetime, const bool depthTest )
{
	int i;
	idVec3 point, lastPoint;

	if( w.GetNumPoints() < 2 )
	{
		return;
	}

	lastPoint = origin + w[w.GetNumPoints() - 1].ToVec3() * axis;
	for( i = 0; i < w.GetNumPoints(); i++ )
	{
		point = origin + w[i].ToVec3() * axis;
		DebugLine( color, lastPoint, point, lifetime, depthTest );
		lastPoint = point;
	}
}

/*
====================
idRenderWorldLocal::DebugCircle
====================
*/
void idRenderWorldLocal::DebugCircle( const idVec4& color, const idVec3& origin, const idVec3& dir, const float radius, const int numSteps, const int lifetime, const bool depthTest )
{
	int i;
	float a;
	idVec3 left, up, point, lastPoint;

	dir.OrthogonalBasis( left, up );
	left *= radius;
	up *= radius;
	lastPoint = origin + up;
	for( i = 1; i <= numSteps; i++ )
	{
		a = idMath::TWO_PI * i / numSteps;
		point = origin + idMath::Sin16( a ) * left + idMath::Cos16( a ) * up;
		DebugLine( color, lastPoint, point, lifetime, depthTest );
		lastPoint = point;
	}
}

/*
============
idRenderWorldLocal::DebugSphere
============
*/
void idRenderWorldLocal::DebugSphere( const idVec4& color, const idSphere& sphere, const int lifetime, const bool depthTest /*_D3XP*/ )
{
	int i, j, n, num;
	float s, c;
	idVec3 p, lastp, *lastArray;

	num = 360 / 15;
	lastArray = ( idVec3* ) _alloca16( num * sizeof( idVec3 ) );
	lastArray[0] = sphere.GetOrigin() + idVec3( 0, 0, sphere.GetRadius() );
	for( n = 1; n < num; n++ )
	{
		lastArray[n] = lastArray[0];
	}

	for( i = 15; i <= 360; i += 15 )
	{
		s = idMath::Sin16( DEG2RAD( i ) );
		c = idMath::Cos16( DEG2RAD( i ) );
		lastp[0] = sphere.GetOrigin()[0];
		lastp[1] = sphere.GetOrigin()[1] + sphere.GetRadius() * s;
		lastp[2] = sphere.GetOrigin()[2] + sphere.GetRadius() * c;
		for( n = 0, j = 15; j <= 360; j += 15, n++ )
		{
			p[0] = sphere.GetOrigin()[0] + idMath::Sin16( DEG2RAD( j ) ) * sphere.GetRadius() * s;
			p[1] = sphere.GetOrigin()[1] + idMath::Cos16( DEG2RAD( j ) ) * sphere.GetRadius() * s;
			p[2] = lastp[2];

			DebugLine( color, lastp, p, lifetime, depthTest );
			DebugLine( color, lastp, lastArray[n], lifetime, depthTest );

			lastArray[n] = lastp;
			lastp = p;
		}
	}
}

/*
====================
idRenderWorldLocal::DebugBounds
====================
*/
void idRenderWorldLocal::DebugBounds( const idVec4& color, const idBounds& bounds, const idVec3& org, const int lifetime )
{
	int i;
	idVec3 v[8];

	if( bounds.IsCleared() )
	{
		return;
	}

	for( i = 0; i < 8; i++ )
	{
		v[i][0] = org[0] + bounds[( i ^ ( i >> 1 ) ) & 1][0];
		v[i][1] = org[1] + bounds[( i >> 1 ) & 1][1];
		v[i][2] = org[2] + bounds[( i >> 2 ) & 1][2];
	}
	for( i = 0; i < 4; i++ )
	{
		DebugLine( color, v[i], v[( i + 1 ) & 3], lifetime );
		DebugLine( color, v[4 + i], v[4 + ( ( i + 1 ) & 3 )], lifetime );
		DebugLine( color, v[i], v[4 + i], lifetime );
	}
}

/*
====================
idRenderWorldLocal::DebugBox
====================
*/
void idRenderWorldLocal::DebugBox( const idVec4& color, const idBox& box, const int lifetime )
{
	int i;
	idVec3 v[8];

	box.ToPoints( v );
	for( i = 0; i < 4; i++ )
	{
		DebugLine( color, v[i], v[( i + 1 ) & 3], lifetime );
		DebugLine( color, v[4 + i], v[4 + ( ( i + 1 ) & 3 )], lifetime );
		DebugLine( color, v[i], v[4 + i], lifetime );
	}
}

/*
============
idRenderWorldLocal::DebugCone

  dir is the cone axis
  radius1 is the radius at the apex
  radius2 is the radius at apex+dir
============
*/
void idRenderWorldLocal::DebugCone( const idVec4& color, const idVec3& apex, const idVec3& dir, float radius1, float radius2, const int lifetime )
{
	int i;
	idMat3 axis;
	idVec3 top, p1, p2, lastp1, lastp2, d;

	axis[2] = dir;
	axis[2].Normalize();
	axis[2].NormalVectors( axis[0], axis[1] );
	axis[1] = -axis[1];

	top = apex + dir;
	lastp2 = top + radius2 * axis[1];

	if( radius1 == 0.0f )
	{
		for( i = 20; i <= 360; i += 20 )
		{
			d = idMath::Sin16( DEG2RAD( i ) ) * axis[0] + idMath::Cos16( DEG2RAD( i ) ) * axis[1];
			p2 = top + d * radius2;
			DebugLine( color, lastp2, p2, lifetime );
			DebugLine( color, p2, apex, lifetime );
			lastp2 = p2;
		}
	}
	else
	{
		lastp1 = apex + radius1 * axis[1];
		for( i = 20; i <= 360; i += 20 )
		{
			d = idMath::Sin16( DEG2RAD( i ) ) * axis[0] + idMath::Cos16( DEG2RAD( i ) ) * axis[1];
			p1 = apex + d * radius1;
			p2 = top + d * radius2;
			DebugLine( color, lastp1, p1, lifetime );
			DebugLine( color, lastp2, p2, lifetime );
			DebugLine( color, p1, p2, lifetime );
			lastp1 = p1;
			lastp2 = p2;
		}
	}
}

/*
================
idRenderWorldLocal::DebugAxis
================
*/
void idRenderWorldLocal::DebugAxis( const idVec3& origin, const idMat3& axis )
{
	idVec3 start = origin;
	idVec3 end = start + axis[0] * 20.0f;
	DebugArrow( colorWhite, start, end, 2 );
	end = start + axis[0] * -20.0f;
	DebugArrow( colorWhite, start, end, 2 );
	end = start + axis[1] * +20.0f;
	DebugArrow( colorGreen, start, end, 2 );
	end = start + axis[1] * -20.0f;
	DebugArrow( colorGreen, start, end, 2 );
	end = start + axis[2] * +20.0f;
	DebugArrow( colorBlue, start, end, 2 );
	end = start + axis[2] * -20.0f;
	DebugArrow( colorBlue, start, end, 2 );
}

/*
====================
idRenderWorldLocal::DebugClearPolygons
====================
*/
void idRenderWorldLocal::DebugClearPolygons( int time )
{
	RB_ClearDebugPolygons( time );
}

/*
====================
idRenderWorldLocal::DebugPolygon
====================
*/
void idRenderWorldLocal::DebugPolygon( const idVec4& color, const idWinding& winding, const int lifeTime, const bool depthTest )
{
	RB_AddDebugPolygon( color, winding, lifeTime, depthTest );
}

/*
================
idRenderWorldLocal::DebugScreenRect
================
*/
void idRenderWorldLocal::DebugScreenRect( const idVec4& color, const idScreenRect& rect, const viewDef_t* viewDef, const int lifetime )
{
	int i;
	float centerx, centery, dScale, hScale, vScale;
	idBounds bounds;
	idVec3 p[4];

	centerx = ( viewDef->viewport.x2 - viewDef->viewport.x1 ) * 0.5f;
	centery = ( viewDef->viewport.y2 - viewDef->viewport.y1 ) * 0.5f;

	dScale = r_znear.GetFloat() + 1.0f;
	hScale = dScale * idMath::Tan16( DEG2RAD( viewDef->renderView.fov_x * 0.5f ) );
	vScale = dScale * idMath::Tan16( DEG2RAD( viewDef->renderView.fov_y * 0.5f ) );

	bounds[0][0] = bounds[1][0] = dScale;
	bounds[0][1] = -( rect.x1 - centerx ) / centerx * hScale;
	bounds[1][1] = -( rect.x2 - centerx ) / centerx * hScale;
	bounds[0][2] = ( rect.y1 - centery ) / centery * vScale;
	bounds[1][2] = ( rect.y2 - centery ) / centery * vScale;

	for( i = 0; i < 4; i++ )
	{
		p[i].x = bounds[0][0];
		p[i].y = bounds[( i ^ ( i >> 1 ) ) & 1].y;
		p[i].z = bounds[( i >> 1 ) & 1].z;
		p[i] = viewDef->renderView.vieworg + p[i] * viewDef->renderView.viewaxis;
	}
	for( i = 0; i < 4; i++ )
	{
		DebugLine( color, p[i], p[( i + 1 ) & 3], false );
	}
}

/*
================
idRenderWorldLocal::DrawTextLength

  returns the length of the given text
================
*/
float idRenderWorldLocal::DrawTextLength( const char* text, float scale, int len )
{
	return RB_DrawTextLength( text, scale, len );
}

/*
================
idRenderWorldLocal::DrawText

  oriented on the viewaxis
  align can be 0-left, 1-center (default), 2-right
================
*/
void idRenderWorldLocal::DrawText( const char* text, const idVec3& origin, float scale, const idVec4& color, const idMat3& viewAxis, const int align, const int lifetime, const bool depthTest )
{
	RB_AddDebugText( text, origin, scale, color, viewAxis, align, lifetime, depthTest );
}

/*
===============
idRenderWorldLocal::RegenerateWorld
===============
*/
void idRenderWorldLocal::RegenerateWorld()
{
	R_FreeDerivedData();
	R_ReCreateWorldReferences();
}

/*
===============
R_GlobalShaderOverride
===============
*/
bool R_GlobalShaderOverride( const idMaterial** shader )
{

	if( !( *shader )->IsDrawn() )
	{
		return false;
	}

	if( tr.primaryRenderView.globalMaterial )
	{
		*shader = tr.primaryRenderView.globalMaterial;
		return true;
	}

	if( r_materialOverride.GetString()[0] != '\0' )
	{
		*shader = declManager->FindMaterial( r_materialOverride.GetString() );
		return true;
	}

	return false;
}

/*
===============
R_RemapShaderBySkin
===============
*/
const idMaterial* R_RemapShaderBySkin( const idMaterial* shader, const idDeclSkin* skin, const idMaterial* customShader )
{

	if( !shader )
	{
		return NULL;
	}

	// never remap surfaces that were originally nodraw, like collision hulls
	if( !shader->IsDrawn() )
	{
		return shader;
	}

	if( customShader )
	{
		// this is sort of a hack, but cause deformed surfaces to map to empty surfaces,
		// so the item highlight overlay doesn't highlight the autosprite surface
		if( shader->Deform() )
		{
			return NULL;
		}
		return const_cast<idMaterial*>( customShader );
	}

	if( !skin )
	{
		return const_cast<idMaterial*>( shader );
	}

	return skin->RemapShaderBySkin( shader );
}
