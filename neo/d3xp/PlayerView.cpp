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

// _D3XP : rename all gameLocal.time to gameLocal.slow.time for merge!

const int IMPULSE_DELAY = 150;
/*
==============
idPlayerView::idPlayerView
==============
*/
idPlayerView::idPlayerView()
{
	memset( screenBlobs, 0, sizeof( screenBlobs ) );
	memset( &view, 0, sizeof( view ) );
	player = NULL;
	tunnelMaterial = declManager->FindMaterial( "textures/decals/tunnel" );
	armorMaterial = declManager->FindMaterial( "armorViewEffect" );
	berserkMaterial = declManager->FindMaterial( "textures/decals/berserk" );
	irGogglesMaterial = declManager->FindMaterial( "textures/decals/irblend" );
	bloodSprayMaterial = declManager->FindMaterial( "textures/decals/bloodspray" );
	bfgMaterial = declManager->FindMaterial( "textures/decals/bfgvision" );
	bfgVision = false;
	dvFinishTime = 0;
	kickFinishTime = 0;
	kickAngles.Zero();
	lastDamageTime = 0.0f;
	fadeTime = 0;
	fadeRate = 0.0;
	fadeFromColor.Zero();
	fadeToColor.Zero();
	fadeColor.Zero();
	shakeAng.Zero();
	fxManager = NULL;

	if( fxManager == NULL )
	{
		fxManager = new( TAG_ENTITY ) FullscreenFXManager;
		fxManager->Initialize( this );
	}

	ClearEffects();
}

/*
==============
idPlayerView::~idPlayerView
==============
*/
idPlayerView::~idPlayerView()
{
	delete fxManager;
}
/*
==============
idPlayerView::Save
==============
*/
void idPlayerView::Save( idSaveGame* savefile ) const
{
	int i;
	const screenBlob_t* blob;

	blob = &screenBlobs[ 0 ];
	for( i = 0; i < MAX_SCREEN_BLOBS; i++, blob++ )
	{
		savefile->WriteMaterial( blob->material );
		savefile->WriteFloat( blob->x );
		savefile->WriteFloat( blob->y );
		savefile->WriteFloat( blob->w );
		savefile->WriteFloat( blob->h );
		savefile->WriteFloat( blob->s1 );
		savefile->WriteFloat( blob->t1 );
		savefile->WriteFloat( blob->s2 );
		savefile->WriteFloat( blob->t2 );
		savefile->WriteInt( blob->finishTime );
		savefile->WriteInt( blob->startFadeTime );
		savefile->WriteFloat( blob->driftAmount );
	}

	savefile->WriteInt( dvFinishTime );
	savefile->WriteInt( kickFinishTime );
	savefile->WriteAngles( kickAngles );
	savefile->WriteBool( bfgVision );

	savefile->WriteMaterial( tunnelMaterial );
	savefile->WriteMaterial( armorMaterial );
	savefile->WriteMaterial( berserkMaterial );
	savefile->WriteMaterial( irGogglesMaterial );
	savefile->WriteMaterial( bloodSprayMaterial );
	savefile->WriteMaterial( bfgMaterial );
	savefile->WriteFloat( lastDamageTime );

	savefile->WriteVec4( fadeColor );
	savefile->WriteVec4( fadeToColor );
	savefile->WriteVec4( fadeFromColor );
	savefile->WriteFloat( fadeRate );
	savefile->WriteInt( fadeTime );

	savefile->WriteAngles( shakeAng );

	savefile->WriteObject( player );
	savefile->WriteRenderView( view );

	if( fxManager )
	{
		fxManager->Save( savefile );
	}
}

/*
==============
idPlayerView::Restore
==============
*/
void idPlayerView::Restore( idRestoreGame* savefile )
{
	int i;
	screenBlob_t* blob;

	blob = &screenBlobs[ 0 ];
	for( i = 0; i < MAX_SCREEN_BLOBS; i++, blob++ )
	{
		savefile->ReadMaterial( blob->material );
		savefile->ReadFloat( blob->x );
		savefile->ReadFloat( blob->y );
		savefile->ReadFloat( blob->w );
		savefile->ReadFloat( blob->h );
		savefile->ReadFloat( blob->s1 );
		savefile->ReadFloat( blob->t1 );
		savefile->ReadFloat( blob->s2 );
		savefile->ReadFloat( blob->t2 );
		savefile->ReadInt( blob->finishTime );
		savefile->ReadInt( blob->startFadeTime );
		savefile->ReadFloat( blob->driftAmount );
	}

	savefile->ReadInt( dvFinishTime );
	savefile->ReadInt( kickFinishTime );
	savefile->ReadAngles( kickAngles );
	savefile->ReadBool( bfgVision );

	savefile->ReadMaterial( tunnelMaterial );
	savefile->ReadMaterial( armorMaterial );
	savefile->ReadMaterial( berserkMaterial );
	savefile->ReadMaterial( irGogglesMaterial );
	savefile->ReadMaterial( bloodSprayMaterial );
	savefile->ReadMaterial( bfgMaterial );
	savefile->ReadFloat( lastDamageTime );

	savefile->ReadVec4( fadeColor );
	savefile->ReadVec4( fadeToColor );
	savefile->ReadVec4( fadeFromColor );
	savefile->ReadFloat( fadeRate );
	savefile->ReadInt( fadeTime );

	savefile->ReadAngles( shakeAng );

	savefile->ReadObject( reinterpret_cast<idClass*&>( player ) );
	savefile->ReadRenderView( view );

	if( fxManager )
	{
		fxManager->Restore( savefile );
	}
}

/*
==============
idPlayerView::SetPlayerEntity
==============
*/
void idPlayerView::SetPlayerEntity( idPlayer* playerEnt )
{
	player = playerEnt;
}

/*
==============
idPlayerView::ClearEffects
==============
*/
void idPlayerView::ClearEffects()
{
	lastDamageTime = MS2SEC( gameLocal.slow.time - 99999 );

	dvFinishTime = ( gameLocal.fast.time - 99999 );
	kickFinishTime = ( gameLocal.slow.time - 99999 );

	for( int i = 0 ; i < MAX_SCREEN_BLOBS ; i++ )
	{
		screenBlobs[i].finishTime = gameLocal.fast.time;
	}

	fadeTime = 0;
	bfgVision = false;
}

/*
==============
idPlayerView::GetScreenBlob
==============
*/
screenBlob_t* idPlayerView::GetScreenBlob()
{
	screenBlob_t* oldest = &screenBlobs[0];

	for( int i = 1 ; i < MAX_SCREEN_BLOBS ; i++ )
	{
		if( screenBlobs[i].finishTime < oldest->finishTime )
		{
			oldest = &screenBlobs[i];
		}
	}
	return oldest;
}

/*
==============
idPlayerView::DamageImpulse

LocalKickDir is the direction of force in the player's coordinate system,
which will determine the head kick direction
==============
*/
void idPlayerView::DamageImpulse( idVec3 localKickDir, const idDict* damageDef )
{
	//
	// double vision effect
	//
	if( lastDamageTime > 0.0f && SEC2MS( lastDamageTime ) + IMPULSE_DELAY > gameLocal.slow.time )
	{
		// keep shotgun from obliterating the view
		return;
	}

	float dvTime = damageDef->GetFloat( "dv_time" );
	if( dvTime )
	{
		if( dvFinishTime < gameLocal.fast.time )
		{
			dvFinishTime = gameLocal.fast.time;
		}
		dvFinishTime += g_dvTime.GetFloat() * dvTime;
		// don't let it add up too much in god mode
		if( dvFinishTime > gameLocal.fast.time + 5000 )
		{
			dvFinishTime = gameLocal.fast.time + 5000;
		}
	}

	//
	// head angle kick
	//
	float kickTime = damageDef->GetFloat( "kick_time" );
	if( kickTime )
	{
		kickFinishTime = gameLocal.slow.time + g_kickTime.GetFloat() * kickTime;

		// forward / back kick will pitch view
		kickAngles[0] = localKickDir[0];

		// side kick will yaw view
		kickAngles[1] = localKickDir[1] * 0.5f;

		// up / down kick will pitch view
		kickAngles[0] += localKickDir[2];

		// roll will come from  side
		kickAngles[2] = localKickDir[1];

		float kickAmplitude = damageDef->GetFloat( "kick_amplitude" );
		if( kickAmplitude )
		{
			kickAngles *= kickAmplitude;
		}
	}

	//
	// screen blob
	//
	float blobTime = damageDef->GetFloat( "blob_time" );
	if( blobTime )
	{
		screenBlob_t*	blob = GetScreenBlob();
		blob->startFadeTime = gameLocal.fast.time;
		blob->finishTime = gameLocal.fast.time + blobTime * g_blobTime.GetFloat();

		const char* materialName = damageDef->GetString( "mtr_blob" );
		blob->material = declManager->FindMaterial( materialName );
		blob->x = damageDef->GetFloat( "blob_x" );
		blob->x += ( gameLocal.random.RandomInt() & 63 ) - 32;
		blob->y = damageDef->GetFloat( "blob_y" );
		blob->y += ( gameLocal.random.RandomInt() & 63 ) - 32;

		float scale = ( 256 + ( ( gameLocal.random.RandomInt() & 63 ) - 32 ) ) / 256.0f;
		blob->w = damageDef->GetFloat( "blob_width" ) * g_blobSize.GetFloat() * scale;
		blob->h = damageDef->GetFloat( "blob_height" ) * g_blobSize.GetFloat() * scale;
		blob->s1 = 0.0f;
		blob->t1 = 0.0f;
		blob->s2 = 1.0f;
		blob->t2 = 1.0f;
	}

	//
	// save lastDamageTime for tunnel vision accentuation
	//
	lastDamageTime = MS2SEC( gameLocal.fast.time );

}

/*
==================
idPlayerView::WeaponFireFeedback

Called when a weapon fires, generates head twitches, etc
==================
*/
void idPlayerView::WeaponFireFeedback( const idDict* weaponDef )
{
	int recoilTime = weaponDef->GetInt( "recoilTime" );
	// don't shorten a damage kick in progress
	if( recoilTime && kickFinishTime < gameLocal.slow.time )
	{
		idAngles angles;
		weaponDef->GetAngles( "recoilAngles", "5 0 0", angles );
		kickAngles = angles;
		int	finish = gameLocal.slow.time + g_kickTime.GetFloat() * recoilTime;
		kickFinishTime = finish;
	}

}

/*
===================
idPlayerView::CalculateShake
===================
*/
void idPlayerView::CalculateShake()
{
	float shakeVolume = gameSoundWorld->CurrentShakeAmplitude();
	//
	// shakeVolume should somehow be molded into an angle here
	// it should be thought of as being in the range 0.0 -> 1.0, although
	// since CurrentShakeAmplitudeForPosition() returns all the shake sounds
	// the player can hear, it can go over 1.0 too.
	//
	shakeAng[0] = gameLocal.random.CRandomFloat() * shakeVolume;
	shakeAng[1] = gameLocal.random.CRandomFloat() * shakeVolume;
	shakeAng[2] = gameLocal.random.CRandomFloat() * shakeVolume;
}

/*
===================
idPlayerView::ShakeAxis
===================
*/
idMat3 idPlayerView::ShakeAxis() const
{
	return shakeAng.ToMat3();
}

/*
===================
idPlayerView::AngleOffset

  kickVector, a world space direction that the attack should
===================
*/
idAngles idPlayerView::AngleOffset() const
{
	idAngles ang( 0.0f, 0.0f, 0.0f );

	if( gameLocal.slow.time < kickFinishTime )
	{
		float offset = kickFinishTime - gameLocal.slow.time;

		ang = kickAngles * offset * offset * g_kickAmplitude.GetFloat();

		for( int i = 0 ; i < 3 ; i++ )
		{
			if( ang[i] > 70.0f )
			{
				ang[i] = 70.0f;
			}
			else if( ang[i] < -70.0f )
			{
				ang[i] = -70.0f;
			}
		}
	}
	return ang;
}

/*
==================
idPlayerView::SingleView
==================
*/
void idPlayerView::SingleView( const renderView_t* view, idMenuHandler_HUD* hudManager )
{

	// normal rendering
	if( !view )
	{
		return;
	}

	// place the sound origin for the player
	gameSoundWorld->PlaceListener( view->vieworg, view->viewaxis, player->entityNumber + 1 );

	// if the objective system is up, don't do normal drawing
	if( player->objectiveSystemOpen )
	{
		if( player->pdaMenu != NULL )
		{
			player->pdaMenu->Update();
		}
		return;
	}

	// hack the shake in at the very last moment, so it can't cause any consistency problems
	renderView_t hackedView = *view;
	hackedView.viewaxis = hackedView.viewaxis * ShakeAxis();

	if( gameLocal.portalSkyEnt.GetEntity() && gameLocal.IsPortalSkyAcive() && g_enablePortalSky.GetBool() )
	{
		renderView_t portalView = hackedView;
		portalView.vieworg = gameLocal.portalSkyEnt.GetEntity()->GetPhysics()->GetOrigin();
		gameRenderWorld->RenderScene( &portalView );
		renderSystem->CaptureRenderToImage( "_currentRender" );

		hackedView.forceUpdate = true;				// FIX: for smoke particles not drawing when portalSky present
	}

	// process the frame
	fxManager->Process( &hackedView );

	if( !hudManager )
	{
		return;
	}

	// draw screen blobs
	if( !pm_thirdPerson.GetBool() && !g_skipViewEffects.GetBool() )
	{
		if( !player->spectating )
		{
			for( int i = 0 ; i < MAX_SCREEN_BLOBS ; i++ )
			{
				screenBlob_t*	blob = &screenBlobs[i];
				if( blob->finishTime <= gameLocal.fast.time )
				{
					continue;
				}

				blob->y += blob->driftAmount;

				float	fade = ( float )( blob->finishTime - gameLocal.fast.time ) / ( blob->finishTime - blob->startFadeTime );
				if( fade > 1.0f )
				{
					fade = 1.0f;
				}
				if( fade )
				{
					renderSystem->SetColor4( 1.0f, 1.0f, 1.0f, fade );
					renderSystem->DrawStretchPic( blob->x, blob->y, blob->w, blob->h, blob->s1, blob->t1, blob->s2, blob->t2, blob->material );
				}
			}
		}
		player->DrawHUD( hudManager );

		if( player->spectating )
		{
			return;
		}

		// armor impulse feedback
		float armorPulse = ( gameLocal.fast.time - player->lastArmorPulse ) / 250.0f;

		if( armorPulse > 0.0f && armorPulse < 1.0f )
		{
			renderSystem->SetColor4( 1.0f, 1.0f, 1.0f, 1.0f - armorPulse );
			renderSystem->DrawStretchPic( 0.0f, 0.0f, renderSystem->GetVirtualWidth(), renderSystem->GetVirtualHeight(), 0.0f, 0.0f, 1.0f, 1.0f, armorMaterial );
		}


		// tunnel vision
		/*
		RB: disabled tunnel vision because the renderer converts colors set by SetColor4 to bytes which are clamped to [0,255]
		so materials that want to access float values greater than 1 with parm0 - parm3 are always broken

		float health = 0.0f;
		if( g_testHealthVision.GetFloat() != 0.0f )
		{
			health = g_testHealthVision.GetFloat();
		}
		else
		{
			health = player->health;
		}
		float alpha = health / 100.0f;
		if( alpha < 0.0f )
		{
			alpha = 0.0f;
		}
		if( alpha > 1.0f )
		{
			alpha = 1.0f;
		}

		if( alpha < 1.0f )
		{
			renderSystem->SetColor4( ( player->health <= 0.0f ) ? MS2SEC( gameLocal.slow.time ) : lastDamageTime, 1.0f, 1.0f, ( player->health <= 0.0f ) ? 0.0f : alpha );
			renderSystem->DrawStretchPic( 0.0f, 0.0f, renderSystem->GetVirtualWidth(), renderSystem->GetVirtualHeight(), 0.0f, 0.0f, 1.0f, 1.0f, tunnelMaterial );
		}
		RB end
		*/

		if( bfgVision )
		{
			renderSystem->SetColor4( 1.0f, 1.0f, 1.0f, 1.0f );
			renderSystem->DrawStretchPic( 0.0f, 0.0f, renderSystem->GetVirtualWidth(), renderSystem->GetVirtualHeight(), 0.0f, 0.0f, 1.0f, 1.0f, bfgMaterial );
		}

	}

	// test a single material drawn over everything
	if( g_testPostProcess.GetString()[0] )
	{
		const idMaterial* mtr = declManager->FindMaterial( g_testPostProcess.GetString(), false );
		if( !mtr )
		{
			common->Printf( "Material not found.\n" );
			g_testPostProcess.SetString( "" );
		}
		else
		{
			renderSystem->SetColor4( 1.0f, 1.0f, 1.0f, 1.0f );
			renderSystem->DrawStretchPic( 0.0f, 0.0f, renderSystem->GetVirtualWidth(), renderSystem->GetVirtualHeight(), 0.0f, 0.0f, 1.0f, 1.0f, mtr );
		}
	}
}


/*
=================
idPlayerView::Flash

flashes the player view with the given color
=================
*/
void idPlayerView::Flash( idVec4 color, int time )
{
	Fade( idVec4( 0.0f, 0.0f, 0.0f, 0.0f ), time );
	fadeFromColor = colorWhite;
}

/*
=================
idPlayerView::Fade

used for level transition fades
assumes: color.w is 0 or 1
=================
*/
void idPlayerView::Fade( idVec4 color, int time )
{
	SetTimeState ts( player->timeGroup );

	if( !fadeTime )
	{
		fadeFromColor.Set( 0.0f, 0.0f, 0.0f, 1.0f - color[ 3 ] );
	}
	else
	{
		fadeFromColor = fadeColor;
	}
	fadeToColor = color;

	if( time <= 0 )
	{
		fadeRate = 0;
		time = 0;
		fadeColor = fadeToColor;
	}
	else
	{
		fadeRate = 1.0f / ( float )time;
	}

	if( gameLocal.realClientTime == 0 && time == 0 )
	{
		fadeTime = 1;
	}
	else
	{
		fadeTime = gameLocal.realClientTime + time;
	}
}

/*
=================
idPlayerView::ScreenFade
=================
*/
void idPlayerView::ScreenFade()
{
	if( !fadeTime )
	{
		return;
	}

	SetTimeState ts( player->timeGroup );

	int msec = fadeTime - gameLocal.realClientTime;

	if( msec <= 0 )
	{
		fadeColor = fadeToColor;
		if( fadeColor[ 3 ] == 0.0f )
		{
			fadeTime = 0;
		}
	}
	else
	{
		float t = ( float )msec * fadeRate;
		fadeColor = fadeFromColor * t + fadeToColor * ( 1.0f - t );
	}

	if( fadeColor[ 3 ] != 0.0f )
	{
		renderSystem->SetColor4( fadeColor[ 0 ], fadeColor[ 1 ], fadeColor[ 2 ], fadeColor[ 3 ] );
		renderSystem->DrawStretchPic( 0.0f, 0.0f, renderSystem->GetVirtualWidth(), renderSystem->GetVirtualHeight(), 0.0f, 0.0f, 1.0f, 1.0f, declManager->FindMaterial( "_white" ) );
	}
}

idCVar	stereoRender_interOccularCentimeters( "stereoRender_interOccularCentimeters", "3.0", CVAR_ARCHIVE | CVAR_RENDERER, "Distance between eyes" );
idCVar	stereoRender_convergence( "stereoRender_convergence", "6", CVAR_RENDERER, "0 = head mounted display, otherwise world units to convergence plane" );

extern	idCVar stereoRender_screenSeparation;	// screen units from center to eyes
extern	idCVar stereoRender_swapEyes;

// In a head mounted display with separate displays for each eye,
// screen separation will be zero and world separation will be the eye distance.
struct stereoDistances_t
{
	// Offset to projection matrix, positive one eye, negative the other.
	// Total distance is twice this, so 0.05 would give a 10% of screen width
	// separation for objects at infinity.
	float	screenSeparation;

	// Game world units from one eye to the centerline.
	// Total distance is twice this.
	float	worldSeparation;
};

float CentimetersToInches( const float cm )
{
	return cm / 2.54f;
}

float CentimetersToWorldUnits( const float cm )
{
	// In Doom 3, one world unit == one inch
	return CentimetersToInches( cm );
}

float	CalculateWorldSeparation(
	const float screenSeparation,
	const float convergenceDistance,
	const float fov_x_degrees )
{

	const float fovRadians = DEG2RAD( fov_x_degrees );
	const float screen = tan( fovRadians * 0.5f ) * fabs( screenSeparation );
	const float worldSeparation = screen * convergenceDistance / 0.5f;

	return worldSeparation;
}

stereoDistances_t	CaclulateStereoDistances(
	const float	interOcularCentimeters,		// distance between two eyes, typically 6.0 - 7.0
	const float screenWidthCentimeters,		// read from operating system
	const float convergenceWorldUnits,		// pass 0 for head mounted display mode
	const float	fov_x_degrees )  			// edge to edge horizontal field of view, typically 60 - 90
{

	stereoDistances_t	dists = {};

	if( convergenceWorldUnits == 0.0f )
	{
		// head mounted display mode
		dists.worldSeparation = CentimetersToInches( interOcularCentimeters * 0.5 );
		dists.screenSeparation = 0.0f;
		return dists;
	}

	// 3DTV mode
	dists.screenSeparation = 0.5f * interOcularCentimeters / screenWidthCentimeters;
	dists.worldSeparation = CalculateWorldSeparation( dists.screenSeparation, convergenceWorldUnits, fov_x_degrees );

	return dists;
}

float	GetScreenSeparationForGuis()
{
	const stereoDistances_t dists = CaclulateStereoDistances(
										stereoRender_interOccularCentimeters.GetFloat(),
										renderSystem->GetPhysicalScreenWidthInCentimeters(),
										stereoRender_convergence.GetFloat(),
										80.0f /* fov */ );

	return dists.screenSeparation;
}

/*
===================
idPlayerView::EmitStereoEyeView
===================
*/
void idPlayerView::EmitStereoEyeView( const int eye, idMenuHandler_HUD* hudManager )
{
	renderView_t* view = player->GetRenderView();
	if( view == NULL )
	{
		return;
	}

	renderView_t eyeView = *view;

	const stereoDistances_t dists = CaclulateStereoDistances(
										stereoRender_interOccularCentimeters.GetFloat(),
										renderSystem->GetPhysicalScreenWidthInCentimeters(),
										stereoRender_convergence.GetFloat(),
										view->fov_x );

	eyeView.vieworg += eye * dists.worldSeparation * eyeView.viewaxis[1];

	eyeView.viewEyeBuffer = stereoRender_swapEyes.GetBool() ? eye : -eye;
	eyeView.stereoScreenSeparation = eye * dists.screenSeparation;

	SingleView( &eyeView, hudManager );
}

/*
===================
IsGameStereoRendered

The crosshair is swapped for a laser sight in stereo rendering
===================
*/
bool	IsGameStereoRendered()
{
	if( renderSystem->GetStereo3DMode() != STEREO3D_OFF )
	{
		return true;
	}
	return false;
}

int EyeForHalfRateFrame( const int frameCount )
{
	return ( renderSystem->GetFrameCount() & 1 ) ? -1 : 1;
}

/*
===================
idPlayerView::RenderPlayerView
===================
*/
void idPlayerView::RenderPlayerView( idMenuHandler_HUD* hudManager )
{
	const renderView_t* view = player->GetRenderView();
	if( renderSystem->GetStereo3DMode() != STEREO3D_OFF )
	{
		// render both eye views each frame on the PC
		for( int eye = 1 ; eye >= -1 ; eye -= 2 )
		{
			EmitStereoEyeView( eye, hudManager );
		}
	}
	else
	{
		SingleView( view, hudManager );
	}
	ScreenFade();
}

/*
===================
idPlayerView::WarpVision
===================
*/
int idPlayerView::AddWarp( idVec3 worldOrigin, float centerx, float centery, float initialRadius, float durationMsec )
{
	FullscreenFX_Warp* fx = ( FullscreenFX_Warp* )( fxManager->FindFX( "warp" ) );

	if( fx )
	{
		fx->EnableGrabber( true );
		return 1;
	}

	return 1;
}

void idPlayerView::FreeWarp( int id )
{
	FullscreenFX_Warp* fx = ( FullscreenFX_Warp* )( fxManager->FindFX( "warp" ) );

	if( fx )
	{
		fx->EnableGrabber( false );
		return;
	}
}





/*
==================
FxFader::FxFader
==================
*/
FxFader::FxFader()
{
	time = 0;
	state = FX_STATE_OFF;
	alpha = 0;
	msec = 1000;
}

/*
==================
FxFader::SetTriggerState
==================
*/
bool FxFader::SetTriggerState( bool active )
{

	// handle on/off states
	if( active && state == FX_STATE_OFF )
	{
		state = FX_STATE_RAMPUP;
		time = gameLocal.slow.time + msec;
	}
	else if( !active && state == FX_STATE_ON )
	{
		state = FX_STATE_RAMPDOWN;
		time = gameLocal.slow.time + msec;
	}

	// handle rampup/rampdown states
	if( state == FX_STATE_RAMPUP )
	{
		if( gameLocal.slow.time >= time )
		{
			state = FX_STATE_ON;
		}
	}
	else if( state == FX_STATE_RAMPDOWN )
	{
		if( gameLocal.slow.time >= time )
		{
			state = FX_STATE_OFF;
		}
	}

	// compute alpha
	switch( state )
	{
		case FX_STATE_ON:
			alpha = 1;
			break;
		case FX_STATE_OFF:
			alpha = 0;
			break;
		case FX_STATE_RAMPUP:
			alpha = 1 - ( float )( time - gameLocal.slow.time ) / msec;
			break;
		case FX_STATE_RAMPDOWN:
			alpha = ( float )( time - gameLocal.slow.time ) / msec;
			break;
	}

	if( alpha > 0 )
	{
		return true;
	}
	else
	{
		return false;
	}
}

/*
==================
FxFader::Save
==================
*/
void FxFader::Save( idSaveGame* savefile )
{
	savefile->WriteInt( time );
	savefile->WriteInt( state );
	savefile->WriteFloat( alpha );
	savefile->WriteInt( msec );
}

/*
==================
FxFader::Restore
==================
*/
void FxFader::Restore( idRestoreGame* savefile )
{
	savefile->ReadInt( time );
	savefile->ReadInt( state );
	savefile->ReadFloat( alpha );
	savefile->ReadInt( msec );
}





/*
==================
FullscreenFX_Helltime::Save
==================
*/
void FullscreenFX::Save( idSaveGame* savefile )
{
	fader.Save( savefile );
}

/*
==================
FullscreenFX_Helltime::Restore
==================
*/
void FullscreenFX::Restore( idRestoreGame* savefile )
{
	fader.Restore( savefile );
}


/*
==================
FullscreenFX_Helltime::Initialize
==================
*/
void FullscreenFX_Helltime::Initialize()
{
	initMaterial = declManager->FindMaterial( "textures/d3bfg/bloodorb/init" );
	drawMaterial = declManager->FindMaterial( "textures/d3bfg/bloodorb/draw" );

	captureMaterials[0] = declManager->FindMaterial( "textures/d3bfg/bloodorb1/capture" );
	captureMaterials[1] = declManager->FindMaterial( "textures/d3bfg/bloodorb2/capture" );
	captureMaterials[2] = declManager->FindMaterial( "textures/d3bfg/bloodorb3/capture" );

	clearAccumBuffer = true;
}

/*
==================
FullscreenFX_Helltime::DetermineLevel
==================
*/
int FullscreenFX_Helltime::DetermineLevel()
{
	int testfx = g_testHelltimeFX.GetInteger();

	// for testing purposes
	if( testfx >= 0 && testfx < 3 )
	{
		return testfx;
	}

	idPlayer* player = fxman->GetPlayer();

	if( player != NULL &&  player->PowerUpActive( INVULNERABILITY ) )
	{
		return 2;
	}
	else if( player != NULL && player->PowerUpActive( BERSERK ) )
	{
		return 1;
	}
	else if( player != NULL && player->PowerUpActive( HELLTIME ) )
	{
		return 0;
	}

	return -1;
}

/*
==================
FullscreenFX_Helltime::Active
==================
*/
bool FullscreenFX_Helltime::Active()
{

	if( gameLocal.inCinematic || common->IsMultiplayer() )
	{
		return false;
	}

	if( DetermineLevel() >= 0 )
	{
		return true;
	}
	else
	{
		// latch the clear flag
		if( fader.GetAlpha() == 0 )
		{
			clearAccumBuffer = true;
		}
	}

	return false;
}

/*
==================
FullscreenFX_Helltime::AccumPass
==================
*/
void FullscreenFX_Helltime::AccumPass( const renderView_t* view )
{

	int level = DetermineLevel();

	// for testing
	if( level < 0 || level > 2 )
	{
		level = 0;
	}

	renderSystem->SetColor4( 1.0f, 1.0f, 1.0f, 1.0f );

	float t0 = 1.0f;
	float t1 = 0.0f;

	// capture pass
	if( clearAccumBuffer )
	{
		clearAccumBuffer = false;
		renderSystem->DrawStretchPic( 0.0f, 0.0f, renderSystem->GetVirtualWidth(), renderSystem->GetVirtualHeight(), 0.0f, t0, 1.0f, t1, initMaterial );
	}
	else
	{
		renderSystem->DrawStretchPic( 0.0f, 0.0f, renderSystem->GetVirtualWidth(), renderSystem->GetVirtualHeight(), 0.0f, t0, 1.0f, t1, captureMaterials[level] );
	}
}

/*
==================
FullscreenFX_Helltime::HighQuality
==================
*/
void FullscreenFX_Helltime::HighQuality()
{
	float t0 = 1.0f;
	float t1 = 0.0f;

	renderSystem->SetColor4( 1.0f, 1.0f, 1.0f, 1.0f );
	renderSystem->DrawStretchPic( 0.0f, 0.0f, renderSystem->GetVirtualWidth(), renderSystem->GetVirtualHeight(), 0.0f, t0, 1.0f, t1, drawMaterial );
}

/*
==================
FullscreenFX_Helltime::Restore
==================
*/
void FullscreenFX_Helltime::Restore( idRestoreGame* savefile )
{
	FullscreenFX::Restore( savefile );

	// latch the clear flag
	clearAccumBuffer = true;
}

/*
==================
FullscreenFX_Multiplayer::Initialize
==================
*/
void FullscreenFX_Multiplayer::Initialize()
{
	initMaterial = declManager->FindMaterial( "textures/d3bfg/multiplayer/init" );
	captureMaterial = declManager->FindMaterial( "textures/d3bfg/multiplayer/capture" );
	drawMaterial = declManager->FindMaterial( "textures/d3bfg/bloodorb/draw" );
	clearAccumBuffer	= true;
}

/*
==================
FullscreenFX_Multiplayer::DetermineLevel
==================
*/
int FullscreenFX_Multiplayer::DetermineLevel()
{
	int testfx = g_testMultiplayerFX.GetInteger();

	// for testing purposes
	if( testfx >= 0 && testfx < 3 )
	{
		return testfx;
	}

	idPlayer* player = fxman->GetPlayer();

	if( player != NULL && player->PowerUpActive( INVULNERABILITY ) )
	{
		return 2;
	}
	//else if ( player->PowerUpActive( HASTE ) ) {
	//	return 1;
	//}
	else if( player != NULL && player->PowerUpActive( BERSERK ) )
	{
		return 0;
	}

	return -1;
}

/*
==================
FullscreenFX_Multiplayer::Active
==================
*/
bool FullscreenFX_Multiplayer::Active()
{

	if( !common->IsMultiplayer() && g_testMultiplayerFX.GetInteger() == -1 )
	{
		return false;
	}

	if( DetermineLevel() >= 0 )
	{
		return true;
	}
	else
	{
		// latch the clear flag
		if( fader.GetAlpha() == 0 )
		{
			clearAccumBuffer = true;
		}
	}

	return false;
}

/*
==================
FullscreenFX_Multiplayer::AccumPass
==================
*/
void FullscreenFX_Multiplayer::AccumPass( const renderView_t* view )
{
	renderSystem->SetColor4( 1.0f, 1.0f, 1.0f, 1.0f );

	float t0 = 1.0f;
	float t1 = 0.0f;

	// capture pass
	if( clearAccumBuffer )
	{
		clearAccumBuffer = false;
		renderSystem->DrawStretchPic( 0.0f, 0.0f, renderSystem->GetVirtualWidth(), renderSystem->GetVirtualHeight(), 0.0f, t0, 1.0f, t1, initMaterial );
	}
	else
	{
		renderSystem->DrawStretchPic( 0.0f, 0.0f, renderSystem->GetVirtualWidth(), renderSystem->GetVirtualHeight(), 0.0f, t0, 1.0f, t1, captureMaterial );
	}
}

/*
==================
FullscreenFX_Multiplayer::HighQuality
==================
*/
void FullscreenFX_Multiplayer::HighQuality()
{
	float t0 = 1.0f;
	float t1 = 0.0f;

	renderSystem->SetColor4( 1.0f, 1.0f, 1.0f, 1.0f );
	renderSystem->DrawStretchPic( 0.0f, 0.0f, renderSystem->GetVirtualWidth(), renderSystem->GetVirtualHeight(), 0.0f, t0, 1.0f, t1, drawMaterial );
}

/*
==================
FullscreenFX_Multiplayer::Restore
==================
*/
void FullscreenFX_Multiplayer::Restore( idRestoreGame* savefile )
{
	FullscreenFX::Restore( savefile );

	// latch the clear flag
	clearAccumBuffer = true;
}





/*
==================
FullscreenFX_Warp::Initialize
==================
*/
void FullscreenFX_Warp::Initialize()
{
	material = declManager->FindMaterial( "textures/d3bfg/warp" );
	grabberEnabled = false;
	startWarpTime = 0;
}

/*
==================
FullscreenFX_Warp::Active
==================
*/
bool FullscreenFX_Warp::Active()
{
	if( grabberEnabled )
	{
		return true;
	}

	return false;
}

/*
==================
FullscreenFX_Warp::Save
==================
*/
void FullscreenFX_Warp::Save( idSaveGame* savefile )
{
	FullscreenFX::Save( savefile );

	savefile->WriteBool( grabberEnabled );
	savefile->WriteInt( startWarpTime );
}

/*
==================
FullscreenFX_Warp::Restore
==================
*/
void FullscreenFX_Warp::Restore( idRestoreGame* savefile )
{
	FullscreenFX::Restore( savefile );

	savefile->ReadBool( grabberEnabled );
	savefile->ReadInt( startWarpTime );
}

/*
==================
FullscreenFX_Warp::DrawWarp
==================
*/
void FullscreenFX_Warp::DrawWarp( WarpPolygon_t wp, float interp )
{
	idVec4 mid1_uv, mid2_uv;
	idVec4 mid1, mid2;
	idVec2 drawPts[6];
	WarpPolygon_t trans;

	trans = wp;

	// compute mid points
	mid1 = trans.outer1 * ( interp ) + trans.center * ( 1 - interp );
	mid2 = trans.outer2 * ( interp ) + trans.center * ( 1 - interp );
	mid1_uv = trans.outer1 * ( 0.5 ) + trans.center * ( 1 - 0.5 );
	mid2_uv = trans.outer2 * ( 0.5 ) + trans.center * ( 1 - 0.5 );

	// draw [outer1, mid2, mid1]
	drawPts[0].Set( trans.outer1.x, trans.outer1.y );
	drawPts[1].Set( mid2.x, mid2.y );
	drawPts[2].Set( mid1.x, mid1.y );
	drawPts[3].Set( trans.outer1.z, trans.outer1.w );
	drawPts[4].Set( mid2_uv.z, mid2_uv.w );
	drawPts[5].Set( mid1_uv.z, mid1_uv.w );
	renderSystem->DrawStretchTri( drawPts[0], drawPts[1], drawPts[2], drawPts[3], drawPts[4], drawPts[5], material );

	// draw [outer1, outer2, mid2]
	drawPts[0].Set( trans.outer1.x, trans.outer1.y );
	drawPts[1].Set( trans.outer2.x, trans.outer2.y );
	drawPts[2].Set( mid2.x, mid2.y );
	drawPts[3].Set( trans.outer1.z, trans.outer1.w );
	drawPts[4].Set( trans.outer2.z, trans.outer2.w );
	drawPts[5].Set( mid2_uv.z, mid2_uv.w );
	renderSystem->DrawStretchTri( drawPts[0], drawPts[1], drawPts[2], drawPts[3], drawPts[4], drawPts[5], material );

	// draw [mid1, mid2, center]
	drawPts[0].Set( mid1.x, mid1.y );
	drawPts[1].Set( mid2.x, mid2.y );
	drawPts[2].Set( trans.center.x, trans.center.y );
	drawPts[3].Set( mid1_uv.z, mid1_uv.w );
	drawPts[4].Set( mid2_uv.z, mid2_uv.w );
	drawPts[5].Set( trans.center.z, trans.center.w );
	renderSystem->DrawStretchTri( drawPts[0], drawPts[1], drawPts[2], drawPts[3], drawPts[4], drawPts[5], material );
}

/*
==================
FullscreenFX_Warp::HighQuality
==================
*/
void FullscreenFX_Warp::HighQuality()
{
	float x1, y1, x2, y2, radius, interp;
	idVec2 center;
	int STEP = 9;
	renderSystem->SetColor4( 1.0f, 1.0f, 1.0f, 1.0f );

	interp = ( idMath::Sin( ( float )( gameLocal.slow.time - startWarpTime ) / 1000 ) + 1 ) / 2.f;
	interp = 0.7 * ( 1 - interp ) + 0.3 * ( interp );

	// draw the warps
	center.x = renderSystem->GetVirtualWidth() / 2.0f;
	center.y = renderSystem->GetVirtualHeight() / 2.0f;
	radius = 200;

	for( float i = 0; i < 360; i += STEP )
	{
		// compute the values
		x1 = idMath::Sin( DEG2RAD( i ) );
		y1 = idMath::Cos( DEG2RAD( i ) );

		x2 = idMath::Sin( DEG2RAD( i + STEP ) );
		y2 = idMath::Cos( DEG2RAD( i + STEP ) );

		// add warp polygon
		WarpPolygon_t p;

		p.outer1.x = center.x + x1 * radius;
		p.outer1.y = center.y + y1 * radius;
		p.outer1.z = p.outer1.x / ( float )renderSystem->GetVirtualWidth();
		p.outer1.w = 1 - ( p.outer1.y / ( float )renderSystem->GetVirtualHeight() );

		p.outer2.x = center.x + x2 * radius;
		p.outer2.y = center.y + y2 * radius;
		p.outer2.z = p.outer2.x / ( float )renderSystem->GetVirtualWidth();
		p.outer2.w = 1 - ( p.outer2.y / ( float )renderSystem->GetVirtualHeight() );

		p.center.x = center.x;
		p.center.y = center.y;
		p.center.z = p.center.x / ( float )renderSystem->GetVirtualWidth();
		p.center.w = 1 - ( p.center.y / ( float )renderSystem->GetVirtualHeight() );

		// draw it
		DrawWarp( p, interp );
	}
}





/*
==================
FullscreenFX_EnviroSuit::Initialize
==================
*/
void FullscreenFX_EnviroSuit::Initialize()
{
	material = declManager->FindMaterial( "textures/d3bfg/enviro_suit" );
}

/*
==================
FullscreenFX_EnviroSuit::Active
==================
*/
bool FullscreenFX_EnviroSuit::Active()
{
	idPlayer* player = fxman->GetPlayer();

	if( player != NULL && player->PowerUpActive( ENVIROSUIT ) )
	{
		return true;
	}

	return false;
}

/*
==================
FullscreenFX_EnviroSuit::HighQuality
==================
*/
void FullscreenFX_EnviroSuit::HighQuality()
{
	renderSystem->SetColor4( 1.0f, 1.0f, 1.0f, 1.0f );
	float s0 = 0.0f;
	float t0 = 1.0f;
	float s1 = 1.0f;
	float t1 = 0.0f;
	renderSystem->DrawStretchPic( 0.0f, 0.0f, renderSystem->GetVirtualWidth(), renderSystem->GetVirtualHeight(), s0, t0, s1, t1, material );
}

/*
==================
FullscreenFX_DoubleVision::Initialize
==================
*/
void FullscreenFX_DoubleVision::Initialize()
{
	material = declManager->FindMaterial( "textures/d3bfg/doubleVision" );
}

/*
==================
FullscreenFX_DoubleVision::Active
==================
*/
bool FullscreenFX_DoubleVision::Active()
{

	if( gameLocal.fast.time < fxman->GetPlayerView()->dvFinishTime )
	{
		return true;
	}

	return false;
}

/*
==================
FullscreenFX_DoubleVision::HighQuality
==================
*/
void FullscreenFX_DoubleVision::HighQuality()
{
	int offset = fxman->GetPlayerView()->dvFinishTime - gameLocal.fast.time;
	float scale = offset * g_dvAmplitude.GetFloat();

	// for testing purposes
	if( !Active() )
	{
		static int test = 0;
		if( test > 312 )
		{
			test = 0;
		}

		offset = test++;
		scale = offset * g_dvAmplitude.GetFloat();
	}

	idPlayer* player = fxman->GetPlayer();

	if( player == NULL )
	{
		return;
	}

	offset *= 2;		// crutch up for higher res

	// set the scale and shift
	if( scale > 0.5f )
	{
		scale = 0.5f;
	}
	float shift = scale * sin( sqrtf( ( float )offset ) * g_dvFrequency.GetFloat() );
	shift = fabs( shift );

	// carry red tint if in berserk mode
	idVec4 color( 1.0f, 1.0f, 1.0f, 1.0f );
	if( gameLocal.fast.time < player->inventory.powerupEndTime[ BERSERK ] )
	{
		color.y = 0.0f;
		color.z = 0.0f;
	}

	if( ( !common->IsMultiplayer() && gameLocal.fast.time < player->inventory.powerupEndTime[ HELLTIME ] ) || gameLocal.fast.time < player->inventory.powerupEndTime[ INVULNERABILITY ] )
	{
		color.y = 0.0f;
		color.z = 0.0f;
	}

	// uv coordinates
	float s0 = shift;
	float t0 = 1.0f;
	float s1 = 1.0f;
	float t1 = 0.0f;


	renderSystem->SetColor4( color.x, color.y, color.z, 1.0f );
	renderSystem->DrawStretchPic( 0.0f, 0.0f, renderSystem->GetVirtualWidth(), renderSystem->GetVirtualHeight(), s0, t0, s1, t1, material );

	renderSystem->SetColor4( color.x, color.y, color.z, 0.5f );
	s0 = 0.0f;
	t0 = 1.0f;
	s1 = ( 1.0 - shift );
	t1 = 0.0f;

	renderSystem->DrawStretchPic( 0.0f, 0.0f, renderSystem->GetVirtualWidth(), renderSystem->GetVirtualHeight(), s0, t0, s1, t1, material );
}

/*
==================
FullscreenFX_InfluenceVision::Initialize
==================
*/
void FullscreenFX_InfluenceVision::Initialize()
{

}

/*
==================
FullscreenFX_InfluenceVision::Active
==================
*/
bool FullscreenFX_InfluenceVision::Active()
{
	idPlayer* player = fxman->GetPlayer();

	if( player != NULL && ( player->GetInfluenceMaterial() || player->GetInfluenceEntity() ) )
	{
		return true;
	}

	return false;
}

/*
==================
FullscreenFX_InfluenceVision::HighQuality
==================
*/
void FullscreenFX_InfluenceVision::HighQuality()
{
	float distance = 0.0f;
	float pct = 1.0f;
	idPlayer* player = fxman->GetPlayer();

	if( player == NULL )
	{
		return;
	}

	if( player->GetInfluenceEntity() )
	{
		distance = ( player->GetInfluenceEntity()->GetPhysics()->GetOrigin() - player->GetPhysics()->GetOrigin() ).Length();
		if( player->GetInfluenceRadius() != 0.0f && distance < player->GetInfluenceRadius() )
		{
			pct = distance / player->GetInfluenceRadius();
			pct = 1.0f - idMath::ClampFloat( 0.0f, 1.0f, pct );
		}
	}

	if( player->GetInfluenceMaterial() )
	{
		renderSystem->SetColor4( 1.0f, 1.0f, 1.0f, pct );
		renderSystem->DrawStretchPic( 0.0f, 0.0f, renderSystem->GetVirtualWidth(), renderSystem->GetVirtualHeight(), 0.0f, 0.0f, 1.0f, 1.0f, player->GetInfluenceMaterial() );
	}
	else if( player->GetInfluenceEntity() == NULL )
	{
		return;
	}
	else
	{
//		int offset =  25 + sinf( gameLocal.slow.time );
//		DoubleVision( hud, view, pct * offset );
	}
}




/*
==================
FullscreenFX_Bloom::Initialize
==================
*/
void FullscreenFX_Bloom::Initialize()
{
	drawMaterial		= declManager->FindMaterial( "textures/d3bfg/bloom2/draw" );
	initMaterial		= declManager->FindMaterial( "textures/d3bfg/bloom2/init" );

	currentIntensity	= 0;
	targetIntensity		= 0;
}

/*
==================
FullscreenFX_Bloom::Active
==================
*/
bool FullscreenFX_Bloom::Active()
{
	idPlayer* player = fxman->GetPlayer();

	if( player != NULL && player->bloomEnabled )
	{
		return true;
	}

	return false;
}

/*
==================
FullscreenFX_Bloom::HighQuality
==================
*/
void FullscreenFX_Bloom::HighQuality()
{
	float shift = 1;
	idPlayer* player = fxman->GetPlayer();
	renderSystem->SetColor4( 1.0f, 1.0f, 1.0f, 1.0f );

	// if intensity value is different, start the blend
	targetIntensity = g_testBloomIntensity.GetFloat();

	if( player != NULL && player->bloomEnabled )
	{
		targetIntensity = player->bloomIntensity;
	}

	float delta = targetIntensity - currentIntensity;
	float step = 0.001f;

	if( step < fabs( delta ) )
	{
		if( delta < 0 )
		{
			step = -step;
		}

		currentIntensity += step;
	}

	// draw the blends
	int num = g_testBloomNumPasses.GetInteger();

	for( int i = 0; i < num; i++ )
	{
		float s1 = 0.0f, t1 = 0.0f, s2 = 1.0f, t2 = 1.0f;
		float alpha;

		// do the center scale
		s1 -= 0.5;
		s1 *= shift;
		s1 += 0.5;

		t1 -= 0.5;
		t1 *= shift;
		t1 += 0.5;

		s2 -= 0.5;
		s2 *= shift;
		s2 += 0.5;

		t2 -= 0.5;
		t2 *= shift;
		t2 += 0.5;

		// draw it
		if( num == 1 )
		{
			alpha = 1;
		}
		else
		{
			alpha = 1 - ( float )i / ( num - 1 );
		}


		float yScale = 1.0f;

		renderSystem->SetColor4( alpha, alpha, alpha, 1 );
		renderSystem->DrawStretchPic( 0.0f, 0.0f, renderSystem->GetVirtualWidth(), renderSystem->GetVirtualHeight(), s1, t2 * yScale, s2, t1 * yScale, drawMaterial );

		shift += currentIntensity;
	}
}

/*
==================
FullscreenFX_Bloom::Save
==================
*/
void FullscreenFX_Bloom::Save( idSaveGame* savefile )
{
	FullscreenFX::Save( savefile );
	savefile->WriteFloat( currentIntensity );
	savefile->WriteFloat( targetIntensity );
}

/*
==================
FullscreenFX_Bloom::Restore
==================
*/
void FullscreenFX_Bloom::Restore( idRestoreGame* savefile )
{
	FullscreenFX::Restore( savefile );
	savefile->ReadFloat( currentIntensity );
	savefile->ReadFloat( targetIntensity );
}






/*
==================
FullscreenFXManager::FullscreenFXManager
==================
*/
FullscreenFXManager::FullscreenFXManager()
{
	playerView = NULL;
	blendBackMaterial = NULL;
}

/*
==================
FullscreenFXManager::~FullscreenFXManager
==================
*/
FullscreenFXManager::~FullscreenFXManager()
{
	fx.DeleteContents();
}

/*
==================
FullscreenFXManager::FindFX
==================
*/
FullscreenFX* FullscreenFXManager::FindFX( idStr name )
{
	for( int i = 0; i < fx.Num(); i++ )
	{
		if( fx[i]->GetName() == name )
		{
			return fx[i];
		}
	}

	return NULL;
}

/*
==================
FullscreenFXManager::CreateFX
==================
*/
void FullscreenFXManager::CreateFX( idStr name, idStr fxtype, int fade )
{
	FullscreenFX* pfx = NULL;

	if( fxtype == "helltime" )
	{
		pfx = new( TAG_FX ) FullscreenFX_Helltime;
	}
	else if( fxtype == "warp" )
	{
		pfx = new( TAG_FX ) FullscreenFX_Warp;
	}
	else if( fxtype == "envirosuit" )
	{
		pfx = new( TAG_FX ) FullscreenFX_EnviroSuit;
	}
	else if( fxtype == "doublevision" )
	{
		pfx = new( TAG_FX ) FullscreenFX_DoubleVision;
	}
	else if( fxtype == "multiplayer" )
	{
		pfx = new( TAG_FX ) FullscreenFX_Multiplayer;
	}
	else if( fxtype == "influencevision" )
	{
		pfx = new( TAG_FX ) FullscreenFX_InfluenceVision;
	}
	else if( fxtype == "bloom" )
	{
		pfx = new( TAG_FX ) FullscreenFX_Bloom;
	}
	else
	{
		assert( 0 );
	}

	if( pfx )
	{
		pfx->Initialize();
		pfx->SetFXManager( this );
		pfx->SetName( name );
		pfx->SetFadeSpeed( fade );
		fx.Append( pfx );
	}
}

/*
==================
FullscreenFXManager::Initialize
==================
*/
void FullscreenFXManager::Initialize( idPlayerView* pv )
{
	// set the playerview
	playerView = pv;
	blendBackMaterial = declManager->FindMaterial( "textures/d3bfg/blendBack" );

	// allocate the fx
	CreateFX( "helltime", "helltime", 1000 );
	CreateFX( "warp", "warp", 0 );
	CreateFX( "envirosuit", "envirosuit", 500 );
	CreateFX( "doublevision", "doublevision", 0 );
	CreateFX( "multiplayer", "multiplayer", 1000 );
	CreateFX( "influencevision", "influencevision", 1000 );
	CreateFX( "bloom", "bloom", 0 );

	// pre-cache the texture grab so we dont hitch
	renderSystem->CropRenderSize( 512, 512 );
	renderSystem->CaptureRenderToImage( "_accum" );
	renderSystem->UnCrop();

	renderSystem->CaptureRenderToImage( "_currentRender" );
}

/*
==================
FullscreenFXManager::Blendback
==================
*/
void FullscreenFXManager::Blendback( float alpha )
{
	// alpha fade
	if( alpha < 1.f )
	{
		renderSystem->SetColor4( 1.0f, 1.0f, 1.0f, 1.0f - alpha );
		float s0 = 0.0f;
		float t0 = 1.0f;
		float s1 = 1.0f;
		float t1 = 0.0f;
		renderSystem->DrawStretchPic( 0.0f, 0.0f, renderSystem->GetVirtualWidth(), renderSystem->GetVirtualHeight(), s0, t0, s1, t1, blendBackMaterial );
	}
}

/*
==================
FullscreenFXManager::Save
==================
*/
void FullscreenFXManager::Save( idSaveGame* savefile )
{
	for( int i = 0; i < fx.Num(); i++ )
	{
		FullscreenFX* pfx = fx[i];
		pfx->Save( savefile );
	}
}

/*
==================
FullscreenFXManager::Restore
==================
*/
void FullscreenFXManager::Restore( idRestoreGame* savefile )
{
	for( int i = 0; i < fx.Num(); i++ )
	{
		FullscreenFX* pfx = fx[i];
		pfx->Restore( savefile );
	}
}

idCVar player_allowScreenFXInStereo( "player_allowScreenFXInStereo", "1", CVAR_BOOL, "allow full screen fx in stereo mode" );

/*
==================
FullscreenFXManager::Process
==================
*/
void FullscreenFXManager::Process( const renderView_t* view )
{
	bool allpass = false;
	bool atLeastOneFX = false;

	if( g_testFullscreenFX.GetInteger() == -2 )
	{
		allpass = true;
	}

	// do the first render
	gameRenderWorld->RenderScene( view );

	// we should consider these on a case-by-case basis for stereo rendering
	// double vision could be implemented "for real" by shifting the
	// eye views
	if( IsGameStereoRendered() && !player_allowScreenFXInStereo.GetBool() )
	{
		return;
	}

// RB: skip for now so the game is playable. These old effects really suck with modern APIs
#if !defined( USE_NVRHI )

	// do the process
	for( int i = 0; i < fx.Num(); i++ )
	{
		FullscreenFX* pfx = fx[i];
		bool drawIt = false;

		// determine if we need to draw
		if( pfx->Active() || g_testFullscreenFX.GetInteger() == i || allpass )
		{
			drawIt = pfx->SetTriggerState( true );
		}
		else
		{
			drawIt = pfx->SetTriggerState( false );
		}

		// do the actual drawing
		if( drawIt )
		{
			atLeastOneFX = true;

			// we need to dump to _currentRender
			renderSystem->CaptureRenderToImage( "_currentRender" );

			// handle the accum pass if we have one
			if( pfx->HasAccum() )
			{
				// we need to crop the accum pass
				renderSystem->CropRenderSize( 512, 512 );
				pfx->AccumPass( view );
				renderSystem->CaptureRenderToImage( "_accum" );
				renderSystem->UnCrop();
			}

			// do the high quality pass
			pfx->HighQuality();

			// do the blendback
			Blendback( pfx->GetFadeAlpha() );
		}
	}
#endif
}



