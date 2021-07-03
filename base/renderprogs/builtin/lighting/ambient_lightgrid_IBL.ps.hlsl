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
uniform sampler2D samp0 : register(s0); // texture 0 is the per-surface normal map
uniform sampler2D samp1 : register(s1); // texture 1 is the per-surface specular or roughness/metallic/AO mixer map
uniform sampler2D samp2 : register(s2); // texture 2 is the per-surface baseColor map 
uniform sampler2D samp3 : register(s3); // texture 3 is the BRDF LUT
uniform sampler2D samp4 : register(s4); // texture 4 is SSAO

uniform sampler2D	samp7 : register(s7); // texture 7 is the irradiance cube map
uniform sampler2D	samp8 : register(s8); // texture 8 is the radiance cube map 1
uniform sampler2D	samp9 : register(s9); // texture 9 is the radiance cube map 2
uniform sampler2D	samp10 : register(s10); // texture 10 is the radiance cube map 3

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
	half4 texcoord7	: TEXCOORD7_centroid;
	half4 color		: COLOR0;
};

struct PS_OUT
{
	half4 color : COLOR;
};
// *INDENT-ON*


// RB: TODO OPTIMIZE
// this is a straight port of idBounds::RayIntersection
bool AABBRayIntersection( float3 b[2], float3 start, float3 dir, out float scale )
{
	int i, ax0, ax1, ax2, side, inside;
	float f;
	float3 hit;

	ax0 = -1;
	inside = 0;
	for( i = 0; i < 3; i++ )
	{
		if( start[i] < b[0][i] )
		{
			side = 0;
		}
		else if( start[i] > b[1][i] )
		{
			side = 1;
		}
		else
		{
			inside++;
			continue;
		}
		if( dir[i] == 0.0f )
		{
			continue;
		}

		f = ( start[i] - b[side][i] );

		if( ax0 < 0 || abs( f ) > abs( scale * dir[i] ) )
		{
			scale = - ( f / dir[i] );
			ax0 = i;
		}
	}

	if( ax0 < 0 )
	{
		scale = 0.0f;

		// return true if the start point is inside the bounds
		return ( inside == 3 );
	}

	ax1 = ( ax0 + 1 ) % 3;
	ax2 = ( ax0 + 2 ) % 3;
	hit[ax1] = start[ax1] + scale * dir[ax1];
	hit[ax2] = start[ax2] + scale * dir[ax2];

	return ( hit[ax1] >= b[0][ax1] && hit[ax1] <= b[1][ax1] &&
			 hit[ax2] >= b[0][ax2] && hit[ax2] <= b[1][ax2] );
}

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

	float3 globalPosition = fragment.texcoord7.xyz;

	float3 globalView = normalize( rpGlobalEyePos.xyz - globalPosition );

	float3 reflectionVector = globalNormal * dot3( globalView, globalNormal );
	reflectionVector = normalize( ( reflectionVector * 2.0f ) - globalView );

#if 0
	// parallax box correction using portal area bounds
	float hitScale = 0.0;
	float3 bounds[2];
	bounds[0].x = rpWobbleSkyX.x;
	bounds[0].y = rpWobbleSkyX.y;
	bounds[0].z = rpWobbleSkyX.z;

	bounds[1].x = rpWobbleSkyY.x;
	bounds[1].y = rpWobbleSkyY.y;
	bounds[1].z = rpWobbleSkyY.z;

	// global fragment position
	float3 rayStart = fragment.texcoord7.xyz;

	// we can't start inside the box so move this outside and use the reverse path
	rayStart += reflectionVector * 10000.0;

	// only do a box <-> ray intersection test if we use a local cubemap
	if( ( rpWobbleSkyX.w > 0.0 ) && AABBRayIntersection( bounds, rayStart, -reflectionVector, hitScale ) )
	{
		float3 hitPoint = rayStart - reflectionVector * hitScale;

		// rpWobbleSkyZ is cubemap center
		reflectionVector = hitPoint - rpWobbleSkyZ.xyz;
	}
#endif

	half vDotN = saturate( dot3( globalView, globalNormal ) );

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
	const float roughness = EstimateLegacyRoughness( specMapSRGB.rgb );

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

	float ao = 1.0;
	ao = tex2D( samp4, screenTexCoord ).r;
	//diffuseColor.rgb *= ao;

	// evaluate diffuse IBL

	float2 normalizedOctCoord = octEncode( globalNormal );
	float2 normalizedOctCoordZeroOne = ( normalizedOctCoord + float2( 1.0 ) ) * 0.5;

// lightgrid atlas

	//float3 lightGridOrigin = float3( -192.0, -128.0, 0 );
	//float3 lightGridSize = float3( 64.0, 64.0, 128.0 );
	//int3 lightGridBounds = int3( 7, 7, 3 );

	float3 lightGridOrigin = rpGlobalLightOrigin.xyz;
	float3 lightGridSize = rpJitterTexScale.xyz;
	int3 lightGridBounds = int3( rpJitterTexOffset.x, rpJitterTexOffset.y, rpJitterTexOffset.z );

	float invXZ = ( 1.0 / ( lightGridBounds[0] * lightGridBounds[2] ) );
	float invY = ( 1.0 / lightGridBounds[1] );

	normalizedOctCoordZeroOne.x *= invXZ;
	normalizedOctCoordZeroOne.y *= invY;

	int3 gridCoord;
	float3 frac;
	float3 lightOrigin = globalPosition - lightGridOrigin;

	for( int i = 0; i < 3; i++ )
	{
		float           v;

		// walls can be sampled behind the grid sometimes so avoid negative weights
		v = max( 0, lightOrigin[i] * ( 1.0 / lightGridSize[i] ) );
		gridCoord[i] = int( floor( v ) );
		frac[ i ] = v - gridCoord[ i ];

		/*
		if( gridCoord[i] < 0 )
		{
			gridCoord[i] = 0;
		}
		else
		*/
		if( gridCoord[i] >= lightGridBounds[i] - 1 )
		{
			gridCoord[i] = lightGridBounds[i] - 1;
		}
	}

	// trilerp the light value
	int3 gridStep;

	gridStep[0] = 1;
	gridStep[1] = lightGridBounds[0];
	gridStep[2] = lightGridBounds[0] * lightGridBounds[1];

	float totalFactor = 0.0;
	float3 irradiance = float3( 0.0, 0.0, 0.0 );

	/*
	for( int i = 0; i < 8; i++ )
	{
		for( int j = 0; j < 3; j++ )
		{
			if( i & ( 1 << j ) )

		results in these offsets
	*/
	const float3 cornerOffsets[8] = float3[](
										float3( 0.0, 0.0, 0.0 ),
										float3( 1.0, 0.0, 0.0 ),
										float3( 0.0, 2.0, 0.0 ),
										float3( 1.0, 2.0, 0.0 ),
										float3( 0.0, 0.0, 4.0 ),
										float3( 1.0, 0.0, 4.0 ),
										float3( 0.0, 2.0, 4.0 ),
										float3( 1.0, 2.0, 4.0 ) );

	for( int i = 0; i < 8; i++ )
	{
		float factor = 1.0;

		int3 gridCoord2 = gridCoord;

		for( int j = 0; j < 3; j++ )
		{
			if( cornerOffsets[ i ][ j ] > 0.0 )
			{
				factor *= frac[ j ];

				gridCoord2[ j ] += 1;
			}
			else
			{
				factor *= ( 1.0 - frac[ j ] );
			}
		}

		// build P
		//float3 P = lightGridOrigin + ( gridCoord2[0] * gridStep[0] + gridCoord2[1] * gridStep[1] + gridCoord2[2] * gridStep[2] );

		float2 atlasOffset;

		atlasOffset.x = ( gridCoord2[0] * gridStep[0] + gridCoord2[2] * gridStep[1] ) * invXZ;
		atlasOffset.y = ( gridCoord2[1] * invY );

		// offset by one pixel border bleed size for linear filtering
#if 1
		// rpScreenCorrectionFactor.w = probeSize factor accounting account offset border, e.g = ( 16 / 18 ) = 0.8888
		float2 octCoordNormalizedToTextureDimensions = ( normalizedOctCoordZeroOne + atlasOffset ) * rpScreenCorrectionFactor.w;

		// skip by default 2 pixels for each grid cell and offset the start position by (1,1)
		// rpScreenCorrectionFactor.z = borderSize e.g = 2
		float2 probeTopLeftPosition;
		probeTopLeftPosition.x = ( gridCoord2[0] * gridStep[0] + gridCoord2[2] * gridStep[1] ) * rpScreenCorrectionFactor.z + rpScreenCorrectionFactor.z * 0.5;
		probeTopLeftPosition.y = ( gridCoord2[1] ) * rpScreenCorrectionFactor.z + rpScreenCorrectionFactor.z * 0.5;

		float2 normalizedProbeTopLeftPosition = probeTopLeftPosition * rpCascadeDistances.zw;

		float2 atlasCoord = normalizedProbeTopLeftPosition + octCoordNormalizedToTextureDimensions;
#else
		float2 atlasCoord = normalizedOctCoordZeroOne + atlasOffset;
#endif

		float3 color = texture( samp7, atlasCoord, 0 ).rgb;

		if( ( color.r + color.g + color.b ) < 0.0001 )
		{
			// ignore samples in walls
			continue;
		}

		irradiance += color * factor;
		totalFactor += factor;
	}

	if( totalFactor > 0.0 && totalFactor < 0.9999 )
	{
		totalFactor = 1.0 / totalFactor;

		irradiance *= totalFactor;
	}

// lightgrid atlas


	float3 diffuseLight = ( kD * irradiance * diffuseColor ) * ao * ( rpDiffuseModifier.xyz * 1.0 );

	// evaluate specular IBL

	// 512^2 = 10 mips
	// however we can't use the last 3 mips with octahedrons because the quality suffers too much
	// so it is 7 - 1
	const float MAX_REFLECTION_LOD = 6.0;
	float mip = clamp( ( roughness * MAX_REFLECTION_LOD ), 0.0, MAX_REFLECTION_LOD );
	//float mip = 0.0;

	normalizedOctCoord = octEncode( reflectionVector );
	normalizedOctCoordZeroOne = ( normalizedOctCoord + float2( 1.0 ) ) * 0.5;

	float3 radiance = textureLod( samp8, normalizedOctCoordZeroOne, mip ).rgb * rpLocalLightOrigin.x;
	radiance += textureLod( samp9, normalizedOctCoordZeroOne, mip ).rgb * rpLocalLightOrigin.y;
	radiance += textureLod( samp10, normalizedOctCoordZeroOne, mip ).rgb * rpLocalLightOrigin.z;
	//radiance = float3( 0.0 );

	// RB: HACK dim down room radiance by better local irradiance brightness
	//float luma = PhotoLuma( irradiance );
	//float luma = dot( irradiance, LUMINANCE_LINEAR.rgb );
	//float luma = length( irradiance.rgb );
	//radiance *= ( luma * rpSpecularModifier.x * 3.0 );

	float2 envBRDF  = texture( samp3, float2( max( vDotN, 0.0 ), roughness ) ).rg;

#if 0
	result.color.rgb = float3( envBRDF.x, envBRDF.y, 0.0 );
	result.color.w = fragment.color.a;
	return;
#endif

	float specAO = ComputeSpecularAO( vDotN, ao, roughness );
	float3 specularLight = radiance * ( kS * envBRDF.x + float3( envBRDF.y ) ) * specAO * ( rpSpecularModifier.xyz * 1.0 );

#if 1
	// Marmoset Horizon Fade trick
	const half horizonFade = 1.3;
	half horiz = saturate( 1.0 + horizonFade * saturate( dot3( reflectionVector, globalNormal ) ) );
	horiz *= horiz;
	//horiz = clamp( horiz, 0.0, 1.0 );
#endif

	//half3 lightColor = sRGBToLinearRGB( rpAmbientColor.rgb );
	half3 lightColor = ( rpAmbientColor.rgb );

	//result.color.rgb = diffuseLight;
	//result.color.rgb = diffuseLight * lightColor;
	//result.color.rgb = specularLight;
	result.color.rgb = ( diffuseLight + specularLight * horiz ) * lightColor * fragment.color.rgb;
	//result.color.rgb = localNormal.xyz * 0.5 + 0.5;
	//result.color.rgb = float3( ao );
	result.color.w = fragment.color.a;
}
