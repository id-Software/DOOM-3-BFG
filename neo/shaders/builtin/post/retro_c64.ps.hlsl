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


float3 BlueNoise3( float2 n, float x )
{
	float2 uv = n.xy * rpJitterTexOffset.xy;

	float3 noise = t_BlueNoise.Sample( samp1, uv ).rgb;

	noise = frac( noise + c_goldenRatioConjugate * rpJitterTexOffset.w * x );

	return noise;
}

#define RESOLUTION_DIVISOR 4.0
#define NUM_COLORS 16

// find nearest palette color using Euclidean distance
float4 EuclidDist( float3 c, float3 pal[NUM_COLORS] )
{
	int idx = 0;
	float nd = distance( c, pal[0] );

	for( int i = 1; i <	NUM_COLORS; i++ )
	{
		float d = distance( c, pal[i] );

		if( d < nd )
		{
			nd = d;
			idx = i;
		}
	}

	return float4( pal[idx], 1.0 );
}


#define RGB(r, g, b) float3(float(r)/255.0, float(g)/255.0, float(b)/255.0)

void main( PS_IN fragment, out PS_OUT result )
{
	float2 uv = ( fragment.texcoord0 );
	float2 uvPixellated = floor( fragment.position.xy / RESOLUTION_DIVISOR ) * RESOLUTION_DIVISOR;
	//float2 uvPixellated = fragment.position.xy;

	float4 color = t_BaseColor.Sample( samp0, uv );

	float3 quantizationPeriod = _float3( 1.0 / NUM_COLORS );

	// get pixellated base color
	float3 dc = t_BaseColor.Sample( samp0, uvPixellated * rpWindowCoord.xy ).rgb;
	color.rgb = dc;

	// add Bayer 8x8 dithering
	float dither = DitherArray8x8( uvPixellated );
	color.rgb += ( float3( dither, dither, dither ) * quantizationPeriod );


	// C64 colors http://unusedino.de/ec64/technical/misc/vic656x/colors/
#if 0
	const float3 palette[NUM_COLORS] =
	{
		RGB( 0, 0, 0 ),
		RGB( 255, 255, 255 ),
		RGB( 116, 67, 53 ),
		RGB( 124, 172, 186 ),
		RGB( 123, 72, 144 ),
		RGB( 100, 151, 79 ),
		RGB( 64, 50, 133 ),
		RGB( 191, 205, 122 ),
		RGB( 123, 91, 47 ),
		RGB( 79, 69, 0 ),
		RGB( 163, 114, 101 ),
		RGB( 80, 80, 80 ),
		RGB( 120, 120, 120 ),
		RGB( 164, 215, 142 ),
		RGB( 120, 106, 189 ),
		RGB( 159, 159, 150 ),
	};
#else
	// gamma corrected version
	const float3 palette[NUM_COLORS] =
	{
		RGB( 0, 0, 0 ),			// black
		RGB( 255, 255, 255 ),	// white
		RGB( 104, 55,  43 ),	// red
		RGB( 112, 164, 178 ),	// cyan
		RGB( 111, 61,  134 ),	// purple
		RGB( 88,  141, 67 ),	// green
		RGB( 53,  40,  121 ),	// blue
		RGB( 184, 199, 111 ),	// yellow
		RGB( 111, 79,  37 ),	// orange
		RGB( 67,  57,  0 ),		// brown
		RGB( 154, 103, 89 ),	// light red
		RGB( 68,  68,  68 ),	// dark grey
		RGB( 108, 108, 108 ),	// grey
		RGB( 154, 210, 132 ),	// light green
		RGB( 108, 94,  181 ),	// light blue
		RGB( 149, 149, 149 ),	// light grey
	};
#endif

	// find closest color match from C64 color palette
	color = EuclidDist( color.rgb, palette );

	result.color = color;
}
