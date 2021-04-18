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

#define LGRID_FILE_EXT			"lightgrid"
#define LGRID_BINARYFILE_EXT	"blightgrid"
#define LGRID_FILEID			"LGRID"
#define LGRID_FILEVERSION		"1.00"

#define STORE_LIGHTGRID_SHDATA 0

static const byte BLGRID_VERSION = 2;
static const unsigned int BLGRID_MAGIC = ( 'P' << 24 ) | ( 'R' << 16 ) | ( 'O' << 8 ) | BLGRID_VERSION;



static const int MAX_LIGHTGRID_ATLAS_SIZE	= 2048;
static const int MAX_AREA_LIGHTGRID_POINTS	= ( MAX_LIGHTGRID_ATLAS_SIZE / LIGHTGRID_IRRADIANCE_SIZE ) * ( MAX_LIGHTGRID_ATLAS_SIZE / LIGHTGRID_IRRADIANCE_SIZE );

LightGrid::LightGrid()
{
	lightGridSize.Set( 64, 64, 128 );
	area = -1;

	irradianceImage = NULL;
}

void LightGrid::SetupLightGrid( const idBounds& bounds, const char* mapName, const idRenderWorld* world, int _area, int limit )
{
	//idLib::Printf( "----- SetupLightGrid -----\n" );

	lightGridSize.Set( 64, 64, 128 );
	lightGridPoints.Clear();

	area = _area;

	idVec3 maxs;
	int j = 0;

	int maxGridPoints = MAX_AREA_LIGHTGRID_POINTS;
	if( limit >= 100 && limit < MAX_AREA_LIGHTGRID_POINTS )
	{
		maxGridPoints = limit;
	}

	int numGridPoints = maxGridPoints + 1;
	while( numGridPoints > maxGridPoints )
	{
		for( int i = 0; i < 3; i++ )
		{
			lightGridOrigin[i] = lightGridSize[i] * ceil( bounds[0][i] / lightGridSize[i] );
			maxs[i] = lightGridSize[i] * floor( bounds[1][i] / lightGridSize[i] );
			lightGridBounds[i] = ( maxs[i] - lightGridOrigin[i] ) / lightGridSize[i] + 1;
		}

		numGridPoints = lightGridBounds[0] * lightGridBounds[1] * lightGridBounds[2];

		if( numGridPoints > maxGridPoints )
		{
			lightGridSize[ j++ % 3 ] += 16.0f;
		}
	}

	if( numGridPoints > 0 )
	{
		lightGridPoints.SetNum( numGridPoints );

		idLib::Printf( "\narea %i (%i x %i x %i) = %i grid points \n", area, lightGridBounds[0], lightGridBounds[1], lightGridBounds[2], numGridPoints );
		idLib::Printf( "area %i grid size (%i %i %i)\n", area, ( int )lightGridSize[0], ( int )lightGridSize[1], ( int )lightGridSize[2] );
		idLib::Printf( "area %i grid bounds (%i %i %i)\n", area, ( int )lightGridBounds[0], ( int )lightGridBounds[1], ( int )lightGridBounds[2] );
		idLib::Printf( "area %i %9u x %" PRIuSIZE " = lightGridSize = (%.2fMB)\n", area, numGridPoints, sizeof( lightGridPoint_t ), ( float )( lightGridPoints.MemoryUsed() ) / ( 1024.0f * 1024.0f ) );

		CalculateLightGridPointPositions( world, area );
	}
}

void LightGrid::GetBaseGridCoord( const idVec3& origin, int gridCoord[3] )
{
	int             pos[3];

	idVec3 lightOrigin = origin - lightGridOrigin;
	for( int i = 0; i < 3; i++ )
	{
		float           v;

		v = lightOrigin[i] * ( 1.0f / lightGridSize[i] );
		pos[i] = floor( v );

		if( pos[i] < 0 )
		{
			pos[i] = 0;
		}
		else if( pos[i] >= lightGridBounds[i] - 1 )
		{
			pos[i] = lightGridBounds[i] - 1;
		}

		gridCoord[i] = pos[i];
	}
}

int	LightGrid::GridCoordToProbeIndex( int gridCoord[3] )
{
	int             gridStep[3];

	gridStep[0] = 1;
	gridStep[1] = lightGridBounds[0];
	gridStep[2] = lightGridBounds[0] * lightGridBounds[1];

	int gridPointIndex = gridCoord[0] * gridStep[0] + gridCoord[1] * gridStep[1] + gridCoord[2] * gridStep[2];

	return gridPointIndex;
}

void LightGrid::ProbeIndexToGridCoord( const int probeIndex, int gridCoord[3] )
{
#if 1
	// slow brute force method only for debugging
	int             gridStep[3];

	gridStep[0] = 1;
	gridStep[1] = lightGridBounds[0];
	gridStep[2] = lightGridBounds[0] * lightGridBounds[1];

	gridCoord[0] = 0;
	gridCoord[1] = 0;
	gridCoord[2] = 0;

	int p = 0;
	for( int i = 0; i < lightGridBounds[0]; i += 1 )
	{
		for( int j = 0; j < lightGridBounds[1]; j += 1 )
		{
			for( int k = 0; k < lightGridBounds[2]; k += 1 )
			{
				if( probeIndex == p )
				{
					gridCoord[0] = i;
					gridCoord[1] = j;
					gridCoord[2] = k;

					return;
				}

				p++;
			}
		}
	}
#else

	gridPoints

	GetBaseGridCoord()
#endif
}

idVec3 LightGrid::GetGridCoordDebugColor( int gridCoord[3] )
{
	idVec3 color( colorGold.x, colorGold.y, colorGold.z );

#if 0
	color.x = float( gridCoord[0] & 1 );
	color.y = float( gridCoord[1] & 1 );
	color.z = float( gridCoord[2] & 1 );

	//color *= ( 1.0f / Max( color.x + color.y + color.z, 0.01f ) );
	//color = color * 0.6f + idVec3( 0.2f );

#else
	int             gridStep[3];

	gridStep[0] = 1;
	gridStep[1] = lightGridBounds[0];
	gridStep[2] = lightGridBounds[0] * lightGridBounds[1];

	int gridPointIndex = gridCoord[0] * gridStep[0] + gridCoord[1] * gridStep[1] + gridCoord[2] * gridStep[2];

	const int numColors = 7;
	static idVec4 colors[numColors] = { colorBlack, colorBlue, colorCyan, colorGreen, colorYellow, colorRed, colorWhite };

	color.x = colors[ gridPointIndex % numColors ].x;
	color.y = colors[ gridPointIndex % numColors ].y;
	color.z = colors[ gridPointIndex % numColors ].z;
#endif

	return color;
}

idVec3 LightGrid::GetProbeIndexDebugColor( const int probeIndex )
{
	idVec3 color( colorGold.x, colorGold.y, colorGold.z );

	int gridCoord[3];
	ProbeIndexToGridCoord( probeIndex, gridCoord );

	return GetGridCoordDebugColor( gridCoord );
}

int LightGrid::CountValidGridPoints() const
{
	int validCount = 0;

	for( int i = 0; i < lightGridPoints.Num(); i += 1 )
	{
		if( lightGridPoints[i].valid > 0 )
		{
			validCount++;
		}
	}

	return validCount;
}

void LightGrid::CalculateLightGridPointPositions( const idRenderWorld* world, int area )
{
	// calculate grid point positions
	int             gridStep[3];
	int             pos[3];
	idVec3          posFloat;

	gridStep[0] = 1;
	gridStep[1] = lightGridBounds[0];
	gridStep[2] = lightGridBounds[0] * lightGridBounds[1];

	int invalidCount = 0;
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

				gridPoint->valid = ( world->PointInArea( gridPoint->origin ) != -1 );
				if( !gridPoint->valid )
				{
					idVec3			origin;
					idVec3          baseOrigin;
					int             step;

					baseOrigin = gridPoint->origin;
#if 1
					// RB: do what q3map1 did - try to nudge the origin around to find a valid point
					for( step = 9; step <= 18; step += 9 )
					{
						for( int c = 0; c < 8; c++ )
						{
							origin = baseOrigin;
							if( c & 1 )
							{
								origin[0] += step;
							}
							else
							{
								origin[0] -= step;
							}
							if( c & 2 )
							{
								origin[1] += step;
							}
							else
							{
								origin[1] -= step;
							}
							if( c & 4 )
							{
								origin[2] += step;
							}
							else
							{
								origin[2] -= step;
							}

							if( world->PointInArea( origin ) != -1 )
							{
								// point is not in the void
								gridPoint->valid = true;
								gridPoint->origin = origin;
								break;
							}
						}

						if( i != 8 )
						{
							break;
						}
					}
#endif

					/*
					if( step > 18 )
					{
						// can't find a valid point at all
						for( i = 0; i < 3; i++ )
						{
							gridPoint->ambient[i] = 0;
							gridPoint->directed[i] = 0;
						}
						gridPoint->latLong[0] = 0;
						gridPoint->latLong[1] = 0;
						return;
					}
					*/

					if( !gridPoint->valid )
					{
						invalidCount++;
					}
				}

				p++;
			}
		}
	}

	//validGridPoints = p - invalidCount;

	idLib::Printf( "area %i: %i of %i grid points in empty space (%.2f%%)\n", area, invalidCount, lightGridPoints.Num(), ( ( float ) invalidCount / lightGridPoints.Num() ) * 100 );
}

void idRenderWorldLocal::SetupLightGrid()
{
	idLib::Printf( "----- SetupLightGrid -----\n" );

	idStrStatic< MAX_OSPATH > baseName = mapName;
	baseName.StripFileExtension();

	idStr filename;
	filename.Format( "%s.lightgrid", baseName.c_str() );
	bool loaded = LoadLightGridFile( filename );
	if( loaded )
	{
		// try to load existing lightgrid image data
		for( int i = 0; i < numPortalAreas; i++ )
		{
			portalArea_t* area = &portalAreas[i];

			filename.Format( "env/%s/area%i_lightgrid_amb", baseName.c_str(), i );
			area->lightGrid.irradianceImage = globalImages->ImageFromFile( filename, TF_NEAREST, TR_CLAMP, TD_R11G11B10F, CF_2D );
		}
	}
	else
	{
		int totalGridPoints = 0;

		for( int i = 0; i < numPortalAreas; i++ )
		{
			portalArea_t* area = &portalAreas[i];

			area->lightGrid.SetupLightGrid( area->globalBounds, mapName, this, i, -1 );

			totalGridPoints += area->lightGrid.CountValidGridPoints();
		}

		idLib::Printf( "----------------------------------\n" );
		idLib::Printf( "Total valid light grid points %i\n", totalGridPoints );
	}
}

/*
===============================================================================

Reading / Writing of light grids files

===============================================================================
*/

void idRenderWorldLocal::WriteLightGridsToFile( const char* filename )
{
	idFile* fp;
	idStr name;

	name = filename;
	name.SetFileExtension( LGRID_FILE_EXT );

	common->Printf( "writing %s\n", name.c_str() );
	fp = fileSystem->OpenFileWrite( name, "fs_basepath" );
	if( !fp )
	{
		common->Warning( "idCollisionModelManagerLocal::WriteCollisionModelsToFile: Error opening file %s\n", name.c_str() );
		return;
	}

	// write file id and version
	fp->WriteFloatString( "%s \"%s\"\n\n", LGRID_FILEID, LGRID_FILEVERSION );

	// write the map file crc
	//fp->WriteFloatString( "%u\n\n", mapFileCRC );

	for( int i = 0; i < numPortalAreas; i++ )
	{
		portalArea_t* area = &portalAreas[i];

		WriteLightGrid( fp, area->lightGrid );
	}

	fileSystem->CloseFile( fp );
}

void idRenderWorldLocal::WriteLightGrid( idFile* fp, const LightGrid& lightGrid )
{
	fp->WriteFloatString( "lightGridPoints { /* area = */ %i /* numLightGridPoints = */ %i\n", lightGrid.area, lightGrid.lightGridPoints.Num() );

	fp->WriteFloatString( "/* gridMins */ " );
	fp->WriteFloatString( "\t ( %f %f %f )\n", lightGrid.lightGridOrigin[0], lightGrid.lightGridOrigin[1], lightGrid.lightGridOrigin[2] );

	fp->WriteFloatString( "/* gridSize */ " );
	fp->WriteFloatString( "\t ( %f %f %f )\n", lightGrid.lightGridSize[0], lightGrid.lightGridSize[1], lightGrid.lightGridSize[2] );

	fp->WriteFloatString( "/* gridBounds */ " );
	fp->WriteFloatString( "%i %i %i\n\n", lightGrid.lightGridBounds[0], lightGrid.lightGridBounds[1], lightGrid.lightGridBounds[2] );

	for( int i = 0 ; i < lightGrid.lightGridPoints.Num() ; i++ )
	{
		const lightGridPoint_t* gridPoint = &lightGrid.lightGridPoints[i];

		fp->WriteFloatString( "/* lgp %i */ %d ( %f %f %f )", i, ( int )gridPoint->valid, gridPoint->origin[0], gridPoint->origin[1], gridPoint->origin[2] );

#if STORE_LIGHTGRID_SHDATA
		// spherical harmonic
		fp->WriteFloatString( "( " );

		for( int j = 0; j < shSize( 3 ); j++ )
		{
			fp->WriteFloatString( "%f %f %f ", gridPoint->shRadiance[j][0], gridPoint->shRadiance[j][1], gridPoint->shRadiance[j][2] );
		}

		fp->WriteFloatString( ")\n" );
#endif
	}

	fp->WriteFloatString( "}\n\n" );
}


bool idRenderWorldLocal::LoadLightGridFile( const char* name )
{
	idToken token;
	idLexer* src;
	//unsigned int crc;

	// load it
	idStrStatic< MAX_OSPATH > fileName = name;

	// check for generated file
	idStrStatic< MAX_OSPATH > generatedFileName = fileName;
	generatedFileName.Insert( "generated/", 0 );
	generatedFileName.SetFileExtension( LGRID_BINARYFILE_EXT );

	// if we are reloading the same map, check the timestamp
	// and try to skip all the work
	ID_TIME_T currentTimeStamp = fileSystem->GetTimestamp( fileName );

	// see if we have a generated version of this
	bool loaded = false;

#if 1
	idFileLocal file( fileSystem->OpenFileReadMemory( generatedFileName ) );
	if( file != NULL )
	{
		int numEntries = 0;
		//int magic = 0;
		file->ReadBig( numEntries );
		//file->ReadString( mapName );
		//file->ReadBig( crc );
		idStrStatic< 32 > fileID;
		idStrStatic< 32 > fileVersion;
		file->ReadString( fileID );
		file->ReadString( fileVersion );
		if( fileID == LGRID_FILEID && fileVersion == LGRID_FILEVERSION ) //&& crc == mapFileCRC && numEntries > 0 )
		{
			loaded = true;
			for( int i = 0; i < numEntries; i++ )
			{
				idStrStatic< MAX_OSPATH > type;
				file->ReadString( type );
				type.ToLower();

				if( type == "lightgridpoints" )
				{
					ReadBinaryLightGridPoints( file );
				}
				else
				{
					idLib::Error( "Binary proc file failed, unexpected type %s\n", type.c_str() );
				}
			}
		}
	}
#endif

	if( !loaded )
	{
		fileName.SetFileExtension( LGRID_FILE_EXT );
		src = new( TAG_RENDER ) idLexer( fileName, LEXFL_NOSTRINGCONCAT | LEXFL_NODOLLARPRECOMPILE );
		if( !src->IsLoaded() )
		{
			delete src;
			return false;
		}

		int numEntries = 0;
		idFileLocal outputFile( fileSystem->OpenFileWrite( generatedFileName, "fs_basepath" ) );
		if( outputFile != NULL )
		{
			outputFile->WriteBig( numEntries );
			//outputFile->WriteString( mapName );
			//outputFile->WriteBig( mapFileCRC );
			outputFile->WriteString( LGRID_FILEID );
			outputFile->WriteString( LGRID_FILEVERSION );
		}

		if( !src->ExpectTokenString( LGRID_FILEID ) )
		{
			common->Warning( "%s is not an CM file.", fileName.c_str() );
			delete src;
			return false;
		}

		if( !src->ReadToken( &token ) || token != LGRID_FILEVERSION )
		{
			common->Warning( "%s has version %s instead of %s", fileName.c_str(), token.c_str(), LGRID_FILEVERSION );
			delete src;
			return false;
		}

		//if( !src->ExpectTokenType( TT_NUMBER, TT_INTEGER, &token ) )
		//{
		//	common->Warning( "%s has no map file CRC", fileName.c_str() );
		//	delete src;
		//	return false;
		//}

		//crc = token.GetUnsignedLongValue();
		//if( mapFileCRC && crc != mapFileCRC )
		//{
		//	common->Printf( "%s is out of date\n", fileName.c_str() );
		//	delete src;
		//	return false;
		//}

		// parse the file
		while( 1 )
		{
			if( !src->ReadToken( &token ) )
			{
				break;
			}

			if( token == "lightGridPoints" )
			{
				ParseLightGridPoints( src, outputFile );

				numEntries++;
				continue;
			}

			src->Error( "idRenderWorldLocal::LoadLightGridFile: bad token \"%s\"", token.c_str() );
		}

		delete src;

		if( outputFile != NULL )
		{
			outputFile->Seek( 0, FS_SEEK_SET );
			outputFile->WriteBig( numEntries );
		}
	}

	return true;
}

void idRenderWorldLocal::ParseLightGridPoints( idLexer* src, idFile* fileOut )
{
	src->ExpectTokenString( "{" );

	int areaIndex = src->ParseInt();
	if( areaIndex < 0 || areaIndex >= NumAreas() )
	{
		src->Error( "ParseLightGridPoints: bad area index %i", areaIndex );
		return;
	}

	int numLightGridPoints = src->ParseInt();
	if( numLightGridPoints < 0 )
	{
		src->Error( "ParseLightGridPoints: bad numLightGridPoints" );
		return;
	}

	if( fileOut != NULL )
	{
		// write out the type so the binary reader knows what to instantiate
		fileOut->WriteString( "lightGridPoints" );
	}

	portalArea_t* area = &portalAreas[areaIndex];
	area->lightGrid.area = areaIndex;

	// gridMins
	src->Parse1DMatrix( 3, area->lightGrid.lightGridOrigin.ToFloatPtr() );
	src->Parse1DMatrix( 3, area->lightGrid.lightGridSize.ToFloatPtr() );
	for( int i = 0; i < 3; i++ )
	{
		area->lightGrid.lightGridBounds[i] = src->ParseInt();
	}

	area->lightGrid.lightGridPoints.SetNum( numLightGridPoints );

	idLib::Printf( "\narea %i (%i x %i x %i) = %i grid points \n", areaIndex, area->lightGrid.lightGridBounds[0], area->lightGrid.lightGridBounds[1], area->lightGrid.lightGridBounds[2], numLightGridPoints );
	idLib::Printf( "area %i grid size (%i %i %i)\n", areaIndex, ( int )area->lightGrid.lightGridSize[0], ( int )area->lightGrid.lightGridSize[1], ( int )area->lightGrid.lightGridSize[2] );
	idLib::Printf( "area %i grid bounds (%i %i %i)\n", areaIndex, ( int )area->lightGrid.lightGridBounds[0], ( int )area->lightGrid.lightGridBounds[1], ( int )area->lightGrid.lightGridBounds[2] );
	idLib::Printf( "area %i %9u x %" PRIuSIZE " = lightGridSize = (%.2fMB)\n", areaIndex, numLightGridPoints, sizeof( lightGridPoint_t ), ( float )( area->lightGrid.lightGridPoints.MemoryUsed() ) / ( 1024.0f * 1024.0f ) );

	if( fileOut != NULL )
	{
		fileOut->WriteBig( areaIndex );
		fileOut->WriteBig( numLightGridPoints );
		fileOut->WriteBig( area->lightGrid.lightGridOrigin );
		fileOut->WriteBig( area->lightGrid.lightGridSize );
		fileOut->WriteBigArray( area->lightGrid.lightGridBounds, 3 );
	}

	for( int i = 0; i < numLightGridPoints; i++ )
	{
		lightGridPoint_t* gridPoint = &area->lightGrid.lightGridPoints[i];

		gridPoint->valid = src->ParseInt();

		src->Parse1DMatrix( 3, gridPoint->origin.ToFloatPtr() );
#if STORE_LIGHTGRID_SHDATA
		src->Parse1DMatrix( shSize( 3 ) * 3, gridPoint->shRadiance[0].ToFloatPtr() );
#endif

		if( fileOut != NULL )
		{
			fileOut->WriteBig( gridPoint->valid );
			fileOut->WriteBig( gridPoint->origin );

#if STORE_LIGHTGRID_SHDATA
			fileOut->WriteBigArray( gridPoint->shRadiance[0].ToFloatPtr(), shSize( 3 ) * 3 );
#endif
		}
	}

	src->ExpectTokenString( "}" );
}

void idRenderWorldLocal::ReadBinaryLightGridPoints( idFile* file )
{
	int areaIndex;
	file->ReadBig( areaIndex );

	if( areaIndex < 0 || areaIndex >= NumAreas() )
	{
		idLib::Error( "ReadBinaryLightGridPoints: bad area index %i", areaIndex );
		return;
	}

	int numLightGridPoints = 0;
	file->ReadBig( numLightGridPoints );
	if( numLightGridPoints < 0 )
	{
		idLib::Error( "ReadBinaryLightGridPoints: bad numLightGridPoints" );
		return;
	}

	portalArea_t* area = &portalAreas[areaIndex];
	area->lightGrid.area = areaIndex;

	// gridMins
	file->ReadBig( area->lightGrid.lightGridOrigin );
	file->ReadBig( area->lightGrid.lightGridSize );
	file->ReadBigArray( area->lightGrid.lightGridBounds, 3 );

	area->lightGrid.lightGridPoints.SetNum( numLightGridPoints );

	idLib::Printf( "\narea %i (%i x %i x %i) = %i grid points \n", areaIndex, area->lightGrid.lightGridBounds[0], area->lightGrid.lightGridBounds[1], area->lightGrid.lightGridBounds[2], numLightGridPoints );
	idLib::Printf( "area %i grid size (%i %i %i)\n", areaIndex, ( int )area->lightGrid.lightGridSize[0], ( int )area->lightGrid.lightGridSize[1], ( int )area->lightGrid.lightGridSize[2] );
	idLib::Printf( "area %i grid bounds (%i %i %i)\n", areaIndex, ( int )area->lightGrid.lightGridBounds[0], ( int )area->lightGrid.lightGridBounds[1], ( int )area->lightGrid.lightGridBounds[2] );
	idLib::Printf( "area %i %9u x %" PRIuSIZE " = lightGridSize = (%.2fMB)\n", areaIndex, numLightGridPoints, sizeof( lightGridPoint_t ), ( float )( area->lightGrid.lightGridPoints.MemoryUsed() ) / ( 1024.0f * 1024.0f ) );

	for( int i = 0; i < numLightGridPoints; i++ )
	{
		lightGridPoint_t* gridPoint = &area->lightGrid.lightGridPoints[i];

		file->ReadBig( gridPoint->valid );
		file->ReadBig( gridPoint->origin );

		//file->ReadBigArray( gridPoint->shRadiance[0].ToFloatPtr(), shSize( 3 ) * 3 );
	}
}

/*
===============================================================================

Baking light grids files

===============================================================================
*/

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
	halfFloat_t*		buffers[6];

	int	start = Sys_Milliseconds();

	for( int i = 0; i < 6; i++ )
	{
		buffers[ i ] = ( halfFloat_t* ) parms->radiance[ i ];
	}

	const float invDstSize = 1.0f / float( parms->outHeight );

	const int numMips = idMath::BitsForInteger( parms->outHeight );

	const idVec2i sourceImageSize( parms->outHeight, parms->outHeight );

	// build L3 Spherical Harmonics from source image
	SphericalHarmonicsT<idVec3, 3> shRadiance;

	for( int i = 0; i < shSize( 3 ); i++ )
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
				R_SampleCubeMapHDR16F( dir, parms->outHeight, buffers, &radiance[0], u, v );

				//radiance = dir * 0.5 + idVec3( 0.5f, 0.5f, 0.5f );

				// convert from [0 .. size-1] to [-1.0 + invSize .. 1.0 - invSize]
				const float uu = 2.0f * ( u * invDstSize ) - 1.0f;
				const float vv = 2.0f * ( v * invDstSize ) - 1.0f;

				float texelArea = CubemapTexelSolidAngle( uu, vv, invDstSize );

				const SphericalHarmonicsT<float, 3>& sh = shEvaluate<3>( dir );

				bool shValid = true;
				for( int i = 0; i < shSize( 3 ); i++ )
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

	for( int i = 0; i < shSize( 3 ); i++ )
	{
		parms->shRadiance[i] = shRadiance[i];
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

	for( int x = 0; x < parms->outWidth; x++ )
	{
		for( int y = 0; y < parms->outHeight; y++ )
		{
			idVec2 octCoord = NormalizedOctCoord( x, y, parms->outWidth );

			// convert UV coord to 3D direction
			idVec3 dir;

			dir.FromOctahedral( octCoord );

			idVec3 outColor( 0, 0, 0 );

#if 1
			// generate ambient colors by evaluating the L3 Spherical Harmonics
			SphericalHarmonicsT<float, 3> shDirection = shEvaluate<3>( dir );

			idVec3 sampleIrradianceSh = shEvaluateDiffuse<idVec3, 3>( shRadiance, dir ) / idMath::PI;

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

	int	end = Sys_Milliseconds();

	parms->time = end - start;
}

REGISTER_PARALLEL_JOB( CalculateLightGridPointJob, "CalculateLightGridPointJob" );

#if 0
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

//	jobParms->samples = 1000;
//	jobParms->filename.Format( "env/%s%s.exr", baseName, suffix );

//	jobParms->printProgress = !useThreads;

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
#endif

CONSOLE_COMMAND( bakeLightGrids, "Bake irradiance/vis light grid data", NULL )
{
	idStr			baseName;
	idStr			filename;
	renderView_t	ref;
	int				blends;
	const char*		extension;
	int				captureSize;

	static const char* envDirection[6] = { "_px", "_nx", "_py", "_ny", "_pz", "_nz" };

	if( args.Argc() != 1 && args.Argc() != 2 )
	{
		common->Printf( "USAGE: bakeLightGrids [limit] (limit is max probes per BSP area)\n" );
		return;
	}

	if( !tr.primaryWorld )
	{
		common->Printf( "No primary world loaded.\n" );
		return;
	}

	bool useThreads = false;

	baseName = tr.primaryWorld->mapName;
	baseName.StripFileExtension();

	captureSize = RADIANCE_CUBEMAP_SIZE;
	blends = 1;

	if( !tr.primaryView )
	{
		common->Printf( "No primary view.\n" );
		return;
	}

	int limit = MAX_AREA_LIGHTGRID_POINTS;
	if( args.Argc() >= 2 )
	{
		limit = atoi( args.Argv( 1 ) );
	}

	idLib::Printf( "Using limit = %i\n", limit );

	const viewDef_t primary = *tr.primaryView;

	//--------------------------------------------
	// CAPTURE SCENE LIGHTING TO CUBEMAPS
	//--------------------------------------------

	int	totalStart = Sys_Milliseconds();

	for( int a = 0; a < tr.primaryWorld->NumAreas(); a++ )
	{
		portalArea_t* area = &tr.primaryWorld->portalAreas[a];

		//int numGridPoints = Min( area->lightGrid.lightGridPoints.Num(), limit );
		//if( numGridPoints == 0 )

		//int numGridPoints = area->lightGrid.lightGridPoints.Num();
		//if( numGridPoints == 0 || numGridPoints > limit )
		//{
		//	continue;
		//}

		area->lightGrid.SetupLightGrid( area->globalBounds, tr.primaryWorld->mapName, tr.primaryWorld, a, limit );

#if 1
		int numGridPoints = area->lightGrid.CountValidGridPoints();
		if( numGridPoints == 0 )
		{
			continue;
		}

		idLib::Printf( "Shooting %i grid probes area %i...\n", numGridPoints, a );

		CommandlineProgressBar progressBar( numGridPoints );
		if( !useThreads )
		{
			progressBar.Start();
		}

		int	start = Sys_Milliseconds();

		int gridStep[3];

		gridStep[0] = 1;
		gridStep[1] = area->lightGrid.lightGridBounds[0];
		gridStep[2] = area->lightGrid.lightGridBounds[0] * area->lightGrid.lightGridBounds[1];

		int gridCoord[3];

		for( int i = 0; i < area->lightGrid.lightGridBounds[0]; i += 1 )
		{
			for( int j = 0; j < area->lightGrid.lightGridBounds[1]; j += 1 )
			{
				for( int k = 0; k < area->lightGrid.lightGridBounds[2]; k += 1 )
				{
					gridCoord[0] = i;
					gridCoord[1] = j;
					gridCoord[2] = k;

					lightGridPoint_t* gridPoint = &area->lightGrid.lightGridPoints[ gridCoord[0] * gridStep[0] + gridCoord[1] * gridStep[1] + gridCoord[2] * gridStep[2] ];
					if( !gridPoint->valid )
					{
						progressBar.Increment();
						continue;
					}

					calcLightGridPointParms_t* jobParms = new calcLightGridPointParms_t;
					jobParms->gridCoord[0] = i;
					jobParms->gridCoord[1] = j;
					jobParms->gridCoord[2] = k;

					for( int side = 0; side < 6; side++ )
					{
						ref = primary.renderView;

						ref.rdflags = RDF_NOAMBIENT | RDF_IRRADIANCE;
						ref.fov_x = ref.fov_y = 90;

						ref.vieworg = gridPoint->origin;
						ref.viewaxis = tr.cubeAxis[ side ];

						extension = envDirection[ side ];

						//tr.TakeScreenshot( size, size, fullname, blends, &ref, EXR );
						byte* float16FRGB = tr.CaptureRenderToBuffer( captureSize, captureSize, &ref );

						jobParms->radiance[ side ] = float16FRGB;

#if 0
						if( i < 3 )
						{
							filename.Format( "env/%s/area%i_lightgridpoint%i%s.exr", baseName.c_str(), a, i, extension );
							R_WriteEXR( filename, float16FRGB, 3, size, size, "fs_basepath" );
						}
#endif
					}

					tr.lightGridJobs.Append( jobParms );

					progressBar.Increment();
				}
			}
		}

		int	end = Sys_Milliseconds();

		common->Printf( "captured light grid radiance for area %i in %5.1f seconds\n\n", a, ( end - start ) * 0.001f );

		//--------------------------------------------
		// GENERATE IRRADIANCE
		//--------------------------------------------

		if( !useThreads )
		{
			progressBar.Reset( tr.lightGridJobs.Num() );
			progressBar.Start();
		}

		start = Sys_Milliseconds();

		for( int j = 0; j < tr.lightGridJobs.Num(); j++ )
		{
			calcLightGridPointParms_t* jobParms = tr.lightGridJobs[ j ];

			jobParms->outWidth = LIGHTGRID_IRRADIANCE_SIZE;
			jobParms->outHeight = LIGHTGRID_IRRADIANCE_SIZE;
			jobParms->outBuffer = ( halfFloat_t* )R_StaticAlloc( idMath::Ceil( LIGHTGRID_IRRADIANCE_SIZE * LIGHTGRID_IRRADIANCE_SIZE * 3 * sizeof( halfFloat_t ) * 1.5f ), TAG_IMAGE );

			if( useThreads )
			{
				tr.envprobeJobList->AddJob( ( jobRun_t )CalculateLightGridPointJob, jobParms );
			}
			else
			{
				CalculateLightGridPointJob( jobParms );
				progressBar.Increment();
			}
		}

		if( useThreads )
		{
			//tr.envprobeJobList->Submit();
			tr.envprobeJobList->Submit( NULL, JOBLIST_PARALLELISM_MAX_CORES );
			tr.envprobeJobList->Wait();
		}



		int atlasWidth = area->lightGrid.lightGridBounds[0] * area->lightGrid.lightGridBounds[2] * LIGHTGRID_IRRADIANCE_SIZE;
		int atlasHeight = area->lightGrid.lightGridBounds[1] * LIGHTGRID_IRRADIANCE_SIZE;

		idTempArray<halfFloat_t> irradianceAtlas( atlasWidth * atlasHeight * 3 );

		// fill everything with solid black
		for( int i = 0; i < ( atlasWidth * atlasHeight ); i++ )
		{
			irradianceAtlas[i * 3 + 0] = F32toF16( 0.0f );
			irradianceAtlas[i * 3 + 1] = F32toF16( 0.0f );
			irradianceAtlas[i * 3 + 2] = F32toF16( 0.0f );
		}

		for( int j = 0; j < tr.lightGridJobs.Num(); j++ )
		{
			calcLightGridPointParms_t* job = tr.lightGridJobs[ j ];

			for( int x = 0; x < LIGHTGRID_IRRADIANCE_SIZE; x++ )
			{
				for( int y = 0; y < LIGHTGRID_IRRADIANCE_SIZE; y++ )
				{
					int xx = x + ( job->gridCoord[0] * gridStep[0] + job->gridCoord[2] * gridStep[1] ) * LIGHTGRID_IRRADIANCE_SIZE;
					int yy = y + job->gridCoord[1] * LIGHTGRID_IRRADIANCE_SIZE;

					irradianceAtlas[( yy * atlasWidth + xx ) * 3 + 0] = job->outBuffer[( y * LIGHTGRID_IRRADIANCE_SIZE + x ) * 3 + 0];
					irradianceAtlas[( yy * atlasWidth + xx ) * 3 + 1] = job->outBuffer[( y * LIGHTGRID_IRRADIANCE_SIZE + x ) * 3 + 1];
					irradianceAtlas[( yy * atlasWidth + xx ) * 3 + 2] = job->outBuffer[( y * LIGHTGRID_IRRADIANCE_SIZE + x ) * 3 + 2];
				}
			}

			// backup SH L3 data
			lightGridPoint_t* gridPoint = &area->lightGrid.lightGridPoints[ job->gridCoord[0] * gridStep[0] + job->gridCoord[1] * gridStep[1] + job->gridCoord[2] * gridStep[2] ];
			for( int i = 0; i < shSize( 3 ); i++ )
			{
				gridPoint->shRadiance[i] = job->shRadiance[i];
			}

			for( int i = 0; i < 6; i++ )
			{
				if( job->radiance[i] )
				{
					Mem_Free( job->radiance[i] );
				}
			}

			Mem_Free( job->outBuffer );

			delete job;
		}

		filename.Format( "env/%s/area%i_lightgrid_amb.exr", baseName.c_str(), a );

		R_WriteEXR( filename.c_str(), irradianceAtlas.Ptr(), 3, atlasWidth, atlasHeight, "fs_basepath" );

		tr.lightGridJobs.Clear();

		end = Sys_Milliseconds();

		common->Printf( "computed light grid irradiance for area %i in %5.1f seconds\n\n", a, ( end - start ) * 0.001f );
#endif
	}


	int totalEnd = Sys_Milliseconds();

	common->Printf( "Baked light grid irradiance in %5.1f minutes\n\n", ( totalEnd - totalStart ) / ( 1000.0f * 60 ) );

	// everything went ok so let's save the configurations to disc
	// so we can load the texture atlases with the correct subdivisions next time
	filename.Format( "%s.lightgrid", baseName.c_str() );
	tr.primaryWorld->WriteLightGridsToFile( filename );
}

#if 0
// straight port of Quake 3 / XreaL
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
			for( int j = 0; j < 3; j++ )
			{
				if( i & ( 1 << j ) )
				{
					factor *= frac[j];

#if 1
					gridPointIndex2 += gridStep[j];
					if( gridPointIndex2 < 0 || gridPointIndex2 >= area->lightGrid.lightGridPoints.Num() )
					{
						// ignore values outside lightgrid
						continue;
					}

					gridPoint2 = &area->lightGrid.lightGridPoints[ gridPointIndex2 ];
#else
					if( pos[j] + 1 > area->lightGrid.lightGridBounds[j] - 1 )
					{
						// ignore values outside lightgrid
						break;
					}

					gridPoint2 += gridStep[j];
#endif
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