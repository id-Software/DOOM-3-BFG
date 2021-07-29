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
Contains the ColorSpace conversion implementation.
================================================================================================
*/

#include "ColorSpace.h"

/*
================================================================================================
To *Color-Convert RGB and YCoCg* ColorSpaces, use the following conversions:


	Y  = [ 1/4  1/2  1/4] [R]
	Co = [ 1/2    0 -1/2] [G] + 128
	CG = [-1/4  1/2 -1/4] [B] + 128

	R  = [   1    1   -1] [Y]
	G  = [   1    0    1] [Co - 128]
	B  = [   1   -1   -1] [Cg - 128]

================================================================================================
*/

#define RGB_TO_YCOCG_Y( r, g, b )	( ( (    r +   (g<<1) +  b     ) + 2 ) >> 2 )
#define RGB_TO_YCOCG_CO( r, g, b )	( ( (   (r<<1)        - (b<<1) ) + 2 ) >> 2 )
#define RGB_TO_YCOCG_CG( r, g, b )	( ( ( -  r +   (g<<1) -  b     ) + 2 ) >> 2 )

#define COCG_TO_R( co, cg )			( co - cg )
#define COCG_TO_G( co, cg )			( cg )
#define COCG_TO_B( co, cg )			( - co - cg )

/*
========================
idColorSpace::ConvertRGBToYCoCg
========================
*/
void idColorSpace::ConvertRGBToYCoCg( byte* dst, const byte* src, int width, int height )
{
	for( int i = 0; i < width * height; i++ )
	{
		int r = src[i * 4 + 0];
		int g = src[i * 4 + 1];
		int b = src[i * 4 + 2];
		int a = src[i * 4 + 3];
		dst[i * 4 + 0] = CLAMP_BYTE( RGB_TO_YCOCG_Y( r, g, b ) );
		dst[i * 4 + 1] = CLAMP_BYTE( RGB_TO_YCOCG_CO( r, g, b ) + 128 );
		dst[i * 4 + 2] = CLAMP_BYTE( RGB_TO_YCOCG_CG( r, g, b ) + 128 );
		dst[i * 4 + 3] = a;
	}
}

/*
========================
idColorSpace::ConvertYCoCgToRGB
========================
*/
void idColorSpace::ConvertYCoCgToRGB( byte* dst, const byte* src, int width, int height )
{
	for( int i = 0; i < width * height; i++ )
	{
		int y  = src[i * 4 + 0];
		int co = src[i * 4 + 1] - 128;
		int cg = src[i * 4 + 2] - 128;
		int a  = src[i * 4 + 3];
		dst[i * 4 + 0] = CLAMP_BYTE( y + COCG_TO_R( co, cg ) );
		dst[i * 4 + 1] = CLAMP_BYTE( y + COCG_TO_G( co, cg ) );
		dst[i * 4 + 2] = CLAMP_BYTE( y + COCG_TO_B( co, cg ) );
		dst[i * 4 + 3] = a;
	}
}

/*
========================
idColorSpace::ConvertRGBToCoCg_Y
========================
*/
void idColorSpace::ConvertRGBToCoCg_Y( byte* dst, const byte* src, int width, int height )
{
	for( int i = 0; i < width * height; i++ )
	{
		int r = src[i * 4 + 0];
		int g = src[i * 4 + 1];
		int b = src[i * 4 + 2];
		//int a = src[i*4+3];
		dst[i * 4 + 0] = CLAMP_BYTE( RGB_TO_YCOCG_CO( r, g, b ) + 128 );
		dst[i * 4 + 1] = CLAMP_BYTE( RGB_TO_YCOCG_CG( r, g, b ) + 128 );
		dst[i * 4 + 2] = 0;
		dst[i * 4 + 3] = CLAMP_BYTE( RGB_TO_YCOCG_Y( r, g, b ) );
	}
}

/*
========================
idColorSpace::ConvertCoCg_YToRGB
========================
*/
void idColorSpace::ConvertCoCg_YToRGB( byte* dst, const byte* src, int width, int height )
{
	for( int i = 0; i < width * height; i++ )
	{
		int co = src[i * 4 + 0] - 128;
		int cg = src[i * 4 + 1] - 128;
		int a  = src[i * 4 + 2];
		int y  = src[i * 4 + 3];
		dst[i * 4 + 0] = CLAMP_BYTE( y + COCG_TO_R( co, cg ) );
		dst[i * 4 + 1] = CLAMP_BYTE( y + COCG_TO_G( co, cg ) );
		dst[i * 4 + 2] = CLAMP_BYTE( y + COCG_TO_B( co, cg ) );
		dst[i * 4 + 3] = a;
	}
}

/*
========================
idColorSpace::ConvertCoCgSYToRGB

A scale factor is encoded in the Z value to give better compression of
the color channels.
========================
*/
void idColorSpace::ConvertCoCgSYToRGB( byte* dst, const byte* src, int width, int height )
{
	for( int i = 0; i < width * height; i++ )
	{
		int co = src[i * 4 + 0] - 128;
		int cg = src[i * 4 + 1] - 128;
		int a  = src[i * 4 + 2];
		int y  = src[i * 4 + 3];
		float	scale = 1.0f / ( 1.0f + a * ( 31.875f / 255.0f ) ) ;
		co = idMath::Ftoi( co * scale );
		cg = idMath::Ftoi( cg * scale );
		dst[i * 4 + 0] = CLAMP_BYTE( y + COCG_TO_R( co, cg ) );
		dst[i * 4 + 1] = CLAMP_BYTE( y + COCG_TO_G( co, cg ) );
		dst[i * 4 + 2] = CLAMP_BYTE( y + COCG_TO_B( co, cg ) );
		dst[i * 4 + 3] = 255;
	}
}

/*
========================
idColorSpace::ConvertRGBToYCoCg420
========================
*/
void idColorSpace::ConvertRGBToYCoCg420( byte* dst, const byte* src, int width, int height )
{
	int numSamples = 0;
	for( int j = 0; j < height; j += 2 )
	{
		for( int i = 0; i < width; i += 2 )
		{
			int r0 = src[( ( j + 0 ) * width + i + 0 ) * 4 + 0];
			int g0 = src[( ( j + 0 ) * width + i + 0 ) * 4 + 1];
			int b0 = src[( ( j + 0 ) * width + i + 0 ) * 4 + 2];
			int r1 = src[( ( j + 0 ) * width + i + 1 ) * 4 + 0];
			int g1 = src[( ( j + 0 ) * width + i + 1 ) * 4 + 1];
			int b1 = src[( ( j + 0 ) * width + i + 1 ) * 4 + 2];
			int r2 = src[( ( j + 1 ) * width + i + 0 ) * 4 + 0];
			int g2 = src[( ( j + 1 ) * width + i + 0 ) * 4 + 1];
			int b2 = src[( ( j + 1 ) * width + i + 0 ) * 4 + 2];
			int r3 = src[( ( j + 1 ) * width + i + 1 ) * 4 + 0];
			int g3 = src[( ( j + 1 ) * width + i + 1 ) * 4 + 1];
			int b3 = src[( ( j + 1 ) * width + i + 1 ) * 4 + 2];
			int y0  = CLAMP_BYTE( RGB_TO_YCOCG_Y( r0, g0, b0 ) );
			int co0 = CLAMP_BYTE( RGB_TO_YCOCG_CO( r0, g0, b0 ) + 128 );
			int cg0 = CLAMP_BYTE( RGB_TO_YCOCG_CG( r0, g0, b0 ) + 128 );
			int y1  = CLAMP_BYTE( RGB_TO_YCOCG_Y( r1, g1, b1 ) );
			int co1 = CLAMP_BYTE( RGB_TO_YCOCG_CO( r1, g1, b1 ) + 128 );
			int cg1 = CLAMP_BYTE( RGB_TO_YCOCG_CG( r1, g1, b1 ) + 128 );
			int y2  = CLAMP_BYTE( RGB_TO_YCOCG_Y( r2, g2, b2 ) );
			int co2 = CLAMP_BYTE( RGB_TO_YCOCG_CO( r2, g2, b2 ) + 128 );
			int cg2 = CLAMP_BYTE( RGB_TO_YCOCG_CG( r2, g2, b2 ) + 128 );
			int y3  = CLAMP_BYTE( RGB_TO_YCOCG_Y( r3, g3, b3 ) );
			int co3 = CLAMP_BYTE( RGB_TO_YCOCG_CO( r3, g3, b3 ) + 128 );
			int cg3 = CLAMP_BYTE( RGB_TO_YCOCG_CG( r3, g3, b3 ) + 128 );
			dst[numSamples + 0] = y0;
			dst[numSamples + 1] = y1;
			dst[numSamples + 2] = y2;
			dst[numSamples + 3] = y3;
			dst[numSamples + 4] = ( co0 + co1 + co2 + co3 ) >> 2;
			dst[numSamples + 5] = ( cg0 + cg1 + cg2 + cg3 ) >> 2;
			numSamples += 6;
		}
		numSamples += width;
	}
}

/*
========================
idColorSpace::ConvertYCoCg420ToRGB
========================
*/
void idColorSpace::ConvertYCoCg420ToRGB( byte* dst, const byte* src, int width, int height )
{
	int numSamples = width * height * 2 - width;
	for( int j = height - 2; j >= 0; j -= 2 )
	{
		for( int i = width - 2; i >= 0; i -= 2 )
		{
			int y0 = src[numSamples - 6];
			int y1 = src[numSamples - 5];
			int y2 = src[numSamples - 4];
			int y3 = src[numSamples - 3];
			int co = src[numSamples - 2] - 128;
			int cg = src[numSamples - 1] - 128;
			numSamples -= 6;
			int r = COCG_TO_R( co, cg );
			int g = COCG_TO_G( co, cg );
			int b = COCG_TO_B( co, cg );
			dst[( ( j + 0 )*width + i + 0 ) * 4 + 0] = CLAMP_BYTE( y0 + r );
			dst[( ( j + 0 )*width + i + 0 ) * 4 + 1] = CLAMP_BYTE( y0 + g );
			dst[( ( j + 0 )*width + i + 0 ) * 4 + 2] = CLAMP_BYTE( y0 + b );
			dst[( ( j + 0 )*width + i + 1 ) * 4 + 0] = CLAMP_BYTE( y1 + r );
			dst[( ( j + 0 )*width + i + 1 ) * 4 + 1] = CLAMP_BYTE( y1 + g );
			dst[( ( j + 0 )*width + i + 1 ) * 4 + 2] = CLAMP_BYTE( y1 + b );
			dst[( ( j + 1 )*width + i + 0 ) * 4 + 0] = CLAMP_BYTE( y2 + r );
			dst[( ( j + 1 )*width + i + 0 ) * 4 + 1] = CLAMP_BYTE( y2 + g );
			dst[( ( j + 1 )*width + i + 0 ) * 4 + 2] = CLAMP_BYTE( y2 + b );
			dst[( ( j + 1 )*width + i + 1 ) * 4 + 0] = CLAMP_BYTE( y3 + r );
			dst[( ( j + 1 )*width + i + 1 ) * 4 + 1] = CLAMP_BYTE( y3 + g );
			dst[( ( j + 1 )*width + i + 1 ) * 4 + 2] = CLAMP_BYTE( y3 + b );
		}
		numSamples -= width;
	}
}

/*
================================================================================================
To *Color-Convert RGB and YCbCr* ColorSpaces, note that YCbCr is defined per
CCIR 601-1, except that Cb and Cr are normalized to the range 0 -> 255 rather than -0.5 -> 0.5.
The conversion equations to be implemented are therefore:


	Y  = [ 0.29900  0.58700  0.11400] [R]
	Cb = [-0.16874 -0.33126  0.50000] [G] + 128
	Cr = [ 0.50000 -0.41869 -0.08131] [B] + 128

	R  = [ 1.00000  0.00000  1.40200] [Y]
	G  = [ 1.00000 -0.34414 -0.71414] [Cb - 128]
	B  = [ 1.00000  1.77200  0.00000] [Cr - 128]


These numbers are derived from TIFF 6.0 section 21, dated 3-June-92. To avoid floating-point
arithmetic, we represent the fractional constants as integers scaled up by 2^16 (about 4 digits
precision); we have to divide the products by 2^16, with appropriate rounding, to get the
correct answer.
================================================================================================
*/

const int ycbcr_shift	= 16;
const int ycbcr_round	= 1 << ( ycbcr_shift - 1 );

const int r029900		= 19595;	// int( 0.29900 * (1<<16) + 0.5 )
const int g058700		= 38470;	// int( 0.58700 * (1<<16) + 0.5 )
const int b011400		= 7471;		// int( 0.11400 * (1<<16) + 0.5 )

const int r016874		= 11059;	// int( 0.16874 * (1<<16) + 0.5 )
const int g033126		= 21709;	// int( 0.33126 * (1<<16) + 0.5 )
const int b050000		= 32768;	// int( 0.50000 * (1<<16) + 0.5 )

const int r050000		= 32768;	// int( 0.50000 * (1<<16) + 0.5 )
const int g041869		= 27439;	// int( 0.41869 * (1<<16) + 0.5 )
const int b008131		= 5329;		// int( 0.08131 * (1<<16) + 0.5 )

const int r140200		= 91881;	// int( 1.40200 * (1<<16) + 0.5 )
const int b177200		= 116130;	// int( 1.77200 * (1<<16) + 0.5 )
const int g071414		= 46802;	// int( 0.71414 * (1<<16) + 0.5 )
const int g034414		= 22554;	// int( 0.34414 * (1<<16) + 0.5 )

#define RGB_TO_YCBCR_Y( r, g, b )	( ( (   r * r029900 + g * g058700 + b * b011400 ) + ycbcr_round ) >> ycbcr_shift )
#define RGB_TO_YCBCR_CB( r, g, b )	( ( ( - r * r016874 - g * g033126 + b * b050000 ) + ycbcr_round ) >> ycbcr_shift )
#define RGB_TO_YCBCR_CR( r, g, b )	( ( (   r * r050000 - g * g041869 - b * b008131 ) + ycbcr_round ) >> ycbcr_shift )

#define CBCR_TO_R( cb, cr )			( ( ycbcr_round + cr * r140200 ) >> ycbcr_shift )
#define CBCR_TO_G( cb, cr )			( ( ycbcr_round - cb * g034414 - cr * g071414 ) >> ycbcr_shift )
#define CBCR_TO_B( cb, cr )			( ( ycbcr_round + cb * b177200 ) >> ycbcr_shift )

/*
========================
idColorSpace::ConvertRGBToYCbCr
========================
*/
void idColorSpace::ConvertRGBToYCbCr( byte* dst, const byte* src, int width, int height )
{
	for( int i = 0; i < width * height; i++ )
	{
		int r = src[i * 4 + 0];
		int g = src[i * 4 + 1];
		int b = src[i * 4 + 2];
		int a = src[i * 4 + 3];
		dst[i * 4 + 0] = CLAMP_BYTE( RGB_TO_YCBCR_Y( r, g, b ) );
		dst[i * 4 + 1] = CLAMP_BYTE( RGB_TO_YCBCR_CB( r, g, b ) + 128 );
		dst[i * 4 + 2] = CLAMP_BYTE( RGB_TO_YCBCR_CR( r, g, b ) + 128 );
		dst[i * 4 + 3] = a;
	}
}

/*
========================
idColorSpace::ConvertYCbCrToRGB
========================
*/
void idColorSpace::ConvertYCbCrToRGB( byte* dst, const byte* src, int width, int height )
{
	for( int i = 0; i < width * height; i++ )
	{
		int y  = src[i * 4 + 0];
		int cb = src[i * 4 + 1] - 128;
		int cr = src[i * 4 + 2] - 128;
		dst[i * 4 + 0] = CLAMP_BYTE( y + CBCR_TO_R( cb, cr ) );
		dst[i * 4 + 1] = CLAMP_BYTE( y + CBCR_TO_G( cb, cr ) );
		dst[i * 4 + 2] = CLAMP_BYTE( y + CBCR_TO_B( cb, cr ) );
	}
}

/*
========================
idColorSpace::ConvertRGBToCbCr_Y
========================
*/
void idColorSpace::ConvertRGBToCbCr_Y( byte* dst, const byte* src, int width, int height )
{
	for( int i = 0; i < width * height; i++ )
	{
		int r = src[i * 4 + 0];
		int g = src[i * 4 + 1];
		int b = src[i * 4 + 2];
		int a = src[i * 4 + 3];
		dst[i * 4 + 0] = CLAMP_BYTE( RGB_TO_YCBCR_CB( r, g, b ) + 128 );
		dst[i * 4 + 1] = CLAMP_BYTE( RGB_TO_YCBCR_CR( r, g, b ) + 128 );
		dst[i * 4 + 2] = a;
		dst[i * 4 + 3] = CLAMP_BYTE( RGB_TO_YCBCR_Y( r, g, b ) );
	}
}

/*
========================
idColorSpace::ConvertCbCr_YToRGB
========================
*/
void idColorSpace::ConvertCbCr_YToRGB( byte* dst, const byte* src, int width, int height )
{
	for( int i = 0; i < width * height; i++ )
	{
		int cb = src[i * 4 + 0] - 128;
		int cr = src[i * 4 + 1] - 128;
		int a  = src[i * 4 + 2];
		int y  = src[i * 4 + 3];
		dst[i * 4 + 0] = CLAMP_BYTE( y + CBCR_TO_R( cb, cr ) );
		dst[i * 4 + 1] = CLAMP_BYTE( y + CBCR_TO_G( cb, cr ) );
		dst[i * 4 + 2] = CLAMP_BYTE( y + CBCR_TO_B( cb, cr ) );
		dst[i * 4 + 3] = a;
	}
}

/*
========================
idColorSpace::ConvertRGBToYCbCr420
========================
*/
void idColorSpace::ConvertRGBToYCbCr420( byte* dst, const byte* src, int width, int height )
{
	int numSamples = 0;
	for( int j = 0; j < height; j += 2 )
	{
		for( int i = 0; i < width; i += 2 )
		{
			int r0 = src[( ( j + 0 ) * width + i + 0 ) * 4 + 0];
			int g0 = src[( ( j + 0 ) * width + i + 0 ) * 4 + 1];
			int b0 = src[( ( j + 0 ) * width + i + 0 ) * 4 + 2];
			int r1 = src[( ( j + 0 ) * width + i + 1 ) * 4 + 0];
			int g1 = src[( ( j + 0 ) * width + i + 1 ) * 4 + 1];
			int b1 = src[( ( j + 0 ) * width + i + 1 ) * 4 + 2];
			int r2 = src[( ( j + 1 ) * width + i + 0 ) * 4 + 0];
			int g2 = src[( ( j + 1 ) * width + i + 0 ) * 4 + 1];
			int b2 = src[( ( j + 1 ) * width + i + 0 ) * 4 + 2];
			int r3 = src[( ( j + 1 ) * width + i + 1 ) * 4 + 0];
			int g3 = src[( ( j + 1 ) * width + i + 1 ) * 4 + 1];
			int b3 = src[( ( j + 1 ) * width + i + 1 ) * 4 + 2];
			int y0  = CLAMP_BYTE( RGB_TO_YCBCR_Y( r0, g0, b0 ) );
			int cb0 = CLAMP_BYTE( RGB_TO_YCBCR_CB( r0, g0, b0 ) + 128 );
			int cr0 = CLAMP_BYTE( RGB_TO_YCBCR_CR( r0, g0, b0 ) + 128 );
			int y1  = CLAMP_BYTE( RGB_TO_YCBCR_Y( r1, g1, b1 ) );
			int cb1 = CLAMP_BYTE( RGB_TO_YCBCR_CB( r1, g1, b1 ) + 128 );
			int cr1 = CLAMP_BYTE( RGB_TO_YCBCR_CR( r1, g1, b1 ) + 128 );
			int y2  = CLAMP_BYTE( RGB_TO_YCBCR_Y( r2, g2, b2 ) );
			int cb2 = CLAMP_BYTE( RGB_TO_YCBCR_CB( r2, g2, b2 ) + 128 );
			int cr2 = CLAMP_BYTE( RGB_TO_YCBCR_CR( r2, g2, b2 ) + 128 );
			int y3  = CLAMP_BYTE( RGB_TO_YCBCR_Y( r3, g3, b3 ) );
			int cb3 = CLAMP_BYTE( RGB_TO_YCBCR_CB( r3, g3, b3 ) + 128 );
			int cr3 = CLAMP_BYTE( RGB_TO_YCBCR_CR( r3, g3, b3 ) + 128 );
			dst[numSamples + 0] = y0;
			dst[numSamples + 1] = y1;
			dst[numSamples + 2] = y2;
			dst[numSamples + 3] = y3;
			dst[numSamples + 4] = ( cb0 + cb1 + cb2 + cb3 ) >> 2;
			dst[numSamples + 5] = ( cr0 + cr1 + cr2 + cr3 ) >> 2;
			numSamples += 6;
		}
		numSamples += width;
	}
}

/*
========================
idColorSpace::ConvertYCbCr420ToRGB
========================
*/
void idColorSpace::ConvertYCbCr420ToRGB( byte* dst, const byte* src, int width, int height )
{
	int numSamples = width * height * 2 - width;
	for( int j = height - 2; j >= 0; j -= 2 )
	{
		for( int i = width - 2; i >= 0; i -= 2 )
		{
			int y0 = src[numSamples - 6];
			int y1 = src[numSamples - 5];
			int y2 = src[numSamples - 4];
			int y3 = src[numSamples - 3];
			int co = src[numSamples - 2] - 128;
			int cg = src[numSamples - 1] - 128;
			numSamples -= 6;
			int r = CBCR_TO_R( co, cg );
			int g = CBCR_TO_G( co, cg );
			int b = CBCR_TO_B( co, cg );
			dst[( ( j + 0 )*width + i + 0 ) * 4 + 0] = CLAMP_BYTE( y0 + r );
			dst[( ( j + 0 )*width + i + 0 ) * 4 + 1] = CLAMP_BYTE( y0 + g );
			dst[( ( j + 0 )*width + i + 0 ) * 4 + 2] = CLAMP_BYTE( y0 + b );
			dst[( ( j + 0 )*width + i + 1 ) * 4 + 0] = CLAMP_BYTE( y1 + r );
			dst[( ( j + 0 )*width + i + 1 ) * 4 + 1] = CLAMP_BYTE( y1 + g );
			dst[( ( j + 0 )*width + i + 1 ) * 4 + 2] = CLAMP_BYTE( y1 + b );
			dst[( ( j + 1 )*width + i + 0 ) * 4 + 0] = CLAMP_BYTE( y2 + r );
			dst[( ( j + 1 )*width + i + 0 ) * 4 + 1] = CLAMP_BYTE( y2 + g );
			dst[( ( j + 1 )*width + i + 0 ) * 4 + 2] = CLAMP_BYTE( y2 + b );
			dst[( ( j + 1 )*width + i + 1 ) * 4 + 0] = CLAMP_BYTE( y3 + r );
			dst[( ( j + 1 )*width + i + 1 ) * 4 + 1] = CLAMP_BYTE( y3 + g );
			dst[( ( j + 1 )*width + i + 1 ) * 4 + 2] = CLAMP_BYTE( y3 + b );
		}
		numSamples -= width;
	}
}

/*
========================
idColorSpace::ConvertNormalMapToStereographicHeightMap

Converts a tangent space normal map to a height map.
The iterative algorithm is pretty crappy but it's reasonably fast and good enough for testing purposes.
The algorithm uses a stereographic projection of the normals to reduce the entropy and preserve
significantly more detail.

A better approach would be to solve the massive but rather sparse matrix system:

[ c(1,0)      c(1,1)      ...  c(1,w*h)    ] [ H(1,1) ]     [ Nx(1,1) ]
[ c(2,0)      c(2,1)      ...  c(2,w*h)    ] [ H(1,2) ]  =  [ Ny(1,1) ]
[ ...                                      ] [ ...    ]     [ ...     ]
[ ...                                      ] [ H(w,h) ]     [ Nx(w,h) ]
[ c(w*h*2,0)  c(w*h*2,1)  ...  c(w*h*2,w*h)]                [ Ny(w,h) ]

Where: w = width, h = height, H(i,j) = height, Nx(i,j) = (normal.x/(1+normal.z), Ny(i,j) = (normal.y/(1+normal.z)
The c(i,j) are setup such that:

Nx(i,j) = H(i,j) - H(i,j+1)
Ny(i,j) = H(i,j) - H(i+1,j)
Nx(i,w) = H(i,w)
Ny(h,j) = H(h,j)

========================
*/
void idColorSpace::ConvertNormalMapToStereographicHeightMap( byte* heightMap, const byte* normalMap, int width, int height, float& scale )
{

	idTempArray<float> buffer( ( width + 1 ) * ( height + 1 ) * sizeof( float ) );
	float* temp = ( float* )buffer.Ptr();
	memset( temp, 0, ( width + 1 ) * ( height + 1 ) * sizeof( float ) );

	const int NUM_ITERATIONS = 32;

	float scale0 = 0.1f;
	float scale1 = 0.9f;

	for( int n = 0; n < NUM_ITERATIONS; n++ )
	{
		for( int i = 0; i < height; i++ )
		{
			for( int j = 1; j < width; j++ )
			{
				float x = NORMALMAP_BYTE_TO_FLOAT( normalMap[( ( i + 0 ) * width + ( j + 0 ) ) * 4 + 0] );
				float z = NORMALMAP_BYTE_TO_FLOAT( normalMap[( ( i + 0 ) * width + ( j + 0 ) ) * 4 + 2] );
				temp[i * width + j] = scale0 * temp[i * width + j] + scale1 * ( temp[( i + 0 ) * width + ( j - 1 )] - ( x / ( 1 + z ) ) );
			}
		}
		for( int i = 1; i < height; i++ )
		{
			for( int j = 0; j < width; j++ )
			{
				float y = NORMALMAP_BYTE_TO_FLOAT( normalMap[( ( i + 0 ) * width + ( j + 0 ) ) * 4 + 1] );
				float z = NORMALMAP_BYTE_TO_FLOAT( normalMap[( ( i + 0 ) * width + ( j + 0 ) ) * 4 + 2] );
				temp[i * width + j] = scale0 * temp[i * width + j] + scale1 * ( temp[( i - 1 ) * width + ( j + 0 )] - ( y / ( 1 + z ) ) );
			}
		}
		for( int i = 0; i < height; i++ )
		{
			for( int j = width - 1; j > 0; j-- )
			{
				float x = NORMALMAP_BYTE_TO_FLOAT( normalMap[( ( i + 0 ) * width + ( j - 1 ) ) * 4 + 0] );
				float z = NORMALMAP_BYTE_TO_FLOAT( normalMap[( ( i + 0 ) * width + ( j - 1 ) ) * 4 + 2] );
				temp[i * width + ( j - 1 )] = scale0 * temp[i * width + ( j - 1 )] + scale1 * ( temp[( i + 0 ) * width + ( j + 0 )] + ( x / ( 1 + z ) ) );
			}
		}
		for( int i = height - 1; i > 0; i-- )
		{
			for( int j = 0; j < width; j++ )
			{
				float y = NORMALMAP_BYTE_TO_FLOAT( normalMap[( ( i - 1 ) * width + ( j + 0 ) ) * 4 + 1] );
				float z = NORMALMAP_BYTE_TO_FLOAT( normalMap[( ( i - 1 ) * width + ( j + 0 ) ) * 4 + 2] );
				temp[( i - 1 ) * width + j] = scale0 * temp[( i - 1 ) * width + j] + scale1 * ( temp[( i + 0 ) * width + ( j + 0 )] + ( y / ( 1 + z ) ) );
			}
		}

		scale1 *= 0.99f;
		scale0 = 1.0f - scale1;
	}

	float minHeight = idMath::INFINITUM;
	float maxHeight = -idMath::INFINITUM;
	for( int j = 0; j < height; j++ )
	{
		for( int i = 0; i < width; i++ )
		{
			if( temp[j * width + i] < minHeight )
			{
				minHeight = temp[j * width + i];
			}
			if( temp[j * width + i] > maxHeight )
			{
				maxHeight = temp[j * width + i];
			}
		}
	}

	scale = ( maxHeight - minHeight );

	float s = 255.0f / scale;
	for( int j = 0; j < height; j++ )
	{
		for( int i = 0; i < width; i++ )
		{
			heightMap[j * width + i] = idMath::Ftob( ( temp[j * width + i] - minHeight ) * s );
		}
	}
}

/*
========================
idColorSpace::ConvertStereographicHeightMapToNormalMap

This converts a heightmap of a stereographically projected normal map back into a regular normal map.
========================
*/
void idColorSpace::ConvertStereographicHeightMapToNormalMap( byte* normalMap, const byte* heightMap, int width, int height, float scale )
{
	for( int i = 0; i < height; i++ )
	{
		int previ = Max( i, 0 );
		int nexti = Min( i + 1, height - 1 );

		for( int j = 0; j < width; j++ )
		{
			int prevj = Max( j, 0 );
			int nextj = Min( j + 1, width - 1 );

			idVec3 normal;
			float pX = scale * ( heightMap[i * width + prevj] - heightMap[i * width + nextj] ) / 255.0f;
			float pY = scale * ( heightMap[previ * width + j] - heightMap[nexti * width + j] ) / 255.0f;
			float denom = 2.0f / ( 1.0f + pX * pX + pY * pY );
			normal.x = pX * denom;
			normal.y = pY * denom;
			normal.z = denom - 1.0f;

			normalMap[( i * width + j ) * 4 + 0 ] = NORMALMAP_FLOAT_TO_BYTE( normal[0] );
			normalMap[( i * width + j ) * 4 + 1 ] = NORMALMAP_FLOAT_TO_BYTE( normal[1] );
			normalMap[( i * width + j ) * 4 + 2 ] = NORMALMAP_FLOAT_TO_BYTE( normal[2] );
			normalMap[( i * width + j ) * 4 + 3 ] = 255;
		}
	}
}

/*
========================
idColorSpace::ConvertRGBToMonochrome
========================
*/
void idColorSpace::ConvertRGBToMonochrome( byte* mono, const byte* rgb, int width, int height )
{
	for( int i = 0; i < height; i++ )
	{
		for( int j = 0; j < width; j++ )
		{
			mono[i * width + j] = ( rgb[( i * width + j ) * 4 + 0] +
									rgb[( i * width + j ) * 4 + 1] +
									rgb[( i * width + j ) * 4 + 2] ) / 3;
		}
	}
}

/*
========================
idColorSpace::ConvertMonochromeToRGB
========================
*/
void idColorSpace::ConvertMonochromeToRGB( byte* rgb, const byte* mono, int width, int height )
{
	for( int i = 0; i < height; i++ )
	{
		for( int j = 0; j < width; j++ )
		{
			rgb[( i * width + j ) * 4 + 0] = mono[i * width + j];
			rgb[( i * width + j ) * 4 + 1] = mono[i * width + j];
			rgb[( i * width + j ) * 4 + 2] = mono[i * width + j];
		}
	}
}
