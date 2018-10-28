
#include "global.inc.hlsl"

// #version 120 // -*- c++ -*-
// #extension GL_EXT_gpu_shader4 : require
// #extension GL_ARB_gpu_shader5 : enable

#define DIFFERENT_DEPTH_RESOLUTIONS 0
#define USE_DEPTH_PEEL 0
#define CS_Z_PACKED_TOGETHER 0
#define TEMPORALLY_VARY_TAPS 0
#define USE_OCT16 0
#define COMPUTE_PEELED_LAYER 0
#define USE_MIPMAPS 1
#define USE_TAP_NORMAL 1

#define HIGH_QUALITY 0

#if HIGH_QUALITY
// Total number of direct samples to take at each pixel
#define NUM_SAMPLES 39

// This is the number of turns around the circle that the spiral pattern makes.  This should be prime to prevent
// taps from lining up.  This particular choice was tuned for NUM_SAMPLES == 9
#define NUM_SPIRAL_TURNS 14

#else

#define NUM_SAMPLES 11
#define NUM_SPIRAL_TURNS 7

#endif

// If using depth mip levels, the log of the maximum pixel offset before we need to switch to a lower
// miplevel to maintain reasonable spatial locality in the cache
// If this number is too small (< 3), too many taps will land in the same pixel, and we'll get bad variance that manifests as flashing.
// If it is too high (> 5), we'll get bad performance because we're not using the MIP levels effectively
#define LOG_MAX_OFFSET (3)

// This must be less than or equal to the MAX_MIP_LEVEL defined in SAmbientOcclusion.cpp
#define MAX_MIP_LEVEL (5)

#define MIN_MIP_LEVEL 0

const float DOOM_TO_METERS = 0.0254;					// doom to meters
const float METERS_TO_DOOM = ( 1.0 / DOOM_TO_METERS );	// meters to doom

/** Used for preventing AO computation on the sky (at infinite depth) and defining the CS Z to bilateral depth key scaling.
    This need not match the real far plane but should not be much more than it.*/
const float FAR_PLANE_Z = -4000.0;

/** World-space AO radius in scene units (r).  e.g., 1.0m */
const float radius = 1.0 * METERS_TO_DOOM;
const float radius2 = radius * radius;
const float invRadius2 = 1.0 / radius2;

/** Bias to avoid AO in smooth corners, e.g., 0.01m */
const float bias = 0.01 * METERS_TO_DOOM;

/** The height in pixels of a 1m object if viewed from 1m away.
    You can compute it from your projection matrix.  The actual value is just
    a scale factor on radius; you can simply hardcode this to a constant (~500)
    and make your radius value unitless (...but resolution dependent.)  */
const float projScale = 500.0;

// #expect NUM_SAMPLES "Integer number of samples to take at each pixel"
// #expect NUM_SPIRAL_TURNS "Integer number of turns around the circle that the spiral pattern makes. The DeepGBufferRadiosity class provides a discrepancy-minimizing value of NUM_SPIRAL_TURNS for each value of NUM_SAMPLES."
// #expect USE_MIPMAPS "1 to enable, 0 to disable"
// #expect USE_DEPTH_PEEL "1 to enable, 0 to disable"
// #expect COMPUTE_PEELED_LAYER "1 to enable, 0 to disable"
// #expect USE_TAP_NORMAL "1 to enable, 0 to disable"
// #expect MIN_MIP_LEVEL "non-negative integer"
// #expect USE_OCT16 "1 to enable, 0 to disable"
// #if (!USE_DEPTH_PEEL) && COMPUTE_PEELED_LAYER
// #error "If computing a peeled layer, must be using depth peel!"
// #endif

// *INDENT-OFF*
uniform sampler2D samp0 : register( s0 ); // view normals
uniform sampler2D samp1 : register( s1 ); // view depth
uniform sampler2D samp2 : register( s2 ); // colors
 
#define normal_buffer	samp0
#define CS_Z_buffer		samp1
#define colorBuffer		samp2
 
struct PS_IN
{
	float2 texcoord0 : TEXCOORD0_centroid;
};
 
struct PS_OUT 
{
	float4 color : COLOR;
};
// *INDENT-ON*



// #if 0 //normal_notNull
// /** Same size as result buffer, do not offset by guard band when reading from it */
// uniform sampler2D       normal_buffer;
// uniform float4          normal_readMultiplyFirst;
// uniform float4          normal_readAddSecond;

// #if USE_OCT16
// #include <oct.glsl>
// #endif

// #endif




#if USE_DEPTH_PEEL
uniform sampler2D       peeledColorBuffer;
uniform sampler2D       peeledNormalBuffer;
#endif

//uniform float           indirectMultiplier;


// Compatibility with future versions of GLSL: the shader still works if you change the
// version line at the top to something like #version 330 compatibility.
// #if __VERSION__ == 120
// #   define texelFetch   texelFetch2D
// #   define textureSize  textureSize2D
// #   define result gl_FragData[0]
// #   else
// out float4            result;
// #endif

// #if COMPUTE_PEELED_LAYER
// #   if __VERSION__ == 120
// #       define peeledResult gl_FragData[1]
// #   else
// out float4       peeledResult;
// #   endif
// #   define indirectPeeledResult  peeledResult.rgb
// #   define peeledVisibility      peeledResult.a
// #endif

#define indirectColor   result.color.rgb
#define visibility      result.color.a

/////////////////////////////////////////////////////////

float reconstructCSZ( float d )
{
	//return clipInfo[0] / (clipInfo[1] * d + clipInfo[2]);
	
	// infinite far perspective matrix
	return -3.0 / ( -1.0 * d + 1.0 );
	
	//d = d * 2.0 - 1.0;
	//return -rpProjectionMatrixZ.w / ( -rpProjectionMatrixZ.z - d );
}

float3 reconstructCSPosition( float2 S, float z )
{
	float4 P;
	P.z = z * 2.0 - 1.0;
	//P.z = reconstructCSZ( z );
	P.xy = ( S * rpScreenCorrectionFactor.xy ) * 2.0 - 1.0;
	P.w = 1.0;
	
	float4 csP;
	csP.x = dot4( P, rpModelMatrixX );
	csP.y = dot4( P, rpModelMatrixY );
	csP.z = dot4( P, rpModelMatrixZ );
	csP.w = dot4( P, rpModelMatrixW );
	
	csP.xyz /= csP.w;
	
	return csP.xyz;
}

float3 sampleNormal( sampler2D normalBuffer, int2 ssC, int mipLevel )
{
#if USE_OCT16
	return decode16( texelFetch( normalBuffer, ssC, mipLevel ).xy * 2.0 - 1.0 );
#else
	return texelFetch( normalBuffer, ssC, mipLevel ).xyz * 2.0 - 1.0;
#endif
}

void sampleBothNormals( sampler2D normalBuffer, int2 ssC, int mipLevel, out float3 n_tap0, out float3 n_tap1 )
{
	float4 encodedNormals = texelFetch( normalBuffer, ssC, mipLevel ) * 2.0 - 1.0;
	n_tap0 = decode16( encodedNormals.xy );
	n_tap1 = decode16( encodedNormals.zw );
}

/** Returns a unit vector and a screen-space radius for the tap on a unit disk (the caller should scale by the actual disk radius) */
float2 tapLocation( int sampleNumber, float spinAngle, float radialJitter, out float ssR )
{
	// Radius relative to ssR
	float alpha = ( float( sampleNumber ) + radialJitter) * ( 1.0 / float( NUM_SAMPLES ) );
	float angle = alpha * ( float( NUM_SPIRAL_TURNS ) * 6.28 ) + spinAngle;
	
	ssR = alpha;
	return float2( cos( angle ), sin( angle ) );
}


/** Read the camera-space position of the point at screen-space pixel ssP */
float3 getPosition( int2 ssP, sampler2D cszBuffer )
{
	float3 P;
	P.z = texelFetch( cszBuffer, ssP, 0 ).r;
	
	// Offset to pixel center
	P = reconstructCSPosition( float2( ssP ) + float2( 0.5 ), P.z );
	
	return P;
}


void getPositions( int2 ssP, sampler2D cszBuffer, out float3 P0, out float3 P1 )
{
	float2 Zs = texelFetch( cszBuffer, ssP, 0 ).rg;
	
	// Offset to pixel center
	P0 = reconstructCSPosition( float2( ssP ) + float2( 0.5 ), Zs.x );
	P1 = reconstructCSPosition( float2( ssP ) + float2( 0.5 ), Zs.y );
}


void computeMipInfo( float ssR, int2 ssP, sampler2D cszBuffer, inout int mipLevel, inout int2 mipP )
{
#if USE_MIPMAPS
	// Derivation:
	//  mipLevel = floor(log(ssR / MAX_OFFSET));
#ifdef GL_EXT_gpu_shader5
	mipLevel = clamp( findMSB( int( ssR ) ) - LOG_MAX_OFFSET, MIN_MIP_LEVEL, MAX_MIP_LEVEL );
#else
	mipLevel = clamp( int( floor( log2( ssR ) ) ) - LOG_MAX_OFFSET, MIN_MIP_LEVEL, MAX_MIP_LEVEL );
#endif
	
	// We need to divide by 2^mipLevel to read the appropriately scaled coordinate from a MIP-map.
	// Manually clamp to the texture size because texelFetch bypasses the texture unit
	//mipP = ssP >> mipLevel;//clamp(ssP >> mipLevel, int2(0), textureSize(CS_Z_buffer, mipLevel) - int2(1));
	
	mipP = clamp( ssP >> mipLevel, int2( 0 ), textureSize( cszBuffer, mipLevel ) - int2( 1 ) );
#else
	mipLevel = 0;
	mipP = ssP;
#endif
}


/** Read the camera-space position of the point at screen-space pixel ssP + unitOffset * ssR.  Assumes length(unitOffset) == 1.
    Use cszBufferScale if reading from the peeled depth buffer, which has been scaled by (1 / invCszBufferScale) from the original */
float3 getOffsetPosition( int2 ssC, float2 unitOffset, float ssR, sampler2D cszBuffer, float invCszBufferScale )
{
	int2 ssP = clamp( int2( ssR * unitOffset ) + ssC, int2( 0 ), int2( g3d_sz2D_colorBuffer.xy - 1 ) );
	
	int mipLevel;
	int2 mipP;
	computeMipInfo( ssR, ssP, cszBuffer, mipLevel, mipP );
	
	float3 P;
	
	P.z = texelFetch( cszBuffer, mipP, mipLevel ).r;
	
	// Offset to pixel center
	P = reconstructCSPosition( ( float2( ssP ) + float2( 0.5 ) ) * invCszBufferScale, P.z );
	
	return P;
}


/** Read the camera-space position of the points at screen-space pixel ssP + unitOffset * ssR in both channels of the packed csz buffer.  Assumes length(unitOffset) == 1. */
void getOffsetPositions( int2 ssC, float2 unitOffset, float ssR, sampler2D cszBuffer, out float3 P0, out float3 P1 )
{
	int2 ssP = clamp( int2( ssR * unitOffset ) + ssC, int2( 0 ), int2( g3d_sz2D_colorBuffer.xy - 1 ) );
	
	int mipLevel;
	int2 mipP;
	computeMipInfo( ssR, ssP, cszBuffer, mipLevel, mipP );
	
	float2 Zs = texelFetch( cszBuffer, mipP, mipLevel ).rg;
	
	// Offset to pixel center
	P0 = reconstructCSPosition( ( float2( ssP ) + float2( 0.5 ) ), Zs.x );
	P1 = reconstructCSPosition( ( float2( ssP ) + float2( 0.5 ) ), Zs.y );
}


void getOffsetPositionNormalAndLambertian
( int2             ssP,
  float            ssR,
  sampler2D        cszBuffer,
  sampler2D        bounceBuffer,
  sampler2D        normalBuffer,
  inout float3     Q,
  inout float3     lambertian_tap,
  inout float3     n_tap )
{

#if USE_MIPMAPS
	int mipLevel;
	int2 texel;
	computeMipInfo( ssR, ssP, cszBuffer, mipLevel, texel );
#else
	int mipLevel = 0;
	int2 texel = ssP;
#endif
	
	float z = texelFetch( cszBuffer, texel, mipLevel ).r;
	
	// FIXME mip map bounce/normal buffers FBOs
#if 0
	float3 n = sampleNormal( normalBuffer, texel, mipLevel );
	lambertian_tap = texelFetch( bounceBuffer, texel, mipLevel ).rgb;
#else
	float3 n = sampleNormal( normalBuffer, ssP, 0 );
	lambertian_tap = texelFetch( bounceBuffer, ssP, 0 ).rgb;
#endif

	//n_tap = normalize( n );	
	n_tap = n;
	
	// Offset to pixel center
	Q = reconstructCSPosition( ( float2( ssP ) + float2( 0.5 ) ), z );
}


void getOffsetPositionsNormalsAndLambertians
( int2           ssP,
  float           ssR,
  sampler2D       cszBuffer,
  sampler2D       bounceBuffer,
  sampler2D       peeledBounceBuffer,
  sampler2D       normalBuffer,
  sampler2D       peeledNormalBuffer,
  out float3      Q0,
  out float3      Q1,
  out float3      lambertian_tap0,
  out float3      lambertian_tap1,
  out float3     n_tap0,
  out float3     n_tap1 )
{

#if USE_MIPMAPS
	int mipLevel;
	int2 texel;
	computeMipInfo( ssR, ssP, cszBuffer, mipLevel, texel );
#else
	int mipLevel = 0;
	int2 texel = ssP;
#endif
	
	float2 Zs = texelFetch( cszBuffer, texel, mipLevel ).rg;
#if USE_OCT16
	sampleBothNormals( normalBuffer, texel, mipLevel, n_tap0, n_tap1 );
#else
	n_tap0 = sampleNormal( normalBuffer, texel, mipLevel );
	n_tap1 = sampleNormal( peeledNormalBuffer, texel, mipLevel );
#endif
	
	lambertian_tap0 = texelFetch( bounceBuffer, texel, mipLevel ).rgb;
	lambertian_tap1 = texelFetch( peeledBounceBuffer, texel, mipLevel ).rgb;
	
	// Offset to pixel center
	Q0 = reconstructCSPosition( ( float2( ssP ) + float2( 0.5 ) ), Zs.x, projInfo );
	Q1 = reconstructCSPosition( ( float2( ssP ) + float2( 0.5 ) ), Zs.y, projInfo );
}


void iiValueFromPositionsAndNormalsAndLambertian( int2 ssP, float3 X, float3 n_X, float3 Y, float3 n_Y, float3 radiosity_Y, inout float3 E, inout float weight_Y, inout float visibilityWeight_Y )
{

	float3 YminusX = Y - X;
	float3 w_i = normalize( YminusX );
	weight_Y = ( ( dot( w_i, n_X ) > 0.0 )
#if USE_TAP_NORMAL
				 && ( dot( -w_i, n_Y ) > 0.01 )
#endif
			   ) ? 1.0 : 0.0; // Backface check
			   
	// E = radiosity_Y * dot(w_i, n_X) * weight_Y * float(dot(YminusX, YminusX) < radius2);
	
	if( ( dot( YminusX, YminusX ) < radius2 ) && // Radius check
			( weight_Y > 0.0 ) )
	{
		E = radiosity_Y * dot( w_i, n_X );
	}
	else
	{
#if USE_TAP_NORMAL == 0
		weight_Y = 0;
#endif
		E = float3( 0 );
	}
}


/** Compute the occlusion due to sample with index \a i about the pixel at \a ssC that corresponds
    to camera-space point \a C with unit normal \a n_C, using maximum screen-space sampling radius \a ssDiskRadius

    When sampling from the peeled depth buffer, make sure ssDiskRadius has been premultiplied by cszBufferScale
*/
void sampleIndirectLight
( in int2             ssC,
  in float3           C,
  in float3           n_C,
  in float3           C_peeled,
  in float3           n_C_peeled,
  in float            ssDiskRadius,
  in int              tapIndex,
  in float            randomPatternRotationAngle,
  in float            radialJitter,
  in sampler2D        cszBuffer,
  in sampler2D        nBuffer,
  in sampler2D        bounceBuffer,
  inout float3        irradianceSum,
  inout float         numSamplesUsed,
  inout float3        iiPeeled,
  inout float         weightSumPeeled )
{

	// Not used yet, quality optimization in progress...
	float visibilityWeightPeeled0, visibilityWeightPeeled1;
	
	// Offset on the unit disk, spun for this pixel
	float ssR;
	float2 unitOffset = tapLocation( tapIndex, randomPatternRotationAngle, radialJitter, ssR );
	ssR *= ssDiskRadius;
	int2 ssP = int2( ssR * unitOffset ) + ssC;
	
#if USE_DEPTH_PEEL
	float3 E, ii_tap0, ii_tap1;
	float weight, weight0, weight1;
	float visibilityWeight0, visibilityWeight1;
	// The occluding point in camera space
	float3 Q0, lambertian_tap0, n_tap0, Q1, lambertian_tap1, n_tap1;
	getOffsetPositionsNormalsAndLambertians( ssP, ssR, cszBuffer, bounceBuffer, peeledColorBuffer, nBuffer, peeledNormalBuffer, Q0, Q1, lambertian_tap0, lambertian_tap1, n_tap0, n_tap1 );
	iiValueFromPositionsAndNormalsAndLambertian( ssP, C, n_C, Q0, n_tap0, lambertian_tap0, ii_tap0, weight0, visibilityWeight0 );
	float adjustedWeight0 = weight0 * dot( ii_tap0, ii_tap0 ) + weight0;
	
	iiValueFromPositionsAndNormalsAndLambertian( ssP, C, n_C, Q1, n_tap1, lambertian_tap1, ii_tap1, weight1, visibilityWeight1 );
	float adjustedWeight1 = weight1 * dot( ii_tap1, ii_tap1 ) + weight1;
	
	weight = ( adjustedWeight0 > adjustedWeight1 ) ? weight0 : weight1;
	E = ( adjustedWeight0 > adjustedWeight1 ) ? ii_tap0 : ii_tap1;
	
#if COMPUTE_PEELED_LAYER
	
	float weightPeeled0, weightPeeled1;
	float3 ii_tapPeeled0, ii_tapPeeled1;
	iiValueFromPositionsAndNormalsAndLambertian( ssP, C_peeled, n_C_peeled, Q0, n_tap0, lambertian_tap0, ii_tapPeeled0, weightPeeled0, visibilityWeightPeeled0 );
	iiValueFromPositionsAndNormalsAndLambertian( ssP, C_peeled, n_C_peeled, Q1, n_tap1, lambertian_tap1, ii_tapPeeled1, weightPeeled1, visibilityWeightPeeled1 );
	
	float iiMag0 = dot( ii_tapPeeled0, ii_tapPeeled0 );
	float iiMag1 = dot( ii_tapPeeled1, ii_tapPeeled1 );
	weightSumPeeled += iiMag0 > iiMag1 ? weightPeeled0 : weightPeeled1;
	iiPeeled        += iiMag0 > iiMag1 ? ii_tapPeeled0 : ii_tapPeeled1;
	
#endif
	
	numSamplesUsed += weight;
	
#else
	
	float3 E;
	float visibilityWeight;
	float weight_Y;
	// The occluding point in camera space
	float3 Q, lambertian_tap, n_tap;
	getOffsetPositionNormalAndLambertian( ssP, ssR, cszBuffer, bounceBuffer, nBuffer, Q, lambertian_tap, n_tap );
	iiValueFromPositionsAndNormalsAndLambertian( ssP, C, n_C, Q, n_tap, lambertian_tap, E, weight_Y, visibilityWeight );
	numSamplesUsed += weight_Y;
#endif
	
	irradianceSum += E;
	//irradianceSum += pow( E, float3( 2.2 ) ); // RB: to linear RGB
}


void main( PS_IN fragment, out PS_OUT result )
{
	result.color = float4( 0.0, 0.0, 0.0, 1.0 );
	
#if 0
	if( fragment.texcoord0.x < 0.5 )
	{
		discard;
	}
#endif
	
	// Pixel being shaded
	int2 ssC = int2( gl_FragCoord.xy );
	
#if COMPUTE_PEELED_LAYER
	float3 C, C_peeled;
	getPositions( ssC, CS_Z_buffer, C, C_peeled );
	float3 n_C_peeled = sampleNormal( peeledNormalBuffer, ssC, 0 );
#else
	// World space point being shaded
	float3 C = getPosition( ssC, CS_Z_buffer );
	float3 C_peeled = float3( 0 );
	float3 n_C_peeled = float3( 0 );
#endif
	
	float3 n_C = sampleNormal( normal_buffer, ssC, 0 );
	//n_C = normalize( n_C );
	
	
	// Choose the screen-space sample radius
	// proportional to the projected area of the sphere
	float ssDiskRadius = -projScale * radius / C.z;
	
	// Hash function used in the HPG12 AlchemyAO paper
	float randomPatternRotationAngle = float( 3 * ssC.x ^ ssC.y + ssC.x * ssC.y ) * 10.0;
#if TEMPORALLY_VARY_TAPS
	randomPatternRotationAngle += rpJitterTexOffset.x;
#endif
	
	float radialJitter = fract( sin( gl_FragCoord.x * 1e2 +
#if TEMPORALLY_VARY_TAPS
									 rpJitterTexOffset.x +
#endif
									 gl_FragCoord.y ) * 1e5 + sin( gl_FragCoord.y * 1e3 ) * 1e3 ) * 0.8 + 0.1;
									 
	float numSamplesUsed = 0.0;
	float3 irradianceSum = float3( 0 );
	float3 ii_peeled = float3( 0 );
	float peeledSum = 0.0;
	for( int i = 0; i < NUM_SAMPLES; ++i )
	{
		sampleIndirectLight( ssC, C, n_C, C_peeled, n_C_peeled, ssDiskRadius, i, randomPatternRotationAngle, radialJitter, CS_Z_buffer, normal_buffer, colorBuffer, irradianceSum, numSamplesUsed, ii_peeled, peeledSum );
	}
	
	const float solidAngleHemisphere = 2.0 * PI;
	float3 E_X = irradianceSum * solidAngleHemisphere / ( numSamplesUsed + 0.00001 );
	
	indirectColor = E_X;
	//indirectColor = pow( E_X, float3( 1.0 / 2.2 ) ); // RB: to sRGB
	
	// What is the ambient visibility of this location
	visibility = 1.0 - numSamplesUsed / float( NUM_SAMPLES );
	//visibility = clamp( 1 - numSamplesUsed / float( NUM_SAMPLES ), 0.0, 1.0 );
	//visibility = pow( max( 0.0, 1.0 - sqrt( sum * ( 3.0 / float( NUM_SAMPLES ) ) ) ), intensity );
	
	//result.color = float4( visibility, visibility, visibility, 1.0 );
	//result.color = float4( n_C * 0.5 + 0.5, 1.0 );
	//result.color = texture( samp2, fragment.texcoord0 ).rgba;
	
#if COMPUTE_PEELED_LAYER
	float A_peeled = 1.0 - peeledSum / float( NUM_SAMPLES );
	float3 E_X_peeled = ii_peeled * solidAngleHemisphere / ( peeledSum + 0.00001 );
	
	indirectPeeledResult    = E_X_peeled;
	peeledVisibility        = A_peeled;
#endif
}
