/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2014 Robert Beckebans

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
Texture2D t_CurrentRender : register( t0 VK_DESCRIPTOR_SET( 0 ) );
Texture2D t_NormalMap : register( t1 VK_DESCRIPTOR_SET( 0 ) );

SamplerState LinearSampler : register( s0 VK_DESCRIPTOR_SET( 1 ) );

struct PS_IN {
	float4 position		: SV_Position;
	float4 texcoord0	: TEXCOORD0_centroid;
	float4 texcoord1	: TEXCOORD1_centroid;
};

struct PS_OUT {
	float4 color : SV_Target;
};
// *INDENT-ON*

float3 mod289( float3 x )
{
	return x - floor( x * ( 1.0 / 289.0 ) ) * 289.0;
}

float4 mod289( float4 x )
{
	return x - floor( x * ( 1.0 / 289.0 ) ) * 289.0;
}

float4 permute( float4 x )
{
	return mod289( ( ( x * 34.0 ) + 1.0 ) * x );
}

float4 taylorInvSqrt( float4 r )
{
	return 1.79284291400159 - 0.85373472095314 * r;
}

float snoise( float3 v )
{
	const float2  C = float2( 1.0 / 6.0, 1.0 / 3.0 );
	const float4  D = float4( 0.0, 0.5, 1.0, 2.0 );

// First corner
	float3 i  = floor( v + dot( v, C.yyy ) );
	float3 x0 =   v - i + dot( i, C.xxx ) ;

// Other corners
	float3 g = step( x0.yzx, x0.xyz );
	float3 l = 1.0 - g;
	float3 i1 = min( g.xyz, l.zxy );
	float3 i2 = max( g.xyz, l.zxy );

	//   x0 = x0 - 0.0 + 0.0 * C.xxx;
	//   x1 = x0 - i1  + 1.0 * C.xxx;
	//   x2 = x0 - i2  + 2.0 * C.xxx;
	//   x3 = x0 - 1.0 + 3.0 * C.xxx;
	float3 x1 = x0 - i1 + C.xxx;
	float3 x2 = x0 - i2 + C.yyy; // 2.0*C.x = 1/3 = C.y
	float3 x3 = x0 - D.yyy;      // -1.0+3.0*C.x = -0.5 = -D.y

// Permutations
	i = mod289( i );
	float4 p = permute( permute( permute(
									 i.z + float4( 0.0, i1.z, i2.z, 1.0 ) )
								 + i.y + float4( 0.0, i1.y, i2.y, 1.0 ) )
						+ i.x + float4( 0.0, i1.x, i2.x, 1.0 ) );

// Gradients: 7x7 points over a square, mapped onto an octahedron.
// The ring size 17*17 = 289 is close to a multiple of 49 (49*6 = 294)
	float n_ = 0.142857142857; // 1.0/7.0
	float3  ns = n_ * D.wyz - D.xzx;

	float4 j = p - 49.0 * floor( p * ns.z * ns.z ); //  mod(p,7*7)

	float4 x_ = floor( j * ns.z );
	float4 y_ = floor( j - 7.0 * x_ );   // mod(j,N)

	float4 x = x_ * ns.x + ns.yyyy;
	float4 y = y_ * ns.x + ns.yyyy;
	float4 h = 1.0 - abs( x ) - abs( y );

	float4 b0 = float4( x.xy, y.xy );
	float4 b1 = float4( x.zw, y.zw );

	//float4 s0 = float4(lessThan(b0,0.0))*2.0 - 1.0;
	//float4 s1 = float4(lessThan(b1,0.0))*2.0 - 1.0;
	float4 s0 = floor( b0 ) * 2.0 + 1.0;
	float4 s1 = floor( b1 ) * 2.0 + 1.0;
	float4 sh = -step( h, _float4( 0.0 ) );

	float4 a0 = b0.xzyw + s0.xzyw * sh.xxyy ;
	float4 a1 = b1.xzyw + s1.xzyw * sh.zzww ;

	float3 p0 = float3( a0.xy, h.x );
	float3 p1 = float3( a0.zw, h.y );
	float3 p2 = float3( a1.xy, h.z );
	float3 p3 = float3( a1.zw, h.w );

//Normalise gradients
	float4 norm = taylorInvSqrt( float4( dot( p0, p0 ), dot( p1, p1 ), dot( p2, p2 ), dot( p3, p3 ) ) );
	p0 *= norm.x;
	p1 *= norm.y;
	p2 *= norm.z;
	p3 *= norm.w;

// Mix final noise value
	float4 m = max( 0.6 - float4( dot( x0, x0 ), dot( x1, x1 ), dot( x2, x2 ), dot( x3, x3 ) ), 0.0 );
	m = m * m;
	return 42.0 * dot( m * m, float4( dot( p0, x0 ), dot( p1, x1 ),
									  dot( p2, x2 ), dot( p3, x3 ) ) );
}

float normnoise( float noise )
{
	return 0.5 * ( noise + 1.0 );
}

float clouds( float2 uv, float time )
{
	uv += float2( time * 0.05, + time * 0.01 );

	float2 off1 = float2( 50.0, 33.0 );
	float2 off2 = float2( 0.0, 0.0 );
	float2 off3 = float2( -300.0, 50.0 );
	float2 off4 = float2( -100.0, 200.0 );
	float2 off5 = float2( 400.0, -200.0 );
	float2 off6 = float2( 100.0, -1000.0 );
	float scale1 = 3.0;
	float scale2 = 6.0;
	float scale3 = 12.0;
	float scale4 = 24.0;
	float scale5 = 48.0;
	float scale6 = 96.0;
	return normnoise( snoise( float3( ( uv + off1 ) * scale1, time * 0.5 ) ) * 0.8 +
					  snoise( float3( ( uv + off2 ) * scale2, time * 0.4 ) ) * 0.4 +
					  snoise( float3( ( uv + off3 ) * scale3, time * 0.1 ) ) * 0.2 +
					  snoise( float3( ( uv + off4 ) * scale4, time * 0.7 ) ) * 0.1 +
					  snoise( float3( ( uv + off5 ) * scale5, time * 0.2 ) ) * 0.05 +
					  snoise( float3( ( uv + off6 ) * scale6, time * 0.3 ) ) * 0.025 );
}

void main( PS_IN fragment, out PS_OUT result )
{
	float2 iResolution = fragment.texcoord1.yz;

	float time = fragment.texcoord1.x;

	float2 uv =  fragment.position.xy / iResolution.x;

	float2 center = float2( 0.5, 0.5 * ( iResolution.y / iResolution.x ) );

	float2 light1 = float2( sin( time * 1.2 + 50.0 ) * 1.0 + cos( time * 0.4 + 10.0 ) * 0.6, sin( time * 1.2 + 100.0 ) * 0.8 + cos( time * 0.2 + 20.0 ) * -0.2 ) * 0.2 + center;
	float3 lightColor1 = float3( .3, .7, .3 );

	/*
	float2 light2 = float2(sin(time+3.0)*-2.0,cos(time+7.0)*1.0)*0.2+center;
	float3 lightColor2 = float3(0.3, 1.0, 0.3);

	float2 light3 = float2(sin(time+3.0)*2.0,cos(time+14.0)*-1.0)*0.2+center;
	float3 lightColor3 = float3(0.3, 0.3, 1.0);
	*/

	float cloudIntensity1 = 0.7 * ( 1.0 - ( 2.5 * distance( uv, light1 ) ) );
	float lighIntensity1 = 1.0 / ( 100.0 * distance( uv, light1 ) );

	/*
	float cloudIntensity2 = 0.7*(1.0-(2.5*distance(uv, light2)));
	float lighIntensity2 = 1.0/(100.0*distance(uv,light2));

	float cloudIntensity3 = 0.7*(1.0-(2.5*distance(uv, light3)));
	float lighIntensity3 = 1.0/(100.0*distance(uv,light3));
	*/

	result.color = float4( _float3( cloudIntensity1 * clouds( uv, time ) ) * lightColor1 + lighIntensity1 * lightColor1
						   , 1.0 );
}