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

#include "global_inc.hlsl"


// *INDENT-OFF*
Texture2D				t_Normal			: register( t0 VK_DESCRIPTOR_SET( 1 ) );
Texture2D				t_Specular			: register( t1 VK_DESCRIPTOR_SET( 1 ) );
Texture2D				t_BaseColor			: register( t2 VK_DESCRIPTOR_SET( 1 ) );

Texture2D				t_LightFalloff		: register( t3 VK_DESCRIPTOR_SET( 2 ) );
Texture2D				t_LightProjection	: register( t4 VK_DESCRIPTOR_SET( 2 ) );

SamplerState			s_Material : register( s0 VK_DESCRIPTOR_SET( 3 ) ); // for the normal/specular/basecolor
SamplerState 			s_Lighting : register( s1 VK_DESCRIPTOR_SET( 3 ) ); // for sampling the jitter

struct PS_IN
{
	float4 position		: SV_Position;
//	float4 texcoord0		: TEXCOORD0_centroid;
	float4 texcoord1		: TEXCOORD1_centroid;
	float4 texcoord2		: TEXCOORD2_centroid;
	float4 texcoord3		: TEXCOORD3_centroid;
	float4 texcoord4		: TEXCOORD4_centroid;
	float4 texcoord5		: TEXCOORD5_centroid;
	float4 texcoord6		: TEXCOORD6_centroid;
	float4 color			: COLOR0;
};

struct PS_OUT
{
	half4 color : SV_Target0;
};
// *INDENT-ON*

void main( PS_IN fragment, out PS_OUT result )
{
	float4 bumpMap =		t_Normal.Sample( s_Material, fragment.texcoord1.xy );
	float4 lightFalloff =	idtex2Dproj( s_Lighting, t_LightFalloff, fragment.texcoord2 );
	float4 lightProj =		idtex2Dproj( s_Lighting, t_LightProjection, fragment.texcoord3 );
	float4 YCoCG =			t_BaseColor.Sample( s_Material, fragment.texcoord4.xy );
	float4 specMapSRGB =	t_Specular.Sample( s_Material, fragment.texcoord5.xy );
	float4 specMap =		sRGBAToLinearRGBA( specMapSRGB );

	const float3 ambientLightVector = half3( 0.5f, 9.5f - 0.385f, 0.8925f );
	float3 lightVector = normalize( ambientLightVector );
	float3 diffuseMap = sRGBToLinearRGB( ConvertYCoCgToRGB( YCoCG ) );

	float3 localNormal;
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
	float ldotN = saturate( dot3( localNormal, lightVector ) );

#if defined(USE_HALF_LAMBERT)
	// RB: http://developer.valvesoftware.com/wiki/Half_Lambert
	float halfLdotN = dot3( localNormal, lightVector ) * 0.5 + 0.5;
	halfLdotN *= halfLdotN;
	float lambert = halfLdotN;
#else
	float lambert = ldotN;
#endif

	const float specularPower = 10.0f;
	float hDotN = dot3( normalize( fragment.texcoord6.xyz ), localNormal );
	// RB: added abs
	float3 specularContribution = _float3( pow( abs( hDotN ), specularPower ) );

	float3 diffuseColor = diffuseMap * ( rpDiffuseModifier.xyz );
	float3 specularColor = specMap.xyz * specularContribution * ( rpSpecularModifier.xyz );
	float3 lightColor = sRGBToLinearRGB( lightProj.xyz * lightFalloff.xyz );

	result.color.xyz = ( diffuseColor + specularColor ) * lightColor * fragment.color.xyz;
	result.color.w = 1.0;
}
