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

#include <stdio.h>


#include "z_zone.h"
#include "doomdef.h"
#include "st_stuff.h"
#include "p_local.h"
#include "w_wad.h"

#include "m_cheat.h"
#include "i_system.h"

// Needs access to LFB.
#include "v_video.h"

// State.
#include "doomstat.h"
#include "r_state.h"

// Data.
#include "dstrings.h"

#include "am_map.h"


// For use if I do walls with outsides/insides

// Automap colors

// drawing stuff



// scale on entry
// how much the automap moves window per tic in frame-::g->buffer coordinates
// moves 140 pixels in 1 second
// how much zoom-in per tic
// goes to 2x in 1 second
// how much zoom-out per tic
// pulls out to 0.5x in 1 second

// translates between frame-::g->buffer and map distances
// translates between frame-::g->buffer and map coordinates

// the following is crap








//
// The vector graphics for the automap.
//  A line drawing of the player pointing right,
//   starting from the middle.
//
#define R ((8*PLAYERRADIUS)/7)
mline_t player_arrow[] = 
{
	{ { -R+R/8, 0 }, { R, 0 } }, // -----
	{ { R, 0 }, { R-R/2, R/4 } },  // ----->
	{ { R, 0 }, { R-R/2, -R/4 } },
	{ { -R+R/8, 0 }, { -R-R/8, R/4 } }, // >---->
	{ { -R+R/8, 0 }, { -R-R/8, -R/4 } },
	{ { -R+3*R/8, 0 }, { -R+R/8, R/4 } }, // >>--->
	{ { -R+3*R/8, 0 }, { -R+R/8, -R/4 } }
};
#undef R

#define R ((8*PLAYERRADIUS)/7)
mline_t cheat_player_arrow[] = 
{
	{ { -R+R/8, 0 }, { R, 0 } }, // -----
	{ { R, 0 }, { R-R/2, R/6 } },  // ----->
	{ { R, 0 }, { R-R/2, -R/6 } },
	{ { -R+R/8, 0 }, { -R-R/8, R/6 } }, // >----->
	{ { -R+R/8, 0 }, { -R-R/8, -R/6 } },
	{ { -R+3*R/8, 0 }, { -R+R/8, R/6 } }, // >>----->
	{ { -R+3*R/8, 0 }, { -R+R/8, -R/6 } },
	{ { -R/2, 0 }, { -R/2, -R/6 } }, // >>-d--->
	{ { -R/2, -R/6 }, { -R/2+R/6, -R/6 } },
	{ { -R/2+R/6, -R/6 }, { -R/2+R/6, R/4 } },
	{ { -R/6, 0 }, { -R/6, -R/6 } }, // >>-dd-->
	{ { -R/6, -R/6 }, { 0, -R/6 } },
	{ { 0, -R/6 }, { 0, R/4 } },
	{ { R/6, R/4 }, { R/6, -R/7 } }, // >>-ddt->
	{ { R/6, -R/7 }, { R/6+R/32, -R/7-R/32 } },
	{ { R/6+R/32, -R/7-R/32 }, { R/6+R/10, -R/7 } }
};
#undef R

#define R (FRACUNIT)
mline_t triangle_guy[] = 
{
	{ { fixed_t(-.867*R), fixed_t(-.5*R) }, { fixed_t(.867*R), fixed_t(-.5*R) } },
	{ { fixed_t(.867*R), fixed_t(-.5*R) } , { 0, R } },
	{ { 0, R }, { fixed_t(-.867*R), fixed_t(-.5*R) } }
};
#undef R

#define R (FRACUNIT)
mline_t thintriangle_guy[] = 
{
	{ { fixed_t(-.5*R), fixed_t(-.7*R) }, { R, 0 } },
	{ { R, 0 }, { fixed_t(-.5*R), fixed_t(.7*R) } },
	{ { fixed_t(-.5*R), fixed_t(.7*R) }, { fixed_t(-.5*R), fixed_t(-.7*R) } }
};
#undef R







// location of window on screen

// size of window on screen




//
// width/height of window on map (map coords)
//

// based on level size


// based on player size



// old stuff for recovery later

// old location used by the Follower routine

// used by MTOF to scale from map-to-frame-::g->buffer coords
// used by FTOM to scale from frame-::g->buffer-to-map coords (=1/::g->scale_mtof)




const unsigned char cheat_amap_seq[] = 
{ 
	0xb2, 0x26, 0x26, 0x2e, 0xff 
};
cheatseq_t cheat_amap = cheatseq_t( cheat_amap_seq, 0 );


//extern byte ::g->screens[][SCREENWIDTH*SCREENHEIGHT];



void
V_MarkRect
( int	x,
 int	y,
 int	width,
 int	height );

// Calculates the slope and slope according to the x-axis of a line
// segment in map coordinates (with the upright y-axis n' all) so
// that it can be used with the brain-dead drawing stuff.

void
AM_getIslope
( mline_t*	ml,
 islope_t*	is )
{
	int dx, dy;

	dy = ml->a.y - ml->b.y;
	dx = ml->b.x - ml->a.x;
	if (!dy) is->islp = (dx<0?-MAXINT:MAXINT);
	else is->islp = FixedDiv(dx, dy);
	if (!dx) is->slp = (dy<0?-MAXINT:MAXINT);
	else is->slp = FixedDiv(dy, dx);

}

//
//
//
void AM_activateNewScale(void)
{
	::g->m_x += ::g->m_w/2;
	::g->m_y += ::g->m_h/2;
	::g->m_w = FTOM(::g->f_w);
	::g->m_h = FTOM(::g->f_h);
	::g->m_x -= ::g->m_w/2;
	::g->m_y -= ::g->m_h/2;
	::g->m_x2 = ::g->m_x + ::g->m_w;
	::g->m_y2 = ::g->m_y + ::g->m_h;
}

//
//
//
void AM_saveScaleAndLoc(void)
{
	::g->old_m_x = ::g->m_x;
	::g->old_m_y = ::g->m_y;
	::g->old_m_w = ::g->m_w;
	::g->old_m_h = ::g->m_h;
}

//
//
//
void AM_restoreScaleAndLoc(void)
{

	::g->m_w = ::g->old_m_w;
	::g->m_h = ::g->old_m_h;
	if (!::g->followplayer)
	{
		::g->m_x = ::g->old_m_x;
		::g->m_y = ::g->old_m_y;
	} else {
		::g->m_x = ::g->amap_plr->mo->x - ::g->m_w/2;
		::g->m_y = ::g->amap_plr->mo->y - ::g->m_h/2;
	}
	::g->m_x2 = ::g->m_x + ::g->m_w;
	::g->m_y2 = ::g->m_y + ::g->m_h;

	// Change the scaling multipliers
	::g->scale_mtof = FixedDiv(::g->f_w<<FRACBITS, ::g->m_w);
	::g->scale_ftom = FixedDiv(FRACUNIT, ::g->scale_mtof);
}

//
// adds a marker at the current location
//
void AM_addMark(void)
{
	::g->markpoints[::g->markpointnum].x = ::g->m_x + ::g->m_w/2;
	::g->markpoints[::g->markpointnum].y = ::g->m_y + ::g->m_h/2;
	::g->markpointnum = (::g->markpointnum + 1) % AM_NUMMARKPOINTS;

}

//
// Determines bounding box of all vertices,
// sets global variables controlling zoom range.
//
void AM_findMinMaxBoundaries(void)
{
	int i;
	fixed_t a;
	fixed_t b;

	::g->min_x = ::g->min_y =  MAXINT;
	::g->max_x = ::g->max_y = -MAXINT;

	for (i=0; i < ::g->numvertexes; i++)
	{
		if (::g->vertexes[i].x < ::g->min_x)
			::g->min_x = ::g->vertexes[i].x;
		else if (::g->vertexes[i].x > ::g->max_x)
			::g->max_x = ::g->vertexes[i].x;

		if (::g->vertexes[i].y < ::g->min_y)
			::g->min_y = ::g->vertexes[i].y;
		else if (::g->vertexes[i].y > ::g->max_y)
			::g->max_y = ::g->vertexes[i].y;
	}

	::g->max_w = ::g->max_x - ::g->min_x;
	::g->max_h = ::g->max_y - ::g->min_y;

	::g->min_w = 2*PLAYERRADIUS; // const? never changed?
	::g->min_h = 2*PLAYERRADIUS;

	a = FixedDiv(::g->f_w<<FRACBITS, ::g->max_w);
	b = FixedDiv(::g->f_h<<FRACBITS, ::g->max_h);

	::g->min_scale_mtof = a < b ? a : b;
	::g->max_scale_mtof = FixedDiv(::g->f_h<<FRACBITS, 2*PLAYERRADIUS);

}


//
//
//
void AM_changeWindowLoc(void)
{
	if (::g->m_paninc.x || ::g->m_paninc.y)
	{
		::g->followplayer = 0;
		::g->f_oldloc.x = MAXINT;
	}

	::g->m_x += ::g->m_paninc.x;
	::g->m_y += ::g->m_paninc.y;

	if (::g->m_x + ::g->m_w/2 > ::g->max_x)
		::g->m_x = ::g->max_x - ::g->m_w/2;
	else if (::g->m_x + ::g->m_w/2 < ::g->min_x)
		::g->m_x = ::g->min_x - ::g->m_w/2;

	if (::g->m_y + ::g->m_h/2 > ::g->max_y)
		::g->m_y = ::g->max_y - ::g->m_h/2;
	else if (::g->m_y + ::g->m_h/2 < ::g->min_y)
		::g->m_y = ::g->min_y - ::g->m_h/2;

	::g->m_x2 = ::g->m_x + ::g->m_w;
	::g->m_y2 = ::g->m_y + ::g->m_h;
}


//
//
//
void AM_initVariables(void)
{
	static event_t st_notify = { ev_keyup, AM_MSGENTERED };
	int pnum;

	::g->automapactive = true;
	::g->fb = ::g->screens[0];

	::g->f_oldloc.x = MAXINT;
	::g->amclock = 0;
	::g->lightlev = 0;

	::g->m_paninc.x = ::g->m_paninc.y = 0;
	::g->ftom_zoommul = FRACUNIT;
	::g->mtof_zoommul = FRACUNIT;

	::g->m_w = FTOM(::g->f_w);
	::g->m_h = FTOM(::g->f_h);

	// find player to center on initially
	if (!::g->playeringame[pnum = ::g->consoleplayer])
		for (pnum=0;pnum<MAXPLAYERS;pnum++)
			if (::g->playeringame[pnum])
				break;

	::g->amap_plr = &::g->players[pnum];
	::g->m_x = ::g->amap_plr->mo->x - ::g->m_w/2;
	::g->m_y = ::g->amap_plr->mo->y - ::g->m_h/2;
	AM_changeWindowLoc();

	// for saving & restoring
	::g->old_m_x = ::g->m_x;
	::g->old_m_y = ::g->m_y;
	::g->old_m_w = ::g->m_w;
	::g->old_m_h = ::g->m_h;

	// inform the status bar of the change
	ST_Responder(&st_notify);

}

//
// 
//
void AM_loadPics(void)
{
	int i;
	char namebuf[9];

	for (i=0;i<10;i++)
	{
		sprintf(namebuf, "AMMNUM%d", i);
		::g->marknums[i] = (patch_t*)W_CacheLumpName(namebuf, PU_STATIC_SHARED);
	}

}

void AM_unloadPics(void)
{
//	int i;

}

void AM_clearMarks(void)
{
	int i;

	for (i=0;i<AM_NUMMARKPOINTS;i++)
		::g->markpoints[i].x = -1; // means empty
	::g->markpointnum = 0;
}

//
// should be called at the start of every level
// right now, i figure it out myself
//
void AM_LevelInit(void)
{
	::g->leveljuststarted = 0;

	::g->f_x = ::g->f_y = 0;
	::g->f_w = ::g->finit_width;
	::g->f_h = ::g->finit_height;

	AM_clearMarks();

	AM_findMinMaxBoundaries();
	::g->scale_mtof = FixedDiv(::g->min_scale_mtof, (int) (0.7*FRACUNIT));
	if (::g->scale_mtof > ::g->max_scale_mtof)
		::g->scale_mtof = ::g->min_scale_mtof;
	::g->scale_ftom = FixedDiv(FRACUNIT, ::g->scale_mtof);
}




//
//
//
void AM_Stop (void)
{
	static event_t st_notify = { (evtype_t)0, ev_keyup, AM_MSGEXITED };

	AM_unloadPics();
	::g->automapactive = false;
	ST_Responder(&st_notify);
	::g->stopped = true;
}

//
//
//
void AM_Start (void)
{

	if (!::g->stopped) AM_Stop();
	::g->stopped = false;
	if (::g->lastlevel != ::g->gamemap || ::g->lastepisode != ::g->gameepisode)
	{
		AM_LevelInit();
		::g->lastlevel = ::g->gamemap;
		::g->lastepisode = ::g->gameepisode;
	}
	AM_initVariables();
	AM_loadPics();
}

//
// set the window scale to the maximum size
//
void AM_minOutWindowScale(void)
{
	::g->scale_mtof = ::g->min_scale_mtof;
	::g->scale_ftom = FixedDiv(FRACUNIT, ::g->scale_mtof);
	AM_activateNewScale();
}

//
// set the window scale to the minimum size
//
void AM_maxOutWindowScale(void)
{
	::g->scale_mtof = ::g->max_scale_mtof;
	::g->scale_ftom = FixedDiv(FRACUNIT, ::g->scale_mtof);
	AM_activateNewScale();
}


//
// Handle ::g->events (user inputs) in automap mode
//
qboolean
AM_Responder
( event_t*	ev )
{

	int rc;
	rc = false;

	if (!::g->automapactive)
	{
		if (ev->type == ev_keydown && ev->data1 == AM_STARTKEY)
		{
			AM_Start ();
			::g->viewactive = false;
			rc = true;
		}
	}

	else if (ev->type == ev_keydown)
	{

		rc = true;
		switch(ev->data1)
		{
		case AM_PANRIGHTKEY: // pan right
			if (!::g->followplayer) ::g->m_paninc.x = FTOM(F_PANINC);
			else rc = false;
			break;
		case AM_PANLEFTKEY: // pan left
			if (!::g->followplayer) ::g->m_paninc.x = -FTOM(F_PANINC);
			else rc = false;
			break;
		case AM_PANUPKEY: // pan up
			if (!::g->followplayer) ::g->m_paninc.y = FTOM(F_PANINC);
			else rc = false;
			break;
		case AM_PANDOWNKEY: // pan down
			if (!::g->followplayer) ::g->m_paninc.y = -FTOM(F_PANINC);
			else rc = false;
			break;
		case AM_ZOOMOUTKEY: // zoom out
			::g->mtof_zoommul = M_ZOOMOUT;
			::g->ftom_zoommul = M_ZOOMIN;
			break;
		case AM_ZOOMINKEY: // zoom in
			::g->mtof_zoommul = M_ZOOMIN;
			::g->ftom_zoommul = M_ZOOMOUT;
			break;
		case AM_ENDKEY:
			::g->bigstate = 0;
			::g->viewactive = true;
			AM_Stop ();
			break;
		case AM_GOBIGKEY:
			::g->bigstate = !::g->bigstate;
			if (::g->bigstate)
			{
				AM_saveScaleAndLoc();
				AM_minOutWindowScale();
			}
			else AM_restoreScaleAndLoc();
			break;
		case AM_FOLLOWKEY:
			::g->followplayer = !::g->followplayer;
			::g->f_oldloc.x = MAXINT;
			::g->amap_plr->message = ::g->followplayer ? AMSTR_FOLLOWON : AMSTR_FOLLOWOFF;
			break;
		case AM_GRIDKEY:
			::g->grid = !::g->grid;
			::g->amap_plr->message = ::g->grid ? AMSTR_GRIDON : AMSTR_GRIDOFF;
			break;
		case AM_MARKKEY:
			sprintf(::g->buffer, "%s %d", AMSTR_MARKEDSPOT, ::g->markpointnum);
			::g->amap_plr->message = ::g->buffer;
			AM_addMark();
			break;
		case AM_CLEARMARKKEY:
			AM_clearMarks();
			::g->amap_plr->message = AMSTR_MARKSCLEARED;
			break;
		default:
			::g->cheatstate=0;
			rc = false;
		}
		if (!::g->deathmatch && cht_CheckCheat(&cheat_amap, ev->data1))
		{
			rc = false;
			::g->cheating = (::g->cheating+1) % 3;
		}
	}

	else if (ev->type == ev_keyup)
	{
		rc = false;
		switch (ev->data1)
		{
		case AM_PANRIGHTKEY:
			if (!::g->followplayer) ::g->m_paninc.x = 0;
			break;
		case AM_PANLEFTKEY:
			if (!::g->followplayer) ::g->m_paninc.x = 0;
			break;
		case AM_PANUPKEY:
			if (!::g->followplayer) ::g->m_paninc.y = 0;
			break;
		case AM_PANDOWNKEY:
			if (!::g->followplayer) ::g->m_paninc.y = 0;
			break;
		case AM_ZOOMOUTKEY:
		case AM_ZOOMINKEY:
			::g->mtof_zoommul = FRACUNIT;
			::g->ftom_zoommul = FRACUNIT;
			break;
		}
	}

	return rc;

}


//
// Zooming
//
void AM_changeWindowScale(void)
{

	// Change the scaling multipliers
	::g->scale_mtof = FixedMul(::g->scale_mtof, ::g->mtof_zoommul);
	::g->scale_ftom = FixedDiv(FRACUNIT, ::g->scale_mtof);

	if (::g->scale_mtof < ::g->min_scale_mtof)
		AM_minOutWindowScale();
	else if (::g->scale_mtof > ::g->max_scale_mtof)
		AM_maxOutWindowScale();
	else
		AM_activateNewScale();
}


//
//
//
void AM_doFollowPlayer(void)
{

	if (::g->f_oldloc.x != ::g->amap_plr->mo->x || ::g->f_oldloc.y != ::g->amap_plr->mo->y)
	{
		::g->m_x = FTOM(MTOF(::g->amap_plr->mo->x)) - ::g->m_w/2;
		::g->m_y = FTOM(MTOF(::g->amap_plr->mo->y)) - ::g->m_h/2;
		::g->m_x2 = ::g->m_x + ::g->m_w;
		::g->m_y2 = ::g->m_y + ::g->m_h;
		::g->f_oldloc.x = ::g->amap_plr->mo->x;
		::g->f_oldloc.y = ::g->amap_plr->mo->y;

		//  ::g->m_x = FTOM(MTOF(::g->amap_plr->mo->x - ::g->m_w/2));
		//  ::g->m_y = FTOM(MTOF(::g->amap_plr->mo->y - ::g->m_h/2));
		//  ::g->m_x = ::g->amap_plr->mo->x - ::g->m_w/2;
		//  ::g->m_y = ::g->amap_plr->mo->y - ::g->m_h/2;

	}

}

//
//
//
void AM_updateLightLev(void)
{
	//static int litelevels[] = { 0, 3, 5, 6, 6, 7, 7, 7 };
	const static int litelevels[] = { 0, 4, 7, 10, 12, 14, 15, 15 };

	// Change light level
	if (::g->amclock>::g->nexttic)
	{
		::g->lightlev = litelevels[::g->litelevelscnt++];
		if (::g->litelevelscnt == sizeof(litelevels)/sizeof(int)) ::g->litelevelscnt = 0;
		::g->nexttic = ::g->amclock + 6 - (::g->amclock % 6);
	}

}


//
// Updates on Game Tick
//
void AM_Ticker (void)
{

	if (!::g->automapactive)
		return;

	::g->amclock++;

	if (::g->followplayer)
		AM_doFollowPlayer();

	// Change the zoom if necessary
	if (::g->ftom_zoommul != FRACUNIT)
		AM_changeWindowScale();

	// Change x,y location
	if (::g->m_paninc.x || ::g->m_paninc.y)
		AM_changeWindowLoc();

	// Update light level
	// AM_updateLightLev();

}


//
// Clear automap frame ::g->buffer.
//
void AM_clearFB(int color)
{
	memset(::g->fb, color, ::g->f_w*::g->f_h);
}


//
// Automap clipping of ::g->lines.
//
// Based on Cohen-Sutherland clipping algorithm but with a slightly
// faster reject and precalculated slopes.  If the speed is needed,
// use a hash algorithm to handle  the common cases.
//
qboolean
AM_clipMline
( mline_t*	ml,
 fline_t*	fl )
{
	enum
	{
		LEFT	=1,
		RIGHT	=2,
		BOTTOM	=4,
		TOP	=8
	};

	register	int outcode1 = 0;
	register	int outcode2 = 0;
	register	int outside;

	fpoint_t	tmp = { 0, 0 };
	int		dx;
	int		dy;




	// do trivial rejects and outcodes
	if (ml->a.y > ::g->m_y2)
		outcode1 = TOP;
	else if (ml->a.y < ::g->m_y)
		outcode1 = BOTTOM;

	if (ml->b.y > ::g->m_y2)
		outcode2 = TOP;
	else if (ml->b.y < ::g->m_y)
		outcode2 = BOTTOM;

	if (outcode1 & outcode2)
		return false; // trivially outside

	if (ml->a.x < ::g->m_x)
		outcode1 |= LEFT;
	else if (ml->a.x > ::g->m_x2)
		outcode1 |= RIGHT;

	if (ml->b.x < ::g->m_x)
		outcode2 |= LEFT;
	else if (ml->b.x > ::g->m_x2)
		outcode2 |= RIGHT;

	if (outcode1 & outcode2)
		return false; // trivially outside

	// transform to frame-::g->buffer coordinates.
	fl->a.x = CXMTOF(ml->a.x);
	fl->a.y = CYMTOF(ml->a.y);
	fl->b.x = CXMTOF(ml->b.x);
	fl->b.y = CYMTOF(ml->b.y);

	DOOUTCODE(outcode1, fl->a.x, fl->a.y);
	DOOUTCODE(outcode2, fl->b.x, fl->b.y);

	if (outcode1 & outcode2)
		return false;

	while (outcode1 | outcode2)
	{
		// may be partially inside box
		// find an outside point
		if (outcode1)
			outside = outcode1;
		else
			outside = outcode2;

		// clip to each side
		if (outside & TOP)
		{
			dy = fl->a.y - fl->b.y;
			dx = fl->b.x - fl->a.x;
			tmp.x = fl->a.x + (dx*(fl->a.y))/dy;
			tmp.y = 0;
		}
		else if (outside & BOTTOM)
		{
			dy = fl->a.y - fl->b.y;
			dx = fl->b.x - fl->a.x;
			tmp.x = fl->a.x + (dx*(fl->a.y-::g->f_h))/dy;
			tmp.y = ::g->f_h-1;
		}
		else if (outside & RIGHT)
		{
			dy = fl->b.y - fl->a.y;
			dx = fl->b.x - fl->a.x;
			tmp.y = fl->a.y + (dy*(::g->f_w-1 - fl->a.x))/dx;
			tmp.x = ::g->f_w-1;
		}
		else if (outside & LEFT)
		{
			dy = fl->b.y - fl->a.y;
			dx = fl->b.x - fl->a.x;
			tmp.y = fl->a.y + (dy*(-fl->a.x))/dx;
			tmp.x = 0;
		}

		if (outside == outcode1)
		{
			fl->a = tmp;
			DOOUTCODE(outcode1, fl->a.x, fl->a.y);
		}
		else
		{
			fl->b = tmp;
			DOOUTCODE(outcode2, fl->b.x, fl->b.y);
		}

		if (outcode1 & outcode2)
			return false; // trivially outside
	}

	return true;
}
#undef DOOUTCODE


//
// Classic Bresenham w/ whatever optimizations needed for speed
//
void
AM_drawFline
( fline_t*	fl,
 int		color )
{
	register int x;
	register int y;
	register int dx;
	register int dy;
	register int sx;
	register int sy;
	register int ax;
	register int ay;
	register int d;

	static int fuck = 0;

	// For debugging only
	if (      fl->a.x < 0 || fl->a.x >= ::g->f_w
		|| fl->a.y < 0 || fl->a.y >= ::g->f_h
		|| fl->b.x < 0 || fl->b.x >= ::g->f_w
		|| fl->b.y < 0 || fl->b.y >= ::g->f_h)
	{
		I_PrintfE("fuck %d \r", fuck++);
		return;
	}


	dx = fl->b.x - fl->a.x;
	ax = 2 * (dx<0 ? -dx : dx);
	sx = dx<0 ? -1 : 1;

	dy = fl->b.y - fl->a.y;
	ay = 2 * (dy<0 ? -dy : dy);
	sy = dy<0 ? -1 : 1;

	x = fl->a.x;
	y = fl->a.y;

	if (ax > ay)
	{
		d = ay - ax/2;
		while (1)
		{
			PUTDOT(x,y,color);
			if (x == fl->b.x) return;
			if (d>=0)
			{
				y += sy;
				d -= ax;
			}
			x += sx;
			d += ay;
		}
	}
	else
	{
		d = ax - ay/2;
		while (1)
		{
			PUTDOT(x, y, color);
			if (y == fl->b.y) return;
			if (d >= 0)
			{
				x += sx;
				d -= ay;
			}
			y += sy;
			d += ax;
		}
	}
}


//
// Clip ::g->lines, draw visible part sof ::g->lines.
//
void
AM_drawMline
( mline_t*	ml,
 int		color )
{
	static fline_t fl;

	if (AM_clipMline(ml, &fl))
		AM_drawFline(&fl, color); // draws it on frame ::g->buffer using ::g->fb coords
}



//
// Draws flat (floor/ceiling tile) aligned ::g->grid ::g->lines.
//
void AM_drawGrid(int color)
{
	fixed_t x, y;
	fixed_t start, end;
	mline_t ml;

	// Figure out start of vertical gridlines
	start = ::g->m_x;
	if ((start-::g->bmaporgx)%(MAPBLOCKUNITS<<FRACBITS))
		start += (MAPBLOCKUNITS<<FRACBITS)
		- ((start-::g->bmaporgx)%(MAPBLOCKUNITS<<FRACBITS));
	end = ::g->m_x + ::g->m_w;

	// draw vertical gridlines
	ml.a.y = ::g->m_y;
	ml.b.y = ::g->m_y+::g->m_h;
	for (x=start; x<end; x+=(MAPBLOCKUNITS<<FRACBITS))
	{
		ml.a.x = x;
		ml.b.x = x;
		AM_drawMline(&ml, color);
	}

	// Figure out start of horizontal gridlines
	start = ::g->m_y;
	if ((start-::g->bmaporgy)%(MAPBLOCKUNITS<<FRACBITS))
		start += (MAPBLOCKUNITS<<FRACBITS)
		- ((start-::g->bmaporgy)%(MAPBLOCKUNITS<<FRACBITS));
	end = ::g->m_y + ::g->m_h;

	// draw horizontal gridlines
	ml.a.x = ::g->m_x;
	ml.b.x = ::g->m_x + ::g->m_w;
	for (y=start; y<end; y+=(MAPBLOCKUNITS<<FRACBITS))
	{
		ml.a.y = y;
		ml.b.y = y;
		AM_drawMline(&ml, color);
	}

}

//
// Determines visible ::g->lines, draws them.
// This is LineDef based, not LineSeg based.
//
void AM_drawWalls(void)
{
	int i;
	static mline_t l;

	for (i = 0; i < ::g->numlines; i++)
	{
		l.a.x = ::g->lines[i].v1->x;
		l.a.y = ::g->lines[i].v1->y;
		l.b.x = ::g->lines[i].v2->x;
		l.b.y = ::g->lines[i].v2->y;
		if (::g->cheating || (::g->lines[i].flags & ML_MAPPED))
		{
			if ((::g->lines[i].flags & LINE_NEVERSEE) && !::g->cheating)
				continue;
			if (!::g->lines[i].backsector)
			{
				AM_drawMline(&l, WALLCOLORS+::g->lightlev);
			}
			else
			{
				if (::g->lines[i].special == 39)
				{ // teleporters
					AM_drawMline(&l, WALLCOLORS+WALLRANGE/2);
				}
				else if (::g->lines[i].flags & ML_SECRET) // secret door
				{
					if (::g->cheating) AM_drawMline(&l, SECRETWALLCOLORS + ::g->lightlev);
					else AM_drawMline(&l, WALLCOLORS+::g->lightlev);
				}
				else if (::g->lines[i].backsector->floorheight
					!= ::g->lines[i].frontsector->floorheight) {
						AM_drawMline(&l, FDWALLCOLORS + ::g->lightlev); // floor level change
					}
				else if (::g->lines[i].backsector->ceilingheight
					!= ::g->lines[i].frontsector->ceilingheight) {
						AM_drawMline(&l, CDWALLCOLORS+::g->lightlev); // ceiling level change
					}
				else if (::g->cheating) {
					AM_drawMline(&l, TSWALLCOLORS+::g->lightlev);
				}
			}
		}
		else if (::g->amap_plr->powers[pw_allmap])
		{
			if (!(::g->lines[i].flags & LINE_NEVERSEE)) AM_drawMline(&l, GRAYS+3);
		}
	}
}


//
// Rotation in 2D.
// Used to rotate player arrow line character.
//
void
AM_rotate
( fixed_t*	x,
 fixed_t*	y,
 angle_t	a )
{
	fixed_t tmpx;

	tmpx =
		FixedMul(*x,finecosine[a>>ANGLETOFINESHIFT])
		- FixedMul(*y,finesine[a>>ANGLETOFINESHIFT]);

	*y   =
		FixedMul(*x,finesine[a>>ANGLETOFINESHIFT])
		+ FixedMul(*y,finecosine[a>>ANGLETOFINESHIFT]);

	*x = tmpx;
}

void
AM_drawLineCharacter
( mline_t*	lineguy,
 int		lineguylines,
 fixed_t	scale,
 angle_t	angle,
 int		color,
 fixed_t	x,
 fixed_t	y )
{
	int		i;
	mline_t	l;

	for (i=0;i<lineguylines;i++)
	{
		l.a.x = lineguy[i].a.x;
		l.a.y = lineguy[i].a.y;

		if (scale)
		{
			l.a.x = FixedMul(scale, l.a.x);
			l.a.y = FixedMul(scale, l.a.y);
		}

		if (angle)
			AM_rotate(&l.a.x, &l.a.y, angle);

		l.a.x += x;
		l.a.y += y;

		l.b.x = lineguy[i].b.x;
		l.b.y = lineguy[i].b.y;

		if (scale)
		{
			l.b.x = FixedMul(scale, l.b.x);
			l.b.y = FixedMul(scale, l.b.y);
		}

		if (angle)
			AM_rotate(&l.b.x, &l.b.y, angle);

		l.b.x += x;
		l.b.y += y;

		AM_drawMline(&l, color);
	}
}

void AM_drawPlayers(void)
{
	int		i;
	player_t*	p;
	static int 	their_colors[] = { GREENS, GRAYS, BROWNS, REDS };
	int		their_color = -1;
	int		color;

	if (!::g->netgame)
	{
		if (::g->cheating)
			AM_drawLineCharacter
			(cheat_player_arrow, NUMCHEATPLYRLINES, 0,
			::g->amap_plr->mo->angle, WHITE, ::g->amap_plr->mo->x, ::g->amap_plr->mo->y);
		else
			AM_drawLineCharacter
			(player_arrow, NUMPLYRLINES, 0, ::g->amap_plr->mo->angle,
			WHITE, ::g->amap_plr->mo->x, ::g->amap_plr->mo->y);
		return;
	}

	for (i=0;i<MAXPLAYERS;i++)
	{
		their_color++;
		p = &::g->players[i];

		if ( (::g->deathmatch && !::g->singledemo) && p != ::g->amap_plr)
			continue;

		if (!::g->playeringame[i])
			continue;

		if (p->powers[pw_invisibility])
			color = 246; // *close* to black
		else
			color = their_colors[their_color];

		AM_drawLineCharacter
			(player_arrow, NUMPLYRLINES, 0, p->mo->angle,
			color, p->mo->x, p->mo->y);
	}

}

void
AM_drawThings
( int	colors,
 int 	colorrange)
{
	int		i;
	mobj_t*	t;

	for (i = 0; i < ::g->numsectors; i++)
	{
		t = ::g->sectors[i].thinglist;
		while (t)
		{
			AM_drawLineCharacter
				(thintriangle_guy, NUMTHINTRIANGLEGUYLINES,
				16<<FRACBITS, t->angle, colors+::g->lightlev, t->x, t->y);
			t = t->snext;
		}
	}
}

void AM_drawMarks(void)
{
	int i, fx, fy, w, h;

	for (i=0;i<AM_NUMMARKPOINTS;i++)
	{
		if (::g->markpoints[i].x != -1)
		{
			//      w = SHORT(::g->marknums[i]->width);
			//      h = SHORT(::g->marknums[i]->height);
			w = 5; // because something's wrong with the wad, i guess
			h = 6; // because something's wrong with the wad, i guess
			fx = CXMTOF(::g->markpoints[i].x);
			fy = CYMTOF(::g->markpoints[i].y);
			if (fx >= ::g->f_x && fx <= ::g->f_w - w && fy >= ::g->f_y && fy <= ::g->f_h - h)
				V_DrawPatch(fx/GLOBAL_IMAGE_SCALER, fy/GLOBAL_IMAGE_SCALER, FB, ::g->marknums[i]);
		}
	}

}

void AM_drawCrosshair(int color)
{
	::g->fb[(::g->f_w*(::g->f_h+1))/2] = color; // single point for now

}

void AM_Drawer (void)
{
	if (!::g->automapactive) return;

	AM_clearFB(BACKGROUND);
	if (::g->grid)
		AM_drawGrid(GRIDCOLORS);
	AM_drawWalls();
	AM_drawPlayers();
	if (::g->cheating==2)
		AM_drawThings(THINGCOLORS, THINGRANGE);
	AM_drawCrosshair(XHAIRCOLORS);

	AM_drawMarks();

	V_MarkRect(::g->f_x, ::g->f_y, ::g->f_w, ::g->f_h);

}

