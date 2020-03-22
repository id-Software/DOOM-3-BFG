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
#ifndef __SYS_STATS_MISC_H__
#define __SYS_STATS_MISC_H__

/*
================================================================================================

	Stats (for achievements, matchmaking, etc.)

================================================================================================
*/

/*
================================================
systemStats_t
This is to give the framework the ability to deal with stat indexes that are
completely up to each game.  The system needs to deal with some stat indexes for things like
the level for matchmaking, etc.
================================================
*/

/*
================================================================================================

	Leader Boards

================================================================================================
*/

const int MAX_LEADERBOARDS			= 256;
const int MAX_LEADERBOARD_COLUMNS	= 16;

enum aggregationMethod_t
{
	AGGREGATE_MIN,  // Write the new value if it is less than the existing value.
	AGGREGATE_MAX,  // Write the new value if it is greater than the existing value.
	AGGREGATE_SUM,  // Add the new value to the existing value and write the result.
	AGGREGATE_LAST, // Write the new value.
};

enum rankOrder_t
{
	RANK_GREATEST_FIRST, // Rank the in descending order, greatest score is best score
	RANK_LEAST_FIRST,	 // Rank the in ascending order, lowest score is best score
};

enum statsColumnDisplayType_t
{
	STATS_COLUMN_DISPLAY_NUMBER,
	STATS_COLUMN_DISPLAY_TIME_MILLISECONDS,
	STATS_COLUMN_DISPLAY_CASH,
	STATS_COLUMN_NEVER_DISPLAY,
};

struct columnDef_t
{
	const char* 				locDisplayName;
	int							bits;
	aggregationMethod_t			aggregationMethod;
	statsColumnDisplayType_t	displayType;
};

struct leaderboardDefinition_t
{

	leaderboardDefinition_t() :
		id( -1 ),
		numColumns( 0 ),
		columnDefs( NULL ),
		rankOrder( RANK_GREATEST_FIRST ),
		supportsAttachments( false ),
		checkAgainstCurrent( false )
	{
	}

	leaderboardDefinition_t( int id_, int numColumns_, const columnDef_t* columnDefs_, rankOrder_t rankOrder_, bool supportsAttachments_, bool checkAgainstCurrent_ ) :
		id( id_ ),
		numColumns( numColumns_ ),
		columnDefs( columnDefs_ ),
		rankOrder( rankOrder_ ),
		supportsAttachments( supportsAttachments_ ),
		checkAgainstCurrent( checkAgainstCurrent_ )
	{
	}

	int32				id;
	int32				numColumns;
	const columnDef_t* 	columnDefs;
	rankOrder_t			rankOrder;
	bool				supportsAttachments;
	bool				checkAgainstCurrent;		// Compare column 0 with the currently stored leaderboard, and only submit the new leaderboard if the new column 0 is better
	idStr				boardName;					// Only used for display name within steam. If Empty, will generate. must be specifically set.
};

struct column_t
{
	column_t( int64 value_ ) : value( value_ ) {}
	column_t() {}

	int64				value;
};


/*
================================================================================================
Contains the Achievement and LeaderBoard free function declarations.
================================================================================================
*/

typedef int32			leaderboardHandle_t;

/*
================================================
idLeaderBoardEntry
================================================
*/
class idLeaderBoardEntry
{
public:
	static const int MAX_LEADERBOARD_COLUMNS = 16;
	idStr username; // aka gamertag
	int64 score;
	int64 columns[ MAX_LEADERBOARD_COLUMNS ];
};

const leaderboardDefinition_t* Sys_FindLeaderboardDef( int id );
leaderboardDefinition_t* 		Sys_CreateLeaderboardDef( int id_, int numColumns_, const columnDef_t* columnDefs_, rankOrder_t rankOrder_, bool supportsAttachments_, bool checkAgainstCurrent_ );
void							Sys_DestroyLeaderboardDefs();

#endif // !__SYS_STATS_MISC_H__
