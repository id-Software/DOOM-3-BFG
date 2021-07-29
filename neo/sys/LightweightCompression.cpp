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
#include "precompiled.h"
#pragma hdrstop
#include "LightweightCompression.h"

/*
========================
HashIndex
========================
*/
static int HashIndex( int w, int k )
{
	return ( w ^ k ) & idLZWCompressor::HASH_MASK;
}

/*
========================
idLZWCompressor::Start
========================
*/
void idLZWCompressor::Start( uint8* data_, int maxSize_, bool append )
{
	// Clear hash
	ClearHash();

	if( append )
	{
		assert( lzwData->nextCode > LZW_FIRST_CODE );

		int originalNextCode = lzwData->nextCode;

		lzwData->nextCode = LZW_FIRST_CODE;

		// If we are appending, then fill up the hash
		for( int i = LZW_FIRST_CODE; i < originalNextCode; i++ )
		{
			AddToDict( lzwData->dictionaryW[i], lzwData->dictionaryK[i] );
		}

		assert( originalNextCode == lzwData->nextCode );
	}
	else
	{
		for( int i = 0; i < LZW_FIRST_CODE; i++ )
		{
			lzwData->dictionaryK[i] = ( uint8 )i;
			lzwData->dictionaryW[i] = 0xFFFF;
		}

		lzwData->nextCode		= LZW_FIRST_CODE;
		lzwData->codeBits		= LZW_START_BITS;
		lzwData->codeWord		= -1;
		lzwData->tempValue		= 0;
		lzwData->tempBits		= 0;
		lzwData->bytesWritten	= 0;
	}

	oldCode		= -1;		// Used by DecompressBlock
	data		= data_;

	blockSize	= 0;
	blockIndex	= 0;

	bytesRead	= 0;

	maxSize		= maxSize_;
	overflowed	= false;

	savedBytesWritten	= 0;
	savedCodeWord		= 0;
	saveCodeBits		= 0;
	savedTempValue		= 0;
	savedTempBits		= 0;
}

/*
========================
idLZWCompressor::ReadBits
========================
*/
int idLZWCompressor::ReadBits( int bits )
{
	int bitsToRead = bits - lzwData->tempBits;

	while( bitsToRead > 0 )
	{
		if( bytesRead >= maxSize )
		{
			return -1;
		}
		lzwData->tempValue |= ( uint64 )data[bytesRead++] << lzwData->tempBits;
		lzwData->tempBits += 8;
		bitsToRead -= 8;
	}

	int value = ( int )lzwData->tempValue & ( ( 1 << bits ) - 1 );
	lzwData->tempValue >>= bits;
	lzwData->tempBits -= bits;

	return value;
}

/*
========================
idLZWCompressor::WriteBits
========================
*/
void idLZWCompressor::WriteBits( uint32 value, int bits )
{

	// Queue up bits into temp value
	lzwData->tempValue |= ( uint64 )value << lzwData->tempBits;
	lzwData->tempBits += bits;

	// Flush 8 bits (1 byte) at a time ( leftovers will get caught in idLZWCompressor::End() )
	while( lzwData->tempBits >= 8 )
	{
		if( lzwData->bytesWritten >= maxSize )
		{
			overflowed = true;
			return;
		}

		data[lzwData->bytesWritten++] = ( uint8 )( lzwData->tempValue & 255 );
		lzwData->tempValue >>= 8;
		lzwData->tempBits -= 8;
	}
}

/*
========================
idLZWCompressor::WriteChain

The chain is stored backwards, so we have to write it to a buffer then output the buffer in
reverse.
========================
*/
int idLZWCompressor::WriteChain( int code )
{
	byte chain[lzwCompressionData_t::LZW_DICT_SIZE];
	int firstChar = 0;
	int i = 0;
	do
	{
		assert( i < lzwCompressionData_t::LZW_DICT_SIZE && code < lzwCompressionData_t::LZW_DICT_SIZE && code >= 0 );
		chain[i++] = ( byte )lzwData->dictionaryK[code];
		code = lzwData->dictionaryW[code];
	}
	while( code != 0xFFFF );
	firstChar = chain[--i];
	for( ; i >= 0; i-- )
	{
		block[blockSize++] = chain[i];
	}
	return firstChar;
}

/*
========================
idLZWCompressor::DecompressBlock
========================
*/
void idLZWCompressor::DecompressBlock()
{
	assert( blockIndex == blockSize );		// Make sure we've read all we can

	blockIndex = 0;
	blockSize = 0;

	int firstChar = -1;
	while( blockSize < LZW_BLOCK_SIZE - lzwCompressionData_t::LZW_DICT_SIZE )
	{
		assert( lzwData->codeBits <= lzwCompressionData_t::LZW_DICT_BITS );

		int code = ReadBits( lzwData->codeBits );
		if( code == -1 )
		{
			break;
		}

		if( oldCode == -1 )
		{
			assert( code < 256 );
			block[blockSize++] = ( uint8 )code;
			oldCode = code;
			firstChar = code;
			continue;
		}

		if( code >= lzwData->nextCode )
		{
			assert( code == lzwData->nextCode );
			firstChar = WriteChain( oldCode );
			block[blockSize++] = ( uint8 )firstChar;
		}
		else
		{
			firstChar = WriteChain( code );
		}
		AddToDict( oldCode, firstChar );
		if( BumpBits() )
		{
			oldCode = -1;
		}
		else
		{
			oldCode = code;
		}
	}
}

/*
========================
idLZWCompressor::ReadByte
========================
*/
int idLZWCompressor::ReadByte( bool ignoreOverflow )
{
	if( blockIndex == blockSize )
	{
		DecompressBlock();
	}

	if( blockIndex == blockSize )    //-V581 DecompressBlock() updates these values, the if() isn't redundant
	{
		if( !ignoreOverflow )
		{
			overflowed = true;
			assert( !"idLZWCompressor::ReadByte overflowed!" );
		}
		return -1;
	}

	return block[blockIndex++];
}


/*
========================
idLZWCompressor::WriteByte
========================
*/
void idLZWCompressor::WriteByte( uint8 value )
{
	int code = Lookup( lzwData->codeWord, value );
	if( code >= 0 )
	{
		lzwData->codeWord = code;
	}
	else
	{
		WriteBits( lzwData->codeWord, lzwData->codeBits );
		if( !BumpBits() )
		{
			AddToDict( lzwData->codeWord, value );
		}
		lzwData->codeWord = value;
	}

	if( lzwData->bytesWritten >= maxSize - ( lzwData->codeBits + lzwData->tempBits + 7 ) / 8 )
	{
		overflowed = true;	// At any point, if we can't perform an End call, then trigger an overflow
		return;
	}
}

/*
========================
idLZWCompressor::Lookup
========================
*/
int idLZWCompressor::Lookup( int w, int k )
{
	if( w == -1 )
	{
		return k;
	}
	else
	{
		int i = HashIndex( w, k );

		for( int j = hash[i]; j != 0xFFFF; j = nextHash[j] )
		{
			assert( j < lzwCompressionData_t::LZW_DICT_SIZE );
			if( lzwData->dictionaryK[j] == k && lzwData->dictionaryW[j] == w )
			{
				return j;
			}
		}
	}
	return -1;
}

/*
========================
idLZWCompressor::AddToDict
========================
*/
int idLZWCompressor::AddToDict( int w, int k )
{
	assert( w < 0xFFFF - 1 );
	assert( k < 256 );
	assert( lzwData->nextCode < lzwCompressionData_t::LZW_DICT_SIZE );

	lzwData->dictionaryK[lzwData->nextCode] = ( uint8 )k;
	lzwData->dictionaryW[lzwData->nextCode] = ( uint16 )w;
	int i = HashIndex( w, k );
	nextHash[lzwData->nextCode] = hash[i];
	hash[i] = ( uint16 )lzwData->nextCode;
	return lzwData->nextCode++;
}

/*
========================
idLZWCompressor::BumpBits

Possibly increments codeBits.
	return: bool	- true, if the dictionary was cleared.
========================
*/
bool idLZWCompressor::BumpBits()
{
	if( lzwData->nextCode == ( 1 << lzwData->codeBits ) )
	{
		lzwData->codeBits ++;
		if( lzwData->codeBits > lzwCompressionData_t::LZW_DICT_BITS )
		{
			lzwData->nextCode = LZW_FIRST_CODE;
			lzwData->codeBits = LZW_START_BITS;
			ClearHash();
			return true;
		}
	}
	return false;
}

/*
========================
idLZWCompressor::End
========================
*/
int idLZWCompressor::End()
{
	assert( lzwData->tempBits < 8 );
	assert( lzwData->bytesWritten < maxSize - ( lzwData->codeBits + lzwData->tempBits + 7 ) / 8 );

	assert( ( Length() > 0 ) == ( lzwData->codeWord != -1 ) );

	if( lzwData->codeWord != -1 )
	{
		WriteBits( lzwData->codeWord, lzwData->codeBits );
	}

	if( lzwData->tempBits > 0 )
	{
		if( lzwData->bytesWritten >= maxSize )
		{
			overflowed = true;
			return -1;
		}
		data[lzwData->bytesWritten++] = ( uint8 )lzwData->tempValue & ( ( 1 << lzwData->tempBits ) - 1 );
	}

	return Length() > 0 ? Length() : -1;		// Total bytes written (or failure)
}

/*
========================
idLZWCompressor::Save
========================
*/
void idLZWCompressor::Save()
{
	assert( !overflowed );
	// Check and make sure we are at a good spot (can call End)
	assert( lzwData->bytesWritten < maxSize - ( lzwData->codeBits + lzwData->tempBits + 7 ) / 8 );

	savedBytesWritten	= lzwData->bytesWritten;
	savedCodeWord		= lzwData->codeWord;
	saveCodeBits		= lzwData->codeBits;
	savedTempValue		= lzwData->tempValue;
	savedTempBits		= lzwData->tempBits;
}

/*
========================
idLZWCompressor::Restore
========================
*/
void idLZWCompressor::Restore()
{
	lzwData->bytesWritten	= savedBytesWritten;
	lzwData->codeWord		= savedCodeWord;
	lzwData->codeBits		= saveCodeBits;
	lzwData->tempValue		= savedTempValue;
	lzwData->tempBits		= savedTempBits;
}

/*
========================
idLZWCompressor::ClearHash
========================
*/
void idLZWCompressor::ClearHash()
{
	memset( hash, 0xFF, sizeof( hash ) );
}

/*
========================
idZeroRunLengthCompressor
Simple zero based run length encoder/decoder
========================
*/

void idZeroRunLengthCompressor::Start( uint8* dest_, idLZWCompressor* comp_, int maxSize_ )
{
	zeroCount	= 0;
	dest		= dest_;
	comp		= comp_;
	compressed	= 0;
	maxSize		= maxSize_;
}

bool idZeroRunLengthCompressor::WriteRun()
{
	if( zeroCount > 0 )
	{
		assert( zeroCount <= 255 );
		if( compressed + 2 > maxSize )
		{
			maxSize = -1;
			return false;
		}
		if( comp != NULL )
		{
			comp->WriteByte( 0 );
			comp->WriteByte( ( uint8 )zeroCount );
		}
		else
		{
			*dest++ = 0;
			*dest++ = ( uint8 )zeroCount;
		}
		compressed += 2;
		zeroCount = 0;
	}
	return true;
}

bool idZeroRunLengthCompressor::WriteByte( uint8 value )
{
	if( value != 0 || zeroCount >= 255 )
	{
		if( !WriteRun() )
		{
			maxSize = -1;
			return false;
		}
	}

	if( value != 0 )
	{
		if( compressed + 1 > maxSize )
		{
			maxSize = -1;
			return false;
		}
		if( comp != NULL )
		{
			comp->WriteByte( value );
		}
		else
		{
			*dest++ = value;
		}
		compressed++;
	}
	else
	{
		zeroCount++;
	}

	return true;
}

byte idZeroRunLengthCompressor::ReadByte()
{
	// See if we need to possibly read more data
	if( zeroCount == 0 )
	{
		int value = ReadInternal();
		if( value == -1 )
		{
			assert( 0 );
		}
		if( value != 0 )
		{
			return ( byte )value;	// Return non zero values immediately
		}
		// Read the number of zeroes
		zeroCount = ReadInternal();
	}

	assert( zeroCount > 0 );

	zeroCount--;
	return 0;
}

void idZeroRunLengthCompressor::ReadBytes( byte* dest, int count )
{
	for( int i = 0; i < count; i++ )
	{
		*dest++ = ReadByte();
	}
}

void idZeroRunLengthCompressor::WriteBytes( uint8* src, int count )
{
	for( int i = 0; i < count; i++ )
	{
		WriteByte( *src++ );
	}
}

int idZeroRunLengthCompressor::End()
{
	WriteRun();
	if( maxSize == -1 )
	{
		return -1;
	}
	return compressed;
}

int idZeroRunLengthCompressor::ReadInternal()
{
	compressed++;
	if( comp != NULL )
	{
		return comp->ReadByte();
	}
	return *dest++;
}