/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 2013-2015 Robert Beckebans

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






idFile_SWF::~idFile_SWF()
{
	if( file != NULL )
	{
		delete file;
		file = NULL;
	}
}



int idFile_SWF::BitCountS( const int64 value, bool isSigned )
{
	int number = idMath::Abs( value );

	int64 x = 1;
	int i;

	for( i = 1; i <= 64; i++ )
	{
		x <<= 1;
		if( x > number )
		{
			break;
		}
	}

	return ( int )( i + ( ( isSigned ) ? 1 : 0 ) );
}

int idFile_SWF::BitCountU( const int value )
{
	//int nBits = idMath::BitCount( value );
	int nBits = BitCountS( value, false );

	return nBits;
}

int idFile_SWF::BitCountFloat( const float value )
{
	int value2 = ( int ) value;

	int nBits = BitCountS( value2, value < 0.0F );

	return nBits;
}

int idFile_SWF::EnlargeBitCountS( const int value, int numBits )
{
	if( value  == 0 )
	{
		return numBits;
	}

	int n = BitCountS( value, true );
	if( n > numBits )
	{
		numBits = n;
	}

	return numBits;
}

int idFile_SWF::EnlargeBitCountU( const int value, int numBits )
{
	if( value  == 0 )
	{
		return numBits;
	}

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
			WriteByte( ( byte )( NBits & 0xFF ) );

			bitPos = 0;
			NBits = 0;
		}
	}
}

void idFile_SWF::WriteSBits( int value, int numBits )
{
	int32 tmp = value & 0x7FFFFFFF;

	if( value < 0 )
	{
		tmp |= ( 1L << ( ( int32 )numBits - 1 ) );
	}

	WriteUBits( tmp, numBits );
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
	WriteByte( ( value >> 8 ) & 0xFF );
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

		WriteUBits( nBits, 5 );
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

		WriteUBits( nBits, 5 );
		WriteSBits( yx, nBits );
		WriteSBits( xy, nBits );
	}

	int nBits = 0;
	int tx = FLOAT2SWFTWIP( matrix.tx );
	int ty = FLOAT2SWFTWIP( matrix.ty );

	nBits = EnlargeBitCountS( tx, nBits );
	nBits = EnlargeBitCountS( ty, nBits );

	WriteUBits( nBits, 5 );
	WriteSBits( tx, nBits );
	WriteSBits( ty, nBits );

	ByteAlign();
}

void idFile_SWF::WriteColorXFormRGBA( const swfColorXform_t& xcf )
{
	bool hasAddTerms = ( xcf.add.x != 0.0f || xcf.add.y != 0.0f || xcf.add.z != 0.0f || xcf.add.w != 0.0f );
	WriteUBits( hasAddTerms ? 1 : 0, 1 );

	bool hasMulTerms = ( xcf.mul.x != 0.0f || xcf.mul.y != 0.0f || xcf.mul.z != 0.0f || xcf.mul.w != 0.0f );
	WriteUBits( hasMulTerms ? 1 : 0, 1 );

	int nBits = 0;

	int mulRed = 0;
	int mulGreen = 0;
	int mulBlue = 0;
	int mulAlpha = 0;
	if( hasMulTerms )
	{
		mulRed = FLOAT2SWFFIXED8( xcf.mul.x );
		mulGreen = FLOAT2SWFFIXED8( xcf.mul.y );
		mulBlue = FLOAT2SWFFIXED8( xcf.mul.z );
		mulAlpha = FLOAT2SWFFIXED8( xcf.mul.w );

		nBits = EnlargeBitCountS( mulRed, nBits );
		nBits = EnlargeBitCountS( mulGreen, nBits );
		nBits = EnlargeBitCountS( mulBlue, nBits );
		nBits = EnlargeBitCountS( mulAlpha, nBits );
	}

	int red = 0;
	int green = 0;
	int blue = 0;
	int alpha = 0;
	if( hasAddTerms )
	{
		red = FLOAT2SWFFIXED8( xcf.add.x );
		green = FLOAT2SWFFIXED8( xcf.add.y );
		blue = FLOAT2SWFFIXED8( xcf.add.z );
		alpha = FLOAT2SWFFIXED8( xcf.add.w );

		nBits = EnlargeBitCountS( red, nBits );
		nBits = EnlargeBitCountS( green, nBits );
		nBits = EnlargeBitCountS( blue, nBits );
		nBits = EnlargeBitCountS( alpha, nBits );
	}

	WriteUBits( nBits, 4 );

	if( hasMulTerms )
	{
		WriteSBits( mulRed, nBits );
		WriteSBits( mulGreen, nBits );
		WriteSBits( mulBlue, nBits );
		WriteSBits( mulAlpha, nBits );
	}

	if( hasAddTerms )
	{
		WriteSBits( red, nBits );
		WriteSBits( green, nBits );
		WriteSBits( blue, nBits );
		WriteSBits( alpha, nBits );
	}

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


