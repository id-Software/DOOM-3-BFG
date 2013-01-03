/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013 Robert Beckebans

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
#pragma hdrstop
#include "precompiled.h"
#include "../snd_local.h"
//#include "../../../doomclassic/doom/i_sound.h"

idCVar s_showLevelMeter( "s_showLevelMeter", "0", CVAR_BOOL | CVAR_ARCHIVE, "Show VU meter" );
idCVar s_meterTopTime( "s_meterTopTime", "1000", CVAR_INTEGER | CVAR_ARCHIVE, "How long (in milliseconds) peaks are displayed on the VU meter" );
idCVar s_meterPosition( "s_meterPosition", "100 100 20 200", CVAR_ARCHIVE, "VU meter location (x y w h)" );
idCVar s_device( "s_device", "-1", CVAR_INTEGER | CVAR_ARCHIVE, "Which audio device to use (listDevices to list, -1 for default)" );
idCVar s_showPerfData( "s_showPerfData", "0", CVAR_BOOL, "Show XAudio2 Performance data" );
extern idCVar s_volume_dB;

/*
========================
idSoundHardware_OpenAL::idSoundHardware_OpenAL
========================
*/
idSoundHardware_OpenAL::idSoundHardware_OpenAL()
{
	openalDevice = NULL;
	openalContext = NULL;
	
	//vuMeterRMS = NULL;
	//vuMeterPeak = NULL;
	
	//outputChannels = 0;
	//channelMask = 0;
	
	voices.SetNum( 0 );
	zombieVoices.SetNum( 0 );
	freeVoices.SetNum( 0 );
	
	lastResetTime = 0;
}

void listDevices_f( const idCmdArgs& args )
{
#if 1
	// TODO
	
	idLib::Warning( "No audio devices found" );
	return;
#else
	UINT32 deviceCount = 0;
	if( pXAudio2->GetDeviceCount( &deviceCount ) != S_OK || deviceCount == 0 )
	{
		idLib::Warning( "No audio devices found" );
		return;
	}
	
	for( unsigned int i = 0; i < deviceCount; i++ )
	{
		XAUDIO2_DEVICE_DETAILS deviceDetails;
		if( pXAudio2->GetDeviceDetails( i, &deviceDetails ) != S_OK )
		{
			continue;
		}
		idStaticList< const char*, 5 > roles;
		if( deviceDetails.Role & DefaultConsoleDevice )
		{
			roles.Append( "Console Device" );
		}
		if( deviceDetails.Role & DefaultMultimediaDevice )
		{
			roles.Append( "Multimedia Device" );
		}
		if( deviceDetails.Role & DefaultCommunicationsDevice )
		{
			roles.Append( "Communications Device" );
		}
		if( deviceDetails.Role & DefaultGameDevice )
		{
			roles.Append( "Game Device" );
		}
		idStaticList< const char*, 11 > channelNames;
		if( deviceDetails.OutputFormat.dwChannelMask & SPEAKER_FRONT_LEFT )
		{
			channelNames.Append( "Front Left" );
		}
		if( deviceDetails.OutputFormat.dwChannelMask & SPEAKER_FRONT_RIGHT )
		{
			channelNames.Append( "Front Right" );
		}
		if( deviceDetails.OutputFormat.dwChannelMask & SPEAKER_FRONT_CENTER )
		{
			channelNames.Append( "Front Center" );
		}
		if( deviceDetails.OutputFormat.dwChannelMask & SPEAKER_LOW_FREQUENCY )
		{
			channelNames.Append( "Low Frequency" );
		}
		if( deviceDetails.OutputFormat.dwChannelMask & SPEAKER_BACK_LEFT )
		{
			channelNames.Append( "Back Left" );
		}
		if( deviceDetails.OutputFormat.dwChannelMask & SPEAKER_BACK_RIGHT )
		{
			channelNames.Append( "Back Right" );
		}
		if( deviceDetails.OutputFormat.dwChannelMask & SPEAKER_FRONT_LEFT_OF_CENTER )
		{
			channelNames.Append( "Front Left of Center" );
		}
		if( deviceDetails.OutputFormat.dwChannelMask & SPEAKER_FRONT_RIGHT_OF_CENTER )
		{
			channelNames.Append( "Front Right of Center" );
		}
		if( deviceDetails.OutputFormat.dwChannelMask & SPEAKER_BACK_CENTER )
		{
			channelNames.Append( "Back Center" );
		}
		if( deviceDetails.OutputFormat.dwChannelMask & SPEAKER_SIDE_LEFT )
		{
			channelNames.Append( "Side Left" );
		}
		if( deviceDetails.OutputFormat.dwChannelMask & SPEAKER_SIDE_RIGHT )
		{
			channelNames.Append( "Side Right" );
		}
		char mbcsDisplayName[ 256 ];
		wcstombs( mbcsDisplayName, deviceDetails.DisplayName, sizeof( mbcsDisplayName ) );
		idLib::Printf( "%3d: %s\n", i, mbcsDisplayName );
		idLib::Printf( "     %d channels, %d Hz\n", deviceDetails.OutputFormat.Format.nChannels, deviceDetails.OutputFormat.Format.nSamplesPerSec );
		if( channelNames.Num() != deviceDetails.OutputFormat.Format.nChannels )
		{
			idLib::Printf( S_COLOR_YELLOW "WARNING: " S_COLOR_RED "Mismatch between # of channels and channel mask\n" );
		}
		if( channelNames.Num() == 1 )
		{
			idLib::Printf( "     %s\n", channelNames[0] );
		}
		else if( channelNames.Num() == 2 )
		{
			idLib::Printf( "     %s and %s\n", channelNames[0], channelNames[1] );
		}
		else if( channelNames.Num() > 2 )
		{
			idLib::Printf( "     %s", channelNames[0] );
			for( int i = 1; i < channelNames.Num() - 1; i++ )
			{
				idLib::Printf( ", %s", channelNames[i] );
			}
			idLib::Printf( ", and %s\n", channelNames[channelNames.Num() - 1] );
		}
		if( roles.Num() == 1 )
		{
			idLib::Printf( "     Default %s\n", roles[0] );
		}
		else if( roles.Num() == 2 )
		{
			idLib::Printf( "     Default %s and %s\n", roles[0], roles[1] );
		}
		else if( roles.Num() > 2 )
		{
			idLib::Printf( "     Default %s", roles[0] );
			for( int i = 1; i < roles.Num() - 1; i++ )
			{
				idLib::Printf( ", %s", roles[i] );
			}
			idLib::Printf( ", and %s\n", roles[roles.Num() - 1] );
		}
	}
#endif
// RB end
}

/*
========================
idSoundHardware_OpenAL::Init
========================
*/
void idSoundHardware_OpenAL::Init()
{
	cmdSystem->AddCommand( "listDevices", listDevices_f, 0, "Lists the connected sound devices", NULL );
	
	common->Printf( "Setup OpenAL device and context... " );
	
	openalDevice = alcOpenDevice( NULL );
	if( openalDevice == NULL )
	{
		common->FatalError( "idSoundHardware_OpenAL::Init: alcOpenDevice() failed\n" );
		return;
	}
	
	openalContext = alcCreateContext( openalDevice, NULL );
	if( alcMakeContextCurrent( openalContext ) == 0 )
	{
		common->FatalError( "idSoundHardware_OpenAL::Init: alcMakeContextCurrent( %p) failed\n", openalContext );
		return;
	}
	
	common->Printf( "Done.\n" );
	
	common->Printf( "OpenAL vendor: %s\n", alGetString( AL_VENDOR ) );
	common->Printf( "OpenAL renderer: %s\n", alGetString( AL_RENDERER ) );
	common->Printf( "OpenAL version: %s\n", alGetString( AL_VERSION ) );
	
	//pMasterVoice->SetVolume( DBtoLinear( s_volume_dB.GetFloat() ) );
	
	//outputChannels = deviceDetails.OutputFormat.Format.nChannels;
	//channelMask = deviceDetails.OutputFormat.dwChannelMask;
	
	//idSoundVoice::InitSurround( outputChannels, channelMask );
	
	// ---------------------
	// Initialize the Doom classic sound system.
	// ---------------------
	//I_InitSoundHardware( outputChannels, channelMask );
	
	// ---------------------
	// Create VU Meter Effect
	// ---------------------
	/*
	IUnknown* vuMeter = NULL;
	XAudio2CreateVolumeMeter( &vuMeter, 0 );
	
	XAUDIO2_EFFECT_DESCRIPTOR descriptor;
	descriptor.InitialState = true;
	descriptor.OutputChannels = outputChannels;
	descriptor.pEffect = vuMeter;
	
	XAUDIO2_EFFECT_CHAIN chain;
	chain.EffectCount = 1;
	chain.pEffectDescriptors = &descriptor;
	
	pMasterVoice->SetEffectChain( &chain );
	
	vuMeter->Release();
	*/
	
	// ---------------------
	// Create VU Meter Graph
	// ---------------------
	
	/*
	vuMeterRMS = console->CreateGraph( outputChannels );
	vuMeterPeak = console->CreateGraph( outputChannels );
	vuMeterRMS->Enable( false );
	vuMeterPeak->Enable( false );
	
	memset( vuMeterPeakTimes, 0, sizeof( vuMeterPeakTimes ) );
	
	vuMeterPeak->SetFillMode( idDebugGraph::GRAPH_LINE );
	vuMeterPeak->SetBackgroundColor( idVec4( 0.0f, 0.0f, 0.0f, 0.0f ) );
	
	vuMeterRMS->AddGridLine( 0.500f, idVec4( 0.5f, 0.5f, 0.5f, 1.0f ) );
	vuMeterRMS->AddGridLine( 0.250f, idVec4( 0.5f, 0.5f, 0.5f, 1.0f ) );
	vuMeterRMS->AddGridLine( 0.125f, idVec4( 0.5f, 0.5f, 0.5f, 1.0f ) );
	
	const char* channelNames[] = { "L", "R", "C", "S", "Lb", "Rb", "Lf", "Rf", "Cb", "Ls", "Rs" };
	for( int i = 0, ci = 0; ci < sizeof( channelNames ) / sizeof( channelNames[0] ); ci++ )
	{
		if( ( channelMask & BIT( ci ) ) == 0 )
		{
			continue;
		}
		vuMeterRMS->SetLabel( i, channelNames[ ci ] );
		i++;
	}
	*/
	
	// OpenAL doesn't really impose a maximum number of sources
	voices.SetNum( voices.Max() );
	freeVoices.SetNum( voices.Max() );
	zombieVoices.SetNum( 0 );
	for( int i = 0; i < voices.Num(); i++ )
	{
		freeVoices[i] = &voices[i];
	}
}

/*
========================
idSoundHardware_OpenAL::Shutdown
========================
*/
void idSoundHardware_OpenAL::Shutdown()
{
	for( int i = 0; i < voices.Num(); i++ )
	{
		voices[ i ].DestroyInternal();
	}
	voices.Clear();
	freeVoices.Clear();
	zombieVoices.Clear();
	
	alcMakeContextCurrent( NULL );
	
	alcDestroyContext( openalContext );
	openalContext = NULL;
	
	alcCloseDevice( openalDevice );
	openalDevice = NULL;
	
	
	// ---------------------
	// Shutdown the Doom classic sound system.
	// ---------------------
	//I_ShutdownSoundHardware();
	
	/*
	if( vuMeterRMS != NULL )
	{
		console->DestroyGraph( vuMeterRMS );
		vuMeterRMS = NULL;
	}
	if( vuMeterPeak != NULL )
	{
		console->DestroyGraph( vuMeterPeak );
		vuMeterPeak = NULL;
	}
	*/
}

/*
========================
idSoundHardware_OpenAL::AllocateVoice
========================
*/
idSoundVoice* idSoundHardware_OpenAL::AllocateVoice( const idSoundSample* leadinSample, const idSoundSample* loopingSample )
{
	if( leadinSample == NULL )
	{
		return NULL;
	}
	if( loopingSample != NULL )
	{
		if( ( leadinSample->format.basic.formatTag != loopingSample->format.basic.formatTag ) || ( leadinSample->format.basic.numChannels != loopingSample->format.basic.numChannels ) )
		{
			idLib::Warning( "Leadin/looping format mismatch: %s & %s", leadinSample->GetName(), loopingSample->GetName() );
			loopingSample = NULL;
		}
	}
	
	// Try to find a free voice that matches the format
	// But fallback to the last free voice if none match the format
	idSoundVoice* voice = NULL;
	for( int i = 0; i < freeVoices.Num(); i++ )
	{
		if( freeVoices[i]->IsPlaying() )
		{
			continue;
		}
		voice = ( idSoundVoice* )freeVoices[i];
		if( voice->CompatibleFormat( ( idSoundSample_OpenAL* )leadinSample ) )
		{
			break;
		}
	}
	if( voice != NULL )
	{
		voice->Create( leadinSample, loopingSample );
		freeVoices.Remove( voice );
		return voice;
	}
	
	return NULL;
}

/*
========================
idSoundHardware_OpenAL::FreeVoice
========================
*/
void idSoundHardware_OpenAL::FreeVoice( idSoundVoice* voice )
{
	voice->Stop();
	
	// Stop() is asyncronous, so we won't flush bufferes until the
	// voice on the zombie channel actually returns !IsPlaying()
	zombieVoices.Append( voice );
}

/*
========================
idSoundHardware_OpenAL::Update
========================
*/
void idSoundHardware_OpenAL::Update()
{
	if( openalDevice == NULL )
	{
		int nowTime = Sys_Milliseconds();
		if( lastResetTime + 1000 < nowTime )
		{
			lastResetTime = nowTime;
			Init();
		}
		return;
	}
	
	if( soundSystem->IsMuted() )
	{
		alListenerf( AL_GAIN, 0.0f );
	}
	else
	{
		alListenerf( AL_GAIN, DBtoLinear( s_volume_dB.GetFloat() ) );
	}
	
	// IXAudio2SourceVoice::Stop() has been called for every sound on the
	// zombie list, but it is documented as asyncronous, so we have to wait
	// until it actually reports that it is no longer playing.
	for( int i = 0; i < zombieVoices.Num(); i++ )
	{
		zombieVoices[i]->FlushSourceBuffers();
		if( !zombieVoices[i]->IsPlaying() )
		{
			freeVoices.Append( zombieVoices[i] );
			zombieVoices.RemoveIndexFast( i );
			i--;
		}
		else
		{
			static int playingZombies;
			playingZombies++;
		}
	}
	
	/*
	if( s_showPerfData.GetBool() )
	{
		XAUDIO2_PERFORMANCE_DATA perfData;
		pXAudio2->GetPerformanceData( &perfData );
		idLib::Printf( "Voices: %d/%d CPU: %.2f%% Mem: %dkb\n", perfData.ActiveSourceVoiceCount, perfData.TotalSourceVoiceCount, perfData.AudioCyclesSinceLastQuery / ( float )perfData.TotalCyclesSinceLastQuery, perfData.MemoryUsageInBytes / 1024 );
	}
	*/
	
	/*
	if( vuMeterRMS == NULL )
	{
		// Init probably hasn't been called yet
		return;
	}
	
	vuMeterRMS->Enable( s_showLevelMeter.GetBool() );
	vuMeterPeak->Enable( s_showLevelMeter.GetBool() );
	
	if( !s_showLevelMeter.GetBool() )
	{
		pMasterVoice->DisableEffect( 0 );
		return;
	}
	else
	{
		pMasterVoice->EnableEffect( 0 );
	}
	
	float peakLevels[ 8 ];
	float rmsLevels[ 8 ];
	
	XAUDIO2FX_VOLUMEMETER_LEVELS levels;
	levels.ChannelCount = outputChannels;
	levels.pPeakLevels = peakLevels;
	levels.pRMSLevels = rmsLevels;
	
	if( levels.ChannelCount > 8 )
	{
		levels.ChannelCount = 8;
	}
	
	pMasterVoice->GetEffectParameters( 0, &levels, sizeof( levels ) );
	
	int currentTime = Sys_Milliseconds();
	for( int i = 0; i < outputChannels; i++ )
	{
		if( vuMeterPeakTimes[i] < currentTime )
		{
			vuMeterPeak->SetValue( i, vuMeterPeak->GetValue( i ) * 0.9f, colorRed );
		}
	}
	
	float width = 20.0f;
	float height = 200.0f;
	float left = 100.0f;
	float top = 100.0f;
	
	sscanf( s_meterPosition.GetString(), "%f %f %f %f", &left, &top, &width, &height );
	
	vuMeterRMS->SetPosition( left, top, width * levels.ChannelCount, height );
	vuMeterPeak->SetPosition( left, top, width * levels.ChannelCount, height );
	
	for( uint32 i = 0; i < levels.ChannelCount; i++ )
	{
		vuMeterRMS->SetValue( i, rmsLevels[ i ], idVec4( 0.5f, 1.0f, 0.0f, 1.00f ) );
		if( peakLevels[ i ] >= vuMeterPeak->GetValue( i ) )
		{
			vuMeterPeak->SetValue( i, peakLevels[ i ], colorRed );
			vuMeterPeakTimes[i] = currentTime + s_meterTopTime.GetInteger();
		}
	}
	*/
}


/*
================================================
idSoundEngineCallback
================================================
*/

/*
========================
idSoundEngineCallback::OnCriticalError
========================
*/
/*
void idSoundEngineCallback::OnCriticalError( HRESULT Error )
{
	soundSystemLocal.SetNeedsRestart();
}
*/
