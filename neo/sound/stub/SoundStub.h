/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2012 Daniel Gibson

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

/*
 * DG: a stub to get d3 bfg to compile without XAudio2, because that doesn't work with MinGW
 * or on non-Windows platforms.
 *
 * Please note that many methods are *not* virtual, so just inheriting from the stubs for the
 * actual implementations may *not* work!
 * (Making them virtual should be evaluated for performance-loss though, it would make the code
 *  cleaner and may be feasible)
 */

#ifndef SOUNDSTUB_H_
#define SOUNDSTUB_H_

#include "idlib/precompiled.h" // TIME_T
#include "../WaveFile.h"

class idSoundVoice : public idSoundVoice_Base
{
public:
	void					Create( const idSoundSample* leadinSample, const idSoundSample* loopingSample ) {}

	// Start playing at a particular point in the buffer.  Does an Update() too
	void					Start( int offsetMS, int ssFlags ) {}

	// Stop playing.
	void					Stop() {}

	// Stop consuming buffers
	void					Pause() {}
	// Start consuming buffers again
	void					UnPause() {}

	// Sends new position/volume/pitch information to the hardware
	bool					Update()
	{
		return false;
	}

	// returns the RMS levels of the most recently processed block of audio, SSF_FLICKER must have been passed to Start
	float					GetAmplitude()
	{
		return 0.0f;
	}

	// returns true if we can re-use this voice
	bool					CompatibleFormat( idSoundSample* s )
	{
		return false;
	}

	uint32					GetSampleRate() const
	{
		return 0;
	}

	// callback function
	void					OnBufferStart( idSoundSample* sample, int bufferNumber ) {}
};

class idSoundHardware
{
public:
	idSoundHardware() {}

	void			Init() {}
	void			Shutdown() {}

	void 			Update() {}

	// FIXME: this is a bad name when having multiple sound backends... and maybe it's not even needed
	void* 		GetIXAudio2() const // NOTE: originally this returned IXAudio2*, but that was casted to void later anyway
	{
		return NULL;
	}

	idSoundVoice* 	AllocateVoice( const idSoundSample* leadinSample, const idSoundSample* loopingSample )
	{
		return NULL;
	}

	void			FreeVoice( idSoundVoice* voice ) {}

	int				GetNumZombieVoices() const
	{
		return 0;
	}

	int				GetNumFreeVoices() const
	{
		return 0;
	}

};

// ok, this one isn't really a stub, because it seems to be XAudio-independent,
// I just copied the class from idSoundSample_XAudio2 and renamed it
class idSoundSample
{
public:
	idSoundSample();

	~idSoundSample(); // destructor should be public so lists of  soundsamples can be destroyed etc

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

protected:

	/*
		friend class idSoundHardware_XAudio2;
		friend class idSoundVoice_XAudio2;
	*/

	bool			LoadWav( const idStr& name );
	bool			LoadAmplitude( const idStr& name );
	void			WriteAllSamples( const idStr& sampleName );
	bool			LoadGeneratedSample( const idStr& name );
	void			WriteGeneratedSample( idFile* fileOut );

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

	int				playBegin;
	int				playLength;

	idWaveFile::waveFmt_t	format;

	idList<byte, TAG_AMPLITUDE> amplitude;
};

#endif /* SOUNDSTUB_H_ */
