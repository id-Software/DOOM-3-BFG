/*
================================================================================================
CONFIDENTIAL AND PROPRIETARY INFORMATION/NOT FOR DISCLOSURE WITHOUT WRITTEN PERMISSION
Copyright 2010 id Software LLC, a ZeniMax Media company. All Rights Reserved.
================================================================================================
*/

/*
================================================================================================

Contains the windows implementation of the network session

================================================================================================
*/

#include "precompiled.h"
#pragma hdrstop

#include "../../framework/Common_local.h"
#include "../sys_session_local.h"
#include "../sys_stats.h"
#include "../sys_savegame.h"
#include "../sys_lobby_backend_direct.h"
#include "../sys_voicechat.h"
#include "achievements.h"
//#include "win_local.h"

/*
========================
Global variables
========================
*/

extern idCVar net_port;

class idLobbyToSessionCBLocal;

/*
========================
idSessionLocalWin::idSessionLocalWin
========================
*/
class idSessionLocalWin : public idSessionLocal
{
	friend class idLobbyToSessionCBLocal;

public:
	idSessionLocalWin();
	virtual ~idSessionLocalWin();

	// idSessionLocal interface
	virtual void		Initialize();
	virtual void		Shutdown();

	virtual void		InitializeSoundRelatedSystems();
	virtual void		ShutdownSoundRelatedSystems();

	virtual void		PlatformPump();

	virtual void		InviteFriends();
	virtual void		InviteParty();
	virtual void		ShowPartySessions();

	virtual void		ShowSystemMarketplaceUI() const;

	virtual void					ListServers( const idCallback& callback );
	virtual void					CancelListServers();
	virtual int						NumServers() const;
	virtual const serverInfo_t* 	ServerInfo( int i ) const;
	virtual void					ConnectToServer( int i );
	virtual void					ShowServerGamerCardUI( int i );

	virtual void			ShowLobbyUserGamerCardUI( lobbyUserID_t lobbyUserID );

	virtual void			ShowOnlineSignin() {}
	virtual void			UpdateRichPresence() {}
	virtual void			CheckVoicePrivileges() {}

	virtual bool			ProcessInputEvent( const sysEvent_t* ev );

	// System UI
	virtual bool			IsSystemUIShowing() const;
	virtual void			SetSystemUIShowing( bool show );

	// Invites
	virtual void			HandleBootableInvite( int64 lobbyId = 0 );
	virtual void			ClearBootableInvite();
	virtual void			ClearPendingInvite();

	virtual bool			HasPendingBootableInvite();
	virtual void			SetDiscSwapMPInvite( void* parm );
	virtual void* 			GetDiscSwapMPInviteParms();

	virtual void			EnumerateDownloadableContent();

	virtual void 			HandleServerQueryRequest( lobbyAddress_t& remoteAddr, idBitMsg& msg, int msgType );
	virtual void 			HandleServerQueryAck( lobbyAddress_t& remoteAddr, idBitMsg& msg );

	// Leaderboards
	virtual void			LeaderboardUpload( lobbyUserID_t lobbyUserID, const leaderboardDefinition_t* leaderboard, const column_t* stats, const idFile_Memory* attachment = NULL );
	virtual void			LeaderboardDownload( int sessionUserIndex, const leaderboardDefinition_t* leaderboard, int startingRank, int numRows, const idLeaderboardCallback& callback );
	virtual void			LeaderboardDownloadAttachment( int sessionUserIndex, const leaderboardDefinition_t* leaderboard, int64 attachmentID );

	// Scoring (currently just for TrueSkill)
	virtual void			SetLobbyUserRelativeScore( lobbyUserID_t lobbyUserID, int relativeScore, int team ) {}

	virtual void			LeaderboardFlush();

	virtual idNetSessionPort& 	GetPort( bool dedicated = false );
	virtual idLobbyBackend* 	CreateLobbyBackend( const idMatchParameters& p, float skillLevel, idLobbyBackend::lobbyBackendType_t lobbyType );
	virtual idLobbyBackend* 	FindLobbyBackend( const idMatchParameters& p, int numPartyUsers, float skillLevel, idLobbyBackend::lobbyBackendType_t lobbyType );
	virtual idLobbyBackend* 	JoinFromConnectInfo( const lobbyConnectInfo_t& connectInfo , idLobbyBackend::lobbyBackendType_t lobbyType );
	virtual void				DestroyLobbyBackend( idLobbyBackend* lobbyBackend );
	virtual void				PumpLobbies();
	virtual void				JoinAfterSwap( void* joinID );

	virtual bool				GetLobbyAddressFromNetAddress( const netadr_t& netAddr, lobbyAddress_t& outAddr ) const;
	virtual bool				GetNetAddressFromLobbyAddress( const lobbyAddress_t& lobbyAddress, netadr_t& outNetAddr ) const;

public:
	void	Connect_f( const idCmdArgs& args );

private:
	void					EnsurePort();

	idLobbyBackend* 		CreateLobbyInternal( idLobbyBackend::lobbyBackendType_t lobbyType );

	idArray< idLobbyBackend*, 3 > lobbyBackends;

	idNetSessionPort		port;
	bool					canJoinLocalHost;

	idLobbyToSessionCBLocal*	 lobbyToSessionCB;
};

idSessionLocalWin sessionLocalWin;
idSession* session = &sessionLocalWin;

/*
========================
idLobbyToSessionCBLocal
========================
*/
class idLobbyToSessionCBLocal : public idLobbyToSessionCB
{
public:
	idLobbyToSessionCBLocal( idSessionLocalWin* sessionLocalWin_ ) : sessionLocalWin( sessionLocalWin_ ) { }

	virtual bool CanJoinLocalHost() const
	{
		sessionLocalWin->EnsurePort();
		return sessionLocalWin->canJoinLocalHost;
	}
	virtual class idLobbyBackend* GetLobbyBackend( idLobbyBackend::lobbyBackendType_t type ) const
	{
		return sessionLocalWin->lobbyBackends[ type ];
	}

private:
	idSessionLocalWin* 			sessionLocalWin;
};

idLobbyToSessionCBLocal lobbyToSessionCBLocal( &sessionLocalWin );
idLobbyToSessionCB* lobbyToSessionCB = &lobbyToSessionCBLocal;

class idVoiceChatMgrWin : public idVoiceChatMgr
{
public:
	virtual bool	GetLocalChatDataInternal( int talkerIndex, byte* data, int& dataSize )
	{
		return false;
	}
	virtual void	SubmitIncomingChatDataInternal( int talkerIndex, const byte* data, int dataSize ) { }
	virtual bool	TalkerHasData( int talkerIndex )
	{
		return false;
	}
	virtual bool	RegisterTalkerInternal( int index )
	{
		return true;
	}
	virtual void	UnregisterTalkerInternal( int index ) { }
};

/*
========================
idSessionLocalWin::idSessionLocalWin
========================
*/
idSessionLocalWin::idSessionLocalWin()
{
	signInManager		= new( TAG_SYSTEM ) idSignInManagerWin;
	saveGameManager		= new( TAG_SAVEGAMES ) idSaveGameManager();
	voiceChat			= new( TAG_SYSTEM ) idVoiceChatMgrWin();
	lobbyToSessionCB	= new( TAG_SYSTEM ) idLobbyToSessionCBLocal( this );

	canJoinLocalHost	= false;

	lobbyBackends.Zero();
}

/*
========================
idSessionLocalWin::idSessionLocalWin
========================
*/
idSessionLocalWin::~idSessionLocalWin()
{
	delete voiceChat;
	delete lobbyToSessionCB;
}

/*
========================
idSessionLocalWin::Initialize
========================
*/
void idSessionLocalWin::Initialize()
{
	idSessionLocal::Initialize();

	// The shipping path doesn't load title storage
	// Instead, we inject values through code which is protected through steam DRM
	titleStorageVars.Set( "MAX_PLAYERS_ALLOWED", "8" );
	titleStorageLoaded = true;

	// First-time check for downloadable content once game is launched
	EnumerateDownloadableContent();

	GetPartyLobby().Initialize( idLobby::TYPE_PARTY, sessionCallbacks );
	GetGameLobby().Initialize( idLobby::TYPE_GAME, sessionCallbacks );
	GetGameStateLobby().Initialize( idLobby::TYPE_GAME_STATE, sessionCallbacks );

	achievementSystem = new( TAG_SYSTEM ) idAchievementSystemWin();
	achievementSystem->Init();
}

/*
========================
idSessionLocalWin::Shutdown
========================
*/
void idSessionLocalWin::Shutdown()
{
	NET_VERBOSE_PRINT( "NET: Shutdown\n" );
	idSessionLocal::Shutdown();

	MoveToMainMenu();

	// Wait until we fully shutdown
	while( localState != STATE_IDLE && localState != STATE_PRESS_START )
	{
		Pump();
	}

	if( achievementSystem != NULL )
	{
		achievementSystem->Shutdown();
		delete achievementSystem;
		achievementSystem = NULL;
	}
}

/*
========================
idSessionLocalWin::InitializeSoundRelatedSystems
========================
*/
void idSessionLocalWin::InitializeSoundRelatedSystems()
{
	if( voiceChat != NULL )
	{
		voiceChat->Init( NULL );
	}
}

/*
========================
idSessionLocalWin::ShutdownSoundRelatedSystems
========================
*/
void idSessionLocalWin::ShutdownSoundRelatedSystems()
{
	if( voiceChat != NULL )
	{
		voiceChat->Shutdown();
	}
}

/*
========================
idSessionLocalWin::PlatformPump
========================
*/
void idSessionLocalWin::PlatformPump()
{
}

/*
========================
idSessionLocalWin::InviteFriends
========================
*/
void idSessionLocalWin::InviteFriends()
{
}

/*
========================
idSessionLocalWin::InviteParty
========================
*/
void idSessionLocalWin::InviteParty()
{
}

/*
========================
idSessionLocalWin::ShowPartySessions
========================
*/
void idSessionLocalWin::ShowPartySessions()
{
}

/*
========================
idSessionLocalWin::ShowSystemMarketplaceUI
========================
*/
void idSessionLocalWin::ShowSystemMarketplaceUI() const
{
}

/*
========================
idSessionLocalWin::ListServers
========================
*/
void idSessionLocalWin::ListServers( const idCallback& callback )
{
	ListServersCommon();
}

/*
========================
idSessionLocalWin::CancelListServers
========================
*/
void idSessionLocalWin::CancelListServers()
{
}

/*
========================
idSessionLocalWin::NumServers
========================
*/
int idSessionLocalWin::NumServers() const
{
	return 0;
}

/*
========================
idSessionLocalWin::ServerInfo
========================
*/
const serverInfo_t* idSessionLocalWin::ServerInfo( int i ) const
{
	return NULL;
}

/*
========================
idSessionLocalWin::ConnectToServer
========================
*/
void idSessionLocalWin::ConnectToServer( int i )
{
}

/*
========================
idSessionLocalWin::Connect_f
========================
*/
void idSessionLocalWin::Connect_f( const idCmdArgs& args )
{
	if( args.Argc() < 2 )
	{
		idLib::Printf( "Usage: Connect to IP. Use IP:Port to specify port (e.g. 10.0.0.1:1234) \n" );
		return;
	}

	Cancel();

	if( signInManager->GetMasterLocalUser() == NULL )
	{
		signInManager->RegisterLocalUser( 0 );
	}

	lobbyConnectInfo_t connectInfo;

	Sys_StringToNetAdr( args.Argv( 1 ), &connectInfo.netAddr, true );
	// DG: don't use net_port to select port to connect to
	//     the port can be specified in the command, else the default port is used
	if( connectInfo.netAddr.port == 0 )
	{
		connectInfo.netAddr.port = 27015;
	}
	// DG end

	ConnectAndMoveToLobby( GetPartyLobby(), connectInfo, false );
}

/*
========================
void Connect_f
========================
*/
CONSOLE_COMMAND( connect, "Connect to the specified IP", NULL )
{
	sessionLocalWin.Connect_f( args );
}

/*
========================
idSessionLocalWin::ShowServerGamerCardUI
========================
*/
void idSessionLocalWin::ShowServerGamerCardUI( int i )
{
}

/*
========================
idSessionLocalWin::ShowLobbyUserGamerCardUI(
========================
*/
void idSessionLocalWin::ShowLobbyUserGamerCardUI( lobbyUserID_t lobbyUserID )
{
}

/*
========================
idSessionLocalWin::ProcessInputEvent
========================
*/
bool idSessionLocalWin::ProcessInputEvent( const sysEvent_t* ev )
{
	if( GetSignInManager().ProcessInputEvent( ev ) )
	{
		return true;
	}
	return false;
}

/*
========================
idSessionLocalWin::IsSystemUIShowing
========================
*/
bool idSessionLocalWin::IsSystemUIShowing() const
{
	// DG: pausing here when window is out of focus like originally done on windows is hacky
	// it's done with com_pause now.
	return isSysUIShowing;
}

/*
========================
idSessionLocalWin::SetSystemUIShowing
========================
*/
void idSessionLocalWin::SetSystemUIShowing( bool show )
{
	isSysUIShowing = show;
}

/*
========================
idSessionLocalWin::HandleServerQueryRequest
========================
*/
void idSessionLocalWin::HandleServerQueryRequest( lobbyAddress_t& remoteAddr, idBitMsg& msg, int msgType )
{
	NET_VERBOSE_PRINT( "HandleServerQueryRequest from %s\n", remoteAddr.ToString() );
}

/*
========================
idSessionLocalWin::HandleServerQueryAck
========================
*/
void idSessionLocalWin::HandleServerQueryAck( lobbyAddress_t& remoteAddr, idBitMsg& msg )
{
	NET_VERBOSE_PRINT( "HandleServerQueryAck from %s\n", remoteAddr.ToString() );

}

/*
========================
idSessionLocalWin::ClearBootableInvite
========================
*/
void idSessionLocalWin::ClearBootableInvite()
{
}

/*
========================
idSessionLocalWin::ClearPendingInvite
========================
*/
void idSessionLocalWin::ClearPendingInvite()
{
}

/*
========================
idSessionLocalWin::HandleBootableInvite
========================
*/
void idSessionLocalWin::HandleBootableInvite( int64 lobbyId )
{
}

/*
========================
idSessionLocalWin::HasPendingBootableInvite
========================
*/
bool idSessionLocalWin::HasPendingBootableInvite()
{
	return false;
}

/*
========================
idSessionLocal::SetDiscSwapMPInvite
========================
*/
void idSessionLocalWin::SetDiscSwapMPInvite( void* parm )
{
}

/*
========================
idSessionLocal::GetDiscSwapMPInviteParms
========================
*/
void* idSessionLocalWin::GetDiscSwapMPInviteParms()
{
	return NULL;
}

/*
========================
idSessionLocalWin::EnumerateDownloadableContent
========================
*/
void idSessionLocalWin::EnumerateDownloadableContent()
{
}

/*
========================
idSessionLocalWin::LeaderboardUpload
========================
*/
void idSessionLocalWin::LeaderboardUpload( lobbyUserID_t lobbyUserID, const leaderboardDefinition_t* leaderboard, const column_t* stats, const idFile_Memory* attachment )
{
}

/*
========================
idSessionLocalWin::LeaderboardFlush
========================
*/
void idSessionLocalWin::LeaderboardFlush()
{
}

/*
========================
idSessionLocalWin::LeaderboardDownload
========================
*/
void idSessionLocalWin::LeaderboardDownload( int sessionUserIndex, const leaderboardDefinition_t* leaderboard, int startingRank, int numRows, const idLeaderboardCallback& callback )
{
}

/*
========================
idSessionLocalWin::LeaderboardDownloadAttachment
========================
*/
void idSessionLocalWin::LeaderboardDownloadAttachment( int sessionUserIndex, const leaderboardDefinition_t* leaderboard, int64 attachmentID )
{
}

/*
========================
idSessionLocalWin::EnsurePort
========================
*/
void idSessionLocalWin::EnsurePort()
{
	// Init the port using reqular sockets
	if( port.IsOpen() )
	{
		return;		// Already initialized
	}

	if( port.InitPort( net_port.GetInteger(), false ) )
	{
		// TODO: what about canJoinLocalHost when running two instances with different net_port values?
		canJoinLocalHost = false;
	}
	else
	{
		// Assume this is another instantiation on the same machine, and just init using any available port
		port.InitPort( PORT_ANY, false );
		canJoinLocalHost = true;
	}
}

/*
========================
idSessionLocalWin::GetPort
========================
*/
idNetSessionPort& idSessionLocalWin::GetPort( bool dedicated )
{
	EnsurePort();
	return port;
}

/*
========================
idSessionLocalWin::CreateLobbyBackend
========================
*/
idLobbyBackend* idSessionLocalWin::CreateLobbyBackend( const idMatchParameters& p, float skillLevel, idLobbyBackend::lobbyBackendType_t lobbyType )
{
	idLobbyBackend* lobbyBackend = CreateLobbyInternal( lobbyType );
	lobbyBackend->StartHosting( p, skillLevel, lobbyType );
	return lobbyBackend;
}

/*
========================
idSessionLocalWin::FindLobbyBackend
========================
*/
idLobbyBackend* idSessionLocalWin::FindLobbyBackend( const idMatchParameters& p, int numPartyUsers, float skillLevel, idLobbyBackend::lobbyBackendType_t lobbyType )
{
	idLobbyBackend* lobbyBackend = CreateLobbyInternal( lobbyType );
	lobbyBackend->StartFinding( p, numPartyUsers, skillLevel );
	return lobbyBackend;
}

/*
========================
idSessionLocalWin::JoinFromConnectInfo
========================
*/
idLobbyBackend* idSessionLocalWin::JoinFromConnectInfo( const lobbyConnectInfo_t& connectInfo, idLobbyBackend::lobbyBackendType_t lobbyType )
{
	idLobbyBackend* lobbyBackend = CreateLobbyInternal( lobbyType );
	lobbyBackend->JoinFromConnectInfo( connectInfo );
	return lobbyBackend;
}

/*
========================
idSessionLocalWin::DestroyLobbyBackend
========================
*/
void idSessionLocalWin::DestroyLobbyBackend( idLobbyBackend* lobbyBackend )
{
	assert( lobbyBackend != NULL );
	assert( lobbyBackends[lobbyBackend->GetLobbyType()] == lobbyBackend );

	lobbyBackends[lobbyBackend->GetLobbyType()] = NULL;

	lobbyBackend->Shutdown();
	delete lobbyBackend;
}

/*
========================
idSessionLocalWin::PumpLobbies
========================
*/
void idSessionLocalWin::PumpLobbies()
{
	assert( lobbyBackends[idLobbyBackend::TYPE_PARTY] == NULL || lobbyBackends[idLobbyBackend::TYPE_PARTY]->GetLobbyType() == idLobbyBackend::TYPE_PARTY );
	assert( lobbyBackends[idLobbyBackend::TYPE_GAME] == NULL || lobbyBackends[idLobbyBackend::TYPE_GAME]->GetLobbyType() == idLobbyBackend::TYPE_GAME );
	assert( lobbyBackends[idLobbyBackend::TYPE_GAME_STATE] == NULL || lobbyBackends[idLobbyBackend::TYPE_GAME_STATE]->GetLobbyType() == idLobbyBackend::TYPE_GAME_STATE );

	// Pump lobbyBackends
	for( int i = 0; i < lobbyBackends.Num(); i++ )
	{
		if( lobbyBackends[i] != NULL )
		{
			lobbyBackends[i]->Pump();
		}
	}
}

/*
========================
idSessionLocalWin::CreateLobbyInternal
========================
*/
idLobbyBackend* idSessionLocalWin::CreateLobbyInternal( idLobbyBackend::lobbyBackendType_t lobbyType )
{
	EnsurePort();
	idLobbyBackend* lobbyBackend = new( TAG_NETWORKING ) idLobbyBackendDirect();

	lobbyBackend->SetLobbyType( lobbyType );

	assert( lobbyBackends[lobbyType] == NULL );
	lobbyBackends[lobbyType] = lobbyBackend;

	return lobbyBackend;
}

/*
========================
idSessionLocalWin::JoinAfterSwap
========================
*/
void idSessionLocalWin::JoinAfterSwap( void* joinID )
{
}

/*
========================
idSessionLocalWin::GetLobbyAddressFromNetAddress
========================
*/
bool idSessionLocalWin::GetLobbyAddressFromNetAddress( const netadr_t& netAddr, lobbyAddress_t& outAddr ) const
{
	return false;
}

/*
========================
idSessionLocalWin::GetNetAddressFromLobbyAddress
========================
*/
bool idSessionLocalWin::GetNetAddressFromLobbyAddress( const lobbyAddress_t& lobbyAddress, netadr_t& outNetAddr ) const
{
	return false;
}
