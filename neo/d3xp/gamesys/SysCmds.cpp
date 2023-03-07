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

#include "../tools/imgui/afeditor/AfEditor.h"

/*
==================
Cmd_GetFloatArg
==================
*/
float Cmd_GetFloatArg( const idCmdArgs& args, int& argNum )
{
	const char* value;

	value = args.Argv( argNum++ );
	return atof( value );
}

/*
===================
Cmd_EntityList_f
===================
*/
void Cmd_EntityList_f( const idCmdArgs& args )
{
	int			e;
	idEntity*	check;
	int			count;
	size_t		size;
	idStr		match;

	if( args.Argc() > 1 )
	{
		match = args.Args();
		match.Replace( " ", "" );
	}
	else
	{
		match = "";
	}

	count = 0;
	size = 0;

	gameLocal.Printf( "%-4s  %-20s %-20s %s\n", " Num", "EntityDef", "Class", "Name" );
	gameLocal.Printf( "--------------------------------------------------------------------\n" );
	for( e = 0; e < MAX_GENTITIES; e++ )
	{
		check = gameLocal.entities[ e ];

		if( !check )
		{
			continue;
		}

		if( !check->name.Filter( match, true ) )
		{
			continue;
		}

		gameLocal.Printf( "%4i: %-20s %-20s %s\n", e,
						  check->GetEntityDefName(), check->GetClassname(), check->name.c_str() );

		count++;
		size += check->spawnArgs.Allocated();
	}

	gameLocal.Printf( "...%d entities\n...%d bytes of spawnargs\n", count, size );
}

/*
===================
Cmd_ActiveEntityList_f
===================
*/
void Cmd_ActiveEntityList_f( const idCmdArgs& args )
{
	idEntity*	check;
	int			count;

	count = 0;

	gameLocal.Printf( "%-4s  %-20s %-20s %s\n", " Num", "EntityDef", "Class", "Name" );
	gameLocal.Printf( "--------------------------------------------------------------------\n" );
	for( check = gameLocal.activeEntities.Next(); check != NULL; check = check->activeNode.Next() )
	{
		char	dormant = check->fl.isDormant ? '-' : ' ';
		gameLocal.Printf( "%4i:%c%-20s %-20s %s\n", check->entityNumber, dormant, check->GetEntityDefName(), check->GetClassname(), check->name.c_str() );
		count++;
	}

	gameLocal.Printf( "...%d active entities\n", count );
}

/*
===================
Cmd_ListSpawnArgs_f
===================
*/
void Cmd_ListSpawnArgs_f( const idCmdArgs& args )
{
	int i;
	idEntity* ent;

	ent = gameLocal.FindEntity( args.Argv( 1 ) );
	if( !ent )
	{
		gameLocal.Printf( "entity not found\n" );
		return;
	}

	for( i = 0; i < ent->spawnArgs.GetNumKeyVals(); i++ )
	{
		const idKeyValue* kv = ent->spawnArgs.GetKeyVal( i );
		gameLocal.Printf( "\"%s\"  " S_COLOR_WHITE "\"%s\"\n", kv->GetKey().c_str(), kv->GetValue().c_str() );
	}
}

/*
===================
Cmd_ReloadScript_f
===================
*/
void Cmd_ReloadScript_f( const idCmdArgs& args )
{
	// shutdown the map because entities may point to script objects
	gameLocal.MapShutdown();

	// recompile the scripts
	gameLocal.program.Startup( SCRIPT_DEFAULT );

	if( fileSystem->ReadFile( "doom_main.script", NULL ) > 0 )
	{
		gameLocal.program.CompileFile( "doom_main.script" );
		gameLocal.program.FinishCompilation();
	}

	// error out so that the user can rerun the scripts
	gameLocal.Error( "Exiting map to reload scripts" );
}

CONSOLE_COMMAND( reloadScript2, "Doesn't thow an error...  Use this when switching game modes", 0 )
{
	// shutdown the map because entities may point to script objects
	gameLocal.MapShutdown();

	// recompile the scripts
	gameLocal.program.Startup( SCRIPT_DEFAULT );

	if( fileSystem->ReadFile( "doom_main.script", NULL ) > 0 )
	{
		gameLocal.program.CompileFile( "doom_main.script" );
		gameLocal.program.FinishCompilation();
	}
}

/*
===================
Cmd_Script_f
===================
*/
void Cmd_Script_f( const idCmdArgs& args )
{
	const char* 	script;
	idStr			text;
	idStr			funcname;
	static int		funccount = 0;
	idThread* 		thread;
	const function_t* func;
	idEntity*		ent;

	if( !gameLocal.CheatsOk() )
	{
		return;
	}

	sprintf( funcname, "ConsoleFunction_%d", funccount++ );

	script = args.Args();
	sprintf( text, "void %s() {%s;}\n", funcname.c_str(), script );
	if( gameLocal.program.CompileText( "console", text, true ) )
	{
		func = gameLocal.program.FindFunction( funcname );
		if( func )
		{
			// set all the entity names in case the user named one in the script that wasn't referenced in the default script
			for( ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() )
			{
				gameLocal.program.SetEntity( ent->name, ent );
			}

			thread = new idThread( func );
			thread->Start();
		}
	}
}

/*
==================
KillEntities

Kills all the entities of the given class in a level.
==================
*/
void KillEntities( const idCmdArgs& args, const idTypeInfo& superClass )
{
	idEntity*	ent;
	idStrList	ignore;
	const char* name;
	int			i;

	if( !gameLocal.GetLocalPlayer() || !gameLocal.CheatsOk( false ) )
	{
		return;
	}

	for( i = 1; i < args.Argc(); i++ )
	{
		name = args.Argv( i );
		ignore.Append( name );
	}

	for( ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() )
	{
		if( ent->IsType( superClass ) )
		{
			for( i = 0; i < ignore.Num(); i++ )
			{
				if( ignore[ i ] == ent->name )
				{
					break;
				}
			}

			if( i >= ignore.Num() )
			{
				ent->PostEventMS( &EV_Remove, 0 );
			}
		}
	}
}

/*
==================
Cmd_KillMonsters_f

Kills all the monsters in a level.
==================
*/
void Cmd_KillMonsters_f( const idCmdArgs& args )
{
	KillEntities( args, idAI::Type );

	// kill any projectiles as well since they have pointers to the monster that created them
	KillEntities( args, idProjectile::Type );
}

/*
==================
Cmd_KillMovables_f

Kills all the moveables in a level.
==================
*/
void Cmd_KillMovables_f( const idCmdArgs& args )
{
	if( !gameLocal.GetLocalPlayer() || !gameLocal.CheatsOk( false ) )
	{
		return;
	}
	KillEntities( args, idMoveable::Type );
}

/*
==================
Cmd_KillRagdolls_f

Kills all the ragdolls in a level.
==================
*/
void Cmd_KillRagdolls_f( const idCmdArgs& args )
{
	if( !gameLocal.GetLocalPlayer() || !gameLocal.CheatsOk( false ) )
	{
		return;
	}
	KillEntities( args, idAFEntity_Generic::Type );
	KillEntities( args, idAFEntity_WithAttachedHead::Type );
}

/*
==================
Cmd_Give_f

Give items to a client
==================
*/
void Cmd_Give_f( const idCmdArgs& args )
{
	const char* name;
	int			i;
	bool		give_all;
	idPlayer*	player;

	player = gameLocal.GetLocalPlayer();
	if( !player || !gameLocal.CheatsOk() )
	{
		return;
	}

	name = args.Argv( 1 );

	if( idStr::Icmp( name, "all" ) == 0 )
	{
		give_all = true;
	}
	else
	{
		give_all = false;
	}

	if( give_all || ( idStr::Cmpn( name, "weapon", 6 ) == 0 ) )
	{
		if( gameLocal.world->spawnArgs.GetBool( "no_Weapons" ) )
		{
			gameLocal.world->spawnArgs.SetBool( "no_Weapons", false );
			for( i = 0; i < gameLocal.numClients; i++ )
			{
				if( gameLocal.entities[ i ] )
				{
					gameLocal.entities[ i ]->PostEventSec( &EV_Player_SelectWeapon, 0.5f, gameLocal.entities[ i ]->spawnArgs.GetString( "def_weapon1" ) );
				}
			}
		}
	}

	if( ( idStr::Cmpn( name, "weapon_", 7 ) == 0 ) || ( idStr::Cmpn( name, "item_", 5 ) == 0 ) || ( idStr::Cmpn( name, "ammo_", 5 ) == 0 ) )
	{
		player->GiveItem( name );
		return;
	}

	if( give_all || idStr::Icmp( name, "health" ) == 0 )
	{
		player->health = player->inventory.maxHealth;
		if( !give_all )
		{
			return;
		}
	}

	if( give_all || idStr::Icmp( name, "weapons" ) == 0 )
	{
		player->inventory.weapons = ( int )( BIT( MAX_WEAPONS ) - 1 );
		player->CacheWeapons();

		if( !give_all )
		{
			return;
		}
	}

	if( give_all || idStr::Icmp( name, "ammo" ) == 0 )
	{
		for( i = 0 ; i < AMMO_NUMTYPES; i++ )
		{
			player->inventory.SetInventoryAmmoForType( i, player->inventory.MaxAmmoForAmmoClass( player, idWeapon::GetAmmoNameForNum( ( ammo_t )i ) ) );
		}
		if( !give_all )
		{
			return;
		}
	}

	if( give_all || idStr::Icmp( name, "armor" ) == 0 )
	{
		player->inventory.armor = player->inventory.maxarmor;
		if( !give_all )
		{
			return;
		}
	}

	if( idStr::Icmp( name, "berserk" ) == 0 )
	{
		player->GivePowerUp( BERSERK, SEC2MS( 30.0f ), ITEM_GIVE_FEEDBACK | ITEM_GIVE_UPDATE_STATE );
		return;
	}

	if( idStr::Icmp( name, "invis" ) == 0 )
	{
		player->GivePowerUp( INVISIBILITY, SEC2MS( 30.0f ), ITEM_GIVE_FEEDBACK | ITEM_GIVE_UPDATE_STATE );
		return;
	}

	if( idStr::Icmp( name, "invulnerability" ) == 0 )
	{
		if( args.Argc() > 2 )
		{
			player->GivePowerUp( INVULNERABILITY, atoi( args.Argv( 2 ) ), ITEM_GIVE_FEEDBACK | ITEM_GIVE_UPDATE_STATE );
		}
		else
		{
			player->GivePowerUp( INVULNERABILITY, 30000, ITEM_GIVE_FEEDBACK | ITEM_GIVE_UPDATE_STATE );
		}
		return;
	}

	if( idStr::Icmp( name, "helltime" ) == 0 )
	{
		if( args.Argc() > 2 )
		{
			player->GivePowerUp( HELLTIME, atoi( args.Argv( 2 ) ), ITEM_GIVE_FEEDBACK | ITEM_GIVE_UPDATE_STATE );
		}
		else
		{
			player->GivePowerUp( HELLTIME, 30000, ITEM_GIVE_FEEDBACK | ITEM_GIVE_UPDATE_STATE );
		}
		return;
	}

	if( idStr::Icmp( name, "envirosuit" ) == 0 )
	{
		if( args.Argc() > 2 )
		{
			player->GivePowerUp( ENVIROSUIT, atoi( args.Argv( 2 ) ), ITEM_GIVE_FEEDBACK | ITEM_GIVE_UPDATE_STATE );
		}
		else
		{
			player->GivePowerUp( ENVIROSUIT, 30000, ITEM_GIVE_FEEDBACK | ITEM_GIVE_UPDATE_STATE );
		}
		return;
	}
	if( idStr::Icmp( name, "pda" ) == 0 )
	{
		if( args.Argc() == 2 )
		{
			player->GivePDA( NULL, NULL );
		}
		else if( idStr::Icmp( args.Argv( 2 ), "all" ) == 0 )
		{
			// Give the personal PDA first
			player->GivePDA( NULL, NULL );
			for( int i = 0; i < declManager->GetNumDecls( DECL_PDA ); i++ )
			{
				player->GivePDA( static_cast<const idDeclPDA*>( declManager->DeclByIndex( DECL_PDA, i ) ), NULL );
			}
		}
		else
		{
			const idDeclPDA* pda = static_cast<const idDeclPDA*>( declManager->FindType( DECL_PDA, args.Argv( 2 ), false ) );
			if( pda == NULL )
			{
				gameLocal.Printf( "Unknown PDA %s\n", args.Argv( 2 ) );
			}
			else
			{
				player->GivePDA( pda, NULL );
			}
		}
		return;
	}

	if( idStr::Icmp( name, "video" ) == 0 )
	{
		const idDeclVideo* video = static_cast<const idDeclVideo*>( declManager->FindType( DECL_VIDEO, args.Argv( 2 ), false ) );
		if( video == NULL )
		{
			gameLocal.Printf( "Unknown video %s\n", args.Argv( 2 ) );
		}
		else
		{
			player->GiveVideo( video, NULL );
		}
		return;
	}

	if( !give_all && !player->Give( args.Argv( 1 ), args.Argv( 2 ), ITEM_GIVE_FEEDBACK | ITEM_GIVE_UPDATE_STATE ) )
	{
		gameLocal.Printf( "unknown item\n" );
	}
}

/*
==================
Cmd_CenterView_f

Centers the players pitch
==================
*/
void Cmd_CenterView_f( const idCmdArgs& args )
{
	idPlayer*	player;
	idAngles	ang;

	player = gameLocal.GetLocalPlayer();
	if( !player )
	{
		return;
	}

	ang = player->viewAngles;
	ang.pitch = 0.0f;
	player->SetViewAngles( ang );
}

/*
==================
Cmd_God_f

Sets client to godmode

argv(0) god
==================
*/
void Cmd_God_f( const idCmdArgs& args )
{
	const char*		msg;
	idPlayer*	player;

	player = gameLocal.GetLocalPlayer();
	if( !player || !gameLocal.CheatsOk() )
	{
		return;
	}

	if( player->godmode )
	{
		player->godmode = false;
		msg = "godmode OFF\n";
	}
	else
	{
		player->godmode = true;
		msg = "godmode ON\n";
	}

	gameLocal.Printf( "%s", msg );
}

/*
==================
Cmd_Notarget_f

Sets client to notarget

argv(0) notarget
==================
*/
void Cmd_Notarget_f( const idCmdArgs& args )
{
	const char*		msg;
	idPlayer*	player;

	player = gameLocal.GetLocalPlayer();
	if( !player || !gameLocal.CheatsOk() )
	{
		return;
	}

	if( player->fl.notarget )
	{
		player->fl.notarget = false;
		msg = "notarget OFF\n";
	}
	else
	{
		player->fl.notarget = true;
		msg = "notarget ON\n";
	}

	gameLocal.Printf( "%s", msg );
}

/*
==================
Cmd_Noclip_f

argv(0) noclip
==================
*/
void Cmd_Noclip_f( const idCmdArgs& args )
{
	const char*		msg;
	idPlayer*	player;

	player = gameLocal.GetLocalPlayer();
	if( !player || !gameLocal.CheatsOk() )
	{
		return;
	}

	if( player->noclip )
	{
		msg = "noclip OFF\n";
	}
	else
	{
		msg = "noclip ON\n";
	}
	player->noclip = !player->noclip;

	gameLocal.Printf( "%s", msg );
}

/*
=================
Cmd_PlayerModel_f
=================
*/
void Cmd_PlayerModel_f( const idCmdArgs& args )
{
	idPlayer*	player;
	const char* name;
	idVec3		pos;
	idAngles	ang;

	player = gameLocal.GetLocalPlayer();
	if( !player || !gameLocal.CheatsOk() )
	{
		return;
	}

	if( args.Argc() < 2 )
	{
		gameLocal.Printf( "usage: playerModel <modelname>\n" );
		return;
	}

	name = args.Argv( 1 );
	player->spawnArgs.Set( "model", name );

	pos = player->GetPhysics()->GetOrigin();
	ang = player->viewAngles;
	player->SpawnToPoint( pos, ang );
}

/*
==================
Cmd_Say
==================
*/
static void Cmd_Say( bool team, const idCmdArgs& args )
{
	const char* cmd = team ? "sayTeam" : "say" ;

	if( !common->IsMultiplayer() )
	{
		gameLocal.Printf( "%s can only be used in a multiplayer game\n", cmd );
		return;
	}

	if( args.Argc() < 2 )
	{
		gameLocal.Printf( "usage: %s <text>\n", cmd );
		return;
	}

	idStr text = args.Args();
	if( text.Length() == 0 )
	{
		return;
	}

	if( text[ text.Length() - 1 ] == '\n' )
	{
		text[ text.Length() - 1 ] = '\0';
	}
	const char* name = "player";

	// "server" will only appear on a dedicated server
	if( common->IsServer() && gameLocal.GetLocalClientNum() == -1 )
	{
		name = "server";
	}
	else
	{
		name = session->GetActingGameStateLobbyBase().GetLobbyUserName( gameLocal.lobbyUserIDs[ gameLocal.GetLocalClientNum() ] );

		// Append the player's location to team chat messages in CTF
		idPlayer* player = static_cast<idPlayer*>( gameLocal.entities[ gameLocal.GetLocalClientNum() ] );
		if( gameLocal.mpGame.IsGametypeFlagBased() && team && player )
		{
			idLocationEntity* locationEntity = gameLocal.LocationForPoint( player->GetEyePosition() );

			if( locationEntity )
			{
				idStr temp = "[";
				temp += locationEntity->GetLocation();
				temp += "] ";
				temp += text;
				text = temp;
			}

		}
	}

	if( common->IsClient() )
	{
		idBitMsg	outMsg;
		byte		msgBuf[ 256 ];
		outMsg.InitWrite( msgBuf, sizeof( msgBuf ) );
		outMsg.WriteString( name );
		outMsg.WriteString( text, -1, false );
		session->GetActingGameStateLobbyBase().SendReliableToHost( team ? GAME_RELIABLE_MESSAGE_TCHAT : GAME_RELIABLE_MESSAGE_CHAT, outMsg );
	}
	else
	{
		gameLocal.mpGame.ProcessChatMessage( gameLocal.GetLocalClientNum(), team, name, text, NULL );
	}
}

/*
==================
Cmd_Say_f
==================
*/
static void Cmd_Say_f( const idCmdArgs& args )
{
	Cmd_Say( false, args );
}

/*
==================
Cmd_SayTeam_f
==================
*/
static void Cmd_SayTeam_f( const idCmdArgs& args )
{
	Cmd_Say( true, args );
}

/*
==================
Cmd_AddChatLine_f
==================
*/
static void Cmd_AddChatLine_f( const idCmdArgs& args )
{
	gameLocal.mpGame.AddChatLine( args.Argv( 1 ) );
}

/*
==================
Cmd_GetViewpos_f
==================
*/
void Cmd_GetViewpos_f( const idCmdArgs& args )
{
	idPlayer*	player;
	idVec3		origin;
	idMat3		axis;

	player = gameLocal.GetLocalPlayer();
	if( !player )
	{
		return;
	}

	const renderView_t* view = player->GetRenderView();
	if( view )
	{
		gameLocal.Printf( "(%s) %.1f\n", view->vieworg.ToString(), view->viewaxis[0].ToYaw() );
	}
	else
	{
		player->GetViewPos( origin, axis );
		gameLocal.Printf( "(%s) %.1f\n", origin.ToString(), axis[0].ToYaw() );
	}
}

/*
=================
Cmd_SetViewpos_f
=================
*/
void Cmd_SetViewpos_f( const idCmdArgs& args )
{
	idVec3		origin;
	idAngles	angles;
	int			i;
	idPlayer*	player;

	player = gameLocal.GetLocalPlayer();
	if( !player || !gameLocal.CheatsOk() )
	{
		return;
	}

	if( ( args.Argc() != 4 ) && ( args.Argc() != 5 ) )
	{
		gameLocal.Printf( "usage: setviewpos <x> <y> <z> <yaw>\n" );
		return;
	}

	angles.Zero();
	if( args.Argc() == 5 )
	{
		angles.yaw = atof( args.Argv( 4 ) );
	}

	for( i = 0 ; i < 3 ; i++ )
	{
		origin[i] = atof( args.Argv( i + 1 ) );
	}
	origin.z -= pm_normalviewheight.GetFloat() - 0.25f;

	player->Teleport( origin, angles, NULL );
}

/*
=================
Cmd_Teleport_f
=================
*/
void Cmd_Teleport_f( const idCmdArgs& args )
{
	idVec3		origin;
	idAngles	angles;
	idPlayer*	player;
	idEntity*	ent;

	player = gameLocal.GetLocalPlayer();
	if( !player || !gameLocal.CheatsOk() )
	{
		return;
	}

	if( args.Argc() != 2 )
	{
		gameLocal.Printf( "usage: teleport <name of entity to teleport to>\n" );
		return;
	}

	ent = gameLocal.FindEntity( args.Argv( 1 ) );
	if( !ent )
	{
		gameLocal.Printf( "entity not found\n" );
		return;
	}

	angles.Zero();
	angles.yaw = ent->GetPhysics()->GetAxis()[ 0 ].ToYaw();
	origin = ent->GetPhysics()->GetOrigin();

	player->Teleport( origin, angles, ent );
}

/*
=================
Cmd_Trigger_f
=================
*/
void Cmd_Trigger_f( const idCmdArgs& args )
{
	idVec3		origin;
	idAngles	angles;
	idPlayer*	player;
	idEntity*	ent;

	player = gameLocal.GetLocalPlayer();
	if( !player || !gameLocal.CheatsOk() )
	{
		return;
	}

	if( args.Argc() != 2 )
	{
		gameLocal.Printf( "usage: trigger <name of entity to trigger>\n" );
		return;
	}

	ent = gameLocal.FindEntity( args.Argv( 1 ) );
	if( !ent )
	{
		gameLocal.Printf( "entity not found\n" );
		return;
	}

	ent->Signal( SIG_TRIGGER );
	ent->ProcessEvent( &EV_Activate, player );
	ent->TriggerGuis();
}

/*
===================
Cmd_Spawn_f
===================
*/
void Cmd_Spawn_f( const idCmdArgs& args )
{
	const char* key, *value;
	int			i;
	float		yaw;
	idVec3		org;
	idPlayer*	player;
	idDict		dict;

	player = gameLocal.GetLocalPlayer();
	if( !player || !gameLocal.CheatsOk( false ) )
	{
		return;
	}

	if( args.Argc() & 1 )  	// must always have an even number of arguments
	{
		gameLocal.Printf( "usage: spawn classname [key/value pairs]\n" );
		return;
	}

	yaw = player->viewAngles.yaw;

	value = args.Argv( 1 );
	dict.Set( "classname", value );
	dict.Set( "angle", va( "%f", yaw + 180 ) );

	org = player->GetPhysics()->GetOrigin() + idAngles( 0, yaw, 0 ).ToForward() * 80 + idVec3( 0, 0, 1 );

	// RB: put env_probes a bit higher
	if( idStr::Icmp( value, "env_probe" ) == 0 )
	{
		//org.z = 64;
		org.SnapInt();

		idStr name = gameEdit->GetUniqueEntityName( "env_probe_ingame_placed" );
		dict.Set( "name", name );
	}

	dict.Set( "origin", org.ToString() );

	for( i = 2; i < args.Argc() - 1; i += 2 )
	{

		key = args.Argv( i );
		value = args.Argv( i + 1 );

		dict.Set( key, value );
	}

	gameLocal.SpawnEntityDef( dict );
}

/*
==================
Cmd_Damage_f

Damages the specified entity
==================
*/
void Cmd_Damage_f( const idCmdArgs& args )
{
	if( !gameLocal.GetLocalPlayer() || !gameLocal.CheatsOk( false ) )
	{
		return;
	}
	if( args.Argc() != 3 )
	{
		gameLocal.Printf( "usage: damage <name of entity to damage> <damage>\n" );
		return;
	}

	idEntity* ent = gameLocal.FindEntity( args.Argv( 1 ) );
	if( !ent )
	{
		gameLocal.Printf( "entity not found\n" );
		return;
	}

	ent->Damage( gameLocal.world, gameLocal.world, idVec3( 0, 0, 1 ), "damage_moverCrush", atoi( args.Argv( 2 ) ), INVALID_JOINT );
}


/*
==================
Cmd_Remove_f

Removes the specified entity
==================
*/
void Cmd_Remove_f( const idCmdArgs& args )
{
	if( !gameLocal.GetLocalPlayer() || !gameLocal.CheatsOk( false ) )
	{
		return;
	}
	if( args.Argc() != 2 )
	{
		gameLocal.Printf( "usage: remove <name of entity to remove>\n" );
		return;
	}

	idEntity* ent = gameLocal.FindEntity( args.Argv( 1 ) );
	if( !ent )
	{
		gameLocal.Printf( "entity not found\n" );
		return;
	}

	delete ent;
}

/*
===================
Cmd_TestLight_f
===================
*/
void Cmd_TestLight_f( const idCmdArgs& args )
{
	int			i;
	idStr		filename;
	const char* key = NULL, *value = NULL, *name = NULL;
	idPlayer* 	player = NULL;
	idDict		dict;

	player = gameLocal.GetLocalPlayer();
	if( !player || !gameLocal.CheatsOk( false ) )
	{
		return;
	}

	renderView_t*	rv = player->GetRenderView();

	float fov = tan( idMath::M_DEG2RAD * rv->fov_x / 2 );


	dict.SetMatrix( "rotation", mat3_default );
	dict.SetVector( "origin", rv->vieworg );
	dict.SetVector( "light_target", rv->viewaxis[0] );
	dict.SetVector( "light_right", rv->viewaxis[1] * -fov );
	dict.SetVector( "light_up", rv->viewaxis[2] * fov );
	dict.SetVector( "light_start", rv->viewaxis[0] * 16 );
	dict.SetVector( "light_end", rv->viewaxis[0] * 1000 );

	if( args.Argc() >= 2 )
	{
		value = args.Argv( 1 );
		filename = args.Argv( 1 );
		filename.DefaultFileExtension( ".tga" );
		dict.Set( "texture", filename );
	}

	dict.Set( "classname", "light" );
	for( i = 2; i < args.Argc() - 1; i += 2 )
	{

		key = args.Argv( i );
		value = args.Argv( i + 1 );

		dict.Set( key, value );
	}

	for( i = 0; i < MAX_GENTITIES; i++ )
	{
		name = va( "spawned_light_%d", i );		// not just light_, or it might pick up a prelight shadow
		if( !gameLocal.FindEntity( name ) )
		{
			break;
		}
	}
	dict.Set( "name", name );

	gameLocal.SpawnEntityDef( dict );

	gameLocal.Printf( "Created new light\n" );
}

/*
===================
Cmd_TestPointLight_f
===================
*/
void Cmd_TestPointLight_f( const idCmdArgs& args )
{
	const char* key = NULL, *value = NULL, *name = NULL;
	int			i;
	idPlayer*	player = NULL;
	idDict		dict;

	player = gameLocal.GetLocalPlayer();
	if( !player || !gameLocal.CheatsOk( false ) )
	{
		return;
	}

	dict.SetVector( "origin", player->GetRenderView()->vieworg );

	if( args.Argc() >= 2 )
	{
		value = args.Argv( 1 );
		dict.Set( "light", value );
	}
	else
	{
		dict.Set( "light", "300" );
	}

	dict.Set( "classname", "light" );
	for( i = 2; i < args.Argc() - 1; i += 2 )
	{

		key = args.Argv( i );
		value = args.Argv( i + 1 );

		dict.Set( key, value );
	}

	for( i = 0; i < MAX_GENTITIES; i++ )
	{
		name = va( "light_%d", i );
		if( !gameLocal.FindEntity( name ) )
		{
			break;
		}
	}
	dict.Set( "name", name );

	gameLocal.SpawnEntityDef( dict );

	gameLocal.Printf( "Created new point light\n" );
}

/*
==================
Cmd_PopLight_f
==================
*/
void Cmd_PopLight_f( const idCmdArgs& args )
{
	idEntity*	ent;
	idMapEntity* mapEnt;
	idMapFile*	mapFile = gameLocal.GetLevelMap();
	idLight*		lastLight;
	int			last;

	if( !gameLocal.CheatsOk() )
	{
		return;
	}

	bool removeFromMap = ( args.Argc() > 1 );

	lastLight = NULL;
	last = -1;
	for( ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() )
	{
		if( !ent->IsType( idLight::Type ) )
		{
			continue;
		}

		if( gameLocal.spawnIds[ ent->entityNumber ] > last )
		{
			last = gameLocal.spawnIds[ ent->entityNumber ];
			lastLight = static_cast<idLight*>( ent );
		}
	}

	if( lastLight )
	{
		// find map file entity
		mapEnt = mapFile->FindEntity( lastLight->name );

		if( removeFromMap && mapEnt )
		{
			mapFile->RemoveEntity( mapEnt );
		}
		gameLocal.Printf( "Removing light %i\n", lastLight->GetLightDefHandle() );
		delete lastLight;
	}
	else
	{
		gameLocal.Printf( "No lights to clear.\n" );
	}

}

/*
====================
Cmd_ClearLights_f
====================
*/
void Cmd_ClearLights_f( const idCmdArgs& args )
{
	idEntity* ent;
	idEntity* next;
	idLight* light;
	idMapEntity* mapEnt;
	idMapFile* mapFile = gameLocal.GetLevelMap();

	bool removeFromMap = ( args.Argc() > 1 );

	gameLocal.Printf( "Clearing all lights.\n" );
	for( ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = next )
	{
		next = ent->spawnNode.Next();
		if( !ent->IsType( idLight::Type ) )
		{
			continue;
		}

		light = static_cast<idLight*>( ent );
		mapEnt = mapFile->FindEntity( light->name );

		if( removeFromMap && mapEnt )
		{
			mapFile->RemoveEntity( mapEnt );
		}

		delete light;
	}
}

/*
==================
Cmd_TestFx_f
==================
*/
void Cmd_TestFx_f( const idCmdArgs& args )
{
	idVec3		offset;
	const char* name;
	idPlayer* 	player;
	idDict		dict;

	player = gameLocal.GetLocalPlayer();
	if( !player || !gameLocal.CheatsOk() )
	{
		return;
	}

	// delete the testModel if active
	if( gameLocal.testFx )
	{
		delete gameLocal.testFx;
		gameLocal.testFx = NULL;
	}

	if( args.Argc() < 2 )
	{
		return;
	}

	name = args.Argv( 1 );

	offset = player->GetPhysics()->GetOrigin() + player->viewAngles.ToForward() * 100.0f;

	dict.Set( "origin", offset.ToString() );
	dict.Set( "test", "1" );
	dict.Set( "fx", name );
	gameLocal.testFx = ( idEntityFx* )gameLocal.SpawnEntityType( idEntityFx::Type, &dict );
}

#define MAX_DEBUGLINES	128

typedef struct
{
	bool used;
	idVec3 start, end;
	int color;
	bool blink;
	bool arrow;
} gameDebugLine_t;

gameDebugLine_t debugLines[MAX_DEBUGLINES];

/*
==================
Cmd_AddDebugLine_f
==================
*/
static void Cmd_AddDebugLine_f( const idCmdArgs& args )
{
	int i, argNum;
	const char* value;

	if( !gameLocal.CheatsOk() )
	{
		return;
	}

	if( args.Argc() < 7 )
	{
		gameLocal.Printf( "usage: addline <x y z> <x y z> <color>\n" );
		return;
	}
	for( i = 0; i < MAX_DEBUGLINES; i++ )
	{
		if( !debugLines[i].used )
		{
			break;
		}
	}
	if( i >= MAX_DEBUGLINES )
	{
		gameLocal.Printf( "no free debug lines\n" );
		return;
	}
	value = args.Argv( 0 );
	if( !idStr::Icmp( value, "addarrow" ) )
	{
		debugLines[i].arrow = true;
	}
	else
	{
		debugLines[i].arrow = false;
	}
	debugLines[i].used = true;
	debugLines[i].blink = false;
	argNum = 1;
	debugLines[i].start.x = Cmd_GetFloatArg( args, argNum );
	debugLines[i].start.y = Cmd_GetFloatArg( args, argNum );
	debugLines[i].start.z = Cmd_GetFloatArg( args, argNum );
	debugLines[i].end.x = Cmd_GetFloatArg( args, argNum );
	debugLines[i].end.y = Cmd_GetFloatArg( args, argNum );
	debugLines[i].end.z = Cmd_GetFloatArg( args, argNum );
	debugLines[i].color = Cmd_GetFloatArg( args, argNum );
}

/*
==================
Cmd_RemoveDebugLine_f
==================
*/
static void Cmd_RemoveDebugLine_f( const idCmdArgs& args )
{
	int i, num;
	const char* value;

	if( !gameLocal.CheatsOk() )
	{
		return;
	}

	if( args.Argc() < 2 )
	{
		gameLocal.Printf( "usage: removeline <num>\n" );
		return;
	}
	value = args.Argv( 1 );
	num = atoi( value );
	for( i = 0; i < MAX_DEBUGLINES; i++ )
	{
		if( debugLines[i].used )
		{
			if( --num < 0 )
			{
				break;
			}
		}
	}
	if( i >= MAX_DEBUGLINES )
	{
		gameLocal.Printf( "line not found\n" );
		return;
	}
	debugLines[i].used = false;
}

/*
==================
Cmd_BlinkDebugLine_f
==================
*/
static void Cmd_BlinkDebugLine_f( const idCmdArgs& args )
{
	int i, num;
	const char* value;

	if( !gameLocal.CheatsOk() )
	{
		return;
	}

	if( args.Argc() < 2 )
	{
		gameLocal.Printf( "usage: blinkline <num>\n" );
		return;
	}
	value = args.Argv( 1 );
	num = atoi( value );
	for( i = 0; i < MAX_DEBUGLINES; i++ )
	{
		if( debugLines[i].used )
		{
			if( --num < 0 )
			{
				break;
			}
		}
	}
	if( i >= MAX_DEBUGLINES )
	{
		gameLocal.Printf( "line not found\n" );
		return;
	}
	debugLines[i].blink = !debugLines[i].blink;
}

/*
==================
PrintFloat
==================
*/
static void PrintFloat( float f )
{
	char buf[128];
	int i;

	for( i = sprintf( buf, "%3.2f", f ); i < 7; i++ )
	{
		buf[i] = ' ';
	}
	buf[i] = '\0';
	gameLocal.Printf( buf );
}

/*
==================
Cmd_ListDebugLines_f
==================
*/
static void Cmd_ListDebugLines_f( const idCmdArgs& args )
{
	int i, num;

	if( !gameLocal.CheatsOk() )
	{
		return;
	}

	num = 0;
	gameLocal.Printf( "line num: x1     y1     z1     x2     y2     z2     c  b  a\n" );
	for( i = 0; i < MAX_DEBUGLINES; i++ )
	{
		if( debugLines[i].used )
		{
			gameLocal.Printf( "line %3d: ", num );
			PrintFloat( debugLines[i].start.x );
			PrintFloat( debugLines[i].start.y );
			PrintFloat( debugLines[i].start.z );
			PrintFloat( debugLines[i].end.x );
			PrintFloat( debugLines[i].end.y );
			PrintFloat( debugLines[i].end.z );
			gameLocal.Printf( "%d  %d  %d\n", debugLines[i].color, debugLines[i].blink, debugLines[i].arrow );
			num++;
		}
	}
	if( !num )
	{
		gameLocal.Printf( "no debug lines\n" );
	}
}

/*
==================
D_DrawDebugLines
==================
*/
void D_DrawDebugLines()
{
	int i;
	idVec3 forward, right, up, p1, p2;
	idVec4 color;
	float l;

	for( i = 0; i < MAX_DEBUGLINES; i++ )
	{
		if( debugLines[i].used )
		{
			if( !debugLines[i].blink || ( gameLocal.time & ( 1 << 9 ) ) )
			{
				color = idVec4( debugLines[i].color & 1, ( debugLines[i].color >> 1 ) & 1, ( debugLines[i].color >> 2 ) & 1, 1 );
				gameRenderWorld->DebugLine( color, debugLines[i].start, debugLines[i].end );
				//
				if( debugLines[i].arrow )
				{
					// draw a nice arrow
					forward = debugLines[i].end - debugLines[i].start;
					l = forward.Normalize() * 0.2f;
					forward.NormalVectors( right, up );

					if( l > 3.0f )
					{
						l = 3.0f;
					}
					p1 = debugLines[i].end - l * forward + ( l * 0.4f ) * right;
					p2 = debugLines[i].end - l * forward - ( l * 0.4f ) * right;
					gameRenderWorld->DebugLine( color, debugLines[i].end, p1 );
					gameRenderWorld->DebugLine( color, debugLines[i].end, p2 );
					gameRenderWorld->DebugLine( color, p1, p2 );
				}
			}
		}
	}
}

/*
==================
Cmd_ListCollisionModels_f
==================
*/
static void Cmd_ListCollisionModels_f( const idCmdArgs& args )
{
	if( !gameLocal.CheatsOk() )
	{
		return;
	}

	collisionModelManager->ListModels();
}

/*
==================
Cmd_CollisionModelInfo_f
==================
*/
static void Cmd_CollisionModelInfo_f( const idCmdArgs& args )
{
	const char* value;

	if( !gameLocal.CheatsOk() )
	{
		return;
	}

	if( args.Argc() < 2 )
	{
		gameLocal.Printf( "usage: collisionModelInfo <modelNum>\n"
						  "use 'all' instead of the model number for accumulated info\n" );
		return;
	}

	value = args.Argv( 1 );
	if( !idStr::Icmp( value, "all" ) )
	{
		collisionModelManager->ModelInfo( -1 );
	}
	else
	{
		collisionModelManager->ModelInfo( atoi( value ) );
	}
}

/*
==================
Cmd_ReloadAnims_f
==================
*/
static void Cmd_ReloadAnims_f( const idCmdArgs& args )
{
	// don't allow reloading anims when cheats are disabled,
	// but if we're not in the game, it's ok
	if( gameLocal.GetLocalPlayer() && !gameLocal.CheatsOk( false ) )
	{
		return;
	}

	animationLib.ReloadAnims();
}

/*
==================
Cmd_ListAnims_f
==================
*/
static void Cmd_ListAnims_f( const idCmdArgs& args )
{
	idEntity* 		ent;
	int				num;
	size_t			size;
	size_t			alloced;
	idAnimator* 	animator;
	const char* 	classname;
	const idDict* 	dict;
	int				i;

	if( args.Argc() > 1 )
	{
		idAnimator animator;

		classname = args.Argv( 1 );

		dict = gameLocal.FindEntityDefDict( classname, false );
		if( !dict )
		{
			gameLocal.Printf( "Entitydef '%s' not found\n", classname );
			return;
		}
		animator.SetModel( dict->GetString( "model" ) );

		gameLocal.Printf( "----------------\n" );
		num = animator.NumAnims();
		for( i = 0; i < num; i++ )
		{
			gameLocal.Printf( "%s\n", animator.AnimFullName( i ) );
		}
		gameLocal.Printf( "%d anims\n", num );
	}
	else
	{
		animationLib.ListAnims();

		size = 0;
		num = 0;
		for( ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() )
		{
			animator = ent->GetAnimator();
			if( animator )
			{
				alloced = animator->Allocated();
				size += alloced;
				num++;
			}
		}

		gameLocal.Printf( "%d memory used in %d entity animators\n", size, num );
	}
}

/*
==================
Cmd_AASStats_f
==================
*/
static void Cmd_AASStats_f( const idCmdArgs& args )
{
	int aasNum;

	if( !gameLocal.CheatsOk() )
	{
		return;
	}

	aasNum = aas_test.GetInteger();
	idAAS* aas = gameLocal.GetAAS( aasNum );
	if( !aas )
	{
		gameLocal.Printf( "No aas #%d loaded\n", aasNum );
	}
	else
	{
		aas->Stats();
	}
}

/*
==================
Cmd_TestDamage_f
==================
*/
static void Cmd_TestDamage_f( const idCmdArgs& args )
{
	idPlayer* player;
	const char* damageDefName;

	player = gameLocal.GetLocalPlayer();
	if( !player || !gameLocal.CheatsOk() )
	{
		return;
	}

	if( args.Argc() < 2 || args.Argc() > 3 )
	{
		gameLocal.Printf( "usage: testDamage <damageDefName> [angle]\n" );
		return;
	}

	damageDefName = args.Argv( 1 );

	idVec3	dir;
	if( args.Argc() == 3 )
	{
		float angle = atof( args.Argv( 2 ) );

		idMath::SinCos( DEG2RAD( angle ), dir[1], dir[0] );
		dir[2] = 0;
	}
	else
	{
		dir.Zero();
	}

	// give the player full health before and after
	// running the damage
	player->health = player->inventory.maxHealth;
	player->Damage( NULL, NULL, dir, damageDefName, 1.0f, INVALID_JOINT );
	player->health = player->inventory.maxHealth;
}

/*
==================
Cmd_TestBoneFx_f
==================
*/
static void Cmd_TestBoneFx_f( const idCmdArgs& args )
{
	idPlayer* player;
	const char* bone, *fx;

	player = gameLocal.GetLocalPlayer();
	if( !player || !gameLocal.CheatsOk() )
	{
		return;
	}

	if( args.Argc() < 3 || args.Argc() > 4 )
	{
		gameLocal.Printf( "usage: testBoneFx <fxName> <boneName>\n" );
		return;
	}

	fx = args.Argv( 1 );
	bone = args.Argv( 2 );

	player->StartFxOnBone( fx, bone );
}

/*
==================
Cmd_TestDamage_f
==================
*/
static void Cmd_TestDeath_f( const idCmdArgs& args )
{
	idPlayer* player;

	player = gameLocal.GetLocalPlayer();
	if( !player || !gameLocal.CheatsOk() )
	{
		return;
	}

	idVec3 dir;
	idMath::SinCos( DEG2RAD( 45.0f ), dir[1], dir[0] );
	dir[2] = 0;

	g_testDeath.SetBool( 1 );
	player->Damage( NULL, NULL, dir, "damage_triggerhurt_1000", 1.0f, INVALID_JOINT );
	if( args.Argc() >= 2 )
	{
		player->SpawnGibs( dir, "damage_triggerhurt_1000" );
	}

}

/*
==================
Cmd_WeaponSplat_f
==================
*/
static void Cmd_WeaponSplat_f( const idCmdArgs& args )
{
	idPlayer* player;

	player = gameLocal.GetLocalPlayer();
	if( !player || !gameLocal.CheatsOk() )
	{
		return;
	}

	player->weapon.GetEntity()->BloodSplat( 2.0f );
}

/*
==================
Cmd_SaveSelected_f
==================
*/
static void Cmd_SaveSelected_f( const idCmdArgs& args )
{
	int i;
	idPlayer* player = NULL;
	idEntity* s = NULL;
	idMapEntity* mapEnt;
	idMapFile* mapFile = gameLocal.GetLevelMap();
	idDict dict;
	idStr mapName;
	const char* name = NULL;

	player = gameLocal.GetLocalPlayer();
	if( !player || !gameLocal.CheatsOk() )
	{
		return;
	}

	s = player->dragEntity.GetSelected();
	if( !s )
	{
		gameLocal.Printf( "no entity selected, set g_dragShowSelection 1 to show the current selection\n" );
		return;
	}

	if( args.Argc() > 1 )
	{
		mapName = args.Argv( 1 );
		mapName = "maps/" + mapName;
	}
	else
	{
		mapName = mapFile->GetName();
	}

	// find map file entity
	mapEnt = mapFile->FindEntity( s->name );
	// create new map file entity if there isn't one for this articulated figure
	if( !mapEnt )
	{
		mapEnt = new( TAG_SYSTEM ) idMapEntity();
		mapFile->AddEntity( mapEnt );
		for( i = 0; i < 9999; i++ )
		{
			name = va( "%s_%d", s->GetEntityDefName(), i );
			if( !gameLocal.FindEntity( name ) )
			{
				break;
			}
		}
		s->name = name;
		mapEnt->epairs.Set( "classname", s->GetEntityDefName() );
		mapEnt->epairs.Set( "name", s->name );
	}

	if( s->IsType( idMoveable::Type ) )
	{
		// save the moveable state
		mapEnt->epairs.Set( "origin", s->GetPhysics()->GetOrigin().ToString( 8 ) );
		mapEnt->epairs.Set( "rotation", s->GetPhysics()->GetAxis().ToString( 8 ) );
	}
	else if( s->IsType( idAFEntity_Generic::Type ) || s->IsType( idAFEntity_WithAttachedHead::Type ) )
	{
		// save the articulated figure state
		dict.Clear();
		static_cast<idAFEntity_Base*>( s )->SaveState( dict );
		mapEnt->epairs.Copy( dict );
	}

	// write out the map file
	mapFile->Write( mapName, ".map" );
}

/*
==================
Cmd_DeleteSelected_f
==================
*/
static void Cmd_DeleteSelected_f( const idCmdArgs& args )
{
	idPlayer* player;

	player = gameLocal.GetLocalPlayer();
	if( !player || !gameLocal.CheatsOk() )
	{
		return;
	}

	if( player )
	{
		player->dragEntity.DeleteSelected();
	}
}

/*
==================
Cmd_SaveMoveables_f
==================
*/
static void Cmd_SaveMoveables_f( const idCmdArgs& args )
{
	int e, i;
	idMoveable* m = NULL;
	idMapEntity* mapEnt = NULL;
	idMapFile* mapFile = gameLocal.GetLevelMap();
	idStr mapName;
	const char* name = NULL;

	if( !gameLocal.CheatsOk() )
	{
		return;
	}

	for( e = 0; e < MAX_GENTITIES; e++ )
	{
		m = static_cast<idMoveable*>( gameLocal.entities[ e ] );

		if( !m || !m->IsType( idMoveable::Type ) )
		{
			continue;
		}

		if( m->IsBound() )
		{
			continue;
		}

		if( !m->IsAtRest() )
		{
			break;
		}
	}

	if( e < MAX_GENTITIES )
	{
		gameLocal.Warning( "map not saved because the moveable entity %s is not at rest", gameLocal.entities[ e ]->name.c_str() );
		return;
	}

	if( args.Argc() > 1 )
	{
		mapName = args.Argv( 1 );
		mapName = "maps/" + mapName;
	}
	else
	{
		mapName = mapFile->GetName();
	}

	for( e = 0; e < MAX_GENTITIES; e++ )
	{
		m = static_cast<idMoveable*>( gameLocal.entities[ e ] );

		if( !m || !m->IsType( idMoveable::Type ) )
		{
			continue;
		}

		if( m->IsBound() )
		{
			continue;
		}

		// find map file entity
		mapEnt = mapFile->FindEntity( m->name );
		// create new map file entity if there isn't one for this articulated figure
		if( !mapEnt )
		{
			mapEnt = new( TAG_SYSTEM ) idMapEntity();
			mapFile->AddEntity( mapEnt );
			for( i = 0; i < 9999; i++ )
			{
				name = va( "%s_%d", m->GetEntityDefName(), i );
				if( !gameLocal.FindEntity( name ) )
				{
					break;
				}
			}
			m->name = name;
			mapEnt->epairs.Set( "classname", m->GetEntityDefName() );
			mapEnt->epairs.Set( "name", m->name );
		}
		// save the moveable state
		mapEnt->epairs.Set( "origin", m->GetPhysics()->GetOrigin().ToString( 8 ) );
		mapEnt->epairs.Set( "rotation", m->GetPhysics()->GetAxis().ToString( 8 ) );
	}

	// write out the map file
	mapFile->Write( mapName, ".map" );
}

/*
==================
Cmd_SaveRagdolls_f
==================
*/
static void Cmd_SaveRagdolls_f( const idCmdArgs& args )
{
	int e, i;
	idAFEntity_Base* af = NULL;
	idMapEntity* mapEnt = NULL;
	idMapFile* mapFile = gameLocal.GetLevelMap();
	idDict dict;
	idStr mapName;
	const char* name = NULL;

	if( !gameLocal.CheatsOk() )
	{
		return;
	}

	if( args.Argc() > 1 )
	{
		mapName = args.Argv( 1 );
		mapName = "maps/" + mapName;
	}
	else
	{
		mapName = mapFile->GetName();
	}

	for( e = 0; e < MAX_GENTITIES; e++ )
	{
		af = static_cast<idAFEntity_Base*>( gameLocal.entities[ e ] );

		if( !af )
		{
			continue;
		}

		if( !af->IsType( idAFEntity_WithAttachedHead::Type ) && !af->IsType( idAFEntity_Generic::Type ) )
		{
			continue;
		}

		if( af->IsBound() )
		{
			continue;
		}

		if( !af->IsAtRest() )
		{
			gameLocal.Warning( "the articulated figure for entity %s is not at rest", gameLocal.entities[ e ]->name.c_str() );
		}

		dict.Clear();
		af->SaveState( dict );

		// find map file entity
		mapEnt = mapFile->FindEntity( af->name );
		// create new map file entity if there isn't one for this articulated figure
		if( !mapEnt )
		{
			mapEnt = new( TAG_SYSTEM ) idMapEntity();
			mapFile->AddEntity( mapEnt );
			for( i = 0; i < 9999; i++ )
			{
				name = va( "%s_%d", af->GetEntityDefName(), i );
				if( !gameLocal.FindEntity( name ) )
				{
					break;
				}
			}
			af->name = name;
			mapEnt->epairs.Set( "classname", af->GetEntityDefName() );
			mapEnt->epairs.Set( "name", af->name );
		}
		// save the articulated figure state
		mapEnt->epairs.Copy( dict );
	}

	// write out the map file
	mapFile->Write( mapName, ".map" );
}

/*
==================
Cmd_BindRagdoll_f
==================
*/
static void Cmd_BindRagdoll_f( const idCmdArgs& args )
{
	idPlayer* player;

	player = gameLocal.GetLocalPlayer();
	if( !player || !gameLocal.CheatsOk() )
	{
		return;
	}

	if( player )
	{
		player->dragEntity.BindSelected();
	}
}

/*
==================
Cmd_UnbindRagdoll_f
==================
*/
static void Cmd_UnbindRagdoll_f( const idCmdArgs& args )
{
	idPlayer* player;

	player = gameLocal.GetLocalPlayer();
	if( !player || !gameLocal.CheatsOk() )
	{
		return;
	}

	if( player )
	{
		player->dragEntity.UnbindSelected();
	}
}

/*
==================
Cmd_GameError_f
==================
*/
static void Cmd_GameError_f( const idCmdArgs& args )
{
	gameLocal.Error( "game error" );
}

/*
==================
Cmd_SaveLights_f
==================
*/
static void Cmd_SaveLights_f( const idCmdArgs& args )
{
	int e, i;
	idLight* light = NULL;
	idMapEntity* mapEnt = NULL;
	idMapFile* mapFile = gameLocal.GetLevelMap();
	idDict dict;
	idStr mapName;
	const char* name = NULL;

	if( !gameLocal.CheatsOk() )
	{
		return;
	}

	if( args.Argc() > 1 )
	{
		mapName = args.Argv( 1 );
		mapName = "maps/" + mapName;
	}
	else
	{
		mapName = mapFile->GetName();
	}

	for( e = 0; e < MAX_GENTITIES; e++ )
	{
		light = static_cast<idLight*>( gameLocal.entities[ e ] );

		if( !light || !light->IsType( idLight::Type ) )
		{
			continue;
		}

		dict.Clear();
		light->SaveState( &dict );

		// find map file entity
		mapEnt = mapFile->FindEntity( light->name );
		// create new map file entity if there isn't one for this light
		if( !mapEnt )
		{
			mapEnt = new( TAG_SYSTEM ) idMapEntity();
			mapFile->AddEntity( mapEnt );
			for( i = 0; i < 9999; i++ )
			{
				name = va( "%s_%d", light->GetEntityDefName(), i );
				if( !gameLocal.FindEntity( name ) )
				{
					break;
				}
			}
			light->name = name;
			mapEnt->epairs.Set( "classname", light->GetEntityDefName() );
			mapEnt->epairs.Set( "name", light->name );
		}
		// save the light state
		mapEnt->epairs.Copy( dict );
	}

	// write out the map file
	mapFile->Write( mapName, ".map" );
}


// RB begin
/*
==================
Cmd_SaveEnvprobes_f
==================
*/
static void Cmd_SaveEnvprobes_f( const idCmdArgs& args )
{
	int e;
	EnvironmentProbe* envProbe = NULL;
	idMapEntity* mapEnt = NULL;
	idMapFile* mapFile = gameLocal.GetLevelMap();
	idDict dict;
	idStr mapName;
	idMapFile mapExportFile;
	const char* name = NULL;

	if( !gameLocal.CheatsOk() )
	{
		return;
	}

	if( args.Argc() > 1 )
	{
		mapName = args.Argv( 1 );
		mapName = "maps/" + mapName;
	}
	else
	{
		mapName = mapFile->GetName();
	}

	mapName += "_extra_ents.map";

	for( e = 0; e < MAX_GENTITIES; e++ )
	{
		envProbe = static_cast<EnvironmentProbe*>( gameLocal.entities[ e ] );

		if( !envProbe || !envProbe->IsType( EnvironmentProbe::Type ) )
		{
			continue;
		}

		dict.Clear();
		envProbe->SaveState( &dict );

		mapEnt = new( TAG_SYSTEM ) idMapEntity();
		mapExportFile.AddEntity( mapEnt );

		envProbe->name = name;
		mapEnt->epairs.Set( "classname", envProbe->GetEntityDefName() );
		mapEnt->epairs.Set( "name", envProbe->name );

		// save the env probes state
		mapEnt->epairs.Copy( dict );
	}

	// write out the map file
	mapExportFile.Write( mapName, ".map" );
}
// RB end

/*
==================
Cmd_SaveParticles_f
==================
*/
static void Cmd_SaveParticles_f( const idCmdArgs& args )
{
	int e;
	idEntity* ent;
	idMapEntity* mapEnt;
	idMapFile* mapFile = gameLocal.GetLevelMap();
	idDict dict;
	idStr mapName, strModel;

	if( !gameLocal.CheatsOk() )
	{
		return;
	}

	if( args.Argc() > 1 )
	{
		mapName = args.Argv( 1 );
		mapName = "maps/" + mapName;
	}
	else
	{
		mapName = mapFile->GetName();
	}

	for( e = 0; e < MAX_GENTITIES; e++ )
	{

		ent = static_cast<idStaticEntity*>( gameLocal.entities[ e ] );

		if( !ent )
		{
			continue;
		}

		strModel = ent->spawnArgs.GetString( "model" );
		if( strModel.Length() && strModel.Find( ".prt" ) > 0 )
		{
			dict.Clear();
			dict.Set( "model", ent->spawnArgs.GetString( "model" ) );
			dict.SetVector( "origin", ent->GetPhysics()->GetOrigin() );

			// find map file entity
			mapEnt = mapFile->FindEntity( ent->name );
			// create new map file entity if there isn't one for this entity
			if( !mapEnt )
			{
				continue;
			}
			// save the particle state
			mapEnt->epairs.Copy( dict );
		}
	}

	// write out the map file
	mapFile->Write( mapName, ".map" );
}


/*
==================
Cmd_DisasmScript_f
==================
*/
static void Cmd_DisasmScript_f( const idCmdArgs& args )
{
	gameLocal.program.Disassemble();
}

/*
==================
Cmd_TestSave_f
==================
*/
static void Cmd_TestSave_f( const idCmdArgs& args )
{
	idFile* f, *strings;

	f = fileSystem->OpenFileWrite( "test.sav" );
	strings = NULL;
	gameLocal.SaveGame( f, strings );
	fileSystem->CloseFile( f );
}

/*
==================
Cmd_RecordViewNotes_f
==================
*/
static void Cmd_RecordViewNotes_f( const idCmdArgs& args )
{
	idPlayer* player;
	idVec3 origin;
	idMat3 axis;

	if( args.Argc() <= 3 )
	{
		return;
	}

	player = gameLocal.GetLocalPlayer();
	if( !player )
	{
		return;
	}

	player->GetViewPos( origin, axis );

	// Argv(1) = filename for map (viewnotes/mapname/person)
	// Argv(2) = note number (person0001)
	// Argv(3) = comments

	idStr str = args.Argv( 1 );
	str.SetFileExtension( ".txt" );

	idFile* file = fileSystem->OpenFileAppend( str );

	if( file )
	{
		file->WriteFloatString( "\"view\"\t( %s )\t( %s )\r\n", origin.ToString(), axis.ToString() );
		file->WriteFloatString( "\"comments\"\t\"%s: %s\"\r\n\r\n", args.Argv( 2 ), args.Argv( 3 ) );
		fileSystem->CloseFile( file );
	}

	idStr viewComments = args.Argv( 1 );
	viewComments.StripLeading( "viewnotes/" );
	viewComments += " -- Loc: ";
	viewComments += origin.ToString();
	viewComments += "\n";
	viewComments += args.Argv( 3 );

	// TODO_SPARTY: removed old hud need to find a way of doing this with the new hud
	//player->hud->SetStateString( "viewcomments", viewComments );
	//player->hud->HandleNamedEvent( "showViewComments" );
}

/*
==================
Cmd_CloseViewNotes_f
==================
*/
static void Cmd_CloseViewNotes_f( const idCmdArgs& args )
{
	idPlayer* player = gameLocal.GetLocalPlayer();

	if( !player )
	{
		return;
	}

	// TODO_SPARTY: removed old hud need to find a way of doing this with the new hud
	//player->hud->SetStateString( "viewcomments", "" );
	//player->hud->HandleNamedEvent( "hideViewComments" );
}

/*
==================
Cmd_ShowViewNotes_f
==================
*/
static void Cmd_ShowViewNotes_f( const idCmdArgs& args )
{
	static idLexer parser( LEXFL_ALLOWPATHNAMES | LEXFL_NOSTRINGESCAPECHARS | LEXFL_NOSTRINGCONCAT | LEXFL_NOFATALERRORS );
	idToken	token;
	idPlayer* player;
	idVec3 origin;
	idMat3 axis;

	player = gameLocal.GetLocalPlayer();

	if( !player )
	{
		return;
	}

	if( !parser.IsLoaded() )
	{
		idStr str = "viewnotes/";
		str += gameLocal.GetMapName();
		str.StripFileExtension();
		str += "/";
		if( args.Argc() > 1 )
		{
			str += args.Argv( 1 );
		}
		else
		{
			str += "comments";
		}
		str.SetFileExtension( ".txt" );
		if( !parser.LoadFile( str ) )
		{
			gameLocal.Printf( "No view notes for %s\n", gameLocal.GetMapName() );
			return;
		}
	}

	if( parser.ExpectTokenString( "view" ) && parser.Parse1DMatrix( 3, origin.ToFloatPtr() ) &&
			parser.Parse1DMatrix( 9, axis.ToFloatPtr() ) && parser.ExpectTokenString( "comments" ) && parser.ReadToken( &token ) )
	{

		// TODO_SPARTY: removed old hud need to find a way of doing this with the new hud
		//player->hud->SetStateString( "viewcomments", token );
		//player->hud->HandleNamedEvent( "showViewComments" );
		player->Teleport( origin, axis.ToAngles(), NULL );
	}
	else
	{
		parser.FreeSource();
		// TODO_SPARTY: removed old hud need to find a way of doing this with the new hud
		//player->hud->HandleNamedEvent( "hideViewComments" );
		return;
	}
}

/*
=================
FindEntityGUIs

helper function for Cmd_NextGUI_f.  Checks the passed entity to determine if it
has any valid gui surfaces.
=================
*/
bool FindEntityGUIs( idEntity* ent, const modelSurface_t** surfaces,  int maxSurfs, int& guiSurfaces )
{
	renderEntity_t*			renderEnt;
	idRenderModel*			renderModel;
	const modelSurface_t*	surf;
	const idMaterial*		shader;
	int						i;

	assert( surfaces != NULL );
	assert( ent != NULL );

	memset( surfaces, 0x00, sizeof( modelSurface_t* ) * maxSurfs );
	guiSurfaces = 0;

	renderEnt  = ent->GetRenderEntity();
	renderModel = renderEnt->hModel;
	if( renderModel == NULL )
	{
		return false;
	}

	for( i = 0; i < renderModel->NumSurfaces(); i++ )
	{
		surf = renderModel->Surface( i );
		if( surf == NULL )
		{
			continue;
		}
		shader = surf->shader;
		if( shader == NULL )
		{
			continue;
		}
		if( shader->GetEntityGui() > 0 )
		{
			surfaces[ guiSurfaces++ ] = surf;
		}
	}

	return ( guiSurfaces != 0 );
}

/*
=================
Cmd_NextGUI_f
=================
*/
void Cmd_NextGUI_f( const idCmdArgs& args )
{
	idVec3					origin;
	idAngles				angles;
	idPlayer*				player;
	idEntity*				ent;
	int						guiSurfaces;
	bool					newEnt;
	renderEntity_t*			renderEnt;
	int						surfIndex;
	srfTriangles_t*			geom;
	idVec3					normal;
	idVec3					center;
	const modelSurface_t*	surfaces[ MAX_RENDERENTITY_GUI ];

	player = gameLocal.GetLocalPlayer();
	if( !player || !gameLocal.CheatsOk() )
	{
		return;
	}

	if( args.Argc() != 1 )
	{
		gameLocal.Printf( "usage: nextgui\n" );
		return;
	}

	// start at the last entity
	ent = gameLocal.lastGUIEnt.GetEntity();

	// see if we have any gui surfaces left to go to on the current entity.
	guiSurfaces = 0;
	newEnt = false;
	if( ent == NULL )
	{
		newEnt = true;
	}
	else if( FindEntityGUIs( ent, surfaces, MAX_RENDERENTITY_GUI, guiSurfaces ) == true )
	{
		if( gameLocal.lastGUI >= guiSurfaces )
		{
			newEnt = true;
		}
	}
	else
	{
		// no actual gui surfaces on this ent, so skip it
		newEnt = true;
	}

	if( newEnt == true )
	{
		// go ahead and skip to the next entity with a gui...
		if( ent == NULL )
		{
			ent = gameLocal.spawnedEntities.Next();
		}
		else
		{
			ent = ent->spawnNode.Next();
		}

		for( ; ent != NULL; ent = ent->spawnNode.Next() )
		{
			if( ent->spawnArgs.GetString( "gui", NULL ) != NULL )
			{
				break;
			}

			if( ent->spawnArgs.GetString( "gui2", NULL ) != NULL )
			{
				break;
			}

			if( ent->spawnArgs.GetString( "gui3", NULL ) != NULL )
			{
				break;
			}

			// try the next entity
			gameLocal.lastGUIEnt = ent;
		}

		gameLocal.lastGUIEnt = ent;
		gameLocal.lastGUI = 0;

		if( !ent )
		{
			gameLocal.Printf( "No more gui entities. Starting over...\n" );
			return;
		}
	}

	if( FindEntityGUIs( ent, surfaces, MAX_RENDERENTITY_GUI, guiSurfaces ) == false )
	{
		gameLocal.Printf( "Entity \"%s\" has gui properties but no gui surfaces.\n", ent->name.c_str() );
	}

	if( guiSurfaces == 0 )
	{
		gameLocal.Printf( "Entity \"%s\" has gui properties but no gui surfaces!\n", ent->name.c_str() );
		return;
	}

	gameLocal.Printf( "Teleporting to gui entity \"%s\", gui #%d.\n" , ent->name.c_str(), gameLocal.lastGUI );

	renderEnt = ent->GetRenderEntity();
	surfIndex = gameLocal.lastGUI++;
	geom = surfaces[ surfIndex ]->geometry;
	if( geom == NULL )
	{
		gameLocal.Printf( "Entity \"%s\" has gui surface %d without geometry!\n", ent->name.c_str(), surfIndex );
		return;
	}

	const idVec3& v0 = geom->verts[geom->indexes[0]].xyz;
	const idVec3& v1 = geom->verts[geom->indexes[1]].xyz;
	const idVec3& v2 = geom->verts[geom->indexes[2]].xyz;

	const idPlane plane( v0, v1, v2 );

	normal = plane.Normal() * renderEnt->axis;
	center = geom->bounds.GetCenter() * renderEnt->axis + renderEnt->origin;

	origin = center + ( normal * 32.0f );
	origin.z -= player->EyeHeight();
	normal *= -1.0f;
	angles = normal.ToAngles();

	//	make sure the player is in noclip
	player->noclip = true;
	player->Teleport( origin, angles, NULL );
}

void Cmd_SetActorState_f( const idCmdArgs& args )
{

	if( args.Argc() != 3 )
	{
		common->Printf( "usage: setActorState <entity name> <state>\n" );
		return;
	}

	idEntity* ent;
	ent = gameLocal.FindEntity( args.Argv( 1 ) );
	if( !ent )
	{
		gameLocal.Printf( "entity not found\n" );
		return;
	}


	if( !ent->IsType( idActor::Type ) )
	{
		gameLocal.Printf( "entity not an actor\n" );
		return;
	}

	idActor* actor = ( idActor* )ent;
	actor->PostEventMS( &AI_SetState, 0, args.Argv( 2 ) );
}

#if 0
// not used
static void ArgCompletion_DefFile( const idCmdArgs& args, void( *callback )( const char* s ) )
{
	cmdSystem->ArgCompletion_FolderExtension( args, callback, "def/", true, ".def", NULL );
}
#endif

/*
===============
Cmd_TestId_f
outputs a string from the string table for the specified id
===============
*/
void Cmd_TestId_f( const idCmdArgs& args )
{
	idStr	id;
	int		i;
	if( args.Argc() == 1 )
	{
		common->Printf( "usage: testid <string id>\n" );
		return;
	}

	for( i = 1; i < args.Argc(); i++ )
	{
		id += args.Argv( i );
	}
	if( idStr::Cmpn( id, STRTABLE_ID, STRTABLE_ID_LENGTH ) != 0 )
	{
		id = STRTABLE_ID + id;
	}
	gameLocal.mpGame.AddChatLine( idLocalization::GetString( id ), "<nothing>", "<nothing>", "<nothing>" );
}


// RB begin
void Cmd_EditLights_f( const idCmdArgs& args )
{
	if( g_editEntityMode.GetInteger() != 1 )
	{
		g_editEntityMode.SetInteger( 1 );

		// turn off com_smp multithreading so we can load and check light textures on main thread
		com_editors |= EDITOR_LIGHT;
	}
	else
	{
		g_editEntityMode.SetInteger( 0 );

		com_editors &= ~EDITOR_LIGHT;

		// turn off light debug drawing in the render backend
		r_singleLight.SetInteger( -1 );
		r_showLights.SetInteger( 0 );
	}
}
// RB end

// SP begin
void Cmd_ShowAfEditor_f( const idCmdArgs& args )
{
	if( g_editEntityMode.GetInteger() != 3 )
	{
		g_editEntityMode.SetInteger( 3 );
		com_editors |= EDITOR_AF;
		ImGuiTools::AfEditor::Instance().Init();
		ImGuiTools::AfEditor::Instance().ShowIt( true );
	}
	else
	{
		g_editEntityMode.SetInteger( 0 );
		com_editors &= ~EDITOR_AF;
		ImGuiTools::AfEditor::Instance().ShowIt( false );
	}
}
// SP end

/*
=================
idGameLocal::InitConsoleCommands

Let the system know about all of our commands
so it can perform tab completion
=================
*/
void idGameLocal::InitConsoleCommands()
{
	cmdSystem->AddCommand( "game_memory",			idClass::DisplayInfo_f,		CMD_FL_GAME,				"displays game class info" );
	cmdSystem->AddCommand( "listClasses",			idClass::ListClasses_f,		CMD_FL_GAME,				"lists game classes" );
	cmdSystem->AddCommand( "listThreads",			idThread::ListThreads_f,	CMD_FL_GAME | CMD_FL_CHEAT,	"lists script threads" );
	cmdSystem->AddCommand( "listEntities",			Cmd_EntityList_f,			CMD_FL_GAME | CMD_FL_CHEAT,	"lists game entities" );
	cmdSystem->AddCommand( "listActiveEntities",	Cmd_ActiveEntityList_f,		CMD_FL_GAME | CMD_FL_CHEAT,	"lists active game entities" );
	cmdSystem->AddCommand( "listMonsters",			idAI::List_f,				CMD_FL_GAME | CMD_FL_CHEAT,	"lists monsters" );
	cmdSystem->AddCommand( "listSpawnArgs",			Cmd_ListSpawnArgs_f,		CMD_FL_GAME | CMD_FL_CHEAT,	"list the spawn args of an entity", idGameLocal::ArgCompletion_EntityName );
	cmdSystem->AddCommand( "say",					Cmd_Say_f,					CMD_FL_GAME,				"text chat" );
	cmdSystem->AddCommand( "sayTeam",				Cmd_SayTeam_f,				CMD_FL_GAME,				"team text chat" );
	cmdSystem->AddCommand( "addChatLine",			Cmd_AddChatLine_f,			CMD_FL_GAME,				"internal use - core to game chat lines" );
	cmdSystem->AddCommand( "give",					Cmd_Give_f,					CMD_FL_GAME | CMD_FL_CHEAT,	"gives one or more items" );
	cmdSystem->AddCommand( "centerview",			Cmd_CenterView_f,			CMD_FL_GAME,				"centers the view" );
	cmdSystem->AddCommand( "god",					Cmd_God_f,					CMD_FL_GAME | CMD_FL_CHEAT,	"enables god mode" );
	cmdSystem->AddCommand( "notarget",				Cmd_Notarget_f,				CMD_FL_GAME | CMD_FL_CHEAT,	"disables the player as a target" );
	cmdSystem->AddCommand( "noclip",				Cmd_Noclip_f,				CMD_FL_GAME | CMD_FL_CHEAT,	"disables collision detection for the player" );
	cmdSystem->AddCommand( "where",					Cmd_GetViewpos_f,			CMD_FL_GAME | CMD_FL_CHEAT,	"prints the current view position" );
	cmdSystem->AddCommand( "getviewpos",			Cmd_GetViewpos_f,			CMD_FL_GAME | CMD_FL_CHEAT,	"prints the current view position" );
	cmdSystem->AddCommand( "setviewpos",			Cmd_SetViewpos_f,			CMD_FL_GAME | CMD_FL_CHEAT,	"sets the current view position" );
	cmdSystem->AddCommand( "teleport",				Cmd_Teleport_f,				CMD_FL_GAME | CMD_FL_CHEAT,	"teleports the player to an entity location", idGameLocal::ArgCompletion_EntityName );
	cmdSystem->AddCommand( "trigger",				Cmd_Trigger_f,				CMD_FL_GAME | CMD_FL_CHEAT,	"triggers an entity", idGameLocal::ArgCompletion_EntityName );
	cmdSystem->AddCommand( "spawn",					Cmd_Spawn_f,				CMD_FL_GAME | CMD_FL_CHEAT,	"spawns a game entity", idCmdSystem::ArgCompletion_Decl<DECL_ENTITYDEF> );
	cmdSystem->AddCommand( "damage",				Cmd_Damage_f,				CMD_FL_GAME | CMD_FL_CHEAT,	"apply damage to an entity", idGameLocal::ArgCompletion_EntityName );
	cmdSystem->AddCommand( "remove",				Cmd_Remove_f,				CMD_FL_GAME | CMD_FL_CHEAT,	"removes an entity", idGameLocal::ArgCompletion_EntityName );
	cmdSystem->AddCommand( "killMonsters",			Cmd_KillMonsters_f,			CMD_FL_GAME | CMD_FL_CHEAT,	"removes all monsters" );
	cmdSystem->AddCommand( "killMoveables",			Cmd_KillMovables_f,			CMD_FL_GAME | CMD_FL_CHEAT,	"removes all moveables" );
	cmdSystem->AddCommand( "killRagdolls",			Cmd_KillRagdolls_f,			CMD_FL_GAME | CMD_FL_CHEAT,	"removes all ragdolls" );
	cmdSystem->AddCommand( "addline",				Cmd_AddDebugLine_f,			CMD_FL_GAME | CMD_FL_CHEAT,	"adds a debug line" );
	cmdSystem->AddCommand( "addarrow",				Cmd_AddDebugLine_f,			CMD_FL_GAME | CMD_FL_CHEAT,	"adds a debug arrow" );
	cmdSystem->AddCommand( "removeline",			Cmd_RemoveDebugLine_f,		CMD_FL_GAME | CMD_FL_CHEAT,	"removes a debug line" );
	cmdSystem->AddCommand( "blinkline",				Cmd_BlinkDebugLine_f,		CMD_FL_GAME | CMD_FL_CHEAT,	"blinks a debug line" );
	cmdSystem->AddCommand( "listLines",				Cmd_ListDebugLines_f,		CMD_FL_GAME | CMD_FL_CHEAT,	"lists all debug lines" );
	cmdSystem->AddCommand( "playerModel",			Cmd_PlayerModel_f,			CMD_FL_GAME | CMD_FL_CHEAT,	"sets the given model on the player", idCmdSystem::ArgCompletion_Decl<DECL_MODELDEF> );
	cmdSystem->AddCommand( "testFx",				Cmd_TestFx_f,				CMD_FL_GAME | CMD_FL_CHEAT,	"tests an FX system", idCmdSystem::ArgCompletion_Decl<DECL_FX> );
	cmdSystem->AddCommand( "testBoneFx",			Cmd_TestBoneFx_f,			CMD_FL_GAME | CMD_FL_CHEAT,	"tests an FX system bound to a joint", idCmdSystem::ArgCompletion_Decl<DECL_FX> );
	cmdSystem->AddCommand( "testLight",				Cmd_TestLight_f,			CMD_FL_GAME | CMD_FL_CHEAT,	"tests a light" );
	cmdSystem->AddCommand( "testPointLight",		Cmd_TestPointLight_f,		CMD_FL_GAME | CMD_FL_CHEAT,	"tests a point light" );
	cmdSystem->AddCommand( "popLight",				Cmd_PopLight_f,				CMD_FL_GAME | CMD_FL_CHEAT,	"removes the last created light" );
	cmdSystem->AddCommand( "testDeath",				Cmd_TestDeath_f,			CMD_FL_GAME | CMD_FL_CHEAT,	"tests death" );
	cmdSystem->AddCommand( "testSave",				Cmd_TestSave_f,				CMD_FL_GAME | CMD_FL_CHEAT,	"writes out a test savegame" );
	cmdSystem->AddCommand( "testModel",				idTestModel::TestModel_f,			CMD_FL_GAME | CMD_FL_CHEAT,	"tests a model", idTestModel::ArgCompletion_TestModel );
	cmdSystem->AddCommand( "testSkin",				idTestModel::TestSkin_f,			CMD_FL_GAME | CMD_FL_CHEAT,	"tests a skin on an existing testModel", idCmdSystem::ArgCompletion_Decl<DECL_SKIN> );
	cmdSystem->AddCommand( "testShaderParm",		idTestModel::TestShaderParm_f,		CMD_FL_GAME | CMD_FL_CHEAT,	"sets a shaderParm on an existing testModel" );
	cmdSystem->AddCommand( "keepTestModel",			idTestModel::KeepTestModel_f,		CMD_FL_GAME | CMD_FL_CHEAT,	"keeps the last test model in the game" );
	cmdSystem->AddCommand( "testAnim",				idTestModel::TestAnim_f,			CMD_FL_GAME | CMD_FL_CHEAT,	"tests an animation", idTestModel::ArgCompletion_TestAnim );
	cmdSystem->AddCommand( "testParticleStopTime",	idTestModel::TestParticleStopTime_f, CMD_FL_GAME | CMD_FL_CHEAT,	"tests particle stop time on a test model" );
	cmdSystem->AddCommand( "nextAnim",				idTestModel::TestModelNextAnim_f,	CMD_FL_GAME | CMD_FL_CHEAT,	"shows next animation on test model" );
	cmdSystem->AddCommand( "prevAnim",				idTestModel::TestModelPrevAnim_f,	CMD_FL_GAME | CMD_FL_CHEAT,	"shows previous animation on test model" );
	cmdSystem->AddCommand( "nextFrame",				idTestModel::TestModelNextFrame_f,	CMD_FL_GAME | CMD_FL_CHEAT,	"shows next animation frame on test model" );
	cmdSystem->AddCommand( "prevFrame",				idTestModel::TestModelPrevFrame_f,	CMD_FL_GAME | CMD_FL_CHEAT,	"shows previous animation frame on test model" );
	cmdSystem->AddCommand( "testBlend",				idTestModel::TestBlend_f,			CMD_FL_GAME | CMD_FL_CHEAT,	"tests animation blending" );
	cmdSystem->AddCommand( "reloadScript",			Cmd_ReloadScript_f,			CMD_FL_GAME | CMD_FL_CHEAT,	"reloads scripts" );
	cmdSystem->AddCommand( "script",				Cmd_Script_f,				CMD_FL_GAME | CMD_FL_CHEAT,	"executes a line of script" );
	cmdSystem->AddCommand( "listCollisionModels",	Cmd_ListCollisionModels_f,	CMD_FL_GAME,				"lists collision models" );
	cmdSystem->AddCommand( "collisionModelInfo",	Cmd_CollisionModelInfo_f,	CMD_FL_GAME,				"shows collision model info" );
	cmdSystem->AddCommand( "reloadanims",			Cmd_ReloadAnims_f,			CMD_FL_GAME | CMD_FL_CHEAT,	"reloads animations" );
	cmdSystem->AddCommand( "listAnims",				Cmd_ListAnims_f,			CMD_FL_GAME,				"lists all animations" );
	cmdSystem->AddCommand( "aasStats",				Cmd_AASStats_f,				CMD_FL_GAME,				"shows AAS stats" );
	cmdSystem->AddCommand( "testDamage",			Cmd_TestDamage_f,			CMD_FL_GAME | CMD_FL_CHEAT,	"tests a damage def", idCmdSystem::ArgCompletion_Decl<DECL_ENTITYDEF> );
	cmdSystem->AddCommand( "weaponSplat",			Cmd_WeaponSplat_f,			CMD_FL_GAME | CMD_FL_CHEAT,	"projects a blood splat on the player weapon" );
	cmdSystem->AddCommand( "saveSelected",			Cmd_SaveSelected_f,			CMD_FL_GAME | CMD_FL_CHEAT,	"saves the selected entity to the .map file" );
	cmdSystem->AddCommand( "deleteSelected",		Cmd_DeleteSelected_f,		CMD_FL_GAME | CMD_FL_CHEAT,	"deletes selected entity" );
	cmdSystem->AddCommand( "saveMoveables",			Cmd_SaveMoveables_f,		CMD_FL_GAME | CMD_FL_CHEAT,	"save all moveables to the .map file" );
	cmdSystem->AddCommand( "saveRagdolls",			Cmd_SaveRagdolls_f,			CMD_FL_GAME | CMD_FL_CHEAT,	"save all ragdoll poses to the .map file" );
	cmdSystem->AddCommand( "bindRagdoll",			Cmd_BindRagdoll_f,			CMD_FL_GAME | CMD_FL_CHEAT,	"binds ragdoll at the current drag position" );
	cmdSystem->AddCommand( "unbindRagdoll",			Cmd_UnbindRagdoll_f,		CMD_FL_GAME | CMD_FL_CHEAT,	"unbinds the selected ragdoll" );
	cmdSystem->AddCommand( "saveLights",			Cmd_SaveLights_f,			CMD_FL_GAME | CMD_FL_CHEAT,	"saves all lights to the .map file" );
	cmdSystem->AddCommand( "saveParticles",			Cmd_SaveParticles_f,		CMD_FL_GAME | CMD_FL_CHEAT,	"saves all lights to the .map file" );
	cmdSystem->AddCommand( "clearLights",			Cmd_ClearLights_f,			CMD_FL_GAME | CMD_FL_CHEAT,	"clears all lights" );
	cmdSystem->AddCommand( "gameError",				Cmd_GameError_f,			CMD_FL_GAME | CMD_FL_CHEAT,	"causes a game error" );

	cmdSystem->AddCommand( "disasmScript",			Cmd_DisasmScript_f,			CMD_FL_GAME | CMD_FL_CHEAT,	"disassembles script" );
	cmdSystem->AddCommand( "recordViewNotes",		Cmd_RecordViewNotes_f,		CMD_FL_GAME | CMD_FL_CHEAT,	"record the current view position with notes" );
	cmdSystem->AddCommand( "showViewNotes",			Cmd_ShowViewNotes_f,		CMD_FL_GAME | CMD_FL_CHEAT,	"show any view notes for the current map, successive calls will cycle to the next note" );
	cmdSystem->AddCommand( "closeViewNotes",		Cmd_CloseViewNotes_f,		CMD_FL_GAME | CMD_FL_CHEAT,	"close the view showing any notes for this map" );

	// RB begin
	cmdSystem->AddCommand( "exportScriptEvents",	idClass::ExportScriptEvents_f,	CMD_FL_GAME | CMD_FL_TOOL,	"update script/doom_events.script" );
	cmdSystem->AddCommand( "editLights",			Cmd_EditLights_f,			CMD_FL_GAME | CMD_FL_TOOL,	"launches the in-game Light Editor" );
	cmdSystem->AddCommand( "saveEnvprobes",			Cmd_SaveEnvprobes_f,		CMD_FL_GAME | CMD_FL_CHEAT,	"saves all autogenerated env_probes to a .extras_env_probes.map file" );
	// RB end

	// SP Begin
	cmdSystem->AddCommand( "editAFs",				Cmd_ShowAfEditor_f,			CMD_FL_GAME | CMD_FL_TOOL, "launches the in-game Articulated Figure Editor" );
	// SP end

	// multiplayer client commands ( replaces old impulses stuff )
	//cmdSystem->AddCommand( "clientDropWeapon",		idMultiplayerGame::DropWeapon_f, CMD_FL_GAME,			"drop current weapon" );
	cmdSystem->AddCommand( "clientMessageMode",		idMultiplayerGame::MessageMode_f, CMD_FL_GAME,			"ingame gui message mode" );
	// FIXME: implement
//	cmdSystem->AddCommand( "clientVote",			idMultiplayerGame::Vote_f,	CMD_FL_GAME,				"cast your vote: clientVote yes | no" );
//	cmdSystem->AddCommand( "clientCallVote",		idMultiplayerGame::CallVote_f,	CMD_FL_GAME,			"call a vote: clientCallVote si_.. proposed_value" );
	cmdSystem->AddCommand( "clientVoiceChat",		idMultiplayerGame::VoiceChat_f,	CMD_FL_GAME,			"voice chats: clientVoiceChat <sound shader>" );
	cmdSystem->AddCommand( "clientVoiceChatTeam",	idMultiplayerGame::VoiceChatTeam_f,	CMD_FL_GAME,		"team voice chats: clientVoiceChat <sound shader>" );

	// multiplayer server commands
	cmdSystem->AddCommand( "serverMapRestart",		idGameLocal::MapRestart_f,	CMD_FL_GAME,				"restart the current game" );

	// localization help commands
	cmdSystem->AddCommand( "nextGUI",				Cmd_NextGUI_f,				CMD_FL_GAME | CMD_FL_CHEAT,	"teleport the player to the next func_static with a gui" );
	cmdSystem->AddCommand( "testid",				Cmd_TestId_f,				CMD_FL_GAME | CMD_FL_CHEAT,	"output the string for the specified id." );

	cmdSystem->AddCommand( "setActorState",			Cmd_SetActorState_f,		CMD_FL_GAME | CMD_FL_CHEAT,	"Manually sets an actors script state", idGameLocal::ArgCompletion_EntityName );
}

/*
=================
idGameLocal::ShutdownConsoleCommands
=================
*/
void idGameLocal::ShutdownConsoleCommands()
{
	cmdSystem->RemoveFlaggedCommands( CMD_FL_GAME );
}
