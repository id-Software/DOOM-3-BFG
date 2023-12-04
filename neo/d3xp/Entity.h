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

#ifndef __GAME_ENTITY_H__
#define __GAME_ENTITY_H__

/*
===============================================================================

	Game entity base class.

===============================================================================
*/

static const int DELAY_DORMANT_TIME = 3000;

extern const idEventDef EV_PostSpawn;
extern const idEventDef EV_FindTargets;
extern const idEventDef EV_Touch;
extern const idEventDef EV_Use;
extern const idEventDef EV_Activate;
extern const idEventDef EV_ActivateTargets;
extern const idEventDef EV_Hide;
extern const idEventDef EV_Show;
extern const idEventDef EV_GetShaderParm;
extern const idEventDef EV_SetShaderParm;
extern const idEventDef EV_SetOwner;
extern const idEventDef EV_GetAngles;
extern const idEventDef EV_SetAngles;
extern const idEventDef EV_SetLinearVelocity;
extern const idEventDef EV_SetAngularVelocity;
extern const idEventDef EV_SetSkin;
extern const idEventDef EV_StartSoundShader;
extern const idEventDef EV_StopSound;
extern const idEventDef EV_CacheSoundShader;

// Think flags
enum
{
	TH_ALL					= -1,
	TH_THINK				= 1,		// run think function each frame
	TH_PHYSICS				= 2,		// run physics each frame
	TH_ANIMATE				= 4,		// update animation each frame
	TH_UPDATEVISUALS		= 8,		// update renderEntity
	TH_UPDATEPARTICLES		= 16
};

//
// Signals
// make sure to change script/doom_defs.script if you add any, or change their order
//
typedef enum
{
	SIG_TOUCH,				// object was touched
	SIG_USE,				// object was used
	SIG_TRIGGER,			// object was activated
	SIG_REMOVED,			// object was removed from the game
	SIG_DAMAGE,				// object was damaged
	SIG_BLOCKED,			// object was blocked

	SIG_MOVER_POS1,			// mover at position 1 (door closed)
	SIG_MOVER_POS2,			// mover at position 2 (door open)
	SIG_MOVER_1TO2,			// mover changing from position 1 to 2
	SIG_MOVER_2TO1,			// mover changing from position 2 to 1

	NUM_SIGNALS
} signalNum_t;

// FIXME: At some point we may want to just limit it to one thread per signal, but
// for now, I'm allowing multiple threads.  We should reevaluate this later in the project
#define MAX_SIGNAL_THREADS 16		// probably overkill, but idList uses a granularity of 16

struct signal_t
{
	int					threadnum;
	const function_t*	function;
};

class signalList_t
{
public:
	idList<signal_t, TAG_ENTITY> signal[ NUM_SIGNALS ];
};


/*
================================================
idNetEvent

Utility for detecting a bool state change:
-server calls ::Set
-client ::Get will return true (once only)

Useful because:
-Hides client from having to manually declare "last" state and manually checking against it
-using int counter prevents problems w/ dropped snapshots

(ie if we just serialized a bool to true for a single ss, if that ss is dropped,skipped,whatever
the client would never handle it. By incrementing a wrapped counter, we are guaranteed to detect
the state change no matter what happens at the net layer).
================================================
*/
template < int max >
struct idNetEvent
{
	idNetEvent() : count( 0 ), lastCount( 0 ) { }
	void	Set()
	{
		count = ( ( count + 1 ) % max );
	}
	bool	Get()
	{
		if( count != lastCount )
		{
			lastCount = count;
			return true;
		}
		return false;
	}
	void	Serialize( idSerializer& ser )
	{
		if( count >= max )
		{
			idLib::Warning( "idNetEvent. count %d > max %d", count, max );
		}
		ser.SerializeUMax( count, max );
	}

public:
	static const int	Maximum = max;
	int		count;
	int		lastCount;
};

typedef idNetEvent< 7 > netBoolEvent_t;

inline void	WriteToBitMsg( const netBoolEvent_t& netEvent, idBitMsg& msg )
{
	msg.WriteBits( netEvent.count, idMath::BitsForInteger( netBoolEvent_t::Maximum ) );

	assert( netEvent.count <= netBoolEvent_t::Maximum );
}

inline void	ReadFromBitMsg( netBoolEvent_t& netEvent, const idBitMsg& msg )
{
	netEvent.count = msg.ReadBits( idMath::BitsForInteger( netBoolEvent_t::Maximum ) );

	assert( netEvent.count <= netBoolEvent_t::Maximum );
}


class idEntity : public idClass
{
public:
	static const int		MAX_PVS_AREAS = 4;
	static const uint32		INVALID_PREDICTION_KEY = 0xFFFFFFFF;

	int						entityNumber;			// index into the entity list
	int						entityDefNumber;		// index into the entity def list

	idLinkList<idEntity>	spawnNode;				// for being linked into spawnedEntities list
	idLinkList<idEntity>	activeNode;				// for being linked into activeEntities list
	idLinkList<idEntity>	aimAssistNode;			// linked into gameLocal.aimAssistEntities

	idLinkList<idEntity>	snapshotNode;			// for being linked into snapshotEntities list
	int						snapshotChanged;		// used to detect snapshot state changes
	int						snapshotBits;			// number of bits this entity occupied in the last snapshot
	bool					snapshotStale;			// Set to true if this entity is considered stale in the snapshot

	idStr					name;					// name of entity
	idDict					spawnArgs;				// key/value pairs used to spawn and initialize entity
	idScriptObject			scriptObject;			// contains all script defined data for this entity

	int						thinkFlags;				// TH_? flags
	int						dormantStart;			// time that the entity was first closed off from player
	bool					cinematic;				// during cinematics, entity will only think if cinematic is set

	renderView_t* 			renderView;				// for camera views from this entity
	idEntity* 				cameraTarget;			// any remoteRenderMap shaders will use this

	idList< idEntityPtr<idEntity>, TAG_ENTITY >	targets;		// when this entity is activated these entities entity are activated

	int						health;					// FIXME: do all objects really need health?

	struct entityFlags_s
	{
		bool				notarget			: 1;	// if true never attack or target this entity
		bool				noknockback			: 1;	// if true no knockback from hits
		bool				takedamage			: 1;	// if true this entity can be damaged
		bool				hidden				: 1;	// if true this entity is not visible
		bool				bindOrientated		: 1;	// if true both the master orientation is used for binding
		bool				solidForTeam		: 1;	// if true this entity is considered solid when a physics team mate pushes entities
		bool				forcePhysicsUpdate	: 1;	// if true always update from the physics whether the object moved or not
		bool				selected			: 1;	// if true the entity is selected for editing
		bool				neverDormant		: 1;	// if true the entity never goes dormant
		bool				isDormant			: 1;	// if true the entity is dormant
		bool				hasAwakened			: 1;	// before a monster has been awakened the first time, use full PVS for dormant instead of area-connected
		bool				networkSync			: 1; // if true the entity is synchronized over the network
		bool				grabbed				: 1;	// if true object is currently being grabbed
		bool				skipReplication		: 1; // don't replicate this entity over the network.
	} fl;

	int						timeGroup;

	bool					noGrab;

	renderEntity_t			xrayEntity;
	qhandle_t				xrayEntityHandle;
	const idDeclSkin* 		xraySkin;

	void					DetermineTimeGroup( bool slowmo );

	void					SetGrabbedState( bool grabbed );
	bool					IsGrabbed();

public:
	ABSTRACT_PROTOTYPE( idEntity );

	idEntity();
	~idEntity();

	void					Spawn();

	void					Save( idSaveGame* savefile ) const;
	void					Restore( idRestoreGame* savefile );

	const char* 			GetEntityDefName() const;
	void					SetName( const char* name );
	const char* 			GetName() const;
	virtual void			UpdateChangeableSpawnArgs( const idDict* source );
	int						GetEntityNumber() const
	{
		return entityNumber;
	}

	// clients generate views based on all the player specific options,
	// cameras have custom code, and everything else just uses the axis orientation
	virtual renderView_t* 	GetRenderView();

	// thinking
	virtual void			Think();
	bool					CheckDormant();	// dormant == on the active list, but out of PVS
	virtual	void			DormantBegin();	// called when entity becomes dormant
	virtual	void			DormantEnd();		// called when entity wakes from being dormant
	bool					IsActive() const;
	void					BecomeActive( int flags );
	void					BecomeInactive( int flags );
	void					UpdatePVSAreas( const idVec3& pos );
	void					BecomeReplicated();

	// visuals
	virtual void			Present();
	virtual renderEntity_t* GetRenderEntity();
	virtual int				GetModelDefHandle();
	virtual void			SetModel( const char* modelname );
	void					SetSkin( const idDeclSkin* skin );
	const idDeclSkin* 		GetSkin() const;
	void					SetShaderParm( int parmnum, float value );
	virtual void			SetColor( float red, float green, float blue );
	virtual void			SetColor( const idVec3& color );
	virtual void			GetColor( idVec3& out ) const;
	virtual void			SetColor( const idVec4& color );
	virtual void			GetColor( idVec4& out ) const;
	virtual void			FreeModelDef();
	virtual void			FreeLightDef();
	virtual void			Hide();
	virtual void			Show();
	bool					IsHidden() const;
	void					UpdateVisuals();
	void					UpdateModel();
	void					UpdateModelTransform();
	virtual void			ProjectOverlay( const idVec3& origin, const idVec3& dir, float size, const char* material );
	int						GetNumPVSAreas();
	const int* 				GetPVSAreas();
	void					ClearPVSAreas();
	bool					PhysicsTeamInPVS( pvsHandle_t pvsHandle );

	// animation
	virtual bool			UpdateAnimationControllers();
	bool					UpdateRenderEntity( renderEntity_t* renderEntity, const renderView_t* renderView );
	static bool				ModelCallback( renderEntity_t* renderEntity, const renderView_t* renderView );
	virtual idAnimator* 	GetAnimator();	// returns animator object used by this entity

	// sound
	virtual bool			CanPlayChatterSounds() const;
	bool					StartSound( const char* soundName, const s_channelType channel, int soundShaderFlags, bool broadcast, int* length );
	bool					StartSoundShader( const idSoundShader* shader, const s_channelType channel, int soundShaderFlags, bool broadcast, int* length );
	void					StopSound( const s_channelType channel, bool broadcast );	// pass SND_CHANNEL_ANY to stop all sounds
	void					SetSoundVolume( float volume );
	void					UpdateSound();
	int						GetListenerId() const;
	idSoundEmitter* 		GetSoundEmitter() const;
	void					FreeSoundEmitter( bool immediate );

	// entity binding
	virtual void			PreBind();
	virtual void			PostBind();
	virtual void			PreUnbind();
	virtual void			PostUnbind();
	void					JoinTeam( idEntity* teammember );
	void					Bind( idEntity* master, bool orientated );
	void					BindToJoint( idEntity* master, const char* jointname, bool orientated );
	void					BindToJoint( idEntity* master, jointHandle_t jointnum, bool orientated );
	void					BindToBody( idEntity* master, int bodyId, bool orientated );
	void					Unbind();
	bool					IsBound() const;
	bool					IsBoundTo( idEntity* master ) const;
	idEntity* 				GetBindMaster() const;
	jointHandle_t			GetBindJoint() const;
	int						GetBindBody() const;
	idEntity* 				GetTeamMaster() const;
	idEntity* 				GetNextTeamEntity() const;
	void					ConvertLocalToWorldTransform( idVec3& offset, idMat3& axis );
	idVec3					GetLocalVector( const idVec3& vec ) const;
	idVec3					GetLocalCoordinates( const idVec3& vec ) const;
	idVec3					GetWorldVector( const idVec3& vec ) const;
	idVec3					GetWorldCoordinates( const idVec3& vec ) const;
	bool					GetMasterPosition( idVec3& masterOrigin, idMat3& masterAxis ) const;
	void					GetWorldVelocities( idVec3& linearVelocity, idVec3& angularVelocity ) const;

	// physics
	// set a new physics object to be used by this entity
	void					SetPhysics( idPhysics* phys );

	// get the physics object used by this entity
	idPhysics* 				GetPhysics() const;

	// restore physics pointer for save games
	void					RestorePhysics( idPhysics* phys );

	// run the physics for this entity
	bool					RunPhysics();

	// Interpolates the physics, used on MP clients.
	void					InterpolatePhysics( const float fraction );

	// InterpolatePhysics actually calls evaluate, this version doesn't.
	void					InterpolatePhysicsOnly( const float fraction, bool updateTeam = false );

	// set the origin of the physics object (relative to bindMaster if not NULL)
	void					SetOrigin( const idVec3& org );

	// set the axis of the physics object (relative to bindMaster if not NULL)
	void					SetAxis( const idMat3& axis );

	// use angles to set the axis of the physics object (relative to bindMaster if not NULL)
	void					SetAngles( const idAngles& ang );

	// get the floor position underneath the physics object
	bool					GetFloorPos( float max_dist, idVec3& floorpos ) const;

	// retrieves the transformation going from the physics origin/axis to the visual origin/axis
	virtual bool			GetPhysicsToVisualTransform( idVec3& origin, idMat3& axis );

	// retrieves the transformation going from the physics origin/axis to the sound origin/axis
	virtual bool			GetPhysicsToSoundTransform( idVec3& origin, idMat3& axis );

	// called from the physics object when colliding, should return true if the physics simulation should stop
	virtual bool			Collide( const trace_t& collision, const idVec3& velocity );

	// retrieves impact information, 'ent' is the entity retrieving the info
	virtual void			GetImpactInfo( idEntity* ent, int id, const idVec3& point, impactInfo_t* info );

	// apply an impulse to the physics object, 'ent' is the entity applying the impulse
	virtual void			ApplyImpulse( idEntity* ent, int id, const idVec3& point, const idVec3& impulse );

	// add a force to the physics object, 'ent' is the entity adding the force
	virtual void			AddForce( idEntity* ent, int id, const idVec3& point, const idVec3& force );

	// activate the physics object, 'ent' is the entity activating this entity
	virtual void			ActivatePhysics( idEntity* ent );

	// returns true if the physics object is at rest
	virtual bool			IsAtRest() const;

	// returns the time the physics object came to rest
	virtual int				GetRestStartTime() const;

	// add a contact entity
	virtual void			AddContactEntity( idEntity* ent );

	// remove a touching entity
	virtual void			RemoveContactEntity( idEntity* ent );

	// damage
	// returns true if this entity can be damaged from the given origin
	virtual bool			CanDamage( const idVec3& origin, idVec3& damagePoint ) const;

	// applies damage to this entity
	virtual	void			Damage( idEntity* inflictor, idEntity* attacker, const idVec3& dir, const char* damageDefName, const float damageScale, const int location );

	// adds a damage effect like overlays, blood, sparks, debris etc.
	virtual void			AddDamageEffect( const trace_t& collision, const idVec3& velocity, const char* damageDefName );

	// callback function for when another entity received damage from this entity.  damage can be adjusted and returned to the caller.
	virtual void			DamageFeedback( idEntity* victim, idEntity* inflictor, int& damage );

	// notifies this entity that it is in pain
	virtual bool			Pain( idEntity* inflictor, idEntity* attacker, int damage, const idVec3& dir, int location );

	// notifies this entity that is has been killed
	virtual void			Killed( idEntity* inflictor, idEntity* attacker, int damage, const idVec3& dir, int location );

	// scripting
	virtual bool			ShouldConstructScriptObjectAtSpawn() const;
	virtual idThread* 		ConstructScriptObject();
	virtual void			DeconstructScriptObject();
	void					SetSignal( signalNum_t signalnum, idThread* thread, const function_t* function );
	void					ClearSignal( idThread* thread, signalNum_t signalnum );
	void					ClearSignalThread( signalNum_t signalnum, idThread* thread );
	bool					HasSignal( signalNum_t signalnum ) const;
	void					Signal( signalNum_t signalnum );
	void					SignalEvent( idThread* thread, signalNum_t signalnum );

	// gui
	void					TriggerGuis();
	bool					HandleGuiCommands( idEntity* entityGui, const char* cmds );
	virtual bool			HandleSingleGuiCommand( idEntity* entityGui, idLexer* src );

	// targets
	void					FindTargets();
	void					RemoveNullTargets();
	void					ActivateTargets( idEntity* activator ) const;

	// misc
	virtual void			Teleport( const idVec3& origin, const idAngles& angles, idEntity* destination );
	bool					TouchTriggers() const;
	idCurve_Spline<idVec3>* GetSpline() const;
	virtual void			ShowEditingDialog();

	enum
	{
		EVENT_STARTSOUNDSHADER,
		EVENT_STOPSOUNDSHADER,
		EVENT_MAXEVENTS
	};

	// Called on clients in an MP game, does the actual interpolation for the entity.
	// This function will eventually replace ClientPredictionThink completely.
	virtual void			ClientThink( const int curTime, const float fraction, const bool predict );

	virtual void			ClientPredictionThink();
	virtual void			WriteToSnapshot( idBitMsg& msg ) const;
	void					ReadFromSnapshot_Ex( const idBitMsg& msg );
	virtual void			ReadFromSnapshot( const idBitMsg& msg );
	virtual bool			ServerReceiveEvent( int event, int time, const idBitMsg& msg );
	virtual bool			ClientReceiveEvent( int event, int time, const idBitMsg& msg );

	void					WriteBindToSnapshot( idBitMsg& msg ) const;
	void					ReadBindFromSnapshot( const idBitMsg& msg );
	void					WriteColorToSnapshot( idBitMsg& msg ) const;
	void					ReadColorFromSnapshot( const idBitMsg& msg );
	void					WriteGUIToSnapshot( idBitMsg& msg ) const;
	void					ReadGUIFromSnapshot( const idBitMsg& msg );

	void					ServerSendEvent( int eventId, const idBitMsg* msg, bool saveEvent, lobbyUserID_t excluding = lobbyUserID_t() ) const;
	void					ClientSendEvent( int eventId, const idBitMsg* msg ) const;

	void					SetUseClientInterpolation( bool use )
	{
		useClientInterpolation = use;
	}

	void					SetSkipReplication( const bool skip )
	{
		fl.skipReplication = skip;
	}
	bool					GetSkipReplication() const
	{
		return fl.skipReplication;
	}
	bool					IsReplicated() const
	{
		return  GetEntityNumber() < ENTITYNUM_FIRST_NON_REPLICATED;
	}

	void					CreateDeltasFromOldOriginAndAxis( const idVec3& oldOrigin, const idMat3& oldAxis );
	void					DecayOriginAndAxisDelta();
	uint32					GetPredictedKey()
	{
		return predictionKey;
	}
	void					SetPredictedKey( uint32 key_ )
	{
		predictionKey = key_;
	}

	void					FlagNewSnapshot();

	idEntity*				GetTeamChain()
	{
		return teamChain;
	}

	// It is only safe to interpolate if this entity has received two snapshots.
	enum interpolationBehavior_t
	{
		USE_NO_INTERPOLATION,
		USE_LATEST_SNAP_ONLY,
		USE_INTERPOLATION
	};

	interpolationBehavior_t GetInterpolationBehavior() const
	{
		return interpolationBehavior;
	}
	unsigned int			GetNumSnapshotsReceived() const
	{
		return snapshotsReceived;
	}

protected:
	renderEntity_t			renderEntity;						// used to present a model to the renderer
	int						modelDefHandle;						// handle to static renderer model
	refSound_t				refSound;							// used to present sound to the audio engine

	idVec3					GetOriginDelta() const
	{
		return originDelta;
	}
	idMat3					GetAxisDelta() const
	{
		return axisDelta;
	}

private:
	idPhysics_Static		defaultPhysicsObj;					// default physics object
	idPhysics* 				physics;							// physics used for this entity
	idEntity* 				bindMaster;							// entity bound to if unequal NULL
	jointHandle_t			bindJoint;							// joint bound to if unequal INVALID_JOINT
	int						bindBody;							// body bound to if unequal -1
	idEntity* 				teamMaster;							// master of the physics team
	idEntity* 				teamChain;							// next entity in physics team
	bool					useClientInterpolation;				// disables interpolation for some objects (handy for weapon world models)
	int						numPVSAreas;						// number of renderer areas the entity covers
	int						PVSAreas[MAX_PVS_AREAS];			// numbers of the renderer areas the entity covers

	signalList_t* 			signals;

	int						mpGUIState;							// local cache to avoid systematic SetStateInt

	uint32					predictionKey;						// Unique key used to sync predicted ents (projectiles) in MP.

	// Delta values that are set when the server or client disagree on where the render model should be. If this happens,
	// they resolve it through DecayOriginAndAxisDelta()
	idVec3					originDelta;
	idMat3					axisDelta;

	interpolationBehavior_t	interpolationBehavior;
	unsigned int			snapshotsReceived;

private:
	void					FixupLocalizedStrings();

	bool					DoDormantTests();				// dormant == on the active list, but out of PVS

	// physics
	// initialize the default physics
	void					InitDefaultPhysics( const idVec3& origin, const idMat3& axis, const idDeclEntityDef* def );

	// update visual position from the physics
	void					UpdateFromPhysics( bool moveBack );

	// get physics timestep
	virtual int				GetPhysicsTimeStep() const;

	// entity binding
	bool					InitBind( idEntity* master );		// initialize an entity binding
	void					FinishBind();					// finish an entity binding
	void					RemoveBinds();				// deletes any entities bound to this object
	void					QuitTeam();					// leave the current team

	void					UpdatePVSAreas();

	// events
public:
// jmarshall
	idVec3					GetOrigin() const;
	float					DistanceTo( idEntity* ent );
	float					DistanceTo( const idVec3& pos ) const;
	idStr					GetNextKey( const char* prefix, const char* lastMatch );
// jmarshall end

	idVec3					GetOriginBrushOffset() const;
	virtual idVec3			GetEditOrigin() const;			// RB: used by idEditEntities

	void					Event_GetName();
	void					Event_SetName( const char* name );
	void					Event_FindTargets();
	void					Event_ActivateTargets( idEntity* activator );
	void					Event_NumTargets();
	void					Event_GetTarget( float index );
	void					Event_RandomTarget( const char* ignore );
	void					Event_Bind( idEntity* master );
	void					Event_BindPosition( idEntity* master );
	void					Event_BindToJoint( idEntity* master, const char* jointname, float orientated );
	void					Event_Unbind();
	void					Event_RemoveBinds();
	void					Event_SpawnBind();
	void					Event_SetOwner( idEntity* owner );
	void					Event_SetModel( const char* modelname );
	void					Event_SetSkin( const char* skinname );
	void					Event_GetShaderParm( int parmnum );
	void					Event_SetShaderParm( int parmnum, float value );
	void					Event_SetShaderParms( float parm0, float parm1, float parm2, float parm3 );
	void					Event_SetColor( float red, float green, float blue );
	void					Event_GetColor();
	void					Event_IsHidden();
	void					Event_Hide();
	void					Event_Show();
	void					Event_CacheSoundShader( const char* soundName );
	void					Event_StartSoundShader( const char* soundName, int channel );
	void					Event_StopSound( int channel, int netSync );
	void					Event_StartSound( const char* soundName, int channel, int netSync );
	void					Event_FadeSound( int channel, float to, float over );
	void					Event_GetWorldOrigin();
	void					Event_SetWorldOrigin( idVec3 const& org );
	void					Event_GetOrigin();
	void					Event_SetOrigin( const idVec3& org );
	void					Event_GetAngles();
	void					Event_SetAngles( const idAngles& ang );
	void					Event_SetLinearVelocity( const idVec3& velocity );
	void					Event_GetLinearVelocity();
	void					Event_SetAngularVelocity( const idVec3& velocity );
	void					Event_GetAngularVelocity();
	void					Event_SetSize( const idVec3& mins, const idVec3& maxs );
	void					Event_GetSize();
	void					Event_GetMins();
	void					Event_GetMaxs();
	void					Event_Touches( idEntity* ent );
	void					Event_SetGuiParm( const char* key, const char* val );
	void					Event_SetGuiFloat( const char* key, float f );
	void					Event_GetNextKey( const char* prefix, const char* lastMatch );
	void					Event_SetKey( const char* key, const char* value );
	void					Event_GetKey( const char* key );
	void					Event_GetIntKey( const char* key );
	void					Event_GetFloatKey( const char* key );
	void					Event_GetVectorKey( const char* key );
	void					Event_GetEntityKey( const char* key );
	void					Event_RestorePosition();
	void					Event_UpdateCameraTarget();
	void					Event_DistanceTo( idEntity* ent );
	void					Event_DistanceToPoint( const idVec3& point );
	void					Event_StartFx( const char* fx );
	void					Event_WaitFrame();
	void					Event_Wait( float time );
	void					Event_HasFunction( const char* name );
	void					Event_CallFunction( const char* name );
	void					Event_SetNeverDormant( int enable );
	void					Event_SetGui( int guiNum, const char* guiName );
	void					Event_PrecacheGui( const char* guiName );
	void					Event_GetGuiParm( int guiNum, const char* key );
	void					Event_GetGuiParmFloat( int guiNum, const char* key );
	void					Event_GuiNamedEvent( int guiNum, const char* event );
};

ID_INLINE float idEntity::DistanceTo( idEntity* ent )
{
	return DistanceTo( ent->GetPhysics()->GetOrigin() );
}

ID_INLINE float idEntity::DistanceTo( const idVec3& pos ) const
{
	return ( pos - GetPhysics()->GetOrigin() ).LengthFast();
}

/*
===============================================================================

	Animated entity base class.

===============================================================================
*/

typedef struct damageEffect_s
{
	jointHandle_t			jointNum;
	idVec3					localOrigin;
	idVec3					localNormal;
	int						time;
	const idDeclParticle*	type;
	struct damageEffect_s* 	next;
} damageEffect_t;

class idAnimatedEntity : public idEntity
{
public:
	CLASS_PROTOTYPE( idAnimatedEntity );

	idAnimatedEntity();
	~idAnimatedEntity();

	void					Save( idSaveGame* savefile ) const;
	void					Restore( idRestoreGame* savefile );

	virtual void			ClientPredictionThink();
	virtual void			ClientThink( const int curTime, const float fraction, const bool predict );
	virtual void			Think();

	void					UpdateAnimation();

	virtual idAnimator* 	GetAnimator();
	virtual void			SetModel( const char* modelname );

	bool					GetJointWorldTransform( jointHandle_t jointHandle, int currentTime, idVec3& offset, idMat3& axis );
	bool					GetJointTransformForAnim( jointHandle_t jointHandle, int animNum, int currentTime, idVec3& offset, idMat3& axis ) const;

	virtual int				GetDefaultSurfaceType() const;
	virtual void			AddDamageEffect( const trace_t& collision, const idVec3& velocity, const char* damageDefName );
	void					AddLocalDamageEffect( jointHandle_t jointNum, const idVec3& localPoint, const idVec3& localNormal, const idVec3& localDir, const idDeclEntityDef* def, const idMaterial* collisionMaterial );
	void					UpdateDamageEffects();

	virtual bool			ClientReceiveEvent( int event, int time, const idBitMsg& msg );

	enum
	{
		EVENT_ADD_DAMAGE_EFFECT = idEntity::EVENT_MAXEVENTS,
		EVENT_MAXEVENTS
	};

protected:
	idAnimator				animator;
	damageEffect_t* 		damageEffects;

private:
	void					Event_GetJointHandle( const char* jointname );
	void 					Event_ClearAllJoints();
	void 					Event_ClearJoint( jointHandle_t jointnum );
	void 					Event_SetJointPos( jointHandle_t jointnum, jointModTransform_t transform_type, const idVec3& pos );
	void 					Event_SetJointAngle( jointHandle_t jointnum, jointModTransform_t transform_type, const idAngles& angles );
	void 					Event_GetJointPos( jointHandle_t jointnum );
	void 					Event_GetJointAngle( jointHandle_t jointnum );
};


class SetTimeState
{
	bool					activated;
	bool					previousFast;
	bool					fast;

public:
	SetTimeState();
	SetTimeState( int timeGroup );
	~SetTimeState();

	void					PushState( int timeGroup );
};

ID_INLINE SetTimeState::SetTimeState()
{
	activated = false;
}

ID_INLINE SetTimeState::SetTimeState( int timeGroup )
{
	activated = false;
	PushState( timeGroup );
}

ID_INLINE void SetTimeState::PushState( int timeGroup )
{

	// Don't mess with time in Multiplayer
	if( !common->IsMultiplayer() )
	{

		activated = true;

		// determine previous fast setting
		if( gameLocal.time == gameLocal.slow.time )
		{
			previousFast = false;
		}
		else
		{
			previousFast = true;
		}

		// determine new fast setting
		if( timeGroup )
		{
			fast = true;
		}
		else
		{
			fast = false;
		}

		// set correct time
		gameLocal.SelectTimeGroup( timeGroup );
	}
}

ID_INLINE SetTimeState::~SetTimeState()
{
	if( activated && !common->IsMultiplayer() )
	{
		// set previous correct time
		gameLocal.SelectTimeGroup( previousFast );
	}
}

#endif /* !__GAME_ENTITY_H__ */
