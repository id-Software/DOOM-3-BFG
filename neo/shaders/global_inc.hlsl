/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2020 Robert Beckebans

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

#include "vulkan.hlsli"

// *INDENT-OFF*
cbuffer globals : register( b0 VK_DESCRIPTOR_SET( 0 ) )
{
	float4 rpScreenCorrectionFactor;
	float4 rpWindowCoord;
	float4 rpDiffuseModifier;
	float4 rpSpecularModifier;

	float4 rpLocalLightOrigin;
	float4 rpLocalViewOrigin;

	float4 rpLightProjectionS;
	float4 rpLightProjectionT;
	float4 rpLightProjectionQ;
	float4 rpLightFalloffS;

	float4 rpBumpMatrixS;
	float4 rpBumpMatrixT;

	float4 rpDiffuseMatrixS;
	float4 rpDiffuseMatrixT;

	float4 rpSpecularMatrixS;
	float4 rpSpecularMatrixT;

	float4 rpVertexColorModulate;
	float4 rpVertexColorAdd;

	float4 rpColor;
	float4 rpViewOrigin;
	float4 rpGlobalEyePos;

	float4 rpMVPmatrixX;
	float4 rpMVPmatrixY;
	float4 rpMVPmatrixZ;
	float4 rpMVPmatrixW;

	float4 rpModelMatrixX;
	float4 rpModelMatrixY;
	float4 rpModelMatrixZ;
	float4 rpModelMatrixW;

	float4 rpProjectionMatrixX;
	float4 rpProjectionMatrixY;
	float4 rpProjectionMatrixZ;
	float4 rpProjectionMatrixW;

	float4 rpModelViewMatrixX;
	float4 rpModelViewMatrixY;
	float4 rpModelViewMatrixZ;
	float4 rpModelViewMatrixW;

	float4 rpTextureMatrixS;
	float4 rpTextureMatrixT;

	float4 rpTexGen0S;
	float4 rpTexGen0T;
	float4 rpTexGen0Q;
	float4 rpTexGen0Enabled;

	float4 rpTexGen1S;
	float4 rpTexGen1T;
	float4 rpTexGen1Q;
	float4 rpTexGen1Enabled;

	float4 rpWobbleSkyX;
	float4 rpWobbleSkyY;
	float4 rpWobbleSkyZ;

	float4 rpOverbright;
	float4 rpEnableSkinning;
	float4 rpAlphaTest;

	// RB begin
	float4 rpAmbientColor;
	float4 rpGlobalLightOrigin;
	float4 rpJitterTexScale;
	float4 rpJitterTexOffset;
	float4 rpCascadeDistances;

	float4 rpShadowMatrices[6 * 4];
	float4 rpShadowAtlasOffsets[6];
	// RB end

	float4 rpUser0;
	float4 rpUser1;
	float4 rpUser2;
	float4 rpUser3;
	float4 rpUser4;
	float4 rpUser5;
	float4 rpUser6;
	float4 rpUser7;
};

// *INDENT-ON*

static float dot2( float2 a, float2 b )
{
	return dot( a, b );
}
static float dot3( float3 a, float3 b )
{
	return dot( a, b );
}
static float dot3( float3 a, float4 b )
{
	return dot( a, b.xyz );
}
static float dot3( float4 a, float3 b )
{
	return dot( a.xyz, b );
}
static float dot3( float4 a, float4 b )
{
	return dot( a.xyz, b.xyz );
}
static float dot4( float4 a, float4 b )
{
	return dot( a, b );
}
static float dot4( float2 a, float4 b )
{
	return dot( float4( a, 0, 1 ), b );
}

// RB begin
#ifndef PI
	#define PI	3.14159265358979323846
#endif

#define DEG2RAD( a )				( ( a ) * PI / 180.0f )
#define RAD2DEG( a )				( ( a ) * 180.0f / PI )

// RB: the golden ratio is useful to animate Blue noise
#define c_goldenRatioConjugate 0.61803398875

// ----------------------
// sRGB <-> Linear RGB Color Conversion
// ----------------------

float Linear1( float c )
{
	return ( c <= 0.04045 ) ? c / 12.92 : pow( ( c + 0.055 ) / 1.055, 2.4 );
}

float3 Linear3( float3 c )
{
	return float3( Linear1( c.r ), Linear1( c.g ), Linear1( c.b ) );
}

float Srgb1( float c )
{
	return ( c < 0.0031308 ? c * 12.92 : 1.055 * pow( c, 0.41666 ) - 0.055 );
}

float3 Srgb3( float3 c )
{
	return float3( Srgb1( c.r ), Srgb1( c.g ), Srgb1( c.b ) );
}

static const float3 photoLuma = float3( 0.2126, 0.7152, 0.0722 );
float PhotoLuma( float3 c )
{
	return dot( c, photoLuma );
}

// RB: Conditional sRGB -> linear conversion. It is the default for all 3D rendering
// and only shaders for 2D rendering of GUIs define USE_SRGB to work directly on the ldr render target
float3 sRGBToLinearRGB( float3 c )
{
#if !defined( USE_SRGB ) || !USE_SRGB
	c = clamp( c, 0.0, 1.0 );

	return Linear3( c );
#else
	return c;
#endif
}

float4 sRGBAToLinearRGBA( float4 c )
{
#if !defined( USE_SRGB ) || !USE_SRGB
	c = clamp( c, 0.0, 1.0 );

	return float4( Linear1( c.r ), Linear1( c.g ), Linear1( c.b ), Linear1( c.a ) );
#else
	return c;
#endif
}

float3 LinearRGBToSRGB( float3 c )
{
#if !defined( USE_SRGB ) || !USE_SRGB
	c = clamp( c, 0.0, 1.0 );

	return Srgb3( c );
#else
	return c;
#endif
}

float4 LinearRGBToSRGB( float4 c )
{
#if !defined( USE_SRGB ) || !USE_SRGB
	c = clamp( c, 0.0, 1.0 );

	return float4( Srgb1( c.r ), Srgb1( c.g ), Srgb1( c.b ), c.a );
#else
	return c;
#endif
}

/** Efficient GPU implementation of the octahedral unit vector encoding from

    Cigolle, Donow, Evangelakos, Mara, McGuire, Meyer,
    A Survey of Efficient Representations for Independent Unit Vectors, Journal of Computer Graphics Techniques (JCGT), vol. 3, no. 2, 1-30, 2014

    Available online http://jcgt.org/published/0003/02/01/
*/

float signNotZeroFloat( float k )
{
	return ( k >= 0.0 ) ? 1.0 : -1.0;
}


float2 signNotZero( float2 v )
{
	return float2( signNotZeroFloat( v.x ), signNotZeroFloat( v.y ) );
}

/** Assumes that v is a unit vector. The result is an octahedral vector on the [-1, +1] square. */
float2 octEncode( float3 v )
{
	float l1norm = abs( v.x ) + abs( v.y ) + abs( v.z );
	float2 oct = v.xy * ( 1.0 / l1norm );
	if( v.z < 0.0 )
	{
		oct = ( 1.0 - abs( oct.yx ) ) * signNotZero( oct.xy );
	}
	return oct;
}


/** Returns a unit vector. Argument o is an octahedral vector packed via octEncode,
    on the [-1, +1] square*/
float3 octDecode( float2 o )
{
	float3 v = float3( o.x, o.y, 1.0 - abs( o.x ) - abs( o.y ) );
	if( v.z < 0.0 )
	{
		v.xy = ( 1.0 - abs( v.yx ) ) * signNotZero( v.xy );
	}
	return normalize( v );
}

// RB end

// ----------------------
// YCoCg Color Conversion
// ----------------------
// Co
#define matrixRGB1toCoCg1YX half4( 0.50,  0.0, -0.50, 0.50196078 )
// Cg
#define matrixRGB1toCoCg1YY half4( -0.25,  0.5, -0.25, 0.50196078 )
// 1.0
#define matrixRGB1toCoCg1YZ half4( 0.0,   0.0,  0.0,  1.0 )
// Y
#define matrixRGB1toCoCg1YW half4( 0.25,  0.5,  0.25, 0.0 )

#define matrixCoCg1YtoRGB1X half4( 1.0, -1.0,  0.0,        1.0 )
// -0.5 * 256.0 / 255.0
#define matrixCoCg1YtoRGB1Y half4( 0.0,  1.0, -0.50196078, 1.0 )
// +1.0 * 256.0 / 255.0
#define matrixCoCg1YtoRGB1Z half4( -1.0, -1.0,  1.00392156, 1.0 )

static half3 ConvertYCoCgToRGB( half4 YCoCg )
{
	half3 rgbColor;

	YCoCg.z = ( YCoCg.z * 31.875 ) + 1.0;			//z = z * 255.0/8.0 + 1.0
	YCoCg.z = 1.0 / YCoCg.z;
	YCoCg.xy *= YCoCg.z;
	rgbColor.x = dot4( YCoCg, matrixCoCg1YtoRGB1X );
	rgbColor.y = dot4( YCoCg, matrixCoCg1YtoRGB1Y );
	rgbColor.z = dot4( YCoCg, matrixCoCg1YtoRGB1Z );
	return rgbColor;
}

static float2 CenterScale( float2 inTC, float2 centerScale )
{
	float scaleX = centerScale.x;
	float scaleY = centerScale.y;
	float4 tc0 = float4( scaleX, 0, 0, 0.5 - ( 0.5f * scaleX ) );
	float4 tc1 = float4( 0, scaleY, 0, 0.5 - ( 0.5f * scaleY ) );

	float2 finalTC;
	finalTC.x = dot4( inTC, tc0 );
	finalTC.y = dot4( inTC, tc1 );
	return finalTC;
}

static float2 Rotate2D( float2 inTC, float2 cs )
{
	float sinValue = cs.y;
	float cosValue = cs.x;

	float4 tc0 = float4( cosValue, -sinValue, 0, ( -0.5f * cosValue ) + ( 0.5f * sinValue ) + 0.5f );
	float4 tc1 = float4( sinValue, cosValue, 0, ( -0.5f * sinValue ) + ( -0.5f * cosValue ) + 0.5f );

	float2 finalTC;
	finalTC.x = dot4( inTC, tc0 );
	finalTC.y = dot4( inTC, tc1 );
	return finalTC;
}

// better noise function available at https://github.com/ashima/webgl-noise
float rand( float2 co )
{
	return frac( sin( dot( co.xy, float2( 12.9898, 78.233 ) ) ) * 43758.5453 );
}

#define square( x )		( x * x )

#define LUMINANCE_SRGB half4( 0.2125, 0.7154, 0.0721, 0.0 )
#define LUMINANCE_LINEAR half4( 0.299, 0.587, 0.144, 0.0 )

#define _half2( x )		half2( x, x )
#define _half3( x )		half3( x, x, x )
#define _half4( x )		half4( x, x, x, x )
#define _float2( x )	float2( x, x )
#define _float3( x )	float3( x, x, x )
#define _float4( x )	float4( x, x, x, x )
#define _int2( x )		int2( x, x )
#define vec2			float2
#define vec3			float3
#define vec4			float4

#define VPOS SV_Position

#define dFdx ddx
#define dFdy ddy

static float4 idtex2Dproj( SamplerState samp, Texture2D t, float4 texCoords )
{
	return t.Sample( samp, texCoords.xy / texCoords.w );
}

static float idtex2Dproj( SamplerState samp, Texture2D<float> t, float4 texCoords )
{
	return t.Sample( samp, texCoords.xy / texCoords.w );
}

static float3 idtex2Dproj( SamplerState samp, Texture2D<float3> t, float4 texCoords )
{
	return t.Sample( samp, texCoords.xy / texCoords.w );
}

static float4 swizzleColor( float4 c )
{
	return c;
	//return sRGBAToLinearRGBA( c );
}

static float4 texelFetch( Texture2D buffer, int2 ssc, int mipLevel )
{
	float4 c = buffer.Load( int3( ssc.xy, mipLevel ) );
	return c;
}

static float3 texelFetch( Texture2D<float3> buffer, int2 ssc, int mipLevel )
{
	float3 c = buffer.Load( int3( ssc.xy, mipLevel ) );
	return c;
}

static float1 texelFetch( Texture2D<float1> buffer, int2 ssc, int mipLevel )
{
	float1 c = buffer.Load( int3( ssc.xy, mipLevel ) );
	return c;
}

static float texelFetch( Texture2D<float> buffer, int2 ssc, int mipLevel )
{
	float1 c = buffer.Load( int3( ssc.xy, mipLevel ) );
	return c;
}

// returns width and height of the texture in texels
static int2 textureSize( Texture2D buffer, int mipLevel )
{
	int width = 1;
	int height = 1;
	int levels = 0;
	buffer.GetDimensions( mipLevel, width, height, levels );
	return int2( width, height );
}

static int2 textureSize( Texture2D<float1> buffer, int mipLevel )
{
	int width = 1;
	int height = 1;
	int levels = 0;
	buffer.GetDimensions( mipLevel, width, height, levels );
	return int2( width, height );
}

static int2 textureSize( Texture2D<float> buffer, int mipLevel )
{
	int width = 1;
	int height = 1;
	int levels = 0;
	buffer.GetDimensions( mipLevel, width, height, levels );
	return int2( width, height );
}

static float2 vposToScreenPosTexCoord( float2 vpos )
{
	return vpos.xy * rpWindowCoord.xy;
}

#define BRANCH
#define IFANY

//note: works for structured patterns too
// [0;1[
float RemapNoiseTriErp( const float v )
{
	float r2 = 0.5 * v;
	float f1 = sqrt( r2 );
	float f2 = 1.0 - sqrt( r2 - 0.25 );
	return ( v < 0.5 ) ? f1 : f2;
}

//note: returns [-intensity;intensity[, magnitude of 2x intensity
//note: from "NEXT GENERATION POST PROCESSING IN CALL OF DUTY: ADVANCED WARFARE"
//      http://advances.realtimerendering.com/s2014/index.html
float InterleavedGradientNoise( float2 uv )
{
	const float3 magic = float3( 0.06711056, 0.00583715, 52.9829189 );
	float rnd = frac( magic.z * frac( dot( uv, magic.xy ) ) );

	return rnd;
}

// this noise, including the 5.58... scrolling constant are from Jorge Jimenez
float InterleavedGradientNoiseAnim( float2 uv, float frameIndex )
{
	uv += ( frameIndex * 5.588238f );

	const float3 magic = float3( 0.06711056, 0.00583715, 52.9829189 );
	float rnd = frac( magic.z * frac( dot( uv, magic.xy ) ) );

	return rnd;
}

// RB: very efficient white noise without sine https://www.shadertoy.com/view/4djSRW
#define HASHSCALE3 float3(443.897, 441.423, 437.195)

float3 Hash33( float3 p3 )
{
	p3 = frac( p3 * HASHSCALE3 );
	p3 += dot( p3, p3.yxz + 19.19 );
	return frac( ( p3.xxy + p3.yxx ) * p3.zyx );
}

/*
static float3 DitherRGB( float3 color, float2 uvSeed, float quantSteps )
{
	// uniform noise
	//float3 noise = Hash33( float3( uvSeed, rpJitterTexOffset.w ) );

	//float3 noise = float3( InterleavedGradientNoise( uvSeed ) );
	float3 noise = _float3( InterleavedGradientNoiseAnim( uvSeed, rpJitterTexOffset.w ) );

	// triangular noise [-0.5;1.5[

#if 1
	noise.x = RemapNoiseTriErp( noise.x );
	noise = noise * 2.0 - 0.5;
#endif

	noise = _float3( noise.x );

	// quantize/truncate color and dither the result
	//float scale = exp2( float( TARGET_BITS ) ) - 1.0;

	// lets assume 2^3 bits = 8
	//float scale = 7.0;
	//const float quantSteps = 8.0;
	float scale = quantSteps - 1.0;

	// apply dither
	color += noise / ( quantSteps );

	color = floor( color * scale ) / scale;

	//float3 color = c + whiteNoise / 255.0;

	return color;
}
*/

/*
static float3 DitherChromaticBlueNoise( float3 color, float2 n, SamplerState blueTex )
{
	// uniform noise
	//float3 noise = Hash33( float3( n, rpJitterTexOffset.w ) );

	//float3 noise = float3( InterleavedGradientNoise( n ) );
	//float3 noise = float3( InterleavedGradientNoiseAnim( n, rpJitterTexOffset.w ) );

	// uv is screen position / sizeof blue noise image
	float2 uv = n.xy * rpJitterTexOffset.xy;
	float3 noise = tex2D( blueTex, uv ).rgb;

	// rpJitterTexOffset.w is frameTime % 64
	noise = frac( noise + c_goldenRatioConjugate * rpJitterTexOffset.w );

	// triangular noise [-0.5;1.5[
	noise.x = RemapNoiseTriErp( noise.x );
	noise = noise * 2.0 - 0.5;

	//noise = float3( noise.x );

	// quantize/truncate color and dither the result
	//float scale = exp2( float( TARGET_BITS ) ) - 1.0;

	// lets assume 2^3 bits = 8
	float quantSteps = 255.0;

	//float3 color = floor( c * scale + noise ) / scale;

	color = floor( 0.5 + color * quantSteps - 0.5 + noise ) * ( 1.0 / ( quantSteps - 1.0 ) );

	return color;
}
*/

#define SMAA_RT_METRICS float4(1.0 / 1280.0, 1.0 / 720.0, 1280.0, 720.0)