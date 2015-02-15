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
#include "../framework/Common_local.h"
#include "PredictedValue_impl.h"

idCVar flashlight_batteryDrainTimeMS( "flashlight_batteryDrainTimeMS", "30000", CVAR_INTEGER, "amount of time (in MS) it takes for full battery to drain (-1 == no battery drain)" );
idCVar flashlight_batteryChargeTimeMS( "flashlight_batteryChargeTimeMS", "3000", CVAR_INTEGER, "amount of time (in MS) it takes to fully recharge battery" );
idCVar flashlight_minActivatePercent( "flashlight_minActivatePercent", ".25", CVAR_FLOAT, "( 0.0 - 1.0 ) minimum amount of battery (%) needed to turn on flashlight" );
idCVar flashlight_batteryFlickerPercent( "flashlight_batteryFlickerPercent", ".1", CVAR_FLOAT, "chance of flickering when battery is low" );

// No longer userinfo, but I don't want to rename the cvar
idCVar ui_showGun( "ui_showGun", "1", CVAR_GAME | CVAR_ARCHIVE | CVAR_BOOL, "show gun" );

// Client-authoritative stuff
idCVar pm_clientAuthoritative_debug( "pm_clientAuthoritative_debug", "0", CVAR_BOOL, "" );
idCVar pm_controllerShake_damageMaxMag( "pm_controllerShake_damageMaxMag", "60.0f", CVAR_FLOAT, "" );
idCVar pm_controllerShake_damageMaxDur( "pm_controllerShake_damageMaxDur", "60.0f", CVAR_FLOAT, "" );

idCVar pm_clientAuthoritative_warnDist( "pm_clientAuthoritative_warnDist", "100.0f", CVAR_FLOAT, "" );
idCVar pm_clientAuthoritative_minDistZ( "pm_clientAuthoritative_minDistZ", "1.0f", CVAR_FLOAT, "" );
idCVar pm_clientAuthoritative_minDist( "pm_clientAuthoritative_minDist", "-1.0f", CVAR_FLOAT, "" );
idCVar pm_clientAuthoritative_Lerp( "pm_clientAuthoritative_Lerp", "0.9f", CVAR_FLOAT, "" );

idCVar pm_clientAuthoritative_Divergence( "pm_clientAuthoritative_Divergence", "200.0f", CVAR_FLOAT, "" );
idCVar pm_clientInterpolation_Divergence( "pm_clientInterpolation_Divergence", "5000.0f", CVAR_FLOAT, "" );

idCVar pm_clientAuthoritative_minSpeedSquared( "pm_clientAuthoritative_minSpeedSquared", "1000.0f", CVAR_FLOAT, "" );

extern idCVar g_demoMode;

/*
===============================================================================

	Player control of the Doom Marine.
	This object handles all player movement and world interaction.

===============================================================================
*/

// distance between ladder rungs (actually is half that distance, but this sounds better)
const int LADDER_RUNG_DISTANCE = 32;

// amount of health per dose from the health station
const int HEALTH_PER_DOSE = 10;

// time before a weapon dropped to the floor disappears
const int WEAPON_DROP_TIME = 20 * 1000;

// time before a next or prev weapon switch happens
const int WEAPON_SWITCH_DELAY = 150;

// how many units to raise spectator above default view height so it's in the head of someone
const int SPECTATE_RAISE = 25;

const int HEALTHPULSE_TIME = 333;

// minimum speed to bob and play run/walk animations at
const float MIN_BOB_SPEED = 5.0f;

// Special team used for spectators that we ONLY store on lobby.  The local team property on player remains as 0 or 1.
const float LOBBY_SPECTATE_TEAM_FOR_VOICE_CHAT = 2;

const idEventDef EV_Player_GetButtons( "getButtons", NULL, 'd' );
const idEventDef EV_Player_GetMove( "getMove", NULL, 'v' );
const idEventDef EV_Player_GetViewAngles( "getViewAngles", NULL, 'v' );
const idEventDef EV_Player_StopFxFov( "stopFxFov" );
const idEventDef EV_Player_EnableWeapon( "enableWeapon" );
const idEventDef EV_Player_DisableWeapon( "disableWeapon" );
const idEventDef EV_Player_GetCurrentWeapon( "getCurrentWeapon", NULL, 's' );
const idEventDef EV_Player_GetPreviousWeapon( "getPreviousWeapon", NULL, 's' );
const idEventDef EV_Player_SelectWeapon( "selectWeapon", "s" );
const idEventDef EV_Player_GetWeaponEntity( "getWeaponEntity", NULL, 'e' );
const idEventDef EV_Player_OpenPDA( "openPDA" );
const idEventDef EV_Player_InPDA( "inPDA", NULL, 'd' );
const idEventDef EV_Player_ExitTeleporter( "exitTeleporter" );
const idEventDef EV_Player_StopAudioLog( "stopAudioLog" );
const idEventDef EV_Player_HideTip( "hideTip" );
const idEventDef EV_Player_LevelTrigger( "levelTrigger" );
const idEventDef EV_SpectatorTouch( "spectatorTouch", "et" );
const idEventDef EV_Player_GiveInventoryItem( "giveInventoryItem", "s" );
const idEventDef EV_Player_RemoveInventoryItem( "removeInventoryItem", "s" );
const idEventDef EV_Player_GetIdealWeapon( "getIdealWeapon", NULL, 's' );
const idEventDef EV_Player_SetPowerupTime( "setPowerupTime", "dd" );
const idEventDef EV_Player_IsPowerupActive( "isPowerupActive", "d", 'd' );
const idEventDef EV_Player_WeaponAvailable( "weaponAvailable", "s", 'd' );
const idEventDef EV_Player_StartWarp( "startWarp" );
const idEventDef EV_Player_StopHelltime( "stopHelltime", "d" );
const idEventDef EV_Player_ToggleBloom( "toggleBloom", "d" );
const idEventDef EV_Player_SetBloomParms( "setBloomParms", "ff" );

CLASS_DECLARATION( idActor, idPlayer )
EVENT( EV_Player_GetButtons,			idPlayer::Event_GetButtons )
EVENT( EV_Player_GetMove,				idPlayer::Event_GetMove )
EVENT( EV_Player_GetViewAngles,			idPlayer::Event_GetViewAngles )
EVENT( EV_Player_StopFxFov,				idPlayer::Event_StopFxFov )
EVENT( EV_Player_EnableWeapon,			idPlayer::Event_EnableWeapon )
EVENT( EV_Player_DisableWeapon,			idPlayer::Event_DisableWeapon )
EVENT( EV_Player_GetCurrentWeapon,		idPlayer::Event_GetCurrentWeapon )
EVENT( EV_Player_GetPreviousWeapon,		idPlayer::Event_GetPreviousWeapon )
EVENT( EV_Player_SelectWeapon,			idPlayer::Event_SelectWeapon )
EVENT( EV_Player_GetWeaponEntity,		idPlayer::Event_GetWeaponEntity )
EVENT( EV_Player_OpenPDA,				idPlayer::Event_OpenPDA )
EVENT( EV_Player_InPDA,					idPlayer::Event_InPDA )
EVENT( EV_Player_ExitTeleporter,		idPlayer::Event_ExitTeleporter )
EVENT( EV_Player_StopAudioLog,			idPlayer::Event_StopAudioLog )
EVENT( EV_Player_HideTip,				idPlayer::Event_HideTip )
EVENT( EV_Player_LevelTrigger,			idPlayer::Event_LevelTrigger )
EVENT( EV_Gibbed,						idPlayer::Event_Gibbed )
EVENT( EV_Player_GiveInventoryItem,		idPlayer::Event_GiveInventoryItem )
EVENT( EV_Player_RemoveInventoryItem,	idPlayer::Event_RemoveInventoryItem )
EVENT( EV_Player_GetIdealWeapon,		idPlayer::Event_GetIdealWeapon )
EVENT( EV_Player_WeaponAvailable,		idPlayer::Event_WeaponAvailable )
EVENT( EV_Player_SetPowerupTime,		idPlayer::Event_SetPowerupTime )
EVENT( EV_Player_IsPowerupActive,		idPlayer::Event_IsPowerupActive )
EVENT( EV_Player_StartWarp,				idPlayer::Event_StartWarp )
EVENT( EV_Player_StopHelltime,			idPlayer::Event_StopHelltime )
EVENT( EV_Player_ToggleBloom,			idPlayer::Event_ToggleBloom )
EVENT( EV_Player_SetBloomParms,			idPlayer::Event_SetBloomParms )
END_CLASS

const int MAX_RESPAWN_TIME = 10000;
const int RAGDOLL_DEATH_TIME = 3000;
const int MAX_PDAS = 64;
const int MAX_PDA_ITEMS = 128;
const int STEPUP_TIME = 200;
const int MAX_INVENTORY_ITEMS = 20;

/*
==============
idInventory::Clear
==============
*/
void idInventory::Clear()
{
	maxHealth		= 0;
	weapons			= 0;
	powerups		= 0;
	armor			= 0;
	maxarmor		= 0;
	deplete_armor	= 0;
	deplete_rate	= 0.0f;
	deplete_ammount	= 0;
	nextArmorDepleteTime = 0;
	
	for( int i = 0; i < ammo.Num(); ++i )
	{
		ammo[i].Set( 0 );
	}
	
	ClearPowerUps();
	
	// set to -1 so that the gun knows to have a full clip the first time we get it and at the start of the level
	for( int i = 0; i < clip.Num(); ++i )
	{
		clip[i].Set( -1 );
	}
	
	items.DeleteContents( true );
	memset( pdasViewed, 0, 4 * sizeof( pdasViewed[0] ) );
	pdas.Clear();
	videos.Clear();
	emails.Clear();
	selVideo = 0;
	selEMail = 0;
	selPDA = 0;
	selAudio = 0;
	pdaOpened = false;
	
	levelTriggers.Clear();
	
	nextItemPickup = 0;
	nextItemNum = 1;
	onePickupTime = 0;
	pickupItemNames.Clear();
	objectiveNames.Clear();
	
	ammoPredictTime = 0;
	
	lastGiveTime = 0;
	
	ammoPulse	= false;
	weaponPulse	= false;
	armorPulse	= false;
}

/*
==============
idInventory::GivePowerUp
==============
*/
void idInventory::GivePowerUp( idPlayer* player, int powerup, int msec )
{
	powerups |= 1 << powerup;
	powerupEndTime[ powerup ] = gameLocal.time + msec;
}

/*
==============
idInventory::ClearPowerUps
==============
*/
void idInventory::ClearPowerUps()
{
	int i;
	for( i = 0; i < MAX_POWERUPS; i++ )
	{
		powerupEndTime[ i ] = 0;
	}
	powerups = 0;
}

/*
==============
idInventory::GetPersistantData
==============
*/
void idInventory::GetPersistantData( idDict& dict )
{
	int		i;
	int		num;
	idDict*	item;
	idStr	key;
	const idKeyValue* kv;
	const char* name;
	
	// armor
	dict.SetInt( "armor", armor );
	
	// don't bother with powerups, maxhealth, maxarmor, or the clip
	
	// ammo
	for( i = 0; i < AMMO_NUMTYPES; i++ )
	{
		name = idWeapon::GetAmmoNameForNum( ( ammo_t )i );
		if( name )
		{
			dict.SetInt( name, ammo[ i ].Get() );
		}
	}
	
	//Save the clip data
	for( i = 0; i < MAX_WEAPONS; i++ )
	{
		dict.SetInt( va( "clip%i", i ), clip[ i ].Get() );
	}
	
	// items
	num = 0;
	for( i = 0; i < items.Num(); i++ )
	{
		item = items[ i ];
		
		// copy all keys with "inv_"
		kv = item->MatchPrefix( "inv_" );
		if( kv )
		{
			while( kv )
			{
				sprintf( key, "item_%i %s", num, kv->GetKey().c_str() );
				dict.Set( key, kv->GetValue() );
				kv = item->MatchPrefix( "inv_", kv );
			}
			num++;
		}
	}
	dict.SetInt( "items", num );
	
	// pdas viewed
	for( i = 0; i < 4; i++ )
	{
		dict.SetInt( va( "pdasViewed_%i", i ), pdasViewed[i] );
	}
	
	dict.SetInt( "selPDA", selPDA );
	dict.SetInt( "selVideo", selVideo );
	dict.SetInt( "selEmail", selEMail );
	dict.SetInt( "selAudio", selAudio );
	dict.SetInt( "pdaOpened", pdaOpened );
	
	// pdas
	for( i = 0; i < pdas.Num(); i++ )
	{
		sprintf( key, "pda_%i", i );
		dict.Set( key, pdas[ i ]->GetName() );
	}
	dict.SetInt( "pdas", pdas.Num() );
	
	// video cds
	for( i = 0; i < videos.Num(); i++ )
	{
		sprintf( key, "video_%i", i );
		dict.Set( key, videos[ i ]->GetName() );
	}
	dict.SetInt( "videos", videos.Num() );
	
	// emails
	for( i = 0; i < emails.Num(); i++ )
	{
		sprintf( key, "email_%i", i );
		dict.Set( key, emails[ i ]->GetName() );
	}
	dict.SetInt( "emails", emails.Num() );
	
	// weapons
	dict.SetInt( "weapon_bits", weapons );
	
	dict.SetInt( "levelTriggers", levelTriggers.Num() );
	for( i = 0; i < levelTriggers.Num(); i++ )
	{
		sprintf( key, "levelTrigger_Level_%i", i );
		dict.Set( key, levelTriggers[i].levelName );
		sprintf( key, "levelTrigger_Trigger_%i", i );
		dict.Set( key, levelTriggers[i].triggerName );
	}
}

/*
==============
idInventory::RestoreInventory
==============
*/
void idInventory::RestoreInventory( idPlayer* owner, const idDict& dict )
{
	int			i;
	int			num;
	idDict*		item;
	idStr		key;
	idStr		itemname;
	const idKeyValue* kv;
	const char*	name;
	
	Clear();
	
	// health/armor
	maxHealth		= dict.GetInt( "maxhealth", "100" );
	armor			= dict.GetInt( "armor", "50" );
	maxarmor		= dict.GetInt( "maxarmor", "100" );
	deplete_armor	= dict.GetInt( "deplete_armor", "0" );
	deplete_rate	= dict.GetFloat( "deplete_rate", "2.0" );
	deplete_ammount	= dict.GetInt( "deplete_ammount", "1" );
	
	// the clip and powerups aren't restored
	
	// ammo
	for( i = 0; i < AMMO_NUMTYPES; i++ )
	{
		name = idWeapon::GetAmmoNameForNum( ( ammo_t )i );
		if( name )
		{
			ammo[ i ] = dict.GetInt( name );
		}
	}
	
	//Restore the clip data
	for( i = 0; i < MAX_WEAPONS; i++ )
	{
		clip[i] = dict.GetInt( va( "clip%i", i ), "-1" );
	}
	
	// items
	num = dict.GetInt( "items" );
	items.SetNum( num );
	for( i = 0; i < num; i++ )
	{
		item = new( TAG_ENTITY ) idDict();
		items[ i ] = item;
		sprintf( itemname, "item_%i ", i );
		kv = dict.MatchPrefix( itemname );
		while( kv )
		{
			key = kv->GetKey();
			key.Strip( itemname );
			item->Set( key, kv->GetValue() );
			kv = dict.MatchPrefix( itemname, kv );
		}
	}
	
	// pdas viewed
	for( i = 0; i < 4; i++ )
	{
		pdasViewed[i] = dict.GetInt( va( "pdasViewed_%i", i ) );
	}
	
	selPDA = dict.GetInt( "selPDA" );
	selEMail = dict.GetInt( "selEmail" );
	selVideo = dict.GetInt( "selVideo" );
	selAudio = dict.GetInt( "selAudio" );
	pdaOpened = dict.GetBool( "pdaOpened" );
	
	// pdas
	num = dict.GetInt( "pdas" );
	pdas.SetNum( num );
	for( i = 0; i < num; i++ )
	{
		sprintf( itemname, "pda_%i", i );
		pdas[i] = static_cast<const idDeclPDA*>( declManager->FindType( DECL_PDA, dict.GetString( itemname, "default" ) ) );
	}
	
	// videos
	num = dict.GetInt( "videos" );
	videos.SetNum( num );
	for( i = 0; i < num; i++ )
	{
		sprintf( itemname, "video_%i", i );
		videos[i] = static_cast<const idDeclVideo*>( declManager->FindType( DECL_VIDEO, dict.GetString( itemname, "default" ) ) );
	}
	
	// emails
	num = dict.GetInt( "emails" );
	emails.SetNum( num );
	for( i = 0; i < num; i++ )
	{
		sprintf( itemname, "email_%i", i );
		emails[i] = static_cast<const idDeclEmail*>( declManager->FindType( DECL_EMAIL, dict.GetString( itemname, "default" ) ) );
	}
	
	// weapons are stored as a number for persistant data, but as strings in the entityDef
	weapons	= dict.GetInt( "weapon_bits", "0" );
	
	if( g_skill.GetInteger() >= 3 || cvarSystem->GetCVarBool( "fs_buildresources" ) )
	{
		Give( owner, dict, "weapon", dict.GetString( "weapon_nightmare" ), NULL, false, ITEM_GIVE_FEEDBACK | ITEM_GIVE_UPDATE_STATE );
	}
	else
	{
		Give( owner, dict, "weapon", dict.GetString( "weapon" ), NULL, false, ITEM_GIVE_FEEDBACK | ITEM_GIVE_UPDATE_STATE );
	}
	
	num = dict.GetInt( "levelTriggers" );
	for( i = 0; i < num; i++ )
	{
		sprintf( itemname, "levelTrigger_Level_%i", i );
		idLevelTriggerInfo lti;
		lti.levelName = dict.GetString( itemname );
		sprintf( itemname, "levelTrigger_Trigger_%i", i );
		lti.triggerName = dict.GetString( itemname );
		levelTriggers.Append( lti );
	}
	
}

/*
==============
idInventory::Save
==============
*/
void idInventory::Save( idSaveGame* savefile ) const
{
	int i;
	
	savefile->WriteInt( maxHealth );
	savefile->WriteInt( weapons );
	savefile->WriteInt( powerups );
	savefile->WriteInt( armor );
	savefile->WriteInt( maxarmor );
	savefile->WriteInt( ammoPredictTime );
	savefile->WriteInt( deplete_armor );
	savefile->WriteFloat( deplete_rate );
	savefile->WriteInt( deplete_ammount );
	savefile->WriteInt( nextArmorDepleteTime );
	
	for( i = 0; i < AMMO_NUMTYPES; i++ )
	{
		savefile->WriteInt( ammo[ i ].Get() );
	}
	for( i = 0; i < MAX_WEAPONS; i++ )
	{
		savefile->WriteInt( clip[ i ].Get() );
	}
	for( i = 0; i < MAX_POWERUPS; i++ )
	{
		savefile->WriteInt( powerupEndTime[ i ] );
	}
	
	savefile->WriteInt( items.Num() );
	for( i = 0; i < items.Num(); i++ )
	{
		savefile->WriteDict( items[ i ] );
	}
	
	savefile->WriteInt( pdasViewed[0] );
	savefile->WriteInt( pdasViewed[1] );
	savefile->WriteInt( pdasViewed[2] );
	savefile->WriteInt( pdasViewed[3] );
	
	savefile->WriteInt( selPDA );
	savefile->WriteInt( selVideo );
	savefile->WriteInt( selEMail );
	savefile->WriteInt( selAudio );
	savefile->WriteBool( pdaOpened );
	
	savefile->WriteInt( pdas.Num() );
	for( i = 0; i < pdas.Num(); i++ )
	{
		savefile->WriteString( pdas[ i ]->GetName() );
	}
	
	savefile->WriteInt( pdaSecurity.Num() );
	for( i = 0; i < pdaSecurity.Num(); i++ )
	{
		savefile->WriteString( pdaSecurity[ i ] );
	}
	
	savefile->WriteInt( videos.Num() );
	for( i = 0; i < videos.Num(); i++ )
	{
		savefile->WriteString( videos[ i ]->GetName() );
	}
	
	savefile->WriteInt( emails.Num() );
	for( i = 0; i < emails.Num(); i++ )
	{
		savefile->WriteString( emails[ i ]->GetName() );
	}
	
	savefile->WriteInt( nextItemPickup );
	savefile->WriteInt( nextItemNum );
	savefile->WriteInt( onePickupTime );
	
	savefile->WriteInt( pickupItemNames.Num() );
	for( i = 0; i < pickupItemNames.Num(); i++ )
	{
		savefile->WriteString( pickupItemNames[i] );
	}
	
	savefile->WriteInt( objectiveNames.Num() );
	for( i = 0; i < objectiveNames.Num(); i++ )
	{
		savefile->WriteMaterial( objectiveNames[i].screenshot );
		savefile->WriteString( objectiveNames[i].text );
		savefile->WriteString( objectiveNames[i].title );
	}
	
	savefile->WriteInt( levelTriggers.Num() );
	for( i = 0; i < levelTriggers.Num(); i++ )
	{
		savefile->WriteString( levelTriggers[i].levelName );
		savefile->WriteString( levelTriggers[i].triggerName );
	}
	
	savefile->WriteBool( ammoPulse );
	savefile->WriteBool( weaponPulse );
	savefile->WriteBool( armorPulse );
	
	savefile->WriteInt( lastGiveTime );
	
	for( i = 0; i < AMMO_NUMTYPES; i++ )
	{
		savefile->WriteInt( rechargeAmmo[i].ammo );
		savefile->WriteInt( rechargeAmmo[i].rechargeTime );
		savefile->WriteString( rechargeAmmo[i].ammoName );
	}
}

/*
==============
idInventory::Restore
==============
*/
void idInventory::Restore( idRestoreGame* savefile )
{
	int i, num;
	
	savefile->ReadInt( maxHealth );
	savefile->ReadInt( weapons );
	savefile->ReadInt( powerups );
	savefile->ReadInt( armor );
	savefile->ReadInt( maxarmor );
	savefile->ReadInt( ammoPredictTime );
	savefile->ReadInt( deplete_armor );
	savefile->ReadFloat( deplete_rate );
	savefile->ReadInt( deplete_ammount );
	savefile->ReadInt( nextArmorDepleteTime );
	
	for( i = 0; i < AMMO_NUMTYPES; i++ )
	{
		int savedAmmo = 0;
		savefile->ReadInt( savedAmmo );
		ammo[ i ].Set( savedAmmo );
	}
	for( i = 0; i < MAX_WEAPONS; i++ )
	{
		int savedClip = 0;
		savefile->ReadInt( savedClip );
		clip[ i ].Set( savedClip );
	}
	for( i = 0; i < MAX_POWERUPS; i++ )
	{
		savefile->ReadInt( powerupEndTime[ i ] );
	}
	
	savefile->ReadInt( num );
	for( i = 0; i < num; i++ )
	{
		idDict* itemdict = new( TAG_ENTITY ) idDict;
		
		savefile->ReadDict( itemdict );
		items.Append( itemdict );
	}
	
	// pdas
	savefile->ReadInt( pdasViewed[0] );
	savefile->ReadInt( pdasViewed[1] );
	savefile->ReadInt( pdasViewed[2] );
	savefile->ReadInt( pdasViewed[3] );
	
	savefile->ReadInt( selPDA );
	savefile->ReadInt( selVideo );
	savefile->ReadInt( selEMail );
	savefile->ReadInt( selAudio );
	savefile->ReadBool( pdaOpened );
	
	savefile->ReadInt( num );
	for( i = 0; i < num; i++ )
	{
		idStr strPda;
		savefile->ReadString( strPda );
		pdas.Append( static_cast<const idDeclPDA*>( declManager->FindType( DECL_PDA, strPda ) ) );
	}
	
	// pda security clearances
	savefile->ReadInt( num );
	for( i = 0; i < num; i++ )
	{
		idStr invName;
		savefile->ReadString( invName );
		pdaSecurity.Append( invName );
	}
	
	// videos
	savefile->ReadInt( num );
	for( i = 0; i < num; i++ )
	{
		idStr strVideo;
		savefile->ReadString( strVideo );
		videos.Append( static_cast<const idDeclVideo*>( declManager->FindType( DECL_VIDEO, strVideo ) ) );
	}
	
	// email
	savefile->ReadInt( num );
	for( i = 0; i < num; i++ )
	{
		idStr strEmail;
		savefile->ReadString( strEmail );
		emails.Append( static_cast<const idDeclEmail*>( declManager->FindType( DECL_EMAIL, strEmail ) ) );
	}
	
	savefile->ReadInt( nextItemPickup );
	savefile->ReadInt( nextItemNum );
	savefile->ReadInt( onePickupTime );
	savefile->ReadInt( num );
	for( i = 0; i < num; i++ )
	{
		idStr itemName;
		savefile->ReadString( itemName );
		pickupItemNames.Append( itemName );
	}
	
	savefile->ReadInt( num );
	for( i = 0; i < num; i++ )
	{
		idObjectiveInfo obj;
		
		savefile->ReadMaterial( obj.screenshot );
		savefile->ReadString( obj.text );
		savefile->ReadString( obj.title );
		
		objectiveNames.Append( obj );
	}
	
	savefile->ReadInt( num );
	for( i = 0; i < num; i++ )
	{
		idLevelTriggerInfo lti;
		savefile->ReadString( lti.levelName );
		savefile->ReadString( lti.triggerName );
		levelTriggers.Append( lti );
	}
	
	savefile->ReadBool( ammoPulse );
	savefile->ReadBool( weaponPulse );
	savefile->ReadBool( armorPulse );
	
	savefile->ReadInt( lastGiveTime );
	
	for( i = 0; i < AMMO_NUMTYPES; i++ )
	{
		savefile->ReadInt( rechargeAmmo[i].ammo );
		savefile->ReadInt( rechargeAmmo[i].rechargeTime );
		
		idStr name;
		savefile->ReadString( name );
		strcpy( rechargeAmmo[i].ammoName, name );
	}
}

/*
==============
idInventory::AmmoIndexForAmmoClass
==============
*/
ammo_t idInventory::AmmoIndexForAmmoClass( const char* ammo_classname ) const
{
	return idWeapon::GetAmmoNumForName( ammo_classname );
}

/*
==============
idInventory::AmmoIndexForAmmoClass
==============
*/
int idInventory::MaxAmmoForAmmoClass( const idPlayer* owner, const char* ammo_classname ) const
{
	return owner->spawnArgs.GetInt( va( "max_%s", ammo_classname ), "0" );
}

/*
==============
idInventory::AmmoPickupNameForIndex
==============
*/
const char* idInventory::AmmoPickupNameForIndex( ammo_t ammonum ) const
{
	return idWeapon::GetAmmoPickupNameForNum( ammonum );
}

/*
==============
idInventory::WeaponIndexForAmmoClass
mapping could be prepared in the constructor
==============
*/
int idInventory::WeaponIndexForAmmoClass( const idDict& spawnArgs, const char* ammo_classname ) const
{
	int i;
	const char* weapon_classname;
	for( i = 0; i < MAX_WEAPONS; i++ )
	{
		weapon_classname = spawnArgs.GetString( va( "def_weapon%d", i ) );
		if( !weapon_classname )
		{
			continue;
		}
		const idDeclEntityDef* decl = gameLocal.FindEntityDef( weapon_classname, false );
		if( !decl )
		{
			continue;
		}
		if( !idStr::Icmp( ammo_classname, decl->dict.GetString( "ammoType" ) ) )
		{
			return i;
		}
	}
	return -1;
}

/*
==============
idInventory::AmmoIndexForWeaponClass
==============
*/
ammo_t idInventory::AmmoIndexForWeaponClass( const char* weapon_classname, int* ammoRequired )
{
	const idDeclEntityDef* decl = gameLocal.FindEntityDef( weapon_classname, false );
	if( !decl )
	{
		//gameLocal.Error( "Unknown weapon in decl '%s'", weapon_classname );
		return 0;
	}
	if( ammoRequired )
	{
		*ammoRequired = decl->dict.GetInt( "ammoRequired" );
	}
	ammo_t ammo_i = AmmoIndexForAmmoClass( decl->dict.GetString( "ammoType" ) );
	return ammo_i;
}

/*
==============
idInventory::AddPickupName
==============
*/
void idInventory::AddPickupName( const char* name, idPlayer* owner )     //_D3XP
{
	int num = pickupItemNames.Num();
	if( ( num == 0 ) || ( pickupItemNames[ num - 1 ].Icmp( name ) != 0 ) )
	{
		if( idStr::Cmpn( name, STRTABLE_ID, STRTABLE_ID_LENGTH ) == 0 )
		{
			pickupItemNames.Append( idLocalization::GetString( name ) );
		}
		else
		{
			pickupItemNames.Append( name );
		}
	}
}

/*
==============
idInventory::Give
==============
*/
bool idInventory::Give( idPlayer* owner, const idDict& spawnArgs, const char* statname, const char* value,
						idPredictedValue< int >* idealWeapon, bool updateHud, unsigned int giveFlags )
{
	int						i;
	const char*				pos;
	const char*				end;
	int						len;
	idStr					weaponString;
	int						max;
	const idDeclEntityDef*	weaponDecl;
	bool					tookWeapon;
	int						amount;
	const char*				name;
	
	if( !idStr::Icmp( statname, "ammo_bloodstone" ) )
	{
		i = AmmoIndexForAmmoClass( statname );
		max = MaxAmmoForAmmoClass( owner, statname );
		
		if( max <= 0 )
		{
			if( giveFlags & ITEM_GIVE_UPDATE_STATE )
			{
				//No Max
				ammo[ i ] += atoi( value );
			}
		}
		else
		{
			//Already at or above the max so don't allow the give
			if( ammo[ i ].Get() >= max )
			{
				if( giveFlags & ITEM_GIVE_UPDATE_STATE )
				{
					ammo[ i ] = max;
				}
				return false;
			}
			if( giveFlags & ITEM_GIVE_UPDATE_STATE )
			{
				//We were below the max so accept the give but cap it at the max
				ammo[ i ] += atoi( value );
				if( ammo[ i ].Get() > max )
				{
					ammo[ i ] = max;
				}
			}
		}
	}
	else if( !idStr::Icmpn( statname, "ammo_", 5 ) )
	{
		i = AmmoIndexForAmmoClass( statname );
		max = MaxAmmoForAmmoClass( owner, statname );
		if( ammo[ i ].Get() >= max )
		{
			return false;
		}
		// Add ammo for the feedback flag because it's predicted.
		// If it is a misprediction, the client will be corrected in
		// a snapshot.
		if( giveFlags & ITEM_GIVE_FEEDBACK )
		{
			amount = atoi( value );
			if( amount )
			{
				ammo[ i ] += amount;
				if( ( max > 0 ) && ( ammo[ i ].Get() > max ) )
				{
					ammo[ i ] = max;
				}
				ammoPulse = true;
			}
			
			name = AmmoPickupNameForIndex( i );
			if( idStr::Length( name ) )
			{
				AddPickupName( name, owner ); //_D3XP
			}
		}
	}
	else if( !idStr::Icmp( statname, "armor" ) )
	{
		if( armor >= maxarmor )
		{
			return false;	// can't hold any more, so leave the item
		}
		if( giveFlags & ITEM_GIVE_UPDATE_STATE )
		{
			amount = atoi( value );
			if( amount )
			{
				armor += amount;
				if( armor > maxarmor )
				{
					armor = maxarmor;
				}
				nextArmorDepleteTime = 0;
				armorPulse = true;
			}
		}
	}
	else if( idStr::FindText( statname, "inclip_" ) == 0 )
	{
		if( giveFlags & ITEM_GIVE_UPDATE_STATE )
		{
			idStr temp = statname;
			i = atoi( temp.Mid( 7, 2 ) );
			if( i != -1 )
			{
				// set, don't add. not going over the clip size limit.
				SetClipAmmoForWeapon( i, atoi( value ) );
			}
		}
	}
	else if( !idStr::Icmp( statname, "invulnerability" ) )
	{
		owner->GivePowerUp( INVULNERABILITY, SEC2MS( atof( value ) ), giveFlags );
	}
	else if( !idStr::Icmp( statname, "helltime" ) )
	{
		owner->GivePowerUp( HELLTIME, SEC2MS( atof( value ) ), giveFlags );
	}
	else if( !idStr::Icmp( statname, "envirosuit" ) )
	{
		owner->GivePowerUp( ENVIROSUIT, SEC2MS( atof( value ) ), giveFlags );
		owner->GivePowerUp( ENVIROTIME, SEC2MS( atof( value ) ), giveFlags );
	}
	else if( !idStr::Icmp( statname, "berserk" ) )
	{
		owner->GivePowerUp( BERSERK, SEC2MS( atof( value ) ), giveFlags );
		//} else if ( !idStr::Icmp( statname, "haste" ) ) {
		//	owner->GivePowerUp( HASTE, SEC2MS( atof( value ) ) );
	}
	else if( !idStr::Icmp( statname, "adrenaline" ) )
	{
		if( giveFlags & ITEM_GIVE_UPDATE_STATE )
		{
			GivePowerUp( owner, ADRENALINE, SEC2MS( atof( value ) ) );
		}
	}
	else if( !idStr::Icmp( statname, "mega" ) )
	{
		if( giveFlags & ITEM_GIVE_UPDATE_STATE )
		{
			GivePowerUp( owner, MEGAHEALTH, SEC2MS( atof( value ) ) );
		}
	}
	else if( !idStr::Icmp( statname, "weapon" ) )
	{
		tookWeapon = false;
		for( pos = value; pos != NULL; pos = end )
		{
			end = strchr( pos, ',' );
			if( end )
			{
				len = end - pos;
				end++;
			}
			else
			{
				len = strlen( pos );
			}
			
			idStr weaponName( pos, 0, len );
			
			// find the number of the matching weapon name
			for( i = 0; i < MAX_WEAPONS; i++ )
			{
				if( weaponName == spawnArgs.GetString( va( "def_weapon%d", i ) ) )
				{
					break;
				}
			}
			
			if( i >= MAX_WEAPONS )
			{
				gameLocal.Warning( "Unknown weapon '%s'", weaponName.c_str() );
				continue;
			}
			
			// cache the media for this weapon
			weaponDecl = gameLocal.FindEntityDef( weaponName, false );
			
			// don't pickup "no ammo" weapon types twice
			// not for D3 SP .. there is only one case in the game where you can get a no ammo
			// weapon when you might already have it, in that case it is more conistent to pick it up
			if( common->IsMultiplayer() && ( weapons & ( 1 << i ) ) && ( weaponDecl != NULL ) && !weaponDecl->dict.GetInt( "ammoRequired" ) )
			{
				continue;
			}
			
			if( !gameLocal.world->spawnArgs.GetBool( "no_Weapons" ) || ( weaponName == "weapon_fists" ) || ( weaponName == "weapon_soulcube" ) )
			{
				if( ( weapons & ( 1 << i ) ) == 0 || common->IsMultiplayer() )
				{
					tookWeapon = true;
					
					// This is done during "feedback" so that clients can predict the ideal weapon.
					if( giveFlags & ITEM_GIVE_FEEDBACK )
					{
						idLobbyBase& lobby = session->GetActingGameStateLobbyBase();
						lobbyUserID_t& lobbyUserID = gameLocal.lobbyUserIDs[owner->entityNumber];
						if( lobby.GetLobbyUserWeaponAutoSwitch( lobbyUserID ) && idealWeapon != NULL && i != owner->weapon_bloodstone_active1 && i != owner->weapon_bloodstone_active2 && i != owner->weapon_bloodstone_active3 )
						{
							idealWeapon->Set( i );
						}
					}
					
					if( giveFlags & ITEM_GIVE_UPDATE_STATE )
					{
						if( updateHud && lastGiveTime + 1000 < gameLocal.time )
						{
							if( owner->hud )
							{
								owner->hud->GiveWeapon( owner, i );
							}
							lastGiveTime = gameLocal.time;
						}
						
						weaponPulse = true;
						weapons |= ( 1 << i );
						
						
						if( weaponName != "weapon_pda" )
						{
							for( int index = 0; index < NUM_QUICK_SLOTS; ++index )
							{
								if( owner->GetQuickSlot( index ) == -1 )
								{
									owner->SetQuickSlot( index, i );
									break;
								}
							}
						}
					}
				}
			}
		}
		return tookWeapon;
	}
	else if( !idStr::Icmp( statname, "item" ) || !idStr::Icmp( statname, "icon" ) || !idStr::Icmp( statname, "name" ) )
	{
		// ignore these as they're handled elsewhere
		return false;
	}
	else
	{
		// unknown item
		gameLocal.Warning( "Unknown stat '%s' added to player's inventory", statname );
		return false;
	}
	
	return true;
}

/*
===============
idInventoy::Drop
===============
*/
void idInventory::Drop( const idDict& spawnArgs, const char* weapon_classname, int weapon_index )
{
	// remove the weapon bit
	// also remove the ammo associated with the weapon as we pushed it in the item
	assert( weapon_index != -1 || weapon_classname );
	if( weapon_index == -1 )
	{
		for( weapon_index = 0; weapon_index < MAX_WEAPONS; weapon_index++ )
		{
			if( !idStr::Icmp( weapon_classname, spawnArgs.GetString( va( "def_weapon%d", weapon_index ) ) ) )
			{
				break;
			}
		}
		if( weapon_index >= MAX_WEAPONS )
		{
			gameLocal.Error( "Unknown weapon '%s'", weapon_classname );
		}
	}
	else if( !weapon_classname )
	{
		weapon_classname = spawnArgs.GetString( va( "def_weapon%d", weapon_index ) );
	}
	weapons &= ( 0xffffffff ^ ( 1 << weapon_index ) );
	ammo_t ammo_i = AmmoIndexForWeaponClass( weapon_classname, NULL );
	if( ammo_i && ammo_i < AMMO_NUMTYPES )
	{
		clip[ weapon_index ] = -1;
		ammo[ ammo_i ] = 0;
	}
}

/*
===============
idInventory::HasAmmo
===============
*/
int idInventory::HasAmmo( ammo_t type, int amount )
{
	if( ( type == 0 ) || !amount )
	{
		// always allow weapons that don't use ammo to fire
		return -1;
	}
	
	// check if we have infinite ammo
	if( ammo[ type ].Get() < 0 )
	{
		return -1;
	}
	
	// return how many shots we can fire
	return ammo[ type ].Get() / amount;
	
}

/*
===============
idInventory::HasAmmo
===============
*/
int idInventory::HasAmmo( const char* weapon_classname, bool includeClip, idPlayer* owner )  		//_D3XP
{
	int ammoRequired;
	ammo_t ammo_i = AmmoIndexForWeaponClass( weapon_classname, &ammoRequired );
	
	int ammoCount = HasAmmo( ammo_i, ammoRequired );
	if( includeClip && owner )
	{
		ammoCount += Max( 0, clip[owner->SlotForWeapon( weapon_classname )].Get() );
	}
	return ammoCount;
	
}

/*
===============
idInventory::HasEmptyClipCannotRefill
===============
*/
bool idInventory::HasEmptyClipCannotRefill( const char* weapon_classname, idPlayer* owner )
{

	int clipSize = clip[owner->SlotForWeapon( weapon_classname )].Get();
	if( clipSize )
	{
		return false;
	}
	
	const idDeclEntityDef* decl = gameLocal.FindEntityDef( weapon_classname, false );
	if( decl == NULL )
	{
		gameLocal.Error( "Unknown weapon in decl '%s'", weapon_classname );
		return false;
	}
	int minclip = decl->dict.GetInt( "minclipsize" );
	if( !minclip )
	{
		return false;
	}
	
	ammo_t ammo_i = AmmoIndexForAmmoClass( decl->dict.GetString( "ammoType" ) );
	int ammoRequired = decl->dict.GetInt( "ammoRequired" );
	int ammoCount = HasAmmo( ammo_i, ammoRequired );
	if( ammoCount < minclip )
	{
		return true;
	}
	return false;
}

/*
===============
idInventory::UseAmmo
===============
*/
bool idInventory::UseAmmo( ammo_t type, int amount )
{
	if( g_infiniteAmmo.GetBool() )
	{
		return true;
	}
	
	if( !HasAmmo( type, amount ) )
	{
		return false;
	}
	
	// take an ammo away if not infinite
	if( ammo[ type ].Get() >= 0 )
	{
		const int currentAmmo = GetInventoryAmmoForType( type );
		SetInventoryAmmoForType( type, currentAmmo - amount );
	}
	
	return true;
}

/*
===============
idInventory::UpdateArmor
===============
*/
void idInventory::UpdateArmor()
{
	if( deplete_armor != 0.0f && deplete_armor < armor )
	{
		if( !nextArmorDepleteTime )
		{
			nextArmorDepleteTime = gameLocal.time + deplete_rate * 1000;
		}
		else if( gameLocal.time > nextArmorDepleteTime )
		{
			armor -= deplete_ammount;
			if( armor < deplete_armor )
			{
				armor = deplete_armor;
			}
			nextArmorDepleteTime = gameLocal.time + deplete_rate * 1000;
		}
	}
}

/*
===============
idInventory::InitRechargeAmmo
===============
* Loads any recharge ammo definitions from the ammo_types entity definitions.
*/
void idInventory::InitRechargeAmmo( idPlayer* owner )
{

	memset( rechargeAmmo, 0, sizeof( rechargeAmmo ) );
	
	const idKeyValue* kv = owner->spawnArgs.MatchPrefix( "ammorecharge_" );
	while( kv )
	{
		idStr key = kv->GetKey();
		idStr ammoname = key.Right( key.Length() - strlen( "ammorecharge_" ) );
		int ammoType = AmmoIndexForAmmoClass( ammoname );
		rechargeAmmo[ammoType].ammo = ( atof( kv->GetValue().c_str() ) * 1000 );
		strcpy( rechargeAmmo[ammoType].ammoName, ammoname );
		kv = owner->spawnArgs.MatchPrefix( "ammorecharge_", kv );
	}
}

/*
===============
idInventory::RechargeAmmo
===============
* Called once per frame to update any ammo amount for ammo types that recharge.
*/
void idInventory::RechargeAmmo( idPlayer* owner )
{

	for( int i = 0; i < AMMO_NUMTYPES; i++ )
	{
		if( rechargeAmmo[i].ammo > 0 )
		{
			if( !rechargeAmmo[i].rechargeTime )
			{
				//Initialize the recharge timer.
				rechargeAmmo[i].rechargeTime = gameLocal.time;
			}
			int elapsed = gameLocal.time - rechargeAmmo[i].rechargeTime;
			if( elapsed >= rechargeAmmo[i].ammo )
			{
				int intervals = ( gameLocal.time - rechargeAmmo[i].rechargeTime ) / rechargeAmmo[i].ammo;
				ammo[i] += intervals;
				
				int max = MaxAmmoForAmmoClass( owner, rechargeAmmo[i].ammoName );
				if( max > 0 )
				{
					if( ammo[i].Get() > max )
					{
						ammo[i] = max;
					}
				}
				rechargeAmmo[i].rechargeTime += intervals * rechargeAmmo[i].ammo;
			}
		}
	}
}

/*
===============
idInventory::CanGive
===============
*/
bool idInventory::CanGive( idPlayer* owner, const idDict& spawnArgs, const char* statname, const char* value )
{

	if( !idStr::Icmp( statname, "ammo_bloodstone" ) )
	{
		int max = MaxAmmoForAmmoClass( owner, statname );
		int i = AmmoIndexForAmmoClass( statname );
		
		if( max <= 0 )
		{
			//No Max
			return true;
		}
		else
		{
			//Already at or above the max so don't allow the give
			if( ammo[ i ].Get() >= max )
			{
				ammo[ i ] = max;
				return false;
			}
			return true;
		}
	}
	else if( !idStr::Icmp( statname, "item" ) || !idStr::Icmp( statname, "icon" ) || !idStr::Icmp( statname, "name" ) )
	{
		// ignore these as they're handled elsewhere
		//These items should not be considered as succesful gives because it messes up the max ammo items
		return false;
	}
	return true;
}

/*
===============
idInventory::SetClipAmmoForWeapon

Ammo is predicted on clients. This function ensures the frame the prediction occurs
is stored so the predicted value doesn't get overwritten by snapshots. Of course
the snapshot-reading function must check this value.
===============
*/
void idInventory::SetClipAmmoForWeapon( const int weapon, const int amount )
{
	clip[weapon] = amount;
}

/*
===============
idInventory::SetInventoryAmmoForType

Ammo is predicted on clients. This function ensures the frame the prediction occurs
is stored so the predicted value doesn't get overwritten by snapshots. Of course
the snapshot-reading function must check this value.
===============
*/
void idInventory::SetInventoryAmmoForType( int ammoType, const int amount )
{
	ammo[ammoType] = amount;
}

/*
===============
idInventory::GetClipAmmoForWeapon
===============
*/
int idInventory::GetClipAmmoForWeapon( const int weapon ) const
{
	return clip[weapon].Get();
}

/*
===============
idInventory::GetInventoryAmmoForType
===============
*/
int	idInventory::GetInventoryAmmoForType( const int ammoType ) const
{
	return ammo[ammoType].Get();
}

/*
===============
idInventory::WriteAmmoToSnapshot
===============
*/
void idInventory::WriteAmmoToSnapshot( idBitMsg& msg ) const
{
	for( int i = 0; i < AMMO_NUMTYPES; i++ )
	{
		msg.WriteBits( ammo[i].Get(), ASYNC_PLAYER_INV_AMMO_BITS );
	}
	for( int i = 0; i < MAX_WEAPONS; i++ )
	{
		msg.WriteBits( clip[i].Get(), ASYNC_PLAYER_INV_CLIP_BITS );
	}
}

/*
===============
idInventory::ReadAmmoFromSnapshot
===============
*/
void idInventory::ReadAmmoFromSnapshot( const idBitMsg& msg, const int ownerEntityNumber )
{
	for( int i = 0; i < ammo.Num(); i++ )
	{
		const int snapshotAmmo = msg.ReadBits( ASYNC_PLAYER_INV_AMMO_BITS );
		ammo[i].UpdateFromSnapshot( snapshotAmmo, ownerEntityNumber );
	}
	for( int i = 0; i < clip.Num(); i++ )
	{
		const int snapshotClip = msg.ReadBits( ASYNC_PLAYER_INV_CLIP_BITS );
		clip[i].UpdateFromSnapshot( snapshotClip, ownerEntityNumber );
	}
}

/*
===============
idInventory::ReadAmmoFromSnapshot

Doesn't really matter what remote client's ammo count is, so just set it to 999.
===============
*/
void idInventory::SetRemoteClientAmmo( const int ownerEntityNumber )
{
	for( int i = 0; i < ammo.Num(); ++i )
	{
		ammo[i].UpdateFromSnapshot( 999, ownerEntityNumber );
	}
}

/*
==============
idPlayer::idPlayer
==============
*/
idPlayer::idPlayer():
	previousViewQuat( 0.0f, 0.0f, 0.0f, 1.0f ),
	nextViewQuat( 0.0f, 0.0f, 0.0f, 1.0f ),
	idealWeapon( -1 ),
	serverOverridePositionTime( 0 ),
	clientFireCount( 0 )
{

	noclip					= false;
	godmode					= false;
	
	spawnAnglesSet			= false;
	spawnAngles				= ang_zero;
	viewAngles				= ang_zero;
	cmdAngles				= ang_zero;
	independentWeaponPitchAngle = 0.0f;
	
	oldButtons				= 0;
	buttonMask				= 0;
	oldImpulseSequence		= 0;
	
	lastHitTime				= 0;
	lastSndHitTime			= 0;
	lastSavingThrowTime		= 0;
	
	laserSightHandle	= -1;
	memset( &laserSightRenderEntity, 0, sizeof( laserSightRenderEntity ) );
	
	weapon					= NULL;
	primaryObjective		= NULL;
	
	hudManager				= new idMenuHandler_HUD();
	hud						= NULL;
	objectiveSystemOpen		= false;
	memset( quickSlot, -1, sizeof( quickSlot ) );
	
	pdaMenu = new( TAG_SWF ) idMenuHandler_PDA();
	pdaVideoMat				= NULL;
	mpMessages				= NULL;
	
	mountedObject			= NULL;
	enviroSuitLight			= NULL;
	
	heartRate				= BASE_HEARTRATE;
	heartInfo.Init( 0, 0, 0, 0 );
	lastHeartAdjust			= 0;
	lastHeartBeat			= 0;
	lastDmgTime				= 0;
	deathClearContentsTime	= 0;
	lastArmorPulse			= -10000;
	stamina					= 0.0f;
	healthPool				= 0.0f;
	nextHealthPulse			= 0;
	healthPulse				= false;
	nextHealthTake			= 0;
	healthTake				= false;
	
	forceScoreBoard			= false;
	forceRespawn			= false;
	spectating				= false;
	spectator				= 0;
	wantSpectate			= true;
	
	carryingFlag			= false;
	
	lastHitToggle			= false;
	
	minRespawnTime			= 0;
	maxRespawnTime			= 0;
	
	firstPersonViewOrigin	= vec3_zero;
	firstPersonViewAxis		= mat3_identity;
	
	hipJoint				= INVALID_JOINT;
	chestJoint				= INVALID_JOINT;
	headJoint				= INVALID_JOINT;
	
	bobFoot					= 0;
	bobFrac					= 0.0f;
	bobfracsin				= 0.0f;
	bobCycle				= 0;
	xyspeed					= 0.0f;
	stepUpTime				= 0;
	stepUpDelta				= 0.0f;
	idealLegsYaw			= 0.0f;
	legsYaw					= 0.0f;
	legsForward				= true;
	oldViewYaw				= 0.0f;
	viewBobAngles			= ang_zero;
	viewBob					= vec3_zero;
	landChange				= 0;
	landTime				= 0;
	
	currentWeapon			= -1;
	previousWeapon			= -1;
	weaponSwitchTime		=  0;
	weaponEnabled			= true;
	weapon_soulcube			= -1;
	weapon_pda				= -1;
	weapon_fists			= -1;
	weapon_chainsaw			= -1;
	weapon_bloodstone		= -1;
	weapon_bloodstone_active1 = -1;
	weapon_bloodstone_active2 = -1;
	weapon_bloodstone_active3 = -1;
	harvest_lock			= false;
	
	hudPowerup				= -1;
	lastHudPowerup			= -1;
	hudPowerupDuration		= 0;
	
	skinIndex				= 0;
	skin					= NULL;
	powerUpSkin				= NULL;
	
	numProjectileKills		= 0;
	numProjectilesFired		= 0;
	numProjectileHits		= 0;
	
	airless					= false;
	airMsec					= 0;
	lastAirDamage			= 0;
	
	gibDeath				= false;
	gibsLaunched			= false;
	gibsDir					= vec3_zero;
	
	zoomFov.Init( 0, 0, 0, 0 );
	centerView.Init( 0, 0, 0, 0 );
	fxFov					= false;
	
	influenceFov			= 0;
	influenceActive			= 0;
	influenceRadius			= 0.0f;
	influenceEntity			= NULL;
	influenceMaterial		= NULL;
	influenceSkin			= NULL;
	
	privateCameraView		= NULL;
	
	memset( loggedViewAngles, 0, sizeof( loggedViewAngles ) );
	memset( loggedAccel, 0, sizeof( loggedAccel ) );
	currentLoggedAccel	= 0;
	
	focusTime				= 0;
	focusGUIent				= NULL;
	focusUI					= NULL;
	focusCharacter			= NULL;
	talkCursor				= 0;
	focusVehicle			= NULL;
	cursor					= NULL;
	
	oldMouseX				= 0;
	oldMouseY				= 0;
	
	lastDamageDef			= 0;
	lastDamageDir			= vec3_zero;
	lastDamageLocation		= 0;
	smoothedFrame			= 0;
	smoothedOriginUpdated	= false;
	smoothedOrigin			= vec3_zero;
	smoothedAngles			= ang_zero;
	
	fl.networkSync			= true;
	
	doingDeathSkin			= false;
	weaponGone				= false;
	useInitialSpawns		= false;
	tourneyRank				= 0;
	lastSpectateTeleport	= 0;
	tourneyLine				= 0;
	hiddenWeapon			= false;
	tipUp					= false;
	objectiveUp				= false;
	teleportEntity			= NULL;
	teleportKiller			= -1;
	respawning				= false;
	leader					= false;
	lastSpectateChange		= 0;
	lastTeleFX				= -9999;
	weaponCatchup			= false;
	clientFireCount			= 0;
	
	MPAim					= -1;
	lastMPAim				= -1;
	lastMPAimTime			= 0;
	MPAimFadeTime			= 0;
	MPAimHighlight			= false;
	
	spawnedTime				= 0;
	lastManOver				= false;
	lastManPlayAgain		= false;
	lastManPresent			= false;
	
	isTelefragged			= false;
	
	isLagged				= false;
	isChatting				= 0;
	
	selfSmooth				= false;
	
	playedTimeSecs			= 0;
	playedTimeResidual		= 0;
	
	ResetControllerShake();
	
	memset( pdaHasBeenRead, 0, sizeof( pdaHasBeenRead ) );
	memset( videoHasBeenViewed, 0, sizeof( videoHasBeenViewed ) );
	memset( audioHasBeenHeard, 0, sizeof( audioHasBeenHeard ) );
}

/*
==============
idPlayer::LinkScriptVariables

set up conditions for animation
==============
*/
void idPlayer::LinkScriptVariables()
{
	AI_FORWARD.LinkTo(	scriptObject, "AI_FORWARD" );
	AI_BACKWARD.LinkTo(	scriptObject, "AI_BACKWARD" );
	AI_STRAFE_LEFT.LinkTo(	scriptObject, "AI_STRAFE_LEFT" );
	AI_STRAFE_RIGHT.LinkTo(	scriptObject, "AI_STRAFE_RIGHT" );
	AI_ATTACK_HELD.LinkTo(	scriptObject, "AI_ATTACK_HELD" );
	AI_WEAPON_FIRED.LinkTo(	scriptObject, "AI_WEAPON_FIRED" );
	AI_JUMP.LinkTo(	scriptObject, "AI_JUMP" );
	AI_DEAD.LinkTo(	scriptObject, "AI_DEAD" );
	AI_CROUCH.LinkTo(	scriptObject, "AI_CROUCH" );
	AI_ONGROUND.LinkTo(	scriptObject, "AI_ONGROUND" );
	AI_ONLADDER.LinkTo(	scriptObject, "AI_ONLADDER" );
	AI_HARDLANDING.LinkTo(	scriptObject, "AI_HARDLANDING" );
	AI_SOFTLANDING.LinkTo(	scriptObject, "AI_SOFTLANDING" );
	AI_RUN.LinkTo(	scriptObject, "AI_RUN" );
	AI_PAIN.LinkTo(	scriptObject, "AI_PAIN" );
	AI_RELOAD.LinkTo(	scriptObject, "AI_RELOAD" );
	AI_TELEPORT.LinkTo(	scriptObject, "AI_TELEPORT" );
	AI_TURN_LEFT.LinkTo(	scriptObject, "AI_TURN_LEFT" );
	AI_TURN_RIGHT.LinkTo(	scriptObject, "AI_TURN_RIGHT" );
}

/*
==============
idPlayer::SetupWeaponEntity
==============
*/
void idPlayer::SetupWeaponEntity()
{
	int w;
	const char* weap;
	
	if( weapon.GetEntity() )
	{
		// get rid of old weapon
		weapon.GetEntity()->Clear();
		currentWeapon = -1;
	}
	else if( !common->IsClient() )
	{
		weapon = static_cast<idWeapon*>( gameLocal.SpawnEntityType( idWeapon::Type, NULL ) );
		weapon.GetEntity()->SetOwner( this );
		currentWeapon = -1;
		
		// flashlight
		flashlight = static_cast<idWeapon*>( gameLocal.SpawnEntityType( idWeapon::Type, NULL ) );
		flashlight.GetEntity()->SetFlashlightOwner( this );
		//FlashlightOff();
	}
	
	for( w = 0; w < MAX_WEAPONS; w++ )
	{
		weap = spawnArgs.GetString( va( "def_weapon%d", w ) );
		if( weap != NULL && *weap != '\0' )
		{
			idWeapon::CacheWeapon( weap );
		}
	}
}

/*
==============
idPlayer::Init
==============
*/
void idPlayer::Init()
{
	const char*			value;
	const idKeyValue*	kv;
	
	noclip					= false;
	godmode					= false;
	
	oldButtons				= 0;
	oldImpulseSequence		= 0;
	
	currentWeapon			= -1;
	idealWeapon				= -1;
	previousWeapon			= -1;
	weaponSwitchTime		= 0;
	weaponEnabled			= true;
	weapon_soulcube			= SlotForWeapon( "weapon_soulcube" );
	weapon_pda				= SlotForWeapon( "weapon_pda" );
	weapon_fists			= SlotForWeapon( "weapon_fists" );
	weapon_flashlight		= SlotForWeapon( "weapon_flashlight" );
	weapon_chainsaw			= SlotForWeapon( "weapon_chainsaw" );
	weapon_bloodstone		= SlotForWeapon( "weapon_bloodstone_passive" );
	weapon_bloodstone_active1 = SlotForWeapon( "weapon_bloodstone_active1" );
	weapon_bloodstone_active2 = SlotForWeapon( "weapon_bloodstone_active2" );
	weapon_bloodstone_active3 = SlotForWeapon( "weapon_bloodstone_active3" );
	harvest_lock			= false;
	
	lastDmgTime				= 0;
	lastArmorPulse			= -10000;
	lastHeartAdjust			= 0;
	lastHeartBeat			= 0;
	heartInfo.Init( 0, 0, 0, 0 );
	
	bobCycle				= 0;
	bobFrac					= 0.0f;
	landChange				= 0;
	landTime				= 0;
	zoomFov.Init( 0, 0, 0, 0 );
	centerView.Init( 0, 0, 0, 0 );
	fxFov					= false;
	
	influenceFov			= 0;
	influenceActive			= 0;
	influenceRadius			= 0.0f;
	influenceEntity			= NULL;
	influenceMaterial		= NULL;
	influenceSkin			= NULL;
	
	mountedObject			= NULL;
	if( enviroSuitLight.IsValid() )
	{
		enviroSuitLight.GetEntity()->PostEventMS( &EV_Remove, 0 );
	}
	enviroSuitLight			= NULL;
	healthRecharge			= false;
	lastHealthRechargeTime	= 0;
	rechargeSpeed			= 500;
	new_g_damageScale		= 1.f;
	bloomEnabled			= false;
	bloomSpeed				= 1.f;
	bloomIntensity			= -0.01f;
	inventory.InitRechargeAmmo( this );
	hudPowerup				= -1;
	lastHudPowerup			= -1;
	hudPowerupDuration		= 0;
	
	currentLoggedAccel		= 0;
	
	focusTime				= 0;
	focusGUIent				= NULL;
	focusUI					= NULL;
	focusCharacter			= NULL;
	talkCursor				= 0;
	focusVehicle			= NULL;
	
	// remove any damage effects
	playerView.ClearEffects();
	
	// damage values
	fl.takedamage			= true;
	ClearPain();
	
	// restore persistent data
	RestorePersistantInfo();
	
	bobCycle		= 0;
	stamina			= 0.0f;
	healthPool		= 0.0f;
	nextHealthPulse = 0;
	healthPulse		= false;
	nextHealthTake	= 0;
	healthTake		= false;
	
	SetupWeaponEntity();
	currentWeapon = -1;
	previousWeapon = -1;
	
	heartRate = BASE_HEARTRATE;
	AdjustHeartRate( BASE_HEARTRATE, 0.0f, 0.0f, true );
	
	idealLegsYaw = 0.0f;
	legsYaw = 0.0f;
	legsForward	= true;
	oldViewYaw = 0.0f;
	
	// set the pm_ cvars
	if( !common->IsMultiplayer() || common->IsServer() )
	{
		kv = spawnArgs.MatchPrefix( "pm_", NULL );
		while( kv )
		{
			cvarSystem->SetCVarString( kv->GetKey(), kv->GetValue() );
			kv = spawnArgs.MatchPrefix( "pm_", kv );
		}
	}
	
	// disable stamina on hell levels
	if( gameLocal.world && gameLocal.world->spawnArgs.GetBool( "no_stamina" ) )
	{
		pm_stamina.SetFloat( 0.0f );
	}
	
	// stamina always initialized to maximum
	stamina = pm_stamina.GetFloat();
	
	// air always initialized to maximum too
	airMsec = pm_airMsec.GetFloat();
	airless = false;
	
	gibDeath = false;
	gibsLaunched = false;
	gibsDir.Zero();
	
	// set the gravity
	physicsObj.SetGravity( gameLocal.GetGravity() );
	
	// start out standing
	SetEyeHeight( pm_normalviewheight.GetFloat() );
	
	stepUpTime = 0;
	stepUpDelta = 0.0f;
	viewBobAngles.Zero();
	viewBob.Zero();
	
	value = spawnArgs.GetString( "model" );
	if( value != NULL && ( *value != 0 ) )
	{
		SetModel( value );
	}
	
	if( hud )
	{
		hud->SetCursorState( this, CURSOR_TALK, 0 );
		hud->SetCursorState( this, CURSOR_IN_COMBAT, 1 );
		hud->SetCursorState( this, CURSOR_ITEM, 0 );
		hud->SetCursorState( this, CURSOR_GRABBER, 0 );
		hud->SetCursorState( this, CURSOR_NONE, 0 );
		hud->UpdateCursorState();
	}
	
	if( ( common->IsMultiplayer() || g_testDeath.GetBool() ) && skin )
	{
		SetSkin( skin );
		renderEntity.shaderParms[6] = 0.0f;
	}
	else if( spawnArgs.GetString( "spawn_skin", NULL, &value ) )
	{
		skin = declManager->FindSkin( value );
		SetSkin( skin );
		renderEntity.shaderParms[6] = 0.0f;
	}
	
	value = spawnArgs.GetString( "bone_hips", "" );
	hipJoint = animator.GetJointHandle( value );
	if( hipJoint == INVALID_JOINT )
	{
		gameLocal.Error( "Joint '%s' not found for 'bone_hips' on '%s'", value, name.c_str() );
	}
	
	value = spawnArgs.GetString( "bone_chest", "" );
	chestJoint = animator.GetJointHandle( value );
	if( chestJoint == INVALID_JOINT )
	{
		gameLocal.Error( "Joint '%s' not found for 'bone_chest' on '%s'", value, name.c_str() );
	}
	
	value = spawnArgs.GetString( "bone_head", "" );
	headJoint = animator.GetJointHandle( value );
	if( headJoint == INVALID_JOINT )
	{
		gameLocal.Error( "Joint '%s' not found for 'bone_head' on '%s'", value, name.c_str() );
	}
	
	// initialize the script variables
	AI_FORWARD		= false;
	AI_BACKWARD		= false;
	AI_STRAFE_LEFT	= false;
	AI_STRAFE_RIGHT	= false;
	AI_ATTACK_HELD	= false;
	AI_WEAPON_FIRED	= false;
	AI_JUMP			= false;
	AI_DEAD			= false;
	AI_CROUCH		= false;
	AI_ONGROUND		= true;
	AI_ONLADDER		= false;
	AI_HARDLANDING	= false;
	AI_SOFTLANDING	= false;
	AI_RUN			= false;
	AI_PAIN			= false;
	AI_RELOAD		= false;
	AI_TELEPORT		= false;
	AI_TURN_LEFT	= false;
	AI_TURN_RIGHT	= false;
	
	// reset the script object
	ConstructScriptObject();
	
	// execute the script so the script object's constructor takes effect immediately
	scriptThread->Execute();
	
	forceScoreBoard		= false;
	
	privateCameraView	= NULL;
	
	lastSpectateChange	= 0;
	lastTeleFX			= -9999;
	
	hiddenWeapon		= false;
	tipUp				= false;
	objectiveUp			= false;
	teleportEntity		= NULL;
	teleportKiller		= -1;
	leader				= false;
	
	SetPrivateCameraView( NULL );
	
	MPAim				= -1;
	lastMPAim			= -1;
	lastMPAimTime		= 0;
	MPAimFadeTime		= 0;
	MPAimHighlight		= false;
	
	//isChatting = false;
	
	achievementManager.Init( this );
	
	flashlightBattery = flashlight_batteryDrainTimeMS.GetInteger();		// fully charged
	
	aimAssist.Init( this );
	
	// laser sight for 3DTV
	memset( &laserSightRenderEntity, 0, sizeof( laserSightRenderEntity ) );
	laserSightRenderEntity.hModel = renderModelManager->FindModel( "_BEAM" );
	laserSightRenderEntity.customShader = declManager->FindMaterial( "stereoRenderLaserSight" );
}

/*
==============
idPlayer::Spawn

Prepare any resources used by the player.
==============
*/
void idPlayer::Spawn()
{
	idStr		temp;
	idBounds	bounds;
	
	if( entityNumber >= MAX_CLIENTS )
	{
		gameLocal.Error( "entityNum > MAX_CLIENTS for player.  Player may only be spawned with a client." );
	}
	
	// allow thinking during cinematics
	cinematic = true;
	
	if( common->IsMultiplayer() )
	{
		// always start in spectating state waiting to be spawned in
		// do this before SetClipModel to get the right bounding box
		spectating = true;
	}
	
	// set our collision model
	physicsObj.SetSelf( this );
	SetClipModel();
	physicsObj.SetMass( spawnArgs.GetFloat( "mass", "100" ) );
	physicsObj.SetContents( CONTENTS_BODY );
	physicsObj.SetClipMask( MASK_PLAYERSOLID );
	SetPhysics( &physicsObj );
	InitAASLocation();
	
	skin = renderEntity.customSkin;
	
	// only the local player needs guis
	if( !common->IsMultiplayer() || IsLocallyControlled() )
	{
	
		// load HUD
		if( hudManager != NULL )
		{
			hudManager->Initialize( "hud", common->SW() );
			hudManager->ActivateMenu( true );
			hud = hudManager->GetHud();
		}
		
		// load cursor
		if( spawnArgs.GetString( "cursor", "", temp ) )
		{
			cursor = uiManager->FindGui( temp, true, common->IsMultiplayer(), common->IsMultiplayer() );
		}
		if( cursor )
		{
			cursor->Activate( true, gameLocal.time );
		}
		
		if( pdaMenu != NULL )
		{
			pdaMenu->Initialize( "pda", common->SW() );
		}
		objectiveSystemOpen = false;
	}
	
	if( common->IsMultiplayer() && mpMessages == NULL )
	{
		mpMessages = new idSWF( "mp_messages", common->SW() );
		mpMessages->Activate( true );
	}
	
	SetLastHitTime( 0 );
	
	// load the armor sound feedback
	declManager->FindSound( "player_sounds_hitArmor" );
	
	// set up conditions for animation
	LinkScriptVariables();
	
	animator.RemoveOriginOffset( true );
	
	// create combat collision hull for exact collision detection
	SetCombatModel();
	
	// init the damage effects
	playerView.SetPlayerEntity( this );
	
	// supress model in non-player views, but allow it in mirrors and remote views
	renderEntity.suppressSurfaceInViewID = entityNumber + 1;
	
	// don't project shadow on self or weapon
	renderEntity.noSelfShadow = true;
	
	idAFAttachment* headEnt = head.GetEntity();
	if( headEnt )
	{
		headEnt->GetRenderEntity()->suppressSurfaceInViewID = entityNumber + 1;
		headEnt->GetRenderEntity()->noSelfShadow = true;
	}
	
	if( common->IsMultiplayer() )
	{
		Init();
		Hide();	// properly hidden if starting as a spectator
		if( !common->IsClient() )
		{
			// set yourself ready to spawn. idMultiplayerGame will decide when/if appropriate and call SpawnFromSpawnSpot
			SetupWeaponEntity();
			SpawnFromSpawnSpot();
			forceRespawn = true;
			wantSpectate = true;
			assert( spectating );
		}
	}
	else
	{
		SetupWeaponEntity();
		SpawnFromSpawnSpot();
	}
	
	// trigger playtesting item gives, if we didn't get here from a previous level
	// the devmap key will be set on the first devmap, but cleared on any level
	// transitions
	if( !common->IsMultiplayer() && gameLocal.serverInfo.FindKey( "devmap" ) )
	{
		// fire a trigger with the name "devmap"
		idEntity* ent = gameLocal.FindEntity( "devmap" );
		if( ent )
		{
			ent->ActivateTargets( this );
		}
	}
	
	if( hud )
	{
		if( weapon_soulcube > 0 && ( inventory.weapons & ( 1 << weapon_soulcube ) ) )
		{
			int max_souls = inventory.MaxAmmoForAmmoClass( this, "ammo_souls" );
			if( inventory.GetInventoryAmmoForType( idWeapon::GetAmmoNumForName( "ammo_souls" ) ) >= max_souls )
			{
				hud->SetShowSoulCubeOnLoad( true );
			}
		}
	}
	
	if( GetPDA() )
	{
		// Add any emails from the inventory
		for( int i = 0; i < inventory.emails.Num(); i++ )
		{
			GetPDA()->AddEmail( inventory.emails[i] );
		}
		GetPDA()->SetSecurity( idLocalization::GetString( "#str_00066" ) );
	}
	
	if( gameLocal.world->spawnArgs.GetBool( "no_Weapons" ) )
	{
		hiddenWeapon = true;
		if( weapon.GetEntity() )
		{
			weapon.GetEntity()->LowerWeapon();
		}
		idealWeapon = weapon_fists;
	}
	else
	{
		hiddenWeapon = false;
	}
	
	UpdateHudWeapon();
	
	tipUp = false;
	objectiveUp = false;
	
	if( inventory.levelTriggers.Num() )
	{
		PostEventMS( &EV_Player_LevelTrigger, 0 );
	}
	
	inventory.pdaOpened = false;
	inventory.selPDA = 0;
	
	if( !common->IsMultiplayer() )
	{
		int startingHealth = gameLocal.world->spawnArgs.GetInt( "startingHealth", health );
		if( health > startingHealth )
		{
			health = startingHealth;
		}
		if( g_skill.GetInteger() < 2 )
		{
			if( health < 25 )
			{
				health = 25;
			}
			if( g_useDynamicProtection.GetBool() )
			{
				new_g_damageScale = 1.0f;
			}
		}
		else
		{
			new_g_damageScale = 1.0f;
			g_armorProtection.SetFloat( ( g_skill.GetInteger() < 2 ) ? 0.4f : 0.2f );
			if( g_skill.GetInteger() == 3 )
			{
				nextHealthTake = gameLocal.time + g_healthTakeTime.GetInteger() * 1000;
			}
		}
	}
	
	//Setup the weapon toggle lists
	const idKeyValue* kv;
	kv = spawnArgs.MatchPrefix( "weapontoggle", NULL );
	while( kv )
	{
		WeaponToggle_t newToggle;
		strcpy( newToggle.name, kv->GetKey().c_str() );
		
		idStr toggleData = kv->GetValue();
		
		idLexer src;
		idToken token;
		src.LoadMemory( toggleData, toggleData.Length(), "toggleData" );
		while( 1 )
		{
			if( !src.ReadToken( &token ) )
			{
				break;
			}
			int index = atoi( token.c_str() );
			newToggle.toggleList.Append( index );
			
			//Skip the ,
			src.ReadToken( &token );
		}
		newToggle.lastUsed = 0;
		weaponToggles.Set( newToggle.name, newToggle );
		
		kv = spawnArgs.MatchPrefix( "weapontoggle", kv );
	}
	
	if( g_skill.GetInteger() >= 3 || cvarSystem->GetCVarBool( "fs_buildresources" ) )
	{
		if( !WeaponAvailable( "weapon_bloodstone_passive" ) )
		{
			GiveInventoryItem( "weapon_bloodstone_passive" );
		}
		if( !WeaponAvailable( "weapon_bloodstone_active1" ) )
		{
			GiveInventoryItem( "weapon_bloodstone_active1" );
		}
		if( !WeaponAvailable( "weapon_bloodstone_active2" ) )
		{
			GiveInventoryItem( "weapon_bloodstone_active2" );
		}
		if( !WeaponAvailable( "weapon_bloodstone_active3" ) )
		{
			GiveInventoryItem( "weapon_bloodstone_active3" );
		}
	}
	
	bloomEnabled			= false;
	bloomSpeed				= 1;
	bloomIntensity			= -0.01f;
	
	if( g_demoMode.GetBool() && weapon.GetEntity() && weapon.GetEntity()->AmmoInClip() == 0 )
	{
		weapon.GetEntity()->ForceAmmoInClip();
	}
	
}

/*
==============
idPlayer::~idPlayer()

Release any resources used by the player.
==============
*/
idPlayer::~idPlayer()
{
	delete weapon.GetEntity();
	weapon = NULL;
	
	delete flashlight.GetEntity();
	flashlight = NULL;
	
	if( enviroSuitLight.IsValid() )
	{
		enviroSuitLight.GetEntity()->ProcessEvent( &EV_Remove );
	}
	// have to do this here, idMultiplayerGame::DisconnectClient() is too late
	if( common->IsMultiplayer() && gameLocal.mpGame.IsGametypeFlagBased() )
	{
		ReturnFlag();
	}
	
	delete hudManager;
	hudManager = NULL;
	
	delete pdaMenu;
	pdaMenu = NULL;
	
	delete mpMessages;
	mpMessages = NULL;
}

/*
===========
idPlayer::Save
===========
*/
void idPlayer::Save( idSaveGame* savefile ) const
{
	int i;
	
	savefile->WriteUsercmd( usercmd );
	playerView.Save( savefile );
	
	savefile->WriteBool( noclip );
	savefile->WriteBool( godmode );
	
	// don't save spawnAnglesSet, since we'll have to reset them after loading the savegame
	savefile->WriteAngles( spawnAngles );
	savefile->WriteAngles( viewAngles );
	savefile->WriteAngles( cmdAngles );
	
	savefile->WriteInt( buttonMask );
	savefile->WriteInt( oldButtons );
	savefile->WriteInt( oldImpulseSequence );
	
	savefile->WriteInt( lastHitTime );
	savefile->WriteInt( lastSndHitTime );
	savefile->WriteInt( lastSavingThrowTime );
	
	// idBoolFields don't need to be saved, just re-linked in Restore
	
	savefile->WriteObject( primaryObjective );
	inventory.Save( savefile );
	weapon.Save( savefile );
	
	for( int i = 0; i < NUM_QUICK_SLOTS; ++i )
	{
		savefile->WriteInt( quickSlot[ i ] );
	}
	
	savefile->WriteInt( weapon_soulcube );
	savefile->WriteInt( weapon_pda );
	savefile->WriteInt( weapon_fists );
	savefile->WriteInt( weapon_flashlight );
	savefile->WriteInt( weapon_chainsaw );
	savefile->WriteInt( weapon_bloodstone );
	savefile->WriteInt( weapon_bloodstone_active1 );
	savefile->WriteInt( weapon_bloodstone_active2 );
	savefile->WriteInt( weapon_bloodstone_active3 );
	savefile->WriteBool( harvest_lock );
	savefile->WriteInt( hudPowerup );
	savefile->WriteInt( lastHudPowerup );
	savefile->WriteInt( hudPowerupDuration );
	
	
	
	savefile->WriteInt( heartRate );
	
	savefile->WriteFloat( heartInfo.GetStartTime() );
	savefile->WriteFloat( heartInfo.GetDuration() );
	savefile->WriteFloat( heartInfo.GetStartValue() );
	savefile->WriteFloat( heartInfo.GetEndValue() );
	
	savefile->WriteInt( lastHeartAdjust );
	savefile->WriteInt( lastHeartBeat );
	savefile->WriteInt( lastDmgTime );
	savefile->WriteInt( deathClearContentsTime );
	savefile->WriteBool( doingDeathSkin );
	savefile->WriteInt( lastArmorPulse );
	savefile->WriteFloat( stamina );
	savefile->WriteFloat( healthPool );
	savefile->WriteInt( nextHealthPulse );
	savefile->WriteBool( healthPulse );
	savefile->WriteInt( nextHealthTake );
	savefile->WriteBool( healthTake );
	
	savefile->WriteBool( hiddenWeapon );
	soulCubeProjectile.Save( savefile );
	
	savefile->WriteInt( spectator );
	savefile->WriteBool( forceScoreBoard );
	savefile->WriteBool( forceRespawn );
	savefile->WriteBool( spectating );
	savefile->WriteInt( lastSpectateTeleport );
	savefile->WriteBool( lastHitToggle );
	savefile->WriteBool( wantSpectate );
	savefile->WriteBool( weaponGone );
	savefile->WriteBool( useInitialSpawns );
	savefile->WriteInt( tourneyRank );
	savefile->WriteInt( tourneyLine );
	
	teleportEntity.Save( savefile );
	savefile->WriteInt( teleportKiller );
	
	savefile->WriteInt( minRespawnTime );
	savefile->WriteInt( maxRespawnTime );
	
	savefile->WriteVec3( firstPersonViewOrigin );
	savefile->WriteMat3( firstPersonViewAxis );
	
	// don't bother saving dragEntity since it's a dev tool
	
	savefile->WriteJoint( hipJoint );
	savefile->WriteJoint( chestJoint );
	savefile->WriteJoint( headJoint );
	
	savefile->WriteStaticObject( physicsObj );
	
	savefile->WriteInt( aasLocation.Num() );
	for( i = 0; i < aasLocation.Num(); i++ )
	{
		savefile->WriteInt( aasLocation[ i ].areaNum );
		savefile->WriteVec3( aasLocation[ i ].pos );
	}
	
	savefile->WriteInt( bobFoot );
	savefile->WriteFloat( bobFrac );
	savefile->WriteFloat( bobfracsin );
	savefile->WriteInt( bobCycle );
	savefile->WriteFloat( xyspeed );
	savefile->WriteInt( stepUpTime );
	savefile->WriteFloat( stepUpDelta );
	savefile->WriteFloat( idealLegsYaw );
	savefile->WriteFloat( legsYaw );
	savefile->WriteBool( legsForward );
	savefile->WriteFloat( oldViewYaw );
	savefile->WriteAngles( viewBobAngles );
	savefile->WriteVec3( viewBob );
	savefile->WriteInt( landChange );
	savefile->WriteInt( landTime );
	
	savefile->WriteInt( currentWeapon );
	savefile->WriteInt( idealWeapon.Get() );
	savefile->WriteInt( previousWeapon );
	savefile->WriteInt( weaponSwitchTime );
	savefile->WriteBool( weaponEnabled );
	
	savefile->WriteInt( skinIndex );
	savefile->WriteSkin( skin );
	savefile->WriteSkin( powerUpSkin );
	
	savefile->WriteInt( numProjectilesFired );
	savefile->WriteInt( numProjectileHits );
	
	savefile->WriteBool( airless );
	savefile->WriteInt( airMsec );
	savefile->WriteInt( lastAirDamage );
	
	savefile->WriteBool( gibDeath );
	savefile->WriteBool( gibsLaunched );
	savefile->WriteVec3( gibsDir );
	
	savefile->WriteFloat( zoomFov.GetStartTime() );
	savefile->WriteFloat( zoomFov.GetDuration() );
	savefile->WriteFloat( zoomFov.GetStartValue() );
	savefile->WriteFloat( zoomFov.GetEndValue() );
	
	savefile->WriteFloat( centerView.GetStartTime() );
	savefile->WriteFloat( centerView.GetDuration() );
	savefile->WriteFloat( centerView.GetStartValue() );
	savefile->WriteFloat( centerView.GetEndValue() );
	
	savefile->WriteBool( fxFov );
	
	savefile->WriteFloat( influenceFov );
	savefile->WriteInt( influenceActive );
	savefile->WriteFloat( influenceRadius );
	savefile->WriteObject( influenceEntity );
	savefile->WriteMaterial( influenceMaterial );
	savefile->WriteSkin( influenceSkin );
	
	savefile->WriteObject( privateCameraView );
	
	for( i = 0; i < NUM_LOGGED_VIEW_ANGLES; i++ )
	{
		savefile->WriteAngles( loggedViewAngles[ i ] );
	}
	for( i = 0; i < NUM_LOGGED_ACCELS; i++ )
	{
		savefile->WriteInt( loggedAccel[ i ].time );
		savefile->WriteVec3( loggedAccel[ i ].dir );
	}
	savefile->WriteInt( currentLoggedAccel );
	
	savefile->WriteObject( focusGUIent );
	// can't save focusUI
	savefile->WriteObject( focusCharacter );
	savefile->WriteInt( talkCursor );
	savefile->WriteInt( focusTime );
	savefile->WriteObject( focusVehicle );
	savefile->WriteUserInterface( cursor, false );
	
	savefile->WriteInt( oldMouseX );
	savefile->WriteInt( oldMouseY );
	
	savefile->WriteBool( tipUp );
	savefile->WriteBool( objectiveUp );
	
	savefile->WriteInt( lastDamageDef );
	savefile->WriteVec3( lastDamageDir );
	savefile->WriteInt( lastDamageLocation );
	savefile->WriteInt( smoothedFrame );
	savefile->WriteBool( smoothedOriginUpdated );
	savefile->WriteVec3( smoothedOrigin );
	savefile->WriteAngles( smoothedAngles );
	
	savefile->WriteBool( respawning );
	savefile->WriteBool( leader );
	savefile->WriteInt( lastSpectateChange );
	savefile->WriteInt( lastTeleFX );
	
	savefile->WriteFloat( pm_stamina.GetFloat() );
	
	// TODO_SPARTY hook this up with new hud
	//if ( hud ) {
	//	hud->SetStateString( "message", idLocalization::GetString( "#str_02916" ) );
	//	hud->HandleNamedEvent( "Message" );
	//}
	
	savefile->WriteInt( weaponToggles.Num() );
	for( i = 0; i < weaponToggles.Num(); i++ )
	{
		WeaponToggle_t* weaponToggle = weaponToggles.GetIndex( i );
		savefile->WriteString( weaponToggle->name );
		savefile->WriteInt( weaponToggle->toggleList.Num() );
		for( int j = 0; j < weaponToggle->toggleList.Num(); j++ )
		{
			savefile->WriteInt( weaponToggle->toggleList[j] );
		}
	}
	savefile->WriteObject( mountedObject );
	enviroSuitLight.Save( savefile );
	savefile->WriteBool( healthRecharge );
	savefile->WriteInt( lastHealthRechargeTime );
	savefile->WriteInt( rechargeSpeed );
	savefile->WriteFloat( new_g_damageScale );
	
	savefile->WriteBool( bloomEnabled );
	savefile->WriteFloat( bloomSpeed );
	savefile->WriteFloat( bloomIntensity );
	
	savefile->WriteObject( flashlight.GetEntity() );
	savefile->WriteInt( flashlightBattery );
	
	achievementManager.Save( savefile );
	
	savefile->WriteInt( playedTimeSecs );
	savefile->WriteInt( playedTimeResidual );
	
	for( int i = 0; i < MAX_PLAYER_PDA; i++ )
	{
		savefile->WriteBool( pdaHasBeenRead[i] );
	}
	
	for( int i = 0; i < MAX_PLAYER_VIDEO; i++ )
	{
		savefile->WriteBool( videoHasBeenViewed[i] );
	}
	
	for( int i = 0; i < MAX_PLAYER_AUDIO; i++ )
	{
		for( int j = 0; j < MAX_PLAYER_AUDIO_ENTRIES; j++ )
		{
			savefile->WriteBool( audioHasBeenHeard[i][j] );
		}
	}
}

/*
===========
idPlayer::Restore
===========
*/
void idPlayer::Restore( idRestoreGame* savefile )
{
	int	  i;
	int	  num;
	float set;
	
	savefile->ReadUsercmd( usercmd );
	playerView.Restore( savefile );
	
	savefile->ReadBool( noclip );
	savefile->ReadBool( godmode );
	
	savefile->ReadAngles( spawnAngles );
	savefile->ReadAngles( viewAngles );
	savefile->ReadAngles( cmdAngles );
	
	memset( usercmd.angles, 0, sizeof( usercmd.angles ) );
	SetViewAngles( viewAngles );
	spawnAnglesSet = true;
	
	savefile->ReadInt( buttonMask );
	savefile->ReadInt( oldButtons );
	savefile->ReadInt( oldImpulseSequence );
	
	usercmd.impulseSequence = 0;
	oldImpulseSequence = 0;
	
	savefile->ReadInt( lastHitTime );
	savefile->ReadInt( lastSndHitTime );
	savefile->ReadInt( lastSavingThrowTime );
	
	// Re-link idBoolFields to the scriptObject, values will be restored in scriptObject's restore
	LinkScriptVariables();
	
	savefile->ReadObject( reinterpret_cast<idClass*&>( primaryObjective ) );
	inventory.Restore( savefile );
	weapon.Restore( savefile );
	
	if( hudManager != NULL )
	{
		hudManager->Initialize( "hud", common->SW() );
		hudManager->ActivateMenu( true );
		hud = hudManager->GetHud();
	}
	
	if( pdaMenu != NULL )
	{
		pdaMenu->Initialize( "pda", common->SW() );
	}
	
	for( i = 0; i < inventory.emails.Num(); i++ )
	{
		GetPDA()->AddEmail( inventory.emails[i] );
	}
	
	
	for( int i = 0; i < NUM_QUICK_SLOTS; ++i )
	{
		savefile->ReadInt( quickSlot[ i ] );
	}
	
	savefile->ReadInt( weapon_soulcube );
	savefile->ReadInt( weapon_pda );
	savefile->ReadInt( weapon_fists );
	savefile->ReadInt( weapon_flashlight );
	savefile->ReadInt( weapon_chainsaw );
	savefile->ReadInt( weapon_bloodstone );
	savefile->ReadInt( weapon_bloodstone_active1 );
	savefile->ReadInt( weapon_bloodstone_active2 );
	savefile->ReadInt( weapon_bloodstone_active3 );
	
	savefile->ReadBool( harvest_lock );
	savefile->ReadInt( hudPowerup );
	savefile->ReadInt( lastHudPowerup );
	savefile->ReadInt( hudPowerupDuration );
	
	
	
	savefile->ReadInt( heartRate );
	
	savefile->ReadFloat( set );
	heartInfo.SetStartTime( set );
	savefile->ReadFloat( set );
	heartInfo.SetDuration( set );
	savefile->ReadFloat( set );
	heartInfo.SetStartValue( set );
	savefile->ReadFloat( set );
	heartInfo.SetEndValue( set );
	
	savefile->ReadInt( lastHeartAdjust );
	savefile->ReadInt( lastHeartBeat );
	savefile->ReadInt( lastDmgTime );
	savefile->ReadInt( deathClearContentsTime );
	savefile->ReadBool( doingDeathSkin );
	savefile->ReadInt( lastArmorPulse );
	savefile->ReadFloat( stamina );
	savefile->ReadFloat( healthPool );
	savefile->ReadInt( nextHealthPulse );
	savefile->ReadBool( healthPulse );
	savefile->ReadInt( nextHealthTake );
	savefile->ReadBool( healthTake );
	
	savefile->ReadBool( hiddenWeapon );
	soulCubeProjectile.Restore( savefile );
	
	savefile->ReadInt( spectator );
	savefile->ReadBool( forceScoreBoard );
	savefile->ReadBool( forceRespawn );
	savefile->ReadBool( spectating );
	savefile->ReadInt( lastSpectateTeleport );
	savefile->ReadBool( lastHitToggle );
	savefile->ReadBool( wantSpectate );
	savefile->ReadBool( weaponGone );
	savefile->ReadBool( useInitialSpawns );
	savefile->ReadInt( tourneyRank );
	savefile->ReadInt( tourneyLine );
	
	teleportEntity.Restore( savefile );
	savefile->ReadInt( teleportKiller );
	
	savefile->ReadInt( minRespawnTime );
	savefile->ReadInt( maxRespawnTime );
	
	savefile->ReadVec3( firstPersonViewOrigin );
	savefile->ReadMat3( firstPersonViewAxis );
	
	// don't bother saving dragEntity since it's a dev tool
	dragEntity.Clear();
	
	savefile->ReadJoint( hipJoint );
	savefile->ReadJoint( chestJoint );
	savefile->ReadJoint( headJoint );
	
	savefile->ReadStaticObject( physicsObj );
	RestorePhysics( &physicsObj );
	
	savefile->ReadInt( num );
	aasLocation.SetGranularity( 1 );
	aasLocation.SetNum( num );
	for( i = 0; i < num; i++ )
	{
		savefile->ReadInt( aasLocation[ i ].areaNum );
		savefile->ReadVec3( aasLocation[ i ].pos );
	}
	
	savefile->ReadInt( bobFoot );
	savefile->ReadFloat( bobFrac );
	savefile->ReadFloat( bobfracsin );
	savefile->ReadInt( bobCycle );
	savefile->ReadFloat( xyspeed );
	savefile->ReadInt( stepUpTime );
	savefile->ReadFloat( stepUpDelta );
	savefile->ReadFloat( idealLegsYaw );
	savefile->ReadFloat( legsYaw );
	savefile->ReadBool( legsForward );
	savefile->ReadFloat( oldViewYaw );
	savefile->ReadAngles( viewBobAngles );
	savefile->ReadVec3( viewBob );
	savefile->ReadInt( landChange );
	savefile->ReadInt( landTime );
	
	savefile->ReadInt( currentWeapon );
	
	int savedIdealWeapon = -1;
	savefile->ReadInt( savedIdealWeapon );
	idealWeapon.Set( savedIdealWeapon );
	
	savefile->ReadInt( previousWeapon );
	savefile->ReadInt( weaponSwitchTime );
	savefile->ReadBool( weaponEnabled );
	
	savefile->ReadInt( skinIndex );
	savefile->ReadSkin( skin );
	savefile->ReadSkin( powerUpSkin );
	
	savefile->ReadInt( numProjectilesFired );
	savefile->ReadInt( numProjectileHits );
	
	savefile->ReadBool( airless );
	savefile->ReadInt( airMsec );
	savefile->ReadInt( lastAirDamage );
	
	savefile->ReadBool( gibDeath );
	savefile->ReadBool( gibsLaunched );
	savefile->ReadVec3( gibsDir );
	
	savefile->ReadFloat( set );
	zoomFov.SetStartTime( set );
	savefile->ReadFloat( set );
	zoomFov.SetDuration( set );
	savefile->ReadFloat( set );
	zoomFov.SetStartValue( set );
	savefile->ReadFloat( set );
	zoomFov.SetEndValue( set );
	
	savefile->ReadFloat( set );
	centerView.SetStartTime( set );
	savefile->ReadFloat( set );
	centerView.SetDuration( set );
	savefile->ReadFloat( set );
	centerView.SetStartValue( set );
	savefile->ReadFloat( set );
	centerView.SetEndValue( set );
	
	savefile->ReadBool( fxFov );
	
	savefile->ReadFloat( influenceFov );
	savefile->ReadInt( influenceActive );
	savefile->ReadFloat( influenceRadius );
	savefile->ReadObject( reinterpret_cast<idClass*&>( influenceEntity ) );
	savefile->ReadMaterial( influenceMaterial );
	savefile->ReadSkin( influenceSkin );
	
	savefile->ReadObject( reinterpret_cast<idClass*&>( privateCameraView ) );
	
	for( i = 0; i < NUM_LOGGED_VIEW_ANGLES; i++ )
	{
		savefile->ReadAngles( loggedViewAngles[ i ] );
	}
	for( i = 0; i < NUM_LOGGED_ACCELS; i++ )
	{
		savefile->ReadInt( loggedAccel[ i ].time );
		savefile->ReadVec3( loggedAccel[ i ].dir );
	}
	savefile->ReadInt( currentLoggedAccel );
	
	savefile->ReadObject( reinterpret_cast<idClass*&>( focusGUIent ) );
	// can't save focusUI
	focusUI = NULL;
	savefile->ReadObject( reinterpret_cast<idClass*&>( focusCharacter ) );
	savefile->ReadInt( talkCursor );
	savefile->ReadInt( focusTime );
	savefile->ReadObject( reinterpret_cast<idClass*&>( focusVehicle ) );
	savefile->ReadUserInterface( cursor );
	
	savefile->ReadInt( oldMouseX );
	savefile->ReadInt( oldMouseY );
	
	savefile->ReadBool( tipUp );
	savefile->ReadBool( objectiveUp );
	
	savefile->ReadInt( lastDamageDef );
	savefile->ReadVec3( lastDamageDir );
	savefile->ReadInt( lastDamageLocation );
	savefile->ReadInt( smoothedFrame );
	savefile->ReadBool( smoothedOriginUpdated );
	savefile->ReadVec3( smoothedOrigin );
	savefile->ReadAngles( smoothedAngles );
	
	savefile->ReadBool( respawning );
	savefile->ReadBool( leader );
	savefile->ReadInt( lastSpectateChange );
	savefile->ReadInt( lastTeleFX );
	
	// set the pm_ cvars
	const idKeyValue*	kv;
	kv = spawnArgs.MatchPrefix( "pm_", NULL );
	while( kv )
	{
		cvarSystem->SetCVarString( kv->GetKey(), kv->GetValue() );
		kv = spawnArgs.MatchPrefix( "pm_", kv );
	}
	
	savefile->ReadFloat( set );
	pm_stamina.SetFloat( set );
	
	// create combat collision hull for exact collision detection
	SetCombatModel();
	
	int weaponToggleCount;
	savefile->ReadInt( weaponToggleCount );
	for( i = 0; i < weaponToggleCount; i++ )
	{
		WeaponToggle_t newToggle;
		memset( &newToggle, 0, sizeof( newToggle ) );
		
		idStr name;
		savefile->ReadString( name );
		strcpy( newToggle.name, name.c_str() );
		
		int indexCount;
		savefile->ReadInt( indexCount );
		for( int j = 0; j < indexCount; j++ )
		{
			int temp;
			savefile->ReadInt( temp );
			newToggle.toggleList.Append( temp );
		}
		newToggle.lastUsed = 0;
		weaponToggles.Set( newToggle.name, newToggle );
	}
	savefile->ReadObject( reinterpret_cast<idClass*&>( mountedObject ) );
	enviroSuitLight.Restore( savefile );
	savefile->ReadBool( healthRecharge );
	savefile->ReadInt( lastHealthRechargeTime );
	savefile->ReadInt( rechargeSpeed );
	savefile->ReadFloat( new_g_damageScale );
	
	savefile->ReadBool( bloomEnabled );
	savefile->ReadFloat( bloomSpeed );
	savefile->ReadFloat( bloomIntensity );
	
	// flashlight
	idWeapon* tempWeapon;
	savefile->ReadObject( reinterpret_cast<idClass*&>( tempWeapon ) );
	tempWeapon->SetIsPlayerFlashlight( true );
	flashlight = tempWeapon;
	savefile->ReadInt( flashlightBattery );
	
	achievementManager.Restore( savefile );
	
	savefile->ReadInt( playedTimeSecs );
	savefile->ReadInt( playedTimeResidual );
	
	aimAssist.Init( this );
	
	laserSightHandle = -1;
	
	// re-init the laser model
	memset( &laserSightRenderEntity, 0, sizeof( laserSightRenderEntity ) );
	laserSightRenderEntity.hModel = renderModelManager->FindModel( "_BEAM" );
	laserSightRenderEntity.customShader = declManager->FindMaterial( "stereoRenderLaserSight" );
	
	for( int i = 0; i < MAX_PLAYER_PDA; i++ )
	{
		savefile->ReadBool( pdaHasBeenRead[i] );
	}
	
	for( int i = 0; i < MAX_PLAYER_VIDEO; i++ )
	{
		savefile->ReadBool( videoHasBeenViewed[i] );
	}
	
	for( int i = 0; i < MAX_PLAYER_AUDIO; i++ )
	{
		for( int j = 0; j < MAX_PLAYER_AUDIO_ENTRIES; j++ )
		{
			savefile->ReadBool( audioHasBeenHeard[i][j] );
		}
	}
	
	// Update the soul cube HUD indicator
	if( hud )
	{
		if( weapon_soulcube > 0 && ( inventory.weapons & ( 1 << weapon_soulcube ) ) )
		{
			int max_souls = inventory.MaxAmmoForAmmoClass( this, "ammo_souls" );
			if( inventory.GetInventoryAmmoForType( idWeapon::GetAmmoNumForName( "ammo_souls" ) ) >= max_souls )
			{
				hud->SetShowSoulCubeOnLoad( true );
			}
		}
	}
	
}

/*
===============
idPlayer::PrepareForRestart
================
*/
void idPlayer::PrepareForRestart()
{
	ClearPowerUps();
	
	if( common->IsClient() == false )
	{
		ServerSpectate( true );
	}
	
	forceRespawn = true;
	
	// Confirm reset hud states
	DropFlag();
	
	if( hud )
	{
		hud->SetFlagState( 0, 0 );
		hud->SetFlagState( 1, 0 );
	}
	
	// we will be restarting program, clear the client entities from program-related things first
	ShutdownThreads();
	
	// the sound world is going to be cleared, don't keep references to emitters
	FreeSoundEmitter( false );
}

/*
===============
idPlayer::Restart
================
*/
void idPlayer::Restart()
{
	idActor::Restart();
	
	// client needs to setup the animation script object again
	if( common->IsClient() )
	{
		// Make sure the weapon spawnId gets re-linked on the next snapshot.
		// Otherwise, its owner might not be set after the map restart, which causes asserts and crashes.
		weapon = NULL;
		flashlight = NULL;
		enviroSuitLight = NULL;
		Init();
	}
	else
	{
		// choose a random spot and prepare the point of view in case player is left spectating
		assert( spectating );
		SpawnFromSpawnSpot();
	}
	
	useInitialSpawns = true;
	UpdateSkinSetup();
}

/*
===============
idPlayer::ServerSpectate
================
*/
void idPlayer::ServerSpectate( bool spectate )
{
	assert( !common->IsClient() );
	
	if( spectating != spectate )
	{
		Spectate( spectate );
		if( !spectate )
		{
			// When coming out of spectate, join the team with the least number of players
			if( gameLocal.mpGame.IsGametypeTeamBased() )
			{
				int teamCounts[2] = { 0, 0 };
				gameLocal.mpGame.NumActualClients( false, teamCounts );
				teamCounts[team]--;
				if( teamCounts[0] < teamCounts[1] )
				{
					team = 0;
				}
				else if( teamCounts[1] < teamCounts[0] )
				{
					team = 1;
				}
				gameLocal.mpGame.SwitchToTeam( entityNumber, -1, team );
			}
			if( gameLocal.gameType == GAME_DM )
			{
				// make sure the scores are reset so you can't exploit by spectating and entering the game back
				// other game types don't matter, as you either can't join back, or it's team scores
				gameLocal.mpGame.ClearFrags( entityNumber );
			}
		}
	}
	if( !spectate )
	{
		SpawnFromSpawnSpot();
	}
	
	// drop the flag if player was carrying it
	if( spectate && common->IsMultiplayer() && gameLocal.mpGame.IsGametypeFlagBased() &&
			carryingFlag )
	{
		DropFlag();
	}
}

/*
===========
idPlayer::SelectInitialSpawnPoint

Try to find a spawn point marked 'initial', otherwise
use normal spawn selection.
============
*/
void idPlayer::SelectInitialSpawnPoint( idVec3& origin, idAngles& angles )
{
	idEntity* spot;
	idStr skin;
	
	spot = gameLocal.SelectInitialSpawnPoint( this );
	
	// set the player skin from the spawn location
	if( spot->spawnArgs.GetString( "skin", NULL, skin ) )
	{
		spawnArgs.Set( "spawn_skin", skin );
	}
	
	// activate the spawn locations targets
	spot->PostEventMS( &EV_ActivateTargets, 0, this );
	
	origin = spot->GetPhysics()->GetOrigin();
	origin[2] += 4.0f + CM_BOX_EPSILON;		// move up to make sure the player is at least an epsilon above the floor
	angles = spot->GetPhysics()->GetAxis().ToAngles();
}

/*
===========
idPlayer::SpawnFromSpawnSpot

Chooses a spawn location and spawns the player
============
*/
void idPlayer::SpawnFromSpawnSpot()
{
	idVec3		spawn_origin;
	idAngles	spawn_angles;
	
	SelectInitialSpawnPoint( spawn_origin, spawn_angles );
	SpawnToPoint( spawn_origin, spawn_angles );
}

/*
===========
idPlayer::SpawnToPoint

Called every time a client is placed fresh in the world:
after the first ClientBegin, and after each respawn
Initializes all non-persistant parts of playerState

when called here with spectating set to true, just place yourself and init
============
*/
void idPlayer::SpawnToPoint( const idVec3& spawn_origin, const idAngles& spawn_angles )
{
	idVec3 spec_origin;
	
	assert( !common->IsClient() );
	
	respawning = true;
	
	Init();
	
	fl.noknockback = false;
	
	// stop any ragdolls being used
	StopRagdoll();
	
	// set back the player physics
	SetPhysics( &physicsObj );
	
	physicsObj.SetClipModelAxis();
	physicsObj.EnableClip();
	
	if( !spectating )
	{
		SetCombatContents( true );
	}
	
	physicsObj.SetLinearVelocity( vec3_origin );
	
	// setup our initial view
	if( !spectating )
	{
		SetOrigin( spawn_origin );
	}
	else
	{
		spec_origin = spawn_origin;
		spec_origin[ 2 ] += pm_normalheight.GetFloat();
		spec_origin[ 2 ] += SPECTATE_RAISE;
		SetOrigin( spec_origin );
	}
	
	// if this is the first spawn of the map, we don't have a usercmd yet,
	// so the delta angles won't be correct.  This will be fixed on the first think.
	viewAngles = ang_zero;
	SetDeltaViewAngles( ang_zero );
	SetViewAngles( spawn_angles );
	spawnAngles = spawn_angles;
	spawnAnglesSet = false;
	
	legsForward = true;
	legsYaw = 0.0f;
	idealLegsYaw = 0.0f;
	oldViewYaw = viewAngles.yaw;
	
	if( spectating )
	{
		Hide();
	}
	else
	{
		Show();
	}
	
	if( common->IsMultiplayer() )
	{
		if( !spectating )
		{
			// we may be called twice in a row in some situations. avoid a double fx and 'fly to the roof'
			if( lastTeleFX < gameLocal.time - 1000 )
			{
				idEntityFx::StartFx( spawnArgs.GetString( "fx_spawn" ), &spawn_origin, NULL, this, true );
				lastTeleFX = gameLocal.time;
			}
		}
		AI_TELEPORT = true;
	}
	else
	{
		AI_TELEPORT = false;
	}
	
	// kill anything at the new position
	if( !spectating )
	{
		physicsObj.SetClipMask( MASK_PLAYERSOLID ); // the clip mask is usually maintained in Move(), but KillBox requires it
		gameLocal.KillBox( this );
	}
	
	// don't allow full run speed for a bit
	physicsObj.SetKnockBack( 100 );
	
	// set our respawn time and buttons so that if we're killed we don't respawn immediately
	minRespawnTime = gameLocal.time;
	maxRespawnTime = gameLocal.time;
	if( !spectating )
	{
		forceRespawn = false;
	}
	
	Respawn_Shared();
	
	privateCameraView = NULL;
	
	BecomeActive( TH_THINK );
	
	// run a client frame to drop exactly to the floor,
	// initialize animations and other things
	Think();
	
	respawning			= false;
	lastManOver			= false;
	lastManPlayAgain	= false;
	isTelefragged		= false;
}

/*
===============
idPlayer::Respawn_Shared
Called on server and client players when they respawn (including on initial spawn)
===============
*/
void idPlayer::Respawn_Shared()
{
	respawn_netEvent.Set();
	
	serverOverridePositionTime = gameLocal.GetServerGameTimeMs();
	
	// Remove the hud respawn message.
	HideRespawnHudMessage();
	
	FlashlightOff();
}

/*
===============
idPlayer::SavePersistantInfo

Saves any inventory and player stats when changing levels.
===============
*/
void idPlayer::SavePersistantInfo()
{
	idDict& playerInfo = gameLocal.persistentPlayerInfo[entityNumber];
	
	playerInfo.Clear();
	inventory.GetPersistantData( playerInfo );
	playerInfo.SetInt( "health", health );
	playerInfo.SetInt( "current_weapon", currentWeapon );
	playerInfo.SetInt( "playedTime", playedTimeSecs );
	
	achievementManager.SavePersistentData( playerInfo );
}

/*
===============
idPlayer::RestorePersistantInfo

Restores any inventory and player stats when changing levels.
===============
*/
void idPlayer::RestorePersistantInfo()
{
	if( common->IsMultiplayer() || g_demoMode.GetBool() )
	{
		gameLocal.persistentPlayerInfo[entityNumber].Clear();
	}
	
	spawnArgs.Copy( gameLocal.persistentPlayerInfo[entityNumber] );
	
	inventory.RestoreInventory( this, spawnArgs );
	health = spawnArgs.GetInt( "health", "100" );
	idealWeapon = spawnArgs.GetInt( "current_weapon", "1" );
	
	playedTimeSecs = spawnArgs.GetInt( "playedTime" );
	
	achievementManager.RestorePersistentData( spawnArgs );
}

/*
==============
idPlayer::UpdateSkinSetup
==============
*/
void idPlayer::UpdateSkinSetup()
{
	if( !common->IsMultiplayer() )
	{
		return;
	}
	
	if( gameLocal.mpGame.IsGametypeTeamBased() )    /* CTF */
	{
		skinIndex = team + 1;
	}
	else
	{
		// Each player will now have their Skin Index Reflect their entity number  ( host = 0, client 1 = 1, client 2 = 2 etc )
		skinIndex = entityNumber; // session->GetActingGameStateLobbyBase().GetLobbyUserSkinIndex( gameLocal.lobbyUserIDs[entityNumber] );
	}
	const char* baseSkinName = gameLocal.mpGame.GetSkinName( skinIndex );
	skin = declManager->FindSkin( baseSkinName, false );
	if( PowerUpActive( BERSERK ) )
	{
		idStr powerSkinName = baseSkinName;
		powerSkinName.Append( "_berserk" );
		powerUpSkin = declManager->FindSkin( powerSkinName );
	}
	else if( PowerUpActive( INVULNERABILITY ) )
	{
		idStr powerSkinName = baseSkinName;
		powerSkinName.Append( "_invuln" );
		powerUpSkin = declManager->FindSkin( powerSkinName );
	}
	else if( PowerUpActive( INVISIBILITY ) )
	{
		const char* invisibleSkin = "";
		spawnArgs.GetString( "skin_invisibility", "", &invisibleSkin );
		powerUpSkin = declManager->FindSkin( invisibleSkin );
	}
}

/*
===============
idPlayer::UpdateHudStats
===============
*/
void idPlayer::UpdateHudStats( idMenuHandler_HUD* _hudManager )
{

	if( _hudManager && _hudManager->GetHud() )
	{
	
		idMenuScreen_HUD* hud = _hudManager->GetHud();
		hud->UpdateHealthArmor( this );
		hud->UpdateStamina( this );
		hud->UpdateWeaponInfo( this );
		
		if( inventory.weaponPulse )
		{
			UpdateHudWeapon();
			inventory.weaponPulse = false;
		}
		
		if( gameLocal.mpGame.IsGametypeFlagBased() )
		{
			hud->SetFlagState( 0, gameLocal.mpGame.GetFlagStatus( 0 ) );
			hud->SetFlagState( 1, gameLocal.mpGame.GetFlagStatus( 1 ) );
			
			hud->SetTeamScore( 0, gameLocal.mpGame.GetFlagPoints( 0 ) );
			hud->SetTeamScore( 1, gameLocal.mpGame.GetFlagPoints( 1 ) );
			
			hud->SetTeam( team );
		}
		
	}
}

/*
===============
idPlayer::UpdateHudWeapon
===============
*/
void idPlayer::UpdateHudWeapon( bool flashWeapon )
{

	idMenuScreen_HUD* curDisplay = hud;
	idPlayer* p = this;
	if( gameLocal.GetLocalClientNum() >= 0 && gameLocal.entities[ gameLocal.GetLocalClientNum() ] && gameLocal.entities[ gameLocal.GetLocalClientNum() ]->IsType( idPlayer::Type ) )
	{
		p = static_cast< idPlayer* >( gameLocal.entities[ gameLocal.GetLocalClientNum() ] );
		if( p->spectating && p->spectator == entityNumber )
		{
			assert( p->hud );
			curDisplay = p->hud;
		}
	}
	
	if( !curDisplay )
	{
		return;
	}
	
	curDisplay->UpdateWeaponStates( p, flashWeapon );
}

/*
===============
idPlayer::UpdateHudWeapon
===============
*/
void idPlayer::UpdateChattingHud()
{

	idMenuScreen_HUD* curDisplay = hud;
	idPlayer* p = this;
	if( gameLocal.GetLocalClientNum() >= 0 && gameLocal.entities[ gameLocal.GetLocalClientNum() ] && gameLocal.entities[ gameLocal.GetLocalClientNum() ]->IsType( idPlayer::Type ) )
	{
		p = static_cast< idPlayer* >( gameLocal.entities[ gameLocal.GetLocalClientNum() ] );
		if( p->spectating && p->spectator == entityNumber )
		{
			assert( p->hud );
			curDisplay = p->hud;
		}
	}
	
	if( !curDisplay )
	{
		return;
	}
	
	curDisplay->UpdateChattingHud( p );
}


/*
========================
idMenuScreen_Scoreboard::UpdateSpectating
========================
*/
void idPlayer::UpdateSpectatingText()
{

	idSWF* spectatorMessages = mpMessages;
	idPlayer* p = this;
	if( gameLocal.GetLocalClientNum() >= 0 && gameLocal.entities[ gameLocal.GetLocalClientNum() ] && gameLocal.entities[ gameLocal.GetLocalClientNum() ]->IsType( idPlayer::Type ) )
	{
		p = static_cast< idPlayer* >( gameLocal.entities[ gameLocal.GetLocalClientNum() ] );
		if( p && p->spectating )
		{
			spectatorMessages = p->mpMessages;
		}
	}
	
	if( !spectatorMessages || !spectatorMessages->IsActive() )
	{
		return;
	}
	
	idPlayer* viewPlayer = static_cast<idPlayer*>( gameLocal.entities[ p->spectator ] );
	if( viewPlayer == NULL )
	{
		return;
	}
	
	idStr spectatetext[ 2 ];
	if( !gameLocal.mpGame.IsScoreboardActive() )
	{
		gameLocal.mpGame.GetSpectateText( p, spectatetext, false );
	}
	
	idSWFScriptObject& root = spectatorMessages->GetRootObject();
	idSWFTextInstance* txtVal = root.GetNestedText( "txtSpectating" );
	if( txtVal != NULL )
	{
		txtVal->tooltip = true;
		txtVal->SetText( spectatetext[0] );
		txtVal->SetStrokeInfo( true, 0.75f, 1.75f );
	}
	
	txtVal = root.GetNestedText( "txtFollow" );
	if( txtVal != NULL )
	{
		txtVal->SetText( spectatetext[1] );
		txtVal->SetStrokeInfo( true, 0.75f, 1.75f );
	}
}

/*
===============
idPlayer::UpdateMpMessages
===============
*/
void idPlayer::AddChatMessage( int index, int alpha, const idStr& message )
{

	if( mpMessages == NULL || !mpMessages->IsActive() )
	{
		return;
	}
	
	idSWFScriptObject* mpChat = mpMessages->GetRootObject().GetNestedObj( "_left", "mpChat" );
	
	idSWFSpriteInstance* info = mpChat->GetNestedSprite( va( "info%i", index ) );
	idSWFTextInstance* txtVal = mpChat->GetNestedText( va( "info%i", index ), "txtVal" );
	if( info )
	{
		info->SetVisible( true );
		if( alpha >= 4 )
		{
			info->SetAlpha( 1.0f );
		}
		else if( alpha == 3 )
		{
			info->SetAlpha( 0.875f );
		}
		else if( alpha == 2 )
		{
			info->SetAlpha( 0.75f );
		}
		else if( alpha == 1 )
		{
			info->SetAlpha( 0.625f );
		}
		else
		{
			info->SetAlpha( 0.5f );
		}
	}
	
	if( txtVal )
	{
		txtVal->SetIgnoreColor( false );
		txtVal->SetText( message );
		txtVal->SetStrokeInfo( true, 0.9f, 1.75f );
	}
	
}

/*
===============
idPlayer::UpdateMpMessages
===============
*/
void idPlayer::ClearChatMessage( int index )
{

	if( mpMessages == NULL || !mpMessages->IsActive() )
	{
		return;
	}
	
	idSWFScriptObject* mpChat = mpMessages->GetRootObject().GetNestedObj( "_left", "mpChat" );
	
	idSWFSpriteInstance* info = mpChat->GetNestedSprite( va( "info%i", index ) );
	idSWFTextInstance* txtVal = mpChat->GetNestedText( va( "info%i", index ), "txtVal" );
	if( info )
	{
		info->SetVisible( false );
	}
	
	if( txtVal )
	{
		txtVal->SetText( "" );
	}
	
}

/*
===============
idPlayer::DrawHUD
===============
*/
void idPlayer::DrawHUD( idMenuHandler_HUD* _hudManager )
{
	SCOPED_PROFILE_EVENT( "idPlayer::DrawHUD" );
	
	if( !weapon.GetEntity() || influenceActive != INFLUENCE_NONE || privateCameraView || gameLocal.GetCamera() || !g_showHud.GetBool() )
	{
		return;
	}
	
	if( common->IsMultiplayer() )
	{
		UpdateChattingHud();
		UpdateSpectatingText();
	}
	
	// Always draw the local client's messages so that chat works correctly while spectating another player.
	idPlayer* localPlayer = static_cast< idPlayer* >( gameLocal.entities[ gameLocal.GetLocalClientNum() ] );
	
	if( localPlayer != NULL && localPlayer->mpMessages != NULL )
	{
		localPlayer->mpMessages->Render( renderSystem, Sys_Milliseconds() );
	}
	
	
	UpdateHudStats( _hudManager );
	
	if( spectating )
	{
		return;
	}
	
	if( _hudManager )
	{
		_hudManager->Update();
	}
	
	weapon.GetEntity()->UpdateGUI();
	
	// weapon targeting crosshair
	if( !GuiActive() )
	{
		// don't show the 2D crosshair in stereo rendering, use the
		// laser sight model instead
		if( _hudManager && _hudManager->GetHud() )
		{
		
			idMenuScreen_HUD* hud = _hudManager->GetHud();
			
			if( weapon.GetEntity()->ShowCrosshair() && !IsGameStereoRendered() )
			{
				if( weapon.GetEntity()->GetGrabberState() == 1 || weapon.GetEntity()->GetGrabberState() == 2 )
				{
					hud->SetCursorState( this, CURSOR_GRABBER, 1 );
					hud->SetCursorState( this, CURSOR_IN_COMBAT, 0 );
				}
				else
				{
					hud->SetCursorState( this, CURSOR_GRABBER, 0 );
					hud->SetCursorState( this, CURSOR_IN_COMBAT, 1 );
				}
			}
			else
			{
				hud->SetCursorState( this, CURSOR_NONE, 1 );
			}
			
			hud->UpdateCursorState();
			
		}
	}
	else if( _hudManager && _hudManager->GetHud() )
	{
	
		idMenuScreen_HUD* hud = _hudManager->GetHud();
		
		hud->SetCursorState( this, CURSOR_NONE, 1 );
		hud->UpdateCursorState();
	}
}

/*
===============
idPlayer::EnterCinematic
===============
*/
void idPlayer::EnterCinematic()
{
	if( PowerUpActive( HELLTIME ) )
	{
		StopHelltime();
	}
	
	Hide();
	StopSound( SND_CHANNEL_PDA_AUDIO, false );
	StopSound( SND_CHANNEL_PDA_VIDEO, false );
	
	if( hudManager )
	{
		hudManager->SetRadioMessage( false );
	}
	physicsObj.SetLinearVelocity( vec3_origin );
	
	SetState( "EnterCinematic" );
	UpdateScript();
	
	if( weaponEnabled && weapon.GetEntity() )
	{
		weapon.GetEntity()->EnterCinematic();
	}
	if( flashlight.GetEntity() )
	{
		flashlight.GetEntity()->EnterCinematic();
	}
	
	AI_FORWARD		= false;
	AI_BACKWARD		= false;
	AI_STRAFE_LEFT	= false;
	AI_STRAFE_RIGHT	= false;
	AI_RUN			= false;
	AI_ATTACK_HELD	= false;
	AI_WEAPON_FIRED	= false;
	AI_JUMP			= false;
	AI_CROUCH		= false;
	AI_ONGROUND		= true;
	AI_ONLADDER		= false;
	AI_DEAD			= ( health <= 0 );
	AI_RUN			= false;
	AI_PAIN			= false;
	AI_HARDLANDING	= false;
	AI_SOFTLANDING	= false;
	AI_RELOAD		= false;
	AI_TELEPORT		= false;
	AI_TURN_LEFT	= false;
	AI_TURN_RIGHT	= false;
}

/*
===============
idPlayer::ExitCinematic
===============
*/
void idPlayer::ExitCinematic()
{
	Show();
	
	if( weaponEnabled && weapon.GetEntity() )
	{
		weapon.GetEntity()->ExitCinematic();
	}
	if( flashlight.GetEntity() )
	{
		flashlight.GetEntity()->ExitCinematic();
	}
	
	// long cinematics would have surpassed the healthTakeTime, causing the player to take damage
	// immediately after the cinematic ends.  Instead we start the healthTake cooldown again once
	// the cinematic ends.
	if( g_skill.GetInteger() == 3 )
	{
		nextHealthTake = gameLocal.time + g_healthTakeTime.GetInteger() * 1000;
	}
	
	SetState( "ExitCinematic" );
	UpdateScript();
}

/*
=====================
idPlayer::UpdateConditions
=====================
*/
void idPlayer::UpdateConditions()
{
	idVec3	velocity;
	float	fallspeed;
	float	forwardspeed;
	float	sidespeed;
	
	// minus the push velocity to avoid playing the walking animation and sounds when riding a mover
	velocity = physicsObj.GetLinearVelocity() - physicsObj.GetPushedLinearVelocity();
	fallspeed = velocity * physicsObj.GetGravityNormal();
	
	if( influenceActive )
	{
		AI_FORWARD		= false;
		AI_BACKWARD		= false;
		AI_STRAFE_LEFT	= false;
		AI_STRAFE_RIGHT	= false;
	}
	else if( gameLocal.time - lastDmgTime < 500 )
	{
		forwardspeed = velocity * viewAxis[ 0 ];
		sidespeed = velocity * viewAxis[ 1 ];
		AI_FORWARD		= AI_ONGROUND && ( forwardspeed > 20.01f );
		AI_BACKWARD		= AI_ONGROUND && ( forwardspeed < -20.01f );
		AI_STRAFE_LEFT	= AI_ONGROUND && ( sidespeed > 20.01f );
		AI_STRAFE_RIGHT	= AI_ONGROUND && ( sidespeed < -20.01f );
	}
	else if( xyspeed > MIN_BOB_SPEED )
	{
		AI_FORWARD		= AI_ONGROUND && ( usercmd.forwardmove > 0 );
		AI_BACKWARD		= AI_ONGROUND && ( usercmd.forwardmove < 0 );
		AI_STRAFE_LEFT	= AI_ONGROUND && ( usercmd.rightmove < 0 );
		AI_STRAFE_RIGHT	= AI_ONGROUND && ( usercmd.rightmove > 0 );
	}
	else
	{
		AI_FORWARD		= false;
		AI_BACKWARD		= false;
		AI_STRAFE_LEFT	= false;
		AI_STRAFE_RIGHT	= false;
	}
	
	AI_RUN			= ( usercmd.buttons & BUTTON_RUN ) && ( ( !pm_stamina.GetFloat() ) || ( stamina > pm_staminathreshold.GetFloat() ) );
	AI_DEAD			= ( health <= 0 );
}

/*
==================
WeaponFireFeedback

Called when a weapon fires, generates head twitches, etc
==================
*/
void idPlayer::WeaponFireFeedback( const idDict* weaponDef )
{
	// force a blink
	blink_time = 0;
	
	// play the fire animation
	AI_WEAPON_FIRED = true;
	
	// update view feedback
	playerView.WeaponFireFeedback( weaponDef );
	
	// shake controller
	float highMagnitude = weaponDef->GetFloat( "controllerShakeHighMag" );
	int highDuration = weaponDef->GetInt( "controllerShakeHighTime" );
	float lowMagnitude = weaponDef->GetFloat( "controllerShakeLowMag" );
	int lowDuration = weaponDef->GetInt( "controllerShakeLowTime" );
	//const char *name = weaponDef->GetString( "inv_name" );
	
	if( IsLocallyControlled() )
	{
		SetControllerShake( highMagnitude, highDuration, lowMagnitude, lowDuration );
	}
}

/*
===============
idPlayer::StopFiring
===============
*/
void idPlayer::StopFiring()
{
	AI_ATTACK_HELD	= false;
	AI_WEAPON_FIRED = false;
	AI_RELOAD		= false;
	if( weapon.GetEntity() )
	{
		weapon.GetEntity()->EndAttack();
	}
}

/*
===============
idPlayer::FireWeapon
===============
*/
idCVar g_infiniteAmmo( "g_infiniteAmmo", "0", CVAR_GAME | CVAR_BOOL, "infinite ammo" );
extern idCVar ui_autoSwitch;
void idPlayer::FireWeapon()
{
	idMat3 axis;
	idVec3 muzzle;
	
	if( privateCameraView )
	{
		return;
	}
	
	if( g_editEntityMode.GetInteger() )
	{
		GetViewPos( muzzle, axis );
		if( gameLocal.editEntities->SelectEntity( muzzle, axis[0], this ) )
		{
			return;
		}
	}
	
	if( !hiddenWeapon && weapon.GetEntity()->IsReady() )
	{
		if( g_infiniteAmmo.GetBool() || weapon.GetEntity()->AmmoInClip() || weapon.GetEntity()->AmmoAvailable() )
		{
			AI_ATTACK_HELD = true;
			weapon.GetEntity()->BeginAttack();
			if( ( weapon_soulcube >= 0 ) && ( currentWeapon == weapon_soulcube ) )
			{
				if( hud )
				{
					hud->UpdateSoulCube( false );
				}
				SelectWeapon( previousWeapon, false );
			}
			if( ( weapon_bloodstone >= 0 ) && ( currentWeapon == weapon_bloodstone ) && inventory.weapons & ( 1 << weapon_bloodstone_active1 ) && weapon.GetEntity()->GetStatus() == WP_READY )
			{
				// tell it to switch to the previous weapon. Only do this once to prevent
				// weapon toggling messing up the previous weapon
				if( idealWeapon == weapon_bloodstone )
				{
					if( previousWeapon == weapon_bloodstone || previousWeapon == -1 )
					{
						NextBestWeapon();
					}
					else
					{
						//Since this is a toggle weapon just select itself and it will toggle to the last weapon
						SelectWeapon( weapon_bloodstone, false );
					}
				}
			}
		}
		else
		{
		
			idLobbyBase& lobby = session->GetActingGameStateLobbyBase();
			lobbyUserID_t& lobbyUserID = gameLocal.lobbyUserIDs[ entityNumber ];
			bool autoSwitch = lobby.GetLobbyUserWeaponAutoSwitch( lobbyUserID );
			if( !autoSwitch )
			{
				return;
			}
			
			// update our ammo clip in our inventory
			if( ( currentWeapon >= 0 ) && ( currentWeapon < MAX_WEAPONS ) )
			{
				inventory.SetClipAmmoForWeapon( currentWeapon, weapon.GetEntity()->AmmoInClip() );
			}
			
			NextBestWeapon();
		}
	}
	
	
	if( tipUp )
	{
		HideTip();
	}
	
	if( objectiveUp )
	{
		HideObjective();
	}
}

/*
===============
idPlayer::CacheWeapons
===============
*/
void idPlayer::CacheWeapons()
{
	idStr	weap;
	int		w;
	
	// check if we have any weapons
	if( !inventory.weapons )
	{
		return;
	}
	
	for( w = 0; w < MAX_WEAPONS; w++ )
	{
		if( inventory.weapons & ( 1 << w ) )
		{
			weap = spawnArgs.GetString( va( "def_weapon%d", w ) );
			if( weap != "" )
			{
				idWeapon::CacheWeapon( weap );
			}
			else
			{
				inventory.weapons &= ~( 1 << w );
			}
		}
	}
}

/*
===============
idPlayer::SetQuickSlot
===============
*/
void idPlayer::SetQuickSlot( int index, int val )
{
	if( index >= NUM_QUICK_SLOTS || index < 0 )
	{
		return;
	}
	
	quickSlot[ index ] = val;
}

/*
===============
idPlayer::GetQuickSlot
===============
*/
int idPlayer::GetQuickSlot( int index )
{

	if( index >= NUM_QUICK_SLOTS || index < 0 )
	{
		return -1;
	}
	
	return quickSlot[ index ];
}

/*
===============
idPlayer::Give
===============
*/
bool idPlayer::Give( const char* statname, const char* value, unsigned int giveFlags )
{
	int amount;
	
	if( AI_DEAD )
	{
		return false;
	}
	
	if( !idStr::Icmp( statname, "health" ) )
	{
		if( health >= inventory.maxHealth )
		{
			return false;
		}
		if( giveFlags & ITEM_GIVE_UPDATE_STATE )
		{
			amount = atoi( value );
			if( amount )
			{
				health += amount;
				if( health > inventory.maxHealth )
				{
					health = inventory.maxHealth;
				}
				healthPulse = true;
			}
		}
	}
	else if( !idStr::Icmp( statname, "stamina" ) )
	{
		if( stamina >= 100 )
		{
			return false;
		}
		if( giveFlags & ITEM_GIVE_UPDATE_STATE )
		{
			stamina += atof( value );
			if( stamina > 100 )
			{
				stamina = 100;
			}
		}
	}
	else if( !idStr::Icmp( statname, "heartRate" ) )
	{
		if( giveFlags & ITEM_GIVE_UPDATE_STATE )
		{
			heartRate += atoi( value );
			if( heartRate > MAX_HEARTRATE )
			{
				heartRate = MAX_HEARTRATE;
			}
		}
	}
	else if( !idStr::Icmp( statname, "air" ) )
	{
		if( airMsec >= pm_airMsec.GetInteger() )
		{
			return false;
		}
		if( giveFlags & ITEM_GIVE_UPDATE_STATE )
		{
			airMsec += pm_airMsec.GetInteger() * atoi( value ) / 100;
			if( airMsec > pm_airMsec.GetInteger() )
			{
				airMsec = pm_airMsec.GetInteger();
			}
		}
	}
	else if( !idStr::Icmp( statname, "enviroTime" ) )
	{
		if( ( giveFlags & ITEM_GIVE_UPDATE_STATE ) && PowerUpActive( ENVIROTIME ) )
		{
			inventory.powerupEndTime[ ENVIROTIME ] += ( atof( value ) * 1000 );
		}
		else
		{
			GivePowerUp( ENVIROTIME, atoi( value ) * 1000, giveFlags );
		}
	}
	else
	{
		bool ret = inventory.Give( this, spawnArgs, statname, value, &idealWeapon, true, giveFlags );
		return ret;
	}
	return true;
}


/*
===============
idPlayer::GiveHealthPool

adds health to the player health pool
===============
*/
void idPlayer::GiveHealthPool( float amt )
{

	if( AI_DEAD )
	{
		return;
	}
	
	if( health > 0 )
	{
		healthPool += amt;
		if( healthPool > inventory.maxHealth - health )
		{
			healthPool = inventory.maxHealth - health;
		}
		nextHealthPulse = gameLocal.time;
	}
}

/*
===============
idPlayer::GiveItem

Returns false if the item shouldn't be picked up
===============
*/
bool idPlayer::GiveItem( idItem* item, unsigned int giveFlags )
{
	int					i;
	const idKeyValue*	arg;
	idDict				attr;
	bool				gave;
	int					numPickup;
	
	if( common->IsMultiplayer() && spectating )
	{
		return false;
	}
	
	if( idStr::FindText( item->GetName(), "weapon_flashlight_new" ) > -1 )
	{
		return false;
	}
	
	if( idStr::FindText( item->GetName(), "weapon_flashlight" ) > -1 )
	{
		// don't allow flashlight weapon unless classic mode is enabled
		return false;
	}
	
	item->GetAttributes( attr );
	
	gave = false;
	numPickup = inventory.pickupItemNames.Num();
	for( i = 0; i < attr.GetNumKeyVals(); i++ )
	{
		arg = attr.GetKeyVal( i );
		if( Give( arg->GetKey(), arg->GetValue(), giveFlags ) )
		{
			gave = true;
		}
	}
	
	if( giveFlags & ITEM_GIVE_FEEDBACK )
	{
		arg = item->spawnArgs.MatchPrefix( "inv_weapon", NULL );
		if( arg )
		{
			// We need to update the weapon hud manually, but not
			// the armor/ammo/health because they are updated every
			// frame no matter what
			UpdateHudWeapon( false );
		}
		
		// display the pickup feedback on the hud
		if( gave && ( numPickup == inventory.pickupItemNames.Num() ) )
		{
			inventory.AddPickupName( item->spawnArgs.GetString( "inv_name" ), this ); //_D3XP
		}
	}
	
	return gave;
}

/*
===============
idPlayer::PowerUpModifier
===============
*/
float idPlayer::PowerUpModifier( int type )
{
	float mod = 1.0f;
	
	if( PowerUpActive( BERSERK ) )
	{
		switch( type )
		{
			case SPEED:
			{
				mod *= 1.7f;
				break;
			}
			case PROJECTILE_DAMAGE:
			{
				mod *= 2.0f;
				break;
			}
			case MELEE_DAMAGE:
			{
				mod *= 30.0f;
				break;
			}
			case MELEE_DISTANCE:
			{
				mod *= 2.0f;
				break;
			}
		}
	}
	
	if( common->IsMultiplayer() && !common->IsClient() )
	{
		if( PowerUpActive( MEGAHEALTH ) )
		{
			if( healthPool <= 0 )
			{
				GiveHealthPool( 100 );
			}
		}
		else
		{
			healthPool = 0;
		}
		
		/*if( PowerUpActive( HASTE ) ) {
			switch( type ) {
			case SPEED: {
				mod = 1.7f;
				break;
						}
			}
		}*/
	}
	
	return mod;
}

/*
===============
idPlayer::PowerUpActive
===============
*/
bool idPlayer::PowerUpActive( int powerup ) const
{
	return ( inventory.powerups & ( 1 << powerup ) ) != 0;
}

/*
===============
idPlayer::GivePowerUp
===============
*/
bool idPlayer::GivePowerUp( int powerup, int time, unsigned int giveFlags )
{
	const char* sound;
	
	if( powerup >= 0 && powerup < MAX_POWERUPS )
	{
	
		if( giveFlags & ITEM_GIVE_UPDATE_STATE )
		{
			if( common->IsServer() )
			{
				idBitMsg	msg;
				byte		msgBuf[MAX_EVENT_PARAM_SIZE];
				
				msg.InitWrite( msgBuf, sizeof( msgBuf ) );
				msg.WriteShort( powerup );
				msg.WriteShort( time );
				ServerSendEvent( EVENT_POWERUP, &msg, false );
			}
			
			if( powerup != MEGAHEALTH )
			{
				inventory.GivePowerUp( this, powerup, time );
			}
		}
		
		switch( powerup )
		{
			case BERSERK:
			{
				if( giveFlags & ITEM_GIVE_FROM_WEAPON )
				{
					// Berserk is granted by the bloodstone in ROE, but we don't want any of the
					// standard behavior (sound fx, switch to fists) when you get it this way.
				}
				else
				{
					if( giveFlags & ITEM_GIVE_FEEDBACK )
					{
						inventory.AddPickupName( "#str_00100627", this );
						
						if( spawnArgs.GetString( "snd_berserk_third", "", &sound ) && sound[ 0 ] != '\0' )
						{
							StartSoundShader( declManager->FindSound( sound ), SND_CHANNEL_DEMONIC, 0, false, NULL );
						}
					}
					
					if( giveFlags & ITEM_GIVE_UPDATE_STATE )
					{
						if( !common->IsClient() )
						{
							idealWeapon = weapon_fists;
						}
					}
				}
				break;
			}
			case INVISIBILITY:
			{
				if( common->IsMultiplayer() && ( giveFlags & ITEM_GIVE_FEEDBACK ) )
				{
					inventory.AddPickupName( "#str_00100628", this );
				}
				if( giveFlags & ITEM_GIVE_UPDATE_STATE )
				{
					// remove any decals from the model
					if( modelDefHandle != -1 )
					{
						gameRenderWorld->RemoveDecals( modelDefHandle );
					}
					if( weapon.GetEntity() )
					{
						weapon.GetEntity()->UpdateSkin();
					}
					if( flashlight.GetEntity() )
					{
						flashlight.GetEntity()->UpdateSkin();
					}
				}
				
				/*				if ( spawnArgs.GetString( "snd_invisibility", "", &sound ) ) {
									StartSoundShader( declManager->FindSound( sound ), SND_CHANNEL_ANY, 0, false, NULL );
								} */
				break;
			}
			case ADRENALINE:
			{
				if( giveFlags & ITEM_GIVE_FEEDBACK )
				{
					inventory.AddPickupName( "#str_00100799", this );
				}
				
				if( giveFlags & ITEM_GIVE_UPDATE_STATE )
				{
					stamina = 100.0f;
				}
				break;
			}
			case MEGAHEALTH:
			{
				if( giveFlags & ITEM_GIVE_FEEDBACK )
				{
					if( common->IsMultiplayer() )
					{
						inventory.AddPickupName( "#str_00100629", this );
					}
					if( spawnArgs.GetString( "snd_megahealth", "", &sound ) )
					{
						StartSoundShader( declManager->FindSound( sound ), SND_CHANNEL_ANY, 0, false, NULL );
					}
				}
				if( giveFlags & ITEM_GIVE_UPDATE_STATE )
				{
					health = 200;
				}
				break;
			}
			case HELLTIME:
			{
				if( spawnArgs.GetString( "snd_helltime_start", "", &sound ) )
				{
					PostEventMS( &EV_StartSoundShader, 0, sound, SND_CHANNEL_ANY );
				}
				if( spawnArgs.GetString( "snd_helltime_loop", "", &sound ) )
				{
					PostEventMS( &EV_StartSoundShader, 0, sound, SND_CHANNEL_DEMONIC );
				}
				break;
			}
			case ENVIROSUIT:
			{
				if( giveFlags & ITEM_GIVE_FEEDBACK )
				{
					// Turn on the envirosuit sound
					if( gameSoundWorld )
					{
						gameSoundWorld->SetEnviroSuit( true );
					}
				}
				
				if( giveFlags & ITEM_GIVE_UPDATE_STATE )
				{
					// Put the helmet and lights on the player
					idDict	args;
					
					// Light
					const idDict* lightDef = gameLocal.FindEntityDefDict( "envirosuit_light", false );
					if( lightDef )
					{
						idEntity* temp;
						gameLocal.SpawnEntityDef( *lightDef, &temp, false );
						
						idLight* eLight = static_cast<idLight*>( temp );
						eLight->GetPhysics()->SetOrigin( firstPersonViewOrigin );
						eLight->UpdateVisuals();
						eLight->Present();
						
						enviroSuitLight = eLight;
					}
				}
				break;
			}
			case ENVIROTIME:
			{
				if( giveFlags & ITEM_GIVE_UPDATE_STATE )
				{
					hudPowerup = ENVIROTIME;
					// The HUD display bar is fixed at 60 seconds
					hudPowerupDuration = 60000;
				}
				break;
			}
			case INVULNERABILITY:
			{
				if( common->IsMultiplayer() && ( giveFlags & ITEM_GIVE_FEEDBACK ) )
				{
					inventory.AddPickupName( "#str_00100630", this );
				}
				break;
			}
		}
		
		if( giveFlags & ITEM_GIVE_UPDATE_STATE )
		{
			UpdateSkinSetup();
		}
		
		return true;
	}
	else
	{
		gameLocal.Warning( "Player given power up %i\n which is out of range", powerup );
	}
	return false;
}

/*
==============
idPlayer::ClearPowerup
==============
*/
void idPlayer::ClearPowerup( int i )
{

	if( common->IsServer() )
	{
		idBitMsg	msg;
		byte		msgBuf[MAX_EVENT_PARAM_SIZE];
		
		msg.InitWrite( msgBuf, sizeof( msgBuf ) );
		msg.WriteShort( i );
		msg.WriteShort( 0 );
		ServerSendEvent( EVENT_POWERUP, &msg, false );
	}
	
	powerUpSkin = NULL;
	inventory.powerups &= ~( 1 << i );
	inventory.powerupEndTime[ i ] = 0;
	switch( i )
	{
		case BERSERK:
		{
			if( common->IsMultiplayer() )
			{
				StopSound( SND_CHANNEL_DEMONIC, false );
			}
			if( !common->IsMultiplayer() )
			{
				StopHealthRecharge();
			}
			break;
		}
		case INVISIBILITY:
		{
			if( weapon.GetEntity() )
			{
				weapon.GetEntity()->UpdateSkin();
			}
			if( flashlight.GetEntity() )
			{
				flashlight.GetEntity()->UpdateSkin();
			}
			break;
		}
		case HELLTIME:
		{
			GetAchievementManager().ResetHellTimeKills();
			StopSound( SND_CHANNEL_DEMONIC, false );
			break;
		}
		case ENVIROSUIT:
		{
		
			hudPowerup = -1;
			
			// Turn off the envirosuit sound
			if( gameSoundWorld )
			{
				gameSoundWorld->SetEnviroSuit( false );
			}
			
			// Take off the helmet and lights
			if( enviroSuitLight.IsValid() )
			{
				enviroSuitLight.GetEntity()->PostEventMS( &EV_Remove, 0 );
			}
			enviroSuitLight = NULL;
			break;
		}
		case INVULNERABILITY:
		{
			if( common->IsMultiplayer() )
			{
				StopSound( SND_CHANNEL_DEMONIC, false );
			}
		}
			/*case HASTE: {
				if(common->IsMultiplayer()) {
					StopSound( SND_CHANNEL_DEMONIC, false );
				}
			}*/
	}
}

/*
==============
idPlayer::UpdatePowerUps
==============
*/
void idPlayer::UpdatePowerUps()
{
	int i;
	
	if( !common->IsClient() )
	{
		for( i = 0; i < MAX_POWERUPS; i++ )
		{
			if( ( inventory.powerups & ( 1 << i ) ) && inventory.powerupEndTime[i] > gameLocal.time )
			{
				switch( i )
				{
					case ENVIROSUIT:
					{
						if( enviroSuitLight.IsValid() )
						{
							idAngles lightAng = firstPersonViewAxis.ToAngles();
							idVec3 lightOrg = firstPersonViewOrigin;
							const idDict* lightDef = gameLocal.FindEntityDefDict( "envirosuit_light", false );
							
							idVec3 enviroOffset = lightDef->GetVector( "enviro_offset" );
							idVec3 enviroAngleOffset = lightDef->GetVector( "enviro_angle_offset" );
							
							lightOrg += ( enviroOffset.x * firstPersonViewAxis[0] );
							lightOrg += ( enviroOffset.y * firstPersonViewAxis[1] );
							lightOrg += ( enviroOffset.z * firstPersonViewAxis[2] );
							lightAng.pitch += enviroAngleOffset.x;
							lightAng.yaw += enviroAngleOffset.y;
							lightAng.roll += enviroAngleOffset.z;
							
							enviroSuitLight.GetEntity()->GetPhysics()->SetOrigin( lightOrg );
							enviroSuitLight.GetEntity()->GetPhysics()->SetAxis( lightAng.ToMat3() );
							enviroSuitLight.GetEntity()->UpdateVisuals();
							enviroSuitLight.GetEntity()->Present();
						}
						break;
					}
					default:
					{
						break;
					}
				}
			}
			if( PowerUpActive( i ) && inventory.powerupEndTime[i] <= gameLocal.time )
			{
				ClearPowerup( i );
			}
		}
	}
	
	if( health > 0 )
	{
		if( powerUpSkin )
		{
			renderEntity.customSkin = powerUpSkin;
		}
		else
		{
			renderEntity.customSkin = skin;
		}
	}
	
	if( healthPool && gameLocal.time > nextHealthPulse && !AI_DEAD && health > 0 )
	{
		assert( !common->IsClient() );	// healthPool never be set on client
		int amt = ( healthPool > 5.0f ) ? 5 : healthPool;
		health += amt;
		if( health > inventory.maxHealth )
		{
			health = inventory.maxHealth;
			healthPool = 0;
		}
		else
		{
			healthPool -= amt;
		}
		if( healthPool < 1.0f )
		{
			healthPool = 0.0f;
		}
		else
		{
			nextHealthPulse = gameLocal.time + HEALTHPULSE_TIME;
			healthPulse = true;
		}
	}
	if( !gameLocal.inCinematic && influenceActive == 0 && g_skill.GetInteger() == 3 && gameLocal.time > nextHealthTake && !AI_DEAD && health > g_healthTakeLimit.GetInteger() )
	{
		assert( !common->IsClient() );	// healthPool never be set on client
		
		if( !PowerUpActive( INVULNERABILITY ) )
		{
			health -= g_healthTakeAmt.GetInteger();
			if( health < g_healthTakeLimit.GetInteger() )
			{
				health = g_healthTakeLimit.GetInteger();
			}
		}
		nextHealthTake = gameLocal.time + g_healthTakeTime.GetInteger() * 1000;
		healthTake = true;
	}
}

/*
===============
idPlayer::ClearPowerUps
===============
*/
void idPlayer::ClearPowerUps()
{
	int i;
	for( i = 0; i < MAX_POWERUPS; i++ )
	{
		if( PowerUpActive( i ) )
		{
			ClearPowerup( i );
		}
	}
	inventory.ClearPowerUps();
	
	if( common->IsMultiplayer() )
	{
		if( enviroSuitLight.IsValid() )
		{
			enviroSuitLight.GetEntity()->PostEventMS( &EV_Remove, 0 );
		}
	}
}

/*
===============
idPlayer::GiveInventoryItem
===============
*/
bool idPlayer::GiveInventoryItem( idDict* item, unsigned int giveFlags )
{
	if( common->IsMultiplayer() && spectating )
	{
		return false;
	}
	
	if( giveFlags & ITEM_GIVE_UPDATE_STATE )
	{
		inventory.items.Append( new( TAG_ENTITY ) idDict( *item ) );
	}
	
	const char* itemName = item->GetString( "inv_name" );
	
	if( giveFlags & ITEM_GIVE_FEEDBACK )
	{
		if( idStr::Cmpn( itemName, STRTABLE_ID, STRTABLE_ID_LENGTH ) == 0 )
		{
			inventory.pickupItemNames.Append( idLocalization::GetString( itemName ) );
		}
		else
		{
			inventory.pickupItemNames.Append( itemName );
		}
		
		const char* icon = item->GetString( "inv_icon" );
		if( hud != NULL )
		{
			hud->ShowNewItem( itemName, icon );
		}
	}
	
	// D3XP added to support powercells
	if( ( giveFlags & ITEM_GIVE_UPDATE_STATE ) && item->GetInt( "inv_powercell" ) && focusUI )
	{
		//Reset the powercell count
		int powerCellCount = 0;
		for( int j = 0; j < inventory.items.Num(); j++ )
		{
			idDict* item = inventory.items[ j ];
			if( item->GetInt( "inv_powercell" ) )
			{
				powerCellCount++;
			}
		}
		focusUI->SetStateInt( "powercell_count", powerCellCount );
	}
	
	return true;
}

/*
==============
idPlayer::GiveInventoryItem
==============
*/
bool idPlayer::GiveInventoryItem( const char* name )
{
	idDict args;
	
	args.Set( "classname", name );
	args.Set( "owner", this->name.c_str() );
	gameLocal.SpawnEntityDef( args );
	return true;
}

/*
===============
idPlayer::GiveObjective
===============
*/
void idPlayer::GiveObjective( const char* title, const char* text, const idMaterial* screenshot )
{
	idObjectiveInfo& info = inventory.objectiveNames.Alloc();
	info.title = title;
	info.text = text;
	info.screenshot = screenshot;
	
	StartSound( "snd_objectiveup", SND_CHANNEL_ANY, 0, false, NULL );
	
	if( hud )
	{
		hud->SetupObjective( title, text, screenshot );
		hud->ShowObjective( false );
		objectiveUp = true;
	}
}

/*
===============
idPlayer::CompleteObjective
===============
*/
void idPlayer::CompleteObjective( const char* title )
{
	int c = inventory.objectiveNames.Num();
	for( int i = 0;  i < c; i++ )
	{
		if( idStr::Icmp( inventory.objectiveNames[i].title, title ) == 0 )
		{
			inventory.objectiveNames.RemoveIndex( i );
			break;
		}
	}
	
	StartSound( "snd_objectiveup", SND_CHANNEL_ANY, 0, false, NULL );
	
	if( hud )
	{
		hud->SetupObjectiveComplete( title );
		hud->ShowObjective( true );
	}
}

/*
===============
idPlayer::GiveVideo
===============
*/
void idPlayer::GiveVideo( const idDeclVideo* video, const char* itemName )
{

	if( video == NULL )
	{
		return;
	}
	
	int oldNumVideos = inventory.videos.Num();
	inventory.videos.AddUnique( video );
	
	if( oldNumVideos < inventory.videos.Num() )
	{
		GetAchievementManager().EventCompletesAchievement( ACHIEVEMENT_WATCH_ALL_VIDEOS );
	}
	
	if( itemName != NULL && itemName[0] != 0 )
	{
		inventory.pickupItemNames.Append( itemName );
	}
	
	if( hud )
	{
		hud->DownloadVideo();
	}
}

/*
===============
idPlayer::GiveSecurity
===============
*/
void idPlayer::GiveSecurity( const char* security )
{
	GetPDA()->SetSecurity( security );
	
	if( hud )
	{
		hud->UpdatedSecurity();
	}
}

/*
===============
idPlayer::GiveEmail
===============
*/
void idPlayer::GiveEmail( const idDeclEmail* email )
{
	if( email == NULL )
	{
		return;
	}
	
	inventory.emails.AddUnique( email );
	GetPDA()->AddEmail( email );
	
	// TODO_SPARTY: hook up new email notification in new hud
	//if ( hud ) {
	//	hud->HandleNamedEvent( "emailPickup" );
	//}
}

/*
===============
idPlayer::GivePDA
===============
*/
void idPlayer::GivePDA( const idDeclPDA* pda, const char* securityItem )
{
	if( common->IsMultiplayer() && spectating )
	{
		return;
	}
	
	if( securityItem != NULL && securityItem[0] != 0 )
	{
		inventory.pdaSecurity.AddUnique( securityItem );
	}
	
	// Just to make sure they want the default player spawn defined pda.
	// Some what of a hack, so i dont have to change any map scripts that initially give
	// the player "personal" pda.
	if( pda == NULL || idStr::Icmp( pda->GetName(), "personal" ) == 0 )
	{
		pda = static_cast<const idDeclPDA*>( declManager->FindType( DECL_PDA, spawnArgs.GetString( "pda_name", "personal" ) ) );
	}
	if( pda == NULL )
	{
		return;
	}
	
	int oldNumPDAs = inventory.pdas.Num();
	inventory.pdas.AddUnique( pda );
	int newNumPDAs = inventory.pdas.Num();
	
	// Set the stat for # of PDAs...
	// Only increment the PDA stat if we've added a new one....
	if( oldNumPDAs < newNumPDAs )
	{
		switch( GetExpansionType() )
		{
			case GAME_BASE:
				GetAchievementManager().EventCompletesAchievement( ACHIEVEMENT_PDAS_BASE );
				break;
			case GAME_D3XP:
				GetAchievementManager().EventCompletesAchievement( ACHIEVEMENT_PDAS_ROE );
				break;
			case GAME_D3LE:
				GetAchievementManager().EventCompletesAchievement( ACHIEVEMENT_PDAS_LE );
				break;
		}
	}
	
	// Copy any videos over
	for( int i = 0; i < pda->GetNumVideos(); i++ )
	{
		const idDeclVideo* video = pda->GetVideoByIndex( i );
		if( video != NULL )
		{
			inventory.videos.AddUnique( video );
		}
	}
	
	// This is kind of a hack, but it works nicely
	// We don't want to display the 'you got a new pda' message during a map load
	if( gameLocal.GetFrameNum() > 10 )
	{
		const char* sec = pda->GetSecurity();
		if( hud )
		{
			hud->DownloadPDA( pda, ( sec != NULL && sec[0] != 0 ) ? true : false );
		}
		if( inventory.pdas.Num() == 1 )
		{
			GetPDA()->RemoveAddedEmailsAndVideos();
			if( !objectiveSystemOpen )
			{
				TogglePDA();
			}
			//ShowTip( spawnArgs.GetString( "text_infoTitle" ), spawnArgs.GetString( "text_firstPDA" ), true );
		}
	}
}

/*
===============
idPlayer::FindInventoryItem
===============
*/
idDict* idPlayer::FindInventoryItem( const char* name )
{
	for( int i = 0; i < inventory.items.Num(); i++ )
	{
		const char* iname = inventory.items[i]->GetString( "inv_name" );
		if( iname != NULL && *iname != '\0' )
		{
			if( idStr::Icmp( name, iname ) == 0 )
			{
				return inventory.items[i];
			}
		}
	}
	return NULL;
}

/*
===============
idPlayer::FindInventoryItem
===============
*/
idDict* idPlayer::FindInventoryItem( int index )
{
	if( index <= inventory.items.Num() )
	{
		return inventory.items[ index ];
	}
	return NULL;
}

/*
===============
idPlayer::GetNumInventoryItems
===============
*/
int idPlayer::GetNumInventoryItems()
{
	return inventory.items.Num();
}

/*
===============
idPlayer::RemoveInventoryItem
===============
*/
void idPlayer::RemoveInventoryItem( const char* name )
{
	//Hack for localization
	if( !idStr::Icmp( name, "Pwr Cell" ) )
	{
		name = idLocalization::GetString( "#str_00101056" );
	}
	idDict* item = FindInventoryItem( name );
	if( item )
	{
		RemoveInventoryItem( item );
	}
}

/*
===============
idPlayer::RemoveInventoryItem
===============
*/
void idPlayer::RemoveInventoryItem( idDict* item )
{
	inventory.items.Remove( item );
	
	if( item->GetInt( "inv_powercell" ) && focusUI )
	{
		//Reset the powercell count
		int powerCellCount = 0;
		for( int j = 0; j < inventory.items.Num(); j++ )
		{
			idDict* item = inventory.items[ j ];
			if( item->GetInt( "inv_powercell" ) )
			{
				powerCellCount++;
			}
		}
		focusUI->SetStateInt( "powercell_count", powerCellCount );
	}
	
	delete item;
}

/*
===============
idPlayer::GiveItem
===============
*/
void idPlayer::GiveItem( const char* itemname )
{
	idDict args;
	
	args.Set( "classname", itemname );
	args.Set( "owner", name.c_str() );
	gameLocal.SpawnEntityDef( args );
}

/*
==================
idPlayer::SlotForWeapon
==================
*/
int idPlayer::SlotForWeapon( const char* weaponName )
{
	int i;
	
	for( i = 0; i < MAX_WEAPONS; i++ )
	{
		const char* weap = spawnArgs.GetString( va( "def_weapon%d", i ) );
		if( !idStr::Cmp( weap, weaponName ) )
		{
			return i;
		}
	}
	
	// not found
	return -1;
}

/*
===============
idPlayer::Reload
===============
*/
void idPlayer::Reload()
{
	if( spectating || gameLocal.inCinematic || influenceActive )
	{
		return;
	}
	
	if( common->IsClient() && !IsLocallyControlled() )
	{
		return;
	}
	
	if( weapon.GetEntity() && weapon.GetEntity()->IsLinked() )
	{
		weapon.GetEntity()->Reload();
	}
}

/*
===============
idPlayer::NextBestWeapon
===============
*/
void idPlayer::NextBestWeapon()
{
	const char* weap;
	int w = MAX_WEAPONS;
	
	if( !weaponEnabled )
	{
		return;
	}
	
	while( w > 0 )
	{
		w--;
		if( w == weapon_flashlight )
		{
			continue;
		}
		weap = spawnArgs.GetString( va( "def_weapon%d", w ) );
		if( !weap[ 0 ] || ( ( inventory.weapons & ( 1 << w ) ) == 0 ) || ( !inventory.HasAmmo( weap, true, this ) ) )
		{
			continue;
		}
		if( !spawnArgs.GetBool( va( "weapon%d_best", w ) ) )
		{
			continue;
		}
		
		//Some weapons will report having ammo but the clip is empty and
		//will not have enough to fill the clip (i.e. Double Barrel Shotgun with 1 round left)
		//We need to skip these weapons because they cannot be used
		if( inventory.HasEmptyClipCannotRefill( weap, this ) )
		{
			continue;
		}
		
		break;
	}
	idealWeapon = w;
	weaponSwitchTime = gameLocal.time + WEAPON_SWITCH_DELAY;
	UpdateHudWeapon();
}

/*
===============
idPlayer::NextWeapon
===============
*/
void idPlayer::NextWeapon()
{

	if( !weaponEnabled || spectating || hiddenWeapon || gameLocal.inCinematic || gameLocal.world->spawnArgs.GetBool( "no_Weapons" ) || health < 0 )
	{
		return;
	}
	
	// check if we have any weapons
	if( !inventory.weapons )
	{
		return;
	}
	
	int w = idealWeapon.Get();
	while( 1 )
	{
		w++;
		if( w >= MAX_WEAPONS )
		{
			w = 0;
		}
		if( w == idealWeapon )
		{
			w = weapon_fists;
			break;
		}
		if( ( inventory.weapons & ( 1 << w ) ) == 0 )
		{
			continue;
		}
		const char* weap = spawnArgs.GetString( va( "def_weapon%d", w ) );
		if( !spawnArgs.GetBool( va( "weapon%d_cycle", w ) ) )
		{
			continue;
		}
		if( !weap[ 0 ] )
		{
			continue;
		}
		
		if( inventory.HasAmmo( weap, true, this ) || w == weapon_bloodstone )
		{
			break;
		}
	}
	
	if( ( w != currentWeapon ) && ( w != idealWeapon ) )
	{
		idealWeapon = w;
		weaponSwitchTime = gameLocal.time + WEAPON_SWITCH_DELAY;
		UpdateHudWeapon();
	}
}

/*
===============
idPlayer::PrevWeapon
===============
*/
void idPlayer::PrevWeapon()
{

	if( !weaponEnabled || spectating || hiddenWeapon || gameLocal.inCinematic || gameLocal.world->spawnArgs.GetBool( "no_Weapons" ) || health < 0 )
	{
		return;
	}
	
	// check if we have any weapons
	if( !inventory.weapons )
	{
		return;
	}
	
	int w = idealWeapon.Get();
	while( 1 )
	{
		w--;
		if( w < 0 )
		{
			w = MAX_WEAPONS - 1;
		}
		if( w == idealWeapon )
		{
			w = weapon_fists;
			break;
		}
		if( ( inventory.weapons & ( 1 << w ) ) == 0 )
		{
			continue;
		}
		const char* weap = spawnArgs.GetString( va( "def_weapon%d", w ) );
		if( !spawnArgs.GetBool( va( "weapon%d_cycle", w ) ) )
		{
			continue;
		}
		if( !weap[ 0 ] )
		{
			continue;
		}
		if( inventory.HasAmmo( weap, true, this ) || w == weapon_bloodstone )
		{
			break;
		}
	}
	
	if( ( w != currentWeapon ) && ( w != idealWeapon ) )
	{
		idealWeapon = w;
		weaponSwitchTime = gameLocal.time + WEAPON_SWITCH_DELAY;
		UpdateHudWeapon();
	}
}

/*
===============
idPlayer::SelectWeapon
===============
*/
void idPlayer::SelectWeapon( int num, bool force )
{
	const char* weap;
	
	if( !weaponEnabled || spectating || gameLocal.inCinematic || health < 0 )
	{
		return;
	}
	
	if( ( num < 0 ) || ( num >= MAX_WEAPONS ) )
	{
		return;
	}
	
	if( num == weapon_flashlight )
	{
		return;
	}
	
	if( ( num != weapon_pda ) && gameLocal.world->spawnArgs.GetBool( "no_Weapons" ) )
	{
		num = weapon_fists;
		hiddenWeapon ^= 1;
		if( hiddenWeapon && weapon.GetEntity() )
		{
			weapon.GetEntity()->LowerWeapon();
		}
		else
		{
			weapon.GetEntity()->RaiseWeapon();
		}
	}
	
	//Is the weapon a toggle weapon
	WeaponToggle_t* weaponToggle;
	if( weaponToggles.Get( va( "weapontoggle%d", num ), &weaponToggle ) )
	{
	
		int weaponToggleIndex = 0;
		
		//Find the current Weapon in the list
		int currentIndex = -1;
		for( int i = 0; i < weaponToggle->toggleList.Num(); i++ )
		{
			if( weaponToggle->toggleList[i] == idealWeapon )
			{
				currentIndex = i;
				break;
			}
		}
		if( currentIndex == -1 )
		{
			//Didn't find the current weapon so select the first item
			weaponToggleIndex = weaponToggle->lastUsed;
		}
		else
		{
			//Roll to the next available item in the list
			weaponToggleIndex = currentIndex;
			weaponToggleIndex++;
			if( weaponToggleIndex >= weaponToggle->toggleList.Num() )
			{
				weaponToggleIndex = 0;
			}
		}
		
		for( int i = 0; i < weaponToggle->toggleList.Num(); i++ )
		{
			int weapNum = weaponToggle->toggleList[weaponToggleIndex];
			//Is it available
			if( inventory.weapons & ( 1 << weapNum ) )
			{
				//Do we have ammo for it
				if( inventory.HasAmmo( spawnArgs.GetString( va( "def_weapon%d", weapNum ) ), true, this ) || spawnArgs.GetBool( va( "weapon%d_allowempty", weapNum ) ) )
				{
					break;
				}
			}
			
			weaponToggleIndex++;
			if( weaponToggleIndex >= weaponToggle->toggleList.Num() )
			{
				weaponToggleIndex = 0;
			}
		}
		weaponToggle->lastUsed = weaponToggleIndex;
		num = weaponToggle->toggleList[weaponToggleIndex];
	}
	
	weap = spawnArgs.GetString( va( "def_weapon%d", num ) );
	if( !weap[ 0 ] )
	{
		gameLocal.Printf( "Invalid weapon\n" );
		return;
	}
	
	if( force || ( inventory.weapons & ( 1 << num ) ) )
	{
		if( !inventory.HasAmmo( weap, true, this ) && !spawnArgs.GetBool( va( "weapon%d_allowempty", num ) ) )
		{
			return;
		}
		if( ( previousWeapon >= 0 ) && ( idealWeapon == num ) && ( spawnArgs.GetBool( va( "weapon%d_toggle", num ) ) ) )
		{
			weap = spawnArgs.GetString( va( "def_weapon%d", previousWeapon ) );
			if( !inventory.HasAmmo( weap, true, this ) && !spawnArgs.GetBool( va( "weapon%d_allowempty", previousWeapon ) ) )
			{
				return;
			}
			idealWeapon = previousWeapon;
		}
		else if( ( weapon_pda >= 0 ) && ( num == weapon_pda ) && ( inventory.pdas.Num() == 0 ) )
		{
			ShowTip( spawnArgs.GetString( "text_infoTitle" ), spawnArgs.GetString( "text_noPDA" ), true );
			return;
		}
		else
		{
			idealWeapon = num;
		}
		UpdateHudWeapon();
	}
}

/*
=================
idPlayer::DropWeapon
=================
*/
void idPlayer::DropWeapon( bool died )
{
	idVec3 forward, up;
	int inclip, ammoavailable;
	
	if( died == false )
	{
		return;
	}
	
	assert( !common->IsClient() );
	
	if( spectating || weaponGone || weapon.GetEntity() == NULL )
	{
		return;
	}
	
	if( ( !died && !weapon.GetEntity()->IsReady() ) || weapon.GetEntity()->IsReloading() )
	{
		return;
	}
	// ammoavailable is how many shots we can fire
	// inclip is which amount is in clip right now
	ammoavailable = weapon.GetEntity()->AmmoAvailable();
	inclip = weapon.GetEntity()->AmmoInClip();
	
	// don't drop a grenade if we have none left
	if( !idStr::Icmp( idWeapon::GetAmmoNameForNum( weapon.GetEntity()->GetAmmoType() ), "ammo_grenades" ) && ( ammoavailable - inclip <= 0 ) )
	{
		return;
	}
	
	ammoavailable += inclip;
	
	// expect an ammo setup that makes sense before doing any dropping
	// ammoavailable is -1 for infinite ammo, and weapons like chainsaw
	// a bad ammo config usually indicates a bad weapon state, so we should not drop
	// used to be an assertion check, but it still happens in edge cases
	
	if( ( ammoavailable != -1 ) && ( ammoavailable < 0 ) )
	{
		common->DPrintf( "idPlayer::DropWeapon: bad ammo setup\n" );
		return;
	}
	idEntity* item = NULL;
	if( died )
	{
		// ain't gonna throw you no weapon if I'm dead
		item = weapon.GetEntity()->DropItem( vec3_origin, 0, WEAPON_DROP_TIME, died );
	}
	else
	{
		viewAngles.ToVectors( &forward, NULL, &up );
		item = weapon.GetEntity()->DropItem( 250.0f * forward + 150.0f * up, 500, WEAPON_DROP_TIME, died );
	}
	if( !item )
	{
		return;
	}
	// set the appropriate ammo in the dropped object
	const idKeyValue* keyval = item->spawnArgs.MatchPrefix( "inv_ammo_" );
	if( keyval )
	{
		item->spawnArgs.SetInt( keyval->GetKey(), ammoavailable );
		idStr inclipKey = keyval->GetKey();
		inclipKey.Insert( "inclip_", 4 );
		inclipKey.Insert( va( "%.2d", currentWeapon ), 11 );
		item->spawnArgs.SetInt( inclipKey, inclip );
	}
	if( !died )
	{
		// remove from our local inventory completely
		inventory.Drop( spawnArgs, item->spawnArgs.GetString( "inv_weapon" ), -1 );
		weapon.GetEntity()->ResetAmmoClip();
		NextWeapon();
		weapon.GetEntity()->WeaponStolen();
		weaponGone = true;
	}
}

/*
=================
idPlayer::StealWeapon
steal the target player's current weapon
=================
*/
void idPlayer::StealWeapon( idPlayer* player )
{
	assert( !common->IsClient() );
	
	// make sure there's something to steal
	idWeapon* player_weapon = static_cast< idWeapon* >( player->weapon.GetEntity() );
	if( !player_weapon || !player_weapon->CanDrop() || weaponGone )
	{
		return;
	}
	// steal - we need to effectively force the other player to abandon his weapon
	int newweap = player->currentWeapon;
	if( newweap == -1 )
	{
		return;
	}
	// might be just dropped - check inventory
	if( !( player->inventory.weapons & ( 1 << newweap ) ) )
	{
		return;
	}
	const char* weapon_classname = spawnArgs.GetString( va( "def_weapon%d", newweap ) );
	assert( weapon_classname );
	int ammoavailable = player->weapon.GetEntity()->AmmoAvailable();
	int inclip = player->weapon.GetEntity()->AmmoInClip();
	
	ammoavailable += inclip;
	
	if( ( ammoavailable != -1 ) && ( ammoavailable < 0 ) )
	{
		// see DropWeapon
		common->DPrintf( "idPlayer::StealWeapon: bad ammo setup\n" );
		// we still steal the weapon, so let's use the default ammo levels
		inclip = -1;
		const idDeclEntityDef* decl = gameLocal.FindEntityDef( weapon_classname );
		assert( decl );
		const idKeyValue* keypair = decl->dict.MatchPrefix( "inv_ammo_" );
		assert( keypair );
		ammoavailable = atoi( keypair->GetValue() );
	}
	
	player->weapon.GetEntity()->WeaponStolen();
	player->inventory.Drop( player->spawnArgs, NULL, newweap );
	player->SelectWeapon( weapon_fists, false );
	// in case the robbed player is firing rounds with a continuous fire weapon like the chaingun/plasma etc.
	// this will ensure the firing actually stops
	player->weaponGone = true;
	
	// give weapon, setup the ammo count
	Give( "weapon", weapon_classname, ITEM_GIVE_FEEDBACK | ITEM_GIVE_UPDATE_STATE );
	ammo_t ammo_i = player->inventory.AmmoIndexForWeaponClass( weapon_classname, NULL );
	idealWeapon = newweap;
	const int currentAmmo = inventory.GetInventoryAmmoForType( ammo_i );
	inventory.SetInventoryAmmoForType( ammo_i, currentAmmo + ammoavailable );
}

/*
===============
idPlayer::ActiveGui
===============
*/
idUserInterface* idPlayer::ActiveGui()
{
	if( objectiveSystemOpen )
	{
		return NULL;
	}
	
	return focusUI;
}

/*
===============
idPlayer::Weapon_Combat
===============
*/
void idPlayer::Weapon_Combat()
{
	if( influenceActive || !weaponEnabled || gameLocal.inCinematic || privateCameraView )
	{
		return;
	}
	
	weapon.GetEntity()->RaiseWeapon();
	if( weapon.GetEntity()->IsReloading() )
	{
		if( !AI_RELOAD )
		{
			AI_RELOAD = true;
			SetState( "ReloadWeapon" );
			UpdateScript();
		}
	}
	else
	{
		AI_RELOAD = false;
	}
	
	if( idealWeapon == weapon_soulcube && soulCubeProjectile.GetEntity() != NULL )
	{
		idealWeapon = currentWeapon;
	}
	
	if( idealWeapon != currentWeapon &&  idealWeapon.Get() < MAX_WEAPONS )
	{
		if( weaponCatchup )
		{
			assert( common->IsClient() );
			
			currentWeapon = idealWeapon.Get();
			weaponGone = false;
			animPrefix = spawnArgs.GetString( va( "def_weapon%d", currentWeapon ) );
			weapon.GetEntity()->GetWeaponDef( animPrefix, inventory.GetClipAmmoForWeapon( currentWeapon ) );
			animPrefix.Strip( "weapon_" );
			
			weapon.GetEntity()->NetCatchup();
			const function_t* newstate = GetScriptFunction( "NetCatchup" );
			if( newstate )
			{
				SetState( newstate );
				UpdateScript();
			}
			weaponCatchup = false;
		}
		else
		{
			if( weapon.GetEntity()->IsReady() )
			{
				weapon.GetEntity()->PutAway();
			}
			
			if( weapon.GetEntity()->IsHolstered() )
			{
				assert( idealWeapon.Get() >= 0 );
				assert( idealWeapon.Get() < MAX_WEAPONS );
				
				if( currentWeapon != weapon_pda && !spawnArgs.GetBool( va( "weapon%d_toggle", currentWeapon ) ) )
				{
					previousWeapon = currentWeapon;
				}
				currentWeapon = idealWeapon.Get();
				weaponGone = false;
				animPrefix = spawnArgs.GetString( va( "def_weapon%d", currentWeapon ) );
				weapon.GetEntity()->GetWeaponDef( animPrefix, inventory.GetClipAmmoForWeapon( currentWeapon ) );
				animPrefix.Strip( "weapon_" );
				
				weapon.GetEntity()->Raise();
			}
		}
	}
	else
	{
		weaponGone = false;	// if you drop and re-get weap, you may miss the = false above
		if( weapon.GetEntity()->IsHolstered() )
		{
			if( !weapon.GetEntity()->AmmoAvailable() )
			{
				// weapons can switch automatically if they have no more ammo
				NextBestWeapon();
			}
			else
			{
				weapon.GetEntity()->Raise();
				state = GetScriptFunction( "RaiseWeapon" );
				if( state )
				{
					SetState( state );
				}
			}
		}
	}
	
	// check for attack
	AI_WEAPON_FIRED = false;
	if( !influenceActive )
	{
		if( ( usercmd.buttons & BUTTON_ATTACK ) && !weaponGone )
		{
			FireWeapon();
		}
		else if( oldButtons & BUTTON_ATTACK )
		{
			AI_ATTACK_HELD = false;
			weapon.GetEntity()->EndAttack();
		}
	}
	
	// update our ammo clip in our inventory
	if( ( currentWeapon >= 0 ) && ( currentWeapon < MAX_WEAPONS ) )
	{
		inventory.SetClipAmmoForWeapon( currentWeapon, weapon.GetEntity()->AmmoInClip() );
	}
}

/*
===============
idPlayer::Weapon_NPC
===============
*/
void idPlayer::Weapon_NPC()
{
	if( idealWeapon != currentWeapon )
	{
		Weapon_Combat();
	}
	StopFiring();
	weapon.GetEntity()->LowerWeapon();
	
	bool wasDown = ( oldButtons & ( BUTTON_ATTACK | BUTTON_USE ) ) != 0;
	bool isDown = ( usercmd.buttons & ( BUTTON_ATTACK | BUTTON_USE ) ) != 0;
	if( isDown && !wasDown )
	{
		buttonMask |= BUTTON_ATTACK;
		focusCharacter->TalkTo( this );
	}
}

/*
===============
idPlayer::LowerWeapon
===============
*/
void idPlayer::LowerWeapon()
{
	if( weapon.GetEntity() && !weapon.GetEntity()->IsHidden() )
	{
		weapon.GetEntity()->LowerWeapon();
	}
}

/*
===============
idPlayer::RaiseWeapon
===============
*/
void idPlayer::RaiseWeapon()
{
	if( weapon.GetEntity() && weapon.GetEntity()->IsHidden() )
	{
		weapon.GetEntity()->RaiseWeapon();
	}
}

/*
===============
idPlayer::WeaponLoweringCallback
===============
*/
void idPlayer::WeaponLoweringCallback()
{
	SetState( "LowerWeapon" );
	UpdateScript();
}

/*
===============
idPlayer::WeaponRisingCallback
===============
*/
void idPlayer::WeaponRisingCallback()
{
	SetState( "RaiseWeapon" );
	UpdateScript();
}

/*
===============
idPlayer::Weapon_GUI
===============
*/
void idPlayer::Weapon_GUI()
{

	if( !objectiveSystemOpen )
	{
		if( idealWeapon != currentWeapon )
		{
			Weapon_Combat();
		}
		StopFiring();
		weapon.GetEntity()->LowerWeapon();
	}
	
	// disable click prediction for the GUIs. handy to check the state sync does the right thing
	if( common->IsClient() && !net_clientPredictGUI.GetBool() )
	{
		return;
	}
	
	bool wasDown = ( oldButtons & ( BUTTON_ATTACK | BUTTON_USE ) ) != 0;
	bool isDown = ( usercmd.buttons & ( BUTTON_ATTACK | BUTTON_USE ) ) != 0;
	if( isDown != wasDown )
	{
		const char* command = NULL;
		idUserInterface* ui = ActiveGui();
		if( ui )
		{
			bool updateVisuals = false;
			sysEvent_t ev = sys->GenerateMouseButtonEvent( 1, isDown );
			command = ui->HandleEvent( &ev, gameLocal.time, &updateVisuals );
			if( updateVisuals && focusGUIent && ui == focusUI )
			{
				focusGUIent->UpdateVisuals();
			}
		}
		if( common->IsClient() )
		{
			// we predict enough, but don't want to execute commands
			return;
		}
		
		// HACK - Check to see who is activating the frag chamber. Im sorry.
		if( common->IsMultiplayer() && focusGUIent )
		{
			if( strcmp( focusGUIent->GetName(), "chamber_gui_console" ) == 0 && strcmp( command, " ; runScript chamber_trigger" ) == 0 )
			{
				gameLocal.playerActivateFragChamber = this;
			}
		}
		
		if( focusGUIent )
		{
			HandleGuiCommands( focusGUIent, command );
		}
		else
		{
			HandleGuiCommands( this, command );
		}
	}
}

/*
===============
idPlayer::UpdateWeapon
===============
*/
void idPlayer::UpdateWeapon()
{
	if( health <= 0 )
	{
		return;
	}
	
	assert( !spectating );
	
	if( common->IsClient() )
	{
		// clients need to wait till the weapon and it's world model entity
		// are present and synchronized ( weapon.worldModel idEntityPtr to idAnimatedEntity )
		if( !weapon.GetEntity()->IsWorldModelReady() )
		{
			return;
		}
	}
	
	// always make sure the weapon is correctly setup before accessing it
	if( !weapon.GetEntity()->IsLinked() )
	{
		if( idealWeapon != -1 )
		{
			animPrefix = spawnArgs.GetString( va( "def_weapon%d", idealWeapon.Get() ) );
			int ammoInClip = inventory.GetClipAmmoForWeapon( idealWeapon.Get() );
			if( common->IsMultiplayer() && respawning )
			{
				// Do not load ammo into the clip here on MP respawn, as it will be done
				// elsewhere. If we take ammo out here then the player will end up losing
				// a clip of ammo for their initial weapon upon respawn.
				ammoInClip = 0;
			}
			weapon.GetEntity()->GetWeaponDef( animPrefix, ammoInClip );
			assert( weapon.GetEntity()->IsLinked() );
		}
		else
		{
			return;
		}
	}
	
	if( hiddenWeapon && tipUp && usercmd.buttons & BUTTON_ATTACK )
	{
		HideTip();
	}
	
	if( g_dragEntity.GetBool() )
	{
		StopFiring();
		weapon.GetEntity()->LowerWeapon();
		dragEntity.Update( this );
	}
	else if( ActiveGui() )
	{
		// gui handling overrides weapon use
		Weapon_GUI();
	}
	else 	if( focusCharacter && ( focusCharacter->health > 0 ) )
	{
		Weapon_NPC();
	}
	else
	{
		Weapon_Combat();
	}
	
	if( hiddenWeapon )
	{
		weapon.GetEntity()->LowerWeapon();
	}
	
	// update weapon state, particles, dlights, etc
	weapon.GetEntity()->PresentWeapon( CanShowWeaponViewmodel() );
}

/*
===============
idPlayer::UpdateFlashLight
===============
*/
void idPlayer::UpdateFlashlight()
{
	if( idealWeapon == weapon_flashlight )
	{
		// force classic flashlight to go away
		NextWeapon();
	}
	
	if( !flashlight.IsValid() )
	{
		return;
	}
	
	if( !flashlight.GetEntity()->GetOwner() )
	{
		return;
	}
	
	// Don't update the flashlight if dead in MP.
	// Otherwise you can see a floating flashlight worldmodel near player's skeletons.
	if( common->IsMultiplayer() )
	{
		if( health < 0 )
		{
			return;
		}
	}
	
	// Flashlight has an infinite battery in multiplayer.
	if( !common->IsMultiplayer() )
	{
		if( flashlight.GetEntity()->lightOn )
		{
			if( flashlight_batteryDrainTimeMS.GetInteger() > 0 )
			{
				flashlightBattery -= ( gameLocal.time - gameLocal.previousTime );
				if( flashlightBattery < 0 )
				{
					FlashlightOff();
					flashlightBattery = 0;
				}
			}
		}
		else
		{
			if( flashlightBattery < flashlight_batteryDrainTimeMS.GetInteger() )
			{
				flashlightBattery += ( gameLocal.time - gameLocal.previousTime ) * Max( 1, ( flashlight_batteryDrainTimeMS.GetInteger() / flashlight_batteryChargeTimeMS.GetInteger() ) );
				if( flashlightBattery > flashlight_batteryDrainTimeMS.GetInteger() )
				{
					flashlightBattery = flashlight_batteryDrainTimeMS.GetInteger();
				}
			}
		}
	}
	
	if( hud )
	{
		hud->UpdateFlashlight( this );
	}
	
	if( common->IsClient() )
	{
		// clients need to wait till the weapon and it's world model entity
		// are present and synchronized ( weapon.worldModel idEntityPtr to idAnimatedEntity )
		if( !flashlight.GetEntity()->IsWorldModelReady() )
		{
			return;
		}
	}
	
	// always make sure the weapon is correctly setup before accessing it
	if( !flashlight.GetEntity()->IsLinked() )
	{
		flashlight.GetEntity()->GetWeaponDef( "weapon_flashlight_new", 0 );
		flashlight.GetEntity()->SetIsPlayerFlashlight( true );
		
		// adjust position / orientation of flashlight
		idAnimatedEntity* worldModel = flashlight.GetEntity()->GetWorldModel();
		worldModel->BindToJoint( this, "Chest", true );
		// Don't interpolate the flashlight world model in mp, let it bind like normal.
		worldModel->SetUseClientInterpolation( false );
		
		assert( flashlight.GetEntity()->IsLinked() );
	}
	
	// this positions the third person flashlight model! (as seen in the mirror)
	idAnimatedEntity* worldModel = flashlight.GetEntity()->GetWorldModel();
	static const idVec3 fl_pos = idVec3( 3.0f, 9.0f, 2.0f );
	worldModel->GetPhysics()->SetOrigin( fl_pos );
	static float fl_pitch = 0.0f;
	static float fl_yaw = 0.0f;
	static float fl_roll = 0.0f;
	static idAngles ang = ang_zero;
	ang.Set( fl_pitch, fl_yaw, fl_roll );
	worldModel->GetPhysics()->SetAxis( ang.ToMat3() );
	
	if( flashlight.GetEntity()->lightOn )
	{
		if( ( flashlightBattery < flashlight_batteryChargeTimeMS.GetInteger() / 2 ) && ( gameLocal.random.RandomFloat() < flashlight_batteryFlickerPercent.GetFloat() ) )
		{
			flashlight.GetEntity()->RemoveMuzzleFlashlight();
		}
		else
		{
			flashlight.GetEntity()->MuzzleFlashLight();
		}
	}
	
	flashlight.GetEntity()->PresentWeapon( true );
	
	if( gameLocal.world->spawnArgs.GetBool( "no_Weapons" ) || gameLocal.inCinematic || spectating || fl.hidden )
	{
		worldModel->Hide();
	}
	else
	{
		worldModel->Show();
	}
}

/*
===============
idPlayer::FlashlightOn
===============
*/
void idPlayer::FlashlightOn()
{
	if( !flashlight.IsValid() )
	{
		return;
	}
	if( flashlightBattery < idMath::Ftoi( flashlight_minActivatePercent.GetFloat() * flashlight_batteryDrainTimeMS.GetFloat() ) )
	{
		return;
	}
	if( gameLocal.inCinematic )
	{
		return;
	}
	if( flashlight.GetEntity()->lightOn )
	{
		return;
	}
	if( health <= 0 )
	{
		return;
	}
	if( spectating )
	{
		return;
	}
	
	flashlight->FlashlightOn();
}

/*
===============
idPlayer::FlashlightOff
===============
*/
void idPlayer::FlashlightOff()
{
	if( !flashlight.IsValid() )
	{
		return;
	}
	if( !flashlight.GetEntity()->lightOn )
	{
		return;
	}
	flashlight->FlashlightOff();
}

/*
===============
idPlayer::SpectateFreeFly
===============
*/
void idPlayer::SpectateFreeFly( bool force )
{
	idPlayer*	player;
	idVec3		newOrig;
	idVec3		spawn_origin;
	idAngles	spawn_angles;
	
	player = gameLocal.GetClientByNum( spectator );
	if( force || gameLocal.time > lastSpectateChange )
	{
		spectator = entityNumber;
		if( player != NULL && player != this && !player->spectating && !player->IsInTeleport() )
		{
			newOrig = player->GetPhysics()->GetOrigin();
			if( player->physicsObj.IsCrouching() )
			{
				newOrig[ 2 ] += pm_crouchviewheight.GetFloat();
			}
			else
			{
				newOrig[ 2 ] += pm_normalviewheight.GetFloat();
			}
			newOrig[ 2 ] += SPECTATE_RAISE;
			idBounds b = idBounds( vec3_origin ).Expand( pm_spectatebbox.GetFloat() * 0.5f );
			idVec3 start = player->GetPhysics()->GetOrigin();
			start[2] += pm_spectatebbox.GetFloat() * 0.5f;
			trace_t t;
			// assuming spectate bbox is inside stand or crouch box
			gameLocal.clip.TraceBounds( t, start, newOrig, b, MASK_PLAYERSOLID, player );
			newOrig.Lerp( start, newOrig, t.fraction );
			SetOrigin( newOrig );
			idAngles angle = player->viewAngles;
			angle[ 2 ] = 0;
			SetViewAngles( angle );
		}
		else
		{
			SelectInitialSpawnPoint( spawn_origin, spawn_angles );
			spawn_origin[ 2 ] += pm_normalviewheight.GetFloat();
			spawn_origin[ 2 ] += SPECTATE_RAISE;
			SetOrigin( spawn_origin );
			SetViewAngles( spawn_angles );
			// This may happen during GAMESTATE_STARTUP in mp, so we must set the spawnAngles too.
			spawnAngles = spawn_angles;
			
			if( force == false )
			{
				// only do this if they hit the cycle button.
				if( common->IsServer() )
				{
					if( player != NULL )
					{
						idBitMsg	msg;
						byte		msgBuf[MAX_EVENT_PARAM_SIZE];
						
						msg.InitWrite( msgBuf, sizeof( msgBuf ) );
						msg.WriteFloat( GetPhysics()->GetOrigin()[0] );
						msg.WriteFloat( GetPhysics()->GetOrigin()[1] );
						msg.WriteFloat( GetPhysics()->GetOrigin()[2] );
						msg.WriteFloat( viewAngles[0] );
						msg.WriteFloat( viewAngles[1] );
						msg.WriteFloat( viewAngles[2] );
						
						ServerSendEvent( idPlayer::EVENT_FORCE_ORIGIN, &msg, false );
					}
				}
			}
		}
		lastSpectateChange = gameLocal.time + 500;
	}
	
	
}

/*
===============
idPlayer::SpectateCycle
===============
*/
void idPlayer::SpectateCycle()
{
	idPlayer* player;
	
	if( gameLocal.time > lastSpectateChange )
	{
		int latchedSpectator = spectator;
		spectator = gameLocal.GetNextClientNum( spectator );
		player = gameLocal.GetClientByNum( spectator );
		assert( player ); // never call here when the current spectator is wrong
		// ignore other spectators
		while( latchedSpectator != spectator && player->spectating )
		{
			spectator = gameLocal.GetNextClientNum( spectator );
			player = gameLocal.GetClientByNum( spectator );
		}
		lastSpectateChange = gameLocal.time + 500;
	}
}

/*
===============
idPlayer::UpdateSpectating
===============
*/
void idPlayer::UpdateSpectating()
{
	assert( spectating );
	assert( !common->IsClient() );
	assert( IsHidden() );
	idPlayer* player;
	if( !common->IsMultiplayer() )
	{
		return;
	}
	player = gameLocal.GetClientByNum( spectator );
	if( !player || ( player->spectating && player != this ) )
	{
		SpectateFreeFly( true );
	}
	else if( usercmd.buttons & BUTTON_JUMP )
	{
		SpectateFreeFly( false );
	}
	else if( usercmd.buttons & BUTTON_USE )
	{
		SpectateCycle();
	}
	else if( usercmd.buttons & BUTTON_ATTACK )
	{
		wantSpectate = false;
	}
}

/*
===============
idPlayer::HandleSingleGuiCommand
===============
*/
bool idPlayer::HandleSingleGuiCommand( idEntity* entityGui, idLexer* src )
{
	idToken token;
	
	if( !src->ReadToken( &token ) )
	{
		return false;
	}
	
	if( token == ";" )
	{
		return false;
	}
	
	if( token.Icmp( "addhealth" ) == 0 )
	{
		if( entityGui && health < 100 )
		{
			int _health = entityGui->spawnArgs.GetInt( "gui_parm1" );
			int amt = ( _health >= HEALTH_PER_DOSE ) ? HEALTH_PER_DOSE : _health;
			_health -= amt;
			entityGui->spawnArgs.SetInt( "gui_parm1", _health );
			if( entityGui->GetRenderEntity() && entityGui->GetRenderEntity()->gui[ 0 ] )
			{
				entityGui->GetRenderEntity()->gui[ 0 ]->SetStateInt( "gui_parm1", _health );
			}
			health += amt;
			if( health > 100 )
			{
				health = 100;
			}
		}
		return true;
	}
	src->UnreadToken( &token );
	return false;
}

/*
==============
idPlayer::PlayAudioLog
==============
*/
void idPlayer::PlayAudioLog( const idSoundShader* shader )
{
	EndVideoDisk();
	if( name.Length() > 0 )
	{
		int ms;
		StartSoundShader( shader, SND_CHANNEL_PDA_AUDIO, 0, false, &ms );
		CancelEvents( &EV_Player_StopAudioLog );
		PostEventMS( &EV_Player_StopAudioLog, ms + 150 );
	}
}

/*
==============
idPlayer::EndAudioLog
==============
*/
void idPlayer::EndAudioLog()
{
	StopSound( SND_CHANNEL_PDA_AUDIO, false );
}

/*
==============
idPlayer::PlayVideoDisk
==============
*/
void idPlayer::PlayVideoDisk( const idDeclVideo* decl )
{
	EndAudioLog();
	pdaVideoMat = decl->GetRoq();
	if( pdaVideoMat )
	{
		int c = pdaVideoMat->GetNumStages();
		for( int i = 0; i < c; i++ )
		{
			const shaderStage_t* stage = pdaVideoMat->GetStage( i );
			if( stage != NULL && stage->texture.cinematic )
			{
				stage->texture.cinematic->ResetTime( Sys_Milliseconds() );
			}
		}
		if( decl->GetWave() != NULL )
		{
			StartSoundShader( decl->GetWave(), SND_CHANNEL_PDA_VIDEO, 0, false, NULL );
		}
	}
}

/*
==============
idPlayer::EndVideoDisk
==============
*/
void idPlayer::EndVideoDisk()
{
	pdaVideoMat = NULL;
	StopSound( SND_CHANNEL_PDA_VIDEO, false );
}

/*
==============
idPlayer::Collide
==============
*/
bool idPlayer::Collide( const trace_t& collision, const idVec3& velocity )
{
	idEntity* other;
	
	if( common->IsClient() && spectating == false )
	{
		return false;
	}
	
	other = gameLocal.entities[ collision.c.entityNum ];
	if( other )
	{
		other->Signal( SIG_TOUCH );
		if( !spectating )
		{
			if( other->RespondsTo( EV_Touch ) )
			{
				other->ProcessEvent( &EV_Touch, this, &collision );
			}
		}
		else
		{
			if( other->RespondsTo( EV_SpectatorTouch ) )
			{
				other->ProcessEvent( &EV_SpectatorTouch, this, &collision );
			}
		}
	}
	return false;
}


/*
================
idPlayer::UpdateLocation

Searches nearby locations
================
*/
void idPlayer::UpdateLocation()
{

	if( hud )
	{
		hud->UpdateLocation( this );
	}
}

/*
================
idPlayer::ClearFocus

Clears the focus cursor
================
*/
void idPlayer::ClearFocus()
{
	focusCharacter	= NULL;
	focusGUIent		= NULL;
	focusUI			= NULL;
	focusVehicle	= NULL;
	talkCursor		= 0;
}

/*
================
idPlayer::UpdateFocus

Searches nearby entities for interactive guis, possibly making one of them
the focus and sending it a mouse move event
================
*/
void idPlayer::UpdateFocus()
{
	idClipModel* clipModelList[ MAX_GENTITIES ];
	idClipModel* clip;
	int			listedClipModels;
	idEntity*	oldFocus;
	idEntity*	ent;
	idUserInterface* oldUI;
	idAI*		oldChar;
	int			oldTalkCursor;
	idAFEntity_Vehicle* oldVehicle;
	int			i, j;
	idVec3		start, end;
	bool		allowFocus;
	const char* command;
	trace_t		trace;
	guiPoint_t	pt;
	const idKeyValue* kv;
	sysEvent_t	ev;
	idUserInterface* ui;
	
	if( gameLocal.inCinematic )
	{
		return;
	}
	
	// only update the focus character when attack button isn't pressed so players
	// can still chainsaw NPC's
	if( common->IsMultiplayer() || ( !focusCharacter && ( usercmd.buttons & BUTTON_ATTACK ) ) )
	{
		allowFocus = false;
	}
	else
	{
		allowFocus = true;
	}
	
	oldFocus		= focusGUIent;
	oldUI			= focusUI;
	oldChar			= focusCharacter;
	oldTalkCursor	= talkCursor;
	oldVehicle		= focusVehicle;
	
	if( focusTime <= gameLocal.time )
	{
		ClearFocus();
	}
	
	// don't let spectators interact with GUIs
	if( spectating )
	{
		return;
	}
	
	start = GetEyePosition();
	end = start + firstPersonViewAxis[0] * 80.0f;
	
	// player identification -> names to the hud
	if( common->IsMultiplayer() && IsLocallyControlled() )
	{
		idVec3 end = start + viewAngles.ToForward() * 768.0f;
		gameLocal.clip.TracePoint( trace, start, end, MASK_SHOT_BOUNDINGBOX, this );
		int iclient = -1;
		if( ( trace.fraction < 1.0f ) && ( trace.c.entityNum < MAX_CLIENTS ) )
		{
			iclient = trace.c.entityNum;
		}
		if( MPAim != iclient )
		{
			lastMPAim = MPAim;
			MPAim = iclient;
			lastMPAimTime = gameLocal.realClientTime;
		}
	}
	
	idBounds bounds( start );
	bounds.AddPoint( end );
	
	listedClipModels = gameLocal.clip.ClipModelsTouchingBounds( bounds, -1, clipModelList, MAX_GENTITIES );
	
	// no pretense at sorting here, just assume that there will only be one active
	// gui within range along the trace
	for( i = 0; i < listedClipModels; i++ )
	{
		clip = clipModelList[ i ];
		ent = clip->GetEntity();
		
		if( ent->IsHidden() )
		{
			continue;
		}
		
		if( allowFocus )
		{
			if( ent->IsType( idAFAttachment::Type ) )
			{
				idEntity* body = static_cast<idAFAttachment*>( ent )->GetBody();
				if( body != NULL && body->IsType( idAI::Type ) && ( static_cast<idAI*>( body )->GetTalkState() >= TALK_OK ) )
				{
					gameLocal.clip.TracePoint( trace, start, end, MASK_SHOT_RENDERMODEL, this );
					if( ( trace.fraction < 1.0f ) && ( trace.c.entityNum == ent->entityNumber ) )
					{
						ClearFocus();
						focusCharacter = static_cast<idAI*>( body );
						talkCursor = 1;
						focusTime = gameLocal.time + FOCUS_TIME;
						break;
					}
				}
				continue;
			}
			
			if( ent->IsType( idAI::Type ) )
			{
				if( static_cast<idAI*>( ent )->GetTalkState() >= TALK_OK )
				{
					gameLocal.clip.TracePoint( trace, start, end, MASK_SHOT_RENDERMODEL, this );
					if( ( trace.fraction < 1.0f ) && ( trace.c.entityNum == ent->entityNumber ) )
					{
						ClearFocus();
						focusCharacter = static_cast<idAI*>( ent );
						talkCursor = 1;
						focusTime = gameLocal.time + FOCUS_TIME;
						break;
					}
				}
				continue;
			}
			
			if( ent->IsType( idAFEntity_Vehicle::Type ) )
			{
				gameLocal.clip.TracePoint( trace, start, end, MASK_SHOT_RENDERMODEL, this );
				if( ( trace.fraction < 1.0f ) && ( trace.c.entityNum == ent->entityNumber ) )
				{
					ClearFocus();
					focusVehicle = static_cast<idAFEntity_Vehicle*>( ent );
					focusTime = gameLocal.time + FOCUS_TIME;
					break;
				}
				continue;
			}
		}
		
		if( !ent->GetRenderEntity() || !ent->GetRenderEntity()->gui[ 0 ] || !ent->GetRenderEntity()->gui[ 0 ]->IsInteractive() )
		{
			continue;
		}
		
		if( ent->spawnArgs.GetBool( "inv_item" ) )
		{
			// don't allow guis on pickup items focus
			continue;
		}
		
		pt = gameRenderWorld->GuiTrace( ent->GetModelDefHandle(), start, end );
		if( pt.x != -1 )
		{
			// we have a hit
			renderEntity_t* focusGUIrenderEntity = ent->GetRenderEntity();
			if( !focusGUIrenderEntity )
			{
				continue;
			}
			
			if( pt.guiId == 1 )
			{
				ui = focusGUIrenderEntity->gui[ 0 ];
			}
			else if( pt.guiId == 2 )
			{
				ui = focusGUIrenderEntity->gui[ 1 ];
			}
			else
			{
				ui = focusGUIrenderEntity->gui[ 2 ];
			}
			
			if( ui == NULL )
			{
				continue;
			}
			
			ClearFocus();
			focusGUIent = ent;
			focusUI = ui;
			
			if( oldFocus != ent )
			{
				// new activation
				// going to see if we have anything in inventory a gui might be interested in
				// need to enumerate inventory items
				focusUI->SetStateInt( "inv_count", inventory.items.Num() );
				for( j = 0; j < inventory.items.Num(); j++ )
				{
					idDict* item = inventory.items[ j ];
					const char* iname = item->GetString( "inv_name" );
					const char* iicon = item->GetString( "inv_icon" );
					const char* itext = item->GetString( "inv_text" );
					
					focusUI->SetStateString( va( "inv_name_%i", j ), iname );
					focusUI->SetStateString( va( "inv_icon_%i", j ), iicon );
					focusUI->SetStateString( va( "inv_text_%i", j ), itext );
					kv = item->MatchPrefix( "inv_id", NULL );
					if( kv )
					{
						focusUI->SetStateString( va( "inv_id_%i", j ), kv->GetValue() );
					}
					focusUI->SetStateInt( iname, 1 );
				}
				
				
				for( j = 0; j < inventory.pdaSecurity.Num(); j++ )
				{
					const char* p = inventory.pdaSecurity[ j ];
					if( p && *p )
					{
						focusUI->SetStateInt( p, 1 );
					}
				}
				
				int powerCellCount = 0;
				for( j = 0; j < inventory.items.Num(); j++ )
				{
					idDict* item = inventory.items[ j ];
					if( item->GetInt( "inv_powercell" ) )
					{
						powerCellCount++;
					}
				}
				focusUI->SetStateInt( "powercell_count", powerCellCount );
				
				int staminapercentage = ( int )( 100.0f * stamina / pm_stamina.GetFloat() );
				focusUI->SetStateString( "player_health", va( "%i", health ) );
				focusUI->SetStateString( "player_stamina", va( "%i%%", staminapercentage ) );
				focusUI->SetStateString( "player_armor", va( "%i%%", inventory.armor ) );
				
				kv = focusGUIent->spawnArgs.MatchPrefix( "gui_parm", NULL );
				while( kv )
				{
					focusUI->SetStateString( kv->GetKey(), kv->GetValue() );
					kv = focusGUIent->spawnArgs.MatchPrefix( "gui_parm", kv );
				}
			}
			
			// clamp the mouse to the corner
			ev = sys->GenerateMouseMoveEvent( -2000, -2000 );
			command = focusUI->HandleEvent( &ev, gameLocal.time );
			HandleGuiCommands( focusGUIent, command );
			
			// move to an absolute position
			ev = sys->GenerateMouseMoveEvent( pt.x * SCREEN_WIDTH, pt.y * SCREEN_HEIGHT );
			command = focusUI->HandleEvent( &ev, gameLocal.time );
			HandleGuiCommands( focusGUIent, command );
			focusTime = gameLocal.time + FOCUS_GUI_TIME;
			break;
		}
	}
	
	if( focusGUIent && focusUI )
	{
		if( !oldFocus || oldFocus != focusGUIent )
		{
			command = focusUI->Activate( true, gameLocal.time );
			HandleGuiCommands( focusGUIent, command );
			StartSound( "snd_guienter", SND_CHANNEL_ANY, 0, false, NULL );
			// HideTip();
			// HideObjective();
		}
	}
	else if( oldFocus && oldUI )
	{
		command = oldUI->Activate( false, gameLocal.time );
		HandleGuiCommands( oldFocus, command );
		StartSound( "snd_guiexit", SND_CHANNEL_ANY, 0, false, NULL );
	}
	
	if( hud )
	{
		hud->SetCursorState( this, CURSOR_TALK, talkCursor );
	}
	
	if( oldChar != focusCharacter && hud )
	{
		if( focusCharacter )
		{
			hud->SetCursorText( "#str_02036", focusCharacter->spawnArgs.GetString( "npc_name", "Joe" ) );
			hud->UpdateCursorState();
		}
		else
		{
			hud->SetCursorText( "", "" );
			hud->UpdateCursorState();
		}
	}
}

/*
=================
idPlayer::CrashLand

Check for hard landings that generate sound events
=================
*/
void idPlayer::CrashLand( const idVec3& oldOrigin, const idVec3& oldVelocity )
{
	idVec3		origin, velocity;
	idVec3		gravityVector, gravityNormal;
	float		delta;
	float		hardDelta, fatalDelta, softDelta;
	float		dist;
	float		vel, acc;
	float		t;
	float		a, b, c, den;
	waterLevel_t waterLevel;
	bool		noDamage;
	
	AI_SOFTLANDING = false;
	AI_HARDLANDING = false;
	
	// if the player is not on the ground
	if( !physicsObj.HasGroundContacts() )
	{
		return;
	}
	
	gravityNormal = physicsObj.GetGravityNormal();
	
	// if the player wasn't going down
	if( ( oldVelocity * -gravityNormal ) >= 0.0f )
	{
		return;
	}
	
	waterLevel = physicsObj.GetWaterLevel();
	
	// never take falling damage if completely underwater
	if( waterLevel == WATERLEVEL_HEAD )
	{
		return;
	}
	
	// no falling damage if touching a nodamage surface
	noDamage = false;
	for( int i = 0; i < physicsObj.GetNumContacts(); i++ )
	{
		const contactInfo_t& contact = physicsObj.GetContact( i );
		if( contact.material->GetSurfaceFlags() & SURF_NODAMAGE )
		{
			noDamage = true;
			StartSound( "snd_land_hard", SND_CHANNEL_ANY, 0, false, NULL );
			break;
		}
	}
	
	origin = GetPhysics()->GetOrigin();
	gravityVector = physicsObj.GetGravity();
	
	// calculate the exact velocity on landing
	dist = ( origin - oldOrigin ) * -gravityNormal;
	vel = oldVelocity * -gravityNormal;
	acc = -gravityVector.Length();
	
	a = acc / 2.0f;
	b = vel;
	c = -dist;
	
	den = b * b - 4.0f * a * c;
	if( den < 0 )
	{
		return;
	}
	t = ( -b - idMath::Sqrt( den ) ) / ( 2.0f * a );
	
	delta = vel + t * acc;
	delta = delta * delta * 0.0001;
	
	// reduce falling damage if there is standing water
	if( waterLevel == WATERLEVEL_WAIST )
	{
		delta *= 0.25f;
	}
	if( waterLevel == WATERLEVEL_FEET )
	{
		delta *= 0.5f;
	}
	
	if( delta < 1.0f )
	{
		return;
	}
	
	// allow falling a bit further for multiplayer
	if( common->IsMultiplayer() )
	{
		fatalDelta	= 75.0f;
		hardDelta	= 50.0f;
		softDelta	= 45.0f;
	}
	else
	{
		fatalDelta	= 65.0f;
		hardDelta	= 45.0f;
		softDelta	= 30.0f;
	}
	
	if( delta > fatalDelta )
	{
		AI_HARDLANDING = true;
		landChange = -32;
		landTime = gameLocal.time;
		if( !noDamage )
		{
			pain_debounce_time = gameLocal.time + pain_delay + 1;  // ignore pain since we'll play our landing anim
			Damage( NULL, NULL, idVec3( 0, 0, -1 ), "damage_fatalfall", 1.0f, 0 );
		}
	}
	else if( delta > hardDelta )
	{
		AI_HARDLANDING = true;
		landChange	= -24;
		landTime	= gameLocal.time;
		if( !noDamage )
		{
			pain_debounce_time = gameLocal.time + pain_delay + 1;  // ignore pain since we'll play our landing anim
			Damage( NULL, NULL, idVec3( 0, 0, -1 ), "damage_hardfall", 1.0f, 0 );
		}
	}
	else if( delta > softDelta )
	{
		AI_HARDLANDING = true;
		landChange	= -16;
		landTime	= gameLocal.time;
		if( !noDamage )
		{
			pain_debounce_time = gameLocal.time + pain_delay + 1;  // ignore pain since we'll play our landing anim
			Damage( NULL, NULL, idVec3( 0, 0, -1 ), "damage_softfall", 1.0f, 0 );
		}
	}
	else if( delta > 7 )
	{
		AI_SOFTLANDING = true;
		landChange	= -8;
		landTime	= gameLocal.time;
	}
	else if( delta > 3 )
	{
		// just walk on
	}
}

/*
===============
idPlayer::BobCycle
===============
*/
void idPlayer::BobCycle( const idVec3& pushVelocity )
{
	float		bobmove;
	int			old, deltaTime;
	idVec3		vel, gravityDir, velocity;
	idMat3		viewaxis;
	float		bob;
	float		delta;
	float		speed;
	float		f;
	
	//
	// calculate speed and cycle to be used for
	// all cyclic walking effects
	//
	velocity = physicsObj.GetLinearVelocity() - pushVelocity;
	
	gravityDir = physicsObj.GetGravityNormal();
	vel = velocity - ( velocity * gravityDir ) * gravityDir;
	xyspeed = vel.LengthFast();
	
	// do not evaluate the bob for other clients
	// when doing a spectate follow, don't do any weapon bobbing
	if( common->IsClient() && !IsLocallyControlled() )
	{
		viewBobAngles.Zero();
		viewBob.Zero();
		return;
	}
	
	if( !physicsObj.HasGroundContacts() || influenceActive == INFLUENCE_LEVEL2 || ( common->IsMultiplayer() && spectating ) )
	{
		// airborne
		bobCycle = 0;
		bobFoot = 0;
		bobfracsin = 0;
	}
	else if( ( !usercmd.forwardmove && !usercmd.rightmove ) || ( xyspeed <= MIN_BOB_SPEED ) )
	{
		// start at beginning of cycle again
		bobCycle = 0;
		bobFoot = 0;
		bobfracsin = 0;
	}
	else
	{
		if( physicsObj.IsCrouching() )
		{
			bobmove = pm_crouchbob.GetFloat();
			// ducked characters never play footsteps
		}
		else
		{
			// vary the bobbing based on the speed of the player
			bobmove = pm_walkbob.GetFloat() * ( 1.0f - bobFrac ) + pm_runbob.GetFloat() * bobFrac;
		}
		
		// check for footstep / splash sounds
		old = bobCycle;
		bobCycle = ( int )( old + bobmove * ( gameLocal.time - gameLocal.previousTime ) ) & 255;
		bobFoot = ( bobCycle & 128 ) >> 7;
		bobfracsin = idMath::Fabs( sin( ( bobCycle & 127 ) / 127.0 * idMath::PI ) );
	}
	
	// calculate angles for view bobbing
	viewBobAngles.Zero();
	
	viewaxis = viewAngles.ToMat3() * physicsObj.GetGravityAxis();
	
	// add angles based on velocity
	delta = velocity * viewaxis[0];
	viewBobAngles.pitch += delta * pm_runpitch.GetFloat();
	
	delta = velocity * viewaxis[1];
	viewBobAngles.roll -= delta * pm_runroll.GetFloat();
	
	// add angles based on bob
	// make sure the bob is visible even at low speeds
	speed = xyspeed > 200 ? xyspeed : 200;
	
	delta = bobfracsin * pm_bobpitch.GetFloat() * speed;
	if( physicsObj.IsCrouching() )
	{
		delta *= 3;		// crouching
	}
	viewBobAngles.pitch += delta;
	delta = bobfracsin * pm_bobroll.GetFloat() * speed;
	if( physicsObj.IsCrouching() )
	{
		delta *= 3;		// crouching accentuates roll
	}
	if( bobFoot & 1 )
	{
		delta = -delta;
	}
	viewBobAngles.roll += delta;
	
	// calculate position for view bobbing
	viewBob.Zero();
	
	if( physicsObj.HasSteppedUp() )
	{
	
		// check for stepping up before a previous step is completed
		deltaTime = gameLocal.time - stepUpTime;
		if( deltaTime < STEPUP_TIME )
		{
			stepUpDelta = stepUpDelta * ( STEPUP_TIME - deltaTime ) / STEPUP_TIME + physicsObj.GetStepUp();
		}
		else
		{
			stepUpDelta = physicsObj.GetStepUp();
		}
		if( stepUpDelta > 2.0f * pm_stepsize.GetFloat() )
		{
			stepUpDelta = 2.0f * pm_stepsize.GetFloat();
		}
		stepUpTime = gameLocal.time;
	}
	
	idVec3 gravity = physicsObj.GetGravityNormal();
	
	// if the player stepped up recently
	deltaTime = gameLocal.time - stepUpTime;
	if( deltaTime < STEPUP_TIME )
	{
		viewBob += gravity * ( stepUpDelta * ( STEPUP_TIME - deltaTime ) / STEPUP_TIME );
	}
	
	// add bob height after any movement smoothing
	bob = bobfracsin * xyspeed * pm_bobup.GetFloat();
	if( bob > 6 )
	{
		bob = 6;
	}
	viewBob[2] += bob;
	
	// add fall height
	delta = gameLocal.time - landTime;
	if( delta < LAND_DEFLECT_TIME )
	{
		f = delta / LAND_DEFLECT_TIME;
		viewBob -= gravity * ( landChange * f );
	}
	else if( delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME )
	{
		delta -= LAND_DEFLECT_TIME;
		f = 1.0 - ( delta / LAND_RETURN_TIME );
		viewBob -= gravity * ( landChange * f );
	}
}

/*
================
idPlayer::UpdateDeltaViewAngles
================
*/
void idPlayer::UpdateDeltaViewAngles( const idAngles& angles )
{
	// set the delta angle
	idAngles delta;
	for( int i = 0; i < 3; i++ )
	{
		delta[ i ] = angles[ i ] - SHORT2ANGLE( usercmd.angles[ i ] );
	}
	SetDeltaViewAngles( delta );
}

/*
================
idPlayer::SetViewAngles
================
*/
void idPlayer::SetViewAngles( const idAngles& angles )
{
	UpdateDeltaViewAngles( angles );
	viewAngles = angles;
}

/*
================
idPlayer::UpdateViewAngles
================
*/
void idPlayer::UpdateViewAngles()
{
	int i;
	idAngles delta;
	
	if( !noclip && ( gameLocal.inCinematic || privateCameraView || gameLocal.GetCamera() || influenceActive == INFLUENCE_LEVEL2 || objectiveSystemOpen ) )
	{
		// no view changes at all, but we still want to update the deltas or else when
		// we get out of this mode, our view will snap to a kind of random angle
		UpdateDeltaViewAngles( viewAngles );
		return;
	}
	
	// if dead
	if( health <= 0 )
	{
		if( pm_thirdPersonDeath.GetBool() )
		{
			viewAngles.roll = 0.0f;
			viewAngles.pitch = 30.0f;
		}
		else
		{
			viewAngles.roll = 40.0f;
			viewAngles.pitch = -15.0f;
		}
		return;
	}
	
	//
	
	
	// circularly clamp the angles with deltas
	for( i = 0; i < 3; i++ )
	{
		cmdAngles[i] = SHORT2ANGLE( usercmd.angles[i] );
		if( influenceActive == INFLUENCE_LEVEL3 )
		{
			viewAngles[i] += idMath::ClampFloat( -1.0f, 1.0f, idMath::AngleDelta( idMath::AngleNormalize180( SHORT2ANGLE( usercmd.angles[i] ) + deltaViewAngles[i] ) , viewAngles[i] ) );
		}
		else
		{
			viewAngles[i] = idMath::AngleNormalize180( SHORT2ANGLE( usercmd.angles[i] ) + deltaViewAngles[i] );
		}
	}
	if( !centerView.IsDone( gameLocal.time ) )
	{
		viewAngles.pitch = centerView.GetCurrentValue( gameLocal.time );
	}
	
	// clamp the pitch
	if( noclip )
	{
		if( viewAngles.pitch > 89.0f )
		{
			// don't let the player look down more than 89 degrees while noclipping
			viewAngles.pitch = 89.0f;
		}
		else if( viewAngles.pitch < -89.0f )
		{
			// don't let the player look up more than 89 degrees while noclipping
			viewAngles.pitch = -89.0f;
		}
	}
	else if( mountedObject )
	{
		int yaw_min, yaw_max, varc;
		
		mountedObject->GetAngleRestrictions( yaw_min, yaw_max, varc );
		
		if( yaw_min < yaw_max )
		{
			viewAngles.yaw = idMath::ClampFloat( yaw_min, yaw_max, viewAngles.yaw );
		}
		else
		{
			if( viewAngles.yaw < 0 )
			{
				viewAngles.yaw = idMath::ClampFloat( -180.f, yaw_max, viewAngles.yaw );
			}
			else
			{
				viewAngles.yaw = idMath::ClampFloat( yaw_min, 180.f, viewAngles.yaw );
			}
		}
		viewAngles.pitch = idMath::ClampFloat( -varc, varc, viewAngles.pitch );
	}
	else
	{
		// don't let the player look up or down more than 90 degrees normally
		const float restrict = 1.0f;
		
		viewAngles.pitch = std::min( viewAngles.pitch, pm_maxviewpitch.GetFloat() * restrict );
		viewAngles.pitch = std::max( viewAngles.pitch, pm_minviewpitch.GetFloat() * restrict );
	}
	
	UpdateDeltaViewAngles( viewAngles );
	
	// orient the model towards the direction we're looking
	SetAngles( idAngles( 0, viewAngles.yaw, 0 ) );
	
	// save in the log for analyzing weapon angle offsets
	loggedViewAngles[ gameLocal.framenum & ( NUM_LOGGED_VIEW_ANGLES - 1 ) ] = viewAngles;
}

/*
==============
idPlayer::AdjustHeartRate

Player heartrate works as follows

DEF_HEARTRATE is resting heartrate

Taking damage when health is above 75 adjusts heart rate by 1 beat per second
Taking damage when health is below 75 adjusts heart rate by 5 beats per second
Maximum heartrate from damage is MAX_HEARTRATE

Firing a weapon adds 1 beat per second up to a maximum of COMBAT_HEARTRATE

Being at less than 25% stamina adds 5 beats per second up to ZEROSTAMINA_HEARTRATE

All heartrates are target rates.. the heart rate will start falling as soon as there have been no adjustments for 5 seconds
Once it starts falling it always tries to get to DEF_HEARTRATE

The exception to the above rule is upon death at which point the rate is set to DYING_HEARTRATE and starts falling
immediately to zero

Heart rate volumes go from zero ( -40 db for DEF_HEARTRATE to 5 db for MAX_HEARTRATE ) the volume is
scaled linearly based on the actual rate

Exception to the above rule is once the player is dead, the dying heart rate starts at either the current volume if
it is audible or -10db and scales to 8db on the last few beats
==============
*/
void idPlayer::AdjustHeartRate( int target, float timeInSecs, float delay, bool force )
{

	if( heartInfo.GetEndValue() == target )
	{
		return;
	}
	
	if( AI_DEAD && !force )
	{
		return;
	}
	
	lastHeartAdjust = gameLocal.time;
	
	heartInfo.Init( gameLocal.time + delay * 1000, timeInSecs * 1000, heartRate, target );
}

/*
==============
idPlayer::GetBaseHeartRate
==============
*/
int idPlayer::GetBaseHeartRate()
{
	int base = idMath::Ftoi( ( BASE_HEARTRATE + LOWHEALTH_HEARTRATE_ADJ ) - ( ( float )health / 100.0f ) * LOWHEALTH_HEARTRATE_ADJ );
	int rate = idMath::Ftoi( base + ( ZEROSTAMINA_HEARTRATE - base ) * ( 1.0f - stamina / pm_stamina.GetFloat() ) );
	int diff = ( lastDmgTime ) ? gameLocal.time - lastDmgTime : 99999;
	rate += ( diff < 5000 ) ? ( diff < 2500 ) ? ( diff < 1000 ) ? 15 : 10 : 5 : 0;
	return rate;
}

/*
==============
idPlayer::SetCurrentHeartRate
==============
*/
void idPlayer::SetCurrentHeartRate()
{

	int base = idMath::Ftoi( ( BASE_HEARTRATE + LOWHEALTH_HEARTRATE_ADJ ) - ( ( float ) health / 100.0f ) * LOWHEALTH_HEARTRATE_ADJ );
	
	if( PowerUpActive( ADRENALINE ) )
	{
		heartRate = 135;
	}
	else
	{
		heartRate = idMath::Ftoi( heartInfo.GetCurrentValue( gameLocal.time ) );
		int currentRate = GetBaseHeartRate();
		if( health >= 0 && gameLocal.time > lastHeartAdjust + 2500 )
		{
			AdjustHeartRate( currentRate, 2.5f, 0.0f, false );
		}
	}
	
	int bps = idMath::Ftoi( 60.0f / heartRate * 1000.0f );
	if( gameLocal.time - lastHeartBeat > bps )
	{
		int dmgVol = DMG_VOLUME;
		int deathVol = DEATH_VOLUME;
		int zeroVol = ZERO_VOLUME;
		float pct = 0.0;
		if( heartRate > BASE_HEARTRATE && health > 0 )
		{
			pct = ( float )( heartRate - base ) / ( MAX_HEARTRATE - base );
			pct *= ( ( float )dmgVol - ( float )zeroVol );
		}
		else if( health <= 0 )
		{
			pct = ( float )( heartRate - DYING_HEARTRATE ) / ( BASE_HEARTRATE - DYING_HEARTRATE );
			if( pct > 1.0f )
			{
				pct = 1.0f;
			}
			else if( pct < 0.0f )
			{
				pct = 0.0f;
			}
			pct *= ( ( float )deathVol - ( float )zeroVol );
		}
		
		pct += ( float )zeroVol;
		
		if( pct != zeroVol )
		{
			StartSound( "snd_heartbeat", SND_CHANNEL_HEART, SSF_PRIVATE_SOUND, false, NULL );
			// modify just this channel to a custom volume
			soundShaderParms_t	parms;
			memset( &parms, 0, sizeof( parms ) );
			parms.volume = pct;
			refSound.referenceSound->ModifySound( SND_CHANNEL_HEART, &parms );
		}
		
		lastHeartBeat = gameLocal.time;
	}
}

/*
==============
idPlayer::UpdateAir
==============
*/
void idPlayer::UpdateAir()
{
	if( health <= 0 )
	{
		return;
	}
	
	// see if the player is connected to the info_vacuum
	bool	newAirless = false;
	
	if( gameLocal.vacuumAreaNum != -1 )
	{
		int	num = GetNumPVSAreas();
		if( num > 0 )
		{
			int		areaNum;
			
			// if the player box spans multiple areas, get the area from the origin point instead,
			// otherwise a rotating player box may poke into an outside area
			if( num == 1 )
			{
				const int*	pvsAreas = GetPVSAreas();
				areaNum = pvsAreas[0];
			}
			else
			{
				areaNum = gameRenderWorld->PointInArea( this->GetPhysics()->GetOrigin() );
			}
			newAirless = gameRenderWorld->AreasAreConnected( gameLocal.vacuumAreaNum, areaNum, PS_BLOCK_AIR );
		}
	}
	
	if( PowerUpActive( ENVIROTIME ) )
	{
		newAirless = false;
	}
	
	if( newAirless )
	{
		if( !airless )
		{
			StartSound( "snd_decompress", SND_CHANNEL_ANY, SSF_GLOBAL, false, NULL );
			StartSound( "snd_noAir", SND_CHANNEL_BODY2, 0, false, NULL );
		}
		airMsec -= ( gameLocal.time - gameLocal.previousTime );
		if( airMsec < 0 )
		{
			airMsec = 0;
			// check for damage
			const idDict* damageDef = gameLocal.FindEntityDefDict( "damage_noair", false );
			int dmgTiming = 1000 * ( ( damageDef ) ? damageDef->GetFloat( "delay", "3.0" ) : 3.0f );
			if( gameLocal.time > lastAirDamage + dmgTiming )
			{
				Damage( NULL, NULL, vec3_origin, "damage_noair", 1.0f, 0 );
				lastAirDamage = gameLocal.time;
			}
		}
		
	}
	else
	{
		if( airless )
		{
			StartSound( "snd_recompress", SND_CHANNEL_ANY, SSF_GLOBAL, false, NULL );
			StopSound( SND_CHANNEL_BODY2, false );
		}
		airMsec += ( gameLocal.time - gameLocal.previousTime );	// regain twice as fast as lose
		if( airMsec > pm_airMsec.GetInteger() )
		{
			airMsec = pm_airMsec.GetInteger();
		}
	}
	
	airless = newAirless;
	
	if( hud )
	{
		hud->UpdateOxygen( airless, 100 * airMsec / pm_airMsec.GetInteger() );
	}
}

void idPlayer::UpdatePowerupHud()
{

	if( health <= 0 )
	{
		return;
	}
	
	if( lastHudPowerup != hudPowerup )
	{
	
		if( hudPowerup == -1 )
		{
			//The powerup hud should be turned off
			// TODO_SPARTY: powerups??
			//if ( hud ) {
			//	hud->HandleNamedEvent( "noPowerup" );
			//}
		}
		else
		{
			//Turn the pwoerup hud on
			// TODO_SPARTY: powerups??
			//if ( hud ) {
			//	hud->HandleNamedEvent( "Powerup" );
			//}
		}
		
		lastHudPowerup = hudPowerup;
	}
	
	if( hudPowerup != -1 && hudPowerup < MAX_POWERUPS )
	{
		if( PowerUpActive( hudPowerup ) )
		{
			//int remaining = inventory.powerupEndTime[ hudPowerup ] - gameLocal.time;
			//int filledbar = idMath::ClampInt( 0, hudPowerupDuration, remaining );
			
			// TODO_SPARTY: powerups??
			//if ( hud ) {
			//	hud->SetStateInt( "player_powerup", 100 * filledbar / hudPowerupDuration );
			//	hud->SetStateInt( "player_poweruptime", remaining / 1000 );
			//}
		}
	}
}

/*
==============
idPlayer::GetPDA
==============
 */
const idDeclPDA* idPlayer::GetPDA() const
{
	if( inventory.pdas.Num() > 0 )
	{
		return inventory.pdas[ 0 ];
	}
	else
	{
		return NULL;
	}
}


/*
==============
idPlayer::GetVideo
==============
*/
const idDeclVideo* idPlayer::GetVideo( int index )
{
	if( index >= 0 && index < inventory.videos.Num() )
	{
		return inventory.videos[index];
	}
	return NULL;
}

/*
==============
idPlayer::TogglePDA
==============
*/
void idPlayer::TogglePDA()
{

	if( inventory.pdas.Num() == 0 )
	{
		ShowTip( spawnArgs.GetString( "text_infoTitle" ), spawnArgs.GetString( "text_noPDA" ), true );
		return;
	}
	
	if( pdaMenu != NULL )
	{
		objectiveSystemOpen = !objectiveSystemOpen;
		pdaMenu->ActivateMenu( objectiveSystemOpen );
		
		if( objectiveSystemOpen )
		{
			if( hud )
			{
				hud->ClearNewPDAInfo();
			}
		}
	}
}

/*
==============
idPlayer::Spectate
==============
*/
void idPlayer::Spectate( bool spectate, bool force )
{
	spectating = spectate;
	
	if( spectating )
	{
		// join the spectators
		ClearPowerUps();
		spectator = this->entityNumber;
		Init();
		StopRagdoll();
		SetPhysics( &physicsObj );
		physicsObj.DisableClip();
		FlashlightOff();
		Hide();
		Event_DisableWeapon();
		
		// Raise me up by a little bit. if i'm the local client.
		if( IsLocallyControlled() )
		{
			SetSpectateOrigin();
		}
		
		HideRespawnHudMessage();
		
		idLib::Printf( "DMP _ GENERAL :> Player %d Spectating \n", entityNumber );
	}
	else
	{
		// put everything back together again
		currentWeapon = -1;	// to make sure the def will be loaded if necessary
		Show();
		Event_EnableWeapon();
		idLib::Printf( "DMP _ GENERAL :> Player %d Not Spectating \n", entityNumber );
		SetEyeHeight( pm_normalviewheight.GetFloat() );
	}
	SetClipModel();
}

/*
==============
idPlayer::SetClipModel
==============
*/
void idPlayer::SetClipModel()
{
	idBounds bounds;
	
	if( spectating )
	{
		bounds = idBounds( vec3_origin ).Expand( pm_spectatebbox.GetFloat() * 0.5f );
	}
	else
	{
		bounds[0].Set( -pm_bboxwidth.GetFloat() * 0.5f, -pm_bboxwidth.GetFloat() * 0.5f, 0 );
		bounds[1].Set( pm_bboxwidth.GetFloat() * 0.5f, pm_bboxwidth.GetFloat() * 0.5f, pm_normalheight.GetFloat() );
	}
	// the origin of the clip model needs to be set before calling SetClipModel
	// otherwise our physics object's current origin value gets reset to 0
	idClipModel* newClip;
	if( pm_usecylinder.GetBool() )
	{
		newClip = new( TAG_PHYSICS_CLIP_ENTITY ) idClipModel( idTraceModel( bounds, 8 ) );
		newClip->Translate( physicsObj.PlayerGetOrigin() );
		physicsObj.SetClipModel( newClip, 1.0f );
	}
	else
	{
		newClip = new( TAG_PHYSICS_CLIP_ENTITY ) idClipModel( idTraceModel( bounds ) );
		newClip->Translate( physicsObj.PlayerGetOrigin() );
		physicsObj.SetClipModel( newClip, 1.0f );
	}
}

/*
==============
idPlayer::UseVehicle
==============
*/
void idPlayer::UseVehicle()
{
	trace_t	trace;
	idVec3 start, end;
	idEntity* ent;
	
	if( GetBindMaster() && GetBindMaster()->IsType( idAFEntity_Vehicle::Type ) )
	{
		Show();
		static_cast<idAFEntity_Vehicle*>( GetBindMaster() )->Use( this );
	}
	else
	{
		start = GetEyePosition();
		end = start + viewAngles.ToForward() * 80.0f;
		gameLocal.clip.TracePoint( trace, start, end, MASK_SHOT_RENDERMODEL, this );
		if( trace.fraction < 1.0f )
		{
			ent = gameLocal.entities[ trace.c.entityNum ];
			if( ent && ent->IsType( idAFEntity_Vehicle::Type ) )
			{
				Hide();
				static_cast<idAFEntity_Vehicle*>( ent )->Use( this );
			}
		}
	}
}

/*
==============
idPlayer::PerformImpulse
==============
*/
void idPlayer::PerformImpulse( int impulse )
{
	bool isIntroMap = ( idStr::FindText( gameLocal.GetMapFileName(), "mars_city1" ) >= 0 );
	
	// Normal 1 - 0 Keys.
	if( impulse >= IMPULSE_0 && impulse <= IMPULSE_12 && !isIntroMap )
	{
		SelectWeapon( impulse, false );
		return;
	}
	
	// DPAD Weapon Quick Select
	if( impulse >= IMPULSE_28 && impulse <= IMPULSE_31 && !isIntroMap )
	{
		SelectWeapon( impulse, false );
		return;
	}
	
	switch( impulse )
	{
		case IMPULSE_13:
		{
			Reload();
			break;
		}
		case IMPULSE_14:
		{
			if( !isIntroMap )
			{
				NextWeapon();
			}
			break;
		}
		case IMPULSE_15:
		{
			if( !isIntroMap )
			{
				PrevWeapon();
			}
			break;
		}
		case IMPULSE_16:
		{
			if( flashlight.IsValid() )
			{
				if( flashlight.GetEntity()->lightOn )
				{
					FlashlightOff();
				}
				else if( !spectating && weaponEnabled && !hiddenWeapon && !gameLocal.world->spawnArgs.GetBool( "no_Weapons" ) )
				{
					FlashlightOn();
				}
			}
			break;
		}
		case IMPULSE_19:
		{
			// when we're not in single player, IMPULSE_19 is used for showScores
			// otherwise it opens the pda
			if( !common->IsMultiplayer() )
			{
#if !defined(ID_RETAIL) && !defined(ID_RETAIL_INTERNAL)
				if( !common->KeyState( 56 ) )  		// don't toggle PDA when LEFT ALT is down
				{
#endif
					if( objectiveSystemOpen )
					{
						TogglePDA();
					}
					else if( weapon_pda >= 0 )
					{
						SelectWeapon( weapon_pda, true );
					}
#if !defined(ID_RETAIL) && !defined(ID_RETAIL_INTERNAL)
				}
#endif
			}
			else
			{
				if( IsLocallyControlled() )
				{
					gameLocal.mpGame.SetScoreboardActive( true );
				}
			}
			break;
		}
		case IMPULSE_22:
		{
			if( common->IsClient() || IsLocallyControlled() )
			{
				gameLocal.mpGame.ToggleSpectate();
			}
			break;
		}
		case IMPULSE_25:
		{
			if( common->IsServer() && gameLocal.mpGame.IsGametypeFlagBased() && ( gameLocal.serverInfo.GetInt( "si_midnight" ) == 2 ) )
			{
				if( enviroSuitLight.IsValid() )
				{
					enviroSuitLight.GetEntity()->PostEventMS( &EV_Remove, 0 );
					enviroSuitLight = NULL;
				}
				else
				{
					const idDict* lightDef = gameLocal.FindEntityDefDict( "envirosuit_light", false );
					if( lightDef )
					{
						idEntity* temp = static_cast<idEntity*>( enviroSuitLight.GetEntity() );
						idAngles lightAng = firstPersonViewAxis.ToAngles();
						idVec3 lightOrg = firstPersonViewOrigin;
						
						idVec3 enviroOffset = lightDef->GetVector( "enviro_offset" );
						idVec3 enviroAngleOffset = lightDef->GetVector( "enviro_angle_offset" );
						
						gameLocal.SpawnEntityDef( *lightDef, &temp, false );
						enviroSuitLight = static_cast<idLight*>( temp );
						
						enviroSuitLight.GetEntity()->fl.networkSync = true;
						
						lightOrg += ( enviroOffset.x * firstPersonViewAxis[0] );
						lightOrg += ( enviroOffset.y * firstPersonViewAxis[1] );
						lightOrg += ( enviroOffset.z * firstPersonViewAxis[2] );
						lightAng.pitch += enviroAngleOffset.x;
						lightAng.yaw += enviroAngleOffset.y;
						lightAng.roll += enviroAngleOffset.z;
						
						enviroSuitLight.GetEntity()->GetPhysics()->SetOrigin( lightOrg );
						enviroSuitLight.GetEntity()->GetPhysics()->SetAxis( lightAng.ToMat3() );
						
						enviroSuitLight.GetEntity()->UpdateVisuals();
						enviroSuitLight.GetEntity()->Present();
					}
				}
			}
			break;
		}
		//Hack so the chainsaw will work in MP
		case IMPULSE_27:
		{
			SelectWeapon( 18, false );
			break;
		}
	}
}

/*
==============
idPlayer::EvaluateControls
==============
*/
void idPlayer::EvaluateControls()
{
	// check for respawning
	if( health <= 0 && !g_testDeath.GetBool() )
	{
		if( common->IsMultiplayer() )
		{
			// in MP, idMultiplayerGame decides spawns
			if( ( gameLocal.time > minRespawnTime ) && ( usercmd.buttons & BUTTON_ATTACK ) )
			{
				forceRespawn = true;
			}
			else if( gameLocal.time > maxRespawnTime )
			{
				forceRespawn = true;
			}
		}
		else
		{
			// in single player, we let the session handle restarting the level or loading a game
			if( gameLocal.time > minRespawnTime )
			{
				gameLocal.sessionCommand = "died";
			}
		}
	}
	
	if( usercmd.impulseSequence != oldImpulseSequence )
	{
		PerformImpulse( usercmd.impulse );
	}
	
	if( forceScoreBoard )
	{
		gameLocal.mpGame.SetScoreboardActive( true );
	}
	
	oldImpulseSequence = usercmd.impulseSequence;
	
	AdjustSpeed();
	
	// update the viewangles
	UpdateViewAngles();
}

/*
==============
idPlayer::AdjustSpeed
==============
*/
void idPlayer::AdjustSpeed()
{
	float speed;
	float rate;
	
	if( spectating )
	{
		speed = pm_spectatespeed.GetFloat();
		bobFrac = 0.0f;
	}
	else if( noclip )
	{
		speed = pm_noclipspeed.GetFloat();
		bobFrac = 0.0f;
	}
	else if( !physicsObj.OnLadder() && ( usercmd.buttons & BUTTON_RUN ) && ( usercmd.forwardmove || usercmd.rightmove ) && !( usercmd.buttons & BUTTON_CROUCH ) )
	{
		if( !common->IsMultiplayer() && !physicsObj.IsCrouching() && !PowerUpActive( ADRENALINE ) )
		{
			stamina -= MS2SEC( gameLocal.time - gameLocal.previousTime );
		}
		if( stamina < 0 )
		{
			stamina = 0;
		}
		if( ( !pm_stamina.GetFloat() ) || ( stamina > pm_staminathreshold.GetFloat() ) )
		{
			bobFrac = 1.0f;
		}
		else if( pm_staminathreshold.GetFloat() <= 0.0001f )
		{
			bobFrac = 0.0f;
		}
		else
		{
			bobFrac = stamina / pm_staminathreshold.GetFloat();
		}
		speed = pm_walkspeed.GetFloat() * ( 1.0f - bobFrac ) + pm_runspeed.GetFloat() * bobFrac;
	}
	else
	{
		rate = pm_staminarate.GetFloat();
		
		// increase 25% faster when not moving
		if( ( usercmd.forwardmove == 0 ) && ( usercmd.rightmove == 0 ) && ( !physicsObj.OnLadder() || ( ( usercmd.buttons & ( BUTTON_CROUCH | BUTTON_JUMP ) ) == 0 ) ) )
		{
			rate *= 1.25f;
		}
		
		stamina += rate * MS2SEC( gameLocal.time - gameLocal.previousTime );
		if( stamina > pm_stamina.GetFloat() )
		{
			stamina = pm_stamina.GetFloat();
		}
		speed = pm_walkspeed.GetFloat();
		bobFrac = 0.0f;
	}
	
	speed *= PowerUpModifier( SPEED );
	
	if( influenceActive == INFLUENCE_LEVEL3 )
	{
		speed *= 0.33f;
	}
	
	physicsObj.SetSpeed( speed, pm_crouchspeed.GetFloat() );
}

/*
==============
idPlayer::AdjustBodyAngles
==============
*/
void idPlayer::AdjustBodyAngles()
{
	idMat3	lookAxis;
	idMat3	legsAxis;
	bool	blend;
	float	diff;
	float	frac;
	float	upBlend;
	float	forwardBlend;
	float	downBlend;
	
	if( health < 0 )
	{
		return;
	}
	
	blend = true;
	
	if( !physicsObj.HasGroundContacts() )
	{
		idealLegsYaw = 0.0f;
		legsForward = true;
	}
	else if( usercmd.forwardmove < 0 )
	{
		idealLegsYaw = idMath::AngleNormalize180( idVec3( -usercmd.forwardmove, usercmd.rightmove, 0.0f ).ToYaw() );
		legsForward = false;
	}
	else if( usercmd.forwardmove > 0 )
	{
		idealLegsYaw = idMath::AngleNormalize180( idVec3( usercmd.forwardmove, -usercmd.rightmove, 0.0f ).ToYaw() );
		legsForward = true;
	}
	else if( ( usercmd.rightmove != 0 ) && physicsObj.IsCrouching() )
	{
		if( !legsForward )
		{
			idealLegsYaw = idMath::AngleNormalize180( idVec3( idMath::Abs( usercmd.rightmove ), usercmd.rightmove, 0.0f ).ToYaw() );
		}
		else
		{
			idealLegsYaw = idMath::AngleNormalize180( idVec3( idMath::Abs( usercmd.rightmove ), -usercmd.rightmove, 0.0f ).ToYaw() );
		}
	}
	else if( usercmd.rightmove != 0 )
	{
		idealLegsYaw = 0.0f;
		legsForward = true;
	}
	else
	{
		legsForward = true;
		diff = idMath::Fabs( idealLegsYaw - legsYaw );
		idealLegsYaw = idealLegsYaw - idMath::AngleNormalize180( viewAngles.yaw - oldViewYaw );
		if( diff < 0.1f )
		{
			legsYaw = idealLegsYaw;
			blend = false;
		}
	}
	
	if( !physicsObj.IsCrouching() )
	{
		legsForward = true;
	}
	
	oldViewYaw = viewAngles.yaw;
	
	AI_TURN_LEFT = false;
	AI_TURN_RIGHT = false;
	if( idealLegsYaw < -45.0f )
	{
		idealLegsYaw = 0;
		AI_TURN_RIGHT = true;
		blend = true;
	}
	else if( idealLegsYaw > 45.0f )
	{
		idealLegsYaw = 0;
		AI_TURN_LEFT = true;
		blend = true;
	}
	
	if( blend )
	{
		legsYaw = legsYaw * 0.9f + idealLegsYaw * 0.1f;
	}
	legsAxis = idAngles( 0.0f, legsYaw, 0.0f ).ToMat3();
	animator.SetJointAxis( hipJoint, JOINTMOD_WORLD, legsAxis );
	
	// calculate the blending between down, straight, and up
	frac = viewAngles.pitch / 90.0f;
	if( frac > 0.0f )
	{
		downBlend		= frac;
		forwardBlend	= 1.0f - frac;
		upBlend			= 0.0f;
	}
	else
	{
		downBlend		= 0.0f;
		forwardBlend	= 1.0f + frac;
		upBlend			= -frac;
	}
	
	animator.CurrentAnim( ANIMCHANNEL_TORSO )->SetSyncedAnimWeight( 0, downBlend );
	animator.CurrentAnim( ANIMCHANNEL_TORSO )->SetSyncedAnimWeight( 1, forwardBlend );
	animator.CurrentAnim( ANIMCHANNEL_TORSO )->SetSyncedAnimWeight( 2, upBlend );
	
	animator.CurrentAnim( ANIMCHANNEL_LEGS )->SetSyncedAnimWeight( 0, downBlend );
	animator.CurrentAnim( ANIMCHANNEL_LEGS )->SetSyncedAnimWeight( 1, forwardBlend );
	animator.CurrentAnim( ANIMCHANNEL_LEGS )->SetSyncedAnimWeight( 2, upBlend );
}

/*
==============
idPlayer::InitAASLocation
==============
*/
void idPlayer::InitAASLocation()
{
	int		i;
	int		num;
	idVec3	size;
	idBounds bounds;
	idAAS*	aas;
	idVec3	origin;
	
	GetFloorPos( 64.0f, origin );
	
	num = gameLocal.NumAAS();
	aasLocation.SetGranularity( 1 );
	aasLocation.SetNum( num );
	for( i = 0; i < aasLocation.Num(); i++ )
	{
		aasLocation[ i ].areaNum = 0;
		aasLocation[ i ].pos = origin;
		aas = gameLocal.GetAAS( i );
		if( aas != NULL && aas->GetSettings() )
		{
			size = aas->GetSettings()->boundingBoxes[0][1];
			bounds[0] = -size;
			size.z = 32.0f;
			bounds[1] = size;
			
			aasLocation[ i ].areaNum = aas->PointReachableAreaNum( origin, bounds, AREA_REACHABLE_WALK );
		}
	}
}

/*
==============
idPlayer::SetAASLocation
==============
*/
void idPlayer::SetAASLocation()
{
	int		i;
	int		areaNum;
	idVec3	size;
	idBounds bounds;
	idAAS*	aas;
	idVec3	origin;
	
	if( !GetFloorPos( 64.0f, origin ) )
	{
		return;
	}
	
	for( i = 0; i < aasLocation.Num(); i++ )
	{
		aas = gameLocal.GetAAS( i );
		if( !aas )
		{
			continue;
		}
		
		size = aas->GetSettings()->boundingBoxes[0][1];
		bounds[0] = -size;
		size.z = 32.0f;
		bounds[1] = size;
		
		areaNum = aas->PointReachableAreaNum( origin, bounds, AREA_REACHABLE_WALK );
		if( areaNum )
		{
			aasLocation[ i ].pos = origin;
			aasLocation[ i ].areaNum = areaNum;
		}
	}
}

/*
==============
idPlayer::GetAASLocation
==============
*/
void idPlayer::GetAASLocation( idAAS* aas, idVec3& pos, int& areaNum ) const
{
	int i;
	
	if( aas != NULL )
	{
		for( i = 0; i < aasLocation.Num(); i++ )
		{
			if( aas == gameLocal.GetAAS( i ) )
			{
				areaNum = aasLocation[ i ].areaNum;
				pos = aasLocation[ i ].pos;
				return;
			}
		}
	}
	
	areaNum = 0;
	pos = physicsObj.GetOrigin();
}

/*
==============
idPlayer::Move_Interpolated
==============
*/
void idPlayer::Move_Interpolated( float fraction )
{

	float newEyeOffset;
	idVec3 oldOrigin;
	idVec3 oldVelocity;
	idVec3 pushVelocity;
	
	// save old origin and velocity for crashlanding
	oldOrigin = physicsObj.GetOrigin();
	oldVelocity = physicsObj.GetLinearVelocity();
	pushVelocity = physicsObj.GetPushedLinearVelocity();
	
	// set physics variables
	physicsObj.SetMaxStepHeight( pm_stepsize.GetFloat() );
	physicsObj.SetMaxJumpHeight( pm_jumpheight.GetFloat() );
	
	if( noclip )
	{
		physicsObj.SetContents( 0 );
		physicsObj.SetMovementType( PM_NOCLIP );
	}
	else if( spectating )
	{
		physicsObj.SetContents( 0 );
		physicsObj.SetMovementType( PM_SPECTATOR );
	}
	else if( health <= 0 )
	{
		physicsObj.SetContents( CONTENTS_CORPSE | CONTENTS_MONSTERCLIP );
		physicsObj.SetMovementType( PM_DEAD );
	}
	else if( gameLocal.inCinematic || gameLocal.GetCamera() || privateCameraView || ( influenceActive == INFLUENCE_LEVEL2 ) )
	{
		physicsObj.SetContents( CONTENTS_BODY );
		physicsObj.SetMovementType( PM_FREEZE );
	}
	else if( mountedObject )
	{
		physicsObj.SetContents( 0 );
		physicsObj.SetMovementType( PM_FREEZE );
	}
	else
	{
		physicsObj.SetContents( CONTENTS_BODY );
		physicsObj.SetMovementType( PM_NORMAL );
	}
	
	if( spectating )
	{
		physicsObj.SetClipMask( MASK_DEADSOLID );
	}
	else if( health <= 0 )
	{
		physicsObj.SetClipMask( MASK_DEADSOLID );
	}
	else
	{
		physicsObj.SetClipMask( MASK_PLAYERSOLID );
	}
	
	physicsObj.SetDebugLevel( g_debugMove.GetBool() );
	
	{
		idVec3	org;
		idMat3	axis;
		GetViewPos( org, axis );
		
		physicsObj.SetPlayerInput( usercmd, axis[0] );
	}
	
	// FIXME: physics gets disabled somehow
	BecomeActive( TH_PHYSICS );
	InterpolatePhysics( fraction );
	
	// update our last valid AAS location for the AI
	SetAASLocation();
	
	if( spectating )
	{
		newEyeOffset = 0.0f;
	}
	else if( health <= 0 )
	{
		newEyeOffset = pm_deadviewheight.GetFloat();
	}
	else if( physicsObj.IsCrouching() )
	{
		newEyeOffset = pm_crouchviewheight.GetFloat();
	}
	else if( GetBindMaster() && GetBindMaster()->IsType( idAFEntity_Vehicle::Type ) )
	{
		newEyeOffset = 0.0f;
	}
	else
	{
		newEyeOffset = pm_normalviewheight.GetFloat();
	}
	
	if( EyeHeight() != newEyeOffset )
	{
		if( spectating )
		{
			SetEyeHeight( newEyeOffset );
		}
		else
		{
			// smooth out duck height changes
			SetEyeHeight( EyeHeight() * pm_crouchrate.GetFloat() + newEyeOffset * ( 1.0f - pm_crouchrate.GetFloat() ) );
		}
	}
	
	if( AI_JUMP )
	{
		// bounce the view weapon
		loggedAccel_t*	acc = &loggedAccel[currentLoggedAccel & ( NUM_LOGGED_ACCELS - 1 )];
		currentLoggedAccel++;
		acc->time = gameLocal.time;
		acc->dir[2] = 200;
		acc->dir[0] = acc->dir[1] = 0;
	}
	
	if( AI_ONLADDER )
	{
		int old_rung = oldOrigin.z / LADDER_RUNG_DISTANCE;
		int new_rung = physicsObj.GetOrigin().z / LADDER_RUNG_DISTANCE;
		
		if( old_rung != new_rung )
		{
			StartSound( "snd_stepladder", SND_CHANNEL_ANY, 0, false, NULL );
		}
	}
	
	BobCycle( pushVelocity );
	CrashLand( oldOrigin, oldVelocity );
	
}

/*
==============
idPlayer::Move
==============
*/
void idPlayer::Move()
{
	float newEyeOffset;
	idVec3 oldOrigin;
	idVec3 oldVelocity;
	idVec3 pushVelocity;
	
	// save old origin and velocity for crashlanding
	oldOrigin = physicsObj.GetOrigin();
	oldVelocity = physicsObj.GetLinearVelocity();
	pushVelocity = physicsObj.GetPushedLinearVelocity();
	
	// set physics variables
	physicsObj.SetMaxStepHeight( pm_stepsize.GetFloat() );
	physicsObj.SetMaxJumpHeight( pm_jumpheight.GetFloat() );
	
	if( noclip )
	{
		physicsObj.SetContents( 0 );
		physicsObj.SetMovementType( PM_NOCLIP );
	}
	else if( spectating )
	{
		physicsObj.SetContents( 0 );
		physicsObj.SetMovementType( PM_SPECTATOR );
	}
	else if( health <= 0 )
	{
		physicsObj.SetContents( CONTENTS_CORPSE | CONTENTS_MONSTERCLIP );
		physicsObj.SetMovementType( PM_DEAD );
	}
	else if( gameLocal.inCinematic || gameLocal.GetCamera() || privateCameraView || ( influenceActive == INFLUENCE_LEVEL2 ) )
	{
		physicsObj.SetContents( CONTENTS_BODY );
		physicsObj.SetMovementType( PM_FREEZE );
	}
	else if( mountedObject )
	{
		physicsObj.SetContents( 0 );
		physicsObj.SetMovementType( PM_FREEZE );
	}
	else
	{
		physicsObj.SetContents( CONTENTS_BODY );
		physicsObj.SetMovementType( PM_NORMAL );
	}
	
	if( spectating )
	{
		physicsObj.SetClipMask( MASK_DEADSOLID );
	}
	else if( health <= 0 )
	{
		physicsObj.SetClipMask( MASK_DEADSOLID );
	}
	else
	{
		physicsObj.SetClipMask( MASK_PLAYERSOLID );
	}
	
	physicsObj.SetDebugLevel( g_debugMove.GetBool() );
	
	{
		idVec3	org;
		idMat3	axis;
		GetViewPos( org, axis );
		
		physicsObj.SetPlayerInput( usercmd, axis[0] );
	}
	
	// FIXME: physics gets disabled somehow
	BecomeActive( TH_PHYSICS );
	RunPhysics();
	
	// update our last valid AAS location for the AI
	SetAASLocation();
	
	if( spectating )
	{
		newEyeOffset = 0.0f;
	}
	else if( health <= 0 )
	{
		newEyeOffset = pm_deadviewheight.GetFloat();
	}
	else if( physicsObj.IsCrouching() )
	{
		newEyeOffset = pm_crouchviewheight.GetFloat();
	}
	else if( GetBindMaster() && GetBindMaster()->IsType( idAFEntity_Vehicle::Type ) )
	{
		newEyeOffset = 0.0f;
	}
	else
	{
		newEyeOffset = pm_normalviewheight.GetFloat();
	}
	
	if( EyeHeight() != newEyeOffset )
	{
		if( spectating )
		{
			SetEyeHeight( newEyeOffset );
		}
		else
		{
			// smooth out duck height changes
			SetEyeHeight( EyeHeight() * pm_crouchrate.GetFloat() + newEyeOffset * ( 1.0f - pm_crouchrate.GetFloat() ) );
		}
	}
	
	if( noclip || gameLocal.inCinematic || ( influenceActive == INFLUENCE_LEVEL2 ) )
	{
		AI_CROUCH	= false;
		AI_ONGROUND	= ( influenceActive == INFLUENCE_LEVEL2 );
		AI_ONLADDER	= false;
		AI_JUMP		= false;
	}
	else
	{
		AI_CROUCH	= physicsObj.IsCrouching();
		AI_ONGROUND	= physicsObj.HasGroundContacts();
		AI_ONLADDER	= physicsObj.OnLadder();
		AI_JUMP		= physicsObj.HasJumped();
		
		// check if we're standing on top of a monster and give a push if we are
		idEntity* groundEnt = physicsObj.GetGroundEntity();
		if( groundEnt != NULL && groundEnt->IsType( idAI::Type ) )
		{
			idVec3 vel = physicsObj.GetLinearVelocity();
			if( vel.ToVec2().LengthSqr() < 0.1f )
			{
				vel.ToVec2() = physicsObj.GetOrigin().ToVec2() - groundEnt->GetPhysics()->GetAbsBounds().GetCenter().ToVec2();
				vel.ToVec2().NormalizeFast();
				vel.ToVec2() *= pm_walkspeed.GetFloat();
			}
			else
			{
				// give em a push in the direction they're going
				vel *= 1.1f;
			}
			physicsObj.SetLinearVelocity( vel );
		}
	}
	
	if( AI_JUMP )
	{
		// bounce the view weapon
		loggedAccel_t*	acc = &loggedAccel[currentLoggedAccel & ( NUM_LOGGED_ACCELS - 1 )];
		currentLoggedAccel++;
		acc->time = gameLocal.time;
		acc->dir[2] = 200;
		acc->dir[0] = acc->dir[1] = 0;
	}
	
	if( AI_ONLADDER )
	{
		int old_rung = oldOrigin.z / LADDER_RUNG_DISTANCE;
		int new_rung = physicsObj.GetOrigin().z / LADDER_RUNG_DISTANCE;
		
		if( old_rung != new_rung )
		{
			StartSound( "snd_stepladder", SND_CHANNEL_ANY, 0, false, NULL );
		}
	}
	
	BobCycle( pushVelocity );
	CrashLand( oldOrigin, oldVelocity );
}

/*
========================
idPlayer::AllowClientAuthPhysics
========================
*/
bool idPlayer::AllowClientAuthPhysics()
{
	// note respawn count > 1: respawn should be called twice - once for initial spawn and once for actual respawn by game mode
	// TODO: I don't think doom 3 will need to care about the respawn count.
	return ( usercmd.serverGameMilliseconds > serverOverridePositionTime && commonLocal.GetUCmdMgr().HasUserCmdForPlayer( entityNumber ) );
}

/*
========================
idPlayer::RunPhysics_RemoteClientCorrection
========================
*/
void idPlayer::RunPhysics_RemoteClientCorrection()
{

	if( !AllowClientAuthPhysics() )
	{
		// We are still overriding client's position
		if( pm_clientAuthoritative_debug.GetBool() )
		{
			//clientGame->renderWorld->DebugPoint( idColor::colorRed, GetOrigin() );
			idLib::Printf( "[%d]Ignoring client auth:  cmd.serverTime: %d  overrideTime: %d \n", entityNumber,  usercmd.serverGameMilliseconds, serverOverridePositionTime );
		}
		return;
	}
	
	
	
	// Client is on a pusher... ignore him so he doesn't lag behind
	bool becameUnlocked = false;
	if( physicsObj.ClientPusherLocked( becameUnlocked ) )
	{
	
		// Check and see how far we've diverged.
		idVec3 cmdPos( usercmd.pos[0], usercmd.pos[1], usercmd.pos[2] );
		idVec3 newOrigin = physicsObj.GetOrigin();
		
		idVec3 divergeVec = cmdPos - newOrigin;
		idLib::Printf( "Client Divergence: %s Length: %2f\n", divergeVec.ToString( 3 ), divergeVec.Length() );
		
		// if the client Diverges over a certain amount, snap him back
		if( divergeVec.Length() < pm_clientAuthoritative_Divergence.GetFloat() )
		{
			return;
		}
		
	}
	if( becameUnlocked )
	{
		// Client just got off of a mover, wait before listening to him
		serverOverridePositionTime = gameLocal.GetServerGameTimeMs();
		return;
	}
	
	
	// Correction
	{
		idVec3 newOrigin = physicsObj.GetOrigin();
		idVec3 cmdPos( usercmd.pos[0], usercmd.pos[1], usercmd.pos[2] );
		idVec3 desiredPos = cmdPos;
		
		float delta = ( desiredPos - newOrigin ).Length();
		// ignore small differences in Z: this can cause player to not have proper ground contacts which messes up
		// velocity/acceleration calculation. If this hack doesn't work out, will may need more precision for at least
		// the Z component of the client's origin.
		if( idMath::Fabs( desiredPos.z - newOrigin.z ) < pm_clientAuthoritative_minDistZ.GetFloat() )
		{
			if( pm_clientAuthoritative_debug.GetBool() )
			{
				//idLib::Printf("[%d]Remote client physics: ignore small z delta: %f\n", usercmd.clientGameFrame, ( desiredPos.z - newOrigin.z ) );
			}
			desiredPos.z = newOrigin.z;
		}
		
		// Origin
		if( delta > pm_clientAuthoritative_minDist.GetFloat() )
		{
		
			if( pm_clientAuthoritative_Lerp.GetFloat() > 0.0f )
			{
				desiredPos.x = idMath::LerpToWithScale( newOrigin.x, desiredPos.x, pm_clientAuthoritative_Lerp.GetFloat() );
				desiredPos.y = idMath::LerpToWithScale( newOrigin.y, desiredPos.y, pm_clientAuthoritative_Lerp.GetFloat() );
			}
			
			// Set corrected position immediately if non deferred
			physicsObj.SetOrigin( desiredPos );
			
			if( pm_clientAuthoritative_debug.GetBool() && delta > pm_clientAuthoritative_warnDist.GetFloat() )
			{
				idLib::Warning( "Remote client player physics: delta movement for frame was %f units", delta );
				gameRenderWorld->DebugLine( colorRed, newOrigin, desiredPos );
			}
		}
		if( pm_clientAuthoritative_debug.GetBool() )
		{
			//idLib::Printf( "[%d]Remote client player physics delta: %.2f. forward: %d pos <%.2f, %.2f, %.2f> \n", usercmd.clientGameFrame, delta, (int)usercmd.forwardmove, desiredPos.x, desiredPos.y, desiredPos.z );
			gameRenderWorld->DebugLine( colorRed, newOrigin, desiredPos );
			//gameRenderWorld->DebugPoint( colorBlue, cmdPos );
		}
		
		// Set velocity if significantly different than client.
		const float serverSpeedSquared = physicsObj.GetLinearVelocity().LengthSqr();
		const float clientSpeedSquared = usercmd.speedSquared;
		
		if( fabsf( serverSpeedSquared - clientSpeedSquared ) > pm_clientAuthoritative_minSpeedSquared.GetFloat() )
		{
			idVec3 normalizedVelocity = physicsObj.GetLinearVelocity();
			
			const float VELOCITY_EPSILON = 0.001f;
			if( normalizedVelocity.LengthSqr() > VELOCITY_EPSILON )
			{
				normalizedVelocity.Normalize();
			}
			
			physicsObj.SetLinearVelocity( normalizedVelocity * idMath::Sqrt( clientSpeedSquared ) );
		}
	}
}

/*
========================
idPlayer::GetPhysicsTimeStep

Uses the time from the usercmd in case the server is running at a slower engineHz
than the client.
========================
*/
int idPlayer::GetPhysicsTimeStep() const
{
	// if the ucDeltaMillisecond value looks wrong, use the game delta milliseconds
	// This can happen if the user brings up the pause menu in SP
	const int ucDeltaMilliseconds = usercmd.clientGameMilliseconds - oldCmd.clientGameMilliseconds;
	if( ucDeltaMilliseconds < 1 || ucDeltaMilliseconds > 20 )
	{
		return gameLocal.time - gameLocal.previousTime;
	}
	else
	{
		return ucDeltaMilliseconds;
	}
}

/*
==============
idPlayer::ShowRespawnHudMessage

Called once when the minimum respawn time has passed after a player has died
so that we can display a message to the user.
==============
*/
void idPlayer::ShowRespawnHudMessage()
{
	if( IsLocallyControlled() )
	{
		hud->ShowRespawnMessage( true );
	}
	else
	{
		// Clients show the hud message through a reliable message.
		idBitMsg	outMsg;
		byte		dummyData[1];
		outMsg.InitWrite( dummyData, sizeof( dummyData ) );
		outMsg.BeginWriting();
		outMsg.WriteByte( 0 );
		session->GetActingGameStateLobbyBase().SendReliableToLobbyUser( gameLocal.lobbyUserIDs[entityNumber], GAME_RELIABLE_MESSAGE_RESPAWN_AVAILABLE, outMsg );
	}
}

/*
==============
idPlayer::HideRespawnHudMessage

Called once when we should remove the respawn message from the hud,
for example, when a player does respawn.
==============
*/
void idPlayer::HideRespawnHudMessage()
{
	if( IsLocallyControlled() )
	{
		hud->ShowRespawnMessage( false );
	}
}

/*
==============
idPlayer::UpdateHud
==============
*/
void idPlayer::UpdateHud()
{
	idPlayer* aimed;
	
	if( !hud )
	{
		return;
	}
	
	if( !IsLocallyControlled() )
	{
		return;
	}
	
	int c = inventory.pickupItemNames.Num();
	if( c > 0 )
	{
		if( hud != NULL && hud->IsPickupListReady() )
		{
			if( inventory.nextItemPickup && gameLocal.time - inventory.nextItemPickup > 2000 )
			{
				inventory.nextItemNum = 1;
			}
			int i;
			
			int count = 5;
			bool showNewPickups = false;
			for( i = 0; i < count; i++ )    //_D3XP
			{
				if( i < c )
				{
					hud->UpdatePickupInfo( i, inventory.pickupItemNames[0] );
					inventory.nextItemNum++;
					showNewPickups = true;
				}
				else
				{
					hud->UpdatePickupInfo( i, "" );
					continue;
				}
				
				inventory.nextItemPickup = gameLocal.time + 2500;
				inventory.pickupItemNames.RemoveIndex( 0 );
			}
			
			if( showNewPickups )
			{
				hud->ShowPickups();
			}
		}
	}
	
	if( gameLocal.realClientTime == lastMPAimTime )
	{
		if( MPAim != -1 && gameLocal.mpGame.IsGametypeTeamBased()  /* CTF */
				&& gameLocal.entities[ MPAim ] && gameLocal.entities[ MPAim ]->IsType( idPlayer::Type )
				&& static_cast< idPlayer* >( gameLocal.entities[ MPAim ] )->team == team )
		{
			aimed = static_cast< idPlayer* >( gameLocal.entities[ MPAim ] );
			
			hud->TriggerHitTarget( true, session->GetActingGameStateLobbyBase().GetLobbyUserName( gameLocal.lobbyUserIDs[ MPAim ] ), aimed->team + 1 );
			MPAimHighlight = true;
			MPAimFadeTime = 0;	// no fade till loosing focus
		}
		else if( MPAimHighlight )
		{
			hud->TriggerHitTarget( false, "" );
			MPAimFadeTime = gameLocal.realClientTime;
			MPAimHighlight = false;
		}
	}
	if( MPAimFadeTime )
	{
		assert( !MPAimHighlight );
		if( gameLocal.realClientTime - MPAimFadeTime > 2000 )
		{
			MPAimFadeTime = 0;
		}
	}
	
	if( common->IsMultiplayer() && IsLocallyControlled() )
	{
	
		hud->ToggleLagged( isLagged );
		
		// TODO_SPARTY: what is this projectile stuff for
		//hud->SetStateInt( "g_showProjectilePct", g_showProjectilePct.GetInteger() );
		//if ( numProjectilesFired ) {
		//	hud->SetStateString( "projectilepct", va( "Hit %% %.1f", ( (float) numProjectileHits / numProjectilesFired ) * 100 ) );
		//} else {
		//	hud->SetStateString( "projectilepct", "Hit % 0.0" );
		//}
		
	}
}

/*
==============
idPlayer::UpdateDeathSkin
==============
*/
void idPlayer::UpdateDeathSkin( bool state_hitch )
{
	if( !( common->IsMultiplayer() || g_testDeath.GetBool() ) )
	{
		return;
	}
	if( health <= 0 )
	{
		if( !doingDeathSkin )
		{
			deathClearContentsTime = spawnArgs.GetInt( "deathSkinTime" );
			doingDeathSkin = true;
			renderEntity.noShadow = true;
			if( state_hitch )
			{
				renderEntity.shaderParms[ SHADERPARM_TIME_OF_DEATH ] = gameLocal.time * 0.001f - 2.0f;
			}
			else
			{
				renderEntity.shaderParms[ SHADERPARM_TIME_OF_DEATH ] = gameLocal.time * 0.001f;
			}
			UpdateVisuals();
		}
		
		// wait a bit before switching off the content
		if( deathClearContentsTime && gameLocal.time > deathClearContentsTime )
		{
			SetCombatContents( false );
			deathClearContentsTime = 0;
		}
	}
	else
	{
		renderEntity.noShadow = false;
		renderEntity.shaderParms[ SHADERPARM_TIME_OF_DEATH ] = 0.0f;
		UpdateVisuals();
		doingDeathSkin = false;
	}
}

/*
==============
idPlayer::StartFxOnBone
==============
*/
void idPlayer::StartFxOnBone( const char* fx, const char* bone )
{
	idVec3 offset;
	idMat3 axis;
	jointHandle_t jointHandle = GetAnimator()->GetJointHandle( bone );
	
	if( jointHandle == INVALID_JOINT )
	{
		gameLocal.Printf( "Cannot find bone %s\n", bone );
		return;
	}
	
	if( GetAnimator()->GetJointTransform( jointHandle, gameLocal.time, offset, axis ) )
	{
		offset = GetPhysics()->GetOrigin() + offset * GetPhysics()->GetAxis();
		axis = axis * GetPhysics()->GetAxis();
	}
	
	idEntityFx::StartFx( fx, &offset, &axis, this, true );
}

/*
==============
idPlayer::HandleGuiEvents
==============
*/
bool idPlayer::HandleGuiEvents( const sysEvent_t* ev )
{

	bool handled = false;
	
	if( hudManager != NULL && hudManager->IsActive() )
	{
		handled = hudManager->HandleGuiEvent( ev );
	}
	
	if( pdaMenu != NULL && pdaMenu->IsActive() )
	{
		handled = pdaMenu->HandleGuiEvent( ev );
	}
	
	return handled;
}

/*
==============
idPlayer::UpdateLaserSight
==============
*/
idCVar	g_laserSightWidth( "g_laserSightWidth", "2.0", CVAR_FLOAT | CVAR_ARCHIVE, "laser sight beam width" );
idCVar	g_laserSightLength( "g_laserSightLength", "250", CVAR_FLOAT | CVAR_ARCHIVE, "laser sight beam length" );

void idPlayer::UpdateLaserSight()
{
	idVec3	muzzleOrigin;
	idMat3	muzzleAxis;
	
	// In Multiplayer, weapon might not have been spawned yet.
	if( weapon.GetEntity() ==  NULL )
	{
		return;
	}
	
	if( !IsGameStereoRendered() ||
			!weapon.GetEntity()->ShowCrosshair() ||
			AI_DEAD ||
			weapon->IsHidden() ||
			!weapon->GetMuzzlePositionWithHacks( muzzleOrigin, muzzleAxis ) )
	{
		// hide it
		laserSightRenderEntity.allowSurfaceInViewID = -1;
		if( laserSightHandle == -1 )
		{
			laserSightHandle = gameRenderWorld->AddEntityDef( &laserSightRenderEntity );
		}
		else
		{
			gameRenderWorld->UpdateEntityDef( laserSightHandle, &laserSightRenderEntity );
		}
		return;
	}
	
	// program the beam model
	
	// only show in the player's view
	laserSightRenderEntity.allowSurfaceInViewID = entityNumber + 1;
	laserSightRenderEntity.axis.Identity();
	
	laserSightRenderEntity.origin = muzzleOrigin - muzzleAxis[0] * 2.0f;
	idVec3&	target = *reinterpret_cast<idVec3*>( &laserSightRenderEntity.shaderParms[SHADERPARM_BEAM_END_X] );
	target = muzzleOrigin + muzzleAxis[0] * g_laserSightLength.GetFloat();
	
	laserSightRenderEntity.shaderParms[SHADERPARM_BEAM_WIDTH] = g_laserSightWidth.GetFloat();
	
	if( IsGameStereoRendered() && laserSightHandle == -1 )
	{
		laserSightHandle = gameRenderWorld->AddEntityDef( &laserSightRenderEntity );
	}
	else
	{
		gameRenderWorld->UpdateEntityDef( laserSightHandle, &laserSightRenderEntity );
	}
}

/*
==============
idPlayer::Think

Called every tic for each player
==============
*/
void idPlayer::Think()
{
	playedTimeResidual += ( gameLocal.time - gameLocal.previousTime );
	playedTimeSecs += playedTimeResidual / 1000;
	playedTimeResidual = playedTimeResidual % 1000;
	
	aimAssist.Update();
	
	UpdatePlayerIcons();
	
	UpdateSkinSetup();
	
	buttonMask &= usercmd.buttons;
	usercmd.buttons &= ~buttonMask;
	
	// clear the ik before we do anything else so the skeleton doesn't get updated twice
	walkIK.ClearJointMods();
	
	// if this is the very first frame of the map, set the delta view angles
	// based on the usercmd angles
	if( !spawnAnglesSet && ( gameLocal.GameState() != GAMESTATE_STARTUP ) )
	{
		spawnAnglesSet = true;
		SetViewAngles( spawnAngles );
		oldImpulseSequence = usercmd.impulseSequence;
	}
	
	if( mountedObject )
	{
		usercmd.forwardmove = 0;
		usercmd.rightmove = 0;
		usercmd.buttons &= ~( BUTTON_JUMP | BUTTON_CROUCH );
	}
	
	if( objectiveSystemOpen || gameLocal.inCinematic || influenceActive )
	{
		if( objectiveSystemOpen && AI_PAIN )
		{
			TogglePDA();
		}
		usercmd.forwardmove = 0;
		usercmd.rightmove = 0;
		usercmd.buttons &= ~( BUTTON_JUMP | BUTTON_CROUCH );
	}
	
	// log movement changes for weapon bobbing effects
	if( usercmd.forwardmove != oldCmd.forwardmove )
	{
		loggedAccel_t*	acc = &loggedAccel[currentLoggedAccel & ( NUM_LOGGED_ACCELS - 1 )];
		currentLoggedAccel++;
		acc->time = gameLocal.time;
		acc->dir[0] = usercmd.forwardmove - oldCmd.forwardmove;
		acc->dir[1] = acc->dir[2] = 0;
	}
	
	if( usercmd.rightmove != oldCmd.rightmove )
	{
		loggedAccel_t*	acc = &loggedAccel[currentLoggedAccel & ( NUM_LOGGED_ACCELS - 1 )];
		currentLoggedAccel++;
		acc->time = gameLocal.time;
		acc->dir[1] = usercmd.rightmove - oldCmd.rightmove;
		acc->dir[0] = acc->dir[2] = 0;
	}
	
	// zooming
	if( ( usercmd.buttons ^ oldCmd.buttons ) & BUTTON_ZOOM )
	{
		if( ( usercmd.buttons & BUTTON_ZOOM ) && weapon.GetEntity() )
		{
			zoomFov.Init( gameLocal.time, 200.0f, CalcFov( false ), weapon.GetEntity()->GetZoomFov() );
		}
		else
		{
			zoomFov.Init( gameLocal.time, 200.0f, zoomFov.GetCurrentValue( gameLocal.time ), DefaultFov() );
		}
	}
	
	// if we have an active gui, we will unrotate the view angles as
	// we turn the mouse movements into gui events
	idUserInterface* gui = ActiveGui();
	if( gui && gui != focusUI )
	{
		RouteGuiMouse( gui );
	}
	
	// set the push velocity on the weapon before running the physics
	if( weapon.GetEntity() )
	{
		weapon.GetEntity()->SetPushVelocity( physicsObj.GetPushedLinearVelocity() );
	}
	
	EvaluateControls();
	
	if( !af.IsActive() )
	{
		AdjustBodyAngles();
		CopyJointsFromBodyToHead();
	}
	
	if( IsLocallyControlled() )
	{
		// Local player on the server. Do normal movement.
		Move();
	}
	else
	{
		// Server is processing a client. Run client's commands like normal...
		Move();
		
		// ...then correct if needed.
		RunPhysics_RemoteClientCorrection();
	}
	
	if( !g_stopTime.GetBool() )
	{
	
		if( !noclip && !spectating && ( health > 0 ) && !IsHidden() )
		{
			TouchTriggers();
		}
		
		// not done on clients for various reasons. don't do it on server and save the sound channel for other things
		if( !common->IsMultiplayer() )
		{
			SetCurrentHeartRate();
			float scale = new_g_damageScale;
			if( g_useDynamicProtection.GetBool() && scale < 1.0f && gameLocal.time - lastDmgTime > 500 )
			{
				if( scale < 1.0f )
				{
					scale += 0.05f;
				}
				if( scale > 1.0f )
				{
					scale = 1.0f;
				}
				new_g_damageScale = scale;
			}
		}
		
		// update GUIs, Items, and character interactions
		UpdateFocus();
		
		UpdateLocation();
		
		// update player script
		UpdateScript();
		
		// service animations
		if( !spectating && !af.IsActive() && !gameLocal.inCinematic )
		{
			UpdateConditions();
			UpdateAnimState();
			CheckBlink();
		}
		
		// clear out our pain flag so we can tell if we recieve any damage between now and the next time we think
		AI_PAIN = false;
	}
	
	// calculate the exact bobbed view position, which is used to
	// position the view weapon, among other things
	CalculateFirstPersonView();
	
	// this may use firstPersonView, or a thirdPeroson / camera view
	CalculateRenderView();
	
	inventory.UpdateArmor();
	
	if( spectating )
	{
		UpdateSpectating();
	}
	else if( health > 0 )
	{
		UpdateWeapon();
	}
	
	UpdateFlashlight();
	
	UpdateAir();
	
	UpdatePowerupHud();
	
	UpdateHud();
	
	UpdatePowerUps();
	
	UpdateDeathSkin( false );
	
	if( common->IsMultiplayer() )
	{
		DrawPlayerIcons();
		
		if( enviroSuitLight.IsValid() )
		{
			idAngles lightAng = firstPersonViewAxis.ToAngles();
			idVec3 lightOrg = firstPersonViewOrigin;
			const idDict* lightDef = gameLocal.FindEntityDefDict( "envirosuit_light", false );
			
			idVec3 enviroOffset = lightDef->GetVector( "enviro_offset" );
			idVec3 enviroAngleOffset = lightDef->GetVector( "enviro_angle_offset" );
			
			lightOrg += ( enviroOffset.x * firstPersonViewAxis[0] );
			lightOrg += ( enviroOffset.y * firstPersonViewAxis[1] );
			lightOrg += ( enviroOffset.z * firstPersonViewAxis[2] );
			lightAng.pitch += enviroAngleOffset.x;
			lightAng.yaw += enviroAngleOffset.y;
			lightAng.roll += enviroAngleOffset.z;
			
			enviroSuitLight.GetEntity()->GetPhysics()->SetOrigin( lightOrg );
			enviroSuitLight.GetEntity()->GetPhysics()->SetAxis( lightAng.ToMat3() );
			enviroSuitLight.GetEntity()->UpdateVisuals();
			enviroSuitLight.GetEntity()->Present();
		}
	}
	
	renderEntity_t* headRenderEnt = NULL;
	if( head.GetEntity() )
	{
		headRenderEnt = head.GetEntity()->GetRenderEntity();
	}
	if( headRenderEnt )
	{
		if( influenceSkin )
		{
			headRenderEnt->customSkin = influenceSkin;
		}
		else
		{
			headRenderEnt->customSkin = NULL;
		}
	}
	
	if( common->IsMultiplayer() || g_showPlayerShadow.GetBool() )
	{
		renderEntity.suppressShadowInViewID	= 0;
		if( headRenderEnt )
		{
			headRenderEnt->suppressShadowInViewID = 0;
		}
	}
	else
	{
		renderEntity.suppressShadowInViewID	= entityNumber + 1;
		if( headRenderEnt )
		{
			headRenderEnt->suppressShadowInViewID = entityNumber + 1;
		}
	}
	// never cast shadows from our first-person muzzle flashes
	renderEntity.suppressShadowInLightID = LIGHTID_VIEW_MUZZLE_FLASH + entityNumber;
	if( headRenderEnt )
	{
		headRenderEnt->suppressShadowInLightID = LIGHTID_VIEW_MUZZLE_FLASH + entityNumber;
	}
	
	if( !g_stopTime.GetBool() )
	{
		UpdateAnimation();
		
		Present();
		
		UpdateDamageEffects();
		
		LinkCombat();
		
		playerView.CalculateShake();
	}
	
	if( !( thinkFlags & TH_THINK ) )
	{
		gameLocal.Printf( "player %d not thinking?\n", entityNumber );
	}
	
	if( g_showEnemies.GetBool() )
	{
		idActor* ent;
		int num = 0;
		for( ent = enemyList.Next(); ent != NULL; ent = ent->enemyNode.Next() )
		{
			gameLocal.Printf( "enemy (%d)'%s'\n", ent->entityNumber, ent->name.c_str() );
			gameRenderWorld->DebugBounds( colorRed, ent->GetPhysics()->GetBounds().Expand( 2 ), ent->GetPhysics()->GetOrigin() );
			num++;
		}
		gameLocal.Printf( "%d: enemies\n", num );
	}
	
	inventory.RechargeAmmo( this );
	
	if( healthRecharge )
	{
		int elapsed = gameLocal.time - lastHealthRechargeTime;
		if( elapsed >= rechargeSpeed )
		{
			int intervals = ( gameLocal.time - lastHealthRechargeTime ) / rechargeSpeed;
			Give( "health", va( "%d", intervals ), ITEM_GIVE_FEEDBACK | ITEM_GIVE_UPDATE_STATE );
			lastHealthRechargeTime += intervals * rechargeSpeed;
		}
	}
	
	// determine if portal sky is in pvs
	gameLocal.portalSkyActive = gameLocal.pvs.CheckAreasForPortalSky( gameLocal.GetPlayerPVS(), GetPhysics()->GetOrigin() );
	
	// stereo rendering laser sight that replaces the crosshair
	UpdateLaserSight();
	
	// Show the respawn hud message if necessary.
	if( common->IsMultiplayer() && ( minRespawnTime != maxRespawnTime ) )
	{
		if( gameLocal.previousTime < minRespawnTime && minRespawnTime <= gameLocal.time )
		{
			// Server will show the hud message directly.
			ShowRespawnHudMessage();
		}
	}
	
	// Make sure voice groups are set to the right team
	if( common->IsMultiplayer() && session->GetState() >= idSession::INGAME && entityNumber < MAX_CLIENTS )  		// The entityNumber < MAX_CLIENTS seems to quiet the static analyzer
	{
		// Make sure we're on the right team (at the lobby level)
		const int voiceTeam = spectating ? LOBBY_SPECTATE_TEAM_FOR_VOICE_CHAT : team;
		
		//idLib::Printf( "SERVER: Sending voice %i / %i\n", entityNumber, voiceTeam );
		
		// Update lobby team
		session->GetActingGameStateLobbyBase().SetLobbyUserTeam( gameLocal.lobbyUserIDs[ entityNumber ], voiceTeam );
		
		// Update voice groups to match in case something changed
		session->SetVoiceGroupsToTeams();
	}
}

/*
=================
idPlayer::StartHealthRecharge
=================
*/
void idPlayer::StartHealthRecharge( int speed )
{
	lastHealthRechargeTime = gameLocal.time;
	healthRecharge = true;
	rechargeSpeed = speed;
}

/*
=================
idPlayer::StopHealthRecharge
=================
*/
void idPlayer::StopHealthRecharge()
{
	healthRecharge = false;
}

/*
=================
idPlayer::GetCurrentWeapon
=================
*/
idStr idPlayer::GetCurrentWeapon()
{
	const char* weapon;
	
	if( currentWeapon >= 0 )
	{
		weapon = spawnArgs.GetString( va( "def_weapon%d", currentWeapon ) );
		return weapon;
	}
	else
	{
		return "";
	}
}

/*
=================
idPlayer::CanGive
=================
*/
bool idPlayer::CanGive( const char* statname, const char* value )
{
	if( AI_DEAD )
	{
		return false;
	}
	
	if( !idStr::Icmp( statname, "health" ) )
	{
		if( health >= inventory.maxHealth )
		{
			return false;
		}
		return true;
	}
	else if( !idStr::Icmp( statname, "stamina" ) )
	{
		if( stamina >= 100 )
		{
			return false;
		}
		return true;
		
	}
	else if( !idStr::Icmp( statname, "heartRate" ) )
	{
		return true;
		
	}
	else if( !idStr::Icmp( statname, "air" ) )
	{
		if( airMsec >= pm_airMsec.GetInteger() )
		{
			return false;
		}
		return true;
	}
	else
	{
		return inventory.CanGive( this, spawnArgs, statname, value );
	}
}

/*
=================
idPlayer::StopHelltime

provides a quick non-ramping way of stopping helltime
=================
*/
void idPlayer::StopHelltime( bool quick )
{
	if( !PowerUpActive( HELLTIME ) )
	{
		return;
	}
	
	// take away the powerups
	if( PowerUpActive( INVULNERABILITY ) )
	{
		ClearPowerup( INVULNERABILITY );
	}
	
	if( PowerUpActive( BERSERK ) )
	{
		ClearPowerup( BERSERK );
	}
	
	if( PowerUpActive( HELLTIME ) )
	{
		ClearPowerup( HELLTIME );
	}
	
	// stop the looping sound
	StopSound( SND_CHANNEL_DEMONIC, false );
	
	// reset the game vars
	if( quick )
	{
		gameLocal.QuickSlowmoReset();
	}
}

/*
=================
idPlayer::Event_ToggleBloom
=================
*/
void idPlayer::Event_ToggleBloom( int on )
{
	if( on )
	{
		bloomEnabled = true;
	}
	else
	{
		bloomEnabled = false;
	}
}

/*
=================
idPlayer::Event_SetBloomParms
=================
*/
void idPlayer::Event_SetBloomParms( float speed, float intensity )
{
	bloomSpeed = speed;
	bloomIntensity = intensity;
}

/*
=================
idPlayer::PlayHelltimeStopSound
=================
*/
void idPlayer::PlayHelltimeStopSound()
{
	const char* sound;
	
	if( spawnArgs.GetString( "snd_helltime_stop", "", &sound ) )
	{
		PostEventMS( &EV_StartSoundShader, 0, sound, SND_CHANNEL_ANY );
	}
}

/*
=================
idPlayer::RouteGuiMouse
=================
*/
void idPlayer::RouteGuiMouse( idUserInterface* gui )
{
	sysEvent_t ev;
	const char* command;
	
	if( usercmd.mx != oldMouseX || usercmd.my != oldMouseY )
	{
		ev = sys->GenerateMouseMoveEvent( usercmd.mx - oldMouseX, usercmd.my - oldMouseY );
		command = gui->HandleEvent( &ev, gameLocal.time );
		oldMouseX = usercmd.mx;
		oldMouseY = usercmd.my;
	}
}

/*
==================
idPlayer::LookAtKiller
==================
*/
void idPlayer::LookAtKiller( idEntity* inflictor, idEntity* attacker )
{
	idVec3 dir;
	
	if( attacker && attacker != this )
	{
		dir = attacker->GetPhysics()->GetOrigin() - GetPhysics()->GetOrigin();
	}
	else if( inflictor && inflictor != this )
	{
		dir = inflictor->GetPhysics()->GetOrigin() - GetPhysics()->GetOrigin();
	}
	else
	{
		dir = viewAxis[ 0 ];
	}
	
	idAngles ang( 0, dir.ToYaw(), 0 );
	SetViewAngles( ang );
}

/*
==============
idPlayer::Kill
==============
*/
void idPlayer::Kill( bool delayRespawn, bool nodamage )
{
	if( spectating )
	{
		SpectateFreeFly( false );
	}
	else if( health > 0 )
	{
		godmode = false;
		if( nodamage )
		{
			ServerSpectate( true );
			idLib::Printf( "TOURNEY Kill :> Player %d On Deck \n", entityNumber );
			forceRespawn = true;
		}
		else
		{
			Damage( this, this, vec3_origin, "damage_suicide", 1.0f, INVALID_JOINT );
			if( delayRespawn )
			{
				forceRespawn = false;
				int delay = spawnArgs.GetFloat( "respawn_delay" );
				minRespawnTime = gameLocal.time + SEC2MS( delay );
				maxRespawnTime = minRespawnTime + MAX_RESPAWN_TIME;
			}
		}
	}
}

/*
==================
idPlayer::Killed
==================
*/
void idPlayer::Killed( idEntity* inflictor, idEntity* attacker, int damage, const idVec3& dir, int location )
{
	float delay;
	
	assert( !common->IsClient() );
	
	// stop taking knockback once dead
	fl.noknockback = true;
	if( health < -999 )
	{
		health = -999;
	}
	
	if( AI_DEAD )
	{
		AI_PAIN = true;
		return;
	}
	
	heartInfo.Init( 0, 0, 0, BASE_HEARTRATE );
	AdjustHeartRate( DEAD_HEARTRATE, 10.0f, 0.0f, true );
	
	if( !g_testDeath.GetBool() && !common->IsMultiplayer() )
	{
		playerView.Fade( colorBlack, 3000 );
	}
	
	AI_DEAD = true;
	SetAnimState( ANIMCHANNEL_LEGS, "Legs_Death", 4 );
	SetAnimState( ANIMCHANNEL_TORSO, "Torso_Death", 4 );
	SetWaitState( "" );
	
	animator.ClearAllJoints();
	
	if( StartRagdoll() )
	{
		pm_modelView.SetInteger( 0 );
		minRespawnTime = gameLocal.time + RAGDOLL_DEATH_TIME;
		maxRespawnTime = minRespawnTime + MAX_RESPAWN_TIME;
	}
	else
	{
		// don't allow respawn until the death anim is done
		// g_forcerespawn may force spawning at some later time
		delay = spawnArgs.GetFloat( "respawn_delay" );
		minRespawnTime = gameLocal.time + SEC2MS( delay );
		maxRespawnTime = minRespawnTime + MAX_RESPAWN_TIME;
	}
	
	physicsObj.SetMovementType( PM_DEAD );
	StartSound( "snd_death", SND_CHANNEL_VOICE, 0, false, NULL );
	StopSound( SND_CHANNEL_BODY2, false );
	
	fl.takedamage = true;		// can still be gibbed
	
	// get rid of weapon
	weapon.GetEntity()->OwnerDied();
	
	// In multiplayer, get rid of the flashlight, or other players
	// will see it floating after the player is dead.
	if( common->IsMultiplayer() )
	{
		FlashlightOff();
		if( flashlight.GetEntity() )
		{
			flashlight.GetEntity()->OwnerDied();
		}
	}
	
	// drop the weapon as an item
	DropWeapon( true );
	
	// drop the flag if player was carrying it
	if( common->IsMultiplayer() && gameLocal.mpGame.IsGametypeFlagBased() && carryingFlag )
	{
		DropFlag();
	}
	
	if( !g_testDeath.GetBool() )
	{
		LookAtKiller( inflictor, attacker );
	}
	
	if( common->IsMultiplayer() || g_testDeath.GetBool() )
	{
		idPlayer* killer = NULL;
		// no gibbing in MP. Event_Gib will early out in MP
		if( attacker->IsType( idPlayer::Type ) )
		{
			killer = static_cast<idPlayer*>( attacker );
			if( health < -20 || killer->PowerUpActive( BERSERK ) )
			{
				gibDeath = true;
				gibsDir = dir;
				gibsLaunched = false;
			}
		}
		gameLocal.mpGame.PlayerDeath( this, killer, isTelefragged );
	}
	else
	{
		physicsObj.SetContents( CONTENTS_CORPSE | CONTENTS_MONSTERCLIP );
	}
	
	ClearPowerUps();
	
	UpdateVisuals();
}

/*
=====================
idPlayer::GetAIAimTargets

Returns positions for the AI to aim at.
=====================
*/
void idPlayer::GetAIAimTargets( const idVec3& lastSightPos, idVec3& headPos, idVec3& chestPos )
{
	idVec3 offset;
	idMat3 axis;
	idVec3 origin;
	
	origin = lastSightPos - physicsObj.GetOrigin();
	
	GetJointWorldTransform( chestJoint, gameLocal.time, offset, axis );
	headPos = offset + origin;
	
	GetJointWorldTransform( headJoint, gameLocal.time, offset, axis );
	chestPos = offset + origin;
}

/*
================
idPlayer::DamageFeedback

callback function for when another entity received damage from this entity.  damage can be adjusted and returned to the caller.
================
*/
void idPlayer::DamageFeedback( idEntity* victim, idEntity* inflictor, int& damage )
{
	// Since we're predicting projectiles on the client now, we might actually get here
	// (used be an assert for clients).
	if( common->IsClient() )
	{
		return;
	}
	
	damage *= PowerUpModifier( BERSERK );
	if( damage && ( victim != this ) && ( victim->IsType( idActor::Type ) || victim->IsType( idDamagable::Type ) ) )
	{
	
		idPlayer* victimPlayer = NULL;
		
		/* No damage feedback sound for hitting friendlies in CTF */
		if( victim->IsType( idPlayer::Type ) )
		{
			victimPlayer = static_cast<idPlayer*>( victim );
		}
		
		if( gameLocal.mpGame.IsGametypeFlagBased() && victimPlayer && this->team == victimPlayer->team )
		{
			/* Do nothing ... */
		}
		else
		{
			SetLastHitTime( gameLocal.time );
		}
	}
}

/*
=================
idPlayer::CalcDamagePoints

Calculates how many health and armor points will be inflicted, but
doesn't actually do anything with them.  This is used to tell when an attack
would have killed the player, possibly allowing a "saving throw"
=================
*/
void idPlayer::CalcDamagePoints( idEntity* inflictor, idEntity* attacker, const idDict* damageDef,
								 const float damageScale, const int location, int* health, int* armor )
{
	int		damage;
	int		armorSave;
	
	damageDef->GetInt( "damage", "20", damage );
	damage = GetDamageForLocation( damage, location );
	
	idPlayer* player = attacker->IsType( idPlayer::Type ) ? static_cast<idPlayer*>( attacker ) : NULL;
	if( !common->IsMultiplayer() )
	{
		if( inflictor != gameLocal.world )
		{
			switch( g_skill.GetInteger() )
			{
				case 0:
					damage *= 0.50f;
					if( damage < 1 )
					{
						damage = 1;
					}
					break;
				case 2:
					damage *= 1.70f;
					break;
				case 3:
					damage *= 3.5f;
					break;
				default:
					break;
			}
		}
	}
	
	damage *= damageScale;
	
	// always give half damage if hurting self
	if( attacker == this )
	{
		if( common->IsMultiplayer() )
		{
			// only do this in mp so single player plasma and rocket splash is very dangerous in close quarters
			damage *= damageDef->GetFloat( "selfDamageScale", "0.5" );
		}
		else
		{
			damage *= damageDef->GetFloat( "selfDamageScale", "1" );
		}
	}
	
	// check for completely getting out of the damage
	if( !damageDef->GetBool( "noGod" ) )
	{
		// check for godmode
		if( godmode )
		{
			damage = 0;
		}
		//Invulnerability is just like god mode
		if( PowerUpActive( INVULNERABILITY ) )
		{
			damage = 0;
		}
	}
	
	// inform the attacker that they hit someone
	attacker->DamageFeedback( this, inflictor, damage );
	
	// save some from armor
	if( !damageDef->GetBool( "noArmor" ) )
	{
		float armor_protection;
		
		armor_protection = ( common->IsMultiplayer() ) ? g_armorProtectionMP.GetFloat() : g_armorProtection.GetFloat();
		
		armorSave = ceil( damage * armor_protection );
		if( armorSave >= inventory.armor )
		{
			armorSave = inventory.armor;
		}
		
		if( !damage )
		{
			armorSave = 0;
		}
		else if( armorSave >= damage )
		{
			armorSave = damage - 1;
			damage = 1;
		}
		else
		{
			damage -= armorSave;
		}
	}
	else
	{
		armorSave = 0;
	}
	
	// check for team damage
	if( gameLocal.mpGame.IsGametypeTeamBased()  /* CTF */
			&& !gameLocal.serverInfo.GetBool( "si_teamDamage" )
			&& !damageDef->GetBool( "noTeam" )
			&& player
			&& player != this		// you get self damage no matter what
			&& player->team == team )
	{
		damage = 0;
	}
	
	*health = damage;
	*armor = armorSave;
}

/*
============
idPlayer::ControllerShakeFromDamage
============
*/
void idPlayer::ControllerShakeFromDamage( int damage )
{

	// If the player is local. SHAkkkkkkeeee!
	if( common->IsMultiplayer() && IsLocallyControlled() )
	{
	
		int maxMagScale = pm_controllerShake_damageMaxMag.GetFloat();
		int maxDurScale = pm_controllerShake_damageMaxDur.GetFloat();
		
		// determine rumble
		// >= 100 damage - will be 300 Mag
		float highMag = ( Max( damage, 100 ) / 100.0f ) * maxMagScale;
		int highDuration = idMath::Ftoi( ( Max( damage, 100 ) / 100.0f ) * maxDurScale );
		float lowMag = highMag * 0.75f;
		int lowDuration = idMath::Ftoi( highDuration );
		
		SetControllerShake( highMag, highDuration, lowMag, lowDuration );
	}
	
}

/*
============
AdjustDamageAmount

Modifies the previously calculated damage to adjust for more factors.
============
*/
int idPlayer::AdjustDamageAmount( const int inputDamage )
{
	int outputDamage = inputDamage;
	
	if( inputDamage > 0 )
	{
	
		if( !common->IsMultiplayer() )
		{
			float scale = new_g_damageScale;
			if( g_useDynamicProtection.GetBool() && g_skill.GetInteger() < 2 )
			{
				if( gameLocal.time > lastDmgTime + 500 && scale > 0.25f )
				{
					scale -= 0.05f;
					new_g_damageScale = scale;
				}
			}
			
			if( scale > 0.0f )
			{
				outputDamage *= scale;
			}
		}
		
		if( g_demoMode.GetBool() )
		{
			outputDamage /= 2;
		}
		
		if( outputDamage < 1 )
		{
			outputDamage = 1;
		}
	}
	
	return outputDamage;
}


/*
============
ServerDealDamage

Only called on the server and in singleplayer. This is where
the player's health is actually modified, but the visual and
sound effects happen elsewhere so that clients can get instant
feedback and hide lag.
============
*/
void idPlayer::ServerDealDamage( int damage, idEntity& inflictor, idEntity& attacker, const idVec3& dir, const char* damageDefName, const int location )
{
	assert( !common->IsClient() );
	
	const idDeclEntityDef* damageDef = gameLocal.FindEntityDef( damageDefName, false );
	if( !damageDef )
	{
		gameLocal.Warning( "Unknown damageDef '%s'", damageDefName );
		return;
	}
	
	// move the world direction vector to local coordinates
	idVec3		damage_from;
	idVec3		localDamageVector;
	
	damage_from = dir;
	damage_from.Normalize();
	
	viewAxis.ProjectVector( damage_from, localDamageVector );
	
	// add to the damage inflicted on a player this frame
	// the total will be turned into screen blends and view angle kicks
	// at the end of the frame
	if( health > 0 )
	{
		playerView.DamageImpulse( localDamageVector, &damageDef->dict );
	}
	
	// do the damage
	if( damage > 0 )
	{
		GetAchievementManager().SetPlayerTookDamage( true );
		
		int oldHealth = health;
		health -= damage;
		
		if( health <= 0 )
		{
		
			if( health < -999 )
			{
				health = -999;
			}
			
			// HACK - A - LICIOUS - Check to see if we are being damaged by the frag chamber.
			if( oldHealth > 0 && strcmp( gameLocal.GetMapName(), "maps/game/mp/d3dm3.map" ) == 0 && strcmp( damageDefName, "damage_triggerhurt_1000_chamber" ) == 0 )
			{
				idPlayer* fragChamberActivator = gameLocal.playerActivateFragChamber;
				if( fragChamberActivator != NULL )
				{
					fragChamberActivator->GetAchievementManager().EventCompletesAchievement( ACHIEVEMENT_MP_CATCH_ENEMY_IN_ROFC );
				}
				gameLocal.playerActivateFragChamber = NULL;
			}
			
			isTelefragged = damageDef->dict.GetBool( "telefrag" );
			
			lastDmgTime = gameLocal.time;
			Killed( &inflictor, &attacker, damage, dir, location );
		}
		else
		{
			if( !g_testDeath.GetBool() )
			{
				lastDmgTime = gameLocal.time;
			}
		}
	}
	else
	{
		// don't accumulate impulses
		if( af.IsLoaded() )
		{
			// clear impacts
			af.Rest();
			
			// physics is turned off by calling af.Rest()
			BecomeActive( TH_PHYSICS );
		}
	}
}

/*
============
Damage

this		entity that is being damaged
inflictor	entity that is causing the damage
attacker	entity that caused the inflictor to damage targ
	example: this=monster, inflictor=rocket, attacker=player

dir			direction of the attack for knockback in global space

damageDef	an idDict with all the options for damage effects

inflictor, attacker, dir, and point can be NULL for environmental effects
============
*/
void idPlayer::Damage( idEntity* inflictor, idEntity* attacker, const idVec3& dir,
					   const char* damageDefName, const float damageScale, const int location )
{
	idVec3		kick;
	int			damage;
	int			armorSave;
	
	SetTimeState ts( timeGroup );
	
	if( !fl.takedamage || noclip || spectating || gameLocal.inCinematic )
	{
		return;
	}
	
	if( !inflictor )
	{
		inflictor = gameLocal.world;
	}
	if( !attacker )
	{
		attacker = gameLocal.world;
	}
	
	if( attacker->IsType( idAI::Type ) )
	{
		if( PowerUpActive( BERSERK ) )
		{
			return;
		}
		// don't take damage from monsters during influences
		if( influenceActive != 0 )
		{
			return;
		}
	}
	
	const idDeclEntityDef* damageDef = gameLocal.FindEntityDef( damageDefName, false );
	if( !damageDef )
	{
		gameLocal.Warning( "Unknown damageDef '%s'", damageDefName );
		return;
	}
	
	if( damageDef->dict.GetBool( "ignore_player" ) )
	{
		return;
	}
	
	// determine knockback
	int knockback = 0;
	damageDef->dict.GetInt( "knockback", "20", knockback );
	
	if( knockback != 0 && !fl.noknockback )
	{
		float attackerPushScale = 0.0f;
		
		if( attacker == this )
		{
			damageDef->dict.GetFloat( "attackerPushScale", "0", attackerPushScale );
		}
		else
		{
			attackerPushScale = 1.0f;
		}
		
		idVec3 kick = dir;
		kick.Normalize();
		kick *= g_knockback.GetFloat() * knockback * attackerPushScale / 200.0f;
		physicsObj.SetLinearVelocity( physicsObj.GetLinearVelocity() + kick );
		
		// set the timer so that the player can't cancel out the movement immediately
		physicsObj.SetKnockBack( idMath::ClampInt( 50, 200, knockback * 2 ) );
		
		if( common->IsServer() )
		{
			idBitMsg	msg;
			byte		msgBuf[MAX_EVENT_PARAM_SIZE];
			
			msg.InitWrite( msgBuf, sizeof( msgBuf ) );
			msg.WriteFloat( physicsObj.GetLinearVelocity()[0] );
			msg.WriteFloat( physicsObj.GetLinearVelocity()[1] );
			msg.WriteFloat( physicsObj.GetLinearVelocity()[2] );
			msg.WriteByte( idMath::ClampInt( 50, 200, knockback * 2 ) );
			ServerSendEvent( idPlayer::EVENT_KNOCKBACK, &msg, false );
		}
	}
	
	// If this is a locally controlled MP client, don't apply damage effects predictively here.
	// Local clients will see the damage feedback (view kick, etc) when their health changes
	// in a snapshot. This ensures that any feedback the local player sees is in sync with
	// his actual health reported by the server.
	if( common->IsMultiplayer() && common->IsClient() && IsLocallyControlled() )
	{
		return;
	}
	
	CalcDamagePoints( inflictor, attacker, &damageDef->dict, damageScale, location, &damage, &armorSave );
	
	// give feedback on the player view and audibly when armor is helping
	if( armorSave )
	{
		inventory.armor -= armorSave;
		
		if( gameLocal.time > lastArmorPulse + 200 )
		{
			StartSound( "snd_hitArmor", SND_CHANNEL_ITEM, 0, false, NULL );
		}
		lastArmorPulse = gameLocal.time;
	}
	
	if( damageDef->dict.GetBool( "burn" ) )
	{
		StartSound( "snd_burn", SND_CHANNEL_BODY3, 0, false, NULL );
	}
	else if( damageDef->dict.GetBool( "no_air" ) )
	{
		if( !armorSave && health > 0 )
		{
			StartSound( "snd_airGasp", SND_CHANNEL_ITEM, 0, false, NULL );
		}
	}
	
	if( g_debugDamage.GetInteger() )
	{
		gameLocal.Printf( "client:%02d\tdamage type:%s\t\thealth:%03d\tdamage:%03d\tarmor:%03d\n", entityNumber, damageDef->GetName(), health, damage, armorSave );
	}
	
	if( common->IsMultiplayer() && IsLocallyControlled() )
	{
		ControllerShakeFromDamage( damage );
	}
	
	// The client needs to know the final damage amount for predictive pain animations.
	const int finalDamage = AdjustDamageAmount( damage );
	
	if( health > 0 )
	{
		// force a blink
		blink_time = 0;
		
		// let the anim script know we took damage
		AI_PAIN = Pain( inflictor, attacker, damage, dir, location );
	}
	
	// Only actually deal the damage here in singleplayer and for locally controlled servers.
	if( !common->IsMultiplayer() || common->IsServer() )
	{
		// Server will deal his damage normally
		ServerDealDamage( finalDamage, *inflictor, *attacker, dir, damageDefName, location );
	}
	else if( attacker->GetEntityNumber() == gameLocal.GetLocalClientNum() )
	{
		// Clients send a reliable message to the server with the parameters of the hit. The
		// server should make sure the client still has line-of-sight to its target before
		// actually applying the damage.
		
		byte		msgBuffer[MAX_GAME_MESSAGE_SIZE];
		idBitMsg	msg;
		
		msg.InitWrite( msgBuffer, sizeof( msgBuffer ) );
		msg.BeginWriting();
		
		msg.WriteShort( attacker->GetEntityNumber() );
		msg.WriteShort( GetEntityNumber() );		// victim
		msg.WriteVectorFloat( dir );
		msg.WriteLong( damageDef->Index() );
		msg.WriteFloat( damageScale );
		msg.WriteLong( location );
		
		idLobbyBase& lobby = session->GetActingGameStateLobbyBase();
		lobby.SendReliableToHost( GAME_RELIABLE_MESSAGE_CLIENT_HITSCAN_HIT, msg );
	}
	
	lastDamageDef = damageDef->Index();
	lastDamageDir = dir;
	lastDamageDir.Normalize();
	lastDamageLocation = location;
}

/*
===========
idPlayer::Teleport
============
*/
void idPlayer::Teleport( const idVec3& origin, const idAngles& angles, idEntity* destination )
{
	idVec3 org;
	
	if( weapon.GetEntity() )
	{
		weapon.GetEntity()->LowerWeapon();
	}
	
	SetOrigin( origin + idVec3( 0, 0, CM_CLIP_EPSILON ) );
	if( !common->IsMultiplayer() && GetFloorPos( 16.0f, org ) )
	{
		SetOrigin( org );
	}
	
	// clear the ik heights so model doesn't appear in the wrong place
	walkIK.EnableAll();
	
	GetPhysics()->SetLinearVelocity( vec3_origin );
	
	SetViewAngles( angles );
	
	legsYaw = 0.0f;
	idealLegsYaw = 0.0f;
	oldViewYaw = viewAngles.yaw;
	
	if( common->IsMultiplayer() )
	{
		playerView.Flash( colorWhite, 140 );
	}
	
	UpdateVisuals();
	
	teleportEntity = destination;
	
	if( !common->IsClient() && !noclip )
	{
		if( common->IsMultiplayer() )
		{
			// kill anything at the new position or mark for kill depending on immediate or delayed teleport
			gameLocal.KillBox( this, destination != NULL );
		}
		else
		{
			// kill anything at the new position
			gameLocal.KillBox( this, true );
		}
	}
	
	if( PowerUpActive( HELLTIME ) )
	{
		StopHelltime();
	}
}

/*
====================
idPlayer::SetPrivateCameraView
====================
*/
void idPlayer::SetPrivateCameraView( idCamera* camView )
{
	privateCameraView = camView;
	if( camView )
	{
		StopFiring();
		Hide();
	}
	else
	{
		if( !spectating )
		{
			Show();
		}
	}
}

/*
====================
idPlayer::DefaultFov

Returns the base FOV
====================
*/
float idPlayer::DefaultFov() const
{
	float fov;
	
	fov = g_fov.GetFloat();
	if( common->IsMultiplayer() )
	{
		if( fov < 80.0f )
		{
			return 80.0f;
		}
		else if( fov > 120.0f )
		{
			return 120.0f;
		}
	}
	
	return fov;
}

/*
====================
idPlayer::CalcFov

Fixed fov at intermissions, otherwise account for fov variable and zooms.
====================
*/
float idPlayer::CalcFov( bool honorZoom )
{
	float fov;
	
	if( fxFov )
	{
		return DefaultFov() + 10.0f + cos( ( gameLocal.time + 2000 ) * 0.01 ) * 10.0f;
	}
	
	if( influenceFov )
	{
		return influenceFov;
	}
	
	if( zoomFov.IsDone( gameLocal.time ) )
	{
		fov = ( honorZoom && usercmd.buttons & BUTTON_ZOOM ) && weapon.GetEntity() ? weapon.GetEntity()->GetZoomFov() : DefaultFov();
	}
	else
	{
		fov = zoomFov.GetCurrentValue( gameLocal.time );
	}
	
	// bound normal viewsize
	if( fov < 1 )
	{
		fov = 1;
	}
	else if( fov > 179 )
	{
		fov = 179;
	}
	
	return fov;
}

/*
==============
idPlayer::GunTurningOffset

generate a rotational offset for the gun based on the view angle
history in loggedViewAngles
==============
*/
idAngles idPlayer::GunTurningOffset()
{
	idAngles	a;
	
	a.Zero();
	
	if( gameLocal.framenum < NUM_LOGGED_VIEW_ANGLES )
	{
		return a;
	}
	
	idAngles current = loggedViewAngles[ gameLocal.framenum & ( NUM_LOGGED_VIEW_ANGLES - 1 ) ];
	
	idAngles	av, base;
	int weaponAngleOffsetAverages;
	float weaponAngleOffsetScale, weaponAngleOffsetMax;
	
	weapon.GetEntity()->GetWeaponAngleOffsets( &weaponAngleOffsetAverages, &weaponAngleOffsetScale, &weaponAngleOffsetMax );
	
	av = current;
	
	// calcualte this so the wrap arounds work properly
	for( int j = 1 ; j < weaponAngleOffsetAverages ; j++ )
	{
		idAngles a2 = loggedViewAngles[( gameLocal.framenum - j ) & ( NUM_LOGGED_VIEW_ANGLES - 1 ) ];
		
		idAngles delta = a2 - current;
		
		if( delta[1] > 180 )
		{
			delta[1] -= 360;
		}
		else if( delta[1] < -180 )
		{
			delta[1] += 360;
		}
		
		av += delta * ( 1.0f / weaponAngleOffsetAverages );
	}
	
	a = ( av - current ) * weaponAngleOffsetScale;
	
	for( int i = 0 ; i < 3 ; i++ )
	{
		if( a[i] < -weaponAngleOffsetMax )
		{
			a[i] = -weaponAngleOffsetMax;
		}
		else if( a[i] > weaponAngleOffsetMax )
		{
			a[i] = weaponAngleOffsetMax;
		}
	}
	
	return a;
}

/*
==============
idPlayer::GunAcceleratingOffset

generate a positional offset for the gun based on the movement
history in loggedAccelerations
==============
*/
idVec3	idPlayer::GunAcceleratingOffset()
{
	idVec3	ofs;
	
	float weaponOffsetTime, weaponOffsetScale;
	
	ofs.Zero();
	
	weapon.GetEntity()->GetWeaponTimeOffsets( &weaponOffsetTime, &weaponOffsetScale );
	
	int stop = currentLoggedAccel - NUM_LOGGED_ACCELS;
	if( stop < 0 )
	{
		stop = 0;
	}
	for( int i = currentLoggedAccel - 1 ; i > stop ; i-- )
	{
		loggedAccel_t*	acc = &loggedAccel[i & ( NUM_LOGGED_ACCELS - 1 )];
		
		float	f;
		float	t = gameLocal.time - acc->time;
		if( t >= weaponOffsetTime )
		{
			break;	// remainder are too old to care about
		}
		
		f = t / weaponOffsetTime;
		f = ( cos( f * 2.0f * idMath::PI ) - 1.0f ) * 0.5f;
		ofs += f * weaponOffsetScale * acc->dir;
	}
	
	return ofs;
}

/*
==============
idPlayer::CalculateViewWeaponPos

Calculate the bobbing position of the view weapon
==============
*/
void idPlayer::CalculateViewWeaponPos( idVec3& origin, idMat3& axis )
{
	float		scale;
	float		fracsin;
	idAngles	angles;
	int			delta;
	
	// CalculateRenderView must have been called first
	const idVec3& viewOrigin = firstPersonViewOrigin;
	const idMat3& viewAxis = firstPersonViewAxis;
	
	// these cvars are just for hand tweaking before moving a value to the weapon def
	idVec3	gunpos( g_gun_x.GetFloat(), g_gun_y.GetFloat(), g_gun_z.GetFloat() );
	
	// as the player changes direction, the gun will take a small lag
	idVec3	gunOfs = GunAcceleratingOffset();
	origin = viewOrigin + ( gunpos + gunOfs ) * viewAxis;
	
	// on odd legs, invert some angles
	if( bobCycle & 128 )
	{
		scale = -xyspeed;
	}
	else
	{
		scale = xyspeed;
	}
	
	// gun angles from bobbing
	angles.roll		= scale * bobfracsin * 0.005f;
	angles.yaw		= scale * bobfracsin * 0.01f;
	angles.pitch	= xyspeed * bobfracsin * 0.005f;
	
	// gun angles from turning
	if( common->IsMultiplayer() )
	{
		idAngles offset = GunTurningOffset();
		offset *= g_mpWeaponAngleScale.GetFloat();
		angles += offset;
	}
	else
	{
		angles += GunTurningOffset();
	}
	
	idVec3 gravity = physicsObj.GetGravityNormal();
	
	// drop the weapon when landing after a jump / fall
	delta = gameLocal.time - landTime;
	if( delta < LAND_DEFLECT_TIME )
	{
		origin -= gravity * ( landChange * 0.25f * delta / LAND_DEFLECT_TIME );
	}
	else if( delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME )
	{
		origin -= gravity * ( landChange * 0.25f * ( LAND_DEFLECT_TIME + LAND_RETURN_TIME - delta ) / LAND_RETURN_TIME );
	}
	
	// speed sensitive idle drift
	scale = xyspeed + 40.0f;
	fracsin = scale * sin( MS2SEC( gameLocal.time ) ) * 0.01f;
	angles.roll		+= fracsin;
	angles.yaw		+= fracsin;
	angles.pitch	+= fracsin;
	
	// decoupled weapon aiming in head mounted displays
	angles.pitch += independentWeaponPitchAngle;
	
	const idMat3	anglesMat = angles.ToMat3();
	const idMat3	scaledMat = anglesMat * g_gunScale.GetFloat();
	
	axis = scaledMat * viewAxis;
}

/*
===============
idPlayer::OffsetThirdPersonView
===============
*/
void idPlayer::OffsetThirdPersonView( float angle, float range, float height, bool clip )
{
	idVec3			view;
	idVec3			focusAngles;
	trace_t			trace;
	idVec3			focusPoint;
	float			focusDist;
	float			forwardScale, sideScale;
	idVec3			origin;
	idAngles		angles;
	idMat3			axis;
	idBounds		bounds;
	
	angles = viewAngles;
	GetViewPos( origin, axis );
	
	if( angle )
	{
		angles.pitch = 0.0f;
	}
	
	if( angles.pitch > 45.0f )
	{
		angles.pitch = 45.0f;		// don't go too far overhead
	}
	
	focusPoint = origin + angles.ToForward() * THIRD_PERSON_FOCUS_DISTANCE;
	focusPoint.z += height;
	view = origin;
	view.z += 8 + height;
	
	angles.pitch *= 0.5f;
	renderView->viewaxis = angles.ToMat3() * physicsObj.GetGravityAxis();
	
	idMath::SinCos( DEG2RAD( angle ), sideScale, forwardScale );
	view -= range * forwardScale * renderView->viewaxis[ 0 ];
	view += range * sideScale * renderView->viewaxis[ 1 ];
	
	if( clip )
	{
		// trace a ray from the origin to the viewpoint to make sure the view isn't
		// in a solid block.  Use an 8 by 8 block to prevent the view from near clipping anything
		bounds = idBounds( idVec3( -4, -4, -4 ), idVec3( 4, 4, 4 ) );
		gameLocal.clip.TraceBounds( trace, origin, view, bounds, MASK_SOLID, this );
		if( trace.fraction != 1.0f )
		{
			view = trace.endpos;
			view.z += ( 1.0f - trace.fraction ) * 32.0f;
			
			// try another trace to this position, because a tunnel may have the ceiling
			// close enough that this is poking out
			gameLocal.clip.TraceBounds( trace, origin, view, bounds, MASK_SOLID, this );
			view = trace.endpos;
		}
	}
	
	// select pitch to look at focus point from vieword
	focusPoint -= view;
	focusDist = idMath::Sqrt( focusPoint[0] * focusPoint[0] + focusPoint[1] * focusPoint[1] );
	if( focusDist < 1.0f )
	{
		focusDist = 1.0f;	// should never happen
	}
	
	angles.pitch = - RAD2DEG( atan2( focusPoint.z, focusDist ) );
	angles.yaw -= angle;
	
	renderView->vieworg = view;
	renderView->viewaxis = angles.ToMat3() * physicsObj.GetGravityAxis();
	renderView->viewID = 0;
}

/*
===============
idPlayer::GetEyePosition
===============
*/
idVec3 idPlayer::GetEyePosition() const
{
	idVec3 org;
	
	// use the smoothed origin if spectating another player in multiplayer
	if( common->IsClient() && !IsLocallyControlled() )
	{
		org = smoothedOrigin;
	}
	else
	{
		org = GetPhysics()->GetOrigin();
	}
	return org + ( GetPhysics()->GetGravityNormal() * -eyeOffset.z );
}

/*
===============
idPlayer::GetViewPos
===============
*/
void idPlayer::GetViewPos( idVec3& origin, idMat3& axis ) const
{
	idAngles angles;
	
	// if dead, fix the angle and don't add any kick
	if( health <= 0 )
	{
		angles.yaw = viewAngles.yaw;
		angles.roll = 40;
		angles.pitch = -15;
		axis = angles.ToMat3();
		origin = GetEyePosition();
	}
	else
	{
		origin = GetEyePosition() + viewBob;
		angles = viewAngles + viewBobAngles + playerView.AngleOffset();
		
		axis = angles.ToMat3() * physicsObj.GetGravityAxis();
		
		// Move pivot point down so looking straight ahead is a no-op on the Z
		const idVec3& gravityVector = physicsObj.GetGravityNormal();
		origin += gravityVector * g_viewNodalZ.GetFloat();
		
		// adjust the origin based on the camera nodal distance (eye distance from neck)
		origin += axis[0] * g_viewNodalX.GetFloat() + axis[2] * g_viewNodalZ.GetFloat();
	}
}

/*
===============
idPlayer::CalculateFirstPersonView
===============
*/
void idPlayer::CalculateFirstPersonView()
{
	if( ( pm_modelView.GetInteger() == 1 ) || ( ( pm_modelView.GetInteger() == 2 ) && ( health <= 0 ) ) )
	{
		//	Displays the view from the point of view of the "camera" joint in the player model
		
		idMat3 axis;
		idVec3 origin;
		idAngles ang;
		
		ang = viewBobAngles + playerView.AngleOffset();
		ang.yaw += viewAxis[ 0 ].ToYaw();
		
		jointHandle_t joint = animator.GetJointHandle( "camera" );
		animator.GetJointTransform( joint, gameLocal.time, origin, axis );
		firstPersonViewOrigin = ( origin + modelOffset ) * ( viewAxis * physicsObj.GetGravityAxis() ) + physicsObj.GetOrigin() + viewBob;
		firstPersonViewAxis = axis * ang.ToMat3() * physicsObj.GetGravityAxis();
	}
	else
	{
		// offset for local bobbing and kicks
		GetViewPos( firstPersonViewOrigin, firstPersonViewAxis );
#if 0
		// shakefrom sound stuff only happens in first person
		firstPersonViewAxis = firstPersonViewAxis * playerView.ShakeAxis();
#endif
	}
}

/*
==================
idPlayer::GetRenderView

Returns the renderView that was calculated for this tic
==================
*/
renderView_t* idPlayer::GetRenderView()
{
	return renderView;
}

/*
==================
idPlayer::CalculateRenderView

create the renderView for the current tic
==================
*/
void idPlayer::CalculateRenderView()
{
	int i;
	float range;
	
	if( !renderView )
	{
		renderView = new( TAG_ENTITY ) renderView_t;
	}
	memset( renderView, 0, sizeof( *renderView ) );
	
	// copy global shader parms
	for( i = 0; i < MAX_GLOBAL_SHADER_PARMS; i++ )
	{
		renderView->shaderParms[ i ] = gameLocal.globalShaderParms[ i ];
	}
	renderView->globalMaterial = gameLocal.GetGlobalMaterial();
	
	renderView->time[0] = gameLocal.slow.time;
	renderView->time[1] = gameLocal.fast.time;
	
	renderView->viewID = 0;
	
	// check if we should be drawing from a camera's POV
	if( !noclip && ( gameLocal.GetCamera() || privateCameraView ) )
	{
		// get origin, axis, and fov
		if( privateCameraView )
		{
			privateCameraView->GetViewParms( renderView );
		}
		else
		{
			gameLocal.GetCamera()->GetViewParms( renderView );
		}
	}
	else
	{
		if( g_stopTime.GetBool() )
		{
			renderView->vieworg = firstPersonViewOrigin;
			renderView->viewaxis = firstPersonViewAxis;
			
			if( !pm_thirdPerson.GetBool() )
			{
				// set the viewID to the clientNum + 1, so we can suppress the right player bodies and
				// allow the right player view weapons
				renderView->viewID = entityNumber + 1;
			}
		}
		else if( pm_thirdPerson.GetBool() )
		{
			OffsetThirdPersonView( pm_thirdPersonAngle.GetFloat(), pm_thirdPersonRange.GetFloat(), pm_thirdPersonHeight.GetFloat(), pm_thirdPersonClip.GetBool() );
		}
		else if( pm_thirdPersonDeath.GetBool() )
		{
			range = gameLocal.time < minRespawnTime ? ( gameLocal.time + RAGDOLL_DEATH_TIME - minRespawnTime ) * ( 120.0f / RAGDOLL_DEATH_TIME ) : 120.0f;
			OffsetThirdPersonView( 0.0f, 20.0f + range, 0.0f, false );
		}
		else
		{
			renderView->vieworg = firstPersonViewOrigin;
			renderView->viewaxis = firstPersonViewAxis;
			
			// set the viewID to the clientNum + 1, so we can suppress the right player bodies and
			// allow the right player view weapons
			renderView->viewID = entityNumber + 1;
		}
		
		gameLocal.CalcFov( CalcFov( true ), renderView->fov_x, renderView->fov_y );
	}
	
	if( renderView->fov_y == 0 )
	{
		common->Error( "renderView->fov_y == 0" );
	}
	
	if( g_showviewpos.GetBool() )
	{
		gameLocal.Printf( "%s : %s\n", renderView->vieworg.ToString(), renderView->viewaxis.ToAngles().ToString() );
	}
}

/*
=============
idPlayer::AddAIKill
=============
*/
void idPlayer::AddAIKill()
{
	int max_souls;
	int ammo_souls;
	
	if( ( weapon_soulcube < 0 ) || ( inventory.weapons & ( 1 << weapon_soulcube ) ) == 0 )
	{
		return;
	}
	
	ammo_souls = idWeapon::GetAmmoNumForName( "ammo_souls" );
	max_souls = inventory.MaxAmmoForAmmoClass( this, "ammo_souls" );
	const int currentSoulAmmo = inventory.GetInventoryAmmoForType( ammo_souls );
	if( currentSoulAmmo < max_souls )
	{
		inventory.SetInventoryAmmoForType( ammo_souls, currentSoulAmmo + 1 );
		if( inventory.GetInventoryAmmoForType( ammo_souls ) >= max_souls )
		{
		
			if( hud )
			{
				hud->UpdateSoulCube( true );
			}
			StartSound( "snd_soulcube_ready", SND_CHANNEL_ANY, 0, false, NULL );
		}
	}
}

/*
=============
idPlayer::SetSoulCubeProjectile
=============
*/
void idPlayer::SetSoulCubeProjectile( idProjectile* projectile )
{
	soulCubeProjectile = projectile;
}

/*
=============
idPlayer::AddProjectilesFired
=============
*/
void idPlayer::AddProjectilesFired( int count )
{
	numProjectilesFired += count;
}

/*
=============
idPlayer::AddProjectileHites
=============
*/
void idPlayer::AddProjectileHits( int count )
{
	numProjectileHits += count;
}

/*
=============
idPlayer::SetLastHitTime
=============
*/
void idPlayer::SetLastHitTime( int time )
{
	idPlayer* aimed = NULL;
	
	if( time && lastHitTime != time )
	{
		lastHitToggle ^= 1;
	}
	lastHitTime = time;
	if( !time )
	{
		// level start and inits
		return;
	}
	if( common->IsMultiplayer() && ( time - lastSndHitTime ) > 10 )
	{
		lastSndHitTime = time;
		StartSound( "snd_hit_feedback", SND_CHANNEL_ANY, SSF_PRIVATE_SOUND, false, NULL );
	}
	
	if( hud )
	{
		hud->CombatCursorFlash();
	}
	
	if( MPAim != -1 )
	{
		if( gameLocal.entities[ MPAim ] && gameLocal.entities[ MPAim ]->IsType( idPlayer::Type ) )
		{
			aimed = static_cast< idPlayer* >( gameLocal.entities[ MPAim ] );
		}
		assert( aimed );
		// full highlight, no fade till loosing aim
		
		if( hud )
		{
			int color = 0;
			if( aimed )
			{
				color = aimed->team + 1;
			}
			hud->TriggerHitTarget( true, session->GetActingGameStateLobbyBase().GetLobbyUserName( gameLocal.lobbyUserIDs[ MPAim ] ), color );
		}
		MPAimHighlight = true;
		MPAimFadeTime = 0;
	}
	else if( lastMPAim != -1 )
	{
		if( gameLocal.entities[ lastMPAim ] && gameLocal.entities[ lastMPAim ]->IsType( idPlayer::Type ) )
		{
			aimed = static_cast< idPlayer* >( gameLocal.entities[ lastMPAim ] );
		}
		assert( aimed );
		// start fading right away
		if( hud )
		{
			int color = 0;
			if( aimed )
			{
				color = aimed->team + 1;
			}
			hud->TriggerHitTarget( true, session->GetActingGameStateLobbyBase().GetLobbyUserName( gameLocal.lobbyUserIDs[ lastMPAim ] ), color );
			hud->TriggerHitTarget( false, "" );
		}
		MPAimHighlight = false;
		MPAimFadeTime = gameLocal.realClientTime;
	}
}

/*
=============
idPlayer::SetInfluenceLevel
=============
*/
void idPlayer::SetInfluenceLevel( int level )
{
	if( level != influenceActive )
	{
		if( level )
		{
			for( idEntity* ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() )
			{
				if( ent->IsType( idProjectile::Type ) )
				{
					// remove all projectiles
					ent->PostEventMS( &EV_Remove, 0 );
				}
			}
			if( weaponEnabled && weapon.GetEntity() )
			{
				weapon.GetEntity()->EnterCinematic();
			}
		}
		else
		{
			physicsObj.SetLinearVelocity( vec3_origin );
			if( weaponEnabled && weapon.GetEntity() )
			{
				weapon.GetEntity()->ExitCinematic();
			}
		}
		influenceActive = level;
	}
}

/*
=============
idPlayer::SetInfluenceView
=============
*/
void idPlayer::SetInfluenceView( const char* mtr, const char* skinname, float radius, idEntity* ent )
{
	influenceMaterial = NULL;
	influenceEntity = NULL;
	influenceSkin = NULL;
	if( mtr && *mtr )
	{
		influenceMaterial = declManager->FindMaterial( mtr );
	}
	if( skinname && *skinname )
	{
		influenceSkin = declManager->FindSkin( skinname );
		if( head.GetEntity() )
		{
			head.GetEntity()->GetRenderEntity()->shaderParms[ SHADERPARM_TIMEOFFSET ] = -MS2SEC( gameLocal.time );
		}
		UpdateVisuals();
	}
	influenceRadius = radius;
	if( radius > 0.0f )
	{
		influenceEntity = ent;
	}
}

/*
=============
idPlayer::SetInfluenceFov
=============
*/
void idPlayer::SetInfluenceFov( float fov )
{
	influenceFov = fov;
}

/*
================
idPlayer::OnLadder
================
*/
bool idPlayer::OnLadder() const
{
	return physicsObj.OnLadder();
}

/*
==================
idPlayer::Event_GetButtons
==================
*/
void idPlayer::Event_GetButtons()
{
	idThread::ReturnInt( usercmd.buttons );
}

/*
==================
idPlayer::Event_GetMove
==================
*/
void idPlayer::Event_GetMove()
{
	int upmove = ( ( usercmd.buttons & BUTTON_JUMP ) ? 127 : 0 ) - ( ( usercmd.buttons & BUTTON_CROUCH ) ? 127 : 0 );
	idVec3 move( usercmd.forwardmove, usercmd.rightmove, upmove );
	idThread::ReturnVector( move );
}

/*
================
idPlayer::Event_GetViewAngles
================
*/
void idPlayer::Event_GetViewAngles()
{
	idThread::ReturnVector( idVec3( viewAngles[0], viewAngles[1], viewAngles[2] ) );
}

/*
==================
idPlayer::Event_StopFxFov
==================
*/
void idPlayer::Event_StopFxFov()
{
	fxFov = false;
}

/*
==================
idPlayer::StartFxFov
==================
*/
void idPlayer::StartFxFov( float duration )
{
	fxFov = true;
	PostEventSec( &EV_Player_StopFxFov, duration );
}

/*
==================
idPlayer::Event_EnableWeapon
==================
*/
void idPlayer::Event_EnableWeapon()
{
	hiddenWeapon = gameLocal.world->spawnArgs.GetBool( "no_Weapons" );
	weaponEnabled = true;
	if( weapon.GetEntity() )
	{
		weapon.GetEntity()->ExitCinematic();
	}
}

/*
==================
idPlayer::Event_DisableWeapon
==================
*/
void idPlayer::Event_DisableWeapon()
{
	hiddenWeapon = gameLocal.world->spawnArgs.GetBool( "no_Weapons" );
	weaponEnabled = false;
	if( weapon.GetEntity() )
	{
		weapon.GetEntity()->EnterCinematic();
	}
}

/*
==================
idPlayer::Event_GiveInventoryItem
==================
*/
void idPlayer::Event_GiveInventoryItem( const char* name )
{
	GiveInventoryItem( name );
}

/*
==================
idPlayer::Event_RemoveInventoryItem
==================
*/
void idPlayer::Event_RemoveInventoryItem( const char* name )
{
	RemoveInventoryItem( name );
}

/*
==================
idPlayer::Event_GetIdealWeapon
==================
*/
void idPlayer::Event_GetIdealWeapon()
{
	const char* weapon;
	
	if( idealWeapon.Get() >= 0 )
	{
		weapon = spawnArgs.GetString( va( "def_weapon%d", idealWeapon.Get() ) );
		idThread::ReturnString( weapon );
	}
	else
	{
		idThread::ReturnString( "" );
	}
}

/*
==================
idPlayer::Event_SetPowerupTime
==================
*/
void idPlayer::Event_SetPowerupTime( int powerup, int time )
{
	if( time > 0 )
	{
		GivePowerUp( powerup, time, ITEM_GIVE_FEEDBACK | ITEM_GIVE_UPDATE_STATE );
	}
	else
	{
		ClearPowerup( powerup );
	}
}

/*
==================
idPlayer::Event_IsPowerupActive
==================
*/
void idPlayer::Event_IsPowerupActive( int powerup )
{
	idThread::ReturnInt( this->PowerUpActive( powerup ) ? 1 : 0 );
}

/*
==================
idPlayer::Event_StartWarp
==================
*/
void idPlayer::Event_StartWarp()
{
	playerView.AddWarp( idVec3( 0, 0, 0 ), SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, 100, 1000 );
}

/*
==================
idPlayer::Event_StopHelltime
==================
*/
void idPlayer::Event_StopHelltime( int mode )
{
	if( mode == 1 )
	{
		StopHelltime( true );
	}
	else
	{
		StopHelltime( false );
	}
}

/*
==================
idPlayer::Event_WeaponAvailable
==================
*/
void idPlayer::Event_WeaponAvailable( const char* name )
{

	idThread::ReturnInt( WeaponAvailable( name ) ? 1 : 0 );
}

bool idPlayer::WeaponAvailable( const char* name )
{
	for( int i = 0; i < MAX_WEAPONS; i++ )
	{
		if( inventory.weapons & ( 1 << i ) )
		{
			const char* weap = spawnArgs.GetString( va( "def_weapon%d", i ) );
			if( !idStr::Cmp( weap, name ) )
			{
				return true;
			}
		}
	}
	return false;
}


/*
==================
idPlayer::Event_GetCurrentWeapon
==================
*/
void idPlayer::Event_GetCurrentWeapon()
{
	const char* weapon;
	
	if( currentWeapon >= 0 )
	{
		weapon = spawnArgs.GetString( va( "def_weapon%d", currentWeapon ) );
		idThread::ReturnString( weapon );
	}
	else
	{
		idThread::ReturnString( "" );
	}
}

/*
==================
idPlayer::Event_GetPreviousWeapon
==================
*/
void idPlayer::Event_GetPreviousWeapon()
{
	const char* weapon;
	
	if( previousWeapon >= 0 )
	{
		int pw = ( gameLocal.world->spawnArgs.GetBool( "no_Weapons" ) ) ? 0 : previousWeapon;
		weapon = spawnArgs.GetString( va( "def_weapon%d", pw ) );
		idThread::ReturnString( weapon );
	}
	else
	{
		idThread::ReturnString( spawnArgs.GetString( "def_weapon0" ) );
	}
}

/*
==================
idPlayer::Event_SelectWeapon
==================
*/
void idPlayer::Event_SelectWeapon( const char* weaponName )
{
	int i;
	int weaponNum;
	
	if( common->IsClient() )
	{
		gameLocal.Warning( "Cannot switch weapons from script in multiplayer" );
		return;
	}
	
	if( hiddenWeapon && gameLocal.world->spawnArgs.GetBool( "no_Weapons" ) )
	{
		idealWeapon = weapon_fists;
		weapon.GetEntity()->HideWeapon();
		return;
	}
	
	weaponNum = -1;
	for( i = 0; i < MAX_WEAPONS; i++ )
	{
		if( inventory.weapons & ( 1 << i ) )
		{
			const char* weap = spawnArgs.GetString( va( "def_weapon%d", i ) );
			if( !idStr::Cmp( weap, weaponName ) )
			{
				weaponNum = i;
				break;
			}
		}
	}
	
	if( weaponNum < 0 )
	{
		gameLocal.Warning( "%s is not carrying weapon '%s'", name.c_str(), weaponName );
		return;
	}
	
	hiddenWeapon = false;
	idealWeapon = weaponNum;
	
	UpdateHudWeapon();
}

/*
==================
idPlayer::Event_GetWeaponEntity
==================
*/
void idPlayer::Event_GetWeaponEntity()
{
	idThread::ReturnEntity( weapon.GetEntity() );
}

/*
==================
idPlayer::Event_OpenPDA
==================
*/
void idPlayer::Event_OpenPDA()
{
	if( !common->IsMultiplayer() )
	{
		TogglePDA();
	}
}

/*
==================
idPlayer::Event_InPDA
==================
*/
void idPlayer::Event_InPDA()
{
	idThread::ReturnInt( objectiveSystemOpen );
}

/*
==================
idPlayer::TeleportDeath
==================
*/
void idPlayer::TeleportDeath( int killer )
{
	teleportKiller = killer;
}

/*
==================
idPlayer::Event_ExitTeleporter
==================
*/
void idPlayer::Event_ForceOrigin( idVec3& origin, idAngles& angles )
{
	SetOrigin( origin + idVec3( 0, 0, CM_CLIP_EPSILON ) );
	//SetViewAngles( angles );
	
	UpdateVisuals();
}

/*
==================
idPlayer::Event_ExitTeleporter
==================
*/
void idPlayer::Event_ExitTeleporter()
{
	idEntity*	exitEnt;
	float		pushVel;
	
	// verify and setup
	exitEnt = teleportEntity.GetEntity();
	if( !exitEnt )
	{
		common->DPrintf( "Event_ExitTeleporter player %d while not being teleported\n", entityNumber );
		return;
	}
	
	pushVel = exitEnt->spawnArgs.GetFloat( "push", "300" );
	
	if( common->IsServer() )
	{
		ServerSendEvent( EVENT_EXIT_TELEPORTER, NULL, false );
	}
	
	SetPrivateCameraView( NULL );
	// setup origin and push according to the exit target
	SetOrigin( exitEnt->GetPhysics()->GetOrigin() + idVec3( 0, 0, CM_CLIP_EPSILON ) );
	SetViewAngles( exitEnt->GetPhysics()->GetAxis().ToAngles() );
	physicsObj.SetLinearVelocity( exitEnt->GetPhysics()->GetAxis()[ 0 ] * pushVel );
	physicsObj.ClearPushedVelocity();
	// teleport fx
	playerView.Flash( colorWhite, 120 );
	
	// clear the ik heights so model doesn't appear in the wrong place
	walkIK.EnableAll();
	
	UpdateVisuals();
	
	StartSound( "snd_teleport_exit", SND_CHANNEL_ANY, 0, false, NULL );
	
	if( teleportKiller != -1 )
	{
		// we got killed while being teleported
		Damage( gameLocal.entities[ teleportKiller ], gameLocal.entities[ teleportKiller ], vec3_origin, "damage_telefrag", 1.0f, INVALID_JOINT );
		teleportKiller = -1;
	}
	else
	{
		// kill anything that would have waited at teleport exit
		gameLocal.KillBox( this );
	}
	teleportEntity = NULL;
}

/*
================
idPlayer::ClientThink
================
*/
void idPlayer::ClientThink( const int curTime, const float fraction, const bool predict )
{
	if( IsLocallyControlled() )
	{
		aimAssist.Update();
	}
	
	UpdateSkinSetup();
	
	if( !IsLocallyControlled() )
	{
		// ignore attack button of other clients. that's no good for predictions
		usercmd.buttons &= ~BUTTON_ATTACK;
	}
	
	buttonMask &= usercmd.buttons;
	usercmd.buttons &= ~buttonMask;
	
	buttonMask &= usercmd.buttons;
	usercmd.buttons &= ~buttonMask;
	
	if( mountedObject )
	{
		usercmd.forwardmove = 0;
		usercmd.rightmove = 0;
		usercmd.buttons &= ~( BUTTON_JUMP | BUTTON_CROUCH );
	}
	
	if( objectiveSystemOpen )
	{
		usercmd.forwardmove = 0;
		usercmd.rightmove = 0;
		usercmd.buttons &= ~( BUTTON_JUMP | BUTTON_CROUCH );
	}
	
	if( IsLocallyControlled() )
	{
		// zooming
		if( ( usercmd.buttons ^ oldCmd.buttons ) & BUTTON_ZOOM )
		{
			if( ( usercmd.buttons & BUTTON_ZOOM ) && weapon.GetEntity() )
			{
				zoomFov.Init( gameLocal.time, 200.0f, CalcFov( false ), weapon.GetEntity()->GetZoomFov() );
			}
			else
			{
				zoomFov.Init( gameLocal.time, 200.0f, zoomFov.GetCurrentValue( gameLocal.time ), DefaultFov() );
			}
		}
	}
	
	// clear the ik before we do anything else so the skeleton doesn't get updated twice
	walkIK.ClearJointMods();
	
	if( gameLocal.isNewFrame )
	{
		if( usercmd.impulseSequence != oldImpulseSequence )
		{
			PerformImpulse( usercmd.impulse );
		}
	}
	
	if( forceScoreBoard )
	{
		gameLocal.mpGame.SetScoreboardActive( true );
	}
	
	AdjustSpeed();
	
	if( IsLocallyControlled() )
	{
		UpdateViewAngles();
	}
	else
	{
		idQuat interpolatedAngles = Slerp( previousViewQuat, nextViewQuat, fraction );
		viewAngles = interpolatedAngles.ToAngles();
	}
	
	smoothedOriginUpdated = false;
	
	if( !af.IsActive() )
	{
		AdjustBodyAngles();
	}
	
	if( !isLagged )
	{
		// don't allow client to move when lagged
		if( IsLocallyControlled() )
		{
			// Locally-controlled clients are authoritative on their positions, so they can move normally.
			Move();
			usercmd.pos = physicsObj.GetOrigin();
		}
		else
		{
			// Non-locally controlled players are interpolated.
			Move_Interpolated( fraction );
		}
	}
	
	if( !g_stopTime.GetBool() )
	{
		if( !noclip && !spectating && ( health > 0 ) && !IsHidden() )
		{
			TouchTriggers();
		}
	}
	
	// update GUIs, Items, and character interactions
	UpdateFocus();
	
	// service animations
	if( !spectating && !af.IsActive() )
	{
		UpdateConditions();
		UpdateAnimState();
		CheckBlink();
	}
	
	// clear out our pain flag so we can tell if we recieve any damage between now and the next time we think
	AI_PAIN = false;
	
	UpdateLocation();
	
	// calculate the exact bobbed view position, which is used to
	// position the view weapon, among other things
	CalculateFirstPersonView();
	
	// this may use firstPersonView, or a thirdPerson / camera view
	CalculateRenderView();
	
	if( !gameLocal.inCinematic && weapon.GetEntity() && ( health > 0 ) && !( common->IsMultiplayer() && spectating ) )
	{
		UpdateWeapon();
	}
	
	UpdateFlashlight();
	
	UpdateHud();
	
	if( gameLocal.isNewFrame )
	{
		UpdatePowerUps();
	}
	
	UpdateDeathSkin( false );
	
	renderEntity_t* headRenderEnt = NULL;
	if( head.GetEntity() )
	{
		headRenderEnt = head.GetEntity()->GetRenderEntity();
	}
	
	if( headRenderEnt )
	{
		if( influenceSkin )
		{
			headRenderEnt->customSkin = influenceSkin;
		}
		else
		{
			headRenderEnt->customSkin = NULL;
		}
	}
	
	if( common->IsMultiplayer() || g_showPlayerShadow.GetBool() )
	{
		renderEntity.suppressShadowInViewID	= 0;
		if( headRenderEnt )
		{
			headRenderEnt->suppressShadowInViewID = 0;
		}
	}
	else
	{
		renderEntity.suppressShadowInViewID	= entityNumber + 1;
		if( headRenderEnt )
		{
			headRenderEnt->suppressShadowInViewID = entityNumber + 1;
		}
	}
	// never cast shadows from our first-person muzzle flashes
	renderEntity.suppressShadowInLightID = LIGHTID_VIEW_MUZZLE_FLASH + entityNumber;
	if( headRenderEnt )
	{
		headRenderEnt->suppressShadowInLightID = LIGHTID_VIEW_MUZZLE_FLASH + entityNumber;
	}
	
	if( !gameLocal.inCinematic )
	{
		UpdateAnimation();
	}
	
	if( enviroSuitLight.IsValid() )
	{
		idAngles lightAng = firstPersonViewAxis.ToAngles();
		idVec3 lightOrg = firstPersonViewOrigin;
		const idDict* lightDef = gameLocal.FindEntityDefDict( "envirosuit_light", false );
		
		idVec3 enviroOffset = lightDef->GetVector( "enviro_offset" );
		idVec3 enviroAngleOffset = lightDef->GetVector( "enviro_angle_offset" );
		
		lightOrg += ( enviroOffset.x * firstPersonViewAxis[0] );
		lightOrg += ( enviroOffset.y * firstPersonViewAxis[1] );
		lightOrg += ( enviroOffset.z * firstPersonViewAxis[2] );
		lightAng.pitch += enviroAngleOffset.x;
		lightAng.yaw += enviroAngleOffset.y;
		lightAng.roll += enviroAngleOffset.z;
		
		enviroSuitLight.GetEntity()->GetPhysics()->SetOrigin( lightOrg );
		enviroSuitLight.GetEntity()->GetPhysics()->SetAxis( lightAng.ToMat3() );
		enviroSuitLight.GetEntity()->UpdateVisuals();
		enviroSuitLight.GetEntity()->Present();
	}
	
	if( common->IsMultiplayer() )
	{
		DrawPlayerIcons();
	}
	
	Present();
	
	UpdateDamageEffects();
	
	LinkCombat();
	
	// stereo rendering laser sight that replaces the crosshair
	UpdateLaserSight();
	
	if( gameLocal.isNewFrame && IsLocallyControlled() )
	{
		playerView.CalculateShake();
	}
	
	// determine if portal sky is in pvs
	pvsHandle_t	clientPVS = gameLocal.pvs.SetupCurrentPVS( GetPVSAreas(), GetNumPVSAreas() );
	gameLocal.portalSkyActive = gameLocal.pvs.CheckAreasForPortalSky( clientPVS, GetPhysics()->GetOrigin() );
	gameLocal.pvs.FreeCurrentPVS( clientPVS );
	
	//InterpolatePhysics( fraction );
	
	// Make sure voice groups are set to the right team
	if( common->IsMultiplayer() && session->GetState() >= idSession::INGAME && entityNumber < MAX_CLIENTS )  		// The entityNumber < MAX_CLIENTS seems to quiet the static analyzer
	{
		// Make sure we're on the right team (at the lobby level)
		const int voiceTeam = spectating ? LOBBY_SPECTATE_TEAM_FOR_VOICE_CHAT : team;
		
		//idLib::Printf( "CLIENT: Sending voice %i / %i\n", entityNumber, voiceTeam );
		
		// Update lobby team
		session->GetActingGameStateLobbyBase().SetLobbyUserTeam( gameLocal.lobbyUserIDs[ entityNumber ], voiceTeam );
		
		// Update voice groups to match in case something changed
		session->SetVoiceGroupsToTeams();
	}
}

/*
================
idPlayer::GetPhysicsToVisualTransform
================
*/
bool idPlayer::GetPhysicsToVisualTransform( idVec3& origin, idMat3& axis )
{
	if( af.IsActive() )
	{
		af.GetPhysicsToVisualTransform( origin, axis );
		return true;
	}
	
	// smoothen the rendered origin and angles of other clients
	// smooth self origin if snapshots are telling us prediction is off
	if( common->IsClient() && gameLocal.framenum >= smoothedFrame && ( !IsLocallyControlled() || selfSmooth ) )
	{
		// render origin and axis
		idMat3 renderAxis = viewAxis * GetPhysics()->GetAxis();
		idVec3 renderOrigin = GetPhysics()->GetOrigin() + modelOffset * renderAxis;
		
		// update the smoothed origin
		if( !smoothedOriginUpdated )
		{
			idVec2 originDiff = renderOrigin.ToVec2() - smoothedOrigin.ToVec2();
			if( originDiff.LengthSqr() < Square( 100.0f ) )
			{
				// smoothen by pushing back to the previous position
				if( selfSmooth )
				{
					assert( IsLocallyControlled() );
					renderOrigin.ToVec2() -= net_clientSelfSmoothing.GetFloat() * originDiff;
				}
				else
				{
					renderOrigin.ToVec2() -= gameLocal.clientSmoothing * originDiff;
				}
			}
			smoothedOrigin = renderOrigin;
			
			smoothedFrame = gameLocal.framenum;
			smoothedOriginUpdated = true;
		}
		
		axis = idAngles( 0.0f, viewAngles.yaw, 0.0f ).ToMat3();
		origin = ( smoothedOrigin - GetPhysics()->GetOrigin() ) * axis.Transpose();
		
	}
	else
	{
	
		axis = viewAxis;
		origin = modelOffset;
	}
	return true;
}

/*
================
idPlayer::GetPhysicsToSoundTransform
================
*/
bool idPlayer::GetPhysicsToSoundTransform( idVec3& origin, idMat3& axis )
{
	idCamera* camera;
	
	if( privateCameraView )
	{
		camera = privateCameraView;
	}
	else
	{
		camera = gameLocal.GetCamera();
	}
	
	if( camera )
	{
		renderView_t view;
		
		memset( &view, 0, sizeof( view ) );
		camera->GetViewParms( &view );
		origin = view.vieworg;
		axis = view.viewaxis;
		return true;
	}
	else
	{
		return idActor::GetPhysicsToSoundTransform( origin, axis );
	}
}

/*
================
idPlayer::HandleUserCmds
================
*/
void idPlayer::HandleUserCmds( const usercmd_t& newcmd )
{
	// latch button actions
	oldButtons = usercmd.buttons;
	
	// grab out usercmd
	oldCmd = usercmd;
	oldImpulseSequence = usercmd.impulseSequence;
	usercmd = newcmd;
}

/*
================
idPlayer::WriteToSnapshot
================
*/
void idPlayer::WriteToSnapshot( idBitMsg& msg ) const
{
	physicsObj.WriteToSnapshot( msg );
	WriteBindToSnapshot( msg );
	// Only remote players will use these actual viewangles.
	idCQuat snapViewCQuat( viewAngles.ToQuat().ToCQuat() );
	msg.WriteFloat( snapViewCQuat.x );
	msg.WriteFloat( snapViewCQuat.y );
	msg.WriteFloat( snapViewCQuat.z );
	msg.WriteDeltaFloat( 0.0f, deltaViewAngles[0] );
	msg.WriteDeltaFloat( 0.0f, deltaViewAngles[1] );
	msg.WriteDeltaFloat( 0.0f, deltaViewAngles[2] );
	msg.WriteShort( health );
	msg.WriteBits( gameLocal.ServerRemapDecl( -1, DECL_ENTITYDEF, lastDamageDef ), gameLocal.entityDefBits );
	msg.WriteDir( lastDamageDir, 9 );
	msg.WriteShort( lastDamageLocation );
	msg.WriteBits( idealWeapon.Get(), idMath::BitsForInteger( MAX_WEAPONS ) );
	msg.WriteBits( inventory.weapons, MAX_WEAPONS );
	msg.WriteBits( weapon.GetSpawnId(), 32 );
	msg.WriteBits( flashlight.GetSpawnId(), 32 );
	msg.WriteBits( spectator, idMath::BitsForInteger( MAX_CLIENTS ) );
	msg.WriteBits( lastHitToggle, 1 );
	msg.WriteBits( weaponGone, 1 );
	msg.WriteBits( isLagged, 1 );
	msg.WriteShort( team );
	WriteToBitMsg( respawn_netEvent, msg );
	
	/* Needed for the scoreboard */
	msg.WriteBits( carryingFlag, 1 );
	msg.WriteBits( enviroSuitLight.GetSpawnId(), 32 );
	
	msg.WriteBits( AI_CROUCH, 1 );
	msg.WriteBits( AI_ONGROUND, 1 );
	msg.WriteBits( AI_ONLADDER, 1 );
	msg.WriteBits( AI_JUMP, 1 );
	msg.WriteBits( AI_WEAPON_FIRED, 1 );
	msg.WriteBits( AI_ATTACK_HELD, 1 );
	
	msg.WriteByte( usercmd.buttons );
	msg.WriteBits( usercmd.forwardmove, -8 );
	msg.WriteBits( usercmd.rightmove, -8 );
	
	msg.WriteBool( spectating );
}

/*
================
idPlayer::ReadFromSnapshot
================
*/
void idPlayer::ReadFromSnapshot( const idBitMsg& msg )
{
	int		oldHealth, newIdealWeapon, weaponSpawnId;
	int		flashlightSpawnId;
	bool	newHitToggle;
	
	oldHealth = health;
	
	physicsObj.ReadFromSnapshot( msg );
	ReadBindFromSnapshot( msg );
	
	// The remote players get updated view angles from the snapshot.
	idCQuat snapViewCQuat;
	snapViewCQuat.x = msg.ReadFloat();
	snapViewCQuat.y = msg.ReadFloat();
	snapViewCQuat.z = msg.ReadFloat();
	
	idAngles tempDeltaViewAngles;
	tempDeltaViewAngles[0] = msg.ReadDeltaFloat( 0.0f );
	tempDeltaViewAngles[1] = msg.ReadDeltaFloat( 0.0f );
	tempDeltaViewAngles[2] = msg.ReadDeltaFloat( 0.0f );
	
	deltaViewAngles = tempDeltaViewAngles;
	
	health = msg.ReadShort();
	lastDamageDef = gameLocal.ClientRemapDecl( DECL_ENTITYDEF, msg.ReadBits( gameLocal.entityDefBits ) );
	lastDamageDir = msg.ReadDir( 9 );
	lastDamageLocation = msg.ReadShort();
	newIdealWeapon = msg.ReadBits( idMath::BitsForInteger( MAX_WEAPONS ) );
	inventory.weapons = msg.ReadBits( MAX_WEAPONS );
	weaponSpawnId = msg.ReadBits( 32 );
	flashlightSpawnId = msg.ReadBits( 32 );
	spectator = msg.ReadBits( idMath::BitsForInteger( MAX_CLIENTS ) );
	newHitToggle = msg.ReadBits( 1 ) != 0;
	weaponGone = msg.ReadBits( 1 ) != 0;
	isLagged = msg.ReadBits( 1 ) != 0;
	team = msg.ReadShort();
	ReadFromBitMsg( respawn_netEvent, msg );
	
	carryingFlag = msg.ReadBits( 1 ) != 0;
	int enviroSpawnId;
	enviroSpawnId = msg.ReadBits( 32 );
	enviroSuitLight.SetSpawnId( enviroSpawnId );
	
	bool snapshotCrouch = msg.ReadBool();
	bool snapshotOnGround = msg.ReadBool();
	bool snapshotOnLadder = msg.ReadBool();
	bool snapshotJump = msg.ReadBool();
	bool snapShotFired = msg.ReadBool();
	bool snapShotAttackHeld = msg.ReadBool();
	
	byte snapshotButtons = msg.ReadByte();
	signed char snapshotForward = msg.ReadBits( -8 );
	signed char snapshotRight = msg.ReadBits( -8 );
	
	const bool snapshotSpectating = msg.ReadBool();
	
	// no msg reading below this
	
	// Update remote remote player state.
	if( !IsLocallyControlled() )
	{
		previousViewQuat = nextViewQuat;
		nextViewQuat = snapViewCQuat.ToQuat();
		
		AI_CROUCH = snapshotCrouch;
		AI_ONGROUND = snapshotOnGround;
		AI_ONLADDER = snapshotOnLadder;
		AI_JUMP = snapshotJump;
		AI_WEAPON_FIRED = snapShotFired;
		AI_ATTACK_HELD = snapShotAttackHeld;
		
		oldCmd = usercmd;
		
		usercmd.buttons = snapshotButtons;
		usercmd.forwardmove = snapshotForward;
		usercmd.rightmove = snapshotRight;
	}
	
	if( weapon.SetSpawnId( weaponSpawnId ) )
	{
		if( weapon.GetEntity() )
		{
			// maintain ownership locally
			weapon.GetEntity()->SetOwner( this );
		}
		currentWeapon = -1;
	}
	
	if( flashlight.SetSpawnId( flashlightSpawnId ) )
	{
		if( flashlight.GetEntity() )
		{
			flashlight.GetEntity()->SetFlashlightOwner( this );
		}
	}
	
	/*
	// if not a local client
	if ( !IsLocallyControlled() ) {
		// assume the client has all ammo types
		inventory.SetRemoteClientAmmo( GetEntityNumber() );
	}
	*/
	
	// Update spectating state
	const bool wasSpectating = spectating;
	spectating = snapshotSpectating;
	
	if( spectating != wasSpectating )
	{
		Spectate( spectating, false );
	}
	
	if( oldHealth > 0 && health <= 0 )
	{
		if( snapshotStale )
		{
			// so we just hide and don't show a death skin
			UpdateDeathSkin( true );
		}
		// die
		AI_DEAD = true;
		ClearPowerUps();
		SetAnimState( ANIMCHANNEL_LEGS, "Legs_Death", 4 );
		SetAnimState( ANIMCHANNEL_TORSO, "Torso_Death", 4 );
		SetWaitState( "" );
		animator.ClearAllJoints();
		StartRagdoll();
		physicsObj.SetMovementType( PM_DEAD );
		if( !snapshotStale )
		{
			StartSound( "snd_death", SND_CHANNEL_VOICE, 0, false, NULL );
		}
		if( weapon.GetEntity() )
		{
			weapon.GetEntity()->OwnerDied();
		}
		if( flashlight.GetEntity() )
		{
			FlashlightOff();
			flashlight.GetEntity()->OwnerDied();
		}
		
		if( IsLocallyControlled() )
		{
			ControllerShakeFromDamage( oldHealth - health );
		}
		
	}
	else if( health < oldHealth && health > 0 )
	{
		if( snapshotStale )
		{
			lastDmgTime = gameLocal.time;
		}
		else
		{
			// damage feedback
			const idDeclEntityDef* def = static_cast<const idDeclEntityDef*>( declManager->DeclByIndex( DECL_ENTITYDEF, lastDamageDef, false ) );
			if( def )
			{
				if( IsLocallyControlled() )
				{
					playerView.DamageImpulse( lastDamageDir * viewAxis.Transpose(), &def->dict );
					AI_PAIN = Pain( NULL, NULL, oldHealth - health, lastDamageDir, lastDamageLocation );
				}
				lastDmgTime = gameLocal.time;
			}
			else
			{
				common->Warning( "NET: no damage def for damage feedback '%d'\n", lastDamageDef );
			}
			
			if( IsLocallyControlled() )
			{
				ControllerShakeFromDamage( oldHealth - health );
			}
			
		}
	}
	else if( health > oldHealth && PowerUpActive( MEGAHEALTH ) && !snapshotStale )
	{
		// just pulse, for any health raise
		healthPulse = true;
	}
	
	// handle respawns
	if( respawn_netEvent.Get() )
	{
		Init();
		StopRagdoll();
		SetPhysics( &physicsObj );
		// Explicitly set the current origin, since locally-controlled clients
		// don't interpolate. Reading the physics object from the snapshot only
		// updates the next state, not the current state.
		physicsObj.SnapToNextState();
		physicsObj.EnableClip();
		SetCombatContents( true );
		if( flashlight.GetEntity() )
		{
			flashlight.GetEntity()->Show();
		}
		Respawn_Shared();
	}
	
	// If the player is alive, restore proper physics object
	if( health > 0 && IsActiveAF() )
	{
		StopRagdoll();
		SetPhysics( &physicsObj );
		physicsObj.EnableClip();
		SetCombatContents( true );
	}
	
	const int oldIdealWeapon = idealWeapon.Get();
	idealWeapon.UpdateFromSnapshot( newIdealWeapon, GetEntityNumber() );
	
	if( oldIdealWeapon != idealWeapon.Get() )
	{
		if( snapshotStale )
		{
			weaponCatchup = true;
		}
		UpdateHudWeapon();
	}
	
	if( lastHitToggle != newHitToggle )
	{
		SetLastHitTime( gameLocal.realClientTime );
	}
	
	if( msg.HasChanged() )
	{
		UpdateVisuals();
	}
}

/*
================
idPlayer::WritePlayerStateToSnapshot
================
*/
void idPlayer::WritePlayerStateToSnapshot( idBitMsg& msg ) const
{
	msg.WriteByte( bobCycle );
	msg.WriteLong( stepUpTime );
	msg.WriteFloat( stepUpDelta );
	msg.WriteLong( inventory.weapons );
	msg.WriteByte( inventory.armor );
	
	inventory.WriteAmmoToSnapshot( msg );
}

/*
================
idPlayer::ReadPlayerStateFromSnapshot
================
*/
void idPlayer::ReadPlayerStateFromSnapshot( const idBitMsg& msg )
{
	int newBobCycle = 0;
	int newStepUpTime = 0;
	int newStepUpDelta = 0;
	
	newBobCycle = msg.ReadByte();
	newStepUpTime = msg.ReadLong();
	newStepUpDelta = msg.ReadFloat();
	
	inventory.weapons = msg.ReadLong();
	inventory.armor = msg.ReadByte();
	
	inventory.ReadAmmoFromSnapshot( msg, GetEntityNumber() );
}

/*
================
idPlayer::ServerReceiveEvent
================
*/
bool idPlayer::ServerReceiveEvent( int event, int time, const idBitMsg& msg )
{

	if( idEntity::ServerReceiveEvent( event, time, msg ) )
	{
		return true;
	}
	
	return false;
}

/*
================
idPlayer::ClientReceiveEvent
================
*/
bool idPlayer::ClientReceiveEvent( int event, int time, const idBitMsg& msg )
{
	switch( event )
	{
		case EVENT_EXIT_TELEPORTER:
			Event_ExitTeleporter();
			return true;
		case EVENT_ABORT_TELEPORTER:
			SetPrivateCameraView( NULL );
			return true;
		case EVENT_POWERUP:
		{
			int powerup = msg.ReadShort();
			int powertime = msg.ReadShort();
			if( powertime > 0 )
			{
				GivePowerUp( powerup, powertime, ITEM_GIVE_UPDATE_STATE );
			}
			else
			{
				ClearPowerup( powerup );
			}
			return true;
		}
		case EVENT_PICKUPNAME:
		{
			char buf[MAX_EVENT_PARAM_SIZE];
			msg.ReadString( buf, MAX_EVENT_PARAM_SIZE );
			inventory.AddPickupName( buf, this ); //_D3XP
			return true;
		}
		case EVENT_SPECTATE:
		{
			bool spectate = ( msg.ReadBits( 1 ) != 0 );
			Spectate( spectate, true );
			return true;
		}
		case EVENT_ADD_DAMAGE_EFFECT:
		{
			if( spectating )
			{
				// if we're spectating, ignore
				// happens if the event and the spectate change are written on the server during the same frame (fraglimit)
				return true;
			}
			return idActor::ClientReceiveEvent( event, time, msg );
		}
		case EVENT_FORCE_ORIGIN:
		{
		
			idVec3 forceOrigin =  ReadFloatArray< idVec3 >( msg );
			idAngles forceAngles;
			forceAngles[0] = msg.ReadFloat();
			forceAngles[1] = msg.ReadFloat();
			forceAngles[2] = msg.ReadFloat();
			
			Event_ForceOrigin( forceOrigin, forceAngles );
			return true;
		}
		case EVENT_KNOCKBACK:
		{
			idVec3 linearVelocity =  ReadFloatArray< idVec3 >( msg );
			int knockbacktime = msg.ReadByte();
			physicsObj.SetLinearVelocity( linearVelocity );
			physicsObj.SetKnockBack( knockbacktime );
			return true;
		}
		default:
		{
			return idActor::ClientReceiveEvent( event, time, msg );
		}
	}
//	return false;
}

/*
================
idPlayer::Hide
================
*/
void idPlayer::Hide()
{
	idWeapon* weap;
	
	idActor::Hide();
	weap = weapon.GetEntity();
	if( weap )
	{
		weap->HideWorldModel();
	}
	idWeapon* flash = flashlight.GetEntity();
	if( flash )
	{
		flash->HideWorldModel();
	}
}

/*
================
idPlayer::Show
================
*/
void idPlayer::Show()
{
	idWeapon* weap;
	
	idActor::Show();
	weap = weapon.GetEntity();
	if( weap )
	{
		weap->ShowWorldModel();
	}
	idWeapon* flash = flashlight.GetEntity();
	if( flash )
	{
		flash->ShowWorldModel();
	}
}

/*
===============
idPlayer::IsSoundChannelPlaying
===============
*/
bool idPlayer::IsSoundChannelPlaying( const s_channelType channel )
{
	if( GetSoundEmitter() != NULL )
	{
		return GetSoundEmitter()->CurrentlyPlaying( channel );
	}
	
	return false;
}

/*
===============
idPlayer::ShowTip
===============
*/
void idPlayer::ShowTip( const char* title, const char* tip, bool autoHide )
{
	if( tipUp )
	{
		return;
	}
	
	if( hudManager )
	{
		hudManager->ShowTip( title, tip, autoHide );
	}
	tipUp = true;
}

/*
===============
idPlayer::HideTip
===============
*/
void idPlayer::HideTip()
{
	if( hudManager )
	{
		hudManager->HideTip();
	}
	tipUp = false;
}

/*
===============
idPlayer::Event_HideTip
===============
*/
void idPlayer::Event_HideTip()
{
	HideTip();
}

/*
===============
idPlayer::HideObjective
===============
*/
void idPlayer::HideObjective()
{

	StartSound( "snd_objectivedown", SND_CHANNEL_ANY, 0, false, NULL );
	
	if( hud )
	{
		if( objectiveUp )
		{
			hud->HideObjective( false );
			objectiveUp = false;
		}
		else
		{
			hud->HideObjective( true );
		}
	}
}

/*
===============
idPlayer::Event_StopAudioLog
===============
*/
void idPlayer::Event_StopAudioLog()
{
	//EndAudioLog();
}

/*
===============
idPlayer::SetSpectateOrigin
===============
*/
void idPlayer::SetSpectateOrigin()
{
	idVec3 neworig;
	
	neworig = GetPhysics()->GetOrigin();
	neworig[ 2 ] += pm_normalviewheight.GetFloat();
	neworig[ 2 ] += SPECTATE_RAISE;
	SetOrigin( neworig );
}

/*
===============
idPlayer::RemoveWeapon
===============
*/
void idPlayer::RemoveWeapon( const char* weap )
{
	if( weap && *weap )
	{
		inventory.Drop( spawnArgs, spawnArgs.GetString( weap ), -1 );
	}
}

/*
===============
idPlayer::RemoveAllButEssentialWeapons
===============
*/
void idPlayer::RemoveAllButEssentialWeapons()
{
	const idKeyValue* kv = spawnArgs.MatchPrefix( "def_weapon", NULL );
	for( ; kv != NULL; kv = spawnArgs.MatchPrefix( "def_weapon", kv ) )
	{
		// This list probably ought to be placed int the player's def
		if( kv->GetValue() == "weapon_fists" || kv->GetValue() == "weapon_soulcube" || kv->GetValue() == "weapon_pda"
				|| kv->GetValue() == "weapon_flashlight" || kv->GetValue() == "weapon_flashlight_new" )
		{
			continue;
		}
		inventory.Drop( spawnArgs, kv->GetValue(), -1 );
	}
}

/*
===============
idPlayer::CanShowWeaponViewmodel
===============
*/
bool idPlayer::CanShowWeaponViewmodel() const
{
	return ui_showGun.GetBool();
}

/*
===============
idPlayer::SetLevelTrigger
===============
*/
void idPlayer::SetLevelTrigger( const char* levelName, const char* triggerName )
{
	if( levelName && *levelName && triggerName && *triggerName )
	{
		idLevelTriggerInfo lti;
		lti.levelName = levelName;
		lti.triggerName = triggerName;
		inventory.levelTriggers.Append( lti );
	}
}

/*
===============
idPlayer::Event_LevelTrigger
===============
*/
void idPlayer::Event_LevelTrigger()
{
	idStr mapName = gameLocal.GetMapName();
	mapName.StripPath();
	mapName.StripFileExtension();
	for( int i = inventory.levelTriggers.Num() - 1; i >= 0; i-- )
	{
		if( idStr::Icmp( mapName, inventory.levelTriggers[i].levelName ) == 0 )
		{
			idEntity* ent = gameLocal.FindEntity( inventory.levelTriggers[i].triggerName );
			if( ent )
			{
				ent->PostEventMS( &EV_Activate, 1, this );
			}
		}
	}
}

/*
===============
idPlayer::Event_Gibbed
===============
*/
void idPlayer::Event_Gibbed()
{
	// do nothing
}

extern idCVar net_clientMaxPrediction;

/*
===============
idPlayer::UpdatePlayerIcons
===============
*/
void idPlayer::UpdatePlayerIcons()
{
	idLobbyBase& lobby = session->GetActingGameStateLobbyBase();
	int lastPacketTime = lobby.GetPeerTimeSinceLastPacket( lobby.PeerIndexFromLobbyUser( gameLocal.lobbyUserIDs[entityNumber] ) );
	isLagged = ( lastPacketTime > net_clientMaxPrediction.GetInteger() );
	//isChatting = ( ( usercmd.buttons & BUTTON_CHATTING ) && ( health > 0 ) );
}

/*
===============
idPlayer::DrawPlayerIcons
===============
*/
void idPlayer::DrawPlayerIcons()
{
	if( !NeedsIcon() )
	{
		playerIcon.FreeIcon();
		return;
	}
	
	// Never draw icons for hidden players.
	if( this->IsHidden() )
		return;
		
	playerIcon.Draw( this, headJoint );
}

/*
===============
idPlayer::HidePlayerIcons
===============
*/
void idPlayer::HidePlayerIcons()
{
	playerIcon.FreeIcon();
}

/*
===============
idPlayer::NeedsIcon
==============
*/
bool idPlayer::NeedsIcon()
{
	// local clients don't render their own icons... they're only info for other clients
	// always draw icons in CTF games
	return !IsLocallyControlled() && ( ( g_CTFArrows.GetBool() && gameLocal.mpGame.IsGametypeFlagBased() && !IsHidden() && !AI_DEAD ) || ( isLagged ) );
}

/*
===============
idPlayer::DropFlag()
==============
*/
void idPlayer::DropFlag()
{
	if( !carryingFlag || !common->IsMultiplayer() || !gameLocal.mpGame.IsGametypeFlagBased() )  /* CTF */
		return;
		
	idEntity* entity = gameLocal.mpGame.GetTeamFlag( 1 - team );
	if( entity )
	{
		idItemTeam* item = static_cast<idItemTeam*>( entity );
		
		if( item->carried && !item->dropped )
		{
			item->Drop( health <= 0 );
			carryingFlag = false;
		}
	}
	
}

void idPlayer::ReturnFlag()
{

	if( !carryingFlag || !common->IsMultiplayer() || !gameLocal.mpGame.IsGametypeFlagBased() )  /* CTF */
		return;
		
	idEntity* entity = gameLocal.mpGame.GetTeamFlag( 1 - team );
	if( entity )
	{
		idItemTeam* item = static_cast<idItemTeam*>( entity );
		
		if( item->carried && !item->dropped )
		{
			item->Return();
			carryingFlag = false;
		}
	}
}

void idPlayer::FreeModelDef()
{
	idAFEntity_Base::FreeModelDef();
	if( common->IsMultiplayer() && gameLocal.mpGame.IsGametypeFlagBased() )
		playerIcon.FreeIcon();
}

/*
========================
idView::SetControllerShake
========================
*/
void idPlayer::SetControllerShake( float highMagnitude, int highDuration, float lowMagnitude, int lowDuration )
{

	// the main purpose of having these buffer is so multiple, individual shake events can co-exist with each other,
	// for instance, a constant low rumble from the chainsaw when it's idle and a harsh rumble when it's being used.
	
	// find active buffer with similar magnitude values
	int activeBufferWithSimilarMags = -1;
	int inactiveBuffer = -1;
	for( int i = 0; i < MAX_SHAKE_BUFFER; i++ )
	{
		if( gameLocal.GetTime() <= controllerShakeHighTime[i] || gameLocal.GetTime() <= controllerShakeLowTime[i] )
		{
			if( idMath::Fabs( highMagnitude - controllerShakeHighMag[i] ) <= 0.1f && idMath::Fabs( lowMagnitude - controllerShakeLowMag[i] ) <= 0.1f )
			{
				activeBufferWithSimilarMags = i;
				break;
			}
		}
		else
		{
			if( inactiveBuffer == -1 )
			{
				inactiveBuffer = i;		// first, inactive buffer..
			}
		}
	}
	
	if( activeBufferWithSimilarMags > -1 )
	{
		// average the magnitudes and adjust the time
		controllerShakeHighMag[ activeBufferWithSimilarMags ] += highMagnitude;
		controllerShakeHighMag[ activeBufferWithSimilarMags ] *= 0.5f;
		
		controllerShakeLowMag[ activeBufferWithSimilarMags ] += lowMagnitude;
		controllerShakeLowMag[ activeBufferWithSimilarMags ] *= 0.5f;
		
		controllerShakeHighTime[ activeBufferWithSimilarMags ] = gameLocal.GetTime() + highDuration;
		controllerShakeLowTime[ activeBufferWithSimilarMags ] = gameLocal.GetTime() + lowDuration;
		controllerShakeTimeGroup = gameLocal.selectedGroup;
		return;
	}
	
	if( inactiveBuffer == -1 )
	{
		inactiveBuffer = 0;			// FIXME: probably want to use the oldest buffer..
	}
	
	controllerShakeHighMag[ inactiveBuffer ] = highMagnitude;
	controllerShakeLowMag[ inactiveBuffer ] = lowMagnitude;
	controllerShakeHighTime[ inactiveBuffer ] = gameLocal.GetTime() + highDuration;
	controllerShakeLowTime[ inactiveBuffer ] = gameLocal.GetTime() + lowDuration;
	controllerShakeTimeGroup = gameLocal.selectedGroup;
}

/*
========================
idView::ResetControllerShake
========================
*/
void idPlayer::ResetControllerShake()
{
	for( int i = 0; i < MAX_SHAKE_BUFFER; i++ )
	{
		controllerShakeHighTime[i] = 0;
	}
	
	for( int i = 0; i < MAX_SHAKE_BUFFER; i++ )
	{
		controllerShakeHighMag[i] = 0.0f;
	}
	
	for( int i = 0; i < MAX_SHAKE_BUFFER; i++ )
	{
		controllerShakeLowTime[i] = 0;
	}
	
	for( int i = 0; i < MAX_SHAKE_BUFFER; i++ )
	{
		controllerShakeLowMag[i] = 0.0f;
	}
}

/*
========================
idPlayer::GetControllerShake
========================
*/
void idPlayer::GetControllerShake( int& highMagnitude, int& lowMagnitude ) const
{

	if( gameLocal.inCinematic )
	{
		// no controller shake during cinematics
		highMagnitude = 0;
		lowMagnitude = 0;
		return;
	}
	
	float lowMag = 0.0f;
	float highMag = 0.0f;
	
	lowMagnitude = 0;
	highMagnitude = 0;
	
	// use highest values from active buffers
	for( int i = 0; i < MAX_SHAKE_BUFFER; i++ )
	{
		if( gameLocal.GetTimeGroupTime( controllerShakeTimeGroup ) < controllerShakeLowTime[i] )
		{
			if( controllerShakeLowMag[i] > lowMag )
			{
				lowMag = controllerShakeLowMag[i];
			}
		}
		if( gameLocal.GetTimeGroupTime( controllerShakeTimeGroup ) < controllerShakeHighTime[i] )
		{
			if( controllerShakeHighMag[i] > highMag )
			{
				highMag = controllerShakeHighMag[i];
			}
		}
	}
	
	lowMagnitude = idMath::Ftoi( lowMag * 65535.0f );
	highMagnitude = idMath::Ftoi( highMag * 65535.0f );
}

/*
========================
idPlayer::GetExpansionType
========================
*/
gameExpansionType_t idPlayer::GetExpansionType() const
{
	const char* expansion = spawnArgs.GetString( "player_expansion", "d3" );
	if( idStr::Icmp( expansion, "d3" ) == 0 )
	{
		return GAME_BASE;
	}
	if( idStr::Icmp( expansion, "d3xp" ) == 0 )
	{
		return GAME_D3XP;
	}
	if( idStr::Icmp( expansion, "d3le" ) == 0 )
	{
		return GAME_D3LE;
	}
	return GAME_UNKNOWN;
}
