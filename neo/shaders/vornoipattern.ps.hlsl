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


/*
	Rounded Voronoi Borders
	-----------------------

	Fabrice came up with an interesting formula to produce more evenly distributed Voronoi values.
	I'm sure there are more interesting ways to use it, but I like the fact that it facilitates
	the creation of more rounded looking borders. I'm sure there are more sophisticated ways to
	produce more accurate borders, but Fabrice's version is simple and elegant.

	The process is explained below. The link to the original is below also.

	I didn't want to cloud the example with too much window dressing, so just for fun, I tried
	to pretty it up by using as little code as possible.

	// 2D version
	2D trabeculum - FabriceNeyret2
	https://www.shadertoy.com/view/4dKSDV

	// 3D version
	hypertexture - trabeculum - FabriceNeyret2
	https://www.shadertoy.com/view/ltj3Dc

	// Straight borders - accurate geometric solution.
	Voronoi - distances - iq
	https://www.shadertoy.com/view/ldl3W8

*/

#include "global_inc.hlsl"


// *INDENT-OFF*

struct PS_IN {
    float4 position     : SV_Position;
    float4 texcoord0    : TEXCOORD0;
    float4 texcoord1    : TEXCOORD1;
};

struct PS_OUT {
    float4 color : SV_Target;
};
// *INDENT-ON*


// float2 to float2 hash.
float2 hash22( float2 p, float iTime )
{

	// Faster, but doesn't disperse things quite as nicely as other combinations. :)
	float n = sin( dot( p, float2( 41, 289 ) ) );
	//return fract(float2(262144, 32768)*n)*.75 + .25;

	// Animated.
	p = frac( float2( 262144, 32768 ) * n );
	return sin( p * 6.2831853 + iTime ) * .35 + .65;

}

// IQ's polynomial-based smooth minimum function.
float smin( float a, float b, float k )
{
	float h = clamp( .5 + .5 * ( b - a ) / k, 0., 1. );
	return lerp( b, a, h ) - k * h * ( 1. - h );
}

// 2D 3rd-order Voronoi: This is just a rehash of Fabrice Neyret's version, which is in
// turn based on IQ's original. I've simplified it slightly, and tidied up the "if-statements,"
// but the clever bit at the end came from Fabrice.
//
// Using a bit of science and art, Fabrice came up with the following formula to produce a more
// rounded, evenly distributed, cellular value:

// d1, d2, d3 - First, second and third closest points (nodes).
// val = 1./(1./(d2 - d1) + 1./(d3 - d1));
//
float Voronoi( in float2 p, float iTime )
{

	float2 g = floor( p ), o;
	p -= g;

	float3 d = _float3( 1 ); // 1.4, etc.

	float r = 0.;

	for( int y = -1; y <= 1; y++ )
	{
		for( int x = -1; x <= 1; x++ )
		{

			o = float2( x, y );
			o += hash22( g + o, iTime ) - p;

			r = dot( o, o );

			// 1st, 2nd and 3rd nearest squared distances.
			d.z = max( d.x, max( d.y, min( d.z, r ) ) ); // 3rd.
			d.y = max( d.x, min( d.y, r ) ); // 2nd.
			d.x = min( d.x, r ); // Closest.

		}
	}

	d = sqrt( d ); // Squared distance to distance.

	// Fabrice's formula.
	return min( 2. / ( 1. / max( d.y - d.x, .001 ) + 1. / max( d.z - d.x, .001 ) ), 1. );
	// Dr2's variation - See "Voronoi Of The Week": https://www.shadertoy.com/view/lsjBz1
	//return min(smin(d.z, d.y, .2) - d.x, 1.);

}

float2 hMap( float2 uv, float iTime )
{

	// Plain Voronoi value. We're saving it and returning it to use when coloring.
	// It's a little less tidy, but saves the need for recalculation later.
	float h = Voronoi( uv * 6., iTime );

	// Adding some bordering and returning the result as the height map value.
	float c = smoothstep( 0., fwidth( h ) * 2., h - .09 ) * h;
	c += ( 1. - smoothstep( 0., fwidth( h ) * 3., h - .22 ) ) * c * .5;

	// Returning the rounded border Voronoi, and the straight Voronoi values.
	return float2( c, h );

}

void main( PS_IN fragment, out PS_OUT result )
{
	float2 iResolution = fragment.texcoord1.yz;

	float iTime = fragment.texcoord1.x;

	// Moving screen coordinates.
	float2 uv = fragment.position.xy / iResolution.y + float2( -.2, .05 ) * iTime;

	// Obtain the height map (rounded Voronoi border) value, then another nearby.
	float2 c = hMap( uv, iTime );
	float2 c2 = hMap( uv + .004, iTime );

	// Take a factored difference of the values above to obtain a very, very basic gradient value.
	// Ie. - a measurement of the bumpiness, or bump value.
	float b = max( c2.x - c.x, 0. ) * 16.;

	// Use the height map value to produce some color. It's all made up on the spot, so don't pay it
	// too much attention.
	//
	float3 col = float3( 1, .05, .25 ) * c.x; // Red base.
	float sv = Voronoi( uv * 16. + c.y, iTime ) * .66 + ( 1. - Voronoi( uv * 48. + c.y * 2., iTime ) ) * .34; // Finer overlay pattern.
	col = col * .85 + float3( 1, .7, .5 ) * sv * sqrt( sv ) * .3; // Mix in a little of the overlay.
	col += ( 1. - col ) * ( 1. - smoothstep( 0., fwidth( c.y ) * 3., c.y - .22 ) ) * c.x; // Highlighting the border.
	col *= col; // Ramping up the contrast, simply because the deeper color seems to look better.

	// Taking a pattern sample a little off to the right, ramping it up, then combining a bit of it
	// with the color above. The result is the flecks of yellowy orange that you see. There's no physics
	// behind it, but the offset tricks your eyes into believing something's happening. :)
	sv = col.x * Voronoi( uv * 6. + .5, iTime );
	col += float3( .7, 1, .3 ) * pow( sv, 4. ) * 8.;

	// Apply the bump - or a powered variation of it - to the color for a bit of highlighting.
	col += float3( .5, .7, 1 ) * ( b * b * .5 + b * b * b * b * .5 );

	// Basic gamma correction
	result.color = float4( sqrt( clamp( col, 0., 1. ) ).xyz, 1 );
}