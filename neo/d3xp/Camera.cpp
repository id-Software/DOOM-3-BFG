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

  idCamera

  Base class for cameras

===============================================================================
*/

ABSTRACT_DECLARATION( idEntity, idCamera )
END_CLASS

/*
=====================
idCamera::Spawn
=====================
*/
void idCamera::Spawn()
{
}

/*
=====================
idCamera::GetRenderView
=====================
*/
renderView_t* idCamera::GetRenderView()
{
	renderView_t* rv = idEntity::GetRenderView();
	GetViewParms( rv );
	return rv;
}

/***********************************************************************

  idCameraView

***********************************************************************/
const idEventDef EV_Camera_SetAttachments( "<getattachments>", NULL );

CLASS_DECLARATION( idCamera, idCameraView )
EVENT( EV_Activate,				idCameraView::Event_Activate )
EVENT( EV_Camera_SetAttachments, idCameraView::Event_SetAttachments )
END_CLASS


/*
===============
idCameraView::idCameraView
================
*/
idCameraView::idCameraView()
{
	fov = 90.0f;
	attachedTo = NULL;
	attachedView = NULL;
}

/*
===============
idCameraView::Save
================
*/
void idCameraView::Save( idSaveGame* savefile ) const
{
	savefile->WriteFloat( fov );
	savefile->WriteObject( attachedTo );
	savefile->WriteObject( attachedView );
}

/*
===============
idCameraView::Restore
================
*/
void idCameraView::Restore( idRestoreGame* savefile )
{
	savefile->ReadFloat( fov );
	savefile->ReadObject( reinterpret_cast<idClass*&>( attachedTo ) );
	savefile->ReadObject( reinterpret_cast<idClass*&>( attachedView ) );
}

/*
===============
idCameraView::Event_SetAttachments
================
*/
void idCameraView::Event_SetAttachments( )
{
	SetAttachment( &attachedTo, "attachedTo" );
	SetAttachment( &attachedView, "attachedView" );
}

/*
===============
idCameraView::Event_Activate
================
*/
void idCameraView::Event_Activate( idEntity* activator )
{
	if( spawnArgs.GetBool( "trigger" ) )
	{
		if( gameLocal.GetCamera() != this )
		{
			if( g_debugCinematic.GetBool() )
			{
				gameLocal.Printf( "%d: '%s' start\n", gameLocal.framenum, GetName() );
			}
			
			gameLocal.SetCamera( this );
		}
		else
		{
			if( g_debugCinematic.GetBool() )
			{
				gameLocal.Printf( "%d: '%s' stop\n", gameLocal.framenum, GetName() );
			}
			gameLocal.SetCamera( NULL );
		}
	}
}

/*
=====================
idCameraView::Stop
=====================
*/
void idCameraView::Stop()
{
	if( g_debugCinematic.GetBool() )
	{
		gameLocal.Printf( "%d: '%s' stop\n", gameLocal.framenum, GetName() );
	}
	gameLocal.SetCamera( NULL );
	ActivateTargets( gameLocal.GetLocalPlayer() );
}


/*
=====================
idCameraView::Spawn
=====================
*/
void idCameraView::SetAttachment( idEntity** e, const char* p )
{
	const char* cam = spawnArgs.GetString( p );
	if( strlen( cam ) )
	{
		*e = gameLocal.FindEntity( cam );
	}
}


/*
=====================
idCameraView::Spawn
=====================
*/
void idCameraView::Spawn()
{
	// if no target specified use ourself
	const char* cam = spawnArgs.GetString( "cameraTarget" );
	if( strlen( cam ) == 0 )
	{
		spawnArgs.Set( "cameraTarget", spawnArgs.GetString( "name" ) );
	}
	fov = spawnArgs.GetFloat( "fov", "90" );
	
	PostEventMS( &EV_Camera_SetAttachments, 0 );
	
	UpdateChangeableSpawnArgs( NULL );
}

/*
=====================
idCameraView::GetViewParms
=====================
*/
void idCameraView::GetViewParms( renderView_t* view )
{
	assert( view );
	
	if( view == NULL )
	{
		return;
	}
	
	idVec3 dir;
	idEntity* ent;
	
	if( attachedTo )
	{
		ent = attachedTo;
	}
	else
	{
		ent = this;
	}
	
	view->vieworg = ent->GetPhysics()->GetOrigin();
	if( attachedView )
	{
		dir = attachedView->GetPhysics()->GetOrigin() - view->vieworg;
		dir.Normalize();
		view->viewaxis = dir.ToMat3();
	}
	else
	{
		view->viewaxis = ent->GetPhysics()->GetAxis();
	}
	
	gameLocal.CalcFov( fov, view->fov_x, view->fov_y );
}

/*
===============================================================================

  idCameraAnim

===============================================================================
*/

const idEventDef EV_Camera_Start( "start", NULL );
const idEventDef EV_Camera_Stop( "stop", NULL );

CLASS_DECLARATION( idCamera, idCameraAnim )
EVENT( EV_Thread_SetCallback,	idCameraAnim::Event_SetCallback )
EVENT( EV_Camera_Stop,			idCameraAnim::Event_Stop )
EVENT( EV_Camera_Start,			idCameraAnim::Event_Start )
EVENT( EV_Activate,				idCameraAnim::Event_Activate )
END_CLASS


/*
=====================
idCameraAnim::idCameraAnim
=====================
*/
idCameraAnim::idCameraAnim()
{
	threadNum = 0;
	offset.Zero();
	frameRate = 0;
	cycle = 1;
	starttime = 0;
	activator = NULL;
	
}

/*
=====================
idCameraAnim::~idCameraAnim
=====================
*/
idCameraAnim::~idCameraAnim()
{
	if( gameLocal.GetCamera() == this )
	{
		gameLocal.SetCamera( NULL );
	}
}

/*
===============
idCameraAnim::Save
================
*/
void idCameraAnim::Save( idSaveGame* savefile ) const
{
	savefile->WriteInt( threadNum );
	savefile->WriteVec3( offset );
	savefile->WriteInt( frameRate );
	savefile->WriteInt( starttime );
	savefile->WriteInt( cycle );
	activator.Save( savefile );
}

/*
===============
idCameraAnim::Restore
================
*/
void idCameraAnim::Restore( idRestoreGame* savefile )
{
	savefile->ReadInt( threadNum );
	savefile->ReadVec3( offset );
	savefile->ReadInt( frameRate );
	savefile->ReadInt( starttime );
	savefile->ReadInt( cycle );
	activator.Restore( savefile );
	
	LoadAnim();
}

/*
=====================
idCameraAnim::Spawn
=====================
*/
void idCameraAnim::Spawn()
{
	if( spawnArgs.GetVector( "old_origin", "0 0 0", offset ) )
	{
		offset = GetPhysics()->GetOrigin() - offset;
	}
	else
	{
		offset.Zero();
	}
	
	// always think during cinematics
	cinematic = true;
	
	LoadAnim();
}

/*
================
idCameraAnim::Load
================
*/
void idCameraAnim::LoadAnim()
{
	int			version;
	idLexer		parser( LEXFL_ALLOWPATHNAMES | LEXFL_NOSTRINGESCAPECHARS | LEXFL_NOSTRINGCONCAT );
	idToken		token;
	int			numFrames;
	int			numCuts;
	int			i;
	idStr		filename;
	const char*	key;
	
	key = spawnArgs.GetString( "anim" );
	if( !key )
	{
		gameLocal.Error( "Missing 'anim' key on '%s'", name.c_str() );
	}
	
	filename = spawnArgs.GetString( va( "anim %s", key ) );
	if( !filename.Length() )
	{
		gameLocal.Error( "Missing 'anim %s' key on '%s'", key, name.c_str() );
	}
	
	filename.SetFileExtension( MD5_CAMERA_EXT );
	if( !parser.LoadFile( filename ) )
	{
		gameLocal.Error( "Unable to load '%s' on '%s'", filename.c_str(), name.c_str() );
	}
	
	cameraCuts.Clear();
	cameraCuts.SetGranularity( 1 );
	camera.Clear();
	camera.SetGranularity( 1 );
	
	parser.ExpectTokenString( MD5_VERSION_STRING );
	version = parser.ParseInt();
	if( version != MD5_VERSION )
	{
		parser.Error( "Invalid version %d.  Should be version %d\n", version, MD5_VERSION );
	}
	
	// skip the commandline
	parser.ExpectTokenString( "commandline" );
	parser.ReadToken( &token );
	
	// parse num frames
	parser.ExpectTokenString( "numFrames" );
	numFrames = parser.ParseInt();
	if( numFrames <= 0 )
	{
		parser.Error( "Invalid number of frames: %d", numFrames );
	}
	
	// parse framerate
	parser.ExpectTokenString( "frameRate" );
	frameRate = parser.ParseInt();
	if( frameRate <= 0 )
	{
		parser.Error( "Invalid framerate: %d", frameRate );
	}
	
	// parse num cuts
	parser.ExpectTokenString( "numCuts" );
	numCuts = parser.ParseInt();
	if( ( numCuts < 0 ) || ( numCuts > numFrames ) )
	{
		parser.Error( "Invalid number of camera cuts: %d", numCuts );
	}
	
	// parse the camera cuts
	parser.ExpectTokenString( "cuts" );
	parser.ExpectTokenString( "{" );
	cameraCuts.SetNum( numCuts );
	for( i = 0; i < numCuts; i++ )
	{
		cameraCuts[ i ] = parser.ParseInt();
		if( ( cameraCuts[ i ] < 1 ) || ( cameraCuts[ i ] >= numFrames ) )
		{
			parser.Error( "Invalid camera cut" );
		}
	}
	parser.ExpectTokenString( "}" );
	
	// parse the camera frames
	parser.ExpectTokenString( "camera" );
	parser.ExpectTokenString( "{" );
	camera.SetNum( numFrames );
	for( i = 0; i < numFrames; i++ )
	{
		parser.Parse1DMatrix( 3, camera[ i ].t.ToFloatPtr() );
		parser.Parse1DMatrix( 3, camera[ i ].q.ToFloatPtr() );
		camera[ i ].fov = parser.ParseFloat();
	}
	parser.ExpectTokenString( "}" );
}

/*
===============
idCameraAnim::Start
================
*/
void idCameraAnim::Start()
{
	cycle = spawnArgs.GetInt( "cycle" );
	if( !cycle )
	{
		cycle = 1;
	}
	
	if( g_debugCinematic.GetBool() )
	{
		gameLocal.Printf( "%d: '%s' start\n", gameLocal.framenum, GetName() );
	}
	
	starttime = gameLocal.time;
	gameLocal.SetCamera( this );
	BecomeActive( TH_THINK );
	
	// if the player has already created the renderview for this frame, have him update it again so that the camera starts this frame
	if( gameLocal.GetLocalPlayer()->GetRenderView()->time[TIME_GROUP2] == gameLocal.fast.time )
	{
		gameLocal.GetLocalPlayer()->CalculateRenderView();
	}
}

/*
=====================
idCameraAnim::Stop
=====================
*/
void idCameraAnim::Stop()
{
	if( gameLocal.GetCamera() == this )
	{
		if( g_debugCinematic.GetBool() )
		{
			gameLocal.Printf( "%d: '%s' stop\n", gameLocal.framenum, GetName() );
		}
		
		BecomeInactive( TH_THINK );
		gameLocal.SetCamera( NULL );
		if( threadNum )
		{
			idThread::ObjectMoveDone( threadNum, this );
			threadNum = 0;
		}
		ActivateTargets( activator.GetEntity() );
	}
}

/*
=====================
idCameraAnim::Think
=====================
*/
void idCameraAnim::Think()
{
}

/*
=====================
idCameraAnim::GetViewParms
=====================
*/
void idCameraAnim::GetViewParms( renderView_t* view )
{
	int				realFrame;
	int				frame;
	int				frameTime;
	float			lerp;
	float			invlerp;
	cameraFrame_t*	camFrame;
	int				i;
	int				cut;
	idQuat			q1, q2, q3;
	
	assert( view );
	if( !view )
	{
		return;
	}
	
	if( camera.Num() == 0 )
	{
		// we most likely are in the middle of a restore
		// FIXME: it would be better to fix it so this doesn't get called during a restore
		return;
	}
	
	SetTimeState ts( timeGroup );
	
	frameTime	= ( gameLocal.time - starttime ) * frameRate;
	frame		= frameTime / 1000;
	lerp		= ( frameTime % 1000 ) * 0.001f;
	
	// skip any frames where camera cuts occur
	realFrame = frame;
	cut = 0;
	for( i = 0; i < cameraCuts.Num(); i++ )
	{
		if( frame < cameraCuts[ i ] )
		{
			break;
		}
		frame++;
		cut++;
	}
	
	if( g_debugCinematic.GetBool() )
	{
		int prevFrameTime	= ( gameLocal.previousTime - starttime ) * frameRate;
		int prevFrame		= prevFrameTime / 1000;
		int prevCut;
		
		prevCut = 0;
		for( i = 0; i < cameraCuts.Num(); i++ )
		{
			if( prevFrame < cameraCuts[ i ] )
			{
				break;
			}
			prevFrame++;
			prevCut++;
		}
		
		if( prevCut != cut )
		{
			gameLocal.Printf( "%d: '%s' cut %d\n", gameLocal.framenum, GetName(), cut );
		}
	}
	
	// clamp to the first frame.  also check if this is a one frame anim.  one frame anims would end immediately,
	// but since they're mainly used for static cams anyway, just stay on it infinitely.
	if( ( frame < 0 ) || ( camera.Num() < 2 ) )
	{
		view->viewaxis = camera[ 0 ].q.ToQuat().ToMat3();
		view->vieworg = camera[ 0 ].t + offset;
		view->fov_x = camera[ 0 ].fov;
	}
	else if( frame > camera.Num() - 2 )
	{
		if( cycle > 0 )
		{
			cycle--;
		}
		
		if( cycle != 0 )
		{
			// advance start time so that we loop
			starttime += ( ( camera.Num() - cameraCuts.Num() ) * 1000 ) / frameRate;
			GetViewParms( view );
			return;
		}
		
		Stop();
		if( gameLocal.GetCamera() != NULL )
		{
			// we activated another camera when we stopped, so get it's viewparms instead
			gameLocal.GetCamera()->GetViewParms( view );
			return;
		}
		else
		{
			// just use our last frame
			camFrame = &camera[ camera.Num() - 1 ];
			view->viewaxis = camFrame->q.ToQuat().ToMat3();
			view->vieworg = camFrame->t + offset;
			view->fov_x = camFrame->fov;
		}
	}
	else if( lerp == 0.0f )
	{
		camFrame = &camera[ frame ];
		view->viewaxis = camFrame[ 0 ].q.ToMat3();
		view->vieworg = camFrame[ 0 ].t + offset;
		view->fov_x = camFrame[ 0 ].fov;
	}
	else
	{
		camFrame = &camera[ frame ];
		invlerp = 1.0f - lerp;
		q1 = camFrame[ 0 ].q.ToQuat();
		q2 = camFrame[ 1 ].q.ToQuat();
		q3.Slerp( q1, q2, lerp );
		view->viewaxis = q3.ToMat3();
		view->vieworg = camFrame[ 0 ].t * invlerp + camFrame[ 1 ].t * lerp + offset;
		view->fov_x = camFrame[ 0 ].fov * invlerp + camFrame[ 1 ].fov * lerp;
	}
	
	gameLocal.CalcFov( view->fov_x, view->fov_x, view->fov_y );
	
	// setup the pvs for this frame
	UpdatePVSAreas( view->vieworg );
	
#if 0
	static int lastFrame = 0;
	static idVec3 lastFrameVec( 0.0f, 0.0f, 0.0f );
	if( gameLocal.time != lastFrame )
	{
		gameRenderWorld->DebugBounds( colorCyan, idBounds( view->vieworg ).Expand( 16.0f ), vec3_origin, 1 );
		gameRenderWorld->DebugLine( colorRed, view->vieworg, view->vieworg + idVec3( 0.0f, 0.0f, 2.0f ), 10000, false );
		gameRenderWorld->DebugLine( colorCyan, lastFrameVec, view->vieworg, 10000, false );
		gameRenderWorld->DebugLine( colorYellow, view->vieworg + view->viewaxis[ 0 ] * 64.0f, view->vieworg + view->viewaxis[ 0 ] * 66.0f, 10000, false );
		gameRenderWorld->DebugLine( colorOrange, view->vieworg + view->viewaxis[ 0 ] * 64.0f, view->vieworg + view->viewaxis[ 0 ] * 64.0f + idVec3( 0.0f, 0.0f, 2.0f ), 10000, false );
		lastFrameVec = view->vieworg;
		lastFrame = gameLocal.time;
	}
#endif
	
	if( g_showcamerainfo.GetBool() )
	{
		gameLocal.Printf( "^5Frame: ^7%d/%d\n\n\n", realFrame + 1, camera.Num() - cameraCuts.Num() );
	}
}

/*
===============
idCameraAnim::Event_Activate
================
*/
void idCameraAnim::Event_Activate( idEntity* _activator )
{
	activator = _activator;
	if( thinkFlags & TH_THINK )
	{
		Stop();
	}
	else
	{
		Start();
	}
}

/*
===============
idCameraAnim::Event_Start
================
*/
void idCameraAnim::Event_Start()
{
	Start();
}

/*
===============
idCameraAnim::Event_Stop
================
*/
void idCameraAnim::Event_Stop()
{
	Stop();
}

/*
================
idCameraAnim::Event_SetCallback
================
*/
void idCameraAnim::Event_SetCallback()
{
	if( ( gameLocal.GetCamera() == this ) && !threadNum )
	{
		threadNum = idThread::CurrentThreadNum();
		idThread::ReturnInt( true );
	}
	else
	{
		idThread::ReturnInt( false );
	}
}
