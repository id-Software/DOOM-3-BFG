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
#include "Main.h"

#include <stdlib.h>

#include "doomdef.h"
#include "doomstat.h"

#include "i_system.h"
#include "z_zone.h"
#include "m_argv.h"
#include "m_random.h"
#include "w_wad.h"

#include "r_local.h"
#include "p_local.h"

#include "g_game.h"

#include "s_sound.h"

// State.
#include "r_state.h"

// Data.
#include "sounds.h"

#include "../../neo/d3xp/Game_local.h"

//
// Animating textures and planes
// There is another anim_t used in wi_stuff, unrelated. BLAH!
// we now use anim_t2
//

//
//      source animation definition
//





//
// P_InitPicAnims
//

// Floor/ceiling animation sequences,
//  defined by first and last frame,
//  i.e. the flat (64x64 tile) name to
//  be used.
// The full animation sequence is given
//  using all the flats between the start
//  and end entry, in the order found in
//  the WAD file.
//
const animdef_t		animdefs[] =
{
	{false,	"NUKAGE3",	"NUKAGE1",	8},
	{false,	"FWATER4",	"FWATER1",	8},
	{false,	"SWATER4",	"SWATER1", 	8},
	{false,	"LAVA4",	"LAVA1",	8},
	{false,	"BLOOD3",	"BLOOD1",	8},

	// DOOM II flat animations.
	{false,	"RROCK08",	"RROCK05",	8},		
	{false,	"SLIME04",	"SLIME01",	8},
	{false,	"SLIME08",	"SLIME05",	8},
	{false,	"SLIME12",	"SLIME09",	8},

	{true,	"BLODGR4",	"BLODGR1",	8},
	{true,	"SLADRIP3",	"SLADRIP1",	8},

	{true,	"BLODRIP4",	"BLODRIP1",	8},
	{true,	"FIREWALL",	"FIREWALA",	8},
	{true,	"GSTFONT3",	"GSTFONT1",	8},
	{true,	"FIRELAVA",	"FIRELAV3",	8},
	{true,	"FIREMAG3",	"FIREMAG1",	8},
	{true,	"FIREBLU2",	"FIREBLU1",	8},
	{true,	"ROCKRED3",	"ROCKRED1",	8},

	{true,	"BFALL4",	"BFALL1",	8},
	{true,	"SFALL4",	"SFALL1",	8},
	{true,	"WFALL4",	"WFALL1",	8},
	{true,	"DBRAIN4",	"DBRAIN1",	8},

	{-1}
};



//
//      Animating line specials
//




void P_InitPicAnims (void)
{
	int		i;


	//	Init animation
	::g->lastanim = ::g->anims;
	for (i=0 ; animdefs[i].istexture != (qboolean)-1 ; i++)
	{
		if (animdefs[i].istexture)
		{
			// different episode ?
			if (R_CheckTextureNumForName(animdefs[i].startname) == -1)
				continue;	

			::g->lastanim->picnum = R_TextureNumForName (animdefs[i].endname);
			::g->lastanim->basepic = R_TextureNumForName (animdefs[i].startname);
		}
		else
		{
			if (W_CheckNumForName(animdefs[i].startname) == -1)
				continue;

			::g->lastanim->picnum = R_FlatNumForName (animdefs[i].endname);
			::g->lastanim->basepic = R_FlatNumForName (animdefs[i].startname);
		}

		::g->lastanim->istexture = animdefs[i].istexture;
		::g->lastanim->numpics = ::g->lastanim->picnum - ::g->lastanim->basepic + 1;

		if (::g->lastanim->numpics < 2)
			I_Error ("P_InitPicAnims: bad cycle from %s to %s",
			animdefs[i].startname,
			animdefs[i].endname);

		::g->lastanim->speed = animdefs[i].speed;
		::g->lastanim++;
	}

}



//
// UTILITIES
//



//
// getSide()
// Will return a side_t*
//  given the number of the current sector,
//  the line number, and the side (0/1) that you want.
//
side_t*
getSide
( int		currentSector,
 int		line,
 int		side )
{
	return &::g->sides[ (::g->sectors[currentSector].lines[line])->sidenum[side] ];
}


//
// getSector()
// Will return a sector_t*
//  given the number of the current sector,
//  the line number and the side (0/1) that you want.
//
sector_t*
getSector
( int		currentSector,
 int		line,
 int		side )
{
	return ::g->sides[ (::g->sectors[currentSector].lines[line])->sidenum[side] ].sector;
}


//
// twoSided()
// Given the sector number and the line number,
//  it will tell you whether the line is two-sided or not.
//
int
twoSided
( int	sector,
 int	line )
{
	return (::g->sectors[sector].lines[line])->flags & ML_TWOSIDED;
}




//
// getNextSector()
// Return sector_t * of sector next to current.
// NULL if not two-sided line
//
sector_t*
getNextSector
( line_t*	line,
 sector_t*	sec )
{
	if (!(line->flags & ML_TWOSIDED))
		return NULL;

	if (line->frontsector == sec)
		return line->backsector;

	return line->frontsector;
}



//
// P_FindLowestFloorSurrounding()
// FIND LOWEST FLOOR HEIGHT IN SURROUNDING SECTORS
//
fixed_t	P_FindLowestFloorSurrounding(sector_t* sec)
{
	int			i;
	line_t*		check;
	sector_t*		other;
	fixed_t		floor = sec->floorheight;

	for (i=0 ;i < sec->linecount ; i++)
	{
		check = sec->lines[i];
		other = getNextSector(check,sec);

		if (!other)
			continue;

		if (other->floorheight < floor)
			floor = other->floorheight;
	}
	return floor;
}



//
// P_FindHighestFloorSurrounding()
// FIND HIGHEST FLOOR HEIGHT IN SURROUNDING SECTORS
//
fixed_t	P_FindHighestFloorSurrounding(sector_t *sec)
{
	int			i;
	line_t*		check;
	sector_t*		other;
	fixed_t		floor = -500*FRACUNIT;

	for (i=0 ;i < sec->linecount ; i++)
	{
		check = sec->lines[i];
		other = getNextSector(check,sec);

		if (!other)
			continue;

		if (other->floorheight > floor)
			floor = other->floorheight;
	}
	return floor;
}



//
// P_FindNextHighestFloor
// FIND NEXT HIGHEST FLOOR IN SURROUNDING SECTORS
// Note: this should be doable w/o a fixed array.

// 20 adjoining ::g->sectors max!

fixed_t
P_FindNextHighestFloor
( sector_t*	sec,
 int		currentheight )
{
	int			i;
	int			h;
	int			min;
	line_t*		check;
	sector_t*		other;
	fixed_t		height = currentheight;


	fixed_t		heightlist[MAX_ADJOINING_SECTORS];		

	for (i=0, h=0 ;i < sec->linecount ; i++)
	{
		check = sec->lines[i];
		other = getNextSector(check,sec);

		if (!other)
			continue;

		if (other->floorheight > height)
			heightlist[h++] = other->floorheight;

		// Check for overflow. Exit.
		if ( h >= MAX_ADJOINING_SECTORS )
		{
			I_PrintfE("Sector with more than 20 adjoining sectors\n" );
			break;
		}
	}

	// Find lowest height in list
	if (!h)
		return currentheight;

	min = heightlist[0];

	// Range checking? 
	for (i = 1;i < h;i++)
		if (heightlist[i] < min)
			min = heightlist[i];

	return min;
}


//
// FIND LOWEST CEILING IN THE SURROUNDING SECTORS
//
fixed_t
P_FindLowestCeilingSurrounding(sector_t* sec)
{
	int			i;
	line_t*		check;
	sector_t*		other;
	fixed_t		height = MAXINT;

	for (i=0 ;i < sec->linecount ; i++)
	{
		check = sec->lines[i];
		other = getNextSector(check,sec);

		if (!other)
			continue;

		if (other->ceilingheight < height)
			height = other->ceilingheight;
	}
	return height;
}


//
// FIND HIGHEST CEILING IN THE SURROUNDING SECTORS
//
fixed_t	P_FindHighestCeilingSurrounding(sector_t* sec)
{
	int		i;
	line_t*	check;
	sector_t*	other;
	fixed_t	height = 0;

	for (i=0 ;i < sec->linecount ; i++)
	{
		check = sec->lines[i];
		other = getNextSector(check,sec);

		if (!other)
			continue;

		if (other->ceilingheight > height)
			height = other->ceilingheight;
	}
	return height;
}



//
// RETURN NEXT SECTOR # THAT LINE TAG REFERS TO
//
int
P_FindSectorFromLineTag
( line_t*	line,
 int		start )
{
	int	i;

	for (i = start+1; i < ::g->numsectors; i++)
		if (::g->sectors[i].tag == line->tag)
			return i;

	return -1;
}




//
// Find minimum light from an adjacent sector
//
int
P_FindMinSurroundingLight
( sector_t*	sector,
 int		max )
{
	int		i;
	int		min;
	line_t*	line;
	sector_t*	check;

	min = max;
	for (i=0 ; i < sector->linecount ; i++)
	{
		line = sector->lines[i];
		check = getNextSector(line,sector);

		if (!check)
			continue;

		if (check->lightlevel < min)
			min = check->lightlevel;
	}
	return min;
}



//
// EVENTS
// Events are operations triggered by using, crossing,
// or shooting special ::g->lines, or by timed thinkers.
//

//
// P_CrossSpecialLine - TRIGGER
// Called every time a thing origin is about
//  to cross a line with a non 0 special.
//
void
P_CrossSpecialLine
( int		linenum,
 int		side,
 mobj_t*	thing )
{
	line_t*	line;
	int		ok;

	line = &::g->lines[linenum];

	//	Triggers that other things can activate
	if (!thing->player)
	{
		// Things that should NOT trigger specials...
		switch(thing->type)
		{
		case MT_ROCKET:
		case MT_PLASMA:
		case MT_BFG:
		case MT_TROOPSHOT:
		case MT_HEADSHOT:
		case MT_BRUISERSHOT:
			return;
			break;

		default: break;
		}

		ok = 0;
		switch(line->special)
		{
		case 39:	// TELEPORT TRIGGER
		case 97:	// TELEPORT RETRIGGER
		case 125:	// TELEPORT MONSTERONLY TRIGGER
		case 126:	// TELEPORT MONSTERONLY RETRIGGER
		case 4:	// RAISE DOOR
		case 10:	// PLAT DOWN-WAIT-UP-STAY TRIGGER
		case 88:	// PLAT DOWN-WAIT-UP-STAY RETRIGGER
			ok = 1;
			break;
		}
		if (!ok)
			return;
	}


	// Note: could use some const's here.
	switch (line->special)
	{
		// TRIGGERS.
		// All from here to RETRIGGERS.
	case 2:
		// Open Door
		EV_DoDoor(line,opened);
		line->special = 0;
		break;

	case 3:
		// Close Door
		EV_DoDoor(line,closed);
		line->special = 0;
		break;

	case 4:
		// Raise Door
		EV_DoDoor(line,normal);
		line->special = 0;
		break;

	case 5:
		// Raise Floor
		EV_DoFloor(line,raiseFloor);
		line->special = 0;
		break;

	case 6:
		// Fast Ceiling Crush & Raise
		EV_DoCeiling(line,fastCrushAndRaise);
		line->special = 0;
		break;

	case 8:
		// Build Stairs
		EV_BuildStairs(line,build8);
		line->special = 0;
		break;

	case 10:
		// PlatDownWaitUp
		EV_DoPlat(line,downWaitUpStay,0);
		line->special = 0;
		break;

	case 12:
		// Light Turn On - brightest near
		EV_LightTurnOn(line,0);
		line->special = 0;
		break;

	case 13:
		// Light Turn On 255
		EV_LightTurnOn(line,255);
		line->special = 0;
		break;

	case 16:
		// Close Door 30
		EV_DoDoor(line,close30ThenOpen);
		line->special = 0;
		break;

	case 17:
		// Start Light Strobing
		EV_StartLightStrobing(line);
		line->special = 0;
		break;

	case 19:
		// Lower Floor
		EV_DoFloor(line,lowerFloor);
		line->special = 0;
		break;

	case 22:
		// Raise floor to nearest height and change texture
		EV_DoPlat(line,raiseToNearestAndChange,0);
		line->special = 0;
		break;

	case 25:
		// Ceiling Crush and Raise
		EV_DoCeiling(line,crushAndRaise);
		line->special = 0;
		break;

	case 30:
		// Raise floor to shortest texture height
		//  on either side of ::g->lines.
		EV_DoFloor(line,raiseToTexture);
		line->special = 0;
		break;

	case 35:
		// Lights Very Dark
		EV_LightTurnOn(line,35);
		line->special = 0;
		break;

	case 36:
		// Lower Floor (TURBO)
		EV_DoFloor(line,turboLower);
		line->special = 0;
		break;

	case 37:
		// LowerAndChange
		EV_DoFloor(line,lowerAndChange);
		line->special = 0;
		break;

	case 38:
		// Lower Floor To Lowest
		EV_DoFloor( line, lowerFloorToLowest );
		line->special = 0;
		break;

	case 39:
		// TELEPORT!
		EV_Teleport( line, side, thing );
		line->special = 0;
		break;

	case 40:
		// RaiseCeilingLowerFloor
		EV_DoCeiling( line, raiseToHighest );
		EV_DoFloor( line, lowerFloorToLowest );
		line->special = 0;
		break;

	case 44:
		// Ceiling Crush
		EV_DoCeiling( line, lowerAndCrush );
		line->special = 0;
		break;

	case 52:
		// EXIT!
		// DHM - Nerve :: Don't exit level in death match, timelimit and fraglimit only
		if ( !::g->deathmatch && ::g->gameaction != ga_completed ) {
			G_ExitLevel();
		}
		break;

	case 53:
		// Perpetual Platform Raise
		EV_DoPlat(line,perpetualRaise,0);
		line->special = 0;
		break;

	case 54:
		// Platform Stop
		EV_StopPlat(line);
		line->special = 0;
		break;

	case 56:
		// Raise Floor Crush
		EV_DoFloor(line,raiseFloorCrush);
		line->special = 0;
		break;

	case 57:
		// Ceiling Crush Stop
		EV_CeilingCrushStop(line);
		line->special = 0;
		break;

	case 58:
		// Raise Floor 24
		EV_DoFloor(line,raiseFloor24);
		line->special = 0;
		break;

	case 59:
		// Raise Floor 24 And Change
		EV_DoFloor(line,raiseFloor24AndChange);
		line->special = 0;
		break;

	case 104:
		// Turn lights off in sector(tag)
		EV_TurnTagLightsOff(line);
		line->special = 0;
		break;

	case 108:
		// Blazing Door Raise (faster than TURBO!)
		EV_DoDoor (line,blazeRaise);
		line->special = 0;
		break;

	case 109:
		// Blazing Door Open (faster than TURBO!)
		EV_DoDoor (line,blazeOpen);
		line->special = 0;
		break;

	case 100:
		// Build Stairs Turbo 16
		EV_BuildStairs(line,turbo16);
		line->special = 0;
		break;

	case 110:
		// Blazing Door Close (faster than TURBO!)
		EV_DoDoor (line,blazeClose);
		line->special = 0;
		break;

	case 119:
		// Raise floor to nearest surr. floor
		EV_DoFloor(line,raiseFloorToNearest);
		line->special = 0;
		break;

	case 121:
		// Blazing PlatDownWaitUpStay
		EV_DoPlat(line,blazeDWUS,0);
		line->special = 0;
		break;

	case 124:
		// Secret EXIT
		if ( !::g->deathmatch && ::g->gameaction != ga_completed ) {
			G_SecretExitLevel ();
		}
		break;

	case 125:
		// TELEPORT MonsterONLY
		if (!thing->player)
		{
			EV_Teleport( line, side, thing );
			line->special = 0;
		}
		break;

	case 130:
		// Raise Floor Turbo
		EV_DoFloor(line,raiseFloorTurbo);
		line->special = 0;
		break;

	case 141:
		// Silent Ceiling Crush & Raise
		EV_DoCeiling(line,silentCrushAndRaise);
		line->special = 0;
		break;

		// RETRIGGERS.  All from here till end.
	case 72:
		// Ceiling Crush
		EV_DoCeiling( line, lowerAndCrush );
		break;

	case 73:
		// Ceiling Crush and Raise
		EV_DoCeiling(line,crushAndRaise);
		break;

	case 74:
		// Ceiling Crush Stop
		EV_CeilingCrushStop(line);
		break;

	case 75:
		// Close Door
		EV_DoDoor(line,closed);
		break;

	case 76:
		// Close Door 30
		EV_DoDoor(line,close30ThenOpen);
		break;

	case 77:
		// Fast Ceiling Crush & Raise
		EV_DoCeiling(line,fastCrushAndRaise);
		break;

	case 79:
		// Lights Very Dark
		EV_LightTurnOn(line,35);
		break;

	case 80:
		// Light Turn On - brightest near
		EV_LightTurnOn(line,0);
		break;

	case 81:
		// Light Turn On 255
		EV_LightTurnOn(line,255);
		break;

	case 82:
		// Lower Floor To Lowest
		EV_DoFloor( line, lowerFloorToLowest );
		break;

	case 83:
		// Lower Floor
		EV_DoFloor(line,lowerFloor);
		break;

	case 84:
		// LowerAndChange
		EV_DoFloor(line,lowerAndChange);
		break;

	case 86:
		// Open Door
		EV_DoDoor(line,opened);
		break;

	case 87:
		// Perpetual Platform Raise
		EV_DoPlat(line,perpetualRaise,0);
		break;

	case 88:
		// PlatDownWaitUp
		EV_DoPlat(line,downWaitUpStay,0);
		break;

	case 89:
		// Platform Stop
		EV_StopPlat(line);
		break;

	case 90:
		// Raise Door
		EV_DoDoor(line,normal);
		break;

	case 91:
		// Raise Floor
		EV_DoFloor(line,raiseFloor);
		break;

	case 92:
		// Raise Floor 24
		EV_DoFloor(line,raiseFloor24);
		break;

	case 93:
		// Raise Floor 24 And Change
		EV_DoFloor(line,raiseFloor24AndChange);
		break;

	case 94:
		// Raise Floor Crush
		EV_DoFloor(line,raiseFloorCrush);
		break;

	case 95:
		// Raise floor to nearest height
		// and change texture.
		EV_DoPlat(line,raiseToNearestAndChange,0);
		break;

	case 96:
		// Raise floor to shortest texture height
		// on either side of ::g->lines.
		EV_DoFloor(line,raiseToTexture);
		break;

	case 97:
		// TELEPORT!
		EV_Teleport( line, side, thing );
		break;

	case 98:
		// Lower Floor (TURBO)
		EV_DoFloor(line,turboLower);
		break;

	case 105:
		// Blazing Door Raise (faster than TURBO!)
		EV_DoDoor (line,blazeRaise);
		break;

	case 106:
		// Blazing Door Open (faster than TURBO!)
		EV_DoDoor (line,blazeOpen);
		break;

	case 107:
		// Blazing Door Close (faster than TURBO!)
		EV_DoDoor (line,blazeClose);
		break;

	case 120:
		// Blazing PlatDownWaitUpStay.
		EV_DoPlat(line,blazeDWUS,0);
		break;

	case 126:
		// TELEPORT MonsterONLY.
		if (!thing->player)
			EV_Teleport( line, side, thing );
		break;

	case 128:
		// Raise To Nearest Floor
		EV_DoFloor(line,raiseFloorToNearest);
		break;

	case 129:
		// Raise Floor Turbo
		EV_DoFloor(line,raiseFloorTurbo);
		break;
	}
}



//
// P_ShootSpecialLine - IMPACT SPECIALS
// Called when a thing shoots a special line.
//
void
P_ShootSpecialLine
( mobj_t*	thing,
 line_t*	line )
{
	int		ok;

	//	Impacts that other things can activate.
	if (!thing->player)
	{
		ok = 0;
		switch(line->special)
		{
		case 46:
			// OPEN DOOR IMPACT
			ok = 1;
			break;
		}
		if (!ok)
			return;
	}

	switch(line->special)
	{
	case 24:
		// RAISE FLOOR
		EV_DoFloor(line,raiseFloor);
		P_ChangeSwitchTexture(line,0);
		break;

	case 46:
		// OPEN DOOR
		EV_DoDoor(line,opened);
		P_ChangeSwitchTexture(line,1);
		break;

	case 47:
		// RAISE FLOOR NEAR AND CHANGE
		EV_DoPlat(line,raiseToNearestAndChange,0);
		P_ChangeSwitchTexture(line,0);
		break;
	}
}



//
// P_PlayerInSpecialSector
// Called every tic frame
//  that the player origin is in a special sector
//
void P_PlayerInSpecialSector (player_t* player)
{
	sector_t*	sector;

	sector = player->mo->subsector->sector;

	// Falling, not all the way down yet?
	if (player->mo->z != sector->floorheight)
		return;	

	// Has hitten ground.
	switch (sector->special)
	{
	case 5:
		// HELLSLIME DAMAGE
		if (!player->powers[pw_ironfeet])
			if (!(::g->leveltime&0x1f))
				P_DamageMobj (player->mo, NULL, NULL, 10);
		break;

	case 7:
		// NUKAGE DAMAGE
		if (!player->powers[pw_ironfeet])
			if (!(::g->leveltime&0x1f))
				P_DamageMobj (player->mo, NULL, NULL, 5);
		break;

	case 16:
		// SUPER HELLSLIME DAMAGE
	case 4:
		// STROBE HURT
		if (!player->powers[pw_ironfeet]
		|| (P_Random()<5) )
		{
			if (!(::g->leveltime&0x1f))
				P_DamageMobj (player->mo, NULL, NULL, 20);
		}
		break;

	case 9:
		// SECRET SECTOR
		player->secretcount++;
		sector->special = 0;


		if ( !::g->demoplayback && ( ::g->usergame && !::g->netgame ) ) {
			// DHM - Nerve :: Let's give achievements in real time in Doom 2
			if ( !common->IsMultiplayer() ) {
				switch( DoomLib::GetGameSKU() ) {
					case GAME_SKU_DOOM1_BFG: {
						// Removing trophies for DOOM and DOOM II BFG due to point limit.
						//gameLocal->UnlockAchievement( Doom1BFG_Trophies::SCOUT_FIND_ANY_SECRET );
						break;
					}
					case GAME_SKU_DOOM2_BFG: {
						//gameLocal->UnlockAchievement( Doom2BFG_Trophies::IMPORTANT_LOOKING_DOOR_FIND_ANY_SECRET );
						idAchievementManager::LocalUser_CompleteAchievement(ACHIEVEMENT_DOOM2_IMPORTANT_LOOKING_DOOR_FIND_ANY_SECRET );
						break;
					}
					case GAME_SKU_DCC: {
						// Not on PC.
						//gameLocal->UnlockAchievement( DOOM_ACHIEVEMENT_FIND_SECRET );
						break;
					}
					default: {
						// No unlocks for other SKUs.
						break;
					}
				}
			}
		}


		break;

	case 11:
		// EXIT SUPER DAMAGE! (for E1M8 finale)
		player->cheats &= ~CF_GODMODE;

		if (!(::g->leveltime&0x1f))
			P_DamageMobj (player->mo, NULL, NULL, 20);

		if (player->health <= 10)
			G_ExitLevel();
		break;

	default:
		I_Error ("P_PlayerInSpecialSector: "
			"unknown special %i",
			sector->special);
		break;
	};
}




//
// P_UpdateSpecials
// Animate planes, scroll walls, etc.
//
int PlayerFrags( int playernum ) {
	int	frags = 0;

	for( int i=0 ; i<MAXPLAYERS ; i++) {
		if ( i != playernum ) {
			frags += ::g->players[playernum].frags[i];
		}
	}

	frags -= ::g->players[playernum].frags[playernum];

	return frags;
}

void P_UpdateSpecials (void)
{
	anim_t2*	anim;
	int		pic;
	int		i;
	line_t*	line;


	//	LEVEL TIMER
	if (::g->levelTimer == true)
	{
		::g->levelTimeCount--;
		if (!::g->levelTimeCount)
			G_ExitLevel();
	}

	// DHM - Nerve :: FRAG COUNT
	if ( ::g->deathmatch && ::g->levelFragCount > 0 ) {
		bool fragCountHit = false;

		for ( int i=0; i<MAXPLAYERS; i++ ) {
			if ( ::g->playeringame[i] ) {
				if ( PlayerFrags(i) >= ::g->levelFragCount ) {
					fragCountHit = true;
				}
			}
		}

		if ( fragCountHit ) {
			G_ExitLevel();
		}
	}

	//	ANIMATE FLATS AND TEXTURES GLOBALLY
	for (anim = ::g->anims ; anim < ::g->lastanim ; anim++)
	{
		for (i=anim->basepic ; i<anim->basepic+anim->numpics ; i++)
		{
			pic = anim->basepic + ( (::g->leveltime/anim->speed + i)%anim->numpics );
			if (anim->istexture)
				::g->texturetranslation[i] = pic;
			else
				::g->flattranslation[i] = pic;
		}
	}


	//	ANIMATE LINE SPECIALS
	for (i = 0; i < ::g->numlinespecials; i++)
	{
		line = ::g->linespeciallist[i];
		switch(line->special)
		{
		case 48:
			// EFFECT FIRSTCOL SCROLL +
			::g->sides[line->sidenum[0]].textureoffset += FRACUNIT;
			break;
		}
	}


	//	DO BUTTONS
	for (i = 0; i < MAXBUTTONS; i++)
		if (::g->buttonlist[i].btimer)
		{
			::g->buttonlist[i].btimer--;
			if (!::g->buttonlist[i].btimer)
			{
				switch(::g->buttonlist[i].where)
				{
				case top:
					::g->sides[::g->buttonlist[i].line->sidenum[0]].toptexture =
						::g->buttonlist[i].btexture;
					break;

				case middle:
					::g->sides[::g->buttonlist[i].line->sidenum[0]].midtexture =
						::g->buttonlist[i].btexture;
					break;

				case bottom:
					::g->sides[::g->buttonlist[i].line->sidenum[0]].bottomtexture =
						::g->buttonlist[i].btexture;
					break;
				}
				S_StartSound((mobj_t *)&::g->buttonlist[i].soundorg,sfx_swtchn);
				memset(&::g->buttonlist[i],0,sizeof(button_t));
			}
		}

}



//
// Special Stuff that can not be categorized
//
int EV_DoDonut(line_t*	line)
{
	sector_t*		s1;
	sector_t*		s2;
	sector_t*		s3;
	int			secnum;
	int			rtn;
	int			i;
	floormove_t*	floor;

	secnum = -1;
	rtn = 0;
	while ((secnum = P_FindSectorFromLineTag(line,secnum)) >= 0)
	{
		s1 = &::g->sectors[secnum];

		// ALREADY MOVING?  IF SO, KEEP GOING...
		if (s1->specialdata)
			continue;

		rtn = 1;
		s2 = getNextSector(s1->lines[0],s1);
		for (i = 0;i < s2->linecount;i++)
		{
			if ((!(s2->lines[i]->flags & ML_TWOSIDED)) ||
				(s2->lines[i]->backsector == s1))
				continue;
			s3 = s2->lines[i]->backsector;

			//	Spawn rising slime
			floor = (floormove_t*)DoomLib::Z_Malloc (sizeof(*floor), PU_LEVEL, 0);
			P_AddThinker (&floor->thinker);
			s2->specialdata = floor;
			floor->thinker.function.acp1 = (actionf_p1) T_MoveFloor;
			floor->type = donutRaise;
			floor->crush = false;
			floor->direction = 1;
			floor->sector = s2;
			floor->speed = FLOORSPEED / 2;
			floor->texture = s3->floorpic;
			floor->newspecial = 0;
			floor->floordestheight = s3->floorheight;

			//	Spawn lowering donut-hole
			floor = (floormove_t*)DoomLib::Z_Malloc (sizeof(*floor), PU_LEVEL, 0);
			P_AddThinker (&floor->thinker);
			s1->specialdata = floor;
			floor->thinker.function.acp1 = (actionf_p1) T_MoveFloor;
			floor->type = lowerFloor;
			floor->crush = false;
			floor->direction = -1;
			floor->sector = s1;
			floor->speed = FLOORSPEED / 2;
			floor->floordestheight = s3->floorheight;
			break;
		}
	}
	return rtn;
}



//
// SPECIAL SPAWNING
//

//
// P_SpawnSpecials
// After the map has been loaded, scan for specials
//  that spawn thinkers
//


// Parses command line parameters.
void P_SpawnSpecials (void)
{
	sector_t*	sector;
	int		i;
	int		episode;

	episode = 1;
	if (W_CheckNumForName("texture2") >= 0)
		episode = 2;


	// See if -TIMER needs to be used.
	::g->levelTimer = false;

	i = M_CheckParm("-avg");
	if (i && ::g->deathmatch)
	{
		::g->levelTimer = true;
		::g->levelTimeCount = 20 * 60 * TICRATE;
	}

	//i = M_CheckParm("-timer");
	//if (i && ::g->deathmatch)
#ifdef ID_ENABLE_DOOM_CLASSIC_NETWORKING
	const int timeLimit = session->GetActingGameStateLobbyBase().GetMatchParms().gameTimeLimit;
#else
	const int timeLimit = 0;
#endif
	if (timeLimit != 0 && g->deathmatch)
	{
		int	time;
		//time = atoi(::g->myargv[i+1]) * 60 * 35;
		time = timeLimit * 60 * TICRATE;
		::g->levelTimer = true;
		::g->levelTimeCount = time;
	}

	//i = M_CheckParm("-fraglimit");
	//if (i && ::g->deathmatch)
#ifdef ID_ENABLE_DOOM_CLASSIC_NETWORKING
	const int fragLimit = gameLocal->GetMatchParms().GetScoreLimit();
#else
	const int fragLimit = 0;
#endif
	if (fragLimit != 0 && ::g->deathmatch)
	{
		//::g->levelFragCount = atoi(::g->myargv[i+1]);
		::g->levelFragCount = fragLimit;
	} else {
		::g->levelFragCount = 0;
	}

	//	Init special SECTORs.
	sector = ::g->sectors;
	for (i=0 ; i < ::g->numsectors ; i++, sector++)
	{
		if (!sector->special)
			continue;

		switch (sector->special)
		{
		case 1:
			// FLICKERING LIGHTS
			P_SpawnLightFlash (sector);
			break;

		case 2:
			// STROBE FAST
			P_SpawnStrobeFlash(sector,FASTDARK,0);
			break;

		case 3:
			// STROBE SLOW
			P_SpawnStrobeFlash(sector,SLOWDARK,0);
			break;

		case 4:
			// STROBE FAST/DEATH SLIME
			P_SpawnStrobeFlash(sector,FASTDARK,0);
			sector->special = 4;
			break;

		case 8:
			// GLOWING LIGHT
			P_SpawnGlowingLight(sector);
			break;
		case 9:
			// SECRET SECTOR
			::g->totalsecret++;
			break;

		case 10:
			// DOOR CLOSE IN 30 SECONDS
			P_SpawnDoorCloseIn30 (sector);
			break;

		case 12:
			// SYNC STROBE SLOW
			P_SpawnStrobeFlash (sector, SLOWDARK, 1);
			break;

		case 13:
			// SYNC STROBE FAST
			P_SpawnStrobeFlash (sector, FASTDARK, 1);
			break;

		case 14:
			// DOOR RAISE IN 5 MINUTES
			P_SpawnDoorRaiseIn5Mins (sector, i);
			break;

		case 17:
			P_SpawnFireFlicker(sector);
			break;
		}
	}


	//	Init line EFFECTs
	::g->numlinespecials = 0;
	for (i = 0;i < ::g->numlines; i++)
	{
		switch(::g->lines[i].special)
		{
		case 48:
			// EFFECT FIRSTCOL SCROLL+
			::g->linespeciallist[::g->numlinespecials] = &::g->lines[i];
			::g->numlinespecials++;
			break;
		}
	}


	//	Init other misc stuff
	for (i = 0;i < MAXCEILINGS;i++)
		::g->activeceilings[i] = NULL;

	for (i = 0;i < MAXPLATS;i++)
		::g->activeplats[i] = NULL;

	for (i = 0;i < MAXBUTTONS;i++)
		memset(&::g->buttonlist[i],0,sizeof(button_t));

	// UNUSED: no horizonal sliders.
	//	P_InitSlidingDoorFrames();
}

