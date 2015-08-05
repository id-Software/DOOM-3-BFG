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
	
	EndAVICapture();
	
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
		Stop();
		StartMenu();
		return;
	}
	
	const bool captureToImage = false;
	UpdateScreen( captureToImage );
	
	AdvanceRenderDemo( true );
	
	numDemoFrames = 1;
	
	timeDemoStartTime = Sys_Milliseconds();
}

/*
================
idCommonLocal::TimeRenderDemo
================
*/
void idCommonLocal::TimeRenderDemo( const char* demoName, bool twice, bool quit )
{
	idStr demo = demoName;
	
	StartPlayingRenderDemo( demo );
	
	if( twice && readDemo )
	{
		while( readDemo )
		{
			const bool captureToImage = false;
			UpdateScreen( captureToImage );
			AdvanceRenderDemo( true );
		}
		
		StartPlayingRenderDemo( demo );
	}
	
	
	if( !readDemo )
	{
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
idCommonLocal::BeginAVICapture
================
*/
void idCommonLocal::BeginAVICapture( const char* demoName )
{
	idStr name = demoName;
	name.ExtractFileBase( aviDemoShortName );
	aviCaptureMode = true;
	aviDemoFrameCount = 0;
	soundWorld->AVIOpen( va( "demos/%s/", aviDemoShortName.c_str() ), aviDemoShortName.c_str() );
}

/*
================
idCommonLocal::EndAVICapture
================
*/
void idCommonLocal::EndAVICapture()
{
	if( !aviCaptureMode )
	{
		return;
	}
	
	soundWorld->AVIClose();
	
	// write a .roqParam file so the demo can be converted to a roq file
	idFile* f = fileSystem->OpenFileWrite( va( "demos/%s/%s.roqParam",
										   aviDemoShortName.c_str(), aviDemoShortName.c_str() ) );
	f->Printf( "INPUT_DIR demos/%s\n", aviDemoShortName.c_str() );
	f->Printf( "FILENAME demos/%s/%s.RoQ\n", aviDemoShortName.c_str(), aviDemoShortName.c_str() );
	f->Printf( "\nINPUT\n" );
	f->Printf( "%s_*.tga [00000-%05i]\n", aviDemoShortName.c_str(), ( int )( aviDemoFrameCount - 1 ) );
	f->Printf( "END_INPUT\n" );
	delete f;
	
	common->Printf( "captured %i frames for %s.\n", ( int )aviDemoFrameCount, aviDemoShortName.c_str() );
	
	aviCaptureMode = false;
}


/*
================
idCommonLocal::AVIRenderDemo
================
*/
void idCommonLocal::AVIRenderDemo( const char* _demoName )
{
	idStr	demoName = _demoName;	// copy off from va() buffer
	
	StartPlayingRenderDemo( demoName );
	if( !readDemo )
	{
		return;
	}
	
	BeginAVICapture( demoName.c_str() ) ;
	
	// I don't understand why I need to do this twice, something
	// strange with the nvidia swapbuffers?
	const bool captureToImage = false;
	UpdateScreen( captureToImage );
}

/*
================
idCommonLocal::AVIGame

Start AVI recording the current game session
================
*/
void idCommonLocal::AVIGame( const char* demoName )
{
	if( aviCaptureMode )
	{
		EndAVICapture();
		return;
	}
	
	if( !mapSpawned )
	{
		common->Printf( "No map spawned.\n" );
	}
	
	if( !demoName || !demoName[0] )
	{
		idStr filename = FindUnusedFileName( "demos/game%03i.game" );
		demoName = filename.c_str();
		
		// write a one byte stub .game file just so the FindUnusedFileName works,
		fileSystem->WriteFile( demoName, demoName, 1 );
	}
	
	BeginAVICapture( demoName ) ;
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
	int	ds = DS_FINISHED;
	readDemo->ReadInt( ds );
	
	switch( ds )
	{
		case DS_FINISHED:
			if( numDemoFrames != 1 )
			{
				// if the demo has a single frame (a demoShot), continuously replay
				// the renderView that has already been read
				Stop();
				StartMenu();
			}
			return;
		case DS_RENDER:
			if( renderWorld->ProcessDemoCommand( readDemo, &currentDemoRenderView, &demoTimeOffset ) )
			{
				// a view is ready to render
				numDemoFrames++;
			}
			break;
		case DS_SOUND:
			soundWorld->ProcessDemoCommand( readDemo );
			break;
		default:
			common->Error( "Bad render demo token" );
	}
}

/*
================
Common_DemoShot_f
================
*/
CONSOLE_COMMAND( demoShot, "writes a screenshot as a demo", NULL )
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
CONSOLE_COMMAND( recordDemo, "records a demo", NULL )
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
CONSOLE_COMMAND( compressDemo, "compresses a demo file", idCmdSystem::ArgCompletion_DemoName )
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
CONSOLE_COMMAND( stopRecording, "stops demo recording", NULL )
{
	commonLocal.StopRecordingRenderDemo();
}

/*
================
Common_PlayDemo_f
================
*/
CONSOLE_COMMAND( playDemo, "plays back a demo", idCmdSystem::ArgCompletion_DemoName )
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
CONSOLE_COMMAND( timeDemo, "times a demo", idCmdSystem::ArgCompletion_DemoName )
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
CONSOLE_COMMAND( timeDemoQuit, "times a demo and quits", idCmdSystem::ArgCompletion_DemoName )
{
	commonLocal.TimeRenderDemo( va( "demos/%s", args.Argv( 1 ) ), true );
}

/*
================
Common_AVIDemo_f
================
*/
CONSOLE_COMMAND( aviDemo, "writes AVIs for a demo", idCmdSystem::ArgCompletion_DemoName )
{
	commonLocal.AVIRenderDemo( va( "demos/%s", args.Argv( 1 ) ) );
}

/*
================
Common_AVIGame_f
================
*/
CONSOLE_COMMAND( aviGame, "writes AVIs for the current game", NULL )
{
	commonLocal.AVIGame( args.Argv( 1 ) );
}
