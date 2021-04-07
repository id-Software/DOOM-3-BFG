/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2021 Robert Beckebans

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
uniform sampler2D				samp0 : register(s0); // texture 1 is the per-surface normal map
uniform sampler2D				samp1 : register(s1); // texture 3 is the per-surface specular or roughness/metallic/AO mixer map
uniform sampler2D				samp2 : register(s2); // texture 2 is the per-surface baseColor map 
uniform sampler2D				samp3 : register(s3); // texture 4 is the light falloff texture
uniform sampler2D				samp4 : register(s4); // texture 5 is the light projection texture

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
	half4 color			: COLOR0;
};

struct PS_OUT
{
	half4 color : COLOR;
};
// *INDENT-ON*

void main( PS_IN fragment, out PS_OUT result )
{
	half4 bumpMap =			tex2D( samp0, fragment.texcoord1.xy );
	half4 lightFalloff =	( idtex2Dproj( samp3, fragment.texcoord2 ) );
	half4 lightProj	=	( idtex2Dproj( samp4, fragment.texcoord3 ) );
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


	half3 halfAngleVector = normalize( lightVector + viewVector );
	half hdotN = clamp( dot3( halfAngleVector, localNormal ), 0.0, 1.0 );

#if defined( USE_PBR )
	const half metallic = specMapSRGB.g;
	const half roughness = specMapSRGB.r;
	const half glossiness = 1.0 - roughness;

	// the vast majority of real-world materials (anything not metal or gems) have F(0�)
	// values in a very narrow range (~0.02 - 0.08)

	// approximate non-metals with linear RGB 0.04 which is 0.08 * 0.5 (default in UE4)
	const half3 dielectricColor = half3( 0.04 );

	// derive diffuse and specular from albedo(m) base color
	const half3 baseColor = diffuseMap;

	half3 diffuseColor = baseColor * ( 1.0 - metallic );
	half3 specularColor = lerp( dielectricColor, baseColor, metallic );
#else
	const float roughness = EstimateLegacyRoughness( specMapSRGB.rgb );

	half3 diffuseColor = diffuseMap;
	half3 specularColor = specMapSRGB.rgb; // RB: should be linear but it looks too flat
#endif


	// RB: compensate r_lightScale 3 and the division of Pi
	//lambert *= 1.3;

	// rpDiffuseModifier contains light color multiplier
	half3 lightColor = sRGBToLinearRGB( lightProj.xyz * lightFalloff.xyz );

	half vdotN = clamp( dot3( viewVector, localNormal ), 0.0, 1.0 );
	half vdotH = clamp( dot3( viewVector, halfAngleVector ), 0.0, 1.0 );
	half ldotH = clamp( dot3( lightVector, halfAngleVector ), 0.0, 1.0 );

	// compensate r_lightScale 3 * 2
	half3 reflectColor = specularColor * rpSpecularModifier.rgb * 1.0;// * 0.5;

	// cheap approximation by ARM with only one division
	// http://community.arm.com/servlet/JiveServlet/download/96891546-19496/siggraph2015-mmg-renaldas-slides.pdf
	// page 26

	float rr = roughness * roughness;
	float rrrr = rr * rr;

	// disney GGX
	float D = ( hdotN * hdotN ) * ( rrrr - 1.0 ) + 1.0;
	float VFapprox = ( ldotH * ldotH ) * ( roughness + 0.5 );
	half3 specularLight = ( rrrr / ( 4.0 * PI * D * D * VFapprox ) ) * ldotN * reflectColor;
	//specularLight = half3( 0.0 );

#if 0
	result.color = float4( _half3( VFapprox ), 1.0 );
	return;
#endif

	// see http://seblagarde.wordpress.com/2012/01/08/pi-or-not-to-pi-in-game-lighting-equation/
	//lambert /= PI;

	//half3 diffuseColor = mix( diffuseMap, F0, metal ) * rpDiffuseModifier.xyz;
	half3 diffuseLight = diffuseColor * lambert * ( rpDiffuseModifier.xyz );

	float3 color = ( diffuseLight + specularLight ) * lightColor * fragment.color.rgb;

	result.color.rgb = color;
	result.color.a = 1.0;
}
