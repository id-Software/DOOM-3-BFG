/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.
Copyright (C) 2022 Justin Marshall

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "precompiled.h"
#pragma hdrstop

#include "snd_local.h"

/*
===================================================================================

  Thread safe decoder memory allocator.

  Each OggVorbis decoder consumes about 150kB of memory.

===================================================================================
*/

const int MIN_OGGVORBIS_MEMORY = 768 * 1024;

extern "C"
{
	void* _decoder_malloc( size_t size );
	void* _decoder_calloc( size_t num, size_t size );
	void* _decoder_realloc( void* memblock, size_t size );
	void _decoder_free( void* memblock );
}

void* _decoder_malloc( size_t size )
{
	void* ptr = malloc( size );
	assert( size == 0 || ptr != NULL );
	return ptr;
}

void* _decoder_calloc( size_t num, size_t size )
{
	void* ptr = malloc( num * size );
	assert( ( num * size ) == 0 || ptr != NULL );
	memset( ptr, 0, num * size );
	return ptr;
}

void* _decoder_realloc( void* memblock, size_t size )
{
	void* ptr = realloc( ( byte* )memblock, size );
	assert( size == 0 || ptr != NULL );
	return ptr;
}

void _decoder_free( void* memblock )
{
	free( ( byte* )memblock );
}


/*
===================================================================================

  OggVorbis file loading/decoding.

===================================================================================
*/

/*
====================
FS_ReadOGG
====================
*/
size_t FS_ReadOGG( void* dest, size_t size1, size_t size2, void* fh )
{
	idFile* f = reinterpret_cast<idFile*>( fh );
	return f->Read( dest, size1 * size2 );
}

/*
====================
FS_SeekOGG
====================
*/
int FS_SeekOGG( void* fh, ogg_int64_t to, int type )
{
	fsOrigin_t retype = FS_SEEK_SET;

	if( type == SEEK_CUR )
	{
		retype = FS_SEEK_CUR;
	}
	else if( type == SEEK_END )
	{
		retype = FS_SEEK_END;
	}
	else if( type == SEEK_SET )
	{
		retype = FS_SEEK_SET;
	}
	else
	{
		common->FatalError( "fs_seekOGG: seek without type\n" );
	}
	idFile* f = reinterpret_cast<idFile*>( fh );
	return f->Seek( to, retype );
}

/*
====================
FS_CloseOGG
====================
*/
int FS_CloseOGG( void* fh )
{
	return 0;
}

/*
====================
FS_TellOGG
====================
*/
long FS_TellOGG( void* fh )
{
	idFile* f = reinterpret_cast<idFile*>( fh );
	return f->Tell();
}

/*
====================
ov_openFile
====================
*/
int ov_openFile( idFile* f, OggVorbis_File* vf )
{
	ov_callbacks callbacks;

	memset( vf, 0, sizeof( OggVorbis_File ) );

	callbacks.read_func = FS_ReadOGG;
	callbacks.seek_func = FS_SeekOGG;
	callbacks.close_func = FS_CloseOGG;
	callbacks.tell_func = FS_TellOGG;
	return ov_open_callbacks( ( void* )f, vf, NULL, -1, callbacks );
}

/*
====================
idSoundDecoder_Vorbis::idSoundDecoder_Vorbis
====================
*/
idSoundDecoder_Vorbis::idSoundDecoder_Vorbis()
{
	sample = nullptr;
	vorbisFile = nullptr;
	mhmmio = nullptr;
}

/*
====================
idSoundDecoder_Vorbis::~idSoundDecoder_Vorbis
====================
*/
idSoundDecoder_Vorbis::~idSoundDecoder_Vorbis()
{
	if( sample )
	{
		sample = nullptr;
	}

	if( vorbisFile )
	{
		delete vorbisFile;
	}

	if( mhmmio )
	{
		fileSystem->CloseFile( mhmmio );
		mhmmio = nullptr;
	}
}

/*
====================
idSoundDecoder_Vorbis::GetFormat
====================
*/
void idSoundDecoder_Vorbis::GetFormat( idWaveFile::waveFmt_t& format )
{
	vorbis_info* vi = ov_info( vorbisFile, -1 );

	format.basic.samplesPerSec = vi->rate;
	format.basic.numChannels = vi->channels;
	format.basic.bitsPerSample = sizeof( short ) * 8;
	format.basic.formatTag = idWaveFile::FORMAT_PCM;
	format.basic.blockSize = format.basic.numChannels * format.basic.bitsPerSample / 8;
	format.basic.avgBytesPerSec = format.basic.samplesPerSec * format.basic.blockSize;
}

/*
====================
idSoundDecoder_Vorbis::Seek
====================
*/
void idSoundDecoder_Vorbis::Seek( int samplePos )
{
	ov_pcm_seek( vorbisFile, samplePos );
}

/*
====================
idSoundDecoder_Vorbis::IsEOS
====================
*/
bool idSoundDecoder_Vorbis::IsEOS( void )
{
	int64 size = ov_pcm_total( this->vorbisFile, -1 );
	return ov_pcm_tell( vorbisFile ) >= size;
}

/*
====================
idSoundDecoder_Vorbis::Size
====================
*/
int64_t idSoundDecoder_Vorbis::Size( void )
{
	vorbis_info* vi = ov_info( vorbisFile, -1 );
	int64 mdwSize = ov_pcm_total( vorbisFile, -1 ) * vi->channels;
	return mdwSize * sizeof( short );
}

/*
====================
idSoundDecoder_Vorbis::CompressedSize
====================
*/
int64_t idSoundDecoder_Vorbis::CompressedSize( void )
{
	return ov_pcm_total( this->vorbisFile, -1 );
}

/*
====================
idSoundDecoder_Vorbis::Read
====================
*/
int idSoundDecoder_Vorbis::Read( void* pBuffer, int dwSizeToRead )
{
	int total = dwSizeToRead;
	char* bufferPtr = ( char* )pBuffer;
	OggVorbis_File* ov = ( OggVorbis_File* )vorbisFile;

	do
	{
		int ret = ov_read( ov, bufferPtr, total >= 4096 ? 4096 : total, Swap_IsBigEndian(), 2, 1, &ov->stream );
		if( ret == 0 )
		{
			break;
		}
		if( ret < 0 )
		{
			return -1;
		}
		bufferPtr += ret;
		total -= ret;
	}
	while( total > 0 );

	dwSizeToRead = ( byte* )bufferPtr - ( byte* )pBuffer;

	return dwSizeToRead;
}

/*
====================
idSoundDecoder_Vorbis::Open
====================
*/
bool idSoundDecoder_Vorbis::Open( const char* fileName )
{
	if( mhmmio )
	{
		fileSystem->CloseFile( mhmmio );
		mhmmio = nullptr;
	}

	if( vorbisFile != nullptr )
	{
		delete vorbisFile;
		vorbisFile = nullptr;
	}

	mhmmio = fileSystem->OpenFileRead( fileName );
	if( !mhmmio )
	{
		return false;
	}

	vorbisFile = new OggVorbis_File;

	if( ov_openFile( mhmmio, vorbisFile ) < 0 )
	{
		delete vorbisFile;
		fileSystem->CloseFile( mhmmio );
		common->FatalError( "ov_openFile failed" );
		return false;
	}

	//this->sample = sample;		// SRS - self assignment not needed here

	return true;
}
