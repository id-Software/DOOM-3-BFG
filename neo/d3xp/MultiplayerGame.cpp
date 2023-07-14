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

// could be a problem if players manage to go down sudden deaths till this .. oh well
#define LASTMAN_NOLIVES -20

extern idCVar ui_skinIndex;

const char* idMultiplayerGame::teamNames[] = { "#str_02499", "#str_02500" };
const char* idMultiplayerGame::skinNames[] =
{
	"skins/characters/player/marine_mp",
	"skins/characters/player/marine_mp_red",
	"skins/characters/player/marine_mp_blue",
	"skins/characters/player/marine_mp_green",
	"skins/characters/player/marine_mp_yellow",
	"skins/characters/player/marine_mp_purple",
	"skins/characters/player/marine_mp_grey",
	"skins/characters/player/marine_mp_orange"
};
const idVec3 idMultiplayerGame::skinColors[] =
{
	idVec3( 0.25f, 0.25f, 0.25f ), // light grey
	idVec3( 1.00f, 0.00f, 0.00f ), // red
	idVec3( 0.20f, 0.50f, 0.80f ), // blue
	idVec3( 0.00f, 0.80f, 0.10f ), // green
	idVec3( 1.00f, 0.80f, 0.10f ), // yellow
	idVec3( 0.39f, 0.199f, 0.3f ), // purple
	idVec3( 0.425f, 0.484f, 0.445f ), // dark grey
	idVec3( 0.484f, 0.312f, 0.074f ) // orange
};
const int idMultiplayerGame::numSkins = sizeof( idMultiplayerGame::skinNames ) / sizeof( idMultiplayerGame::skinNames[0] );

const char* 	idMultiplayerGame::GetTeamName( int team ) const
{
	return teamNames[idMath::ClampInt( 0, 1, team )];
}
const char* 	idMultiplayerGame::GetSkinName( int skin ) const
{
	return skinNames[idMath::ClampInt( 0, numSkins - 1, skin )];
}
const idVec3& 	idMultiplayerGame::GetSkinColor( int skin ) const
{
	return skinColors[idMath::ClampInt( 0, numSkins - 1, skin )];
}

// make CTF not included in game modes for consoles

const char* gameTypeNames_WithCTF[] = { "Deathmatch", "Tourney", "Team DM", "Last Man", "CTF", "" };
const char* gameTypeDisplayNames_WithCTF[] = { "#str_04260", "#str_04261", "#str_04262", "#str_04263", "#str_swf_mode_ctf", "" };

const char* gameTypeNames_WithoutCTF[] = { "Deathmatch", "Tourney", "Team DM", "Last Man", "" };
const char* gameTypeDisplayNames_WithoutCTF[] = { "#str_04260", "#str_04261", "#str_04262", "#str_04263", "" };

compile_time_assert( GAME_DM == 0 );
compile_time_assert( GAME_TOURNEY == 1 );
compile_time_assert( GAME_TDM == 2 );
compile_time_assert( GAME_LASTMAN == 3 );
compile_time_assert( GAME_CTF == 4 );

idCVar g_spectatorChat( "g_spectatorChat", "0", CVAR_GAME | CVAR_ARCHIVE | CVAR_BOOL, "let spectators talk to everyone during game" );

// global sounds transmitted by index - 0 .. SND_COUNT
// sounds in this list get precached on MP start
const char* idMultiplayerGame::GlobalSoundStrings[] =
{
	"sound/vo/feedback/voc_youwin.wav",
	"sound/vo/feedback/voc_youlose.wav",
	"sound/vo/feedback/fight.wav",
	"sound/vo/feedback/three.wav",
	"sound/vo/feedback/two.wav",
	"sound/vo/feedback/one.wav",
	"sound/vo/feedback/sudden_death.wav",
	"sound/vo/ctf/flag_capped_yours.wav",
	"sound/vo/ctf/flag_capped_theirs.wav",
	"sound/vo/ctf/flag_return.wav",
	"sound/vo/ctf/flag_taken_yours.wav",
	"sound/vo/ctf/flag_taken_theirs.wav",
	"sound/vo/ctf/flag_dropped_yours.wav",
	"sound/vo/ctf/flag_dropped_theirs.wav"
};

// handy verbose
const char* idMultiplayerGame::GameStateStrings[] =
{
	"INACTIVE",
	"WARMUP",
	"COUNTDOWN",
	"GAMEON",
	"SUDDENDEATH",
	"GAMEREVIEW",
	"NEXTGAME"
};

/*
================
idMultiplayerGame::idMultiplayerGame
================
*/
idMultiplayerGame::idMultiplayerGame()
{

	teamFlags[0] = NULL;
	teamFlags[1] = NULL;

	teamPoints[0] = 0;
	teamPoints[1] = 0;

	flagMsgOn = true;

	player_blue_flag = -1;
	player_red_flag = -1;

	Clear();

	scoreboardManager = NULL;
}

/*
================
idMultiplayerGame::Shutdown
================
*/
void idMultiplayerGame::Shutdown()
{
	Clear();
}

/*
================
idMultiplayerGame::Reset
================
*/
void idMultiplayerGame::Reset()
{
	Clear();
	ClearChatData();

	if( common->IsMultiplayer() )
	{
		scoreboardManager = new idMenuHandler_Scoreboard();
		if( scoreboardManager != NULL )
		{
			scoreboardManager->Initialize( "scoreboard", common->SW() );
		}
	}

	ClearChatData();
	warmupEndTime = 0;
	lastChatLineTime = 0;

}

/*
================
idMultiplayerGame::ServerClientConnect
================
*/
void idMultiplayerGame::ServerClientConnect( int clientNum )
{
	memset( &playerState[ clientNum ], 0, sizeof( playerState[ clientNum ] ) );
}

/*
================
idMultiplayerGame::SpawnPlayer
================
*/
void idMultiplayerGame::SpawnPlayer( int clientNum )
{
	idLobbyBase& lobby = session->GetActingGameStateLobbyBase();
	lobbyUserID_t& lobbyUserID = gameLocal.lobbyUserIDs[clientNum];

	AddChatLine( idLocalization::GetString( "#str_07177" ), lobby.GetLobbyUserName( lobbyUserID ) );

	memset( &playerState[ clientNum ], 0, sizeof( playerState[ clientNum ] ) );
	if( !common->IsClient() )
	{
		if( gameLocal.gameType == GAME_LASTMAN )
		{
			// Players spawn with no lives to prevent them from getting the clean slate achievement if
			// they didn't earn it.
			playerState[ clientNum ].fragCount = LASTMAN_NOLIVES;
		}

		idPlayer* p = static_cast< idPlayer* >( gameLocal.entities[ clientNum ] );
		p->spawnedTime = gameLocal.serverTime;
		p->team = 0;
		p->tourneyRank = 0;
		if( gameLocal.gameType == GAME_TOURNEY && gameState == GAMEON )
		{
			p->tourneyRank++;
		}
	}
}

/*
================
idMultiplayerGame::Clear
================
*/
void idMultiplayerGame::Clear()
{
	int i;

	gameState = INACTIVE;
	nextState = INACTIVE;
	nextStateSwitch = 0;
	matchStartedTime = 0;
	currentTourneyPlayer[ 0 ] = -1;
	currentTourneyPlayer[ 1 ] = -1;
	one = two = three = false;
	memset( &playerState, 0 , sizeof( playerState ) );
	lastWinner = -1;
	pureReady = false;

	CleanupScoreboard();

	fragLimitTimeout = 0;
	voiceChatThrottle = 0;
	for( i = 0; i < NUM_CHAT_NOTIFY; i++ )
	{
		chatHistory[ i ].line.Clear();
	}
	startFragLimit = -1;
}

/*
================
idMultiplayerGame::Clear
================
*/
void idMultiplayerGame::CleanupScoreboard()
{
	if( scoreboardManager != NULL )
	{
		delete scoreboardManager;
		scoreboardManager = NULL;
	}
}

/*
================
idMultiplayerGame::GetFlagPoints

Gets number of captures in CTF game.

0 = red team
1 = blue team
================
*/
int idMultiplayerGame::GetFlagPoints( int team )
{
	assert( team <= 1 );

	return teamPoints[ team ];
}

/*
================
idMultiplayerGame::UpdatePlayerRanks
================
*/
void idMultiplayerGame::UpdatePlayerRanks()
{
	int i, j, k;
	idPlayer* players[MAX_CLIENTS];
	idEntity* ent;
	idPlayer* player;

	memset( players, 0, sizeof( players ) );
	numRankedPlayers = 0;

	for( i = 0; i < gameLocal.numClients; i++ )
	{
		ent = gameLocal.entities[ i ];
		if( !ent || !ent->IsType( idPlayer::Type ) )
		{
			continue;
		}
		player = static_cast< idPlayer* >( ent );
		if( !CanPlay( player ) )
		{
			continue;
		}
		if( gameLocal.gameType == GAME_TOURNEY )
		{
			if( i != currentTourneyPlayer[ 0 ] && i != currentTourneyPlayer[ 1 ] )
			{
				continue;
			}
		}
		if( gameLocal.gameType == GAME_LASTMAN && playerState[ i ].fragCount == LASTMAN_NOLIVES )
		{
			continue;
		}
		for( j = 0; j < numRankedPlayers; j++ )
		{
			bool insert = false;

			if( IsGametypeTeamBased() )    /* CTF */
			{
				if( player->team != players[ j ]->team )
				{
					if( playerState[ i ].teamFragCount > playerState[ players[ j ]->entityNumber ].teamFragCount )
					{
						// team scores
						insert = true;
					}
					else if( playerState[ i ].teamFragCount == playerState[ players[ j ]->entityNumber ].teamFragCount && player->team < players[ j ]->team )
					{
						// at equal scores, sort by team number
						insert = true;
					}
				}
				else if( playerState[ i ].fragCount > playerState[ players[ j ]->entityNumber ].fragCount )
				{
					// in the same team, sort by frag count
					insert = true;
				}
			}
			else
			{
				insert = ( playerState[ i ].fragCount > playerState[ players[ j ]->entityNumber ].fragCount );
			}
			if( insert )
			{
				for( k = numRankedPlayers; k > j; k-- )
				{
					players[ k ] = players[ k - 1 ];
				}
				players[ j ] = player;
				break;
			}
		}
		if( j == numRankedPlayers )
		{
			players[ numRankedPlayers ] = player;
		}
		numRankedPlayers++;
	}

	memcpy( rankedPlayers, players, sizeof( players ) );
}


/*
================
idMultiplayerGame::UpdateRankColor
================
*/
void idMultiplayerGame::UpdateRankColor( idUserInterface* gui, const char* mask, int i, const idVec3& vec )
{
	for( int j = 1; j < 4; j++ )
	{
		gui->SetStateFloat( va( mask, i, j ), vec[ j - 1 ] );
	}
}

/*
================
idMultiplayerGame::IsScoreboardActive
================
*/
bool idMultiplayerGame::IsScoreboardActive()
{

	if( scoreboardManager != NULL )
	{
		return scoreboardManager->IsActive();
	}

	return false;
}

/*
================
idMultiplayerGame::HandleGuiEvent
================
*/
bool idMultiplayerGame::HandleGuiEvent( const sysEvent_t* sev )
{

	if( scoreboardManager != NULL && scoreboardManager->IsActive() )
	{
		scoreboardManager->HandleGuiEvent( sev );
		return true;
	}

	return false;
}

/*
================
idMultiplayerGame::UpdateScoreboard
================
*/
void idMultiplayerGame::UpdateScoreboard( idMenuHandler_Scoreboard* scoreboard, idPlayer* owner )
{

	if( owner == NULL )
	{
		return;
	}

	int redScore = 0;
	int blueScore = 0;

	idLobbyBase& lobby = session->GetActingGameStateLobbyBase();
	idList< mpScoreboardInfo > scoreboardInfo;
	for( int i = 0; i < MAX_CLIENTS; ++i )
	{

		if( i < gameLocal.numClients )
		{

			idEntity* ent = gameLocal.entities[ i ];
			if( !ent || !ent->IsType( idPlayer::Type ) )
			{
				continue;
			}
			idPlayer* player = static_cast<idPlayer*>( ent );
			if( !player )
			{
				continue;
			}

			idStr spectateData;
			idStr playerName;
			int score = 0;
			int wins = 0;
			int ping = 0;
			int playerNum = player->entityNumber;
			int team = - 1;

			lobbyUserID_t& lobbyUserID = gameLocal.lobbyUserIDs[ playerNum ];

			if( IsGametypeTeamBased() )
			{
				team = player->team;
				if( team == 0 )
				{
					redScore = playerState[ playerNum ].teamFragCount;
				}
				else if( team == 1 )
				{
					blueScore = playerState[ playerNum ].teamFragCount;
				}
			}

			score = idMath::ClampInt( MP_PLAYER_MINFRAGS, MP_PLAYER_MAXFRAGS, playerState[ playerNum ].fragCount );

			// HACK -
			if( gameLocal.gameType == GAME_LASTMAN && score == LASTMAN_NOLIVES )
			{
				score = 0;
			}

			wins = idMath::ClampInt( 0, MP_PLAYER_MAXWINS, playerState[ playerNum ].wins );
			ping = playerState[ playerNum ].ping;

			if( gameState == WARMUP )
			{
				if( player->spectating )
				{
					spectateData = idLocalization::GetString( "#str_04246" );
				}
				else
				{
					spectateData = idLocalization::GetString( "#str_04247" );
				}
			}
			else
			{
				if( gameLocal.gameType == GAME_LASTMAN && playerState[ playerNum ].fragCount == LASTMAN_NOLIVES )
				{
					spectateData = idLocalization::GetString( "#str_06736" );
				}
				else if( player->spectating )
				{
					spectateData = idLocalization::GetString( "#str_04246" );
				}
			}

			if( playerNum == owner->entityNumber )
			{
				playerName = "^3";
				playerName.Append( lobby.GetLobbyUserName( lobbyUserID ) );
				playerName.Append( "^0" );
			}
			else
			{
				playerName = lobby.GetLobbyUserName( lobbyUserID );
			}

			mpScoreboardInfo info;
			info.voiceState = session->GetDisplayStateFromVoiceState( session->GetLobbyUserVoiceState( lobbyUserID ) );
			info.name = playerName;
			info.team = team;
			info.score = score;
			info.wins = wins;
			info.ping = ping;
			info.playerNum = playerNum;
			info.spectateData = spectateData;

			bool added = false;
			for( int i = 0; i < scoreboardInfo.Num(); ++i )
			{
				if( info.team == scoreboardInfo[i].team )
				{
					if( info.score > scoreboardInfo[i].score )
					{
						scoreboardInfo.Insert( info, i );
						added = true;
						break;
					}
				}
				else if( info.team < scoreboardInfo[i].team )
				{
					scoreboardInfo.Insert( info, i );
					added = true;
					break;
				}
			}

			if( !added )
			{
				scoreboardInfo.Append( info );
			}
		}
	}

	idStr gameInfo;
	if( gameState == GAMEREVIEW )
	{
		int timeRemaining = nextStateSwitch - gameLocal.serverTime;
		int ms = ( int ) ceilf( timeRemaining / 1000.0f );
		if( ms == 1 )
		{
			gameInfo = idLocalization::GetString( "#str_online_game_starts_in_second" );
			gameInfo.Replace( "<DNT_VAL>", idStr( ms ) );
		}
		else if( ms > 0 && ms < 30 )
		{
			gameInfo = idLocalization::GetString( "#str_online_game_starts_in_seconds" );
			gameInfo.Replace( "<DNT_VAL>", idStr( ms ) );
		}
	}
	else
	{
		if( gameLocal.gameType == GAME_LASTMAN )
		{
			if( gameState == GAMEON || gameState == SUDDENDEATH )
			{
				gameInfo = va( "%s: %i", idLocalization::GetString( "#str_04264" ), startFragLimit );
			}
			else
			{
				gameInfo = va( "%s: %i", idLocalization::GetString( "#str_04264" ), gameLocal.serverInfo.GetInt( "si_fragLimit" ) );
			}
		}
		else if( gameLocal.gameType == GAME_CTF )
		{
			int captureLimit = gameLocal.serverInfo.GetInt( "si_fragLimit" );
			gameInfo = va( idLocalization::GetString( "#str_11108" ), captureLimit );
		}
		else
		{
			gameInfo = va( "%s: %i", idLocalization::GetString( "#str_01982" ), gameLocal.serverInfo.GetInt( "si_fragLimit" ) );
		}

		if( gameLocal.serverInfo.GetInt( "si_timeLimit" ) > 0 )
		{
			gameInfo.Append( va( "        %s: %i", idLocalization::GetString( "#str_01983" ), gameLocal.serverInfo.GetInt( "si_timeLimit" ) ) );
		}
	}

	if( scoreboardManager )
	{

		if( IsGametypeFlagBased() )
		{
			scoreboardManager->SetTeamScores( GetFlagPoints( 0 ), GetFlagPoints( 1 ) );
		}
		else if( IsGametypeTeamBased() )
		{
			scoreboardManager->SetTeamScores( redScore, blueScore );
		}

		scoreboardManager->UpdateScoreboard( scoreboardInfo, gameInfo );
		scoreboardManager->UpdateScoreboardSelection();
		scoreboardManager->Update();
	}
}

/*
================
idMultiplayerGame::GameTime
================
*/
const char* idMultiplayerGame::GameTime()
{
	static char buff[16];
	int m, s, t, ms;

	if( gameState == COUNTDOWN )
	{
		ms = warmupEndTime - gameLocal.serverTime;

		// we never want to show double dashes.
		if( ms <= 0 )
		{

			// Try to setup time again.
			warmupEndTime = gameLocal.serverTime + 1000 * cvarSystem->GetCVarInteger( "g_countDown" );
			ms = warmupEndTime - gameLocal.serverTime;
		}

		s = ms / 1000 + 1;
		if( ms <= 0 )
		{
			strcpy( buff, "WMP --" );
		}
		else
		{
			idStr::snPrintf( buff, sizeof( buff ), "WMP %i", s );
		}
	}
	else
	{
		int timeLimit = gameLocal.serverInfo.GetInt( "si_timeLimit" );
		if( timeLimit )
		{
			ms = ( timeLimit * 60000 ) - ( gameLocal.serverTime - matchStartedTime );
		}
		else
		{
			ms = gameLocal.serverTime - matchStartedTime;
		}
		if( ms < 0 )
		{
			ms = 0;
		}

		s = ms / 1000;
		m = s / 60;
		s -= m * 60;
		t = s / 10;
		s -= t * 10;

		idStr::snPrintf( buff, sizeof( buff ), "%i:%i%i", m, t, s );
	}
	return &buff[0];
}

/*
================
idMultiplayerGame::NumActualClients
================
*/
int idMultiplayerGame::NumActualClients( bool countSpectators, int* teamcounts )
{
	idPlayer* p;
	int c = 0;

	if( teamcounts )
	{
		teamcounts[ 0 ] = teamcounts[ 1 ] = 0;
	}
	for( int i = 0 ; i < gameLocal.numClients ; i++ )
	{
		idEntity* ent = gameLocal.entities[ i ];
		if( !ent || !ent->IsType( idPlayer::Type ) )
		{
			continue;
		}
		p = static_cast< idPlayer* >( ent );
		if( countSpectators || CanPlay( p ) )
		{
			c++;
		}
		if( teamcounts && CanPlay( p ) )
		{
			teamcounts[ p->team ]++;
		}
	}
	return c;
}

/*
================
idMultiplayerGame::EnoughClientsToPlay
================
*/
bool idMultiplayerGame::EnoughClientsToPlay()
{
	int teamCount[ 2 ];
	int clients = NumActualClients( false, teamCount );
	if( IsGametypeTeamBased() )    /* CTF */
	{
		return clients >= 2 && teamCount[ 0 ] && teamCount[ 1 ];
	}
	else
	{
		return clients >= 2;
	}
}

/*
================
idMultiplayerGame::FragLimitHit
return the winning player (team player)
if there is no FragLeader(), the game is tied and we return NULL
================
*/
idPlayer* idMultiplayerGame::FragLimitHit()
{
	int i;
	int fragLimit		= gameLocal.serverInfo.GetInt( "si_fragLimit" );
	idPlayer* leader;

	if( IsGametypeFlagBased() )  /* CTF */
	{
		return NULL;
	}

	leader = FragLeader();
	if( !leader )
	{
		return NULL;
	}

	if( fragLimit <= 0 )
	{
		fragLimit = MP_PLAYER_MAXFRAGS;
	}

	if( gameLocal.gameType == GAME_LASTMAN )
	{
		// we have a leader, check if any other players have frags left
		assert( !static_cast< idPlayer* >( leader )->lastManOver );
		for( i = 0 ; i < gameLocal.numClients ; i++ )
		{
			idEntity* ent = gameLocal.entities[ i ];
			if( !ent || !ent->IsType( idPlayer::Type ) )
			{
				continue;
			}
			if( !CanPlay( static_cast< idPlayer* >( ent ) ) )
			{
				continue;
			}
			if( ent == leader )
			{
				continue;
			}
			if( playerState[ ent->entityNumber ].fragCount > 0 )
			{
				return NULL;
			}
		}
		// there is a leader, his score may even be negative, but no one else has frags left or is !lastManOver
		return leader;
	}
	else if( IsGametypeTeamBased() )      /* CTF */
	{
		if( playerState[ leader->entityNumber ].teamFragCount >= fragLimit )
		{
			return leader;
		}
	}
	else
	{
		if( playerState[ leader->entityNumber ].fragCount >= fragLimit )
		{
			return leader;
		}
	}

	return NULL;
}

/*
================
idMultiplayerGame::TimeLimitHit
================
*/
bool idMultiplayerGame::TimeLimitHit()
{
	int timeLimit = gameLocal.serverInfo.GetInt( "si_timeLimit" );
	if( timeLimit )
	{
		if( gameLocal.serverTime >= matchStartedTime + timeLimit * 60000 )
		{
			return true;
		}
	}
	return false;
}

/*
================
idMultiplayerGame::WinningTeam
return winning team
-1 if tied or no players
================
*/
int idMultiplayerGame::WinningTeam()
{
	if( teamPoints[0] > teamPoints[1] )
	{
		return 0;
	}
	if( teamPoints[0] < teamPoints[1] )
	{
		return 1;
	}
	return -1;
}

/*
================
idMultiplayerGame::PointLimitHit
================
*/
bool idMultiplayerGame::PointLimitHit()
{
	int pointLimit = gameLocal.serverInfo.GetInt( "si_fragLimit" );

	// default to MP_CTF_MAXPOINTS if needed
	if( pointLimit > MP_CTF_MAXPOINTS )
	{
		pointLimit = MP_CTF_MAXPOINTS;
	}
	else if( pointLimit <= 0 )
	{
		pointLimit = MP_CTF_MAXPOINTS;
	}

	if( teamPoints[0] == teamPoints[1] )
	{
		return false;
	}

	if( teamPoints[0] >= pointLimit ||
			teamPoints[1] >= pointLimit )
	{
		return true;
	}

	return false;
}

/*
================
idMultiplayerGame::FragLeader
return the current winner ( or a player from the winning team )
NULL if even
================
*/
idPlayer* idMultiplayerGame::FragLeader()
{
	int i;
	int frags[ MAX_CLIENTS ];
	idPlayer* leader = NULL;
	idEntity* ent;
	idPlayer* p;
	int high = -9999;
	int count = 0;
	bool teamLead[ 2 ] = { false, false };

	for( i = 0 ; i < gameLocal.numClients ; i++ )
	{
		ent = gameLocal.entities[ i ];
		if( !ent || !ent->IsType( idPlayer::Type ) )
		{
			continue;
		}
		if( !CanPlay( static_cast< idPlayer* >( ent ) ) )
		{
			continue;
		}
		if( gameLocal.gameType == GAME_TOURNEY && ent->entityNumber != currentTourneyPlayer[ 0 ] && ent->entityNumber != currentTourneyPlayer[ 1 ] )
		{
			continue;
		}
		if( static_cast< idPlayer* >( ent )->lastManOver )
		{
			continue;
		}

		int fragc = ( IsGametypeTeamBased() ) ? playerState[i].teamFragCount : playerState[i].fragCount; /* CTF */
		if( fragc > high )
		{
			high = fragc;
		}

		frags[ i ] = fragc;
	}

	for( i = 0; i < gameLocal.numClients; i++ )
	{
		ent = gameLocal.entities[ i ];
		if( !ent || !ent->IsType( idPlayer::Type ) )
		{
			continue;
		}
		p = static_cast< idPlayer* >( ent );
		p->SetLeader( false );

		if( !CanPlay( p ) )
		{
			continue;
		}
		if( gameLocal.gameType == GAME_TOURNEY && ent->entityNumber != currentTourneyPlayer[ 0 ] && ent->entityNumber != currentTourneyPlayer[ 1 ] )
		{
			continue;
		}
		if( p->lastManOver )
		{
			continue;
		}
		if( p->spectating )
		{
			continue;
		}

		if( frags[ i ] >= high )
		{
			leader = p;
			count++;
			p->SetLeader( true );
			if( IsGametypeTeamBased() )    /* CTF */
			{
				teamLead[ p->team ] = true;
			}
		}
	}

	if( !IsGametypeTeamBased() )    /* CTF */
	{
		// more than one player at the highest frags
		if( count > 1 )
		{
			return NULL;
		}
		else
		{
			return leader;
		}
	}
	else
	{
		if( teamLead[ 0 ] && teamLead[ 1 ] )
		{
			// even game in team play
			return NULL;
		}
		return leader;
	}
}

/*
================
idGameLocal::UpdateWinsLosses
================
*/
void idMultiplayerGame::UpdateWinsLosses( idPlayer* winner )
{
	if( winner )
	{
		// run back through and update win/loss count
		for( int i = 0; i < gameLocal.numClients; i++ )
		{
			idEntity* ent = gameLocal.entities[ i ];
			if( !ent || !ent->IsType( idPlayer::Type ) )
			{
				continue;
			}
			idPlayer* player = static_cast<idPlayer*>( ent );
			if( IsGametypeTeamBased() )    /* CTF */
			{
				if( player == winner || ( player != winner && player->team == winner->team ) )
				{
					playerState[ i ].wins++;
					PlayGlobalSound( i, SND_YOUWIN );
				}
				else
				{
					PlayGlobalSound( i, SND_YOULOSE );
				}
			}
			else if( gameLocal.gameType == GAME_LASTMAN )
			{
				if( player == winner )
				{
					playerState[ i ].wins++;
					PlayGlobalSound( i, SND_YOUWIN );
				}
				else if( !player->wantSpectate )
				{
					PlayGlobalSound( i, SND_YOULOSE );
				}
			}
			else if( gameLocal.gameType == GAME_TOURNEY )
			{
				if( player == winner )
				{
					playerState[ i ].wins++;
					PlayGlobalSound( i, SND_YOUWIN );
				}
				else if( i == currentTourneyPlayer[ 0 ] || i == currentTourneyPlayer[ 1 ] )
				{
					PlayGlobalSound( i, SND_YOULOSE );
				}
			}
			else
			{
				if( player == winner )
				{
					playerState[i].wins++;
					PlayGlobalSound( i, SND_YOUWIN );
				}
				else if( !player->wantSpectate )
				{
					PlayGlobalSound( i, SND_YOULOSE );
				}
			}
		}
	}
	else if( IsGametypeFlagBased() )      /* CTF */
	{
		int winteam = WinningTeam();

		if( winteam != -1 )	// TODO : print a message telling it why the hell the game ended with no winning team?
			for( int i = 0; i < gameLocal.numClients; i++ )
			{
				idEntity* ent = gameLocal.entities[ i ];
				if( !ent || !ent->IsType( idPlayer::Type ) )
				{
					continue;
				}
				idPlayer* player = static_cast<idPlayer*>( ent );

				if( player->team == winteam )
				{
					PlayGlobalSound( i, SND_YOUWIN );
					playerState[ i ].wins++;
				}
				else
				{
					PlayGlobalSound( i, SND_YOULOSE );
				}
			}
	}

	if( winner )
	{
		lastWinner = winner->entityNumber;
	}
	else
	{
		lastWinner = -1;
	}
}

/*
================
idMultiplayerGame::TeamScoreCTF
================
*/
void idMultiplayerGame::TeamScoreCTF( int team, int delta )
{
	if( team < 0 || team > 1 )
	{
		return;
	}

	teamPoints[team] += delta;

	if( gameState == GAMEON || gameState == SUDDENDEATH )
	{
		PrintMessageEvent( MSG_SCOREUPDATE, teamPoints[0], teamPoints[1] );
	}
}

/*
================
idMultiplayerGame::PlayerScoreCTF
================
*/
void idMultiplayerGame::PlayerScoreCTF( int playerIdx, int delta )
{
	if( playerIdx < 0 || playerIdx >= MAX_CLIENTS )
	{
		return;
	}

	playerState[ playerIdx ].fragCount += delta;
}

/*
================
idMultiplayerGame::GetFlagCarrier
================
*/
int	idMultiplayerGame::GetFlagCarrier( int team )
{
	int iFlagCarrier = -1;

	for( int i = 0; i < gameLocal.numClients; i++ )
	{
		idEntity* ent = gameLocal.entities[ i ];
		if( !ent || !ent->IsType( idPlayer::Type ) )
		{
			continue;
		}

		idPlayer* player = static_cast<idPlayer*>( ent );
		if( player->team != team )
		{
			continue;
		}

		if( player->carryingFlag )
		{
			if( iFlagCarrier != -1 )
			{
				gameLocal.Warning( "BUG: more than one flag carrier on %s team", team == 0 ? "red" : "blue" );
			}
			iFlagCarrier = i;
		}
	}

	return iFlagCarrier;
}

/*
================
idMultiplayerGame::TeamScore
================
*/
void idMultiplayerGame::TeamScore( int entityNumber, int team, int delta )
{
	playerState[ entityNumber ].fragCount += delta;
	for( int i = 0 ; i < gameLocal.numClients ; i++ )
	{
		idEntity* ent = gameLocal.entities[ i ];
		if( !ent || !ent->IsType( idPlayer::Type ) )
		{
			continue;
		}
		idPlayer* player = static_cast<idPlayer*>( ent );
		if( player->team == team )
		{
			playerState[ player->entityNumber ].teamFragCount += delta;
		}
	}
}

/*
================
idMultiplayerGame::PlayerDeath
================
*/
void idMultiplayerGame::PlayerDeath( idPlayer* dead, idPlayer* killer, bool telefrag )
{

	// don't do PrintMessageEvent and shit
	assert( !common->IsClient() );

	if( gameState == COUNTDOWN || gameState == WARMUP )
	{
		// No Kill scores are gained during warmup.
		return;
	}

	if( dead == NULL )
	{
		idLib::Warning( "idMultiplayerGame::PlayerDeath dead ptr == NULL, kill will not count" );
		return;
	}

	playerState[ dead->entityNumber ].deaths++;

	if( killer )
	{
		if( gameLocal.gameType == GAME_LASTMAN )
		{
			playerState[ dead->entityNumber ].fragCount--;

		}
		else if( IsGametypeTeamBased() )      /* CTF */
		{
			if( killer == dead || killer->team == dead->team )
			{
				// suicide or teamkill
				TeamScore( killer->entityNumber, killer->team, -1 );
			}
			else
			{
				TeamScore( killer->entityNumber, killer->team, +1 );
			}
		}
		else
		{
			playerState[ killer->entityNumber ].fragCount += ( killer == dead ) ? -1 : 1;
		}
	}

	if( killer && killer == dead )
	{
		PrintMessageEvent( MSG_SUICIDE, dead->entityNumber );
	}
	else if( killer )
	{
		if( telefrag )
		{
			killer->GetAchievementManager().EventCompletesAchievement( ACHIEVEMENT_MP_KILL_PLAYER_VIA_TELEPORT );
			PrintMessageEvent( MSG_TELEFRAGGED, dead->entityNumber, killer->entityNumber );
		}
		else if( IsGametypeTeamBased() && dead->team == killer->team )      /* CTF */
		{
			PrintMessageEvent( MSG_KILLEDTEAM, dead->entityNumber, killer->entityNumber );
		}
		else
		{
			if( killer->PowerUpActive( INVISIBILITY ) )
			{
				killer->GetAchievementManager().EventCompletesAchievement( ACHIEVEMENT_MP_KILL_5_PLAYERS_USING_INVIS );
			}
			if( killer->PowerUpActive( BERSERK ) )
			{
				killer->GetAchievementManager().EventCompletesAchievement( ACHIEVEMENT_MP_USE_BERSERK_TO_KILL_PLAYER );
			}
			PrintMessageEvent( MSG_KILLED, dead->entityNumber, killer->entityNumber );
		}
	}
	else
	{
		PrintMessageEvent( MSG_DIED, dead->entityNumber );
		playerState[ dead->entityNumber ].fragCount--;
	}
}

/*
================
idMultiplayerGame::PlayerStats
================
*/
void idMultiplayerGame::PlayerStats( int clientNum, char* data, const int len )
{

	idEntity* ent;
	int team;

	*data = 0;

	// make sure we don't exceed the client list
	if( clientNum < 0 || clientNum > gameLocal.numClients )
	{
		return;
	}

	// find which team this player is on
	ent = gameLocal.entities[ clientNum ];
	if( ent && ent->IsType( idPlayer::Type ) )
	{
		team = static_cast< idPlayer* >( ent )->team;
	}
	else
	{
		return;
	}

	idStr::snPrintf( data, len, "team=%d score=%ld tks=%ld", team, playerState[ clientNum ].fragCount, playerState[ clientNum ].teamFragCount );

	return;

}

/*
================
idMultiplayerGame::DumpTourneyLine
================
*/
void idMultiplayerGame::DumpTourneyLine()
{
	int i;
	for( i = 0; i < gameLocal.numClients; i++ )
	{
		if( gameLocal.entities[ i ] && gameLocal.entities[ i ]->IsType( idPlayer::Type ) )
		{
			common->Printf( "client %d: rank %d\n", i, static_cast< idPlayer* >( gameLocal.entities[ i ] )->tourneyRank );
		}
	}
}

/*
================
idMultiplayerGame::NewState
================
*/
void idMultiplayerGame::NewState( gameState_t news, idPlayer* player )
{
	idBitMsg	outMsg;
	byte		msgBuf[MAX_GAME_MESSAGE_SIZE];
	int			i;

	assert( news != gameState );
	assert( !common->IsClient() );
	assert( news < STATE_COUNT );
	gameLocal.DPrintf( "%s -> %s\n", GameStateStrings[ gameState ], GameStateStrings[ news ] );
	switch( news )
	{
		case GAMEON:
		{
			gameLocal.LocalMapRestart();
			outMsg.InitWrite( msgBuf, sizeof( msgBuf ) );
			outMsg.WriteBits( 0, 1 );
			session->GetActingGameStateLobbyBase().SendReliable( GAME_RELIABLE_MESSAGE_RESTART, outMsg, false );

			teamPoints[0] = 0;
			teamPoints[1] = 0;

			PlayGlobalSound( -1, SND_FIGHT );
			matchStartedTime = gameLocal.serverTime;

			idBitMsg	matchStartedTimeMsg;
			byte		matchStartedTimeMsgBuf[ sizeof( matchStartedTime ) ];
			matchStartedTimeMsg.InitWrite( matchStartedTimeMsgBuf, sizeof( matchStartedTimeMsgBuf ) );
			matchStartedTimeMsg.WriteLong( matchStartedTime );
			session->GetActingGameStateLobbyBase().SendReliable( GAME_RELIABLE_MESSAGE_MATCH_STARTED_TIME, matchStartedTimeMsg, false );

			fragLimitTimeout = 0;
			for( i = 0; i < gameLocal.numClients; i++ )
			{
				idEntity* ent = gameLocal.entities[ i ];
				if( !ent || !ent->IsType( idPlayer::Type ) )
				{
					continue;
				}
				idPlayer* p = static_cast<idPlayer*>( ent );
				p->wantSpectate = false; // Make sure everyone is in the game.
				p->SetLeader( false ); // don't carry the flag from previous games
				if( gameLocal.gameType == GAME_TOURNEY && currentTourneyPlayer[ 0 ] != i && currentTourneyPlayer[ 1 ] != i )
				{
					p->ServerSpectate( true );
					idLib::Printf( "TOURNEY NewState GAMEON :> Player %d Benched \n", p->entityNumber );
					p->tourneyRank++;
				}
				else
				{
					int fragLimit = gameLocal.serverInfo.GetInt( "si_fragLimit" );
					int startingCount = ( gameLocal.gameType == GAME_LASTMAN ) ? fragLimit : 0;
					playerState[ i ].fragCount = startingCount;
					playerState[ i ].teamFragCount = startingCount;
					if( !static_cast<idPlayer*>( ent )->wantSpectate )
					{
						static_cast<idPlayer*>( ent )->ServerSpectate( false );
						idLib::Printf( "TOURNEY NewState :> Player %d On Deck \n", ent->entityNumber );
						if( gameLocal.gameType == GAME_TOURNEY )
						{
							p->tourneyRank = 0;
						}
					}
				}
				if( CanPlay( p ) )
				{
					p->lastManPresent = true;
				}
				else
				{
					p->lastManPresent = false;
				}
			}
			NewState_GameOn_ServerAndClient();
			break;
		}
		case GAMEREVIEW:
		{
			SetFlagMsg( false );
			nextState = INACTIVE;	// used to abort a game. cancel out any upcoming state change
			// set all players spectating
			for( i = 0; i < gameLocal.numClients; i++ )
			{
				idEntity* ent = gameLocal.entities[ i ];
				if( !ent || !ent->IsType( idPlayer::Type ) )
				{
					continue;
				}
				static_cast<idPlayer*>( ent )->ServerSpectate( true );
				idLib::Printf( "TOURNEY NewState GAMEREVIEW :> Player %d Benched \n", ent->entityNumber );
			}
			UpdateWinsLosses( player );
			SetFlagMsg( true );
			NewState_GameReview_ServerAndClient();
			break;
		}
		case SUDDENDEATH:
		{
			PrintMessageEvent( MSG_SUDDENDEATH );
			PlayGlobalSound( -1, SND_SUDDENDEATH );
			break;
		}
		case COUNTDOWN:
		{
			idBitMsg	outMsg;
			byte		msgBuf[ 128 ];

			warmupEndTime = gameLocal.serverTime + 1000 * cvarSystem->GetCVarInteger( "g_countDown" );

			outMsg.InitWrite( msgBuf, sizeof( msgBuf ) );
			outMsg.WriteLong( warmupEndTime );
			session->GetActingGameStateLobbyBase().SendReliable( GAME_RELIABLE_MESSAGE_WARMUPTIME, outMsg, false );

			// Reset all the scores.
			for( i = 0; i < gameLocal.numClients; i++ )
			{

				int fragLimit = gameLocal.serverInfo.GetInt( "si_fragLimit" );
				int startingCount = ( gameLocal.gameType == GAME_LASTMAN ) ? fragLimit : 0;
				playerState[ i ].fragCount = startingCount;
				playerState[ i ].teamFragCount = startingCount;
				playerState[ i ].deaths = 0;

			}

			NewState_Countdown_ServerAndClient();
			break;
		}
		case WARMUP:
		{
			teamPoints[0] = 0;
			teamPoints[1] = 0;
			if( IsGametypeFlagBased() )
			{
				// reset player scores to zero, only required for CTF
				for( i = 0; i < gameLocal.numClients; i++ )
				{
					idEntity* ent = gameLocal.entities[ i ];
					if( !ent || !ent->IsType( idPlayer::Type ) )
					{
						continue;
					}
					playerState[ i ].fragCount = 0;
				}
			}
			NewState_Warmup_ServerAndClient();
			break;
		}
		default:
			break;
	}

	gameState = news;
}

/*
================
idMultiplayerGame::NewState_Warmup_ServerAndClient
Called on both servers and clients once when the game state changes to WARMUP.
================
*/
void idMultiplayerGame::NewState_Warmup_ServerAndClient()
{
	SetScoreboardActive( false );
}

/*
================
idMultiplayerGame::NewState_Countdown_ServerAndClient
Called on both servers and clients once when the game state changes to COUNTDOWN.
================
*/
void idMultiplayerGame::NewState_Countdown_ServerAndClient()
{
	SetScoreboardActive( false );
}

/*
================
idMultiplayerGame::NewState_GameOn_ServerAndClient
Called on both servers and clients once when the game state changes to GAMEON.
================
*/
void idMultiplayerGame::NewState_GameOn_ServerAndClient()
{
	startFragLimit = gameLocal.serverInfo.GetInt( "si_fragLimit" );
	SetScoreboardActive( false );
}

/*
================
idMultiplayerGame::NewState_GameReview_ServerAndClient
Called on both servers and clients once when the game state changes to GAMEREVIEW.
================
*/
void idMultiplayerGame::NewState_GameReview_ServerAndClient()
{
	SetScoreboardActive( true );
}

/*
================
idMultiplayerGame::FillTourneySlots
NOTE: called each frame during warmup to keep the tourney slots filled
================
*/
void idMultiplayerGame::FillTourneySlots()
{
	int i, j, rankmax, rankmaxindex;
	idEntity* ent;
	idPlayer* p;

	idLib::Printf( "TOURNEY :> Executing FillTourneySlots  \n" );

	// fill up the slots based on tourney ranks
	for( i = 0; i < 2; i++ )
	{
		if( currentTourneyPlayer[ i ] != -1 )
		{
			continue;
		}
		rankmax = -1;
		rankmaxindex = -1;
		for( j = 0; j < gameLocal.numClients; j++ )
		{
			ent = gameLocal.entities[ j ];
			if( !ent || !ent->IsType( idPlayer::Type ) )
			{
				continue;
			}
			if( currentTourneyPlayer[ 0 ] == j || currentTourneyPlayer[ 1 ] == j )
			{
				continue;
			}

			p = static_cast< idPlayer* >( ent );
			if( p->wantSpectate )
			{
				idLib::Printf( "FillTourneySlots: Skipping Player %d ( Wants Spectate )\n", p->entityNumber );
				continue;
			}


			if( p->tourneyRank >= rankmax )
			{
				// when ranks are equal, use time in game
				if( p->tourneyRank == rankmax )
				{
					assert( rankmaxindex >= 0 );
					if( p->spawnedTime > static_cast< idPlayer* >( gameLocal.entities[ rankmaxindex ] )->spawnedTime )
					{
						continue;
					}
				}
				rankmax = static_cast< idPlayer* >( ent )->tourneyRank;
				rankmaxindex = j;
			}
		}
		currentTourneyPlayer[ i ] = rankmaxindex; // may be -1 if we found nothing
	}

	idLib::Printf( "TOURNEY :> Player 1: %d Player 2: %d \n", currentTourneyPlayer[ 0 ], currentTourneyPlayer[ 1 ] );


}

/*
================
idMultiplayerGame::UpdateTourneyLine
we manipulate tourneyRank on player entities for internal ranking. it's easier to deal with.
but we need a real wait list to be synced down to clients for GUI
ignore current players, ignore wantSpectate
================
*/
void idMultiplayerGame::UpdateTourneyLine()
{
	assert( !common->IsClient() );
	if( gameLocal.gameType != GAME_TOURNEY )
	{
		return;
	}

	int globalmax = -1;
	for( int j = 1; j <= gameLocal.numClients; j++ )
	{
		int max = -1;
		int imax = -1;
		for( int i = 0; i < gameLocal.numClients; i++ )
		{
			if( currentTourneyPlayer[ 0 ] == i || currentTourneyPlayer[ 1 ] == i )
			{
				continue;
			}
			idPlayer* p = static_cast< idPlayer* >( gameLocal.entities[ i ] );
			if( !p || p->wantSpectate )
			{
				continue;
			}
			if( p->tourneyRank > max && ( globalmax == -1 || p->tourneyRank < globalmax ) )
			{
				max = p->tourneyRank;
				imax = i;
			}
		}
		if( imax == -1 )
		{
			break;
		}

		idBitMsg outMsg;
		byte msgBuf[1024];
		outMsg.InitWrite( msgBuf, sizeof( msgBuf ) );
		outMsg.WriteByte( j );
		session->GetActingGameStateLobbyBase().SendReliableToLobbyUser( gameLocal.lobbyUserIDs[imax], GAME_RELIABLE_MESSAGE_TOURNEYLINE, outMsg );

		globalmax = max;
	}
}

/*
================
idMultiplayerGame::CycleTourneyPlayers
================
*/
void idMultiplayerGame::CycleTourneyPlayers()
{
	int i;
	idEntity* ent;
	idPlayer* player;

	currentTourneyPlayer[ 0 ] = -1;
	currentTourneyPlayer[ 1 ] = -1;
	// if any, winner from last round will play again
	if( lastWinner != -1 )
	{
		idEntity* ent = gameLocal.entities[ lastWinner ];
		if( ent && ent->IsType( idPlayer::Type ) )
		{
			currentTourneyPlayer[ 0 ] = lastWinner;
		}
	}
	FillTourneySlots();
	// force selected players in/out of the game and update the ranks
	for( i = 0 ; i < gameLocal.numClients ; i++ )
	{
		if( currentTourneyPlayer[ 0 ] == i || currentTourneyPlayer[ 1 ] == i )
		{
			player = static_cast<idPlayer*>( gameLocal.entities[ i ] );
			player->ServerSpectate( false );
			idLib::Printf( "TOURNEY CycleTourneyPlayers:> Player %d On Deck \n", player->entityNumber );
		}
		else
		{
			ent = gameLocal.entities[ i ];
			if( ent && ent->IsType( idPlayer::Type ) )
			{
				player = static_cast<idPlayer*>( gameLocal.entities[ i ] );
				player->ServerSpectate( true );
				idLib::Printf( "TOURNEY CycleTourneyPlayers:> Player %d Benched \n", player->entityNumber );
			}
		}
	}
	UpdateTourneyLine();
}

/*
================
idMultiplayerGame::Warmup
================
*/
bool idMultiplayerGame::Warmup()
{
	return ( gameState == WARMUP );
}


void idMultiplayerGame::GameHasBeenWon()
{


	// Only allow leaderboard submissions within public games.
	const idMatchParameters& matchParameters = session->GetActingGameStateLobbyBase().GetMatchParms();

	if( matchParameters.matchFlags & MATCH_RANKED )
	{

		// Upload all player's scores to the leaderboard.
		for( int playerIdx = 0; playerIdx < gameLocal.numClients; playerIdx++ )
		{

			leaderboardStats_t stats = { playerState[ playerIdx ].fragCount, playerState[ playerIdx ].wins, playerState[ playerIdx ].teamFragCount, playerState[ playerIdx].deaths };

			LeaderboardLocal_Upload( gameLocal.lobbyUserIDs[ playerIdx ], gameLocal.gameType, stats );
		}

		// Flush all the collectively queued leaderboards all at once. ( Otherwise you get a busy signal on the second "flush" )
		session->LeaderboardFlush();
	}

	// Award Any players that have not died. An Achievement
	for( int i = 0; i < gameLocal.numClients; i++ )
	{
		idPlayer* player = static_cast< idPlayer* >( gameLocal.entities[ i ] );
		if( player == NULL )
		{
			continue;
		}

		// Only validate players that were in the tourney.
		if( gameLocal.gameType == GAME_TOURNEY )
		{
			if( i != currentTourneyPlayer[ 0 ] && i != currentTourneyPlayer[ 1 ] )
			{
				continue;
			}
		}

		// In LMS, players that join mid-game will not have ever died
		if( gameLocal.gameType == GAME_LASTMAN )
		{
			if( playerState[i].fragCount == LASTMAN_NOLIVES )
			{
				continue;
			}
		}

		if( playerState[ i ].deaths == 0 )
		{
			player->GetAchievementManager().EventCompletesAchievement( ACHIEVEMENT_MP_COMPLETE_MATCH_WITHOUT_DYING );
		}
	}
}

/*
================
idMultiplayerGame::Run
================
*/
void idMultiplayerGame::Run()
{
	int i, timeLeft;
	idPlayer* player;
	int gameReviewPause;

	assert( common->IsMultiplayer() );
	assert( !common->IsClient() );

	pureReady = true;

	if( gameState == INACTIVE )
	{
		NewState( WARMUP );
	}

	CheckRespawns();

	if( nextState != INACTIVE && gameLocal.serverTime > nextStateSwitch )
	{
		NewState( nextState );
		nextState = INACTIVE;
	}

	idLobbyBase& lobby = session->GetActingGameStateLobbyBase();
	for( i = 0; i < gameLocal.numClients; i++ )
	{
		idPlayer* player = static_cast<idPlayer*>( gameLocal.entities[i] );
		if( player != NULL )
		{
			playerState[i].ping = lobby.GetLobbyUserQoS( gameLocal.lobbyUserIDs[i] );
		}
	}

	switch( gameState )
	{
		case GAMEREVIEW:
		{
			if( nextState == INACTIVE )
			{
				gameReviewPause = cvarSystem->GetCVarInteger( "g_gameReviewPause" );
				nextState = NEXTGAME;
				nextStateSwitch = gameLocal.serverTime + 1000 * gameReviewPause;
			}
			break;
		}
		case NEXTGAME:
		{
			if( nextState == INACTIVE )
			{
				// make sure flags are returned
				if( IsGametypeFlagBased() )
				{
					idItemTeam* flag;
					flag = GetTeamFlag( 0 );
					if( flag )
					{
						flag->Return();
					}
					flag = GetTeamFlag( 1 );
					if( flag )
					{
						flag->Return();
					}
				}
				NewState( WARMUP );
				if( gameLocal.gameType == GAME_TOURNEY )
				{
					CycleTourneyPlayers();
				}
				// put everyone back in from endgame spectate
				for( i = 0; i < gameLocal.numClients; i++ )
				{
					idEntity* ent = gameLocal.entities[ i ];
					if( ent && ent->IsType( idPlayer::Type ) )
					{
						if( !static_cast< idPlayer* >( ent )->wantSpectate )
						{
							CheckRespawns( static_cast<idPlayer*>( ent ) );
						}
					}
				}
			}
			break;
		}
		case WARMUP:
		{
			if( EnoughClientsToPlay() )
			{
				NewState( COUNTDOWN );
				nextState = GAMEON;
				nextStateSwitch = gameLocal.serverTime + 1000 * cvarSystem->GetCVarInteger( "g_countDown" );
			}
			one = two = three = false;
			break;
		}
		case COUNTDOWN:
		{
			timeLeft = ( nextStateSwitch - gameLocal.serverTime ) / 1000 + 1;
			if( timeLeft == 3 && !three )
			{
				PlayGlobalSound( -1, SND_THREE );
				three = true;
			}
			else if( timeLeft == 2 && !two )
			{
				PlayGlobalSound( -1, SND_TWO );
				two = true;
			}
			else if( timeLeft == 1 && !one )
			{
				PlayGlobalSound( -1, SND_ONE );
				one = true;
			}
			break;
		}
		case GAMEON:
		{
			if( IsGametypeFlagBased() )    /* CTF */
			{
				// totally different logic branch for CTF
				if( PointLimitHit() )
				{
					int team = WinningTeam();
					assert( team != -1 );

					NewState( GAMEREVIEW, NULL );
					PrintMessageEvent( MSG_POINTLIMIT, team );
					GameHasBeenWon();

				}
				else if( TimeLimitHit() )
				{
					int team = WinningTeam();
					if( EnoughClientsToPlay() && team == -1 )
					{
						NewState( SUDDENDEATH );
					}
					else
					{
						NewState( GAMEREVIEW, NULL );
						PrintMessageEvent( MSG_TIMELIMIT );
						GameHasBeenWon();
					}
				}
				break;
			}

			player = FragLimitHit();
			if( player )
			{
				// delay between detecting frag limit and ending game. let the death anims play
				if( !fragLimitTimeout )
				{
					common->DPrintf( "enter FragLimit timeout, player %d is leader\n", player->entityNumber );
					fragLimitTimeout = gameLocal.serverTime + FRAGLIMIT_DELAY;
				}
				if( gameLocal.serverTime > fragLimitTimeout )
				{
					NewState( GAMEREVIEW, player );
					PrintMessageEvent( MSG_FRAGLIMIT, IsGametypeTeamBased() ? player->team : player->entityNumber );
					GameHasBeenWon();
				}
			}
			else
			{
				if( fragLimitTimeout )
				{
					// frag limit was hit and cancelled. means the two teams got even during FRAGLIMIT_DELAY
					// enter sudden death, the next frag leader will win
					SuddenRespawn();
					PrintMessageEvent( MSG_HOLYSHIT );
					fragLimitTimeout = 0;
					NewState( SUDDENDEATH );
				}
				else if( TimeLimitHit() )
				{
					player = FragLeader();
					if( !player )
					{
						NewState( SUDDENDEATH );
					}
					else
					{
						NewState( GAMEREVIEW, player );
						PrintMessageEvent( MSG_TIMELIMIT );
						GameHasBeenWon();
					}
				}
				else
				{
					BalanceTeams();
				}
			}
			break;
		}
		case SUDDENDEATH:
		{
			if( IsGametypeFlagBased() )    /* CTF */
			{
				int team = WinningTeam();
				if( team != -1 )
				{
					// TODO : implement pointLimitTimeout
					NewState( GAMEREVIEW, NULL );
					PrintMessageEvent( MSG_POINTLIMIT, team );
					GameHasBeenWon();
				}
				break;
			}

			player = FragLeader();
			if( player )
			{
				if( !fragLimitTimeout )
				{
					common->DPrintf( "enter sudden death FragLeader timeout, player %d is leader\n", player->entityNumber );
					fragLimitTimeout = gameLocal.serverTime + FRAGLIMIT_DELAY;
				}
				if( gameLocal.serverTime > fragLimitTimeout )
				{
					NewState( GAMEREVIEW, player );
					PrintMessageEvent( MSG_FRAGLIMIT, IsGametypeTeamBased() ? player->team : player->entityNumber );
					GameHasBeenWon();
				}
			}
			else if( fragLimitTimeout )
			{
				SuddenRespawn();
				PrintMessageEvent( MSG_HOLYSHIT );
				fragLimitTimeout = 0;
			}
			break;
		}
	}
}

/*
================
idMultiplayerGame::Draw
================
*/
bool idMultiplayerGame::Draw( int clientNum )
{
	idPlayer* player, *viewPlayer;

	// clear the render entities for any players that don't need
	// icons and which might not be thinking because they weren't in
	// the last snapshot.
	for( int i = 0; i < gameLocal.numClients; i++ )
	{
		player = static_cast<idPlayer*>( gameLocal.entities[ i ] );
		if( player && !player->NeedsIcon() )
		{
			player->HidePlayerIcons();
		}
	}

	player = viewPlayer = static_cast<idPlayer*>( gameLocal.entities[ clientNum ] );

	if( player == NULL )
	{
		return false;
	}

	if( player->spectating )
	{
		viewPlayer = static_cast<idPlayer*>( gameLocal.entities[ player->spectator ] );
		if( viewPlayer == NULL )
		{
			return false;
		}

		// if the player you are viewing is holding a flag, hide it.
		idEntity* flag = GetTeamFlag( viewPlayer->team ? 0 : 1 );
		if( flag )
		{
			if( viewPlayer->carryingFlag )
			{
				flag->Hide();
			}
			else
			{
				flag->Show();
			}
		}
	}

	UpdatePlayerRanks();
	UpdateHud( viewPlayer, player->hudManager );
	// use the hud of the local player
	viewPlayer->playerView.RenderPlayerView( player->hudManager );

	idStr spectatetext[ 2 ];
	GetSpectateText( player, spectatetext, true );

	if( scoreboardManager != NULL )
	{
		scoreboardManager->UpdateSpectating( spectatetext[0].c_str(), spectatetext[1].c_str() );
	}

	DrawChat( player );
	DrawScoreBoard( player );

	return true;
}

/*
================
idMultiplayerGame::GetSpectateText
================
*/
void idMultiplayerGame::GetSpectateText( idPlayer* player, idStr spectatetext[ 2 ], bool scoreboard )
{
	if( !player->spectating )
	{
		return;
	}
	if( player->wantSpectate && !scoreboard )
	{
		if( gameLocal.gameType == GAME_LASTMAN && gameState == GAMEON )
		{
			// If we're in GAMEON in lastman, you can't actully spawn, or you'll have an unfair
			// advantage with more lives than everyone else.
			spectatetext[ 0 ] = idLocalization::GetString( "#str_04246" );
			spectatetext[ 0 ] += idLocalization::GetString( "#str_07003" );
		}
		else
		{

			if( gameLocal.gameType == GAME_TOURNEY &&
					( currentTourneyPlayer[0] != -1 && currentTourneyPlayer[1] != -1 ) &&
					( currentTourneyPlayer[0] != player->entityNumber && currentTourneyPlayer[1] != player->entityNumber ) )
			{

				spectatetext[ 0 ] = idLocalization::GetString( "#str_04246" );
				switch( player->tourneyLine )
				{
					case 0:
						spectatetext[ 0 ] += idLocalization::GetString( "#str_07003" );
						break;
					case 1:
						spectatetext[ 0 ] += idLocalization::GetString( "#str_07004" );
						break;
					case 2:
						spectatetext[ 0 ] += idLocalization::GetString( "#str_07005" );
						break;
					default:
						spectatetext[ 0 ] += va( idLocalization::GetString( "#str_07006" ), player->tourneyLine );
						break;
				}

			}
			else
			{
				spectatetext[ 0 ] = idLocalization::GetString( "#str_respawn_message" );
			}
		}
	}
	else
	{
		if( gameLocal.gameType == GAME_TOURNEY )
		{
			spectatetext[ 0 ] = idLocalization::GetString( "#str_04246" );
			switch( player->tourneyLine )
			{
				case 0:
					spectatetext[ 0 ] += idLocalization::GetString( "#str_07003" );
					break;
				case 1:
					spectatetext[ 0 ] += idLocalization::GetString( "#str_07004" );
					break;
				case 2:
					spectatetext[ 0 ] += idLocalization::GetString( "#str_07005" );
					break;
				default:
					spectatetext[ 0 ] += va( idLocalization::GetString( "#str_07006" ), player->tourneyLine );
					break;
			}
		}
		else if( gameLocal.gameType == GAME_LASTMAN )
		{
			spectatetext[ 0 ] = idLocalization::GetString( "#str_07007" );
		}
		else
		{
			spectatetext[ 0 ] = idLocalization::GetString( "#str_04246" );
		}
	}
	if( player->spectator != player->entityNumber )
	{
		idLobbyBase& lobby = session->GetActingGameStateLobbyBase();
		spectatetext[ 1 ] = va( idLocalization::GetString( "#str_07008" ), lobby.GetLobbyUserName( gameLocal.lobbyUserIDs[ player->spectator ] ) );
	}
}

/*
================
idMultiplayerGame::UpdateHud
================
*/
void idMultiplayerGame::UpdateHud( idPlayer* player, idMenuHandler_HUD* hudManager )
{

	if( hudManager && hudManager->GetHud() )
	{

		idMenuScreen_HUD* hud = hudManager->GetHud();

		if( Warmup() )
		{
			hud->UpdateMessage( true, "#str_04251" );

			if( IsGametypeTeamBased() )
			{
				hud->SetTeamScore( 0, 0 );
				hud->SetTeamScore( 1, 0 );
			}

		}
		else if( gameState == SUDDENDEATH )
		{
			hud->UpdateMessage( true, "#str_04252" );
		}
		else
		{
			hud->UpdateGameTime( GameTime() );
		}

		if( IsGametypeTeamBased() || IsGametypeFlagBased() )
		{
			hud->ToggleMPInfo( true, true, IsGametypeFlagBased() );
		}
		else
		{
			hud->ToggleMPInfo( true, false );
		}

		if( gameState == GAMEON || gameState == COUNTDOWN || gameState == WARMUP )
		{
			if( IsGametypeTeamBased() && !IsGametypeFlagBased() )
			{
				for( int i = 0; i < gameLocal.numClients; ++i )
				{
					idEntity* ent = gameLocal.entities[ i ];
					if( ent == NULL || !ent->IsType( idPlayer::Type ) )
					{
						continue;
					}

					idPlayer* player = static_cast< idPlayer* >( ent );
					hud->SetTeamScore( player->team, playerState[ player->entityNumber ].teamFragCount );
				}
			}
		}

		if( IsGametypeFlagBased() || IsGametypeTeamBased() )
		{
			hud->SetTeam( player->team );
		}
		else
		{
			hud->SetTeam( -1 );
		}

	}
}

/*
================
idMultiplayerGame::SetScoreboardActive
================
*/
void idMultiplayerGame::SetScoreboardActive( bool active )
{
	if( scoreboardManager != NULL )
	{
		if( active )
		{
			if( IsGametypeTeamBased() || IsGametypeFlagBased() )
			{
				scoreboardManager->SetActivationScreen( SCOREBOARD_AREA_TEAM, MENU_TRANSITION_SIMPLE );
			}
			else
			{
				scoreboardManager->SetActivationScreen( SCOREBOARD_AREA_DEFAULT, MENU_TRANSITION_SIMPLE );
			}

			scoreboardManager->ActivateMenu( true );
		}
		else
		{

			scoreboardManager->ActivateMenu( false );
		}
	}
}

/*
================
idMultiplayerGame::DrawScoreBoard
================
*/
void idMultiplayerGame::DrawScoreBoard( idPlayer* player )
{
	if( scoreboardManager && scoreboardManager->IsActive() == true )
	{
		UpdateScoreboard( scoreboardManager, player );
	}
}

/*
===============
idMultiplayerGame::ClearChatData
===============
*/
void idMultiplayerGame::ClearChatData()
{
	chatHistoryIndex	= 0;
	chatHistorySize		= 0;
	chatDataUpdated		= true;
}

/*
===============
idMultiplayerGame::AddChatLine
===============
*/
void idMultiplayerGame::AddChatLine( const char* fmt, ... )
{
	idStr temp;
	va_list argptr;

	va_start( argptr, fmt );
	vsprintf( temp, fmt, argptr );
	va_end( argptr );

	gameLocal.Printf( "%s\n", temp.c_str() );

	chatHistory[ chatHistoryIndex % NUM_CHAT_NOTIFY ].line = temp;
	chatHistory[ chatHistoryIndex % NUM_CHAT_NOTIFY ].fade = 6;

	chatHistoryIndex++;
	if( chatHistorySize < NUM_CHAT_NOTIFY )
	{
		chatHistorySize++;
	}
	chatDataUpdated = true;
	lastChatLineTime = Sys_Milliseconds();
}

/*
===============
idMultiplayerGame::DrawChat
===============
*/
void idMultiplayerGame::DrawChat( idPlayer* player )
{
	int i, j;

	if( player )
	{
		if( Sys_Milliseconds() - lastChatLineTime > CHAT_FADE_TIME )
		{
			if( chatHistorySize > 0 )
			{
				for( i = chatHistoryIndex - chatHistorySize; i < chatHistoryIndex; i++ )
				{
					chatHistory[ i % NUM_CHAT_NOTIFY ].fade--;
					if( chatHistory[ i % NUM_CHAT_NOTIFY ].fade < 0 )
					{
						chatHistorySize--; // this assumes the removals are always at the beginning
					}
				}
				chatDataUpdated = true;
			}
			lastChatLineTime = Sys_Milliseconds();
		}
		if( chatDataUpdated )
		{
			j = 0;
			i = chatHistoryIndex - chatHistorySize;
			while( i < chatHistoryIndex )
			{
				player->AddChatMessage( j, Min( 4, ( int )chatHistory[ i % NUM_CHAT_NOTIFY ].fade ), chatHistory[ i % NUM_CHAT_NOTIFY ].line );
				j++;
				i++;
			}
			while( j < NUM_CHAT_NOTIFY )
			{
				player->ClearChatMessage( j );
				j++;
			}
			chatDataUpdated = false;
		}

	}
}

//D3XP: Adding one to frag count to allow for the negative flag in numbers greater than 255
const int ASYNC_PLAYER_FRAG_BITS = -( idMath::BitsForInteger( MP_PLAYER_MAXFRAGS - MP_PLAYER_MINFRAGS ) + 1 );	// player can have negative frags
const int ASYNC_PLAYER_WINS_BITS = idMath::BitsForInteger( MP_PLAYER_MAXWINS );
const int ASYNC_PLAYER_PING_BITS = idMath::BitsForInteger( MP_PLAYER_MAXPING );

/*
================
idMultiplayerGame::WriteToSnapshot
================
*/
void idMultiplayerGame::WriteToSnapshot( idBitMsg& msg ) const
{
	int i;
	int value;

	// This is a hack - I need a place to read the lobby ids before the player entities are
	// read (SpawnPlayer requires a valid lobby id for the player).
	for( int i = 0; i < gameLocal.lobbyUserIDs.Num(); ++i )
	{
		gameLocal.lobbyUserIDs[i].WriteToMsg( msg );
	}

	msg.WriteByte( gameState );
	msg.WriteLong( nextStateSwitch );
	msg.WriteShort( currentTourneyPlayer[ 0 ] );
	msg.WriteShort( currentTourneyPlayer[ 1 ] );
	for( i = 0; i < MAX_CLIENTS; i++ )
	{
		// clamp all values to min/max possible value that we can send over
		value = idMath::ClampInt( MP_PLAYER_MINFRAGS, MP_PLAYER_MAXFRAGS, playerState[i].fragCount );
		msg.WriteBits( value, ASYNC_PLAYER_FRAG_BITS );
		value = idMath::ClampInt( MP_PLAYER_MINFRAGS, MP_PLAYER_MAXFRAGS, playerState[i].teamFragCount );
		msg.WriteBits( value, ASYNC_PLAYER_FRAG_BITS );
		value = idMath::ClampInt( 0, MP_PLAYER_MAXWINS, playerState[i].wins );
		msg.WriteBits( value, ASYNC_PLAYER_WINS_BITS );
		value = idMath::ClampInt( 0, MP_PLAYER_MAXPING, playerState[i].ping );
		msg.WriteBits( value, ASYNC_PLAYER_PING_BITS );
	}

	msg.WriteShort( teamPoints[0] );
	msg.WriteShort( teamPoints[1] );
	msg.WriteShort( player_red_flag );
	msg.WriteShort( player_blue_flag );
}

/*
================
idMultiplayerGame::ReadFromSnapshot
================
*/
void idMultiplayerGame::ReadFromSnapshot( const idBitMsg& msg )
{
	int i;
	gameState_t newState;

	// This is a hack - I need a place to read the lobby ids before the player entities are
	// read (SpawnPlayer requires a valid lobby id for the player).
	for( int i = 0; i < gameLocal.lobbyUserIDs.Num(); ++i )
	{
		gameLocal.lobbyUserIDs[i].ReadFromMsg( msg );
	}

	newState = ( idMultiplayerGame::gameState_t )msg.ReadByte();
	nextStateSwitch = msg.ReadLong();
	if( newState != gameState && newState < STATE_COUNT )
	{
		gameLocal.DPrintf( "%s -> %s\n", GameStateStrings[ gameState ], GameStateStrings[ newState ] );
		gameState = newState;

		switch( gameState )
		{
			case GAMEON:
			{
				NewState_GameOn_ServerAndClient();
				break;
			}
			case GAMEREVIEW:
			{
				NewState_GameReview_ServerAndClient();
				break;
			}
			case WARMUP:
			{
				NewState_Warmup_ServerAndClient();
				break;
			}
			case COUNTDOWN:
			{
				NewState_Countdown_ServerAndClient();
				break;
			}
		}
	}
	currentTourneyPlayer[ 0 ] = msg.ReadShort();
	currentTourneyPlayer[ 1 ] = msg.ReadShort();
	for( i = 0; i < MAX_CLIENTS; i++ )
	{
		playerState[i].fragCount = msg.ReadBits( ASYNC_PLAYER_FRAG_BITS );
		playerState[i].teamFragCount = msg.ReadBits( ASYNC_PLAYER_FRAG_BITS );
		playerState[i].wins = msg.ReadBits( ASYNC_PLAYER_WINS_BITS );
		playerState[i].ping = msg.ReadBits( ASYNC_PLAYER_PING_BITS );
	}

	teamPoints[0] = msg.ReadShort();
	teamPoints[1] = msg.ReadShort();

	player_red_flag = msg.ReadShort();
	player_blue_flag = msg.ReadShort();
}

/*
================
idMultiplayerGame::PlayGlobalSound
================
*/
void idMultiplayerGame::PlayGlobalSound( int toPlayerNum, snd_evt_t evt, const char* shader )
{

	if( toPlayerNum < 0 || toPlayerNum == gameLocal.GetLocalClientNum() )
	{
		if( shader )
		{
			if( gameSoundWorld )
			{
				gameSoundWorld->PlayShaderDirectly( shader );
			}
		}
		else
		{
			if( gameSoundWorld )
			{
				gameSoundWorld->PlayShaderDirectly( GlobalSoundStrings[ evt ] );
			}
		}
	}

	if( !common->IsClient() && toPlayerNum != gameLocal.GetLocalClientNum() )
	{
		idBitMsg outMsg;
		byte msgBuf[1024];
		outMsg.InitWrite( msgBuf, sizeof( msgBuf ) );
		int type = 0;

		if( shader )
		{
			const idSoundShader* shaderDecl = declManager->FindSound( shader );
			if( !shaderDecl )
			{
				return;
			}
			outMsg.WriteLong( gameLocal.ServerRemapDecl( -1, DECL_SOUND, shaderDecl->Index() ) );
			type = GAME_RELIABLE_MESSAGE_SOUND_INDEX;
		}
		else
		{
			outMsg.WriteByte( evt );
			type = GAME_RELIABLE_MESSAGE_SOUND_EVENT;
		}
		if( toPlayerNum >= 0 )
		{
			session->GetActingGameStateLobbyBase().SendReliableToLobbyUser( gameLocal.lobbyUserIDs[toPlayerNum], type, outMsg );
		}
		else
		{
			session->GetActingGameStateLobbyBase().SendReliable( type, outMsg, false );
		}
	}
}

/*
================
idMultiplayerGame::PlayTeamSound
================
*/
void idMultiplayerGame::PlayTeamSound( int toTeam, snd_evt_t evt, const char* shader )
{
	for( int i = 0; i < gameLocal.numClients; i++ )
	{
		idEntity* ent = gameLocal.entities[ i ];
		if( !ent || !ent->IsType( idPlayer::Type ) )
		{
			continue;
		}
		idPlayer* player = static_cast<idPlayer*>( ent );
		if( player->team != toTeam )
		{
			continue;
		}
		PlayGlobalSound( i, evt, shader );
	}
}

/*
================
idMultiplayerGame::PrintMessageEvent
================
*/
void idMultiplayerGame::PrintMessageEvent( msg_evt_t evt, int parm1, int parm2 )
{
	idLobbyBase& lobby = session->GetActingGameStateLobbyBase();

	switch( evt )
	{
		case MSG_LEFTGAME:
			assert( parm1 >= 0 );
			AddChatLine( idLocalization::GetString( "#str_11604" ), lobby.GetLobbyUserName( gameLocal.lobbyUserIDs[parm1] ) );
			break;
		case MSG_SUICIDE:
			assert( parm1 >= 0 );
			AddChatLine( idLocalization::GetString( "#str_04293" ), lobby.GetLobbyUserName( gameLocal.lobbyUserIDs[parm1] ) );
			break;
		case MSG_KILLED:
			assert( parm1 >= 0 && parm2 >= 0 );
			AddChatLine( idLocalization::GetString( "#str_04292" ), lobby.GetLobbyUserName( gameLocal.lobbyUserIDs[parm1] ), lobby.GetLobbyUserName( gameLocal.lobbyUserIDs[parm2] ) );
			break;
		case MSG_KILLEDTEAM:
			assert( parm1 >= 0 && parm2 >= 0 );
			AddChatLine( idLocalization::GetString( "#str_04291" ), lobby.GetLobbyUserName( gameLocal.lobbyUserIDs[parm1] ), lobby.GetLobbyUserName( gameLocal.lobbyUserIDs[parm2] ) );
			break;
		case MSG_TELEFRAGGED:
			assert( parm1 >= 0 && parm2 >= 0 );
			AddChatLine( idLocalization::GetString( "#str_04290" ), lobby.GetLobbyUserName( gameLocal.lobbyUserIDs[parm1] ), lobby.GetLobbyUserName( gameLocal.lobbyUserIDs[parm2] ) );
			break;
		case MSG_DIED:
			assert( parm1 >= 0 );
			AddChatLine( idLocalization::GetString( "#str_04289" ), lobby.GetLobbyUserName( gameLocal.lobbyUserIDs[parm1] ) );
			break;
		case MSG_SUDDENDEATH:
			AddChatLine( idLocalization::GetString( "#str_04287" ) );
			break;
		case MSG_JOINEDSPEC:
			AddChatLine( idLocalization::GetString( "#str_04285" ), lobby.GetLobbyUserName( gameLocal.lobbyUserIDs[parm1] ) );
			break;
		case MSG_TIMELIMIT:
			AddChatLine( idLocalization::GetString( "#str_04284" ) );
			break;
		case MSG_FRAGLIMIT:
			if( gameLocal.gameType == GAME_LASTMAN )
			{
				AddChatLine( idLocalization::GetString( "#str_04283" ), lobby.GetLobbyUserName( gameLocal.lobbyUserIDs[parm1] ) );
			}
			else if( IsGametypeTeamBased() )      /* CTF */
			{
				AddChatLine( idLocalization::GetString( "#str_04282" ), idLocalization::GetString( teamNames[ parm1 ] ) );
			}
			else
			{
				AddChatLine( idLocalization::GetString( "#str_04281" ), lobby.GetLobbyUserName( gameLocal.lobbyUserIDs[parm1] ) );
			}
			break;
		case MSG_JOINTEAM:
			AddChatLine( idLocalization::GetString( "#str_04280" ), lobby.GetLobbyUserName( gameLocal.lobbyUserIDs[parm1] ), parm2 ? idLocalization::GetString( "#str_02500" ) : idLocalization::GetString( "#str_02499" ) );
			break;
		case MSG_HOLYSHIT:
			AddChatLine( idLocalization::GetString( "#str_06732" ) );
			break;
		case MSG_POINTLIMIT:
			AddChatLine( idLocalization::GetString( "#str_11100" ), parm1 ? idLocalization::GetString( "#str_11110" ) : idLocalization::GetString( "#str_11111" ) );
			break;

		case MSG_FLAGTAKEN :
			if( gameLocal.GetLocalPlayer() == NULL )
			{
				break;
			}

			if( parm2 < 0 || parm2 >= MAX_CLIENTS )
			{
				break;
			}

			if( gameLocal.GetLocalPlayer()->team != parm1 )
			{
				AddChatLine( idLocalization::GetString( "#str_11101" ), lobby.GetLobbyUserName( gameLocal.lobbyUserIDs[parm2] ) );	// your team
			}
			else
			{
				AddChatLine( idLocalization::GetString( "#str_11102" ), lobby.GetLobbyUserName( gameLocal.lobbyUserIDs[parm2] ) );	// enemy
			}
			break;

		case MSG_FLAGDROP :
			if( gameLocal.GetLocalPlayer() == NULL )
			{
				break;
			}

			if( gameLocal.GetLocalPlayer()->team != parm1 )
			{
				AddChatLine( idLocalization::GetString( "#str_11103" ) );	// your team
			}
			else
			{
				AddChatLine( idLocalization::GetString( "#str_11104" ) );	// enemy
			}
			break;

		case MSG_FLAGRETURN :
			if( gameLocal.GetLocalPlayer() == NULL )
			{
				break;
			}

			if( parm2 >= 0 && parm2 < MAX_CLIENTS )
			{
				if( gameLocal.GetLocalPlayer()->team != parm1 )
				{
					AddChatLine( idLocalization::GetString( "#str_11120" ), lobby.GetLobbyUserName( gameLocal.lobbyUserIDs[parm2] ) );	// your team
				}
				else
				{
					AddChatLine( idLocalization::GetString( "#str_11121" ), lobby.GetLobbyUserName( gameLocal.lobbyUserIDs[parm2] ) );	// enemy
				}
			}
			else
			{
				AddChatLine( idLocalization::GetString( "#str_11105" ), parm1 ? idLocalization::GetString( "#str_11110" ) : idLocalization::GetString( "#str_11111" ) );
			}
			break;

		case MSG_FLAGCAPTURE :
			if( gameLocal.GetLocalPlayer() == NULL )
			{
				break;
			}

			if( parm2 < 0 || parm2 >= MAX_CLIENTS )
			{
				break;
			}

			if( gameLocal.GetLocalPlayer()->team != parm1 )
			{
				AddChatLine( idLocalization::GetString( "#str_11122" ), lobby.GetLobbyUserName( gameLocal.lobbyUserIDs[parm2] ) );	// your team
			}
			else
			{
				AddChatLine( idLocalization::GetString( "#str_11123" ), lobby.GetLobbyUserName( gameLocal.lobbyUserIDs[parm2] ) );	// enemy
			}

//			AddChatLine( idLocalization::GetString( "#str_11106" ), parm1 ? idLocalization::GetString( "#str_11110" ) : idLocalization::GetString( "#str_11111" ) );
			break;

		case MSG_SCOREUPDATE:
			AddChatLine( idLocalization::GetString( "#str_11107" ), parm1, parm2 );
			break;
		default:
			gameLocal.DPrintf( "PrintMessageEvent: unknown message type %d\n", evt );
			return;
	}
	if( !common->IsClient() )
	{
		idBitMsg outMsg;
		byte msgBuf[1024];
		outMsg.InitWrite( msgBuf, sizeof( msgBuf ) );
		outMsg.WriteByte( evt );
		outMsg.WriteByte( parm1 );
		outMsg.WriteByte( parm2 );
		session->GetActingGameStateLobbyBase().SendReliable( GAME_RELIABLE_MESSAGE_DB, outMsg, false );
	}
}

/*
================
idMultiplayerGame::SuddenRespawns
solely for LMN if an end game ( fragLimitTimeout ) was entered and aborted before expiration
LMN players which still have lives left need to be respawned without being marked lastManOver
================
*/
void idMultiplayerGame::SuddenRespawn()
{
	int i;

	if( gameLocal.gameType != GAME_LASTMAN )
	{
		return;
	}

	for( i = 0; i < gameLocal.numClients; i++ )
	{
		if( !gameLocal.entities[ i ] || !gameLocal.entities[ i ]->IsType( idPlayer::Type ) )
		{
			continue;
		}
		if( !CanPlay( static_cast< idPlayer* >( gameLocal.entities[ i ] ) ) )
		{
			continue;
		}
		if( static_cast< idPlayer* >( gameLocal.entities[ i ] )->lastManOver )
		{
			continue;
		}
		static_cast< idPlayer* >( gameLocal.entities[ i ] )->lastManPlayAgain = true;
	}
}

/*
================
idMultiplayerGame::CheckSpawns
================
*/
void idMultiplayerGame::CheckRespawns( idPlayer* spectator )
{
	for( int i = 0 ; i < gameLocal.numClients ; i++ )
	{
		idEntity* ent = gameLocal.entities[ i ];
		if( !ent || !ent->IsType( idPlayer::Type ) )
		{
			continue;
		}
		idPlayer* p = static_cast<idPlayer*>( ent );
		// once we hit sudden death, nobody respawns till game has ended
		if( WantRespawn( p ) || p == spectator )
		{
			if( gameState == SUDDENDEATH && gameLocal.gameType != GAME_LASTMAN )
			{
				// respawn rules while sudden death are different
				// sudden death may trigger while a player is dead, so there are still cases where we need to respawn
				// don't do any respawns while we are in end game delay though
				if( !fragLimitTimeout )
				{
					if( IsGametypeTeamBased() || p->IsLeader() )                        /* CTF */
					{
#ifdef _DEBUG
						if( gameLocal.gameType == GAME_TOURNEY )
						{
							assert( p->entityNumber == currentTourneyPlayer[ 0 ] || p->entityNumber == currentTourneyPlayer[ 1 ] );
						}
#endif
						p->ServerSpectate( false );
					}
					else if( !p->IsLeader() )
					{
						// sudden death is rolling, this player is not a leader, have him spectate
						p->ServerSpectate( true );
						CheckAbortGame();
					}
				}
			}
			else
			{
				if( gameLocal.gameType == GAME_DM ||		// CTF : 3wave sboily, was DM really included before?
						IsGametypeTeamBased() )
				{
					if( gameState == WARMUP || gameState == COUNTDOWN || gameState == GAMEON )
					{
						p->ServerSpectate( false );
					}
				}
				else if( gameLocal.gameType == GAME_TOURNEY )
				{
					if( i == currentTourneyPlayer[ 0 ] || i == currentTourneyPlayer[ 1 ] )
					{
						if( gameState == WARMUP || gameState == COUNTDOWN || gameState == GAMEON )
						{
							p->ServerSpectate( false );
							idLib::Printf( "TOURNEY CheckRespawns :> Player %d On Deck \n", p->entityNumber );
						}
					}
					else if( gameState == WARMUP )
					{
						// make sure empty tourney slots get filled first
						FillTourneySlots();
						if( i == currentTourneyPlayer[ 0 ] || i == currentTourneyPlayer[ 1 ] )
						{
							p->ServerSpectate( false );
							idLib::Printf( "TOURNEY CheckRespawns WARMUP :> Player %d On Deck \n", p->entityNumber );
						}
					}
				}
				else if( gameLocal.gameType == GAME_LASTMAN )
				{
					if( gameState == WARMUP || gameState == COUNTDOWN )
					{
						// Player has spawned in game, give him lives.
						playerState[ i ].fragCount = gameLocal.serverInfo.GetInt( "si_fragLimit" );
						p->ServerSpectate( false );
					}
					else if( gameState == GAMEON || gameState == SUDDENDEATH )
					{
						if( gameState == GAMEON && playerState[ i ].fragCount > 0 && p->lastManPresent )
						{
							assert( !p->lastManOver );
							p->ServerSpectate( false );
						}
						else if( p->lastManPlayAgain && p->lastManPresent )
						{
							assert( gameState == SUDDENDEATH );
							p->ServerSpectate( false );
						}
						else
						{
							// if a fragLimitTimeout was engaged, do NOT mark lastManOver as that could mean
							// everyone ends up spectator and game is stalled with no end
							// if the frag limit delay is engaged and cancels out before expiring, LMN players are
							// respawned to play the tie again ( through SuddenRespawn and lastManPlayAgain )
							if( !fragLimitTimeout && !p->lastManOver )
							{
								common->DPrintf( "client %d has lost all last man lives\n", i );
								// end of the game for this guy, send him to spectators
								p->lastManOver = true;
								// clients don't have access to lastManOver
								// so set the fragCount to something silly ( used in scoreboard and player ranking )
								playerState[ i ].fragCount = LASTMAN_NOLIVES;
								p->ServerSpectate( true );

								//Check for a situation where the last two player dies at the same time and don't
								//try to respawn manually...This was causing all players to go into spectate mode
								//and the server got stuck
								{
									int j;
									for( j = 0; j < gameLocal.numClients; j++ )
									{
										if( !gameLocal.entities[ j ] )
										{
											continue;
										}
										if( !CanPlay( static_cast< idPlayer* >( gameLocal.entities[ j ] ) ) )
										{
											continue;
										}
										if( !static_cast< idPlayer* >( gameLocal.entities[ j ] )->lastManOver )
										{
											break;
										}
									}
									if( j == gameLocal.numClients )
									{
										//Everyone is dead so don't allow this player to spectate
										//so the match will end
										p->ServerSpectate( false );
									}
								}
							}
						}
					}
				}
			}
		}
		else if( p->wantSpectate && !p->spectating )
		{
			playerState[ i ].fragCount = 0; // whenever you willingly go spectate during game, your score resets
			p->ServerSpectate( true );
			idLib::Printf( "TOURNEY CheckRespawns :> Player %d Wants Spectate \n", p->entityNumber );
			UpdateTourneyLine();
			CheckAbortGame();
		}
	}
}

/*
================
idMultiplayerGame::DropWeapon
================
*/
void idMultiplayerGame::DropWeapon( int clientNum )
{
	assert( !common->IsClient() );
	idEntity* ent = gameLocal.entities[ clientNum ];
	if( !ent || !ent->IsType( idPlayer::Type ) )
	{
		return;
	}
	static_cast< idPlayer* >( ent )->DropWeapon( false );
}

/*
================
idMultiplayerGame::DropWeapon_f
================
*/
void idMultiplayerGame::DropWeapon_f( const idCmdArgs& args )
{
	if( !common->IsMultiplayer() )
	{
		common->Printf( "clientDropWeapon: only valid in multiplayer\n" );
		return;
	}
	idBitMsg outMsg;
	session->GetActingGameStateLobbyBase().SendReliableToHost( GAME_RELIABLE_MESSAGE_DROPWEAPON, outMsg );
}

/*
================
idMultiplayerGame::MessageMode_f
================
*/
void idMultiplayerGame::MessageMode_f( const idCmdArgs& args )
{
	if( !common->IsMultiplayer() )
	{
		return;
	}
	gameLocal.mpGame.MessageMode( args );
}

/*
================
idMultiplayerGame::MessageMode
================
*/
void idMultiplayerGame::MessageMode( const idCmdArgs& args )
{
	idEntity* ent = gameLocal.entities[ gameLocal.GetLocalClientNum() ];
	if( !ent || !ent->IsType( idPlayer::Type ) )
	{
		return;
	}
	idPlayer* player = static_cast< idPlayer* >( ent );
	if( player && !player->spectating )
	{
		if( args.Argc() != 2 )
		{
			player->isChatting = 1;
		}
		else
		{
			player->isChatting = 2;
		}
	}
}

/*
================
idMultiplayerGame::DisconnectClient
================
*/
void idMultiplayerGame::DisconnectClient( int clientNum )
{
	if( lastWinner == clientNum )
	{
		lastWinner = -1;
	}

	// Show that the user has left the game.
	PrintMessageEvent( MSG_LEFTGAME, clientNum, -1 );

	UpdatePlayerRanks();
	CheckAbortGame();
}

/*
================
idMultiplayerGame::CheckAbortGame
================
*/
void idMultiplayerGame::CheckAbortGame()
{
	int i;
	if( gameLocal.gameType == GAME_TOURNEY && gameState == WARMUP )
	{
		// if a tourney player joined spectators, let someone else have his spot
		for( i = 0; i < 2; i++ )
		{
			if( !gameLocal.entities[ currentTourneyPlayer[ i ] ] || static_cast< idPlayer* >( gameLocal.entities[ currentTourneyPlayer[ i ] ] )->spectating )
			{
				currentTourneyPlayer[ i ] = -1;
			}
		}
	}
	// only checks for aborts -> game review below
	if( gameState != COUNTDOWN && gameState != GAMEON && gameState != SUDDENDEATH )
	{
		return;
	}
	switch( gameLocal.gameType )
	{
		case GAME_TOURNEY:
			for( i = 0; i < 2; i++ )
			{

				idPlayer* player = static_cast< idPlayer* >( gameLocal.entities[ currentTourneyPlayer[ i ] ] );

				if( !gameLocal.entities[ currentTourneyPlayer[ i ] ] || player->spectating )
				{
					NewState( GAMEREVIEW );
					return;
				}
			}
			break;
		default:
			if( !EnoughClientsToPlay() )
			{
				NewState( GAMEREVIEW );
			}
			break;
	}
}

/*
================
idMultiplayerGame::MapRestart
================
*/
void idMultiplayerGame::MapRestart()
{

	assert( !common->IsClient() );
	if( gameState != WARMUP )
	{
		NewState( WARMUP );
		nextState = INACTIVE;
		nextStateSwitch = 0;
	}

	teamPoints[0] = 0;
	teamPoints[1] = 0;

	BalanceTeams();
}

/*
================
idMultiplayerGame::BalanceTeams
================
*/
void idMultiplayerGame::BalanceTeams()
{
	if( !IsGametypeTeamBased() )
	{
		return;
	}
	int teamCount[ 2 ] = { 0, 0 };
	NumActualClients( false, teamCount );
	int outOfBalance = abs( teamCount[0] - teamCount[1] );
	if( outOfBalance <= 1 )
	{
		return;
	}

	// Teams are out of balance
	// Move N players from the large team to the small team
	int numPlayersToMove = outOfBalance / 2;
	int smallTeam = MinIndex( teamCount[0], teamCount[1] );
	int largeTeam = 1 - smallTeam;

	idLobbyBase& lobby = session->GetActingGameStateLobbyBase();

	// First move players from the large team that match a party token on the small team
	for( int a = 0; a < gameLocal.numClients; a++ )
	{
		idPlayer* playerA = gameLocal.GetClientByNum( a );
		if( playerA->team == largeTeam && CanPlay( playerA ) )
		{
			for( int b = 0; b < gameLocal.numClients; b++ )
			{
				if( a == b )
				{
					continue;
				}
				idPlayer* playerB = gameLocal.GetClientByNum( b );
				if( playerB->team == smallTeam && CanPlay( playerB ) )
				{
					if( lobby.GetLobbyUserPartyToken( gameLocal.lobbyUserIDs[ a ] ) == lobby.GetLobbyUserPartyToken( gameLocal.lobbyUserIDs[ b ] ) )
					{
						SwitchToTeam( a, largeTeam, smallTeam );
						numPlayersToMove--;
						if( numPlayersToMove == 0 )
						{
							return;
						}
						break;
					}
				}
			}
		}
	}

	// Then move players from the large team that DON'T match party tokens on the large team
	for( int a = 0; a < gameLocal.numClients; a++ )
	{
		idPlayer* playerA = gameLocal.GetClientByNum( a );
		if( playerA->team == largeTeam && CanPlay( playerA ) )
		{
			bool match = false;
			for( int b = 0; b < gameLocal.numClients; b++ )
			{
				if( a == b )
				{
					continue;
				}
				idPlayer* playerB = gameLocal.GetClientByNum( b );
				if( playerB->team == largeTeam && CanPlay( playerB ) )
				{
					if( lobby.GetLobbyUserPartyToken( gameLocal.lobbyUserIDs[ a ] ) == lobby.GetLobbyUserPartyToken( gameLocal.lobbyUserIDs[ b ] ) )
					{
						match = true;
						break;
					}
				}
			}
			if( !match )
			{
				SwitchToTeam( a, largeTeam, smallTeam );
				numPlayersToMove--;
				if( numPlayersToMove == 0 )
				{
					return;
				}
			}
		}
	}

	// Then move any players from the large team to the small team
	for( int a = 0; a < gameLocal.numClients; a++ )
	{
		idPlayer* playerA = gameLocal.GetClientByNum( a );
		if( playerA->team == largeTeam && CanPlay( playerA ) )
		{
			SwitchToTeam( a, largeTeam, smallTeam );
			numPlayersToMove--;
			if( numPlayersToMove == 0 )
			{
				return;
			}
		}
	}
}

/*
================
idMultiplayerGame::SwitchToTeam
================
*/
void idMultiplayerGame::SwitchToTeam( int clientNum, int oldteam, int newteam )
{
	idPlayer* p = static_cast<idPlayer*>( gameLocal.entities[ clientNum ] );
	p->team = newteam;
	session->GetActingGameStateLobbyBase().SetLobbyUserTeam( gameLocal.lobbyUserIDs[ clientNum ], newteam );
	session->SetVoiceGroupsToTeams();

	assert( IsGametypeTeamBased() ); /* CTF */
	assert( oldteam != newteam );
	assert( !common->IsClient() );

	if( !common->IsClient() && newteam >= 0 )
	{
		PrintMessageEvent( MSG_JOINTEAM, clientNum, newteam );
	}
	// assign the right teamFragCount
	int i;
	for( i = 0; i < gameLocal.numClients; i++ )
	{
		if( i == clientNum )
		{
			continue;
		}
		idEntity* ent = gameLocal.entities[ i ];
		if( ent && ent->IsType( idPlayer::Type ) && static_cast< idPlayer* >( ent )->team == newteam )
		{
			playerState[ clientNum ].teamFragCount = playerState[ i ].teamFragCount;
			break;
		}
	}
	if( i == gameLocal.numClients )
	{
		// alone on this team
		playerState[ clientNum ].teamFragCount = 0;
	}
	if( ( gameState == GAMEON || ( IsGametypeFlagBased() && gameState == SUDDENDEATH ) ) && oldteam != -1 )
	{
		// when changing teams during game, kill and respawn
		if( p->IsInTeleport() )
		{
			p->ServerSendEvent( idPlayer::EVENT_ABORT_TELEPORTER, NULL, false );
			p->SetPrivateCameraView( NULL );
		}
		p->Kill( true, true );
		if( IsGametypeFlagBased() )
		{
			p->DropFlag();
		}
		CheckAbortGame();
	}
	else if( IsGametypeFlagBased() && oldteam != -1 )
	{
		p->DropFlag();
	}
}

/*
================
idMultiplayerGame::ProcessChatMessage
================
*/
void idMultiplayerGame::ProcessChatMessage( int clientNum, bool team, const char* name, const char* text, const char* sound )
{
	idBitMsg	outMsg;
	byte		msgBuf[ 256 ];
	const char* prefix = NULL;
	int			send_to; // 0 - all, 1 - specs, 2 - team
	int			i;
	idEntity*	 ent;
	idPlayer*	pfrom;
	idStr		prefixed_name;

	assert( !common->IsClient() );

	if( clientNum >= 0 )
	{
		pfrom = static_cast< idPlayer* >( gameLocal.entities[ clientNum ] );
		if( !( pfrom && pfrom->IsType( idPlayer::Type ) ) )
		{
			return;
		}

		if( pfrom->spectating )
		{
			prefix = "spectating";
			if( team || ( !g_spectatorChat.GetBool() && ( gameState == GAMEON || gameState == SUDDENDEATH ) ) )
			{
				// to specs
				send_to = 1;
			}
			else
			{
				// to all
				send_to = 0;
			}
		}
		else if( team )
		{
			prefix = "team";
			// to team
			send_to = 2;
		}
		else
		{
			// to all
			send_to = 0;
		}
	}
	else
	{
		pfrom = NULL;
		send_to = 0;
	}
	// put the message together
	outMsg.InitWrite( msgBuf, sizeof( msgBuf ) );
	if( prefix )
	{
		prefixed_name = va( "(%s) %s", prefix, name );
	}
	else
	{
		prefixed_name = name;
	}
	outMsg.WriteString( prefixed_name );
	outMsg.WriteString( text, -1, false );
	if( !send_to )
	{
		AddChatLine( "%s^0: %s\n", prefixed_name.c_str(), text );
		session->GetActingGameStateLobbyBase().SendReliable( GAME_RELIABLE_MESSAGE_CHAT, outMsg, false );
		if( sound )
		{
			PlayGlobalSound( -1, SND_COUNT, sound );
		}
	}
	else
	{
		for( i = 0; i < gameLocal.numClients; i++ )
		{
			ent = gameLocal.entities[ i ];
			if( !ent || !ent->IsType( idPlayer::Type ) )
			{
				continue;
			}
			idPlayer* pent = static_cast< idPlayer* >( ent );
			if( send_to == 1 && pent->spectating )
			{
				if( sound )
				{
					PlayGlobalSound( i, SND_COUNT, sound );
				}
				if( i == gameLocal.GetLocalClientNum() )
				{
					AddChatLine( "%s^0: %s\n", prefixed_name.c_str(), text );
				}
				else
				{
					session->GetActingGameStateLobbyBase().SendReliableToLobbyUser( gameLocal.lobbyUserIDs[i], GAME_RELIABLE_MESSAGE_CHAT, outMsg );
				}
			}
			else if( send_to == 2 && pent->team == pfrom->team )
			{
				if( sound )
				{
					PlayGlobalSound( i, SND_COUNT, sound );
				}
				if( i == gameLocal.GetLocalClientNum() )
				{
					AddChatLine( "%s^0: %s\n", prefixed_name.c_str(), text );
				}
				else
				{
					session->GetActingGameStateLobbyBase().SendReliableToLobbyUser( gameLocal.lobbyUserIDs[i], GAME_RELIABLE_MESSAGE_CHAT, outMsg );
				}
			}
		}
	}
}

/*
================
idMultiplayerGame::Precache
================
*/
void idMultiplayerGame::Precache()
{
	if( !common->IsMultiplayer() )
	{
		return;
	}

	// player
	declManager->FindType( DECL_ENTITYDEF, gameLocal.GetMPPlayerDefName(), false );

	// skins
	for( int i = 0; i < numSkins; i++ )
	{
		idStr baseSkinName = skinNames[ i ];
		declManager->FindSkin( baseSkinName, false );
		declManager->FindSkin( baseSkinName + "_berserk", false );
		declManager->FindSkin( baseSkinName + "_invuln", false );
	}

	// MP game sounds
	for( int i = 0; i < SND_COUNT; i++ )
	{
		declManager->FindSound( GlobalSoundStrings[ i ] );
	}
}

/*
================
idMultiplayerGame::ToggleSpectate
================
*/
void idMultiplayerGame::ToggleSpectate()
{
	assert( common->IsClient() || gameLocal.GetLocalClientNum() == 0 );

	idPlayer* player = ( idPlayer* )gameLocal.entities[gameLocal.GetLocalClientNum()];
	bool spectating = player->spectating;
	// only allow toggling to spectate if spectators are enabled.
	if( !spectating && !gameLocal.serverInfo.GetBool( "si_spectators" ) )
	{
		gameLocal.mpGame.AddChatLine( idLocalization::GetString( "#str_06747" ) );
		return;
	}

	byte msgBuf[ 256 ];
	idBitMsg outMsg;
	outMsg.InitWrite( msgBuf, sizeof( msgBuf ) );
	outMsg.WriteBool( !spectating );
	outMsg.WriteByteAlign();
	session->GetActingGameStateLobbyBase().SendReliableToHost( GAME_RELIABLE_MESSAGE_SPECTATE, outMsg );
}

/*
================
idMultiplayerGame::CanPlay
================
*/
bool idMultiplayerGame::CanPlay( idPlayer* p )
{
	return !p->wantSpectate;
}

/*
================
idMultiplayerGame::WantRespawn
================
*/
bool idMultiplayerGame::WantRespawn( idPlayer* p )
{
	return p->forceRespawn && !p->wantSpectate;
}

/*
================
idMultiplayerGame::VoiceChat
================
*/
void idMultiplayerGame::VoiceChat_f( const idCmdArgs& args )
{
	gameLocal.mpGame.VoiceChat( args, false );
}

/*
================
idMultiplayerGame::VoiceChatTeam
================
*/
void idMultiplayerGame::VoiceChatTeam_f( const idCmdArgs& args )
{
	gameLocal.mpGame.VoiceChat( args, true );
}

/*
================
idMultiplayerGame::VoiceChat
================
*/
void idMultiplayerGame::VoiceChat( const idCmdArgs& args, bool team )
{
	idBitMsg			outMsg;
	byte				msgBuf[128];
	const char*			voc;
	const idDict*		spawnArgs;
	const idKeyValue*	keyval;
	int					index;

	if( !common->IsMultiplayer() )
	{
		common->Printf( "clientVoiceChat: only valid in multiplayer\n" );
		return;
	}
	if( args.Argc() != 2 )
	{
		common->Printf( "clientVoiceChat: bad args\n" );
		return;
	}
	// throttle
	if( gameLocal.realClientTime < voiceChatThrottle )
	{
		return;
	}
	if( gameLocal.GetLocalPlayer() == NULL )
	{
		return;
	}

	voc = args.Argv( 1 );

	spawnArgs = &gameLocal.GetLocalPlayer()->spawnArgs;

	keyval = spawnArgs->MatchPrefix( "snd_voc_", NULL );
	index = 0;
	while( keyval )
	{
		if( !keyval->GetValue().Icmp( voc ) )
		{
			break;
		}
		keyval = spawnArgs->MatchPrefix( "snd_voc_", keyval );
		index++;
	}
	if( !keyval )
	{
		common->Printf( "Voice command not found: %s\n", voc );
		return;
	}
	voiceChatThrottle = gameLocal.realClientTime + 1000;

	outMsg.InitWrite( msgBuf, sizeof( msgBuf ) );
	outMsg.WriteLong( index );
	outMsg.WriteBits( team ? 1 : 0, 1 );
	session->GetActingGameStateLobbyBase().SendReliableToHost( GAME_RELIABLE_MESSAGE_VCHAT, outMsg );
}

/*
================
idMultiplayerGame::ProcessVoiceChat
================
*/
void idMultiplayerGame::ProcessVoiceChat( int clientNum, bool team, int index )
{
	const idDict*		spawnArgs;
	const idKeyValue*	keyval;
	idStr				name;
	idStr				snd_key;
	idStr				text_key;
	idPlayer*			p;

	p = static_cast< idPlayer* >( gameLocal.entities[ clientNum ] );
	if( !( p && p->IsType( idPlayer::Type ) ) )
	{
		return;
	}

	if( p->spectating )
	{
		return;
	}

	// lookup the sound def
	spawnArgs = &p->spawnArgs;

	keyval = spawnArgs->MatchPrefix( "snd_voc_", NULL );
	while( index > 0 && keyval )
	{
		keyval = spawnArgs->MatchPrefix( "snd_voc_", keyval );
		index--;
	}
	if( !keyval )
	{
		common->DPrintf( "ProcessVoiceChat: unknown chat index %d\n", index );
		return;
	}
	snd_key = keyval->GetKey();
	name = session->GetActingGameStateLobbyBase().GetLobbyUserName( gameLocal.lobbyUserIDs[ clientNum ] );
	sprintf( text_key, "txt_%s", snd_key.Right( snd_key.Length() - 4 ).c_str() );
	if( team || gameState == COUNTDOWN || gameState == GAMEREVIEW )
	{
		ProcessChatMessage( clientNum, team, name, spawnArgs->GetString( text_key ), spawnArgs->GetString( snd_key ) );
	}
	else
	{
		p->StartSound( snd_key, SND_CHANNEL_ANY, 0, true, NULL );
		ProcessChatMessage( clientNum, team, name, spawnArgs->GetString( text_key ), NULL );
	}
}

/*
================
idMultiplayerGame::ServerWriteInitialReliableMessages
================
*/
void idMultiplayerGame::ServerWriteInitialReliableMessages( int clientNum, lobbyUserID_t lobbyUserID )
{
	idBitMsg	outMsg;
	byte		msgBuf[ MAX_GAME_MESSAGE_SIZE ];

	outMsg.InitWrite( msgBuf, sizeof( msgBuf ) );
	outMsg.BeginWriting();
	// send the game state and start time
	outMsg.WriteByte( gameState );
	outMsg.WriteLong( matchStartedTime );
	outMsg.WriteShort( startFragLimit );
	// send the powerup states and the spectate states
	for( int i = 0; i < gameLocal.numClients; i++ )
	{
		idEntity* ent = gameLocal.entities[ i ];
		if( i != clientNum && ent && ent->IsType( idPlayer::Type ) )
		{
			outMsg.WriteByte( i );
			outMsg.WriteBits( static_cast< idPlayer* >( ent )->inventory.powerups, 15 );
			outMsg.WriteBits( static_cast< idPlayer* >( ent )->spectating, 1 );
		}
	}
	outMsg.WriteByte( MAX_CLIENTS );
	session->GetActingGameStateLobbyBase().SendReliableToLobbyUser( lobbyUserID, GAME_RELIABLE_MESSAGE_STARTSTATE, outMsg );

	// warmup time
	if( gameState == COUNTDOWN )
	{
		outMsg.BeginWriting();
		outMsg.WriteByte( GAME_RELIABLE_MESSAGE_WARMUPTIME );
		outMsg.WriteLong( warmupEndTime );
		session->GetActingGameStateLobbyBase().SendReliableToLobbyUser( lobbyUserID, GAME_RELIABLE_MESSAGE_WARMUPTIME, outMsg );
	}
}

/*
================
idMultiplayerGame::ClientReadStartState
================
*/
void idMultiplayerGame::ClientReadStartState( const idBitMsg& msg )
{
	// read the state in preparation for reading snapshot updates
	gameState = ( idMultiplayerGame::gameState_t )msg.ReadByte();
	matchStartedTime = msg.ReadLong();
	startFragLimit = msg.ReadShort();

	int client;
	while( ( client = msg.ReadByte() ) != MAX_CLIENTS )
	{

		// Do not process players that are not here.
		if( gameLocal.entities[ client ] == NULL )
		{
			continue;
		}

		assert( gameLocal.entities[ client ] && gameLocal.entities[ client ]->IsType( idPlayer::Type ) );
		int powerup = msg.ReadBits( 15 );
		for( int i = 0; i < MAX_POWERUPS; i++ )
		{
			if( powerup & ( 1 << i ) )
			{
				static_cast< idPlayer* >( gameLocal.entities[ client ] )->GivePowerUp( i, 0, ITEM_GIVE_FEEDBACK | ITEM_GIVE_UPDATE_STATE );
			}
		}
		bool spectate = ( msg.ReadBits( 1 ) != 0 );
		static_cast< idPlayer* >( gameLocal.entities[ client ] )->Spectate( spectate );
	}
}

/*
================
idMultiplayerGame::ClientReadWarmupTime
================
*/
void idMultiplayerGame::ClientReadWarmupTime( const idBitMsg& msg )
{
	warmupEndTime = msg.ReadLong();
}

/*
================
idMultiplayerGame::ClientReadWarmupTime
================
*/
void idMultiplayerGame::ClientReadMatchStartedTime( const idBitMsg& msg )
{
	matchStartedTime = msg.ReadLong();
}


/*
================
idMultiplayerGame::ClientReadAchievementUnlock
================
*/
void idMultiplayerGame::ClientReadAchievementUnlock( const idBitMsg& msg )
{

	int playerid = msg.ReadByte();
	achievement_t achieve = ( achievement_t )msg.ReadByte();

	idPlayer* player = static_cast<idPlayer*>( gameLocal.entities[ playerid ] );

	if( player != NULL )
	{

		idLib::Printf( "Client Receiving Achievement\n" );
		player->GetAchievementManager().EventCompletesAchievement( achieve );
	}

}

/*
================
idMultiplayerGame::IsGametypeTeamBased
================
*/
bool idMultiplayerGame::IsGametypeTeamBased() /* CTF */
{
	switch( gameLocal.gameType )
	{
		case GAME_SP:
		case GAME_DM:
		case GAME_TOURNEY:
		case GAME_LASTMAN:
			return false;
		case GAME_CTF:
		case GAME_TDM:
			return true;

		default:
			assert( !"Add support for your new gametype here." );
	}

	return false;
}

/*
================
idMultiplayerGame::IsGametypeFlagBased
================
*/
bool idMultiplayerGame::IsGametypeFlagBased()
{
	switch( gameLocal.gameType )
	{
		case GAME_SP:
		case GAME_DM:
		case GAME_TOURNEY:
		case GAME_LASTMAN:
		case GAME_TDM:
			return false;
		case GAME_CTF:
			return true;
		default:
			assert( !"Add support for your new gametype here." );
	}

	return false;

}

/*
================
idMultiplayerGame::GetTeamFlag
================
*/
idItemTeam* idMultiplayerGame::GetTeamFlag( int team )
{
	assert( team == 0 || team == 1 );

	if( !IsGametypeFlagBased() || ( team != 0 && team != 1 ) )  /* CTF */
	{
		return NULL;
	}

	// TODO : just call on map start
	FindTeamFlags();

	return teamFlags[team];
}

/*
================
idMultiplayerGame::GetTeamFlag
================
*/
void idMultiplayerGame::FindTeamFlags()
{
	const char* flagDefs[2] =
	{
		"team_CTF_redflag",
		"team_CTF_blueflag"
	};

	for( int i = 0; i < 2; i++ )
	{
		idEntity* entity = gameLocal.FindEntityUsingDef( NULL, flagDefs[i] );
		do
		{
			if( entity == NULL )
			{
				return;
			}

			idItemTeam* flag = static_cast<idItemTeam*>( entity );

			if( flag->team == i )
			{
				teamFlags[i] = flag;
				break;
			}

			entity = gameLocal.FindEntityUsingDef( entity, flagDefs[i] );
		}
		while( entity );
	}
}

/*
================
idMultiplayerGame::GetFlagStatus
================
*/
flagStatus_t idMultiplayerGame::GetFlagStatus( int team )
{
	//assert( IsGametypeFlagBased() );

	idItemTeam* teamFlag = GetTeamFlag( team );
	//assert( teamFlag != NULL );

	if( teamFlag != NULL )
	{
		if( teamFlag->carried == false && teamFlag->dropped == false )
		{
			return FLAGSTATUS_INBASE;
		}

		if( teamFlag->carried == true )
		{
			return FLAGSTATUS_TAKEN;
		}

		if( teamFlag->carried == false && teamFlag->dropped == true )
		{
			return FLAGSTATUS_STRAY;
		}
	}

	//assert( !"Invalid flag state." );
	return FLAGSTATUS_NONE;
}

/*
================
idMultiplayerGame::SetFlagMsgs
================
*/
void idMultiplayerGame::SetFlagMsg( bool b )
{
	flagMsgOn = b;
}

/*
================
idMultiplayerGame::IsFlagMsgOn
================
*/
bool idMultiplayerGame::IsFlagMsgOn()
{
	return ( GetGameState() == WARMUP || GetGameState() == GAMEON || GetGameState() == SUDDENDEATH ) && flagMsgOn;
}

/*
================
idMultiplayerGame::ReloadScoreboard
================
*/
void idMultiplayerGame::ReloadScoreboard()
{

	if( scoreboardManager != NULL )
	{
		scoreboardManager->Initialize( "scoreboard", common->SW() );
	}

	Precache();
}

/*
================
idMultiplayerGame::GetGameModes
================
*/
int idMultiplayerGame::GetGameModes( const char** * gameModes, const char** * gameModesDisplay )
{

	bool defaultValue = true;

	if( session->GetTitleStorageBool( "CTF_Enabled", defaultValue ) )
	{
		*gameModes = gameTypeNames_WithCTF;
		*gameModesDisplay = gameTypeDisplayNames_WithCTF;

		return GAME_COUNT;
	}
	else
	{

		*gameModes = gameTypeNames_WithoutCTF;
		*gameModesDisplay = gameTypeDisplayNames_WithoutCTF;

		return GAME_COUNT - 1;
	}
}
