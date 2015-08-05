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

#include "Precompiled.h"
#include "globaldata.h"


// Data.
#include "doomdef.h"
#include "dstrings.h"
#include "sounds.h"

#include "doomstat.h"

#include "m_random.h"
#include "i_system.h"

#include "am_map.h"

#include "p_local.h"

#include "s_sound.h"

#ifdef __GNUG__
#pragma implementation "p_inter.h"
#endif
#include "p_inter.h"

#include "Main.h"

#include "sys/sys_signin.h"

#include "../../neo/d3xp/Game_local.h"

// a weapon is found with two clip loads,
// a big item has five clip loads
const int	maxammo[NUMAMMO] = {200, 50, 300, 50};
const int	clipammo[NUMAMMO] = {10, 4, 20, 1};


//
// GET STUFF
//

//
// P_GiveAmmo
// Num is the number of clip loads,
// not the individual count (0= 1/2 clip).
// Returns false if the ammo can't be picked up at all
//

qboolean
P_GiveAmmo
( player_t*	player,
 ammotype_t	ammo,
 int		num )
{
	int		oldammo;

	if (ammo == am_noammo)
		return false;

	if (ammo < 0 || ammo > NUMAMMO)
		I_Error ("P_GiveAmmo: bad type %i", ammo);

	if ( player->ammo[ammo] == player->maxammo[ammo]  )
		return false;

	if (num)
		num *= clipammo[ammo];
	else
		num = clipammo[ammo]/2;

	if (::g->gameskill == sk_baby
		|| ::g->gameskill == sk_nightmare)
	{
		// give double ammo in trainer mode,
		// you'll need in nightmare
		num <<= 1;
	}


	oldammo = player->ammo[ammo];
	player->ammo[ammo] += num;

	if (player->ammo[ammo] > player->maxammo[ammo])
		player->ammo[ammo] = player->maxammo[ammo];

	// If non zero ammo, 
	// don't change up weapons,
	// player was lower on purpose.
	if (oldammo)
		return true;	

	// We were down to zero,
	// so select a new weapon.
	// Preferences are not user selectable.
	switch (ammo)
	{
	case am_clip:
		if (player->readyweapon == wp_fist)
		{
			if (player->weaponowned[wp_chaingun])
				player->pendingweapon = wp_chaingun;
			else
				player->pendingweapon = wp_pistol;
		}
		break;

	case am_shell:
		if (player->readyweapon == wp_fist
			|| player->readyweapon == wp_pistol)
		{
			if (player->weaponowned[wp_shotgun])
				player->pendingweapon = wp_shotgun;
		}
		break;

	case am_cell:
		if (player->readyweapon == wp_fist
			|| player->readyweapon == wp_pistol)
		{
			if (player->weaponowned[wp_plasma])
				player->pendingweapon = wp_plasma;
		}
		break;

	case am_misl:
		if (player->readyweapon == wp_fist)
		{
			if (player->weaponowned[wp_missile])
				player->pendingweapon = wp_missile;
		}
	default:
		break;
	}

	return true;
}


//
// P_GiveWeapon
// The weapon name may have a MF_DROPPED flag ored in.
//
qboolean
P_GiveWeapon
( player_t*	player,
 weapontype_t	weapon,
 qboolean	dropped )
{
	qboolean	gaveammo;
	qboolean	gaveweapon;

	if (::g->netgame
		&& (::g->deathmatch!=2)
		&& !dropped )
	{
		// leave placed weapons forever on net games
		if (player->weaponowned[weapon])
			return false;

		player->bonuscount += BONUSADD;
		player->weaponowned[weapon] = true;

		if (::g->deathmatch)
			P_GiveAmmo (player, weaponinfo[weapon].ammo, 5);
		else
			P_GiveAmmo (player, weaponinfo[weapon].ammo, 2);
		player->pendingweapon = weapon;

		if (player == &::g->players[::g->consoleplayer])
			S_StartSound (player->mo, sfx_wpnup);
		return false;
	}

	if (weaponinfo[weapon].ammo != am_noammo)
	{
		// give one clip with a dropped weapon,
		// two clips with a found weapon
		if (dropped)
			gaveammo = P_GiveAmmo (player, weaponinfo[weapon].ammo, 1);
		else
			gaveammo = P_GiveAmmo (player, weaponinfo[weapon].ammo, 2);
	}
	else
		gaveammo = false;

	if (player->weaponowned[weapon])
		gaveweapon = false;
	else
	{
		gaveweapon = true;
		player->weaponowned[weapon] = true;
		player->pendingweapon = weapon;
	}

	return (gaveweapon || gaveammo);
}



//
// P_GiveBody
// Returns false if the body isn't needed at all
//
qboolean
P_GiveBody
( player_t*	player,
 int		num )
{
	if (player->health >= MAXHEALTH)
		return false;

	player->health += num;
	if (player->health > MAXHEALTH)
		player->health = MAXHEALTH;
	player->mo->health = player->health;

	return true;
}



//
// P_GiveArmor
// Returns false if the armor is worse
// than the current armor.
//
qboolean
P_GiveArmor
( player_t*	player,
 int		armortype )
{
	int		hits;

	hits = armortype*100;
	if (player->armorpoints >= hits)
		return false;	// don't pick up

	player->armortype = armortype;
	player->armorpoints = hits;

	return true;
}



//
// P_GiveCard
//
void P_GiveCard( player_t* player, card_t card, const char *pickup_message ) {

	if ( ( ::g->demoplayback && ::g->netgame ) || common->IsMultiplayer() ) {
		for ( int i=0; i < MAXPLAYERS; i++ ) {
			if ( ::g->playeringame[i] ) {
				player_t *thePlayer = &::g->players[i];

				if (thePlayer->cards[card])
					continue;

				thePlayer->bonuscount = BONUSADD;
				thePlayer->message = pickup_message;
				thePlayer->cards[card] = 1;
			}
		}
	} else {
		if (player->cards[card])
			return;

		player->bonuscount = BONUSADD;
		player->message = pickup_message;
		player->cards[card] = 1;
	}
}


//
// P_GivePower
//
qboolean
P_GivePower
( player_t*	player,
 int /*powertype_t*/	power )
{
	if (power == pw_invulnerability)
	{
		player->powers[power] = INVULNTICS;
		return true;
	}

	if (power == pw_invisibility)
	{
		player->powers[power] = INVISTICS;
		player->mo->flags |= MF_SHADOW;
		return true;
	}

	if (power == pw_infrared)
	{
		player->powers[power] = INFRATICS;
		return true;
	}

	if (power == pw_ironfeet)
	{
		player->powers[power] = IRONTICS;
		return true;
	}

	if (power == pw_strength)
	{
		P_GiveBody (player, 100);
		player->powers[power] = 1;
		return true;
	}

	if (player->powers[power])
		return false;	// already got it

	player->powers[power] = 1;
	return true;
}



//
// P_TouchSpecialThing
//
void
P_TouchSpecialThing
( mobj_t*	special,
 mobj_t*	toucher )
{
	player_t*	player;
	int		i;
	fixed_t	delta;
	int		sound;

	delta = special->z - toucher->z;

	if (delta > toucher->height
		|| delta < -8*FRACUNIT)
	{
		// out of reach
		return;
	}


	sound = sfx_itemup;	
	player = toucher->player;

	// Dead thing touching.
	// Can happen with a sliding player corpse.
	if (toucher->health <= 0)
		return;

	// Identify by sprite.
	switch (special->sprite)
	{
		// armor
	case SPR_ARM1:
		if (!P_GiveArmor (player, 1))
			return;
		player->message = GOTARMOR;
		break;

	case SPR_ARM2:
		if (!P_GiveArmor (player, 2))
			return;
		player->message = GOTMEGA;
		break;

		// bonus items
	case SPR_BON1:
		player->health++;		// can go over 100%
		if (player->health > 200)
			player->health = 200;
		player->mo->health = player->health;
		player->message = GOTHTHBONUS;
		break;

	case SPR_BON2:
		player->armorpoints++;		// can go over 100%
		if (player->armorpoints > 200)
			player->armorpoints = 200;
		if (!player->armortype)
			player->armortype = 1;
		player->message = GOTARMBONUS;
		break;

	case SPR_SOUL:
		player->health += 100;
		if (player->health > 200)
			player->health = 200;
		player->mo->health = player->health;
		player->message = GOTSUPER;
		sound = sfx_getpow;
		break;

	case SPR_MEGA:
		if (::g->gamemode != commercial)
			return;
		player->health = 200;
		player->mo->health = player->health;
		P_GiveArmor (player,2);
		player->message = GOTMSPHERE;
		sound = sfx_getpow;
		break;

		// cards
		// leave cards for everyone
	case SPR_BKEY:
		//if (!player->cards[it_bluecard])
			//player->message = GOTBLUECARD;
		P_GiveCard (player, it_bluecard, GOTBLUECARD);
		if (!::g->netgame)
			break;
		return;

	case SPR_YKEY:
		//if (!player->cards[it_yellowcard])
			//player->message = GOTYELWCARD;
		P_GiveCard (player, it_yellowcard, GOTYELWCARD);
		if (!::g->netgame)
			break;
		return;

	case SPR_RKEY:
		//if (!player->cards[it_redcard])
			//player->message = GOTREDCARD;
		P_GiveCard (player, it_redcard, GOTREDCARD);
		if (!::g->netgame)
			break;
		return;

	case SPR_BSKU:
		//if (!player->cards[it_blueskull])
			//player->message = GOTBLUESKUL;
		P_GiveCard (player, it_blueskull, GOTBLUESKUL);
		if (!::g->netgame)
			break;
		return;

	case SPR_YSKU:
		//if (!player->cards[it_yellowskull])
			//player->message = GOTYELWSKUL;
		P_GiveCard (player, it_yellowskull, GOTYELWSKUL);
		if (!::g->netgame)
			break;
		return;

	case SPR_RSKU:
		//if (!player->cards[it_redskull])
			//player->message = GOTREDSKULL;
		P_GiveCard (player, it_redskull, GOTREDSKULL);
		if (!::g->netgame)
			break;
		return;

		// medikits, heals
	case SPR_STIM:
		if (!P_GiveBody (player, 10))
			return;
		player->message = GOTSTIM;
		break;

	case SPR_MEDI:
		if (!P_GiveBody (player, 25))
			return;

		if (player->health < 25)
			player->message = GOTMEDINEED;
		else
			player->message = GOTMEDIKIT;
		break;


		// power ups
	case SPR_PINV:
		if (!P_GivePower (player, pw_invulnerability))
			return;
		player->message = GOTINVUL;
		sound = sfx_getpow;
		break;

	case SPR_PSTR:
		if (!P_GivePower (player, pw_strength))
			return;
		player->message = GOTBERSERK;
		if (player->readyweapon != wp_fist)
			player->pendingweapon = wp_fist;
		sound = sfx_getpow;
		break;

	case SPR_PINS:
		if (!P_GivePower (player, pw_invisibility))
			return;
		player->message = GOTINVIS;
		sound = sfx_getpow;
		break;

	case SPR_SUIT:
		if (!P_GivePower (player, pw_ironfeet))
			return;
		player->message = GOTSUIT;
		sound = sfx_getpow;
		break;

	case SPR_PMAP:
		if (!P_GivePower (player, pw_allmap))
			return;
		player->message = GOTMAP;
		sound = sfx_getpow;
		break;

	case SPR_PVIS:
		if (!P_GivePower (player, pw_infrared))
			return;
		player->message = GOTVISOR;
		sound = sfx_getpow;
		break;

		// ammo
	case SPR_CLIP:
		if (special->flags & MF_DROPPED)
		{
			if (!P_GiveAmmo (player,am_clip,0))
				return;
		}
		else
		{
			if (!P_GiveAmmo (player,am_clip,1))
				return;
		}
		player->message = GOTCLIP;
		break;

	case SPR_AMMO:
		if (!P_GiveAmmo (player, am_clip,5))
			return;
		player->message = GOTCLIPBOX;
		break;

	case SPR_ROCK:
		if (!P_GiveAmmo (player, am_misl,1))
			return;
		player->message = GOTROCKET;
		break;

	case SPR_BROK:
		if (!P_GiveAmmo (player, am_misl,5))
			return;
		player->message = GOTROCKBOX;
		break;

	case SPR_CELL:
		if (!P_GiveAmmo (player, am_cell,1))
			return;
		player->message = GOTCELL;
		break;

	case SPR_CELP:
		if (!P_GiveAmmo (player, am_cell,5))
			return;
		player->message = GOTCELLBOX;
		break;

	case SPR_SHEL:
		if (!P_GiveAmmo (player, am_shell,1))
			return;
		player->message = GOTSHELLS;
		break;

	case SPR_SBOX:
		if (!P_GiveAmmo (player, am_shell,5))
			return;
		player->message = GOTSHELLBOX;
		break;

	case SPR_BPAK:
		if (!player->backpack)
		{
			for (i=0 ; i<NUMAMMO ; i++)
				player->maxammo[i] *= 2;
			player->backpack = true;
		}
		for (i=0 ; i<NUMAMMO ; i++)
			P_GiveAmmo (player, (ammotype_t)i, 1);
		player->message = GOTBACKPACK;
		break;

		// weapons
	case SPR_BFUG:
		if (!P_GiveWeapon (player, wp_bfg, false) )
			return;

		// DHM - Nerve :: Give achievement
		if ( !common->IsMultiplayer() ) {
			switch( DoomLib::GetGameSKU() ) {
				case GAME_SKU_DOOM2_BFG: {
					idAchievementManager::LocalUser_CompleteAchievement( ACHIEVEMENT_DOOM2_REALLY_BIG_GUN_FIND_BFG_SINGLEPLAYER );
				}
				default: {
					// No unlocks for other SKUs.
					break;
				}
			}
		}

		player->message = GOTBFG9000;
		sound = sfx_wpnup;	
		break;

	case SPR_MGUN:
		if (!P_GiveWeapon (player, wp_chaingun, special->flags&MF_DROPPED) )
			return;
		player->message = GOTCHAINGUN;
		sound = sfx_wpnup;	
		break;

	case SPR_CSAW:
		if (!P_GiveWeapon (player, wp_chainsaw, false) )
			return;
		player->message = GOTCHAINSAW;
		sound = sfx_wpnup;	
		break;

	case SPR_LAUN:
		if (!P_GiveWeapon (player, wp_missile, false) )
			return;
		player->message = GOTLAUNCHER;
		sound = sfx_wpnup;	
		break;

	case SPR_PLAS:
		if (!P_GiveWeapon (player, wp_plasma, false) )
			return;
		player->message = GOTPLASMA;
		sound = sfx_wpnup;	
		break;

	case SPR_SHOT:
		if (!P_GiveWeapon (player, wp_shotgun, special->flags&MF_DROPPED ) )
			return;
		player->message = GOTSHOTGUN;
		sound = sfx_wpnup;	
		break;

	case SPR_SGN2:
		if (!P_GiveWeapon (player, wp_supershotgun, special->flags&MF_DROPPED ) )
			return;

		player->message = GOTSHOTGUN2;
		sound = sfx_wpnup;	
		break;

	default:
		I_Error ("P_SpecialThing: Unknown gettable thing");
	}

	if (special->flags & MF_COUNTITEM)
		player->itemcount++;
	P_RemoveMobj (special);
	player->bonuscount += BONUSADD;
	if (player == &::g->players[::g->consoleplayer])
		S_StartSound (player->mo, sound);
}

//
// IsOnlineDeathmatchWithLocalProfile
//
// Helper to simplify the online frag stat tracking. Returns the
// master user's profile if successful, NULL if not.
// 
idPlayerProfile * IsOnlineDeathmatchWithLocalProfile() {
	if ( !MatchTypeIsOnline( session->GetGameLobbyBase().GetMatchParms().matchFlags ) ) {
		return NULL;
	}

	if ( !::g ) {
		return NULL;
	}

	if ( !::g->deathmatch ) {
		return NULL;
	}

	// Assume that the master local user is the one playing.
	idLocalUser * user = session->GetSignInManager().GetMasterLocalUser();
	if ( user == NULL ) {
		return NULL;
	}

	idPlayerProfile * profile = user->GetProfile();

	if ( profile == NULL ) {
		return NULL;
	}
	
	return profile;
}

//
// KillMobj
//
void
P_KillMobj
( mobj_t*	source,
 mobj_t*	target )
{
	mobjtype_t	item;
	mobj_t*	mo;

	target->flags &= ~(MF_SHOOTABLE|MF_FLOAT|MF_SKULLFLY);

	if (target->type != MT_SKULL)
		target->flags &= ~MF_NOGRAVITY;

	target->flags |= MF_CORPSE|MF_DROPOFF;
	target->height >>= 2;

	if (source && source->player)
	{
		// count for intermission
		if (target->flags & MF_COUNTKILL)
			source->player->killcount++;	

		if (target->player) {
			source->player->frags[target->player-::g->players]++;

			// Keep track of the local player's total frags for trophy awards.

			// Make sure the killing player is the local player
			if ( source->player == &(::g->players[::g->consoleplayer]) ) {
				// Make sure this is an online game.
				// TODO: PC
			}
		}

		// DHM - Nerve :: Check for killing cyberdemon with fists achievement
		// JAF TROPHY int port = gameLocal->GetPortForPlayer( DoomLib::GetPlayer() );

		if ( source->player->readyweapon == wp_fist && target->type == MT_CYBORG && !common->IsMultiplayer() ) {
			switch( DoomLib::GetGameSKU() ) {
				case GAME_SKU_DOOM2_BFG: {
					// Removing trophies for DOOM and DOOM II BFG due to point limit.
					//gameLocal->UnlockAchievement( Doom2BFG_Trophies::YOU_HAVE_HUGE_GUTS_KILL_CYBERDEMON_WITH_FISTS );
					break;
				}
				case GAME_SKU_DCC: {
					// Not for PC.
					//session->GetAchievementSystem().AchievementUnlock( session->GetSignInManager().GetMasterLocalUser(), DOOM_ACHIEVEMENT_KILL_CYBER_DEMON_WITH_FISTS );
					break;
				}
				default: {
					// No unlocks for other SKUs.
					break;
				}
			}
		}

		// DHM - Nerve :: Chainsaw kills
		if ( source->player->readyweapon == wp_chainsaw && !common->IsMultiplayer() ) {
			source->player->chainsawKills++;
			if ( source->player->chainsawKills == 20 ) {
				switch( DoomLib::GetGameSKU() ) {
					case GAME_SKU_DOOM2_BFG: {
						// Removing trophies for DOOM and DOOM II BFG due to point limit.
						//gameLocal->UnlockAchievement( Doom2BFG_Trophies::GREAT_COMMUNICATOR_20_CHAINSAW_KILLS );
						break;
					}
					case GAME_SKU_DCC: {
						// Not for PC.
						//gameLocal->UnlockAchievement( DOOM_ACHIEVEMENT_20KILLS_CHAINSAW );
						break;
					}
					default: {
						// No unlocks for other SKUs.
						break;
					}
				}
			}
		}

		// DHM - Nerve :: Berserker kills
		if ( source->player->readyweapon == wp_fist && source->player->powers[pw_strength] && !common->IsMultiplayer()) {
			source->player->berserkKills++;
			idLib::Printf( "Player has %d berserk kills\n", source->player->berserkKills );
			if ( source->player->berserkKills == 20 ) {
				switch( DoomLib::GetGameSKU() ) {
					case GAME_SKU_DOOM2_BFG: {
						// Removing trophies for DOOM and DOOM II BFG due to point limit.
						//gameLocal->UnlockAchievement( Doom2BFG_Trophies::MAN_AND_A_HALF_20_BERSERK_KILLS );
						break;
					}
					case GAME_SKU_DCC: {
						// Not for PC.
						//gameLocal->UnlockAchievement( DOOM_ACHIEVEMENT_20KILLS_BERSERKER );
						break;
					}
					default: {
						// No unlocks for other SKUs.
						break;
					}
				}				
			}
		}
	}
	else if (!::g->netgame && (target->flags & MF_COUNTKILL) )
	{
		// count all monster deaths,
		// even those caused by other monsters
		::g->players[0].killcount++;
	}

	if (target->player)
	{
		// count environment kills against you
		if (!source)	
			target->player->frags[target->player-::g->players]++;

		target->flags &= ~MF_SOLID;
		target->player->playerstate = PST_DEAD;
		P_DropWeapon (target->player);

		if (target->player == &::g->players[::g->consoleplayer]
		&& ::g->automapactive)
		{
			// don't die in auto map,
			// switch view prior to dying
			AM_Stop ();
		}

	}

	if (target->health < -target->info->spawnhealth 
		&& target->info->xdeathstate)
	{
		P_SetMobjState (target, (statenum_t)target->info->xdeathstate);
	}
	else
		P_SetMobjState (target, (statenum_t)target->info->deathstate);
	target->tics -= P_Random()&3;

	if (target->tics < 1)
		target->tics = 1;

	//	I_StartSound (&actor->r, actor->info->deathsound);


	// Drop stuff.
	// This determines the kind of object spawned
	// during the death frame of a thing.
	switch (target->type)
	{
	case MT_WOLFSS:
	case MT_POSSESSED:
		item = MT_CLIP;
		break;

	case MT_SHOTGUY:
		item = MT_SHOTGUN;
		break;

	case MT_CHAINGUY:
		item = MT_CHAINGUN;
		break;

	default:
		return;
	}

	mo = P_SpawnMobj (target->x,target->y,ONFLOORZ, item);
	mo->flags |= MF_DROPPED;	// special versions of items
}




//
// P_DamageMobj
// Damages both enemies and ::g->players
// "inflictor" is the thing that caused the damage
//  creature or missile, can be NULL (slime, etc)
// "source" is the thing to target after taking damage
//  creature or NULL
// Source and inflictor are the same for melee attacks.
// Source can be NULL for slime, barrel explosions
// and other environmental stuff.
//
void
P_DamageMobj
( mobj_t*	target,
 mobj_t*	inflictor,
 mobj_t*	source,
 int 		damage )
{
	unsigned	ang;
	int		saved;
	player_t*	player;
	fixed_t	thrust;
	int		temp;

	if ( !(target->flags & MF_SHOOTABLE) )
		return;	// shouldn't happen...

	if (target->health <= 0)
		return;

	if ( target->flags & MF_SKULLFLY )
	{
		target->momx = target->momy = target->momz = 0;
	}

	player = target->player;
	if (player && ::g->gameskill == sk_baby)
		damage >>= 1; 	// take half damage in trainer mode


	// Some close combat weapons should not
	// inflict thrust and push the victim out of reach,
	// thus kick away unless using the chainsaw.
	if (inflictor
		&& !(target->flags & MF_NOCLIP)
		&& (!source
		|| !source->player
		|| source->player->readyweapon != wp_chainsaw))
	{
		ang = R_PointToAngle2 ( inflictor->x,
			inflictor->y,
			target->x,
			target->y);

		thrust = damage*(FRACUNIT>>3)*100/target->info->mass;

		// make fall forwards sometimes
		if ( damage < 40
			&& damage > target->health
			&& target->z - inflictor->z > 64*FRACUNIT
			&& (P_Random ()&1) )
		{
			ang += ANG180;
			thrust *= 4;
		}

		ang >>= ANGLETOFINESHIFT;
		target->momx += FixedMul (thrust, finecosine[ang]);
		target->momy += FixedMul (thrust, finesine[ang]);
	}

	// player specific
	if (player)
	{	

		// end of game hell hack
		if (target->subsector->sector->special == 11
			&& damage >= target->health)
		{
			damage = target->health - 1;
		}

		float baseShake_High = 0.5f;
		int baseShake_High_Dur = 100;
		float baseShake_Low = 0.5f;
		int baseShake_Low_Dur = 100;
		int damageClamp = Min( damage, 100 ); 
		float damageFloat = std::min( (float)damageClamp / 100.0f, 100.0f );
		float additional = 0.5f * damageFloat;
		int additional_time = 500.0f * damageFloat;

		if( ::g->plyr == player ) {
		}


		// Below certain threshold,
		// ignore damage in GOD mode, or with INVUL power.
		if ( damage < 1000
			&& ( (player->cheats&CF_GODMODE)
			|| player->powers[pw_invulnerability] ) )
		{
			return;
		}


		if (player->armortype)
		{
			if (player->armortype == 1)
				saved = damage/3;
			else
				saved = damage/2;

			if (player->armorpoints <= saved)
			{
				// armor is used up
				saved = player->armorpoints;
				player->armortype = 0;
			}
			player->armorpoints -= saved;
			damage -= saved;
		}
		player->health -= damage; 	// mirror mobj health here for Dave
		if (player->health < 0)
			player->health = 0;

		player->attacker = source;
		player->damagecount += damage;	// add damage after armor / invuln

		if (player->damagecount > 100)
			player->damagecount = 100;	// teleport stomp does 10k points...

		temp = damage < 100 ? damage : 100;
	}

	// do the damage	
	target->health -= damage;	
	if (target->health <= 0)
	{
		P_KillMobj (source, target);
		return;
	}

	if ( (P_Random () < target->info->painchance)
		&& !(target->flags&MF_SKULLFLY) )
	{
		target->flags |= MF_JUSTHIT;	// fight back!

		P_SetMobjState (target, (statenum_t)target->info->painstate);
	}

	target->reactiontime = 0;		// we're awake now...	

	if ( (!target->threshold || target->type == MT_VILE)
		&& source && source != target
		&& source->type != MT_VILE)
	{
		// if not intent on another player,
		// chase after this one
		target->target = source;
		target->threshold = BASETHRESHOLD;
		if (target->state == &::g->states[target->info->spawnstate]
		&& target->info->seestate != S_NULL)
			P_SetMobjState (target, (statenum_t)target->info->seestate);
	}

}


