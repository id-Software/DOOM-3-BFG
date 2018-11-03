/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company. 
Copyright (C) 2013-2014 Robert Beckebans

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

#include "global.inc.hlsl"
#include "BRDF.inc.hlsl"

uniform sampler2D               samp0 : register(s0); // texture 1 is the per-surface normal map
uniform sampler2D               samp1 : register(s1); // texture 3 is the per-surface specular or roughness/metallic/AO mixer map
uniform sampler2D               samp2 : register(s2); // texture 2 is the per-surface baseColor map 
uniform sampler2D               samp3 : register(s3); // texture 4 is the light falloff texture
uniform sampler2D               samp4 : register(s4); // texture 5 is the light projection texture
uniform sampler2DArrayShadow	samp5 : register(s5); // texture 6 is the shadowmap array
uniform sampler2D				samp6 : register(s6); // texture 7 is the jitter texture 



struct PS_IN
{
	half4 position		: VPOS;
	half4 texcoord0		: TEXCOORD0_centroid;
	half4 texcoord1		: TEXCOORD1_centroid;
	half4 texcoord2		: TEXCOORD2_centroid;
	half4 texcoord3		: TEXCOORD3_centroid;
	half4 texcoord4		: TEXCOORD4_centroid;
	half4 texcoord5		: TEXCOORD5_centroid;
	half4 texcoord6		: TEXCOORD6_centroid;
	half4 texcoord7		: TEXCOORD7_centroid;
	half4 texcoord8		: TEXCOORD8_centroid;
	half4 texcoord9		: TEXCOORD9_centroid;
	half4 color			: COLOR0;
};

struct PS_OUT
{
	half4 color : COLOR;
};

void main( PS_IN fragment, out PS_OUT result )
{
	half4 bumpMap =			tex2D( samp0, fragment.texcoord1.xy );
	half4 lightFalloff =	( idtex2Dproj( samp3, fragment.texcoord2 ) );
	half4 lightProj	=		( idtex2Dproj( samp4, fragment.texcoord3 ) );
	half4 YCoCG =			tex2D( samp2, fragment.texcoord4.xy );
	half4 specMapSRGB =		tex2D( samp1, fragment.texcoord5.xy );
	half4 specMap =			sRGBAToLinearRGBA( specMapSRGB );

	half3 lightVector = normalize( fragment.texcoord0.xyz );
	half3 viewVector = normalize( fragment.texcoord6.xyz );
	half3 diffuseMap = sRGBToLinearRGB( ConvertYCoCgToRGB( YCoCG ) );

	half3 localNormal;
	// RB begin
#if defined(USE_NORMAL_FMT_RGB8)
	localNormal.xy = bumpMap.rg - 0.5;
#else
	localNormal.xy = bumpMap.wy - 0.5;
#endif
	// RB end
	localNormal.z = sqrt( abs( dot( localNormal.xy, localNormal.xy ) - 0.25 ) );
	localNormal = normalize( localNormal );
	
	// traditional very dark Lambert light model used in Doom 3
	half ldotN = saturate( dot3( localNormal, lightVector ) );

#if defined(USE_HALF_LAMBERT)
	// RB: http://developer.valvesoftware.com/wiki/Half_Lambert
	half halfLdotN = dot3( localNormal, lightVector ) * 0.5 + 0.5;
	halfLdotN *= halfLdotN;

	// tweak to not loose so many details
	half lambert = lerp( ldotN, halfLdotN, 0.5 );
#else
	half lambert = ldotN;
#endif


	//
	// shadow mapping
	//
	int shadowIndex = 0;
	
#if defined( LIGHT_POINT )
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

#endif // #if defined( POINTLIGHT )

#if defined( LIGHT_PARALLEL )
	
	float viewZ = -fragment.texcoord9.z;
	
	shadowIndex = 4;
	for( int i = 0; i < 4; i++ )
	{
		if( viewZ < rpCascadeDistances[i] )
		{
			shadowIndex = i;
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
	
	shadowTexcoord.z = shadowTexcoord.z * rpScreenCorrectionFactor.w;
	//shadowTexcoord.z = shadowTexcoord.z * 0.999991;
	//shadowTexcoord.z = shadowTexcoord.z - bias;
	shadowTexcoord.w = float(shadowIndex);

#if 0
	result.color.xyz = float3( shadowTexcoord.z, shadowTexcoord.z, shadowTexcoord.z );
	result.color.w = 1.0;
	return;
#endif

	// multiple taps
	
#if 0
	float4 base = shadowTexcoord;
	
	base.xy += rpJitterTexScale.xy * -0.5;
	
	float shadow = 0.0;
	
	//float stepSize = 1.0 / 16.0;
	float numSamples = 16;
	float stepSize = 1.0 / numSamples;
	
	float4 jitterTC = ( fragment.position * rpScreenCorrectionFactor ) + rpJitterTexOffset;
	for( float i = 0.0; i < numSamples; i += 1.0 )
	{
		float4 jitter = base + tex2D( samp6, jitterTC.xy ) * rpJitterTexScale;
		jitter.zw = shadowTexcoord.zw;
	
		shadow += texture( samp5, jitter.xywz );
		jitterTC.x += stepSize;
	}
	
	shadow *= stepSize;
#else
	
	const float2 poissonDisk[12] = float2[](
	float2(0.6111618, 0.1050905),
	float2(0.1088336, 0.1127091),
	float2(0.3030421, -0.6292974),
	float2(0.4090526, 0.6716492),
	float2(-0.1608387, -0.3867823),
	float2(0.7685862, -0.6118501),
	float2(-0.1935026, -0.856501),
	float2(-0.4028573, 0.07754025),
	float2(-0.6411021, -0.4748057),
	float2(-0.1314865, 0.8404058),
	float2(-0.7005203, 0.4596822),
	float2(-0.9713828, -0.06329931) );
	
	float shadow = 0.0;
	
	// RB: casting a float to int and using it as index can really kill the performance ...
	float numSamples = 12.0; //int(rpScreenCorrectionFactor.w);
	float stepSize = 1.0 / numSamples;
	
	float4 jitterTC = ( fragment.position * rpScreenCorrectionFactor ) + rpJitterTexOffset;
	float4 random = tex2D( samp6, jitterTC.xy ) * PI;
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
       
        shadow += texture( samp5, shadowTexcoordJittered.xywz);
    }

   shadow *= stepSize;

	//float shadow = texture( samp5, shadowTexcoord.xywz );
#endif


	half3 halfAngleVector = normalize( lightVector + viewVector );
	half hdotN = clamp( dot3( halfAngleVector, localNormal ), 0.0, 1.0 );

#if 0 //defined(USE_PBR)
		
#if 0 //defined(USE_METALNESS)
	const half metallic = specMapSRGB.g;
	const half roughness = specMapSRGB.r;
	const half glossiness = 1.0 - roughness;

	// the vast majority of real-world materials (anything not metal or gems) have F(0°)
	// values in a very narrow range (~0.02 - 0.08)
	
	// approximate non-metals with linear RGB 0.04 which is 0.08 * 0.5 (default in UE4)
	const half3 dielectricColor = half3( 0.04 );
	
	// derive diffuse and specular from albedo(m) base color
	const half3 baseColor = diffuseMap;
	
	half3 diffuseColor = baseColor * ( 1.0 - metallic );
	half3 specularColor = lerp( dielectricColor, baseColor, metallic );
#else
	// HACK calculate roughness from D3 gloss maps
	float Y = dot( LUMINANCE_SRGB.rgb, specMapSRGB.rgb );
	
	//const float glossiness = clamp( 1.0 - specMapSRGB.r, 0.0, 0.98 );
	const float glossiness = clamp( pow( Y, 1.0 / 2.0 ), 0.0, 0.98 );
	
	const float roughness = 1.0 - glossiness;
	
	half3 diffuseColor = diffuseMap;
	half3 specularColor = specMap.rgb;
#endif

		
	// RB: compensate r_lightScale 3 and the division of Pi
	//lambert *= 1.3;
	
	// rpDiffuseModifier contains light color multiplier
	half3 lightColor = sRGBToLinearRGB( lightProj.xyz * lightFalloff.xyz );// * rpDiffuseModifier.xyz;
	
	half vdotN = clamp( dot3( viewVector, localNormal ), 0.0, 1.0 );
	half vdotH = clamp( dot3( viewVector, halfAngleVector ), 0.0, 1.0 );
	half ldotH = clamp( dot3( lightVector, halfAngleVector ), 0.0, 1.0 );
	
	// compensate r_lightScale 3 * 2
	half3 reflectColor = specMap.rgb * rpSpecularModifier.rgb * 1.0;// * 0.5;
	
	// cheap approximation by ARM with only one division
	// http://community.arm.com/servlet/JiveServlet/download/96891546-19496/siggraph2015-mmg-renaldas-slides.pdf
	// page 26
	
	float rr = roughness * roughness;
	float rrrr = rr * rr;
	
	// disney GGX
	float D = ( hdotN * hdotN ) * ( rrrr - 1.0 ) + 1.0;
	float VFapprox = ( ldotH * ldotH ) * ( roughness + 0.5 );
	half3 specularBRDF = ( rrrr / ( 4.0 * PI * D * D * VFapprox ) ) * ldotN * reflectColor;
	//specularBRDF = half3( 0.0 );
	
#if 0
	result.color = float4( _half3( VFapprox ), 1.0 );
	return;
#endif
	
	// see http://seblagarde.wordpress.com/2012/01/08/pi-or-not-to-pi-in-game-lighting-equation/
	//lambert /= PI;
	
	//half3 diffuseColor = mix( diffuseMap, F0, metal ) * rpDiffuseModifier.xyz;
	half3 diffuseBRDF = diffuseColor * lambert * sRGBToLinearRGB( rpDiffuseModifier.xyz );
		
	result.color.xyz = ( diffuseBRDF + specularBRDF ) * lightColor * fragment.color.rgb * shadow;
	result.color.w = 1.0;
	
#else
	
	/*
	OLD Blinn Phong
	*/

	const half specularPower = 10.0f;
	
	// RB: added abs
	half3 specularContribution = _half3( pow( hdotN, specularPower ) );

	half3 diffuseColor = diffuseMap * sRGBToLinearRGB( rpDiffuseModifier.xyz );
	half3 specularColor = specMap.xyz * specularContribution * sRGBToLinearRGB( rpSpecularModifier.xyz );
	half3 lightColor = sRGBToLinearRGB( lightProj.xyz * lightFalloff.xyz );
	
	/*
	half rim =  1.0f - saturate( hdotN );
	half rimPower = 16.0f;
	half3 rimColor = diffuseColor * lightProj.xyz * lightFalloff.xyz * 1.0f * pow( rim, rimPower ) * fragment.color.rgb;// * halfLdotN;
	*/

	result.color.xyz = ( diffuseColor + specularColor ) * lambert * lightColor * fragment.color.rgb * shadow;// + rimColor;
	//result.color.xyz = lightColor * lambert * fragment.color.rgb * shadow;// + rimColor;
	result.color.w = 1.0;
#endif
}
