/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2021 Robert Beckebans
Copyright (C) 2020 Panos Karabelas

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

#include "global_inc.hlsl"
#include "BRDF.inc.hlsl"


// *INDENT-OFF*
Texture2D				t_Normal			: register( t0 VK_DESCRIPTOR_SET( 1 ) );
Texture2D				t_Specular			: register( t1 VK_DESCRIPTOR_SET( 1 ) );
Texture2D				t_BaseColor			: register( t2 VK_DESCRIPTOR_SET( 1 ) );

Texture2D				t_LightFalloff		: register( t3 VK_DESCRIPTOR_SET( 2 ) );
Texture2D				t_LightProjection	: register( t4 VK_DESCRIPTOR_SET( 2 ) );
#if USE_SHADOW_ATLAS
Texture2D				t_ShadowAtlas		: register( t5 VK_DESCRIPTOR_SET( 2 ) );
#else
Texture2DArray<float>	t_ShadowMapArray	: register( t5 VK_DESCRIPTOR_SET( 2 ) );
#endif
Texture2D				t_Jitter			: register( t6 VK_DESCRIPTOR_SET( 2 ) );

SamplerState			s_Material : register( s0 VK_DESCRIPTOR_SET( 3 ) ); // for the normal/specular/basecolor
SamplerState 			s_Lighting : register( s1 VK_DESCRIPTOR_SET( 3 ) ); // for sampling the jitter
SamplerComparisonState  s_Shadow   : register( s2 VK_DESCRIPTOR_SET( 3 ) ); // for the depth shadow map sampler with a compare function
SamplerState 			s_Jitter   : register( s3 VK_DESCRIPTOR_SET( 3 ) ); // for sampling the jitter

struct PS_IN
{
	float4 position		: SV_Position;
	float4 texcoord0	: TEXCOORD0_centroid;
	float4 texcoord1	: TEXCOORD1_centroid;
	float4 texcoord2	: TEXCOORD2_centroid;
	float4 texcoord3	: TEXCOORD3_centroid;
	float4 texcoord4	: TEXCOORD4_centroid;
	float4 texcoord5	: TEXCOORD5_centroid;
	float4 texcoord6	: TEXCOORD6_centroid;
	float4 texcoord7	: TEXCOORD7_centroid;
	float4 texcoord8	: TEXCOORD8_centroid;
	float4 texcoord9	: TEXCOORD9_centroid;
	float4 color		: COLOR0;
};

struct PS_OUT
{
	float4 color : SV_Target0;
};
// *INDENT-ON*

float BlueNoise( float2 n, float x )
{
	float2 uv = n.xy * rpJitterTexOffset.xy;

	float noise = t_Jitter.Sample( s_Jitter, uv ).r;

	noise = frac( noise + c_goldenRatioConjugate * rpJitterTexOffset.w * x );

	//noise = RemapNoiseTriErp( noise );
	//noise = noise * 2.0 - 0.5;

	return noise;
}

float2 VogelDiskSample( float sampleIndex, float samplesCount, float phi )
{
	float goldenAngle = 2.4f;

	float r = sqrt( sampleIndex + 0.5f ) / sqrt( samplesCount );
	float theta = sampleIndex * goldenAngle + phi;

	float sine = sin( theta );
	float cosine = cos( theta );

	return float2( r * cosine, r * sine );
}

void main( PS_IN fragment, out PS_OUT result )
{
	float4 bumpMap =		t_Normal.Sample( s_Material, fragment.texcoord1.xy );
	float4 lightFalloff =	idtex2Dproj( s_Lighting, t_LightFalloff, fragment.texcoord2 );
	float4 lightProj =		idtex2Dproj( s_Lighting, t_LightProjection, fragment.texcoord3 );
	float4 YCoCG =			t_BaseColor.Sample( s_Material, fragment.texcoord4.xy );
	float4 specMapSRGB =	t_Specular.Sample( s_Material, fragment.texcoord5.xy );
	float4 specMap =		sRGBAToLinearRGBA( specMapSRGB );

	float3 lightVector = normalize( fragment.texcoord0.xyz );
	float3 viewVector = normalize( fragment.texcoord6.xyz );
	float3 diffuseMap = sRGBToLinearRGB( ConvertYCoCgToRGB( YCoCG ) );

	float3 localNormal;
	// RB begin
#if USE_NORMAL_FMT_RGB8
	localNormal.xy = bumpMap.rg - 0.5;
#else
	localNormal.xy = bumpMap.wy - 0.5;
#endif
	// RB end
	localNormal.z = sqrt( abs( dot( localNormal.xy, localNormal.xy ) - 0.25 ) );
	localNormal = normalize( localNormal );

	// traditional very dark Lambert light model used in Doom 3
	float ldotN = saturate( dot3( localNormal, lightVector ) );

#if defined(USE_HALF_LAMBERT)
	// RB: http://developer.valvesoftware.com/wiki/Half_Lambert
	float halfLdotN = dot3( localNormal, lightVector ) * 0.5 + 0.5;
	halfLdotN *= halfLdotN;

	// tweak to not loose so many details
	float lambert = lerp( ldotN, halfLdotN, 0.5 );
#else
	float lambert = ldotN;
#endif


	//
	// shadow mapping
	//
	int shadowIndex = 0;

#if LIGHT_POINT
	float3 toLightGlobal = normalize( fragment.texcoord8.xyz );

	float axis[6];
	axis[0] = -toLightGlobal.x;
	axis[1] =  toLightGlobal.x;
	axis[2] = -toLightGlobal.y;
	axis[3] =  toLightGlobal.y;
	axis[4] = -toLightGlobal.z;
	axis[5] =  toLightGlobal.z;

	for( int i = 0; i < 6; i++ )
	{
		if( axis[i] > axis[shadowIndex] )
		{
			shadowIndex = i;
		}
	}

#endif // #if defined( LIGHT_POINT )

#if LIGHT_PARALLEL

	float viewZ = -fragment.texcoord9.z;

	shadowIndex = 4;
	for( int ci = 0; ci < 4; ci++ )
	{
		if( viewZ < rpCascadeDistances[ci] )
		{
			shadowIndex = ci;
			break;
		}
	}
#endif

#if 0
	if( shadowIndex == 0 )
	{
		result.color = float4( 1.0, 0.0, 0.0, 1.0 );
	}
	else if( shadowIndex == 1 )
	{
		result.color = float4( 0.0, 1.0, 0.0, 1.0 );
	}
	else if( shadowIndex == 2 )
	{
		result.color = float4( 0.0, 0.0, 1.0, 1.0 );
	}
	else if( shadowIndex == 3 )
	{
		result.color = float4( 1.0, 1.0, 0.0, 1.0 );
	}
	else if( shadowIndex == 4 )
	{
		result.color = float4( 1.0, 0.0, 1.0, 1.0 );
	}
	else if( shadowIndex == 5 )
	{
		result.color = float4( 0.0, 1.0, 1.0, 1.0 );
	}

	//result.color.xyz *= lightColor;
	return;
#endif

	// rpShadowMatrices contain model -> world -> shadow transformation for evaluation
	float4 shadowMatrixX = rpShadowMatrices[ int ( shadowIndex * 4 + 0 ) ];
	float4 shadowMatrixY = rpShadowMatrices[ int ( shadowIndex * 4 + 1 ) ];
	float4 shadowMatrixZ = rpShadowMatrices[ int ( shadowIndex * 4 + 2 ) ];
	float4 shadowMatrixW = rpShadowMatrices[ int ( shadowIndex * 4 + 3 ) ];

	float4 modelPosition = float4( fragment.texcoord7.xyz, 1.0 );

	float4 shadowTexcoord;
	shadowTexcoord.x = dot4( modelPosition, shadowMatrixX );
	shadowTexcoord.y = dot4( modelPosition, shadowMatrixY );
	shadowTexcoord.z = dot4( modelPosition, shadowMatrixZ );
	shadowTexcoord.w = dot4( modelPosition, shadowMatrixW );

	//float bias = 0.005 * tan( acos( ldotN ) );
	//bias = clamp( bias, 0, 0.01 );
	float bias = 0.001;

	shadowTexcoord.xyz /= shadowTexcoord.w;

	// receiver / occluder terminology like in ESM
	float receiver = shadowTexcoord.z * rpScreenCorrectionFactor.w;
	//shadowTexcoord.z = shadowTexcoord.z * 0.999991;
	//shadowTexcoord.z = shadowTexcoord.z - bias;
	shadowTexcoord.w = float( shadowIndex );

	// multiple taps

#if 0
	float4 base = shadowTexcoord;

	base.xy += rpJitterTexScale.xy * -0.5;

	float shadow = 0.0;

	//float stepSize = 1.0 / 16.0;
	float numSamples = 16;
	float stepSize = 1.0 / numSamples;

	float2 jitterTC = ( fragment.position.xy * rpScreenCorrectionFactor.xy ) + rpJitterTexOffset.ww;
	for( float n = 0.0; n < numSamples; n += 1.0 )
	{
		float4 jitter = base + t_Jitter.Sample( samp1, jitterTC.xy ) * rpJitterTexScale;
		jitter.zw = shadowTexcoord.zw;

		shadow += t_Jitter.Sample( samp1, jitter.xy / jitter.z ).r;
		jitterTC.x += stepSize;
	}

	shadow *= stepSize;


#elif 0

	// Poisson Disk with White Noise used for years int RBDOOM-3-BFG

	const float2 poissonDisk[12] =
	{
		float2( 0.6111618, 0.1050905 ),
		float2( 0.1088336, 0.1127091 ),
		float2( 0.3030421, -0.6292974 ),
		float2( 0.4090526, 0.6716492 ),
		float2( -0.1608387, -0.3867823 ),
		float2( 0.7685862, -0.6118501 ),
		float2( -0.1935026, -0.856501 ),
		float2( -0.4028573, 0.07754025 ),
		float2( -0.6411021, -0.4748057 ),
		float2( -0.1314865, 0.8404058 ),
		float2( -0.7005203, 0.4596822 ),
		float2( -0.9713828, -0.06329931 )
	};

	float shadow = 0.0;

	// RB: casting a float to int and using it as index can really kill the performance ...
	float numSamples = 12.0; //int(rpScreenCorrectionFactor.w);
	float stepSize = 1.0 / numSamples;

	float4 jitterTC = ( fragment.position * rpScreenCorrectionFactor ) + rpJitterTexOffset;
	float4 random = t_Jitter.Sample( s_Jitter, jitterTC.xy ) * PI;
	//float4 random = fragment.position;

	float2 rot;
	rot.x = cos( random.x );
	rot.y = sin( random.x );

	float shadowTexelSize = rpScreenCorrectionFactor.z * rpJitterTexScale.x;
	for( int i = 0; i < 12; i++ )
	{
		float2 jitter = poissonDisk[i];
		float2 jitterRotated;
		jitterRotated.x = jitter.x * rot.x - jitter.y * rot.y;
		jitterRotated.y = jitter.x * rot.y + jitter.y * rot.x;

		float4 shadowTexcoordJittered = float4( shadowTexcoord.xy + jitterRotated * shadowTexelSize, shadowTexcoord.z, shadowTexcoord.w );

		shadow += idtex2Dproj( samp1, t_ShadowMapArray, shadowTexcoordJittered.xywz ).r;
	}

	shadow *= stepSize;

#elif 1

#if 0

	// Poisson Disk with animated Blue Noise or Interleaved Gradient Noise

	const float2 poissonDisk[12] =
	{
		float2( 0.6111618, 0.1050905 ),
		float2( 0.1088336, 0.1127091 ),
		float2( 0.3030421, -0.6292974 ),
		float2( 0.4090526, 0.6716492 ),
		float2( -0.1608387, -0.3867823 ),
		float2( 0.7685862, -0.6118501 ),
		float2( -0.1935026, -0.856501 ),
		float2( -0.4028573, 0.07754025 ),
		float2( -0.6411021, -0.4748057 ),
		float2( -0.1314865, 0.8404058 ),
		float2( -0.7005203, 0.4596822 ),
		float2( -0.9713828, -0.06329931 )
	};

	float shadow = 0.0;

	// RB: casting a float to int and using it as index can really kill the performance ...
	float numSamples = 12.0;
	float stepSize = 1.0 / numSamples;

	float random = BlueNoise( fragment.position.xy, 1.0 );
	//float random = InterleavedGradientNoiseAnim( fragment.position.xy, rpJitterTexOffset.w );

	random *= PI;

	float2 rot;
	rot.x = cos( random );
	rot.y = sin( random );

	float shadowTexelSize = rpScreenCorrectionFactor.z * rpJitterTexScale.x;
	for( int si = 0; si < 12; si++ )
	{
		float2 jitter = poissonDisk[si];
		float2 jitterRotated;
		jitterRotated.x = jitter.x * rot.x - jitter.y * rot.y;
		jitterRotated.y = jitter.x * rot.y + jitter.y * rot.x;

		// [0 .. 1] -> rectangle in atlas transform
		float2 shadowTexcoordAtlas = shadowTexcoord.xy * rpJitterTexScale.y + rpShadowAtlasOffsets[ shadowIndex ].xy;

		float2 shadowTexcoordJittered = shadowTexcoordAtlas.xy + jitterRotated * shadowTexelSize;

		shadow += t_ShadowAtlas.SampleCmpLevelZero( s_Shadow, shadowTexcoordJittered.xy, receiver );
	}

	shadow *= stepSize;


#else

	// Vogel Disk Sampling
	// https://twitter.com/panoskarabelas1/status/1222663889659355140

	// this approach is more dynamic and can be controlled by r_shadowMapSamples

	float shadow = 0.0;

	float numSamples = rpJitterTexScale.w;
	float stepSize = 1.0 / numSamples;

	float vogelPhi = BlueNoise( fragment.position.xy, 1.0 );
	//float vogelPhi = InterleavedGradientNoiseAnim( fragment.position.xy, rpJitterTexOffset.w );

	float shadowTexelSize = rpScreenCorrectionFactor.z * rpJitterTexScale.x;
	for( float si = 0.0; si < numSamples; si += 1.0 )
	{
		float2 jitter = VogelDiskSample( si, numSamples, vogelPhi );

#if USE_SHADOW_ATLAS
		// [0 .. 1] -> rectangle in atlas transform
		float2 shadowTexcoordAtlas = shadowTexcoord.xy * rpJitterTexScale.y + rpShadowAtlasOffsets[ shadowIndex ].xy;

		float2 shadowTexcoordJittered = shadowTexcoordAtlas + jitter * shadowTexelSize;

		shadow += t_ShadowAtlas.SampleCmpLevelZero( s_Shadow, shadowTexcoordJittered.xy, receiver );
#else
		float3 shadowTexcoordJittered = float3( shadowTexcoord.xy + jitter * shadowTexelSize, shadowTexcoord.w );

		shadow += t_ShadowMapArray.SampleCmpLevelZero( s_Shadow, shadowTexcoordJittered, receiver );
#endif
	}

	shadow *= stepSize;
#endif

#else

#if USE_SHADOW_ATLAS
	float2 uvShadow;
	uvShadow.x = shadowTexcoord.x;
	uvShadow.y = shadowTexcoord.y;

	// [0 .. 1] -> rectangle in atlas transform
	uvShadow = uvShadow * rpJitterTexScale.y + rpShadowAtlasOffsets[ shadowIndex ].xy;

	float shadow = t_ShadowAtlas.SampleCmpLevelZero( s_Shadow, uvShadow.xy, receiver );
#else
	float3 uvzShadow;
	uvzShadow.x = shadowTexcoord.x;
	uvzShadow.y = shadowTexcoord.y;
	uvzShadow.z = shadowTexcoord.w;
	float shadow = t_ShadowMapArray.SampleCmpLevelZero( samp2, uvzShadow, receiver );
#endif

#if 0
	if( shadowIndex == 0 )
	{
		result.color = float4( 1.0, 0.0, 0.0, 1.0 );
	}
	else if( shadowIndex == 1 )
	{
		result.color = float4( 0.0, 1.0, 0.0, 1.0 );
	}
	else if( shadowIndex == 2 )
	{
		result.color = float4( 0.0, 0.0, 1.0, 1.0 );
	}
	else if( shadowIndex == 3 )
	{
		result.color = float4( 1.0, 1.0, 0.0, 1.0 );
	}
	else if( shadowIndex == 4 )
	{
		result.color = float4( 1.0, 0.0, 1.0, 1.0 );
	}
	else if( shadowIndex == 5 )
	{
		result.color = float4( 0.0, 1.0, 1.0, 1.0 );
	}

	result.color.rgb *= shadow;
	return;
#endif

#endif

	// allow shadows to fade out
	shadow = saturate( max( shadow, rpJitterTexScale.z ) );

	float3 halfAngleVector = normalize( lightVector + viewVector );
	float hdotN = clamp( dot3( halfAngleVector, localNormal ), 0.0, 1.0 );

#if USE_PBR
	const float metallic = specMapSRGB.g;
	const float roughness = specMapSRGB.r;
	const float glossiness = 1.0 - roughness;

	// the vast majority of real-world materials (anything not metal or gems) have F(0)
	// values in a very narrow range (~0.02 - 0.08)

	// approximate non-metals with linear RGB 0.04 which is 0.08 * 0.5 (default in UE4)
	const float3 dielectricColor = _float3( 0.04 );

	// derive diffuse and specular from albedo(m) base color
	const float3 baseColor = diffuseMap;

	float3 diffuseColor = baseColor * ( 1.0 - metallic );
	float3 specularColor = lerp( dielectricColor, baseColor, metallic );
#else
	const float roughness = EstimateLegacyRoughness( specMapSRGB.rgb );

	float3 diffuseColor = diffuseMap;
	float3 specularColor = specMapSRGB.rgb; // RB: should be linear but it looks too flat
#endif


	// RB: compensate r_lightScale 3 and the division of Pi
	//lambert *= 1.3;

	// rpDiffuseModifier contains light color multiplier
	float3 lightColor = sRGBToLinearRGB( lightProj.xyz * lightFalloff.xyz );

	float vdotN = clamp( dot3( viewVector, localNormal ), 0.0, 1.0 );
	float vdotH = clamp( dot3( viewVector, halfAngleVector ), 0.0, 1.0 );
	float ldotH = clamp( dot3( lightVector, halfAngleVector ), 0.0, 1.0 );

	// compensate r_lightScale 3 * 2
	float3 reflectColor = specularColor * rpSpecularModifier.rgb * 1.0;// * 0.5;

	// cheap approximation by ARM with only one division
	// http://community.arm.com/servlet/JiveServlet/download/96891546-19496/siggraph2015-mmg-renaldas-slides.pdf
	// page 26

	float rr = roughness * roughness;
	float rrrr = rr * rr;

	// disney GGX
	float D = ( hdotN * hdotN ) * ( rrrr - 1.0 ) + 1.0;
	float VFapprox = ( ldotH * ldotH ) * ( roughness + 0.5 );
	float3 specularLight = ( rrrr / ( 4.0 * PI * D * D * VFapprox ) ) * ldotN * reflectColor;
	//specularLight = float3( 0.0 );

#if 0
	result.color = float4( _float3( VFapprox ), 1.0 );
	return;
#endif

	// see http://seblagarde.wordpress.com/2012/01/08/pi-or-not-to-pi-in-game-lighting-equation/
	//lambert /= PI;

	//float3 diffuseColor = mix( diffuseMap, F0, metal ) * rpDiffuseModifier.xyz;
	float3 diffuseLight = diffuseColor * lambert * ( rpDiffuseModifier.xyz );

	float3 color = ( diffuseLight + specularLight ) * lightColor * fragment.color.rgb * shadow;

	result.color.rgb = color;
	result.color.a = 1.0;
}
