/**
 \file AmbientOcclusion_minify.pix
 \author Morgan McGuire and Michael Mara, NVIDIA Research

  Open Source under the "BSD" license: http://www.opensource.org/licenses/bsd-license.php

  Copyright (c) 2011-2012, NVIDIA
  Copyright (c) 2016 Robert Beckebans ( id Tech 4.x integration )
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

  Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
  Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 */
#include "global_inc.hlsl"


// *INDENT-OFF*
Texture2D t_ViewDepth : register( t0 VK_DESCRIPTOR_SET( 1 ) );

SamplerState LinearSampler : register( s0 VK_DESCRIPTOR_SET( 2 ) );

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

//#extension GL_EXT_gpu_shader4 : enable

//#expect USE_PEELED_DEPTH_BUFFER "binary"

#if 0 //( USE_PEELED_DEPTH_BUFFER != 0 )
	#define mask rg
#else
	#define mask r
#endif

float reconstructCSZ( float d )
{
	//return clipInfo[0] / (clipInfo[1] * d + clipInfo[2]);

	// infinite far perspective matrix
	return -3.0 / ( -1.0 * d + 1.0 );

	//d = d * 2.0 - 1.0;
	//return -rpProjectionMatrixZ.w / ( -rpProjectionMatrixZ.z - d );
}

void main( PS_IN fragment, out PS_OUT result )
{
#if defined(BRIGHTPASS)
	float2 ssC = fragment.texcoord0;
	float depth = t_ViewDepth.Sample( LinearSampler, ssC ).r;
	//depth = reconstructCSZ( depth );
	result.color.mask = depth;
#else
	//int2 ssP = int2( gl_FragCoord.xy );
	int2 ssP = int2( fragment.texcoord0 * rpScreenCorrectionFactor.zw );

	int previousMIPNumber = int( rpJitterTexScale.x );

	// Rotated grid subsampling to avoid XY directional bias or Z precision bias while downsampling.
	// On DX9, the bit-and can be implemented with floating-point modulo
	//result.color.mask = texture( samp0, clamp( ssP * 2 + int2( ssP.y & 1, ssP.x & 1 ), int2( 0 ), textureSize( samp0, previousMIPNumber ) - int2( 1 ) ) * rpScreenCorrectionFactor.xy, previousMIPNumber ).mask;
	int2 ssc = clamp( ssP * 2 + int2( ssP.y & 1, ssP.x & 1 ), int2( 0 ), textureSize( t_ViewDepth, previousMIPNumber ) - int2( 1 ) );
	result.color.mask = texelFetch( t_ViewDepth, ssc, previousMIPNumber ).mask;
	//result.color.mask = texelFetch2D( samp0, int3( ssP * 2 + int2( ( ssP.y & 1 ) ^ 1, ( ssP.x & 1 ) ^ 1 ), 0 ) );

	// result.color.mask = texelFetch( samp0, ssP, 0 ).r;

	//float2 ssC = float2( ssP * 2 + int2( ( ssP.y & 1 ) ^ 1, ( ssP.x & 1 ) ^ 1 ) ) * rpScreenCorrectionFactor.xy;
	//float2 ssC = float2( ssP ) * rpScreenCorrectionFactor.xy;
	//float2 ssC = fragment.texcoord0;
	//float depth = tex2D( samp0, ssC ).r;
	result.color.mask = 0.5;
#endif
}
