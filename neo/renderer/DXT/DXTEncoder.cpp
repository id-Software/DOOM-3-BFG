/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2014-2016 Kot in Action Creative Artel
Copyright (C) 2016-2017 Dustin Land
Copyright (C) 2014-2020 Robert Beckebans

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
#include "framework/Common_local.h"
#include "DXTCodec_local.h"
#include "DXTCodec.h"

#define INSET_COLOR_SHIFT		4		// inset the bounding box with ( range >> shift )
#define INSET_ALPHA_SHIFT		5		// inset alpha channel

#define C565_5_MASK				0xF8	// 0xFF minus last three bits
#define C565_6_MASK				0xFC	// 0xFF minus last two bits

#define NVIDIA_7X_HARDWARE_BUG_FIX		// keep the DXT5 colors sorted as: max, min

typedef uint16	word;
typedef uint32	dword;

// LordHavoc: macros required by gimp-dds code:
#ifndef MIN
	#ifdef __GNUC__
		#define MIN(a, b)  ({decltype(a) _a=(a); decltype(b) _b=(b); _a < _b ? _a : _b;})
	#else
		#define MIN(a, b)  ((a) < (b) ? (a) : (b))
	#endif
#endif

#define PUTL16( buf, s ) \
	( buf )[0] = ( ( s )      ) & 0xff; \
	( buf )[1] = ( ( s ) >> 8 ) & 0xff;

#define PUTL32( buf, l ) \
	( buf )[0] = ( ( l )       ) & 0xff; \
	( buf )[1] = ( ( l ) >>  8 ) & 0xff; \
	( buf )[2] = ( ( l ) >> 16 ) & 0xff; \
	( buf )[3] = ( ( l ) >> 24 ) & 0xff;

#define INSET_SHIFT  4

#define BLOCK_OFFSET( x, y, w, bs )  ( ( ( y ) >> 2 ) * ( ( bs ) * ( ( ( w ) + 3 ) >> 2 ) ) + ( ( bs ) * ( ( x ) >> 2 ) ) )

/*
========================
idDxtEncoder::NV4XHardwareBugFix
========================
*/
void idDxtEncoder::NV4XHardwareBugFix( byte* minColor, byte* maxColor ) const
{
	int minq = ( ( minColor[0] << 16 ) | ( minColor[1] << 8 ) | minColor[2] ) & 0x00F8FCF8;
	int maxq = ( ( maxColor[0] << 16 ) | ( maxColor[1] << 8 ) | maxColor[2] ) & 0x00F8FCF8;
	int mask = -( minq > maxq ) & 0x00FFFFFF;
	int min = *( int* )minColor;
	int max = *( int* )maxColor;
	min ^= max;
	max ^= ( min & mask );
	min ^= max;
	*( int* )minColor = min;
	*( int* )maxColor = max;
}

/*
========================
idDxtEncoder::HasConstantValuePer4x4Block
========================
*/
bool idDxtEncoder::HasConstantValuePer4x4Block( const byte* inBuf, int width, int height, int channel ) const
{
	if( width < 4 || height < 4 )
	{
		byte value = inBuf[channel];
		for( int k = 0; k < height; k++ )
		{
			for( int l = 0; l < width; l++ )
			{
				if( inBuf[( k * width + l ) * 4 + channel] != value )
				{
					return false;
				}
			}
		}
		return true;
	}

	for( int j = 0; j < height; j += 4, inBuf += width * 4 * 4 )
	{
		for( int i = 0; i < width; i += 4 )
		{
			const byte* inPtr = inBuf + i * 4;
			byte value = inPtr[channel];
			for( int k = 0; k < 4; k++ )
			{
				for( int l = 0; l < 4; l++ )
				{
					if( inPtr[( k * width + l ) * 4 + channel] != value )
					{
						return false;
					}
				}
			}
		}
		inBuf += srcPadding;
	}
	return true;
}

/*
========================
idDxtEncoder::WriteTinyColorDXT1
========================
*/
void idDxtEncoder::WriteTinyColorDXT1( const byte* inBuf, int width, int height )
{
	int numBlocks = ( ( width + 3 ) / 4 ) * ( ( height + 3 ) / 4 );
	int stride = ( ( width * height ) / numBlocks ) * 4;	// number of bytes from one block to the next
	// example: 2x8 pixels
	// numBlocks = 2
	// stride = 32 bytes (8 pixels)

	for( int i = 0; i < numBlocks; i++ )
	{
		// FIXME: This just emits a fake block based on the color at position 0,0
		EmitUShort( ColorTo565( inBuf ) );
		EmitUShort( 0 );	// dummy, never used
		EmitUInt( 0 );		// 4 color index bytes all use the first color

		inBuf += stride;
	}
}

/*
========================
idDxtEncoder::WriteTinyColorDXT5
========================
*/
void idDxtEncoder::WriteTinyColorDXT5( const byte* inBuf, int width, int height )
{
	int numBlocks = ( ( width + 3 ) / 4 ) * ( ( height + 3 ) / 4 );
	int stride = ( ( width * height ) / numBlocks ) * 4;	// number of bytes from one block to the next
	// example: 2x8 pixels
	// numBlocks = 2
	// stride = 32 bytes (8 pixels)

	for( int i = 0; i < numBlocks; i++ )
	{
		// FIXME: This just emits a fake block based on the color at position 0,0
		EmitByte( inBuf[3] );
		EmitByte( 0 );		// dummy, never used
		EmitByte( 0 );		// 6 alpha index bytes all use the first alpha
		EmitByte( 0 );
		EmitByte( 0 );
		EmitByte( 0 );
		EmitByte( 0 );
		EmitByte( 0 );

		EmitUShort( ColorTo565( inBuf ) );
		EmitUShort( 0 );	// dummy, never used
		EmitUInt( 0 );		// 4 color index bytes all use the first color

		inBuf += stride;
	}
}

/*
========================
idDxtEncoder::WriteTinyColorCTX1DXT5A
========================
*/
void idDxtEncoder::WriteTinyColorCTX1DXT5A( const byte* inBuf, int width, int height )
{
	int numBlocks = ( ( width + 3 ) / 4 ) * ( ( height + 3 ) / 4 );
	int stride = ( ( width * height ) / numBlocks ) * 4;	// number of bytes from one block to the next
	// example: 2x8 pixels
	// numBlocks = 2
	// stride = 32 bytes (8 pixels)

	for( int i = 0; i < numBlocks; i++ )
	{
		// FIXME: This just emits a fake block based on the color at position 0,0
		EmitByte( inBuf[0] );
		EmitByte( inBuf[1] );
		EmitByte( inBuf[0] );
		EmitByte( inBuf[1] );
		EmitUInt( 0 );		// 4 color index bytes all use the first color

		EmitByte( inBuf[3] );
		EmitByte( 0 );		// dummy, never used
		EmitByte( 0 );		// 6 alpha index bytes all use the first alpha
		EmitByte( 0 );
		EmitByte( 0 );
		EmitByte( 0 );
		EmitByte( 0 );
		EmitByte( 0 );

		inBuf += stride;
	}
}

/*
========================
idDxtEncoder::WriteTinyNormalMapDXT5
========================
*/
void idDxtEncoder::WriteTinyNormalMapDXT5( const byte* inBuf, int width, int height )
{
	int numBlocks = ( ( width + 3 ) / 4 ) * ( ( height + 3 ) / 4 );
	int stride = ( ( width * height ) / numBlocks ) * 4;	// number of bytes from one block to the next
	// example: 2x8 pixels
	// numBlocks = 2
	// stride = 32 bytes (8 pixels)

	for( int i = 0; i < numBlocks; i++ )
	{
		// FIXME: This just emits a fake block based on the normal at position 0,0
		EmitByte( inBuf[3] );
		EmitByte( 0 );		// dummy, never used
		EmitByte( 0 );		// 6 alpha index bytes all use the first alpha
		EmitByte( 0 );
		EmitByte( 0 );
		EmitByte( 0 );
		EmitByte( 0 );
		EmitByte( 0 );

		EmitUShort( ColorTo565( inBuf[0], inBuf[1], inBuf[2] ) );
		EmitUShort( 0 );	// dummy, never used
		EmitUInt( 0 );		// 4 color index bytes all use the first color

		inBuf += stride;
	}
}

/*
========================
idDxtEncoder::WriteTinyNormalMapDXN
========================
*/
void idDxtEncoder::WriteTinyNormalMapDXN( const byte* inBuf, int width, int height )
{
	int numBlocks = ( ( width + 3 ) / 4 ) * ( ( height + 3 ) / 4 );
	int stride = ( ( width * height ) / numBlocks ) * 4;	// number of bytes from one block to the next
	// example: 2x8 pixels
	// numBlocks = 2
	// stride = 32 bytes (8 pixels)

	for( int i = 0; i < numBlocks; i++ )
	{
		// FIXME: This just emits a fake block based on the normal at position 0,0
		EmitByte( inBuf[0] );
		EmitByte( 0 );		// dummy, never used
		EmitByte( 0 );		// 6 alpha index bytes all use the first alpha
		EmitByte( 0 );
		EmitByte( 0 );
		EmitByte( 0 );
		EmitByte( 0 );
		EmitByte( 0 );

		EmitByte( inBuf[1] );
		EmitByte( 0 );		// dummy, never used
		EmitByte( 0 );		// 6 alpha index bytes all use the first alpha
		EmitByte( 0 );
		EmitByte( 0 );
		EmitByte( 0 );
		EmitByte( 0 );
		EmitByte( 0 );

		inBuf += stride;
	}
}

/*
========================
idDxtEncoder::WriteTinyDXT5A
========================
*/
void idDxtEncoder::WriteTinyDXT5A( const byte* inBuf, int width, int height )
{
	int numBlocks = ( ( width + 3 ) / 4 ) * ( ( height + 3 ) / 4 );
	int stride = ( ( width * height ) / numBlocks ) * 4;	// number of bytes from one block to the next
	// example: 2x8 pixels
	// numBlocks = 2
	// stride = 32 bytes (8 pixels)

	for( int i = 0; i < numBlocks; i++ )
	{
		// FIXME: This just emits a fake block based on the normal at position 0,0
		EmitByte( inBuf[0] );
		EmitByte( 0 );		// dummy, never used
		EmitByte( 0 );		// 6 alpha index bytes all use the first alpha
		EmitByte( 0 );
		EmitByte( 0 );
		EmitByte( 0 );
		EmitByte( 0 );
		EmitByte( 0 );

		inBuf += stride;
	}
}

/*
========================
idDxtEncoder::ExtractBlock

params:	inPtr		- input image, 4 bytes per pixel
paramO:	colorBlock	- 4*4 output tile, 4 bytes per pixel
========================
*/
ID_INLINE void idDxtEncoder::ExtractBlock( const byte* inPtr, int width, byte* colorBlock ) const
{
	for( int j = 0; j < 4; j++ )
	{
		memcpy( &colorBlock[j * 4 * 4], inPtr, 4 * 4 );
		inPtr += width * 4;
	}
}

/*
========================
SwapColors
========================
*/
void SwapColors( byte* c1, byte* c2 )
{
	byte tm[3];
	memcpy( tm, c1, 3 );
	memcpy( c1, c2, 3 );
	memcpy( c2, tm, 3 );
}

/*
========================
idDxtEncoder::GetMinMaxColorsMaxDist

Finds the two RGB colors in a 4x4 block furthest apart. Also finds the two alpha values
furthest apart.

params: colorBlock	- 4*4 input tile, 4 bytes per pixel
paramO:	minColor	- 4 byte min color
paramO:	maxColor	- 4 byte max color
========================
*/
void idDxtEncoder::GetMinMaxColorsMaxDist( const byte* colorBlock, byte* minColor, byte* maxColor ) const
{
	int maxDistC = -1;
	int maxDistA = -1;

	for( int i = 0; i < 64 - 4; i += 4 )
	{
		for( int j = i + 4; j < 64; j += 4 )
		{
			int dc = ColorDistance( &colorBlock[i], &colorBlock[j] );
			if( dc > maxDistC )
			{
				maxDistC = dc;
				memcpy( minColor, colorBlock + i, 3 );
				memcpy( maxColor, colorBlock + j, 3 );
			}
			int da = AlphaDistance( colorBlock[i + 3], colorBlock[j + 3] );
			if( da > maxDistA )
			{
				maxDistA = da;
				minColor[3] = colorBlock[i + 3];
				maxColor[3] = colorBlock[j + 3];
			}
		}
	}
	if( maxColor[0] < minColor[0] )
	{
		SwapColors( minColor, maxColor );
	}
}

/*
========================
idDxtEncoder::GetMinMaxColorsLuminance

Finds the two RGB colors in a 4x4 block furthest apart based on luminance. Also finds the two
alpha values furthest apart.

params: colorBlock	- 4*4 input tile, 4 bytes per pixel
paramO:	minColor	- 4 byte min color
paramO:	maxColor	- 4 byte max color
========================
*/
void idDxtEncoder::GetMinMaxColorsLuminance( const byte* colorBlock, byte* minColor, byte* maxColor ) const
{
	int maxLumC = 0, minLumC = 256 * 4;
	int maxAlpha = 0, minAlpha = 256 * 4;

	for( int i = 0; i < 16; i++ )
	{
		int luminance = colorBlock[i * 4 + 0] + colorBlock[i * 4 + 1] * 2 + colorBlock[i * 4 + 2];
		if( luminance > maxLumC )
		{
			maxLumC = luminance;
			memcpy( maxColor, colorBlock + i * 4, 3 );
		}
		if( luminance < minLumC )
		{
			minLumC = luminance;
			memcpy( minColor, colorBlock + i * 4, 3 );
		}
		int alpha = colorBlock[i * 4 + 3];
		if( alpha > maxAlpha )
		{
			maxAlpha = alpha;
			maxColor[3] = ( byte )alpha;
		}
		if( alpha < minAlpha )
		{
			minAlpha = alpha;
			minColor[3] = ( byte )alpha;
		}
	}
	if( maxColor[0] < minColor[0] )
	{
		SwapColors( minColor, maxColor );
	}
}

/*
========================
idDxtEncoder::GetSquareAlphaError

params:	colorBlock	- 16 pixel block for which to find color indexes
paramO:	minAlpha	- Min alpha found
paramO:	maxAlpha	- Max alpha found
return: 4 byte color index block
========================
*/
int idDxtEncoder::GetSquareAlphaError( const byte* colorBlock, const int alphaOffset, const byte minAlpha, const byte maxAlpha, int lastError ) const
{
	int i, j;
	byte alphas[8];

	alphas[0] = maxAlpha;
	alphas[1] = minAlpha;

	if( maxAlpha > minAlpha )
	{
		alphas[2] = ( 6 * alphas[0] + 1 * alphas[1] ) / 7;
		alphas[3] = ( 5 * alphas[0] + 2 * alphas[1] ) / 7;
		alphas[4] = ( 4 * alphas[0] + 3 * alphas[1] ) / 7;
		alphas[5] = ( 3 * alphas[0] + 4 * alphas[1] ) / 7;
		alphas[6] = ( 2 * alphas[0] + 5 * alphas[1] ) / 7;
		alphas[7] = ( 1 * alphas[0] + 6 * alphas[1] ) / 7;
	}
	else
	{
		alphas[2] = ( 4 * alphas[0] + 1 * alphas[1] ) / 5;
		alphas[3] = ( 3 * alphas[0] + 2 * alphas[1] ) / 5;
		alphas[4] = ( 2 * alphas[0] + 3 * alphas[1] ) / 5;
		alphas[5] = ( 1 * alphas[0] + 4 * alphas[1] ) / 5;
		alphas[6] = 0;
		alphas[7] = 255;
	}

	int error = 0;
	for( i = 0; i < 16; i++ )
	{
		unsigned int minDist = MAX_UNSIGNED_TYPE( int );
		byte a = colorBlock[i * 4 + alphaOffset];
		for( j = 0; j < 8; j++ )
		{
			unsigned int dist = AlphaDistance( a, alphas[j] );
			if( dist < minDist )
			{
				minDist = dist;
			}
		}
		error += minDist;

		if( error >= lastError )
		{
			return error;
		}
	}

	return error;
}

/*
========================
idDxtEncoder::GetMinMaxAlphaHQ

params:	colorBlock	- 4*4 input tile, 4 bytes per pixel
paramO:	minColor		- 4 byte min color found
paramO:	maxColor		- 4 byte max color found
========================
*/
int idDxtEncoder::GetMinMaxAlphaHQ( const byte* colorBlock, const int alphaOffset, byte* minColor, byte* maxColor ) const
{
	int i, j;
	byte alphaMin, alphaMax;
	int error, bestError = MAX_TYPE( int );

	alphaMin = 255;
	alphaMax = 0;

	// get alpha min / max
	for( i = 0; i < 16; i++ )
	{
		if( colorBlock[i * 4 + alphaOffset] < alphaMin )
		{
			alphaMin = colorBlock[i * 4 + alphaOffset];
		}
		if( colorBlock[i * 4 + alphaOffset] > alphaMax )
		{
			alphaMax = colorBlock[i * 4 + alphaOffset];
		}
	}

	const int ALPHA_EXPAND = 32;

	alphaMin = ( alphaMin <= ALPHA_EXPAND ) ? 0 : alphaMin - ALPHA_EXPAND;
	alphaMax = ( alphaMax >= 255 - ALPHA_EXPAND ) ? 255 : alphaMax + ALPHA_EXPAND;

	for( i = alphaMin; i <= alphaMax; i++ )
	{
		for( j = alphaMax; j >= i; j-- )
		{

			error = GetSquareAlphaError( colorBlock, alphaOffset, ( byte )i, ( byte )j, bestError );
			if( error < bestError )
			{
				bestError = error;
				minColor[alphaOffset] = ( byte )i;
				maxColor[alphaOffset] = ( byte )j;
			}

			error = GetSquareAlphaError( colorBlock, alphaOffset, ( byte )j, ( byte )i, bestError );
			if( error < bestError )
			{
				bestError = error;
				minColor[alphaOffset] = ( byte )i;
				maxColor[alphaOffset] = ( byte )j;
			}
		}
	}

	return bestError;
}

/*
========================
idDxtEncoder::GetSquareColorsError

params:	colorBlock	- 16 pixel block for which to find color indexes
paramO:	color0		- 4 byte min color found
paramO:	color1		- 4 byte max color found
return: 4 byte color index block
========================
*/
int idDxtEncoder::GetSquareColorsError( const byte* colorBlock, const unsigned short color0, const unsigned short color1, int lastError ) const
{
	int i, j;
	byte colors[4][4];

	ColorFrom565( color0, colors[0] );
	ColorFrom565( color1, colors[1] );

	if( color0 > color1 )
	{
		colors[2][0] = ( 2 * colors[0][0] + 1 * colors[1][0] ) / 3;
		colors[2][1] = ( 2 * colors[0][1] + 1 * colors[1][1] ) / 3;
		colors[2][2] = ( 2 * colors[0][2] + 1 * colors[1][2] ) / 3;
		colors[3][0] = ( 1 * colors[0][0] + 2 * colors[1][0] ) / 3;
		colors[3][1] = ( 1 * colors[0][1] + 2 * colors[1][1] ) / 3;
		colors[3][2] = ( 1 * colors[0][2] + 2 * colors[1][2] ) / 3;
	}
	else
	{
		colors[2][0] = ( 1 * colors[0][0] + 1 * colors[1][0] ) / 2;
		colors[2][1] = ( 1 * colors[0][1] + 1 * colors[1][1] ) / 2;
		colors[2][2] = ( 1 * colors[0][2] + 1 * colors[1][2] ) / 2;
		colors[3][0] = 0;
		colors[3][1] = 0;
		colors[3][2] = 0;
	}

	int error = 0;
	for( i = 0; i < 16; i++ )
	{
		unsigned int minDist = MAX_UNSIGNED_TYPE( int );
		for( j = 0; j < 4; j++ )
		{
			unsigned int dist = ColorDistance( &colorBlock[i * 4], &colors[j][0] );
			if( dist < minDist )
			{
				minDist = dist;
			}
		}
		// accumulated error
		error += minDist;

		if( error > lastError )
		{
			return error;
		}
	}
	return error;
}

/*
========================
idDxtEncoder::GetSquareNormalYError

params:	colorBlock	- 16 pixel block for which to find color indexes
paramO:	color0		- 4 byte min color found
paramO:	color1		- 4 byte max color found
return: 4 byte color index block
========================
*/
int idDxtEncoder::GetSquareNormalYError( const byte* colorBlock, const unsigned short color0, const unsigned short color1, int lastError, int scale ) const
{
	int i, j;
	byte colors[4][4];

	ColorFrom565( color0, colors[0] );
	ColorFrom565( color1, colors[1] );

	if( color0 > color1 )
	{
		colors[2][0] = ( 2 * colors[0][0] + 1 * colors[1][0] ) / 3;
		colors[2][1] = ( 2 * colors[0][1] + 1 * colors[1][1] ) / 3;
		colors[2][2] = ( 2 * colors[0][2] + 1 * colors[1][2] ) / 3;
		colors[3][0] = ( 1 * colors[0][0] + 2 * colors[1][0] ) / 3;
		colors[3][1] = ( 1 * colors[0][1] + 2 * colors[1][1] ) / 3;
		colors[3][2] = ( 1 * colors[0][2] + 2 * colors[1][2] ) / 3;
	}
	else
	{
		colors[2][0] = ( 1 * colors[0][0] + 1 * colors[1][0] ) / 2;
		colors[2][1] = ( 1 * colors[0][1] + 1 * colors[1][1] ) / 2;
		colors[2][2] = ( 1 * colors[0][2] + 1 * colors[1][2] ) / 2;
		colors[3][0] = 0;
		colors[3][1] = 0;
		colors[3][2] = 0;
	}

	int error = 0;
	for( i = 0; i < 16; i++ )
	{
		unsigned int minDist = MAX_UNSIGNED_TYPE( int );
		for( j = 0; j < 4; j++ )
		{
			float r = ( float ) colorBlock[i * 4 + 1] / scale;
			float s = ( float ) colors[j][1] / scale;
			unsigned int dist = idMath::Ftoi( ( r - s ) * ( r - s ) );
			if( dist < minDist )
			{
				minDist = dist;
			}
		}
		// accumulated error
		error += minDist;

		if( error > lastError )
		{
			return error;
		}
	}
	return error;
}

/*
========================
idDxtEncoder::GetMinMaxColorsHQ

Uses an exhaustive search to find the two RGB colors that produce the least error when used to
compress the 4x4 block. Also finds the minimum and maximum alpha values.

params:	colorBlock	- 4*4 input tile, 4 bytes per pixel
paramO:	minColor	- 4 byte min color found
paramO:	maxColor	- 4 byte max color found
========================
*/
int idDxtEncoder::GetMinMaxColorsHQ( const byte* colorBlock, byte* minColor, byte* maxColor, bool noBlack ) const
{
	int i;
	int i0, i1, i2, j0, j1, j2;
	unsigned short minColor565, maxColor565, bestMinColor565, bestMaxColor565;
	byte bboxMin[3], bboxMax[3], minAxisDist[3];
	int error, bestError = MAX_TYPE( int );

	bboxMin[0] = bboxMin[1] = bboxMin[2] = 255;
	bboxMax[0] = bboxMax[1] = bboxMax[2] = 0;

	// get color bbox
	for( i = 0; i < 16; i++ )
	{
		if( colorBlock[i * 4 + 0] < bboxMin[0] )
		{
			bboxMin[0] = colorBlock[i * 4 + 0];
		}
		if( colorBlock[i * 4 + 1] < bboxMin[1] )
		{
			bboxMin[1] = colorBlock[i * 4 + 1];
		}
		if( colorBlock[i * 4 + 2] < bboxMin[2] )
		{
			bboxMin[2] = colorBlock[i * 4 + 2];
		}
		if( colorBlock[i * 4 + 0] > bboxMax[0] )
		{
			bboxMax[0] = colorBlock[i * 4 + 0];
		}
		if( colorBlock[i * 4 + 1] > bboxMax[1] )
		{
			bboxMax[1] = colorBlock[i * 4 + 1];
		}
		if( colorBlock[i * 4 + 2] > bboxMax[2] )
		{
			bboxMax[2] = colorBlock[i * 4 + 2];
		}
	}

	// decrease range for 565 encoding
	bboxMin[0] >>= 3;
	bboxMin[1] >>= 2;
	bboxMin[2] >>= 3;
	bboxMax[0] >>= 3;
	bboxMax[1] >>= 2;
	bboxMax[2] >>= 3;

	// get the minimum distance the end points of the line must be apart along each axis
	for( i = 0; i < 3; i++ )
	{
		minAxisDist[i] = ( bboxMax[i] - bboxMin[i] );
		if( minAxisDist[i] >= 16 )
		{
			minAxisDist[i] = minAxisDist[i] * 3 / 4;
		}
		else if( minAxisDist[i] >= 8 )
		{
			minAxisDist[i] = minAxisDist[i] * 2 / 4;
		}
		else if( minAxisDist[i] >= 4 )
		{
			minAxisDist[i] = minAxisDist[i] * 1 / 4;
		}
		else
		{
			minAxisDist[i] = 0;
		}
	}

	// expand the bounding box
	const int C565_BBOX_EXPAND = 1;

	bboxMin[0] = ( bboxMin[0] <= C565_BBOX_EXPAND ) ? 0 : bboxMin[0] - C565_BBOX_EXPAND;
	bboxMin[1] = ( bboxMin[1] <= C565_BBOX_EXPAND ) ? 0 : bboxMin[1] - C565_BBOX_EXPAND;
	bboxMin[2] = ( bboxMin[2] <= C565_BBOX_EXPAND ) ? 0 : bboxMin[2] - C565_BBOX_EXPAND;
	bboxMax[0] = ( bboxMax[0] >= ( 255 >> 3 ) - C565_BBOX_EXPAND ) ? ( 255 >> 3 ) : bboxMax[0] + C565_BBOX_EXPAND;
	bboxMax[1] = ( bboxMax[1] >= ( 255 >> 2 ) - C565_BBOX_EXPAND ) ? ( 255 >> 2 ) : bboxMax[1] + C565_BBOX_EXPAND;
	bboxMax[2] = ( bboxMax[2] >= ( 255 >> 3 ) - C565_BBOX_EXPAND ) ? ( 255 >> 3 ) : bboxMax[2] + C565_BBOX_EXPAND;

	bestMinColor565 = 0;
	bestMaxColor565 = 0;

	for( i0 = bboxMin[0]; i0 <= bboxMax[0]; i0++ )
	{
		for( j0 = bboxMax[0]; j0 >= bboxMin[0]; j0-- )
		{
			if( abs( i0 - j0 ) < minAxisDist[0] )
			{
				continue;
			}

			for( i1 = bboxMin[1]; i1 <= bboxMax[1]; i1++ )
			{
				for( j1 = bboxMax[1]; j1 >= bboxMin[1]; j1-- )
				{
					if( abs( i1 - j1 ) < minAxisDist[1] )
					{
						continue;
					}

					for( i2 = bboxMin[2]; i2 <= bboxMax[2]; i2++ )
					{
						for( j2 = bboxMax[2]; j2 >= bboxMin[2]; j2-- )
						{
							if( abs( i2 - j2 ) < minAxisDist[2] )
							{
								continue;
							}

							minColor565 = ( unsigned short )( ( i0 << 11 ) | ( i1 << 5 ) | ( i2 << 0 ) );
							maxColor565 = ( unsigned short )( ( j0 << 11 ) | ( j1 << 5 ) | ( j2 << 0 ) );

							if( !noBlack )
							{
								error = GetSquareColorsError( colorBlock, maxColor565, minColor565, bestError );
								if( error < bestError )
								{
									bestError = error;
									bestMinColor565 = minColor565;
									bestMaxColor565 = maxColor565;
								}
							}
							else
							{
								if( minColor565 <= maxColor565 )
								{
									SwapValues( minColor565, maxColor565 );
								}
							}

							error = GetSquareColorsError( colorBlock, minColor565, maxColor565, bestError );
							if( error < bestError )
							{
								bestError = error;
								bestMinColor565 = minColor565;
								bestMaxColor565 = maxColor565;
							}
						}
					}
				}
			}
		}
	}

	ColorFrom565( bestMinColor565, minColor );
	ColorFrom565( bestMaxColor565, maxColor );

	return bestError;
}

/*
========================
idDxtEncoder::GetSquareCTX1Error

params:	colorBlock	- 16 pixel block for which to find color indexes
paramO:	color0		- Min color found
paramO:	color1		- Max color found
return: 4 byte color index block
========================
*/
int idDxtEncoder::GetSquareCTX1Error( const byte* colorBlock, const byte* color0, const byte* color1, int lastError ) const
{
	int i, j;
	byte colors[4][4];

	colors[0][0] = color0[0];
	colors[0][1] = color0[1];
	colors[1][0] = color1[0];
	colors[1][1] = color1[1];

	colors[2][0] = ( 2 * colors[0][0] + 1 * colors[1][0] ) / 3;
	colors[2][1] = ( 2 * colors[0][1] + 1 * colors[1][1] ) / 3;
	colors[3][0] = ( 1 * colors[0][0] + 2 * colors[1][0] ) / 3;
	colors[3][1] = ( 1 * colors[0][1] + 2 * colors[1][1] ) / 3;

	int error = 0;
	for( i = 0; i < 16; i++ )
	{
		unsigned int minDist = MAX_UNSIGNED_TYPE( int );
		for( j = 0; j < 4; j++ )
		{
			unsigned int dist = CTX1Distance( &colorBlock[i * 4], &colors[j][0] );
			if( dist < minDist )
			{
				minDist = dist;
			}
		}
		// accumulated error
		error += minDist;

		if( error > lastError )
		{
			return error;
		}
	}
	return error;
}

/*
========================
idDxtEncoder::GetMinMaxCTX1HQ

Uses an exhaustive search to find the two RGB colors that produce the least error when used to
compress the 4x4 block. Also finds the minimum and maximum alpha values.

params:	colorBlock	- 4*4 input tile, 4 bytes per pixel
paramO:	minColor	- 4 byte Min color found
paramO:	maxColor	- 4 byte Max color found
========================
*/
int idDxtEncoder::GetMinMaxCTX1HQ( const byte* colorBlock, byte* minColor, byte* maxColor ) const
{
	int i;
	int i0, i1, j0, j1;
	byte curMinColor[2], curMaxColor[2];
	byte bboxMin[2], bboxMax[2], minAxisDist[2];
	int error, bestError = MAX_TYPE( int );

	bboxMin[0] = bboxMin[1] = 255;
	bboxMax[0] = bboxMax[1] = 0;

	// get color bbox
	for( i = 0; i < 16; i++ )
	{
		if( colorBlock[i * 4 + 0] < bboxMin[0] )
		{
			bboxMin[0] = colorBlock[i * 4 + 0];
		}
		if( colorBlock[i * 4 + 1] < bboxMin[1] )
		{
			bboxMin[1] = colorBlock[i * 4 + 1];
		}
		if( colorBlock[i * 4 + 0] > bboxMax[0] )
		{
			bboxMax[0] = colorBlock[i * 4 + 0];
		}
		if( colorBlock[i * 4 + 1] > bboxMax[1] )
		{
			bboxMax[1] = colorBlock[i * 4 + 1];
		}
	}

	// get the minimum distance the end points of the line must be apart along each axis
	for( i = 0; i < 2; i++ )
	{
		minAxisDist[i] = ( bboxMax[i] - bboxMin[i] );
		if( minAxisDist[i] >= 64 )
		{
			minAxisDist[i] = minAxisDist[i] * 3 / 4;
		}
		else if( minAxisDist[i] >= 32 )
		{
			minAxisDist[i] = minAxisDist[i] * 2 / 4;
		}
		else if( minAxisDist[i] >= 16 )
		{
			minAxisDist[i] = minAxisDist[i] * 1 / 4;
		}
		else
		{
			minAxisDist[i] = 0;
		}
	}

	// expand the bounding box
	const int CXT1_BBOX_EXPAND = 6;

	bboxMin[0] = ( bboxMin[0] <= CXT1_BBOX_EXPAND ) ? 0 : bboxMin[0] - CXT1_BBOX_EXPAND;
	bboxMin[1] = ( bboxMin[1] <= CXT1_BBOX_EXPAND ) ? 0 : bboxMin[1] - CXT1_BBOX_EXPAND;
	bboxMax[0] = ( bboxMax[0] >= 255 - CXT1_BBOX_EXPAND ) ? 255 : bboxMax[0] + CXT1_BBOX_EXPAND;
	bboxMax[1] = ( bboxMax[1] >= 255 - CXT1_BBOX_EXPAND ) ? 255 : bboxMax[1] + CXT1_BBOX_EXPAND;

	for( i0 = bboxMin[0]; i0 <= bboxMax[0]; i0++ )
	{
		for( j0 = bboxMax[0]; j0 >= bboxMin[0]; j0-- )
		{
			if( abs( i0 - j0 ) < minAxisDist[0] )
			{
				continue;
			}

			for( i1 = bboxMin[1]; i1 <= bboxMax[1]; i1++ )
			{
				for( j1 = bboxMax[1]; j1 >= bboxMin[1]; j1-- )
				{
					if( abs( i1 - j1 ) < minAxisDist[1] )
					{
						continue;
					}

					curMinColor[0] = ( byte )i0;
					curMinColor[1] = ( byte )i1;

					curMaxColor[0] = ( byte )j0;
					curMaxColor[1] = ( byte )j1;

					error = GetSquareCTX1Error( colorBlock, curMinColor, curMaxColor, bestError );
					if( error < bestError )
					{
						bestError = error;
						memcpy( minColor, curMinColor, 2 );
						memcpy( maxColor, curMaxColor, 2 );
					}
				}
			}
		}
	}

	return bestError;
}

/*
========================
idDxtEncoder::GetMinMaxNormalYHQ

Uses an exhaustive search to find the two RGB colors that produce the least error when used to
compress the 4x4 block. Also finds the minimum and maximum alpha values.

params:	colorBlock	- 4*4 input tile, 4 bytes per pixel
paramO:	minColor	- 4 byte Min color found
paramO:	maxColor	- 4 byte Max color found
========================
*/
int idDxtEncoder::GetMinMaxNormalYHQ( const byte* colorBlock, byte* minColor, byte* maxColor, bool noBlack, int scale ) const
{
	unsigned short bestMinColor565, bestMaxColor565;
	byte bboxMin[3], bboxMax[3];
	int error, bestError = MAX_TYPE( int );

	bboxMin[1] = 255;
	bboxMax[1] = 0;

	// get color bbox
	for( int i = 0; i < 16; i++ )
	{
		if( colorBlock[i * 4 + 1] < bboxMin[1] )
		{
			bboxMin[1] = colorBlock[i * 4 + 1];
		}
		if( colorBlock[i * 4 + 1] > bboxMax[1] )
		{
			bboxMax[1] = colorBlock[i * 4 + 1];
		}
	}

	// decrease range for 565 encoding
	bboxMin[1] >>= 2;
	bboxMax[1] >>= 2;

	// expand the bounding box
	const int C565_BBOX_EXPAND = 1;

	bboxMin[1] = ( bboxMin[1] <= C565_BBOX_EXPAND ) ? 0 : bboxMin[1] - C565_BBOX_EXPAND;
	bboxMax[1] = ( bboxMax[1] >= ( 255 >> 2 ) - C565_BBOX_EXPAND ) ? ( 255 >> 2 ) : bboxMax[1] + C565_BBOX_EXPAND;

	bestMinColor565 = 0;
	bestMaxColor565 = 0;

	for( int i1 = bboxMin[1]; i1 <= bboxMax[1]; i1++ )
	{
		for( int j1 = bboxMax[1]; j1 >= bboxMin[1]; j1-- )
		{
			if( abs( i1 - j1 ) < 0 )
			{
				continue;
			}

			unsigned short minColor565 = ( unsigned short )i1 << 5;
			unsigned short maxColor565 = ( unsigned short )j1 << 5;

			if( !noBlack )
			{
				error = GetSquareNormalYError( colorBlock, maxColor565, minColor565, bestError, scale );
				if( error < bestError )
				{
					bestError = error;
					bestMinColor565 = minColor565;
					bestMaxColor565 = maxColor565;
				}
			}
			else
			{
				if( minColor565 <= maxColor565 )
				{
					SwapValues( minColor565, maxColor565 );
				}
			}

			error = GetSquareNormalYError( colorBlock, minColor565, maxColor565, bestError, scale );
			if( error < bestError )
			{
				bestError = error;
				bestMinColor565 = minColor565;
				bestMaxColor565 = maxColor565;
			}
		}
	}

	ColorFrom565( bestMinColor565, minColor );
	ColorFrom565( bestMaxColor565, maxColor );

	int bias = colorBlock[0 * 4 + 0];
	int size = colorBlock[0 * 4 + 2];

	minColor[0] = maxColor[0] = ( byte )bias;
	minColor[2] = maxColor[2] = ( byte )size;

	return bestError;
}

ALIGN16( static float SIMD_SSE2_float_scale[4] ) = { 2.0f / 255.0f, 2.0f / 255.0f, 2.0f / 255.0f, 2.0f / 255.0f };
ALIGN16( static float SIMD_SSE2_float_descale[4] ) = { 255.0f / 2.0f, 255.0f / 2.0f, 255.0f / 2.0f, 255.0f / 2.0f };
ALIGN16( static float SIMD_SSE2_float_zero[4] ) = { 0.0f, 0.0f, 0.0f, 0.0f };
ALIGN16( static float SIMD_SSE2_float_one[4] ) = { 1.0f, 1.0f, 1.0f, 1.0f };
ALIGN16( static float SIMD_SSE2_float_half[4] ) = { 0.5f, 0.5f, 0.5f, 0.5f };
ALIGN16( static float SIMD_SSE2_float_255[4] ) = { 255.0f, 255.0f, 255.0f, 255.0f };
ALIGN16( static float SIMD_SP_rsqrt_c0[4] ) = { 3.0f, 3.0f, 3.0f, 3.0f };
ALIGN16( static float SIMD_SP_rsqrt_c1[4] ) = { -0.5f, -0.5f, -0.5f, -0.5f };
ALIGN16( static dword SIMD_SSE2_dword_maskFirstThree[4] ) = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000 };
ALIGN16( static dword SIMD_SSE2_dword_maskWords[4] ) = { 0x0000FFFF, 0x0000FFFF, 0x0000FFFF, 0x00000000 };
#define R_SHUFFLE_PS( x, y, z, w )	(( (w) & 3 ) << 6 | ( (z) & 3 ) << 4 | ( (y) & 3 ) << 2 | ( (x) & 3 ))

/*
========================
NormalDistanceDXT1
========================
*/
int NormalDistanceDXT1( const int* vector, const int* normalized )
{
#if defined(_MSC_VER) && defined(_M_IX86)
	int result;
	__asm
	{
		mov			esi, vector
		mov			edi, normalized
		cvtdq2ps	xmm0, [esi]
		mulps		xmm0, SIMD_SSE2_float_scale
		subps		xmm0, SIMD_SSE2_float_one
		pand		xmm0, SIMD_SSE2_dword_maskFirstThree
		movaps		xmm1, xmm0
		mulps		xmm1, xmm1
		pshufd		xmm2, xmm1, R_SHUFFLE_PS( 2, 3, 0, 1 )
		addps		xmm2, xmm1
		pshufd		xmm1, xmm2, R_SHUFFLE_PS( 1, 0, 1, 0 )
		addps		xmm2, xmm1

		rsqrtps		xmm1, xmm2
		mulps		xmm2, xmm1
		mulps		xmm2, xmm1
		subps		xmm2, SIMD_SP_rsqrt_c0
		mulps		xmm1, SIMD_SP_rsqrt_c1
		mulps		xmm2, xmm1

		mulps		xmm0, xmm2
		addps		xmm0, SIMD_SSE2_float_one
		mulps		xmm0, SIMD_SSE2_float_descale
		addps		xmm0, SIMD_SSE2_float_half
		maxps		xmm0, SIMD_SSE2_float_zero
		minps		xmm0, SIMD_SSE2_float_255
		cvttps2dq	xmm0, xmm0
		psubd		xmm0, [edi]
		pand		xmm0, SIMD_SSE2_dword_maskWords
		pmullw		xmm0, xmm0
		pshufd		xmm1, xmm0, R_SHUFFLE_PS( 2, 3, 0, 1 )
		paddd		xmm0, xmm1
		pshufd		xmm1, xmm0, R_SHUFFLE_PS( 1, 0, 1, 0 )
		paddd		xmm0, xmm1
		movd		result, xmm0
	}
	return result;
#else
	float floatNormal[3];
	byte intNormal[4];
	floatNormal[0] = vector[0] * ( 2.0f / 255.0f ) - 1.0f;
	floatNormal[1] = vector[1] * ( 2.0f / 255.0f ) - 1.0f;
	floatNormal[2] = vector[2] * ( 2.0f / 255.0f ) - 1.0f;
	float rcplen = idMath::InvSqrt( floatNormal[0] * floatNormal[0] + floatNormal[1] * floatNormal[1] + floatNormal[2] * floatNormal[2] );
	floatNormal[0] *= rcplen;
	floatNormal[1] *= rcplen;
	floatNormal[2] *= rcplen;
	intNormal[0] = idMath::Ftob( ( floatNormal[0] + 1.0f ) * ( 255.0f / 2.0f ) + 0.5f );
	intNormal[1] = idMath::Ftob( ( floatNormal[1] + 1.0f ) * ( 255.0f / 2.0f ) + 0.5f );
	intNormal[2] = idMath::Ftob( ( floatNormal[2] + 1.0f ) * ( 255.0f / 2.0f ) + 0.5f );
	int result =	( ( intNormal[ 0 ] - normalized[ 0 ] ) * ( intNormal[ 0 ] - normalized[ 0 ] ) ) +
					( ( intNormal[ 1 ] - normalized[ 1 ] ) * ( intNormal[ 1 ] - normalized[ 1 ] ) ) +
					( ( intNormal[ 2 ] - normalized[ 2 ] ) * ( intNormal[ 2 ] - normalized[ 2 ] ) );
	return result;
#endif
}

/*
========================
NormalDistanceDXT5
========================
*/
int NormalDistanceDXT5( const int* vector, const int* normalized )
{
#if _MSC_VER && defined(_M_IX86)
	int result;
	__asm
	{
		mov			esi, vector
		mov			edi, normalized
#if 0	// object-space
		pshufd		xmm0, [esi], R_SHUFFLE_PS( 0, 1, 3, 2 )
#else
		pshufd		xmm0, [esi], R_SHUFFLE_PS( 1, 2, 3, 0 )
#endif
		cvtdq2ps	xmm0, xmm0
		mulps		xmm0, SIMD_SSE2_float_scale
		subps		xmm0, SIMD_SSE2_float_one
		pand		xmm0, SIMD_SSE2_dword_maskFirstThree
		movaps		xmm1, xmm0
		mulps		xmm1, xmm1
		pshufd		xmm2, xmm1, R_SHUFFLE_PS( 2, 3, 0, 1 )
		addps		xmm2, xmm1
		pshufd		xmm1, xmm2, R_SHUFFLE_PS( 1, 0, 1, 0 )
		addps		xmm2, xmm1

		rsqrtps		xmm1, xmm2
		mulps		xmm2, xmm1
		mulps		xmm2, xmm1
		subps		xmm2, SIMD_SP_rsqrt_c0
		mulps		xmm1, SIMD_SP_rsqrt_c1
		mulps		xmm2, xmm1

		mulps		xmm0, xmm2
		addps		xmm0, SIMD_SSE2_float_one
		mulps		xmm0, SIMD_SSE2_float_descale
		addps		xmm0, SIMD_SSE2_float_half
		maxps		xmm0, SIMD_SSE2_float_zero
		minps		xmm0, SIMD_SSE2_float_255
		cvttps2dq	xmm0, xmm0
#if 0	// object-space
		pshufd		xmm3, [edi], R_SHUFFLE_PS( 0, 1, 3, 2 )
#else
		pshufd		xmm3, [edi], R_SHUFFLE_PS( 1, 2, 3, 0 )
#endif
		psubd		xmm0, xmm3
		pand		xmm0, SIMD_SSE2_dword_maskWords
		pmullw		xmm0, xmm0
		pshufd		xmm1, xmm0, R_SHUFFLE_PS( 2, 3, 0, 1 )
		paddd		xmm0, xmm1
		pshufd		xmm1, xmm0, R_SHUFFLE_PS( 1, 0, 1, 0 )
		paddd		xmm0, xmm1
		movd		result, xmm0
	}
	return result;
#else
#if 0	// object-space
	const int c0 = 0;
	const int c1 = 1;
	const int c2 = 3;
#else
	const int c0 = 1;
	const int c1 = 2;
	const int c2 = 3;
#endif
	float floatNormal[3];
	byte intNormal[4];
	floatNormal[0] = vector[c0] / 255.0f * 2.0f - 1.0f;
	floatNormal[1] = vector[c1] / 255.0f * 2.0f - 1.0f;
	floatNormal[2] = vector[c2] / 255.0f * 2.0f - 1.0f;
	float rcplen = idMath::InvSqrt( floatNormal[0] * floatNormal[0] + floatNormal[1] * floatNormal[1] + floatNormal[2] * floatNormal[2] );
	floatNormal[0] *= rcplen;
	floatNormal[1] *= rcplen;
	floatNormal[2] *= rcplen;
	intNormal[c0] = idMath::Ftob( ( floatNormal[0] + 1.0f ) / 2.0f * 255.0f + 0.5f );
	intNormal[c1] = idMath::Ftob( ( floatNormal[1] + 1.0f ) / 2.0f * 255.0f + 0.5f );
	intNormal[c2] = idMath::Ftob( ( floatNormal[2] + 1.0f ) / 2.0f * 255.0f + 0.5f );
	int result =	( ( intNormal[ c0 ] - normalized[ c0 ] ) * ( intNormal[ c0 ] - normalized[ c0 ] ) ) +
					( ( intNormal[ c1 ] - normalized[ c1 ] ) * ( intNormal[ c1 ] - normalized[ c1 ] ) ) +
					( ( intNormal[ c2 ] - normalized[ c2 ] ) * ( intNormal[ c2 ] - normalized[ c2 ] ) );
	return result;
#endif
}

/*
========================
idDxtEncoder::GetSquareNormalsDXT1Error

params:	colorBlock	- 4*4 input tile, 4 bytes per pixel
paramO:	color0		- 4 byte Min color found
paramO:	color1		- 4 byte Max color found
return: 4 byte color index block
========================
*/
int idDxtEncoder::GetSquareNormalsDXT1Error( const int* colorBlock, const unsigned short color0, const unsigned short color1, int lastError, unsigned int& colorIndices ) const
{
	byte byteColors[2][4];
	ALIGN16( int colors[4][4] );

	ColorFrom565( color0, byteColors[0] );
	ColorFrom565( color1, byteColors[1] );

	for( int i = 0; i < 4; i++ )
	{
		colors[0][i] = byteColors[0][i];
		colors[1][i] = byteColors[1][i];
	}

	if( color0 > color1 )
	{
		colors[2][0] = ( 2 * colors[0][0] + 1 * colors[1][0] ) / 3;
		colors[2][1] = ( 2 * colors[0][1] + 1 * colors[1][1] ) / 3;
		colors[2][2] = ( 2 * colors[0][2] + 1 * colors[1][2] ) / 3;
		colors[3][0] = ( 1 * colors[0][0] + 2 * colors[1][0] ) / 3;
		colors[3][1] = ( 1 * colors[0][1] + 2 * colors[1][1] ) / 3;
		colors[3][2] = ( 1 * colors[0][2] + 2 * colors[1][2] ) / 3;
	}
	else
	{
		assert( color0 == color1 );
		colors[2][0] = ( 1 * colors[0][0] + 1 * colors[1][0] ) / 2;
		colors[2][1] = ( 1 * colors[0][1] + 1 * colors[1][1] ) / 2;
		colors[2][2] = ( 1 * colors[0][2] + 1 * colors[1][2] ) / 2;
		colors[3][0] = 0;
		colors[3][1] = 0;
		colors[3][2] = 0;
	}

	int error = 0;
	int tempColorIndices[16];
	for( int i = 0; i < 16; i++ )
	{
		unsigned int minDist = MAX_UNSIGNED_TYPE( int );

		for( int j = 0; j < 4; j++ )
		{
			unsigned int dist = NormalDistanceDXT1( &colors[j][0], &colorBlock[i * 4] );
			if( dist < minDist )
			{
				minDist = dist;
				tempColorIndices[i] = j;
			}
		}
		// accumulated error
		error += minDist;

		if( error > lastError )
		{
			return error;
		}
	}

	colorIndices = 0;
	for( int i = 0; i < 16; i++ )
	{
		colorIndices |= ( tempColorIndices[i] << ( unsigned int )( i << 1 ) );
	}

	return error;
}

/*
========================
idDxtEncoder::GetMinMaxNormalsDXT1HQ

Uses an exhaustive search to find the two RGB colors that produce the least error when used to
compress the 4x4 block. Also finds the minimum and maximum alpha values.

params:	colorBlock	- 4*4 input tile, 4 bytes per pixel
paramO:	minColor	- 4 byte Min color found
paramO:	maxColor	- 4 byte Max color found
========================
*/
int idDxtEncoder::GetMinMaxNormalsDXT1HQ( const byte* colorBlock, byte* minColor, byte* maxColor, unsigned int& colorIndices, bool noBlack ) const
{
	int i;
	int i0, i1, i2, j0, j1, j2;
	unsigned short bestMinColor565 = 0;
	unsigned short bestMaxColor565 = 0;
	byte bboxMin[3], bboxMax[3], minAxisDist[3];
	int error, bestError = MAX_TYPE( int );
	unsigned int tempColorIndices;
	ALIGN16( int intColorBlock[16 * 4] );

	bboxMin[0] = bboxMin[1] = bboxMin[2] = 128;
	bboxMax[0] = bboxMax[1] = bboxMax[2] = 128;

	// get color bbox
	for( i = 0; i < 16; i++ )
	{
		if( colorBlock[i * 4 + 0] < bboxMin[0] )
		{
			bboxMin[0] = colorBlock[i * 4 + 0];
		}
		if( colorBlock[i * 4 + 1] < bboxMin[1] )
		{
			bboxMin[1] = colorBlock[i * 4 + 1];
		}
		if( colorBlock[i * 4 + 2] < bboxMin[2] )
		{
			bboxMin[2] = colorBlock[i * 4 + 2];
		}
		if( colorBlock[i * 4 + 0] > bboxMax[0] )
		{
			bboxMax[0] = colorBlock[i * 4 + 0];
		}
		if( colorBlock[i * 4 + 1] > bboxMax[1] )
		{
			bboxMax[1] = colorBlock[i * 4 + 1];
		}
		if( colorBlock[i * 4 + 2] > bboxMax[2] )
		{
			bboxMax[2] = colorBlock[i * 4 + 2];
		}
	}

	for( int i = 0; i < 64; i++ )
	{
		intColorBlock[i] = colorBlock[i];
	}

	// decrease range for 565 encoding
	bboxMin[0] >>= 3;
	bboxMin[1] >>= 2;
	bboxMin[2] >>= 3;
	bboxMax[0] >>= 3;
	bboxMax[1] >>= 2;
	bboxMax[2] >>= 3;

	// get the minimum distance the end points of the line must be apart along each axis
	for( i = 0; i < 3; i++ )
	{
		minAxisDist[i] = 0;
	}

	// expand the bounding box
	const int C565_BBOX_EXPAND = 2;

	bboxMin[0] = ( bboxMin[0] <= C565_BBOX_EXPAND ) ? 0 : bboxMin[0] - C565_BBOX_EXPAND;
	bboxMin[1] = ( bboxMin[1] <= C565_BBOX_EXPAND ) ? 0 : bboxMin[1] - C565_BBOX_EXPAND;
	bboxMin[2] = ( bboxMin[2] <= C565_BBOX_EXPAND ) ? 0 : bboxMin[2] - C565_BBOX_EXPAND;
	bboxMax[0] = ( bboxMax[0] >= ( 255 >> 3 ) - C565_BBOX_EXPAND ) ? ( 255 >> 3 ) : bboxMax[0] + C565_BBOX_EXPAND;
	bboxMax[1] = ( bboxMax[1] >= ( 255 >> 2 ) - C565_BBOX_EXPAND ) ? ( 255 >> 2 ) : bboxMax[1] + C565_BBOX_EXPAND;
	bboxMax[2] = ( bboxMax[2] >= ( 255 >> 3 ) - C565_BBOX_EXPAND ) ? ( 255 >> 3 ) : bboxMax[2] + C565_BBOX_EXPAND;

	for( i0 = bboxMin[0]; i0 <= bboxMax[0]; i0++ )
	{
		for( j0 = bboxMax[0]; j0 >= bboxMin[0]; j0-- )
		{
			if( abs( i0 - j0 ) < minAxisDist[0] )
			{
				continue;
			}

			for( i1 = bboxMin[1]; i1 <= bboxMax[1]; i1++ )
			{
				for( j1 = bboxMax[1]; j1 >= bboxMin[1]; j1-- )
				{
					if( abs( i1 - j1 ) < minAxisDist[1] )
					{
						continue;
					}

					for( i2 = bboxMin[2]; i2 <= bboxMax[2]; i2++ )
					{
						for( j2 = bboxMax[2]; j2 >= bboxMin[2]; j2-- )
						{
							if( abs( i2 - j2 ) < minAxisDist[2] )
							{
								continue;
							}

							unsigned short minColor565 = ( unsigned short )( ( i0 << 11 ) | ( i1 << 5 ) | ( i2 << 0 ) );
							unsigned short maxColor565 = ( unsigned short )( ( j0 << 11 ) | ( j1 << 5 ) | ( j2 << 0 ) );

							if( !noBlack )
							{
								error = GetSquareNormalsDXT1Error( intColorBlock, maxColor565, minColor565, bestError, tempColorIndices );
								if( error < bestError )
								{
									bestError = error;
									bestMinColor565 = minColor565;
									bestMaxColor565 = maxColor565;
									colorIndices = tempColorIndices;
								}
							}
							else
							{
								if( minColor565 <= maxColor565 )
								{
									SwapValues( minColor565, maxColor565 );
								}
							}

							error = GetSquareNormalsDXT1Error( intColorBlock, minColor565, maxColor565, bestError, tempColorIndices );
							if( error < bestError )
							{
								bestError = error;
								bestMinColor565 = minColor565;
								bestMaxColor565 = maxColor565;
								colorIndices = tempColorIndices;
							}
						}
					}
				}
			}
		}
	}

	ColorFrom565( bestMinColor565, minColor );
	ColorFrom565( bestMaxColor565, maxColor );

	return bestError;
}

/*
========================
idDxtEncoder::GetSquareNormalsDXT5Error

params:	normalBlock	- 16 pixel block for which to find normal indexes
paramO:	minNormal	- Min normal found
paramO:	maxNormal	- Max normal found
========================
*/
int idDxtEncoder::GetSquareNormalsDXT5Error( const int* normalBlock, const byte* minNormal, const byte* maxNormal, int lastError, unsigned int& colorIndices, byte* alphaIndices ) const
{
	byte alphas[8];
	byte colors[4][4];

	unsigned short smin = ColorTo565( minNormal );
	unsigned short smax = ColorTo565( maxNormal );

	ColorFrom565( smax, colors[0] );
	ColorFrom565( smin, colors[1] );

	if( smax > smin )
	{
		colors[2][0] = ( 2 * colors[0][0] + 1 * colors[1][0] ) / 3;
		colors[2][1] = ( 2 * colors[0][1] + 1 * colors[1][1] ) / 3;
		colors[2][2] = ( 2 * colors[0][2] + 1 * colors[1][2] ) / 3;
		colors[3][0] = ( 1 * colors[0][0] + 2 * colors[1][0] ) / 3;
		colors[3][1] = ( 1 * colors[0][1] + 2 * colors[1][1] ) / 3;
		colors[3][2] = ( 1 * colors[0][2] + 2 * colors[1][2] ) / 3;
	}
	else
	{
		assert( smax == smin );
		colors[2][0] = ( 1 * colors[0][0] + 1 * colors[1][0] ) / 2;
		colors[2][1] = ( 1 * colors[0][1] + 1 * colors[1][1] ) / 2;
		colors[2][2] = ( 1 * colors[0][2] + 1 * colors[1][2] ) / 2;
		colors[3][0] = 0;
		colors[3][1] = 0;
		colors[3][2] = 0;
	}

	alphas[0] = maxNormal[3];
	alphas[1] = minNormal[3];

	if( maxNormal[3] > minNormal[3] )
	{
		alphas[2] = ( 6 * alphas[0] + 1 * alphas[1] ) / 7;
		alphas[3] = ( 5 * alphas[0] + 2 * alphas[1] ) / 7;
		alphas[4] = ( 4 * alphas[0] + 3 * alphas[1] ) / 7;
		alphas[5] = ( 3 * alphas[0] + 4 * alphas[1] ) / 7;
		alphas[6] = ( 2 * alphas[0] + 5 * alphas[1] ) / 7;
		alphas[7] = ( 1 * alphas[0] + 6 * alphas[1] ) / 7;
	}
	else
	{
		alphas[2] = ( 4 * alphas[0] + 1 * alphas[1] ) / 5;
		alphas[3] = ( 3 * alphas[0] + 2 * alphas[1] ) / 5;
		alphas[4] = ( 2 * alphas[0] + 3 * alphas[1] ) / 5;
		alphas[5] = ( 1 * alphas[0] + 4 * alphas[1] ) / 5;
		alphas[6] = 0;
		alphas[7] = 255;
	}

	int error = 0;
	int tempColorIndices[16];
	int tempAlphaIndices[16];
	for( int i = 0; i < 16; i++ )
	{
		ALIGN16( int normal[4] );
		unsigned int minDist = MAX_UNSIGNED_TYPE( int );

		for( int j = 0; j < 4; j++ )
		{
			normal[0] = colors[j][0];
			normal[1] = colors[j][1];
			normal[2] = colors[j][2];

			for( int k = 0; k < 8; k++ )
			{
				normal[3] = alphas[k];
				unsigned int dist = NormalDistanceDXT5( normal, &normalBlock[i * 4] );
				if( dist < minDist )
				{
					minDist = dist;
					tempColorIndices[i] = j;
					tempAlphaIndices[i] = k;
				}
			}
		}
		error += minDist;

		if( error >= lastError )
		{
			return error;
		}
	}

	alphaIndices[0] = byte( ( tempAlphaIndices[ 0] >> 0 ) | ( tempAlphaIndices[ 1] << 3 ) | ( tempAlphaIndices[ 2] << 6 ) );
	alphaIndices[1] = byte( ( tempAlphaIndices[ 2] >> 2 ) | ( tempAlphaIndices[ 3] << 1 ) | ( tempAlphaIndices[ 4] << 4 ) | ( tempAlphaIndices[ 5] << 7 ) );
	alphaIndices[2] = byte( ( tempAlphaIndices[ 5] >> 1 ) | ( tempAlphaIndices[ 6] << 2 ) | ( tempAlphaIndices[ 7] << 5 ) );

	alphaIndices[3] = byte( ( tempAlphaIndices[ 8] >> 0 ) | ( tempAlphaIndices[ 9] << 3 ) | ( tempAlphaIndices[10] << 6 ) );
	alphaIndices[4] = byte( ( tempAlphaIndices[10] >> 2 ) | ( tempAlphaIndices[11] << 1 ) | ( tempAlphaIndices[12] << 4 ) | ( tempAlphaIndices[13] << 7 ) );
	alphaIndices[5] = byte( ( tempAlphaIndices[13] >> 1 ) | ( tempAlphaIndices[14] << 2 ) | ( tempAlphaIndices[15] << 5 ) );

	colorIndices = 0;
	for( int i = 0; i < 16; i++ )
	{
		colorIndices |= ( tempColorIndices[i] << ( unsigned int )( i << 1 ) );
	}

	return error;
}

/*
========================
idDxtEncoder::GetMinMaxNormalsDXT5HQ

Uses an exhaustive search to find the two RGB colors that produce the least error when used to
compress the 4x4 block. Also finds the minimum and maximum alpha values.

params:	colorBlock	- 4*4 input tile, 4 bytes per pixel
paramO:	minColor	- 4 byte Min color found
paramO:	maxColor	- 4 byte Max color found
========================
*/
int idDxtEncoder::GetMinMaxNormalsDXT5HQ( const byte* colorBlock, byte* minColor, byte* maxColor, unsigned int& colorIndices, byte* alphaIndices ) const
{
	int i;
	int i0, i1, i3, j0, j1, j3;
	byte bboxMin[4], bboxMax[4], minAxisDist[4];
	byte tmin[4], tmax[4];
	int error, bestError = MAX_TYPE( int );
	unsigned int tempColorIndices;
	byte tempAlphaIndices[6];
	ALIGN16( int intColorBlock[16 * 4] );

	bboxMin[0] = bboxMin[1] = bboxMin[2] = bboxMin[3] = 255;
	bboxMax[0] = bboxMax[1] = bboxMax[2] = bboxMax[3] = 0;

	// get color bbox
	for( i = 0; i < 16; i++ )
	{
		if( colorBlock[i * 4 + 0] < bboxMin[0] )
		{
			bboxMin[0] = colorBlock[i * 4 + 0];
		}
		if( colorBlock[i * 4 + 1] < bboxMin[1] )
		{
			bboxMin[1] = colorBlock[i * 4 + 1];
		}
		if( colorBlock[i * 4 + 2] < bboxMin[2] )
		{
			bboxMin[2] = colorBlock[i * 4 + 2];
		}
		if( colorBlock[i * 4 + 3] < bboxMin[3] )
		{
			bboxMin[3] = colorBlock[i * 4 + 3];
		}
		if( colorBlock[i * 4 + 0] > bboxMax[0] )
		{
			bboxMax[0] = colorBlock[i * 4 + 0];
		}
		if( colorBlock[i * 4 + 1] > bboxMax[1] )
		{
			bboxMax[1] = colorBlock[i * 4 + 1];
		}
		if( colorBlock[i * 4 + 2] > bboxMax[2] )
		{
			bboxMax[2] = colorBlock[i * 4 + 2];
		}
		if( colorBlock[i * 4 + 3] > bboxMax[3] )
		{
			bboxMax[3] = colorBlock[i * 4 + 3];
		}
	}

	for( int i = 0; i < 64; i++ )
	{
		intColorBlock[i] = colorBlock[i];
	}

	// decrease range for 565 encoding
	bboxMin[0] >>= 3;
	bboxMin[1] >>= 2;
	bboxMax[0] >>= 3;
	bboxMax[1] >>= 2;

	// get the minimum distance the end points of the line must be apart along each axis
	for( i = 0; i < 4; i++ )
	{
		minAxisDist[i] = 0;
	}

	// expand the bounding box
	const int C565_BBOX_EXPAND = 2;
	const int ALPHA_BBOX_EXPAND = 32;

	bboxMin[0] = ( bboxMin[0] <= C565_BBOX_EXPAND ) ? 0 : bboxMin[0] - C565_BBOX_EXPAND;
	bboxMin[1] = ( bboxMin[1] <= C565_BBOX_EXPAND ) ? 0 : bboxMin[1] - C565_BBOX_EXPAND;
	bboxMin[3] = ( bboxMin[3] <= ALPHA_BBOX_EXPAND ) ? 0 : bboxMin[3] - ALPHA_BBOX_EXPAND;
	bboxMax[0] = ( bboxMax[0] >= ( 255 >> 3 ) - C565_BBOX_EXPAND ) ? ( 255 >> 3 ) : bboxMax[0] + C565_BBOX_EXPAND;
	bboxMax[1] = ( bboxMax[1] >= ( 255 >> 2 ) - C565_BBOX_EXPAND ) ? ( 255 >> 2 ) : bboxMax[1] + C565_BBOX_EXPAND;
	bboxMax[3] = ( bboxMax[3] >= ( 255 ) - ALPHA_BBOX_EXPAND ) ? ( 255 ) : bboxMax[3] + ALPHA_BBOX_EXPAND;

	for( i0 = bboxMin[0]; i0 <= bboxMax[0]; i0++ )
	{
		for( j0 = bboxMax[0]; j0 >= bboxMin[0]; j0-- )
		{
			if( abs( i0 - j0 ) < minAxisDist[0] )
			{
				continue;
			}

			for( i1 = bboxMin[1]; i1 <= bboxMax[1]; i1++ )
			{
				for( j1 = bboxMax[1]; j1 >= bboxMin[1]; j1-- )
				{
					if( abs( i1 - j1 ) < minAxisDist[1] )
					{
						continue;
					}

					tmin[0] = ( byte )j0 << 3;
					tmin[1] = ( byte )j1 << 2;
					tmin[2] = 0;

					tmax[0] = ( byte )i0 << 3;
					tmax[1] = ( byte )i1 << 2;
					tmax[2] = 0;

					for( i3 = bboxMin[3]; i3 <= bboxMax[3]; i3++ )
					{
						for( j3 = bboxMax[3]; j3 >= bboxMin[3]; j3-- )
						{
							if( abs( i3 - j3 ) < minAxisDist[3] )
							{
								continue;
							}

							tmin[3] = ( byte )j3;
							tmax[3] = ( byte )i3;

							error = GetSquareNormalsDXT5Error( intColorBlock, tmin, tmax, bestError, tempColorIndices, tempAlphaIndices );
							if( error < bestError )
							{
								bestError = error;
								memcpy( minColor, tmin, 4 );
								memcpy( maxColor, tmax, 4 );
								colorIndices = tempColorIndices;
								memcpy( alphaIndices, tempAlphaIndices, 6 );
							}

							tmin[3] = ( byte )i3;
							tmax[3] = ( byte )j3;

							error = GetSquareNormalsDXT5Error( intColorBlock, tmin, tmax, bestError, tempColorIndices, tempAlphaIndices );
							if( error < bestError )
							{
								bestError = error;
								memcpy( minColor, tmin, 4 );
								memcpy( maxColor, tmax, 4 );
								colorIndices = tempColorIndices;
								memcpy( alphaIndices, tempAlphaIndices, 6 );
							}
						}
					}
				}
			}
		}
	}

	return bestError;
}

/*
========================
idDxtEncoder::GetMinMaxNormalsDXT5HQFast

Uses an exhaustive search to find the two RGB colors that produce the least error when used to
compress the 4x4 block. Also finds the minimum and maximum alpha values.

params:	colorBlock	- 4*4 input tile, 4 bytes per pixel
paramO:	minColor	- 4 byte Min color found
paramO:	maxColor	- 4 byte Max color found
========================
*/
int idDxtEncoder::GetMinMaxNormalsDXT5HQFast( const byte* colorBlock, byte* minColor, byte* maxColor, unsigned int& colorIndices, byte* alphaIndices ) const
{
	int i0, i1, i2, i3, j0, j1, j2, j3;
	byte bboxMin[4], bboxMax[4], minAxisDist[4];
	byte tmin[4], tmax[4];
	int error, bestError = MAX_TYPE( int );
	unsigned int tempColorIndices;
	byte tempAlphaIndices[6];
	ALIGN16( int intColorBlock[16 * 4] );

	bboxMin[0] = bboxMin[1] = bboxMin[2] = bboxMin[3] = 255;
	bboxMax[0] = bboxMax[1] = bboxMax[2] = bboxMax[3] = 0;

	// get color bbox
	for( int i = 0; i < 16; i++ )
	{
		if( colorBlock[i * 4 + 0] < bboxMin[0] )
		{
			bboxMin[0] = colorBlock[i * 4 + 0];
		}
		if( colorBlock[i * 4 + 1] < bboxMin[1] )
		{
			bboxMin[1] = colorBlock[i * 4 + 1];
		}
		if( colorBlock[i * 4 + 2] < bboxMin[2] )
		{
			bboxMin[2] = colorBlock[i * 4 + 2];
		}
		if( colorBlock[i * 4 + 3] < bboxMin[3] )
		{
			bboxMin[3] = colorBlock[i * 4 + 3];
		}
		if( colorBlock[i * 4 + 0] > bboxMax[0] )
		{
			bboxMax[0] = colorBlock[i * 4 + 0];
		}
		if( colorBlock[i * 4 + 1] > bboxMax[1] )
		{
			bboxMax[1] = colorBlock[i * 4 + 1];
		}
		if( colorBlock[i * 4 + 2] > bboxMax[2] )
		{
			bboxMax[2] = colorBlock[i * 4 + 2];
		}
		if( colorBlock[i * 4 + 3] > bboxMax[3] )
		{
			bboxMax[3] = colorBlock[i * 4 + 3];
		}
	}

	for( int i = 0; i < 64; i++ )
	{
		intColorBlock[i] = colorBlock[i];
	}

	// decrease range for 565 encoding
	bboxMin[0] >>= 3;
	bboxMin[1] >>= 2;
	bboxMin[2] >>= 3;
	bboxMax[0] >>= 3;
	bboxMax[1] >>= 2;
	bboxMax[2] >>= 3;

	bboxMin[3] = 0;
	bboxMax[3] = 255;

	// get the minimum distance the end points of the line must be apart along each axis
	for( int i = 0; i < 4; i++ )
	{
		minAxisDist[i] = 0;
	}

	// expand the bounding box
	const int C565_BBOX_EXPAND = 1;
	const int ALPHA_BBOX_EXPAND = 128;

#if 0 // object-space
	bboxMin[0] = ( bboxMin[0] <= C565_BBOX_EXPAND ) ? 0 : bboxMin[0] - C565_BBOX_EXPAND;
	bboxMax[0] = ( bboxMax[0] >= ( 255 >> 3 ) - C565_BBOX_EXPAND ) ? ( 255 >> 3 ) : bboxMax[0] + C565_BBOX_EXPAND;
	bboxMin[2] = 0;
	bboxMax[2] = 0;
#else
	bboxMin[0] = 0;
	bboxMax[0] = 0;
	bboxMin[2] = ( bboxMin[2] <= C565_BBOX_EXPAND ) ? 0 : bboxMin[2] - C565_BBOX_EXPAND;
	bboxMax[2] = ( bboxMax[2] >= ( 255 >> 2 ) - C565_BBOX_EXPAND ) ? ( 255 >> 2 ) : bboxMax[2] + C565_BBOX_EXPAND;
#endif

	bboxMin[1] = ( bboxMin[1] <= C565_BBOX_EXPAND ) ? 0 : bboxMin[1] - C565_BBOX_EXPAND;
	bboxMax[1] = ( bboxMax[1] >= ( 255 >> 2 ) - C565_BBOX_EXPAND ) ? ( 255 >> 2 ) : bboxMax[1] + C565_BBOX_EXPAND;

	bboxMin[3] = ( bboxMin[3] <= ALPHA_BBOX_EXPAND ) ? 0 : bboxMin[3] - ALPHA_BBOX_EXPAND;
	bboxMax[3] = ( bboxMax[3] >= ( 255 ) - ALPHA_BBOX_EXPAND ) ? ( 255 ) : bboxMax[3] + ALPHA_BBOX_EXPAND;

	for( i0 = bboxMin[0]; i0 <= bboxMax[0]; i0++ )
	{
		for( j0 = bboxMax[0]; j0 >= bboxMin[0]; j0-- )
		{
			if( abs( i0 - j0 ) < minAxisDist[0] )
			{
				continue;
			}

			for( i1 = bboxMin[1]; i1 <= bboxMax[1]; i1++ )
			{
				for( j1 = bboxMax[1]; j1 >= bboxMin[1]; j1-- )
				{
					if( abs( i1 - j1 ) < minAxisDist[1] )
					{
						continue;
					}

					for( i2 = bboxMin[2]; i2 <= bboxMax[2]; i2++ )
					{
						for( j2 = bboxMax[2]; j2 >= bboxMin[2]; j2-- )
						{
							if( abs( i2 - j2 ) < minAxisDist[2] )
							{
								continue;
							}

							unsigned short minColor565 = ( unsigned short )( ( i0 << 11 ) | ( i1 << 5 ) | i2 );
							unsigned short maxColor565 = ( unsigned short )( ( j0 << 11 ) | ( j1 << 5 ) | j2 );

							if( minColor565 > maxColor565 )
							{
								SwapValues( minColor565, maxColor565 );
							}

							error = GetSquareNormalsDXT1Error( intColorBlock, maxColor565, minColor565, bestError, tempColorIndices );
							if( error < bestError )
							{
								bestError = error;
								ColorFrom565( minColor565, minColor );
								ColorFrom565( maxColor565, maxColor );
								colorIndices = tempColorIndices;
							}
						}
					}
				}
			}
		}
	}

	bestError = MAX_TYPE( int );

	memcpy( tmin, minColor, 4 );
	memcpy( tmax, maxColor, 4 );

	for( i3 = bboxMin[3]; i3 <= bboxMax[3]; i3++ )
	{
		for( j3 = bboxMax[3]; j3 >= bboxMin[3]; j3-- )
		{
			if( abs( i3 - j3 ) < minAxisDist[3] )
			{
				continue;
			}

			tmin[3] = ( byte )j3;
			tmax[3] = ( byte )i3;

			error = GetSquareNormalsDXT5Error( intColorBlock, tmin, tmax, bestError, tempColorIndices, tempAlphaIndices );
			if( error < bestError )
			{
				bestError = error;
				memcpy( minColor, tmin, 4 );
				memcpy( maxColor, tmax, 4 );
				colorIndices = tempColorIndices;
				memcpy( alphaIndices, tempAlphaIndices, 6 );
			}

			tmin[3] = ( byte )i3;
			tmax[3] = ( byte )j3;

			error = GetSquareNormalsDXT5Error( intColorBlock, tmin, tmax, bestError, tempColorIndices, tempAlphaIndices );
			if( error < bestError )
			{
				bestError = error;
				memcpy( minColor, tmin, 4 );
				memcpy( maxColor, tmax, 4 );
				colorIndices = tempColorIndices;
				memcpy( alphaIndices, tempAlphaIndices, 6 );
			}
		}
	}

	return bestError;
}

/*
========================
idDxtEncoder::FindColorIndices

params:	colorBlock	- 16 pixel block for which find color indexes
paramO:	color0		- Min color found
paramO:	color1		- Max color found
return: 4 byte color index block
========================
*/
int idDxtEncoder::FindColorIndices( const byte* colorBlock, const unsigned short color0, const unsigned short color1, unsigned int& result ) const
{
	int i, j;
	unsigned int indexes[16];
	byte colors[4][4];

	ColorFrom565( color0, colors[0] );
	ColorFrom565( color1, colors[1] );

	if( color0 > color1 )
	{
		colors[2][0] = ( 2 * colors[0][0] + 1 * colors[1][0] ) / 3;
		colors[2][1] = ( 2 * colors[0][1] + 1 * colors[1][1] ) / 3;
		colors[2][2] = ( 2 * colors[0][2] + 1 * colors[1][2] ) / 3;
		colors[3][0] = ( 1 * colors[0][0] + 2 * colors[1][0] ) / 3;
		colors[3][1] = ( 1 * colors[0][1] + 2 * colors[1][1] ) / 3;
		colors[3][2] = ( 1 * colors[0][2] + 2 * colors[1][2] ) / 3;
	}
	else
	{
		colors[2][0] = ( 1 * colors[0][0] + 1 * colors[1][0] ) / 2;
		colors[2][1] = ( 1 * colors[0][1] + 1 * colors[1][1] ) / 2;
		colors[2][2] = ( 1 * colors[0][2] + 1 * colors[1][2] ) / 2;
		colors[3][0] = 0;
		colors[3][1] = 0;
		colors[3][2] = 0;
	}

	int error = 0;
	for( i = 0; i < 16; i++ )
	{
		unsigned int minDist = MAX_UNSIGNED_TYPE( int );
		for( j = 0; j < 4; j++ )
		{
			unsigned int dist = ColorDistance( &colorBlock[i * 4], &colors[j][0] );
			if( dist < minDist )
			{
				minDist = dist;
				indexes[i] = j;
			}
		}
		// accumulated error
		error += minDist;
	}

	result = 0;
	for( i = 0; i < 16; i++ )
	{
		result |= ( indexes[i] << ( unsigned int )( i << 1 ) );
	}

	return error;
}

/*
========================
idDxtEncoder::FindAlphaIndices

params:	colorBlock	- 16 pixel block for which find alpha indexes
paramO:	alpha0		- Min alpha found
paramO:	alpha1		- Max alpha found
params:	rindexes	- 6 byte alpha index block
return: error metric for this compression
========================
*/
int idDxtEncoder::FindAlphaIndices( const byte* colorBlock, const int alphaOffset, const byte alpha0, const byte alpha1, byte* rindexes ) const
{
	int i, j;
	unsigned int indexes[16];
	byte alphas[8];

	alphas[0] = alpha0;
	alphas[1] = alpha1;
	if( alpha0 > alpha1 )
	{
		alphas[2] = ( 6 * alpha0 + 1 * alpha1 ) / 7;
		alphas[3] = ( 5 * alpha0 + 2 * alpha1 ) / 7;
		alphas[4] = ( 4 * alpha0 + 3 * alpha1 ) / 7;
		alphas[5] = ( 3 * alpha0 + 4 * alpha1 ) / 7;
		alphas[6] = ( 2 * alpha0 + 5 * alpha1 ) / 7;
		alphas[7] = ( 1 * alpha0 + 6 * alpha1 ) / 7;
	}
	else
	{
		alphas[2] = ( 4 * alpha0 + 1 * alpha1 ) / 5;
		alphas[3] = ( 3 * alpha0 + 2 * alpha1 ) / 5;
		alphas[4] = ( 2 * alpha0 + 3 * alpha1 ) / 5;
		alphas[5] = ( 1 * alpha0 + 4 * alpha1 ) / 5;
		alphas[6] = 0;
		alphas[7] = 255;
	}

	int error = 0;
	for( i = 0; i < 16; i++ )
	{
		unsigned int minDist = MAX_UNSIGNED_TYPE( int );
		byte a = colorBlock[i * 4 + alphaOffset];
		for( j = 0; j < 8; j++ )
		{
			unsigned int dist = AlphaDistance( a, alphas[j] );
			if( dist < minDist )
			{
				minDist = dist;
				indexes[i] = j;
			}
		}
		error += minDist;
	}

	rindexes[0] = byte( ( indexes[ 0] >> 0 ) | ( indexes[ 1] << 3 ) | ( indexes[ 2] << 6 ) );
	rindexes[1] = byte( ( indexes[ 2] >> 2 ) | ( indexes[ 3] << 1 ) | ( indexes[ 4] << 4 ) | ( indexes[ 5] << 7 ) );
	rindexes[2] = byte( ( indexes[ 5] >> 1 ) | ( indexes[ 6] << 2 ) | ( indexes[ 7] << 5 ) );

	rindexes[3] = byte( ( indexes[ 8] >> 0 ) | ( indexes[ 9] << 3 ) | ( indexes[10] << 6 ) );
	rindexes[4] = byte( ( indexes[10] >> 2 ) | ( indexes[11] << 1 ) | ( indexes[12] << 4 ) | ( indexes[13] << 7 ) );
	rindexes[5] = byte( ( indexes[13] >> 1 ) | ( indexes[14] << 2 ) | ( indexes[15] << 5 ) );

	return error;
}

/*
========================
idDxtEncoder::FindCTX1Indices

params:	colorBlock	- 16 pixel block for which find color indexes
paramO:	color0		- Min color found
paramO:	color1		- Max color found
return: 4 byte color index block
========================
*/
int idDxtEncoder::FindCTX1Indices( const byte* colorBlock, const byte* color0, const byte* color1, unsigned int& result ) const
{
	int i, j;
	unsigned int indexes[16];
	byte colors[4][4];

	colors[0][0] = color1[0];
	colors[0][1] = color1[1];
	colors[1][0] = color0[0];
	colors[1][1] = color0[1];

	colors[2][0] = ( 2 * colors[0][0] + 1 * colors[1][0] ) / 3;
	colors[2][1] = ( 2 * colors[0][1] + 1 * colors[1][1] ) / 3;
	colors[3][0] = ( 1 * colors[0][0] + 2 * colors[1][0] ) / 3;
	colors[3][1] = ( 1 * colors[0][1] + 2 * colors[1][1] ) / 3;

	int error = 0;
	for( i = 0; i < 16; i++ )
	{
		unsigned int minDist = MAX_UNSIGNED_TYPE( int );
		for( j = 0; j < 4; j++ )
		{
			unsigned int dist = CTX1Distance( &colorBlock[i * 4], &colors[j][0] );
			if( dist < minDist )
			{
				minDist = dist;
				indexes[i] = j;
			}
		}
		// accumulated error
		error += minDist;
	}

	result = 0;
	for( i = 0; i < 16; i++ )
	{
		result |= ( indexes[i] << ( unsigned int )( i << 1 ) );
	}

	return error;
}

/*
========================
idDxtEncoder::CompressImageDXT1HQ

params:	inBuf		- image to compress
paramO:	outBuf		- result of compression
params:	width		- width of image
params:	height		- height of image
========================
*/
void idDxtEncoder::CompressImageDXT1HQ( const byte* inBuf, byte* outBuf, int width, int height )
{
	ALIGN16( byte block[64] );
	unsigned int colorIndices1;
	unsigned int colorIndices2;
	byte col1[4];
	byte col2[4];
	int error1;
	int error2;

	this->width = width;
	this->height = height;
	this->outData = outBuf;

	if( width > 4 && ( width & 3 ) != 0 )
	{
		return;
	}
	if( height > 4 && ( height & 3 ) != 0 )
	{
		return;
	}

	if( width < 4 || height < 4 )
	{
		WriteTinyColorDXT1( inBuf, width, height );
		return;
	}

	for( int j = 0; j < height; j += 4, inBuf += width * 4 * 4 )
	{
		for( int i = 0; i < width; i += 4 )
		{
			commonLocal.LoadPacifierBinarizeProgressIncrement( 16 );

			ExtractBlock( inBuf + i * 4, width, block );

			GetMinMaxColorsHQ( block, col1, col2, false );

			// Write out color data. Try and find minimum error for the two encoding methods.
			unsigned short scol1 = ColorTo565( col1 );
			unsigned short scol2 = ColorTo565( col2 );

			error1 = FindColorIndices( block, scol1, scol2, colorIndices1 );
			error2 = FindColorIndices( block, scol2, scol1, colorIndices2 );

			if( error1 < error2 )
			{

				EmitUShort( scol1 );
				EmitUShort( scol2 );
				EmitUInt( colorIndices1 );

			}
			else
			{

				EmitUShort( scol2 );
				EmitUShort( scol1 );
				EmitUInt( colorIndices2 );
			}

			//idLib::Printf( "\r%3d%%", ( j * width + i ) * 100 / ( width * height ) );
		}
		outData += dstPadding;
		inBuf += srcPadding;
	}

	//idLib::Printf( "\r100%%\n" );
}

/*
========================
idDxtEncoder::CompressImageDXT5HQ

params:	inBuf		- image to compress
paramO:	outBuf		- result of compression
params:	width		- width of image
params:	height		- height of image
========================
*/
void idDxtEncoder::CompressImageDXT5HQ( const byte* inBuf, byte* outBuf, int width, int height )
{
	ALIGN16( byte block[64] );
	byte alphaIndices1[6];
	byte alphaIndices2[6];
	unsigned int colorIndices;
	byte col1[4];
	byte col2[4];
	int error1;
	int error2;

	this->width = width;
	this->height = height;
	this->outData = outBuf;

	if( width > 4 && ( width & 3 ) != 0 )
	{
		return;
	}
	if( height > 4 && ( height & 3 ) != 0 )
	{
		return;
	}

	if( width < 4 || height < 4 )
	{
		WriteTinyColorDXT5( inBuf, width, height );
		return;
	}

	for( int j = 0; j < height; j += 4, inBuf += width * 4 * 4 )
	{
		for( int i = 0; i < width; i += 4 )
		{
			commonLocal.LoadPacifierBinarizeProgressIncrement( 16 );

			ExtractBlock( inBuf + i * 4, width, block );

			GetMinMaxColorsHQ( block, col1, col2, true );
			GetMinMaxAlphaHQ( block, 3, col1, col2 );

			// Write out alpha data. Try and find minimum error for the two encoding methods.
			error1 = FindAlphaIndices( block, 3, col1[3], col2[3], alphaIndices1 );
			error2 = FindAlphaIndices( block, 3, col2[3], col1[3], alphaIndices2 );

			if( error1 < error2 )
			{

				EmitByte( col1[3] );
				EmitByte( col2[3] );
				EmitByte( alphaIndices1[0] );
				EmitByte( alphaIndices1[1] );
				EmitByte( alphaIndices1[2] );
				EmitByte( alphaIndices1[3] );
				EmitByte( alphaIndices1[4] );
				EmitByte( alphaIndices1[5] );

			}
			else
			{

				EmitByte( col2[3] );
				EmitByte( col1[3] );
				EmitByte( alphaIndices2[0] );
				EmitByte( alphaIndices2[1] );
				EmitByte( alphaIndices2[2] );
				EmitByte( alphaIndices2[3] );
				EmitByte( alphaIndices2[4] );
				EmitByte( alphaIndices2[5] );
			}

#ifdef NVIDIA_7X_HARDWARE_BUG_FIX
			NV4XHardwareBugFix( col2, col1 );
#endif

			// Write out color data. Always take the path with 4 interpolated values.
			unsigned short scol1 = ColorTo565( col1 );
			unsigned short scol2 = ColorTo565( col2 );

			EmitUShort( scol1 );
			EmitUShort( scol2 );

			FindColorIndices( block, scol1, scol2, colorIndices );
			EmitUInt( colorIndices );

			//idLib::Printf( "\r%3d%%", ( j * width + i ) * 100 / ( width * height ) );
		}
		outData += dstPadding;
		inBuf += srcPadding;
	}

	//idLib::Printf( "\r100%%\n" );
}

/*
========================
idDxtEncoder::CompressImageCTX1HQ

params:	inBuf		- image to compress
paramO:	outBuf		- result of compression
params:	width		- width of image
params:	height		- height of image
========================
*/
void idDxtEncoder::CompressImageCTX1HQ( const byte* inBuf, byte* outBuf, int width, int height )
{
	ALIGN16( byte block[64] );
	unsigned int colorIndices;
	byte col1[4];
	byte col2[4];

	this->width = width;
	this->height = height;
	this->outData = outBuf;

	if( width > 4 && ( width & 3 ) != 0 )
	{
		return;
	}
	if( height > 4 && ( height & 3 ) != 0 )
	{
		return;
	}

	if( width < 4 || height < 4 )
	{
		WriteTinyColorCTX1DXT5A( inBuf, width, height );
		return;
	}

	for( int j = 0; j < height; j += 4, inBuf += width * 4 * 4 )
	{
		for( int i = 0; i < width; i += 4 )
		{
			commonLocal.LoadPacifierBinarizeProgressIncrement( 16 );

			ExtractBlock( inBuf + i * 4, width, block );

			GetMinMaxCTX1HQ( block, col1, col2 );

			EmitByte( col2[0] );
			EmitByte( col2[1] );
			EmitByte( col1[0] );
			EmitByte( col1[1] );

			FindCTX1Indices( block, col1, col2, colorIndices );
			EmitUInt( colorIndices );

			//idLib::Printf( "\r%3d%%", ( j * width + i ) * 100 / ( width * height ) );
		}
		outData += dstPadding;
		inBuf += srcPadding;
	}

	//idLib::Printf( "\r100%%\n" );
}

/*
========================
idDxtEncoder::ScaleYCoCg

params:	colorBlock	- 16 pixel block for which find color indexes
========================
*/
void idDxtEncoder::ScaleYCoCg( byte* colorBlock ) const
{
	ALIGN16( byte minColor[4] );
	ALIGN16( byte maxColor[4] );

	minColor[0] = minColor[1] = minColor[2] = minColor[3] = 255;
	maxColor[0] = maxColor[1] = maxColor[2] = maxColor[3] = 0;

	for( int i = 0; i < 16; i++ )
	{
		if( colorBlock[i * 4 + 0] < minColor[0] )
		{
			minColor[0] = colorBlock[i * 4 + 0];
		}
		if( colorBlock[i * 4 + 1] < minColor[1] )
		{
			minColor[1] = colorBlock[i * 4 + 1];
		}
		if( colorBlock[i * 4 + 0] > maxColor[0] )
		{
			maxColor[0] = colorBlock[i * 4 + 0];
		}
		if( colorBlock[i * 4 + 1] > maxColor[1] )
		{
			maxColor[1] = colorBlock[i * 4 + 1];
		}
	}

	int m0 = abs( minColor[0] - 128 );
	int m1 = abs( minColor[1] - 128 );
	int m2 = abs( maxColor[0] - 128 );
	int m3 = abs( maxColor[1] - 128 );

	if( m1 > m0 )
	{
		m0 = m1;
	}
	if( m3 > m2 )
	{
		m2 = m3;
	}
	if( m2 > m0 )
	{
		m0 = m2;
	}

	const int s0 = 128 / 2 - 1;
	const int s1 = 128 / 4 - 1;

	int scale = 1 + ( m0 <= s0 ) + 2 * ( m0 <= s1 );

	for( int i = 0; i < 16; i++ )
	{
		colorBlock[i * 4 + 0] = byte( ( colorBlock[i * 4 + 0] - 128 ) * scale + 128 );
		colorBlock[i * 4 + 1] = byte( ( colorBlock[i * 4 + 1] - 128 ) * scale + 128 );
		colorBlock[i * 4 + 2] = byte( ( scale - 1 ) << 3 );
	}
}

// LordHavoc begin

/*
========================
idDxtEncoder::ExtractBlockGimpDDS

Extract 4x4 BGRA block
========================
*/
void idDxtEncoder::ExtractBlockGimpDDS( const byte* src, int x, int y, int w, int h, byte* block )
{
	int i, j;
	int bw = MIN( w - x, 4 );
	int bh = MIN( h - y, 4 );
	int bx, by;
	const int rem[] =
	{
		0, 0, 0, 0,
		0, 1, 0, 1,
		0, 1, 2, 0,
		0, 1, 2, 3
	};

	for( i = 0; i < 4; i++ )
	{
		by = rem[( bh - 1 ) * 4 + i] + y;
		for( j = 0; j < 4; j++ )
		{
			bx = rem[( bw - 1 ) * 4 + j] + x;
			block[( i * 4 * 4 ) + ( j * 4 ) + 0] = src[( by * ( w * 4 ) ) + ( bx * 4 ) + 0];
			block[( i * 4 * 4 ) + ( j * 4 ) + 1] = src[( by * ( w * 4 ) ) + ( bx * 4 ) + 1];
			block[( i * 4 * 4 ) + ( j * 4 ) + 2] = src[( by * ( w * 4 ) ) + ( bx * 4 ) + 2];
			block[( i * 4 * 4 ) + ( j * 4 ) + 3] = src[( by * ( w * 4 ) ) + ( bx * 4 ) + 3];
		}
	}
}

/*
========================
idDxtEncoder::EncodeAlphaBlockBC3GimpDDS

Write DXT5 alpha block
========================
*/
void idDxtEncoder::EncodeAlphaBlockBC3GimpDDS( byte* dst, const byte* block, const int offset )
{
	int i, v, mn, mx;
	int dist, bias, dist2, dist4, bits, mask;
	int a, idx, t;

	block += offset;
	block += 3;
	// find min/max alpha pair
	mn = mx = block[0];
	for( i = 0; i < 16; i++ )
	{
		v = block[4 * i];
		if( v > mx )
		{
			mx = v;
		}
		if( v < mn )
		{
			mn = v;
		}
	}
	// encode them
	*dst++ = mx;
	*dst++ = mn;
	// determine bias and emit indices
	// given the choice of mx/mn, these indices are optimal:
	// http://fgiesen.wordpress.com/2009/12/15/dxt5-alpha-block-index-determination/
	dist = mx - mn;
	dist4 = dist * 4;
	dist2 = dist * 2;
	bias = ( dist < 8 ) ? ( dist - 1 ) : ( dist / 2 + 2 );
	bias -= mn * 7;
	bits = 0;
	mask = 0;
	for( i = 0; i < 16; i++ )
	{
		a = block[4 * i] * 7 + bias;
		// Select index. This is a "linear scale" lerp factor between 0 (val=min) and 7 (val=max).
		t = ( a >= dist4 ) ? -1 : 0;
		idx =  t & 4;
		a -= dist4 & t;
		t = ( a >= dist2 ) ? -1 : 0;
		idx += t & 2;
		a -= dist2 & t;
		idx += ( a >= dist );
		// turn linear scale into DXT index (0/1 are extremal pts)
		idx = -idx & 7;
		idx ^= ( 2 > idx );
		// write index
		mask |= idx << bits;
		if( ( bits += 3 ) >= 8 )
		{
			*dst++ = mask;
			mask >>= 8;
			bits -= 8;
		}
	}
}

/*
========================
idDxtEncoder::GetMinMaxYCoCgGimpDDS
========================
*/
void idDxtEncoder::GetMinMaxYCoCgGimpDDS( const byte* block, byte* mincolor, byte* maxcolor )
{
	int i;

	mincolor[2] = mincolor[1] = 255;
	maxcolor[2] = maxcolor[1] = 0;
	for( i = 0; i < 16; i++ )
	{
		if( block[4 * i + 2] < mincolor[2] )
		{
			mincolor[2] = block[4 * i + 2];
		}
		if( block[4 * i + 1] < mincolor[1] )
		{
			mincolor[1] = block[4 * i + 1];
		}
		if( block[4 * i + 2] > maxcolor[2] )
		{
			maxcolor[2] = block[4 * i + 2];
		}
		if( block[4 * i + 1] > maxcolor[1] )
		{
			maxcolor[1] = block[4 * i + 1];
		}
	}
}

/*
========================
idDxtEncoder::ScaleYCoCgGimpDDS
========================
*/
void idDxtEncoder::ScaleYCoCgGimpDDS( byte* block, byte* mincolor, byte* maxcolor )
{
	const int s0 = 128 / 2 - 1;
	const int s1 = 128 / 4 - 1;
	int m0, m1, m2, m3;
	int mask0, mask1, scale;
	int i;

	m0 = abs( mincolor[2] - 128 );
	m1 = abs( mincolor[1] - 128 );
	m2 = abs( maxcolor[2] - 128 );
	m3 = abs( maxcolor[1] - 128 );
	if( m1 > m0 )
	{
		m0 = m1;
	}
	if( m3 > m2 )
	{
		m2 = m3;
	}
	if( m2 > m0 )
	{
		m0 = m2;
	}

	mask0 = -( m0 <= s0 );
	mask1 = -( m0 <= s1 );
	scale = 1 + ( 1 & mask0 ) + ( 2 & mask1 );

	mincolor[2] = ( mincolor[2] - 128 ) * scale + 128;
	mincolor[1] = ( mincolor[1] - 128 ) * scale + 128;
	mincolor[0] = ( scale - 1 ) << 3;

	maxcolor[2] = ( maxcolor[2] - 128 ) * scale + 128;
	maxcolor[1] = ( maxcolor[1] - 128 ) * scale + 128;
	maxcolor[0] = ( scale - 1 ) << 3;

	for( i = 0; i < 16; i++ )
	{
		block[i * 4 + 2] = ( block[i * 4 + 2] - 128 ) * scale + 128;
		block[i * 4 + 1] = ( block[i * 4 + 1] - 128 ) * scale + 128;
	}
}

/*
========================
idDxtEncoder::InsetBBoxYCoCgGimpDDS
========================
*/
void idDxtEncoder::InsetBBoxYCoCgGimpDDS( byte* mincolor, byte* maxcolor )
{
	int inset[4], mini[4], maxi[4];

	inset[2] = ( maxcolor[2] - mincolor[2] ) - ( ( 1 << ( INSET_SHIFT - 1 ) ) - 1 );
	inset[1] = ( maxcolor[1] - mincolor[1] ) - ( ( 1 << ( INSET_SHIFT - 1 ) ) - 1 );

	mini[2] = ( ( mincolor[2] << INSET_SHIFT ) + inset[2] ) >> INSET_SHIFT;
	mini[1] = ( ( mincolor[1] << INSET_SHIFT ) + inset[1] ) >> INSET_SHIFT;

	maxi[2] = ( ( maxcolor[2] << INSET_SHIFT ) - inset[2] ) >> INSET_SHIFT;
	maxi[1] = ( ( maxcolor[1] << INSET_SHIFT ) - inset[1] ) >> INSET_SHIFT;

	mini[2] = ( mini[2] >= 0 ) ? mini[2] : 0;
	mini[1] = ( mini[1] >= 0 ) ? mini[1] : 0;

	maxi[2] = ( maxi[2] <= 255 ) ? maxi[2] : 255;
	maxi[1] = ( maxi[1] <= 255 ) ? maxi[1] : 255;

	mincolor[2] = ( mini[2] & 0xf8 ) | ( mini[2] >> 5 );
	mincolor[1] = ( mini[1] & 0xfc ) | ( mini[1] >> 6 );

	maxcolor[2] = ( maxi[2] & 0xf8 ) | ( maxi[2] >> 5 );
	maxcolor[1] = ( maxi[1] & 0xfc ) | ( maxi[1] >> 6 );
}

/*
========================
idDxtEncoder::SelectDiagonalYCoCgGimpDDS
========================
*/
void idDxtEncoder::SelectDiagonalYCoCgGimpDDS( const byte* block, byte* mincolor, byte* maxcolor )
{
	byte mid0, mid1, side, mask, b0, b1, c0, c1;
	int i;

	mid0 = ( ( int )mincolor[2] + maxcolor[2] + 1 ) >> 1;
	mid1 = ( ( int )mincolor[1] + maxcolor[1] + 1 ) >> 1;

	side = 0;
	for( i = 0; i < 16; i++ )
	{
		b0 = block[i * 4 + 2] >= mid0;
		b1 = block[i * 4 + 1] >= mid1;
		side += ( b0 ^ b1 );
	}

	mask = -( side > 8 );
	mask &= -( mincolor[2] != maxcolor[2] );

	c0 = mincolor[1];
	c1 = maxcolor[1];

	c0 ^= c1;
	c1 ^= c0 & mask;
	c0 ^= c1;

	mincolor[1] = c0;
	maxcolor[1] = c1;
}

/*
========================
idDxtEncoder::LerpRGB13GimpDDS

Linear interpolation at 1/3 point between a and b
========================
*/
void idDxtEncoder::LerpRGB13GimpDDS( byte* dst, byte* a, byte* b )
{
#if 0
	dst[0] = blerp( a[0], b[0], 0x55 );
	dst[1] = blerp( a[1], b[1], 0x55 );
	dst[2] = blerp( a[2], b[2], 0x55 );
#else
	// according to the S3TC/DX10 specs, this is the correct way to do the
	// interpolation (with no rounding bias)
	//
	// dst = ( 2 * a + b ) / 3;
	dst[0] = ( 2 * a[0] + b[0] ) / 3;
	dst[1] = ( 2 * a[1] + b[1] ) / 3;
	dst[2] = ( 2 * a[2] + b[2] ) / 3;
#endif
}

/*
========================
idDxtEncoder::Mul8BitGimpDDS
========================
*/
inline int idDxtEncoder::Mul8BitGimpDDS( int a, int b )
{
	int t = a * b + 128;
	return ( ( t + ( t >> 8 ) ) >> 8 );
}

/*
========================
idDxtEncoder::PackRGB565GimpDDS

Pack BGR8 to RGB565
========================
*/
inline unsigned short idDxtEncoder::PackRGB565GimpDDS( const byte* c )
{
	return( ( Mul8BitGimpDDS( c[2], 31 ) << 11 ) |
			( Mul8BitGimpDDS( c[1], 63 ) <<  5 ) |
			( Mul8BitGimpDDS( c[0], 31 ) ) );
}

/*
========================
idDxtEncoder::EncodeYCoCgBlockGimpDDS
========================
*/
void idDxtEncoder::EncodeYCoCgBlockGimpDDS( byte* dst, byte* block )
{
	byte colors[4][3], *maxcolor, *mincolor;
	unsigned int mask;
	int c0, c1, d0, d1, d2, d3;
	int b0, b1, b2, b3, b4;
	int x0, x1, x2;
	int i, idx;

	maxcolor = &colors[0][0];
	mincolor = &colors[1][0];

	GetMinMaxYCoCgGimpDDS( block, mincolor, maxcolor );
	ScaleYCoCgGimpDDS( block, mincolor, maxcolor );
	InsetBBoxYCoCgGimpDDS( mincolor, maxcolor );
	SelectDiagonalYCoCgGimpDDS( block, mincolor, maxcolor );

	LerpRGB13GimpDDS( &colors[2][0], maxcolor, mincolor );
	LerpRGB13GimpDDS( &colors[3][0], mincolor, maxcolor );

	mask = 0;

	for( i = 0; i < 16; i++ )
	{
		c0 = block[4 * i + 2];
		c1 = block[4 * i + 1];

		d0 = abs( colors[0][2] - c0 ) + abs( colors[0][1] - c1 );
		d1 = abs( colors[1][2] - c0 ) + abs( colors[1][1] - c1 );
		d2 = abs( colors[2][2] - c0 ) + abs( colors[2][1] - c1 );
		d3 = abs( colors[3][2] - c0 ) + abs( colors[3][1] - c1 );

		b0 = d0 > d3;
		b1 = d1 > d2;
		b2 = d0 > d2;
		b3 = d1 > d3;
		b4 = d2 > d3;

		x0 = b1 & b2;
		x1 = b0 & b3;
		x2 = b0 & b4;

		idx = ( x2 | ( ( x0 | x1 ) << 1 ) );

		mask |= idx << ( 2 * i );
	}

	PUTL16( dst + 0, PackRGB565GimpDDS( maxcolor ) );
	PUTL16( dst + 2, PackRGB565GimpDDS( mincolor ) );
	PUTL32( dst + 4, mask );
}

// LordHavoc end

/*
========================
idDxtEncoder::CompressYCoCgDXT5HQ

params:	inBuf		- image to compress
paramO:	outBuf		- result of compression
params:	width		- width of image
params:	height		- height of image
========================
*/
void idDxtEncoder::CompressYCoCgDXT5HQ( const byte* inBuf, byte* outBuf, int width, int height )
{
	ALIGN16( byte block[64] );
	byte alphaIndices1[6];
	byte alphaIndices2[6];
	unsigned int colorIndices;
	byte col1[4];
	byte col2[4];
	int error1;
	int error2;

	assert( HasConstantValuePer4x4Block( inBuf, width, height, 2 ) );

	this->width = width;
	this->height = height;
	this->outData = outBuf;

	if( width > 4 && ( width & 3 ) != 0 )
	{
		return;
	}
	if( height > 4 && ( height & 3 ) != 0 )
	{
		return;
	}

	if( width < 4 || height < 4 )
	{
		WriteTinyColorDXT5( inBuf, width, height );
		return;
	}

	for( int j = 0; j < height; j += 4, inBuf += width * 4 * 4 )
	{
		for( int i = 0; i < width; i += 4 )
		{
			commonLocal.LoadPacifierBinarizeProgressIncrement( 16 );

			ExtractBlock( inBuf + i * 4, width, block );
			ScaleYCoCg( block );

			GetMinMaxColorsHQ( block, col1, col2, true );
			GetMinMaxAlphaHQ( block, 3, col1, col2 );

			// Write out alpha data. Try and find minimum error for the two encoding methods.
			error1 = FindAlphaIndices( block, 3, col1[3], col2[3], alphaIndices1 );
			error2 = FindAlphaIndices( block, 3, col2[3], col1[3], alphaIndices2 );

			if( error1 < error2 )
			{

				EmitByte( col1[3] );
				EmitByte( col2[3] );
				EmitByte( alphaIndices1[0] );
				EmitByte( alphaIndices1[1] );
				EmitByte( alphaIndices1[2] );
				EmitByte( alphaIndices1[3] );
				EmitByte( alphaIndices1[4] );
				EmitByte( alphaIndices1[5] );

			}
			else
			{

				EmitByte( col2[3] );
				EmitByte( col1[3] );
				EmitByte( alphaIndices2[0] );
				EmitByte( alphaIndices2[1] );
				EmitByte( alphaIndices2[2] );
				EmitByte( alphaIndices2[3] );
				EmitByte( alphaIndices2[4] );
				EmitByte( alphaIndices2[5] );
			}

#ifdef NVIDIA_7X_HARDWARE_BUG_FIX
			NV4XHardwareBugFix( col2, col1 );
#endif

			// Write out color data. Always take the path with 4 interpolated values.
			unsigned short scol1 = ColorTo565( col1 );
			unsigned short scol2 = ColorTo565( col2 );

			EmitUShort( scol1 );
			EmitUShort( scol2 );

			FindColorIndices( block, scol1, scol2, colorIndices );
			EmitUInt( colorIndices );

			//idLib::Printf( "\r%3d%%", ( j * width + i ) * 100 / ( width * height ) );
		}
		outData += dstPadding;
		inBuf += srcPadding;
	}

	//idLib::Printf( "\r100%%\n" );
}

/*
========================
idDxtEncoder::CompressYCoCgCTX1DXT5AHQ

params:	inBuf		- image to compress
paramO:	outBuf		- result of compression
params:	width		- width of image
params:	height		- height of image
========================
*/
void idDxtEncoder::CompressYCoCgCTX1DXT5AHQ( const byte* inBuf, byte* outBuf, int width, int height )
{
	ALIGN16( byte block[64] );
	byte alphaIndices1[6];
	byte alphaIndices2[6];
	unsigned int colorIndices;
	byte col1[4];
	byte col2[4];
	int error1;
	int error2;

	assert( HasConstantValuePer4x4Block( inBuf, width, height, 2 ) );

	this->width = width;
	this->height = height;
	this->outData = outBuf;

	if( width > 4 && ( width & 3 ) != 0 )
	{
		return;
	}
	if( height > 4 && ( height & 3 ) != 0 )
	{
		return;
	}

	if( width < 4 || height < 4 )
	{
		WriteTinyColorCTX1DXT5A( inBuf, width, height );
		return;
	}

	for( int j = 0; j < height; j += 4, inBuf += width * 4 * 4 )
	{
		for( int i = 0; i < width; i += 4 )
		{
			commonLocal.LoadPacifierBinarizeProgressIncrement( 16 );

			ExtractBlock( inBuf + i * 4, width, block );

			GetMinMaxAlphaHQ( block, 3, col1, col2 );

			// Write out alpha data. Try and find minimum error for the two encoding methods.
			error1 = FindAlphaIndices( block, 3, col1[3], col2[3], alphaIndices1 );
			error2 = FindAlphaIndices( block, 3, col2[3], col1[3], alphaIndices2 );

			if( error1 < error2 )
			{

				EmitByte( col1[3] );
				EmitByte( col2[3] );
				EmitByte( alphaIndices1[0] );
				EmitByte( alphaIndices1[1] );
				EmitByte( alphaIndices1[2] );
				EmitByte( alphaIndices1[3] );
				EmitByte( alphaIndices1[4] );
				EmitByte( alphaIndices1[5] );

			}
			else
			{

				EmitByte( col2[3] );
				EmitByte( col1[3] );
				EmitByte( alphaIndices2[0] );
				EmitByte( alphaIndices2[1] );
				EmitByte( alphaIndices2[2] );
				EmitByte( alphaIndices2[3] );
				EmitByte( alphaIndices2[4] );
				EmitByte( alphaIndices2[5] );
			}

			GetMinMaxCTX1HQ( block, col1, col2 );

			EmitByte( col2[0] );
			EmitByte( col2[1] );
			EmitByte( col1[0] );
			EmitByte( col1[1] );

			FindCTX1Indices( block, col1, col2, colorIndices );
			EmitUInt( colorIndices );

			//idLib::Printf( "\r%3d%%", ( j * width + i ) * 100 / ( width * height ) );
		}
		outData += dstPadding;
		inBuf += srcPadding;
	}

	//idLib::Printf( "\r100%%\n" );
}

/*
========================
idDxtEncoder::RotateNormalsDXT1
========================
*/
void idDxtEncoder::RotateNormalsDXT1( byte* block ) const
{
	byte rotatedBlock[64];
	byte col1[4];
	byte col2[4];
	int bestError = MAX_TYPE( int );
	int bestRotation = 0;

	for( int i = 0; i < 32; i += 1 )
	{
		int r = ( i << 3 ) | ( i >> 2 );
		float angle = ( r / 255.0f ) * idMath::PI;
		float s = sin( angle );
		float c = cos( angle );

		for( int j = 0; j < 16; j++ )
		{
			float x = block[j * 4 + 0] / 255.0f * 2.0f - 1.0f;
			float y = block[j * 4 + 1] / 255.0f * 2.0f - 1.0f;
			float rx = c * x - s * y;
			float ry = s * x + c * y;
			rotatedBlock[j * 4 + 0] = idMath::Ftob( ( rx + 1.0f ) / 2.0f * 255.0f );
			rotatedBlock[j * 4 + 1] = idMath::Ftob( ( ry + 1.0f ) / 2.0f * 255.0f );
		}

		int error = GetMinMaxColorsHQ( rotatedBlock, col1, col2, true );
		if( error < bestError )
		{
			bestError = error;
			bestRotation = r;
		}
	}

	float angle = ( bestRotation / 255.0f ) * idMath::PI;
	float s = sin( angle );
	float c = cos( angle );

	for( int j = 0; j < 16; j++ )
	{
		float x = block[j * 4 + 0] / 255.0f * 2.0f - 1.0f;
		float y = block[j * 4 + 1] / 255.0f * 2.0f - 1.0f;
		float rx = c * x - s * y;
		float ry = s * x + c * y;
		block[j * 4 + 0] = idMath::Ftob( ( rx + 1.0f ) / 2.0f * 255.0f );
		block[j * 4 + 1] = idMath::Ftob( ( ry + 1.0f ) / 2.0f * 255.0f );
		block[j * 4 + 2] = ( byte )bestRotation;
	}
}

/*
========================
idDxtEncoder::CompressNormalMapDXT1HQ

params:	inBuf		- image to compress
paramO:	outBuf		- result of compression
params:	width		- width of image
params:	height		- height of image
========================
*/
void idDxtEncoder::CompressNormalMapDXT1HQ( const byte* inBuf, byte* outBuf, int width, int height )
{
	ALIGN16( byte block[64] );
	unsigned int colorIndices;
	byte col1[4];
	byte col2[4];

	this->width = width;
	this->height = height;
	this->outData = outBuf;

	if( width > 4 && ( width & 3 ) != 0 )
	{
		return;
	}
	if( height > 4 && ( height & 3 ) != 0 )
	{
		return;
	}

	if( width < 4 || height < 4 )
	{
		WriteTinyColorDXT1( inBuf, width, height );
		return;
	}

	for( int j = 0; j < height; j += 4, inBuf += width * 4 * 4 )
	{
		for( int i = 0; i < width; i += 4 )
		{
			commonLocal.LoadPacifierBinarizeProgressIncrement( 16 );

			ExtractBlock( inBuf + i * 4, width, block );

			for( int k = 0; k < 16; k++ )
			{
				block[k * 4 + 2] = 0;
			}

			GetMinMaxColorsHQ( block, col1, col2, true );

			// Write out color data. Always take the path with 4 interpolated values.
			unsigned short scol1 = ColorTo565( col1 );
			unsigned short scol2 = ColorTo565( col2 );

			EmitUShort( scol1 );
			EmitUShort( scol2 );

			FindColorIndices( block, scol1, scol2, colorIndices );
			EmitUInt( colorIndices );

			//idLib::Printf( "\r%3d%%", ( j * width + i * 4 ) * 100 / ( width * height ) );
		}
		outData += dstPadding;
		inBuf += srcPadding;
	}

	//idLib::Printf( "\r100%%\n" );
}

/*
========================
idDxtEncoder::CompressNormalMapDXT1RenormalizeHQ

params:	inBuf		- image to compress
paramO:	outBuf		- result of compression
params:	width		- width of image
params:	height		- height of image
========================
*/
void idDxtEncoder::CompressNormalMapDXT1RenormalizeHQ( const byte* inBuf, byte* outBuf, int width, int height )
{
	ALIGN16( byte block[64] );
	unsigned int colorIndices;
	byte col1[4];
	byte col2[4];

	this->width = width;
	this->height = height;
	this->outData = outBuf;

	if( width > 4 && ( width & 3 ) != 0 )
	{
		return;
	}
	if( height > 4 && ( height & 3 ) != 0 )
	{
		return;
	}

	if( width < 4 || height < 4 )
	{
		WriteTinyColorDXT1( inBuf, width, height );
		return;
	}

	for( int j = 0; j < height; j += 4, inBuf += width * 4 * 4 )
	{
		for( int i = 0; i < width; i += 4 )
		{
			commonLocal.LoadPacifierBinarizeProgressIncrement( 16 );

			ExtractBlock( inBuf + i * 4, width, block );

			// clear alpha channel
			for( int k = 0; k < 16; k++ )
			{
				block[k * 4 + 3] = 0;
			}

			GetMinMaxNormalsDXT1HQ( block, col1, col2, colorIndices, true );

			// Write out color data. Always take the path with 4 interpolated values.
			unsigned short scol1 = ColorTo565( col1 );
			unsigned short scol2 = ColorTo565( col2 );

			EmitUShort( scol1 );
			EmitUShort( scol2 );
			EmitUInt( colorIndices );

			////idLib::Printf( "\r%3d%%", ( j * width + i * 4 ) * 100 / ( width * height ) );
		}
		outData += dstPadding;
		inBuf += srcPadding;
	}

	////idLib::Printf( "\r100%%\n" );
}

#define USE_SCALE		1
#define USE_BIAS		1

static int c_blocks;
static int c_scaled;
static int c_scaled2x;
static int c_scaled4x;
static int c_differentBias;
static int c_biasHelped;

/*
========================
idDxtEncoder::BiasScaleNormalY

	* scale2x = 33%
	* scale4x = 23%
	* bias + scale2x = 30%
	* bias + scale4x = 55%
========================
*/
void idDxtEncoder::BiasScaleNormalY( byte* colorBlock ) const
{

	byte minColor = 255;
	byte maxColor = 0;

	for( int i = 0; i < 16; i++ )
	{
		if( colorBlock[i * 4 + 1] < minColor )
		{
			minColor = colorBlock[i * 4 + 1];
		}
		if( colorBlock[i * 4 + 1] > maxColor )
		{
			maxColor = colorBlock[i * 4 + 1];
		}
	}

	int bestBias = 128;
	int bestRange = Max( abs( minColor - bestBias ), abs( maxColor - bestBias ) );
#if USE_BIAS
	for( int i = 0; i < 32; i++ )
	{
		int bias = ( ( i << 3 ) | ( i >> 2 ) ) - 4;
		int range = Max( abs( minColor - bias ), abs( maxColor - bias ) );
		if( range < bestRange )
		{
			bestRange = range;
			bestBias = bias;
		}
	}
#endif

	const int s0 = 128 / 2 - 1;
	const int s1 = 128 / 4 - 1;

#if USE_SCALE
	int scale = 1 + ( bestRange <= s0 ) + 2 * ( bestRange <= s1 );
#else
	int scale = 1;
#endif

	if( scale == 1 )
	{
		bestBias = 128;
	}
	else
	{
		c_scaled++;
		if( scale == 2 )
		{
			c_scaled2x++;
		}
		if( scale == 4 )
		{
			c_scaled4x++;
		}
		if( bestBias != 128 )
		{
			c_differentBias++;
			int r = Max( abs( minColor - 128 ), abs( maxColor - 128 ) );
			int s = 1 + ( r <= s0 ) + 2 * ( r <= s1 );
			if( scale > s )
			{
				c_biasHelped++;
			}
		}
	}

	c_blocks++;

	for( int i = 0; i < 16; i++ )
	{
		colorBlock[i * 4 + 0] = byte( bestBias + 4 );
		colorBlock[i * 4 + 1] = byte( ( colorBlock[i * 4 + 1] - bestBias ) * scale + 128 );
		colorBlock[i * 4 + 2] = byte( ( scale - 1 ) << 3 );
	}
}

/*
========================
idDxtEncoder::RotateNormalsDXT5
========================
*/
void idDxtEncoder::RotateNormalsDXT5( byte* block ) const
{
	byte rotatedBlock[64];
	byte col1[4];
	byte col2[4];
	int bestError = MAX_TYPE( int );
	int bestRotation = 0;
	int bestScale = 1;

	for( int i = 0; i < 32; i += 1 )
	{
		int r = ( i << 3 ) | ( i >> 2 );
		float angle = ( r / 255.0f ) * idMath::PI;
		float s = sin( angle );
		float c = cos( angle );

		for( int j = 0; j < 16; j++ )
		{
			float x = block[j * 4 + 3] / 255.0f * 2.0f - 1.0f;
			float y = block[j * 4 + 1] / 255.0f * 2.0f - 1.0f;
			float rx = c * x - s * y;
			float ry = s * x + c * y;
			rotatedBlock[j * 4 + 3] = idMath::Ftob( ( rx + 1.0f ) / 2.0f * 255.0f );
			rotatedBlock[j * 4 + 1] = idMath::Ftob( ( ry + 1.0f ) / 2.0f * 255.0f );
		}

#if USE_SCALE
		byte minColor = 255;
		byte maxColor = 0;

		for( int j = 0; j < 16; j++ )
		{
			if( rotatedBlock[j * 4 + 1] < minColor )
			{
				minColor = rotatedBlock[j * 4 + 1];
			}
			if( rotatedBlock[j * 4 + 1] > maxColor )
			{
				maxColor = rotatedBlock[j * 4 + 1];
			}
		}

		const int s0 = 128 / 2 - 1;
		const int s1 = 128 / 4 - 1;

		int range = Max( abs( minColor - 128 ), abs( maxColor - 128 ) );
		int scale = 1 + ( range <= s0 ) + 2 * ( range <= s1 );

		for( int j = 0; j < 16; j++ )
		{
			rotatedBlock[j * 4 + 1] = byte( ( rotatedBlock[j * 4 + 1] - 128 ) * scale + 128 );
		}
#endif

		int errorY = GetMinMaxNormalYHQ( rotatedBlock, col1, col2, true, scale );
		int errorX = GetMinMaxAlphaHQ( rotatedBlock, 3, col1, col2 );
		int error = errorX + errorY;
		if( error < bestError )
		{
			bestError = error;
			bestRotation = r;
			bestScale = scale;
		}
	}

	float angle = ( bestRotation / 255.0f ) * idMath::PI;
	float s = sin( angle );
	float c = cos( angle );

	for( int j = 0; j < 16; j++ )
	{
		float x = block[j * 4 + 3] / 255.0f * 2.0f - 1.0f;
		float y = block[j * 4 + 1] / 255.0f * 2.0f - 1.0f;
		float rx = c * x - s * y;
		float ry = s * x + c * y;
		block[j * 4 + 0] = ( byte )bestRotation;
		block[j * 4 + 1] = idMath::Ftob( ( ry + 1.0f ) / 2.0f * 255.0f );
		block[j * 4 + 3] = idMath::Ftob( ( rx + 1.0f ) / 2.0f * 255.0f );

#if USE_SCALE
		block[j * 4 + 1] = byte( ( block[j * 4 + 1] - 128 ) * bestScale + 128 );
		block[j * 4 + 2] = byte( ( bestScale - 1 ) << 3 );
#endif
	}
}

/*
========================
idDxtEncoder::CompressNormalMapDXT5HQ

params:	inBuf		- image to compress
paramO:	outBuf		- result of compression
params:	width		- width of image
params:	height		- height of image
========================
*/
void idDxtEncoder::CompressNormalMapDXT5HQ( const byte* inBuf, byte* outBuf, int width, int height )
{
	ALIGN16( byte block[64] );
	byte alphaIndices1[6];
	byte alphaIndices2[6];
	unsigned int colorIndices;
	byte col1[4];
	byte col2[4];
	int error1;
	int error2;

	this->width = width;
	this->height = height;
	this->outData = outBuf;

	if( width > 4 && ( width & 3 ) != 0 )
	{
		return;
	}
	if( height > 4 && ( height & 3 ) != 0 )
	{
		return;
	}

	if( width < 4 || height < 4 )
	{
		WriteTinyColorDXT5( inBuf, width, height );
		return;
	}

	for( int j = 0; j < height; j += 4, inBuf += width * 4 * 4 )
	{
		for( int i = 0; i < width; i += 4 )
		{
			commonLocal.LoadPacifierBinarizeProgressIncrement( 16 );

			ExtractBlock( inBuf + i * 4, width, block );

			// swizzle components
			for( int k = 0; k < 16; k++ )
			{
				block[k * 4 + 3] = block[k * 4 + 0];
				block[k * 4 + 0] = 0;
				block[k * 4 + 2] = 0;
			}

			//BiasScaleNormalY( block );
			//RotateNormalsDXT5( block );

			GetMinMaxNormalYHQ( block, col1, col2, true, 1 );
			GetMinMaxAlphaHQ( block, 3, col1, col2 );

			// Write out alpha data. Try and find minimum error for the two encoding methods.
			error1 = FindAlphaIndices( block, 3, col1[3], col2[3], alphaIndices1 );
			error2 = FindAlphaIndices( block, 3, col2[3], col1[3], alphaIndices2 );

			if( error1 < error2 )
			{

				EmitByte( col1[3] );
				EmitByte( col2[3] );
				EmitByte( alphaIndices1[0] );
				EmitByte( alphaIndices1[1] );
				EmitByte( alphaIndices1[2] );
				EmitByte( alphaIndices1[3] );
				EmitByte( alphaIndices1[4] );
				EmitByte( alphaIndices1[5] );

			}
			else
			{

				EmitByte( col2[3] );
				EmitByte( col1[3] );
				EmitByte( alphaIndices2[0] );
				EmitByte( alphaIndices2[1] );
				EmitByte( alphaIndices2[2] );
				EmitByte( alphaIndices2[3] );
				EmitByte( alphaIndices2[4] );
				EmitByte( alphaIndices2[5] );
			}

#ifdef NVIDIA_7X_HARDWARE_BUG_FIX
			NV4XHardwareBugFix( col2, col1 );
#endif

			// Write out color data. Always take the path with 4 interpolated values.
			unsigned short scol1 = ColorTo565( col1 );
			unsigned short scol2 = ColorTo565( col2 );

			EmitUShort( scol1 );
			EmitUShort( scol2 );

			FindColorIndices( block, scol1, scol2, colorIndices );
			EmitUInt( colorIndices );

			//idLib::Printf( "\r%3d%%", ( j * width + i ) * 100 / ( width * height ) );
		}
		outData += dstPadding;
		inBuf += srcPadding;
	}

	//idLib::Printf( "\r100%%\n" );
}

/*
========================
idDxtEncoder::CompressNormalMapDXT5RenormalizeHQ

params:	inBuf		- image to compress
paramO:	outBuf		- result of compression
params:	width		- width of image
params:	height		- height of image
========================
*/
void idDxtEncoder::CompressNormalMapDXT5RenormalizeHQ( const byte* inBuf, byte* outBuf, int width, int height )
{
	ALIGN16( byte block[64] );
	unsigned int colorIndices;
	byte alphaIndices[6];
	byte col1[4];
	byte col2[4];

	this->width = width;
	this->height = height;
	this->outData = outBuf;

	if( width > 4 && ( width & 3 ) != 0 )
	{
		return;
	}
	if( height > 4 && ( height & 3 ) != 0 )
	{
		return;
	}

	if( width < 4 || height < 4 )
	{
		WriteTinyColorDXT5( inBuf, width, height );
		return;
	}

	for( int j = 0; j < height; j += 4, inBuf += width * 4 * 4 )
	{
		for( int i = 0; i < width; i += 4 )
		{
			commonLocal.LoadPacifierBinarizeProgressIncrement( 16 );

			ExtractBlock( inBuf + i * 4, width, block );

			// swizzle components
			for( int k = 0; k < 16; k++ )
			{
#if 0 // object-space
				block[k * 4 + 3] = block[k * 4 + 2];
				block[k * 4 + 2] = 0;
#else
				block[k * 4 + 3] = block[k * 4 + 0];
				block[k * 4 + 0] = 0;
#endif
			}

			GetMinMaxNormalsDXT5HQFast( block, col1, col2, colorIndices, alphaIndices );

			EmitByte( col2[3] );
			EmitByte( col1[3] );
			EmitByte( alphaIndices[0] );
			EmitByte( alphaIndices[1] );
			EmitByte( alphaIndices[2] );
			EmitByte( alphaIndices[3] );
			EmitByte( alphaIndices[4] );
			EmitByte( alphaIndices[5] );

			unsigned short scol1 = ColorTo565( col1 );
			unsigned short scol2 = ColorTo565( col2 );

			EmitUShort( scol2 );
			EmitUShort( scol1 );
			EmitUInt( colorIndices );

			////idLib::Printf( "\r%3d%%", ( j * width + i ) * 100 / ( width * height ) );
		}
		outData += dstPadding;
		inBuf += srcPadding;

	}

	////idLib::Printf( "\r100%%\n" );
}

/*
========================
idDxtEncoder::CompressNormalMapDXN2HQ

params:	inBuf		- image to compress
paramO:	outBuf		- result of compression
params:	width		- width of image
params:	height		- height of image
========================
*/
void idDxtEncoder::CompressNormalMapDXN2HQ( const byte* inBuf, byte* outBuf, int width, int height )
{
	ALIGN16( byte block[64] );
	byte alphaIndices1[6];
	byte alphaIndices2[6];
	byte col1[4];
	byte col2[4];
	int error1;
	int error2;

	this->width = width;
	this->height = height;
	this->outData = outBuf;

	if( width > 4 && ( width & 3 ) != 0 )
	{
		return;
	}
	if( height > 4 && ( height & 3 ) != 0 )
	{
		return;
	}

	if( width < 4 || height < 4 )
	{
		WriteTinyColorDXT5( inBuf, width, height );
		return;
	}

	for( int j = 0; j < height; j += 4, inBuf += width * 4 * 4 )
	{
		for( int i = 0; i < width; i += 4 )
		{
			commonLocal.LoadPacifierBinarizeProgressIncrement( 16 );

			ExtractBlock( inBuf + i * 4, width, block );

			for( int k = 0; k < 2; k++ )
			{
				GetMinMaxAlphaHQ( block, k, col1, col2 );

				// Write out alpha data. Try and find minimum error for the two encoding methods.
				error1 = FindAlphaIndices( block, k, col1[k], col2[k], alphaIndices1 );
				error2 = FindAlphaIndices( block, k, col2[k], col1[k], alphaIndices2 );

				if( error1 < error2 )
				{

					EmitByte( col1[k] );
					EmitByte( col2[k] );
					EmitByte( alphaIndices1[0] );
					EmitByte( alphaIndices1[1] );
					EmitByte( alphaIndices1[2] );
					EmitByte( alphaIndices1[3] );
					EmitByte( alphaIndices1[4] );
					EmitByte( alphaIndices1[5] );

				}
				else
				{

					EmitByte( col2[k] );
					EmitByte( col1[k] );
					EmitByte( alphaIndices2[0] );
					EmitByte( alphaIndices2[1] );
					EmitByte( alphaIndices2[2] );
					EmitByte( alphaIndices2[3] );
					EmitByte( alphaIndices2[4] );
					EmitByte( alphaIndices2[5] );
				}
			}

			//idLib::Printf( "\r%3d%%", ( j * width + i ) * 100 / ( width * height ) );
		}
		outData += dstPadding;
		inBuf += srcPadding;
	}

	//idLib::Printf( "\r100%%\n" );
}

/*
========================
idDxtEncoder::GetMinMaxBBox

Takes the extents of the bounding box of the colors in the 4x4 block in RGB space.
Also finds the minimum and maximum alpha values.

params:	colorBlock	- 4*4 input tile, 4 bytes per pixel
paramO:	minColor	- 4 byte Min color found
paramO:	maxColor	- 4 byte Max color found
========================
*/
ID_INLINE void idDxtEncoder::GetMinMaxBBox( const byte* colorBlock, byte* minColor, byte* maxColor ) const
{

	minColor[0] = minColor[1] = minColor[2] = minColor[3] = 255;
	maxColor[0] = maxColor[1] = maxColor[2] = maxColor[3] = 0;

	for( int i = 0; i < 16; i++ )
	{
		if( colorBlock[i * 4 + 0] < minColor[0] )
		{
			minColor[0] = colorBlock[i * 4 + 0];
		}
		if( colorBlock[i * 4 + 1] < minColor[1] )
		{
			minColor[1] = colorBlock[i * 4 + 1];
		}
		if( colorBlock[i * 4 + 2] < minColor[2] )
		{
			minColor[2] = colorBlock[i * 4 + 2];
		}
		if( colorBlock[i * 4 + 3] < minColor[3] )
		{
			minColor[3] = colorBlock[i * 4 + 3];
		}
		if( colorBlock[i * 4 + 0] > maxColor[0] )
		{
			maxColor[0] = colorBlock[i * 4 + 0];
		}
		if( colorBlock[i * 4 + 1] > maxColor[1] )
		{
			maxColor[1] = colorBlock[i * 4 + 1];
		}
		if( colorBlock[i * 4 + 2] > maxColor[2] )
		{
			maxColor[2] = colorBlock[i * 4 + 2];
		}
		if( colorBlock[i * 4 + 3] > maxColor[3] )
		{
			maxColor[3] = colorBlock[i * 4 + 3];
		}
	}
}

/*
========================
idDxtEncoder::InsetColorsBBox
========================
*/
ID_INLINE void idDxtEncoder::InsetColorsBBox( byte* minColor, byte* maxColor ) const
{
	byte inset[4];

	inset[0] = ( maxColor[0] - minColor[0] ) >> INSET_COLOR_SHIFT;
	inset[1] = ( maxColor[1] - minColor[1] ) >> INSET_COLOR_SHIFT;
	inset[2] = ( maxColor[2] - minColor[2] ) >> INSET_COLOR_SHIFT;
	inset[3] = ( maxColor[3] - minColor[3] ) >> INSET_ALPHA_SHIFT;

	minColor[0] = ( minColor[0] + inset[0] <= 255 ) ? minColor[0] + inset[0] : 255;
	minColor[1] = ( minColor[1] + inset[1] <= 255 ) ? minColor[1] + inset[1] : 255;
	minColor[2] = ( minColor[2] + inset[2] <= 255 ) ? minColor[2] + inset[2] : 255;
	minColor[3] = ( minColor[3] + inset[3] <= 255 ) ? minColor[3] + inset[3] : 255;

	maxColor[0] = ( maxColor[0] >= inset[0] ) ? maxColor[0] - inset[0] : 0;
	maxColor[1] = ( maxColor[1] >= inset[1] ) ? maxColor[1] - inset[1] : 0;
	maxColor[2] = ( maxColor[2] >= inset[2] ) ? maxColor[2] - inset[2] : 0;
	maxColor[3] = ( maxColor[3] >= inset[3] ) ? maxColor[3] - inset[3] : 0;
}

/*
========================
idDxtEncoder::SelectColorsDiagonal
========================
*/
void idDxtEncoder::SelectColorsDiagonal( const byte* colorBlock, byte* minColor, byte* maxColor ) const
{

	byte mid0 = byte( ( ( int ) minColor[0] + maxColor[0] + 1 ) >> 1 );
	byte mid1 = byte( ( ( int ) minColor[1] + maxColor[1] + 1 ) >> 1 );
	byte mid2 = byte( ( ( int ) minColor[2] + maxColor[2] + 1 ) >> 1 );

#if 0

	// using the covariance is the best way to select the diagonal
	int side0 = 0;
	int side1 = 0;
	for( int i = 0; i < 16; i++ )
	{
		int b0 = colorBlock[i * 4 + 0] - mid0;
		int b1 = colorBlock[i * 4 + 1] - mid1;
		int b2 = colorBlock[i * 4 + 2] - mid2;
		side0 += ( b0 * b1 );
		side1 += ( b1 * b2 );
	}
	byte mask0 = -( side0 < 0 );
	byte mask1 = -( side1 < 0 );

#else

	// calculating the covariance of just the sign bits is much faster and gives almost the same result
	int side0 = 0;
	int side1 = 0;
	for( int i = 0; i < 16; i++ )
	{
		byte b0 = colorBlock[i * 4 + 0] >= mid0;
		byte b1 = colorBlock[i * 4 + 1] >= mid1;
		byte b2 = colorBlock[i * 4 + 2] >= mid2;
		side0 += ( b0 ^ b1 );
		side1 += ( b1 ^ b2 );
	}
	byte mask0 = -( side0 > 8 );
	byte mask1 = -( side1 > 8 );

#endif

	byte c0 = minColor[0];
	byte c1 = maxColor[0];
	byte c2 = minColor[2];
	byte c3 = maxColor[2];

	c0 ^= c1;
	mask0 &= c0;
	c1 ^= mask0;
	c0 ^= c1;

	c2 ^= c3;
	mask1 &= c2;
	c3 ^= mask1;
	c2 ^= c3;

	minColor[0] = c0;
	maxColor[0] = c1;
	minColor[2] = c2;
	maxColor[2] = c3;

	if( ColorTo565( minColor ) > ColorTo565( maxColor ) )
	{
		SwapValues( minColor[0], maxColor[0] );
		SwapValues( minColor[1], maxColor[1] );
		SwapValues( minColor[2], maxColor[2] );
	}
}

/*
========================
idDxtEncoder::EmitColorIndices

params:	colorBlock	- 16 pixel block for which find color indexes
paramO:	minColor	- Min color found
paramO:	maxColor	- Max color found
return: 4 byte color index block
========================
*/
void idDxtEncoder::EmitColorIndices( const byte* colorBlock, const byte* minColor, const byte* maxColor )
{
#if 1

	ALIGN16( uint16 colors[4][4] );
	unsigned int result = 0;

	colors[0][0] = ( maxColor[0] & C565_5_MASK ) | ( maxColor[0] >> 5 );
	colors[0][1] = ( maxColor[1] & C565_6_MASK ) | ( maxColor[1] >> 6 );
	colors[0][2] = ( maxColor[2] & C565_5_MASK ) | ( maxColor[2] >> 5 );
	colors[0][3] = 0;
	colors[1][0] = ( minColor[0] & C565_5_MASK ) | ( minColor[0] >> 5 );
	colors[1][1] = ( minColor[1] & C565_6_MASK ) | ( minColor[1] >> 6 );
	colors[1][2] = ( minColor[2] & C565_5_MASK ) | ( minColor[2] >> 5 );
	colors[1][3] = 0;
	colors[2][0] = ( 2 * colors[0][0] + 1 * colors[1][0] ) / 3;
	colors[2][1] = ( 2 * colors[0][1] + 1 * colors[1][1] ) / 3;
	colors[2][2] = ( 2 * colors[0][2] + 1 * colors[1][2] ) / 3;
	colors[2][3] = 0;
	colors[3][0] = ( 1 * colors[0][0] + 2 * colors[1][0] ) / 3;
	colors[3][1] = ( 1 * colors[0][1] + 2 * colors[1][1] ) / 3;
	colors[3][2] = ( 1 * colors[0][2] + 2 * colors[1][2] ) / 3;
	colors[3][3] = 0;

	// uses sum of absolute differences instead of squared distance to find the best match
	for( int i = 15; i >= 0; i-- )
	{
		int c0, c1, c2, c3, m, d0, d1, d2, d3;

		c0 = colorBlock[i * 4 + 0];
		c1 = colorBlock[i * 4 + 1];
		c2 = colorBlock[i * 4 + 2];
		c3 = colorBlock[i * 4 + 3];

		m = colors[0][0] - c0;
		d0 = abs( m );
		m = colors[1][0] - c0;
		d1 = abs( m );
		m = colors[2][0] - c0;
		d2 = abs( m );
		m = colors[3][0] - c0;
		d3 = abs( m );

		m = colors[0][1] - c1;
		d0 += abs( m );
		m = colors[1][1] - c1;
		d1 += abs( m );
		m = colors[2][1] - c1;
		d2 += abs( m );
		m = colors[3][1] - c1;
		d3 += abs( m );

		m = colors[0][2] - c2;
		d0 += abs( m );
		m = colors[1][2] - c2;
		d1 += abs( m );
		m = colors[2][2] - c2;
		d2 += abs( m );
		m = colors[3][2] - c2;
		d3 += abs( m );

#if 0
		int b0 = d0 > d2;
		int b1 = d1 > d3;
		int b2 = d0 > d3;
		int b3 = d1 > d2;
		int b4 = d0 > d1;
		int b5 = d2 > d3;

		result |= ( ( !b3 & b4 ) | ( b2 & b5 ) | ( ( ( b0 & b3 ) | ( b1 & b2 ) ) << 1 ) ) << ( i << 1 );
#else
		bool b0 = d0 > d3;
		bool b1 = d1 > d2;
		bool b2 = d0 > d2;
		bool b3 = d1 > d3;
		bool b4 = d2 > d3;

		int x0 = b1 & b2;
		int x1 = b0 & b3;
		int x2 = b0 & b4;

		result |= ( x2 | ( ( x0 | x1 ) << 1 ) ) << ( i << 1 );
#endif
	}

	EmitUInt( result );

#elif 1

	byte colors[4][4];
	unsigned int indexes[16];

	colors[0][0] = ( maxColor[0] & C565_5_MASK ) | ( maxColor[0] >> 6 );
	colors[0][1] = ( maxColor[1] & C565_6_MASK ) | ( maxColor[1] >> 5 );
	colors[0][2] = ( maxColor[2] & C565_5_MASK ) | ( maxColor[2] >> 6 );
	colors[0][3] = 0;
	colors[1][0] = ( minColor[0] & C565_5_MASK ) | ( minColor[0] >> 6 );
	colors[1][1] = ( minColor[1] & C565_6_MASK ) | ( minColor[1] >> 5 );
	colors[1][2] = ( minColor[2] & C565_5_MASK ) | ( minColor[2] >> 6 );
	colors[1][3] = 0;
	colors[2][0] = ( 2 * colors[0][0] + 1 * colors[1][0] ) / 3;
	colors[2][1] = ( 2 * colors[0][1] + 1 * colors[1][1] ) / 3;
	colors[2][2] = ( 2 * colors[0][2] + 1 * colors[1][2] ) / 3;
	colors[2][3] = 0;
	colors[3][0] = ( 1 * colors[0][0] + 2 * colors[1][0] ) / 3;
	colors[3][1] = ( 1 * colors[0][1] + 2 * colors[1][1] ) / 3;
	colors[3][2] = ( 1 * colors[0][2] + 2 * colors[1][2] ) / 3;
	colors[3][3] = 0;

	for( int i = 0; i < 16; i++ )
	{
		int c0, c1, c2, m, d, minDist;

		c0 = colorBlock[i * 4 + 0];
		c1 = colorBlock[i * 4 + 1];
		c2 = colorBlock[i * 4 + 2];

		m = colors[0][0] - c0;
		d = m * m;
		m = colors[0][1] - c1;
		d += m * m;
		m = colors[0][2] - c2;
		d += m * m;

		minDist = d;
		indexes[i] = 0;

		m = colors[1][0] - c0;
		d = m * m;
		m = colors[1][1] - c1;
		d += m * m;
		m = colors[1][2] - c2;
		d += m * m;

		if( d < minDist )
		{
			minDist = d;
			indexes[i] = 1;
		}

		m = colors[2][0] - c0;
		d = m * m;
		m = colors[2][1] - c1;
		d += m * m;
		m = colors[2][2] - c2;
		d += m * m;

		if( d < minDist )
		{
			minDist = d;
			indexes[i] = 2;
		}

		m = colors[3][0] - c0;
		d = m * m;
		m = colors[3][1] - c1;
		d += m * m;
		m = colors[3][2] - c2;
		d += m * m;

		if( d < minDist )
		{
			minDist = d;
			indexes[i] = 3;
		}
	}

	unsigned int result = 0;
	for( int i = 0; i < 16; i++ )
	{
		result |= ( indexes[i] << ( unsigned int )( i << 1 ) );
	}

	EmitUInt( result );

#else

	byte colors[4][4];
	unsigned int indexes[16];

	colors[0][0] = ( maxColor[0] & C565_5_MASK ) | ( maxColor[0] >> 6 );
	colors[0][1] = ( maxColor[1] & C565_6_MASK ) | ( maxColor[1] >> 5 );
	colors[0][2] = ( maxColor[2] & C565_5_MASK ) | ( maxColor[2] >> 6 );
	colors[0][3] = 0;
	colors[1][0] = ( minColor[0] & C565_5_MASK ) | ( minColor[0] >> 6 );
	colors[1][1] = ( minColor[1] & C565_6_MASK ) | ( minColor[1] >> 5 );
	colors[1][2] = ( minColor[2] & C565_5_MASK ) | ( minColor[2] >> 6 );
	colors[1][3] = 0;
	colors[2][0] = ( 2 * colors[0][0] + 1 * colors[1][0] ) / 3;
	colors[2][1] = ( 2 * colors[0][1] + 1 * colors[1][1] ) / 3;
	colors[2][2] = ( 2 * colors[0][2] + 1 * colors[1][2] ) / 3;
	colors[2][3] = 0;
	colors[3][0] = ( 1 * colors[0][0] + 2 * colors[1][0] ) / 3;
	colors[3][1] = ( 1 * colors[0][1] + 2 * colors[1][1] ) / 3;
	colors[3][2] = ( 1 * colors[0][2] + 2 * colors[1][2] ) / 3;
	colors[3][3] = 0;

	for( int i = 0; i < 16; i++ )
	{
		unsigned int minDist = ( 255 * 255 ) * 4;
		for( int j = 0; j < 4; j++ )
		{
			unsigned int dist = ColorDistance( &colorBlock[i * 4], &colors[j][0] );
			if( dist < minDist )
			{
				minDist = dist;
				indexes[i] = j;
			}
		}
	}

	unsigned int result = 0;
	for( int i = 0; i < 16; i++ )
	{
		result |= ( indexes[i] << ( unsigned int )( i << 1 ) );
	}

	EmitUInt( result );

#endif
}

/*
========================
idDxtEncoder::EmitColorAlphaIndices

params:	colorBlock	- 16 pixel block for which find color indexes
paramO:	minColor	- Min color found
paramO:	maxColor	- Max color found
return: 4 byte color index block
========================
*/
void idDxtEncoder::EmitColorAlphaIndices( const byte* colorBlock, const byte* minColor, const byte* maxColor )
{
	ALIGN16( uint16 colors[4][4] );
	unsigned int result = 0;

	colors[0][0] = ( minColor[0] & C565_5_MASK ) | ( minColor[0] >> 5 );
	colors[0][1] = ( minColor[1] & C565_6_MASK ) | ( minColor[1] >> 6 );
	colors[0][2] = ( minColor[2] & C565_5_MASK ) | ( minColor[2] >> 5 );
	colors[0][3] = 255;
	colors[1][0] = ( maxColor[0] & C565_5_MASK ) | ( maxColor[0] >> 5 );
	colors[1][1] = ( maxColor[1] & C565_6_MASK ) | ( maxColor[1] >> 6 );
	colors[1][2] = ( maxColor[2] & C565_5_MASK ) | ( maxColor[2] >> 5 );
	colors[1][3] = 255;
	colors[2][0] = ( colors[0][0] + colors[1][0] ) / 2;
	colors[2][1] = ( colors[0][1] + colors[1][1] ) / 2;
	colors[2][2] = ( colors[0][2] + colors[1][2] ) / 2;
	colors[2][3] = 255;
	colors[3][0] = 0;
	colors[3][1] = 0;
	colors[3][2] = 0;
	colors[3][3] = 0;

	// uses sum of absolute differences instead of squared distance to find the best match
	for( int i = 15; i >= 0; i-- )
	{
		int c0, c1, c2, c3, m, d0, d1, d2;

		c0 = colorBlock[i * 4 + 0];
		c1 = colorBlock[i * 4 + 1];
		c2 = colorBlock[i * 4 + 2];
		c3 = colorBlock[i * 4 + 3];

		m = colors[0][0] - c0;
		d0 = abs( m );
		m = colors[1][0] - c0;
		d1 = abs( m );
		m = colors[2][0] - c0;
		d2 = abs( m );

		m = colors[0][1] - c1;
		d0 += abs( m );
		m = colors[1][1] - c1;
		d1 += abs( m );
		m = colors[2][1] - c1;
		d2 += abs( m );

		m = colors[0][2] - c2;
		d0 += abs( m );
		m = colors[1][2] - c2;
		d1 += abs( m );
		m = colors[2][2] - c2;
		d2 += abs( m );

		unsigned int b0 = d2 > d0;
		unsigned int b1 = d2 > d1;
		unsigned int b2 = d1 > d0;
		unsigned int b3 = c3 < 128;

		// DG: add some parenthesis to appease (often rightly) warning compiler
		result |= ( ( ( ( b0 & b1 ) | b3 ) << 1 ) | ( ( b2 ^ b1 ) | b3 ) ) << ( i << 1 );
		// DG end
	}

	EmitUInt( result );
}

/*
========================
idDxtEncoder::EmitCTX1Indices

params:	colorBlock	- 16 pixel block for which find color indexes
paramO:	minColor	- Min color found
paramO:	maxColor	- Max color found
return: 4 byte color index block
========================
*/
void idDxtEncoder::EmitCTX1Indices( const byte* colorBlock, const byte* minColor, const byte* maxColor )
{
	ALIGN16( uint16 colors[4][2] );
	unsigned int result = 0;

	colors[0][0] = maxColor[0];
	colors[0][1] = maxColor[1];
	colors[1][0] = minColor[0];
	colors[1][1] = minColor[1];

	colors[2][0] = ( 2 * colors[0][0] + 1 * colors[1][0] ) / 3;
	colors[2][1] = ( 2 * colors[0][1] + 1 * colors[1][1] ) / 3;
	colors[3][0] = ( 1 * colors[0][0] + 2 * colors[1][0] ) / 3;
	colors[3][1] = ( 1 * colors[0][1] + 2 * colors[1][1] ) / 3;

	for( int i = 15; i >= 0; i-- )
	{
		int c0, c1, m, d0, d1, d2, d3;

		c0 = colorBlock[i * 4 + 0];
		c1 = colorBlock[i * 4 + 1];

		m = colors[0][0] - c0;
		d0 = abs( m );
		m = colors[1][0] - c0;
		d1 = abs( m );
		m = colors[2][0] - c0;
		d2 = abs( m );
		m = colors[3][0] - c0;
		d3 = abs( m );

		m = colors[0][1] - c1;
		d0 += abs( m );
		m = colors[1][1] - c1;
		d1 += abs( m );
		m = colors[2][1] - c1;
		d2 += abs( m );
		m = colors[3][1] - c1;
		d3 += abs( m );

		bool b0 = d0 > d3;
		bool b1 = d1 > d2;
		bool b2 = d0 > d2;
		bool b3 = d1 > d3;
		bool b4 = d2 > d3;

		int x0 = b1 & b2;
		int x1 = b0 & b3;
		int x2 = b0 & b4;

		result |= ( x2 | ( ( x0 | x1 ) << 1 ) ) << ( i << 1 );
	}

	EmitUInt( result );
}

/*
========================
idDxtEncoder::EmitAlphaIndices

params:	colorBlock	- 16 pixel block for which find alpha indexes
paramO:	minAlpha	- Min alpha found
paramO:	maxAlpha	- Max alpha found
========================
*/
void idDxtEncoder::EmitAlphaIndices( const byte* colorBlock, const int offset, const byte minAlpha, const byte maxAlpha )
{

	assert( maxAlpha >= minAlpha );

	const int ALPHA_RANGE = 7;

#if 1

	byte ab1, ab2, ab3, ab4, ab5, ab6, ab7;
	ALIGN16( byte indexes[16] );

	ab1 = ( 13 * maxAlpha +  1 * minAlpha + ALPHA_RANGE ) / ( ALPHA_RANGE * 2 );
	ab2 = ( 11 * maxAlpha +  3 * minAlpha + ALPHA_RANGE ) / ( ALPHA_RANGE * 2 );
	ab3 = ( 9 * maxAlpha +  5 * minAlpha + ALPHA_RANGE ) / ( ALPHA_RANGE * 2 );
	ab4 = ( 7 * maxAlpha +  7 * minAlpha + ALPHA_RANGE ) / ( ALPHA_RANGE * 2 );
	ab5 = ( 5 * maxAlpha +  9 * minAlpha + ALPHA_RANGE ) / ( ALPHA_RANGE * 2 );
	ab6 = ( 3 * maxAlpha + 11 * minAlpha + ALPHA_RANGE ) / ( ALPHA_RANGE * 2 );
	ab7 = ( 1 * maxAlpha + 13 * minAlpha + ALPHA_RANGE ) / ( ALPHA_RANGE * 2 );

	colorBlock += offset;

	for( int i = 0; i < 16; i++ )
	{
		byte a = colorBlock[i * 4];
		int b1 = ( a >= ab1 );
		int b2 = ( a >= ab2 );
		int b3 = ( a >= ab3 );
		int b4 = ( a >= ab4 );
		int b5 = ( a >= ab5 );
		int b6 = ( a >= ab6 );
		int b7 = ( a >= ab7 );
		int index = ( 8 - b1 - b2 - b3 - b4 - b5 - b6 - b7 ) & 7;
		indexes[i] = byte( index ^ ( 2 > index ) );
	}

	EmitByte( ( indexes[ 0] >> 0 ) | ( indexes[ 1] << 3 ) | ( indexes[ 2] << 6 ) );
	EmitByte( ( indexes[ 2] >> 2 ) | ( indexes[ 3] << 1 ) | ( indexes[ 4] << 4 ) | ( indexes[ 5] << 7 ) );
	EmitByte( ( indexes[ 5] >> 1 ) | ( indexes[ 6] << 2 ) | ( indexes[ 7] << 5 ) );

	EmitByte( ( indexes[ 8] >> 0 ) | ( indexes[ 9] << 3 ) | ( indexes[10] << 6 ) );
	EmitByte( ( indexes[10] >> 2 ) | ( indexes[11] << 1 ) | ( indexes[12] << 4 ) | ( indexes[13] << 7 ) );
	EmitByte( ( indexes[13] >> 1 ) | ( indexes[14] << 2 ) | ( indexes[15] << 5 ) );

#elif 0

	ALIGN16( byte indexes[16] );
	byte delta = maxAlpha - minAlpha;
	byte half = delta >> 1;
	byte bias = delta / ( 2 * ALPHA_RANGE );
	byte bottom = minAlpha + bias;
	byte top = maxAlpha - bias;

	colorBlock += offset;

	for( int i = 0; i < 16; i++ )
	{
		byte a = colorBlock[i * 4];
		if( a <= bottom )
		{
			indexes[i] = 1;
		}
		else if( a >= top )
		{
			indexes[i] = 0;
		}
		else
		{
			indexes[i] = ( ALPHA_RANGE + 1 ) + ( ( minAlpha - a ) * ALPHA_RANGE - half ) / delta;
		}
	}

	EmitByte( ( indexes[ 0] >> 0 ) | ( indexes[ 1] << 3 ) | ( indexes[ 2] << 6 ) );
	EmitByte( ( indexes[ 2] >> 2 ) | ( indexes[ 3] << 1 ) | ( indexes[ 4] << 4 ) | ( indexes[ 5] << 7 ) );
	EmitByte( ( indexes[ 5] >> 1 ) | ( indexes[ 6] << 2 ) | ( indexes[ 7] << 5 ) );

	EmitByte( ( indexes[ 8] >> 0 ) | ( indexes[ 9] << 3 ) | ( indexes[10] << 6 ) );
	EmitByte( ( indexes[10] >> 2 ) | ( indexes[11] << 1 ) | ( indexes[12] << 4 ) | ( indexes[13] << 7 ) );
	EmitByte( ( indexes[13] >> 1 ) | ( indexes[14] << 2 ) | ( indexes[15] << 5 ) );

#elif 0

	ALIGN16( byte indexes[16] );
	byte delta = maxAlpha - minAlpha;
	byte half = delta >> 1;
	byte bias = delta / ( 2 * ALPHA_RANGE );
	byte bottom = minAlpha + bias;
	byte top = maxAlpha - bias;

	colorBlock += offset;

	for( int i = 0; i < 16; i++ )
	{
		byte a = colorBlock[i * 4];
		int index = ( ALPHA_RANGE + 1 ) + ( ( minAlpha - a ) * ALPHA_RANGE - half ) / delta;
		int c0 = a > bottom;
		int c1 = a < top;
		indexes[i] = ( index & -( c0 & c1 ) ) | ( c0 ^ 1 );
	}

	EmitByte( ( indexes[ 0] >> 0 ) | ( indexes[ 1] << 3 ) | ( indexes[ 2] << 6 ) );
	EmitByte( ( indexes[ 2] >> 2 ) | ( indexes[ 3] << 1 ) | ( indexes[ 4] << 4 ) | ( indexes[ 5] << 7 ) );
	EmitByte( ( indexes[ 5] >> 1 ) | ( indexes[ 6] << 2 ) | ( indexes[ 7] << 5 ) );

	EmitByte( ( indexes[ 8] >> 0 ) | ( indexes[ 9] << 3 ) | ( indexes[10] << 6 ) );
	EmitByte( ( indexes[10] >> 2 ) | ( indexes[11] << 1 ) | ( indexes[12] << 4 ) | ( indexes[13] << 7 ) );
	EmitByte( ( indexes[13] >> 1 ) | ( indexes[14] << 2 ) | ( indexes[15] << 5 ) );

#else

	ALIGN16( byte indexes[16] );
	ALIGN16( byte alphas[8] );

	alphas[0] = maxAlpha;
	alphas[1] = minAlpha;
	alphas[2] = ( 6 * maxAlpha + 1 * minAlpha ) / ALPHA_RANGE;
	alphas[3] = ( 5 * maxAlpha + 2 * minAlpha ) / ALPHA_RANGE;
	alphas[4] = ( 4 * maxAlpha + 3 * minAlpha ) / ALPHA_RANGE;
	alphas[5] = ( 3 * maxAlpha + 4 * minAlpha ) / ALPHA_RANGE;
	alphas[6] = ( 2 * maxAlpha + 5 * minAlpha ) / ALPHA_RANGE;
	alphas[7] = ( 1 * maxAlpha + 6 * minAlpha ) / ALPHA_RANGE;

	colorBlock += offset;

	for( int i = 0; i < 16; i++ )
	{
		int minDist = INT_MAX;
		byte a = colorBlock[i * 4];
		for( int j = 0; j < 8; j++ )
		{
			int dist = abs( a - alphas[j] );
			if( dist < minDist )
			{
				minDist = dist;
				indexes[i] = j;
			}
		}
	}

	EmitByte( ( indexes[ 0] >> 0 ) | ( indexes[ 1] << 3 ) | ( indexes[ 2] << 6 ) );
	EmitByte( ( indexes[ 2] >> 2 ) | ( indexes[ 3] << 1 ) | ( indexes[ 4] << 4 ) | ( indexes[ 5] << 7 ) );
	EmitByte( ( indexes[ 5] >> 1 ) | ( indexes[ 6] << 2 ) | ( indexes[ 7] << 5 ) );

	EmitByte( ( indexes[ 8] >> 0 ) | ( indexes[ 9] << 3 ) | ( indexes[10] << 6 ) );
	EmitByte( ( indexes[10] >> 2 ) | ( indexes[11] << 1 ) | ( indexes[12] << 4 ) | ( indexes[13] << 7 ) );
	EmitByte( ( indexes[13] >> 1 ) | ( indexes[14] << 2 ) | ( indexes[15] << 5 ) );

#endif
}

/*
========================
idDxtEncoder::CompressImageDXT1Fast_Generic

params:	inBuf		- image to compress
paramO:	outBuf		- result of compression
params:	width		- width of image
params:	height		- height of image
========================
*/
void idDxtEncoder::CompressImageDXT1Fast_Generic( const byte* inBuf, byte* outBuf, int width, int height )
{
	ALIGN16( byte block[64] );
	ALIGN16( byte minColor[4] );
	ALIGN16( byte maxColor[4] );

	assert( width >= 4 && ( width & 3 ) == 0 );
	assert( height >= 4 && ( height & 3 ) == 0 );

	this->width = width;
	this->height = height;
	this->outData = outBuf;

	for( int j = 0; j < height; j += 4, inBuf += width * 4 * 4 )
	{
		for( int i = 0; i < width; i += 4 )
		{
			commonLocal.LoadPacifierBinarizeProgressIncrement( 16 );

			ExtractBlock( inBuf + i * 4, width, block );

			GetMinMaxBBox( block, minColor, maxColor );
			//SelectColorsDiagonal( block, minColor, maxColor );
			InsetColorsBBox( minColor, maxColor );

			EmitUShort( ColorTo565( maxColor ) );
			EmitUShort( ColorTo565( minColor ) );

			EmitColorIndices( block, minColor, maxColor );
		}
		outData += dstPadding;
		inBuf += srcPadding;
	}
}

/*
========================
idDxtEncoder::CompressImageDXT1AlphaFast_Generic

params:	inBuf		- image to compress
paramO:	outBuf		- result of compression
params:	width		- width of image
params:	height		- height of image
========================
*/
void idDxtEncoder::CompressImageDXT1AlphaFast_Generic( const byte* inBuf, byte* outBuf, int width, int height )
{
	ALIGN16( byte block[64] );
	ALIGN16( byte minColor[4] );
	ALIGN16( byte maxColor[4] );

	assert( width >= 4 && ( width & 3 ) == 0 );
	assert( height >= 4 && ( height & 3 ) == 0 );

	this->width = width;
	this->height = height;
	this->outData = outBuf;

	for( int j = 0; j < height; j += 4, inBuf += width * 4 * 4 )
	{
		for( int i = 0; i < width; i += 4 )
		{
			commonLocal.LoadPacifierBinarizeProgressIncrement( 16 );

			ExtractBlock( inBuf + i * 4, width, block );

			GetMinMaxBBox( block, minColor, maxColor );
			byte minAlpha = minColor[3];
			//SelectColorsDiagonal( block, minColor, maxColor );
			InsetColorsBBox( minColor, maxColor );

			if( minAlpha >= 128 )
			{
				EmitUShort( ColorTo565( maxColor ) );
				EmitUShort( ColorTo565( minColor ) );
				EmitColorIndices( block, minColor, maxColor );
			}
			else
			{
				EmitUShort( ColorTo565( minColor ) );
				EmitUShort( ColorTo565( maxColor ) );
				EmitColorAlphaIndices( block, minColor, maxColor );
			}
		}
		outData += dstPadding;
		inBuf += srcPadding;
	}
}

/*
========================
idDxtEncoder::CompressImageDXT5Fast_Generic

params:	inBuf		- image to compress
paramO:	outBuf		- result of compression
params:	width		- width of image
params:	height		- height of image
========================
*/
void idDxtEncoder::CompressImageDXT5Fast_Generic( const byte* inBuf, byte* outBuf, int width, int height )
{
	ALIGN16( byte block[64] );
	ALIGN16( byte minColor[4] );
	ALIGN16( byte maxColor[4] );

	assert( width >= 4 && ( width & 3 ) == 0 );
	assert( height >= 4 && ( height & 3 ) == 0 );

	this->width = width;
	this->height = height;
	this->outData = outBuf;

	for( int j = 0; j < height; j += 4, inBuf += width * 4 * 4 )
	{
		for( int i = 0; i < width; i += 4 )
		{
			commonLocal.LoadPacifierBinarizeProgressIncrement( 16 );

			ExtractBlock( inBuf + i * 4, width, block );

			GetMinMaxBBox( block, minColor, maxColor );
			//SelectColorsDiagonal( block, minColor, maxColor );
			InsetColorsBBox( minColor, maxColor );

			EmitByte( maxColor[3] );
			EmitByte( minColor[3] );

			EmitAlphaIndices( block, 3, minColor[3], maxColor[3] );

#ifdef NVIDIA_7X_HARDWARE_BUG_FIX
			// the colors are already always guaranteed to be sorted properly
#endif

			EmitUShort( ColorTo565( maxColor ) );
			EmitUShort( ColorTo565( minColor ) );

			EmitColorIndices( block, minColor, maxColor );
		}
		outData += dstPadding;
		inBuf += srcPadding;
	}
}

/*
========================
idDxtEncoder::ScaleYCoCg
========================
*/
void idDxtEncoder::ScaleYCoCg( byte* colorBlock, byte* minColor, byte* maxColor ) const
{
	int m0 = abs( minColor[0] - 128 );
	int m1 = abs( minColor[1] - 128 );
	int m2 = abs( maxColor[0] - 128 );
	int m3 = abs( maxColor[1] - 128 );

	if( m1 > m0 )
	{
		m0 = m1;
	}
	if( m3 > m2 )
	{
		m2 = m3;
	}
	if( m2 > m0 )
	{
		m0 = m2;
	}

	const int s0 = 128 / 2 - 1;
	const int s1 = 128 / 4 - 1;

	int mask0 = -( m0 <= s0 );
	int mask1 = -( m0 <= s1 );
	int scale = 1 + ( 1 & mask0 ) + ( 2 & mask1 );

	minColor[0] = byte( ( minColor[0] - 128 ) * scale + 128 );
	minColor[1] = byte( ( minColor[1] - 128 ) * scale + 128 );
	minColor[2] = byte( ( scale - 1 ) << 3 );
	maxColor[0] = byte( ( maxColor[0] - 128 ) * scale + 128 );
	maxColor[1] = byte( ( maxColor[1] - 128 ) * scale + 128 );
	maxColor[2] = byte( ( scale - 1 ) << 3 );

	for( int i = 0; i < 16; i++ )
	{
		colorBlock[i * 4 + 0] = byte( ( colorBlock[i * 4 + 0] - 128 ) * scale + 128 );
		colorBlock[i * 4 + 1] = byte( ( colorBlock[i * 4 + 1] - 128 ) * scale + 128 );
	}
}

/*
========================
idDxtEncoder::InsetYCoCgBBox
========================
*/
ID_INLINE void idDxtEncoder::InsetYCoCgBBox( byte* minColor, byte* maxColor ) const
{

#if 0

	byte inset[4];

	inset[0] = ( maxColor[0] - minColor[0] ) >> INSET_COLOR_SHIFT;
	inset[1] = ( maxColor[1] - minColor[1] ) >> INSET_COLOR_SHIFT;
	inset[3] = ( maxColor[3] - minColor[3] ) >> INSET_ALPHA_SHIFT;

	minColor[0] = ( minColor[0] + inset[0] <= 255 ) ? minColor[0] + inset[0] : 255;
	minColor[1] = ( minColor[1] + inset[1] <= 255 ) ? minColor[1] + inset[1] : 255;
	minColor[3] = ( minColor[3] + inset[3] <= 255 ) ? minColor[3] + inset[3] : 255;

	maxColor[0] = ( maxColor[0] >= inset[0] ) ? maxColor[0] - inset[0] : 0;
	maxColor[1] = ( maxColor[1] >= inset[1] ) ? maxColor[1] - inset[1] : 0;
	maxColor[3] = ( maxColor[3] >= inset[3] ) ? maxColor[3] - inset[3] : 0;

	minColor[0] = ( minColor[0] & C565_5_MASK ) | ( minColor[0] >> 5 );
	minColor[1] = ( minColor[1] & C565_6_MASK ) | ( minColor[1] >> 6 );

	maxColor[0] = ( maxColor[0] & C565_5_MASK ) | ( maxColor[0] >> 5 );
	maxColor[1] = ( maxColor[1] & C565_6_MASK ) | ( maxColor[1] >> 6 );

#elif 0

	float inset[4];
	float minf[4];
	float maxf[4];

	for( int i = 0; i < 4; i++ )
	{
		minf[i] = minColor[i] / 255.0f;
		maxf[i] = maxColor[i] / 255.0f;
	}

	inset[0] = ( maxf[0] - minf[0] ) / 16.0f;
	inset[1] = ( maxf[1] - minf[1] ) / 16.0f;
	inset[2] = ( maxf[2] - minf[2] ) / 16.0f;
	inset[3] = ( maxf[3] - minf[3] ) / 32.0f;

	for( int i = 0; i < 4; i++ )
	{
		minf[i] = ( minf[i] + inset[i] <= 1.0f ) ? minf[i] + inset[i] : 1.0f;
		maxf[i] = ( maxf[i] >= inset[i] ) ? maxf[i] - inset[i] : 0;
	}

	minColor[0] = ( ( int )floor( minf[0] * 31 ) ) & ( ( 1 << 5 ) - 1 );
	minColor[1] = ( ( int )floor( minf[1] * 63 ) ) & ( ( 1 << 6 ) - 1 );

	maxColor[0] = ( ( int )ceil( maxf[0] * 31 ) ) & ( ( 1 << 5 ) - 1 );
	maxColor[1] = ( ( int )ceil( maxf[1] * 63 ) ) & ( ( 1 << 6 ) - 1 );

	minColor[0] = ( minColor[0] << 3 ) | ( minColor[0] >> 2 );
	minColor[1] = ( minColor[1] << 2 ) | ( minColor[1] >> 4 );

	maxColor[0] = ( maxColor[0] << 3 ) | ( maxColor[0] >> 2 );
	maxColor[1] = ( maxColor[1] << 2 ) | ( maxColor[1] >> 4 );

	minColor[3] = ( int )floor( minf[3] * 255.0f );
	maxColor[3] = ( int )ceil( maxf[3] * 255.0f );

#elif 0

	int inset[4];
	int mini[4];
	int maxi[4];

	inset[0] = ( maxColor[0] - minColor[0] );
	inset[1] = ( maxColor[1] - minColor[1] );
	inset[3] = ( maxColor[3] - minColor[3] );

	mini[0] = ( minColor[0] << INSET_COLOR_SHIFT ) + inset[0];
	mini[1] = ( minColor[1] << INSET_COLOR_SHIFT ) + inset[1];
	mini[3] = ( minColor[3] << INSET_ALPHA_SHIFT ) + inset[3];

	maxi[0] = ( maxColor[0] << INSET_COLOR_SHIFT ) - inset[0];
	maxi[1] = ( maxColor[1] << INSET_COLOR_SHIFT ) - inset[1];
	maxi[3] = ( maxColor[3] << INSET_ALPHA_SHIFT ) - inset[3];

	mini[0] = ( mini[0] - ( ( 1 << ( 3 ) ) - 1 ) ) >> ( INSET_COLOR_SHIFT + 3 );
	mini[1] = ( mini[1] - ( ( 1 << ( 3 ) ) - 1 ) ) >> ( INSET_COLOR_SHIFT + 2 );
	mini[3] = ( mini[3] - ( ( 1 << ( 2 ) ) - 1 ) ) >> ( INSET_ALPHA_SHIFT + 0 );

	maxi[0] = ( maxi[0] + ( ( 1 << ( 3 ) ) - 1 ) ) >> ( INSET_COLOR_SHIFT + 3 );
	maxi[1] = ( maxi[1] + ( ( 1 << ( 3 ) ) - 1 ) ) >> ( INSET_COLOR_SHIFT + 2 );
	maxi[3] = ( maxi[3] + ( ( 1 << ( 2 ) ) - 1 ) ) >> ( INSET_ALPHA_SHIFT + 0 );

	if( mini[0] < 0 )
	{
		mini[0] = 0;
	}
	if( mini[1] < 0 )
	{
		mini[1] = 0;
	}
	if( mini[3] < 0 )
	{
		mini[3] = 0;
	}

	if( maxi[0] > 31 )
	{
		maxi[0] = 31;
	}
	if( maxi[1] > 63 )
	{
		maxi[1] = 63;
	}
	if( maxi[3] > 255 )
	{
		maxi[3] = 255;
	}

	minColor[0] = ( mini[0] << 3 ) | ( mini[0] >> 2 );
	minColor[1] = ( mini[1] << 2 ) | ( mini[1] >> 4 );
	minColor[3] = mini[3];

	maxColor[0] = ( maxi[0] << 3 ) | ( maxi[0] >> 2 );
	maxColor[1] = ( maxi[1] << 2 ) | ( maxi[1] >> 4 );
	maxColor[3] = maxi[3];

#elif 1

	int inset[4];
	int mini[4];
	int maxi[4];

	inset[0] = ( maxColor[0] - minColor[0] ) - ( ( 1 << ( INSET_COLOR_SHIFT - 1 ) ) - 1 );
	inset[1] = ( maxColor[1] - minColor[1] ) - ( ( 1 << ( INSET_COLOR_SHIFT - 1 ) ) - 1 );
	inset[3] = ( maxColor[3] - minColor[3] ) - ( ( 1 << ( INSET_ALPHA_SHIFT - 1 ) ) - 1 );

	mini[0] = ( ( minColor[0] << INSET_COLOR_SHIFT ) + inset[0] ) >> INSET_COLOR_SHIFT;
	mini[1] = ( ( minColor[1] << INSET_COLOR_SHIFT ) + inset[1] ) >> INSET_COLOR_SHIFT;
	mini[3] = ( ( minColor[3] << INSET_ALPHA_SHIFT ) + inset[3] ) >> INSET_ALPHA_SHIFT;

	maxi[0] = ( ( maxColor[0] << INSET_COLOR_SHIFT ) - inset[0] ) >> INSET_COLOR_SHIFT;
	maxi[1] = ( ( maxColor[1] << INSET_COLOR_SHIFT ) - inset[1] ) >> INSET_COLOR_SHIFT;
	maxi[3] = ( ( maxColor[3] << INSET_ALPHA_SHIFT ) - inset[3] ) >> INSET_ALPHA_SHIFT;

	mini[0] = ( mini[0] >= 0 ) ? mini[0] : 0;
	mini[1] = ( mini[1] >= 0 ) ? mini[1] : 0;
	mini[3] = ( mini[3] >= 0 ) ? mini[3] : 0;

	maxi[0] = ( maxi[0] <= 255 ) ? maxi[0] : 255;
	maxi[1] = ( maxi[1] <= 255 ) ? maxi[1] : 255;
	maxi[3] = ( maxi[3] <= 255 ) ? maxi[3] : 255;

	minColor[0] = byte( ( mini[0] & C565_5_MASK ) | ( mini[0] >> 5 ) );
	minColor[1] = byte( ( mini[1] & C565_6_MASK ) | ( mini[1] >> 6 ) );
	minColor[3] = byte( mini[3] );

	maxColor[0] = byte( ( maxi[0] & C565_5_MASK ) | ( maxi[0] >> 5 ) );
	maxColor[1] = byte( ( maxi[1] & C565_6_MASK ) | ( maxi[1] >> 6 ) );
	maxColor[3] = byte( maxi[3] );

#endif
}

/*
========================
idDxtEncoder::InsetYCoCgAlpaBBox
========================
*/
ID_INLINE void idDxtEncoder::InsetYCoCgAlpaBBox( byte* minColor, byte* maxColor ) const
{
	int inset[4];
	int mini[4];
	int maxi[4];

	inset[0] = ( maxColor[0] - minColor[0] ) - ( ( 1 << ( INSET_COLOR_SHIFT - 1 ) ) - 1 );
	inset[1] = ( maxColor[1] - minColor[1] ) - ( ( 1 << ( INSET_COLOR_SHIFT - 1 ) ) - 1 );
	inset[2] = ( maxColor[2] - minColor[2] ) - ( ( 1 << ( INSET_COLOR_SHIFT - 1 ) ) - 1 );
	inset[3] = ( maxColor[3] - minColor[3] ) - ( ( 1 << ( INSET_ALPHA_SHIFT - 1 ) ) - 1 );

	mini[0] = ( ( minColor[0] << INSET_COLOR_SHIFT ) + inset[0] ) >> INSET_COLOR_SHIFT;
	mini[1] = ( ( minColor[1] << INSET_COLOR_SHIFT ) + inset[1] ) >> INSET_COLOR_SHIFT;
	mini[2] = ( ( minColor[2] << INSET_COLOR_SHIFT ) + inset[2] ) >> INSET_COLOR_SHIFT;
	mini[3] = ( ( minColor[3] << INSET_ALPHA_SHIFT ) + inset[3] ) >> INSET_ALPHA_SHIFT;

	maxi[0] = ( ( maxColor[0] << INSET_COLOR_SHIFT ) - inset[0] ) >> INSET_COLOR_SHIFT;
	maxi[1] = ( ( maxColor[1] << INSET_COLOR_SHIFT ) - inset[1] ) >> INSET_COLOR_SHIFT;
	maxi[2] = ( ( maxColor[2] << INSET_COLOR_SHIFT ) - inset[2] ) >> INSET_COLOR_SHIFT;
	maxi[3] = ( ( maxColor[3] << INSET_ALPHA_SHIFT ) - inset[3] ) >> INSET_ALPHA_SHIFT;

	mini[0] = ( mini[0] >= 0 ) ? mini[0] : 0;
	mini[1] = ( mini[1] >= 0 ) ? mini[1] : 0;
	mini[2] = ( mini[2] >= 0 ) ? mini[2] : 0;
	mini[3] = ( mini[3] >= 0 ) ? mini[3] : 0;

	maxi[0] = ( maxi[0] <= 255 ) ? maxi[0] : 255;
	maxi[1] = ( maxi[1] <= 255 ) ? maxi[1] : 255;
	maxi[2] = ( maxi[2] <= 255 ) ? maxi[2] : 255;
	maxi[3] = ( maxi[3] <= 255 ) ? maxi[3] : 255;

	minColor[0] = byte( ( mini[0] & C565_5_MASK ) | ( mini[0] >> 5 ) );
	minColor[1] = byte( ( mini[1] & C565_6_MASK ) | ( mini[1] >> 6 ) );
	minColor[2] = byte( ( mini[2] & C565_5_MASK ) | ( mini[2] >> 5 ) );
	minColor[3] = byte( mini[3] );

	maxColor[0] = byte( ( maxi[0] & C565_5_MASK ) | ( maxi[0] >> 5 ) );
	maxColor[1] = byte( ( maxi[1] & C565_6_MASK ) | ( maxi[1] >> 6 ) );
	maxColor[2] = byte( ( maxi[2] & C565_5_MASK ) | ( maxi[2] >> 5 ) );
	maxColor[3] = byte( maxi[3] );
}

/*
========================
idDxtEncoder::SelectYCoCgDiagonal
========================
*/
void idDxtEncoder::SelectYCoCgDiagonal( const byte* colorBlock, byte* minColor, byte* maxColor ) const
{
	byte side = 0;

	byte mid0 = byte( ( ( int ) minColor[0] + maxColor[0] + 1 ) >> 1 );
	byte mid1 = byte( ( ( int ) minColor[1] + maxColor[1] + 1 ) >> 1 );

	for( int i = 0; i < 16; i++ )
	{
		byte b0 = colorBlock[i * 4 + 0] >= mid0;
		byte b1 = colorBlock[i * 4 + 1] >= mid1;
		side += ( b0 ^ b1 );
	}

	byte mask = -( side > 8 );

#if defined NVIDIA_7X_HARDWARE_BUG_FIX
	mask &= -( minColor[0] != maxColor[0] );
#endif

	byte c0 = minColor[1];
	byte c1 = maxColor[1];

	c0 ^= c1;
	mask &= c0;
	c1 ^= mask;
	c0 ^= c1;

	minColor[1] = c0;
	maxColor[1] = c1;
}

/*
========================
idDxtEncoder::CompressYCoCgDXT5Fast_Generic

params:	inBuf		- image to compress
paramO:	outBuf		- result of compression
params:	width		- width of image
params:	height		- height of image
========================
*/
void idDxtEncoder::CompressYCoCgDXT5Fast_Generic( const byte* inBuf, byte* outBuf, int width, int height )
{
	ALIGN16( byte block[64] );
	ALIGN16( byte minColor[4] );
	ALIGN16( byte maxColor[4] );

	//assert( HasConstantValuePer4x4Block( inBuf, width, height, 2 ) );

	assert( width >= 4 && ( width & 3 ) == 0 );
	assert( height >= 4 && ( height & 3 ) == 0 );

	this->width = width;
	this->height = height;
	this->outData = outBuf;

	for( int j = 0; j < height; j += 4, inBuf += width * 4 * 4 )
	{
		for( int i = 0; i < width; i += 4 )
		{
			commonLocal.LoadPacifierBinarizeProgressIncrement( 16 );

			ExtractBlock( inBuf + i * 4, width, block );

			GetMinMaxBBox( block, minColor, maxColor );
			ScaleYCoCg( block, minColor, maxColor );
			InsetYCoCgBBox( minColor, maxColor );
			SelectYCoCgDiagonal( block, minColor, maxColor );

			EmitByte( maxColor[3] );
			EmitByte( minColor[3] );

			EmitAlphaIndices( block, 3, minColor[3], maxColor[3] );

#ifdef NVIDIA_7X_HARDWARE_BUG_FIX
			// the colors are already sorted when selecting the diagonal
#endif

			EmitUShort( ColorTo565( maxColor ) );
			EmitUShort( ColorTo565( minColor ) );

			EmitColorIndices( block, minColor, maxColor );
		}
		outData += dstPadding;
		inBuf += srcPadding;
	}
}

/*
========================
idDxtEncoder::CompressYCoCgAlphaDXT5Fast

params:	inBuf		- image to compress
paramO:	outBuf		- result of compression
params:	width		- width of image
params:	height		- height of image
========================
*/
void idDxtEncoder::CompressYCoCgAlphaDXT5Fast( const byte* inBuf, byte* outBuf, int width, int height )
{
	ALIGN16( byte block[64] );
	ALIGN16( byte minColor[4] );
	ALIGN16( byte maxColor[4] );

	assert( width >= 4 && ( width & 3 ) == 0 );
	assert( height >= 4 && ( height & 3 ) == 0 );

	this->width = width;
	this->height = height;
	this->outData = outBuf;

	for( int j = 0; j < height; j += 4, inBuf += width * 4 * 4 )
	{
		for( int i = 0; i < width; i += 4 )
		{
			commonLocal.LoadPacifierBinarizeProgressIncrement( 16 );

			ExtractBlock( inBuf + i * 4, width, block );

			// scale down the chroma of texels that are close to gray with low luminance
			for( int k = 0; k < 16; k++ )
			{
				if( abs( block[k * 4 + 0] - 132 ) <= 8 &&
						abs( block[k * 4 + 2] - 132 ) <= 8 &&
						block[k * 4 + 3] < 96 )
				{
					block[k * 4 + 0] = ( block[k * 4 + 0] - 132 ) / 2 + 132;
					block[k * 4 + 2] = ( block[k * 4 + 2] - 132 ) / 2 + 132;
				}
			}

			GetMinMaxBBox( block, minColor, maxColor );
			InsetYCoCgAlpaBBox( minColor, maxColor );
			SelectColorsDiagonal( block, minColor, maxColor );

			EmitByte( maxColor[3] );
			EmitByte( minColor[3] );

			EmitAlphaIndices( block, 3, minColor[3], maxColor[3] );

#ifdef NVIDIA_7X_HARDWARE_BUG_FIX
			// the colors are already sorted when selecting the diagonal
#endif

			EmitUShort( ColorTo565( maxColor ) );
			EmitUShort( ColorTo565( minColor ) );

			EmitColorIndices( block, minColor, maxColor );
		}
		outData += dstPadding;
		inBuf += srcPadding;
	}
}

/*
========================
idDxtEncoder::CompressYCoCgCTX1DXT5AFast_Generic

params:	inBuf		- image to compress
paramO:	outBuf		- result of compression
params:	width		- width of image
params:	height		- height of image
========================
*/
void idDxtEncoder::CompressYCoCgCTX1DXT5AFast_Generic( const byte* inBuf, byte* outBuf, int width, int height )
{
	ALIGN16( byte block[64] );
	ALIGN16( byte minColor[4] );
	ALIGN16( byte maxColor[4] );

	assert( HasConstantValuePer4x4Block( inBuf, width, height, 2 ) );

	assert( width >= 4 && ( width & 3 ) == 0 );
	assert( height >= 4 && ( height & 3 ) == 0 );

	this->width = width;
	this->height = height;
	this->outData = outBuf;

	for( int j = 0; j < height; j += 4, inBuf += width * 4 * 4 )
	{
		for( int i = 0; i < width; i += 4 )
		{
			commonLocal.LoadPacifierBinarizeProgressIncrement( 16 );

			ExtractBlock( inBuf + i * 4, width, block );

			GetMinMaxBBox( block, minColor, maxColor );
			SelectYCoCgDiagonal( block, minColor, maxColor );
			InsetColorsBBox( minColor, maxColor );

			EmitByte( maxColor[3] );
			EmitByte( minColor[3] );

			EmitAlphaIndices( block, 3, minColor[3], maxColor[3] );

			EmitByte( maxColor[0] );
			EmitByte( maxColor[1] );
			EmitByte( minColor[0] );
			EmitByte( minColor[1] );

			EmitCTX1Indices( block, minColor, maxColor );
		}
		outData += dstPadding;
		inBuf += srcPadding;
	}
}

/*
========================
idDxtEncoder::EmitGreenIndices

params:	block		- block for which to find green indices
paramO:	minGreen	- Min green found
paramO:	maxGreen	- Max green found
========================
*/
void idDxtEncoder::EmitGreenIndices( const byte* block, const int offset, const byte minGreen, const byte maxGreen )
{

	assert( maxGreen >= minGreen );

	const int COLOR_RANGE = 3;

#if 1

	byte yb1 = ( 5 * maxGreen + 1 * minGreen + COLOR_RANGE ) / ( 2 * COLOR_RANGE );
	byte yb2 = ( 3 * maxGreen + 3 * minGreen + COLOR_RANGE ) / ( 2 * COLOR_RANGE );
	byte yb3 = ( 1 * maxGreen + 5 * minGreen + COLOR_RANGE ) / ( 2 * COLOR_RANGE );

	unsigned int result = 0;

	block += offset;

	for( int i = 15; i >= 0; i-- )
	{
		result <<= 2;
		byte y = block[i * 4];
		int b1 = ( y >= yb1 );
		int b2 = ( y >= yb2 );
		int b3 = ( y >= yb3 );
		int index = ( 4 - b1 - b2 - b3 ) & 3;
		index ^= ( 2 > index );
		result |= index;
	}

	EmitUInt( result );

#else

	byte green[4];

	green[0] = maxGreen;
	green[1] = minGreen;
	green[2] = ( 2 * green[0] + 1 * green[1] ) / 3;
	green[3] = ( 1 * green[0] + 2 * green[1] ) / 3;

	unsigned int result = 0;

	block += offset;

	for( int i = 15; i >= 0; i-- )
	{
		result <<= 2;
		byte y = block[i * 4];
		int minDist = INT_MAX;
		int index;
		for( int j = 0; j < 4; j++ )
		{
			int dist = abs( y - green[j] );
			if( dist < minDist )
			{
				minDist = dist;
				index = j;
			}
		}
		result |= index;
	}

	EmitUInt( result );

#endif
}

/*
========================
idDxtEncoder::InsetNormalsBBoxDXT5
========================
*/
void idDxtEncoder::InsetNormalsBBoxDXT5( byte* minNormal, byte* maxNormal ) const
{
	int inset[4];
	int mini[4];
	int maxi[4];

	inset[3] = ( maxNormal[3] - minNormal[3] ) - ( ( 1 << ( INSET_ALPHA_SHIFT - 1 ) ) - 1 );
	inset[1] = ( maxNormal[1] - minNormal[1] ) - ( ( 1 << ( INSET_COLOR_SHIFT - 1 ) ) - 1 );

	mini[3] = ( ( minNormal[3] << INSET_ALPHA_SHIFT ) + inset[3] ) >> INSET_ALPHA_SHIFT;
	mini[1] = ( ( minNormal[1] << INSET_COLOR_SHIFT ) + inset[1] ) >> INSET_COLOR_SHIFT;

	maxi[3] = ( ( maxNormal[3] << INSET_ALPHA_SHIFT ) - inset[3] ) >> INSET_ALPHA_SHIFT;
	maxi[1] = ( ( maxNormal[1] << INSET_COLOR_SHIFT ) - inset[1] ) >> INSET_COLOR_SHIFT;

	mini[3] = ( mini[3] >= 0 ) ? mini[3] : 0;
	mini[1] = ( mini[1] >= 0 ) ? mini[1] : 0;

	maxi[3] = ( maxi[3] <= 255 ) ? maxi[3] : 255;
	maxi[1] = ( maxi[1] <= 255 ) ? maxi[1] : 255;

	minNormal[3] = byte( mini[3] );
	minNormal[1] = byte( ( mini[1] & C565_6_MASK ) | ( mini[1] >> 6 ) );

	maxNormal[3] = byte( maxi[3] );
	maxNormal[1] = byte( ( maxi[1] & C565_6_MASK ) | ( maxi[1] >> 6 ) );
}

/*
========================
idDxtEncoder::InsetNormalsBBox3Dc
========================
*/
void idDxtEncoder::InsetNormalsBBox3Dc( byte* minNormal, byte* maxNormal ) const
{
	int inset[4];
	int mini[4];
	int maxi[4];

	inset[0] = ( maxNormal[0] - minNormal[0] ) - ( ( 1 << ( INSET_ALPHA_SHIFT - 1 ) ) - 1 );
	inset[1] = ( maxNormal[1] - minNormal[1] ) - ( ( 1 << ( INSET_ALPHA_SHIFT - 1 ) ) - 1 );

	mini[0] = ( ( minNormal[0] << INSET_ALPHA_SHIFT ) + inset[0] ) >> INSET_ALPHA_SHIFT;
	mini[1] = ( ( minNormal[1] << INSET_ALPHA_SHIFT ) + inset[1] ) >> INSET_ALPHA_SHIFT;

	maxi[0] = ( ( maxNormal[0] << INSET_ALPHA_SHIFT ) - inset[0] ) >> INSET_ALPHA_SHIFT;
	maxi[1] = ( ( maxNormal[1] << INSET_ALPHA_SHIFT ) - inset[1] ) >> INSET_ALPHA_SHIFT;

	mini[0] = ( mini[0] >= 0 ) ? mini[0] : 0;
	mini[1] = ( mini[1] >= 0 ) ? mini[1] : 0;

	maxi[0] = ( maxi[0] <= 255 ) ? maxi[0] : 255;
	maxi[1] = ( maxi[1] <= 255 ) ? maxi[1] : 255;

	minNormal[0] = ( byte )mini[0];
	minNormal[1] = ( byte )mini[1];

	maxNormal[0] = ( byte )maxi[0];
	maxNormal[1] = ( byte )maxi[1];
}

/*
========================
idDxtEncoder::CompressNormalMapDXT5Fast_Generic

params:	inBuf		- image to compress
paramO:	outBuf		- result of compression
params:	width		- width of image
params:	height		- height of image
========================
*/
void idDxtEncoder::CompressNormalMapDXT5Fast_Generic( const byte* inBuf, byte* outBuf, int width, int height )
{
	ALIGN16( byte block[64] );
	ALIGN16( byte normal1[4] );
	ALIGN16( byte normal2[4] );

	assert( width >= 4 && ( width & 3 ) == 0 );
	assert( height >= 4 && ( height & 3 ) == 0 );

	this->width = width;
	this->height = height;
	this->outData = outBuf;

	for( int j = 0; j < height; j += 4, inBuf += width * 4 * 4 )
	{
		for( int i = 0; i < width; i += 4 )
		{
			commonLocal.LoadPacifierBinarizeProgressIncrement( 16 );

			ExtractBlock( inBuf + i * 4, width, block );

			GetMinMaxBBox( block, normal1, normal2 );
			InsetNormalsBBoxDXT5( normal1, normal2 );

			// Write out Nx into alpha channel.
			EmitByte( normal2[3] );
			EmitByte( normal1[3] );
			EmitAlphaIndices( block, 3, normal1[3], normal2[3] );

			// Write out Ny into green channel.
			EmitUShort( ColorTo565( block[0], normal2[1], block[2] ) );
			EmitUShort( ColorTo565( block[0], normal1[1], block[2] ) );
			EmitGreenIndices( block, 1, normal1[1], normal2[1] );
		}
		outData += dstPadding;
		inBuf += srcPadding;
	}
}

/*
========================
idDxtEncoder::CompressImageDXN1Fast_Generic

params:	inBuf		- image to compress
paramO:	outBuf		- result of compression
params:	width		- width of image
params:	height		- height of image
========================
*/
void idDxtEncoder::CompressImageDXN1Fast_Generic( const byte* inBuf, byte* outBuf, int width, int height )
{
	ALIGN16( byte block[64] );
	ALIGN16( byte min[4] );
	ALIGN16( byte max[4] );

	assert( width >= 4 && ( width & 3 ) == 0 );
	assert( height >= 4 && ( height & 3 ) == 0 );

	this->width = width;
	this->height = height;
	this->outData = outBuf;

	for( int j = 0; j < height; j += 4, inBuf += width * 4 * 4 )
	{
		for( int i = 0; i < width; i += 4 )
		{
			commonLocal.LoadPacifierBinarizeProgressIncrement( 16 );

			ExtractBlock( inBuf + i * 4, width, block );

			GetMinMaxBBox( block, min, max );
			InsetNormalsBBox3Dc( min, max );

			// Write out an alpha channel.
			EmitByte( max[0] );
			EmitByte( min[0] );
			EmitAlphaIndices( block, 0, min[0], max[0] );
		}
		outData += dstPadding;
		inBuf += srcPadding;
	}
}

/*
========================
idDxtEncoder::CompressNormalMapDXN2Fast_Generic

params:	inBuf		- image to compress
paramO:	outBuf		- result of compression
params:	width		- width of image
params:	height		- height of image
========================
*/
void idDxtEncoder::CompressNormalMapDXN2Fast_Generic( const byte* inBuf, byte* outBuf, int width, int height )
{
	ALIGN16( byte block[64] );
	ALIGN16( byte normal1[4] );
	ALIGN16( byte normal2[4] );

	assert( width >= 4 && ( width & 3 ) == 0 );
	assert( height >= 4 && ( height & 3 ) == 0 );

	this->width = width;
	this->height = height;
	this->outData = outBuf;

	for( int j = 0; j < height; j += 4, inBuf += width * 4 * 4 )
	{
		for( int i = 0; i < width; i += 4 )
		{
			commonLocal.LoadPacifierBinarizeProgressIncrement( 16 );

			ExtractBlock( inBuf + i * 4, width, block );

			GetMinMaxBBox( block, normal1, normal2 );
			InsetNormalsBBox3Dc( normal1, normal2 );

			// Write out Nx as an alpha channel.
			EmitByte( normal2[0] );
			EmitByte( normal1[0] );
			EmitAlphaIndices( block, 0, normal1[0], normal2[0] );

			// Write out Ny as an alpha channel.
			EmitByte( normal2[1] );
			EmitByte( normal1[1] );
			EmitAlphaIndices( block, 1, normal1[1], normal2[1] );
		}
		outData += dstPadding;
		inBuf += srcPadding;
	}
}

/*
========================
idDxtEncoder::DecodeDXNAlphaValues
========================
*/
void idDxtEncoder::DecodeDXNAlphaValues( const byte* inBuf, byte* values )
{
	int i;
	unsigned int indices;
	byte alphas[8];

	if( inBuf[0] <= inBuf[1] )
	{
		alphas[0] = inBuf[0];
		alphas[1] = inBuf[1];
		alphas[2] = ( 4 * alphas[0] + 1 * alphas[1] ) / 5;
		alphas[3] = ( 3 * alphas[0] + 2 * alphas[1] ) / 5;
		alphas[4] = ( 2 * alphas[0] + 3 * alphas[1] ) / 5;
		alphas[5] = ( 1 * alphas[0] + 4 * alphas[1] ) / 5;
		alphas[6] = 0;
		alphas[7] = 255;
	}
	else
	{
		alphas[0] = inBuf[0];
		alphas[1] = inBuf[1];
		alphas[2] = ( 6 * alphas[0] + 1 * alphas[1] ) / 7;
		alphas[3] = ( 5 * alphas[0] + 2 * alphas[1] ) / 7;
		alphas[4] = ( 4 * alphas[0] + 3 * alphas[1] ) / 7;
		alphas[5] = ( 3 * alphas[0] + 4 * alphas[1] ) / 7;
		alphas[6] = ( 2 * alphas[0] + 5 * alphas[1] ) / 7;
		alphas[7] = ( 1 * alphas[0] + 6 * alphas[1] ) / 7;
	}

	indices = ( int )inBuf[2] | ( ( int )inBuf[3] << 8 ) | ( ( int )inBuf[4] << 16 );
	for( i = 0; i < 8; i++ )
	{
		values[i] = alphas[indices & 7];
		indices >>= 3;
	}

	indices = ( int )inBuf[5] | ( ( int )inBuf[6] << 8 ) | ( ( int )inBuf[7] << 16 );
	for( i = 8; i < 16; i++ )
	{
		values[i] = alphas[indices & 7];
		indices >>= 3;
	}
}

/*
========================
idDxtEncoder::EncodeNormalRGBIndices

params:	values	- 16 normal block for which to find normal Y indices
paramO:	min		- Min grayscale value
paramO:	max		- Max grayscale value
========================
*/
void idDxtEncoder::EncodeNormalRGBIndices( byte* outBuf, const byte min, const byte max, const byte* values )
{

	const int COLOR_RANGE = 3;

	byte maskedMin, maskedMax, mid, yb1, yb2, yb3;

	maskedMax = max & C565_6_MASK;
	maskedMin = min & C565_6_MASK;
	mid = ( maskedMax - maskedMin ) / ( 2 * COLOR_RANGE );

	yb1 = maskedMax - mid;
	yb2 = ( 2 * maskedMax + 1 * maskedMin ) / COLOR_RANGE - mid;
	yb3 = ( 1 * maskedMax + 2 * maskedMin ) / COLOR_RANGE - mid;

	unsigned int result = 0;

	for( int i = 15; i >= 0; i-- )
	{
		result <<= 2;
		byte y = values[i];
		int b1 = ( y >= yb1 );
		int b2 = ( y >= yb2 );
		int b3 = ( y >= yb3 );
		int index = ( 4 - b1 - b2 - b3 ) & 3;
		index ^= ( 2 > index );
		result |= index;
	}

	unsigned short maskedMax5 = ( max & C565_5_MASK ) >> 3;
	unsigned short maskedMin5 = ( min & C565_5_MASK ) >> 3;

	unsigned short smax = ( maskedMax5 << 11 ) | ( maskedMax << 3 ) | maskedMax5;
	unsigned short smin = ( maskedMin5 << 11 ) | ( maskedMin << 3 ) | maskedMin5;

	outBuf[0] = byte( ( smax >> 0 ) & 0xFF );
	outBuf[1] = byte( ( smax >> 8 ) & 0xFF );
	outBuf[2] = byte( ( smin >> 0 ) & 0xFF );
	outBuf[3] = byte( ( smin >> 8 ) & 0xFF );

	outBuf[4] = byte( ( result >>  0 ) & 0xFF );
	outBuf[5] = byte( ( result >>  8 ) & 0xFF );
	outBuf[6] = byte( ( result >> 16 ) & 0xFF );
	outBuf[7] = byte( ( result >> 24 ) & 0xFF );
}

/*
========================
idDxtEncoder::ConvertNormalMapDXN2_DXT5

params:	inBuf		- normal map compressed in DXN2 format
paramO:	outBuf		- result of compression in DXT5 format
params:	width		- width of image
params:	height		- height of image
========================
*/
void idDxtEncoder::ConvertNormalMapDXN2_DXT5( const byte* inBuf, byte* outBuf, int width, int height )
{
	ALIGN16( byte values[16] );

	this->width = width;
	this->height = height;
	this->outData = outBuf;

	if( width > 4 && ( width & 3 ) != 0 )
	{
		return;
	}
	if( height > 4 && ( height & 3 ) != 0 )
	{
		return;
	}

	if( width < 4 || height < 4 )
	{
		assert( 0 );
		return;
	}

	for( int j = 0; j < height; j += 4 )
	{
		for( int i = 0; i < width; i += 4, inBuf += 16, outBuf += 16 )
		{
			commonLocal.LoadPacifierBinarizeProgressIncrement( 16 );

			// decode normal Y stored as a DXT5 alpha channel
			DecodeDXNAlphaValues( inBuf + 0, values );

			// copy normal X
			memcpy( outBuf + 0, inBuf + 8, 8 );

			// get the min/max Y
			byte minNormalY = 255;
			byte maxNormalY = 0;
			for( int i = 0; i < 16; i++ )
			{
				if( values[i] < minNormalY )
				{
					minNormalY = values[i];
				}
				if( values[i] > maxNormalY )
				{
					maxNormalY = values[i];
				}
			}

			// encode normal Y into DXT5 color channels
			EncodeNormalRGBIndices( outBuf + 8, minNormalY, maxNormalY, values );
		}
		outData += dstPadding;
		inBuf += srcPadding;
	}
}

/*
========================
idDxtEncoder::DecodeNormalYValues
========================
*/
void idDxtEncoder::DecodeNormalYValues( const byte* inBuf, byte& min, byte& max, byte* values )
{
	int i;
	unsigned int indexes;
	unsigned short normal0, normal1;
	byte normalsY[4];

	normal0 = inBuf[0] | ( inBuf[1] << 8 );
	normal1 = inBuf[2] | ( inBuf[3] << 8 );

	assert( normal0 >= normal1 );

	normalsY[0] = GreenFrom565( normal0 );
	normalsY[1] = GreenFrom565( normal1 );
	normalsY[2] = ( 2 * normalsY[0] + 1 * normalsY[1] ) / 3;
	normalsY[3] = ( 1 * normalsY[0] + 2 * normalsY[1] ) / 3;

	indexes = ( unsigned int )inBuf[4] | ( ( unsigned int )inBuf[5] << 8 ) | ( ( unsigned int )inBuf[6] << 16 ) | ( ( unsigned int )inBuf[7] << 24 );
	for( i = 0; i < 16; i++ )
	{
		values[i] = normalsY[indexes & 3];
		indexes >>= 2;
	}

	max = normalsY[0];
	min = normalsY[1];
}

/*
========================
idDxtEncoder::EncodeDXNAlphaValues
========================
*/
void idDxtEncoder::EncodeDXNAlphaValues( byte* outBuf, const byte min, const byte max, const byte* values )
{
	int i;
	byte alphas[8];
	int j;
	unsigned int indexes[16];

	alphas[0] = max;
	alphas[1] = min;
	alphas[2] = ( 6 * alphas[0] + 1 * alphas[1] ) / 7;
	alphas[3] = ( 5 * alphas[0] + 2 * alphas[1] ) / 7;
	alphas[4] = ( 4 * alphas[0] + 3 * alphas[1] ) / 7;
	alphas[5] = ( 3 * alphas[0] + 4 * alphas[1] ) / 7;
	alphas[6] = ( 2 * alphas[0] + 5 * alphas[1] ) / 7;
	alphas[7] = ( 1 * alphas[0] + 6 * alphas[1] ) / 7;

	int error = 0;
	for( i = 0; i < 16; i++ )
	{
		int minDist = MAX_TYPE( int );
		byte a = values[i];
		for( j = 0; j < 8; j++ )
		{
			int dist = AlphaDistance( a, alphas[j] );
			if( dist < minDist )
			{
				minDist = dist;
				indexes[i] = j;
			}
		}
		error += minDist;
	}

	outBuf[0] = max;
	outBuf[1] = min;

	outBuf[2] = byte( ( indexes[ 0] >> 0 ) | ( indexes[ 1] << 3 ) | ( indexes[ 2] << 6 ) );
	outBuf[3] = byte( ( indexes[ 2] >> 2 ) | ( indexes[ 3] << 1 ) | ( indexes[ 4] << 4 ) | ( indexes[ 5] << 7 ) );
	outBuf[4] = byte( ( indexes[ 5] >> 1 ) | ( indexes[ 6] << 2 ) | ( indexes[ 7] << 5 ) );

	outBuf[5] = byte( ( indexes[ 8] >> 0 ) | ( indexes[ 9] << 3 ) | ( indexes[10] << 6 ) );
	outBuf[6] = byte( ( indexes[10] >> 2 ) | ( indexes[11] << 1 ) | ( indexes[12] << 4 ) | ( indexes[13] << 7 ) );
	outBuf[7] = byte( ( indexes[13] >> 1 ) | ( indexes[14] << 2 ) | ( indexes[15] << 5 ) );
}

/*
========================
idDxtEncoder::ConvertNormalMapDXT5_DXN2

params:	inBuf		- image to compress
paramO:	outBuf		- result of compression
params:	width		- width of image
params:	height		- height of image
========================
*/
void idDxtEncoder::ConvertNormalMapDXT5_DXN2( const byte* inBuf, byte* outBuf, int width, int height )
{
	ALIGN16( byte values[16] );
	byte minNormalY, maxNormalY;

	this->width = width;
	this->height = height;
	this->outData = outBuf;

	if( width > 4 && ( width & 3 ) != 0 )
	{
		return;
	}
	if( height > 4 && ( height & 3 ) != 0 )
	{
		return;
	}

	if( width < 4 || height < 4 )
	{
		assert( 0 );
		return;
	}

	for( int j = 0; j < height; j += 4 )
	{
		for( int i = 0; i < width; i += 4, inBuf += 16, outBuf += 16 )
		{
			commonLocal.LoadPacifierBinarizeProgressIncrement( 16 );

			// decode normal Y stored as a DXT5 alpha channel
			DecodeNormalYValues( inBuf + 8, minNormalY, maxNormalY, values );

			memcpy( outBuf + 8, inBuf + 0, 8 );

			// encode normal Y into DXT5 green channel
			EncodeDXNAlphaValues( outBuf + 0, minNormalY, maxNormalY, values );
		}
		outData += dstPadding;
		inBuf += srcPadding;
	}
}

/*
========================
idDxtEncoder::ConvertImageDXN1_DXT1

params:	inBuf		- normal map compressed in DXN1 format
paramO:	outBuf		- result of compression in DXT1 format
params:	width		- width of image
params:	height		- height of image
========================
*/
void idDxtEncoder::ConvertImageDXN1_DXT1( const byte* inBuf, byte* outBuf, int width, int height )
{
	ALIGN16( byte values[16] );

	this->width = width;
	this->height = height;
	this->outData = outBuf;

	if( width > 4 && ( width & 3 ) != 0 )
	{
		return;
	}
	if( height > 4 && ( height & 3 ) != 0 )
	{
		return;
	}

	if( width < 4 || height < 4 )
	{
		assert( 0 );
		return;
	}

	for( int j = 0; j < height; j += 4 )
	{
		for( int i = 0; i < width; i += 4, inBuf += 8, outBuf += 8 )
		{
			commonLocal.LoadPacifierBinarizeProgressIncrement( 16 );

			// decode single channel stored as a DXT5 alpha channel
			DecodeDXNAlphaValues( inBuf + 0, values );

			// get the min/max
			byte min = 255;
			byte max = 0;

			// Dustin: corrected iteration
			for( int k = 0; k < 16; k++ )
			{
				if( values[k] < min )
				{
					min = values[k];
				}
				if( values[k] > max )
				{
					max = values[k];
				}
			}

			// encode single channel into DXT1
			EncodeNormalRGBIndices( outBuf + 0, min, max, values );
		}
		outData += dstPadding;
		inBuf += srcPadding;
	}
}
