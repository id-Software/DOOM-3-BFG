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

// the all-important zone //
memzone_t*	mainzone;

idFile *	wadFileHandles[MAXWADFILES];
int		numWadFiles;

//  am_map.vars begin // 
int 	cheating ;
int 	grid ;
int 	leveljuststarted ;
qboolean    	automapactive ;
int 	finit_width ;
int 	finit_height ;
int 	f_x;
int	f_y;
int 	f_w;
int	f_h;
int 	lightlev; 		// used for funky strobing effect
byte*	fb; 			// pseudo-frame buffer
int 	amclock;
mpoint_t m_paninc; // how far the window pans each tic (map coords)
fixed_t 	mtof_zoommul; // how far the window zooms in each tic (map coords)
fixed_t 	ftom_zoommul; // how far the window zooms in each tic (fb coords)
fixed_t 	m_x, m_y;   // LL x,y where the window is on the map (map coords)
fixed_t 	m_x2, m_y2; // UR x,y where the window is on the map (map coords)
fixed_t 	m_w;
fixed_t	m_h;
fixed_t 	min_x;
fixed_t	min_y; 
fixed_t 	max_x;
fixed_t  max_y;
fixed_t 	max_w; // max_x-min_x,
fixed_t  max_h; // max_y-min_y
fixed_t 	min_w;
fixed_t  min_h;
fixed_t 	min_scale_mtof; // used to tell when to stop zooming out
fixed_t 	max_scale_mtof; // used to tell when to stop zooming in
fixed_t old_m_w, old_m_h;
fixed_t old_m_x, old_m_y;
mpoint_t f_oldloc;
fixed_t scale_mtof ;
fixed_t scale_ftom;
player_t *amap_plr; // the player represented by an arrow
patch_t *marknums[10]; // numbers used for marking by the automap
mpoint_t markpoints[AM_NUMMARKPOINTS]; // where the points are
int markpointnum ;
int followplayer ;
qboolean stopped ;
int lastlevel ;
int lastepisode ;
int cheatstate;
int bigstate;
char buffer[20];
int nexttic ;
int litelevelscnt ;
// am_map.vars end // 
//  doomlib.vars begin // 
fixed_t realoffset;
fixed_t viewxoffset;
fixed_t viewyoffset;
// doomlib.vars end // 
//  doomstat.vars begin // 
GameMode_t gamemode ;
int	gamemission ;
Language_t   language ;
qboolean	modifiedgame;
// doomstat.vars end // 
//  d_main.vars begin // 
qboolean		devparm;	// started game with -devparm
qboolean         nomonsters;	// checkparm of -nomonsters
qboolean         respawnparm;	// checkparm of -respawn
qboolean         fastparm;	// checkparm of -fast
qboolean         drone;
qboolean		singletics ;
skill_t		startskill;
int             startepisode;
int		startmap;
qboolean		autostart;
FILE*		debugfile;
qboolean		advancedemo;
char		wadfile[1024];		// primary wad file
char		mapdir[1024];           // directory of development maps
char		basedefault[1024];      // default file
event_t         events[MAXEVENTS];
int             eventhead;
int 		eventtail;
gamestate_t     wipegamestate ;
 qboolean		viewactivestate ;
 qboolean		menuactivestate ;
 qboolean		inhelpscreensstate ;
 qboolean		fullscreen ;
 int			borderdrawcount;
qboolean			wipe ;
int				wipestart;
qboolean			wipedone;
int             demosequence;
int             pagetic;
char                    *pagename;
char            title[128];
// d_main.vars end // 
//  d_net.vars begin // 
doomcom_t	doomcom;
doomdata_t*	netbuffer;		// points inside doomcom
ticcmd_t	localcmds[BACKUPTICS];
ticcmd_t        netcmds[MAXPLAYERS][BACKUPTICS];
int         	nettics[MAXNETNODES];
qboolean		nodeingame[MAXNETNODES];		// set false as nodes leave game
qboolean		remoteresend[MAXNETNODES];		// set when local needs tics
int		resendto[MAXNETNODES];			// set when remote needs tics
int		resendcount[MAXNETNODES];
int		nodeforplayer[MAXPLAYERS];
int             maketic;
int		lastnettic;
int		skiptics;
int		ticdup;		
int		maxsend;	// BACKUPTICS/(2*ticdup)-1
qboolean		reboundpacket;
doomdata_t	reboundstore;
char    exitmsg[80];
int      gametime;
qboolean	gotinfo[MAXNETNODES];
int	frametics[4];
int	frameon;
int	frameskip[4];
int	oldnettics;
int	oldtrt_entertics;
int trt_phase ;
int		trt_lowtic;
int		trt_entertic;
int		trt_realtics;
int		trt_availabletics;
int		trt_counts;
int		trt_numplaying;
// d_net.vars end // 
//  f_finale.vars begin // 
int		finalestage;
int		finalecount;
int		castnum;
int		casttics;
state_t*	caststate;
qboolean		castdeath;
int		castframes;
int		castonmelee;
int		caststartmenu;
qboolean		castattacking;
int	laststage;
// f_finale.vars end // 
//  f_wipe.vars begin // 
qboolean	go ;
byte*	wipe_scr_start;
byte*	wipe_scr_end;
byte*	wipe_scr;
void *g_tempPointer;
int*	wipe_y;
// f_wipe.vars end // 
//  g_game.vars begin // 
gameaction_t    gameaction; 
gamestate_t     gamestate;
gamestate_t		oldgamestate;
skill_t         gameskill; 
qboolean		respawnmonsters;
int             gameepisode; 
int             gamemap; 
qboolean         paused; 
qboolean         sendpause;             	// send a pause event next tic 
qboolean         sendsave;             	// send a save event next tic 
qboolean         usergame;               // ok to save / end game 
qboolean         timingdemo;             // if true, exit with report on completion 
qboolean         nodrawers;              // for comparative timing purposes 
qboolean         noblit;                 // for comparative timing purposes 
int             starttime;          	// for comparative timing purposes  	 
qboolean         viewactive; 
qboolean         deathmatch;           	// only if started as net death 
qboolean         netgame;                // only true if packets are broadcast 
qboolean         playeringame[MAXPLAYERS]; 
player_t        players[MAXPLAYERS]; 
int             consoleplayer;          // player taking events and displaying 
int             displayplayer;          // view being displayed 
int             gametic; 
int             levelstarttic;          // gametic at level start 
int             totalkills, totalitems, totalsecret;    // for intermission 
char            demoname[32]; 
qboolean        demoplayback;
qboolean        demorecording;
qboolean		netdemo; 
byte*		demobuffer;
byte*		demo_p;
byte*		demoend; 
qboolean         singledemo;            	// quit after playing a demo from cmdline 
qboolean         precache ;
wbstartstruct_t wminfo;               	// parms for world map / intermission 
short		consistancy[MAXPLAYERS][BACKUPTICS]; 
byte*		savebuffer;
int			savebufferSize;
int             key_right;
int		key_left;
int		key_up;
int		key_down; 
int             key_strafeleft;
int		key_straferight; 
int             key_fire;
int		key_use;
int		key_strafe;
int		key_speed; 
int             mousebfire; 
int             mousebstrafe; 
int             mousebforward; 
int             joybfire; 
int             joybstrafe; 
int             joybuse; 
int             joybspeed; 
fixed_t		forwardmove[2];
fixed_t		sidemove[2];
fixed_t		angleturn[3];
qboolean         gamekeydown[NUMKEYS]; 
int             turnheld;				// for accelerative turning 
qboolean		mousearray[4]; 
qboolean*	mousebuttons ;
int             mousex;
int		mousey;         
int             dclicktime;
int		dclickstate;
int		dclicks; 
int             dclicktime2;
int		dclickstate2;
int		dclicks2;
int             joyxmove;
int		joyymove;
qboolean         joyarray[5]; 
qboolean*	joybuttons ;
int		savegameslot; 
char		savedescription[32]; 
mobj_t*		bodyque[BODYQUESIZE]; 
int		bodyqueslot; 
char turbomessage[80];
qboolean		secretexit; 
char	savename[256];
skill_t	d_skill; 
int     d_episode; 
int     d_map; 
int		d_mission;
char*	defdemoname; 
// g_game.vars end // 
//  hu_lib.vars begin // 
qboolean	lastautomapactive ;
// hu_lib.vars end // 
//  hu_stuff.vars begin // 
char			chat_char; // remove later.
player_t*	plr;
patch_t*		hu_font[HU_FONTSIZE];
hu_textline_t	w_title;
qboolean			chat_on;
hu_itext_t	w_chat;
qboolean		always_off ;
char		chat_dest[MAXPLAYERS];
hu_itext_t w_inputbuffer[MAXPLAYERS];
qboolean		message_on;
qboolean			message_dontfuckwithme;
qboolean		message_nottobefuckedwith;
hu_stext_t	w_message;
int		message_counter;
qboolean		headsupactive ;
char	chatchars[QUEUESIZE];
int	head ;
int	tail ;
char		lastmessage[HU_MAXLINELENGTH+1];
qboolean	shiftdown ;
qboolean	altdown ;
int		num_nobrainers ;
// hu_stuff.vars end // 
//  i_input.vars begin // 
InputEvent mouseEvents[2];
InputEvent joyEvents[18];
// i_input.vars end // 
//  i_net_xbox.vars begin // 
int			sendsocket;
int			insocket;
//struct	sockaddr_in	sendaddress[MAXNETNODES];
// i_net_xbox.vars end // 
//  i_system.vars begin // 
int	mb_used ;
ticcmd_t	emptycmd;
int current_time ;
// i_system.vars end // 
//  i_video_xbox.vars begin // 

unsigned int	XColorMap[256];
unsigned int *ImageBuff;
unsigned int *ImageBuffs[2];

// i_video_xbox.vars end // 
//  m_argv.vars begin // 
int		myargc;
char**		myargv;
// m_argv.vars end // 
//  m_cheat.vars begin // 
int		firsttime ;
unsigned char	cheat_xlate_table[256];
unsigned char cheatbuffer[256];
int usedcheatbuffer ;
// m_cheat.vars end // 
//  m_menu.vars begin // 
int			mouseSensitivity;       // has default
int			showMessages;
int			detailLevel;		
int			screenblocks;		// has default
int			screenSize;		
int			quickSaveSlot;          
int			messageToPrint;
char*			messageString;		
int			messx;			
int			messy;
int			messageLastMenuActive;
qboolean			messageNeedsInput;     
messageRoutine_t messageRoutine;
int			saveStringEnter;              
int             	saveSlot;	// which slot to save in
int			saveCharIndex;	// which char we're editing
char			saveOldString[SAVESTRINGSIZE];  
qboolean			inhelpscreens;
qboolean			menuactive;
char			savegamestrings[10][SAVESTRINGSIZE];
char			savegamepaths[10][MAX_PATH];
char	endstring[160];
short		itemOn;			// menu item skull is on
short		skullAnimCounter;	// skull animation counter
short		whichSkull;		// which skull to draw
menu_t*	currentMenu;                          
menuitem_t MainMenu[5];
menu_t  QuitDef;
menuitem_t QuitMenu[3];
menu_t  MainDef;
menuitem_t EpisodeMenu[4];
menu_t  EpiDef;
menuitem_t ExpansionMenu[2];
menu_t  ExpDef;
menuitem_t NewGameMenu[5];
menu_t  NewDef;
menuitem_t OptionsMenu[8];
menu_t  OptionsDef;
menuitem_t SoundMenu[4];
menu_t  SoundDef;
menuitem_t LoadMenu[6];
menu_t  LoadDef;
menuitem_t LoadExpMenu[2];
menu_t  LoadExpDef;
menuitem_t SaveMenu[6];
menu_t  SaveDef;
char    tempstring[80];
int     epi;
int     exp;
int     quitsounds[8];
int     quitsounds2[8];
 int     joywait ;
 int     mousewait ;
 int     mmenu_mousey ;
 int     lasty ;
 int     mmenu_mousex ;
 int     lastx ;
short	md_x;
short	md_y;
// m_menu.vars end // 
//  m_misc.vars begin // 
const char * g_pszSaveFile ;
const char * g_pszImagePath ;
const char * g_pszImageMeta ;
int		usemouse;
int		usejoystick;
char*		mousetype;
char*		mousedev;
default_t	defaults[35];
int	numdefaults;
const char*	defaultfile;
// m_misc.vars end // 
//  m_random.vars begin // 
int	rndindex ;
int	prndindex ;
// m_random.vars end // 
//  p_ceilng.vars begin // 
ceiling_t*	activeceilings[MAXCEILINGS];
// p_ceilng.vars end // 
//  p_enemy.vars begin // 
mobj_t*		soundtarget;
int	TRACEANGLE ;
mobj_t*		corpsehit;
mobj_t*		vileobj;
fixed_t		viletryx;
fixed_t		viletryy;
mobj_t*		braintargets[32];
int		numbraintargets;
int		braintargeton;
int	easy ;
// p_enemy.vars end // 
//  p_map.vars begin // 
fixed_t		tmbbox[4];
mobj_t*		tmthing;
int		tmflags;
fixed_t		tmx;
fixed_t		tmy;
qboolean		floatok;
fixed_t		tmfloorz;
fixed_t		tmceilingz;
fixed_t		tmdropoffz;
line_t*		ceilingline;
line_t*		spechit[MAXSPECIALCROSS];
int		numspechit;
fixed_t		bestslidefrac;
fixed_t		secondslidefrac;
line_t*		bestslideline;
line_t*		secondslideline;
mobj_t*		slidemo;
fixed_t		tmxmove;
fixed_t		tmymove;
mobj_t*		linetarget;	// who got hit (or NULL)
mobj_t*		shootthing;
fixed_t		shootz;	
int		la_damage;
fixed_t		attackrange;
fixed_t		aimslope;
mobj_t*		usething;
mobj_t*		bombsource;
mobj_t*		bombspot;
int		bombdamage;
qboolean		crushchange;
qboolean		nofit;
// p_map.vars end // 
//  p_maputl.vars begin // 
fixed_t opentop;
fixed_t openbottom;
fixed_t openrange;
fixed_t	lowfloor;
intercept_t	intercepts[MAXINTERCEPTS];
intercept_t*	intercept_p;
divline_t 	trace;
qboolean 	earlyout;
int		ptflags;
// p_maputl.vars end // 
//  p_mobj.vars begin // 
int test;
mapthing_t	itemrespawnque[ITEMQUESIZE];
int		itemrespawntime[ITEMQUESIZE];
int		iquehead;
int		iquetail;
// p_mobj.vars end // 
//  p_plats.vars begin // 
plat_t*		activeplats[MAXPLATS];
// p_plats.vars end // 
//  p_pspr.vars begin // 
fixed_t		swingx;
fixed_t		swingy;
fixed_t		bulletslope;
// p_pspr.vars end // 
//  p_saveg.vars begin // 
byte*		save_p;
// p_saveg.vars end // 
//  p_setup.vars begin // 
int		numvertexes;
vertex_t*	vertexes;
int		numsegs;
seg_t*		segs;
int		numsectors;
sector_t*	sectors;
int		numsubsectors;
subsector_t*	subsectors;
int		numnodes;
node_t*		nodes;
int		numlines;
line_t*		lines;
int		numsides;
side_t*		sides;
int		bmapwidth;
int		bmapheight;	// size in mapblocks
short*		blockmap;	// int for larger maps
short*		blockmaplump;		
fixed_t		bmaporgx;
fixed_t		bmaporgy;
mobj_t**	blocklinks;		
mapthing_t	deathmatchstarts[MAX_DEATHMATCH_STARTS];
mapthing_t*	deathmatch_p;
mapthing_t	playerstarts[MAXPLAYERS];
// p_setup.vars end // 
//  p_sight.vars begin // 
fixed_t		sightzstart;		// eye z of looker
fixed_t		topslope;
fixed_t		bottomslope;		// slopes to top and bottom of target
divline_t	strace;			// from t1 to t2
fixed_t		t2x;
fixed_t		t2y;
int		sightcounts[2];
// p_sight.vars end // 
//  p_spec.vars begin // 
anim_t2		anims[MAXANIMS];
anim_t2*		lastanim;
qboolean		levelTimer;
int		levelTimeCount;
int		levelFragCount;
short		numlinespecials;
line_t*		linespeciallist[MAXLINEANIMS];
// p_spec.vars end // 
//  p_switch.vars begin // 
int		switchlist[MAXSWITCHES * 2];
int		numswitches;
button_t        buttonlist[MAXBUTTONS];
// p_switch.vars end // 
//  p_tick.vars begin // 
int	leveltime;
thinker_t	thinkercap;
// p_tick.vars end // 
//  p_user.vars begin // 
qboolean		onground;
// p_user.vars end // 
//  r_bsp.vars begin // 
seg_t*		curline;
side_t*		sidedef;
line_t*		linedef;
sector_t*	frontsector;
sector_t*	backsector;
drawseg_t	drawsegs[MAXDRAWSEGS];
drawseg_t*	ds_p;
cliprange_t*	newend;
cliprange_t	solidsegs[MAXSEGS];
int	checkcoord[12][4];
// r_bsp.vars end // 
//  r_data.vars begin // 
int		firstflat;
int		lastflat;
int		numflats;
int		firstpatch;
int		lastpatch;
int		numpatches;
int		firstspritelump;
int		lastspritelump;
int		numspritelumps;
int*		flattranslation;
int*		texturetranslation;
fixed_t*	spritewidth;	
fixed_t*	spriteoffset;
fixed_t*	spritetopoffset;
lighttable_t	*colormaps;
int		flatmemory;
int		texturememory;
int		spritememory;
// r_data.vars end // 
//  r_draw.vars begin // 
byte*		viewimage; 
int		viewwidth;
int		scaledviewwidth;
int		viewheight;
int		viewwindowx;
int		viewwindowy; 
byte*		ylookup[MAXHEIGHT]; 
int		columnofs[MAXWIDTH]; 
byte		translations[3][256];	
lighttable_t*		dc_colormap; 
int			dc_x; 
int			dc_yl; 
int			dc_yh; 
fixed_t			dc_iscale; 
fixed_t			dc_texturemid;
byte*			dc_source;		
int			dccount;
int	fuzzoffset[FUZZTABLE];
int	fuzzpos ;
byte*	dc_translation;
byte*	translationtables;
int			ds_y; 
int			ds_x1; 
int			ds_x2;
lighttable_t*		ds_colormap; 
fixed_t			ds_xfrac; 
fixed_t			ds_yfrac; 
fixed_t			ds_xstep; 
fixed_t			ds_ystep;
byte*			ds_source;	
int			dscount;
// r_draw.vars end // 
//  r_main.vars begin // 
int			viewangleoffset;
int			validcount ;
lighttable_t*		fixedcolormap;
int			centerx;
int			centery;
fixed_t			centerxfrac;
fixed_t			centeryfrac;
fixed_t			projection;
int			framecount;	
int			sscount;
int			linecount;
int			loopcount;
fixed_t			viewx;
fixed_t			viewy;
fixed_t			viewz;
angle_t			viewangle;
fixed_t			viewcos;
fixed_t			viewsin;
player_t*		viewplayer;
int			detailshift;	
angle_t			clipangle;
int			viewangletox[FINEANGLES/2];
angle_t			xtoviewangle[SCREENWIDTH+1];
lighttable_t*		scalelight[LIGHTLEVELS][MAXLIGHTSCALE];
lighttable_t*		scalelightfixed[MAXLIGHTSCALE];
lighttable_t*		zlight[LIGHTLEVELS][MAXLIGHTZ];
int			extralight;			
qboolean		setsizeneeded;
int		setblocks;
int		setdetail;
// r_main.vars end // 
//  r_plane.vars begin // 
planefunction_t		floorfunc;
planefunction_t		ceilingfunc;
short			openings[MAXOPENINGS];
short*			lastopening;
short			floorclip[SCREENWIDTH];
short			ceilingclip[SCREENWIDTH];
int			spanstart[SCREENHEIGHT];
int			spanstop[SCREENHEIGHT];
lighttable_t**		planezlight;
fixed_t			planeheight;
fixed_t			yslope[SCREENHEIGHT];
fixed_t			distscale[SCREENWIDTH];
fixed_t			basexscale;
fixed_t			baseyscale;
fixed_t			cachedheight[SCREENHEIGHT];
fixed_t			cacheddistance[SCREENHEIGHT];
fixed_t			cachedxstep[SCREENHEIGHT];
fixed_t			cachedystep[SCREENHEIGHT];
// r_plane.vars end // 
//  r_segs.vars begin // 
qboolean		segtextured;	
qboolean		markfloor;	
qboolean		markceiling;
qboolean		maskedtexture;
int		toptexture;
int		bottomtexture;
int		midtexture;
angle_t		rw_normalangle;
int		rw_angle1;	
int		rw_x;
int		rw_stopx;
angle_t		rw_centerangle;
fixed_t		rw_offset;
fixed_t		rw_distance;
fixed_t		rw_scale;
fixed_t		rw_scalestep;
fixed_t		rw_midtexturemid;
fixed_t		rw_toptexturemid;
fixed_t		rw_bottomtexturemid;
int		worldtop;
int		worldbottom;
int		worldhigh;
int		worldlow;
fixed_t		pixhigh;
fixed_t		pixlow;
fixed_t		pixhighstep;
fixed_t		pixlowstep;
fixed_t		topfrac;
fixed_t		topstep;
fixed_t		bottomfrac;
fixed_t		bottomstep;
lighttable_t**	walllights;
short*		maskedtexturecol;
// r_segs.vars end // 
//  r_sky.vars begin // 
int			skyflatnum;
int			skytexture;
int			skytexturemid;
// r_sky.vars end // 
//  r_things.vars begin // 
fixed_t		pspritescale;
fixed_t		pspriteiscale;
lighttable_t**	spritelights;
short		negonearray[SCREENWIDTH];
short		screenheightarray[SCREENWIDTH];
spritedef_t*	sprites;
int		numsprites;
spriteframe_t	sprtemp[29];
int		maxframe;
vissprite_t	vissprites[MAXVISSPRITES];
vissprite_t*	vissprite_p;
int		newvissprite;
vissprite_t	overflowsprite;
short*		mfloorclip;
short*		mceilingclip;
fixed_t		spryscale;
fixed_t		sprtopscreen;
vissprite_t	vsprsortedhead;
// r_things.vars end // 
//  sounds.vars begin // 
musicinfo_t S_music[80];
// sounds.vars end // 
//  st_lib.vars begin // 
patch_t*		sttminus;
// st_lib.vars end // 
//  st_stuff.vars begin // 
player_t*	plyr; 
qboolean		st_firsttime;
int		veryfirsttime ;
int		lu_palette;
unsigned int	st_clock;
int		st_msgcounter;
st_chatstateenum_t	st_chatstate;
st_stateenum_t	st_gamestate;
qboolean		st_statusbaron;
qboolean		st_chat;
qboolean		st_oldchat;
qboolean		st_cursoron;
qboolean		st_notdeathmatch; 
qboolean		st_armson;
qboolean		st_fragson; 
patch_t*		sbar; // AffLANHACK IGNORE
patch_t*		tallnum[10]; // AffLANHACK IGNORE
patch_t*		tallpercent; // AffLANHACK IGNORE
patch_t*		shortnum[10]; // AffLANHACK IGNORE
patch_t*		keys[NUMCARDS];  // AffLANHACK IGNORE
patch_t*		faces[ST_NUMFACES]; // AffLANHACK IGNORE
patch_t*		faceback; // AffLANHACK IGNORE
patch_t*		armsbg; // AffLANHACK IGNORE
patch_t*		arms[6][2];  // AffLANHACK IGNORE
st_number_t	w_ready;
st_number_t	w_frags;
st_percent_t	w_health;
st_binicon_t	w_armsbg; 
st_multicon_t	w_arms[6];
st_multicon_t	w_faces; 
st_multicon_t	w_keyboxes[3];
st_percent_t	w_armor;
st_number_t	w_ammo[4];
st_number_t	w_maxammo[4]; 
int	st_fragscount;
int	st_oldhealth ;
qboolean	oldweaponsowned[NUMWEAPONS]; 
int	st_facecount ;
int	st_faceindex ;
int	keyboxes[3]; 
int	st_randomnumber;  
cheatseq_t	cheat_powerup[7]; 
int	lastcalc;
int	oldhealth ;
int	lastattackdown ;
int	priority ;
int	largeammo ;
int st_palette ;
qboolean	st_stopped ;
// st_stuff.vars end // 
//  s_sound.vars begin // 
channel_t*	channels;
qboolean		mus_paused;	
qboolean		mus_looping;
musicinfo_t*	mus_playing;
int			numChannels;	
int		nextcleanup;
// s_sound.vars end // 
//  v_video.vars begin // 
byte*				screens[5];	
int				dirtybox[4]; 
int	usegamma;
// v_video.vars end // 
//  wi_stuff.vars begin // 
anim_t epsd0animinfo[10];
anim_t epsd1animinfo[9];
anim_t epsd2animinfo[6];
anim_t* wi_stuff_anims[NUMEPISODES];
int NUMANIMS[NUMEPISODES];
const char* chat_macros[10];
int acceleratestage;
int me;
stateenum_t state;
wbstartstruct_t* wbs;
int cnt;  
int bcnt;
int firstrefresh; 
int cnt_kills[MAXPLAYERS];
int cnt_items[MAXPLAYERS];
int cnt_secret[MAXPLAYERS];
int cnt_time;
int cnt_par;
int cnt_pause;
int NUMCMAPS; 
patch_t* colon;
qboolean		snl_pointeron ;
int		dm_state;
int		dm_frags[MAXPLAYERS][MAXPLAYERS];
int		dm_totals[MAXPLAYERS];
int	cnt_frags[MAXPLAYERS];
int	dofrags;
int	ng_state;
int	sp_state;
// wi_stuff.vars end // 
//  w_wad.vars begin // 
int			reloadlump;
// w_wad.vars end // 
//  z_zone.vars begin // 
int sizes[NUM_ZONES+1];
memzone_t*	zones[NUM_ZONES] ;
int NumAlloc ;
// z_zone.vars end // 
// info vars begin //
state_t	states[NUMSTATES];
// info vars end //
// p_local begin //
byte*		rejectmatrix;
// p_local end //
// r_data begin //
int		s_numtextures;
texture_t**	s_textures;
int*			s_texturewidthmask;
// needed for texture pegging 
fixed_t*		s_textureheight;
short**			s_texturecolumnlump;
unsigned short**	s_texturecolumnofs;
byte**			s_texturecomposite;
int*			s_texturecompositesize;
// r_data end //
// r_plane begin //
visplane_t		visplanes[MAXVISPLANES]; 
visplane_t*		lastvisplane; 
visplane_t*		floorplane; 
visplane_t*		ceilingplane; 
// r_plane end //

// wi_stuff
// background (map of levels).
patch_t* bg; 

// You Are Here graphic
patch_t* yah[2];  

// splat
patch_t* splat; 

// %, : graphics
patch_t* percent; 

// 0-9 graphic
patch_t* num[10]; 

// minus sign
patch_t* wiminus; 

// "Finished!" graphics
patch_t* finished; 

// "Entering" graphic
patch_t* entering;  

// "secret"
patch_t* sp_secret; 

 // "Kills", "Scrt", "Items", "Frags"
patch_t* kills;
patch_t* secret;
patch_t* items;
patch_t* wistuff_frags;

// Time sucks.
patch_t* time;
patch_t* par;
patch_t* sucks;

// "killers", "victims"
patch_t* killers;
patch_t* victims; 

// "Total", your face, your dead face
patch_t* total;
patch_t* star;
patch_t* bstar;

// "red P[1..MAXPLAYERS]"
patch_t* wistuff_p[MAXPLAYERS];

// "gray P[1..MAXPLAYERS]"
patch_t* wistuff_bp[MAXPLAYERS];

 // Name graphics of each level (centered)
patch_t** lnames;

const char*		spritename;


