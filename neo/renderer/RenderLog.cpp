/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2020 Robert Beckebans

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
#include "RenderCommon.h"

/*
================================================================================================
Contains the RenderLog implementation.

TODO:	Emit statistics to the logfile at the end of views and frames.
================================================================================================
*/

idCVar r_logLevel( "r_logLevel", "2", CVAR_INTEGER, "1 = blocks only, 2 = everything", 1, 2 );

static const int LOG_LEVEL_BLOCKS_ONLY	= 1;
static const int LOG_LEVEL_EVERYTHING	= 2;

const char* renderLogMainBlockLabels[] =
{
	ASSERT_ENUM_STRING( MRB_GPU_TIME,						0 ),
	ASSERT_ENUM_STRING( MRB_BEGIN_DRAWING_VIEW,				1 ),
	ASSERT_ENUM_STRING( MRB_FILL_DEPTH_BUFFER,				2 ),
	ASSERT_ENUM_STRING( MRB_FILL_GEOMETRY_BUFFER,			3 ), // RB
	ASSERT_ENUM_STRING( MRB_SSAO_PASS,						4 ), // RB
	ASSERT_ENUM_STRING( MRB_AMBIENT_PASS,					5 ), // RB
	ASSERT_ENUM_STRING( MRB_DRAW_INTERACTIONS,				6 ),
	ASSERT_ENUM_STRING( MRB_DRAW_SHADER_PASSES,				7 ),
	ASSERT_ENUM_STRING( MRB_FOG_ALL_LIGHTS,					8 ),
	ASSERT_ENUM_STRING( MRB_DRAW_SHADER_PASSES_POST,		9 ),
	ASSERT_ENUM_STRING( MRB_DRAW_DEBUG_TOOLS,				10 ),
	ASSERT_ENUM_STRING( MRB_CAPTURE_COLORBUFFER,			11 ),
	ASSERT_ENUM_STRING( MRB_POSTPROCESS,					12 ),
	ASSERT_ENUM_STRING( MRB_TOTAL,							13 )
};

#if defined( USE_VULKAN )
	compile_time_assert( NUM_TIMESTAMP_QUERIES >= ( MRB_TOTAL_QUERIES ) );
#endif

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
struct pixEvent_t
{
	char		name[256];
	uint64		cpuTime;
	uint64		gpuTime;
};

idCVar r_pix( "r_pix", "0", CVAR_INTEGER, "print GPU/CPU event timing" );

#if !defined( USE_VULKAN )
	static const int	MAX_PIX_EVENTS = 256;
	// defer allocation of this until needed, so we don't waste lots of memory
	pixEvent_t* 		pixEvents;	// [MAX_PIX_EVENTS]
	int					numPixEvents;
	int					numPixLevels;
	static GLuint		timeQueryIds[MAX_PIX_EVENTS];
#endif

/*
========================
PC_BeginNamedEvent

FIXME: this is not thread safe on the PC
========================
*/
void PC_BeginNamedEvent( const char* szName, const idVec4& color )
{
#if defined( USE_VULKAN )

	// start an annotated group of calls under the this name
	if( vkcontext.debugMarkerSupportAvailable )
	{
		VkDebugMarkerMarkerInfoEXT  label = {};
		label.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
		label.pMarkerName = szName;
		label.color[0] = color.x;
		label.color[1] = color.y;
		label.color[2] = color.z;
		label.color[3] = color.w;

		qvkCmdDebugMarkerBeginEXT( vkcontext.commandBuffer[ vkcontext.frameParity ], &label );
	}
	else if( vkcontext.debugUtilsSupportAvailable )
	{
		VkDebugUtilsLabelEXT label = {};
		label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
		label.pLabelName = szName;
		label.color[0] = color.x;
		label.color[1] = color.y;
		label.color[2] = color.z;
		label.color[3] = color.w;

		qvkCmdBeginDebugUtilsLabelEXT( vkcontext.commandBuffer[ vkcontext.frameParity ], &label );
	}
#else
	// RB: colors are not supported in OpenGL

	// only do this if RBDOOM-3-BFG was started by RenderDoc or some similar tool
	if( glConfig.gremedyStringMarkerAvailable && glConfig.khronosDebugAvailable )
	{
		glPushDebugGroup( GL_DEBUG_SOURCE_APPLICATION_ARB, 0, GLsizei( strlen( szName ) ), szName );
	}
#endif

#if 0
	if( !r_pix.GetBool() )
	{
		return;
	}
	if( !pixEvents )
	{
		// lazy allocation to not waste memory
		pixEvents = ( pixEvent_t* )Mem_ClearedAlloc( sizeof( *pixEvents ) * MAX_PIX_EVENTS, TAG_CRAP );
	}
	if( numPixEvents >= MAX_PIX_EVENTS )
	{
		idLib::FatalError( "PC_BeginNamedEvent: event overflow" );
	}
	if( ++numPixLevels > 1 )
	{
		return;	// only get top level timing information
	}
	if( !glGetQueryObjectui64vEXT )
	{
		return;
	}

	GL_CheckErrors();
	if( timeQueryIds[0] == 0 )
	{
		glGenQueries( MAX_PIX_EVENTS, timeQueryIds );
	}
	glFinish();
	glBeginQuery( GL_TIME_ELAPSED_EXT, timeQueryIds[numPixEvents] );
	GL_CheckErrors();

	pixEvent_t* ev = &pixEvents[numPixEvents++];
	strncpy( ev->name, szName, sizeof( ev->name ) - 1 );
	ev->cpuTime = Sys_Microseconds();
#endif
}

/*
========================
PC_EndNamedEvent
========================
*/
void PC_EndNamedEvent()
{
#if defined( USE_VULKAN )
	if( vkcontext.debugMarkerSupportAvailable )
	{
		qvkCmdDebugMarkerEndEXT( vkcontext.commandBuffer[ vkcontext.frameParity ] );
	}
	else if( vkcontext.debugUtilsSupportAvailable )
	{
		qvkCmdEndDebugUtilsLabelEXT( vkcontext.commandBuffer[ vkcontext.frameParity ] );
	}
#else
	// only do this if RBDOOM-3-BFG was started by RenderDoc or some similar tool
	if( glConfig.gremedyStringMarkerAvailable && glConfig.khronosDebugAvailable )
	{
		glPopDebugGroup();
	}
#endif

#if 0
	if( !r_pix.GetBool() )
	{
		return;
	}
	if( numPixLevels <= 0 )
	{
		idLib::FatalError( "PC_EndNamedEvent: level underflow" );
	}
	if( --numPixLevels > 0 )
	{
		// only do timing on top level events
		return;
	}
	if( !glGetQueryObjectui64vEXT )
	{
		return;
	}

	pixEvent_t* ev = &pixEvents[numPixEvents - 1];
	ev->cpuTime = Sys_Microseconds() - ev->cpuTime;

	GL_CheckErrors();
	glEndQuery( GL_TIME_ELAPSED_EXT );
	GL_CheckErrors();
#endif
}

/*
========================
PC_EndFrame
========================
*/
void PC_EndFrame()
{
#if 0
	if( !r_pix.GetBool() )
	{
		return;
	}

	int64 totalGPU = 0;
	int64 totalCPU = 0;

	idLib::Printf( "----- GPU Events -----\n" );
	for( int i = 0 ; i < numPixEvents ; i++ )
	{
		pixEvent_t* ev = &pixEvents[i];

		int64 gpuTime = 0;
		glGetQueryObjectui64vEXT( timeQueryIds[i], GL_QUERY_RESULT, ( GLuint64EXT* )&gpuTime );
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
idRenderLog::idRenderLog()
{
	activeLevel = 0;
	indentString[0] = '\0';
	indentLevel = 0;
//	logFile = NULL;

	frameStartTime = 0;
	closeBlockTime = 0;
	logLevel = 0;
}

/*
========================
idRenderLog::StartFrame
========================
*/
void idRenderLog::StartFrame()
{
	if( r_logFile.GetInteger() == 0 )
	{
		return;
	}

	// open a new logfile
	indentLevel = 0;
	indentString[0] = '\0';
	activeLevel = r_logLevel.GetInteger();

	/*
	struct tm*		newtime;
	time_t			aclock;

	char ospath[ MAX_OSPATH ];

	char qpath[128];
	sprintf( qpath, "renderlogPC_%04i.txt", r_logFile.GetInteger() );
	//idStr finalPath = fileSystem->RelativePathToOSPath( qpath );
	sprintf( ospath, "%s", qpath );
	*/
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

	/*
	if( logFile != NULL )
	{
		fileSystem->CloseFile( logFile );
		logFile = NULL;
	}

	logFile = fileSystem->OpenFileWrite( ospath );
	if( logFile == NULL )
	{
		idLib::Warning( "Failed to open logfile %s", ospath );
		return;
	}
	idLib::Printf( "Opened logfile %s\n", ospath );

	// write the time out to the top of the file
	time( &aclock );
	newtime = localtime( &aclock );
	const char* str = asctime( newtime );
	logFile->Printf( "// %s", str );
	logFile->Printf( "// %s\n\n", com_version.GetString() );
	*/

	frameStartTime = Sys_Microseconds();
	closeBlockTime = frameStartTime;
	OpenBlock( "Frame" );
}

/*
========================
idRenderLog::EndFrame
========================
*/
void idRenderLog::EndFrame()
{
	PC_EndFrame();

	//if( logFile != NULL )
	if( r_logFile.GetInteger() != 0 )
	{
		if( r_logFile.GetInteger() == 1 )
		{
			Close();
		}
		// log is open, so decrement r_logFile and stop if it is zero
		//r_logFile.SetInteger( r_logFile.GetInteger() - 1 );
		//idLib::Printf( "Frame logged.\n" );
		return;
	}
}

/*
========================
idRenderLog::Close
========================
*/
void idRenderLog::Close()
{
	//if( logFile != NULL )
	if( r_logFile.GetInteger() != 0 )
	{
		CloseBlock();
		//idLib::Printf( "Closing logfile\n" );
		//fileSystem->CloseFile( logFile );
		//logFile = NULL;
		activeLevel = 0;
	}
}

/*
========================
idRenderLog::OpenMainBlock
========================
*/
void idRenderLog::OpenMainBlock( renderLogMainBlock_t block )
{
}

/*
========================
idRenderLog::CloseMainBlock
========================
*/
void idRenderLog::CloseMainBlock()
{
}

/*
========================
idRenderLog::OpenBlock
========================
*/
void idRenderLog::OpenBlock( const char* label )
{
	// Allow the PIX functionality even when logFile is not running.
	PC_BeginNamedEvent( label );

	//if( logFile != NULL )
	if( r_logFile.GetInteger() != 0 )
	{
		LogOpenBlock( RENDER_LOG_INDENT_MAIN_BLOCK, "%s", label );
	}
}

/*
========================
idRenderLog::CloseBlock
========================
*/
void idRenderLog::CloseBlock()
{
	PC_EndNamedEvent();

	//if( logFile != NULL )
	if( r_logFile.GetInteger() != 0 )
	{
		LogCloseBlock( RENDER_LOG_INDENT_MAIN_BLOCK );
	}
}

/*
========================
idRenderLog::Printf
========================
*/
void idRenderLog::Printf( const char* fmt, ... )
{
#if !defined(USE_VULKAN)
	if( activeLevel <= LOG_LEVEL_BLOCKS_ONLY )
	{
		return;
	}

	//if( logFile == NULL )
	if( r_logFile.GetInteger() == 0 || !glConfig.gremedyStringMarkerAvailable )
	{
		return;
	}

	va_list		marker;
	char		msg[4096];

	idStr		out = indentString;

	va_start( marker, fmt );
	idStr::vsnPrintf( msg, sizeof( msg ), fmt, marker );
	va_end( marker );

	msg[sizeof( msg ) - 1] = '\0';

	out.Append( msg );

	glStringMarkerGREMEDY( out.Length(), out.c_str() );

	//logFile->Printf( "%s", indentString );
	//va_start( marker, fmt );
	//logFile->VPrintf( fmt, marker );
	//va_end( marker );


//	logFile->Flush();		this makes it take waaaay too long
#endif
}

/*
========================
idRenderLog::LogOpenBlock
========================
*/
void idRenderLog::LogOpenBlock( renderLogIndentLabel_t label, const char* fmt, ... )
{
	uint64 now = Sys_Microseconds();

	//if( logFile != NULL )
	if( r_logFile.GetInteger() != 0 )
	{
		//if( now - closeBlockTime >= 1000 )
		//{
		//logFile->Printf( "%s%1.1f msec gap from last closeblock\n", indentString, ( now - closeBlockTime ) * ( 1.0f / 1000.0f ) );
		//}

#if !defined(USE_VULKAN)
		if( glConfig.gremedyStringMarkerAvailable )
		{
			//Printf( fmt, args );
			//Printf( " {\n" );

			//logFile->Printf( "%s", indentString );
			//logFile->VPrintf( fmt, args );
			//logFile->Printf( " {\n" );

			va_list		marker;
			char		msg[4096];

			idStr		out = indentString;

			va_start( marker, fmt );
			idStr::vsnPrintf( msg, sizeof( msg ), fmt, marker );
			va_end( marker );

			msg[sizeof( msg ) - 1] = '\0';

			out.Append( msg );
			out += " {";

			glStringMarkerGREMEDY( out.Length(), out.c_str() );
		}
#endif
	}

	Indent( label );

	if( logLevel >= MAX_LOG_LEVELS )
	{
		idLib::Warning( "logLevel %d >= MAX_LOG_LEVELS", logLevel );
	}


	logLevel++;
}

/*
========================
idRenderLog::LogCloseBlock
========================
*/
void idRenderLog::LogCloseBlock( renderLogIndentLabel_t label )
{
	closeBlockTime = Sys_Microseconds();

	//assert( logLevel > 0 );
	logLevel--;

	Outdent( label );

	//if( logFile != NULL )
	//{
	//}
}

#else	// !STUB_RENDER_LOG

// RB begin
/*
========================
idRenderLog::idRenderLog
========================
*/
idRenderLog::idRenderLog()
{
}

#if 1

/*
========================
idRenderLog::OpenMainBlock
========================
*/
void idRenderLog::OpenMainBlock( renderLogMainBlock_t block )
{
	mainBlock = block;

#if defined( USE_VULKAN )
	if( vkcontext.queryIndex[ vkcontext.frameParity ] >= ( NUM_TIMESTAMP_QUERIES - 1 ) )
	{
		return;
	}

	VkCommandBuffer commandBuffer = vkcontext.commandBuffer[ vkcontext.frameParity ];
	VkQueryPool queryPool = vkcontext.queryPools[ vkcontext.frameParity ];

	uint32 queryIndex = vkcontext.queryAssignedIndex[ vkcontext.frameParity ][ mainBlock * 2 + 0 ] = vkcontext.queryIndex[ vkcontext.frameParity ]++;
	vkCmdWriteTimestamp( commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, queryPool, queryIndex );

#else

	if( glConfig.timerQueryAvailable )
	{
		if( glcontext.renderLogMainBlockTimeQueryIds[ glcontext.frameParity ][ block * 2 ] == 0 )
		{
			glCreateQueries( GL_TIMESTAMP, 2, &glcontext.renderLogMainBlockTimeQueryIds[ glcontext.frameParity ][ block * 2 ] );
		}

		glQueryCounter( glcontext.renderLogMainBlockTimeQueryIds[ glcontext.frameParity ][ block * 2 + 0 ], GL_TIMESTAMP );
		glcontext.renderLogMainBlockTimeQueryIssued[ glcontext.frameParity ][ block * 2 + 0 ]++;
	}
#endif
}

/*
========================
idRenderLog::CloseMainBlock
========================
*/
void idRenderLog::CloseMainBlock()
{
#if defined( USE_VULKAN )

	if( vkcontext.queryIndex[ vkcontext.frameParity ] >= ( NUM_TIMESTAMP_QUERIES - 1 ) )
	{
		return;
	}

	VkCommandBuffer commandBuffer = vkcontext.commandBuffer[ vkcontext.frameParity ];
	VkQueryPool queryPool = vkcontext.queryPools[ vkcontext.frameParity ];

	uint32 queryIndex = vkcontext.queryAssignedIndex[ vkcontext.frameParity ][ mainBlock * 2 + 1 ] = vkcontext.queryIndex[ vkcontext.frameParity ]++;
	vkCmdWriteTimestamp( commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, queryPool, queryIndex );

#else

	if( glConfig.timerQueryAvailable )
	{
		glQueryCounter( glcontext.renderLogMainBlockTimeQueryIds[ glcontext.frameParity ][ mainBlock * 2 + 1 ], GL_TIMESTAMP );
		glcontext.renderLogMainBlockTimeQueryIssued[ glcontext.frameParity ][ mainBlock * 2 + 1 ]++;
	}
#endif
}

#endif

/*
========================
idRenderLog::OpenBlock
========================
*/
void idRenderLog::OpenBlock( const char* label, const idVec4& color )
{
	PC_BeginNamedEvent( label, color );
}

/*
========================
idRenderLog::CloseBlock
========================
*/
void idRenderLog::CloseBlock()
{
	PC_EndNamedEvent();
}
// RB end

#endif // !STUB_RENDER_LOG
