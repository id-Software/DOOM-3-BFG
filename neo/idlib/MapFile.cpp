/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2015 Robert Beckebans

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


/*
===============
FloatCRC
===============
*/
ID_INLINE unsigned int FloatCRC( float f )
{
	return *( unsigned int* )&f;
}

/*
===============
StringCRC
===============
*/
ID_INLINE unsigned int StringCRC( const char* str )
{
	unsigned int i, crc;
	const unsigned char* ptr;

	crc = 0;
	ptr = reinterpret_cast<const unsigned char*>( str );
	for( i = 0; str[i]; i++ )
	{
		crc ^= str[i] << ( i & 3 );
	}
	return crc;
}

/*
=================
ComputeAxisBase

WARNING : special case behaviour of atan2(y,x) <-> atan(y/x) might not be the same everywhere when x == 0
rotation by (0,RotY,RotZ) assigns X to normal
=================
*/
static void ComputeAxisBase( const idVec3& normal, idVec3& texS, idVec3& texT )
{
	float RotY, RotZ;
	idVec3 n;

	// do some cleaning
	n[0] = ( idMath::Fabs( normal[0] ) < 1e-6f ) ? 0.0f : normal[0];
	n[1] = ( idMath::Fabs( normal[1] ) < 1e-6f ) ? 0.0f : normal[1];
	n[2] = ( idMath::Fabs( normal[2] ) < 1e-6f ) ? 0.0f : normal[2];

	RotY = -atan2( n[2], idMath::Sqrt( n[1] * n[1] + n[0] * n[0] ) );
	RotZ = atan2( n[1], n[0] );
	// rotate (0,1,0) and (0,0,1) to compute texS and texT
	texS[0] = -sin( RotZ );
	texS[1] = cos( RotZ );
	texS[2] = 0;
	// the texT vector is along -Z ( T texture coorinates axis )
	texT[0] = -sin( RotY ) * cos( RotZ );
	texT[1] = -sin( RotY ) * sin( RotZ );
	texT[2] = -cos( RotY );
}

/*
=================
idMapBrushSide::GetTextureVectors
=================
*/
void idMapBrushSide::GetTextureVectors( idVec4 v[2] ) const
{
	int i;
	idVec3 texX, texY;

	ComputeAxisBase( plane.Normal(), texX, texY );
	for( i = 0; i < 2; i++ )
	{
		v[i][0] = texX[0] * texMat[i][0] + texY[0] * texMat[i][1];
		v[i][1] = texX[1] * texMat[i][0] + texY[1] * texMat[i][1];
		v[i][2] = texX[2] * texMat[i][0] + texY[2] * texMat[i][1];
		v[i][3] = texMat[i][2] + ( origin * v[i].ToVec3() );
	}
}

/*
=================
idMapPatch::Parse
=================
*/
idMapPatch* idMapPatch::Parse( idLexer& src, const idVec3& origin, bool patchDef3, float version )
{
	float		info[7];
	idDrawVert* vert;
	idToken		token;
	int			i, j;

	if( !src.ExpectTokenString( "{" ) )
	{
		return NULL;
	}

	// read the material (we had an implicit 'textures/' in the old format...)
	if( !src.ReadToken( &token ) )
	{
		src.Error( "idMapPatch::Parse: unexpected EOF" );
		return NULL;
	}

	// Parse it
	if( patchDef3 )
	{
		if( !src.Parse1DMatrix( 7, info ) )
		{
			src.Error( "idMapPatch::Parse: unable to Parse patchDef3 info" );
			return NULL;
		}
	}
	else
	{
		if( !src.Parse1DMatrix( 5, info ) )
		{
			src.Error( "idMapPatch::Parse: unable to parse patchDef2 info" );
			return NULL;
		}
	}

	idMapPatch* patch = new( TAG_IDLIB ) idMapPatch( info[0], info[1] );

	patch->SetSize( info[0], info[1] );
	if( version < 2.0f )
	{
		patch->SetMaterial( "textures/" + token );
	}
	else
	{
		patch->SetMaterial( token );
	}

	if( patchDef3 )
	{
		patch->SetHorzSubdivisions( info[2] );
		patch->SetVertSubdivisions( info[3] );
		patch->SetExplicitlySubdivided( true );
	}

	if( patch->GetWidth() < 0 || patch->GetHeight() < 0 )
	{
		src.Error( "idMapPatch::Parse: bad size" );
		delete patch;
		return NULL;
	}

	// these were written out in the wrong order, IMHO
	if( !src.ExpectTokenString( "(" ) )
	{
		src.Error( "idMapPatch::Parse: bad patch vertex data" );
		delete patch;
		return NULL;
	}


	for( j = 0; j < patch->GetWidth(); j++ )
	{
		if( !src.ExpectTokenString( "(" ) )
		{
			src.Error( "idMapPatch::Parse: bad vertex row data" );
			delete patch;
			return NULL;
		}
		for( i = 0; i < patch->GetHeight(); i++ )
		{
			float v[5];

			if( !src.Parse1DMatrix( 5, v ) )
			{
				src.Error( "idMapPatch::Parse: bad vertex column data" );
				delete patch;
				return NULL;
			}

			vert = &( ( *patch )[i * patch->GetWidth() + j] );
			vert->xyz[0] = v[0] - origin[0];
			vert->xyz[1] = v[1] - origin[1];
			vert->xyz[2] = v[2] - origin[2];
			vert->SetTexCoord( v[3], v[4] );
		}
		if( !src.ExpectTokenString( ")" ) )
		{
			delete patch;
			src.Error( "idMapPatch::Parse: unable to parse patch control points" );
			return NULL;
		}
	}

	if( !src.ExpectTokenString( ")" ) )
	{
		src.Error( "idMapPatch::Parse: unable to parse patch control points, no closure" );
		delete patch;
		return NULL;
	}

	// read any key/value pairs
	while( src.ReadToken( &token ) )
	{
		if( token == "}" )
		{
			src.ExpectTokenString( "}" );
			break;
		}
		if( token.type == TT_STRING )
		{
			idStr key = token;
			src.ExpectTokenType( TT_STRING, 0, &token );
			patch->epairs.Set( key, token );
		}
	}

	return patch;
}

/*
============
idMapPatch::Write
============
*/
bool idMapPatch::Write( idFile* fp, int primitiveNum, const idVec3& origin ) const
{
	int i, j;
	const idDrawVert* v;

	if( GetExplicitlySubdivided() )
	{
		fp->WriteFloatString( "// primitive %d\n{\n patchDef3\n {\n", primitiveNum );
		fp->WriteFloatString( "  \"%s\"\n  ( %d %d %d %d 0 0 0 )\n", GetMaterial(), GetWidth(), GetHeight(), GetHorzSubdivisions(), GetVertSubdivisions() );
	}
	else
	{
		fp->WriteFloatString( "// primitive %d\n{\n patchDef2\n {\n", primitiveNum );
		fp->WriteFloatString( "  \"%s\"\n  ( %d %d 0 0 0 )\n", GetMaterial(), GetWidth(), GetHeight() );
	}

	fp->WriteFloatString( "  (\n" );
	idVec2 st;
	for( i = 0; i < GetWidth(); i++ )
	{
		fp->WriteFloatString( "   ( " );
		for( j = 0; j < GetHeight(); j++ )
		{
			v = &verts[ j * GetWidth() + i ];
			st = v->GetTexCoord();
			fp->WriteFloatString( " ( %f %f %f %f %f )", v->xyz[0] + origin[0],
								  v->xyz[1] + origin[1], v->xyz[2] + origin[2], st[0], st[1] );
		}
		fp->WriteFloatString( " )\n" );
	}
	fp->WriteFloatString( "  )\n }\n}\n" );

	return true;
}

/*
===============
idMapPatch::GetGeometryCRC
===============
*/
unsigned int idMapPatch::GetGeometryCRC() const
{
	int i, j;
	unsigned int crc;

	crc = GetHorzSubdivisions() ^ GetVertSubdivisions();
	for( i = 0; i < GetWidth(); i++ )
	{
		for( j = 0; j < GetHeight(); j++ )
		{
			crc ^= FloatCRC( verts[j * GetWidth() + i].xyz.x );
			crc ^= FloatCRC( verts[j * GetWidth() + i].xyz.y );
			crc ^= FloatCRC( verts[j * GetWidth() + i].xyz.z );
		}
	}

	crc ^= StringCRC( GetMaterial() );

	return crc;
}

/*
=================
idMapBrush::Parse
=================
*/
idMapBrush* idMapBrush::Parse( idLexer& src, const idVec3& origin, bool newFormat, float version )
{
	int i;
	idVec3 planepts[3];
	idToken token;
	idList<idMapBrushSide*> sides;
	idMapBrushSide*	side;
	idDict epairs;

	if( !src.ExpectTokenString( "{" ) )
	{
		return NULL;
	}

	do
	{
		if( !src.ReadToken( &token ) )
		{
			src.Error( "idMapBrush::Parse: unexpected EOF" );
			sides.DeleteContents( true );
			return NULL;
		}
		if( token == "}" )
		{
			break;
		}

		// here we may have to jump over brush epairs ( only used in editor )
		do
		{
			// if token is a brace
			if( token == "(" )
			{
				break;
			}
			// the token should be a key string for a key/value pair
			if( token.type != TT_STRING )
			{
				src.Error( "idMapBrush::Parse: unexpected %s, expected ( or epair key string", token.c_str() );
				sides.DeleteContents( true );
				return NULL;
			}

			idStr key = token;

			if( !src.ReadTokenOnLine( &token ) || token.type != TT_STRING )
			{
				src.Error( "idMapBrush::Parse: expected epair value string not found" );
				sides.DeleteContents( true );
				return NULL;
			}

			epairs.Set( key, token );

			// try to read the next key
			if( !src.ReadToken( &token ) )
			{
				src.Error( "idMapBrush::Parse: unexpected EOF" );
				sides.DeleteContents( true );
				return NULL;
			}
		}
		while( 1 );

		src.UnreadToken( &token );

		side = new( TAG_IDLIB ) idMapBrushSide();
		sides.Append( side );

		if( newFormat )
		{
			if( !src.Parse1DMatrix( 4, side->plane.ToFloatPtr() ) )
			{
				src.Error( "idMapBrush::Parse: unable to read brush side plane definition" );
				sides.DeleteContents( true );
				return NULL;
			}
		}
		else
		{
			// read the three point plane definition
			if( !src.Parse1DMatrix( 3, planepts[0].ToFloatPtr() ) ||
					!src.Parse1DMatrix( 3, planepts[1].ToFloatPtr() ) ||
					!src.Parse1DMatrix( 3, planepts[2].ToFloatPtr() ) )
			{
				src.Error( "idMapBrush::Parse: unable to read brush side plane definition" );
				sides.DeleteContents( true );
				return NULL;
			}

			planepts[0] -= origin;
			planepts[1] -= origin;
			planepts[2] -= origin;

			side->plane.FromPoints( planepts[0], planepts[1], planepts[2] );
		}

		// read the texture matrix
		// this is odd, because the texmat is 2D relative to default planar texture axis
		if( !src.Parse2DMatrix( 2, 3, side->texMat[0].ToFloatPtr() ) )
		{
			src.Error( "idMapBrush::Parse: unable to read brush side texture matrix" );
			sides.DeleteContents( true );
			return NULL;
		}
		side->origin = origin;

		// read the material
		if( !src.ReadTokenOnLine( &token ) )
		{
			src.Error( "idMapBrush::Parse: unable to read brush side material" );
			sides.DeleteContents( true );
			return NULL;
		}

		// we had an implicit 'textures/' in the old format...
		if( version < 2.0f )
		{
			side->material = "textures/" + token;
		}
		else
		{
			side->material = token;
		}

		// Q2 allowed override of default flags and values, but we don't any more
		if( src.ReadTokenOnLine( &token ) )
		{
			if( src.ReadTokenOnLine( &token ) )
			{
				if( src.ReadTokenOnLine( &token ) )
				{
				}
			}
		}
	}
	while( 1 );

	if( !src.ExpectTokenString( "}" ) )
	{
		sides.DeleteContents( true );
		return NULL;
	}

	idMapBrush* brush = new( TAG_IDLIB ) idMapBrush();
	for( i = 0; i < sides.Num(); i++ )
	{
		brush->AddSide( sides[i] );
	}

	brush->epairs = epairs;

	return brush;
}

/*
=================
idMapBrush::ParseQ3
=================
*/
idMapBrush* idMapBrush::ParseQ3( idLexer& src, const idVec3& origin )
{
	int i, shift[2], rotate;
	float scale[2];
	idVec3 planepts[3];
	idToken token;
	idList<idMapBrushSide*> sides;
	idMapBrushSide*	side;
	idDict epairs;

	do
	{
		if( src.CheckTokenString( "}" ) )
		{
			break;
		}

		side = new( TAG_IDLIB ) idMapBrushSide();
		sides.Append( side );

		// read the three point plane definition
		if( !src.Parse1DMatrix( 3, planepts[0].ToFloatPtr() ) ||
				!src.Parse1DMatrix( 3, planepts[1].ToFloatPtr() ) ||
				!src.Parse1DMatrix( 3, planepts[2].ToFloatPtr() ) )
		{
			src.Error( "idMapBrush::ParseQ3: unable to read brush side plane definition" );
			sides.DeleteContents( true );
			return NULL;
		}

		planepts[0] -= origin;
		planepts[1] -= origin;
		planepts[2] -= origin;

		side->plane.FromPoints( planepts[0], planepts[1], planepts[2] );

		// read the material
		if( !src.ReadTokenOnLine( &token ) )
		{
			src.Error( "idMapBrush::ParseQ3: unable to read brush side material" );
			sides.DeleteContents( true );
			return NULL;
		}

		// we have an implicit 'textures/' in the old format
		side->material = "textures/" + token;

		// read the texture shift, rotate and scale
		shift[0] = src.ParseInt();
		shift[1] = src.ParseInt();
		rotate = src.ParseInt();
		scale[0] = src.ParseFloat();
		scale[1] = src.ParseFloat();
		side->texMat[0] = idVec3( 0.03125f, 0.0f, 0.0f );
		side->texMat[1] = idVec3( 0.0f, 0.03125f, 0.0f );
		side->origin = origin;

		// Q2 allowed override of default flags and values, but we don't any more
		if( src.ReadTokenOnLine( &token ) )
		{
			if( src.ReadTokenOnLine( &token ) )
			{
				if( src.ReadTokenOnLine( &token ) )
				{
				}
			}
		}
	}
	while( 1 );

	idMapBrush* brush = new( TAG_IDLIB ) idMapBrush();
	for( i = 0; i < sides.Num(); i++ )
	{
		brush->AddSide( sides[i] );
	}

	brush->epairs = epairs;

	return brush;
}

/*
============
idMapBrush::Write
============
*/
bool idMapBrush::Write( idFile* fp, int primitiveNum, const idVec3& origin ) const
{
	int i;
	idMapBrushSide* side;

	fp->WriteFloatString( "// primitive %d\n{\n brushDef3\n {\n", primitiveNum );

	// write brush epairs
	for( i = 0; i < epairs.GetNumKeyVals(); i++ )
	{
		fp->WriteFloatString( "  \"%s\" \"%s\"\n", epairs.GetKeyVal( i )->GetKey().c_str(), epairs.GetKeyVal( i )->GetValue().c_str() );
	}

	// write brush sides
	for( i = 0; i < GetNumSides(); i++ )
	{
		side = GetSide( i );
		fp->WriteFloatString( "  ( %f %f %f %f ) ", side->plane[0], side->plane[1], side->plane[2], side->plane[3] );
		fp->WriteFloatString( "( ( %f %f %f ) ( %f %f %f ) ) \"%s\" 0 0 0\n",
							  side->texMat[0][0], side->texMat[0][1], side->texMat[0][2],
							  side->texMat[1][0], side->texMat[1][1], side->texMat[1][2],
							  side->material.c_str() );
	}

	fp->WriteFloatString( " }\n}\n" );

	return true;
}

/*
===============
idMapBrush::GetGeometryCRC
===============
*/
unsigned int idMapBrush::GetGeometryCRC() const
{
	int i, j;
	idMapBrushSide* mapSide;
	unsigned int crc;

	crc = 0;
	for( i = 0; i < GetNumSides(); i++ )
	{
		mapSide = GetSide( i );
		for( j = 0; j < 4; j++ )
		{
			crc ^= FloatCRC( mapSide->GetPlane()[j] );
		}
		crc ^= StringCRC( mapSide->GetMaterial() );
	}

	return crc;
}

/*
================
idMapEntity::Parse
================
*/
idMapEntity* idMapEntity::Parse( idLexer& src, bool worldSpawn, float version )
{
	idToken	token;
	idMapEntity* mapEnt;
	idMapPatch* mapPatch;
	idMapBrush* mapBrush;
	// RB begin
	MapPolygonMesh* mapMesh;
	// RB end
	bool worldent;
	idVec3 origin;
	double v1, v2, v3;

	if( !src.ReadToken( &token ) )
	{
		return NULL;
	}

	if( token != "{" )
	{
		src.Error( "idMapEntity::Parse: { not found, found %s", token.c_str() );
		return NULL;
	}

	mapEnt = new( TAG_IDLIB ) idMapEntity();

	if( worldSpawn )
	{
		mapEnt->primitives.Resize( 1024, 256 );
	}

	origin.Zero();
	worldent = false;
	do
	{
		if( !src.ReadToken( &token ) )
		{
			src.Error( "idMapEntity::Parse: EOF without closing brace" );
			return NULL;
		}
		if( token == "}" )
		{
			break;
		}

		if( token == "{" )
		{
			// parse a brush or patch
			if( !src.ReadToken( &token ) )
			{
				src.Error( "idMapEntity::Parse: unexpected EOF" );
				return NULL;
			}

			if( worldent )
			{
				origin.Zero();
			}

			// if is it a brush: brush, brushDef, brushDef2, brushDef3
			if( token.Icmpn( "brush", 5 ) == 0 )
			{
				mapBrush = idMapBrush::Parse( src, origin, ( !token.Icmp( "brushDef2" ) || !token.Icmp( "brushDef3" ) ), version );
				if( !mapBrush )
				{
					return NULL;
				}
				mapEnt->AddPrimitive( mapBrush );
			}
			// if is it a patch: patchDef2, patchDef3
			else if( token.Icmpn( "patch", 5 ) == 0 )
			{
				mapPatch = idMapPatch::Parse( src, origin, !token.Icmp( "patchDef3" ), version );
				if( !mapPatch )
				{
					return NULL;
				}
				mapEnt->AddPrimitive( mapPatch );
			}
			// RB: new mesh primitive with ngons
			else if( token.Icmpn( "mesh", 4 ) == 0 )
			{
				mapMesh = MapPolygonMesh::Parse( src, origin, version );
				if( !mapMesh )
				{
					return NULL;
				}
				mapEnt->AddPrimitive( mapMesh );
			}
			// RB end
			// assume it's a brush in Q3 or older style
			else
			{
				src.UnreadToken( &token );
				mapBrush = idMapBrush::ParseQ3( src, origin );
				if( !mapBrush )
				{
					return NULL;
				}
				mapEnt->AddPrimitive( mapBrush );
			}
		}
		else
		{
			idStr key, value;

			// parse a key / value pair
			key = token;
			src.ReadTokenOnLine( &token );
			value = token;

			// strip trailing spaces that sometimes get accidentally
			// added in the editor
			value.StripTrailingWhitespace();
			key.StripTrailingWhitespace();

			mapEnt->epairs.Set( key, value );

			if( !idStr::Icmp( key, "origin" ) )
			{
				// scanf into doubles, then assign, so it is idVec size independent
				v1 = v2 = v3 = 0;
				sscanf( value, "%lf %lf %lf", &v1, &v2, &v3 );
				origin.x = v1;
				origin.y = v2;
				origin.z = v3;
			}
			else if( !idStr::Icmp( key, "classname" ) && !idStr::Icmp( value, "worldspawn" ) )
			{
				worldent = true;
			}
		}
	}
	while( 1 );

	return mapEnt;
}

/*
============
idMapEntity::Write
============
*/
bool idMapEntity::Write( idFile* fp, int entityNum ) const
{
	int i;
	idMapPrimitive* mapPrim;
	idVec3 origin;

	fp->WriteFloatString( "// entity %d\n{\n", entityNum );

	// write entity epairs
	for( i = 0; i < epairs.GetNumKeyVals(); i++ )
	{
		fp->WriteFloatString( "\"%s\" \"%s\"\n", epairs.GetKeyVal( i )->GetKey().c_str(), epairs.GetKeyVal( i )->GetValue().c_str() );
	}

	epairs.GetVector( "origin", "0 0 0", origin );

	// write pritimives
	for( i = 0; i < GetNumPrimitives(); i++ )
	{
		mapPrim = GetPrimitive( i );

		switch( mapPrim->GetType() )
		{
			case idMapPrimitive::TYPE_BRUSH:
				static_cast<idMapBrush*>( mapPrim )->Write( fp, i, origin );
				break;
			case idMapPrimitive::TYPE_PATCH:
				static_cast<idMapPatch*>( mapPrim )->Write( fp, i, origin );
				break;
			// RB begin
			case idMapPrimitive::TYPE_MESH:
				static_cast<MapPolygonMesh*>( mapPrim )->Write( fp, i, origin );
				break;
				// RB end
		}
	}

	fp->WriteFloatString( "}\n" );

	return true;
}

// RB begin
bool idMapEntity::WriteJSON( idFile* fp, int entityNum, int numEntities ) const
{
	idVec3 origin;

	fp->WriteFloatString( "\t\t{\n\t\t\t\"entity\": \"%d\",\n", entityNum );

	idStr key;
	idStr value;

	for( int i = 0; i < epairs.GetNumKeyVals(); i++ )
	{
		key = epairs.GetKeyVal( i )->GetKey();

		key.ReplaceChar( '\t', ' ' );

		value = epairs.GetKeyVal( i )->GetValue();
		value.BackSlashesToSlashes();

		fp->WriteFloatString( "\t\t\t\"%s\": \"%s\"%s\n", key.c_str(), value.c_str(), ( ( i == ( epairs.GetNumKeyVals() - 1 ) ) && !GetNumPrimitives() ) ? "" : "," );
	}

	epairs.GetVector( "origin", "0 0 0", origin );

	// write pritimives
	if( GetNumPrimitives() )
	{
		fp->WriteFloatString( "\t\t\t\"primitives\":\n\t\t\t[\n" );
	}

	int numPrimitives = GetNumPrimitives();
	for( int i = 0; i < numPrimitives; i++ )
	{
		idMapPrimitive* mapPrim = GetPrimitive( i );

		switch( mapPrim->GetType() )
		{
#if 0
			case idMapPrimitive::TYPE_BRUSH:
				static_cast<idMapBrush*>( mapPrim )->Write( fp, i, origin );
				break;
			case idMapPrimitive::TYPE_PATCH:
				static_cast<idMapPatch*>( mapPrim )->Write( fp, i, origin );
				break;
#endif
			case idMapPrimitive::TYPE_MESH:
				static_cast<MapPolygonMesh*>( mapPrim )->WriteJSON( fp, i, origin );
				break;

			default:
				continue;
		}

		// find next mesh primitive
		idMapPrimitive* nextPrim = NULL;
		for( int j = i + 1; j < numPrimitives; j++ )
		{
			nextPrim = GetPrimitive( j );
			if( nextPrim->GetType() == idMapPrimitive::TYPE_MESH )
			{
				break;
			}
		}


		if( nextPrim && ( nextPrim->GetType() == idMapPrimitive::TYPE_MESH ) )
		{
			fp->WriteFloatString( ",\n" );
		}
		else
		{
			fp->WriteFloatString( "\n" );
		}
	}

	if( GetNumPrimitives() )
	{
		fp->WriteFloatString( "\t\t\t]\n" );
	}

	fp->WriteFloatString( "\t\t}%s\n", ( entityNum == ( numEntities - 1 ) ) ? "" : "," );

	return true;
}

idMapEntity* idMapEntity::ParseJSON( idLexer& src )
{
	idToken	token;
	idMapEntity* mapEnt;
	//idMapPatch* mapPatch;
	//idMapBrush* mapBrush;
	// RB begin
	MapPolygonMesh* mapMesh;
	// RB end
	bool worldent;
	idVec3 origin;
	double v1, v2, v3;

	if( !src.ReadToken( &token ) )
	{
		return NULL;
	}

	if( token == "]" )
	{
		return NULL;
	}

	if( token == "," )
	{
		if( !src.ReadToken( &token ) )
		{
			return NULL;
		}
	}

	if( token != "{" )
	{
		src.Error( "idMapEntity::ParseJSON: { not found, found %s", token.c_str() );
		return NULL;
	}

	mapEnt = new idMapEntity();

	/*
	if( worldSpawn )
	{
		mapEnt->primitives.Resize( 1024, 256 );
	}
	*/

	origin.Zero();
	worldent = false;
	do
	{
		if( !src.ReadToken( &token ) )
		{
			src.Error( "idMapEntity::ParseJSON: EOF without closing brace" );
			return NULL;
		}

		if( token == "}" )
		{
			break;
		}

		if( token == "," )
		{
			continue;
		}

		// RB: new mesh primitive with ngons
		if( token == "primitives" )
		{
			if( !src.ExpectTokenString( ":" ) )
			{
				delete mapEnt;
				src.Error( "idMapEntity::ParseJSON: expected : for primitives" );
				return NULL;
			}

			if( !src.ExpectTokenString( "[" ) )
			{
				delete mapEnt;
				src.Error( "idMapEntity::ParseJSON: expected [ for primitives" );
				return NULL;
			}

			while( true )
			{
				if( !src.ReadToken( &token ) )
				{
					src.Error( "idMapEntity::ParseJSON: EOF without closing brace" );
					return NULL;
				}

				if( token == "]" )
				{
					break;
				}

				if( token == "," )
				{
					continue;
				}

				if( token == "{" )
				{
					mapMesh = MapPolygonMesh::ParseJSON( src );
					if( !mapMesh )
					{
						break;
					}

					mapEnt->AddPrimitive( mapMesh );
				}
			}
		}
		else
		{
			idStr key, value;

			// parse a key / value pair
			key = token;

			if( !src.ReadToken( &token ) )
			{
				src.Error( "idMapEntity::ParseJSON: EOF without closing brace" );
				delete mapEnt;
				return NULL;
			}

			if( token != ":" )
			{
				delete mapEnt;
				return NULL;
			}

			src.ReadTokenOnLine( &token );
			value = token;

			// strip trailing spaces that sometimes get accidentally
			// added in the editor
			value.StripTrailingWhitespace();
			key.StripTrailingWhitespace();

			mapEnt->epairs.Set( key, value );

			if( !idStr::Icmp( key, "origin" ) )
			{
				// scanf into doubles, then assign, so it is idVec size independent
				v1 = v2 = v3 = 0;
				sscanf( value, "%lf %lf %lf", &v1, &v2, &v3 );
				origin.x = v1;
				origin.y = v2;
				origin.z = v3;
			}
			else if( !idStr::Icmp( key, "classname" ) && !idStr::Icmp( value, "worldspawn" ) )
			{
				worldent = true;
			}
		}
	}
	while( 1 );

	return mapEnt;
}
// RB end

/*
===============
idMapEntity::RemovePrimitiveData
===============
*/
void idMapEntity::RemovePrimitiveData()
{
	primitives.DeleteContents( true );
}

/*
===============
idMapEntity::GetGeometryCRC
===============
*/
unsigned int idMapEntity::GetGeometryCRC() const
{
	int i;
	unsigned int crc;
	idMapPrimitive*	mapPrim;

	crc = 0;
	for( i = 0; i < GetNumPrimitives(); i++ )
	{
		mapPrim = GetPrimitive( i );

		switch( mapPrim->GetType() )
		{
			case idMapPrimitive::TYPE_BRUSH:
				crc ^= static_cast<idMapBrush*>( mapPrim )->GetGeometryCRC();
				break;
			case idMapPrimitive::TYPE_PATCH:
				crc ^= static_cast<idMapPatch*>( mapPrim )->GetGeometryCRC();
				break;

			// RB begin
			case idMapPrimitive::TYPE_MESH:
				crc ^= static_cast<MapPolygonMesh*>( mapPrim )->GetGeometryCRC();
				break;
				// RB end
		}
	}

	return crc;
}

class idSort_CompareMapEntity : public idSort_Quick< idMapEntity*, idSort_CompareMapEntity >
{
public:
	int Compare( idMapEntity* const& a, idMapEntity* const& b ) const
	{
		if( idStr::Icmp( a->epairs.GetString( "name" ), "worldspawn" ) == 0 )
		{
			return 1;
		}

		if( idStr::Icmp( b->epairs.GetString( "name" ), "worldspawn" ) == 0 )
		{
			return -1;
		}

		return idStr::Icmp( a->epairs.GetString( "name" ), b->epairs.GetString( "name" ) );
	}
};

/*
===============
idMapFile::Parse
===============
*/
bool idMapFile::Parse( const char* filename, bool ignoreRegion, bool osPath )
{
	// no string concatenation for epairs and allow path names for materials
	idLexer src( LEXFL_NOSTRINGCONCAT | LEXFL_NOSTRINGESCAPECHARS | LEXFL_ALLOWPATHNAMES );
	idToken token;
	idStr fullName;
	idMapEntity* mapEnt;
	int i, j, k;

	name = filename;
	name.StripFileExtension();
	fullName = name;
	hasPrimitiveData = false;

	bool isJSON = false;
	if( !ignoreRegion )
	{
		// RB: try loading a .json file first
		fullName.SetFileExtension( "json" );
		src.LoadFile( fullName, osPath );

		if( src.IsLoaded() )
		{
			isJSON = true;
		}
	}

	if( !src.IsLoaded() )
	{
		// now try a .map file
		fullName.SetFileExtension( "map" );
		src.LoadFile( fullName, osPath );
		if( !src.IsLoaded() )
		{
			// didn't get anything at all
			return false;
		}
	}

	version = OLD_MAP_VERSION;
	fileTime = src.GetFileTime();
	entities.DeleteContents( true );

	if( !src.ReadToken( &token ) )
	{
		return false;
	}

	if( token == "{" )
	{
		isJSON = true;
	}

	if( isJSON )
	{
		while( true )
		{
			if( !src.ReadToken( &token ) )
			{
				break;
			}

			if( token == "entities" )
			{
				if( !src.ReadToken( &token ) )
				{
					return false;
				}

				if( token != ":" )
				{
					src.Error( "idMapFile::Parse: : not found, found %s", token.c_str() );
					return false;
				}

				if( !src.ReadToken( &token ) )
				{
					return false;
				}

				if( token != "[" )
				{
					src.Error( "idMapFile::Parse: [ not found, found %s", token.c_str() );
					return false;
				}

				while( true )
				{
					mapEnt = idMapEntity::ParseJSON( src );
					if( !mapEnt )
					{
						break;
					}
					entities.Append( mapEnt );
				}
			}
		}

		//entities.SortWithTemplate( idSort_CompareMapEntity() );

		if( entities.Num() > 0 && ( idStr::Icmp( entities[0]->epairs.GetString( "name" ), "worldspawn" ) != 0 ) )
		{
			// move world spawn to first place
			for( int i = 1; i < entities.Num(); i++ )
			{
				if( idStr::Icmp( entities[i]->epairs.GetString( "name" ), "worldspawn" ) == 0 )
				{
					idMapEntity* tmp = entities[0];
					entities[0] = entities[i];
					entities[i] = tmp;
					break;
				}
			}
		}
	}
	else
	{
		if( token == "Version" )
		{
			src.ReadTokenOnLine( &token );
			version = token.GetFloatValue();
		}

		while( 1 )
		{
			mapEnt = idMapEntity::Parse( src, ( entities.Num() == 0 ), version );
			if( !mapEnt )
			{
				break;
			}
			entities.Append( mapEnt );
		}
	}

	SetGeometryCRC();

	// if the map has a worldspawn
	if( entities.Num() )
	{

		// "removeEntities" "classname" can be set in the worldspawn to remove all entities with the given classname
		const idKeyValue* removeEntities = entities[0]->epairs.MatchPrefix( "removeEntities", NULL );
		while( removeEntities )
		{
			RemoveEntities( removeEntities->GetValue() );
			removeEntities = entities[0]->epairs.MatchPrefix( "removeEntities", removeEntities );
		}

		// "overrideMaterial" "material" can be set in the worldspawn to reset all materials
		idStr material;
		if( entities[0]->epairs.GetString( "overrideMaterial", "", material ) )
		{
			for( i = 0; i < entities.Num(); i++ )
			{
				mapEnt = entities[i];
				for( j = 0; j < mapEnt->GetNumPrimitives(); j++ )
				{
					idMapPrimitive* mapPrimitive = mapEnt->GetPrimitive( j );
					switch( mapPrimitive->GetType() )
					{
						case idMapPrimitive::TYPE_BRUSH:
						{
							idMapBrush* mapBrush = static_cast<idMapBrush*>( mapPrimitive );
							for( k = 0; k < mapBrush->GetNumSides(); k++ )
							{
								mapBrush->GetSide( k )->SetMaterial( material );
							}
							break;
						}
						case idMapPrimitive::TYPE_PATCH:
						{
							static_cast<idMapPatch*>( mapPrimitive )->SetMaterial( material );
							break;
						}
					}
				}
			}
		}

		// force all entities to have a name key/value pair
		if( entities[0]->epairs.GetBool( "forceEntityNames" ) )
		{
			for( i = 1; i < entities.Num(); i++ )
			{
				mapEnt = entities[i];
				if( !mapEnt->epairs.FindKey( "name" ) )
				{
					mapEnt->epairs.Set( "name", va( "%s%d", mapEnt->epairs.GetString( "classname", "forcedName" ), i ) );
				}
			}
		}

		// move the primitives of any func_group entities to the worldspawn
		if( entities[0]->epairs.GetBool( "moveFuncGroups" ) )
		{
			for( i = 1; i < entities.Num(); i++ )
			{
				mapEnt = entities[i];
				if( idStr::Icmp( mapEnt->epairs.GetString( "classname" ), "func_group" ) == 0 )
				{
					entities[0]->primitives.Append( mapEnt->primitives );
					mapEnt->primitives.Clear();
				}
			}
		}
	}

	hasPrimitiveData = true;
	return true;
}

/*
============
idMapFile::Write
============
*/
bool idMapFile::Write( const char* fileName, const char* ext, bool fromBasePath )
{
	int i;
	idStr qpath;
	idFile* fp;

	qpath = fileName;
	qpath.SetFileExtension( ext );

	idLib::common->Printf( "writing %s...\n", qpath.c_str() );

	if( fromBasePath )
	{
		fp = idLib::fileSystem->OpenFileWrite( qpath, "fs_basepath" );
	}
	else
	{
		fp = idLib::fileSystem->OpenExplicitFileWrite( qpath );
	}

	if( !fp )
	{
		idLib::common->Warning( "Couldn't open %s\n", qpath.c_str() );
		return false;
	}

	fp->WriteFloatString( "Version %f\n", ( float ) CURRENT_MAP_VERSION );

	for( i = 0; i < entities.Num(); i++ )
	{
		entities[i]->Write( fp, i );
	}

	idLib::fileSystem->CloseFile( fp );

	return true;
}

// RB begin
bool idMapFile::WriteJSON( const char* fileName, const char* ext, bool fromBasePath )
{
	int i;
	idStr qpath;
	idFile* fp;

	qpath = fileName;
	qpath.SetFileExtension( ext );

	idLib::common->Printf( "writing %s...\n", qpath.c_str() );

	if( fromBasePath )
	{
		fp = idLib::fileSystem->OpenFileWrite( qpath, "fs_basepath" );
	}
	else
	{
		fp = idLib::fileSystem->OpenExplicitFileWrite( qpath );
	}

	if( !fp )
	{
		idLib::common->Warning( "Couldn't open %s\n", qpath.c_str() );
		return false;
	}

	fp->Printf( "{\n" );
	fp->WriteFloatString( "\t\"version\": \"%f\",\n", ( float ) CURRENT_MAP_VERSION );
	fp->Printf( "\t\"entities\": \n\t[\n" );

	for( i = 0; i < entities.Num(); i++ )
	{
		entities[i]->WriteJSON( fp, i, entities.Num() );
	}

	fp->Printf( "\t]\n" );
	fp->Printf( "}\n" );

	idLib::fileSystem->CloseFile( fp );

	return true;
}
// RB end

/*
===============
idMapFile::SetGeometryCRC
===============
*/
void idMapFile::SetGeometryCRC()
{
	int i;

	geometryCRC = 0;
	for( i = 0; i < entities.Num(); i++ )
	{
		geometryCRC ^= entities[i]->GetGeometryCRC();
	}
}

/*
===============
idMapFile::AddEntity
===============
*/
int idMapFile::AddEntity( idMapEntity* mapEnt )
{
	int ret = entities.Append( mapEnt );
	return ret;
}

/*
===============
idMapFile::FindEntity
===============
*/
idMapEntity* idMapFile::FindEntity( const char* name )
{
	for( int i = 0; i < entities.Num(); i++ )
	{
		idMapEntity* ent = entities[i];
		if( idStr::Icmp( ent->epairs.GetString( "name" ), name ) == 0 )
		{
			return ent;
		}
	}
	return NULL;
}

/*
===============
idMapFile::RemoveEntity
===============
*/
void idMapFile::RemoveEntity( idMapEntity* mapEnt )
{
	entities.Remove( mapEnt );
	delete mapEnt;
}

/*
===============
idMapFile::RemoveEntity
===============
*/
void idMapFile::RemoveEntities( const char* classname )
{
	for( int i = 0; i < entities.Num(); i++ )
	{
		idMapEntity* ent = entities[i];
		if( idStr::Icmp( ent->epairs.GetString( "classname" ), classname ) == 0 )
		{
			delete entities[i];
			entities.RemoveIndex( i );
			i--;
		}
	}
}

/*
===============
idMapFile::RemoveAllEntities
===============
*/
void idMapFile::RemoveAllEntities()
{
	entities.DeleteContents( true );
	hasPrimitiveData = false;
}

/*
===============
idMapFile::RemovePrimitiveData
===============
*/
void idMapFile::RemovePrimitiveData()
{
	for( int i = 0; i < entities.Num(); i++ )
	{
		idMapEntity* ent = entities[i];
		ent->RemovePrimitiveData();
	}
	hasPrimitiveData = false;
}

/*
===============
idMapFile::NeedsReload
===============
*/
bool idMapFile::NeedsReload()
{
	if( name.Length() )
	{
		ID_TIME_T time = FILE_NOT_FOUND_TIMESTAMP;
		if( idLib::fileSystem->ReadFile( name, NULL, &time ) > 0 )
		{
			return ( time > fileTime );
		}
	}
	return true;
}


// RB begin
MapPolygonMesh::MapPolygonMesh()
{
	type = TYPE_MESH;
	originalType = TYPE_MESH;
	polygons.Resize( 8, 4 );

	contents = CONTENTS_SOLID;
	opaque = true;
}

void MapPolygonMesh::ConvertFromBrush( const idMapBrush* mapBrush, int entityNum, int primitiveNum )
{
	originalType = TYPE_BRUSH;

	// fix degenerate planes
	idPlane* planes = ( idPlane* ) _alloca16( mapBrush->GetNumSides() * sizeof( planes[0] ) );
	for( int i = 0; i < mapBrush->GetNumSides(); i++ )
	{
		planes[i] = mapBrush->GetSide( i )->GetPlane();
		planes[i].FixDegeneracies( DEGENERATE_DIST_EPSILON );
	}

	idList<idFixedWinding> planeWindings;
	idBounds bounds;
	bounds.Clear();

	int numVerts = 0;
	int numIndexes = 0;

	bool badBrush = false;

	for( int i = 0; i < mapBrush->GetNumSides(); i++ )
	{
		idMapBrushSide* mapSide = mapBrush->GetSide( i );

		const idMaterial* material = declManager->FindMaterial( mapSide->GetMaterial() );
		//contents |= ( material->GetContentFlags() & CONTENTS_REMOVE_UTIL );
		//materials.AddUnique( material );

		// chop base plane by other brush sides
		idFixedWinding& w = planeWindings.Alloc();
		w.BaseForPlane( -planes[i] );

		if( !w.GetNumPoints() )
		{
			common->Printf( "Entity %i, Brush %i: base winding has no points\n", entityNum, primitiveNum );
			badBrush = true;
			break;
		}

		for( int j = 0; j < mapBrush->GetNumSides() && w.GetNumPoints(); j++ )
		{
			if( i == j )
			{
				continue;
			}

			if( !w.ClipInPlace( -planes[j], 0 ) )
			{
				// no intersection
				//badBrush = true;
				common->Printf( "Entity %i, Brush %i: no intersection with other brush plane\n", entityNum, primitiveNum );
				//break;
			}
		}

		if( w.GetNumPoints() <= 2 )
		{
			continue;
		}

		// only used for debugging
		for( int j = 0; j < w.GetNumPoints(); j++ )
		{
			const idVec3& v = w[j].ToVec3();
			bounds.AddPoint( v );
		}
	}

	if( badBrush )
	{
		//common->Error( "" )
		return;
	}

	// copy the data from the windings and build polygons
	for( int i = 0; i < mapBrush->GetNumSides(); i++ )
	{
		idMapBrushSide* mapSide = mapBrush->GetSide( i );

		idFixedWinding& w = planeWindings[i];
		if( !w.GetNumPoints() )
		{
			continue;
		}

		MapPolygon& polygon = polygons.Alloc();
		polygon.SetMaterial( mapSide->GetMaterial() );


		//for( int j = 0; j < w.GetNumPoints(); j++ )

		// reverse order, so normal does not point inwards
		for( int j = w.GetNumPoints() - 1; j >= 0; j-- )
		{
			polygon.AddIndex( verts.Num() + j );
		}

		for( int j = 0; j < w.GetNumPoints(); j++ )
		{
			idDrawVert& dv = verts.Alloc();

			const idVec3& xyz = w[j].ToVec3();

			dv.xyz = xyz;

			// calculate texture s/t from brush primitive texture matrix
			idVec4 texVec[2];
			mapSide->GetTextureVectors( texVec );

			idVec2 st;
			st.x = ( xyz * texVec[0].ToVec3() ) + texVec[0][3];
			st.y = ( xyz * texVec[1].ToVec3() ) + texVec[1][3];

			// flip y
			//st.y = 1.0f - st.y;

			dv.SetTexCoord( st );

			// copy normal
			dv.SetNormal( mapSide->GetPlane().Normal() );

			//if( dv->GetNormal().Length() < 0.9 || dv->GetNormal().Length() > 1.1 )
			//{
			//	common->Error( "Bad normal in TriListForSide" );
			//}
		}
	}

	SetContents();
}

void MapPolygonMesh::ConvertFromPatch( const idMapPatch* patch, int entityNum, int primitiveNum )
{
	originalType = TYPE_PATCH;

	idSurface_Patch* cp = new idSurface_Patch( *patch );

	if( patch->GetExplicitlySubdivided() )
	{
		cp->SubdivideExplicit( patch->GetHorzSubdivisions(), patch->GetVertSubdivisions(), true );
	}
	else
	{
		cp->Subdivide( DEFAULT_CURVE_MAX_ERROR, DEFAULT_CURVE_MAX_ERROR, DEFAULT_CURVE_MAX_LENGTH, true );
	}

	for( int i = 0; i < cp->GetNumIndexes(); i += 3 )
	{
		verts.Append( ( *cp )[cp->GetIndexes()[i + 1]] );
		verts.Append( ( *cp )[cp->GetIndexes()[i + 2]] );
		verts.Append( ( *cp )[cp->GetIndexes()[i + 0]] );
	}

	for( int i = 0; i < cp->GetNumIndexes(); i += 3 )
	{
		MapPolygon& polygon = polygons.Alloc();
		polygon.SetMaterial( patch->GetMaterial() );

		polygon.AddIndex( i + 0 );
		polygon.AddIndex( i + 1 );
		polygon.AddIndex( i + 2 );
	}

	delete cp;

	SetContents();
}

bool MapPolygonMesh::Write( idFile* fp, int primitiveNum, const idVec3& origin ) const
{
	fp->WriteFloatString( "// primitive %d\n{\n meshDef\n {\n", primitiveNum );
	//fp->WriteFloatString( "  \"%s\"\n  ( %d %d 0 0 0 )\n", GetMaterial(), GetWidth(), GetHeight() );

	fp->WriteFloatString( "  ( %d %d 0 0 0 )\n", verts.Num(), polygons.Num() );

	fp->WriteFloatString( "  (\n" );
	idVec2 st;
	idVec3 n;
	for( int i = 0; i < verts.Num(); i++ )
	{
		const idDrawVert* v = &verts[ i ];
		st = v->GetTexCoord();
		n = v->GetNormalRaw();

		//fp->WriteFloatString( "   ( %f %f %f %f %f %f %f %f )\n", v->xyz[0] + origin[0], v->xyz[1] + origin[1], v->xyz[2] + origin[2], st[0], st[1], n[0], n[1], n[2] );
		fp->WriteFloatString( "   ( %f %f %f %f %f %f %f %f )\n", v->xyz[0], v->xyz[1], v->xyz[2], st[0], st[1], n[0], n[1], n[2] );
	}
	fp->WriteFloatString( "  )\n" );

	fp->WriteFloatString( "  (\n" );
	for( int i = 0; i < polygons.Num(); i++ )
	{
		const MapPolygon& poly = polygons[ i ];

		fp->WriteFloatString( "   \"%s\" %d = ", poly.GetMaterial(), poly.indexes.Num() );

		for( int j = 0; j < poly.indexes.Num(); j++ )
		{
			fp->WriteFloatString( "%d ", poly.indexes[j] );
		}
		fp->WriteFloatString( "\n" );
	}
	fp->WriteFloatString( "  )\n" );

	fp->WriteFloatString( " }\n}\n" );

	return true;
}

bool MapPolygonMesh::WriteJSON( idFile* fp, int primitiveNum, const idVec3& origin ) const
{
	fp->WriteFloatString( "\t\t\t\t{\n\t\t\t\t\t\"primitive\": \"%d\",\n", primitiveNum );

	if( originalType == TYPE_BRUSH )
	{
		fp->WriteFloatString( "\t\t\t\t\t\"original\": \"brush\",\n" );
	}
	else if( originalType == TYPE_PATCH )
	{
		fp->WriteFloatString( "\t\t\t\t\t\"original\": \"curve\",\n" );
	}

	fp->WriteFloatString( "\t\t\t\t\t\"verts\":\n\t\t\t\t\t[\n" );
	idVec2 st;
	idVec3 n;
	for( int i = 0; i < verts.Num(); i++ )
	{
		const idDrawVert& v = verts[ i ];
		st = v.GetTexCoord();
		n = v.GetNormalRaw();

		//if( IsNAN( v.xyz ) )
		//{
		//   continue;
		//}

		//idVec3 xyz = v.xyz - origin;
		fp->WriteFloatString( "\t\t\t\t\t\t{ \"xyz\": [%f, %f, %f], \"st\": [%f, %f], \"normal\": [%f, %f, %f] }%s\n", v.xyz[0], v.xyz[1], v.xyz[2], st[0], st[1], n[0], n[1], n[2], ( i == ( verts.Num() - 1 ) ) ? "" : "," );
	}
	fp->WriteFloatString( "\t\t\t\t\t],\n" );

	fp->WriteFloatString( "\t\t\t\t\t\"polygons\":\n\t\t\t\t\t[\n" );
	for( int i = 0; i < polygons.Num(); i++ )
	{
		const MapPolygon& poly = polygons[ i ];

		fp->WriteFloatString( "\t\t\t\t\t\t{ \"material\": \"%s\", \"indices\": [", poly.GetMaterial() );

#if 0
		for( int j = 0; j < poly.indexes.Num(); j++ )
		{
			fp->WriteFloatString( "%d%s", poly.indexes[j], ( j == poly.indexes.Num() - 1 ) ? "" : ", " );
		}
#else
		for( int j = poly.indexes.Num() - 1 ; j >= 0; j-- )
		{
			fp->WriteFloatString( "%d%s", poly.indexes[j], ( j == 0 ) ? "" : ", " );
		}
#endif
		fp->WriteFloatString( "] }%s\n", ( i == ( polygons.Num() - 1 ) ) ? "" : "," );
	}
	fp->WriteFloatString( "\t\t\t\t\t]\n" );

	fp->WriteFloatString( "\t\t\t\t}" );

	return true;
}

MapPolygonMesh* MapPolygonMesh::Parse( idLexer& src, const idVec3& origin, float version )
{
	float		info[7];
	idToken		token;
	int			i;

	if( !src.ExpectTokenString( "{" ) )
	{
		return NULL;
	}

	// Parse it
	if( !src.Parse1DMatrix( 5, info ) )
	{
		src.Error( "MapPolygonMesh::Parse: unable to parse meshDef info" );
		return NULL;
	}

	const int numVertices = ( int ) info[0];
	const int numPolygons = ( int ) info[1];

	MapPolygonMesh* mesh = new MapPolygonMesh();

	// parse vertices
	if( !src.ExpectTokenString( "(" ) )
	{
		src.Error( "MapPolygonMesh::Parse: bad mesh vertex data" );
		delete mesh;
		return NULL;
	}

	for( i = 0; i < numVertices; i++ )
	{
		float v[8];

		if( !src.Parse1DMatrix( 8, v ) )
		{
			src.Error( "MapPolygonMesh::Parse: bad vertex column data" );
			delete mesh;
			return NULL;
		}

		// TODO optimize: preallocate vertices
		//vert = &( ( *patch )[i * patch->GetWidth() + j] );

		idDrawVert vert;

		vert.xyz[0] = v[0];// - origin[0];
		vert.xyz[1] = v[1];// - origin[1];
		vert.xyz[2] = v[2];// - origin[2];
		vert.SetTexCoord( v[3], v[4] );

		idVec3 n( v[5], v[6], v[7] );
		vert.SetNormal( n );

		mesh->AddVertex( vert );
	}

	if( !src.ExpectTokenString( ")" ) )
	{
		delete mesh;
		src.Error( "MapPolygonMesh::Parse: unable to parse vertices" );
		return NULL;
	}

	// parse polygons
	if( !src.ExpectTokenString( "(" ) )
	{
		src.Error( "MapPolygonMesh::Parse: bad mesh polygon data" );
		delete mesh;
		return NULL;
	}

	for( i = 0; i < numPolygons; i++ )
	{
		// get material name
		MapPolygon& polygon = mesh->polygons.Alloc();

		src.ReadToken( &token );
		if( token.type == TT_STRING )
		{
			polygon.SetMaterial( token );;
		}
		else
		{
			src.Error( "MapPolygonMesh::Parse: bad mesh polygon data" );
			delete mesh;
			return NULL;
		}

		int numIndexes = src.ParseInt();

		if( !src.ExpectTokenString( "=" ) )
		{
			src.Error( "MapPolygonMesh::Parse: bad mesh polygon data" );
			delete mesh;
			return NULL;
		}

		//idTempArray<int> indexes( numIndexes );
		for( int j = 0; j < numIndexes; j++ )
		{
			//indexes[j] = src.ParseInt();

			int index = src.ParseInt();
			polygon.AddIndex( index );
		}

		//polygon->SetIndexes( indexes );
	}

	if( !src.ExpectTokenString( ")" ) )
	{
		delete mesh;
		src.Error( "MapPolygonMesh::Parse: unable to parse polygons" );
		return NULL;
	}

	if( !src.ExpectTokenString( "}" ) )
	{
		delete mesh;
		src.Error( "MapPolygonMesh::Parse: unable to parse mesh primitive end" );
		return NULL;
	}

	if( !src.ExpectTokenString( "}" ) )
	{
		delete mesh;
		src.Error( "MapPolygonMesh::Parse: unable to parse mesh primitive end" );
		return NULL;
	}

	mesh->SetContents();

	return mesh;
}

MapPolygonMesh* MapPolygonMesh::ParseJSON( idLexer& src )
{
	idToken		token;

	MapPolygonMesh* mesh = new MapPolygonMesh();

	while( true )
	{
		if( !src.ReadToken( &token ) )
		{
			src.Error( "MapPolygonMesh::ParseJSON: EOF without closing brace" );
			return NULL;
		}

		if( token == "}" )
		{
			break;
		}

		if( token == "," )
		{
			continue;
		}

		if( token == "verts" )
		{
			idDrawVert vert;
			float v[8];

			while( true )
			{
				if( !src.ReadToken( &token ) )
				{
					src.Error( "MapPolygonMesh::ParseJSON: EOF without closing brace" );
					return NULL;
				}

				if( token == "}" )
				{
					mesh->AddVertex( vert );
					continue;
				}

				if( token == "]" )
				{
					break;
				}

				if( token == "," )
				{
					continue;
				}

				if( token == "xyz" )
				{
					if( !src.ExpectTokenString( ":" ) )
					{
						delete mesh;
						src.Error( "MapPolygonMesh::ParseJSON: EOF without closing brace" );
						return NULL;
					}

					if( !src.Parse1DMatrixJSON( 3, v ) )
					{
						delete mesh;
						src.Error( "MapPolygonMesh::ParseJSON: bad vertex column data" );
						return NULL;
					}

					vert.xyz[0] = v[0];
					vert.xyz[1] = v[1];
					vert.xyz[2] = v[2];
				}
				else if( token == "st" )
				{
					if( !src.ExpectTokenString( ":" ) )
					{
						delete mesh;
						src.Error( "MapPolygonMesh::ParseJSON: EOF without closing brace" );
						return NULL;
					}

					if( !src.Parse1DMatrixJSON( 2, v ) )
					{
						delete mesh;
						src.Error( "MapPolygonMesh::ParseJSON: bad vertex column data" );
						return NULL;
					}

					vert.SetTexCoord( v[0], v[1] );
				}
				else if( token == "normal" )
				{
					if( !src.ExpectTokenString( ":" ) )
					{
						delete mesh;
						src.Error( "MapPolygonMesh::ParseJSON: EOF without closing brace" );
						return NULL;
					}

					if( !src.Parse1DMatrixJSON( 3, v ) )
					{
						delete mesh;
						src.Error( "MapPolygonMesh::ParseJSON: bad vertex column data" );
						return NULL;
					}

					idVec3 n( v[0], v[1], v[2] );
					vert.SetNormal( n );
				}
			}
		}


		if( token == "polygons" )
		{
			MapPolygon* polygon = NULL;

			while( true )
			{
				if( !src.ReadToken( &token ) )
				{
					src.Error( "MapPolygonMesh::ParseJSON: EOF without closing brace" );
					return NULL;
				}

				if( token == "{" )
				{
					polygon = &mesh->polygons.Alloc();
					continue;
				}

				if( token == "]" )
				{
					break;
				}

				if( token == "," )
				{
					continue;
				}

				if( token == "material" )
				{
					if( !src.ExpectTokenString( ":" ) )
					{
						delete mesh;
						src.Error( "MapPolygonMesh::ParseJSON: EOF without closing brace" );
						return NULL;
					}

					src.ReadToken( &token );
					if( token.type == TT_STRING )
					{
						polygon->SetMaterial( token );
					}
				}
				else if( token == "indices" )
				{
					idList<int> indices;

					while( true )
					{
						if( !src.ReadToken( &token ) )
						{
							src.Error( "MapPolygonMesh::ParseJSON: EOF without closing brace" );
							return NULL;
						}

						if( token == "]" )
						{
							// reverse order from Blender
							for( int i = indices.Num() - 1; i >= 0; i-- )
							{
								polygon->AddIndex( indices[i] );
							}
							break;
						}
						else if( token.type == TT_NUMBER )
						{
							int index = token.GetIntValue();
							indices.Append( index );
						}
						else if( token == "," )
						{
							continue;
						}
					}
				}
			}
		}
	}

	mesh->SetContents();

	return mesh;
}

void MapPolygonMesh::SetContents()
{
	if( polygons.Num() < 1 )
	{
		contents = CONTENTS_SOLID;
		opaque = true;

		return;
	}

	int			c2;

	MapPolygon* poly = &polygons[0];

	const idMaterial* mat = declManager->FindMaterial( poly->GetMaterial() );
	contents = mat->GetContentFlags();

	//b->contentShader = s->material;
	bool mixed = false;

	// a brush is only opaque if all sides are opaque
	opaque = true;

	for( int i = 1 ; i < polygons.Num() ; i++ )
	{
		poly = &polygons[i];

		const idMaterial* mat2 = declManager->FindMaterial( poly->GetMaterial() );

		c2 = mat2->GetContentFlags();
		if( c2 != contents )
		{
			mixed = true;
			contents |= c2;
		}

		if( mat2->Coverage() != MC_OPAQUE )
		{
			opaque = false;
		}
	}
}

unsigned int MapPolygonMesh::GetGeometryCRC() const
{
	int i;
	unsigned int crc;

	crc = 0;
	for( i = 0; i < verts.Num(); i++ )
	{
		crc ^= FloatCRC( verts[i].xyz.x );
		crc ^= FloatCRC( verts[i].xyz.y );
		crc ^= FloatCRC( verts[i].xyz.z );
	}

	for( i = 0; i < polygons.Num(); i++ )
	{
		const MapPolygon& poly = polygons[i];

		crc ^= StringCRC( poly.GetMaterial() );
	}

	return crc;
}

bool MapPolygonMesh::IsAreaportal() const
{
	return ( ( contents & CONTENTS_AREAPORTAL ) != 0 );
}

void MapPolygonMesh::GetBounds( idBounds& bounds ) const
{
	if( !verts.Num() )
	{
		bounds.Clear();
		return;
	}


	bounds[0] = bounds[1] = verts[0].xyz;
	for( int i = 1; i < verts.Num(); i++ )
	{
		const idVec3& p = verts[i].xyz;

		if( p.x < bounds[0].x )
		{
			bounds[0].x = p.x;
		}
		else if( p.x > bounds[1].x )
		{
			bounds[1].x = p.x;
		}
		if( p.y < bounds[0].y )
		{
			bounds[0].y = p.y;
		}
		else if( p.y > bounds[1].y )
		{
			bounds[1].y = p.y;
		}
		if( p.z < bounds[0].z )
		{
			bounds[0].z = p.z;
		}
		else if( p.z > bounds[1].z )
		{
			bounds[1].z = p.z;
		}
	}
}

bool idMapFile::ConvertToPolygonMeshFormat()
{
	int count = GetNumEntities();
	for( int j = 0; j < count; j++ )
	{
		idMapEntity* ent = GetEntity( j );
		if( ent )
		{
			idStr classname = ent->epairs.GetString( "classname" );

			//if( classname == "worldspawn" )
			{
				for( int i = 0; i < ent->GetNumPrimitives(); i++ )
				{
					idMapPrimitive*	mapPrim;

					mapPrim = ent->GetPrimitive( i );
					if( mapPrim->GetType() == idMapPrimitive::TYPE_BRUSH )
					{
						MapPolygonMesh* meshPrim = new MapPolygonMesh();
						meshPrim->epairs.Copy( mapPrim->epairs );

						meshPrim->ConvertFromBrush( static_cast<idMapBrush*>( mapPrim ), j, i );
						ent->primitives[ i ] = meshPrim;

						delete mapPrim;

						continue;
					}
					else if( mapPrim->GetType() == idMapPrimitive::TYPE_PATCH )
					{
						MapPolygonMesh* meshPrim = new MapPolygonMesh();
						meshPrim->epairs.Copy( mapPrim->epairs );

						meshPrim->ConvertFromPatch( static_cast<idMapPatch*>( mapPrim ), j, i );
						ent->primitives[ i ] = meshPrim;

						delete mapPrim;

						continue;
					}
				}
			}
		}
	}

	return true;
}

// RB end
