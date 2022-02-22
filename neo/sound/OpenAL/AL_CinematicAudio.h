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
#include <precompiled.h>
#include "../CinematicAudio.h"
// SRS - Added check on OSX for OpenAL Soft headers vs macOS SDK headers
#if defined(__APPLE__) && !defined(USE_OPENAL_SOFT_INCLUDES)
	#include <OpenAL/al.h>
#else
	#include <AL/al.h>
#endif
#ifndef __CINEMATIC_AUDIO_AL_H__
#define __CINEMATIC_AUDIO_AL_H__

#include <queue>
#define NUM_BUFFERS 4

class CinematicAudio_OpenAL: public CinematicAudio
{
public:
	CinematicAudio_OpenAL();
	void InitAudio( void* audioContext );
	void PlayAudio( uint8_t* data, int size );
	void ResetAudio();
	void ShutdownAudio();
private:
	ALuint		alMusicSourceVoicecin;
	ALuint		alMusicBuffercin[NUM_BUFFERS];
	ALenum		av_sample_cin;
	int			av_rate_cin;
	int			offset;
	bool		trigger;

	//GK: Unlike XAudio2 which can accept buffer until the end of this world.
	//	  OpenAL can accept buffers only as long as there are freely available buffers.
	//	  So, what happens if there are no freely available buffers but we still geting audio frames ? Loss of data.
	//	  That why now I am using two queues in order to store the frames (and their sizes) and when we have available buffers,
	//	  then start popping those frames instead of the current, so we don't lose any audio frames and the sound doesn't crack anymore.
	std::queue<uint8_t*>	tBuffer;
	std::queue<int>			sizes;
	std::queue<ALuint>		bufids;		// SRS - Added queue of free alBuffer ids to handle audio frame underflow (starvation) case
};

#endif
