/*
===========================================================================

newpixie CRT

Copyright (c) 2016 Mattias Gustavsson
Copyright (c) 2023 Robert Beckebans

MIT License

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

===========================================================================
*/

#include <global_inc.hlsl>


// *INDENT-OFF*
Texture2D t_CurrentRender	: register( t0 VK_DESCRIPTOR_SET( 0 ) );
Texture2D t_BlueNoise		: register( t1 VK_DESCRIPTOR_SET( 0 ) );

SamplerState LinearSampler	: register(s0 VK_DESCRIPTOR_SET( 1 ) );
SamplerState samp1			: register(s1 VK_DESCRIPTOR_SET( 1 ) ); // blue noise 256

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
	//float2 uv = ( fragment.texcoord0 );

	//float4 color = t_Screen.Sample( samp0, uv );

	float2 iResolution = rpWindowCoord.zw;
	float iTime = rpJitterTexOffset.x;

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
