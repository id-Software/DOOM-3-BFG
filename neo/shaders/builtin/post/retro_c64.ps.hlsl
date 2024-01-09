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
#define NUM_COLORS 16


// find nearest palette color using Euclidean distance
float3 LinearSearch( float3 c, float3 pal[NUM_COLORS] )
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

	return pal[idx];
}

float3 GetClosest( float3 val1, float3 val2, float3 target )
{
	if( distance( target, val1 ) >= distance( val2, target ) )
	{
		return val2;
	}
	else
	{
		return val1;
	}
}

// find nearest palette color using Euclidean disntance and binary search
// this requires an already sorted palette as input
float3 BinarySearch( float3 target, float3 pal[NUM_COLORS] )
{
	float targetY = PhotoLuma( target );

	// left-side case
	if( targetY <= PhotoLuma( pal[0] ) )
	{
		return pal[0];
	}

	// right-side case
	if( targetY >= PhotoLuma( pal[NUM_COLORS - 1] ) )
	{
		return pal[NUM_COLORS - 1];
	}

	int i = 0, j = NUM_COLORS, mid = 0;
	while( i < j )
	{
		mid = ( i + j ) / 2;

		if( distance( pal[mid], target ) < 0.01 )
		{
			return pal[mid];
		}

		// if target is less than array element, then search in left
		if( targetY < PhotoLuma( pal[mid] ) )
		{
			// if target is greater than previous
			// to mid, return closest of two
			if( mid > 0 && targetY > PhotoLuma( pal[mid - 1] ) )
			{
				return GetClosest( pal[mid - 1], pal[mid], target );
			}
			j = mid;
		}
		else
		{
			if( mid < ( NUM_COLORS - 1 ) && targetY < PhotoLuma( pal[mid + 1] ) )
			{
				return GetClosest( pal[mid], pal[mid + 1], target );
			}
			i = mid + 1;
		}
	}

	// only single element left after search
	return pal[mid];
}


#define RGB(r, g, b) float3(float(r)/255.0, float(g)/255.0, float(b)/255.0)

void main( PS_IN fragment, out PS_OUT result )
{
	float2 uv = ( fragment.texcoord0 );
	float2 uvPixelated = floor( fragment.position.xy / RESOLUTION_DIVISOR ) * RESOLUTION_DIVISOR;

	float3 quantizationPeriod = _float3( 1.0 / NUM_COLORS );

	// get pixellated base color
	float3 color = t_BaseColor.Sample( samp0, uvPixelated * rpWindowCoord.xy ).rgb;

#if 0
	if( uv.y < 0.125 )
	{
		color = HSVToRGB( float3( uv.x, 1.0, uv.y * 8.0 ) );
		color = floor( color * NUM_COLORS ) * ( 1.0 / ( NUM_COLORS - 1.0 ) );

		//result.color = float4( color, 1.0 );
		//return;
	}
	else if( uv.y < 0.1875 )
	{
		color = _float3( uv.x );
		color = floor( color * NUM_COLORS ) * ( 1.0 / ( NUM_COLORS - 1.0 ) );
	}
#endif

	// add Bayer 8x8 dithering
	float2 uvDither = uvPixelated;
	//if( rpJitterTexScale.x > 1.0 )
	{
		uvDither = fragment.position.xy / ( RESOLUTION_DIVISOR / rpJitterTexScale.x );
	}
	float dither = DitherArray8x8( uvDither ) - 0.5;

	color.rgb += float3( dither, dither, dither ) * quantizationPeriod;


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
	color = LinearSearch( color.rgb, palette );
	//color = float4( BinarySearch( color.rgb, palette ), 1.0 );

	result.color = float4( color, 1.0 );
}
