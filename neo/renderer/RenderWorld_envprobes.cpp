/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2020-2021 Robert Beckebans
Copyright (C) 2022 Stephen Pridham

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

#include "precompiled.h"
#pragma hdrstop

#include "../libs/mesa/format_r11g11b10f.h"

#include "RenderCommon.h"
#include "CmdlineProgressbar.h"
#include "../framework/Common_local.h" // commonLocal.WaitGameThread();

#if defined( USE_NVRHI )
	#include <sys/DeviceManager.h>
	extern DeviceManager* deviceManager;
#endif

/*
=============
R_SetEnvprobeDefViewEnvprobe

If the envprobeDef is not already on the viewEnvprobe list, create
a viewEnvprobe and add it to the list with an empty scissor rect.
=============
*/
viewEnvprobe_t* R_SetEnvprobeDefViewEnvprobe( RenderEnvprobeLocal* probe )
{
	if( probe->viewCount == tr.viewCount )
	{
		// already set up for this frame
		return probe->viewEnvprobe;
	}
	probe->viewCount = tr.viewCount;

	// add to the view light chain
	viewEnvprobe_t* vProbe = ( viewEnvprobe_t* )R_ClearedFrameAlloc( sizeof( *vProbe ), FRAME_ALLOC_VIEW_LIGHT );
	vProbe->envprobeDef = probe;

	// the scissorRect will be expanded as the envprobe bounds is accepted into visible portal chains
	// and the scissor will be reduced in R_AddSingleEnvprobe based on the screen space projection
	vProbe->scissorRect.Clear();

	// copy data used by backend
	// RB: this would normaly go into R_AddSingleEnvprobe
	vProbe->globalOrigin = probe->parms.origin;
	vProbe->globalProbeBounds = probe->globalProbeBounds;
	vProbe->inverseBaseProbeProject = probe->inverseBaseProbeProject;

	//if( probe->irradianceImage->IsLoaded() )
	{
		vProbe->irradianceImage = probe->irradianceImage;
	}
	//else
	//{
	//	vProbe->irradianceImage = globalImages->defaultUACIrradianceCube;
	//}

	//if( probe->radianceImage->IsLoaded() )
	{
		vProbe->radianceImage = probe->radianceImage;
	}
	//else
	//{
	//	vProbe->radianceImage = globalImages->defaultUACRadianceCube;
	//}

	// link the view light
	vProbe->next = tr.viewDef->viewEnvprobes;
	tr.viewDef->viewEnvprobes = vProbe;

	probe->viewEnvprobe = vProbe;

	return vProbe;
}


/*
================
CullEnvprobeByPortals

Return true if the light frustum does not intersect the current portal chain.
================
*/
bool idRenderWorldLocal::CullEnvprobeByPortals( const RenderEnvprobeLocal* probe, const portalStack_t* ps )
{
	if( r_useLightPortalCulling.GetInteger() == 1 )
	{
		ALIGNTYPE16 frustumCorners_t corners;
		idRenderMatrix::GetFrustumCorners( corners, probe->inverseBaseProbeProject, bounds_zeroOneCube );
		for( int i = 0; i < ps->numPortalPlanes; i++ )
		{
			if( idRenderMatrix::CullFrustumCornersToPlane( corners, ps->portalPlanes[i] ) == FRUSTUM_CULL_FRONT )
			{
				return true;
			}
		}

	}

	return false;
}

/*
===================
AddAreaViewEnvprobes

This is the only point where lights get added to the viewLights list.
Any lights that are visible through the current portalStack will have their scissor rect updated.
===================
*/
void idRenderWorldLocal::AddAreaViewEnvprobes( int areaNum, const portalStack_t* ps )
{
	portalArea_t* area = &portalAreas[ areaNum ];

	for( areaReference_t* lref = area->envprobeRefs.areaNext; lref != &area->envprobeRefs; lref = lref->areaNext )
	{
		RenderEnvprobeLocal* probe = lref->envprobe;

		// debug tool to allow viewing of only one light at a time
		if( r_singleEnvprobe.GetInteger() >= 0 && r_singleEnvprobe.GetInteger() != probe->index )
		{
			continue;
		}

#if 0
		// check for being closed off behind a door
		// a light that doesn't cast shadows will still light even if it is behind a door
		if( r_useLightAreaCulling.GetBool() //&& !envprobe->LightCastsShadows()
				&& probe->areaNum != -1 && !tr.viewDef->connectedAreas[ probe->areaNum ] )
		{
			continue;
		}

		// cull frustum
		if( CullEnvprobeByPortals( probe, ps ) )
		{
			// we are culled out through this portal chain, but it might
			// still be visible through others
			continue;
		}
#endif

		viewEnvprobe_t* vProbe = R_SetEnvprobeDefViewEnvprobe( probe );

		// expand the scissor rect
		vProbe->scissorRect.Union( ps->rect );
	}
}

/*
==================
R_SampleCubeMapHDR
==================
*/
void R_SampleCubeMapHDR( const idVec3& dir, int size, byte* buffers[6], float result[3], float& u, float& v )
{
	float	adir[3];
	int		axis, x, y;

	adir[0] = fabs( dir[0] );
	adir[1] = fabs( dir[1] );
	adir[2] = fabs( dir[2] );

	if( dir[0] >= adir[1] && dir[0] >= adir[2] )
	{
		axis = 0;
	}
	else if( -dir[0] >= adir[1] && -dir[0] >= adir[2] )
	{
		axis = 1;
	}
	else if( dir[1] >= adir[0] && dir[1] >= adir[2] )
	{
		axis = 2;
	}
	else if( -dir[1] >= adir[0] && -dir[1] >= adir[2] )
	{
		axis = 3;
	}
	else if( dir[2] >= adir[1] && dir[2] >= adir[2] )
	{
		axis = 4;
	}
	else
	{
		axis = 5;
	}

	float	fx = ( dir * tr.cubeAxis[axis][1] ) / ( dir * tr.cubeAxis[axis][0] );
	float	fy = ( dir * tr.cubeAxis[axis][2] ) / ( dir * tr.cubeAxis[axis][0] );

	fx = -fx;
	fy = -fy;
	x = size * 0.5 * ( fx + 1 );
	y = size * 0.5 * ( fy + 1 );
	if( x < 0 )
	{
		x = 0;
	}
	else if( x >= size )
	{
		x = size - 1;
	}
	if( y < 0 )
	{
		y = 0;
	}
	else if( y >= size )
	{
		y = size - 1;
	}

	u = x;
	v = y;

	// unpack RGBA8 to 3 floats
	union
	{
		uint32	i;
		byte	b[4];
	} tmp;

	tmp.b[0] = buffers[axis][( y * size + x ) * 4 + 0];
	tmp.b[1] = buffers[axis][( y * size + x ) * 4 + 1];
	tmp.b[2] = buffers[axis][( y * size + x ) * 4 + 2];
	tmp.b[3] = buffers[axis][( y * size + x ) * 4 + 3];

	//uint32_t value = ( *( const uint32_t* )buffers[axis][( y * size + x ) * 4 + 0] );

	r11g11b10f_to_float3( tmp.i, result );
}

void R_SampleCubeMapHDR16F( const idVec3& dir, int size, halfFloat_t* buffers[6], float result[3], float& u, float& v )
{
	float	adir[3];
	int		axis, x, y;

	adir[0] = fabs( dir[0] );
	adir[1] = fabs( dir[1] );
	adir[2] = fabs( dir[2] );

	if( dir[0] >= adir[1] && dir[0] >= adir[2] )
	{
		axis = 0;
	}
	else if( -dir[0] >= adir[1] && -dir[0] >= adir[2] )
	{
		axis = 1;
	}
	else if( dir[1] >= adir[0] && dir[1] >= adir[2] )
	{
		axis = 2;
	}
	else if( -dir[1] >= adir[0] && -dir[1] >= adir[2] )
	{
		axis = 3;
	}
	else if( dir[2] >= adir[1] && dir[2] >= adir[2] )
	{
		axis = 4;
	}
	else
	{
		axis = 5;
	}

	float	fx = ( dir * tr.cubeAxis[axis][1] ) / ( dir * tr.cubeAxis[axis][0] );
	float	fy = ( dir * tr.cubeAxis[axis][2] ) / ( dir * tr.cubeAxis[axis][0] );

	fx = -fx;
	fy = -fy;
	x = size * 0.5 * ( fx + 1 );
	y = size * 0.5 * ( fy + 1 );
	if( x < 0 )
	{
		x = 0;
	}
	else if( x >= size )
	{
		x = size - 1;
	}
	if( y < 0 )
	{
		y = 0;
	}
	else if( y >= size )
	{
		y = size - 1;
	}

	u = x;
	v = y;

	// unpack RGB16F to 3 floats
	result[0] = F16toF32( buffers[axis][( y * size + x ) * 3 + 0] );
	result[1] = F16toF32( buffers[axis][( y * size + x ) * 3 + 1] );
	result[2] = F16toF32( buffers[axis][( y * size + x ) * 3 + 2] );
}




// http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html

// To implement the Hammersley point set we only need an efficent way to implement the Van der Corput radical inverse phi2(i).
// Since it is in base 2 we can use some basic bit operations to achieve this.
// The brilliant book Hacker's Delight [warren01] provides us a a simple way to reverse the bits in a given 32bit integer. Using this, the following code then implements phi2(i)


// RB: radical inverse implementation from the Mitsuba PBR system

// Van der Corput radical inverse in base 2 with single precision
inline float RadicalInverse_VdC( uint32_t n, uint32_t scramble = 0U )
{
	/* Efficiently reverse the bits in 'n' using binary operations */
#if (defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 2))) || defined(__clang__)
	n = __builtin_bswap32( n );
#else
	n = ( n << 16 ) | ( n >> 16 );
	n = ( ( n & 0x00ff00ff ) << 8 ) | ( ( n & 0xff00ff00 ) >> 8 );
#endif
	n = ( ( n & 0x0f0f0f0f ) << 4 ) | ( ( n & 0xf0f0f0f0 ) >> 4 );
	n = ( ( n & 0x33333333 ) << 2 ) | ( ( n & 0xcccccccc ) >> 2 );
	n = ( ( n & 0x55555555 ) << 1 ) | ( ( n & 0xaaaaaaaa ) >> 1 );

	// Account for the available precision and scramble
	n = ( n >> ( 32 - 24 ) ) ^ ( scramble & ~ -( 1 << 24 ) );

	return ( float ) n / ( float )( 1U << 24 );
}

// The ith point xi is then computed by
inline idVec2 Hammersley2D( uint i, uint N )
{
	return idVec2( float( i ) / float( N ), RadicalInverse_VdC( i ) );
}

idVec3 ImportanceSampleGGX( const idVec2& Xi, const idVec3& N, float roughness )
{
	float a = roughness * roughness;

	// cosinus distributed direction (Z-up or tangent space) from the hammersley point xi
	float Phi = 2 * idMath::PI * Xi.x;
	float cosTheta = idMath::Sqrt( ( 1 - Xi.y ) / ( 1 + ( a * a - 1 ) * Xi.y ) );
	float sinTheta = idMath::Sqrt( 1 - cosTheta * cosTheta );

	idVec3 H;
	H.x = sinTheta * idMath::Cos( Phi );
	H.y = sinTheta * idMath::Sin( Phi );
	H.z = cosTheta;

	// rotate from tangent space to world space along N
	idVec3 upVector = abs( N.z ) < 0.999f ? idVec3( 0, 0, 1 ) : idVec3( 1, 0, 0 );
	idVec3 tangentX = upVector.Cross( N );
	tangentX.Normalize();
	idVec3 tangentY = N.Cross( tangentX );

	idVec3 sampleVec = tangentX * H.x + tangentY * H.y + N * H.z;
	sampleVec.Normalize();

	return sampleVec;
}

float Geometry_SchlickGGX( float NdotV, float roughness )
{
	// note that we use a different k for IBL
	float a = roughness;
	float k = ( a * a ) / 2.0;

	float nom = NdotV;
	float denom = NdotV * ( 1.0 - k ) + k;

	return nom / denom;
}

float Geometry_Smith( idVec3 N, idVec3 V, idVec3 L, float roughness )
{
	float NdotV = Max( ( N * V ), 0.0f );
	float NdotL = Max( ( N * L ), 0.0f );

	float ggx2 = Geometry_SchlickGGX( NdotV, roughness );
	float ggx1 = Geometry_SchlickGGX( NdotL, roughness );

	return ggx1 * ggx2;
}

idVec2 IntegrateBRDF( float NdotV, float roughness, int sampleCount )
{
	idVec3 V;
	V.x = sqrt( 1.0 - NdotV * NdotV );
	V.y = 0.0;
	V.z = NdotV;

	float A = 0.0;
	float B = 0.0;

	idVec3 N( 0.0f, 0.0f, 1.0f );
	for( int i = 0; i < sampleCount; ++i )
	{
		// generates a sample vector that's biased towards the
		// preferred alignment direction (importance sampling).
		idVec2 Xi = Hammersley2D( i, sampleCount );

		idVec3 H = ImportanceSampleGGX( Xi, N, roughness );
		idVec3 L = ( 2.0 * ( V * H ) * H - V );
		L.Normalize();

		float NdotL = Max( L.z, 0.0f );
		float NdotH = Max( H.z, 0.0f );
		float VdotH = Max( ( V * H ), 0.0f );

		if( NdotL > 0.0 )
		{
			float G = Geometry_Smith( N, V, L, roughness );
			float G_Vis = ( G * VdotH ) / ( NdotH * NdotV );
			float Fc = idMath::Pow( 1.0 - VdotH, 5.0 );

			A += ( 1.0 - Fc ) * G_Vis;
			B += Fc * G_Vis;
		}
	}

	A /= float( sampleCount );
	B /= float( sampleCount );

	return idVec2( A, B );
}


// Compute normalized oct coord, mapping top left of top left pixel to (-1,-1)
idVec2 NormalizedOctCoord( int x, int y, const int probeWithBorderSide )
{
#if 0
	// 1 pixel border
	const int margin = 1;

	int probeSideLength = Max( 2, probeWithBorderSide - ( margin * 2 ) );

	idVec2 octFragCoord;
	octFragCoord.x = idMath::ClampInt( 0, probeSideLength - 1, x - margin );
	octFragCoord.y = idMath::ClampInt( 0, probeSideLength - 1, y - margin );

	return ( idVec2( octFragCoord ) ) * ( 2.0f / float( probeSideLength ) ) - idVec2( 1.0f, 1.0f );
#else

	const int margin = 2;

	// RB: FIXME - margin * 2 is wrong but looks better
	// figure out why
	int probeSideLength = Max( 2, probeWithBorderSide - ( margin * 2 ) );

	idVec2 octFragCoord = idVec2( ( x - margin ) % probeWithBorderSide, ( y - margin ) % probeWithBorderSide );

	// Add back the half pixel to get pixel center normalized coordinates
	return ( idVec2( octFragCoord ) + idVec2( 0.5f, 0.5f ) ) * ( 2.0f / float( probeSideLength ) ) - idVec2( 1.0f, 1.0f );

#endif
}

static inline idVec2 NormalizedOctCoordNoBorder( int x, int y, const int probeWithBorderSide )
{
	int probeSideLength = probeWithBorderSide;

	idVec2 octFragCoord = idVec2( x % probeWithBorderSide, y % probeWithBorderSide );

	// Add back the half pixel to get pixel center normalized coordinates
	return ( idVec2( octFragCoord ) + idVec2( 0.5f, 0.5f ) ) * ( 2.0f / float( probeSideLength ) ) - idVec2( 1.0f, 1.0f );
}

/*
static inline float LatLongTexelArea( const idVec2i& pos, const idVec2i& imageSize )
{
	idVec2 uv0;
	uv0.x = pos.x / imageSize.x;
	uv0.y = pos.y / imageSize.y;

	idVec2 uv1;
	uv1.x = ( pos.x + 1 ) / imageSize.x;
	uv1.y = ( pos.y + 1 ) / imageSize.y;

	float theta0 = idMath::PI * ( uv0.x * 2.0f - 1.0f );
	float theta1 = idMath::PI * ( uv1.x * 2.0f - 1.0f );

	float phi0 = idMath::PI * ( uv0.y - 0.5f );
	float phi1 = idMath::PI * ( uv1.y - 0.5f );

	return abs( theta1 - theta0 ) * abs( sin( phi1 ) - sin( phi0 ) );
}


static inline idVec2 CartesianToLatLongTexcoord( const idVec3& p )
{
	// http://gl.ict.usc.edu/Data/HighResProbes

	float u = ( 1.0f + idMath::ATan( p.x, -p.z ) / idMath::PI );
	float v = idMath::ACos( p.y ) / idMath::PI;

	return idVec2( u * 0.5f, v );
}
*/


/// http://www.mpia-hd.mpg.de/~mathar/public/mathar20051002.pdf
/// http://www.rorydriscoll.com/2012/01/15/cubemap-texel-solid-angle/
static inline float AreaElement( float _x, float _y )
{
	return atan2f( _x * _y, sqrtf( _x * _x + _y * _y + 1.0f ) );
}

/// u and v should be center adressing and in [-1.0 + invSize.. 1.0 - invSize] range.
static inline float CubemapTexelSolidAngle( float u, float v, float _invFaceSize )
{
	// Specify texel area.
	const float x0 = u - _invFaceSize;
	const float x1 = u + _invFaceSize;
	const float y0 = v - _invFaceSize;
	const float y1 = v + _invFaceSize;

	// Compute solid angle of texel area.
	const float solidAngle = AreaElement( x1, y1 )
							 - AreaElement( x0, y1 )
							 - AreaElement( x1, y0 )
							 + AreaElement( x0, y0 )
							 ;

	return solidAngle;
}

static inline idVec3 MapXYSToDirection( uint64 x, uint64 y, uint64 s, uint64 width, uint64 height )
{
	float u = ( ( x + 0.5f ) / float( width ) ) * 2.0f - 1.0f;
	float v = ( ( y + 0.5f ) / float( height ) ) * 2.0f - 1.0f;
	v *= -1.0f;

	idVec3 dir( 0, 0, 0 );

	// +x, -x, +y, -y, +z, -z
	switch( s )
	{
		case 0:
			dir = idVec3( 1.0f, v, -u );
			break;
		case 1:
			dir = idVec3( -1.0f, v, u );
			break;
		case 2:
			dir = idVec3( u, 1.0f, -v );
			break;
		case 3:
			dir = idVec3( u, -1.0f, v );
			break;
		case 4:
			dir = idVec3( u, v, 1.0f );
			break;
		case 5:
			dir = idVec3( -u, v, -1.0f );
			break;
	}

	dir.Normalize();

	return dir;
}

void CalculateIrradianceJob( calcEnvprobeParms_t* parms )
{
	halfFloat_t*		buffers[6];

	int	start = Sys_Milliseconds();

	for( int i = 0; i < 6; i++ )
	{
		buffers[ i ] = ( halfFloat_t* ) parms->radiance[ i ];
	}

	const float invDstSize = 1.0f / float( ENVPROBE_CAPTURE_SIZE );
	const idVec2i sourceImageSize( ENVPROBE_CAPTURE_SIZE, ENVPROBE_CAPTURE_SIZE );

	CommandlineProgressBar progressBar( R_CalculateUsedAtlasPixels( parms->outHeight ), parms->printWidth, parms->printHeight );
	if( parms->printProgress )
	{
		progressBar.Start();
	}

	// build L4 Spherical Harmonics from source image
	SphericalHarmonicsT<idVec3, 4> shRadiance;

	for( int i = 0; i < shSize( 4 ); i++ )
	{
		shRadiance[i].Zero();
	}

	// build SH by iterating over all cubemap pixels

	idVec4 dstRect = R_CalculateMipRect( parms->outHeight, 0 );

	for( int side = 0; side < 6; side++ )
	{
		for( int x = 0; x < sourceImageSize.x; x++ )
		{
			for( int y = 0; y < sourceImageSize.y; y++ )
			{
				// convert UV coord to 3D direction
				idVec3 dir = MapXYSToDirection( x, y, side, sourceImageSize.x, sourceImageSize.y );

				float u, v;
				idVec3 radiance;
				R_SampleCubeMapHDR16F( dir, ENVPROBE_CAPTURE_SIZE, buffers, &radiance[0], u, v );

				//radiance = dir * 0.5 + idVec3( 0.5f, 0.5f, 0.5f );

				// convert from [0 .. size-1] to [-1.0 + invSize .. 1.0 - invSize]
				const float uu = 2.0f * ( u * invDstSize ) - 1.0f;
				const float vv = 2.0f * ( v * invDstSize ) - 1.0f;

				float texelArea = CubemapTexelSolidAngle( uu, vv, invDstSize );

				const SphericalHarmonicsT<float, 4>& sh = shEvaluate<4>( dir );

				bool shValid = true;
				for( int i = 0; i < shSize( 4 ); i++ )
				{
					if( IsNAN( sh[i] ) )
					{
						shValid = false;
						break;
					}
				}

				if( shValid )
				{
					shAddWeighted( shRadiance, sh, radiance * texelArea );
				}
			}
		}
	}

	// reset image to black
	for( int x = 0; x < parms->outWidth; x++ )
	{
		for( int y = 0; y < parms->outHeight; y++ )
		{
			parms->outBuffer[( y * parms->outWidth + x ) * 3 + 0] = F32toF16( 0 );
			parms->outBuffer[( y * parms->outWidth + x ) * 3 + 1] = F32toF16( 0 );
			parms->outBuffer[( y * parms->outWidth + x ) * 3 + 2] = F32toF16( 0 );
		}
	}

	const int numMips = idMath::BitsForInteger( parms->outHeight );

	for( int mip = 0; mip < numMips; mip++ )
	{
		idVec4 dstRect = R_CalculateMipRect( parms->outHeight, mip );

		for( int x = dstRect.x; x < ( dstRect.x + dstRect.z ); x++ )
		{
			for( int y = dstRect.y; y < ( dstRect.y + dstRect.w ); y++ )
			{
				idVec2 octCoord;
				if( mip > 0 )
				{
					// move back to [0, 1] coords
					octCoord = NormalizedOctCoordNoBorder( x - dstRect.x, y - dstRect.y, dstRect.z );
				}
				else
				{
					octCoord = NormalizedOctCoordNoBorder( x, y, dstRect.z );
				}

				// convert UV coord to 3D direction
				idVec3 dir;

				dir.FromOctahedral( octCoord );

				idVec3 outColor( 0, 0, 0 );

#if 1
				// generate ambient colors by evaluating the L4 Spherical Harmonics
				SphericalHarmonicsT<float, 4> shDirection = shEvaluate<4>( dir );

				idVec3 sampleIrradianceSh = shEvaluateDiffuse<idVec3, 4>( shRadiance, dir ) / idMath::PI;

				outColor[0] = Max( 0.0f, sampleIrradianceSh.x );
				outColor[1] = Max( 0.0f, sampleIrradianceSh.y );
				outColor[2] = Max( 0.0f, sampleIrradianceSh.z );
#else
				// generate ambient colors using Monte Carlo method
				for( int s = 0; s < parms->samples; s++ )
				{
					idVec2 Xi = Hammersley2D( s, parms->samples );
					idVec3 H = ImportanceSampleGGX( Xi, dir, 0.95f );

					float u, v;
					idVec3 radiance;
					R_SampleCubeMapHDR( H, parms->outHeight, buffers, &radiance[0], u, v );

					outColor[0] += radiance[0];
					outColor[1] += radiance[1];
					outColor[2] += radiance[2];
				}

				outColor[0] /= parms->samples;
				outColor[1] /= parms->samples;
				outColor[2] /= parms->samples;
#endif

				//outColor = dir * 0.5 + idVec3( 0.5f, 0.5f, 0.5f );

				parms->outBuffer[( y * parms->outWidth + x ) * 3 + 0] = F32toF16( outColor[0] );
				parms->outBuffer[( y * parms->outWidth + x ) * 3 + 1] = F32toF16( outColor[1] );
				parms->outBuffer[( y * parms->outWidth + x ) * 3 + 2] = F32toF16( outColor[2] );

				if( parms->printProgress )
				{
					progressBar.Increment( true );
				}
			}
		}
	}

	int	end = Sys_Milliseconds();

	parms->time = end - start;
}

void CalculateRadianceJob( calcEnvprobeParms_t* parms )
{
	halfFloat_t*		buffers[6];

	int	start = Sys_Milliseconds();

	for( int i = 0; i < 6; i++ )
	{
		buffers[ i ] = ( halfFloat_t* ) parms->radiance[ i ];
	}

	const float invDstSize = 1.0f / float( parms->outHeight );

	const int numMips = idMath::BitsForInteger( parms->outHeight );
	const int numOctahedronMips = numMips - 3; // the last 3 mips are too low quality for filtering

	CommandlineProgressBar progressBar( R_CalculateUsedAtlasPixels( parms->outHeight ), parms->printWidth, parms->printHeight );
	if( parms->printProgress )
	{
		progressBar.Start();
	}

	// reset output image to black
	for( int x = 0; x < parms->outWidth; x++ )
	{
		for( int y = 0; y < parms->outHeight; y++ )
		{
			parms->outBuffer[( y * parms->outWidth + x ) * 3 + 0] = F32toF16( 0 );
			parms->outBuffer[( y * parms->outWidth + x ) * 3 + 1] = F32toF16( 0 );
			parms->outBuffer[( y * parms->outWidth + x ) * 3 + 2] = F32toF16( 0 );
		}
	}

	for( int mip = 0; mip < numOctahedronMips; mip++ )
	{
		float roughness = ( float )mip / ( float )( numOctahedronMips - 1 );

		idVec4 dstRect = R_CalculateMipRect( parms->outHeight, mip );

		for( int x = dstRect.x; x < ( dstRect.x + dstRect.z ); x++ )
		{
			for( int y = dstRect.y; y < ( dstRect.y + dstRect.w ); y++ )
			{
				idVec2 octCoord;
				if( mip > 0 )
				{
					// move back to [0, 1] coords
					octCoord = NormalizedOctCoordNoBorder( x - dstRect.x, y - dstRect.y, dstRect.z );
				}
				else
				{
					octCoord = NormalizedOctCoordNoBorder( x, y, dstRect.z );
				}

				// convert UV coord to 3D direction
				idVec3 N;

				N.FromOctahedral( octCoord );

				idVec3 outColor( 0, 0, 0 );

				// RB: Split Sum approximation explanation

				// Epic Games makes a further approximation by assuming the view direction
				// (and thus the specular reflection direction) to be equal to the output sample direction ωo.
				// This translates itself to the following code:
				const idVec3 R = N;
				const idVec3 V = R;

				float totalWeight = 0.0f;

				for( int s = 0; s < parms->samples; s++ )
				{
					idVec2 Xi = Hammersley2D( s, parms->samples );
					idVec3 H = ImportanceSampleGGX( Xi, N, roughness );
					idVec3 L = ( 2.0 * ( H * ( V * H ) ) - V );

					float NdotL = Max( ( N * L ), 0.0f );
					if( NdotL > 0.0 )
					{
						float sample[3];
						float u, v;

						R_SampleCubeMapHDR16F( H, ENVPROBE_CAPTURE_SIZE, buffers, sample, u, v );

						outColor[0] += sample[0] * NdotL;
						outColor[1] += sample[1] * NdotL;
						outColor[2] += sample[2] * NdotL;

						totalWeight += NdotL;
					}
				}

				outColor[0] /= totalWeight;
				outColor[1] /= totalWeight;
				outColor[2] /= totalWeight;

				parms->outBuffer[( y * parms->outWidth + x ) * 3 + 0] = F32toF16( outColor[0] );
				parms->outBuffer[( y * parms->outWidth + x ) * 3 + 1] = F32toF16( outColor[1] );
				parms->outBuffer[( y * parms->outWidth + x ) * 3 + 2] = F32toF16( outColor[2] );

				if( parms->printProgress )
				{
					progressBar.Increment( true );
				}
			}
		}
	}

	int	end = Sys_Milliseconds();

	parms->time = end - start;
}

REGISTER_PARALLEL_JOB( CalculateIrradianceJob, "CalculateIrradianceJob" );
REGISTER_PARALLEL_JOB( CalculateRadianceJob, "CalculateRadianceJob" );


void R_MakeAmbientMap( const char* baseName, byte* buffers[6], const char* suffix, int outSize, bool specular, bool useThreads )
{
	idStr		fullname;
	renderView_t	ref;
	viewDef_t	primary;
	//byte*		buffers[6];
	//int			width = 0, height = 0;

	// set up the job
	calcEnvprobeParms_t* jobParms = new calcEnvprobeParms_t;

	for( int i = 0; i < 6; i++ )
	{
		jobParms->radiance[ i ] = buffers[ i ];
	}

	jobParms->freeRadiance = specular ? 1 : 0;

	jobParms->samples = 1000;
	jobParms->filename.Format( "env/%s%s.exr", baseName, suffix );

	jobParms->printProgress = !useThreads;
	jobParms->printWidth = renderSystem->GetWidth();
	jobParms->printHeight = renderSystem->GetHeight();

	jobParms->outWidth = int( outSize * 1.5f );
	jobParms->outHeight = outSize;
	jobParms->outBuffer = ( halfFloat_t* )R_StaticAlloc( idMath::Ceil( outSize * outSize * 3 * sizeof( halfFloat_t ) * 1.5f ), TAG_IMAGE );

	tr.envprobeJobs.Append( jobParms );

	if( useThreads )
	{
		if( specular )
		{
			tr.envprobeJobList->AddJob( ( jobRun_t )CalculateRadianceJob, jobParms );
		}
		else
		{
			tr.envprobeJobList->AddJob( ( jobRun_t )CalculateIrradianceJob, jobParms );
		}
	}
	else
	{
		if( specular )
		{
			CalculateRadianceJob( jobParms );
		}
		else
		{
			CalculateIrradianceJob( jobParms );
		}
	}
}

CONSOLE_COMMAND( bakeEnvironmentProbes, "Bake environment probes", NULL )
{
	idStr			fullname;
	idStr			baseName;
	renderView_t	ref;
	int				captureSize;

	if( !tr.primaryWorld )
	{
		common->Printf( "No primary world loaded.\n" );
		return;
	}

	int sysWidth = renderSystem->GetWidth();
	int sysHeight = renderSystem->GetHeight();

	bool useThreads = true;

	baseName = tr.primaryWorld->mapName;
	baseName.StripFileExtension();

	captureSize = ENVPROBE_CAPTURE_SIZE;

	if( !tr.primaryView )
	{
		common->Printf( "No primary view.\n" );
		return;
	}

	const viewDef_t primary = *tr.primaryView;

	//--------------------------------------------
	// CONVOLVE CUBEMAPS
	//--------------------------------------------

	// make sure the game / draw thread has completed
	commonLocal.WaitGameThread();

	//glConfig.nativeScreenWidth = captureSize;
	//glConfig.nativeScreenHeight = captureSize;

	// disable scissor, so we don't need to adjust all those rects
	r_useScissor.SetBool( false );

	// RB: this really sucks but prevents a crash I couldn't track down
	extern idCVar r_useParallelAddModels;
	extern idCVar r_useParallelAddShadows;
	extern idCVar r_useParallelAddLights;

	r_useParallelAddModels.SetBool( false );
	r_useParallelAddShadows.SetBool( false );
	r_useParallelAddLights.SetBool( false );

	// discard anything currently on the list (this triggers SwapBuffers)
	tr.SwapCommandBuffers( NULL, NULL, NULL, NULL, NULL, NULL );

	tr.takingEnvprobe = true;

	int totalProcessedProbes = 0;
	int	totalStart = Sys_Milliseconds();

	for( int i = 0; i < tr.primaryWorld->envprobeDefs.Num(); i++ )
	{
		RenderEnvprobeLocal* def = tr.primaryWorld->envprobeDefs[i];
		if( def == NULL )
		{
			continue;
		}

		totalProcessedProbes++;
	}

	idLib::Printf( "Deleting old probes...\n" );

	fullname.Format( "env/%s", baseName.c_str() );

	idFileList* files = fileSystem->ListFilesTree( fullname, "envprobe*.exr", true );
	for( int i = 0; i < files->GetNumFiles(); i++ )
	{
		idLib::Printf( "deleting old envprobe data '%s'\n", files->GetFile( i ) );
		fileSystem->RemoveFile( files->GetFile( i ) );
	}
	fileSystem->FreeFileList( files );

	idLib::Printf( "Shooting %i environment probes...\n", totalProcessedProbes );

	CommandlineProgressBar progressBar( totalProcessedProbes, sysWidth, sysHeight );
	progressBar.Start();

	int	start = Sys_Milliseconds();

	for( int i = 0; i < tr.primaryWorld->envprobeDefs.Num(); i++ )
	{
		RenderEnvprobeLocal* def = tr.primaryWorld->envprobeDefs[i];
		if( def == NULL )
		{
			continue;
		}

		byte* buffers[6];

		for( int j = 0; j < 6; j++ )
		{
			ref = primary.renderView;

			ref.rdflags = RDF_NOAMBIENT | RDF_IRRADIANCE;
			ref.fov_x = ref.fov_y = 90;

			ref.vieworg = def->parms.origin;
			ref.viewaxis = tr.cubeAxis[j];

			// discard anything currently on the list
			//tr.SwapCommandBuffers( NULL, NULL, NULL, NULL, NULL, NULL );

			// build commands to render the scene
			tr.primaryWorld->RenderScene( &ref );

			// finish off these commands
			const emptyCommand_t* cmd = tr.SwapCommandBuffers( NULL, NULL, NULL, NULL, NULL, NULL );

			// issue the commands to the GPU
			tr.RenderCommandBuffers( cmd );

			// discard anything currently on the list (this triggers SwapBuffers)
			tr.SwapCommandBuffers( NULL, NULL, NULL, NULL, NULL, NULL );

#if defined( USE_VULKAN )
			// TODO
#elif defined( USE_NVRHI )

			byte* floatRGB16F = NULL;

			R_ReadPixelsRGB16F( deviceManager->GetDevice(), &tr.backend.GetCommonPasses(), globalImages->envprobeHDRImage->GetTextureHandle() , nvrhi::ResourceStates::RenderTarget, &floatRGB16F, captureSize, captureSize );

#if 0
			idStr testName;
			testName.Format( "env/test/envprobe_%i_side_%i.exr", i, j );
			R_WriteEXR( testName, floatRGB16F, 3, captureSize, captureSize, "fs_basepath" );
#endif

#else
			int pix = captureSize * captureSize;
			const int bufferSize = pix * 3 * 2;

			byte* floatRGB16F = ( byte* )R_StaticAlloc( bufferSize );


			glFinish();

			glReadBuffer( GL_BACK );

			globalFramebuffers.envprobeFBO->Bind();

			glPixelStorei( GL_PACK_ROW_LENGTH, ENVPROBE_CAPTURE_SIZE );
			glReadPixels( 0, 0, captureSize, captureSize, GL_RGB, GL_HALF_FLOAT, float16FRGB );

			R_VerticalFlipRGB16F( float16FRGB, captureSize, captureSize );

			Framebuffer::Unbind();
#endif
			buffers[ j ] = floatRGB16F;
		}

		tr.takingEnvprobe = false;
		progressBar.Increment( true );
		tr.takingEnvprobe = true;

		int areaNum = tr.primaryWorld->PointInArea( def->parms.origin );
		idVec3 point = def->parms.origin;
		point.SnapInt();

		fullname.Format( "%s/area%i_envprobe_%i_%i_%i", baseName.c_str(), areaNum, int( point.x ), int( point.y ), int( point.z ) );
		fullname.ReplaceChar( '-', '_' );

		// create 2 jobs
		R_MakeAmbientMap( fullname.c_str(), buffers, "_amb", IRRADIANCE_OCTAHEDRON_SIZE, false, useThreads );
		R_MakeAmbientMap( fullname.c_str(), buffers, "_spec", RADIANCE_OCTAHEDRON_SIZE, true, useThreads );
	}

	int	end = Sys_Milliseconds();

	tr.takingEnvprobe = false;

	r_useScissor.SetBool( true );
	r_useParallelAddModels.SetBool( true );
	r_useParallelAddShadows.SetBool( true );
	r_useParallelAddLights.SetBool( true );

	common->Printf( "captured environment probes %5.1f seconds\n\n", ( end - start ) * 0.001f );

	if( useThreads )
	{
		idLib::Printf( "Processing probes on all available cores... Please wait.\n" );
		common->UpdateScreen( false );
		common->UpdateScreen( false );

		//tr.envprobeJobList->Submit();
		tr.envprobeJobList->Submit( NULL, JOBLIST_PARALLELISM_MAX_CORES );
		tr.envprobeJobList->Wait();
	}

	for( int j = 0; j < tr.envprobeJobs.Num(); j++ )
	{
		calcEnvprobeParms_t* job = tr.envprobeJobs[ j ];

		R_WriteEXR( job->filename, ( byte* )job->outBuffer, 3, job->outWidth, job->outHeight, "fs_basepath" );

		common->Printf( "%s convolved in %5.1f seconds\n\n", job->filename.c_str(), job->time * 0.001f );

		if( job->freeRadiance > 0 )
		{
			for( int i = 0; i < 6; i++ )
			{
				if( job->radiance[i] )
				{
					Mem_Free( job->radiance[i] );
				}
			}
		}

		Mem_Free( job->outBuffer );

		delete job;
	}

	tr.envprobeJobs.Clear();

	int	totalEnd = Sys_Milliseconds();

#if defined( USE_NVRHI )
	nvrhi::CommandListHandle commandList = deviceManager->GetDevice()->createCommandList();

	commandList->open();
#endif

	//--------------------------------------------
	// LOAD CONVOLVED OCTAHEDRONS INTO THE GPU
	//--------------------------------------------
	for( int i = 0; i < tr.primaryWorld->envprobeDefs.Num(); i++ )
	{
		RenderEnvprobeLocal* def = tr.primaryWorld->envprobeDefs[i];
		if( def == NULL )
		{
			continue;
		}

		def->irradianceImage->Reload( true, commandList );
		def->radianceImage->Reload( true, commandList );
	}

#if defined( USE_NVRHI )
	commandList->close();
	deviceManager->GetDevice()->executeCommandList( commandList );
#endif

	idLib::Printf( "----------------------------------\n" );
	idLib::Printf( "Processed %i light probes\n", totalProcessedProbes );
	common->Printf( "Baked SH irradiance and GGX mip maps in %5.1f seconds\n\n", ( totalEnd - totalStart ) / ( 1000.0f ) );
}

CONSOLE_COMMAND( makeBrdfLUT, "make a GGX BRDF lookup table", NULL )
{
	int			outSize = 256;
	int			width = 0, height = 0;

	//if( args.Argc() != 2 )
	//{
	//	common->Printf( "USAGE: makeBrdfLut [size]\n" );
	//	return;
	//}

	//if( args.Argc() == 2 )
	//{
	//	outSize = atoi( args.Argv( 1 ) );
	//}

	// resample with hemispherical blending
	int	samples = 1024;

	int ldrBufferSize = outSize * outSize * 4;
	byte* ldrBuffer = ( byte* )Mem_Alloc( ldrBufferSize, TAG_TEMP );

	int hdrBufferSize = outSize * outSize * 2 * sizeof( halfFloat_t );
	halfFloat_t* hdrBuffer = ( halfFloat_t* )Mem_Alloc( hdrBufferSize, TAG_TEMP );

	int sysWidth = renderSystem->GetWidth();
	int sysHeight = renderSystem->GetHeight();

	CommandlineProgressBar progressBar( outSize * outSize, sysWidth, sysHeight );

	int	start = Sys_Milliseconds();

	for( int x = 0 ; x < outSize ; x++ )
	{
		float NdotV = ( x + 0.5f ) / outSize;

		for( int y = 0 ; y < outSize ; y++ )
		{
			float roughness = ( y + 0.5f ) / outSize;

			idVec2 output = IntegrateBRDF( NdotV, roughness, samples );

			ldrBuffer[( y * outSize + x ) * 4 + 0] = byte( output.x * 255 );
			ldrBuffer[( y * outSize + x ) * 4 + 1] = byte( output.y * 255 );
			ldrBuffer[( y * outSize + x ) * 4 + 2] = 0;
			ldrBuffer[( y * outSize + x ) * 4 + 3] = 255;

			halfFloat_t half1 = F32toF16( output.x );
			halfFloat_t half2 = F32toF16( output.y );

			hdrBuffer[( y * outSize + x ) * 2 + 0] = half1;
			hdrBuffer[( y * outSize + x ) * 2 + 1] = half2;
			//hdrBuffer[( y * outSize + x ) * 4 + 2] = 0;
			//hdrBuffer[( y * outSize + x ) * 4 + 3] = 1;

			progressBar.Increment( true );
		}
	}

	idStr fullname = "env/_brdfLut.png";
	idLib::Printf( "writing %s\n", fullname.c_str() );

	R_WritePNG( fullname, ldrBuffer, 4, outSize, outSize, true, "fs_basepath" );
	//R_WriteEXR( "env/_brdfLut.exr", hdrBuffer, 4, outSize, outSize, "fs_basepath" );


	idFileLocal headerFile( fileSystem->OpenFileWrite( "env/Image_brdfLut.h", "fs_basepath" ) );

	static const char* intro = R"(
#ifndef BRDFLUT_TEX_H
#define BRDFLUT_TEX_H

#define BRDFLUT_TEX_WIDTH 256
#define BRDFLUT_TEX_HEIGHT 256
#define BRDFLUT_TEX_PITCH (BRDFLUT_TEX_WIDTH * 2)
#define BRDFLUT_TEX_SIZE (BRDFLUT_TEX_WIDTH * BRDFLUT_TEX_PITCH)

// Stored in R16G16F format
static const unsigned char brfLutTexBytes[] =
{
)";

	headerFile->Printf( "%s\n", intro );

	const byte* hdrBytes = (const byte* ) hdrBuffer;
	for( int i = 0; i < hdrBufferSize; i++ )
	{
		byte b = hdrBytes[i];

		if( i < ( hdrBufferSize - 1 ) )
		{
			headerFile->Printf( "0x%02hhx, ", b );
		}
		else
		{
			headerFile->Printf( "0x%02hhx", b );
		}

		if( i % 12 == 0 )
		{
			headerFile->Printf( "\n" );
		}
	}
	headerFile->Printf( "\n};\n#endif\n" );

	int	end = Sys_Milliseconds();

	common->Printf( "%s integrated in %5.1f seconds\n\n", fullname.c_str(), ( end - start ) * 0.001f );

	Mem_Free( ldrBuffer );
	Mem_Free( hdrBuffer );
}

CONSOLE_COMMAND( makeImageHeader, "load an image and turn it into a .h file", NULL )
{
	byte*		buffer;
	int			width = 0, height = 0;

	if( args.Argc() < 2 )
	{
		common->Printf( "USAGE: makeImageHeader filename [exportname]\n" );
		return;
	}

	idStr filename = args.Argv( 1 );

	R_LoadImage( filename, &buffer, &width, &height, NULL, true, NULL );
	if( !buffer )
	{
		common->Printf( "loading %s failed.\n", filename.c_str() );
		return;
	}

	filename.StripFileExtension();

	idStr exportname;

	if( args.Argc() == 3 )
	{
		exportname.Format( "Image_%s.h", args.Argv( 2 ) );
	}
	else
	{
		exportname.Format( "Image_%s.h", filename.c_str() );
	}

	for( int i = 0; i < exportname.Length(); i++ )
	{
		if( exportname[ i ] == '/' )
		{
			exportname[ i ] = '_';
		}
	}

	idFileLocal headerFile( fileSystem->OpenFileWrite( exportname, "fs_basepath" ) );

	idStr uppername = exportname;
	uppername.ToUpper();

	for( int i = 0; i < uppername.Length(); i++ )
	{
		if( uppername[ i ] == '.' )
		{
			uppername[ i ] = '_';
		}
	}

	headerFile->Printf( "#ifndef %s_TEX_H\n", uppername.c_str() );
	headerFile->Printf( "#define %s_TEX_H\n\n", uppername.c_str() );

	headerFile->Printf( "#define %s_TEX_WIDTH %i\n", uppername.c_str(), width );
	headerFile->Printf( "#define %s_TEX_HEIGHT %i\n\n", uppername.c_str(), height );

	headerFile->Printf( "static const unsigned char %s_Bytes[] = {\n", uppername.c_str() );

	int bufferSize = width * height * 4;

	for( int i = 0; i < bufferSize; i++ )
	{
		byte b = buffer[i];

		if( i < ( bufferSize - 1 ) )
		{
			headerFile->Printf( "0x%02hhx, ", b );
		}
		else
		{
			headerFile->Printf( "0x%02hhx", b );
		}

		if( i % 12 == 0 )
		{
			headerFile->Printf( "\n" );
		}
	}
	headerFile->Printf( "\n};\n#endif\n" );

	Mem_Free( buffer );
}
