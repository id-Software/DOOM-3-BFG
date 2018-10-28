/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company. 

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

/*

This shader will cover a square block of pixel coordinates, but some of them might
be scissored off if the edges of the screen or the midpoint divider are visible through
the optics.

*/

uniform sampler2D samp0 : register(s0);

struct PS_IN {
	vec4 texcoord0	: TEXCOORD0_centroid;
};


struct PS_OUT {
	float4 color : COLOR;
};

void main( PS_IN fragment, out PS_OUT result ) {
    const float screenWarp_range   = 1.45;

    const vec2    warpCenter = vec2( 0.5, 0.5 );
    vec2    centeredTexcoord = fragment.texcoord0.xy - warpCenter;

	float	radialLength = length( centeredTexcoord );
	vec2	radialDir = normalize( centeredTexcoord );

	// get it down into the 0 - PI/2 range
	float	range = screenWarp_range;
	float	scaledRadialLength = radialLength * range;
	float	tanScaled = tan( scaledRadialLength );

    float   rescaleValue = tan( 0.5 * range );

    // If radialLength was 0.5, we want rescaled to also come out
    // as 0.5, so the edges of the rendered image are at the edges
    // of the warped image.
	float	rescaled = tanScaled / rescaleValue;

    vec2	warped = warpCenter + vec2( 0.5, 0.5 ) * radialDir * rescaled;

	result.color = tex2D( samp0, warped );
}
