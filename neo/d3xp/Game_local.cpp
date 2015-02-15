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

#ifdef GAME_DLL

idSys* 						sys = NULL;
idCommon* 					common = NULL;
idCmdSystem* 				cmdSystem = NULL;
idCVarSystem* 				cvarSystem = NULL;
idFileSystem* 				fileSystem = NULL;
idRenderSystem* 			renderSystem = NULL;
idSoundSystem* 				soundSystem = NULL;
idRenderModelManager* 		renderModelManager = NULL;
idUserInterfaceManager* 	uiManager = NULL;
idDeclManager* 				declManager = NULL;
idAASFileManager* 			AASFileManager = NULL;
idCollisionModelManager* 	collisionModelManager = NULL;
idCVar* 					idCVar::staticVars = NULL;

idCVar com_forceGenericSIMD( "com_forceGenericSIMD", "0", CVAR_BOOL | CVAR_SYSTEM, "force generic platform independent SIMD" );

#endif

idRenderWorld* 				gameRenderWorld = NULL;		// all drawing is done to this world
idSoundWorld* 				gameSoundWorld = NULL;		// all audio goes to this world

static gameExport_t			gameExport;

// global animation lib
idAnimManager				animationLib;

// the rest of the engine will only reference the "game" variable, while all local aspects stay hidden
idGameLocal					gameLocal;
idGame* 					game = &gameLocal;	// statically pointed at an idGameLocal

const char* idGameLocal::sufaceTypeNames[ MAX_SURFACE_TYPES ] =
{
	"none",	"metal", "stone", "flesh", "wood", "cardboard", "liquid", "glass", "plastic",
	"ricochet", "surftype10", "surftype11", "surftype12", "surftype13", "surftype14", "surftype15"
};

idCVar net_usercmd_timing_debug( "net_usercmd_timing_debug", "0", CVAR_BOOL, "Print messages about usercmd timing." );


// List of all defs used by the player that will stay on the fast timeline
static const char* fastEntityList[] =
{
	"player_doommarine",
	"weapon_chainsaw",
	"weapon_fists",
	"weapon_flashlight",
	"weapon_rocketlauncher",
	"projectile_rocket",
	"weapon_machinegun",
	"projectile_bullet_machinegun",
	"weapon_pistol",
	"projectile_bullet_pistol",
	"weapon_handgrenade",
	"projectile_grenade",
	"weapon_bfg",
	"projectile_bfg",
	"weapon_chaingun",
	"projectile_chaingunbullet",
	"weapon_pda",
	"weapon_plasmagun",
	"projectile_plasmablast",
	"weapon_shotgun",
	"projectile_bullet_shotgun",
	"weapon_soulcube",
	"projectile_soulblast",
	"weapon_shotgun_double",
	"projectile_shotgunbullet_double",
	"weapon_grabber",
	"weapon_bloodstone_active1",
	"weapon_bloodstone_active2",
	"weapon_bloodstone_active3",
	"weapon_bloodstone_passive",
	NULL
};
/*
===========
GetGameAPI
============
*/
#if __MWERKS__
#pragma export on
#endif
#if __GNUC__ >= 4
#pragma GCC visibility push(default)
#endif
extern "C" gameExport_t* GetGameAPI( gameImport_t* import )
{
#if __MWERKS__
#pragma export off
#endif

	if( import->version == GAME_API_VERSION )
	{
	
		// set interface pointers used by the game
		sys							= import->sys;
		common						= import->common;
		cmdSystem					= import->cmdSystem;
		cvarSystem					= import->cvarSystem;
		fileSystem					= import->fileSystem;
		renderSystem				= import->renderSystem;
		soundSystem					= import->soundSystem;
		renderModelManager			= import->renderModelManager;
		uiManager					= import->uiManager;
		declManager					= import->declManager;
		AASFileManager				= import->AASFileManager;
		collisionModelManager		= import->collisionModelManager;
	}
	
	// set interface pointers used by idLib
	idLib::sys					= sys;
	idLib::common				= common;
	idLib::cvarSystem			= cvarSystem;
	idLib::fileSystem			= fileSystem;
	
	// setup export interface
	gameExport.version = GAME_API_VERSION;
	gameExport.game = game;
	gameExport.gameEdit = gameEdit;
	
	return &gameExport;
}
#if __GNUC__ >= 4
#pragma GCC visibility pop
#endif

/*
===========
TestGameAPI
============
*/
void TestGameAPI()
{
	gameImport_t testImport;
	gameExport_t testExport;
	
	testImport.sys						= ::sys;
	testImport.common					= ::common;
	testImport.cmdSystem				= ::cmdSystem;
	testImport.cvarSystem				= ::cvarSystem;
	testImport.fileSystem				= ::fileSystem;
	testImport.renderSystem				= ::renderSystem;
	testImport.soundSystem				= ::soundSystem;
	testImport.renderModelManager		= ::renderModelManager;
	testImport.uiManager				= ::uiManager;
	testImport.declManager				= ::declManager;
	testImport.AASFileManager			= ::AASFileManager;
	testImport.collisionModelManager	= ::collisionModelManager;
	
	testExport = *GetGameAPI( &testImport );
}

/*
===========
idGameLocal::idGameLocal
============
*/
idGameLocal::idGameLocal()
{
	Clear();
}

/*
===========
idGameLocal::Clear
============
*/
void idGameLocal::Clear()
{
	int i;
	
	serverInfo.Clear();
	numClients = 0;
	for( i = 0; i < MAX_CLIENTS; i++ )
	{
		persistentPlayerInfo[i].Clear();
	}
	memset( entities, 0, sizeof( entities ) );
	memset( spawnIds, -1, sizeof( spawnIds ) );
	firstFreeEntityIndex[0] = 0;
	firstFreeEntityIndex[1] = ENTITYNUM_FIRST_NON_REPLICATED;
	num_entities = 0;
	spawnedEntities.Clear();
	activeEntities.Clear();
	numEntitiesToDeactivate = 0;
	sortPushers = false;
	sortTeamMasters = false;
	persistentLevelInfo.Clear();
	memset( globalShaderParms, 0, sizeof( globalShaderParms ) );
	random.SetSeed( 0 );
	world = NULL;
	frameCommandThread = NULL;
	testmodel = NULL;
	testFx = NULL;
	clip.Shutdown();
	pvs.Shutdown();
	sessionCommand.Clear();
	locationEntities = NULL;
	smokeParticles = NULL;
	editEntities = NULL;
	entityHash.Clear( 1024, MAX_GENTITIES );
	cinematicSkipTime = 0;
	cinematicStopTime = 0;
	cinematicMaxSkipTime = 0;
	inCinematic = false;
	framenum = 0;
	previousTime = 0;
	time = 0;
	vacuumAreaNum = 0;
	mapFileName.Clear();
	mapFile = NULL;
	spawnCount = INITIAL_SPAWN_COUNT;
	mapSpawnCount = 0;
	camera = NULL;
	aasList.Clear();
	aasNames.Clear();
	lastAIAlertEntity = NULL;
	lastAIAlertTime = 0;
	spawnArgs.Clear();
	gravity.Set( 0, 0, -1 );
	playerPVS.h = ( unsigned int ) - 1;
	playerConnectedAreas.h = ( unsigned int ) - 1;
	gamestate = GAMESTATE_UNINITIALIZED;
	skipCinematic = false;
	influenceActive = false;
	
	realClientTime = 0;
	isNewFrame = true;
	clientSmoothing = 0.1f;
	entityDefBits = 0;
	
	nextGibTime = 0;
	globalMaterial = NULL;
	newInfo.Clear();
	lastGUIEnt = NULL;
	lastGUI = 0;
	
	eventQueue.Init();
	savedEventQueue.Init();
	
	shellHandler = NULL;
	selectedGroup = 0;
	portalSkyEnt			= NULL;
	portalSkyActive			= false;
	
	ResetSlowTimeVars();
	
	lastCmdRunTimeOnClient.Zero();
	lastCmdRunTimeOnServer.Zero();
}

/*
===========
idGameLocal::Init

  initialize the game object, only happens once at startup, not each level load
============
*/
void idGameLocal::Init()
{
	const idDict* dict;
	idAAS* aas;
	
#ifndef GAME_DLL
	
	TestGameAPI();
	
#else
	
	// initialize idLib
	idLib::Init();
	
	// register static cvars declared in the game
	idCVar::RegisterStaticVars();
	
	// initialize processor specific SIMD
	idSIMD::InitProcessor( "game", com_forceGenericSIMD.GetBool() );
	
#endif
	
	Printf( "--------- Initializing Game ----------\n" );
	Printf( "gamename: %s\n", GAME_VERSION );
	Printf( "gamedate: %s\n", __DATE__ );
	
	// register game specific decl types
	declManager->RegisterDeclType( "model",				DECL_MODELDEF,		idDeclAllocator<idDeclModelDef> );
	declManager->RegisterDeclType( "export",			DECL_MODELEXPORT,	idDeclAllocator<idDecl> );
	
	// register game specific decl folders
	declManager->RegisterDeclFolder( "def",				".def",				DECL_ENTITYDEF );
	declManager->RegisterDeclFolder( "fx",				".fx",				DECL_FX );
	declManager->RegisterDeclFolder( "particles",		".prt",				DECL_PARTICLE );
	declManager->RegisterDeclFolder( "af",				".af",				DECL_AF );
	declManager->RegisterDeclFolder( "newpdas",			".pda",				DECL_PDA );
	
	cmdSystem->AddCommand( "listModelDefs", idListDecls_f<DECL_MODELDEF>, CMD_FL_SYSTEM | CMD_FL_GAME, "lists model defs" );
	cmdSystem->AddCommand( "printModelDefs", idPrintDecls_f<DECL_MODELDEF>, CMD_FL_SYSTEM | CMD_FL_GAME, "prints a model def", idCmdSystem::ArgCompletion_Decl<DECL_MODELDEF> );
	
	Clear();
	
	idEvent::Init();
	idClass::Init();
	
	InitConsoleCommands();
	
	shellHandler = new( TAG_SWF ) idMenuHandler_Shell();
	
	if( !g_xp_bind_run_once.GetBool() )
	{
		//The default config file contains remapped controls that support the XP weapons
		//We want to run this once after the base doom config file has run so we can
		//have the correct xp binds
		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "exec default.cfg\n" );
		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "seta g_xp_bind_run_once 1\n" );
		cmdSystem->ExecuteCommandBuffer();
	}
	
	// load default scripts
	program.Startup( SCRIPT_DEFAULT );
	
	smokeParticles = new( TAG_PARTICLE ) idSmokeParticles;
	
	// set up the aas
	dict = FindEntityDefDict( "aas_types" );
	if( dict == NULL )
	{
		Error( "Unable to find entityDef for 'aas_types'" );
		return;
	}
	
	// allocate space for the aas
	const idKeyValue* kv = dict->MatchPrefix( "type" );
	while( kv != NULL )
	{
		aas = idAAS::Alloc();
		aasList.Append( aas );
		aasNames.Append( kv->GetValue() );
		kv = dict->MatchPrefix( "type", kv );
	}
	
	gamestate = GAMESTATE_NOMAP;
	
	Printf( "...%d aas types\n", aasList.Num() );
	Printf( "game initialized.\n" );
	Printf( "--------------------------------------\n" );
	
}

/*
===========
idGameLocal::Shutdown

  shut down the entire game
============
*/
void idGameLocal::Shutdown()
{

	if( !common )
	{
		return;
	}
	
	Printf( "------------ Game Shutdown -----------\n" );
	
	Shell_Cleanup();
	
	mpGame.Shutdown();
	
	MapShutdown();
	
	aasList.DeleteContents( true );
	aasNames.Clear();
	
	idAI::FreeObstacleAvoidanceNodes();
	
	idEvent::Shutdown();
	
	delete[] locationEntities;
	locationEntities = NULL;
	
	delete smokeParticles;
	smokeParticles = NULL;
	
	idClass::Shutdown();
	
	// clear list with forces
	idForce::ClearForceList();
	
	// free the program data
	program.FreeData();
	
	// delete the .map file
	delete mapFile;
	mapFile = NULL;
	
	// free the collision map
	collisionModelManager->FreeMap();
	
	ShutdownConsoleCommands();
	
	// free memory allocated by class objects
	Clear();
	
	// shut down the animation manager
	animationLib.Shutdown();
	
	Printf( "--------------------------------------\n" );
	
#ifdef GAME_DLL
	
	// remove auto-completion function pointers pointing into this DLL
	cvarSystem->RemoveFlaggedAutoCompletion( CVAR_GAME );
	
	// enable leak test
	Mem_EnableLeakTest( "game" );
	
	// shutdown idLib
	idLib::ShutDown();
	
#endif
}

idCVar g_recordSaveGameTrace( "g_recordSaveGameTrace", "0", CVAR_BOOL, "" );

/*
===========
idGameLocal::SaveGame

save the current player state, level name, and level state
the session may have written some data to the file already
============
*/
void idGameLocal::SaveGame( idFile* f, idFile* strings )
{
	int i;
	idEntity* ent;
	idEntity* link;
	
	int startTimeMs = Sys_Milliseconds();
	if( g_recordSaveGameTrace.GetBool() )
	{
		bool result = BeginTraceRecording( "e:\\savegame_trace.pix2" );
		if( !result )
		{
			//idLib::Printf( "BeginTraceRecording: error %d\n", GetLastError() );
		}
	}
	
	idSaveGame savegame( f, strings, BUILD_NUMBER );
	
	if( g_flushSave.GetBool( ) == true )
	{
		// force flushing with each write... for tracking down
		// save game bugs.
		f->ForceFlush();
	}
	
	// go through all entities and threads and add them to the object list
	for( i = 0; i < MAX_GENTITIES; i++ )
	{
		ent = entities[i];
		
		if( ent )
		{
			if( ent->GetTeamMaster() && ent->GetTeamMaster() != ent )
			{
				continue;
			}
			for( link = ent; link != NULL; link = link->GetNextTeamEntity() )
			{
				savegame.AddObject( link );
			}
		}
	}
	
	idList<idThread*> threads;
	threads = idThread::GetThreads();
	
	for( i = 0; i < threads.Num(); i++ )
	{
		savegame.AddObject( threads[i] );
	}
	
	// write out complete object list
	savegame.WriteObjectList();
	
	program.Save( &savegame );
	
	savegame.WriteInt( g_skill.GetInteger() );
	
	savegame.WriteDecls();
	
	savegame.WriteDict( &serverInfo );
	
	savegame.WriteInt( numClients );
	for( i = 0; i < numClients; i++ )
	{
		//savegame.WriteUsercmd( usercmds[ i ] );
		// Now that usercmds are handled by the idUserCmdMgr,
		// do we need another solution here?
		usercmd_t dummy;
		savegame.WriteUsercmd( dummy );
		
		savegame.WriteDict( &persistentPlayerInfo[ i ] );
	}
	
	for( i = 0; i < MAX_GENTITIES; i++ )
	{
		savegame.WriteObject( entities[ i ] );
		savegame.WriteInt( spawnIds[ i ] );
	}
	
	// There shouldn't be any non-replicated entities in SP,
	// so we shouldn't have to save the first free replicated entity index.
	savegame.WriteInt( firstFreeEntityIndex[0] );
	savegame.WriteInt( num_entities );
	
	// enityHash is restored by idEntity::Restore setting the entity name.
	
	savegame.WriteObject( world );
	
	savegame.WriteInt( spawnedEntities.Num() );
	for( ent = spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() )
	{
		savegame.WriteObject( ent );
	}
	
	savegame.WriteInt( activeEntities.Num() );
	for( ent = activeEntities.Next(); ent != NULL; ent = ent->activeNode.Next() )
	{
		savegame.WriteObject( ent );
	}
	
	savegame.WriteInt( numEntitiesToDeactivate );
	savegame.WriteBool( sortPushers );
	savegame.WriteBool( sortTeamMasters );
	savegame.WriteDict( &persistentLevelInfo );
	
	for( i = 0; i < MAX_GLOBAL_SHADER_PARMS; i++ )
	{
		savegame.WriteFloat( globalShaderParms[ i ] );
	}
	
	savegame.WriteInt( random.GetSeed() );
	savegame.WriteObject( frameCommandThread );
	
	// clip
	// push
	// pvs
	
	testmodel = NULL;
	testFx = NULL;
	
	savegame.WriteString( sessionCommand );
	
	// FIXME: save smoke particles
	
	savegame.WriteInt( cinematicSkipTime );
	savegame.WriteInt( cinematicStopTime );
	savegame.WriteInt( cinematicMaxSkipTime );
	savegame.WriteBool( inCinematic );
	savegame.WriteBool( skipCinematic );
	
	savegame.WriteInt( gameType );
	
	savegame.WriteInt( framenum );
	savegame.WriteInt( previousTime );
	savegame.WriteInt( time );
	
	savegame.WriteInt( vacuumAreaNum );
	
	savegame.WriteInt( entityDefBits );
	
	savegame.WriteInt( GetLocalClientNum() );
	
	// snapshotEntities is used for multiplayer only
	
	savegame.WriteInt( realClientTime );
	savegame.WriteBool( isNewFrame );
	savegame.WriteFloat( clientSmoothing );
	
	portalSkyEnt.Save( &savegame );
	savegame.WriteBool( portalSkyActive );
	
	fast.Save( &savegame );
	slow.Save( &savegame );
	
	savegame.WriteInt( slowmoState );
	savegame.WriteFloat( slowmoScale );
	savegame.WriteBool( quickSlowmoReset );
	
	savegame.WriteBool( mapCycleLoaded );
	savegame.WriteInt( spawnCount );
	
	if( !locationEntities )
	{
		savegame.WriteInt( 0 );
	}
	else
	{
		savegame.WriteInt( gameRenderWorld->NumAreas() );
		for( i = 0; i < gameRenderWorld->NumAreas(); i++ )
		{
			savegame.WriteObject( locationEntities[ i ] );
		}
	}
	
	savegame.WriteObject( camera );
	
	savegame.WriteMaterial( globalMaterial );
	
	lastAIAlertEntity.Save( &savegame );
	savegame.WriteInt( lastAIAlertTime );
	
	savegame.WriteDict( &spawnArgs );
	
	savegame.WriteInt( playerPVS.i );
	savegame.WriteInt( playerPVS.h );
	savegame.WriteInt( playerConnectedAreas.i );
	savegame.WriteInt( playerConnectedAreas.h );
	
	savegame.WriteVec3( gravity );
	
	// gamestate
	
	savegame.WriteBool( influenceActive );
	savegame.WriteInt( nextGibTime );
	
	// spawnSpots
	// initialSpots
	// currentInitialSpot
	// newInfo
	// makingBuild
	// shakeSounds
	
	// write out pending events
	idEvent::Save( &savegame );
	
	savegame.Close();
	
	int endTimeMs = Sys_Milliseconds();
	idLib::Printf( "Save time: %dms\n", ( endTimeMs - startTimeMs ) );
	
	if( g_recordSaveGameTrace.GetBool() )
	{
		EndTraceRecording();
		g_recordSaveGameTrace.SetBool( false );
	}
}

/*
===========
idGameLocal::GetSaveGameDetails
============
*/
void idGameLocal::GetSaveGameDetails( idSaveGameDetails& gameDetails )
{
	idLocationEntity* locationEnt = LocationForPoint( gameLocal.GetLocalPlayer()->GetEyePosition() );
	const char* locationStr = locationEnt ? locationEnt->GetLocation() : idLocalization::GetString( "#str_02911" );
	
	idStrStatic< MAX_OSPATH > shortMapName = mapFileName;
	shortMapName.StripFileExtension();
	shortMapName.StripLeading( "maps/" );
	
	const idDeclEntityDef* mapDef = static_cast<const idDeclEntityDef*>( declManager->FindType( DECL_MAPDEF, shortMapName, false ) );
	const char* mapPrettyName = mapDef ? idLocalization::GetString( mapDef->dict.GetString( "name", shortMapName ) ) : shortMapName.c_str();
	idPlayer* player = GetClientByNum( 0 );
	int playTime = player ? player->GetPlayedTime() : 0;
	gameExpansionType_t expansionType = player ? player->GetExpansionType() : GAME_BASE;
	
	gameDetails.descriptors.Clear();
	gameDetails.descriptors.SetInt( SAVEGAME_DETAIL_FIELD_EXPANSION, expansionType );
	gameDetails.descriptors.Set( SAVEGAME_DETAIL_FIELD_MAP, mapPrettyName );
	gameDetails.descriptors.Set( SAVEGAME_DETAIL_FIELD_MAP_LOCATE, locationStr );
	gameDetails.descriptors.SetInt( SAVEGAME_DETAIL_FIELD_SAVE_VERSION, BUILD_NUMBER );
	gameDetails.descriptors.SetInt( SAVEGAME_DETAIL_FIELD_DIFFICULTY, g_skill.GetInteger() );
	gameDetails.descriptors.SetInt( SAVEGAME_DETAIL_FIELD_PLAYTIME, playTime );
	
	// PS3 only strings that use the dict just set
	
	// even though we don't use this when we enumerate, when we save, we use this descriptors file later so we need the date populated now
	gameDetails.date = ::time( NULL );
}

/*
===========
idGameLocal::GetPersistentPlayerInfo
============
*/
const idDict& idGameLocal::GetPersistentPlayerInfo( int clientNum )
{
	idEntity*	ent;
	
	persistentPlayerInfo[ clientNum ].Clear();
	ent = entities[ clientNum ];
	if( ent && ent->IsType( idPlayer::Type ) )
	{
		static_cast<idPlayer*>( ent )->SavePersistantInfo();
	}
	
	return persistentPlayerInfo[ clientNum ];
}

/*
===========
idGameLocal::SetPersistentPlayerInfo
============
*/
void idGameLocal::SetPersistentPlayerInfo( int clientNum, const idDict& playerInfo )
{
	persistentPlayerInfo[ clientNum ] = playerInfo;
}

/*
============
idGameLocal::Printf
============
*/
void idGameLocal::Printf( const char* fmt, ... ) const // DG: FIXME: printf-annotation
{
	va_list		argptr;
	char		text[MAX_STRING_CHARS];
	
	va_start( argptr, fmt );
	idStr::vsnPrintf( text, sizeof( text ), fmt, argptr );
	va_end( argptr );
	
	common->Printf( "%s", text );
}

/*
============
idGameLocal::DPrintf
============
*/
void idGameLocal::DPrintf( const char* fmt, ... ) const
{
	va_list		argptr;
	char		text[MAX_STRING_CHARS];
	
	if( !developer.GetBool() )
	{
		return;
	}
	
	va_start( argptr, fmt );
	idStr::vsnPrintf( text, sizeof( text ), fmt, argptr );
	va_end( argptr );
	
	common->Printf( "%s", text );
}

/*
============
idGameLocal::Warning
============
*/
void idGameLocal::Warning( const char* fmt, ... ) const
{
	va_list		argptr;
	char		text[MAX_STRING_CHARS];
	idThread* 	thread;
	
	va_start( argptr, fmt );
	idStr::vsnPrintf( text, sizeof( text ), fmt, argptr );
	va_end( argptr );
	
	thread = idThread::CurrentThread();
	if( thread )
	{
		thread->Warning( "%s", text );
	}
	else
	{
		common->Warning( "%s", text );
	}
}

/*
============
idGameLocal::DWarning
============
*/
void idGameLocal::DWarning( const char* fmt, ... ) const
{
	va_list		argptr;
	char		text[MAX_STRING_CHARS];
	idThread* 	thread;
	
	if( !developer.GetBool() )
	{
		return;
	}
	
	va_start( argptr, fmt );
	idStr::vsnPrintf( text, sizeof( text ), fmt, argptr );
	va_end( argptr );
	
	thread = idThread::CurrentThread();
	if( thread )
	{
		thread->Warning( "%s", text );
	}
	else
	{
		common->DWarning( "%s", text );
	}
}

/*
============
idGameLocal::Error
============
*/
void idGameLocal::Error( const char* fmt, ... ) const
{
	va_list		argptr;
	char		text[MAX_STRING_CHARS];
	idThread* 	thread;
	
	va_start( argptr, fmt );
	idStr::vsnPrintf( text, sizeof( text ), fmt, argptr );
	va_end( argptr );
	
	thread = idThread::CurrentThread();
	if( thread )
	{
		thread->Error( "%s", text );
	}
	else
	{
		common->Error( "%s", text );
	}
}

/*
===============
gameError
===============
*/
void gameError( const char* fmt, ... )
{
	va_list		argptr;
	char		text[MAX_STRING_CHARS];
	
	va_start( argptr, fmt );
	idStr::vsnPrintf( text, sizeof( text ), fmt, argptr );
	va_end( argptr );
	
	gameLocal.Error( "%s", text );
}

/*
========================
idGameLocal::SetServerGameTimeMs
========================
*/
void idGameLocal::SetServerGameTimeMs( const int time )
{
	previousServerTime = this->serverTime;
	this->serverTime = time;
}

/*
========================
idGameLocal::GetServerGameTimeMs
========================
*/
int idGameLocal::GetServerGameTimeMs() const
{
	return serverTime;
}

/*
===========
idGameLocal::SetServerInfo
============
*/
void idGameLocal::SetServerInfo( const idDict& _serverInfo )
{
	serverInfo = _serverInfo;
	
	if( gameType == GAME_LASTMAN )
	{
		if( serverInfo.GetInt( "si_fraglimit" ) <= 0 )
		{
			common->Warning( "Last Man Standing - setting fraglimit 1" );
			serverInfo.SetInt( "si_fraglimit", 1 );
		}
	}
}

/*
===========
idGameLocal::GetServerInfo
============
*/
const idDict& idGameLocal::GetServerInfo()
{
	return serverInfo;
}

/*
===================
idGameLocal::LoadMap

Initializes all map variables common to both save games and spawned games.
===================
*/
void idGameLocal::LoadMap( const char* mapName, int randseed )
{
	bool sameMap = ( mapFile && idStr::Icmp( mapFileName, mapName ) == 0 );
	
	// clear the sound system
	gameSoundWorld->ClearAllSoundEmitters();
	
	// clear envirosuit sound fx
	gameSoundWorld->SetEnviroSuit( false );
	gameSoundWorld->SetSlowmoSpeed( 1.0f );
	
	InitAsyncNetwork();
	
	if( !sameMap || ( mapFile && mapFile->NeedsReload() ) )
	{
		// load the .map file
		if( mapFile )
		{
			delete mapFile;
		}
		mapFile = new( TAG_GAME ) idMapFile;
		if( !mapFile->Parse( idStr( mapName ) + ".map" ) )
		{
			delete mapFile;
			mapFile = NULL;
			Error( "Couldn't load %s", mapName );
		}
	}
	mapFileName = mapFile->GetName();
	
	// load the collision map
	collisionModelManager->LoadMap( mapFile );
	collisionModelManager->Preload( mapName );
	
	numClients = 0;
	
	// initialize all entities for this game
	memset( entities, 0, sizeof( entities ) );
	memset( spawnIds, -1, sizeof( spawnIds ) );
	spawnCount = INITIAL_SPAWN_COUNT;
	
	spawnedEntities.Clear();
	activeEntities.Clear();
	aimAssistEntities.Clear();
	numEntitiesToDeactivate = 0;
	sortTeamMasters = false;
	sortPushers = false;
	lastGUIEnt = NULL;
	lastGUI = 0;
	
	globalMaterial = NULL;
	
	memset( globalShaderParms, 0, sizeof( globalShaderParms ) );
	
	// These used to be a non-pot adjustment for portal skies
	// they're no longer needed, but we can't update the materials
	globalShaderParms[4] = 1.0f;
	globalShaderParms[5] = 1.0f;
	
	// always leave room for the max number of clients,
	// even if they aren't all used, so numbers inside that
	// range are NEVER anything but clients
	num_entities			= MAX_CLIENTS;
	firstFreeEntityIndex[0]	= MAX_CLIENTS;
	
	// reset the random number generator.
	random.SetSeed( common->IsMultiplayer() ? randseed : 0 );
	
	camera			= NULL;
	world			= NULL;
	testmodel		= NULL;
	testFx			= NULL;
	
	lastAIAlertEntity = NULL;
	lastAIAlertTime = 0;
	
	previousTime	= 0;
	time			= 0;
	framenum		= 0;
	sessionCommand = "";
	nextGibTime		= 0;
	
	portalSkyEnt			= NULL;
	portalSkyActive			= false;
	
	ResetSlowTimeVars();
	
	vacuumAreaNum = -1;		// if an info_vacuum is spawned, it will set this
	
	if( !editEntities )
	{
		editEntities = new( TAG_GAME ) idEditEntities;
	}
	
	gravity.Set( 0, 0, -g_gravity.GetFloat() );
	
	spawnArgs.Clear();
	
	inCinematic = false;
	skipCinematic = false;
	cinematicSkipTime = 0;
	cinematicStopTime = 0;
	cinematicMaxSkipTime = 0;
	
	clip.Init();
	
	common->UpdateLevelLoadPacifier();
	
	pvs.Init();
	
	common->UpdateLevelLoadPacifier();
	
	playerPVS.i = -1;
	playerConnectedAreas.i = -1;
	
	// load navigation system for all the different monster sizes
	for( int i = 0; i < aasNames.Num(); i++ )
	{
		aasList[ i ]->Init( idStr( mapFileName ).SetFileExtension( aasNames[ i ] ).c_str(), mapFile->GetGeometryCRC() );
	}
	
	// clear the smoke particle free list
	smokeParticles->Init();
	
	common->UpdateLevelLoadPacifier();
	
	// cache miscellaneous media references
	FindEntityDef( "preCacheExtras", false );
	FindEntityDef( "ammo_types", false );
	FindEntityDef( "ammo_names", false );
	FindEntityDef( "ammo_types_d3xp", false );
	
	FindEntityDef( "damage_noair", false );
	FindEntityDef( "damage_moverCrush", false );
	FindEntityDef( "damage_crush", false );
	FindEntityDef( "damage_triggerhurt_1000", false );
	FindEntityDef( "damage_telefrag", false );
	FindEntityDef( "damage_suicide", false );
	FindEntityDef( "damage_explosion", false );
	FindEntityDef( "damage_generic", false );
	FindEntityDef( "damage_painTrigger", false );
	FindEntityDef( "damage_thrown_ragdoll", false );
	FindEntityDef( "damage_gib", false );
	FindEntityDef( "damage_softfall", false );
	FindEntityDef( "damage_hardfall", false );
	FindEntityDef( "damage_fatalfall", false );
	
	FindEntityDef( "envirosuit_light", false );
	
	declManager->FindType( DECL_EMAIL, "highScore", false );
	declManager->FindType( DECL_EMAIL, "MartianBuddyGameComplete", false );
	
	declManager->FindMaterial( "itemHighlightShell" );
	
	common->UpdateLevelLoadPacifier();
	
	if( !sameMap )
	{
		mapFile->RemovePrimitiveData();
	}
}

/*
===================
idGameLocal::LocalMapRestart
===================
*/
void idGameLocal::LocalMapRestart( )
{
	int i, latchSpawnCount;
	
	Printf( "----------- Game Map Restart ------------\n" );
	
	gamestate = GAMESTATE_SHUTDOWN;
	
	for( i = 0; i < MAX_CLIENTS; i++ )
	{
		if( entities[ i ] && entities[ i ]->IsType( idPlayer::Type ) )
		{
			static_cast< idPlayer* >( entities[ i ] )->PrepareForRestart();
		}
	}
	
	eventQueue.Shutdown();
	savedEventQueue.Shutdown();
	
	MapClear( false );
	
	
	
	// clear the smoke particle free list
	smokeParticles->Init();
	
	// clear the sound system
	if( gameSoundWorld )
	{
		gameSoundWorld->ClearAllSoundEmitters();
		// clear envirosuit sound fx
		gameSoundWorld->SetEnviroSuit( false );
		gameSoundWorld->SetSlowmoSpeed( 1.0f );
	}
	
	// the spawnCount is reset to zero temporarily to spawn the map entities with the same spawnId
	// if we don't do that, network clients are confused and don't show any map entities
	latchSpawnCount = spawnCount;
	spawnCount = INITIAL_SPAWN_COUNT;
	
	gamestate = GAMESTATE_STARTUP;
	
	program.Restart();
	
	InitScriptForMap();
	
	MapPopulate();
	
	// once the map is populated, set the spawnCount back to where it was so we don't risk any collision
	// (note that if there are no players in the game, we could just leave it at it's current value)
	spawnCount = latchSpawnCount;
	
	// setup the client entities again
	for( i = 0; i < MAX_CLIENTS; i++ )
	{
		if( entities[ i ] && entities[ i ]->IsType( idPlayer::Type ) )
		{
			static_cast< idPlayer* >( entities[ i ] )->Restart();
		}
	}
	
	gamestate = GAMESTATE_ACTIVE;
	
	Printf( "--------------------------------------\n" );
}

/*
===================
idGameLocal::MapRestart
===================
*/
void idGameLocal::MapRestart()
{
	if( common->IsClient() )
	{
		LocalMapRestart();
	}
	else
	{
		idBitMsg msg;
		session->GetActingGameStateLobbyBase().SendReliable( GAME_RELIABLE_MESSAGE_RESTART, msg, false );
		
		LocalMapRestart();
		mpGame.MapRestart();
	}
	
	if( common->IsMultiplayer() )
	{
		gameLocal.mpGame.ReloadScoreboard();
	}
}

/*
===================
idGameLocal::MapRestart_f
===================
*/
void idGameLocal::MapRestart_f( const idCmdArgs& args )
{
	if( !common->IsMultiplayer() || common->IsClient() )
	{
		common->Printf( "server is not running - use spawnServer\n" );
		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "spawnServer\n" );
		return;
	}
	
	gameLocal.MapRestart( );
}

/*
===================
idGameLocal::MapPopulate
===================
*/
void idGameLocal::MapPopulate()
{

	if( common->IsMultiplayer() )
	{
		cvarSystem->SetCVarBool( "r_skipSpecular", false );
	}
	// parse the key/value pairs and spawn entities
	SpawnMapEntities();
	
	// mark location entities in all connected areas
	SpreadLocations();
	
	// prepare the list of randomized initial spawn spots
	RandomizeInitialSpawns();
	
	// spawnCount - 1 is the number of entities spawned into the map, their indexes started at MAX_CLIENTS (included)
	// mapSpawnCount is used as the max index of map entities, it's the first index of non-map entities
	mapSpawnCount = MAX_CLIENTS + spawnCount - 1;
	
	// execute pending events before the very first game frame
	// this makes sure the map script main() function is called
	// before the physics are run so entities can bind correctly
	Printf( "==== Processing events ====\n" );
	idEvent::ServiceEvents();
	
	// Must set GAME_FPS for script after populating, because some maps run their own scripts
	// when spawning the world, and GAME_FPS will not be found before then.
	SetScriptFPS( com_engineHz_latched );
}

/*
===================
idGameLocal::InitFromNewMap
===================
*/
void idGameLocal::InitFromNewMap( const char* mapName, idRenderWorld* renderWorld, idSoundWorld* soundWorld, int gameMode, int randseed )
{

	this->gameType = ( gameType_t )idMath::ClampInt( GAME_SP, GAME_COUNT - 1, gameMode );
	
	if( mapFileName.Length() )
	{
		MapShutdown();
	}
	
	Printf( "----------- Game Map Init ------------\n" );
	
	gamestate = GAMESTATE_STARTUP;
	
	gameRenderWorld = renderWorld;
	gameSoundWorld = soundWorld;
	
	if( common->IsMultiplayer() )
	{
		g_skill.SetInteger( 1 );
	}
	else
	{
		g_skill.SetInteger( idMath::ClampInt( 0, 3, g_skill.GetInteger() ) );
	}
	
	LoadMap( mapName, randseed );
	
	InitScriptForMap();
	
	MapPopulate();
	
	mpGame.Reset();
	mpGame.Precache();
	
	SyncPlayersWithLobbyUsers( true );
	
	// free up any unused animations
	animationLib.FlushUnusedAnims();
	
	gamestate = GAMESTATE_ACTIVE;
	
	Printf( "--------------------------------------\n" );
}

/*
=================
idGameLocal::InitFromSaveGame
=================
*/
bool idGameLocal::InitFromSaveGame( const char* mapName, idRenderWorld* renderWorld, idSoundWorld* soundWorld, idFile* saveGameFile, idFile* stringTableFile, int saveGameVersion )
{
	int i;
	int num;
	idEntity* ent;
	idDict si;
	
	if( mapFileName.Length() )
	{
		MapShutdown();
	}
	
	Printf( "------- Game Map Init SaveGame -------\n" );
	
	gamestate = GAMESTATE_STARTUP;
	
	gameRenderWorld = renderWorld;
	gameSoundWorld = soundWorld;
	
	SetScriptFPS( com_engineHz_latched );
	
	// load the map needed for this savegame
	LoadMap( mapName, 0 );
	
	idFile_SaveGamePipelined* pipelineFile = new( TAG_SAVEGAMES ) idFile_SaveGamePipelined();
	pipelineFile->OpenForReading( saveGameFile );
	idRestoreGame savegame( pipelineFile, stringTableFile, saveGameVersion );
	
	// Create the list of all objects in the game
	savegame.CreateObjects();
	
	// Load the idProgram, also checking to make sure scripting hasn't changed since the savegame
	if( program.Restore( &savegame ) == false )
	{
	
		// Abort the load process, and let the session know so that it can restart the level
		// with the player persistent data.
		savegame.DeleteObjects();
		program.Restart();
		return false;
	}
	
	savegame.ReadInt( i );
	g_skill.SetInteger( i );
	
	// precache any media specified in the map
	savegame.ReadDecls();
	
	savegame.ReadDict( &si );
	SetServerInfo( si );
	
	savegame.ReadInt( numClients );
	for( i = 0; i < numClients; i++ )
	{
		//savegame.ReadUsercmd( usercmds[ i ] );
		// Now that usercmds are handled by the idUserCmdMgr,
		// do we need another solution here?
		usercmd_t dummy;
		savegame.ReadUsercmd( dummy );
		savegame.ReadDict( &persistentPlayerInfo[ i ] );
	}
	
	for( i = 0; i < MAX_GENTITIES; i++ )
	{
		savegame.ReadObject( reinterpret_cast<idClass*&>( entities[ i ] ) );
		savegame.ReadInt( spawnIds[ i ] );
		
		// restore the entityNumber
		if( entities[ i ] != NULL )
		{
			entities[ i ]->entityNumber = i;
		}
	}
	
	// Connect players with lobby users
	// There should only be 1 player and 1 lobby user, but I'm using a loop just to be safe
	idLobbyBase& lobby = session->GetActingGameStateLobbyBase();
	int numLobbyUsers = lobby.GetNumLobbyUsers();
	int lobbyUserNum = 0;
	assert( numLobbyUsers == 1 );
	for( int i = 0; i < MAX_PLAYERS && lobbyUserNum < numLobbyUsers; i++ )
	{
		if( entities[i] == NULL )
		{
			continue;
		}
		lobbyUserIDs[i] = lobby.GetLobbyUserIdByOrdinal( lobbyUserNum++ );
	}
	
	savegame.ReadInt( firstFreeEntityIndex[0] );
	savegame.ReadInt( num_entities );
	
	// enityHash is restored by idEntity::Restore setting the entity name.
	
	savegame.ReadObject( reinterpret_cast<idClass*&>( world ) );
	
	savegame.ReadInt( num );
	for( i = 0; i < num; i++ )
	{
		savegame.ReadObject( reinterpret_cast<idClass*&>( ent ) );
		assert( ent );
		if( ent )
		{
			ent->spawnNode.AddToEnd( spawnedEntities );
		}
	}
	
	savegame.ReadInt( num );
	for( i = 0; i < num; i++ )
	{
		savegame.ReadObject( reinterpret_cast<idClass*&>( ent ) );
		assert( ent );
		if( ent )
		{
			ent->activeNode.AddToEnd( activeEntities );
		}
	}
	
	savegame.ReadInt( numEntitiesToDeactivate );
	savegame.ReadBool( sortPushers );
	savegame.ReadBool( sortTeamMasters );
	savegame.ReadDict( &persistentLevelInfo );
	
	for( i = 0; i < MAX_GLOBAL_SHADER_PARMS; i++ )
	{
		savegame.ReadFloat( globalShaderParms[ i ] );
	}
	
	savegame.ReadInt( i );
	random.SetSeed( i );
	
	savegame.ReadObject( reinterpret_cast<idClass*&>( frameCommandThread ) );
	
	// clip
	// push
	// pvs
	
	// testmodel = "<NULL>"
	// testFx = "<NULL>"
	
	savegame.ReadString( sessionCommand );
	
	// FIXME: save smoke particles
	
	if( saveGameVersion > BUILD_NUMBER_SAVE_VERSION_BEFORE_SKIP_CINEMATIC )
	{
		savegame.ReadInt( cinematicSkipTime );
		savegame.ReadInt( cinematicStopTime );
		savegame.ReadInt( cinematicMaxSkipTime );
	}
	
	savegame.ReadBool( inCinematic );
	
	if( saveGameVersion > BUILD_NUMBER_SAVE_VERSION_BEFORE_SKIP_CINEMATIC )
	{
		savegame.ReadBool( skipCinematic );
	}
	
	savegame.ReadInt( ( int& )gameType );
	
	savegame.ReadInt( framenum );
	savegame.ReadInt( previousTime );
	savegame.ReadInt( time );
	
	savegame.ReadInt( vacuumAreaNum );
	
	savegame.ReadInt( entityDefBits );
	
	// the localClientNum member of idGameLocal was removed,
	// but to preserve savegame compatibility, we still need
	// to read an int here even though it's not used.
	int dummyLocalClientNum = 0;
	savegame.ReadInt( dummyLocalClientNum );
	
	// snapshotEntities is used for multiplayer only
	
	savegame.ReadInt( realClientTime );
	savegame.ReadBool( isNewFrame );
	savegame.ReadFloat( clientSmoothing );
	
	portalSkyEnt.Restore( &savegame );
	savegame.ReadBool( portalSkyActive );
	
	fast.Restore( &savegame );
	slow.Restore( &savegame );
	
	framenum = MSEC_TO_FRAME_FLOOR( fast.time );
	
	int blah;
	savegame.ReadInt( blah );
	slowmoState = ( slowmoState_t )blah;
	
	savegame.ReadFloat( slowmoScale );
	savegame.ReadBool( quickSlowmoReset );
	
	if( gameSoundWorld )
	{
		gameSoundWorld->SetSlowmoSpeed( slowmoScale );
	}
	
	savegame.ReadBool( mapCycleLoaded );
	savegame.ReadInt( spawnCount );
	
	savegame.ReadInt( num );
	if( num )
	{
		if( num != gameRenderWorld->NumAreas() )
		{
			savegame.Error( "idGameLocal::InitFromSaveGame: number of areas in map differs from save game." );
		}
		
		locationEntities = new( TAG_GAME ) idLocationEntity *[ num ];
		for( i = 0; i < num; i++ )
		{
			savegame.ReadObject( reinterpret_cast<idClass*&>( locationEntities[ i ] ) );
		}
	}
	
	savegame.ReadObject( reinterpret_cast<idClass*&>( camera ) );
	
	savegame.ReadMaterial( globalMaterial );
	
	lastAIAlertEntity.Restore( &savegame );
	savegame.ReadInt( lastAIAlertTime );
	
	savegame.ReadDict( &spawnArgs );
	
	savegame.ReadInt( playerPVS.i );
	savegame.ReadInt( ( int& )playerPVS.h );
	savegame.ReadInt( playerConnectedAreas.i );
	savegame.ReadInt( ( int& )playerConnectedAreas.h );
	
	savegame.ReadVec3( gravity );
	
	// gamestate is restored after restoring everything else
	
	savegame.ReadBool( influenceActive );
	savegame.ReadInt( nextGibTime );
	
	// spawnSpots
	// initialSpots
	// currentInitialSpot
	// newInfo
	// makingBuild
	// shakeSounds
	
	// Read out pending events
	idEvent::Restore( &savegame );
	
	savegame.RestoreObjects();
	
	mpGame.Reset();
	
	mpGame.Precache();
	
	// free up any unused animations
	animationLib.FlushUnusedAnims();
	
	gamestate = GAMESTATE_ACTIVE;
	
	Printf( "--------------------------------------\n" );
	
	delete pipelineFile;
	pipelineFile = NULL;
	
	return true;
}

/*
===========
idGameLocal::MapClear
===========
*/
void idGameLocal::MapClear( bool clearClients )
{
	int i;
	
	for( i = ( clearClients ? 0 : MAX_CLIENTS ); i < MAX_GENTITIES; i++ )
	{
		delete entities[ i ];
		// ~idEntity is in charge of setting the pointer to NULL
		// it will also clear pending events for this entity
		assert( !entities[ i ] );
		spawnIds[ i ] = -1;
	}
	
	entityHash.Clear( 1024, MAX_GENTITIES );
	
	if( !clearClients )
	{
		// add back the hashes of the clients
		for( i = 0; i < MAX_CLIENTS; i++ )
		{
			if( !entities[ i ] )
			{
				continue;
			}
			entityHash.Add( entityHash.GenerateKey( entities[ i ]->name.c_str(), true ), i );
		}
	}
	
	delete frameCommandThread;
	frameCommandThread = NULL;
	
	if( editEntities )
	{
		delete editEntities;
		editEntities = NULL;
	}
	
	delete[] locationEntities;
	locationEntities = NULL;
}

/*
===========
idGameLocal::MapShutdown
============
*/
void idGameLocal::MapShutdown()
{
	Printf( "--------- Game Map Shutdown ----------\n" );
	
	gamestate = GAMESTATE_SHUTDOWN;
	
	if( gameRenderWorld )
	{
		// clear any debug lines, text, and polygons
		gameRenderWorld->DebugClearLines( 0 );
		gameRenderWorld->DebugClearPolygons( 0 );
	}
	
	// clear out camera if we're in a cinematic
	if( inCinematic )
	{
		camera = NULL;
		inCinematic = false;
	}
	
	MapClear( true );
	
	common->UpdateLevelLoadPacifier();
	
	// reset the script to the state it was before the map was started
	program.Restart();
	
	if( smokeParticles )
	{
		smokeParticles->Shutdown();
	}
	
	pvs.Shutdown();
	
	common->UpdateLevelLoadPacifier();
	
	clip.Shutdown();
	idClipModel::ClearTraceModelCache();
	
	common->UpdateLevelLoadPacifier();
	
	collisionModelManager->FreeMap();		// Fixes an issue where when maps were reloaded the materials wouldn't get their surfaceFlags re-set.  Now we free the map collision model forcing materials to be reparsed.
	
	common->UpdateLevelLoadPacifier();
	
	ShutdownAsyncNetwork();
	
	idStrStatic< MAX_OSPATH > mapName = mapFileName;
	mapName.StripPath();
	mapName.StripFileExtension();
	fileSystem->UnloadMapResources( mapName );
	
	mapFileName.Clear();
	
	gameRenderWorld = NULL;
	gameSoundWorld = NULL;
	
	gamestate = GAMESTATE_NOMAP;
	
	Printf( "--------------------------------------\n" );
}

/*
========================
idGameLocal::GetAimAssistAngles
========================
*/
void idGameLocal::GetAimAssistAngles( idAngles& angles )
{
	angles.Zero();
	
	// Take a look at serializing this to the clients
	idPlayer* player = GetLocalPlayer();
	if( player == NULL )
	{
		return;
	}
	
	idAimAssist* aimAssist = player->GetAimAssist();
	if( aimAssist == NULL )
	{
		return;
	}
	
	aimAssist->GetAngleCorrection( angles );
}

/*
========================
idGameLocal::GetAimAssistSensitivity
========================
*/
float idGameLocal::GetAimAssistSensitivity()
{
	// Take a look at serializing this to the clients
	idPlayer* player = GetLocalPlayer();
	if( player == NULL )
	{
		return 1.0f;
	}
	
	idAimAssist* aimAssist = player->GetAimAssist();
	if( aimAssist == NULL )
	{
		return 1.0f;
	}
	
	return aimAssist->GetFrictionScalar();
}

/*
========================
idGameLocal::MapPeerToClient
========================
*/
int idGameLocal::MapPeerToClient( int peer ) const
{
	idLobbyBase& lobby = session->GetActingGameStateLobbyBase();
	for( int userNum = 0; userNum < lobbyUserIDs.Num(); ++userNum )
	{
		const int peerForUser = lobby.PeerIndexFromLobbyUser( lobbyUserIDs[userNum] );
		if( peerForUser == peer )
		{
			return userNum;
		}
	}
	
	return -1;
}

/*
========================
idGameLocal::GetLocalClientNum
========================
*/
int idGameLocal::GetLocalClientNum() const
{
	localUserHandle_t localUserHandle = session->GetSignInManager().GetMasterLocalUserHandle();
	if( !localUserHandle.IsValid() )
	{
		return 0;
	}
	
	for( int i = 0; i < lobbyUserIDs.Num(); i++ )
	{
		lobbyUserID_t lobbyUserID = lobbyUserIDs[i];
		if( localUserHandle == lobbyUserID.GetLocalUserHandle() )
		{
			return i;
		}
	}
	return 0;
}

/*
===================
idGameLocal::Preload

===================
*/
void idGameLocal::Preload( const idPreloadManifest& manifest )
{
	animationLib.Preload( manifest );
}

/*
===================
idGameLocal::CacheDictionaryMedia

This is called after parsing an EntityDef and for each entity spawnArgs before
merging the entitydef.  It could be done post-merge, but that would
avoid the fast pre-cache check associated with each entityDef
===================
*/
void idGameLocal::CacheDictionaryMedia( const idDict* dict )
{
	const idKeyValue* kv;
	
	kv = dict->MatchPrefix( "model" );
	while( kv )
	{
		if( kv->GetValue().Length() )
		{
			declManager->MediaPrint( "Precaching model %s\n", kv->GetValue().c_str() );
			// precache model/animations
			if( declManager->FindType( DECL_MODELDEF, kv->GetValue(), false ) == NULL )
			{
				// precache the render model
				renderModelManager->FindModel( kv->GetValue() );
				// precache .cm files only
				collisionModelManager->LoadModel( kv->GetValue() );
			}
		}
		kv = dict->MatchPrefix( "model", kv );
	}
	
	kv = dict->FindKey( "s_shader" );
	if( kv != NULL && kv->GetValue().Length() )
	{
		declManager->FindType( DECL_SOUND, kv->GetValue() );
	}
	
	kv = dict->MatchPrefix( "snd", NULL );
	while( kv != NULL )
	{
		if( kv->GetValue().Length() )
		{
			declManager->FindType( DECL_SOUND, kv->GetValue() );
		}
		kv = dict->MatchPrefix( "snd", kv );
	}
	
	
	kv = dict->MatchPrefix( "gui", NULL );
	while( kv )
	{
		if( kv->GetValue().Length() )
		{
			if( !idStr::Icmp( kv->GetKey(), "gui_noninteractive" )
					|| !idStr::Icmpn( kv->GetKey(), "gui_parm", 8 )
					|| !idStr::Icmp( kv->GetKey(), "gui_inventory" ) )
			{
				// unfortunate flag names, they aren't actually a gui
			}
			else
			{
				declManager->MediaPrint( "Precaching gui %s\n", kv->GetValue().c_str() );
				idUserInterface* gui = uiManager->Alloc();
				if( gui )
				{
					gui->InitFromFile( kv->GetValue() );
					uiManager->DeAlloc( gui );
				}
			}
		}
		kv = dict->MatchPrefix( "gui", kv );
	}
	
	kv = dict->FindKey( "texture" );
	if( kv != NULL && kv->GetValue().Length() )
	{
		declManager->FindType( DECL_MATERIAL, kv->GetValue() );
	}
	
	kv = dict->MatchPrefix( "mtr", NULL );
	while( kv != NULL )
	{
		if( kv->GetValue().Length() )
		{
			declManager->FindType( DECL_MATERIAL, kv->GetValue() );
		}
		kv = dict->MatchPrefix( "mtr", kv );
	}
	
	// handles hud icons
	kv = dict->MatchPrefix( "inv_icon", NULL );
	while( kv != NULL )
	{
		if( kv->GetValue().Length() )
		{
			declManager->FindType( DECL_MATERIAL, kv->GetValue() );
		}
		kv = dict->MatchPrefix( "inv_icon", kv );
	}
	
	// handles teleport fx.. this is not ideal but the actual decision on which fx to use
	// is handled by script code based on the teleport number
	kv = dict->MatchPrefix( "teleport", NULL );
	if( kv != NULL && kv->GetValue().Length() )
	{
		int teleportType = atoi( kv->GetValue() );
		const char* p = ( teleportType ) ? va( "fx/teleporter%i.fx", teleportType ) : "fx/teleporter.fx";
		declManager->FindType( DECL_FX, p );
	}
	
	kv = dict->MatchPrefix( "fx", NULL );
	while( kv != NULL )
	{
		if( kv->GetValue().Length() )
		{
			declManager->MediaPrint( "Precaching fx %s\n", kv->GetValue().c_str() );
			declManager->FindType( DECL_FX, kv->GetValue() );
		}
		kv = dict->MatchPrefix( "fx", kv );
	}
	
	kv = dict->MatchPrefix( "smoke", NULL );
	while( kv != NULL )
	{
		if( kv->GetValue().Length() )
		{
			idStr prtName = kv->GetValue();
			int dash = prtName.Find( '-' );
			if( dash > 0 )
			{
				prtName = prtName.Left( dash );
			}
			declManager->FindType( DECL_PARTICLE, prtName );
		}
		kv = dict->MatchPrefix( "smoke", kv );
	}
	
	kv = dict->MatchPrefix( "skin", NULL );
	while( kv != NULL )
	{
		if( kv->GetValue().Length() )
		{
			declManager->MediaPrint( "Precaching skin %s\n", kv->GetValue().c_str() );
			declManager->FindType( DECL_SKIN, kv->GetValue() );
		}
		kv = dict->MatchPrefix( "skin", kv );
	}
	
	kv = dict->MatchPrefix( "def", NULL );
	while( kv != NULL )
	{
		if( kv->GetValue().Length() )
		{
			FindEntityDef( kv->GetValue().c_str(), false );
		}
		kv = dict->MatchPrefix( "def", kv );
	}
	
	// Precache all available grabber "catch" damage decls
	kv = dict->MatchPrefix( "def_damage", NULL );
	while( kv != NULL )
	{
		if( kv->GetValue().Length() )
		{
			FindEntityDef( kv->GetValue() + "_catch", false );
		}
		kv = dict->MatchPrefix( "def_damage", kv );
	}
	
	// Should have been def_monster_damage!!
	kv = dict->FindKey( "monster_damage" );
	if( kv != NULL && kv->GetValue().Length() )
	{
		FindEntityDef( kv->GetValue(), false );
	}
	
	kv = dict->MatchPrefix( "item", NULL );
	while( kv != NULL )
	{
		if( kv->GetValue().Length() )
		{
			FindEntityDefDict( kv->GetValue().c_str(), false );
		}
		kv = dict->MatchPrefix( "item", kv );
	}
	
	kv = dict->MatchPrefix( "pda_name", NULL );
	while( kv != NULL )
	{
		if( kv->GetValue().Length() )
		{
			declManager->FindType( DECL_PDA, kv->GetValue().c_str(), false );
		}
		kv = dict->MatchPrefix( "pda_name", kv );
	}
	
	kv = dict->MatchPrefix( "video", NULL );
	while( kv != NULL )
	{
		if( kv->GetValue().Length() )
		{
			declManager->FindType( DECL_VIDEO, kv->GetValue().c_str(), false );
		}
		kv = dict->MatchPrefix( "video", kv );
	}
	
	kv = dict->MatchPrefix( "audio", NULL );
	while( kv != NULL )
	{
		if( kv->GetValue().Length() )
		{
			declManager->FindType( DECL_AUDIO, kv->GetValue().c_str(), false );
		}
		kv = dict->MatchPrefix( "audio", kv );
	}
	
	kv = dict->MatchPrefix( "email", NULL );
	while( kv != NULL )
	{
		if( kv->GetValue().Length() )
		{
			declManager->FindType( DECL_EMAIL, kv->GetValue().c_str(), false );
		}
		kv = dict->MatchPrefix( "email", kv );
	}
}

/*
===========
idGameLocal::InitScriptForMap
============
*/
void idGameLocal::InitScriptForMap()
{
	// create a thread to run frame commands on
	frameCommandThread = new idThread();
	frameCommandThread->ManualDelete();
	frameCommandThread->SetThreadName( "frameCommands" );
	
	// run the main game script function (not the level specific main)
	const function_t* func = program.FindFunction( SCRIPT_DEFAULTFUNC );
	if( func != NULL )
	{
		idThread* thread = new idThread( func );
		if( thread->Start() )
		{
			// thread has finished executing, so delete it
			delete thread;
		}
	}
}

/*
===================
idGameLocal::SetScriptFPS
===================
*/
void idGameLocal::SetScriptFPS( const float engineHz )
{
	idVarDef* fpsDef = program.GetDef( &type_float, "GAME_FPS", &def_namespace );
	if( fpsDef != NULL )
	{
		eval_t fpsValue;
		fpsValue._float = engineHz;
		fpsDef->SetValue( fpsValue, false );
	}
}

/*
===========
idGameLocal::GetMPPlayerDefName
============
*/
const char* idGameLocal::GetMPPlayerDefName() const
{
	if( gameType == GAME_CTF )
	{
		return "player_doommarine_ctf";
	}
	
	return "player_doommarine_mp";
}

/*
===========
idGameLocal::SpawnPlayer
============
*/
void idGameLocal::SpawnPlayer( int clientNum )
{
	idEntity*	ent;
	idDict		args;
	
	// they can connect
	Printf( "SpawnPlayer: %i\n", clientNum );
	
	args.SetInt( "spawn_entnum", clientNum );
	args.Set( "name", va( "player%d", clientNum + 1 ) );
	if( common->IsMultiplayer() )
	{
		args.Set( "classname", GetMPPlayerDefName() );
	}
	else
	{
		// precache the player
		args.Set( "classname", gameLocal.world->spawnArgs.GetString( "def_player", "player_doommarine" ) );
	}
	
	// It's important that we increment numClients before calling SpawnEntityDef, because some
	// entities want to check gameLocal.numClients to see who to operate on (such as target_removeweapons)
	if( clientNum >= numClients )
	{
		numClients = clientNum + 1;
	}
	
	if( !SpawnEntityDef( args, &ent ) || clientNum >= MAX_GENTITIES || entities[ clientNum ] == NULL )
	{
		Error( "Failed to spawn player as '%s'", args.GetString( "classname" ) );
	}
	
	// make sure it's a compatible class
	if( !ent->IsType( idPlayer::Type ) )
	{
		Error( "'%s' spawned the player as a '%s'.  Player spawnclass must be a subclass of idPlayer.", args.GetString( "classname" ), ent->GetClassname() );
	}
	mpGame.SpawnPlayer( clientNum );
}

/*
================
idGameLocal::GetClientByNum
================
*/
idPlayer* idGameLocal::GetClientByNum( int current ) const
{
	if( current < 0 || current >= numClients )
	{
		current = 0;
	}
	if( entities[current] )
	{
		return static_cast<idPlayer*>( entities[ current ] );
	}
	return NULL;
}

/*
================
idGameLocal::GetNextClientNum
================
*/
int idGameLocal::GetNextClientNum( int _current ) const
{
	int i, current;
	
	current = 0;
	for( i = 0; i < numClients; i++ )
	{
		current = ( _current + i + 1 ) % numClients;
		if( entities[ current ] && entities[ current ]->IsType( idPlayer::Type ) )
		{
			return current;
		}
	}
	
	return current;
}

/*
================
idGameLocal::GetLocalPlayer

Nothing in the game tic should EVER make a decision based on what the
local client number is, it shouldn't even be aware that there is a
draw phase even happening.  This just returns client 0, which will
be correct for single player.
================
*/
idPlayer* idGameLocal::GetLocalPlayer() const
{
	if( GetLocalClientNum() < 0 )
	{
		return NULL;
	}
	
	if( !entities[ GetLocalClientNum() ] || !entities[ GetLocalClientNum() ]->IsType( idPlayer::Type ) )
	{
		// not fully in game yet
		return NULL;
	}
	return static_cast<idPlayer*>( entities[ GetLocalClientNum() ] );
}

/*
================
idGameLocal::SetupClientPVS
================
*/
pvsHandle_t idGameLocal::GetClientPVS( idPlayer* player, pvsType_t type )
{
	if( player->GetPrivateCameraView() )
	{
		return pvs.SetupCurrentPVS( player->GetPrivateCameraView()->GetPVSAreas(), player->GetPrivateCameraView()->GetNumPVSAreas() );
	}
	else if( camera )
	{
		return pvs.SetupCurrentPVS( camera->GetPVSAreas(), camera->GetNumPVSAreas() );
	}
	else
	{
		return pvs.SetupCurrentPVS( player->GetPVSAreas(), player->GetNumPVSAreas() );
	}
}

/*
================
idGameLocal::SetupPlayerPVS
================
*/
void idGameLocal::SetupPlayerPVS()
{
	int			i;
	idEntity* 	ent;
	idPlayer* 	player;
	pvsHandle_t	otherPVS, newPVS;
	
	playerPVS.i = -1;
	for( i = 0; i < numClients; i++ )
	{
		ent = entities[i];
		if( !ent || !ent->IsType( idPlayer::Type ) )
		{
			continue;
		}
		
		player = static_cast<idPlayer*>( ent );
		
		if( playerPVS.i == -1 )
		{
			playerPVS = GetClientPVS( player, PVS_NORMAL );
		}
		else
		{
			otherPVS = GetClientPVS( player, PVS_NORMAL );
			newPVS = pvs.MergeCurrentPVS( playerPVS, otherPVS );
			pvs.FreeCurrentPVS( playerPVS );
			pvs.FreeCurrentPVS( otherPVS );
			playerPVS = newPVS;
		}
		
		if( playerConnectedAreas.i == -1 )
		{
			playerConnectedAreas = GetClientPVS( player, PVS_CONNECTED_AREAS );
		}
		else
		{
			otherPVS = GetClientPVS( player, PVS_CONNECTED_AREAS );
			newPVS = pvs.MergeCurrentPVS( playerConnectedAreas, otherPVS );
			pvs.FreeCurrentPVS( playerConnectedAreas );
			pvs.FreeCurrentPVS( otherPVS );
			playerConnectedAreas = newPVS;
		}
		
		// if portalSky is preset, then merge into pvs so we get rotating brushes, etc
		if( portalSkyEnt.GetEntity() )
		{
			idEntity* skyEnt = portalSkyEnt.GetEntity();
			
			otherPVS = pvs.SetupCurrentPVS( skyEnt->GetPVSAreas(), skyEnt->GetNumPVSAreas() );
			newPVS = pvs.MergeCurrentPVS( playerPVS, otherPVS );
			pvs.FreeCurrentPVS( playerPVS );
			pvs.FreeCurrentPVS( otherPVS );
			playerPVS = newPVS;
			
			otherPVS = pvs.SetupCurrentPVS( skyEnt->GetPVSAreas(), skyEnt->GetNumPVSAreas() );
			newPVS = pvs.MergeCurrentPVS( playerConnectedAreas, otherPVS );
			pvs.FreeCurrentPVS( playerConnectedAreas );
			pvs.FreeCurrentPVS( otherPVS );
			playerConnectedAreas = newPVS;
		}
	}
}

/*
================
idGameLocal::FreePlayerPVS
================
*/
void idGameLocal::FreePlayerPVS()
{
	if( playerPVS.i != -1 )
	{
		pvs.FreeCurrentPVS( playerPVS );
		playerPVS.i = -1;
	}
	if( playerConnectedAreas.i != -1 )
	{
		pvs.FreeCurrentPVS( playerConnectedAreas );
		playerConnectedAreas.i = -1;
	}
}

/*
================
idGameLocal::InPlayerPVS

  should only be called during entity thinking and event handling
================
*/
bool idGameLocal::InPlayerPVS( idEntity* ent ) const
{
	if( playerPVS.i == -1 )
	{
		return false;
	}
	return pvs.InCurrentPVS( playerPVS, ent->GetPVSAreas(), ent->GetNumPVSAreas() );
}

/*
================
idGameLocal::InPlayerConnectedArea

  should only be called during entity thinking and event handling
================
*/
bool idGameLocal::InPlayerConnectedArea( idEntity* ent ) const
{
	if( playerConnectedAreas.i == -1 )
	{
		return false;
	}
	return pvs.InCurrentPVS( playerConnectedAreas, ent->GetPVSAreas(), ent->GetNumPVSAreas() );
}

/*
================
idGameLocal::UpdateGravity
================
*/
void idGameLocal::UpdateGravity()
{
	idEntity* ent;
	
	if( g_gravity.IsModified() )
	{
		if( g_gravity.GetFloat() == 0.0f )
		{
			g_gravity.SetFloat( 1.0f );
		}
		gravity.Set( 0, 0, -g_gravity.GetFloat() );
		
		// update all physics objects
		for( ent = spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() )
		{
			if( ent->IsType( idAFEntity_Generic::Type ) )
			{
				idPhysics* phys = ent->GetPhysics();
				if( phys )
				{
					phys->SetGravity( gravity );
				}
			}
		}
		g_gravity.ClearModified();
	}
}

/*
================
idGameLocal::GetGravity
================
*/
const idVec3& idGameLocal::GetGravity() const
{
	return gravity;
}

/*
================
idGameLocal::SortActiveEntityList

  Sorts the active entity list such that pushing entities come first,
  actors come next and physics team slaves appear after their master.
================
*/
void idGameLocal::SortActiveEntityList()
{
	idEntity* ent, *next_ent, *master, *part;
	
	// if the active entity list needs to be reordered to place physics team masters at the front
	if( sortTeamMasters )
	{
		for( ent = activeEntities.Next(); ent != NULL; ent = next_ent )
		{
			next_ent = ent->activeNode.Next();
			master = ent->GetTeamMaster();
			if( master && master == ent )
			{
				ent->activeNode.Remove();
				ent->activeNode.AddToFront( activeEntities );
			}
		}
	}
	
	// if the active entity list needs to be reordered to place pushers at the front
	if( sortPushers )
	{
	
		for( ent = activeEntities.Next(); ent != NULL; ent = next_ent )
		{
			next_ent = ent->activeNode.Next();
			master = ent->GetTeamMaster();
			if( !master || master == ent )
			{
				// check if there is an actor on the team
				for( part = ent; part != NULL; part = part->GetNextTeamEntity() )
				{
					if( part->GetPhysics()->IsType( idPhysics_Actor::Type ) )
					{
						break;
					}
				}
				// if there is an actor on the team
				if( part )
				{
					ent->activeNode.Remove();
					ent->activeNode.AddToFront( activeEntities );
				}
			}
		}
		
		for( ent = activeEntities.Next(); ent != NULL; ent = next_ent )
		{
			next_ent = ent->activeNode.Next();
			master = ent->GetTeamMaster();
			if( !master || master == ent )
			{
				// check if there is an entity on the team using parametric physics
				for( part = ent; part != NULL; part = part->GetNextTeamEntity() )
				{
					if( part->GetPhysics()->IsType( idPhysics_Parametric::Type ) )
					{
						break;
					}
				}
				// if there is an entity on the team using parametric physics
				if( part )
				{
					ent->activeNode.Remove();
					ent->activeNode.AddToFront( activeEntities );
				}
			}
		}
	}
	
	sortTeamMasters = false;
	sortPushers = false;
}



/*
========================
idGameLocal::SetInterpolation
========================
*/
void idGameLocal::SetInterpolation( const float fraction, const int serverGameMS, const int ssStartTime, const int ssEndTime )
{
	netInterpolationInfo.previousServerGameMs = netInterpolationInfo.serverGameMs;
	netInterpolationInfo.pct = fraction;
	netInterpolationInfo.serverGameMs = serverGameMS;
	netInterpolationInfo.ssStartTime = ssStartTime;
	netInterpolationInfo.ssEndTime = ssEndTime;
}


/*
================
idGameLocal::RunTimeGroup2
================
*/
void idGameLocal::RunTimeGroup2( idUserCmdMgr& userCmdMgr )
{
	idEntity* ent;
	int num = 0;
	
	SelectTimeGroup( true );
	
	for( ent = activeEntities.Next(); ent != NULL; ent = ent->activeNode.Next() )
	{
		if( ent->timeGroup != TIME_GROUP2 )
		{
			continue;
		}
		RunEntityThink( *ent, userCmdMgr );
		num++;
	}
	
	SelectTimeGroup( false );
}

/*
================
idGameLocal::RunEntityThink
================
*/
void idGameLocal::RunEntityThink( idEntity& ent, idUserCmdMgr& userCmdMgr )
{
	if( ent.entityNumber < MAX_PLAYERS )
	{
		// Players may run more than one think per frame in MP,
		// if there is a large buffer of usercmds from the network.
		// Players will always run exactly one think in singleplayer.
		RunAllUserCmdsForPlayer( userCmdMgr, ent.entityNumber );
	}
	else
	{
		// Non-player entities always run one think.
		ent.Think();
	}
}

idCVar g_recordTrace( "g_recordTrace", "0", CVAR_BOOL, "" );

/*
================
idGameLocal::RunFrame
================
*/
void idGameLocal::RunFrame( idUserCmdMgr& cmdMgr, gameReturn_t& ret )
{
	idEntity* 	ent;
	int			num;
	float		ms;
	idTimer		timer_think, timer_events, timer_singlethink;
	
	idPlayer*	player;
	const renderView_t* view;
	
	if( g_recordTrace.GetBool() )
	{
		bool result = BeginTraceRecording( "e:\\gametrace.pix2" );
		if( !result )
		{
			//idLib::Printf( "BeginTraceRecording: error %d\n", GetLastError() );
		}
	}
	
#ifdef _DEBUG
	if( common->IsMultiplayer() )
	{
		assert( !common->IsClient() );
	}
#endif
	
	if( gameRenderWorld == NULL )
	{
		return;
	}
	
	SyncPlayersWithLobbyUsers( false );
	ServerSendNetworkSyncCvars();
	
	player = GetLocalPlayer();
	
	if( !common->IsMultiplayer() && g_stopTime.GetBool() )
	{
		// clear any debug lines from a previous frame
		gameRenderWorld->DebugClearLines( time + 1 );
		
		// set the user commands for this frame
		if( player )
		{
			player->HandleUserCmds( cmdMgr.GetUserCmdForPlayer( GetLocalClientNum() ) );
			cmdMgr.MakeReadPtrCurrentForPlayer( GetLocalClientNum() );
			player->Think();
		}
	}
	else
	{
		do
		{
			// update the game time
			framenum++;
			fast.previousTime = FRAME_TO_MSEC( framenum - 1 );
			fast.time = FRAME_TO_MSEC( framenum );
			fast.realClientTime = fast.time;
			SetServerGameTimeMs( fast.time );
			
			ComputeSlowScale();
			
			slow.previousTime = slow.time;
			slow.time += idMath::Ftoi( ( fast.time - fast.previousTime ) * slowmoScale );
			slow.realClientTime = slow.time;
			
			SelectTimeGroup( false );
			
#ifdef GAME_DLL
			// allow changing SIMD usage on the fly
			if( com_forceGenericSIMD.IsModified() )
			{
				idSIMD::InitProcessor( "game", com_forceGenericSIMD.GetBool() );
			}
#endif
			
			// make sure the random number counter is used each frame so random events
			// are influenced by the player's actions
			random.RandomInt();
			
			if( player )
			{
				// update the renderview so that any gui videos play from the right frame
				view = player->GetRenderView();
				if( view )
				{
					gameRenderWorld->SetRenderView( view );
				}
			}
			
			// clear any debug lines from a previous frame
			gameRenderWorld->DebugClearLines( time );
			
			// clear any debug polygons from a previous frame
			gameRenderWorld->DebugClearPolygons( time );
			
			// free old smoke particles
			smokeParticles->FreeSmokes();
			
			// process events on the server
			ServerProcessEntityNetworkEventQueue();
			
			// update our gravity vector if needed.
			UpdateGravity();
			
			// create a merged pvs for all players
			SetupPlayerPVS();
			
			// sort the active entity list
			SortActiveEntityList();
			
			timer_think.Clear();
			timer_think.Start();
			
			// let entities think
			if( g_timeentities.GetFloat() )
			{
				num = 0;
				for( ent = activeEntities.Next(); ent != NULL; ent = ent->activeNode.Next() )
				{
					if( g_cinematic.GetBool() && inCinematic && !ent->cinematic )
					{
						ent->GetPhysics()->UpdateTime( time );
						continue;
					}
					timer_singlethink.Clear();
					timer_singlethink.Start();
					RunEntityThink( *ent, cmdMgr );
					timer_singlethink.Stop();
					ms = timer_singlethink.Milliseconds();
					if( ms >= g_timeentities.GetFloat() )
					{
						Printf( "%d: entity '%s': %.1f ms\n", time, ent->name.c_str(), ms );
					}
					num++;
				}
			}
			else
			{
				if( inCinematic )
				{
					num = 0;
					for( ent = activeEntities.Next(); ent != NULL; ent = ent->activeNode.Next() )
					{
						if( g_cinematic.GetBool() && !ent->cinematic )
						{
							ent->GetPhysics()->UpdateTime( time );
							continue;
						}
						RunEntityThink( *ent, cmdMgr );
						num++;
					}
				}
				else
				{
					num = 0;
					for( ent = activeEntities.Next(); ent != NULL; ent = ent->activeNode.Next() )
					{
						if( ent->timeGroup != TIME_GROUP1 )
						{
							continue;
						}
						RunEntityThink( *ent, cmdMgr );
						num++;
					}
				}
			}
			
			RunTimeGroup2( cmdMgr );
			
			// Run catch-up for any client projectiles.
			// This is done after the main think so that all projectiles will be up-to-date
			// when snapshots are created.
			//if ( common->IsMultiplayer() ) {
			//while ( SimulateProjectiles() ) {
			//clientGame.gameLibEffects.Update( clientGame.GetGameMs(), clientGame.GetGameMsPerFrame(), clientGame.GetServerGameTime() );
			//}
			//}
			
			
			// remove any entities that have stopped thinking
			if( numEntitiesToDeactivate )
			{
				idEntity* next_ent;
				int c = 0;
				for( ent = activeEntities.Next(); ent != NULL; ent = next_ent )
				{
					next_ent = ent->activeNode.Next();
					if( !ent->thinkFlags )
					{
						ent->activeNode.Remove();
						c++;
					}
				}
				//assert( numEntitiesToDeactivate == c );
				numEntitiesToDeactivate = 0;
			}
			
			timer_think.Stop();
			timer_events.Clear();
			timer_events.Start();
			
			// service any pending events
			idEvent::ServiceEvents();
			
			// service pending fast events
			SelectTimeGroup( true );
			idEvent::ServiceFastEvents();
			SelectTimeGroup( false );
			
			timer_events.Stop();
			
			// free the player pvs
			FreePlayerPVS();
			
			// do multiplayer related stuff
			if( common->IsMultiplayer() )
			{
				mpGame.Run();
			}
			
			// display how long it took to calculate the current game frame
			if( g_frametime.GetBool() )
			{
				Printf( "game %d: all:%.1f th:%.1f ev:%.1f %d ents \n",
						time, timer_think.Milliseconds() + timer_events.Milliseconds(),
						timer_think.Milliseconds(), timer_events.Milliseconds(), num );
			}
			
			BuildReturnValue( ret );
			
			// see if a target_sessionCommand has forced a changelevel
			if( sessionCommand.Length() )
			{
				strncpy( ret.sessionCommand, sessionCommand, sizeof( ret.sessionCommand ) );
				break;
			}
			
			// make sure we don't loop forever when skipping a cinematic
			if( skipCinematic && ( time > cinematicMaxSkipTime ) )
			{
				Warning( "Exceeded maximum cinematic skip length.  Cinematic may be looping infinitely." );
				skipCinematic = false;
				break;
			}
		}
		while( ( inCinematic || ( time < cinematicStopTime ) ) && skipCinematic );
		
		// i think this loop always play all that content until we skip the cinematic,
		// when then we keep until the end of the function
	}
	
	//ret.syncNextGameFrame = skipCinematic; // this is form dhewm3 but it seems it's no longer useful
	if( skipCinematic )
	{
		soundSystem->SetMute( false );
		skipCinematic = false;
	}
	
	// show any debug info for this frame
	RunDebugInfo();
	D_DrawDebugLines();
	
	if( g_recordTrace.GetBool() )
	{
		EndTraceRecording();
		g_recordTrace.SetBool( false );
	}
}

/*
====================
idGameLocal::BuildReturnValue

Fills out gameReturn_t, called on server and clients.
====================
*/
void idGameLocal::BuildReturnValue( gameReturn_t& ret )
{
	ret.sessionCommand[0] = 0;
	
	if( GetLocalPlayer() != NULL )
	{
		GetLocalPlayer()->GetControllerShake( ret.vibrationLow, ret.vibrationHigh );
	}
	else
	{
		// Dedicated server?
		ret.vibrationLow = 0;
		ret.vibrationHigh = 0;
	}
	
	// see if a target_sessionCommand has forced a changelevel
	if( sessionCommand.Length() )
	{
		strncpy( ret.sessionCommand, sessionCommand, sizeof( ret.sessionCommand ) );
		sessionCommand.Clear();	// Fixes a double loading bug for the e3 demo.  Since we run the game thread in SMP mode, we could run this twice and re-set the ret.sessionCommand to a stale sessionCommand (since that doesn't get cleared until LoadMap is called!)
	}
}

/*
====================
idGameLocal::RunSinglelUserCmd

Runs a Think or a ClientThink for a player. Will write the client's
position and firecount to the usercmd.
====================
*/
void idGameLocal::RunSingleUserCmd( usercmd_t& cmd, idPlayer& player )
{
	player.HandleUserCmds( cmd );
	
	// To fix the stupid chaingun script that depends on frame counts instead of
	// milliseconds in the case of the server running at 60Hz and the client running
	// at 120Hz, we need to set the script's GAME_FPS value to the client's effective rate.
	// (I'd like to just fix the script to use milliseconds, but we don't want to change assets
	// at this point.)
	if( !player.IsLocallyControlled() )
	{
		const float usercmdMillisecondDelta = player.usercmd.clientGameMilliseconds - player.oldCmd.clientGameMilliseconds;
		const float clientEngineHz = 1000.0f / usercmdMillisecondDelta;
		
		// Force to 60 or 120, those are the only values allowed in multiplayer.
		const float forcedClientEngineHz = ( clientEngineHz < 90.0f ) ? 60.0f : 120.0f;
		SetScriptFPS( forcedClientEngineHz );
	}
	
	if( !common->IsMultiplayer() || common->IsServer() )
	{
		player.Think();
		
		// Keep track of the client time of the usercmd we just ran. We will send this back to clients
		// in a snapshot so they know when they can stop predicting certain things.
		usercmdLastClientMilliseconds[ player.GetEntityNumber() ] = cmd.clientGameMilliseconds;
	}
	else
	{
		player.ClientThink( netInterpolationInfo.serverGameMs, netInterpolationInfo.pct, true );
	}
	
	// Since the client is authoritative on its position, we have to update the usercmd
	// that will be sent over the network after the player thinks.
	cmd.pos = player.usercmd.pos;
	cmd.fireCount = player.GetClientFireCount();
	if( player.GetPhysics() )
	{
		cmd.speedSquared = player.GetPhysics()->GetLinearVelocity().LengthSqr();
	}
}

/*
====================
idGameLocal::RunAllUserCmdsForPlayer
Runs a Think or ClientThink for each usercmd, but leaves a few cmds in the buffer
so that we have something to process while we wait for more from the network.
====================
*/
void idGameLocal::RunAllUserCmdsForPlayer( idUserCmdMgr& cmdMgr, const int playerNumber )
{
//idLib::Printf( "Frame: %i = [%i-%i] ", gameLocal.framenum,
//cmdMgr.readFrame[0], cmdMgr.writeFrame[0] );	// !@#

	// Run thinks on any players that have queued up usercmds for networking.
	assert( playerNumber < MAX_PLAYERS );
	
	if( entities[ playerNumber ] == NULL )
	{
		return;
	}
	
	idPlayer& player = static_cast< idPlayer& >( *entities[ playerNumber ] );
	
	// Only run a single userCmd each game frame for local players, otherwise when
	// we are running < 60fps things like footstep sounds may get started right on top
	// of each other instead of spread out in time.
	if( player.IsLocallyControlled() )
	{
		if( cmdMgr.HasUserCmdForPlayer( playerNumber ) )
		{
			if( net_usercmd_timing_debug.GetBool() )
			{
				idLib::Printf( "[%d]Running local cmd for player %d, %d buffered.\n",
							   common->GetGameFrame(), playerNumber, cmdMgr.GetNumUnreadFrames( playerNumber ) );
			}
			RunSingleUserCmd( cmdMgr.GetWritableUserCmdForPlayer( playerNumber ), player );
		}
		else
		{
			RunSingleUserCmd( player.usercmd, player );
		}
		return;
	}
	
	// Only the server runs remote commands.
	if( common->IsClient() )
	{
		return;
	}
	
	// Make sure to run a command for remote players. May duplicate the previous command
	// if the server is running faster, or run an "empty" command if the buffer
	// underflows.
	if( cmdMgr.HasUserCmdForPlayer( player.GetEntityNumber() ) )
	{
		const int clientTimeOfNextCommand = cmdMgr.GetNextUserCmdClientTime( playerNumber );
		const int timeDeltaBetweenClientCommands = clientTimeOfNextCommand - lastCmdRunTimeOnClient[ playerNumber ];
		const int timeSinceServerRanLastCommand = gameLocal.time - lastCmdRunTimeOnServer[ playerNumber ];
		int clientTimeRunSoFar = 0;
		
		// Handle clients who may be running faster than the server. Potentiallly runs multiple
		// usercmds so that the server can catch up.
		if( timeDeltaBetweenClientCommands - timeSinceServerRanLastCommand <= 1 )
		{
			while( clientTimeRunSoFar < ( timeSinceServerRanLastCommand - 1 ) && cmdMgr.HasUserCmdForPlayer( player.GetEntityNumber() ) )
			{
				usercmd_t& currentCommand = cmdMgr.GetWritableUserCmdForPlayer( playerNumber );
				RunSingleUserCmd( currentCommand, player );
				lastCmdRunTimeOnClient[ playerNumber ] = currentCommand.clientGameMilliseconds;
				lastCmdRunTimeOnServer[ playerNumber ] = gameLocal.serverTime;
				clientTimeRunSoFar += timeDeltaBetweenClientCommands;
				if( clientTimeRunSoFar == 0 )
				{
					// Hack to avoid infinite loop
					break;
				}
				if( net_usercmd_timing_debug.GetBool() )
				{
					idLib::Printf( "[%d]Running initial cmd for player %d, %d buffered, %d so far, %d serverDelta.\n",
								   common->GetGameFrame(), playerNumber, cmdMgr.GetNumUnreadFrames( playerNumber ),
								   clientTimeRunSoFar, timeSinceServerRanLastCommand );
				}
			}
		}
		else
		{
			// If we get here, it is likely that the client is running at 60Hz but the server
			// is running at 120Hz. Duplicate the previous
			// usercmd and run it so that the server doesn't starve.
			usercmd_t lastPlayerCmd = player.usercmd;
			RunSingleUserCmd( lastPlayerCmd, player );
			if( net_usercmd_timing_debug.GetBool() )
			{
				idLib::Printf( "[%d]Running duplicated command for player %d to prevent server from starving. clientCmdTimeDelta = %d, runCmdTimeDeltaOnServer = %d.\n",
							   common->GetGameFrame(), playerNumber, timeDeltaBetweenClientCommands, timeSinceServerRanLastCommand );
			}
		}
	}
	else
	{
		// Run an "empty" cmd, ran out of buffer.
		usercmd_t emptyCmd = player.usercmd;
		emptyCmd.forwardmove = 0;
		emptyCmd.rightmove = 0;
		RunSingleUserCmd( emptyCmd, player );
		lastCmdRunTimeOnServer[ playerNumber ] = gameLocal.serverTime;
		if( net_usercmd_timing_debug.GetBool() )
		{
			idLib::Printf( "[%d]Ran out of commands for player %d.\n", common->GetGameFrame(), playerNumber );
		}
	}
	
	// For remote players on the server, run enough commands
	// to leave only a buffer that will hold us over for a
	// number of milliseconds equal to the net_ucmdRate + one frame.
	const int MaxExtraCommandsPerFrame = 15;
	int numPasses = 0;
	
	for( ; numPasses < MaxExtraCommandsPerFrame; numPasses++ )
	{
		// Run remote player extra commands
		extern idCVar net_ucmdRate;
		// Add some extra time to smooth out network inconsistencies.
		const int extraFrameMilliseconds = FRAME_TO_MSEC( common->GetGameFrame() + 2 ) - FRAME_TO_MSEC( common->GetGameFrame() );
		const int millisecondBuffer = MSEC_ALIGN_TO_FRAME( net_ucmdRate.GetInteger() + extraFrameMilliseconds );
		
		const bool hasNextCmd = cmdMgr.HasUserCmdForClientTimeBuffer( playerNumber, millisecondBuffer );
		
		if( hasNextCmd )
		{
			usercmd_t& currentCommand = cmdMgr.GetWritableUserCmdForPlayer( playerNumber );
			
			if( net_usercmd_timing_debug.GetBool() )
			{
				idLib::Printf( "[%d]Pass %d, running extra cmd for player %d, %d buffered\n", common->GetGameFrame(), numPasses, playerNumber, cmdMgr.GetNumUnreadFrames( playerNumber ) );
			}
			
			RunSingleUserCmd( currentCommand, player );
			lastCmdRunTimeOnClient[ playerNumber ] = currentCommand.clientGameMilliseconds;
			lastCmdRunTimeOnServer[ playerNumber ] = gameLocal.serverTime;
		}
		else
		{
			break;
		}
	}
	
	// Reset the script FPS in case it was changed to accomodate an MP client
	// running at a different framerate.
	SetScriptFPS( com_engineHz_latched );
	
//idLib::Printf( "\n" );//!@#
}


/*
======================================================================

  Game view drawing

======================================================================
*/

/*
====================
idGameLocal::CalcFov

Calculates the horizontal and vertical field of view based on a horizontal field of view and custom aspect ratio
====================
*/
void idGameLocal::CalcFov( float base_fov, float& fov_x, float& fov_y ) const
{
	const int width = renderSystem->GetWidth();
	const int height = renderSystem->GetHeight();
	if( width == height )
	{
		// this is the Rift, so don't mess with our aspect ratio corrections
		fov_x = base_fov;
		fov_y = base_fov;
		return;
	}
	
	// Calculate the fov_y based on an ideal aspect ratio
	const float ideal_ratio_x = 16.0f;
	const float ideal_ratio_y = 9.0f;
	const float tanHalfX = idMath::Tan( DEG2RAD( base_fov * 0.5f ) );
	fov_y = 2.0f * RAD2DEG( idMath::ATan( ideal_ratio_y * tanHalfX, ideal_ratio_x ) );
	
	// Then calculate fov_x based on the true aspect ratio
	const float ratio_x = width * renderSystem->GetPixelAspect();
	const float ratio_y = height;
	const float tanHalfY = idMath::Tan( DEG2RAD( fov_y * 0.5f ) );
	fov_x = 2.0f * RAD2DEG( idMath::ATan( ratio_x * tanHalfY, ratio_y ) );
}

/*
================
idGameLocal::Draw

makes rendering and sound system calls
================
*/
bool idGameLocal::Draw( int clientNum )
{

	if( clientNum == -1 )
	{
		return false;
	}
	
	if( common->IsMultiplayer() && session->GetState() == idSession::INGAME )
	{
		return mpGame.Draw( clientNum );
	}
	
	// chose the optimized or legacy device context code
	uiManager->SetDrawingDC();
	
	idPlayer* player = static_cast<idPlayer*>( entities[ clientNum ] );
	
	if( ( player == NULL ) || ( player->GetRenderView() == NULL ) )
	{
		return false;
	}
	
	// render the scene
	player->playerView.RenderPlayerView( player->hudManager );
	
	return true;
}

/*
================
idGameLocal::HandleGuiCommands
================
*/
bool idGameLocal::HandlePlayerGuiEvent( const sysEvent_t* ev )
{

	idPlayer* player = GetLocalPlayer();
	bool handled = false;
	
	if( player != NULL )
	{
		handled = player->HandleGuiEvents( ev );
	}
	
	if( common->IsMultiplayer() && !handled )
	{
		handled = mpGame.HandleGuiEvent( ev );
	}
	
	return handled;
}

/*
================
idGameLocal::GetLevelMap

  should only be used for in-game level editing
================
*/
idMapFile* idGameLocal::GetLevelMap()
{
	if( mapFile && mapFile->HasPrimitiveData() )
	{
		return mapFile;
	}
	if( !mapFileName.Length() )
	{
		return NULL;
	}
	
	if( mapFile )
	{
		delete mapFile;
	}
	
	mapFile = new( TAG_GAME ) idMapFile;
	if( !mapFile->Parse( mapFileName ) )
	{
		delete mapFile;
		mapFile = NULL;
	}
	
	return mapFile;
}

/*
================
idGameLocal::GetMapName
================
*/
const char* idGameLocal::GetMapName() const
{
	return mapFileName.c_str();
}

/*
================
idGameLocal::CallFrameCommand
================
*/
void idGameLocal::CallFrameCommand( idEntity* ent, const function_t* frameCommand )
{
	frameCommandThread->CallFunction( ent, frameCommand, true );
	frameCommandThread->Execute();
}

/*
================
idGameLocal::CallObjectFrameCommand
================
*/
void idGameLocal::CallObjectFrameCommand( idEntity* ent, const char* frameCommand )
{
	const function_t* func;
	
	func = ent->scriptObject.GetFunction( frameCommand );
	if( !func )
	{
		if( !ent->IsType( idTestModel::Type ) )
		{
			Error( "Unknown function '%s' called for frame command on entity '%s'", frameCommand, ent->name.c_str() );
		}
	}
	else
	{
		frameCommandThread->CallFunction( ent, func, true );
		frameCommandThread->Execute();
	}
}

/*
================
idGameLocal::ShowTargets
================
*/
void idGameLocal::ShowTargets()
{
	idMat3		axis = GetLocalPlayer()->viewAngles.ToMat3();
	idVec3		up = axis[ 2 ] * 5.0f;
	const idVec3& viewPos = GetLocalPlayer()->GetPhysics()->GetOrigin();
	idBounds	viewTextBounds( viewPos );
	idBounds	viewBounds( viewPos );
	idBounds	box( idVec3( -4.0f, -4.0f, -4.0f ), idVec3( 4.0f, 4.0f, 4.0f ) );
	idEntity*	ent;
	idEntity*	target;
	int			i;
	idBounds	totalBounds;
	
	viewTextBounds.ExpandSelf( 128.0f );
	viewBounds.ExpandSelf( 512.0f );
	for( ent = spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() )
	{
		totalBounds = ent->GetPhysics()->GetAbsBounds();
		for( i = 0; i < ent->targets.Num(); i++ )
		{
			target = ent->targets[ i ].GetEntity();
			if( target )
			{
				totalBounds.AddBounds( target->GetPhysics()->GetAbsBounds() );
			}
		}
		
		if( !viewBounds.IntersectsBounds( totalBounds ) )
		{
			continue;
		}
		
		float dist;
		idVec3 dir = totalBounds.GetCenter() - viewPos;
		dir.NormalizeFast();
		totalBounds.RayIntersection( viewPos, dir, dist );
		float frac = ( 512.0f - dist ) / 512.0f;
		if( frac < 0.0f )
		{
			continue;
		}
		
		gameRenderWorld->DebugBounds( ( ent->IsHidden() ? colorLtGrey : colorOrange ) * frac, ent->GetPhysics()->GetAbsBounds() );
		if( viewTextBounds.IntersectsBounds( ent->GetPhysics()->GetAbsBounds() ) )
		{
			idVec3 center = ent->GetPhysics()->GetAbsBounds().GetCenter();
			gameRenderWorld->DrawText( ent->name.c_str(), center - up, 0.1f, colorWhite * frac, axis, 1 );
			gameRenderWorld->DrawText( ent->GetEntityDefName(), center, 0.1f, colorWhite * frac, axis, 1 );
			gameRenderWorld->DrawText( va( "#%d", ent->entityNumber ), center + up, 0.1f, colorWhite * frac, axis, 1 );
		}
		
		for( i = 0; i < ent->targets.Num(); i++ )
		{
			target = ent->targets[ i ].GetEntity();
			if( target )
			{
				gameRenderWorld->DebugArrow( colorYellow * frac, ent->GetPhysics()->GetAbsBounds().GetCenter(), target->GetPhysics()->GetOrigin(), 10, 0 );
				gameRenderWorld->DebugBounds( colorGreen * frac, box, target->GetPhysics()->GetOrigin() );
			}
		}
	}
}

/*
================
idGameLocal::RunDebugInfo
================
*/
void idGameLocal::RunDebugInfo()
{
	idEntity* ent;
	idPlayer* player;
	
	player = GetLocalPlayer();
	if( !player )
	{
		return;
	}
	
	const idVec3& origin = player->GetPhysics()->GetOrigin();
	
	if( g_showEntityInfo.GetBool() )
	{
		idMat3		axis = player->viewAngles.ToMat3();
		idVec3		up = axis[ 2 ] * 5.0f;
		idBounds	viewTextBounds( origin );
		idBounds	viewBounds( origin );
		
		viewTextBounds.ExpandSelf( 128.0f );
		viewBounds.ExpandSelf( 512.0f );
		for( ent = spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() )
		{
			// don't draw the worldspawn
			if( ent == world )
			{
				continue;
			}
			
			// skip if the entity is very far away
			if( !viewBounds.IntersectsBounds( ent->GetPhysics()->GetAbsBounds() ) )
			{
				continue;
			}
			
			const idBounds& entBounds = ent->GetPhysics()->GetAbsBounds();
			int contents = ent->GetPhysics()->GetContents();
			if( contents & CONTENTS_BODY )
			{
				gameRenderWorld->DebugBounds( colorCyan, entBounds );
			}
			else if( contents & CONTENTS_TRIGGER )
			{
				gameRenderWorld->DebugBounds( colorOrange, entBounds );
			}
			else if( contents & CONTENTS_SOLID )
			{
				gameRenderWorld->DebugBounds( colorGreen, entBounds );
			}
			else
			{
				if( !entBounds.GetVolume() )
				{
					gameRenderWorld->DebugBounds( colorMdGrey, entBounds.Expand( 8.0f ) );
				}
				else
				{
					gameRenderWorld->DebugBounds( colorMdGrey, entBounds );
				}
			}
			if( viewTextBounds.IntersectsBounds( entBounds ) )
			{
				gameRenderWorld->DrawText( ent->name.c_str(), entBounds.GetCenter(), 0.1f, colorWhite, axis, 1 );
				gameRenderWorld->DrawText( va( "#%d", ent->entityNumber ), entBounds.GetCenter() + up, 0.1f, colorWhite, axis, 1 );
			}
		}
	}
	
	// debug tool to draw bounding boxes around active entities
	if( g_showActiveEntities.GetBool() )
	{
		for( ent = activeEntities.Next(); ent != NULL; ent = ent->activeNode.Next() )
		{
			idBounds	b = ent->GetPhysics()->GetBounds();
			if( b.GetVolume() <= 0 )
			{
				b[0][0] = b[0][1] = b[0][2] = -8;
				b[1][0] = b[1][1] = b[1][2] = 8;
			}
			if( ent->fl.isDormant )
			{
				gameRenderWorld->DebugBounds( colorYellow, b, ent->GetPhysics()->GetOrigin() );
			}
			else
			{
				gameRenderWorld->DebugBounds( colorGreen, b, ent->GetPhysics()->GetOrigin() );
			}
		}
	}
	
	if( g_showTargets.GetBool() )
	{
		ShowTargets();
	}
	
	if( g_showTriggers.GetBool() )
	{
		idTrigger::DrawDebugInfo();
	}
	
	if( ai_showCombatNodes.GetBool() )
	{
		idCombatNode::DrawDebugInfo();
	}
	
	if( ai_showPaths.GetBool() )
	{
		idPathCorner::DrawDebugInfo();
	}
	
	if( g_editEntityMode.GetBool() )
	{
		editEntities->DisplayEntities();
	}
	
	if( g_showCollisionWorld.GetBool() )
	{
		collisionModelManager->DrawModel( 0, vec3_origin, mat3_identity, origin, 128.0f );
	}
	
	if( g_showCollisionModels.GetBool() )
	{
		clip.DrawClipModels( player->GetEyePosition(), g_maxShowDistance.GetFloat(), pm_thirdPerson.GetBool() ? NULL : player );
	}
	
	if( g_showCollisionTraces.GetBool() )
	{
		clip.PrintStatistics();
	}
	
	if( g_showPVS.GetInteger() )
	{
		pvs.DrawPVS( origin, ( g_showPVS.GetInteger() == 2 ) ? PVS_ALL_PORTALS_OPEN : PVS_NORMAL );
	}
	
	if( aas_test.GetInteger() >= 0 )
	{
		idAAS* aas = GetAAS( aas_test.GetInteger() );
		if( aas )
		{
			aas->Test( origin );
			if( ai_testPredictPath.GetBool() )
			{
				idVec3 velocity;
				predictedPath_t path;
				
				velocity.x = cos( DEG2RAD( player->viewAngles.yaw ) ) * 100.0f;
				velocity.y = sin( DEG2RAD( player->viewAngles.yaw ) ) * 100.0f;
				velocity.z = 0.0f;
				idAI::PredictPath( player, aas, origin, velocity, 1000, 100, SE_ENTER_OBSTACLE | SE_BLOCKED | SE_ENTER_LEDGE_AREA, path );
			}
		}
	}
	
	if( ai_showObstacleAvoidance.GetInteger() == 2 )
	{
		idAAS* aas = GetAAS( 0 );
		if( aas )
		{
			idVec3 seekPos;
			obstaclePath_t path;
			
			seekPos = player->GetPhysics()->GetOrigin() + player->viewAxis[0] * 200.0f;
			idAI::FindPathAroundObstacles( player->GetPhysics(), aas, NULL, player->GetPhysics()->GetOrigin(), seekPos, path );
		}
	}
	
	// collision map debug output
	collisionModelManager->DebugOutput( player->GetEyePosition() );
}

/*
==================
idGameLocal::NumAAS
==================
*/
int	idGameLocal::NumAAS() const
{
	return aasList.Num();
}

/*
==================
idGameLocal::GetAAS
==================
*/
idAAS* idGameLocal::GetAAS( int num ) const
{
	if( ( num >= 0 ) && ( num < aasList.Num() ) )
	{
		if( aasList[ num ] && aasList[ num ]->GetSettings() )
		{
			return aasList[ num ];
		}
	}
	return NULL;
}

/*
==================
idGameLocal::GetAAS
==================
*/
idAAS* idGameLocal::GetAAS( const char* name ) const
{
	int i;
	
	for( i = 0; i < aasNames.Num(); i++ )
	{
		if( aasNames[ i ] == name )
		{
			if( !aasList[ i ]->GetSettings() )
			{
				return NULL;
			}
			else
			{
				return aasList[ i ];
			}
		}
	}
	return NULL;
}

/*
==================
idGameLocal::SetAASAreaState
==================
*/
void idGameLocal::SetAASAreaState( const idBounds& bounds, const int areaContents, bool closed )
{
	int i;
	
	for( i = 0; i < aasList.Num(); i++ )
	{
		aasList[ i ]->SetAreaState( bounds, areaContents, closed );
	}
}

/*
==================
idGameLocal::AddAASObstacle
==================
*/
aasHandle_t idGameLocal::AddAASObstacle( const idBounds& bounds )
{
	int i;
	aasHandle_t obstacle;
	aasHandle_t check;
	
	if( !aasList.Num() )
	{
		return -1;
	}
	
	obstacle = aasList[ 0 ]->AddObstacle( bounds );
	for( i = 1; i < aasList.Num(); i++ )
	{
		check = aasList[ i ]->AddObstacle( bounds );
		assert( check == obstacle );
	}
	
	return obstacle;
}

/*
==================
idGameLocal::RemoveAASObstacle
==================
*/
void idGameLocal::RemoveAASObstacle( const aasHandle_t handle )
{
	int i;
	
	for( i = 0; i < aasList.Num(); i++ )
	{
		aasList[ i ]->RemoveObstacle( handle );
	}
}

/*
==================
idGameLocal::RemoveAllAASObstacles
==================
*/
void idGameLocal::RemoveAllAASObstacles()
{
	int i;
	
	for( i = 0; i < aasList.Num(); i++ )
	{
		aasList[ i ]->RemoveAllObstacles();
	}
}

/*
==================
idGameLocal::CheatsOk
==================
*/
bool idGameLocal::CheatsOk( bool requirePlayer )
{
	extern idCVar net_allowCheats;
	if( common->IsMultiplayer() && !net_allowCheats.GetBool() )
	{
		Printf( "Not allowed in multiplayer.\n" );
		return false;
	}
	
	if( developer.GetBool() )
	{
		return true;
	}
	
	idPlayer* player = GetLocalPlayer();
	if( !requirePlayer || ( player != NULL && ( player->health > 0 ) ) )
	{
		return true;
	}
	
	Printf( "You must be alive to use this command.\n" );
	
	return false;
}

/*
===================
idGameLocal::RegisterEntity
===================
*/
void idGameLocal::RegisterEntity( idEntity* ent, int forceSpawnId, const idDict& spawnArgsToCopy )
{
	int spawn_entnum;
	
	ent->fl.skipReplication = spawnArgsToCopy.GetBool( "net_skip_replication", false );
	
	if( spawnCount >= ( 1 << ( 32 - GENTITYNUM_BITS ) ) )
	{
		Error( "idGameLocal::RegisterEntity: spawn count overflow" );
	}
	
	if( !spawnArgsToCopy.GetInt( "spawn_entnum", "0", spawn_entnum ) )
	{
		const int freeListType = ( common->IsMultiplayer() && ent->GetSkipReplication() ) ? 1 : 0;
		const int maxEntityNum = ( common->IsMultiplayer() && !ent->GetSkipReplication() ) ? ENTITYNUM_FIRST_NON_REPLICATED : ENTITYNUM_MAX_NORMAL;
		int freeIndex = firstFreeEntityIndex[ freeListType ];
		
		while( entities[ freeIndex ] != NULL && freeIndex < maxEntityNum )
		{
			freeIndex++;
		}
		if( freeIndex >= maxEntityNum )
		{
			Error( "no free entities[%d]", freeListType );
		}
		spawn_entnum = freeIndex++;
		
		firstFreeEntityIndex[ freeListType ] = freeIndex;
	}
	
	entities[ spawn_entnum ] = ent;
	spawnIds[ spawn_entnum ] = ( forceSpawnId >= 0 ) ? forceSpawnId : spawnCount++;
	ent->entityNumber = spawn_entnum;
	ent->spawnNode.AddToEnd( spawnedEntities );
	
	// Make a copy because TransferKeyValues clears the input parameter.
	idDict copiedArgs = spawnArgsToCopy;
	ent->spawnArgs.TransferKeyValues( copiedArgs );
	
	if( spawn_entnum >= num_entities )
	{
		num_entities++;
	}
}

/*
===================
idGameLocal::UnregisterEntity
===================
*/
void idGameLocal::UnregisterEntity( idEntity* ent )
{
	assert( ent );
	
	if( editEntities )
	{
		editEntities->RemoveSelectedEntity( ent );
	}
	
	if( !verify( ent->entityNumber < MAX_GENTITIES ) )
	{
		idLib::Error( "invalid entity number" );
		return;
	}
	
	if( ( ent->entityNumber != ENTITYNUM_NONE ) && ( entities[ ent->entityNumber ] == ent ) )
	{
		ent->spawnNode.Remove();
		entities[ ent->entityNumber ] = NULL;
		spawnIds[ ent->entityNumber ] = -1;
		
		int freeListType = ( common->IsMultiplayer() && ent->entityNumber >= ENTITYNUM_FIRST_NON_REPLICATED ) ? 1 : 0;
		
		if( ent->entityNumber >= MAX_CLIENTS && ent->entityNumber < firstFreeEntityIndex[ freeListType ] )
		{
			firstFreeEntityIndex[ freeListType ] = ent->entityNumber;
		}
		ent->entityNumber = ENTITYNUM_NONE;
	}
}

/*
================
idGameLocal::SpawnEntityType
================
*/
idEntity* idGameLocal::SpawnEntityType( const idTypeInfo& classdef, const idDict* args, bool bIsClientReadSnapshot )
{
	idClass* obj;
	
#if _DEBUG
	if( common->IsClient() )
	{
		assert( bIsClientReadSnapshot );
	}
#endif
	
	if( !classdef.IsType( idEntity::Type ) )
	{
		Error( "Attempted to spawn non-entity class '%s'", classdef.classname );
	}
	
	try
	{
		if( args )
		{
			spawnArgs = *args;
		}
		else
		{
			spawnArgs.Clear();
		}
		obj = classdef.CreateInstance();
		obj->CallSpawn();
	}
	
	catch( idAllocError& )
	{
		obj = NULL;
	}
	spawnArgs.Clear();
	
	return static_cast<idEntity*>( obj );
}

/*
===================
idGameLocal::SpawnEntityDef

Finds the spawn function for the entity and calls it,
returning false if not found
===================
*/
bool idGameLocal::SpawnEntityDef( const idDict& args, idEntity** ent, bool setDefaults )
{
	const char*	classname;
	const char*	spawn;
	idTypeInfo*	cls;
	idClass*		obj;
	idStr		error;
	const char*  name;
	
	if( ent )
	{
		*ent = NULL;
	}
	
	spawnArgs = args;
	
	if( spawnArgs.GetString( "name", "", &name ) )
	{
		sprintf( error, " on '%s'", name );
	}
	
	spawnArgs.GetString( "classname", NULL, &classname );
	
	const idDeclEntityDef* def = FindEntityDef( classname, false );
	
	if( !def )
	{
		Warning( "Unknown classname '%s'%s.", classname, error.c_str() );
		return false;
	}
	
	spawnArgs.SetDefaults( &def->dict );
	
	if( !spawnArgs.FindKey( "slowmo" ) )
	{
		bool slowmo = true;
		
		for( int i = 0; fastEntityList[i]; i++ )
		{
			if( !idStr::Cmp( classname, fastEntityList[i] ) )
			{
				slowmo = false;
				break;
			}
		}
		
		if( !slowmo )
		{
			spawnArgs.SetBool( "slowmo", slowmo );
		}
	}
	
	// check if we should spawn a class object
	spawnArgs.GetString( "spawnclass", NULL, &spawn );
	if( spawn )
	{
	
		cls = idClass::GetClass( spawn );
		if( !cls )
		{
			Warning( "Could not spawn '%s'.  Class '%s' not found%s.", classname, spawn, error.c_str() );
			return false;
		}
		
		obj = cls->CreateInstance();
		if( !obj )
		{
			Warning( "Could not spawn '%s'. Instance could not be created%s.", classname, error.c_str() );
			return false;
		}
		
		obj->CallSpawn();
		
		if( ent && obj->IsType( idEntity::Type ) )
		{
			*ent = static_cast<idEntity*>( obj );
		}
		
		return true;
	}
	
	// check if we should call a script function to spawn
	spawnArgs.GetString( "spawnfunc", NULL, &spawn );
	if( spawn )
	{
		const function_t* func = program.FindFunction( spawn );
		if( !func )
		{
			Warning( "Could not spawn '%s'.  Script function '%s' not found%s.", classname, spawn, error.c_str() );
			return false;
		}
		idThread* thread = new idThread( func );
		thread->DelayedStart( 0 );
		return true;
	}
	
	Warning( "%s doesn't include a spawnfunc or spawnclass%s.", classname, error.c_str() );
	return false;
}

/*
================
idGameLocal::FindEntityDef
================
*/
const idDeclEntityDef* idGameLocal::FindEntityDef( const char* name, bool makeDefault ) const
{
	const idDecl* decl = NULL;
	if( common->IsMultiplayer() )
	{
		decl = declManager->FindType( DECL_ENTITYDEF, va( "%s_mp", name ), false );
	}
	if( !decl )
	{
		decl = declManager->FindType( DECL_ENTITYDEF, name, makeDefault );
	}
	return static_cast<const idDeclEntityDef*>( decl );
}

/*
================
idGameLocal::FindEntityDefDict
================
*/
const idDict* idGameLocal::FindEntityDefDict( const char* name, bool makeDefault ) const
{
	const idDeclEntityDef* decl = FindEntityDef( name, makeDefault );
	return decl ? &decl->dict : NULL;
}

/*
================
idGameLocal::InhibitEntitySpawn
================
*/
bool idGameLocal::InhibitEntitySpawn( idDict& spawnArgs )
{

	bool result = false;
	
	if( common->IsMultiplayer() )
	{
		spawnArgs.GetBool( "not_multiplayer", "0", result );
	}
	else if( g_skill.GetInteger() == 0 )
	{
		spawnArgs.GetBool( "not_easy", "0", result );
	}
	else if( g_skill.GetInteger() == 1 )
	{
		spawnArgs.GetBool( "not_medium", "0", result );
	}
	else
	{
		spawnArgs.GetBool( "not_hard", "0", result );
		if( !result && g_skill.GetInteger() == 3 )
		{
			spawnArgs.GetBool( "not_nightmare", "0", result );
		}
	}
	
	if( g_skill.GetInteger() == 3 )
	{
		const char* name = spawnArgs.GetString( "classname" );
		// _D3XP :: remove moveable medkit packs also
		if( idStr::Icmp( name, "item_medkit" ) == 0 || idStr::Icmp( name, "item_medkit_small" ) == 0 ||
				idStr::Icmp( name, "moveable_item_medkit" ) == 0 || idStr::Icmp( name, "moveable_item_medkit_small" ) == 0 )
		{
		
			result = true;
		}
	}
	
	if( common->IsMultiplayer() )
	{
		const char* name = spawnArgs.GetString( "classname" );
		if( idStr::Icmp( name, "weapon_bfg" ) == 0 || idStr::Icmp( name, "weapon_soulcube" ) == 0 )
		{
			result = true;
		}
	}
	
	return result;
}

/*
==============
idGameLocal::GameState

Used to allow entities to know if they're being spawned during the initial spawn.
==============
*/
gameState_t	idGameLocal::GameState() const
{
	return gamestate;
}

/*
==============
idGameLocal::SpawnMapEntities

Parses textual entity definitions out of an entstring and spawns gentities.
==============
*/
void idGameLocal::SpawnMapEntities()
{
	int			i;
	int			num;
	int			inhibit;
	idMapEntity*	mapEnt;
	int			numEntities;
	idDict		args;
	
	Printf( "Spawning entities\n" );
	
	if( mapFile == NULL )
	{
		Printf( "No mapfile present\n" );
		return;
	}
	
	numEntities = mapFile->GetNumEntities();
	if( numEntities == 0 )
	{
		Error( "...no entities" );
	}
	
	// the worldspawn is a special that performs any global setup
	// needed by a level
	mapEnt = mapFile->GetEntity( 0 );
	args = mapEnt->epairs;
	args.SetInt( "spawn_entnum", ENTITYNUM_WORLD );
	if( !SpawnEntityDef( args ) || !entities[ ENTITYNUM_WORLD ] || !entities[ ENTITYNUM_WORLD ]->IsType( idWorldspawn::Type ) )
	{
		Error( "Problem spawning world entity" );
	}
	
	num = 1;
	inhibit = 0;
	
	for( i = 1 ; i < numEntities ; i++ )
	{
		common->UpdateLevelLoadPacifier();
		
		
		mapEnt = mapFile->GetEntity( i );
		args = mapEnt->epairs;
		
		if( !InhibitEntitySpawn( args ) )
		{
			// precache any media specified in the map entity
			CacheDictionaryMedia( &args );
			
			SpawnEntityDef( args );
			num++;
		}
		else
		{
			inhibit++;
		}
	}
	
	Printf( "...%i entities spawned, %i inhibited\n\n", num, inhibit );
}

/*
================
idGameLocal::AddEntityToHash
================
*/
void idGameLocal::AddEntityToHash( const char* name, idEntity* ent )
{
	if( FindEntity( name ) )
	{
		Error( "Multiple entities named '%s'", name );
	}
	entityHash.Add( entityHash.GenerateKey( name, true ), ent->entityNumber );
}

/*
================
idGameLocal::RemoveEntityFromHash
================
*/
bool idGameLocal::RemoveEntityFromHash( const char* name, idEntity* ent )
{
	int hash, i;
	
	hash = entityHash.GenerateKey( name, true );
	for( i = entityHash.First( hash ); i != -1; i = entityHash.Next( i ) )
	{
		if( entities[i] && entities[i] == ent && entities[i]->name.Icmp( name ) == 0 )
		{
			entityHash.Remove( hash, i );
			return true;
		}
	}
	return false;
}

/*
================
idGameLocal::GetTargets
================
*/
int idGameLocal::GetTargets( const idDict& args, idList< idEntityPtr<idEntity> >& list, const char* ref ) const
{
	int i, num, refLength;
	const idKeyValue* arg;
	idEntity* ent;
	
	list.Clear();
	
	refLength = strlen( ref );
	num = args.GetNumKeyVals();
	for( i = 0; i < num; i++ )
	{
	
		arg = args.GetKeyVal( i );
		if( arg->GetKey().Icmpn( ref, refLength ) == 0 )
		{
		
			ent = FindEntity( arg->GetValue() );
			if( ent )
			{
				idEntityPtr<idEntity>& entityPtr = list.Alloc();
				entityPtr = ent;
			}
		}
	}
	
	return list.Num();
}

/*
=============
idGameLocal::GetTraceEntity

returns the master entity of a trace.  for example, if the trace entity is the player's head, it will return the player.
=============
*/
idEntity* idGameLocal::GetTraceEntity( const trace_t& trace ) const
{
	idEntity* master;
	
	if( !entities[ trace.c.entityNum ] )
	{
		return NULL;
	}
	master = entities[ trace.c.entityNum ]->GetBindMaster();
	if( master )
	{
		return master;
	}
	return entities[ trace.c.entityNum ];
}

/*
=============
idGameLocal::ArgCompletion_EntityName

Argument completion for entity names
=============
*/
void idGameLocal::ArgCompletion_EntityName( const idCmdArgs& args, void( *callback )( const char* s ) )
{
	int i;
	
	for( i = 0; i < gameLocal.num_entities; i++ )
	{
		if( gameLocal.entities[ i ] )
		{
			callback( va( "%s %s", args.Argv( 0 ), gameLocal.entities[ i ]->name.c_str() ) );
		}
	}
}

/*
=============
idGameLocal::FindEntity

Returns the entity whose name matches the specified string.
=============
*/
idEntity* idGameLocal::FindEntity( const char* name ) const
{
	int hash, i;
	
	hash = entityHash.GenerateKey( name, true );
	for( i = entityHash.First( hash ); i != -1; i = entityHash.Next( i ) )
	{
		if( entities[i] && entities[i]->name.Icmp( name ) == 0 )
		{
			return entities[i];
		}
	}
	
	return NULL;
}

/*
=============
idGameLocal::FindEntityUsingDef

Searches all active entities for the next one using the specified entityDef.

Searches beginning at the entity after from, or the beginning if NULL
NULL will be returned if the end of the list is reached.
=============
*/
idEntity* idGameLocal::FindEntityUsingDef( idEntity* from, const char* match ) const
{
	idEntity*	ent;
	
	if( !from )
	{
		ent = spawnedEntities.Next();
	}
	else
	{
		ent = from->spawnNode.Next();
	}
	
	for( ; ent != NULL; ent = ent->spawnNode.Next() )
	{
		assert( ent );
		if( idStr::Icmp( ent->GetEntityDefName(), match ) == 0 )
		{
			return ent;
		}
	}
	
	return NULL;
}

/*
=============
idGameLocal::FindTraceEntity

Searches all active entities for the closest ( to start ) match that intersects
the line start,end
=============
*/
idEntity* idGameLocal::FindTraceEntity( idVec3 start, idVec3 end, const idTypeInfo& c, const idEntity* skip ) const
{
	idEntity* ent;
	idEntity* bestEnt;
	float scale;
	float bestScale;
	idBounds b;
	
	bestEnt = NULL;
	bestScale = 1.0f;
	for( ent = spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() )
	{
		if( ent->IsType( c ) && ent != skip )
		{
			b = ent->GetPhysics()->GetAbsBounds().Expand( 16 );
			if( b.RayIntersection( start, end - start, scale ) )
			{
				if( scale >= 0.0f && scale < bestScale )
				{
					bestEnt = ent;
					bestScale = scale;
				}
			}
		}
	}
	
	return bestEnt;
}

/*
================
idGameLocal::EntitiesWithinRadius
================
*/
int idGameLocal::EntitiesWithinRadius( const idVec3 org, float radius, idEntity** entityList, int maxCount ) const
{
	idEntity* ent;
	idBounds bo( org );
	int entCount = 0;
	
	bo.ExpandSelf( radius );
	for( ent = spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() )
	{
		if( ent->GetPhysics()->GetAbsBounds().IntersectsBounds( bo ) )
		{
			entityList[entCount++] = ent;
		}
	}
	
	return entCount;
}

/*
=================
idGameLocal::KillBox

Kills all entities that would touch the proposed new positioning of ent. The ent itself will not being killed.
Checks if player entities are in the teleporter, and marks them to die at teleport exit instead of immediately.
If catch_teleport, this only marks teleport players for death on exit
=================
*/
void idGameLocal::KillBox( idEntity* ent, bool catch_teleport )
{
	int			i;
	int			num;
	idEntity* 	hit;
	idClipModel* cm;
	idClipModel* clipModels[ MAX_GENTITIES ];
	idPhysics*	phys;
	
	phys = ent->GetPhysics();
	if( !phys->GetNumClipModels() )
	{
		return;
	}
	
	num = clip.ClipModelsTouchingBounds( phys->GetAbsBounds(), phys->GetClipMask(), clipModels, MAX_GENTITIES );
	for( i = 0; i < num; i++ )
	{
		cm = clipModels[ i ];
		
		// don't check render entities
		if( cm->IsRenderModel() )
		{
			continue;
		}
		
		hit = cm->GetEntity();
		if( ( hit == ent ) || !hit->fl.takedamage )
		{
			continue;
		}
		
		if( !phys->ClipContents( cm ) )
		{
			continue;
		}
		
		// nail it
		idPlayer* otherPlayer = NULL;
		if( hit->IsType( idPlayer::Type ) )
		{
			otherPlayer = static_cast< idPlayer* >( hit );
		}
		if( otherPlayer != NULL )
		{
			if( otherPlayer->IsInTeleport() )
			{
				otherPlayer->TeleportDeath( ent->entityNumber );
			}
			else if( !catch_teleport )
			{
				hit->Damage( ent, ent, vec3_origin, "damage_telefrag", 1.0f, INVALID_JOINT );
			}
		}
		else if( !catch_teleport )
		{
			hit->Damage( ent, ent, vec3_origin, "damage_telefrag", 1.0f, INVALID_JOINT );
		}
	}
}

/*
================
idGameLocal::RequirementMet
================
*/
bool idGameLocal::RequirementMet( idEntity* activator, const idStr& requires, int removeItem )
{
	if( requires.Length() )
	{
		if( activator->IsType( idPlayer::Type ) )
		{
			idPlayer* player = static_cast<idPlayer*>( activator );
			idDict* item = player->FindInventoryItem( requires );
			if( item )
			{
				if( removeItem )
				{
					player->RemoveInventoryItem( item );
				}
				return true;
			}
			else
			{
				return false;
			}
		}
	}
	
	return true;
}

/*
============
idGameLocal::AlertAI
============
*/
void idGameLocal::AlertAI( idEntity* ent )
{
	if( ent && ent->IsType( idActor::Type ) )
	{
		// alert them for the next frame
		lastAIAlertTime = time + 1;
		lastAIAlertEntity = static_cast<idActor*>( ent );
	}
}

/*
============
idGameLocal::GetAlertEntity
============
*/
idActor* idGameLocal::GetAlertEntity()
{
	int timeGroup = 0;
	if( lastAIAlertTime && lastAIAlertEntity.GetEntity() )
	{
		timeGroup = lastAIAlertEntity.GetEntity()->timeGroup;
	}
	SetTimeState ts( timeGroup );
	
	if( lastAIAlertTime >= time )
	{
		return lastAIAlertEntity.GetEntity();
	}
	
	return NULL;
}

/*
============
idGameLocal::RadiusDamage
============
*/
void idGameLocal::RadiusDamage( const idVec3& origin, idEntity* inflictor, idEntity* attacker, idEntity* ignoreDamage, idEntity* ignorePush, const char* damageDefName, float dmgPower )
{
	float		dist, damageScale, attackerDamageScale, attackerPushScale;
	idEntity* 	ent;
	idEntity* 	entityList[ MAX_GENTITIES ];
	int			numListedEntities;
	idBounds	bounds;
	idVec3 		v, damagePoint, dir;
	int			i, e, damage, radius, push;
	
	const idDict* damageDef = FindEntityDefDict( damageDefName, false );
	if( !damageDef )
	{
		Warning( "Unknown damageDef '%s'", damageDefName );
		return;
	}
	
	damageDef->GetInt( "damage", "20", damage );
	damageDef->GetInt( "radius", "50", radius );
	damageDef->GetInt( "push", va( "%d", damage * 100 ), push );
	damageDef->GetFloat( "attackerDamageScale", "0.5", attackerDamageScale );
	damageDef->GetFloat( "attackerPushScale", "0", attackerPushScale );
	
	if( radius < 1 )
	{
		radius = 1;
	}
	
	bounds = idBounds( origin ).Expand( radius );
	
	// get all entities touching the bounds
	numListedEntities = clip.EntitiesTouchingBounds( bounds, -1, entityList, MAX_GENTITIES );
	
	if( inflictor && inflictor->IsType( idAFAttachment::Type ) )
	{
		inflictor = static_cast<idAFAttachment*>( inflictor )->GetBody();
	}
	if( attacker && attacker->IsType( idAFAttachment::Type ) )
	{
		attacker = static_cast<idAFAttachment*>( attacker )->GetBody();
	}
	if( ignoreDamage && ignoreDamage->IsType( idAFAttachment::Type ) )
	{
		ignoreDamage = static_cast<idAFAttachment*>( ignoreDamage )->GetBody();
	}
	
	// apply damage to the entities
	for( e = 0; e < numListedEntities; e++ )
	{
		ent = entityList[ e ];
		assert( ent );
		
		if( !ent->fl.takedamage )
		{
			continue;
		}
		
		if( ent == inflictor || ( ent->IsType( idAFAttachment::Type ) && static_cast<idAFAttachment*>( ent )->GetBody() == inflictor ) )
		{
			continue;
		}
		
		if( ent == ignoreDamage || ( ent->IsType( idAFAttachment::Type ) && static_cast<idAFAttachment*>( ent )->GetBody() == ignoreDamage ) )
		{
			continue;
		}
		
		// don't damage a dead player
		if( common->IsMultiplayer() && ent->entityNumber < MAX_CLIENTS && ent->IsType( idPlayer::Type ) && static_cast< idPlayer* >( ent )->health < 0 )
		{
			continue;
		}
		
		// find the distance from the edge of the bounding box
		for( i = 0; i < 3; i++ )
		{
			if( origin[ i ] < ent->GetPhysics()->GetAbsBounds()[0][ i ] )
			{
				v[ i ] = ent->GetPhysics()->GetAbsBounds()[0][ i ] - origin[ i ];
			}
			else if( origin[ i ] > ent->GetPhysics()->GetAbsBounds()[1][ i ] )
			{
				v[ i ] = origin[ i ] - ent->GetPhysics()->GetAbsBounds()[1][ i ];
			}
			else
			{
				v[ i ] = 0;
			}
		}
		
		dist = v.Length();
		if( dist >= radius )
		{
			continue;
		}
		
		if( ent->CanDamage( origin, damagePoint ) )
		{
			// push the center of mass higher than the origin so players
			// get knocked into the air more
			dir = ent->GetPhysics()->GetOrigin() - origin;
			dir[ 2 ] += 24;
			
			// get the damage scale
			damageScale = dmgPower * ( 1.0f - dist / radius );
			if( ent == attacker || ( ent->IsType( idAFAttachment::Type ) && static_cast<idAFAttachment*>( ent )->GetBody() == attacker ) )
			{
				damageScale *= attackerDamageScale;
			}
			
			bool killedBySplash = true;
			
			if( ent->health <= 0 )
			{
				killedBySplash = false;
			}
			
			ent->Damage( inflictor, attacker, dir, damageDefName, damageScale, INVALID_JOINT );
			
			
			// If the player is local. SHAkkkkkkeeee!
			if( !common->IsMultiplayer() &&  ent->entityNumber == GetLocalClientNum() )
			{
			
				if( ent->IsType( idPlayer::Type ) )
				{
					idPlayer* player = static_cast< idPlayer* >( ent );
					if( player )
					{
						player->ControllerShakeFromDamage( damage );
					}
				}
			}
			
			// if our inflictor is a projectile, we want to count up how many kills the splash damage has chalked up.
			if( ent->health <= 0 && killedBySplash )
			{
				if( inflictor && inflictor->IsType( idProjectile::Type ) )
				{
					if( attacker && attacker->IsType( idPlayer::Type ) )
					{
						if( ent->IsType( idActor::Type ) && ent != attacker )
						{
							idPlayer* player = static_cast<idPlayer*>( attacker );
							player->AddProjectileKills();
						}
					}
				}
			}
			
		}
	}
	
	// push physics objects
	if( push )
	{
		RadiusPush( origin, radius, push * dmgPower, attacker, ignorePush, attackerPushScale, false );
	}
}

/*
==============
idGameLocal::RadiusPush
==============
*/
void idGameLocal::RadiusPush( const idVec3& origin, const float radius, const float push, const idEntity* inflictor, const idEntity* ignore, float inflictorScale, const bool quake )
{
	int i, numListedClipModels;
	idClipModel* clipModel;
	idClipModel* clipModelList[ MAX_GENTITIES ];
	idVec3 dir;
	idBounds bounds;
	modelTrace_t result;
	idEntity* ent;
	float scale;
	
	dir.Set( 0.0f, 0.0f, 1.0f );
	
	bounds = idBounds( origin ).Expand( radius );
	
	// get all clip models touching the bounds
	numListedClipModels = clip.ClipModelsTouchingBounds( bounds, -1, clipModelList, MAX_GENTITIES );
	
	if( inflictor && inflictor->IsType( idAFAttachment::Type ) )
	{
		inflictor = static_cast<const idAFAttachment*>( inflictor )->GetBody();
	}
	if( ignore && ignore->IsType( idAFAttachment::Type ) )
	{
		ignore = static_cast<const idAFAttachment*>( ignore )->GetBody();
	}
	
	// apply impact to all the clip models through their associated physics objects
	for( i = 0; i < numListedClipModels; i++ )
	{
	
		clipModel = clipModelList[i];
		
		// never push render models
		if( clipModel->IsRenderModel() )
		{
			continue;
		}
		
		ent = clipModel->GetEntity();
		
		// never push projectiles
		if( ent->IsType( idProjectile::Type ) )
		{
			continue;
		}
		
		// players use "knockback" in idPlayer::Damage
		if( ent->IsType( idPlayer::Type ) && !quake )
		{
			continue;
		}
		
		// don't push the ignore entity
		if( ent == ignore || ( ent->IsType( idAFAttachment::Type ) && static_cast<idAFAttachment*>( ent )->GetBody() == ignore ) )
		{
			continue;
		}
		
		if( gameRenderWorld->FastWorldTrace( result, origin, clipModel->GetOrigin() ) )
		{
			continue;
		}
		
		// scale the push for the inflictor
		if( ent == inflictor || ( ent->IsType( idAFAttachment::Type ) && static_cast<idAFAttachment*>( ent )->GetBody() == inflictor ) )
		{
			scale = inflictorScale;
		}
		else
		{
			scale = 1.0f;
		}
		
		if( quake )
		{
			clipModel->GetEntity()->ApplyImpulse( world, clipModel->GetId(), clipModel->GetOrigin(), scale * push * dir );
		}
		else
		{
			RadiusPushClipModel( origin, scale * push, clipModel );
		}
	}
}

/*
==============
idGameLocal::RadiusPushClipModel
==============
*/
void idGameLocal::RadiusPushClipModel( const idVec3& origin, const float push, const idClipModel* clipModel )
{
	int i, j;
	float dot, dist, area;
	const idTraceModel* trm;
	const traceModelPoly_t* poly;
	idFixedWinding w;
	idVec3 v, localOrigin, center, impulse;
	
	trm = clipModel->GetTraceModel();
	if( !trm || 1 )
	{
		impulse = clipModel->GetAbsBounds().GetCenter() - origin;
		impulse.Normalize();
		impulse.z += 1.0f;
		clipModel->GetEntity()->ApplyImpulse( world, clipModel->GetId(), clipModel->GetOrigin(), push * impulse );
		return;
	}
	
	localOrigin = ( origin - clipModel->GetOrigin() ) * clipModel->GetAxis().Transpose();
	for( i = 0; i < trm->numPolys; i++ )
	{
		poly = &trm->polys[i];
		
		center.Zero();
		for( j = 0; j < poly->numEdges; j++ )
		{
			v = trm->verts[ trm->edges[ abs( poly->edges[j] ) ].v[ INT32_SIGNBITSET( poly->edges[j] ) ] ];
			center += v;
			v -= localOrigin;
			v.NormalizeFast();	// project point on a unit sphere
			w.AddPoint( v );
		}
		center /= poly->numEdges;
		v = center - localOrigin;
		dist = v.NormalizeFast();
		dot = v * poly->normal;
		if( dot > 0.0f )
		{
			continue;
		}
		area = w.GetArea();
		// impulse in polygon normal direction
		impulse = poly->normal * clipModel->GetAxis();
		// always push up for nicer effect
		impulse.z -= 1.0f;
		// scale impulse based on visible surface area and polygon angle
		impulse *= push * ( dot * area * ( 1.0f / ( 4.0f * idMath::PI ) ) );
		// scale away distance for nicer effect
		impulse *= ( dist * 2.0f );
		// impulse is applied to the center of the polygon
		center = clipModel->GetOrigin() + center * clipModel->GetAxis();
		
		clipModel->GetEntity()->ApplyImpulse( world, clipModel->GetId(), center, impulse );
	}
}

/*
===============
idGameLocal::ProjectDecal
===============
*/
void idGameLocal::ProjectDecal( const idVec3& origin, const idVec3& dir, float depth, bool parallel, float size, const char* material, float angle )
{
	float s, c;
	idMat3 axis, axistemp;
	idFixedWinding winding;
	idVec3 windingOrigin, projectionOrigin;
	
	static idVec3 decalWinding[4] =
	{
		idVec3( 1.0f,  1.0f, 0.0f ),
		idVec3( -1.0f,  1.0f, 0.0f ),
		idVec3( -1.0f, -1.0f, 0.0f ),
		idVec3( 1.0f, -1.0f, 0.0f )
	};
	
	if( !g_decals.GetBool() )
	{
		return;
	}
	
	// randomly rotate the decal winding
	idMath::SinCos16( ( angle ) ? angle : random.RandomFloat() * idMath::TWO_PI, s, c );
	
	// winding orientation
	axis[2] = dir;
	axis[2].Normalize();
	axis[2].NormalVectors( axistemp[0], axistemp[1] );
	axis[0] = axistemp[ 0 ] * c + axistemp[ 1 ] * -s;
	axis[1] = axistemp[ 0 ] * -s + axistemp[ 1 ] * -c;
	
	windingOrigin = origin + depth * axis[2];
	if( parallel )
	{
		projectionOrigin = origin - depth * axis[2];
	}
	else
	{
		projectionOrigin = origin;
	}
	
	size *= 0.5f;
	
	winding.Clear();
	winding += idVec5( windingOrigin + ( axis * decalWinding[0] ) * size, idVec2( 1, 1 ) );
	winding += idVec5( windingOrigin + ( axis * decalWinding[1] ) * size, idVec2( 0, 1 ) );
	winding += idVec5( windingOrigin + ( axis * decalWinding[2] ) * size, idVec2( 0, 0 ) );
	winding += idVec5( windingOrigin + ( axis * decalWinding[3] ) * size, idVec2( 1, 0 ) );
	gameRenderWorld->ProjectDecalOntoWorld( winding, projectionOrigin, parallel, depth * 0.5f, declManager->FindMaterial( material ), gameLocal.slow.time /* _D3XP */ );
}

/*
==============
idGameLocal::BloodSplat
==============
*/
void idGameLocal::BloodSplat( const idVec3& origin, const idVec3& dir, float size, const char* material )
{
	float halfSize = size * 0.5f;
	idVec3 verts[] = {	idVec3( 0.0f, +halfSize, +halfSize ),
						idVec3( 0.0f, +halfSize, -halfSize ),
						idVec3( 0.0f, -halfSize, -halfSize ),
						idVec3( 0.0f, -halfSize, +halfSize )
					 };
	idTraceModel trm;
	idClipModel mdl;
	trace_t results;
	
	// FIXME: get from damage def
	if( !g_bloodEffects.GetBool() )
	{
		return;
	}
	
	size = halfSize + random.RandomFloat() * halfSize;
	trm.SetupPolygon( verts, 4 );
	mdl.LoadModel( trm );
	clip.Translation( results, origin, origin + dir * 64.0f, &mdl, mat3_identity, CONTENTS_SOLID, NULL );
	ProjectDecal( results.endpos, dir, 2.0f * size, true, size, material );
}

/*
=============
idGameLocal::SetCamera
=============
*/
void idGameLocal::SetCamera( idCamera* cam )
{
	int i;
	idEntity* ent;
	idAI* ai;
	
	// this should fix going into a cinematic when dead.. rare but happens
	idPlayer* client = GetLocalPlayer();
	if( client->health <= 0 || client->AI_DEAD )
	{
		return;
	}
	
	camera = cam;
	if( camera )
	{
		inCinematic = true;
		
		if( skipCinematic && camera->spawnArgs.GetBool( "disconnect" ) )
		{
			camera->spawnArgs.SetBool( "disconnect", false );
			cvarSystem->SetCVarFloat( "r_znear", 3.0f );
			cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "disconnect\n" );
			skipCinematic = false;
			return;
		}
		
		if( time > cinematicStopTime )
		{
			cinematicSkipTime = time + CINEMATIC_SKIP_DELAY;
		}
		
		// set r_znear so that transitioning into/out of the player's head doesn't clip through the view
		cvarSystem->SetCVarFloat( "r_znear", 1.0f );
		
		// hide all the player models
		for( i = 0; i < numClients; i++ )
		{
			if( entities[ i ] )
			{
				client = static_cast< idPlayer* >( entities[ i ] );
				client->EnterCinematic();
			}
		}
		
		if( !cam->spawnArgs.GetBool( "ignore_enemies" ) )
		{
			// kill any active monsters that are enemies of the player
			for( ent = spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() )
			{
				if( ent->cinematic || ent->fl.isDormant )
				{
					// only kill entities that aren't needed for cinematics and aren't dormant
					continue;
				}
				
				if( ent->IsType( idAI::Type ) )
				{
					ai = static_cast<idAI*>( ent );
					if( !ai->GetEnemy() || !ai->IsActive() )
					{
						// no enemy, or inactive, so probably safe to ignore
						continue;
					}
				}
				else if( ent->IsType( idProjectile::Type ) )
				{
					// remove all projectiles
				}
				else if( ent->spawnArgs.GetBool( "cinematic_remove" ) )
				{
					// remove anything marked to be removed during cinematics
				}
				else
				{
					// ignore everything else
					continue;
				}
				
				// remove it
				DPrintf( "removing '%s' for cinematic\n", ent->GetName() );
				ent->PostEventMS( &EV_Remove, 0 );
			}
		}
		
	}
	else
	{
		inCinematic = false;
		cinematicStopTime = time + 1;
		
		// restore r_znear
		cvarSystem->SetCVarFloat( "r_znear", 3.0f );
		
		// show all the player models
		for( i = 0; i < numClients; i++ )
		{
			if( entities[ i ] )
			{
				idPlayer* client = static_cast< idPlayer* >( entities[ i ] );
				client->ExitCinematic();
			}
		}
	}
}

/*
=============
idGameLocal::GetCamera
=============
*/
idCamera* idGameLocal::GetCamera() const
{
	return camera;
}

/*
=============
idGameLocal::SkipCinematic
=============
*/
bool idGameLocal::SkipCinematic( void )
{
	if( camera )
	{
		if( camera->spawnArgs.GetBool( "disconnect" ) )
		{
			camera->spawnArgs.SetBool( "disconnect", false );
			cvarSystem->SetCVarFloat( "r_znear", 3.0f );
			cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "disconnect\n" );
			skipCinematic = false;
			return false;
		}
		
		if( camera->spawnArgs.GetBool( "instantSkip" ) )
		{
			camera->Stop();
			return false;
		}
	}
	
	soundSystem->SetMute( true );
	if( !skipCinematic )
	{
		skipCinematic = true;
		cinematicMaxSkipTime = gameLocal.time + SEC2MS( g_cinematicMaxSkipTime.GetFloat() );
	}
	
	return true;
}
/*
=============
idGameLocal::SkipCinematicScene
=============
*/
bool idGameLocal::SkipCinematicScene()
{
	return SkipCinematic();
}
/*
=============
idGameLocal::CheckInCinematic
=============
*/
bool idGameLocal::CheckInCinematic()
{
	return inCinematic;
}
/*
======================
idGameLocal::SpreadLocations

Now that everything has been spawned, associate areas with location entities
======================
*/
void idGameLocal::SpreadLocations()
{
	idEntity* ent;
	
	// allocate the area table
	int	numAreas = gameRenderWorld->NumAreas();
	locationEntities = new( TAG_GAME ) idLocationEntity *[ numAreas ];
	memset( locationEntities, 0, numAreas * sizeof( *locationEntities ) );
	
	// for each location entity, make pointers from every area it touches
	for( ent = spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() )
	{
		if( !ent->IsType( idLocationEntity::Type ) )
		{
			continue;
		}
		idVec3	point = ent->spawnArgs.GetVector( "origin" );
		int areaNum = gameRenderWorld->PointInArea( point );
		if( areaNum < 0 )
		{
			Printf( "SpreadLocations: location '%s' is not in a valid area\n", ent->spawnArgs.GetString( "name" ) );
			continue;
		}
		if( areaNum >= numAreas )
		{
			Error( "idGameLocal::SpreadLocations: areaNum >= gameRenderWorld->NumAreas()" );
		}
		if( locationEntities[areaNum] )
		{
			Warning( "location entity '%s' overlaps '%s'", ent->spawnArgs.GetString( "name" ),
					 locationEntities[areaNum]->spawnArgs.GetString( "name" ) );
			continue;
		}
		locationEntities[areaNum] = static_cast<idLocationEntity*>( ent );
		
		// spread to all other connected areas
		for( int i = 0 ; i < numAreas ; i++ )
		{
			if( i == areaNum )
			{
				continue;
			}
			if( gameRenderWorld->AreasAreConnected( areaNum, i, PS_BLOCK_LOCATION ) )
			{
				locationEntities[i] = static_cast<idLocationEntity*>( ent );
			}
		}
	}
}

/*
===================
idGameLocal::LocationForPoint

The player checks the location each frame to update the HUD text display
May return NULL
===================
*/
idLocationEntity* idGameLocal::LocationForPoint( const idVec3& point )
{
	if( !locationEntities )
	{
		// before SpreadLocations() has been called
		return NULL;
	}
	
	int areaNum = gameRenderWorld->PointInArea( point );
	if( areaNum < 0 )
	{
		return NULL;
	}
	if( areaNum >= gameRenderWorld->NumAreas() )
	{
		Error( "idGameLocal::LocationForPoint: areaNum >= gameRenderWorld->NumAreas()" );
	}
	
	return locationEntities[ areaNum ];
}

/*
============
idGameLocal::SetPortalState
============
*/
void idGameLocal::SetPortalState( qhandle_t portal, int blockingBits )
{
	gameRenderWorld->SetPortalState( portal, blockingBits );
}

/*
============
idGameLocal::sortSpawnPoints
============
*/
int idGameLocal::sortSpawnPoints( const void* ptr1, const void* ptr2 )
{
	const spawnSpot_t* spot1 = static_cast<const spawnSpot_t*>( ptr1 );
	const spawnSpot_t* spot2 = static_cast<const spawnSpot_t*>( ptr2 );
	float diff;
	
	diff = spot1->dist - spot2->dist;
	if( diff < 0.0f )
	{
		return 1;
	}
	else if( diff > 0.0f )
	{
		return -1;
	}
	else
	{
		return 0;
	}
}

/*
===========
idGameLocal::RandomizeInitialSpawns
randomize the order of the initial spawns
prepare for a sequence of initial player spawns
============
*/
void idGameLocal::RandomizeInitialSpawns()
{
	spawnSpot_t	spot;
	int i, j;
	int k;
	
	idEntity* ent;
	
	if( !common->IsServer() )
	{
		return;
	}
	spawnSpots.Clear();
	initialSpots.Clear();
	teamSpawnSpots[0].Clear();
	teamSpawnSpots[1].Clear();
	teamInitialSpots[0].Clear();
	teamInitialSpots[1].Clear();
	
	spot.dist = 0;
	spot.ent = FindEntityUsingDef( NULL, "info_player_deathmatch" );
	while( spot.ent )
	{
		spot.ent->spawnArgs.GetInt( "team", "-1", spot.team );
		
		if( mpGame.IsGametypeFlagBased() )  /* CTF */
		{
			if( spot.team == 0 || spot.team == 1 )
				teamSpawnSpots[spot.team].Append( spot );
			else
				common->Warning( "info_player_deathmatch : invalid or no team attached to spawn point\n" );
		}
		spawnSpots.Append( spot );
		if( spot.ent->spawnArgs.GetBool( "initial" ) )
		{
			if( mpGame.IsGametypeFlagBased() )  /* CTF */
			{
				assert( spot.team == 0 || spot.team == 1 );
				teamInitialSpots[ spot.team ].Append( spot.ent );
			}
			
			initialSpots.Append( spot.ent );
		}
		spot.ent = FindEntityUsingDef( spot.ent, "info_player_deathmatch" );
	}
	
	if( mpGame.IsGametypeFlagBased() )    /* CTF */
	{
		if( !teamSpawnSpots[0].Num() )
			common->Warning( "red team : no info_player_deathmatch in map" );
		if( !teamSpawnSpots[1].Num() )
			common->Warning( "blue team : no info_player_deathmatch in map" );
			
		if( !teamSpawnSpots[0].Num() || !teamSpawnSpots[1].Num() )
			return;
	}
	
	if( !spawnSpots.Num() )
	{
		common->Warning( "no info_player_deathmatch in map" );
		return;
	}
	
	if( mpGame.IsGametypeFlagBased() )    /* CTF */
	{
		common->Printf( "red team : %d spawns (%d initials)\n", teamSpawnSpots[ 0 ].Num(), teamInitialSpots[ 0 ].Num() );
		// if there are no initial spots in the map, consider they can all be used as initial
		if( !teamInitialSpots[ 0 ].Num() )
		{
			common->Warning( "red team : no info_player_deathmatch entities marked initial in map" );
			for( i = 0; i < teamSpawnSpots[ 0 ].Num(); i++ )
			{
				teamInitialSpots[ 0 ].Append( teamSpawnSpots[ 0 ][ i ].ent );
			}
		}
		
		common->Printf( "blue team : %d spawns (%d initials)\n", teamSpawnSpots[ 1 ].Num(), teamInitialSpots[ 1 ].Num() );
		// if there are no initial spots in the map, consider they can all be used as initial
		if( !teamInitialSpots[ 1 ].Num() )
		{
			common->Warning( "blue team : no info_player_deathmatch entities marked initial in map" );
			for( i = 0; i < teamSpawnSpots[ 1 ].Num(); i++ )
			{
				teamInitialSpots[ 1 ].Append( teamSpawnSpots[ 1 ][ i ].ent );
			}
		}
	}
	
	common->Printf( "%d spawns (%d initials)\n", spawnSpots.Num(), initialSpots.Num() );
	// if there are no initial spots in the map, consider they can all be used as initial
	if( !initialSpots.Num() )
	{
		common->Warning( "no info_player_deathmatch entities marked initial in map" );
		for( i = 0; i < spawnSpots.Num(); i++ )
		{
			initialSpots.Append( spawnSpots[ i ].ent );
		}
	}
	
	for( k = 0; k < 2; k++ )
	{
		for( i = 0; i < teamInitialSpots[ k ].Num(); i++ )
		{
			j = random.RandomInt( teamInitialSpots[ k ].Num() );
			ent = teamInitialSpots[ k ][ i ];
			teamInitialSpots[ k ][ i ] = teamInitialSpots[ k ][ j ];
			teamInitialSpots[ k ][ j ] = ent;
		}
	}
	
	for( i = 0; i < initialSpots.Num(); i++ )
	{
		j = random.RandomInt( initialSpots.Num() );
		ent = initialSpots[ i ];
		initialSpots[ i ] = initialSpots[ j ];
		initialSpots[ j ] = ent;
	}
	// reset the counter
	currentInitialSpot = 0;
	
	teamCurrentInitialSpot[0] = 0;
	teamCurrentInitialSpot[1] = 0;
}

/*
===========
idGameLocal::SelectInitialSpawnPoint
spectators are spawned randomly anywhere
in-game clients are spawned based on distance to active players (randomized on the first half)
upon map restart, initial spawns are used (randomized ordered list of spawns flagged "initial")
  if there are more players than initial spots, overflow to regular spawning
============
*/
idEntity* idGameLocal::SelectInitialSpawnPoint( idPlayer* player )
{
	int				i, j, which;
	spawnSpot_t		spot;
	idVec3			pos;
	float			dist;
	bool			alone;
	
	if( !common->IsMultiplayer() || !spawnSpots.Num() || ( mpGame.IsGametypeFlagBased() && ( !teamSpawnSpots[0].Num() || !teamSpawnSpots[1].Num() ) ) )    /* CTF */
	{
		spot.ent = FindEntityUsingDef( NULL, "info_player_start" );
		if( !spot.ent )
		{
			Error( "No info_player_start on map.\n" );
		}
		return spot.ent;
	}
	
	bool useInitialSpots = false;
	if( mpGame.IsGametypeFlagBased() )    /* CTF */
	{
		assert( player->team == 0 || player->team == 1 );
		useInitialSpots = player->useInitialSpawns && teamCurrentInitialSpot[ player->team ] < teamInitialSpots[ player->team ].Num();
	}
	else
	{
		useInitialSpots = player->useInitialSpawns && currentInitialSpot < initialSpots.Num();
	}
	
	if( player->spectating )
	{
		// plain random spot, don't bother
		return spawnSpots[ random.RandomInt( spawnSpots.Num() ) ].ent;
	}
	else if( useInitialSpots )
	{
		if( mpGame.IsGametypeFlagBased() )    /* CTF */
		{
			assert( player->team == 0 || player->team == 1 );
			player->useInitialSpawns = false;							// only use the initial spawn once
			return teamInitialSpots[ player->team ][ teamCurrentInitialSpot[ player->team ]++ ];
		}
		return initialSpots[ currentInitialSpot++ ];
	}
	else
	{
		// check if we are alone in map
		alone = true;
		for( j = 0; j < MAX_CLIENTS; j++ )
		{
			if( entities[ j ] && entities[ j ] != player )
			{
				alone = false;
				break;
			}
		}
		if( alone )
		{
			if( mpGame.IsGametypeFlagBased() )    /* CTF */
			{
				assert( player->team == 0 || player->team == 1 );
				return teamSpawnSpots[ player->team ][ random.RandomInt( teamSpawnSpots[ player->team ].Num() ) ].ent;
			}
			// don't do distance-based
			return spawnSpots[ random.RandomInt( spawnSpots.Num() ) ].ent;
		}
		
		if( mpGame.IsGametypeFlagBased() )    /* CTF */
		{
			// TODO : make as reusable method, same code as below
			int team = player->team;
			assert( team == 0 || team == 1 );
			
			// find the distance to the closest active player for each spawn spot
			for( i = 0; i < teamSpawnSpots[ team ].Num(); i++ )
			{
				pos = teamSpawnSpots[ team ][ i ].ent->GetPhysics()->GetOrigin();
				
				// skip initial spawn points for CTF
				if( teamSpawnSpots[ team ][ i ].ent->spawnArgs.GetBool( "initial" ) )
				{
					teamSpawnSpots[ team ][ i ].dist = 0x0;
					continue;
				}
				
				teamSpawnSpots[ team ][ i ].dist = 0x7fffffff;
				
				for( j = 0; j < MAX_CLIENTS; j++ )
				{
					if( !entities[ j ] || !entities[ j ]->IsType( idPlayer::Type )
							|| entities[ j ] == player
							|| static_cast< idPlayer* >( entities[ j ] )->spectating )
					{
						continue;
					}
					
					dist = ( pos - entities[ j ]->GetPhysics()->GetOrigin() ).LengthSqr();
					if( dist < teamSpawnSpots[ team ][ i ].dist )
					{
						teamSpawnSpots[ team ][ i ].dist = dist;
					}
				}
			}
			
			// sort the list
			qsort( ( void* )teamSpawnSpots[ team ].Ptr(), teamSpawnSpots[ team ].Num(), sizeof( spawnSpot_t ), ( int ( * )( const void*, const void* ) )sortSpawnPoints );
			
			// choose a random one in the top half
			which = random.RandomInt( teamSpawnSpots[ team ].Num() / 2 );
			spot = teamSpawnSpots[ team ][ which ];
//			assert( teamSpawnSpots[ team ][ which ].dist != 0 );

			return spot.ent;
		}
		
		// find the distance to the closest active player for each spawn spot
		for( i = 0; i < spawnSpots.Num(); i++ )
		{
			pos = spawnSpots[ i ].ent->GetPhysics()->GetOrigin();
			spawnSpots[ i ].dist = 0x7fffffff;
			for( j = 0; j < MAX_CLIENTS; j++ )
			{
				if( !entities[ j ] || !entities[ j ]->IsType( idPlayer::Type )
						|| entities[ j ] == player
						|| static_cast< idPlayer* >( entities[ j ] )->spectating )
				{
					continue;
				}
				
				dist = ( pos - entities[ j ]->GetPhysics()->GetOrigin() ).LengthSqr();
				if( dist < spawnSpots[ i ].dist )
				{
					spawnSpots[ i ].dist = dist;
				}
			}
		}
		
		// sort the list
		qsort( ( void* )spawnSpots.Ptr(), spawnSpots.Num(), sizeof( spawnSpot_t ), ( int ( * )( const void*, const void* ) )sortSpawnPoints );
		
		// choose a random one in the top half
		which = random.RandomInt( spawnSpots.Num() / 2 );
		spot = spawnSpots[ which ];
	}
	return spot.ent;
}

/*
================
idGameLocal::SetGlobalMaterial
================
*/
void idGameLocal::SetGlobalMaterial( const idMaterial* mat )
{
	globalMaterial = mat;
}

/*
================
idGameLocal::GetGlobalMaterial
================
*/
const idMaterial* idGameLocal::GetGlobalMaterial()
{
	return globalMaterial;
}

/*
================
idGameLocal::GetSpawnId
================
*/
int idGameLocal::GetSpawnId( const idEntity* ent ) const
{
	return ( gameLocal.spawnIds[ ent->entityNumber ] << GENTITYNUM_BITS ) | ent->entityNumber;
}


/*
=================
idPlayer::SetPortalSkyEnt
=================
*/
void idGameLocal::SetPortalSkyEnt( idEntity* ent )
{
	portalSkyEnt = ent;
}

/*
=================
idPlayer::IsPortalSkyAcive
=================
*/
bool idGameLocal::IsPortalSkyAcive()
{
	return portalSkyActive;
}

/*
===========
idGameLocal::SelectTimeGroup
============
*/
void idGameLocal::SelectTimeGroup( int timeGroup )
{
	if( timeGroup )
	{
		fast.Get( time, previousTime, realClientTime );
	}
	else
	{
		slow.Get( time, previousTime, realClientTime );
	}
	
	selectedGroup = timeGroup;
}

/*
===========
idGameLocal::GetTimeGroupTime
============
*/
int idGameLocal::GetTimeGroupTime( int timeGroup )
{
	if( timeGroup )
	{
		return fast.time;
	}
	else
	{
		return slow.time;
	}
}

/*
===========
idGameLocal::ComputeSlowScale
============
*/
void idGameLocal::ComputeSlowScale()
{

	// check if we need to do a quick reset
	if( quickSlowmoReset )
	{
		quickSlowmoReset = false;
		
		// stop the sounds
		if( gameSoundWorld )
		{
			gameSoundWorld->SetSlowmoSpeed( 1.0f );
		}
		
		// stop the state
		slowmoState = SLOWMO_STATE_OFF;
		slowmoScale = 1.0f;
	}
	
	// check the player state
	idPlayer* player = GetLocalPlayer();
	bool powerupOn = false;
	
	if( player != NULL && player->PowerUpActive( HELLTIME ) )
	{
		powerupOn = true;
	}
	else if( g_enableSlowmo.GetBool() )
	{
		powerupOn = true;
	}
	
	// determine proper slowmo state
	if( powerupOn && slowmoState == SLOWMO_STATE_OFF )
	{
		slowmoState = SLOWMO_STATE_RAMPUP;
		
		if( gameSoundWorld )
		{
			gameSoundWorld->SetSlowmoSpeed( slowmoScale );
		}
	}
	else if( !powerupOn && slowmoState == SLOWMO_STATE_ON )
	{
		slowmoState = SLOWMO_STATE_RAMPDOWN;
		
		// play the stop sound
		if( player != NULL )
		{
			player->PlayHelltimeStopSound();
		}
	}
	
	// do any necessary ramping
	if( slowmoState == SLOWMO_STATE_RAMPUP )
	{
		float delta = ( 0.25f - slowmoScale );
		
		if( fabs( delta ) < g_slowmoStepRate.GetFloat() )
		{
			slowmoScale = 0.25f;
			slowmoState = SLOWMO_STATE_ON;
		}
		else
		{
			slowmoScale += delta * g_slowmoStepRate.GetFloat();
		}
		
		if( gameSoundWorld != NULL )
		{
			gameSoundWorld->SetSlowmoSpeed( slowmoScale );
		}
	}
	else if( slowmoState == SLOWMO_STATE_RAMPDOWN )
	{
		float delta = ( 1.0f - slowmoScale );
		
		if( fabs( delta ) < g_slowmoStepRate.GetFloat() )
		{
			slowmoScale = 1.0f;
			slowmoState = SLOWMO_STATE_OFF;
		}
		else
		{
			slowmoScale += delta * g_slowmoStepRate.GetFloat();
		}
		
		if( gameSoundWorld != NULL )
		{
			gameSoundWorld->SetSlowmoSpeed( slowmoScale );
		}
	}
}

/*
===========
idGameLocal::ResetSlowTimeVars
============
*/
void idGameLocal::ResetSlowTimeVars()
{
	slowmoScale			= 1.0f;
	slowmoState			= SLOWMO_STATE_OFF;
	
	fast.previousTime	= 0;
	fast.time			= 0;
	
	slow.previousTime	= 0;
	slow.time			= 0;
}

/*
===========
idGameLocal::QuickSlowmoReset
============
*/
void idGameLocal::QuickSlowmoReset()
{
	quickSlowmoReset = true;
}


/*
================
idGameLocal::GetClientStats
================
*/
void idGameLocal::GetClientStats( int clientNum, char* data, const int len )
{
	mpGame.PlayerStats( clientNum, data, len );
}

/*
================
idGameLocal::GetMPGameModes
================
*/
int idGameLocal::GetMPGameModes( const char** * gameModes, const char** * gameModesDisplay )
{
	return mpGame.GetGameModes( gameModes, gameModesDisplay );
}

/*
========================
idGameLocal::IsPDAOpen
========================
*/
bool idGameLocal::IsPDAOpen() const
{
	return GetLocalPlayer() && GetLocalPlayer()->objectiveSystemOpen;
}

/*
========================
idGameLocal::IsPlayerChatting
========================
*/
bool idGameLocal::IsPlayerChatting() const
{
	return GetLocalPlayer() && ( GetLocalPlayer()->isChatting > 0 ) && !GetLocalPlayer()->spectating;
}

/*
========================
idGameLocal::InhibitControls
========================
*/
bool idGameLocal::InhibitControls()
{
	return ( Shell_IsActive() || IsPDAOpen() || IsPlayerChatting() || ( common->IsMultiplayer() && mpGame.IsScoreboardActive() ) );
}

/*
===============================
idGameLocal::Leaderboards_Init
===============================
*/
void idGameLocal::Leaderboards_Init()
{
	LeaderboardLocal_Init();
}

/*
===============================
idGameLocal::Leaderboards_Shutdown
===============================
*/
void idGameLocal::Leaderboards_Shutdown()
{
	LeaderboardLocal_Shutdown();
}

/*
========================
idGameLocal::Shell_ClearRepeater
========================
*/
void idGameLocal::Shell_ClearRepeater()
{
	if( shellHandler != NULL )
	{
		shellHandler->ClearWidgetActionRepeater();
	}
}

/*
========================
idGameLocal::Shell_Init
========================
*/
void idGameLocal::Shell_Init( const char* filename, idSoundWorld* sw )
{
	if( shellHandler != NULL )
	{
		shellHandler->Initialize( filename, sw );
	}
}

/*
========================
idGameLocal::Shell_Init
========================
*/
void idGameLocal::Shell_Cleanup()
{
	if( shellHandler != NULL )
	{
		delete shellHandler;
		shellHandler = NULL;
	}
	
	mpGame.CleanupScoreboard();
}

/*
========================
idGameLocal::Shell_CreateMenu
========================
*/
void idGameLocal::Shell_CreateMenu( bool inGame )
{
	Shell_ResetMenu();
	
	if( shellHandler != NULL )
	{
		if( !inGame )
		{
			shellHandler->SetInGame( false );
			Shell_Init( "shell", common->MenuSW() );
		}
		else
		{
			shellHandler->SetInGame( true );
			if( common->IsMultiplayer() )
			{
				Shell_Init( "pause", common->SW() );
			}
			else
			{
				Shell_Init( "pause", common->MenuSW() );
			}
		}
	}
}

/*
========================
idGameLocal::Shell_ClosePause
========================
*/
void idGameLocal::Shell_ClosePause()
{
	if( shellHandler != NULL )
	{
	
		if( !common->IsMultiplayer() && GetLocalPlayer() && GetLocalPlayer()->health <= 0 )
		{
			return;
		}
		
		if( shellHandler->GetGameComplete() )
		{
			return;
		}
		
		shellHandler->SetNextScreen( SHELL_AREA_INVALID, MENU_TRANSITION_SIMPLE );
	}
}

/*
========================
idGameLocal::Shell_Show
========================
*/
void idGameLocal::Shell_Show( bool show )
{
	if( shellHandler != NULL )
	{
		shellHandler->ActivateMenu( show );
	}
}

/*
========================
idGameLocal::Shell_IsActive
========================
*/
bool idGameLocal::Shell_IsActive() const
{
	if( shellHandler != NULL )
	{
		return shellHandler->IsActive();
	}
	return false;
}

/*
========================
idGameLocal::Shell_HandleGuiEvent
========================
*/
bool idGameLocal::Shell_HandleGuiEvent( const sysEvent_t* sev )
{
	if( shellHandler != NULL )
	{
		return shellHandler->HandleGuiEvent( sev );
	}
	return false;
}

/*
========================
idGameLocal::Shell_Render
========================
*/
void idGameLocal::Shell_Render()
{
	if( shellHandler != NULL )
	{
		shellHandler->Update();
	}
}

/*
========================
idGameLocal::Shell_ResetMenu
========================
*/
void idGameLocal::Shell_ResetMenu()
{
	if( shellHandler != NULL )
	{
		delete shellHandler;
		shellHandler = new( TAG_SWF ) idMenuHandler_Shell();
	}
}

/*
=================
idGameLocal::Shell_SyncWithSession
=================
*/
void idGameLocal::Shell_SyncWithSession()
{
	if( shellHandler == NULL )
	{
		return;
	}
	switch( session->GetState() )
	{
		case idSession::PRESS_START:
			shellHandler->SetShellState( SHELL_STATE_PRESS_START );
			break;
		case idSession::INGAME:
			shellHandler->SetShellState( SHELL_STATE_PAUSED );
			break;
		case idSession::IDLE:
			shellHandler->SetShellState( SHELL_STATE_IDLE );
			break;
		case idSession::PARTY_LOBBY:
			shellHandler->SetShellState( SHELL_STATE_PARTY_LOBBY );
			break;
		case idSession::GAME_LOBBY:
			shellHandler->SetShellState( SHELL_STATE_GAME_LOBBY );
			break;
		case idSession::SEARCHING:
			shellHandler->SetShellState( SHELL_STATE_SEARCHING );
			break;
		case idSession::LOADING:
			shellHandler->SetShellState( SHELL_STATE_LOADING );
			break;
		case idSession::CONNECTING:
			shellHandler->SetShellState( SHELL_STATE_CONNECTING );
			break;
		case idSession::BUSY:
			shellHandler->SetShellState( SHELL_STATE_BUSY );
			break;
	}
}

void idGameLocal::Shell_SetGameComplete()
{
	if( shellHandler != NULL )
	{
		shellHandler->SetGameComplete();
		Shell_Show( true );
	}
}

/*
========================
idGameLocal::Shell_SetState_GameLobby
========================
*/
void idGameLocal::Shell_UpdateSavedGames()
{
	if( shellHandler != NULL )
	{
		shellHandler->UpdateSavedGames();
	}
}

/*
========================
idGameLocal::Shell_SetCanContinue
========================
*/
void idGameLocal::Shell_SetCanContinue( bool valid )
{
	if( shellHandler != NULL )
	{
		shellHandler->SetCanContinue( valid );
	}
}

/*
========================
idGameLocal::Shell_SetState_GameLobby
========================
*/
void idGameLocal::Shell_UpdateClientCountdown( int countdown )
{
	if( shellHandler != NULL )
	{
		shellHandler->SetTimeRemaining( countdown );
	}
}

/*
========================
idGameLocal::Shell_SetState_GameLobby
========================
*/
void idGameLocal::Shell_UpdateLeaderboard( const idLeaderboardCallback* callback )
{
	if( shellHandler != NULL )
	{
		shellHandler->UpdateLeaderboard( callback );
	}
}

/*
========================
idGameLocal::SimulateProjectiles
========================
*/
bool idGameLocal::SimulateProjectiles()
{
	bool moreProjectiles = false;
	// Simulate projectiles
	for( int i = 0; i < idProjectile::MAX_SIMULATED_PROJECTILES; i++ )
	{
		if( idProjectile::projectilesToSimulate[i].projectile != NULL && idProjectile::projectilesToSimulate[i].startTime != 0 )
		{
			const int startTime = idProjectile::projectilesToSimulate[i].startTime;
			const int startFrame = MSEC_TO_FRAME_FLOOR( startTime );
			const int endFrame = startFrame + 1;
			const int endTime = FRAME_TO_MSEC( endFrame );
			
			idProjectile::projectilesToSimulate[i].projectile->SimulateProjectileFrame( endTime - startTime, endTime );
			
			if( idProjectile::projectilesToSimulate[i].projectile != NULL )
			{
				if( endTime >= previousServerTime )
				{
					idProjectile::projectilesToSimulate[i].projectile->PostSimulate( endTime );
					idProjectile::projectilesToSimulate[i].startTime = 0;
					idProjectile::projectilesToSimulate[i].projectile = NULL;
				}
				else
				{
					idProjectile::projectilesToSimulate[i].startTime = endTime;
					moreProjectiles = true;
				}
			}
		}
	}
	
	return moreProjectiles;
}
