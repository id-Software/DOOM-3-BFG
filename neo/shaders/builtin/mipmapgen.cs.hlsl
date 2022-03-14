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

#define GROUP_SIZE 16
#define LOD0_TILE_SIZE 8
#define NUM_LODS 4

// Number of compute dispatches needed to reduce all the
// mip-levels at a maximum resolution of 16k :
//     (uint)(std::ceil(std::log2f(16384)/NUM_LODS)) = 4
#define MAX_PASSES 4

#define MODE_COLOR  0
#define MODE_MIN    1
#define MODE_MAX    2
#define MODE_MINMAX 3

struct MipmmapGenConstants
{
	uint dispatch;
	uint numLODs;
	uint padding[2];
};

#if MODE == MODE_COLOR
#define VALUE_TYPE float3
float3 reduce( float3 a, float3 b )
{
	return lerp( a, b, 0.5 );
}
float3 reduce( float3 a, float3 b, float3 c, float3 d )
{
	return reduce( reduce( a, b ), reduce( c, d ) );
}
#elif MODE == MODE_MINMAX
#define VALUE_TYPE float2
float2 reduce( float2 a, float2 b )
{
	return float2( min( a.x, b.x ), max( a.y, b.y ) );
}
float2 reduce( float2 a, float2 b, float2 c, float2 d )
{
	return reduce( reduce( a, b ), reduce( c, d ) );
}
#else
#define VALUE_TYPE float
VALUE_TYPE reduce( VALUE_TYPE a, VALUE_TYPE b )
{
#if MODE == MODE_MIN
	return min( a, b );
#elif MODE == MODE_MAX
	return max( a, b );
#endif
}
VALUE_TYPE reduce( float4 a )
{
	return reduce( reduce( a.x, a.y ), reduce( a.z, a.w ) );
}
#endif

// *INDENT-OFF*
cbuffer c_MipMapgen : register( b0 )
{
	MipmmapGenConstants g_MipMapGen;
};

RWTexture2D<VALUE_TYPE> u_output[NUM_LODS] : register( u0 );
Texture2D<VALUE_TYPE> t_input : register( t0 );
// *INDENT-ON*

groupshared VALUE_TYPE s_ReductionData[GROUP_SIZE][GROUP_SIZE];

[numthreads( 16, 16, 1 )] void main(
	uint2 groupIdx : SV_GroupID,
	uint2 globalIdx : SV_DispatchThreadID,
	uint2 threadIdx : SV_GroupThreadID )
{
	VALUE_TYPE value = t_input.mips[0][globalIdx.xy];

#if MODE == MODE_MINMAX
	if( g_MipMapGen.dispatch == 0 )
	{
		value.y = value.x;
	}
#endif

	[unroll]
	for( uint level = 1; level <= NUM_LODS; level++ )
	{
		if( level == g_MipMapGen.numLODs + 1 )
		{
			break;
		}

		uint outGroupSize = uint( GROUP_SIZE ) >> level;
		uint inGroupSize = outGroupSize << 1;

		if( all( threadIdx.xy < inGroupSize ) )
		{
			s_ReductionData[threadIdx.y][threadIdx.x] = value;
		}

		GroupMemoryBarrierWithGroupSync();

		if( all( threadIdx.xy < outGroupSize ) )
		{
			VALUE_TYPE a = s_ReductionData[threadIdx.y * 2 + 0][threadIdx.x * 2 + 0];
			VALUE_TYPE b = s_ReductionData[threadIdx.y * 2 + 0][threadIdx.x * 2 + 1];
			VALUE_TYPE c = s_ReductionData[threadIdx.y * 2 + 1][threadIdx.x * 2 + 0];
			VALUE_TYPE d = s_ReductionData[threadIdx.y * 2 + 1][threadIdx.x * 2 + 1];

#if MODE == MODE_COLOR || MODE == MODE_MINMAX
			value = reduce( a, b, c, d );
#else
			value = reduce( float4( a, b, c, d ) );
#endif
			u_output[level - 1][groupIdx.xy * outGroupSize + threadIdx.xy] = value;
		}

		GroupMemoryBarrierWithGroupSync();
	}
}