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

#include <blit.cb.h>

#ifdef SPIRV

[[vk::push_constant]] ConstantBuffer<BlitConstants> g_Blit;

#else

cbuffer g_Blit :
register( b0 )
{
	BlitConstants g_Blit;
}

#endif

// *INDENT-OFF*
struct VS_IN
{
	uint iVertex : SV_VertexID;
};

struct VS_OUT
{
	float4 posClip	: SV_Position;
	float2 uv		: UV;
};
// *INDENT-ON*

void main( VS_IN vertex, out VS_OUT result )
{
	uint u = vertex.iVertex & 1;
	uint v = ( vertex.iVertex >> 1 ) & 1;

	float2 src_uv = float2( u, v ) * g_Blit.sourceSize + g_Blit.sourceOrigin;
	float2 dst_uv = float2( u, v ) * g_Blit.targetSize + g_Blit.targetOrigin;

	result.posClip = float4( dst_uv.x * 2 - 1, 1 - dst_uv.y * 2, 0, 1 );
	result.uv = src_uv;
}
