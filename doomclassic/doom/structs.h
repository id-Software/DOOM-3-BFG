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

#pragma once

//  am_map.structs begin // 
typedef struct
{
	int x, y;
} fpoint_t;
typedef struct
{
	fpoint_t a, b;
} fline_t;
typedef struct
{
	fixed_t		x,y;
} mpoint_t;
typedef struct
{
	mpoint_t a, b;
} mline_t;
typedef struct
{
	fixed_t slp, islp;
} islope_t;
// am_map.structs end // 
//  f_finale.structs begin // 
typedef struct
{
    const char		*name;
    mobjtype_t	type;
} castinfo_t;
// f_finale.structs end // 
//  i_input.structs begin // 

enum  {
	J_DELTAX,
	J_DELTAY,
};
enum InputEventType
{
	IETAxis,
	IETButtonDigital,
	IETButtonAnalog,
	IETNone,
} ;	
struct InputEvent
{
	InputEventType type;
	int data;
	int action;
	int port;
} ;
// i_input.structs end // 

//  mus2midi.structs begin // 
typedef struct tagMUSheader_t {
	char    ID[4];          // identifier "MUS" 0x1A
	WORD    scoreLen;
	WORD    scoreStart;
	WORD    channels;	// count of primary channels
	WORD    sec_channels;	// count of secondary channels
	WORD    instrCnt;
	WORD    dummy;
	//// variable-length part starts here
} MUSheader_t ;
typedef struct tagMidiHeaderChunk_t {
	char name[4];
	int  length;

	short format;			// make 0
	short ntracks;			// make 1
	short division;			// 0xe250??
} MidiHeaderChunk_t;
typedef struct tagMidiTrackChunk_t {
	char name[4];
	int	length;
} MidiTrackChunk_t;
// mus2midi.structs end // 
//  m_menu.structs begin // 
typedef struct
{
	// 0 = no cursor here, 1 = ok, 2 = arrows ok
	short	status;

	char	name[10];

	// choice = menu item #.
	// if status = 2,
	//   choice=0:leftarrow,1:rightarrow
	void	(*routine)(int choice);

	// hotkey in menu
	char	alphaKey;			
} menuitem_t;
typedef struct menu_s
{
	short		numitems;	// # of menu items
	struct menu_s*	prevMenu;	// previous menu
	menuitem_t*		menuitems;	// menu items
	void		(*routine)();	// draw routine
	short		x;
	short		y;		// x,y of menu
	short		lastOn;		// last item user was on in menu
} menu_t;
typedef enum
{
    newgame = 0,
    options,
    loadgame,
    savegame,    
    quitdoom,
    main_end
} main_e;
typedef enum
{
	g_accept,
	g_cancel,
	g_change,
	qut_end
} quit_e;
typedef enum
{
	ep1,
	ep2,
	ep3,
	ep4,
	ep_end
} episodes_e;
typedef enum
{
	ex1,
	ex2,
	ex_end
} expansions_e;
typedef enum
{
	killthings,
	toorough,
	hurtme,
	violence,
	nightmare,
	newg_end
} newgame_e;
typedef enum
{
	endgame,
	scrnsize,
	messages,
	//detail,	
	option_empty1,
	mousesens,
	option_empty2,
	soundvol,
	opt_end
} options_e;
typedef enum
{
	rdthsempty1,
	read1_end
} read_e;
typedef enum
{
	rdthsempty2,
	read2_end
} read_e2;
typedef enum
{
	sfx_vol,
	sfx_empty1,
	music_vol,
	sfx_empty2,
	sound_end
} sound_e;
typedef enum
{
	load1,
	load2,
	load3,
	load4,
	load5,
	load6,
	load_end
} load_e;
// m_menu.structs end // 
//  m_misc.structs begin // 
struct default_t
{
    const char*	name;
	union {
		int *			location;
		const char * *	charLocation;
	};
	union {
		int				defaultvalue;
		const char *	charDefault;
	};
    int		scantranslate;		// PC scan code hack
    int		untranslated;		// lousy hack

	default_t( ) :
		name( NULL ),
		location( NULL ),
		defaultvalue( 0 ),
		scantranslate( 0 ),
		untranslated( 0 ) {
	}

	default_t( const char * name_, int * location_, int defaultvalue_ ) :
		name( name_ ),
		location( location_ ),
		defaultvalue( defaultvalue_ ) {
	}

	default_t( const char * name_, const char * * charLocation_, const char * charDefault_ ) :
		name( name_ ),
		charLocation( charLocation_ ),
		charDefault( charDefault_ ) {
	}
};
typedef struct
{
    char		manufacturer;
    char		version;
    char		encoding;
    char		bits_per_pixel;

    unsigned short	xmin;
    unsigned short	ymin;
    unsigned short	xmax;
    unsigned short	ymax;
    
    unsigned short	hres;
    unsigned short	vres;

    unsigned char	palette[48];
    
    char		reserved;
    char		color_planes;
    unsigned short	bytes_per_line;
    unsigned short	palette_type;
    
    char		filler[58];
    unsigned char	data;		// unbounded
} pcx_t;
// m_misc.structs end // 
//  p_enemy.structs begin // 
typedef enum
{
    DI_EAST,
    DI_NORTHEAST,
    DI_NORTH,
    DI_NORTHWEST,
    DI_WEST,
    DI_SOUTHWEST,
    DI_SOUTH,
    DI_SOUTHEAST,
    DI_NODIR,
    NUMDIRS
    
} dirtype_t;
// p_enemy.structs end // 
//  p_saveg.structs begin // 
typedef enum
{
    tc_end = 0,
    tc_mobj

} thinkerclass_t;
typedef enum
{
    tc_ceiling = 2,
    tc_door,
    tc_floor,
    tc_plat,
    tc_flash,
    tc_strobe,
    tc_glow,
    tc_endspecials,
	tc_fire

} specials_e;	
// p_saveg.structs end // 
//  p_spec.structs begin // 
typedef struct
{
	qboolean	istexture;
	int		picnum;
	int		basepic;
	int		numpics;
	int		speed;

} anim_t2;
typedef struct
{
	qboolean	istexture;	// if false, it is a flat
	char	endname[9];
	char	startname[9];
	int		speed;
} animdef_t;
// p_spec.structs end // 
//  r_bsp.structs begin // 
typedef	struct
{
    int	first;
    int last;
    
} cliprange_t;
// r_bsp.structs end // 
//  r_data.structs begin // 
typedef struct
{
    short	originx;
    short	originy;
    short	patch;
    short	stepdir;
    short	colormap;
} mappatch_t;
typedef struct
{
    char		name[8];
    int			masked;	
    short		width;
    short		height;

    // FR: Replaced obsolete void **columndirectory with int
    // for 64-bit compatibility
    int			columndirectory;	// OBSOLETE
    short		patchcount;
    mappatch_t	patches[1];
} maptexture_t;
typedef struct
{
    // Block origin (allways UL),
    // which has allready accounted
    // for the internal origin of the patch.
    int		originx;	
    int		originy;
    int		patch;
} texpatch_t;
typedef struct
{
    // Keep name for switch changing, etc.
    char	name[8];		
    short	width;
    short	height;
    
    // All the patches[patchcount]
    //  are drawn back to front into the cached texture.
    short	patchcount;
    texpatch_t	patches[1];		
    
} texture_t;
// r_data.structs end // 
//  r_things.structs begin // 
typedef struct
{
    int		x1;
    int		x2;
	
    int		column;
    int		topclip;
    int		bottomclip;

} maskdraw_t;
// r_things.structs end // 
//  st_stuff.structs begin // 
typedef enum
{
    NoState = -1,
    StatCount,
    ShowNextLoc

} stateenum_t;
// st_stuff.structs end // 
//  s_sound.structs begin // 
typedef struct
{
	// sound information (if null, channel avail.)
	sfxinfo_t*	sfxinfo;

	// origin of sound
	void*	origin;

	// handle of the sound being played
	int		handle;

} channel_t;
// s_sound.structs end // 
//  wi_stuff.structs begin // 
typedef enum
{
    ANIM_ALWAYS,
    ANIM_RANDOM,
    ANIM_LEVEL

} animenum_t;
typedef struct
{
    int		x;
    int		y;
    
} point_t;
typedef struct
{
    animenum_t	type;

    // period in tics between animations
    int		period;

    // number of animation frames
    int		nanims;

    // location of animation
    point_t	loc;

    // ALWAYS: n/a,
    // RANDOM: period deviation (<256),
    // LEVEL: level
    int		data1;

    // ALWAYS: n/a,
    // RANDOM: random base period,
    // LEVEL: n/a
    int		data2; 

    // actual graphics for frames of animations
    patch_t*	p[3]; 

    // following must be initialized to zero before use!

    // next value of bcnt (used in conjunction with period)
    int		nexttic;

    // last drawn animation frame
    int		lastdrawn;

    // next frame number to animate
    int		ctr;
    
    // used by RANDOM and LEVEL when animating
    int		state;  

} anim_t;
// wi_stuff.structs end // 
//  z_zone.structs begin // 
struct lumplookup
{
	int lump;
	lumplookup *next;
	lumplookup *prev;
};
typedef struct
{
    // total bytes malloced, including header
    int		size;

    // start / end cap for linked list
    memblock_t	blocklist;
    
    memblock_t*	rover;
    
} memzone_t;
// z_zone.structs end // 
