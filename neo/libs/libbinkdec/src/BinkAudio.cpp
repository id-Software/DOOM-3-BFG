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
#include "BinkAudio.h"
#include "Util.h"
#include <algorithm> // DG: for std::min/max

const int kAudio16Bits = 0x4000;
const int kAudioStereo = 0x2000;
const int kAudioUseDCT = 0x1000;

const int kNumBands = 25;

/**
 * Clip a signed integer value into the -32768,32767 range.
 * @param a value to clip
 * @return clipped value
 */
static /*av_always_inline*/ const int16_t av_clip_int16_c(int a)
{
    if ((a+0x8000) & ~0xFFFF) return (a>>31) ^ 0x7FFF;
    else                      return a;
}

static /*av_always_inline*/ int float_to_int16_one(const float *src){
    return av_clip_int16_c((int)(*src));
}

// from fmtconvert.c
static void float_to_int16_interleave_c(int16_t *dst, const float **src, long len, int channels)
{
    if (channels==2)
	{
        for (int i = 0; i < len; i++)
		{
            dst[2*i]   = float_to_int16_one(src[0] + i);
            dst[2*i+1] = float_to_int16_one(src[1] + i);
        }
    }
	else
	{
        for (int c = 0; c < channels; c++)
		{
            for (int i = 0, j = c; i < len; i++, j += channels)
                dst[j] = float_to_int16_one(src[c] + i);
		}
    }
}

float BinkDecoder::GetAudioFloat(BinkCommon::BitReader &bits)
{
    int power = bits.GetBits(5);
    float f = ldexpf((float)bits.GetBits(23), power - 23);
    if (bits.GetBit())
        f = -f;

    return f;
}

bool BinkDecoder::CreateAudioTrack(uint32_t sampleRate, uint16_t flags)
{
	AudioTrack *track = new AudioTrack;

	// determine frame length
	uint32_t frameLenBits, nChannels;

	if (sampleRate < 22050) {
		frameLenBits = 9;
	}
	else if (sampleRate < 44100) {
		frameLenBits = 10;
	}
	else {
		frameLenBits = 11;
	}

	// check nunber of channels
	if (flags & kAudioStereo) {
		nChannels = 2;
	}
	else {
		nChannels = 1;
	}

	track->nChannelsReal = nChannels;
	
	if (flags & kAudioUseDCT)
	{
		track->transformType = kTransformTypeDCT;
	}
	else
	{
		track->transformType = kTransformTypeRDFT;

		// audio is already interleaved for the RDFT format variant
		sampleRate   *= nChannels;
		frameLenBits += av_log2_c(nChannels);
		nChannels = 1;
	}

	int frameLength = 1 << frameLenBits;
	int overlapLength = frameLength / 16;
	int blockSize = (frameLength - overlapLength) * nChannels;

	int sampleRateHalf = (sampleRate + 1) / 2;
	float root = 2.0f / sqrt((float)frameLength);

	uint32_t nBands = 0;

	// calculate number of bands
	for (nBands = 1; nBands < kNumBands; nBands++)
	{
		if (sampleRateHalf <= criticalFrequencies[nBands - 1])
			break;
	}

	track->bands.resize(nBands + 1);

	// populate bands data
	track->bands[0] = 2;

	for (uint32_t i = 1; i < nBands; i++)
	{
		track->bands[i] = (criticalFrequencies[i - 1] * frameLength / sampleRateHalf) & ~1;
	}

	track->bands[nBands] = frameLength;

	// initialise coefficients pointer array
	for (uint32_t i = 0; i < nChannels; i++)
	{
		track->coeffsPtr[i] = track->coeffs + i * frameLength;
	}

	if (kTransformTypeRDFT == track->transformType)
		ff_rdft_init(&track->trans.rdft, frameLenBits, DFT_C2R);
	else if (kTransformTypeDCT == track->transformType)
		ff_dct_init(&track->trans.dct, frameLenBits, DCT_III);
	else
		return false;

	track->blockBufferSize = frameLength * nChannels * sizeof(int16_t);
	track->blockBuffer     = new int16_t[track->blockBufferSize];

	track->blockSize      = blockSize;
	track->frameLenBits   = frameLenBits;
	track->frameLength    = frameLength;
	track->nBands         = nBands;
	track->nChannels      = nChannels;
	track->overlapLength  = overlapLength;
	track->root           = root;
	track->sampleRate     = sampleRate;
	track->sampleRateHalf = sampleRateHalf;
	track->first          = true;

	// should contain entire frames worth of audio when frame is decoded
	track->bufferSize = 0;
	track->buffer     = 0;
	track->bytesReadThisFrame = 0;
	
	// add the track to the track vector
	audioTracks.push_back(track);

	return true;
}

void BinkDecoder::AudioPacket(uint32_t trackIndex, uint32_t packetSize)
{
	// create a temp bit reader for this packet
	BinkCommon::BitReader bits(file, packetSize);

	AudioTrack *track = audioTracks[trackIndex];

	uint8_t *bufferPtr = track->buffer;

	// each call to DecodeAudioBlock should produce this many bytes
	uint32_t bytesDone = track->blockSize * 2;

	while (bits.GetPosition() < bits.GetSize())
	{
		DecodeAudioBlock(trackIndex, bits);

		memcpy(bufferPtr, track->blockBuffer, std::min(track->bufferSize - track->bytesReadThisFrame, bytesDone));
		bufferPtr += bytesDone;

		track->bytesReadThisFrame += bytesDone;

		// align to a 32 bit boundary
		int n = (-(int)bits.GetPosition()) & 31;
		if (n) bits.SkipBits(n);
	}
}

void BinkDecoder::DecodeAudioBlock(uint32_t trackIndex, BinkCommon::BitReader &bits)
{
	int i, j, k;
	float q, quant[25];
	int width, coeff;

	AudioTrack *track = audioTracks[trackIndex];

	int16_t *out = track->blockBuffer;

	if (kTransformTypeDCT == track->transformType)
		bits.SkipBits(2);

	for (uint32_t ch = 0; ch < track->nChannels; ch++) 
	{
		float *coeffs = track->coeffsPtr[ch];

		coeffs[0] = GetAudioFloat(bits) * track->root;
		coeffs[1] = GetAudioFloat(bits) * track->root;

        for (uint32_t b = 0; b < track->nBands; b++) 
		{
            /* constant is result of 0.066399999/log10(M_E) */
            int value = bits.GetBits(8);
            quant[b] = expf(std::min(value, (int)95) * 0.15289164787221953823f) * track->root;
        }

        k = 0;
        q = quant[0];

        // parse coefficients
        i = 2;
        while (i < track->frameLength)
		{
			if (bits.GetBit())
			{
                j = i + RLEentries[bits.GetBits(4)] * 8;
            } else {
                j = i + 8;
            }

            j = std::min(j, track->frameLength);

            width = bits.GetBits(4);
            if (width == 0) 
			{
                memset(coeffs + i, 0, (j - i) * sizeof(*coeffs));
                i = j;
                while (track->bands[k] < i)
                    q = quant[k++];
            }
			else
			{
                while (i < j)
				{
                    if (track->bands[k] == i)
                        q = quant[k++];
                    coeff = bits.GetBits(width);
                    if (coeff) 
					{
                        if (bits.GetBit())
                            coeffs[i] = -q * coeff;
                        else
                            coeffs[i] =  q * coeff;
                    } 
					else 
					{
                        coeffs[i] = 0.0f;
                    }
                    i++;
                }
            }
        }

		if (kTransformTypeRDFT == track->transformType)
		{
			track->trans.rdft.rdft_calc(&track->trans.rdft, coeffs);
		}
		else if (kTransformTypeDCT == track->transformType)
		{
			coeffs[0] /= 0.5f;
			track->trans.dct.dct_calc(&track->trans.dct,  coeffs);

			float mul = track->frameLength;

			// vector_fmul_scalar()
			for (int i = 0; i < track->frameLength; i++)
				coeffs[i] = coeffs[i] * mul;
		}
	}

	float_to_int16_interleave_c(out, (const float **)track->coeffsPtr, track->frameLength, track->nChannels);

	if (!track->first) {
        int count = track->overlapLength * track->nChannels;
        int shift = av_log2_c(count);
        for (i = 0; i < count; i++) {
            out[i] = (track->previous[i] * (count - i) + out[i] * i) >> shift;
		}
	}

	memcpy(track->previous, out + track->blockSize, track->overlapLength * track->nChannels * sizeof(*out));

	track->first = false;
}

uint32_t BinkDecoder::GetNumAudioTracks()
{
	return audioTracks.size();
}

AudioInfo BinkDecoder::GetAudioTrackDetails(uint32_t trackIndex)
{
	AudioInfo info;
	AudioTrack *track = audioTracks[trackIndex];

	info.sampleRate = track->sampleRate;
	info.nChannels  = track->nChannelsReal;

	// undo sample rate adjustment we do internally for RDFT audio
	if (kTransformTypeRDFT == track->transformType)
	{
		info.sampleRate /= track->nChannelsReal;
	}

	// audio buffer size in bytes
	info.idealBufferSize = track->bufferSize;

	return info;
}

uint32_t BinkDecoder::GetAudioData(uint32_t trackIndex, int16_t *audioBuffer)
{
	if (!audioBuffer)
		return 0;

	AudioTrack *track = audioTracks[trackIndex];

	if (track->bytesReadThisFrame)
		memcpy(audioBuffer, track->buffer, std::min(track->bufferSize, track->bytesReadThisFrame));

	return track->bytesReadThisFrame;
}
