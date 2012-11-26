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
#include <stdlib.h>

#include "i_system.h"
#include "i_sound.h"
#include "sounds.h"
#include "s_sound.h"

#include "z_zone.h"
#include "m_random.h"
#include "w_wad.h"

#include "doomdef.h"
#include "p_local.h"

#include "doomstat.h"
#include "Main.h"

// Purpose?
const char snd_prefixen[]
= { 'P', 'P', 'A', 'S', 'S', 'S', 'M', 'M', 'M', 'S', 'S', 'S' };


// when to clip out sounds
// Does not fit the large outdoor areas.

// Distance tp origin when sounds should be maxed out.
// This should relate to movement clipping resolution
// (see BLOCKMAP handling).
// Originally: (200*0x10000).



// Adjustable by menu.



// percent attenuation from front to back



// Current music/sfx card - index useless
//  w/o a reference LUT in a sound module.
// Config file? Same disclaimer as above.





// the set of ::g->channels available

// These are not used, but should be (menu).
// Maximum volume of a sound effect.
// Internal default is max out of 0-15.

// Maximum volume of music. Useless so far.



// whether songs are ::g->mus_paused

// music currently being played

// following is set
//  by the defaults code in M_misc:
// number of ::g->channels available




//
// Internals.
//
int
S_getChannel
( void*		origin,
 sfxinfo_t*	sfxinfo );


int
S_AdjustSoundParams
( mobj_t*	listener,
 mobj_t*	source,
 int*		vol,
 int*		sep,
 int*		pitch );

void S_StopChannel(int cnum);



//
// Initializes sound stuff, including volume
// Sets ::g->channels, SFX and music volume,
//  allocates channel buffer, sets S_sfx lookup.
//
void S_Init
( int		sfxVolume,
 int		musicVolume )
{  
	int		i;

	// Whatever these did with DMX, these are rather dummies now.
	I_SetChannels();

	S_SetSfxVolume(sfxVolume);
	S_SetMusicVolume(musicVolume);

	// Allocating the internal ::g->channels for mixing
	// (the maximum numer of sounds rendered
	// simultaneously) within zone memory.
	::g->channels =
		(channel_t *) DoomLib::Z_Malloc(::g->numChannels*sizeof(channel_t), PU_STATIC, 0);

	// Free all ::g->channels for use
	for (i=0 ; i < ::g->numChannels ; i++)
		::g->channels[i].sfxinfo = 0;

	// no sounds are playing, and they are not ::g->mus_paused
	::g->mus_paused = 0;
	::g->mus_looping = 0;

	// Note that sounds have not been cached (yet).
	for (i=1 ; i<NUMSFX ; i++)
		S_sfx[i].lumpnum = S_sfx[i].usefulness = -1;
}




//
// Per level startup code.
// Kills playing sounds at start of level,
//  determines music if any, changes music.
//
void S_Start(void)
{
	int cnum;
	int mnum;

	// kill all playing sounds at start of level
	//  (trust me - a good idea)
	if( ::g->channels ) {
		for (cnum=0 ; cnum < ::g->numChannels ; cnum++)
			if (::g->channels[cnum].sfxinfo)
				S_StopChannel(cnum);
	}

	// start new music for the level
	::g->mus_paused = 0;

	if (::g->gamemode == commercial) {
		
		mnum = mus_runnin + ::g->gamemap - 1;
		
		/*
		Is this necessary?
		
		if ( ::g->gamemission == 0 ) {
			mnum = mus_runnin + ::g->gamemap - 1;
		}
		else {
			mnum = mus_runnin + ( gameLocal->GetMissionData( ::g->gamemission )->mapMusic[ ::g->gamemap-1 ] - 1 );
		}
		*/
	}
	else
	{
		int spmus[] = {
			// Song -	Who? -			Where?
			mus_e3m4,	// American		e4m1
			mus_e3m2,	// Romero		e4m2
			mus_e3m3,	// Shawn		e4m3
			mus_e1m5,	// American		e4m4
			mus_e2m7,	// Tim			e4m5
			mus_e2m4,	// Romero		e4m6
			mus_e2m6,	// J.Anderson	e4m7 CHIRON.WAD
			mus_e2m5,	// Shawn		e4m8
			mus_e1m9	// Tim			e4m9
		};

		if (::g->gameepisode < 4)
			mnum = mus_e1m1 + (::g->gameepisode-1)*9 + ::g->gamemap-1;
		else
			mnum = spmus[::g->gamemap-1];
	}	

	S_StopMusic();
	S_ChangeMusic(mnum, true);

	::g->nextcleanup = 15;
}	





void
S_StartSoundAtVolume
( void*		origin_p,
 int		sfx_id,
 int		volume )
{

	int		rc;
	int		sep;
	int		pitch;
	int		priority;
	sfxinfo_t*	sfx;
	int		cnum;

	mobj_t*	origin = (mobj_t *) origin_p;


	// Debug.
	/*I_PrintfE
	"S_StartSoundAtVolume: playing sound %d (%s)\n",
	sfx_id, S_sfx[sfx_id].name );*/

	// check for bogus sound #
	if (sfx_id < 1 || sfx_id > NUMSFX)
		I_Error("Bad sfx #: %d", sfx_id);

	sfx = &S_sfx[sfx_id];

	// Initialize sound parameters
	if (sfx->link)
	{
		pitch = sfx->pitch;
		priority = sfx->priority;
		volume += sfx->volume;

		if (volume < 1)
			return;

		if ( volume > s_volume_sound.GetInteger() )
			volume = s_volume_sound.GetInteger();
	}
	else
	{
		pitch = NORM_PITCH;
		priority = NORM_PRIORITY;

		if (volume < 1)
			return;

		if ( volume > s_volume_sound.GetInteger() )
			volume = s_volume_sound.GetInteger();

	}


	// Check to see if it is audible,
	//  and if not, modify the params
	// DHM - Nerve :: chainsaw!!!
	if (origin && ::g->players[::g->consoleplayer].mo && origin != ::g->players[::g->consoleplayer].mo)
	{
		rc = S_AdjustSoundParams(::g->players[::g->consoleplayer].mo,
			origin,
			&volume,
			&sep,
			&pitch);

		if ( origin->x == ::g->players[::g->consoleplayer].mo->x
			&& origin->y == ::g->players[::g->consoleplayer].mo->y)
		{	
			sep 	= NORM_SEP;
		}

		if (!rc)
			return;
	}	
	else
	{
		sep = NORM_SEP;
	}

	// hacks to vary the sfx pitches
	const bool isChainsawSound = sfx_id >= sfx_sawup && sfx_id <= sfx_sawhit;

	if ( !isChainsawSound && sfx_id != sfx_itemup && sfx_id != sfx_tink)
	{
		pitch += 16 - (M_Random()&31);

		if (pitch<0)
			pitch = 0;
		else if (pitch>255)
			pitch = 255;
	}

	// kill old sound
	//S_StopSound(origin);

	// try to find a channel
	cnum = S_getChannel(origin, sfx);

	if (cnum<0) {
		printf( "No sound channels available for sfxid %d.\n", sfx_id );
		return;
	}

	// get lumpnum if necessary
	if (sfx->lumpnum < 0)
		sfx->lumpnum = I_GetSfxLumpNum(sfx);

	// increase the usefulness
	if (sfx->usefulness++ < 0)
		sfx->usefulness = 1;

	// Assigns the handle to one of the ::g->channels in the
	//  mix/output buffer.
	::g->channels[cnum].handle = I_StartSound(sfx_id, origin, ::g->players[::g->consoleplayer].mo, volume, pitch, priority);
}	

void S_StartSound ( void*		origin, int		sfx_id )
{
	S_StartSoundAtVolume(origin, sfx_id, s_volume_sound.GetInteger() );
}




void S_StopSound(void *origin)
{

	int cnum;

	for (cnum=0 ; cnum < ::g->numChannels ; cnum++)
	{
		if (::g->channels[cnum].sfxinfo && ::g->channels[cnum].origin == origin)
		{
			S_StopChannel(cnum);
			break;
		}
	}
}





//
// Stop and resume music, during game PAUSE.
//
void S_PauseSound(void)
{
	if (::g->mus_playing && !::g->mus_paused)
	{
		I_PauseSong(::g->mus_playing->handle);
		::g->mus_paused = true;
	}
}

void S_ResumeSound(void)
{
	if (::g->mus_playing && ::g->mus_paused)
	{
		I_ResumeSong(::g->mus_playing->handle);
		::g->mus_paused = false;
	}
}


//
// Updates music & sounds
//
void S_UpdateSounds(void* listener_p)
{
	int		audible;
	int		cnum;
	int		volume;
	int		sep;
	int		pitch;
	sfxinfo_t*	sfx;
	channel_t*	c;

	mobj_t*	listener = (mobj_t*)listener_p;

	for (cnum=0 ; cnum < ::g->numChannels ; cnum++)
	{
		c = &::g->channels[cnum];
		sfx = c->sfxinfo;

		if(sfx)
		{
			if (I_SoundIsPlaying(c->handle))
			{
				// initialize parameters
				volume = s_volume_sound.GetInteger();
				pitch = NORM_PITCH;
				sep = NORM_SEP;

				if (sfx->link)
				{
					pitch = sfx->pitch;
					volume += sfx->volume;

					if (volume < 1)	{
						S_StopChannel(cnum);
						continue;

					} else if ( volume > s_volume_sound.GetInteger() ) {
						volume = s_volume_sound.GetInteger();
					}
				}

				// check non-local sounds for distance clipping or modify their params
				if (c->origin && listener_p != c->origin)
				{
					audible = S_AdjustSoundParams(listener,	(mobj_t*)c->origin,	&volume, &sep, &pitch);
					if (!audible) {
						S_StopChannel(cnum);
					}
				}
			}
			else
			{
				// if channel is allocated but sound has stopped, free it
				S_StopChannel(cnum);
			}
		}
	}
}


void S_SetMusicVolume(int volume)
{
	I_SetMusicVolume(volume);
	s_volume_midi.SetInteger( volume );
}



void S_SetSfxVolume(int volume)
{
	I_SetSfxVolume(volume);
	s_volume_sound.SetInteger( volume );
}

//
// Starts some music with the music id found in sounds.h.
//
void S_StartMusic(int m_id)
{
	S_ChangeMusic(m_id, false);
}

void S_ChangeMusic ( int			musicnum, int			looping )
{
#ifdef ID_ENABLE_DOOM_CLASSIC_NETWORKING
	if (gameLocal->IsSplitscreen() && DoomLib::GetPlayer() > 0 )
	{
		// we aren't the key player... we have no control over music
		return;
	}
#endif

	musicinfo_t*	music = NULL;

	if ( (musicnum <= mus_None)
		|| (musicnum >= NUMMUSIC) )
	{
		I_Error("Bad music number %d", musicnum);
	}
	else
		music = &::g->S_music[musicnum];

	if (::g->mus_playing == music)
		return;

	//I_Printf("S_ChangeMusic: Playing new track: '%s'\n", music->name);

	I_PlaySong( music->name, looping );

	::g->mus_playing = music;
}


void S_StopMusic(void)
{
	if (::g->doomcom.consoleplayer)
	{
		// we aren't the key player... we have no control over music
		return;
	}
	
	if (::g->mus_playing)
	{
		if (::g->mus_paused)
			I_ResumeSong(::g->mus_playing->handle);

		I_StopSong(::g->mus_playing->handle);
		I_UnRegisterSong(::g->mus_playing->handle);
		//Z_FreeTags( PU_MUSIC_SHARED, PU_MUSIC_SHARED );

		::g->mus_playing->data = 0;
		::g->mus_playing = 0;
	}
}




void S_StopChannel(int cnum)
{

	int		i;
	channel_t*	c = &::g->channels[cnum];

	if (c->sfxinfo)
	{
		// stop the sound playing
		if (I_SoundIsPlaying(c->handle))
		{
#ifdef SAWDEBUG
			if (c->sfxinfo == &S_sfx[sfx_sawful])
				I_PrintfE( "stopped\n");
#endif
			I_StopSound(c->handle);
		}

		// check to see
		//  if other ::g->channels are playing the sound
		for (i=0 ; i < ::g->numChannels ; i++)
		{
			if (cnum != i
				&& c->sfxinfo == ::g->channels[i].sfxinfo)
			{
				break;
			}
		}

		// degrade usefulness of sound data
		c->sfxinfo->usefulness--;

		c->sfxinfo = 0;
	}
}



//
// Changes volume, stereo-separation, and pitch variables
//  from the norm of a sound effect to be played.
// If the sound is not audible, returns a 0.
// Otherwise, modifies parameters and returns 1.
//
int S_AdjustSoundParams( mobj_t* listener, mobj_t* source, int* vol, int* sep, int* pitch ) {
	fixed_t	approx_dist;
	fixed_t	adx;
	fixed_t	ady;

	// DHM - Nerve :: Could happen in multiplayer if a player exited the level holding the chainsaw
	if ( listener == NULL || source == NULL ) {
		return 0;
	}

	// calculate the distance to sound origin
	//  and clip it if necessary
	adx = abs(listener->x - source->x);
	ady = abs(listener->y - source->y);

	// From _GG1_ p.428. Appox. eucledian distance fast.
	approx_dist = adx + ady - ((adx < ady ? adx : ady)>>1);

	if ( approx_dist > S_CLIPPING_DIST)	{
		return 0;
	}

	// angle of source to listener
	*sep = NORM_SEP;

	// volume calculation
	if (approx_dist < S_CLOSE_DIST)	{
		*vol = s_volume_sound.GetInteger();
	}
	else {
		// distance effect
		*vol = ( s_volume_sound.GetInteger()
			* ((S_CLIPPING_DIST - approx_dist)>>FRACBITS))
			/ S_ATTENUATOR; 
	}

	return (*vol > 0);
}




//
// S_getChannel :
//   If none available, return -1.  Otherwise channel #.
//
int
S_getChannel
( void*		origin,
 sfxinfo_t*	sfxinfo )
{
	// channel number to use
	int		cnum;

	channel_t*	c;

	// Find an open channel
	for (cnum=0 ; cnum < ::g->numChannels ; cnum++)
	{
		if (!::g->channels[cnum].sfxinfo)
			break;
		else if ( origin && ::g->channels[cnum].origin == origin && 
				(::g->channels[cnum].handle == sfx_sawidl || ::g->channels[cnum].handle == sfx_sawful) )
		{
			S_StopChannel(cnum);
			break;
		}
	}

	// None available
	if (cnum == ::g->numChannels)
	{
		// Look for lower priority
		for (cnum=0 ; cnum < ::g->numChannels ; cnum++)
			if (::g->channels[cnum].sfxinfo->priority >= sfxinfo->priority) break;

		if (cnum == ::g->numChannels)
		{
			// FUCK!  No lower priority.  Sorry, Charlie.    
			return -1;
		}
		else
		{
			// Otherwise, kick out lower priority.
			S_StopChannel(cnum);
		}
	}

	c = &::g->channels[cnum];

	// channel is decided to be cnum.
	c->sfxinfo = sfxinfo;
	c->origin = origin;

	return cnum;
}





