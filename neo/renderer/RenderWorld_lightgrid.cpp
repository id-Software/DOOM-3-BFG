/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2021 Robert Beckebans
Copyright (C) 2022 Stephen Pridham

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
#include "CmdlineProgressbar.h"
#include "../framework/Common_local.h" // commonLocal.WaitGameThread();

#if defined( USE_NVRHI )
	#include <sys/DeviceManager.h>
	extern DeviceManager* deviceManager;
#endif


#define LGRID_FILE_EXT			"lightgrid"
#define LGRID_BINARYFILE_EXT	"blightgrid"
#define LGRID_FILEID			"LGRID"
const byte LGRID_VERSION = 3;



static const byte BLGRID_VERSION = 4;
static const unsigned int BLGRID_MAGIC = ( 'P' << 24 ) | ( 'R' << 16 ) | ( 'O' << 8 ) | BLGRID_VERSION;


static const int MAX_LIGHTGRID_ATLAS_SIZE	= 2048;
static const int MAX_AREA_LIGHTGRID_POINTS	= ( MAX_LIGHTGRID_ATLAS_SIZE / LIGHTGRID_IRRADIANCE_SIZE ) * ( MAX_LIGHTGRID_ATLAS_SIZE / LIGHTGRID_IRRADIANCE_SIZE );

static idVec3 defaultLightGridSize = idVec3( 64, 64, 128 );

LightGrid::LightGrid()
{
	lightGridSize = defaultLightGridSize;
	area = -1;

	irradianceImage = NULL;
	imageSingleProbeSize = LIGHTGRID_IRRADIANCE_SIZE;
	imageBorderSize = LIGHTGRID_IRRADIANCE_BORDER_SIZE;
}

void LightGrid::SetupLightGrid( const idBounds& bounds, const char* mapName, const idRenderWorld* world, const idVec3& gridSize, int _area, int numAreas, int maxProbes, bool printToConsole )
{
	//idLib::Printf( "----- SetupLightGrid -----\n" );

	lightGridSize = gridSize;
	lightGridPoints.Clear();

	area = _area;

	imageSingleProbeSize = LIGHTGRID_IRRADIANCE_SIZE;
	imageBorderSize = LIGHTGRID_IRRADIANCE_BORDER_SIZE;

	idVec3 maxs;
	int j = 0;

	int maxGridPoints = MAX_AREA_LIGHTGRID_POINTS;
	if( maxProbes >= 100 )// && maxProbes < MAX_AREA_LIGHTGRID_POINTS )
	{
		maxGridPoints = maxProbes;
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

		if( printToConsole )
		{
			idLib::Printf( "\narea %i of %i (%i x %i x %i) = %i grid points \n", area, numAreas, lightGridBounds[0], lightGridBounds[1], lightGridBounds[2], numGridPoints );
			idLib::Printf( "area %i grid size (%i %i %i)\n", area, ( int )lightGridSize[0], ( int )lightGridSize[1], ( int )lightGridSize[2] );
			idLib::Printf( "area %i grid bounds (%i %i %i)\n", area, ( int )lightGridBounds[0], ( int )lightGridBounds[1], ( int )lightGridBounds[2] );
			//idLib::Printf( "area %i %9u x %" PRIuSIZE " = lightGridSize = (%.2fMB)\n", area, numGridPoints, sizeof( lightGridPoint_t ), ( float )( lightGridPoints.MemoryUsed() ) / ( 1024.0f * 1024.0f ) );
		}

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
		LoadLightGridImages();
	}
	else
	{
		int totalGridPoints = 0;

		for( int i = 0; i < numPortalAreas; i++ )
		{
			portalArea_t* area = &portalAreas[i];

			area->lightGrid.SetupLightGrid( area->globalBounds, mapName, this, defaultLightGridSize, i, numPortalAreas, -1, true );

			totalGridPoints += area->lightGrid.CountValidGridPoints();
		}

		idLib::Printf( "----------------------------------\n" );
		idLib::Printf( "Total valid light grid points %i\n", totalGridPoints );
	}
}

void idRenderWorldLocal::LoadLightGridImages()
{
	idLib::Printf( "----- LoadLightGridImages -----\n" );

	idStrStatic< MAX_OSPATH > baseName = mapName;
	baseName.StripFileExtension();

	idStr filename;

#if defined( USE_NVRHI )
	nvrhi::CommandListHandle commandList = deviceManager->GetDevice()->createCommandList();

	commandList->open();
#endif

	// try to load existing lightgrid image data
	for( int i = 0; i < numPortalAreas; i++ )
	{
		portalArea_t* area = &portalAreas[i];

		if( !area->lightGrid.irradianceImage )
		{
			filename.Format( "env/%s/area%i_lightgrid_amb", baseName.c_str(), i );
			area->lightGrid.irradianceImage = globalImages->ImageFromFile( filename, TF_LINEAR, TR_CLAMP, TD_R11G11B10F, CF_2D );
		}
		else
		{
			area->lightGrid.irradianceImage->Reload( true, commandList );
		}
	}

#if defined( USE_NVRHI )
	commandList->close();

	deviceManager->GetDevice()->executeCommandList( commandList );
#endif
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
	fp->WriteFloatString( "%s \"%i\"\n\n", LGRID_FILEID, BLGRID_VERSION );

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
	fp->WriteFloatString( "lightGridPoints { /* area = */ %i /* numLightGridPoints = */ %i /* imageSingleProbeSize = */ %i /* imageBorderSize = */ %i \n", lightGrid.area, lightGrid.lightGridPoints.Num(), lightGrid.imageSingleProbeSize, lightGrid.imageBorderSize );

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
	ID_TIME_T sourceTimeStamp = fileSystem->GetTimestamp( fileName );

	// see if we have a generated version of this
	bool loaded = false;

#if 1
	idFileLocal file( fileSystem->OpenFileReadMemory( generatedFileName ) );
	if( file != NULL )
	{
		int numEntries = 0;
		int magic = 0;
		ID_TIME_T storedTimeStamp;
		file->ReadBig( magic );
		file->ReadBig( storedTimeStamp );
		file->ReadBig( numEntries );
		if( ( magic == BLGRID_MAGIC ) && ( sourceTimeStamp == storedTimeStamp ) && ( numEntries > 0 ) )
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
			int magic = BLGRID_MAGIC;
			outputFile->WriteBig( magic );
			outputFile->WriteBig( sourceTimeStamp );
			outputFile->WriteBig( numEntries );
		}

		if( !src->ExpectTokenString( LGRID_FILEID ) )
		{
			common->Warning( "%s is not an CM file.", fileName.c_str() );
			delete src;
			return false;
		}

		int lightGridVersion = 0;
		if( !src->ReadToken( &token ) )
		{
			src->Error( "couldn't read expected integer" );
			delete src;
			return false;
		}
		if( token.type == TT_PUNCTUATION && token == "-" )
		{
			src->ExpectTokenType( TT_NUMBER, TT_INTEGER, &token );
			lightGridVersion = -( ( signed int ) token.GetIntValue() );
		}
		else if( token.type != TT_NUMBER && token.subtype != TT_STRING && token.subtype != TT_NAME )
		{
			src->Error( "expected integer value or string, found '%s'", token.c_str() );
		}

		if( token.type == TT_NUMBER )
		{
			lightGridVersion = token.GetIntValue();
		}
		else if( token.type == TT_STRING || token.subtype == TT_NAME )
		{
			lightGridVersion = atoi( token );
		}

		if( lightGridVersion != LGRID_VERSION && lightGridVersion != 4 )
		{
			common->Warning( "%s has version %i instead of %i", fileName.c_str(), lightGridVersion, LGRID_VERSION );
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

			int magic = BLGRID_MAGIC;
			outputFile->WriteBig( magic );
			outputFile->WriteBig( sourceTimeStamp );
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

	int imageProbeSize = src->ParseInt();
	if( imageProbeSize < 8 )
	{
		src->Error( "ParseLightGridPoints: bad single image probe size %i", imageProbeSize );
		return;
	}

	int imageBorderSize = src->ParseInt();
	if( imageBorderSize < 0 )
	{
		src->Error( "ParseLightGridPoints: bad probe border size %i", imageBorderSize );
		return;
	}

	if( fileOut != NULL )
	{
		// write out the type so the binary reader knows what to instantiate
		fileOut->WriteString( "lightGridPoints" );
	}

	portalArea_t* area = &portalAreas[areaIndex];
	area->lightGrid.area = areaIndex;

	area->lightGrid.imageSingleProbeSize = imageProbeSize;
	area->lightGrid.imageBorderSize = imageBorderSize;

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
	//idLib::Printf( "area %i %9u x %" PRIuSIZE " = lightGridSize = (%.2fMB)\n", areaIndex, numLightGridPoints, sizeof( lightGridPoint_t ), ( float )( area->lightGrid.lightGridPoints.MemoryUsed() ) / ( 1024.0f * 1024.0f ) );
	idLib::Printf( "area %i probe size (%i %i)\n", areaIndex, imageProbeSize, imageBorderSize );

	if( fileOut != NULL )
	{
		fileOut->WriteBig( areaIndex );
		fileOut->WriteBig( numLightGridPoints );
		fileOut->WriteBig( imageProbeSize );
		fileOut->WriteBig( imageBorderSize );
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
		src->Parse1DMatrix( shSize( 4 ) * 3, gridPoint->shRadiance[0].ToFloatPtr() );
#endif

		if( fileOut != NULL )
		{
			fileOut->WriteBig( gridPoint->valid );
			fileOut->WriteBig( gridPoint->origin );

#if STORE_LIGHTGRID_SHDATA
			fileOut->WriteBigArray( gridPoint->shRadiance[0].ToFloatPtr(), shSize( 4 ) * 3 );
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

	int imageProbeSize = 0;
	file->ReadBig( imageProbeSize );
	if( imageProbeSize < 0 )
	{
		idLib::Error( "ReadBinaryLightGridPoints: bad imageProbeSize %i", imageProbeSize );
		return;
	}

	int imageBorderSize = 0;
	file->ReadBig( imageBorderSize );
	if( imageBorderSize < 0 )
	{
		idLib::Error( "ReadBinaryLightGridPoints: bad imageBorderSize %i", imageBorderSize );
		return;
	}

	portalArea_t* area = &portalAreas[areaIndex];
	area->lightGrid.area = areaIndex;

	area->lightGrid.imageSingleProbeSize = imageProbeSize;
	area->lightGrid.imageBorderSize = imageBorderSize;

	// gridMins
	file->ReadBig( area->lightGrid.lightGridOrigin );
	file->ReadBig( area->lightGrid.lightGridSize );
	file->ReadBigArray( area->lightGrid.lightGridBounds, 3 );

	area->lightGrid.lightGridPoints.SetNum( numLightGridPoints );

	idLib::Printf( "\narea %i (%i x %i x %i) = %i grid points \n", areaIndex, area->lightGrid.lightGridBounds[0], area->lightGrid.lightGridBounds[1], area->lightGrid.lightGridBounds[2], numLightGridPoints );
	idLib::Printf( "area %i grid size (%i %i %i)\n", areaIndex, ( int )area->lightGrid.lightGridSize[0], ( int )area->lightGrid.lightGridSize[1], ( int )area->lightGrid.lightGridSize[2] );
	idLib::Printf( "area %i grid bounds (%i %i %i)\n", areaIndex, ( int )area->lightGrid.lightGridBounds[0], ( int )area->lightGrid.lightGridBounds[1], ( int )area->lightGrid.lightGridBounds[2] );
	//idLib::Printf( "area %i %9u x %" PRIuSIZE " = lightGridSize = (%.2fMB)\n", areaIndex, numLightGridPoints, sizeof( lightGridPoint_t ), ( float )( area->lightGrid.lightGridPoints.MemoryUsed() ) / ( 1024.0f * 1024.0f ) );
	idLib::Printf( "area %i probe size (%i %i)\n", areaIndex, imageProbeSize, imageBorderSize );

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

	const float invDstSize = 1.0f / float( ENVPROBE_CAPTURE_SIZE );
	const idVec2i sourceImageSize( ENVPROBE_CAPTURE_SIZE, ENVPROBE_CAPTURE_SIZE );

	// build L4 Spherical Harmonics from source image
	SphericalHarmonicsT<idVec3, 4> shRadiance;

	for( int i = 0; i < shSize( 4 ); i++ )
	{
		shRadiance[i].Zero();
	}

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

#if STORE_LIGHTGRID_SHDATA
	for( int i = 0; i < shSize( 4 ); i++ )
	{
		parms->shRadiance[i] = shRadiance[i];
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

	for( int x = 0; x < parms->outWidth; x++ )
	{
		for( int y = 0; y < parms->outHeight; y++ )
		{
			idVec2 octCoord = NormalizedOctCoord( x, y, parms->outWidth );

			// convert UV coord to 3D direction
			idVec3 dir;

			dir.FromOctahedral( octCoord );

			idVec3 outColor( 0, 0, 0 );

			// generate ambient colors by evaluating the L4 Spherical Harmonics
			SphericalHarmonicsT<float, 4> shDirection = shEvaluate<4>( dir );

			idVec3 sampleIrradianceSh = shEvaluateDiffuse<idVec3, 4>( shRadiance, dir ) / idMath::PI;

			outColor[0] = Max( 0.0f, sampleIrradianceSh.x );
			outColor[1] = Max( 0.0f, sampleIrradianceSh.y );
			outColor[2] = Max( 0.0f, sampleIrradianceSh.z );

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



CONSOLE_COMMAND( bakeLightGrids, "Bake irradiance/vis light grid data", NULL )
{
	idStr			baseName;
	idStr			filename;
	renderView_t	ref;
	int				captureSize;

	int limit = MAX_AREA_LIGHTGRID_POINTS;
	int bounces = 1;
	idVec3 gridSize = defaultLightGridSize;

	bool helpRequested = false;
	idStr option;

	for( int i = 1; i < args.Argc(); i++ )
	{
		option = args.Argv( i );
		option.StripLeading( '-' );

		if( option.IcmpPrefix( "limit" ) == 0 )
		{
			option.StripLeading( "limit" );
			limit = atoi( option );
			limit = Max( 1000, limit );
		}
		else if( option.IcmpPrefix( "bounce" ) == 0 )
		{
			option.StripLeading( "bounce" );
			bounces = atoi( option );
			bounces = Max( 1, bounces );
		}
		else if( option.IcmpPrefix( "grid" ) == 0 )
		{
			if( ( i + 5 ) < args.Argc() )
			{
				// skip res
				i++;

				// skip (
				i++;

				gridSize[0] = atoi( args.Argv( i++ ) );
				gridSize[1] = atoi( args.Argv( i++ ) );
				gridSize[2] = atoi( args.Argv( i++ ) );

				// skip )
				i++;
				continue;
			}
		}
		else if( option.Icmp( "h" ) == 0 || option.Icmp( "help" ) == 0 )
		{
			helpRequested = true;
			break;
		}
	}

	if( helpRequested )
	{
		idLib::Printf( "USAGE: bakeLightGrids [<switches>...]\n\n" );
		idLib::Printf( "<Switches>\n" );
		idLib::Printf( " limit[num] : max probes per BSP area (default %i)\n", MAX_AREA_LIGHTGRID_POINTS );
		idLib::Printf( " bounce[num] : number of bounces or number of light reuse (default 1)\n" );
		idLib::Printf( " grid( xdim ydim zdim ) : light grid size steps into each direction (default 64 64 128)\n" );

		return;
	}

	if( !tr.primaryWorld )
	{
		idLib::Printf( "No primary world loaded.\n" );
		return;
	}

	if( !tr.primaryView )
	{
		idLib::Printf( "No primary view.\n" );
		return;
	}

	int sysWidth = renderSystem->GetWidth();
	int sysHeight = renderSystem->GetHeight();

	bool useThreads = true;

	baseName = tr.primaryWorld->mapName;
	baseName.StripFileExtension();

	captureSize = ENVPROBE_CAPTURE_SIZE;

	idLib::Printf( "Using limit = %i\n", limit );
	idLib::Printf( "Using bounces = %i\n", bounces );
	idLib::Printf( "Preferred lightGridSize (%i %i %i)\n", ( int )gridSize[0], ( int )gridSize[1], ( int )gridSize[2] );

	const viewDef_t primary = *tr.primaryView;

	int totalProcessedAreas = 0;
	int totalProcessedProbes = 0;
	int	totalStart = Sys_Milliseconds();

	// estimate time for bake so the user can quit and adjust parameters if needed
	for( int bounce = 0; bounce < bounces; bounce++ )
	{
		for( int a = 0; a < tr.primaryWorld->NumAreas(); a++ )
		{
			portalArea_t* area = &tr.primaryWorld->portalAreas[a];

			area->lightGrid.SetupLightGrid( area->globalBounds, tr.primaryWorld->mapName, tr.primaryWorld, gridSize, a, tr.primaryWorld->NumAreas(), limit, false );

			int numGridPoints = area->lightGrid.CountValidGridPoints();
			if( numGridPoints == 0 )
			{
				continue;
			}

			if( bounce == 0 )
			{
				totalProcessedAreas++;
				totalProcessedProbes += numGridPoints;
			}
		}
	}

	idLib::Printf( "----------------------------------\n" );
	idLib::Printf( "Processing %i light probes in %i areas for %i bounces\n", totalProcessedProbes, totalProcessedAreas, bounces );
	//common->Printf( "ETA %5.1f minutes\n\n", ( totalEnd - totalStart ) / ( 1000.0f * 60 ) );

	for( int bounce = 0; bounce < bounces; bounce++ )
	{
		for( int a = 0; a < tr.primaryWorld->NumAreas(); a++ )
		{
			portalArea_t* area = &tr.primaryWorld->portalAreas[a];

			area->lightGrid.SetupLightGrid( area->globalBounds, tr.primaryWorld->mapName, tr.primaryWorld, gridSize, a, tr.primaryWorld->NumAreas(), limit, true );

			int numGridPoints = area->lightGrid.CountValidGridPoints();
			if( numGridPoints == 0 )
			{
				continue;
			}

			idLib::Printf( "Shooting %i grid probes in area %i...\n", numGridPoints, a );

			CommandlineProgressBar progressBar( numGridPoints, sysWidth, sysHeight );
			progressBar.Start();

			int	start = Sys_Milliseconds();

			int gridStep[3];

			gridStep[0] = 1;
			gridStep[1] = area->lightGrid.lightGridBounds[0];
			gridStep[2] = area->lightGrid.lightGridBounds[0] * area->lightGrid.lightGridBounds[1];

			int gridCoord[3];

			//--------------------------------------------
			// CAPTURE SCENE LIGHTING TO CUBEMAPS
			//--------------------------------------------

			// make sure the game / draw thread has completed
			commonLocal.WaitGameThread();

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
							//progressBar.Increment();
							continue;
						}

						calcLightGridPointParms_t* jobParms = new calcLightGridPointParms_t;
						jobParms->gridCoord[0] = i;
						jobParms->gridCoord[1] = j;
						jobParms->gridCoord[2] = k;

						for( int side = 0; side < 6; side++ )
						{
							ref = primary.renderView;

							ref.rdflags = RDF_IRRADIANCE;
							if( bounce == 0 )
							{
								ref.rdflags |= RDF_NOAMBIENT;
							}

							ref.fov_x = ref.fov_y = 90;

							ref.vieworg = gridPoint->origin;
							ref.viewaxis = tr.cubeAxis[ side ];

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
							// make sure that all frames have finished rendering
							//deviceManager->GetDevice()->waitForIdle();

							byte* floatRGB16F = NULL;

#if 0
							// this probe fails in game/admin.map
							if( a == 5 && tr.lightGridJobs.Num() == 17 && side == 4 )
							{
								idLib::Printf( "debugging shitty capture\n" );
							}
#endif
							//bool validCapture =
							R_ReadPixelsRGB16F( deviceManager->GetDevice(), &tr.backend.GetCommonPasses(), globalImages->envprobeHDRImage->GetTextureHandle() , nvrhi::ResourceStates::RenderTarget, &floatRGB16F, captureSize, captureSize );

							// release all in-flight references to the render targets
							//deviceManager->GetDevice()->runGarbageCollection();
#if 0
							if( !validCapture )
							{
								idStr testName;
								testName.Format( "env/test/area%i_envprobe_%i_side_%i.exr", a, tr.lightGridJobs.Num(), side );
								R_WriteEXR( testName, floatRGB16F, 3, captureSize, captureSize, "fs_basepath" );
							}
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
							jobParms->radiance[ side ] = floatRGB16F;
						}

						tr.lightGridJobs.Append( jobParms );


						tr.takingEnvprobe = false;
						progressBar.Increment( true );
						tr.takingEnvprobe = true;
					}
				}
			}

			int	end = Sys_Milliseconds();

			tr.takingEnvprobe = false;

			r_useScissor.SetBool( true );
			r_useParallelAddModels.SetBool( true );
			r_useParallelAddShadows.SetBool( true );
			r_useParallelAddLights.SetBool( true );

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
					progressBar.Increment( true );
				}
			}

			if( useThreads )
			{
				idLib::Printf( "Processing probes on all available cores... Please wait.\n" );
				common->UpdateScreen( false );
				common->UpdateScreen( false );

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

				// backup SH L4 data
#if STORE_LIGHTGRID_SHDATA
				lightGridPoint_t* gridPoint = &area->lightGrid.lightGridPoints[ job->gridCoord[0] * gridStep[0] + job->gridCoord[1] * gridStep[1] + job->gridCoord[2] * gridStep[2] ];
				for( int i = 0; i < shSize( 4 ); i++ )
				{
					gridPoint->shRadiance[i] = job->shRadiance[i];
				}
#endif

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
		}

		tr.primaryWorld->LoadLightGridImages();
	}

	int totalEnd = Sys_Milliseconds();

	// everything went ok so let's save the configurations to disc
	// so we can load the texture atlases with the correct subdivisions next time
	filename.Format( "%s.lightgrid", baseName.c_str() );
	tr.primaryWorld->WriteLightGridsToFile( filename );

	idLib::Printf( "----------------------------------\n" );
	idLib::Printf( "Processed %i light probes in %i areas\n", totalProcessedProbes, totalProcessedAreas );
	common->Printf( "Baked light grid irradiance in %5.1f minutes\n\n", ( totalEnd - totalStart ) / ( 1000.0f * 60 ) );
}

