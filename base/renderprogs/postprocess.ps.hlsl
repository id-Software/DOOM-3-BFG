/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2015 Robert Beckebans

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

// *INDENT-OFF*
uniform sampler2D samp0		: register(s0);
uniform sampler2D samp1		: register(s1);
 
struct PS_IN
{
	float4 position : VPOS;
	float2 texcoord0 : TEXCOORD0_centroid;
};
 
struct PS_OUT
{
	float4 color : COLOR;
};
// *INDENT-ON*

#define USE_TECHNICOLOR						1		// [0 or 1]

#define Technicolor_Amount					0.5		// [0.00 to 1.00]
#define Technicolor_Power					4.0		// [0.00 to 8.00]
#define Technicolor_RedNegativeAmount		0.88	// [0.00 to 1.00]
#define Technicolor_GreenNegativeAmount		0.88	// [0.00 to 1.00]
#define Technicolor_BlueNegativeAmount		0.88	// [0.00 to 1.00]

#define USE_VIBRANCE						1
#define Vibrance							0.5	// [-1.00 to 1.00]
#define	Vibrance_RGB_Balance				float3( 1.0, 1.0, 1.0 )

#define USE_FILMGRAIN 						1

float3 overlay( float3 a, float3 b )
{
	//return pow( abs( b ), 2.2 ) < 0.5 ? ( 2.0 * a * b ) : float3( 1.0, 1.0, 1.0 ) - float3( 2.0, 2.0, 2.0 ) * ( float3( 1.0, 1.0, 1.0 ) - a ) * ( 1.0 - b );
	//return abs( b ) < 0.5 ? ( 2.0 * a * b ) : float3( 1.0, 1.0, 1.0 ) - float3( 2.0, 2.0, 2.0 ) * ( float3( 1.0, 1.0, 1.0 ) - a ) * ( 1.0 - b );
	
	return float3(
			   b.x < 0.5 ? ( 2.0 * a.x * b.x ) : ( 1.0 - 2.0 * ( 1.0 - a.x ) * ( 1.0 - b.x ) ),
			   b.y < 0.5 ? ( 2.0 * a.y * b.y ) : ( 1.0 - 2.0 * ( 1.0 - a.y ) * ( 1.0 - b.y ) ),
			   b.z < 0.5 ? ( 2.0 * a.z * b.z ) : ( 1.0 - 2.0 * ( 1.0 - a.z ) * ( 1.0 - b.z ) ) );
}


void TechnicolorPass( inout float4 color )
{
	const float3 cyanFilter = float3( 0.0, 1.30, 1.0 );
	const float3 magentaFilter = float3( 1.0, 0.0, 1.05 );
	const float3 yellowFilter = float3( 1.6, 1.6, 0.05 );
	const float3 redOrangeFilter = float3( 1.05, 0.62, 0.0 );
	const float3 greenFilter = float3( 0.3, 1.0, 0.0 );
	
	float2 redNegativeMul   = color.rg * ( 1.0 / ( Technicolor_RedNegativeAmount * Technicolor_Power ) );
	float2 greenNegativeMul = color.rg * ( 1.0 / ( Technicolor_GreenNegativeAmount * Technicolor_Power ) );
	float2 blueNegativeMul  = color.rb * ( 1.0 / ( Technicolor_BlueNegativeAmount * Technicolor_Power ) );
	
	float redNegative   = dot( redOrangeFilter.rg, redNegativeMul );
	float greenNegative = dot( greenFilter.rg, greenNegativeMul );
	float blueNegative  = dot( magentaFilter.rb, blueNegativeMul );
	
	float3 redOutput   = float3( redNegative ) + cyanFilter;
	float3 greenOutput = float3( greenNegative ) + magentaFilter;
	float3 blueOutput  = float3( blueNegative ) + yellowFilter;
	
	float3 result = redOutput * greenOutput * blueOutput;
	color.rgb = lerp( color.rgb, result, Technicolor_Amount );
}


void VibrancePass( inout float4 color )
{
	const float3 vibrance = Vibrance_RGB_Balance * Vibrance;
	
	float Y = dot( LUMINANCE_SRGB, color );
	
	float minColor = min( color.r, min( color.g, color.b ) );
	float maxColor = max( color.r, max( color.g, color.b ) );
	
	float colorSat = maxColor - minColor;
	
	color.rgb = lerp( float3( Y ), color.rgb, ( 1.0 + ( vibrance * ( 1.0 - ( sign( vibrance ) * colorSat ) ) ) ) );
}


void FilmgrainPass( inout float4 color )
{
	float4 jitterTC = ( fragment.position * rpScreenCorrectionFactor ) + rpJitterTexOffset;
	//float4 jitterTC = ( fragment.position * ( 1.0 / 128.0 ) ) + rpJitterTexOffset;
	//float2 jitterTC = fragment.position.xy * 2.0;
	//jitterTC.x *= rpWindowCoord.y / rpWindowCoord.x;
	
	float4 noiseColor = tex2D( samp1, fragment.position.xy + jitterTC.xy );
	float Y = noiseColor.r;
	//float Y = dot( LUMINANCE_VECTOR, noiseColor );
	//noiseColor.rgb = float3( Y, Y, Y );
	
	float exposureFactor = 1.0;
	exposureFactor = sqrt( exposureFactor );
	const float noiseIntensity = 1.7; //rpScreenCorrectionFactor.z;
	
	float t = lerp( 3.5 * noiseIntensity, 1.13 * noiseIntensity, exposureFactor );
	color.rgb = overlay( color.rgb, lerp( float3( 0.5 ), noiseColor.rgb, t ) );
	
	//color.rgb = noiseColor.rgb;
	//color.rgb = lerp( color.rgb, noiseColor.rgb, 0.3 );
}


void main( PS_IN fragment, out PS_OUT result )
{
	float2 tCoords = fragment.texcoord0;
	
	// base color with tone mapping and other post processing applied
	float4 color = tex2D( samp0, tCoords );
	
#if USE_TECHNICOLOR
	TechnicolorPass( color );
#endif
	
#if USE_VIBRANCE
	VibrancePass( color );
#endif
	
#if USE_FILMGRAIN
	FilmgrainPass( color );
#endif
	
	result.color = color;
}
