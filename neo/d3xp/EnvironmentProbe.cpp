/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2015-2021 Robert Beckebans

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

const idEventDef EV_Envprobe_GetEnvprobeParm( "getEnvprobeParm", "d", 'f' );
const idEventDef EV_Envprobe_SetEnvprobeParm( "setEnvprobeParm", "df" );
const idEventDef EV_Envprobe_SetEnvprobeParms( "setEnvprobeParms", "ffff" );
//const idEventDef EV_Envprobe_SetRadiusXYZ( "setRadiusXYZ", "fff" );
//const idEventDef EV_Envprobe_SetRadius( "setRadius", "f" );
const idEventDef EV_Envprobe_On( "On", NULL );
const idEventDef EV_Envprobe_Off( "Off", NULL );
const idEventDef EV_Envprobe_FadeOut( "fadeOutEnvprobe", "f" );
const idEventDef EV_Envprobe_FadeIn( "fadeInEnvprobe", "f" );

CLASS_DECLARATION( idEntity, EnvironmentProbe )
EVENT( EV_Envprobe_GetEnvprobeParm,		EnvironmentProbe::Event_GetEnvprobeParm )
EVENT( EV_Envprobe_SetEnvprobeParm,		EnvironmentProbe::Event_SetEnvprobeParm )
EVENT( EV_Envprobe_SetEnvprobeParms,	EnvironmentProbe::Event_SetEnvprobeParms )
//EVENT( EV_Envprobe_SetRadiusXYZ,		EnvironmentProbe::Event_SetRadiusXYZ )
//EVENT( EV_Envprobe_SetRadius,			EnvironmentProbe::Event_SetRadius )
EVENT( EV_Hide,							EnvironmentProbe::Event_Hide )
EVENT( EV_Show,							EnvironmentProbe::Event_Show )
EVENT( EV_Envprobe_On,					EnvironmentProbe::Event_On )
EVENT( EV_Envprobe_Off,					EnvironmentProbe::Event_Off )
EVENT( EV_Activate,						EnvironmentProbe::Event_ToggleOnOff )
//EVENT( EV_PostSpawn,					EnvironmentProbe::Event_SetSoundHandles )
EVENT( EV_Envprobe_FadeOut,				EnvironmentProbe::Event_FadeOut )
EVENT( EV_Envprobe_FadeIn,				EnvironmentProbe::Event_FadeIn )
END_CLASS


/*
================
idGameEdit::ParseSpawnArgsToRenderLight

parse the light parameters
this is the canonical renderLight parm parsing,
which should be used by dmap and the editor
================
*/
void idGameEdit::ParseSpawnArgsToRenderEnvprobe( const idDict* args, renderEnvironmentProbe_t* renderEnvprobe )
{
	idVec3	color;

	memset( renderEnvprobe, 0, sizeof( *renderEnvprobe ) );

	if( !args->GetVector( "light_origin", "", renderEnvprobe->origin ) )
	{
		args->GetVector( "origin", "", renderEnvprobe->origin );
	}

	// check for other attributes
	args->GetVector( "_color", "1 1 1", color );
	renderEnvprobe->shaderParms[ SHADERPARM_RED ]	= color[0];
	renderEnvprobe->shaderParms[ SHADERPARM_GREEN ]	= color[1];
	renderEnvprobe->shaderParms[ SHADERPARM_BLUE ]	= color[2];
	args->GetFloat( "shaderParm3", "1", renderEnvprobe->shaderParms[ SHADERPARM_TIMESCALE ] );
	if( !args->GetFloat( "shaderParm4", "0", renderEnvprobe->shaderParms[ SHADERPARM_TIMEOFFSET ] ) )
	{
		// offset the start time of the shader to sync it to the game time
		renderEnvprobe->shaderParms[ SHADERPARM_TIMEOFFSET ] = -MS2SEC( gameLocal.time );
	}

	args->GetFloat( "shaderParm5", "0", renderEnvprobe->shaderParms[5] );
	args->GetFloat( "shaderParm6", "0", renderEnvprobe->shaderParms[6] );
	args->GetFloat( "shaderParm7", "0", renderEnvprobe->shaderParms[ SHADERPARM_MODE ] );
}

/*
================
EnvironmentProbe::UpdateChangeableSpawnArgs
================
*/
void EnvironmentProbe::UpdateChangeableSpawnArgs( const idDict* source )
{
	idEntity::UpdateChangeableSpawnArgs( source );

	if( source )
	{
		source->Print();
	}

	gameEdit->ParseSpawnArgsToRenderEnvprobe( source ? source : &spawnArgs, &renderEnvprobe );

	UpdateVisuals();
}

/*
================
EnvironmentProbe::EnvironmentProbe
================
*/
EnvironmentProbe::EnvironmentProbe():
	previousBaseColor( vec3_zero ) ,
	nextBaseColor( vec3_zero )
{
	memset( &renderEnvprobe, 0, sizeof( renderEnvprobe ) );
	localEnvprobeOrigin	= vec3_zero;
	localEnvprobeAxis	= mat3_identity;
	envprobeDefHandle	= -1;
	levels				= 0;
	currentLevel		= 0;
	baseColor			= vec3_zero;
	count				= 0;
	triggercount		= 0;
	lightParent			= NULL;
	fadeFrom.Set( 1, 1, 1, 1 );
	fadeTo.Set( 1, 1, 1, 1 );
	fadeStart			= 0;
	fadeEnd				= 0;
}

/*
================
EnvironmentProbe::~idLight
================
*/
EnvironmentProbe::~EnvironmentProbe()
{
	if( envprobeDefHandle != -1 )
	{
		gameRenderWorld->FreeEnvprobeDef( envprobeDefHandle );
	}
}

/*
================
EnvironmentProbe::Save

archives object for save game file
================
*/
void EnvironmentProbe::Save( idSaveGame* savefile ) const
{
	savefile->WriteRenderEnvprobe( renderEnvprobe );

	savefile->WriteVec3( localEnvprobeOrigin );
	savefile->WriteMat3( localEnvprobeAxis );

	savefile->WriteInt( levels );
	savefile->WriteInt( currentLevel );

	savefile->WriteVec3( baseColor );
	savefile->WriteInt( count );
	savefile->WriteInt( triggercount );
	savefile->WriteObject( lightParent );

	savefile->WriteVec4( fadeFrom );
	savefile->WriteVec4( fadeTo );
	savefile->WriteInt( fadeStart );
	savefile->WriteInt( fadeEnd );
}

/*
================
EnvironmentProbe::Restore

unarchives object from save game file
================
*/
void EnvironmentProbe::Restore( idRestoreGame* savefile )
{
	savefile->ReadRenderEnvprobe( renderEnvprobe );

	savefile->ReadVec3( localEnvprobeOrigin );
	savefile->ReadMat3( localEnvprobeAxis );

	savefile->ReadInt( levels );
	savefile->ReadInt( currentLevel );

	savefile->ReadVec3( baseColor );
	savefile->ReadInt( count );
	savefile->ReadInt( triggercount );
	savefile->ReadObject( reinterpret_cast<idClass*&>( lightParent ) );

	savefile->ReadVec4( fadeFrom );
	savefile->ReadVec4( fadeTo );
	savefile->ReadInt( fadeStart );
	savefile->ReadInt( fadeEnd );

	envprobeDefHandle = -1;

	SetLightLevel();
}

/*
================
EnvironmentProbe::Spawn
================
*/
void EnvironmentProbe::Spawn()
{
	bool start_off;

	// do the parsing the same way dmap and the editor do
	gameEdit->ParseSpawnArgsToRenderEnvprobe( &spawnArgs, &renderEnvprobe );

	// we need the origin and axis relative to the physics origin/axis
	localEnvprobeOrigin = ( renderEnvprobe.origin - GetPhysics()->GetOrigin() ) * GetPhysics()->GetAxis().Transpose();
	localEnvprobeAxis = /*renderEnvprobe.axis * */ GetPhysics()->GetAxis().Transpose();

	// set the base color from the shader parms
	baseColor.Set( renderEnvprobe.shaderParms[ SHADERPARM_RED ], renderEnvprobe.shaderParms[ SHADERPARM_GREEN ], renderEnvprobe.shaderParms[ SHADERPARM_BLUE ] );
	previousBaseColor.Set( renderEnvprobe.shaderParms[ SHADERPARM_RED ], renderEnvprobe.shaderParms[ SHADERPARM_GREEN ], renderEnvprobe.shaderParms[ SHADERPARM_BLUE ] );
	nextBaseColor.Set( renderEnvprobe.shaderParms[ SHADERPARM_RED ], renderEnvprobe.shaderParms[ SHADERPARM_GREEN ], renderEnvprobe.shaderParms[ SHADERPARM_BLUE ] );

	// set the number of light levels
	spawnArgs.GetInt( "levels", "1", levels );
	currentLevel = levels;
	if( levels <= 0 )
	{
		gameLocal.Error( "Invalid light level set on entity #%d(%s)", entityNumber, name.c_str() );
	}

	// game specific functionality, not mirrored in
	// editor or dmap light parsing

	envprobeDefHandle = -1;		// no static version yet

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
	spawnArgs.GetInt( "count", "1", count );

	triggercount = 0;

	fadeFrom.Set( 1, 1, 1, 1 );
	fadeTo.Set( 1, 1, 1, 1 );
	fadeStart			= 0;
	fadeEnd				= 0;

	PostEventMS( &EV_PostSpawn, 0 );

	UpdateVisuals();
}

/*
================
EnvironmentProbe::SetLightLevel
================
*/
void EnvironmentProbe::SetLightLevel()
{
	idVec3	color;
	float	intensity;

	intensity = ( float )currentLevel / ( float )levels;
	color = baseColor * intensity;
	renderEnvprobe.shaderParms[ SHADERPARM_RED ]	= color[ 0 ];
	renderEnvprobe.shaderParms[ SHADERPARM_GREEN ]	= color[ 1 ];
	renderEnvprobe.shaderParms[ SHADERPARM_BLUE ]	= color[ 2 ];

	PresentEnvprobeDefChange();
}

/*
================
EnvironmentProbe::GetColor
================
*/
void EnvironmentProbe::GetColor( idVec3& out ) const
{
	out[ 0 ] = renderEnvprobe.shaderParms[ SHADERPARM_RED ];
	out[ 1 ] = renderEnvprobe.shaderParms[ SHADERPARM_GREEN ];
	out[ 2 ] = renderEnvprobe.shaderParms[ SHADERPARM_BLUE ];
}

/*
================
EnvironmentProbe::GetColor
================
*/
void EnvironmentProbe::GetColor( idVec4& out ) const
{
	out[ 0 ] = renderEnvprobe.shaderParms[ SHADERPARM_RED ];
	out[ 1 ] = renderEnvprobe.shaderParms[ SHADERPARM_GREEN ];
	out[ 2 ] = renderEnvprobe.shaderParms[ SHADERPARM_BLUE ];
	out[ 3 ] = renderEnvprobe.shaderParms[ SHADERPARM_ALPHA ];
}

/*
================
EnvironmentProbe::SetColor
================
*/
void EnvironmentProbe::SetColor( float red, float green, float blue )
{
	baseColor.Set( red, green, blue );
	SetLightLevel();
}

/*
================
EnvironmentProbe::SetColor
================
*/
void EnvironmentProbe::SetColor( const idVec4& color )
{
	baseColor = color.ToVec3();
	renderEnvprobe.shaderParms[ SHADERPARM_ALPHA ]		= color[ 3 ];
	SetLightLevel();
}

/*
================
EnvironmentProbe::SetColor
================
*/
void EnvironmentProbe::SetColor( const idVec3& color )
{
	baseColor = color;
	SetLightLevel();
}

/*
================
EnvironmentProbe::SetEnvprobeParm
================
*/
void EnvironmentProbe::SetEnvprobeParm( int parmnum, float value )
{
	if( ( parmnum < 0 ) || ( parmnum >= MAX_ENTITY_SHADER_PARMS ) )
	{
		gameLocal.Error( "shader parm index (%d) out of range", parmnum );
		return;
	}

	renderEnvprobe.shaderParms[ parmnum ] = value;
	PresentEnvprobeDefChange();
}

/*
================
EnvironmentProbe::SetEnvprobeParms
================
*/
void EnvironmentProbe::SetEnvprobeParms( float parm0, float parm1, float parm2, float parm3 )
{
	renderEnvprobe.shaderParms[ SHADERPARM_RED ]		= parm0;
	renderEnvprobe.shaderParms[ SHADERPARM_GREEN ]		= parm1;
	renderEnvprobe.shaderParms[ SHADERPARM_BLUE ]		= parm2;
	renderEnvprobe.shaderParms[ SHADERPARM_ALPHA ]		= parm3;
	PresentEnvprobeDefChange();
}

/*
================
EnvironmentProbe::On
================
*/
void EnvironmentProbe::On()
{
	currentLevel = levels;
	// offset the start time of the shader to sync it to the game time
	renderEnvprobe.shaderParms[ SHADERPARM_TIMEOFFSET ] = -MS2SEC( gameLocal.time );

	SetLightLevel();
	BecomeActive( TH_UPDATEVISUALS );
}

/*
================
EnvironmentProbe::Off
================
*/
void EnvironmentProbe::Off()
{
	currentLevel = 0;

	SetLightLevel();
	BecomeActive( TH_UPDATEVISUALS );
}

/*
================
EnvironmentProbe::Fade
================
*/
void EnvironmentProbe::Fade( const idVec4& to, float fadeTime )
{
	GetColor( fadeFrom );
	fadeTo = to;
	fadeStart = gameLocal.time;
	fadeEnd = gameLocal.time + SEC2MS( fadeTime );
	BecomeActive( TH_THINK );
}

/*
================
EnvironmentProbe::FadeOut
================
*/
void EnvironmentProbe::FadeOut( float time )
{
	Fade( colorBlack, time );
}

/*
================
EnvironmentProbe::FadeIn
================
*/
void EnvironmentProbe::FadeIn( float time )
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
EnvironmentProbe::PresentEnvprobeDefChange
================
*/
void EnvironmentProbe::PresentEnvprobeDefChange()
{
	// let the renderer apply it to the world
	if( ( envprobeDefHandle != -1 ) )
	{
		gameRenderWorld->UpdateEnvprobeDef( envprobeDefHandle, &renderEnvprobe );
	}
	else
	{
		envprobeDefHandle = gameRenderWorld->AddEnvprobeDef( &renderEnvprobe );
	}
}

/*
================
EnvironmentProbe::Present
================
*/
void EnvironmentProbe::Present()
{
	// don't present to the renderer if the entity hasn't changed
	if( !( thinkFlags & TH_UPDATEVISUALS ) )
	{
		return;
	}

	// add the model
	idEntity::Present();

	// current transformation
//	renderEnvprobe.axis	= localEnvprobeAxis * GetPhysics()->GetAxis();
	renderEnvprobe.origin  = GetPhysics()->GetOrigin() + GetPhysics()->GetAxis() * localEnvprobeOrigin;

	// reference the sound for shader synced effects
	// FIXME TODO?
	/*
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
	*/

	// update the renderLight and renderEntity to render the light and flare
	PresentEnvprobeDefChange();
}

/*
================
EnvironmentProbe::Think
================
*/
void EnvironmentProbe::Think()
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
EnvironmentProbe::ClientThink
================
*/
void EnvironmentProbe::ClientThink( const int curTime, const float fraction, const bool predict )
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
EnvironmentProbe::GetPhysicsToSoundTransform
================
*/
bool EnvironmentProbe::GetPhysicsToSoundTransform( idVec3& origin, idMat3& axis )
{
	//origin = localEnvprobeOrigin + renderEnvprobe.lightCenter;
	//axis = localLightAxis * GetPhysics()->GetAxis();
	//return true;

	return false;
}

/*
================
EnvironmentProbe::FreeEnvprobeDef
================
*/
void EnvironmentProbe::FreeEnvprobeDef()
{
	if( envprobeDefHandle != -1 )
	{
		gameRenderWorld->FreeEnvprobeDef( envprobeDefHandle );
		envprobeDefHandle = -1;
	}
}

/*
================
EnvironmentProbe::SaveState
================
*/
void EnvironmentProbe::SaveState( idDict* args )
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
EnvironmentProbe::ShowEditingDialog
===============
*/
void EnvironmentProbe::ShowEditingDialog()
{
}

/*
================
EnvironmentProbe::Event_GetEnvprobeParm
================
*/
void EnvironmentProbe::Event_GetEnvprobeParm( int parmnum )
{
	if( ( parmnum < 0 ) || ( parmnum >= MAX_ENTITY_SHADER_PARMS ) )
	{
		gameLocal.Error( "shader parm index (%d) out of range", parmnum );
		return;
	}

	idThread::ReturnFloat( renderEnvprobe.shaderParms[ parmnum ] );
}

/*
================
EnvironmentProbe::Event_SetEnvprobeParm
================
*/
void EnvironmentProbe::Event_SetEnvprobeParm( int parmnum, float value )
{
	SetEnvprobeParm( parmnum, value );
}

/*
================
EnvironmentProbe::Event_SetEnvprobetParms
================
*/
void EnvironmentProbe::Event_SetEnvprobeParms( float parm0, float parm1, float parm2, float parm3 )
{
	SetEnvprobeParms( parm0, parm1, parm2, parm3 );
}

/*
================
EnvironmentProbe::Event_Hide
================
*/
void EnvironmentProbe::Event_Hide()
{
	Hide();
	Off();
}

/*
================
EnvironmentProbe::Event_Show
================
*/
void EnvironmentProbe::Event_Show()
{
	Show();
	On();
}

/*
================
EnvironmentProbe::Event_On
================
*/
void EnvironmentProbe::Event_On()
{
	On();
}

/*
================
EnvironmentProbe::Event_Off
================
*/
void EnvironmentProbe::Event_Off()
{
	Off();
}

/*
================
EnvironmentProbe::Event_ToggleOnOff
================
*/
void EnvironmentProbe::Event_ToggleOnOff( idEntity* activator )
{
	triggercount++;
	if( triggercount < count )
	{
		return;
	}

	// reset trigger count
	triggercount = 0;

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
EnvironmentProbe::Event_SetSoundHandles

  set the same sound def handle on all targeted lights
================
*/
/*
void EnvironmentProbe::Event_SetSoundHandles()
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
		if( targetEnt != NULL && targetEnt->IsType( EnvironmentProbe::Type ) )
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
*/

/*
================
EnvironmentProbe::Event_FadeOut
================
*/
void EnvironmentProbe::Event_FadeOut( float time )
{
	FadeOut( time );
}

/*
================
EnvironmentProbe::Event_FadeIn
================
*/
void EnvironmentProbe::Event_FadeIn( float time )
{
	FadeIn( time );
}

/*
================
EnvironmentProbe::ClientPredictionThink
================
*/
void EnvironmentProbe::ClientPredictionThink()
{
	Think();
}

/*
================
EnvironmentProbe::WriteToSnapshot
================
*/
void EnvironmentProbe::WriteToSnapshot( idBitMsg& msg ) const
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
	//msg.WriteFloat( renderEnvprobe.lightRadius[0], 5, 10 );
	//msg.WriteFloat( renderEnvprobe.lightRadius[1], 5, 10 );
	//msg.WriteFloat( renderEnvprobe.lightRadius[2], 5, 10 );

	msg.WriteLong( PackColor( idVec4( renderEnvprobe.shaderParms[SHADERPARM_RED],
									  renderEnvprobe.shaderParms[SHADERPARM_GREEN],
									  renderEnvprobe.shaderParms[SHADERPARM_BLUE],
									  renderEnvprobe.shaderParms[SHADERPARM_ALPHA] ) ) );

	msg.WriteFloat( renderEnvprobe.shaderParms[SHADERPARM_TIMESCALE], 5, 10 );
	msg.WriteLong( renderEnvprobe.shaderParms[SHADERPARM_TIMEOFFSET] );
	msg.WriteShort( renderEnvprobe.shaderParms[SHADERPARM_MODE] );

	WriteColorToSnapshot( msg );
}

/*
================
EnvironmentProbe::ReadFromSnapshot
================
*/
void EnvironmentProbe::ReadFromSnapshot( const idBitMsg& msg )
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
	//renderLight.lightRadius[0] = msg.ReadFloat( 5, 10 );
	//renderLight.lightRadius[1] = msg.ReadFloat( 5, 10 );
	//renderLight.lightRadius[2] = msg.ReadFloat( 5, 10 );

	UnpackColor( msg.ReadLong(), shaderColor );
	renderEnvprobe.shaderParms[SHADERPARM_RED] = shaderColor[0];
	renderEnvprobe.shaderParms[SHADERPARM_GREEN] = shaderColor[1];
	renderEnvprobe.shaderParms[SHADERPARM_BLUE] = shaderColor[2];
	renderEnvprobe.shaderParms[SHADERPARM_ALPHA] = shaderColor[3];

	renderEnvprobe.shaderParms[SHADERPARM_TIMESCALE] = msg.ReadFloat( 5, 10 );
	renderEnvprobe.shaderParms[SHADERPARM_TIMEOFFSET] = msg.ReadLong();
	renderEnvprobe.shaderParms[SHADERPARM_MODE] = msg.ReadShort();

	ReadColorFromSnapshot( msg );

	if( msg.HasChanged() )
	{
		if( ( currentLevel != oldCurrentLevel ) || ( previousBaseColor != nextBaseColor ) )
		{
			SetLightLevel();
		}
		else
		{
			PresentEnvprobeDefChange();
		}
	}
}

/*
================
EnvironmentProbe::ClientReceiveEvent
================
*/
/*
bool EnvironmentProbe::ClientReceiveEvent( int event, int time, const idBitMsg& msg )
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
*/