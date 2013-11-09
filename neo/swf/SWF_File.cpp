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



//uint idFile_SWF::GetNumBits( int64 value, bool isSigned )
//{
//	//return idMath::BitCount(
//}

uint idFile_SWF::GetNumBitsInt( const int value )
{
	return idMath::BitCount( value );
}

int	idFile_SWF::Write( const void* buffer, int len )
{
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
	int nBits = rect.BitCount();
	
	int tl_x = PIXEL2SWFTWIP( rect.tl.x );
	int br_x = PIXEL2SWFTWIP( rect.br.x );
	int tl_y = PIXEL2SWFTWIP( rect.tl.y );
	int br_y = PIXEL2SWFTWIP( rect.br.y );
	
	WriteUBits( nBits, 5 );
	WriteSBits( tl_x, nBits );
	WriteSBits( br_x, nBits );
	WriteSBits( tl_y, nBits );
	WriteSBits( br_y, nBits );
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


