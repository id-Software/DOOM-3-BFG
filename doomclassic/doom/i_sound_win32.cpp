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

//
// DESCRIPTION:
//	System interface for sound.
//
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <sys/types.h>
#include <fcntl.h>
// Timer stuff. Experimental.
#include <time.h>
#include <signal.h>
#include "z_zone.h"
#include "i_system.h"
#include "i_sound.h"
#include "m_argv.h"
#include "m_misc.h"
#include "w_wad.h"
#include "d_main.h"
#include "doomdef.h"
#include "../timidity/timidity.h"
#include "../timidity/controls.h"

#include "sound/snd_local.h"

#ifdef _MSC_VER // DG: xaudio can only be used with MSVC
#include <xaudio2.h>
#include <x3daudio.h>
#endif // DG end

#pragma warning ( disable : 4244 )

#define	MIDI_CHANNELS		2
#if 1
#define MIDI_RATE			22050
#define MIDI_SAMPLETYPE		XAUDIOSAMPLETYPE_8BITPCM
#define MIDI_FORMAT			AUDIO_U8
#define MIDI_FORMAT_BYTES	1
#else
#define MIDI_RATE			48000
#define MIDI_SAMPLETYPE		XAUDIOSAMPLETYPE_16BITPCM
#define MIDI_FORMAT			AUDIO_S16MSB
#define MIDI_FORMAT_BYTES	2
#endif

#ifdef _MSC_VER // DG: xaudio can only be used with MSVC
IXAudio2SourceVoice*	pMusicSourceVoice;
#endif

MidiSong*				doomMusic;
byte*					musicBuffer;
int						totalBufferSize;

HANDLE	hMusicThread;
bool	waitingForMusic;
bool	musicReady;


typedef struct tagActiveSound_t {
	IXAudio2SourceVoice*     m_pSourceVoice;         // Source voice
	X3DAUDIO_DSP_SETTINGS   m_DSPSettings;
	X3DAUDIO_EMITTER        m_Emitter;
	X3DAUDIO_CONE           m_Cone;
	int id;
	int valid;
	int start;
	int player;
	bool localSound;
	mobj_t *originator;
} activeSound_t;


// cheap little struct to hold a sound
typedef struct {
	int vol;
	int player;
	int pitch;
	int priority;
	mobj_t *originator;
	mobj_t *listener;
} soundEvent_t;

// array of all the possible sounds
// in split screen we only process the loudest sound of each type per frame
soundEvent_t soundEvents[128];
extern int PLAYERCOUNT;

// Real volumes
const float		GLOBAL_VOLUME_MULTIPLIER = 0.5f;

float			x_SoundVolume = GLOBAL_VOLUME_MULTIPLIER;
float			x_MusicVolume = GLOBAL_VOLUME_MULTIPLIER;

// The actual lengths of all sound effects.
static int 		lengths[NUMSFX];
activeSound_t	activeSounds[NUM_SOUNDBUFFERS] = {0};

int				S_initialized = 0;
bool			Music_initialized = false;

// XAUDIO
float			g_EmitterAzimuths [] = { 0.f };
static int		numOutputChannels = 0;
static bool		soundHardwareInitialized = false;

// DG: xaudio can only be used with MSVC
#ifdef _MSC_VER
X3DAUDIO_HANDLE					X3DAudioInstance;

X3DAUDIO_LISTENER				doom_Listener;
#endif

//float							localSoundVolumeEntries[] = { 0.f, 0.f, 0.9f, 0.5f, 0.f, 0.f };
float							localSoundVolumeEntries[] = { 0.8f, 0.8f, 0.8f, 0.8f, 0.8f, 0.8f, 0.8f, 0.8f, 0.8f, 0.8f, 0.8f };


void							I_InitSoundChannel( int channel, int numOutputChannels_ );

/*
======================
getsfx
======================
*/
// This function loads the sound data from the WAD lump,
//  for single sound.
//
void* getsfx ( const char* sfxname, int* len )
{
	unsigned char*      sfx;
	unsigned char*	    sfxmem;
	int                 size;
	char                name[20];
	int                 sfxlump;
	float				scale = 1.0f;

	// Get the sound data from the WAD, allocate lump
	//  in zone memory.
	sprintf(name, "ds%s", sfxname);

	// Scale down the plasma gun, it clips
	if ( strcmp( sfxname, "plasma" ) == 0 ) {
		scale = 0.75f;
	}
	if ( strcmp( sfxname, "itemup" ) == 0 ) {
		scale = 1.333f;
	}

	// If sound requested is not found in current WAD, use pistol as default
	if ( W_CheckNumForName(name) == -1 )
		sfxlump = W_GetNumForName("dspistol");
	else
		sfxlump = W_GetNumForName(name);

	// Sound lump headers are 8 bytes.
	const int SOUND_LUMP_HEADER_SIZE_IN_BYTES = 8;

	size = W_LumpLength( sfxlump ) - SOUND_LUMP_HEADER_SIZE_IN_BYTES;

	sfx = (unsigned char*)W_CacheLumpNum( sfxlump, PU_CACHE_SHARED );
	const unsigned char * sfxSampleStart = sfx + SOUND_LUMP_HEADER_SIZE_IN_BYTES;

	// Allocate from zone memory.
	//sfxmem = (float*)DoomLib::Z_Malloc( size*(sizeof(float)), PU_SOUND_SHARED, 0 );
	sfxmem = (unsigned char*)malloc( size * sizeof(unsigned char) );

	// Now copy, and convert to Xbox360 native float samples, do initial volume ramp, and scale
	for ( int i=0; i<size; i++ ) {
		sfxmem[i] = sfxSampleStart[i];// * scale;
	}

	// Remove the cached lump.
	Z_Free( sfx );

	// Set length.
	*len = size;

	// Return allocated padded data.
	return (void *) (sfxmem);
}

/*
======================
I_SetChannels
======================
*/
void I_SetChannels() {
	// Original Doom set up lookup tables here
}	

/*
======================
I_SetSfxVolume
======================
*/
void I_SetSfxVolume(int volume) {
	x_SoundVolume = ((float)volume / 15.f) * GLOBAL_VOLUME_MULTIPLIER;
}

/*
======================
I_GetSfxLumpNum
======================
*/
//
// Retrieve the raw data lump index
//  for a given SFX name.
//
int I_GetSfxLumpNum(sfxinfo_t* sfx)
{
	char namebuf[9];
	sprintf(namebuf, "ds%s", sfx->name);
	return W_GetNumForName(namebuf);
}

/*
======================
I_StartSound2
======================
*/
// Starting a sound means adding it
//  to the current list of active sounds
//  in the internal channels.
// As the SFX info struct contains
//  e.g. a pointer to the raw data,
//  it is ignored.
// As our sound handling does not handle
//  priority, it is ignored.
// Pitching (that is, increased speed of playback) is set
//
int I_StartSound2 ( int id, int player, mobj_t *origin, mobj_t *listener_origin, int pitch, int priority ) {
	if ( !soundHardwareInitialized ) {
		return id;
	}
	
	int i;
	 XAUDIO2_VOICE_STATE state;
	activeSound_t* sound = 0;
	int oldest = 0, oldestnum = -1;

	// these id's should not overlap
	if ( id == sfx_sawup || id == sfx_sawidl || id == sfx_sawful || id == sfx_sawhit || id == sfx_stnmov ) {
		// Loop all channels, check.
		for (i=0 ; i < NUM_SOUNDBUFFERS ; i++)
		{
			sound = &activeSounds[i];

			if (sound->valid && ( sound->id == id && sound->player == player ) ) {
				I_StopSound( sound->id, player );
				break;
			}
		}
	}

	// find a valid channel, or one that has finished playing
	for (i = 0; i < NUM_SOUNDBUFFERS; ++i) {
		sound = &activeSounds[i];
		
		if (!sound->valid)
			break;

		if (!oldest || oldest > sound->start) {
			oldestnum = i;
			oldest = sound->start;
		}

		sound->m_pSourceVoice->GetState( &state );
		if ( state.BuffersQueued == 0 ) {
			break;
		}
	}

	// none found, so use the oldest one
	if (i == NUM_SOUNDBUFFERS)
	{
		i = oldestnum;
		sound = &activeSounds[i];
	}

	// stop the sound with a FlushPackets
	sound->m_pSourceVoice->Stop();
	sound->m_pSourceVoice->FlushSourceBuffers();

	// Set up packet
	XAUDIO2_BUFFER Packet = { 0 };
	Packet.Flags = XAUDIO2_END_OF_STREAM;
	Packet.AudioBytes = lengths[id];
	Packet.pAudioData = (BYTE*)S_sfx[id].data;
	Packet.PlayBegin = 0;
	Packet.PlayLength = 0;
	Packet.LoopBegin = XAUDIO2_NO_LOOP_REGION;
	Packet.LoopLength = 0;
	Packet.LoopCount = 0;
	Packet.pContext = NULL;


	// Set voice volumes
	sound->m_pSourceVoice->SetVolume( x_SoundVolume );

	// Set voice pitch
	sound->m_pSourceVoice->SetFrequencyRatio( 1 + ((float)pitch-128.f)/95.f );

	// Set initial spatialization
	if ( origin && origin != listener_origin ) {
		// Update Emitter Position
		sound->m_Emitter.Position.x = (float)(origin->x >> FRACBITS);
		sound->m_Emitter.Position.y = 0.f;
		sound->m_Emitter.Position.z = (float)(origin->y >> FRACBITS);

		// Calculate 3D positioned speaker volumes
		DWORD dwCalculateFlags = X3DAUDIO_CALCULATE_MATRIX;
		X3DAudioCalculate( X3DAudioInstance, &doom_Listener, &sound->m_Emitter, dwCalculateFlags, &sound->m_DSPSettings );

		// Pan the voice according to X3DAudio calculation
		sound->m_pSourceVoice->SetOutputMatrix( NULL, 1, numOutputChannels, sound->m_DSPSettings.pMatrixCoefficients );

		sound->localSound = false;
	} else {
		// Local(or Global) sound, fixed speaker volumes
		sound->m_pSourceVoice->SetOutputMatrix( NULL, 1, numOutputChannels, localSoundVolumeEntries );

		sound->localSound = true;
	}

	// Submit packet
	HRESULT hr;
	if( FAILED( hr = sound->m_pSourceVoice->SubmitSourceBuffer( &Packet ) ) ) {
		int fail = 1;
	}

	// Play the source voice
	if( FAILED( hr = sound->m_pSourceVoice->Start( 0 ) ) ) {
		int fail = 1;
	}

	// set id, and start time
	sound->id = id;
	sound->start = ::g->gametic;
	sound->valid = 1;
	sound->player = player;
	sound->originator = origin;

	return id;
}

/*
======================
I_ProcessSoundEvents
======================
*/
void I_ProcessSoundEvents() {
	for( int i = 0; i < 128; i++ ) {
		if( soundEvents[i].pitch ) {
			I_StartSound2( i, soundEvents[i].player, soundEvents[i].originator, soundEvents[i].listener, soundEvents[i].pitch, soundEvents[i].priority );
		}
	}
	memset( soundEvents, 0, sizeof( soundEvents ) );
}

/*
======================
I_StartSound
======================
*/
int I_StartSound ( int id, mobj_t *origin, mobj_t *listener_origin, int vol, int pitch, int priority ) {
	// only allow player 0s sounds in intermission and finale screens
	if( ::g->gamestate != GS_LEVEL && DoomLib::GetPlayer() != 0 ) {
		return 0;
	}

	// if we're only one player or we're trying to play the chainsaw sound, do it normal
	// otherwise only allow one sound of each type per frame
	if( PLAYERCOUNT == 1 || id == sfx_sawup || id == sfx_sawidl || id == sfx_sawful || id == sfx_sawhit ) {
		return I_StartSound2( id, ::g->consoleplayer, origin, listener_origin, pitch, priority );
	}
	else {
		if( soundEvents[ id ].vol < vol ) {
			soundEvents[ id ].player = DoomLib::GetPlayer();
			soundEvents[ id ].pitch = pitch;
			soundEvents[ id ].priority = priority;
			soundEvents[ id ].vol = vol;
			soundEvents[ id ].originator = origin;
			soundEvents[ id ].listener = listener_origin;
		}
		return id;
	}
}

/*
======================
I_StopSound
======================
*/
void I_StopSound (int handle, int player)
{
	// You need the handle returned by StartSound.
	// Would be looping all channels,
	//  tracking down the handle,
	//  an setting the channel to zero.

	int i;
	activeSound_t* sound = 0;

	for (i = 0; i < NUM_SOUNDBUFFERS; ++i)
	{
		sound = &activeSounds[i];
		if (!sound->valid || sound->id != handle || (player >= 0 && sound->player != player) )
			continue;
		break;
	}

	if (i == NUM_SOUNDBUFFERS)
		return;

	// stop the sound
	if ( sound->m_pSourceVoice != NULL ) {
		sound->m_pSourceVoice->Stop( 0 );
	}

	sound->valid = 0;
	sound->player = -1;
}

/*
======================
I_SoundIsPlaying
======================
*/
int I_SoundIsPlaying(int handle) {
	if ( !soundHardwareInitialized ) {
		return 0;
	}

	int i;
	XAUDIO2_VOICE_STATE	state;
	activeSound_t* sound;

	for (i = 0; i < NUM_SOUNDBUFFERS; ++i)
	{
		sound = &activeSounds[i];
		if (!sound->valid || sound->id != handle)
			continue;

		sound->m_pSourceVoice->GetState( &state );
		if ( state.BuffersQueued > 0 ) {
			return 1;
		}
	}

	return 0;
}

/*
======================
I_UpdateSound
======================
*/
// Update Listener Position and go through all the
// channels and update speaker volumes for 3D sound.
void I_UpdateSound() {
	if ( !soundHardwareInitialized ) {
		return;
	}

	int i;
	XAUDIO2_VOICE_STATE	state;
	activeSound_t* sound;

	for ( i=0; i < NUM_SOUNDBUFFERS; i++ ) {
		sound = &activeSounds[i];

		if ( !sound->valid || sound->localSound ) {
			continue;
		}

		sound->m_pSourceVoice->GetState( &state );

		if ( state.BuffersQueued > 0 ) {
			mobj_t *playerObj = ::g->players[ sound->player ].mo;

			// Update Listener Orientation and Position
			angle_t	pAngle = playerObj->angle;
			fixed_t fx, fz;

			pAngle >>= ANGLETOFINESHIFT;

			fx = finecosine[pAngle];
			fz = finesine[pAngle];

			doom_Listener.OrientFront.x = (float)(fx) / 65535.f;
			doom_Listener.OrientFront.y = 0.f;
			doom_Listener.OrientFront.z = (float)(fz) / 65535.f;

			doom_Listener.Position.x = (float)(playerObj->x >> FRACBITS);
			doom_Listener.Position.y = 0.f;
			doom_Listener.Position.z = (float)(playerObj->y >> FRACBITS);

			// Update Emitter Position
			sound->m_Emitter.Position.x = (float)(sound->originator->x >> FRACBITS);
			sound->m_Emitter.Position.y = 0.f;
			sound->m_Emitter.Position.z = (float)(sound->originator->y >> FRACBITS);

			// Calculate 3D positioned speaker volumes
			DWORD dwCalculateFlags = X3DAUDIO_CALCULATE_MATRIX;
			X3DAudioCalculate( X3DAudioInstance, &doom_Listener, &sound->m_Emitter, dwCalculateFlags, &sound->m_DSPSettings );

			// Pan the voice according to X3DAudio calculation
			sound->m_pSourceVoice->SetOutputMatrix( NULL, 1, numOutputChannels, sound->m_DSPSettings.pMatrixCoefficients );
		}
	}
}

/*
======================
I_UpdateSoundParams
======================
*/
void I_UpdateSoundParams( int handle, int vol, int sep, int pitch) {
}

/*
======================
I_ShutdownSound
======================
*/
void I_ShutdownSound(void) {
	int done = 0;
	int i;

	if ( S_initialized ) {
		// Stop all sounds, but don't destroy the XAudio2 buffers.
		for ( i = 0; i < NUM_SOUNDBUFFERS; ++i ) {
			activeSound_t * sound = &activeSounds[i];

			if ( sound == NULL ) {
				continue;
			}

			I_StopSound( sound->id, 0 );

			if ( sound->m_pSourceVoice ) {
				sound->m_pSourceVoice->FlushSourceBuffers();
			}
		}

		for (i=1 ; i<NUMSFX ; i++) {
			if ( S_sfx[i].data && !(S_sfx[i].link) ) {
				//Z_Free( S_sfx[i].data );
				free( S_sfx[i].data );
			}
		}
	}

	I_StopSong( 0 );

	S_initialized = 0;
	// Done.
	return;
}

/*
======================
I_InitSoundHardware

Called from the tech4x initialization code. Sets up Doom classic's
sound channels.
======================
*/
void I_InitSoundHardware( int numOutputChannels_, int channelMask ) {
	::numOutputChannels = numOutputChannels_;

	// Initialize the X3DAudio
	//  Speaker geometry configuration on the final mix, specifies assignment of channels
	//  to speaker positions, defined as per WAVEFORMATEXTENSIBLE.dwChannelMask
	//  SpeedOfSound - not used by doomclassic
	X3DAudioInitialize( channelMask, 340.29f, X3DAudioInstance );

	for ( int i = 0; i < NUM_SOUNDBUFFERS; ++i ) {
		// Initialize source voices
		I_InitSoundChannel( i, numOutputChannels );
	}

	I_InitMusic();

	soundHardwareInitialized = true;
}


/*
======================
I_ShutdownitSoundHardware

Called from the tech4x shutdown code. Tears down Doom classic's
sound channels.
======================
*/
void I_ShutdownSoundHardware() {
	soundHardwareInitialized = false;

	I_ShutdownMusic();

	for ( int i = 0; i < NUM_SOUNDBUFFERS; ++i ) {
		activeSound_t * sound = &activeSounds[i];

		if ( sound == NULL ) {
			continue;
		}

		if ( sound->m_pSourceVoice ) {
			sound->m_pSourceVoice->Stop();
			sound->m_pSourceVoice->FlushSourceBuffers();
			sound->m_pSourceVoice->DestroyVoice();
			sound->m_pSourceVoice = NULL;
		}

		if ( sound->m_DSPSettings.pMatrixCoefficients ) {
			delete [] sound->m_DSPSettings.pMatrixCoefficients;
			sound->m_DSPSettings.pMatrixCoefficients = NULL;
		}
	}
}

/*
======================
I_InitSoundChannel
======================
*/
void I_InitSoundChannel( int channel, int numOutputChannels_ ) {
	activeSound_t	*soundchannel = &activeSounds[ channel ];

	// RB: fixed non-aggregates cannot be initialized with initializer list
#if defined(USE_WINRT) //(_WIN32_WINNT >= 0x0602 /*_WIN32_WINNT_WIN8*/)
	X3DAUDIO_VECTOR ZeroVector( 0.0f, 0.0f, 0.0f );
#else
	X3DAUDIO_VECTOR ZeroVector = { 0.0f, 0.0f, 0.0f };
#endif
	// RB end

	// Set up emitter parameters
	soundchannel->m_Emitter.OrientFront.x         = 0.0f;
	soundchannel->m_Emitter.OrientFront.y         = 0.0f;
	soundchannel->m_Emitter.OrientFront.z         = 1.0f;
	soundchannel->m_Emitter.OrientTop.x           = 0.0f;
	soundchannel->m_Emitter.OrientTop.y           = 1.0f;
	soundchannel->m_Emitter.OrientTop.z           = 0.0f;
	soundchannel->m_Emitter.Position              = ZeroVector;
	soundchannel->m_Emitter.Velocity              = ZeroVector;
	soundchannel->m_Emitter.pCone                 = &(soundchannel->m_Cone);
	soundchannel->m_Emitter.pCone->InnerAngle     = 0.0f; // Setting the inner cone angles to X3DAUDIO_2PI and
	// outer cone other than 0 causes
	// the emitter to act like a point emitter using the
	// INNER cone settings only.
	soundchannel->m_Emitter.pCone->OuterAngle     = 0.0f; // Setting the outer cone angles to zero causes
	// the emitter to act like a point emitter using the
	// OUTER cone settings only.
	soundchannel->m_Emitter.pCone->InnerVolume    = 0.0f;
	soundchannel->m_Emitter.pCone->OuterVolume    = 1.0f;
	soundchannel->m_Emitter.pCone->InnerLPF       = 0.0f;
	soundchannel->m_Emitter.pCone->OuterLPF       = 1.0f;
	soundchannel->m_Emitter.pCone->InnerReverb    = 0.0f;
	soundchannel->m_Emitter.pCone->OuterReverb    = 1.0f;

	soundchannel->m_Emitter.ChannelCount          = 1;
	soundchannel->m_Emitter.ChannelRadius         = 0.0f;
	soundchannel->m_Emitter.pVolumeCurve          = NULL;
	soundchannel->m_Emitter.pLFECurve             = NULL;
	soundchannel->m_Emitter.pLPFDirectCurve       = NULL;
	soundchannel->m_Emitter.pLPFReverbCurve       = NULL;
	soundchannel->m_Emitter.pReverbCurve          = NULL;
	soundchannel->m_Emitter.CurveDistanceScaler   = 1200.0f;
	soundchannel->m_Emitter.DopplerScaler         = 1.0f;
	soundchannel->m_Emitter.pChannelAzimuths      = g_EmitterAzimuths;

	soundchannel->m_DSPSettings.SrcChannelCount     = 1;
	soundchannel->m_DSPSettings.DstChannelCount     = numOutputChannels_;
	soundchannel->m_DSPSettings.pMatrixCoefficients = new FLOAT[ numOutputChannels_ ];

	// Create Source voice
	WAVEFORMATEX voiceFormat = {0};
	voiceFormat.wFormatTag = WAVE_FORMAT_PCM;
	voiceFormat.nChannels = 1;
    voiceFormat.nSamplesPerSec = 11025;
    voiceFormat.nAvgBytesPerSec = 11025;
    voiceFormat.nBlockAlign = 1;
    voiceFormat.wBitsPerSample = 8;
    voiceFormat.cbSize = 0;

	soundSystemLocal.hardware.GetIXAudio2()->CreateSourceVoice( &soundchannel->m_pSourceVoice, (WAVEFORMATEX *)&voiceFormat );
}

/*
======================
I_InitSound
======================
*/
void I_InitSound() {

	if (S_initialized == 0) {
		int i;

		// RB: non-aggregates cannot be initialized with initializer list
#if defined(USE_WINRT) // (_WIN32_WINNT >= 0x0602 /*_WIN32_WINNT_WIN8*/)
		X3DAUDIO_VECTOR ZeroVector( 0.0f, 0.0f, 0.0f );
#else
		X3DAUDIO_VECTOR ZeroVector = { 0.0f, 0.0f, 0.0f };
#endif
		// RB end

		// Set up listener parameters
		doom_Listener.OrientFront.x        = 0.0f;
		doom_Listener.OrientFront.y        = 0.0f;
		doom_Listener.OrientFront.z        = 1.0f;
		doom_Listener.OrientTop.x          = 0.0f;
		doom_Listener.OrientTop.y          = 1.0f;
		doom_Listener.OrientTop.z          = 0.0f;
		doom_Listener.Position             = ZeroVector;
		doom_Listener.Velocity             = ZeroVector;

		for (i=1 ; i<NUMSFX ; i++)
		{ 
			// Alias? Example is the chaingun sound linked to pistol.
			if (!S_sfx[i].link)
			{
				// Load data from WAD file.
				S_sfx[i].data = getsfx( S_sfx[i].name, &lengths[i] );
			}	
			else
			{
				// Previously loaded already?
				S_sfx[i].data = S_sfx[i].link->data;
				lengths[i] = lengths[(S_sfx[i].link - S_sfx)/sizeof(sfxinfo_t)];
			}
		}

		S_initialized = 1;
	}
}

/*
======================
I_SubmitSound
======================
*/
void I_SubmitSound(void)
{
	// Only do this for player 0, it will still handle positioning
	//		for other players, but it can't be outside the game 
	//		frame like the soundEvents are.
	if ( DoomLib::GetPlayer() == 0 ) {
		// Do 3D positioning of sounds
		I_UpdateSound();

		// Check for XMP notifications
		I_UpdateMusic();
	}
}


// =========================================================
// =========================================================
// Background Music
// =========================================================
// =========================================================

/*
======================
I_SetMusicVolume
======================
*/
void I_SetMusicVolume(int volume)
{
	x_MusicVolume = (float)volume / 15.f;
}

/*
======================
I_InitMusic
======================
*/
void I_InitMusic(void)		
{
	if ( !Music_initialized ) {
		// Initialize Timidity
		Timidity_Init( MIDI_RATE, MIDI_FORMAT, MIDI_CHANNELS, MIDI_RATE, "classicmusic/gravis.cfg" );

		hMusicThread = NULL;
		musicBuffer = NULL;
		totalBufferSize = 0;
		waitingForMusic = false;
		musicReady = false;

		// Create Source voice
		WAVEFORMATEX voiceFormat = {0};
		voiceFormat.wFormatTag = WAVE_FORMAT_PCM;
		voiceFormat.nChannels = 2;
		voiceFormat.nSamplesPerSec = MIDI_RATE;
		voiceFormat.nAvgBytesPerSec = MIDI_RATE * MIDI_FORMAT_BYTES * 2;
		voiceFormat.nBlockAlign = MIDI_FORMAT_BYTES * 2;
		voiceFormat.wBitsPerSample = MIDI_FORMAT_BYTES * 8;
		voiceFormat.cbSize = 0;

// RB: XAUDIO2_VOICE_MUSIC not available on Windows 8 SDK
#if !defined(USE_WINRT) //(_WIN32_WINNT < 0x0602 /*_WIN32_WINNT_WIN8*/)
		soundSystemLocal.hardware.GetIXAudio2()->CreateSourceVoice( &pMusicSourceVoice, (WAVEFORMATEX *)&voiceFormat, XAUDIO2_VOICE_MUSIC );
#endif
// RB end

		Music_initialized = true;
	}
}

/*
======================
I_ShutdownMusic
======================
*/
void I_ShutdownMusic(void)	
{
	I_StopSong( 0 );

	if ( Music_initialized ) {
		if ( pMusicSourceVoice ) {
			pMusicSourceVoice->Stop();
			pMusicSourceVoice->FlushSourceBuffers();
			pMusicSourceVoice->DestroyVoice();
			pMusicSourceVoice = NULL;
		}

		if ( hMusicThread ) {
			DWORD	rc;

			do {
				GetExitCodeThread( hMusicThread, &rc );
				if ( rc == STILL_ACTIVE ) {
					Sleep( 1 );
				}
			} while( rc == STILL_ACTIVE );

			CloseHandle( hMusicThread );
		}
		if ( musicBuffer ) {
			free( musicBuffer );
		}

		Timidity_Shutdown();
	}

	pMusicSourceVoice = NULL;
	hMusicThread = NULL;
	musicBuffer = NULL;

	totalBufferSize = 0;
	waitingForMusic = false;
	musicReady = false;

	Music_initialized = false;
}

int Mus2Midi(unsigned char* bytes, unsigned char* out, int* len);

namespace {
	const int MaxMidiConversionSize = 1024 * 1024;
	unsigned char midiConversionBuffer[MaxMidiConversionSize];
}

/*
======================
I_LoadSong
======================
*/
DWORD WINAPI I_LoadSong( LPVOID songname ) {
	idStr lumpName = "d_";
	lumpName += static_cast< const char * >( songname );

	unsigned char * musFile = static_cast< unsigned char * >( W_CacheLumpName( lumpName.c_str(), PU_STATIC_SHARED ) );

	int length = 0;
	Mus2Midi( musFile, midiConversionBuffer, &length );

	doomMusic = Timidity_LoadSongMem( midiConversionBuffer, length );

	if ( doomMusic ) {
		musicBuffer = (byte *)malloc( MIDI_CHANNELS * MIDI_FORMAT_BYTES * doomMusic->samples );
		totalBufferSize = doomMusic->samples * MIDI_CHANNELS * MIDI_FORMAT_BYTES;

		Timidity_Start( doomMusic );

		int		rc = RC_NO_RETURN_VALUE;
		int		num_bytes = 0;
		int		offset = 0;

		do {
			rc = Timidity_PlaySome( musicBuffer + offset, MIDI_RATE, &num_bytes );
			offset += num_bytes;
		} while ( rc != RC_TUNE_END );

		Timidity_Stop();
		Timidity_FreeSong( doomMusic );
	}

	musicReady = true;

	return ERROR_SUCCESS;
}

/*
======================
I_PlaySong
======================
*/
void I_PlaySong( const char *songname, int looping)
{
	if ( !Music_initialized ) {
		return;
	}

	if ( pMusicSourceVoice != NULL ) {
		// Stop the voice and flush packets before freeing the musicBuffer
		pMusicSourceVoice->Stop();
		pMusicSourceVoice->FlushSourceBuffers();
	}

	// Make sure voice is stopped before we free the buffer
	bool isStopped = false;
	int d = 0;
	while ( !isStopped ) {
		XAUDIO2_VOICE_STATE test;

		if ( pMusicSourceVoice != NULL ) {
			pMusicSourceVoice->GetState( &test );
		}

		if ( test.pCurrentBufferContext == NULL && test.BuffersQueued == 0 ) {
			isStopped = true;
		}
		//I_Printf( "waiting to stop (%d)\n", d++ );
	}

	// Clear old state
	if ( musicBuffer != NULL ) {
		free( musicBuffer );
		musicBuffer = NULL;
	}

	musicReady = false;
	I_LoadSong( (LPVOID)songname );
	waitingForMusic = true;

	if ( DoomLib::GetPlayer() >= 0 ) {
		::g->mus_looping = looping;
	}
}

/*
======================
I_UpdateMusic
======================
*/
void I_UpdateMusic() {
	if ( !Music_initialized ) {
		return;
	}

	if ( waitingForMusic ) {

		if ( musicReady && pMusicSourceVoice != NULL ) {

			if ( musicBuffer ) {
				// Set up packet
				XAUDIO2_BUFFER Packet = { 0 };
				Packet.Flags = XAUDIO2_END_OF_STREAM;
				Packet.AudioBytes = totalBufferSize;
				Packet.pAudioData = (BYTE*)musicBuffer;
				Packet.PlayBegin = 0;
				Packet.PlayLength = 0;
				Packet.LoopBegin = 0;
				Packet.LoopLength = 0;
				Packet.LoopCount = ::g->mus_looping ? XAUDIO2_LOOP_INFINITE : 0;
				Packet.pContext = NULL;

				// Submit packet
				HRESULT hr;
				if( FAILED( hr = pMusicSourceVoice->SubmitSourceBuffer( &Packet ) ) ) {
					int fail = 1;
				}

				// Play the source voice
				if( FAILED( hr = pMusicSourceVoice->Start( 0 ) ) ) {
					int fail = 1;
				}
			}

			waitingForMusic = false;
		}
	}

	if ( pMusicSourceVoice != NULL ) {
		// Set the volume
		pMusicSourceVoice->SetVolume( x_MusicVolume * GLOBAL_VOLUME_MULTIPLIER );
	}
}

/*
======================
I_PauseSong
======================
*/
void I_PauseSong (int handle)
{
	if ( !Music_initialized ) {
		return;
	}

	if ( pMusicSourceVoice != NULL ) {
		// Stop the music source voice
		pMusicSourceVoice->Stop( 0 );
	}
}

/*
======================
I_ResumeSong
======================
*/
void I_ResumeSong (int handle)
{
	if ( !Music_initialized ) {
		return;
	}

	// Stop the music source voice
	if ( pMusicSourceVoice != NULL ) {
		pMusicSourceVoice->Start( 0 );
	}
}

/*
======================
I_StopSong
======================
*/
void I_StopSong(int handle)
{
	if ( !Music_initialized ) {
		return;
	}

	// Stop the music source voice
	if ( pMusicSourceVoice != NULL ) {
		pMusicSourceVoice->Stop( 0 );
	}
}

/*
======================
I_UnRegisterSong
======================
*/
void I_UnRegisterSong(int handle)
{
	// does nothing
}

/*
======================
I_RegisterSong
======================
*/
int I_RegisterSong(void* data, int length)
{
	// does nothing
	return 0;
}
