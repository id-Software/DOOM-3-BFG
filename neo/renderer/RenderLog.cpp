/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2022 Robert Beckebans

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

#if defined( USE_NVRHI )
	#include <sys/DeviceManager.h>
	extern DeviceManager* deviceManager;
#endif

/*
================================================================================================
Contains the RenderLog implementation.

TODO:	Emit statistics to the logfile at the end of views and frames.
================================================================================================
*/

idCVar r_logLevel( "r_logLevel", "0", CVAR_INTEGER, "1 = blocks only, 2 = everything", 0, 2 );

static const int LOG_LEVEL_BLOCKS_ONLY	= 1;
static const int LOG_LEVEL_EVERYTHING	= 2;

const char* renderLogMainBlockLabels[] =
{
	ASSERT_ENUM_STRING( MRB_GPU_TIME,						0 ),
	ASSERT_ENUM_STRING( MRB_BEGIN_DRAWING_VIEW,				1 ),
	ASSERT_ENUM_STRING( MRB_FILL_DEPTH_BUFFER,				2 ),
	ASSERT_ENUM_STRING( MRB_FILL_GEOMETRY_BUFFER,			3 ),
	ASSERT_ENUM_STRING( MRB_SSAO_PASS,						4 ),
	ASSERT_ENUM_STRING( MRB_AMBIENT_PASS,					5 ),
	ASSERT_ENUM_STRING( MRB_SHADOW_ATLAS_PASS,				6 ),
	ASSERT_ENUM_STRING( MRB_DRAW_INTERACTIONS,				7 ),
	ASSERT_ENUM_STRING( MRB_DRAW_SHADER_PASSES,				8 ),
	ASSERT_ENUM_STRING( MRB_FOG_ALL_LIGHTS,					9 ),
	ASSERT_ENUM_STRING( MRB_BLOOM,							10 ),
	ASSERT_ENUM_STRING( MRB_DRAW_SHADER_PASSES_POST,		11 ),
	ASSERT_ENUM_STRING( MRB_DRAW_DEBUG_TOOLS,				12 ),
	ASSERT_ENUM_STRING( MRB_CAPTURE_COLORBUFFER,			13 ),
	ASSERT_ENUM_STRING( MRB_TAA,							14 ),
	ASSERT_ENUM_STRING( MRB_POSTPROCESS,					15 ),
	ASSERT_ENUM_STRING( MRB_DRAW_GUI,                       16 ),
	ASSERT_ENUM_STRING( MRB_TOTAL,							17 )
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

#if !defined( USE_VULKAN ) && !defined(USE_NVRHI)
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
void PC_BeginNamedEvent( const char* szName, const idVec4& color, nvrhi::ICommandList* commandList )
{
	if( r_logLevel.GetInteger() <= 0 )
	{
		return;
	}

#if defined( USE_NVRHI )
	if( commandList )
	{
		commandList->beginMarker( szName );
	}

#elif defined( USE_VULKAN )

	// start an annotated group of calls under the this name
	// SRS - Prefer VK_EXT_debug_utils over VK_EXT_debug_marker/VK_EXT_debug_report (deprecated by VK_EXT_debug_utils)
	if( vkcontext.debugUtilsSupportAvailable )
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
	else if( vkcontext.debugMarkerSupportAvailable )
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
void PC_EndNamedEvent( nvrhi::ICommandList* commandList )
{
	if( r_logLevel.GetInteger() <= 0 )
	{
		return;
	}

#if defined( USE_NVRHI )
	if( commandList )
	{
		commandList->endMarker();
	}

#elif defined( USE_VULKAN )
	// SRS - Prefer VK_EXT_debug_utils over VK_EXT_debug_marker/VK_EXT_debug_report (deprecated by VK_EXT_debug_utils)
	if( vkcontext.debugUtilsSupportAvailable )
	{
		qvkCmdEndDebugUtilsLabelEXT( vkcontext.commandBuffer[ vkcontext.frameParity ] );
	}
	else if( vkcontext.debugMarkerSupportAvailable )
	{
		qvkCmdDebugMarkerEndEXT( vkcontext.commandBuffer[ vkcontext.frameParity ] );
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


idRenderLog::idRenderLog()
{
	frameCounter = 0;
	frameParity = 0;
}

void idRenderLog::Init()
{
#if defined( USE_NVRHI )
	for( int i = 0; i < MRB_TOTAL * NUM_FRAME_DATA; i++ )
	{
		timerQueries.Append( deviceManager->GetDevice()->createTimerQuery() );
		timerUsed.Append( false );
	}
#endif
}

void idRenderLog::Shutdown()
{
#if defined( USE_NVRHI )
	for( int i = 0; i < MRB_TOTAL * NUM_FRAME_DATA; i++ )
	{
		timerQueries[i].Reset();
	}
#endif
}

void idRenderLog::StartFrame( nvrhi::ICommandList* _commandList )
{
#if defined( USE_NVRHI )
	commandList = _commandList;
#endif
}

void idRenderLog::EndFrame()
{
#if defined( USE_NVRHI )
	commandList = nullptr;
#endif
}



/*
========================
idRenderLog::OpenMainBlock
========================
*/
void idRenderLog::OpenMainBlock( renderLogMainBlock_t block )
{
	// SRS - Use glConfig.timerQueryAvailable flag to control timestamp capture for all platforms
	if( glConfig.timerQueryAvailable )
	{
		mainBlock = block;

#if defined( USE_NVRHI )

		int timerIndex = mainBlock + frameParity * MRB_TOTAL;

		// SRS - Only issue a new start timer query if timer slot unused
		if( !timerUsed[ timerIndex ] )
		{
			commandList->beginTimerQuery( timerQueries[ timerIndex ] );
		}

#elif defined( USE_VULKAN )
		if( vkcontext.queryIndex[ vkcontext.frameParity ] >= ( NUM_TIMESTAMP_QUERIES - 1 ) )
		{
			return;
		}

		VkCommandBuffer commandBuffer = vkcontext.commandBuffer[ vkcontext.frameParity ];
		VkQueryPool queryPool = vkcontext.queryPools[ vkcontext.frameParity ];

		uint32 queryIndex = vkcontext.queryAssignedIndex[ vkcontext.frameParity ][ mainBlock * 2 + 0 ] = vkcontext.queryIndex[ vkcontext.frameParity ]++;
		vkCmdWriteTimestamp( commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, queryPool, queryIndex );

#elif defined(__APPLE__)
		// SRS - For OSX use elapsed time query for Apple OpenGL 4.1 using GL_TIME_ELAPSED vs GL_TIMESTAMP (which is not implemented on OSX)
		// SRS - OSX AMD drivers have a rendering bug (flashing colours) with an elasped time query when Shadow Mapping is on - turn off query for that case unless r_skipAMDWorkarounds is set
		if( !r_useShadowMapping.GetBool() || glConfig.vendor != VENDOR_AMD || r_skipAMDWorkarounds.GetBool() )
		{
			if( glcontext.renderLogMainBlockTimeQueryIds[ glcontext.frameParity ][ mainBlock * 2 + 1 ] == 0 )
			{
				glGenQueries( 1, &glcontext.renderLogMainBlockTimeQueryIds[ glcontext.frameParity ][ mainBlock * 2 + 1 ] );
			}

			glBeginQuery( GL_TIME_ELAPSED_EXT, glcontext.renderLogMainBlockTimeQueryIds[ glcontext.frameParity ][ mainBlock * 2 + 1 ] );
		}

#else
		if( glcontext.renderLogMainBlockTimeQueryIds[ glcontext.frameParity ][ mainBlock * 2 ] == 0 )
		{
			glCreateQueries( GL_TIMESTAMP, 2, &glcontext.renderLogMainBlockTimeQueryIds[ glcontext.frameParity ][ mainBlock * 2 ] );
		}

		glQueryCounter( glcontext.renderLogMainBlockTimeQueryIds[ glcontext.frameParity ][ mainBlock * 2 + 0 ], GL_TIMESTAMP );
		glcontext.renderLogMainBlockTimeQueryIssued[ glcontext.frameParity ][ mainBlock * 2 + 0 ]++;
#endif
	}
}

/*
========================
idRenderLog::CloseMainBlock
========================
*/
void idRenderLog::CloseMainBlock( int _block )
{
	// SRS - Use glConfig.timerQueryAvailable flag to control timestamp capture for all platforms
	if( glConfig.timerQueryAvailable )
	{
		renderLogMainBlock_t block = mainBlock;

		if( _block != -1 )
		{
			block = renderLogMainBlock_t( _block );
		}

#if defined( USE_NVRHI )

		int timerIndex = block + frameParity * MRB_TOTAL;

		// SRS - Only issue a new end timer query if timer slot unused
		if( !timerUsed[ timerIndex ] )
		{
			commandList->endTimerQuery( timerQueries[ timerIndex ] );
			timerUsed[ timerIndex ] = true;
		}

#elif defined( USE_VULKAN )
		if( vkcontext.queryIndex[ vkcontext.frameParity ] >= ( NUM_TIMESTAMP_QUERIES - 1 ) )
		{
			return;
		}

		VkCommandBuffer commandBuffer = vkcontext.commandBuffer[ vkcontext.frameParity ];
		VkQueryPool queryPool = vkcontext.queryPools[ vkcontext.frameParity ];

		uint32 queryIndex = vkcontext.queryAssignedIndex[ vkcontext.frameParity ][ block * 2 + 1 ] = vkcontext.queryIndex[ vkcontext.frameParity ]++;
		vkCmdWriteTimestamp( commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, queryPool, queryIndex );

#elif defined(__APPLE__)
		// SRS - For OSX use elapsed time query for Apple OpenGL 4.1 using GL_TIME_ELAPSED vs GL_TIMESTAMP (which is not implemented on OSX)
		// SRS - OSX AMD drivers have a rendering bug (flashing colours) with an elasped time query when Shadow Mapping is on - turn off query for that case unless r_skipAMDWorkarounds is set
		if( !r_useShadowMapping.GetBool() || glConfig.vendor != VENDOR_AMD || r_skipAMDWorkarounds.GetBool() )
		{
			glEndQuery( GL_TIME_ELAPSED_EXT );
			glcontext.renderLogMainBlockTimeQueryIssued[ glcontext.frameParity ][ block * 2 + 1 ]++;
		}

#else
		glQueryCounter( glcontext.renderLogMainBlockTimeQueryIds[ glcontext.frameParity ][ block * 2 + 1 ], GL_TIMESTAMP );
		glcontext.renderLogMainBlockTimeQueryIssued[ glcontext.frameParity ][ block * 2 + 1 ]++;
#endif
	}
}

/*
========================
idRenderLog::FetchGPUTimers
========================
*/
void idRenderLog::FetchGPUTimers( backEndCounters_t& pc )
{
	frameCounter++;
	frameParity = ( frameParity + 1 ) % NUM_FRAME_DATA;

#if defined( USE_NVRHI )

	for( int i = 0; i < MRB_TOTAL; i++ )
	{
		int timerIndex = i + frameParity * MRB_TOTAL;

		if( timerUsed[timerIndex] )
		{
			double time = deviceManager->GetDevice()->getTimerQueryTime( timerQueries[ timerIndex ] );
			time *= 1000000.0; // seconds -> microseconds

			if( i == MRB_GPU_TIME )
			{
				pc.gpuMicroSec = time;
			}
			else if( i == MRB_FILL_DEPTH_BUFFER )
			{
				pc.gpuDepthMicroSec = time;
			}
			else if( i == MRB_SSAO_PASS )
			{
				pc.gpuScreenSpaceAmbientOcclusionMicroSec = time;
			}
			else if( i == MRB_AMBIENT_PASS )
			{
				pc.gpuAmbientPassMicroSec = time;
			}
			else if( i == MRB_SHADOW_ATLAS_PASS )
			{
				pc.gpuShadowAtlasPassMicroSec = time;
			}
			else if( i == MRB_DRAW_INTERACTIONS )
			{
				pc.gpuInteractionsMicroSec = time;
			}
			else if( i == MRB_DRAW_SHADER_PASSES )
			{
				pc.gpuShaderPassMicroSec = time;
			}
			else if( i == MRB_TAA )
			{
				pc.gpuTemporalAntiAliasingMicroSec = time;
			}
			else if( i == MRB_POSTPROCESS )
			{
				pc.gpuPostProcessingMicroSec = time;
			}
		}

		// reset timer
		timerUsed[timerIndex] = false;
	}
#endif
}


/*
========================
idRenderLog::OpenBlock
========================
*/
void idRenderLog::OpenBlock( const char* label, const idVec4& color )
{
	PC_BeginNamedEvent( label, color, commandList );
}

/*
========================
idRenderLog::CloseBlock
========================
*/
void idRenderLog::CloseBlock()
{
	PC_EndNamedEvent( commandList );
}
// RB end
