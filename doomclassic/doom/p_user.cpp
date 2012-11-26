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


#include "doomdef.h"
#include "d_event.h"

#include "p_local.h"

#include "doomstat.h"



// Index of the special effects (INVUL inverse) map.


//
// Movement.
//

// 16 pixels of bob



//
// P_Thrust
// Moves the given origin along a given angle.
//
void
P_Thrust
( player_t*	player,
 angle_t	angle,
 fixed_t	move ) 
{
	angle >>= ANGLETOFINESHIFT;

	player->mo->momx += FixedMul(move,finecosine[angle]); 
	player->mo->momy += FixedMul(move,finesine[angle]);
}




//
// P_CalcHeight
// Calculate the walking / running height adjustment
//
void P_CalcHeight (player_t* player) 
{
	int		angle;
	fixed_t	bob;

	// Regular movement bobbing
	// (needs to be calculated for gun swing
	// even if not on ground)
	// OPTIMIZE: tablify angle
	// Note: a LUT allows for effects
	//  like a ramp with low health.
	player->bob =
		FixedMul (player->mo->momx, player->mo->momx)
		+ FixedMul (player->mo->momy,player->mo->momy);

	player->bob >>= 2;

	// DHM - NERVE :: player bob reduced by 25%, MAXBOB reduced by 25% as well
	player->bob = (fixed_t)( (float)(player->bob) * 0.75f );
	if (player->bob>MAXBOB)
		player->bob = MAXBOB;

	if ((player->cheats & CF_NOMOMENTUM) || !::g->onground)
	{
		player->viewz = player->mo->z + VIEWHEIGHT;

		if (player->viewz > player->mo->ceilingz-4*FRACUNIT)
			player->viewz = player->mo->ceilingz-4*FRACUNIT;

		player->viewz = player->mo->z + player->viewheight;
		return;
	}

	angle = (FINEANGLES/20*::g->leveltime)&FINEMASK;
	bob = FixedMul ( player->bob/2, finesine[angle]);


	// move ::g->viewheight
	if (player->playerstate == PST_LIVE)
	{
		player->viewheight += player->deltaviewheight;

		if (player->viewheight > VIEWHEIGHT)
		{
			player->viewheight = VIEWHEIGHT;
			player->deltaviewheight = 0;
		}

		if (player->viewheight < VIEWHEIGHT/2)
		{
			player->viewheight = VIEWHEIGHT/2;
			if (player->deltaviewheight <= 0)
				player->deltaviewheight = 1;
		}

		if (player->deltaviewheight)	
		{
			player->deltaviewheight += FRACUNIT/4;
			if (!player->deltaviewheight)
				player->deltaviewheight = 1;
		}
	}
	player->viewz = player->mo->z + player->viewheight + bob;

	if (player->viewz > player->mo->ceilingz-4*FRACUNIT)
		player->viewz = player->mo->ceilingz-4*FRACUNIT;
}



//
// P_MovePlayer
//
void P_MovePlayer (player_t* player)
{
	ticcmd_t*		cmd;

	cmd = &player->cmd;

	player->mo->angle += (cmd->angleturn<<16);

	// Do not let the player control movement
	//  if not ::g->onground.
	::g->onground = (player->mo->z <= player->mo->floorz);

	if (cmd->forwardmove && ::g->onground)
		P_Thrust (player, player->mo->angle, cmd->forwardmove*2048);

	if (cmd->sidemove && ::g->onground)
		P_Thrust (player, player->mo->angle-ANG90, cmd->sidemove*2048);

	if ( (cmd->forwardmove || cmd->sidemove) 
		&& player->mo->state == &::g->states[S_PLAY] )
	{
		P_SetMobjState (player->mo, S_PLAY_RUN1);
	}
}	



//
// P_DeathThink
// Fall on your face when dying.
// Decrease POV height to floor height.
//
extern byte demoversion;

void P_DeathThink (player_t* player)
{
	angle_t		angle;
	angle_t		delta;

	P_MovePsprites (player);

	// fall to the ground
	if (player->viewheight > 6*FRACUNIT)
		player->viewheight -= FRACUNIT;

	if (player->viewheight < 6*FRACUNIT)
		player->viewheight = 6*FRACUNIT;

	player->deltaviewheight = 0;
	::g->onground = (player->mo->z <= player->mo->floorz);
	P_CalcHeight (player);

	if (player->attacker && player->attacker != player->mo)
	{
		angle = R_PointToAngle2 (player->mo->x,
			player->mo->y,
			player->attacker->x,
			player->attacker->y);

		delta = angle - player->mo->angle;

		if (delta < ANG5 || delta > (unsigned)-ANG5)
		{
			// Looking at killer,
			//  so fade damage flash down.
			player->mo->angle = angle;

			if (player->damagecount)
				player->damagecount--;
		}
		else if (delta < ANG180)
			player->mo->angle += ANG5;
		else
			player->mo->angle -= ANG5;
	}
	else if (player->damagecount)
		player->damagecount--;


	if (player->cmd.buttons & BT_USE)
		player->playerstate = PST_REBORN;
}



//
// P_PlayerThink
//
void P_PlayerThink (player_t* player)
{
	ticcmd_t*		cmd;
	weapontype_t	newweapon = wp_fist;

	// fixme: do this in the cheat code
	if (player->cheats & CF_NOCLIP)
		player->mo->flags |= MF_NOCLIP;
	else
		player->mo->flags &= ~MF_NOCLIP;

	// chain saw run forward
	cmd = &player->cmd;
	if (player->mo->flags & MF_JUSTATTACKED)
	{
		cmd->angleturn = 0;
		cmd->forwardmove = 0xc800/512;
		cmd->sidemove = 0;
		player->mo->flags &= ~MF_JUSTATTACKED;
	}


	if (player->playerstate == PST_DEAD)
	{
		P_DeathThink (player);
		return;
	}

	// Move around.
	// Reactiontime is used to prevent movement
	//  for a bit after a teleport.
	if (player->mo->reactiontime)
		player->mo->reactiontime--;
	else
		P_MovePlayer (player);

	P_CalcHeight (player);

	if (player->mo->subsector->sector->special)
		P_PlayerInSpecialSector (player);

	// Check for weapon change.

	// A special event has no other buttons.
	if (cmd->buttons & BT_SPECIAL)
		cmd->buttons = 0;			

	if (::g->demoplayback && demoversion < VERSION )
	{
		if ( cmd->buttons & BT_CHANGE)
		{
			// The actual changing of the weapon is done
			//  when the weapon psprite can do it
			//  (read: not in the middle of an attack).
			newweapon = (weapontype_t)((cmd->buttons&BT_WEAPONMASK)>>BT_WEAPONSHIFT);

			if (newweapon == wp_fist
				&& player->weaponowned[wp_chainsaw]
			&& !(player->readyweapon == wp_chainsaw
				&& player->powers[pw_strength]))
			{
				newweapon = wp_chainsaw;
			}

			if ( (::g->gamemode == commercial)
				&& newweapon == wp_shotgun 
				&& player->weaponowned[wp_supershotgun]
			&& player->readyweapon != wp_supershotgun)
			{
				newweapon = wp_supershotgun;
			}


			if (player->weaponowned[newweapon]
			&& newweapon != player->readyweapon)
			{
				// Do not go to plasma or BFG in shareware,
				//  even if cheated.
				if ((newweapon != wp_plasma
					&& newweapon != wp_bfg)
					|| (::g->gamemode != shareware) )
				{
					player->pendingweapon = newweapon;
				}
			}
		}
	}
	else if ( cmd->buttons & BT_CHANGE )
	{ 
		int k, which;
		// The actual changing of the weapon is done
		//  when the weapon psprite can do it
		//  (read: not in the middle of an attack).
		which = ((cmd->buttons&BT_WEAPONMASK)>>BT_WEAPONSHIFT);

		if ( cmd->nextPrevWeapon > 0) {
			newweapon = player->readyweapon;

			for ( k = 0; k < NUMWEAPONS; ++k) 
			{
				newweapon = (weapontype_t)( (cmd->nextPrevWeapon - 1) ? (newweapon + 1) : (newweapon - 1));

				if (newweapon == wp_nochange)
					continue;

				weapontype_t maxweapon = (::g->gamemode == retail) ? wp_chainsaw : wp_supershotgun;

				if (newweapon < 0)
					newweapon = maxweapon;

				if (newweapon > maxweapon)
					newweapon = wp_fist;
				

				if (player->weaponowned[newweapon] && newweapon != player->readyweapon)
				{
					player->pendingweapon = newweapon;
					break;
				}
			}
		}
		else {

			newweapon = (weapontype_t)((cmd->buttons&BT_WEAPONMASK)>>BT_WEAPONSHIFT);

			if (newweapon == wp_fist
				&& player->weaponowned[wp_chainsaw]
			&& !(player->readyweapon == wp_chainsaw
				&& player->powers[pw_strength]))
			{
				newweapon = wp_chainsaw;
			}

			if ( (::g->gamemode == commercial)
				&& newweapon == wp_shotgun 
				&& player->weaponowned[wp_supershotgun]
			&& player->readyweapon != wp_supershotgun)
			{
				newweapon = wp_supershotgun;
			}

			if ( player->weaponowned[ newweapon ] && newweapon != player->readyweapon ) {
				player->pendingweapon = newweapon;
			}
		}
	}

	// check for use
	if (cmd->buttons & BT_USE)
	{
		if (!player->usedown)
		{
			P_UseLines (player);
			player->usedown = true;
		}
	}
	else
		player->usedown = false;

	// cycle psprites
	P_MovePsprites (player);

	// Counters, time dependend power ups.

	// Strength counts up to diminish fade.
	if (player->powers[pw_strength])
		player->powers[pw_strength]++;	

	if (player->powers[pw_invulnerability])
		player->powers[pw_invulnerability]--;

	if (player->powers[pw_invisibility])
		if (! --player->powers[pw_invisibility] )
			player->mo->flags &= ~MF_SHADOW;

	if (player->powers[pw_infrared])
		player->powers[pw_infrared]--;

	if (player->powers[pw_ironfeet])
		player->powers[pw_ironfeet]--;

	if (player->damagecount)
		player->damagecount--;

	if (player->bonuscount)
		player->bonuscount--;


	// Handling ::g->colormaps.
	if (player->powers[pw_invulnerability])
	{
		if (player->powers[pw_invulnerability] > 4*32
			|| (player->powers[pw_invulnerability]&8) )
			player->fixedcolormap = INVERSECOLORMAP;
		else
			player->fixedcolormap = 0;
	}
	else if (player->powers[pw_infrared])	
	{
		if (player->powers[pw_infrared] > 4*32
			|| (player->powers[pw_infrared]&8) )
		{
			// almost full bright
			player->fixedcolormap = 1;
		}
		else
			player->fixedcolormap = 0;
	}
	else
		player->fixedcolormap = 0;
}



