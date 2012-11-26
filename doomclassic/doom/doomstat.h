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

#ifndef __D_STATE__
#define __D_STATE__

// We need globally shared data structures,
//  for defining the global state variables.
#include "doomdata.h"
#include "d_net.h"

// We need the playr data structure as well.
#include "d_player.h"


#ifdef __GNUG__
#pragma interface
#endif



// ------------------------
// Command line parameters.
//
extern  qboolean	nomonsters;	// checkparm of -nomonsters
extern  qboolean	respawnparm;	// checkparm of -respawn
extern  qboolean	fastparm;	// checkparm of -fast

extern  qboolean	devparm;	// DEBUG: launched with -devparm



// -----------------------------------------------------
// Game Mode - identify IWAD as shareware, retail etc.
//
extern GameMode_t	gamemode;
extern int	gamemission;

// Set if homebrew PWAD stuff has been added.
extern  qboolean	modifiedgame;


// -------------------------------------------
// Language.
extern  Language_t   language;


// -------------------------------------------
// Selected skill type, map etc.
//

// Defaults for menu, methinks.
extern  skill_t		startskill;
extern  int             startepisode;
extern	int		startmap;

extern  qboolean		autostart;

// Selected by user. 
extern  skill_t         gameskill;
extern  int		gameepisode;
extern  int		gamemap;

// Nightmare mode flag, single player.
extern  qboolean         respawnmonsters;

// Netgame? Only true if >1 player.
extern  qboolean	netgame;

// Flag: true only if started as net deathmatch.
// An enum might handle altdeath/cooperative better.
extern  qboolean	deathmatch;	
	
// -------------------------
// Internal parameters for sound rendering.
// These have been taken from the DOS version,
//  but are not (yet) supported with Linux
//  (e.g. no sound volume adjustment with menu.


// Current music/sfx card - index useless
//  w/o a reference LUT in a sound module.
// Ideally, this would use indices found
//  in: /usr/include/linux/soundcard.h
extern int snd_MusicDevice;
extern int snd_SfxDevice;
// Config file? Same disclaimer as above.
extern int snd_DesiredMusicDevice;
extern int snd_DesiredSfxDevice;


// -------------------------
// Status flags for refresh.
//

// Depending on view size - no status bar?
// Note that there is no way to disable the
//  status bar explicitely.
extern  qboolean statusbaractive;

extern  qboolean automapactive;	// In AutoMap mode?
extern  qboolean	menuactive;	// Menu overlayed?
extern  qboolean	paused;		// Game Pause?


extern  qboolean		viewactive;

extern  qboolean		nodrawers;
extern  qboolean		noblit;

extern	int		viewwindowx;
extern	int		viewwindowy;
extern	int		viewheight;
extern	int		viewwidth;
extern	int		scaledviewwidth;






// This one is related to the 3-screen display mode.
// ANG90 = left side, ANG270 = right
extern  int	viewangleoffset;

// Player taking events, and displaying.
extern  int	consoleplayer;	
extern  int	displayplayer;


// -------------------------------------
// Scores, rating.
// Statistics on a given map, for intermission.
//
extern  int	totalkills;
extern	int	totalitems;
extern	int	totalsecret;

// Timer, for scores.
extern  int	levelstarttic;	// gametic at level start
extern  int	leveltime;	// tics in game play for par



// --------------------------------------
// DEMO playback/recording related stuff.
// No demo, there is a human player in charge?
// Disable save/end game?
extern  qboolean	usergame;

//?
extern  qboolean	demoplayback;

// Quit after playing a demo from cmdline.
extern  qboolean		singledemo;	




//?
extern  gamestate_t     gamestate;






//-----------------------------
// Internal parameters, fixed.
// These are set by the engine, and not changed
//  according to user inputs. Partly load from
//  WAD, partly set at startup time.



extern	int		gametic;


// Bookkeeping on players - state.
extern	player_t	players[MAXPLAYERS];

// Alive? Disconnected?
extern  qboolean		playeringame[MAXPLAYERS];


// Player spawn spots for deathmatch.
#define MAX_DM_STARTS   10
extern  mapthing_t      deathmatchstarts[MAX_DM_STARTS];
extern  mapthing_t*	deathmatch_p;

// Player spawn spots.
extern  mapthing_t      playerstarts[MAXPLAYERS];

// Intermission stats.
// Parameters for world map / intermission.
extern  wbstartstruct_t		wminfo;	


// LUT of ammunition limits for each kind.
// This doubles with BackPack powerup item.
const extern  int		maxammo[NUMAMMO];





//-----------------------------------------
// Internal parameters, used for engine.
//

// File handling stuff.
extern	char		basedefault[1024];
extern  FILE*		debugfile;

// if true, load all graphics at level load
extern  qboolean         precache;


// wipegamestate can be set to -1
//  to force a wipe on the next draw
extern  gamestate_t     wipegamestate;

extern  int             mouseSensitivity;
//?
// debug flag to cancel adaptiveness
extern  qboolean         singletics;	

extern  int             bodyqueslot;



// Needed to store the number of the dummy sky flat.
// Used for rendering,
//  as well as tracking projectiles etc.
extern int		skyflatnum;



// Netgame stuff (buffers and pointers, i.e. indices).

// This is ???
extern  doomcom_t	doomcom;

// This points inside doomcom.
extern  doomdata_t*	netbuffer;	


extern  ticcmd_t	localcmds[BACKUPTICS];
extern	int		rndindex;

extern	int		maketic;
extern  int             nettics[MAXNETNODES];

extern  ticcmd_t        netcmds[MAXPLAYERS][BACKUPTICS];
extern	int		ticdup;



#endif

