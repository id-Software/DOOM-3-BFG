
#include "precompiled.h"
#pragma hdrstop

/*
Copyright (c) 1996 Lars Wirzenius.  All rights reserved.

June 14 2003: TTimo <ttimo@idsoftware.com>
	modified + endian bug fixes
	http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=197039

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/

/*
============
idBase64::Encode
============
*/
static const char sixtet_to_base64[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

void idBase64::Encode( const byte* from, int size )
{
	int i, j;
	unsigned int w; // DG: use int instead of long for 64bit compatibility
	byte* to;

	EnsureAlloced( 4 * ( size + 3 ) / 3 + 2 ); // ratio and padding + trailing \0
	to = data;

	w = 0;
	i = 0;
	while( size > 0 )
	{
		w |= *from << i * 8;
		++from;
		--size;
		++i;
		if( size == 0 || i == 3 )
		{
			byte out[4];
			SixtetsForInt( out, w );
			for( j = 0; j * 6 < i * 8; ++j )
			{
				*to++ = sixtet_to_base64[ out[j] ];
			}
			if( size == 0 )
			{
				for( j = i; j < 3; ++j )
				{
					*to++ = '=';
				}
			}
			w = 0;
			i = 0;
		}
	}

	*to++ = '\0';
	len = to - data;
}

/*
============
idBase64::DecodeLength
returns the minimum size in bytes of the target buffer for decoding
4 base64 digits <-> 3 bytes
============
*/
int idBase64::DecodeLength() const
{
	return 3 * len / 4;
}

/*
============
idBase64::Decode
============
*/
int idBase64::Decode( byte* to ) const
{
	unsigned int w; // DG: use int instead of long for 64bit compatibility
	int i, j;
	size_t n;
	static char base64_to_sixtet[256];
	static int tab_init = 0;
	byte* from = data;

	if( !tab_init )
	{
		memset( base64_to_sixtet, 0, 256 );
		for( i = 0; ( j = sixtet_to_base64[i] ) != '\0'; ++i )
		{
			base64_to_sixtet[j] = i;
		}
		tab_init = 1;
	}

	w = 0;
	i = 0;
	n = 0;
	byte in[4] = {0, 0, 0, 0};
	while( *from != '\0' && *from != '=' )
	{
		if( *from == ' ' || *from == '\n' )
		{
			++from;
			continue;
		}
		in[i] = base64_to_sixtet[* ( unsigned char* ) from];
		++i;
		++from;
		if( *from == '\0' || *from == '=' || i == 4 )
		{
			w = IntForSixtets( in );
			for( j = 0; j * 8 < i * 6; ++j )
			{
				*to++ = w & 0xff;
				++n;
				w >>= 8;
			}
			i = 0;
			w = 0;
		}
	}
	return n;
}

/*
============
idBase64::Encode
============
*/
void idBase64::Encode( const idStr& src )
{
	Encode( ( const byte* )src.c_str(), src.Length() );
}

/*
============
idBase64::Decode
============
*/
void idBase64::Decode( idStr& dest ) const
{
	byte* buf = new( TAG_IDLIB ) byte[ DecodeLength() + 1 ]; // +1 for trailing \0
	int out = Decode( buf );
	buf[out] = '\0';
	dest = ( const char* )buf;
	delete[] buf;
}

/*
============
idBase64::Decode
============
*/
void idBase64::Decode( idFile* dest ) const
{
	byte* buf = new( TAG_IDLIB ) byte[ DecodeLength() + 1 ]; // +1 for trailing \0
	int out = Decode( buf );
	dest->Write( buf, out );
	delete[] buf;
}

#if 0

void idBase64_TestBase64()
{

	idStr src;
	idBase64 dest;
	src = "Encode me in base64";
	dest.Encode( src );
	idLib::common->Printf( "%s -> %s\n", src.c_str(), dest.c_str() );
	dest.Decode( src );
	idLib::common->Printf( "%s -> %s\n", dest.c_str(), src.c_str() );

	idDict src_dict;
	src_dict.SetFloat( "float", 0.5f );
	src_dict.SetBool( "bool", true );
	src_dict.Set( "value", "foo" );
	idFile_Memory src_fmem( "serialize_dict" );
	src_dict.WriteToFileHandle( &src_fmem );
	dest.Encode( ( const byte* )src_fmem.GetDataPtr(), src_fmem.Length() );
	idLib::common->Printf( "idDict encoded to %s\n", dest.c_str() );

	// now decode to another stream and build back
	idFile_Memory dest_fmem( "build_back" );
	dest.Decode( &dest_fmem );
	dest_fmem.MakeReadOnly();
	idDict dest_dict;
	dest_dict.ReadFromFileHandle( &dest_fmem );
	idLib::common->Printf( "idDict reconstructed after base64 decode\n" );
	dest_dict.Print();

	// test idDict read from file - from python generated files, see idDict.py
	idFile* file = idLib::fileSystem->OpenFileRead( "idDict.test" );
	if( file )
	{
		idDict test_dict;
		test_dict.ReadFromFileHandle( file );
		//
		idLib::common->Printf( "read idDict.test:\n" );
		test_dict.Print();
		idLib::fileSystem->CloseFile( file );
		file = NULL;
	}
	else
	{
		idLib::common->Printf( "idDict.test not found\n" );
	}

	idBase64 base64_src;
	void* buffer;
	if( idLib::fileSystem->ReadFile( "idDict.base64.test", &buffer ) != -1 )
	{
		idFile_Memory mem_src( "dict" );
		idLib::common->Printf( "read: %d %s\n", idStr::Length( ( char* )buffer ), buffer );
		base64_src = ( char* )buffer;
		base64_src.Decode( &mem_src );
		mem_src.MakeReadOnly();
		idDict test_dict;
		test_dict.ReadFromFileHandle( &mem_src );
		idLib::common->Printf( "read idDict.base64.test:\n" );
		test_dict.Print();
		idLib::fileSystem->FreeFile( buffer );
	}
	else
	{
		idLib::common->Printf( "idDict.base64.test not found\n" );
	}
}

#endif
