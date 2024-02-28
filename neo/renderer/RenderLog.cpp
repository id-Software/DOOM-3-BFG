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

#include <sys/DeviceManager.h>
extern DeviceManager* deviceManager;

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
	ASSERT_ENUM_STRING( MRB_MOTION_VECTORS,					14 ),
	ASSERT_ENUM_STRING( MRB_TAA,							15 ),
	ASSERT_ENUM_STRING( MRB_TONE_MAP_PASS,					16 ),
	ASSERT_ENUM_STRING( MRB_POSTPROCESS,					17 ),
	ASSERT_ENUM_STRING( MRB_DRAW_GUI,                       18 ),
	ASSERT_ENUM_STRING( MRB_CRT_POSTPROCESS,                19 ),
	ASSERT_ENUM_STRING( MRB_TOTAL,							20 )
};

extern uint64 Sys_Microseconds();

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

	if( commandList )
	{
		commandList->beginMarker( szName );
	}
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

	if( commandList )
	{
		commandList->endMarker();
	}
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
	for( int i = 0; i < MRB_TOTAL * NUM_FRAME_DATA; i++ )
	{
		timerQueries.Append( deviceManager->GetDevice()->createTimerQuery() );
		timerUsed.Append( false );
	}
}

void idRenderLog::Shutdown()
{
	commandList = nullptr;

	for( int i = 0; i < MRB_TOTAL * NUM_FRAME_DATA; i++ )
	{
		timerQueries[i].Reset();
	}
}

void idRenderLog::StartFrame( nvrhi::ICommandList* _commandList )
{
	commandList = _commandList;
}

void idRenderLog::EndFrame()
{
	commandList = nullptr;
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

		int timerIndex = mainBlock + frameParity * MRB_TOTAL;

		// SRS - Only issue a new start timer query if timer slot unused
		if( !timerUsed[ timerIndex ] )
		{
			commandList->beginTimerQuery( timerQueries[ timerIndex ] );
		}
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

		int timerIndex = block + frameParity * MRB_TOTAL;

		// SRS - Only issue a new end timer query if timer slot unused
		if( !timerUsed[ timerIndex ] )
		{
			commandList->endTimerQuery( timerQueries[ timerIndex ] );
			timerUsed[ timerIndex ] = true;
		}
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

	for( int i = 0; i < MRB_TOTAL; i++ )
	{
		int timerIndex = i + frameParity * MRB_TOTAL;

		if( timerUsed[timerIndex] )
		{
			double time = deviceManager->GetDevice()->getTimerQueryTime( timerQueries[ timerIndex ] );
			time *= 1000000.0; // seconds -> microseconds

			switch( i )
			{
				case MRB_GPU_TIME:
					pc.gpuMicroSec = time;
					break;

				case MRB_BEGIN_DRAWING_VIEW:
					pc.gpuBeginDrawingMicroSec = time;
					break;

				case MRB_FILL_DEPTH_BUFFER:
					pc.gpuDepthMicroSec = time;
					break;

				case MRB_FILL_GEOMETRY_BUFFER:
					pc.gpuGeometryMicroSec = time;
					break;

				case MRB_SSAO_PASS:
					pc.gpuScreenSpaceAmbientOcclusionMicroSec = time;
					break;

				case MRB_AMBIENT_PASS:
					pc.gpuAmbientPassMicroSec = time;
					break;

				case MRB_SHADOW_ATLAS_PASS:
					pc.gpuShadowAtlasPassMicroSec = time;
					break;

				case MRB_DRAW_INTERACTIONS:
					pc.gpuInteractionsMicroSec = time;
					break;

				case MRB_DRAW_SHADER_PASSES:
					pc.gpuShaderPassMicroSec = time;
					break;

				case MRB_FOG_ALL_LIGHTS:
					pc.gpuFogAllLightsMicroSec = time;
					break;

				case MRB_BLOOM:
					pc.gpuBloomMicroSec = time;
					break;

				case MRB_DRAW_SHADER_PASSES_POST:
					pc.gpuShaderPassPostMicroSec = time;
					break;

				case MRB_MOTION_VECTORS:
					pc.gpuMotionVectorsMicroSec = time;
					break;

				case MRB_TAA:
					pc.gpuTemporalAntiAliasingMicroSec = time;
					break;

				case MRB_TONE_MAP_PASS:
					pc.gpuToneMapPassMicroSec = time;
					break;

				case MRB_POSTPROCESS:
					pc.gpuPostProcessingMicroSec = time;
					break;

				case MRB_DRAW_GUI:
					pc.gpuDrawGuiMicroSec = time;
					break;

				case MRB_CRT_POSTPROCESS:
					pc.gpuCrtPostProcessingMicroSec = time;
					break;

				default:
					break;
			}
		}

		// reset timer
		timerUsed[timerIndex] = false;
	}
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
