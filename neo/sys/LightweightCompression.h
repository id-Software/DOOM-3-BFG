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
#ifndef __LIGHTWEIGHT_COMPRESSION_H__
#define __LIGHTWEIGHT_COMPRESSION_H__


struct lzwCompressionData_t
{
	static const int	LZW_DICT_BITS	= 12;
	static const int	LZW_DICT_SIZE	= 1 << LZW_DICT_BITS;

	uint8					dictionaryK[LZW_DICT_SIZE];
	uint16					dictionaryW[LZW_DICT_SIZE];

	int						nextCode;
	int						codeBits;

	int						codeWord;

	uint64					tempValue;
	int						tempBits;
	int						bytesWritten;
};

/*
========================
idLZWCompressor
Simple lzw based encoder/decoder
========================
*/
class idLZWCompressor
{
public:
	idLZWCompressor( lzwCompressionData_t* lzwData_ ) : lzwData( lzwData_ ) {}

	static const int	LZW_BLOCK_SIZE	= ( 1 << 15 );
	static const int	LZW_START_BITS	= 9;
	static const int	LZW_FIRST_CODE	= ( 1 << ( LZW_START_BITS - 1 ) );

	void	Start( uint8* data_, int maxSize, bool append = false );
	int		ReadBits( int bits );
	int		WriteChain( int code );
	void	DecompressBlock();
	void	WriteBits( uint32 value, int bits );
	int		ReadByte( bool ignoreOverflow = false );
	void	WriteByte( uint8 value );
	int		Lookup( int w, int k );
	int		AddToDict( int w, int k );
	bool	BumpBits();
	int		End();

	int		Length() const
	{
		return lzwData->bytesWritten;
	}
	int		GetReadCount() const
	{
		return bytesRead;
	}

	void	Save();
	void	Restore();

	bool	IsOverflowed()
	{
		return overflowed;
	}

	int		Write( const void* data, int length )
	{
		uint8* src = ( uint8* )data;

		for( int i = 0; i < length && !IsOverflowed(); i++ )
		{
			WriteByte( src[i] );
		}

		return length;
	}

	int		Read( void* data, int length, bool ignoreOverflow = false )
	{
		uint8* src = ( uint8* )data;

		for( int i = 0; i < length; i++ )
		{
			int byte = ReadByte( ignoreOverflow );

			if( byte == -1 )
			{
				return i;
			}

			src[i] = ( uint8 )byte;
		}

		return length;
	}

	int		WriteR( const void* data, int length )
	{
		uint8* src = ( uint8* )data;

		for( int i = 0; i < length && !IsOverflowed(); i++ )
		{
			WriteByte( src[length - i - 1] );
		}

		return length;
	}

	int		ReadR( void* data, int length, bool ignoreOverflow = false )
	{
		uint8* src = ( uint8* )data;

		for( int i = 0; i < length; i++ )
		{
			int byte = ReadByte( ignoreOverflow );

			if( byte == -1 )
			{
				return i;
			}

			src[length - i - 1] = ( uint8 )byte;
		}

		return length;
	}

	template<class type> ID_INLINE size_t WriteAgnostic( const type& c )
	{
		return Write( &c, sizeof( c ) );
	}

	template<class type> ID_INLINE size_t ReadAgnostic( type& c, bool ignoreOverflow = false )
	{
		size_t r = Read( &c, sizeof( c ), ignoreOverflow );
		return r;
	}

	static const int DICTIONARY_HASH_BITS	= 10;
	static const int MAX_DICTIONARY_HASH	= 1 << DICTIONARY_HASH_BITS;
	static const int HASH_MASK				= MAX_DICTIONARY_HASH - 1;

private:
	void ClearHash();

	lzwCompressionData_t* 	lzwData;
	uint16					hash[MAX_DICTIONARY_HASH];
	uint16					nextHash[lzwCompressionData_t::LZW_DICT_SIZE];

	// Used by DecompressBlock
	int					oldCode;

	uint8* 				data;		// Read/write
	int					maxSize;
	bool				overflowed;

	// For reading
	int					bytesRead;
	uint8				block[LZW_BLOCK_SIZE];
	int					blockSize;
	int					blockIndex;

	// saving/restoring when overflow (when writing).
	// Must call End directly after restoring (dictionary is bad so can't keep writing)
	int					savedBytesWritten;
	int					savedCodeWord;
	int					saveCodeBits;
	uint64				savedTempValue;
	int					savedTempBits;
};

/*
========================
idZeroRunLengthCompressor
Simple zero based run length encoder/decoder
========================
*/
class idZeroRunLengthCompressor
{
public:
	idZeroRunLengthCompressor() : zeroCount( 0 ), destStart( NULL )
	{
	}

	void Start( uint8* dest_, idLZWCompressor* comp_, int maxSize_ );
	bool WriteRun();
	bool WriteByte( uint8 value );
	byte ReadByte();
	void ReadBytes( byte* dest, int count );
	void WriteBytes( uint8* src, int count );
	int End();

	int CompressedSize() const
	{
		return compressed;
	}

private:
	int ReadInternal();

	int					zeroCount;		// Number of pending zeroes
	idLZWCompressor* 	comp;
	uint8* 				destStart;
	uint8* 				dest;
	int					compressed;		// Compressed size
	int					maxSize;
};

#endif // __LIGHTWEIGHT_COMPRESSION_H__
