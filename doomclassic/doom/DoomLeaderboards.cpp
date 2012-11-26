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

#include "DoomLeaderboards.h"
#include "tech5/engine/framework/precompiled.h"
#include "tech5\engine\sys\sys_stats.h"
#include "../doomengine/source/doomdef.h"

#include <map>

static columnDef_t columnDefTime[] = { 
	{ "Time", 64, AGGREGATE_MIN, STATS_COLUMN_DISPLAY_TIME_MILLISECONDS }
};

const static int NUMLEVELS_ULTIMATE_DOOM		= 36;						
const static int NUMLEVELS_DOOM2_HELL_ON_EARTH	= 32;
const static int NUMLEVELS_FINALDOOM_TNT		= 32;
const static int NUMLEVELS_FINALDOOM_PLUTONIA	= 32;
const static int NUMLEVELS_DOOM2_MASTER_LEVELS	= 21;
const static int NUMLEVELS_DOOM2_NERVE			= 9;


const static int NUM_LEVEL_LIST[] = {
	NUMLEVELS_ULTIMATE_DOOM,
	NUMLEVELS_DOOM2_HELL_ON_EARTH,
	NUMLEVELS_FINALDOOM_TNT,
	NUMLEVELS_FINALDOOM_PLUTONIA,
	NUMLEVELS_DOOM2_MASTER_LEVELS,
	NUMLEVELS_DOOM2_NERVE
};

/*
========================
GenerateLeaderboard ID
Generates a Leaderboard ID based on current Expansion + episode + map + skill + Type 

Expansion is the base value that the leaderboard ID comes from

========================
*/
const int GenerateLeaderboardID( int expansion, int episode, int map, int skill ) {
	
	int realMapNumber = ( episode * map - 1 );

	if( common->GetGameSKU() == GAME_SKU_DOOM1_BFG  ) {

		// Doom levels start at 620 .. yeah.. hack.
		int block = 615;
		int mapAndSkill = ( realMapNumber * ( (int)sk_nightmare + 1 ) )  + skill ;

		return block + mapAndSkill;
	} else if( common->GetGameSKU() == GAME_SKU_DOOM2_BFG ) {

		if( expansion == 1 ) {
			// Doom 2 Levels start at 800.. Yep.. another hack.
			int block = 795;
			int mapAndSkill = ( realMapNumber * ( (int)sk_nightmare + 1 ) )  + skill ;

			return block + mapAndSkill;
		} else {
			// Nerve Levels start at 960... another hack!
			int block = 955;
			int mapAndSkill = ( realMapNumber * ( (int)sk_nightmare + 1 ) )  + skill ;

			return block + mapAndSkill;
		}

	} else {

		// DCC Content
		int block = 0;
		if( expansion  > 0 ){
			for( int expi = 0; expi < expansion; expi++ ) {
				block += NUM_LEVEL_LIST[ expi ] * ( (int)sk_nightmare + 1);
			}
		}
		int mapAndSkill = ( realMapNumber * ( (int)sk_nightmare + 1 ) )  + skill ;
		return block + mapAndSkill;
	}
}

/*
========================
InitLeaderboards

Generates all the possible leaderboard definitions
and stores into an STL Map, with leaderboard ID as the Hash/key value.

========================
*/
void InitLeaderboards() {

	for( int expi = 0; expi < ARRAY_COUNT( NUM_LEVEL_LIST ); expi++ ) {
	
		for( int udi = 1; udi <= NUM_LEVEL_LIST[expi] ; udi++ ) {

			for( int skilli = 0; skilli <= sk_nightmare; skilli++ ) {

				// Create the Time Trial leaderboard for each level.
				int timeTrial_leaderboardID = GenerateLeaderboardID( expi, 1, udi, skilli ); 
				leaderboardDefinition_t * timeTrial_Leaderboard = new leaderboardDefinition_t( timeTrial_leaderboardID,	ARRAY_COUNT( columnDefTime ), columnDefTime, RANK_LEAST_FIRST,	false, true );

			}
		}	
	}
}

/*
========================
GenerateLeaderboard
Generates a Leaderboard based on current Expansion + episode + map + skill + Type 

Expansion is the base value that the leaderboard ID comes from

========================
*/
const leaderboardDefinition_t * GetLeaderboard( int expansion, int episode, int map, int skill ) {

	int leaderboardID = GenerateLeaderboardID( expansion, episode, map, skill );

	return Sys_FindLeaderboardDef( leaderboardID );;
}