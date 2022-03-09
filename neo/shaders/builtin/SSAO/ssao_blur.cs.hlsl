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

#include <global_inc.hlsl>

struct SsaoConstants
{
	float2      clipToView;
	float2      invQuantizedGbufferSize;

	int2        quantizedViewportOrigin;
	float       amount;
	float       invBackgroundViewDepth;
	float       radiusWorld;
	float       surfaceBias;

	float       radiusToScreen;
	float       powerExponent;
};

// *INDENT-OFF*
SamplerState s_Point : register(s0);
Texture2DArray<float> t_DeinterleavedDepth : register(t0);
#if DIRECTIONAL_OCCLUSION
Texture2DArray<float4> t_DeinterleavedSSAO : register(t1);
RWTexture2D<float4> u_RenderTarget : register(u0);
#else
Texture2DArray<float> t_DeinterleavedSSAO : register(t1);
RWTexture2D<float> u_RenderTarget : register(u0);
#endif

cbuffer c_Ssao : register(b1)
{
    SsaoConstants g_Ssao;
};
// *INDENT-ON*

void divrem( float a, float b, out float div, out float rem )
{
	a = ( a + 0.5 ) / b;
	div = floor( a );
	rem = floor( frac( a ) * b );
}

// Warning: do not change the group size or shared array dimensions without a complete understanding of the data loading method.

#if DIRECTIONAL_OCCLUSION
	groupshared float s_Depth[24][24];
	groupshared float4 s_Occlusion[24][24];
#else
	groupshared float2 s_DepthAndSSAO[24][24];
#endif

[numthreads( 16, 16, 1 )]
void main( uint2 groupId : SV_GroupID, uint2 threadId : SV_GroupThreadID, uint2 globalId : SV_DispatchThreadID )
{
	int linearIdx = int( ( threadId.y << 4 ) + threadId.x );

	if( linearIdx < 144 )
	{
		// Rename the threads to a 3x3x16 grid where X and Y are "offsetUV" and Z is "slice"

		float2 offsetUVf;
		float a, slice;
		divrem( linearIdx, 3.0, a, offsetUVf.x );
		divrem( a, 3.0, slice, offsetUVf.y );

		offsetUVf *= 8;
		int2 offsetUV = int2( offsetUVf );

		int2 pixelPos = ( int2( groupId.xy ) * 16 ) + offsetUV + g_Ssao.quantizedViewportOrigin;

		float2 UV = ( float2( pixelPos ) + 1.0 ) * g_Ssao.invQuantizedGbufferSize.xy;

		// Load 4 pixels from each texture, overall the thread group loads a 24x24 block of pixels.
		// For a 16x16 thread group, 20x20 pixels would be enough, but it's impossible to do that with Gather.

		// Each Gather instruction loads 4 adjacent pixels from a deinterleaved array, and the
		// screen-space distance between those pixels is 4, not 1.

		offsetUV.x += int( slice ) & 3;
		offsetUV.y += int( slice ) >> 2;

		float4 depths = t_DeinterleavedDepth.GatherRed( s_Point, float3( UV, slice ), int2( 0, 0 ) );
#if DIRECTIONAL_OCCLUSION
		int3 deinterleavedPixelPos = int3( ( pixelPos >> 2 ) - 1, slice );
		float4 occlusion_00 = t_DeinterleavedSSAO[deinterleavedPixelPos + int3( 0, 0, 0 )];
		float4 occlusion_10 = t_DeinterleavedSSAO[deinterleavedPixelPos + int3( 1, 0, 0 )];
		float4 occlusion_01 = t_DeinterleavedSSAO[deinterleavedPixelPos + int3( 0, 1, 0 )];
		float4 occlusion_11 = t_DeinterleavedSSAO[deinterleavedPixelPos + int3( 1, 1, 0 )];

		s_Depth[offsetUV.y + 4][offsetUV.x + 0] = depths.x;
		s_Depth[offsetUV.y + 4][offsetUV.x + 4] = depths.y;
		s_Depth[offsetUV.y + 0][offsetUV.x + 4] = depths.z;
		s_Depth[offsetUV.y + 0][offsetUV.x + 0] = depths.w;

		s_Occlusion[offsetUV.y + 4][offsetUV.x + 0] = occlusion_01;
		s_Occlusion[offsetUV.y + 4][offsetUV.x + 4] = occlusion_11;
		s_Occlusion[offsetUV.y + 0][offsetUV.x + 4] = occlusion_10;
		s_Occlusion[offsetUV.y + 0][offsetUV.x + 0] = occlusion_00;
#else
		float4 occlusions = t_DeinterleavedSSAO.GatherRed( s_Point, float3( UV, slice ), int2( 0, 0 ) );

		s_DepthAndSSAO[offsetUV.y + 4][offsetUV.x + 0] = float2( depths.x, occlusions.x );
		s_DepthAndSSAO[offsetUV.y + 4][offsetUV.x + 4] = float2( depths.y, occlusions.y );
		s_DepthAndSSAO[offsetUV.y + 0][offsetUV.x + 4] = float2( depths.z, occlusions.z );
		s_DepthAndSSAO[offsetUV.y + 0][offsetUV.x + 0] = float2( depths.w, occlusions.w );
#endif
	}

	GroupMemoryBarrierWithGroupSync();

	float totalWeight = 0;
#if DIRECTIONAL_OCCLUSION
	float4 totalOcclusion = 0;
	float pixelDepth = s_Depth[threadId.y + 4][threadId.x + 4];
#else
	float totalOcclusion = 0;
	float pixelDepth = s_DepthAndSSAO[threadId.y + 4][threadId.x + 4].x;
#endif
	float rcpPixelDepth = rcp( pixelDepth );

	const bool enableFilter = true;
	const int filterLeft = enableFilter ? 3 : 4;
	const int filterRight = enableFilter ? 6 : 4;
	int2 filterOffset;
	for( filterOffset.y = filterLeft; filterOffset.y <= filterRight; filterOffset.y++ )
		for( filterOffset.x = filterLeft; filterOffset.x <= filterRight; filterOffset.x++ )
		{
#if DIRECTIONAL_OCCLUSION
			float sampleDepth = s_Depth[threadId.y + filterOffset.y][threadId.x + filterOffset.x];
			float4 sampleOcclusion = s_Occlusion[threadId.y + filterOffset.y][threadId.x + filterOffset.x];
#else
			float2 sampleDAO = s_DepthAndSSAO[threadId.y + filterOffset.y][threadId.x + filterOffset.x].xy;
			float sampleDepth = sampleDAO.x;
			float sampleOcclusion = sampleDAO.y;
#endif
			float weight = saturate( 1.0 - abs( pixelDepth - sampleDepth ) * rcpPixelDepth * g_Ssao.radiusWorld );
			totalOcclusion += sampleOcclusion * weight;
			totalWeight += weight;
		}

	totalOcclusion *= rcp( totalWeight );

#if !DIRECTIONAL_OCCLUSION
	totalOcclusion = pow( saturate( 1.0 - totalOcclusion ), g_Ssao.powerExponent );
#endif

	int2 storePos = int2( globalId.xy ) + g_Ssao.quantizedViewportOrigin;
	float2 storePosF = float2( storePos );

	//if (all(storePosF >= g_Ssao.view.viewportOrigin.xy) && all(storePosF < g_Ssao.view.viewportOrigin.xy + g_Ssao.view.viewportSize.xy))
	//{
	//    u_RenderTarget[storePos] = totalOcclusion;
	//}
}
