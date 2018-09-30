/*
 * libbinkdec - Bink video decoder
 * Copyright (C) 2011 Barry Duncan
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

/* This code is based on the Bink decoder from the FFmpeg project which can be obtained from http://www.ffmpeg.org/
 * below is the license from FFmpeg
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 * Bink video decoder
 * Copyright (c) 2009 Konstantin Shishkov
 * Copyright (C) 2011 Peter Ross <pross@xvid.org>
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "BinkDecoder.h"
#include "LogError.h"
#include "FFmpeg_includes.h"

std::vector<class BinkDecoder*> classInstances;

BinkHandle Bink_Open(const char* fileName)
{
	BinkHandle newHandle;
	newHandle.isValid = false;
	newHandle.instanceIndex = -1;

	BinkDecoder *newDecoder = new BinkDecoder();
	if (!newDecoder->Open(fileName))
	{
		delete newDecoder;
		return newHandle;
	}

	// loaded ok, make handle valid
	newHandle.isValid = true;
#if 0
	// find a free slot if available
	for (int i = 0; i < classInstances.size(); i++)
	{
		if (!classInstances[i])
		{
			class
		}
	}
#endif
	// add instance to global instance vector
	classInstances.push_back(newDecoder);

	// get a handle ID
	newHandle.instanceIndex = classInstances.size() - 1;
	
	return newHandle;
}

void Bink_Close(BinkHandle &handle)
{
	if (!classInstances.at(handle.instanceIndex))
	{
		// invalid handle
		return;
	}

	// close bink decoder
	delete classInstances[handle.instanceIndex];
	classInstances[handle.instanceIndex] = 0;

	handle.instanceIndex = -1;
	handle.isValid = false;
}

uint32_t Bink_GetNumAudioTracks(BinkHandle &handle)
{
	if (handle.instanceIndex == -1)
		return 0;
	else
		return classInstances[handle.instanceIndex]->GetNumAudioTracks();
}

AudioInfo Bink_GetAudioTrackDetails(BinkHandle &handle, uint32_t trackIndex)
{
	return classInstances[handle.instanceIndex]->GetAudioTrackDetails(trackIndex);
}

/* Get a frame's worth of audio data. 
 * 
 * 'data' needs to be a pointer to allocated memory that this function will fill.
 * You can find the size (in bytes) to make this buffer by calling Bink_GetAudioTrackDetails()
 * and checking the 'idealBufferSize' member in the returned AudioInfo struct
 */
uint32_t Bink_GetAudioData(BinkHandle &handle, uint32_t trackIndex, int16_t *data)
{
	return classInstances[handle.instanceIndex]->GetAudioData(trackIndex, data);
}

uint32_t Bink_GetNumFrames(BinkHandle &handle)
{
	return classInstances[handle.instanceIndex]->GetNumFrames();
}

void Bink_GetFrameSize(BinkHandle &handle, uint32_t &width, uint32_t &height)
{
	width  = classInstances[handle.instanceIndex]->frameWidth;
	height = classInstances[handle.instanceIndex]->frameHeight;
}

uint32_t Bink_GetCurrentFrameNum(BinkHandle &handle)
{
	return classInstances[handle.instanceIndex]->GetCurrentFrameNum();
}

uint32_t Bink_GetNextFrame(BinkHandle &handle, YUVbuffer yuv)
{
	BinkDecoder *decoder = classInstances[handle.instanceIndex];

	uint32_t frameIndex = decoder->GetCurrentFrameNum();

	decoder->GetNextFrame(yuv);

	return frameIndex;
}

float Bink_GetFrameRate(BinkHandle &handle)
{
	return classInstances[handle.instanceIndex]->GetFrameRate();
}

void Bink_GotoFrame(BinkHandle &handle, uint32_t frameNum)
{
	classInstances[handle.instanceIndex]->GotoFrame(frameNum);
}

BinkDecoder::BinkDecoder()
{
	nFrames = 0;
	currentFrame = 0;
}

BinkDecoder::~BinkDecoder()
{
	for (uint32_t i = 0; i < planes.size(); i++)
	{
		delete[] planes[i].current;
		delete[] planes[i].last;
	}

	for (uint32_t i = 0; i < audioTracks.size(); i++)
	{
		delete[] audioTracks[i]->buffer;
		delete[] audioTracks[i]->blockBuffer;

		if (kTransformTypeRDFT == audioTracks[i]->transformType)
			ff_rdft_end(&audioTracks[i]->trans.rdft);
		else if (kTransformTypeDCT == audioTracks[i]->transformType)
			ff_dct_end(&audioTracks[i]->trans.dct);

		delete audioTracks[i];
	}
}

uint32_t BinkDecoder::GetNumFrames()
{
	return nFrames;
}

uint32_t BinkDecoder::GetCurrentFrameNum()
{
	return currentFrame;
}

float BinkDecoder::GetFrameRate()
{
	return (float)fpsDividend / (float)fpsDivider;
}

void BinkDecoder::GotoFrame(uint32_t frameNum)
{
	// seek to the desired frame (just set currentFrame)
	currentFrame = frameNum;

	// what else? (memset some stuff?)
}

bool BinkDecoder::Open(const std::string &fileName)
{
	// open the file (read only)
	file.Open(fileName);
	if (!file.Is_Open())
	{
		BinkCommon::LogError("Can't open file " + fileName);
		return false;
	}

	// check the file signature
	signature = file.ReadUint32BE();
	if ((signature != kBIKfID) 
		&& (signature != kBIKgID)
		&& (signature != kBIKhID)
		&& (signature != kBIKiID))
	{
		BinkCommon::LogError("Unknown Bink signature");
		return false;
	}

	fileSize = file.ReadUint32LE() + 8;

	nFrames = file.ReadUint32LE();

	if (nFrames > 1000000)
	{
		BinkCommon::LogError("Invalid header, more than 1000000 frames");
		return false;
	}

	largestFrameSize = file.ReadUint32LE();
	if (largestFrameSize > fileSize)
	{
		BinkCommon::LogError("Largest frame size is greater than file size");
		return false;
	}

	// skip some unknown data
	file.Skip(4);

	frameWidth  = file.ReadUint32LE();
	frameHeight = file.ReadUint32LE();
	fpsDividend = file.ReadUint32LE();
	fpsDivider  = file.ReadUint32LE();
	videoFlags  = file.ReadUint32LE();
	
	nAudioTracks = file.ReadUint32LE();

	// audio is available
	if (nAudioTracks)
	{
		// skip some useless values (unknown and audio channels)
		file.Skip(4 * nAudioTracks);

		for (uint32_t i = 0; i < nAudioTracks; i++)
		{
			uint16_t sampleRate = file.ReadUint16LE();
			uint16_t flags      = file.ReadUint16LE();

			CreateAudioTrack(sampleRate, flags);
		}

		// skip the audio track IDs
		file.Skip(4 * nAudioTracks);
	}

	// read the video frames
	frames.resize(nFrames);

	uint32_t pos, nextPos;

	nextPos = file.ReadUint32LE();
	
	for (uint32_t i = 0; i < nFrames; i++)
	{
		pos = nextPos;
		if (i == nFrames - 1)
		{
			nextPos = fileSize;
			frames[i].keyFrame = 0;
		}
		else
		{
			nextPos = file.ReadUint32LE();
			frames[i].keyFrame = pos & 1;
		}

		pos &= ~1;
		nextPos &= ~1;

		frames[i].offset = pos;
		frames[i].size   = nextPos - pos;
	}

	// determine buffer sizes for audio tracks
	file.Seek(frames[0].offset);

	for (uint32_t trackIndex = 0; trackIndex < audioTracks.size(); trackIndex++)
	{
		// check for audio
		uint32_t audioPacketSize = file.ReadUint32LE();
			
		if (audioPacketSize >= 4)
		{
			// size in bytes of largest decoded audio
			uint32_t reportedSize = file.ReadUint32LE();

			AudioTrack *track = audioTracks[trackIndex];

			// size in bytes
			track->bufferSize = reportedSize;
			track->buffer = new uint8_t[reportedSize];

			// skip to next audio track (and -4 for reportedSize int we read) 
			file.Skip(audioPacketSize-4);
		}
		else
		{
			file.Skip(audioPacketSize);
		}
	}

	hasAlpha = videoFlags & kFlagAlpha;
	swapPlanes = signature >= kBIKhID;

	InitBundles();
	InitTrees();

	uint32_t width  = frameWidth;
	uint32_t height = frameHeight;

	// init plane memory
	Plane newPlane;
	planes.push_back(newPlane);

	// luma plane
	planes.back().Init(width, height);

	// chroma planes
	width  /= 2;
	height /= 2;

	// 1
	planes.push_back(newPlane);
	planes.back().Init(width, height);

	// 2
	planes.push_back(newPlane);
	planes.back().Init(width, height);

	// alpha plane
	if (hasAlpha)
	{
		width  *= 2;
		height *= 2;

		planes.push_back(newPlane);
		planes.back().Init(width, height);
	}

	return true;
}

void BinkDecoder::GetNextFrame(YUVbuffer yuv)
{
	// seek to fame offset
	file.Seek(frames[currentFrame].offset);
	uint32_t frameSize = frames[currentFrame].size;

	for (uint32_t trackIndex = 0; trackIndex < audioTracks.size(); trackIndex++)
	{
		// reset bytes read per frame (we might not get any audio for this frame)
		audioTracks[trackIndex]->bytesReadThisFrame = 0;

		// check for audio
		uint32_t audioPacketSize = file.ReadUint32LE();

		frameSize -= 4 + audioPacketSize;
			
		if (audioPacketSize >= 4)
		{
			uint32_t nSamples = file.ReadUint32LE();

			AudioPacket(trackIndex, audioPacketSize-4);
		}
		else
		{
			file.Skip(audioPacketSize);
		}
	}

	// get video packet
	VideoPacket(frameSize);

	// set planes data
	for (uint32_t i = 0; i < planes.size(); i++)
	{
		yuv[i].width  = planes[i].width;
		yuv[i].height = planes[i].height;
		yuv[i].pitch  = planes[i].pitch;
		yuv[i].data   = planes[i].last;
	}

	// frame done
	currentFrame++;
}
