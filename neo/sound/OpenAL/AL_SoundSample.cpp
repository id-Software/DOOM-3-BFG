/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013 Robert Beckebans
Copyright (C) 1997-2012 Sam Lantinga <slouken@libsdl.org>  (MS ADPCM decoder)
Copyright (c) 2011 Chris Robinson <chris.kcat@gmail.com> (OpenAL helpers)

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

			// upload PCM data to OpenAL
			CreateOpenALBuffer();

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

void idSoundSample_OpenAL::CreateOpenALBuffer()
{
	// build OpenAL buffer
	CheckALErrors();
	alGenBuffers( 1, &openalBuffer );

	if( CheckALErrors() != AL_NO_ERROR )
	{
		common->Error( "idSoundSample_OpenAL::CreateOpenALBuffer: error generating OpenAL hardware buffer" );
	}

	if( alIsBuffer( openalBuffer ) )
	{
		CheckALErrors();

		void* buffer = NULL;
		uint32 bufferSize = 0;

		if( format.basic.formatTag == idWaveFile::FORMAT_ADPCM )
		{
			// RB: decode idWaveFile::FORMAT_ADPCM to idWaveFile::FORMAT_PCM

			buffer = buffers[0].buffer;
			bufferSize = buffers[0].bufferSize;

			if( MS_ADPCM_decode( ( uint8** ) &buffer, &bufferSize ) < 0 )
			{
				common->Error( "idSoundSample_OpenAL::CreateOpenALBuffer: could not decode ADPCM '%s' to 16 bit format", GetName() );
			}

			buffers[0].buffer = buffer;
			buffers[0].bufferSize = bufferSize;

			totalBufferSize = bufferSize;
		}
		else if( format.basic.formatTag == idWaveFile::FORMAT_XMA2 )
		{
			// RB: not used in the PC version of the BFG edition
			common->Error( "idSoundSample_OpenAL::CreateOpenALBuffer: could not decode XMA2 '%s' to 16 bit format", GetName() );
		}
		else if( format.basic.formatTag == idWaveFile::FORMAT_EXTENSIBLE )
		{
			// RB: not used in the PC version of the BFG edition
			common->Error( "idSoundSample_OpenAL::CreateOpenALBuffer: could not decode extensible WAV format '%s' to 16 bit format", GetName() );
		}
		else
		{
			// TODO concatenate buffers

			assert( buffers.Num() == 1 );

			buffer = buffers[0].buffer;
			bufferSize = buffers[0].bufferSize;
		}

#if 0 //#if defined(AL_SOFT_buffer_samples)
		if( alIsExtensionPresent( "AL_SOFT_buffer_samples" ) )
		{
			ALenum type = AL_SHORT_SOFT;

			if( format.basic.bitsPerSample != 16 )
			{
				//common->Error( "idSoundSample_OpenAL::LoadResource: '%s' not a 16 bit format", GetName() );
			}

			ALenum channels = NumChannels() == 1 ? AL_MONO_SOFT : AL_STEREO_SOFT;
			ALenum alFormat = GetOpenALSoftFormat( channels, type );

			alBufferSamplesSOFT( openalBuffer, format.basic.samplesPerSec, alFormat, BytesToFrames( bufferSize, channels, type ), channels, type, buffer );
		}
		else
#endif
		{
			alBufferData( openalBuffer, GetOpenALBufferFormat(), buffer, bufferSize, format.basic.samplesPerSec );
		}

		if( CheckALErrors() != AL_NO_ERROR )
		{
			common->Error( "idSoundSample_OpenAL::CreateOpenALBuffer: error loading data into OpenAL hardware buffer" );
		}
	}
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


	CheckALErrors();
	alGenBuffers( 1, &openalBuffer );

	if( CheckALErrors() != AL_NO_ERROR )
	{
		common->Error( "idSoundSample_OpenAL::MakeDefault: error generating OpenAL hardware buffer" );
	}

	if( alIsBuffer( openalBuffer ) )
	{
		CheckALErrors();
		alBufferData( openalBuffer, GetOpenALBufferFormat(), defaultBuffer, totalBufferSize, format.basic.samplesPerSec );
		if( CheckALErrors() != AL_NO_ERROR )
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
		CheckALErrors();

		alDeleteBuffers( 1, &openalBuffer );
		if( CheckALErrors() != AL_NO_ERROR )
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


#if 0 //defined(AL_SOFT_buffer_samples)
const char* idSoundSample_OpenAL::OpenALSoftChannelsName( ALenum chans ) const
{
	switch( chans )
	{
		case AL_MONO_SOFT:
			return "Mono";
		case AL_STEREO_SOFT:
			return "Stereo";
		case AL_REAR_SOFT:
			return "Rear";
		case AL_QUAD_SOFT:
			return "Quadraphonic";
		case AL_5POINT1_SOFT:
			return "5.1 Surround";
		case AL_6POINT1_SOFT:
			return "6.1 Surround";
		case AL_7POINT1_SOFT:
			return "7.1 Surround";
	}

	return "Unknown Channels";
}

const char* idSoundSample_OpenAL::OpenALSoftTypeName( ALenum type ) const
{
	switch( type )
	{
		case AL_BYTE_SOFT:
			return "S8";
		case AL_UNSIGNED_BYTE_SOFT:
			return "U8";
		case AL_SHORT_SOFT:
			return "S16";
		case AL_UNSIGNED_SHORT_SOFT:
			return "U16";
		case AL_INT_SOFT:
			return "S32";
		case AL_UNSIGNED_INT_SOFT:
			return "U32";
		case AL_FLOAT_SOFT:
			return "Float32";
		case AL_DOUBLE_SOFT:
			return "Float64";
	}

	return "Unknown Type";
}

ALsizei idSoundSample_OpenAL::FramesToBytes( ALsizei size, ALenum channels, ALenum type ) const
{
	switch( channels )
	{
		case AL_MONO_SOFT:
			size *= 1;
			break;
		case AL_STEREO_SOFT:
			size *= 2;
			break;
		case AL_REAR_SOFT:
			size *= 2;
			break;
		case AL_QUAD_SOFT:
			size *= 4;
			break;
		case AL_5POINT1_SOFT:
			size *= 6;
			break;
		case AL_6POINT1_SOFT:
			size *= 7;
			break;
		case AL_7POINT1_SOFT:
			size *= 8;
			break;
	}

	switch( type )
	{
		case AL_BYTE_SOFT:
			size *= sizeof( ALbyte );
			break;
		case AL_UNSIGNED_BYTE_SOFT:
			size *= sizeof( ALubyte );
			break;
		case AL_SHORT_SOFT:
			size *= sizeof( ALshort );
			break;
		case AL_UNSIGNED_SHORT_SOFT:
			size *= sizeof( ALushort );
			break;
		case AL_INT_SOFT:
			size *= sizeof( ALint );
			break;
		case AL_UNSIGNED_INT_SOFT:
			size *= sizeof( ALuint );
			break;
		case AL_FLOAT_SOFT:
			size *= sizeof( ALfloat );
			break;
		case AL_DOUBLE_SOFT:
			size *= sizeof( ALdouble );
			break;
	}

	return size;
}

ALsizei idSoundSample_OpenAL::BytesToFrames( ALsizei size, ALenum channels, ALenum type ) const
{
	return size / FramesToBytes( 1, channels, type );
}

ALenum idSoundSample_OpenAL::GetOpenALSoftFormat( ALenum channels, ALenum type ) const
{
	ALenum format = AL_NONE;

	/* If using AL_SOFT_buffer_samples, try looking through its formats */
	if( alIsExtensionPresent( "AL_SOFT_buffer_samples" ) )
	{
		/* AL_SOFT_buffer_samples is more lenient with matching formats. The
		 * specified sample type does not need to match the returned format,
		 * but it is nice to try to get something close. */
		if( type == AL_UNSIGNED_BYTE_SOFT || type == AL_BYTE_SOFT )
		{
			if( channels == AL_MONO_SOFT )
			{
				format = AL_MONO8_SOFT;
			}
			else if( channels == AL_STEREO_SOFT )
			{
				format = AL_STEREO8_SOFT;
			}
			else if( channels == AL_QUAD_SOFT )
			{
				format = AL_QUAD8_SOFT;
			}
			else if( channels == AL_5POINT1_SOFT )
			{
				format = AL_5POINT1_8_SOFT;
			}
			else if( channels == AL_6POINT1_SOFT )
			{
				format = AL_6POINT1_8_SOFT;
			}
			else if( channels == AL_7POINT1_SOFT )
			{
				format = AL_7POINT1_8_SOFT;
			}
		}
		else if( type == AL_UNSIGNED_SHORT_SOFT || type == AL_SHORT_SOFT )
		{
			if( channels == AL_MONO_SOFT )
			{
				format = AL_MONO16_SOFT;
			}
			else if( channels == AL_STEREO_SOFT )
			{
				format = AL_STEREO16_SOFT;
			}
			else if( channels == AL_QUAD_SOFT )
			{
				format = AL_QUAD16_SOFT;
			}
			else if( channels == AL_5POINT1_SOFT )
			{
				format = AL_5POINT1_16_SOFT;
			}
			else if( channels == AL_6POINT1_SOFT )
			{
				format = AL_6POINT1_16_SOFT;
			}
			else if( channels == AL_7POINT1_SOFT )
			{
				format = AL_7POINT1_16_SOFT;
			}
		}
		else if( type == AL_UNSIGNED_BYTE3_SOFT || type == AL_BYTE3_SOFT ||
				 type == AL_UNSIGNED_INT_SOFT || type == AL_INT_SOFT ||
				 type == AL_FLOAT_SOFT || type == AL_DOUBLE_SOFT )
		{
			if( channels == AL_MONO_SOFT )
			{
				format = AL_MONO32F_SOFT;
			}
			else if( channels == AL_STEREO_SOFT )
			{
				format = AL_STEREO32F_SOFT;
			}
			else if( channels == AL_QUAD_SOFT )
			{
				format = AL_QUAD32F_SOFT;
			}
			else if( channels == AL_5POINT1_SOFT )
			{
				format = AL_5POINT1_32F_SOFT;
			}
			else if( channels == AL_6POINT1_SOFT )
			{
				format = AL_6POINT1_32F_SOFT;
			}
			else if( channels == AL_7POINT1_SOFT )
			{
				format = AL_7POINT1_32F_SOFT;
			}
		}

		if( format != AL_NONE && !alIsBufferFormatSupportedSOFT( format ) )
		{
			format = AL_NONE;
		}

		/* A matching format was not found or supported. Try 32-bit float. */
		if( format == AL_NONE )
		{
			if( channels == AL_MONO_SOFT )
			{
				format = AL_MONO32F_SOFT;
			}
			else if( channels == AL_STEREO_SOFT )
			{
				format = AL_STEREO32F_SOFT;
			}
			else if( channels == AL_QUAD_SOFT )
			{
				format = AL_QUAD32F_SOFT;
			}
			else if( channels == AL_5POINT1_SOFT )
			{
				format = AL_5POINT1_32F_SOFT;
			}
			else if( channels == AL_6POINT1_SOFT )
			{
				format = AL_6POINT1_32F_SOFT;
			}
			else if( channels == AL_7POINT1_SOFT )
			{
				format = AL_7POINT1_32F_SOFT;
			}

			if( format != AL_NONE && !alIsBufferFormatSupportedSOFT( format ) )
			{
				format = AL_NONE;
			}
		}
		/* 32-bit float not supported. Try 16-bit int. */
		if( format == AL_NONE )
		{
			if( channels == AL_MONO_SOFT )
			{
				format = AL_MONO16_SOFT;
			}
			else if( channels == AL_STEREO_SOFT )
			{
				format = AL_STEREO16_SOFT;
			}
			else if( channels == AL_QUAD_SOFT )
			{
				format = AL_QUAD16_SOFT;
			}
			else if( channels == AL_5POINT1_SOFT )
			{
				format = AL_5POINT1_16_SOFT;
			}
			else if( channels == AL_6POINT1_SOFT )
			{
				format = AL_6POINT1_16_SOFT;
			}
			else if( channels == AL_7POINT1_SOFT )
			{
				format = AL_7POINT1_16_SOFT;
			}

			if( format != AL_NONE && !alIsBufferFormatSupportedSOFT( format ) )
			{
				format = AL_NONE;
			}
		}
		/* 16-bit int not supported. Try 8-bit int. */
		if( format == AL_NONE )
		{
			if( channels == AL_MONO_SOFT )
			{
				format = AL_MONO8_SOFT;
			}
			else if( channels == AL_STEREO_SOFT )
			{
				format = AL_STEREO8_SOFT;
			}
			else if( channels == AL_QUAD_SOFT )
			{
				format = AL_QUAD8_SOFT;
			}
			else if( channels == AL_5POINT1_SOFT )
			{
				format = AL_5POINT1_8_SOFT;
			}
			else if( channels == AL_6POINT1_SOFT )
			{
				format = AL_6POINT1_8_SOFT;
			}
			else if( channels == AL_7POINT1_SOFT )
			{
				format = AL_7POINT1_8_SOFT;
			}

			if( format != AL_NONE && !alIsBufferFormatSupportedSOFT( format ) )
			{
				format = AL_NONE;
			}
		}

		return format;
	}

	/* We use the AL_EXT_MCFORMATS extension to provide output of Quad, 5.1,
	 * and 7.1 channel configs, AL_EXT_FLOAT32 for 32-bit float samples, and
	 * AL_EXT_DOUBLE for 64-bit float samples. */
	if( type == AL_UNSIGNED_BYTE_SOFT )
	{
		if( channels == AL_MONO_SOFT )
		{
			format = AL_FORMAT_MONO8;
		}
		else if( channels == AL_STEREO_SOFT )
		{
			format = AL_FORMAT_STEREO8;
		}
		else if( alIsExtensionPresent( "AL_EXT_MCFORMATS" ) )
		{
			if( channels == AL_QUAD_SOFT )
			{
				format = alGetEnumValue( "AL_FORMAT_QUAD8" );
			}
			else if( channels == AL_5POINT1_SOFT )
			{
				format = alGetEnumValue( "AL_FORMAT_51CHN8" );
			}
			else if( channels == AL_6POINT1_SOFT )
			{
				format = alGetEnumValue( "AL_FORMAT_61CHN8" );
			}
			else if( channels == AL_7POINT1_SOFT )
			{
				format = alGetEnumValue( "AL_FORMAT_71CHN8" );
			}
		}
	}
	else if( type == AL_SHORT_SOFT )
	{
		if( channels == AL_MONO_SOFT )
		{
			format = AL_FORMAT_MONO16;
		}
		else if( channels == AL_STEREO_SOFT )
		{
			format = AL_FORMAT_STEREO16;
		}
		else if( alIsExtensionPresent( "AL_EXT_MCFORMATS" ) )
		{
			if( channels == AL_QUAD_SOFT )
			{
				format = alGetEnumValue( "AL_FORMAT_QUAD16" );
			}
			else if( channels == AL_5POINT1_SOFT )
			{
				format = alGetEnumValue( "AL_FORMAT_51CHN16" );
			}
			else if( channels == AL_6POINT1_SOFT )
			{
				format = alGetEnumValue( "AL_FORMAT_61CHN16" );
			}
			else if( channels == AL_7POINT1_SOFT )
			{
				format = alGetEnumValue( "AL_FORMAT_71CHN16" );
			}
		}
	}
	else if( type == AL_FLOAT_SOFT && alIsExtensionPresent( "AL_EXT_FLOAT32" ) )
	{
		if( channels == AL_MONO_SOFT )
		{
			format = alGetEnumValue( "AL_FORMAT_MONO_FLOAT32" );
		}
		else if( channels == AL_STEREO_SOFT )
		{
			format = alGetEnumValue( "AL_FORMAT_STEREO_FLOAT32" );
		}
		else if( alIsExtensionPresent( "AL_EXT_MCFORMATS" ) )
		{
			if( channels == AL_QUAD_SOFT )
			{
				format = alGetEnumValue( "AL_FORMAT_QUAD32" );
			}
			else if( channels == AL_5POINT1_SOFT )
			{
				format = alGetEnumValue( "AL_FORMAT_51CHN32" );
			}
			else if( channels == AL_6POINT1_SOFT )
			{
				format = alGetEnumValue( "AL_FORMAT_61CHN32" );
			}
			else if( channels == AL_7POINT1_SOFT )
			{
				format = alGetEnumValue( "AL_FORMAT_71CHN32" );
			}
		}
	}
	else if( type == AL_DOUBLE_SOFT && alIsExtensionPresent( "AL_EXT_DOUBLE" ) )
	{
		if( channels == AL_MONO_SOFT )
		{
			format = alGetEnumValue( "AL_FORMAT_MONO_DOUBLE" );
		}
		else if( channels == AL_STEREO_SOFT )
		{
			format = alGetEnumValue( "AL_FORMAT_STEREO_DOUBLE" );
		}
	}

	/* NOTE: It seems OSX returns -1 from alGetEnumValue for unknown enums, as
	 * opposed to 0. Correct it. */
	if( format == -1 )
	{
		format = 0;
	}

	return format;
}
#endif // #if defined(AL_SOFT_buffer_samples)

ALenum idSoundSample_OpenAL::GetOpenALBufferFormat() const
{
	ALenum alFormat;

	if( format.basic.formatTag == idWaveFile::FORMAT_PCM )
	{
		alFormat = NumChannels() == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
	}
	else if( format.basic.formatTag == idWaveFile::FORMAT_ADPCM )
	{
		//alFormat = NumChannels() == 1 ? AL_FORMAT_MONO8 : AL_FORMAT_STEREO8;
		alFormat = NumChannels() == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
		//alFormat = NumChannels() == 1 ? AL_FORMAT_MONO_IMA4 : AL_FORMAT_STEREO_IMA4;
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

int32 idSoundSample_OpenAL::MS_ADPCM_nibble( MS_ADPCM_decodeState_t* state, int8 nybble )
{
	const int32 max_audioval = ( ( 1 << ( 16 - 1 ) ) - 1 );
	const int32 min_audioval = -( 1 << ( 16 - 1 ) );
	const int32 adaptive[] =
	{
		230, 230, 230, 230, 307, 409, 512, 614,
		768, 614, 512, 409, 307, 230, 230, 230
	};

	int32 new_sample, delta;

	new_sample = ( ( state->iSamp1 * state->coef1 ) +
				   ( state->iSamp2 * state->coef2 ) ) / 256;

	if( nybble & 0x08 )
	{
		new_sample += state->iDelta * ( nybble - 0x10 );
	}
	else
	{
		new_sample += state->iDelta * nybble;
	}

	if( new_sample < min_audioval )
	{
		new_sample = min_audioval;
	}
	else if( new_sample > max_audioval )
	{
		new_sample = max_audioval;
	}

	delta = ( ( int32 ) state->iDelta * adaptive[nybble] ) / 256;
	if( delta < 16 )
	{
		delta = 16;
	}

	state->iDelta = ( uint16 ) delta;
	state->iSamp2 = state->iSamp1;
	state->iSamp1 = ( int16 ) new_sample;

	return ( new_sample );
}

int idSoundSample_OpenAL::MS_ADPCM_decode( uint8** audio_buf, uint32* audio_len )
{
	static MS_ADPCM_decodeState_t	states[2];
	MS_ADPCM_decodeState_t*			state[2];

	uint8* freeable, *encoded, *decoded;
	int32 encoded_len, samplesleft;
	int8 nybble;
	int8 stereo;
	int32 new_sample;

	// Allocate the proper sized output buffer
	encoded_len = *audio_len;
	encoded = *audio_buf;
	freeable = *audio_buf;

	*audio_len = ( encoded_len / format.basic.blockSize ) * format.extra.adpcm.samplesPerBlock * format.basic.numChannels * sizeof( int16 );

	*audio_buf = ( uint8* ) Mem_Alloc( *audio_len, TAG_AUDIO );
	if( *audio_buf == NULL )
	{
		//SDL_Error( SDL_ENOMEM );
		return ( -1 );
	}
	decoded = *audio_buf;

	assert( format.basic.numChannels == 1 || format.basic.numChannels == 2 );

	// Get ready... Go!
	stereo = ( format.basic.numChannels == 2 ) ? 1 : 0;
	state[0] = &states[0];
	state[1] = &states[stereo];

	while( encoded_len >= format.basic.blockSize )
	{
		// Grab the initial information for this block
		state[0]->hPredictor = *encoded++;

		assert( state[0]->hPredictor < format.extra.adpcm.numCoef );
		state[0]->hPredictor = idMath::ClampInt( 0, 6, state[0]->hPredictor );

		state[0]->coef1 = format.extra.adpcm.aCoef[state[0]->hPredictor].coef1;
		state[0]->coef2 = format.extra.adpcm.aCoef[state[0]->hPredictor].coef2;

		if( stereo )
		{
			state[1]->hPredictor = *encoded++;

			assert( state[1]->hPredictor < format.extra.adpcm.numCoef );
			state[1]->hPredictor = idMath::ClampInt( 0, 6, state[1]->hPredictor );

			state[1]->coef1 = format.extra.adpcm.aCoef[state[1]->hPredictor].coef1;
			state[1]->coef2 = format.extra.adpcm.aCoef[state[1]->hPredictor].coef2;
		}

		state[0]->iDelta = ( ( encoded[1] << 8 ) | encoded[0] );
		encoded += sizeof( int16 );
		if( stereo )
		{
			state[1]->iDelta = ( ( encoded[1] << 8 ) | encoded[0] );
			encoded += sizeof( int16 );
		}

		state[0]->iSamp1 = ( ( encoded[1] << 8 ) | encoded[0] );
		encoded += sizeof( int16 );
		if( stereo )
		{
			state[1]->iSamp1 = ( ( encoded[1] << 8 ) | encoded[0] );
			encoded += sizeof( int16 );
		}

		state[0]->iSamp2 = ( ( encoded[1] << 8 ) | encoded[0] );
		encoded += sizeof( int16 );
		if( stereo )
		{
			state[1]->iSamp2 = ( ( encoded[1] << 8 ) | encoded[0] );
			encoded += sizeof( int16 );
		}



		// Store the two initial samples we start with
		decoded[0] = state[0]->iSamp2 & 0xFF;
		decoded[1] = ( state[0]->iSamp2 >> 8 ) & 0xFF;
		decoded += 2;
		if( stereo )
		{
			decoded[0] = state[1]->iSamp2 & 0xFF;
			decoded[1] = ( state[1]->iSamp2 >> 8 ) & 0xFF;
			decoded += 2;
		}

		decoded[0] = state[0]->iSamp1 & 0xFF;
		decoded[1] = ( state[0]->iSamp1 >> 8 ) & 0xFF;
		decoded += 2;
		if( stereo )
		{
			decoded[0] = state[1]->iSamp1 & 0xFF;
			decoded[1] = ( state[1]->iSamp1 >> 8 ) & 0xFF;
			decoded += 2;
		}

		// Decode and store the other samples in this block
		samplesleft = ( format.extra.adpcm.samplesPerBlock - 2 ) * format.basic.numChannels;

		while( samplesleft > 0 )
		{
			nybble = ( *encoded ) >> 4;
			new_sample = MS_ADPCM_nibble( state[0], nybble );

			decoded[0] = new_sample & 0xFF;
			decoded[1] = ( new_sample >> 8 ) & 0xFF;
			decoded += 2;

			nybble = ( *encoded ) & 0x0F;
			new_sample = MS_ADPCM_nibble( state[1], nybble );

			decoded[0] = new_sample & 0xFF;
			decoded[1] = ( new_sample >> 8 ) & 0xFF;
			decoded += 2;

			++encoded;
			samplesleft -= 2;
		}

		encoded_len -= format.basic.blockSize;
	}

	Mem_Free( freeable );

	return 0;
}

