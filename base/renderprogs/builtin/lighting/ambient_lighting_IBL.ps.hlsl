/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2020 Robert Beckebans

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

#include "renderprogs/global.inc.hlsl"

#include "renderprogs/BRDF.inc.hlsl"


// *INDENT-OFF*
uniform sampler2D samp0 : register(s0); // texture 0 is the per-surface normal map
uniform sampler2D samp1 : register(s1); // texture 1 is the per-surface specular or roughness/metallic/AO mixer map
uniform sampler2D samp2 : register(s2); // texture 2 is the per-surface baseColor map 
uniform sampler2D samp3 : register(s3); // texture 3 is the BRDF LUT
uniform sampler2D samp4 : register(s4); // texture 4 is SSAO

uniform sampler2D	samp7 : register(s7); // texture 7 is the irradiance cube map
uniform sampler2D	samp8 : register(s8); // texture 8 is the radiance cube map

struct PS_IN 
{
	half4 position	: VPOS;
	half4 texcoord0	: TEXCOORD0_centroid;
	half4 texcoord1	: TEXCOORD1_centroid;
	half4 texcoord2	: TEXCOORD2_centroid;
	half4 texcoord3	: TEXCOORD3_centroid;
	half4 texcoord4	: TEXCOORD4_centroid;
	half4 texcoord5	: TEXCOORD5_centroid;
	half4 texcoord6	: TEXCOORD6_centroid;
	half4 color		: COLOR0;
};

struct PS_OUT
{
	half4 color : COLOR;
};
// *INDENT-ON*


void main( PS_IN fragment, out PS_OUT result )
{
	half4 bumpMap =			tex2D( samp0, fragment.texcoord0.xy );
	half4 YCoCG =			tex2D( samp2, fragment.texcoord1.xy );
	half4 specMapSRGB =		tex2D( samp1, fragment.texcoord2.xy );
	half4 specMap =			sRGBAToLinearRGBA( specMapSRGB );

	half3 diffuseMap = sRGBToLinearRGB( ConvertYCoCgToRGB( YCoCG ) );

	half3 localNormal;
#if defined(USE_NORMAL_FMT_RGB8)
	localNormal.xy = bumpMap.rg - 0.5;
#else
	localNormal.xy = bumpMap.wy - 0.5;
#endif
	localNormal.z = sqrt( abs( dot( localNormal.xy, localNormal.xy ) - 0.25 ) );
	localNormal = normalize( localNormal );

	float3 globalNormal;
	globalNormal.x = dot3( localNormal, fragment.texcoord4 );
	globalNormal.y = dot3( localNormal, fragment.texcoord5 );
	globalNormal.z = dot3( localNormal, fragment.texcoord6 );
	globalNormal = normalize( globalNormal );

	float3 globalEye = normalize( fragment.texcoord3.xyz );

	float3 reflectionVector = globalNormal * dot3( globalEye, globalNormal );
	reflectionVector = normalize( ( reflectionVector * 2.0f ) - globalEye );

	half vDotN = saturate( dot3( globalEye, globalNormal ) );

#if defined( USE_PBR )
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

#if defined( DEBUG_PBR )
	diffuseColor = half3( 0.0, 0.0, 0.0 );
	specularColor = half3( 0.0, 1.0, 0.0 );
#endif

	float3 kS = Fresnel_SchlickRoughness( specularColor, vDotN, roughness );
	float3 kD = ( float3( 1.0, 1.0, 1.0 ) - kS ) * ( 1.0 - metallic );

#else
	// HACK calculate roughness from D3 gloss maps
	float Y = dot( LUMINANCE_SRGB.rgb, specMapSRGB.rgb );

	//const float glossiness = clamp( 1.0 - specMapSRGB.r, 0.0, 0.98 );
	const float glossiness = clamp( pow( Y, 1.0 / 2.0 ), 0.0, 0.98 );

	const float roughness = 1.0 - glossiness;

	half3 diffuseColor = diffuseMap;
	half3 specularColor = specMap.rgb;

#if defined( DEBUG_PBR )
	diffuseColor = half3( 0.0, 0.0, 0.0 );
	specularColor = half3( 1.0, 0.0, 0.0 );
#endif

	float3 kS = Fresnel_SchlickRoughness( specularColor, vDotN, roughness );

	// NOTE: metalness is missing
	float3 kD = ( float3( 1.0, 1.0, 1.0 ) - kS );

#endif

	//diffuseColor = half3( 1.0, 1.0, 1.0 );
	//diffuseColor = half3( 0.0, 0.0, 0.0 );

	// calculate the screen texcoord in the 0.0 to 1.0 range
	//float2 screenTexCoord = vposToScreenPosTexCoord( fragment.position.xy );
	float2 screenTexCoord = fragment.position.xy * rpWindowCoord.xy;

	float ao = tex2D( samp4, screenTexCoord ).r;
	//diffuseColor.rgb *= ao;

	// evaluate diffuse IBL

	float2 normalizedOctCoord = octEncode( globalNormal );
	float2 normalizedOctCoordZeroOne = ( normalizedOctCoord + float2( 1.0 ) ) * 0.5;

	float3 irradiance = tex2D( samp7, normalizedOctCoordZeroOne ).rgb;
	float3 diffuseLight = ( kD * irradiance * diffuseColor ) * ao * ( rpDiffuseModifier.xyz * 1.0 );

	// evaluate specular IBL

	// should be 8 = numMips - 1, 256^2 = 9 mips
	const float MAX_REFLECTION_LOD = 10.0;
	float mip = clamp( ( roughness * MAX_REFLECTION_LOD ), 0.0, MAX_REFLECTION_LOD );
	//float mip = 0.0;

	normalizedOctCoord = octEncode( reflectionVector );
	normalizedOctCoordZeroOne = ( normalizedOctCoord + float2( 1.0 ) ) * 0.5;

	float3 radiance = textureLod( samp8, normalizedOctCoordZeroOne, mip ).rgb;
	//radiance = float3( 0.0 );

	float2 envBRDF  = texture( samp3, float2( max( vDotN, 0.0 ), roughness ) ).rg;

#if 0
	result.color.rgb = float3( envBRDF.x, envBRDF.y, 0.0 );
	result.color.w = fragment.color.a;
	return;
#endif

	float specAO = ComputeSpecularAO( vDotN, ao, roughness );
	float3 specularLight = radiance * ( kS * envBRDF.x + float3( envBRDF.y ) ) * specAO * ( rpSpecularModifier.xyz * 0.5 );

#if 0
	// Marmoset Horizon Fade trick
	const half horizonFade = 1.3;
	half horiz = saturate( 1.0 + horizonFade * saturate( dot3( reflectionVector, globalNormal ) ) );
	horiz *= horiz;
	//horiz = clamp( horiz, 0.0, 1.0 );
#endif

	half3 lightColor = sRGBToLinearRGB( rpAmbientColor.rgb );

	//result.color.rgb = diffuseLight;
	//result.color.rgb = diffuseLight * lightColor;
	//result.color.rgb = specularLight;
	result.color.rgb = ( diffuseLight + specularLight ) * lightColor * fragment.color.rgb;
	//result.color.rgb = localNormal.xyz * 0.5 + 0.5;
	//result.color.rgb = float3( ao );
	result.color.w = fragment.color.a;
}
