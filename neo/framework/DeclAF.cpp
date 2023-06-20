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

/*
===============================================================================

	idDeclAF

===============================================================================
*/

/*
================
idAFVector::idAFVector
================
*/
idAFVector::idAFVector()
{
	type = VEC_COORDS;
	vec.Zero();
	negate = false;
}

/*
================
idAFVector::Parse
================
*/
bool idAFVector::Parse( idLexer& src )
{
	idToken token;

	if( !src.ReadToken( &token ) )
	{
		return false;
	}

	if( token == "-" )
	{
		negate = true;
		if( !src.ReadToken( &token ) )
		{
			return false;
		}
	}
	else
	{
		negate = false;
	}

	if( token == "(" )
	{
		type = idAFVector::VEC_COORDS;
		vec.x = src.ParseFloat();
		src.ExpectTokenString( "," );
		vec.y = src.ParseFloat();
		src.ExpectTokenString( "," );
		vec.z = src.ParseFloat();
		src.ExpectTokenString( ")" );
	}
	else if( token == "joint" )
	{
		type = idAFVector::VEC_JOINT;
		src.ExpectTokenString( "(" );
		src.ReadToken( &token );
		joint1 = token;
		src.ExpectTokenString( ")" );
	}
	else if( token == "bonecenter" )
	{
		type = idAFVector::VEC_BONECENTER;
		src.ExpectTokenString( "(" );
		src.ReadToken( &token );
		joint1 = token;
		src.ExpectTokenString( "," );
		src.ReadToken( &token );
		joint2 = token;
		src.ExpectTokenString( ")" );
	}
	else if( token == "bonedir" )
	{
		type = idAFVector::VEC_BONEDIR;
		src.ExpectTokenString( "(" );
		src.ReadToken( &token );
		joint1 = token;
		src.ExpectTokenString( "," );
		src.ReadToken( &token );
		joint2 = token;
		src.ExpectTokenString( ")" );
	}
	else
	{
		src.Error( "unknown token %s in vector", token.c_str() );
		return false;
	}

	return true;
}

/*
================
idAFVector::Finish
================
*/
bool idAFVector::Finish( const char* fileName, const getJointTransform_t GetJointTransform, const idJointMat* frame, void* model ) const
{
	idMat3 axis;
	idVec3 start, end;

	switch( type )
	{
		case idAFVector::VEC_COORDS:
		{
			break;
		}
		case idAFVector::VEC_JOINT:
		{
			if( !GetJointTransform( model, frame, joint1, vec, axis ) )
			{
				common->Warning( "invalid joint %s in joint() in '%s'", joint1.c_str(), fileName );
				vec.Zero();
			}
			break;
		}
		case idAFVector::VEC_BONECENTER:
		{
			if( !GetJointTransform( model, frame, joint1, start, axis ) )
			{
				common->Warning( "invalid joint %s in bonecenter() in '%s'", joint1.c_str(), fileName );
				start.Zero();
			}
			if( !GetJointTransform( model, frame, joint2, end, axis ) )
			{
				common->Warning( "invalid joint %s in bonecenter() in '%s'", joint2.c_str(), fileName );
				end.Zero();
			}
			vec = ( start + end ) * 0.5f;
			break;
		}
		case idAFVector::VEC_BONEDIR:
		{
			if( !GetJointTransform( model, frame, joint1, start, axis ) )
			{
				common->Warning( "invalid joint %s in bonedir() in '%s'", joint1.c_str(), fileName );
				start.Zero();
			}
			if( !GetJointTransform( model, frame, joint2, end, axis ) )
			{
				common->Warning( "invalid joint %s in bonedir() in '%s'", joint2.c_str(), fileName );
				end.Zero();
			}
			vec = ( end - start );
			break;
		}
		default:
		{
			vec.Zero();
			break;
		}
	}

	if( negate )
	{
		vec = -vec;
	}

	return true;
}

/*
================
idAFVector::Write
================
*/
bool idAFVector::Write( idFile* f ) const
{

	if( negate )
	{
		f->WriteFloatString( "-" );
	}
	switch( type )
	{
		case idAFVector::VEC_COORDS:
		{
			f->WriteFloatString( "( %f, %f, %f )", vec.x, vec.y, vec.z );
			break;
		}
		case idAFVector::VEC_JOINT:
		{
			f->WriteFloatString( "joint( \"%s\" )", joint1.c_str() );
			break;
		}
		case idAFVector::VEC_BONECENTER:
		{
			f->WriteFloatString( "bonecenter( \"%s\", \"%s\" )", joint1.c_str(), joint2.c_str() );
			break;
		}
		case idAFVector::VEC_BONEDIR:
		{
			f->WriteFloatString( "bonedir( \"%s\", \"%s\" )", joint1.c_str(), joint2.c_str() );
			break;
		}
		default:
		{
			break;
		}
	}
	return true;
}

/*
================
idAFVector::ToString
================
*/
const char* idAFVector::ToString( idStr& str, const int precision )
{

	switch( type )
	{
		case idAFVector::VEC_COORDS:
		{
			char format[128];
			idStr::snPrintf( format, sizeof( format ), "( %%.%df, %%.%df, %%.%df )", precision, precision, precision );
			sprintf( str, format, vec.x, vec.y, vec.z );
			break;
		}
		case idAFVector::VEC_JOINT:
		{
			sprintf( str, "joint( \"%s\" )", joint1.c_str() );
			break;
		}
		case idAFVector::VEC_BONECENTER:
		{
			sprintf( str, "bonecenter( \"%s\", \"%s\" )", joint1.c_str(), joint2.c_str() );
			break;
		}
		case idAFVector::VEC_BONEDIR:
		{
			sprintf( str, "bonedir( \"%s\", \"%s\" )", joint1.c_str(), joint2.c_str() );
			break;
		}
		default:
		{
			break;
		}
	}
	if( negate )
	{
		str = "-" + str;
	}
	return str.c_str();
}

/*
================
idDeclAF_Body::SetDefault
================
*/
void idDeclAF_Body::SetDefault( const idDeclAF* file )
{
	name = "noname";
	modelType = TRM_BOX;
	v1.type = idAFVector::VEC_COORDS;
	v1.ToVec3().x = v1.ToVec3().y = v1.ToVec3().z = -10.0f;
	v2.type = idAFVector::VEC_COORDS;
	v2.ToVec3().x = v2.ToVec3().y = v2.ToVec3().z = 10.0f;
	numSides = 3;
	origin.ToVec3().Zero();
	angles.Zero();
	density = 0.2f;
	inertiaScale.Identity();
	linearFriction = file->defaultLinearFriction;
	angularFriction = file->defaultAngularFriction;
	contactFriction = file->defaultContactFriction;
	contents = file->contents;
	clipMask = file->clipMask;
	selfCollision = file->selfCollision;
	frictionDirection.ToVec3().Zero();
	contactMotorDirection.ToVec3().Zero();
	jointName = "origin";
	jointMod = DECLAF_JOINTMOD_AXIS;
	containedJoints = "*origin";
}

/*
================
idDeclAF_Constraint::SetDefault
================
*/
void idDeclAF_Constraint::SetDefault( const idDeclAF* file )
{
	name = "noname";
	type = DECLAF_CONSTRAINT_UNIVERSALJOINT;
	if( file->bodies.Num() )
	{
		body1 = file->bodies[0]->name;
	}
	else
	{
		body1 = "world";
	}
	body2 = "world";
	friction = file->defaultConstraintFriction;
	anchor.ToVec3().Zero();
	anchor2.ToVec3().Zero();
	axis.ToVec3().Set( 1.0f, 0.0f, 0.0f );
	shaft[0].ToVec3().Set( 0.0f, 0.0f, -1.0f );
	shaft[1].ToVec3().Set( 0.0f, 0.0f, 1.0f );
	limit = idDeclAF_Constraint::LIMIT_NONE;
	limitAngles[0] =
		limitAngles[1] =
			limitAngles[2] = 0.0f;
	limitAxis.ToVec3().Set( 0.0f, 0.0f, -1.0f );
}

/*
================
idDeclAF::WriteBody
================
*/
bool idDeclAF::WriteBody( idFile* f, const idDeclAF_Body& body ) const
{
	idStr str;

	f->WriteFloatString( "\nbody \"%s\" {\n", body.name.c_str() );
	f->WriteFloatString( "\tjoint \"%s\"\n", body.jointName.c_str() );
	f->WriteFloatString( "\tmod %s\n", JointModToString( body.jointMod ) );
	switch( body.modelType )
	{
		case TRM_BOX:
		{
			f->WriteFloatString( "\tmodel box( " );
			body.v1.Write( f );
			f->WriteFloatString( ", " );
			body.v2.Write( f );
			f->WriteFloatString( " )\n" );
			break;
		}
		case TRM_OCTAHEDRON:
		{
			f->WriteFloatString( "\tmodel octahedron( " );
			body.v1.Write( f );
			f->WriteFloatString( ", " );
			body.v2.Write( f );
			f->WriteFloatString( " )\n" );
			break;
		}
		case TRM_DODECAHEDRON:
		{
			f->WriteFloatString( "\tmodel dodecahedron( " );
			body.v1.Write( f );
			f->WriteFloatString( ", " );
			body.v2.Write( f );
			f->WriteFloatString( " )\n" );
			break;
		}
		case TRM_CYLINDER:
		{
			f->WriteFloatString( "\tmodel cylinder( " );
			body.v1.Write( f );
			f->WriteFloatString( ", " );
			body.v2.Write( f );
			f->WriteFloatString( ", %d )\n", body.numSides );
			break;
		}
		case TRM_CONE:
		{
			f->WriteFloatString( "\tmodel cone( " );
			body.v1.Write( f );
			f->WriteFloatString( ", " );
			body.v2.Write( f );
			f->WriteFloatString( ", %d )\n", body.numSides );
			break;
		}
		case TRM_BONE:
		{
			f->WriteFloatString( "\tmodel bone( " );
			body.v1.Write( f );
			f->WriteFloatString( ", " );
			body.v2.Write( f );
			f->WriteFloatString( ", %f )\n", body.width );
			break;
		}
		default:
			assert( 0 );
			break;
	}
	f->WriteFloatString( "\torigin " );
	body.origin.Write( f );
	f->WriteFloatString( "\n" );
	if( body.angles != ang_zero )
	{
		f->WriteFloatString( "\tangles ( %f, %f, %f )\n", body.angles.pitch, body.angles.yaw, body.angles.roll );
	}
	f->WriteFloatString( "\tdensity %f\n", body.density );
	if( body.inertiaScale != mat3_identity )
	{
		const idMat3& ic = body.inertiaScale;
		f->WriteFloatString( "\tinertiaScale (%f %f %f %f %f %f %f %f %f)\n",
							 ic[0][0], ic[0][1], ic[0][2],
							 ic[1][0], ic[1][1], ic[1][2],
							 ic[2][0], ic[2][1], ic[2][2] );
	}
	if( body.linearFriction != -1 )
	{
		f->WriteFloatString( "\tfriction %f, %f, %f\n", body.linearFriction, body.angularFriction, body.contactFriction );
	}
	f->WriteFloatString( "\tcontents %s\n", ContentsToString( body.contents, str ) );
	f->WriteFloatString( "\tclipMask %s\n", ContentsToString( body.clipMask, str ) );
	f->WriteFloatString( "\tselfCollision %d\n", body.selfCollision );
	if( body.frictionDirection.ToVec3() != vec3_origin )
	{
		f->WriteFloatString( "\tfrictionDirection " );
		body.frictionDirection.Write( f );
		f->WriteFloatString( "\n" );
	}
	if( body.contactMotorDirection.ToVec3() != vec3_origin )
	{
		f->WriteFloatString( "\tcontactMotorDirection " );
		body.contactMotorDirection.Write( f );
		f->WriteFloatString( "\n" );
	}
	f->WriteFloatString( "\tcontainedJoints \"%s\"\n", body.containedJoints.c_str() );
	f->WriteFloatString( "}\n" );
	return true;
}

/*
================
idDeclAF::WriteFixed
================
*/
bool idDeclAF::WriteFixed( idFile* f, const idDeclAF_Constraint& c ) const
{
	f->WriteFloatString( "\nfixed \"%s\" {\n", c.name.c_str() );
	f->WriteFloatString( "\tbody1 \"%s\"\n", c.body1.c_str() );
	f->WriteFloatString( "\tbody2 \"%s\"\n", c.body2.c_str() );
	f->WriteFloatString( "}\n" );
	return true;
}

/*
================
idDeclAF::WriteBallAndSocketJoint
================
*/
bool idDeclAF::WriteBallAndSocketJoint( idFile* f, const idDeclAF_Constraint& c ) const
{
	f->WriteFloatString( "\nballAndSocketJoint \"%s\" {\n", c.name.c_str() );
	f->WriteFloatString( "\tbody1 \"%s\"\n", c.body1.c_str() );
	f->WriteFloatString( "\tbody2 \"%s\"\n", c.body2.c_str() );
	f->WriteFloatString( "\tanchor " );
	c.anchor.Write( f );
	f->WriteFloatString( "\n" );
	f->WriteFloatString( "\tfriction %f\n", c.friction );
	if( c.limit == idDeclAF_Constraint::LIMIT_CONE )
	{
		f->WriteFloatString( "\tconeLimit " );
		c.limitAxis.Write( f );
		f->WriteFloatString( ", %f, ", c.limitAngles[0] );
		c.shaft[0].Write( f );
		f->WriteFloatString( "\n" );
	}
	else if( c.limit == idDeclAF_Constraint::LIMIT_PYRAMID )
	{
		f->WriteFloatString( "\tpyramidLimit " );
		c.limitAxis.Write( f );
		f->WriteFloatString( ", %f, %f, %f, ", c.limitAngles[0], c.limitAngles[1], c.limitAngles[2] );
		c.shaft[0].Write( f );
		f->WriteFloatString( "\n" );
	}
	f->WriteFloatString( "}\n" );
	return true;
}

/*
================
idDeclAF::WriteUniversalJoint
================
*/
bool idDeclAF::WriteUniversalJoint( idFile* f, const idDeclAF_Constraint& c ) const
{
	f->WriteFloatString( "\nuniversalJoint \"%s\" {\n", c.name.c_str() );
	f->WriteFloatString( "\tbody1 \"%s\"\n", c.body1.c_str() );
	f->WriteFloatString( "\tbody2 \"%s\"\n", c.body2.c_str() );
	f->WriteFloatString( "\tanchor " );
	c.anchor.Write( f );
	f->WriteFloatString( "\n" );
	f->WriteFloatString( "\tshafts " );
	c.shaft[0].Write( f );
	f->WriteFloatString( ", " );
	c.shaft[1].Write( f );
	f->WriteFloatString( "\n" );
	f->WriteFloatString( "\tfriction %f\n", c.friction );
	if( c.limit == idDeclAF_Constraint::LIMIT_CONE )
	{
		f->WriteFloatString( "\tconeLimit " );
		c.limitAxis.Write( f );
		f->WriteFloatString( ", %f\n", c.limitAngles[0] );
	}
	else if( c.limit == idDeclAF_Constraint::LIMIT_PYRAMID )
	{
		f->WriteFloatString( "\tpyramidLimit " );
		c.limitAxis.Write( f );
		f->WriteFloatString( ", %f, %f, %f\n", c.limitAngles[0], c.limitAngles[1], c.limitAngles[2] );
	}
	f->WriteFloatString( "}\n" );
	return true;
}

/*
================
idDeclAF::WriteHinge
================
*/
bool idDeclAF::WriteHinge( idFile* f, const idDeclAF_Constraint& c ) const
{
	f->WriteFloatString( "\nhinge \"%s\" {\n", c.name.c_str() );
	f->WriteFloatString( "\tbody1 \"%s\"\n", c.body1.c_str() );
	f->WriteFloatString( "\tbody2 \"%s\"\n", c.body2.c_str() );
	f->WriteFloatString( "\tanchor " );
	c.anchor.Write( f );
	f->WriteFloatString( "\n" );
	f->WriteFloatString( "\taxis " );
	c.axis.Write( f );
	f->WriteFloatString( "\n" );
	f->WriteFloatString( "\tfriction %f\n", c.friction );
	if( c.limit == idDeclAF_Constraint::LIMIT_CONE )
	{
		f->WriteFloatString( "\tlimit " );
		f->WriteFloatString( "%f, %f, %f", c.limitAngles[0], c.limitAngles[1], c.limitAngles[2] );
		f->WriteFloatString( "\n" );
	}
	f->WriteFloatString( "}\n" );
	return true;
}

/*
================
idDeclAF::WriteSlider
================
*/
bool idDeclAF::WriteSlider( idFile* f, const idDeclAF_Constraint& c ) const
{
	f->WriteFloatString( "\nslider \"%s\" {\n", c.name.c_str() );
	f->WriteFloatString( "\tbody1 \"%s\"\n", c.body1.c_str() );
	f->WriteFloatString( "\tbody2 \"%s\"\n", c.body2.c_str() );
	f->WriteFloatString( "\taxis " );
	c.axis.Write( f );
	f->WriteFloatString( "\n" );
	f->WriteFloatString( "\tfriction %f\n", c.friction );
	f->WriteFloatString( "}\n" );
	return true;
}

/*
================
idDeclAF::WriteSpring
================
*/
bool idDeclAF::WriteSpring( idFile* f, const idDeclAF_Constraint& c ) const
{
	f->WriteFloatString( "\nspring \"%s\" {\n", c.name.c_str() );
	f->WriteFloatString( "\tbody1 \"%s\"\n", c.body1.c_str() );
	f->WriteFloatString( "\tbody2 \"%s\"\n", c.body2.c_str() );
	f->WriteFloatString( "\tanchor1 " );
	c.anchor.Write( f );
	f->WriteFloatString( "\n" );
	f->WriteFloatString( "\tanchor2 " );
	c.anchor2.Write( f );
	f->WriteFloatString( "\n" );
	f->WriteFloatString( "\tfriction %f\n", c.friction );
	f->WriteFloatString( "\tstretch %f\n", c.stretch );
	f->WriteFloatString( "\tcompress %f\n", c.compress );
	f->WriteFloatString( "\tdamping %f\n", c.damping );
	f->WriteFloatString( "\trestLength %f\n", c.restLength );
	f->WriteFloatString( "\tminLength %f\n", c.minLength );
	f->WriteFloatString( "\tmaxLength %f\n", c.maxLength );
	f->WriteFloatString( "}\n" );
	return true;
}

/*
================
idDeclAF::WriteConstraint
================
*/
bool idDeclAF::WriteConstraint( idFile* f, const idDeclAF_Constraint& c ) const
{
	switch( c.type )
	{
		case DECLAF_CONSTRAINT_FIXED:
			return WriteFixed( f, c );
		case DECLAF_CONSTRAINT_BALLANDSOCKETJOINT:
			return WriteBallAndSocketJoint( f, c );
		case DECLAF_CONSTRAINT_UNIVERSALJOINT:
			return WriteUniversalJoint( f, c );
		case DECLAF_CONSTRAINT_HINGE:
			return WriteHinge( f, c );
		case DECLAF_CONSTRAINT_SLIDER:
			return WriteSlider( f, c );
		case DECLAF_CONSTRAINT_SPRING:
			return WriteSpring( f, c );
		default:
			break;
	}
	return false;
}

/*
================
idDeclAF::WriteSettings
================
*/
bool idDeclAF::WriteSettings( idFile* f ) const
{
	idStr str;

	f->WriteFloatString( "\nsettings {\n" );
	f->WriteFloatString( "\tmodel \"%s\"\n", model.c_str() );
	f->WriteFloatString( "\tskin \"%s\"\n",	skin.c_str() );
	f->WriteFloatString( "\tfriction %f, %f, %f, %f\n", defaultLinearFriction, defaultAngularFriction, defaultContactFriction, defaultConstraintFriction );
	f->WriteFloatString( "\tsuspendSpeed %f, %f, %f, %f\n", suspendVelocity[0], suspendVelocity[1], suspendAcceleration[0], suspendAcceleration[1] );
	f->WriteFloatString( "\tnoMoveTime %f\n", noMoveTime );
	f->WriteFloatString( "\tnoMoveTranslation %f\n", noMoveTranslation );
	f->WriteFloatString( "\tnoMoveRotation %f\n", noMoveRotation );
	f->WriteFloatString( "\tminMoveTime %f\n", minMoveTime );
	f->WriteFloatString( "\tmaxMoveTime %f\n", maxMoveTime );
	f->WriteFloatString( "\ttotalMass %f\n", totalMass );
	f->WriteFloatString( "\tcontents %s\n", ContentsToString( contents, str ) );
	f->WriteFloatString( "\tclipMask %s\n", ContentsToString( clipMask, str ) );
	f->WriteFloatString( "\tselfCollision %d\n", selfCollision );
	f->WriteFloatString( "}\n" );
	return true;
}


/*
================
idDeclAF::RebuildTextSource
================
*/
bool idDeclAF::RebuildTextSource()
{
	int i;
	idFile_Memory f;

	f.WriteFloatString( "\n\n/*\n"
						"\tGenerated by the Articulated Figure Editor.\n"
						"\tDo not edit directly but launch the game and type 'editAFs' on the console.\n"
						"*/\n" );

	f.WriteFloatString( "\narticulatedFigure %s {\n", GetName() );

	if( !WriteSettings( &f ) )
	{
		return false;
	}

	for( i = 0; i < bodies.Num(); i++ )
	{
		if( !WriteBody( &f, *bodies[i] ) )
		{
			return false;
		}
	}

	for( i = 0; i < constraints.Num(); i++ )
	{
		if( !WriteConstraint( &f, *constraints[i] ) )
		{
			return false;
		}
	}

	f.WriteFloatString( "\n}" );

	SetText( f.GetDataPtr() );

	return true;
}

/*
================
idDeclAF::Save
================
*/
bool idDeclAF::Save()
{
	RebuildTextSource();
	ReplaceSourceFileText();
	modified = false;
	return true;
}

/*
================
idDeclAF::ContentsFromString
================
*/
int idDeclAF::ContentsFromString( const char* str )
{
	int c;
	idToken token;
	idLexer src( str, idStr::Length( str ), "idDeclAF::ContentsFromString" );

	c = 0;
	while( src.ReadToken( &token ) )
	{
		if( token.Icmp( "none" ) == 0 )
		{
			c = 0;
		}
		else if( token.Icmp( "solid" ) == 0 )
		{
			c |= CONTENTS_SOLID;
		}
		else if( token.Icmp( "body" ) == 0 )
		{
			c |= CONTENTS_BODY;
		}
		else if( token.Icmp( "corpse" ) == 0 )
		{
			c |= CONTENTS_CORPSE;
		}
		else if( token.Icmp( "playerclip" ) == 0 )
		{
			c |= CONTENTS_PLAYERCLIP;
		}
		else if( token.Icmp( "monsterclip" ) == 0 )
		{
			c |= CONTENTS_MONSTERCLIP;
		}
		else if( token == "," )
		{
			continue;
		}
		else
		{
			return c;
		}
	}
	return c;
}

/*
================
idDeclAF::ContentsToString
================
*/
const char* idDeclAF::ContentsToString( const int contents, idStr& str )
{
	str = "";
	if( contents & CONTENTS_SOLID )
	{
		if( str.Length() )
		{
			str += ", ";
		}
		str += "solid";
	}
	if( contents & CONTENTS_BODY )
	{
		if( str.Length() )
		{
			str += ", ";
		}
		str += "body";
	}
	if( contents & CONTENTS_CORPSE )
	{
		if( str.Length() )
		{
			str += ", ";
		}
		str += "corpse";
	}
	if( contents & CONTENTS_PLAYERCLIP )
	{
		if( str.Length() )
		{
			str += ", ";
		}
		str += "playerclip";
	}
	if( contents & CONTENTS_MONSTERCLIP )
	{
		if( str.Length() )
		{
			str += ", ";
		}
		str += "monsterclip";
	}
	if( str[0] == '\0' )
	{
		str = "none";
	}
	return str.c_str();
}

/*
================
idDeclAF::JointModFromString
================
*/
declAFJointMod_t idDeclAF::JointModFromString( const char* str )
{
	if( idStr::Icmp( str, "orientation" ) == 0 )
	{
		return DECLAF_JOINTMOD_AXIS;
	}
	if( idStr::Icmp( str, "position" ) == 0 )
	{
		return DECLAF_JOINTMOD_ORIGIN;
	}
	if( idStr::Icmp( str, "both" ) == 0 )
	{
		return DECLAF_JOINTMOD_BOTH;
	}
	return DECLAF_JOINTMOD_AXIS;
}

/*
================
idDeclAF::JointModToString
================
*/
const char* idDeclAF::JointModToString( declAFJointMod_t jointMod )
{
	switch( jointMod )
	{
		case DECLAF_JOINTMOD_AXIS:
		{
			return "orientation";
		}
		case DECLAF_JOINTMOD_ORIGIN:
		{
			return "position";
		}
		case DECLAF_JOINTMOD_BOTH:
		{
			return "both";
		}
	}
	return "orientation";
}

/*
=================
idDeclAF::Size
=================
*/
size_t idDeclAF::Size() const
{
	return sizeof( idDeclAF );
}

/*
================
idDeclAF::ParseContents
================
*/
bool idDeclAF::ParseContents( idLexer& src, int& c ) const
{
	idToken token;
	idStr str;

	while( src.ReadToken( &token ) )
	{
		str += token;
		if( !src.CheckTokenString( "," ) )
		{
			break;
		}
		str += ",";
	}
	c = ContentsFromString( str );
	return true;
}

/*
================
idDeclAF::ParseBody
================
*/
bool idDeclAF::ParseBody( idLexer& src )
{
	bool hasJoint = false;
	idToken token;
	idAFVector angles;
	idDeclAF_Body* body = new( TAG_DECL ) idDeclAF_Body;

	bodies.Alloc() = body;

	body->SetDefault( this );

	if( !src.ExpectTokenType( TT_STRING, 0, &token ) ||
			!src.ExpectTokenString( "{" ) )
	{
		return false;
	}

	body->name = token;
	if( !body->name.Icmp( "origin" ) || !body->name.Icmp( "world" ) )
	{
		src.Error( "a body may not be named \"origin\" or \"world\"" );
		return false;
	}

	while( src.ReadToken( &token ) )
	{

		if( !token.Icmp( "model" ) )
		{
			if( !src.ExpectTokenType( TT_NAME, 0, &token ) )
			{
				return false;
			}
			if( !token.Icmp( "box" ) )
			{
				body->modelType = TRM_BOX;
				if( !src.ExpectTokenString( "(" ) ||
						!body->v1.Parse( src ) ||
						!src.ExpectTokenString( "," ) ||
						!body->v2.Parse( src ) ||
						!src.ExpectTokenString( ")" ) )
				{
					return false;
				}
			}
			else if( !token.Icmp( "octahedron" ) )
			{
				body->modelType = TRM_OCTAHEDRON;
				if( !src.ExpectTokenString( "(" ) ||
						!body->v1.Parse( src ) ||
						!src.ExpectTokenString( "," ) ||
						!body->v2.Parse( src ) ||
						!src.ExpectTokenString( ")" ) )
				{
					return false;
				}
			}
			else if( !token.Icmp( "dodecahedron" ) )
			{
				body->modelType = TRM_DODECAHEDRON;
				if( !src.ExpectTokenString( "(" ) ||
						!body->v1.Parse( src ) ||
						!src.ExpectTokenString( "," ) ||
						!body->v2.Parse( src ) ||
						!src.ExpectTokenString( ")" ) )
				{
					return false;
				}
			}
			else if( !token.Icmp( "cylinder" ) )
			{
				body->modelType = TRM_CYLINDER;
				if( !src.ExpectTokenString( "(" ) ||
						!body->v1.Parse( src ) ||
						!src.ExpectTokenString( "," ) ||
						!body->v2.Parse( src ) ||
						!src.ExpectTokenString( "," ) )
				{
					return false;
				}
				body->numSides = src.ParseInt();
				if( !src.ExpectTokenString( ")" ) )
				{
					return false;
				}
			}
			else if( !token.Icmp( "cone" ) )
			{
				body->modelType = TRM_CONE;
				if( !src.ExpectTokenString( "(" ) ||
						!body->v1.Parse( src ) ||
						!src.ExpectTokenString( "," ) ||
						!body->v2.Parse( src ) ||
						!src.ExpectTokenString( "," ) )
				{
					return false;
				}
				body->numSides = src.ParseInt();
				if( !src.ExpectTokenString( ")" ) )
				{
					return false;
				}
			}
			else if( !token.Icmp( "bone" ) )
			{
				body->modelType = TRM_BONE;
				if( !src.ExpectTokenString( "(" ) ||
						!body->v1.Parse( src ) ||
						!src.ExpectTokenString( "," ) ||
						!body->v2.Parse( src ) ||
						!src.ExpectTokenString( "," ) )
				{
					return false;
				}
				body->width = src.ParseFloat();
				if( !src.ExpectTokenString( ")" ) )
				{
					return false;
				}
			}
			else if( !token.Icmp( "custom" ) )
			{
				src.Error( "custom models not yet implemented" );
				return false;
			}
			else
			{
				src.Error( "unknown model type %s", token.c_str() );
				return false;
			}
		}
		else if( !token.Icmp( "origin" ) )
		{
			if( !body->origin.Parse( src ) )
			{
				return false;
			}
		}
		else if( !token.Icmp( "angles" ) )
		{
			if( !angles.Parse( src ) )
			{
				return false;
			}
			body->angles = idAngles( angles.ToVec3().x, angles.ToVec3().y, angles.ToVec3().z );
		}
		else if( !token.Icmp( "joint" ) )
		{
			if( !src.ExpectTokenType( TT_STRING, 0, &token ) )
			{
				return false;
			}
			body->jointName = token;
			hasJoint = true;
		}
		else if( !token.Icmp( "mod" ) )
		{
			if( !src.ExpectAnyToken( &token ) )
			{
				return false;
			}
			body->jointMod = JointModFromString( token.c_str() );
		}
		else if( !token.Icmp( "density" ) )
		{
			body->density = src.ParseFloat();
		}
		else if( !token.Icmp( "inertiaScale" ) )
		{
			src.Parse1DMatrix( 9, body->inertiaScale[0].ToFloatPtr() );
		}
		else if( !token.Icmp( "friction" ) )
		{
			body->linearFriction = src.ParseFloat();
			src.ExpectTokenString( "," );
			body->angularFriction = src.ParseFloat();
			src.ExpectTokenString( "," );
			body->contactFriction = src.ParseFloat();
		}
		else if( !token.Icmp( "contents" ) )
		{
			ParseContents( src, body->contents );
		}
		else if( !token.Icmp( "clipMask" ) )
		{
			ParseContents( src, body->clipMask );
			body->clipMask &= ~CONTENTS_CORPSE;		// never allow collisions against corpses
		}
		else if( !token.Icmp( "selfCollision" ) )
		{
			body->selfCollision = src.ParseBool();
		}
		else if( !token.Icmp( "containedjoints" ) )
		{
			if( !src.ExpectTokenType( TT_STRING, 0, &token ) )
			{
				return false;
			}
			body->containedJoints = token;
		}
		else if( !token.Icmp( "frictionDirection" ) )
		{
			if( !body->frictionDirection.Parse( src ) )
			{
				return false;
			}
		}
		else if( !token.Icmp( "contactMotorDirection" ) )
		{
			if( !body->contactMotorDirection.Parse( src ) )
			{
				return false;
			}
		}
		else if( token == "}" )
		{
			break;
		}
		else
		{
			src.Error( "unknown token %s in body", token.c_str() );
			return false;
		}
	}

	if( body->modelType == TRM_INVALID )
	{
		src.Error( "no model set for body" );
		return false;
	}

	if( !hasJoint )
	{
		src.Error( "no joint set for body" );
		return false;
	}

	body->clipMask |= CONTENTS_MOVEABLECLIP;

	return true;
}

/*
================
idDeclAF::ParseFixed
================
*/
bool idDeclAF::ParseFixed( idLexer& src )
{
	idToken token;
	idDeclAF_Constraint* constraint = new( TAG_DECL ) idDeclAF_Constraint;

	constraint->SetDefault( this );
	constraints.Alloc() = constraint;

	if( !src.ExpectTokenType( TT_STRING, 0, &token ) ||
			!src.ExpectTokenString( "{" ) )
	{
		return false;
	}

	constraint->type = DECLAF_CONSTRAINT_FIXED;
	constraint->name = token;

	while( src.ReadToken( &token ) )
	{

		if( !token.Icmp( "body1" ) )
		{
			src.ExpectTokenType( TT_STRING, 0, &token );
			constraint->body1 = token;
		}
		else if( !token.Icmp( "body2" ) )
		{
			src.ExpectTokenType( TT_STRING, 0, &token );
			constraint->body2 = token;
		}
		else if( token == "}" )
		{
			break;
		}
		else
		{
			src.Error( "unknown token %s in ball and socket joint", token.c_str() );
			return false;
		}
	}

	return true;
}

/*
================
idDeclAF::ParseBallAndSocketJoint
================
*/
bool idDeclAF::ParseBallAndSocketJoint( idLexer& src )
{
	idToken token;
	idDeclAF_Constraint* constraint = new( TAG_DECL ) idDeclAF_Constraint;

	constraint->SetDefault( this );
	constraints.Alloc() = constraint;

	if( !src.ExpectTokenType( TT_STRING, 0, &token ) ||
			!src.ExpectTokenString( "{" ) )
	{
		return false;
	}

	constraint->type = DECLAF_CONSTRAINT_BALLANDSOCKETJOINT;
	constraint->limit = idDeclAF_Constraint::LIMIT_NONE;
	constraint->name = token;
	constraint->friction = 0.5f;
	constraint->anchor.ToVec3().Zero();
	constraint->shaft[0].ToVec3().Zero();

	while( src.ReadToken( &token ) )
	{

		if( !token.Icmp( "body1" ) )
		{
			src.ExpectTokenType( TT_STRING, 0, &token );
			constraint->body1 = token;
		}
		else if( !token.Icmp( "body2" ) )
		{
			src.ExpectTokenType( TT_STRING, 0, &token );
			constraint->body2 = token;
		}
		else if( !token.Icmp( "anchor" ) )
		{
			if( !constraint->anchor.Parse( src ) )
			{
				return false;
			}
		}
		else if( !token.Icmp( "conelimit" ) )
		{
			if( !constraint->limitAxis.Parse( src ) ||
					!src.ExpectTokenString( "," ) )
			{
				return false;
			}
			constraint->limitAngles[0] = src.ParseFloat();
			if( !src.ExpectTokenString( "," ) ||
					!constraint->shaft[0].Parse( src ) )
			{
				return false;
			}
			constraint->limit = idDeclAF_Constraint::LIMIT_CONE;
		}
		else if( !token.Icmp( "pyramidlimit" ) )
		{
			if( !constraint->limitAxis.Parse( src ) ||
					!src.ExpectTokenString( "," ) )
			{
				return false;
			}
			constraint->limitAngles[0] = src.ParseFloat();
			if( !src.ExpectTokenString( "," ) )
			{
				return false;
			}
			constraint->limitAngles[1] = src.ParseFloat();
			if( !src.ExpectTokenString( "," ) )
			{
				return false;
			}
			constraint->limitAngles[2] = src.ParseFloat();
			if( !src.ExpectTokenString( "," ) ||
					!constraint->shaft[0].Parse( src ) )
			{
				return false;
			}
			constraint->limit = idDeclAF_Constraint::LIMIT_PYRAMID;
		}
		else if( !token.Icmp( "friction" ) )
		{
			constraint->friction = src.ParseFloat();
		}
		else if( token == "}" )
		{
			break;
		}
		else
		{
			src.Error( "unknown token %s in ball and socket joint", token.c_str() );
			return false;
		}
	}

	return true;
}

/*
================
idDeclAF::ParseUniversalJoint
================
*/
bool idDeclAF::ParseUniversalJoint( idLexer& src )
{
	idToken token;
	idDeclAF_Constraint* constraint = new( TAG_DECL ) idDeclAF_Constraint;

	constraint->SetDefault( this );
	constraints.Alloc() = constraint;

	if( !src.ExpectTokenType( TT_STRING, 0, &token ) ||
			!src.ExpectTokenString( "{" ) )
	{
		return false;
	}

	constraint->type = DECLAF_CONSTRAINT_UNIVERSALJOINT;
	constraint->limit = idDeclAF_Constraint::LIMIT_NONE;
	constraint->name = token;
	constraint->friction = 0.5f;
	constraint->anchor.ToVec3().Zero();
	constraint->shaft[0].ToVec3().Zero();
	constraint->shaft[1].ToVec3().Zero();

	while( src.ReadToken( &token ) )
	{

		if( !token.Icmp( "body1" ) )
		{
			src.ExpectTokenType( TT_STRING, 0, &token );
			constraint->body1 = token;
		}
		else if( !token.Icmp( "body2" ) )
		{
			src.ExpectTokenType( TT_STRING, 0, &token );
			constraint->body2 = token;
		}
		else if( !token.Icmp( "anchor" ) )
		{
			if( !constraint->anchor.Parse( src ) )
			{
				return false;
			}
		}
		else if( !token.Icmp( "shafts" ) )
		{
			if( !constraint->shaft[0].Parse( src ) ||
					!src.ExpectTokenString( "," ) ||
					!constraint->shaft[1].Parse( src ) )
			{
				return false;
			}
		}
		else if( !token.Icmp( "conelimit" ) )
		{
			if( !constraint->limitAxis.Parse( src ) ||
					!src.ExpectTokenString( "," ) )
			{
				return false;
			}
			constraint->limitAngles[0] = src.ParseFloat();
			constraint->limit = idDeclAF_Constraint::LIMIT_CONE;
		}
		else if( !token.Icmp( "pyramidlimit" ) )
		{
			if( !constraint->limitAxis.Parse( src ) ||
					!src.ExpectTokenString( "," ) )
			{
				return false;
			}
			constraint->limitAngles[0] = src.ParseFloat();
			if( !src.ExpectTokenString( "," ) )
			{
				return false;
			}
			constraint->limitAngles[1] = src.ParseFloat();
			if( !src.ExpectTokenString( "," ) )
			{
				return false;
			}
			constraint->limitAngles[2] = src.ParseFloat();
			constraint->limit = idDeclAF_Constraint::LIMIT_PYRAMID;
		}
		else if( !token.Icmp( "friction" ) )
		{
			constraint->friction = src.ParseFloat();
		}
		else if( token == "}" )
		{
			break;
		}
		else
		{
			src.Error( "unknown token %s in universal joint", token.c_str() );
			return false;
		}
	}

	return true;
}

/*
================
idDeclAF::ParseHinge
================
*/
bool idDeclAF::ParseHinge( idLexer& src )
{
	idToken token;
	idDeclAF_Constraint* constraint = new( TAG_DECL ) idDeclAF_Constraint;

	constraint->SetDefault( this );
	constraints.Alloc() = constraint;

	if( !src.ExpectTokenType( TT_STRING, 0, &token ) ||
			!src.ExpectTokenString( "{" ) )
	{
		return false;
	}

	constraint->type = DECLAF_CONSTRAINT_HINGE;
	constraint->limit = idDeclAF_Constraint::LIMIT_NONE;
	constraint->name = token;
	constraint->friction = 0.5f;
	constraint->anchor.ToVec3().Zero();
	constraint->axis.ToVec3().Zero();

	while( src.ReadToken( &token ) )
	{

		if( !token.Icmp( "body1" ) )
		{
			src.ExpectTokenType( TT_STRING, 0, &token );
			constraint->body1 = token;
		}
		else if( !token.Icmp( "body2" ) )
		{
			src.ExpectTokenType( TT_STRING, 0, &token );
			constraint->body2 = token;
		}
		else if( !token.Icmp( "anchor" ) )
		{
			if( !constraint->anchor.Parse( src ) )
			{
				return false;
			}
		}
		else if( !token.Icmp( "axis" ) )
		{
			if( !constraint->axis.Parse( src ) )
			{
				return false;
			}
		}
		else if( !token.Icmp( "limit" ) )
		{
			constraint->limitAngles[0] = src.ParseFloat();
			if( !src.ExpectTokenString( "," ) )
			{
				return false;
			}
			constraint->limitAngles[1] = src.ParseFloat();
			if( !src.ExpectTokenString( "," ) )
			{
				return false;
			}
			constraint->limitAngles[2] = src.ParseFloat();
			constraint->limit = idDeclAF_Constraint::LIMIT_CONE;
		}
		else if( !token.Icmp( "friction" ) )
		{
			constraint->friction = src.ParseFloat();
		}
		else if( token == "}" )
		{
			break;
		}
		else
		{
			src.Error( "unknown token %s in hinge", token.c_str() );
			return false;
		}
	}

	return true;
}

/*
================
idDeclAF::ParseSlider
================
*/
bool idDeclAF::ParseSlider( idLexer& src )
{
	idToken token;
	idDeclAF_Constraint* constraint = new( TAG_DECL ) idDeclAF_Constraint;

	constraint->SetDefault( this );
	constraints.Alloc() = constraint;

	if( !src.ExpectTokenType( TT_STRING, 0, &token ) ||
			!src.ExpectTokenString( "{" ) )
	{
		return false;
	}

	constraint->type = DECLAF_CONSTRAINT_SLIDER;
	constraint->limit = idDeclAF_Constraint::LIMIT_NONE;
	constraint->name = token;
	constraint->friction = 0.5f;

	while( src.ReadToken( &token ) )
	{

		if( !token.Icmp( "body1" ) )
		{
			src.ExpectTokenType( TT_STRING, 0, &token );
			constraint->body1 = token;
		}
		else if( !token.Icmp( "body2" ) )
		{
			src.ExpectTokenType( TT_STRING, 0, &token );
			constraint->body2 = token;
		}
		else if( !token.Icmp( "axis" ) )
		{
			if( !constraint->axis.Parse( src ) )
			{
				return false;
			}
		}
		else if( !token.Icmp( "friction" ) )
		{
			constraint->friction = src.ParseFloat();
		}
		else if( token == "}" )
		{
			break;
		}
		else
		{
			src.Error( "unknown token %s in slider", token.c_str() );
			return false;
		}
	}

	return true;
}

/*
================
idDeclAF::ParseSpring
================
*/
bool idDeclAF::ParseSpring( idLexer& src )
{
	idToken token;
	idDeclAF_Constraint* constraint = new( TAG_DECL ) idDeclAF_Constraint;

	constraint->SetDefault( this );
	constraints.Alloc() = constraint;

	if( !src.ExpectTokenType( TT_STRING, 0, &token ) ||
			!src.ExpectTokenString( "{" ) )
	{
		return false;
	}

	constraint->type = DECLAF_CONSTRAINT_SPRING;
	constraint->limit = idDeclAF_Constraint::LIMIT_NONE;
	constraint->name = token;
	constraint->friction = 0.5f;

	while( src.ReadToken( &token ) )
	{

		if( !token.Icmp( "body1" ) )
		{
			src.ExpectTokenType( TT_STRING, 0, &token );
			constraint->body1 = token;
		}
		else if( !token.Icmp( "body2" ) )
		{
			src.ExpectTokenType( TT_STRING, 0, &token );
			constraint->body2 = token;
		}
		else if( !token.Icmp( "anchor1" ) )
		{
			if( !constraint->anchor.Parse( src ) )
			{
				return false;
			}
		}
		else if( !token.Icmp( "anchor2" ) )
		{
			if( !constraint->anchor2.Parse( src ) )
			{
				return false;
			}
		}
		else if( !token.Icmp( "friction" ) )
		{
			constraint->friction = src.ParseFloat();
		}
		else if( !token.Icmp( "stretch" ) )
		{
			constraint->stretch = src.ParseFloat();
		}
		else if( !token.Icmp( "compress" ) )
		{
			constraint->compress = src.ParseFloat();
		}
		else if( !token.Icmp( "damping" ) )
		{
			constraint->damping = src.ParseFloat();
		}
		else if( !token.Icmp( "restLength" ) )
		{
			constraint->restLength = src.ParseFloat();
		}
		else if( !token.Icmp( "minLength" ) )
		{
			constraint->minLength = src.ParseFloat();
		}
		else if( !token.Icmp( "maxLength" ) )
		{
			constraint->maxLength = src.ParseFloat();
		}
		else if( token == "}" )
		{
			break;
		}
		else
		{
			src.Error( "unknown token %s in spring", token.c_str() );
			return false;
		}
	}

	return true;
}

/*
================
idDeclAF::ParseSettings
================
*/
bool idDeclAF::ParseSettings( idLexer& src )
{
	idToken token;

	if( !src.ExpectTokenString( "{" ) )
	{
		return false;
	}

	while( src.ReadToken( &token ) )
	{

		if( !token.Icmp( "mesh" ) )
		{
			if( !src.ExpectTokenType( TT_STRING, 0, &token ) )
			{
				return false;
			}
		}
		else if( !token.Icmp( "anim" ) )
		{
			if( !src.ExpectTokenType( TT_STRING, 0, &token ) )
			{
				return false;
			}
		}
		else if( !token.Icmp( "model" ) )
		{
			if( !src.ExpectTokenType( TT_STRING, 0, &token ) )
			{
				return false;
			}
			model = token;
		}
		else if( !token.Icmp( "skin" ) )
		{
			if( !src.ExpectTokenType( TT_STRING, 0, &token ) )
			{
				return false;
			}
			skin = token;
		}
		else if( !token.Icmp( "friction" ) )
		{

			defaultLinearFriction = src.ParseFloat();
			if( !src.ExpectTokenString( "," ) )
			{
				return false;
			}
			defaultAngularFriction = src.ParseFloat();
			if( !src.ExpectTokenString( "," ) )
			{
				return false;
			}
			defaultContactFriction = src.ParseFloat();
			if( src.CheckTokenString( "," ) )
			{
				defaultConstraintFriction = src.ParseFloat();
			}
		}
		else if( !token.Icmp( "totalMass" ) )
		{
			totalMass = src.ParseFloat();
		}
		else if( !token.Icmp( "suspendSpeed" ) )
		{

			suspendVelocity[0] = src.ParseFloat();
			if( !src.ExpectTokenString( "," ) )
			{
				return false;
			}
			suspendVelocity[1] = src.ParseFloat();
			if( !src.ExpectTokenString( "," ) )
			{
				return false;
			}
			suspendAcceleration[0] = src.ParseFloat();
			if( !src.ExpectTokenString( "," ) )
			{
				return false;
			}
			suspendAcceleration[1] = src.ParseFloat();
		}
		else if( !token.Icmp( "noMoveTime" ) )
		{
			noMoveTime = src.ParseFloat();
		}
		else if( !token.Icmp( "noMoveTranslation" ) )
		{
			noMoveTranslation = src.ParseFloat();
		}
		else if( !token.Icmp( "noMoveRotation" ) )
		{
			noMoveRotation = src.ParseFloat();
		}
		else if( !token.Icmp( "minMoveTime" ) )
		{
			minMoveTime = src.ParseFloat();
		}
		else if( !token.Icmp( "maxMoveTime" ) )
		{
			maxMoveTime = src.ParseFloat();
		}
		else if( !token.Icmp( "contents" ) )
		{
			ParseContents( src, contents );
		}
		else if( !token.Icmp( "clipMask" ) )
		{
			ParseContents( src, clipMask );
			clipMask &= ~CONTENTS_CORPSE;	// never allow collisions against corpses
		}
		else if( !token.Icmp( "selfCollision" ) )
		{
			selfCollision = src.ParseBool();
		}
		else if( token == "}" )
		{
			break;
		}
		else
		{
			src.Error( "unknown token %s in settings", token.c_str() );
			return false;
		}
	}

	return true;
}

/*
================
idDeclAF::Parse
================
*/
bool idDeclAF::Parse( const char* text, const int textLength, bool allowBinaryVersion )
{
	int i, j;
	idLexer src;
	idToken token;

	src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	src.SetFlags( DECL_LEXER_FLAGS );
	src.SkipUntilString( "{" );

	while( src.ReadToken( &token ) )
	{

		if( !token.Icmp( "settings" ) )
		{
			if( !ParseSettings( src ) )
			{
				return false;
			}
		}
		else if( !token.Icmp( "body" ) )
		{
			if( !ParseBody( src ) )
			{
				return false;
			}
		}
		else if( !token.Icmp( "fixed" ) )
		{
			if( !ParseFixed( src ) )
			{
				return false;
			}
		}
		else if( !token.Icmp( "ballAndSocketJoint" ) )
		{
			if( !ParseBallAndSocketJoint( src ) )
			{
				return false;
			}
		}
		else if( !token.Icmp( "universalJoint" ) )
		{
			if( !ParseUniversalJoint( src ) )
			{
				return false;
			}
		}
		else if( !token.Icmp( "hinge" ) )
		{
			if( !ParseHinge( src ) )
			{
				return false;
			}
		}
		else if( !token.Icmp( "slider" ) )
		{
			if( !ParseSlider( src ) )
			{
				return false;
			}
		}
		else if( !token.Icmp( "spring" ) )
		{
			if( !ParseSpring( src ) )
			{
				return false;
			}
		}
		else if( token == "}" )
		{
			break;
		}
		else
		{
			src.Error( "unknown keyword %s", token.c_str() );
			return false;
		}
	}

	for( i = 0; i < bodies.Num(); i++ )
	{
		// check for multiple bodies with the same name
		for( j = i + 1; j < bodies.Num(); j++ )
		{
			if( bodies[i]->name == bodies[j]->name )
			{
				src.Error( "two bodies with the same name \"%s\"", bodies[i]->name.c_str() );
			}
		}
	}

	for( i = 0; i < constraints.Num(); i++ )
	{
		// check for multiple constraints with the same name
		for( j = i + 1; j < constraints.Num(); j++ )
		{
			if( constraints[i]->name == constraints[j]->name )
			{
				src.Error( "two constraints with the same name \"%s\"", constraints[i]->name.c_str() );
			}
		}
		// check if there are two valid bodies set
		if( constraints[i]->body1 == "" )
		{
			src.Error( "no valid body1 specified for constraint '%s'", constraints[i]->name.c_str() );
		}
		if( constraints[i]->body2 == "" )
		{
			src.Error( "no valid body2 specified for constraint '%s'", constraints[i]->name.c_str() );
		}
	}

	// make sure the body which modifies the origin comes first
	for( i = 0; i < bodies.Num(); i++ )
	{
		if( bodies[i]->jointName == "origin" )
		{
			if( i != 0 )
			{
				idDeclAF_Body* b = bodies[0];
				bodies[0] = bodies[i];
				bodies[i] = b;
			}
			break;
		}
	}

	return true;
}

/*
================
idDeclAF::DefaultDefinition
================
*/
const char* idDeclAF::DefaultDefinition() const
{
	return
		"{\n"
		"\t"	"settings {\n"
		"\t\t"		"model \"\"\n"
		"\t\t"		"skin \"\"\n"
		"\t\t"		"friction 0.01, 0.01, 0.8, 0.5\n"
		"\t\t"		"suspendSpeed 20, 30, 40, 60\n"
		"\t\t"		"noMoveTime 1\n"
		"\t\t"		"noMoveTranslation 10\n"
		"\t\t"		"noMoveRotation 10\n"
		"\t\t"		"minMoveTime -1\n"
		"\t\t"		"maxMoveTime -1\n"
		"\t\t"		"totalMass -1\n"
		"\t\t"		"contents corpse\n"
		"\t\t"		"clipMask solid, corpse\n"
		"\t\t"		"selfCollision 1\n"
		"\t"	"}\n"
		"\t"	"body \"body\" {\n"
		"\t\t"		"joint \"origin\"\n"
		"\t\t"		"mod orientation\n"
		"\t\t"		"model box( ( -10, -10, -10 ), ( 10, 10, 10 ) )\n"
		"\t\t"		"origin ( 0, 0, 0 )\n"
		"\t\t"		"density 0.2\n"
		"\t\t"		"friction 0.01, 0.01, 0.8\n"
		"\t\t"		"contents corpse\n"
		"\t\t"		"clipMask solid, corpse\n"
		"\t\t"		"selfCollision 1\n"
		"\t\t"		"containedJoints \"*origin\"\n"
		"\t"	"}\n"
		"}\n";
}

/*
================
idDeclAF::FreeData
================
*/
void idDeclAF::FreeData()
{
	modified = false;
	defaultLinearFriction = 0.01f;
	defaultAngularFriction = 0.01f;
	defaultContactFriction = 0.8f;
	defaultConstraintFriction = 0.5f;
	totalMass = -1;
	suspendVelocity.Set( 20.0f, 30.0f );
	suspendAcceleration.Set( 40.0f, 60.0f );
	noMoveTime = 1.0f;
	noMoveTranslation = 10.0f;
	noMoveRotation = 10.0f;
	minMoveTime = -1.0f;
	maxMoveTime = -1.0f;
	selfCollision = true;
	contents = CONTENTS_CORPSE;
	clipMask = CONTENTS_SOLID;
	bodies.DeleteContents( true );
	constraints.DeleteContents( true );
}

/*
================
idDeclAF::Finish
================
*/
void idDeclAF::Finish( const getJointTransform_t GetJointTransform, const idJointMat* frame, void* model ) const
{
	int i;

	const char* name = GetName();
	for( i = 0; i < bodies.Num(); i++ )
	{
		idDeclAF_Body* body = bodies[i];
		body->v1.Finish( name, GetJointTransform, frame, model );
		body->v2.Finish( name, GetJointTransform, frame, model );
		body->origin.Finish( name, GetJointTransform, frame, model );
		body->frictionDirection.Finish( name, GetJointTransform, frame, model );
		body->contactMotorDirection.Finish( name, GetJointTransform, frame, model );
	}
	for( i = 0; i < constraints.Num(); i++ )
	{
		idDeclAF_Constraint* constraint = constraints[i];
		constraint->anchor.Finish( name, GetJointTransform, frame, model );
		constraint->anchor2.Finish( name, GetJointTransform, frame, model );
		constraint->shaft[0].Finish( name, GetJointTransform, frame, model );
		constraint->shaft[1].Finish( name, GetJointTransform, frame, model );
		constraint->axis.Finish( name, GetJointTransform, frame, model );
		constraint->limitAxis.Finish( name, GetJointTransform, frame, model );
	}
}

/*
================
idDeclAF::NewBody
================
*/
void idDeclAF::NewBody( const char* name )
{
	idDeclAF_Body* body;

	body = new( TAG_DECL ) idDeclAF_Body();
	body->SetDefault( this );
	body->name = name;
	bodies.Append( body );
}

/*
================
idDeclAF::RenameBody

  rename the body with the given name and rename
  all constraint body references
================
*/
void idDeclAF::RenameBody( const char* oldName, const char* newName )
{
	int i;

	for( i = 0; i < bodies.Num(); i++ )
	{
		if( bodies[i]->name.Icmp( oldName ) == 0 )
		{
			bodies[i]->name = newName;
			break;
		}
	}
	for( i = 0; i < constraints.Num(); i++ )
	{
		if( constraints[i]->body1.Icmp( oldName ) == 0 )
		{
			constraints[i]->body1 = newName;
		}
		else if( constraints[i]->body2.Icmp( oldName ) == 0 )
		{
			constraints[i]->body2 = newName;
		}
	}
}

/*
================
idDeclAF::DeleteBody

  delete the body with the given name and delete
  all constraints that reference the body
================
*/
void idDeclAF::DeleteBody( const char* name )
{
	int i;

	for( i = 0; i < bodies.Num(); i++ )
	{
		if( bodies[i]->name.Icmp( name ) == 0 )
		{
			delete bodies[i];
			bodies.RemoveIndex( i );
			break;
		}
	}
	for( i = 0; i < constraints.Num(); i++ )
	{
		if( constraints[i]->body1.Icmp( name ) == 0 ||
				constraints[i]->body2.Icmp( name ) == 0 )
		{
			delete constraints[i];
			constraints.RemoveIndex( i );
			i--;
		}
	}
}

/*
================
idDeclAF::NewConstraint
================
*/
void idDeclAF::NewConstraint( const char* name )
{
	idDeclAF_Constraint* constraint;

	constraint = new( TAG_DECL ) idDeclAF_Constraint;
	constraint->SetDefault( this );
	constraint->name = name;
	constraints.Append( constraint );
}

/*
================
idDeclAF::RenameConstraint
================
*/
void idDeclAF::RenameConstraint( const char* oldName, const char* newName )
{
	int i;

	for( i = 0; i < constraints.Num(); i++ )
	{
		if( constraints[i]->name.Icmp( oldName ) == 0 )
		{
			constraints[i]->name = newName;
			return;
		}
	}
}

/*
================
idDeclAF::DeleteConstraint
================
*/
void idDeclAF::DeleteConstraint( const char* name )
{
	int i;

	for( i = 0; i < constraints.Num(); i++ )
	{
		if( constraints[i]->name.Icmp( name ) == 0 )
		{
			delete constraints[i];
			constraints.RemoveIndex( i );
			return;
		}
	}
}

/*
================
idDeclAF::idDeclAF
================
*/
idDeclAF::idDeclAF()
{
	FreeData();
}

/*
================
idDeclAF::~idDeclAF
================
*/
idDeclAF::~idDeclAF()
{
	bodies.DeleteContents( true );
	constraints.DeleteContents( true );
}
