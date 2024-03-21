/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2014-2016 Robert Beckebans
Copyright (C) 2014-2016 Kot in Action Creative Artel

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

#ifndef __GAME_H__
#define __GAME_H__

/*
===============================================================================

	Public game interface with methods to run the game.

===============================================================================
*/

// default scripts
#define SCRIPT_DEFAULTDEFS			"script/doom_defs.script"
#define SCRIPT_DEFAULT				"script/doom_main.script"
#define SCRIPT_DEFAULTFUNC			"doom_main"

struct gameReturn_t
{

	gameReturn_t() :
		sessionCommand( "" ),       // SRS - Explicitly init sessionCommand otherwise can be optimized out and skipped with gcc or Apple clang
		syncNextGameFrame( false ),
		vibrationLow( 0 ),
		vibrationHigh( 0 )
	{

	}

	char		sessionCommand[MAX_STRING_CHARS];	// "map", "disconnect", "victory", etc
	bool		syncNextGameFrame;					// used when cinematics are skipped to prevent session from simulating several game frames to
	// keep the game time in sync with real time
	int			vibrationLow;
	int			vibrationHigh;
};

#define TIME_GROUP1		0
#define TIME_GROUP2		1

class idGame
{
public:
	virtual						~idGame() {}

	// Initialize the game for the first time.
	virtual void				Init() = 0;

	// Shut down the entire game.
	virtual void				Shutdown() = 0;

	// Sets the serverinfo at map loads and when it changes.
	virtual void				SetServerInfo( const idDict& serverInfo ) = 0;

	// Gets the serverinfo, common calls this before saving the game
	virtual const idDict& 		GetServerInfo() = 0;

	// Interpolated server time
	virtual void				SetServerGameTimeMs( const int time ) = 0;

	// Interpolated server time
	virtual int					GetServerGameTimeMs() const = 0;

	virtual int					GetSSEndTime() const  = 0;
	virtual int					GetSSStartTime() const = 0;

	// common calls this before moving the single player game to a new level.
	virtual const idDict& 		GetPersistentPlayerInfo( int clientNum ) = 0;

	// common calls this right before a new level is loaded.
	virtual void				SetPersistentPlayerInfo( int clientNum, const idDict& playerInfo ) = 0;

	// Loads a map and spawns all the entities.
	virtual void				InitFromNewMap( const char* mapName, idRenderWorld* renderWorld, idSoundWorld* soundWorld, int gameMode, int randseed ) = 0;

	// Loads a map from a savegame file.
	virtual bool				InitFromSaveGame( const char* mapName, idRenderWorld* renderWorld, idSoundWorld* soundWorld, idFile* saveGameFile, idFile* stringTableFile, int saveGameVersion ) = 0;

	// Saves the current game state, common may have written some data to the file already.
	virtual void				SaveGame( idFile* saveGameFile, idFile* stringTableFile ) = 0;

	// Pulls the current player location from the game information
	virtual void				GetSaveGameDetails( idSaveGameDetails& gameDetails ) = 0;

	// Shut down the current map.
	virtual void				MapShutdown() = 0;

	// Caches media referenced from in key/value pairs in the given dictionary.
	virtual void				CacheDictionaryMedia( const idDict* dict ) = 0;

	virtual void				Preload( const idPreloadManifest& manifest ) = 0;

	// Runs a game frame, may return a session command for level changing, etc
	virtual void				RunFrame( idUserCmdMgr& cmdMgr, gameReturn_t& gameReturn ) = 0;

	// Makes rendering and sound system calls to display for a given clientNum.
	virtual bool				Draw( int clientNum ) = 0;

	virtual bool				HandlePlayerGuiEvent( const sysEvent_t* ev ) = 0;

	// Writes a snapshot of the server game state.
	virtual void				ServerWriteSnapshot( idSnapShot& ss ) = 0;

	// Processes a reliable message
	virtual void				ProcessReliableMessage( int clientNum, int type, const idBitMsg& msg ) = 0;

	virtual void				SetInterpolation( const float fraction, const int serverGameMS, const int ssStartTime, const int ssEndTime ) = 0;

	// Reads a snapshot and updates the client game state.
	virtual void				ClientReadSnapshot( const idSnapShot& ss ) = 0;

	// Runs prediction on entities at the client.
	virtual void				ClientRunFrame( idUserCmdMgr& cmdMgr, bool lastPredictFrame, gameReturn_t& ret ) = 0;

	// Used to manage divergent time-lines
	virtual int					GetTimeGroupTime( int timeGroup ) = 0;

	// Returns a list of available multiplayer game modes
	virtual int					GetMPGameModes( const char** * gameModes, const char** * gameModesDisplay ) = 0;

	// Returns a summary of stats for a given client
	virtual void				GetClientStats( int clientNum, char* data, const int len ) = 0;

	virtual bool				IsInGame() const = 0;

	// Get the player entity number for a network peer.
	virtual int					MapPeerToClient( int peer ) const = 0;

	// Get the player entity number of the local player.
	virtual int					GetLocalClientNum() const = 0;

	// compute an angle offset to be applied to the given client's aim
	virtual void				GetAimAssistAngles( idAngles& angles ) = 0;
	virtual float				GetAimAssistSensitivity() = 0;

	// Release the mouse when the PDA is open
	virtual bool				IsPDAOpen() const = 0;
	virtual bool				IsPlayerChatting() const = 0;

	// Creates leaderboards for each map/mode defined.
	virtual void				Leaderboards_Init() = 0;
	virtual void				Leaderboards_Shutdown() = 0;

	// MAIN MENU FUNCTIONS
	virtual bool				InhibitControls() = 0;
	virtual void				Shell_Init( const char* filename, idSoundWorld* sw ) = 0;
	virtual void				Shell_Cleanup() = 0;
	virtual void				Shell_CreateMenu( bool inGame ) = 0;
	virtual void				Shell_ClosePause() = 0;
	virtual void				Shell_Show( bool show ) = 0;
	virtual bool				Shell_IsActive() const = 0;
	virtual bool				Shell_HandleGuiEvent( const sysEvent_t* sev ) = 0;
	virtual void				Shell_Render() = 0;
	virtual void				Shell_ResetMenu() = 0;
	virtual void				Shell_SyncWithSession() = 0;
	virtual void				Shell_UpdateSavedGames() = 0;
	virtual void				Shell_SetCanContinue( bool valid ) = 0;
	virtual void				Shell_UpdateClientCountdown( int countdown ) = 0;
	virtual void				Shell_UpdateLeaderboard( const idLeaderboardCallback* callback ) = 0;
	virtual void				Shell_SetGameComplete() = 0;
	virtual bool				SkipCinematicScene() = 0;
	virtual bool				CheckInCinematic() = 0;
};

extern idGame* 					game;


/*
===============================================================================

	Public game interface with methods for in-game editing.

===============================================================================
*/

typedef struct
{
	idSoundEmitter* 			referenceSound;	// this is the interface to the sound system, created
	// with idSoundWorld::AllocSoundEmitter() when needed
	idVec3						origin;
	int							listenerId;		// SSF_PRIVATE_SOUND only plays if == listenerId from PlaceListener
	// no spatialization will be performed if == listenerID
	const idSoundShader* 		shader;			// this really shouldn't be here, it is a holdover from single channel behavior
	float						diversity;		// 0.0 to 1.0 value used to select which
	// samples in a multi-sample list from the shader are used
	bool						waitfortrigger;	// don't start it at spawn time
	soundShaderParms_t			parms;			// override volume, flags, etc
} refSound_t;

enum
{
	TEST_PARTICLE_MODEL = 0,
	TEST_PARTICLE_IMPACT,
	TEST_PARTICLE_MUZZLE,
	TEST_PARTICLE_FLIGHT,
	TEST_PARTICLE_SELECTED
};

class idEntity;
class idMD5Anim;

// FIXME: this interface needs to be reworked but it properly separates code for the time being
class idGameEdit
{
public:
	virtual						~idGameEdit() {}

	// These are the canonical idDict to parameter parsing routines used by both the game and tools.
	virtual void				ParseSpawnArgsToRenderLight( const idDict* args, renderLight_t* renderLight );
	virtual void				ParseSpawnArgsToRenderEntity( const idDict* args, renderEntity_t* renderEntity, const idDeclEntityDef* def = NULL );
	virtual void				ParseSpawnArgsToRenderEnvprobe( const idDict* args, renderEnvironmentProbe_t* renderEnvprobe ); // RB
	virtual void				ParseSpawnArgsToRefSound( const idDict* args, refSound_t* refSound );

	// Animation system calls for non-game based skeletal rendering.
	virtual idRenderModel* 		ANIM_GetModelFromEntityDef( const char* classname );
	virtual const idVec3&		ANIM_GetModelOffsetFromEntityDef( const char* classname );
	virtual idRenderModel* 		ANIM_GetModelFromEntityDef( const idDict* args );
	virtual idRenderModel* 		ANIM_GetModelFromName( const char* modelName );
	virtual const idMD5Anim* 	ANIM_GetAnimFromEntityDef( const char* classname, const char* animname );
	virtual int					ANIM_GetNumAnimsFromEntityDef( const idDict* args );
	virtual const char* 		ANIM_GetAnimNameFromEntityDef( const idDict* args, int animNum );
	virtual const idMD5Anim* 	ANIM_GetAnim( const char* fileName );
	virtual int					ANIM_GetLength( const idMD5Anim* anim );
	virtual int					ANIM_GetNumFrames( const idMD5Anim* anim );
	virtual void				ANIM_CreateAnimFrame( const idRenderModel* model, const idMD5Anim* anim, int numJoints, idJointMat* frame, int time, const idVec3& offset, bool remove_origin_offset );
	virtual idRenderModel* 		ANIM_CreateMeshForAnim( idRenderModel* model, const char* classname, const char* animname, int frame, bool remove_origin_offset );

	// Articulated Figure calls for AF editor and Radiant.
	virtual bool				AF_SpawnEntity( const char* fileName );
	virtual void				AF_UpdateEntities( const char* fileName );
	virtual void				AF_UndoChanges();
	virtual idRenderModel* 		AF_CreateMesh( const idDict& args, idVec3& meshOrigin, idMat3& meshAxis, bool& poseIsSet );


	// Entity selection.
	virtual void				ClearEntitySelection();
	virtual int					GetSelectedEntities( idEntity* list[], int max );
	virtual void				AddSelectedEntity( idEntity* ent );

	// Selection methods
	virtual void				TriggerSelected();

	// Entity defs and spawning.
	virtual const idDict* 		FindEntityDefDict( const char* name, bool makeDefault = true ) const;
	virtual void				SpawnEntityDef( const idDict& args, idEntity** ent );
	virtual idEntity* 			FindEntity( const char* name ) const;
	virtual const char* 		GetUniqueEntityName( const char* classname ) const;

	// Entity methods.
	virtual void				EntityGetOrigin( idEntity* ent, idVec3& org ) const;
	virtual void				EntityGetAxis( idEntity* ent, idMat3& axis ) const;
	virtual void				EntitySetOrigin( idEntity* ent, const idVec3& org );
	virtual void				EntitySetAxis( idEntity* ent, const idMat3& axis );
	virtual void				EntityTranslate( idEntity* ent, const idVec3& org );
	virtual const idDict* 		EntityGetSpawnArgs( idEntity* ent ) const;
	virtual void				EntityUpdateChangeableSpawnArgs( idEntity* ent, const idDict* dict );
	virtual void				EntityChangeSpawnArgs( idEntity* ent, const idDict* newArgs );
	virtual void				EntityUpdateVisuals( idEntity* ent );
	virtual void				EntitySetModel( idEntity* ent, const char* val );
	virtual void				EntityStopSound( idEntity* ent );
	virtual void				EntityDelete( idEntity* ent );
	virtual void				EntitySetColor( idEntity* ent, const idVec3 color );

	// Player methods.
	virtual bool				PlayerIsValid() const;
	virtual void				PlayerGetOrigin( idVec3& org ) const;
	virtual void				PlayerGetAxis( idMat3& axis ) const;
	virtual void				PlayerGetViewAngles( idAngles& angles ) const;
	virtual void				PlayerGetEyePosition( idVec3& org ) const;
	virtual bool				PlayerGetRenderView( renderView_t& rv ) const;

	// In game map editing support.
	virtual const idDict* 		MapGetEntityDict( const char* name ) const;
	virtual void				MapSave( const char* path = NULL ) const;
	virtual void				MapSetEntityKeyVal( const char* name, const char* key, const char* val ) const ;
	virtual void				MapCopyDictToEntity( const char* name, const idDict* dict ) const;
	virtual void				MapCopyDictToEntityAtOrigin( const idVec3& org, const idDict* dict ) const;
	virtual int					MapGetUniqueMatchingKeyVals( const char* key, const char* list[], const int max ) const;
	virtual void				MapAddEntity( const idDict* dict ) const;
	virtual int					MapGetEntitiesMatchingClassWithString( const char* classname, const char* match, const char* list[], const int max ) const;
	virtual void				MapRemoveEntity( const char* name ) const;
	virtual void				MapEntityTranslate( const char* name, const idVec3& v ) const;

};

extern idGameEdit* 				gameEdit;


/*
===============================================================================

	Game API.

===============================================================================
*/

const int GAME_API_VERSION		= 8;

typedef struct
{

	int							version;				// API version
	idSys* 						sys;					// non-portable system services
	idCommon* 					common;					// common
	idCmdSystem* 				cmdSystem;				// console command system
	idCVarSystem* 				cvarSystem;				// console variable system
	idFileSystem* 				fileSystem;				// file system
	idRenderSystem* 			renderSystem;			// render system
	idSoundSystem* 				soundSystem;			// sound system
	idRenderModelManager* 		renderModelManager;		// render model manager
	idUserInterfaceManager* 	uiManager;				// user interface manager
	idDeclManager* 				declManager;			// declaration manager
	idAASFileManager* 			AASFileManager;			// AAS file manager
	idCollisionModelManager* 	collisionModelManager;	// collision model manager

} gameImport_t;

typedef struct
{

	int							version;				// API version
	idGame* 					game;					// interface to run the game
	idGameEdit* 				gameEdit;				// interface for in-game editing

} gameExport_t;

extern "C" {
	typedef gameExport_t* ( *GetGameAPI_t )( gameImport_t* import );
}

#endif /* !__GAME_H__ */
