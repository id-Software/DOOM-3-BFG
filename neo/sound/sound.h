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

#ifndef __SOUND__
#define __SOUND__

/*
===============================================================================

	SOUND SHADER DECL

===============================================================================
*/

// unfortunately, our minDistance / maxDistance is specified in meters, and
// we have far too many of them to change at this time.
const float DOOM_TO_METERS = 0.0254f;					// doom to meters
const float METERS_TO_DOOM = ( 1.0f / DOOM_TO_METERS );	// meters to doom

const float DB_SILENCE = -60.0f;

class idSoundSample;

// sound shader flags
static const int	SSF_PRIVATE_SOUND =		BIT( 0 );	// only plays for the current listenerId
static const int	SSF_ANTI_PRIVATE_SOUND = BIT( 1 );	// plays for everyone but the current listenerId
static const int	SSF_NO_OCCLUSION =		BIT( 2 );	// don't flow through portals, only use straight line
static const int	SSF_GLOBAL =			BIT( 3 );	// play full volume to all speakers and all listeners
static const int	SSF_OMNIDIRECTIONAL =	BIT( 4 );	// fall off with distance, but play same volume in all speakers
static const int	SSF_LOOPING =			BIT( 5 );	// repeat the sound continuously
static const int	SSF_PLAY_ONCE =			BIT( 6 );	// never restart if already playing on any channel of a given emitter
static const int	SSF_UNCLAMPED =			BIT( 7 );	// don't clamp calculated volumes at 1.0
static const int	SSF_NO_FLICKER =		BIT( 8 );	// always return 1.0 for volume queries
static const int	SSF_NO_DUPS =			BIT( 9 );	// try not to play the same sound twice in a row
static const int	SSF_VO =				BIT( 10 ); // VO - direct a portion of the sound through the center channel (set automatically on shaders that contain files that start with "sound/vo/")
static const int	SSF_MUSIC =				BIT( 11 ); // Music - Muted when the player is playing his own music

// these options can be overriden from sound shader defaults on a per-emitter and per-channel basis
typedef struct
{
	float					minDistance;
	float					maxDistance;
	float					volume;					// in dB.  Negative values get quieter
	float					shakes;
	int						soundShaderFlags;		// SSF_* bit flags
	int						soundClass;				// for global fading of sounds
} soundShaderParms_t;

// sound classes are used to fade most sounds down inside cinematics, leaving dialog
// flagged with a non-zero class full volume
const int		SOUND_MAX_CLASSES		= 4;

// it is somewhat tempting to make this a virtual class to hide the private
// details here, but that doesn't fit easily with the decl manager at the moment.
class idSoundShader : public idDecl
{
public:
	idSoundShader();
	virtual					~idSoundShader();

	virtual size_t			Size() const;
	virtual bool			SetDefaultText();
	virtual const char* 	DefaultDefinition() const;
	virtual bool			Parse( const char* text, const int textLength, bool allowBinaryVersion );
	virtual void			FreeData();
	virtual void			List() const;

	// so the editor can draw correct default sound spheres
	// this is currently defined as meters, which sucks, IMHO.
	virtual float			GetMinDistance() const;		// FIXME: replace this with a GetSoundShaderParms()
	virtual float			GetMaxDistance() const;

	// returns NULL if an AltSound isn't defined in the shader.
	// we use this for pairing a specific broken light sound with a normal light sound
	virtual const idSoundShader* GetAltSound() const;

	virtual bool			HasDefaultSound() const;

	virtual const soundShaderParms_t* GetParms() const;
	virtual int				GetNumSounds() const;
	virtual const char* 	GetSound( int index ) const;

private:
	friend class idSoundWorldLocal;
	friend class idSoundEmitterLocal;
	friend class idSoundChannel;

	// options from sound shader text
	soundShaderParms_t		parms;			// can be overriden on a per-channel basis

	int						speakerMask;
	const idSoundShader* 	altSound;

	bool					leadin;			// true if this sound has a leadin
	float					leadinVolume;	// allows light breaking leadin sounds to be much louder than the broken loop

	idList<idSoundSample*, TAG_AUDIO>	entries;

private:
	void					Init();
	bool					ParseShader( idLexer& src );
};

/*
===============================================================================

	SOUND EMITTER

===============================================================================
*/

// sound channels
static const int SCHANNEL_ANY = 0;	// used in queries and commands to effect every channel at once, in
// startSound to have it not override any other channel
static const int SCHANNEL_ONE = 1;	// any following integer can be used as a channel number
typedef int s_channelType;	// the game uses its own series of enums, and we don't want to require casts


class idSoundEmitter
{
public:
	virtual					~idSoundEmitter() {}

	// a non-immediate free will let all currently playing sounds complete
	// soundEmitters are not actually deleted, they are just marked as
	// reusable by the soundWorld
	virtual void			Free( bool immediate ) = 0;

	// the parms specified will be the default overrides for all sounds started on this emitter.
	// NULL is acceptable for parms
	virtual void			UpdateEmitter( const idVec3& origin, int listenerId, const soundShaderParms_t* parms ) = 0;

	// returns the length of the started sound in msec
	virtual int				StartSound( const idSoundShader* shader, const s_channelType channel, float diversity = 0, int shaderFlags = 0, bool allowSlow = true ) = 0;

	// pass SCHANNEL_ANY to effect all channels
	virtual void			ModifySound( const s_channelType channel, const soundShaderParms_t* parms ) = 0;
	virtual void			StopSound( const s_channelType channel ) = 0;
	// to is in Db, over is in seconds
	virtual void			FadeSound( const s_channelType channel, float to, float over ) = 0;

	// returns true if there are any sounds playing from this emitter.  There is some conservative
	// slop at the end to remove inconsistent race conditions with the sound thread updates.
	// FIXME: network game: on a dedicated server, this will always be false
	virtual bool			CurrentlyPlaying( const s_channelType channel = SCHANNEL_ANY ) const = 0;

	// returns a 0.0 to 1.0 value based on the current sound amplitude, allowing
	// graphic effects to be modified in time with the audio.
	// just samples the raw wav file, it doesn't account for volume overrides in the
	virtual	float			CurrentAmplitude() = 0;

	// for save games.  Index will always be > 0
	virtual	int				Index() const = 0;
};

/*
===============================================================================

	SOUND WORLD

There can be multiple independent sound worlds, just as there can be multiple
independent render worlds.  The prime example is the editor sound preview
option existing simultaniously with a live game.
===============================================================================
*/

class idSoundWorld
{
public:
	virtual					~idSoundWorld() {}

	// call at each map start
	virtual void			ClearAllSoundEmitters() = 0;
	virtual void			StopAllSounds() = 0;

	// get a new emitter that can play sounds in this world
	virtual idSoundEmitter* AllocSoundEmitter() = 0;

	// for load games, index 0 will return NULL
	virtual idSoundEmitter* EmitterForIndex( int index ) = 0;

	// query sound samples from all emitters reaching a given listener
	virtual float			CurrentShakeAmplitude() = 0;

	// where is the camera/microphone
	// listenerId allows listener-private and antiPrivate sounds to be filtered
	virtual void			PlaceListener( const idVec3& origin, const idMat3& axis, const int listenerId ) = 0;

	// fade all sounds in the world with a given shader soundClass
	// to is in Db, over is in seconds
	virtual void			FadeSoundClasses( const int soundClass, const float to, const float over ) = 0;

	// menu sounds
	virtual	int				PlayShaderDirectly( const char* name, int channel = -1 ) = 0;

	// when cinematics are skipped, we need to advance sound time this much
	virtual void			Skip( int time ) = 0;

	// pause and unpause the sound world
	virtual void			Pause() = 0;
	virtual void			UnPause() = 0;
	virtual bool			IsPaused() = 0;

	// SaveGame / demo Support
	virtual void			WriteToSaveGame( idFile* savefile ) = 0;
	virtual void			ReadFromSaveGame( idFile* savefile ) = 0;

	virtual void			SetSlowmoSpeed( float speed ) = 0;
	virtual void			SetEnviroSuit( bool active ) = 0;
};


/*
===============================================================================

	SOUND SYSTEM

===============================================================================
*/

typedef struct
{
	idStr					name;
	idStr					format;
	int						numChannels;
	int						numSamplesPerSecond;
	int						num44kHzSamples;
	int						numBytes;
	bool					looping;
	float					lastVolume;
	int						start44kHzTime;
	int						current44kHzTime;
} soundDecoderInfo_t;


class idSoundSystem
{
public:
	virtual					~idSoundSystem() {}

	// All non-hardware initialization.
	virtual void			Init() = 0;

	// Shutdown routine.
	virtual	void			Shutdown() = 0;

	// The renderWorld is used for visualization and light amplitude sampling.
	virtual idSoundWorld* 	AllocSoundWorld( idRenderWorld* rw ) = 0;
	virtual void			FreeSoundWorld( idSoundWorld* sw ) = 0;

	// Specifying NULL will cause silence to be played.
	virtual void			SetPlayingSoundWorld( idSoundWorld* soundWorld ) = 0;

	// Some tools, like the sound dialog, may be used in both the game and the editor
	// This can return NULL, so check!
	virtual idSoundWorld* 	GetPlayingSoundWorld() = 0;

	// Sends the current playing sound world information to the sound hardware.
	virtual void			Render() = 0;

	virtual void			MuteBackgroundMusic( bool mute ) = 0;

	// Sets the final output volume to 0.
	virtual void			SetMute( bool mute ) = 0;
	virtual bool			IsMuted() = 0;

	// Called by the decl system when a sound decl is reloaded
	virtual void			OnReloadSound( const idDecl* sound ) = 0;

	// Called before freeing any sound sample resources
	virtual void			StopAllSounds() = 0;

	// May be called to free memory for level loads
	virtual void			InitStreamBuffers() = 0;
	virtual void			FreeStreamBuffers() = 0;

	// video playback needs to get this
	virtual void* 			GetIXAudio2() const = 0; // FIXME: stupid name if we have other backends

#if defined(USE_OPENAL)
	virtual void*			GetOpenALDevice() const = 0;
#endif

	// for the sound level meter window
	virtual cinData_t		ImageForTime( const int milliseconds, const bool waveform ) = 0;

	// Free all sounds loaded during the last map load
	virtual	void			BeginLevelLoad() = 0;

	// Load all sounds marked as used this level
	virtual	void			EndLevelLoad() = 0;

	virtual void			Preload( idPreloadManifest& preload ) = 0;

	// prints memory info
	virtual void			PrintMemInfo( MemInfo_t* mi ) = 0;
};

extern idSoundSystem*	soundSystem;

#endif /* !__SOUND__ */
