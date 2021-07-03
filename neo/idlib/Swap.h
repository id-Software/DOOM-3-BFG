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
#ifndef __SWAP_H__
#define __SWAP_H__

/*
================================================================================================
Contains the Swap class, for CrossPlatform endian conversion.

works
================================================================================================
*/

/*
========================
IsPointer
========================
*/
template< typename type >
bool IsPointer( type )
{
	return false;
}

/*
========================
IsPointer
========================
*/
template< typename type >
bool IsPointer( type* )
{
	return true;
}

/*
================================================
The *Swap* static template class, idSwap, is used by the SwapClass template class for
performing EndianSwapping.
================================================
*/
class idSwap
{
public:
	//#define SwapBytes( x, y )		(x) ^= (y) ^= (x) ^= (y)
#define SwapBytes( x, y )		{ byte t = (x); (x) = (y); (y) = t; }

	template<class type> static void Little( type& c )
	{
		// byte swapping pointers is pointless because we should never store pointers on disk
		assert( !IsPointer( c ) );

	}

	template<class type> static void Big( type& c )
	{
		// byte swapping pointers is pointless because we should never store pointers on disk
		assert( !IsPointer( c ) );

		if( sizeof( type ) == 1 )
		{
		}
		else if( sizeof( type ) == 2 )
		{
			byte* b = ( byte* )&c;
			SwapBytes( b[0], b[1] );
		}
		else if( sizeof( type ) == 4 )
		{
			byte* b = ( byte* )&c;
			SwapBytes( b[0], b[3] );
			SwapBytes( b[1], b[2] );
		}
		else if( sizeof( type ) == 8 )
		{
			byte* b = ( byte* )&c;
			SwapBytes( b[0], b[7] );
			SwapBytes( b[1], b[6] );
			SwapBytes( b[2], b[5] );
			SwapBytes( b[3], b[4] );
		}
		else
		{
			assert( false );
		}
	}

	template<class type> static void LittleArray( type* c, int count )
	{
	}

	template<class type> static void BigArray( type* c, int count )
	{
		for( int i = 0; i < count; i++ )
		{
			Big( c[i] );
		}
	}

	static void SixtetsForInt( byte* out, int src )
	{
		byte* b = ( byte* )&src;
		out[0] = ( b[0] & 0xfc ) >> 2;
		out[1] = ( ( b[0] & 0x3 ) << 4 ) + ( ( b[1] & 0xf0 ) >> 4 );
		out[2] = ( ( b[1] & 0xf ) << 2 ) + ( ( b[2] & 0xc0 ) >> 6 );
		out[3] = b[2] & 0x3f;
	}

	static int IntForSixtets( byte* in )
	{
		int ret = 0;
		byte* b = ( byte* )&ret;
		b[0] |= in[0] << 2;
		b[0] |= ( in[1] & 0x30 ) >> 4;
		b[1] |= ( in[1] & 0xf ) << 4;
		b[1] |= ( in[2] & 0x3c ) >> 2;
		b[2] |= ( in[2] & 0x3 ) << 6;
		b[2] |= in[3];
		return ret;
	}

public:		// specializations
#ifndef ID_SWAP_LITE // avoid dependency avalanche for SPU code
#define SWAP_VECTOR( x ) \
	static void Little( x &c ) { LittleArray( c.ToFloatPtr(), c.GetDimension() ); } \
	static void Big( x &c ) {    BigArray( c.ToFloatPtr(), c.GetDimension() ); }

	SWAP_VECTOR( idVec2 );
	SWAP_VECTOR( idVec3 );
	SWAP_VECTOR( idVec4 );
	SWAP_VECTOR( idVec5 );
	SWAP_VECTOR( idVec6 );
	SWAP_VECTOR( idMat2 );
	SWAP_VECTOR( idMat3 );
	SWAP_VECTOR( idMat4 );
	SWAP_VECTOR( idMat5 );
	SWAP_VECTOR( idMat6 );
	SWAP_VECTOR( idPlane );
	SWAP_VECTOR( idQuat );
	SWAP_VECTOR( idCQuat );
	SWAP_VECTOR( idAngles );
	SWAP_VECTOR( idBounds );

	static void Little( idDrawVert& v )
	{
		Little( v.xyz );
		LittleArray( v.st, 2 );
		LittleArray( v.normal, 4 );
		LittleArray( v.tangent, 4 );
		LittleArray( v.color, 4 );
	}
	static void Big( idDrawVert& v )
	{
		Big( v.xyz );
		BigArray( v.st, 2 );
		BigArray( v.normal, 4 );
		BigArray( v.tangent, 4 );
		BigArray( v.color, 4 );
	}
#endif
};

/*
================================================
idSwapClass is a template class for performing EndianSwapping.
================================================
*/
template<class classType>
class idSwapClass
{
public:
	idSwapClass()
	{
#ifdef _DEBUG
		size = 0;
#endif
	}
	~idSwapClass()
	{
#ifdef _DEBUG
		assert( size == sizeof( classType ) );
#endif
	}

	template<class type> void Little( type& c )
	{
		idSwap::Little( c );
#ifdef _DEBUG
		size += sizeof( type );
#endif
	}

	template<class type> void Big( type& c )
	{
		idSwap::Big( c );
#ifdef _DEBUG
		size += sizeof( type );
#endif
	}

	template<class type> void LittleArray( type* c, int count )
	{
		idSwap::LittleArray( c, count );
#ifdef _DEBUG
		size += count * sizeof( type );
#endif
	}

	template<class type> void BigArray( type* c, int count )
	{
		idSwap::BigArray( c, count );
#ifdef _DEBUG
		size += count * sizeof( type );
#endif
	}

#ifdef _DEBUG
private:
	int size;
#endif
};

#define BIG32(v) ((((uint32)(v)) >> 24) | (((uint32)(v) & 0x00FF0000) >> 8) | (((uint32)(v) & 0x0000FF00) << 8) | ((uint32)(v) << 24))
#define BIG16(v) ((((uint16)(v)) >> 8) | ((uint16)(v) << 8))

#endif // !__SWAP_H__
