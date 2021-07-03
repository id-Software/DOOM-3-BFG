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

#include "Game_local.h"

/*
===============================================================================

	idProjectile

===============================================================================
*/


idCVar g_projectileDebug( "g_projectileDebug", "0", CVAR_BOOL, "Debug projectiles" );


// This is used in MP to simulate frames to catchup a projectile's state. Similiar to how players work much lighter weight.

// This is used in MP to simulate frames to catchup a projectile's state. Similiar to how players work much lighter weight.
idArray< idProjectile::simulatedProjectile_t, idProjectile::MAX_SIMULATED_PROJECTILES >	idProjectile::projectilesToSimulate;

static const int BFG_DAMAGE_FREQUENCY			= 333;
static const float BOUNCE_SOUND_MIN_VELOCITY	= 200.0f;
static const float BOUNCE_SOUND_MAX_VELOCITY	= 400.0f;

const idEventDef EV_Explode( "<explode>", NULL );
const idEventDef EV_Fizzle( "<fizzle>", NULL );
const idEventDef EV_RadiusDamage( "<radiusdmg>", "e" );
const idEventDef EV_GetProjectileState( "getProjectileState", NULL, 'd' );

const idEventDef EV_CreateProjectile( "projectileCreateProjectile", "evv" );
const idEventDef EV_LaunchProjectile( "projectileLaunchProjectile", "vvv" );
const idEventDef EV_SetGravity( "setGravity", "f" );

CLASS_DECLARATION( idEntity, idProjectile )
EVENT( EV_Explode,				idProjectile::Event_Explode )
EVENT( EV_Fizzle,				idProjectile::Event_Fizzle )
EVENT( EV_Touch,				idProjectile::Event_Touch )
EVENT( EV_RadiusDamage,			idProjectile::Event_RadiusDamage )
EVENT( EV_GetProjectileState,	idProjectile::Event_GetProjectileState )
EVENT( EV_CreateProjectile,		idProjectile::Event_CreateProjectile )
EVENT( EV_LaunchProjectile,		idProjectile::Event_LaunchProjectile )
EVENT( EV_SetGravity,			idProjectile::Event_SetGravity )
END_CLASS

/*
================
idProjectile::idProjectile
================
*/
idProjectile::idProjectile() :
	launchOrigin( 0.0f ),
	launchAxis( mat3_identity )
{
	owner				= NULL;
	lightDefHandle		= -1;
	thrust				= 0.0f;
	thrust_end			= 0;
	smokeFly			= NULL;
	smokeFlyTime		= 0;
	state				= SPAWNED;
	lightOffset			= vec3_zero;
	lightStartTime		= 0;
	lightEndTime		= 0;
	lightColor			= vec3_zero;
	damagePower			= 1.0f;
	launchedFromGrabber = false;
	mTouchTriggers		= false;
	mNoExplodeDisappear = false;
	memset( &projectileFlags, 0, sizeof( projectileFlags ) );
	memset( &renderLight, 0, sizeof( renderLight ) );

	// note: for net_instanthit projectiles, we will force this back to false at spawn time
	fl.networkSync		= true;
}

/*
================
idProjectile::Spawn
================
*/
void idProjectile::Spawn()
{
	physicsObj.SetSelf( this );
	physicsObj.SetClipModel( new( TAG_PHYSICS_CLIP_ENTITY ) idClipModel( GetPhysics()->GetClipModel() ), 1.0f );
	physicsObj.SetContents( 0 );
	physicsObj.SetClipMask( 0 );
	physicsObj.PutToRest();
	SetPhysics( &physicsObj );
	mNoExplodeDisappear = spawnArgs.GetBool( "no_explode_disappear", mNoExplodeDisappear );
	mTouchTriggers = spawnArgs.GetBool( "touch_triggers", mTouchTriggers );
}

/*
================
idProjectile::Save
================
*/
void idProjectile::Save( idSaveGame* savefile ) const
{

	owner.Save( savefile );

	projectileFlags_s flags = projectileFlags;
	LittleBitField( &flags, sizeof( flags ) );
	savefile->Write( &flags, sizeof( flags ) );

	savefile->WriteFloat( thrust );
	savefile->WriteInt( thrust_end );

	savefile->WriteRenderLight( renderLight );
	savefile->WriteInt( ( int )lightDefHandle );
	savefile->WriteVec3( lightOffset );
	savefile->WriteInt( lightStartTime );
	savefile->WriteInt( lightEndTime );
	savefile->WriteVec3( lightColor );

	savefile->WriteParticle( smokeFly );
	savefile->WriteInt( smokeFlyTime );

	savefile->WriteInt( originalTimeGroup );

	savefile->WriteInt( ( int )state );

	savefile->WriteFloat( damagePower );

	savefile->WriteStaticObject( physicsObj );
	savefile->WriteStaticObject( thruster );
}

/*
================
idProjectile::Restore
================
*/
void idProjectile::Restore( idRestoreGame* savefile )
{

	owner.Restore( savefile );

	savefile->Read( &projectileFlags, sizeof( projectileFlags ) );
	LittleBitField( &projectileFlags, sizeof( projectileFlags ) );

	savefile->ReadFloat( thrust );
	savefile->ReadInt( thrust_end );

	savefile->ReadRenderLight( renderLight );
	savefile->ReadInt( ( int& )lightDefHandle );
	savefile->ReadVec3( lightOffset );
	savefile->ReadInt( lightStartTime );
	savefile->ReadInt( lightEndTime );
	savefile->ReadVec3( lightColor );

	savefile->ReadParticle( smokeFly );
	savefile->ReadInt( smokeFlyTime );

	savefile->ReadInt( originalTimeGroup );

	savefile->ReadInt( ( int& )state );

	savefile->ReadFloat( damagePower );

	savefile->ReadStaticObject( physicsObj );
	RestorePhysics( &physicsObj );

	savefile->ReadStaticObject( thruster );
	thruster.SetPhysics( &physicsObj );

	if( smokeFly != NULL )
	{
		idVec3 dir;
		dir = physicsObj.GetLinearVelocity();
		dir.NormalizeFast();
		gameLocal.smokeParticles->EmitSmoke( smokeFly, gameLocal.time, gameLocal.random.RandomFloat(), GetPhysics()->GetOrigin(), GetPhysics()->GetAxis(), timeGroup /*_D3XP*/ );
	}

	if( lightDefHandle >= 0 )
	{
		lightDefHandle = gameRenderWorld->AddLightDef( &renderLight );
	}
}

/*
================
idProjectile::GetOwner
================
*/
idEntity* idProjectile::GetOwner() const
{
	return owner.GetEntity();
}

/*
================
idProjectile::Create
================
*/
void idProjectile::Create( idEntity* owner, const idVec3& start, const idVec3& dir )
{
	idDict		args;
	idStr		shaderName;
	idVec3		light_color;
	idVec3		light_offset;
	idVec3		tmp;
	idMat3		axis;

	Unbind();

	// align z-axis of model with the direction
	axis = dir.ToMat3();
	tmp = axis[2];
	axis[2] = axis[0];
	axis[0] = -tmp;

	physicsObj.SetOrigin( start );
	physicsObj.SetAxis( axis );

	physicsObj.GetClipModel()->SetOwner( owner );

	this->owner = owner;

	memset( &renderLight, 0, sizeof( renderLight ) );
	shaderName = spawnArgs.GetString( "mtr_light_shader" );
	if( *( const char* )shaderName )
	{
		renderLight.shader = declManager->FindMaterial( shaderName, false );
		renderLight.pointLight = true;
		renderLight.lightRadius[0] =
			renderLight.lightRadius[1] =
				renderLight.lightRadius[2] = spawnArgs.GetFloat( "light_radius" );
#ifdef ID_PC
		renderLight.lightRadius *= 1.5f;
		renderLight.forceShadows = true;
#endif
		spawnArgs.GetVector( "light_color", "1 1 1", light_color );
		renderLight.shaderParms[0] = light_color[0];
		renderLight.shaderParms[1] = light_color[1];
		renderLight.shaderParms[2] = light_color[2];
		renderLight.shaderParms[3] = 1.0f;
	}

	spawnArgs.GetVector( "light_offset", "0 0 0", lightOffset );

	lightStartTime = 0;
	lightEndTime = 0;
	smokeFlyTime = 0;

	damagePower = 1.0f;

	if( spawnArgs.GetBool( "reset_time_offset", "0" ) )
	{
		renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ] = -MS2SEC( gameLocal.time );
	}

	UpdateVisuals();

	state = CREATED;
}

/*
=================
idProjectile::~idProjectile
=================
*/
idProjectile::~idProjectile()
{
	StopSound( SND_CHANNEL_ANY, false );
	FreeLightDef();
}

/*
=================
idProjectile::FreeLightDef
=================
*/
void idProjectile::FreeLightDef()
{
	if( lightDefHandle != -1 )
	{
		gameRenderWorld->FreeLightDef( lightDefHandle );
		lightDefHandle = -1;
	}
}

/*
=================
idProjectile::Launch
=================
*/
void idProjectile::Launch( const idVec3& start, const idVec3& dir, const idVec3& pushVelocity, const float timeSinceFire, const float launchPower, const float dmgPower )
{
	float			fuse;
	float			startthrust;
	float			endthrust;
	idVec3			velocity;
	idAngles		angular_velocity;
	float			linear_friction;
	float			angular_friction;
	float			contact_friction;
	float			bounce;
	float			mass;
	float			speed;
	float			gravity;
	idVec3			gravVec;
	idVec3			tmp;
	idMat3			axis;
	int				thrust_start;
	int				contents;
	int				clipMask;

	// allow characters to throw projectiles during cinematics, but not the player
	if( owner.GetEntity() && !owner.GetEntity()->IsType( idPlayer::Type ) )
	{
		cinematic = owner.GetEntity()->cinematic;
	}
	else
	{
		cinematic = false;
	}

	thrust				= spawnArgs.GetFloat( "thrust" );
	startthrust			= spawnArgs.GetFloat( "thrust_start" );
	endthrust			= spawnArgs.GetFloat( "thrust_end" );

	spawnArgs.GetVector( "velocity", "0 0 0", velocity );

	speed = velocity.Length() * launchPower;

	damagePower = dmgPower;

	spawnArgs.GetAngles( "angular_velocity", "0 0 0", angular_velocity );

	linear_friction		= spawnArgs.GetFloat( "linear_friction" );
	angular_friction	= spawnArgs.GetFloat( "angular_friction" );
	contact_friction	= spawnArgs.GetFloat( "contact_friction" );
	bounce				= spawnArgs.GetFloat( "bounce" );
	mass				= spawnArgs.GetFloat( "mass" );
	gravity				= spawnArgs.GetFloat( "gravity" );
	fuse				= spawnArgs.GetFloat( "fuse" );

	projectileFlags.detonate_on_world	= spawnArgs.GetBool( "detonate_on_world" );
	projectileFlags.detonate_on_actor	= spawnArgs.GetBool( "detonate_on_actor" );
	projectileFlags.randomShaderSpin	= spawnArgs.GetBool( "random_shader_spin" );

	if( mass <= 0 )
	{
		gameLocal.Error( "Invalid mass on '%s'\n", GetEntityDefName() );
	}

	thrust *= mass;
	thrust_start = SEC2MS( startthrust ) + gameLocal.time;
	thrust_end = SEC2MS( endthrust ) + gameLocal.time;

	lightStartTime = 0;
	lightEndTime = 0;

	if( health )
	{
		fl.takedamage = true;
	}

	gravVec = gameLocal.GetGravity();
	gravVec.NormalizeFast();

	Unbind();

	// align z-axis of model with the direction
	axis = dir.ToMat3();
	tmp = axis[2];
	axis[2] = axis[0];
	axis[0] = -tmp;

	contents = 0;
	clipMask = MASK_SHOT_RENDERMODEL;
	if( spawnArgs.GetBool( "detonate_on_trigger" ) )
	{
		contents |= CONTENTS_TRIGGER;
	}
	if( !spawnArgs.GetBool( "no_contents" ) )
	{
		contents |= CONTENTS_PROJECTILE;
		clipMask |= CONTENTS_PROJECTILE;
	}

	if( !idStr::Cmp( this->GetEntityDefName(), "projectile_helltime_killer" ) )
	{
		contents = CONTENTS_MOVEABLECLIP;
		clipMask = CONTENTS_MOVEABLECLIP;
		fuse = 10.0f;
	}

	// don't do tracers on client, we don't know origin and direction
	if( spawnArgs.GetBool( "tracers" ) && gameLocal.random.RandomFloat() > 0.5f )
	{
		SetModel( spawnArgs.GetString( "model_tracer" ) );
		projectileFlags.isTracer = true;
	}

	physicsObj.SetMass( mass );
	physicsObj.SetFriction( linear_friction, angular_friction, contact_friction );
	if( contact_friction == 0.0f )
	{
		physicsObj.NoContact();
	}
	physicsObj.SetBouncyness( bounce );
	physicsObj.SetGravity( gravVec * gravity );
	physicsObj.SetContents( contents );
	physicsObj.SetClipMask( clipMask );
	physicsObj.SetLinearVelocity( axis[ 2 ] * speed + pushVelocity );
	physicsObj.SetAngularVelocity( angular_velocity.ToAngularVelocity() * axis );
	physicsObj.SetOrigin( start );
	physicsObj.SetAxis( axis );

	launchOrigin = start;
	launchAxis = axis;

	thruster.SetPosition( &physicsObj, 0, idVec3( GetPhysics()->GetBounds()[ 0 ].x, 0, 0 ) );

	if( !common->IsClient() || fl.skipReplication )
	{
		if( fuse <= 0 )
		{
			// run physics for 1 second
			RunPhysics();
			PostEventMS( &EV_Remove, spawnArgs.GetInt( "remove_time", "1500" ) );
		}
		else if( spawnArgs.GetBool( "detonate_on_fuse" ) )
		{
			fuse -= timeSinceFire;
			if( fuse < 0.0f )
			{
				fuse = 0.0f;
			}
			PostEventSec( &EV_Explode, fuse );
		}
		else
		{
			fuse -= timeSinceFire;
			if( fuse < 0.0f )
			{
				fuse = 0.0f;
			}
			PostEventSec( &EV_Fizzle, fuse );
		}
	}

	if( projectileFlags.isTracer )
	{
		StartSound( "snd_tracer", SND_CHANNEL_BODY, 0, false, NULL );
	}
	else
	{
		StartSound( "snd_fly", SND_CHANNEL_BODY, 0, false, NULL );
	}

	smokeFlyTime = 0;
	const char* smokeName = spawnArgs.GetString( "smoke_fly" );
	if( *smokeName != '\0' )
	{
		smokeFly = static_cast<const idDeclParticle*>( declManager->FindType( DECL_PARTICLE, smokeName ) );
		smokeFlyTime = gameLocal.time;
	}

	originalTimeGroup = timeGroup;

	// used for the plasma bolts but may have other uses as well
	if( projectileFlags.randomShaderSpin )
	{
		float f = gameLocal.random.RandomFloat();
		f *= 0.5f;
		renderEntity.shaderParms[SHADERPARM_DIVERSITY] = f;
	}

	UpdateVisuals();

	state = LAUNCHED;
}

/*
================
idProjectile::Think
================
*/
void idProjectile::Think()
{

	if( thinkFlags & TH_THINK )
	{
		if( thrust && ( gameLocal.time < thrust_end ) )
		{
			// evaluate force
			thruster.SetForce( GetPhysics()->GetAxis()[ 0 ] * thrust );
			thruster.Evaluate( gameLocal.time );
		}
	}

	if( mTouchTriggers )
	{
		TouchTriggers();
	}

	// if the projectile owner is a player
	if( owner.GetEntity() && owner.GetEntity()->IsType( idPlayer::Type ) )
	{
		idPlayer* player = static_cast<idPlayer*>( owner.GetEntity() );

		// Remove any projectiles spectators threw.
		if( player != NULL && player->spectating )
		{
			PostEventMS( &EV_Remove, 0 );
		}
	}

	// run physics
	RunPhysics();

	DecayOriginAndAxisDelta();

	Present();

	AddParticlesAndLight();
}

/*
=================
idProjectile::AddParticlesAndLight
=================
*/
void idProjectile::AddParticlesAndLight()
{
	// add the particles
	if( smokeFly != NULL && smokeFlyTime && !IsHidden() )
	{
		idVec3 dir = -GetPhysics()->GetLinearVelocity();
		dir.Normalize();
		SetTimeState ts( originalTimeGroup );

		if( !gameLocal.smokeParticles->EmitSmoke( smokeFly, smokeFlyTime, gameLocal.random.RandomFloat(), GetPhysics()->GetOrigin(), dir.ToMat3(), timeGroup /*_D3XP*/ ) )
		{
			smokeFlyTime = gameLocal.time;
		}
	}

	// add the light
	if( renderLight.lightRadius.x > 0.0f && g_projectileLights.GetBool() )
	{
		renderLight.origin = GetPhysics()->GetOrigin() + GetPhysics()->GetAxis() * lightOffset;
		renderLight.axis = GetPhysics()->GetAxis();
		if( ( lightDefHandle != -1 ) )
		{
			if( lightEndTime > 0 && gameLocal.time <= lightEndTime )
			{
				idVec3 color( 0, 0, 0 );
				if( gameLocal.time < lightEndTime )
				{
					float frac = ( float )( gameLocal.time - lightStartTime ) / ( float )( lightEndTime - lightStartTime );
					color.Lerp( lightColor, color, frac );
				}
				renderLight.shaderParms[SHADERPARM_RED] = color.x;
				renderLight.shaderParms[SHADERPARM_GREEN] = color.y;
				renderLight.shaderParms[SHADERPARM_BLUE] = color.z;
			}
			gameRenderWorld->UpdateLightDef( lightDefHandle, &renderLight );
		}
		else
		{
			lightDefHandle = gameRenderWorld->AddLightDef( &renderLight );
		}
	}
}

/*
=================
idProjectile::Collide
=================
*/
bool idProjectile::Collide( const trace_t& collision, const idVec3& velocity )
{
	idEntity*	ent;
	idEntity*	ignore;
	const char*	damageDefName;
	idVec3		dir;
	float		push;
	float		damageScale;

	if( state == EXPLODED || state == FIZZLED )
	{
		return true;
	}

	const bool isHitscan = spawnArgs.GetBool( "net_instanthit" );

	// hanlde slow projectiles here.
	if( common->IsClient() && !isHitscan )
	{

		// This is a replicated slow projectile, predict the explosion.
		if( ClientPredictionCollide( this, spawnArgs, collision, velocity, !isHitscan ) )
		{
			Explode( collision, NULL );
			return true;
		}

	}

	// remove projectile when a 'noimpact' surface is hit
	if( ( collision.c.material != NULL ) && ( collision.c.material->GetSurfaceFlags() & SURF_NOIMPACT ) )
	{
		PostEventMS( &EV_Remove, 0 );
		common->DPrintf( "Projectile collision no impact\n" );
		return true;
	}

	// get the entity the projectile collided with
	ent = gameLocal.entities[ collision.c.entityNum ];
	if( ent == owner.GetEntity() )
	{
		assert( 0 );
		return true;
	}

	// just get rid of the projectile when it hits a player in noclip
	if( ent->IsType( idPlayer::Type ) && static_cast<idPlayer*>( ent )->noclip )
	{
		PostEventMS( &EV_Remove, 0 );
		return true;
	}

	// direction of projectile
	dir = velocity;
	dir.Normalize();

	// projectiles can apply an additional impulse next to the rigid body physics impulse
	if( spawnArgs.GetFloat( "push", "0", push ) && push > 0.0f )
	{
		if( !common->IsClient() )
		{
			ent->ApplyImpulse( this, collision.c.id, collision.c.point, push * dir );
		}
	}

	// MP: projectiles open doors
	if( common->IsMultiplayer() && ent->IsType( idDoor::Type ) && !static_cast< idDoor* >( ent )->IsOpen() && !ent->spawnArgs.GetBool( "no_touch" ) )
	{
		if( !common->IsClient() )
		{
			ent->ProcessEvent( &EV_Activate , this );
		}
	}

	if( ent->IsType( idActor::Type ) || ( ent->IsType( idAFAttachment::Type ) && static_cast<const idAFAttachment*>( ent )->GetBody()->IsType( idActor::Type ) ) )
	{
		if( !projectileFlags.detonate_on_actor )
		{
			return false;
		}
	}
	else
	{
		if( !projectileFlags.detonate_on_world )
		{
			if( !StartSound( "snd_ricochet", SND_CHANNEL_ITEM, 0, true, NULL ) )
			{
				float len = velocity.Length();
				if( len > BOUNCE_SOUND_MIN_VELOCITY )
				{
					SetSoundVolume( len > BOUNCE_SOUND_MAX_VELOCITY ? 1.0f : idMath::Sqrt( len - BOUNCE_SOUND_MIN_VELOCITY ) * ( 1.0f / idMath::Sqrt( BOUNCE_SOUND_MAX_VELOCITY - BOUNCE_SOUND_MIN_VELOCITY ) ) );
					StartSound( "snd_bounce", SND_CHANNEL_ANY, 0, true, NULL );
				}
			}
			return false;
		}
	}

	SetOrigin( collision.endpos );
	SetAxis( collision.endAxis );

	// To see the explosion on the collision surface instead of
	// at the muzzle, clear the deltas.
	CreateDeltasFromOldOriginAndAxis( GetPhysics()->GetOrigin(), GetPhysics()->GetAxis() );

	// unlink the clip model because we no longer need it
	GetPhysics()->UnlinkClip();

	damageDefName = spawnArgs.GetString( "def_damage" );

	ignore = NULL;

	// if the projectile causes a damage effect
	if( spawnArgs.GetBool( "impact_damage_effect" ) )
	{
		// if the hit entity has a special damage effect
		if( ent->spawnArgs.GetBool( "bleed" ) )
		{
			ent->AddDamageEffect( collision, velocity, damageDefName );
		}
		else
		{
			AddDefaultDamageEffect( collision, velocity );
		}
	}

	// if the hit entity takes damage
	if( ent->fl.takedamage )
	{
		if( damagePower )
		{
			damageScale = damagePower;
		}
		else
		{
			damageScale = 1.0f;
		}

		// if the projectile owner is a player
		if( owner.GetEntity() && owner.GetEntity()->IsType( idPlayer::Type ) )
		{
			// if the projectile hit an actor
			if( ent->IsType( idActor::Type ) )
			{
				idPlayer* player = static_cast<idPlayer*>( owner.GetEntity() );
				player->AddProjectileHits( 1 );
				damageScale *= player->PowerUpModifier( PROJECTILE_DAMAGE );
			}
		}

		if( damageDefName[0] != '\0' )
		{

			bool killedByImpact = true;

			if( ent->health <= 0 )
			{
				killedByImpact = false;
			}

			// Only handle the server's own attacks here. Attacks by other players on the server occur through
			// reliable messages.
			if( !common->IsMultiplayer() || common->IsClient() || ( common->IsServer() && owner.GetEntityNum() == gameLocal.GetLocalClientNum() ) || ( common->IsServer() && !isHitscan ) )
			{
				ent->Damage( this, owner.GetEntity(), dir, damageDefName, damageScale, CLIPMODEL_ID_TO_JOINT_HANDLE( collision.c.id ) );
			}
			else
			{
				if( ent->IsType( idPlayer::Type ) ==  false && common->IsServer() )
				{
					ent->Damage( this, owner.GetEntity(), dir, damageDefName, damageScale, CLIPMODEL_ID_TO_JOINT_HANDLE( collision.c.id ) );
				}
			}

			// Check if we are hitting an actor. and see if we killed him.
			if( !common->IsClient() && ent->health <= 0 && killedByImpact )
			{
				if( owner.GetEntity() && owner.GetEntity()->IsType( idPlayer::Type ) )
				{
					if( ent->IsType( idActor::Type ) && ent != owner.GetEntity() )
					{
						idPlayer* player = static_cast<idPlayer*>( owner.GetEntity() );
						player->AddProjectileKills();
					}
				}

			}

			ignore = ent;
		}
	}

	Explode( collision, ignore );

	if( !common->IsClient() && owner.GetEntity() != NULL && owner.GetEntity()->IsType( idPlayer::Type ) )
	{
		idPlayer* player = static_cast<idPlayer*>( owner.GetEntity() );
		int kills = player->GetProjectileKills();

		if( kills >= 2 && common->IsMultiplayer() && strstr( GetName(), "projectile_rocket" ) != 0 )
		{
			player->GetAchievementManager().EventCompletesAchievement( ACHIEVEMENT_MP_KILL_2_GUYS_IN_ROOM_WITH_BFG );
		}

		// projectile is done dealing damage.
		player->ResetProjectileKills();
	}

	return true;
}

/*
=================
idProjectile::DefaultDamageEffect
=================
*/
void idProjectile::DefaultDamageEffect( idEntity* soundEnt, const idDict& projectileDef, const trace_t& collision, const idVec3& velocity )
{
	const char* decal, *sound, *typeName;
	surfTypes_t materialType;

	if( collision.c.material != NULL )
	{
		materialType = collision.c.material->GetSurfaceType();
	}
	else
	{
		materialType = SURFTYPE_METAL;
	}

	// get material type name
	typeName = gameLocal.sufaceTypeNames[ materialType ];

	// play impact sound
	sound = projectileDef.GetString( va( "snd_%s", typeName ) );
	if( *sound == '\0' )
	{
		sound = projectileDef.GetString( "snd_metal" );
	}
	if( *sound == '\0' )
	{
		sound = projectileDef.GetString( "snd_impact" );
	}
	if( *sound != '\0' )
	{
		soundEnt->StartSoundShader( declManager->FindSound( sound ), SND_CHANNEL_BODY, 0, false, NULL );
	}

	// project decal
	decal = projectileDef.GetString( va( "mtr_detonate_%s", typeName ) );
	if( *decal == '\0' )
	{
		decal = projectileDef.GetString( "mtr_detonate" );
	}
	if( *decal != '\0' )
	{
		gameLocal.ProjectDecal( collision.c.point, -collision.c.normal, 8.0f, true, projectileDef.GetFloat( "decal_size", "6.0" ), decal );
	}
}

/*
=================
idProjectile::AddDefaultDamageEffect
=================
*/
void idProjectile::AddDefaultDamageEffect( const trace_t& collision, const idVec3& velocity )
{

	DefaultDamageEffect( this, spawnArgs, collision, velocity );

	if( common->IsServer() && fl.networkSync )
	{
		idBitMsg	msg;
		byte		msgBuf[MAX_EVENT_PARAM_SIZE];
		lobbyUserID_t excluding;

		if( spawnArgs.GetBool( "net_instanthit" ) && owner.GetEntityNum() < MAX_PLAYERS )
		{
			excluding = gameLocal.lobbyUserIDs[owner.GetEntityNum()];
		}

		msg.InitWrite( msgBuf, sizeof( msgBuf ) );
		msg.BeginWriting();
		msg.WriteFloat( collision.c.point[0] );
		msg.WriteFloat( collision.c.point[1] );
		msg.WriteFloat( collision.c.point[2] );
		msg.WriteDir( collision.c.normal, 24 );
		msg.WriteLong( ( collision.c.material != NULL ) ? gameLocal.ServerRemapDecl( -1, DECL_MATERIAL, collision.c.material->Index() ) : -1 );
		msg.WriteFloat( velocity[0], 5, 10 );
		msg.WriteFloat( velocity[1], 5, 10 );
		msg.WriteFloat( velocity[2], 5, 10 );
		ServerSendEvent( EVENT_DAMAGE_EFFECT, &msg, false, excluding );
	}
}

/*
================
idProjectile::Killed
================
*/
void idProjectile::Killed( idEntity* inflictor, idEntity* attacker, int damage, const idVec3& dir, int location )
{
	if( spawnArgs.GetBool( "detonate_on_death" ) )
	{
		trace_t collision;

		memset( &collision, 0, sizeof( collision ) );
		collision.endAxis = GetPhysics()->GetAxis();
		collision.endpos = GetPhysics()->GetOrigin();
		collision.c.point = GetPhysics()->GetOrigin();
		collision.c.normal.Set( 0, 0, 1 );
		Explode( collision, NULL );
		physicsObj.ClearContacts();
		physicsObj.PutToRest();
	}
	else
	{
		Fizzle();
	}
}

/*
================
idProjectile::Fizzle
================
*/
void idProjectile::Fizzle()
{

	if( state == EXPLODED || state == FIZZLED )
	{
		return;
	}

	StopSound( SND_CHANNEL_BODY, false );
	StartSound( "snd_fizzle", SND_CHANNEL_BODY, 0, false, NULL );

	// fizzle FX
	const char* psystem = spawnArgs.GetString( "smoke_fuse" );
	if( psystem != NULL && *psystem != '\0' )
	{
//FIXME:SMOKE		gameLocal.particles->SpawnParticles( GetPhysics()->GetOrigin(), vec3_origin, psystem );
	}

	// we need to work out how long the effects last and then remove them at that time
	// for example, bullets have no real effects
	if( smokeFly && smokeFlyTime )
	{
		smokeFlyTime = 0;
	}

	fl.takedamage = false;
	physicsObj.SetContents( 0 );
	physicsObj.GetClipModel()->Unlink();
	physicsObj.PutToRest();

	Hide();
	FreeLightDef();

	state = FIZZLED;

	if( common->IsClient() && !fl.skipReplication )
	{
		return;
	}

	CancelEvents( &EV_Fizzle );
	PostEventMS( &EV_Remove, spawnArgs.GetInt( "remove_time", "1500" ) );
}

/*
================
idProjectile::Event_RadiusDamage
================
*/
void idProjectile::Event_RadiusDamage( idEntity* ignore )
{
	const char* splash_damage = spawnArgs.GetString( "def_splash_damage" );
	if( splash_damage[0] != '\0' )
	{
		gameLocal.RadiusDamage( physicsObj.GetOrigin(), this, owner.GetEntity(), ignore, this, splash_damage, damagePower );
	}
}

/*
================
idProjectile::Event_RadiusDamage
================
*/
void idProjectile::Event_GetProjectileState()
{
	idThread::ReturnInt( state );
}

/*
================
idProjectile::Explode
================
*/
void idProjectile::Explode( const trace_t& collision, idEntity* ignore )
{
	const char* fxname, *light_shader, *sndExplode;
	float		light_fadetime;
	idVec3		normal;
	int			removeTime;

	if( mNoExplodeDisappear )
	{
		PostEventMS( &EV_Remove, 0 );
		return;
	}

	if( state == EXPLODED || state == FIZZLED )
	{
		return;
	}

	// activate rumble for player
	idPlayer* player = gameLocal.GetLocalPlayer();
	const bool isHitscan = spawnArgs.GetBool( "net_instanthit" );
	if( player != NULL && isHitscan == false )
	{

		// damage
		const char* damageDefName = spawnArgs.GetString( "def_damage" );
		const idDict* damageDef = gameLocal.FindEntityDefDict( damageDefName );
		int damage;
		if( damageDef != NULL )
		{
			damage = damageDef->GetInt( "damage" );
		}
		else
		{
			damage = 200;
		}
		float damageScale = idMath::ClampFloat( 0.25f, 1.0f, ( float )damage * ( 1.0f / 200.0f ) );	// 50...200 -> min...max rumble

		// distance
		float dist = ( GetPhysics()->GetOrigin() - player->GetPhysics()->GetOrigin() ).LengthFast();
		float distScale = 1.0f - idMath::ClampFloat( 0.0f, 1.0f, ( dist * ( 1.0f / 4000.0f ) ) + 0.25f );		// 0...4000 -> max...min rumble

		distScale *= damageScale;	// apply damage scale here, weaker damage produces less rumble

		// determine rumble
		float highMag = distScale;
		int highDuration = idMath::Ftoi( 300.0f * distScale );
		float lowMag = distScale * 0.75f;
		int lowDuration = idMath::Ftoi( 500.0f * distScale );

		player->SetControllerShake( highMag, highDuration, lowMag, lowDuration );
	}

	// stop sound
	StopSound( SND_CHANNEL_BODY2, false );

	// play explode sound
	switch( ( int ) damagePower )
	{
		case 2:
			sndExplode = "snd_explode2";
			break;
		case 3:
			sndExplode = "snd_explode3";
			break;
		case 4:
			sndExplode = "snd_explode4";
			break;
		default:
			sndExplode = "snd_explode";
			break;
	}
	StartSound( sndExplode, SND_CHANNEL_BODY, 0, true, NULL );

	// we need to work out how long the effects last and then remove them at that time
	// for example, bullets have no real effects
	if( smokeFly && smokeFlyTime )
	{
		smokeFlyTime = 0;
	}

	Hide();
	FreeLightDef();

	if( spawnArgs.GetVector( "detonation_axis", "", normal ) )
	{
		GetPhysics()->SetAxis( normal.ToMat3() );
	}
	GetPhysics()->SetOrigin( collision.endpos + 2.0f * collision.c.normal );

	// default remove time
	if( fl.skipReplication && !spawnArgs.GetBool( "net_instanthit" ) )
	{
		removeTime = spawnArgs.GetInt( "remove_time", "6000" );
	}
	else
	{
		removeTime = spawnArgs.GetInt( "remove_time", "1500" );
	}

	// change the model, usually to a PRT
	fxname = NULL;
	if( g_testParticle.GetInteger() == TEST_PARTICLE_IMPACT )
	{
		fxname = g_testParticleName.GetString();
	}
	else
	{
		fxname = spawnArgs.GetString( "model_detonate" );
	}

	int surfaceType = collision.c.material != NULL ? collision.c.material->GetSurfaceType() : SURFTYPE_METAL;
	if( !( fxname != NULL && *fxname != '\0' ) )
	{
		if( ( surfaceType == SURFTYPE_NONE ) || ( surfaceType == SURFTYPE_METAL ) || ( surfaceType == SURFTYPE_STONE ) )
		{
			fxname = spawnArgs.GetString( "model_smokespark" );
		}
		else if( surfaceType == SURFTYPE_RICOCHET )
		{
			fxname = spawnArgs.GetString( "model_ricochet" );
		}
		else
		{
			fxname = spawnArgs.GetString( "model_smoke" );
		}
	}

	// If the explosion is in liquid, spawn a particle splash
	idVec3 testOrg = GetPhysics()->GetOrigin();
	int testC = gameLocal.clip.Contents( testOrg, NULL, mat3_identity, CONTENTS_WATER, this );
	if( testC & CONTENTS_WATER )
	{
		idFuncEmitter* splashEnt;
		idDict splashArgs;

		splashArgs.Set( "model", "sludgebulletimpact.prt" );
		splashArgs.Set( "start_off", "1" );
		splashEnt = static_cast<idFuncEmitter*>( gameLocal.SpawnEntityType( idFuncEmitter::Type, &splashArgs ) );

		splashEnt->GetPhysics()->SetOrigin( testOrg );
		splashEnt->PostEventMS( &EV_Activate, 0, this );
		splashEnt->PostEventMS( &EV_Remove, 1500 );

		// HACK - if this is a chaingun bullet, don't do the normal effect
		if( !idStr::Cmp( spawnArgs.GetString( "def_damage" ), "damage_bullet_chaingun" ) )
		{
			fxname = NULL;
		}
	}

	if( fxname && *fxname )
	{
		SetModel( fxname );
		renderEntity.shaderParms[SHADERPARM_RED] =
			renderEntity.shaderParms[SHADERPARM_GREEN] =
				renderEntity.shaderParms[SHADERPARM_BLUE] =
					renderEntity.shaderParms[SHADERPARM_ALPHA] = 1.0f;
		renderEntity.shaderParms[SHADERPARM_TIMEOFFSET] = -MS2SEC( gameLocal.time );
		renderEntity.shaderParms[SHADERPARM_DIVERSITY] = gameLocal.random.CRandomFloat();
		Show();
		removeTime = ( removeTime > 3000 ) ? removeTime : 3000;
	}

	// explosion light
	light_shader = spawnArgs.GetString( "mtr_explode_light_shader" );

	if( gameLocal.mpGame.IsGametypeFlagBased() && gameLocal.serverInfo.GetBool( "si_midnight" ) )
	{
		light_shader = "lights/midnight_grenade";
	}

	if( *light_shader )
	{
		renderLight.shader = declManager->FindMaterial( light_shader, false );
		renderLight.pointLight = true;
		renderLight.lightRadius[0] =
			renderLight.lightRadius[1] =
				renderLight.lightRadius[2] = spawnArgs.GetFloat( "explode_light_radius" );
#ifdef ID_PC
		renderLight.lightRadius *= 2.0f;
		renderLight.forceShadows = true;
#endif

		// Midnight ctf
		if( gameLocal.mpGame.IsGametypeFlagBased() && gameLocal.serverInfo.GetBool( "si_midnight" ) )
		{
			renderLight.lightRadius[0] =
				renderLight.lightRadius[1] =
					renderLight.lightRadius[2] = spawnArgs.GetFloat( "explode_light_radius" ) * 2;
		}

		spawnArgs.GetVector( "explode_light_color", "1 1 1", lightColor );
		renderLight.shaderParms[SHADERPARM_RED] = lightColor.x;
		renderLight.shaderParms[SHADERPARM_GREEN] = lightColor.y;
		renderLight.shaderParms[SHADERPARM_BLUE] = lightColor.z;
		renderLight.shaderParms[SHADERPARM_ALPHA] = 1.0f;
		renderLight.shaderParms[SHADERPARM_TIMEOFFSET] = -MS2SEC( gameLocal.time );

		// Midnight ctf
		if( gameLocal.mpGame.IsGametypeFlagBased() && gameLocal.serverInfo.GetBool( "si_midnight" ) )
		{
			light_fadetime = 3.0f;
		}
		else
		{
			light_fadetime = spawnArgs.GetFloat( "explode_light_fadetime", "0.5" );
		}

		lightStartTime = gameLocal.time;
		lightEndTime = MSEC_ALIGN_TO_FRAME( gameLocal.time + SEC2MS( light_fadetime ) );
		BecomeActive( TH_THINK );
	}

	fl.takedamage = false;
	physicsObj.SetContents( 0 );
	physicsObj.PutToRest();

	state = EXPLODED;

	if( common->IsClient() && !fl.skipReplication )
	{
		return;
	}

	// alert the ai
	gameLocal.AlertAI( owner.GetEntity() );

	// bind the projectile to the impact entity if necesary
	if( gameLocal.entities[collision.c.entityNum] && spawnArgs.GetBool( "bindOnImpact" ) )
	{
		Bind( gameLocal.entities[collision.c.entityNum], true );
	}

	// splash damage
	if( !projectileFlags.noSplashDamage )
	{
		float delay = spawnArgs.GetFloat( "delay_splash" );
		if( delay )
		{
			if( removeTime < delay * 1000 )
			{
				removeTime = ( delay + 0.10 ) * 1000;
			}
			PostEventSec( &EV_RadiusDamage, delay, ignore );
		}
		else
		{
			Event_RadiusDamage( ignore );
		}
	}

	// spawn debris entities
	int fxdebris = spawnArgs.GetInt( "debris_count" );
	if( fxdebris )
	{
		const idDict* debris = gameLocal.FindEntityDefDict( "projectile_debris", false );
		if( debris )
		{
			int amount = gameLocal.random.RandomInt( fxdebris );
			for( int i = 0; i < amount; i++ )
			{
				idEntity* ent;
				idVec3 dir;
				dir.x = gameLocal.random.CRandomFloat() * 4.0f;
				dir.y = gameLocal.random.CRandomFloat() * 4.0f;
				dir.z = gameLocal.random.RandomFloat() * 8.0f;
				dir.Normalize();

				gameLocal.SpawnEntityDef( *debris, &ent, false );
				if( ent == NULL || !ent->IsType( idDebris::Type ) )
				{
					gameLocal.Error( "'projectile_debris' is not an idDebris" );
					return;
				}

				idDebris* debris = static_cast<idDebris*>( ent );
				debris->Create( owner.GetEntity(), physicsObj.GetOrigin(), dir.ToMat3() );
				debris->Launch();
			}
		}
		debris = gameLocal.FindEntityDefDict( "projectile_shrapnel", false );
		if( debris )
		{
			int amount = gameLocal.random.RandomInt( fxdebris );
			for( int i = 0; i < amount; i++ )
			{
				idEntity* ent;
				idVec3 dir;
				dir.x = gameLocal.random.CRandomFloat() * 8.0f;
				dir.y = gameLocal.random.CRandomFloat() * 8.0f;
				dir.z = gameLocal.random.RandomFloat() * 8.0f + 8.0f;
				dir.Normalize();

				gameLocal.SpawnEntityDef( *debris, &ent, false );
				if( ent == NULL || !ent->IsType( idDebris::Type ) )
				{
					gameLocal.Error( "'projectile_shrapnel' is not an idDebris" );
					break;
				}

				idDebris* debris = static_cast<idDebris*>( ent );
				debris->Create( owner.GetEntity(), physicsObj.GetOrigin(), dir.ToMat3() );
				debris->Launch();
			}
		}
	}

	CancelEvents( &EV_Explode );
	PostEventMS( &EV_Remove, removeTime );
}

/*
================
idProjectile::GetVelocity
================
*/
idVec3 idProjectile::GetVelocity( const idDict* projectile )
{
	idVec3 velocity;

	projectile->GetVector( "velocity", "0 0 0", velocity );
	return velocity;
}

/*
================
idProjectile::GetGravity
================
*/
idVec3 idProjectile::GetGravity( const idDict* projectile )
{
	float gravity;

	gravity = projectile->GetFloat( "gravity" );
	return idVec3( 0, 0, -gravity );
}

/*
================
idProjectile::Event_Explode
================
*/
void idProjectile::Event_Explode()
{
	trace_t collision;

	memset( &collision, 0, sizeof( collision ) );
	collision.endAxis = GetPhysics()->GetAxis();
	collision.endpos = GetPhysics()->GetOrigin();
	collision.c.point = GetPhysics()->GetOrigin();
	collision.c.normal.Set( 0, 0, 1 );
	AddDefaultDamageEffect( collision, collision.c.normal );
	Explode( collision, NULL );
}

/*
================
idProjectile::Event_Fizzle
================
*/
void idProjectile::Event_Fizzle()
{
	Fizzle();
}

/*
================
idProjectile::Event_Touch
================
*/
void idProjectile::Event_Touch( idEntity* other, trace_t* trace )
{
	if( common->IsClient() )
	{
		return;
	}

	if( IsHidden() )
	{
		return;
	}

	// Projectiles do not collide with flags
	if( other->IsType( idItemTeam::Type ) )
	{
		return;
	}

	if( other != owner.GetEntity() )
	{
		trace_t collision;

		memset( &collision, 0, sizeof( collision ) );
		collision.endAxis = GetPhysics()->GetAxis();
		collision.endpos = GetPhysics()->GetOrigin();
		collision.c.point = GetPhysics()->GetOrigin();
		collision.c.normal.Set( 0, 0, 1 );
		AddDefaultDamageEffect( collision, collision.c.normal );
		Explode( collision, NULL );
	}
}

/*
================
idProjectile::CatchProjectile
================
*/
void idProjectile::CatchProjectile( idEntity* o, const char* reflectName )
{
	idEntity* prevowner = owner.GetEntity();

	owner = o;
	physicsObj.GetClipModel()->SetOwner( o );

	if( this->IsType( idGuidedProjectile::Type ) )
	{
		idGuidedProjectile* proj = static_cast<idGuidedProjectile*>( this );

		proj->SetEnemy( prevowner );
	}

	idStr s = spawnArgs.GetString( "def_damage" );
	s += reflectName;

	const idDict* damageDef = gameLocal.FindEntityDefDict( s, false );
	if( damageDef )
	{
		spawnArgs.Set( "def_damage", s );
	}
}

/*
================
idProjectile::GetProjectileState
================
*/
int idProjectile::GetProjectileState()
{

	return ( int )state;
}

/*
================
idProjectile::Event_CreateProjectile
================
*/
void idProjectile::Event_CreateProjectile( idEntity* owner, const idVec3& start, const idVec3& dir )
{
	Create( owner, start, dir );
}

/*
================
idProjectile::Event_LaunchProjectile
================
*/
void idProjectile::Event_LaunchProjectile( const idVec3& start, const idVec3& dir, const idVec3& pushVelocity )
{
	Launch( start, dir, pushVelocity );
}

/*
================
idProjectile::Event_SetGravity
================
*/
void idProjectile::Event_SetGravity( float gravity )
{
	idVec3 gravVec;

	gravVec = gameLocal.GetGravity();
	gravVec.NormalizeFast();
	physicsObj.SetGravity( gravVec * gravity );
}

/*
=================
idProjectile::ClientPredictionCollide
=================
*/
bool idProjectile::ClientPredictionCollide( idEntity* soundEnt, const idDict& projectileDef, const trace_t& collision, const idVec3& velocity, bool addDamageEffect )
{
	idEntity* ent;

	// remove projectile when a 'noimpact' surface is hit
	if( collision.c.material && ( collision.c.material->GetSurfaceFlags() & SURF_NOIMPACT ) )
	{
		return false;
	}

	// get the entity the projectile collided with
	ent = gameLocal.entities[ collision.c.entityNum ];
	if( ent == NULL )
	{
		return false;
	}

	// don't do anything if hitting a noclip player
	if( ent->IsType( idPlayer::Type ) && static_cast<idPlayer*>( ent )->noclip )
	{
		return false;
	}

	if( ent->IsType( idActor::Type ) || ( ent->IsType( idAFAttachment::Type ) && static_cast<const idAFAttachment*>( ent )->GetBody()->IsType( idActor::Type ) ) )
	{
		if( !projectileDef.GetBool( "detonate_on_actor" ) )
		{
			return false;
		}
	}
	else
	{
		if( !projectileDef.GetBool( "detonate_on_world" ) )
		{
			return false;
		}
	}

	// if the projectile causes a damage effect
	if( addDamageEffect && projectileDef.GetBool( "impact_damage_effect" ) )
	{
		// if the hit entity does not have a special damage effect
		if( !ent->spawnArgs.GetBool( "bleed" ) )
		{
			// predict damage effect
			DefaultDamageEffect( soundEnt, projectileDef, collision, velocity );
		}
	}
	return true;
}

/*
================
idProjectile::ClientThink
================
*/
void idProjectile::ClientThink( const int curTime, const float fraction, const bool predict )
{
	if( fl.skipReplication )
	{
		Think();
	}
	else
	{
		if( !renderEntity.hModel )
		{
			return;
		}
		InterpolatePhysicsOnly( fraction );
		Present();
		AddParticlesAndLight();
	}
}

/*
================
idProjectile::ClientPredictionThink
================
*/
void idProjectile::ClientPredictionThink()
{
	if( !renderEntity.hModel )
	{
		return;
	}
	Think();
}

/*
================
idProjectile::WriteToSnapshot
================
*/
void idProjectile::WriteToSnapshot( idBitMsg& msg ) const
{
	msg.WriteBits( owner.GetSpawnId(), 32 );
	msg.WriteBits( state, 3 );
	msg.WriteBits( fl.hidden, 1 );

	physicsObj.WriteToSnapshot( msg );
}

/*
================
idProjectile::ReadFromSnapshot
================
*/
void idProjectile::ReadFromSnapshot( const idBitMsg& msg )
{
	projectileState_t newState;

	owner.SetSpawnId( msg.ReadBits( 32 ) );
	newState = ( projectileState_t ) msg.ReadBits( 3 );

	if( msg.ReadBits( 1 ) )
	{
		Hide();
	}
	else
	{
		Show();
	}

	while( state != newState )
	{
		switch( state )
		{
			case SPAWNED:
			{
				Create( owner.GetEntity(), vec3_origin, idVec3( 1, 0, 0 ) );
				break;
			}
			case CREATED:
			{
				// the right origin and direction are required if you want bullet traces
				Launch( vec3_origin, idVec3( 1, 0, 0 ), vec3_origin );
				break;
			}
			case LAUNCHED:
			{
				if( newState == FIZZLED )
				{
					Fizzle();
				}
				else
				{
					trace_t collision;
					memset( &collision, 0, sizeof( collision ) );
					collision.endAxis = GetPhysics()->GetAxis();
					collision.endpos = GetPhysics()->GetOrigin();
					collision.c.point = GetPhysics()->GetOrigin();
					collision.c.normal.Set( 0, 0, 1 );
					Explode( collision, NULL );
				}
				break;
			}
			case FIZZLED:
			case EXPLODED:
			{
				StopSound( SND_CHANNEL_BODY2, false );
				gameEdit->ParseSpawnArgsToRenderEntity( &spawnArgs, &renderEntity );
				state = SPAWNED;
				break;
			}
		}
	}

	physicsObj.ReadFromSnapshot( msg );

	if( msg.HasChanged() )
	{
		UpdateVisuals();
	}
}

/*
================
idProjectile::ClientReceiveEvent
================
*/
bool idProjectile::ClientReceiveEvent( int event, int time, const idBitMsg& msg )
{
	trace_t collision;
	idVec3 velocity;

	switch( event )
	{
		case EVENT_DAMAGE_EFFECT:
		{
			memset( &collision, 0, sizeof( collision ) );
			collision.c.point[0] = msg.ReadFloat();
			collision.c.point[1] = msg.ReadFloat();
			collision.c.point[2] = msg.ReadFloat();
			collision.c.normal = msg.ReadDir( 24 );
			int index = gameLocal.ClientRemapDecl( DECL_MATERIAL, msg.ReadLong() );
			collision.c.material = ( index != -1 ) ? static_cast<const idMaterial*>( declManager->DeclByIndex( DECL_MATERIAL, index ) ) : NULL;
			velocity[0] = msg.ReadFloat( 5, 10 );
			velocity[1] = msg.ReadFloat( 5, 10 );
			velocity[2] = msg.ReadFloat( 5, 10 );
			DefaultDamageEffect( this, spawnArgs, collision, velocity );
			return true;
		}
		default:
		{
			return idEntity::ClientReceiveEvent( event, time, msg );
		}
	}
}


/*
========================
idProjectile::QueueToSimulate
========================
*/
void idProjectile::QueueToSimulate( int startTime )
{
	assert( common->IsMultiplayer() && common->IsServer() );

	for( int i = 0; i < MAX_SIMULATED_PROJECTILES; i++ )
	{
		if( projectilesToSimulate[i].projectile == NULL )
		{
			projectilesToSimulate[i].projectile = this;
			projectilesToSimulate[i].startTime = startTime;
			if( g_projectileDebug.GetBool() )
			{
				int delta = gameLocal.GetServerGameTimeMs() - startTime;
				idLib::Printf( "Simulating projectile %d. Approx %d delay.\n", GetEntityNumber(), delta );
			}
			return;
		}
	}

	idLib::Warning( "Unable to simulate more projectiles this frame" );
}

/*
========================
idProjectile::SimulateProjectileFrame
========================
*/
void idProjectile::SimulateProjectileFrame( int msec, int endTime )
{
	idVec3 oldOrigin = GetPhysics()->GetOrigin();

	GetPhysics()->Evaluate( msec, endTime );
	SetOrigin( GetPhysics()->GetOrigin() );
	SetAxis( GetPhysics()->GetAxis() );

	if( g_projectileDebug.GetBool() )
	{
		float delta = ( GetPhysics()->GetOrigin() - oldOrigin ).Length();
		idLib::Printf( "Simulated projectile %d. Delta: %.2f \n", GetEntityNumber(), delta );
		//clientGame->renderWorld->DebugLine( idColor::colorYellow, oldOrigin, GetPhysics()->GetOrigin(), 5000 );
	}
}

/*
========================
idProjectile::PostSimulate
========================
*/
void idProjectile::PostSimulate( int endTime )
{
	if( state == EXPLODED || state == FIZZLED )
	{
		// Already exploded. To see the explosion on the collision surface instead of
		// at the muzzle, don't set the deltas to the launch origin and axis.
		CreateDeltasFromOldOriginAndAxis( GetPhysics()->GetOrigin(), GetPhysics()->GetAxis() );
	}
	else
	{
		CreateDeltasFromOldOriginAndAxis( launchOrigin, launchAxis );
	}
}



/*
===============================================================================

	idGuidedProjectile

===============================================================================
*/

const idEventDef EV_SetEnemy( "setEnemy", "E" );

CLASS_DECLARATION( idProjectile, idGuidedProjectile )
EVENT( EV_SetEnemy,		idGuidedProjectile::Event_SetEnemy )
END_CLASS

/*
================
idGuidedProjectile::idGuidedProjectile
================
*/
idGuidedProjectile::idGuidedProjectile()
{
	enemy			= NULL;
	speed			= 0.0f;
	turn_max		= 0.0f;
	clamp_dist		= 0.0f;
	rndScale		= ang_zero;
	rndAng			= ang_zero;
	rndUpdateTime	= 0;
	angles			= ang_zero;
	burstMode		= false;
	burstDist		= 0;
	burstVelocity	= 0.0f;
	unGuided		= false;
}

/*
=================
idGuidedProjectile::~idGuidedProjectile
=================
*/
idGuidedProjectile::~idGuidedProjectile()
{
}

/*
================
idGuidedProjectile::Spawn
================
*/
void idGuidedProjectile::Spawn()
{
}

/*
================
idGuidedProjectile::Save
================
*/
void idGuidedProjectile::Save( idSaveGame* savefile ) const
{
	enemy.Save( savefile );
	savefile->WriteFloat( speed );
	savefile->WriteAngles( rndScale );
	savefile->WriteAngles( rndAng );
	savefile->WriteInt( rndUpdateTime );
	savefile->WriteFloat( turn_max );
	savefile->WriteFloat( clamp_dist );
	savefile->WriteAngles( angles );
	savefile->WriteBool( burstMode );
	savefile->WriteBool( unGuided );
	savefile->WriteFloat( burstDist );
	savefile->WriteFloat( burstVelocity );
}

/*
================
idGuidedProjectile::Restore
================
*/
void idGuidedProjectile::Restore( idRestoreGame* savefile )
{
	enemy.Restore( savefile );
	savefile->ReadFloat( speed );
	savefile->ReadAngles( rndScale );
	savefile->ReadAngles( rndAng );
	savefile->ReadInt( rndUpdateTime );
	savefile->ReadFloat( turn_max );
	savefile->ReadFloat( clamp_dist );
	savefile->ReadAngles( angles );
	savefile->ReadBool( burstMode );
	savefile->ReadBool( unGuided );
	savefile->ReadFloat( burstDist );
	savefile->ReadFloat( burstVelocity );
}


/*
================
idGuidedProjectile::GetSeekPos
================
*/
void idGuidedProjectile::GetSeekPos( idVec3& out )
{
	idEntity* enemyEnt = enemy.GetEntity();
	if( enemyEnt )
	{
		if( enemyEnt->IsType( idActor::Type ) )
		{
			out = static_cast<idActor*>( enemyEnt )->GetEyePosition();
			out.z -= 12.0f;
		}
		else
		{
			out = enemyEnt->GetPhysics()->GetOrigin();
		}
	}
	else
	{
		out = GetPhysics()->GetOrigin() + physicsObj.GetLinearVelocity() * 2.0f;
	}
}

/*
================
idGuidedProjectile::Think
================
*/
void idGuidedProjectile::Think()
{
	idVec3		dir;
	idVec3		seekPos;
	idVec3		velocity;
	idVec3		nose;
	idVec3		tmp;
	idMat3		axis;
	idAngles	dirAng;
	idAngles	diff;
	float		dist;
	float		frac;
	int			i;

	if( state == LAUNCHED && !unGuided )
	{

		GetSeekPos( seekPos );

		if( rndUpdateTime < gameLocal.time )
		{
			rndAng[ 0 ] = rndScale[ 0 ] * gameLocal.random.CRandomFloat();
			rndAng[ 1 ] = rndScale[ 1 ] * gameLocal.random.CRandomFloat();
			rndAng[ 2 ] = rndScale[ 2 ] * gameLocal.random.CRandomFloat();
			rndUpdateTime = gameLocal.time + 200;
		}

		nose = physicsObj.GetOrigin() + 10.0f * physicsObj.GetAxis()[0];

		dir = seekPos - nose;
		dist = dir.Normalize();
		dirAng = dir.ToAngles();

		// make it more accurate as it gets closer
		frac = dist / clamp_dist;
		if( frac > 1.0f )
		{
			frac = 1.0f;
		}

		diff = dirAng - angles + rndAng * frac;

		// clamp the to the max turn rate
		diff.Normalize180();
		for( i = 0; i < 3; i++ )
		{
			if( diff[ i ] > turn_max )
			{
				diff[ i ] = turn_max;
			}
			else if( diff[ i ] < -turn_max )
			{
				diff[ i ] = -turn_max;
			}
		}
		angles += diff;

		// make the visual model always points the dir we're traveling
		dir = angles.ToForward();
		velocity = dir * speed;

		if( burstMode && dist < burstDist )
		{
			unGuided = true;
			velocity *= burstVelocity;
		}

		physicsObj.SetLinearVelocity( velocity );

		// align z-axis of model with the direction
		axis = dir.ToMat3();
		tmp = axis[2];
		axis[2] = axis[0];
		axis[0] = -tmp;

		GetPhysics()->SetAxis( axis );
	}

	idProjectile::Think();
}

/*
=================
idGuidedProjectile::Launch
=================
*/
void idGuidedProjectile::Launch( const idVec3& start, const idVec3& dir, const idVec3& pushVelocity, const float timeSinceFire, const float launchPower, float dmgPower )
{
	idProjectile::Launch( start, dir, pushVelocity, timeSinceFire, launchPower, dmgPower );
	if( owner.GetEntity() )
	{
		if( owner.GetEntity()->IsType( idAI::Type ) )
		{
			enemy = static_cast<idAI*>( owner.GetEntity() )->GetEnemy();
		}
		else if( owner.GetEntity()->IsType( idPlayer::Type ) )
		{
			trace_t tr;
			idPlayer* player = static_cast<idPlayer*>( owner.GetEntity() );
			idVec3 start = player->GetEyePosition();
			idVec3 end = start + player->viewAxis[0] * 1000.0f;
			gameLocal.clip.TracePoint( tr, start, end, MASK_SHOT_RENDERMODEL | CONTENTS_BODY, owner.GetEntity() );
			if( tr.fraction < 1.0f )
			{
				enemy = gameLocal.GetTraceEntity( tr );
			}
			// ignore actors on the player's team
			if( enemy.GetEntity() == NULL || !enemy.GetEntity()->IsType( idActor::Type ) || ( static_cast<idActor*>( enemy.GetEntity() )->team == player->team ) )
			{
				enemy = player->EnemyWithMostHealth();
			}
		}
	}
	const idVec3& vel = physicsObj.GetLinearVelocity();
	angles = vel.ToAngles();
	speed = vel.Length();
	rndScale = spawnArgs.GetAngles( "random", "15 15 0" );
	turn_max = spawnArgs.GetFloat( "turn_max", "180" ) / com_engineHz_latched;
	clamp_dist = spawnArgs.GetFloat( "clamp_dist", "256" );
	burstMode = spawnArgs.GetBool( "burstMode" );
	unGuided = false;
	burstDist = spawnArgs.GetFloat( "burstDist", "64" );
	burstVelocity = spawnArgs.GetFloat( "burstVelocity", "1.25" );
	UpdateVisuals();
}

void idGuidedProjectile::SetEnemy( idEntity* ent )
{
	enemy = ent;
}
void idGuidedProjectile::Event_SetEnemy( idEntity* ent )
{
	SetEnemy( ent );
}

/*
===============================================================================

idSoulCubeMissile

===============================================================================
*/

CLASS_DECLARATION( idGuidedProjectile, idSoulCubeMissile )
END_CLASS

/*
================
idSoulCubeMissile::Spawn()
================
*/
void idSoulCubeMissile::Spawn()
{
	startingVelocity.Zero();
	endingVelocity.Zero();
	accelTime = 0.0f;
	launchTime = 0;
	killPhase = false;
	returnPhase = false;
	smokeKillTime = 0;
	smokeKill = NULL;
}

/*
=================
idSoulCubeMissile::~idSoulCubeMissile
=================
*/
idSoulCubeMissile::~idSoulCubeMissile()
{
}

/*
================
idSoulCubeMissile::Save
================
*/
void idSoulCubeMissile::Save( idSaveGame* savefile ) const
{
	savefile->WriteVec3( startingVelocity );
	savefile->WriteVec3( endingVelocity );
	savefile->WriteFloat( accelTime );
	savefile->WriteInt( launchTime );
	savefile->WriteBool( killPhase );
	savefile->WriteBool( returnPhase );
	savefile->WriteVec3( destOrg );
	savefile->WriteInt( orbitTime );
	savefile->WriteVec3( orbitOrg );
	savefile->WriteInt( smokeKillTime );
	savefile->WriteParticle( smokeKill );
}

/*
================
idSoulCubeMissile::Restore
================
*/
void idSoulCubeMissile::Restore( idRestoreGame* savefile )
{
	savefile->ReadVec3( startingVelocity );
	savefile->ReadVec3( endingVelocity );
	savefile->ReadFloat( accelTime );
	savefile->ReadInt( launchTime );
	savefile->ReadBool( killPhase );
	savefile->ReadBool( returnPhase );
	savefile->ReadVec3( destOrg );
	savefile->ReadInt( orbitTime );
	savefile->ReadVec3( orbitOrg );
	savefile->ReadInt( smokeKillTime );
	savefile->ReadParticle( smokeKill );
}

/*
================
idSoulCubeMissile::KillTarget
================
*/
void idSoulCubeMissile::KillTarget( const idVec3& dir )
{
	idEntity*	ownerEnt;
	const char*	smokeName;
	idActor*		act;

	ReturnToOwner();
	if( enemy.GetEntity() && enemy.GetEntity()->IsType( idActor::Type ) )
	{
		act = static_cast<idActor*>( enemy.GetEntity() );
		killPhase = true;
		orbitOrg = act->GetPhysics()->GetAbsBounds().GetCenter();
		orbitTime = gameLocal.time;
		smokeKillTime = 0;
		smokeName = spawnArgs.GetString( "smoke_kill" );
		if( *smokeName != '\0' )
		{
			smokeKill = static_cast<const idDeclParticle*>( declManager->FindType( DECL_PARTICLE, smokeName ) );
			smokeKillTime = gameLocal.time;
		}
		ownerEnt = owner.GetEntity();
		if( ( act->health > 0 ) && ownerEnt != NULL && ownerEnt->IsType( idPlayer::Type ) && ( ownerEnt->health > 0 ) && !act->spawnArgs.GetBool( "boss" ) )
		{
			static_cast<idPlayer*>( ownerEnt )->GiveHealthPool( act->health );
		}
		act->Damage( this, owner.GetEntity(), dir,  spawnArgs.GetString( "def_damage" ), 1.0f, INVALID_JOINT );
		act->GetAFPhysics()->SetTimeScale( 0.25 );
		StartSound( "snd_explode", SND_CHANNEL_BODY, 0, false, NULL );
	}
}

/*
================
idSoulCubeMissile::Think
================
*/
void idSoulCubeMissile::Think()
{
	float		pct;
	idVec3		seekPos;
	idEntity*	ownerEnt;

	if( state == LAUNCHED )
	{
		if( killPhase )
		{
			// orbit the mob, cascading down
			if( gameLocal.time < orbitTime + 1500 )
			{
				if( !gameLocal.smokeParticles->EmitSmoke( smokeKill, smokeKillTime, gameLocal.random.CRandomFloat(), orbitOrg, mat3_identity, timeGroup /*_D3XP*/ ) )
				{
					smokeKillTime = gameLocal.time;
				}
			}
		}
		else
		{
			if( accelTime && gameLocal.time < launchTime + accelTime * 1000 )
			{
				pct = ( gameLocal.time - launchTime ) / ( accelTime * 1000 );
				speed = ( startingVelocity + ( startingVelocity + endingVelocity ) * pct ).Length();
			}
		}
		idGuidedProjectile::Think();
		GetSeekPos( seekPos );
		if( ( seekPos - physicsObj.GetOrigin() ).Length() < 32.0f )
		{
			if( returnPhase )
			{
				StopSound( SND_CHANNEL_ANY, false );
				StartSound( "snd_return", SND_CHANNEL_BODY2, 0, false, NULL );
				Hide();
				PostEventSec( &EV_Remove, 2.0f );

				ownerEnt = owner.GetEntity();
				if( ownerEnt != NULL && ownerEnt->IsType( idPlayer::Type ) )
				{
					static_cast<idPlayer*>( ownerEnt )->SetSoulCubeProjectile( NULL );
				}

				state = FIZZLED;
			}
			else if( !killPhase )
			{
				KillTarget( physicsObj.GetAxis()[0] );
			}
		}
	}
}

/*
================
idSoulCubeMissile::GetSeekPos
================
*/
void idSoulCubeMissile::GetSeekPos( idVec3& out )
{
	if( returnPhase && owner.GetEntity() && owner.GetEntity()->IsType( idActor::Type ) )
	{
		idActor* act = static_cast<idActor*>( owner.GetEntity() );
		out = act->GetEyePosition();
		return;
	}
	if( destOrg != vec3_zero )
	{
		out = destOrg;
		return;
	}
	idGuidedProjectile::GetSeekPos( out );
}


/*
================
idSoulCubeMissile::Event_ReturnToOwner
================
*/
void idSoulCubeMissile::ReturnToOwner()
{
	speed *= 0.65f;
	killPhase = false;
	returnPhase = true;
	smokeFlyTime = 0;
}


/*
=================
idSoulCubeMissile::Launch
=================
*/
void idSoulCubeMissile::Launch( const idVec3& start, const idVec3& dir, const idVec3& pushVelocity, const float timeSinceFire, const float launchPower, float dmgPower )
{
	idVec3		newStart;
	idVec3		offs;
	idEntity*	ownerEnt;

	// push it out a little
	newStart = start + dir * spawnArgs.GetFloat( "launchDist" );
	offs = spawnArgs.GetVector( "launchOffset", "0 0 -4" );
	newStart += offs;
	idGuidedProjectile::Launch( newStart, dir, pushVelocity, timeSinceFire, launchPower, dmgPower );
	if( enemy.GetEntity() == NULL || !enemy.GetEntity()->IsType( idActor::Type ) )
	{
		destOrg = start + dir * 256.0f;
	}
	else
	{
		destOrg.Zero();
	}
	physicsObj.SetClipMask( 0 ); // never collide.. think routine will decide when to detonate
	startingVelocity = spawnArgs.GetVector( "startingVelocity", "15 0 0" );
	endingVelocity = spawnArgs.GetVector( "endingVelocity", "1500 0 0" );
	accelTime = spawnArgs.GetFloat( "accelTime", "5" );
	physicsObj.SetLinearVelocity( startingVelocity.Length() * physicsObj.GetAxis()[2] );
	launchTime = gameLocal.time;
	killPhase = false;
	UpdateVisuals();

	ownerEnt = owner.GetEntity();
	if( ownerEnt != NULL && ownerEnt->IsType( idPlayer::Type ) )
	{
		static_cast<idPlayer*>( ownerEnt )->SetSoulCubeProjectile( this );
	}

}


/*
===============================================================================

idBFGProjectile

===============================================================================
*/
const idEventDef EV_RemoveBeams( "<removeBeams>", NULL );

CLASS_DECLARATION( idProjectile, idBFGProjectile )
EVENT( EV_RemoveBeams,		idBFGProjectile::Event_RemoveBeams )
END_CLASS


/*
=================
idBFGProjectile::idBFGProjectile
=================
*/
idBFGProjectile::idBFGProjectile()
{
	memset( &secondModel, 0, sizeof( secondModel ) );
	secondModelDefHandle = -1;
	nextDamageTime = 0;
}

/*
=================
idBFGProjectile::~idBFGProjectile
=================
*/
idBFGProjectile::~idBFGProjectile()
{
	FreeBeams();

	if( secondModelDefHandle >= 0 )
	{
		gameRenderWorld->FreeEntityDef( secondModelDefHandle );
		secondModelDefHandle = -1;
	}
}

/*
================
idBFGProjectile::Spawn
================
*/
void idBFGProjectile::Spawn()
{
	beamTargets.Clear();
	memset( &secondModel, 0, sizeof( secondModel ) );
	secondModelDefHandle = -1;
	const char* temp = spawnArgs.GetString( "model_two" );
	if( temp != NULL && *temp != '\0' )
	{
		secondModel.hModel = renderModelManager->FindModel( temp );
		secondModel.bounds = secondModel.hModel->Bounds( &secondModel );
		secondModel.shaderParms[ SHADERPARM_RED ] =
			secondModel.shaderParms[ SHADERPARM_GREEN ] =
				secondModel.shaderParms[ SHADERPARM_BLUE ] =
					secondModel.shaderParms[ SHADERPARM_ALPHA ] = 1.0f;
		secondModel.noSelfShadow = true;
		secondModel.noShadow = true;
	}
	nextDamageTime = 0;
	damageFreq = NULL;
}

/*
================
idBFGProjectile::Save
================
*/
void idBFGProjectile::Save( idSaveGame* savefile ) const
{
	int i;

	savefile->WriteInt( beamTargets.Num() );
	for( i = 0; i < beamTargets.Num(); i++ )
	{
		beamTargets[i].target.Save( savefile );
		savefile->WriteRenderEntity( beamTargets[i].renderEntity );
		savefile->WriteInt( beamTargets[i].modelDefHandle );
	}

	savefile->WriteRenderEntity( secondModel );
	savefile->WriteInt( secondModelDefHandle );
	savefile->WriteInt( nextDamageTime );
	savefile->WriteString( damageFreq );
}

/*
================
idBFGProjectile::Restore
================
*/
void idBFGProjectile::Restore( idRestoreGame* savefile )
{
	int i, num;

	savefile->ReadInt( num );
	beamTargets.SetNum( num );
	for( i = 0; i < num; i++ )
	{
		beamTargets[i].target.Restore( savefile );
		savefile->ReadRenderEntity( beamTargets[i].renderEntity );
		savefile->ReadInt( beamTargets[i].modelDefHandle );

		if( beamTargets[i].modelDefHandle >= 0 )
		{
			beamTargets[i].modelDefHandle = gameRenderWorld->AddEntityDef( &beamTargets[i].renderEntity );
		}
	}

	savefile->ReadRenderEntity( secondModel );
	savefile->ReadInt( secondModelDefHandle );
	savefile->ReadInt( nextDamageTime );
	savefile->ReadString( damageFreq );

	if( secondModelDefHandle >= 0 )
	{
		secondModelDefHandle = gameRenderWorld->AddEntityDef( &secondModel );
	}
}

/*
=================
idBFGProjectile::FreeBeams
=================
*/
void idBFGProjectile::FreeBeams()
{
	for( int i = 0; i < beamTargets.Num(); i++ )
	{
		if( beamTargets[i].modelDefHandle >= 0 )
		{
			gameRenderWorld->FreeEntityDef( beamTargets[i].modelDefHandle );
			beamTargets[i].modelDefHandle = -1;
		}
	}

	idPlayer* player = gameLocal.GetLocalPlayer();
	if( player )
	{
		player->playerView.EnableBFGVision( false );
	}
}

/*
================
idBFGProjectile::Think
================
*/
void idBFGProjectile::Think()
{
	if( state == LAUNCHED )
	{

		// update beam targets
		for( int i = 0; i < beamTargets.Num(); i++ )
		{
			if( beamTargets[i].target.GetEntity() == NULL )
			{
				continue;
			}
			idPlayer* player = ( beamTargets[i].target.GetEntity()->IsType( idPlayer::Type ) ) ? static_cast<idPlayer*>( beamTargets[i].target.GetEntity() ) : NULL;
			// Major hack for end boss.  :(
			idAnimatedEntity*	beamEnt;
			idVec3				org;
			bool				forceDamage = false;

			beamEnt = static_cast<idAnimatedEntity*>( beamTargets[i].target.GetEntity() );
			if( !idStr::Cmp( beamEnt->GetEntityDefName(), "monster_boss_d3xp_maledict" ) )
			{
				SetTimeState	ts( beamEnt->timeGroup );
				idMat3			temp;
				jointHandle_t	bodyJoint;

				bodyJoint = beamEnt->GetAnimator()->GetJointHandle( "Chest1" );
				beamEnt->GetJointWorldTransform( bodyJoint, gameLocal.time, org, temp );

				forceDamage = true;
			}
			else
			{
				org = beamEnt->GetPhysics()->GetAbsBounds().GetCenter();
			}
			beamTargets[i].renderEntity.origin = GetPhysics()->GetOrigin();
			beamTargets[i].renderEntity.shaderParms[ SHADERPARM_BEAM_END_X ] = org.x;
			beamTargets[i].renderEntity.shaderParms[ SHADERPARM_BEAM_END_Y ] = org.y;
			beamTargets[i].renderEntity.shaderParms[ SHADERPARM_BEAM_END_Z ] = org.z;
			beamTargets[i].renderEntity.shaderParms[ SHADERPARM_RED ] =
				beamTargets[i].renderEntity.shaderParms[ SHADERPARM_GREEN ] =
					beamTargets[i].renderEntity.shaderParms[ SHADERPARM_BLUE ] =
						beamTargets[i].renderEntity.shaderParms[ SHADERPARM_ALPHA ] = 1.0f;
			if( gameLocal.time > nextDamageTime )
			{
				bool bfgVision = true;
				if( damageFreq && *( const char* )damageFreq && beamTargets[i].target.GetEntity() && ( forceDamage || beamTargets[i].target.GetEntity()->CanDamage( GetPhysics()->GetOrigin(), org ) ) )
				{
					org = beamTargets[i].target.GetEntity()->GetPhysics()->GetOrigin() - GetPhysics()->GetOrigin();
					org.Normalize();
					beamTargets[i].target.GetEntity()->Damage( this, owner.GetEntity(), org, damageFreq, ( damagePower ) ? damagePower : 1.0f, INVALID_JOINT );
				}
				else
				{
					beamTargets[i].renderEntity.shaderParms[ SHADERPARM_RED ] =
						beamTargets[i].renderEntity.shaderParms[ SHADERPARM_GREEN ] =
							beamTargets[i].renderEntity.shaderParms[ SHADERPARM_BLUE ] =
								beamTargets[i].renderEntity.shaderParms[ SHADERPARM_ALPHA ] = 0.0f;
					bfgVision = false;
				}
				if( player )
				{
					player->playerView.EnableBFGVision( bfgVision );
				}
				nextDamageTime = gameLocal.time + BFG_DAMAGE_FREQUENCY;
			}
			gameRenderWorld->UpdateEntityDef( beamTargets[i].modelDefHandle, &beamTargets[i].renderEntity );
		}

		if( secondModelDefHandle >= 0 )
		{
			secondModel.origin = GetPhysics()->GetOrigin();
			gameRenderWorld->UpdateEntityDef( secondModelDefHandle, &secondModel );
		}

		idAngles ang;

		ang.pitch = ( gameLocal.time & 4095 ) * 360.0f / -4096.0f;
		ang.yaw = ang.pitch;
		ang.roll = 0.0f;
		SetAngles( ang );

		ang.pitch = ( gameLocal.time & 2047 ) * 360.0f / -2048.0f;
		ang.yaw = ang.pitch;
		ang.roll = 0.0f;
		secondModel.axis = ang.ToMat3();

		UpdateVisuals();
	}

	idProjectile::Think();
}

/*
=================
idBFGProjectile::Launch
=================
*/
void idBFGProjectile::Launch( const idVec3& start, const idVec3& dir, const idVec3& pushVelocity, const float timeSinceFire, const float power, const float dmgPower )
{
	idProjectile::Launch( start, dir, pushVelocity, 0.0f, power, dmgPower );

	// dmgPower * radius is the target acquisition area
	// acquisition should make sure that monsters are not dormant
	// which will cut down on hitting monsters not actively fighting
	// but saves on the traces making sure they are visible
	// damage is not applied until the projectile explodes

	idEntity* 	ent;
	idEntity* 	entityList[ MAX_GENTITIES ];
	int			numListedEntities;
	idBounds	bounds;
	idVec3		damagePoint;

	float radius;
	spawnArgs.GetFloat( "damageRadius", "512", radius );
	bounds = idBounds( GetPhysics()->GetOrigin() ).Expand( radius );

	float beamWidth = spawnArgs.GetFloat( "beam_WidthFly" );
	const char* skin = spawnArgs.GetString( "skin_beam" );

	memset( &secondModel, 0, sizeof( secondModel ) );
	secondModelDefHandle = -1;
	const char* temp = spawnArgs.GetString( "model_two" );
	if( temp != NULL && *temp != '\0' )
	{
		secondModel.hModel = renderModelManager->FindModel( temp );
		secondModel.bounds = secondModel.hModel->Bounds( &secondModel );
		secondModel.shaderParms[ SHADERPARM_RED ] =
			secondModel.shaderParms[ SHADERPARM_GREEN ] =
				secondModel.shaderParms[ SHADERPARM_BLUE ] =
					secondModel.shaderParms[ SHADERPARM_ALPHA ] = 1.0f;
		secondModel.noSelfShadow = true;
		secondModel.noShadow = true;
		secondModel.origin = GetPhysics()->GetOrigin();
		secondModel.axis = GetPhysics()->GetAxis();
		secondModelDefHandle = gameRenderWorld->AddEntityDef( &secondModel );
	}

	idVec3 delta( 15.0f, 15.0f, 15.0f );
	//physicsObj.SetAngularExtrapolation( extrapolation_t(EXTRAPOLATION_LINEAR|EXTRAPOLATION_NOSTOP), gameLocal.time, 0, physicsObj.GetAxis().ToAngles(), delta, ang_zero );

	// get all entities touching the bounds
	numListedEntities = gameLocal.clip.EntitiesTouchingBounds( bounds, CONTENTS_BODY, entityList, MAX_GENTITIES );
	for( int e = 0; e < numListedEntities; e++ )
	{
		ent = entityList[ e ];
		assert( ent );

		if( ent == this || ent == owner.GetEntity() || ent->IsHidden() || !ent->IsActive() || !ent->fl.takedamage || ent->health <= 0 || !ent->IsType( idActor::Type ) )
		{
			continue;
		}

		if( !ent->CanDamage( GetPhysics()->GetOrigin(), damagePoint ) )
		{
			continue;
		}

		if( ent->IsType( idPlayer::Type ) )
		{
			idPlayer* player = static_cast<idPlayer*>( ent );
			player->playerView.EnableBFGVision( true );
		}

		beamTarget_t bt;
		memset( &bt.renderEntity, 0, sizeof( renderEntity_t ) );
		bt.renderEntity.origin = GetPhysics()->GetOrigin();
		bt.renderEntity.axis = GetPhysics()->GetAxis();
		bt.renderEntity.shaderParms[ SHADERPARM_BEAM_WIDTH ] = beamWidth;
		bt.renderEntity.shaderParms[ SHADERPARM_RED ] = 1.0f;
		bt.renderEntity.shaderParms[ SHADERPARM_GREEN ] = 1.0f;
		bt.renderEntity.shaderParms[ SHADERPARM_BLUE ] = 1.0f;
		bt.renderEntity.shaderParms[ SHADERPARM_ALPHA ] = 1.0f;
		bt.renderEntity.shaderParms[ SHADERPARM_DIVERSITY] = gameLocal.random.CRandomFloat() * 0.75;
		bt.renderEntity.hModel = renderModelManager->FindModel( "_beam" );
		bt.renderEntity.callback = NULL;
		bt.renderEntity.numJoints = 0;
		bt.renderEntity.joints = NULL;
		bt.renderEntity.bounds.Clear();
		bt.renderEntity.customSkin = declManager->FindSkin( skin );
		bt.target = ent;
		bt.modelDefHandle = gameRenderWorld->AddEntityDef( &bt.renderEntity );
		beamTargets.Append( bt );
	}

	// Major hack for end boss.  :(
	idAnimatedEntity* maledict = static_cast<idAnimatedEntity*>( gameLocal.FindEntity( "monster_boss_d3xp_maledict_1" ) );

	if( maledict )
	{
		SetTimeState	ts( maledict->timeGroup );

		idVec3			realPoint;
		idMat3			temp;
		float			dist;
		jointHandle_t	bodyJoint;

		bodyJoint = maledict->GetAnimator()->GetJointHandle( "Chest1" );
		maledict->GetJointWorldTransform( bodyJoint, gameLocal.time, realPoint, temp );

		dist = idVec3( realPoint - GetPhysics()->GetOrigin() ).Length();

		if( dist < radius )
		{
			beamTarget_t bt;
			memset( &bt.renderEntity, 0, sizeof( renderEntity_t ) );
			bt.renderEntity.origin = GetPhysics()->GetOrigin();
			bt.renderEntity.axis = GetPhysics()->GetAxis();
			bt.renderEntity.shaderParms[ SHADERPARM_BEAM_WIDTH ] = beamWidth;
			bt.renderEntity.shaderParms[ SHADERPARM_RED ] = 1.0f;
			bt.renderEntity.shaderParms[ SHADERPARM_GREEN ] = 1.0f;
			bt.renderEntity.shaderParms[ SHADERPARM_BLUE ] = 1.0f;
			bt.renderEntity.shaderParms[ SHADERPARM_ALPHA ] = 1.0f;
			bt.renderEntity.shaderParms[ SHADERPARM_DIVERSITY] = gameLocal.random.CRandomFloat() * 0.75;
			bt.renderEntity.hModel = renderModelManager->FindModel( "_beam" );
			bt.renderEntity.callback = NULL;
			bt.renderEntity.numJoints = 0;
			bt.renderEntity.joints = NULL;
			bt.renderEntity.bounds.Clear();
			bt.renderEntity.customSkin = declManager->FindSkin( skin );
			bt.target = maledict;
			bt.modelDefHandle = gameRenderWorld->AddEntityDef( &bt.renderEntity );
			beamTargets.Append( bt );

			numListedEntities++;
		}
	}

	if( numListedEntities )
	{
		StartSound( "snd_beam", SND_CHANNEL_BODY2, 0, false, NULL );
	}
	damageFreq = spawnArgs.GetString( "def_damageFreq" );
	nextDamageTime = gameLocal.time + BFG_DAMAGE_FREQUENCY;
	UpdateVisuals();
}

/*
================
idProjectile::Event_RemoveBeams
================
*/
void idBFGProjectile::Event_RemoveBeams()
{
	FreeBeams();
	UpdateVisuals();
}

/*
================
idProjectile::Explode
================
*/
void idBFGProjectile::Explode( const trace_t& collision, idEntity* ignore )
{
	int			i;
	idVec3		dmgPoint;
	idVec3		dir;
	float		beamWidth;
	float		damageScale;
	const char* damage;
	idPlayer* 	player;
	idEntity* 	ownerEnt;

	ownerEnt = owner.GetEntity();
	if( ownerEnt != NULL && ownerEnt->IsType( idPlayer::Type ) )
	{
		player = static_cast< idPlayer* >( ownerEnt );
	}
	else
	{
		player = NULL;
	}

	beamWidth = spawnArgs.GetFloat( "beam_WidthExplode" );
	damage = spawnArgs.GetString( "def_damage" );

	for( i = 0; i < beamTargets.Num(); i++ )
	{
		if( ( beamTargets[i].target.GetEntity() == NULL ) || ( ownerEnt == NULL ) )
		{
			continue;
		}

		if( !beamTargets[i].target.GetEntity()->CanDamage( GetPhysics()->GetOrigin(), dmgPoint ) )
		{
			continue;
		}

		beamTargets[i].renderEntity.shaderParms[SHADERPARM_BEAM_WIDTH] = beamWidth;

		// if the hit entity takes damage
		if( damagePower )
		{
			damageScale = damagePower;
		}
		else
		{
			damageScale = 1.0f;
		}

		// if the projectile owner is a player
		if( player )
		{
			// if the projectile hit an actor
			if( beamTargets[i].target.GetEntity()->IsType( idActor::Type ) )
			{
				player->SetLastHitTime( gameLocal.time );
				player->AddProjectileHits( 1 );
				damageScale *= player->PowerUpModifier( PROJECTILE_DAMAGE );
			}
		}

		if( damage[0] && ( beamTargets[i].target.GetEntity()->entityNumber > gameLocal.numClients - 1 ) )
		{
			dir = beamTargets[i].target.GetEntity()->GetPhysics()->GetOrigin() - GetPhysics()->GetOrigin();
			dir.Normalize();
			beamTargets[i].target.GetEntity()->Damage( this, ownerEnt, dir, damage, damageScale, ( collision.c.id < 0 ) ? CLIPMODEL_ID_TO_JOINT_HANDLE( collision.c.id ) : INVALID_JOINT );
		}
	}

	if( secondModelDefHandle >= 0 )
	{
		gameRenderWorld->FreeEntityDef( secondModelDefHandle );
		secondModelDefHandle = -1;
	}

	if( ignore == NULL )
	{
		projectileFlags.noSplashDamage = true;
	}

	if( !common->IsClient() || fl.skipReplication )
	{
		if( ignore != NULL )
		{
			PostEventMS( &EV_RemoveBeams, 750 );
		}
		else
		{
			PostEventMS( &EV_RemoveBeams, 0 );
		}
	}

	return idProjectile::Explode( collision, ignore );
}


/*
===============================================================================

	idDebris

===============================================================================
*/

CLASS_DECLARATION( idEntity, idDebris )
EVENT( EV_Explode,			idDebris::Event_Explode )
EVENT( EV_Fizzle,			idDebris::Event_Fizzle )
END_CLASS

/*
================
idDebris::Spawn
================
*/
void idDebris::Spawn()
{
	owner = NULL;
	smokeFly = NULL;
	smokeFlyTime = 0;
}

/*
================
idDebris::Create
================
*/
void idDebris::Create( idEntity* owner, const idVec3& start, const idMat3& axis )
{
	Unbind();
	GetPhysics()->SetOrigin( start );
	GetPhysics()->SetAxis( axis );
	GetPhysics()->SetContents( 0 );
	this->owner = owner;
	smokeFly = NULL;
	smokeFlyTime = 0;
	sndBounce = NULL;
	noGrab = true;
	UpdateVisuals();
}

/*
=================
idDebris::idDebris
=================
*/
idDebris::idDebris()
{
	owner = NULL;
	smokeFly = NULL;
	smokeFlyTime = 0;
	sndBounce = NULL;
}

/*
=================
idDebris::~idDebris
=================
*/
idDebris::~idDebris()
{
}

/*
=================
idDebris::Save
=================
*/
void idDebris::Save( idSaveGame* savefile ) const
{
	owner.Save( savefile );

	savefile->WriteStaticObject( physicsObj );

	savefile->WriteParticle( smokeFly );
	savefile->WriteInt( smokeFlyTime );
	savefile->WriteSoundShader( sndBounce );
}

/*
=================
idDebris::Restore
=================
*/
void idDebris::Restore( idRestoreGame* savefile )
{
	owner.Restore( savefile );

	savefile->ReadStaticObject( physicsObj );
	RestorePhysics( &physicsObj );

	savefile->ReadParticle( smokeFly );
	savefile->ReadInt( smokeFlyTime );
	savefile->ReadSoundShader( sndBounce );
}

/*
=================
idDebris::Launch
=================
*/
void idDebris::Launch()
{
	float		fuse;
	idVec3		velocity;
	idAngles	angular_velocity;
	float		linear_friction;
	float		angular_friction;
	float		contact_friction;
	float		bounce;
	float		mass;
	float		gravity;
	idVec3		gravVec;
	bool		randomVelocity;
	idMat3		axis;

	renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ] = -MS2SEC( gameLocal.time );

	spawnArgs.GetVector( "velocity", "0 0 0", velocity );
	spawnArgs.GetAngles( "angular_velocity", "0 0 0", angular_velocity );

	linear_friction		= spawnArgs.GetFloat( "linear_friction" );
	angular_friction	= spawnArgs.GetFloat( "angular_friction" );
	contact_friction	= spawnArgs.GetFloat( "contact_friction" );
	bounce				= spawnArgs.GetFloat( "bounce" );
	mass				= spawnArgs.GetFloat( "mass" );
	gravity				= spawnArgs.GetFloat( "gravity" );
	fuse				= spawnArgs.GetFloat( "fuse" );
	randomVelocity		= spawnArgs.GetBool( "random_velocity" );

	if( mass <= 0 )
	{
		gameLocal.Error( "Invalid mass on '%s'\n", GetEntityDefName() );
	}

	if( randomVelocity )
	{
		velocity.x *= gameLocal.random.RandomFloat() + 0.5f;
		velocity.y *= gameLocal.random.RandomFloat() + 0.5f;
		velocity.z *= gameLocal.random.RandomFloat() + 0.5f;
	}

	if( health )
	{
		fl.takedamage = true;
	}

	gravVec = gameLocal.GetGravity();
	gravVec.NormalizeFast();
	axis = GetPhysics()->GetAxis();

	Unbind();

	physicsObj.SetSelf( this );

	// check if a clip model is set
	const char* clipModelName;
	idTraceModel trm;
	spawnArgs.GetString( "clipmodel", "", &clipModelName );
	if( !clipModelName[0] )
	{
		clipModelName = spawnArgs.GetString( "model" );		// use the visual model
	}

	// load the trace model
	if( !collisionModelManager->TrmFromModel( clipModelName, trm ) )
	{
		// default to a box
		physicsObj.SetClipBox( renderEntity.bounds, 1.0f );
	}
	else
	{
		physicsObj.SetClipModel( new( TAG_PHYSICS_CLIP_ENTITY ) idClipModel( trm ), 1.0f );
	}

	physicsObj.GetClipModel()->SetOwner( owner.GetEntity() );
	physicsObj.SetMass( mass );
	physicsObj.SetFriction( linear_friction, angular_friction, contact_friction );
	if( contact_friction == 0.0f )
	{
		physicsObj.NoContact();
	}
	physicsObj.SetBouncyness( bounce );
	physicsObj.SetGravity( gravVec * gravity );
	physicsObj.SetContents( 0 );
	physicsObj.SetClipMask( MASK_SOLID | CONTENTS_MOVEABLECLIP );
	physicsObj.SetLinearVelocity( axis[ 0 ] * velocity[ 0 ] + axis[ 1 ] * velocity[ 1 ] + axis[ 2 ] * velocity[ 2 ] );
	physicsObj.SetAngularVelocity( angular_velocity.ToAngularVelocity() * axis );
	physicsObj.SetOrigin( GetPhysics()->GetOrigin() );
	physicsObj.SetAxis( axis );
	SetPhysics( &physicsObj );

	if( !common->IsClient() )
	{
		if( fuse <= 0 )
		{
			// run physics for 1 second
			RunPhysics();
			PostEventMS( &EV_Remove, 0 );
		}
		else if( spawnArgs.GetBool( "detonate_on_fuse" ) )
		{
			if( fuse < 0.0f )
			{
				fuse = 0.0f;
			}
			RunPhysics();
			PostEventSec( &EV_Explode, fuse );
		}
		else
		{
			if( fuse < 0.0f )
			{
				fuse = 0.0f;
			}
			PostEventSec( &EV_Fizzle, fuse );
		}
	}

	StartSound( "snd_fly", SND_CHANNEL_BODY, 0, false, NULL );

	smokeFly = NULL;
	smokeFlyTime = 0;
	const char* smokeName = spawnArgs.GetString( "smoke_fly" );
	if( *smokeName != '\0' )
	{
		smokeFly = static_cast<const idDeclParticle*>( declManager->FindType( DECL_PARTICLE, smokeName ) );
		smokeFlyTime = gameLocal.time;
		gameLocal.smokeParticles->EmitSmoke( smokeFly, smokeFlyTime, gameLocal.random.CRandomFloat(), GetPhysics()->GetOrigin(), GetPhysics()->GetAxis(), timeGroup /*_D3XP*/ );
	}

	const char* sndName = spawnArgs.GetString( "snd_bounce" );
	if( *sndName != '\0' )
	{
		sndBounce = declManager->FindSound( sndName );
	}

	UpdateVisuals();
}

/*
================
idDebris::Think
================
*/
void idDebris::Think()
{

	// run physics
	RunPhysics();
	Present();

	if( smokeFly && smokeFlyTime )
	{
		if( !gameLocal.smokeParticles->EmitSmoke( smokeFly, smokeFlyTime, gameLocal.random.CRandomFloat(), GetPhysics()->GetOrigin(), GetPhysics()->GetAxis(), timeGroup /*_D3XP*/ ) )
		{
			smokeFlyTime = 0;
		}
	}
}

/*
================
idDebris::Killed
================
*/
void idDebris::Killed( idEntity* inflictor, idEntity* attacker, int damage, const idVec3& dir, int location )
{
	if( spawnArgs.GetBool( "detonate_on_death" ) )
	{
		Explode();
	}
	else
	{
		Fizzle();
	}
}

/*
=================
idDebris::Collide
=================
*/
bool idDebris::Collide( const trace_t& collision, const idVec3& velocity )
{
	if( sndBounce != NULL )
	{
		StartSoundShader( sndBounce, SND_CHANNEL_BODY, 0, false, NULL );
	}
	sndBounce = NULL;
	return false;
}


/*
================
idDebris::Fizzle
================
*/
void idDebris::Fizzle()
{
	if( IsHidden() )
	{
		// already exploded
		return;
	}

	StopSound( SND_CHANNEL_ANY, false );
	StartSound( "snd_fizzle", SND_CHANNEL_BODY, 0, false, NULL );

	// fizzle FX
	const char* smokeName = spawnArgs.GetString( "smoke_fuse" );
	if( *smokeName != '\0' )
	{
		smokeFly = static_cast<const idDeclParticle*>( declManager->FindType( DECL_PARTICLE, smokeName ) );
		smokeFlyTime = gameLocal.time;
		gameLocal.smokeParticles->EmitSmoke( smokeFly, smokeFlyTime, gameLocal.random.CRandomFloat(), GetPhysics()->GetOrigin(), GetPhysics()->GetAxis(), timeGroup /*_D3XP*/ );
	}

	fl.takedamage = false;
	physicsObj.SetContents( 0 );
	physicsObj.PutToRest();

	Hide();

	if( common->IsClient() && !fl.skipReplication )
	{
		return;
	}

	CancelEvents( &EV_Fizzle );
	PostEventMS( &EV_Remove, 0 );
}

/*
================
idDebris::Explode
================
*/
void idDebris::Explode()
{
	if( IsHidden() )
	{
		// already exploded
		return;
	}

	StopSound( SND_CHANNEL_ANY, false );
	StartSound( "snd_explode", SND_CHANNEL_BODY, 0, false, NULL );

	Hide();

	// these must not be "live forever" particle systems
	smokeFly = NULL;
	smokeFlyTime = 0;
	const char* smokeName = spawnArgs.GetString( "smoke_detonate" );
	if( *smokeName != '\0' )
	{
		smokeFly = static_cast<const idDeclParticle*>( declManager->FindType( DECL_PARTICLE, smokeName ) );
		smokeFlyTime = gameLocal.time;
		gameLocal.smokeParticles->EmitSmoke( smokeFly, smokeFlyTime, gameLocal.random.CRandomFloat(), GetPhysics()->GetOrigin(), GetPhysics()->GetAxis(), timeGroup /*_D3XP*/ );
	}

	fl.takedamage = false;
	physicsObj.SetContents( 0 );
	physicsObj.PutToRest();

	CancelEvents( &EV_Explode );
	PostEventMS( &EV_Remove, 0 );
}

/*
================
idDebris::Event_Explode
================
*/
void idDebris::Event_Explode()
{
	Explode();
}

/*
================
idDebris::Event_Fizzle
================
*/
void idDebris::Event_Fizzle()
{
	Fizzle();
}

/*
===============================================================================

	idHomingProjectile

===============================================================================
*/

CLASS_DECLARATION( idProjectile, idHomingProjectile )
EVENT( EV_SetEnemy,		idHomingProjectile::Event_SetEnemy )
END_CLASS

/*
================
idHomingProjectile::idHomingProjectile
================
*/
idHomingProjectile::idHomingProjectile()
{
	enemy			= NULL;
	speed			= 0.0f;
	turn_max		= 0.0f;
	clamp_dist		= 0.0f;
	rndScale		= ang_zero;
	rndAng			= ang_zero;
	angles			= ang_zero;
	burstMode		= false;
	burstDist		= 0;
	burstVelocity	= 0.0f;
	unGuided		= false;
	seekPos			= vec3_origin;
}

/*
=================
idHomingProjectile::~idHomingProjectile
=================
*/
idHomingProjectile::~idHomingProjectile()
{
}

/*
================
idHomingProjectile::Spawn
================
*/
void idHomingProjectile::Spawn()
{
}

/*
================
idHomingProjectile::Save
================
*/
void idHomingProjectile::Save( idSaveGame* savefile ) const
{
	enemy.Save( savefile );
	savefile->WriteFloat( speed );
	savefile->WriteAngles( rndScale );
	savefile->WriteAngles( rndAng );
	savefile->WriteFloat( turn_max );
	savefile->WriteFloat( clamp_dist );
	savefile->WriteAngles( angles );
	savefile->WriteBool( burstMode );
	savefile->WriteBool( unGuided );
	savefile->WriteFloat( burstDist );
	savefile->WriteFloat( burstVelocity );
	savefile->WriteVec3( seekPos );
}

/*
================
idHomingProjectile::Restore
================
*/
void idHomingProjectile::Restore( idRestoreGame* savefile )
{
	enemy.Restore( savefile );
	savefile->ReadFloat( speed );
	savefile->ReadAngles( rndScale );
	savefile->ReadAngles( rndAng );
	savefile->ReadFloat( turn_max );
	savefile->ReadFloat( clamp_dist );
	savefile->ReadAngles( angles );
	savefile->ReadBool( burstMode );
	savefile->ReadBool( unGuided );
	savefile->ReadFloat( burstDist );
	savefile->ReadFloat( burstVelocity );
	savefile->ReadVec3( seekPos );
}


/*
================
idHomingProjectile::Think
================
*/
void idHomingProjectile::Think()
{
	if( seekPos == vec3_zero )
	{
		// ai def uses a single def_projectile .. guardian has two projectile types so when seekPos is zero, just run regular projectile
		idProjectile::Think();
		return;
	}

	idVec3		dir;
	idVec3		velocity;
	idVec3		nose;
	idVec3		tmp;
	idMat3		axis;
	idAngles	dirAng;
	idAngles	diff;
	float		dist;
	float		frac;
	int			i;


	nose = physicsObj.GetOrigin() + 10.0f * physicsObj.GetAxis()[0];

	dir = seekPos - nose;
	dist = dir.Normalize();
	dirAng = dir.ToAngles();

	// make it more accurate as it gets closer
	frac = ( dist * 2.0f ) / clamp_dist;
	if( frac > 1.0f )
	{
		frac = 1.0f;
	}

	diff = dirAng - angles * frac;

	// clamp the to the max turn rate
	diff.Normalize180();
	for( i = 0; i < 3; i++ )
	{
		if( diff[ i ] > turn_max )
		{
			diff[ i ] = turn_max;
		}
		else if( diff[ i ] < -turn_max )
		{
			diff[ i ] = -turn_max;
		}
	}
	angles += diff;

	// make the visual model always points the dir we're traveling
	dir = angles.ToForward();
	velocity = dir * speed;

	if( burstMode && dist < burstDist )
	{
		unGuided = true;
		velocity *= burstVelocity;
	}

	physicsObj.SetLinearVelocity( velocity );

	// align z-axis of model with the direction
	axis = dir.ToMat3();
	tmp = axis[2];
	axis[2] = axis[0];
	axis[0] = -tmp;

	GetPhysics()->SetAxis( axis );

	idProjectile::Think();
}

/*
=================
idHomingProjectile::Launch
=================
*/
void idHomingProjectile::Launch( const idVec3& start, const idVec3& dir, const idVec3& pushVelocity, const float timeSinceFire, const float launchPower, float dmgPower )
{
	idProjectile::Launch( start, dir, pushVelocity, timeSinceFire, launchPower, dmgPower );
	if( owner.GetEntity() )
	{
		if( owner.GetEntity()->IsType( idAI::Type ) )
		{
			enemy = static_cast<idAI*>( owner.GetEntity() )->GetEnemy();
		}
		else if( owner.GetEntity()->IsType( idPlayer::Type ) )
		{
			trace_t tr;
			idPlayer* player = static_cast<idPlayer*>( owner.GetEntity() );
			idVec3 start = player->GetEyePosition();
			idVec3 end = start + player->viewAxis[0] * 1000.0f;
			gameLocal.clip.TracePoint( tr, start, end, MASK_SHOT_RENDERMODEL | CONTENTS_BODY, owner.GetEntity() );
			if( tr.fraction < 1.0f )
			{
				enemy = gameLocal.GetTraceEntity( tr );
			}
			// ignore actors on the player's team
			if( enemy.GetEntity() == NULL || !enemy.GetEntity()->IsType( idActor::Type ) || ( static_cast<idActor*>( enemy.GetEntity() )->team == player->team ) )
			{
				enemy = player->EnemyWithMostHealth();
			}
		}
	}
	const idVec3& vel = physicsObj.GetLinearVelocity();
	angles = vel.ToAngles();
	speed = vel.Length();
	rndScale = spawnArgs.GetAngles( "random", "15 15 0" );
	turn_max = spawnArgs.GetFloat( "turn_max", "180" ) / com_engineHz_latched;
	clamp_dist = spawnArgs.GetFloat( "clamp_dist", "256" );
	burstMode = spawnArgs.GetBool( "burstMode" );
	unGuided = false;
	burstDist = spawnArgs.GetFloat( "burstDist", "64" );
	burstVelocity = spawnArgs.GetFloat( "burstVelocity", "1.25" );
	UpdateVisuals();
}

void idHomingProjectile::SetEnemy( idEntity* ent )
{
	enemy = ent;
}
void idHomingProjectile::SetSeekPos( idVec3 pos )
{
	seekPos = pos;
}
void idHomingProjectile::Event_SetEnemy( idEntity* ent )
{
	SetEnemy( ent );
}
