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


float3 tsample( Texture2D tex, vec2 tc, float offs, vec2 resolution )
{
#if 1
	//tc = tc * vec2( 1.025, 0.92 ) + vec2( -0.0125, 0.04 );
	float3 s = pow( abs( tex.Sample( LinearSampler, vec2( tc.x, 1.0 - tc.y ) ).rgb ), _float3( 2.2 ) );
	return s * _float3( 1.25 );
#else
	float3 s = tex.Sample( LinearSampler, vec2( tc.x, 1.0 - tc.y ) ).rgb;
	return s;
#endif
}

float3 filmic( float3 LinearColor )
{
	float3 x = max( _float3( 0.0 ), LinearColor - _float3( 0.004 ) );
	return ( x * ( 6.2 * x + 0.5 ) ) / ( x * ( 6.2 * x + 1.7 ) + 0.06 );
}

float2 curve( float2 uv, float curvature )
{
	uv = ( uv - 0.5 ) * 2.0;
	uv *= 1.1;
	uv.x *= 1.0 + pow( ( abs( uv.y ) / 5.0 ), 2.0 );
	uv.y *= 1.0 + pow( ( abs( uv.x ) / 4.0 ), 2.0 );
	uv  = ( uv / curvature ) + 0.5;
	uv =  uv * 0.92 + 0.04;
	return uv;
}

float2 curve2( float2 uv, float curvature )
{
	uv = ( uv - 0.5 ) * 2.0;
	uv *= 1.1;
	uv.x *= 1.0 + pow( ( abs( uv.y ) / 4.0 ), 2.0 );
	uv.y *= 1.0 + pow( ( abs( uv.x ) / 3.0 ), 2.0 );
	uv  = ( uv / curvature ) + 0.5;
	uv =  uv * 0.92 + 0.04;
	return uv;
}

void main( PS_IN fragment, out PS_OUT result )
{
	// revised version from RetroArch

	struct Params
	{
		float curvature;
		float ghosting;
		float scanroll;
		float vignette;
		float wiggle_toggle;
		int FrameCount;
	};

	Params params;
	params.curvature = rpWindowCoord.x;
	params.ghosting = 0.0;
	params.scanroll = 1.0;
	params.wiggle_toggle = 0.0;
	params.vignette = rpWindowCoord.y;
	params.FrameCount = int( rpJitterTexOffset.w );

	// stop time variable so the screen doesn't wiggle
	float time = params.FrameCount % 849 * 36.0;
	float2 uv = fragment.texcoord0.xy;
	uv.y = 1.0 - uv.y;
	uv = saturate( uv );

	/* Curve */
	//float2 curved_uv = lerp( curve2( uv, params.curvature ), uv, 0.4 );
	//float scale = -0.101;
	//float2 scuv = curved_uv * ( 1.0 - scale ) + scale / 2.0 + float2( 0.003, -0.001 );

	//uv = scuv;

	float2 curved_uv = uv;

	if( params.curvature > 0.0 )
	{
		curved_uv = curve( uv, 2.0 );
	}
	float2 scuv = curved_uv;

	float2 resolution = rpWindowCoord.zw;

	/* Main color, Bleed */
	float3 col;
	float x = params.wiggle_toggle * sin( 0.1 * time + curved_uv.y * 13.0 ) * sin( 0.23 * time + curved_uv.y * 19.0 ) * sin( 0.3 + 0.11 * time + curved_uv.y * 23.0 ) * 0.0012;

	// make time do something again
	time = float( params.FrameCount % 640 * 1 );

	Texture2D backbuffer = t_CurrentRender;
#if 1
	col.r = tsample( backbuffer, vec2( x + scuv.x + 0.0009, scuv.y + 0.0009 ), resolution.y / 800.0, resolution ).x + 0.02;
	col.g = tsample( backbuffer, vec2( x + scuv.x + 0.0000, scuv.y - 0.0011 ), resolution.y / 800.0, resolution ).y + 0.02;
	col.b = tsample( backbuffer, vec2( x + scuv.x - 0.0015, scuv.y + 0.0000 ), resolution.y / 800.0, resolution ).z + 0.02;
#else
	col.r = t_CurrentRender.Sample( LinearSampler, float2( x + uv.x + 0.001, uv.y + 0.001 ) ).x + 0.05;
	col.g = t_CurrentRender.Sample( LinearSampler, float2( x + uv.x + 0.000, uv.y - 0.002 ) ).y + 0.05;
	col.b = t_CurrentRender.Sample( LinearSampler, float2( x + uv.x - 0.002, uv.y + 0.000 ) ).z + 0.05;
#endif



	/* Ghosting */
#if 1
	{
		float o = sin( -fragment.position.y * 1.5 ) / resolution.x;
		x += o * 0.25;

		float ghs = 0.15 * params.ghosting;
		Texture2D blurbuffer = t_CurrentRender; // FIXME
		float3 r = tsample( blurbuffer, float2( x - 0.014 * 1.0, -0.027 ) * 0.85 + 0.007 * float2( 0.35 * sin( 1.0 / 7.0 + 15.0 * curved_uv.y + 0.9 * time ),
							0.35 * sin( 2.0 / 7.0 + 10.0 * curved_uv.y + 1.37 * time ) ) + float2( scuv.x + 0.001, scuv.y + 0.001 ),
							5.5 + 1.3 * sin( 3.0 / 9.0 + 31.0 * curved_uv.x + 1.70 * time ), resolution ).xyz * float3( 0.5, 0.25, 0.25 );
		float3 g = tsample( blurbuffer, float2( x - 0.019 * 1.0, -0.020 ) * 0.85 + 0.007 * float2( 0.35 * cos( 1.0 / 9.0 + 15.0 * curved_uv.y + 0.5 * time ),
							0.35 * sin( 2.0 / 9.0 + 10.0 * curved_uv.y + 1.50 * time ) ) + float2( scuv.x + 0.000, scuv.y - 0.002 ),
							5.4 + 1.3 * sin( 3.0 / 3.0 + 71.0 * curved_uv.x + 1.90 * time ), resolution ).xyz * float3( 0.25, 0.5, 0.25 );
		float3 b = tsample( blurbuffer, float2( x - 0.017 * 1.0, -0.003 ) * 0.85 + 0.007 * float2( 0.35 * sin( 2.0 / 3.0 + 15.0 * curved_uv.y + 0.7 * time ),
							0.35 * cos( 2.0 / 3.0 + 10.0 * curved_uv.y + 1.63 * time ) ) + float2( scuv.x - 0.002, scuv.y + 0.000 ),
							5.3 + 1.3 * sin( 3.0 / 7.0 + 91.0 * curved_uv.x + 1.65 * time ), resolution ).xyz * float3( 0.25, 0.25, 0.5 );

		float i = clamp( col.r * 0.299 + col.g * 0.587 + col.b * 0.114, 0.0, 1.0 );
		i = pow( 1.0 - pow( i, 2.0 ), 1.0 );
		i = ( 1.0 - i ) * 0.85 + 0.15;

		col += _float3( ghs * ( 1.0 - 0.299 ) ) * pow( clamp( _float3( 3.0 ) * r, _float3( 0.0 ), _float3( 1.0 ) ), _float3( 2.0 ) ) * _float3( i );
		col += _float3( ghs * ( 1.0 - 0.587 ) ) * pow( clamp( _float3( 3.0 ) * g, _float3( 0.0 ), _float3( 1.0 ) ), _float3( 2.0 ) ) * _float3( i );
		col += _float3( ghs * ( 1.0 - 0.114 ) ) * pow( clamp( _float3( 3.0 ) * b, _float3( 0.0 ), _float3( 1.0 ) ), _float3( 2.0 ) ) * _float3( i );
	}
#endif

	/* Level adjustment (curves) */
#if 1
	col *= float3( 0.95, 1.05, 0.95 );
	col = clamp( col * 1.3 + 0.75 * col * col + 1.25 * col * col * col * col * col, _float3( 0.0 ), _float3( 10.0 ) );
#endif

	/* Vignette */
#if 1
	float vig = ( ( 1.0 - 0.99 * params.vignette ) + 1.0 * 16.0 * curved_uv.x * curved_uv.y * ( 1.0 - curved_uv.x ) * ( 1.0 - curved_uv.y ) );
	vig = 1.3 * pow( vig, 0.5 );
	col *= vig;
#endif

	time *= params.scanroll;

	/* Scanlines */
	float scans = clamp( 0.35 + 0.18 * sin( 6.0 * time - curved_uv.y * resolution.y * 1.5 ), 0.0, 1.0 );
	float s = pow( scans, 0.9 );
	col = col * _float3( s );

	/* Vertical lines (shadow mask) */
	col *= 1.0 - 0.23 * ( clamp( ( fragment.position.xy.x % 3.0 ) / 2.0, 0.0, 1.0 ) );

	/* Tone map */
	col = filmic( col );

	/* Noise */
#if 1
	/*float2 seed = floor(curved_uv*resolution.xy*float2(0.5))/resolution.xy;*/
	float2 seed = curved_uv * resolution.xy;
	/* seed = curved_uv; */
	col -= 0.015 * pow( float3( rand( seed + time ), rand( seed + time * 2.0 ), rand( seed + time * 3.0 ) ), _float3( 1.5 ) );
#endif

	/* Flicker */
	col *= ( 1.0 - 0.004 * ( sin( 50.0 * time + curved_uv.y * 2.0 ) * 0.5 + 0.5 ) );

	/* Clamp */
#if 1
	if( curved_uv.x < 0.0 || curved_uv.x > 1.0 )
	{
		col *= 0.0;
	}
	if( curved_uv.y < 0.0 || curved_uv.y > 1.0 )
	{
		col *= 0.0;
	}
#endif

	uv = curved_uv;

#if 1
	/* Frame */
	float2 fscale = float2( 0.026, -0.018 ); //float2( -0.018, -0.013 );
	//uv = float2( uv.x, 1.0 - uv.y );

	//float4 f = texture( frametexture, vTexCoord.xy ); //*((1.0)+2.0*fscale)-fscale-float2(-0.0, 0.005));
	//f.xyz = mix( f.xyz, float3( 0.5, 0.5, 0.5 ), 0.5 );
	float4 f = _float4( 0.5 );
	float fvig = clamp( -0.00 + 512.0 * uv.x * uv.y * ( 1.0 - uv.x ) * ( 1.0 - uv.y ), 0.2, 0.8 );
	//col = lerp( col, lerp( max( col, 0.0 ), pow( abs( f.xyz ), _float3( 1.4 ) ) * fvig, f.w * f.w ), _float3( use_frame ) );
	//col = lerp( col, lerp( max( col, 0.0 ), pow( abs( f.xyz ), _float3( 1.4 ) ) * fvig, f.w * f.w ), _float3( 1.0 ) );

	// Gamma correction since we are not rendering to an sRGB render target.
	col = pow( col, _float3( 1.0 / 1.1 ) );
#endif

	result.color = float4( col, 1.0 );
}
