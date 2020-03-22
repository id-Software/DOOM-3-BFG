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
#ifndef __AL_SOUNDSAMPLE_H__
#define __AL_SOUNDSAMPLE_H__

/*
================================================
idSoundSample_OpenAL
================================================
*/
class idSampleInfo;
class idSoundSample_OpenAL
{
public:
	idSoundSample_OpenAL();

	// Loads and initializes the resource based on the name.
	virtual void	 LoadResource();

	void			SetName( const char* n )
	{
		name = n;
	}
	const char* 	GetName() const
	{
		return name;
	}
	ID_TIME_T		GetTimestamp() const
	{
		return timestamp;
	}

	// turns it into a beep
	void			MakeDefault();

	// frees all data
	void			FreeData();

	int				LengthInMsec() const
	{
		return SamplesToMsec( NumSamples(), SampleRate() );
	}
	int				SampleRate() const
	{
		return format.basic.samplesPerSec;
	}
	int				NumSamples() const
	{
		return playLength;
	}
	int				NumChannels() const
	{
		return format.basic.numChannels;
	}
	int				BufferSize() const
	{
		return totalBufferSize;
	}

	bool			IsCompressed() const
	{
		return ( format.basic.formatTag != idWaveFile::FORMAT_PCM );
	}

	bool			IsDefault() const
	{
		return timestamp == FILE_NOT_FOUND_TIMESTAMP;
	}
	bool			IsLoaded() const
	{
		return loaded;
	}

	void			SetNeverPurge()
	{
		neverPurge = true;
	}
	bool			GetNeverPurge() const
	{
		return neverPurge;
	}

	void			SetLevelLoadReferenced()
	{
		levelLoadReferenced = true;
	}
	void			ResetLevelLoadReferenced()
	{
		levelLoadReferenced = false;
	}
	bool			GetLevelLoadReferenced() const
	{
		return levelLoadReferenced;
	}

	int				GetLastPlayedTime() const
	{
		return lastPlayedTime;
	}
	void			SetLastPlayedTime( int t )
	{
		lastPlayedTime = t;
	}

	float			GetAmplitude( int timeMS ) const;

#if 0 //defined(AL_SOFT_buffer_samples)
	const char*		OpenALSoftChannelsName( ALenum chans ) const;

	const char*		OpenALSoftTypeName( ALenum type ) const;

	ALsizei			FramesToBytes( ALsizei size, ALenum channels, ALenum type ) const;
	ALsizei			BytesToFrames( ALsizei size, ALenum channels, ALenum type ) const;

	/* Retrieves a compatible buffer format given the channel configuration and
	 * sample type. If an alIsBufferFormatSupportedSOFT-compatible function is
	 * provided, it will be called to find the closest-matching format from
	 * AL_SOFT_buffer_samples. Returns AL_NONE (0) if no supported format can be
	 * found. */
	ALenum			GetOpenALSoftFormat( ALenum channels, ALenum type ) const;
#endif

	ALenum			GetOpenALBufferFormat() const;

	void			CreateOpenALBuffer();

protected:
	friend class idSoundHardware_OpenAL;
	friend class idSoundVoice_OpenAL;

	~idSoundSample_OpenAL();

	bool			LoadWav( const idStr& name );
	bool			LoadAmplitude( const idStr& name );
	void			WriteAllSamples( const idStr& sampleName );
	bool			LoadGeneratedSample( const idStr& name );
	void			WriteGeneratedSample( idFile* fileOut );

	struct MS_ADPCM_decodeState_t
	{
		uint8 hPredictor;
		int16 coef1;
		int16 coef2;

		uint16 iDelta;
		int16 iSamp1;
		int16 iSamp2;
	};

	int32			MS_ADPCM_nibble( MS_ADPCM_decodeState_t* state, int8 nybble );
	int				MS_ADPCM_decode( uint8** audio_buf, uint32* audio_len );

	struct sampleBuffer_t
	{
		void* buffer;
		int bufferSize;
		int numSamples;
	};

	idStr			name;

	ID_TIME_T		timestamp;
	bool			loaded;

	bool			neverPurge;
	bool			levelLoadReferenced;
	bool			usesMapHeap;

	uint32			lastPlayedTime;

	int				totalBufferSize;	// total size of all the buffers
	idList<sampleBuffer_t, TAG_AUDIO> buffers;

	// OpenAL buffer that contains all buffers
	ALuint			openalBuffer;

	int				playBegin;
	int				playLength;

	idWaveFile::waveFmt_t	format;

	idList<byte, TAG_AMPLITUDE> amplitude;
};

/*
================================================
idSoundSample

This reverse-inheritance purportedly makes working on
multiple platforms easier.
================================================
*/
class idSoundSample : public idSoundSample_OpenAL
{
public:
};

#endif
