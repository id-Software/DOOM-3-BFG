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
#include "../../doomclassic/doom/doomdef.h"

idCVar achievements_Verbose( "achievements_Verbose", "1", CVAR_BOOL, "debug spam" );
idCVar g_demoMode( "g_demoMode", "0", CVAR_INTEGER, "this is a demo" );

bool idAchievementManager::cheatingDialogShown = false;

const struct achievementInfo_t
{
	int required;
	bool lifetime; // true means the current count is stored on the player profile.  Doesn't matter for single count achievements.
} achievementInfo [ACHIEVEMENTS_NUM] =
{
	{ 50, true }, // ACHIEVEMENT_EARN_ALL_50_TROPHIES
	{ 1, true }, // ACHIEVEMENT_COMPLETED_DIFFICULTY_0
	{ 1, true }, // ACHIEVEMENT_COMPLETED_DIFFICULTY_1
	{ 1, true }, // ACHIEVEMENT_COMPLETED_DIFFICULTY_2
	{ 1, true }, // ACHIEVEMENT_COMPLETED_DIFFICULTY_3
	{ 64, false }, // ACHIEVEMENT_PDAS_BASE
	{ 14, false }, // ACHIEVEMENT_WATCH_ALL_VIDEOS
	{ 1, false }, // ACHIEVEMENT_KILL_MONSTER_WITH_1_HEALTH_LEFT
	{ 35, false }, // ACHIEVEMENT_OPEN_ALL_LOCKERS
	{ 20, true }, // ACHIEVEMENT_KILL_20_ENEMY_FISTS_HANDS
	{ 1, true }, // ACHIEVEMENT_KILL_SCI_NEXT_TO_RCR
	{ 1, true }, // ACHIEVEMENT_KILL_TWO_IMPS_ONE_SHOTGUN
	{ 1, true }, // ACHIEVEMENT_SCORE_25000_TURKEY_PUNCHER
	{ 50, true }, // ACHIEVEMENT_DESTROY_BARRELS
	{ 1, true }, // ACHIEVEMENT_GET_BFG_FROM_SECURITY_OFFICE
	{ 1, true }, // ACHIEVEMENT_COMPLETE_LEVEL_WITHOUT_TAKING_DMG
	{ 1, true }, // ACHIEVEMENT_FIND_RAGE_LOGO
	{ 1, true }, // ACHIEVEMENT_SPEED_RUN
	{ 1, true }, // ACHIEVEMENT_DEFEAT_VAGARY_BOSS
	{ 1, true }, // ACHIEVEMENT_DEFEAT_GUARDIAN_BOSS
	{ 1, true }, // ACHIEVEMENT_DEFEAT_SABAOTH_BOSS
	{ 1, true }, // ACHIEVEMENT_DEFEAT_CYBERDEMON_BOSS
	{ 1, true }, // ACHIEVEMENT_SENTRY_BOT_ALIVE_TO_DEST
	{ 20, true }, // ACHIEVEMENT_KILL_20_ENEMY_WITH_CHAINSAW
	{ 1, true }, // ACHIEVEMENT_ID_LOGO_SECRET_ROOM
	{ 1, true }, // ACHIEVEMENT_BLOODY_HANDWORK_OF_BETRUGER
	{ 1, true }, // ACHIEVEMENT_TWO_DEMONS_FIGHT_EACH_OTHER
	{ 20, true }, // ACHIEVEMENT_USE_SOUL_CUBE_TO_DEFEAT_20_ENEMY
	{ 1, true }, // ACHIEVEMENT_ROE_COMPLETED_DIFFICULTY_0
	{ 1, true }, // ACHIEVEMENT_ROE_COMPLETED_DIFFICULTY_1
	{ 1, true }, // ACHIEVEMENT_ROE_COMPLETED_DIFFICULTY_2
	{ 1, true }, // ACHIEVEMENT_ROE_COMPLETED_DIFFICULTY_3
	{ 22, false }, // ACHIEVEMENT_PDAS_ROE
	{ 1, true }, // ACHIEVEMENT_KILL_5_ENEMY_HELL_TIME
	{ 1, true }, // ACHIEVEMENT_DEFEAT_HELLTIME_HUNTER
	{ 1, true }, // ACHIEVEMENT_DEFEAT_BERSERK_HUNTER
	{ 1, true }, // ACHIEVEMENT_DEFEAT_INVULNERABILITY_HUNTER
	{ 1, true }, // ACHIEVEMENT_DEFEAT_MALEDICT_BOSS
	{ 20, true }, // ACHIEVEMENT_GRABBER_KILL_20_ENEMY
	{ 20, true }, // ACHIEVEMENT_ARTIFACT_WITH_BERSERK_PUNCH_20
	{ 1, true }, // ACHIEVEMENT_LE_COMPLETED_DIFFICULTY_0
	{ 1, true }, // ACHIEVEMENT_LE_COMPLETED_DIFFICULTY_1
	{ 1, true }, // ACHIEVEMENT_LE_COMPLETED_DIFFICULTY_2
	{ 1, true }, // ACHIEVEMENT_LE_COMPLETED_DIFFICULTY_3
	{ 10, false }, // ACHIEVEMENT_PDAS_LE
	{ 1, true }, // ACHIEVEMENT_MP_KILL_PLAYER_VIA_TELEPORT
	{ 1, true }, // ACHIEVEMENT_MP_CATCH_ENEMY_IN_ROFC
	{ 5, true }, // ACHIEVEMENT_MP_KILL_5_PLAYERS_USING_INVIS
	{ 1, true }, // ACHIEVEMENT_MP_COMPLETE_MATCH_WITHOUT_DYING
	{ 1, true }, // ACHIEVEMENT_MP_USE_BERSERK_TO_KILL_PLAYER
	{ 1, true }, // ACHIEVEMENT_MP_KILL_2_GUYS_IN_ROOM_WITH_BFG
};

/*
================================================================================================

	idAchievementManager

================================================================================================
*/

/*
========================
idAchievementManager::idAchievementManager
========================
*/
idAchievementManager::idAchievementManager() :
	lastPlayerKilledTime( 0 ),
	lastImpKilledTime( 0 ),
	playerTookDamage( false )
{
	counts.Zero();
	ResetHellTimeKills();
}

/*
========================
idAchievementManager::Init
========================
*/
void idAchievementManager::Init( idPlayer* player )
{
	owner = player;
	SyncAchievments();
}

/*
========================
idAchievementManager::SyncAchievments
========================
*/
void idAchievementManager::SyncAchievments()
{
	idLocalUser* user = GetLocalUser();
	if( user == NULL || user->GetProfile() == NULL )
	{
		return;
	}
	
	// Set achievement counts
	for( int i = 0; i < counts.Num(); i++ )
	{
		if( user->GetProfile()->GetAchievement( i ) )
		{
			counts[i] = achievementInfo[i].required;
		}
		else if( achievementInfo[i].lifetime )
		{
			counts[i] = user->GetStatInt( i );
		}
	}
}

/*
========================
idAchievementManager::GetLocalUser
========================
*/
idLocalUser* idAchievementManager::GetLocalUser()
{
	if( !verify( owner != NULL ) )
	{
		return NULL;
	}
	return session->GetGameLobbyBase().GetLocalUserFromLobbyUser( gameLocal.lobbyUserIDs[ owner->GetEntityNumber() ] );
}

/*
========================
idAchievementManager::Save
========================
*/
void idAchievementManager::Save( idSaveGame* savefile ) const
{
	owner.Save( savefile );
	
	for( int i = 0; i < ACHIEVEMENTS_NUM; i++ )
	{
		savefile->WriteInt( counts[i] );
	}
	
	savefile->WriteInt( lastImpKilledTime );
	savefile->WriteInt( lastPlayerKilledTime );
	savefile->WriteBool( playerTookDamage );
	savefile->WriteInt( currentHellTimeKills );
}

/*
========================
idAchievementManager::Restore
========================
*/
void idAchievementManager::Restore( idRestoreGame* savefile )
{
	owner.Restore( savefile );
	
	for( int i = 0; i < ACHIEVEMENTS_NUM; i++ )
	{
		savefile->ReadInt( counts[i] );
	}
	
	savefile->ReadInt( lastImpKilledTime );
	savefile->ReadInt( lastPlayerKilledTime );
	savefile->ReadBool( playerTookDamage );
	savefile->ReadInt( currentHellTimeKills );
	
	SyncAchievments();
}

/*
========================
idAchievementManager::EventCompletesAchievement
========================
*/
void idAchievementManager::EventCompletesAchievement( const achievement_t eventId )
{
	if( g_demoMode.GetBool() )
	{
		return;
	}
	
	idLocalUser* localUser = GetLocalUser();
	if( localUser == NULL || localUser->GetProfile() == NULL )
	{
	
		// Send a Reliable Message to the User that needs to unlock this.
		if( owner != NULL )
		{
			int playerId = owner->entityNumber;
			const int bufferSize = sizeof( playerId ) + sizeof( eventId );
			byte buffer[ bufferSize ];
			idBitMsg msg;
			msg.InitWrite( buffer, bufferSize );
			
			msg.WriteByte( playerId );
			msg.WriteByte( eventId );
			
			msg.WriteByteAlign();
			idLib::Printf( "Host Sending Achievement\n" );
			session->GetActingGameStateLobbyBase().SendReliableToLobbyUser( gameLocal.lobbyUserIDs[ owner->entityNumber ], GAME_RELIABLE_MESSAGE_ACHIEVEMENT_UNLOCK, msg );
		}
		
		return; // Remote user or build game
	}
	
	// Check to see if we've already given the achievement.
	// If so, don't do again because we don't want to autosave every time a trigger is hit
	if( localUser->GetProfile()->GetAchievement( eventId ) )
	{
		return;
	}
	
#ifdef ID_RETAIL
	if( common->GetConsoleUsed() )
	{
		if( !cheatingDialogShown )
		{
			common->Dialog().AddDialog( GDM_ACHIEVEMENTS_DISABLED_DUE_TO_CHEATING, DIALOG_ACCEPT, NULL, NULL, true );
			cheatingDialogShown = true;
		}
		return;
	}
#endif
	
	counts[eventId]++;
	
	if( counts[eventId] >= achievementInfo[eventId].required )
	{
		session->GetAchievementSystem().AchievementUnlock( localUser, eventId );
	}
	else
	{
		if( achievementInfo[eventId].lifetime )
		{
			localUser->SetStatInt( eventId, counts[eventId] );
		}
	}
}

/*
========================
idAchievementManager::IncrementHellTimeKills
========================
*/
void idAchievementManager::IncrementHellTimeKills()
{
	currentHellTimeKills++;
	if( currentHellTimeKills >= 5 )
	{
		EventCompletesAchievement( ACHIEVEMENT_KILL_5_ENEMY_HELL_TIME );
	}
}

/*
========================
idAchievementManager::SavePersistentData
========================
*/
void idAchievementManager::SavePersistentData( idDict& playerInfo )
{
	for( int i = 0; i < ACHIEVEMENTS_NUM; ++i )
	{
		playerInfo.SetInt( va( "ach_%d", i ), counts[i] );
	}
}

/*
========================
idAchievementManager::RestorePersistentData
========================
*/
void idAchievementManager::RestorePersistentData( const idDict& spawnArgs )
{
	for( int i = 0; i < ACHIEVEMENTS_NUM; ++i )
	{
		counts[i] = spawnArgs.GetInt( va( "ach_%d", i ), "0" );
	}
}


/*
========================
idAchievementManager::LocalUser_CompleteAchievement
========================
*/
void idAchievementManager::LocalUser_CompleteAchievement( achievement_t id )
{
	idLocalUser* localUser = session->GetSignInManager().GetMasterLocalUser();
	
	// Check to see if we've already given the achievement.
	// If so, don't do again because we don't want to autosave every time a trigger is hit
	if( localUser == NULL || localUser->GetProfile()->GetAchievement( id ) )
	{
		return;
	}
	
#ifdef ID_RETAIL
	if( common->GetConsoleUsed() )
	{
		if( !cheatingDialogShown )
		{
			common->Dialog().AddDialog( GDM_ACHIEVEMENTS_DISABLED_DUE_TO_CHEATING, DIALOG_ACCEPT, NULL, NULL, true );
			cheatingDialogShown = true;
		}
		return;
	}
#endif
	
	session->GetAchievementSystem().AchievementUnlock( localUser, id );
}

/*
========================
idAchievementManager::CheckDoomClassicsAchievements

Processed when the player finishes a level.
========================
*/
// RB begin
#if defined(USE_DOOMCLASSIC)
void idAchievementManager::CheckDoomClassicsAchievements( int killcount, int itemcount, int secretcount, int skill, int mission, int map, int episode, int totalkills, int totalitems, int totalsecret )
{

	const skill_t difficulty = ( skill_t )skill;
	const currentGame_t currentGame = common->GetCurrentGame();
	const GameMission_t expansion = ( GameMission_t )mission;
	
	
	idLocalUser* localUser = session->GetSignInManager().GetMasterLocalUser();
	if( localUser != NULL && localUser->GetProfile() != NULL )
	{
	
		// GENERAL ACHIEVEMENT UNLOCKING.
		if( currentGame == DOOM_CLASSIC )
		{
			LocalUser_CompleteAchievement( ACHIEVEMENT_DOOM1_NEOPHYTE_COMPLETE_ANY_LEVEL );
		}
		else if( currentGame == DOOM2_CLASSIC )
		{
			LocalUser_CompleteAchievement( ACHIEVEMENT_DOOM2_JUST_GETTING_STARTED_COMPLETE_ANY_LEVEL );
		}
		
		// Complete Any Level on Nightmare.
		if( difficulty == sk_nightmare && currentGame == DOOM_CLASSIC )
		{
			LocalUser_CompleteAchievement( ACHIEVEMENT_DOOM1_NIGHTMARE_COMPLETE_ANY_LEVEL_NIGHTMARE );
		}
		
		const bool gotAllKills = killcount >= totalkills;
		const bool gotAllItems = itemcount >= totalitems;
		const bool gotAllSecrets = secretcount >= totalsecret;
		
		if( gotAllItems && gotAllKills && gotAllSecrets )
		{
			if( currentGame == DOOM_CLASSIC )
			{
				LocalUser_CompleteAchievement( ACHIEVEMENT_DOOM1_BURNING_OUT_OF_CONTROL_COMPLETE_KILLS_ITEMS_SECRETS );
			}
			else if( currentGame == DOOM2_CLASSIC )
			{
				LocalUser_CompleteAchievement( ACHIEVEMENT_DOOM2_BURNING_OUT_OF_CONTROL_COMPLETE_KILLS_ITEMS_SECRETS );
			}
		}
		
		// DOOM EXPANSION ACHIEVEMENTS
		if( expansion == doom )
		{
		
			if( map == 8 )
			{
			
				// Medium or higher skill level.
				if( difficulty >= sk_medium )
				{
					localUser->SetStatInt( STAT_DOOM_COMPLETED_EPISODE_1_MEDIUM + ( episode - 1 ), 1 );
				}
				
				// Hard or higher skill level.
				if( difficulty >= sk_hard )
				{
					localUser->SetStatInt( STAT_DOOM_COMPLETED_EPISODE_1_HARD + ( episode - 1 ), 1 );
					localUser->SetStatInt( STAT_DOOM_COMPLETED_EPISODE_1_MEDIUM + ( episode - 1 ), 1 );
				}
				
				if( difficulty == sk_nightmare )
				{
					localUser->SetStatInt( STAT_DOOM_COMPLETED_EPISODE_1_HARD + ( episode - 1 ), 1 );
					localUser->SetStatInt( STAT_DOOM_COMPLETED_EPISODE_1_MEDIUM + ( episode - 1 ), 1 );
				}
				
				// Save the Settings.
				localUser->SaveProfileSettings();
			}
			
			// Check to see if we've completed all episodes.
			const int episode1completed = localUser->GetStatInt( STAT_DOOM_COMPLETED_EPISODE_1_MEDIUM );
			const int episode2completed = localUser->GetStatInt( STAT_DOOM_COMPLETED_EPISODE_2_MEDIUM );
			const int episode3completed = localUser->GetStatInt( STAT_DOOM_COMPLETED_EPISODE_3_MEDIUM );
			const int episode4completed = localUser->GetStatInt( STAT_DOOM_COMPLETED_EPISODE_4_MEDIUM );
			
			const int episode1completed_hard = localUser->GetStatInt( STAT_DOOM_COMPLETED_EPISODE_1_HARD );
			const int episode2completed_hard = localUser->GetStatInt( STAT_DOOM_COMPLETED_EPISODE_2_HARD );
			const int episode3completed_hard = localUser->GetStatInt( STAT_DOOM_COMPLETED_EPISODE_3_HARD );
			const int episode4completed_hard = localUser->GetStatInt( STAT_DOOM_COMPLETED_EPISODE_4_HARD );
			
			if( currentGame == DOOM_CLASSIC )
			{
				if( episode1completed )
				{
					LocalUser_CompleteAchievement( ACHIEVEMENT_DOOM1_EPISODE1_COMPLETE_MEDIUM );
				}
				
				if( episode2completed )
				{
					LocalUser_CompleteAchievement( ACHIEVEMENT_DOOM1_EPISODE2_COMPLETE_MEDIUM );
				}
				
				if( episode3completed )
				{
					LocalUser_CompleteAchievement( ACHIEVEMENT_DOOM1_EPISODE3_COMPLETE_MEDIUM );
				}
				
				if( episode4completed )
				{
					LocalUser_CompleteAchievement( ACHIEVEMENT_DOOM1_EPISODE4_COMPLETE_MEDIUM );
				}
				
				if( episode1completed_hard && episode2completed_hard && episode3completed_hard && episode4completed_hard )
				{
					LocalUser_CompleteAchievement( ACHIEVEMENT_DOOM1_RAMPAGE_COMPLETE_ALL_HARD );
				}
			}
		}
		else if( expansion == doom2 )
		{
		
			if( map == 30 )
			{
			
				if( currentGame == DOOM2_CLASSIC )
				{
					LocalUser_CompleteAchievement( ACHIEVEMENT_DOOM2_FROM_EARTH_TO_HELL_COMPLETE_HELL_ON_EARTH );
					
					if( difficulty >= sk_hard )
					{
						LocalUser_CompleteAchievement( ACHIEVEMENT_DOOM2_SUPERIOR_FIREPOWER_COMPLETE_ALL_HARD );
					}
				}
			}
		}
		else if( expansion ==  pack_nerve )
		{
			if( map == 8 )
			{
			
				if( currentGame == DOOM2_CLASSIC )
				{
					LocalUser_CompleteAchievement( ACHIEVEMENT_DOOM2_AND_BACK_AGAIN_COMPLETE_NO_REST );
				}
			}
		}
		
	}
}
#endif // #if defined(USE_DOOMCLASSIC)
// RB end

/*
=================
AchievementsReset
=================
*/
CONSOLE_COMMAND( AchievementsReset, "Lock an achievement", NULL )
{
	idLocalUser* user = session->GetSignInManager().GetMasterLocalUser();
	if( user == NULL )
	{
		idLib::Printf( "Must be signed in\n" );
		return;
	}
	if( args.Argc() == 1 )
	{
		for( int i = 0; i < ACHIEVEMENTS_NUM; i++ )
		{
			user->SetStatInt( i, 0 );
			session->GetAchievementSystem().AchievementLock( user, i );
		}
	}
	else
	{
		int i = atoi( args.Argv( 1 ) );
		user->SetStatInt( i, 0 );
		session->GetAchievementSystem().AchievementLock( user, i );
	}
	user->SaveProfileSettings();
}

/*
=================
AchievementsUnlock
=================
*/
CONSOLE_COMMAND( AchievementsUnlock, "Unlock an achievement", NULL )
{
	idLocalUser* user = session->GetSignInManager().GetMasterLocalUser();
	if( user == NULL )
	{
		idLib::Printf( "Must be signed in\n" );
		return;
	}
	if( args.Argc() == 1 )
	{
		for( int i = 0; i < ACHIEVEMENTS_NUM; i++ )
		{
			user->SetStatInt( i, achievementInfo[i].required );
			session->GetAchievementSystem().AchievementUnlock( user, i );
		}
	}
	else
	{
		int i = atoi( args.Argv( 1 ) );
		user->SetStatInt( i, achievementInfo[i].required );
		session->GetAchievementSystem().AchievementUnlock( user, i );
	}
	user->SaveProfileSettings();
}

/*
=================
AchievementsList
=================
*/
CONSOLE_COMMAND( AchievementsList, "Lists achievements and status", NULL )
{
	idPlayer* player = gameLocal.GetLocalPlayer();
	idLocalUser* user = ( player == NULL ) ? session->GetSignInManager().GetMasterLocalUser() : session->GetGameLobbyBase().GetLocalUserFromLobbyUser( gameLocal.lobbyUserIDs[ player->GetEntityNumber() ] );
	if( user == NULL )
	{
		idLib::Printf( "Must be signed in\n" );
		return;
	}
	idPlayerProfile* profile = user->GetProfile();
	
	idArray<bool, 128> achievementState;
	bool achievementStateValid = session->GetAchievementSystem().GetAchievementState( user, achievementState );
	
	for( int i = 0; i < ACHIEVEMENTS_NUM; i++ )
	{
		const char* pInfo = "";
		if( profile == NULL )
		{
			pInfo = S_COLOR_RED  "unknown" S_COLOR_DEFAULT;
		}
		else if( !profile->GetAchievement( i ) )
		{
			pInfo = S_COLOR_YELLOW "locked" S_COLOR_DEFAULT;
		}
		else
		{
			pInfo = S_COLOR_GREEN "unlocked" S_COLOR_DEFAULT;
		}
		const char* sInfo = "";
		if( !achievementStateValid )
		{
			sInfo = S_COLOR_RED  "unknown" S_COLOR_DEFAULT;
		}
		else if( !achievementState[i] )
		{
			sInfo = S_COLOR_YELLOW "locked" S_COLOR_DEFAULT;
		}
		else
		{
			sInfo = S_COLOR_GREEN "unlocked" S_COLOR_DEFAULT;
		}
		int count = 0;
		if( achievementInfo[i].lifetime )
		{
			count = user->GetStatInt( i );
		}
		else if( player != NULL )
		{
			count = player->GetAchievementManager().GetCount( ( achievement_t ) i );
		}
		else
		{
			count = 0;
		}
		
		achievementDescription_t data;
		bool descriptionValid = session->GetAchievementSystem().GetAchievementDescription( user, i, data );
		
		idLib::Printf( "%02d: %2d/%2d | %12.12s | %12.12s | %s%s\n", i, count, achievementInfo[i].required, pInfo, sInfo, descriptionValid ? data.hidden ? "(hidden) " : "" : "(unknown) ", descriptionValid ? data.name : "" );
	}
}
