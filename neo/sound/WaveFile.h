/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.

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
#ifndef __WAVEFILE_H
#define __WAVEFILE_H

/*
================================================================================================
Contains the WaveFile declaration.
================================================================================================
*/

/*
================================================
idWaveFile is used for reading generic RIFF WAVE files.
================================================
*/
class idWaveFile
{
public:
	ID_INLINE 	idWaveFile();
	ID_INLINE 	~idWaveFile();

	bool		Open( const char* filename );
	void		Close();
	uint32		SeekToChunk( uint32 id );
	size_t		Read( void* buffer, size_t len )
	{
		return file->Read( buffer, len );
	}
	uint32		GetChunkOffset( uint32 id );

	ID_TIME_T	Timestamp()
	{
		return file->Timestamp();
	}
	const char* Name()
	{
		return ( file == NULL ? "" : file->GetName() );
	}

	// This maps to the channel mask in waveFmtExtensible_t
	enum
	{
		CHANNEL_INDEX_FRONT_LEFT,
		CHANNEL_INDEX_FRONT_RIGHT,
		CHANNEL_INDEX_FRONT_CENTER,
		CHANNEL_INDEX_LOW_FREQUENCY,
		CHANNEL_INDEX_BACK_LEFT,
		CHANNEL_INDEX_BACK_RIGHT,
		CHANNEL_INDEX_FRONT_LEFT_CENTER,
		CHANNEL_INDEX_FRONT_RIGHT_CENTER,
		CHANNEL_INDEX_BACK_CENTER,
		CHANNEL_INDEX_SIDE_LEFT,
		CHANNEL_INDEX_SIDE_RIGHT,
		CHANNEL_INDEX_MAX
	};
	enum
	{
		CHANNEL_MASK_FRONT_LEFT			= BIT( CHANNEL_INDEX_FRONT_LEFT ),
		CHANNEL_MASK_FRONT_RIGHT		= BIT( CHANNEL_INDEX_FRONT_RIGHT ),
		CHANNEL_MASK_FRONT_CENTER		= BIT( CHANNEL_INDEX_FRONT_CENTER ),
		CHANNEL_MASK_LOW_FREQUENCY		= BIT( CHANNEL_INDEX_LOW_FREQUENCY ),
		CHANNEL_MASK_BACK_LEFT			= BIT( CHANNEL_INDEX_BACK_LEFT ),
		CHANNEL_MASK_BACK_RIGHT			= BIT( CHANNEL_INDEX_BACK_RIGHT ),
		CHANNEL_MASK_FRONT_LEFT_CENTER	= BIT( CHANNEL_INDEX_FRONT_LEFT_CENTER ),
		CHANNEL_MASK_FRONT_RIGHT_CENTER	= BIT( CHANNEL_INDEX_FRONT_RIGHT_CENTER ),
		CHANNEL_MASK_BACK_CENTER		= BIT( CHANNEL_INDEX_BACK_CENTER ),
		CHANNEL_MASK_SIDE_LEFT			= BIT( CHANNEL_INDEX_SIDE_LEFT ),
		CHANNEL_MASK_SIDE_RIGHT			= BIT( CHANNEL_INDEX_SIDE_RIGHT ),
		CHANNEL_MASK_ALL				= BIT( CHANNEL_INDEX_MAX ) - 1,
	};

	// This matches waveFmt_t::formatTag
	// These are the only wave formats that we understand
	enum
	{
		FORMAT_UNKNOWN		= 0x0000,
		FORMAT_PCM			= 0x0001,
		FORMAT_ADPCM		= 0x0002,
		FORMAT_XMA2			= 0x0166,
		FORMAT_EXTENSIBLE	= 0xFFFF,
	};

#pragma pack( push, 1 )
	struct waveFmt_t
	{
		static const uint32 id = 'fmt ';
		// This is the basic data we'd expect to see in any valid wave file
		struct basic_t
		{
			uint16 formatTag;
			uint16 numChannels;
			uint32 samplesPerSec;
			uint32 avgBytesPerSec;
			uint16 blockSize;
			uint16 bitsPerSample;
		} basic;
		// Some wav file formats have extra data after the basic header
		uint16 extraSize;
		// We have a few known formats that we handle:
		union extra_t
		{
			// Valid if basic.formatTag == FORMAT_EXTENSIBLE
			struct extensible_t
			{
				uint16 validBitsPerSample;	// Valid bits in each sample container
				uint32 channelMask;			// Positions of the audio channels
				struct guid_t
				{
					uint32 data1;
					uint16 data2;
					uint16 data3;
					uint16 data4;
					byte data5[ 6 ];
				} subFormat;				// Format identifier GUID
			} extensible;
			// Valid if basic.formatTag == FORMAT_ADPCM
			// The microsoft ADPCM struct has a zero-sized array at the end
			// but the array is always 7 entries, so we set it to that size
			// so we can embed it in our format union.  Otherwise, the struct
			// is exactly the same as the one in audiodefs.h
			struct adpcm_t
			{
				uint16 samplesPerBlock;
				uint16 numCoef;
				struct adpcmcoef_t
				{
					short coef1;
					short coef2;
				} aCoef[7];  // Always 7 coefficient pairs for MS ADPCM
			} adpcm;
			// Valid if basic.formatTag == FORMAT_XMA2
			struct xma2_t
			{
				uint16 numStreams;		// Number of audio streams (1 or 2 channels each)
				uint32 channelMask;		// matches the CHANNEL_MASK enum above
				uint32 samplesEncoded;	// Total number of PCM samples the file decodes to
				uint32 bytesPerBlock;	// XMA block size (but the last one may be shorter)
				uint32 playBegin;		// First valid sample in the decoded audio
				uint32 playLength;		// Length of the valid part of the decoded audio
				uint32 loopBegin;		// Beginning of the loop region in decoded sample terms
				uint32 loopLength;		// Length of the loop region in decoded sample terms
				byte   loopCount;		// Number of loop repetitions; 255 = infinite
				byte   encoderVersion;	// Version of XMA encoder that generated the file
				uint16 blockCount;		// XMA blocks in file (and entries in its seek table)
			} xma2;
		} extra;
	};

#pragma pack( pop )

	struct dataChunk_t
	{
		static const uint32 id = 'data';
		uint32 size;
		void* data;
	};

	struct formatChunk_t
	{
		static const uint32 id = 'fmt ';
		uint32 size;
		uint16 compressionCode;
		uint16 numChannels;
		uint32 sampleRate;
		uint32 averageBytesPerSecond;
		uint16 blockAlign;
		uint16 bitsPerSample;
		uint16 numExtraFormatByte;
	};

	struct samplerChunk_t
	{
		static const uint32 id = 'smpl';
		uint32 manufacturer;		// ignored
		uint32 product;				// ignored
		uint32 samplePeriod;		// ignored (normally 1000000000/samplesPerSec)
		uint32 MIDIUnityNote;		// ignored
		uint32 MIDIPitchFraction;	// ignored
		uint32 SMPTEFormat;			// ignored
		uint32 SMPTEOffset;			// ignored
		uint32 numSampleLoops;		// number of samples in wave file
		uint32 extraSamplerData;	// ignored, should always be 0
	};

	struct sampleData_t
	{
		uint32 identifier;		// ignored
		uint32 type;			// 0 for loop 33 multi-sample sample type
		uint32 start;			// start of the loop point
		uint32 end;				// end of the loop point
		uint32 fraction;		// ignored
		uint32 playCount;		// ignored
	};

	const char* ReadWaveFormat( waveFmt_t& waveFmt );
	static bool  ReadWaveFormatDirect( waveFmt_t& format, idFile* file );
	static bool  WriteWaveFormatDirect( waveFmt_t& format, idFile* file );
	static bool  WriteSampleDataDirect( idList< sampleData_t >& sampleData, idFile* file );
	static bool  WriteDataDirect( char* _data, uint32 size, idFile* file );
	static bool  WriteHeaderDirect( uint32 fileSize, idFile* file );

	bool		 ReadLoopData( int& start, int& end );

private:
	idFile* 					file;

	struct chunk_t
	{
		uint32 id;
		uint32 size;
		uint32 offset;
	};

	idStaticList< chunk_t, 32 >	chunks;


};

/*
========================
idWaveFile::idWaveFile
========================
*/
ID_INLINE idWaveFile::idWaveFile() : file( NULL )
{
}

/*
========================
idWaveFile::~idWaveFile
========================
*/
ID_INLINE idWaveFile::~idWaveFile()
{
	Close();
}

#endif // !__WAVEFILE_H__
