/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2012 Robert Beckebans

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
#include "Simd_Generic.h"
#include "Simd_SSE.h"

//===============================================================
//                                                        M
//  SSE implementation of idSIMDProcessor                MrE
//                                                        E
//===============================================================

#if defined(USE_INTRINSICS_SSE)

#include <xmmintrin.h>

#ifndef M_PI // DG: this is already defined in math.h
	#define M_PI	3.14159265358979323846f
#endif

/*
============
idSIMD_SSE::GetName
============
*/
const char* idSIMD_SSE::GetName() const
{
	return "MMX & SSE";
}

/*
============
idSIMD_SSE::BlendJoints
============
*/
void VPCALL idSIMD_SSE::BlendJoints( idJointQuat* joints, const idJointQuat* blendJoints, const float lerp, const int* index, const int numJoints )
{

	if( lerp <= 0.0f )
	{
		return;
	}
	else if( lerp >= 1.0f )
	{
		for( int i = 0; i < numJoints; i++ )
		{
			int j = index[i];
			joints[j] = blendJoints[j];
		}
		return;
	}

	const __m128 vlerp = { lerp, lerp, lerp, lerp };

	const __m128 vector_float_one		= { 1.0f, 1.0f, 1.0f, 1.0f };
	const __m128 vector_float_sign_bit	= __m128c( _mm_set_epi32( 0x80000000, 0x80000000, 0x80000000, 0x80000000 ) );
	const __m128 vector_float_rsqrt_c0	= {  -3.0f,  -3.0f,  -3.0f,  -3.0f };
	const __m128 vector_float_rsqrt_c1	= {  -0.5f,  -0.5f,  -0.5f,  -0.5f };
	const __m128 vector_float_tiny		= {    1e-10f,    1e-10f,    1e-10f,    1e-10f };
	const __m128 vector_float_half_pi	= { M_PI * 0.5f, M_PI * 0.5f, M_PI * 0.5f, M_PI * 0.5f };

	const __m128 vector_float_sin_c0	= { -2.39e-08f, -2.39e-08f, -2.39e-08f, -2.39e-08f };
	const __m128 vector_float_sin_c1	= {  2.7526e-06f, 2.7526e-06f, 2.7526e-06f, 2.7526e-06f };
	const __m128 vector_float_sin_c2	= { -1.98409e-04f, -1.98409e-04f, -1.98409e-04f, -1.98409e-04f };
	const __m128 vector_float_sin_c3	= {  8.3333315e-03f, 8.3333315e-03f, 8.3333315e-03f, 8.3333315e-03f };
	const __m128 vector_float_sin_c4	= { -1.666666664e-01f, -1.666666664e-01f, -1.666666664e-01f, -1.666666664e-01f };

	const __m128 vector_float_atan_c0	= {  0.0028662257f,  0.0028662257f,  0.0028662257f,  0.0028662257f };
	const __m128 vector_float_atan_c1	= { -0.0161657367f, -0.0161657367f, -0.0161657367f, -0.0161657367f };
	const __m128 vector_float_atan_c2	= {  0.0429096138f,  0.0429096138f,  0.0429096138f,  0.0429096138f };
	const __m128 vector_float_atan_c3	= { -0.0752896400f, -0.0752896400f, -0.0752896400f, -0.0752896400f };
	const __m128 vector_float_atan_c4	= {  0.1065626393f,  0.1065626393f,  0.1065626393f,  0.1065626393f };
	const __m128 vector_float_atan_c5	= { -0.1420889944f, -0.1420889944f, -0.1420889944f, -0.1420889944f };
	const __m128 vector_float_atan_c6	= {  0.1999355085f,  0.1999355085f,  0.1999355085f,  0.1999355085f };
	const __m128 vector_float_atan_c7	= { -0.3333314528f, -0.3333314528f, -0.3333314528f, -0.3333314528f };

	int i = 0;
	for( ; i < numJoints - 3; i += 4 )
	{
		const int n0 = index[i + 0];
		const int n1 = index[i + 1];
		const int n2 = index[i + 2];
		const int n3 = index[i + 3];

		__m128 jqa_0 = _mm_load_ps( joints[n0].q.ToFloatPtr() );
		__m128 jqb_0 = _mm_load_ps( joints[n1].q.ToFloatPtr() );
		__m128 jqc_0 = _mm_load_ps( joints[n2].q.ToFloatPtr() );
		__m128 jqd_0 = _mm_load_ps( joints[n3].q.ToFloatPtr() );

		__m128 jta_0 = _mm_load_ps( joints[n0].t.ToFloatPtr() );
		__m128 jtb_0 = _mm_load_ps( joints[n1].t.ToFloatPtr() );
		__m128 jtc_0 = _mm_load_ps( joints[n2].t.ToFloatPtr() );
		__m128 jtd_0 = _mm_load_ps( joints[n3].t.ToFloatPtr() );

		__m128 bqa_0 = _mm_load_ps( blendJoints[n0].q.ToFloatPtr() );
		__m128 bqb_0 = _mm_load_ps( blendJoints[n1].q.ToFloatPtr() );
		__m128 bqc_0 = _mm_load_ps( blendJoints[n2].q.ToFloatPtr() );
		__m128 bqd_0 = _mm_load_ps( blendJoints[n3].q.ToFloatPtr() );

		__m128 bta_0 = _mm_load_ps( blendJoints[n0].t.ToFloatPtr() );
		__m128 btb_0 = _mm_load_ps( blendJoints[n1].t.ToFloatPtr() );
		__m128 btc_0 = _mm_load_ps( blendJoints[n2].t.ToFloatPtr() );
		__m128 btd_0 = _mm_load_ps( blendJoints[n3].t.ToFloatPtr() );

		bta_0 = _mm_sub_ps( bta_0, jta_0 );
		btb_0 = _mm_sub_ps( btb_0, jtb_0 );
		btc_0 = _mm_sub_ps( btc_0, jtc_0 );
		btd_0 = _mm_sub_ps( btd_0, jtd_0 );

		jta_0 = _mm_madd_ps( vlerp, bta_0, jta_0 );
		jtb_0 = _mm_madd_ps( vlerp, btb_0, jtb_0 );
		jtc_0 = _mm_madd_ps( vlerp, btc_0, jtc_0 );
		jtd_0 = _mm_madd_ps( vlerp, btd_0, jtd_0 );

		_mm_store_ps( joints[n0].t.ToFloatPtr(), jta_0 );
		_mm_store_ps( joints[n1].t.ToFloatPtr(), jtb_0 );
		_mm_store_ps( joints[n2].t.ToFloatPtr(), jtc_0 );
		_mm_store_ps( joints[n3].t.ToFloatPtr(), jtd_0 );

		__m128 jqr_0 = _mm_unpacklo_ps( jqa_0, jqc_0 );
		__m128 jqs_0 = _mm_unpackhi_ps( jqa_0, jqc_0 );
		__m128 jqt_0 = _mm_unpacklo_ps( jqb_0, jqd_0 );
		__m128 jqu_0 = _mm_unpackhi_ps( jqb_0, jqd_0 );

		__m128 bqr_0 = _mm_unpacklo_ps( bqa_0, bqc_0 );
		__m128 bqs_0 = _mm_unpackhi_ps( bqa_0, bqc_0 );
		__m128 bqt_0 = _mm_unpacklo_ps( bqb_0, bqd_0 );
		__m128 bqu_0 = _mm_unpackhi_ps( bqb_0, bqd_0 );

		__m128 jqx_0 = _mm_unpacklo_ps( jqr_0, jqt_0 );
		__m128 jqy_0 = _mm_unpackhi_ps( jqr_0, jqt_0 );
		__m128 jqz_0 = _mm_unpacklo_ps( jqs_0, jqu_0 );
		__m128 jqw_0 = _mm_unpackhi_ps( jqs_0, jqu_0 );

		__m128 bqx_0 = _mm_unpacklo_ps( bqr_0, bqt_0 );
		__m128 bqy_0 = _mm_unpackhi_ps( bqr_0, bqt_0 );
		__m128 bqz_0 = _mm_unpacklo_ps( bqs_0, bqu_0 );
		__m128 bqw_0 = _mm_unpackhi_ps( bqs_0, bqu_0 );

		__m128 cosoma_0 = _mm_mul_ps( jqx_0, bqx_0 );
		__m128 cosomb_0 = _mm_mul_ps( jqy_0, bqy_0 );
		__m128 cosomc_0 = _mm_mul_ps( jqz_0, bqz_0 );
		__m128 cosomd_0 = _mm_mul_ps( jqw_0, bqw_0 );

		__m128 cosome_0 = _mm_add_ps( cosoma_0, cosomb_0 );
		__m128 cosomf_0 = _mm_add_ps( cosomc_0, cosomd_0 );
		__m128 cosomg_0 = _mm_add_ps( cosome_0, cosomf_0 );

		__m128 sign_0 = _mm_and_ps( cosomg_0, vector_float_sign_bit );
		__m128 cosom_0 = _mm_xor_ps( cosomg_0, sign_0 );
		__m128 ss_0 = _mm_nmsub_ps( cosom_0, cosom_0, vector_float_one );

		ss_0 = _mm_max_ps( ss_0, vector_float_tiny );

		__m128 rs_0 = _mm_rsqrt_ps( ss_0 );
		__m128 sq_0 = _mm_mul_ps( rs_0, rs_0 );
		__m128 sh_0 = _mm_mul_ps( rs_0, vector_float_rsqrt_c1 );
		__m128 sx_0 = _mm_madd_ps( ss_0, sq_0, vector_float_rsqrt_c0 );
		__m128 sinom_0 = _mm_mul_ps( sh_0, sx_0 );						// sinom = sqrt( ss );

		ss_0 = _mm_mul_ps( ss_0, sinom_0 );

		__m128 min_0 = _mm_min_ps( ss_0, cosom_0 );
		__m128 max_0 = _mm_max_ps( ss_0, cosom_0 );
		__m128 mask_0 = _mm_cmpeq_ps( min_0, cosom_0 );
		__m128 masksign_0 = _mm_and_ps( mask_0, vector_float_sign_bit );
		__m128 maskPI_0 = _mm_and_ps( mask_0, vector_float_half_pi );

		__m128 rcpa_0 = _mm_rcp_ps( max_0 );
		__m128 rcpb_0 = _mm_mul_ps( max_0, rcpa_0 );
		__m128 rcpd_0 = _mm_add_ps( rcpa_0, rcpa_0 );
		__m128 rcp_0 = _mm_nmsub_ps( rcpb_0, rcpa_0, rcpd_0 );			// 1 / y or 1 / x
		__m128 ata_0 = _mm_mul_ps( min_0, rcp_0 );						// x / y or y / x

		__m128 atb_0 = _mm_xor_ps( ata_0, masksign_0 );					// -x / y or y / x
		__m128 atc_0 = _mm_mul_ps( atb_0, atb_0 );
		__m128 atd_0 = _mm_madd_ps( atc_0, vector_float_atan_c0, vector_float_atan_c1 );

		atd_0 = _mm_madd_ps( atd_0, atc_0, vector_float_atan_c2 );
		atd_0 = _mm_madd_ps( atd_0, atc_0, vector_float_atan_c3 );
		atd_0 = _mm_madd_ps( atd_0, atc_0, vector_float_atan_c4 );
		atd_0 = _mm_madd_ps( atd_0, atc_0, vector_float_atan_c5 );
		atd_0 = _mm_madd_ps( atd_0, atc_0, vector_float_atan_c6 );
		atd_0 = _mm_madd_ps( atd_0, atc_0, vector_float_atan_c7 );
		atd_0 = _mm_madd_ps( atd_0, atc_0, vector_float_one );

		__m128 omega_a_0 = _mm_madd_ps( atd_0, atb_0, maskPI_0 );
		__m128 omega_b_0 = _mm_mul_ps( vlerp, omega_a_0 );
		omega_a_0 = _mm_sub_ps( omega_a_0, omega_b_0 );

		__m128 sinsa_0 = _mm_mul_ps( omega_a_0, omega_a_0 );
		__m128 sinsb_0 = _mm_mul_ps( omega_b_0, omega_b_0 );
		__m128 sina_0 = _mm_madd_ps( sinsa_0, vector_float_sin_c0, vector_float_sin_c1 );
		__m128 sinb_0 = _mm_madd_ps( sinsb_0, vector_float_sin_c0, vector_float_sin_c1 );
		sina_0 = _mm_madd_ps( sina_0, sinsa_0, vector_float_sin_c2 );
		sinb_0 = _mm_madd_ps( sinb_0, sinsb_0, vector_float_sin_c2 );
		sina_0 = _mm_madd_ps( sina_0, sinsa_0, vector_float_sin_c3 );
		sinb_0 = _mm_madd_ps( sinb_0, sinsb_0, vector_float_sin_c3 );
		sina_0 = _mm_madd_ps( sina_0, sinsa_0, vector_float_sin_c4 );
		sinb_0 = _mm_madd_ps( sinb_0, sinsb_0, vector_float_sin_c4 );
		sina_0 = _mm_madd_ps( sina_0, sinsa_0, vector_float_one );
		sinb_0 = _mm_madd_ps( sinb_0, sinsb_0, vector_float_one );
		sina_0 = _mm_mul_ps( sina_0, omega_a_0 );
		sinb_0 = _mm_mul_ps( sinb_0, omega_b_0 );
		__m128 scalea_0 = _mm_mul_ps( sina_0, sinom_0 );
		__m128 scaleb_0 = _mm_mul_ps( sinb_0, sinom_0 );

		scaleb_0 = _mm_xor_ps( scaleb_0, sign_0 );

		jqx_0 = _mm_mul_ps( jqx_0, scalea_0 );
		jqy_0 = _mm_mul_ps( jqy_0, scalea_0 );
		jqz_0 = _mm_mul_ps( jqz_0, scalea_0 );
		jqw_0 = _mm_mul_ps( jqw_0, scalea_0 );

		jqx_0 = _mm_madd_ps( bqx_0, scaleb_0, jqx_0 );
		jqy_0 = _mm_madd_ps( bqy_0, scaleb_0, jqy_0 );
		jqz_0 = _mm_madd_ps( bqz_0, scaleb_0, jqz_0 );
		jqw_0 = _mm_madd_ps( bqw_0, scaleb_0, jqw_0 );

		__m128 tp0_0 = _mm_unpacklo_ps( jqx_0, jqz_0 );
		__m128 tp1_0 = _mm_unpackhi_ps( jqx_0, jqz_0 );
		__m128 tp2_0 = _mm_unpacklo_ps( jqy_0, jqw_0 );
		__m128 tp3_0 = _mm_unpackhi_ps( jqy_0, jqw_0 );

		__m128 p0_0 = _mm_unpacklo_ps( tp0_0, tp2_0 );
		__m128 p1_0 = _mm_unpackhi_ps( tp0_0, tp2_0 );
		__m128 p2_0 = _mm_unpacklo_ps( tp1_0, tp3_0 );
		__m128 p3_0 = _mm_unpackhi_ps( tp1_0, tp3_0 );

		_mm_store_ps( joints[n0].q.ToFloatPtr(), p0_0 );
		_mm_store_ps( joints[n1].q.ToFloatPtr(), p1_0 );
		_mm_store_ps( joints[n2].q.ToFloatPtr(), p2_0 );
		_mm_store_ps( joints[n3].q.ToFloatPtr(), p3_0 );
	}

	for( ; i < numJoints; i++ )
	{
		int n = index[i];

		idVec3& jointVert = joints[n].t;
		const idVec3& blendVert = blendJoints[n].t;

		jointVert[0] += lerp * ( blendVert[0] - jointVert[0] );
		jointVert[1] += lerp * ( blendVert[1] - jointVert[1] );
		jointVert[2] += lerp * ( blendVert[2] - jointVert[2] );
		joints[n].w = 0.0f;

		idQuat& jointQuat = joints[n].q;
		const idQuat& blendQuat = blendJoints[n].q;

		float cosom;
		float sinom;
		float omega;
		float scale0;
		float scale1;
		// DG: use int instead of long for 64bit compatibility
		unsigned int signBit;
		// DG end

		cosom = jointQuat.x * blendQuat.x + jointQuat.y * blendQuat.y + jointQuat.z * blendQuat.z + jointQuat.w * blendQuat.w;

		// DG: use int instead of long for 64bit compatibility
		signBit = ( *( unsigned int* )&cosom ) & ( 1 << 31 );

		( *( unsigned int* )&cosom ) ^= signBit;
		// DG end

		scale0 = 1.0f - cosom * cosom;
		scale0 = ( scale0 <= 0.0f ) ? 1e-10f : scale0;
		sinom = idMath::InvSqrt( scale0 );
		omega = idMath::ATan16( scale0 * sinom, cosom );
		scale0 = idMath::Sin16( ( 1.0f - lerp ) * omega ) * sinom;
		scale1 = idMath::Sin16( lerp * omega ) * sinom;

		( *( unsigned int* )&scale1 ) ^= signBit; // DG: use int instead of long for 64bit compatibility

		jointQuat.x = scale0 * jointQuat.x + scale1 * blendQuat.x;
		jointQuat.y = scale0 * jointQuat.y + scale1 * blendQuat.y;
		jointQuat.z = scale0 * jointQuat.z + scale1 * blendQuat.z;
		jointQuat.w = scale0 * jointQuat.w + scale1 * blendQuat.w;
	}
}

/*
============
idSIMD_SSE::BlendJointsFast
============
*/
void VPCALL idSIMD_SSE::BlendJointsFast( idJointQuat* joints, const idJointQuat* blendJoints, const float lerp, const int* index, const int numJoints )
{
	assert_16_byte_aligned( joints );
	assert_16_byte_aligned( blendJoints );
	assert_16_byte_aligned( JOINTQUAT_Q_OFFSET );
	assert_16_byte_aligned( JOINTQUAT_T_OFFSET );
	assert_sizeof_16_byte_multiple( idJointQuat );

	if( lerp <= 0.0f )
	{
		return;
	}
	else if( lerp >= 1.0f )
	{
		for( int i = 0; i < numJoints; i++ )
		{
			int j = index[i];
			joints[j] = blendJoints[j];
		}
		return;
	}

	const __m128 vector_float_sign_bit	= __m128c( _mm_set_epi32( 0x80000000, 0x80000000, 0x80000000, 0x80000000 ) );
	const __m128 vector_float_rsqrt_c0	= {  -3.0f,  -3.0f,  -3.0f,  -3.0f };
	const __m128 vector_float_rsqrt_c1	= {  -0.5f,  -0.5f,  -0.5f,  -0.5f };

	const float scaledLerp = lerp / ( 1.0f - lerp );
	const __m128 vlerp = { lerp, lerp, lerp, lerp };
	const __m128 vscaledLerp = { scaledLerp, scaledLerp, scaledLerp, scaledLerp };

	int i = 0;
	for( ; i < numJoints - 3; i += 4 )
	{
		const int n0 = index[i + 0];
		const int n1 = index[i + 1];
		const int n2 = index[i + 2];
		const int n3 = index[i + 3];

		__m128 jqa_0 = _mm_load_ps( joints[n0].q.ToFloatPtr() );
		__m128 jqb_0 = _mm_load_ps( joints[n1].q.ToFloatPtr() );
		__m128 jqc_0 = _mm_load_ps( joints[n2].q.ToFloatPtr() );
		__m128 jqd_0 = _mm_load_ps( joints[n3].q.ToFloatPtr() );

		__m128 jta_0 = _mm_load_ps( joints[n0].t.ToFloatPtr() );
		__m128 jtb_0 = _mm_load_ps( joints[n1].t.ToFloatPtr() );
		__m128 jtc_0 = _mm_load_ps( joints[n2].t.ToFloatPtr() );
		__m128 jtd_0 = _mm_load_ps( joints[n3].t.ToFloatPtr() );

		__m128 bqa_0 = _mm_load_ps( blendJoints[n0].q.ToFloatPtr() );
		__m128 bqb_0 = _mm_load_ps( blendJoints[n1].q.ToFloatPtr() );
		__m128 bqc_0 = _mm_load_ps( blendJoints[n2].q.ToFloatPtr() );
		__m128 bqd_0 = _mm_load_ps( blendJoints[n3].q.ToFloatPtr() );

		__m128 bta_0 = _mm_load_ps( blendJoints[n0].t.ToFloatPtr() );
		__m128 btb_0 = _mm_load_ps( blendJoints[n1].t.ToFloatPtr() );
		__m128 btc_0 = _mm_load_ps( blendJoints[n2].t.ToFloatPtr() );
		__m128 btd_0 = _mm_load_ps( blendJoints[n3].t.ToFloatPtr() );

		bta_0 = _mm_sub_ps( bta_0, jta_0 );
		btb_0 = _mm_sub_ps( btb_0, jtb_0 );
		btc_0 = _mm_sub_ps( btc_0, jtc_0 );
		btd_0 = _mm_sub_ps( btd_0, jtd_0 );

		jta_0 = _mm_madd_ps( vlerp, bta_0, jta_0 );
		jtb_0 = _mm_madd_ps( vlerp, btb_0, jtb_0 );
		jtc_0 = _mm_madd_ps( vlerp, btc_0, jtc_0 );
		jtd_0 = _mm_madd_ps( vlerp, btd_0, jtd_0 );

		_mm_store_ps( joints[n0].t.ToFloatPtr(), jta_0 );
		_mm_store_ps( joints[n1].t.ToFloatPtr(), jtb_0 );
		_mm_store_ps( joints[n2].t.ToFloatPtr(), jtc_0 );
		_mm_store_ps( joints[n3].t.ToFloatPtr(), jtd_0 );

		__m128 jqr_0 = _mm_unpacklo_ps( jqa_0, jqc_0 );
		__m128 jqs_0 = _mm_unpackhi_ps( jqa_0, jqc_0 );
		__m128 jqt_0 = _mm_unpacklo_ps( jqb_0, jqd_0 );
		__m128 jqu_0 = _mm_unpackhi_ps( jqb_0, jqd_0 );

		__m128 bqr_0 = _mm_unpacklo_ps( bqa_0, bqc_0 );
		__m128 bqs_0 = _mm_unpackhi_ps( bqa_0, bqc_0 );
		__m128 bqt_0 = _mm_unpacklo_ps( bqb_0, bqd_0 );
		__m128 bqu_0 = _mm_unpackhi_ps( bqb_0, bqd_0 );

		__m128 jqx_0 = _mm_unpacklo_ps( jqr_0, jqt_0 );
		__m128 jqy_0 = _mm_unpackhi_ps( jqr_0, jqt_0 );
		__m128 jqz_0 = _mm_unpacklo_ps( jqs_0, jqu_0 );
		__m128 jqw_0 = _mm_unpackhi_ps( jqs_0, jqu_0 );

		__m128 bqx_0 = _mm_unpacklo_ps( bqr_0, bqt_0 );
		__m128 bqy_0 = _mm_unpackhi_ps( bqr_0, bqt_0 );
		__m128 bqz_0 = _mm_unpacklo_ps( bqs_0, bqu_0 );
		__m128 bqw_0 = _mm_unpackhi_ps( bqs_0, bqu_0 );

		__m128 cosoma_0 = _mm_mul_ps( jqx_0, bqx_0 );
		__m128 cosomb_0 = _mm_mul_ps( jqy_0, bqy_0 );
		__m128 cosomc_0 = _mm_mul_ps( jqz_0, bqz_0 );
		__m128 cosomd_0 = _mm_mul_ps( jqw_0, bqw_0 );

		__m128 cosome_0 = _mm_add_ps( cosoma_0, cosomb_0 );
		__m128 cosomf_0 = _mm_add_ps( cosomc_0, cosomd_0 );
		__m128 cosom_0 = _mm_add_ps( cosome_0, cosomf_0 );

		__m128 sign_0 = _mm_and_ps( cosom_0, vector_float_sign_bit );

		__m128 scale_0 = _mm_xor_ps( vscaledLerp, sign_0 );

		jqx_0 = _mm_madd_ps( scale_0, bqx_0, jqx_0 );
		jqy_0 = _mm_madd_ps( scale_0, bqy_0, jqy_0 );
		jqz_0 = _mm_madd_ps( scale_0, bqz_0, jqz_0 );
		jqw_0 = _mm_madd_ps( scale_0, bqw_0, jqw_0 );

		__m128 da_0 = _mm_mul_ps( jqx_0, jqx_0 );
		__m128 db_0 = _mm_mul_ps( jqy_0, jqy_0 );
		__m128 dc_0 = _mm_mul_ps( jqz_0, jqz_0 );
		__m128 dd_0 = _mm_mul_ps( jqw_0, jqw_0 );

		__m128 de_0 = _mm_add_ps( da_0, db_0 );
		__m128 df_0 = _mm_add_ps( dc_0, dd_0 );
		__m128 d_0 = _mm_add_ps( de_0, df_0 );

		__m128 rs_0 = _mm_rsqrt_ps( d_0 );
		__m128 sq_0 = _mm_mul_ps( rs_0, rs_0 );
		__m128 sh_0 = _mm_mul_ps( rs_0, vector_float_rsqrt_c1 );
		__m128 sx_0 = _mm_madd_ps( d_0, sq_0, vector_float_rsqrt_c0 );
		__m128 s_0 = _mm_mul_ps( sh_0, sx_0 );

		jqx_0 = _mm_mul_ps( jqx_0, s_0 );
		jqy_0 = _mm_mul_ps( jqy_0, s_0 );
		jqz_0 = _mm_mul_ps( jqz_0, s_0 );
		jqw_0 = _mm_mul_ps( jqw_0, s_0 );

		__m128 tp0_0 = _mm_unpacklo_ps( jqx_0, jqz_0 );
		__m128 tp1_0 = _mm_unpackhi_ps( jqx_0, jqz_0 );
		__m128 tp2_0 = _mm_unpacklo_ps( jqy_0, jqw_0 );
		__m128 tp3_0 = _mm_unpackhi_ps( jqy_0, jqw_0 );

		__m128 p0_0 = _mm_unpacklo_ps( tp0_0, tp2_0 );
		__m128 p1_0 = _mm_unpackhi_ps( tp0_0, tp2_0 );
		__m128 p2_0 = _mm_unpacklo_ps( tp1_0, tp3_0 );
		__m128 p3_0 = _mm_unpackhi_ps( tp1_0, tp3_0 );

		_mm_store_ps( joints[n0].q.ToFloatPtr(), p0_0 );
		_mm_store_ps( joints[n1].q.ToFloatPtr(), p1_0 );
		_mm_store_ps( joints[n2].q.ToFloatPtr(), p2_0 );
		_mm_store_ps( joints[n3].q.ToFloatPtr(), p3_0 );
	}

	for( ; i < numJoints; i++ )
	{
		const int n = index[i];

		idVec3& jointVert = joints[n].t;
		const idVec3& blendVert = blendJoints[n].t;

		jointVert[0] += lerp * ( blendVert[0] - jointVert[0] );
		jointVert[1] += lerp * ( blendVert[1] - jointVert[1] );
		jointVert[2] += lerp * ( blendVert[2] - jointVert[2] );

		idQuat& jointQuat = joints[n].q;
		const idQuat& blendQuat = blendJoints[n].q;

		float cosom;
		float scale;
		float s;

		cosom = jointQuat.x * blendQuat.x + jointQuat.y * blendQuat.y + jointQuat.z * blendQuat.z + jointQuat.w * blendQuat.w;

		scale = __fsels( cosom, scaledLerp, -scaledLerp );

		jointQuat.x += scale * blendQuat.x;
		jointQuat.y += scale * blendQuat.y;
		jointQuat.z += scale * blendQuat.z;
		jointQuat.w += scale * blendQuat.w;

		s = jointQuat.x * jointQuat.x + jointQuat.y * jointQuat.y + jointQuat.z * jointQuat.z + jointQuat.w * jointQuat.w;
		s = __frsqrts( s );

		jointQuat.x *= s;
		jointQuat.y *= s;
		jointQuat.z *= s;
		jointQuat.w *= s;
	}
}

/*
============
idSIMD_SSE::ConvertJointQuatsToJointMats
============
*/
void VPCALL idSIMD_SSE::ConvertJointQuatsToJointMats( idJointMat* jointMats, const idJointQuat* jointQuats, const int numJoints )
{
	assert( sizeof( idJointQuat ) == JOINTQUAT_SIZE );
	assert( sizeof( idJointMat ) == JOINTMAT_SIZE );

	// RB: changed int to intptr_t
	assert( ( intptr_t )( &( ( idJointQuat* )0 )->t ) == ( intptr_t )( &( ( idJointQuat* )0 )->q ) + ( intptr_t )sizeof( ( ( idJointQuat* )0 )->q ) );
	// RB end

	const float* jointQuatPtr = ( float* )jointQuats;
	float* jointMatPtr = ( float* )jointMats;

	const __m128 vector_float_first_sign_bit		= __m128c( _mm_set_epi32( 0x00000000, 0x00000000, 0x00000000, 0x80000000 ) );
	const __m128 vector_float_last_three_sign_bits	= __m128c( _mm_set_epi32( 0x80000000, 0x80000000, 0x80000000, 0x00000000 ) );
	const __m128 vector_float_first_pos_half		= {   0.5f,   0.0f,   0.0f,   0.0f };	// +.5 0 0 0
	const __m128 vector_float_first_neg_half		= {  -0.5f,   0.0f,   0.0f,   0.0f };	// -.5 0 0 0
	const __m128 vector_float_quat2mat_mad1			= {  -1.0f,  -1.0f,  +1.0f,  -1.0f };	//  - - + -
	const __m128 vector_float_quat2mat_mad2			= {  -1.0f,  +1.0f,  -1.0f,  -1.0f };	//  - + - -
	const __m128 vector_float_quat2mat_mad3			= {  +1.0f,  -1.0f,  -1.0f,  +1.0f };	//  + - - +

	int i = 0;
	for( ; i + 1 < numJoints; i += 2 )
	{

		__m128 q0 = _mm_load_ps( &jointQuatPtr[i * 8 + 0 * 8 + 0] );
		__m128 q1 = _mm_load_ps( &jointQuatPtr[i * 8 + 1 * 8 + 0] );

		__m128 t0 = _mm_load_ps( &jointQuatPtr[i * 8 + 0 * 8 + 4] );
		__m128 t1 = _mm_load_ps( &jointQuatPtr[i * 8 + 1 * 8 + 4] );

		__m128 d0 = _mm_add_ps( q0, q0 );
		__m128 d1 = _mm_add_ps( q1, q1 );

		__m128 sa0 = _mm_perm_ps( q0, _MM_SHUFFLE( 1, 0, 0, 1 ) );							//   y,   x,   x,   y
		__m128 sb0 = _mm_perm_ps( d0, _MM_SHUFFLE( 2, 2, 1, 1 ) );							//  y2,  y2,  z2,  z2
		__m128 sc0 = _mm_perm_ps( q0, _MM_SHUFFLE( 3, 3, 3, 2 ) );							//   z,   w,   w,   w
		__m128 sd0 = _mm_perm_ps( d0, _MM_SHUFFLE( 0, 1, 2, 2 ) );							//  z2,  z2,  y2,  x2
		__m128 sa1 = _mm_perm_ps( q1, _MM_SHUFFLE( 1, 0, 0, 1 ) );							//   y,   x,   x,   y
		__m128 sb1 = _mm_perm_ps( d1, _MM_SHUFFLE( 2, 2, 1, 1 ) );							//  y2,  y2,  z2,  z2
		__m128 sc1 = _mm_perm_ps( q1, _MM_SHUFFLE( 3, 3, 3, 2 ) );							//   z,   w,   w,   w
		__m128 sd1 = _mm_perm_ps( d1, _MM_SHUFFLE( 0, 1, 2, 2 ) );							//  z2,  z2,  y2,  x2

		sa0 = _mm_xor_ps( sa0, vector_float_first_sign_bit );
		sa1 = _mm_xor_ps( sa1, vector_float_first_sign_bit );

		sc0 = _mm_xor_ps( sc0, vector_float_last_three_sign_bits );							// flip stupid inverse quaternions
		sc1 = _mm_xor_ps( sc1, vector_float_last_three_sign_bits );							// flip stupid inverse quaternions

		__m128 ma0 = _mm_add_ps( _mm_mul_ps( sa0, sb0 ), vector_float_first_pos_half );		//  .5 - yy2,  xy2,  xz2,  yz2		//  .5 0 0 0
		__m128 mb0 = _mm_add_ps( _mm_mul_ps( sc0, sd0 ), vector_float_first_neg_half );		// -.5 + zz2,  wz2,  wy2,  wx2		// -.5 0 0 0
		__m128 mc0 = _mm_sub_ps( vector_float_first_pos_half, _mm_mul_ps( q0, d0 ) );		//  .5 - xx2, -yy2, -zz2, -ww2		//  .5 0 0 0
		__m128 ma1 = _mm_add_ps( _mm_mul_ps( sa1, sb1 ), vector_float_first_pos_half );		//  .5 - yy2,  xy2,  xz2,  yz2		//  .5 0 0 0
		__m128 mb1 = _mm_add_ps( _mm_mul_ps( sc1, sd1 ), vector_float_first_neg_half );		// -.5 + zz2,  wz2,  wy2,  wx2		// -.5 0 0 0
		__m128 mc1 = _mm_sub_ps( vector_float_first_pos_half, _mm_mul_ps( q1, d1 ) );		//  .5 - xx2, -yy2, -zz2, -ww2		//  .5 0 0 0

		__m128 mf0 = _mm_shuffle_ps( ma0, mc0, _MM_SHUFFLE( 0, 0, 1, 1 ) );					//       xy2,  xy2, .5 - xx2, .5 - xx2	// 01, 01, 10, 10
		__m128 md0 = _mm_shuffle_ps( mf0, ma0, _MM_SHUFFLE( 3, 2, 0, 2 ) );					//  .5 - xx2,  xy2,  xz2,  yz2			// 10, 01, 02, 03
		__m128 me0 = _mm_shuffle_ps( ma0, mb0, _MM_SHUFFLE( 3, 2, 1, 0 ) );					//  .5 - yy2,  xy2,  wy2,  wx2			// 00, 01, 12, 13
		__m128 mf1 = _mm_shuffle_ps( ma1, mc1, _MM_SHUFFLE( 0, 0, 1, 1 ) );					//       xy2,  xy2, .5 - xx2, .5 - xx2	// 01, 01, 10, 10
		__m128 md1 = _mm_shuffle_ps( mf1, ma1, _MM_SHUFFLE( 3, 2, 0, 2 ) );					//  .5 - xx2,  xy2,  xz2,  yz2			// 10, 01, 02, 03
		__m128 me1 = _mm_shuffle_ps( ma1, mb1, _MM_SHUFFLE( 3, 2, 1, 0 ) );					//  .5 - yy2,  xy2,  wy2,  wx2			// 00, 01, 12, 13

		__m128 ra0 = _mm_add_ps( _mm_mul_ps( mb0, vector_float_quat2mat_mad1 ), ma0 );		// 1 - yy2 - zz2, xy2 - wz2, xz2 + wy2,					// - - + -
		__m128 rb0 = _mm_add_ps( _mm_mul_ps( mb0, vector_float_quat2mat_mad2 ), md0 );		// 1 - xx2 - zz2, xy2 + wz2,          , yz2 - wx2		// - + - -
		__m128 rc0 = _mm_add_ps( _mm_mul_ps( me0, vector_float_quat2mat_mad3 ), md0 );		// 1 - xx2 - yy2,          , xz2 - wy2, yz2 + wx2		// + - - +
		__m128 ra1 = _mm_add_ps( _mm_mul_ps( mb1, vector_float_quat2mat_mad1 ), ma1 );		// 1 - yy2 - zz2, xy2 - wz2, xz2 + wy2,					// - - + -
		__m128 rb1 = _mm_add_ps( _mm_mul_ps( mb1, vector_float_quat2mat_mad2 ), md1 );		// 1 - xx2 - zz2, xy2 + wz2,          , yz2 - wx2		// - + - -
		__m128 rc1 = _mm_add_ps( _mm_mul_ps( me1, vector_float_quat2mat_mad3 ), md1 );		// 1 - xx2 - yy2,          , xz2 - wy2, yz2 + wx2		// + - - +

		__m128 ta0 = _mm_shuffle_ps( ra0, t0, _MM_SHUFFLE( 0, 0, 2, 2 ) );
		__m128 tb0 = _mm_shuffle_ps( rb0, t0, _MM_SHUFFLE( 1, 1, 3, 3 ) );
		__m128 tc0 = _mm_shuffle_ps( rc0, t0, _MM_SHUFFLE( 2, 2, 0, 0 ) );
		__m128 ta1 = _mm_shuffle_ps( ra1, t1, _MM_SHUFFLE( 0, 0, 2, 2 ) );
		__m128 tb1 = _mm_shuffle_ps( rb1, t1, _MM_SHUFFLE( 1, 1, 3, 3 ) );
		__m128 tc1 = _mm_shuffle_ps( rc1, t1, _MM_SHUFFLE( 2, 2, 0, 0 ) );

		ra0 = _mm_shuffle_ps( ra0, ta0, _MM_SHUFFLE( 2, 0, 1, 0 ) );						// 00 01 02 10
		rb0 = _mm_shuffle_ps( rb0, tb0, _MM_SHUFFLE( 2, 0, 0, 1 ) );						// 01 00 03 11
		rc0 = _mm_shuffle_ps( rc0, tc0, _MM_SHUFFLE( 2, 0, 3, 2 ) );						// 02 03 00 12
		ra1 = _mm_shuffle_ps( ra1, ta1, _MM_SHUFFLE( 2, 0, 1, 0 ) );						// 00 01 02 10
		rb1 = _mm_shuffle_ps( rb1, tb1, _MM_SHUFFLE( 2, 0, 0, 1 ) );						// 01 00 03 11
		rc1 = _mm_shuffle_ps( rc1, tc1, _MM_SHUFFLE( 2, 0, 3, 2 ) );						// 02 03 00 12

		_mm_store_ps( &jointMatPtr[i * 12 + 0 * 12 + 0], ra0 );
		_mm_store_ps( &jointMatPtr[i * 12 + 0 * 12 + 4], rb0 );
		_mm_store_ps( &jointMatPtr[i * 12 + 0 * 12 + 8], rc0 );
		_mm_store_ps( &jointMatPtr[i * 12 + 1 * 12 + 0], ra1 );
		_mm_store_ps( &jointMatPtr[i * 12 + 1 * 12 + 4], rb1 );
		_mm_store_ps( &jointMatPtr[i * 12 + 1 * 12 + 8], rc1 );
	}

	for( ; i < numJoints; i++ )
	{

		__m128 q0 = _mm_load_ps( &jointQuatPtr[i * 8 + 0 * 8 + 0] );
		__m128 t0 = _mm_load_ps( &jointQuatPtr[i * 8 + 0 * 8 + 4] );

		__m128 d0 = _mm_add_ps( q0, q0 );

		__m128 sa0 = _mm_perm_ps( q0, _MM_SHUFFLE( 1, 0, 0, 1 ) );							//   y,   x,   x,   y
		__m128 sb0 = _mm_perm_ps( d0, _MM_SHUFFLE( 2, 2, 1, 1 ) );							//  y2,  y2,  z2,  z2
		__m128 sc0 = _mm_perm_ps( q0, _MM_SHUFFLE( 3, 3, 3, 2 ) );							//   z,   w,   w,   w
		__m128 sd0 = _mm_perm_ps( d0, _MM_SHUFFLE( 0, 1, 2, 2 ) );							//  z2,  z2,  y2,  x2

		sa0 = _mm_xor_ps( sa0, vector_float_first_sign_bit );
		sc0 = _mm_xor_ps( sc0, vector_float_last_three_sign_bits );							// flip stupid inverse quaternions

		__m128 ma0 = _mm_add_ps( _mm_mul_ps( sa0, sb0 ), vector_float_first_pos_half );		//  .5 - yy2,  xy2,  xz2,  yz2		//  .5 0 0 0
		__m128 mb0 = _mm_add_ps( _mm_mul_ps( sc0, sd0 ), vector_float_first_neg_half );		// -.5 + zz2,  wz2,  wy2,  wx2		// -.5 0 0 0
		__m128 mc0 = _mm_sub_ps( vector_float_first_pos_half, _mm_mul_ps( q0, d0 ) );		//  .5 - xx2, -yy2, -zz2, -ww2		//  .5 0 0 0

		__m128 mf0 = _mm_shuffle_ps( ma0, mc0, _MM_SHUFFLE( 0, 0, 1, 1 ) );					//       xy2,  xy2, .5 - xx2, .5 - xx2	// 01, 01, 10, 10
		__m128 md0 = _mm_shuffle_ps( mf0, ma0, _MM_SHUFFLE( 3, 2, 0, 2 ) );					//  .5 - xx2,  xy2,  xz2,  yz2			// 10, 01, 02, 03
		__m128 me0 = _mm_shuffle_ps( ma0, mb0, _MM_SHUFFLE( 3, 2, 1, 0 ) );					//  .5 - yy2,  xy2,  wy2,  wx2			// 00, 01, 12, 13

		__m128 ra0 = _mm_add_ps( _mm_mul_ps( mb0, vector_float_quat2mat_mad1 ), ma0 );		// 1 - yy2 - zz2, xy2 - wz2, xz2 + wy2,					// - - + -
		__m128 rb0 = _mm_add_ps( _mm_mul_ps( mb0, vector_float_quat2mat_mad2 ), md0 );		// 1 - xx2 - zz2, xy2 + wz2,          , yz2 - wx2		// - + - -
		__m128 rc0 = _mm_add_ps( _mm_mul_ps( me0, vector_float_quat2mat_mad3 ), md0 );		// 1 - xx2 - yy2,          , xz2 - wy2, yz2 + wx2		// + - - +

		__m128 ta0 = _mm_shuffle_ps( ra0, t0, _MM_SHUFFLE( 0, 0, 2, 2 ) );
		__m128 tb0 = _mm_shuffle_ps( rb0, t0, _MM_SHUFFLE( 1, 1, 3, 3 ) );
		__m128 tc0 = _mm_shuffle_ps( rc0, t0, _MM_SHUFFLE( 2, 2, 0, 0 ) );

		ra0 = _mm_shuffle_ps( ra0, ta0, _MM_SHUFFLE( 2, 0, 1, 0 ) );						// 00 01 02 10
		rb0 = _mm_shuffle_ps( rb0, tb0, _MM_SHUFFLE( 2, 0, 0, 1 ) );						// 01 00 03 11
		rc0 = _mm_shuffle_ps( rc0, tc0, _MM_SHUFFLE( 2, 0, 3, 2 ) );						// 02 03 00 12

		_mm_store_ps( &jointMatPtr[i * 12 + 0 * 12 + 0], ra0 );
		_mm_store_ps( &jointMatPtr[i * 12 + 0 * 12 + 4], rb0 );
		_mm_store_ps( &jointMatPtr[i * 12 + 0 * 12 + 8], rc0 );
	}
}

/*
============
idSIMD_SSE::ConvertJointMatsToJointQuats
============
*/
void VPCALL idSIMD_SSE::ConvertJointMatsToJointQuats( idJointQuat* jointQuats, const idJointMat* jointMats, const int numJoints )
{

	assert( sizeof( idJointQuat ) == JOINTQUAT_SIZE );
	assert( sizeof( idJointMat ) == JOINTMAT_SIZE );

	// RB: changed int to intptr_t
	assert( ( intptr_t )( &( ( idJointQuat* )0 )->t ) == ( intptr_t )( &( ( idJointQuat* )0 )->q ) + ( intptr_t )sizeof( ( ( idJointQuat* )0 )->q ) );
	// RB end

	const __m128 vector_float_zero		= _mm_setzero_ps();
	const __m128 vector_float_one		= { 1.0f, 1.0f, 1.0f, 1.0f };
	const __m128 vector_float_not		= __m128c( _mm_set_epi32( -1, -1, -1, -1 ) );
	const __m128 vector_float_sign_bit	= __m128c( _mm_set_epi32( 0x80000000, 0x80000000, 0x80000000, 0x80000000 ) );
	const __m128 vector_float_rsqrt_c0	= {  -3.0f,  -3.0f,  -3.0f,  -3.0f };
	const __m128 vector_float_rsqrt_c2	= { -0.25f, -0.25f, -0.25f, -0.25f };

	int i = 0;
	for( ; i < numJoints - 3; i += 4 )
	{
		const float* __restrict m = ( float* )&jointMats[i];
		float* __restrict q = ( float* )&jointQuats[i];

		__m128 ma0 = _mm_load_ps( &m[0 * 12 + 0] );
		__m128 ma1 = _mm_load_ps( &m[0 * 12 + 4] );
		__m128 ma2 = _mm_load_ps( &m[0 * 12 + 8] );

		__m128 mb0 = _mm_load_ps( &m[1 * 12 + 0] );
		__m128 mb1 = _mm_load_ps( &m[1 * 12 + 4] );
		__m128 mb2 = _mm_load_ps( &m[1 * 12 + 8] );

		__m128 mc0 = _mm_load_ps( &m[2 * 12 + 0] );
		__m128 mc1 = _mm_load_ps( &m[2 * 12 + 4] );
		__m128 mc2 = _mm_load_ps( &m[2 * 12 + 8] );

		__m128 md0 = _mm_load_ps( &m[3 * 12 + 0] );
		__m128 md1 = _mm_load_ps( &m[3 * 12 + 4] );
		__m128 md2 = _mm_load_ps( &m[3 * 12 + 8] );

		__m128 ta0 = _mm_unpacklo_ps( ma0, mc0 );	// a0, c0, a1, c1
		__m128 ta1 = _mm_unpackhi_ps( ma0, mc0 );	// a2, c2, a3, c3
		__m128 ta2 = _mm_unpacklo_ps( mb0, md0 );	// b0, d0, b1, b2
		__m128 ta3 = _mm_unpackhi_ps( mb0, md0 );	// b2, d2, b3, d3

		__m128 tb0 = _mm_unpacklo_ps( ma1, mc1 );	// a0, c0, a1, c1
		__m128 tb1 = _mm_unpackhi_ps( ma1, mc1 );	// a2, c2, a3, c3
		__m128 tb2 = _mm_unpacklo_ps( mb1, md1 );	// b0, d0, b1, b2
		__m128 tb3 = _mm_unpackhi_ps( mb1, md1 );	// b2, d2, b3, d3

		__m128 tc0 = _mm_unpacklo_ps( ma2, mc2 );	// a0, c0, a1, c1
		__m128 tc1 = _mm_unpackhi_ps( ma2, mc2 );	// a2, c2, a3, c3
		__m128 tc2 = _mm_unpacklo_ps( mb2, md2 );	// b0, d0, b1, b2
		__m128 tc3 = _mm_unpackhi_ps( mb2, md2 );	// b2, d2, b3, d3

		__m128 m00 = _mm_unpacklo_ps( ta0, ta2 );
		__m128 m01 = _mm_unpackhi_ps( ta0, ta2 );
		__m128 m02 = _mm_unpacklo_ps( ta1, ta3 );
		__m128 m03 = _mm_unpackhi_ps( ta1, ta3 );

		__m128 m10 = _mm_unpacklo_ps( tb0, tb2 );
		__m128 m11 = _mm_unpackhi_ps( tb0, tb2 );
		__m128 m12 = _mm_unpacklo_ps( tb1, tb3 );
		__m128 m13 = _mm_unpackhi_ps( tb1, tb3 );

		__m128 m20 = _mm_unpacklo_ps( tc0, tc2 );
		__m128 m21 = _mm_unpackhi_ps( tc0, tc2 );
		__m128 m22 = _mm_unpacklo_ps( tc1, tc3 );
		__m128 m23 = _mm_unpackhi_ps( tc1, tc3 );

		__m128 b00 = _mm_add_ps( m00, m11 );
		__m128 b11 = _mm_cmpgt_ps( m00, m22 );
		__m128 b01 = _mm_add_ps( b00, m22 );
		__m128 b10 = _mm_cmpgt_ps( m00, m11 );
		__m128 b0  = _mm_cmpgt_ps( b01, vector_float_zero );
		__m128 b1  = _mm_and_ps( b10, b11 );
		__m128 b2  = _mm_cmpgt_ps( m11, m22 );

		__m128 m0  = b0;
		__m128 m1  = _mm_and_ps( _mm_xor_ps( b0, vector_float_not ), b1 );
		__m128 p1  = _mm_or_ps( b0, b1 );
		__m128 p2  = _mm_or_ps( p1, b2 );
		__m128 m2  = _mm_and_ps( _mm_xor_ps( p1, vector_float_not ), b2 );
		__m128 m3  = _mm_xor_ps( p2, vector_float_not );

		__m128 i0  = _mm_or_ps( m2, m3 );
		__m128 i1  = _mm_or_ps( m1, m3 );
		__m128 i2  = _mm_or_ps( m1, m2 );

		__m128 s0  = _mm_and_ps( i0, vector_float_sign_bit );
		__m128 s1  = _mm_and_ps( i1, vector_float_sign_bit );
		__m128 s2  = _mm_and_ps( i2, vector_float_sign_bit );

		m00 = _mm_xor_ps( m00, s0 );
		m11 = _mm_xor_ps( m11, s1 );
		m22 = _mm_xor_ps( m22, s2 );
		m21 = _mm_xor_ps( m21, s0 );
		m02 = _mm_xor_ps( m02, s1 );
		m10 = _mm_xor_ps( m10, s2 );

		__m128 t0  = _mm_add_ps( m00, m11 );
		__m128 t1  = _mm_add_ps( m22, vector_float_one );
		__m128 q0  = _mm_add_ps( t0, t1 );
		__m128 q1  = _mm_sub_ps( m01, m10 );
		__m128 q2  = _mm_sub_ps( m20, m02 );
		__m128 q3  = _mm_sub_ps( m12, m21 );

		__m128 rs = _mm_rsqrt_ps( q0 );
		__m128 sq = _mm_mul_ps( rs, rs );
		__m128 sh = _mm_mul_ps( rs, vector_float_rsqrt_c2 );
		__m128 sx = _mm_madd_ps( q0, sq, vector_float_rsqrt_c0 );
		__m128 s  = _mm_mul_ps( sh, sx );

		q0 = _mm_mul_ps( q0, s );
		q1 = _mm_mul_ps( q1, s );
		q2 = _mm_mul_ps( q2, s );
		q3 = _mm_mul_ps( q3, s );

		m0 = _mm_or_ps( m0, m2 );
		m2 = _mm_or_ps( m2, m3 );

		__m128 fq0 = _mm_sel_ps( q0, q3, m0 );
		__m128 fq1 = _mm_sel_ps( q1, q2, m0 );
		__m128 fq2 = _mm_sel_ps( q2, q1, m0 );
		__m128 fq3 = _mm_sel_ps( q3, q0, m0 );

		__m128 rq0 = _mm_sel_ps( fq0, fq2, m2 );
		__m128 rq1 = _mm_sel_ps( fq1, fq3, m2 );
		__m128 rq2 = _mm_sel_ps( fq2, fq0, m2 );
		__m128 rq3 = _mm_sel_ps( fq3, fq1, m2 );

		__m128 tq0 = _mm_unpacklo_ps( rq0, rq2 );
		__m128 tq1 = _mm_unpackhi_ps( rq0, rq2 );
		__m128 tq2 = _mm_unpacklo_ps( rq1, rq3 );
		__m128 tq3 = _mm_unpackhi_ps( rq1, rq3 );

		__m128 sq0 = _mm_unpacklo_ps( tq0, tq2 );
		__m128 sq1 = _mm_unpackhi_ps( tq0, tq2 );
		__m128 sq2 = _mm_unpacklo_ps( tq1, tq3 );
		__m128 sq3 = _mm_unpackhi_ps( tq1, tq3 );

		__m128 tt0 = _mm_unpacklo_ps( m03, m23 );
		__m128 tt1 = _mm_unpackhi_ps( m03, m23 );
		__m128 tt2 = _mm_unpacklo_ps( m13, vector_float_zero );
		__m128 tt3 = _mm_unpackhi_ps( m13, vector_float_zero );

		__m128 st0 = _mm_unpacklo_ps( tt0, tt2 );
		__m128 st1 = _mm_unpackhi_ps( tt0, tt2 );
		__m128 st2 = _mm_unpacklo_ps( tt1, tt3 );
		__m128 st3 = _mm_unpackhi_ps( tt1, tt3 );

		_mm_store_ps( &q[0 * 4], sq0 );
		_mm_store_ps( &q[1 * 4], st0 );
		_mm_store_ps( &q[2 * 4], sq1 );
		_mm_store_ps( &q[3 * 4], st1 );
		_mm_store_ps( &q[4 * 4], sq2 );
		_mm_store_ps( &q[5 * 4], st2 );
		_mm_store_ps( &q[6 * 4], sq3 );
		_mm_store_ps( &q[7 * 4], st3 );
	}

	float sign[2] = { 1.0f, -1.0f };

	for( ; i < numJoints; i++ )
	{
		const float* __restrict m = ( float* )&jointMats[i];
		float* __restrict q = ( float* )&jointQuats[i];

		int b0 = m[0 * 4 + 0] + m[1 * 4 + 1] + m[2 * 4 + 2] > 0.0f;
		int b1 = m[0 * 4 + 0] > m[1 * 4 + 1] && m[0 * 4 + 0] > m[2 * 4 + 2];
		int b2 = m[1 * 4 + 1] > m[2 * 4 + 2];

		int m0 = b0;
		int m1 = ( !b0 ) & b1;
		int m2 = ( !( b0 | b1 ) ) & b2;
		int m3 = !( b0 | b1 | b2 );

		int i0 = ( m2 | m3 );
		int i1 = ( m1 | m3 );
		int i2 = ( m1 | m2 );

		float s0 = sign[i0];
		float s1 = sign[i1];
		float s2 = sign[i2];

		float t = s0 * m[0 * 4 + 0] + s1 * m[1 * 4 + 1] + s2 * m[2 * 4 + 2] + 1.0f;
		float s = __frsqrts( t );
		s = ( t * s * s + -3.0f ) * ( s * -0.25f );

		q[0] = t * s;
		q[1] = ( m[0 * 4 + 1] - s2 * m[1 * 4 + 0] ) * s;
		q[2] = ( m[2 * 4 + 0] - s1 * m[0 * 4 + 2] ) * s;
		q[3] = ( m[1 * 4 + 2] - s0 * m[2 * 4 + 1] ) * s;

		if( m0 | m2 )
		{
			// reverse
			SwapValues( q[0], q[3] );
			SwapValues( q[1], q[2] );
		}
		if( m2 | m3 )
		{
			// rotate 2
			SwapValues( q[0], q[2] );
			SwapValues( q[1], q[3] );
		}

		q[4] = m[0 * 4 + 3];
		q[5] = m[1 * 4 + 3];
		q[6] = m[2 * 4 + 3];
		q[7] = 0.0f;
	}
}

/*
============
idSIMD_SSE::TransformJoints
============
*/
void VPCALL idSIMD_SSE::TransformJoints( idJointMat* jointMats, const int* parents, const int firstJoint, const int lastJoint )
{
	const __m128 vector_float_mask_keep_last	= __m128c( _mm_set_epi32( 0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000 ) );

	const float* __restrict firstMatrix = jointMats->ToFloatPtr() + ( firstJoint + firstJoint + firstJoint - 3 ) * 4;

	__m128 pma = _mm_load_ps( firstMatrix + 0 );
	__m128 pmb = _mm_load_ps( firstMatrix + 4 );
	__m128 pmc = _mm_load_ps( firstMatrix + 8 );

	for( int joint = firstJoint; joint <= lastJoint; joint++ )
	{
		const int parent = parents[joint];
		const float* __restrict parentMatrix = jointMats->ToFloatPtr() + ( parent + parent + parent ) * 4;
		float* __restrict childMatrix = jointMats->ToFloatPtr() + ( joint + joint + joint ) * 4;

		if( parent != joint - 1 )
		{
			pma = _mm_load_ps( parentMatrix + 0 );
			pmb = _mm_load_ps( parentMatrix + 4 );
			pmc = _mm_load_ps( parentMatrix + 8 );
		}

		__m128 cma = _mm_load_ps( childMatrix + 0 );
		__m128 cmb = _mm_load_ps( childMatrix + 4 );
		__m128 cmc = _mm_load_ps( childMatrix + 8 );

		__m128 ta = _mm_splat_ps( pma, 0 );
		__m128 tb = _mm_splat_ps( pmb, 0 );
		__m128 tc = _mm_splat_ps( pmc, 0 );

		__m128 td = _mm_splat_ps( pma, 1 );
		__m128 te = _mm_splat_ps( pmb, 1 );
		__m128 tf = _mm_splat_ps( pmc, 1 );

		__m128 tg = _mm_splat_ps( pma, 2 );
		__m128 th = _mm_splat_ps( pmb, 2 );
		__m128 ti = _mm_splat_ps( pmc, 2 );

		pma = _mm_madd_ps( ta, cma, _mm_and_ps( pma, vector_float_mask_keep_last ) );
		pmb = _mm_madd_ps( tb, cma, _mm_and_ps( pmb, vector_float_mask_keep_last ) );
		pmc = _mm_madd_ps( tc, cma, _mm_and_ps( pmc, vector_float_mask_keep_last ) );

		pma = _mm_madd_ps( td, cmb, pma );
		pmb = _mm_madd_ps( te, cmb, pmb );
		pmc = _mm_madd_ps( tf, cmb, pmc );

		pma = _mm_madd_ps( tg, cmc, pma );
		pmb = _mm_madd_ps( th, cmc, pmb );
		pmc = _mm_madd_ps( ti, cmc, pmc );

		_mm_store_ps( childMatrix + 0, pma );
		_mm_store_ps( childMatrix + 4, pmb );
		_mm_store_ps( childMatrix + 8, pmc );
	}
}

/*
============
idSIMD_SSE::UntransformJoints
============
*/
void VPCALL idSIMD_SSE::UntransformJoints( idJointMat* jointMats, const int* parents, const int firstJoint, const int lastJoint )
{
	const __m128 vector_float_mask_keep_last	= __m128c( _mm_set_epi32( 0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000 ) );

	for( int joint = lastJoint; joint >= firstJoint; joint-- )
	{
		assert( parents[joint] < joint );
		const int parent = parents[joint];
		const float* __restrict parentMatrix = jointMats->ToFloatPtr() + ( parent + parent + parent ) * 4;
		float* __restrict childMatrix = jointMats->ToFloatPtr() + ( joint + joint + joint ) * 4;

		__m128 pma = _mm_load_ps( parentMatrix + 0 );
		__m128 pmb = _mm_load_ps( parentMatrix + 4 );
		__m128 pmc = _mm_load_ps( parentMatrix + 8 );

		__m128 cma = _mm_load_ps( childMatrix + 0 );
		__m128 cmb = _mm_load_ps( childMatrix + 4 );
		__m128 cmc = _mm_load_ps( childMatrix + 8 );

		__m128 ta = _mm_splat_ps( pma, 0 );
		__m128 tb = _mm_splat_ps( pma, 1 );
		__m128 tc = _mm_splat_ps( pma, 2 );

		__m128 td = _mm_splat_ps( pmb, 0 );
		__m128 te = _mm_splat_ps( pmb, 1 );
		__m128 tf = _mm_splat_ps( pmb, 2 );

		__m128 tg = _mm_splat_ps( pmc, 0 );
		__m128 th = _mm_splat_ps( pmc, 1 );
		__m128 ti = _mm_splat_ps( pmc, 2 );

		cma = _mm_sub_ps( cma, _mm_and_ps( pma, vector_float_mask_keep_last ) );
		cmb = _mm_sub_ps( cmb, _mm_and_ps( pmb, vector_float_mask_keep_last ) );
		cmc = _mm_sub_ps( cmc, _mm_and_ps( pmc, vector_float_mask_keep_last ) );

		pma = _mm_mul_ps( ta, cma );
		pmb = _mm_mul_ps( tb, cma );
		pmc = _mm_mul_ps( tc, cma );

		pma = _mm_madd_ps( td, cmb, pma );
		pmb = _mm_madd_ps( te, cmb, pmb );
		pmc = _mm_madd_ps( tf, cmb, pmc );

		pma = _mm_madd_ps( tg, cmc, pma );
		pmb = _mm_madd_ps( th, cmc, pmb );
		pmc = _mm_madd_ps( ti, cmc, pmc );

		_mm_store_ps( childMatrix + 0, pma );
		_mm_store_ps( childMatrix + 4, pmb );
		_mm_store_ps( childMatrix + 8, pmc );
	}
}

#endif // #if defined(USE_INTRINSICS_SSE)

