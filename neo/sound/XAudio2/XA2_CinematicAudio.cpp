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
#include "XA2_CinematicAudio.h"
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

CinematicAudio_XAudio2::CinematicAudio_XAudio2():
	pMusicSourceVoice1( NULL )
{
}

// SRS - Implement the voice callback interface to determine when audio buffers can be freed
class VoiceCallback : public IXAudio2VoiceCallback
{
public:
	// SRS - We must free the audio buffer once it has finished playing
	void OnBufferEnd( void* data )
	{
#if defined(USE_FFMPEG)
		av_freep( &data );
#elif defined(USE_BINKDEC)
		Mem_Free( data );
#endif
	}
	//Unused methods are stubs
	void OnBufferStart( void* pBufferContext ) { }
	void OnLoopEnd( void* pBufferContext ) { }
	void OnStreamEnd( ) { }
	void OnVoiceError( void* pBufferContext, HRESULT Error ) { }
	void OnVoiceProcessingPassEnd( ) { }
	void OnVoiceProcessingPassStart( UINT32 BytesRequired ) { }
};

VoiceCallback voiceCallback;
// SRS end

void CinematicAudio_XAudio2::InitAudio( void* audioContext )
{
#if defined(USE_FFMPEG)
	AVCodecContext* dec_ctx2 = ( AVCodecContext* )audioContext;
	int format_byte = 0;
	bool use_ext = false;

	switch( dec_ctx2->sample_fmt )
	{
		case AV_SAMPLE_FMT_U8:
		case AV_SAMPLE_FMT_U8P:
		{
			format_byte = 1;
			break;
		}
		case AV_SAMPLE_FMT_S16:
		case AV_SAMPLE_FMT_S16P:
		{
			format_byte = 2;
			break;
		}
		case AV_SAMPLE_FMT_S32:
		case AV_SAMPLE_FMT_S32P:
		{
			format_byte = 4;
			break;
		}
		case AV_SAMPLE_FMT_FLT:
		case AV_SAMPLE_FMT_FLTP:
		{
			format_byte = 4;
			use_ext = true;
			break;
		}
		default:
		{
			common->Warning( "Unknown or incompatible cinematic audio format for XAudio2, sample_fmt = %d\n", dec_ctx2->sample_fmt );
			return;
		}
	}
	voiceFormatcine.nChannels = dec_ctx2->channels; //fixed
	voiceFormatcine.nSamplesPerSec = dec_ctx2->sample_rate; //fixed
#elif defined(USE_BINKDEC)
	AudioInfo* binkInfo = ( AudioInfo* )audioContext;
	int format_byte = 2;
	bool use_ext = false;
	voiceFormatcine.nChannels = binkInfo->nChannels; //fixed
	voiceFormatcine.nSamplesPerSec = binkInfo->sampleRate; //fixed
#endif

	WAVEFORMATEXTENSIBLE exvoice = { 0 };
	voiceFormatcine.wFormatTag = WAVE_FORMAT_EXTENSIBLE; //Use extensible wave format in order to handle properly the audio
	voiceFormatcine.wBitsPerSample = format_byte * 8; //fixed
	voiceFormatcine.nBlockAlign = format_byte * voiceFormatcine.nChannels; //fixed
	voiceFormatcine.nAvgBytesPerSec = voiceFormatcine.nSamplesPerSec * voiceFormatcine.nBlockAlign; //fixed
	voiceFormatcine.cbSize = 22; //fixed
	exvoice.Format = voiceFormatcine;
	switch( voiceFormatcine.nChannels )
	{
		case 1:
			exvoice.dwChannelMask = SPEAKER_MONO;
			break;
		case 2:
			exvoice.dwChannelMask = SPEAKER_STEREO;
			break;
		case 4:
			exvoice.dwChannelMask = SPEAKER_QUAD;
			break;
		case 5:
			exvoice.dwChannelMask = SPEAKER_5POINT1_SURROUND;
			break;
		case 7:
			exvoice.dwChannelMask = SPEAKER_7POINT1_SURROUND;
			break;
		default:
			exvoice.dwChannelMask = SPEAKER_MONO;
			break;
	}
	exvoice.Samples.wReserved = 0;
	exvoice.Samples.wValidBitsPerSample = voiceFormatcine.wBitsPerSample;
	exvoice.Samples.wSamplesPerBlock = voiceFormatcine.wBitsPerSample;
	exvoice.SubFormat = use_ext ? KSDATAFORMAT_SUBTYPE_IEEE_FLOAT : KSDATAFORMAT_SUBTYPE_PCM;
	// Use the XAudio2 that the game has initialized instead of making our own
	// SRS - Hook up the voice callback interface to get notice when audio buffers can be freed
	( ( IXAudio2* )soundSystemLocal.GetIXAudio2() )->CreateSourceVoice( &pMusicSourceVoice1, ( WAVEFORMATEX* )&exvoice, XAUDIO2_VOICE_USEFILTER, XAUDIO2_DEFAULT_FREQ_RATIO, &voiceCallback );
}

void CinematicAudio_XAudio2::PlayAudio( uint8_t* data, int size )
{
	//Store the data to XAudio2 buffer
	Packet.Flags = XAUDIO2_END_OF_STREAM;
	Packet.AudioBytes = size;
	Packet.pAudioData = ( BYTE* )data;
	Packet.PlayBegin = 0;
	Packet.PlayLength = 0;
	Packet.LoopBegin = 0;
	Packet.LoopLength = 0;
	Packet.LoopCount = 0;
	Packet.pContext = ( BYTE* )data;	// SRS - Pass the audio buffer pointer to the voice callback methods so it can be freed when buffer playback is finished
	HRESULT hr;
	if( FAILED( hr = pMusicSourceVoice1->SubmitSourceBuffer( &Packet ) ) )
	{
		int fail = 1;
	}

	// Play the source voice
	if( FAILED( hr = pMusicSourceVoice1->Start( 0 ) ) )
	{
		int fail = 1;
	}
}

void CinematicAudio_XAudio2::ResetAudio()
{
	if( pMusicSourceVoice1 )
	{
		pMusicSourceVoice1->Stop();
		pMusicSourceVoice1->FlushSourceBuffers();
	}
}

void CinematicAudio_XAudio2::ShutdownAudio()
{
	if( pMusicSourceVoice1 )
	{
		pMusicSourceVoice1->Stop();
		pMusicSourceVoice1->FlushSourceBuffers();
		pMusicSourceVoice1->DestroyVoice();
		pMusicSourceVoice1 = NULL;
	}
}
