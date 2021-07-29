/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.

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

#include "AASFile.h"
#include "AASFile_local.h"

/*
===============================================================================

	idReachability

===============================================================================
*/

/*
================
Reachability_Write
================
*/
bool Reachability_Write( idFile* fp, idReachability* reach )
{
	fp->WriteFloatString( "\t\t%d %d (%f %f %f) (%f %f %f) %d %d",
						  ( int ) reach->travelType, ( int ) reach->toAreaNum, reach->start.x, reach->start.y, reach->start.z,
						  reach->end.x, reach->end.y, reach->end.z, reach->edgeNum, ( int ) reach->travelTime );
	return true;
}

/*
================
Reachability_Read
================
*/
bool Reachability_Read( idLexer& src, idReachability* reach )
{
	reach->travelType = src.ParseInt();
	reach->toAreaNum = src.ParseInt();
	src.Parse1DMatrix( 3, reach->start.ToFloatPtr() );
	src.Parse1DMatrix( 3, reach->end.ToFloatPtr() );
	reach->edgeNum = src.ParseInt();
	reach->travelTime = src.ParseInt();
	return true;
}

/*
================
idReachability::CopyBase
================
*/
void idReachability::CopyBase( idReachability& reach )
{
	travelType = reach.travelType;
	toAreaNum = reach.toAreaNum;
	start = reach.start;
	end = reach.end;
	edgeNum = reach.edgeNum;
	travelTime = reach.travelTime;
}


/*
===============================================================================

	idReachability_Special

===============================================================================
*/

/*
================
Reachability_Special_Write
================
*/
bool Reachability_Special_Write( idFile* fp, idReachability_Special* reach )
{
	int i;
	const idKeyValue* keyValue;

	fp->WriteFloatString( "\n\t\t{\n" );
	for( i = 0; i < reach->dict.GetNumKeyVals(); i++ )
	{
		keyValue = reach->dict.GetKeyVal( i );
		fp->WriteFloatString( "\t\t\t\"%s\" \"%s\"\n", keyValue->GetKey().c_str(), keyValue->GetValue().c_str() );
	}
	fp->WriteFloatString( "\t\t}\n" );

	return true;
}

/*
================
Reachability_Special_Read
================
*/
bool Reachability_Special_Read( idLexer& src, idReachability_Special* reach )
{
	idToken key, value;

	src.ExpectTokenString( "{" );
	while( src.ReadToken( &key ) )
	{
		if( key == "}" )
		{
			return true;
		}
		src.ExpectTokenType( TT_STRING, 0, &value );
		reach->dict.Set( key, value );
	}
	return false;
}

/*
===============================================================================

	idAASSettings

===============================================================================
*/

/*
============
idAASSettings::idAASSettings
============
*/
idAASSettings::idAASSettings()
{
	numBoundingBoxes = 1;
	boundingBoxes[0] = idBounds( idVec3( -16, -16, 0 ), idVec3( 16, 16, 72 ) );
	usePatches = false;
	writeBrushMap = false;
	playerFlood = false;
	noOptimize = false;
	allowSwimReachabilities = false;
	allowFlyReachabilities = false;
	fileExtension = "aas48";
	// physics settings
	gravity = idVec3( 0, 0, -1066 );
	gravityDir = gravity;
	gravityValue = gravityDir.Normalize();
	invGravityDir = -gravityDir;
	maxStepHeight = 14.0f;
	maxBarrierHeight = 32.0f;
	maxWaterJumpHeight = 20.0f;
	maxFallHeight = 64.0f;
	minFloorCos = 0.7f;
	// fixed travel times
	tt_barrierJump = 100;
	tt_startCrouching = 100;
	tt_waterJump = 100;
	tt_startWalkOffLedge = 100;
}

/*
============
idAASSettings::ParseBool
============
*/
bool idAASSettings::ParseBool( idLexer& src, bool& b )
{
	if( !src.ExpectTokenString( "=" ) )
	{
		return false;
	}
	b = src.ParseBool();
	return true;
}

/*
============
idAASSettings::ParseInt
============
*/
bool idAASSettings::ParseInt( idLexer& src, int& i )
{
	if( !src.ExpectTokenString( "=" ) )
	{
		return false;
	}
	i = src.ParseInt();
	return true;
}

/*
============
idAASSettings::ParseFloat
============
*/
bool idAASSettings::ParseFloat( idLexer& src, float& f )
{
	if( !src.ExpectTokenString( "=" ) )
	{
		return false;
	}
	f = src.ParseFloat();
	return true;
}

/*
============
idAASSettings::ParseVector
============
*/
bool idAASSettings::ParseVector( idLexer& src, idVec3& vec )
{
	if( !src.ExpectTokenString( "=" ) )
	{
		return false;
	}
	return ( src.Parse1DMatrix( 3, vec.ToFloatPtr() ) != 0 );
}

/*
============
idAASSettings::ParseBBoxes
============
*/
bool idAASSettings::ParseBBoxes( idLexer& src )
{
	idToken token;
	idBounds bounds;

	numBoundingBoxes = 0;

	if( !src.ExpectTokenString( "{" ) )
	{
		return false;
	}
	while( src.ReadToken( &token ) )
	{
		if( token == "}" )
		{
			return true;
		}
		src.UnreadToken( &token );
		src.Parse1DMatrix( 3, bounds[0].ToFloatPtr() );
		if( !src.ExpectTokenString( "-" ) )
		{
			return false;
		}
		src.Parse1DMatrix( 3, bounds[1].ToFloatPtr() );

		boundingBoxes[numBoundingBoxes++] = bounds;
	}
	return false;
}

/*
============
idAASSettings::FromParser
============
*/
bool idAASSettings::FromParser( idLexer& src )
{
	idToken token;

	if( !src.ExpectTokenString( "{" ) )
	{
		return false;
	}

	// parse the file
	while( 1 )
	{
		if( !src.ReadToken( &token ) )
		{
			break;
		}

		if( token == "}" )
		{
			break;
		}

		if( token == "bboxes" )
		{
			if( !ParseBBoxes( src ) )
			{
				return false;
			}
		}
		else if( token == "usePatches" )
		{
			if( !ParseBool( src, usePatches ) )
			{
				return false;
			}
		}
		else if( token == "writeBrushMap" )
		{
			if( !ParseBool( src, writeBrushMap ) )
			{
				return false;
			}
		}
		else if( token == "playerFlood" )
		{
			if( !ParseBool( src, playerFlood ) )
			{
				return false;
			}
		}
		else if( token == "allowSwimReachabilities" )
		{
			if( !ParseBool( src, allowSwimReachabilities ) )
			{
				return false;
			}
		}
		else if( token == "allowFlyReachabilities" )
		{
			if( !ParseBool( src, allowFlyReachabilities ) )
			{
				return false;
			}
		}
		else if( token == "fileExtension" )
		{
			src.ExpectTokenString( "=" );
			src.ExpectTokenType( TT_STRING, 0, &token );
			fileExtension = token;
		}
		else if( token == "gravity" )
		{
			ParseVector( src, gravity );
			gravityDir = gravity;
			gravityValue = gravityDir.Normalize();
			invGravityDir = -gravityDir;
		}
		else if( token == "maxStepHeight" )
		{
			if( !ParseFloat( src, maxStepHeight ) )
			{
				return false;
			}
		}
		else if( token == "maxBarrierHeight" )
		{
			if( !ParseFloat( src, maxBarrierHeight ) )
			{
				return false;
			}
		}
		else if( token == "maxWaterJumpHeight" )
		{
			if( !ParseFloat( src, maxWaterJumpHeight ) )
			{
				return false;
			}
		}
		else if( token == "maxFallHeight" )
		{
			if( !ParseFloat( src, maxFallHeight ) )
			{
				return false;
			}
		}
		else if( token == "minFloorCos" )
		{
			if( !ParseFloat( src, minFloorCos ) )
			{
				return false;
			}
		}
		else if( token == "tt_barrierJump" )
		{
			if( !ParseInt( src, tt_barrierJump ) )
			{
				return false;
			}
		}
		else if( token == "tt_startCrouching" )
		{
			if( !ParseInt( src, tt_startCrouching ) )
			{
				return false;
			}
		}
		else if( token == "tt_waterJump" )
		{
			if( !ParseInt( src, tt_waterJump ) )
			{
				return false;
			}
		}
		else if( token == "tt_startWalkOffLedge" )
		{
			if( !ParseInt( src, tt_startWalkOffLedge ) )
			{
				return false;
			}
		}
		else
		{
			src.Error( "invalid token '%s'", token.c_str() );
		}
	}

	if( numBoundingBoxes <= 0 )
	{
		src.Error( "no valid bounding box" );
	}

	return true;
}

/*
============
idAASSettings::FromFile
============
*/
bool idAASSettings::FromFile( const idStr& fileName )
{
	idLexer src( LEXFL_ALLOWPATHNAMES | LEXFL_NOSTRINGESCAPECHARS | LEXFL_NOSTRINGCONCAT );
	idStr name;

	name = fileName;

	common->Printf( "loading %s\n", name.c_str() );

	if( !src.LoadFile( name ) )
	{
		common->Error( "WARNING: couldn't load %s\n", name.c_str() );
		return false;
	}

	if( !src.ExpectTokenString( "settings" ) )
	{
		common->Error( "%s is not a settings file", name.c_str() );
		return false;
	}

	if( !FromParser( src ) )
	{
		common->Error( "failed to parse %s", name.c_str() );
		return false;
	}

	return true;
}

/*
============
idAASSettings::FromDict
============
*/
bool idAASSettings::FromDict( const char* name, const idDict* dict )
{
	idBounds bounds;

	if( !dict->GetVector( "mins", "0 0 0", bounds[ 0 ] ) )
	{
		common->Error( "Missing 'mins' in entityDef '%s'", name );
	}
	if( !dict->GetVector( "maxs", "0 0 0", bounds[ 1 ] ) )
	{
		common->Error( "Missing 'maxs' in entityDef '%s'", name );
	}

	numBoundingBoxes = 1;
	boundingBoxes[0] = bounds;

	if( !dict->GetBool( "usePatches", "0", usePatches ) )
	{
		common->Error( "Missing 'usePatches' in entityDef '%s'", name );
	}

	if( !dict->GetBool( "writeBrushMap", "0", writeBrushMap ) )
	{
		common->Error( "Missing 'writeBrushMap' in entityDef '%s'", name );
	}

	if( !dict->GetBool( "playerFlood", "0", playerFlood ) )
	{
		common->Error( "Missing 'playerFlood' in entityDef '%s'", name );
	}

	if( !dict->GetBool( "allowSwimReachabilities", "0", allowSwimReachabilities ) )
	{
		common->Error( "Missing 'allowSwimReachabilities' in entityDef '%s'", name );
	}

	if( !dict->GetBool( "allowFlyReachabilities", "0", allowFlyReachabilities ) )
	{
		common->Error( "Missing 'allowFlyReachabilities' in entityDef '%s'", name );
	}

	if( !dict->GetString( "fileExtension", "", fileExtension ) )
	{
		common->Error( "Missing 'fileExtension' in entityDef '%s'", name );
	}

	if( !dict->GetVector( "gravity", "0 0 -1066", gravity ) )
	{
		common->Error( "Missing 'gravity' in entityDef '%s'", name );
	}
	gravityDir = gravity;
	gravityValue = gravityDir.Normalize();
	invGravityDir = -gravityDir;

	if( !dict->GetFloat( "maxStepHeight", "0", maxStepHeight ) )
	{
		common->Error( "Missing 'maxStepHeight' in entityDef '%s'", name );
	}

	if( !dict->GetFloat( "maxBarrierHeight", "0", maxBarrierHeight ) )
	{
		common->Error( "Missing 'maxBarrierHeight' in entityDef '%s'", name );
	}

	if( !dict->GetFloat( "maxWaterJumpHeight", "0", maxWaterJumpHeight ) )
	{
		common->Error( "Missing 'maxWaterJumpHeight' in entityDef '%s'", name );
	}

	if( !dict->GetFloat( "maxFallHeight", "0", maxFallHeight ) )
	{
		common->Error( "Missing 'maxFallHeight' in entityDef '%s'", name );
	}

	if( !dict->GetFloat( "minFloorCos", "0", minFloorCos ) )
	{
		common->Error( "Missing 'minFloorCos' in entityDef '%s'", name );
	}

	if( !dict->GetInt( "tt_barrierJump", "0", tt_barrierJump ) )
	{
		common->Error( "Missing 'tt_barrierJump' in entityDef '%s'", name );
	}

	if( !dict->GetInt( "tt_startCrouching", "0", tt_startCrouching ) )
	{
		common->Error( "Missing 'tt_startCrouching' in entityDef '%s'", name );
	}

	if( !dict->GetInt( "tt_waterJump", "0", tt_waterJump ) )
	{
		common->Error( "Missing 'tt_waterJump' in entityDef '%s'", name );
	}

	if( !dict->GetInt( "tt_startWalkOffLedge", "0", tt_startWalkOffLedge ) )
	{
		common->Error( "Missing 'tt_startWalkOffLedge' in entityDef '%s'", name );
	}

	return true;
}


/*
============
idAASSettings::WriteToFile
============
*/
bool idAASSettings::WriteToFile( idFile* fp ) const
{
	int i;

	fp->WriteFloatString( "{\n" );
	fp->WriteFloatString( "\tbboxes\n\t{\n" );
	for( i = 0; i < numBoundingBoxes; i++ )
	{
		fp->WriteFloatString( "\t\t(%f %f %f)-(%f %f %f)\n", boundingBoxes[i][0].x, boundingBoxes[i][0].y,
							  boundingBoxes[i][0].z, boundingBoxes[i][1].x, boundingBoxes[i][1].y, boundingBoxes[i][1].z );
	}
	fp->WriteFloatString( "\t}\n" );
	fp->WriteFloatString( "\tusePatches = %d\n", usePatches );
	fp->WriteFloatString( "\twriteBrushMap = %d\n", writeBrushMap );
	fp->WriteFloatString( "\tplayerFlood = %d\n", playerFlood );
	fp->WriteFloatString( "\tallowSwimReachabilities = %d\n", allowSwimReachabilities );
	fp->WriteFloatString( "\tallowFlyReachabilities = %d\n", allowFlyReachabilities );
	fp->WriteFloatString( "\tfileExtension = \"%s\"\n", fileExtension.c_str() );
	fp->WriteFloatString( "\tgravity = (%f %f %f)\n", gravity.x, gravity.y, gravity.z );
	fp->WriteFloatString( "\tmaxStepHeight = %f\n", maxStepHeight );
	fp->WriteFloatString( "\tmaxBarrierHeight = %f\n", maxBarrierHeight );
	fp->WriteFloatString( "\tmaxWaterJumpHeight = %f\n", maxWaterJumpHeight );
	fp->WriteFloatString( "\tmaxFallHeight = %f\n", maxFallHeight );
	fp->WriteFloatString( "\tminFloorCos = %f\n", minFloorCos );
	fp->WriteFloatString( "\ttt_barrierJump = %d\n", tt_barrierJump );
	fp->WriteFloatString( "\ttt_startCrouching = %d\n", tt_startCrouching );
	fp->WriteFloatString( "\ttt_waterJump = %d\n", tt_waterJump );
	fp->WriteFloatString( "\ttt_startWalkOffLedge = %d\n", tt_startWalkOffLedge );
	fp->WriteFloatString( "}\n" );
	return true;
}

/*
============
idAASSettings::ValidForBounds
============
*/
bool idAASSettings::ValidForBounds( const idBounds& bounds ) const
{
	int i;

	for( i = 0; i < 3; i++ )
	{
		if( bounds[0][i] < boundingBoxes[0][0][i] )
		{
			return false;
		}
		if( bounds[1][i] > boundingBoxes[0][1][i] )
		{
			return false;
		}
	}
	return true;
}

/*
============
idAASSettings::ValidEntity
============
*/
bool idAASSettings::ValidEntity( const char* classname ) const
{
	idStr			use_aas;
	idVec3			size;
	idBounds		bounds;

	if( playerFlood )
	{
		if( !strcmp( classname, "info_player_start" ) || !strcmp( classname , "info_player_deathmatch" ) || !strcmp( classname, "func_teleporter" ) )
		{
			return true;
		}
	}

	const idDeclEntityDef* decl = static_cast<const idDeclEntityDef*>( declManager->FindType( DECL_ENTITYDEF, classname, false ) );
	if( ( decl != NULL ) && decl->dict.GetString( "use_aas", NULL, use_aas ) && !fileExtension.Icmp( use_aas ) )
	{
		if( decl->dict.GetVector( "mins", NULL, bounds[0] ) )
		{
			decl->dict.GetVector( "maxs", NULL, bounds[1] );
		}
		else if( decl->dict.GetVector( "size", NULL, size ) )
		{
			bounds[ 0 ].Set( size.x * -0.5f, size.y * -0.5f, 0.0f );
			bounds[ 1 ].Set( size.x * 0.5f, size.y * 0.5f, size.z );
		}

		if( !ValidForBounds( bounds ) )
		{
			common->Error( "%s cannot use %s\n", classname, fileExtension.c_str() );
		}

		return true;
	}

	return false;
}


/*
===============================================================================

	idAASFileLocal

===============================================================================
*/

#define AAS_LIST_GRANULARITY	1024
#define AAS_INDEX_GRANULARITY	4096
#define AAS_PLANE_GRANULARITY	4096
#define AAS_VERTEX_GRANULARITY	4096
#define AAS_EDGE_GRANULARITY	4096

/*
================
idAASFileLocal::idAASFileLocal
================
*/
idAASFileLocal::idAASFileLocal()
{
	planeList.SetGranularity( AAS_PLANE_GRANULARITY );
	vertices.SetGranularity( AAS_VERTEX_GRANULARITY );
	edges.SetGranularity( AAS_EDGE_GRANULARITY );
	edgeIndex.SetGranularity( AAS_INDEX_GRANULARITY );
	faces.SetGranularity( AAS_LIST_GRANULARITY );
	faceIndex.SetGranularity( AAS_INDEX_GRANULARITY );
	areas.SetGranularity( AAS_LIST_GRANULARITY );
	nodes.SetGranularity( AAS_LIST_GRANULARITY );
	portals.SetGranularity( AAS_LIST_GRANULARITY );
	portalIndex.SetGranularity( AAS_INDEX_GRANULARITY );
	clusters.SetGranularity( AAS_LIST_GRANULARITY );
}

/*
================
idAASFileLocal::~idAASFileLocal
================
*/
idAASFileLocal::~idAASFileLocal()
{
	int i;
	idReachability* reach, *next;

	for( i = 0; i < areas.Num(); i++ )
	{
		for( reach = areas[i].reach; reach; reach = next )
		{
			next = reach->next;
			delete reach;
		}
	}
}

/*
================
idAASFileLocal::Clear
================
*/
void idAASFileLocal::Clear()
{
	planeList.Clear();
	vertices.Clear();
	edges.Clear();
	edgeIndex.Clear();
	faces.Clear();
	faceIndex.Clear();
	areas.Clear();
	nodes.Clear();
	portals.Clear();
	portalIndex.Clear();
	clusters.Clear();
}

/*
================
idAASFileLocal::Write
================
*/
bool idAASFileLocal::Write( const idStr& fileName, unsigned int mapFileCRC )
{
	int i, num;
	idFile* aasFile;
	idReachability* reach;

	common->Printf( "[Write AAS]\n" );
	common->Printf( "writing %s\n", fileName.c_str() );

	name = fileName;
	crc = mapFileCRC;

	aasFile = fileSystem->OpenFileWrite( fileName, "fs_basepath" );
	if( !aasFile )
	{
		common->Error( "Error opening %s", fileName.c_str() );
		return false;
	}

	aasFile->WriteFloatString( "%s \"%s\"\n\n", AAS_FILEID, AAS_FILEVERSION );
	aasFile->WriteFloatString( "%u\n\n", mapFileCRC );

	// write out the settings
	aasFile->WriteFloatString( "settings\n" );
	settings.WriteToFile( aasFile );

	// write out planes
	aasFile->WriteFloatString( "planes %d {\n", planeList.Num() );
	for( i = 0; i < planeList.Num(); i++ )
	{
		aasFile->WriteFloatString( "\t%d ( %f %f %f %f )\n", i,
								   planeList[i].Normal().x, planeList[i].Normal().y, planeList[i].Normal().z, planeList[i].Dist() );
	}
	aasFile->WriteFloatString( "}\n" );

	// write out vertices
	aasFile->WriteFloatString( "vertices %d {\n", vertices.Num() );
	for( i = 0; i < vertices.Num(); i++ )
	{
		aasFile->WriteFloatString( "\t%d ( %f %f %f )\n", i, vertices[i].x, vertices[i].y, vertices[i].z );
	}
	aasFile->WriteFloatString( "}\n" );

	// write out edges
	aasFile->WriteFloatString( "edges %d {\n", edges.Num() );
	for( i = 0; i < edges.Num(); i++ )
	{
		aasFile->WriteFloatString( "\t%d ( %d %d )\n", i, edges[i].vertexNum[0], edges[i].vertexNum[1] );
	}
	aasFile->WriteFloatString( "}\n" );

	// write out edgeIndex
	aasFile->WriteFloatString( "edgeIndex %d {\n", edgeIndex.Num() );
	for( i = 0; i < edgeIndex.Num(); i++ )
	{
		aasFile->WriteFloatString( "\t%d ( %d )\n", i, edgeIndex[i] );
	}
	aasFile->WriteFloatString( "}\n" );

	// write out faces
	aasFile->WriteFloatString( "faces %d {\n", faces.Num() );
	for( i = 0; i < faces.Num(); i++ )
	{
		aasFile->WriteFloatString( "\t%d ( %d %d %d %d %d %d )\n", i, faces[i].planeNum, faces[i].flags,
								   faces[i].areas[0], faces[i].areas[1], faces[i].firstEdge, faces[i].numEdges );
	}
	aasFile->WriteFloatString( "}\n" );

	// write out faceIndex
	aasFile->WriteFloatString( "faceIndex %d {\n", faceIndex.Num() );
	for( i = 0; i < faceIndex.Num(); i++ )
	{
		aasFile->WriteFloatString( "\t%d ( %d )\n", i, faceIndex[i] );
	}
	aasFile->WriteFloatString( "}\n" );

	// write out areas
	aasFile->WriteFloatString( "areas %d {\n", areas.Num() );
	for( i = 0; i < areas.Num(); i++ )
	{
		for( num = 0, reach = areas[i].reach; reach; reach = reach->next )
		{
			num++;
		}
		aasFile->WriteFloatString( "\t%d ( %d %d %d %d %d %d ) %d {\n", i, areas[i].flags, areas[i].contents,
								   areas[i].firstFace, areas[i].numFaces, areas[i].cluster, areas[i].clusterAreaNum, num );
		for( reach = areas[i].reach; reach; reach = reach->next )
		{
			Reachability_Write( aasFile, reach );
			switch( reach->travelType )
			{
				case TFL_SPECIAL:
					Reachability_Special_Write( aasFile, static_cast<idReachability_Special*>( reach ) );
					break;
			}
			aasFile->WriteFloatString( "\n" );
		}
		aasFile->WriteFloatString( "\t}\n" );
	}
	aasFile->WriteFloatString( "}\n" );

	// write out nodes
	aasFile->WriteFloatString( "nodes %d {\n", nodes.Num() );
	for( i = 0; i < nodes.Num(); i++ )
	{
		aasFile->WriteFloatString( "\t%d ( %d %d %d )\n", i, nodes[i].planeNum, nodes[i].children[0], nodes[i].children[1] );
	}
	aasFile->WriteFloatString( "}\n" );

	// write out portals
	aasFile->WriteFloatString( "portals %d {\n", portals.Num() );
	for( i = 0; i < portals.Num(); i++ )
	{
		aasFile->WriteFloatString( "\t%d ( %d %d %d %d %d )\n", i, portals[i].areaNum, portals[i].clusters[0],
								   portals[i].clusters[1], portals[i].clusterAreaNum[0], portals[i].clusterAreaNum[1] );
	}
	aasFile->WriteFloatString( "}\n" );

	// write out portalIndex
	aasFile->WriteFloatString( "portalIndex %d {\n", portalIndex.Num() );
	for( i = 0; i < portalIndex.Num(); i++ )
	{
		aasFile->WriteFloatString( "\t%d ( %d )\n", i, portalIndex[i] );
	}
	aasFile->WriteFloatString( "}\n" );

	// write out clusters
	aasFile->WriteFloatString( "clusters %d {\n", clusters.Num() );
	for( i = 0; i < clusters.Num(); i++ )
	{
		aasFile->WriteFloatString( "\t%d ( %d %d %d %d )\n", i, clusters[i].numAreas, clusters[i].numReachableAreas,
								   clusters[i].firstPortal, clusters[i].numPortals );
	}
	aasFile->WriteFloatString( "}\n" );

	// close file
	fileSystem->CloseFile( aasFile );

	common->Printf( "done.\n" );

	return true;
}

/*
================
idAASFileLocal::ParseIndex
================
*/
bool idAASFileLocal::ParseIndex( idLexer& src, idList<aasIndex_t>& indexes )
{
	int numIndexes, i;
	aasIndex_t index;

	numIndexes = src.ParseInt();
	indexes.Resize( numIndexes );
	if( !src.ExpectTokenString( "{" ) )
	{
		return false;
	}
	for( i = 0; i < numIndexes; i++ )
	{
		src.ParseInt();
		src.ExpectTokenString( "(" );
		index = src.ParseInt();
		src.ExpectTokenString( ")" );
		indexes.Append( index );
	}
	if( !src.ExpectTokenString( "}" ) )
	{
		return false;
	}
	return true;
}

/*
================
idAASFileLocal::ParsePlanes
================
*/
bool idAASFileLocal::ParsePlanes( idLexer& src )
{
	int numPlanes, i;
	idPlane plane;
	idVec4 vec;

	numPlanes = src.ParseInt();
	planeList.Resize( numPlanes );
	if( !src.ExpectTokenString( "{" ) )
	{
		return false;
	}
	for( i = 0; i < numPlanes; i++ )
	{
		src.ParseInt();
		if( !src.Parse1DMatrix( 4, vec.ToFloatPtr() ) )
		{
			return false;
		}
		plane.SetNormal( vec.ToVec3() );
		plane.SetDist( vec[3] );
		planeList.Append( plane );
	}
	if( !src.ExpectTokenString( "}" ) )
	{
		return false;
	}
	return true;
}

/*
================
idAASFileLocal::ParseVertices
================
*/
bool idAASFileLocal::ParseVertices( idLexer& src )
{
	int numVertices, i;
	idVec3 vec;

	numVertices = src.ParseInt();
	vertices.Resize( numVertices );
	if( !src.ExpectTokenString( "{" ) )
	{
		return false;
	}
	for( i = 0; i < numVertices; i++ )
	{
		src.ParseInt();
		if( !src.Parse1DMatrix( 3, vec.ToFloatPtr() ) )
		{
			return false;
		}
		vertices.Append( vec );
	}
	if( !src.ExpectTokenString( "}" ) )
	{
		return false;
	}
	return true;
}

/*
================
idAASFileLocal::ParseEdges
================
*/
bool idAASFileLocal::ParseEdges( idLexer& src )
{
	int numEdges, i;
	aasEdge_t edge;

	numEdges = src.ParseInt();
	edges.Resize( numEdges );
	if( !src.ExpectTokenString( "{" ) )
	{
		return false;
	}
	for( i = 0; i < numEdges; i++ )
	{
		src.ParseInt();
		src.ExpectTokenString( "(" );
		edge.vertexNum[0] = src.ParseInt();
		edge.vertexNum[1] = src.ParseInt();
		src.ExpectTokenString( ")" );
		edges.Append( edge );
	}
	if( !src.ExpectTokenString( "}" ) )
	{
		return false;
	}
	return true;
}

/*
================
idAASFileLocal::ParseFaces
================
*/
bool idAASFileLocal::ParseFaces( idLexer& src )
{
	int numFaces, i;
	aasFace_t face;

	numFaces = src.ParseInt();
	faces.Resize( numFaces );
	if( !src.ExpectTokenString( "{" ) )
	{
		return false;
	}
	for( i = 0; i < numFaces; i++ )
	{
		src.ParseInt();
		src.ExpectTokenString( "(" );
		face.planeNum = src.ParseInt();
		face.flags = src.ParseInt();
		face.areas[0] = src.ParseInt();
		face.areas[1] = src.ParseInt();
		face.firstEdge = src.ParseInt();
		face.numEdges = src.ParseInt();
		src.ExpectTokenString( ")" );
		faces.Append( face );
	}
	if( !src.ExpectTokenString( "}" ) )
	{
		return false;
	}
	return true;
}

/*
================
idAASFileLocal::ParseReachabilities
================
*/
bool idAASFileLocal::ParseReachabilities( idLexer& src, int areaNum )
{
	int num, j;
	aasArea_t* area;
	idReachability reach, *newReach;
	idReachability_Special* special;

	area = &areas[areaNum];

	num = src.ParseInt();
	src.ExpectTokenString( "{" );
	area->reach = NULL;
	area->rev_reach = NULL;
	area->travelFlags = AreaContentsTravelFlags( areaNum );
	for( j = 0; j < num; j++ )
	{
		Reachability_Read( src, &reach );
		switch( reach.travelType )
		{
			case TFL_SPECIAL:
				newReach = special = new( TAG_AAS ) idReachability_Special();
				Reachability_Special_Read( src, special );
				break;
			default:
				newReach = new( TAG_AAS ) idReachability();
				break;
		}
		newReach->CopyBase( reach );
		newReach->fromAreaNum = areaNum;
		newReach->next = area->reach;
		area->reach = newReach;
	}
	src.ExpectTokenString( "}" );
	return true;
}

/*
================
idAASFileLocal::LinkReversedReachability
================
*/
void idAASFileLocal::LinkReversedReachability()
{
	int i;
	idReachability* reach;

	// link reversed reachabilities
	for( i = 0; i < areas.Num(); i++ )
	{
		for( reach = areas[i].reach; reach; reach = reach->next )
		{
			reach->rev_next = areas[reach->toAreaNum].rev_reach;
			areas[reach->toAreaNum].rev_reach = reach;
		}
	}
}

/*
================
idAASFileLocal::ParseAreas
================
*/
bool idAASFileLocal::ParseAreas( idLexer& src )
{
	int numAreas, i;
	aasArea_t area;

	numAreas = src.ParseInt();
	areas.Resize( numAreas );
	if( !src.ExpectTokenString( "{" ) )
	{
		return false;
	}
	for( i = 0; i < numAreas; i++ )
	{
		src.ParseInt();
		src.ExpectTokenString( "(" );
		area.flags = src.ParseInt();
		area.contents = src.ParseInt();
		area.firstFace = src.ParseInt();
		area.numFaces = src.ParseInt();
		area.cluster = src.ParseInt();
		area.clusterAreaNum = src.ParseInt();
		src.ExpectTokenString( ")" );
		areas.Append( area );
		ParseReachabilities( src, i );
	}
	if( !src.ExpectTokenString( "}" ) )
	{
		return false;
	}

	LinkReversedReachability();

	return true;
}

/*
================
idAASFileLocal::ParseNodes
================
*/
bool idAASFileLocal::ParseNodes( idLexer& src )
{
	int numNodes, i;
	aasNode_t node;

	numNodes = src.ParseInt();
	nodes.Resize( numNodes );
	if( !src.ExpectTokenString( "{" ) )
	{
		return false;
	}
	for( i = 0; i < numNodes; i++ )
	{
		src.ParseInt();
		src.ExpectTokenString( "(" );
		node.planeNum = src.ParseInt();
		node.children[0] = src.ParseInt();
		node.children[1] = src.ParseInt();
		src.ExpectTokenString( ")" );
		nodes.Append( node );
	}
	if( !src.ExpectTokenString( "}" ) )
	{
		return false;
	}
	return true;
}

/*
================
idAASFileLocal::ParsePortals
================
*/
bool idAASFileLocal::ParsePortals( idLexer& src )
{
	int numPortals, i;
	aasPortal_t portal;

	numPortals = src.ParseInt();
	portals.Resize( numPortals );
	if( !src.ExpectTokenString( "{" ) )
	{
		return false;
	}
	for( i = 0; i < numPortals; i++ )
	{
		src.ParseInt();
		src.ExpectTokenString( "(" );
		portal.areaNum = src.ParseInt();
		portal.clusters[0] = src.ParseInt();
		portal.clusters[1] = src.ParseInt();
		portal.clusterAreaNum[0] = src.ParseInt();
		portal.clusterAreaNum[1] = src.ParseInt();
		src.ExpectTokenString( ")" );
		portals.Append( portal );
	}
	if( !src.ExpectTokenString( "}" ) )
	{
		return false;
	}
	return true;
}

/*
================
idAASFileLocal::ParseClusters
================
*/
bool idAASFileLocal::ParseClusters( idLexer& src )
{
	int numClusters, i;
	aasCluster_t cluster;

	numClusters = src.ParseInt();
	clusters.Resize( numClusters );
	if( !src.ExpectTokenString( "{" ) )
	{
		return false;
	}
	for( i = 0; i < numClusters; i++ )
	{
		src.ParseInt();
		src.ExpectTokenString( "(" );
		cluster.numAreas = src.ParseInt();
		cluster.numReachableAreas = src.ParseInt();
		cluster.firstPortal = src.ParseInt();
		cluster.numPortals = src.ParseInt();
		src.ExpectTokenString( ")" );
		clusters.Append( cluster );
	}
	if( !src.ExpectTokenString( "}" ) )
	{
		return false;
	}
	return true;
}

/*
================
idAASFileLocal::FinishAreas
================
*/
void idAASFileLocal::FinishAreas()
{
	int i;

	for( i = 0; i < areas.Num(); i++ )
	{
		areas[i].center = AreaReachableGoal( i );
		areas[i].bounds = AreaBounds( i );
	}
}

/*
================
idAASFileLocal::Load
================
*/
bool idAASFileLocal::Load( const idStr& fileName, unsigned int mapFileCRC )
{
	idLexer src( LEXFL_NOFATALERRORS | LEXFL_NOSTRINGESCAPECHARS | LEXFL_NOSTRINGCONCAT | LEXFL_ALLOWPATHNAMES );
	idToken token;
	int depth;
	unsigned int c;

	name = fileName;
	crc = mapFileCRC;

	common->Printf( "[Load AAS]\n" );
	common->Printf( "loading %s\n", name.c_str() );

	if( !src.LoadFile( name ) )
	{
		return false;
	}

	if( !src.ExpectTokenString( AAS_FILEID ) )
	{
		common->Warning( "Not an AAS file: '%s'", name.c_str() );
		return false;
	}

	if( !src.ReadToken( &token ) || token != AAS_FILEVERSION )
	{
		common->Warning( "AAS file '%s' has version %s instead of %s", name.c_str(), token.c_str(), AAS_FILEVERSION );
		return false;
	}

	if( !src.ExpectTokenType( TT_NUMBER, TT_INTEGER, &token ) )
	{
		common->Warning( "AAS file '%s' has no map file CRC", name.c_str() );
		return false;
	}

	c = token.GetUnsignedLongValue();
	if( mapFileCRC && c != mapFileCRC )
	{
		common->Warning( "AAS file '%s' is out of date", name.c_str() );
		return false;
	}

	// clear the file in memory
	Clear();

	// parse the file
	while( 1 )
	{
		if( !src.ReadToken( &token ) )
		{
			break;
		}

		if( token == "settings" )
		{
			if( !settings.FromParser( src ) )
			{
				return false;
			}
		}
		else if( token == "planes" )
		{
			if( !ParsePlanes( src ) )
			{
				return false;
			}
		}
		else if( token == "vertices" )
		{
			if( !ParseVertices( src ) )
			{
				return false;
			}
		}
		else if( token == "edges" )
		{
			if( !ParseEdges( src ) )
			{
				return false;
			}
		}
		else if( token == "edgeIndex" )
		{
			if( !ParseIndex( src, edgeIndex ) )
			{
				return false;
			}
		}
		else if( token == "faces" )
		{
			if( !ParseFaces( src ) )
			{
				return false;
			}
		}
		else if( token == "faceIndex" )
		{
			if( !ParseIndex( src, faceIndex ) )
			{
				return false;
			}
		}
		else if( token == "areas" )
		{
			if( !ParseAreas( src ) )
			{
				return false;
			}
		}
		else if( token == "nodes" )
		{
			if( !ParseNodes( src ) )
			{
				return false;
			}
		}
		else if( token == "portals" )
		{
			if( !ParsePortals( src ) )
			{
				return false;
			}
		}
		else if( token == "portalIndex" )
		{
			if( !ParseIndex( src, portalIndex ) )
			{
				return false;
			}
		}
		else if( token == "clusters" )
		{
			if( !ParseClusters( src ) )
			{
				return false;
			}
		}
		else
		{
			src.Error( "idAASFileLocal::Load: bad token \"%s\"", token.c_str() );
			return false;
		}
	}

	FinishAreas();

	depth = MaxTreeDepth();
	if( depth > MAX_AAS_TREE_DEPTH )
	{
		src.Error( "idAASFileLocal::Load: tree depth = %d", depth );
	}

	common->UpdateLevelLoadPacifier();

	common->Printf( "done.\n" );

	return true;
}

/*
================
idAASFileLocal::MemorySize
================
*/
int idAASFileLocal::MemorySize() const
{
	int size;

	size = planeList.Size();
	size += vertices.Size();
	size += edges.Size();
	size += edgeIndex.Size();
	size += faces.Size();
	size += faceIndex.Size();
	size += areas.Size();
	size += nodes.Size();
	size += portals.Size();
	size += portalIndex.Size();
	size += clusters.Size();
	size += sizeof( idReachability_Walk ) * NumReachabilities();

	return size;
}

/*
================
idAASFileLocal::PrintInfo
================
*/
void idAASFileLocal::PrintInfo() const
{
	common->Printf( "%6d KB file size\n", MemorySize() >> 10 );
	common->Printf( "%6d areas\n", areas.Num() );
	common->Printf( "%6d max tree depth\n", MaxTreeDepth() );
	ReportRoutingEfficiency();
}

/*
================
idAASFileLocal::NumReachabilities
================
*/
int idAASFileLocal::NumReachabilities() const
{
	int i, num;
	idReachability* reach;

	num = 0;
	for( i = 0; i < areas.Num(); i++ )
	{
		for( reach = areas[i].reach; reach; reach = reach->next )
		{
			num++;
		}
	}
	return num;
}

/*
================
idAASFileLocal::ReportRoutingEfficiency
================
*/
void idAASFileLocal::ReportRoutingEfficiency() const
{
	int numReachableAreas, total, i, n;

	numReachableAreas = 0;
	total = 0;
	for( i = 0; i < clusters.Num(); i++ )
	{
		n = clusters[i].numReachableAreas;
		numReachableAreas += n;
		total += n * n;
	}
	total += numReachableAreas * portals.Num();

	common->Printf( "%6d reachable areas\n", numReachableAreas );
	common->Printf( "%6d reachabilities\n", NumReachabilities() );
	common->Printf( "%6d KB max routing cache\n", ( total * 3 ) >> 10 );
}

/*
================
idAASFileLocal::DeleteReachabilities
================
*/
void idAASFileLocal::DeleteReachabilities()
{
	int i;
	idReachability* reach, *nextReach;

	for( i = 0; i < areas.Num(); i++ )
	{
		for( reach = areas[i].reach; reach; reach = nextReach )
		{
			nextReach = reach->next;
			delete reach;
		}
		areas[i].reach = NULL;
		areas[i].rev_reach = NULL;
	}
}

/*
================
idAASFileLocal::DeleteClusters
================
*/
void idAASFileLocal::DeleteClusters()
{
	aasPortal_t portal;
	aasCluster_t cluster;

	portals.Clear();
	portalIndex.Clear();
	clusters.Clear();

	// first portal is a dummy
	memset( &portal, 0, sizeof( portal ) );
	portals.Append( portal );

	// first cluster is a dummy
	memset( &cluster, 0, sizeof( cluster ) );
	clusters.Append( cluster );
}
