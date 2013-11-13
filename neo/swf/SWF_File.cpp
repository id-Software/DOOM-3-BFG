/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 2013 Robert Beckebans

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
#pragma hdrstop
#include "precompiled.h"






idFile_SWF::~idFile_SWF()
{
	if( file != NULL )
	{
		delete file;
		file = NULL;
	}
}



int idFile_SWF::BitCountS( const int value )
{
	int count = 0;
	
#if 1
	count = idMath::BitCount( value );
#else
	int v = value;
	while( v > 0 )
	{
		if( ( v & 1 ) == 1 )
		{
			// lower bit is set
			count++;
		}
	
		// shift bits, remove lower bit
		v >>= 1;
	}
#endif
	
	return count;
}

int idFile_SWF::BitCountU( const int value )
{
	int nBits = idMath::BitCount( value );
	
	return nBits;
}

int idFile_SWF::BitCountFloat( const float value )
{
	int value2 = ( int ) value;
	
	int nBits = BitCountS( value2 );
	
	return nBits;
}

int idFile_SWF::EnlargeBitCountS( const int value, int numBits )
{
	int n = BitCountS( value );
	if( n > numBits )
	{
		numBits = n;
	}
	
	return numBits;
}

int idFile_SWF::EnlargeBitCountU( const int value, int numBits )
{
	int n = BitCountU( value );
	if( n > numBits )
	{
		numBits = n;
	}
	
	return numBits;
}




int	idFile_SWF::Write( const void* buffer, int len )
{
	ByteAlign();
	
	return file->Write( buffer, len );
}

void idFile_SWF::WriteByte( byte bits )
{
	file->WriteUnsignedChar( bits );
}

void idFile_SWF::WriteUBits( int value, int numBits )
{
	for( int bit = 0; bit < numBits; bit++ )
	{
		int nb = ( int )( ( value >> ( numBits - 1 - bit ) ) & 1 );
		
		NBits += nb * ( 1 << ( 7 - bitPos ) );
		bitPos++;
		
		if( bitPos == 8 )
		{
			WriteByte( NBits );
			
			bitPos = 0;
			NBits = 0;
		}
	}
}

void idFile_SWF::WriteSBits( int value, int numBits )
{
	WriteUBits( value, numBits );
}

void idFile_SWF::WriteU8( uint8 value )
{
	ByteAlign();
	
	WriteByte( value );
}

void idFile_SWF::WriteU16( uint16 value )
{
	ByteAlign();
	
	WriteByte( value & 0xFF );
	WriteByte( value >> 8 );
}

void idFile_SWF::WriteU32( uint32 value )
{
	ByteAlign();
	
	WriteByte( value & 0xFF );
	WriteByte( ( value >> 8 ) & 0xFF );
	WriteByte( ( value >> 16 ) & 0xFF );
	WriteByte( ( value >> 24 ) & 0xFF );
}

void idFile_SWF::WriteRect( const swfRect_t& rect )
{
	int tl_x = FLOAT2SWFTWIP( rect.tl.x );
	int br_x = FLOAT2SWFTWIP( rect.br.x );
	int tl_y = FLOAT2SWFTWIP( rect.tl.y );
	int br_y = FLOAT2SWFTWIP( rect.br.y );
	
	int nBits = 0;
	
	nBits = EnlargeBitCountS( tl_x, nBits );
	nBits = EnlargeBitCountS( br_x, nBits );
	nBits = EnlargeBitCountS( tl_y, nBits );
	nBits = EnlargeBitCountS( br_y, nBits );
	
	WriteUBits( nBits, 5 );
	WriteSBits( tl_x, nBits );
	WriteSBits( br_x, nBits );
	WriteSBits( tl_y, nBits );
	WriteSBits( br_y, nBits );
	
	ByteAlign();
}

void idFile_SWF::WriteMatrix( const swfMatrix_t& matrix )
{
	//ByteAlign();
	
	bool hasScale = ( matrix.xx != 1.0f || matrix.yy != 1.0f );
	WriteUBits( hasScale ? 1 : 0, 1 );
	
	if( hasScale )
	{
		int nBits = 0;
		
		int xx = FLOAT2SWFFIXED16( matrix.xx );
		int yy = FLOAT2SWFFIXED16( matrix.yy );
		
		nBits = EnlargeBitCountS( xx, nBits );
		nBits = EnlargeBitCountS( yy, nBits );
		
		WriteUBits( 5, nBits );
		WriteSBits( xx, nBits );
		WriteSBits( yy, nBits );
	}
	
	bool hasRotate = ( matrix.yx != 0 && matrix.xy != 0 );
	WriteUBits( hasRotate ? 1 : 0, 1 );
	
	if( hasRotate )
	{
		int nBits = 0;
		
		int yx = FLOAT2SWFFIXED16( matrix.yx );
		int xy = FLOAT2SWFFIXED16( matrix.xy );
		
		nBits = EnlargeBitCountS( yx, nBits );
		nBits = EnlargeBitCountS( xy, nBits );
		
		WriteUBits( 5, nBits );
		WriteSBits( yx, nBits );
		WriteSBits( xy, nBits );
	}
	
	int nBits = 0;
	int tx = FLOAT2SWFTWIP( matrix.tx );
	int ty = FLOAT2SWFTWIP( matrix.ty );
	
	nBits = EnlargeBitCountS( tx, nBits );
	nBits = EnlargeBitCountS( ty, nBits );
	
	WriteUBits( 5, nBits );
	WriteSBits( tx, nBits );
	WriteSBits( ty, nBits );
	
	ByteAlign();
}

void idFile_SWF::WriteColorRGB( const swfColorRGB_t& color )
{
	ByteAlign();
	
	WriteByte( color.r );
	WriteByte( color.g );
	WriteByte( color.b );
}

void idFile_SWF::WriteColorRGBA( const swfColorRGBA_t& color )
{
	ByteAlign();
	
	WriteByte( color.r );
	WriteByte( color.g );
	WriteByte( color.b );
	WriteByte( color.a );
}


void idFile_SWF::WriteTagHeader( swfTag_t tag, int32 tagLength )
{
	uint16 tagIDLength = ( ( int ) tag << 6 );
	
	if( tagLength < 0x3F )
	{
		tagIDLength += tagLength;
		WriteU16( tagIDLength );
	}
	else
	{
		tagIDLength += 0x3F;
		WriteU16( tagIDLength );
		WriteU32( tagLength );
	}
}

int32 idFile_SWF::GetTagHeaderSize( swfTag_t tag, int32 tagLength )
{
	int32 size = 2;
	
	if( tagLength >= 0x3F )
	{
		size = 6;
	}
	
	return size;
}


