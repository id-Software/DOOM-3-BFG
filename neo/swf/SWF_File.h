/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
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
#ifndef __SWF_FILE_H__
#define __SWF_FILE_H__

class idFile_SWF// : public idFile
{
public:
	// Constructor that accepts and stores the file pointer.
	idFile_SWF( idFile* _file )	: file( _file )
	{
		bitPos = 0;
		NBits = 0;
	}

	// Destructor that will destroy (close) the file when this wrapper class goes out of scope.
	~idFile_SWF();

	// Cast to a file pointer.
	operator idFile* () const
	{
		return file;
	}

	// Member access operator for treating the wrapper as if it were the file, itself.
	idFile* operator -> () const
	{
		return file;
	}

	void ByteAlign()
	{
		if( bitPos > 0 )
		{
			WriteByte( NBits );

			bitPos = 0;
			NBits = 0;
		}
	}


	static int		BitCountS( const int64 value, bool isSigned );
	static int		BitCountU( const int value );
	static int		BitCountFloat( const float value );

	static int		EnlargeBitCountS( const int value, int numBits );
	static int		EnlargeBitCountU( const int value, int numBits );

	virtual int		Write( const void* buffer, int len );

	void			WriteUBits( int value, int numBits );
	void			WriteSBits( int value, int numBits );

	void			WriteU8( uint8 value );
	void			WriteU16( uint16 value );
	void			WriteU32( uint32 value );

	void			WriteRect( const swfRect_t& rect );
	void			WriteMatrix( const swfMatrix_t& matrix );
	void			WriteColorRGB( const swfColorRGB_t& color );
	void			WriteColorRGBA( const swfColorRGBA_t& color );
	void			WriteColorXFormRGBA( const swfColorXform_t& xcf );

	static int32	GetTagHeaderSize( swfTag_t tag, int32 tagLength );
	void			WriteTagHeader( swfTag_t tag, int32 tagLength );

private:

	void			WriteByte( byte bits );

	idFile*			file;	// The managed file pointer.

	int				bitPos;
	int				NBits;

};



#endif // !__SWF_FILE_H__
