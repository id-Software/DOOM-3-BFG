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


#include "../Game_local.h"

#define	MAX_SECTOR_DEPTH				12
#define MAX_SECTORS						((1<<(MAX_SECTOR_DEPTH+1))-1)

typedef struct clipSector_s
{
	int						axis;		// -1 = leaf node
	float					dist;
	struct clipSector_s* 	children[2];
	struct clipLink_s* 		clipLinks;
} clipSector_t;

typedef struct clipLink_s
{
	idClipModel* 			clipModel;
	struct clipSector_s* 	sector;
	struct clipLink_s* 		prevInSector;
	struct clipLink_s* 		nextInSector;
	struct clipLink_s* 		nextLink;
} clipLink_t;

typedef struct trmCache_s
{
	idTraceModel			trm;
	int						refCount;
	float					volume;
	idVec3					centerOfMass;
	idMat3					inertiaTensor;
} trmCache_t;

idVec3 vec3_boxEpsilon( CM_BOX_EPSILON, CM_BOX_EPSILON, CM_BOX_EPSILON );

idBlockAlloc<clipLink_t, 1024>	clipLinkAllocator;


/*
===============================================================

	idClipModel trace model cache

===============================================================
*/

static idList<trmCache_s*>		traceModelCache;
static idList<trmCache_s*>		traceModelCache_Unsaved;
static idHashIndex				traceModelHash;
static idHashIndex				traceModelHash_Unsaved;
const static int				TRACE_MODEL_SAVED = BIT( 16 );

// SRS - statically define the default trace model used for the default clip model
const static idTraceModel		defaultTraceModel = idBounds( idVec3( 0, 0, 0 ) ).Expand( 8 );

/*
===============
idClipModel::ClearTraceModelCache
===============
*/
void idClipModel::ClearTraceModelCache()
{
	traceModelCache.DeleteContents( true );
	traceModelCache_Unsaved.DeleteContents( true );
	traceModelHash.Free();
	traceModelHash_Unsaved.Free();
}

/*
===============
idClipModel::TraceModelCacheSize
===============
*/
int idClipModel::TraceModelCacheSize()
{
	return traceModelCache.Num() * sizeof( idTraceModel );
}

/*
===============
idClipModel::AllocTraceModel
===============
*/
int idClipModel::AllocTraceModel( const idTraceModel& trm, bool persistantThroughSaves )
{
	int i, hashKey, traceModelIndex;
	trmCache_t* entry;

	hashKey = GetTraceModelHashKey( trm );

	if( persistantThroughSaves )
	{
		// Look Inside the saved list.
		for( i = traceModelHash.First( hashKey ); i >= 0; i = traceModelHash.Next( i ) )
		{
			if( traceModelCache[i]->trm == trm )
			{
				traceModelCache[i]->refCount++;
				int flagged_index = i | TRACE_MODEL_SAVED;
				return flagged_index;
			}
		}
	}
	else
	{

		// Look inside the unsaved list.
		for( i = traceModelHash_Unsaved.First( hashKey ); i >= 0; i = traceModelHash_Unsaved.Next( i ) )
		{
			if( traceModelCache_Unsaved[i]->trm == trm )
			{
				traceModelCache_Unsaved[i]->refCount++;
				return i;
			}
		}
	}


	entry = new( TAG_PHYSICS_CLIP ) trmCache_t;
	entry->trm = trm;
	entry->trm.GetMassProperties( 1.0f, entry->volume, entry->centerOfMass, entry->inertiaTensor );
	entry->refCount = 1;

	if( persistantThroughSaves )
	{
		traceModelIndex = traceModelCache.Append( entry );
		traceModelHash.Add( hashKey, traceModelIndex );

		// Set the saved bit.
		traceModelIndex |= TRACE_MODEL_SAVED;

	}
	else
	{
		traceModelIndex = traceModelCache_Unsaved.Append( entry );
		traceModelHash_Unsaved.Add( hashKey, traceModelIndex );

		// remove the saved bit
		traceModelIndex &= ~TRACE_MODEL_SAVED;

	}

	return traceModelIndex;
}

/*
===============
idClipModel::FreeTraceModel
===============
*/
void idClipModel::FreeTraceModel( int traceModelIndex )
{

	int realTraceModelIndex = traceModelIndex & ~TRACE_MODEL_SAVED;

	// Check which cache we are using.
	if( traceModelIndex & TRACE_MODEL_SAVED )
	{

		if( realTraceModelIndex < 0 || realTraceModelIndex >= traceModelCache.Num() || traceModelCache[realTraceModelIndex]->refCount <= 0 )
		{
			gameLocal.Warning( "idClipModel::FreeTraceModel: tried to free uncached trace model" );
			return;
		}
		traceModelCache[realTraceModelIndex]->refCount--;

	}
	else
	{

		if( realTraceModelIndex < 0 || realTraceModelIndex >= traceModelCache_Unsaved.Num() || traceModelCache_Unsaved[realTraceModelIndex]->refCount <= 0 )
		{
			gameLocal.Warning( "idClipModel::FreeTraceModel: tried to free uncached trace model" );
			return;
		}
		traceModelCache_Unsaved[realTraceModelIndex]->refCount--;

	}
}

/*
===============
idClipModel::GetCachedTraceModel
===============
*/
idTraceModel* idClipModel::GetCachedTraceModel( int traceModelIndex )
{
	int realTraceModelIndex = traceModelIndex & ~TRACE_MODEL_SAVED;

	if( traceModelIndex & TRACE_MODEL_SAVED )
	{
		return &traceModelCache[realTraceModelIndex]->trm;
	}
	else
	{
		return &traceModelCache_Unsaved[realTraceModelIndex]->trm;
	}
}

/*
===============
idClipModel::GetCachedTraceModel
===============
*/
trmCache_t* idClipModel::GetTraceModelEntry( int traceModelIndex )
{

	int realTraceModelIndex = traceModelIndex & ~TRACE_MODEL_SAVED;

	if( traceModelIndex & TRACE_MODEL_SAVED )
	{
		return traceModelCache[realTraceModelIndex];
	}
	else
	{
		return traceModelCache_Unsaved[realTraceModelIndex];
	}
}

/*
===============
idClipModel::GetTraceModelHashKey
===============
*/
int idClipModel::GetTraceModelHashKey( const idTraceModel& trm )
{
	const idVec3& v = trm.bounds[0];
	return ( trm.type << 8 ) ^ ( trm.numVerts << 4 ) ^ ( trm.numEdges << 2 ) ^ ( trm.numPolys << 0 ) ^ idMath::FloatHash( v.ToFloatPtr(), v.GetDimension() );
}

/*
===============
idClipModel::SaveTraceModels
===============
*/
void idClipModel::SaveTraceModels( idSaveGame* savefile )
{
	int i;

	savefile->WriteInt( traceModelCache.Num() );
	for( i = 0; i < traceModelCache.Num(); i++ )
	{
		trmCache_t* entry = traceModelCache[i];

		savefile->WriteTraceModel( entry->trm );
		savefile->WriteFloat( entry->volume );
		savefile->WriteVec3( entry->centerOfMass );
		savefile->WriteMat3( entry->inertiaTensor );
	}
}

/*
===============
idClipModel::RestoreTraceModels
===============
*/
void idClipModel::RestoreTraceModels( idRestoreGame* savefile )
{
	int i, num;

	ClearTraceModelCache();

	savefile->ReadInt( num );
	traceModelCache.SetNum( num );

	for( i = 0; i < num; i++ )
	{
		trmCache_t* entry = new( TAG_PHYSICS_CLIP ) trmCache_t;

		savefile->ReadTraceModel( entry->trm );

		savefile->ReadFloat( entry->volume );
		savefile->ReadVec3( entry->centerOfMass );
		savefile->ReadMat3( entry->inertiaTensor );
		entry->refCount = 0;

		traceModelCache[i] = entry;
		traceModelHash.Add( GetTraceModelHashKey( entry->trm ), i );
	}

	// SRS - find or allocate default trace model here since it's not referenced by objects
	gameLocal.clip.DefaultClipModel()->traceModelIndex = AllocTraceModel( defaultTraceModel );
}


/*
===============================================================

	idClipModel

===============================================================
*/

/*
================
idClipModel::LoadModel
================
*/
bool idClipModel::LoadModel( const char* name )
{
	renderModelHandle = -1;
	if( traceModelIndex != -1 )
	{
		FreeTraceModel( traceModelIndex );
		traceModelIndex = -1;
	}
	collisionModelHandle = collisionModelManager->LoadModel( name, false );
	if( collisionModelHandle )
	{
		collisionModelManager->GetModelBounds( collisionModelHandle, bounds );
		collisionModelManager->GetModelContents( collisionModelHandle, contents );
		return true;
	}
	else
	{
		bounds.Zero();
		return false;
	}
}

/*
================
idClipModel::LoadModel
================
*/
void idClipModel::LoadModel( const idTraceModel& trm, bool persistantThroughSave )
{
	collisionModelHandle = 0;
	renderModelHandle = -1;
	if( traceModelIndex != -1 )
	{
		FreeTraceModel( traceModelIndex );
	}
	traceModelIndex = AllocTraceModel( trm, persistantThroughSave );
	bounds = trm.bounds;
}

/*
================
idClipModel::LoadModel
================
*/
void idClipModel::LoadModel( const int renderModelHandle )
{
	collisionModelHandle = 0;
	this->renderModelHandle = renderModelHandle;
	if( renderModelHandle != -1 )
	{
		const renderEntity_t* renderEntity = gameRenderWorld->GetRenderEntity( renderModelHandle );
		if( renderEntity )
		{
			bounds = renderEntity->bounds;
		}
	}
	if( traceModelIndex != -1 )
	{
		FreeTraceModel( traceModelIndex );
		traceModelIndex = -1;
	}
}

/*
================
idClipModel::Init
================
*/
void idClipModel::Init()
{
	enabled = true;
	entity = NULL;
	id = 0;
	owner = NULL;
	origin.Zero();
	axis.Identity();
	bounds.Zero();
	absBounds.Zero();
	material = NULL;
	contents = CONTENTS_BODY;
	collisionModelHandle = 0;
	renderModelHandle = -1;
	traceModelIndex = -1;
	clipLinks = NULL;
	touchCount = -1;
}

/*
================
idClipModel::idClipModel
================
*/
idClipModel::idClipModel()
{
	Init();
}

/*
================
idClipModel::idClipModel
================
*/
idClipModel::idClipModel( const char* name )
{
	Init();
	LoadModel( name );
}

/*
================
idClipModel::idClipModel
================
*/
idClipModel::idClipModel( const idTraceModel& trm )
{
	Init();
	LoadModel( trm, true );
}

/*
================
idClipModel::idClipModel
================
*/
idClipModel::idClipModel( const idTraceModel& trm, bool persistantThroughSave )
{
	Init();
	LoadModel( trm, persistantThroughSave );
}

/*
================
idClipModel::idClipModel
================
*/
idClipModel::idClipModel( const int renderModelHandle )
{
	Init();
	contents = CONTENTS_RENDERMODEL;
	LoadModel( renderModelHandle );
}

/*
================
idClipModel::idClipModel
================
*/
idClipModel::idClipModel( const idClipModel* model )
{
	enabled = model->enabled;
	entity = model->entity;
	id = model->id;
	owner = model->owner;
	origin = model->origin;
	axis = model->axis;
	bounds = model->bounds;
	absBounds = model->absBounds;
	material = model->material;
	contents = model->contents;
	collisionModelHandle = model->collisionModelHandle;
	traceModelIndex = -1;
	if( model->traceModelIndex != -1 )
	{
		LoadModel( *GetCachedTraceModel( model->traceModelIndex ) );
	}
	renderModelHandle = model->renderModelHandle;
	clipLinks = NULL;
	touchCount = -1;
}

/*
================
idClipModel::~idClipModel
================
*/
idClipModel::~idClipModel()
{
	// make sure the clip model is no longer linked
	Unlink();
	if( traceModelIndex != -1 )
	{
		FreeTraceModel( traceModelIndex );
	}
}

/*
================
idClipModel::Save
================
*/
void idClipModel::Save( idSaveGame* savefile ) const
{
	savefile->WriteBool( enabled );
	savefile->WriteObject( entity );
	savefile->WriteInt( id );
	savefile->WriteObject( owner );
	savefile->WriteVec3( origin );
	savefile->WriteMat3( axis );
	savefile->WriteBounds( bounds );
	savefile->WriteBounds( absBounds );
	savefile->WriteMaterial( material );
	savefile->WriteInt( contents );
	if( collisionModelHandle >= 0 )
	{
		savefile->WriteString( collisionModelManager->GetModelName( collisionModelHandle ) );
	}
	else
	{
		savefile->WriteString( "" );
	}
	savefile->WriteInt( traceModelIndex );
	savefile->WriteInt( renderModelHandle );
	savefile->WriteBool( clipLinks != NULL );
	savefile->WriteInt( touchCount );
}

/*
================
idClipModel::Restore
================
*/
void idClipModel::Restore( idRestoreGame* savefile )
{
	idStr collisionModelName;
	bool linked;

	savefile->ReadBool( enabled );
	savefile->ReadObject( reinterpret_cast<idClass*&>( entity ) );
	savefile->ReadInt( id );
	savefile->ReadObject( reinterpret_cast<idClass*&>( owner ) );
	savefile->ReadVec3( origin );
	savefile->ReadMat3( axis );
	savefile->ReadBounds( bounds );
	savefile->ReadBounds( absBounds );
	savefile->ReadMaterial( material );
	savefile->ReadInt( contents );
	savefile->ReadString( collisionModelName );
	if( collisionModelName.Length() )
	{
		collisionModelHandle = collisionModelManager->LoadModel( collisionModelName, false );
	}
	else
	{
		collisionModelHandle = -1;
	}
	savefile->ReadInt( traceModelIndex );
	if( traceModelIndex >= 0 )
	{
		int realIndex = traceModelIndex & ~TRACE_MODEL_SAVED;
		traceModelCache[realIndex]->refCount++;
	}
	savefile->ReadInt( renderModelHandle );
	savefile->ReadBool( linked );
	savefile->ReadInt( touchCount );

	// the render model will be set when the clip model is linked
	renderModelHandle = -1;
	clipLinks = NULL;
	touchCount = -1;

	if( linked )
	{
		Link( gameLocal.clip, entity, id, origin, axis, renderModelHandle );
	}
}

/*
================
idClipModel::SetPosition
================
*/
void idClipModel::SetPosition( const idVec3& newOrigin, const idMat3& newAxis )
{
	if( clipLinks )
	{
		Unlink();	// unlink from old position
	}
	origin = newOrigin;
	axis = newAxis;
}

/*
================
idClipModel::Handle
================
*/
cmHandle_t idClipModel::Handle() const
{
	assert( renderModelHandle == -1 );
	if( collisionModelHandle )
	{
		return collisionModelHandle;
	}
	else if( traceModelIndex != -1 )
	{
		return collisionModelManager->SetupTrmModel( *GetCachedTraceModel( traceModelIndex ), material );
	}
	else
	{
		// this happens in multiplayer on the combat models
		gameLocal.Warning( "idClipModel::Handle: clip model %d on '%s' (%x) is not a collision or trace model", id, entity->name.c_str(), entity->entityNumber );
		return 0;
	}
}

/*
================
idClipModel::GetMassProperties
================
*/
void idClipModel::GetMassProperties( const float density, float& mass, idVec3& centerOfMass, idMat3& inertiaTensor ) const
{
	if( traceModelIndex == -1 )
	{
		gameLocal.Error( "idClipModel::GetMassProperties: clip model %d on '%s' is not a trace model\n", id, entity->name.c_str() );
	}

	trmCache_t* entry = GetTraceModelEntry( traceModelIndex ); //traceModelCache[traceModelIndex];
	mass = entry->volume * density;
	centerOfMass = entry->centerOfMass;
	inertiaTensor = density * entry->inertiaTensor;
}

/*
===============
idClipModel::Unlink
===============
*/
void idClipModel::Unlink()
{
	clipLink_t* link;

	for( link = clipLinks; link; link = clipLinks )
	{
		clipLinks = link->nextLink;
		if( link->prevInSector )
		{
			link->prevInSector->nextInSector = link->nextInSector;
		}
		else
		{
			link->sector->clipLinks = link->nextInSector;
		}
		if( link->nextInSector )
		{
			link->nextInSector->prevInSector = link->prevInSector;
		}
		clipLinkAllocator.Free( link );
	}
}

/*
===============
idClipModel::Link_r
===============
*/
void idClipModel::Link_r( struct clipSector_s* node )
{
	clipLink_t* link;

	while( node->axis != -1 )
	{
		if( absBounds[0][node->axis] > node->dist )
		{
			node = node->children[0];
		}
		else if( absBounds[1][node->axis] < node->dist )
		{
			node = node->children[1];
		}
		else
		{
			Link_r( node->children[0] );
			node = node->children[1];
		}
	}

	link = clipLinkAllocator.Alloc();
	link->clipModel = this;
	link->sector = node;
	link->nextInSector = node->clipLinks;
	link->prevInSector = NULL;
	if( node->clipLinks )
	{
		node->clipLinks->prevInSector = link;
	}
	node->clipLinks = link;
	link->nextLink = clipLinks;
	clipLinks = link;
}

/*
===============
idClipModel::Link
===============
*/
void idClipModel::Link( idClip& clp )
{

	assert( idClipModel::entity );
	if( !idClipModel::entity )
	{
		return;
	}

	if( clipLinks )
	{
		Unlink();	// unlink from old position
	}

	if( bounds.IsCleared() )
	{
		return;
	}

	// set the abs box
	if( axis.IsRotated() )
	{
		// expand for rotation
		absBounds.FromTransformedBounds( bounds, origin, axis );
	}
	else
	{
		// normal
		absBounds[0] = bounds[0] + origin;
		absBounds[1] = bounds[1] + origin;
	}

	// because movement is clipped an epsilon away from an actual edge,
	// we must fully check even when bounding boxes don't quite touch
	absBounds[0] -= vec3_boxEpsilon;
	absBounds[1] += vec3_boxEpsilon;

	Link_r( clp.clipSectors );
}

/*
===============
idClipModel::Link
===============
*/
void idClipModel::Link( idClip& clp, idEntity* ent, int newId, const idVec3& newOrigin, const idMat3& newAxis, int renderModelHandle )
{

	this->entity = ent;
	this->id = newId;
	this->origin = newOrigin;
	this->axis = newAxis;
	if( renderModelHandle != -1 )
	{
		this->renderModelHandle = renderModelHandle;
		const renderEntity_t* renderEntity = gameRenderWorld->GetRenderEntity( renderModelHandle );
		if( renderEntity )
		{
			this->bounds = renderEntity->bounds;
		}
	}
	this->Link( clp );
}

/*
============
idClipModel::CheckModel
============
*/
cmHandle_t idClipModel::CheckModel( const char* name )
{
	return collisionModelManager->LoadModel( name, false );
}


/*
===============================================================

	idClip

===============================================================
*/

/*
===============
idClip::idClip
===============
*/
idClip::idClip()
{
	numClipSectors = 0;
	clipSectors = NULL;
	worldBounds.Zero();
	numRotations = numTranslations = numMotions = numRenderModelTraces = numContents = numContacts = 0;
}

/*
===============
idClip::CreateClipSectors_r

Builds a uniformly subdivided tree for the given world size
===============
*/
clipSector_t* idClip::CreateClipSectors_r( const int depth, const idBounds& bounds, idVec3& maxSector )
{
	int				i;
	clipSector_t*	anode;
	idVec3			size;
	idBounds		front, back;

	anode = &clipSectors[idClip::numClipSectors];
	idClip::numClipSectors++;

	if( depth == MAX_SECTOR_DEPTH )
	{
		anode->axis = -1;
		anode->children[0] = anode->children[1] = NULL;

		for( i = 0; i < 3; i++ )
		{
			if( bounds[1][i] - bounds[0][i] > maxSector[i] )
			{
				maxSector[i] = bounds[1][i] - bounds[0][i];
			}
		}
		return anode;
	}

	size = bounds[1] - bounds[0];
	if( size[0] >= size[1] && size[0] >= size[2] )
	{
		anode->axis = 0;
	}
	else if( size[1] >= size[0] && size[1] >= size[2] )
	{
		anode->axis = 1;
	}
	else
	{
		anode->axis = 2;
	}

	anode->dist = 0.5f * ( bounds[1][anode->axis] + bounds[0][anode->axis] );

	front = bounds;
	back = bounds;

	front[0][anode->axis] = back[1][anode->axis] = anode->dist;

	anode->children[0] = CreateClipSectors_r( depth + 1, front, maxSector );
	anode->children[1] = CreateClipSectors_r( depth + 1, back, maxSector );

	return anode;
}

/*
===============
idClip::Init
===============
*/
void idClip::Init()
{
	cmHandle_t h;
	idVec3 size, maxSector = vec3_origin;

	// clear clip sectors
	clipSectors = new( TAG_PHYSICS_CLIP ) clipSector_t[MAX_SECTORS];
	memset( clipSectors, 0, MAX_SECTORS * sizeof( clipSector_t ) );
	numClipSectors = 0;
	touchCount = -1;

	// get world map bounds
	h = collisionModelManager->LoadModel( "worldMap", false );
	collisionModelManager->GetModelBounds( h, worldBounds );

	// create world sectors
	CreateClipSectors_r( 0, worldBounds, maxSector );

	size = worldBounds[1] - worldBounds[0];
	gameLocal.Printf( "map bounds are (%1.1f, %1.1f, %1.1f)\n", size[0], size[1], size[2] );
	gameLocal.Printf( "max clip sector is (%1.1f, %1.1f, %1.1f)\n", maxSector[0], maxSector[1], maxSector[2] );

	// initialize a default clip model
	defaultClipModel.LoadModel( defaultTraceModel );

	// set counters to zero
	numRotations = numTranslations = numMotions = numRenderModelTraces = numContents = numContacts = 0;
}

/*
===============
idClip::Shutdown
===============
*/
void idClip::Shutdown()
{
	delete[] clipSectors;
	clipSectors = NULL;

	// free the trace model used for the temporaryClipModel
	if( temporaryClipModel.traceModelIndex != -1 )
	{
		idClipModel::FreeTraceModel( temporaryClipModel.traceModelIndex );
		temporaryClipModel.traceModelIndex = -1;
	}

	// free the trace model used for the defaultClipModel
	if( defaultClipModel.traceModelIndex != -1 )
	{
		idClipModel::FreeTraceModel( defaultClipModel.traceModelIndex );
		defaultClipModel.traceModelIndex = -1;
	}

	clipLinkAllocator.Shutdown();
}

/*
====================
idClip::ClipModelsTouchingBounds_r
====================
*/
typedef struct listParms_s
{
	idBounds		bounds;
	int				contentMask;
	idClipModel**		list;
	int				count;
	int				maxCount;
} listParms_t;

void idClip::ClipModelsTouchingBounds_r( const struct clipSector_s* node, listParms_t& parms ) const
{

	while( node->axis != -1 )
	{
		if( parms.bounds[0][node->axis] > node->dist )
		{
			node = node->children[0];
		}
		else if( parms.bounds[1][node->axis] < node->dist )
		{
			node = node->children[1];
		}
		else
		{
			ClipModelsTouchingBounds_r( node->children[0], parms );
			node = node->children[1];
		}
	}

	for( clipLink_t* link = node->clipLinks; link; link = link->nextInSector )
	{
		idClipModel*	check = link->clipModel;

		// if the clip model is enabled
		if( !check->enabled )
		{
			continue;
		}

		// avoid duplicates in the list
		if( check->touchCount == touchCount )
		{
			continue;
		}

		// if the clip model does not have any contents we are looking for
		if( !( check->contents & parms.contentMask ) )
		{
			continue;
		}

		// if the bounds really do overlap
		if(	check->absBounds[0][0] > parms.bounds[1][0] ||
				check->absBounds[1][0] < parms.bounds[0][0] ||
				check->absBounds[0][1] > parms.bounds[1][1] ||
				check->absBounds[1][1] < parms.bounds[0][1] ||
				check->absBounds[0][2] > parms.bounds[1][2] ||
				check->absBounds[1][2] < parms.bounds[0][2] )
		{
			continue;
		}

		if( parms.count >= parms.maxCount )
		{
			gameLocal.Warning( "idClip::ClipModelsTouchingBounds_r: max count" );
			return;
		}

		check->touchCount = touchCount;
		parms.list[parms.count] = check;
		parms.count++;
	}
}

/*
================
idClip::ClipModelsTouchingBounds
================
*/
int idClip::ClipModelsTouchingBounds( const idBounds& bounds, int contentMask, idClipModel** clipModelList, int maxCount ) const
{
	listParms_t parms;

	if(	bounds[0][0] > bounds[1][0] ||
			bounds[0][1] > bounds[1][1] ||
			bounds[0][2] > bounds[1][2] )
	{
		// we should not go through the tree for degenerate or backwards bounds
		assert( false );
		return 0;
	}

	parms.bounds[0] = bounds[0] - vec3_boxEpsilon;
	parms.bounds[1] = bounds[1] + vec3_boxEpsilon;
	parms.contentMask = contentMask;
	parms.list = clipModelList;
	parms.count = 0;
	parms.maxCount = maxCount;

	touchCount++;
	ClipModelsTouchingBounds_r( clipSectors, parms );

	return parms.count;
}

/*
================
idClip::EntitiesTouchingBounds
================
*/
int idClip::EntitiesTouchingBounds( const idBounds& bounds, int contentMask, idEntity** entityList, int maxCount ) const
{
	idClipModel* clipModelList[MAX_GENTITIES];
	int i, j, count, entCount;

	count = idClip::ClipModelsTouchingBounds( bounds, contentMask, clipModelList, MAX_GENTITIES );
	entCount = 0;
	for( i = 0; i < count; i++ )
	{
		// entity could already be in the list because an entity can use multiple clip models
		for( j = 0; j < entCount; j++ )
		{
			if( entityList[j] == clipModelList[i]->entity )
			{
				break;
			}
		}
		if( j >= entCount )
		{
			if( entCount >= maxCount )
			{
				gameLocal.Warning( "idClip::EntitiesTouchingBounds: max count" );
				return entCount;
			}
			entityList[entCount] = clipModelList[i]->entity;
			entCount++;
		}
	}

	return entCount;
}

/*
====================
idClip::GetTraceClipModels

  an ent will be excluded from testing if:
  cm->entity == passEntity ( don't clip against the pass entity )
  cm->entity == passOwner ( missiles don't clip with owner )
  cm->owner == passEntity ( don't interact with your own missiles )
  cm->owner == passOwner ( don't interact with other missiles from same owner )
====================
*/
int idClip::GetTraceClipModels( const idBounds& bounds, int contentMask, const idEntity* passEntity, idClipModel** clipModelList ) const
{
	int i, num;
	idClipModel*	cm;
	idEntity* passOwner;

	num = ClipModelsTouchingBounds( bounds, contentMask, clipModelList, MAX_GENTITIES );

	if( !passEntity )
	{
		return num;
	}

	if( passEntity->GetPhysics()->GetNumClipModels() > 0 )
	{
		passOwner = passEntity->GetPhysics()->GetClipModel()->GetOwner();
	}
	else
	{
		passOwner = NULL;
	}

	for( i = 0; i < num; i++ )
	{

		cm = clipModelList[i];

		// check if we should ignore this entity
		if( cm->entity == passEntity )
		{
			clipModelList[i] = NULL;			// don't clip against the pass entity
		}
		else if( cm->entity == passOwner )
		{
			clipModelList[i] = NULL;			// missiles don't clip with their owner
		}
		else if( cm->owner )
		{
			if( cm->owner == passEntity )
			{
				clipModelList[i] = NULL;		// don't clip against own missiles
			}
			else if( cm->owner == passOwner )
			{
				clipModelList[i] = NULL;		// don't clip against other missiles from same owner
			}
		}
	}

	return num;
}

/*
============
idClip::TraceRenderModel
============
*/
void idClip::TraceRenderModel( trace_t& trace, const idVec3& start, const idVec3& end, const float radius, const idMat3& axis, idClipModel* touch ) const
{
	trace.fraction = 1.0f;

	// if the trace is passing through the bounds
	if( touch->absBounds.Expand( radius ).LineIntersection( start, end ) )
	{
		modelTrace_t modelTrace;

		// test with exact render model and modify trace_t structure accordingly
		if( gameRenderWorld->ModelTrace( modelTrace, touch->renderModelHandle, start, end, radius ) )
		{
			trace.fraction = modelTrace.fraction;
			trace.endAxis = axis;
			trace.endpos = modelTrace.point;
			trace.c.normal = modelTrace.normal;
			trace.c.dist = modelTrace.point * modelTrace.normal;
			trace.c.point = modelTrace.point;
			trace.c.type = CONTACT_TRMVERTEX;
			trace.c.modelFeature = 0;
			trace.c.trmFeature = 0;
			trace.c.contents = modelTrace.material->GetContentFlags();
			trace.c.material = modelTrace.material;
			// NOTE: trace.c.id will be the joint number
			touch->id = JOINT_HANDLE_TO_CLIPMODEL_ID( modelTrace.jointNumber );
		}
	}
}

/*
============
idClip::TraceModelForClipModel
============
*/
const idTraceModel* idClip::TraceModelForClipModel( const idClipModel* mdl ) const
{
	if( !mdl )
	{
		return NULL;
	}
	else
	{
		if( !mdl->IsTraceModel() )
		{
			if( mdl->GetEntity() )
			{
				gameLocal.Error( "TraceModelForClipModel: clip model %d on '%s' is not a trace model\n", mdl->GetId(), mdl->GetEntity()->name.c_str() );
			}
			else
			{
				gameLocal.Error( "TraceModelForClipModel: clip model %d is not a trace model\n", mdl->GetId() );
			}
		}
		return idClipModel::GetCachedTraceModel( mdl->traceModelIndex );
	}
}

/*
============
idClip::TestHugeTranslation
============
*/
ID_INLINE bool TestHugeTranslation( trace_t& results, const idClipModel* mdl, const idVec3& start, const idVec3& end, const idMat3& trmAxis )
{
	if( mdl != NULL && ( end - start ).LengthSqr() > Square( CM_MAX_TRACE_DIST ) )
	{
#ifndef CTF
		// May be important: This occurs in CTF when a player connects and spawns
		// in the PVS of a player that has a flag that is spawning the idMoveableItem
		// "nuggets".  The error seems benign and the assert was getting in the way
		// of testing.
		assert( 0 );
#endif

		results.fraction = 0.0f;
		results.endpos = start;
		results.endAxis = trmAxis;
		memset( &results.c, 0, sizeof( results.c ) );
		results.c.point = start;

		if( mdl->GetEntity() )
		{
			gameLocal.Printf( "huge translation for clip model %d on entity %d '%s'\n", mdl->GetId(), mdl->GetEntity()->entityNumber, mdl->GetEntity()->GetName() );
		}
		else
		{
			gameLocal.Printf( "huge translation for clip model %d\n", mdl->GetId() );
		}
		return true;
	}
	return false;
}

/*
============
idClip::TranslationEntities
============
*/
void idClip::TranslationEntities( trace_t& results, const idVec3& start, const idVec3& end,
								  const idClipModel* mdl, const idMat3& trmAxis, int contentMask, const idEntity* passEntity )
{
	int i, num;
	idClipModel* touch, *clipModelList[MAX_GENTITIES];
	idBounds traceBounds;
	float radius;
	trace_t trace;
	const idTraceModel* trm;

	if( TestHugeTranslation( results, mdl, start, end, trmAxis ) )
	{
		return;
	}

	trm = TraceModelForClipModel( mdl );

	results.fraction = 1.0f;
	results.endpos = end;
	results.endAxis = trmAxis;

	if( !trm )
	{
		traceBounds.FromPointTranslation( start, end - start );
		radius = 0.0f;
	}
	else
	{
		traceBounds.FromBoundsTranslation( trm->bounds, start, trmAxis, end - start );
		radius = trm->bounds.GetRadius();
	}

	num = GetTraceClipModels( traceBounds, contentMask, passEntity, clipModelList );

	for( i = 0; i < num; i++ )
	{
		touch = clipModelList[i];

		if( !touch )
		{
			continue;
		}

		if( touch->renderModelHandle != -1 )
		{
			idClip::numRenderModelTraces++;
			TraceRenderModel( trace, start, end, radius, trmAxis, touch );
		}
		else
		{
			idClip::numTranslations++;
			collisionModelManager->Translation( &trace, start, end, trm, trmAxis, contentMask,
												touch->Handle(), touch->origin, touch->axis );
		}

		if( trace.fraction < results.fraction )
		{
			results = trace;
			results.c.entityNum = touch->entity->entityNumber;
			results.c.id = touch->id;
			if( results.fraction == 0.0f )
			{
				break;
			}
		}
	}
}

/*
============
idClip::Translation
============
*/
bool idClip::Translation( trace_t& results, const idVec3& start, const idVec3& end,
						  const idClipModel* mdl, const idMat3& trmAxis, int contentMask, const idEntity* passEntity )
{
	int i, num;
	idClipModel* touch, *clipModelList[MAX_GENTITIES];
	idBounds traceBounds;
	float radius;
	trace_t trace;
	const idTraceModel* trm;

	if( TestHugeTranslation( results, mdl, start, end, trmAxis ) )
	{
		return true;
	}

	trm = TraceModelForClipModel( mdl );

	if( !passEntity || passEntity->entityNumber != ENTITYNUM_WORLD )
	{
		// test world
		idClip::numTranslations++;
		collisionModelManager->Translation( &results, start, end, trm, trmAxis, contentMask, 0, vec3_origin, mat3_default );
		results.c.entityNum = results.fraction != 1.0f ? ENTITYNUM_WORLD : ENTITYNUM_NONE;
		if( results.fraction == 0.0f )
		{
			return true;		// blocked immediately by the world
		}
	}
	else
	{
		memset( &results, 0, sizeof( results ) );
		results.fraction = 1.0f;
		results.endpos = end;
		results.endAxis = trmAxis;
	}

	if( !trm )
	{
		traceBounds.FromPointTranslation( start, results.endpos - start );
		radius = 0.0f;
	}
	else
	{
		traceBounds.FromBoundsTranslation( trm->bounds, start, trmAxis, results.endpos - start );
		radius = trm->bounds.GetRadius();
	}

	num = GetTraceClipModels( traceBounds, contentMask, passEntity, clipModelList );

	for( i = 0; i < num; i++ )
	{
		touch = clipModelList[i];

		if( !touch )
		{
			continue;
		}

		if( touch->renderModelHandle != -1 )
		{
			idClip::numRenderModelTraces++;
			TraceRenderModel( trace, start, end, radius, trmAxis, touch );
		}
		else
		{
			idClip::numTranslations++;
			collisionModelManager->Translation( &trace, start, end, trm, trmAxis, contentMask,
												touch->Handle(), touch->origin, touch->axis );
		}

		if( trace.fraction < results.fraction )
		{
			results = trace;
			results.c.entityNum = touch->entity->entityNumber;
			results.c.id = touch->id;
			if( results.fraction == 0.0f )
			{
				break;
			}
		}
	}

	return ( results.fraction < 1.0f );
}

/*
============
idClip::Rotation
============
*/
bool idClip::Rotation( trace_t& results, const idVec3& start, const idRotation& rotation,
					   const idClipModel* mdl, const idMat3& trmAxis, int contentMask, const idEntity* passEntity )
{
	int i, num;
	idClipModel* touch, *clipModelList[MAX_GENTITIES];
	idBounds traceBounds;
	trace_t trace;
	const idTraceModel* trm;

	trm = TraceModelForClipModel( mdl );

	if( !passEntity || passEntity->entityNumber != ENTITYNUM_WORLD )
	{
		// test world
		idClip::numRotations++;
		collisionModelManager->Rotation( &results, start, rotation, trm, trmAxis, contentMask, 0, vec3_origin, mat3_default );
		results.c.entityNum = results.fraction != 1.0f ? ENTITYNUM_WORLD : ENTITYNUM_NONE;
		if( results.fraction == 0.0f )
		{
			return true;		// blocked immediately by the world
		}
	}
	else
	{
		memset( &results, 0, sizeof( results ) );
		results.fraction = 1.0f;
		results.endpos = start;
		results.endAxis = trmAxis * rotation.ToMat3();
	}

	if( !trm )
	{
		traceBounds.FromPointRotation( start, rotation );
	}
	else
	{
		traceBounds.FromBoundsRotation( trm->bounds, start, trmAxis, rotation );
	}

	num = GetTraceClipModels( traceBounds, contentMask, passEntity, clipModelList );

	for( i = 0; i < num; i++ )
	{
		touch = clipModelList[i];

		if( !touch )
		{
			continue;
		}

		// no rotational collision with render models
		if( touch->renderModelHandle != -1 )
		{
			continue;
		}

		idClip::numRotations++;
		collisionModelManager->Rotation( &trace, start, rotation, trm, trmAxis, contentMask,
										 touch->Handle(), touch->origin, touch->axis );

		if( trace.fraction < results.fraction )
		{
			results = trace;
			results.c.entityNum = touch->entity->entityNumber;
			results.c.id = touch->id;
			if( results.fraction == 0.0f )
			{
				break;
			}
		}
	}

	return ( results.fraction < 1.0f );
}

/*
============
idClip::Motion
============
*/
bool idClip::Motion( trace_t& results, const idVec3& start, const idVec3& end, const idRotation& rotation,
					 const idClipModel* mdl, const idMat3& trmAxis, int contentMask, const idEntity* passEntity )
{
	int i, num;
	idClipModel* touch, *clipModelList[MAX_GENTITIES];
	idVec3 dir, endPosition;
	idBounds traceBounds;
	float radius;
	trace_t translationalTrace, rotationalTrace, trace;
	idRotation endRotation;
	const idTraceModel* trm;

	assert( rotation.GetOrigin() == start );

	if( TestHugeTranslation( results, mdl, start, end, trmAxis ) )
	{
		return true;
	}

	if( mdl != NULL && rotation.GetAngle() != 0.0f && rotation.GetVec() != vec3_origin )
	{
		// if no translation
		if( start == end )
		{
			// pure rotation
			return Rotation( results, start, rotation, mdl, trmAxis, contentMask, passEntity );
		}
	}
	else if( start != end )
	{
		// pure translation
		return Translation( results, start, end, mdl, trmAxis, contentMask, passEntity );
	}
	else
	{
		// no motion
		results.fraction = 1.0f;
		results.endpos = start;
		results.endAxis = trmAxis;
		return false;
	}

	trm = TraceModelForClipModel( mdl );

	radius = trm->bounds.GetRadius();

	if( !passEntity || passEntity->entityNumber != ENTITYNUM_WORLD )
	{
		// translational collision with world
		idClip::numTranslations++;
		collisionModelManager->Translation( &translationalTrace, start, end, trm, trmAxis, contentMask, 0, vec3_origin, mat3_default );
		translationalTrace.c.entityNum = translationalTrace.fraction != 1.0f ? ENTITYNUM_WORLD : ENTITYNUM_NONE;
	}
	else
	{
		memset( &translationalTrace, 0, sizeof( translationalTrace ) );
		translationalTrace.fraction = 1.0f;
		translationalTrace.endpos = end;
		translationalTrace.endAxis = trmAxis;
	}

	if( translationalTrace.fraction != 0.0f )
	{

		traceBounds.FromBoundsRotation( trm->bounds, start, trmAxis, rotation );
		dir = translationalTrace.endpos - start;
		for( i = 0; i < 3; i++ )
		{
			if( dir[i] < 0.0f )
			{
				traceBounds[0][i] += dir[i];
			}
			else
			{
				traceBounds[1][i] += dir[i];
			}
		}

		num = GetTraceClipModels( traceBounds, contentMask, passEntity, clipModelList );

		for( i = 0; i < num; i++ )
		{
			touch = clipModelList[i];

			if( !touch )
			{
				continue;
			}

			if( touch->renderModelHandle != -1 )
			{
				idClip::numRenderModelTraces++;
				TraceRenderModel( trace, start, end, radius, trmAxis, touch );
			}
			else
			{
				idClip::numTranslations++;
				collisionModelManager->Translation( &trace, start, end, trm, trmAxis, contentMask,
													touch->Handle(), touch->origin, touch->axis );
			}

			if( trace.fraction < translationalTrace.fraction )
			{
				translationalTrace = trace;
				translationalTrace.c.entityNum = touch->entity->entityNumber;
				translationalTrace.c.id = touch->id;
				if( translationalTrace.fraction == 0.0f )
				{
					break;
				}
			}
		}
	}
	else
	{
		num = -1;
	}

	endPosition = translationalTrace.endpos;
	endRotation = rotation;
	endRotation.SetOrigin( endPosition );

	if( !passEntity || passEntity->entityNumber != ENTITYNUM_WORLD )
	{
		// rotational collision with world
		idClip::numRotations++;
		collisionModelManager->Rotation( &rotationalTrace, endPosition, endRotation, trm, trmAxis, contentMask, 0, vec3_origin, mat3_default );
		rotationalTrace.c.entityNum = rotationalTrace.fraction != 1.0f ? ENTITYNUM_WORLD : ENTITYNUM_NONE;
	}
	else
	{
		memset( &rotationalTrace, 0, sizeof( rotationalTrace ) );
		rotationalTrace.fraction = 1.0f;
		rotationalTrace.endpos = endPosition;
		rotationalTrace.endAxis = trmAxis * rotation.ToMat3();
	}

	if( rotationalTrace.fraction != 0.0f )
	{

		if( num == -1 )
		{
			traceBounds.FromBoundsRotation( trm->bounds, endPosition, trmAxis, endRotation );
			num = GetTraceClipModels( traceBounds, contentMask, passEntity, clipModelList );
		}

		for( i = 0; i < num; i++ )
		{
			touch = clipModelList[i];

			if( !touch )
			{
				continue;
			}

			// no rotational collision detection with render models
			if( touch->renderModelHandle != -1 )
			{
				continue;
			}

			idClip::numRotations++;
			collisionModelManager->Rotation( &trace, endPosition, endRotation, trm, trmAxis, contentMask,
											 touch->Handle(), touch->origin, touch->axis );

			if( trace.fraction < rotationalTrace.fraction )
			{
				rotationalTrace = trace;
				rotationalTrace.c.entityNum = touch->entity->entityNumber;
				rotationalTrace.c.id = touch->id;
				if( rotationalTrace.fraction == 0.0f )
				{
					break;
				}
			}
		}
	}

	if( rotationalTrace.fraction < 1.0f )
	{
		results = rotationalTrace;
	}
	else
	{
		results = translationalTrace;
		results.endAxis = rotationalTrace.endAxis;
	}

	results.fraction = Max( translationalTrace.fraction, rotationalTrace.fraction );

	return ( translationalTrace.fraction < 1.0f || rotationalTrace.fraction < 1.0f );
}

/*
============
idClip::Contacts
============
*/
int idClip::Contacts( contactInfo_t* contacts, const int maxContacts, const idVec3& start, const idVec6& dir, const float depth,
					  const idClipModel* mdl, const idMat3& trmAxis, int contentMask, const idEntity* passEntity )
{
	int i, j, num, n, numContacts;
	idClipModel* touch, *clipModelList[MAX_GENTITIES];
	idBounds traceBounds;
	const idTraceModel* trm;

	trm = TraceModelForClipModel( mdl );

	if( !passEntity || passEntity->entityNumber != ENTITYNUM_WORLD )
	{
		// test world
		idClip::numContacts++;
		numContacts = collisionModelManager->Contacts( contacts, maxContacts, start, dir, depth, trm, trmAxis, contentMask, 0, vec3_origin, mat3_default );
	}
	else
	{
		numContacts = 0;
	}

	for( i = 0; i < numContacts; i++ )
	{
		contacts[i].entityNum = ENTITYNUM_WORLD;
		contacts[i].id = 0;
	}

	if( numContacts >= maxContacts )
	{
		return numContacts;
	}

	if( !trm )
	{
		traceBounds = idBounds( start ).Expand( depth );
	}
	else
	{
		traceBounds.FromTransformedBounds( trm->bounds, start, trmAxis );
		traceBounds.ExpandSelf( depth );
	}

	num = GetTraceClipModels( traceBounds, contentMask, passEntity, clipModelList );

	for( i = 0; i < num; i++ )
	{
		touch = clipModelList[i];

		if( !touch )
		{
			continue;
		}

		// no contacts with render models
		if( touch->renderModelHandle != -1 )
		{
			continue;
		}

		idClip::numContacts++;
		n = collisionModelManager->Contacts( contacts + numContacts, maxContacts - numContacts,
											 start, dir, depth, trm, trmAxis, contentMask,
											 touch->Handle(), touch->origin, touch->axis );

		for( j = 0; j < n; j++ )
		{
			contacts[numContacts].entityNum = touch->entity->entityNumber;
			contacts[numContacts].id = touch->id;
			numContacts++;
		}

		if( numContacts >= maxContacts )
		{
			break;
		}
	}

	return numContacts;
}

/*
============
idClip::Contents
============
*/
int idClip::Contents( const idVec3& start, const idClipModel* mdl, const idMat3& trmAxis, int contentMask, const idEntity* passEntity )
{
	int i, num, contents;
	idClipModel* touch, *clipModelList[MAX_GENTITIES];
	idBounds traceBounds;
	const idTraceModel* trm;

	trm = TraceModelForClipModel( mdl );

	if( !passEntity || passEntity->entityNumber != ENTITYNUM_WORLD )
	{
		// test world
		idClip::numContents++;
		contents = collisionModelManager->Contents( start, trm, trmAxis, contentMask, 0, vec3_origin, mat3_default );
	}
	else
	{
		contents = 0;
	}

	if( !trm )
	{
		traceBounds[0] = start;
		traceBounds[1] = start;
	}
	else if( trmAxis.IsRotated() )
	{
		traceBounds.FromTransformedBounds( trm->bounds, start, trmAxis );
	}
	else
	{
		traceBounds[0] = trm->bounds[0] + start;
		traceBounds[1] = trm->bounds[1] + start;
	}

	num = GetTraceClipModels( traceBounds, -1, passEntity, clipModelList );

	for( i = 0; i < num; i++ )
	{
		touch = clipModelList[i];

		if( !touch )
		{
			continue;
		}

		// no contents test with render models
		if( touch->renderModelHandle != -1 )
		{
			continue;
		}

		// if the entity does not have any contents we are looking for
		if( ( touch->contents & contentMask ) == 0 )
		{
			continue;
		}

		// if the entity has no new contents flags
		if( ( touch->contents & contents ) == touch->contents )
		{
			continue;
		}

		idClip::numContents++;
		if( collisionModelManager->Contents( start, trm, trmAxis, contentMask, touch->Handle(), touch->origin, touch->axis ) )
		{
			contents |= ( touch->contents & contentMask );
		}
	}

	return contents;
}

/*
============
idClip::TranslationModel
============
*/
void idClip::TranslationModel( trace_t& results, const idVec3& start, const idVec3& end,
							   const idClipModel* mdl, const idMat3& trmAxis, int contentMask,
							   cmHandle_t model, const idVec3& modelOrigin, const idMat3& modelAxis )
{
	const idTraceModel* trm = TraceModelForClipModel( mdl );
	idClip::numTranslations++;
	collisionModelManager->Translation( &results, start, end, trm, trmAxis, contentMask, model, modelOrigin, modelAxis );
}

/*
============
idClip::RotationModel
============
*/
void idClip::RotationModel( trace_t& results, const idVec3& start, const idRotation& rotation,
							const idClipModel* mdl, const idMat3& trmAxis, int contentMask,
							cmHandle_t model, const idVec3& modelOrigin, const idMat3& modelAxis )
{
	const idTraceModel* trm = TraceModelForClipModel( mdl );
	idClip::numRotations++;
	collisionModelManager->Rotation( &results, start, rotation, trm, trmAxis, contentMask, model, modelOrigin, modelAxis );
}

/*
============
idClip::ContactsModel
============
*/
int idClip::ContactsModel( contactInfo_t* contacts, const int maxContacts, const idVec3& start, const idVec6& dir, const float depth,
						   const idClipModel* mdl, const idMat3& trmAxis, int contentMask,
						   cmHandle_t model, const idVec3& modelOrigin, const idMat3& modelAxis )
{
	const idTraceModel* trm = TraceModelForClipModel( mdl );
	idClip::numContacts++;
	return collisionModelManager->Contacts( contacts, maxContacts, start, dir, depth, trm, trmAxis, contentMask, model, modelOrigin, modelAxis );
}

/*
============
idClip::ContentsModel
============
*/
int idClip::ContentsModel( const idVec3& start,
						   const idClipModel* mdl, const idMat3& trmAxis, int contentMask,
						   cmHandle_t model, const idVec3& modelOrigin, const idMat3& modelAxis )
{
	const idTraceModel* trm = TraceModelForClipModel( mdl );
	idClip::numContents++;
	return collisionModelManager->Contents( start, trm, trmAxis, contentMask, model, modelOrigin, modelAxis );
}

/*
============
idClip::GetModelContactFeature
============
*/
bool idClip::GetModelContactFeature( const contactInfo_t& contact, const idClipModel* clipModel, idFixedWinding& winding ) const
{
	int i;
	cmHandle_t handle;
	idVec3 start, end;

	handle = -1;
	winding.Clear();

	if( clipModel == NULL )
	{
		handle = 0;
	}
	else
	{
		if( clipModel->renderModelHandle != -1 )
		{
			winding += contact.point;
			return true;
		}
		else if( clipModel->traceModelIndex != -1 )
		{
			handle = collisionModelManager->SetupTrmModel( *idClipModel::GetCachedTraceModel( clipModel->traceModelIndex ), clipModel->material );
		}
		else
		{
			handle = clipModel->collisionModelHandle;
		}
	}

	// if contact with a collision model
	if( handle != -1 )
	{
		switch( contact.type )
		{
			case CONTACT_EDGE:
			{
				// the model contact feature is a collision model edge
				collisionModelManager->GetModelEdge( handle, contact.modelFeature, start, end );
				winding += start;
				winding += end;
				break;
			}
			case CONTACT_MODELVERTEX:
			{
				// the model contact feature is a collision model vertex
				collisionModelManager->GetModelVertex( handle, contact.modelFeature, start );
				winding += start;
				break;
			}
			case CONTACT_TRMVERTEX:
			{
				// the model contact feature is a collision model polygon
				collisionModelManager->GetModelPolygon( handle, contact.modelFeature, winding );
				break;
			}
		}
	}

	// transform the winding to world space
	if( clipModel )
	{
		for( i = 0; i < winding.GetNumPoints(); i++ )
		{
			winding[i].ToVec3() *= clipModel->axis;
			winding[i].ToVec3() += clipModel->origin;
		}
	}

	return true;
}

/*
============
idClip::PrintStatistics
============
*/
void idClip::PrintStatistics()
{
	gameLocal.Printf( "t = %-3d, r = %-3d, m = %-3d, render = %-3d, contents = %-3d, contacts = %-3d\n",
					  numTranslations, numRotations, numMotions, numRenderModelTraces, numContents, numContacts );
	numRotations = numTranslations = numMotions = numRenderModelTraces = numContents = numContacts = 0;
}

/*
============
idClip::DrawClipModels
============
*/
void idClip::DrawClipModels( const idVec3& eye, const float radius, const idEntity* passEntity )
{
	int				i, num;
	idBounds		bounds;
	idClipModel*		clipModelList[MAX_GENTITIES];
	idClipModel*		clipModel;

	bounds = idBounds( eye ).Expand( radius );

	num = idClip::ClipModelsTouchingBounds( bounds, -1, clipModelList, MAX_GENTITIES );

	for( i = 0; i < num; i++ )
	{
		clipModel = clipModelList[i];
		if( clipModel->GetEntity() == passEntity )
		{
			continue;
		}
		if( clipModel->renderModelHandle != -1 )
		{
			gameRenderWorld->DebugBounds( colorCyan, clipModel->GetAbsBounds() );
		}
		else
		{
			collisionModelManager->DrawModel( clipModel->Handle(), clipModel->GetOrigin(), clipModel->GetAxis(), eye, radius );
		}
	}
}

/*
============
idClip::DrawModelContactFeature
============
*/
bool idClip::DrawModelContactFeature( const contactInfo_t& contact, const idClipModel* clipModel, int lifetime ) const
{
	int i;
	idMat3 axis;
	idFixedWinding winding;

	if( !GetModelContactFeature( contact, clipModel, winding ) )
	{
		return false;
	}

	axis = contact.normal.ToMat3();

	if( winding.GetNumPoints() == 1 )
	{
		gameRenderWorld->DebugLine( colorCyan, winding[0].ToVec3(), winding[0].ToVec3() + 2.0f * axis[0], lifetime );
		gameRenderWorld->DebugLine( colorWhite, winding[0].ToVec3() - 1.0f * axis[1], winding[0].ToVec3() + 1.0f * axis[1], lifetime );
		gameRenderWorld->DebugLine( colorWhite, winding[0].ToVec3() - 1.0f * axis[2], winding[0].ToVec3() + 1.0f * axis[2], lifetime );
	}
	else
	{
		for( i = 0; i < winding.GetNumPoints(); i++ )
		{
			gameRenderWorld->DebugLine( colorCyan, winding[i].ToVec3(), winding[( i + 1 ) % winding.GetNumPoints()].ToVec3(), lifetime );
		}
	}

	axis[0] = -axis[0];
	axis[2] = -axis[2];
	gameRenderWorld->DrawText( contact.material->GetName(), winding.GetCenter() - 4.0f * axis[2], 0.1f, colorWhite, axis, 1, 5000 );

	return true;
}
