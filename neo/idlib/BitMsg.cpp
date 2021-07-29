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

/*
================================================================================================

	idBitMsg

================================================================================================
*/

/*
========================
idBitMsg::CheckOverflow
========================
*/
bool idBitMsg::CheckOverflow( int numBits )
{
	if( numBits > GetRemainingWriteBits() )
	{
		if( !allowOverflow )
		{
			idLib::FatalError( "idBitMsg: overflow without allowOverflow set; maxsize=%i size=%i numBits=%i numRemainingWriteBits=%i",
							   GetMaxSize(), GetSize(), numBits, GetRemainingWriteBits() );
		}
		if( numBits > ( maxSize << 3 ) )
		{
			idLib::FatalError( "idBitMsg: %i bits is > full message size", numBits );
		}
		idLib::Printf( "idBitMsg: overflow\n" );
		BeginWriting();
		overflowed = true;
		return true;
	}
	return false;
}

/*
========================
idBitMsg::GetByteSpace
========================
*/
byte* idBitMsg::GetByteSpace( int length )
{
	byte* ptr;

	if( !writeData )
	{
		idLib::FatalError( "idBitMsg::GetByteSpace: cannot write to message" );
	}

	// round up to the next byte
	WriteByteAlign();

	// check for overflow
	CheckOverflow( length << 3 );

	ptr = writeData + curSize;
	curSize += length;
	return ptr;
}

#define NBM( x ) (uint64)( ( 1LL << x ) - 1 )
static uint64 maskForNumBits64[33] = {	NBM( 0x00 ), NBM( 0x01 ), NBM( 0x02 ), NBM( 0x03 ),
										NBM( 0x04 ), NBM( 0x05 ), NBM( 0x06 ), NBM( 0x07 ),
										NBM( 0x08 ), NBM( 0x09 ), NBM( 0x0A ), NBM( 0x0B ),
										NBM( 0x0C ), NBM( 0x0D ), NBM( 0x0E ), NBM( 0x0F ),
										NBM( 0x10 ), NBM( 0x11 ), NBM( 0x12 ), NBM( 0x13 ),
										NBM( 0x14 ), NBM( 0x15 ), NBM( 0x16 ), NBM( 0x17 ),
										NBM( 0x18 ), NBM( 0x19 ), NBM( 0x1A ), NBM( 0x1B ),
										NBM( 0x1C ), NBM( 0x1D ), NBM( 0x1E ), NBM( 0x1F ), 0xFFFFFFFF
									 };

/*
========================
idBitMsg::WriteBits

If the number of bits is negative a sign is included.
========================
*/
void idBitMsg::WriteBits( int value, int numBits )
{
	if( !writeData )
	{
		idLib::FatalError( "idBitMsg::WriteBits: cannot write to message" );
	}

	// check if the number of bits is valid
	if( numBits == 0 || numBits < -31 || numBits > 32 )
	{
		idLib::FatalError( "idBitMsg::WriteBits: bad numBits %i", numBits );
	}

	// check for value overflows
	if( numBits != 32 )
	{
		if( numBits > 0 )
		{
			if( value > ( 1 << numBits ) - 1 )
			{
				idLib::FatalError( "idBitMsg::WriteBits: value overflow %d %d",
								   value, numBits );

			}
			else if( value < 0 )
			{
				idLib::FatalError( "idBitMsg::WriteBits: value overflow %d %d",
								   value, numBits );
			}
		}
		else
		{
			const unsigned shift = ( -1 - numBits );
			int r = 1 << shift;
			if( value > r - 1 )
			{
				idLib::FatalError( "idBitMsg::WriteBits: value overflow %d %d",
								   value, numBits );

			}
			else if( value < -r )
			{
				idLib::FatalError( "idBitMsg::WriteBits: value overflow %d %d",
								   value, numBits );
			}
		}
	}

	if( numBits < 0 )
	{
		numBits = -numBits;
	}

	// check for msg overflow
	if( CheckOverflow( numBits ) )
	{
		return;
	}

	// Merge value with possible previous leftover
	tempValue |= ( ( ( int64 )value ) & maskForNumBits64[numBits] ) << writeBit;

	writeBit += numBits;

	// Flush 8 bits (1 byte) at a time
	while( writeBit >= 8 )
	{
		writeData[curSize++] = tempValue & 255;
		tempValue >>= 8;
		writeBit -= 8;
	}

	// Write leftover now, in case this is the last WriteBits call
	if( writeBit > 0 )
	{
		writeData[curSize] = tempValue & 255;
	}
}

/*
========================
idBitMsg::WriteString
========================
*/
void idBitMsg::WriteString( const char* s, int maxLength, bool make7Bit )
{
	if( !s )
	{
		WriteData( "", 1 );
	}
	else
	{
		int i, l;
		byte* dataPtr;
		const byte* bytePtr;

		l = idStr::Length( s );
		if( maxLength >= 0 && l >= maxLength )
		{
			l = maxLength - 1;
		}
		dataPtr = GetByteSpace( l + 1 );
		bytePtr = reinterpret_cast< const byte* >( s );
		if( make7Bit )
		{
			for( i = 0; i < l; i++ )
			{
				if( bytePtr[i] > 127 )
				{
					dataPtr[i] = '.';
				}
				else
				{
					dataPtr[i] = bytePtr[i];
				}
			}
		}
		else
		{
			for( i = 0; i < l; i++ )
			{
				dataPtr[i] = bytePtr[i];
			}
		}
		dataPtr[i] = '\0';
	}
}

/*
========================
idBitMsg::WriteData
========================
*/
void idBitMsg::WriteData( const void* data, int length )
{
	memcpy( GetByteSpace( length ), data, length );
}

/*
========================
idBitMsg::WriteNetadr
========================
*/
void idBitMsg::WriteNetadr( const netadr_t adr )
{
	WriteData( adr.ip, 4 );
	WriteUShort( adr.port );
	WriteByte( adr.type );
}

/*
========================
idBitMsg::WriteDeltaDict
========================
*/
bool idBitMsg::WriteDeltaDict( const idDict& dict, const idDict* base )
{
	int i;
	const idKeyValue* kv, *basekv;
	bool changed = false;

	if( base != NULL )
	{

		for( i = 0; i < dict.GetNumKeyVals(); i++ )
		{
			kv = dict.GetKeyVal( i );
			basekv = base->FindKey( kv->GetKey() );
			if( basekv == NULL || basekv->GetValue().Icmp( kv->GetValue() ) != 0 )
			{
				WriteString( kv->GetKey() );
				WriteString( kv->GetValue() );
				changed = true;
			}
		}

		WriteString( "" );

		for( i = 0; i < base->GetNumKeyVals(); i++ )
		{
			basekv = base->GetKeyVal( i );
			kv = dict.FindKey( basekv->GetKey() );
			if( kv == NULL )
			{
				WriteString( basekv->GetKey() );
				changed = true;
			}
		}

		WriteString( "" );

	}
	else
	{

		for( i = 0; i < dict.GetNumKeyVals(); i++ )
		{
			kv = dict.GetKeyVal( i );
			WriteString( kv->GetKey() );
			WriteString( kv->GetValue() );
			changed = true;
		}
		WriteString( "" );

		WriteString( "" );

	}

	return changed;
}

/*
========================
idBitMsg::ReadBits

If the number of bits is negative a sign is included.
========================
*/
int idBitMsg::ReadBits( int numBits ) const
{
	int		value;
	int		valueBits;
	int		get;
	int		fraction;
	bool	sgn;

	if( !readData )
	{
		idLib::FatalError( "idBitMsg::ReadBits: cannot read from message" );
	}

	// check if the number of bits is valid
	if( numBits == 0 || numBits < -31 || numBits > 32 )
	{
		idLib::FatalError( "idBitMsg::ReadBits: bad numBits %i", numBits );
	}

	value = 0;
	valueBits = 0;

	if( numBits < 0 )
	{
		numBits = -numBits;
		sgn = true;
	}
	else
	{
		sgn = false;
	}

	// check for overflow
	if( numBits > GetRemainingReadBits() )
	{
		return -1;
	}

	while( valueBits < numBits )
	{
		if( readBit == 0 )
		{
			readCount++;
		}
		get = 8 - readBit;
		if( get > ( numBits - valueBits ) )
		{
			get = ( numBits - valueBits );
		}
		fraction = readData[readCount - 1];
		fraction >>= readBit;
		// this doesn't really do anything: the bits set to 0 here are already shifted to 0
		fraction &= ( 1 << get ) - 1;
		value |= fraction << valueBits;

		valueBits += get;
		readBit = ( readBit + get ) & 7;
	}

	if( sgn )
	{
		if( value & ( 1 << ( numBits - 1 ) ) )
		{
			value |= -1 ^ ( ( 1 << numBits ) - 1 );
		}
	}

	return value;
}

/*
========================
idBitMsg::ReadString
========================
*/
int idBitMsg::ReadString( char* buffer, int bufferSize ) const
{
	int	l, c;

	ReadByteAlign();
	l = 0;
	while( 1 )
	{
		c = ReadByte();
		if( c <= 0 || c >= 255 )
		{
			break;
		}
		// translate all fmt spec to avoid crash bugs in string routines
		if( c == '%' )
		{
			c = '.';
		}

		// we will read past any excessively long string, so
		// the following data can be read, but the string will
		// be truncated
		if( l < bufferSize - 1 )
		{
			buffer[l] = c;
			l++;
		}
	}

	buffer[l] = 0;
	return l;
}

/*
========================
idBitMsg::ReadString
========================
*/
int idBitMsg::ReadString( idStr& str ) const
{
	ReadByteAlign();

	int cnt = 0;
	for( int i = readCount; i < curSize; i++ )
	{
		if( readData[i] == 0 )
		{
			break;
		}
		cnt++;
	}

	str.Clear();
	str.Append( ( const char* )readData + readCount, cnt );
	readCount += cnt + 1;

	return str.Length();
}

/*
========================
idBitMsg::ReadData
========================
*/
int idBitMsg::ReadData( void* data, int length ) const
{
	int cnt;

	ReadByteAlign();
	cnt = readCount;

	if( readCount + length > curSize )
	{
		if( data )
		{
			memcpy( data, readData + readCount, GetRemainingData() );
		}
		readCount = curSize;
	}
	else
	{
		if( data )
		{
			memcpy( data, readData + readCount, length );
		}
		readCount += length;
	}

	return ( readCount - cnt );
}

/*
========================
idBitMsg::ReadNetadr
========================
*/
void idBitMsg::ReadNetadr( netadr_t* adr ) const
{
	ReadData( adr->ip, 4 );
	adr->port = ReadUShort();
	adr->type = ( netadrtype_t ) ReadByte();
}

/*
========================
idBitMsg::ReadDeltaDict
========================
*/
bool idBitMsg::ReadDeltaDict( idDict& dict, const idDict* base ) const
{
	char		key[MAX_STRING_CHARS];
	char		value[MAX_STRING_CHARS];
	bool		changed = false;

	if( base != NULL )
	{
		dict = *base;
	}
	else
	{
		dict.Clear();
	}

	while( ReadString( key, sizeof( key ) ) != 0 )
	{
		ReadString( value, sizeof( value ) );
		dict.Set( key, value );
		changed = true;
	}

	while( ReadString( key, sizeof( key ) ) != 0 )
	{
		dict.Delete( key );
		changed = true;
	}

	return changed;
}

/*
========================
idBitMsg::DirToBits
========================
*/
int idBitMsg::DirToBits( const idVec3& dir, int numBits )
{
	int max, bits;
	float bias;

	assert( numBits >= 6 && numBits <= 32 );
	assert( dir.LengthSqr() - 1.0f < 0.01f );

	numBits /= 3;
	max = ( 1 << ( numBits - 1 ) ) - 1;
	bias = 0.5f / max;

	bits = IEEE_FLT_SIGNBITSET( dir.x ) << ( numBits * 3 - 1 );
	bits |= ( idMath::Ftoi( ( idMath::Fabs( dir.x ) + bias ) * max ) ) << ( numBits * 2 );
	bits |= IEEE_FLT_SIGNBITSET( dir.y ) << ( numBits * 2 - 1 );
	bits |= ( idMath::Ftoi( ( idMath::Fabs( dir.y ) + bias ) * max ) ) << ( numBits * 1 );
	bits |= IEEE_FLT_SIGNBITSET( dir.z ) << ( numBits * 1 - 1 );
	bits |= ( idMath::Ftoi( ( idMath::Fabs( dir.z ) + bias ) * max ) ) << ( numBits * 0 );
	return bits;
}

/*
========================
idBitMsg::BitsToDir
========================
*/
idVec3 idBitMsg::BitsToDir( int bits, int numBits )
{
	static float sign[2] = { 1.0f, -1.0f };
	int max;
	float invMax;
	idVec3 dir;

	assert( numBits >= 6 && numBits <= 32 );

	numBits /= 3;
	max = ( 1 << ( numBits - 1 ) ) - 1;
	invMax = 1.0f / max;

	dir.x = sign[( bits >> ( numBits * 3 - 1 ) ) & 1] * ( ( bits >> ( numBits * 2 ) ) & max )
			* invMax;

	dir.y = sign[( bits >> ( numBits * 2 - 1 ) ) & 1] * ( ( bits >> ( numBits * 1 ) ) & max )
			* invMax;

	dir.z = sign[( bits >> ( numBits * 1 - 1 ) ) & 1] * ( ( bits >> ( numBits * 0 ) ) & max )
			* invMax;

	dir.NormalizeFast();
	return dir;
}
