/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2014-2016 Robert Beckebans
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

/*
================
FindUnusedFileName
================
*/
static idStr FindUnusedFileName( const char* format )
{
	idStr filename;

	for( int i = 0 ; i < 999 ; i++ )
	{
		filename.Format( format, i );
		int len = fileSystem->ReadFile( filename, NULL, NULL );
		if( len <= 0 )
		{
			return filename;	// file doesn't exist
		}
	}

	return filename;
}

//extern idCVar com_smp;            // SRS - No longer require non-smp mode for demos

void WriteDeclCache( idDemoFile* f, int demoCategory, int demoCode, declType_t  declType )
{
	f->WriteInt( demoCategory );
	f->WriteInt( demoCode );

	int numDecls = 0;

	for( int i = 0; i < declManager->GetNumDecls( declType ); i++ )
	{
		const idDecl* decl = declManager->DeclByIndex( declType, i, false );
		if( decl && decl->IsValid() )
		{
			++numDecls;
		}
	}

	f->WriteInt( numDecls );
	for( int i = 0; i < declManager->GetNumDecls( declType ); i++ )
	{
		const idDecl* decl = declManager->DeclByIndex( declType, i, false );
		if( decl && decl->IsValid() )
		{
			f->WriteHashString( decl->GetName() );
		}
	}
}

/*
================
idCommonLocal::StartRecordingRenderDemo
================
*/
void idCommonLocal::StartRecordingRenderDemo( const char* demoName )
{
	if( writeDemo )
	{
		// allow it to act like a toggle
		StopRecordingRenderDemo();
		return;
	}

	if( !demoName[0] )
	{
		common->Printf( "idCommonLocal::StartRecordingRenderDemo: no name specified\n" );
		return;
	}

	console->Close();

//  com_smp.SetInteger( 0 );        // SRS - No longer require non-smp mode for demos

	writeDemo = new( TAG_SYSTEM ) idDemoFile;
	if( !writeDemo->OpenForWriting( demoName ) )
	{
		common->Printf( "error opening %s\n", demoName );
		delete writeDemo;
		writeDemo = NULL;
		return;
	}

	common->Printf( "recording to %s\n", writeDemo->GetName() );

	writeDemo->WriteInt( DS_VERSION );
	writeDemo->WriteInt( RENDERDEMO_VERSION );

	// if we are in a map already, dump the current state
	soundWorld->StartWritingDemo( writeDemo );
	renderWorld->StartWritingDemo( writeDemo );
}

/*
================
idCommonLocal::StopRecordingRenderDemo
================
*/
void idCommonLocal::StopRecordingRenderDemo()
{
	if( !writeDemo )
	{
		common->Printf( "idCommonLocal::StopRecordingRenderDemo: not recording\n" );
		return;
	}
	soundWorld->StopWritingDemo();
	renderWorld->StopWritingDemo();

	writeDemo->Close();
	common->Printf( "stopped recording %s.\n", writeDemo->GetName() );
	delete writeDemo;
	writeDemo = NULL;
//	com_smp.SetInteger( 1 ); // motorsep 12-30-2014; turn multithreading back on;  SRS - No longer require non-smp mode for demos
}

/*
================
idCommonLocal::StopPlayingRenderDemo

Reports timeDemo numbers and finishes any avi recording
================
*/
void idCommonLocal::StopPlayingRenderDemo()
{
	if( !readDemo )
	{
		timeDemo = TD_NO;
		return;
	}

	// Record the stop time before doing anything that could be time consuming
	int timeDemoStopTime = Sys_Milliseconds();

	readDemo->Close();

	soundWorld->StopAllSounds();
	soundSystem->SetPlayingSoundWorld( menuSoundWorld );

	common->Printf( "stopped playing %s.\n", readDemo->GetName() );
	delete readDemo;
	readDemo = NULL;

	if( timeDemo )
	{
		// report the stats
		float	demoSeconds = ( timeDemoStopTime - timeDemoStartTime ) * 0.001f;
		float	demoFPS = numDemoFrames / demoSeconds;
		idStr	message = va( "%i frames rendered in %3.1f seconds = %3.1f fps\n", numDemoFrames, demoSeconds, demoFPS );

		common->Printf( message );
		if( timeDemo == TD_YES_THEN_QUIT )
		{
			cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "quit\n" );
		}
		timeDemo = TD_NO;
	}

//    com_smp.SetInteger( 1 ); // motorsep 12-30-2014; turn multithreading back on;  SRS - No longer require non-smp mode for demos
}

/*
================
idCommonLocal::DemoShot

A demoShot is a single frame demo
================
*/
void idCommonLocal::DemoShot( const char* demoName )
{
	StartRecordingRenderDemo( demoName );

	// force draw one frame
	const bool captureToImage = false;
	UpdateScreen( captureToImage );

	StopRecordingRenderDemo();
}

/*
================
idCommonLocal::StartPlayingRenderDemo
================
*/
void idCommonLocal::StartPlayingRenderDemo( idStr demoName )
{
	if( !demoName[0] )
	{
		common->Printf( "idCommonLocal::StartPlayingRenderDemo: no name specified\n" );
		return;
	}

//  com_smp.SetInteger( 0 );        // SRS - No longer require non-smp mode for demos

	// make sure localSound / GUI intro music shuts up
	soundWorld->StopAllSounds();
	soundWorld->PlayShaderDirectly( "", 0 );
	menuSoundWorld->StopAllSounds();
	menuSoundWorld->PlayShaderDirectly( "", 0 );

	// exit any current game
	Stop();

	// automatically put the console away
	console->Close();

	readDemo = new( TAG_SYSTEM ) idDemoFile;
	demoName.DefaultFileExtension( ".demo" );
	if( !readDemo->OpenForReading( demoName ) )
	{
		common->Printf( "couldn't open %s\n", demoName.c_str() );
		delete readDemo;
		readDemo = NULL;

		CreateMainMenu();                   // SRS - drop back to main menu if demo playback fails
		StartMenu();
		return;
	}

	int opcode = -1, demoVersion = -1;
	readDemo->ReadInt( opcode );
	if( opcode != DS_VERSION )
	{
		common->Printf( "StartPlayingRenderDemo invalid demo file\n" );
		readDemo->Close();
		delete readDemo;
		readDemo = NULL;

		CreateMainMenu();                   // SRS - drop back to main menu if demo playback fails
		StartMenu();
		return;
	}

	readDemo->ReadInt( demoVersion );
	if( demoVersion != RENDERDEMO_VERSION )
	{
		common->Printf( "StartPlayingRenderDemo got version %d, expected version %d\n", demoVersion, RENDERDEMO_VERSION );
		readDemo->Close();
		delete readDemo;
		readDemo = NULL;

		CreateMainMenu();                   // SRS - drop back to main menu if demo playback fails
		StartMenu();
		return;
	}

	numDemoFrames = 0;                      // SRS - Moved ahead of first call to AdvanceRenderDemo to properly handle demoshots
	numShotFrames = 0;                      // SRS - Initialize count of demoShot frames to play before timeout to main menu

	renderSystem->BeginLevelLoad();         // SRS - Free static data from previous level before loading demo assets
	soundSystem->BeginLevelLoad();          // SRS - Free sound media from previous level before loading demo assets
	declManager->BeginLevelLoad();          // SRS - Clear declaration manager data before loading demo assets
	uiManager->BeginLevelLoad();            // SRS - Clear gui manager data before loading demo assets

	AdvanceRenderDemo( true );              // SRS - Call AdvanceRenderDemo() once to load map and initial assets (like level load)

	renderSystem->EndLevelLoad();           // SRS - Define static data for use by RB_StencilShadowPass if stencil shadows enabled
	soundSystem->EndLevelLoad();
	declManager->EndLevelLoad();
	uiManager->EndLevelLoad( "" );          // SRS - FIXME: No gui assets are currently saved/reloaded in demo file, fix later?

	Game()->StartDemoPlayback( renderWorld );

	renderWorld->GenerateAllInteractions();

	soundSystem->SetPlayingSoundWorld( soundWorld );

	timeDemoStartTime = Sys_Milliseconds();
}

/*
================
idCommonLocal::TimeRenderDemo
================
*/
void idCommonLocal::TimeRenderDemo( const char* demoName, bool twice, bool quit )
{
	extern idCVar com_smp;
	idStr demo = demoName;

	StartPlayingRenderDemo( demo );

	if( twice && readDemo )
	{
		timeDemo = TD_YES;                      // SRS - Set timeDemo to TD_YES to disable time demo playback pause when window not in focus

		int smp_mode = com_smp.GetInteger();
		com_smp.SetInteger( 0 );                // SRS - First pass of timedemo is effectively in com_smp == 0 mode, so set this for ImGui timings to be correct

		while( readDemo )
		{
			BusyWait();                         // SRS - BusyWait() calls UpdateScreen() which draws and renders out-of-sequence but still supports frame timing
			commonLocal.frameTiming.finishSyncTime_EndFrame = Sys_Microseconds();
			commonLocal.mainFrameTiming = commonLocal.frameTiming;
			// ** End of current logical frame **

			// ** Start of next logical frame **
			commonLocal.frameTiming.startSyncTime = Sys_Microseconds();
			commonLocal.frameTiming.finishSyncTime = commonLocal.frameTiming.startSyncTime;
			commonLocal.frameTiming.startGameTime = commonLocal.frameTiming.finishSyncTime;

			AdvanceRenderDemo( true );          // SRS - Advance demo commands to manually run the next game frame during first pass of the timedemo
			commonLocal.frameTiming.finishGameTime = Sys_Microseconds();

			eventLoop->RunEventLoop( false );   // SRS - Run event loop (with no commands) to allow keyboard escape to cancel first pass of the timedemo
		}

		com_smp.SetInteger( smp_mode );         // SRS - Restore original com_smp mode before second pass of timedemo which runs within normal rendering loop

		StartPlayingRenderDemo( demo );
	}


	if( !readDemo )
	{
		timeDemo = TD_NO;                       // SRS - Make sure timeDemo flag is off if readDemo is NULL
		return;
	}

	if( quit )
	{
		// this allows hardware vendors to automate some testing
		timeDemo = TD_YES_THEN_QUIT;
	}
	else
	{
		timeDemo = TD_YES;
	}
}

/*
================
idCommonLocal::CompressDemoFile
================
*/
void idCommonLocal::CompressDemoFile( const char* scheme, const char* demoName )
{
	idStr	fullDemoName = "demos/";
	fullDemoName += demoName;
	fullDemoName.DefaultFileExtension( ".demo" );
	idStr compressedName = fullDemoName;
	compressedName.StripFileExtension();
	compressedName.Append( "_compressed.demo" );

	int savedCompression = cvarSystem->GetCVarInteger( "com_compressDemos" );
	bool savedPreload = cvarSystem->GetCVarBool( "com_preloadDemos" );
	cvarSystem->SetCVarBool( "com_preloadDemos", false );
	cvarSystem->SetCVarInteger( "com_compressDemos", atoi( scheme ) );

	idDemoFile demoread, demowrite;
	if( !demoread.OpenForReading( fullDemoName ) )
	{
		common->Printf( "Could not open %s for reading\n", fullDemoName.c_str() );
		return;
	}
	if( !demowrite.OpenForWriting( compressedName ) )
	{
		common->Printf( "Could not open %s for writing\n", compressedName.c_str() );
		demoread.Close();
		cvarSystem->SetCVarBool( "com_preloadDemos", savedPreload );
		cvarSystem->SetCVarInteger( "com_compressDemos", savedCompression );
		return;
	}
	common->SetRefreshOnPrint( true );
	common->Printf( "Compressing %s to %s...\n", fullDemoName.c_str(), compressedName.c_str() );

	static const int bufferSize = 65535;
	char buffer[bufferSize];
	int bytesRead;
	while( 0 != ( bytesRead = demoread.Read( buffer, bufferSize ) ) )
	{
		demowrite.Write( buffer, bytesRead );
		common->Printf( "." );
	}

	demoread.Close();
	demowrite.Close();

	cvarSystem->SetCVarBool( "com_preloadDemos", savedPreload );
	cvarSystem->SetCVarInteger( "com_compressDemos", savedCompression );

	common->Printf( "Done\n" );
	common->SetRefreshOnPrint( false );

}

/*
===============
idCommonLocal::AdvanceRenderDemo
===============
*/
void idCommonLocal::AdvanceRenderDemo( bool singleFrameOnly )
{
	while( true )
	{
		int	ds = DS_FINISHED;
		readDemo->ReadInt( ds );

		switch( ds )
		{
			case DS_FINISHED:
				if( numDemoFrames == 1 )
				{
					// if the demo has a single frame (a demoShot), continuously replay
					// the renderView that has already been read
					if( numShotFrames++ < com_engineHz_latched * 10 )   // SRS - play demoShot for min 10 sec then timeout
					{
						return;
					}
				}
				LeaveGame();                                            // SRS - drop back to main menu after demo playback is finished
				return;
			case DS_RENDER:
				if( renderWorld->ProcessDemoCommand( readDemo, &currentDemoRenderView, &demoTimeOffset ) )
				{
					// a view is ready to render
					numDemoFrames++;
					return;
				}
				break;
			case DS_SOUND:
				soundWorld->ProcessDemoCommand( readDemo );
				break;
			case DS_GAME:
				Game()->ProcessDemoCommand( readDemo );
				break;
			default:
				common->Error( "Bad render demo token %d", ds );
		}
	}
}

// SRS - Changed macro from CONSOLE_COMMAND to CONSOLE_COMMAND_SHIP for demo-related commands - i.e. include in release builds

/*
================
Common_DemoShot_f
================
*/
CONSOLE_COMMAND_SHIP( demoShot, "writes a screenshot as a demo", NULL )
{
	if( args.Argc() != 2 )
	{
		idStr filename = FindUnusedFileName( "demos/shot%03i.demo" );
		commonLocal.DemoShot( filename );
	}
	else
	{
		commonLocal.DemoShot( va( "demos/shot_%s.demo", args.Argv( 1 ) ) );
	}
}

/*
================
Common_RecordDemo_f
================
*/
CONSOLE_COMMAND_SHIP( recordDemo, "records a demo", NULL )
{
	if( args.Argc() != 2 )
	{
		idStr filename = FindUnusedFileName( "demos/demo%03i.demo" );
		commonLocal.StartRecordingRenderDemo( filename );
	}
	else
	{
		commonLocal.StartRecordingRenderDemo( va( "demos/%s.demo", args.Argv( 1 ) ) );
	}
}

/*
================
Common_CompressDemo_f
================
*/
CONSOLE_COMMAND_SHIP( compressDemo, "compresses a demo file", idCmdSystem::ArgCompletion_DemoName )
{
	if( args.Argc() == 2 )
	{
		commonLocal.CompressDemoFile( "2", args.Argv( 1 ) );
	}
	else if( args.Argc() == 3 )
	{
		commonLocal.CompressDemoFile( args.Argv( 2 ), args.Argv( 1 ) );
	}
	else
	{
		common->Printf( "use: CompressDemo <file> [scheme]\nscheme is the same as com_compressDemo, defaults to 2" );
	}
}

/*
================
Common_StopRecordingDemo_f
================
*/
CONSOLE_COMMAND_SHIP( stopRecording, "stops demo recording", NULL )
{
	commonLocal.StopRecordingRenderDemo();
}

/*
================
Common_PlayDemo_f
================
*/
CONSOLE_COMMAND_SHIP( playDemo, "plays back a demo", idCmdSystem::ArgCompletion_DemoName )
{
	if( args.Argc() >= 2 )
	{
		commonLocal.StartPlayingRenderDemo( va( "demos/%s", args.Argv( 1 ) ) );
	}
}

/*
================
Common_TimeDemo_f
================
*/
CONSOLE_COMMAND_SHIP( timeDemo, "times a demo", idCmdSystem::ArgCompletion_DemoName )
{
	if( args.Argc() >= 2 )
	{
		commonLocal.TimeRenderDemo( va( "demos/%s", args.Argv( 1 ) ), ( args.Argc() > 2 ), false );
	}
}

/*
================
Common_TimeDemoQuit_f
================
*/
CONSOLE_COMMAND_SHIP( timeDemoQuit, "times a demo and quits", idCmdSystem::ArgCompletion_DemoName )
{
	commonLocal.TimeRenderDemo( va( "demos/%s", args.Argv( 1 ) ), ( args.Argc() > 2 ), true );    // SRS - fixed missing "twice" argument
}