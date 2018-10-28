/**
 \file AmbientOcclusion_AO.pix
 \author Morgan McGuire and Michael Mara, NVIDIA Research

 Reference implementation of the Scalable Ambient Obscurance (AmbientOcclusion) screen-space ambient obscurance algorithm.

 The optimized algorithmic structure of AmbientOcclusion was published in McGuire, Mara, and Luebke, Scalable Ambient Obscurance,
 <i>HPG</i> 2012, and was developed at NVIDIA with support from Louis Bavoil.

 The mathematical ideas of AlchemyAO were first described in McGuire, Osman, Bukowski, and Hennessy, The
 Alchemy Screen-Space Ambient Obscurance Algorithm, <i>HPG</i> 2011 and were developed at
 Vicarious Visions.

 DX11 HLSL port by Leonardo Zide of Treyarch

 <hr>

  Open Source under the "BSD" license: http://www.opensource.org/licenses/bsd-license.php

  Copyright (c) 2011-2012, NVIDIA
  Copyright (c) 2016 Robert Beckebans ( id Tech 4.x integration )
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

  Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
  Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  */

#include "global.inc.hlsl"



#define DIFFERENT_DEPTH_RESOLUTIONS 0
#define USE_DEPTH_PEEL 0
#define CS_Z_PACKED_TOGETHER 0
#define TEMPORALLY_VARY_TAPS 0
#define HIGH_QUALITY 1
#define USE_OCT16 0
#define USE_MIPMAPS 1

// Total number of direct samples to take at each pixel
#define NUM_SAMPLES 11

// This is the number of turns around the circle that the spiral pattern makes.  This should be prime to prevent
// taps from lining up.  This particular choice was tuned for NUM_SAMPLES == 9
#define NUM_SPIRAL_TURNS 7

// If using depth mip levels, the log of the maximum pixel offset before we need to switch to a lower
// miplevel to maintain reasonable spatial locality in the cache
// If this number is too small (< 3), too many taps will land in the same pixel, and we'll get bad variance that manifests as flashing.
// If it is too high (> 5), we'll get bad performance because we're not using the MIP levels effectively
#define LOG_MAX_OFFSET (3)

// This must be less than or equal to the MAX_MIP_LEVEL defined in SAmbientOcclusion.cpp
#define MAX_MIP_LEVEL (5)

#define MIN_MIP_LEVEL 0

const float DOOM_TO_METERS = 0.0254;					// doom to meters
const float METERS_TO_DOOM = ( 1.0 / DOOM_TO_METERS );	// meters to doom

/** Used for preventing AO computation on the sky (at infinite depth) and defining the CS Z to bilateral depth key scaling.
    This need not match the real far plane but should not be much more than it.*/
//const float FAR_PLANE_Z = -4000.0;

/** World-space AO radius in scene units (r).  e.g., 1.0m */
const float radius = 1.0 * METERS_TO_DOOM;
const float radius2 = radius * radius;
const float invRadius2 = 1.0 / radius2;

/** Bias to avoid AO in smooth corners, e.g., 0.01m */
const float bias = 0.01 * METERS_TO_DOOM;

/** intensity / radius^6 */
const float intensity = 0.6;
const float intensityDivR6 = intensity / ( radius* radius* radius* radius* radius* radius );

/** The height in pixels of a 1m object if viewed from 1m away.
    You can compute it from your projection matrix.  The actual value is just
    a scale factor on radius; you can simply hardcode this to a constant (~500)
    and make your radius value unitless (...but resolution dependent.)  */
const float projScale = 500.0;

//#expect NUM_SAMPLES "Integer number of samples to take at each pixels"
//#expect NUM_SPIRAL_TURNS "Integer number of turns around the circle that the spiral pattern makes. The G3D::AmbientOcclusion class provides a discrepancy-minimizing value of NUM_SPIRAL_TURNS for eac value of NUM_SAMPLES."
//#expect DIFFERENT_DEPTH_RESOLUTIONS "1 if the peeled depth buffer is at a different resolution than the primary depth buffer"
//#expect USE_DEPTH_PEEL "1 to enable, 0 to disable"
//#expect CS_Z_PACKED_TOGETHER "1 to enable, 0 to disable"
//#expect TEMPORALLY_VARY_SAMPLES "1 to enable, 0 to disable"

// *INDENT-OFF*
uniform sampler2D samp0 : register( s0 ); // view normal/roughness
uniform sampler2D samp1 : register( s1 ); // view depth
 
#define CS_Z_buffer		samp1
 
struct PS_IN
{
	float2 texcoord0 : TEXCOORD0_centroid;
};
 
struct PS_OUT 
{
	float4 color : COLOR;
};
// *INDENT-ON*




/** Used for packing Z into the GB channels */
// float CSZToKey( float z )
// {
// return clamp( z * ( 1.0 / FAR_PLANE_Z ), 0.0, 1.0 );
// }

/** Used for packing Z into the GB channels */
void packKey( float key, out float2 p )
{
	// Round to the nearest 1/256.0
	float temp = floor( key * 256.0 );
	
	// Integer part
	p.x = temp * ( 1.0 / 256.0 );
	
	// Fractional part
	p.y = key * 256.0 - temp;
}

/** Reconstructs screen-space unit normal from screen-space position */
float3 reconstructCSFaceNormal( float3 C )
{
	return normalize( cross( dFdy( C ), dFdx( C ) ) );
}

float3 reconstructNonUnitCSFaceNormal( float3 C )
{
	float3 n = cross( dFdy( C ), dFdx( C ) );
	//n.z = sqrt( abs( dot( n.xy, n.xy ) - 0.25 ) );
	return n;
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

float3 sampleNormal( sampler2D normalBuffer, int2 ssC, int mipLevel )
{
#if USE_OCT16
	return decode16( texelFetch( normalBuffer, ssC, mipLevel ).xy * 2.0 - 1.0 );
#else
	return texelFetch( normalBuffer, ssC, mipLevel ).xyz * 2.0 - 1.0;
#endif
}

/** Returns a unit vector and a screen-space radius for the tap on a unit disk
    (the caller should scale by the actual disk radius) */
float2 tapLocation( int sampleNumber, float spinAngle, out float ssR )
{
	// Radius relative to ssR
	float alpha = ( float( sampleNumber ) + 0.5 ) * ( 1.0 / float( NUM_SAMPLES ) );
	float angle = alpha * ( float( NUM_SPIRAL_TURNS ) * 6.28 ) + spinAngle;
	
	ssR = alpha;
	return float2( cos( angle ), sin( angle ) );
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

void computeMipInfo( float ssR, int2 ssP, sampler2D cszBuffer, out int mipLevel, out int2 mipP )
{
	// Derivation:
	//  mipLevel = floor(log(ssR / MAX_OFFSET));
#ifdef GL_EXT_gpu_shader5
	mipLevel = clamp( findMSB( int( ssR ) ) - LOG_MAX_OFFSET, 0, MAX_MIP_LEVEL );
#else
	mipLevel = clamp( int( floor( log2( ssR ) ) ) - LOG_MAX_OFFSET, 0, MAX_MIP_LEVEL );
#endif
	
	// We need to divide by 2^mipLevel to read the appropriately scaled coordinate from a MIP-map.
	// Manually clamp to the texture size because texelFetch bypasses the texture unit
	
	// used in newer radiosity
	//mipP = ssP >> mipLevel;
	
	mipP = clamp( ssP >> mipLevel, int2( 0 ), textureSize( cszBuffer, mipLevel ) - int2( 1 ) );
}

/** Read the camera-space position of the point at screen-space pixel ssP + unitOffset * ssR.  Assumes length(unitOffset) == 1.
    Use cszBufferScale if reading from the peeled depth buffer, which has been scaled by (1 / invCszBufferScale) from the original */
float3 getOffsetPosition( int2 issC, float2 unitOffset, float ssR, sampler2D cszBuffer, float invCszBufferScale )
{
	int2 ssP = int2( ssR * unitOffset ) + issC;
	
	float3 P;
	
	int mipLevel;
	int2 mipP;
	computeMipInfo( ssR, ssP, cszBuffer, mipLevel, mipP );
	
#if USE_MIPMAPS
	// RB: this is the key for fast ambient occlusion - use a hierarchical depth buffer
	// for more information see McGuire12SAO.pdf - Scalable Ambient Obscurance
	// http://graphics.cs.williams.edu/papers/SAOHPG12/
	P.z = texelFetch( cszBuffer, mipP, mipLevel ).r;
#else
	P.z = texelFetch( cszBuffer, ssP, 0 ).r;
#endif
	
	// Offset to pixel center
	P = reconstructCSPosition( float2( ssP ) + float2( 0.5 ), P.z );
	//P = reconstructCSPosition( ( float2( ssP ) + float2( 0.5 ) ) * invCszBufferScale, P.z );
	
	return P;
}

float fallOffFunction( float vv, float vn, float epsilon )
{
	// A: From the HPG12 paper
	// Note large epsilon to avoid overdarkening within cracks
	//  Assumes the desired result is intensity/radius^6 in main()
	// return float(vv < radius2) * max((vn - bias) / (epsilon + vv), 0.0) * radius2 * 0.6;
	
	// B: Smoother transition to zero (lowers contrast, smoothing out corners). [Recommended]
#if HIGH_QUALITY
	// Epsilon inside the sqrt for rsqrt operation
	float f = max( 1.0 - vv * invRadius2, 0.0 );
	return f * max( ( vn - bias ) * rsqrt( epsilon + vv ), 0.0 );
#else
	// Avoid the square root from above.
	//  Assumes the desired result is intensity/radius^6 in main()
	float f = max( radius2 - vv, 0.0 );
	return f * f * f * max( ( vn - bias ) / ( epsilon + vv ), 0.0 );
#endif
	
	// C: Medium contrast (which looks better at high radii), no division.  Note that the
	// contribution still falls off with radius^2, but we've adjusted the rate in a way that is
	// more computationally efficient and happens to be aesthetically pleasing.  Assumes
	// division by radius^6 in main()
	//return 4.0 * max(1.0 - vv * invRadius2, 0.0) * max(vn - bias, 0.0);
	
	// D: Low contrast, no division operation
	//return 2.0 * float(vv < radius * radius) * max(vn - bias, 0.0);
}

/** Compute the occlusion due to sample point \a Q about camera-space point \a C with unit normal \a n_C */
float aoValueFromPositionsAndNormal( float3 C, float3 n_C, float3 Q )
{
	float3 v = Q - C;
	//v = normalize( v );
	float vv = dot( v, v );
	float vn = dot( v, n_C );
	const float epsilon = 0.001;
	
	// Without the angular adjustment term, surfaces seen head on have less AO
	return fallOffFunction( vv, vn, epsilon ) * lerp( 1.0, max( 0.0, 1.5 * n_C.z ), 0.35 );
}


/** Compute the occlusion due to sample with index \a i about the pixel at \a ssC that corresponds
    to camera-space point \a C with unit normal \a n_C, using maximum screen-space sampling radius \a ssDiskRadius

    Note that units of H() in the HPG12 paper are meters, not
    unitless.  The whole falloff/sampling function is therefore
    unitless.  In this implementation, we factor out (9 / radius).

    Four versions of the falloff function are implemented below

    When sampling from the peeled depth buffer, make sure ssDiskRadius has been premultiplied by cszBufferScale
*/
float sampleAO( int2 issC, in float3 C, in float3 n_C, in float ssDiskRadius, in int tapIndex, in float randomPatternRotationAngle, in sampler2D cszBuffer, in float invCszBufferScale )
{
	// Offset on the unit disk, spun for this pixel
	float ssR;
	float2 unitOffset = tapLocation( tapIndex, randomPatternRotationAngle, ssR );
	
	// Ensure that the taps are at least 1 pixel away
	ssR = max( 0.75, ssR * ssDiskRadius );
	
#if (CS_Z_PACKED_TOGETHER != 0)
	vec3 Q0, Q1;
	getOffsetPositions( ssC, unitOffset, ssR, cszBuffer, Q0, Q1 );
	
	return max( aoValueFromPositionsAndNormal( C, n_C, Q0 ), aoValueFromPositionsAndNormal( C, n_C, Q1 ) );
#else
	// The occluding point in camera space
	vec3 Q = getOffsetPosition( issC, unitOffset, ssR, cszBuffer, invCszBufferScale );
	
	return aoValueFromPositionsAndNormal( C, n_C, Q );
#endif
}

const float MIN_RADIUS = 3.0; // pixels

#define visibility      result.color.r
#define bilateralKey    result.color.gb

void main( PS_IN fragment, out PS_OUT result )
{
	result.color = float4( 1.0, 0.0, 0.0, 1.0 );
	
#if 0
	if( fragment.texcoord0.x < 0.5 )
	{
		discard;
	}
#endif
	
	// Pixel being shaded
	//float2 ssC = fragment.texcoord0;
	//int2 issC = int2( ssC.x * rpScreenCorrectionFactor.z, ssC.y * rpScreenCorrectionFactor.w );
	
	int2 ssP = int2( gl_FragCoord.xy );
	
	// World space point being shaded
	vec3 C = getPosition( ssP, CS_Z_buffer );
	
	//float z = length( C - rpGlobalEyePos.xyz );
	//bilateralKey = CSZToKey( C.z );
	//packKey( CSZToKey( C.z ), bilateralKey );
	
	//float key = CSZToKey( C.z );
	
#if 0
	if( key >= 1.0 )
	{
		visibility = 0.0;
		return;
	}
#endif
	
	visibility = 0.0;
	
#if 1
	float3 n_C = sampleNormal( samp0, ssP, 0 );
	
	if( length( n_C ) < 0.01 )
	{
		visibility = 1.0;
		return;
	}
	
	n_C = normalize( n_C );
	//n_C = -n_C;
	
#else
	// Reconstruct normals from positions.
	float3 n_C = reconstructNonUnitCSFaceNormal( C );
	// Since n_C is computed from the cross product of camera-space edge vectors from points at adjacent pixels, its magnitude will be proportional to the square of distance from the camera
	if( dot( n_C, n_C ) > ( square( C.z * C.z * 0.00006 ) ) ) // if the threshold # is too big you will see black dots where we used a bad normal at edges, too small -> white
	{
		// The normals from depth should be very small values before normalization,
		// except at depth discontinuities, where they will be large and lead
		// to 1-pixel false occlusions because they are not reliable
		visibility = 1.0;
		//result.color = float4( visibility, visibility, visibility, 1.0 );
		return;
	}
	else
	{
		n_C = normalize( -n_C );
	}
#endif
	
	// Hash function used in the HPG12 AlchemyAO paper
	float randomPatternRotationAngle = float( ( ( 3 * ssP.x ) ^ ( ssP.y + ssP.x * ssP.y ) )
#if TEMPORALLY_VARY_TAPS
									   + rpJitterTexOffset.x
#endif
											) * 10.0;
											
	// Choose the screen-space sample radius
	// proportional to the projected area of the sphere
	float ssDiskRadius = -projScale * radius / C.z;
	
#if 1
	if( ssDiskRadius <= MIN_RADIUS )
	{
		// There is no way to compute AO at this radius
		visibility = 1.0;
		return;
	}
#endif
	
#if USE_DEPTH_PEEL == 1
#if DIFFERENT_DEPTH_RESOLUTIONS == 1
	float unpeeledToPeeledScale = 1.0 / peeledToUnpeeledScale;
#endif
#endif
	
	float sum = 0.0;
	for( int i = 0; i < NUM_SAMPLES; ++i )
	{
		sum += sampleAO( ssP, C, n_C, ssDiskRadius, i, randomPatternRotationAngle, CS_Z_buffer, 1.0 );
	}
	
#if HIGH_QUALITY
	float A = pow( max( 0.0, 1.0 - sqrt( sum * ( 3.0 / float( NUM_SAMPLES ) ) ) ), intensity );
#else
	float A = max( 0.0, 1.0 - sum * intensityDivR6 * ( 5.0 / float( NUM_SAMPLES ) ) );
	// Anti-tone map to reduce contrast and drag dark region farther
	// (x^0.2 + 1.2 * x^4)/2.2
	//A = ( pow( A, 0.2 ) + 1.2 * A * A * A * A ) / 2.2;
#endif
	
	// Visualize random spin distribution
	//A = mod(randomPatternRotationAngle / (2 * 3.141592653589), 1.0);
	
	// Fade in as the radius reaches 2 pixels
	visibility = lerp( 1.0, A, saturate( ssDiskRadius - MIN_RADIUS ) );
	//visibility = A;
	
#if defined(BRIGHTPASS)
	//result.color = float4( visibility, bilateralKey, 0.0, 1.0 );
	//result.color = float4( bilateralKey, bilateralKey, bilateralKey, 1.0 );
	result.color = float4( visibility, visibility, visibility, 1.0 );
	//result.color = float4( n_C * 0.5 + 0.5, 1.0 );
	//result.color = float4( n_C, 1.0 );
	//result.color = texture( samp0, fragment.texcoord0 ).rgba;
#endif
}
