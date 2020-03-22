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
/*
================================================================================================
Contains the DxtDecoder implementation.
================================================================================================
*/

#pragma hdrstop
#include "DXTCodec_local.h"
#include "DXTCodec.h"

/*
========================
idDxtDecoder::EmitBlock
========================
*/
void idDxtDecoder::EmitBlock( byte* outPtr, int x, int y, const byte* colorBlock )
{
	outPtr += ( y * width + x ) * 4;
	for( int j = 0; j < 4; j++ )
	{
		memcpy( outPtr, &colorBlock[j * 4 * 4], 4 * 4 );
		outPtr += width * 4;
	}
}

/*
========================
idDxtDecoder::DecodeAlphaValues
========================
*/
void idDxtDecoder::DecodeAlphaValues( byte* colorBlock, const int offset )
{
	int i;
	unsigned int indexes;
	byte alphas[8];

	alphas[0] = ReadByte();
	alphas[1] = ReadByte();

	if( alphas[0] > alphas[1] )
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

	colorBlock += offset;

	indexes = ( int )ReadByte() | ( ( int )ReadByte() << 8 ) | ( ( int )ReadByte() << 16 );
	for( i = 0; i < 8; i++ )
	{
		colorBlock[i * 4] = alphas[indexes & 7];
		indexes >>= 3;
	}

	indexes = ( int )ReadByte() | ( ( int )ReadByte() << 8 ) | ( ( int )ReadByte() << 16 );
	for( i = 8; i < 16; i++ )
	{
		colorBlock[i * 4] = alphas[indexes & 7];
		indexes >>= 3;
	}
}

/*
========================
idDxtDecoder::DecodeColorValues
========================
*/
void idDxtDecoder::DecodeColorValues( byte* colorBlock, bool noBlack, bool writeAlpha )
{
	byte colors[4][4];

	unsigned short color0 = ReadUShort();
	unsigned short color1 = ReadUShort();

	ColorFrom565( color0, colors[0] );
	ColorFrom565( color1, colors[1] );

	colors[0][3] = 255;
	colors[1][3] = 255;

	if( noBlack || color0 > color1 )
	{
		colors[2][0] = ( 2 * colors[0][0] + 1 * colors[1][0] ) / 3;
		colors[2][1] = ( 2 * colors[0][1] + 1 * colors[1][1] ) / 3;
		colors[2][2] = ( 2 * colors[0][2] + 1 * colors[1][2] ) / 3;
		colors[2][3] = 255;

		colors[3][0] = ( 1 * colors[0][0] + 2 * colors[1][0] ) / 3;
		colors[3][1] = ( 1 * colors[0][1] + 2 * colors[1][1] ) / 3;
		colors[3][2] = ( 1 * colors[0][2] + 2 * colors[1][2] ) / 3;
		colors[3][3] = 255;
	}
	else
	{
		colors[2][0] = ( 1 * colors[0][0] + 1 * colors[1][0] ) / 2;
		colors[2][1] = ( 1 * colors[0][1] + 1 * colors[1][1] ) / 2;
		colors[2][2] = ( 1 * colors[0][2] + 1 * colors[1][2] ) / 2;
		colors[2][3] = 255;

		colors[3][0] = 0;
		colors[3][1] = 0;
		colors[3][2] = 0;
		colors[3][3] = 0;
	}

	unsigned int indexes = ReadUInt();
	for( int i = 0; i < 16; i++ )
	{
		colorBlock[i * 4 + 0] = colors[indexes & 3][0];
		colorBlock[i * 4 + 1] = colors[indexes & 3][1];
		colorBlock[i * 4 + 2] = colors[indexes & 3][2];
		if( writeAlpha )
		{
			colorBlock[i * 4 + 3] = colors[indexes & 3][3];
		}
		indexes >>= 2;
	}
}

/*
========================
idDxtDecoder::DecodeCTX1Values
========================
*/
void idDxtDecoder::DecodeCTX1Values( byte* colorBlock )
{
	byte colors[4][2];

	colors[0][0] = ReadByte();
	colors[0][1] = ReadByte();
	colors[1][0] = ReadByte();
	colors[1][1] = ReadByte();

	colors[2][0] = ( 2 * colors[0][0] + 1 * colors[1][0] ) / 3;
	colors[2][1] = ( 2 * colors[0][1] + 1 * colors[1][1] ) / 3;
	colors[3][0] = ( 1 * colors[0][0] + 2 * colors[1][0] ) / 3;
	colors[3][1] = ( 1 * colors[0][1] + 2 * colors[1][1] ) / 3;

	unsigned int indexes = ReadUInt();
	for( int i = 0; i < 16; i++ )
	{
		colorBlock[i * 4 + 0] = colors[indexes & 3][0];
		colorBlock[i * 4 + 1] = colors[indexes & 3][1];
		indexes >>= 2;
	}
}

/*
========================
idDxtDecoder::DecompressImageDXT1
========================
*/
void idDxtDecoder::DecompressImageDXT1( const byte* inBuf, byte* outBuf, int width, int height )
{
	byte block[64];

	this->width = width;
	this->height = height;
	this->inData = inBuf;

	for( int j = 0; j < height; j += 4 )
	{
		for( int i = 0; i < width; i += 4 )
		{
			DecodeColorValues( block, false, true );
			EmitBlock( outBuf, i, j, block );
		}
	}
}

/*
========================
idDxtDecoder::DecompressImageDXT5
========================
*/
void idDxtDecoder::DecompressImageDXT5( const byte* inBuf, byte* outBuf, int width, int height )
{
	byte block[64];

	this->width = width;
	this->height = height;
	this->inData = inBuf;

	for( int j = 0; j < height; j += 4 )
	{
		for( int i = 0; i < width; i += 4 )
		{
			DecodeAlphaValues( block, 3 );
			DecodeColorValues( block, true, false );
			EmitBlock( outBuf, i, j, block );
		}
	}
}

/*
========================
idDxtDecoder::DecompressImageDXT5_nVidia7x
========================
*/
void idDxtDecoder::DecompressImageDXT5_nVidia7x( const byte* inBuf, byte* outBuf, int width, int height )
{
	byte block[64];

	this->width = width;
	this->height = height;
	this->inData = inBuf;

	for( int j = 0; j < height; j += 4 )
	{
		for( int i = 0; i < width; i += 4 )
		{
			DecodeAlphaValues( block, 3 );
			DecodeColorValues( block, false, false );
			EmitBlock( outBuf, i, j, block );
		}
	}
}

/*
========================
idDxtDecoder::DecompressYCoCgDXT5
========================
*/
void idDxtDecoder::DecompressYCoCgDXT5( const byte* inBuf, byte* outBuf, int width, int height )
{
	DecompressImageDXT5_nVidia7x( inBuf, outBuf, width, height );
	// descale the CoCg values and set the scale factor effectively to 1
	for( int i = 0; i < width * height; i++ )
	{
		int scale = ( outBuf[i * 4 + 2] >> 3 ) + 1;
		outBuf[i * 4 + 0] = byte( ( outBuf[i * 4 + 0] - 128 ) / scale + 128 );
		outBuf[i * 4 + 1] = byte( ( outBuf[i * 4 + 1] - 128 ) / scale + 128 );
		outBuf[i * 4 + 2] = 0;	// this translates to a scale factor of 1 for uncompressed
	}
}


/*
========================
idDxtDecoder::DecompressYCoCgCTX1DXT5A
========================
*/
void idDxtDecoder::DecompressYCoCgCTX1DXT5A( const byte* inBuf, byte* outBuf, int width, int height )
{
	byte block[64];

	this->width = width;
	this->height = height;
	this->inData = inBuf;

	for( int j = 0; j < height; j += 4 )
	{
		for( int i = 0; i < width; i += 4 )
		{
			DecodeAlphaValues( block, 3 );
			DecodeCTX1Values( block );
			EmitBlock( outBuf, i, j, block );
		}
	}
}

/*
========================
idDxtDecoder::DecodeNormalYValues
========================
*/
void idDxtDecoder::DecodeNormalYValues( byte* normalBlock, const int offsetY, byte& c0, byte& c1 )
{
	int i;
	unsigned int indexes;
	unsigned short normal0, normal1;
	byte normalsY[4];

	normal0 = ReadUShort();
	normal1 = ReadUShort();

	assert( normal0 >= normal1 );

	normalsY[0] = NormalYFrom565( normal0 );
	normalsY[1] = NormalYFrom565( normal1 );
	normalsY[2] = ( 2 * normalsY[0] + 1 * normalsY[1] ) / 3;
	normalsY[3] = ( 1 * normalsY[0] + 2 * normalsY[1] ) / 3;

	c0 = NormalBiasFrom565( normal0 );
	c1 = NormalScaleFrom565( normal0 );

	byte* normalYPtr = normalBlock + offsetY;

	indexes = ReadUInt();
	for( i = 0; i < 16; i++ )
	{
		normalYPtr[i * 4] = normalsY[indexes & 3];
		indexes >>= 2;
	}
}

/*
========================
UShortSqrt
========================
*/
byte UShortSqrt( unsigned short s )
{
#if 1
	int t, b, r, x;

	r = 0;
	for( b = 0x10000000; b != 0; b >>= 2 )
	{
		t = r + b;
		r >>= 1;
		x = -( t <= s );
		s = s - ( unsigned short )( t & x );
		r += b & x;
	}
	return byte( r );
#else
	int t, b, r;

	r = 0;
	for( b = 0x10000000; b != 0; b >>= 2 )
	{
		t = r + b;
		r >>= 1;
		if( t <= s )
		{
			s -= t;
			r += b;
		}
	}
	return r;
#endif
}

/*
========================
idDxtDecoder::DeriveNormalZValues
========================
*/
void idDxtDecoder::DeriveNormalZValues( byte* normalBlock )
{
	int i;

	for( i = 0; i < 16; i++ )
	{
		int x = normalBlock[i * 4 + 0] - 127;
		int y = normalBlock[i * 4 + 1] - 127;
		normalBlock[i * 4 + 2] = 128 + UShortSqrt( ( unsigned short )( 16383 - x * x - y * y ) );
	}
}

/*
========================
idDxtDecoder::UnRotateNormals
========================
*/
void UnRotateNormals( const byte* block, float* normals, byte c0, byte c1 )
{
	int rotation = c0;
	float angle = -( rotation / 255.0f ) * idMath::PI;
	float s = sin( angle );
	float c = cos( angle );

	int scale = ( c1 >> 3 ) + 1;
	for( int i = 0; i < 16; i++ )
	{
		float x = block[i * 4 + 0] / 255.0f * 2.0f - 1.0f;
		float y = ( ( block[i * 4 + 1] - 128 ) / scale + 128 ) / 255.0f * 2.0f - 1.0f;
		float rx = c * x - s * y;
		float ry = s * x + c * y;
		normals[i * 4 + 0] = rx;
		normals[i * 4 + 1] = ry;
	}
}

/*
========================
idDxtDecoder::DecompressNormalMapDXT1
========================
*/
void idDxtDecoder::DecompressNormalMapDXT1( const byte* inBuf, byte* outBuf, int width, int height )
{
	byte block[64];

	this->width = width;
	this->height = height;
	this->inData = inBuf;

	for( int j = 0; j < height; j += 4 )
	{
		for( int i = 0; i < width; i += 4 )
		{
			DecodeColorValues( block, false, true );
#if 1
			float normals[16 * 4];
			/*
			for ( int k = 0; k < 16; k++ ) {
				normals[k*4+0] = block[k*4+0] / 255.0f * 2.0f - 1.0f;
				normals[k*4+1] = block[k*4+1] / 255.0f * 2.0f - 1.0f;
			}
			*/
			UnRotateNormals( block, normals, block[0 * 4 + 2], 0 );
			for( int k = 0; k < 16; k++ )
			{
				float x = normals[k * 4 + 0];
				float y = normals[k * 4 + 1];
				float z = 1.0f - x * x - y * y;
				if( z < 0.0f )
				{
					z = 0.0f;
				}
				normals[k * 4 + 2] = sqrt( z );
			}
			for( int k = 0; k < 16; k++ )
			{
				block[k * 4 + 0] = idMath::Ftob( ( normals[k * 4 + 0] + 1.0f ) / 2.0f * 255.0f );
				block[k * 4 + 1] = idMath::Ftob( ( normals[k * 4 + 1] + 1.0f ) / 2.0f * 255.0f );
				block[k * 4 + 2] = idMath::Ftob( ( normals[k * 4 + 2] + 1.0f ) / 2.0f * 255.0f );
			}
#else
			DeriveNormalZValues( block );
#endif
			EmitBlock( outBuf, i, j, block );
		}
	}
}

/*
========================
idDxtDecoder::DecompressNormalMapDXT1Renormalize
========================
*/
void idDxtDecoder::DecompressNormalMapDXT1Renormalize( const byte* inBuf, byte* outBuf, int width, int height )
{
	byte block[64];

	this->width = width;
	this->height = height;
	this->inData = inBuf;

	for( int j = 0; j < height; j += 4 )
	{
		for( int i = 0; i < width; i += 4 )
		{
			DecodeColorValues( block, false, true );

			for( int k = 0; k < 16; k++ )
			{
				float normal[3];
				normal[0] = block[k * 4 + 0] / 255.0f * 2.0f - 1.0f;
				normal[1] = block[k * 4 + 1] / 255.0f * 2.0f - 1.0f;
				normal[2] = block[k * 4 + 2] / 255.0f * 2.0f - 1.0f;
				float rsq = idMath::InvSqrt( normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2] );
				normal[0] *= rsq;
				normal[1] *= rsq;
				normal[2] *= rsq;
				block[k * 4 + 0] = idMath::Ftob( ( normal[0] + 1.0f ) / 2.0f * 255.0f + 0.5f );
				block[k * 4 + 1] = idMath::Ftob( ( normal[1] + 1.0f ) / 2.0f * 255.0f + 0.5f );
				block[k * 4 + 2] = idMath::Ftob( ( normal[2] + 1.0f ) / 2.0f * 255.0f + 0.5f );
			}

			EmitBlock( outBuf, i, j, block );
		}
	}
}

/*
========================
idDxtDecoder::DecompressNormalMapDXT5Renormalize
========================
*/
void idDxtDecoder::DecompressNormalMapDXT5Renormalize( const byte* inBuf, byte* outBuf, int width, int height )
{
	byte block[64];

	this->width = width;
	this->height = height;
	this->inData = inBuf;

	for( int j = 0; j < height; j += 4 )
	{
		for( int i = 0; i < width; i += 4 )
		{
			DecodeAlphaValues( block, 3 );
			DecodeColorValues( block, false, false );

			for( int k = 0; k < 16; k++ )
			{
				float normal[3];
#if 0 // object-space
				normal[0] = block[k * 4 + 0] / 255.0f * 2.0f - 1.0f;
				normal[1] = block[k * 4 + 1] / 255.0f * 2.0f - 1.0f;
				normal[2] = block[k * 4 + 3] / 255.0f * 2.0f - 1.0f;
#else
				normal[0] = block[k * 4 + 3] / 255.0f * 2.0f - 1.0f;
				normal[1] = block[k * 4 + 1] / 255.0f * 2.0f - 1.0f;
				normal[2] = block[k * 4 + 2] / 255.0f * 2.0f - 1.0f;
#endif
				float rsq = idMath::InvSqrt( normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2] );
				normal[0] *= rsq;
				normal[1] *= rsq;
				normal[2] *= rsq;
				block[k * 4 + 0] = idMath::Ftob( ( normal[0] + 1.0f ) / 2.0f * 255.0f + 0.5f );
				block[k * 4 + 1] = idMath::Ftob( ( normal[1] + 1.0f ) / 2.0f * 255.0f + 0.5f );
				block[k * 4 + 2] = idMath::Ftob( ( normal[2] + 1.0f ) / 2.0f * 255.0f + 0.5f );
			}

			EmitBlock( outBuf, i, j, block );
		}
	}
}

/*
========================
idDxtDecoder::BiasScaleNormalY
========================
*/
void BiasScaleNormalY( byte* normals, const int offsetY, const byte c0, const byte c1 )
{
	int bias = c0 - 4;
	int scale = ( c1 >> 3 ) + 1;
	for( int i = 0; i < 16; i++ )
	{
		normals[i * 4 + offsetY] = byte( ( normals[i * 4 + offsetY] - 128 ) / scale + bias );
	}
}

/*
========================
idDxtDecoder::BiasScaleNormals
========================
*/
void BiasScaleNormals( const byte* block, float* normals, const byte c0, const byte c1 )
{
	int bias = c0 - 4;
	int scale = ( c1 >> 3 ) + 1;
	for( int i = 0; i < 16; i++ )
	{
		normals[i * 4 + 0] = block[i * 4 + 0] / 255.0f * 2.0f - 1.0f;
		normals[i * 4 + 1] = ( ( block[i * 4 + 1] - 128.0f ) / scale + bias ) / 255.0f * 2.0f - 1.0f;
	}
}

/*
========================
idDxtDecoder::DecompressNormalMapDXT5
========================
*/
void idDxtDecoder::DecompressNormalMapDXT5( const byte* inBuf, byte* outBuf, int width, int height )
{
	byte block[64];
	byte c0, c1;

	this->width = width;
	this->height = height;
	this->inData = inBuf;

	for( int j = 0; j < height; j += 4 )
	{
		for( int i = 0; i < width; i += 4 )
		{
			DecodeAlphaValues( block, 0 );
			DecodeNormalYValues( block, 1, c0, c1 );
#if 1
			float normals[16 * 4];
			//BiasScaleNormals( block, normals, c0, c1 );
			UnRotateNormals( block, normals, c0, c1 );
			for( int k = 0; k < 16; k++ )
			{
				float x = normals[k * 4 + 0];
				float y = normals[k * 4 + 1];
				float z = 1.0f - x * x - y * y;
				if( z < 0.0f )
				{
					z = 0.0f;
				}
				normals[k * 4 + 2] = sqrt( z );
			}
			for( int k = 0; k < 16; k++ )
			{
				block[k * 4 + 0] = idMath::Ftob( ( normals[k * 4 + 0] + 1.0f ) / 2.0f * 255.0f );
				block[k * 4 + 1] = idMath::Ftob( ( normals[k * 4 + 1] + 1.0f ) / 2.0f * 255.0f );
				block[k * 4 + 2] = idMath::Ftob( ( normals[k * 4 + 2] + 1.0f ) / 2.0f * 255.0f );
			}
#else
			BiasScaleNormalY( block, 1, c0, c1 );
			DeriveNormalZValues( block );
#endif

			EmitBlock( outBuf, i, j, block );
		}
	}
}

/*
========================
idDxtDecoder::DecompressNormalMapDXN2
========================
*/
void idDxtDecoder::DecompressNormalMapDXN2( const byte* inBuf, byte* outBuf, int width, int height )
{
	byte block[64];

	this->width = width;
	this->height = height;
	this->inData = inBuf;

	for( int j = 0; j < height; j += 4 )
	{
		for( int i = 0; i < width; i += 4 )
		{
			DecodeAlphaValues( block, 0 );
			DecodeAlphaValues( block, 1 );
#if 1
			float normals[16 * 4];
			for( int k = 0; k < 16; k++ )
			{
				normals[k * 4 + 0] = block[k * 4 + 0] / 255.0f * 2.0f - 1.0f;
				normals[k * 4 + 1] = block[k * 4 + 1] / 255.0f * 2.0f - 1.0f;
			}
			for( int k = 0; k < 16; k++ )
			{
				float x = normals[k * 4 + 0];
				float y = normals[k * 4 + 1];
				float z = 1.0f - x * x - y * y;
				if( z < 0.0f )
				{
					z = 0.0f;
				}
				normals[k * 4 + 2] = sqrt( z );
			}
			for( int k = 0; k < 16; k++ )
			{
				block[k * 4 + 0] = idMath::Ftob( ( normals[k * 4 + 0] + 1.0f ) / 2.0f * 255.0f );
				block[k * 4 + 1] = idMath::Ftob( ( normals[k * 4 + 1] + 1.0f ) / 2.0f * 255.0f );
				block[k * 4 + 2] = idMath::Ftob( ( normals[k * 4 + 2] + 1.0f ) / 2.0f * 255.0f );
			}
#else
			DeriveNormalZValues( block );
#endif
			EmitBlock( outBuf, i, j, block );
		}
	}
}

/*
========================
idDxtDecoder::DecomposeColorBlock
========================
*/
void idDxtDecoder::DecomposeColorBlock( byte colors[2][4], byte colorIndices[16], bool noBlack )
{
	int i;
	unsigned int indices;
	unsigned short color0, color1;
	int colorRemap1[] = { 3, 0, 2, 1 };
	int colorRemap2[] = { 1, 3, 2, 0 };
	int* crm;

	color0 = ReadUShort();
	color1 = ReadUShort();

	ColorFrom565( color0, colors[0] );
	ColorFrom565( color1, colors[1] );

	if( noBlack || color0 > color1 )
	{
		crm = colorRemap1;
	}
	else
	{
		crm = colorRemap2;
	}

	indices = ReadUInt();
	for( i = 0; i < 16; i++ )
	{
		colorIndices[i] = ( byte )crm[ indices & 3 ];
		indices >>= 2;
	}
}

/*
========================
idDxtDecoder::DecomposeAlphaBlock
========================
*/
void idDxtDecoder::DecomposeAlphaBlock( byte colors[2][4], byte alphaIndices[16] )
{
	int i;
	unsigned char alpha0, alpha1;
	unsigned int indices;
	int alphaRemap1[] = { 7, 0, 6, 5, 4, 3, 2, 1 };
	int alphaRemap2[] = { 1, 6, 2, 3, 4, 5, 0, 7 };
	int* arm;

	alpha0 = ReadByte();
	alpha1 = ReadByte();

	colors[0][3] = alpha0;
	colors[1][3] = alpha1;

	if( alpha0 > alpha1 )
	{
		arm = alphaRemap1;
	}
	else
	{
		arm = alphaRemap2;
	}

	indices = ( int )ReadByte() | ( ( int )ReadByte() << 8 ) | ( ( int )ReadByte() << 16 );
	for( i = 0; i < 8; i++ )
	{
		alphaIndices[i] = ( byte )arm[ indices & 7 ];
		indices >>= 3;
	}

	indices = ( int )ReadByte() | ( ( int )ReadByte() << 8 ) | ( ( int )ReadByte() << 16 );
	for( i = 8; i < 16; i++ )
	{
		alphaIndices[i] = ( byte )arm[ indices & 7 ];
		indices >>= 3;
	}
}

/*
========================
idDxtDecoder::DecomposeImageDXT1
========================
*/
void idDxtDecoder::DecomposeImageDXT1( const byte* inBuf, byte* colorIndices, byte* pic1, byte* pic2, int width, int height )
{
	byte colors[2][4];
	byte indices[16];

	this->width = width;
	this->height = height;
	this->inData = inBuf;

	// extract the colors from the DXT
	for( int j = 0; j < height; j += 4 )
	{
		for( int i = 0; i < width; i += 4 )
		{
			DecomposeColorBlock( colors, indices, false );

			memcpy( colorIndices + ( j + 0 ) * width + i, indices + 0, 4 );
			memcpy( colorIndices + ( j + 1 ) * width + i, indices + 4, 4 );
			memcpy( colorIndices + ( j + 2 ) * width + i, indices + 8, 4 );
			memcpy( colorIndices + ( j + 3 ) * width + i, indices + 12, 4 );

			memcpy( pic1 + j * width / 4 + i, colors[0], 4 );

			memcpy( pic2 + j * width / 4 + i, colors[1], 4 );
		}
	}
}

/*
========================
idDxtDecoder::DecomposeImageDXT5
========================
*/
void idDxtDecoder::DecomposeImageDXT5( const byte* inBuf, byte* colorIndices, byte* alphaIndices, byte* pic1, byte* pic2, int width, int height )
{
	byte colors[2][4];
	byte colorInd[16];
	byte alphaInd[16];

	this->width = width;
	this->height = height;
	this->inData = inBuf;

	// extract the colors from the DXT
	for( int j = 0; j < height; j += 4 )
	{
		for( int i = 0; i < width; i += 4 )
		{
			DecomposeAlphaBlock( colors, alphaInd );
			DecomposeColorBlock( colors, colorInd, true );

			memcpy( colorIndices + ( j + 0 ) * width + i, colorInd + 0, 4 );
			memcpy( colorIndices + ( j + 1 ) * width + i, colorInd + 4, 4 );
			memcpy( colorIndices + ( j + 2 ) * width + i, colorInd + 8, 4 );
			memcpy( colorIndices + ( j + 3 ) * width + i, colorInd + 12, 4 );

			memcpy( colorIndices + ( j + 0 ) * width + i, alphaInd + 0, 4 );
			memcpy( colorIndices + ( j + 1 ) * width + i, alphaInd + 4, 4 );
			memcpy( colorIndices + ( j + 2 ) * width + i, alphaInd + 8, 4 );
			memcpy( colorIndices + ( j + 3 ) * width + i, alphaInd + 12, 4 );

			memcpy( pic1 + j * width / 4 + i, colors[0], 4 );

			memcpy( pic2 + j * width / 4 + i, colors[1], 4 );
		}
	}
}

