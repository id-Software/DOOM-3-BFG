/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2014-2016 Robert Beckebans
Copyright (C) 2014-2016 Kot in Action Creative Artel

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
#include "precompiled.h"
#pragma hdrstop

#include "snd_local.h"

idCVar s_lockListener( "s_lockListener", "0", CVAR_BOOL, "lock listener updates" );
idCVar s_constantAmplitude( "s_constantAmplitude", "-1", CVAR_FLOAT, "" );
idCVar s_maxEmitterChannels( "s_maxEmitterChannels", "48", CVAR_INTEGER, "Can be set lower than the absolute max of MAX_HARDWARE_VOICES" );
idCVar s_cushionFadeChannels( "s_cushionFadeChannels", "2", CVAR_INTEGER, "Ramp currentCushionDB so this many emitter channels should be silent" );
idCVar s_cushionFadeRate( "s_cushionFadeRate", "60", CVAR_FLOAT, "DB / second change to currentCushionDB" );
idCVar s_cushionFadeLimit( "s_cushionFadeLimit", "-30", CVAR_FLOAT, "Never cushion fade beyond this level" );
idCVar s_cushionFadeOver( "s_cushionFadeOver", "10", CVAR_FLOAT, "DB above s_cushionFadeLimit to start ramp to silence" );
idCVar s_unpauseFadeInTime( "s_unpauseFadeInTime", "250", CVAR_INTEGER, "When unpausing a sound world, milliseconds to fade sounds in over" );
idCVar s_doorDistanceAdd( "s_doorDistanceAdd", "150", CVAR_FLOAT, "reduce sound volume with this distance when going through a door" );
idCVar s_drawSounds( "s_drawSounds", "0", CVAR_INTEGER, "", 0, 2, idCmdSystem::ArgCompletion_Integer<0, 2> );
idCVar s_showVoices( "s_showVoices", "0", CVAR_BOOL, "show active voices" );
idCVar s_volume_dB( "s_volume_dB", "0", CVAR_ARCHIVE | CVAR_FLOAT, "volume in dB" );
extern idCVar s_noSound;

extern void WriteDeclCache( idDemoFile* f, int demoCategory, int demoCode, declType_t  declType );

/*
========================
idSoundWorldLocal::idSoundWorldLocal
========================
*/
idSoundWorldLocal::idSoundWorldLocal()
{
	volumeFade.Clear();
	for( int i = 0; i < SOUND_MAX_CLASSES; i++ )
	{
		soundClassFade[i].Clear();
	}
	renderWorld = NULL;
	writeDemo = NULL;

	listener.axis.Identity();
	listener.pos.Zero();
	listener.id = -1;
	listener.area = 0;

	shakeAmp = 0.0f;
	currentCushionDB = DB_SILENCE;

	localSound = AllocSoundEmitter();

	pauseFade.Clear();
	pausedTime = 0;
	accumulatedPauseTime = 0;
	isPaused = false;

	slowmoSpeed = 1.0f;
	enviroSuitActive = false;
}

/*
========================
idSoundWorldLocal::~idSoundWorldLocal
========================
*/
idSoundWorldLocal::~idSoundWorldLocal()
{

	if( soundSystemLocal.currentSoundWorld == this )
	{
		soundSystemLocal.currentSoundWorld = NULL;
	}

	for( int i = 0; i < emitters.Num(); i++ )
	{
		emitters[i]->Reset();
		emitterAllocator.Free( emitters[i] );
	}

	// Make sure we aren't leaking emitters or channels
	assert( emitterAllocator.GetAllocCount() == 0 );
	assert( channelAllocator.GetAllocCount() == 0 );

	emitterAllocator.Shutdown();
	channelAllocator.Shutdown();

	renderWorld = NULL;
	localSound = NULL;
}

/*
========================
idSoundWorldLocal::AllocSoundEmitter

This is called from the main thread.
========================
*/
idSoundEmitter* idSoundWorldLocal::AllocSoundEmitter()
{
	idSoundEmitterLocal* emitter = emitterAllocator.Alloc();
	emitter->Init( emitters.Append( emitter ), this );
	return emitter;
}

/*
========================
idSoundWorldLocal::AllocSoundChannel
========================
*/
idSoundChannel* idSoundWorldLocal::AllocSoundChannel()
{
	return channelAllocator.Alloc();
}

/*
========================
idSoundWorldLocal::FreeSoundChannel
========================
*/
void idSoundWorldLocal::FreeSoundChannel( idSoundChannel* channel )
{
	channel->Mute();
	channelAllocator.Free( channel );
}

/*
========================
idSoundWorldLocal::CurrentShakeAmplitude
========================
*/
float idSoundWorldLocal::CurrentShakeAmplitude()
{
	if( s_constantAmplitude.GetFloat() >= 0.0f )
	{
		return s_constantAmplitude.GetFloat();
	}
	return shakeAmp;
}

/*
========================
idSoundWorldLocal::PlaceListener
========================
*/
void idSoundWorldLocal::PlaceListener( const idVec3& origin, const idMat3& axis, const int id )
{
	if( writeDemo )
	{
		writeDemo->WriteInt( DS_SOUND );
		writeDemo->WriteInt( SCMD_PLACE_LISTENER );
		writeDemo->WriteVec3( origin );
		writeDemo->WriteMat3( axis );
		writeDemo->WriteInt( id );
	}

	if( s_lockListener.GetBool() )
	{
		return;
	}

	listener.axis = axis;
	listener.pos = origin;
	listener.id = id;

	if( renderWorld )
	{
		listener.area = renderWorld->PointInArea( origin );	// where are we?
	}
	else
	{
		listener.area = 0;
	}
}

/*
========================
idSoundWorldLocal::WriteSoundShaderLoad
========================
*/
void idSoundWorldLocal::WriteSoundShaderLoad( const idSoundShader* snd )
{
	if( writeDemo )
	{
		writeDemo->WriteInt( DS_SOUND );
		writeDemo->WriteInt( SCMD_CACHESOUNDSHADER );
		writeDemo->WriteInt( 1 );
		writeDemo->WriteHashString( snd->GetName() );
	}
}

/*
========================
idActiveChannel
========================
*/
class idActiveChannel
{
public:
	idActiveChannel() :
		channel( NULL ),
		sortKey( 0 ) {}
	idActiveChannel( idSoundChannel* channel_, int sortKey_ ) :
		channel( channel_ ),
		sortKey( sortKey_ ) {}

	idSoundChannel* 	channel;
	int					sortKey;
};

/*
========================
MapVolumeFromFadeDB

Ramp down volumes that are close to fadeDB so that fadeDB is DB_SILENCE
========================
*/
float MapVolumeFromFadeDB( const float volumeDB, const float fadeDB )
{
	if( volumeDB <= fadeDB )
	{
		return DB_SILENCE;
	}

	const float fadeOver = s_cushionFadeOver.GetFloat();
	const float fadeFrom = fadeDB + fadeOver;

	if( volumeDB >= fadeFrom )
	{
		// unchanged
		return volumeDB;
	}
	const float fadeFraction = ( volumeDB - fadeDB ) / fadeOver;

	const float mappedDB = DB_SILENCE + ( fadeFrom - DB_SILENCE ) * fadeFraction;
	return mappedDB;
}

/*
========================
AdjustForCushionChannels

In the very common case of having more sounds that would contribute to the
mix than there are available hardware voices, it can be an audible discontinuity
when a channel initially gets a voice or loses a voice.
To avoid this, make sure that the last few hardware voices are mixed with a volume
of zero, so they won't make a difference as they come and go.
It isn't obvious what the exact best volume ramping method should be, just that
it smoothly change frame to frame.
========================
*/
static float AdjustForCushionChannels( const idStaticList< idActiveChannel, MAX_HARDWARE_VOICES >& activeEmitterChannels,
									   const int uncushionedChannels, const float currentCushionDB, const float driftRate )
{

	float	targetCushionDB;
	if( activeEmitterChannels.Num() <= uncushionedChannels )
	{
		// we should be able to hear all of them
		targetCushionDB = DB_SILENCE;
	}
	else
	{
		// we should be able to hear all of them
		targetCushionDB = activeEmitterChannels[uncushionedChannels].channel->volumeDB;
		if( targetCushionDB < DB_SILENCE )
		{
			targetCushionDB = DB_SILENCE;
		}
		else if( targetCushionDB > s_cushionFadeLimit.GetFloat() )
		{
			targetCushionDB = s_cushionFadeLimit.GetFloat();
		}
	}

	// linearly drift the currentTargetCushionDB towards targetCushionDB
	float	driftedDB = currentCushionDB;
	if( driftedDB < targetCushionDB )
	{
		driftedDB += driftRate;
		if( driftedDB > targetCushionDB )
		{
			driftedDB = targetCushionDB;
		}
	}
	else
	{
		driftedDB -= driftRate;
		if( driftedDB < targetCushionDB )
		{
			driftedDB = targetCushionDB;
		}
	}

	// ramp the lower sound volumes down
	for( int i = 0; i < activeEmitterChannels.Num(); i++ )
	{
		idSoundChannel* chan = activeEmitterChannels[i].channel;
		chan->volumeDB = MapVolumeFromFadeDB( chan->volumeDB, driftedDB );
	}

	return driftedDB;
}

/*
========================
idSoundWorldLocal::Update
========================
*/
void idSoundWorldLocal::Update()
{

	if( s_noSound.GetBool() )
	{
		return;
	}

	// ------------------
	// Update emitters
	//
	// Only loop through the list once to avoid extra cache misses
	// ------------------

	// The naming convention is weird here because we reuse the name "channel"
	// An idSoundChannel is a channel on an emitter, which may have an explicit channel assignment or SND_CHANNEL_ANY
	// A hardware channel is a channel from the sound file itself (IE: left, right, LFE)
	// We only allow MAX_HARDWARE_CHANNELS channels, which may wind up being a smaller number of idSoundChannels
	idStaticList< idActiveChannel, MAX_HARDWARE_VOICES > activeEmitterChannels;
	int	maxEmitterChannels = s_maxEmitterChannels.GetInteger() + 1;	// +1 to leave room for insert-before-sort
	if( maxEmitterChannels > MAX_HARDWARE_VOICES )
	{
		maxEmitterChannels = MAX_HARDWARE_VOICES;
	}

	int activeHardwareChannels = 0;
	int	totalHardwareChannels = 0;
	int	totalEmitterChannels = 0;

	int currentTime = GetSoundTime();
	for( int e = emitters.Num() - 1; e >= 0; e-- )
	{
		// check for freeing a one-shot emitter that is finished playing
		if( emitters[e]->CheckForCompletion( currentTime ) )
		{
			// do a fast list collapse by swapping the last element into
			// the slot we are deleting
			emitters[e]->Reset();
			emitterAllocator.Free( emitters[e] );
			int lastEmitter = emitters.Num() - 1;
			if( e != lastEmitter )
			{
				emitters[e] = emitters[lastEmitter];
				emitters[e]->index = e;
			}
			emitters.SetNum( lastEmitter );
			continue;
		}

		emitters[e]->Update( currentTime );

		totalEmitterChannels += emitters[e]->channels.Num();

		// sort the active channels into the hardware list
		for( int i = 0; i < emitters[e]->channels.Num(); i++ )
		{
			idSoundChannel* channel = emitters[e]->channels[i];

			// check if this channel contributes at all
			const bool canMute = channel->CanMute();
			if( canMute && channel->volumeDB <= DB_SILENCE )
			{
#if !defined(USE_OPENAL)
				channel->Mute();
				continue;
#endif
			}

			// Calculate the sort key.
			// VO can't be stopped and restarted accurately, so always keep VO channels by adding a large value to the sort key.
			const int sortKey = idMath::Ftoi( channel->volumeDB * 100.0f + ( canMute ? 0.0f : 100000.0f ) );

			// Keep track of the total number of hardware channels.
			// This is done after calculating the sort key to avoid a load-hit-store that
			// would occur when using the sort key in the loop below after the Ftoi above.
			const int sampleChannels = channel->leadinSample->NumChannels();
			totalHardwareChannels += sampleChannels;

			// Find the location to insert this channel based on the sort key.
			int insertIndex = 0;
			for( insertIndex = 0; insertIndex < activeEmitterChannels.Num(); insertIndex++ )
			{
				if( sortKey > activeEmitterChannels[insertIndex].sortKey )
				{
					break;
				}
			}

			// Only insert at the end if there is room.
			if( insertIndex == activeEmitterChannels.Num() )
			{
				// Always leave one spot free in the 'activeEmitterChannels' so there is room to insert sort a potentially louder sound later.
				if( activeEmitterChannels.Num() + 1 >= activeEmitterChannels.Max() || activeHardwareChannels + sampleChannels > MAX_HARDWARE_CHANNELS )
				{
					// We don't have enough voices to play this, so mute it if it was playing.
					channel->Mute();
					continue;
				}
			}

			// We want to insert the sound at this point.
			activeEmitterChannels.Insert( idActiveChannel( channel, sortKey ), insertIndex );
			activeHardwareChannels += sampleChannels;

			// If we are over our voice limit or at our channel limit, mute sounds until it fits.
			// If activeEmitterChannels is full, always remove the last one so there is room to insert sort a potentially louder sound later.
			while( activeEmitterChannels.Num() == maxEmitterChannels || activeHardwareChannels > MAX_HARDWARE_CHANNELS )
			{
				const int indexToRemove = activeEmitterChannels.Num() - 1;
				idSoundChannel* const channelToMute = activeEmitterChannels[ indexToRemove ].channel;
				channelToMute->Mute();
				activeHardwareChannels -= channelToMute->leadinSample->NumChannels();
				activeEmitterChannels.RemoveIndex( indexToRemove );
			}
		}
	}

	const float secondsPerFrame = 1.0f / com_engineHz_latched;

	// ------------------
	// In the very common case of having more sounds that would contribute to the
	// mix than there are available hardware voices, it can be an audible discontinuity
	// when a channel initially gets a voice or loses a voice.
	// To avoid this, make sure that the last few hardware voices are mixed with a volume
	// of zero, so they won't make a difference as they come and go.
	// It isn't obvious what the exact best volume ramping method should be, just that
	// it smoothly change frame to frame.
	// ------------------
	const int uncushionedChannels = maxEmitterChannels - s_cushionFadeChannels.GetInteger();
	currentCushionDB = AdjustForCushionChannels( activeEmitterChannels, uncushionedChannels,
					   currentCushionDB, s_cushionFadeRate.GetFloat() * secondsPerFrame );

	// ------------------
	// Update Hardware
	// ------------------
	shakeAmp = 0.0f;

	idStr showVoiceTable;
	bool showVoices = s_showVoices.GetBool();
	if( showVoices )
	{
		showVoiceTable.Format( "currentCushionDB: %5.1f  freeVoices: %i zombieVoices: %i buffers:%i/%i\n", currentCushionDB,
							   soundSystemLocal.hardware.GetNumFreeVoices(), soundSystemLocal.hardware.GetNumZombieVoices(),
							   soundSystemLocal.activeStreamBufferContexts.Num(), soundSystemLocal.freeStreamBufferContexts.Num() );
	}
	for( int i = 0; i < activeEmitterChannels.Num(); i++ )
	{
		idSoundChannel* chan = activeEmitterChannels[i].channel;
		chan->UpdateHardware( 0.0f, currentTime );

		if( showVoices )
		{
			idStr voiceLine;
			voiceLine.Format( "%5.1f db [%3i:%2i] %s", chan->volumeDB, chan->emitter->index, chan->logicalChannel, chan->CanMute() ? "" : " <CANT MUTE>\n" );
			idSoundSample* leadinSample = chan->leadinSample;
			idSoundSample* loopingSample = chan->loopingSample;
			if( loopingSample == NULL )
			{
				voiceLine.Append( va( "%ikhz*%i %s\n", leadinSample->SampleRate() / 1000, leadinSample->NumChannels(), leadinSample->GetName() ) );
			}
			else if( loopingSample == leadinSample )
			{
				voiceLine.Append( va( "%ikhz*%i <LOOPING> %s\n", leadinSample->SampleRate() / 1000, leadinSample->NumChannels(), leadinSample->GetName() ) );
			}
			else
			{
				voiceLine.Append( va( "%ikhz*%i %s | %ikhz*%i %s\n", leadinSample->SampleRate() / 1000, leadinSample->NumChannels(), leadinSample->GetName(), loopingSample->SampleRate() / 1000, loopingSample->NumChannels(), loopingSample->GetName() ) );
			}
			showVoiceTable += voiceLine;
		}

		// Calculate shakes
		if( chan->hardwareVoice == NULL )
		{
			continue;
		}

		shakeAmp += chan->parms.shakes * chan->hardwareVoice->GetGain() * chan->currentAmplitude;
	}
	if( showVoices )
	{
		static idOverlayHandle handle;
		console->PrintOverlay( handle, JUSTIFY_LEFT, showVoiceTable.c_str() );
	}

	if( s_drawSounds.GetBool() && renderWorld != NULL )
	{
		for( int e = 0; e < emitters.Num(); e++ )
		{
			idSoundEmitterLocal* emitter = emitters[e];
			bool audible = false;
			float maxGain = 0.0f;
			for( int c = 0; c < emitter->channels.Num(); c++ )
			{
				if( emitter->channels[c]->hardwareVoice != NULL )
				{
					audible = true;
					maxGain = Max( maxGain, emitter->channels[c]->hardwareVoice->GetGain() );
				}
			}
			if( !audible )
			{
				continue;
			}

			static const int lifetime = 20;

			idBounds ref;
			ref.Clear();
			ref.AddPoint( idVec3( -10.0f ) );
			ref.AddPoint( idVec3( 10.0f ) );

			// draw a box
			renderWorld->DebugBounds( idVec4( maxGain, maxGain, 1.0f, 1.0f ), ref, emitter->origin, lifetime );
			if( emitter->origin != emitter->spatializedOrigin )
			{
				renderWorld->DebugLine( idVec4( 1.0f, 0.0f, 0.0f, 1.0f ), emitter->origin, emitter->spatializedOrigin, lifetime );
			}

			// draw the index
			idVec3 textPos = emitter->origin;
			textPos.z -= 8;
			renderWorld->DrawText( va( "%i", e ), textPos, 0.1f, idVec4( 1, 0, 0, 1 ), listener.axis, 1, lifetime );
			textPos.z += 8;

			// run through all the channels
			for( int k = 0; k < emitter->channels.Num(); k++ )
			{
				idSoundChannel* chan = emitter->channels[k];
				float	min = chan->parms.minDistance;
				float	max = chan->parms.maxDistance;
				const char* defaulted = chan->leadinSample->IsDefault() ? " *DEFAULTED*" : "";
				idStr text;
				text.Format( "%s (%i %i/%i)%s", chan->soundShader->GetName(), idMath::Ftoi( emitter->spatializedDistance ), idMath::Ftoi( min ), idMath::Ftoi( max ), defaulted );
				renderWorld->DrawText( text, textPos, 0.1f, idVec4( 1, 0, 0, 1 ), listener.axis, 1, lifetime );
				textPos.z += 8;
			}
		}
	}
}

/*
========================
idSoundWorldLocal::OnReloadSound
========================
*/
void idSoundWorldLocal::OnReloadSound( const idDecl* shader )
{
	for( int i = 0; i < emitters.Num(); i++ )
	{
		emitters[i]->OnReloadSound( shader );
	}
}

/*
========================
idSoundWorldLocal::EmitterForIndex
========================
*/
idSoundEmitter* idSoundWorldLocal::EmitterForIndex( int index )
{
	// This is only used by save/load code which assumes index = 0 is invalid
	// Which is fine since we use index 0 for the local sound emitter anyway
	if( index <= 0 )
	{
		return NULL;
	}
	if( index >= emitters.Num() )
	{
		idLib::Error( "idSoundWorldLocal::EmitterForIndex: %i >= %i", index, emitters.Num() );
	}
	return emitters[index];
}

/*
========================
idSoundWorldLocal::ClearAllSoundEmitters
========================
*/
void idSoundWorldLocal::ClearAllSoundEmitters()
{
	for( int i = 0; i < emitters.Num(); i++ )
	{
		emitters[i]->Reset();
		emitterAllocator.Free( emitters[i] );
	}
	emitters.Clear();
	localSound = AllocSoundEmitter();
}

/*
========================
idSoundWorldLocal::StopAllSounds

This is called from the main thread.
========================
*/
void idSoundWorldLocal::StopAllSounds()
{
	for( int i = 0; i < emitters.Num(); i++ )
	{
		emitters[i]->Reset();
	}
}

/*
========================
idSoundWorldLocal::PlayShaderDirectly
========================
*/
int idSoundWorldLocal::PlayShaderDirectly( const char* name, int channel )
{
	if( name == NULL || name[0] == 0 )
	{
		localSound->StopSound( channel );
		return 0;
	}
	const idSoundShader* shader = declManager->FindSound( name );
	if( shader == NULL )
	{
		localSound->StopSound( channel );
		return 0;
	}
	else
	{
		return localSound->StartSound( shader, channel, soundSystemLocal.random.RandomFloat(), SSF_GLOBAL, true );
	}
}

/*
========================
idSoundWorldLocal::Skip
========================
*/
void idSoundWorldLocal::Skip( int time )
{
	accumulatedPauseTime -= time;
	pauseFade.SetVolume( DB_SILENCE );
	pauseFade.Fade( 0.0f, s_unpauseFadeInTime.GetInteger(), GetSoundTime() );
}

/*
========================
idSoundWorldLocal::Pause
========================
*/
void idSoundWorldLocal::Pause()
{
	if( !isPaused )
	{
		pausedTime = soundSystemLocal.SoundTime();
		isPaused = true;
		// just pause all unmutable voices (normally just voice overs)
		for( int e = emitters.Num() - 1; e > 0; e-- )
		{
			for( int i = 0; i < emitters[e]->channels.Num(); i++ )
			{
				idSoundChannel* channel = emitters[e]->channels[i];
				if( !channel->CanMute() && channel->hardwareVoice != NULL )
				{
					channel->hardwareVoice->Pause();
				}
			}
		}
	}
}

/*
========================
idSoundWorldLocal::UnPause
========================
*/
void idSoundWorldLocal::UnPause()
{
	if( isPaused )
	{
		isPaused = false;
		accumulatedPauseTime += soundSystemLocal.SoundTime() - pausedTime;
		pauseFade.SetVolume( DB_SILENCE );
		pauseFade.Fade( 0.0f, s_unpauseFadeInTime.GetInteger(), GetSoundTime() );

		// just unpause all unmutable voices (normally just voice overs)
		for( int e = emitters.Num() - 1; e > 0; e-- )
		{
			for( int i = 0; i < emitters[e]->channels.Num(); i++ )
			{
				idSoundChannel* channel = emitters[e]->channels[i];
				if( !channel->CanMute() && channel->hardwareVoice != NULL )
				{
					channel->hardwareVoice->UnPause();
				}
			}
		}
	}
}

/*
========================
idSoundWorldLocal::GetSoundTime
========================
*/
int idSoundWorldLocal::GetSoundTime()
{
	if( isPaused )
	{
		return pausedTime - accumulatedPauseTime;
	}
	else
	{
		return soundSystemLocal.SoundTime() - accumulatedPauseTime;
	}
}

/*
===================
idSoundWorldLocal::ResolveOrigin

Find out of the sound is completely occluded by a closed door portal, or
the virtual sound origin position at the portal closest to the listener.
  this is called by the main thread

dist is the distance from the orignial sound origin to the current portal that enters soundArea
def->distance is the distance we are trying to reduce.

If there is no path through open portals from the sound to the listener, def->spatializedDistance will remain
set at maxDistance
===================
*/
static const int MAX_PORTAL_TRACE_DEPTH = 10;

void idSoundWorldLocal::ResolveOrigin( const int stackDepth, const soundPortalTrace_t* prevStack, const int soundArea, const float dist, const idVec3& soundOrigin, idSoundEmitterLocal* def )
{

	if( dist >= def->spatializedDistance )
	{
		// we can't possibly hear the sound through this chain of portals
		return;
	}

	if( soundArea == listener.area )
	{
		float fullDist = dist + ( soundOrigin - listener.pos ).LengthFast();
		if( fullDist < def->spatializedDistance )
		{
			def->spatializedDistance = fullDist;
			def->spatializedOrigin = soundOrigin;
		}
		return;
	}

	if( stackDepth == MAX_PORTAL_TRACE_DEPTH )
	{
		// don't spend too much time doing these calculations in big maps
		return;
	}

	soundPortalTrace_t newStack;
	newStack.portalArea = soundArea;
	newStack.prevStack = prevStack;

	int numPortals = renderWorld->NumPortalsInArea( soundArea );
	for( int p = 0; p < numPortals; p++ )
	{
		exitPortal_t re = renderWorld->GetPortal( soundArea, p );

		float occlusionDistance = 0;

		// air blocking windows will block sound like closed doors
		if( ( re.blockingBits & ( PS_BLOCK_VIEW | PS_BLOCK_AIR ) ) )
		{
			// we could just completely cut sound off, but reducing the volume works better
			// continue;
			occlusionDistance = s_doorDistanceAdd.GetFloat();
		}

		// what area are we about to go look at
		int otherArea = re.areas[0];
		if( re.areas[0] == soundArea )
		{
			otherArea = re.areas[1];
		}

		// if this area is already in our portal chain, don't bother looking into it
		const soundPortalTrace_t* prev;
		for( prev = prevStack ; prev ; prev = prev->prevStack )
		{
			if( prev->portalArea == otherArea )
			{
				break;
			}
		}
		if( prev )
		{
			continue;
		}

		// pick a point on the portal to serve as our virtual sound origin
		idVec3	source;

		idPlane	pl;
		re.w->GetPlane( pl );

		float	scale;
		idVec3	dir = listener.pos - soundOrigin;
		if( !pl.RayIntersection( soundOrigin, dir, scale ) )
		{
			source = re.w->GetCenter();
		}
		else
		{
			source = soundOrigin + scale * dir;

			// if this point isn't inside the portal edges, slide it in
			for( int i = 0 ; i < re.w->GetNumPoints() ; i++ )
			{
				int j = ( i + 1 ) % re.w->GetNumPoints();
				idVec3	edgeDir = ( *( re.w ) )[j].ToVec3() - ( *( re.w ) )[i].ToVec3();
				idVec3	edgeNormal;

				edgeNormal.Cross( pl.Normal(), edgeDir );

				idVec3	fromVert = source - ( *( re.w ) )[j].ToVec3();

				float d = edgeNormal * fromVert;
				if( d > 0 )
				{
					// move it in
					float div = edgeNormal.Normalize();
					d /= div;

					source -= d * edgeNormal;
				}
			}
		}

		idVec3 tlen = source - soundOrigin;
		float tlenLength = tlen.LengthFast();

		ResolveOrigin( stackDepth + 1, &newStack, otherArea, dist + tlenLength + occlusionDistance, source, def );
	}
}

/*
========================
idSoundWorldLocal::StartWritingDemo
========================
*/
void idSoundWorldLocal::StartWritingDemo( idDemoFile* demo )
{
	writeDemo = demo;

	WriteDeclCache( writeDemo, DS_SOUND, SCMD_CACHESOUNDSHADER, DECL_SOUND );

	writeDemo->WriteInt( DS_SOUND );
	writeDemo->WriteInt( SCMD_STATE );

	// use the normal save game code to archive all the emitters
	WriteToSaveGame( writeDemo );
}

/*
========================
idSoundWorldLocal::StopWritingDemo
========================
*/
void idSoundWorldLocal::StopWritingDemo()
{
	writeDemo = NULL;
}

/*
========================
idSoundWorldLocal::ProcessDemoCommand
========================
*/
void idSoundWorldLocal::ProcessDemoCommand( idDemoFile* readDemo )
{

	if( !readDemo )
	{
		return;
	}

	int index;
	soundDemoCommand_t	dc;

	if( !readDemo->ReadInt( ( int& )dc ) )
	{
		return;
	}

	switch( dc )
	{
		case SCMD_CACHESOUNDSHADER:
		{
			int numCaches = 0;
			readDemo->ReadInt( numCaches );
			for( int i = 0; i < numCaches; ++i )
			{
				const char* declName = readDemo->ReadHashString();
				declManager->FindSound( declName );
			}
			break;
		}
		case SCMD_STATE:
		{
			ReadFromSaveGame( readDemo );
			UnPause();
			break;
		}
		case SCMD_PLACE_LISTENER:
		{
			idVec3	origin;
			idMat3	axis;
			int		listenerId;

			readDemo->ReadVec3( origin );
			readDemo->ReadMat3( axis );
			readDemo->ReadInt( listenerId );

			PlaceListener( origin, axis, listenerId );
		};
		break;
		case SCMD_ALLOC_EMITTER:
		{
			readDemo->ReadInt( index );

			while( emitters.Num() <= index )
			{
				// append a brand new one
				AllocSoundEmitter();
			}
		}
		break;
		case SCMD_FREE:
		{
			int	immediate;

			readDemo->ReadInt( index );
			readDemo->ReadInt( immediate );
			EmitterForIndex( index )->Free( immediate != 0 );
		}
		break;
		case SCMD_UPDATE:
		{
			idVec3 origin;
			int listenerId;
			soundShaderParms_t parms;

			readDemo->ReadInt( index );
			readDemo->ReadVec3( origin );
			readDemo->ReadInt( listenerId );
			readDemo->ReadFloat( parms.minDistance );
			readDemo->ReadFloat( parms.maxDistance );
			readDemo->ReadFloat( parms.volume );
			readDemo->ReadFloat( parms.shakes );
			readDemo->ReadInt( parms.soundShaderFlags );
			readDemo->ReadInt( parms.soundClass );
			EmitterForIndex( index )->UpdateEmitter( origin, listenerId, &parms );
		}
		break;
		case SCMD_START:
		{
			const idSoundShader* shader;
			int			channel;
			float		diversity;
			int			shaderFlags;

			readDemo->ReadInt( index );
			shader = declManager->FindSound( readDemo->ReadHashString() );
			readDemo->ReadInt( channel );
			readDemo->ReadFloat( diversity );
			readDemo->ReadInt( shaderFlags );
			EmitterForIndex( index )->StartSound( shader, ( s_channelType )channel, diversity, shaderFlags );
		}
		break;
		case SCMD_MODIFY:
		{
			int		channel;
			soundShaderParms_t parms;

			readDemo->ReadInt( index );
			readDemo->ReadInt( channel );
			readDemo->ReadFloat( parms.minDistance );
			readDemo->ReadFloat( parms.maxDistance );
			readDemo->ReadFloat( parms.volume );
			readDemo->ReadFloat( parms.shakes );
			readDemo->ReadInt( parms.soundShaderFlags );
			readDemo->ReadInt( parms.soundClass );
			EmitterForIndex( index )->ModifySound( ( s_channelType )channel, &parms );
		}
		break;
		case SCMD_STOP:
		{
			int		channel;

			readDemo->ReadInt( index );
			readDemo->ReadInt( channel );
			EmitterForIndex( index )->StopSound( ( s_channelType )channel );
		}
		break;
		case SCMD_FADE:
		{
			int		channel;
			float	to, over;

			readDemo->ReadInt( index );
			readDemo->ReadInt( channel );
			readDemo->ReadFloat( to );
			readDemo->ReadFloat( over );
			EmitterForIndex( index )->FadeSound( ( s_channelType )channel, to, over );
		}
		break;
	}
}

/*
=================
idSoundWorldLocal::WriteToSaveGame
=================
*/
void idSoundWorldLocal::WriteToSaveGame( idFile* savefile )
{
	struct helper
	{
		static void WriteSoundFade( idFile* savefile, idSoundFade& sf )
		{
			savefile->WriteInt( sf.fadeStartTime );
			savefile->WriteInt( sf.fadeEndTime );
			savefile->WriteFloat( sf.fadeStartVolume );
			savefile->WriteFloat( sf.fadeEndVolume );
		}
		static void WriteShaderParms( idFile* savefile, soundShaderParms_t& parms )
		{
			savefile->WriteFloat( parms.minDistance );
			savefile->WriteFloat( parms.maxDistance );
			savefile->WriteFloat( parms.volume );
			savefile->WriteFloat( parms.shakes );
			savefile->WriteInt( parms.soundShaderFlags );
			savefile->WriteInt( parms.soundClass );
		}
	};
	savefile->WriteInt( GetSoundTime() );

	helper::WriteSoundFade( savefile, volumeFade );
	for( int c = 0; c < SOUND_MAX_CLASSES; c++ )
	{
		helper::WriteSoundFade( savefile, soundClassFade[c] );
	}
	savefile->WriteFloat( slowmoSpeed );
	savefile->WriteBool( enviroSuitActive );

	savefile->WriteMat3( listener.axis );
	savefile->WriteVec3( listener.pos );
	savefile->WriteInt( listener.id );
	savefile->WriteInt( listener.area );

	savefile->WriteFloat( shakeAmp );

	int num = emitters.Num();
	savefile->WriteInt( num );
	// Start at 1 because the local sound emitter is not saved
	for( int e = 1; e < emitters.Num(); e++ )
	{
		idSoundEmitterLocal* emitter = emitters[e];
		savefile->WriteBool( emitter->canFree );
		savefile->WriteVec3( emitter->origin );
		savefile->WriteInt( emitter->emitterId );
		helper::WriteShaderParms( savefile, emitter->parms );
		savefile->WriteInt( emitter->channels.Num() );
		for( int c = 0; c < emitter->channels.Num(); c++ )
		{
			idSoundChannel* channel = emitter->channels[c];
			savefile->WriteInt( channel->startTime );
			savefile->WriteInt( channel->endTime );
			savefile->WriteInt( channel->logicalChannel );
			savefile->WriteBool( channel->allowSlow );
			helper::WriteShaderParms( savefile, channel->parms );
			helper::WriteSoundFade( savefile, channel->volumeFade );
			savefile->WriteString( channel->soundShader->GetName() );
			int leadin = -1;
			int looping = -1;
			for( int i = 0; i < channel->soundShader->entries.Num(); i++ )
			{
				if( channel->soundShader->entries[i] == channel->leadinSample )
				{
					leadin = i;
				}
				if( channel->soundShader->entries[i] == channel->loopingSample )
				{
					looping = i;
				}
			}
			savefile->WriteInt( leadin );
			savefile->WriteInt( looping );
		}
	}
}

/*
=================
idSoundWorldLocal::ReadFromSaveGame
=================
*/
void idSoundWorldLocal::ReadFromSaveGame( idFile* savefile )
{
	struct helper
	{
		static void ReadSoundFade( idFile* savefile, idSoundFade& sf, int timeDelta )
		{
			savefile->ReadInt( sf.fadeStartTime );
			savefile->ReadInt( sf.fadeEndTime );
			savefile->ReadFloat( sf.fadeStartVolume );
			savefile->ReadFloat( sf.fadeEndVolume );
			if( sf.fadeEndTime > 0 )
			{
				sf.fadeStartTime += timeDelta;
				sf.fadeEndTime += timeDelta;
			}
		}
		static void ReadShaderParms( idFile* savefile, soundShaderParms_t& parms )
		{
			savefile->ReadFloat( parms.minDistance );
			savefile->ReadFloat( parms.maxDistance );
			savefile->ReadFloat( parms.volume );
			savefile->ReadFloat( parms.shakes );
			savefile->ReadInt( parms.soundShaderFlags );
			savefile->ReadInt( parms.soundClass );
		}
	};
	int oldSoundTime = 0;
	savefile->ReadInt( oldSoundTime );
	int timeDelta = GetSoundTime() - oldSoundTime;

	helper::ReadSoundFade( savefile, volumeFade, timeDelta );
	for( int c = 0; c < SOUND_MAX_CLASSES; c++ )
	{
		helper::ReadSoundFade( savefile, soundClassFade[c], timeDelta );
	}
	savefile->ReadFloat( slowmoSpeed );
	savefile->ReadBool( enviroSuitActive );

	savefile->ReadMat3( listener.axis );
	savefile->ReadVec3( listener.pos );
	savefile->ReadInt( listener.id );
	savefile->ReadInt( listener.area );

	savefile->ReadFloat( shakeAmp );

	int numEmitters = 0;
	savefile->ReadInt( numEmitters );
	ClearAllSoundEmitters();
	idStr shaderName;
	// Start at 1 because the local sound emitter is not saved
	for( int e = 1; e < numEmitters; e++ )
	{
		idSoundEmitterLocal* emitter = ( idSoundEmitterLocal* )AllocSoundEmitter();
		assert( emitter == emitters[e] );
		assert( emitter->index == e );
		assert( emitter->soundWorld == this );
		assert( emitter->channels.Num() == 0 );
		savefile->ReadBool( emitter->canFree );
		savefile->ReadVec3( emitter->origin );
		savefile->ReadInt( emitter->emitterId );
		helper::ReadShaderParms( savefile, emitter->parms );
		int numChannels = 0;
		savefile->ReadInt( numChannels );
		emitter->channels.SetNum( numChannels );
		for( int c = 0; c < numChannels; c++ )
		{
			idSoundChannel* channel = AllocSoundChannel();
			emitter->channels[c] = channel;
			channel->emitter = emitter;
			savefile->ReadInt( channel->startTime );
			savefile->ReadInt( channel->endTime );
			savefile->ReadInt( channel->logicalChannel );
			savefile->ReadBool( channel->allowSlow );
			helper::ReadShaderParms( savefile, channel->parms );
			helper::ReadSoundFade( savefile, channel->volumeFade, timeDelta );
			savefile->ReadString( shaderName );
			channel->soundShader = declManager->FindSound( shaderName );
			int leadin = 0;
			int looping = 0;
			savefile->ReadInt( leadin );
			savefile->ReadInt( looping );
			// If the leadin entry is not valid (possible if the shader changed after saving) then the looping entry can't be valid either
			if( leadin >= 0 && leadin < channel->soundShader->entries.Num() )
			{
				channel->leadinSample = channel->soundShader->entries[ leadin ];
				if( looping >= 0 && looping < channel->soundShader->entries.Num() )
				{
					channel->loopingSample = channel->soundShader->entries[ looping ];
				}
			}
			else
			{
				channel->leadinSample = NULL;
				channel->loopingSample = NULL;
			}
			channel->startTime += timeDelta;
			if( channel->endTime == 0 )
			{
				// Do nothing, endTime == 0 means loop forever
			}
			else if( channel->endTime <= oldSoundTime )
			{
				// Channel already stopped
				channel->endTime = 1;
			}
			else
			{
				channel->endTime += timeDelta;
			}
		}
	}
}

/*
=================
idSoundWorldLocal::FadeSoundClasses

fade all sounds in the world with a given shader soundClass
to is in Db, over is in seconds
=================
*/
void idSoundWorldLocal::FadeSoundClasses( const int soundClass, const float to, const float over )
{
	if( soundClass < 0 || soundClass >= SOUND_MAX_CLASSES )
	{
		common->Error( "idSoundWorldLocal::FadeSoundClasses: bad soundClass %i", soundClass );
		return;
	}
	soundClassFade[ soundClass ].Fade( to, SEC2MS( over ), GetSoundTime() );
}

/*
=================
idSoundWorldLocal::SetSlowmoSpeed
=================
*/
void idSoundWorldLocal::SetSlowmoSpeed( float speed )
{
	slowmoSpeed = speed;
}

/*
=================
idSoundWorldLocal::SetEnviroSuit
=================
*/
void idSoundWorldLocal::SetEnviroSuit( bool active )
{
	enviroSuitActive = active;
}
