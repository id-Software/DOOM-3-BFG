/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company. 
Copyright (C) 2009-2015 Robert Beckebans

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

uniform sampler2D	samp0 : register(s0); // texture 0 is _currentRender
uniform sampler2D	samp1 : register(s1); // texture 1 is heatmap

struct PS_IN
{
	float4 position : VPOS;
	float2 texcoord0 : TEXCOORD0_centroid;
};

struct PS_OUT
{
	float4 color : COLOR;
};


 
float3 Uncharted2Tonemap( float3 x )
{
	float A = 0.22; // shoulder strength
	float B = 0.3;	// linear strength
	float C = 0.10;	// linear angle
	float D = 0.20;	// toe strength
	float E = 0.01;	// toe numerator
	float F = 0.30;	// toe denominator
	float W = 11.2;	// linear white point

	return ( ( x * ( A * x + C * B ) + D * E ) / ( x * ( A * x + B ) + D * F ) ) - E / F;
}

// https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
float3 ACESFilm( float3 x )
{
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return saturate( ( x * ( a * x + b ) ) / ( x * ( c * x + d ) + e ) );
}

void main( PS_IN fragment, out PS_OUT result )
{
	float2 tCoords = fragment.texcoord0;
	
#if defined(BRIGHTPASS_FILTER)
	// multiply with 4 because the FBO is only 1/4th of the screen resolution
	tCoords *= float2( 4.0, 4.0 );
#endif
	
	float4 color = tex2D( samp0, tCoords );
	
	// get the luminance of the current pixel
	float Y = dot( LUMINANCE_SRGB, color );

	const float hdrGamma = 2.2;
	float gamma = hdrGamma;
	
#if 0
	// convert from sRGB to linear RGB
	color.r = pow( color.r, gamma );
	color.g = pow( color.g, gamma );
	color.b = pow( color.b, gamma );
#endif
	
#if defined(BRIGHTPASS)
	if(Y < 0.1)
	{
		//discard;
		result.color = float4( 0.0, 0.0, 0.0, 1.0 );
		return;
	}
#endif
	
	float hdrKey = rpScreenCorrectionFactor.x;
	float hdrAverageLuminance = rpScreenCorrectionFactor.y;
	float hdrMaxLuminance = rpScreenCorrectionFactor.z;
	
	// calculate the relative luminance
	float Yr = ( hdrKey * Y ) / hdrAverageLuminance;

	float Ymax = hdrMaxLuminance;

#define OPERATOR 2
	
#if OPERATOR == 0
	// advanced Reinhard operator, artistically desirable to burn out bright areas
	float L = Yr * ( 1.0 + Yr / ( Ymax * Ymax ) ) / ( 1.0 + Yr );
	color.rgb *= L;

#elif OPERATOR == 1
	// http://freespace.virgin.net/hugo.elias/graphics/x_posure.htm
	// exponential tone mapper that is very similar to the Uncharted one
	// very good in keeping the colors natural
	float exposure = 1.0;
	float L = ( 1.0 - exp( -Yr * exposure ) );
	color.rgb *= L;
	
	// Kodak filmic tone mappping, includes gamma correction
	//float3 rgb = max( float3( 0 ), color.rgb - float3( 0.004 ) );
	//color.rgb = rgb * ( float3( 0.5 ) + 6.2 * rgb ) / ( float3( 0.06 ) + rgb * ( float3( 1.7 ) + 6.2 * rgb ) );
	
	// http://iwasbeingirony.blogspot.de/2010/04/approximating-film-with-tonemapping.html
	//const float cutoff = 0.025;
	//color.rgb += ( cutoff * 2.0 - color.rgb ) * saturate( cutoff * 2 - color.rgb ) * ( 0.25 / cutoff ) - cutoff;
	//color.rgb = color.rgb * ( float3( 0.5 ) + 6.2 * color.rgb ) / ( float3( 0.06 ) + color.rgb * ( float3( 1.7 ) + 6.2 * color.rgb ) );
	
#elif OPERATOR == 2

	// can be in range [-4.0 .. 4.0]
	//float exposureOffset = rpScreenCorrectionFactor.w;
	
	float avgLuminance = max( hdrAverageLuminance, 0.001 );
	float linearExposure = ( hdrKey / avgLuminance );
	float exposure = log2( max( linearExposure, 0.0001 ) );
	
	//exposure = -2.0;
	float3 exposedColor = exp2( exposure ) * color.rgb;
	
	color.rgb = ACESFilm( exposedColor );
	
#elif OPERATOR == 3

	// can be in range [-4.0 .. 4.0]
	float exposure = rpScreenCorrectionFactor.w;
	
	// exposure curves ranges from 0.0625 to 16.0
	float3 exposedColor = exp2( exposure ) * color.rgb;
	//float3 exposedColor = exposure * color.rgb;
	
	float3 curr = ACESFilm( exposedColor );
	
	float3 whiteScale = 1.0 / ACESFilm( float3( Ymax ) );
	color.rgb = curr * whiteScale;
	
#elif OPERATOR == 4
	// Uncharted 2 tone mapping based on Kodak film curve

	//float exposure = ( hdrKey / hdrAverageLuminance ) * 0.2;
	//float exposure = Yr * 1.0;
	float exposure = rpScreenCorrectionFactor.w;
	float3 exposedColor = exposure * color.rgb;
	
	float3 curr = Uncharted2Tonemap( exposedColor );
	
	float3 whiteScale = 1.0 / Uncharted2Tonemap( float3( Ymax ) );
	color.rgb = curr * whiteScale;
#endif
	
#if defined(BRIGHTPASS)
	// adjust contrast
	//L = pow( L, 1.32 );
	
	const half hdrContrastThreshold = rpOverbright.x;
	const half hdrContrastOffset = rpOverbright.y;
	
	//float T = max( ( Yr * ( 1.0 + Yr / ( Ymax * Ymax * 2.0 ) ) ) - hdrContrastThreshold, 0.0 );
	//float T = max( 1.0 - exp( -Yr ) - hdrContrastThreshold, 0.0 );
	float T = max( Yr - hdrContrastThreshold, 0.0 );
	
	float B = T > 0.0 ? T / ( hdrContrastOffset + T ) : T;
	
	color.rgb *= clamp( B, 0.0, 1.0 );
#endif
	
#if 1
	// convert from linear RGB to sRGB

	//float hdrGamma = 2.2;
	gamma = 1.0 / hdrGamma;
	color.r = pow( color.r, gamma );
	color.g = pow( color.g, gamma );
	color.b = pow( color.b, gamma );
#endif
	
#if defined(HDR_DEBUG)
	//color = tex2D( samp1, float2( L, 0.0 ) );
	color = tex2D( samp1, float2( dot( LUMINANCE_SRGB, color ), 0.0 ) );
#endif

	result.color = color;
	
#if 0
	result.color = float4( L, L, L, 1.0 );
#endif
}
