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

#ifndef __CLIP_H__
#define __CLIP_H__

/*
===============================================================================

  Handles collision detection with the world and between physics objects.

===============================================================================
*/

#define CLIPMODEL_ID_TO_JOINT_HANDLE( id )	( ( id ) >= 0 ? INVALID_JOINT : ((jointHandle_t) ( -1 - id )) )
#define JOINT_HANDLE_TO_CLIPMODEL_ID( id )	( -1 - id )

class idClip;
class idClipModel;
class idEntity;

//===============================================================
//
//	idClipModel
//
//===============================================================

class idClipModel
{

	friend class idClip;

public:
	idClipModel();
	explicit idClipModel( const char* name );
	explicit idClipModel( const idTraceModel& trm );
	explicit idClipModel( const idTraceModel& trm, bool persistantThroughSave );
	explicit idClipModel( const int renderModelHandle );
	explicit idClipModel( const idClipModel* model );
	~idClipModel();

	bool					LoadModel( const char* name );
	void					LoadModel( const idTraceModel& trm, bool persistantThroughSave = true );
	void					LoadModel( const int renderModelHandle );

	void					Save( idSaveGame* savefile ) const;
	void					Restore( idRestoreGame* savefile );

	void					Link( idClip& clp );				// must have been linked with an entity and id before
	void					Link( idClip& clp, idEntity* ent, int newId, const idVec3& newOrigin, const idMat3& newAxis, int renderModelHandle = -1 );
	void					Unlink();						// unlink from sectors
	void					SetPosition( const idVec3& newOrigin, const idMat3& newAxis );	// unlinks the clip model
	void					Translate( const idVec3& translation );							// unlinks the clip model
	void					Rotate( const idRotation& rotation );							// unlinks the clip model
	void					Enable();						// enable for clipping
	void					Disable();					// keep linked but disable for clipping
	void					SetMaterial( const idMaterial* m );
	const idMaterial* 		GetMaterial() const;
	void					SetContents( int newContents );		// override contents
	int						GetContents() const;
	void					SetEntity( idEntity* newEntity );
	idEntity* 				GetEntity() const;
	void					SetId( int newId );
	int						GetId() const;
	void					SetOwner( idEntity* newOwner );
	idEntity* 				GetOwner() const;
	const idBounds& 		GetBounds() const;
	const idBounds& 		GetAbsBounds() const;
	const idVec3& 			GetOrigin() const;
	const idMat3& 			GetAxis() const;
	bool					IsTraceModel() const;			// returns true if this is a trace model
	bool					IsRenderModel() const;		// returns true if this is a render model
	bool					IsLinked() const;				// returns true if the clip model is linked
	bool					IsEnabled() const;			// returns true if enabled for collision detection
	bool					IsEqual( const idTraceModel& trm ) const;
	cmHandle_t				Handle() const;				// returns handle used to collide vs this model
	const idTraceModel* 	GetTraceModel() const;
	void					GetMassProperties( const float density, float& mass, idVec3& centerOfMass, idMat3& inertiaTensor ) const;

	static cmHandle_t		CheckModel( const char* name );
	static void				ClearTraceModelCache();
	static int				TraceModelCacheSize();

	static void				SaveTraceModels( idSaveGame* savefile );
	static void				RestoreTraceModels( idRestoreGame* savefile );

private:
	bool					enabled;				// true if this clip model is used for clipping
	idEntity* 				entity;					// entity using this clip model
	int						id;						// id for entities that use multiple clip models
	idEntity* 				owner;					// owner of the entity that owns this clip model
	idVec3					origin;					// origin of clip model
	idMat3					axis;					// orientation of clip model
	idBounds				bounds;					// bounds
	idBounds				absBounds;				// absolute bounds
	const idMaterial* 		material;				// material for trace models
	int						contents;				// all contents ored together
	cmHandle_t				collisionModelHandle;	// handle to collision model
	int						traceModelIndex;		// trace model used for collision detection
	int						renderModelHandle;		// render model def handle

	struct clipLink_s* 		clipLinks;				// links into sectors
	int						touchCount;

	void					Init();			// initialize
	void					Link_r( struct clipSector_s* node );

	static int				AllocTraceModel( const idTraceModel& trm, bool persistantThroughSaves = true );
	static void				FreeTraceModel( int traceModelIndex );
	static idTraceModel* 	GetCachedTraceModel( int traceModelIndex );
	static int				GetTraceModelHashKey( const idTraceModel& trm );
	static struct trmCache_s* 		GetTraceModelEntry( int traceModelIndex );
};


ID_INLINE void idClipModel::Translate( const idVec3& translation )
{
	Unlink();
	origin += translation;
}

ID_INLINE void idClipModel::Rotate( const idRotation& rotation )
{
	Unlink();
	origin *= rotation;
	axis *= rotation.ToMat3();
}

ID_INLINE void idClipModel::Enable()
{
	enabled = true;
}

ID_INLINE void idClipModel::Disable()
{
	enabled = false;
}

ID_INLINE void idClipModel::SetMaterial( const idMaterial* m )
{
	material = m;
}

ID_INLINE const idMaterial* idClipModel::GetMaterial() const
{
	return material;
}

ID_INLINE void idClipModel::SetContents( int newContents )
{
	contents = newContents;
}

ID_INLINE int idClipModel::GetContents() const
{
	return contents;
}

ID_INLINE void idClipModel::SetEntity( idEntity* newEntity )
{
	entity = newEntity;
}

ID_INLINE idEntity* idClipModel::GetEntity() const
{
	return entity;
}

ID_INLINE void idClipModel::SetId( int newId )
{
	id = newId;
}

ID_INLINE int idClipModel::GetId() const
{
	return id;
}

ID_INLINE void idClipModel::SetOwner( idEntity* newOwner )
{
	owner = newOwner;
}

ID_INLINE idEntity* idClipModel::GetOwner() const
{
	return owner;
}

ID_INLINE const idBounds& idClipModel::GetBounds() const
{
	return bounds;
}

ID_INLINE const idBounds& idClipModel::GetAbsBounds() const
{
	return absBounds;
}

ID_INLINE const idVec3& idClipModel::GetOrigin() const
{
	return origin;
}

ID_INLINE const idMat3& idClipModel::GetAxis() const
{
	return axis;
}

ID_INLINE bool idClipModel::IsRenderModel() const
{
	return ( renderModelHandle != -1 );
}

ID_INLINE bool idClipModel::IsTraceModel() const
{
	return ( traceModelIndex != -1 );
}

ID_INLINE bool idClipModel::IsLinked() const
{
	return ( clipLinks != NULL );
}

ID_INLINE bool idClipModel::IsEnabled() const
{
	return enabled;
}

ID_INLINE bool idClipModel::IsEqual( const idTraceModel& trm ) const
{
	return ( traceModelIndex != -1 && *GetCachedTraceModel( traceModelIndex ) == trm );
}

ID_INLINE const idTraceModel* idClipModel::GetTraceModel() const
{
	if( !IsTraceModel() )
	{
		return NULL;
	}
	return idClipModel::GetCachedTraceModel( traceModelIndex );
}


//===============================================================
//
//	idClip
//
//===============================================================

class idClip
{

	friend class idClipModel;

public:
	idClip();

	void					Init();
	void					Shutdown();

	// clip versus the rest of the world
	bool					Translation( trace_t& results, const idVec3& start, const idVec3& end,
										 const idClipModel* mdl, const idMat3& trmAxis, int contentMask, const idEntity* passEntity );
	bool					Rotation( trace_t& results, const idVec3& start, const idRotation& rotation,
									  const idClipModel* mdl, const idMat3& trmAxis, int contentMask, const idEntity* passEntity );
	bool					Motion( trace_t& results, const idVec3& start, const idVec3& end, const idRotation& rotation,
									const idClipModel* mdl, const idMat3& trmAxis, int contentMask, const idEntity* passEntity );
	int						Contacts( contactInfo_t* contacts, const int maxContacts, const idVec3& start, const idVec6& dir, const float depth,
									  const idClipModel* mdl, const idMat3& trmAxis, int contentMask, const idEntity* passEntity );
	int						Contents( const idVec3& start,
									  const idClipModel* mdl, const idMat3& trmAxis, int contentMask, const idEntity* passEntity );

	// special case translations versus the rest of the world
	bool					TracePoint( trace_t& results, const idVec3& start, const idVec3& end,
										int contentMask, const idEntity* passEntity );
	bool					TraceBounds( trace_t& results, const idVec3& start, const idVec3& end, const idBounds& bounds,
										 int contentMask, const idEntity* passEntity );

	// clip versus a specific model
	void					TranslationModel( trace_t& results, const idVec3& start, const idVec3& end,
			const idClipModel* mdl, const idMat3& trmAxis, int contentMask,
			cmHandle_t model, const idVec3& modelOrigin, const idMat3& modelAxis );
	void					RotationModel( trace_t& results, const idVec3& start, const idRotation& rotation,
										   const idClipModel* mdl, const idMat3& trmAxis, int contentMask,
										   cmHandle_t model, const idVec3& modelOrigin, const idMat3& modelAxis );
	int						ContactsModel( contactInfo_t* contacts, const int maxContacts, const idVec3& start, const idVec6& dir, const float depth,
										   const idClipModel* mdl, const idMat3& trmAxis, int contentMask,
										   cmHandle_t model, const idVec3& modelOrigin, const idMat3& modelAxis );
	int						ContentsModel( const idVec3& start,
										   const idClipModel* mdl, const idMat3& trmAxis, int contentMask,
										   cmHandle_t model, const idVec3& modelOrigin, const idMat3& modelAxis );

	// clip versus all entities but not the world
	void					TranslationEntities( trace_t& results, const idVec3& start, const idVec3& end,
			const idClipModel* mdl, const idMat3& trmAxis, int contentMask, const idEntity* passEntity );

	// get a contact feature
	bool					GetModelContactFeature( const contactInfo_t& contact, const idClipModel* clipModel, idFixedWinding& winding ) const;

	// get entities/clip models within or touching the given bounds
	int						EntitiesTouchingBounds( const idBounds& bounds, int contentMask, idEntity** entityList, int maxCount ) const;
	int						ClipModelsTouchingBounds( const idBounds& bounds, int contentMask, idClipModel** clipModelList, int maxCount ) const;

	const idBounds& 		GetWorldBounds() const;
	idClipModel* 			DefaultClipModel();

	// stats and debug drawing
	void					PrintStatistics();
	void					DrawClipModels( const idVec3& eye, const float radius, const idEntity* passEntity );
	bool					DrawModelContactFeature( const contactInfo_t& contact, const idClipModel* clipModel, int lifetime ) const;

private:
	int						numClipSectors;
	struct clipSector_s* 	clipSectors;
	idBounds				worldBounds;
	idClipModel				temporaryClipModel;
	idClipModel				defaultClipModel;
	mutable int				touchCount;
	// statistics
	int						numTranslations;
	int						numRotations;
	int						numMotions;
	int						numRenderModelTraces;
	int						numContents;
	int						numContacts;

private:
	struct clipSector_s* 	CreateClipSectors_r( const int depth, const idBounds& bounds, idVec3& maxSector );
	void					ClipModelsTouchingBounds_r( const struct clipSector_s* node, struct listParms_s& parms ) const;
	const idTraceModel* 	TraceModelForClipModel( const idClipModel* mdl ) const;
	int						GetTraceClipModels( const idBounds& bounds, int contentMask, const idEntity* passEntity, idClipModel** clipModelList ) const;
	void					TraceRenderModel( trace_t& trace, const idVec3& start, const idVec3& end, const float radius, const idMat3& axis, idClipModel* touch ) const;
};


ID_INLINE bool idClip::TracePoint( trace_t& results, const idVec3& start, const idVec3& end, int contentMask, const idEntity* passEntity )
{
	Translation( results, start, end, NULL, mat3_identity, contentMask, passEntity );
	return ( results.fraction < 1.0f );
}

ID_INLINE bool idClip::TraceBounds( trace_t& results, const idVec3& start, const idVec3& end, const idBounds& bounds, int contentMask, const idEntity* passEntity )
{
	temporaryClipModel.LoadModel( idTraceModel( bounds ) );
	Translation( results, start, end, &temporaryClipModel, mat3_identity, contentMask, passEntity );
	return ( results.fraction < 1.0f );
}

ID_INLINE const idBounds& idClip::GetWorldBounds() const
{
	return worldBounds;
}

ID_INLINE idClipModel* idClip::DefaultClipModel()
{
	return &defaultClipModel;
}

#endif /* !__CLIP_H__ */
