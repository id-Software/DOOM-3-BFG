/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company. 
Copyright (C) 2013-2016 Robert Beckebans

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

// RB begin
#ifndef PI
#define PI	3.14159265358979323846
#endif

#define DEG2RAD( a )				( ( a ) * PI / 180.0f )
#define RAD2DEG( a )				( ( a ) * 180.0f / PI )

// ----------------------
// sRGB <-> Linear RGB Color Conversion
// ----------------------

half3 sRGBToLinearRGB( half3 rgb )
{
#if defined( USE_LINEAR_RGB ) && !defined( USE_SRGB )
	return max( pow( rgb, half3( 2.2 ) ), half3( 0.0 ) );
#else
	return rgb;
#endif
}

half4 sRGBAToLinearRGBA( half4 rgba )
{
#if defined( USE_LINEAR_RGB ) && !defined( USE_SRGB )
	return float4( max( pow( rgba.rgb, half3( 2.2 ) ), half3( 0.0 ) ), rgba.a );
#else
	return rgba;
#endif
}

half3 LinearRGBToSRGB( half3 rgb )
{
#if defined( USE_LINEAR_RGB ) && !defined( USE_SRGB )
	return pow( rgb, half3( 1.0 ) / half3( 2.2 ) );
#else
	return rgb;
#endif
}

half4 LinearRGBToSRGB( half4 rgba )
{
#if defined( USE_LINEAR_RGB ) && !defined( USE_SRGB )
	rgba.rgb = pow( rgba.rgb, half3( 1.0 ) / half3( 2.2 ) );
	return rgba; //pow( rgba, half4( 1.0 ) / half4( 2.2 ) );
#else
	return rgba;
#endif
}


// RB end

// ----------------------
// YCoCg Color Conversion
// ----------------------
static const half4 matrixRGB1toCoCg1YX = half4(  0.50,  0.0, -0.50, 0.50196078 );	// Co
static const half4 matrixRGB1toCoCg1YY = half4( -0.25,  0.5, -0.25, 0.50196078 );	// Cg
static const half4 matrixRGB1toCoCg1YZ = half4(  0.0,   0.0,  0.0,  1.0 );			// 1.0
static const half4 matrixRGB1toCoCg1YW = half4(  0.25,  0.5,  0.25, 0.0 );			// Y

static const half4 matrixCoCg1YtoRGB1X = half4(  1.0, -1.0,  0.0,        1.0 );
static const half4 matrixCoCg1YtoRGB1Y = half4(  0.0,  1.0, -0.50196078, 1.0 ); // -0.5 * 256.0 / 255.0
static const half4 matrixCoCg1YtoRGB1Z = half4( -1.0, -1.0,  1.00392156, 1.0 ); // +1.0 * 256.0 / 255.0

static half3 ConvertYCoCgToRGB( half4 YCoCg ) {
	half3 rgbColor;

	YCoCg.z = ( YCoCg.z * 31.875 ) + 1.0;			//z = z * 255.0/8.0 + 1.0
	YCoCg.z = 1.0 / YCoCg.z;
	YCoCg.xy *= YCoCg.z;
	rgbColor.x = dot4( YCoCg, matrixCoCg1YtoRGB1X );
	rgbColor.y = dot4( YCoCg, matrixCoCg1YtoRGB1Y );
	rgbColor.z = dot4( YCoCg, matrixCoCg1YtoRGB1Z );
	return rgbColor;
}

static float2 CenterScale( float2 inTC, float2 centerScale ) {
	float scaleX = centerScale.x;
	float scaleY = centerScale.y;
	float4 tc0 = float4( scaleX, 0, 0, 0.5 - ( 0.5f * scaleX ) );
	float4 tc1 = float4( 0, scaleY, 0, 0.5 - ( 0.5f * scaleY ) );

	float2 finalTC;
	finalTC.x = dot4( inTC, tc0 );
	finalTC.y = dot4( inTC, tc1 );
	return finalTC;
}

static float2 Rotate2D( float2 inTC, float2 cs ) {
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
float rand( float2 co ) {
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
static float4 idtex2Dproj( sampler2D samp, float4 texCoords ) { return tex2Dproj( samp, texCoords.xyw ); }
static float4 swizzleColor( float4 c )
{ 
	return c;
	//return sRGBAToLinearRGBA( c ); 
}
static float2 vposToScreenPosTexCoord( float2 vpos ) { return vpos.xy * rpWindowCoord.xy; }

#define BRANCH
#define IFANY
