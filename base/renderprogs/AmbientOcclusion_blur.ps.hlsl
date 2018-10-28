/**
  \file AmbientOcclusion_blur.pix
  \author Morgan McGuire and Michael Mara, NVIDIA Research

  \brief 7-tap 1D cross-bilateral blur using a packed depth key

  Open Source under the "BSD" license: http://www.opensource.org/licenses/bsd-license.php

  Copyright (c) 2011-2014, NVIDIA
  Copyright (c) 2016 Robert Beckebans ( id Tech 4.x integration )
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

  Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
  Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include "global.inc.hlsl"

// *INDENT-OFF*
uniform sampler2D samp0 : register( s0 ); // view normals
uniform sampler2D samp1 : register( s1 ); // view depth
uniform sampler2D samp2 : register( s2 ); // view AO
 
#define normal_buffer	samp0
#define cszBuffer		samp1
#define source			samp2 
 
struct PS_IN
{
	float2 texcoord0 : TEXCOORD0_centroid;
};
 
struct PS_OUT 
{
	float4 color : COLOR;
};
// *INDENT-ON*

#define PEELED_LAYER 0
#define USE_OCT16 0
#define USE_NORMALS 1

//#expect PEELED_LAYER "binary"

//////////////////////////////////////////////////////////////////////////////////////////////
// Tunable Parameters:

//#define NUM_KEY_COMPONENTS 1

// The other parameters in this section must be passed in as macro values

/** Increase to make depth edges crisper. Decrease to reduce flicker. */
#define EDGE_SHARPNESS     (1.0)

/** Step in 2-pixel intervals since we already blurred against neighbors in the
    first AO pass.  This constant can be increased while R decreases to improve
    performance at the expense of some dithering artifacts.

    Morgan found that a scale of 3 left a 1-pixel checkerboard grid that was
    unobjectionable after shading was applied but eliminated most temporal incoherence
    from using small numbers of sample taps.
    */
#define SCALE               (2)

/** Filter radius in pixels. This will be multiplied by SCALE. */
#define R                   (4)

#define MDB_WEIGHTS 	0
//////////////////////////////////////////////////////////////////////////////////////////////

/** Type of data to read from source.  This macro allows
    the same blur shader to be used on different kinds of input data. */
#define VALUE_TYPE        float

/** Swizzle to use to extract the channels of source. This macro allows
    the same blur shader to be used on different kinds of input data. */
#define VALUE_COMPONENTS   r

#define VALUE_IS_KEY       0




/** (1, 0) or (0, 1)*/
//uniform int2       axis;

#if USE_OCT16
#include <oct.glsl>
#endif

float3 sampleNormal( sampler2D normalBuffer, int2 ssC, int mipLevel )
{
#if USE_OCT16
	return decode16( texelFetch( normalBuffer, ssC, mipLevel ).xy * 2.0 - 1.0 );
#else
	return normalize( texelFetch( normalBuffer, ssC, mipLevel ).xyz * 2.0 - 1.0 );
#endif
}

#define  aoResult       result.color.VALUE_COMPONENTS
#define  keyPassThrough result.color.KEY_COMPONENTS




/** Used for preventing AO computation on the sky (at infinite depth) and defining the CS Z to bilateral depth key scaling.
    This need not match the real far plane but should not be much more than it.*/
const float FAR_PLANE_Z = -16000.0;

float CSZToKey( float z )
{
	return clamp( z * ( 1.0 / FAR_PLANE_Z ), 0.0, 1.0 );
}

float reconstructCSZ( float d )
{
	//return clipInfo[0] / (clipInfo[1] * d + clipInfo[2]);
	
	// infinite far perspective matrix
	return -3.0 / ( -1.0 * d + 1.0 );
	
	//d = d * 2.0 - 1.0;
	//return -rpProjectionMatrixZ.w / ( -rpProjectionMatrixZ.z - d );
}

float3 reconstructCSPosition( float2 S, float z )
{
	float4 P;
	P.z = z * 2.0 - 1.0;
	P.xy = ( S * rpScreenCorrectionFactor.xy ) * 2.0 - 1.0;
	P.w = 1.0;
	
	float4 csP;
	csP.x = dot4( P, rpModelMatrixX );
	csP.y = dot4( P, rpModelMatrixY );
	csP.z = dot4( P, rpModelMatrixZ );
	csP.w = dot4( P, rpModelMatrixW );
	
	csP.xyz /= csP.w;
	
	return csP.xyz;
}

float getKey( int2 ssP )
{
#if PEELED_LAYER
	float key = texelFetch( cszBuffer, ssP, 0 ).g;
#else
	float key = texelFetch( cszBuffer, ssP, 0 ).r;
#endif
	
#if 0
	key = reconstructCSZ( key );
#else
	float3 P = reconstructCSPosition( float2( ssP ) + float2( 0.5 ), key );
	key = P.z;
#endif
	
	key = clamp( key * ( 1.0 / FAR_PLANE_Z ), 0.0, 1.0 );
	return key;
}

float3 positionFromKey( float key, int2 ssC )
{
	float z = key * FAR_PLANE_Z;
	float3 C = reconstructCSPosition( float2( ssC ) + float2( 0.5 ), z );
	return C;
}

/** Read the camera-space position of the point at screen-space pixel ssP */
float3 getPosition( int2 ssP, sampler2D cszBuffer )
{
	float3 P;
	P.z = texelFetch( cszBuffer, ssP, 0 ).r;
	
	// Offset to pixel center
	P = reconstructCSPosition( float2( ssP ) + float2( 0.5 ), P.z );
	
	return P;
}

float calculateBilateralWeight( float key, float tapKey, int2 tapLoc, float3 n_C, float3 C )
{
	// range domain (the "bilateral" weight). As depth difference increases, decrease weight.
	float depthWeight = max( 0.0, 1.0 - ( EDGE_SHARPNESS * 2000.0 ) * abs( tapKey - key ) );
	
	float k_normal = 1.0;
	float k_plane = 1.0;
	
	// Prevents blending over creases.
	float normalWeight = 1.0;
	float planeWeight = 1.0;
	
#if USE_NORMALS
	float3 tapN_C = sampleNormal( normal_buffer, tapLoc, 0 );
	depthWeight = 1.0;
	
	float normalError = 1.0 - dot( tapN_C, n_C ) * k_normal;
	normalWeight = max( ( 1.0 - EDGE_SHARPNESS * normalError ), 0.00 );
	
	float lowDistanceThreshold2 = 0.001;
	
	//float3 tapC = positionFromKey( tapKey, tapLoc, projInfo );
	float3 tapC = getPosition( tapLoc, cszBuffer );
	
	// Change in position in camera space
	float3 dq = C - tapC;
	
	// How far away is this point from the original sample
	// in camera space? (Max value is unbounded)
	float distance2 = dot( dq, dq );
	
	// How far off the expected plane (on the perpendicular) is this point?  Max value is unbounded.
	float planeError = max( abs( dot( dq, tapN_C ) ), abs( dot( dq, n_C ) ) );
	
	planeWeight = ( distance2 < lowDistanceThreshold2 ) ? 1.0 :
				  pow( max( 0.0, 1.0 - EDGE_SHARPNESS * 2.0 * k_plane * planeError / sqrt( distance2 ) ), 2.0 );
				  
				  
#endif
				  
	return depthWeight * normalWeight * planeWeight;
}



void main( PS_IN fragment, out PS_OUT result )
{

//#   if __VERSION__ < 330
	float kernel[R + 1];
//      if R == 0, we never call this shader
#if R == 1
	kernel[0] = 0.5;
	kernel[1] = 0.25;
#elif R == 2
	kernel[0] = 0.153170;
	kernel[1] = 0.144893;
	kernel[2] = 0.122649;
#elif R == 3
	kernel[0] = 0.153170;
	kernel[1] = 0.144893;
	kernel[2] = 0.122649;
	kernel[3] = 0.092902;
#elif R == 4
	kernel[0] = 0.153170;
	kernel[1] = 0.144893;
	kernel[2] = 0.122649;
	kernel[3] = 0.092902;
	kernel[4] = 0.062970;
#elif R == 5
	kernel[0] = 0.111220;
	kernel[1] = 0.107798;
	kernel[2] = 0.098151;
	kernel[3] = 0.083953;
	kernel[4] = 0.067458;
	kernel[5] = 0.050920;
#elif R == 6
	kernel[0] = 0.111220;
	kernel[1] = 0.107798;
	kernel[2] = 0.098151;
	kernel[3] = 0.083953;
	kernel[4] = 0.067458;
	kernel[5] = 0.050920;
	kernel[6] = 0.036108;
#endif
//#endif

	int2 ssC = int2( gl_FragCoord.xy );
	
	float4 temp = texelFetch( source, ssC, 0 );
	
#if 0
	if( fragment.texcoord0.x < 0.75 )
	{
		result.color = temp;
		return;
	}
#endif
	
#if 0
	float key = getKey( ssC );
	float3 C = positionFromKey( key, ssC );
#else
	float3 C = getPosition( ssC, cszBuffer );
	float key = CSZToKey( C.z );
#endif
	
	VALUE_TYPE sum = temp.VALUE_COMPONENTS;
	
	if( key == 1.0 )
	{
		// Sky pixel (if you aren't using depth keying, disable this test)
		aoResult = sum;
#if defined(BRIGHTPASS)
		result.color = float4( aoResult, aoResult, aoResult, 1.0 );
#endif
		return;
	}
	
	// Base weight for falloff.  Increase this for more blurriness,
	// decrease it for better edge discrimination
	float BASE = kernel[0];
	float totalWeight = BASE;
	sum *= totalWeight;
	
	float3 n_C;
#if USE_NORMALS
	n_C = sampleNormal( normal_buffer, ssC, 0 );
#endif
	
#if MDB_WEIGHTS == 0
	for( int r = -R; r <= R; ++r )
	{
		// We already handled the zero case above.  This loop should be unrolled and the static branch optimized out,
		// so the IF statement has no runtime cost
		if( r != 0 )
		{
			int2 tapLoc = ssC + int2( rpJitterTexScale.xy ) * ( r * SCALE );
			temp = texelFetch( source, tapLoc, 0 );
			
			
			float tapKey = getKey( tapLoc );
			VALUE_TYPE value  = temp.VALUE_COMPONENTS;
			
			// spatial domain: offset kernel tap
			float weight = 0.3 + kernel[abs( r )];
			
			float bilateralWeight = calculateBilateralWeight( key, tapKey, tapLoc, n_C, C );
			
			weight *= bilateralWeight;
			sum += value * weight;
			totalWeight += weight;
		}
	}
#else
	
	float lastBilateralWeight = 9999.0;
	for( int r = -1; r >= -R; --r )
	{
		int2 tapLoc = ssC + int2( rpJitterTexScale.xy ) * ( r * SCALE );
		temp = texelFetch( source, tapLoc, 0 );
		float      tapKey = getKey( tapLoc );
	
		VALUE_TYPE value  = temp.VALUE_COMPONENTS;
	
		// spatial domain: offset kernel tap
		float weight = 0.3 + kernel[abs( r )];
	
		// range domain (the "bilateral" weight). As depth difference increases, decrease weight.
		float bilateralWeight = calculateBilateralWeight( key, tapKey, tapLoc, n_C, C );
		bilateralWeight = min( lastBilateralWeight, bilateralWeight );
		lastBilateralWeight = bilateralWeight;
		weight *= bilateralWeight;
		sum += value * weight;
		totalWeight += weight;
	}
	
	lastBilateralWeight = 9999.0;
	for( int r = 1; r <= R; ++r )
	{
		int2 tapLoc = ssC + int2( rpJitterTexScale.xy ) * ( r * SCALE );
		temp = texelFetch( source, tapLoc, 0 );
		float      tapKey = getKey( tapLoc );
		VALUE_TYPE value  = temp.VALUE_COMPONENTS;
	
		// spatial domain: offset kernel tap
		float weight = 0.3 + kernel[abs( r )];
	
		// range domain (the "bilateral" weight). As depth difference increases, decrease weight.
		float bilateralWeight = calculateBilateralWeight( key, tapKey, tapLoc, n_C, C );
		bilateralWeight = min( lastBilateralWeight, bilateralWeight );
		lastBilateralWeight = bilateralWeight;
		weight *= bilateralWeight;
		sum += value * weight;
		totalWeight += weight;
	}
#endif
	
	const float epsilon = 0.0001;
	aoResult = sum / ( totalWeight + epsilon );
	
#if defined(BRIGHTPASS)
	result.color = float4( aoResult, aoResult, aoResult, 1.0 );
#endif
}
