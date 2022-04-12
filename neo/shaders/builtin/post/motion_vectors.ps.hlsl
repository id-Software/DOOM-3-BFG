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

#pragma pack_matrix(row_major)

//#include <donut/shaders/taa_cb.h>

struct TemporalAntiAliasingConstants
{
	float4x4 reprojectionMatrix;

	float2 inputViewOrigin;
	float2 inputViewSize;

	float2 outputViewOrigin;
	float2 outputViewSize;

	float2 inputPixelOffset;
	float2 outputTextureSizeInv;

	float2 inputOverOutputViewSize;
	float2 outputOverInputViewSize;

	float clampingFactor;
	float newFrameWeight;
	float pqC;
	float invPqC;

	uint stencilMask;
};

cbuffer c_TemporalAA :
register( b0 )
{
	TemporalAntiAliasingConstants g_TemporalAA;
};

Texture2D<float> t_GBufferDepth :
register( t0 );
#if USE_STENCIL
Texture2D<uint2> t_GBufferStencil :
register( t1 );
#endif

void main(
	in float4 i_position : SV_Position,
	in float2 i_uv : UV,
	out float4 o_color : SV_Target0
)
{
	o_color = 0;

#if USE_STENCIL
	uint stencil = t_GBufferStencil[i_position.xy].y;
	if( ( stencil & g_TemporalAA.stencilMask ) == g_TemporalAA.stencilMask )
	{
		discard;
	}
#endif
	float depth = t_GBufferDepth[i_position.xy].x;

	float4 clipPos;
	clipPos.x = i_uv.x * 2 - 1;
	clipPos.y = 1 - i_uv.y * 2;
	clipPos.z = depth;
	clipPos.w = 1;

	float4 prevClipPos = mul( clipPos, g_TemporalAA.reprojectionMatrix );

	if( prevClipPos.w <= 0 )
	{
		return;
	}

	prevClipPos.xyz /= prevClipPos.w;
	float2 prevUV;
	prevUV.x = 0.5 + prevClipPos.x * 0.5;
	prevUV.y = 0.5 - prevClipPos.y * 0.5;

	float2 prevWindowPos = prevUV * g_TemporalAA.inputViewSize + g_TemporalAA.inputViewOrigin;

	o_color.xy = prevWindowPos.xy - i_position.xy;
}
