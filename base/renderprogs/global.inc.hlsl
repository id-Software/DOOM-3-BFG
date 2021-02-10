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

// *INDENT-OFF*
uniform float4 rpScreenCorrectionFactor	:	register(c0);
uniform float4 rpWindowCoord			:	register(c1);
uniform float4 rpDiffuseModifier		:	register(c2);
uniform float4 rpSpecularModifier		:	register(c3);

uniform float4 rpLocalLightOrigin		:	register(c4);
uniform float4 rpLocalViewOrigin		:	register(c5);

uniform float4 rpLightProjectionS		:	register(c6);
uniform float4 rpLightProjectionT		:	register(c7);
uniform float4 rpLightProjectionQ		:	register(c8);
uniform float4 rpLightFalloffS			:	register(c9);

uniform float4 rpBumpMatrixS			:	register(c10);
uniform float4 rpBumpMatrixT			:	register(c11);

uniform float4 rpDiffuseMatrixS			:	register(c12);
uniform float4 rpDiffuseMatrixT			:	register(c13);

uniform float4 rpSpecularMatrixS		:	register(c14);
uniform float4 rpSpecularMatrixT		:	register(c15);

uniform float4 rpVertexColorModulate	:	register(c16);
uniform float4 rpVertexColorAdd			:	register(c17);

uniform float4 rpColor					:	register(c18);
uniform float4 rpViewOrigin				:	register(c19);
uniform float4 rpGlobalEyePos			:	register(c20);

uniform float4 rpMVPmatrixX				:	register(c21);
uniform float4 rpMVPmatrixY				:	register(c22);
uniform float4 rpMVPmatrixZ				:	register(c23);
uniform float4 rpMVPmatrixW				:	register(c24);

uniform float4 rpModelMatrixX			:	register(c25);
uniform float4 rpModelMatrixY			:	register(c26);
uniform float4 rpModelMatrixZ			:	register(c27);
uniform float4 rpModelMatrixW			:	register(c28);

uniform float4 rpProjectionMatrixX		:	register(c29);
uniform float4 rpProjectionMatrixY		:	register(c30);
uniform float4 rpProjectionMatrixZ		:	register(c31);
uniform float4 rpProjectionMatrixW		:	register(c32);

uniform float4 rpModelViewMatrixX		:	register(c33);
uniform float4 rpModelViewMatrixY		:	register(c34);
uniform float4 rpModelViewMatrixZ		:	register(c35);
uniform float4 rpModelViewMatrixW		:	register(c36);

uniform float4 rpTextureMatrixS			:	register(c37);
uniform float4 rpTextureMatrixT			:	register(c38);

uniform float4 rpTexGen0S				:	register(c39);
uniform float4 rpTexGen0T				:	register(c40);
uniform float4 rpTexGen0Q				:	register(c41);
uniform float4 rpTexGen0Enabled			:	register(c42);

uniform float4 rpTexGen1S				:	register(c43);
uniform float4 rpTexGen1T				:	register(c44);
uniform float4 rpTexGen1Q				:	register(c45);
uniform float4 rpTexGen1Enabled			:	register(c46);

uniform float4 rpWobbleSkyX				:	register(c47);
uniform float4 rpWobbleSkyY				:	register(c48);
uniform float4 rpWobbleSkyZ				:	register(c49);

uniform float4 rpOverbright				:	register(c50);
uniform float4 rpEnableSkinning			:	register(c51);
uniform float4 rpAlphaTest				:	register(c52);

// RB begin
uniform float4 rpAmbientColor			:	register(c53);
uniform float4 rpGlobalLightOrigin		:	register(c54);
uniform float4 rpJitterTexScale			:	register(c55);
uniform float4 rpJitterTexOffset		:	register(c56);
uniform float4 rpCascadeDistances		:	register(c57);

#if defined( USE_UNIFORM_ARRAYS )
uniform float4 rpShadowMatrices			:	register(c58);
#else
uniform float4 rpShadowMatrices[6*4]	:	register(c59);
#endif
// RB end

static float dot2( float2 a, float2 b ) { return dot( a, b ); }
static float dot3( float3 a, float3 b ) { return dot( a, b ); }
static float dot3( float3 a, float4 b ) { return dot( a, b.xyz ); }
static float dot3( float4 a, float3 b ) { return dot( a.xyz, b ); }
static float dot3( float4 a, float4 b ) { return dot( a.xyz, b.xyz ); }
static float dot4( float4 a, float4 b ) { return dot( a, b ); }
static float dot4( float2 a, float4 b ) { return dot( float4( a, 0, 1 ), b ); }

// *INDENT-ON*

// RB begin
#ifndef PI
	#define PI	3.14159265358979323846
#endif

#define DEG2RAD( a )				( ( a ) * PI / 180.0f )
#define RAD2DEG( a )				( ( a ) * 180.0f / PI )


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

float3 sRGBToLinearRGB( float3 c )
{
#if defined( USE_LINEAR_RGB ) && !defined( USE_SRGB )
	c = clamp( c, 0.0, 1.0 );

	return Linear3( c );
#else
	return c;
#endif
}

float4 sRGBAToLinearRGBA( float4 c )
{
#if defined( USE_LINEAR_RGB ) && !defined( USE_SRGB )
	c = clamp( c, 0.0, 1.0 );

	return float4( Linear1( c.r ), Linear1( c.g ), Linear1( c.b ), Linear1( c.a ) );
#else
	return c;
#endif
}

float3 LinearRGBToSRGB( float3 c )
{
#if defined( USE_LINEAR_RGB ) && !defined( USE_SRGB )
	c = clamp( c, 0.0, 1.0 );

	return Srgb3( c );
#else
	return c;
#endif
}

float4 LinearRGBToSRGB( float4 c )
{
#if defined( USE_LINEAR_RGB ) && !defined( USE_SRGB )
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
static const half4 matrixRGB1toCoCg1YX = half4( 0.50,  0.0, -0.50, 0.50196078 );	// Co
static const half4 matrixRGB1toCoCg1YY = half4( -0.25,  0.5, -0.25, 0.50196078 );	// Cg
static const half4 matrixRGB1toCoCg1YZ = half4( 0.0,   0.0,  0.0,  1.0 );			// 1.0
static const half4 matrixRGB1toCoCg1YW = half4( 0.25,  0.5,  0.25, 0.0 );			// Y

static const half4 matrixCoCg1YtoRGB1X = half4( 1.0, -1.0,  0.0,        1.0 );
static const half4 matrixCoCg1YtoRGB1Y = half4( 0.0,  1.0, -0.50196078, 1.0 );  // -0.5 * 256.0 / 255.0
static const half4 matrixCoCg1YtoRGB1Z = half4( -1.0, -1.0,  1.00392156, 1.0 ); // +1.0 * 256.0 / 255.0

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

static const half4 LUMINANCE_SRGB = half4( 0.2125, 0.7154, 0.0721, 0.0 );
static const half4 LUMINANCE_LINEAR = half4( 0.299, 0.587, 0.144, 0.0 );

#define _half2( x )		half2( x )
#define _half3( x )		half3( x )
#define _half4( x )		half4( x )
#define _float2( x )	float2( x )
#define _float3( x )	float3( x )
#define _float4( x )	float4( x )

#define VPOS WPOS
static float4 idtex2Dproj( sampler2D samp, float4 texCoords )
{
	return tex2Dproj( samp, texCoords.xyw );
}
static float4 swizzleColor( float4 c )
{
	return c;
	//return sRGBAToLinearRGBA( c );
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
	float rnd = fract( magic.z * fract( dot( uv, magic.xy ) ) );

	return rnd;
}

// this noise, including the 5.58... scrolling constant are from Jorge Jimenez
float InterleavedGradientNoiseAnim( float2 uv, float frameIndex )
{
	uv += ( frameIndex * 5.588238f );

	const float3 magic = float3( 0.06711056, 0.00583715, 52.9829189 );
	float rnd = fract( magic.z * fract( dot( uv, magic.xy ) ) );

	return rnd;
}

// RB: the golden ratio is useful to animate Blue noise
const float c_goldenRatioConjugate = 0.61803398875;


// RB: very efficient white noise without sine https://www.shadertoy.com/view/4djSRW
#define HASHSCALE3 float3(443.897, 441.423, 437.195)

float3 Hash33( float3 p3 )
{
	p3 = fract( p3 * HASHSCALE3 );
	p3 += dot( p3, p3.yxz + 19.19 );
	return fract( ( p3.xxy + p3.yxx ) * p3.zyx );
}

static float3 ditherRGB( float3 color, float2 uvSeed, float quantSteps )
{
	// uniform noise
	//float3 noise = Hash33( float3( uvSeed, rpJitterTexOffset.w ) );

	//float3 noise = float3( InterleavedGradientNoise( uvSeed ) );
	float3 noise = float3( InterleavedGradientNoiseAnim( uvSeed, rpJitterTexOffset.w ) );


	// triangular noise [-0.5;1.5[

#if 1
	noise.x = RemapNoiseTriErp( noise.x );
	noise = noise * 2.0 - 0.5;
#endif

	noise = float3( noise.x );


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

#if defined( USE_LINEAR_RGB )

#endif

	return color;
}

static float3 ditherChromaticBlueNoise( float3 color, float2 n, sampler2D blueTex )
{
	// uniform noise
	//float3 noise = Hash33( float3( n, rpJitterTexOffset.w ) );

	//float3 noise = float3( InterleavedGradientNoise( n ) );
	//float3 noise = float3( InterleavedGradientNoiseAnim( n, rpJitterTexOffset.w ) );

	// uv is screen position / sizeof blue noise image
	float2 uv = n.xy * rpJitterTexOffset.xy;
	float3 noise = tex2D( blueTex, uv ).rgb;

	// rpJitterTexOffset.w is frameTime % 64
	noise = fract( noise + c_goldenRatioConjugate * rpJitterTexOffset.w );

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
