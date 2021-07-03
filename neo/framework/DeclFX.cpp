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
=================
idDeclFX::Size
=================
*/
size_t idDeclFX::Size() const
{
	return sizeof( idDeclFX );
}

/*
===============
idDeclFX::Print
===============
*/
void idDeclFX::Print() const
{
	const idDeclFX* list = this;

	common->Printf( "%d events\n", list->events.Num() );
	for( int i = 0; i < list->events.Num(); i++ )
	{
		switch( list->events[i].type )
		{
			case FX_LIGHT:
				common->Printf( "FX_LIGHT %s\n", list->events[i].data.c_str() );
				break;
			case FX_PARTICLE:
				common->Printf( "FX_PARTICLE %s\n", list->events[i].data.c_str() );
				break;
			case FX_MODEL:
				common->Printf( "FX_MODEL %s\n", list->events[i].data.c_str() );
				break;
			case FX_SOUND:
				common->Printf( "FX_SOUND %s\n", list->events[i].data.c_str() );
				break;
			case FX_DECAL:
				common->Printf( "FX_DECAL %s\n", list->events[i].data.c_str() );
				break;
			case FX_SHAKE:
				common->Printf( "FX_SHAKE %s\n", list->events[i].data.c_str() );
				break;
			case FX_ATTACHLIGHT:
				common->Printf( "FX_ATTACHLIGHT %s\n", list->events[i].data.c_str() );
				break;
			case FX_ATTACHENTITY:
				common->Printf( "FX_ATTACHENTITY %s\n", list->events[i].data.c_str() );
				break;
			case FX_LAUNCH:
				common->Printf( "FX_LAUNCH %s\n", list->events[i].data.c_str() );
				break;
			case FX_SHOCKWAVE:
				common->Printf( "FX_SHOCKWAVE %s\n", list->events[i].data.c_str() );
				break;
		}
	}
}

/*
===============
idDeclFX::List
===============
*/
void idDeclFX::List() const
{
	common->Printf( "%s, %d stages\n", GetName(), events.Num() );
}

/*
================
idDeclFX::ParseSingleFXAction
================
*/
void idDeclFX::ParseSingleFXAction( idLexer& src, idFXSingleAction& FXAction )
{
	idToken token;

	FXAction.type = -1;
	FXAction.sibling = -1;

	FXAction.data = "<none>";
	FXAction.name = "<none>";
	FXAction.fire = "<none>";

	FXAction.delay = 0.0f;
	FXAction.duration = 0.0f;
	FXAction.restart = 0.0f;
	FXAction.size = 0.0f;
	FXAction.fadeInTime = 0.0f;
	FXAction.fadeOutTime = 0.0f;
	FXAction.shakeTime = 0.0f;
	FXAction.shakeAmplitude = 0.0f;
	FXAction.shakeDistance = 0.0f;
	FXAction.shakeFalloff = false;
	FXAction.shakeImpulse = 0.0f;
	FXAction.shakeIgnoreMaster = false;
	FXAction.lightRadius = 0.0f;
	FXAction.rotate = 0.0f;
	FXAction.random1 = 0.0f;
	FXAction.random2 = 0.0f;

	FXAction.lightColor = vec3_origin;
	FXAction.offset = vec3_origin;
	FXAction.axis = mat3_identity;

	FXAction.bindParticles = false;
	FXAction.explicitAxis = false;
	FXAction.noshadows = false;
	FXAction.particleTrackVelocity = false;
	FXAction.trackOrigin = false;
	FXAction.soundStarted = false;

	while( 1 )
	{
		if( !src.ReadToken( &token ) )
		{
			break;
		}

		if( !token.Icmp( "}" ) )
		{
			break;
		}

		if( !token.Icmp( "shake" ) )
		{
			FXAction.type = FX_SHAKE;
			FXAction.shakeTime = src.ParseFloat();
			src.ExpectTokenString( "," );
			FXAction.shakeAmplitude = src.ParseFloat();
			src.ExpectTokenString( "," );
			FXAction.shakeDistance = src.ParseFloat();
			src.ExpectTokenString( "," );
			FXAction.shakeFalloff = src.ParseBool();
			src.ExpectTokenString( "," );
			FXAction.shakeImpulse = src.ParseFloat();
			continue;
		}

		if( !token.Icmp( "noshadows" ) )
		{
			FXAction.noshadows = true;
			continue;
		}

		if( !token.Icmp( "name" ) )
		{
			src.ReadToken( &token );
			FXAction.name = token;
			continue;
		}

		if( !token.Icmp( "fire" ) )
		{
			src.ReadToken( &token );
			FXAction.fire = token;
			continue;
		}

		if( !token.Icmp( "random" ) )
		{
			FXAction.random1 = src.ParseFloat();
			src.ExpectTokenString( "," );
			FXAction.random2 = src.ParseFloat();
			FXAction.delay = 0.0f;		// check random
			continue;
		}

		if( !token.Icmp( "delay" ) )
		{
			FXAction.delay = src.ParseFloat();
			continue;
		}

		if( !token.Icmp( "rotate" ) )
		{
			FXAction.rotate = src.ParseFloat();
			continue;
		}

		if( !token.Icmp( "duration" ) )
		{
			FXAction.duration = src.ParseFloat();
			continue;
		}

		if( !token.Icmp( "trackorigin" ) )
		{
			FXAction.trackOrigin = src.ParseBool();
			continue;
		}

		if( !token.Icmp( "restart" ) )
		{
			FXAction.restart = src.ParseFloat();
			continue;
		}

		if( !token.Icmp( "fadeIn" ) )
		{
			FXAction.fadeInTime = src.ParseFloat();
			continue;
		}

		if( !token.Icmp( "fadeOut" ) )
		{
			FXAction.fadeOutTime = src.ParseFloat();
			continue;
		}

		if( !token.Icmp( "size" ) )
		{
			FXAction.size = src.ParseFloat();
			continue;
		}

		if( !token.Icmp( "offset" ) )
		{
			FXAction.offset.x = src.ParseFloat();
			src.ExpectTokenString( "," );
			FXAction.offset.y = src.ParseFloat();
			src.ExpectTokenString( "," );
			FXAction.offset.z = src.ParseFloat();
			continue;
		}

		if( !token.Icmp( "axis" ) )
		{
			idVec3 v;
			v.x = src.ParseFloat();
			src.ExpectTokenString( "," );
			v.y = src.ParseFloat();
			src.ExpectTokenString( "," );
			v.z = src.ParseFloat();
			v.Normalize();
			FXAction.axis = v.ToMat3();
			FXAction.explicitAxis = true;
			continue;
		}

		if( !token.Icmp( "angle" ) )
		{
			idAngles a;
			a[0] = src.ParseFloat();
			src.ExpectTokenString( "," );
			a[1] = src.ParseFloat();
			src.ExpectTokenString( "," );
			a[2] = src.ParseFloat();
			FXAction.axis = a.ToMat3();
			FXAction.explicitAxis = true;
			continue;
		}

		if( !token.Icmp( "uselight" ) )
		{
			src.ReadToken( &token );
			FXAction.data = token;
			for( int i = 0; i < events.Num(); i++ )
			{
				if( events[i].name.Icmp( FXAction.data ) == 0 )
				{
					FXAction.sibling = i;
					FXAction.lightColor = events[i].lightColor;
					FXAction.lightRadius = events[i].lightRadius;
				}
			}
			FXAction.type = FX_LIGHT;

			// precache the light material
			declManager->FindMaterial( FXAction.data );
			continue;
		}

		if( !token.Icmp( "attachlight" ) )
		{
			src.ReadToken( &token );
			FXAction.data = token;
			FXAction.type = FX_ATTACHLIGHT;

			// precache it
			declManager->FindMaterial( FXAction.data );
			continue;
		}

		if( !token.Icmp( "attachentity" ) )
		{
			src.ReadToken( &token );
			FXAction.data = token;
			FXAction.type = FX_ATTACHENTITY;

			// precache the model
			renderModelManager->FindModel( FXAction.data );
			continue;
		}

		if( !token.Icmp( "launch" ) )
		{
			src.ReadToken( &token );
			FXAction.data = token;
			FXAction.type = FX_LAUNCH;

			// precache the entity def
			declManager->FindType( DECL_ENTITYDEF, FXAction.data );
			continue;
		}

		if( !token.Icmp( "useModel" ) )
		{
			src.ReadToken( &token );
			FXAction.data = token;
			for( int i = 0; i < events.Num(); i++ )
			{
				if( events[i].name.Icmp( FXAction.data ) == 0 )
				{
					FXAction.sibling = i;
				}
			}
			FXAction.type = FX_MODEL;

			// precache the model
			renderModelManager->FindModel( FXAction.data );
			continue;
		}

		if( !token.Icmp( "light" ) )
		{
			src.ReadToken( &token );
			FXAction.data = token;
			src.ExpectTokenString( "," );
			FXAction.lightColor[0] = src.ParseFloat();
			src.ExpectTokenString( "," );
			FXAction.lightColor[1] = src.ParseFloat();
			src.ExpectTokenString( "," );
			FXAction.lightColor[2] = src.ParseFloat();
			src.ExpectTokenString( "," );
			FXAction.lightRadius = src.ParseFloat();
			FXAction.type = FX_LIGHT;

			// precache the light material
			declManager->FindMaterial( FXAction.data );
			continue;
		}

		if( !token.Icmp( "model" ) )
		{
			src.ReadToken( &token );
			FXAction.data = token;
			FXAction.type = FX_MODEL;

			// precache it
			renderModelManager->FindModel( FXAction.data );
			continue;
		}

		if( !token.Icmp( "particle" ) )  	// FIXME: now the same as model
		{
			src.ReadToken( &token );
			FXAction.data = token;
			FXAction.type = FX_PARTICLE;

			// precache it
			renderModelManager->FindModel( FXAction.data );
			continue;
		}

		if( !token.Icmp( "decal" ) )
		{
			src.ReadToken( &token );
			FXAction.data = token;
			FXAction.type = FX_DECAL;

			// precache it
			declManager->FindMaterial( FXAction.data );
			continue;
		}

		if( !token.Icmp( "particleTrackVelocity" ) )
		{
			FXAction.particleTrackVelocity = true;
			continue;
		}

		if( !token.Icmp( "sound" ) )
		{
			src.ReadToken( &token );
			FXAction.data = token;
			FXAction.type = FX_SOUND;

			// precache it
			declManager->FindSound( FXAction.data );
			continue;
		}

		if( !token.Icmp( "ignoreMaster" ) )
		{
			FXAction.shakeIgnoreMaster = true;
			continue;
		}

		if( !token.Icmp( "shockwave" ) )
		{
			src.ReadToken( &token );
			FXAction.data = token;
			FXAction.type = FX_SHOCKWAVE;

			// precache the entity def
			declManager->FindType( DECL_ENTITYDEF, FXAction.data );
			continue;
		}

		src.Warning( "FX File: bad token" );
		continue;
	}
}

/*
================
idDeclFX::Parse
================
*/
bool idDeclFX::Parse( const char* text, const int textLength, bool allowBinaryVersion )
{
	idLexer src;
	idToken token;

	src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	src.SetFlags( DECL_LEXER_FLAGS );
	src.SkipUntilString( "{" );

	// scan through, identifying each individual parameter
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

		if( !token.Icmp( "bindto" ) )
		{
			src.ReadToken( &token );
			joint = token;
			continue;
		}

		if( !token.Icmp( "{" ) )
		{
			idFXSingleAction action;
			ParseSingleFXAction( src, action );
			events.Append( action );
			continue;
		}
	}

	if( src.HadError() )
	{
		src.Warning( "FX decl '%s' had a parse error", GetName() );
		return false;
	}
	return true;
}

/*
===================
idDeclFX::DefaultDefinition
===================
*/
const char* idDeclFX::DefaultDefinition() const
{
	return
		"{\n"
		"\t"	"{\n"
		"\t\t"		"duration\t5\n"
		"\t\t"		"model\t\t_default\n"
		"\t"	"}\n"
		"}";
}

/*
===================
idDeclFX::FreeData
===================
*/
void idDeclFX::FreeData()
{
	events.Clear();
}
