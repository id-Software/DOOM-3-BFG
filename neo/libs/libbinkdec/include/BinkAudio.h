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

#ifndef _BinkAudio_h_
#define _BinkAudio_h_

#include <stdint.h>
#include <vector>

extern "C" {
#include "rdft.h"
#include "dct.h"
}

// audio transform type
enum eTransformType {
	kTransformTypeRDFT = 0,
	kTransformTypeDCT  = 1
};

const int kMaxChannels = 2;
const int kBlockMaxSize = kMaxChannels << 11;

static const int kNumCriticalFrequencies = 25;
static const uint16_t criticalFrequencies[kNumCriticalFrequencies] =
{
	100,   200,   300,   400,
	510,   630,   770,   920,
	1080,  1270,  1480,  1720,
	2000,  2320,  2700,  3150,
	3700,  4400,  5300,  6400,
	7700,  9500, 12000, 15500,
	24500
};

static const int kNumRLEentries = 16;
static const uint8_t RLEentries[kNumRLEentries] = 
{
	2,  3,  4,  5,
	6,  8,  9, 10,
	11, 12, 13, 14,
	15, 16, 32, 64
};

struct AudioTrack
{
	uint32_t sampleRate;
	uint32_t nChannels;     // internal use only
	uint32_t nChannelsReal; // number of channels you should pass to your audio API

	eTransformType transformType;
	int frameLenBits;
	int frameLength;        // transform size (samples)
	int overlapLength;      // overlap size (samples)
	int blockSize;
	uint32_t sampleRateHalf;
	uint32_t nBands;
	bool first;
	float root;

	union {
		RDFTContext rdft;
		DCTContext  dct;
	} trans;

	std::vector<uint32_t> bands;

	float coeffs[kBlockMaxSize];
	int16_t previous[kBlockMaxSize / 16];    // coeffs from previous audio block

	float *coeffsPtr[kMaxChannels];    // pointers to the coeffs arrays for float_to_int16_interleave

	int16_t  *blockBuffer;
	uint32_t blockBufferSize;

	uint8_t  *buffer;
	uint32_t bufferSize; // size in bytes

	uint32_t bytesReadThisFrame;
};

#endif