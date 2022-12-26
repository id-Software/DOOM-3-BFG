/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Loosely based on postprocessing shader by inigo quilez, License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

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

=================================================a==========================
*/

#include "global_inc.hlsl"


// *INDENT-OFF*

Texture2D t_CurrentRender : register( t0 VK_DESCRIPTOR_SET( 0 ) );

SamplerState LinearSampler : register( s0 VK_DESCRIPTOR_SET( 1 ) );

struct PS_IN 
{
    float4 position     : SV_Position;
	float4 texcoord0	: TEXCOORD0_centroid;
	float4 texcoord1	: TEXCOORD1_centroid;
};

struct PS_OUT 
{
    float4 color : SV_Target;
};
// *INDENT-ON*

float2 curve( float2 uv )
{
	uv = ( uv - 0.5 ) * 2.0;
	uv *= 1.1;
	uv.x *= 1.0 + pow( ( abs( uv.y ) / 5.0 ), 2.0 );
	uv.y *= 1.0 + pow( ( abs( uv.x ) / 4.0 ), 2.0 );
	uv  = ( uv / 2.0 ) + 0.5;
	uv =  uv * 0.92 + 0.04;
	return uv;
}

void main( PS_IN fragment, out PS_OUT result )
{
	float2 iResolution = fragment.texcoord1.yz;
	float iTime = fragment.texcoord1.x;

	float2 q = fragment.texcoord0.xy;
	q = saturate( q );

	float2 uv = q;
	uv = curve( uv );
	float3 oricol = t_CurrentRender.Sample( LinearSampler, float2( q.x, q.y ) ).xyz;
	float3 col;
	float x =  sin( 0.3 * iTime + uv.y * 21.0 ) * sin( 0.7 * iTime + uv.y * 29.0 ) * sin( 0.3 + 0.33 * iTime + uv.y * 31.0 ) * 0.0017;

	col.r = t_CurrentRender.Sample( LinearSampler, float2( x + uv.x + 0.001, uv.y + 0.001 ) ).x + 0.05;
	col.g = t_CurrentRender.Sample( LinearSampler, float2( x + uv.x + 0.000, uv.y - 0.002 ) ).y + 0.05;
	col.b = t_CurrentRender.Sample( LinearSampler, float2( x + uv.x - 0.002, uv.y + 0.000 ) ).z + 0.05;
	col.r += 0.08 * t_CurrentRender.Sample( LinearSampler, 0.75 * float2( x + 0.025, -0.027 ) + float2( uv.x + 0.001, uv.y + 0.001 ) ).x;
	col.g += 0.05 * t_CurrentRender.Sample( LinearSampler, 0.75 * float2( x + -0.022, -0.02 ) + float2( uv.x + 0.000, uv.y - 0.002 ) ).y;
	col.b += 0.08 * t_CurrentRender.Sample( LinearSampler, 0.75 * float2( x + -0.02, -0.018 ) + float2( uv.x - 0.002, uv.y + 0.000 ) ).z;

	col = clamp( col * 0.6 + 0.4 * col * col * 1.0, 0.0, 1.0 );

	float vig = ( 0.0 + 1.0 * 16.0 * uv.x * uv.y * ( 1.0 - uv.x ) * ( 1.0 - uv.y ) );
	col *= _float3( pow( vig, 0.3 ) );

	col *= float3( 0.95, 1.05, 0.95 );
	col *= 2.8;

	float scans = clamp( 0.35 + 0.35 * sin( 3.5 * iTime + uv.y * iResolution.y * 1.5 ), 0.0, 1.0 );

	float s = pow( scans, 1.7 );
	col = col * _float3( 0.4 + 0.7 * s ) ;

	col *= 1.0 + 0.01 * sin( 110.0 * iTime );
	if( uv.x < 0.0 || uv.x > 1.0 )
	{
		col *= 0.0;
	}
	if( uv.y < 0.0 || uv.y > 1.0 )
	{
		col *= 0.0;
	}

	col *= 1.0 - 0.65 * _float3( clamp( ( fmod( fragment.texcoord0.x, 2.0 ) - 1.0 ) * 2.0, 0.0, 1.0 ) );

	float comp = smoothstep( 0.1, 0.9, sin( iTime ) );

	// Remove the next line to stop cross-fade between original and postprocess
	//col = mix( col, oricol, comp );

	result.color = float4( col.x, col.y, col.z, 1.0 );

	//result.color = tex2D(samp0, fragment.texcoord0.xy );
	//result.color = float4(1.0f, 1.0f, 1.0f, 1.0f);
}