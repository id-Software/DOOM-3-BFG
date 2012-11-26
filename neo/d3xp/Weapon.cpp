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

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"
#include "PredictedValue_impl.h"

/***********************************************************************

  idWeapon  
	
***********************************************************************/

//
// event defs
//
const idEventDef EV_Weapon_Clear( "<clear>" );
const idEventDef EV_Weapon_GetOwner( "getOwner", NULL, 'e' );
const idEventDef EV_Weapon_Next( "nextWeapon" );
const idEventDef EV_Weapon_State( "weaponState", "sd" );
const idEventDef EV_Weapon_UseAmmo( "useAmmo", "d" );
const idEventDef EV_Weapon_AddToClip( "addToClip", "d" );
const idEventDef EV_Weapon_AmmoInClip( "ammoInClip", NULL, 'f' );
const idEventDef EV_Weapon_AmmoAvailable( "ammoAvailable", NULL, 'f' );
const idEventDef EV_Weapon_TotalAmmoCount( "totalAmmoCount", NULL, 'f' );
const idEventDef EV_Weapon_ClipSize( "clipSize", NULL, 'f' );
const idEventDef EV_Weapon_WeaponOutOfAmmo( "weaponOutOfAmmo" );
const idEventDef EV_Weapon_WeaponReady( "weaponReady" );
const idEventDef EV_Weapon_WeaponReloading( "weaponReloading" );
const idEventDef EV_Weapon_WeaponHolstered( "weaponHolstered" );
const idEventDef EV_Weapon_WeaponRising( "weaponRising" );
const idEventDef EV_Weapon_WeaponLowering( "weaponLowering" );
const idEventDef EV_Weapon_Flashlight( "flashlight", "d" );
const idEventDef EV_Weapon_LaunchProjectiles( "launchProjectiles", "dffff" );
const idEventDef EV_Weapon_CreateProjectile( "createProjectile", NULL, 'e' );
const idEventDef EV_Weapon_EjectBrass( "ejectBrass" );
const idEventDef EV_Weapon_Melee( "melee", NULL, 'd' );
const idEventDef EV_Weapon_GetWorldModel( "getWorldModel", NULL, 'e' );
const idEventDef EV_Weapon_AllowDrop( "allowDrop", "d" );
const idEventDef EV_Weapon_AutoReload( "autoReload", NULL, 'f' );
const idEventDef EV_Weapon_NetReload( "netReload" );
const idEventDef EV_Weapon_IsInvisible( "isInvisible", NULL, 'f' );
const idEventDef EV_Weapon_NetEndReload( "netEndReload" );
const idEventDef EV_Weapon_GrabberHasTarget( "grabberHasTarget", NULL, 'd' );
const idEventDef EV_Weapon_Grabber( "grabber", "d" );
const idEventDef EV_Weapon_Grabber_SetGrabDistance( "grabberGrabDistance", "f" );
const idEventDef EV_Weapon_LaunchProjectilesEllipse( "launchProjectilesEllipse", "dffff" );
const idEventDef EV_Weapon_LaunchPowerup( "launchPowerup", "sfd" );
const idEventDef EV_Weapon_StartWeaponSmoke( "startWeaponSmoke" );
const idEventDef EV_Weapon_StopWeaponSmoke( "stopWeaponSmoke" );
const idEventDef EV_Weapon_StartWeaponParticle( "startWeaponParticle", "s" );
const idEventDef EV_Weapon_StopWeaponParticle( "stopWeaponParticle", "s" );
const idEventDef EV_Weapon_StartWeaponLight( "startWeaponLight", "s" );
const idEventDef EV_Weapon_StopWeaponLight( "stopWeaponLight", "s" );

//
// class def
//
CLASS_DECLARATION( idAnimatedEntity, idWeapon )
	EVENT( EV_Weapon_Clear,						idWeapon::Event_Clear )
	EVENT( EV_Weapon_GetOwner,					idWeapon::Event_GetOwner )
	EVENT( EV_Weapon_State,						idWeapon::Event_WeaponState )
	EVENT( EV_Weapon_WeaponReady,				idWeapon::Event_WeaponReady )
	EVENT( EV_Weapon_WeaponOutOfAmmo,			idWeapon::Event_WeaponOutOfAmmo )
	EVENT( EV_Weapon_WeaponReloading,			idWeapon::Event_WeaponReloading )
	EVENT( EV_Weapon_WeaponHolstered,			idWeapon::Event_WeaponHolstered )
	EVENT( EV_Weapon_WeaponRising,				idWeapon::Event_WeaponRising )
	EVENT( EV_Weapon_WeaponLowering,			idWeapon::Event_WeaponLowering )
	EVENT( EV_Weapon_UseAmmo,					idWeapon::Event_UseAmmo )
	EVENT( EV_Weapon_AddToClip,					idWeapon::Event_AddToClip )
	EVENT( EV_Weapon_AmmoInClip,				idWeapon::Event_AmmoInClip )
	EVENT( EV_Weapon_AmmoAvailable,				idWeapon::Event_AmmoAvailable )
	EVENT( EV_Weapon_TotalAmmoCount,			idWeapon::Event_TotalAmmoCount )
	EVENT( EV_Weapon_ClipSize,					idWeapon::Event_ClipSize )
	EVENT( AI_PlayAnim,							idWeapon::Event_PlayAnim )
	EVENT( AI_PlayCycle,						idWeapon::Event_PlayCycle )
	EVENT( AI_SetBlendFrames,					idWeapon::Event_SetBlendFrames )
	EVENT( AI_GetBlendFrames,					idWeapon::Event_GetBlendFrames )
	EVENT( AI_AnimDone,							idWeapon::Event_AnimDone )
	EVENT( EV_Weapon_Next,						idWeapon::Event_Next )
	EVENT( EV_SetSkin,							idWeapon::Event_SetSkin )
	EVENT( EV_Weapon_Flashlight,				idWeapon::Event_Flashlight )
	EVENT( EV_Light_GetLightParm,				idWeapon::Event_GetLightParm )
	EVENT( EV_Light_SetLightParm,				idWeapon::Event_SetLightParm )
	EVENT( EV_Light_SetLightParms,				idWeapon::Event_SetLightParms )
	EVENT( EV_Weapon_LaunchProjectiles,			idWeapon::Event_LaunchProjectiles )
	EVENT( EV_Weapon_CreateProjectile,			idWeapon::Event_CreateProjectile )
	EVENT( EV_Weapon_EjectBrass,				idWeapon::Event_EjectBrass )
	EVENT( EV_Weapon_Melee,						idWeapon::Event_Melee )
	EVENT( EV_Weapon_GetWorldModel,				idWeapon::Event_GetWorldModel )
	EVENT( EV_Weapon_AllowDrop,					idWeapon::Event_AllowDrop )
	EVENT( EV_Weapon_AutoReload,				idWeapon::Event_AutoReload )
	EVENT( EV_Weapon_NetReload,					idWeapon::Event_NetReload )
	EVENT( EV_Weapon_IsInvisible,				idWeapon::Event_IsInvisible )
	EVENT( EV_Weapon_NetEndReload,				idWeapon::Event_NetEndReload )
	EVENT( EV_Weapon_Grabber,					idWeapon::Event_Grabber )
	EVENT( EV_Weapon_GrabberHasTarget,			idWeapon::Event_GrabberHasTarget )
	EVENT( EV_Weapon_Grabber_SetGrabDistance,	idWeapon::Event_GrabberSetGrabDistance )
	EVENT( EV_Weapon_LaunchProjectilesEllipse,	idWeapon::Event_LaunchProjectilesEllipse )
	EVENT( EV_Weapon_LaunchPowerup,				idWeapon::Event_LaunchPowerup )
	EVENT( EV_Weapon_StartWeaponSmoke,			idWeapon::Event_StartWeaponSmoke )
	EVENT( EV_Weapon_StopWeaponSmoke,			idWeapon::Event_StopWeaponSmoke )
	EVENT( EV_Weapon_StartWeaponParticle,		idWeapon::Event_StartWeaponParticle )
	EVENT( EV_Weapon_StopWeaponParticle,		idWeapon::Event_StopWeaponParticle )
	EVENT( EV_Weapon_StartWeaponLight,			idWeapon::Event_StartWeaponLight )
	EVENT( EV_Weapon_StopWeaponLight,			idWeapon::Event_StopWeaponLight )
END_CLASS


idCVar cg_projectile_clientAuthoritative_maxCatchup( "cg_projectile_clientAuthoritative_maxCatchup", "500", CVAR_INTEGER, "" );

idCVar g_useWeaponDepthHack( "g_useWeaponDepthHack", "1", CVAR_BOOL, "Crunch z depth on weapons" );

idCVar g_weaponShadows( "g_weaponShadows", "0", CVAR_BOOL | CVAR_ARCHIVE, "Cast shadows from weapons" );

extern idCVar cg_predictedSpawn_debug;

/***********************************************************************

	init

***********************************************************************/

/*
================
idWeapon::idWeapon()
================
*/
idWeapon::idWeapon() {
	owner					= NULL;
	worldModel				= NULL;
	weaponDef				= NULL;
	thread					= NULL;

	memset( &guiLight, 0, sizeof( guiLight ) );
	memset( &muzzleFlash, 0, sizeof( muzzleFlash ) );
	memset( &worldMuzzleFlash, 0, sizeof( worldMuzzleFlash ) );
	memset( &nozzleGlow, 0, sizeof( nozzleGlow ) );

	muzzleFlashEnd			= 0;
	flashColor				= vec3_origin;
	muzzleFlashHandle		= -1;
	worldMuzzleFlashHandle	= -1;
	guiLightHandle			= -1;
	nozzleGlowHandle		= -1;
	modelDefHandle			= -1;
	grabberState			= -1;

	berserk					= 2;
	brassDelay				= 0;

	allowDrop				= true;
	isPlayerFlashlight		= false;

	fraccos = 0.0f;
	fraccos2 = 0.0f;

	Clear();

	fl.networkSync = true;
}

/*
================
idWeapon::~idWeapon()
================
*/
idWeapon::~idWeapon() {
	Clear();
	delete worldModel.GetEntity();
}


/*
================
idWeapon::Spawn
================
*/
void idWeapon::Spawn() {
	if ( !common->IsClient() ) {
		// setup the world model
		worldModel = static_cast< idAnimatedEntity * >( gameLocal.SpawnEntityType( idAnimatedEntity::Type, NULL ) );
		worldModel.GetEntity()->fl.networkSync = true;
	}

	if ( 1 /*!common->IsMultiplayer()*/ ) {
		grabber.Initialize();
	}

	thread = new idThread();
	thread->ManualDelete();
	thread->ManualControl();
}

/*
================
idWeapon::SetOwner

Only called at player spawn time, not each weapon switch
================
*/
void idWeapon::SetOwner( idPlayer *_owner ) {
	assert( !owner );
	owner = _owner;
	SetName( va( "%s_weapon", owner->name.c_str() ) );

	if ( worldModel.GetEntity() ) {
		worldModel.GetEntity()->SetName( va( "%s_weapon_worldmodel", owner->name.c_str() ) );
	}
}

/*
================
idWeapon::SetFlashlightOwner

Only called at player spawn time, not each weapon switch
================
*/
void idWeapon::SetFlashlightOwner( idPlayer *_owner ) {
	assert( !owner );
	owner = _owner;
	SetName( va( "%s_weapon_flashlight", owner->name.c_str() ) );

	if ( worldModel.GetEntity() ) {
		worldModel.GetEntity()->SetName( va( "%s_weapon_flashlight_worldmodel", owner->name.c_str() ) );
	}
}

/*
================
idWeapon::ShouldConstructScriptObjectAtSpawn

Called during idEntity::Spawn to see if it should construct the script object or not.
Overridden by subclasses that need to spawn the script object themselves.
================
*/
bool idWeapon::ShouldConstructScriptObjectAtSpawn() const {
	return false;
}

/*
================
idWeapon::CacheWeapon
================
*/
void idWeapon::CacheWeapon( const char *weaponName ) {
	const idDeclEntityDef *weaponDef;
	const char *brassDefName;
	const char *clipModelName;
	idTraceModel trm;
	const char *guiName;

	weaponDef = gameLocal.FindEntityDef( weaponName, false );
	if ( !weaponDef ) {
		return;
	}

	// precache the brass collision model
	brassDefName = weaponDef->dict.GetString( "def_ejectBrass" );
	if ( brassDefName[0] ) {
		const idDeclEntityDef *brassDef = gameLocal.FindEntityDef( brassDefName, false );
		if ( brassDef ) {
			brassDef->dict.GetString( "clipmodel", "", &clipModelName );
			if ( !clipModelName[0] ) {
				clipModelName = brassDef->dict.GetString( "model" );		// use the visual model
			}
			// load the trace model
			collisionModelManager->TrmFromModel( clipModelName, trm );
		}
	}

	guiName = weaponDef->dict.GetString( "gui" );
	if ( guiName[0] ) {
		uiManager->FindGui( guiName, true, false, true );
	}
}

/*
================
idWeapon::Save
================
*/
void idWeapon::Save( idSaveGame *savefile ) const {

	savefile->WriteInt( status );
	savefile->WriteObject( thread );
	savefile->WriteString( state );
	savefile->WriteString( idealState );
	savefile->WriteInt( animBlendFrames );
	savefile->WriteInt( animDoneTime );
	savefile->WriteBool( isLinked );

	savefile->WriteObject( owner );
	worldModel.Save( savefile );

	savefile->WriteInt( hideTime );
	savefile->WriteFloat( hideDistance );
	savefile->WriteInt( hideStartTime );
	savefile->WriteFloat( hideStart );
	savefile->WriteFloat( hideEnd );
	savefile->WriteFloat( hideOffset );
	savefile->WriteBool( hide );
	savefile->WriteBool( disabled );

	savefile->WriteInt( berserk );

	savefile->WriteVec3( playerViewOrigin );
	savefile->WriteMat3( playerViewAxis );

	savefile->WriteVec3( viewWeaponOrigin );
	savefile->WriteMat3( viewWeaponAxis );

	savefile->WriteVec3( muzzleOrigin );
	savefile->WriteMat3( muzzleAxis );

	savefile->WriteVec3( pushVelocity );

	savefile->WriteString( weaponDef->GetName() );
	savefile->WriteFloat( meleeDistance );
	savefile->WriteString( meleeDefName );
	savefile->WriteInt( brassDelay );
	savefile->WriteString( icon );
	savefile->WriteString( pdaIcon );
	savefile->WriteString( displayName );
	savefile->WriteString( itemDesc );

	savefile->WriteInt( guiLightHandle );
	savefile->WriteRenderLight( guiLight );

	savefile->WriteInt( muzzleFlashHandle );
	savefile->WriteRenderLight( muzzleFlash );

	savefile->WriteInt( worldMuzzleFlashHandle );
	savefile->WriteRenderLight( worldMuzzleFlash );

	savefile->WriteVec3( flashColor );
	savefile->WriteInt( muzzleFlashEnd );
	savefile->WriteInt( flashTime );

	savefile->WriteBool( lightOn );
	savefile->WriteBool( silent_fire );

	savefile->WriteInt( kick_endtime );
	savefile->WriteInt( muzzle_kick_time );
	savefile->WriteInt( muzzle_kick_maxtime );
	savefile->WriteAngles( muzzle_kick_angles );
	savefile->WriteVec3( muzzle_kick_offset );

	savefile->WriteInt( ammoType );
	savefile->WriteInt( ammoRequired );
	savefile->WriteInt( clipSize );
	savefile->WriteInt( ammoClip.Get() );
	savefile->WriteInt( lowAmmo );
	savefile->WriteBool( powerAmmo );

	// savegames <= 17
	savefile->WriteInt( 0 );

	savefile->WriteInt( zoomFov );

	savefile->WriteJoint( barrelJointView );
	savefile->WriteJoint( flashJointView );
	savefile->WriteJoint( ejectJointView );
	savefile->WriteJoint( guiLightJointView );
	savefile->WriteJoint( ventLightJointView );

	savefile->WriteJoint( flashJointWorld );
	savefile->WriteJoint( barrelJointWorld );
	savefile->WriteJoint( ejectJointWorld );

	savefile->WriteBool( hasBloodSplat );

	savefile->WriteSoundShader( sndHum );

	savefile->WriteParticle( weaponSmoke );
	savefile->WriteInt( weaponSmokeStartTime );
	savefile->WriteBool( continuousSmoke );
	savefile->WriteParticle( strikeSmoke );
	savefile->WriteInt( strikeSmokeStartTime );
	savefile->WriteVec3( strikePos );
	savefile->WriteMat3( strikeAxis );
	savefile->WriteInt( nextStrikeFx );

	savefile->WriteBool( nozzleFx );
	savefile->WriteInt( nozzleFxFade );

	savefile->WriteInt( lastAttack );

	savefile->WriteInt( nozzleGlowHandle );
	savefile->WriteRenderLight( nozzleGlow );

	savefile->WriteVec3( nozzleGlowColor );
	savefile->WriteMaterial( nozzleGlowShader );
	savefile->WriteFloat( nozzleGlowRadius );

	savefile->WriteInt( weaponAngleOffsetAverages );
	savefile->WriteFloat( weaponAngleOffsetScale );
	savefile->WriteFloat( weaponAngleOffsetMax );
	savefile->WriteFloat( weaponOffsetTime );
	savefile->WriteFloat( weaponOffsetScale );

	savefile->WriteBool( allowDrop );
	savefile->WriteObject( projectileEnt );

	savefile->WriteStaticObject( grabber );
	savefile->WriteInt( grabberState );

	savefile->WriteJoint ( smokeJointView );

	savefile->WriteInt(weaponParticles.Num());
	for(int i = 0; i < weaponParticles.Num(); i++) {
		WeaponParticle_t* part = weaponParticles.GetIndex(i);
		savefile->WriteString( part->name );
		savefile->WriteString( part->particlename );
		savefile->WriteBool( part->active );
		savefile->WriteInt( part->startTime );
		savefile->WriteJoint( part->joint );
		savefile->WriteBool( part->smoke );
		if(!part->smoke) {
			savefile->WriteObject(part->emitter);
		}
	}
	savefile->WriteInt(weaponLights.Num());
	for(int i = 0; i < weaponLights.Num(); i++) {
		WeaponLight_t* light = weaponLights.GetIndex(i);
		savefile->WriteString( light->name );
		savefile->WriteBool( light->active );
		savefile->WriteInt( light->startTime );
		savefile->WriteJoint( light->joint );
		savefile->WriteInt( light->lightHandle );
		savefile->WriteRenderLight( light->light );
	}

}

/*
================
idWeapon::Restore
================
*/
void idWeapon::Restore( idRestoreGame *savefile ) {

	savefile->ReadInt( (int &)status );
	savefile->ReadObject( reinterpret_cast<idClass *&>( thread ) );
	savefile->ReadString( state );
	savefile->ReadString( idealState );
	savefile->ReadInt( animBlendFrames );
	savefile->ReadInt( animDoneTime );
	savefile->ReadBool( isLinked );

	// Re-link script fields
	WEAPON_ATTACK.LinkTo(		scriptObject, "WEAPON_ATTACK" );
	WEAPON_RELOAD.LinkTo(		scriptObject, "WEAPON_RELOAD" );
	WEAPON_NETRELOAD.LinkTo(	scriptObject, "WEAPON_NETRELOAD" );
	WEAPON_NETENDRELOAD.LinkTo(	scriptObject, "WEAPON_NETENDRELOAD" );
	WEAPON_NETFIRING.LinkTo(	scriptObject, "WEAPON_NETFIRING" );
	WEAPON_RAISEWEAPON.LinkTo(	scriptObject, "WEAPON_RAISEWEAPON" );
	WEAPON_LOWERWEAPON.LinkTo(	scriptObject, "WEAPON_LOWERWEAPON" );

	savefile->ReadObject( reinterpret_cast<idClass *&>( owner ) );
	worldModel.Restore( savefile );

	savefile->ReadInt( hideTime );
	savefile->ReadFloat( hideDistance );
	savefile->ReadInt( hideStartTime );
	savefile->ReadFloat( hideStart );
	savefile->ReadFloat( hideEnd );
	savefile->ReadFloat( hideOffset );
	savefile->ReadBool( hide );
	savefile->ReadBool( disabled );

	savefile->ReadInt( berserk );

	savefile->ReadVec3( playerViewOrigin );
	savefile->ReadMat3( playerViewAxis );

	savefile->ReadVec3( viewWeaponOrigin );
	savefile->ReadMat3( viewWeaponAxis );

	savefile->ReadVec3( muzzleOrigin );
	savefile->ReadMat3( muzzleAxis );

	savefile->ReadVec3( pushVelocity );

	idStr objectname;
	savefile->ReadString( objectname );
	weaponDef = gameLocal.FindEntityDef( objectname );
	meleeDef = gameLocal.FindEntityDef( weaponDef->dict.GetString( "def_melee" ), false );

	const idDeclEntityDef *projectileDef = gameLocal.FindEntityDef( weaponDef->dict.GetString( "def_projectile" ), false );
	if ( projectileDef ) {
		projectileDict = projectileDef->dict;
	} else {
		projectileDict.Clear();
	}

	const idDeclEntityDef *brassDef = gameLocal.FindEntityDef( weaponDef->dict.GetString( "def_ejectBrass" ), false );
	if ( brassDef ) {
		brassDict = brassDef->dict;
	} else {
		brassDict.Clear();
	}

	savefile->ReadFloat( meleeDistance );
	savefile->ReadString( meleeDefName );
	savefile->ReadInt( brassDelay );
	savefile->ReadString( icon );
	savefile->ReadString( pdaIcon );
	savefile->ReadString( displayName );
	savefile->ReadString( itemDesc );

	savefile->ReadInt( guiLightHandle );
	savefile->ReadRenderLight( guiLight );
	if ( guiLightHandle >= 0 ) {
		guiLightHandle = gameRenderWorld->AddLightDef( &guiLight );
	}

	savefile->ReadInt( muzzleFlashHandle );
	savefile->ReadRenderLight( muzzleFlash );
	if ( muzzleFlashHandle >= 0 ) {
		muzzleFlashHandle = gameRenderWorld->AddLightDef( &muzzleFlash );
	}

	savefile->ReadInt( worldMuzzleFlashHandle );
	savefile->ReadRenderLight( worldMuzzleFlash );
	if ( worldMuzzleFlashHandle >= 0 ) {
		worldMuzzleFlashHandle = gameRenderWorld->AddLightDef( &worldMuzzleFlash );
	}

	savefile->ReadVec3( flashColor );
	savefile->ReadInt( muzzleFlashEnd );
	savefile->ReadInt( flashTime );

	savefile->ReadBool( lightOn );
	savefile->ReadBool( silent_fire );

	savefile->ReadInt( kick_endtime );
	savefile->ReadInt( muzzle_kick_time );
	savefile->ReadInt( muzzle_kick_maxtime );
	savefile->ReadAngles( muzzle_kick_angles );
	savefile->ReadVec3( muzzle_kick_offset );

	savefile->ReadInt( (int &)ammoType );
	savefile->ReadInt( ammoRequired );
	savefile->ReadInt( clipSize );
	
	int savedAmmoClip = 0;
	savefile->ReadInt( savedAmmoClip );
	ammoClip = savedAmmoClip;

	savefile->ReadInt( lowAmmo );
	savefile->ReadBool( powerAmmo );

	// savegame versions <= 17
	int foo;
	savefile->ReadInt( foo );

	savefile->ReadInt( zoomFov );

	savefile->ReadJoint( barrelJointView );
	savefile->ReadJoint( flashJointView );
	savefile->ReadJoint( ejectJointView );
	savefile->ReadJoint( guiLightJointView );
	savefile->ReadJoint( ventLightJointView );

	savefile->ReadJoint( flashJointWorld );
	savefile->ReadJoint( barrelJointWorld );
	savefile->ReadJoint( ejectJointWorld );

	savefile->ReadBool( hasBloodSplat );

	savefile->ReadSoundShader( sndHum );

	savefile->ReadParticle( weaponSmoke );
	savefile->ReadInt( weaponSmokeStartTime );
	savefile->ReadBool( continuousSmoke );
	savefile->ReadParticle( strikeSmoke );
	savefile->ReadInt( strikeSmokeStartTime );
	savefile->ReadVec3( strikePos );
	savefile->ReadMat3( strikeAxis );
	savefile->ReadInt( nextStrikeFx );

	savefile->ReadBool( nozzleFx );
	savefile->ReadInt( nozzleFxFade );

	savefile->ReadInt( lastAttack );

	savefile->ReadInt( nozzleGlowHandle );
	savefile->ReadRenderLight( nozzleGlow );
	if ( nozzleGlowHandle >= 0 ) {
		nozzleGlowHandle = gameRenderWorld->AddLightDef( &nozzleGlow );
	}

	savefile->ReadVec3( nozzleGlowColor );
	savefile->ReadMaterial( nozzleGlowShader );
	savefile->ReadFloat( nozzleGlowRadius );

	savefile->ReadInt( weaponAngleOffsetAverages );
	savefile->ReadFloat( weaponAngleOffsetScale );
	savefile->ReadFloat( weaponAngleOffsetMax );
	savefile->ReadFloat( weaponOffsetTime );
	savefile->ReadFloat( weaponOffsetScale );

	savefile->ReadBool( allowDrop );
	savefile->ReadObject( reinterpret_cast<idClass *&>( projectileEnt ) );

	savefile->ReadStaticObject( grabber );
	savefile->ReadInt( grabberState );

	savefile->ReadJoint ( smokeJointView );

	int particleCount;
	savefile->ReadInt( particleCount );
	for(int i = 0; i < particleCount; i++) {
		WeaponParticle_t newParticle;
		memset(&newParticle, 0, sizeof(newParticle));

		idStr name, particlename;
		savefile->ReadString( name );
		savefile->ReadString( particlename );

		strcpy( newParticle.name, name.c_str() );
		strcpy( newParticle.particlename, particlename.c_str() );

		savefile->ReadBool( newParticle.active );
		savefile->ReadInt( newParticle.startTime );
		savefile->ReadJoint( newParticle.joint );
		savefile->ReadBool( newParticle.smoke );
		if(newParticle.smoke) {
			newParticle.particle = static_cast<const idDeclParticle *>( declManager->FindType( DECL_PARTICLE, particlename, false ) );
		} else {
			savefile->ReadObject(reinterpret_cast<idClass *&>(newParticle.emitter));
		}

		weaponParticles.Set(newParticle.name, newParticle);
	}

	int lightCount;
	savefile->ReadInt( lightCount );
	for(int i = 0; i < lightCount; i++) {
		WeaponLight_t newLight;
		memset(&newLight, 0, sizeof(newLight));
		
		idStr name;
		savefile->ReadString( name );
		strcpy( newLight.name, name.c_str() );

		savefile->ReadBool( newLight.active );
		savefile->ReadInt( newLight.startTime );
		savefile->ReadJoint( newLight.joint );
		savefile->ReadInt( newLight.lightHandle );
		savefile->ReadRenderLight( newLight.light );
		if ( newLight.lightHandle >= 0 ) {
			newLight.lightHandle = gameRenderWorld->AddLightDef( &newLight.light );
		}
		weaponLights.Set(newLight.name, newLight);
	}
}

/***********************************************************************

	Weapon definition management

***********************************************************************/

/*
================
idWeapon::Clear
================
*/
void idWeapon::Clear() {
	CancelEvents( &EV_Weapon_Clear );

	DeconstructScriptObject();
	scriptObject.Free();

	WEAPON_ATTACK.Unlink();
	WEAPON_RELOAD.Unlink();
	WEAPON_NETRELOAD.Unlink();
	WEAPON_NETENDRELOAD.Unlink();
	WEAPON_NETFIRING.Unlink();
	WEAPON_RAISEWEAPON.Unlink();
	WEAPON_LOWERWEAPON.Unlink();

	if ( muzzleFlashHandle != -1 ) {
		gameRenderWorld->FreeLightDef( muzzleFlashHandle );
		muzzleFlashHandle = -1;
	}
	if ( muzzleFlashHandle != -1 ) {
		gameRenderWorld->FreeLightDef( muzzleFlashHandle );
		muzzleFlashHandle = -1;
	}
	if ( worldMuzzleFlashHandle != -1 ) {
		gameRenderWorld->FreeLightDef( worldMuzzleFlashHandle );
		worldMuzzleFlashHandle = -1;
	}
	if ( guiLightHandle != -1 ) {
		gameRenderWorld->FreeLightDef( guiLightHandle );
		guiLightHandle = -1;
	}
	if ( nozzleGlowHandle != -1 ) {
		gameRenderWorld->FreeLightDef( nozzleGlowHandle );
		nozzleGlowHandle = -1;
	}

	memset( &renderEntity, 0, sizeof( renderEntity ) );
	renderEntity.entityNum	= entityNumber;

	renderEntity.noShadow		= true;
	renderEntity.noSelfShadow	= true;
	renderEntity.customSkin		= NULL;

	// set default shader parms
	renderEntity.shaderParms[ SHADERPARM_RED ]	= 1.0f;
	renderEntity.shaderParms[ SHADERPARM_GREEN ]= 1.0f;
	renderEntity.shaderParms[ SHADERPARM_BLUE ]	= 1.0f;
	renderEntity.shaderParms[3] = 1.0f;
	renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ] = 0.0f;
	renderEntity.shaderParms[5] = 0.0f;
	renderEntity.shaderParms[6] = 0.0f;
	renderEntity.shaderParms[7] = 0.0f;

	if ( refSound.referenceSound ) {
		refSound.referenceSound->Free( true );
	}
	memset( &refSound, 0, sizeof( refSound_t ) );
	
	// setting diversity to 0 results in no random sound.  -1 indicates random.
	refSound.diversity = -1.0f;

	if ( owner ) {
		// don't spatialize the weapon sounds
		refSound.listenerId = owner->GetListenerId();
	}

	// clear out the sounds from our spawnargs since we'll copy them from the weapon def
	const idKeyValue *kv = spawnArgs.MatchPrefix( "snd_" );
	while( kv ) {
		spawnArgs.Delete( kv->GetKey() );
		kv = spawnArgs.MatchPrefix( "snd_" );
	}

	hideTime		= 300;
	hideDistance	= -15.0f;
	hideStartTime	= gameLocal.time - hideTime;
	hideStart		= 0.0f;
	hideEnd			= 0.0f;
	hideOffset		= 0.0f;
	hide			= false;
	disabled		= false;

	weaponSmoke		= NULL;
	weaponSmokeStartTime = 0;
	continuousSmoke = false;
	strikeSmoke		= NULL;
	strikeSmokeStartTime = 0;
	strikePos.Zero();
	strikeAxis = mat3_identity;
	nextStrikeFx = 0;

	icon			= "";
	pdaIcon			= "";
	displayName		= "";
	itemDesc		= "";

	playerViewAxis.Identity();
	playerViewOrigin.Zero();
	viewWeaponAxis.Identity();
	viewWeaponOrigin.Zero();
	muzzleAxis.Identity();
	muzzleOrigin.Zero();
	pushVelocity.Zero();

	status			= WP_HOLSTERED;
	state			= "";
	idealState		= "";
	animBlendFrames	= 0;
	animDoneTime	= 0;

	projectileDict.Clear();
	meleeDef		= NULL;
	meleeDefName	= "";
	meleeDistance	= 0.0f;
	brassDict.Clear();

	flashTime		= 250;
	lightOn			= false;
	silent_fire		= false;

	grabberState	= -1;
	grabber.Update( owner, true );

	ammoType		= 0;
	ammoRequired	= 0;
	ammoClip		= 0;
	clipSize		= 0;
	lowAmmo			= 0;
	powerAmmo		= false;

	kick_endtime		= 0;
	muzzle_kick_time	= 0;
	muzzle_kick_maxtime	= 0;
	muzzle_kick_angles.Zero();
	muzzle_kick_offset.Zero();

	zoomFov = 90;

	barrelJointView		= INVALID_JOINT;
	flashJointView		= INVALID_JOINT;
	ejectJointView		= INVALID_JOINT;
	guiLightJointView	= INVALID_JOINT;
	ventLightJointView	= INVALID_JOINT;

	barrelJointWorld	= INVALID_JOINT;
	flashJointWorld		= INVALID_JOINT;
	ejectJointWorld		= INVALID_JOINT;

	smokeJointView		= INVALID_JOINT;

	//Clean up the weapon particles
	for(int i = 0; i < weaponParticles.Num(); i++) {
		WeaponParticle_t* part = weaponParticles.GetIndex(i);
		if(!part->smoke) {
			if ( part->emitter != NULL ) {
				//Destroy the emitters
				part->emitter->PostEventMS(&EV_Remove, 0 );
			}
		}
	}
	weaponParticles.Clear();

	//Clean up the weapon lights
	for(int i = 0; i < weaponLights.Num(); i++) {
		WeaponLight_t* light = weaponLights.GetIndex(i);
		if ( light->lightHandle != -1 ) {
			gameRenderWorld->FreeLightDef( light->lightHandle );
		}
	}
	weaponLights.Clear();

	hasBloodSplat		= false;
	nozzleFx			= false;
	nozzleFxFade		= 1500;
	lastAttack			= 0;
	nozzleGlowHandle	= -1;
	nozzleGlowShader	= NULL;
	nozzleGlowRadius	= 10;
	nozzleGlowColor.Zero();

	weaponAngleOffsetAverages	= 0;
	weaponAngleOffsetScale		= 0.0f;
	weaponAngleOffsetMax		= 0.0f;
	weaponOffsetTime			= 0.0f;
	weaponOffsetScale			= 0.0f;

	allowDrop			= true;

	animator.ClearAllAnims( gameLocal.time, 0 );
	FreeModelDef();

	sndHum				= NULL;

	isLinked			= false;
	projectileEnt		= NULL;

	isFiring			= false;
}

/*
================
idWeapon::InitWorldModel
================
*/
void idWeapon::InitWorldModel( const idDeclEntityDef *def ) {
	idEntity *ent;

	ent = worldModel.GetEntity();

	assert( ent );
	assert( def );

	const char *model = def->dict.GetString( "model_world" );
	const char *attach = def->dict.GetString( "joint_attach" );

	ent->SetSkin( NULL );
	if ( model[0] && attach[0] ) {
		ent->Show();
		ent->SetModel( model );
		if ( ent->GetAnimator()->ModelDef() ) {
			ent->SetSkin( ent->GetAnimator()->ModelDef()->GetDefaultSkin() );
		}
		ent->GetPhysics()->SetContents( 0 );
		ent->GetPhysics()->SetClipModel( NULL, 1.0f );
		ent->BindToJoint( owner, attach, true );
		ent->GetPhysics()->SetOrigin( vec3_origin );
		ent->GetPhysics()->SetAxis( mat3_identity );

		// We don't want to interpolate the world model of weapons, let them
		// just bind normally to the player's joint and be driven by the player's
		// animation so that the weapon and the player don't appear out of sync.
		ent->SetUseClientInterpolation( false );

		// supress model in player views, but allow it in mirrors and remote views
		renderEntity_t *worldModelRenderEntity = ent->GetRenderEntity();
		if ( worldModelRenderEntity ) {
			worldModelRenderEntity->suppressSurfaceInViewID = owner->entityNumber+1;
			worldModelRenderEntity->suppressShadowInViewID = owner->entityNumber+1;
			worldModelRenderEntity->suppressShadowInLightID = LIGHTID_VIEW_MUZZLE_FLASH + owner->entityNumber;
		}
	} else {
		ent->SetModel( "" );
		ent->Hide();
	}

	flashJointWorld = ent->GetAnimator()->GetJointHandle( "flash" );
	barrelJointWorld = ent->GetAnimator()->GetJointHandle( "muzzle" );
	ejectJointWorld = ent->GetAnimator()->GetJointHandle( "eject" );
}

/*
================
idWeapon::GetWeaponDef
================
*/
void idWeapon::GetWeaponDef( const char *objectname, int ammoinclip ) {
	const char *shader;
	const char *objectType;
	const char *vmodel;
	const char *guiName;
	const char *projectileName;
	const char *brassDefName;
	const char *smokeName;
	int			ammoAvail;

	Clear();

	if ( !objectname || !objectname[ 0 ] ) {
		return;
	}

	assert( owner );

	weaponDef			= gameLocal.FindEntityDef( objectname );

	ammoType			= GetAmmoNumForName( weaponDef->dict.GetString( "ammoType" ) );
	ammoRequired		= weaponDef->dict.GetInt( "ammoRequired" );
	clipSize			= weaponDef->dict.GetInt( "clipSize" );
	lowAmmo				= weaponDef->dict.GetInt( "lowAmmo" );

	icon				= weaponDef->dict.GetString( "icon" );
	pdaIcon				= weaponDef->dict.GetString( "pdaIcon" );
	displayName			= weaponDef->dict.GetString( "display_name" );
	itemDesc			= weaponDef->dict.GetString( "inv_desc" );

	silent_fire			= weaponDef->dict.GetBool( "silent_fire" );
	powerAmmo			= weaponDef->dict.GetBool( "powerAmmo" );

	muzzle_kick_time	= SEC2MS( weaponDef->dict.GetFloat( "muzzle_kick_time" ) );
	muzzle_kick_maxtime	= SEC2MS( weaponDef->dict.GetFloat( "muzzle_kick_maxtime" ) );
	muzzle_kick_angles	= weaponDef->dict.GetAngles( "muzzle_kick_angles" );
	muzzle_kick_offset	= weaponDef->dict.GetVector( "muzzle_kick_offset" );

	hideTime			= SEC2MS( weaponDef->dict.GetFloat( "hide_time", "0.3" ) );
	hideDistance		= weaponDef->dict.GetFloat( "hide_distance", "-15" );

	// muzzle smoke
	smokeName = weaponDef->dict.GetString( "smoke_muzzle" );
	if ( *smokeName != '\0' ) {
		weaponSmoke = static_cast<const idDeclParticle *>( declManager->FindType( DECL_PARTICLE, smokeName ) );
	} else {
		weaponSmoke = NULL;
	}
	continuousSmoke = weaponDef->dict.GetBool( "continuousSmoke" );
	weaponSmokeStartTime = ( continuousSmoke ) ? gameLocal.time : 0;

	smokeName = weaponDef->dict.GetString( "smoke_strike" );
	if ( *smokeName != '\0' ) {
		strikeSmoke = static_cast<const idDeclParticle *>( declManager->FindType( DECL_PARTICLE, smokeName ) );
	} else {
		strikeSmoke = NULL;
	}
	strikeSmokeStartTime = 0;
	strikePos.Zero();
	strikeAxis = mat3_identity;
	nextStrikeFx = 0;

	// setup gui light
	memset( &guiLight, 0, sizeof( guiLight ) );
	const char *guiLightShader = weaponDef->dict.GetString( "mtr_guiLightShader" );
	if ( *guiLightShader != '\0' ) {
		guiLight.shader = declManager->FindMaterial( guiLightShader, false );
		guiLight.lightRadius[0] = guiLight.lightRadius[1] = guiLight.lightRadius[2] = 3;
		guiLight.pointLight = true;
	}

	// setup the view model
	vmodel = weaponDef->dict.GetString( "model_view" );
	SetModel( vmodel );

	// setup the world model
	InitWorldModel( weaponDef );

	// copy the sounds from the weapon view model def into out spawnargs
	const idKeyValue *kv = weaponDef->dict.MatchPrefix( "snd_" );
	while( kv ) {
		spawnArgs.Set( kv->GetKey(), kv->GetValue() );
		kv = weaponDef->dict.MatchPrefix( "snd_", kv );
	}

	// find some joints in the model for locating effects
	barrelJointView = animator.GetJointHandle( "barrel" );
	flashJointView = animator.GetJointHandle( "flash" );
	ejectJointView = animator.GetJointHandle( "eject" );
	guiLightJointView = animator.GetJointHandle( "guiLight" );
	ventLightJointView = animator.GetJointHandle( "ventLight" );

	idStr smokeJoint = weaponDef->dict.GetString("smoke_joint");
	if(smokeJoint.Length() > 0) {
		smokeJointView = animator.GetJointHandle( smokeJoint );
	} else {
		smokeJointView = INVALID_JOINT;
	}

	// get the projectile
	projectileDict.Clear();

	projectileName = weaponDef->dict.GetString( "def_projectile" );
	if ( projectileName[0] != '\0' ) {
		const idDeclEntityDef *projectileDef = gameLocal.FindEntityDef( projectileName, false );
		if ( !projectileDef ) {
			gameLocal.Warning( "Unknown projectile '%s' in weapon '%s'", projectileName, objectname );
		} else {
			const char *spawnclass = projectileDef->dict.GetString( "spawnclass" );
			idTypeInfo *cls = idClass::GetClass( spawnclass );
			if ( !cls || !cls->IsType( idProjectile::Type ) ) {
				gameLocal.Warning( "Invalid spawnclass '%s' on projectile '%s' (used by weapon '%s')", spawnclass, projectileName, objectname );
			} else {
				projectileDict = projectileDef->dict;
			}
		}
	}

	// set up muzzleflash render light
	const idMaterial*flashShader;
	idVec3			flashTarget;
	idVec3			flashUp;
	idVec3			flashRight;
	float			flashRadius;
	bool			flashPointLight;

	weaponDef->dict.GetString( "mtr_flashShader", "", &shader );
	flashShader = declManager->FindMaterial( shader, false );
	flashPointLight = weaponDef->dict.GetBool( "flashPointLight", "1" );
	weaponDef->dict.GetVector( "flashColor", "0 0 0", flashColor );
	flashRadius		= (float)weaponDef->dict.GetInt( "flashRadius" );	// if 0, no light will spawn
	flashTime		= SEC2MS( weaponDef->dict.GetFloat( "flashTime", "0.25" ) );
	flashTarget		= weaponDef->dict.GetVector( "flashTarget" );
	flashUp			= weaponDef->dict.GetVector( "flashUp" );
	flashRight		= weaponDef->dict.GetVector( "flashRight" );

	memset( &muzzleFlash, 0, sizeof( muzzleFlash ) );
	muzzleFlash.lightId = LIGHTID_VIEW_MUZZLE_FLASH + owner->entityNumber;
	muzzleFlash.allowLightInViewID = owner->entityNumber+1;

	// the weapon lights will only be in first person
	guiLight.allowLightInViewID = owner->entityNumber+1;
	nozzleGlow.allowLightInViewID = owner->entityNumber+1;

	muzzleFlash.pointLight								= flashPointLight;
	muzzleFlash.shader									= flashShader;
	muzzleFlash.shaderParms[ SHADERPARM_RED ]			= flashColor[0];
	muzzleFlash.shaderParms[ SHADERPARM_GREEN ]			= flashColor[1];
	muzzleFlash.shaderParms[ SHADERPARM_BLUE ]			= flashColor[2];
	muzzleFlash.shaderParms[ SHADERPARM_TIMESCALE ]		= 1.0f;

	muzzleFlash.lightRadius[0]							= flashRadius;
	muzzleFlash.lightRadius[1]							= flashRadius;
	muzzleFlash.lightRadius[2]							= flashRadius;

	if ( !flashPointLight ) {
		muzzleFlash.target								= flashTarget;
		muzzleFlash.up									= flashUp;
		muzzleFlash.right								= flashRight;
		muzzleFlash.end									= flashTarget;
	}

	// the world muzzle flash is the same, just positioned differently
	worldMuzzleFlash = muzzleFlash;
	worldMuzzleFlash.suppressLightInViewID = owner->entityNumber+1;
	worldMuzzleFlash.allowLightInViewID = 0;
	worldMuzzleFlash.lightId = LIGHTID_WORLD_MUZZLE_FLASH + owner->entityNumber;

	//-----------------------------------

	nozzleFx			= weaponDef->dict.GetBool("nozzleFx");
	nozzleFxFade		= weaponDef->dict.GetInt("nozzleFxFade", "1500");
	nozzleGlowColor		= weaponDef->dict.GetVector("nozzleGlowColor", "1 1 1");
	nozzleGlowRadius	= weaponDef->dict.GetFloat("nozzleGlowRadius", "10");
	weaponDef->dict.GetString( "mtr_nozzleGlowShader", "", &shader );
	nozzleGlowShader = declManager->FindMaterial( shader, false );

	// get the melee damage def
	meleeDistance = weaponDef->dict.GetFloat( "melee_distance" );
	meleeDefName = weaponDef->dict.GetString( "def_melee" );
	if ( meleeDefName.Length() ) {
		meleeDef = gameLocal.FindEntityDef( meleeDefName, false );
		if ( !meleeDef ) {
			gameLocal.Error( "Unknown melee '%s'", meleeDefName.c_str() );
		}
	}

	// get the brass def
	brassDict.Clear();
	brassDelay = weaponDef->dict.GetInt( "ejectBrassDelay", "0" );
	brassDefName = weaponDef->dict.GetString( "def_ejectBrass" );

	if ( brassDefName[0] ) {
		const idDeclEntityDef *brassDef = gameLocal.FindEntityDef( brassDefName, false );
		if ( !brassDef ) {
			gameLocal.Warning( "Unknown brass '%s'", brassDefName );
		} else {
			brassDict = brassDef->dict;
		}
	}

	if ( ( ammoType < 0 ) || ( ammoType >= AMMO_NUMTYPES ) ) {
		gameLocal.Warning( "Unknown ammotype in object '%s'", objectname );
	}

	ammoClip = ammoinclip;
	if ( ( ammoClip.Get() < 0 ) || ( ammoClip.Get() > clipSize ) ) {
		// first time using this weapon so have it fully loaded to start
		ammoClip = clipSize;
		ammoAvail = owner->inventory.HasAmmo( ammoType, ammoRequired );
		if ( ammoClip.Get() > ammoAvail ) {
			ammoClip = ammoAvail;
		}
		//In D3XP we use ammo as soon as it is moved into the clip. This allows for weapons that share ammo
		owner->inventory.UseAmmo( ammoType, ammoClip.Get() );
	}

	renderEntity.gui[ 0 ] = NULL;
	guiName = weaponDef->dict.GetString( "gui" );
	if ( guiName[0] ) {
		renderEntity.gui[ 0 ] = uiManager->FindGui( guiName, true, false, true );
	}

	zoomFov = weaponDef->dict.GetInt( "zoomFov", "70" );
	berserk = weaponDef->dict.GetInt( "berserk", "2" );

	weaponAngleOffsetAverages = weaponDef->dict.GetInt( "weaponAngleOffsetAverages", "10" );
	weaponAngleOffsetScale = weaponDef->dict.GetFloat( "weaponAngleOffsetScale", "0.25" );
	weaponAngleOffsetMax = weaponDef->dict.GetFloat( "weaponAngleOffsetMax", "10" );

	weaponOffsetTime = weaponDef->dict.GetFloat( "weaponOffsetTime", "400" );
	weaponOffsetScale = weaponDef->dict.GetFloat( "weaponOffsetScale", "0.005" );

	if ( !weaponDef->dict.GetString( "weapon_scriptobject", NULL, &objectType ) ) {
		gameLocal.Error( "No 'weapon_scriptobject' set on '%s'.", objectname );
	}
	
	// setup script object
	if ( !scriptObject.SetType( objectType ) ) {
		gameLocal.Error( "Script object '%s' not found on weapon '%s'.", objectType, objectname );
	}

	WEAPON_ATTACK.LinkTo(		scriptObject, "WEAPON_ATTACK" );
	WEAPON_RELOAD.LinkTo(		scriptObject, "WEAPON_RELOAD" );
	WEAPON_NETRELOAD.LinkTo(	scriptObject, "WEAPON_NETRELOAD" );
	WEAPON_NETENDRELOAD.LinkTo(	scriptObject, "WEAPON_NETENDRELOAD" );
	WEAPON_NETFIRING.LinkTo(	scriptObject, "WEAPON_NETFIRING" );
	WEAPON_RAISEWEAPON.LinkTo(	scriptObject, "WEAPON_RAISEWEAPON" );
	WEAPON_LOWERWEAPON.LinkTo(	scriptObject, "WEAPON_LOWERWEAPON" );

	spawnArgs = weaponDef->dict;

	shader = spawnArgs.GetString( "snd_hum" );
	if ( shader && *shader ) {
		sndHum = declManager->FindSound( shader );
		StartSoundShader( sndHum, SND_CHANNEL_BODY, 0, false, NULL );
	}

	isLinked = true;

	// call script object's constructor
	ConstructScriptObject();

	// make sure we have the correct skin
	UpdateSkin();

	idEntity *ent = worldModel.GetEntity();
	DetermineTimeGroup( weaponDef->dict.GetBool( "slowmo", "0" ) );
	if ( ent ) {
		ent->DetermineTimeGroup( weaponDef->dict.GetBool( "slowmo", "0" ) );
	}

	//Initialize the particles
	if ( !common->IsMultiplayer() ) {

		const idKeyValue *pkv = weaponDef->dict.MatchPrefix( "weapon_particle", NULL );
		while( pkv ) {
			WeaponParticle_t newParticle;
			memset( &newParticle, 0, sizeof( newParticle ) );

			idStr name = pkv->GetValue();

			strcpy(newParticle.name, name.c_str());

			idStr jointName = weaponDef->dict.GetString(va("%s_joint", name.c_str()));
			newParticle.joint = animator.GetJointHandle(jointName.c_str());
			newParticle.smoke = weaponDef->dict.GetBool(va("%s_smoke", name.c_str()));
			newParticle.active = false;
			newParticle.startTime = 0;

			idStr particle = weaponDef->dict.GetString(va("%s_particle", name.c_str()));
			strcpy(newParticle.particlename, particle.c_str());

			if(newParticle.smoke) {
				newParticle.particle = static_cast<const idDeclParticle *>( declManager->FindType( DECL_PARTICLE, particle, false ) );
			} else {
				idDict args;

				const idDeclEntityDef *emitterDef = gameLocal.FindEntityDef( "func_emitter", false );
				args = emitterDef->dict;
				args.Set("model", particle.c_str());
				args.SetBool("start_off", true);

				idEntity* ent;
				gameLocal.SpawnEntityDef(args, &ent, false);
				newParticle.emitter = (idFuncEmitter*)ent;

				if ( newParticle.emitter != NULL ) {
					newParticle.emitter->BecomeActive(TH_THINK);
				}
			}

			weaponParticles.Set(name.c_str(), newParticle);

			pkv = weaponDef->dict.MatchPrefix( "weapon_particle", pkv );
		}

		const idKeyValue *lkv = weaponDef->dict.MatchPrefix( "weapon_light", NULL );
		while( lkv ) {
			WeaponLight_t newLight;
			memset( &newLight, 0, sizeof( newLight ) );

			newLight.lightHandle = -1;
			newLight.active = false;
			newLight.startTime = 0;

			idStr name = lkv->GetValue();
			strcpy(newLight.name, name.c_str());

			idStr jointName = weaponDef->dict.GetString(va("%s_joint", name.c_str()));
			newLight.joint = animator.GetJointHandle(jointName.c_str());

			idStr shader = weaponDef->dict.GetString(va("%s_shader", name.c_str()));
			newLight.light.shader = declManager->FindMaterial( shader, false );

			float radius = weaponDef->dict.GetFloat(va("%s_radius", name.c_str()));
			newLight.light.lightRadius[0] = newLight.light.lightRadius[1] = newLight.light.lightRadius[2] = radius;
			newLight.light.pointLight = true;
			newLight.light.noShadows = true;

			newLight.light.allowLightInViewID = owner->entityNumber+1;

			weaponLights.Set(name.c_str(), newLight);

			lkv = weaponDef->dict.MatchPrefix( "weapon_light", lkv );
		}
	}
}

/***********************************************************************

	GUIs

***********************************************************************/

/*
================
idWeapon::Icon
================
*/
const char *idWeapon::Icon() const {
	return icon;
}

/*
================
idWeapon::PdaIcon
================
*/
const char *idWeapon::PdaIcon() const {
	return pdaIcon;
}

/*
================
idWeapon::DisplayName
================
*/
const char * idWeapon::DisplayName() const {
	return idLocalization::GetString( displayName );
}

/*
================
idWeapon::Description
================
*/
const char * idWeapon::Description() const {
	return idLocalization::GetString( itemDesc );
}

/*
================
idWeapon::UpdateGUI
================
*/
void idWeapon::UpdateGUI() {
	if ( !renderEntity.gui[ 0 ] ) {
		return;
	}
	
	if ( status == WP_HOLSTERED ) {
		return;
	}

	if ( owner->weaponGone ) {
		// dropping weapons was implemented wierd, so we have to not update the gui when it happens or we'll get a negative ammo count
		return;
	}

	if ( !owner->IsLocallyControlled() ) {
		// if updating the hud for a followed client
		if ( gameLocal.GetLocalClientNum() >= 0 && gameLocal.entities[ gameLocal.GetLocalClientNum() ] && gameLocal.entities[ gameLocal.GetLocalClientNum() ]->IsType( idPlayer::Type ) ) {
			idPlayer *p = static_cast< idPlayer * >( gameLocal.entities[ gameLocal.GetLocalClientNum() ] );
			if ( !p->spectating || p->spectator != owner->entityNumber ) {
				return;
			}
		} else {
			return;
		}
	}

	int inclip = AmmoInClip();
	int ammoamount = AmmoAvailable();

	if ( ammoamount < 0 ) {
		// show infinite ammo
		renderEntity.gui[ 0 ]->SetStateString( "player_ammo", "" );
	} else {
		// show remaining ammo
		renderEntity.gui[ 0 ]->SetStateString( "player_totalammo", va( "%i", ammoamount) );
		renderEntity.gui[ 0 ]->SetStateString( "player_ammo", ClipSize() ? va( "%i", inclip ) : "--" );
		renderEntity.gui[ 0 ]->SetStateString( "player_clips", ClipSize() ? va("%i", ammoamount / ClipSize()) : "--" );

		renderEntity.gui[ 0 ]->SetStateString( "player_allammo", va( "%i/%i", inclip, ammoamount ) );
	}
	renderEntity.gui[ 0 ]->SetStateBool( "player_ammo_empty", ( ammoamount == 0 ) );
	renderEntity.gui[ 0 ]->SetStateBool( "player_clip_empty", ( inclip == 0 ) );
	renderEntity.gui[ 0 ]->SetStateBool( "player_clip_low", ( inclip <= lowAmmo ) );

	//Let the HUD know the total amount of ammo regardless of the ammo required value
	renderEntity.gui[ 0 ]->SetStateString( "player_ammo_count", va("%i", AmmoCount()));

	//Grabber Gui Info
	renderEntity.gui[ 0 ]->SetStateString( "grabber_state", va("%i", grabberState));
}

/***********************************************************************

	Model and muzzleflash

***********************************************************************/

/*
================
idWeapon::UpdateFlashPosition
================
*/
void idWeapon::UpdateFlashPosition() {
	// the flash has an explicit joint for locating it
	GetGlobalJointTransform( true, flashJointView, muzzleFlash.origin, muzzleFlash.axis );

	if ( isPlayerFlashlight ) {
		static float pscale = 2.0f;
		static float yscale = 0.25f;

// 		static idVec3 baseAdjustPos = vec3_zero;	//idVec3( 0.0f, 10.0f, 0.0f );
// 		idVec3 adjustPos = baseAdjustPos;
//		muzzleFlash.origin += adjustPos.x * muzzleFlash.axis[1] + adjustPos.y * muzzleFlash.axis[0] + adjustPos.z * muzzleFlash.axis[2];
 		muzzleFlash.origin += owner->GetViewBob();

//		static idAngles baseAdjustAng = ang_zero;	//idAngles( 0.0f, 10.0f, 0.0f );
		idAngles adjustAng = /*baseAdjustAng +*/ idAngles( fraccos * yscale, 0.0f, fraccos2 * pscale );
		idAngles bobAngles = owner->GetViewBobAngles();
		SwapValues( bobAngles.pitch, bobAngles.roll );
		adjustAng += bobAngles * 3.0f;
		muzzleFlash.axis = adjustAng.ToMat3() * muzzleFlash.axis /** adjustAng.ToMat3()*/;
	}

	// if the desired point is inside or very close to a wall, back it up until it is clear
	idVec3	start = muzzleFlash.origin - playerViewAxis[0] * 16;
	idVec3	end = muzzleFlash.origin + playerViewAxis[0] * 8;
	trace_t	tr;
	gameLocal.clip.TracePoint( tr, start, end, MASK_SHOT_RENDERMODEL, owner );
	// be at least 8 units away from a solid
	muzzleFlash.origin = tr.endpos - playerViewAxis[0] * 8;

	muzzleFlash.noShadows = !g_weaponShadows.GetBool();

	// put the world muzzle flash on the end of the joint, no matter what
	GetGlobalJointTransform( false, flashJointWorld, worldMuzzleFlash.origin, worldMuzzleFlash.axis );
}

/*
================
idWeapon::MuzzleFlashLight
================
*/
void idWeapon::MuzzleFlashLight() {
	
	if ( !lightOn && ( !g_muzzleFlash.GetBool() || !muzzleFlash.lightRadius[0] ) ) {
		return;
	}

	if ( flashJointView == INVALID_JOINT ) {
		return;
	}

	UpdateFlashPosition();

	// these will be different each fire
	muzzleFlash.shaderParms[ SHADERPARM_TIMEOFFSET ]	= -MS2SEC( gameLocal.time );
	muzzleFlash.shaderParms[ SHADERPARM_DIVERSITY ]		= renderEntity.shaderParms[ SHADERPARM_DIVERSITY ];

	worldMuzzleFlash.shaderParms[ SHADERPARM_TIMEOFFSET ]	= -MS2SEC( gameLocal.time );
	worldMuzzleFlash.shaderParms[ SHADERPARM_DIVERSITY ]	= renderEntity.shaderParms[ SHADERPARM_DIVERSITY ];

	// the light will be removed at this time
	muzzleFlashEnd = gameLocal.time + flashTime;

	if ( muzzleFlashHandle != -1 ) {
		gameRenderWorld->UpdateLightDef( muzzleFlashHandle, &muzzleFlash );
		gameRenderWorld->UpdateLightDef( worldMuzzleFlashHandle, &worldMuzzleFlash );
	} else {
		muzzleFlashHandle = gameRenderWorld->AddLightDef( &muzzleFlash );
		worldMuzzleFlashHandle = gameRenderWorld->AddLightDef( &worldMuzzleFlash );
	}
}

/*
================
idWeapon::UpdateSkin
================
*/
bool idWeapon::UpdateSkin() {
	const function_t *func;

	if ( !isLinked ) {
		return false;
	}

	func = scriptObject.GetFunction( "UpdateSkin" );
	if ( !func ) {
		common->Warning( "Can't find function 'UpdateSkin' in object '%s'", scriptObject.GetTypeName() );
		return false;
	}
	
	// use the frameCommandThread since it's safe to use outside of framecommands
	gameLocal.frameCommandThread->CallFunction( this, func, true );
	gameLocal.frameCommandThread->Execute();

	return true;
}


/*
================
idWeapon::FlashlightOn
================
*/
void idWeapon::FlashlightOn() {
	const function_t *func;

	if ( !isLinked ) {
		return;
	}

	func = scriptObject.GetFunction( "TurnOn" );
	if ( !func ) {
		common->Warning( "Can't find function 'TurnOn' in object '%s'", scriptObject.GetTypeName() );
		return;
	}

	// use the frameCommandThread since it's safe to use outside of framecommands
	gameLocal.frameCommandThread->CallFunction( this, func, true );
	gameLocal.frameCommandThread->Execute();

	return;
}


/*
================
idWeapon::FlashlightOff
================
*/
void idWeapon::FlashlightOff() {
	const function_t *func;

	if ( !isLinked ) {
		return;
	}

	func = scriptObject.GetFunction( "TurnOff" );
	if ( !func ) {
		common->Warning( "Can't find function 'TurnOff' in object '%s'", scriptObject.GetTypeName() );
		return;
	}

	// use the frameCommandThread since it's safe to use outside of framecommands
	gameLocal.frameCommandThread->CallFunction( this, func, true );
	gameLocal.frameCommandThread->Execute();

	return;
}

/*
================
idWeapon::SetModel
================
*/
void idWeapon::SetModel( const char *modelname ) {
	assert( modelname );

	if ( modelDefHandle >= 0 ) {
		gameRenderWorld->RemoveDecals( modelDefHandle );
	}

	renderEntity.hModel = animator.SetModel( modelname );
	if ( renderEntity.hModel ) {
		renderEntity.customSkin = animator.ModelDef()->GetDefaultSkin();
		animator.GetJoints( &renderEntity.numJoints, &renderEntity.joints );
	} else {
		renderEntity.customSkin = NULL;
		renderEntity.callback = NULL;
		renderEntity.numJoints = 0;
		renderEntity.joints = NULL;
	}

	// hide the model until an animation is played
	Hide();
}

/*
================
idWeapon::GetGlobalJointTransform

This returns the offset and axis of a weapon bone in world space, suitable for attaching models or lights
================
*/
bool idWeapon::GetGlobalJointTransform( bool viewModel, const jointHandle_t jointHandle, idVec3 &offset, idMat3 &axis ) {
	if ( viewModel ) {
		// view model
		if ( animator.GetJointTransform( jointHandle, gameLocal.time, offset, axis ) ) {
			offset = offset * viewWeaponAxis + viewWeaponOrigin;
			axis = axis * viewWeaponAxis;
			return true;
		}
	} else {
		// world model
		if ( worldModel.GetEntity() && worldModel.GetEntity()->GetAnimator()->GetJointTransform( jointHandle, gameLocal.time, offset, axis ) ) {
			offset = worldModel.GetEntity()->GetPhysics()->GetOrigin() + offset * worldModel.GetEntity()->GetPhysics()->GetAxis();
			axis = axis * worldModel.GetEntity()->GetPhysics()->GetAxis();
			return true;
		}
	}
	offset = viewWeaponOrigin;
	axis = viewWeaponAxis;
	return false;
}

/*
================
idWeapon::SetPushVelocity
================
*/
void idWeapon::SetPushVelocity( const idVec3 &pushVelocity ) {
	this->pushVelocity = pushVelocity;
}


/***********************************************************************

	State control/player interface

***********************************************************************/

/*
================
idWeapon::Think
================
*/
void idWeapon::Think() {
	// do nothing because the present is called from the player through PresentWeapon
}

/*
================
idWeapon::Raise
================
*/
void idWeapon::Raise() {
	if ( isLinked ) {
		WEAPON_RAISEWEAPON = true;
	}
}

/*
================
idWeapon::PutAway
================
*/
void idWeapon::PutAway() {
	hasBloodSplat = false;
	if ( isLinked ) {
		WEAPON_LOWERWEAPON = true;
	}
}

/*
================
idWeapon::Reload
NOTE: this is only for impulse-triggered reload, auto reload is scripted
================
*/
void idWeapon::Reload() {
	if ( isLinked ) {
		WEAPON_RELOAD = true;
	}
}

/*
================
idWeapon::LowerWeapon
================
*/
void idWeapon::LowerWeapon() {
	if ( !hide ) {
		hideStart	= 0.0f;
		hideEnd		= hideDistance;
		if ( gameLocal.time - hideStartTime < hideTime ) {
			hideStartTime = gameLocal.time - ( hideTime - ( gameLocal.time - hideStartTime ) );
		} else {
			hideStartTime = gameLocal.time;
		}
		hide = true;
	}
}

/*
================
idWeapon::RaiseWeapon
================
*/
void idWeapon::RaiseWeapon() {
	Show();

	if ( hide ) {
		hideStart	= hideDistance;
		hideEnd		= 0.0f;
		if ( gameLocal.time - hideStartTime < hideTime ) {
			hideStartTime = gameLocal.time - ( hideTime - ( gameLocal.time - hideStartTime ) );
		} else {
			hideStartTime = gameLocal.time;
		}
		hide = false;
	}
}

/*
================
idWeapon::HideWeapon
================
*/
void idWeapon::HideWeapon() {
	Hide();
	if ( worldModel.GetEntity() ) {
		worldModel.GetEntity()->Hide();
	}
	muzzleFlashEnd = 0;
}

/*
================
idWeapon::ShowWeapon
================
*/
void idWeapon::ShowWeapon() {
	Show();
	if ( worldModel.GetEntity() ) {
		worldModel.GetEntity()->Show();
	}
	if ( lightOn ) {
		MuzzleFlashLight();
	}
}

/*
================
idWeapon::HideWorldModel
================
*/
void idWeapon::HideWorldModel() {
	if ( worldModel.GetEntity() ) {
		worldModel.GetEntity()->Hide();
	}
}

/*
================
idWeapon::ShowWorldModel
================
*/
void idWeapon::ShowWorldModel() {
	if ( worldModel.GetEntity() ) {
		worldModel.GetEntity()->Show();
	}
}

/*
================
idWeapon::OwnerDied
================
*/
void idWeapon::OwnerDied() {
	if ( isLinked ) {
		SetState( "OwnerDied", 0 );
		thread->Execute();

		// Update the grabber effects
		if ( /*!common->IsMultiplayer() &&*/ grabberState != -1 ) {
			grabber.Update( owner, hide );
		}
	}

	Hide();
	if ( worldModel.GetEntity() ) {
		worldModel.GetEntity()->Hide();
	}

	// don't clear the weapon immediately since the owner might have killed himself by firing the weapon
	// within the current stack frame
	PostEventMS( &EV_Weapon_Clear, 0 );
}

/*
================
idWeapon::BeginAttack
================
*/
void idWeapon::BeginAttack() {	
	if ( status != WP_OUTOFAMMO ) {
		lastAttack = gameLocal.time;
	}

	if ( !isLinked ) {
		return;
	}

	if ( !WEAPON_ATTACK ) {
		if ( sndHum && grabberState == -1 ) {	// _D3XP :: don't stop grabber hum
			StopSound( SND_CHANNEL_BODY, false );
		}
	}
	WEAPON_ATTACK = true;
}

/*
================
idWeapon::EndAttack
================
*/
void idWeapon::EndAttack() {
	if ( !WEAPON_ATTACK.IsLinked() ) {
		return;
	}
	if ( WEAPON_ATTACK ) {
		WEAPON_ATTACK = false;
		if ( sndHum && grabberState == -1 ) {	// _D3XP :: don't stop grabber hum
			StartSoundShader( sndHum, SND_CHANNEL_BODY, 0, false, NULL );
		}
	}
}

/*
================
idWeapon::isReady
================
*/
bool idWeapon::IsReady() const {
	return !hide && !IsHidden() && ( ( status == WP_RELOAD ) || ( status == WP_READY ) || ( status == WP_OUTOFAMMO ) );
}

/*
================
idWeapon::IsReloading
================
*/
bool idWeapon::IsReloading() const {
	return ( status == WP_RELOAD );
}

/*
================
idWeapon::IsHolstered
================
*/
bool idWeapon::IsHolstered() const {
	return ( status == WP_HOLSTERED );
}

/*
================
idWeapon::ShowCrosshair
================
*/
bool idWeapon::ShowCrosshair() const {
// JDC: this code would never function as written, I'm assuming they wanted the following behavior
//	return !( state == idStr( WP_RISING ) || state == idStr( WP_LOWERING ) || state == idStr( WP_HOLSTERED ) );
	return !( status == WP_RISING || status == WP_LOWERING || status == WP_HOLSTERED || status == WP_RELOAD );
}

/*
=====================
idWeapon::CanDrop
=====================
*/
bool idWeapon::CanDrop() const {
	if ( !weaponDef || !worldModel.GetEntity() ) {
		return false;
	}
	const char *classname = weaponDef->dict.GetString( "def_dropItem" );
	if ( !classname[ 0 ] ) {
		return false;
	}
	return true;
}

/*
================
idWeapon::WeaponStolen
================
*/
void idWeapon::WeaponStolen() {
	assert( !common->IsClient() );
	if ( projectileEnt ) {
		if ( isLinked ) {
			SetState( "WeaponStolen", 0 );
			thread->Execute();
		}
		projectileEnt = NULL;
	}

	// set to holstered so we can switch weapons right away
	status = WP_HOLSTERED;

	HideWeapon();
}

/*
=====================
idWeapon::DropItem
=====================
*/
idEntity * idWeapon::DropItem( const idVec3 &velocity, int activateDelay, int removeDelay, bool died ) {
	if ( !weaponDef || !worldModel.GetEntity() ) {
		return NULL;
	}
	if ( !allowDrop ) {
		return NULL;
	}
	const char *classname = weaponDef->dict.GetString( "def_dropItem" );
	if ( !classname[0] ) {
		return NULL;
	}
	StopSound( SND_CHANNEL_BODY, true );
	StopSound( SND_CHANNEL_BODY3, true );

	return idMoveableItem::DropItem( classname, worldModel.GetEntity()->GetPhysics()->GetOrigin(), worldModel.GetEntity()->GetPhysics()->GetAxis(), velocity, activateDelay, removeDelay );
}

/***********************************************************************

	Script state management

***********************************************************************/

/*
=====================
idWeapon::SetState
=====================
*/
void idWeapon::SetState( const char *statename, int blendFrames ) {
	const function_t *func;

	if ( !isLinked ) {
		return;
	}

	func = scriptObject.GetFunction( statename );
	if ( !func ) {
		assert( 0 );
		gameLocal.Error( "Can't find function '%s' in object '%s'", statename, scriptObject.GetTypeName() );
	}

	thread->CallFunction( this, func, true );
	state = statename;

	animBlendFrames = blendFrames;
	if ( g_debugWeapon.GetBool() ) {
		gameLocal.Printf( "%d: weapon state : %s\n", gameLocal.time, statename );
	}

	idealState = "";
}


/***********************************************************************

	Particles/Effects

***********************************************************************/

/*
================
idWeapon::UpdateNozzelFx
================
*/
void idWeapon::UpdateNozzleFx() {
	if ( !nozzleFx ) {
		return;
	}

	//
	// shader parms
	//
	int la = gameLocal.time - lastAttack + 1;
	float s = 1.0f;
	float l = 0.0f;
	if ( la < nozzleFxFade ) {
		s = ((float)la / nozzleFxFade);
		l = 1.0f - s;
	}
	renderEntity.shaderParms[5] = s;
	renderEntity.shaderParms[6] = l;

	if ( ventLightJointView == INVALID_JOINT ) {
		return;
	}

	//
	// vent light
	//
	if ( nozzleGlowHandle == -1 ) {
		memset(&nozzleGlow, 0, sizeof(nozzleGlow));
		if ( owner ) {
			nozzleGlow.allowLightInViewID = owner->entityNumber+1;
		}
		nozzleGlow.pointLight = true;
		nozzleGlow.noShadows = true;
		nozzleGlow.lightRadius.x = nozzleGlowRadius;
		nozzleGlow.lightRadius.y = nozzleGlowRadius;
		nozzleGlow.lightRadius.z = nozzleGlowRadius;
		nozzleGlow.shader = nozzleGlowShader;
		nozzleGlow.shaderParms[ SHADERPARM_TIMESCALE ]	= 1.0f;
		nozzleGlow.shaderParms[ SHADERPARM_TIMEOFFSET ]	= -MS2SEC( gameLocal.time );
		GetGlobalJointTransform( true, ventLightJointView, nozzleGlow.origin, nozzleGlow.axis );
		nozzleGlowHandle = gameRenderWorld->AddLightDef(&nozzleGlow);
	}

	GetGlobalJointTransform( true, ventLightJointView, nozzleGlow.origin, nozzleGlow.axis );

	nozzleGlow.shaderParms[ SHADERPARM_RED ] = nozzleGlowColor.x * s;
	nozzleGlow.shaderParms[ SHADERPARM_GREEN ] = nozzleGlowColor.y * s;
	nozzleGlow.shaderParms[ SHADERPARM_BLUE ] = nozzleGlowColor.z * s;
	gameRenderWorld->UpdateLightDef(nozzleGlowHandle, &nozzleGlow);
}


/*
================
idWeapon::BloodSplat
================
*/
bool idWeapon::BloodSplat( float size ) {
	float s, c;
	idMat3 localAxis, axistemp;
	idVec3 localOrigin, normal;

	if ( hasBloodSplat ) {
		return true;
	}

	hasBloodSplat = true;

	if ( modelDefHandle < 0 ) {
		return false;
	}

	if ( !GetGlobalJointTransform( true, ejectJointView, localOrigin, localAxis ) ) {
		return false;
	}

	localOrigin[0] += gameLocal.random.RandomFloat() * -10.0f;
	localOrigin[1] += gameLocal.random.RandomFloat() * 1.0f;
	localOrigin[2] += gameLocal.random.RandomFloat() * -2.0f;

	normal = idVec3( gameLocal.random.CRandomFloat(), -gameLocal.random.RandomFloat(), -1 );
	normal.Normalize();

	idMath::SinCos16( gameLocal.random.RandomFloat() * idMath::TWO_PI, s, c );

	localAxis[2] = -normal;
	localAxis[2].NormalVectors( axistemp[0], axistemp[1] );
	localAxis[0] = axistemp[ 0 ] * c + axistemp[ 1 ] * -s;
	localAxis[1] = axistemp[ 0 ] * -s + axistemp[ 1 ] * -c;

	localAxis[0] *= 1.0f / size;
	localAxis[1] *= 1.0f / size;

	idPlane		localPlane[2];

	localPlane[0] = localAxis[0];
	localPlane[0][3] = -(localOrigin * localAxis[0]) + 0.5f;

	localPlane[1] = localAxis[1];
	localPlane[1][3] = -(localOrigin * localAxis[1]) + 0.5f;

	const idMaterial *mtr = declManager->FindMaterial( "textures/decals/duffysplatgun" );

	gameRenderWorld->ProjectOverlay( modelDefHandle, localPlane, mtr, gameLocal.slow.time );

	return true;
}


/***********************************************************************

	Visual presentation

***********************************************************************/

/*
================
idWeapon::MuzzleRise

The machinegun and chaingun will incrementally back up as they are being fired
================
*/
void idWeapon::MuzzleRise( idVec3 &origin, idMat3 &axis ) {
	int			time;
	float		amount;
	idAngles	ang;
	idVec3		offset;

	time = kick_endtime - gameLocal.time;
	if ( time <= 0 ) {
		return;
	}

	if ( muzzle_kick_maxtime <= 0 ) {
		return;
	}

	if ( time > muzzle_kick_maxtime ) {
		time = muzzle_kick_maxtime;
	}
	
	amount = ( float )time / ( float )muzzle_kick_maxtime;
	ang		= muzzle_kick_angles * amount;
	offset	= muzzle_kick_offset * amount;

	origin = origin - axis * offset;
	axis = ang.ToMat3() * axis;
}

/*
================
idWeapon::ConstructScriptObject

Called during idEntity::Spawn.  Calls the constructor on the script object.
Can be overridden by subclasses when a thread doesn't need to be allocated.
================
*/
idThread *idWeapon::ConstructScriptObject() {
	const function_t *constructor;

	thread->EndThread();

	// call script object's constructor
	constructor = scriptObject.GetConstructor();
	if ( !constructor ) {
		gameLocal.Error( "Missing constructor on '%s' for weapon", scriptObject.GetTypeName() );
	}

	// init the script object's data
	scriptObject.ClearObject();
	thread->CallFunction( this, constructor, true );
	thread->Execute();

	return thread;
}

/*
================
idWeapon::DeconstructScriptObject

Called during idEntity::~idEntity.  Calls the destructor on the script object.
Can be overridden by subclasses when a thread doesn't need to be allocated.
Not called during idGameLocal::MapShutdown.
================
*/
void idWeapon::DeconstructScriptObject() {
	const function_t *destructor;

	if ( !thread ) {
		return;
	}
	
	// don't bother calling the script object's destructor on map shutdown
	if ( gameLocal.GameState() == GAMESTATE_SHUTDOWN ) {
		return;
	}

	thread->EndThread();

	// call script object's destructor
	destructor = scriptObject.GetDestructor();
	if ( destructor ) {
		// start a thread that will run immediately and end
		thread->CallFunction( this, destructor, true );
		thread->Execute();
		thread->EndThread();
	}

	// clear out the object's memory
	scriptObject.ClearObject();
}

/*
================
idWeapon::UpdateScript
================
*/
void idWeapon::UpdateScript() {
	int	count;

	if ( !isLinked ) {
		return;
	}

	// only update the script on new frames
	if ( !gameLocal.isNewFrame ) {
		return;
	}

	if ( idealState.Length() ) {
		SetState( idealState, animBlendFrames );
	}

	// update script state, which may call Event_LaunchProjectiles, among other things
	count = 10;
	while( ( thread->Execute() || idealState.Length() ) && count-- ) {
		// happens for weapons with no clip (like grenades)
		if ( idealState.Length() ) {
			SetState( idealState, animBlendFrames );
		}
	}

	WEAPON_RELOAD = false;
}

/*
================
idWeapon::AlertMonsters
================
*/
void idWeapon::AlertMonsters() {
	trace_t	tr;
	idEntity *ent;
	idVec3 end = muzzleFlash.origin + muzzleFlash.axis * muzzleFlash.target;

	gameLocal.clip.TracePoint( tr, muzzleFlash.origin, end, CONTENTS_OPAQUE | MASK_SHOT_RENDERMODEL | CONTENTS_FLASHLIGHT_TRIGGER, owner );
	if ( g_debugWeapon.GetBool() ) {
		gameRenderWorld->DebugLine( colorYellow, muzzleFlash.origin, end, 0 );
		gameRenderWorld->DebugArrow( colorGreen, muzzleFlash.origin, tr.endpos, 2, 0 );
	}

	if ( tr.fraction < 1.0f ) {
		ent = gameLocal.GetTraceEntity( tr );
		if ( ent->IsType( idAI::Type ) ) {
			static_cast<idAI *>( ent )->TouchedByFlashlight( owner );
		} else if ( ent->IsType( idTrigger::Type ) ) {
			ent->Signal( SIG_TOUCH );
			ent->ProcessEvent( &EV_Touch, owner, &tr );
		}
	}

	// jitter the trace to try to catch cases where a trace down the center doesn't hit the monster
	end += muzzleFlash.axis * muzzleFlash.right * idMath::Sin16( MS2SEC( gameLocal.time ) * 31.34f );
	end += muzzleFlash.axis * muzzleFlash.up * idMath::Sin16( MS2SEC( gameLocal.time ) * 12.17f );
	gameLocal.clip.TracePoint( tr, muzzleFlash.origin, end, CONTENTS_OPAQUE | MASK_SHOT_RENDERMODEL | CONTENTS_FLASHLIGHT_TRIGGER, owner );
	if ( g_debugWeapon.GetBool() ) {
		gameRenderWorld->DebugLine( colorYellow, muzzleFlash.origin, end, 0 );
		gameRenderWorld->DebugArrow( colorGreen, muzzleFlash.origin, tr.endpos, 2, 0 );
	}

	if ( tr.fraction < 1.0f ) {
		ent = gameLocal.GetTraceEntity( tr );
		if ( ent->IsType( idAI::Type ) ) {
			static_cast<idAI *>( ent )->TouchedByFlashlight( owner );
		} else if ( ent->IsType( idTrigger::Type ) ) {
			ent->Signal( SIG_TOUCH );
			ent->ProcessEvent( &EV_Touch, owner, &tr );
		}
	}
}

/*
================
idWeapon::GetMuzzlePositionWithHacks

Some weapons that have a barrel joint either have it pointing in the wrong
direction (rocket launcher), or don't animate it properly (pistol).

For good 3D TV / head mounted display work, we need to display a laser sight
in the world.

Fixing the animated meshes would be ideal, but hacking it in code is
the pragmatic move right now.

Returns false for hands, grenades, and chainsaw.
================
*/
bool idWeapon::GetMuzzlePositionWithHacks( idVec3 & origin, idMat3 & axis ) {
	// I couldn't find a simple enum to identify the weapons that need
	// workaround hacks...
	const idStr & weaponIconName = pdaIcon;

	origin = playerViewOrigin;
	axis = playerViewAxis;
	
	if ( weaponIconName == "guis/assets/hud/icons/grenade_new.tga" ) {
		return false;
	}

	if ( weaponIconName == "guis/assets/hud/icons/chainsaw_new.tga" ) {
		return false;
	}

	if ( weaponIconName == "guis/assets/hud/icons/soul_cube.tga" ) {
		return false;
	}

	if ( barrelJointView != INVALID_JOINT ) {
		GetGlobalJointTransform( true, barrelJointView, origin, axis );
	} else if ( guiLightJointView != INVALID_JOINT ) {
		GetGlobalJointTransform( true, guiLightJointView, origin, axis );
	} else {
		return false;
	}

	// get better axis joints for weapons where the barrelJointView isn't
	// animated properly
	idVec3	discardedOrigin;
	if ( weaponIconName == "guis/assets/hud/icons/pistol_new.tga" ) {
		// muzzle doesn't animate during firing, Bod does
		const jointHandle_t bodJoint = animator.GetJointHandle( "Bod" );
		GetGlobalJointTransform( true, bodJoint, discardedOrigin, axis );
	}
	if ( weaponIconName == "guis/assets/hud/icons/rocketlauncher_new.tga" ) {
		// joint doesn't point straight, so rotate it
		std::swap( axis[0], axis[2] );
	}
	if ( weaponIconName == "guis/assets/hud/icons/shotgun_new.tga" ) {
		// joint doesn't point straight, so rotate it
		const jointHandle_t bodJoint = animator.GetJointHandle( "trigger" );
		GetGlobalJointTransform( true, bodJoint, discardedOrigin, axis );
		std::swap( axis[0], axis[2] );
		axis[0] = -axis[0];
	}
	
	// we probably should fix the above hacks above that are based on texture names above at some
	// point
	if ( weaponDef != NULL ) {
		if ( ( idStr::Icmp( "weapon_shotgun_double", weaponDef->GetName() ) == 0 ) || ( idStr::Icmp( "weapon_shotgun_double_mp", weaponDef->GetName() ) == 0 ) ) {
			// joint doesn't point straight, so rotate it
			std::swap( axis[0], axis[2] );
		} else if ( idStr::Icmp( "weapon_grabber", weaponDef->GetName() ) == 0 ) {
			idVec3 forward = axis[0];
			forward.Normalize();
			const float scaleOffset = 4.0f;
			forward *= scaleOffset;
			origin += forward;
		}
	}
	
	return true;
}

/*
================
idWeapon::PresentWeapon
================
*/
void idWeapon::PresentWeapon( bool showViewModel ) {
	playerViewOrigin = owner->firstPersonViewOrigin;
	playerViewAxis = owner->firstPersonViewAxis;

	if ( isPlayerFlashlight ) {
		viewWeaponOrigin = playerViewOrigin;
		viewWeaponAxis = playerViewAxis;

		fraccos = cos( ( gameLocal.framenum & 255 ) / 127.0f * idMath::PI );

		static unsigned int divisor = 32;
		unsigned int val = ( gameLocal.framenum + gameLocal.framenum / divisor ) & 255;
		fraccos2 = cos( val / 127.0f * idMath::PI );

		static idVec3 baseAdjustPos = idVec3( -8.0f, -20.0f, -10.0f );		// rt, fwd, up
		static float pscale = 0.5f;
		static float yscale = 0.125f;
		idVec3 adjustPos = baseAdjustPos;// + ( idVec3( fraccos, 0.0f, fraccos2 ) * scale );
		viewWeaponOrigin += adjustPos.x * viewWeaponAxis[1] + adjustPos.y * viewWeaponAxis[0] + adjustPos.z * viewWeaponAxis[2];
//		viewWeaponOrigin += owner->viewBob;

		static idAngles baseAdjustAng = idAngles( 88.0f, 10.0f, 0.0f );		//
		idAngles adjustAng = baseAdjustAng + idAngles( fraccos * pscale, fraccos2 * yscale, 0.0f );
//		adjustAng += owner->GetViewBobAngles();
		viewWeaponAxis = adjustAng.ToMat3() * viewWeaponAxis;
	} else {
		// calculate weapon position based on player movement bobbing
		owner->CalculateViewWeaponPos( viewWeaponOrigin, viewWeaponAxis );

		// hide offset is for dropping the gun when approaching a GUI or NPC
		// This is simpler to manage than doing the weapon put-away animation
		if ( gameLocal.time - hideStartTime < hideTime ) {		
			float frac = ( float )( gameLocal.time - hideStartTime ) / ( float )hideTime;
			if ( hideStart < hideEnd ) {
				frac = 1.0f - frac;
				frac = 1.0f - frac * frac;
			} else {
				frac = frac * frac;
			}
			hideOffset = hideStart + ( hideEnd - hideStart ) * frac;
		} else {
			hideOffset = hideEnd;
			if ( hide && disabled ) {
				Hide();
			}
		}
		viewWeaponOrigin += hideOffset * viewWeaponAxis[ 2 ];

		// kick up based on repeat firing
		MuzzleRise( viewWeaponOrigin, viewWeaponAxis );
	}

	// set the physics position and orientation
	GetPhysics()->SetOrigin( viewWeaponOrigin );
	GetPhysics()->SetAxis( viewWeaponAxis );
	UpdateVisuals();

	// update the weapon script
	UpdateScript();

	UpdateGUI();

	// update animation
	UpdateAnimation();

	// only show the surface in player view
	renderEntity.allowSurfaceInViewID = owner->entityNumber+1;

	// crunch the depth range so it never pokes into walls this breaks the machine gun gui
	renderEntity.weaponDepthHack = g_useWeaponDepthHack.GetBool();

	// present the model
	if ( showViewModel ) {
		Present();
	} else {
		FreeModelDef();
	}

	if ( worldModel.GetEntity() && worldModel.GetEntity()->GetRenderEntity() ) {
		// deal with the third-person visible world model
		// don't show shadows of the world model in first person
		if ( common->IsMultiplayer() || g_showPlayerShadow.GetBool() || pm_thirdPerson.GetBool() ) {
			worldModel.GetEntity()->GetRenderEntity()->suppressShadowInViewID	= 0;
		} else {
			worldModel.GetEntity()->GetRenderEntity()->suppressShadowInViewID	= owner->entityNumber+1;
			worldModel.GetEntity()->GetRenderEntity()->suppressShadowInLightID = LIGHTID_VIEW_MUZZLE_FLASH + owner->entityNumber;
		}
	}

	if ( nozzleFx ) {
		UpdateNozzleFx();
	}

	// muzzle smoke
	if ( showViewModel && !disabled && weaponSmoke && ( weaponSmokeStartTime != 0 ) ) {
		// use the barrel joint if available

		if(smokeJointView != INVALID_JOINT) {
			GetGlobalJointTransform( true, smokeJointView, muzzleOrigin, muzzleAxis );
		} else if (barrelJointView != INVALID_JOINT) {
			GetGlobalJointTransform( true, barrelJointView, muzzleOrigin, muzzleAxis );
		} else {
			// default to going straight out the view
			muzzleOrigin = playerViewOrigin;
			muzzleAxis = playerViewAxis;
		}
		// spit out a particle
		if ( !gameLocal.smokeParticles->EmitSmoke( weaponSmoke, weaponSmokeStartTime, gameLocal.random.RandomFloat(), muzzleOrigin, muzzleAxis, timeGroup /*_D3XP*/ ) ) {
			weaponSmokeStartTime = ( continuousSmoke ) ? gameLocal.time : 0;
		}
	}

	if ( showViewModel && strikeSmoke && strikeSmokeStartTime != 0 ) {
		// spit out a particle
		if ( !gameLocal.smokeParticles->EmitSmoke( strikeSmoke, strikeSmokeStartTime, gameLocal.random.RandomFloat(), strikePos, strikeAxis, timeGroup /*_D3XP*/ ) ) {
			strikeSmokeStartTime = 0;
		}
	}

	if ( showViewModel && !hide ) {

		for( int i = 0; i < weaponParticles.Num(); i++ ) {
			WeaponParticle_t* part = weaponParticles.GetIndex(i);

			if(part->active) {
				if(part->smoke) {
					if(part->joint != INVALID_JOINT) {
						GetGlobalJointTransform( true, part->joint, muzzleOrigin, muzzleAxis );
					} else {
						// default to going straight out the view
						muzzleOrigin = playerViewOrigin;
						muzzleAxis = playerViewAxis;
					}
					if ( !gameLocal.smokeParticles->EmitSmoke( part->particle, part->startTime, gameLocal.random.RandomFloat(), muzzleOrigin, muzzleAxis, timeGroup /*_D3XP*/ ) ) {
						part->active = false;	// all done
						part->startTime = 0;
					}
				} else {
					if ( part->emitter != NULL ) {
						//Manually update the position of the emitter so it follows the weapon
						renderEntity_t* rendEnt = part->emitter->GetRenderEntity();
						GetGlobalJointTransform( true, part->joint, rendEnt->origin, rendEnt->axis );

						if ( part->emitter->GetModelDefHandle() != -1 ) {
							gameRenderWorld->UpdateEntityDef( part->emitter->GetModelDefHandle(), rendEnt );
						}
					}
				}
			}
		}

		for(int i = 0; i < weaponLights.Num(); i++) {
			WeaponLight_t* light = weaponLights.GetIndex(i);

			if(light->active) {

				GetGlobalJointTransform( true, light->joint, light->light.origin, light->light.axis );
				if ( ( light->lightHandle != -1 ) ) {
					gameRenderWorld->UpdateLightDef( light->lightHandle, &light->light );
				} else {
					light->lightHandle = gameRenderWorld->AddLightDef( &light->light );
				}
			}
		}
	}

	// Update the grabber effects
	if ( grabberState != -1 ) {
		grabberState = grabber.Update( owner, hide );
	}

	// remove the muzzle flash light when it's done
	if ( ( !lightOn && ( gameLocal.time >= muzzleFlashEnd ) ) || IsHidden() ) {
		if ( muzzleFlashHandle != -1 ) {
			gameRenderWorld->FreeLightDef( muzzleFlashHandle );
			muzzleFlashHandle = -1;
		}
		if ( worldMuzzleFlashHandle != -1 ) {
			gameRenderWorld->FreeLightDef( worldMuzzleFlashHandle );
			worldMuzzleFlashHandle = -1;
		}
	}

	// update the muzzle flash light, so it moves with the gun
	if ( muzzleFlashHandle != -1 ) {
		UpdateFlashPosition();
		gameRenderWorld->UpdateLightDef( muzzleFlashHandle, &muzzleFlash );
		gameRenderWorld->UpdateLightDef( worldMuzzleFlashHandle, &worldMuzzleFlash );

		// wake up monsters with the flashlight
		if ( !common->IsMultiplayer() && lightOn && !owner->fl.notarget ) {
			AlertMonsters();
		}
	}

	// update the gui light
	if ( guiLight.lightRadius[0] && guiLightJointView != INVALID_JOINT ) {
		GetGlobalJointTransform( true, guiLightJointView, guiLight.origin, guiLight.axis );

		if ( ( guiLightHandle != -1 ) ) {
			gameRenderWorld->UpdateLightDef( guiLightHandle, &guiLight );
		} else {
			guiLightHandle = gameRenderWorld->AddLightDef( &guiLight );
		}
	}

	if ( status != WP_READY && sndHum ) {
		StopSound( SND_CHANNEL_BODY, false );
	}

	UpdateSound();

	// constant rumble...
	float highMagnitude = weaponDef->dict.GetFloat( "controllerConstantShakeHighMag" );
	int highDuration = weaponDef->dict.GetInt( "controllerConstantShakeHighTime" );
	float lowMagnitude = weaponDef->dict.GetFloat( "controllerConstantShakeLowMag" );
	int lowDuration = weaponDef->dict.GetInt( "controllerConstantShakeLowTime" );

	if( owner->IsLocallyControlled() ) {
		owner->SetControllerShake( highMagnitude, highDuration, lowMagnitude, lowDuration );
	}
}

/*
================
idWeapon::RemoveMuzzleFlashlight
================
*/
void idWeapon::RemoveMuzzleFlashlight() {
	if ( muzzleFlashHandle != -1 ) {
		gameRenderWorld->FreeLightDef( muzzleFlashHandle );
		muzzleFlashHandle = -1;
	}
	if ( worldMuzzleFlashHandle != -1 ) {
		gameRenderWorld->FreeLightDef( worldMuzzleFlashHandle );
		worldMuzzleFlashHandle = -1;
	}
}

/*
================
idWeapon::EnterCinematic
================
*/
void idWeapon::EnterCinematic() {
	StopSound( SND_CHANNEL_ANY, false );

	if ( isLinked ) {
		SetState( "EnterCinematic", 0 );
		thread->Execute();

		WEAPON_ATTACK		= false;
		WEAPON_RELOAD		= false;
		WEAPON_NETRELOAD	= false;
		WEAPON_NETENDRELOAD	= false;
		WEAPON_NETFIRING	= false;
		WEAPON_RAISEWEAPON	= false;
		WEAPON_LOWERWEAPON	= false;

		grabber.Update( this->GetOwner(), true );
	}

	disabled = true;

	LowerWeapon();
}

/*
================
idWeapon::ExitCinematic
================
*/
void idWeapon::ExitCinematic() {
	disabled = false;

	if ( isLinked ) {
		SetState( "ExitCinematic", 0 );
		thread->Execute();
	}

	RaiseWeapon();
}

/*
================
idWeapon::NetCatchup
================
*/
void idWeapon::NetCatchup() {
	if ( isLinked ) {
		SetState( "NetCatchup", 0 );
		thread->Execute();
	}
}

/*
================
idWeapon::GetZoomFov
================
*/
int	idWeapon::GetZoomFov() {
	return zoomFov;
}

/*
================
idWeapon::GetWeaponAngleOffsets
================
*/
void idWeapon::GetWeaponAngleOffsets( int *average, float *scale, float *max ) {
	*average = weaponAngleOffsetAverages;
	*scale = weaponAngleOffsetScale;
	*max = weaponAngleOffsetMax;
}

/*
================
idWeapon::GetWeaponTimeOffsets
================
*/
void idWeapon::GetWeaponTimeOffsets( float *time, float *scale ) {
	*time = weaponOffsetTime;
	*scale = weaponOffsetScale;
}


/***********************************************************************

	Ammo

***********************************************************************/

/*
================
idWeapon::GetAmmoNumForName
================
*/
ammo_t idWeapon::GetAmmoNumForName( const char *ammoname ) {
	int num;
	const idDict *ammoDict;

	assert( ammoname );

	ammoDict = gameLocal.FindEntityDefDict( "ammo_types", false );
	if ( ammoDict == NULL ) {
		gameLocal.Error( "Could not find entity definition for 'ammo_types'\n" );
		return 0;
	}

	if ( !ammoname[ 0 ] ) {
		return 0;
	}

	if ( !ammoDict->GetInt( ammoname, "-1", num ) ) {

	}

	if ( ( num < 0 ) || ( num >= AMMO_NUMTYPES ) ) {
		gameLocal.Warning( "Ammo type '%s' value out of range.  Maximum ammo types is %d.\n", ammoname, AMMO_NUMTYPES );
		num = 0;
	}

	return ( ammo_t )num;
}

/*
================
idWeapon::GetAmmoNameForNum
================
*/
const char *idWeapon::GetAmmoNameForNum( ammo_t ammonum ) {
	int i;
	int num;
	const idDict *ammoDict;
	const idKeyValue *kv;
	char text[ 32 ];

	ammoDict = gameLocal.FindEntityDefDict( "ammo_types", false );
	if ( ammoDict == NULL ) {
		gameLocal.Error( "Could not find entity definition for 'ammo_types'\n" );
		return NULL;
	}

	sprintf( text, "%d", ammonum );

	num = ammoDict->GetNumKeyVals();
	for( i = 0; i < num; i++ ) {
		kv = ammoDict->GetKeyVal( i );
		if ( kv->GetValue() == text ) {
			return kv->GetKey();
		}
	}

	return NULL;
}

/*
================
idWeapon::GetAmmoPickupNameForNum
================
*/
const char *idWeapon::GetAmmoPickupNameForNum( ammo_t ammonum ) {
	int i;
	int num;
	const idDict *ammoDict;
	const idKeyValue *kv;

	ammoDict = gameLocal.FindEntityDefDict( "ammo_names", false );
	if ( !ammoDict ) {
		gameLocal.Error( "Could not find entity definition for 'ammo_names'\n" );
	}

	const char *name = GetAmmoNameForNum( ammonum );

	if ( name != NULL && *name != NULL ) {
		num = ammoDict->GetNumKeyVals();
		for( i = 0; i < num; i++ ) {
			kv = ammoDict->GetKeyVal( i );
			if ( idStr::Icmp( kv->GetKey(), name) == 0 ) {
				return kv->GetValue();
			}
		}
	}

	return "";
}

/*
================
idWeapon::AmmoAvailable
================
*/
int idWeapon::AmmoAvailable() const {
	if ( owner ) {
		return owner->inventory.HasAmmo( ammoType, ammoRequired );
	} else {
		if ( g_infiniteAmmo.GetBool() ) {
			return 10;	// arbitrary number, just so whatever's calling thinks there's sufficient ammo...
		} else {
			return 0;
		}
	}
}

/*
================
idWeapon::AmmoInClip
================
*/
int idWeapon::AmmoInClip() const {
	return ammoClip.Get();
}

/*
================
idWeapon::ResetAmmoClip
================
*/
void idWeapon::ResetAmmoClip() {
	ammoClip = -1;
}

/*
================
idWeapon::GetAmmoType
================
*/
ammo_t idWeapon::GetAmmoType() const {
	return ammoType;
}

/*
================
idWeapon::ClipSize
================
*/
int	idWeapon::ClipSize() const {
	return clipSize;
}

/*
================
idWeapon::LowAmmo
================
*/
int	idWeapon::LowAmmo() const {
	return lowAmmo;
}

/*
================
idWeapon::AmmoRequired
================
*/
int	idWeapon::AmmoRequired() const {
	return ammoRequired;
}

/*
================
idWeapon::GetGrabberState

Returns the current grabberState
================
*/
int idWeapon::GetGrabberState() const {

	return grabberState;
}

/*
================
idWeapon::AmmoCount

Returns the total number of rounds regardless of the required ammo
================
*/
int idWeapon::AmmoCount() const {

	if ( owner ) {
		return owner->inventory.HasAmmo( ammoType, 1 );
	} else {
		return 0;
	}
}

/*
================
idWeapon::WriteToSnapshot
================
*/
void idWeapon::WriteToSnapshot( idBitMsg &msg ) const {
	msg.WriteBits( ammoClip.Get(), ASYNC_PLAYER_INV_CLIP_BITS );
	msg.WriteBits( worldModel.GetSpawnId(), 32 );
	msg.WriteBits( lightOn, 1 );
	msg.WriteBits( isFiring ? 1 : 0, 1 );
}

/*
================
idWeapon::ReadFromSnapshot
================
*/
void idWeapon::ReadFromSnapshot( const idBitMsg &msg ) {	
	const int snapshotAmmoClip = msg.ReadBits( ASYNC_PLAYER_INV_CLIP_BITS );
	worldModel.SetSpawnId( msg.ReadBits( 32 ) );
	const bool snapshotLightOn = msg.ReadBits( 1 ) != 0;
	isFiring = msg.ReadBits( 1 ) != 0;

	// Local clients predict the ammo in the clip. Only use the ammo cpunt from the snapshot for local clients
	// if the server has processed the same usercmd in which we predicted the ammo count.
	if ( owner != NULL ) {
		ammoClip.UpdateFromSnapshot( snapshotAmmoClip, owner->GetEntityNumber() );
	}

	// WEAPON_NETFIRING is only turned on for other clients we're predicting. not for local client
	if ( owner && !owner->IsLocallyControlled() && WEAPON_NETFIRING.IsLinked() ) {

		// immediately go to the firing state so we don't skip fire animations
		if ( !WEAPON_NETFIRING && isFiring ) {
			idealState = "Fire";
		}

        // immediately switch back to idle
        if ( WEAPON_NETFIRING && !isFiring ) {
            idealState = "Idle";
        }

		WEAPON_NETFIRING = isFiring;
		WEAPON_ATTACK = isFiring;
	}

	// Only update the flashlight state if it has changed, and if this isn't the local player.
	// The local player sets their flashlight immediately for responsiveness.
	if ( owner != NULL && !owner->IsLocallyControlled() && lightOn != snapshotLightOn ) {
		if ( snapshotLightOn ) {
			FlashlightOn();
		} else {
			FlashlightOff();
		}
	}
}

/*
================
idWeapon::ClientReceiveEvent
================
*/
bool idWeapon::ClientReceiveEvent( int event, int time, const idBitMsg &msg ) {

	switch( event ) {
		case EVENT_RELOAD: {
			// Local clients predict reloads, only process this event for remote clients.
			if ( owner != NULL && !owner->IsLocallyControlled() && ( gameLocal.time - time < 1000 ) ) {
				if ( WEAPON_NETRELOAD.IsLinked() ) {
					WEAPON_NETRELOAD = true;
					WEAPON_NETENDRELOAD = false;
				}
			}
			return true;
		}
		case EVENT_ENDRELOAD: {
			// Local clients predict reloads, only process this event for remote clients.
			if ( owner != NULL && !owner->IsLocallyControlled() && WEAPON_NETENDRELOAD.IsLinked() ) {
				WEAPON_NETENDRELOAD = true;
			}
			return true;
		}
		case EVENT_CHANGESKIN: {
			int index = gameLocal.ClientRemapDecl( DECL_SKIN, msg.ReadLong() );
			renderEntity.customSkin = ( index != -1 ) ? static_cast<const idDeclSkin *>( declManager->DeclByIndex( DECL_SKIN, index ) ) : NULL;
			UpdateVisuals();
			if ( worldModel.GetEntity() ) {
				worldModel.GetEntity()->SetSkin( renderEntity.customSkin );
			}
			return true;
		}
		default: {
			return idEntity::ClientReceiveEvent( event, time, msg );
		}
	}
}

/***********************************************************************

	Script events

***********************************************************************/

/*
===============
idWeapon::Event_Clear
===============
*/
void idWeapon::Event_Clear() {
	Clear();
}

/*
===============
idWeapon::Event_GetOwner
===============
*/
void idWeapon::Event_GetOwner() {
	idThread::ReturnEntity( owner );
}

/*
===============
idWeapon::Event_WeaponState
===============
*/
void idWeapon::Event_WeaponState( const char *statename, int blendFrames ) {
	const function_t *func;

	func = scriptObject.GetFunction( statename );
	if ( !func ) {
		assert( 0 );
		gameLocal.Error( "Can't find function '%s' in object '%s'", statename, scriptObject.GetTypeName() );
	}

	idealState = statename;

	// HACK, Fixes reload animation on player not playing on second reload ( on non local client players, and only with host viewing. )
	if( common->IsMultiplayer() && strcmp( weaponDef->GetName(), "weapon_shotgun_double_mp" ) == 0 ) {
		if( strcmp( statename, "Reload" ) != 0 ) {
			if( status ==  WP_RELOAD ) {
				status =  WP_READY;
			}
		}
	}

	if ( !idealState.Icmp( "Fire" ) ) {
		isFiring = true;
	} else {
		isFiring = false;
	}

	animBlendFrames = blendFrames;
	thread->DoneProcessing();
}

/*
===============
idWeapon::Event_WeaponReady
===============
*/
void idWeapon::Event_WeaponReady() {
	status = WP_READY;
	idLib::PrintfIf( g_debugWeapon.GetBool(), "Weapon Status WP_READY \n" );
	if ( isLinked ) {
		WEAPON_RAISEWEAPON = false;
	}
	if ( sndHum ) {
		StartSoundShader( sndHum, SND_CHANNEL_BODY, 0, false, NULL );
	}

}

/*
===============
idWeapon::Event_WeaponOutOfAmmo
===============
*/
void idWeapon::Event_WeaponOutOfAmmo() {
	status = WP_OUTOFAMMO;
	idLib::PrintfIf( g_debugWeapon.GetBool(), "Weapon Status WP_OUTOFAMMO \n" );
	if ( isLinked ) {
		WEAPON_RAISEWEAPON = false;
	}
}

/*
===============
idWeapon::Event_WeaponReloading
===============
*/
void idWeapon::Event_WeaponReloading() {
	status = WP_RELOAD;
	idLib::PrintfIf( g_debugWeapon.GetBool(), "Weapon Status WP_RELOAD \n" );
}

/*
===============
idWeapon::Event_WeaponHolstered
===============
*/
void idWeapon::Event_WeaponHolstered() {
	status = WP_HOLSTERED;
	idLib::PrintfIf( g_debugWeapon.GetBool(), "Weapon Status WP_HOLSTERED \n" );
	if ( isLinked ) {
		WEAPON_LOWERWEAPON = false;
	}
}

/*
===============
idWeapon::Event_WeaponRising
===============
*/
void idWeapon::Event_WeaponRising() {
	status = WP_RISING;
	idLib::PrintfIf( g_debugWeapon.GetBool(), "Weapon Status WP_RISING \n" );
	if ( isLinked ) {
		WEAPON_LOWERWEAPON = false;
	}
	owner->WeaponRisingCallback();
}

/*
===============
idWeapon::Event_WeaponLowering
===============
*/
void idWeapon::Event_WeaponLowering() {
	status = WP_LOWERING;
	idLib::PrintfIf( g_debugWeapon.GetBool(), "Weapon Status WP_LOWERING \n" );
	if ( isLinked ) {
		WEAPON_RAISEWEAPON = false;
	}
	owner->WeaponLoweringCallback();
}

/*
===============
idWeapon::Event_UseAmmo
===============
*/
void idWeapon::Event_UseAmmo( int amount ) {
	if ( owner == NULL || ( common->IsClient() && !owner->IsLocallyControlled() ) ) {
		return;
	}

	owner->inventory.UseAmmo( ammoType, ( powerAmmo ) ? amount : ( amount * ammoRequired ) );
	if ( clipSize && ammoRequired ) {
		ammoClip -= powerAmmo ? amount : ( amount * ammoRequired );
		if ( ammoClip.Get() < 0 ) {
			ammoClip = 0;
		}
	}
}

/*
===============
idWeapon::Event_AddToClip
===============
*/
void idWeapon::Event_AddToClip( int amount ) {
	int ammoAvail;

	if ( owner == NULL || ( common->IsClient() && !owner->IsLocallyControlled() ) ) {
		return;
	}

	int oldAmmo = ammoClip.Get();
	ammoAvail = owner->inventory.HasAmmo( ammoType, ammoRequired ) + AmmoInClip();

	ammoClip += amount;
	if ( ammoClip.Get() > clipSize ) {
		ammoClip = clipSize;
	}


	if ( ammoClip.Get() > ammoAvail ) {
		ammoClip = ammoAvail;
	}

	// for shared ammo we need to use the ammo when it is moved into the clip
	int usedAmmo = ammoClip.Get() - oldAmmo;
	owner->inventory.UseAmmo(ammoType, usedAmmo);
}

/*
===============
idWeapon::Event_AmmoInClip
===============
*/
void idWeapon::Event_AmmoInClip() {
	int ammo = AmmoInClip();
	idThread::ReturnFloat( ammo );	
}

/*
===============
idWeapon::Event_AmmoAvailable
===============
*/
void idWeapon::Event_AmmoAvailable() {
	int ammoAvail = owner->inventory.HasAmmo( ammoType, ammoRequired );
	ammoAvail += AmmoInClip();

	idThread::ReturnFloat( ammoAvail );
}

/*
===============
idWeapon::Event_TotalAmmoCount
===============
*/
void idWeapon::Event_TotalAmmoCount() {
	int ammoAvail = owner->inventory.HasAmmo( ammoType, 1 );
	idThread::ReturnFloat( ammoAvail );
}

/*
===============
idWeapon::Event_ClipSize
===============
*/
void idWeapon::Event_ClipSize() {
	idThread::ReturnFloat( clipSize );	
}

/*
===============
idWeapon::Event_AutoReload
===============
*/
void idWeapon::Event_AutoReload() {
	assert( owner );
	
	if ( common->IsClient() && owner != NULL && !owner->IsLocallyControlled() ) {
		idThread::ReturnFloat( 0.0f );
		return;
	}

	idLobbyBase & lobby = session->GetActingGameStateLobbyBase();
	lobbyUserID_t & lobbyUserID = gameLocal.lobbyUserIDs[owner->entityNumber];
	idThread::ReturnFloat( lobby.GetLobbyUserWeaponAutoReload( lobbyUserID ) );
}

/*
===============
idWeapon::Event_NetReload
===============
*/
void idWeapon::Event_NetReload() {
	assert( owner );
	if ( common->IsServer() ) {
		ServerSendEvent( EVENT_RELOAD, NULL, false );
	}
}

/*
===============
idWeapon::Event_NetEndReload
===============
*/
void idWeapon::Event_NetEndReload() {
	assert( owner );
	if ( common->IsServer() ) {
		ServerSendEvent( EVENT_ENDRELOAD, NULL, false );
	}
}

/*
===============
idWeapon::Event_PlayAnim
===============
*/
void idWeapon::Event_PlayAnim( int channel, const char *animname ) {
	int anim;
	
	anim = animator.GetAnim( animname );
	if ( !anim ) {
		gameLocal.Warning( "missing '%s' animation on '%s' (%s)", animname, name.c_str(), GetEntityDefName() );
		animator.Clear( channel, gameLocal.time, FRAME2MS( animBlendFrames ) );
		animDoneTime = 0;
	} else {
		if ( !( owner && owner->GetInfluenceLevel() ) ) {
			Show();
		}
		animator.PlayAnim( channel, anim, gameLocal.time, FRAME2MS( animBlendFrames ) );
		animDoneTime = animator.CurrentAnim( channel )->GetEndTime();
		if ( worldModel.GetEntity() ) {
			anim = worldModel.GetEntity()->GetAnimator()->GetAnim( animname );
			if ( anim ) {
				worldModel.GetEntity()->GetAnimator()->PlayAnim( channel, anim, gameLocal.time, FRAME2MS( animBlendFrames ) );
			}
		}
	}
	animBlendFrames = 0;
	idThread::ReturnInt( 0 );
}

/*
===============
idWeapon::Event_PlayCycle
===============
*/
void idWeapon::Event_PlayCycle( int channel, const char *animname ) {
	int anim;

	anim = animator.GetAnim( animname );
	if ( !anim ) {
		gameLocal.Warning( "missing '%s' animation on '%s' (%s)", animname, name.c_str(), GetEntityDefName() );
		animator.Clear( channel, gameLocal.time, FRAME2MS( animBlendFrames ) );
		animDoneTime = 0;
	} else {
		if ( !( owner && owner->GetInfluenceLevel() ) ) {
			Show();
		}
		animator.CycleAnim( channel, anim, gameLocal.time, FRAME2MS( animBlendFrames ) );
		animDoneTime = animator.CurrentAnim( channel )->GetEndTime();
		if ( worldModel.GetEntity() ) {
			anim = worldModel.GetEntity()->GetAnimator()->GetAnim( animname );
			worldModel.GetEntity()->GetAnimator()->CycleAnim( channel, anim, gameLocal.time, FRAME2MS( animBlendFrames ) );
		}
	}
	animBlendFrames = 0;
	idThread::ReturnInt( 0 );
}

/*
===============
idWeapon::Event_AnimDone
===============
*/
void idWeapon::Event_AnimDone( int channel, int blendFrames ) {
	if ( animDoneTime - FRAME2MS( blendFrames ) <= gameLocal.time ) {
		idThread::ReturnInt( true );
	} else {
		idThread::ReturnInt( false );
	}
}

/*
===============
idWeapon::Event_SetBlendFrames
===============
*/
void idWeapon::Event_SetBlendFrames( int channel, int blendFrames ) {
	animBlendFrames = blendFrames;
}

/*
===============
idWeapon::Event_GetBlendFrames
===============
*/
void idWeapon::Event_GetBlendFrames( int channel ) {
	idThread::ReturnInt( animBlendFrames );
}

/*
================
idWeapon::Event_Next
================
*/
void idWeapon::Event_Next() {
	// change to another weapon if possible
	owner->NextBestWeapon();
}

/*
================
idWeapon::Event_SetSkin
================
*/
void idWeapon::Event_SetSkin( const char *skinname ) {
	const idDeclSkin *skinDecl = NULL;
	if ( !skinname || !skinname[ 0 ] ) {
		skinDecl = NULL;
	} else {
		skinDecl = declManager->FindSkin( skinname );
	}

	// Don't update if the skin hasn't changed.
	if ( renderEntity.customSkin == skinDecl && worldModel.GetEntity() != NULL && worldModel.GetEntity()->GetSkin() == skinDecl ) {
		return;
	}

	renderEntity.customSkin = skinDecl;
	UpdateVisuals();

	if ( worldModel.GetEntity() ) {
		worldModel.GetEntity()->SetSkin( skinDecl );
	}

	// Hack, don't send message if flashlight, because clients process the flashlight instantly.
	if ( common->IsServer() && !isPlayerFlashlight ) {
		idBitMsg			msg;
		byte				msgBuf[MAX_EVENT_PARAM_SIZE];

		msg.InitWrite( msgBuf, sizeof( msgBuf ) );
		msg.WriteLong( ( skinDecl != NULL ) ? gameLocal.ServerRemapDecl( -1, DECL_SKIN, skinDecl->Index() ) : -1 );
		ServerSendEvent( EVENT_CHANGESKIN, &msg, false );
	}
}

/*
================
idWeapon::Event_Flashlight
================
*/
void idWeapon::Event_Flashlight( int enable ) {
	if ( enable ) {
		lightOn = true;
		MuzzleFlashLight();
	} else {
		lightOn = false;
		muzzleFlashEnd = 0;
	}
}

/*
================
idWeapon::Event_GetLightParm
================
*/
void idWeapon::Event_GetLightParm( int parmnum ) {
	if ( ( parmnum < 0 ) || ( parmnum >= MAX_ENTITY_SHADER_PARMS ) ) {
		gameLocal.Error( "shader parm index (%d) out of range", parmnum );
		return;
	}

	idThread::ReturnFloat( muzzleFlash.shaderParms[ parmnum ] );
}

/*
================
idWeapon::Event_SetLightParm
================
*/
void idWeapon::Event_SetLightParm( int parmnum, float value ) {
	if ( ( parmnum < 0 ) || ( parmnum >= MAX_ENTITY_SHADER_PARMS ) ) {
		gameLocal.Error( "shader parm index (%d) out of range", parmnum );
		return;
	}

	muzzleFlash.shaderParms[ parmnum ]		= value;
	worldMuzzleFlash.shaderParms[ parmnum ]	= value;
	UpdateVisuals();
}

/*
================
idWeapon::Event_SetLightParms
================
*/
void idWeapon::Event_SetLightParms( float parm0, float parm1, float parm2, float parm3 ) {
	muzzleFlash.shaderParms[ SHADERPARM_RED ]			= parm0;
	muzzleFlash.shaderParms[ SHADERPARM_GREEN ]			= parm1;
	muzzleFlash.shaderParms[ SHADERPARM_BLUE ]			= parm2;
	muzzleFlash.shaderParms[ SHADERPARM_ALPHA ]			= parm3;

	worldMuzzleFlash.shaderParms[ SHADERPARM_RED ]		= parm0;
	worldMuzzleFlash.shaderParms[ SHADERPARM_GREEN ]	= parm1;
	worldMuzzleFlash.shaderParms[ SHADERPARM_BLUE ]		= parm2;
	worldMuzzleFlash.shaderParms[ SHADERPARM_ALPHA ]	= parm3;

	UpdateVisuals();
}

/*
================
idWeapon::Event_Grabber
================
*/
void idWeapon::Event_Grabber( int enable ) {
	if ( enable ) {
		grabberState = 0;
	} else {
		grabberState = -1;
	}
}

/*
================
idWeapon::Event_GrabberHasTarget
================
*/
void idWeapon::Event_GrabberHasTarget() {
	idThread::ReturnInt( grabberState );
}

/*
================
idWeapon::Event_GrabberSetGrabDistance
================
*/
void idWeapon::Event_GrabberSetGrabDistance( float dist ) {

	grabber.SetDragDistance( dist );
}

/*
================
idWeapon::Event_CreateProjectile
================
*/
void idWeapon::Event_CreateProjectile() {
	if ( !common->IsClient() ) {
		projectileEnt = NULL;
		gameLocal.SpawnEntityDef( projectileDict, &projectileEnt, false );
		if ( projectileEnt ) {
			projectileEnt->SetOrigin( GetPhysics()->GetOrigin() );
			projectileEnt->Bind( owner, false );
			projectileEnt->Hide();
		}
		idThread::ReturnEntity( projectileEnt );
	} else {
		idThread::ReturnEntity( NULL );
	}
}

/*
================
idWeapon::GetProjectileLaunchOriginAndAxis
================
*/
void idWeapon::GetProjectileLaunchOriginAndAxis( idVec3 & origin, idMat3 & axis ) {
	assert( owner != NULL );
	
	// calculate the muzzle position
	if ( barrelJointView != INVALID_JOINT && projectileDict.GetBool( "launchFromBarrel" ) ) {
		// there is an explicit joint for the muzzle
		// GetGlobalJointTransform( true, barrelJointView, muzzleOrigin, muzzleAxis );
		GetMuzzlePositionWithHacks( origin, axis );
	} else {
		// go straight out of the view
		origin = playerViewOrigin;
		axis = playerViewAxis;
	}

	axis = playerViewAxis;	// Fix for plasma rifle not firing correctly on initial shot of a burst fire
}

/*
================
idWeapon::Event_LaunchProjectiles
================
*/
void idWeapon::Event_LaunchProjectiles( int num_projectiles, float spread, float fuseOffset, float launchPower, float dmgPower ) {
	idProjectile	*proj;
	idEntity		*ent;
	int				i;
	idVec3			dir;
	float			ang;
	float			spin;
	float			distance;
	trace_t			tr;
	idVec3			start;
	idVec3			muzzle_pos;
	idBounds		ownerBounds, projBounds;

	assert( owner != NULL );

	if ( IsHidden() ) {
		return;
	}

	if ( !projectileDict.GetNumKeyVals() ) {
		const char *classname = weaponDef->dict.GetString( "classname" );
		gameLocal.Warning( "No projectile defined on '%s'", classname );
		return;
	}

	// Predict clip ammo on locally controlled MP clients.
	if ( common->IsServer() || owner->IsLocallyControlled() ) {
		if ( ( clipSize != 0 ) && ( ammoClip.Get() <= 0 ) ) {
			return;
		}

		// if this is a power ammo weapon ( currently only the bfg ) then make sure 
		// we only fire as much power as available in each clip
		if ( powerAmmo ) {
			// power comes in as a float from zero to max
			// if we use this on more than the bfg will need to define the max
			// in the .def as opposed to just in the script so proper calcs
			// can be done here. 
			dmgPower = ( int )dmgPower + 1;
			if ( dmgPower > ammoClip.Get() ) {
				dmgPower = ammoClip.Get();
			}
		}

		if(clipSize == 0) {
			//Weapons with a clip size of 0 launch straight from inventory without moving to a clip

			//In D3XP we used the ammo when the ammo was moved into the clip so we don't want to
			//use it now.
			owner->inventory.UseAmmo( ammoType, ( powerAmmo ) ? dmgPower : ammoRequired );
		}

		if ( clipSize && ammoRequired && !g_infiniteAmmo.GetBool() ) {
			ammoClip -= powerAmmo ? dmgPower : ammoRequired;
		}
	}

	if ( !silent_fire ) {
		// wake up nearby monsters
		gameLocal.AlertAI( owner );
	}

	// set the shader parm to the time of last projectile firing,
	// which the gun material shaders can reference for single shot barrel glows, etc
	renderEntity.shaderParms[ SHADERPARM_DIVERSITY ]	= gameLocal.random.CRandomFloat();
	renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ]	= -MS2SEC( gameLocal.realClientTime );

	if ( worldModel.GetEntity() ) {
		worldModel.GetEntity()->SetShaderParm( SHADERPARM_DIVERSITY, renderEntity.shaderParms[ SHADERPARM_DIVERSITY ] );
		worldModel.GetEntity()->SetShaderParm( SHADERPARM_TIMEOFFSET, renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ] );
	}

	// calculate the muzzle position
	GetProjectileLaunchOriginAndAxis( muzzleOrigin, muzzleAxis );

	// add some to the kick time, incrementally moving repeat firing weapons back
	if ( kick_endtime < gameLocal.realClientTime ) {
		kick_endtime = gameLocal.realClientTime;
	}
	kick_endtime += muzzle_kick_time;
	if ( kick_endtime > gameLocal.realClientTime + muzzle_kick_maxtime ) {
		kick_endtime = gameLocal.realClientTime + muzzle_kick_maxtime;
	}

	// "Predict" damage effects on clients by just spawning a local projectile that deals no damage. Used only
	// for sound & visual effects. Damage will be handled through reliable messages to the host.
	const bool isHitscan = projectileDict.GetBool( "net_instanthit" );
	const bool attackerIsLocal = owner->IsLocallyControlled();
	const bool actuallySpawnProjectile = common->IsServer() || attackerIsLocal || isHitscan;

	if ( actuallySpawnProjectile ) {
		
		ownerBounds = owner->GetPhysics()->GetAbsBounds();

		owner->AddProjectilesFired( num_projectiles );

		float spreadRad = DEG2RAD( spread );
		for( i = 0; i < num_projectiles; i++ ) {
			ang = idMath::Sin( spreadRad * gameLocal.random.RandomFloat() );
			spin = (float)DEG2RAD( 360.0f ) * gameLocal.random.RandomFloat();
			dir = muzzleAxis[ 0 ] + muzzleAxis[ 2 ] * ( ang * idMath::Sin( spin ) ) - muzzleAxis[ 1 ] * ( ang * idMath::Cos( spin ) );
			dir.Normalize();

			if ( projectileEnt ) {
				ent = projectileEnt;
				ent->Show();
				ent->Unbind();
				projectileEnt = NULL;
			} else {
				if ( common->IsClient() ) {
					// This is predicted on a client, don't replicate.
					// Must be set before spawn, so that the entity can be spawned into the correct area of the entities array.
					projectileDict.SetBool( "net_skip_replication", true );
				} else {
					projectileDict.SetBool( "net_skip_replication", false );
				}
				gameLocal.SpawnEntityDef( projectileDict, &ent, false );
			}

			if ( ent == NULL || !ent->IsType( idProjectile::Type ) ) {
				const char *projectileName = weaponDef->dict.GetString( "def_projectile" );
				gameLocal.Error( "'%s' is not an idProjectile", projectileName );
				return;
			}

			int predictedKey = idEntity::INVALID_PREDICTION_KEY;

			if ( projectileDict.GetBool( "net_instanthit" ) ) {
				// don't synchronize this on top of the already predicted effect
				ent->fl.networkSync = false;
			} else if ( owner != NULL ) {
				// Set the prediction key only for non-instanthit projectiles. 
				if ( common->IsClient() ) {
					owner->IncrementFireCount();	
				}
				predictedKey = gameLocal.GeneratePredictionKey( this, owner, -1 );
				ent->SetPredictedKey( predictedKey );
			}

			proj = static_cast<idProjectile *>(ent);
			proj->Create( owner, muzzleOrigin, dir );

			projBounds = proj->GetPhysics()->GetBounds().Rotate( proj->GetPhysics()->GetAxis() );

			// make sure the projectile starts inside the bounding box of the owner
			if ( i == 0 ) {
				muzzle_pos = muzzleOrigin + muzzleAxis[ 0 ] * 2.0f;
				if ( ( ownerBounds - projBounds).RayIntersection( muzzle_pos, muzzleAxis[0], distance ) ) {
					start = muzzle_pos + distance * muzzleAxis[0];
				} else {
					start = ownerBounds.GetCenter();
				}
				gameLocal.clip.Translation( tr, start, muzzle_pos, proj->GetPhysics()->GetClipModel(), proj->GetPhysics()->GetClipModel()->GetAxis(), MASK_SHOT_RENDERMODEL, owner );
				muzzle_pos = tr.endpos;
			}

			// If this is the server simulating a remote client, the client has spawned the projectile in the past.
			// The server will catch-up the projectile so that its position will be as if the projectile had spawned
			// when the client fired it.
			if ( common->IsServer() && owner != NULL && !owner->IsLocallyControlled() && !projectileDict.GetBool( "net_instanthit" ) ) {
				int serverTimeOnClient = owner->usercmd.serverGameMilliseconds;

				int delta = idMath::ClampInt( 0, cg_projectile_clientAuthoritative_maxCatchup.GetInteger(), gameLocal.GetServerGameTimeMs() - serverTimeOnClient );

				int startTime = gameLocal.GetServerGameTimeMs() - delta;

				proj->Launch( muzzle_pos, dir, pushVelocity, fuseOffset, launchPower, dmgPower );

				// predictively spawned, but no need to simulate - was needed for correct processing of client mines when spawned on the server (because we're futzing with the clip models)
				proj->QueueToSimulate( startTime );

				if ( cg_predictedSpawn_debug.GetBool() ) {
					idLib::Printf( "Spawning throw item projectile for player %d. PredictiveKey: %d \n", owner->GetEntityNumber(), predictedKey );
				}
			} else {
				// Normal launch
				proj->Launch( muzzle_pos, dir, pushVelocity, fuseOffset, launchPower, dmgPower );
			}
		}

		// toss the brass
		if ( brassDelay >= 0 ) {
			PostEventMS( &EV_Weapon_EjectBrass, brassDelay );
		}
	}

	// add the light for the muzzleflash
	if ( !lightOn ) {
		MuzzleFlashLight();
	}

	owner->WeaponFireFeedback( &weaponDef->dict );

	// reset muzzle smoke
	weaponSmokeStartTime = gameLocal.realClientTime;
}

/*
================
idWeapon::Event_LaunchProjectilesEllipse
================
*/
void idWeapon::Event_LaunchProjectilesEllipse( int num_projectiles, float spreada, float spreadb, float fuseOffset, float power ) {
	idProjectile	*proj;
	idEntity		*ent;
	int				i;
	idVec3			dir;
	float			anga, angb;
	float			spin;
	float			distance;
	trace_t			tr;
	idVec3			start;
	idVec3			muzzle_pos;
	idBounds		ownerBounds, projBounds;

	if ( IsHidden() ) {
		return;
	}

	if ( !projectileDict.GetNumKeyVals() ) {
		const char *classname = weaponDef->dict.GetString( "classname" );
		gameLocal.Warning( "No projectile defined on '%s'", classname );
		return;
	}

	// avoid all ammo considerations on a client
	if ( !common->IsClient() ) {
		if ( ( clipSize != 0 ) && ( ammoClip.Get() <= 0 ) ) {
			return;
		}

		if( clipSize == 0 ) {
			//Weapons with a clip size of 0 launch strait from inventory without moving to a clip
			owner->inventory.UseAmmo( ammoType, ammoRequired );
		}

		if ( clipSize && ammoRequired ) {
			ammoClip -= ammoRequired;
		}

		if ( !silent_fire ) {
			// wake up nearby monsters
			gameLocal.AlertAI( owner );
		}

	}

	// set the shader parm to the time of last projectile firing,
	// which the gun material shaders can reference for single shot barrel glows, etc
	renderEntity.shaderParms[ SHADERPARM_DIVERSITY ]	= gameLocal.random.CRandomFloat();
	renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ]	= -MS2SEC( gameLocal.time );

	if ( worldModel.GetEntity() ) {
		worldModel.GetEntity()->SetShaderParm( SHADERPARM_DIVERSITY, renderEntity.shaderParms[ SHADERPARM_DIVERSITY ] );
		worldModel.GetEntity()->SetShaderParm( SHADERPARM_TIMEOFFSET, renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ] );
	}

	// calculate the muzzle position
	if ( barrelJointView != INVALID_JOINT && projectileDict.GetBool( "launchFromBarrel" ) ) {
		// there is an explicit joint for the muzzle
		GetGlobalJointTransform( true, barrelJointView, muzzleOrigin, muzzleAxis );
	} else {
		// go straight out of the view
		muzzleOrigin = playerViewOrigin;
		muzzleAxis = playerViewAxis;
	}

	// add some to the kick time, incrementally moving repeat firing weapons back
	if ( kick_endtime < gameLocal.time ) {
		kick_endtime = gameLocal.time;
	}
	kick_endtime += muzzle_kick_time;
	if ( kick_endtime > gameLocal.time + muzzle_kick_maxtime ) {
		kick_endtime = gameLocal.time + muzzle_kick_maxtime;
	}

	if ( !common->IsClient() ) {
		ownerBounds = owner->GetPhysics()->GetAbsBounds();

		owner->AddProjectilesFired( num_projectiles );

		float spreadRadA = DEG2RAD( spreada );
		float spreadRadB = DEG2RAD( spreadb );

		for( i = 0; i < num_projectiles; i++ ) {
			//Ellipse Form
			spin = (float)DEG2RAD( 360.0f ) * gameLocal.random.RandomFloat();
			anga = idMath::Sin(spreadRadA * gameLocal.random.RandomFloat());
			angb = idMath::Sin(spreadRadB * gameLocal.random.RandomFloat());
			dir = playerViewAxis[ 0 ] + playerViewAxis[ 2 ] * ( angb*idMath::Sin( spin ) ) - playerViewAxis[ 1 ] * ( anga*idMath::Cos( spin ) );
			dir.Normalize();

			gameLocal.SpawnEntityDef( projectileDict, &ent );
			if ( ent == NULL || !ent->IsType( idProjectile::Type ) ) {
				const char *projectileName = weaponDef->dict.GetString( "def_projectile" );
				gameLocal.Error( "'%s' is not an idProjectile", projectileName );
				return;
			}

			proj = static_cast<idProjectile *>(ent);
			proj->Create( owner, muzzleOrigin, dir );

			projBounds = proj->GetPhysics()->GetBounds().Rotate( proj->GetPhysics()->GetAxis() );

			// make sure the projectile starts inside the bounding box of the owner
			if ( i == 0 ) {
				muzzle_pos = muzzleOrigin + playerViewAxis[ 0 ] * 2.0f;
				if ( ( ownerBounds - projBounds).RayIntersection( muzzle_pos, playerViewAxis[0], distance ) ) {
					start = muzzle_pos + distance * playerViewAxis[0];
				}
				else {
					start = ownerBounds.GetCenter();
				}
				gameLocal.clip.Translation( tr, start, muzzle_pos, proj->GetPhysics()->GetClipModel(), proj->GetPhysics()->GetClipModel()->GetAxis(), MASK_SHOT_RENDERMODEL, owner );
				muzzle_pos = tr.endpos;
			}

			proj->Launch( muzzle_pos, dir, pushVelocity, fuseOffset, power );
		}

		// toss the brass
		if( brassDelay >= 0 ) {
			PostEventMS( &EV_Weapon_EjectBrass, brassDelay );
		}
	}

	// add the light for the muzzleflash
	if ( !lightOn ) {
		MuzzleFlashLight();
	}

	owner->WeaponFireFeedback( &weaponDef->dict );

	// reset muzzle smoke
	weaponSmokeStartTime = gameLocal.time;

}

/** 
* Gives the player a powerup as if it were a weapon shot. It will use the ammo amount specified
* as ammoRequired.
*/
void idWeapon::Event_LaunchPowerup( const char* powerup, float duration, int useAmmo ) {

	if ( IsHidden() ) {
		return; 
	}

	// check if we're out of ammo
	if(useAmmo) {
		int ammoAvail = owner->inventory.HasAmmo( ammoType, ammoRequired );
		if ( !ammoAvail ) {
			return;
		}
		owner->inventory.UseAmmo( ammoType, ammoRequired );
	}

	// set the shader parm to the time of last projectile firing,
	// which the gun material shaders can reference for single shot barrel glows, etc
	renderEntity.shaderParms[ SHADERPARM_DIVERSITY ]	= gameLocal.random.CRandomFloat();
	renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ]	= -MS2SEC( gameLocal.time );

	if ( worldModel.GetEntity() ) {
		worldModel.GetEntity()->SetShaderParm( SHADERPARM_DIVERSITY, renderEntity.shaderParms[ SHADERPARM_DIVERSITY ] );
		worldModel.GetEntity()->SetShaderParm( SHADERPARM_TIMEOFFSET, renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ] );
	}

	// add the light for the muzzleflash
	if ( !lightOn ) {
		MuzzleFlashLight();
	}

	owner->Give(powerup, va("%f", duration), ITEM_GIVE_FEEDBACK | ITEM_GIVE_UPDATE_STATE | ITEM_GIVE_FROM_WEAPON );


}

void idWeapon::Event_StartWeaponSmoke() {

	// reset muzzle smoke
	weaponSmokeStartTime = gameLocal.time;
}

void idWeapon::Event_StopWeaponSmoke() {

	// reset muzzle smoke
	weaponSmokeStartTime = 0;
}

void idWeapon::Event_StartWeaponParticle( const char* name) {
	WeaponParticle_t* part;
	weaponParticles.Get(name, &part);
	if(part) {
		part->active = true;
		part->startTime = gameLocal.time;

		//Toggle the emitter
		if( !part->smoke && part->emitter != NULL ) {
			part->emitter->Show();
			part->emitter->PostEventMS(&EV_Activate, 0, this);
		}
	}
}

void idWeapon::Event_StopWeaponParticle( const char* name) {
	WeaponParticle_t* part;
	weaponParticles.Get(name, &part);
	if(part) {
		part->active = false;
		part->startTime = 0;

		//Toggle the emitter
		if(!part->smoke) {
			if ( part->emitter != NULL ) {
				part->emitter->Hide();
				part->emitter->PostEventMS(&EV_Activate, 0, this);
			}
		}
	}
}

void idWeapon::Event_StartWeaponLight( const char* name) {
	WeaponLight_t* light;
	weaponLights.Get(name, &light);
	if(light) {
		light->active = true;
		light->startTime = gameLocal.time;
	}
}

void idWeapon::Event_StopWeaponLight( const char* name) {
	WeaponLight_t* light;
	weaponLights.Get(name, &light);
	if(light) {
		light->active = false;
		light->startTime = 0;
		if(light->lightHandle != -1) {
			gameRenderWorld->FreeLightDef( light->lightHandle );
			light->lightHandle = -1;
		}
	}
}
/*
=====================
idWeapon::Event_Melee
=====================
*/
void idWeapon::Event_Melee() {
	idEntity	*ent;
	trace_t		tr;

	if ( weaponDef == NULL ) {
		gameLocal.Error( "No weaponDef on '%s'", this->GetName() );
		return;
	}

	if ( meleeDef == NULL ) {
		gameLocal.Error( "No meleeDef on '%s'", weaponDef->dict.GetString( "classname" ) );
		return;
	}

	if ( !common->IsClient() ) {
		idVec3 start = playerViewOrigin;
		idVec3 end = start + playerViewAxis[0] * ( meleeDistance * owner->PowerUpModifier( MELEE_DISTANCE ) );
		gameLocal.clip.TracePoint( tr, start, end, MASK_SHOT_RENDERMODEL, owner );
		if ( tr.fraction < 1.0f ) {
			ent = gameLocal.GetTraceEntity( tr );
		} else {
			ent = NULL;
		}

		if ( g_debugWeapon.GetBool() ) {
			gameRenderWorld->DebugLine( colorYellow, start, end, 100 );
			if ( ent != NULL ) {
				gameRenderWorld->DebugBounds( colorRed, ent->GetPhysics()->GetBounds(), ent->GetPhysics()->GetOrigin(), 100 );
			}
		}

		bool hit = false;
		const char *hitSound = meleeDef->dict.GetString( "snd_miss" );

		if ( ent != NULL ) {

			float push = meleeDef->dict.GetFloat( "push" );
			idVec3 impulse = -push * owner->PowerUpModifier( SPEED ) * tr.c.normal;

			if ( gameLocal.world->spawnArgs.GetBool( "no_Weapons" ) && ( ent->IsType( idActor::Type ) || ent->IsType( idAFAttachment::Type) ) ) {
				idThread::ReturnInt( 0 );
				return;
			}

			ent->ApplyImpulse( this, tr.c.id, tr.c.point, impulse );

			// weapon stealing - do this before damaging so weapons are not dropped twice
			if ( common->IsMultiplayer()
				&& weaponDef->dict.GetBool( "stealing" )
				&& ent->IsType( idPlayer::Type )
				&& !owner->PowerUpActive( BERSERK )
				&& ( (gameLocal.gameType != GAME_TDM ) || gameLocal.serverInfo.GetBool( "si_teamDamage" ) || ( owner->team != static_cast< idPlayer * >( ent )->team ) )
				) {
                
                if ( !gameLocal.mpGame.IsGametypeFlagBased() ) {
					owner->StealWeapon( static_cast< idPlayer * >( ent ) );
				}
			}

			if ( ent->fl.takedamage ) {
				idVec3 kickDir, globalKickDir;
				meleeDef->dict.GetVector( "kickDir", "0 0 0", kickDir );
				globalKickDir = muzzleAxis * kickDir;
				//Adjust the melee powerup modifier for the invulnerability boss fight
				float mod = owner->PowerUpModifier( MELEE_DAMAGE );
				if(!strcmp(ent->GetEntityDefName(), "monster_hunter_invul")) {
					//Only do a quater of the damage mod
					mod *= 0.25f;
				}
				ent->Damage( owner, owner, globalKickDir, meleeDefName, mod, tr.c.id );
				hit = true;
			}

			if ( weaponDef->dict.GetBool( "impact_damage_effect" ) ) {

				if ( ent->spawnArgs.GetBool( "bleed" ) ) {

					hitSound = meleeDef->dict.GetString( owner->PowerUpActive( BERSERK ) ? "snd_hit_berserk" : "snd_hit" );

					ent->AddDamageEffect( tr, impulse, meleeDef->dict.GetString( "classname" ) );

				} else {

					int type = tr.c.material->GetSurfaceType();
					if ( type == SURFTYPE_NONE ) {
						type = GetDefaultSurfaceType();
					}

					const char *materialType = gameLocal.sufaceTypeNames[ type ];

					// start impact sound based on material type
					hitSound = meleeDef->dict.GetString( va( "snd_%s", materialType ) );
					if ( *hitSound == '\0' ) {
						hitSound = meleeDef->dict.GetString( "snd_metal" );
					}

					if ( gameLocal.time > nextStrikeFx ) {
						const char *decal;
						// project decal
						decal = weaponDef->dict.GetString( "mtr_strike" );
						if ( decal != NULL && *decal != NULL ) {
							gameLocal.ProjectDecal( tr.c.point, -tr.c.normal, 8.0f, true, 6.0, decal );
						}
						nextStrikeFx = gameLocal.time + 200;
					} else {
						hitSound = "";
					}

					strikeSmokeStartTime = gameLocal.time;
					strikePos = tr.c.point;
					strikeAxis = -tr.endAxis;
				}
			}
		}

		if ( *hitSound != '\0' ) {
			const idSoundShader *snd = declManager->FindSound( hitSound );
			StartSoundShader( snd, SND_CHANNEL_BODY2, 0, true, NULL );
		}

		idThread::ReturnInt( hit );
		owner->WeaponFireFeedback( &weaponDef->dict );
		return;
	}

	idThread::ReturnInt( 0 );
	owner->WeaponFireFeedback( &weaponDef->dict );
}

/*
=====================
idWeapon::Event_GetWorldModel
=====================
*/
void idWeapon::Event_GetWorldModel() {
	idThread::ReturnEntity( worldModel.GetEntity() );
}

/*
=====================
idWeapon::Event_AllowDrop
=====================
*/
void idWeapon::Event_AllowDrop( int allow ) {
	if ( allow ) {
		allowDrop = true;
	} else {
		allowDrop = false;
	}
}

/*
================
idWeapon::Event_EjectBrass

Toss a shell model out from the breach if the bone is present
================
*/
void idWeapon::Event_EjectBrass() {
	if ( !g_showBrass.GetBool() || !owner->CanShowWeaponViewmodel() ) {
		return;
	}

	if ( ejectJointView == INVALID_JOINT || !brassDict.GetNumKeyVals() ) {
		return;
	}

	if ( common->IsClient() ) {
		return;
	}

	idMat3 axis;
	idVec3 origin, linear_velocity, angular_velocity;
	idEntity *ent;

	if ( !GetGlobalJointTransform( true, ejectJointView, origin, axis ) ) {
		return;
	}

	gameLocal.SpawnEntityDef( brassDict, &ent, false );
	if ( !ent || !ent->IsType( idDebris::Type ) ) {
		gameLocal.Error( "'%s' is not an idDebris", weaponDef ? weaponDef->dict.GetString( "def_ejectBrass" ) : "def_ejectBrass" );
	}
	idDebris *debris = static_cast<idDebris *>(ent);
	debris->Create( owner, origin, axis );
	debris->Launch();

	linear_velocity = 40 * ( playerViewAxis[0] + playerViewAxis[1] + playerViewAxis[2] );
	angular_velocity.Set( 10 * gameLocal.random.CRandomFloat(), 10 * gameLocal.random.CRandomFloat(), 10 * gameLocal.random.CRandomFloat() );

	debris->GetPhysics()->SetLinearVelocity( linear_velocity );
	debris->GetPhysics()->SetAngularVelocity( angular_velocity );
}

/*
===============
idWeapon::Event_IsInvisible
===============
*/
void idWeapon::Event_IsInvisible() {
	if ( !owner ) {
		idThread::ReturnFloat( 0 );
		return;
	}
	idThread::ReturnFloat( owner->PowerUpActive( INVISIBILITY ) ? 1 : 0 );
}

/*
===============
idWeapon::ClientThink
===============
*/
void idWeapon::ClientThink( const int curTime, const float fraction, const bool predict ) {
	UpdateAnimation();	
}

/*
===============
idWeapon::ClientPredictionThink
===============
*/
void idWeapon::ClientPredictionThink() {
	UpdateAnimation();	
}


void idWeapon::ForceAmmoInClip() {
	ammoClip = clipSize;
}