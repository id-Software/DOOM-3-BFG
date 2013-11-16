/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2012 Daniel Gibson

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

// DG: this is a stub implementing the classic doom sound interface so we don't need XAudio to compile
// (that doesn't work with MinGW and is only available on Windows)

#include "Precompiled.h"
#include "i_sound.h"
#include "w_wad.h"

// Init at program start...
void I_InitSound(){}
void I_InitSoundHardware( int numOutputChannels_, int channelMask ){}

// ... update sound buffer and audio device at runtime...
void I_UpdateSound(void){}
void I_SubmitSound(void){}

// ... shut down and relase at program termination.
void I_ShutdownSound(void){}
void I_ShutdownSoundHardware(){}

//
//  SFX I/O
//

// Initialize channels?
void I_SetChannels(){}

// Get raw data lump index for sound descriptor.
int I_GetSfxLumpNum (sfxinfo_t* sfxinfo )
{
	char namebuf[9];
	sprintf(namebuf, "ds%s", sfxinfo->name);
	return W_GetNumForName(namebuf);
}

void I_ProcessSoundEvents( void ){}

// Starts a sound in a particular sound channel.
int I_StartSound( int id, mobj_t *origin, mobj_t *listener_origin, int vol, int pitch, int priority )
{ return 0; }


// Stops a sound channel.
void I_StopSound(int handle, int player){}

// Called by S_*() functions
//  to see if a channel is still playing.
// Returns 0 if no longer playing, 1 if playing.
int I_SoundIsPlaying(int handle)
{ return 0; }

// Updates the volume, separation,
//  and pitch of a sound channel.
void I_UpdateSoundParams( int handle, int vol, int sep, int pitch ){}

void I_SetSfxVolume( int ){}
//
//  MUSIC I/O
//
void I_InitMusic(void){}

void I_ShutdownMusic(void){}

// Volume.
void I_SetMusicVolume(int volume){}

// PAUSE game handling.
void I_PauseSong(int handle){}

void I_ResumeSong(int handle){}

// Registers a song handle to song data.
int I_RegisterSong(void *data, int length)
{ return 0; }

// Called by anything that wishes to start music.
//  plays a song, and when the song is done,
//  starts playing it again in an endless loop.
// Horrible thing to do, considering.
void I_PlaySong( const char *songname, int looping ){}

// Stops a song over 3 seconds.
void I_StopSong(int handle){}

// See above (register), then think backwards
void I_UnRegisterSong(int handle){}

// Update Music (XMP), check for notifications
void I_UpdateMusic(void){}
