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
#pragma hdrstop
#include "tr_local.h"

/*
================================================================================================
Contains the RenderLog implementation.

TODO:	Emit statistics to the logfile at the end of views and frames.
================================================================================================
*/

idCVar r_logLevel( "r_logLevel", "2", CVAR_INTEGER, "1 = blocks only, 2 = everything", 1, 2 );

static const int LOG_LEVEL_BLOCKS_ONLY	= 1;
static const int LOG_LEVEL_EVERYTHING	= 2;

const char * renderLogMainBlockLabels[] = {
	ASSERT_ENUM_STRING( MRB_NONE,							0 ),
	ASSERT_ENUM_STRING( MRB_BEGIN_DRAWING_VIEW,				1 ),
	ASSERT_ENUM_STRING( MRB_FILL_DEPTH_BUFFER,				2 ),
	ASSERT_ENUM_STRING( MRB_DRAW_INTERACTIONS,				3 ),
	ASSERT_ENUM_STRING( MRB_DRAW_SHADER_PASSES,				4 ),
	ASSERT_ENUM_STRING( MRB_FOG_ALL_LIGHTS,					5 ),
	ASSERT_ENUM_STRING( MRB_DRAW_SHADER_PASSES_POST,		6 ),
	ASSERT_ENUM_STRING( MRB_DRAW_DEBUG_TOOLS,				7 ),
	ASSERT_ENUM_STRING( MRB_CAPTURE_COLORBUFFER,			8 ),
	ASSERT_ENUM_STRING( MRB_POSTPROCESS,					9 ),
	ASSERT_ENUM_STRING( MRB_GPU_SYNC,						10 ),
	ASSERT_ENUM_STRING( MRB_END_FRAME,						11 ),
	ASSERT_ENUM_STRING( MRB_BINK_FRAME,						12 ),
	ASSERT_ENUM_STRING( MRB_BINK_NEXT_FRAME,				13 ),
	ASSERT_ENUM_STRING( MRB_TOTAL,							14 ),
	ASSERT_ENUM_STRING( MRB_MAX,							15 )
};

extern uint64 Sys_Microseconds();
/*
================================================================================================

PIX events on all platforms

================================================================================================
*/


/*
================================================
pixEvent_t 
================================================
*/
struct pixEvent_t {
	char		name[256];
	uint64		cpuTime;
	uint64		gpuTime;
};

idCVar r_pix( "r_pix", "0", CVAR_INTEGER, "print GPU/CPU event timing" );

static const int	MAX_PIX_EVENTS = 256;
// defer allocation of this until needed, so we don't waste lots of memory
pixEvent_t *		pixEvents;	// [MAX_PIX_EVENTS]
int					numPixEvents;
int					numPixLevels;
static GLuint		timeQueryIds[MAX_PIX_EVENTS];

/*
========================
PC_BeginNamedEvent

FIXME: this is not thread safe on the PC
========================
*/
void PC_BeginNamedEvent( const char *szName, ... ) {
#if 0
	if ( !r_pix.GetBool() ) {
		return;
	}
	if ( !pixEvents ) {
		// lazy allocation to not waste memory
		pixEvents = (pixEvent_t *)Mem_ClearedAlloc( sizeof( *pixEvents ) * MAX_PIX_EVENTS, TAG_CRAP );
	}
	if ( numPixEvents >= MAX_PIX_EVENTS ) {
		idLib::FatalError( "PC_BeginNamedEvent: event overflow" );
	}
	if ( ++numPixLevels > 1 ) {
		return;	// only get top level timing information
	}
	if ( !qglGetQueryObjectui64vEXT ) {
		return;
	}

	GL_CheckErrors();
	if ( timeQueryIds[0] == 0 ) {
		qglGenQueriesARB( MAX_PIX_EVENTS, timeQueryIds );
	}
	qglFinish();
	qglBeginQueryARB( GL_TIME_ELAPSED_EXT, timeQueryIds[numPixEvents] );
	GL_CheckErrors();

	pixEvent_t *ev = &pixEvents[numPixEvents++];
	strncpy( ev->name, szName, sizeof( ev->name ) - 1 );
	ev->cpuTime = Sys_Microseconds();
#endif
}

/*
========================
PC_EndNamedEvent
========================
*/
void PC_EndNamedEvent() {
#if 0
	if ( !r_pix.GetBool() ) {
		return;
	}
	if ( numPixLevels <= 0 ) {
		idLib::FatalError( "PC_EndNamedEvent: level underflow" );
	}
	if ( --numPixLevels > 0 ) {
		// only do timing on top level events
		return;
	}
	if ( !qglGetQueryObjectui64vEXT ) {
		return;
	}

	pixEvent_t *ev = &pixEvents[numPixEvents-1];
	ev->cpuTime = Sys_Microseconds() - ev->cpuTime;

	GL_CheckErrors();
	qglEndQueryARB( GL_TIME_ELAPSED_EXT );
	GL_CheckErrors();
#endif
}

/*
========================
PC_EndFrame
========================
*/
void PC_EndFrame() {
#if 0
	if ( !r_pix.GetBool() ) {
		return;
	}

	int64 totalGPU = 0;
	int64 totalCPU = 0;

	idLib::Printf( "----- GPU Events -----\n" );
	for ( int i = 0 ; i < numPixEvents ; i++ ) {
		pixEvent_t *ev = &pixEvents[i];

		int64 gpuTime = 0;
		qglGetQueryObjectui64vEXT( timeQueryIds[i], GL_QUERY_RESULT, (GLuint64EXT *)&gpuTime );
		ev->gpuTime = gpuTime;

		idLib::Printf( "%2d: %1.2f (GPU) %1.3f (CPU) = %s\n", i, ev->gpuTime / 1000000.0f, ev->cpuTime / 1000.0f, ev->name );
		totalGPU += ev->gpuTime;
		totalCPU += ev->cpuTime;
	}
	idLib::Printf( "%2d: %1.2f (GPU) %1.3f (CPU) = total\n", numPixEvents, totalGPU / 1000000.0f, totalCPU / 1000.0f );
	memset( pixEvents, 0, numPixLevels * sizeof( pixEvents[0] ) );
	
	numPixEvents = 0;
	numPixLevels = 0;
#endif
}


/*
================================================================================================

idRenderLog

================================================================================================
*/

idRenderLog	renderLog;

#if !defined( STUB_RENDER_LOG )

/*
========================
idRenderLog::idRenderLog
========================
*/
idRenderLog::idRenderLog() {
	activeLevel = 0;
	indentString[0] = '\0';
	indentLevel = 0;
	logFile = NULL;

	frameStartTime = 0;
	closeBlockTime = 0;
	logLevel = 0;
}

/*
========================
idRenderLog::StartFrame
========================
*/
void idRenderLog::StartFrame() {
	if ( r_logFile.GetInteger() == 0 ) {
		return;
	}

	// open a new logfile
	indentLevel = 0;
	indentString[0] = '\0';
	activeLevel = r_logLevel.GetInteger();

	struct tm		*newtime;
	time_t			aclock;

	char ospath[ MAX_OSPATH ];

	char qpath[128];
	sprintf( qpath, "renderlogPC_%04i.txt", r_logFile.GetInteger() );
	idStr finalPath = fileSystem->RelativePathToOSPath( qpath );		
	sprintf( ospath, "%s", finalPath.c_str() );
	/*
	for ( int i = 0; i < 9999 ; i++ ) {
		char qpath[128];
		sprintf( qpath, "renderlog_%04i.txt", r_logFile.GetInteger() );
		idStr finalPath = fileSystem->RelativePathToOSPath( qpath );
		fileSystem->RelativePathToOSPath( qpath, ospath, MAX_OSPATH ,FSPATH_BASE );
		if ( !fileSystem->FileExists( finalPath.c_str() ) ) {
			break; // use this name
		}
	}
	*/

	common->SetRefreshOnPrint( false );	// problems are caused if this print causes a refresh...

	if ( logFile != NULL ) {
		fileSystem->CloseFile( logFile );
		logFile = NULL;
	}

	logFile = fileSystem->OpenFileWrite( ospath );	
	if ( logFile == NULL ) {
		idLib::Warning( "Failed to open logfile %s", ospath );
		return;
	}
	idLib::Printf( "Opened logfile %s\n", ospath );

	// write the time out to the top of the file
	time( &aclock );
	newtime = localtime( &aclock );
	const char *str = asctime( newtime );
	logFile->Printf( "// %s", str );
	logFile->Printf( "// %s\n\n", com_version.GetString() );

	frameStartTime = Sys_Microseconds();
	closeBlockTime = frameStartTime;
	OpenBlock( "Frame" );
}

/*
========================
idRenderLog::EndFrame
========================
*/
void idRenderLog::EndFrame() {
	PC_EndFrame();

	if ( logFile != NULL ) {
		if ( r_logFile.GetInteger() == 1 ) {
			Close();
		}
		// log is open, so decrement r_logFile and stop if it is zero
		r_logFile.SetInteger( r_logFile.GetInteger() - 1 );
		idLib::Printf( "Frame logged.\n" );
		return;
	}
}

/*
========================
idRenderLog::Close
========================
*/
void idRenderLog::Close() {
	if ( logFile != NULL ) {
		CloseBlock();
		idLib::Printf( "Closing logfile\n" );
		fileSystem->CloseFile( logFile );
		logFile = NULL;
		activeLevel = 0;
	}
}

/*
========================
idRenderLog::OpenMainBlock
========================
*/
void idRenderLog::OpenMainBlock( renderLogMainBlock_t block ) {
}

/*
========================
idRenderLog::CloseMainBlock
========================
*/
void idRenderLog::CloseMainBlock() {
}

/*
========================
idRenderLog::OpenBlock
========================
*/
void idRenderLog::OpenBlock( const char *label ) {
	// Allow the PIX functionality even when logFile is not running.
	PC_BeginNamedEvent( label );

	if ( logFile != NULL ) {
		LogOpenBlock( RENDER_LOG_INDENT_MAIN_BLOCK, label, NULL );
	}
}

/*
========================
idRenderLog::CloseBlock
========================
*/
void idRenderLog::CloseBlock() {
	PC_EndNamedEvent();

	if ( logFile != NULL ) {
		LogCloseBlock( RENDER_LOG_INDENT_MAIN_BLOCK );
	}
}

/*
========================
idRenderLog::Printf
========================
*/
void idRenderLog::Printf( const char *fmt, ... ) {
	if ( activeLevel <= LOG_LEVEL_BLOCKS_ONLY ) {
		return;
	}

	if ( logFile == NULL ) {
		return;
	}

	va_list marker;
	logFile->Printf( "%s", indentString );
	va_start( marker, fmt );
	logFile->VPrintf( fmt, marker );
	va_end( marker );
//	logFile->Flush();		this makes it take waaaay too long
}

/*
========================
idRenderLog::LogOpenBlock
========================
*/
void idRenderLog::LogOpenBlock( renderLogIndentLabel_t label, const char * fmt, va_list args ) {

	uint64 now = Sys_Microseconds();

	if ( logFile != NULL ) {
		if ( now - closeBlockTime >= 1000 ) {
			logFile->Printf( "%s%1.1f msec gap from last closeblock\n", indentString, ( now - closeBlockTime ) * ( 1.0f / 1000.0f ) );
		}
		logFile->Printf( "%s", indentString );
		logFile->VPrintf( fmt, args );
		logFile->Printf( " {\n" );
	}

	Indent( label );

	if ( logLevel >= MAX_LOG_LEVELS ) {
		idLib::Warning( "logLevel %d >= MAX_LOG_LEVELS", logLevel );
	}


	logLevel++;
}

/*
========================
idRenderLog::LogCloseBlock
========================
*/
void idRenderLog::LogCloseBlock( renderLogIndentLabel_t label ) {
	closeBlockTime = Sys_Microseconds();

	assert( logLevel > 0 );
	logLevel--;

	Outdent( label );

	if ( logFile != NULL ) {
	}
}

#else	// !STUB_RENDER_LOG

/*
========================
idRenderLog::OpenBlock
========================
*/
void idRenderLog::OpenBlock( const char *label ) {
	PC_BeginNamedEvent( label );
}

/*
========================
idRenderLog::CloseBlock
========================
*/
void idRenderLog::CloseBlock() {
	PC_EndNamedEvent();
}

#endif // !STUB_RENDER_LOG
