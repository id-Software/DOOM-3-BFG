/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2014-2016 Robert Beckebans
Copyright (C) 2014-2016 Kot in Action Creative Artel

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

#if defined(USE_INTRINSICS_SSE)

//#define TEST_COMPRESSION
#ifdef TEST_COMPRESSION
	#include <malloc.h>
#endif

#define INSET_COLOR_SHIFT		4		// inset the bounding box with ( range >> shift )
#define INSET_ALPHA_SHIFT		5		// inset alpha channel

#define C565_5_MASK				0xF8	// 0xFF minus last three bits
#define C565_6_MASK				0xFC	// 0xFF minus last two bits

#define NVIDIA_7X_HARDWARE_BUG_FIX		// keep the DXT5 colors sorted as: max, min

#if !defined( R_SHUFFLE_D )
	#define R_SHUFFLE_D( x, y, z, w )	(( (w) & 3 ) << 6 | ( (z) & 3 ) << 4 | ( (y) & 3 ) << 2 | ( (x) & 3 ))
#endif

typedef uint16	word;
typedef uint32	dword;

ALIGN16( static __m128i SIMD_SSE2_zero ) = _mm_set_epi32( 0, 0, 0, 0 );
ALIGN16( static dword SIMD_SSE2_dword_byte_mask[4] ) = { 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF };
ALIGN16( static dword SIMD_SSE2_dword_word_mask[4] ) = { 0x0000FFFF, 0x0000FFFF, 0x0000FFFF, 0x0000FFFF };
ALIGN16( static dword SIMD_SSE2_dword_red_mask[4] )   = { 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF };
ALIGN16( static dword SIMD_SSE2_dword_green_mask[4] ) = { 0x0000FF00, 0x0000FF00, 0x0000FF00, 0x0000FF00 };
ALIGN16( static dword SIMD_SSE2_dword_blue_mask[4] )  = { 0x00FF0000, 0x00FF0000, 0x00FF0000, 0x00FF0000 };
ALIGN16( static dword SIMD_SSE2_dword_colorMask_1010[4] ) = { 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000 };
ALIGN16( static dword SIMD_SSE2_dword_colorMask_0100[4] ) = { 0x00000000, 0xFFFFFFFF, 0x00000000, 0x00000000 };
ALIGN16( static dword SIMD_SSE2_dword_alpha_bit_mask0[4] ) = { 7 << 0, 0, 7 << 0, 0 };
ALIGN16( static dword SIMD_SSE2_dword_alpha_bit_mask1[4] ) = { 7 << 3, 0, 7 << 3, 0 };
ALIGN16( static dword SIMD_SSE2_dword_alpha_bit_mask2[4] ) = { 7 << 6, 0, 7 << 6, 0 };
ALIGN16( static dword SIMD_SSE2_dword_alpha_bit_mask3[4] ) = { 7 << 9, 0, 7 << 9, 0 };
ALIGN16( static dword SIMD_SSE2_dword_alpha_bit_mask4[4] ) = { 7 << 12, 0, 7 << 12, 0 };
ALIGN16( static dword SIMD_SSE2_dword_alpha_bit_mask5[4] ) = { 7 << 15, 0, 7 << 15, 0 };
ALIGN16( static dword SIMD_SSE2_dword_alpha_bit_mask6[4] ) = { 7 << 18, 0, 7 << 18, 0 };
ALIGN16( static dword SIMD_SSE2_dword_alpha_bit_mask7[4] ) = { 7 << 21, 0, 7 << 21, 0 };
ALIGN16( static dword SIMD_SSE2_dword_color_bit_mask0[4] ) = { 3 << 0, 0, 3 << 0, 0 };
ALIGN16( static dword SIMD_SSE2_dword_color_bit_mask1[4] ) = { 3 << 2, 0, 3 << 2, 0 };
ALIGN16( static dword SIMD_SSE2_dword_color_bit_mask2[4] ) = { 3 << 4, 0, 3 << 4, 0 };
ALIGN16( static dword SIMD_SSE2_dword_color_bit_mask3[4] ) = { 3 << 6, 0, 3 << 6, 0 };
ALIGN16( static dword SIMD_SSE2_dword_color_bit_mask4[4] ) = { 3 << 8, 0, 3 << 8, 0 };
ALIGN16( static dword SIMD_SSE2_dword_color_bit_mask5[4] ) = { 3 << 10, 0, 3 << 10, 0 };
ALIGN16( static dword SIMD_SSE2_dword_color_bit_mask6[4] ) = { 3 << 12, 0, 3 << 12, 0 };
ALIGN16( static dword SIMD_SSE2_dword_color_bit_mask7[4] ) = { 3 << 14, 0, 3 << 14, 0 };
ALIGN16( static word SIMD_SSE2_word_0[8] ) = { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 };
ALIGN16( static word SIMD_SSE2_word_1[8] ) = { 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001 };
ALIGN16( static word SIMD_SSE2_word_2[8] ) = { 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002 };
ALIGN16( static word SIMD_SSE2_word_3[8] ) = { 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003 };
ALIGN16( static word SIMD_SSE2_word_7[8] ) = { 0x0007, 0x0007, 0x0007, 0x0007, 0x0007, 0x0007, 0x0007, 0x0007 };
ALIGN16( static word SIMD_SSE2_word_8[8] ) = { 0x0008, 0x0008, 0x0008, 0x0008, 0x0008, 0x0008, 0x0008, 0x0008 };
ALIGN16( static word SIMD_SSE2_word_31[8] ) = { 31, 31, 31, 31, 31, 31, 31, 31 };
ALIGN16( static word SIMD_SSE2_word_63[8] ) = { 63, 63, 63, 63, 63, 63, 63, 63 };
ALIGN16( static word SIMD_SSE2_word_127[8] ) = { 127, 127, 127, 127, 127, 127, 127, 127 };
ALIGN16( static word SIMD_SSE2_word_255[8] ) = { 255, 255, 255, 255, 255, 255, 255, 255 };
ALIGN16( static word SIMD_SSE2_word_center_128[8] ) = { 128, 128, 0, 0, 0, 0, 0, 0 };
ALIGN16( static word SIMD_SSE2_word_div_by_3[8] ) = { ( 1 << 16 ) / 3 + 1, ( 1 << 16 ) / 3 + 1, ( 1 << 16 ) / 3 + 1, ( 1 << 16 ) / 3 + 1, ( 1 << 16 ) / 3 + 1, ( 1 << 16 ) / 3 + 1, ( 1 << 16 ) / 3 + 1, ( 1 << 16 ) / 3 + 1 };
ALIGN16( static word SIMD_SSE2_word_div_by_6[8] ) = { ( 1 << 16 ) / 6 + 1, ( 1 << 16 ) / 6 + 1, ( 1 << 16 ) / 6 + 1, ( 1 << 16 ) / 6 + 1, ( 1 << 16 ) / 6 + 1, ( 1 << 16 ) / 6 + 1, ( 1 << 16 ) / 6 + 1, ( 1 << 16 ) / 6 + 1 };
ALIGN16( static word SIMD_SSE2_word_div_by_14[8] ) = { ( 1 << 16 ) / 14 + 1, ( 1 << 16 ) / 14 + 1, ( 1 << 16 ) / 14 + 1, ( 1 << 16 ) / 14 + 1, ( 1 << 16 ) / 14 + 1, ( 1 << 16 ) / 14 + 1, ( 1 << 16 ) / 14 + 1, ( 1 << 16 ) / 14 + 1 };
ALIGN16( static word SIMD_SSE2_word_scale_7_9_11_13[8] ) = { 7, 7, 9, 9, 11, 11, 13, 13 };
ALIGN16( static word SIMD_SSE2_word_scale_7_5_3_1[8] ) = { 7, 7, 5, 5, 3, 3, 1, 1 };
ALIGN16( static word SIMD_SSE2_word_scale_5_3_1[8] ) = { 5, 3, 1, 0, 5, 3, 1, 0 };
ALIGN16( static word SIMD_SSE2_word_scale_1_3_5[8] ) = { 1, 3, 5, 0, 1, 3, 5, 0 };
ALIGN16( static word SIMD_SSE2_word_insetShift[8] ) = { 1 << ( 16 - INSET_COLOR_SHIFT ), 1 << ( 16 - INSET_COLOR_SHIFT ), 1 << ( 16 - INSET_COLOR_SHIFT ), 1 << ( 16 - INSET_ALPHA_SHIFT ), 0, 0, 0, 0 };
ALIGN16( static word SIMD_SSE2_word_insetYCoCgRound[8] ) = { ( ( 1 << ( INSET_COLOR_SHIFT - 1 ) ) - 1 ), ( ( 1 << ( INSET_COLOR_SHIFT - 1 ) ) - 1 ), ( ( 1 << ( INSET_COLOR_SHIFT - 1 ) ) - 1 ), ( ( 1 << ( INSET_ALPHA_SHIFT - 1 ) ) - 1 ), 0, 0, 0, 0 };
ALIGN16( static word SIMD_SSE2_word_insetYCoCgMask[8] ) = { 0xFFFF, 0xFFFF, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0xFFFF };
ALIGN16( static word SIMD_SSE2_word_insetYCoCgShiftUp[8] ) = { 1 << INSET_COLOR_SHIFT, 1 << INSET_COLOR_SHIFT, 1 << INSET_COLOR_SHIFT, 1 << INSET_ALPHA_SHIFT, 0, 0, 0, 0 };
ALIGN16( static word SIMD_SSE2_word_insetYCoCgShiftDown[8] ) = { 1 << ( 16 - INSET_COLOR_SHIFT ), 1 << ( 16 - INSET_COLOR_SHIFT ), 1 << ( 16 - INSET_COLOR_SHIFT ), 1 << ( 16 - INSET_ALPHA_SHIFT ), 0, 0, 0, 0 };
ALIGN16( static word SIMD_SSE2_word_insetYCoCgQuantMask[8] ) = { C565_5_MASK, C565_6_MASK, C565_5_MASK, 0xFF, C565_5_MASK, C565_6_MASK, C565_5_MASK, 0xFF };
ALIGN16( static word SIMD_SSE2_word_insetYCoCgRep[8] ) = { 1 << ( 16 - 5 ), 1 << ( 16 - 6 ), 1 << ( 16 - 5 ), 0, 1 << ( 16 - 5 ), 1 << ( 16 - 6 ), 1 << ( 16 - 5 ), 0 };
ALIGN16( static word SIMD_SSE2_word_insetNormalDXT5Round[8] ) = { 0, ( ( 1 << ( INSET_COLOR_SHIFT - 1 ) ) - 1 ), 0, ( ( 1 << ( INSET_ALPHA_SHIFT - 1 ) ) - 1 ), 0, 0, 0, 0 };
ALIGN16( static word SIMD_SSE2_word_insetNormalDXT5Mask[8] ) = { 0x0000, 0xFFFF, 0x0000, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000 };
ALIGN16( static word SIMD_SSE2_word_insetNormalDXT5ShiftUp[8] ) = { 1, 1 << INSET_COLOR_SHIFT, 1, 1 << INSET_ALPHA_SHIFT, 1, 1, 1, 1 };
ALIGN16( static word SIMD_SSE2_word_insetNormalDXT5ShiftDown[8] ) = { 0, 1 << ( 16 - INSET_COLOR_SHIFT ), 0, 1 << ( 16 - INSET_ALPHA_SHIFT ), 0, 0, 0, 0 };
ALIGN16( static word SIMD_SSE2_word_insetNormalDXT5QuantMask[8] ) = { 0xFF, C565_6_MASK, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
ALIGN16( static word SIMD_SSE2_word_insetNormalDXT5Rep[8] ) = { 0, 1 << ( 16 - 6 ), 0, 0, 0, 0, 0, 0 };
ALIGN16( static word SIMD_SSE2_word_insetNormal3DcRound[8] ) = { ( ( 1 << ( INSET_ALPHA_SHIFT - 1 ) ) - 1 ), ( ( 1 << ( INSET_ALPHA_SHIFT - 1 ) ) - 1 ), 0, 0, 0, 0, 0, 0 };
ALIGN16( static word SIMD_SSE2_word_insetNormal3DcMask[8] ) = { 0xFFFF, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 };
ALIGN16( static word SIMD_SSE2_word_insetNormal3DcShiftUp[8] ) = { 1 << INSET_ALPHA_SHIFT, 1 << INSET_ALPHA_SHIFT, 1, 1, 1, 1, 1, 1 };
ALIGN16( static word SIMD_SSE2_word_insetNormal3DcShiftDown[8] ) = { 1 << ( 16 - INSET_ALPHA_SHIFT ), 1 << ( 16 - INSET_ALPHA_SHIFT ), 0, 0, 0, 0, 0, 0 };
ALIGN16( static byte SIMD_SSE2_byte_0[16] ) = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
ALIGN16( static byte SIMD_SSE2_byte_1[16] ) = { 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 };
ALIGN16( static byte SIMD_SSE2_byte_2[16] ) = { 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02 };
ALIGN16( static byte SIMD_SSE2_byte_3[16] ) = { 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03 };
ALIGN16( static byte SIMD_SSE2_byte_4[16] ) = { 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04 };
ALIGN16( static byte SIMD_SSE2_byte_7[16] ) = { 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07 };
ALIGN16( static byte SIMD_SSE2_byte_8[16] ) = { 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08 };
ALIGN16( static byte SIMD_SSE2_byte_not[16] ) = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
ALIGN16( static byte SIMD_SSE2_byte_colorMask[16] ) = { C565_5_MASK, C565_6_MASK, C565_5_MASK, 0x00, 0x00, 0x00, 0x00, 0x00, C565_5_MASK, C565_6_MASK, C565_5_MASK, 0x00, 0x00, 0x00, 0x00, 0x00 };
ALIGN16( static byte SIMD_SSE2_byte_colorMask2[16] ) = { 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00 };
ALIGN16( static byte SIMD_SSE2_byte_ctx1Mask[16] ) = { 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
ALIGN16( static byte SIMD_SSE2_byte_diagonalMask[16] ) = { 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
ALIGN16( static byte SIMD_SSE2_byte_scale_mask0[16] ) = { 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF };
ALIGN16( static byte SIMD_SSE2_byte_scale_mask1[16] ) = { 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00 };
ALIGN16( static byte SIMD_SSE2_byte_scale_mask2[16] ) = { 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00 };
ALIGN16( static byte SIMD_SSE2_byte_scale_mask3[16] ) = { 0xFF, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00 };
ALIGN16( static byte SIMD_SSE2_byte_scale_mask4[16] ) = { 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00 };
ALIGN16( static byte SIMD_SSE2_byte_minus_128_0[16] ) = { ( byte ) - 128, ( byte ) - 128, 0, 0, ( byte ) - 128, ( byte ) - 128, 0, 0, ( byte ) - 128, ( byte ) - 128, 0, 0, ( byte ) - 128, ( byte ) - 128, 0, 0 };

/*
========================
idDxtEncoder::ExtractBlock_SSE2

params:	inPtr		- input image, 4 bytes per pixel
paramO:	colorBlock	- 4*4 output tile, 4 bytes per pixel
========================
*/
ID_INLINE void idDxtEncoder::ExtractBlock_SSE2( const byte* inPtr, int width, byte* colorBlock ) const
{
	*( ( __m128i* )( &colorBlock[ 0] ) ) = _mm_load_si128( ( __m128i* )( inPtr + width * 4 * 0 ) );
	*( ( __m128i* )( &colorBlock[16] ) ) = _mm_load_si128( ( __m128i* )( inPtr + width * 4 * 1 ) );
	*( ( __m128i* )( &colorBlock[32] ) ) = _mm_load_si128( ( __m128i* )( inPtr + width * 4 * 2 ) );
	*( ( __m128i* )( &colorBlock[48] ) ) = _mm_load_si128( ( __m128i* )( inPtr + width * 4 * 3 ) );
}

/*
========================
idDxtEncoder::GetMinMaxBBox_SSE2

Takes the extents of the bounding box of the colors in the 4x4 block.

params:	colorBlock	- 4*4 input tile, 4 bytes per pixel
paramO:	minColor	- Min 4 byte output color
paramO:	maxColor	- Max 4 byte output color
========================
*/
ID_INLINE void idDxtEncoder::GetMinMaxBBox_SSE2( const byte* colorBlock, byte* minColor, byte* maxColor ) const
{
	__m128i block0 = *( ( __m128i* )( &colorBlock[ 0] ) );
	__m128i block1 = *( ( __m128i* )( &colorBlock[16] ) );
	__m128i block2 = *( ( __m128i* )( &colorBlock[32] ) );
	__m128i block3 = *( ( __m128i* )( &colorBlock[48] ) );

	__m128i max1 = _mm_max_epu8( block0, block1 );
	__m128i min1 = _mm_min_epu8( block0, block1 );
	__m128i max2 = _mm_max_epu8( block2, block3 );
	__m128i min2 = _mm_min_epu8( block2, block3 );

	__m128i max3 = _mm_max_epu8( max1, max2 );
	__m128i min3 = _mm_min_epu8( min1, min2 );

	__m128i max4 = _mm_shuffle_epi32( max3, R_SHUFFLE_D( 2, 3, 2, 3 ) );
	__m128i min4 = _mm_shuffle_epi32( min3, R_SHUFFLE_D( 2, 3, 2, 3 ) );

	__m128i max5 = _mm_max_epu8( max3, max4 );
	__m128i min5 = _mm_min_epu8( min3, min4 );

	__m128i max6 = _mm_shufflelo_epi16( max5, R_SHUFFLE_D( 2, 3, 2, 3 ) );
	__m128i min6 = _mm_shufflelo_epi16( min5, R_SHUFFLE_D( 2, 3, 2, 3 ) );

	max6 = _mm_max_epu8( max5, max6 );
	min6 = _mm_min_epu8( min5, min6 );

	*( ( int* )maxColor ) = _mm_cvtsi128_si32( max6 );
	*( ( int* )minColor ) = _mm_cvtsi128_si32( min6 );
}

/*
========================
idDxtEncoder::InsetColorsBBox_SSE2
========================
*/
ID_INLINE void idDxtEncoder::InsetColorsBBox_SSE2( byte* minColor, byte* maxColor ) const
{
	__m128i min = _mm_cvtsi32_si128( *( int* )minColor );
	__m128i max = _mm_cvtsi32_si128( *( int* )maxColor );

	__m128i xmm0 = _mm_unpacklo_epi8( min, *( __m128i* )SIMD_SSE2_byte_0 );
	__m128i xmm1 = _mm_unpacklo_epi8( max, *( __m128i* )SIMD_SSE2_byte_0 );

	__m128i xmm2 = _mm_sub_epi16( xmm1, xmm0 );

	xmm2 = _mm_mulhi_epi16( xmm2, *( __m128i* )SIMD_SSE2_word_insetShift );

	xmm0 = _mm_add_epi16( xmm0, xmm2 );
	xmm1 = _mm_sub_epi16( xmm1, xmm2 );

	xmm0 = _mm_packus_epi16( xmm0, xmm0 );
	xmm1 = _mm_packus_epi16( xmm1, xmm1 );

	*( ( int* )minColor ) = _mm_cvtsi128_si32( xmm0 );
	*( ( int* )maxColor ) = _mm_cvtsi128_si32( xmm1 );
}

/*
========================
idDxtEncoder::EmitColorIndices_SSE2

params:	colorBlock	- 16 pixel block for which to find color indices
paramO:	minColor	- Min alpha found
paramO:	maxColor	- Max alpha found
return: 4 byte color index block
========================
*/
void idDxtEncoder::EmitColorIndices_SSE2( const byte* colorBlock, const byte* minColor_, const byte* maxColor_ )
{
	__m128c zero = SIMD_SSE2_zero;
	__m128c result = SIMD_SSE2_zero;
	__m128c color0, color1, color2, color3;
	__m128c temp0, temp1, temp2, temp3, temp4, temp5, temp6, temp7;
	__m128c minColor = _mm_cvtsi32_si128( *( int* )minColor_ );
	__m128c maxColor = _mm_cvtsi32_si128( *( int* )maxColor_ );
	__m128c blocka[2], blockb[2];
	blocka[0] = *( ( __m128i* )( &colorBlock[ 0] ) );
	blocka[1] = *( ( __m128i* )( &colorBlock[32] ) );
	blockb[0] = *( ( __m128i* )( &colorBlock[16] ) );
	blockb[1] = *( ( __m128i* )( &colorBlock[48] ) );

	temp0 = _mm_and_si128( maxColor, ( const __m128i& )SIMD_SSE2_byte_colorMask );
	temp0 = _mm_unpacklo_epi8( temp0, zero );
	temp4 = _mm_shufflelo_epi16( temp0, R_SHUFFLE_D( 0, 3, 2, 3 ) );
	temp5 = _mm_shufflelo_epi16( temp0, R_SHUFFLE_D( 3, 1, 3, 3 ) );
	temp4 = _mm_srli_epi16( temp4, 5 );
	temp5 = _mm_srli_epi16( temp5, 6 );
	temp0 = _mm_or_si128( temp0, temp4 );
	temp0 = _mm_or_si128( temp0, temp5 );


	temp1 = _mm_and_si128( minColor, ( const __m128i& )SIMD_SSE2_byte_colorMask );
	temp1 = _mm_unpacklo_epi8( temp1, zero );
	temp4 = _mm_shufflelo_epi16( temp1, R_SHUFFLE_D( 0, 3, 2, 3 ) );
	temp5 = _mm_shufflelo_epi16( temp1, R_SHUFFLE_D( 3, 1, 3, 3 ) );
	temp4 = _mm_srli_epi16( temp4, 5 );
	temp5 = _mm_srli_epi16( temp5, 6 );
	temp1 = _mm_or_si128( temp1, temp4 );
	temp1 = _mm_or_si128( temp1, temp5 );


	temp2 = _mm_packus_epi16( temp0, zero );
	color0 = _mm_shuffle_epi32( temp2, R_SHUFFLE_D( 0, 1, 0, 1 ) );

	temp6 = _mm_add_epi16( temp0, temp0 );
	temp6 = _mm_add_epi16( temp6, temp1 );
	temp6 = _mm_mulhi_epi16( temp6, ( const __m128i& )SIMD_SSE2_word_div_by_3 );		// * ( ( 1 << 16 ) / 3 + 1 ) ) >> 16
	temp6 = _mm_packus_epi16( temp6, zero );
	color2 = _mm_shuffle_epi32( temp6, R_SHUFFLE_D( 0, 1, 0, 1 ) );

	temp3 = _mm_packus_epi16( temp1, zero );
	color1 = _mm_shuffle_epi32( temp3, R_SHUFFLE_D( 0, 1, 0, 1 ) );

	temp1 = _mm_add_epi16( temp1, temp1 );
	temp0 = _mm_add_epi16( temp0, temp1 );
	temp0 = _mm_mulhi_epi16( temp0, ( const __m128i& )SIMD_SSE2_word_div_by_3 );		// * ( ( 1 << 16 ) / 3 + 1 ) ) >> 16
	temp0 = _mm_packus_epi16( temp0, zero );
	color3 = _mm_shuffle_epi32( temp0, R_SHUFFLE_D( 0, 1, 0, 1 ) );

	for( int i = 1; i >= 0; i-- )
	{
		// Load block
		temp3 = _mm_shuffle_epi32( blocka[i], R_SHUFFLE_D( 0, 2, 1, 3 ) );
		temp5 = _mm_shuffle_ps( blocka[i], zero, R_SHUFFLE_D( 2, 3, 0, 1 ) );
		temp5 = _mm_shuffle_epi32( temp5, R_SHUFFLE_D( 0, 2, 1, 3 ) );

		temp0 = _mm_sad_epu8( temp3, color0 );
		temp6 = _mm_sad_epu8( temp5, color0 );
		temp0 = _mm_packs_epi32( temp0, temp6 );

		temp1 = _mm_sad_epu8( temp3, color1 );
		temp6 = _mm_sad_epu8( temp5, color1 );
		temp1 = _mm_packs_epi32( temp1, temp6 );

		temp2 = _mm_sad_epu8( temp3, color2 );
		temp6 = _mm_sad_epu8( temp5, color2 );
		temp2 = _mm_packs_epi32( temp2, temp6 );

		temp3 = _mm_sad_epu8( temp3, color3 );
		temp5 = _mm_sad_epu8( temp5, color3 );
		temp3 = _mm_packs_epi32( temp3, temp5 );

		// Load block
		temp4 = _mm_shuffle_epi32( blockb[i], R_SHUFFLE_D( 0, 2, 1, 3 ) );
		temp5 = _mm_shuffle_ps( blockb[i], zero, R_SHUFFLE_D( 2, 3, 0, 1 ) );
		temp5 = _mm_shuffle_epi32( temp5, R_SHUFFLE_D( 0, 2, 1, 3 ) );

		temp6 = _mm_sad_epu8( temp4, color0 );
		temp7 = _mm_sad_epu8( temp5, color0 );
		temp6 = _mm_packs_epi32( temp6, temp7 );
		temp0 = _mm_packs_epi32( temp0, temp6 );	// d0

		temp6 = _mm_sad_epu8( temp4, color1 );
		temp7 = _mm_sad_epu8( temp5, color1 );
		temp6 = _mm_packs_epi32( temp6, temp7 );
		temp1 = _mm_packs_epi32( temp1, temp6 );	// d1

		temp6 = _mm_sad_epu8( temp4, color2 );
		temp7 = _mm_sad_epu8( temp5, color2 );
		temp6 = _mm_packs_epi32( temp6, temp7 );
		temp2 = _mm_packs_epi32( temp2, temp6 );	// d2

		temp4 = _mm_sad_epu8( temp4, color3 );
		temp5 = _mm_sad_epu8( temp5, color3 );
		temp4 = _mm_packs_epi32( temp4, temp5 );
		temp3 = _mm_packs_epi32( temp3, temp4 );	// d3

		temp7 = _mm_slli_epi32( result, 16 );

		temp4 = _mm_cmpgt_epi16( temp0, temp2 );	// b2
		temp5 = _mm_cmpgt_epi16( temp1, temp3 );	// b3
		temp0 = _mm_cmpgt_epi16( temp0, temp3 );	// b0
		temp1 = _mm_cmpgt_epi16( temp1, temp2 );	// b1
		temp2 = _mm_cmpgt_epi16( temp2, temp3 );	// b4

		temp4 = _mm_and_si128( temp4, temp1 );		// x0
		temp5 = _mm_and_si128( temp5, temp0 );		// x1
		temp2 = _mm_and_si128( temp2, temp0 );		// x2
		temp4 = _mm_or_si128( temp4, temp5 );
		temp2 = _mm_and_si128( temp2, ( const __m128i& )SIMD_SSE2_word_1 );
		temp4 = _mm_and_si128( temp4, ( const __m128i& )SIMD_SSE2_word_2 );
		temp2 = _mm_or_si128( temp2, temp4 );

		temp5 = _mm_shuffle_epi32( temp2, R_SHUFFLE_D( 2, 3, 0, 1 ) );
		temp2 = _mm_unpacklo_epi16( temp2, ( const __m128i& )SIMD_SSE2_word_0 );
		temp5 = _mm_unpacklo_epi16( temp5, ( const __m128i& )SIMD_SSE2_word_0 );
		temp5 = _mm_slli_epi32( temp5, 8 );
		temp7 = _mm_or_si128( temp7, temp5 );
		result = _mm_or_si128( temp7, temp2 );
	}

	temp4 = _mm_shuffle_epi32( result, R_SHUFFLE_D( 1, 2, 3, 0 ) );
	temp5 = _mm_shuffle_epi32( result, R_SHUFFLE_D( 2, 3, 0, 1 ) );
	temp6 = _mm_shuffle_epi32( result, R_SHUFFLE_D( 3, 0, 1, 2 ) );
	temp4 = _mm_slli_epi32( temp4, 2 );
	temp5 = _mm_slli_epi32( temp5, 4 );
	temp6 = _mm_slli_epi32( temp6, 6 );
	temp7 = _mm_or_si128( result, temp4 );
	temp7 = _mm_or_si128( temp7, temp5 );
	temp7 = _mm_or_si128( temp7, temp6 );

	unsigned int out = _mm_cvtsi128_si32( temp7 );
	EmitUInt( out );
}

/*
========================
idDxtEncoder::EmitColorAlphaIndices_SSE2

params:	colorBlock	- 16 pixel block for which find color indexes
paramO:	minColor	- Min color found
paramO:	maxColor	- Max color found
return: 4 byte color index block
========================
*/
void idDxtEncoder::EmitColorAlphaIndices_SSE2( const byte* colorBlock, const byte* minColor_, const byte* maxColor_ )
{
	__m128c zero = SIMD_SSE2_zero;
	__m128c result = SIMD_SSE2_zero;
	__m128c color0, color1, color2;
	__m128c temp0, temp1, temp2, temp3, temp4, temp5, temp6, temp7;
	__m128c minColor = _mm_cvtsi32_si128( *( int* )minColor_ );
	__m128c maxColor = _mm_cvtsi32_si128( *( int* )maxColor_ );
	__m128c blocka[2], blockb[2];
	blocka[0] = *( ( __m128i* )( &colorBlock[ 0] ) );
	blocka[1] = *( ( __m128i* )( &colorBlock[32] ) );
	blockb[0] = *( ( __m128i* )( &colorBlock[16] ) );
	blockb[1] = *( ( __m128i* )( &colorBlock[48] ) );

	temp0 = _mm_and_si128( maxColor, *( __m128c* )SIMD_SSE2_byte_colorMask );
	temp0 = _mm_unpacklo_epi8( temp0, zero );
	temp4 = _mm_shufflelo_epi16( temp0, R_SHUFFLE_D( 0, 3, 2, 3 ) );
	temp5 = _mm_shufflelo_epi16( temp0, R_SHUFFLE_D( 3, 1, 3, 3 ) );
	temp4 = _mm_srli_epi16( temp4, 5 );
	temp5 = _mm_srli_epi16( temp5, 6 );
	temp0 = _mm_or_si128( temp0, temp4 );
	temp0 = _mm_or_si128( temp0, temp5 );

	temp1 = _mm_and_si128( minColor, *( __m128c* )SIMD_SSE2_byte_colorMask );
	temp1 = _mm_unpacklo_epi8( temp1, zero );
	temp4 = _mm_shufflelo_epi16( temp1, R_SHUFFLE_D( 0, 3, 2, 3 ) );
	temp5 = _mm_shufflelo_epi16( temp1, R_SHUFFLE_D( 3, 1, 3, 3 ) );
	temp4 = _mm_srli_epi16( temp4, 5 );
	temp5 = _mm_srli_epi16( temp5, 6 );
	temp1 = _mm_or_si128( temp1, temp4 );
	temp1 = _mm_or_si128( temp1, temp5 );

	temp2 = _mm_packus_epi16( temp0, zero );
	color0 = _mm_shuffle_epi32( temp2, R_SHUFFLE_D( 0, 1, 0, 1 ) );

	temp6 = _mm_add_epi16( temp0, temp0 );
	temp6 = _mm_srli_epi16( temp6, 1 );			// diff from color
	temp6 = _mm_packus_epi16( temp6, zero );
	color2 = _mm_shuffle_epi32( temp6, R_SHUFFLE_D( 0, 1, 0, 1 ) );

	temp3 = _mm_packus_epi16( temp1, zero );
	color1 = _mm_shuffle_epi32( temp3, R_SHUFFLE_D( 0, 1, 0, 1 ) );

	// not used
	//color3 = zero;

	for( int i = 1; i >= 0; i-- )
	{
		// Load block
		temp3 = _mm_shuffle_epi32( blocka[i], R_SHUFFLE_D( 0, 2, 1, 3 ) );
		temp5 = _mm_shuffle_ps( blocka[i], zero, R_SHUFFLE_D( 2, 3, 0, 1 ) );
		temp5 = _mm_shuffle_epi32( temp5, R_SHUFFLE_D( 0, 2, 1, 3 ) );

		temp0 = _mm_sad_epu8( temp3, color0 );
		temp6 = _mm_sad_epu8( temp5, color0 );
		temp0 = _mm_packs_epi32( temp0, temp6 );

		temp1 = _mm_sad_epu8( temp3, color1 );
		temp6 = _mm_sad_epu8( temp5, color1 );
		temp1 = _mm_packs_epi32( temp1, temp6 );

		temp2 = _mm_sad_epu8( temp3, color2 );
		temp6 = _mm_sad_epu8( temp5, color2 );
		temp2 = _mm_packs_epi32( temp2, temp6 );


		// diff from color
		temp3 = _mm_shuffle_ps( temp3, temp5, R_SHUFFLE_D( 0, 2, 0, 2 ) );
		temp3 = _mm_srli_epi32( temp3, 24 );
		temp3 = _mm_packs_epi32( temp3, temp3 );


		// Load block
		temp4 = _mm_shuffle_epi32( blockb[i], R_SHUFFLE_D( 0, 2, 1, 3 ) );
		temp5 = _mm_shuffle_ps( blockb[i], zero, R_SHUFFLE_D( 2, 3, 0, 1 ) );
		temp5 = _mm_shuffle_epi32( temp5, R_SHUFFLE_D( 0, 2, 1, 3 ) );

		temp6 = _mm_sad_epu8( temp4, color0 );
		temp7 = _mm_sad_epu8( temp5, color0 );
		temp6 = _mm_packs_epi32( temp6, temp7 );
		temp0 = _mm_packs_epi32( temp0, temp6 );	// d0

		temp6 = _mm_sad_epu8( temp4, color1 );
		temp7 = _mm_sad_epu8( temp5, color1 );
		temp6 = _mm_packs_epi32( temp6, temp7 );
		temp1 = _mm_packs_epi32( temp1, temp6 );	// d1

		temp6 = _mm_sad_epu8( temp4, color2 );
		temp7 = _mm_sad_epu8( temp5, color2 );
		temp6 = _mm_packs_epi32( temp6, temp7 );
		temp2 = _mm_packs_epi32( temp2, temp6 );	// d2


		// diff from color
		temp4 = _mm_shuffle_ps( temp4, temp5, R_SHUFFLE_D( 0, 2, 0, 2 ) );		// c3
		temp4 = _mm_srli_epi32( temp4, 24 );
		temp4 = _mm_packs_epi32( temp4, temp4 );
		temp3 = _mm_unpacklo_epi64( temp3, temp4 );

		temp7 = _mm_slli_epi32( result, 16 );


		// diff from color
		temp4 = _mm_cmpgt_epi16( temp2, temp1 );						// b1
		temp2 = _mm_cmpgt_epi16( temp2, temp0 );						// b0
		temp1 = _mm_cmpgt_epi16( temp1, temp0 );						// b2
		temp3 = _mm_max_epi16( temp3, ( const __m128i& )SIMD_SSE2_word_127 );	// b3
		temp3 = _mm_cmpeq_epi16( temp3, ( const __m128i& )SIMD_SSE2_word_127 );

		temp2 = _mm_and_si128( temp2, temp4 );
		temp2 = _mm_or_si128( temp2, temp3 );							// b0 & b1 | b3
		temp1 = _mm_xor_si128( temp1, temp4 );
		temp1 = _mm_or_si128( temp1, temp3 );							// b2 ^ b1 | b3
		temp2 = _mm_and_si128( temp2, ( const __m128i& )SIMD_SSE2_word_2 );
		temp1 = _mm_and_si128( temp1, ( const __m128i& )SIMD_SSE2_word_1 );
		temp2 = _mm_or_si128( temp2, temp1 );



		temp5 = _mm_shuffle_epi32( temp2, R_SHUFFLE_D( 2, 3, 0, 1 ) );
		temp2 = _mm_unpacklo_epi16( temp2, ( const __m128i& )SIMD_SSE2_word_0 );
		temp5 = _mm_unpacklo_epi16( temp5, ( const __m128i& )SIMD_SSE2_word_0 );
		temp5 = _mm_slli_epi32( temp5, 8 );
		temp7 = _mm_or_si128( temp7, temp5 );
		result = _mm_or_si128( temp7, temp2 );
	}

	temp4 = _mm_shuffle_epi32( result, R_SHUFFLE_D( 1, 2, 3, 0 ) );
	temp5 = _mm_shuffle_epi32( result, R_SHUFFLE_D( 2, 3, 0, 1 ) );
	temp6 = _mm_shuffle_epi32( result, R_SHUFFLE_D( 3, 0, 1, 2 ) );
	temp4 = _mm_slli_epi32( temp4, 2 );
	temp5 = _mm_slli_epi32( temp5, 4 );
	temp6 = _mm_slli_epi32( temp6, 6 );
	temp7 = _mm_or_si128( result, temp4 );
	temp7 = _mm_or_si128( temp7, temp5 );
	temp7 = _mm_or_si128( temp7, temp6 );

	unsigned int out = _mm_cvtsi128_si32( temp7 );
	EmitUInt( out );
}

/*
========================
idDxtEncoder::EmitCoCgIndices_SSE2

params:	colorBlock	- 16 pixel block for which to find color indices
paramO:	minColor	- Min alpha found
paramO:	maxColor	- Max alpha found
return: 4 byte color index block
========================
*/
void idDxtEncoder::EmitCoCgIndices_SSE2( const byte* colorBlock, const byte* minColor_, const byte* maxColor_ )
{
	__m128c zero = SIMD_SSE2_zero;
	__m128c result = SIMD_SSE2_zero;
	__m128c color0, color1, color2, color3;
	__m128c temp0, temp1, temp2, temp3, temp4, temp5, temp6, temp7;
	__m128c minColor = _mm_cvtsi32_si128( *( int* )minColor_ );
	__m128c maxColor = _mm_cvtsi32_si128( *( int* )maxColor_ );
	__m128c blocka[2], blockb[2];
	blocka[0] = *( ( __m128i* )( &colorBlock[ 0] ) );
	blocka[1] = *( ( __m128i* )( &colorBlock[32] ) );
	blockb[0] = *( ( __m128i* )( &colorBlock[16] ) );
	blockb[1] = *( ( __m128i* )( &colorBlock[48] ) );

	temp7 = zero;

	temp0 = maxColor;
	temp0 = _mm_and_si128( temp0, *( __m128c* )SIMD_SSE2_byte_colorMask2 );
	color0 = _mm_shuffle_epi32( temp0, R_SHUFFLE_D( 0, 1, 0, 1 ) );

	temp1 = minColor;
	temp1 = _mm_and_si128( temp1, *( __m128c* )SIMD_SSE2_byte_colorMask2 );
	color1 = _mm_shuffle_epi32( temp1, R_SHUFFLE_D( 0, 1, 0, 1 ) );

	temp0 = _mm_unpacklo_epi8( color0, zero );
	temp1 = _mm_unpacklo_epi8( color1, zero );

	temp6 = _mm_add_epi16( temp1, temp0 );
	temp0 = _mm_add_epi16( temp0, temp6 );
	temp0 = _mm_mulhi_epi16( temp0, ( const __m128i& )SIMD_SSE2_word_div_by_3 );		// * ( ( 1 << 16 ) / 3 + 1 ) ) >> 16
	temp0 = _mm_packus_epi16( temp0, zero );
	color2 = _mm_shuffle_epi32( temp0, R_SHUFFLE_D( 0, 1, 0, 1 ) );

	temp1 = _mm_add_epi16( temp1, temp6 );
	temp1 = _mm_mulhi_epi16( temp1, ( const __m128i& )SIMD_SSE2_word_div_by_3 );		// * ( ( 1 << 16 ) / 3 + 1 ) ) >> 16
	temp1 = _mm_packus_epi16( temp1, zero );
	color3 = _mm_shuffle_epi32( temp1, R_SHUFFLE_D( 0, 1, 0, 1 ) );

	for( int i = 1; i >= 0; i-- )
	{
		// Load block
		temp3 = _mm_shuffle_epi32( blocka[i], R_SHUFFLE_D( 0, 2, 1, 3 ) );
		temp5 = _mm_shuffle_ps( blocka[i], zero, R_SHUFFLE_D( 2, 3, 0, 1 ) );
		temp5 = _mm_shuffle_epi32( temp5, R_SHUFFLE_D( 0, 2, 1, 3 ) );

		temp0 = _mm_sad_epu8( temp3, color0 );
		temp6 = _mm_sad_epu8( temp5, color0 );
		temp0 = _mm_packs_epi32( temp0, temp6 );

		temp1 = _mm_sad_epu8( temp3, color1 );
		temp6 = _mm_sad_epu8( temp5, color1 );
		temp1 = _mm_packs_epi32( temp1, temp6 );

		temp2 = _mm_sad_epu8( temp3, color2 );
		temp6 = _mm_sad_epu8( temp5, color2 );
		temp2 = _mm_packs_epi32( temp2, temp6 );

		temp3 = _mm_sad_epu8( temp3, color3 );
		temp5 = _mm_sad_epu8( temp5, color3 );
		temp3 = _mm_packs_epi32( temp3, temp5 );


		// Load block
		temp4 = _mm_shuffle_epi32( blockb[i], R_SHUFFLE_D( 0, 2, 1, 3 ) );
		temp5 = _mm_shuffle_ps( blockb[i], zero, R_SHUFFLE_D( 2, 3, 0, 1 ) );
		temp5 = _mm_shuffle_epi32( temp5, R_SHUFFLE_D( 0, 2, 1, 3 ) );

		temp6 = _mm_sad_epu8( temp4, color0 );
		temp7 = _mm_sad_epu8( temp5, color0 );
		temp6 = _mm_packs_epi32( temp6, temp7 );
		temp0 = _mm_packs_epi32( temp0, temp6 );	// d0

		temp6 = _mm_sad_epu8( temp4, color1 );
		temp7 = _mm_sad_epu8( temp5, color1 );
		temp6 = _mm_packs_epi32( temp6, temp7 );
		temp1 = _mm_packs_epi32( temp1, temp6 );	// d1

		temp6 = _mm_sad_epu8( temp4, color2 );
		temp7 = _mm_sad_epu8( temp5, color2 );
		temp6 = _mm_packs_epi32( temp6, temp7 );
		temp2 = _mm_packs_epi32( temp2, temp6 );	// d2

		temp4 = _mm_sad_epu8( temp4, color3 );
		temp5 = _mm_sad_epu8( temp5, color3 );
		temp4 = _mm_packs_epi32( temp4, temp5 );
		temp3 = _mm_packs_epi32( temp3, temp4 );	// d3

		temp7 = _mm_slli_epi32( result, 16 );

		temp4 = _mm_cmpgt_epi16( temp0, temp2 );	// b2
		temp5 = _mm_cmpgt_epi16( temp1, temp3 );	// b3
		temp0 = _mm_cmpgt_epi16( temp0, temp3 );	// b0
		temp1 = _mm_cmpgt_epi16( temp1, temp2 );	// b1
		temp2 = _mm_cmpgt_epi16( temp2, temp3 );	// b4
		temp4 = _mm_and_si128( temp4, temp1 );		// x0
		temp5 = _mm_and_si128( temp5, temp0 );		// x1
		temp2 = _mm_and_si128( temp2, temp0 );		// x2
		temp4 = _mm_or_si128( temp4, temp5 );
		temp2 = _mm_and_si128( temp2, ( const __m128i& )SIMD_SSE2_word_1 );
		temp4 = _mm_and_si128( temp4, ( const __m128i& )SIMD_SSE2_word_2 );
		temp2 = _mm_or_si128( temp2, temp4 );

		temp5 = _mm_shuffle_epi32( temp2, R_SHUFFLE_D( 2, 3, 0, 1 ) );
		temp2 = _mm_unpacklo_epi16( temp2, ( const __m128i& )SIMD_SSE2_word_0 );
		temp5 = _mm_unpacklo_epi16( temp5, ( const __m128i& )SIMD_SSE2_word_0 );
		temp5 = _mm_slli_epi32( temp5, 8 );
		temp7 = _mm_or_si128( temp7, temp5 );
		result = _mm_or_si128( temp7, temp2 );
	}

	temp4 = _mm_shuffle_epi32( result, R_SHUFFLE_D( 1, 2, 3, 0 ) );
	temp5 = _mm_shuffle_epi32( result, R_SHUFFLE_D( 2, 3, 0, 1 ) );
	temp6 = _mm_shuffle_epi32( result, R_SHUFFLE_D( 3, 0, 1, 2 ) );
	temp4 = _mm_slli_epi32( temp4, 2 );
	temp5 = _mm_slli_epi32( temp5, 4 );
	temp6 = _mm_slli_epi32( temp6, 6 );
	temp7 = _mm_or_si128( result, temp4 );
	temp7 = _mm_or_si128( temp7, temp5 );
	temp7 = _mm_or_si128( temp7, temp6 );

	unsigned int out = _mm_cvtsi128_si32( temp7 );
	EmitUInt( out );
}

/*
========================
idDxtEncoder::EmitAlphaIndices_SSE2

params:	block		- 16 pixel block for which to find alpha indices
paramO:	minAlpha	- Min alpha found
paramO:	maxAlpha	- Max alpha found
========================
*/
void idDxtEncoder::EmitAlphaIndices_SSE2( const byte* block, const int minAlpha_, const int maxAlpha_ )
{
	__m128i block0 = *( ( __m128i* )( &block[ 0] ) );
	__m128i block1 = *( ( __m128i* )( &block[16] ) );
	__m128i block2 = *( ( __m128i* )( &block[32] ) );
	__m128i block3 = *( ( __m128i* )( &block[48] ) );
	__m128c temp0, temp1, temp2, temp3, temp4, temp5, temp6, temp7;

	temp0 = _mm_srli_epi32( block0, 24 );
	temp5 = _mm_srli_epi32( block1, 24 );
	temp6 = _mm_srli_epi32( block2, 24 );
	temp4 = _mm_srli_epi32( block3, 24 );

	temp0 = _mm_packus_epi16( temp0, temp5 );
	temp6 = _mm_packus_epi16( temp6, temp4 );

	//---------------------

	// ab0 = (  7 * maxAlpha +  7 * minAlpha + ALPHA_RANGE ) / 14
	// ab3 = (  9 * maxAlpha +  5 * minAlpha + ALPHA_RANGE ) / 14
	// ab2 = ( 11 * maxAlpha +  3 * minAlpha + ALPHA_RANGE ) / 14
	// ab1 = ( 13 * maxAlpha +  1 * minAlpha + ALPHA_RANGE ) / 14

	// ab4 = (  7 * maxAlpha +  7 * minAlpha + ALPHA_RANGE ) / 14
	// ab5 = (  5 * maxAlpha +  9 * minAlpha + ALPHA_RANGE ) / 14
	// ab6 = (  3 * maxAlpha + 11 * minAlpha + ALPHA_RANGE ) / 14
	// ab7 = (  1 * maxAlpha + 13 * minAlpha + ALPHA_RANGE ) / 14

	temp5 = _mm_cvtsi32_si128( maxAlpha_ );
	temp5 = _mm_shufflelo_epi16( temp5, R_SHUFFLE_D( 0, 0, 0, 0 ) );
	temp5 = _mm_shuffle_epi32( temp5, R_SHUFFLE_D( 0, 0, 0, 0 ) );

	temp2 = _mm_cvtsi32_si128( minAlpha_ );
	temp2 = _mm_shufflelo_epi16( temp2, R_SHUFFLE_D( 0, 0, 0, 0 ) );
	temp2 = _mm_shuffle_epi32( temp2, R_SHUFFLE_D( 0, 0, 0, 0 ) );

	temp7 = _mm_mullo_epi16( temp5, ( const __m128i& )SIMD_SSE2_word_scale_7_5_3_1 );
	temp5 = _mm_mullo_epi16( temp5, ( const __m128i& )SIMD_SSE2_word_scale_7_9_11_13 );
	temp3 = _mm_mullo_epi16( temp2, ( const __m128i& )SIMD_SSE2_word_scale_7_9_11_13 );
	temp2 = _mm_mullo_epi16( temp2, ( const __m128i& )SIMD_SSE2_word_scale_7_5_3_1 );

	temp5 = _mm_add_epi16( temp5, temp2 );
	temp7 = _mm_add_epi16( temp7, temp3 );

	temp5 = _mm_add_epi16( temp5, ( const __m128i& )SIMD_SSE2_word_7 );
	temp7 = _mm_add_epi16( temp7, ( const __m128i& )SIMD_SSE2_word_7 );

	temp5 = _mm_mulhi_epi16( temp5, ( const __m128i& )SIMD_SSE2_word_div_by_14 );
	temp7 = _mm_mulhi_epi16( temp7, ( const __m128i& )SIMD_SSE2_word_div_by_14 );

	temp1 = _mm_shuffle_epi32( temp5, R_SHUFFLE_D( 3, 3, 3, 3 ) );
	temp2 = _mm_shuffle_epi32( temp5, R_SHUFFLE_D( 2, 2, 2, 2 ) );
	temp3 = _mm_shuffle_epi32( temp5, R_SHUFFLE_D( 1, 1, 1, 1 ) );
	temp1 = _mm_packus_epi16( temp1, temp1 );
	temp2 = _mm_packus_epi16( temp2, temp2 );
	temp3 = _mm_packus_epi16( temp3, temp3 );

	temp0 = _mm_packus_epi16( temp0, temp6 );

	temp4 = _mm_shuffle_epi32( temp7, R_SHUFFLE_D( 0, 0, 0, 0 ) );
	temp5 = _mm_shuffle_epi32( temp7, R_SHUFFLE_D( 1, 1, 1, 1 ) );
	temp6 = _mm_shuffle_epi32( temp7, R_SHUFFLE_D( 2, 2, 2, 2 ) );
	temp7 = _mm_shuffle_epi32( temp7, R_SHUFFLE_D( 3, 3, 3, 3 ) );
	temp4 = _mm_packus_epi16( temp4, temp4 );
	temp5 = _mm_packus_epi16( temp5, temp5 );
	temp6 = _mm_packus_epi16( temp6, temp6 );
	temp7 = _mm_packus_epi16( temp7, temp7 );

	temp1 = _mm_max_epu8( temp1, temp0 );
	temp2 = _mm_max_epu8( temp2, temp0 );
	temp3 = _mm_max_epu8( temp3, temp0 );
	temp1 = _mm_cmpeq_epi8( temp1, temp0 );
	temp2 = _mm_cmpeq_epi8( temp2, temp0 );
	temp3 = _mm_cmpeq_epi8( temp3, temp0 );
	temp4 = _mm_max_epu8( temp4, temp0 );
	temp5 = _mm_max_epu8( temp5, temp0 );
	temp6 = _mm_max_epu8( temp6, temp0 );
	temp7 = _mm_max_epu8( temp7, temp0 );
	temp4 = _mm_cmpeq_epi8( temp4, temp0 );
	temp5 = _mm_cmpeq_epi8( temp5, temp0 );
	temp6 = _mm_cmpeq_epi8( temp6, temp0 );
	temp7 = _mm_cmpeq_epi8( temp7, temp0 );
	temp0 = _mm_adds_epi8( ( const __m128i& )SIMD_SSE2_byte_8, temp1 );
	temp2 = _mm_adds_epi8( temp2, temp3 );
	temp4 = _mm_adds_epi8( temp4, temp5 );
	temp6 = _mm_adds_epi8( temp6, temp7 );
	temp0 = _mm_adds_epi8( temp0, temp2 );
	temp4 = _mm_adds_epi8( temp4, temp6 );
	temp0 = _mm_adds_epi8( temp0, temp4 );
	temp0 = _mm_and_si128( temp0, ( const __m128i& )SIMD_SSE2_byte_7 );
	temp1 = _mm_cmpgt_epi8( ( const __m128i& )SIMD_SSE2_byte_2, temp0 );
	temp1 = _mm_and_si128( temp1, ( const __m128i& )SIMD_SSE2_byte_1 );
	temp0 = _mm_xor_si128( temp0, temp1 );

	temp1 = _mm_srli_epi64( temp0,  8 -  3 );
	temp2 = _mm_srli_epi64( temp0, 16 -  6 );
	temp3 = _mm_srli_epi64( temp0, 24 -  9 );
	temp4 = _mm_srli_epi64( temp0, 32 - 12 );
	temp5 = _mm_srli_epi64( temp0, 40 - 15 );
	temp6 = _mm_srli_epi64( temp0, 48 - 18 );
	temp7 = _mm_srli_epi64( temp0, 56 - 21 );
	temp0 = _mm_and_si128( temp0, ( const __m128i& )SIMD_SSE2_dword_alpha_bit_mask0 );
	temp1 = _mm_and_si128( temp1, ( const __m128i& )SIMD_SSE2_dword_alpha_bit_mask1 );
	temp2 = _mm_and_si128( temp2, ( const __m128i& )SIMD_SSE2_dword_alpha_bit_mask2 );
	temp3 = _mm_and_si128( temp3, ( const __m128i& )SIMD_SSE2_dword_alpha_bit_mask3 );
	temp4 = _mm_and_si128( temp4, ( const __m128i& )SIMD_SSE2_dword_alpha_bit_mask4 );
	temp5 = _mm_and_si128( temp5, ( const __m128i& )SIMD_SSE2_dword_alpha_bit_mask5 );
	temp6 = _mm_and_si128( temp6, ( const __m128i& )SIMD_SSE2_dword_alpha_bit_mask6 );
	temp7 = _mm_and_si128( temp7, ( const __m128i& )SIMD_SSE2_dword_alpha_bit_mask7 );
	temp0 = _mm_or_si128( temp0, temp1 );
	temp2 = _mm_or_si128( temp2, temp3 );
	temp4 = _mm_or_si128( temp4, temp5 );
	temp6 = _mm_or_si128( temp6, temp7 );
	temp0 = _mm_or_si128( temp0, temp2 );
	temp4 = _mm_or_si128( temp4, temp6 );
	temp0 = _mm_or_si128( temp0, temp4 );


	int out = _mm_cvtsi128_si32( temp0 );
	EmitUInt( out );
	outData--;

	temp1 = _mm_shuffle_epi32( temp0, R_SHUFFLE_D( 2, 3, 0, 1 ) );

	out = _mm_cvtsi128_si32( temp1 );
	EmitUInt( out );
	outData--;
}

/*
========================
idDxtEncoder::EmitAlphaIndices_SSE2
========================
*/
void idDxtEncoder::EmitAlphaIndices_SSE2( const byte* block, const int channelBitOffset, const int minAlpha_, const int maxAlpha_ )
{
	__m128i block0 = *( ( __m128i* )( &block[ 0] ) );
	__m128i block1 = *( ( __m128i* )( &block[16] ) );
	__m128i block2 = *( ( __m128i* )( &block[32] ) );
	__m128i block3 = *( ( __m128i* )( &block[48] ) );
	__m128c temp0, temp1, temp2, temp3, temp4, temp5, temp6, temp7;

	temp7 = _mm_cvtsi32_si128( channelBitOffset );

	temp0 = _mm_srl_epi32( block0, temp7 );
	temp5 = _mm_srl_epi32( block1, temp7 );
	temp6 = _mm_srl_epi32( block2, temp7 );
	temp4 = _mm_srl_epi32( block3, temp7 );

	temp0 = _mm_and_si128( temp0, ( const __m128i& )SIMD_SSE2_dword_byte_mask );
	temp5 = _mm_and_si128( temp5, ( const __m128i& )SIMD_SSE2_dword_byte_mask );
	temp6 = _mm_and_si128( temp6, ( const __m128i& )SIMD_SSE2_dword_byte_mask );
	temp4 = _mm_and_si128( temp4, ( const __m128i& )SIMD_SSE2_dword_byte_mask );

	temp0 = _mm_packus_epi16( temp0, temp5 );
	temp6 = _mm_packus_epi16( temp6, temp4 );

	//---------------------

	// ab0 = (  7 * maxAlpha +  7 * minAlpha + ALPHA_RANGE ) / 14
	// ab3 = (  9 * maxAlpha +  5 * minAlpha + ALPHA_RANGE ) / 14
	// ab2 = ( 11 * maxAlpha +  3 * minAlpha + ALPHA_RANGE ) / 14
	// ab1 = ( 13 * maxAlpha +  1 * minAlpha + ALPHA_RANGE ) / 14

	// ab4 = (  7 * maxAlpha +  7 * minAlpha + ALPHA_RANGE ) / 14
	// ab5 = (  5 * maxAlpha +  9 * minAlpha + ALPHA_RANGE ) / 14
	// ab6 = (  3 * maxAlpha + 11 * minAlpha + ALPHA_RANGE ) / 14
	// ab7 = (  1 * maxAlpha + 13 * minAlpha + ALPHA_RANGE ) / 14

	temp5 = _mm_cvtsi32_si128( maxAlpha_ );
	temp5 = _mm_shufflelo_epi16( temp5, R_SHUFFLE_D( 0, 0, 0, 0 ) );
	temp5 = _mm_shuffle_epi32( temp5, R_SHUFFLE_D( 0, 0, 0, 0 ) );

	temp2 = _mm_cvtsi32_si128( minAlpha_ );
	temp2 = _mm_shufflelo_epi16( temp2, R_SHUFFLE_D( 0, 0, 0, 0 ) );
	temp2 = _mm_shuffle_epi32( temp2, R_SHUFFLE_D( 0, 0, 0, 0 ) );

	temp7 = _mm_mullo_epi16( temp5, ( const __m128i& )SIMD_SSE2_word_scale_7_5_3_1 );
	temp5 = _mm_mullo_epi16( temp5, ( const __m128i& )SIMD_SSE2_word_scale_7_9_11_13 );
	temp3 = _mm_mullo_epi16( temp2, ( const __m128i& )SIMD_SSE2_word_scale_7_9_11_13 );
	temp2 = _mm_mullo_epi16( temp2, ( const __m128i& )SIMD_SSE2_word_scale_7_5_3_1 );

	temp5 = _mm_add_epi16( temp5, temp2 );
	temp7 = _mm_add_epi16( temp7, temp3 );

	temp5 = _mm_add_epi16( temp5, ( const __m128i& )SIMD_SSE2_word_7 );
	temp7 = _mm_add_epi16( temp7, ( const __m128i& )SIMD_SSE2_word_7 );

	temp5 = _mm_mulhi_epi16( temp5, ( const __m128i& )SIMD_SSE2_word_div_by_14 );
	temp7 = _mm_mulhi_epi16( temp7, ( const __m128i& )SIMD_SSE2_word_div_by_14 );

	temp1 = _mm_shuffle_epi32( temp5, R_SHUFFLE_D( 3, 3, 3, 3 ) );
	temp2 = _mm_shuffle_epi32( temp5, R_SHUFFLE_D( 2, 2, 2, 2 ) );
	temp3 = _mm_shuffle_epi32( temp5, R_SHUFFLE_D( 1, 1, 1, 1 ) );
	temp1 = _mm_packus_epi16( temp1, temp1 );
	temp2 = _mm_packus_epi16( temp2, temp2 );
	temp3 = _mm_packus_epi16( temp3, temp3 );

	temp0 = _mm_packus_epi16( temp0, temp6 );

	temp4 = _mm_shuffle_epi32( temp7, R_SHUFFLE_D( 0, 0, 0, 0 ) );
	temp5 = _mm_shuffle_epi32( temp7, R_SHUFFLE_D( 1, 1, 1, 1 ) );
	temp6 = _mm_shuffle_epi32( temp7, R_SHUFFLE_D( 2, 2, 2, 2 ) );
	temp7 = _mm_shuffle_epi32( temp7, R_SHUFFLE_D( 3, 3, 3, 3 ) );
	temp4 = _mm_packus_epi16( temp4, temp4 );
	temp5 = _mm_packus_epi16( temp5, temp5 );
	temp6 = _mm_packus_epi16( temp6, temp6 );
	temp7 = _mm_packus_epi16( temp7, temp7 );

	temp1 = _mm_max_epu8( temp1, temp0 );
	temp2 = _mm_max_epu8( temp2, temp0 );
	temp3 = _mm_max_epu8( temp3, temp0 );
	temp1 = _mm_cmpeq_epi8( temp1, temp0 );
	temp2 = _mm_cmpeq_epi8( temp2, temp0 );
	temp3 = _mm_cmpeq_epi8( temp3, temp0 );
	temp4 = _mm_max_epu8( temp4, temp0 );
	temp5 = _mm_max_epu8( temp5, temp0 );
	temp6 = _mm_max_epu8( temp6, temp0 );
	temp7 = _mm_max_epu8( temp7, temp0 );
	temp4 = _mm_cmpeq_epi8( temp4, temp0 );
	temp5 = _mm_cmpeq_epi8( temp5, temp0 );
	temp6 = _mm_cmpeq_epi8( temp6, temp0 );
	temp7 = _mm_cmpeq_epi8( temp7, temp0 );
	temp0 = _mm_adds_epi8( ( const __m128i& )SIMD_SSE2_byte_8, temp1 );
	temp2 = _mm_adds_epi8( temp2, temp3 );
	temp4 = _mm_adds_epi8( temp4, temp5 );
	temp6 = _mm_adds_epi8( temp6, temp7 );
	temp0 = _mm_adds_epi8( temp0, temp2 );
	temp4 = _mm_adds_epi8( temp4, temp6 );
	temp0 = _mm_adds_epi8( temp0, temp4 );
	temp0 = _mm_and_si128( temp0, ( const __m128i& )SIMD_SSE2_byte_7 );
	temp1 = _mm_cmpgt_epi8( ( const __m128i& )SIMD_SSE2_byte_2, temp0 );
	temp1 = _mm_and_si128( temp1, ( const __m128i& )SIMD_SSE2_byte_1 );
	temp0 = _mm_xor_si128( temp0, temp1 );

	temp1 = _mm_srli_epi64( temp0,  8 -  3 );
	temp2 = _mm_srli_epi64( temp0, 16 -  6 );
	temp3 = _mm_srli_epi64( temp0, 24 -  9 );
	temp4 = _mm_srli_epi64( temp0, 32 - 12 );
	temp5 = _mm_srli_epi64( temp0, 40 - 15 );
	temp6 = _mm_srli_epi64( temp0, 48 - 18 );
	temp7 = _mm_srli_epi64( temp0, 56 - 21 );
	temp0 = _mm_and_si128( temp0, ( const __m128i& )SIMD_SSE2_dword_alpha_bit_mask0 );
	temp1 = _mm_and_si128( temp1, ( const __m128i& )SIMD_SSE2_dword_alpha_bit_mask1 );
	temp2 = _mm_and_si128( temp2, ( const __m128i& )SIMD_SSE2_dword_alpha_bit_mask2 );
	temp3 = _mm_and_si128( temp3, ( const __m128i& )SIMD_SSE2_dword_alpha_bit_mask3 );
	temp4 = _mm_and_si128( temp4, ( const __m128i& )SIMD_SSE2_dword_alpha_bit_mask4 );
	temp5 = _mm_and_si128( temp5, ( const __m128i& )SIMD_SSE2_dword_alpha_bit_mask5 );
	temp6 = _mm_and_si128( temp6, ( const __m128i& )SIMD_SSE2_dword_alpha_bit_mask6 );
	temp7 = _mm_and_si128( temp7, ( const __m128i& )SIMD_SSE2_dword_alpha_bit_mask7 );
	temp0 = _mm_or_si128( temp0, temp1 );
	temp2 = _mm_or_si128( temp2, temp3 );
	temp4 = _mm_or_si128( temp4, temp5 );
	temp6 = _mm_or_si128( temp6, temp7 );
	temp0 = _mm_or_si128( temp0, temp2 );
	temp4 = _mm_or_si128( temp4, temp6 );
	temp0 = _mm_or_si128( temp0, temp4 );


	int out = _mm_cvtsi128_si32( temp0 );
	EmitUInt( out );
	outData--;

	temp1 = _mm_shuffle_epi32( temp0, R_SHUFFLE_D( 2, 3, 0, 1 ) );

	out = _mm_cvtsi128_si32( temp1 );
	EmitUInt( out );
	outData--;
}

/*
========================
idDxtEncoder::CompressImageDXT1Fast_SSE2

params:	inBuf		- image to compress
paramO:	outBuf		- result of compression
params:	width		- width of image
params:	height		- height of image
========================
*/
void idDxtEncoder::CompressImageDXT1Fast_SSE2( const byte* inBuf, byte* outBuf, int width, int height )
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
		commonLocal.LoadPacifierBinarizeProgressIncrement( width * 4 );

		for( int i = 0; i < width; i += 4 )
		{
			ExtractBlock_SSE2( inBuf + i * 4, width, block );
			GetMinMaxBBox_SSE2( block, minColor, maxColor );
			InsetColorsBBox_SSE2( minColor, maxColor );

			EmitUShort( ColorTo565( maxColor ) );
			EmitUShort( ColorTo565( minColor ) );

			EmitColorIndices_SSE2( block, minColor, maxColor );
		}
		outData += dstPadding;
		inBuf += srcPadding;
	}

#ifdef TEST_COMPRESSION
	int tmpDstPadding = dstPadding;
	dstPadding = 0;
	byte* testOutBuf = ( byte* ) _alloca16( width * height / 2 );
	CompressImageDXT1Fast_Generic( inBuf, testOutBuf, width, height );
	for( int j = 0; j < height / 4; j++ )
	{
		for( int i = 0; i < width / 4; i++ )
		{
			byte* ptr1 = outBuf + ( j * width / 4 + i ) * 8 + j * tmpDstPadding;
			byte* ptr2 = testOutBuf + ( j * width / 4 + i ) * 8;
			for( int k = 0; k < 8; k++ )
			{
				assert( ptr1[k] == ptr2[k] );
			}
		}
	}
	dstPadding = tmpDstPadding;
#endif
}

/*
========================
idDxtEncoder::CompressImageDXT1AlphaFast_SSE2

params:	inBuf		- image to compress
paramO:	outBuf		- result of compression
params:	width		- width of image
params:	height		- height of image
========================
*/
void idDxtEncoder::CompressImageDXT1AlphaFast_SSE2( const byte* inBuf, byte* outBuf, int width, int height )
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
		commonLocal.LoadPacifierBinarizeProgressIncrement( width * 4 );
		for( int i = 0; i < width; i += 4 )
		{
			ExtractBlock_SSE2( inBuf + i * 4, width, block );
			GetMinMaxBBox_SSE2( block, minColor, maxColor );
			byte minAlpha = minColor[3];
			InsetColorsBBox_SSE2( minColor, maxColor );

			if( minAlpha >= 128 )
			{
				EmitUShort( ColorTo565( maxColor ) );
				EmitUShort( ColorTo565( minColor ) );
				EmitColorIndices_SSE2( block, minColor, maxColor );
			}
			else
			{
				EmitUShort( ColorTo565( minColor ) );
				EmitUShort( ColorTo565( maxColor ) );
				EmitColorAlphaIndices_SSE2( block, minColor, maxColor );
			}
		}
		outData += dstPadding;
		inBuf += srcPadding;
	}

#ifdef TEST_COMPRESSION
	int tmpDstPadding = dstPadding;
	dstPadding = 0;
	byte* testOutBuf = ( byte* ) _alloca16( width * height / 2 );
	CompressImageDXT1AlphaFast_Generic( inBuf, testOutBuf, width, height );
	for( int j = 0; j < height / 4; j++ )
	{
		for( int i = 0; i < width / 4; i++ )
		{
			byte* ptr1 = outBuf + ( j * width / 4 + i ) * 8 + j * tmpDstPadding;
			byte* ptr2 = testOutBuf + ( j * width / 4 + i ) * 8;
			for( int k = 0; k < 8; k++ )
			{
				assert( ptr1[k] == ptr2[k] );
			}
		}
	}
	dstPadding = tmpDstPadding;
#endif
}

/*
========================
idDxtEncoder::CompressImageDXT5Fast_SSE2

params:	inBuf		- image to compress
paramO:	outBuf		- result of compression
params:	width		- width of image
params:	height		- height of image
========================
*/
void idDxtEncoder::CompressImageDXT5Fast_SSE2( const byte* inBuf, byte* outBuf, int width, int height )
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
			ExtractBlock_SSE2( inBuf + i * 4, width, block );
			GetMinMaxBBox_SSE2( block, minColor, maxColor );
			InsetColorsBBox_SSE2( minColor, maxColor );

			EmitByte( maxColor[3] );
			EmitByte( minColor[3] );

			EmitAlphaIndices_SSE2( block, minColor[3], maxColor[3] );

			EmitUShort( ColorTo565( maxColor ) );
			EmitUShort( ColorTo565( minColor ) );

			EmitColorIndices_SSE2( block, minColor, maxColor );
		}
		outData += dstPadding;
		inBuf += srcPadding;
	}

#ifdef TEST_COMPRESSION
	int tmpDstPadding = dstPadding;
	dstPadding = 0;
	byte* testOutBuf = ( byte* ) _alloca16( width * height );
	CompressImageDXT5Fast_Generic( inBuf, testOutBuf, width, height );
	for( int j = 0; j < height / 4; j++ )
	{
		for( int i = 0; i < width / 4; i++ )
		{
			byte* ptr1 = outBuf + ( j * width / 4 + i ) * 16 + j * tmpDstPadding;
			byte* ptr2 = testOutBuf + ( j * width / 4 + i ) * 16;
			for( int k = 0; k < 16; k++ )
			{
				assert( ptr1[k] == ptr2[k] );
			}
		}
	}
	dstPadding = tmpDstPadding;
#endif
}

/*
========================
idDxtEncoder::ScaleYCoCg_SSE2
========================
*/
ID_INLINE void idDxtEncoder::ScaleYCoCg_SSE2( byte* colorBlock, byte* minColor, byte* maxColor ) const
{
	__m128i block0 = *( ( __m128i* )( &colorBlock[ 0] ) );
	__m128i block1 = *( ( __m128i* )( &colorBlock[16] ) );
	__m128i block2 = *( ( __m128i* )( &colorBlock[32] ) );
	__m128i block3 = *( ( __m128i* )( &colorBlock[48] ) );

	__m128c temp0, temp1, temp2, temp3, temp4, temp5, temp6, temp7;
	temp0 = _mm_cvtsi32_si128( *( int* )minColor );
	temp1 = _mm_cvtsi32_si128( *( int* )maxColor );

	temp0 = _mm_unpacklo_epi8( temp0, ( const __m128i& )SIMD_SSE2_byte_0 );
	temp1 = _mm_unpacklo_epi8( temp1, ( const __m128i& )SIMD_SSE2_byte_0 );

	// TODO: Algorithm seems to be get the absolute difference
	temp6 = _mm_sub_epi16( ( const __m128i& )SIMD_SSE2_word_center_128, temp0 );
	temp7 = _mm_sub_epi16( ( const __m128i& )SIMD_SSE2_word_center_128, temp1 );
	temp0 = _mm_sub_epi16( temp0, ( const __m128i& )SIMD_SSE2_word_center_128 );
	temp1 = _mm_sub_epi16( temp1, ( const __m128i& )SIMD_SSE2_word_center_128 );
	temp6 = _mm_max_epi16( temp6, temp0 );
	temp7 = _mm_max_epi16( temp7, temp1 );

	temp6 = _mm_max_epi16( temp6, temp7 );
	temp7 = _mm_shufflelo_epi16( temp6, R_SHUFFLE_D( 1, 0, 1, 0 ) );
	temp6 = _mm_max_epi16( temp6, temp7 );
	temp6 = _mm_shuffle_epi32( temp6, R_SHUFFLE_D( 0, 0, 0, 0 ) );

	temp7 = temp6;
	temp6 = _mm_cmpgt_epi16( temp6, ( const __m128i& )SIMD_SSE2_word_63 );			// mask0
	temp7 = _mm_cmpgt_epi16( temp7, ( const __m128i& )SIMD_SSE2_word_31 );			// mask1

	temp7 = _mm_andnot_si128( temp7, ( const __m128i& )SIMD_SSE2_byte_2 );
	temp7 = _mm_or_si128( temp7, ( const __m128i& )SIMD_SSE2_byte_1 );
	temp6 = _mm_andnot_si128( temp6, temp7 );
	temp3 = temp6;
	temp7 = temp6;
	temp7 = _mm_xor_si128( temp7, ( const __m128i& )SIMD_SSE2_byte_not );
	temp7 = _mm_or_si128( temp7, ( const __m128i& )SIMD_SSE2_byte_scale_mask0 );		// 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00
	temp6 = _mm_add_epi16( temp6, ( const __m128i& )SIMD_SSE2_byte_1 );
	temp6 = _mm_and_si128( temp6, ( const __m128i& )SIMD_SSE2_byte_scale_mask1 );	// 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF
	temp6 = _mm_or_si128( temp6, ( const __m128i& )SIMD_SSE2_byte_scale_mask2 );		// 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00

	// TODO: remove this second store
	temp4 = _mm_cvtsi32_si128( *( int* )minColor );
	temp5 = _mm_cvtsi32_si128( *( int* )maxColor );

	temp4 = _mm_and_si128( temp4, ( const __m128i& )SIMD_SSE2_byte_scale_mask3 );	// 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0xFF
	temp5 = _mm_and_si128( temp5, ( const __m128i& )SIMD_SSE2_byte_scale_mask3 );

	temp3 = _mm_slli_epi32( temp3, 3 );
	temp3 = _mm_and_si128( temp3, ( const __m128i& )SIMD_SSE2_byte_scale_mask4 );	// 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00

	temp4 = _mm_or_si128( temp4, temp3 );
	temp5 = _mm_or_si128( temp5, temp3 );

	temp4 = _mm_add_epi8( temp4, ( const __m128i& )SIMD_SSE2_byte_minus_128_0 );
	temp5 = _mm_add_epi8( temp5, ( const __m128i& )SIMD_SSE2_byte_minus_128_0 );

	temp4 = _mm_mullo_epi16( temp4, temp6 );
	temp5 = _mm_mullo_epi16( temp5, temp6 );

	temp4 = _mm_and_si128( temp4, temp7 );
	temp5 = _mm_and_si128( temp5, temp7 );

	temp4 = _mm_sub_epi8( temp4, ( const __m128i& )SIMD_SSE2_byte_minus_128_0 );
	temp5 = _mm_sub_epi8( temp5, ( const __m128i& )SIMD_SSE2_byte_minus_128_0 );

	*( int* )minColor = _mm_cvtsi128_si32( temp4 );
	*( int* )maxColor = _mm_cvtsi128_si32( temp5 );

	temp0 = _mm_add_epi8( block0, ( const __m128i& )SIMD_SSE2_byte_minus_128_0 );
	temp1 = _mm_add_epi8( block1, ( const __m128i& )SIMD_SSE2_byte_minus_128_0 );
	temp2 = _mm_add_epi8( block2, ( const __m128i& )SIMD_SSE2_byte_minus_128_0 );
	temp3 = _mm_add_epi8( block3, ( const __m128i& )SIMD_SSE2_byte_minus_128_0 );

	temp0 = _mm_mullo_epi16( temp0, temp6 );
	temp1 = _mm_mullo_epi16( temp1, temp6 );
	temp2 = _mm_mullo_epi16( temp2, temp6 );
	temp3 = _mm_mullo_epi16( temp3, temp6 );

	temp0 = _mm_and_si128( temp0, temp7 );
	temp1 = _mm_and_si128( temp1, temp7 );
	temp2 = _mm_and_si128( temp2, temp7 );
	temp3 = _mm_and_si128( temp3, temp7 );

	*( ( __m128i* )( &colorBlock[ 0] ) ) = _mm_sub_epi8( temp0, ( const __m128i& )SIMD_SSE2_byte_minus_128_0 );
	*( ( __m128i* )( &colorBlock[16] ) ) = _mm_sub_epi8( temp1, ( const __m128i& )SIMD_SSE2_byte_minus_128_0 );
	*( ( __m128i* )( &colorBlock[32] ) ) = _mm_sub_epi8( temp2, ( const __m128i& )SIMD_SSE2_byte_minus_128_0 );
	*( ( __m128i* )( &colorBlock[48] ) ) = _mm_sub_epi8( temp3, ( const __m128i& )SIMD_SSE2_byte_minus_128_0 );
}

/*
========================
idDxtEncoder::InsetYCoCgBBox_SSE2
========================
*/
ID_INLINE void idDxtEncoder::InsetYCoCgBBox_SSE2( byte* minColor, byte* maxColor ) const
{
	__m128c temp0, temp1, temp2, temp3, temp4, temp5, temp6, temp7;

	temp0 = _mm_cvtsi32_si128( *( int* )minColor );
	temp1 = _mm_cvtsi32_si128( *( int* )maxColor );

	temp0 = _mm_unpacklo_epi8( temp0, ( const __m128i& )SIMD_SSE2_byte_0 );
	temp1 = _mm_unpacklo_epi8( temp1, ( const __m128i& )SIMD_SSE2_byte_0 );

	temp2 = _mm_sub_epi16( temp1, temp0 );
	temp2 = _mm_sub_epi16( temp2, ( const __m128i& )SIMD_SSE2_word_insetYCoCgRound );
	temp2 = _mm_and_si128( temp2, ( const __m128i& )SIMD_SSE2_word_insetYCoCgMask );
	temp0 = _mm_mullo_epi16( temp0, ( const __m128i& )SIMD_SSE2_word_insetYCoCgShiftUp );
	temp1 = _mm_mullo_epi16( temp1, ( const __m128i& )SIMD_SSE2_word_insetYCoCgShiftUp );
	temp0 = _mm_add_epi16( temp0, temp2 );
	temp1 = _mm_sub_epi16( temp1, temp2 );
	temp0 = _mm_mulhi_epi16( temp0, ( const __m128i& )SIMD_SSE2_word_insetYCoCgShiftDown );
	temp1 = _mm_mulhi_epi16( temp1, ( const __m128i& )SIMD_SSE2_word_insetYCoCgShiftDown );
	temp0 = _mm_max_epi16( temp0, ( const __m128i& )SIMD_SSE2_word_0 );
	temp1 = _mm_max_epi16( temp1, ( const __m128i& )SIMD_SSE2_word_0 );
	temp0 = _mm_and_si128( temp0, ( const __m128i& )SIMD_SSE2_word_insetYCoCgQuantMask );
	temp1 = _mm_and_si128( temp1, ( const __m128i& )SIMD_SSE2_word_insetYCoCgQuantMask );
	temp2 = _mm_mulhi_epi16( temp0, ( const __m128i& )SIMD_SSE2_word_insetYCoCgRep );
	temp3 = _mm_mulhi_epi16( temp1, ( const __m128i& )SIMD_SSE2_word_insetYCoCgRep );
	temp0 = _mm_or_si128( temp0, temp2 );
	temp1 = _mm_or_si128( temp1, temp3 );
	temp0 = _mm_packus_epi16( temp0, temp0 );
	temp1 = _mm_packus_epi16( temp1, temp1 );

	*( int* )minColor = _mm_cvtsi128_si32( temp0 );
	*( int* )maxColor = _mm_cvtsi128_si32( temp1 );
}

/*
========================
idDxtEncoder::SelectYCoCgDiagonal_SSE2

params:	colorBlock	- 16 pixel block to find color indexes for
paramO:	minColor	- min color found
paramO:	maxColor	- max color found
return: diagonal to use
========================
*/
ID_INLINE void idDxtEncoder::SelectYCoCgDiagonal_SSE2( const byte* colorBlock, byte* minColor, byte* maxColor ) const
{
	__m128i block0 = *( ( __m128i* )( &colorBlock[ 0] ) );
	__m128i block1 = *( ( __m128i* )( &colorBlock[16] ) );
	__m128i block2 = *( ( __m128i* )( &colorBlock[32] ) );
	__m128i block3 = *( ( __m128i* )( &colorBlock[48] ) );
	__m128c temp0, temp1, temp2, temp3, temp4, temp5, temp6, temp7;

	temp0 = _mm_and_si128( block0, ( const __m128i& )SIMD_SSE2_dword_word_mask );
	temp1 = _mm_and_si128( block1, ( const __m128i& )SIMD_SSE2_dword_word_mask );
	temp2 = _mm_and_si128( block2, ( const __m128i& )SIMD_SSE2_dword_word_mask );
	temp3 = _mm_and_si128( block3, ( const __m128i& )SIMD_SSE2_dword_word_mask );

	temp1 = _mm_slli_si128( temp1, 2 );
	temp3 = _mm_slli_si128( temp3, 2 );
	temp0 = _mm_or_si128( temp0, temp1 );
	temp2 = _mm_or_si128( temp2, temp3 );

	temp6 = _mm_cvtsi32_si128( *( int* )minColor );
	temp7 = _mm_cvtsi32_si128( *( int* )maxColor );

	temp1 = _mm_avg_epu8( temp6, temp7 );
	temp1 = _mm_shufflelo_epi16( temp1, R_SHUFFLE_D( 0, 0, 0, 0 ) );
	temp1 = _mm_shuffle_epi32( temp1, R_SHUFFLE_D( 0, 0, 0, 0 ) );

	temp3 = _mm_max_epu8( temp1, temp2 );
	temp1 = _mm_max_epu8( temp1, temp0 );
	temp1 = _mm_cmpeq_epi8( temp1, temp0 );
	temp3 = _mm_cmpeq_epi8( temp3, temp2 );

	temp0 = _mm_srli_si128( temp1, 1 );
	temp2 = _mm_srli_si128( temp3, 1 );

	temp0 = _mm_xor_si128( temp0, temp1 );
	temp2 = _mm_xor_si128( temp2, temp3 );
	temp0 = _mm_and_si128( temp0, ( const __m128i& )SIMD_SSE2_word_1 );
	temp2 = _mm_and_si128( temp2, ( const __m128i& )SIMD_SSE2_word_1 );

	temp0 = _mm_add_epi16( temp0, temp2 );
	temp0 = _mm_sad_epu8( temp0, ( const __m128i& )SIMD_SSE2_byte_0 );
	temp1 = _mm_shuffle_epi32( temp0, R_SHUFFLE_D( 2, 3, 0, 1 ) );

#ifdef NVIDIA_7X_HARDWARE_BUG_FIX
	temp1 = _mm_add_epi16( temp1, temp0 );
	temp1 = _mm_cmpgt_epi16( temp1, ( const __m128i& )SIMD_SSE2_word_8 );
	temp1 = _mm_and_si128( temp1, ( const __m128i& )SIMD_SSE2_byte_diagonalMask );
	temp0 = _mm_cmpeq_epi8( temp6, temp7 );
	temp0 = _mm_slli_si128( temp0, 1 );
	temp0 = _mm_andnot_si128( temp0, temp1 );
#else
	temp0 = _mm_add_epi16( temp0, temp1 );
	temp0 = _mm_cmpgt_epi16( temp0, ( const __m128i& )SIMD_SSE2_word_8 );
	temp0 = _mm_and_si128( temp0, ( const __m128i& )SIMD_SSE2_byte_diagonalMask );
#endif

	temp6 = _mm_xor_si128( temp6, temp7 );
	temp0 = _mm_and_si128( temp0, temp6 );
	temp7 = _mm_xor_si128( temp7, temp0 );
	temp6 = _mm_xor_si128( temp6, temp7 );

	*( int* )minColor = _mm_cvtsi128_si32( temp6 );
	*( int* )maxColor = _mm_cvtsi128_si32( temp7 );
}

/*
========================
idDxtEncoder::CompressYCoCgDXT5Fast_SSE2

params:	inBuf		- image to compress
paramO:	outBuf		- result of compression
params:	width		- width of image
params:	height		- height of image
========================
*/
void idDxtEncoder::CompressYCoCgDXT5Fast_SSE2( const byte* inBuf, byte* outBuf, int width, int height )
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
		commonLocal.LoadPacifierBinarizeProgressIncrement( width * 4 );

		for( int i = 0; i < width; i += 4 )
		{
			ExtractBlock_SSE2( inBuf + i * 4, width, block );
			GetMinMaxBBox_SSE2( block, minColor, maxColor );

			ScaleYCoCg_SSE2( block, minColor, maxColor );
			InsetYCoCgBBox_SSE2( minColor, maxColor );
			SelectYCoCgDiagonal_SSE2( block, minColor, maxColor );

			EmitByte( maxColor[3] );
			EmitByte( minColor[3] );

			EmitAlphaIndices_SSE2( block, minColor[3], maxColor[3] );

			EmitUShort( ColorTo565( maxColor ) );
			EmitUShort( ColorTo565( minColor ) );

			EmitCoCgIndices_SSE2( block, minColor, maxColor );
		}
		outData += dstPadding;
		inBuf += srcPadding;
	}

#ifdef TEST_COMPRESSION
	int tmpDstPadding = dstPadding;
	dstPadding = 0;
	byte* testOutBuf = ( byte* ) _alloca16( width * height );
	CompressYCoCgDXT5Fast_Generic( inBuf, testOutBuf, width, height );
	for( int j = 0; j < height / 4; j++ )
	{
		for( int i = 0; i < width / 4; i++ )
		{
			byte* ptr1 = outBuf + ( j * width / 4 + i ) * 16 + j * tmpDstPadding;
			byte* ptr2 = testOutBuf + ( j * width / 4 + i ) * 16;
			for( int k = 0; k < 16; k++ )
			{
				assert( ptr1[k] == ptr2[k] );
			}
		}
	}
	dstPadding = tmpDstPadding;
#endif
}

/*
========================
idDxtEncoder::EmitGreenIndices_SSE2

params:	block		- 16-normal block for which to find normal Y indices
paramO:	minGreen	- Minimal normal Y found
paramO:	maxGreen	- Maximal normal Y found
========================
*/
void idDxtEncoder::EmitGreenIndices_SSE2( const byte* block, const int channelBitOffset, const int minGreen, const int maxGreen )
{
	__m128i block0 = *( ( __m128i* )( &block[ 0] ) );
	__m128i block1 = *( ( __m128i* )( &block[16] ) );
	__m128i block2 = *( ( __m128i* )( &block[32] ) );
	__m128i block3 = *( ( __m128i* )( &block[48] ) );
	__m128c temp0, temp1, temp2, temp3, temp4, temp5, temp6, temp7;

	temp7 = _mm_cvtsi32_si128( channelBitOffset );

	temp0 = _mm_srl_epi32( block0, temp7 );
	temp5 = _mm_srl_epi32( block1, temp7 );
	temp6 = _mm_srl_epi32( block2, temp7 );
	temp4 = _mm_srl_epi32( block3, temp7 );

	temp0 = _mm_and_si128( temp0, ( const __m128i& )SIMD_SSE2_dword_byte_mask );
	temp5 = _mm_and_si128( temp5, ( const __m128i& )SIMD_SSE2_dword_byte_mask );
	temp6 = _mm_and_si128( temp6, ( const __m128i& )SIMD_SSE2_dword_byte_mask );
	temp4 = _mm_and_si128( temp4, ( const __m128i& )SIMD_SSE2_dword_byte_mask );

	temp0 = _mm_packus_epi16( temp0, temp5 );
	temp6 = _mm_packus_epi16( temp6, temp4 );

	//---------------------

	temp2 = _mm_cvtsi32_si128( maxGreen );
	temp2 = _mm_shufflelo_epi16( temp2, R_SHUFFLE_D( 0, 0, 0, 0 ) );

	temp3 = _mm_cvtsi32_si128( minGreen );
	temp3 = _mm_shufflelo_epi16( temp3, R_SHUFFLE_D( 0, 0, 0, 0 ) );

	temp2 = _mm_mullo_epi16( temp2, ( const __m128i& )SIMD_SSE2_word_scale_5_3_1 );
	temp3 = _mm_mullo_epi16( temp3, ( const __m128i& )SIMD_SSE2_word_scale_1_3_5 );
	temp2 = _mm_add_epi16( temp2, ( const __m128i& )SIMD_SSE2_word_3 );
	temp3 = _mm_add_epi16( temp3, temp2 );
	temp3 = _mm_mulhi_epi16( temp3, ( const __m128i& )SIMD_SSE2_word_div_by_6 );

	temp1 = _mm_shufflelo_epi16( temp3, R_SHUFFLE_D( 0, 0, 0, 0 ) );
	temp2 = _mm_shufflelo_epi16( temp3, R_SHUFFLE_D( 1, 1, 1, 1 ) );
	temp3 = _mm_shufflelo_epi16( temp3, R_SHUFFLE_D( 2, 2, 2, 2 ) );

	temp1 = _mm_shuffle_epi32( temp1, R_SHUFFLE_D( 0, 0, 0, 0 ) );
	temp2 = _mm_shuffle_epi32( temp2, R_SHUFFLE_D( 0, 0, 0, 0 ) );
	temp3 = _mm_shuffle_epi32( temp3, R_SHUFFLE_D( 0, 0, 0, 0 ) );

	temp1 = _mm_packus_epi16( temp1, temp1 );
	temp2 = _mm_packus_epi16( temp2, temp2 );
	temp3 = _mm_packus_epi16( temp3, temp3 );

	temp0 = _mm_packus_epi16( temp0, temp6 );

	temp1 = _mm_max_epu8( temp1, temp0 );
	temp2 = _mm_max_epu8( temp2, temp0 );
	temp3 = _mm_max_epu8( temp3, temp0 );
	temp1 = _mm_cmpeq_epi8( temp1, temp0 );
	temp2 = _mm_cmpeq_epi8( temp2, temp0 );
	temp3 = _mm_cmpeq_epi8( temp3, temp0 );
	temp0 = ( const __m128i& )SIMD_SSE2_byte_4;

	temp0 = _mm_adds_epi8( temp0, temp1 );
	temp2 = _mm_adds_epi8( temp2, temp3 );
	temp0 = _mm_adds_epi8( temp0, temp2 );
	temp0 = _mm_and_si128( temp0, ( const __m128i& )SIMD_SSE2_byte_3 );
	temp4 = ( const __m128i& )SIMD_SSE2_byte_2;
	temp4 = _mm_cmpgt_epi8( temp4, temp0 );
	temp4 = _mm_and_si128( temp4, ( const __m128i& )SIMD_SSE2_byte_1 );

	temp0 = _mm_xor_si128( temp0, temp4 );
	temp4 = _mm_srli_epi64( temp0,  8 - 2 );
	temp5 = _mm_srli_epi64( temp0, 16 - 4 );
	temp6 = _mm_srli_epi64( temp0, 24 - 6 );
	temp7 = _mm_srli_epi64( temp0, 32 - 8 );

	temp4 = _mm_and_si128( temp4, ( const __m128i& )SIMD_SSE2_dword_color_bit_mask1 );
	temp5 = _mm_and_si128( temp5, ( const __m128i& )SIMD_SSE2_dword_color_bit_mask2 );
	temp6 = _mm_and_si128( temp6, ( const __m128i& )SIMD_SSE2_dword_color_bit_mask3 );
	temp7 = _mm_and_si128( temp7, ( const __m128i& )SIMD_SSE2_dword_color_bit_mask4 );
	temp5 = _mm_or_si128( temp5, temp4 );
	temp7 = _mm_or_si128( temp7, temp6 );
	temp7 = _mm_or_si128( temp7, temp5 );

	temp4 = _mm_srli_epi64( temp0, 40 - 10 );
	temp5 = _mm_srli_epi64( temp0, 48 - 12 );
	temp6 = _mm_srli_epi64( temp0, 56 - 14 );
	temp0 = _mm_and_si128( temp0, ( const __m128i& )SIMD_SSE2_dword_color_bit_mask0 );
	temp4 = _mm_and_si128( temp4, ( const __m128i& )SIMD_SSE2_dword_color_bit_mask5 );
	temp5 = _mm_and_si128( temp5, ( const __m128i& )SIMD_SSE2_dword_color_bit_mask6 );
	temp6 = _mm_and_si128( temp6, ( const __m128i& )SIMD_SSE2_dword_color_bit_mask7 );
	temp4 = _mm_or_si128( temp4, temp5 );
	temp0 = _mm_or_si128( temp0, temp6 );
	temp7 = _mm_or_si128( temp7, temp4 );
	temp7 = _mm_or_si128( temp7, temp0 );

	temp7 = _mm_shuffle_epi32( temp7, R_SHUFFLE_D( 0, 2, 1, 3 ) );
	temp7 = _mm_shufflelo_epi16( temp7, R_SHUFFLE_D( 0, 2, 1, 3 ) );

	int result = _mm_cvtsi128_si32( temp7 );
	EmitUInt( result );
}

/*
========================
idDxtEncoder::InsetNormalsBBoxDXT5_SSE2
========================
*/
void idDxtEncoder::InsetNormalsBBoxDXT5_SSE2( byte* minNormal, byte* maxNormal ) const
{
	__m128i temp0, temp1, temp2, temp3;

	temp0 = _mm_cvtsi32_si128( *( int* )minNormal );
	temp1 = _mm_cvtsi32_si128( *( int* )maxNormal );

	temp0 = _mm_unpacklo_epi8( temp0, ( const __m128i& )SIMD_SSE2_byte_0 );
	temp1 = _mm_unpacklo_epi8( temp1, ( const __m128i& )SIMD_SSE2_byte_0 );

	temp2 = _mm_sub_epi16( temp1, temp0 );
	temp2 = _mm_sub_epi16( temp2, ( const __m128i& )SIMD_SSE2_word_insetNormalDXT5Round );
	temp2 = _mm_and_si128( temp2, ( const __m128i& )SIMD_SSE2_word_insetNormalDXT5Mask );		// xmm2 = inset (1 & 3)

	temp0 = _mm_mullo_epi16( temp0, ( const __m128i& )SIMD_SSE2_word_insetNormalDXT5ShiftUp );
	temp1 = _mm_mullo_epi16( temp1, ( const __m128i& )SIMD_SSE2_word_insetNormalDXT5ShiftUp );
	temp0 = _mm_add_epi16( temp0, temp2 );
	temp1 = _mm_sub_epi16( temp1, temp2 );
	temp0 = _mm_mulhi_epi16( temp0, ( const __m128i& )SIMD_SSE2_word_insetNormalDXT5ShiftDown );	// xmm0 = mini
	temp1 = _mm_mulhi_epi16( temp1, ( const __m128i& )SIMD_SSE2_word_insetNormalDXT5ShiftDown );	// xmm1 = maxi

	// mini and maxi must be >= 0 and <= 255
	temp0 = _mm_max_epi16( temp0, ( const __m128i& )SIMD_SSE2_word_0 );
	temp1 = _mm_max_epi16( temp1, ( const __m128i& )SIMD_SSE2_word_0 );
	temp0 = _mm_min_epi16( temp0, ( const __m128i& )SIMD_SSE2_word_255 );
	temp1 = _mm_min_epi16( temp1, ( const __m128i& )SIMD_SSE2_word_255 );

	temp0 = _mm_and_si128( temp0, ( const __m128i& )SIMD_SSE2_word_insetNormalDXT5QuantMask );
	temp1 = _mm_and_si128( temp1, ( const __m128i& )SIMD_SSE2_word_insetNormalDXT5QuantMask );
	temp2 = _mm_mulhi_epi16( temp0, ( const __m128i& )SIMD_SSE2_word_insetNormalDXT5Rep );
	temp3 = _mm_mulhi_epi16( temp1, ( const __m128i& )SIMD_SSE2_word_insetNormalDXT5Rep );
	temp0 = _mm_or_si128( temp0, temp2 );
	temp1 = _mm_or_si128( temp1, temp3 );
	temp0 = _mm_packus_epi16( temp0, temp0 );
	temp1 = _mm_packus_epi16( temp1, temp1 );

	*( int* )minNormal = _mm_cvtsi128_si32( temp0 );
	*( int* )maxNormal = _mm_cvtsi128_si32( temp1 );
}

/*
========================
idDxtEncoder::CompressNormalMapDXT5Fast_SSE2

params:	inBuf		- image to compress in _y_x component order
paramO:	outBuf		- result of compression
params:	width		- width of image
params:	height		- height of image
========================
*/
void idDxtEncoder::CompressNormalMapDXT5Fast_SSE2( const byte* inBuf, byte* outBuf, int width, int height )
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
		commonLocal.LoadPacifierBinarizeProgressIncrement( width * 4 );

		for( int i = 0; i < width; i += 4 )
		{
			ExtractBlock_SSE2( inBuf + i * 4, width, block );
			GetMinMaxBBox_SSE2( block, normal1, normal2 );
			InsetNormalsBBoxDXT5_SSE2( normal1, normal2 );

			// Write out Nx into alpha channel.
			EmitByte( normal2[3] );
			EmitByte( normal1[3] );
			EmitAlphaIndices_SSE2( block, 3 * 8, normal1[3], normal2[3] );

			// Write out Ny into green channel.
			EmitUShort( ColorTo565( block[0], normal2[1], block[2] ) );
			EmitUShort( ColorTo565( block[0], normal1[1], block[2] ) );
			EmitGreenIndices_SSE2( block, 1 * 8, normal1[1], normal2[1] );
		}
		outData += dstPadding;
		inBuf += srcPadding;
	}

#ifdef TEST_COMPRESSION
	int tmpDstPadding = dstPadding;
	dstPadding = 0;
	byte* testOutBuf = ( byte* ) _alloca16( width * height );
	CompressNormalMapDXT5Fast_Generic( inBuf, testOutBuf, width, height );
	for( int j = 0; j < height / 4; j++ )
	{
		for( int i = 0; i < width / 4; i++ )
		{
			byte* ptr1 = outBuf + ( j * width / 4 + i ) * 16 + j * tmpDstPadding;
			byte* ptr2 = testOutBuf + ( j * width / 4 + i ) * 16;
			for( int k = 0; k < 16; k++ )
			{
				assert( ptr1[k] == ptr2[k] );
			}
		}
	}
	dstPadding = tmpDstPadding;
#endif
}

#endif // #if defined(USE_INTRINSICS_SSE)
