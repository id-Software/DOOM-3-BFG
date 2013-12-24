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

#include <ctype.h>

// Functions.
#include "i_system.h"
#include "m_swap.h"
#include "z_zone.h"
#include "v_video.h"
#include "w_wad.h"
#include "s_sound.h"

// Data.
#include "dstrings.h"
#include "sounds.h"

#include "doomstat.h"
#include "r_state.h"

#include "Main.h"
#include "d3xp/Game_local.h"

// ?
//#include "doomstat.h"
//#include "r_local.h"
//#include "f_finale.h"

// Stage of animation:
//  0 = text, 1 = art screen, 2 = character cast



const char*	e1text = E1TEXT;
const char*	e2text = E2TEXT;
const char*	e3text = E3TEXT;
const char*	e4text = E4TEXT;

const char*	c1text = C1TEXT;
const char*	c2text = C2TEXT;
const char*	c3text = C3TEXT;
const char*	c4text = C4TEXT;
const char*	c5text = C5TEXT;
const char*	c6text = C6TEXT;
const char* c7text = C7TEXT;
const char* c8Text = C8TEXT;

const char*	p1text = P1TEXT;
const char*	p2text = P2TEXT;
const char*	p3text = P3TEXT;
const char*	p4text = P4TEXT;
const char*	p5text = P5TEXT;
const char*	p6text = P6TEXT;

const char*	t1text = T1TEXT;
const char*	t2text = T2TEXT;
const char*	t3text = T3TEXT;
const char*	t4text = T4TEXT;
const char*	t5text = T5TEXT;
const char*	t6text = T6TEXT;

const char*	finaletext;
const char*	finaleflat;

void	F_StartCast (void);
void	F_CastTicker (void);
qboolean F_CastResponder (event_t *ev);
void	F_CastDrawer (void);

//
// F_StartFinale
//
void F_StartFinale (void)
{
    ::g->gameaction = ga_nothing;
    ::g->gamestate = GS_FINALE;
    ::g->viewactive = false;
    ::g->automapactive = false;

	// Check for end of episode/mission
	bool endOfMission = false;

	if ( ( ::g->gamemission == doom || ::g->gamemission == doom2 || ::g->gamemission == pack_tnt ||  ::g->gamemission == pack_plut  ) && ::g->gamemap == 30 ) {
		endOfMission = true;
	}
	else if ( ::g->gamemission == pack_nerve && ::g->gamemap == 8 ) {
		endOfMission = true;
	}
	else if ( ::g->gamemission == pack_master && ::g->gamemap == 21 ) {
		endOfMission = true;
	}

	localCalculateAchievements( endOfMission );

    // Okay - IWAD dependend stuff.
    // This has been changed severly, and
    //  some stuff might have changed in the process.
    switch ( ::g->gamemode )
    {

		// DOOM 1 - E1, E3 or E4, but each nine missions
		case shareware:
		case registered:
		case retail:
		{
			S_ChangeMusic(mus_victor, true);

			switch (::g->gameepisode)
			{
			  case 1:
				finaleflat = "FLOOR4_8";
				finaletext = e1text;
				break;
			  case 2:
				finaleflat = "SFLR6_1";
				finaletext = e2text;
				break;
			  case 3:
				finaleflat = "MFLR8_4";
				finaletext = e3text;
				break;
			  case 4:
				finaleflat = "MFLR8_3";
				finaletext = e4text;
				break;
			  default:
				// Ouch.
				break;
			}
			break;
		}
      
		// DOOM II and missions packs with E1, M34
		case commercial:
		{
			S_ChangeMusic(mus_read_m, true);

			if ( ::g->gamemission == doom2 || ::g->gamemission == pack_tnt || ::g->gamemission == pack_plut ) {
				switch (::g->gamemap)
				{
				  case 6:
					finaleflat = "SLIME16";
					finaletext = c1text;
					break;
				  case 11:
					finaleflat = "RROCK14";
					finaletext = c2text;
					break;
				  case 20:
					finaleflat = "RROCK07";
					finaletext = c3text;
					break;
				  case 30:
					finaleflat = "RROCK17";
					finaletext = c4text;
					break;
				  case 15:
					finaleflat = "RROCK13";
					finaletext = c5text;
					break;
				  case 31:
					finaleflat = "RROCK19";
					finaletext = c6text;
					break;
				  default:
					// Ouch.
					break;
				}
			} else if( ::g->gamemission == pack_master ) {
				switch (::g->gamemap)
				{
					case 21:
						finaleflat = "SLIME16";
						finaletext = c8Text;
						break;
				}
			} else if ( ::g->gamemission == pack_nerve ) {
				switch( ::g->gamemap ){
					case 8:
						finaleflat = "SLIME16";
						finaletext = c7text;
						break;
				}
			}
		
			break;
		}	

		// Indeterminate.
		default:
			S_ChangeMusic(mus_read_m, true);
			finaleflat = "F_SKY1"; // Not used anywhere else.
			finaletext = c1text;  // FIXME - other text, music?
			break;
	}
    
    ::g->finalestage = 0;
    ::g->finalecount = 0;
}


bool finaleButtonPressed = false;
bool startButtonPressed = false;
qboolean F_Responder (event_t *event)
{
	if( !common->IsMultiplayer() && event->type == ev_keydown && event->data1 == KEY_ESCAPE ) {
		startButtonPressed = true;
		return true;
	}

	if (::g->finalestage == 2)
		return F_CastResponder (event);

    return false;
}


//
// F_Ticker
//
void F_Ticker (void)
{
    int		i;
    
	// check for skipping
	if ( (::g->gamemode == commercial) && ( ::g->finalecount > 50) )
	{
		// go on to the next level
		for (i=0 ; i<MAXPLAYERS ; i++)
			if (::g->players[i].cmd.buttons)
				break;

		if ( finaleButtonPressed || i < MAXPLAYERS)
		{	
			bool castStarted = false;
			if( ::g->gamemission == doom2 || ::g->gamemission == pack_plut || ::g->gamemission == pack_tnt ) {
				if (::g->gamemap == 30) {
					F_StartCast ();
					castStarted = true;
				}

			} else if(  ::g->gamemission == pack_master ) {
				if( :: g->gamemap == 21 ) {
					F_StartCast ();
					castStarted = true;
				}

			} else if(  ::g->gamemission == pack_nerve ) {
				if( :: g->gamemap == 8 ) {
					F_StartCast ();
					castStarted = true;
				}

			} 

			if( castStarted == false ) {
				::g->gameaction = ga_worlddone;
			}
		}
	}

	bool SkipTheText = 	finaleButtonPressed;

    // advance animation
    ::g->finalecount++;
	finaleButtonPressed = false;
	
    if (::g->finalestage == 2)
    {
		F_CastTicker ();
		return;
    }
	
    if ( ::g->gamemode == commercial) {
		startButtonPressed = false;
		return;
	}
	
	if( SkipTheText && ( ::g->finalecount > 50) ) {
		::g->finalecount =  static_cast<int>(strlen(finaletext)) * TEXTSPEED + TEXTWAIT;
	}

    if (!::g->finalestage && ::g->finalecount > static_cast<int>(strlen(finaletext)) * TEXTSPEED + TEXTWAIT)
    {
		::g->finalecount = 0;
		::g->finalestage = 1;
		::g->wipegamestate = (gamestate_t)-1;		// force a wipe
		if (::g->gameepisode == 3)
		    S_StartMusic (mus_bunny);
    }

	startButtonPressed = false;

}



//
// F_TextWrite
//

#include "hu_stuff.h"


void F_TextWrite (void)
{
    byte*	src;
    byte*	dest;
    
    int		x,y,w;
    int		count;
    const char*	ch;
    int		c;
    int		cx;
    int		cy;
    
	if(::g->finalecount == 60 ) {
		DoomLib::ShowXToContinue( true );
	}

    // erase the entire screen to a tiled background
    src = (byte*)W_CacheLumpName ( finaleflat , PU_CACHE_SHARED);
    dest = ::g->screens[0];
	
    for (y=0 ; y<SCREENHEIGHT ; y++)
    {
	for (x=0 ; x<SCREENWIDTH/64 ; x++)
	{
	    memcpy (dest, src+((y&63)<<6), 64);
	    dest += 64;
	}
	if (SCREENWIDTH&63)
	{
	    memcpy (dest, src+((y&63)<<6), SCREENWIDTH&63);
	    dest += (SCREENWIDTH&63);
	}
    }

    V_MarkRect (0, 0, SCREENWIDTH, SCREENHEIGHT);
    
    // draw some of the text onto the screen
    cx = 10;
    cy = 10;
    ch = finaletext;
	
    count = (::g->finalecount - 10)/TEXTSPEED;
    if (count < 0)
	count = 0;
    for ( ; count ; count-- )
    {
	c = *ch++;
	if (!c)
	    break;
	if (c == '\n')
	{
	    cx = 10;
	    cy += 11;
	    continue;
	}
		
	c = toupper(c) - HU_FONTSTART;
	if (c < 0 || c> HU_FONTSIZE)
	{
	    cx += 4;
	    continue;
	}
		
	w = SHORT (::g->hu_font[c]->width);
	if (cx+w > SCREENWIDTH)
	    break;
	V_DrawPatch(cx, cy, 0, ::g->hu_font[c]);
	cx+=w;
    }
	
}

//
// Final DOOM 2 animation
// Casting by id Software.
//   in order of appearance
//

castinfo_t	castorder[] = 
{
    {CC_ZOMBIE, MT_POSSESSED},
    {CC_SHOTGUN, MT_SHOTGUY},
    {CC_HEAVY, MT_CHAINGUY},
    {CC_IMP, MT_TROOP},
    {CC_DEMON, MT_SERGEANT},
    {CC_LOST, MT_SKULL},
    {CC_CACO, MT_HEAD},
    {CC_HELL, MT_KNIGHT},
    {CC_BARON, MT_BRUISER},
    {CC_ARACH, MT_BABY},
    {CC_PAIN, MT_PAIN},
    {CC_REVEN, MT_UNDEAD},
    {CC_MANCU, MT_FATSO},
    {CC_ARCH, MT_VILE},
    {CC_SPIDER, MT_SPIDER},
    {CC_CYBER, MT_CYBORG},
    {CC_HERO, MT_PLAYER},

    {NULL,(mobjtype_t)0}
};



//
// F_StartCast
//


void F_StartCast (void)
{
	if ( ::g->finalestage != 2 ) {
		::g->wipegamestate = (gamestate_t)-1;		// force a screen wipe
		::g->castnum = 0;
		::g->caststate = &::g->states[mobjinfo[castorder[::g->castnum].type].seestate];
		::g->casttics = ::g->caststate->tics;
		::g->castdeath = false;
		::g->finalestage = 2;	
		::g->castframes = 0;
		::g->castonmelee = 0;
		::g->castattacking = false;
		S_ChangeMusic(mus_evil, true);

		::g->caststartmenu = ::g->finalecount + 50;
	}	
}


//
// F_CastTicker
//
void F_CastTicker (void)
{
    int		st;
    int		sfx;

	if( ::g->finalecount == ::g->caststartmenu ) {
		DoomLib::ShowXToContinue( true );
	}

    if (--::g->casttics > 0)
	return;			// not time to change state yet
		
    if (::g->caststate->tics == -1 || ::g->caststate->nextstate == S_NULL)
    {
	// switch from deathstate to next monster
	::g->castnum++;
	::g->castdeath = false;
	if (castorder[::g->castnum].name == NULL)
	    ::g->castnum = 0;
	if (mobjinfo[castorder[::g->castnum].type].seesound)
	    S_StartSound (NULL, mobjinfo[castorder[::g->castnum].type].seesound);
	::g->caststate = &::g->states[mobjinfo[castorder[::g->castnum].type].seestate];
	::g->castframes = 0;
    }
    else
    {
	// just advance to next state in animation
	if (::g->caststate == &::g->states[S_PLAY_ATK1])
	    goto stopattack;	// Oh, gross hack!
	st = ::g->caststate->nextstate;
	::g->caststate = &::g->states[st];
	::g->castframes++;
	
	// sound hacks....
	switch (st)
	{
	  case S_PLAY_ATK1:	sfx = sfx_dshtgn; break;
	  case S_POSS_ATK2:	sfx = sfx_pistol; break;
	  case S_SPOS_ATK2:	sfx = sfx_shotgn; break;
	  case S_VILE_ATK2:	sfx = sfx_vilatk; break;
	  case S_SKEL_FIST2:	sfx = sfx_skeswg; break;
	  case S_SKEL_FIST4:	sfx = sfx_skepch; break;
	  case S_SKEL_MISS2:	sfx = sfx_skeatk; break;
	  case S_FATT_ATK8:
	  case S_FATT_ATK5:
	  case S_FATT_ATK2:	sfx = sfx_firsht; break;
	  case S_CPOS_ATK2:
	  case S_CPOS_ATK3:
	  case S_CPOS_ATK4:	sfx = sfx_shotgn; break;
	  case S_TROO_ATK3:	sfx = sfx_claw; break;
	  case S_SARG_ATK2:	sfx = sfx_sgtatk; break;
	  case S_BOSS_ATK2:
	  case S_BOS2_ATK2:
	  case S_HEAD_ATK2:	sfx = sfx_firsht; break;
	  case S_SKULL_ATK2:	sfx = sfx_sklatk; break;
	  case S_SPID_ATK2:
	  case S_SPID_ATK3:	sfx = sfx_shotgn; break;
	  case S_BSPI_ATK2:	sfx = sfx_plasma; break;
	  case S_CYBER_ATK2:
	  case S_CYBER_ATK4:
	  case S_CYBER_ATK6:	sfx = sfx_rlaunc; break;
	  case S_PAIN_ATK3:	sfx = sfx_sklatk; break;
	  default: sfx = 0; break;
	}
		
	if (sfx)
	    S_StartSound (NULL, sfx);
    }
	
    if (::g->castframes == 12)
    {
	// go into attack frame
	::g->castattacking = true;
	if (::g->castonmelee)
	    ::g->caststate=&::g->states[mobjinfo[castorder[::g->castnum].type].meleestate];
	else
	    ::g->caststate=&::g->states[mobjinfo[castorder[::g->castnum].type].missilestate];
	::g->castonmelee ^= 1;
	if (::g->caststate == &::g->states[S_NULL])
	{
	    if (::g->castonmelee)
		::g->caststate=
		    &::g->states[mobjinfo[castorder[::g->castnum].type].meleestate];
	    else
		::g->caststate=
		    &::g->states[mobjinfo[castorder[::g->castnum].type].missilestate];
	}
    }
	
    if (::g->castattacking)
    {
	if (::g->castframes == 24
	    ||	::g->caststate == &::g->states[mobjinfo[castorder[::g->castnum].type].seestate] )
	{
	  stopattack:
	    ::g->castattacking = false;
	    ::g->castframes = 0;
	    ::g->caststate = &::g->states[mobjinfo[castorder[::g->castnum].type].seestate];
	}
    }
	
    ::g->casttics = ::g->caststate->tics;
    if (::g->casttics == -1)
	::g->casttics = 15;
}


//
// F_CastResponder
//

qboolean F_CastResponder (event_t* ev)
{
    if (ev->type != ev_keydown)
	return false;
		
    if (::g->castdeath)
	return true;			// already in dying frames
		
    // go into death frame
    ::g->castdeath = true;
    ::g->caststate = &::g->states[mobjinfo[castorder[::g->castnum].type].deathstate];
    ::g->casttics = ::g->caststate->tics;
    ::g->castframes = 0;
    ::g->castattacking = false;
    if (mobjinfo[castorder[::g->castnum].type].deathsound)
	S_StartSound (NULL, mobjinfo[castorder[::g->castnum].type].deathsound);
	
    return true;
}


void F_CastPrint (const char* text)
{
    const char*	ch;
    int		c;
    int		cx;
    int		w;
    int		width;
    
    // find width
    ch = text;
    width = 0;
	
    while (ch)
    {
	c = *ch++;
	if (!c)
	    break;
	c = toupper(c) - HU_FONTSTART;
	if (c < 0 || c> HU_FONTSIZE)
	{
	    width += 4;
	    continue;
	}
		
	w = SHORT (::g->hu_font[c]->width);
	width += w;
    }
    
    // draw it
    cx = 160-width/2;
    ch = text;
    while (ch)
    {
	c = *ch++;
	if (!c)
	    break;
	c = toupper(c) - HU_FONTSTART;
	if (c < 0 || c> HU_FONTSIZE)
	{
	    cx += 4;
	    continue;
	}
		
	w = SHORT (::g->hu_font[c]->width);
	V_DrawPatch(cx, 180, 0, ::g->hu_font[c]);
	cx+=w;
    }
	
}


//
// F_CastDrawer
//
void V_DrawPatchFlipped (int x, int y, int scrn, patch_t *patch);

void F_CastDrawer (void)
{
    spritedef_t*	sprdef;
    spriteframe_t*	sprframe;
    int			lump;
    qboolean		flip;
    patch_t*		patch;
    
    // erase the entire screen to a background
    V_DrawPatch (0,0,0, (patch_t*)W_CacheLumpName ("BOSSBACK", PU_CACHE_SHARED));

    F_CastPrint (castorder[::g->castnum].name);
    
    // draw the current frame in the middle of the screen
    sprdef = &::g->sprites[::g->caststate->sprite];
    sprframe = &sprdef->spriteframes[ ::g->caststate->frame & FF_FRAMEMASK];
    lump = sprframe->lump[0];
    flip = (qboolean)sprframe->flip[0];
			
    patch = (patch_t*)W_CacheLumpNum (lump+::g->firstspritelump, PU_CACHE_SHARED);
    if (flip)
		V_DrawPatchFlipped (160,170,0,patch);
    else
		V_DrawPatch (160,170,0,patch);
}


//
// F_DrawPatchCol
//
void
F_DrawPatchCol( int x, patch_t* patch, int col ) {
    postColumn_t*	column;
    byte*			source;
    int				count;
	
    column = (postColumn_t *)((byte *)patch + LONG(patch->columnofs[col]));

	int destx = x;
	int desty = 0;

    // step through the posts in a column
    while (column->topdelta != 0xff )
    {
		source = (byte *)column + 3;
		desty = column->topdelta;
		count = column->length;
			
		while (count--)
		{
			int scaledx, scaledy;
			scaledx = destx * GLOBAL_IMAGE_SCALER;
			scaledy = desty * GLOBAL_IMAGE_SCALER;
			byte src = *source++;

			for ( int i = 0; i < GLOBAL_IMAGE_SCALER; i++ ) {
				for ( int j = 0; j < GLOBAL_IMAGE_SCALER; j++ ) {
					::g->screens[0][( scaledx + j ) + ( scaledy + i ) * SCREENWIDTH] = src;
				}
			}

			desty++;
		}
		column = (postColumn_t *)(  (byte *)column + column->length + 4 );
    }
}


//
// F_BunnyScroll
//
void F_BunnyScroll (void)
{
    int		scrolled;
    int		x;
    patch_t*	p1;
    patch_t*	p2;
    char	name[10];
    int		stage;
		
    p1 = (patch_t*)W_CacheLumpName ("PFUB2", PU_LEVEL_SHARED);
    p2 = (patch_t*)W_CacheLumpName ("PFUB1", PU_LEVEL_SHARED);

    V_MarkRect (0, 0, SCREENWIDTH, SCREENHEIGHT);
	
    scrolled = 320 - (::g->finalecount-230)/2;
    if (scrolled > 320)
	scrolled = 320;
    if (scrolled < 0)
	scrolled = 0;
		
    for ( x=0 ; x<ORIGINAL_WIDTH ; x++)
    {
	if (x+scrolled < 320)
	    F_DrawPatchCol (x, p1, x+scrolled);
	else
	    F_DrawPatchCol (x, p2, x+scrolled - 320);		
    }
	
    if (::g->finalecount < 1130)
	return;
    if (::g->finalecount < 1180)
    {
	V_DrawPatch ((ORIGINAL_WIDTH-13*8)/2,
		     (ORIGINAL_HEIGHT-8*8)/2,0, (patch_t*)W_CacheLumpName ("END0",PU_CACHE_SHARED));
	::g->laststage = 0;
	return;
    }
	
    stage = (::g->finalecount-1180) / 5;
    if (stage > 6)
	stage = 6;
    if (stage > ::g->laststage)
    {
	S_StartSound (NULL, sfx_pistol);
	::g->laststage = stage;
    }
	
    sprintf (name,"END%i",stage);
    V_DrawPatch ((ORIGINAL_WIDTH-13*8)/2, (ORIGINAL_HEIGHT-8*8)/2,0, (patch_t*)W_CacheLumpName (name,PU_CACHE_SHARED));
}


//
// F_Drawer
//
void F_Drawer (void)
{
    if (::g->finalestage == 2)
    {
	F_CastDrawer ();
	return;
    }

    if (!::g->finalestage)
	F_TextWrite ();
    else
    {
	switch (::g->gameepisode)
	{
	  case 1:
	    if ( ::g->gamemode == retail )
	      V_DrawPatch (0,0,0,
			 (patch_t*)W_CacheLumpName("CREDIT",PU_CACHE_SHARED));
	    else
	      V_DrawPatch (0,0,0,
			 (patch_t*)W_CacheLumpName("HELP2",PU_CACHE_SHARED));
	    break;
	  case 2:
	    V_DrawPatch(0,0,0,
			(patch_t*)W_CacheLumpName("VICTORY2",PU_CACHE_SHARED));
	    break;
	  case 3:
	    F_BunnyScroll ();
	    break;
	  case 4:
	    V_DrawPatch (0,0,0,
			 (patch_t*)W_CacheLumpName("ENDPIC",PU_CACHE_SHARED));
	    break;
	}
    }
			
}



