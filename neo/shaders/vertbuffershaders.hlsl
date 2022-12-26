/*
* Copyright (c) 2014-2021, NVIDIA CORPORATION. All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*/

#include "global_inc.hlsl"

#pragma pack_matrix(row_major)

// *INDENT-OFF*
struct VS_INPUT
{
	float3 pos		: POSITION;
	float4 color	: COLOR;
	float2 uv		: TEXCOORD0;
};

struct PS_INPUT
{
	float4 pos		: SV_POSITION;
	float4 color	: COLOR0;
	float2 uv		: TEXCOORD0;
};
// *INDENT-ON*

PS_INPUT main_vs( VS_INPUT input )
{
	float4 pos = float4( input.pos, 1 );

	PS_INPUT output;

	output.pos.x = dot4( pos, rpMVPmatrixX );
	output.pos.y = dot4( pos, rpMVPmatrixY );
	output.pos.z = dot4( pos, rpMVPmatrixZ );
	output.pos.w = dot4( pos, rpMVPmatrixW );

	output.color = swizzleColor( input.color );
	output.uv = input.uv;

	return output;
}

Texture2D t_Texture :
register( t0 );
SamplerState s_Sampler :
register( s0 );

float4 main_ps( PS_INPUT input ) : SV_Target
{
	float4 color = t_Texture.Sample( s_Sampler, input.uv ) * input.color;

	clip( color.a - rpAlphaTest.x );

	return sRGBAToLinearRGBA( color );
}
