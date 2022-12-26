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

#include "vulkan.hlsli"

struct ToneMappingConstants
{
	uint2 viewOrigin;
	uint2 viewSize;

	float logLuminanceScale;
	float logLuminanceBias;
	float histogramLowPercentile;
	float histogramHighPercentile;

	float eyeAdaptationSpeedUp;
	float eyeAdaptationSpeedDown;
	float minAdaptedLuminance;
	float maxAdaptedLuminance;

	float frameTime;
	float exposureScale;
	float whitePointInvSquared;
	uint sourceSlice;

	float2 colorLUTTextureSize;
	float2 colorLUTTextureSizeInv;
};

// *INDENT-OFF*
#if SOURCE_ARRAY
Texture2DArray t_Source : register(t0);
#else
Texture2D t_Source : register(t0);
#endif
Buffer<uint> t_Exposure : register(t1);

Texture2D t_ColorLUT : register(t2);
SamplerState s_ColorLUTSampler : register(s0);

cbuffer c_ToneMapping : register(b0)
{
    ToneMappingConstants g_ToneMapping;
};
// *INDENT-ON*

float3 ACESFilm( float3 x )
{
	float a = 2.51;
	float b = 0.03;
	float c = 2.43;
	float d = 0.59;
	float e = 0.14;
	return saturate( ( x * ( a * x + b ) ) / ( x * ( c * x + d ) + e ) );
}

float Luminance( float3 color )
{
	return 0.2126 * color.r + 0.7152 * color.g + 0.0722 * color.b;
}

float3 ConvertToLDR( float3 color )
{
	float srcLuminance = Luminance( color );

	if( srcLuminance <= 0 )
	{
		return 0;
	}

	float adaptedLuminance = asfloat( t_Exposure[0] );
	if( adaptedLuminance <= 0 )
	{
		adaptedLuminance = g_ToneMapping.minAdaptedLuminance;
	}

	float scaledLuminance = g_ToneMapping.exposureScale * srcLuminance / adaptedLuminance;
	float mappedLuminance = ( scaledLuminance * ( 1 + scaledLuminance * g_ToneMapping.whitePointInvSquared ) ) / ( 1 + scaledLuminance );

	return color * ( mappedLuminance / srcLuminance );
}

float3 ApplyColorLUT( float3 color )
{
	const float size = g_ToneMapping.colorLUTTextureSize.y;

	color = saturate( color );

	float r = color.r * ( size - 1 ) + 0.5;
	float g = color.g * ( size - 1 ) + 0.5;

	float b = color.b * ( size - 1 );

	float2 uv1 = float2( ( floor( b ) + 0 ) * size + r, g ) * g_ToneMapping.colorLUTTextureSizeInv.xy;
	float2 uv2 = float2( ( floor( b ) + 1 ) * size + r, g ) * g_ToneMapping.colorLUTTextureSizeInv.xy;

	float3 c1 = t_ColorLUT.SampleLevel( s_ColorLUTSampler, uv1, 0 ).rgb;
	float3 c2 = t_ColorLUT.SampleLevel( s_ColorLUTSampler, uv2, 0 ).rgb;

	return lerp( c1, c2, frac( b ) );
}


void main(
	in float4 pos : SV_Position,
	in float2 uv : UV,
	out float4 o_rgba : SV_Target )
{
#if SOURCE_ARRAY
	float4 HdrColor = t_Source[uint3( pos.xy, g_ToneMapping.sourceSlice )];
#else
	float4 HdrColor = t_Source[pos.xy];
#endif
	o_rgba.rgb = ConvertToLDR( HdrColor.rgb );
	o_rgba.a = HdrColor.a;

	if( g_ToneMapping.colorLUTTextureSize.x > 0 )
	{
		o_rgba.rgb = ApplyColorLUT( o_rgba.rgb );
	}
	else
	{
		// Tonemapping curve is applied after exposure.
		o_rgba.rgb = ACESFilm( o_rgba.rgb );
	}

	// Gamma correction since we are not rendering to an sRGB render target.
	const float hdrGamma = 2.2;
	float gamma = 1.0 / hdrGamma;
	o_rgba.r = pow( o_rgba.r, gamma );
	o_rgba.g = pow( o_rgba.g, gamma );
	o_rgba.b = pow( o_rgba.b, gamma );
}
