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

#ifndef __MULTIPLAYERGAME_H__
#define	__MULTIPLAYERGAME_H__

/*
===============================================================================

	Basic DOOM multiplayer

===============================================================================
*/

class idPlayer;
class idMenuHandler_HUD;
class idMenuHandler_Scoreboard;
class idItemTeam;

enum gameType_t
{
	GAME_SP = -2,
	GAME_RANDOM = -1,
	GAME_DM = 0,
	GAME_TOURNEY,
	GAME_TDM,
	GAME_LASTMAN,
	GAME_CTF,
	GAME_COUNT,
};

// Used by the UI
typedef enum
{
	FLAGSTATUS_INBASE = 0,
	FLAGSTATUS_TAKEN  = 1,
	FLAGSTATUS_STRAY  = 2,
	FLAGSTATUS_NONE   = 3
} flagStatus_t;

typedef struct mpPlayerState_s
{
	int				ping;			// player ping
	int				fragCount;		// kills
	int				teamFragCount;	// team kills
	int				wins;			// wins
	bool			scoreBoardUp;	// toggle based on player scoreboard button, used to activate de-activate the scoreboard gui
	int				deaths;
} mpPlayerState_t;

const int NUM_CHAT_NOTIFY	= 5;
const int CHAT_FADE_TIME	= 400;
const int FRAGLIMIT_DELAY	= 2000;

const int MP_PLAYER_MINFRAGS = -100;
const int MP_PLAYER_MAXFRAGS = 400;	// in CTF frags are player points
const int MP_PLAYER_MAXWINS	= 100;
const int MP_PLAYER_MAXPING	= 999;
const int MP_CTF_MAXPOINTS = 400;

typedef struct mpChatLine_s
{
	idStr			line;
	short			fade;			// starts high and decreases, line is removed once reached 0
} mpChatLine_t;

typedef enum
{
	SND_YOUWIN = 0,
	SND_YOULOSE,
	SND_FIGHT,
	SND_THREE,
	SND_TWO,
	SND_ONE,
	SND_SUDDENDEATH,
	SND_FLAG_CAPTURED_YOURS,
	SND_FLAG_CAPTURED_THEIRS,
	SND_FLAG_RETURN,
	SND_FLAG_TAKEN_YOURS,
	SND_FLAG_TAKEN_THEIRS,
	SND_FLAG_DROPPED_YOURS,
	SND_FLAG_DROPPED_THEIRS,
	SND_COUNT
} snd_evt_t;

class idMultiplayerGame
{
public:

	idMultiplayerGame();

	void			Shutdown();

	// resets everything and prepares for a match
	void			Reset();

	// setup local data for a new player
	void			SpawnPlayer( int clientNum );

	// checks rules and updates state of the mp game
	void			Run();

	// draws mp hud, scoredboard, etc..
	bool			Draw( int clientNum );

	// updates frag counts and potentially ends the match in sudden death
	void			PlayerDeath( idPlayer* dead, idPlayer* killer, bool telefrag );

	void			AddChatLine( VERIFY_FORMAT_STRING const char* fmt, ... );

	void			WriteToSnapshot( idBitMsg& msg ) const;
	void			ReadFromSnapshot( const idBitMsg& msg );

	// game state
	typedef enum
	{
		INACTIVE = 0,						// not running
		WARMUP,								// warming up
		COUNTDOWN,							// post warmup pre-game
		GAMEON,								// game is on
		SUDDENDEATH,						// game is on but in sudden death, first frag wins
		GAMEREVIEW,							// game is over, scoreboard is up. we wait si_gameReviewPause seconds (which has a min value)
		NEXTGAME,
		STATE_COUNT
	} gameState_t;
	static const char* GameStateStrings[ STATE_COUNT ];
	idMultiplayerGame::gameState_t		GetGameState() const;

	static const char* GlobalSoundStrings[ SND_COUNT ];
	void			PlayGlobalSound( int toPlayerNum, snd_evt_t evt, const char* shader = NULL );
	void			PlayTeamSound( int toTeam, snd_evt_t evt, const char* shader = NULL );	// sound that's sent only to member of toTeam team

	// more compact than a chat line
	typedef enum
	{
		MSG_SUICIDE = 0,
		MSG_KILLED,
		MSG_KILLEDTEAM,
		MSG_DIED,
		MSG_SUDDENDEATH,
		MSG_JOINEDSPEC,
		MSG_TIMELIMIT,
		MSG_FRAGLIMIT,
		MSG_TELEFRAGGED,
		MSG_JOINTEAM,
		MSG_HOLYSHIT,
		MSG_POINTLIMIT,
		MSG_FLAGTAKEN,
		MSG_FLAGDROP,
		MSG_FLAGRETURN,
		MSG_FLAGCAPTURE,
		MSG_SCOREUPDATE,
		MSG_LEFTGAME,
		MSG_COUNT
	} msg_evt_t;
	void			PrintMessageEvent( msg_evt_t evt, int parm1 = -1, int parm2 = -1 );

	void			DisconnectClient( int clientNum );
	static void		DropWeapon_f( const idCmdArgs& args );
	static void		MessageMode_f( const idCmdArgs& args );
	static void		VoiceChat_f( const idCmdArgs& args );
	static void		VoiceChatTeam_f( const idCmdArgs& args );

	int				NumActualClients( bool countSpectators, int* teamcount = NULL );
	void			DropWeapon( int clientNum );
	void			MapRestart();
	void			BalanceTeams();
	void			SwitchToTeam( int clientNum, int oldteam, int newteam );
	bool			IsPureReady() const;
	void			ProcessChatMessage( int clientNum, bool team, const char* name, const char* text, const char* sound );
	void			ProcessVoiceChat( int clientNum, bool team, int index );
	bool			HandleGuiEvent( const sysEvent_t* sev );
	bool			IsScoreboardActive();
	void			SetScoreboardActive( bool active );
	void			CleanupScoreboard();

	void			Precache();

	void			ToggleSpectate();

	void			GetSpectateText( idPlayer* player, idStr spectatetext[ 2 ], bool scoreboard );

	void			ClearFrags( int clientNum );

	bool			CanPlay( idPlayer* p );
	bool			WantRespawn( idPlayer* p );

	void			ServerWriteInitialReliableMessages( int clientNum, lobbyUserID_t lobbyUserID );
	void			ClientReadStartState( const idBitMsg& msg );
	void			ClientReadWarmupTime( const idBitMsg& msg );
	void			ClientReadMatchStartedTime( const idBitMsg& msg );
	void			ClientReadAchievementUnlock( const idBitMsg& msg );

	void			ServerClientConnect( int clientNum );
	int             GetFlagPoints( int team );	// Team points in CTF
	void			SetFlagMsg( bool b );		// allow flag event messages to be sent
	bool			IsFlagMsgOn();		// should flag event messages go through?

	int             player_red_flag;            // Ent num of red flag carrier for HUD
	int             player_blue_flag;           // Ent num of blue flag carrier for HUD

	void			PlayerStats( int clientNum, char* data, const int len );

private:
	static const char* teamNames[];
	static const char* skinNames[];
	static const idVec3 skinColors[];
	static const int	numSkins;

	// state vars
	gameState_t		gameState;				// what state the current game is in
	gameState_t		nextState;				// state to switch to when nextStateSwitch is hit

	mpPlayerState_t	playerState[ MAX_CLIENTS ];

	// keep track of clients which are willingly in spectator mode
	// time related
	int				nextStateSwitch;		// time next state switch
	int				warmupEndTime;			// warmup till..
	int				matchStartedTime;		// time current match started

	// tourney
	int				currentTourneyPlayer[2];// our current set of players
	int				lastWinner;				// plays again

	// warmup
	bool			one, two, three;		// keeps count down voice from repeating

	// guis
	idMenuHandler_Scoreboard* scoreboardManager;

	// chat data
	mpChatLine_t	chatHistory[ NUM_CHAT_NOTIFY ];
	int				chatHistoryIndex;
	int				chatHistorySize;		// 0 <= x < NUM_CHAT_NOTIFY
	bool			chatDataUpdated;
	int				lastChatLineTime;

	// rankings are used by UpdateScoreboard and UpdateHud
	int				numRankedPlayers;		// ranked players, others may be empty slots or spectators
	idPlayer* 		rankedPlayers[MAX_CLIENTS];

	bool			pureReady;				// defaults to false, set to true once server game is running with pure checksums
	int				fragLimitTimeout;

	int				voiceChatThrottle;

	int				startFragLimit;			// synchronize to clients in initial state, set on -> GAMEON

	idItemTeam* 	teamFlags[ 2 ];
	int				teamPoints[ 2 ];

	bool			flagMsgOn;

private:
	void			UpdatePlayerRanks();
	void			GameHasBeenWon();

	// updates the passed gui with current score information
	void			UpdateRankColor( idUserInterface* gui, const char* mask, int i, const idVec3& vec );
	void			UpdateScoreboard( idMenuHandler_Scoreboard* scoreboard, idPlayer* owner );

	void			DrawScoreBoard( idPlayer* player );

	void			UpdateHud( idPlayer* player, idMenuHandler_HUD* hudManager );
	bool			Warmup();
	idPlayer* 		FragLimitHit();
	idPlayer* 		FragLeader();
	bool			TimeLimitHit();
	bool			PointLimitHit();
	// return team with most points
	int				WinningTeam();
	void			NewState( gameState_t news, idPlayer* player = NULL );
	void			UpdateWinsLosses( idPlayer* winner );
	// fill any empty tourney slots based on the current tourney ranks
	void			FillTourneySlots();
	void			CycleTourneyPlayers();
	// walk through the tourneyRank to build a wait list for the clients
	void			UpdateTourneyLine();
	const char* 	GameTime();
	void			Clear();
	bool			EnoughClientsToPlay();
	void			ClearChatData();
	void			DrawChat( idPlayer* player );
	// go through the clients, and see if they want to be respawned, and if the game allows it
	// called during normal gameplay for death -> respawn cycles
	// and for a spectator who want back in the game (see param)
	void			CheckRespawns( idPlayer* spectator = NULL );
	// when clients disconnect or join spectate during game, check if we need to end the game
	void			CheckAbortGame();
	void			MessageMode( const idCmdArgs& args );
	// scores in TDM
	void			TeamScore( int entityNumber, int team, int delta );
	void			VoiceChat( const idCmdArgs& args, bool team );
	void			DumpTourneyLine();
	void			SuddenRespawn();

	void			FindTeamFlags();

	void			NewState_Warmup_ServerAndClient();
	void			NewState_Countdown_ServerAndClient();
	void			NewState_GameOn_ServerAndClient();
	void			NewState_GameReview_ServerAndClient();

public:

	const char* 	GetTeamName( int team ) const;
	const char* 	GetSkinName( int skin ) const;
	const idVec3& 	GetSkinColor( int skin ) const;

	idItemTeam* 	GetTeamFlag( int team );
	flagStatus_t    GetFlagStatus( int team );
	void			TeamScoreCTF( int team, int delta );
	void			PlayerScoreCTF( int playerIdx, int delta );
	// returns entityNum to team flag carrier, -1 if no flag carrier
	int				GetFlagCarrier( int team );
	void            UpdateScoreboardFlagStatus();
	void			ReloadScoreboard();

	int				GetGameModes( const char** * gameModes, const char** * gameModesDisplay );

	bool            IsGametypeFlagBased();
	bool            IsGametypeTeamBased();

};

ID_INLINE idMultiplayerGame::gameState_t idMultiplayerGame::GetGameState() const
{
	return gameState;
}

ID_INLINE bool idMultiplayerGame::IsPureReady() const
{
	return pureReady;
}

ID_INLINE void idMultiplayerGame::ClearFrags( int clientNum )
{
	playerState[ clientNum ].fragCount = 0;
}

#endif	/* !__MULTIPLAYERGAME_H__ */

