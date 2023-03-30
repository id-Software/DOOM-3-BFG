/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2012-2016 Robert Beckebans
Copyright (C) 2014-2016 Kot in Action Creative Artel

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

#include "Common_local.h"
#include "../renderer/Image.h"

// RB begin
#if defined(USE_DOOMCLASSIC)
	#include "../../doomclassic/doom/doomlib.h"
	#include "../../doomclassic/doom/globaldata.h"
#endif
// RB end

/*

New for tech4x:

Unlike previous SMP work, the actual GPU command drawing is done in the main thread, which avoids the
OpenGL problems with needing windows to be created by the same thread that creates the context, as well
as the issues with passing context ownership back and forth on the 360.

The game tic and the generation of the draw command list is now run in a separate thread, and overlapped
with the interpretation of the previous draw command list.

While the game tic should be nicely contained, the draw command generation winds through the user interface
code, and is potentially hazardous.  For now, the overlap will be restricted to the renderer back end,
which should also be nicely contained.

*/
#define DEFAULT_FIXED_TIC "0"
#define DEFAULT_NO_SLEEP "0"

idCVar com_deltaTimeClamp( "com_deltaTimeClamp", "50", CVAR_INTEGER, "don't process more than this time in a single frame" );

idCVar com_fixedTic( "com_fixedTic", DEFAULT_FIXED_TIC, CVAR_BOOL, "run a single game frame per render frame" );
idCVar com_noSleep( "com_noSleep", DEFAULT_NO_SLEEP, CVAR_BOOL, "don't sleep if the game is running too fast" );
idCVar com_smp( "com_smp", "1", CVAR_BOOL | CVAR_SYSTEM | CVAR_NOCHEAT, "run the game and draw code in a separate thread" );
idCVar com_aviDemoWidth( "com_aviDemoWidth", "256", CVAR_SYSTEM, "" );
idCVar com_aviDemoHeight( "com_aviDemoHeight", "256", CVAR_SYSTEM, "" );
idCVar com_skipGameDraw( "com_skipGameDraw", "0", CVAR_SYSTEM | CVAR_BOOL, "" );

idCVar com_sleepGame( "com_sleepGame", "0", CVAR_SYSTEM | CVAR_INTEGER, "intentionally add a sleep in the game time" );
idCVar com_sleepDraw( "com_sleepDraw", "0", CVAR_SYSTEM | CVAR_INTEGER, "intentionally add a sleep in the draw time" );
idCVar com_sleepRender( "com_sleepRender", "0", CVAR_SYSTEM | CVAR_INTEGER, "intentionally add a sleep in the render time" );

idCVar net_drawDebugHud( "net_drawDebugHud", "0", CVAR_SYSTEM | CVAR_INTEGER, "0 = None, 1 = Hud 1, 2 = Hud 2, 3 = Snapshots" );

idCVar timescale( "timescale", "1", CVAR_SYSTEM | CVAR_FLOAT, "Number of game frames to run per render frame", 0.001f, 100.0f );

extern idCVar in_useJoystick;
extern idCVar in_joystickRumble;

/*
===============
idGameThread::Run

Run in a background thread for performance, but can also
be called directly in the foreground thread for comparison.
===============
*/
int idGameThread::Run()
{
	OPTICK_THREAD( "idGameThread" );

	commonLocal.frameTiming.startGameTime = Sys_Microseconds();

	// debugging tool to test frame dropping behavior
	if( com_sleepGame.GetInteger() )
	{
		Sys_Sleep( com_sleepGame.GetInteger() );
	}

	if( numGameFrames == 0 )
	{
		// Ensure there's no stale gameReturn data from a paused game
		ret = gameReturn_t();
	}

	if( isClient )
	{
		// run the game logic
		for( int i = 0; i < numGameFrames; i++ )
		{
			SCOPED_PROFILE_EVENT( "Client Prediction" );
			if( userCmdMgr )
			{
				game->ClientRunFrame( *userCmdMgr, ( i == numGameFrames - 1 ), ret );
			}
			if( ret.syncNextGameFrame || ret.sessionCommand[0] != 0 )
			{
				break;
			}
		}
	}
	else
	{
		// run the game logic
		for( int i = 0; i < numGameFrames; i++ )
		{
			SCOPED_PROFILE_EVENT( "GameTic" );
			if( userCmdMgr )
			{
				game->RunFrame( *userCmdMgr, ret );
			}
			if( ret.syncNextGameFrame || ret.sessionCommand[0] != 0 )
			{
				break;
			}
		}
	}

	// we should have consumed all of our usercmds
	if( userCmdMgr )
	{
		// RB begin
#if defined(USE_DOOMCLASSIC)
		if( userCmdMgr->HasUserCmdForPlayer( game->GetLocalClientNum() ) && common->GetCurrentGame() == DOOM3_BFG )
#else
		if( userCmdMgr->HasUserCmdForPlayer( game->GetLocalClientNum() ) )
#endif
			// RB end
		{
			idLib::Printf( "idGameThread::Run: didn't consume all usercmds\n" );
		}
	}

	commonLocal.frameTiming.finishGameTime = Sys_Microseconds();

	SetThreadGameTime( ( commonLocal.frameTiming.finishGameTime - commonLocal.frameTiming.startGameTime ) / 1000 );

	// build render commands and geometry
	{
		SCOPED_PROFILE_EVENT( "Draw" );
		commonLocal.Draw();
	}

	commonLocal.frameTiming.finishDrawTime = Sys_Microseconds();

	SetThreadRenderTime( ( commonLocal.frameTiming.finishDrawTime - commonLocal.frameTiming.finishGameTime ) / 1000 );

	SetThreadTotalTime( ( commonLocal.frameTiming.finishDrawTime - commonLocal.frameTiming.startGameTime ) / 1000 );

	return 0;
}

/*
===============
idGameThread::RunGameAndDraw

===============
*/
gameReturn_t idGameThread::RunGameAndDraw( int numGameFrames_, idUserCmdMgr& userCmdMgr_, bool isClient_, int startGameFrame )
{
	// this should always immediately return
	this->WaitForThread();

	// save the usercmds for the background thread to pick up
	userCmdMgr = &userCmdMgr_;

	isClient = isClient_;

	// grab the return value created by the last thread execution
	gameReturn_t latchedRet = ret;

	numGameFrames = numGameFrames_;

	// start the thread going
	// foresthale 2014-05-12: also check com_editors as many of them are not particularly thread-safe (editLights for example)
	if( !com_smp.GetBool() || com_editors != 0 )
	{
		// run it in the main thread so PIX profiling catches everything
		Run();
	}
	else
	{
		this->SignalWork();
	}

	// return the latched result while the thread runs in the background
	return latchedRet;
}


/*
===============
idCommonLocal::DrawWipeModel

Draw the fade material over everything that has been drawn
===============
*/
void idCommonLocal::DrawWipeModel()
{

	if( wipeStartTime >= wipeStopTime )
	{
		return;
	}

	int currentTime = Sys_Milliseconds();

	if( !wipeHold && currentTime > wipeStopTime )
	{
		return;
	}

	float fade = ( float )( currentTime - wipeStartTime ) / ( wipeStopTime - wipeStartTime );
	renderSystem->SetColor4( 1, 1, 1, fade );
	renderSystem->DrawStretchPic( 0, 0, renderSystem->GetVirtualWidth(), renderSystem->GetVirtualHeight(), 0, 0, 1, 1, wipeMaterial );
}

/*
===============
idCommonLocal::Draw
===============
*/
void idCommonLocal::Draw()
{
	// debugging tool to test frame dropping behavior
	if( com_sleepDraw.GetInteger() )
	{
		Sys_Sleep( com_sleepDraw.GetInteger() );
	}

	if( loadPacifierBinarizeActive )
	{
		// foresthale 2014-05-30: when binarizing an asset we show a special
		// overlay indicating progress
		renderSystem->SetColor( colorBlack );
		renderSystem->DrawStretchPic( 0, 0, renderSystem->GetVirtualWidth(), renderSystem->GetVirtualHeight(), 0, 0, 1, 1, whiteMaterial );

		// render the loading gui (idSWF actually) if it is loaded
		// (we want to see progress of the loading gui binarize too)
		if( loadGUI != NULL )
		{
			loadGUI->Render( renderSystem, Sys_Milliseconds() );
		}

		// update our progress estimates
		int time = Sys_Milliseconds();
		if( loadPacifierBinarizeProgress > 0.0f )
		{
			loadPacifierBinarizeTimeLeft = ( 1.0 - loadPacifierBinarizeProgress ) * ( time - loadPacifierBinarizeStartTime ) * 0.001f / loadPacifierBinarizeProgress;
		}
		else
		{
			loadPacifierBinarizeTimeLeft = -1.0f;
		}

		// prepare our strings
		const char* text;
		if( loadPacifierBinarizeTimeLeft >= 99.5f )
		{
			text = va( "Binarizing %3.0f%% ETA %2.0f minutes", loadPacifierBinarizeProgress * 100.0f, loadPacifierBinarizeTimeLeft / 60.0f );
		}
		else if( loadPacifierBinarizeTimeLeft )
		{
			text = va( "Binarizing %3.0f%% ETA %2.0f seconds", loadPacifierBinarizeProgress * 100.0f, loadPacifierBinarizeTimeLeft );
		}
		else
		{
			text = va( "Binarizing %3.0f%%", loadPacifierBinarizeProgress * 100.0f );
		}

		// draw our basic overlay
		renderSystem->SetColor( idVec4( 0.0f, 0.0f, 0.5f, 1.0f ) );
		renderSystem->DrawStretchPic( 0, renderSystem->GetVirtualHeight() - 48, renderSystem->GetVirtualWidth(), 48, 0, 0, 1, 1, whiteMaterial );
		renderSystem->SetColor( idVec4( 0.0f, 0.5f, 0.8f, 1.0f ) );
		renderSystem->DrawStretchPic( 0, renderSystem->GetVirtualHeight() - 48, loadPacifierBinarizeProgress * renderSystem->GetVirtualWidth(), 32, 0, 0, 1, 1, whiteMaterial );
		renderSystem->DrawSmallStringExt( 0, renderSystem->GetVirtualHeight() - 48, loadPacifierBinarizeFilename.c_str(), idVec4( 1.0f, 1.0f, 1.0f, 1.0f ), true );
		renderSystem->DrawSmallStringExt( 0, renderSystem->GetVirtualHeight() - 32, va( "%s %d/%d lvls", loadPacifierBinarizeInfo.c_str(), loadPacifierBinarizeMiplevel, loadPacifierBinarizeMiplevelTotal ), idVec4( 1.0f, 1.0f, 1.0f, 1.0f ), true );
		renderSystem->DrawSmallStringExt( 0, renderSystem->GetVirtualHeight() - 16, text, idVec4( 1.0f, 1.0f, 1.0f, 1.0f ), true );
	}
	else if( loadGUI != NULL )
	{
		// foresthale 2014-05-30: showing a black background looks better than flickering in widescreen
		renderSystem->SetColor( colorBlack );
		renderSystem->DrawStretchPic( 0, 0, renderSystem->GetVirtualWidth(), renderSystem->GetVirtualHeight(), 0, 0, 1, 1, whiteMaterial );

		loadGUI->Render( renderSystem, Sys_Milliseconds() );
	}
	// RB begin
#if defined(USE_DOOMCLASSIC)
	else if( currentGame == DOOM_CLASSIC || currentGame == DOOM2_CLASSIC )
	{
		const float sysWidth = renderSystem->GetWidth() * renderSystem->GetPixelAspect();
		const float sysHeight = renderSystem->GetHeight();
		const float sysAspect = sysWidth / sysHeight;
		const float doomAspect = 4.0f / 3.0f;
		const float adjustment = sysAspect / doomAspect;
		const float barHeight = ( adjustment >= 1.0f ) ? 0.0f : ( 1.0f - adjustment ) * ( float )renderSystem->GetVirtualHeight() * 0.25f;
		const float barWidth = ( adjustment <= 1.0f ) ? 0.0f : ( adjustment - 1.0f ) * ( float )renderSystem->GetVirtualWidth() * 0.25f;
		if( barHeight > 0.0f )
		{
			renderSystem->SetColor( colorBlack );
			renderSystem->DrawStretchPic( 0, 0, renderSystem->GetVirtualWidth(), barHeight, 0, 0, 1, 1, whiteMaterial );
			renderSystem->DrawStretchPic( 0, renderSystem->GetVirtualHeight() - barHeight, renderSystem->GetVirtualWidth(), barHeight, 0, 0, 1, 1, whiteMaterial );
		}
		if( barWidth > 0.0f )
		{
			renderSystem->SetColor( colorBlack );
			renderSystem->DrawStretchPic( 0, 0, barWidth, renderSystem->GetVirtualHeight(), 0, 0, 1, 1, whiteMaterial );
			renderSystem->DrawStretchPic( renderSystem->GetVirtualWidth() - barWidth, 0, barWidth, renderSystem->GetVirtualHeight(), 0, 0, 1, 1, whiteMaterial );
		}
		renderSystem->SetColor4( 1, 1, 1, 1 );
		renderSystem->DrawStretchPic( barWidth, barHeight, renderSystem->GetVirtualWidth() - barWidth * 2.0f, renderSystem->GetVirtualHeight() - barHeight * 2.0f, 0, 0, 1, 1, doomClassicMaterial );
	}
#endif
	// RB end
	else if( game && game->Shell_IsActive() )
	{
		bool gameDraw = game->Draw( game->GetLocalClientNum() );
		if( !gameDraw )
		{
			renderSystem->SetColor( colorBlack );
			renderSystem->DrawStretchPic( 0, 0, renderSystem->GetVirtualWidth(), renderSystem->GetVirtualHeight(), 0, 0, 1, 1, whiteMaterial );
		}
		game->Shell_Render();
	}
	else if( readDemo )
	{
		// SRS - Advance demo inside Frame() instead of Draw() to support smp mode playback
		// AdvanceRenderDemo( true );
		renderWorld->RenderScene( &currentDemoRenderView );
		renderSystem->DrawDemoPics();
	}
	else if( mapSpawned )
	{
		bool gameDraw = false;
		// normal drawing for both single and multi player
		if( !com_skipGameDraw.GetBool() && Game()->GetLocalClientNum() >= 0 )
		{
			// draw the game view
			int	start = Sys_Milliseconds();
			if( game )
			{
				gameDraw = game->Draw( Game()->GetLocalClientNum() );
			}
			int end = Sys_Milliseconds();
			time_gameDraw += ( end - start );	// note time used for com_speeds
		}
		if( !gameDraw )
		{
			renderSystem->SetColor( colorBlack );
			renderSystem->DrawStretchPic( 0, 0, renderSystem->GetVirtualWidth(), renderSystem->GetVirtualHeight(), 0, 0, 1, 1, whiteMaterial );
		}

		// save off the 2D drawing from the game
		if( writeDemo )
		{
			renderSystem->WriteDemoPics();
			renderSystem->WriteEndFrame();
		}
	}
	else
	{
		renderSystem->SetColor4( 0, 0, 0, 1 );
		renderSystem->DrawStretchPic( 0, 0, renderSystem->GetVirtualWidth(), renderSystem->GetVirtualHeight(), 0, 0, 1, 1, whiteMaterial );
	}

	{
		SCOPED_PROFILE_EVENT( "Post-Draw" );

		// draw the wipe material on top of this if it hasn't completed yet
		DrawWipeModel();

		Dialog().Render( loadGUI != NULL );

		// draw the half console / notify console on top of everything
		console->Draw( false );
	}
}

/*
===============
idCommonLocal::UpdateScreen

This is an out-of-sequence screen update, not the normal game rendering
===============
*/
// DG: added possibility to *not* release mouse in UpdateScreen(), it fucks up the view angle for screenshots
void idCommonLocal::UpdateScreen( bool captureToImage, bool releaseMouse )
{
	if( insideUpdateScreen || com_shuttingDown )
	{
		return;
	}
	insideUpdateScreen = true;

	// make sure the game / draw thread has completed
	gameThread.WaitForThread();

	// release the mouse capture back to the desktop
	if( releaseMouse )
	{
		Sys_GrabMouseCursor( false );
	}
	// DG end

	// build all the draw commands without running a new game tic
	Draw();
	frameTiming.finishDrawTime = Sys_Microseconds();    // SRS - Added frame timing for out-of-sequence updates (e.g. used in timedemo "twice" mode)

	// foresthale 2014-03-01: note: the only place that has captureToImage=true is idAutoRender::StartBackgroundAutoSwaps
	if( captureToImage )
	{
		renderSystem->CaptureRenderToImage( "_currentRender", false );
	}

	// this should exit right after vsync, with the GPU idle and ready to draw
	const emptyCommand_t* cmd = renderSystem->SwapCommandBuffers( &time_frontend, &time_backend, &time_shadows, &time_gpu, &stats_backend, &stats_frontend );

	// get the GPU busy with new commands
	frameTiming.startRenderTime = Sys_Microseconds();   // SRS - Added frame timing for out-of-sequence updates (e.g. used in timedemo "twice" mode)
	renderSystem->RenderCommandBuffers( cmd );
	frameTiming.finishRenderTime = Sys_Microseconds();  // SRS - Added frame timing for out-of-sequence updates (e.g. used in timedemo "twice" mode)

	insideUpdateScreen = false;
}
/*
================
idCommonLocal::ProcessGameReturn
================
*/
void idCommonLocal::ProcessGameReturn( const gameReturn_t& ret )
{
	// set joystick rumble
	if( in_useJoystick.GetBool() && in_joystickRumble.GetBool() && !game->Shell_IsActive() && session->GetSignInManager().GetMasterInputDevice() >= 0 )
	{
		Sys_SetRumble( session->GetSignInManager().GetMasterInputDevice(), ret.vibrationLow, ret.vibrationHigh );		// Only set the rumble on the active controller
	}
	else
	{
		for( int i = 0; i < MAX_INPUT_DEVICES; i++ )
		{
			Sys_SetRumble( i, 0, 0 );
		}
	}

	syncNextGameFrame = ret.syncNextGameFrame;

	if( ret.sessionCommand[0] )
	{
		idCmdArgs args;

		args.TokenizeString( ret.sessionCommand, false );

		if( !idStr::Icmp( args.Argv( 0 ), "map" ) )
		{
			MoveToNewMap( args.Argv( 1 ), false );
		}
		else if( !idStr::Icmp( args.Argv( 0 ), "devmap" ) )
		{
			MoveToNewMap( args.Argv( 1 ), true );
		}
		else if( !idStr::Icmp( args.Argv( 0 ), "died" ) )
		{
			if( !IsMultiplayer() )
			{
				game->Shell_Show( true );
			}
		}
		else if( !idStr::Icmp( args.Argv( 0 ), "disconnect" ) )
		{
			cmdSystem->BufferCommandText( CMD_EXEC_INSERT, "stoprecording ; disconnect" );
		}
		else if( !idStr::Icmp( args.Argv( 0 ), "endOfDemo" ) )
		{
			cmdSystem->BufferCommandText( CMD_EXEC_NOW, "endOfDemo" );
		}
	}
}

extern idCVar com_forceGenericSIMD;

extern idCVar com_pause;

/*
=================
idCommonLocal::Frame
=================
*/
void idCommonLocal::Frame()
{
	try
	{
		SCOPED_PROFILE_EVENT( "Common::Frame" );

		// This is the only place this is incremented
		idLib::frameNumber++;

		//OPTICK_TAG( "N", idLib::frameNumber );

		// allow changing SIMD usage on the fly
		if( com_forceGenericSIMD.IsModified() )
		{
			idSIMD::InitProcessor( "doom", com_forceGenericSIMD.GetBool() );
			com_forceGenericSIMD.ClearModified();
		}

		// RB begin
#if defined(USE_DOOMCLASSIC)
		// Do the actual switch between Doom 3 and the classics here so
		// that things don't get confused in the middle of the frame.
		PerformGameSwitch();
#endif
		// RB end

		// pump all the events
		Sys_GenerateEvents();

		// write config file if anything changed
		WriteConfiguration();

		eventLoop->RunEventLoop();

		renderSystem->OnFrame();

		// DG: prepare new ImGui frame - I guess this is a good place, as all new events should be available?
		ImGuiHook::NewFrame();

		// Activate the shell if it's been requested
		if( showShellRequested && game )
		{
			game->Shell_Show( true );
			showShellRequested = false;
		}

		// if the console or another gui is down, we don't need to hold the mouse cursor
		bool chatting = false;

		// DG: Add pause from com_pause cvar
		// RB begin
#if defined(USE_DOOMCLASSIC)
		if( com_pause.GetInteger() || console->Active() || Dialog().IsDialogActive() || session->IsSystemUIShowing()
				|| ( game && game->InhibitControls() && !IsPlayingDoomClassic() ) || ImGuiTools::ReleaseMouseForTools() )
#else
		if( com_pause.GetInteger() || console->Active() || Dialog().IsDialogActive() || session->IsSystemUIShowing()
				|| ( game && game->InhibitControls() ) ||  ImGuiTools::ReleaseMouseForTools() )
#endif
			// RB end, DG end
		{
			// RB: don't release the mouse when opening a PDA or menu
			// SRS - but always release at main menu after exiting game or demo
			if( console->Active() || !mapSpawned || ImGuiTools::ReleaseMouseForTools() )
			{
				Sys_GrabMouseCursor( false );
			}
			usercmdGen->InhibitUsercmd( INHIBIT_SESSION, true );
			chatting = true;
		}
		else
		{
			Sys_GrabMouseCursor( true );
			usercmdGen->InhibitUsercmd( INHIBIT_SESSION, false );
		}

		// RB begin
#if defined(USE_DOOMCLASSIC)
		const bool pauseGame = ( !mapSpawned
								 || ( !IsMultiplayer()
									  && ( Dialog().IsDialogPausing() || session->IsSystemUIShowing()
										   || ( game && game->Shell_IsActive() ) || com_pause.GetInteger() ) ) )
							   && !IsPlayingDoomClassic();
#else
		const bool pauseGame = ( !mapSpawned
								 || ( !IsMultiplayer()
									  && ( Dialog().IsDialogPausing() || session->IsSystemUIShowing()
										   || ( game && game->Shell_IsActive() ) || com_pause.GetInteger() ) ) );
#endif
		// RB end

		//--------------------------------------------
		// wait for the GPU to finish drawing
		//
		// It is imporant to minimize the time spent between this
		// section and the call to renderSystem->RenderCommandBuffers(),
		// because the GPU is completely idle.
		//--------------------------------------------
		// this should exit right after vsync, with the GPU idle and ready to draw
		// This may block if the GPU isn't finished renderng the previous frame.
		frameTiming.startSyncTime = Sys_Microseconds();
		const emptyCommand_t* renderCommands = NULL;

		// foresthale 2014-05-12: also check com_editors as many of them are not particularly thread-safe (editLights for example)
		if( com_smp.GetBool() && com_editors == 0 )
		{
			renderCommands = renderSystem->SwapCommandBuffers( &time_frontend, &time_backend, &time_shadows, &time_gpu, &stats_backend, &stats_frontend );
		}
		else
		{
			// the GPU will stay idle through command generation for minimal
			// input latency
			renderSystem->SwapCommandBuffers_FinishRendering( &time_frontend, &time_backend, &time_shadows, &time_gpu, &stats_backend, &stats_frontend );
		}
		frameTiming.finishSyncTime = Sys_Microseconds();

		//--------------------------------------------
		// Determine how many game tics we are going to run,
		// now that the previous frame is completely finished.
		//
		// It is important that any waiting on the GPU be done
		// before this, or there will be a bad stuttering when
		// dropping frames for performance management.
		//--------------------------------------------

		// input:
		// thisFrameTime
		// com_noSleep
		// com_engineHz
		// com_fixedTic
		// com_deltaTimeClamp
		// IsMultiplayer
		//
		// in/out state:
		// gameFrame
		// gameTimeResidual
		// lastFrameTime
		// syncNextFrame
		//
		// Output:
		// numGameFrames

		// How many game frames to run
		int numGameFrames = 0;

		{

			OPTICK_CATEGORY( "Wait for Frame", Optick::Category::Wait );

			for( ;; )
			{
				const int thisFrameTime = Sys_Milliseconds();
				static int lastFrameTime = thisFrameTime;	// initialized only the first time
				const int deltaMilliseconds = thisFrameTime - lastFrameTime;
				lastFrameTime = thisFrameTime;

				// if there was a large gap in time since the last frame, or the frame
				// rate is very very low, limit the number of frames we will run
				const int clampedDeltaMilliseconds = Min( deltaMilliseconds, com_deltaTimeClamp.GetInteger() );

				gameTimeResidual += clampedDeltaMilliseconds * timescale.GetFloat();

				// don't run any frames when paused
				// jpcy: the game is paused when playing a demo, but playDemo should wait like the game does
				// SRS - don't wait if window not in focus and playDemo itself paused
				if( pauseGame && ( !( readDemo && !timeDemo ) || session->IsSystemUIShowing() || com_pause.GetInteger() ) )
				{
					gameFrame++;
					gameTimeResidual = 0;
					break;
				}

				// debug cvar to force multiple game tics
				if( com_fixedTic.GetInteger() > 0 )
				{
					numGameFrames = com_fixedTic.GetInteger();
					gameFrame += numGameFrames;
					gameTimeResidual = 0;
					break;
				}

				if( syncNextGameFrame )
				{
					// don't sleep at all
					syncNextGameFrame = false;
					gameFrame++;
					numGameFrames++;
					gameTimeResidual = 0;
					break;
				}

				for( ;; )
				{
					// How much time to wait before running the next frame,
					// based on com_engineHz
					const int frameDelay = FRAME_TO_MSEC( gameFrame + 1 ) - FRAME_TO_MSEC( gameFrame );
					if( gameTimeResidual < frameDelay )
					{
						break;
					}
					gameTimeResidual -= frameDelay;
					gameFrame++;
					numGameFrames++;
					// if there is enough residual left, we may run additional frames
				}

				if( numGameFrames > 0 )
				{
					// ready to actually run them
					break;
				}

				// if we are vsyncing, we always want to run at least one game
				// frame and never sleep, which might happen due to scheduling issues
				// if we were just looking at real time.
				if( com_noSleep.GetBool() )
				{
					numGameFrames = 1;
					gameFrame += numGameFrames;
					gameTimeResidual = 0;
					break;
				}

				// not enough time has passed to run a frame, as might happen if
				// we don't have vsync on, or the monitor is running at 120hz while
				// com_engineHz is 60, so sleep a bit and check again
				Sys_Sleep( 0 );
			}
		}

		// jpcy: playDemo uses the game frame wait logic, but shouldn't run any game frames
		if( readDemo && !timeDemo )
		{
			numGameFrames = 0;
		}

		//--------------------------------------------
		// It would be better to push as much of this as possible
		// either before or after the renderSystem->SwapCommandBuffers(),
		// because the GPU is completely idle.
		//--------------------------------------------

		// Update session and syncronize to the new session state after sleeping
		session->UpdateSignInManager();
		session->Pump();
		session->ProcessSnapAckQueue();

		if( session->GetState() == idSession::LOADING )
		{
			// If the session reports we should be loading a map, load it!
			ExecuteMapChange();
			mapSpawnData.savegameFile = NULL;
			mapSpawnData.persistentPlayerInfo.Clear();
			return;
		}
		else if( session->GetState() != idSession::INGAME && mapSpawned )
		{
			// If the game is running, but the session reports we are not in a game, disconnect
			// This happens when a server disconnects us or we sign out
			LeaveGame();
			return;
		}

		if( mapSpawned && !pauseGame )
		{
			if( IsClient() )
			{
				RunNetworkSnapshotFrame();
			}
		}

		ExecuteReliableMessages();

		// send frame and mouse events to active guis
		GuiFrameEvents();

		// SRS - Advance demos inside Frame() vs. Draw() to support smp mode playback
		// SRS - Pause playDemo (but not timeDemo) when window not in focus
		if( readDemo && ( !( session->IsSystemUIShowing() || com_pause.GetInteger() ) || timeDemo ) )
		{
			AdvanceRenderDemo( true );
			if( !readDemo )
			{
				// SRS - Important to return after demo playback is finished to avoid command buffer sync issues
				return;
			}
		}

		//--------------------------------------------
		// Prepare usercmds and kick off the game processing
		// in a background thread
		//--------------------------------------------

		// get the previous usercmd for bypassed head tracking transform
		const usercmd_t	previousCmd = usercmdGen->GetCurrentUsercmd();

		// build a new usercmd
		int deviceNum = session->GetSignInManager().GetMasterInputDevice();
		usercmdGen->BuildCurrentUsercmd( deviceNum );
		if( deviceNum == -1 )
		{
			for( int i = 0; i < MAX_INPUT_DEVICES; i++ )
			{
				Sys_PollJoystickInputEvents( i );
				Sys_EndJoystickInputEvents();
			}
		}
		if( pauseGame )
		{
			usercmdGen->Clear();
		}

		usercmd_t newCmd = usercmdGen->GetCurrentUsercmd();

		// Store server game time - don't let time go past last SS time in case we are extrapolating
		if( IsClient() )
		{
			newCmd.serverGameMilliseconds = Min( Game()->GetServerGameTimeMs(), Game()->GetSSEndTime() );
		}
		else
		{
			newCmd.serverGameMilliseconds = Game()->GetServerGameTimeMs();
		}

		userCmdMgr.MakeReadPtrCurrentForPlayer( Game()->GetLocalClientNum() );

		// Stuff a copy of this userCmd for each game frame we are going to run.
		// Ideally, the usercmds would be built in another thread so you could
		// still get 60hz control accuracy when the game is running slower.
		for( int i = 0 ; i < numGameFrames ; i++ )
		{
			newCmd.clientGameMilliseconds = FRAME_TO_MSEC( gameFrame - numGameFrames + i + 1 );
			userCmdMgr.PutUserCmdForPlayer( game->GetLocalClientNum(), newCmd );
		}

		// RB begin
#if defined(USE_DOOMCLASSIC)
		// If we're in Doom or Doom 2, run tics and upload the new texture.
		// SRS - Add check for com_pause cvar to make sure window is in focus - if not classic game should be paused
		if( ( GetCurrentGame() == DOOM_CLASSIC || GetCurrentGame() == DOOM2_CLASSIC ) && !( Dialog().IsDialogPausing() || session->IsSystemUIShowing() || com_pause.GetInteger() ) )
		{
			RunDoomClassicFrame();
		}
#endif
		// RB end

		// start the game / draw command generation thread going in the background
		gameReturn_t ret = gameThread.RunGameAndDraw( numGameFrames, userCmdMgr, IsClient(), gameFrame - numGameFrames );

		frameTiming.startRenderTime = Sys_Microseconds();

		// foresthale 2014-05-12: also check com_editors as many of them are not particularly thread-safe (editLights for example)
		if( !com_smp.GetBool() || com_editors != 0 )
		{
			// in non-smp mode, run the commands we just generated, instead of
			// frame-delayed ones from a background thread
			renderCommands = renderSystem->SwapCommandBuffers_FinishCommandBuffers();
		}

		//----------------------------------------
		// Run the render back end, getting the GPU busy with new commands
		// ASAP to minimize the pipeline bubble.
		//----------------------------------------
		renderSystem->RenderCommandBuffers( renderCommands );
		if( com_sleepRender.GetInteger() > 0 )
		{
			// debug tool to test frame adaption
			Sys_Sleep( com_sleepRender.GetInteger() );
		}
		frameTiming.finishRenderTime = Sys_Microseconds();

		// SRS - Use finishSyncTime_EndFrame to record timing just before gameThread.WaitForThread() for com_smp = 1
		frameTiming.finishSyncTime_EndFrame = Sys_Microseconds();

		// make sure the game / draw thread has completed
		// This may block if the game is taking longer than the render back end
		gameThread.WaitForThread();

		// Send local usermds to the server.
		// This happens after the game frame has run so that prediction data is up to date.
		SendUsercmds( Game()->GetLocalClientNum() );

		// Now that we have an updated game frame, we can send out new snapshots to our clients
		session->Pump(); // Pump to get updated usercmds to relay
		SendSnapshots();

		// Render the sound system using the latest commands from the game thread
		// SRS - Enable sound during normal playDemo playback but not during timeDemo
		if( pauseGame && !( readDemo && !timeDemo ) )
		{
			soundWorld->Pause();
			soundSystem->SetPlayingSoundWorld( menuSoundWorld );
			soundSystem->SetMute( false );
		}
		else
		{
			soundWorld->UnPause();
			soundSystem->SetPlayingSoundWorld( soundWorld );
			soundSystem->SetMute( false );
		}
		// SRS - Mute all sound output when dialog waiting or window not in focus (mutes Doom3, Classic, Cinematic Audio)
		if( Dialog().IsDialogPausing() || session->IsSystemUIShowing() || com_pause.GetInteger() )
		{
			soundSystem->SetMute( true );
		}

		soundSystem->Render();

		// process the game return for map changes, etc
		ProcessGameReturn( ret );

		idLobbyBase& lobby = session->GetActivePlatformLobbyBase();
		if( lobby.HasActivePeers() )
		{
			if( net_drawDebugHud.GetInteger() == 1 )
			{
				lobby.DrawDebugNetworkHUD();
			}
			if( net_drawDebugHud.GetInteger() == 2 )
			{
				lobby.DrawDebugNetworkHUD2();
			}
			lobby.DrawDebugNetworkHUD_ServerSnapshotMetrics( net_drawDebugHud.GetInteger() == 3 );
		}

		// report timing information
		if( com_speeds.GetBool() )
		{
			static int lastTime = Sys_Milliseconds();
			int	nowTime = Sys_Milliseconds();
			int	com_frameMsec = nowTime - lastTime;
			lastTime = nowTime;
			Printf( "frame:%d all:%3d gfr:%3d rf:%3lld bk:%3lld\n", idLib::frameNumber, com_frameMsec, time_gameFrame, time_frontend / 1000, time_backend / 1000 );
			time_gameFrame = 0;
			time_gameDraw = 0;
		}

		// the FPU stack better be empty at this point or some bad code or compiler bug left values on the stack
		if( !Sys_FPU_StackIsEmpty() )
		{
			Printf( "%s", Sys_FPU_GetState() );
			FatalError( "idCommon::Frame: the FPU stack is not empty at the end of the frame\n" );
		}

		mainFrameTiming = frameTiming;

		session->GetSaveGameManager().Pump();
	}
	catch( idException& )
	{
		// an ERP_DROP was thrown
#if defined(USE_DOOMCLASSIC)
		if( currentGame == DOOM_CLASSIC || currentGame == DOOM2_CLASSIC )
		{
			return;
		}
#endif

		// kill loading gui
		delete loadGUI;
		loadGUI = NULL;

		// drop back to main menu
		LeaveGame();

		// force the console open to show error messages
		console->Open();
		return;
	}
}

// RB begin
#if defined(USE_DOOMCLASSIC)

/*
=================
idCommonLocal::RunDoomClassicFrame
=================
*/
void idCommonLocal::RunDoomClassicFrame()
{
	static int doomTics = 0;

	if( DoomLib::expansionDirty )
	{

		// re-Initialize the Doom Engine.
		DoomLib::Interface.Shutdown();
		DoomLib::Interface.Startup( 1, false );
		DoomLib::expansionDirty = false;
	}


	if( DoomLib::Interface.Frame( doomTics, &userCmdMgr ) )
	{
		Globals* data = ( Globals* )DoomLib::GetGlobalData( 0 );

		idArray< unsigned int, 256 > palette;
		std::copy( data->XColorMap, data->XColorMap + palette.Num(), palette.Ptr() );

		// Do the palette lookup.
		for( int row = 0; row < DOOMCLASSIC_RENDERHEIGHT; ++row )
		{
			for( int column = 0; column < DOOMCLASSIC_RENDERWIDTH; ++column )
			{
				const int doomScreenPixelIndex = row * DOOMCLASSIC_RENDERWIDTH + column;
				const byte paletteIndex = data->screens[0][doomScreenPixelIndex];
				const unsigned int paletteColor = palette[paletteIndex];
				const byte red = ( paletteColor & 0xFF000000 ) >> 24;
				const byte green = ( paletteColor & 0x00FF0000 ) >> 16;
				const byte blue = ( paletteColor & 0x0000FF00 ) >> 8;

				const int imageDataPixelIndex = row * DOOMCLASSIC_RENDERWIDTH * DOOMCLASSIC_BYTES_PER_PIXEL + column * DOOMCLASSIC_BYTES_PER_PIXEL;
				doomClassicImageData[imageDataPixelIndex]		= red;
				doomClassicImageData[imageDataPixelIndex + 1]	= green;
				doomClassicImageData[imageDataPixelIndex + 2]	= blue;
				doomClassicImageData[imageDataPixelIndex + 3]	= 255;
			}
		}
	}

	renderSystem->UploadImage( "_doomClassic", doomClassicImageData.Ptr(), DOOMCLASSIC_RENDERWIDTH, DOOMCLASSIC_RENDERHEIGHT );
	doomTics++;
}

#endif
// RB end

