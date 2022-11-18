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
#ifndef __ANIM_H__
#define __ANIM_H__

//
// animation channels
// these can be changed by modmakers and licensees to be whatever they need.
const int ANIM_NumAnimChannels		= 5;
const int ANIM_MaxAnimsPerChannel	= 3;
const int ANIM_MaxSyncedAnims		= 3;

//
// animation channels.  make sure to change script/doom_defs.script if you add any channels, or change their order
//
const int ANIMCHANNEL_ALL			= 0;
const int ANIMCHANNEL_TORSO			= 1;
const int ANIMCHANNEL_LEGS			= 2;
const int ANIMCHANNEL_HEAD			= 3;
const int ANIMCHANNEL_EYELIDS		= 4;

// for converting from 24 frames per second to milliseconds
ID_INLINE int FRAME2MS( int framenum )
{
	return ( framenum * 1000 ) / 24;
}

class idRenderModel;
class idAnimator;
class idAnimBlend;
class function_t;
class idEntity;
class idSaveGame;
class idRestoreGame;

typedef struct
{
	int		cycleCount;	// how many times the anim has wrapped to the begining (0 for clamped anims)
	int		frame1;
	int		frame2;
	float	frontlerp;
	float	backlerp;
} frameBlend_t;

typedef struct
{
	int						nameIndex;
	int						parentNum;
	int						animBits;
	int						firstComponent;
} jointAnimInfo_t;

typedef struct
{
	jointHandle_t			num;
	jointHandle_t			parentNum;
	int						channel;
} jointInfo_t;

//
// joint modifier modes.  make sure to change script/doom_defs.script if you add any, or change their order.
//
typedef enum
{
	JOINTMOD_NONE,				// no modification
	JOINTMOD_LOCAL,				// modifies the joint's position or orientation in joint local space
	JOINTMOD_LOCAL_OVERRIDE,	// sets the joint's position or orientation in joint local space
	JOINTMOD_WORLD,				// modifies joint's position or orientation in model space
	JOINTMOD_WORLD_OVERRIDE		// sets the joint's position or orientation in model space
} jointModTransform_t;

typedef struct
{
	jointHandle_t			jointnum;
	idMat3					mat;
	idVec3					pos;
	jointModTransform_t		transform_pos;
	jointModTransform_t		transform_axis;
} jointMod_t;

#define	ANIM_BIT_TX			0
#define	ANIM_BIT_TY			1
#define	ANIM_BIT_TZ			2
#define	ANIM_BIT_QX			3
#define	ANIM_BIT_QY			4
#define	ANIM_BIT_QZ			5

#define	ANIM_TX				BIT( ANIM_BIT_TX )
#define	ANIM_TY				BIT( ANIM_BIT_TY )
#define	ANIM_TZ				BIT( ANIM_BIT_TZ )
#define	ANIM_QX				BIT( ANIM_BIT_QX )
#define	ANIM_QY				BIT( ANIM_BIT_QY )
#define	ANIM_QZ				BIT( ANIM_BIT_QZ )

typedef enum
{
	FC_SCRIPTFUNCTION,
	FC_SCRIPTFUNCTIONOBJECT,
	FC_EVENTFUNCTION,
	FC_SOUND,
	FC_SOUND_VOICE,
	FC_SOUND_VOICE2,
	FC_SOUND_BODY,
	FC_SOUND_BODY2,
	FC_SOUND_BODY3,
	FC_SOUND_WEAPON,
	FC_SOUND_ITEM,
	FC_SOUND_GLOBAL,
	FC_SOUND_CHATTER,
	FC_SKIN,
	FC_TRIGGER,
	FC_TRIGGER_SMOKE_PARTICLE,
	FC_MELEE,
	FC_DIRECTDAMAGE,
	FC_BEGINATTACK,
	FC_ENDATTACK,
	FC_MUZZLEFLASH,
	FC_CREATEMISSILE,
	FC_LAUNCHMISSILE,
	FC_FIREMISSILEATTARGET,
	FC_FOOTSTEP,
	FC_LEFTFOOT,
	FC_RIGHTFOOT,
	FC_ENABLE_EYE_FOCUS,
	FC_DISABLE_EYE_FOCUS,
	FC_FX,
	FC_DISABLE_GRAVITY,
	FC_ENABLE_GRAVITY,
	FC_JUMP,
	FC_ENABLE_CLIP,
	FC_DISABLE_CLIP,
	FC_ENABLE_WALK_IK,
	FC_DISABLE_WALK_IK,
	FC_ENABLE_LEG_IK,
	FC_DISABLE_LEG_IK,
	FC_RECORDDEMO,
	FC_AVIGAME
	, FC_LAUNCH_PROJECTILE,
	FC_TRIGGER_FX,
	FC_START_EMITTER,
	FC_STOP_EMITTER,
} frameCommandType_t;

typedef struct
{
	int						num;
	int						firstCommand;
} frameLookup_t;

typedef struct
{
	frameCommandType_t		type;
	idStr*					string;

	union
	{
		const idSoundShader*	soundShader;
		const function_t*	function;
		const idDeclSkin*	skin;
		int					index;
	};
} frameCommand_t;

typedef struct
{
	bool					prevent_idle_override		: 1;
	bool					random_cycle_start			: 1;
	bool					ai_no_turn					: 1;
	bool					anim_turn					: 1;
} animFlags_t;

/*
==============================================================================================

	idMD5Anim

==============================================================================================
*/

class idMD5Anim
{
private:
	int						numFrames;
	int						frameRate;
	int						animLength;
	int						numJoints;
	int						numAnimatedComponents;
	idList<idBounds, TAG_MD5_ANIM>		bounds;
	idList<jointAnimInfo_t, TAG_MD5_ANIM>	jointInfo;
	idList<idJointQuat, TAG_MD5_ANIM>		baseFrame;
	idList<float, TAG_MD5_ANIM>			componentFrames;
	idStr					name;
	idVec3					totaldelta;
	mutable int				ref_count;
	// RB
	idImportOptions			importOptions;

public:
	idMD5Anim();
	~idMD5Anim();

	void					Free();
	bool					Reload();
	size_t					Allocated() const;
	size_t					Size() const
	{
		return sizeof( *this ) + Allocated();
	};
	bool					LoadAnim( const char* filename, const idImportOptions* options );
	bool					LoadBinary( idFile* file, ID_TIME_T sourceTimeStamp );
	void					WriteBinary( idFile* file, ID_TIME_T sourceTimeStamp );

	void					IncreaseRefs() const;
	void					DecreaseRefs() const;
	int						NumRefs() const;

	void					CheckModelHierarchy( const idRenderModel* model ) const;
	void					GetInterpolatedFrame( frameBlend_t& frame, idJointQuat* joints, const int* index, int numIndexes ) const;
	void					GetSingleFrame( int framenum, idJointQuat* joints, const int* index, int numIndexes ) const;
	int						Length() const;
	int						NumFrames() const;
	int						NumJoints() const;
	const idVec3&			TotalMovementDelta() const;
	const char*				Name() const;

	void					GetFrameBlend( int framenum, frameBlend_t& frame ) const;	// frame 1 is first frame
	void					ConvertTimeToFrame( int time, int cyclecount, frameBlend_t& frame ) const;

	void					GetOrigin( idVec3& offset, int currentTime, int cyclecount ) const;
	void					GetOriginRotation( idQuat& rotation, int time, int cyclecount ) const;
	void					GetBounds( idBounds& bounds, int currentTime, int cyclecount ) const;
};

/*
==============================================================================================

	idAnim

==============================================================================================
*/

class idAnim
{
private:
	const class idDeclModelDef*	modelDef;
	const idMD5Anim*				anims[ ANIM_MaxSyncedAnims ];
	int							numAnims;
	idStr						name;
	idStr						realname;
	idList<frameLookup_t, TAG_ANIM>		frameLookup;
	idList<frameCommand_t, TAG_ANIM>		frameCommands;
	animFlags_t					flags;

public:
	idAnim();
	idAnim( const idDeclModelDef* modelDef, const idAnim* anim );
	~idAnim();

	void						SetAnim( const idDeclModelDef* modelDef, const char* sourcename, const char* animname, int num, const idMD5Anim* md5anims[ ANIM_MaxSyncedAnims ] );
	const char*					Name() const;
	const char*					FullName() const;
	const idMD5Anim*				MD5Anim( int num ) const;
	const idDeclModelDef*		ModelDef() const;
	int							Length() const;
	int							NumFrames() const;
	int							NumAnims() const;
	const idVec3&				TotalMovementDelta() const;
	bool						GetOrigin( idVec3& offset, int animNum, int time, int cyclecount ) const;
	bool						GetOriginRotation( idQuat& rotation, int animNum, int currentTime, int cyclecount ) const;
	bool						GetBounds( idBounds& bounds, int animNum, int time, int cyclecount ) const;
	const char*					AddFrameCommand( const class idDeclModelDef* modelDef, int framenum, idLexer& src, const idDict* def );
	void						CallFrameCommands( idEntity* ent, int from, int to ) const;
	bool						HasFrameCommands() const;

	// returns first frame (zero based) that command occurs.  returns -1 if not found.
	int							FindFrameForFrameCommand( frameCommandType_t framecommand, const frameCommand_t** command ) const;
	void						SetAnimFlags( const animFlags_t& animflags );
	const animFlags_t&			GetAnimFlags() const;
};

/*
==============================================================================================

	idDeclModelDef

==============================================================================================
*/

class idDeclModelDef : public idDecl
{
public:
	idDeclModelDef();
	~idDeclModelDef();

	virtual size_t				Size() const;
	virtual const char* 		DefaultDefinition() const;
	virtual bool				Parse( const char* text, const int textLength, bool allowBinaryVersion );
	virtual void				FreeData();

	void						Touch() const;

	const idDeclSkin* 			GetDefaultSkin() const;
	const idJointQuat* 			GetDefaultPose() const;
	void						SetupJoints( int* numJoints, idJointMat** jointList, idBounds& frameBounds, bool removeOriginOffset ) const;
	idRenderModel* 				ModelHandle() const;
	void						GetJointList( const char* jointnames, idList<jointHandle_t>& jointList ) const;
	const jointInfo_t* 			FindJoint( const char* name ) const;

	int							NumAnims() const;
	const idAnim* 				GetAnim( int index ) const;
	int							GetSpecificAnim( const char* name ) const;
	int							GetAnim( const char* name ) const;
	bool						HasAnim( const char* name ) const;
	const idDeclSkin* 			GetSkin() const;
	const char* 				GetModelName() const;
	const idList<jointInfo_t>& 	Joints() const;
	const int* 					JointParents() const;
	int							NumJoints() const;
	const jointInfo_t* 			GetJoint( int jointHandle ) const;
	const char* 				GetJointName( int jointHandle ) const;
	int							NumJointsOnChannel( int channel ) const;
	const int* 					GetChannelJoints( int channel ) const;

	const idVec3& 				GetVisualOffset() const;

private:
	void						CopyDecl( const idDeclModelDef* decl );
	bool						ParseAnim( idLexer& src, int numDefaultAnims, const idStr& defaultCommands );

private:
	idVec3						offset;
	idList<jointInfo_t, TAG_ANIM>			joints;
	idList<int, TAG_ANIM>					jointParents;
	idList<int, TAG_ANIM>					channelJoints[ ANIM_NumAnimChannels ];
	idRenderModel* 				modelHandle;
	idList<idAnim*, TAG_ANIM>			anims;
	const idDeclSkin* 			skin;
};

/*
==============================================================================================

	idAnimBlend

==============================================================================================
*/

class idAnimBlend
{
private:
	const class idDeclModelDef*	modelDef;
	int							starttime;
	int							endtime;
	int							timeOffset;
	float						rate;

	int							blendStartTime;
	int							blendDuration;
	float						blendStartValue;
	float						blendEndValue;

	float						animWeights[ ANIM_MaxSyncedAnims ];
	short						cycle;
	short						frame;
	short						animNum;
	bool						allowMove;
	bool						allowFrameCommands;

	friend class				idAnimator;

	void						Reset( const idDeclModelDef* _modelDef );
	void						CallFrameCommands( idEntity* ent, int fromtime, int totime ) const;
	void						SetFrame( const idDeclModelDef* modelDef, int animnum, int frame, int currenttime, int blendtime );
	void						CycleAnim( const idDeclModelDef* modelDef, int animnum, int currenttime, int blendtime );
	void						PlayAnim( const idDeclModelDef* modelDef, int animnum, int currenttime, int blendtime );
	bool						BlendAnim( int currentTime, int channel, int numJoints, idJointQuat* blendFrame, float& blendWeight, bool removeOrigin, bool overrideBlend, bool printInfo ) const;
	void						BlendOrigin( int currentTime, idVec3& blendPos, float& blendWeight, bool removeOriginOffset ) const;
	void						BlendDelta( int fromtime, int totime, idVec3& blendDelta, float& blendWeight ) const;
	void						BlendDeltaRotation( int fromtime, int totime, idQuat& blendDelta, float& blendWeight ) const;
	bool						AddBounds( int currentTime, idBounds& bounds, bool removeOriginOffset ) const;

public:
	idAnimBlend();
	void						Save( idSaveGame* savefile ) const;
	void						Restore( idRestoreGame* savefile, const idDeclModelDef* modelDef );
	const char*					AnimName() const;
	const char*					AnimFullName() const;
	float						GetWeight( int currenttime ) const;
	float						GetFinalWeight() const;
	void						SetWeight( float newweight, int currenttime, int blendtime );
	int							NumSyncedAnims() const;
	bool						SetSyncedAnimWeight( int num, float weight );
	void						Clear( int currentTime, int clearTime );
	bool						IsDone( int currentTime ) const;
	bool						FrameHasChanged( int currentTime ) const;
	int							GetCycleCount() const;
	void						SetCycleCount( int count );
	void						SetPlaybackRate( int currentTime, float newRate );
	float						GetPlaybackRate() const;
	void						SetStartTime( int startTime );
	int							GetStartTime() const;
	int							GetEndTime() const;
	int							GetFrameNumber( int currenttime ) const;
	int							AnimTime( int currenttime ) const;
	int							NumFrames() const;
	int							Length() const;
	int							PlayLength() const;
	void						AllowMovement( bool allow );
	void						AllowFrameCommands( bool allow );
	const idAnim*				Anim() const;
	int							AnimNum() const;
};

/*
==============================================================================================

	idAFPoseJointMod

==============================================================================================
*/

typedef enum
{
	AF_JOINTMOD_AXIS,
	AF_JOINTMOD_ORIGIN,
	AF_JOINTMOD_BOTH
} AFJointModType_t;

class idAFPoseJointMod
{
public:
	idAFPoseJointMod();

	AFJointModType_t			mod;
	idMat3						axis;
	idVec3						origin;
};

ID_INLINE idAFPoseJointMod::idAFPoseJointMod()
{
	mod = AF_JOINTMOD_AXIS;
	axis.Identity();
	origin.Zero();
}

/*
==============================================================================================

	idAnimator

==============================================================================================
*/

class idAnimator
{
public:
	idAnimator();
	~idAnimator();

	size_t						Allocated() const;
	size_t						Size() const;

	void						Save( idSaveGame* savefile ) const;					// archives object for save game file
	void						Restore( idRestoreGame* savefile );					// unarchives object from save game file

	void						SetEntity( idEntity* ent );
	idEntity*					GetEntity() const ;
	void						RemoveOriginOffset( bool remove );
	bool						RemoveOrigin() const;

	void						GetJointList( const char* jointnames, idList<jointHandle_t>& jointList ) const;

	int							NumAnims() const;
	const idAnim*				GetAnim( int index ) const;
	int							GetAnim( const char* name ) const;
	bool						HasAnim( const char* name ) const;

	void						ServiceAnims( int fromtime, int totime );
	bool						IsAnimating( int currentTime ) const;

	void						GetJoints( int* numJoints, idJointMat** jointsPtr );
	int							NumJoints() const;
	jointHandle_t				GetFirstChild( jointHandle_t jointnum ) const;
	jointHandle_t				GetFirstChild( const char* name ) const;

	idRenderModel*				SetModel( const char* modelname );
	idRenderModel*				ModelHandle() const;
	const idDeclModelDef*		ModelDef() const;

	void						ForceUpdate();
	void						ClearForceUpdate();
	bool						CreateFrame( int animtime, bool force );
	bool						FrameHasChanged( int animtime ) const;
	void						GetDelta( int fromtime, int totime, idVec3& delta ) const;
	bool						GetDeltaRotation( int fromtime, int totime, idMat3& delta ) const;
	void						GetOrigin( int currentTime, idVec3& pos ) const;
	bool						GetBounds( int currentTime, idBounds& bounds );

	idAnimBlend*					CurrentAnim( int channelNum );
	void						Clear( int channelNum, int currentTime, int cleartime );
	void						SetFrame( int channelNum, int animnum, int frame, int currenttime, int blendtime );
	void						CycleAnim( int channelNum, int animnum, int currenttime, int blendtime );
	void						PlayAnim( int channelNum, int animnum, int currenttime, int blendTime );

	// copies the current anim from fromChannelNum to channelNum.
	// the copied anim will have frame commands disabled to avoid executing them twice.
	void						SyncAnimChannels( int channelNum, int fromChannelNum, int currenttime, int blendTime );

	void						SetJointPos( jointHandle_t jointnum, jointModTransform_t transform_type, const idVec3& pos );
	void						SetJointAxis( jointHandle_t jointnum, jointModTransform_t transform_type, const idMat3& mat );
	void						ClearJoint( jointHandle_t jointnum );
	void						ClearAllJoints();

	void						InitAFPose();
	void						SetAFPoseJointMod( const jointHandle_t jointNum, const AFJointModType_t mod, const idMat3& axis, const idVec3& origin );
	void						FinishAFPose( int animnum, const idBounds& bounds, const int time );
	void						SetAFPoseBlendWeight( float blendWeight );
	bool						BlendAFPose( idJointQuat* blendFrame ) const;
	void						ClearAFPose();

	void						ClearAllAnims( int currentTime, int cleartime );

	jointHandle_t				GetJointHandle( const char* name ) const;
	const char* 				GetJointName( jointHandle_t handle ) const;
	int							GetChannelForJoint( jointHandle_t joint ) const;
	bool						GetJointTransform( jointHandle_t jointHandle, int currenttime, idVec3& offset, idMat3& axis );
	bool						GetJointLocalTransform( jointHandle_t jointHandle, int currentTime, idVec3& offset, idMat3& axis );

	const animFlags_t			GetAnimFlags( int animnum ) const;
	int							NumFrames( int animnum ) const;
	int							NumSyncedAnims( int animnum ) const;
	const char*					AnimName( int animnum ) const;
	const char*					AnimFullName( int animnum ) const;
	int							AnimLength( int animnum ) const;
	const idVec3&				TotalMovementDelta( int animnum ) const;

private:
	void						FreeData();
	void						PushAnims( int channel, int currentTime, int blendTime );

private:
	const idDeclModelDef* 		modelDef;
	idEntity* 					entity;

	idAnimBlend					channels[ ANIM_NumAnimChannels ][ ANIM_MaxAnimsPerChannel ];
	idList<jointMod_t*, TAG_ANIM>		jointMods;
	int							numJoints;
	idJointMat* 				joints;

	mutable int					lastTransformTime;		// mutable because the value is updated in CreateFrame
	mutable bool				stoppedAnimatingUpdate;
	bool						removeOriginOffset;
	bool						forceUpdate;

	idBounds					frameBounds;

	float						AFPoseBlendWeight;
	idList<int, TAG_ANIM>					AFPoseJoints;
	idList<idAFPoseJointMod, TAG_ANIM>	AFPoseJointMods;
	idList<idJointQuat, TAG_ANIM>			AFPoseJointFrame;
	idBounds					AFPoseBounds;
	int							AFPoseTime;
};

/*
==============================================================================================

	idAnimManager

==============================================================================================
*/

class idAnimManager
{
public:
	idAnimManager();
	~idAnimManager();

	static bool					forceExport;

	void						Shutdown();
	idMD5Anim* 					GetAnim( const char* name, const idImportOptions* options ); // RB: added import options
	void						Preload( const idPreloadManifest& manifest );
	void						ReloadAnims();
	void						ListAnims() const;
	int							JointIndex( const char* name );
	const char* 				JointName( int index ) const;

	void						ClearAnimsInUse();
	void						FlushUnusedAnims();

private:
	idHashTable<idMD5Anim*>	animations;
	idStrList					jointnames;
	idHashIndex					jointnamesHash;
};

#endif /* !__ANIM_H__ */
