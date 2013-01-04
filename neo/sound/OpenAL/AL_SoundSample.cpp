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

extern idCVar s_useCompression;
extern idCVar s_noSound;

#define GPU_CONVERT_CPU_TO_CPU_CACHED_READONLY_ADDRESS( x ) x

const uint32 SOUND_MAGIC_IDMSA = 0x6D7A7274;

extern idCVar sys_lang;

/*
========================
AllocBuffer
========================
*/
static void* AllocBuffer( int size, const char* name )
{
	return Mem_Alloc( size, TAG_AUDIO );
}

/*
========================
FreeBuffer
========================
*/
static void FreeBuffer( void* p )
{
	return Mem_Free( p );
}

/*
========================
idSoundSample_OpenAL::idSoundSample_OpenAL
========================
*/
idSoundSample_OpenAL::idSoundSample_OpenAL()
{
	timestamp = FILE_NOT_FOUND_TIMESTAMP;
	loaded = false;
	neverPurge = false;
	levelLoadReferenced = false;
	
	memset( &format, 0, sizeof( format ) );
	
	totalBufferSize = 0;
	
	playBegin = 0;
	playLength = 0;
	
	lastPlayedTime = 0;
	
	openalBuffer = 0;
}

/*
========================
idSoundSample_OpenAL::~idSoundSample_OpenAL
========================
*/
idSoundSample_OpenAL::~idSoundSample_OpenAL()
{
	FreeData();
}

/*
========================
idSoundSample_OpenAL::WriteGeneratedSample
========================
*/
void idSoundSample_OpenAL::WriteGeneratedSample( idFile* fileOut )
{
	fileOut->WriteBig( SOUND_MAGIC_IDMSA );
	fileOut->WriteBig( timestamp );
	fileOut->WriteBig( loaded );
	fileOut->WriteBig( playBegin );
	fileOut->WriteBig( playLength );
	idWaveFile::WriteWaveFormatDirect( format, fileOut );
	fileOut->WriteBig( ( int )amplitude.Num() );
	fileOut->Write( amplitude.Ptr(), amplitude.Num() );
	fileOut->WriteBig( totalBufferSize );
	fileOut->WriteBig( ( int )buffers.Num() );
	for( int i = 0; i < buffers.Num(); i++ )
	{
		fileOut->WriteBig( buffers[ i ].numSamples );
		fileOut->WriteBig( buffers[ i ].bufferSize );
		fileOut->Write( buffers[ i ].buffer, buffers[ i ].bufferSize );
	};
}
/*
========================
idSoundSample_OpenAL::WriteAllSamples
========================
*/
void idSoundSample_OpenAL::WriteAllSamples( const idStr& sampleName )
{
	idSoundSample_OpenAL* samplePC = new idSoundSample_OpenAL();
	{
		idStrStatic< MAX_OSPATH > inName = sampleName;
		inName.Append( ".msadpcm" );
		idStrStatic< MAX_OSPATH > inName2 = sampleName;
		inName2.Append( ".wav" );
		
		idStrStatic< MAX_OSPATH > outName = "generated/";
		outName.Append( sampleName );
		outName.Append( ".idwav" );
		
		if( samplePC->LoadWav( inName ) || samplePC->LoadWav( inName2 ) )
		{
			idFile* fileOut = fileSystem->OpenFileWrite( outName, "fs_basepath" );
			samplePC->WriteGeneratedSample( fileOut );
			delete fileOut;
		}
	}
	delete samplePC;
}

/*
========================
idSoundSample_OpenAL::LoadGeneratedSound
========================
*/
bool idSoundSample_OpenAL::LoadGeneratedSample( const idStr& filename )
{
#if 1
	idFileLocal fileIn( fileSystem->OpenFileReadMemory( filename ) );
	if( fileIn != NULL )
	{
		uint32 magic;
		fileIn->ReadBig( magic );
		fileIn->ReadBig( timestamp );
		fileIn->ReadBig( loaded );
		fileIn->ReadBig( playBegin );
		fileIn->ReadBig( playLength );
		idWaveFile::ReadWaveFormatDirect( format, fileIn );
		int num;
		fileIn->ReadBig( num );
		amplitude.Clear();
		amplitude.SetNum( num );
		fileIn->Read( amplitude.Ptr(), amplitude.Num() );
		fileIn->ReadBig( totalBufferSize );
		fileIn->ReadBig( num );
		buffers.SetNum( num );
		for( int i = 0; i < num; i++ )
		{
			fileIn->ReadBig( buffers[ i ].numSamples );
			fileIn->ReadBig( buffers[ i ].bufferSize );
			buffers[ i ].buffer = AllocBuffer( buffers[ i ].bufferSize, GetName() );
			fileIn->Read( buffers[ i ].buffer, buffers[ i ].bufferSize );
			buffers[ i ].buffer = GPU_CONVERT_CPU_TO_CPU_CACHED_READONLY_ADDRESS( buffers[ i ].buffer );
		}
		return true;
	}
#endif
	
	return false;
}
/*
========================
idSoundSample_OpenAL::Load
========================
*/
void idSoundSample_OpenAL::LoadResource()
{
	FreeData();
	
	if( idStr::Icmpn( GetName(), "_default", 8 ) == 0 )
	{
		MakeDefault();
		return;
	}
	
	if( s_noSound.GetBool() )
	{
		MakeDefault();
		return;
	}
	
	loaded = false;
	
	for( int i = 0; i < 2; i++ )
	{
		idStrStatic< MAX_OSPATH > sampleName = GetName();
		if( ( i == 0 ) && !sampleName.Replace( "/vo/", va( "/vo/%s/", sys_lang.GetString() ) ) )
		{
			i++;
		}
		idStrStatic< MAX_OSPATH > generatedName = "generated/";
		generatedName.Append( sampleName );
		
		{
			if( s_useCompression.GetBool() )
			{
				sampleName.Append( ".msadpcm" );
			}
			else
			{
				sampleName.Append( ".wav" );
			}
			generatedName.Append( ".idwav" );
		}
		loaded = LoadGeneratedSample( generatedName ) || LoadWav( sampleName );
		
		if( !loaded && s_useCompression.GetBool() )
		{
			sampleName.SetFileExtension( "wav" );
			loaded = LoadWav( sampleName );
		}
		
		if( loaded )
		{
			if( cvarSystem->GetCVarBool( "fs_buildresources" ) )
			{
				fileSystem->AddSamplePreload( GetName() );
				WriteAllSamples( GetName() );
				
				if( sampleName.Find( "/vo/" ) >= 0 )
				{
					for( int i = 0; i < Sys_NumLangs(); i++ )
					{
						const char* lang = Sys_Lang( i );
						if( idStr::Icmp( lang, ID_LANG_ENGLISH ) == 0 )
						{
							continue;
						}
						idStrStatic< MAX_OSPATH > locName = GetName();
						locName.Replace( "/vo/", va( "/vo/%s/", Sys_Lang( i ) ) );
						WriteAllSamples( locName );
					}
				}
			}
			
			// build OpenAL buffer
			alGetError();
			alGenBuffers( 1, &openalBuffer );
			
			if( alGetError() != AL_NO_ERROR )
			{
				common->Error( "idSoundSample_OpenAL::MakeDefault: error generating OpenAL hardware buffer" );
			}
			
			if( alIsBuffer( openalBuffer ) )
			{
				alGetError();
				
				// RB: TODO decode idWaveFile::FORMAT_ADPCM to idWaveFile::FORMAT_PCM
				// and build one big OpenAL buffer using the alBufferSubData extension
				
				alBufferData( openalBuffer, GetOpenALBufferFormat(), buffers[0].buffer, buffers[0].bufferSize, format.basic.samplesPerSec );
				if( alGetError() != AL_NO_ERROR )
				{
					common->Error( "idSoundSample_OpenAL::MakeDefault: error loading data into OpenAL hardware buffer" );
				}
			}
			
			return;
		}
	}
	
	if( !loaded )
	{
		// make it default if everything else fails
		MakeDefault();
	}
	return;
}

/*
========================
idSoundSample_OpenAL::LoadWav
========================
*/
bool idSoundSample_OpenAL::LoadWav( const idStr& filename )
{

	// load the wave
	idWaveFile wave;
	if( !wave.Open( filename ) )
	{
		return false;
	}
	
	idStrStatic< MAX_OSPATH > sampleName = filename;
	sampleName.SetFileExtension( "amp" );
	LoadAmplitude( sampleName );
	
	const char* formatError = wave.ReadWaveFormat( format );
	if( formatError != NULL )
	{
		idLib::Warning( "LoadWav( %s ) : %s", filename.c_str(), formatError );
		MakeDefault();
		return false;
	}
	timestamp = wave.Timestamp();
	
	totalBufferSize = wave.SeekToChunk( 'data' );
	
	if( format.basic.formatTag == idWaveFile::FORMAT_PCM || format.basic.formatTag == idWaveFile::FORMAT_EXTENSIBLE )
	{
	
		if( format.basic.bitsPerSample != 16 )
		{
			idLib::Warning( "LoadWav( %s ) : %s", filename.c_str(), "Not a 16 bit PCM wav file" );
			MakeDefault();
			return false;
		}
		
		playBegin = 0;
		playLength = ( totalBufferSize ) / format.basic.blockSize;
		
		buffers.SetNum( 1 );
		buffers[0].bufferSize = totalBufferSize;
		buffers[0].numSamples = playLength;
		buffers[0].buffer = AllocBuffer( totalBufferSize, GetName() );
		
		
		wave.Read( buffers[0].buffer, totalBufferSize );
		
		if( format.basic.bitsPerSample == 16 )
		{
			idSwap::LittleArray( ( short* )buffers[0].buffer, totalBufferSize / sizeof( short ) );
		}
		
		buffers[0].buffer = GPU_CONVERT_CPU_TO_CPU_CACHED_READONLY_ADDRESS( buffers[0].buffer );
		
	}
	else if( format.basic.formatTag == idWaveFile::FORMAT_ADPCM )
	{
	
		playBegin = 0;
		playLength = ( ( totalBufferSize / format.basic.blockSize ) * format.extra.adpcm.samplesPerBlock );
		
		buffers.SetNum( 1 );
		buffers[0].bufferSize = totalBufferSize;
		buffers[0].numSamples = playLength;
		buffers[0].buffer  = AllocBuffer( totalBufferSize, GetName() );
		
		wave.Read( buffers[0].buffer, totalBufferSize );
		
		buffers[0].buffer = GPU_CONVERT_CPU_TO_CPU_CACHED_READONLY_ADDRESS( buffers[0].buffer );
		
	}
	else if( format.basic.formatTag == idWaveFile::FORMAT_XMA2 )
	{
	
		if( format.extra.xma2.blockCount == 0 )
		{
			idLib::Warning( "LoadWav( %s ) : %s", filename.c_str(), "No data blocks in file" );
			MakeDefault();
			return false;
		}
		
		int bytesPerBlock = format.extra.xma2.bytesPerBlock;
		assert( format.extra.xma2.blockCount == ALIGN( totalBufferSize, bytesPerBlock ) / bytesPerBlock );
		assert( format.extra.xma2.blockCount * bytesPerBlock >= totalBufferSize );
		assert( format.extra.xma2.blockCount * bytesPerBlock < totalBufferSize + bytesPerBlock );
		
		buffers.SetNum( format.extra.xma2.blockCount );
		for( int i = 0; i < buffers.Num(); i++ )
		{
			if( i == buffers.Num() - 1 )
			{
				buffers[i].bufferSize = totalBufferSize - ( i * bytesPerBlock );
			}
			else
			{
				buffers[i].bufferSize = bytesPerBlock;
			}
			
			buffers[i].buffer = AllocBuffer( buffers[i].bufferSize, GetName() );
			wave.Read( buffers[i].buffer, buffers[i].bufferSize );
			buffers[i].buffer = GPU_CONVERT_CPU_TO_CPU_CACHED_READONLY_ADDRESS( buffers[i].buffer );
		}
		
		int seekTableSize = wave.SeekToChunk( 'seek' );
		if( seekTableSize != 4 * buffers.Num() )
		{
			idLib::Warning( "LoadWav( %s ) : %s", filename.c_str(), "Wrong number of entries in seek table" );
			MakeDefault();
			return false;
		}
		
		for( int i = 0; i < buffers.Num(); i++ )
		{
			wave.Read( &buffers[i].numSamples, sizeof( buffers[i].numSamples ) );
			idSwap::Big( buffers[i].numSamples );
		}
		
		playBegin = format.extra.xma2.loopBegin;
		playLength = format.extra.xma2.loopLength;
		
		if( buffers[buffers.Num() - 1].numSamples < playBegin + playLength )
		{
			// This shouldn't happen, but it's not fatal if it does
			playLength = buffers[buffers.Num() - 1].numSamples - playBegin;
		}
		else
		{
			// Discard samples beyond playLength
			for( int i = 0; i < buffers.Num(); i++ )
			{
				if( buffers[i].numSamples > playBegin + playLength )
				{
					buffers[i].numSamples = playBegin + playLength;
					// Ideally, the following loop should always have 0 iterations because playBegin + playLength ends in the last block already
					// But there is no guarantee for that, so to be safe, discard all buffers beyond this one
					for( int j = i + 1; j < buffers.Num(); j++ )
					{
						FreeBuffer( buffers[j].buffer );
					}
					buffers.SetNum( i + 1 );
					break;
				}
			}
		}
		
	}
	else
	{
		idLib::Warning( "LoadWav( %s ) : Unsupported wave format %d", filename.c_str(), format.basic.formatTag );
		MakeDefault();
		return false;
	}
	
	wave.Close();
	
	if( format.basic.formatTag == idWaveFile::FORMAT_EXTENSIBLE )
	{
		// HACK: XAudio2 doesn't really support FORMAT_EXTENSIBLE so we convert it to a basic format after extracting the channel mask
		format.basic.formatTag = format.extra.extensible.subFormat.data1;
	}
	
	// sanity check...
	assert( buffers[buffers.Num() - 1].numSamples == playBegin + playLength );
	
	return true;
}


/*
========================
idSoundSample_OpenAL::MakeDefault
========================
*/
void idSoundSample_OpenAL::MakeDefault()
{
	FreeData();
	
	static const int DEFAULT_NUM_SAMPLES = 4096;
	
	timestamp = FILE_NOT_FOUND_TIMESTAMP;
	loaded = true;
	
	memset( &format, 0, sizeof( format ) );
	format.basic.formatTag = idWaveFile::FORMAT_PCM;
	format.basic.numChannels = 1;
	format.basic.bitsPerSample = 16;
	format.basic.samplesPerSec = 22050; //44100; //XAUDIO2_MIN_SAMPLE_RATE;
	format.basic.blockSize = format.basic.numChannels * format.basic.bitsPerSample / 8;
	format.basic.avgBytesPerSec = format.basic.samplesPerSec * format.basic.blockSize;
	
	assert( format.basic.blockSize == 2 );
	
	totalBufferSize = DEFAULT_NUM_SAMPLES * 2;// * sizeof( short );
	
	short* defaultBuffer = ( short* )AllocBuffer( totalBufferSize, GetName() );
	for( int i = 0; i < DEFAULT_NUM_SAMPLES; i += 2 )
	{
		float v = sin( idMath::PI * 2 * i / 64 );
		int sample = v * 0x4000;
		defaultBuffer[i + 0] = sample;
		defaultBuffer[i + 1] = sample;
		
		//defaultBuffer[i + 0] = SHRT_MIN;
		//defaultBuffer[i + 1] = SHRT_MAX;
	}
	
	buffers.SetNum( 1 );
	buffers[0].buffer = defaultBuffer;
	buffers[0].bufferSize = totalBufferSize;
	buffers[0].numSamples = DEFAULT_NUM_SAMPLES;
	buffers[0].buffer = GPU_CONVERT_CPU_TO_CPU_CACHED_READONLY_ADDRESS( buffers[0].buffer );
	
	playBegin = 0;
	playLength = DEFAULT_NUM_SAMPLES;
	
	
	alGetError();
	alGenBuffers( 1, &openalBuffer );
	
	if( alGetError() != AL_NO_ERROR )
	{
		common->Error( "idSoundSample_OpenAL::MakeDefault: error generating OpenAL hardware buffer" );
	}
	
	if( alIsBuffer( openalBuffer ) )
	{
		alGetError();
		alBufferData( openalBuffer, GetOpenALBufferFormat(), defaultBuffer, totalBufferSize, format.basic.samplesPerSec );
		if( alGetError() != AL_NO_ERROR )
		{
			common->Error( "idSoundSample_OpenAL::MakeDefault: error loading data into OpenAL hardware buffer" );
		}
	}
}

/*
========================
idSoundSample_OpenAL::FreeData

Called before deleting the object and at the start of LoadResource()
========================
*/
void idSoundSample_OpenAL::FreeData()
{
	if( buffers.Num() > 0 )
	{
		soundSystemLocal.StopVoicesWithSample( ( idSoundSample* )this );
		for( int i = 0; i < buffers.Num(); i++ )
		{
			FreeBuffer( buffers[i].buffer );
		}
		buffers.Clear();
	}
	amplitude.Clear();
	
	timestamp = FILE_NOT_FOUND_TIMESTAMP;
	memset( &format, 0, sizeof( format ) );
	loaded = false;
	totalBufferSize = 0;
	playBegin = 0;
	playLength = 0;
	
	if( alIsBuffer( openalBuffer ) )
	{
		alGetError();
		alDeleteBuffers( 1, &openalBuffer );
		if( alGetError() != AL_NO_ERROR )
		{
			common->Error( "idSoundSample_OpenAL::FreeData: error unloading data from OpenAL hardware buffer" );
		}
		else
		{
			openalBuffer = 0;
		}
	}
}

/*
========================
idSoundSample_OpenAL::LoadAmplitude
========================
*/
bool idSoundSample_OpenAL::LoadAmplitude( const idStr& name )
{
	amplitude.Clear();
	idFileLocal f( fileSystem->OpenFileRead( name ) );
	if( f == NULL )
	{
		return false;
	}
	amplitude.SetNum( f->Length() );
	f->Read( amplitude.Ptr(), amplitude.Num() );
	return true;
}

/*
========================
idSoundSample_OpenAL::GetAmplitude
========================
*/
float idSoundSample_OpenAL::GetAmplitude( int timeMS ) const
{
	if( timeMS < 0 || timeMS > LengthInMsec() )
	{
		return 0.0f;
	}
	if( IsDefault() )
	{
		return 1.0f;
	}
	int index = timeMS * 60 / 1000;
	if( index < 0 || index >= amplitude.Num() )
	{
		return 0.0f;
	}
	return ( float )amplitude[index] / 255.0f;
}


ALenum idSoundSample_OpenAL::GetOpenALBufferFormat() const
{
	ALenum alFormat;
	
	if( format.basic.formatTag == idWaveFile::FORMAT_PCM )
	{
		alFormat = NumChannels() == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
	}
	else if( format.basic.formatTag == idWaveFile::FORMAT_ADPCM )
	{
		alFormat = NumChannels() == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
		//alFormat = NumChannels() == 1 ? AL_FORMAT_IMA_ADPCM_MONO16_EXT : AL_FORMAT_IMA_ADPCM_STEREO16_EXT;
	}
	else if( format.basic.formatTag == idWaveFile::FORMAT_XMA2 )
	{
		alFormat = NumChannels() == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
	}
	else
	{
		alFormat = NumChannels() == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
	}
	
	return alFormat;
}

