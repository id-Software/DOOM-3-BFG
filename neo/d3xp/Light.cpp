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

#include "Game_local.h"

/*
===============================================================================

  idLight

===============================================================================
*/

const idEventDef EV_Light_SetShader( "setShader", "s" );
const idEventDef EV_Light_GetLightParm( "getLightParm", "d", 'f' );
const idEventDef EV_Light_SetLightParm( "setLightParm", "df" );
const idEventDef EV_Light_SetLightParms( "setLightParms", "ffff" );
const idEventDef EV_Light_SetRadiusXYZ( "setRadiusXYZ", "fff" );
const idEventDef EV_Light_SetRadius( "setRadius", "f" );
const idEventDef EV_Light_On( "On", NULL );
const idEventDef EV_Light_Off( "Off", NULL );
const idEventDef EV_Light_FadeOut( "fadeOutLight", "f" );
const idEventDef EV_Light_FadeIn( "fadeInLight", "f" );

CLASS_DECLARATION( idEntity, idLight )
EVENT( EV_Light_SetShader,		idLight::Event_SetShader )
EVENT( EV_Light_GetLightParm,	idLight::Event_GetLightParm )
EVENT( EV_Light_SetLightParm,	idLight::Event_SetLightParm )
EVENT( EV_Light_SetLightParms,	idLight::Event_SetLightParms )
EVENT( EV_Light_SetRadiusXYZ,	idLight::Event_SetRadiusXYZ )
EVENT( EV_Light_SetRadius,		idLight::Event_SetRadius )
EVENT( EV_Hide,					idLight::Event_Hide )
EVENT( EV_Show,					idLight::Event_Show )
EVENT( EV_Light_On,				idLight::Event_On )
EVENT( EV_Light_Off,			idLight::Event_Off )
EVENT( EV_Activate,				idLight::Event_ToggleOnOff )
EVENT( EV_PostSpawn,			idLight::Event_SetSoundHandles )
EVENT( EV_Light_FadeOut,		idLight::Event_FadeOut )
EVENT( EV_Light_FadeIn,			idLight::Event_FadeIn )
END_CLASS


/*
================
idGameEdit::ParseSpawnArgsToRenderLight

parse the light parameters
this is the canonical renderLight parm parsing,
which should be used by dmap and the editor
================
*/
void idGameEdit::ParseSpawnArgsToRenderLight( const idDict* args, renderLight_t* renderLight )
{
	bool	gotTarget, gotUp, gotRight;
	const char*	texture;
	idVec3	color;

	memset( renderLight, 0, sizeof( *renderLight ) );

	if( !args->GetVector( "light_origin", "", renderLight->origin ) )
	{
		args->GetVector( "origin", "", renderLight->origin );
	}

	gotTarget = args->GetVector( "light_target", "", renderLight->target );
	gotUp = args->GetVector( "light_up", "", renderLight->up );
	gotRight = args->GetVector( "light_right", "", renderLight->right );
	args->GetVector( "light_start", "0 0 0", renderLight->start );
	if( !args->GetVector( "light_end", "", renderLight->end ) )
	{
		renderLight->end = renderLight->target;
	}

	// we should have all of the target/right/up or none of them
	if( ( gotTarget || gotUp || gotRight ) != ( gotTarget && gotUp && gotRight ) )
	{
		gameLocal.Printf( "Light at (%f,%f,%f) has bad target info\n",
						  renderLight->origin[0], renderLight->origin[1], renderLight->origin[2] );
		return;
	}

	if( !gotTarget )
	{
		renderLight->pointLight = true;

		// allow an optional relative center of light and shadow offset
		args->GetVector( "light_center", "0 0 0", renderLight->lightCenter );

		// create a point light
		if( !args->GetVector( "light_radius", "300 300 300", renderLight->lightRadius ) )
		{
			float radius;

			args->GetFloat( "light", "300", radius );
			renderLight->lightRadius[0] = renderLight->lightRadius[1] = renderLight->lightRadius[2] = radius;
		}
	}

	// get the rotation matrix in either full form, or single angle form
	idAngles angles;
	idMat3 mat;
	if( !args->GetMatrix( "light_rotation", "1 0 0 0 1 0 0 0 1", mat ) )
	{
		if( !args->GetMatrix( "rotation", "1 0 0 0 1 0 0 0 1", mat ) )
		{
			// RB: TrenchBroom interop
			// support "angles" like in Quake 3

			if( args->GetAngles( "angles", "0 0 0", angles ) )
			{
				angles[ 0 ] = idMath::AngleNormalize360( angles[ 0 ] );
				angles[ 1 ] = idMath::AngleNormalize360( angles[ 1 ] );
				angles[ 2 ] = idMath::AngleNormalize360( angles[ 2 ] );

				mat = angles.ToMat3();
			}
			else
			{
				args->GetFloat( "angle", "0", angles[ 1 ] );
				angles[ 0 ] = 0;
				angles[ 1 ] = idMath::AngleNormalize360( angles[ 1 ] );
				angles[ 2 ] = 0;
				mat = angles.ToMat3();
			}
		}
	}

	// fix degenerate identity matrices
	mat[0].FixDegenerateNormal();
	mat[1].FixDegenerateNormal();
	mat[2].FixDegenerateNormal();

	renderLight->axis = mat;

	// check for other attributes
	args->GetVector( "_color", "1 1 1", color );
	renderLight->shaderParms[ SHADERPARM_RED ]		= color[0];
	renderLight->shaderParms[ SHADERPARM_GREEN ]	= color[1];
	renderLight->shaderParms[ SHADERPARM_BLUE ]		= color[2];
	args->GetFloat( "shaderParm3", "1", renderLight->shaderParms[ SHADERPARM_TIMESCALE ] );
	if( !args->GetFloat( "shaderParm4", "0", renderLight->shaderParms[ SHADERPARM_TIMEOFFSET ] ) )
	{
		// offset the start time of the shader to sync it to the game time
		renderLight->shaderParms[ SHADERPARM_TIMEOFFSET ] = -MS2SEC( gameLocal.time );
	}

	args->GetFloat( "shaderParm5", "0", renderLight->shaderParms[5] );
	args->GetFloat( "shaderParm6", "0", renderLight->shaderParms[6] );
	args->GetFloat( "shaderParm7", "0", renderLight->shaderParms[ SHADERPARM_MODE ] );
	args->GetBool( "noshadows", "0", renderLight->noShadows );
	args->GetBool( "nospecular", "0", renderLight->noSpecular );
	args->GetBool( "parallel", "0", renderLight->parallel );

	args->GetString( "texture", "lights/squarelight1", &texture );

	// allow this to be NULL
	renderLight->shader = declManager->FindMaterial( texture, false );
}

/*
================
idLight::UpdateChangeableSpawnArgs
================
*/
void idLight::UpdateChangeableSpawnArgs( const idDict* source )
{
// jmarshall
	lightStyleFrameTime = spawnArgs.GetInt( "ls_frametime", "100" );
	lightStyle = spawnArgs.GetInt( "style", -1 );

	lightStyleState.Reset();
// jmarshall end

	idEntity::UpdateChangeableSpawnArgs( source );

	if( source )
	{
		source->Print();
	}
	FreeSoundEmitter( true );
	gameEdit->ParseSpawnArgsToRefSound( source ? source : &spawnArgs, &refSound );
	if( refSound.shader && !refSound.waitfortrigger )
	{
		StartSoundShader( refSound.shader, SND_CHANNEL_ANY, 0, false, NULL );
	}

	gameEdit->ParseSpawnArgsToRenderLight( source ? source : &spawnArgs, &renderLight );

	// RB: allow the ingame light editor to move the light
	GetPhysics()->SetOrigin( renderLight.origin );
	GetPhysics()->SetAxis( renderLight.axis );
	// RB end

	UpdateVisuals();
}

/*
================
idLight::idLight
================
*/
idLight::idLight():
	previousBaseColor( vec3_zero ) ,
	nextBaseColor( vec3_zero )
{
	memset( &renderLight, 0, sizeof( renderLight ) );
	localLightOrigin	= vec3_zero;
	localLightAxis		= mat3_identity;
	lightDefHandle		= -1;
	levels				= 0;
	currentLevel		= 0;
	baseColor			= vec3_zero;
	breakOnTrigger		= false;
	count				= 0;
	triggercount		= 0;
	lightParent			= NULL;
	fadeFrom.Set( 1, 1, 1, 1 );
	fadeTo.Set( 1, 1, 1, 1 );
	fadeStart			= 0;
	fadeEnd				= 0;
	soundWasPlaying		= false;

// RB begin
	lightStyle			= -1;
	lightStyleFrameTime = 100;
	lightStyleBase.Set( 300, 300, 300 );

	lightStyleState.Reset();
// RB end
}

/*
================
idLight::~idLight
================
*/
idLight::~idLight()
{
	if( lightDefHandle != -1 )
	{
		gameRenderWorld->FreeLightDef( lightDefHandle );
	}
}

/*
================
idLight::Save

archives object for save game file
================
*/
void idLight::Save( idSaveGame* savefile ) const
{
	savefile->WriteRenderLight( renderLight );

	savefile->WriteBool( renderLight.prelightModel != NULL );

	savefile->WriteVec3( localLightOrigin );
	savefile->WriteMat3( localLightAxis );

	savefile->WriteString( brokenModel );
	savefile->WriteInt( levels );
	savefile->WriteInt( currentLevel );

	savefile->WriteVec3( baseColor );
	savefile->WriteBool( breakOnTrigger );
	savefile->WriteInt( count );
	savefile->WriteInt( triggercount );
	savefile->WriteObject( lightParent );

	savefile->WriteVec4( fadeFrom );
	savefile->WriteVec4( fadeTo );
	savefile->WriteInt( fadeStart );
	savefile->WriteInt( fadeEnd );
	savefile->WriteBool( soundWasPlaying );
}

/*
================
idLight::Restore

unarchives object from save game file
================
*/
void idLight::Restore( idRestoreGame* savefile )
{
	bool hadPrelightModel;

	savefile->ReadRenderLight( renderLight );

	savefile->ReadBool( hadPrelightModel );
	renderLight.prelightModel = renderModelManager->CheckModel( va( "_prelight_%s", name.c_str() ) );
	if( ( renderLight.prelightModel == NULL ) && hadPrelightModel )
	{
		assert( 0 );
		if( developer.GetBool() )
		{
			// we really want to know if this happens
			gameLocal.Error( "idLight::Restore: prelightModel '_prelight_%s' not found", name.c_str() );
		}
		else
		{
			// but let it slide after release
			gameLocal.Warning( "idLight::Restore: prelightModel '_prelight_%s' not found", name.c_str() );
		}
	}

	savefile->ReadVec3( localLightOrigin );
	savefile->ReadMat3( localLightAxis );

	savefile->ReadString( brokenModel );
	savefile->ReadInt( levels );
	savefile->ReadInt( currentLevel );

	savefile->ReadVec3( baseColor );
	savefile->ReadBool( breakOnTrigger );
	savefile->ReadInt( count );
	savefile->ReadInt( triggercount );
	savefile->ReadObject( reinterpret_cast<idClass*&>( lightParent ) );

	savefile->ReadVec4( fadeFrom );
	savefile->ReadVec4( fadeTo );
	savefile->ReadInt( fadeStart );
	savefile->ReadInt( fadeEnd );
	savefile->ReadBool( soundWasPlaying );

	lightDefHandle = -1;

	SetLightLevel();
}

/*
================
idLight::Spawn
================
*/
void idLight::Spawn()
{
	bool start_off;
	bool needBroken;
	const char* demonic_shader;

	// do the parsing the same way dmap and the editor do
	gameEdit->ParseSpawnArgsToRenderLight( &spawnArgs, &renderLight );
// jmarshall: Store the original light radius for the light style.
	lightStyleBase = renderLight.lightRadius;
// jmarshall end

	// we need the origin and axis relative to the physics origin/axis
	localLightOrigin = ( renderLight.origin - GetPhysics()->GetOrigin() ) * GetPhysics()->GetAxis().Transpose();
	localLightAxis = renderLight.axis * GetPhysics()->GetAxis().Transpose();

	// set the base color from the shader parms
	baseColor.Set( renderLight.shaderParms[ SHADERPARM_RED ], renderLight.shaderParms[ SHADERPARM_GREEN ], renderLight.shaderParms[ SHADERPARM_BLUE ] );
	previousBaseColor.Set( renderLight.shaderParms[ SHADERPARM_RED ], renderLight.shaderParms[ SHADERPARM_GREEN ], renderLight.shaderParms[ SHADERPARM_BLUE ] );
	nextBaseColor.Set( renderLight.shaderParms[ SHADERPARM_RED ], renderLight.shaderParms[ SHADERPARM_GREEN ], renderLight.shaderParms[ SHADERPARM_BLUE ] );

	// set the number of light levels
	spawnArgs.GetInt( "levels", "1", levels );
	currentLevel = levels;
	if( levels <= 0 )
	{
		gameLocal.Error( "Invalid light level set on entity #%d(%s)", entityNumber, name.c_str() );
	}

	// make sure the demonic shader is cached
	if( spawnArgs.GetString( "mat_demonic", NULL, &demonic_shader ) )
	{
		declManager->FindType( DECL_MATERIAL, demonic_shader );
	}

	// game specific functionality, not mirrored in
	// editor or dmap light parsing

	// also put the light texture on the model, so light flares
	// can get the current intensity of the light
	renderEntity.referenceShader = renderLight.shader;

	lightDefHandle = -1;		// no static version yet

	// see if an optimized shadow volume exists
	// the renderer will ignore this value after a light has been moved,
	// but there may still be a chance to get it wrong if the game moves
	// a light before the first present, and doesn't clear the prelight
	renderLight.prelightModel = 0;
	if( name[ 0 ] )
	{
		// this will return 0 if not found
		renderLight.prelightModel = renderModelManager->CheckModel( va( "_prelight_%s", name.c_str() ) );
	}

	spawnArgs.GetBool( "start_off", "0", start_off );
	if( start_off )
	{
		Off();
	}

	// Midnight CTF
	if( gameLocal.mpGame.IsGametypeFlagBased() && gameLocal.serverInfo.GetBool( "si_midnight" ) && !spawnArgs.GetBool( "midnight_override" ) )
	{
		Off();
	}

	health = spawnArgs.GetInt( "health", "0" );
	spawnArgs.GetString( "broken", "", brokenModel );
	spawnArgs.GetBool( "break", "0", breakOnTrigger );
	spawnArgs.GetInt( "count", "1", count );

// jmarshall
	lightStyleFrameTime = spawnArgs.GetInt( "ls_frametime", "100" );
	lightStyle = spawnArgs.GetInt( "style", -1 );

	int numStyles = spawnArgs.GetInt( "num_styles", "0" );
	if( numStyles > 0 )
	{
		for( int i = 0; i < numStyles; i++ )
		{
			idStr style = spawnArgs.GetString( va( "light_style%d", i ) );
			light_styles.Append( style );
		}
	}
	else
	{
		// RB: it's not defined in entityDef light so use predefined Quake 1 table
		for( int i = 0; i < 12; i++ )
		{
			idStr style = spawnArgs.GetString( va( "light_style%d", i ), predef_lightstyles[ i ] );
			light_styles.Append( style );
		}
	}
// jmarshall end

	triggercount = 0;

	fadeFrom.Set( 1, 1, 1, 1 );
	fadeTo.Set( 1, 1, 1, 1 );
	fadeStart			= 0;
	fadeEnd				= 0;

	// if we have a health make light breakable
	if( health )
	{
		idStr model = spawnArgs.GetString( "model" );		// get the visual model
		if( !model.Length() )
		{
			gameLocal.Error( "Breakable light without a model set on entity #%d(%s)", entityNumber, name.c_str() );
		}

		fl.takedamage	= true;

		// see if we need to create a broken model name
		needBroken = true;
		if( model.Length() && !brokenModel.Length() )
		{
			int	pos;

			needBroken = false;

			pos = model.Find( "." );
			if( pos < 0 )
			{
				pos = model.Length();
			}
			if( pos > 0 )
			{
				model.Left( pos, brokenModel );
			}
			brokenModel += "_broken";
			if( pos > 0 )
			{
				brokenModel += &model[ pos ];
			}
		}

		// make sure the model gets cached
		if( !renderModelManager->CheckModel( brokenModel ) )
		{
			if( needBroken )
			{
				gameLocal.Error( "Model '%s' not found for entity %d(%s)", brokenModel.c_str(), entityNumber, name.c_str() );
			}
			else
			{
				brokenModel = "";
			}
		}

		GetPhysics()->SetContents( spawnArgs.GetBool( "nonsolid" ) ? 0 : CONTENTS_SOLID );

		// make sure the collision model gets cached
		idClipModel::CheckModel( brokenModel );
	}

	PostEventMS( &EV_PostSpawn, 0 );

	UpdateVisuals();
}

/*
================
idLight::SetLightLevel
================
*/
void idLight::SetLightLevel()
{
	idVec3	color;
	float	intensity;

	intensity = ( float )currentLevel / ( float )levels;
	color = baseColor * intensity;
	renderLight.shaderParms[ SHADERPARM_RED ]	= color[ 0 ];
	renderLight.shaderParms[ SHADERPARM_GREEN ]	= color[ 1 ];
	renderLight.shaderParms[ SHADERPARM_BLUE ]	= color[ 2 ];
	renderEntity.shaderParms[ SHADERPARM_RED ]	= color[ 0 ];
	renderEntity.shaderParms[ SHADERPARM_GREEN ] = color[ 1 ];
	renderEntity.shaderParms[ SHADERPARM_BLUE ]	= color[ 2 ];
	PresentLightDefChange();
	PresentModelDefChange();
}

/*
================
idLight::GetColor
================
*/
void idLight::GetColor( idVec3& out ) const
{
	out[ 0 ] = renderLight.shaderParms[ SHADERPARM_RED ];
	out[ 1 ] = renderLight.shaderParms[ SHADERPARM_GREEN ];
	out[ 2 ] = renderLight.shaderParms[ SHADERPARM_BLUE ];
}

/*
================
idLight::GetColor
================
*/
void idLight::GetColor( idVec4& out ) const
{
	out[ 0 ] = renderLight.shaderParms[ SHADERPARM_RED ];
	out[ 1 ] = renderLight.shaderParms[ SHADERPARM_GREEN ];
	out[ 2 ] = renderLight.shaderParms[ SHADERPARM_BLUE ];
	out[ 3 ] = renderLight.shaderParms[ SHADERPARM_ALPHA ];
}

/*
================
idLight::SetColor
================
*/
void idLight::SetColor( float red, float green, float blue )
{
	baseColor.Set( red, green, blue );
	SetLightLevel();
}

/*
================
idLight::SetColor
================
*/
void idLight::SetColor( const idVec4& color )
{
	baseColor = color.ToVec3();
	renderLight.shaderParms[ SHADERPARM_ALPHA ]		= color[ 3 ];
	renderEntity.shaderParms[ SHADERPARM_ALPHA ]	= color[ 3 ];
	SetLightLevel();
}

/*
================
idLight::SetColor
================
*/
void idLight::SetColor( const idVec3& color )
{
	baseColor = color;
	SetLightLevel();
}

/*
================
idLight::SetShader
================
*/
void idLight::SetShader( const char* shadername )
{
	// allow this to be NULL
	renderLight.shader = declManager->FindMaterial( shadername, false );
	PresentLightDefChange();
}

/*
================
idLight::SetLightParm
================
*/
void idLight::SetLightParm( int parmnum, float value )
{
	if( ( parmnum < 0 ) || ( parmnum >= MAX_ENTITY_SHADER_PARMS ) )
	{
		gameLocal.Error( "shader parm index (%d) out of range", parmnum );
		return;
	}

	renderLight.shaderParms[ parmnum ] = value;
	PresentLightDefChange();
}

/*
================
idLight::SetLightParms
================
*/
void idLight::SetLightParms( float parm0, float parm1, float parm2, float parm3 )
{
	renderLight.shaderParms[ SHADERPARM_RED ]		= parm0;
	renderLight.shaderParms[ SHADERPARM_GREEN ]		= parm1;
	renderLight.shaderParms[ SHADERPARM_BLUE ]		= parm2;
	renderLight.shaderParms[ SHADERPARM_ALPHA ]		= parm3;
	renderEntity.shaderParms[ SHADERPARM_RED ]		= parm0;
	renderEntity.shaderParms[ SHADERPARM_GREEN ]	= parm1;
	renderEntity.shaderParms[ SHADERPARM_BLUE ]		= parm2;
	renderEntity.shaderParms[ SHADERPARM_ALPHA ]	= parm3;
	PresentLightDefChange();
	PresentModelDefChange();
}

/*
================
idLight::SetRadiusXYZ
================
*/
void idLight::SetRadiusXYZ( float x, float y, float z )
{
	renderLight.lightRadius[0] = x;
	renderLight.lightRadius[1] = y;
	renderLight.lightRadius[2] = z;
	PresentLightDefChange();
}

/*
================
idLight::SetRadius
================
*/
void idLight::SetRadius( float radius )
{
	renderLight.lightRadius[0] = renderLight.lightRadius[1] = renderLight.lightRadius[2] = radius;
	PresentLightDefChange();
}

/*
================
idLight::On
================
*/
void idLight::On()
{
	currentLevel = levels;
	// offset the start time of the shader to sync it to the game time
	renderLight.shaderParms[ SHADERPARM_TIMEOFFSET ] = -MS2SEC( gameLocal.time );
	if( ( soundWasPlaying || refSound.waitfortrigger ) && refSound.shader )
	{
		StartSoundShader( refSound.shader, SND_CHANNEL_ANY, 0, false, NULL );
		soundWasPlaying = false;
	}
	SetLightLevel();
	BecomeActive( TH_UPDATEVISUALS );
}

/*
================
idLight::Off
================
*/
void idLight::Off()
{
	currentLevel = 0;
	// kill any sound it was making
	if( refSound.referenceSound && refSound.referenceSound->CurrentlyPlaying() )
	{
		StopSound( SND_CHANNEL_ANY, false );
		soundWasPlaying = true;
	}
	SetLightLevel();
	BecomeActive( TH_UPDATEVISUALS );
}

/*
================
idLight::Fade
================
*/
void idLight::Fade( const idVec4& to, float fadeTime )
{
	GetColor( fadeFrom );
	fadeTo = to;
	fadeStart = gameLocal.time;
	fadeEnd = gameLocal.time + SEC2MS( fadeTime );
	BecomeActive( TH_THINK );
}

/*
================
idLight::FadeOut
================
*/
void idLight::FadeOut( float time )
{
	Fade( colorBlack, time );
}

/*
================
idLight::FadeIn
================
*/
void idLight::FadeIn( float time )
{
	idVec3 color;
	idVec4 color4;

	currentLevel = levels;
	spawnArgs.GetVector( "_color", "1 1 1", color );
	color4.Set( color.x, color.y, color.z, 1.0f );
	Fade( color4, time );
}

/*
================
idLight::Killed
================
*/
void idLight::Killed( idEntity* inflictor, idEntity* attacker, int damage, const idVec3& dir, int location )
{
	BecomeBroken( attacker );
}

/*
================
idLight::BecomeBroken
================
*/
void idLight::BecomeBroken( idEntity* activator )
{
	const char* damageDefName;

	fl.takedamage = false;

	if( brokenModel.Length() )
	{
		SetModel( brokenModel );

		if( !spawnArgs.GetBool( "nonsolid" ) )
		{
			GetPhysics()->SetClipModel( new( TAG_PHYSICS_CLIP_ENTITY ) idClipModel( brokenModel.c_str() ), 1.0f );
			GetPhysics()->SetContents( CONTENTS_SOLID );
		}
	}
	else if( spawnArgs.GetBool( "hideModelOnBreak" ) )
	{
		SetModel( "" );
		GetPhysics()->SetContents( 0 );
	}

	if( common->IsServer() )
	{

		ServerSendEvent( EVENT_BECOMEBROKEN, NULL, true );

		if( spawnArgs.GetString( "def_damage", "", &damageDefName ) )
		{
			idVec3 origin = renderEntity.origin + renderEntity.bounds.GetCenter() * renderEntity.axis;
			gameLocal.RadiusDamage( origin, activator, activator, this, this, damageDefName );
		}

	}

	ActivateTargets( activator );

	// offset the start time of the shader to sync it to the game time
	renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ] = -MS2SEC( gameLocal.time );
	renderLight.shaderParms[ SHADERPARM_TIMEOFFSET ] = -MS2SEC( gameLocal.time );

	// set the state parm
	renderEntity.shaderParms[ SHADERPARM_MODE ] = 1;
	renderLight.shaderParms[ SHADERPARM_MODE ] = 1;

	// if the light has a sound, either start the alternate (broken) sound, or stop the sound
	const char* parm = spawnArgs.GetString( "snd_broken" );
	if( refSound.shader || ( parm != NULL && *parm != '\0' ) )
	{
		StopSound( SND_CHANNEL_ANY, false );
		const idSoundShader* alternate = refSound.shader ? refSound.shader->GetAltSound() : declManager->FindSound( parm );
		if( alternate )
		{
			// start it with no diversity, so the leadin break sound plays
			refSound.referenceSound->StartSound( alternate, SND_CHANNEL_ANY, 0.0, 0 );
		}
	}

	parm = spawnArgs.GetString( "mtr_broken" );
	if( parm != NULL && *parm != '\0' )
	{
		SetShader( parm );
	}

	UpdateVisuals();
}

/*
================
idLight::PresentLightDefChange
================
*/
void idLight::PresentLightDefChange()
{
	// let the renderer apply it to the world
	if( ( lightDefHandle != -1 ) )
	{
		gameRenderWorld->UpdateLightDef( lightDefHandle, &renderLight );
	}
	else
	{
		lightDefHandle = gameRenderWorld->AddLightDef( &renderLight );
	}
}

/*
================
idLight::PresentModelDefChange
================
*/
void idLight::PresentModelDefChange()
{

	if( !renderEntity.hModel || IsHidden() )
	{
		return;
	}

	// add to refresh list
	if( modelDefHandle == -1 )
	{
		modelDefHandle = gameRenderWorld->AddEntityDef( &renderEntity );
	}
	else
	{
		gameRenderWorld->UpdateEntityDef( modelDefHandle, &renderEntity );
	}
}

/*
================
idLight::Present
================
*/
void idLight::Present()
{
	// don't present to the renderer if the entity hasn't changed
	if( !( thinkFlags & TH_UPDATEVISUALS ) )
	{
		return;
	}

	// add the model
	idEntity::Present();

	// current transformation
	renderLight.axis	= localLightAxis * GetPhysics()->GetAxis();
	renderLight.origin  = GetPhysics()->GetOrigin() + GetPhysics()->GetAxis() * localLightOrigin;

	// reference the sound for shader synced effects
	if( lightParent )
	{
		renderLight.referenceSound = lightParent->GetSoundEmitter();
		renderEntity.referenceSound = lightParent->GetSoundEmitter();
	}
	else
	{
		renderLight.referenceSound = refSound.referenceSound;
		renderEntity.referenceSound = refSound.referenceSound;
	}

	// update the renderLight and renderEntity to render the light and flare
	PresentLightDefChange();
	PresentModelDefChange();
}

/*
================
idLight::Think
================
*/
void idLight::Think()
{
	idVec4 color;

	if( thinkFlags & TH_THINK )
	{
		if( fadeEnd > 0 )
		{
			if( gameLocal.time < fadeEnd )
			{
				color.Lerp( fadeFrom, fadeTo, ( float )( gameLocal.time - fadeStart ) / ( float )( fadeEnd - fadeStart ) );
			}
			else
			{
				color = fadeTo;
				fadeEnd = 0;
				BecomeInactive( TH_THINK );
			}
			SetColor( color );
		}
	}

	RunPhysics();
	Present();
}

/*
================
idLight::SharedThink
================
*/
// jmarshall
void idLight::SharedThink()
{
	float lightval;
	int stringlength;
	float offset;
	int offsetwhole;
	int otime;
	int lastch, nextch;

	if( lightStyle == -1 )
	{
		return;
	}

	if( lightStyle > light_styles.Num() )
	{
		//gameLocal.Error( "Light style out of range\n" );
		return;
	}

	idStr dl_stylestring = light_styles[lightStyle];

	otime = gameLocal.time - lightStyleState.dl_time;
	stringlength = dl_stylestring.Length();

	// it's been a long time since you were updated, lets assume a reset
	if( otime > 2 * lightStyleFrameTime )
	{
		otime = 0;
		lightStyleState.dl_frame = lightStyleState.dl_oldframe = 0;
		lightStyleState.dl_backlerp = 0;
	}

	lightStyleState.dl_time = gameLocal.time;

	offset = ( ( float )otime ) / lightStyleFrameTime;
	offsetwhole = ( int )offset;

	lightStyleState.dl_backlerp += offset;


	if( lightStyleState.dl_backlerp > 1 )                      // we're moving on to the next frame
	{
		lightStyleState.dl_oldframe = lightStyleState.dl_oldframe + ( int )lightStyleState.dl_backlerp;
		lightStyleState.dl_frame = lightStyleState.dl_oldframe + 1;
		if( lightStyleState.dl_oldframe >= stringlength )
		{
			lightStyleState.dl_oldframe = ( lightStyleState.dl_oldframe ) % stringlength;
			//if (cent->dl_oldframe < 3 && cent->dl_sound) { // < 3 so if an alarm comes back into the pvs it will only start a sound if it's going to be closely synced with the light, otherwise wait till the next cycle
			//	engine->S_StartSound(NULL, cent->currentState.number, CHAN_AUTO, cgs.gameSounds[cent->dl_sound]);
			//}
		}

		if( lightStyleState.dl_frame >= stringlength )
		{
			lightStyleState.dl_frame = ( lightStyleState.dl_frame ) % stringlength;
		}

		lightStyleState.dl_backlerp = lightStyleState.dl_backlerp - ( int )lightStyleState.dl_backlerp;
	}


	lastch = dl_stylestring[lightStyleState.dl_oldframe] - 'a';
	nextch = dl_stylestring[lightStyleState.dl_frame] - 'a';

	lightval = ( lastch * ( 1.0 - lightStyleState.dl_backlerp ) ) + ( nextch * lightStyleState.dl_backlerp );

	// ydnar: dlight values go from 0-1.5ish
#if 0
	lightval = ( lightval * ( 1000.0f / 24.0f ) ) - 200.0f; // they want 'm' as the "middle" value as 300
	lightval = max( 0.0f, lightval );
	lightval = min( 1000.0f, lightval );
#else
	lightval *= 0.071429f;
	lightval = Max( 0.0f, lightval );
	lightval = Min( 20.0f, lightval );
#endif

	renderLight.lightRadius.x = lightval * lightStyleBase.x;
	renderLight.lightRadius.y = lightval * lightStyleBase.y;
	renderLight.lightRadius.z = lightval * lightStyleBase.z;


	if( !common->IsClient() )
	{
		BecomeActive( TH_THINK );
	}

	PresentLightDefChange();
}
// jmarshall end

/*
================
idLight::ClientThink
================
*/
void idLight::ClientThink( const int curTime, const float fraction, const bool predict )
{

	InterpolatePhysics( fraction );

	if( baseColor != nextBaseColor )
	{
		baseColor = Lerp( previousBaseColor, nextBaseColor, fraction );
		SetColor( baseColor );
		BecomeActive( TH_UPDATEVISUALS );
	}

	Present();
}

/*
================
idLight::GetPhysicsToSoundTransform
================
*/
bool idLight::GetPhysicsToSoundTransform( idVec3& origin, idMat3& axis )
{
	origin = localLightOrigin + renderLight.lightCenter;
	axis = localLightAxis * GetPhysics()->GetAxis();
	return true;
}

/*
================
idLight::FreeLightDef
================
*/
void idLight::FreeLightDef()
{
	if( lightDefHandle != -1 )
	{
		gameRenderWorld->FreeLightDef( lightDefHandle );
		lightDefHandle = -1;
	}
}

/*
================
idLight::SaveState
================
*/
void idLight::SaveState( idDict* args )
{
	int i, c = spawnArgs.GetNumKeyVals();
	for( i = 0; i < c; i++ )
	{
		const idKeyValue* pv = spawnArgs.GetKeyVal( i );
		if( pv->GetKey().Find( "editor_", false ) >= 0 || pv->GetKey().Find( "parse_", false ) >= 0 )
		{
			continue;
		}
		args->Set( pv->GetKey(), pv->GetValue() );
	}
}

/*
===============
idLight::ShowEditingDialog
===============
*/
void idLight::ShowEditingDialog()
{
	if( g_editEntityMode.GetInteger() == 1 )
	{
		common->InitTool( EDITOR_LIGHT, &spawnArgs , this );
	}
	else
	{
		common->InitTool( EDITOR_SOUND, &spawnArgs, this );
	}
}

/*
================
idLight::Event_SetShader
================
*/
void idLight::Event_SetShader( const char* shadername )
{
	SetShader( shadername );
}

/*
================
idLight::Event_GetLightParm
================
*/
void idLight::Event_GetLightParm( int parmnum )
{
	if( ( parmnum < 0 ) || ( parmnum >= MAX_ENTITY_SHADER_PARMS ) )
	{
		gameLocal.Error( "shader parm index (%d) out of range", parmnum );
		return;
	}

	idThread::ReturnFloat( renderLight.shaderParms[ parmnum ] );
}

/*
================
idLight::Event_SetLightParm
================
*/
void idLight::Event_SetLightParm( int parmnum, float value )
{
	SetLightParm( parmnum, value );
}

/*
================
idLight::Event_SetLightParms
================
*/
void idLight::Event_SetLightParms( float parm0, float parm1, float parm2, float parm3 )
{
	SetLightParms( parm0, parm1, parm2, parm3 );
}

/*
================
idLight::Event_SetRadiusXYZ
================
*/
void idLight::Event_SetRadiusXYZ( float x, float y, float z )
{
	SetRadiusXYZ( x, y, z );
}

/*
================
idLight::Event_SetRadius
================
*/
void idLight::Event_SetRadius( float radius )
{
	SetRadius( radius );
}

/*
================
idLight::Event_Hide
================
*/
void idLight::Event_Hide()
{
	Hide();
	PresentModelDefChange();
	Off();
}

/*
================
idLight::Event_Show
================
*/
void idLight::Event_Show()
{
	Show();
	PresentModelDefChange();
	On();
}

/*
================
idLight::Event_On
================
*/
void idLight::Event_On()
{
	On();
}

/*
================
idLight::Event_Off
================
*/
void idLight::Event_Off()
{
	Off();
}

/*
================
idLight::Event_ToggleOnOff
================
*/
void idLight::Event_ToggleOnOff( idEntity* activator )
{
	triggercount++;
	if( triggercount < count )
	{
		return;
	}

	// reset trigger count
	triggercount = 0;

	if( breakOnTrigger )
	{
		BecomeBroken( activator );
		breakOnTrigger = false;
		return;
	}

	if( !currentLevel )
	{
		On();
	}
	else
	{
		currentLevel--;
		if( !currentLevel )
		{
			Off();
		}
		else
		{
			SetLightLevel();
		}
	}
}

/*
================
idLight::Event_SetSoundHandles

  set the same sound def handle on all targeted lights
================
*/
void idLight::Event_SetSoundHandles()
{
	int i;
	idEntity* targetEnt;

	if( !refSound.referenceSound )
	{
		return;
	}

	for( i = 0; i < targets.Num(); i++ )
	{
		targetEnt = targets[ i ].GetEntity();
		if( targetEnt != NULL && targetEnt->IsType( idLight::Type ) )
		{
			idLight*	light = static_cast<idLight*>( targetEnt );
			light->lightParent = this;

			// explicitly delete any sounds on the entity
			light->FreeSoundEmitter( true );

			// manually set the refSound to this light's refSound
			light->renderEntity.referenceSound = renderEntity.referenceSound;

			// update the renderEntity to the renderer
			light->UpdateVisuals();
		}
	}
}

/*
================
idLight::Event_FadeOut
================
*/
void idLight::Event_FadeOut( float time )
{
	FadeOut( time );
}

/*
================
idLight::Event_FadeIn
================
*/
void idLight::Event_FadeIn( float time )
{
	FadeIn( time );
}

/*
================
idLight::ClientPredictionThink
================
*/
void idLight::ClientPredictionThink()
{
	Think();
}

/*
================
idLight::WriteToSnapshot
================
*/
void idLight::WriteToSnapshot( idBitMsg& msg ) const
{

	GetPhysics()->WriteToSnapshot( msg );
	WriteBindToSnapshot( msg );

	msg.WriteByte( currentLevel );
	msg.WriteLong( PackColor( baseColor ) );
	// msg.WriteBits( lightParent.GetEntityNum(), GENTITYNUM_BITS );

	/*	// only helps prediction
		msg.WriteLong( PackColor( fadeFrom ) );
		msg.WriteLong( PackColor( fadeTo ) );
		msg.WriteLong( fadeStart );
		msg.WriteLong( fadeEnd );
	*/

	// FIXME: send renderLight.shader
	msg.WriteFloat( renderLight.lightRadius[0], 5, 10 );
	msg.WriteFloat( renderLight.lightRadius[1], 5, 10 );
	msg.WriteFloat( renderLight.lightRadius[2], 5, 10 );

	msg.WriteLong( PackColor( idVec4( renderLight.shaderParms[SHADERPARM_RED],
									  renderLight.shaderParms[SHADERPARM_GREEN],
									  renderLight.shaderParms[SHADERPARM_BLUE],
									  renderLight.shaderParms[SHADERPARM_ALPHA] ) ) );

	msg.WriteFloat( renderLight.shaderParms[SHADERPARM_TIMESCALE], 5, 10 );
	msg.WriteLong( renderLight.shaderParms[SHADERPARM_TIMEOFFSET] );
	//msg.WriteByte( renderLight.shaderParms[SHADERPARM_DIVERSITY] );
	msg.WriteShort( renderLight.shaderParms[SHADERPARM_MODE] );

	WriteColorToSnapshot( msg );
}

/*
================
idLight::ReadFromSnapshot
================
*/
void idLight::ReadFromSnapshot( const idBitMsg& msg )
{
	idVec4	shaderColor;
	int		oldCurrentLevel = currentLevel;
	idVec3	oldBaseColor = baseColor;

	previousBaseColor = nextBaseColor;

	GetPhysics()->ReadFromSnapshot( msg );
	ReadBindFromSnapshot( msg );

	currentLevel = msg.ReadByte();
	if( currentLevel != oldCurrentLevel )
	{
		// need to call On/Off for flickering lights to start/stop the sound
		// while doing it this way rather than through events, the flickering is out of sync between clients
		// but at least there is no question about saving the event and having them happening globally in the world
		if( currentLevel )
		{
			On();
		}
		else
		{
			Off();
		}
	}

	UnpackColor( msg.ReadLong(), nextBaseColor );

	// lightParentEntityNum = msg.ReadBits( GENTITYNUM_BITS );

	/*	// only helps prediction
		UnpackColor( msg.ReadLong(), fadeFrom );
		UnpackColor( msg.ReadLong(), fadeTo );
		fadeStart = msg.ReadLong();
		fadeEnd = msg.ReadLong();
	*/

	// FIXME: read renderLight.shader
	renderLight.lightRadius[0] = msg.ReadFloat( 5, 10 );
	renderLight.lightRadius[1] = msg.ReadFloat( 5, 10 );
	renderLight.lightRadius[2] = msg.ReadFloat( 5, 10 );

	UnpackColor( msg.ReadLong(), shaderColor );
	renderLight.shaderParms[SHADERPARM_RED] = shaderColor[0];
	renderLight.shaderParms[SHADERPARM_GREEN] = shaderColor[1];
	renderLight.shaderParms[SHADERPARM_BLUE] = shaderColor[2];
	renderLight.shaderParms[SHADERPARM_ALPHA] = shaderColor[3];

	renderLight.shaderParms[SHADERPARM_TIMESCALE] = msg.ReadFloat( 5, 10 );
	renderLight.shaderParms[SHADERPARM_TIMEOFFSET] = msg.ReadLong();
	//renderLight.shaderParms[SHADERPARM_DIVERSITY] = msg.ReadFloat();
	renderLight.shaderParms[SHADERPARM_MODE] = msg.ReadShort();

	ReadColorFromSnapshot( msg );

	if( msg.HasChanged() )
	{
		if( ( currentLevel != oldCurrentLevel ) || ( previousBaseColor != nextBaseColor ) )
		{
			SetLightLevel();
		}
		else
		{
			PresentLightDefChange();
			PresentModelDefChange();
		}
	}
}

/*
================
idLight::ClientReceiveEvent
================
*/
bool idLight::ClientReceiveEvent( int event, int time, const idBitMsg& msg )
{

	switch( event )
	{
		case EVENT_BECOMEBROKEN:
		{
			BecomeBroken( NULL );
			return true;
		}
		default:
		{
			return idEntity::ClientReceiveEvent( event, time, msg );
		}
	}
}
