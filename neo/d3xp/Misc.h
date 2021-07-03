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

#ifndef __GAME_MISC_H__
#define __GAME_MISC_H__


/*
===============================================================================

idSpawnableEntity

A simple, spawnable entity with a model and no functionable ability of it's own.
For example, it can be used as a placeholder during development, for marking
locations on maps for script, or for simple placed models without any behavior
that can be bound to other entities.  Should not be subclassed.
===============================================================================
*/

class idSpawnableEntity : public idEntity
{
public:
	CLASS_PROTOTYPE( idSpawnableEntity );

	void				Spawn();

private:
};

/*
===============================================================================

  Potential spawning position for players.
  The first time a player enters the game, they will be at an 'initial' spot.
  Targets will be fired when someone spawns in on them.

  When triggered, will cause player to be teleported to spawn spot.

===============================================================================
*/

class idPlayerStart : public idEntity
{
public:
	CLASS_PROTOTYPE( idPlayerStart );

	enum
	{
		EVENT_TELEPORTPLAYER = idEntity::EVENT_MAXEVENTS,
		EVENT_MAXEVENTS
	};

	idPlayerStart();

	void				Spawn();

	void				Save( idSaveGame* savefile ) const;
	void				Restore( idRestoreGame* savefile );

	virtual bool		ClientReceiveEvent( int event, int time, const idBitMsg& msg );

private:
	int					teleportStage;

	void				Event_TeleportPlayer( idEntity* activator );
	void				Event_TeleportStage( idEntity* player );
	void				TeleportPlayer( idPlayer* player );
};


/*
===============================================================================

  Non-displayed entity used to activate triggers when it touches them.
  Bind to a mover to have the mover activate a trigger as it moves.
  When target by triggers, activating the trigger will toggle the
  activator on and off. Check "start_off" to have it spawn disabled.

===============================================================================
*/

class idActivator : public idEntity
{
public:
	CLASS_PROTOTYPE( idActivator );

	void				Spawn();

	void				Save( idSaveGame* savefile ) const;
	void				Restore( idRestoreGame* savefile );

	virtual void		Think();

private:
	bool				stay_on;

	void				Event_Activate( idEntity* activator );
};


/*
===============================================================================

  Path entities for monsters to follow.

===============================================================================
*/
class idPathCorner : public idEntity
{
public:
	CLASS_PROTOTYPE( idPathCorner );

	void				Spawn();

	static void			DrawDebugInfo();

	static idPathCorner* RandomPath( const idEntity* source, const idEntity* ignore );

private:
	void				Event_RandomPath();
};


/*
===============================================================================

  Object that fires targets and changes shader parms when damaged.

===============================================================================
*/

class idDamagable : public idEntity
{
public:
	CLASS_PROTOTYPE( idDamagable );

	idDamagable();

	void				Save( idSaveGame* savefile ) const;
	void				Restore( idRestoreGame* savefile );

	void				Spawn();
	void				Killed( idEntity* inflictor, idEntity* attacker, int damage, const idVec3& dir, int location );

	virtual void		Hide();
	virtual void		Show();

private:
	int					count;
	int					nextTriggerTime;

	void				BecomeBroken( idEntity* activator );
	void				Event_BecomeBroken( idEntity* activator );
	void				Event_RestoreDamagable();
};


/*
===============================================================================

  Hidden object that explodes when activated

===============================================================================
*/

class idExplodable : public idEntity
{
public:
	CLASS_PROTOTYPE( idExplodable );

	void				Spawn();

private:
	void				Event_Explode( idEntity* activator );
};


/*
===============================================================================

  idSpring

===============================================================================
*/

class idSpring : public idEntity
{
public:
	CLASS_PROTOTYPE( idSpring );

	void				Spawn();

	virtual void		Think();

private:
	idEntity* 			ent1;
	idEntity* 			ent2;
	int					id1;
	int					id2;
	idVec3				p1;
	idVec3				p2;
	idForce_Spring		spring;

	void				Event_LinkSpring();
};


/*
===============================================================================

  idForceField

===============================================================================
*/

class idForceField : public idEntity
{
public:
	CLASS_PROTOTYPE( idForceField );

	void				Save( idSaveGame* savefile ) const;
	void				Restore( idRestoreGame* savefile );

	void				Spawn();

	virtual void		Think();
	virtual void		ClientThink( const int curTime, const float fraction, const bool predict ) ;
private:
	idForce_Field		forceField;

	void				Toggle();

	void				Event_Activate( idEntity* activator );
	void				Event_Toggle();
	void				Event_FindTargets();
};


/*
===============================================================================

  idAnimated

===============================================================================
*/

class idAnimated : public idAFEntity_Gibbable
{
public:
	CLASS_PROTOTYPE( idAnimated );

	idAnimated();
	~idAnimated();

	void					Save( idSaveGame* savefile ) const;
	void					Restore( idRestoreGame* savefile );

	void					Spawn();
	virtual bool			LoadAF();
	bool					StartRagdoll();
	virtual bool			GetPhysicsToSoundTransform( idVec3& origin, idMat3& axis );

private:
	int						num_anims;
	int						current_anim_index;
	int						anim;
	int						blendFrames;
	jointHandle_t			soundJoint;
	idEntityPtr<idEntity>	activator;
	bool					activated;
	int						achievement;

	void					PlayNextAnim();

	void					Event_Activate( idEntity* activator );
	void					Event_Start();
	void					Event_StartRagdoll();
	void					Event_AnimDone( int animIndex );
	void					Event_Footstep();
	void					Event_LaunchMissiles( const char* projectilename, const char* sound, const char* launchjoint, const char* targetjoint, int numshots, int framedelay );
	void					Event_LaunchMissilesUpdate( int launchjoint, int targetjoint, int numshots, int framedelay );
	void					Event_SetAnimation( const char* animName );
	void					Event_GetAnimationLength();
};


/*
===============================================================================

  idStaticEntity

===============================================================================
*/

class idStaticEntity : public idEntity
{
public:
	CLASS_PROTOTYPE( idStaticEntity );

	idStaticEntity();

	void				Save( idSaveGame* savefile ) const;
	void				Restore( idRestoreGame* savefile );

	void				Spawn();
	void				ShowEditingDialog();
	virtual void		Hide();
	virtual void		Show();
	void				Fade( const idVec4& to, float fadeTime );
	virtual void		Think();

	virtual void		WriteToSnapshot( idBitMsg& msg ) const;
	virtual void		ReadFromSnapshot( const idBitMsg& msg );

private:
	void				Event_Activate( idEntity* activator );

	int					spawnTime;
	bool				active;
	idVec4				fadeFrom;
	idVec4				fadeTo;
	int					fadeStart;
	int					fadeEnd;
	bool				runGui;
};


/*
===============================================================================

idFuncEmitter

===============================================================================
*/

class idFuncEmitter : public idStaticEntity
{
public:
	CLASS_PROTOTYPE( idFuncEmitter );

	idFuncEmitter();

	void				Save( idSaveGame* savefile ) const;
	void				Restore( idRestoreGame* savefile );

	void				Spawn();
	void				Event_Activate( idEntity* activator );

	virtual void		WriteToSnapshot( idBitMsg& msg ) const;
	virtual void		ReadFromSnapshot( const idBitMsg& msg );

private:
	bool				hidden;

};


/*
===============================================================================

idFuncShootProjectile

===============================================================================
*/

class idFuncShootProjectile : public idStaticEntity
{
public:
	CLASS_PROTOTYPE( idFuncShootProjectile );

	idFuncShootProjectile();

	void						Save( idSaveGame* savefile ) const;
	void						Restore( idRestoreGame* savefile );

	void						Spawn();
	void						Event_Activate( idEntity* activator );

	virtual void				Think();

	virtual void				WriteToSnapshot( idBitMsg& msg ) const;
	virtual void				ReadFromSnapshot( const idBitMsg& msg );

private:
	int							mRespawnDelay;
	int							mRespawnTime;
	float						mShootSpeed;
	idVec3						mShootDir;
	idStr						mEntityDefName;
	idEntityPtr< idEntity >		mLastProjectile;

};


/*
===============================================================================

idFuncSmoke

===============================================================================
*/

class idFuncSmoke : public idEntity
{
public:
	CLASS_PROTOTYPE( idFuncSmoke );

	idFuncSmoke();

	void					Spawn();

	void					Save( idSaveGame* savefile ) const;
	void					Restore( idRestoreGame* savefile );

	virtual void			Think();
	void					Event_Activate( idEntity* activator );

private:
	int						smokeTime;
	const idDeclParticle* 	smoke;
	bool					restart;
};


/*
===============================================================================

idFuncSplat

===============================================================================
*/

class idFuncSplat : public idFuncEmitter
{
public:
	CLASS_PROTOTYPE( idFuncSplat );

	idFuncSplat();

	void				Spawn();

private:
	void				Event_Activate( idEntity* activator );
	void				Event_Splat();
};


/*
===============================================================================

idTextEntity

===============================================================================
*/

class idTextEntity : public idEntity
{
public:
	CLASS_PROTOTYPE( idTextEntity );

	void				Spawn();

	void				Save( idSaveGame* savefile ) const;
	void				Restore( idRestoreGame* savefile );

	virtual void		Think();

private:
	idStr				text;
	bool				playerOriented;
};


/*
===============================================================================

idLocationEntity

===============================================================================
*/

class idLocationEntity : public idEntity
{
public:
	CLASS_PROTOTYPE( idLocationEntity );

	void				Spawn();

	const char* 		GetLocation() const;

private:
};

class idLocationSeparatorEntity : public idEntity
{
public:
	CLASS_PROTOTYPE( idLocationSeparatorEntity );

	void				Spawn();

private:
};

class idVacuumSeparatorEntity : public idEntity
{
public:
	CLASS_PROTOTYPE( idVacuumSeparatorEntity );

	idVacuumSeparatorEntity();

	void				Spawn();

	void				Save( idSaveGame* savefile ) const;
	void				Restore( idRestoreGame* savefile );

	void				Event_Activate( idEntity* activator );

private:
	qhandle_t			portal;
};

class idVacuumEntity : public idEntity
{
public:
	CLASS_PROTOTYPE( idVacuumEntity );

	void				Spawn();

private:
};


/*
===============================================================================

  idBeam

===============================================================================
*/

class idBeam : public idEntity
{
public:
	CLASS_PROTOTYPE( idBeam );

	idBeam();

	void				Spawn();

	void				Save( idSaveGame* savefile ) const;
	void				Restore( idRestoreGame* savefile );

	virtual void		Think();

	void				SetMaster( idBeam* masterbeam );
	void				SetBeamTarget( const idVec3& origin );

	virtual void		Show();

	virtual void		WriteToSnapshot( idBitMsg& msg ) const;
	virtual void		ReadFromSnapshot( const idBitMsg& msg );

private:
	void				Event_MatchTarget();
	void				Event_Activate( idEntity* activator );

	idEntityPtr<idBeam>	target;
	idEntityPtr<idBeam>	master;
};


/*
===============================================================================

  idLiquid

===============================================================================
*/

class idRenderModelLiquid;

class idLiquid : public idEntity
{
public:
	CLASS_PROTOTYPE( idLiquid );

	void				Spawn();

	void				Save( idSaveGame* savefile ) const;
	void				Restore( idRestoreGame* savefile );

private:
	void				Event_Touch( idEntity* other, trace_t* trace );


	idRenderModelLiquid* model;
};


/*
===============================================================================

  idShaking

===============================================================================
*/

class idShaking : public idEntity
{
public:
	CLASS_PROTOTYPE( idShaking );

	idShaking();

	void					Spawn();

	void					Save( idSaveGame* savefile ) const;
	void					Restore( idRestoreGame* savefile );

private:
	idPhysics_Parametric	physicsObj;
	bool					active;

	void					BeginShaking();
	void					Event_Activate( idEntity* activator );
};


/*
===============================================================================

  idEarthQuake

===============================================================================
*/

class idEarthQuake : public idEntity
{
public:
	CLASS_PROTOTYPE( idEarthQuake );

	idEarthQuake();

	void				Spawn();

	void				Save( idSaveGame* savefile ) const;
	void				Restore( idRestoreGame* savefile );

	virtual void		Think();

private:
	int					nextTriggerTime;
	int					shakeStopTime;
	float				wait;
	float				random;
	bool				triggered;
	bool				playerOriented;
	bool				disabled;
	float				shakeTime;

	void				Event_Activate( idEntity* activator );
};


/*
===============================================================================

  idFuncPortal

===============================================================================
*/

class idFuncPortal : public idEntity
{
public:
	CLASS_PROTOTYPE( idFuncPortal );

	idFuncPortal();

	void				Spawn();

	void				Save( idSaveGame* savefile ) const;
	void				Restore( idRestoreGame* savefile );

private:
	qhandle_t			portal;
	bool				state;

	void				Event_Activate( idEntity* activator );
};

/*
===============================================================================

  idFuncAASPortal

===============================================================================
*/

class idFuncAASPortal : public idEntity
{
public:
	CLASS_PROTOTYPE( idFuncAASPortal );

	idFuncAASPortal();

	void				Spawn();

	void				Save( idSaveGame* savefile ) const;
	void				Restore( idRestoreGame* savefile );

private:
	bool				state;

	void				Event_Activate( idEntity* activator );
};

/*
===============================================================================

  idFuncAASObstacle

===============================================================================
*/

class idFuncAASObstacle : public idEntity
{
public:
	CLASS_PROTOTYPE( idFuncAASObstacle );

	idFuncAASObstacle();

	void				Spawn();

	void				Save( idSaveGame* savefile ) const;
	void				Restore( idRestoreGame* savefile );

private:
	bool				state;

	void				Event_Activate( idEntity* activator );
};


/*
===============================================================================

idFuncRadioChatter

===============================================================================
*/

class idFuncRadioChatter : public idEntity
{
public:
	CLASS_PROTOTYPE( idFuncRadioChatter );

	idFuncRadioChatter();

	void				Spawn();

	void				Save( idSaveGame* savefile ) const;
	void				Restore( idRestoreGame* savefile );

private:
	float				time;
	void				Event_Activate( idEntity* activator );
	void				Event_ResetRadioHud( idEntity* activator );
};


/*
===============================================================================

  idPhantomObjects

===============================================================================
*/

class idPhantomObjects : public idEntity
{
public:
	CLASS_PROTOTYPE( idPhantomObjects );

	idPhantomObjects();

	void				Spawn();

	void				Save( idSaveGame* savefile ) const;
	void				Restore( idRestoreGame* savefile );

	virtual void		Think();

private:
	void				Event_Activate( idEntity* activator );
	void				Event_Throw();
	void				Event_ShakeObject( idEntity* object, int starttime );

	int					end_time;
	float				throw_time;
	float				shake_time;
	idVec3				shake_ang;
	float				speed;
	int					min_wait;
	int					max_wait;
	idEntityPtr<idActor>target;
	idList<int>			targetTime;
	idList<idVec3>		lastTargetPos;
};

/*
===============================================================================

idShockwave

===============================================================================
*/
class idShockwave : public idEntity
{
public:
	CLASS_PROTOTYPE( idShockwave );

	idShockwave();
	~idShockwave();

	void				Spawn();
	void				Think();

	void				Save( idSaveGame* savefile ) const;
	void				Restore( idRestoreGame* savefile );

private:
	void				Event_Activate( idEntity* activator );

	bool				isActive;
	int					startTime;
	int					duration;

	float				startSize;
	float				endSize;
	float				currentSize;

	float				magnitude;

	float				height;
	bool				playerDamaged;
	float				playerDamageSize;

};

/*
===============================================================================

idFuncMountedObject

===============================================================================
*/
class idFuncMountedObject : public idEntity
{
public:
	CLASS_PROTOTYPE( idFuncMountedObject );

	idFuncMountedObject();
	~idFuncMountedObject();

	void				Spawn();
	void				Think();

	void				GetAngleRestrictions( int& yaw_min, int& yaw_max, int& pitch );

private:
	int					harc;
	int					varc;

	void				Event_Touch( idEntity* other, trace_t* trace );
	void				Event_Activate( idEntity* activator );

public:
	bool				isMounted;
	function_t*			scriptFunction;
	idPlayer* 			mountedPlayer;
};


class idFuncMountedWeapon : public idFuncMountedObject
{
public:
	CLASS_PROTOTYPE( idFuncMountedWeapon );

	idFuncMountedWeapon();
	~idFuncMountedWeapon();

	void				Spawn();
	void				Think();

private:

	// The actual turret that moves with the player's view
	idEntity*			turret;

	// the muzzle bone's position, used for launching projectiles and trailing smoke
	idVec3				muzzleOrigin;
	idMat3				muzzleAxis;

	float				weaponLastFireTime;
	float				weaponFireDelay;

	const idDict* 		projectile;

	const idSoundShader*	soundFireWeapon;

	void				Event_PostSpawn();
};

/*
===============================================================================

idPortalSky

===============================================================================
*/
class idPortalSky : public idEntity
{
public:
	CLASS_PROTOTYPE( idPortalSky );

	idPortalSky();
	~idPortalSky();

	void				Spawn();
	void				Event_PostSpawn();
	void				Event_Activate( idEntity* activator );
};


#endif /* !__GAME_MISC_H__ */
