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
#include "sys/sys_session.h"
#include "sys/sys_signin.h"
//#include "DoomLeaderboards.h"
#include "d3xp/Game_local.h"


#include <stdio.h>

#include "z_zone.h"

#include "m_random.h"
#include "m_swap.h"

#include "i_system.h"

#include "w_wad.h"

#include "g_game.h"

#include "r_local.h"
#include "s_sound.h"

#include "doomstat.h"

// Data.
#include "sounds.h"

// Needs access to LFB.
#include "v_video.h"

#include "wi_stuff.h"


//
// Data needed to add patches to full screen intermission pics.
// Patches are statistics messages, and animations.
// Loads of by-pixel layout and placement, offsets etc.
//


//
// Different vetween registered DOOM (1994) and
//  Ultimate DOOM - Final edition (retail, 1995?).
// This is supposedly ignored for commercial
//  release (aka DOOM II), which had 34 maps
//  in one episode. So there.


// in tics
//U #define PAUSELEN		(TICRATE*2) 
//U #define SCORESTEP		100
//U #define ANIMPERIOD		32
// pixel distance from "(YOU)" to "PLAYER N"
//U #define STARDIST		10 
//U #define WK 1


// GLOBAL LOCATIONS

// SINGPLE-PLAYER STUFF



// NET GAME STUFF



// DEATHMATCH STUFF










//
// Animation.
// There is another anim_t used in p_spec.
//


const point_t lnodes[NUMEPISODES][NUMMAPS] =
{
    // Episode 0 World Map
    {
	{ 185, 164 },	// location of level 0 (CJ)
	{ 148, 143 },	// location of level 1 (CJ)
	{ 69, 122 },	// location of level 2 (CJ)
	{ 209, 102 },	// location of level 3 (CJ)
	{ 116, 89 },	// location of level 4 (CJ)
	{ 166, 55 },	// location of level 5 (CJ)
	{ 71, 56 },	// location of level 6 (CJ)
	{ 135, 29 },	// location of level 7 (CJ)
	{ 71, 24 }	// location of level 8 (CJ)
    },

    // Episode 1 World Map should go here
    {
	{ 254, 25 },	// location of level 0 (CJ)
	{ 97, 50 },	// location of level 1 (CJ)
	{ 188, 64 },	// location of level 2 (CJ)
	{ 128, 78 },	// location of level 3 (CJ)
	{ 214, 92 },	// location of level 4 (CJ)
	{ 133, 130 },	// location of level 5 (CJ)
	{ 208, 136 },	// location of level 6 (CJ)
	{ 148, 140 },	// location of level 7 (CJ)
	{ 235, 158 }	// location of level 8 (CJ)
    },

    // Episode 2 World Map should go here
    {
	{ 156, 168 },	// location of level 0 (CJ)
	{ 48, 154 },	// location of level 1 (CJ)
	{ 174, 95 },	// location of level 2 (CJ)
	{ 265, 75 },	// location of level 3 (CJ)
	{ 130, 48 },	// location of level 4 (CJ)
	{ 279, 23 },	// location of level 5 (CJ)
	{ 198, 48 },	// location of level 6 (CJ)
	{ 140, 25 },	// location of level 7 (CJ)
	{ 281, 136 }	// location of level 8 (CJ)
    }

};


//
// Animation locations for episode 0 (1).
// Using patches saves a lot of space,
//  as they replace 320x200 full screen frames.
//
extern const anim_t temp_epsd0animinfo[10];
const anim_t temp_epsd0animinfo[10] =
{
    { ANIM_ALWAYS, TICRATE/3, 3, { 224, 104 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 184, 160 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 112, 136 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 72, 112 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 88, 96 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 64, 48 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 192, 40 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 136, 16 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 80, 16 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 64, 24 } }
};

extern const anim_t temp_epsd1animinfo[9];
const anim_t temp_epsd1animinfo[9] =
{
    { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 1 },
    { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 2 },
    { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 3 },
    { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 4 },
    { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 5 },
    { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 6 },
    { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 7 },
    { ANIM_LEVEL, TICRATE/3, 3, { 192, 144 }, 8 },
    { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 8 }
};

extern const anim_t temp_epsd2animinfo[6];
const anim_t temp_epsd2animinfo[6] =
{
    { ANIM_ALWAYS, TICRATE/3, 3, { 104, 168 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 40, 136 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 160, 96 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 104, 80 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 120, 32 } },
    { ANIM_ALWAYS, TICRATE/4, 3, { 40, 0 } }
};


//
// GENERAL DATA
//

//
// Locally used stuff.
//


// States for single-player


// in seconds
//#define SHOWLASTLOCDELAY	SHOWNEXTLOCDELAY


// used to accelerate or skip a stage

// ::g->wbs->pnum

 // specifies current ::g->state

// contains information passed into intermission

const wbplayerstruct_t* plrs;  // ::g->wbs->plyr[]

// used for general timing

// used for timing of background animation

// signals to refresh everything for one frame


// # of commercial levels


//
//	GRAPHICS
//


//
// CODE
//



void localCalculateAchievements(bool epComplete)
{

	if( !common->IsMultiplayer() ) {

			player_t  *player = &::g->players[::g->consoleplayer];

			// Calculate Any Achievements earned from stat cumulation.
			idAchievementManager::CheckDoomClassicsAchievements( player->killcount, player->itemcount, player->secretcount, ::g->gameskill, ::g->gamemission, ::g->gamemap, ::g->gameepisode, ::g->totalkills, ::g->totalitems, ::g->totalsecret );


	}
}

// slam background
// UNUSED static unsigned char *background=0;


void WI_slamBackground(void)
{
    memcpy(::g->screens[0], ::g->screens[1], SCREENWIDTH * SCREENHEIGHT);
    V_MarkRect (0, 0, SCREENWIDTH, SCREENHEIGHT);
}

// The ticker is used to detect keys
//  because of timing issues in netgames.
qboolean WI_Responder(event_t* ev)
{
    return false;
}

// Draws "<Levelname> Finished!"
void WI_drawLF(void)
{
    int y = WI_TITLEY;

    // draw <LevelName> 
	V_DrawPatch((ORIGINAL_WIDTH - SHORT(::g->lnames[::g->wbs->last]->width))/2,
		y, FB, ::g->lnames[::g->wbs->last]);

    // draw "Finished!"
    y += (5*SHORT(::g->lnames[::g->wbs->last]->height))/4;
    
    V_DrawPatch((ORIGINAL_WIDTH - SHORT(::g->finished->width))/2,
		y, FB, ::g->finished);
}



// Draws "Entering <LevelName>"
void WI_drawEL(void)
{
    int y = WI_TITLEY;

    // draw "Entering"
    V_DrawPatch((ORIGINAL_WIDTH - SHORT(::g->entering->width))/2,
		y, FB, ::g->entering);

    // draw level
    y += (5*SHORT(::g->lnames[::g->wbs->next]->height))/4;

    V_DrawPatch((ORIGINAL_WIDTH - SHORT(::g->lnames[::g->wbs->next]->width))/2,
		y, FB, ::g->lnames[::g->wbs->next]);

}

void
WI_drawOnLnode
( int		n,
  patch_t*	c[] )
{

    int		i;
    int		left;
    int		top;
    int		right;
    int		bottom;
    qboolean	fits = false;

    i = 0;
    do
    {
	left = lnodes[::g->wbs->epsd][n].x - SHORT(c[i]->leftoffset);
	top = lnodes[::g->wbs->epsd][n].y - SHORT(c[i]->topoffset);
	right = left + SHORT(c[i]->width);
	bottom = top + SHORT(c[i]->height);

	if (left >= 0
	    && right < SCREENWIDTH
	    && top >= 0
	    && bottom < SCREENHEIGHT)
	{
	    fits = true;
	}
	else
	{
	    i++;
	}
    } while (!fits && i!=2);

    if (fits && i<2)
    {
	V_DrawPatch(lnodes[::g->wbs->epsd][n].x, lnodes[::g->wbs->epsd][n].y,
		    FB, c[i]);
    }
    else
    {
	// DEBUG
	I_Printf("Could not place patch on level %d", n+1); 
    }
}



void WI_initAnimatedBack(void)
{
    int		i;
    anim_t*	a;

    if (::g->gamemode == commercial)
	return;

    if (::g->wbs->epsd > 2)
	return;

    for (i=0;i < ::g->NUMANIMS[::g->wbs->epsd];i++)
    {
		 a = &::g->wi_stuff_anims[::g->wbs->epsd][i];

	// init variables
	a->ctr = -1;

	// specify the next time to draw it
	if (a->type == ANIM_ALWAYS)
	    a->nexttic = ::g->bcnt + 1 + (M_Random()%a->period);
	else if (a->type == ANIM_RANDOM)
	    a->nexttic = ::g->bcnt + 1 + a->data2+(M_Random()%a->data1);
	else if (a->type == ANIM_LEVEL)
	    a->nexttic = ::g->bcnt + 1;
    }

}


void WI_updateAnimatedBack(void)
{
    int		i;
    anim_t*	a;

    if (::g->gamemode == commercial)
	return;

    if (::g->wbs->epsd > 2)
	return;

    for (i=0;i < ::g->NUMANIMS[::g->wbs->epsd];i++)
    {
		 a = &::g->wi_stuff_anims[::g->wbs->epsd][i];

	if (::g->bcnt == a->nexttic)
	{
	    switch (a->type)
	    {
	      case ANIM_ALWAYS:
		if (++a->ctr >= a->nanims) a->ctr = 0;
		a->nexttic = ::g->bcnt + a->period;
		break;

	      case ANIM_RANDOM:
		a->ctr++;
		if (a->ctr == a->nanims)
		{
		    a->ctr = -1;
		    a->nexttic = ::g->bcnt+a->data2+(M_Random()%a->data1);
		}
		else a->nexttic = ::g->bcnt + a->period;
		break;
		
	      case ANIM_LEVEL:
		// gawd-awful hack for level anims
		if (!(::g->state == StatCount && i == 7)
		    && ::g->wbs->next == a->data1)
		{
		    a->ctr++;
		    if (a->ctr == a->nanims) a->ctr--;
		    a->nexttic = ::g->bcnt + a->period;
		}
		break;
	    }
	}

    }

}

void WI_drawAnimatedBack(void)
{
    int			i;
    anim_t*		a;

    if (commercial)
	return;

    if (::g->wbs->epsd > 2)
	return;

    for (i=0 ; i < ::g->NUMANIMS[::g->wbs->epsd] ; i++)
    {
		 a = &::g->wi_stuff_anims[::g->wbs->epsd][i];

	if (a->ctr >= 0)
	    V_DrawPatch(a->loc.x, a->loc.y, FB, a->p[a->ctr]);
    }

}

//
// Draws a number.
// If digits > 0, then use that many digits minimum,
//  otherwise only use as many as necessary.
// Returns new x position.
//

int
WI_drawNum
( int		x,
  int		y,
  int		n,
  int		digits )
{

    int		fontwidth = SHORT(::g->num[0]->width);
    int		neg;
    int		temp;

    if (digits < 0)
    {
	if (!n)
	{
	    // make variable-length zeros 1 digit long
	    digits = 1;
	}
	else
	{
	    // figure out # of digits in #
	    digits = 0;
	    temp = n;

	    while (temp)
	    {
		temp /= 10;
		digits++;
	    }
	}
    }

    neg = n < 0;
    if (neg)
	n = -n;

    // if non-number, do not draw it
    if (n == 1994)
	return 0;

    // draw the new number
    while (digits--)
    {
	x -= fontwidth;
	V_DrawPatch(x, y, FB, ::g->num[ n % 10 ]);
	n /= 10;
    }

    // draw a minus sign if necessary
    if (neg)
	V_DrawPatch(x-=8, y, FB, ::g->wiminus);

    return x;

}

void
WI_drawPercent
( int		x,
  int		y,
  int		p )
{
    if (p < 0)
	return;

    V_DrawPatch(x, y, FB, ::g->percent);
    WI_drawNum(x, y, p, -1);
}



//
// Display level completion time and par,
//  or "sucks" message if overflow.
//
void
WI_drawTime
( int		x,
  int		y,
  int		t )
{

    int		div;
    int		n;

    if (t<0)
	return;

    if (t <= 61*59)
    {
	div = 1;

	do
	{
	    n = (t / div) % 60;
	    x = WI_drawNum(x, y, n, 2) - SHORT(::g->colon->width);
	    div *= 60;

	    // draw
	    if (div==60 || t / div)
		V_DrawPatch(x, y, FB, ::g->colon);
	    
	} while (t / div);
    }
    else
    {
	// "sucks"
	V_DrawPatch(x - SHORT(::g->sucks->width), y, FB, ::g->sucks); 
    }
}


void WI_End(void)
{
    void WI_unloadData(void);
    WI_unloadData();
}

void WI_initNoState(void)
{
    ::g->state = NoState;
    ::g->acceleratestage = 0;
    ::g->cnt = 10;
}

void WI_updateNoState(void) {

    WI_updateAnimatedBack();

    if (!--::g->cnt) {
		// Unload data
		WI_End();
		G_WorldDone();
    }

	DoomLib::ActivateGame();
}



void WI_initShowNextLoc(void)
{
    ::g->state = ShowNextLoc;
    ::g->acceleratestage = 0;
    ::g->cnt = SHOWNEXTLOCDELAY * TICRATE;

    WI_initAnimatedBack();

	DoomLib::ActivateGame();
}

void WI_updateShowNextLoc(void)
{
    WI_updateAnimatedBack();

    if (!--::g->cnt || ::g->acceleratestage) {
		WI_initNoState();
		DoomLib::ShowXToContinue( false );
	} else {
		::g->snl_pointeron = (::g->cnt & 31) < 20;
	}
}

void WI_drawShowNextLoc(void)
{

    int		i;
    int		last;

    WI_slamBackground();

    // draw animated background
    WI_drawAnimatedBack(); 

    if ( ::g->gamemode != commercial)
    {
  	if (::g->wbs->epsd > 2)
	{
	    WI_drawEL();
	    return;
	}
	
	last = (::g->wbs->last == 8) ? ::g->wbs->next - 1 : ::g->wbs->last;

	// don't draw any splats for extra secret levels
	if( last == 9 ) {
		for (i=0 ; i<MAXPLAYERS ; i++) {
				::g->players[i].didsecret = false; 
		}
		::g->wbs->didsecret = false;
		last = -1;
	}

	// draw a splat on taken cities.
	for (i=0 ; i<=last ; i++)
	    WI_drawOnLnode(i, &::g->splat);

	// splat the secret level?
	if (::g->wbs->didsecret)
	    WI_drawOnLnode(8, &::g->splat);

	// draw flashing ptr
	if (::g->snl_pointeron)
	    WI_drawOnLnode(::g->wbs->next, ::g->yah); 
    }

    // draws which level you are entering..
    if ( (::g->gamemode != commercial)
	 || ::g->wbs->next != 30)
	WI_drawEL();  

}

void WI_drawNoState(void)
{
    ::g->snl_pointeron = true;
    WI_drawShowNextLoc();
}

int WI_fragSum(int playernum)
{
    int		i;
    int		frags = 0;
    
	for (i=0 ; i<MAXPLAYERS ; i++)
	{
		if (/*::g->playeringame[i] &&*/ i!=playernum)
		{
			frags += plrs[playernum].frags[i];
		}
	}

	
    // JDC hack - negative frags.
    frags -= plrs[playernum].frags[playernum];
    // UNUSED if (frags < 0)
    // 	frags = 0;

    return frags;
}

int WI_fragOnlySum(int playernum)
{
	int		i;
	int		frags = 0;

	for (i=0 ; i<MAXPLAYERS ; i++)
	{
		if (/*::g->playeringame[i] &&*/ i!=playernum)
		{
			frags += plrs[playernum].frags[i];
		}
	}

	return frags;
}

int WI_deathSum(int playernum)
{
	int		i;
	int		deaths = 0;

	for (i=0 ; i<MAXPLAYERS ; i++)
	{
		if ( 1 /*::g->playeringame[i]*/)
		{
			deaths += plrs[i].frags[playernum];
		}
	}

	return deaths;
}






void WI_initDeathmatchStats(void)
{

    int		i;
    int		j;

    ::g->state = StatCount;
    ::g->acceleratestage = 0;
    ::g->dm_state = 1;

    ::g->cnt_pause = TICRATE;

    for (i=0 ; i<MAXPLAYERS ; i++)
    {
	if (::g->playeringame[i])
	{
	    for (j=0 ; j<MAXPLAYERS ; j++)
		if (::g->playeringame[j])
		    ::g->dm_frags[i][j] = 0;

	    ::g->dm_totals[i] = 0;
	}
    }
    
    WI_initAnimatedBack();

	if ( common->IsMultiplayer() ) {
		localCalculateAchievements(false);

		/* JAF PS3 
		gameLocal->liveSession.GetDMSession().SetEndOfMatchStats();
		gameLocal->liveSession.GetDMSession().WriteTrueskill();

		// Write stats
		if ( gameLocal->liveSession.IsHost( ::g->consoleplayer ) ) {
			gameLocal->liveSession.GetDMSession().BeginEndLevel();
		}
		*/
	}

	DoomLib::ShowXToContinue( true );
}



void WI_updateDeathmatchStats(void)
{

    int		i;
    int		j;
    
    qboolean	stillticking;

    WI_updateAnimatedBack();

    if (::g->acceleratestage && ::g->dm_state != 4)
    {
	::g->acceleratestage = 0;

	for (i=0 ; i<MAXPLAYERS ; i++)
	{
	    if (::g->playeringame[i])
	    {
		for (j=0 ; j<MAXPLAYERS ; j++)
		    if (::g->playeringame[j])
			::g->dm_frags[i][j] = plrs[i].frags[j];

		::g->dm_totals[i] = WI_fragSum(i);
	    }
	}
	

	S_StartSound(0, sfx_barexp);
	::g->dm_state = 4;
    }

    
    if (::g->dm_state == 2)
    {
	if (!(::g->bcnt&3))
	    S_StartSound(0, sfx_pistol);
	
	stillticking = false;

	for (i=0 ; i<MAXPLAYERS ; i++)
	{
	    if (::g->playeringame[i])
	    {
		for (j=0 ; j<MAXPLAYERS ; j++)
		{
		    if (::g->playeringame[j]
			&& ::g->dm_frags[i][j] != plrs[i].frags[j])
		    {
			if (plrs[i].frags[j] < 0)
			    ::g->dm_frags[i][j]--;
			else
			    ::g->dm_frags[i][j]++;

			if (::g->dm_frags[i][j] > 99)
			    ::g->dm_frags[i][j] = 99;

			if (::g->dm_frags[i][j] < -99)
			    ::g->dm_frags[i][j] = -99;
			
			stillticking = true;
		    }
		}
		::g->dm_totals[i] = WI_fragSum(i);

		if (::g->dm_totals[i] > 99)
		    ::g->dm_totals[i] = 99;
		
		if (::g->dm_totals[i] < -99)
		    ::g->dm_totals[i] = -99;
	    }
	    
	}
	if (!stillticking)
	{
	    S_StartSound(0, sfx_barexp);
	    ::g->dm_state++;
	}

    }
    else if (::g->dm_state == 4)
    {
		if (::g->acceleratestage)
		{
			if ( !::g->demoplayback && ( ::g->usergame || ::g->netgame ) ) {
					// This sound plays repeatedly after a player continues at the end of a deathmatch,
					// and sounds bad. Quick fix is to just not play it.
					//S_StartSound(0, sfx_slop);

					DoomLib::HandleEndMatch();
			}
		}
    }
    else if (::g->dm_state & 1)
    {
		if (!--::g->cnt_pause)
		{
			::g->dm_state++;
			::g->cnt_pause = TICRATE;
		}
    }
}



void WI_drawDeathmatchStats(void)
{

    int		i;
    int		j;
    int		x;
    int		y;
    int		w;
    
    int		lh;	// line height

    lh = WI_SPACINGY;

    WI_slamBackground();
    
    // draw animated background
    WI_drawAnimatedBack(); 
    WI_drawLF();

    // draw stat titles (top line)
    V_DrawPatch(DM_TOTALSX-SHORT(::g->total->width)/2,
		DM_MATRIXY-WI_SPACINGY+10,
		FB,
		::g->total);
    
    V_DrawPatch(DM_KILLERSX, DM_KILLERSY, FB, ::g->killers);
    V_DrawPatch(DM_VICTIMSX, DM_VICTIMSY, FB, ::g->victims);

    // draw P?
    x = DM_MATRIXX + DM_SPACINGX;
    y = DM_MATRIXY;

    for (i=0 ; i<MAXPLAYERS ; i++)
    {
	if (::g->playeringame[i])
	{
	    V_DrawPatch(x-SHORT(::g->wistuff_p[i]->width)/2,
			DM_MATRIXY - WI_SPACINGY,
			FB,
			::g->wistuff_p[i]);
	    
	    V_DrawPatch(DM_MATRIXX-SHORT(::g->wistuff_p[i]->width)/2,
			y,
			FB,
			::g->wistuff_p[i]);

		// No splitscreen on PC currently
	    if (i == ::g->me /* && !gameLocal->IsSplitscreen() */ )
	    {
		V_DrawPatch(x-SHORT(::g->wistuff_p[i]->width)/2,
			    DM_MATRIXY - WI_SPACINGY,
			    FB,
			    ::g->bstar);

		V_DrawPatch(DM_MATRIXX-SHORT(::g->wistuff_p[i]->width)/2,
			    y,
			    FB,
			    ::g->star);
	    }
	}
	else
	{
		//V_DrawPatch(x-SHORT(::g->wistuff_bp[i]->width)/2,
	    //  DM_MATRIXY - WI_SPACINGY, FB, ::g->wistuff_bp[i]);
		//V_DrawPatch(DM_MATRIXX-SHORT(::g->wistuff_bp[i]->width)/2,
		// y, FB, ::g->wistuff_bp[i]);
	}
	x += DM_SPACINGX;
	y += WI_SPACINGY;
    }

    // draw stats
    y = DM_MATRIXY+10;
    w = SHORT(::g->num[0]->width);

    for (i=0 ; i<MAXPLAYERS ; i++)
    {
	x = DM_MATRIXX + DM_SPACINGX;

	if (::g->playeringame[i])
	{
	    for (j=0 ; j<MAXPLAYERS ; j++)
	    {
		if (::g->playeringame[j])
		    WI_drawNum(x+w, y, ::g->dm_frags[i][j], 2);

		x += DM_SPACINGX;
	    }
	    WI_drawNum(DM_TOTALSX+w, y, ::g->dm_totals[i], 2);
	}
	y += WI_SPACINGY;
    }
}


void WI_initNetgameStats(void)
{

    int i;

    ::g->state = StatCount;
    ::g->acceleratestage = 0;
    ::g->ng_state = 1;

    ::g->cnt_pause = TICRATE;

    for (i=0 ; i<MAXPLAYERS ; i++)
    {
	if (!::g->playeringame[i])
	    continue;

	::g->cnt_kills[i] = ::g->cnt_items[i] = ::g->cnt_secret[i] = ::g->cnt_frags[i] = 0;

	::g->dofrags += WI_fragSum(i);
    }

    ::g->dofrags = !!::g->dofrags;

    WI_initAnimatedBack();

	// JAF PS3 
	/*
	if ( gameLocal->IsMultiplayer() ) {
		if(gameLocal->IsFullVersion() && gameLocal->liveSession.IsHost( ::g->consoleplayer )) {
			bool endOfMission = false;

			if ( ::g->gamemission == 0 && ::g->gamemap == 30 ) {
				endOfMission = true;
			}
			else if ( ::g->gamemission > 0 && ::g->gamemap == 8 ) {
				endOfMission = true;
			}

			gameLocal->liveSession.GetCoopSession().BeginEndLevel( endOfMission );
		}
	}
	*/

	DoomLib::ShowXToContinue( true );
	
}



void WI_updateNetgameStats(void)
{

    int		i;
    int		fsum;
    
    qboolean	stillticking;

    WI_updateAnimatedBack();

    if (::g->acceleratestage && ::g->ng_state != 10)
    {
	::g->acceleratestage = 0;

	for (i=0 ; i<MAXPLAYERS ; i++)
	{
	    if (!::g->playeringame[i])
		continue;

	    ::g->cnt_kills[i] = (plrs[i].skills * 100) / ::g->wbs->maxkills;
	    ::g->cnt_items[i] = (plrs[i].sitems * 100) / ::g->wbs->maxitems;
	    ::g->cnt_secret[i] = (plrs[i].ssecret * 100) / ::g->wbs->maxsecret;

	    if (::g->dofrags)
		::g->cnt_frags[i] = WI_fragSum(i);
	}
	S_StartSound(0, sfx_barexp);
	::g->ng_state = 10;
    }

    if (::g->ng_state == 2)
    {
	if (!(::g->bcnt&3))
	    S_StartSound(0, sfx_pistol);

	stillticking = false;

	for (i=0 ; i<MAXPLAYERS ; i++)
	{
	    if (!::g->playeringame[i])
		continue;

	    ::g->cnt_kills[i] += 2;

	    if (::g->cnt_kills[i] >= (plrs[i].skills * 100) / ::g->wbs->maxkills)
		::g->cnt_kills[i] = (plrs[i].skills * 100) / ::g->wbs->maxkills;
	    else
		stillticking = true;
	}
	
	if (!stillticking)
	{
	    S_StartSound(0, sfx_barexp);
	    ::g->ng_state++;
	}
    }
    else if (::g->ng_state == 4)
    {
	if (!(::g->bcnt&3))
	    S_StartSound(0, sfx_pistol);

	stillticking = false;

	for (i=0 ; i<MAXPLAYERS ; i++)
	{
	    if (!::g->playeringame[i])
		continue;

	    ::g->cnt_items[i] += 2;
	    if (::g->cnt_items[i] >= (plrs[i].sitems * 100) / ::g->wbs->maxitems)
		::g->cnt_items[i] = (plrs[i].sitems * 100) / ::g->wbs->maxitems;
	    else
		stillticking = true;
	}
	if (!stillticking)
	{
	    S_StartSound(0, sfx_barexp);
	    ::g->ng_state++;
	}
    }
    else if (::g->ng_state == 6)
    {
	if (!(::g->bcnt&3))
	    S_StartSound(0, sfx_pistol);

	stillticking = false;

	for (i=0 ; i<MAXPLAYERS ; i++)
	{
	    if (!::g->playeringame[i])
		continue;

	    ::g->cnt_secret[i] += 2;

	    if (::g->cnt_secret[i] >= (plrs[i].ssecret * 100) / ::g->wbs->maxsecret)
		::g->cnt_secret[i] = (plrs[i].ssecret * 100) / ::g->wbs->maxsecret;
	    else
		stillticking = true;
	}
	
	if (!stillticking)
	{
	    S_StartSound(0, sfx_barexp);
	    ::g->ng_state += 1 + 2*!::g->dofrags;
	}
    }
    else if (::g->ng_state == 8)
    {
	if (!(::g->bcnt&3))
	    S_StartSound(0, sfx_pistol);

	stillticking = false;

	for (i=0 ; i<MAXPLAYERS ; i++)
	{
	    if (!::g->playeringame[i])
		continue;

	    ::g->cnt_frags[i] += 1;

	    if (::g->cnt_frags[i] >= (fsum = WI_fragSum(i)))
		::g->cnt_frags[i] = fsum;
	    else
		stillticking = true;
	}
	
	if (!stillticking)
	{
	    S_StartSound(0, sfx_pldeth);
	    ::g->ng_state++;
	}
    }
    else if (::g->ng_state == 10)
    {
	if (::g->acceleratestage)
	{
		if ( !::g->demoplayback && ( ::g->usergame || ::g->netgame ) ) {
			S_StartSound(0, sfx_sgcock);

			// need to do this again if they buy it
			localCalculateAchievements(false);
			if (::g->gamemode == commercial){
				WI_initNoState();
				DoomLib::ShowXToContinue( false );
			}
			else{
				WI_initShowNextLoc();
			}
		}
	}
    }
    else if (::g->ng_state & 1)
    {
	if (!--::g->cnt_pause)
	{
	    ::g->ng_state++;
	    ::g->cnt_pause = TICRATE;
	}
    }
}



void WI_drawNetgameStats(void)
{
    int		i;
    int		x;
    int		y;
    int		pwidth = SHORT(::g->percent->width);

    WI_slamBackground();
    
    // draw animated background
    WI_drawAnimatedBack(); 

    WI_drawLF();

    // draw stat titles (top line)
    V_DrawPatch(NG_STATSX+NG_SPACINGX-SHORT(::g->kills->width),
		NG_STATSY, FB, ::g->kills);

    V_DrawPatch(NG_STATSX+2*NG_SPACINGX-SHORT(::g->items->width),
		NG_STATSY, FB, ::g->items);

    V_DrawPatch(NG_STATSX+3*NG_SPACINGX-SHORT(::g->secret->width),
		NG_STATSY, FB, ::g->secret);
    
    if (::g->dofrags)
	V_DrawPatch(NG_STATSX+4*NG_SPACINGX-SHORT(::g->wistuff_frags->width),
		    NG_STATSY, FB, ::g->wistuff_frags);

    // draw stats
    y = NG_STATSY + SHORT(::g->kills->height);

    for (i=0 ; i<MAXPLAYERS ; i++)
    {
	if (!::g->playeringame[i])
	    continue;

	x = NG_STATSX;
	V_DrawPatch(x-SHORT(::g->wistuff_p[i]->width), y, FB, ::g->wistuff_p[i]);

	// No splitscreen on PC
	if (i == ::g->me /* && !gameLocal->IsSplitscreen() */ )
	    V_DrawPatch(x-SHORT(::g->wistuff_p[i]->width), y, FB, ::g->star);

	x += NG_SPACINGX;
	WI_drawPercent(x-pwidth, y+10, ::g->cnt_kills[i]);	x += NG_SPACINGX;
	WI_drawPercent(x-pwidth, y+10, ::g->cnt_items[i]);	x += NG_SPACINGX;
	WI_drawPercent(x-pwidth, y+10, ::g->cnt_secret[i]);	x += NG_SPACINGX;

	if (::g->dofrags)
	    WI_drawNum(x, y+10, ::g->cnt_frags[i], -1);

	y += WI_SPACINGY;
    }

}


void WI_initStats(void)
{
    ::g->state = StatCount;
    ::g->acceleratestage = 0;
    ::g->sp_state = 1;
    ::g->cnt_kills[0] = ::g->cnt_items[0] = ::g->cnt_secret[0] = -1;
    ::g->cnt_time = ::g->cnt_par = -1;
    ::g->cnt_pause = TICRATE;

    WI_initAnimatedBack();

	DoomLib::ShowXToContinue( true );
}

void WI_updateStats(void)
{

    WI_updateAnimatedBack();

    if (::g->acceleratestage && ::g->sp_state != 10)
    {
		::g->acceleratestage = 0;
		::g->cnt_kills[0] = (plrs[::g->me].skills * 100) / ::g->wbs->maxkills;
		::g->cnt_items[0] = (plrs[::g->me].sitems * 100) / ::g->wbs->maxitems;
		::g->cnt_secret[0] = (plrs[::g->me].ssecret * 100) / ::g->wbs->maxsecret;
		::g->cnt_time = plrs[::g->me].stime / TICRATE;
		::g->cnt_par = ::g->wbs->partime / TICRATE;
		S_StartSound(0, sfx_barexp);
		::g->sp_state = 10;
    }

    if (::g->sp_state == 2)
    {
		::g->cnt_kills[0] += 2;

		if (!(::g->bcnt&3))
			S_StartSound(0, sfx_pistol);

		if (::g->cnt_kills[0] >= (plrs[::g->me].skills * 100) / ::g->wbs->maxkills)
		{
			::g->cnt_kills[0] = (plrs[::g->me].skills * 100) / ::g->wbs->maxkills;
			S_StartSound(0, sfx_barexp);
			::g->sp_state++;
		}
    }
    else if (::g->sp_state == 4)
    {
		::g->cnt_items[0] += 2;

		if (!(::g->bcnt&3))
			S_StartSound(0, sfx_pistol);

		if (::g->cnt_items[0] >= (plrs[::g->me].sitems * 100) / ::g->wbs->maxitems)
		{
			::g->cnt_items[0] = (plrs[::g->me].sitems * 100) / ::g->wbs->maxitems;
			S_StartSound(0, sfx_barexp);
			::g->sp_state++;
		}
    }
    else if (::g->sp_state == 6)
    {
		::g->cnt_secret[0] += 2;

		if (!(::g->bcnt&3))
			S_StartSound(0, sfx_pistol);

		if (::g->cnt_secret[0] >= (plrs[::g->me].ssecret * 100) / ::g->wbs->maxsecret)
		{
			::g->cnt_secret[0] = (plrs[::g->me].ssecret * 100) / ::g->wbs->maxsecret;
			S_StartSound(0, sfx_barexp);
			::g->sp_state++;
		}
    }

    else if (::g->sp_state == 8)
    {
		if (!(::g->bcnt&3))
			S_StartSound(0, sfx_pistol);

		::g->cnt_time += 3;

		if (::g->cnt_time >= plrs[::g->me].stime / TICRATE)
			::g->cnt_time = plrs[::g->me].stime / TICRATE;

		::g->cnt_par += 3;

		if (::g->cnt_par >= ::g->wbs->partime / TICRATE)
		{
			::g->cnt_par = ::g->wbs->partime / TICRATE;

			if (::g->cnt_time >= plrs[::g->me].stime / TICRATE)
			{
				S_StartSound(0, sfx_barexp);
				::g->sp_state++;
			}
		}
    }
    else if (::g->sp_state == 10)
    {
        if (::g->acceleratestage)
		{
			if ( !::g->demoplayback && ( ::g->usergame || ::g->netgame ) ) {

				S_StartSound(0, sfx_sgcock);

				// need to do this again if they buy it
				localCalculateAchievements(false);

				if (::g->gamemode == commercial) {
					WI_initNoState();
				}
				else{
					WI_initShowNextLoc();
				}
			}
		}
    }
    else if (::g->sp_state & 1)
    {
		if (!--::g->cnt_pause)
		{
			::g->sp_state++;
			::g->cnt_pause = TICRATE;
		}
    }

}

void WI_drawStats(void)
{
    // line height
    int lh;	

    lh = (3*SHORT(::g->num[0]->height))/2;

    WI_slamBackground();

    // draw animated background
    WI_drawAnimatedBack();
    
    WI_drawLF();

    V_DrawPatch(SP_STATSX, SP_STATSY, FB, ::g->kills);
    WI_drawPercent(ORIGINAL_WIDTH - SP_STATSX, SP_STATSY, ::g->cnt_kills[0]);

    V_DrawPatch(SP_STATSX, SP_STATSY+lh, FB, ::g->items);
    WI_drawPercent(ORIGINAL_WIDTH - SP_STATSX, SP_STATSY+lh, ::g->cnt_items[0]);

    V_DrawPatch(SP_STATSX, SP_STATSY+2*lh, FB, ::g->sp_secret);
    WI_drawPercent(ORIGINAL_WIDTH - SP_STATSX, SP_STATSY+2*lh, ::g->cnt_secret[0]);

    V_DrawPatch(SP_TIMEX, SP_TIMEY, FB, ::g->time);
    WI_drawTime(ORIGINAL_WIDTH/2 - SP_TIMEX, SP_TIMEY, ::g->cnt_time);

	// DHM - Nerve :: Added episode 4 par times
    //if (::g->wbs->epsd < 3)
    //{
	V_DrawPatch(ORIGINAL_WIDTH/2 + SP_TIMEX, SP_TIMEY, FB, ::g->par);
	WI_drawTime(ORIGINAL_WIDTH - SP_TIMEX, SP_TIMEY, ::g->cnt_par);
    //}

}

void WI_checkForAccelerate(void)
{
    int   i;
    player_t  *player;

    // check for button presses to skip delays
    for (i=0, player = ::g->players ; i<MAXPLAYERS ; i++, player++)
    {
		if (::g->playeringame[i])
		{
			if (player->cmd.buttons & BT_ATTACK)
			{
				if (!player->attackdown) {
					::g->acceleratestage = 1;
				}
				player->attackdown = true;
			} else {
				player->attackdown = false;
			}
			if (player->cmd.buttons & BT_USE)
			{
				if (!player->usedown) {
					::g->acceleratestage = 1;
				}
				player->usedown = true;
				
			} else {
				player->usedown = false;
			}
		}
    }
}



// Updates stuff each tick
void WI_Ticker(void)
{
    // counter for general background animation
    ::g->bcnt++;  

    if (::g->bcnt == 1)
    {
	// intermission music
  	if ( ::g->gamemode == commercial )
	  S_ChangeMusic(mus_dm2int, true);
	else
	  S_ChangeMusic(mus_inter, true); 
    }

    WI_checkForAccelerate();

    switch (::g->state)
    {
      case StatCount:
	if (::g->deathmatch) WI_updateDeathmatchStats();
	else if (::g->netgame) WI_updateNetgameStats();
	else WI_updateStats();
	break;
	
      case ShowNextLoc:
	WI_updateShowNextLoc();
	break;
	
      case NoState:
	WI_updateNoState();
	break;
    }

}

void WI_loadData(void)
{	
	int		i;
	int		j;
	char	name[9];
	anim_t*	a;

	if (::g->gamemode == commercial)
		strcpy(name, "INTERPIC");
		// DHM - Nerve :: Use our background image
		//strcpy(name, "DMENUPIC");
	else 
		idStr::snPrintf(name, sizeof( name ), "WIMAP%d", ::g->wbs->epsd);

	if ( ::g->gamemode == retail )
	{
		if (::g->wbs->epsd == 3)
			strcpy(name,"INTERPIC");
	}

	// background
	::g->bg = (patch_t*)W_CacheLumpName(name, PU_LEVEL_SHARED);    

	V_DrawPatch(0, 0, 1, ::g->bg);


    // UNUSED unsigned char *pic = ::g->screens[1];
    // if (::g->gamemode == commercial)
    // {
    // darken the background image
    // while (pic != ::g->screens[1] + SCREENHEIGHT*SCREENWIDTH)
    // {
    //   *pic = ::g->colormaps[256*25 + *pic];
    //   pic++;
    // }
    //}

	if (::g->gamemode == commercial)
	{
		::g->NUMCMAPS = 32;
		::g->lnames = (patch_t **) DoomLib::Z_Malloc(sizeof(patch_t*) * ::g->NUMCMAPS, PU_LEVEL_SHARED, 0);
		for (i=0 ; i < ::g->NUMCMAPS ; i++)
		{								
			idStr::snPrintf(name, sizeof( name ), "CWILV%2.2d", i);
			::g->lnames[i] = (patch_t*)W_CacheLumpName(name, PU_LEVEL_SHARED);
		}					
	}
	else
	{
		::g->lnames = (patch_t **) DoomLib::Z_Malloc(sizeof(patch_t*) * ( NUMMAPS ), PU_LEVEL_SHARED, 0);
		for (i=0 ; i<NUMMAPS ; i++)
		{
			sprintf(name, "WILV%d%d", ::g->wbs->epsd, i);
			::g->lnames[i] = (patch_t*)W_CacheLumpName(name, PU_LEVEL_SHARED);
		}

		// you are here
		::g->yah[0] = (patch_t*)W_CacheLumpName("WIURH0", PU_LEVEL_SHARED);

		// you are here (alt.)
		::g->yah[1] = (patch_t*)W_CacheLumpName("WIURH1", PU_LEVEL_SHARED);

		// splat
		::g->splat = (patch_t*)W_CacheLumpName("WISPLAT", PU_LEVEL_SHARED); 
	
		if (::g->wbs->epsd < 3)
		{
			for (j=0;j < ::g->NUMANIMS[::g->wbs->epsd];j++)
			{
				a = &::g->wi_stuff_anims[::g->wbs->epsd][j];
				for (i=0;i<a->nanims;i++)
				{
					// MONDO HACK!
					if (::g->wbs->epsd != 1 || j != 8) 
					{
						// animations
						idStr::snPrintf(name, sizeof( name ), "WIA%d%.2d%.2d", ::g->wbs->epsd, j, i);  
						a->p[i] = (patch_t*)W_CacheLumpName(name, PU_LEVEL_SHARED);
					}
					else
					{
						// HACK ALERT!
						a->p[i] = ::g->wi_stuff_anims[1][4].p[i]; 
					}
				}
			}
		}
	}

	// More hacks on minus sign.
	::g->wiminus = (patch_t*)W_CacheLumpName("WIMINUS", PU_LEVEL_SHARED); 

	for (i=0;i<10;i++)
	{
		// numbers 0-9
		sprintf(name, "WINUM%d", i);     
		::g->num[i] = (patch_t*)W_CacheLumpName(name, PU_LEVEL_SHARED);
	}

	// percent sign
	::g->percent = (patch_t*)W_CacheLumpName("WIPCNT", PU_LEVEL_SHARED);

	// "finished"
	::g->finished = (patch_t*)W_CacheLumpName("WIF", PU_LEVEL_SHARED);

	// "entering"
	::g->entering = (patch_t*)W_CacheLumpName("WIENTER", PU_LEVEL_SHARED);

	// "kills"
	::g->kills = (patch_t*)W_CacheLumpName("WIOSTK", PU_LEVEL_SHARED);   

	// "scrt"
	::g->secret = (patch_t*)W_CacheLumpName("WIOSTS", PU_LEVEL_SHARED);

	 // "secret"
	::g->sp_secret = (patch_t*)W_CacheLumpName("WISCRT2", PU_LEVEL_SHARED);

	::g->items = (patch_t*)W_CacheLumpName("WIOSTI", PU_LEVEL_SHARED);

	// "frgs"
	::g->wistuff_frags = (patch_t*)W_CacheLumpName("WIFRGS", PU_LEVEL_SHARED);    

	// ":"
	::g->colon = (patch_t*)W_CacheLumpName("WICOLON", PU_LEVEL_SHARED); 

	// "time"
	::g->time = (patch_t*)W_CacheLumpName("WITIME", PU_LEVEL_SHARED);   

	// "sucks"
	::g->sucks = (patch_t*)W_CacheLumpName("WISUCKS", PU_LEVEL_SHARED);  

	// "par"
	::g->par = (patch_t*)W_CacheLumpName("WIPAR", PU_LEVEL_SHARED);   

	// "killers" (vertical)
	::g->killers = (patch_t*)W_CacheLumpName("WIKILRS", PU_LEVEL_SHARED);

	// "victims" (horiz)
	::g->victims = (patch_t*)W_CacheLumpName("WIVCTMS", PU_LEVEL_SHARED);

	// "total"
	::g->total = (patch_t*)W_CacheLumpName("WIMSTT", PU_LEVEL_SHARED);   

	// your face
	::g->star = (patch_t*)W_CacheLumpName("STFST01", PU_STATIC_SHARED); // ALAN: this is statically in the game...

	// dead face
	::g->bstar = (patch_t*)W_CacheLumpName("STFDEAD0", PU_STATIC_SHARED);    

	for (i=0 ; i<MAXPLAYERS ; i++)
	{
		// "1,2,3,4"
		sprintf(name, "STPB%d", i);      
		::g->wistuff_p[i] = (patch_t*)W_CacheLumpName(name, PU_LEVEL_SHARED);

		// "1,2,3,4"
		sprintf(name, "WIBP%d", i+1);     
		::g->wistuff_bp[i] = (patch_t*)W_CacheLumpName(name, PU_LEVEL_SHARED);
	}

}

void WI_unloadData(void)
{
	Z_FreeTags( PU_LEVEL_SHARED, PU_LEVEL_SHARED );
	// HACK ALERT - reset these to help stability? they are used for consistency checking
	for (int i=0 ; i<MAXPLAYERS ; i++)
	{
		if (::g->playeringame[i]) 
		{ 
			::g->players[i].mo = NULL;
		}
	}
	::g->bg = NULL;	
}

void WI_Drawer (void)
{
    switch (::g->state)
    {
      case StatCount:
	if (::g->deathmatch)
	    WI_drawDeathmatchStats();
	else if (::g->netgame)
	    WI_drawNetgameStats();
	else
	    WI_drawStats();
	break;
	
      case ShowNextLoc:
	WI_drawShowNextLoc();
	break;
	
      case NoState:
	WI_drawNoState();
	break;
    }
}


void WI_initVariables(wbstartstruct_t* wbstartstruct)
{

    ::g->wbs = wbstartstruct;

#ifdef RANGECHECKING
    if (::g->gamemode != commercial)
    {
      if ( ::g->gamemode == retail )
	RNGCHECK(::g->wbs->epsd, 0, 3);
      else
	RNGCHECK(::g->wbs->epsd, 0, 2);
    }
    else
    {
	RNGCHECK(::g->wbs->last, 0, 8);
	RNGCHECK(::g->wbs->next, 0, 8);
    }
    RNGCHECK(::g->wbs->pnum, 0, MAXPLAYERS);
    RNGCHECK(::g->wbs->pnum, 0, MAXPLAYERS);
#endif

    ::g->acceleratestage = 0;
    ::g->cnt = ::g->bcnt = 0;
    ::g->firstrefresh = 1;
    ::g->me = ::g->wbs->pnum;
    plrs = ::g->wbs->plyr;

    if (!::g->wbs->maxkills)
	::g->wbs->maxkills = 1;

    if (!::g->wbs->maxitems)
	::g->wbs->maxitems = 1;

    if (!::g->wbs->maxsecret)
	::g->wbs->maxsecret = 1;

    if ( ::g->gamemode != retail )
      if (::g->wbs->epsd > 2)
	::g->wbs->epsd -= 3;
}

void WI_Start(wbstartstruct_t* wbstartstruct)
{

    WI_initVariables(wbstartstruct);
    WI_loadData();

    if (::g->deathmatch)
	WI_initDeathmatchStats();
    else if (::g->netgame)
	WI_initNetgameStats();
    else
	WI_initStats();
}
