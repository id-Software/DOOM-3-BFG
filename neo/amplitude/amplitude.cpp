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

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

static const int SAMPLE_RATE = 60;

enum errorCodes_t
{
	E_OK = 0,
	E_ARGS,
	E_OPEN_IN,
	E_OPEN_OUT,
	E_PROCESSING
};

struct chunk_t
{
	unsigned int id;
	unsigned int size;
	unsigned int offset;
};

static unsigned short FORMAT_PCM = 0x0001;

struct format_t
{
	unsigned short formatTag;
	unsigned short numChannels;
	unsigned int samplesPerSec;
	unsigned int avgBytesPerSec;
	unsigned short sampleSize;
	unsigned short bitsPerSample;
};

#define SwapBytes( x, y ) { unsigned char t = (x); (x) = (y); (y) = t; }

template<class type> static void Swap( type& c )
{
	if( sizeof( type ) == 1 )
	{
	}
	else if( sizeof( type ) == 2 )
	{
		unsigned char* b = ( unsigned char* )&c;
		SwapBytes( b[0], b[1] );
	}
	else if( sizeof( type ) == 4 )
	{
		unsigned char* b = ( unsigned char* )&c;
		SwapBytes( b[0], b[3] );
		SwapBytes( b[1], b[2] );
	}
	else if( sizeof( type ) == 8 )
	{
		unsigned char* b = ( unsigned char* )&c;
		SwapBytes( b[0], b[7] );
		SwapBytes( b[1], b[6] );
		SwapBytes( b[2], b[5] );
		SwapBytes( b[3], b[4] );
	}
	else
	{
		int* null = 0;
		c = *null;
	}
}

int WAVE_ReadHeader( FILE* f )
{
	struct header_t
	{
		unsigned int id;
		unsigned int size;
		unsigned int format;
	} header;

	fread( &header, sizeof( header ), 1, f );
	Swap( header.id );
	Swap( header.format );

	if( header.id != 'RIFF' || header.format != 'WAVE' || header.size < 4 )
	{
		return 0;
	}

	return header.size;
}

int WAVE_ReadChunks( FILE* f, unsigned int fileSize, chunk_t* chunks, int maxChunks )
{
	unsigned int offset = ftell( f );
	int numChunks = 0;

	while( offset < fileSize )
	{
		struct chuckHeader_t
		{
			unsigned int id;
			unsigned int size;
		} chunkHeader;
		if( fread( &chunkHeader, sizeof( chunkHeader ), 1, f ) != 1 )
		{
			return numChunks;
		}
		Swap( chunkHeader.id );
		offset += sizeof( chunkHeader );

		if( numChunks == maxChunks )
		{
			return maxChunks + 1;
		}

		chunks[numChunks].id = chunkHeader.id;
		chunks[numChunks].size = chunkHeader.size;
		chunks[numChunks].offset = offset;
		numChunks++;

		offset += chunkHeader.size;
		fseek( f, offset, SEEK_SET );
	}

	return numChunks;
}

bool Process( FILE* in, FILE* out )
{

	int headerSize = WAVE_ReadHeader( in );
	if( headerSize == 0 )
	{
		printf( "Header invalid\n" );
		return false;
	}

	static const int MAX_CHUNKS = 32;
	chunk_t chunks[MAX_CHUNKS] = {};

	int numChunks = WAVE_ReadChunks( in, headerSize + 8, chunks, MAX_CHUNKS );
	if( numChunks == 0 )
	{
		printf( "No chunks\n" );
		return false;
	}
	if( numChunks > MAX_CHUNKS )
	{
		printf( "Too many chunks\n" );
		return false;
	}

	format_t format;
	bool foundFormat = false;
	unsigned int dataOffset = 0;
	unsigned int dataSize = 0;
	for( int i = 0; i < numChunks; i++ )
	{
		if( chunks[i].id == 'fmt ' )
		{
			if( foundFormat )
			{
				printf( "Found multiple format chunks\n" );
				return false;
			}
			if( chunks[i].size < sizeof( format ) )
			{
				printf( "Format chunk too small\n" );
				return false;
			}
			fseek( in, chunks[i].offset, SEEK_SET );
			fread( &format, sizeof( format ), 1, in );
			foundFormat = true;
		}
		if( chunks[i].id == 'data' )
		{
			if( dataOffset > 0 )
			{
				printf( "Found multiple data chunks\n" );
				return false;
			}
			dataOffset = chunks[i].offset;
			dataSize = chunks[i].size;
		}
	}
	if( dataOffset == 0 )
	{
		printf( "Colud not find data chunk\n" );
		return false;
	}
	if( !foundFormat )
	{
		printf( "Could not find fmt chunk\n" );
		return false;
	}
	if( format.formatTag != FORMAT_PCM )
	{
		printf( "Only PCM files supported (%d)\n", format.formatTag );
		return false;
	}
	if( format.bitsPerSample != 8 && format.bitsPerSample != 16 )
	{
		printf( "Only 8 or 16 bit files supported (%d)\n", format.bitsPerSample );
		return false;
	}
	if( format.numChannels != 1 && format.numChannels != 2 )
	{
		printf( "Only stereo or mono files supported (%d)\n", format.numChannels );
		return false;
	}
	unsigned short expectedSampleSize = format.numChannels * format.bitsPerSample / 8;
	if( format.sampleSize != expectedSampleSize )
	{
		printf( "Invalid sampleSize (%d, expected %d)\n", format.sampleSize, expectedSampleSize );
		return false;
	}
	unsigned int numSamples = dataSize / expectedSampleSize;

	void* inputData = malloc( dataSize );
	if( inputData == NULL )
	{
		printf( "Out of memory\n" );
		return false;
	}
	fseek( in, dataOffset, SEEK_SET );
	fread( inputData, dataSize, 1, in );

	int numOutputSamples = 1 + ( numSamples * SAMPLE_RATE / format.samplesPerSec );
	float* min = ( float* )malloc( numOutputSamples * sizeof( float ) );
	float* max = ( float* )malloc( numOutputSamples * sizeof( float ) );
	unsigned char* outputData = ( unsigned char* )malloc( numOutputSamples );
	if( min == NULL || max == NULL || outputData == NULL )
	{
		printf( "Out of memory\n" );
		free( inputData );
		free( min );
		free( max );
		free( outputData );
		return false;
	}
	for( int i = 0; i < numOutputSamples; i++ )
	{
		max[i] = -1.0f;
		min[i] = 1.0f;
	}

	if( format.bitsPerSample == 16 )
	{
		short* sdata = ( short* )inputData;
		if( format.numChannels == 1 )
		{
			for( unsigned int i = 0; i < numSamples; i++ )
			{
				unsigned int index = i * SAMPLE_RATE / format.samplesPerSec;
				float fdata = ( float )sdata[i] / 32767.0f;
				min[index] = __min( min[index], fdata );
				max[index] = __max( max[index], fdata );
			}
		}
		else
		{
			unsigned int j = 0;
			for( unsigned int i = 0; i < numSamples; i++ )
			{
				unsigned int index = i * SAMPLE_RATE / format.samplesPerSec;
				for( unsigned int c = 0; c < format.numChannels; c++ )
				{
					float fdata = ( float )sdata[j++] / 32767.0f;
					min[index] = __min( min[index], fdata );
					max[index] = __max( max[index], fdata );
				}
			}
		}
	}
	else
	{
		unsigned char* bdata = ( unsigned char* )inputData;
		if( format.numChannels == 1 )
		{
			for( unsigned int i = 0; i < numSamples; i++ )
			{
				unsigned int index = i * SAMPLE_RATE / format.samplesPerSec;
				float fdata = ( ( float )bdata[i] - 128.0f ) / 127.0f;
				min[index] = __min( min[index], fdata );
				max[index] = __max( max[index], fdata );
			}
		}
		else
		{
			unsigned int j = 0;
			for( unsigned int i = 0; i < numSamples; i++ )
			{
				unsigned int index = i * SAMPLE_RATE / format.samplesPerSec;
				for( unsigned int c = 0; c < format.numChannels; c++ )
				{
					float fdata = ( ( float )bdata[j++] - 128.0f ) / 127.0f;
					min[index] = __min( min[index], fdata );
					max[index] = __max( max[index], fdata );
				}
			}
		}
	}
	for( int i = 0; i < numOutputSamples; i++ )
	{
		float amp = atan( max[i] - min[i] ) / 0.7853981633974483f;
		int o = ( int )( amp * 255.0f );
		if( o > 255 )
		{
			outputData[i] = 255;
		}
		else if( o < 0 )
		{
			outputData[i] = 0;
		}
		else
		{
			outputData[i] = ( unsigned char )o;
		}
	}
	fwrite( outputData, numOutputSamples, 1, out );

	free( inputData );
	free( min );
	free( max );
	free( outputData );

	printf( "Success\n" );
	return true;
}

int main( int argc, char* argv[] )
{

	if( argc < 2 )
	{
		printf( "Usage: %s <wav>\n", argv[0] );
		return E_ARGS;
	}
	const char* inputFileName = argv[1];

	printf( "Processing %s: ", inputFileName );

	FILE* in = NULL;
	if( fopen_s( &in, inputFileName, "rb" ) != 0 )
	{
		printf( "Could not open input file\n" );
		return E_OPEN_IN;
	}
	char outputFileName[1024] = {0};
	if( strcpy_s( outputFileName, inputFileName ) != 0 )
	{
		printf( "Filename too long\n" );
		return E_ARGS;
	}
	char* dot = strrchr( outputFileName, '.' );
	if( dot == NULL )
	{
		dot = outputFileName + strlen( outputFileName );
	}
	if( strcpy_s( dot, sizeof( outputFileName ) - ( dot - outputFileName ), ".amp" ) != 0 )
	{
		printf( "Filename too long\n" );
		return E_ARGS;
	}

	FILE* out = NULL;
	if( fopen_s( &out, outputFileName, "wb" ) != 0 )
	{
		printf( "Could not open output file %s\n", outputFileName );
		return E_OPEN_OUT;
	}

	bool success = Process( in, out );

	fclose( in );
	fclose( out );

	if( !success )
	{
		remove( outputFileName );
		return E_PROCESSING;
	}

	return E_OK;
}
