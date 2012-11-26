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

	memset(::g, 0, sizeof(*::g));
//  am_map.constructs begin // 
	::g->cheating = 0;
	::g->grid = 0;
	::g->leveljuststarted = 1; 	// kluge until AM_LevelInit() is called
	::g->automapactive = false;
	::g->finit_width = SCREENWIDTH;
	::g->finit_height = SCREENHEIGHT - (32 * GLOBAL_IMAGE_SCALER);
	::g->scale_mtof = (fixed_t)INITSCALEMTOF;
	::g->markpointnum = 0; // next point to be assigned
	::g->followplayer = 1; // specifies whether to follow the player around
	::g->stopped = true;
	::g->lastlevel = -1;
	::g->lastepisode = -1;
	::g->cheatstate=0;
	::g->bigstate=0;
	::g->nexttic = 0;
	::g->litelevelscnt = 0;
// am_map.constructs end // 
//  doomstat.constructs begin // 
	::g->gamemode = indetermined;
	::g->gamemission = doom;
	::g->language = english;
// doomstat.constructs end // 
//  d_main.constructs begin // 
	::g->singletics = false; // debug flag to cancel adaptiveness
	::g->oldgamestate = (gamestate_t)-1;
	::g->wipegamestate = GS_DEMOSCREEN;
	::g->viewactivestate = false;
	::g->menuactivestate = false;
	::g->inhelpscreensstate = false;
	::g->fullscreen = false;
	::g->wipe = false;
	::g->wipedone = true;
// d_main.constructs end // 
//  d_net.constructs begin // 
doomcom_t temp_doomcom = {
	0
};
memcpy( &::g->doomcom, &temp_doomcom, sizeof(temp_doomcom) );
// d_net.constructs end // 
//  f_wipe.constructs begin // 
	::g->go = 0;
// f_wipe.constructs end // 
//  g_game.constructs begin // 
	::g->precache = true;        // if true, load all graphics at start 
fixed_t	 temp_forwardmove[2] = {
0x19, 0x32
}; 
memcpy( ::g->forwardmove, temp_forwardmove, sizeof(temp_forwardmove) );
fixed_t	 temp_sidemove[2] = {
0x18, 0x28
}; 
memcpy( ::g->sidemove, temp_sidemove, sizeof(temp_sidemove) );
fixed_t	 temp_angleturn[3] = {
640, 1280, 320 // + slow turn
};	 
memcpy( ::g->angleturn, temp_angleturn, sizeof(temp_angleturn) );
	::g->mousebuttons = &::g->mousearray[1];		// allow [-1]
	::g->joybuttons = &::g->joyarray[1];		// allow [-1] 
// g_game.constructs end // 
//  hu_lib.constructs begin // 
	::g->lastautomapactive = true;
// hu_lib.constructs end // 
//  hu_stuff.constructs begin // 
	::g->always_off = false;
	::g->headsupactive = false;
	::g->head = 0;
	::g->tail = 0;
	::g->shiftdown = false;
	::g->altdown = false;
	::g->num_nobrainers = 0;
// hu_stuff.constructs end // 
//  i_input.constructs begin // 
// i_input.constructs end // 
//  i_system.constructs begin // 
	::g->mb_used = 2;
	::g->current_time = 0;
// i_system.constructs end // 
//  m_cheat.constructs begin // 
	::g->firsttime = 1;
	::g->usedcheatbuffer = 0;
// m_cheat.constructs end // 
//  m_menu.constructs begin // 

menuitem_t temp_QuitMenu[3] = {
	{1,"M_ACPT", M_ExitGame,'a'},
	{1,"M_CAN", M_CancelExit,'c'},
	{1,"M_CHG", M_GameSelection,'g'}	
};
memcpy( ::g->QuitMenu, temp_QuitMenu, sizeof(temp_QuitMenu) );
menu_t  temp_QuitDef = {
	qut_end,		// # of menu items
	&::g->MainDef,		// previous menu
	::g->QuitMenu,	// menuitem_t ->
	M_DrawQuit,	// drawing routine ->
	48,63,              // x,y
	g_accept			// lastOn
};
memcpy( &::g->QuitDef, &temp_QuitDef, sizeof(temp_QuitDef) );

menuitem_t temp_MainMenu[5]=
{
	{1,"M_NGAME",M_NewGame,'n'},
	{1,"M_OPTION",M_Options,'o'},
	{1,"M_LOADG",M_LoadGame,'l'},
	{1,"M_SAVEG",M_SaveGame,'m'},
	// Another hickup with Special edition.
	//{1,"M_RDTHIS",M_ReadThis,'r'},
	{1,"M_QUITG",M_QuitDOOM,'q'}
};
memcpy( &::g->MainMenu, temp_MainMenu, sizeof(temp_MainMenu) );
menu_t  temp_MainDef = {
	main_end,
		NULL,
		::g->MainMenu,
		M_DrawMainMenu,
		97,64,
		0
};


memcpy( &::g->MainDef, &temp_MainDef, sizeof(temp_MainDef) );
menuitem_t temp_EpisodeMenu[4] = {
	{1,"M_EPI1", M_Episode,'k'},
	{1,"M_EPI2", M_Episode,'t'},
	{1,"M_EPI3", M_Episode,'i'},
	{1,"M_EPI4", M_Episode,'t'}
};
memcpy( ::g->EpisodeMenu, temp_EpisodeMenu, sizeof(temp_EpisodeMenu) );
menu_t  temp_EpiDef = {
	ep_end,		// # of menu items
		&::g->MainDef,		// previous menu
		::g->EpisodeMenu,	// menuitem_t ->
		M_DrawEpisode,	// drawing routine ->
		48,63,              // x,y
		ep1			// lastOn
};
memcpy( &::g->EpiDef, &temp_EpiDef, sizeof(temp_EpiDef) );

menuitem_t temp_ExpansionMenu[2] = {
	{1,"M_EPI1", M_Expansion,'h'},
	{1,"M_EPI2", M_Expansion,'n'},
};
memcpy( ::g->ExpansionMenu, temp_ExpansionMenu, sizeof(temp_ExpansionMenu) );
menu_t  temp_ExpDef = {
	ex_end,		// # of menu items
	&::g->MainDef,		// previous menu
	::g->ExpansionMenu,	// menuitem_t ->
	M_DrawEpisode,	// drawing routine ->
	48,63,              // x,y
	ex1			// lastOn
};
memcpy( &::g->ExpDef, &temp_ExpDef, sizeof(temp_ExpDef) );

menuitem_t temp_LoadExpMenu[2] = {
	{1,"M_EPI1", M_LoadExpansion,'h'},
	{1,"M_EPI2", M_LoadExpansion,'n'},
};
memcpy( ::g->LoadExpMenu, temp_LoadExpMenu, sizeof(temp_LoadExpMenu) );
menu_t  temp_LoadExpDef = {
	ex_end,		// # of menu items
	&::g->MainDef,		// previous menu
	::g->LoadExpMenu,	// menuitem_t ->
	M_DrawEpisode,	// drawing routine ->
	48,63,              // x,y
	ex1			// lastOn
};
memcpy( &::g->LoadExpDef, &temp_LoadExpDef, sizeof(temp_LoadExpDef) );

menuitem_t temp_NewGameMenu[5] = {
	{1,"M_JKILL",	M_ChooseSkill, 'i'},
	{1,"M_ROUGH",	M_ChooseSkill, 'h'},
	{1,"M_HURT",	M_ChooseSkill, 'h'},
	{1,"M_ULTRA",	M_ChooseSkill, 'u'},
	{1,"M_NMARE",	M_ChooseSkill, 'n'}
};
memcpy( ::g->NewGameMenu, temp_NewGameMenu, sizeof(temp_NewGameMenu) );
menu_t  temp_NewDef = {
	newg_end,		// # of menu items
		&::g->EpiDef,		// previous menu
		::g->NewGameMenu,	// menuitem_t ->
		M_DrawNewGame,	// drawing routine ->
		48,63,              // x,y
		hurtme		// lastOn
};
memcpy( &::g->NewDef, &temp_NewDef, sizeof(temp_NewDef) );
menuitem_t temp_OptionsMenu[8] = {
	{1,"M_GDHIGH",	M_FullScreen,'f'},
	{1,"M_SCRNSZ",	M_ChangeGPad,'m'},
	{1,"M_MESSG",	M_ChangeMessages,'m'},
	//{1,"M_DETAIL",	M_ChangeDetail,'g'},
	//{2,"M_SCRNSZ",	M_SizeDisplay,'s'},
	{-1,"",0},
	{2,"M_MSENS",	M_ChangeSensitivity,'m'},
	{-1,"",0},
	{1,"M_SVOL",	M_Sound,'s'}
};
memcpy( ::g->OptionsMenu, temp_OptionsMenu, sizeof(temp_OptionsMenu) );
menu_t  temp_OptionsDef = {
	opt_end,
		&::g->MainDef,
		::g->OptionsMenu,
		M_DrawOptions,
		60,37,
		0
};
memcpy( &::g->OptionsDef, &temp_OptionsDef, sizeof(temp_OptionsDef) );
menuitem_t temp_SoundMenu[4] = {
	{2,"M_SFXVOL",M_SfxVol,'s'},
	{-1,"",0},
	{2,"M_MUSVOL",M_MusicVol,'m'},
	{-1,"",0}
};
memcpy( ::g->SoundMenu, temp_SoundMenu, sizeof(temp_SoundMenu) );
menu_t  temp_SoundDef = {
	sound_end,
		&::g->OptionsDef,
		::g->SoundMenu,
		M_DrawSound,
		80,64,
		0
};
memcpy( &::g->SoundDef, &temp_SoundDef, sizeof(temp_SoundDef) );
menuitem_t temp_LoadMenu[6] = {
	{1,"", M_LoadSelect,'1'},
	{1,"", M_LoadSelect,'2'},
	{1,"", M_LoadSelect,'3'},
	{1,"", M_LoadSelect,'4'},
	{1,"", M_LoadSelect,'5'},
	{1,"", M_LoadSelect,'6'}
};
memcpy( ::g->LoadMenu, temp_LoadMenu, sizeof(temp_LoadMenu) );
menu_t  temp_LoadDef = {
	load_end,
		&::g->MainDef,
		::g->LoadMenu,
		M_DrawLoad,
		80,54,
		0
};
memcpy( &::g->LoadDef, &temp_LoadDef, sizeof(temp_LoadDef) );
menuitem_t temp_SaveMenu[6] = {
	{1,"", M_SaveSelect,'1'},
	{1,"", M_SaveSelect,'2'},
	{1,"", M_SaveSelect,'3'},
	{1,"", M_SaveSelect,'4'},
	{1,"", M_SaveSelect,'5'},
	{1,"", M_SaveSelect,'6'}
};
memcpy( ::g->SaveMenu, temp_SaveMenu, sizeof(temp_SaveMenu) );
menu_t  temp_SaveDef = {
	load_end,
		&::g->MainDef,
		::g->SaveMenu,
		M_DrawSave,
		80,54,
		0
};
memcpy( &::g->SaveDef, &temp_SaveDef, sizeof(temp_SaveDef) );
int     temp_quitsounds[8] = {
	sfx_pldeth,
		sfx_dmpain,
		sfx_popain,
		sfx_slop,
		sfx_telept,
		sfx_posit1,
		sfx_posit3,
		sfx_sgtatk
};
memcpy( ::g->quitsounds, temp_quitsounds, sizeof(temp_quitsounds) );
int     temp_quitsounds2[8] = {
	sfx_vilact,
		sfx_getpow,
		sfx_boscub,
		sfx_slop,
		sfx_skeswg,
		sfx_kntdth,
		sfx_bspact,
		sfx_sgtatk
};
memcpy( ::g->quitsounds2, temp_quitsounds2, sizeof(temp_quitsounds2) );
	::g->joywait = 0;
	::g->mousewait = 0;
	::g->mmenu_mousey = 0;
	::g->lasty = 0;
	::g->mmenu_mousex = 0;
	::g->lastx = 0;
// m_menu.constructs end // 
//  m_misc.constructs begin // 
	::g->g_pszSaveFile = "\\save.dat";
	::g->g_pszImagePath = "d:\\saveimage.xbx";
	::g->g_pszImageMeta = "saveimage.xbx";
	extern const char* const temp_chat_macros[];
	for (int i = 0; i < 10; i++)
	{
		chat_macros[i] = temp_chat_macros[i];
	}
default_t temp_defaults[35] = {
    default_t( "mouse_sensitivity",&::g->mouseSensitivity, 7 ),
   
    default_t( "show_messages",&::g->showMessages, 1 ),
    
    default_t( "key_right",&::g->key_right, KEY_RIGHTARROW ),
    default_t( "key_left",&::g->key_left, KEY_LEFTARROW ),
    default_t( "key_up",&::g->key_up, KEY_UPARROW ),
    default_t( "key_down",&::g->key_down, KEY_DOWNARROW ),
    default_t( "key_strafeleft",&::g->key_strafeleft, ',' ),
    default_t( "key_straferight",&::g->key_straferight, '.' ),

    default_t( "key_fire",&::g->key_fire, KEY_RCTRL ),
    default_t( "key_use",&::g->key_use, ' ' ),
    default_t( "key_strafe",&::g->key_strafe, KEY_RALT ),
    default_t( "key_speed",&::g->key_speed, KEY_RSHIFT ),

    default_t( "use_mouse",&::g->usemouse, 1 ),
    default_t( "mouseb_fire",&::g->mousebfire,0 ),
    default_t( "mouseb_strafe",&::g->mousebstrafe,1 ),
    default_t( "mouseb_forward",&::g->mousebforward,2 ),

    default_t( "use_joystick",&::g->usejoystick, 0 ),
    default_t( "joyb_fire",&::g->joybfire,0 ),
    default_t( "joyb_strafe",&::g->joybstrafe,1 ),
    default_t( "joyb_use",&::g->joybuse,3 ),
    default_t( "joyb_speed",&::g->joybspeed,2 ),

    default_t( "screenblocks",&::g->screenblocks, 10 ),
    default_t( "detaillevel",&::g->detailLevel, 0 ),

    default_t( "snd_channels",&::g->numChannels, S_NUMCHANNELS ),



    default_t( "usegamma",&::g->usegamma, 0 ),

	default_t( "chatmacro0", &::g->chat_macros[0], HUSTR_CHATMACRO0 ),
    default_t( "chatmacro1", &::g->chat_macros[1], HUSTR_CHATMACRO1 ),
    default_t( "chatmacro2", &::g->chat_macros[2], HUSTR_CHATMACRO2 ),
    default_t( "chatmacro3", &::g->chat_macros[3], HUSTR_CHATMACRO3 ),
    default_t( "chatmacro4", &::g->chat_macros[4], HUSTR_CHATMACRO4 ),
    default_t( "chatmacro5", &::g->chat_macros[5], HUSTR_CHATMACRO5 ),
    default_t( "chatmacro6", &::g->chat_macros[6], HUSTR_CHATMACRO6 ),
    default_t( "chatmacro7", &::g->chat_macros[7], HUSTR_CHATMACRO7 ),
    default_t( "chatmacro8", &::g->chat_macros[8], HUSTR_CHATMACRO8 ),
    default_t( "chatmacro9", &::g->chat_macros[9], HUSTR_CHATMACRO9 )

};
memcpy( ::g->defaults, temp_defaults, sizeof(temp_defaults) );
// m_misc.constructs end // 
//  m_random.constructs begin // 
	::g->rndindex = 0;
	::g->prndindex = 0;
// m_random.constructs end // 
//  p_enemy.constructs begin // 
	::g->TRACEANGLE = 0xc000000;
	::g->easy = 0;
// p_enemy.constructs end // 
//  r_bsp.constructs begin // 
int temp_checkcoord[12][4] = {
    {3,0,2,1},
    {3,0,2,0},
    {3,1,2,0},
    {0},
    {2,0,2,1},
    {0,0,0,0},
    {3,1,3,0},
    {0},
    {2,0,3,1},
    {2,1,3,1},
    {2,1,3,0}
};
memcpy( ::g->checkcoord, temp_checkcoord, sizeof(temp_checkcoord) );
// r_bsp.constructs end // 
//  r_draw.constructs begin // 
int temp_fuzzoffset[FUZZTABLE] = {
	FUZZOFF,-FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,
		FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,
		FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,
		FUZZOFF,-FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,
		FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,-FUZZOFF,FUZZOFF,
		FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,
		FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF 
}; 
memcpy( ::g->fuzzoffset, temp_fuzzoffset, sizeof(temp_fuzzoffset) );
	::g->fuzzpos = 0; 
// r_draw.constructs end // 
//  r_main.constructs begin // 
	::g->validcount = 1;		
// r_main.constructs end // 
//  sounds.constructs begin // 
musicinfo_t temp_S_music[80] = {
    { 0 },
    { "e1m1", 0 },
    { "e1m2", 0 },
    { "e1m3", 0 },
    { "e1m4", 0 },
    { "e1m5", 0 },
    { "e1m6", 0 },
    { "e1m7", 0 },
    { "e1m8", 0 },
    { "e1m9", 0 },
    { "e2m1", 0 },
    { "e2m2", 0 },
    { "e2m3", 0 },
    { "e2m4", 0 },
    { "e2m5", 0 },
    { "e2m6", 0 },
    { "e2m7", 0 },
    { "e2m8", 0 },
    { "e2m9", 0 },
    { "e3m1", 0 },
    { "e3m2", 0 },
    { "e3m3", 0 },
    { "e3m4", 0 },
    { "e3m5", 0 },
    { "e3m6", 0 },
    { "e3m7", 0 },
    { "e3m8", 0 },
    { "e3m9", 0 },
    { "inter", 0 },
    { "intro", 0 },
    { "bunny", 0 },
    { "victor", 0 },
    { "introa", 0 },
    { "runnin", 0 },
    { "stalks", 0 },
    { "countd", 0 },
    { "betwee", 0 },
    { "doom", 0 },
    { "the_da", 0 },
    { "shawn", 0 },
    { "ddtblu", 0 },
    { "in_cit", 0 },
    { "dead", 0 },
    { "stlks2", 0 },
    { "theda2", 0 },
    { "doom2", 0 },
    { "ddtbl2", 0 },
    { "runni2", 0 },
    { "dead2", 0 },
    { "stlks3", 0 },
    { "romero", 0 },
    { "shawn2", 0 },
    { "messag", 0 },
    { "count2", 0 },
    { "ddtbl3", 0 },
    { "ampie", 0 },
    { "theda3", 0 },
    { "adrian", 0 },
    { "messg2", 0 },
    { "romer2", 0 },
    { "tense", 0 },
    { "shawn3", 0 },
    { "openin", 0 },
    { "evil", 0 },
    { "ultima", 0 },
    { "read_m", 0 },
    { "dm2ttl", 0 },
    { "dm2int", 0 } 
};
memcpy( ::g->S_music, temp_S_music, sizeof(temp_S_music) );
// sounds.constructs end // 
//  st_stuff.constructs begin // 
	::g->veryfirsttime = 1;
	::g->st_msgcounter=0;
	::g->st_oldhealth = -1;
	::g->st_facecount = 0;
	::g->st_faceindex = 0;
	::g->oldhealth = -1;
	::g->lastattackdown = -1;
	::g->priority = 0;
	::g->largeammo = 1994; // means "n/a"
	::g->st_palette = 0;
	::g->st_stopped = true;
// st_stuff.constructs end // 
//  s_sound.constructs begin //
	::g->mus_playing=0;
// s_sound.constructs end // 
//  wi_stuff.constructs begin // 
int temp_NUMANIMS[NUMEPISODES] = {
    sizeof(epsd0animinfo)/sizeof(anim_t),
    sizeof(epsd1animinfo)/sizeof(anim_t),
    sizeof(epsd2animinfo)/sizeof(anim_t)
};
memcpy( ::g->NUMANIMS, temp_NUMANIMS, sizeof(temp_NUMANIMS) );
	::g->snl_pointeron = false;
	extern const anim_t temp_epsd0animinfo[10];
	extern const anim_t temp_epsd1animinfo[9];
	extern const anim_t temp_epsd2animinfo[6];
	memcpy(::g->epsd0animinfo, temp_epsd0animinfo, sizeof(temp_epsd0animinfo));
	memcpy(::g->epsd1animinfo, temp_epsd1animinfo, sizeof(temp_epsd1animinfo));
	memcpy(::g->epsd2animinfo, temp_epsd2animinfo, sizeof(temp_epsd2animinfo));
	wi_stuff_anims[0] = ::g->epsd0animinfo;
	wi_stuff_anims[1] = ::g->epsd1animinfo;
	wi_stuff_anims[2] = ::g->epsd2animinfo;
// wi_stuff.constructs end // 
//  z_zone.constructs begin // 
	::g->zones[NUM_ZONES] = NULL;
	::g->NumAlloc = 0;
// z_zone.constructs end // 
// info constructs begin //
	extern const state_t	tempStates[NUMSTATES];
	memcpy(::g->states, tempStates, sizeof(tempStates));
// info constructs end //
// p_local begin //
	::g->rejectmatrix = NULL;
// p_local end //
// r_data begin //]
	::g->s_numtextures = 0;
// r_data end //


