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

//  am_map.defs begin // 
#define REDS		(256-5*16)
#define REDRANGE	16
#define BLUES		(256-4*16+8)
#define BLUERANGE	8
#define GREENS		(7*16)
#define GREENRANGE	16
#define GRAYS		(6*16)
#define GRAYSRANGE	16
#define BROWNS		(4*16)
#define BROWNRANGE	16
#define YELLOWS		(256-32+7)
#define YELLOWRANGE	1
#define BLACK		0
#define WHITE		(256-47)
#define BACKGROUND	BLACK
#define YOURCOLORS	WHITE
#define YOURRANGE	0
#define WALLCOLORS	REDS
#define WALLRANGE	REDRANGE
#define TSWALLCOLORS	GRAYS
#define TSWALLRANGE	GRAYSRANGE
#define FDWALLCOLORS	BROWNS
#define FDWALLRANGE	BROWNRANGE
#define CDWALLCOLORS	YELLOWS
#define CDWALLRANGE	YELLOWRANGE
#define THINGCOLORS	GREENS
#define THINGRANGE	GREENRANGE
#define SECRETWALLCOLORS WALLCOLORS
#define SECRETWALLRANGE WALLRANGE
#define GRIDCOLORS	(GRAYS + GRAYSRANGE/2)
#define GRIDRANGE	0
#define XHAIRCOLORS	GRAYS
#define	FB		0
#define AM_PANDOWNKEY	KEY_DOWNARROW
#define AM_PANUPKEY	KEY_UPARROW
#define AM_PANRIGHTKEY	KEY_RIGHTARROW
#define AM_PANLEFTKEY	KEY_LEFTARROW
#define AM_ZOOMINKEY	K_EQUALS
#define AM_ZOOMOUTKEY	K_MINUS
#define AM_STARTKEY	KEY_TAB
#define AM_ENDKEY	KEY_TAB
#define AM_GOBIGKEY	K_0
#define AM_FOLLOWKEY	K_F
#define AM_GRIDKEY	K_G
#define AM_MARKKEY	K_M
#define AM_CLEARMARKKEY	K_C
#define AM_NUMMARKPOINTS 10
#define INITSCALEMTOF (.2*FRACUNIT)
#define F_PANINC	4
#define M_ZOOMIN        ((int) (1.02*FRACUNIT))
#define M_ZOOMOUT       ((int) (FRACUNIT/1.02))
#define FTOM(x) FixedMul(((x)<<16),::g->scale_ftom)
#define MTOF(x) (FixedMul((x),::g->scale_mtof)>>16)
#define CXMTOF(x)  (::g->f_x + MTOF((x)-::g->m_x))
#define CYMTOF(y)  (::g->f_y + (::g->f_h - MTOF((y)-::g->m_y)))
#define LINE_NEVERSEE ML_DONTDRAW
#define NUMPLYRLINES (sizeof(player_arrow)/sizeof(mline_t))
#define NUMCHEATPLYRLINES (sizeof(cheat_player_arrow)/sizeof(mline_t))
#define NUMTRIANGLEGUYLINES (sizeof(triangle_guy)/sizeof(mline_t))
#define NUMTHINTRIANGLEGUYLINES (sizeof(thintriangle_guy)/sizeof(mline_t))
#define DOOUTCODE(oc, mx, my) \
	(oc) = 0; \
	if ((my) < 0) (oc) |= TOP; \
	else if ((my) >= ::g->f_h) (oc) |= BOTTOM; \
	if ((mx) < 0) (oc) |= LEFT; \
	else if ((mx) >= ::g->f_w) (oc) |= RIGHT;
#define PUTDOT(xx,yy,cc) ::g->fb[(yy)*::g->f_w+(xx)]=(cc)
// am_map.defs end // 
//  d_main.defs begin // 
#define	BGCOLOR		7
#define	FGCOLOR		8
#define DOOMWADDIR "wads/"
// d_main.defs end // 
//  d_net.defs begin // 
#define	NCMD_EXIT		0x80000000
#define	NCMD_RETRANSMIT		0x40000000
#define	NCMD_SETUP		0x20000000
#define	NCMD_KILL		0x10000000	// kill game
#define	NCMD_CHECKSUM	 	0x0fffffff
#define	RESENDCOUNT	10
#define	PL_DRONE	0x80	// bit flag in doomdata->player
// d_net.defs end // 
//  f_finale.defs begin // 
#define	TEXTSPEED	3
#define	TEXTWAIT	250
// f_finale.defs end // 
//  g_game.defs begin // 
#define SAVESTRINGSIZE	64
#define MAXPLMOVE		(::g->forwardmove[1]) 
#define TURBOTHRESHOLD	0x32
#define SLOWTURNTICS	6 
#define NUMKEYS		256 
#define	BODYQUESIZE	32
#define VERSIONSIZE		16 
#define DEMOMARKER		0x80
// g_game.defs end // 
//  hu_lib.defs begin // 
#define noterased ::g->viewwindowx
// hu_lib.defs end // 
//  hu_stuff.defs begin // 
#define HU_TITLE	(mapnames[(::g->gameepisode-1)*9+::g->gamemap-1])
#define HU_TITLE2	(mapnames2[::g->gamemap-1])
#define HU_TITLEP	(mapnamesp[::g->gamemap-1])
#define HU_TITLET	(mapnamest[::g->gamemap-1])
#define HU_TITLEHEIGHT	1
#define HU_TITLEX	0
#define HU_TITLEY	(167 - SHORT(::g->hu_font[0]->height))
#define HU_INPUTTOGGLE	K_T
#define HU_INPUTX	HU_MSGX
#define HU_INPUTY	(HU_MSGY + HU_MSGHEIGHT*(SHORT(::g->hu_font[0]->height) +1))
#define HU_INPUTWIDTH	64
#define HU_INPUTHEIGHT	1
#define QUEUESIZE		128
// hu_stuff.defs end // 
//  i_net.defs begin // 
// SMF
/*
#define ntohl(x) \
        ((unsigned long int)((((unsigned long int)(x) & 0x000000ffU) << 24) | \
                             (((unsigned long int)(x) & 0x0000ff00U) <<  8) | \
                             (((unsigned long int)(x) & 0x00ff0000U) >>  8) | \
                             (((unsigned long int)(x) & 0xff000000U) >> 24)))
#define ntohs(x) \
        ((unsigned short int)((((unsigned short int)(x) & 0x00ff) << 8) | \
                              (((unsigned short int)(x) & 0xff00) >> 8))) \
#define htonl(x) ntohl(x)
#define htons(x) ntohs(x)
// i_net.defs end // 
//  i_net_xbox.defs begin // 
#define ntohl(x) \
	((unsigned long int)((((unsigned long int)(x) & 0x000000ffU) << 24) | \
	(((unsigned long int)(x) & 0x0000ff00U) <<  8) | \
	(((unsigned long int)(x) & 0x00ff0000U) >>  8) | \
	(((unsigned long int)(x) & 0xff000000U) >> 24)))
#define ntohs(x) \
	((unsigned short int)((((unsigned short int)(x) & 0x00ff) << 8) | \
	(((unsigned short int)(x) & 0xff00) >> 8))) \

#define htonl(x) ntohl(x)
#define htons(x) ntohs(x)
*/	  

#define IPPORT_USERRESERVED	5000
// i_net_xbox.defs end // 
//  i_sound_xbox.defs begin // 
#define SAMPLECOUNT		512
#define NUM_SOUNDBUFFERS		64
#define BUFMUL                  4
#define MIXBUFFERSIZE		(SAMPLECOUNT*BUFMUL)
// i_sound_xbox.defs end // 
//  i_video_xbox.defs begin // 
//#define TEXTUREWIDTH	512
//#define TEXTUREHEIGHT	256
// i_video_xbox.defs end // 
//  mus2midi.defs begin // 
#define MUSEVENT_KEYOFF	0
#define MUSEVENT_KEYON	1
#define MUSEVENT_PITCHWHEEL	2
#define MUSEVENT_CHANNELMODE	3
#define MUSEVENT_CONTROLLERCHANGE	4
#define MUSEVENT_END	6
#define MIDI_MAXCHANNELS	16
#define MIDIHEADERSIZE 14
// mus2midi.defs end // 
//  m_menu.defs begin // 
#define SAVESTRINGSIZE 	64
#define SKULLXOFF		-32
#define LINEHEIGHT		16
// m_menu.defs end // 
//  p_enemy.defs begin // 
#define MAXSPECIALCROSS	8
#define	FATSPREAD	(ANG90/8)
#define	SKULLSPEED		(20*FRACUNIT)
// p_enemy.defs end // 
//  p_inter.defs begin // 
#define BONUSADD	6
// p_inter.defs end // 
//  p_map.defs begin // 
#define MAXSPECIALCROSS		8
// p_map.defs end // 
//  p_mobj.defs begin // 
#define STOPSPEED		0x1000
#define FRICTION		0xe800
// p_mobj.defs end // 
//  p_pspr.defs begin // 
#define LOWERSPEED		FRACUNIT*6
#define RAISESPEED		FRACUNIT*6
#define WEAPONBOTTOM	128*FRACUNIT
#define WEAPONTOP		32*FRACUNIT
#define BFGCELLS		40		
// p_pspr.defs end // 
//  p_saveg.defs begin // 
#define PADSAVEP()	::g->save_p += (4 - ((intptr_t) ::g->save_p & 3)) & 3
// p_saveg.defs end // 
//  p_setup.defs begin // 
#define MAX_DEATHMATCH_STARTS	10
// p_setup.defs end // 
//  p_spec.defs begin // 
#define MAXANIMS                32
#define MAXLINEANIMS            64
#define MAX_ADJOINING_SECTORS    	20
// p_spec.defs end // 
//  p_user.defs begin // 
#define INVERSECOLORMAP		32

// DHM - NERVE :: MAXBOB reduced 25%
//#define MAXBOB	0x100000
#define MAXBOB	0xC0000

#define ANG5   	(ANG90/18)
// p_user.defs end // 
//  r_bsp.defs begin // 
#define MAXSEGS		32
// r_bsp.defs end // 
//  r_draw.defs begin // 
//#define MAXWIDTH			1120
//#define MAXHEIGHT			832
#define SBARHEIGHT		32 * GLOBAL_IMAGE_SCALER
#define FUZZTABLE		50 
#define FUZZOFF	(SCREENWIDTH)
// r_draw.defs end // 
//  r_main.defs begin // 
#define FIELDOFVIEW		2048	
#define DISTMAP		2
// r_main.defs end // 
//  r_plane.defs begin // 
//#define MAXVISPLANES	128
#define MAXVISPLANES	384
#define MAXOPENINGS	SCREENWIDTH*64
// r_plane.defs end // 
//  r_segs.defs begin // 
#define HEIGHTBITS		12
#define HEIGHTUNIT		(1<<HEIGHTBITS)
// r_segs.defs end // 
//  r_things.defs begin // 
#define MINZ				(FRACUNIT*4)
#define BASEYCENTER			100
// r_things.defs end // 
//  st_stuff.defs begin // 
#define STARTREDPALS		1
#define STARTBONUSPALS		9
#define NUMREDPALS			8
#define NUMBONUSPALS		4
#define RADIATIONPAL		13
#define ST_FACEPROBABILITY		96
#define ST_TOGGLECHAT		KEY_ENTER
#define ST_X				0
#define ST_X2				104
#define ST_FX  			143
#define ST_FY  			169
#define ST_TALLNUMWIDTH		(::g->tallnum[0]->width)
#define ST_NUMPAINFACES		5
#define ST_NUMSTRAIGHTFACES	3
#define ST_NUMTURNFACES		2
#define ST_NUMSPECIALFACES		3
#define ST_FACESTRIDE \
	(ST_NUMSTRAIGHTFACES+ST_NUMTURNFACES+ST_NUMSPECIALFACES)
#define ST_NUMEXTRAFACES		2
#define ST_NUMFACES \
	(ST_FACESTRIDE*ST_NUMPAINFACES+ST_NUMEXTRAFACES)
#define ST_TURNOFFSET		(ST_NUMSTRAIGHTFACES)
#define ST_OUCHOFFSET		(ST_TURNOFFSET + ST_NUMTURNFACES)
#define ST_EVILGRINOFFSET		(ST_OUCHOFFSET + 1)
#define ST_RAMPAGEOFFSET		(ST_EVILGRINOFFSET + 1)
#define ST_GODFACE			(ST_NUMPAINFACES*ST_FACESTRIDE)
#define ST_DEADFACE			(ST_GODFACE+1)
#define ST_FACESX			143
#define ST_FACESY			168
#define ST_EVILGRINCOUNT		(2*TICRATE)
#define ST_STRAIGHTFACECOUNT	(TICRATE/2)
#define ST_TURNCOUNT		(1*TICRATE)
#define ST_OUCHCOUNT		(1*TICRATE)
#define ST_RAMPAGEDELAY		(2*TICRATE)
#define ST_MUCHPAIN			20
#define ST_AMMOWIDTH		3	
#define ST_AMMOX			44
#define ST_AMMOY			171
#define ST_HEALTHWIDTH		3	
#define ST_HEALTHX			90
#define ST_HEALTHY			171
#define ST_ARMSX			111
#define ST_ARMSY			172
#define ST_ARMSBGX			104
#define ST_ARMSBGY			168
#define ST_ARMSXSPACE		12
#define ST_ARMSYSPACE		10
#define ST_FRAGSX			138
#define ST_FRAGSY			171	
#define ST_FRAGSWIDTH		2
#define ST_ARMORWIDTH		3
#define ST_ARMORX			221
#define ST_ARMORY			171
#define ST_KEY0WIDTH		8
#define ST_KEY0HEIGHT		5
#define ST_KEY0X			239
#define ST_KEY0Y			171
#define ST_KEY1WIDTH		ST_KEY0WIDTH
#define ST_KEY1X			239
#define ST_KEY1Y			181
#define ST_KEY2WIDTH		ST_KEY0WIDTH
#define ST_KEY2X			239
#define ST_KEY2Y			191
#define ST_AMMO0WIDTH		3
#define ST_AMMO0HEIGHT		6
#define ST_AMMO0X			288
#define ST_AMMO0Y			173
#define ST_AMMO1WIDTH		ST_AMMO0WIDTH
#define ST_AMMO1X			288
#define ST_AMMO1Y			179
#define ST_AMMO2WIDTH		ST_AMMO0WIDTH
#define ST_AMMO2X			288
#define ST_AMMO2Y			191
#define ST_AMMO3WIDTH		ST_AMMO0WIDTH
#define ST_AMMO3X			288
#define ST_AMMO3Y			185
#define ST_MAXAMMO0WIDTH		3
#define ST_MAXAMMO0HEIGHT		5
#define ST_MAXAMMO0X		314
#define ST_MAXAMMO0Y		173
#define ST_MAXAMMO1WIDTH		ST_MAXAMMO0WIDTH
#define ST_MAXAMMO1X		314
#define ST_MAXAMMO1Y		179
#define ST_MAXAMMO2WIDTH		ST_MAXAMMO0WIDTH
#define ST_MAXAMMO2X		314
#define ST_MAXAMMO2Y		191
#define ST_MAXAMMO3WIDTH		ST_MAXAMMO0WIDTH
#define ST_MAXAMMO3X		314
#define ST_MAXAMMO3Y		185
#define ST_WEAPON0X			110 
#define ST_WEAPON0Y			172
#define ST_WEAPON1X			122 
#define ST_WEAPON1Y			172
#define ST_WEAPON2X			134 
#define ST_WEAPON2Y			172
#define ST_WEAPON3X			110 
#define ST_WEAPON3Y			181
#define ST_WEAPON4X			122 
#define ST_WEAPON4Y			181
#define ST_WEAPON5X			134
#define ST_WEAPON5Y			181
#define ST_WPNSX			109 
#define ST_WPNSY			191
#define ST_DETHX			109
#define ST_DETHY			191
#define ST_MSGTEXTX			0
#define ST_MSGTEXTY			0
#define ST_MSGWIDTH			52
#define ST_MSGHEIGHT		1
#define ST_OUTTEXTX			0
#define ST_OUTTEXTY			6
#define ST_OUTWIDTH			52 
#define ST_OUTHEIGHT		1
#define ST_MAPWIDTH	\
	(strlen(mapnames[(::g->gameepisode-1)*9+(::g->gamemap-1)]))
#define ST_MAPTITLEX \
	(SCREENWIDTH - ST_MAPWIDTH * ST_CHATFONTWIDTH)
#define ST_MAPTITLEY		0
#define ST_MAPHEIGHT		1
// st_stuff.defs end // 
//  s_sound.defs begin // 
#define S_MAX_VOLUME		127
#define S_CLIPPING_DIST		(1200*0x10000)
#define S_CLOSE_DIST		(160*0x10000)
#define S_ATTENUATOR		((S_CLIPPING_DIST-S_CLOSE_DIST)>>FRACBITS)
#define NORM_VOLUME    		snd_MaxVolume
#define NORM_PITCH     		128
#define NORM_PRIORITY		64
#define NORM_SEP		128
#define S_PITCH_PERTURB		1
#define S_STEREO_SWING		(96*0x10000)
#define S_IFRACVOL		30
#define NA			0
#define S_NUMCHANNELS		256
// s_sound.defs end // 
//  wi_stuff.defs begin // 
#define NUMEPISODES	4
#define NUMMAPS		9
#define WI_TITLEY		2
#define WI_SPACINGY    		33
#define SP_STATSX		50
#define SP_STATSY		50
#define SP_TIMEX		16
#define SP_TIMEY		(ORIGINAL_HEIGHT-32)
#define NG_STATSY		50
#define NG_STATSX		(32 + SHORT(::g->star->width)/2 + 32*!::g->dofrags)
#define NG_SPACINGX    		64
#define DM_MATRIXX		42
#define DM_MATRIXY		68
#define DM_SPACINGX		40
#define DM_TOTALSX		269
#define DM_KILLERSX		10
#define DM_KILLERSY		100
#define DM_VICTIMSX    		5
#define DM_VICTIMSY		50
#define FB 0
#define SP_KILLS		0
#define SP_ITEMS		2
#define SP_SECRET		4
#define SP_FRAGS		6 
#define SP_TIME			8 
#define SP_PAR			ST_TIME
#define SP_PAUSE		1
#define SHOWNEXTLOCDELAY	4
// wi_stuff.defs end // 
//  w_wad.defs begin // 

// w_wad.defs end // 
//  z_zone.defs begin // 
#define ZONEID	0x1d4a11
#define NUM_ZONES 11
#define MINFRAGMENT		64
#define NO_SHARE_LUMPS
// z_zone.defs end // 
