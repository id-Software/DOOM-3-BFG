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
Buffer<uint> t_Histogram : register(t0);
RWBuffer<uint> u_Exposure : register(u0);

cbuffer c_ToneMapping :
register( b0 )
{
	ToneMappingConstants g_ToneMapping;
};
// *INDENT-ON*

#define FIXED_POINT_FRAC_BITS 6
#define FIXED_POINT_FRAC_MULTIPLIER (1 << FIXED_POINT_FRAC_BITS)

[numthreads( 1, 1, 1 )]
void main()
{
	float cdf = 0;
	uint i;

	[loop]
	for( i = 0; i < HISTOGRAM_BINS; ++i )
	{
		cdf += float( t_Histogram[i] ) / FIXED_POINT_FRAC_MULTIPLIER;
	}

	float lowCdf = cdf * g_ToneMapping.histogramLowPercentile;
	float highCdf = cdf * g_ToneMapping.histogramHighPercentile;

	float weightSum = 0;
	float binSum = 0;
	cdf = 0;

	[loop]
	for( i = 0; i < HISTOGRAM_BINS; ++i )
	{
		float binValue = float( t_Histogram[i] ) / FIXED_POINT_FRAC_MULTIPLIER;

		if( lowCdf <= cdf + binValue && cdf <= highCdf )
		{
			float histogramBinLuminance = exp2( ( i / ( float )HISTOGRAM_BINS ) * g_ToneMapping.logLuminanceScale
												+ g_ToneMapping.logLuminanceBias );

			weightSum += histogramBinLuminance * binValue;
			binSum += binValue;
		}

		cdf += binValue;
	}

	float targetExposure = ( binSum > 0 ) ? ( weightSum / binSum ) : 0;

	targetExposure = clamp(
						 targetExposure,
						 g_ToneMapping.minAdaptedLuminance,
						 g_ToneMapping.maxAdaptedLuminance );

	float oldExposure = asfloat( u_Exposure[0] );
	float diff = oldExposure - targetExposure;

	float adaptationSpeed = ( diff < 0 )
							? g_ToneMapping.eyeAdaptationSpeedUp
							: g_ToneMapping.eyeAdaptationSpeedDown;

	if( adaptationSpeed > 0 )
	{
		targetExposure += diff * exp2( -g_ToneMapping.frameTime * adaptationSpeed );
	}

	u_Exposure[0] = asuint( targetExposure );
}
