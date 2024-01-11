/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2023 Robert Beckebans

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
Texture2D t_BaseColor	: register( t0 VK_DESCRIPTOR_SET( 0 ) );
Texture2D t_BlueNoise	: register( t1 VK_DESCRIPTOR_SET( 0 ) );

SamplerState samp0		: register(s0 VK_DESCRIPTOR_SET( 1 ) );
SamplerState samp1		: register(s1 VK_DESCRIPTOR_SET( 1 ) ); // blue noise 256

struct PS_IN
{
	float4 position : SV_Position;
	float2 texcoord0 : TEXCOORD0_centroid;
};

struct PS_OUT
{
	float4 color : SV_Target0;
};
// *INDENT-ON*


#define RESOLUTION_DIVISOR				4.0
#define Dithering_QuantizationSteps     8.0 // 8.0 = 2 ^ 3 quantization bits

float3 Quantize( float3 color, float3 period )
{
	return floor( color * Dithering_QuantizationSteps ) * ( 1.0 / ( Dithering_QuantizationSteps - 1.0 ) );

	//return floor( ( color + period / 2.0 ) / period ) * period;
}


float3 BlueNoise3( float2 n, float x )
{
	float2 uv = n.xy * rpJitterTexOffset.xy;

	float3 noise = t_BlueNoise.Sample( samp1, uv ).rgb;

	noise = frac( noise + c_goldenRatioConjugate * rpJitterTexOffset.w * x );

	noise.x = RemapNoiseTriErp( noise.x );
	noise.y = RemapNoiseTriErp( noise.y );
	noise.z = RemapNoiseTriErp( noise.z );

	noise = noise * 2.0 - 1.0;

	return noise;
}

#define RGB(r, g, b) float3(float(r)/255.0, float(g)/255.0, float(b)/255.0)

void main( PS_IN fragment, out PS_OUT result )
{
	float2 uv = ( fragment.texcoord0 );
	float2 uvPixelated = floor( fragment.position.xy / RESOLUTION_DIVISOR ) * RESOLUTION_DIVISOR;

	// the Sega Mega Drive has a 9 bit HW palette making a total of 512 available colors
	// that is 3 bit per RGB channel
	// 2^3 = 8
	// 8 * 8 * 8 = 512 colors
	// although only 61 colors were available on the screen at the same time but we ignore this for now

	const int quantizationSteps = Dithering_QuantizationSteps;
	float3 quantizationPeriod = _float3( 1.0 / ( quantizationSteps - 1 ) );

	// get pixellated base color
	float3 color = t_BaseColor.Sample( samp0, uvPixelated * rpWindowCoord.xy ).rgb;

	float2 uvDither = uvPixelated;
	//if( rpJitterTexScale.x > 1.0 )
	{
		uvDither = fragment.position.xy / ( RESOLUTION_DIVISOR / rpJitterTexScale.x );
	}
	float dither = DitherArray8x8( uvDither ) - 0.5;

#if 0
	if( uv.y < 0.0625 )
	{
		color = HSVToRGB( float3( uv.x, 1.0, uv.y * 16.0 ) );

		result.color = float4( color, 1.0 );
		return;
	}
	else if( uv.y < 0.125 )
	{
		// quantized
		color = HSVToRGB( float3( uv.x, 1.0, ( uv.y - 0.0625 ) * 16.0 ) );
		color = Quantize( color, quantizationPeriod );

		result.color = float4( color, 1.0 );
		return;
	}
	else if( uv.y < 0.1875 )
	{
		// dithered quantized
		color = HSVToRGB( float3( uv.x, 1.0, ( uv.y - 0.125 ) * 16.0 ) );

		color.rgb += float3( dither, dither, dither ) * quantizationPeriod;
		color = Quantize( color, quantizationPeriod );

		result.color = float4( color, 1.0 );
		return;
	}
	else if( uv.y < 0.25 )
	{
		color = _float3( uv.x );
		color = Quantize( color, quantizationPeriod );
	}
#endif

	color.rgb += float3( dither, dither, dither ) * quantizationPeriod;// * rpJitterTexScale.y;

	// find closest color match from Sega Mega Drive color palette
	color = Quantize( color, quantizationPeriod );

	result.color = float4( color, 1.0 );
}
