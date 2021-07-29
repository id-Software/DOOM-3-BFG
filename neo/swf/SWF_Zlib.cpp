/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2015 Robert Beckebans

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
#include "precompiled.h"
#pragma hdrstop
#include <zlib.h>

/*
========================
idSWF::Inflate
========================
*/
bool idSWF::Inflate( const byte* input, int inputSize, byte* output, int outputSize )
{
	struct local_swf_alloc_t
	{
		static void* zalloc( void* opaque, uint32 items, uint32 size )
		{
			return Mem_Alloc( items * size, TAG_SWF );
		}
		static void zfree( void* opaque, void* ptr )
		{
			Mem_Free( ptr );
		}
	};
	z_stream stream;
	memset( &stream, 0, sizeof( stream ) );
	stream.next_in = ( Bytef* )input;
	stream.avail_in = inputSize;
	stream.next_out = ( Bytef* )output;
	stream.avail_out = outputSize;
	stream.zalloc = local_swf_alloc_t::zalloc;
	stream.zfree = local_swf_alloc_t::zfree;
	inflateInit( &stream );
	bool success = ( inflate( &stream, Z_FINISH ) == Z_STREAM_END );
	inflateEnd( &stream );

	return success;
}

// RB begin
bool idSWF::Deflate( const byte* input, int inputSize, byte* output, int& outputSize )
{
	struct local_swf_alloc_t
	{
		static void* zalloc( void* opaque, uint32 items, uint32 size )
		{
			return Mem_Alloc( items * size, TAG_SWF );
		}
		static void zfree( void* opaque, void* ptr )
		{
			Mem_Free( ptr );
		}
	};
	z_stream stream;
	memset( &stream, 0, sizeof( stream ) );
	stream.next_in = ( Bytef* )input;
	stream.avail_in = inputSize;
	stream.next_out = ( Bytef* )output;
	stream.avail_out = outputSize;
	stream.zalloc = local_swf_alloc_t::zalloc;
	stream.zfree = local_swf_alloc_t::zfree;

	int err = deflateInit( &stream, Z_DEFAULT_COMPRESSION );
	if( err != Z_OK )
	{
		return false;
	}

	err = deflate( &stream, Z_FINISH );

	outputSize = stream.total_out;

	deflateEnd( &stream );

	return ( err == Z_STREAM_END );
}
// RB end
