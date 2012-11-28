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

#ifndef __LEADERBOARDS_LOCAL_H__
#define __LEADERBOARDS_LOCAL_H__


struct leaderboardStats_t
{
	int		frags;
	int		wins;
	int		teamfrags;
	int		deaths;
};

struct columnGameMode_t
{

	columnDef_t* 	columnDef;					// The Column definition for the game mode.
	int				numColumns;
	rankOrder_t		rankOrder;					// rank ordering of the game mode. (  RANK_GREATEST_FIRST, RANK_LEAST_FIRST )
	bool			supportsAttachments;
	bool			checkAgainstCurrent;
	const char* 	abrevName;					// Leaderboard Game Mode Abrev.
};

/*
================================================================================================

	Leaderboards

================================================================================================
*/



// creates and stores all the leaderboards inside the internal map ( see Sys_FindLeaderboardDef on retrieving definition )
void			LeaderboardLocal_Init();

// Destroys any leaderboard definitions allocated by LeaderboardLocal_Init()
void			LeaderboardLocal_Shutdown();

// Gets a leaderboard ID with map index and game type.
int				LeaderboardLocal_GetID( int mapIndex, int gametype );

// Do it all function. Will create the column_t with the correct stats from the game type, and upload it to the leaderboard system.
void			LeaderboardLocal_Upload( lobbyUserID_t lobbyUserID, int gameType, leaderboardStats_t& stats );

extern const columnGameMode_t gameMode_columnDefs[];

#endif // __LEADERBOARDS_LOCAL_H__
