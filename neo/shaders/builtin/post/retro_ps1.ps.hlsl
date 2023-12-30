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


#define RESOLUTION_DIVISOR 4.0
#define Dithering_QuantizationSteps        32.0 // 8.0 = 2 ^ 3 quantization bits

float3 Quantize( float3 color, float3 period )
{
	return floor( color * Dithering_QuantizationSteps ) * ( 1.0 / ( Dithering_QuantizationSteps - 1.0 ) );

	//return floor( ( color + period / 2.0 ) / period ) * period;
}


void main( PS_IN fragment, out PS_OUT result )
{
	float2 uv = ( fragment.texcoord0 );
	float2 uvPixellated = floor( fragment.position.xy / RESOLUTION_DIVISOR ) * RESOLUTION_DIVISOR;

	// most Sony Playstation 1 titles used 5 bit per RGB channel
	// 2^5 = 32
	// 32 * 32 * 32 = 32768 colors

	const int quantizationSteps = Dithering_QuantizationSteps;
	float3 quantizationPeriod = _float3( 1.0 / ( quantizationSteps - 1 ) );

	// get pixellated base color
	float3 color = t_BaseColor.Sample( samp0, uvPixellated * rpWindowCoord.xy ).rgb;

	// add Bayer 8x8 dithering
#if 0
	// this looks awesome with 3 bits per color channel
	float2 uvDither = fragment.position.xy / ( RESOLUTION_DIVISOR / 2.0 );
#else
	// more faithful to the PSX look
	float2 uvDither = fragment.position.xy / RESOLUTION_DIVISOR;
#endif

	float dither = DitherArray8x8( uvDither ) - 0.5;

#if 0
	if( uv.y < 0.05 )
	{
		color = _float3( uv.x );
	}
	else if( uv.y < 0.1 )
	{
		// quantized signal
		color = _float3( uv.x );
		color = Quantize( color, quantizationPeriod );
	}
	else if( uv.y < 0.15 )
	{
		// quantized signal dithered
		color = _float3( uv.x );
		color = Quantize( color, quantizationPeriod );

		color.rgb += float3( dither, dither, dither ) * quantizationPeriod;
	}
	else if( uv.y < 0.2 )
	{
		color.rgb = float3( dither, dither, dither ) * quantizationPeriod;
	}
	else
#endif
	{
		color.rgb += float3( dither, dither, dither ) * quantizationPeriod;

		// PSX color quantization
		color = Quantize( color, quantizationPeriod );
	}



	//color = t_BaseColor.Sample( samp0, uv ).rgb;

	result.color = float4( color, 1.0 );
}
