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
#include "Leaderboards.h"
#include "MultiplayerGame.h"

/*
================================================================================================

	Leaderboard stats column definitions  - per Game Type.

================================================================================================
*/


static columnDef_t public_Deathmatch[] =
{
	{ "Frags", 64, AGGREGATE_MAX, STATS_COLUMN_DISPLAY_NUMBER },
	//{ "Deaths", 64, AGGREGATE_MAX, STATS_COLUMN_DISPLAY_NUMBER },
	//{ "Wins", 64, AGGREGATE_MAX, STATS_COLUMN_DISPLAY_NUMBER },
	//{ "Score", 64, AGGREGATE_MAX, STATS_COLUMN_DISPLAY_NUMBER },
};

static columnDef_t public_Tourney[] =
{
	{ "Wins", 64, AGGREGATE_SUM, STATS_COLUMN_DISPLAY_NUMBER },
};

static columnDef_t public_TeamDeathmatch[] =
{
	{ "Wins", 64, AGGREGATE_MAX, STATS_COLUMN_DISPLAY_NUMBER },
};

static columnDef_t public_LastmanStanding[] =
{
	{ "Wins", 64, AGGREGATE_MAX, STATS_COLUMN_DISPLAY_NUMBER },
};

static columnDef_t public_CaptureTheFlag[] =
{
	{ "Wins", 64, AGGREGATE_MAX, STATS_COLUMN_DISPLAY_NUMBER },
};

// This should match up to the ordering of the gameType_t. ( in MultiplayerGame.h )
const columnGameMode_t gameMode_columnDefs[] =
{
	{ public_Deathmatch,		ARRAY_COUNT( public_Deathmatch ),		RANK_GREATEST_FIRST, false, false, "DM" },			// DEATHMATCH
	{ public_Tourney,			ARRAY_COUNT( public_Tourney ),			RANK_GREATEST_FIRST, false, false, "TOURNEY" },		// TOURNEY
	{ public_TeamDeathmatch,	ARRAY_COUNT( public_TeamDeathmatch ),	RANK_GREATEST_FIRST, false, false, "TDM" },			// TEAM DEATHMATCH
	{ public_LastmanStanding,	ARRAY_COUNT( public_LastmanStanding ),	RANK_GREATEST_FIRST, false, false, "LMS" },			// LASTMAN STANDING
	{ public_CaptureTheFlag,	ARRAY_COUNT( public_CaptureTheFlag ),	RANK_GREATEST_FIRST, false, false, "CTF" },			// CAPTURE THE FLAG
};

/*
=====================================
RetreiveLeaderboardID

Each map will move in blocks of n*modes.
ex. map 0 will have 0 - 4 Leaderboard id's blocked out.
	map 1 will have 5 - 10 leaderboard id's blocked out.

	if gamemode is added it will move in blocks of ARRAY_COUNT( modes )

=====================================
*/
int LeaderboardLocal_GetID( int mapIndex, int gametype )
{
	assert( gametype > GAME_RANDOM );

	return mapIndex * ARRAY_COUNT( gameMode_columnDefs ) + gametype;
}

/*
=====================================
LeaderboardLocal_Init
=====================================
*/
void LeaderboardLocal_Init()
{

	const idList< mpMap_t > maps = common->GetMapList();

	const char** gameModes = NULL;
	const char** gameModesDisplay = NULL;
	int numModes = game->GetMPGameModes( &gameModes, &gameModesDisplay );

	// Iterate through all the available maps, and generate leaderboard Defs, and IDs for each.
	for( int mapIdx = 0; mapIdx < maps.Num(); mapIdx++ )
	{

		for( int modeIdx = 0; modeIdx < numModes; modeIdx++ )
		{

			// Check the supported modes on the map.
			if( maps[ mapIdx ].supportedModes & BIT( modeIdx ) )
			{

				const columnGameMode_t gamemode = gameMode_columnDefs[ modeIdx ];

				// Generate a Leaderboard ID for the map/mode
				int boardID = LeaderboardLocal_GetID( mapIdx, modeIdx );


				// Create and Register the leaderboard with the sys_stats registered Leaderboards
				leaderboardDefinition_t* newLeaderboardDef = Sys_CreateLeaderboardDef( boardID,
						gamemode.numColumns,
						gamemode.columnDef,
						gamemode.rankOrder,
						gamemode.supportsAttachments,
						gamemode.checkAgainstCurrent );



				// Set the leaderboard name.
				const char* mapname = idLocalization::GetString( maps[ mapIdx ].mapName );
				newLeaderboardDef->boardName.Format( "%s %s", mapname, gamemode.abrevName );

				// sanity check.
				if( Sys_FindLeaderboardDef( boardID ) != newLeaderboardDef )
				{
					idLib::Error( "Leaderboards_Init leaderboard creation failed" );
				}

			}
		}
	}

}

/*
=====================================
LeaderboardLocal_Shutdown
=====================================
*/
void LeaderboardLocal_Shutdown()
{

	Sys_DestroyLeaderboardDefs();
}

/*
=====================================
LeaderboardLocal_Upload
=====================================
*/

const static int FRAG_MULTIPLIER  = 100;
const static int DEATH_MULTIPLIER = -50;
const static int WINS_MULTIPLIER  = 20;

void LeaderboardLocal_Upload( lobbyUserID_t lobbyUserID, int gameType, leaderboardStats_t& stats )
{
	assert( gameType > GAME_RANDOM );

	int mapIdx = 0;

	// Need to figure out  What stat columns we want to base rank on. ( for now we'll use wins )
	const column_t* gameTypeColumn = NULL;
	const column_t defaultStats[] = { stats.wins };

	// calculate DM score.
	int dmScore = stats.frags * FRAG_MULTIPLIER + stats.deaths * DEATH_MULTIPLIER + stats.wins * WINS_MULTIPLIER ;
	// TODO: Once leaderboard menu has correct columns- enable this -> const column_t dmStats[] = { stats.frags, stats.deaths, stats.wins, dmScore };
	const column_t dmStats[] = { dmScore };

	// Calculate TDM score.
	int tdmScore = stats.frags * FRAG_MULTIPLIER + stats.teamfrags * FRAG_MULTIPLIER + stats.deaths * DEATH_MULTIPLIER + stats.wins * WINS_MULTIPLIER ;
	const column_t tdmStats[] = { tdmScore };

	// Calculate Tourney score.
	int tourneyScore = stats.wins;
	const column_t tnyStats[] = { tourneyScore };

	// Calculate LMS score.
	int lmsFrags = stats.frags;
	if( lmsFrags < 0 )
	{
		lmsFrags = 0;  // LMS NO LIVES LEFT = -20 on fragCount.
	}

	int lmsScore = lmsFrags * FRAG_MULTIPLIER + stats.wins * ( WINS_MULTIPLIER * 10 ) ;
	const column_t lmsStats[] = { lmsScore };

	// Calculate CTF score.
	int ctfScore = stats.frags * FRAG_MULTIPLIER + stats.deaths * DEATH_MULTIPLIER + stats.wins * ( WINS_MULTIPLIER * 10 );
	const column_t ctfStats[] = { ctfScore };

	switch( gameType )
	{
		case GAME_DM:
			gameTypeColumn = dmStats;
			break;
		case GAME_TDM:
			gameTypeColumn = tdmStats;
			break;
		case GAME_TOURNEY:
			gameTypeColumn = tnyStats;
			break;
		case GAME_LASTMAN:
			gameTypeColumn = lmsStats;
			break;
		case GAME_CTF:
		{
			gameTypeColumn = ctfStats;
			break;
		}
		default:
			gameTypeColumn = defaultStats;
	}

	const idMatchParameters& matchParameters = session->GetActingGameStateLobbyBase().GetMatchParms();
	const idList< mpMap_t > maps = common->GetMapList();

	// need to find the map Index number
	for( mapIdx = 0; mapIdx < maps.Num(); mapIdx++ )
	{
		if( matchParameters.mapName.Cmp( maps[ mapIdx ].mapFile ) == 0 )
		{
			break;
		}
	}

	int boardID = LeaderboardLocal_GetID( mapIdx, gameType );
	const leaderboardDefinition_t* board =  Sys_FindLeaderboardDef( boardID );

	if( board )
	{
		session->LeaderboardUpload( lobbyUserID, board, gameTypeColumn );
	}
	else
	{
		idLib::Warning( "LeaderboardLocal_Upload invalid leaderboard with id of %d", boardID );
	}
}

class idLeaderboardCallbackTest : public idLeaderboardCallback
{
	void Call()
	{
		idLib::Printf( "Leaderboard information retrieved in user callback.\n" );
		idLib::Printf( "%d total entries in leaderboard %d.\n", numRowsInLeaderboard, def->id );
		for( int i = 0; i < rows.Num(); i++ )
		{
			idLib::Printf( "%d: %s rank:%lld", i, rows[i].name.c_str(), rows[i].rank );
			for( int j = 0; j < def->numColumns; j++ )
			{
				idLib::Printf( ", score[%d]: %lld", j, rows[i].columns[j] );
			}
			idLib::Printf( "\n" );
		}
	}
	idLeaderboardCallback* Clone() const
	{
		return new( TAG_PSN ) idLeaderboardCallbackTest( *this );
	}
};

CONSOLE_COMMAND( testLeaderboardDownload, "<id 0 - n > <start = 1> <end = 100>", 0 )
{
	idLeaderboardCallbackTest leaderboardCallbackTest;

	int leaderboardID = 0;
	int start = 1;
	int end = 100;

	if( args.Argc() > 1 )
	{
		leaderboardID = atoi( args.Argv( 1 ) );
	}

	if( args.Argc() > 2 )
	{
		start = atoi( args.Argv( 2 ) );
	}

	if( args.Argc() > 3 )
	{
		end = atoi( args.Argv( 3 ) );
	}

	const leaderboardDefinition_t* leaderboardDef = Sys_FindLeaderboardDef( leaderboardID );

	if( leaderboardDef )
	{
		session->LeaderboardDownload( 0, leaderboardDef, start, end, leaderboardCallbackTest );
	}
	else
	{
		idLib::Warning( "Sys_FindLeaderboardDef() Unable to find board %d\n", leaderboardID );
	}

}

CONSOLE_COMMAND( testLeaderboardUpload, "<gameType 0 - 4 > <frags = 0> <wins = 1>", 0 )
{

	idLobbyBase& lobby = session->GetActingGameStateLobbyBase();
	lobbyUserID_t user = lobby.GetLobbyUserIdByOrdinal( 0 );

	gameType_t gameType = GAME_DM;
	int frags = 0;
	int wins = 1;

	if( args.Argc() > 1 )
	{
		gameType = static_cast<gameType_t>( atoi( args.Argv( 1 ) ) );
	}

	if( args.Argc() > 2 )
	{
		frags = atoi( args.Argv( 2 ) );
	}

	if( args.Argc() > 3 )
	{
		wins = atoi( args.Argv( 3 ) );
	}

	leaderboardStats_t stats = { frags, wins, 0, 0 };

	LeaderboardLocal_Upload( user, gameType , stats );

	session->LeaderboardFlush();

}



CONSOLE_COMMAND( testLeaderboardUpload_SendToClients, "<gameType 0 - 4 > <frags = 0> <wins = 1>", 0 )
{

	for( int playerIdx = 0; playerIdx < gameLocal.numClients; playerIdx++ )
	{

		leaderboardStats_t stats = { 1, 1, 1, 1 };

		LeaderboardLocal_Upload( gameLocal.lobbyUserIDs[ playerIdx ], gameLocal.gameType, stats );
	}

	// Dont do this more than once.
	session->LeaderboardFlush();
}

