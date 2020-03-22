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
#ifndef __ACHIEVEMENTS_H__
#define __ACHIEVEMENTS_H__

enum achievement_t
{

	ACHIEVEMENT_INVALID = -1,

	ACHIEVEMENT_EARN_ALL_50_TROPHIES,					// 0 // DONE -- (automagic?)

	ACHIEVEMENT_COMPLETED_DIFFICULTY_0,					// 1 // DONE -- Recruit
	ACHIEVEMENT_COMPLETED_DIFFICULTY_1,					// 2 // DONE -- Marine
	ACHIEVEMENT_COMPLETED_DIFFICULTY_2,					// 3 // DONE -- Veteran
	ACHIEVEMENT_COMPLETED_DIFFICULTY_3,					// 4 // DONE -- Nightmare

	ACHIEVEMENT_PDAS_BASE,								// 5 // DONE --
	ACHIEVEMENT_WATCH_ALL_VIDEOS,						// 6 // DONE --
	ACHIEVEMENT_KILL_MONSTER_WITH_1_HEALTH_LEFT,		// 7 // DONE --
	ACHIEVEMENT_OPEN_ALL_LOCKERS,						// 8 // DONE --
	ACHIEVEMENT_KILL_20_ENEMY_FISTS_HANDS,				// 9 // DONE --- kill 20 enemies with fists & hands
	ACHIEVEMENT_KILL_SCI_NEXT_TO_RCR,					// 10 // DONE -----> ADD TARGET TO MAP   kill scientist trapped next to reactor control room
	ACHIEVEMENT_KILL_TWO_IMPS_ONE_SHOTGUN,				// 11 // DONE --
	ACHIEVEMENT_SCORE_25000_TURKEY_PUNCHER,				// 12 // DONE --
	ACHIEVEMENT_DESTROY_BARRELS,						// 13 // DONE --
	ACHIEVEMENT_GET_BFG_FROM_SECURITY_OFFICE,			// 14 // DONE -----> ADD TARGET TO MAP
	ACHIEVEMENT_COMPLETE_LEVEL_WITHOUT_TAKING_DMG,		// 15 // DONE --
	ACHIEVEMENT_FIND_RAGE_LOGO,							// 16 // DONE -----> ADD TARGET TO MAP (jerry)
	ACHIEVEMENT_SPEED_RUN,								// 17 // DONE --

	ACHIEVEMENT_DEFEAT_VAGARY_BOSS,						// 18 // DONE --
	ACHIEVEMENT_DEFEAT_GUARDIAN_BOSS,					// 19 // DONE --
	ACHIEVEMENT_DEFEAT_SABAOTH_BOSS,					// 20 // DONE --
	ACHIEVEMENT_DEFEAT_CYBERDEMON_BOSS,					// 21 // DONE --

	ACHIEVEMENT_SENTRY_BOT_ALIVE_TO_DEST,				// 22 // DONE -----> ADD TARGET TO MAP
	ACHIEVEMENT_KILL_20_ENEMY_WITH_CHAINSAW,			// 23 // DONE --
	ACHIEVEMENT_ID_LOGO_SECRET_ROOM,					// 24 // DONE -----> ADD TARGET TO MAP
	ACHIEVEMENT_BLOODY_HANDWORK_OF_BETRUGER,			// 25 // DONE -----> ADD TARGET TO MAP
	ACHIEVEMENT_TWO_DEMONS_FIGHT_EACH_OTHER,			// 26 // DONE --
	ACHIEVEMENT_USE_SOUL_CUBE_TO_DEFEAT_20_ENEMY,		// 27 // DONE --

	ACHIEVEMENT_ROE_COMPLETED_DIFFICULTY_0,				// 28 // DONE -- Recruit
	ACHIEVEMENT_ROE_COMPLETED_DIFFICULTY_1,				// 29 // DONE -- Marine
	ACHIEVEMENT_ROE_COMPLETED_DIFFICULTY_2,				// 30 // DONE -- Veteran
	ACHIEVEMENT_ROE_COMPLETED_DIFFICULTY_3,				// 31 // DONE -- Nightmare

	ACHIEVEMENT_PDAS_ROE,								// 32 // DONE -- read all pdas in RoE
	ACHIEVEMENT_KILL_5_ENEMY_HELL_TIME,					// 33 // DONE --
	ACHIEVEMENT_DEFEAT_HELLTIME_HUNTER,					// 34 // DONE --
	ACHIEVEMENT_DEFEAT_BERSERK_HUNTER,					// 35 // DONE --
	ACHIEVEMENT_DEFEAT_INVULNERABILITY_HUNTER,			// 36 // DONE --
	ACHIEVEMENT_DEFEAT_MALEDICT_BOSS,					// 37 // DONE --
	ACHIEVEMENT_GRABBER_KILL_20_ENEMY,					// 38 // DONE --
	ACHIEVEMENT_ARTIFACT_WITH_BERSERK_PUNCH_20,			// 39 // DONE --

	ACHIEVEMENT_LE_COMPLETED_DIFFICULTY_0,				// 40 // DONE -- Recruit
	ACHIEVEMENT_LE_COMPLETED_DIFFICULTY_1,				// 41 // DONE -- Marine
	ACHIEVEMENT_LE_COMPLETED_DIFFICULTY_2,				// 42 // DONE -- Veteran
	ACHIEVEMENT_LE_COMPLETED_DIFFICULTY_3,				// 43 // DONE -- Nightmare

	ACHIEVEMENT_PDAS_LE,								// 44 // DONE -- read all pdas in LE

	ACHIEVEMENT_MP_KILL_PLAYER_VIA_TELEPORT,			// 45 // DONE --
	ACHIEVEMENT_MP_CATCH_ENEMY_IN_ROFC,					// 46 // DONE -- needs to be tested -- Reactor of Frag Chamber
	ACHIEVEMENT_MP_KILL_5_PLAYERS_USING_INVIS,			// 47 // DONE --
	ACHIEVEMENT_MP_COMPLETE_MATCH_WITHOUT_DYING,		// 48 // DONE --
	ACHIEVEMENT_MP_USE_BERSERK_TO_KILL_PLAYER,			// 49 // DONE --
	ACHIEVEMENT_MP_KILL_2_GUYS_IN_ROOM_WITH_BFG,		// 50 // DONE --

	ACHIEVEMENT_DOOM1_NEOPHYTE_COMPLETE_ANY_LEVEL,							// 51
	ACHIEVEMENT_DOOM1_EPISODE1_COMPLETE_MEDIUM,								// 52
	ACHIEVEMENT_DOOM1_EPISODE2_COMPLETE_MEDIUM,								// 53
	ACHIEVEMENT_DOOM1_EPISODE3_COMPLETE_MEDIUM,								// 54
	ACHIEVEMENT_DOOM1_EPISODE4_COMPLETE_MEDIUM,								// 55
	ACHIEVEMENT_DOOM1_RAMPAGE_COMPLETE_ALL_HARD,							// 56
	ACHIEVEMENT_DOOM1_NIGHTMARE_COMPLETE_ANY_LEVEL_NIGHTMARE,				// 57
	ACHIEVEMENT_DOOM1_BURNING_OUT_OF_CONTROL_COMPLETE_KILLS_ITEMS_SECRETS,	// 58

	ACHIEVEMENT_DOOM2_JUST_GETTING_STARTED_COMPLETE_ANY_LEVEL,				// 59
	ACHIEVEMENT_DOOM2_FROM_EARTH_TO_HELL_COMPLETE_HELL_ON_EARTH,			// 60
	ACHIEVEMENT_DOOM2_AND_BACK_AGAIN_COMPLETE_NO_REST,						// 61
	ACHIEVEMENT_DOOM2_SUPERIOR_FIREPOWER_COMPLETE_ALL_HARD,					// 62
	ACHIEVEMENT_DOOM2_REALLY_BIG_GUN_FIND_BFG_SINGLEPLAYER,					// 63
	ACHIEVEMENT_DOOM2_BURNING_OUT_OF_CONTROL_COMPLETE_KILLS_ITEMS_SECRETS,	// 64
	ACHIEVEMENT_DOOM2_IMPORTANT_LOOKING_DOOR_FIND_ANY_SECRET,				// 65

	ACHIEVEMENTS_NUM,

	STAT_DOOM_COMPLETED_EPISODE_1_MEDIUM,
	STAT_DOOM_COMPLETED_EPISODE_2_MEDIUM,
	STAT_DOOM_COMPLETED_EPISODE_3_MEDIUM,
	STAT_DOOM_COMPLETED_EPISODE_4_MEDIUM,

	STAT_DOOM_COMPLETED_EPISODE_1_HARD,
	STAT_DOOM_COMPLETED_EPISODE_2_HARD,
	STAT_DOOM_COMPLETED_EPISODE_3_HARD,
	STAT_DOOM_COMPLETED_EPISODE_4_HARD,
};

compile_time_assert( ACHIEVEMENTS_NUM <= idPlayerProfile::MAX_PLAYER_PROFILE_STATS );

/*
================================================
idAchievementManager

Manages a List of Achievements associated with a particular Player.

This is setup to only have one achievement manager per game.
================================================
*/
class idAchievementManager
{
public:
	idAchievementManager();

	void		Init( idPlayer* player );
	bool		IsInitialized() const
	{
		return owner != NULL;
	}

	// save games
	void		Save( idSaveGame* savefile ) const;					// archives object for save game file
	void		Restore( idRestoreGame* savefile );					// unarchives object from save game file

	// Debug tool to reset achievement state and counts
	void		Reset();
	int			GetCount( const achievement_t eventId ) const
	{
		return counts[eventId];
	}

	// Adds a count to the tracked number of events, these events can be applied to multiple achievements
	void		EventCompletesAchievement( const achievement_t eventId );

	int			GetLastImpKilledTime()
	{
		return lastImpKilledTime;
	}
	void		SetLastImpKilledTime( int time )
	{
		lastImpKilledTime = time;
	}
	int			GetLastPlayerKilledTime()
	{
		return lastPlayerKilledTime;
	}
	void		SetLastPlayerKilledTime( int time )
	{
		lastPlayerKilledTime = time;
	}
	bool		GetPlayerTookDamage()
	{
		return playerTookDamage;
	}
	void		SetPlayerTookDamage( bool bl )
	{
		playerTookDamage = bl;
	}
	void		IncrementHellTimeKills();
	void		ResetHellTimeKills()
	{
		currentHellTimeKills = 0;
	}
	void		SavePersistentData( idDict& playerInfo );
	void		RestorePersistentData( const idDict& spawnArgs );

	static void LocalUser_CompleteAchievement( achievement_t id );

	// RB begin
#if defined(USE_DOOMCLASSIC)
	static void	CheckDoomClassicsAchievements( int killcount, int itemcount, int secretcount, int skill, int mission, int map, int episode, int totalkills, int totalitems, int totalsecret );
#endif
	// RB end

private:
	idEntityPtr< idPlayer >	owner;
	idArray<int, ACHIEVEMENTS_NUM> counts; // How many times has each achievement been given

	int				lastPlayerKilledTime;
	int				lastImpKilledTime;
	bool			playerTookDamage;
	int				currentHellTimeKills;

	static bool		cheatingDialogShown;

	idLocalUser* 	GetLocalUser();
	void			SyncAchievments();
};

#endif // !__ACHIEVEMENTS_H__
