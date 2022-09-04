/**
* Copyright (C) 2021 George Kalmpokis
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* this software and associated documentation files (the "Software"), to deal in
* the Software without restriction, including without limitation the rights to
* use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
* of the Software, and to permit persons to whom the Software is furnished to
* do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software. As clarification, there
* is no requirement that the copyright notice and permission be included in
* binary distributions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/
#include "precompiled.h"
#include "AL_CinematicAudio.h"
#include <sound/snd_local.h>

#if defined(USE_FFMPEG)
extern "C"
{
#define __STDC_CONSTANT_MACROS
#include <libavcodec/avcodec.h>
}
#endif

#if defined(USE_BINKDEC)
	#include <BinkDecoder.h>
#endif

extern idCVar s_noSound;
extern idCVar s_volume_dB;

CinematicAudio_OpenAL::CinematicAudio_OpenAL():
	av_rate_cin( 0 ),
	av_sample_cin( 0 ),
	offset( 0 ),
	trigger( false )
{
	alGenSources( 1, &alMusicSourceVoicecin );

	alSource3i( alMusicSourceVoicecin, AL_POSITION, 0, 0, 0 );
	alSourcei( alMusicSourceVoicecin, AL_SOURCE_RELATIVE, AL_TRUE );
	alSourcei( alMusicSourceVoicecin, AL_ROLLOFF_FACTOR, 0 );
	alListenerf( AL_GAIN, s_noSound.GetBool() ? 0.0f : DBtoLinear( s_volume_dB.GetFloat() ) ); //GK: Set the sound volume the same that is used in DOOM 3
	alGenBuffers( NUM_BUFFERS, &alMusicBuffercin[0] );
}

void CinematicAudio_OpenAL::InitAudio( void* audioContext )
{
#if defined(USE_FFMPEG)
	AVCodecContext* dec_ctx2 = ( AVCodecContext* )audioContext;
	av_rate_cin = dec_ctx2->sample_rate;

	switch( dec_ctx2->sample_fmt )
	{
		case AV_SAMPLE_FMT_U8:
		case AV_SAMPLE_FMT_U8P:
		{
			av_sample_cin = dec_ctx2->channels == 2 ? AL_FORMAT_STEREO8 : AL_FORMAT_MONO8;
			break;
		}
		case AV_SAMPLE_FMT_S16:
		case AV_SAMPLE_FMT_S16P:
		{
			av_sample_cin = dec_ctx2->channels == 2 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;
			break;
		}
		case AV_SAMPLE_FMT_FLT:
		case AV_SAMPLE_FMT_FLTP:
		{
			av_sample_cin = dec_ctx2->channels == 2 ? AL_FORMAT_STEREO_FLOAT32 : AL_FORMAT_MONO_FLOAT32;
			break;
		}
		default:
		{
			common->Warning( "Unknown or incompatible cinematic audio format for OpenAL, sample_fmt = %d\n", dec_ctx2->sample_fmt );
			return;
		}
	}
#elif defined(USE_BINKDEC)
	AudioInfo* binkInfo = ( AudioInfo* )audioContext;
	av_rate_cin = binkInfo->sampleRate;
	av_sample_cin = binkInfo->nChannels == 2 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;
#endif

	alSourceRewind( alMusicSourceVoicecin );
	alSourcei( alMusicSourceVoicecin, AL_BUFFER, 0 );
	offset = 0;
	trigger = false;
}

void CinematicAudio_OpenAL::PlayAudio( uint8_t* data, int size )
{
	ALint processed, state;

	alGetSourcei( alMusicSourceVoicecin, AL_SOURCE_STATE, &state );
	alGetSourcei( alMusicSourceVoicecin, AL_BUFFERS_PROCESSED, &processed );

	if( trigger )
	{
		tBuffer.push( data );
		sizes.push( size );
		while( processed > 0 )
		{
			ALuint bufid;

			// SRS - Only unqueue an alBuffer if we don't already have a free bufid to use
			if( bufids.size() == 0 )
			{
				alSourceUnqueueBuffers( alMusicSourceVoicecin, 1, &bufid );
				bufids.push( bufid );
				processed--;
			}
			if( !tBuffer.empty() )
			{
				uint8_t* tempdata = tBuffer.front();
				tBuffer.pop();
				int tempSize = sizes.front();
				sizes.pop();
				if( tempSize > 0 )
				{
					bufid = bufids.front();
					bufids.pop();
					alBufferData( bufid, av_sample_cin, tempdata, tempSize, av_rate_cin );
					// SRS - We must free the audio buffer once it has been copied into an alBuffer
#if defined(USE_FFMPEG)
					av_freep( &tempdata );
#elif defined(USE_BINKDEC)
					Mem_Free( tempdata );
#endif
					alSourceQueueBuffers( alMusicSourceVoicecin, 1, &bufid );
					ALenum error = alGetError();
					if( error != AL_NO_ERROR )
					{
						common->Warning( "OpenAL Cinematic: %s\n", alGetString( error ) );
						return;
					}
				}
			}
			else	// SRS - When no new audio frames left to queue, break and continue playing
			{
				break;
			}
		}
	}
	else
	{
		alBufferData( alMusicBuffercin[offset], av_sample_cin, data, size, av_rate_cin );
		// SRS - We must free the audio buffer once it has been copied into an alBuffer
#if defined(USE_FFMPEG)
		av_freep( &data );
#elif defined(USE_BINKDEC)
		Mem_Free( data );
#endif
		offset++;
		if( offset == NUM_BUFFERS )
		{
			alSourceQueueBuffers( alMusicSourceVoicecin, offset, alMusicBuffercin );
			ALenum error = alGetError();
			if( error != AL_NO_ERROR )
			{
				common->Warning( "OpenAL Cinematic: %s\n", alGetString( error ) );
				return;
			}
			trigger = true;
		}
	}

	if( trigger )
	{
		if( state != AL_PLAYING )
		{
			ALint queued;
			alGetSourcei( alMusicSourceVoicecin, AL_BUFFERS_QUEUED, &queued );
			if( queued == 0 )
			{
				return;
			}
			alSourcePlay( alMusicSourceVoicecin );
			ALenum error = alGetError();
			if( error != AL_NO_ERROR )
			{
				common->Warning( "OpenAL Cinematic: %s\n", alGetString( error ) );
				return;
			}
		}
	}
}

void CinematicAudio_OpenAL::ResetAudio()
{
	if( alIsSource( alMusicSourceVoicecin ) )
	{
		alSourceRewind( alMusicSourceVoicecin );
		alSourcei( alMusicSourceVoicecin, AL_BUFFER, 0 );
	}

	while( !tBuffer.empty() )
	{
		uint8_t* tempdata = tBuffer.front();
		tBuffer.pop();
		sizes.pop();
		if( tempdata )
		{
			// SRS - We must free any audio buffers that have not been copied into an alBuffer
#if defined(USE_FFMPEG)
			av_freep( &tempdata );
#elif defined(USE_BINKDEC)
			Mem_Free( tempdata );
#endif
		}
	}

	while( !bufids.empty() )
	{
		bufids.pop();
	}

	offset = 0;
	trigger = false;
}

void CinematicAudio_OpenAL::ShutdownAudio()
{
	if( alIsSource( alMusicSourceVoicecin ) )
	{
		alSourceStop( alMusicSourceVoicecin );
		alSourcei( alMusicSourceVoicecin, AL_BUFFER, 0 );
		alDeleteSources( 1, &alMusicSourceVoicecin );
		if( CheckALErrors() == AL_NO_ERROR )
		{
			alMusicSourceVoicecin = 0;
		}
	}

	alDeleteBuffers( NUM_BUFFERS, &alMusicBuffercin[0] );
	if( CheckALErrors() == AL_NO_ERROR )
	{
		for( int i = 0; i < NUM_BUFFERS; i++ )
		{
			alMusicBuffercin[ i ] = 0;
		}
	}

	while( !tBuffer.empty() )
	{
		uint8_t* tempdata = tBuffer.front();
		tBuffer.pop();
		sizes.pop();
		if( tempdata )
		{
			// SRS - We must free any audio buffers that have not been copied into an alBuffer
#if defined(USE_FFMPEG)
			av_freep( &tempdata );
#elif defined(USE_BINKDEC)
			Mem_Free( tempdata );
#endif
		}
	}

	while( !bufids.empty() )
	{
		bufids.pop();
	}
}
