/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2024 Robert Beckebans

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
#define NUM_COLORS 32 // original 27


float LinearTweak1( float c )
{
	return ( c <= 0.04045 ) ? c / 12.92 : pow( ( c + 0.055 ) / 1.055, 1.4 );
}

float3 LinearTweak3( float3 c )
{
	return float3( Linear1( c.r ), Linear1( c.g ), Linear1( c.b ) );
}

// find nearest palette color using Euclidean distance
float3 LinearSearch( float3 c, float3 pal[NUM_COLORS] )
{
	int idx = 0;
	float nd = distance( c, pal[0] );

	for( int i = 1; i <	NUM_COLORS; i++ )
	{
		//float d = distance( c, pal[i] );
		float d = distance( c, ( pal[i] ) );

		if( d < nd )
		{
			nd = d;
			idx = i;
		}
	}

	return pal[idx];
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
	float dither = ( DitherArray8x8( uvDither ) - 0.5 ) * 1.0;

	color.rgb += float3( dither, dither, dither ) * quantizationPeriod;


#if 1
	// Amstrad CPC colors https://www.cpcwiki.eu/index.php/CPC_Palette
	const float3 palette[NUM_COLORS] =
	{
		RGB( 0, 0, 0 ),			// black
		RGB( 0, 0, 128 ),		// blue
		RGB( 0, 0, 255 ),		// bright blue
		RGB( 128, 0, 0 ),		// red
		RGB( 128, 0, 128 ),		// magenta
		RGB( 128, 0, 255 ),		// mauve
		RGB( 255, 0, 0 ),		// bright red
		RGB( 255, 0, 128 ),		// purple
		RGB( 255, 0, 255 ),		// bright magenta
		RGB( 0, 128, 0 ),		// green
		RGB( 0, 128, 128 ),		// cyan
		RGB( 0, 128, 255 ),		// sky blue
		RGB( 128, 128, 0 ),		// yellow
		RGB( 128, 128, 128 ),	// white
		RGB( 128, 128, 255 ),	// pastel blue
		RGB( 255, 128, 0 ),		// orange
		RGB( 255, 128, 128 ),	// pink
		RGB( 255, 128, 255 ),	// pastel magenta
		RGB( 0, 255, 0 ),		// bright green
		RGB( 0, 255, 128 ),		// sea green
		RGB( 0, 255, 255 ),		// bright cyan
		RGB( 128, 255, 0 ),		// lime
		RGB( 128, 255, 128 ),	// pastel green
		RGB( 128, 255, 255 ),	// pastel cyan
		RGB( 255, 255, 0 ),		// bright yellow
		RGB( 255, 255, 128 ),	// pastel yellow
		RGB( 255, 255, 255 ),	// bright white

		//RGB( 79, 69, 0 ),		// brown
		//RGB( 120, 120, 120 ),	// dark grey
		//RGB( 164, 215, 142 ),	// grey

#if 0
		RGB( 68,  68,  68 ),	// dark grey
		RGB( 80, 80, 80 ),
		RGB( 108, 108, 108 ),	// grey
		RGB( 120, 120, 120 ),
		RGB( 149, 149, 149 ),	// light grey
#else
		//RGB( 4, 4, 4 ),			// black
		RGB( 16, 16, 16 ),		// black
		RGB( 0, 28, 28 ),		// dark cyan
		RGB( 128, 0, 255 ) * 0.9,		// mauve
		RGB( 111, 79,  37 ) * 1.2,	// orange
		//RGB( 149, 149, 149 ),	// light grey
		//RGB( 154, 210, 132 ),	// light green
		RGB( 112, 164, 178 ) * 1.3,	// cyan
#endif
	};

#elif 0
	// Tweaked LOSPEC CPC BOY PALETTE which is less saturated by Arne Niklas Jansson
	// https://lospec.com/palette-list/cpc-boy

	const float3 palette[NUM_COLORS] = // 32
	{
		RGB( 0, 0, 0 ),
		RGB( 27, 27, 101 ),
		RGB( 53, 53, 201 ),
		RGB( 102, 30, 37 ),
		RGB( 85, 51, 97 ),
		RGB( 127, 53, 201 ),
		RGB( 188, 53, 53 ),
		RGB( 192, 70, 110 ),
		RGB( 223, 109, 155 ),
		RGB( 27, 101, 27 ),
		RGB( 27, 110, 131 ),
		RGB( 30, 121, 229 ),
		RGB( 121, 95, 27 ),
		RGB( 128, 128, 128 ),
		RGB( 145, 148, 223 ),
		RGB( 201, 127, 53 ),
		RGB( 227, 155, 141 ),
		RGB( 248, 120, 248 ),
		RGB( 53, 175, 53 ),
		RGB( 53, 183, 143 ),
		RGB( 53, 193, 215 ),
		RGB( 127, 201, 53 ),
		RGB( 173, 200, 170 ),
		RGB( 141, 225, 199 ),
		RGB( 225, 198, 67 ),
		RGB( 228, 221, 154 ),
		RGB( 255, 255, 255 ),
		RGB( 238, 234, 224 ),
		RGB( 172, 181, 107 ),
		RGB( 118, 132, 72 ),
		RGB( 63, 80, 63 ),
		RGB( 36, 49, 55 ),
	};

#elif 1

	// NES 1 very good
	// https://lospec.com/palette-list/nintendo-entertainment-system

	const float3 palette[NUM_COLORS] = // 55
	{
		RGB( 0, 0, 0 ),
		//RGB( 0, 0, 0 ),
		//RGB( 0, 0, 0 ),
		//RGB( 0, 0, 0 ),
		//RGB( 0, 0, 0 ),
		//RGB( 0, 0, 0 ),
		//RGB( 0, 0, 0 ),
		//RGB( 0, 0, 0 ),
		//RGB( 0, 0, 0 ),
		//RGB( 0, 0, 0 ),

		RGB( 252, 252, 252 ),
		RGB( 248, 248, 248 ),
		RGB( 188, 188, 188 ),
		RGB( 124, 124, 124 ),
		RGB( 164, 228, 252 ),
		RGB( 60, 188, 252 ),
		RGB( 0, 120, 248 ),
		RGB( 0, 0, 252 ),
		RGB( 184, 184, 248 ),
		RGB( 104, 136, 252 ),
		RGB( 0, 88, 248 ),
		RGB( 0, 0, 188 ),
		RGB( 216, 184, 248 ),
		RGB( 152, 120, 248 ),
		RGB( 104, 68, 252 ),
		RGB( 68, 40, 188 ),
		RGB( 248, 184, 248 ),
		RGB( 248, 120, 248 ),
		RGB( 216, 0, 204 ),
		RGB( 148, 0, 132 ),
		RGB( 248, 164, 192 ),
		RGB( 248, 88, 152 ),
		RGB( 228, 0, 88 ),
		RGB( 168, 0, 32 ),
		RGB( 240, 208, 176 ),
		RGB( 248, 120, 88 ),
		RGB( 248, 56, 0 ),
		RGB( 168, 16, 0 ),
		RGB( 252, 224, 168 ),
		RGB( 252, 160, 68 ),
		RGB( 228, 92, 16 ),
		RGB( 136, 20, 0 ),
		RGB( 248, 216, 120 ),
		RGB( 248, 184, 0 ),
		RGB( 172, 124, 0 ),
		RGB( 80, 48, 0 ),
		RGB( 216, 248, 120 ),
		RGB( 184, 248, 24 ),
		RGB( 0, 184, 0 ),
		RGB( 0, 120, 0 ),
		RGB( 184, 248, 184 ),
		RGB( 88, 216, 84 ),
		RGB( 0, 168, 0 ),
		RGB( 0, 104, 0 ),
		RGB( 184, 248, 216 ),
		RGB( 88, 248, 152 ),
		RGB( 0, 168, 68 ),
		RGB( 0, 88, 0 ),
		RGB( 0, 252, 252 ),
		RGB( 0, 232, 216 ),
		RGB( 0, 136, 136 ),
		RGB( 0, 64, 88 ),
		RGB( 248, 216, 248 ),
		RGB( 120, 120, 120 ),
	};

#elif 0

	const float3 palette[NUM_COLORS] = // 32
	{
		RGB( 0, 0, 0 ),
		RGB( 192, 64, 80 ),
		RGB( 240, 240, 240 ),
		RGB( 192, 192, 176 ),
		RGB( 128, 144, 144 ),
		RGB( 96, 96, 112 ),
		RGB( 96, 64, 64 ),
		RGB( 64, 48, 32 ),
		RGB( 112, 80, 48 ),
		RGB( 176, 112, 64 ),
		RGB( 224, 160, 80 ),
		RGB( 224, 192, 128 ),
		RGB( 240, 224, 96 ),
		RGB( 224, 128, 48 ),
		RGB( 208, 80, 32 ),
		RGB( 144, 48, 32 ),
		RGB( 96, 48, 112 ),
		RGB( 176, 96, 160 ),
		RGB( 224, 128, 192 ),
		RGB( 192, 160, 208 ),
		RGB( 112, 112, 192 ),
		RGB( 48, 64, 144 ),
		RGB( 32, 32, 64 ),
		RGB( 32, 96, 208 ),
		RGB( 64, 160, 224 ),
		RGB( 128, 208, 224 ),
		RGB( 160, 240, 144 ),
		RGB( 48, 160, 96 ),
		RGB( 48, 64, 48 ),
		RGB( 48, 112, 32 ),
		RGB( 112, 160, 48 ),
		RGB( 160, 208, 80 ),
	};

#elif 0

	// Gameboy
	const float3 palette[NUM_COLORS] = // 4
	{
		RGB( 27, 42, 9 ),
		RGB( 14, 69, 11 ),
		RGB( 73, 107, 34 ),
		RGB( 154, 158, 63 ),
	};

#else

	// https://lospec.com/palette-list/existential-demo
	const float3 palette[NUM_COLORS] = // 8
	{
		RGB( 248, 243, 253 ),
		RGB( 250, 198, 180 ),
		RGB( 154, 218, 231 ),
		RGB( 151, 203, 29 ),
		RGB( 93, 162, 202 ),
		RGB( 218, 41, 142 ),
		RGB( 11, 134, 51 ),
		RGB( 46, 43, 18 ),
	};

#endif

	// find closest color match from CPC color palette
	color = LinearSearch( color.rgb, palette );

	result.color = float4( color, 1.0 );
}
