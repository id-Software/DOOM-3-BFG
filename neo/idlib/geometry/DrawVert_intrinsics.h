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

#ifndef __DRAWVERT_INTRINSICS_H__
#define __DRAWVERT_INTRINSICS_H__


#if defined(USE_INTRINSICS_SSE)
static const __m128i vector_int_f32_sign_mask					= _mm_set1_epi32( 1U << IEEE_FLT_SIGN_BIT );
static const __m128i vector_int_f32_exponent_mask				= _mm_set1_epi32( ( ( 1U << IEEE_FLT_EXPONENT_BITS ) - 1 ) << IEEE_FLT_MANTISSA_BITS );
static const __m128i vector_int_f32_mantissa_mask				= _mm_set1_epi32( ( 1U << IEEE_FLT_MANTISSA_BITS ) - 1 );
static const __m128i vector_int_f16_min_exponent				= _mm_set1_epi32( 0 );
static const __m128i vector_int_f16_max_exponent				= _mm_set1_epi32( ( 30 << IEEE_FLT16_MANTISSA_BITS ) );
static const __m128i vector_int_f16_min_mantissa				= _mm_set1_epi32( 0 );
static const __m128i vector_int_f16_max_mantissa				= _mm_set1_epi32( ( ( 1 << IEEE_FLT16_MANTISSA_BITS ) - 1 ) );
static const __m128i vector_int_f32_to_f16_exponent_bias		= _mm_set1_epi32( ( IEEE_FLT_EXPONENT_BIAS - IEEE_FLT16_EXPONENT_BIAS ) << IEEE_FLT16_MANTISSA_BITS );
static const int f32_to_f16_sign_shift							= IEEE_FLT_SIGN_BIT - IEEE_FLT16_SIGN_BIT;
static const int f32_to_f16_exponent_shift						= IEEE_FLT_MANTISSA_BITS - IEEE_FLT16_MANTISSA_BITS;
static const int f32_to_f16_mantissa_shift						= IEEE_FLT_MANTISSA_BITS - IEEE_FLT16_MANTISSA_BITS;

static const __m128i vector_int_zero							= _mm_setzero_si128();
static const __m128i vector_int_one								= _mm_set_epi32( 1, 1, 1, 1 );

static const __m128 vector_float_mask_clear_last				= __m128c( _mm_set_epi32( 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF ) );
static const __m128 vector_float_last_one						= {   0.0f,	  0.0f,   0.0f,   1.0f };
static const __m128 vector_float_1_over_255						= { 1.0f / 255.0f, 1.0f / 255.0f, 1.0f / 255.0f, 1.0f / 255.0f };
static const __m128 vector_float_1_over_4						= { 1.0f / 4.0f, 1.0f / 4.0f, 1.0f / 4.0f, 1.0f / 4.0f };

#endif

/*
====================
FastF32toF16
====================
*/
#if defined(USE_INTRINSICS_SSE)
ID_INLINE_EXTERN __m128i FastF32toF16( __m128i f32_bits )
{
	__m128i f16_sign     = _mm_srli_epi32( _mm_and_si128( f32_bits, vector_int_f32_sign_mask ), f32_to_f16_sign_shift );
	__m128i f16_exponent = _mm_srli_epi32( _mm_and_si128( f32_bits, vector_int_f32_exponent_mask ), f32_to_f16_exponent_shift );
	__m128i f16_mantissa = _mm_srli_epi32( _mm_and_si128( f32_bits, vector_int_f32_mantissa_mask ), f32_to_f16_mantissa_shift );

	f16_exponent = _mm_sub_epi32( f16_exponent, vector_int_f32_to_f16_exponent_bias );

	const __m128i underflow = _mm_cmplt_epi32( f16_exponent, vector_int_f16_min_exponent );
	const __m128i overflow  = _mm_cmpgt_epi32( f16_exponent, vector_int_f16_max_exponent );

	f16_exponent = _mm_sel_si128( f16_exponent, vector_int_f16_min_exponent, underflow );
	f16_exponent = _mm_sel_si128( f16_exponent, vector_int_f16_max_exponent, overflow );
	f16_mantissa = _mm_sel_si128( f16_mantissa, vector_int_f16_min_mantissa, underflow );
	f16_mantissa = _mm_sel_si128( f16_mantissa, vector_int_f16_max_mantissa, overflow );

	__m128i flt16 = _mm_or_si128( _mm_or_si128( f16_sign, f16_exponent ), f16_mantissa );

	return _mm_packs_epi32( flt16, flt16 );
}
#endif


ID_INLINE_EXTERN halfFloat_t Scalar_FastF32toF16( float f32 )
{
	const int f32_sign_mask				= 1U << IEEE_FLT_SIGN_BIT;
	const int f32_exponent_mask			= ( ( 1U << IEEE_FLT_EXPONENT_BITS ) - 1 ) << IEEE_FLT_MANTISSA_BITS;
	const int f32_mantissa_mask			= ( 1U << IEEE_FLT_MANTISSA_BITS ) - 1;
	const int f16_min_exponent			= 0;
	const int f16_max_exponent			= ( 30 << IEEE_FLT16_MANTISSA_BITS );
	const int f16_min_mantissa			= 0;
	const int f16_max_mantissa			= ( ( 1 << IEEE_FLT16_MANTISSA_BITS ) - 1 );
	const int f32_to_f16_sign_shift		= IEEE_FLT_SIGN_BIT - IEEE_FLT16_SIGN_BIT;
	const int f32_to_f16_exponent_shift	= IEEE_FLT_MANTISSA_BITS - IEEE_FLT16_MANTISSA_BITS;
	const int f32_to_f16_mantissa_shift	= IEEE_FLT_MANTISSA_BITS - IEEE_FLT16_MANTISSA_BITS;
	const int f32_to_f16_exponent_bias	= ( IEEE_FLT_EXPONENT_BIAS - IEEE_FLT16_EXPONENT_BIAS ) << IEEE_FLT16_MANTISSA_BITS;

	int f32_bits = *( unsigned int* )&f32;

	int f16_sign     = ( ( unsigned int )( f32_bits & f32_sign_mask ) >> f32_to_f16_sign_shift );
	int f16_exponent = ( ( unsigned int )( f32_bits & f32_exponent_mask ) >> f32_to_f16_exponent_shift );
	int f16_mantissa = ( ( unsigned int )( f32_bits & f32_mantissa_mask ) >> f32_to_f16_mantissa_shift );

	f16_exponent -= f32_to_f16_exponent_bias;

	const bool underflow = ( f16_exponent < f16_min_exponent );
	const bool overflow  = ( f16_exponent > f16_max_exponent );

	f16_exponent = underflow ? f16_min_exponent : f16_exponent;
	f16_exponent = overflow  ? f16_max_exponent : f16_exponent;
	f16_mantissa = underflow ? f16_min_mantissa : f16_mantissa;
	f16_mantissa = overflow  ? f16_max_mantissa : f16_mantissa;

	return ( halfFloat_t )( f16_sign | f16_exponent | f16_mantissa );
}

/*
====================
LoadSkinnedDrawVertPosition
====================
*/
#if defined(USE_INTRINSICS_SSE)
ID_INLINE_EXTERN __m128 LoadSkinnedDrawVertPosition( const idDrawVert& base, const idJointMat* joints )
{
	const idJointMat& j0 = joints[base.color[0]];
	const idJointMat& j1 = joints[base.color[1]];
	const idJointMat& j2 = joints[base.color[2]];
	const idJointMat& j3 = joints[base.color[3]];

	__m128i weights_b = _mm_cvtsi32_si128( *( const unsigned int* )base.color2 );
	__m128i weights_s = _mm_unpacklo_epi8( weights_b, vector_int_zero );
	__m128i weights_i = _mm_unpacklo_epi16( weights_s, vector_int_zero );

	__m128 weights = _mm_cvtepi32_ps( weights_i );
	weights = _mm_mul_ps( weights, vector_float_1_over_255 );

	__m128 w0 = _mm_splat_ps( weights, 0 );
	__m128 w1 = _mm_splat_ps( weights, 1 );
	__m128 w2 = _mm_splat_ps( weights, 2 );
	__m128 w3 = _mm_splat_ps( weights, 3 );

	__m128 matX = _mm_mul_ps( _mm_load_ps( j0.ToFloatPtr() + 0 * 4 ), w0 );
	__m128 matY = _mm_mul_ps( _mm_load_ps( j0.ToFloatPtr() + 1 * 4 ), w0 );
	__m128 matZ = _mm_mul_ps( _mm_load_ps( j0.ToFloatPtr() + 2 * 4 ), w0 );

	matX = _mm_madd_ps( _mm_load_ps( j1.ToFloatPtr() + 0 * 4 ), w1, matX );
	matY = _mm_madd_ps( _mm_load_ps( j1.ToFloatPtr() + 1 * 4 ), w1, matY );
	matZ = _mm_madd_ps( _mm_load_ps( j1.ToFloatPtr() + 2 * 4 ), w1, matZ );

	matX = _mm_madd_ps( _mm_load_ps( j2.ToFloatPtr() + 0 * 4 ), w2, matX );
	matY = _mm_madd_ps( _mm_load_ps( j2.ToFloatPtr() + 1 * 4 ), w2, matY );
	matZ = _mm_madd_ps( _mm_load_ps( j2.ToFloatPtr() + 2 * 4 ), w2, matZ );

	matX = _mm_madd_ps( _mm_load_ps( j3.ToFloatPtr() + 0 * 4 ), w3, matX );
	matY = _mm_madd_ps( _mm_load_ps( j3.ToFloatPtr() + 1 * 4 ), w3, matY );
	matZ = _mm_madd_ps( _mm_load_ps( j3.ToFloatPtr() + 2 * 4 ), w3, matZ );

	__m128 v = _mm_load_ps( base.xyz.ToFloatPtr() );
	v = _mm_and_ps( v, vector_float_mask_clear_last );
	v = _mm_or_ps( v, vector_float_last_one );

	__m128 t0 = _mm_mul_ps( matX, v );
	__m128 t1 = _mm_mul_ps( matY, v );
	__m128 t2 = _mm_mul_ps( matZ, v );
	__m128 t3 = vector_float_1_over_4;

	__m128 s0 = _mm_unpacklo_ps( t0, t2 );	// x0, z0, x1, z1
	__m128 s1 = _mm_unpackhi_ps( t0, t2 );	// x2, z2, x3, z3
	__m128 s2 = _mm_unpacklo_ps( t1, t3 );	// y0, w0, y1, w1
	__m128 s3 = _mm_unpackhi_ps( t1, t3 );	// y2, w2, y3, w3

	__m128 r0 = _mm_unpacklo_ps( s0, s2 );	// x0, y0, z0, w0
	__m128 r1 = _mm_unpackhi_ps( s0, s2 );	// x1, y1, z1, w1
	__m128 r2 = _mm_unpacklo_ps( s1, s3 );	// x2, y2, z2, w2
	__m128 r3 = _mm_unpackhi_ps( s1, s3 );	// x3, y3, z3, w3

	r0 = _mm_add_ps( r0, r1 );
	r2 = _mm_add_ps( r2, r3 );
	r0 = _mm_add_ps( r0, r2 );

	return r0;
}
#endif

ID_INLINE_EXTERN idVec3 Scalar_LoadSkinnedDrawVertPosition( const idDrawVert& vert, const idJointMat* joints )
{
	const idJointMat& j0 = joints[vert.color[0]];
	const idJointMat& j1 = joints[vert.color[1]];
	const idJointMat& j2 = joints[vert.color[2]];
	const idJointMat& j3 = joints[vert.color[3]];

	const float w0 = vert.color2[0] * ( 1.0f / 255.0f );
	const float w1 = vert.color2[1] * ( 1.0f / 255.0f );
	const float w2 = vert.color2[2] * ( 1.0f / 255.0f );
	const float w3 = vert.color2[3] * ( 1.0f / 255.0f );

	idJointMat accum;
	idJointMat::Mul( accum, j0, w0 );
	idJointMat::Mad( accum, j1, w1 );
	idJointMat::Mad( accum, j2, w2 );
	idJointMat::Mad( accum, j3, w3 );

	return accum * idVec4( vert.xyz.x, vert.xyz.y, vert.xyz.z, 1.0f );
}

#endif /* !__DRAWVERT_INTRINSICS_H__ */
