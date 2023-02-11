/*
* Copyright (c) 2014-2021, NVIDIA CORPORATION. All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*/

#pragma pack_matrix(row_major)

#include <global_inc.hlsl>

struct SsaoConstants
{
	float2		viewportOrigin;
	float2		viewportSize;
	float2		pixelOffset;
	float2		unused; // padding

	float4x4	matClipToView;
	float4x4	matWorldToView;
	float4x4	matViewToWorld;

	float2      clipToView;
	float2      invQuantizedGbufferSize;

	int2        quantizedViewportOrigin;
	float       amount;
	float       invBackgroundViewDepth;
	float       radiusWorld;
	float       surfaceBias;

	float       radiusToScreen;
	float       powerExponent;
};

// *INDENT-OFF*
Texture2DArray<float> t_DeinterleavedDepth : register(t0);
#if OCT_ENCODED_NORMALS
Texture2D<uint> t_Normals : register(t1);
#else
Texture2D<float4> t_Normals : register(t1);
#endif
#if DIRECTIONAL_OCCLUSION
RWTexture2DArray<float4> u_RenderTarget : register(u0);
#else
RWTexture2DArray<float> u_RenderTarget : register(u0);
#endif

cbuffer c_Ssao : register(b1)
{
    SsaoConstants g_Ssao;
};
// *INDENT-ON*

// Set of samples with distance from center increasing linearly,
// and angle also increasing linearly with a step of 4.5678 radians.
// Plotted on x-y dimensions, it looks pretty much random, but is intended
// to make more samples closer to the center because they have greater weight.
static const float2 g_SamplePositions[] =
{
	float2( -0.016009523, -0.10995169 ),
	float2( -0.159746436, 0.047527402 ),
	float2( 0.09339819, 0.201641995 ),
	float2( 0.232600698, 0.151846663 ),
	float2( -0.220531935, -0.24995355 ),
	float2( -0.251498143, 0.29661971 ),
	float2( 0.376870668, .23558303 ),
	float2( 0.201175979, 0.457742532 ),
	float2( -0.535502966, -0.147913991 ),
	float2( -0.076133435, 0.606350138 ),
	float2( 0.666537538, 0.013120791 ),
	float2( -0.118107615, -0.712499494 ),
	float2( -0.740973793, 0.236423582 ),
	float2( 0.365057451, .749117816 ),
	float2( 0.734614792, 0.500464349 ),
	float2( -0.638657704, -0.695766948 )
};

// Blue noise
static const float g_RandomValues[16] =
{
	0.059, 0.529, 0.176, 0.647,
	0.765, 0.294, 0.882, 0.412,
	0.235, 0.706, 0.118, 0.588,
	0.941, 0.471, 0.824, 0.353
};

// Helper function to reflect the folds of the lower hemisphere
// over the diagonals in the octahedral map
float2 octWrap( float2 v )
{
	return ( 1.f - abs( v.yx ) ) * ( v.xy >= 0.f ? 1.f : -1.f );
}

/**********************/
// Signed encodings
// Converts a normalized direction to the octahedral map (non-equal area, signed)
// n - normalized direction
// Returns a signed position in octahedral map [-1, 1] for each component
float2 ndirToOctSigned( float3 n )
{
	// Project the sphere onto the octahedron (|x|+|y|+|z| = 1) and then onto the xy-plane
	float2 p = n.xy * ( 1.f / ( abs( n.x ) + abs( n.y ) + abs( n.z ) ) );
	return ( n.z < 0.f ) ? octWrap( p ) : p;
}

// Converts a point in the octahedral map to a normalized direction (non-equal area, signed)
// p - signed position in octahedral map [-1, 1] for each component
// Returns normalized direction
float3 octToNdirSigned( float2 p )
{
	// https://twitter.com/Stubbesaurus/status/937994790553227264
	float3 n = float3( p.x, p.y, 1.0 - abs( p.x ) - abs( p.y ) );
	float t = max( 0, -n.z );
	n.xy += n.xy >= 0.0 ? -t : t;
	return normalize( n );
}

/**********************/
// Unorm 32 bit encodings
// Converts a normalized direction to the octahedral map (non-equal area, unsigned normalized)
// n - normalized direction
// Returns a packed 32 bit unsigned normalized position in octahedral map
// The two components of the result are stored in UNORM16 format, [0..1]
uint ndirToOctUnorm32( float3 n )
{
	float2 p = ndirToOctSigned( n );
	p = saturate( p.xy * 0.5 + 0.5 );
	return uint( p.x * 0xfffe ) | ( uint( p.y * 0xfffe ) << 16 );
}

// Converts a point in the octahedral map (non-equal area, unsigned normalized) to normalized direction
// pNorm - a packed 32 bit unsigned normalized position in octahedral map
// Returns normalized direction
float3 octToNdirUnorm32( uint pUnorm )
{
	float2 p;
	p.x = saturate( float( pUnorm & 0xffff ) / 0xfffe );
	p.y = saturate( float( pUnorm >> 16 ) / 0xfffe );
	p = p * 2.0 - 1.0;
	return octToNdirSigned( p );
}

// Project the direction onto the first two spherical harmonics.
// The coefficients like sqrt(1/4pi) have been dropped because they
// cancel out at interpolation stage.
// Multiply the output of this function by the signal magnitude.
float4 directionToSphericalHarmonics( float3 normalizedDirection )
{
	return float4( normalizedDirection, 1.0 );
}

// V = unnormalized vector from center pixel to current sample
// N = normal at center pixel
float ComputeAO( float3 V, float3 N, float InvR2 )
{
	float VdotV = dot( V, V );
	float NdotV = dot( N, V ) * rsqrt( VdotV );
	float lambertian = saturate( NdotV - g_Ssao.surfaceBias );
	float falloff = saturate( 1 - VdotV * InvR2 );
	return saturate( lambertian * falloff * g_Ssao.amount );
}

float2 WindowToClip( float2 windowPos )
{
	float2 clipToWindowScale = float2( 0.5 * g_Ssao.viewportSize.x, -0.5 * g_Ssao.viewportSize.y );
	float2 clipToWindowBias = g_Ssao.viewportOrigin.xy + g_Ssao.viewportSize.xy * 0.5;

	float2 windowToClipScale = 1.0 / clipToWindowScale;
	float2 windowToClipBias = -clipToWindowBias * windowToClipScale;

	//return windowPos.xy * windowToClipScale + windowToClipBias;
	return ( windowPos.xy + g_Ssao.pixelOffset.xy ) * windowToClipScale + windowToClipBias;
}

float3 ViewDepthToViewPos( float2 clipPosXY, float viewDepth )
{
	return float3( clipPosXY * g_Ssao.clipToView.xy * viewDepth, viewDepth );
}

[numthreads( 8, 8, 1 )]
void main( uint3 globalId : SV_DispatchThreadID )
{
	int sliceIndex = int( globalId.z );
	int2 sliceOffset = int2( sliceIndex & 3, sliceIndex >> 2 );

	int2 pixelPos = ( int2( globalId.xy ) << 2 ) + sliceOffset + g_Ssao.quantizedViewportOrigin;
	int2 quarterResPixelPos = pixelPos >> 2;

	float pixelViewDepth = t_DeinterleavedDepth[int3( quarterResPixelPos, sliceIndex )];
#if OCT_ENCODED_NORMALS
	float3 pixelNormal = octToNdirUnorm32( t_Normals[pixelPos] );
#else
	float3 pixelNormal = t_Normals[pixelPos].xyz;
#endif

	// RB: pixelNormal is already in view space but it has to be negated to look correct which is weird
	pixelNormal = -normalize( pixelNormal * 2.0 - 1.0 );
	//pixelNormal = normalize( mul( float4( pixelNormal, 0 ), g_Ssao.matWorldToView ).xyz );

	float2 pixelClipPos = WindowToClip( pixelPos );
	float3 pixelViewPos = ViewDepthToViewPos( pixelClipPos.xy, pixelViewDepth );

	float radiusWorld = g_Ssao.radiusWorld * max( 1.0, pixelViewDepth * g_Ssao.invBackgroundViewDepth );
	float radiusPixels = radiusWorld * g_Ssao.radiusToScreen / pixelViewDepth;

#if DIRECTIONAL_OCCLUSION
	float4 result = 0;
#else
	float result = 0;
#endif

	if( radiusPixels > 1 )
	{
		float invRadiusWorld2 = rcp( radiusWorld * radiusWorld );

		float angle = g_RandomValues[( pixelPos.x & 3 ) + ( ( pixelPos.y & 3 ) << 2 )] * PI;
		float2 sincos = float2( sin( angle ), cos( angle ) );

		int numSamples = 16;
		float numValidSamples = 0;

		[unroll]
		for( int nSample = 0; nSample < numSamples; nSample++ )
		{
			float2 sampleOffset = g_SamplePositions[nSample];
			sampleOffset = float2(
							   sampleOffset.x * sincos.y - sampleOffset.y * sincos.x,
							   sampleOffset.x * sincos.x + sampleOffset.y * sincos.y );

			float2 sampleWindowPos = pixelPos + sampleOffset * radiusPixels + 0.5;
			int2 sampleWindowPosInt = int2( floor( sampleWindowPos * 0.25 ) );

			float sampleViewDepth = t_DeinterleavedDepth[int3( sampleWindowPosInt, sliceIndex )];
			float2 actualClipPos = WindowToClip( float2( sampleWindowPosInt ) * 4.0 + sliceOffset + 0.5 );

			if( sampleViewDepth > 0 && any( quarterResPixelPos != sampleWindowPosInt ) && all( abs( actualClipPos.xy ) < 1.0 ) )
			{
				float3 sampleViewPos = ViewDepthToViewPos( actualClipPos, sampleViewDepth );
				float3 pixelToSample = sampleViewPos - pixelViewPos;
				float AO = ComputeAO( pixelToSample, pixelNormal, invRadiusWorld2 );
#if DIRECTIONAL_OCCLUSION
				result += directionToSphericalHarmonics( normalize( pixelToSample ) ) * AO;
#else
				result += AO;
#endif
				numValidSamples += 1;
			}
		}

		if( numValidSamples > 0 )
		{
			result /= numValidSamples;
		}
	}

#if DIRECTIONAL_OCCLUSION
	// Rotate the directional part of the SH into world space
	float directionalLength = length( result.xyz );
	if( directionalLength > 0 )
	{
		result.xyz = mul( float4( normalize( result.xyz ), 0 ), g_Ssao.matViewToWorld ).xyz * directionalLength;
	}
#endif

	u_RenderTarget[int3( quarterResPixelPos.xy, sliceIndex )] = result;
}
