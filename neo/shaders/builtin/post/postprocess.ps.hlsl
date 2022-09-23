/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2015-2022 Robert Beckebans
Copyright (C) 2014 Timothy Lottes (AMD)
Copyright (C) 2019 Justin Marshall (IcedTech)

This file is part of the Doom 3 BFG Edition GPL Source Code ("Doom 3 BFG Edition Source Code").

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "global_inc.hlsl"


// *INDENT-OFF*
Texture2D t_BaseColor	: register( t0 VK_DESCRIPTOR_SET( 0 ) );
Texture2D t_BlueNoise	: register( t1 VK_DESCRIPTOR_SET( 0 ) );

SamplerState samp0		: register(s0 VK_DESCRIPTOR_SET( 1 ) );
SamplerState samp1		: register(s1 VK_DESCRIPTOR_SET( 1 ) ); // blue noise 256

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

#define USE_CHROMATIC_ABERRATION			1
#define Chromatic_Amount					0.075

#define USE_TECHNICOLOR						0		// [0 or 1]

#define Technicolor_Amount					1.0		// [0.00 to 1.00]
#define Technicolor_Power					4.0		// [0.00 to 8.00]
#define Technicolor_RedNegativeAmount		0.88	// [0.00 to 1.00]
#define Technicolor_GreenNegativeAmount		0.88	// [0.00 to 1.00]
#define Technicolor_BlueNegativeAmount		0.88	// [0.00 to 1.00]

#define USE_VIBRANCE						0
#define Vibrance							0.5	// [-1.00 to 1.00]
#define	Vibrance_RGB_Balance				float3( 1.0, 1.0, 1.0 )

#define USE_CAS                             0

#define USE_DITHERING 						1
#define Dithering_QuantizationSteps         16.0 // 8.0 = 2 ^ 3 quantization bits
#define Dithering_NoiseBoost                1.0
#define Dithering_Wide                      1.0
#define DITHER_IN_LINEAR_SPACE              0
#define DITHER_GENERATE_NOISE               0

float3 overlay( float3 a, float3 b )
{
	//return pow( abs( b ), 2.2 ) < 0.5 ? ( 2.0 * a * b ) : float3( 1.0, 1.0, 1.0 ) - float3( 2.0, 2.0, 2.0 ) * ( float3( 1.0, 1.0, 1.0 ) - a ) * ( 1.0 - b );
	//return abs( b ) < 0.5 ? ( 2.0 * a * b ) : float3( 1.0, 1.0, 1.0 ) - float3( 2.0, 2.0, 2.0 ) * ( float3( 1.0, 1.0, 1.0 ) - a ) * ( 1.0 - b );

	return float3(
			   b.x < 0.5 ? ( 2.0 * a.x * b.x ) : ( 1.0 - 2.0 * ( 1.0 - a.x ) * ( 1.0 - b.x ) ),
			   b.y < 0.5 ? ( 2.0 * a.y * b.y ) : ( 1.0 - 2.0 * ( 1.0 - a.y ) * ( 1.0 - b.y ) ),
			   b.z < 0.5 ? ( 2.0 * a.z * b.z ) : ( 1.0 - 2.0 * ( 1.0 - a.z ) * ( 1.0 - b.z ) ) );
}


void TechnicolorPass( inout float4 color )
{
	const float3 cyanFilter = float3( 0.0, 1.30, 1.0 );
	const float3 magentaFilter = float3( 1.0, 0.0, 1.05 );
	const float3 yellowFilter = float3( 1.6, 1.6, 0.05 );
	const float3 redOrangeFilter = float3( 1.05, 0.62, 0.0 );
	const float3 greenFilter = float3( 0.3, 1.0, 0.0 );

	float2 redNegativeMul   = color.rg * ( 1.0 / ( Technicolor_RedNegativeAmount * Technicolor_Power ) );
	float2 greenNegativeMul = color.rg * ( 1.0 / ( Technicolor_GreenNegativeAmount * Technicolor_Power ) );
	float2 blueNegativeMul  = color.rb * ( 1.0 / ( Technicolor_BlueNegativeAmount * Technicolor_Power ) );

	float redNegative   = dot( redOrangeFilter.rg, redNegativeMul );
	float greenNegative = dot( greenFilter.rg, greenNegativeMul );
	float blueNegative  = dot( magentaFilter.rb, blueNegativeMul );

	float3 redOutput   = _float3( redNegative ) + cyanFilter;
	float3 greenOutput = _float3( greenNegative ) + magentaFilter;
	float3 blueOutput  = _float3( blueNegative ) + yellowFilter;

	float3 result = redOutput * greenOutput * blueOutput;
	color.rgb = lerp( color.rgb, result, Technicolor_Amount );
}


void VibrancePass( inout float4 color )
{
	const float3 vibrance = Vibrance_RGB_Balance * Vibrance;

	float Y = dot( LUMINANCE_SRGB, color );

	float minColor = min( color.r, min( color.g, color.b ) );
	float maxColor = max( color.r, max( color.g, color.b ) );

	float colorSat = maxColor - minColor;

	color.rgb = lerp( _float3( Y ), color.rgb, ( 1.0 + ( vibrance * ( 1.0 - ( sign( vibrance ) * colorSat ) ) ) ) );
}


// CHROMATIC ABBERATION

float2 BarrelDistortion( float2 xy, float amount )
{
	float2 cc = xy - 0.5;
	float dist = dot2( cc, cc );

	return xy + cc * dist * amount;
}

float Linterp( float t )
{
	return saturate( 1.0 - abs( 2.0 * t - 1.0 ) );
}

float Remap( float t, float a, float b )
{
	return saturate( ( t - a ) / ( b - a ) );
}

float3 SpectrumOffset( float t )
{
	float lo = step( t, 0.5 );
	float hi = 1.0 - lo;
	float w = Linterp( Remap( t, 1.0 / 6.0, 5.0 / 6.0 ) );
	float3 ret = float3( lo, 1.0, hi ) * float3( 1.0 - w, w, 1.0 - w );

	return pow( ret, _float3( 1.0 / 2.2 ) );
}

void ChromaticAberrationPass( inout float4 color, PS_IN fragment )
{
	float amount = Chromatic_Amount; //color.a * 1.0; //rpUser0.x;

	float3 sum = _float3( 0.0 );
	float3 sumColor = _float3( 0.0 );

	//float samples = rpOverbright.x;
	float samples = 12.0; // * 2;

	for( float i = 0.0; i < samples; i = i + 1.0 )
	{
		//float t = ( ( i / ( samples - 1.0 ) ) - 0.5 );
		float t = ( i / ( samples - 1.0 ) );
		//float t = log( i / ( samples - 1.0 ) );

		float3 so = SpectrumOffset( t );

		sum += so.xyz;
		sumColor += so * t_BaseColor.Sample( samp0, BarrelDistortion( fragment.texcoord0, ( 0.5 * amount * t ) ) ).rgb;
	}

	float3 outColor = ( sumColor / sum );
	color.rgb = lerp( color.rgb, outColor, Technicolor_Amount );
}

void ChromaticAberrationPass2( inout float4 color, PS_IN fragment )
{
	float amount = 6.0;

	float2 uv = fragment.texcoord0;

	//float2 texel = 1.0 / rpWindowCoord.zw;
	float2 texel = 1.0 / float2( 1920.0, 1080.0 );

	float2 coords = ( uv - 0.5 ) * 2.0;
	float coordDot = dot( coords, coords );

	float2 precompute = amount * coordDot * coords;
	float2 uvR = uv - texel.xy * precompute;
	float2 uvB = uv + texel.xy * precompute;

	float3 outColor;
	outColor.r = t_BaseColor.Sample( samp0, uvR ).r;
	outColor.g = t_BaseColor.Sample( samp0, uv ).g;
	outColor.b = t_BaseColor.Sample( samp0, uvB ).b;

	color.rgb = lerp( color.rgb, outColor, Technicolor_Amount );
}






// https://gpuopen.com/vdr-follow-up-fine-art-of-film-grain/

//
// TEMPORAL DITHERING TEST IN LINEAR
//

//
// This is biased (saturates + adds contrast) because dithering done in non-linear space.
//

// Shows proper dithering of a signal (overlapping of dither between bands).
// Results in about a 1-stop improvement in dynamic range over conventional dither
//   which does not overlap dither across bands
//   (try "#define WIDE 0.5" to see the difference below).
//
// This would work a lot better with a proper random number generator (flicker etc is bad).
// Sorry there is a limit to what can be done easily in shadertoy.
//
// Proper dithering algorithm,
//
//   color = floor(color * steps + noise) * (1.0/(steps-1.0))
//
// Where,
//
//   color ... output color {0 to 1}
//   noise ... random number between {-1 to 1}
//   steps ... quantization steps, ie 8-bit = 256
//
// The noise in this case is shaped by a high pass filter.
// This is to produce a better quality temporal dither.

// Scale the width of the dither

//-----------------------------------------------------------------------





float BlueNoise( float2 n, float x )
{
	float noise = t_BlueNoise.Sample( samp1, ( n.xy * rpJitterTexOffset.xy ) * 1.0 ).r;

	noise = frac( noise + 0.61803398875 * rpJitterTexOffset.z * x );

	//noise = InterleavedGradientNoise( n );

	noise = RemapNoiseTriErp( noise );

	noise = noise * 2.0 - 1.0;

	return noise;
}

float3 BlueNoise3( float2 n, float x )
{
	float2 uv = n.xy * rpJitterTexOffset.xy;

	float3 noise = t_BlueNoise.Sample( samp1, uv ).rgb;

	noise = frac( noise + c_goldenRatioConjugate * rpJitterTexOffset.w * x );

	//noise.x = RemapNoiseTriErp( noise.x );
	//noise.y = RemapNoiseTriErp( noise.y );
	//noise.z = RemapNoiseTriErp( noise.z );

	//noise = noise * 2.0 - 1.0;

	return noise;
}


float Noise( float2 n, float x )
{
	n += x;

#if 1
	return frac( sin( dot( n.xy, float2( 12.9898, 78.233 ) ) ) * 43758.5453 ) * 2.0 - 1.0;
#else
	//return BlueNoise( n, 55.0 );
	return BlueNoise( n, 1.0 );

	//return InterleavedGradientNoise( n ) * 2.0 - 1.0;
#endif
}

// Step 1 in generation of the dither source texture.
float Step1( float2 uv, float n )
{
	float a = 1.0, b = 2.0, c = -12.0, t = 1.0;

	return ( 1.0 / ( a * 4.0 + b * 4.0 - c ) ) * (
			   Noise( uv + float2( -1.0, -1.0 ) * t, n ) * a +
			   Noise( uv + float2( 0.0, -1.0 ) * t, n ) * b +
			   Noise( uv + float2( 1.0, -1.0 ) * t, n ) * a +
			   Noise( uv + float2( -1.0, 0.0 ) * t, n ) * b +
			   Noise( uv + float2( 0.0, 0.0 ) * t, n ) * c +
			   Noise( uv + float2( 1.0, 0.0 ) * t, n ) * b +
			   Noise( uv + float2( -1.0, 1.0 ) * t, n ) * a +
			   Noise( uv + float2( 0.0, 1.0 ) * t, n ) * b +
			   Noise( uv + float2( 1.0, 1.0 ) * t, n ) * a +
			   0.0 );
}

// Step 2 in generation of the dither source texture.
float Step2( float2 uv, float n )
{
	float a = 1.0, b = 2.0, c = -2.0, t = 1.0;

#if DITHER_IN_LINEAR_SPACE
	return ( 1.0 / ( a * 4.0 + b * 4.0 - c ) ) * (
#else
	return ( 4.0 / ( a * 4.0 + b * 4.0 - c ) ) * (
#endif
			   Step1( uv + float2( -1.0, -1.0 ) * t, n ) * a +
			   Step1( uv + float2( 0.0, -1.0 ) * t, n ) * b +
			   Step1( uv + float2( 1.0, -1.0 ) * t, n ) * a +
			   Step1( uv + float2( -1.0, 0.0 ) * t, n ) * b +
			   Step1( uv + float2( 0.0, 0.0 ) * t, n ) * c +
			   Step1( uv + float2( 1.0, 0.0 ) * t, n ) * b +
			   Step1( uv + float2( -1.0, 1.0 ) * t, n ) * a +
			   Step1( uv + float2( 0.0, 1.0 ) * t, n ) * b +
			   Step1( uv + float2( 1.0, 1.0 ) * t, n ) * a +
			   0.0 );
}

// Used for stills.
float3 Step3( float2 uv )
{
#if DITHER_GENERATE_NOISE
	float a = Step2( uv, 0.07 );
	float b = Step2( uv, 0.11 );
	float c = Step2( uv, 0.13 );

	return float3( a, b, c );
#else
	float3 noise = BlueNoise3( uv, 0.0 );

#if 1
	noise.x = RemapNoiseTriErp( noise.x );
	noise.y = RemapNoiseTriErp( noise.y );
	noise.z = RemapNoiseTriErp( noise.z );

	noise = noise * 2.0 - 1.0;
#endif

	return noise;
#endif
}

// Used for temporal dither.
float3 Step3T( float2 uv )
{
#if DITHER_GENERATE_NOISE
	float a = Step2( uv, 0.07 * fract( rpJitterTexOffset.z ) );
	float b = Step2( uv, 0.11 * fract( rpJitterTexOffset.z ) );
	float c = Step2( uv, 0.13 * fract( rpJitterTexOffset.z ) );

	return float3( a, b, c );
#else
	float3 noise = BlueNoise3( uv, 1.0 );

#if 1
	noise.x = RemapNoiseTriErp( noise.x );
	noise.y = RemapNoiseTriErp( noise.y );
	noise.z = RemapNoiseTriErp( noise.z );

	noise = noise * 2.0 - 1.0;
#endif

	return noise;
#endif
}


void DitheringPass( inout float4 fragColor, PS_IN fragment )
{
	float2 uv = fragment.position.xy * 1.0;
	float2 uv2 = fragment.texcoord0;

	float3 color = fragColor.rgb;

#if 0
// BOTTOM: Show bands.
	if( uv2.y >= 0.975 )
	{
		// quantized signal
		color = float3( uv2.x );

		// dithered still
		//color = floor( color * Dithering_QuantizationSteps + Step3( uv ) * Dithering_NoiseBoost ) * ( 1.0 / ( Dithering_QuantizationSteps - 1.0 ) );
	}
	else if( uv2.y >= 0.95 )
	{
		// quantized signal
		color = float3( uv2.x );
		color = floor( color * Dithering_QuantizationSteps ) * ( 1.0 / ( Dithering_QuantizationSteps - 1.0 ) );
	}
	else if( uv2.y >= 0.925 )
	{
		// quantized signal dithered temporally
		color = float3( uv2.x );
		color = floor( color * Dithering_QuantizationSteps + Step3( uv ) * Dithering_NoiseBoost ) * ( 1.0 / ( Dithering_QuantizationSteps - 1.0 ) );
	}
	// TOP: Show dither texture.
	else if( uv2.y >= 0.9 )
	{
		color = Step3( uv ) * ( 0.25 * Dithering_NoiseBoost ) + 0.5;
	}
	else
#endif
	{

#if DITHER_IN_LINEAR_SPACE

		color = Linear3( color );

		// Add grain in linear space.
#if 0
		// Slow more correct solutions.
#if 1
		// Too expensive.
		// Helps understand the fast solutions.
		float3 amount = Linear3( Srgb3( color ) + ( Dithering_NoiseBoost / Dithering_QuantizationSteps ) ) - color;
#else
		// Less too expensive.
		float luma = PhotoLuma( color );

		// Implement this as a texture lookup table.
		float amount = Linear1( Srgb1( luma ) + ( Dithering_NoiseBoost / Dithering_QuantizationSteps ) ) - luma;
#endif

#else
		// Fast solutions.
#if 1
		// Hack 1 (fastest).
		// For HDR need saturate() around luma.
		float luma = PhotoLuma( color );
		float amount = mix(
						   Linear1( Dithering_NoiseBoost / Dithering_QuantizationSteps ),
						   Linear1( ( Dithering_NoiseBoost / Dithering_QuantizationSteps ) + 1.0 ) - 1.0,
						   luma );
#else
		// Hack 2 (slower?).
		// For HDR need saturate() around color in mix().
		float3 amount = mix(
							float3( Linear1( Dithering_NoiseBoost / Dithering_QuantizationSteps ) ),
							float3( Linear1( ( Dithering_NoiseBoost / Dithering_QuantizationSteps ) + 1.0 ) - 1.0 ),
							color );
#endif

#endif
		color += Step3T( uv ) * amount;// * Dithering_NoiseBoost;

		// The following represents hardware linear->sRGB xform
		// which happens on sRGB formatted render targets,
		// except using a lot less bits/pixel.
		color = max( float3( 0.0 ), color );
		color = Srgb3( color );
		color = floor( color * Dithering_QuantizationSteps ) * ( 1.0 / ( Dithering_QuantizationSteps - 1.0 ) );

#else

#if 0
		if( uv2.x <= 0.5 )
		{
			// quantized but not dithered
			color = floor( 0.5 + color * ( Dithering_QuantizationSteps + Dithering_Wide - 1.0 ) + ( -Dithering_Wide * 0.5 ) ) * ( 1.0 / ( Dithering_QuantizationSteps - 1.0 ) );
		}
		else
#endif
		{
			color = floor( 0.5 + color * ( Dithering_QuantizationSteps + Dithering_Wide - 1.0 ) + ( -Dithering_Wide * 0.5 ) + Step3T( uv ) * ( Dithering_Wide ) ) * ( 1.0 / ( Dithering_QuantizationSteps - 1.0 ) );
		}
#endif

	}

	fragColor.rgb = color;
}


float Min3( float x, float y, float z )
{
	return min( x, min( y, z ) );
}

float Max3( float x, float y, float z )
{
	return max( x, max( y, z ) );
}


float rcp( float v )
{
	return 1.0 / v;
}

void ContrastAdaptiveSharpeningPass( inout float4 color, PS_IN fragment )
{
	float2 texcoord = fragment.texcoord0;
	float Sharpness = 1;

	// fetch a 3x3 neighborhood around the pixel 'e',
	//  a b c
	//  d(e)f
	//  g h i
	int imgWidth = 1;
	int imgHeight = 1;
	t_BaseColor.GetDimensions( imgWidth, imgHeight );
	int2 bufferSize = int2( imgWidth, imgHeight );

	float pixelX = ( 1.0 / bufferSize.x );
	float pixelY = ( 1.0 / bufferSize.y );

	float3 a = t_BaseColor.Sample( samp0, texcoord + float2( -pixelX, -pixelY ) ).rgb;
	float3 b = t_BaseColor.Sample( samp0, texcoord + float2( 0.0, -pixelY ) ).rgb;
	float3 c = t_BaseColor.Sample( samp0, texcoord + float2( pixelX, -pixelY ) ).rgb;
	float3 d = t_BaseColor.Sample( samp0, texcoord + float2( -pixelX, 0.0 ) ).rgb;
	float3 e = t_BaseColor.Sample( samp0, texcoord ).rgb;
	float3 f = t_BaseColor.Sample( samp0, texcoord + float2( pixelX, 0.0 ) ).rgb;
	float3 g = t_BaseColor.Sample( samp0, texcoord + float2( -pixelX, pixelY ) ).rgb;
	float3 h = t_BaseColor.Sample( samp0, texcoord + float2( 0.0, pixelY ) ).rgb;
	float3 i = t_BaseColor.Sample( samp0, texcoord + float2( pixelX, pixelY ) ).rgb;

	// Soft min and max.
	//  a b c             b
	//  d e f * 0.5  +  d e f * 0.5
	//  g h i             h
	// These are 2.0x bigger (factored out the extra multiply).
	float mnR = Min3( Min3( d.r, e.r, f.r ), b.r, h.r );
	float mnG = Min3( Min3( d.g, e.g, f.g ), b.g, h.g );
	float mnB = Min3( Min3( d.b, e.b, f.b ), b.b, h.b );

	float mnR2 = Min3( Min3( mnR, a.r, c.r ), g.r, i.r );
	float mnG2 = Min3( Min3( mnG, a.g, c.g ), g.g, i.g );
	float mnB2 = Min3( Min3( mnB, a.b, c.b ), g.b, i.b );
	mnR = mnR + mnR2;
	mnG = mnG + mnG2;
	mnB = mnB + mnB2;

	float mxR = Max3( Max3( d.r, e.r, f.r ), b.r, h.r );
	float mxG = Max3( Max3( d.g, e.g, f.g ), b.g, h.g );
	float mxB = Max3( Max3( d.b, e.b, f.b ), b.b, h.b );

	float mxR2 = Max3( Max3( mxR, a.r, c.r ), g.r, i.r );
	float mxG2 = Max3( Max3( mxG, a.g, c.g ), g.g, i.g );
	float mxB2 = Max3( Max3( mxB, a.b, c.b ), g.b, i.b );
	mxR = mxR + mxR2;
	mxG = mxG + mxG2;
	mxB = mxB + mxB2;

	// Smooth minimum distance to signal limit divided by smooth max.
	float rcpMR = rcp( mxR );
	float rcpMG = rcp( mxG );
	float rcpMB = rcp( mxB );

	float ampR = saturate( min( mnR, 2.0 - mxR ) * rcpMR );
	float ampG = saturate( min( mnG, 2.0 - mxG ) * rcpMG );
	float ampB = saturate( min( mnB, 2.0 - mxB ) * rcpMB );

	// Shaping amount of sharpening.
	ampR = sqrt( ampR );
	ampG = sqrt( ampG );
	ampB = sqrt( ampB );

	// Filter shape.
	//  0 w 0
	//  w 1 w
	//  0 w 0
	float peak = -rcp( lerp( 8.0, 5.0, saturate( Sharpness ) ) );

	float wR = ampR * peak;
	float wG = ampG * peak;
	float wB = ampB * peak;

	float rcpWeightR = rcp( 1.0 + 4.0 * wR );
	float rcpWeightG = rcp( 1.0 + 4.0 * wG );
	float rcpWeightB = rcp( 1.0 + 4.0 * wB );

	float3 outColor = float3( saturate( ( b.r * wR + d.r * wR + f.r * wR + h.r * wR + e.r ) * rcpWeightR ),
							  saturate( ( b.g * wG + d.g * wG + f.g * wG + h.g * wG + e.g ) * rcpWeightG ),
							  saturate( ( b.b * wB + d.b * wB + f.b * wB + h.b * wB + e.b ) * rcpWeightB ) );

	color.rgb = outColor;
	//color.rgb = lerp( color.rgb, outColor, Technicolor_Amount );
}

void main( PS_IN fragment, out PS_OUT result )
{
	float2 tCoords = fragment.texcoord0;

	// base color with tone mapping and other post processing applied
	float4 color = t_BaseColor.Sample( samp0, tCoords );

#if USE_CAS
	ContrastAdaptiveSharpeningPass( color, fragment );
#endif

#if USE_CHROMATIC_ABERRATION
	ChromaticAberrationPass( color, fragment );
#endif

#if USE_TECHNICOLOR
	TechnicolorPass( color );
#endif

#if USE_VIBRANCE
	VibrancePass( color );
#endif

#if USE_DITHERING
	DitheringPass( color, fragment );
#endif

	result.color = color;
}
