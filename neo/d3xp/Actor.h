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

#ifndef __GAME_ACTOR_H__
#define __GAME_ACTOR_H__

/*
===============================================================================

	idActor

===============================================================================
*/

extern const idEventDef AI_EnableEyeFocus;
extern const idEventDef AI_DisableEyeFocus;
extern const idEventDef EV_Footstep;
extern const idEventDef EV_FootstepLeft;
extern const idEventDef EV_FootstepRight;
extern const idEventDef EV_EnableWalkIK;
extern const idEventDef EV_DisableWalkIK;
extern const idEventDef EV_EnableLegIK;
extern const idEventDef EV_DisableLegIK;
extern const idEventDef AI_SetAnimPrefix;
extern const idEventDef AI_PlayAnim;
extern const idEventDef AI_PlayCycle;
extern const idEventDef AI_AnimDone;
extern const idEventDef AI_SetBlendFrames;
extern const idEventDef AI_GetBlendFrames;

extern const idEventDef AI_SetState;

class idDeclParticle;

class idAnimState
{
public:
	bool					idleAnim;
	idStr					state;
	int						animBlendFrames;
	int						lastAnimBlendFrames;		// allows override anims to blend based on the last transition time

public:
	idAnimState();
	~idAnimState();

	void					Save( idSaveGame* savefile ) const;
	void					Restore( idRestoreGame* savefile );

	void					Init( idActor* owner, idAnimator* _animator, int animchannel );
	void					Shutdown();
	void					SetState( const char* name, int blendFrames );
	void					StopAnim( int frames );
	void					PlayAnim( int anim );
	void					CycleAnim( int anim );
	void					BecomeIdle();
	bool					UpdateState();
	bool					Disabled() const;
	void					Enable( int blendFrames );
	void					Disable();
	bool					AnimDone( int blendFrames ) const;
	bool					IsIdle() const;
	animFlags_t				GetAnimFlags() const;

private:
	idActor* 				self;
	idAnimator* 			animator;
	idThread* 				thread;
	int						channel;
	bool					disabled;
};

class idAttachInfo
{
public:
	idEntityPtr<idEntity>	ent;
	int						channel;
};

typedef struct
{
	jointModTransform_t		mod;
	jointHandle_t			from;
	jointHandle_t			to;
} copyJoints_t;

class idActor : public idAFEntity_Gibbable
{
public:
	CLASS_PROTOTYPE( idActor );

	int						team;
	int						rank;				// monsters don't fight back if the attacker's rank is higher
	idMat3					viewAxis;			// view axis of the actor

	idLinkList<idActor>		enemyNode;			// node linked into an entity's enemy list for quick lookups of who is attacking him
	idLinkList<idActor>		enemyList;			// list of characters that have targeted the player as their enemy

public:
	idActor();
	virtual					~idActor();

	void					Spawn();
	virtual void			Restart();

	void					Save( idSaveGame* savefile ) const;
	void					Restore( idRestoreGame* savefile );

	virtual void			Hide();
	virtual void			Show();
	virtual int				GetDefaultSurfaceType() const;
	virtual void			ProjectOverlay( const idVec3& origin, const idVec3& dir, float size, const char* material );

	virtual bool			LoadAF();
	void					SetupBody();

	void					CheckBlink();

	virtual bool			GetPhysicsToVisualTransform( idVec3& origin, idMat3& axis );
	virtual bool			GetPhysicsToSoundTransform( idVec3& origin, idMat3& axis );

	// script state management
	void					ShutdownThreads();
	virtual bool			ShouldConstructScriptObjectAtSpawn() const;
	virtual idThread* 		ConstructScriptObject();
	void					UpdateScript();
	const function_t*		GetScriptFunction( const char* funcname );
	void					SetState( const function_t* newState );
	void					SetState( const char* statename );

	// vision testing
	void					SetEyeHeight( float height );
	float					EyeHeight() const;
	idVec3					EyeOffset() const;
	idVec3					GetEyePosition() const;
	virtual void			GetViewPos( idVec3& origin, idMat3& axis ) const;
	void					SetFOV( float fov );
	bool					CheckFOV( const idVec3& pos ) const;
	bool					CanSee( idEntity* ent, bool useFOV ) const;
	bool					PointVisible( const idVec3& point ) const;
	virtual void			GetAIAimTargets( const idVec3& lastSightPos, idVec3& headPos, idVec3& chestPos );

	// damage
	void					SetupDamageGroups();
	virtual	void			Damage( idEntity* inflictor, idEntity* attacker, const idVec3& dir, const char* damageDefName, const float damageScale, const int location );
	int						GetDamageForLocation( int damage, int location );
	const char* 			GetDamageGroup( int location );
	void					ClearPain();
	virtual bool			Pain( idEntity* inflictor, idEntity* attacker, int damage, const idVec3& dir, int location );

	// model/combat model/ragdoll
	void					SetCombatModel();
	idClipModel* 			GetCombatModel() const;
	virtual void			LinkCombat();
	virtual void			UnlinkCombat();
	bool					StartRagdoll();
	void					StopRagdoll();
	virtual bool			UpdateAnimationControllers();

	// delta view angles to allow movers to rotate the view of the actor
	const idAngles& 		GetDeltaViewAngles() const;
	void					SetDeltaViewAngles( const idAngles& delta );

	bool					HasEnemies() const;
	idActor* 				ClosestEnemyToPoint( const idVec3& pos );
	idActor* 				EnemyWithMostHealth();

	virtual bool			OnLadder() const;

	virtual void			GetAASLocation( idAAS* aas, idVec3& pos, int& areaNum ) const;

	void					Attach( idEntity* ent );

	virtual void			Teleport( const idVec3& origin, const idAngles& angles, idEntity* destination );

	virtual	renderView_t* 	GetRenderView();

	// animation state control
	int						GetAnim( int channel, const char* name );
	void					UpdateAnimState();
	void					SetAnimState( int channel, const char* name, int blendFrames );
	const char* 			GetAnimState( int channel ) const;
	bool					InAnimState( int channel, const char* name ) const;
	const char* 			WaitState() const;
	void					SetWaitState( const char* _waitstate );
	bool					AnimDone( int channel, int blendFrames ) const;
	virtual void			SpawnGibs( const idVec3& dir, const char* damageDefName );

	idEntity*				GetHeadEntity()
	{
		return head.GetEntity();
	};

protected:
	friend class			idAnimState;

	float					fovDot;				// cos( fovDegrees )
	idVec3					eyeOffset;			// offset of eye relative to physics origin
	idVec3					modelOffset;		// offset of visual model relative to the physics origin

	idAngles				deltaViewAngles;	// delta angles relative to view input angles

	int						pain_debounce_time;	// next time the actor can show pain
	int						pain_delay;			// time between playing pain sound
	int						pain_threshold;		// how much damage monster can take at any one time before playing pain animation

	idStrList				damageGroups;		// body damage groups
	idList<float, TAG_ACTOR>			damageScale;		// damage scale per damage gruop

	bool						use_combat_bbox;	// whether to use the bounding box for combat collision
	idEntityPtr<idAFAttachment>	head;
	idList<copyJoints_t, TAG_ACTOR>		copyJoints;			// copied from the body animation to the head model

	// state variables
	const function_t*		state;
	const function_t*		idealState;

	// joint handles
	jointHandle_t			leftEyeJoint;
	jointHandle_t			rightEyeJoint;
	jointHandle_t			soundJoint;

	idIK_Walk				walkIK;

	idStr					animPrefix;
	idStr					painAnim;

	// blinking
	int						blink_anim;
	int						blink_time;
	int						blink_min;
	int						blink_max;

	// script variables
	idThread* 				scriptThread;
	idStr					waitState;
	idAnimState				headAnim;
	idAnimState				torsoAnim;
	idAnimState				legsAnim;

	bool					allowPain;
	bool					allowEyeFocus;
	bool					finalBoss;

	int						painTime;
	bool					damageNotByFists;

	idList<idAttachInfo, TAG_ACTOR>	attachments;

	int						damageCap;

	virtual void			Gib( const idVec3& dir, const char* damageDefName );

	// removes attachments with "remove" set for when character dies
	void					RemoveAttachments();

	// copies animation from body to head joints
	void					CopyJointsFromBodyToHead();

private:
	void					SyncAnimChannels( int channel, int syncToChannel, int blendFrames );
	void					FinishSetup();
	void					SetupHead();
	void					PlayFootStepSound();

	void					Event_EnableEyeFocus();
	void					Event_DisableEyeFocus();
	void					Event_Footstep();
	void					Event_EnableWalkIK();
	void					Event_DisableWalkIK();
	void					Event_EnableLegIK( int num );
	void					Event_DisableLegIK( int num );
	void					Event_SetAnimPrefix( const char* name );
	void					Event_LookAtEntity( idEntity* ent, float duration );
	void					Event_PreventPain( float duration );
	void					Event_DisablePain();
	void					Event_EnablePain();
	void					Event_GetPainAnim();
	void					Event_StopAnim( int channel, int frames );
	void					Event_PlayAnim( int channel, const char* name );
	void					Event_PlayCycle( int channel, const char* name );
	void					Event_IdleAnim( int channel, const char* name );
	void					Event_SetSyncedAnimWeight( int channel, int anim, float weight );
	void					Event_OverrideAnim( int channel );
	void					Event_EnableAnim( int channel, int blendFrames );
	void					Event_SetBlendFrames( int channel, int blendFrames );
	void					Event_GetBlendFrames( int channel );
	void					Event_AnimState( int channel, const char* name, int blendFrames );
	void					Event_GetAnimState( int channel );
	void					Event_InAnimState( int channel, const char* name );
	void					Event_FinishAction( const char* name );
	void					Event_AnimDone( int channel, int blendFrames );
	void					Event_HasAnim( int channel, const char* name );
	void					Event_CheckAnim( int channel, const char* animname );
	void					Event_ChooseAnim( int channel, const char* animname );
	void					Event_AnimLength( int channel, const char* animname );
	void					Event_AnimDistance( int channel, const char* animname );
	void					Event_HasEnemies();
	void					Event_NextEnemy( idEntity* ent );
	void					Event_ClosestEnemyToPoint( const idVec3& pos );
	void					Event_StopSound( int channel, int netsync );
	void					Event_SetNextState( const char* name );
	void					Event_SetState( const char* name );
	void					Event_GetState();
	void					Event_GetHead();
	void					Event_SetDamageGroupScale( const char* groupName, float scale );
	void					Event_SetDamageGroupScaleAll( float scale );
	void					Event_GetDamageGroupScale( const char* groupName );
	void					Event_SetDamageCap( float _damageCap );
	void					Event_SetWaitState( const char* waitState );
	void					Event_GetWaitState();

};

#endif /* !__GAME_ACTOR_H__ */
