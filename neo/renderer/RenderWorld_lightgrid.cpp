/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2021 Robert Beckebans

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "precompiled.h"
#pragma hdrstop

#include "RenderCommon.h"

// RB: old constant from q3map2
#define	MAX_MAP_LIGHTGRID_POINTS		0x100000

void LightGrid::SetupLightGrid( const idBounds& bounds )
{
	//idLib::Printf( "----- SetupLightGrid -----\n" );

	lightGridSize.Set( 64, 64, 128 );
	lightGridPoints.Clear();

	idVec3 maxs;
	int j = 0;
	int numGridPoints = MAX_MAP_LIGHTGRID_POINTS + 1;
	while( numGridPoints > MAX_MAP_LIGHTGRID_POINTS )
	{
		for( int i = 0; i < 3; i++ )
		{
			lightGridOrigin[i] = lightGridSize[i] * ceil( bounds[0][i] / lightGridSize[i] );
			maxs[i] = lightGridSize[i] * floor( bounds[1][i] / lightGridSize[i] );
			lightGridBounds[i] = ( maxs[i] - lightGridOrigin[i] ) / lightGridSize[i] + 1;
		}

		numGridPoints = lightGridBounds[0] * lightGridBounds[1] * lightGridBounds[2];

		if( numGridPoints > MAX_MAP_LIGHTGRID_POINTS )
		{
			lightGridSize[ j++ % 3 ] += 16.0f;
		}
	}

	idLib::Printf( "grid size (%i %i %i)\n", ( int )lightGridSize[0], ( int )lightGridSize[1], ( int )lightGridSize[2] );
	idLib::Printf( "grid bounds (%i %i %i)\n", ( int )lightGridBounds[0], ( int )lightGridBounds[1], ( int )lightGridBounds[2] );

	idLib::Printf( "%i x %i x %i = %i grid points \n", lightGridBounds[0], lightGridBounds[1], lightGridBounds[2], numGridPoints );

	lightGridPoints.SetNum( numGridPoints );

	idLib::Printf( "%9u x %" PRIuSIZE " = lightGridSize = (%.2fMB)\n", numGridPoints, sizeof( lightGridPoint_t ), ( float )( lightGridPoints.MemoryUsed() ) / ( 1024.0f * 1024.0f ) );

	CalculateLightGridPointPositions();
}

void LightGrid::ProbeIndexToGridIndex( const int probeIndex, int gridIndex[3] )
{
	// slow brute force method only for debugging
	int             gridStep[3];

	gridStep[0] = 1;
	gridStep[1] = lightGridBounds[0];
	gridStep[2] = lightGridBounds[0] * lightGridBounds[1];

	gridIndex[0] = 0;
	gridIndex[1] = 0;
	gridIndex[2] = 0;

	int p = 0;
	for( int i = 0; i < lightGridBounds[0]; i += 1 )
	{
		for( int j = 0; j < lightGridBounds[1]; j += 1 )
		{
			for( int k = 0; k < lightGridBounds[2]; k += 1 )
			{
				if( probeIndex == p )
				{
					gridIndex[0] = i;
					gridIndex[1] = j;
					gridIndex[2] = k;

					return;
				}

				p++;
			}
		}
	}
}

idVec3 LightGrid::GetProbeIndexDebugColor( const int probeIndex )
{
	idVec3 color( colorGold.x, colorGold.y, colorGold.z );

	int gridIndex[3];
	ProbeIndexToGridIndex( probeIndex, gridIndex );

	color.x = float( gridIndex[0] & 1 );
	color.y = float( gridIndex[1] & 1 );
	color.z = float( gridIndex[2] & 1 );

	color *= ( 1.0f / Max( color.x + color.y + color.z, 0.01f ) );
	color = color * 0.6f + idVec3( 0.2f );

	return color;
}

void LightGrid::CalculateLightGridPointPositions()
{
	// calculate grid point positions
	int             gridStep[3];
	int             pos[3];
	idVec3          posFloat;

	gridStep[0] = 1;
	gridStep[1] = lightGridBounds[0];
	gridStep[2] = lightGridBounds[0] * lightGridBounds[1];

	int p = 0;
	for( int i = 0; i < lightGridBounds[0]; i += 1 )
	{
		for( int j = 0; j < lightGridBounds[1]; j += 1 )
		{
			for( int k = 0; k < lightGridBounds[2]; k += 1 )
			{
				pos[0] = i;
				pos[1] = j;
				pos[2] = k;

				posFloat[0] = i * lightGridSize[0];
				posFloat[1] = j * lightGridSize[1];
				posFloat[2] = k * lightGridSize[2];

				lightGridPoint_t* gridPoint = &lightGridPoints[ pos[0] * gridStep[0] + pos[1] * gridStep[1] + pos[2] * gridStep[2] ];

				gridPoint->origin = lightGridOrigin + posFloat;

				p++;
			}
		}
	}
}

void idRenderWorldLocal::SetupLightGrid()
{
	idLib::Printf( "----- SetupLightGrid -----\n" );

	idBounds bounds;
	bounds.Clear();

	for( int i = 0; i < numPortalAreas; i++ )
	{
		portalArea_t* area = &portalAreas[i];

		bounds.AddBounds( area->globalBounds );
	}

	lightGrid.SetupLightGrid( bounds );
}


static const char* envDirection[6] = { "_px", "_nx", "_py", "_ny", "_pz", "_nz" };

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

void CalculateLightGridPointJob( calcLightGridPointParms_t* parms )
{
	byte*		buffers[6];

	int	start = Sys_Milliseconds();

	for( int i = 0; i < 6; i++ )
	{
		buffers[ i ] = parms->buffers[ i ];
	}

	const float invDstSize = 1.0f / float( parms->outHeight );

	const int numMips = idMath::BitsForInteger( parms->outHeight );

	const idVec2i sourceImageSize( parms->outHeight, parms->outHeight );

	// build L4 Spherical Harmonics from source image
	SphericalHarmonicsT<idVec3, 4> shRadiance;

	for( int i = 0; i < shSize( 4 ); i++ )
	{
		shRadiance[i].Zero();
	}

#if 0

	// build SH by only iterating over the octahedron
	// RB: not used because I don't know the texel area of an octahedron pixel and the cubemap texel area is too small
	// however it would be nice to use this because it would be 6 times faster

	idVec4 dstRect = R_CalculateMipRect( parms->outHeight, 0 );

	for( int x = dstRect.x; x < ( dstRect.x + dstRect.z ); x++ )
	{
		for( int y = dstRect.y; y < ( dstRect.y + dstRect.w ); y++ )
		{
			idVec2 octCoord = NormalizedOctCoord( x, y, dstRect.z );

			// convert UV coord to 3D direction
			idVec3 dir;

			dir.FromOctahedral( octCoord );

			float u, v;
			idVec3 radiance;
			R_SampleCubeMapHDR( dir, parms->outHeight, buffers, &radiance[0], u, v );

			//radiance = dir * 0.5 + idVec3( 0.5f, 0.5f, 0.5f );

			// convert from [0 .. size-1] to [-1.0 + invSize .. 1.0 - invSize]
			const float uu = 2.0f * ( u * invDstSize ) - 1.0f;
			const float vv = 2.0f * ( v * invDstSize ) - 1.0f;

			float texelArea = CubemapTexelSolidAngle( uu, vv, invDstSize );

			const SphericalHarmonicsT<float, 4>& sh = shEvaluate<4>( dir );

			bool shValid = true;
			for( int i = 0; i < 25; i++ )
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

#else

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
				R_SampleCubeMapHDR( dir, parms->outHeight, buffers, &radiance[0], u, v );

				//radiance = dir * 0.5 + idVec3( 0.5f, 0.5f, 0.5f );

				// convert from [0 .. size-1] to [-1.0 + invSize .. 1.0 - invSize]
				const float uu = 2.0f * ( u * invDstSize ) - 1.0f;
				const float vv = 2.0f * ( v * invDstSize ) - 1.0f;

				float texelArea = CubemapTexelSolidAngle( uu, vv, invDstSize );

				const SphericalHarmonicsT<float, 4>& sh = shEvaluate<4>( dir );

				bool shValid = true;
				for( int i = 0; i < 25; i++ )
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

#endif

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
			}
		}
	}

	int	end = Sys_Milliseconds();

	parms->time = end - start;
}

REGISTER_PARALLEL_JOB( CalculateLightGridPointJob, "CalculateLightGridPointJob" );

void R_MakeAmbientGridPoint( const char* baseName, const char* suffix, int outSize, bool deleteTempFiles, bool useThreads )
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
	calcLightGridPointParms_t* jobParms = new calcLightGridPointParms_t;

	for( int i = 0; i < 6; i++ )
	{
		jobParms->buffers[ i ] = buffers[ i ];
	}

	jobParms->samples = 1000;
	jobParms->filename.Format( "env/%s%s.exr", baseName, suffix );

	jobParms->printProgress = !useThreads;

	jobParms->outWidth = int( outSize * 1.5f );
	jobParms->outHeight = outSize;
	jobParms->outBuffer = ( halfFloat_t* )R_StaticAlloc( idMath::Ceil( outSize * outSize * 3 * sizeof( halfFloat_t ) * 1.5f ), TAG_IMAGE );

	tr.lightGridJobs.Append( jobParms );

	if( useThreads )
	{
		tr.envprobeJobList->AddJob( ( jobRun_t )CalculateLightGridPointJob, jobParms );
	}
	else
	{
		CalculateLightGridPointJob( jobParms );
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

CONSOLE_COMMAND( generateLightGrid, "Generate light grid data", NULL )
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

	bool useThreads = false;

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

	//--------------------------------------------
	// CAPTURE SCENE LIGHTING TO CUBEMAPS
	//--------------------------------------------

	for( int i = 0; i < tr.primaryWorld->lightGrid.lightGridPoints.Num(); i++ )
	{
		lightGridPoint_t* gridPoint = &tr.primaryWorld->lightGrid.lightGridPoints[i];

		for( int j = 0 ; j < 6 ; j++ )
		{
			ref = primary.renderView;

			ref.rdflags = RDF_NOAMBIENT | RDF_IRRADIANCE;
			ref.fov_x = ref.fov_y = 90;

			ref.vieworg = gridPoint->origin;
			ref.viewaxis = tr.cubeAxis[j];

			extension = envDirection[ j ];
			fullname.Format( "env/%s/lightgridpoint%i%s", baseName.c_str(), i, extension );

			tr.TakeScreenshot( size, size, fullname, blends, &ref, EXR );
			//tr.CaptureRenderToFile( fullname, false );
		}
	}


	common->Printf( "Wrote a env set with the name %s\n", baseName.c_str() );

	//--------------------------------------------
	// GENERATE IRRADIANCE
	//--------------------------------------------

	CommandlineProgressBar progressBar( tr.primaryWorld->lightGrid.lightGridPoints.Num() );
	if( !useThreads )
	{
		progressBar.Start();
	}

	int	start = Sys_Milliseconds();

	for( int i = 0; i < tr.primaryWorld->lightGrid.lightGridPoints.Num(); i++ )
	{
		lightGridPoint_t* gridPoint = &tr.primaryWorld->lightGrid.lightGridPoints[i];

		fullname.Format( "%s/lightgridpoint%i", baseName.c_str(), i );

		R_MakeAmbientGridPoint( fullname.c_str(), "_amb", IRRADIANCE_CUBEMAP_SIZE, true, useThreads );

		if( !useThreads )
		{
			progressBar.Increment();
		}
	}

	if( useThreads )
	{
		//tr.envprobeJobList->Submit();
		tr.envprobeJobList->Submit( NULL, JOBLIST_PARALLELISM_MAX_CORES );
		tr.envprobeJobList->Wait();
	}

	for( int j = 0; j < tr.lightGridJobs.Num(); j++ )
	{
		calcLightGridPointParms_t* job = tr.lightGridJobs[ j ];

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

	tr.lightGridJobs.Clear();

	int	end = Sys_Milliseconds();

	common->Printf( "convolved probes in %5.1f seconds\n\n", ( end - start ) * 0.001f );
}

#if 0
// straight port of Quake 3
void idRenderWorldLocal::SetupEntityGridLighting( idRenderEntityLocal* def )
{
	// lighting calculations
#if 0
	if( def->lightgridCalculated )
	{
		return;
	}

	def->lightgridCalculated = true;
#endif

	if( lightGridPoints.Num() > 0 )
	{
		idVec3          lightOrigin;
		int             pos[3];
		int             i, j;
		int				gridPointIndex;
		lightGridPoint_t*  gridPoint;
		lightGridPoint_t*  gridPoint2;
		float           frac[3];
		int             gridStep[3];
		idVec3          direction;
		idVec3			direction2;
		float			lattitude;
		float			longitude;
		float           totalFactor;

#if 0
		if( forcedOrigin )
		{
			VectorCopy( forcedOrigin, lightOrigin );
		}
		else
		{
			if( ent->e.renderfx & RF_LIGHTING_ORIGIN )
			{
				// seperate lightOrigins are needed so an object that is
				// sinking into the ground can still be lit, and so
				// multi-part models can be lit identically
				VectorCopy( ent->e.lightingOrigin, lightOrigin );
			}
			else
			{
				VectorCopy( ent->e.origin, lightOrigin );
			}
		}
#else
		// some models, like empty particles have no volume
#if 1
		lightOrigin = def->parms.origin;
#else
		if( def->referenceBounds.IsCleared() )
		{
			lightOrigin = def->parms.origin;
		}
		else
		{
			lightOrigin = def->volumeMidPoint;
		}
#endif

#endif

		lightOrigin -= lightGridOrigin;
		for( i = 0; i < 3; i++ )
		{
			float           v;

			v = lightOrigin[i] * ( 1.0f / lightGridSize[i] );
			pos[i] = floor( v );
			frac[i] = v - pos[i];
			if( pos[i] < 0 )
			{
				pos[i] = 0;
			}
			else if( pos[i] >= lightGridBounds[i] - 1 )
			{
				pos[i] = lightGridBounds[i] - 1;
			}
		}

		def->ambientLight.Zero();
		def->directedLight.Zero();
		direction.Zero();

		// trilerp the light value
		gridStep[0] = 1;
		gridStep[1] = lightGridBounds[0];
		gridStep[2] = lightGridBounds[0] * lightGridBounds[1];

		gridPointIndex = pos[0] * gridStep[0] + pos[1] * gridStep[1] + pos[2] * gridStep[2];
		gridPoint = &lightGridPoints[ gridPointIndex ];

		totalFactor = 0;
		for( i = 0; i < 8; i++ )
		{
			float           factor;

			factor = 1.0;
			gridPoint2 = gridPoint;
			for( j = 0; j < 3; j++ )
			{
				if( i & ( 1 << j ) )
				{
					int gridPointIndex2 = gridPointIndex + gridStep[j];

					if( gridPointIndex2 < 0 || gridPointIndex2 >= lightGridPoints.Num() )
					{
						continue;
					}

					factor *= frac[j];

					gridPoint2 = &lightGridPoints[ gridPointIndex + gridStep[j] ];
				}
				else
				{
					factor *= ( 1.0f - frac[j] );
				}
			}

			if( !( gridPoint2->ambient[0] + gridPoint2->ambient[1] + gridPoint2->ambient[2] ) )
			{
				continue;			// ignore samples in walls
			}

			totalFactor += factor;

			def->ambientLight[0] += factor * gridPoint2->ambient[0] * ( 1.0f / 255.0f );
			def->ambientLight[1] += factor * gridPoint2->ambient[1] * ( 1.0f / 255.0f );
			def->ambientLight[2] += factor * gridPoint2->ambient[2] * ( 1.0f / 255.0f );

			def->directedLight[0] += factor * gridPoint2->directed[0] * ( 1.0f / 255.0f );
			def->directedLight[1] += factor * gridPoint2->directed[1] * ( 1.0f / 255.0f );
			def->directedLight[2] += factor * gridPoint2->directed[2] * ( 1.0f / 255.0f );

			lattitude = DEG2RAD( gridPoint2->latLong[1] * ( 360.0f / 255.0f ) );
			longitude = DEG2RAD( gridPoint2->latLong[0] * ( 360.0f / 255.0f ) );

			direction2[0] = idMath::Cos( lattitude ) * idMath::Sin( longitude );
			direction2[1] = idMath::Sin( lattitude ) * idMath::Sin( longitude );
			direction2[2] = idMath::Cos( longitude );

			direction += ( direction2 * factor );

			//direction += ( gridPoint2->dir * factor );
		}

#if 1
		if( totalFactor > 0 && totalFactor < 0.99 )
		{
			totalFactor = 1.0f / totalFactor;
			def->ambientLight *= totalFactor;
			def->directedLight *= totalFactor;
		}
#endif

		def->ambientLight[0] = idMath::ClampFloat( 0, 1, def->ambientLight[0] );
		def->ambientLight[1] = idMath::ClampFloat( 0, 1, def->ambientLight[1] );
		def->ambientLight[2] = idMath::ClampFloat( 0, 1, def->ambientLight[2] );

		def->directedLight[0] = idMath::ClampFloat( 0, 1, def->directedLight[0] );
		def->directedLight[1] = idMath::ClampFloat( 0, 1, def->directedLight[1] );
		def->directedLight[2] = idMath::ClampFloat( 0, 1, def->directedLight[2] );

		def->lightDir = direction;
		def->lightDir.Normalize();

#if 0
		if( VectorLength( ent->ambientLight ) < r_forceAmbient->value )
		{
			ent->ambientLight[0] = r_forceAmbient->value;
			ent->ambientLight[1] = r_forceAmbient->value;
			ent->ambientLight[2] = r_forceAmbient->value;
		}
#endif
	}
}

#endif