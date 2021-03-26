/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2020-2021 Robert Beckebans

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

#pragma hdrstop
#include "precompiled.h"

#include "../libs/mesa/format_r11g11b10f.h"

#include "RenderCommon.h"

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
	vProbe->inverseBaseLightProject = probe->inverseBaseLightProject;

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
		idRenderMatrix::GetFrustumCorners( corners, probe->inverseBaseLightProject, bounds_zeroOneCube );
		for( int i = 0; i < ps->numPortalPlanes; i++ )
		{
			if( idRenderMatrix::CullFrustumCornersToPlane( corners, ps->portalPlanes[i] ) == FRUSTUM_CULL_FRONT )
			{
				return true;
			}
		}

	}
	else if( r_useLightPortalCulling.GetInteger() >= 2 )
	{

		idPlane frustumPlanes[6];
		idRenderMatrix::GetFrustumPlanes( frustumPlanes, probe->baseLightProject, true, true );

		// exact clip of light faces against all planes
		for( int i = 0; i < 6; i++ )
		{
			// the light frustum planes face inward, so the planes that have the
			// view origin on the positive side will be the "back" faces of the light,
			// which must have some fragment inside the the portal stack planes to be visible
			if( frustumPlanes[i].Distance( tr.viewDef->renderView.vieworg ) <= 0.0f )
			{
				continue;
			}

			// calculate a winding for this frustum side
			idFixedWinding w;
			w.BaseForPlane( frustumPlanes[i] );
			for( int j = 0; j < 6; j++ )
			{
				if( j == i )
				{
					continue;
				}
				if( !w.ClipInPlace( frustumPlanes[j], ON_EPSILON ) )
				{
					break;
				}
			}
			if( w.GetNumPoints() <= 2 )
			{
				continue;
			}

			assert( ps->numPortalPlanes <= MAX_PORTAL_PLANES );
			assert( w.GetNumPoints() + ps->numPortalPlanes < MAX_POINTS_ON_WINDING );

			// now clip the winding against each of the portalStack planes
			// skip the last plane which is the last portal itself
			for( int j = 0; j < ps->numPortalPlanes - 1; j++ )
			{
				if( !w.ClipInPlace( -ps->portalPlanes[j], ON_EPSILON ) )
				{
					break;
				}
			}

			if( w.GetNumPoints() > 2 )
			{
				// part of the winding is visible through the portalStack,
				// so the light is not culled
				return false;
			}
		}

		// nothing was visible
		return true;
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
static idMat3		cubeAxis[6];
static const char* envDirection[6] = { "_px", "_nx", "_py", "_ny", "_pz", "_nz" };

void R_SampleCubeMapHDR( const idVec3& dir, int size, byte* buffers[6], float result[3] )
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

	float	fx = ( dir * cubeAxis[axis][1] ) / ( dir * cubeAxis[axis][0] );
	float	fy = ( dir * cubeAxis[axis][2] ) / ( dir * cubeAxis[axis][0] );

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

class CommandlineProgressBar
{
private:
	size_t tics = 0;
	size_t nextTicCount = 0;
	int	count = 0;
	int expectedCount = 0;

public:
	CommandlineProgressBar( int _expectedCount )
	{
		expectedCount = _expectedCount;

		common->Printf( "0%%  10   20   30   40   50   60   70   80   90   100%%\n" );
		common->Printf( "|----|----|----|----|----|----|----|----|----|----|\n" );

		common->UpdateScreen( false );
	}

	void Increment()
	{
		if( ( count + 1 ) >= nextTicCount )
		{
			size_t ticsNeeded = ( size_t )( ( ( double )( count + 1 ) / expectedCount ) * 50.0 );

			do
			{
				common->Printf( "*" );
			}
			while( ++tics < ticsNeeded );

			nextTicCount = ( size_t )( ( tics / 50.0 ) * expectedCount );
			if( count == ( expectedCount - 1 ) )
			{
				if( tics < 51 )
				{
					common->Printf( "*" );
				}
				common->Printf( "\n" );
			}

			common->UpdateScreen( false );
		}

		count++;
	}
};


// http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html

// To implement the Hammersley point set we only need an efficent way to implement the Van der Corput radical inverse phi2(i).
// Since it is in base 2 we can use some basic bit operations to achieve this.
// The brilliant book Hacker's Delight [warren01] provides us a a simple way to reverse the bits in a given 32bit integer. Using this, the following code then implements phi2(i)

/*
GLSL version

float radicalInverse_VdC( uint bits )
{
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}
*/

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
idVec2 NormalizedOctCoord( int x, int y, const int probeSideLength )
{
	const int margin = 0;

	int probeWithBorderSide = probeSideLength + margin;

	idVec2 octFragCoord = idVec2( ( x - margin ) % probeWithBorderSide, ( y - margin ) % probeWithBorderSide );

	// Add back the half pixel to get pixel center normalized coordinates
	return ( idVec2( octFragCoord ) + idVec2( 0.5f, 0.5f ) ) * ( 2.0f / float( probeSideLength ) ) - idVec2( 1.0f, 1.0f );
}




/*
==================
R_MakeAmbientMap_f

R_MakeAmbientMap_f <basename> [size]

Saves out env/<basename>_amb_ft.tga, etc
==================
*/
void R_MakeAmbientMap( const char* baseName, const char* suffix, int outSize, bool specular, bool deleteTempFiles )
{
	idStr		fullname;
	renderView_t	ref;
	viewDef_t	primary;
	byte*		buffers[6];
	int			width = 0, height = 0;

	// read all of the images
	for( int i = 0 ; i < 6 ; i++ )
	{
		fullname.Format( "env/%s%s.exr", baseName, envDirection[i] );

		const bool captureToImage = false;
		common->UpdateScreen( captureToImage );

		R_LoadImage( fullname, &buffers[i], &width, &height, NULL, true, NULL );
		if( !buffers[i] )
		{
			common->Printf( "loading %s failed.\n", fullname.c_str() );
			for( i-- ; i >= 0 ; i-- )
			{
				Mem_Free( buffers[i] );
			}
			return;
		}
	}

	// resample with hemispherical blending
	int	samples = 1000;

	int outWidth = int( outSize * 1.5f );
	int outHeight = outSize;

	//halfFloat_t* outBuffer = ( halfFloat_t* )_alloca( outSize * outSize * 3 * sizeof( halfFloat_t ) );
	halfFloat_t* outBuffer = ( halfFloat_t* )R_StaticAlloc( idMath::Ceil( outSize * outSize * 3 * sizeof( halfFloat_t ) * 1.5f ), TAG_IMAGE );

	{
		// output an octahedron probe

		CommandlineProgressBar progressBar( R_CalculateUsedAtlasPixels( outSize ) );

		int	start = Sys_Milliseconds();

		const float invDstSize = 1.0f / float( outSize );

		const int numMips = idMath::BitsForInteger( outSize );

		// reset image to black
		for( int x = 0; x < outWidth; x++ )
		{
			for( int y = 0; y < outHeight; y++ )
			{
				outBuffer[( y * outWidth + x ) * 3 + 0] = F32toF16( 0 );
				outBuffer[( y * outWidth + x ) * 3 + 1] = F32toF16( 0 );
				outBuffer[( y * outWidth + x ) * 3 + 2] = F32toF16( 0 );
			}
		}

		for( int mip = 0; mip < numMips; mip++ )
		{
			float roughness = ( float )mip / ( float )( numMips - 1 );

			idVec4 dstRect = R_CalculateMipRect( outSize, mip );

			for( int x = dstRect.x; x < ( dstRect.x + dstRect.z ); x++ )
			{
				for( int y = dstRect.y; y < ( dstRect.y + dstRect.w ); y++ )
				{
					idVec2 octCoord;
					if( mip > 0 )
					{
						// move back to [0, 1] coords
						octCoord = NormalizedOctCoord( x - dstRect.x, y - dstRect.y, dstRect.z );
					}
					else
					{
						octCoord = NormalizedOctCoord( x, y, dstRect.z );
					}

					// convert UV coord to 3D direction
					idVec3 N;

					N.FromOctahedral( octCoord );

#if 1
					// RB: Split Sum approximation explanation

					// Epic Games makes a further approximation by assuming the view direction
					// (and thus the specular reflection direction) to be equal to the output sample direction ωo.
					// This translates itself to the following code:
					const idVec3 R = N;
					const idVec3 V = R;

					idVec3 prefilteredColor( 0, 0, 0 );

					if( specular )
					{
						float totalWeight = 0.0f;

						for( int s = 0; s < samples; s++ )
						{
							idVec2 Xi = Hammersley2D( s, samples );
							idVec3 H = ImportanceSampleGGX( Xi, N, roughness );
							idVec3 L = ( 2.0 * ( H * ( V * H ) ) - V );

							float NdotL = Max( ( N * L ), 0.0f );
							if( NdotL > 0.0 )
							{
								float sample[3];

								R_SampleCubeMapHDR( H, width, buffers, sample );

								prefilteredColor[0] += sample[0] * NdotL;
								prefilteredColor[1] += sample[1] * NdotL;
								prefilteredColor[2] += sample[2] * NdotL;

								totalWeight += NdotL;
							}
						}

						prefilteredColor[0] /= totalWeight;
						prefilteredColor[1] /= totalWeight;
						prefilteredColor[2] /= totalWeight;
					}
					else
					{
						for( int s = 0; s < samples; s++ )
						{
							idVec2 Xi = Hammersley2D( s, samples );
							idVec3 H = ImportanceSampleGGX( Xi, N, 0.95f );

							float sample[3];

							R_SampleCubeMapHDR( H, width, buffers, sample );

							prefilteredColor[0] += sample[0];
							prefilteredColor[1] += sample[1];
							prefilteredColor[2] += sample[2];
						}

						prefilteredColor[0] /= samples;
						prefilteredColor[1] /= samples;
						prefilteredColor[2] /= samples;
					}

					outBuffer[( y * outWidth + x ) * 3 + 0] = F32toF16( prefilteredColor[0] );
					outBuffer[( y * outWidth + x ) * 3 + 1] = F32toF16( prefilteredColor[1] );
					outBuffer[( y * outWidth + x ) * 3 + 2] = F32toF16( prefilteredColor[2] );
#else
					outBuffer[( y * outWidth + x ) * 3 + 0] = F32toF16( ( N.x * 0.5f + 0.5f ) );
					outBuffer[( y * outWidth + x ) * 3 + 1] = F32toF16( ( N.y * 0.5f + 0.5f ) );
					outBuffer[( y * outWidth + x ) * 3 + 2] = F32toF16( ( N.z * 0.5f + 0.5f ) );
#endif

					progressBar.Increment();
				}
			}
		}


		fullname.Format( "env/%s%s.exr", baseName, suffix );
		//common->Printf( "writing %s\n", fullname.c_str() );

		const bool captureToImage = false;
		common->UpdateScreen( captureToImage );

		//R_WriteTGA( fullname, outBuffer, outSize, outSize, false, "fs_basepath" );
		//R_WritePNG( fullname, outBuffer, 4, outSize, outSize, true, "fs_basepath" );
		R_WriteEXR( fullname, ( byte* )outBuffer, 3, outWidth, outHeight, "fs_basepath" );

		int	end = Sys_Milliseconds();

		common->Printf( "env/%s convolved in %5.1f seconds\n\n", baseName, ( end - start ) * 0.001f );
	}

	for( int i = 0 ; i < 6 ; i++ )
	{
		if( buffers[i] )
		{
			Mem_Free( buffers[i] );
		}
	}

	Mem_Free( outBuffer );

	if( deleteTempFiles )
	{
		for( int i = 0 ; i < 6 ; i++ )
		{
			fullname.Format( "env/%s%s.exr", baseName, envDirection[i] );

			fileSystem->RemoveFile( fullname );
		}
	}
}




void CalculateIrradianceJob( calcEnvprobeParms_t* parms )
{
	byte*		buffers[6];

	int	start = Sys_Milliseconds();

	for( int i = 0; i < 6; i++ )
	{
		buffers[ i ] = parms->buffers[ i ];
	}

	const float invDstSize = 1.0f / float( parms->outHeight );

	const int numMips = idMath::BitsForInteger( parms->outHeight );

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

	for( int mip = 0; mip < numMips; mip++ )
	{
		float roughness = ( float )mip / ( float )( numMips - 1 );

		idVec4 dstRect = R_CalculateMipRect( parms->outHeight, mip );

		for( int x = dstRect.x; x < ( dstRect.x + dstRect.z ); x++ )
		{
			for( int y = dstRect.y; y < ( dstRect.y + dstRect.w ); y++ )
			{
				idVec2 octCoord;
				if( mip > 0 )
				{
					// move back to [0, 1] coords
					octCoord = NormalizedOctCoord( x - dstRect.x, y - dstRect.y, dstRect.z );
				}
				else
				{
					octCoord = NormalizedOctCoord( x, y, dstRect.z );
				}

				// convert UV coord to 3D direction
				idVec3 N;

				N.FromOctahedral( octCoord );

				idVec3 outColor( 0, 0, 0 );

				for( int s = 0; s < parms->samples; s++ )
				{
					idVec2 Xi = Hammersley2D( s, parms->samples );
					idVec3 H = ImportanceSampleGGX( Xi, N, 0.95f );

					float sample[3];

					R_SampleCubeMapHDR( H, parms->outHeight, buffers, sample );

					outColor[0] += sample[0];
					outColor[1] += sample[1];
					outColor[2] += sample[2];
				}

				outColor[0] /= parms->samples;
				outColor[1] /= parms->samples;
				outColor[2] /= parms->samples;

				parms->outBuffer[( y * parms->outWidth + x ) * 3 + 0] = F32toF16( outColor[0] );
				parms->outBuffer[( y * parms->outWidth + x ) * 3 + 1] = F32toF16( outColor[1] );
				parms->outBuffer[( y * parms->outWidth + x ) * 3 + 2] = F32toF16( outColor[2] );
			}
		}
	}

	int	end = Sys_Milliseconds();

	parms->time = end - start;
}

void CalculateRadianceJob( calcEnvprobeParms_t* parms )
{
	byte*		buffers[6];

	int	start = Sys_Milliseconds();

	for( int i = 0; i < 6; i++ )
	{
		buffers[ i ] = parms->buffers[ i ];
	}

	const float invDstSize = 1.0f / float( parms->outHeight );

	const int numMips = idMath::BitsForInteger( parms->outHeight );

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

	for( int mip = 0; mip < numMips; mip++ )
	{
		float roughness = ( float )mip / ( float )( numMips - 1 );

		idVec4 dstRect = R_CalculateMipRect( parms->outHeight, mip );

		for( int x = dstRect.x; x < ( dstRect.x + dstRect.z ); x++ )
		{
			for( int y = dstRect.y; y < ( dstRect.y + dstRect.w ); y++ )
			{
				idVec2 octCoord;
				if( mip > 0 )
				{
					// move back to [0, 1] coords
					octCoord = NormalizedOctCoord( x - dstRect.x, y - dstRect.y, dstRect.z );
				}
				else
				{
					octCoord = NormalizedOctCoord( x, y, dstRect.z );
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

						R_SampleCubeMapHDR( H, parms->outHeight, buffers, sample );

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
			}
		}
	}

	int	end = Sys_Milliseconds();

	parms->time = end - start;
}

REGISTER_PARALLEL_JOB( CalculateIrradianceJob, "CalculateIrradianceJob" );
REGISTER_PARALLEL_JOB( CalculateRadianceJob, "CalculateRadianceJob" );


void R_MakeAmbientMapThreaded( const char* baseName, const char* suffix, int outSize, bool specular, bool deleteTempFiles )
{
	idStr		fullname;
	renderView_t	ref;
	viewDef_t	primary;
	byte*		buffers[6];
	int			width = 0, height = 0;

	// read all of the images
	for( int i = 0 ; i < 6 ; i++ )
	{
		fullname.Format( "env/%s%s.exr", baseName, envDirection[i] );

		const bool captureToImage = false;
		common->UpdateScreen( captureToImage );

		R_LoadImage( fullname, &buffers[i], &width, &height, NULL, true, NULL );
		if( !buffers[i] )
		{
			common->Printf( "loading %s failed.\n", fullname.c_str() );
			for( i-- ; i >= 0 ; i-- )
			{
				Mem_Free( buffers[i] );
			}
			return;
		}
	}

	// set up the job
	calcEnvprobeParms_t* jobParms = new calcEnvprobeParms_t;

	for( int i = 0; i < 6; i++ )
	{
		jobParms->buffers[ i ] = buffers[ i ];
	}

	jobParms->samples = 1000;
	jobParms->filename.Format( "env/%s%s.exr", baseName, suffix );

	jobParms->outWidth = int( outSize * 1.5f );
	jobParms->outHeight = outSize;
	jobParms->outBuffer = ( halfFloat_t* )R_StaticAlloc( idMath::Ceil( outSize * outSize * 3 * sizeof( halfFloat_t ) * 1.5f ), TAG_IMAGE );

	tr.irradianceJobs.Append( jobParms );

	if( specular )
	{
		tr.envprobeJobList->AddJob( ( jobRun_t )CalculateRadianceJob, jobParms );
	}
	else
	{
		tr.envprobeJobList->AddJob( ( jobRun_t )CalculateIrradianceJob, jobParms );
	}

	if( deleteTempFiles )
	{
		for( int i = 0 ; i < 6 ; i++ )
		{
			fullname.Format( "env/%s%s.exr", baseName, envDirection[i] );

			fileSystem->RemoveFile( fullname );
		}
	}
}

CONSOLE_COMMAND( generateEnvironmentProbes, "Generate environment probes", NULL )
{
	idStr			fullname;
	idStr			baseName;
	renderView_t	ref;
	int				blends;
	const char*		extension;
	int				size;

	static const char* envDirection[6] = { "_px", "_nx", "_py", "_ny", "_pz", "_nz" };

	if( !tr.primaryWorld )
	{
		common->Printf( "No primary world loaded.\n" );
		return;
	}

	bool useThreads = true;

	baseName = tr.primaryWorld->mapName;
	baseName.StripFileExtension();

	size = RADIANCE_CUBEMAP_SIZE;
	blends = 1;

	if( !tr.primaryView )
	{
		common->Printf( "No primary view.\n" );
		return;
	}

	const viewDef_t primary = *tr.primaryView;

	memset( &cubeAxis, 0, sizeof( cubeAxis ) );

	// +X
	cubeAxis[0][0][0] = 1;
	cubeAxis[0][1][2] = 1;
	cubeAxis[0][2][1] = 1;

	// -X
	cubeAxis[1][0][0] = -1;
	cubeAxis[1][1][2] = -1;
	cubeAxis[1][2][1] = 1;

	// +Y
	cubeAxis[2][0][1] = 1;
	cubeAxis[2][1][0] = -1;
	cubeAxis[2][2][2] = -1;

	// -Y
	cubeAxis[3][0][1] = -1;
	cubeAxis[3][1][0] = -1;
	cubeAxis[3][2][2] = 1;

	// +Z
	cubeAxis[4][0][2] = 1;
	cubeAxis[4][1][0] = -1;
	cubeAxis[4][2][1] = 1;

	// -Z
	cubeAxis[5][0][2] = -1;
	cubeAxis[5][1][0] = 1;
	cubeAxis[5][2][1] = 1;

	//--------------------------------------------
	// CAPTURE SCENE LIGHTING TO CUBEMAPS
	//--------------------------------------------

	for( int i = 0; i < tr.primaryWorld->envprobeDefs.Num(); i++ )
	{
		RenderEnvprobeLocal* def = tr.primaryWorld->envprobeDefs[i];
		if( def == NULL )
		{
			continue;
		}

		for( int j = 0 ; j < 6 ; j++ )
		{
			ref = primary.renderView;

			ref.rdflags = RDF_NOAMBIENT | RDF_IRRADIANCE;
			ref.fov_x = ref.fov_y = 90;

			ref.vieworg = def->parms.origin;
			ref.viewaxis = cubeAxis[j];

			extension = envDirection[ j ];
			fullname.Format( "env/%s/envprobe%i%s", baseName.c_str(), i, extension );

			tr.TakeScreenshot( size, size, fullname, blends, &ref, EXR );
			//tr.CaptureRenderToFile( fullname, false );
		}
	}


	common->Printf( "Wrote a env set with the name %s\n", baseName.c_str() );

	//--------------------------------------------
	// CONVOLVE CUBEMAPS
	//--------------------------------------------

	int	start = Sys_Milliseconds();

	for( int i = 0; i < tr.primaryWorld->envprobeDefs.Num(); i++ )
	{
		RenderEnvprobeLocal* def = tr.primaryWorld->envprobeDefs[i];
		if( def == NULL )
		{
			continue;
		}

		fullname.Format( "%s/envprobe%i", baseName.c_str(), i );

		if( useThreads )
		{
			R_MakeAmbientMapThreaded( fullname.c_str(), "_amb", IRRADIANCE_CUBEMAP_SIZE, false, false );
			R_MakeAmbientMapThreaded( fullname.c_str(), "_spec", RADIANCE_CUBEMAP_SIZE, true, true );
		}
		else
		{
			R_MakeAmbientMap( fullname.c_str(), "_amb", IRRADIANCE_CUBEMAP_SIZE, false, false );
			R_MakeAmbientMap( fullname.c_str(), "_spec", RADIANCE_CUBEMAP_SIZE, true, true );
		}
	}

	if( useThreads )
	{
		//tr.envprobeJobList->Submit();
		tr.envprobeJobList->Submit( NULL, JOBLIST_PARALLELISM_MAX_CORES );
		tr.envprobeJobList->Wait();

		for( int j = 0; j < tr.irradianceJobs.Num(); j++ )
		{
			calcEnvprobeParms_t* job = tr.irradianceJobs[ j ];

			R_WriteEXR( job->filename, ( byte* )job->outBuffer, 3, job->outWidth, job->outHeight, "fs_basepath" );

			common->Printf( "%s convolved in %5.1f seconds\n\n", job->filename.c_str(), job->time * 0.001f );

			for( int i = 0; i < 6; i++ )
			{
				if( job->buffers[i] )
				{
					Mem_Free( job->buffers[i] );
				}
			}

			Mem_Free( job->outBuffer );

			delete job;
		}

		tr.irradianceJobs.Clear();
	}

	int	end = Sys_Milliseconds();

	common->Printf( "convolved probes in %5.1f seconds\n\n", ( end - start ) * 0.001f );
}

/*
==================
R_MakeAmbientMap_f

R_MakeAmbientMap_f <basename> [size]

Saves out env/<basename>_amb_ft.tga, etc
==================
*/
//void R_MakeAmbientMap_f( const idCmdArgs& args )
CONSOLE_COMMAND( makeAmbientMap, "Saves out env/<basename>_amb_ft.tga, etc", NULL )
{
	const char*	baseName;
	int			outSize;
	float		roughness;

	if( args.Argc() != 2 && args.Argc() != 3 && args.Argc() != 4 )
	{
		common->Printf( "USAGE: makeAmbientMap <basename> [size]\n" );
		return;
	}
	baseName = args.Argv( 1 );

	if( args.Argc() >= 3 )
	{
		outSize = atoi( args.Argv( 2 ) );
	}
	else
	{
		outSize = 32;
	}

	if( args.Argc() == 4 )
	{
		roughness = atof( args.Argv( 3 ) );
	}
	else
	{
		roughness = 0.95f;
	}

	if( roughness > 0.8f )
	{
		R_MakeAmbientMap( baseName, "_amb", outSize, false, false );
	}
	else
	{
		R_MakeAmbientMap( baseName, "_spec", outSize, true, false );
	}
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

	CommandlineProgressBar progressBar( outSize * outSize );

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

			progressBar.Increment();
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
