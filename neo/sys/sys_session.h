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
#ifndef __SYS_SESSION_H__
#define __SYS_SESSION_H__

#include "../framework/Serializer.h"
#include "sys_localuser.h"

typedef uint8 peerMask_t;
static const int MAX_PLAYERS			= 8;

static const int MAX_REDUNDANT_CMDS	= 3;

static const int MAX_LOCAL_PLAYERS		= 2;
static const int MAX_INPUT_DEVICES		= 4;
enum matchFlags_t
{
	MATCH_STATS						= BIT( 0 ),		// Match will upload leaderboard/achievement scores
	MATCH_ONLINE					= BIT( 1 ),		// Match will require users to be online
	MATCH_RANKED					= BIT( 2 ),		// Match will affect rank
	MATCH_PRIVATE					= BIT( 3 ),		// Match will NOT be searchable through FindOrCreateMatch
	MATCH_INVITE_ONLY				= BIT( 4 ),		// Match visible through invite only

	MATCH_REQUIRE_PARTY_LOBBY		= BIT( 5 ),		// This session uses a party lobby
	MATCH_PARTY_INVITE_PLACEHOLDER	= BIT( 6 ),		// Party is never shown in the UI, it's simply used as a placeholder for invites
	MATCH_JOIN_IN_PROGRESS			= BIT( 7 ),		// Join in progress supported for this match
};

ID_INLINE bool MatchTypeIsOnline( uint8 matchFlags )
{
	return ( matchFlags & MATCH_ONLINE ) ? true : false;
}
ID_INLINE bool MatchTypeIsLocal( uint8 matchFlags )
{
	return !MatchTypeIsOnline( matchFlags );
}
ID_INLINE bool MatchTypeIsPrivate( uint8 matchFlags )
{
	return ( matchFlags & MATCH_PRIVATE ) ? true : false;
}
ID_INLINE bool MatchTypeIsRanked( uint8 matchFlags )
{
	return ( matchFlags & MATCH_RANKED ) ? true : false;
}
ID_INLINE bool MatchTypeHasStats( uint8 matchFlags )
{
	return ( matchFlags & MATCH_STATS ) ? true : false;
}
ID_INLINE bool MatchTypeInviteOnly( uint8 matchFlags )
{
	return ( matchFlags & MATCH_INVITE_ONLY ) ? true : false;
}
ID_INLINE bool MatchTypeIsSearchable( uint8 matchFlags )
{
	return !MatchTypeIsPrivate( matchFlags );
}
ID_INLINE bool MatchTypeIsJoinInProgress( uint8 matchFlags )
{
	return ( matchFlags & MATCH_JOIN_IN_PROGRESS ) ? true : false;
}

class idCompressor;
class idLeaderboardSubmission;
class idLeaderboardQuery;
class idSignInManagerBase;
class idPlayerProfile;
class idGameSpawnInfo;
class idSaveLoadParms;
class idSaveGameManager;
class idLocalUser;
class idDedicatedServerSearch;
class idAchievementSystem;
class idLeaderboardCallback;

struct leaderboardDefinition_t;
struct column_t;

const int8 GAME_MODE_RANDOM = -1;
const int8 GAME_MODE_SINGLEPLAYER = -2;

const int8 GAME_MAP_RANDOM = -1;
const int8 GAME_MAP_SINGLEPLAYER = -2;

const int8 GAME_EPISODE_UNKNOWN = -1;
const int8 GAME_SKILL_DEFAULT = -1;

const int DefaultPartyFlags			= MATCH_JOIN_IN_PROGRESS | MATCH_ONLINE;
const int DefaultPublicGameFlags	= MATCH_JOIN_IN_PROGRESS | MATCH_REQUIRE_PARTY_LOBBY | MATCH_RANKED |  MATCH_STATS;
const int DefaultPrivateGameFlags	= MATCH_JOIN_IN_PROGRESS | MATCH_REQUIRE_PARTY_LOBBY | MATCH_PRIVATE;

/*
================================================
idMatchParameters
================================================
*/
class idMatchParameters
{
public:
	idMatchParameters() :
		numSlots( MAX_PLAYERS ),
		gameMode( GAME_MODE_RANDOM ),
		gameMap( GAME_MAP_RANDOM ),
		gameEpisode( GAME_EPISODE_UNKNOWN ),
		gameSkill( GAME_SKILL_DEFAULT ),
		matchFlags( 0 )
	{}

	void Write( idBitMsg& msg )
	{
		idSerializer s( msg, true );
		Serialize( s );
	}
	void Read( idBitMsg& msg )
	{
		idSerializer s( msg, false );
		Serialize( s );
	}

	void Serialize( idSerializer& serializer )
	{
		serializer.Serialize( gameMode );
		serializer.Serialize( gameMap );
		serializer.Serialize( gameEpisode );
		serializer.Serialize( gameSkill );
		serializer.Serialize( numSlots );
		serializer.Serialize( matchFlags );
		serializer.SerializeString( mapName );
		serverInfo.Serialize( serializer );
	}

	uint8 	numSlots;
	int8	gameMode;
	int8 	gameMap;
	int8	gameEpisode;		// Episode for doom classic support.
	int8	gameSkill;			// Skill for doom classic support.
	uint8	matchFlags;

	idStr	mapName; // This is only used for SP (gameMap == GAME_MAP_SINGLEPLAYER)
	idDict	serverInfo;
};

/*
================================================
serverInfo_t
Results from calling ListServers/ServerInfo for game browser / system link.
================================================
*/
struct serverInfo_t
{
	serverInfo_t() :
		gameMode( GAME_MODE_RANDOM ),
		gameMap( GAME_MAP_RANDOM ),
		joinable(),
		numPlayers(),
		maxPlayers()
	{}


	void Write( idBitMsg& msg )
	{
		idSerializer s( msg, true );
		Serialize( s );
	}
	void Read( idBitMsg& msg )
	{
		idSerializer s( msg, false );
		Serialize( s );
	}

	void Serialize( idSerializer& serializer )
	{
		serializer.SerializeString( serverName );
		serializer.Serialize( gameMode );
		serializer.Serialize( gameMap );
		SERIALIZE_BOOL( serializer, joinable );
		serializer.Serialize( numPlayers );
		serializer.Serialize( maxPlayers );
	}

	idStr	serverName;
	int8	gameMode;
	int8 	gameMap;
	bool	joinable;
	int 	numPlayers;
	int 	maxPlayers;
};

//------------------------
// voiceState_t
//------------------------
enum voiceState_t
{
	VOICECHAT_STATE_NO_MIC,
	VOICECHAT_STATE_MUTED_LOCAL,
	VOICECHAT_STATE_MUTED_REMOTE,
	VOICECHAT_STATE_MUTED_ALL,
	VOICECHAT_STATE_NOT_TALKING,
	VOICECHAT_STATE_TALKING,
	VOICECHAT_STATE_TALKING_GLOBAL,
	NUM_VOICECHAT_STATE
};

//------------------------
// voiceStateDisplay_t
//------------------------
enum voiceStateDisplay_t
{
	VOICECHAT_DISPLAY_NONE,
	VOICECHAT_DISPLAY_NOTTALKING,
	VOICECHAT_DISPLAY_TALKING,
	VOICECHAT_DISPLAY_TALKING_GLOBAL,
	VOICECHAT_DISPLAY_MUTED,
	VOICECHAT_DISPLAY_MAX
};

static const int QOS_RESULT_CRAPPY	= 200;
static const int QOS_RESULT_WEAK	= 100;
static const int QOS_RESULT_GOOD	= 50;
static const int QOS_RESULT_GREAT	= 0;

//------------------------
// qosState_t
//------------------------
enum qosState_t
{
	QOS_STATE_CRAPPY = 1,
	QOS_STATE_WEAK,
	QOS_STATE_GOOD,
	QOS_STATE_GREAT,
	QOS_STATE_MAX,
};

//------------------------
// leaderboardDisplayError_t
//------------------------
enum leaderboardDisplayError_t
{
	LEADERBOARD_DISPLAY_ERROR_NONE,
	LEADERBOARD_DISPLAY_ERROR_FAILED,				// General error occurred
	LEADERBOARD_DISPLAY_ERROR_NOT_ONLINE,			// No longer online
	LEADERBOARD_DISPLAY_ERROR_NOT_RANKED,			// Attempting to view a "My Score" leaderboard you aren't ranked on
	LEADERBOARD_DISPLAY_ERROR_MAX
};

/*
================================================
lobbyUserID_t
================================================
*/
struct lobbyUserID_t
{
public:

	lobbyUserID_t() : lobbyType( 0xFF ) {}

	explicit lobbyUserID_t( localUserHandle_t localUser_, byte lobbyType_ ) : localUserHandle( localUser_ ), lobbyType( lobbyType_ ) {}

	bool operator == ( const lobbyUserID_t& other ) const
	{
		return localUserHandle == other.localUserHandle && lobbyType == other.lobbyType;		// Lobby type must match
	}

	bool operator != ( const lobbyUserID_t& other ) const
	{
		return !( *this == other );
	}

	bool operator < ( const lobbyUserID_t& other ) const
	{
		if( localUserHandle == other.localUserHandle )
		{
			return lobbyType < other.lobbyType;		// Lobby type tie breaker
		}

		return localUserHandle < other.localUserHandle;
	}

	bool CompareIgnoreLobbyType( const lobbyUserID_t& other ) const
	{
		return localUserHandle == other.localUserHandle;
	}

	localUserHandle_t	GetLocalUserHandle() const
	{
		return localUserHandle;
	}
	byte				GetLobbyType() const
	{
		return lobbyType;
	}

	bool IsValid() const
	{
		return localUserHandle.IsValid() && lobbyType != 0xFF;
	}

	void WriteToMsg( idBitMsg& msg )
	{
		localUserHandle.WriteToMsg( msg );
		msg.WriteByte( lobbyType );
	}

	void ReadFromMsg( const idBitMsg& msg )
	{
		localUserHandle.ReadFromMsg( msg );
		lobbyType = msg.ReadByte();
	}

	void Serialize( idSerializer& ser );

private:
	localUserHandle_t	localUserHandle;
	byte				lobbyType;
};

/*
================================================
idLobbyBase
================================================
*/
class idLobbyBase
{
public:
	// General lobby functionality
	virtual bool						IsHost() const = 0;
	virtual bool						IsPeer() const = 0;
	virtual bool						HasActivePeers() const = 0;
	virtual int							GetNumLobbyUsers() const = 0;
	virtual int							GetNumActiveLobbyUsers() const = 0;
	virtual bool						IsLobbyUserConnected( int index ) const = 0;

	virtual lobbyUserID_t				GetLobbyUserIdByOrdinal( int userIndex ) const = 0;
	virtual	int							GetLobbyUserIndexFromLobbyUserID( lobbyUserID_t lobbyUserID ) const = 0;

	virtual void						SendReliable( int type, idBitMsg& msg, bool callReceiveReliable = true, peerMask_t sessionUserMask = MAX_UNSIGNED_TYPE( peerMask_t ) ) = 0;
	virtual void						SendReliableToLobbyUser( lobbyUserID_t lobbyUserID, int type, idBitMsg& msg ) = 0;
	virtual void						SendReliableToHost( int type, idBitMsg& msg ) = 0;

	// Lobby user access
	virtual const char* 				GetLobbyUserName( lobbyUserID_t lobbyUserID ) const = 0;
	virtual void						KickLobbyUser( lobbyUserID_t lobbyUserID ) = 0;
	virtual bool						IsLobbyUserValid( lobbyUserID_t lobbyUserID ) const = 0;
	virtual bool						IsLobbyUserLoaded( lobbyUserID_t lobbyUserID ) const = 0;
	virtual bool						LobbyUserHasFirstFullSnap( lobbyUserID_t lobbyUserID ) const = 0;
	virtual void						EnableSnapshotsForLobbyUser( lobbyUserID_t lobbyUserID ) = 0;

	virtual int							GetLobbyUserSkinIndex( lobbyUserID_t lobbyUserID ) const = 0;
	virtual bool						GetLobbyUserWeaponAutoReload( lobbyUserID_t lobbyUserID ) const = 0;
	virtual bool						GetLobbyUserWeaponAutoSwitch( lobbyUserID_t lobbyUserID ) const = 0;
	virtual int							GetLobbyUserLevel( lobbyUserID_t lobbyUserID ) const = 0;
	virtual int							GetLobbyUserQoS( lobbyUserID_t lobbyUserID ) const = 0;
	virtual int							GetLobbyUserTeam( lobbyUserID_t lobbyUserID ) const = 0;
	virtual bool						SetLobbyUserTeam( lobbyUserID_t lobbyUserID, int teamNumber ) = 0;
	virtual int							GetLobbyUserPartyToken( lobbyUserID_t lobbyUserID ) const = 0;
	virtual idPlayerProfile* 			GetProfileFromLobbyUser( lobbyUserID_t lobbyUserID ) = 0;
	virtual idLocalUser* 				GetLocalUserFromLobbyUser( lobbyUserID_t lobbyUserID ) = 0;
	virtual int							GetNumLobbyUsersOnTeam( int teamNumber ) const = 0;

	virtual int							PeerIndexFromLobbyUser( lobbyUserID_t lobbyUserID ) const = 0;
	virtual int							GetPeerTimeSinceLastPacket( int peerIndex ) const = 0;
	virtual int							PeerIndexForHost() const = 0;

	virtual lobbyUserID_t				AllocLobbyUserSlotForBot( const char* botName ) = 0;
	virtual void						RemoveBotFromLobbyUserList( lobbyUserID_t lobbyUserID ) = 0;
	virtual bool						GetLobbyUserIsBot( lobbyUserID_t lobbyUserID ) const = 0;

	virtual const char* 				GetHostUserName() const = 0;
	virtual const idMatchParameters& 	GetMatchParms() const = 0;
	virtual bool						IsLobbyFull() const = 0;

	// Peer access
	virtual bool						EnsureAllPeersHaveBaseState() = 0;
	virtual bool						AllPeersInGame() const = 0;
	virtual int							GetNumConnectedPeers() const = 0;
	virtual int							GetNumConnectedPeersInGame() const = 0;
	virtual int							PeerIndexOnHost() const = 0;
	virtual bool						IsPeerDisconnected( int peerIndex ) const = 0;

	// Snapshots
	virtual bool						AllPeersHaveStaleSnapObj( int objId ) = 0;
	virtual bool						AllPeersHaveExpectedSnapObj( int objId ) = 0;
	virtual void						RefreshSnapObj( int objId ) = 0;
	virtual void						MarkSnapObjDeleted( int objId ) = 0;
	virtual void						AddSnapObjTemplate( int objID, idBitMsg& msg ) = 0;

	// Debugging
	virtual void						DrawDebugNetworkHUD() const = 0;
	virtual void						DrawDebugNetworkHUD2() const = 0;
	virtual void						DrawDebugNetworkHUD_ServerSnapshotMetrics( bool draw ) = 0;
};

/*
================================================
idSession
================================================
*/
class idSession
{
public:

	enum sessionState_t
	{
		PRESS_START,
		IDLE,
		SEARCHING,
		CONNECTING,
		PARTY_LOBBY,
		GAME_LOBBY,
		LOADING,
		INGAME,
		BUSY,
		MAX_STATES
	};

	enum sessionOption_t
	{
		OPTION_LEAVE_WITH_PARTY			= BIT( 0 ),		// As a party leader, whether or not to drag your party members with you when you leave a game lobby
		OPTION_ALL						= 0xFFFFFFFF
	};

	idSession() :
		signInManager( NULL ),
		saveGameManager( NULL ),
		achievementSystem( NULL ),
		dedicatedServerSearch( NULL ) { }
	virtual 		~idSession();

	virtual void			Initialize() = 0;
	virtual void			Shutdown() = 0;

	virtual void			InitializeSoundRelatedSystems() = 0;
	virtual void			ShutdownSoundRelatedSystems() = 0;

	//=====================================================================================================
	// Lobby management
	//=====================================================================================================
	virtual void			CreatePartyLobby( const idMatchParameters& parms_ ) = 0;
	virtual void			FindOrCreateMatch( const idMatchParameters& parms_ ) = 0;
	virtual void			CreateMatch( const idMatchParameters& parms_ ) = 0;
	virtual void			CreateGameStateLobby( const idMatchParameters& parms_ ) = 0;
	virtual void			UpdateMatchParms( const idMatchParameters& parms_ ) = 0;
	virtual void			UpdatePartyParms( const idMatchParameters& parms_ ) = 0;
	virtual void			StartMatch() = 0;
	virtual void			EndMatch( bool premature = false ) = 0;	// Meant for host to end match gracefully, go back to lobby, tally scores, etc
	virtual void			MatchFinished() = 0;					// this is for when the game is over before we go back to lobby. Need this incase the host leaves during this time
	virtual void			QuitMatch() = 0;						// Meant for host or peer to quit the match before it ends, will instigate host migration, etc
	virtual void			QuitMatchToTitle() = 0;					// Will forcefully quit the match and return to the title screen.
	virtual	void			SetSessionOption( sessionOption_t option ) = 0;
	virtual	void			ClearSessionOption( sessionOption_t option ) = 0;
	virtual sessionState_t	GetBackState() = 0;
	virtual void			Cancel() = 0;
	virtual void			MoveToPressStart() = 0;
	virtual void			FinishDisconnect() = 0;
	virtual void			LoadingFinished() = 0;
	virtual bool			IsCurrentLobbyMigrating() const = 0;
	virtual bool			IsLosingConnectionToHost() const = 0;
	virtual bool			WasMigrationGame() const = 0;
	virtual bool			ShouldRelaunchMigrationGame() const = 0;
	virtual bool			WasGameLobbyCoalesced() const = 0;

	virtual bool			GetMigrationGameData( idBitMsg& msg, bool reading ) = 0;
	virtual bool			GetMigrationGameDataUser( lobbyUserID_t lobbyUserID, idBitMsg& msg, bool reading ) = 0;

	virtual bool			GetMatchParamUpdate( int& peer, int& msg ) = 0;

	virtual void			Pump() = 0;
	virtual void			ProcessSnapAckQueue() = 0;

	virtual void			InviteFriends() = 0;
	virtual void			InviteParty() = 0;
	virtual void			ShowPartySessions() = 0;

	virtual bool			IsPlatformPartyInLobby() = 0;

	// Lobby user/peer access
	// The party and game lobby are the two platform lobbies that notify the backends (Steam/PSN/LIVE of changes)
	virtual idLobbyBase& 	GetPartyLobbyBase()	= 0;
	virtual idLobbyBase& 	GetGameLobbyBase() = 0;

	// Game state lobby is the lobby used while in-game.  It is so the dedicated server can host this lobby
	// and have all platform clients join. It does NOT notify the backends of changes, it's purely for the dedicated
	// server to be able to host the in-game lobby.
	virtual idLobbyBase& 	GetActingGameStateLobbyBase() = 0;

	// GetActivePlatformLobbyBase will return either the game or party lobby, it won't return the game state lobby
	// This function is generally used for menus, in-game code should refer to GetActingGameStateLobby
	virtual idLobbyBase& 	GetActivePlatformLobbyBase() = 0;

	virtual idLobbyBase& 	GetLobbyFromLobbyUserID( lobbyUserID_t lobbyUserID ) = 0;

	virtual idPlayerProfile* 	GetProfileFromMasterLocalUser() = 0;

	virtual bool			ProcessInputEvent( const sysEvent_t* ev ) = 0;
	virtual float			GetUpstreamDropRate() = 0;
	virtual float			GetUpstreamQueueRate() = 0;
	virtual int				GetQueuedBytes() = 0;

	virtual	int				GetLoadingID() = 0;
	virtual bool			IsAboutToLoad() const = 0;

	virtual const char* 	GetLocalUserName( int i ) const = 0;

	virtual sessionState_t	GetState() const = 0;
	virtual const char* 	GetStateString() const = 0;

	virtual int				NumServers() const = 0;
	virtual void			ListServers( const idCallback& callback ) = 0;
	virtual void			CancelListServers() = 0;
	virtual void			ConnectToServer( int i ) = 0;
	virtual const serverInfo_t* ServerInfo( int i ) const = 0;
	virtual const idList< idStr >* ServerPlayerList( int i ) = 0;
	virtual void			ShowServerGamerCardUI( int i ) = 0;

	virtual void			ShowOnlineSignin() = 0;

	virtual void			DropClient( int peerNum, int lobbyType ) = 0;

	virtual void			JoinAfterSwap( void* joinID ) = 0;

	//=====================================================================================================
	//	Downloadable Content
	//=====================================================================================================
	virtual void			EnumerateDownloadableContent() = 0;
	virtual int				GetNumContentPackages() const = 0;
	virtual int				GetContentPackageID( int contentIndex ) const = 0;
	virtual const char* 	GetContentPackagePath( int contentIndex ) const = 0;
	virtual int				GetContentPackageIndexForID( int contentID ) const = 0;

	virtual void			ShowSystemMarketplaceUI() const = 0;
	virtual bool			GetSystemMarketplaceHasNewContent() const = 0;
	virtual void			SetSystemMarketplaceHasNewContent( bool hasNewContent ) = 0;

	//=====================================================================================================
	// Title Storage Vars
	//=====================================================================================================
	virtual float			GetTitleStorageFloat( const char* name, float defaultFloat ) const = 0;
	virtual int				GetTitleStorageInt( const char* name, int defaultInt ) const = 0;
	virtual bool			GetTitleStorageBool( const char* name, bool defaultBool ) const = 0;
	virtual const char* 	GetTitleStorageString( const char* name, const char* defaultString ) const = 0;

	virtual bool			GetTitleStorageFloat( const char* name, float defaultFloat, float& out ) const
	{
		out = defaultFloat;
		return false;
	}
	virtual bool			GetTitleStorageInt( const char* name, int defaultInt, int& out ) const
	{
		out = defaultInt;
		return false;
	}
	virtual bool			GetTitleStorageBool( const char* name, bool defaultBool, bool& out ) const
	{
		out = defaultBool;
		return false;
	}
	virtual bool			GetTitleStorageString( const char* name, const char* defaultString, const char** out ) const
	{
		if( out != NULL )
		{
			*out = defaultString;
		}
		return false;
	}

	virtual bool			IsTitleStorageLoaded() = 0;

	//=====================================================================================================
	// Leaderboard
	//=====================================================================================================
	virtual void			LeaderboardUpload( lobbyUserID_t lobbyUserID, const leaderboardDefinition_t* leaderboard, const column_t* stats, const idFile_Memory* attachment = NULL ) = 0;
	virtual void			LeaderboardDownload( int sessionUserIndex, const leaderboardDefinition_t* leaderboard, int startingRank, int numRows, const idLeaderboardCallback& callback ) = 0;
	virtual void			LeaderboardDownloadAttachment( int sessionUserIndex, const leaderboardDefinition_t* leaderboard, int64 attachmentID ) = 0;
	virtual void			LeaderboardFlush() = 0;

	//=====================================================================================================
	// Scoring (currently just for TrueSkill)
	//=====================================================================================================
	virtual void			SetLobbyUserRelativeScore( lobbyUserID_t lobbyUserID, int relativeScore, int team ) = 0;


	//=====================================================================================================
	// Savegames
	//
	// Default async implementations, saves to a folder, uses game.details file to describe each save.
	// Files saved are up to the game and provide through a callback mechanism.  If you want to be notified when
	// one of these operations have completed, either modify the framework's completedCallback of each of the
	// savegame processors or create your own processors and execute with the savegameManager.
	//=====================================================================================================
	virtual saveGameHandle_t		SaveGameSync( const char* name, const saveFileEntryList_t& files, const idSaveGameDetails& description ) = 0;
	virtual saveGameHandle_t		SaveGameAsync( const char* name, const saveFileEntryList_t& files, const idSaveGameDetails& description ) = 0;
	virtual saveGameHandle_t		LoadGameSync( const char* name, saveFileEntryList_t& files ) = 0;
	virtual saveGameHandle_t		EnumerateSaveGamesSync() = 0;
	virtual saveGameHandle_t		EnumerateSaveGamesAsync() = 0;
	virtual saveGameHandle_t		DeleteSaveGameSync( const char* name ) = 0;
	virtual saveGameHandle_t		DeleteSaveGameAsync( const char* name ) = 0;

	virtual bool					IsSaveGameCompletedFromHandle( const saveGameHandle_t& handle ) const = 0;
	virtual void					CancelSaveGameWithHandle( const saveGameHandle_t& handle ) = 0;

	// Needed for main menu integration
	virtual bool					IsEnumerating() const = 0;
	virtual saveGameHandle_t		GetEnumerationHandle() const = 0;

	// Returns the known list of savegames, must first enumerate for the savegames ( via session->Enumerate() )
	virtual const saveGameDetailsList_t& GetEnumeratedSavegames() const = 0;

	// These are on session and not idGame so it can persist across game deallocations
	virtual void					SetCurrentSaveSlot( const char* slotName ) = 0;
	virtual const char* 			GetCurrentSaveSlot() const = 0;

	// Error checking
	virtual bool					IsDLCAvailable( const char* mapName ) = 0;
	virtual bool					LoadGameCheckDiscNumber( idSaveLoadParms& parms ) = 0;

	//=====================================================================================================
	//	GamerCard UI
	//=====================================================================================================

	virtual void				ShowLobbyUserGamerCardUI( lobbyUserID_t lobbyUserID ) = 0;

	//=====================================================================================================
	virtual void				UpdateRichPresence() = 0;

	virtual void				SendUsercmds( idBitMsg& msg ) = 0;
	virtual void				SendSnapshot( class idSnapShot& ss ) = 0;

	virtual int					GetInputRouting( int inputRouting[ MAX_INPUT_DEVICES ] );

	virtual void				UpdateSignInManager() = 0;

	idSignInManagerBase& 		GetSignInManager()
	{
		return *signInManager;
	}
	idSaveGameManager& 			GetSaveGameManager()
	{
		return *saveGameManager;
	}
	idAchievementSystem& 		GetAchievementSystem()
	{
		return *achievementSystem;
	}

	bool						HasSignInManager() const
	{
		return ( signInManager != NULL );
	}
	bool						HasAchievementSystem() const
	{
		return ( achievementSystem != NULL );
	}

	virtual bool				IsSystemUIShowing() const = 0;
	virtual void				SetSystemUIShowing( bool show ) = 0;

	//=====================================================================================================
	//	Voice chat
	//=====================================================================================================
	virtual voiceState_t		GetLobbyUserVoiceState( lobbyUserID_t lobbyUserID ) = 0;
	virtual voiceStateDisplay_t	GetDisplayStateFromVoiceState( voiceState_t voiceState ) const = 0;
	virtual void				ToggleLobbyUserVoiceMute( lobbyUserID_t lobbyUserID ) = 0;
	virtual void				SetActiveChatGroup( int groupIndex ) = 0;
	virtual void				CheckVoicePrivileges() = 0;
	virtual void				SetVoiceGroupsToTeams() = 0;
	virtual void				ClearVoiceGroups() = 0;

	//=====================================================================================================
	//	Bandwidth / QoS checking
	//=====================================================================================================
	virtual bool				StartOrContinueBandwidthChallenge( bool forceStart ) = 0;
	virtual void				DebugSetPeerSnaprate( int peerIndex, int snapRateMS ) = 0;
	virtual float				GetIncomingByteRate() = 0;

	//=====================================================================================================
	// Invites
	//=====================================================================================================
	virtual void				HandleBootableInvite( int64 lobbyId = 0 ) = 0;
	virtual void				HandleExitspawnInvite( const lobbyConnectInfo_t& connectInfo ) {}
	virtual void				ClearBootableInvite() = 0;
	virtual void				ClearPendingInvite() = 0;
	virtual bool				HasPendingBootableInvite() = 0;
	virtual void				SetDiscSwapMPInvite( void* parm ) = 0;
	virtual void* 				GetDiscSwapMPInviteParms() = 0;
	virtual bool				IsDiscSwapMPInviteRequested() const = 0;

	//=====================================================================================================
	// Notifications
	//=====================================================================================================
	// This is called when a LocalUser is signed in/out
	virtual void				OnLocalUserSignin( idLocalUser* user ) = 0;
	virtual void				OnLocalUserSignout( idLocalUser* user ) = 0;

	// This is called when the master LocalUser is signed in/out, these are called after OnLocalUserSignin/out()
	virtual void				OnMasterLocalUserSignout() = 0;
	virtual void				OnMasterLocalUserSignin() = 0;

	// After a local user has signed in and their profile has loaded
	virtual void				OnLocalUserProfileLoaded( idLocalUser* user ) = 0;

protected:
	idSignInManagerBase*			signInManager;			// pointer so we can treat dynamically bind platform-specific impl
	idSaveGameManager* 			saveGameManager;
	idAchievementSystem* 		achievementSystem;		// pointer so we can treat dynamically bind platform-specific impl
	idDedicatedServerSearch* 	dedicatedServerSearch;
};

/*
========================
idSession::idGetInputRouting
========================
*/
ID_INLINE int idSession::GetInputRouting( int inputRouting[ MAX_INPUT_DEVICES ] )
{
	for( int i = 0; i < MAX_INPUT_DEVICES; i++ )
	{
		inputRouting[ i ] = -1;
	}
	inputRouting[0] = 0;
	return 1;
}

extern idSession* session;

#endif // __SYS_SESSION_H__
