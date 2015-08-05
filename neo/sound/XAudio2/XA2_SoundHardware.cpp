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
#if defined(USE_DOOMCLASSIC)
#include "../../../doomclassic/doom/i_sound.h"
#endif

idCVar s_showLevelMeter( "s_showLevelMeter", "0", CVAR_BOOL | CVAR_ARCHIVE, "Show VU meter" );
idCVar s_meterTopTime( "s_meterTopTime", "1000", CVAR_INTEGER | CVAR_ARCHIVE, "How long (in milliseconds) peaks are displayed on the VU meter" );
idCVar s_meterPosition( "s_meterPosition", "100 100 20 200", CVAR_ARCHIVE, "VU meter location (x y w h)" );
idCVar s_device( "s_device", "-1", CVAR_INTEGER | CVAR_ARCHIVE, "Which audio device to use (listDevices to list, -1 for default)" );
idCVar s_showPerfData( "s_showPerfData", "0", CVAR_BOOL, "Show XAudio2 Performance data" );
extern idCVar s_volume_dB;

#if defined(USE_WINRT)
HRESULT GetAudioDeviceDetails( _In_ IMMDevice* immDevice, _Out_ AudioDevice* pInfo )
{
	IPropertyStore* propStore = nullptr;
	PROPVARIANT     varName;
	PROPVARIANT     varId;
	
	PropVariantInit( &varId );
	PropVariantInit( &varName );
	
	HRESULT hResult = immDevice->OpenPropertyStore( STGM_READ, &propStore );
	
	if( SUCCEEDED( hResult ) )
	{
		hResult = propStore->GetValue( PKEY_AudioEndpoint_Path, &varId );
	}
	
	if( SUCCEEDED( hResult ) )
	{
		hResult = propStore->GetValue( PKEY_Device_FriendlyName, &varName );
	}
	
	if( SUCCEEDED( hResult ) )
	{
		assert( varId.vt == VT_LPWSTR );
		assert( varName.vt == VT_LPWSTR );
		
		// Now save somewhere the device display name & id
		pInfo->name = varName.pwszVal;
		pInfo->id = varId.pwszVal;
	}
	
	PropVariantClear( &varName );
	PropVariantClear( &varId );
	
	if( propStore != nullptr )
	{
		propStore->Release();
	}
	
	return hResult;
}

std::vector<AudioDevice> EnumerateAudioDevices( _Out_opt_ AudioDevice* defaultDevice = nullptr )
{
	UINT32                   deviceCount      = 0;
	IMMDeviceEnumerator*     immDevEnum       = nullptr;
	IMMDeviceCollection*     immDevCollection = nullptr;
	IMMDevice*               immDev           = nullptr;
	std::vector<AudioDevice> vAudioDevices;
	
	HRESULT hResult = CoCreateInstance(
						  __uuidof( MMDeviceEnumerator ), NULL,
						  CLSCTX_ALL, __uuidof( IMMDeviceEnumerator ), ( void** ) &immDevEnum );
						  
	if( FAILED( hResult ) )
	{
		idLib::Warning( "Failed to get audio enumerator" );
		return std::move( vAudioDevices );
	}
	
	if( defaultDevice != nullptr )
	{
		ZeroMemory( defaultDevice, sizeof( AudioDevice ) );
		
		IMMDevice* defaultImmDev = nullptr;
		
		// @pjb: get the default audio endpoint and make it the first one in the list
		if( SUCCEEDED( immDevEnum->GetDefaultAudioEndpoint( eRender, eConsole, &defaultImmDev ) ) )
		{
			GetAudioDeviceDetails( defaultImmDev, defaultDevice );
			defaultImmDev->Release();
		}
	}
	
	hResult = immDevEnum->EnumAudioEndpoints( eRender, DEVICE_STATE_ACTIVE, &immDevCollection );
	if( FAILED( hResult ) )
	{
		idLib::Warning( "Failed to get audio endpoints" );
		return std::move( vAudioDevices );
	}
	
	hResult = immDevCollection->GetCount( &deviceCount );
	if( FAILED( hResult ) )
	{
		idLib::Warning( "No audio devices found" );
		return std::move( vAudioDevices );
	}
	
	for( UINT i = 0; i < deviceCount; i++ )
	{
		AudioDevice ad;
		
		hResult = immDevCollection->Item( i, &immDev );
		if( SUCCEEDED( hResult ) )
		{
			hResult = GetAudioDeviceDetails( immDev, &ad );
		}
		
		if( SUCCEEDED( hResult ) )
		{
			vAudioDevices.push_back( ad );
		}
		
		if( immDev != nullptr )
		{
			immDev->Release();
		}
	}
	
	immDevCollection->Release();
	immDevEnum->Release();
	
	return std::move( vAudioDevices );
}
#endif

/*
========================
idSoundHardware_XAudio2::idSoundHardware_XAudio2
========================
*/
idSoundHardware_XAudio2::idSoundHardware_XAudio2()
{
	pXAudio2 = NULL;
	pMasterVoice = NULL;
	pSubmixVoice = NULL;
	
	vuMeterRMS = NULL;
	vuMeterPeak = NULL;
	
	outputChannels = 0;
	channelMask = 0;
	
	voices.SetNum( 0 );
	zombieVoices.SetNum( 0 );
	freeVoices.SetNum( 0 );
	
	lastResetTime = 0;
}

void listDevices_f( const idCmdArgs& args )
{

	IXAudio2* pXAudio2 = soundSystemLocal.hardware.GetIXAudio2();
	
	if( pXAudio2 == NULL )
	{
		idLib::Warning( "No xaudio object" );
		return;
	}
	
// RB: not available on Windows 8 SDK
#if defined(USE_WINRT) //(_WIN32_WINNT >= 0x0602 /*_WIN32_WINNT_WIN8*/)
	AudioDevice defaultDevice;
	auto vAudioDevices = EnumerateAudioDevices( &defaultDevice );
	if( vAudioDevices.size() == 0 )
	{
		idLib::Warning( "No audio devices found" );
		return;
	}
	
	for( size_t i = 0; i < vAudioDevices.size(); ++i )
	{
		idLib::Printf( "%s %3d: %S %S\n", vAudioDevices[i].id == defaultDevice.id ? "*" : " ", i, vAudioDevices[i].name.c_str(), vAudioDevices[i].id.c_str() );
	}
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
idSoundHardware_XAudio2::Init
========================
*/
void idSoundHardware_XAudio2::Init()
{

	cmdSystem->AddCommand( "listDevices", listDevices_f, 0, "Lists the connected sound devices", NULL );
	
	DWORD xAudioCreateFlags = 0;
	
// RB: not available on Windows 8 SDK
#if !defined(USE_WINRT) && defined(_DEBUG) // (_WIN32_WINNT < 0x0602 /*_WIN32_WINNT_WIN8*/) && defined(_DEBUG)
	xAudioCreateFlags |= XAUDIO2_DEBUG_ENGINE;
#endif
// RB end

	XAUDIO2_PROCESSOR xAudioProcessor = XAUDIO2_DEFAULT_PROCESSOR;
	
// RB: not available on Windows 8 SDK
	if( FAILED( XAudio2Create( &pXAudio2, xAudioCreateFlags, xAudioProcessor ) ) )
	{
#if !defined(USE_WINRT) && defined(_DEBUG) // (_WIN32_WINNT < 0x0602 /*_WIN32_WINNT_WIN8*/) && defined(_DEBUG)
		if( xAudioCreateFlags & XAUDIO2_DEBUG_ENGINE )
		{
			// in case the debug engine isn't installed
			xAudioCreateFlags &= ~XAUDIO2_DEBUG_ENGINE;
			if( FAILED( XAudio2Create( &pXAudio2, xAudioCreateFlags, xAudioProcessor ) ) )
			{
				idLib::FatalError( "Failed to create XAudio2 engine.  Try installing the latest DirectX." );
				return;
			}
		}
		else
#endif
// RB end
		{
			idLib::FatalError( "Failed to create XAudio2 engine.  Try installing the latest DirectX." );
			return;
		}
	}
#ifdef _DEBUG
	XAUDIO2_DEBUG_CONFIGURATION debugConfiguration = { 0 };
	debugConfiguration.TraceMask = XAUDIO2_LOG_WARNINGS;
	debugConfiguration.BreakMask = XAUDIO2_LOG_ERRORS;
	pXAudio2->SetDebugConfiguration( &debugConfiguration );
#endif
	
	// Register the sound engine callback
	pXAudio2->RegisterForCallbacks( &soundEngineCallback );
	soundEngineCallback.hardware = this;
	DWORD outputSampleRate = 44100; // Max( (DWORD)XAUDIO2FX_REVERB_MIN_FRAMERATE, Min( (DWORD)XAUDIO2FX_REVERB_MAX_FRAMERATE, deviceDetails.OutputFormat.Format.nSamplesPerSec ) );
	
	idCmdArgs args;
	listDevices_f( args );
	
	// RB: not available on Windows 8 SDK
#if defined(USE_WINRT) //(_WIN32_WINNT >= 0x0602 /*_WIN32_WINNT_WIN8*/)
	AudioDevice defaultDevice;
	std::vector<AudioDevice> vAudioDevices = EnumerateAudioDevices( &defaultDevice );
	
	if( !vAudioDevices.empty() )
	{
	
		AudioDevice selectedDevice;
		
		int preferredDevice = s_device.GetInteger();
		bool validPreference = ( preferredDevice >= 0 && preferredDevice < ( int )vAudioDevices.size() );
		// Do we select a device automatically?
		if( validPreference )
		{
			// Use the user's selected device
			selectedDevice = vAudioDevices[preferredDevice];
		}
		else if( !defaultDevice.id.empty() )
		{
			// Fall back to the default device if there is one
			selectedDevice = defaultDevice;
		}
		else
		{
			// Fall back to first device
			selectedDevice = vAudioDevices[0];
		}
		
		if( SUCCEEDED( pXAudio2->CreateMasteringVoice( &pMasterVoice,
					   XAUDIO2_DEFAULT_CHANNELS,
					   outputSampleRate,
					   0,
					   selectedDevice.id.c_str(),
					   NULL,
					   AudioCategory_GameEffects ) ) )
		{
			XAUDIO2_VOICE_DETAILS deviceDetails;
			pMasterVoice->GetVoiceDetails( &deviceDetails );
			
			pMasterVoice->SetVolume( DBtoLinear( s_volume_dB.GetFloat() ) );
			
			outputChannels = deviceDetails.InputChannels;
			DWORD win8_channelMask;
			pMasterVoice->GetChannelMask( &win8_channelMask );
			
			channelMask = ( unsigned int )win8_channelMask;
			idLib::Printf( "Using device %S\n", selectedDevice.name );
		}
		else
		{
			idLib::Warning( "Failed to create master voice" );
			pXAudio2->Release();
			pXAudio2 = NULL;
			return;
		}
	}
	
#else
	UINT32 deviceCount = 0;
	if( pXAudio2->GetDeviceCount( &deviceCount ) != S_OK || deviceCount == 0 )
	{
		idLib::Warning( "No audio devices found" );
		pXAudio2->Release();
		pXAudio2 = NULL;
		return;
	}
	
	int preferredDevice = s_device.GetInteger();
	if( preferredDevice < 0 || preferredDevice >= ( int )deviceCount )
	{
		int preferredChannels = 0;
		for( unsigned int i = 0; i < deviceCount; i++ )
		{
			XAUDIO2_DEVICE_DETAILS deviceDetails;
			if( pXAudio2->GetDeviceDetails( i, &deviceDetails ) != S_OK )
			{
				continue;
			}
	
			if( deviceDetails.Role & DefaultGameDevice )
			{
				// if we find a device the user marked as their preferred 'game' device, then always use that
				preferredDevice = i;
				preferredChannels = deviceDetails.OutputFormat.Format.nChannels;
				break;
			}
	
			if( deviceDetails.OutputFormat.Format.nChannels > preferredChannels )
			{
				preferredDevice = i;
				preferredChannels = deviceDetails.OutputFormat.Format.nChannels;
			}
		}
	}
	
	idLib::Printf( "Using device %d\n", preferredDevice );
	
	XAUDIO2_DEVICE_DETAILS deviceDetails;
	if( pXAudio2->GetDeviceDetails( preferredDevice, &deviceDetails ) != S_OK )
	{
		// One way this could happen is if a device is removed between the loop and this line of code
		// Highly unlikely but possible
		idLib::Warning( "Failed to get device details" );
		pXAudio2->Release();
		pXAudio2 = NULL;
		return;
	}
	
	
	if( FAILED( pXAudio2->CreateMasteringVoice( &pMasterVoice, XAUDIO2_DEFAULT_CHANNELS, outputSampleRate, 0, preferredDevice, NULL ) ) )
	{
		idLib::Warning( "Failed to create master voice" );
		pXAudio2->Release();
		pXAudio2 = NULL;
		return;
	}
	pMasterVoice->SetVolume( DBtoLinear( s_volume_dB.GetFloat() ) );
	
	outputChannels = deviceDetails.OutputFormat.Format.nChannels;
	channelMask = deviceDetails.OutputFormat.dwChannelMask;
	
#endif // #if (_WIN32_WINNT < 0x0602 /*_WIN32_WINNT_WIN8*/)
	
	idSoundVoice::InitSurround( outputChannels, channelMask );
	
#if defined(USE_DOOMCLASSIC)
	// ---------------------
	// Initialize the Doom classic sound system.
	// ---------------------
	I_InitSoundHardware( outputChannels, channelMask );
#endif
	
	// ---------------------
	// Create VU Meter Effect
	// ---------------------
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
	
	// ---------------------
	// Create VU Meter Graph
	// ---------------------
	
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
	
	// ---------------------
	// Create submix buffer
	// ---------------------
	if( FAILED( pXAudio2->CreateSubmixVoice( &pSubmixVoice, 1, outputSampleRate, 0, 0, NULL, NULL ) ) )
	{
		idLib::FatalError( "Failed to create submix voice" );
	}
	
	// XAudio doesn't really impose a maximum number of voices
	voices.SetNum( voices.Max() );
	freeVoices.SetNum( voices.Max() );
	zombieVoices.SetNum( 0 );
	for( int i = 0; i < voices.Num(); i++ )
	{
		freeVoices[i] = &voices[i];
	}
// RB end
}

/*
========================
idSoundHardware_XAudio2::Shutdown
========================
*/
void idSoundHardware_XAudio2::Shutdown()
{
	for( int i = 0; i < voices.Num(); i++ )
	{
		voices[ i ].DestroyInternal();
	}
	voices.Clear();
	freeVoices.Clear();
	zombieVoices.Clear();
	
#if defined(USE_DOOMCLASSIC)
	// ---------------------
	// Shutdown the Doom classic sound system.
	// ---------------------
	I_ShutdownSoundHardware();
#endif
	
	if( pXAudio2 != NULL )
	{
		// Unregister the sound engine callback
		pXAudio2->UnregisterForCallbacks( &soundEngineCallback );
	}
	
	if( pSubmixVoice != NULL )
	{
		pSubmixVoice->DestroyVoice();
		pSubmixVoice = NULL;
	}
	if( pMasterVoice != NULL )
	{
		// release the vu meter effect
		pMasterVoice->SetEffectChain( NULL );
		pMasterVoice->DestroyVoice();
		pMasterVoice = NULL;
	}
	if( pXAudio2 != NULL )
	{
		XAUDIO2_PERFORMANCE_DATA perfData;
		pXAudio2->GetPerformanceData( &perfData );
		idLib::Printf( "Final pXAudio2 performanceData: Voices: %d/%d CPU: %.2f%% Mem: %dkb\n", perfData.ActiveSourceVoiceCount, perfData.TotalSourceVoiceCount, perfData.AudioCyclesSinceLastQuery / ( float )perfData.TotalCyclesSinceLastQuery, perfData.MemoryUsageInBytes / 1024 );
		pXAudio2->Release();
		pXAudio2 = NULL;
	}
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
}

/*
========================
idSoundHardware_XAudio2::AllocateVoice
========================
*/
idSoundVoice* idSoundHardware_XAudio2::AllocateVoice( const idSoundSample* leadinSample, const idSoundSample* loopingSample )
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
		if( voice->CompatibleFormat( ( idSoundSample_XAudio2* )leadinSample ) )
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
idSoundHardware_XAudio2::FreeVoice
========================
*/
void idSoundHardware_XAudio2::FreeVoice( idSoundVoice* voice )
{
	voice->Stop();
	
	// Stop() is asyncronous, so we won't flush bufferes until the
	// voice on the zombie channel actually returns !IsPlaying()
	zombieVoices.Append( voice );
}

/*
========================
idSoundHardware_XAudio2::Update
========================
*/
void idSoundHardware_XAudio2::Update()
{
	if( pXAudio2 == NULL )
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
		pMasterVoice->SetVolume( 0.0f, OPERATION_SET );
	}
	else
	{
		pMasterVoice->SetVolume( DBtoLinear( s_volume_dB.GetFloat() ), OPERATION_SET );
	}
	
	pXAudio2->CommitChanges( XAUDIO2_COMMIT_ALL );
	
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
	
	if( s_showPerfData.GetBool() )
	{
		XAUDIO2_PERFORMANCE_DATA perfData;
		pXAudio2->GetPerformanceData( &perfData );
		idLib::Printf( "Voices: %d/%d CPU: %.2f%% Mem: %dkb\n", perfData.ActiveSourceVoiceCount, perfData.TotalSourceVoiceCount, perfData.AudioCyclesSinceLastQuery / ( float )perfData.TotalCyclesSinceLastQuery, perfData.MemoryUsageInBytes / 1024 );
	}
	
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
void idSoundEngineCallback::OnCriticalError( HRESULT Error )
{
	soundSystemLocal.SetNeedsRestart();
}
