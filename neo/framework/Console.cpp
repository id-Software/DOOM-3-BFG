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
#include "ConsoleHistory.h"
#include "../renderer/ResolutionScale.h"
#include "Common_local.h"
#include "../imgui/BFGimgui.h"

#include <sys/DeviceManager.h>
extern DeviceManager* deviceManager;

#define	CON_TEXTSIZE			0x30000
#define	NUM_CON_TIMES			4
#define CONSOLE_FIRSTREPEAT		200
#define CONSOLE_REPEAT			100

#define	COMMAND_HISTORY			64

struct overlayText_t
{
	idStr			text;
	justify_t		justify;
	int				time;
};

// the console will query the cvar and command systems for
// command completion information

class idConsoleLocal : public idConsole
{
public:
	virtual	void		Init();
	virtual void		Shutdown();
	virtual	bool		ProcessEvent( const sysEvent_t* event, bool forceAccept );
	virtual	bool		Active();
	virtual	void		ClearNotifyLines();
	virtual void		Open();
	virtual	void		Close();
	virtual	void		Print( const char* text );
	virtual	void		Draw( bool forceFullScreen );

	virtual void		PrintOverlay( idOverlayHandle& handle, justify_t justify, const char* text, ... );

	virtual idDebugGraph* 	CreateGraph( int numItems );
	virtual void			DestroyGraph( idDebugGraph* graph );

	void				Dump( const char* toFile );
	void				Clear();

private:
	void				Resize();

	void				KeyDownEvent( int key );

	void				Linefeed();

	void				PageUp();
	void				PageDown();
	void				Top();
	void				Bottom();

	void				DrawInput();
	void				DrawNotify();
	void				DrawSolidConsole( float frac );

	void				Scroll();
	void				SetDisplayFraction( float frac );
	void				UpdateDisplayFraction();

	void				DrawTextLeftAlign( float x, float& y, const char* text, ... );
	void				DrawTextRightAlign( float x, float& y, const char* text, ... );

	float				DrawFPS( float y );
	float				DrawMemoryUsage( float y );

	void				DrawOverlayText( float& leftY, float& rightY, float& centerY );
	void				DrawDebugGraphs();

	//============================

	// allow these constants to be adjusted for HMD
	int					LOCALSAFE_LEFT;
	int					LOCALSAFE_RIGHT;
	int					LOCALSAFE_TOP;
	int					LOCALSAFE_BOTTOM;
	int					LOCALSAFE_WIDTH;
	int					LOCALSAFE_HEIGHT;
	int					LINE_WIDTH;
	int					TOTAL_LINES;

	bool				keyCatching;

	short				text[CON_TEXTSIZE];
	int					current;		// line where next message will be printed
	int					x;				// offset in current line for next print
	int					display;		// bottom of console displays this line
	int					lastKeyEvent;	// time of last key event for scroll delay
	int					nextKeyEvent;	// keyboard repeat rate

	float				displayFrac;	// approaches finalFrac at con_speed
	float				finalFrac;		// 0.0 to 1.0 lines of console to display
	int					fracTime;		// time of last displayFrac update

	int					vislines;		// in scanlines

	int					times[NUM_CON_TIMES];	// cls.realtime time the line was generated
	// for transparent notify lines
	idVec4				color;

	idEditField			historyEditLines[COMMAND_HISTORY];

	int					nextHistoryLine;// the last line in the history buffer, not masked
	int					historyLine;	// the line being displayed from history buffer
	// will be <= nextHistoryLine

	idEditField			consoleField;

	idList< overlayText_t >	overlayText;
	idList< idDebugGraph*> debugGraphs;

	int					lastVirtualScreenWidth;
	int					lastVirtualScreenHeight;

	static idCVar		con_speed;
	static idCVar		con_notifyTime;
	static idCVar		con_noPrint;
};

static idConsoleLocal localConsole;
idConsole* console = &localConsole;

idCVar idConsoleLocal::con_speed( "con_speed", "3", CVAR_SYSTEM, "speed at which the console moves up and down" );
idCVar idConsoleLocal::con_notifyTime( "con_notifyTime", "3", CVAR_SYSTEM, "time messages are displayed onscreen when console is pulled up" );
#ifdef DEBUG
	idCVar idConsoleLocal::con_noPrint( "con_noPrint", "0", CVAR_BOOL | CVAR_SYSTEM | CVAR_NOCHEAT, "print on the console but not onscreen when console is pulled up" );
#else
	idCVar idConsoleLocal::con_noPrint( "con_noPrint", "1", CVAR_BOOL | CVAR_SYSTEM | CVAR_NOCHEAT, "print on the console but not onscreen when console is pulled up" );
#endif

/*
=============================================================================

	Misc stats

=============================================================================
*/

/*
==================
idConsoleLocal::DrawTextLeftAlign
==================
*/
void idConsoleLocal::DrawTextLeftAlign( float x, float& y, const char* text, ... )
{
	char string[MAX_STRING_CHARS];
	va_list argptr;
	va_start( argptr, text );
	idStr::vsnPrintf( string, sizeof( string ), text, argptr );
	va_end( argptr );
	renderSystem->DrawSmallStringExt( x, y + 2, string, colorWhite, true );
	y += SMALLCHAR_HEIGHT + 4;
}

/*
==================
idConsoleLocal::DrawTextRightAlign
==================
*/
void idConsoleLocal::DrawTextRightAlign( float x, float& y, const char* text, ... )
{
	char string[MAX_STRING_CHARS];
	va_list argptr;
	va_start( argptr, text );
	int i = idStr::vsnPrintf( string, sizeof( string ), text, argptr );
	va_end( argptr );
	renderSystem->DrawSmallStringExt( x - i * SMALLCHAR_WIDTH, y + 2, string, colorWhite, true );
	y += SMALLCHAR_HEIGHT + 4;
}




/*
==================
idConsoleLocal::DrawFPS
==================
*/
extern bool R_UseTemporalAA();

#define	FPS_FRAMES	6
#define FPS_FRAMES_HISTORY 90
float idConsoleLocal::DrawFPS( float y )
{
	extern idCVar r_swapInterval;

	static float previousTimes[FPS_FRAMES];
	static float previousTimesNormalized[FPS_FRAMES_HISTORY];
	static int index;
	static int previous;
	static int valuesOffset = 0;

	bool renderImGuiPerfWindow = ImGuiHook::IsReadyToRender() && ( com_showFPS.GetInteger() > 1 );

	// don't use serverTime, because that will be drifting to
	// correct for internet lag changes, timescales, timedemos, etc
	int t = Sys_Milliseconds();
	int frameTime = t - previous;
	previous = t;

	int fps = 0;

	const float milliSecondsPerFrame = 1000.0f / com_engineHz_latched;

	previousTimes[index % FPS_FRAMES] = frameTime;
	previousTimesNormalized[index % FPS_FRAMES_HISTORY] = frameTime / milliSecondsPerFrame;
	valuesOffset = ( valuesOffset + 1 ) % FPS_FRAMES_HISTORY;
	index++;
	if( index > FPS_FRAMES )
	{
		// average multiple frames together to smooth changes out a bit
		int total = 0;
		for( int i = 0 ; i < FPS_FRAMES ; i++ )
		{
			total += previousTimes[i];
		}
		if( !total )
		{
			total = 1;
		}
		fps = 1000000 * FPS_FRAMES / total;
		fps = ( fps + 500 ) / 1000;

		const char* s = va( "%ifps", fps );
		int w = strlen( s ) * BIGCHAR_WIDTH;

		if( com_showFPS.GetInteger() == 1 )
		{
			renderSystem->DrawBigStringExt( LOCALSAFE_RIGHT - w, idMath::Ftoi( y ) + 2, s, colorWhite, true );
		}
	}

	y += BIGCHAR_HEIGHT + 4;

	// DG: "com_showFPS 1" means: show FPS only, like in classic doom3
	if( com_showFPS.GetInteger() == 1 )
	{
		return y;
	}
	// DG end

	// SRS - Shouldn't use these getters since they access and return int-sized variables measured in milliseconds
	//const uint64 gameThreadTotalTime = commonLocal.GetGameThreadTotalTime();
	//const uint64 gameThreadGameTime = commonLocal.GetGameThreadGameTime();
	//const uint64 gameThreadRenderTime = commonLocal.GetGameThreadRenderTime();

	const uint64 gameThreadTotalTime	= commonLocal.mainFrameTiming.finishDrawTime - commonLocal.mainFrameTiming.startGameTime;
	const uint64 gameThreadGameTime		= commonLocal.mainFrameTiming.finishGameTime - commonLocal.mainFrameTiming.startGameTime;
	const uint64 gameThreadRenderTime	= commonLocal.mainFrameTiming.finishDrawTime - commonLocal.mainFrameTiming.finishGameTime;

	const uint64 rendererBackEndTime = commonLocal.GetRendererBackEndMicroseconds();
	const uint64 rendererShadowsTime = commonLocal.GetRendererShadowsMicroseconds();
	const uint64 rendererGPUTime = commonLocal.GetRendererGPUMicroseconds();
	const uint64 rendererGPUEarlyZTime = commonLocal.GetRendererGpuEarlyZMicroseconds();
	const uint64 rendererGPU_SSAOTime = commonLocal.GetRendererGpuSSAOMicroseconds();
	const uint64 rendererGPU_SSRTime = commonLocal.GetRendererGpuSSRMicroseconds();
	const uint64 rendererGPUAmbientPassTime = commonLocal.GetRendererGpuAmbientPassMicroseconds();
	const uint64 rendererGPUShadowAtlasTime = commonLocal.GetRendererGpuShadowAtlasPassMicroseconds();
	const uint64 rendererGPUInteractionsTime = commonLocal.GetRendererGpuInteractionsMicroseconds();
	const uint64 rendererGPUShaderPassesTime = commonLocal.GetRendererGpuShaderPassMicroseconds();
	const uint64 rendererGPU_TAATime = commonLocal.GetRendererGpuTAAMicroseconds();
	const uint64 rendererGPUPostProcessingTime = commonLocal.GetRendererGpuPostProcessingMicroseconds();

	// SRS - Calculate max fps and max frame time based on glConfig.displayFrequency if vsync enabled and lower than engine Hz, otherwise use com_engineHz_latched
	const int maxFPS = ( r_swapInterval.GetInteger() > 0 && glConfig.displayFrequency > 0 ? std::min( glConfig.displayFrequency, int( com_engineHz_latched ) ) : com_engineHz_latched );
	const int maxTime = ( 1000.0 / maxFPS ) * 1050; // slight 5% tolerance offset to avoid flickering of the stats

	// SRS - Frame idle and busy time calculations are based on direct frame-over-frame measurement relative to finishSyncTime
	const int64 frameIdleTime = int64( commonLocal.mainFrameTiming.startGameTime ) - int64( commonLocal.mainFrameTiming.finishSyncTime );
	const int64 frameBusyTime = int64( commonLocal.frameTiming.finishSyncTime ) - int64( commonLocal.mainFrameTiming.startGameTime );

	// SRS - Frame sync time represents swap buffer synchronization + game thread wait + other time spent outside of rendering
	const int64 frameSyncTime = int64( commonLocal.frameTiming.finishSyncTime ) - int64( commonLocal.mainFrameTiming.startRenderTime ) - int64( rendererBackEndTime );

	// SRS - GPU idle time is simply the difference between measured frame-over-frame time and GPU busy time (directly from GPU timers)
	const int64 rendererGPUIdleTime = frameBusyTime + frameIdleTime - rendererGPUTime;

#if 1

	// RB: use ImGui to show more detailed stats about the scene loads
	if( ImGuiHook::IsReadyToRender() )
	{
		// start smaller
		int32 statsWindowWidth = 320;
		int32 statsWindowHeight = 315;

		if( com_showFPS.GetInteger() > 2 )
		{
			statsWindowWidth += 230;
			statsWindowHeight += 120;
		}

		ImVec2 pos;
		pos.x = renderSystem->GetWidth() - statsWindowWidth;
		pos.y = 0;

		ImGui::SetNextWindowPos( pos );
		ImGui::SetNextWindowSize( ImVec2( statsWindowWidth, statsWindowHeight ) );

		static ImVec4 colorBlack	= ImVec4( 0.00f, 0.00f, 0.00f, 1.00f );
		static ImVec4 colorWhite	= ImVec4( 1.00f, 1.00f, 1.00f, 1.00f );
		static ImVec4 colorRed		= ImVec4( 1.00f, 0.00f, 0.00f, 1.00f );
		static ImVec4 colorGreen	= ImVec4( 0.00f, 1.00f, 0.00f, 1.00f );
		static ImVec4 colorBlue		= ImVec4( 0.00f, 0.00f, 1.00f, 1.00f );
		static ImVec4 colorYellow	= ImVec4( 1.00f, 1.00f, 0.00f, 1.00f );
		static ImVec4 colorMagenta	= ImVec4( 1.00f, 0.00f, 1.00f, 1.00f );
		static ImVec4 colorCyan		= ImVec4( 0.00f, 1.00f, 1.00f, 1.00f );
		static ImVec4 colorOrange	= ImVec4( 1.00f, 0.50f, 0.00f, 1.00f );
		static ImVec4 colorPurple	= ImVec4( 0.60f, 0.00f, 0.60f, 1.00f );
		static ImVec4 colorPink		= ImVec4( 0.73f, 0.40f, 0.48f, 1.00f );
		static ImVec4 colorBrown	= ImVec4( 0.40f, 0.35f, 0.08f, 1.00f );
		static ImVec4 colorLtGrey	= ImVec4( 0.75f, 0.75f, 0.75f, 1.00f );
		static ImVec4 colorMdGrey	= ImVec4( 0.50f, 0.50f, 0.50f, 1.00f );
		static ImVec4 colorDkGrey	= ImVec4( 0.25f, 0.25f, 0.25f, 1.00f );
		static ImVec4 colorGold		= ImVec4( 0.68f, 0.63f, 0.36f, 1.00f );

		ImGui::Begin( "Performance Stats" );

		static const int gfxNumValues = 3;

		static const char* gfxValues[gfxNumValues] =
		{
			"DX11",
			"DX12",
			"Vulkan",
		};

		const char* API = gfxValues[ int( deviceManager->GetGraphicsAPI() ) ];

		extern idCVar r_antiAliasing;

#if ID_MSAA
		static const int aaNumValues = 5;

		static const char* aaValues[aaNumValues] =
		{
			"None",
			"None",
			"SMAA 1X",
			"MSAA 2X",
			"MSAA 4X",
		};

		static const char* taaValues[aaNumValues] =
		{
			"None",
			"TAA",
			"TAA + SMAA 1X",
			"MSAA 2X",
			"MSAA 4X",
		};

		compile_time_assert( aaNumValues == ( ANTI_ALIASING_MSAA_4X + 1 ) );
#else
		static const int aaNumValues = 2;

		static const char* aaValues[aaNumValues] =
		{
			"None",
			"None",
		};

		static const char* taaValues[aaNumValues] =
		{
			"None",
			"TAA",
		};

		compile_time_assert( aaNumValues == ( ANTI_ALIASING_TAA + 1 ) );
#endif

		const char* aaMode = NULL;
		if( R_UseTemporalAA() )
		{
			aaMode = taaValues[ r_antiAliasing.GetInteger() ];
		}
		else
		{
			aaMode = aaValues[ r_antiAliasing.GetInteger() ];
		}

		idStr resolutionText;
		resolutionScale.GetConsoleText( resolutionText );

		int width = renderSystem->GetWidth();
		int height = renderSystem->GetHeight();

		ImGui::TextColored( colorCyan, "API: %s, AA[%i, %i]: %s, %s", API, width, height, aaMode, resolutionText.c_str() );

		ImGui::TextColored( colorGold, "Device: %s", deviceManager->GetRendererString() );

		ImGui::TextColored( colorLtGrey, "GENERAL: views:%i draws:%i tris:%i",
							commonLocal.stats_frontend.c_numViews,
							commonLocal.stats_backend.c_drawElements + commonLocal.stats_backend.c_shadowElements,
							( commonLocal.stats_backend.c_drawIndexes + commonLocal.stats_backend.c_shadowIndexes ) / 3 );

		if( com_showFPS.GetInteger() > 2 )
		{
			int atlasPercentage = idMath::Ftoi( 100.0f * ( commonLocal.stats_backend.c_shadowAtlasUsage / ( float )Square( r_shadowMapAtlasSize.GetInteger() ) ) );
			ImGui::TextColored( colorLtGrey, "SHADOWS: atlas usage:%2i%% views:%i draws:%i tris:%i",
								atlasPercentage,
								commonLocal.stats_backend.c_shadowViews,
								commonLocal.stats_backend.c_shadowElements,
								commonLocal.stats_backend.c_shadowIndexes / 3 );

			ImGui::TextColored( colorLtGrey, "DYNAMIC: callback:%-2i md5:%i dfrmVerts:%i dfrmTris:%i tangTris:%i guis:%i",
								commonLocal.stats_frontend.c_entityDefCallbacks,
								commonLocal.stats_frontend.c_generateMd5,
								commonLocal.stats_frontend.c_deformedVerts,
								commonLocal.stats_frontend.c_deformedIndexes / 3,
								commonLocal.stats_frontend.c_tangentIndexes / 3,
								commonLocal.stats_frontend.c_guiSurfs
							  );

			//ImGui::Text( "Cull: %i box in %i box out\n",
			//					commonLocal.stats_frontend.c_box_cull_in, commonLocal.stats_frontend.c_box_cull_out );

			ImGui::TextColored( colorLtGrey, "ADDMODEL: callback:%-2i createInteractions:%i createShadowVolumes:%i",
								commonLocal.stats_frontend.c_entityDefCallbacks,
								commonLocal.stats_frontend.c_createInteractions,
								commonLocal.stats_frontend.c_createShadowVolumes );

			ImGui::TextColored( colorLtGrey, "viewEntities:%-3i  shadowEntities:%-3i  viewLights:%i\n",	commonLocal.stats_frontend.c_visibleViewEntities,
								commonLocal.stats_frontend.c_shadowViewEntities,
								commonLocal.stats_frontend.c_viewLights );

			ImGui::TextColored( colorLtGrey, "UPDATES: entityUpdates:%-3i  entityRefs:%-3i  lightUpdates:%-2i  lightRefs:%i\n",
								commonLocal.stats_frontend.c_entityUpdates, commonLocal.stats_frontend.c_entityReferences,
								commonLocal.stats_frontend.c_lightUpdates, commonLocal.stats_frontend.c_lightReferences );
		}

		//ImGui::Text( "frameData: %i (%i)\n", frameData->frameMemoryAllocated.GetValue(), frameData->highWaterAllocated );

		//ImGui::Spacing();
		//ImGui::Spacing();
		ImGui::Spacing();

		if( com_showFPS.GetInteger() > 2 )
		{
			const char* overlay = va( "Average FPS %i", fps );

			ImGui::PlotLines( "Relative\nFrametime ms", previousTimesNormalized, FPS_FRAMES_HISTORY, valuesOffset, overlay, -10.0f, 10.0f, ImVec2( 0, 50 ) );
		}
		else
		{
			ImGui::TextColored( fps < maxFPS ? colorRed : colorYellow, "Average FPS %i", fps );
		}

		ImGui::Spacing();

		ImGui::TextColored( colorMdGrey,													"CPU                 GPU" );
		ImGui::TextColored( gameThreadTotalTime > maxTime ? colorRed : colorWhite,			"Game+RF: %5llu us   EarlyZ:       %5llu us", gameThreadTotalTime, rendererGPUEarlyZTime );
		ImGui::TextColored( gameThreadGameTime > maxTime ? colorRed : colorWhite,			"Game:    %5llu us   SSAO:         %5llu us", gameThreadGameTime, rendererGPU_SSAOTime );
		ImGui::TextColored( gameThreadRenderTime > maxTime ? colorRed : colorWhite,			"RF:      %5llu us   SSR:          %5llu us", gameThreadRenderTime, rendererGPU_SSRTime );
		ImGui::TextColored( rendererBackEndTime > maxTime ? colorRed : colorWhite,			"RB:      %5llu us   Ambient Pass: %5llu us", rendererBackEndTime, rendererGPUAmbientPassTime );
		ImGui::TextColored( rendererGPUShadowAtlasTime > maxTime ? colorRed : colorWhite,	"Shadows: %5llu us   Shadow Atlas: %5llu us", rendererShadowsTime, rendererGPUShadowAtlasTime );
		ImGui::TextColored( rendererGPUInteractionsTime > maxTime ? colorRed : colorWhite,	"Sync:    %5lld us   Interactions: %5llu us", frameSyncTime, rendererGPUInteractionsTime );
		ImGui::TextColored( rendererGPUShaderPassesTime > maxTime ? colorRed : colorWhite,	"                    Shader Pass:  %5llu us", rendererGPUShaderPassesTime );
		ImGui::TextColored( rendererGPU_TAATime > maxTime ? colorRed : colorWhite,			"                    TAA:          %5llu us", rendererGPU_TAATime );
		ImGui::TextColored( rendererGPUPostProcessingTime > maxTime ? colorRed : colorWhite, "                    PostFX:       %5llu us", rendererGPUPostProcessingTime );
		ImGui::TextColored( frameBusyTime > maxTime || rendererGPUTime > maxTime ? colorRed : colorWhite, "Total:   %5lld us   Total:        %5lld us", frameBusyTime, rendererGPUTime );
		ImGui::TextColored( colorWhite,														"Idle:    %5lld us   Idle:         %5lld us", frameIdleTime, rendererGPUIdleTime );

		ImGui::End();
	}

	return y;
#else

	// print the resolution scale so we can tell when we are at reduced resolution
	idStr resolutionText;
	resolutionScale.GetConsoleText( resolutionText );
	int w = resolutionText.Length() * BIGCHAR_WIDTH;
	renderSystem->DrawBigStringExt( LOCALSAFE_RIGHT - w, idMath::Ftoi( y ) + 2, resolutionText.c_str(), colorWhite, true );

	y += SMALLCHAR_HEIGHT + 4;
	idStr timeStr;
	timeStr.Format( "%sG+RF: %4d", gameThreadTotalTime > maxTime ? S_COLOR_RED : "", gameThreadTotalTime );
	w = timeStr.LengthWithoutColors() * SMALLCHAR_WIDTH;
	renderSystem->DrawSmallStringExt( LOCALSAFE_RIGHT - w, idMath::Ftoi( y ) + 2, timeStr.c_str(), colorWhite, false );
	y += SMALLCHAR_HEIGHT + 4;

	timeStr.Format( "%sG: %4d", gameThreadGameTime > maxTime ? S_COLOR_RED : "", gameThreadGameTime );
	w = timeStr.LengthWithoutColors() * SMALLCHAR_WIDTH;
	renderSystem->DrawSmallStringExt( LOCALSAFE_RIGHT - w, idMath::Ftoi( y ) + 2, timeStr.c_str(), colorWhite, false );
	y += SMALLCHAR_HEIGHT + 4;

	timeStr.Format( "%sRF: %4d", gameThreadRenderTime > maxTime ? S_COLOR_RED : "", gameThreadRenderTime );
	w = timeStr.LengthWithoutColors() * SMALLCHAR_WIDTH;
	renderSystem->DrawSmallStringExt( LOCALSAFE_RIGHT - w, idMath::Ftoi( y ) + 2, timeStr.c_str(), colorWhite, false );
	y += SMALLCHAR_HEIGHT + 4;

	timeStr.Format( "%sRB: %4.1f", rendererBackEndTime > maxTime * 1000 ? S_COLOR_RED : "", rendererBackEndTime / 1000.0f );
	w = timeStr.LengthWithoutColors() * SMALLCHAR_WIDTH;
	renderSystem->DrawSmallStringExt( LOCALSAFE_RIGHT - w, idMath::Ftoi( y ) + 2, timeStr.c_str(), colorWhite, false );
	y += SMALLCHAR_HEIGHT + 4;

	timeStr.Format( "%sSV: %4.1f", rendererShadowsTime > maxTime * 1000 ? S_COLOR_RED : "", rendererShadowsTime / 1000.0f );
	w = timeStr.LengthWithoutColors() * SMALLCHAR_WIDTH;
	renderSystem->DrawSmallStringExt( LOCALSAFE_RIGHT - w, idMath::Ftoi( y ) + 2, timeStr.c_str(), colorWhite, false );
	y += SMALLCHAR_HEIGHT + 4;

	timeStr.Format( "%sIDLE: %4.1f", rendererGPUIdleTime > maxTime * 1000 ? S_COLOR_RED : "", rendererGPUIdleTime / 1000.0f );
	w = timeStr.LengthWithoutColors() * SMALLCHAR_WIDTH;
	renderSystem->DrawSmallStringExt( LOCALSAFE_RIGHT - w, idMath::Ftoi( y ) + 2, timeStr.c_str(), colorWhite, false );
	y += SMALLCHAR_HEIGHT + 4;

	timeStr.Format( "%sGPU: %4.1f", rendererGPUTime > maxTime * 1000 ? S_COLOR_RED : "", rendererGPUTime / 1000.0f );
	w = timeStr.LengthWithoutColors() * SMALLCHAR_WIDTH;
	renderSystem->DrawSmallStringExt( LOCALSAFE_RIGHT - w, idMath::Ftoi( y ) + 2, timeStr.c_str(), colorWhite, false );

	return y + BIGCHAR_HEIGHT + 4;
#endif
}

/*
==================
idConsoleLocal::DrawMemoryUsage
==================
*/
float idConsoleLocal::DrawMemoryUsage( float y )
{
	return y;
}

//=========================================================================

/*
==============
Con_Clear_f
==============
*/
static void Con_Clear_f( const idCmdArgs& args )
{
	localConsole.Clear();
}

/*
==============
Con_Dump_f
==============
*/
static void Con_Dump_f( const idCmdArgs& args )
{
	if( args.Argc() != 2 )
	{
		common->Printf( "usage: conDump <filename>\n" );
		return;
	}

	idStr fileName = args.Argv( 1 );
	fileName.DefaultFileExtension( ".txt" );

	common->Printf( "Dumped console text to %s.\n", fileName.c_str() );

	localConsole.Dump( fileName.c_str() );
}

/*
==============
idConsoleLocal::Init
==============
*/
void idConsoleLocal::Init()
{
	int		i;

	keyCatching = false;

	LOCALSAFE_LEFT		= 0;
	LOCALSAFE_RIGHT		= SCREEN_WIDTH - LOCALSAFE_LEFT;
	LOCALSAFE_TOP		= 24;
	LOCALSAFE_BOTTOM	= SCREEN_HEIGHT - LOCALSAFE_TOP;
	LOCALSAFE_WIDTH		= LOCALSAFE_RIGHT - LOCALSAFE_LEFT;
	LOCALSAFE_HEIGHT	= LOCALSAFE_BOTTOM - LOCALSAFE_TOP;

	LINE_WIDTH = ( ( LOCALSAFE_WIDTH / SMALLCHAR_WIDTH ) - 2 );
	TOTAL_LINES = ( CON_TEXTSIZE / LINE_WIDTH );

	lastKeyEvent = -1;
	nextKeyEvent = CONSOLE_FIRSTREPEAT;

	consoleField.Clear();
	consoleField.SetWidthInChars( LINE_WIDTH );

	for( i = 0 ; i < COMMAND_HISTORY ; i++ )
	{
		historyEditLines[i].Clear();
		historyEditLines[i].SetWidthInChars( LINE_WIDTH );
	}

	cmdSystem->AddCommand( "clear", Con_Clear_f, CMD_FL_SYSTEM, "clears the console" );
	cmdSystem->AddCommand( "conDump", Con_Dump_f, CMD_FL_SYSTEM, "dumps the console text to a file" );
}

/*
==============
idConsoleLocal::Shutdown
==============
*/
void idConsoleLocal::Shutdown()
{
	cmdSystem->RemoveCommand( "clear" );
	cmdSystem->RemoveCommand( "conDump" );

	debugGraphs.DeleteContents( true );
}

/*
================
idConsoleLocal::Active
================
*/
bool	idConsoleLocal::Active()
{
	return keyCatching;
}

/*
================
idConsoleLocal::ClearNotifyLines
================
*/
void	idConsoleLocal::ClearNotifyLines()
{
	int		i;

	for( i = 0 ; i < NUM_CON_TIMES ; i++ )
	{
		times[i] = 0;
	}
}

/*
================
idConsoleLocal::Open
================
*/
void	idConsoleLocal::Open()
{
	if( keyCatching )
	{
		return;    // already open
	}

	consoleField.ClearAutoComplete();
	consoleField.Clear();
	keyCatching = true;
	SetDisplayFraction( 0.5f );
}

/*
================
idConsoleLocal::Close
================
*/
void	idConsoleLocal::Close()
{
	keyCatching = false;
	SetDisplayFraction( 0 );
	displayFrac = 0;	// don't scroll to that point, go immediately
	ClearNotifyLines();
}

/*
================
idConsoleLocal::Clear
================
*/
void idConsoleLocal::Clear()
{
	int		i;

	for( i = 0 ; i < CON_TEXTSIZE ; i++ )
	{
		text[i] = ( idStr::ColorIndex( C_COLOR_WHITE ) << 8 ) | ' ';
	}

	Bottom();		// go to end
}

/*
================
idConsoleLocal::Dump

Save the console contents out to a file
================
*/
void idConsoleLocal::Dump( const char* fileName )
{
	int		l, x, i;
	short* 	line;
	idFile* f;
	char*	 buffer = ( char* )alloca( LINE_WIDTH + 3 );

	f = fileSystem->OpenFileWrite( fileName );
	if( !f )
	{
		common->Warning( "couldn't open %s", fileName );
		return;
	}

	// skip empty lines
	l = current - TOTAL_LINES + 1;
	if( l < 0 )
	{
		l = 0;
	}
	for( ; l <= current ; l++ )
	{
		line = text + ( l % TOTAL_LINES ) * LINE_WIDTH;
		for( x = 0; x < LINE_WIDTH; x++ )
			if( ( line[x] & 0xff ) > ' ' )
			{
				break;
			}
		if( x != LINE_WIDTH )
		{
			break;
		}
	}

	// write the remaining lines
	for( ; l <= current; l++ )
	{
		line = text + ( l % TOTAL_LINES ) * LINE_WIDTH;
		for( i = 0; i < LINE_WIDTH; i++ )
		{
			buffer[i] = line[i] & 0xff;
		}
		for( x = LINE_WIDTH - 1; x >= 0; x-- )
		{
			if( buffer[x] <= ' ' )
			{
				buffer[x] = 0;
			}
			else
			{
				break;
			}
		}
		buffer[x + 1] = '\r';
		buffer[x + 2] = '\n';
		buffer[x + 3] = 0;
		f->Write( buffer, strlen( buffer ) );
	}

	fileSystem->CloseFile( f );
}

/*
==============
idConsoleLocal::Resize
==============
*/
void idConsoleLocal::Resize()
{
	if( renderSystem->GetVirtualWidth() == lastVirtualScreenWidth && renderSystem->GetVirtualHeight() == lastVirtualScreenHeight )
	{
		return;
	}

	lastVirtualScreenWidth = renderSystem->GetVirtualWidth();
	lastVirtualScreenHeight = renderSystem->GetVirtualHeight();
	LOCALSAFE_RIGHT		= renderSystem->GetVirtualWidth() - LOCALSAFE_LEFT;
	LOCALSAFE_BOTTOM	= renderSystem->GetVirtualHeight() - LOCALSAFE_TOP;
	LOCALSAFE_WIDTH		= LOCALSAFE_RIGHT - LOCALSAFE_LEFT;
	LOCALSAFE_HEIGHT	= LOCALSAFE_BOTTOM - LOCALSAFE_TOP;
}

/*
================
idConsoleLocal::PageUp
================
*/
void idConsoleLocal::PageUp()
{
	display -= 2;
	if( current - display >= TOTAL_LINES )
	{
		display = current - TOTAL_LINES + 1;
	}
}

/*
================
idConsoleLocal::PageDown
================
*/
void idConsoleLocal::PageDown()
{
	display += 2;
	if( display > current )
	{
		display = current;
	}
}

/*
================
idConsoleLocal::Top
================
*/
void idConsoleLocal::Top()
{
	display = 0;
}

/*
================
idConsoleLocal::Bottom
================
*/
void idConsoleLocal::Bottom()
{
	display = current;
}


/*
=============================================================================

CONSOLE LINE EDITING

==============================================================================
*/

/*
====================
KeyDownEvent

Handles history and console scrollback
====================
*/
void idConsoleLocal::KeyDownEvent( int key )
{

	// Execute F key bindings
	if( key >= K_F1 && key <= K_F12 )
	{
		idKeyInput::ExecKeyBinding( key );
		return;
	}

	// ctrl-L clears screen
	if( key == K_L && ( idKeyInput::IsDown( K_LCTRL ) || idKeyInput::IsDown( K_RCTRL ) ) )
	{
		Clear();
		return;
	}

	// enter finishes the line
	if( key == K_ENTER || key == K_KP_ENTER )
	{

		common->Printf( "]%s\n", consoleField.GetBuffer() );

		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, consoleField.GetBuffer() );	// valid command
		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "\n" );

		// copy line to history buffer

		if( consoleField.GetBuffer()[ 0 ] != '\n' && consoleField.GetBuffer()[ 0 ] != '\0' )
		{
			consoleHistory.AddToHistory( consoleField.GetBuffer() );
		}

		consoleField.Clear();
		consoleField.SetWidthInChars( LINE_WIDTH );

		const bool captureToImage = false;
		common->UpdateScreen( captureToImage );// force an update, because the command
		// may take some time
		return;
	}

	// command completion

	if( key == K_TAB )
	{
		consoleField.AutoComplete();
		return;
	}

	// command history (ctrl-p ctrl-n for unix style)

	if( ( key == K_UPARROW ) ||
			( key == K_P && ( idKeyInput::IsDown( K_LCTRL ) || idKeyInput::IsDown( K_RCTRL ) ) ) )
	{
		idStr hist = consoleHistory.RetrieveFromHistory( true );
		if( !hist.IsEmpty() )
		{
			consoleField.SetBuffer( hist );
		}
		return;
	}

	if( ( key == K_DOWNARROW ) ||
			( key == K_N && ( idKeyInput::IsDown( K_LCTRL ) || idKeyInput::IsDown( K_RCTRL ) ) ) )
	{
		idStr hist = consoleHistory.RetrieveFromHistory( false );
		if( !hist.IsEmpty() )
		{
			consoleField.SetBuffer( hist );
		}
		else // DG: if no more lines are in the history, show a blank line again
		{
			consoleField.Clear();
		} // DG end

		return;
	}

	// console scrolling
	if( key == K_PGUP )
	{
		PageUp();
		lastKeyEvent = eventLoop->Milliseconds();
		nextKeyEvent = CONSOLE_FIRSTREPEAT;
		return;
	}

	if( key == K_PGDN )
	{
		PageDown();
		lastKeyEvent = eventLoop->Milliseconds();
		nextKeyEvent = CONSOLE_FIRSTREPEAT;
		return;
	}

	if( key == K_MWHEELUP )
	{
		PageUp();
		return;
	}

	if( key == K_MWHEELDOWN )
	{
		PageDown();
		return;
	}

	// ctrl-home = top of console
	if( key == K_HOME && ( idKeyInput::IsDown( K_LCTRL ) || idKeyInput::IsDown( K_RCTRL ) ) )
	{
		Top();
		return;
	}

	// ctrl-end = bottom of console
	if( key == K_END && ( idKeyInput::IsDown( K_LCTRL ) || idKeyInput::IsDown( K_RCTRL ) ) )
	{
		Bottom();
		return;
	}

	// pass to the normal editline routine
	consoleField.KeyDownEvent( key );
}

/*
==============
Scroll
deals with scrolling text because we don't have key repeat
==============
*/
void idConsoleLocal::Scroll()
{
	if( lastKeyEvent == -1 || ( lastKeyEvent + 200 ) > eventLoop->Milliseconds() )
	{
		return;
	}
	// console scrolling
	if( idKeyInput::IsDown( K_PGUP ) )
	{
		PageUp();
		nextKeyEvent = CONSOLE_REPEAT;
		return;
	}

	if( idKeyInput::IsDown( K_PGDN ) )
	{
		PageDown();
		nextKeyEvent = CONSOLE_REPEAT;
		return;
	}
}

/*
==============
SetDisplayFraction

Causes the console to start opening the desired amount.
==============
*/
void idConsoleLocal::SetDisplayFraction( float frac )
{
	finalFrac = frac;
	fracTime = Sys_Milliseconds();
}

/*
==============
UpdateDisplayFraction

Scrolls the console up or down based on conspeed
==============
*/
void idConsoleLocal::UpdateDisplayFraction()
{
	if( con_speed.GetFloat() <= 0.1f )
	{
		fracTime = Sys_Milliseconds();
		displayFrac = finalFrac;
		return;
	}

	// scroll towards the destination height
	if( finalFrac < displayFrac )
	{
		displayFrac -= con_speed.GetFloat() * ( Sys_Milliseconds() - fracTime ) * 0.001f;
		if( finalFrac > displayFrac )
		{
			displayFrac = finalFrac;
		}
		fracTime = Sys_Milliseconds();
	}
	else if( finalFrac > displayFrac )
	{
		displayFrac += con_speed.GetFloat() * ( Sys_Milliseconds() - fracTime ) * 0.001f;
		if( finalFrac < displayFrac )
		{
			displayFrac = finalFrac;
		}
		fracTime = Sys_Milliseconds();
	}
}

/*
==============
ProcessEvent
==============
*/
bool	idConsoleLocal::ProcessEvent( const sysEvent_t* event, bool forceAccept )
{
	const bool consoleKey = event->evType == SE_KEY && event->evValue == K_GRAVE && com_allowConsole.GetBool();

	// we always catch the console key event
	if( !forceAccept && consoleKey )
	{
		// ignore up events
		if( event->evValue2 == 0 )
		{
			return true;
		}

		consoleField.ClearAutoComplete();

		// a down event will toggle the destination lines
		if( keyCatching )
		{
			Close();
			Sys_GrabMouseCursor( true );
		}
		else
		{
			consoleField.Clear();
			keyCatching = true;
			if( idKeyInput::IsDown( K_LSHIFT ) || idKeyInput::IsDown( K_RSHIFT ) )
			{
				// if the shift key is down, don't open the console as much
				SetDisplayFraction( 0.2f );
			}
			else
			{
				SetDisplayFraction( 0.5f );
			}
		}
		return true;
	}

	// if we aren't key catching, dump all the other events
	if( !forceAccept && !keyCatching )
	{
		return false;
	}

	// handle key and character events
	if( event->evType == SE_CHAR )
	{
		// never send the console key as a character
		if( event->evValue != '`' && event->evValue != '~' )
		{
			consoleField.CharEvent( event->evValue );
		}
		return true;
	}

	if( event->evType == SE_KEY )
	{
		// ignore up key events
		if( event->evValue2 == 0 )
		{
			return true;
		}

		KeyDownEvent( event->evValue );
		return true;
	}

	// we don't handle things like mouse, joystick, and network packets
	return false;
}

/*
==============================================================================

PRINTING

==============================================================================
*/

/*
===============
Linefeed
===============
*/
void idConsoleLocal::Linefeed()
{
	int		i;

	// mark time for transparent overlay
	if( current >= 0 )
	{
		times[current % NUM_CON_TIMES] = Sys_Milliseconds();
	}

	x = 0;
	if( display == current )
	{
		display++;
	}
	current++;
	for( i = 0; i < LINE_WIDTH; i++ )
	{
		int offset = ( ( unsigned int )current % TOTAL_LINES ) * LINE_WIDTH + i;
		text[offset] = ( idStr::ColorIndex( C_COLOR_WHITE ) << 8 ) | ' ';
	}
}


/*
================
Print

Handles cursor positioning, line wrapping, etc
================
*/
void idConsoleLocal::Print( const char* txt )
{
	int		y;
	int		c, l;
	int		color;

	if( TOTAL_LINES == 0 )
	{
		// not yet initialized
		return;
	}

	color = idStr::ColorIndex( C_COLOR_WHITE );

	while( ( c = *( const unsigned char* )txt ) != 0 )
	{
		if( idStr::IsColor( txt ) )
		{
			if( *( txt + 1 ) == C_COLOR_DEFAULT )
			{
				color = idStr::ColorIndex( C_COLOR_WHITE );
			}
			else
			{
				color = idStr::ColorIndex( *( txt + 1 ) );
			}
			txt += 2;
			continue;
		}

		y = current % TOTAL_LINES;

		// if we are about to print a new word, check to see
		// if we should wrap to the new line
		if( c > ' ' && ( x == 0 || text[y * LINE_WIDTH + x - 1] <= ' ' ) )
		{
			// count word length
			for( l = 0 ; l < LINE_WIDTH ; l++ )
			{
				if( txt[l] <= ' ' )
				{
					break;
				}
			}

			// word wrap
			if( l != LINE_WIDTH && ( x + l >= LINE_WIDTH ) )
			{
				Linefeed();
			}
		}

		txt++;

		switch( c )
		{
			case '\n':
				Linefeed();
				break;
			case '\t':
				do
				{
					text[y * LINE_WIDTH + x] = ( color << 8 ) | ' ';
					x++;
					if( x >= LINE_WIDTH )
					{
						Linefeed();
						x = 0;
					}
				}
				while( x & 3 );
				break;
			case '\r':
				x = 0;
				break;
			default:	// display character and advance
				text[y * LINE_WIDTH + x] = ( color << 8 ) | c;
				x++;
				if( x >= LINE_WIDTH )
				{
					Linefeed();
					x = 0;
				}
				break;
		}
	}


	// mark time for transparent overlay
	if( current >= 0 )
	{
		times[current % NUM_CON_TIMES] = Sys_Milliseconds();
	}
}


/*
==============================================================================

DRAWING

==============================================================================
*/


/*
================
DrawInput

Draw the editline after a ] prompt
================
*/
void idConsoleLocal::DrawInput()
{
	int y, autoCompleteLength;

	y = vislines - ( SMALLCHAR_HEIGHT * 2 );

	if( consoleField.GetAutoCompleteLength() != 0 )
	{
		autoCompleteLength = strlen( consoleField.GetBuffer() ) - consoleField.GetAutoCompleteLength();

		if( autoCompleteLength > 0 )
		{
			renderSystem->DrawFilled( idVec4( 0.8f, 0.2f, 0.2f, 0.45f ),
									  LOCALSAFE_LEFT + 2 * SMALLCHAR_WIDTH + consoleField.GetAutoCompleteLength() * SMALLCHAR_WIDTH,
									  y + 2, autoCompleteLength * SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT - 2 );
		}
	}

	renderSystem->SetColor( idStr::ColorForIndex( C_COLOR_WHITE ) );

	renderSystem->DrawSmallChar( LOCALSAFE_LEFT + 1 * SMALLCHAR_WIDTH, y, ']' );

	consoleField.Draw( LOCALSAFE_LEFT + 2 * SMALLCHAR_WIDTH, y, renderSystem->GetVirtualWidth() - 3 * SMALLCHAR_WIDTH, true );
}


/*
================
DrawNotify

Draws the last few lines of output transparently over the game top
================
*/
void idConsoleLocal::DrawNotify()
{
	int		x, v;
	short*	text_p;
	int		i;
	int		time;
	int		currentColor;

	if( con_noPrint.GetBool() )
	{
		return;
	}

	currentColor = idStr::ColorIndex( C_COLOR_WHITE );
	renderSystem->SetColor( idStr::ColorForIndex( currentColor ) );

	v = 0;
	for( i = current - NUM_CON_TIMES + 1; i <= current; i++ )
	{
		if( i < 0 )
		{
			continue;
		}
		time = times[i % NUM_CON_TIMES];
		if( time == 0 )
		{
			continue;
		}
		time = Sys_Milliseconds() - time;
		if( time > con_notifyTime.GetFloat() * 1000 )
		{
			continue;
		}
		text_p = text + ( i % TOTAL_LINES ) * LINE_WIDTH;

		for( x = 0; x < LINE_WIDTH; x++ )
		{
			if( ( text_p[x] & 0xff ) == ' ' )
			{
				continue;
			}
			if( idStr::ColorIndex( text_p[x] >> 8 ) != currentColor )
			{
				currentColor = idStr::ColorIndex( text_p[x] >> 8 );
				renderSystem->SetColor( idStr::ColorForIndex( currentColor ) );
			}
			renderSystem->DrawSmallChar( LOCALSAFE_LEFT + ( x + 1 )*SMALLCHAR_WIDTH, v, text_p[x] & 0xff );
		}

		v += SMALLCHAR_HEIGHT;
	}

	renderSystem->SetColor( colorWhite );
}

/*
================
DrawSolidConsole

Draws the console with the solid background
================
*/
void idConsoleLocal::DrawSolidConsole( float frac )
{
	int				i, x;
	float			y;
	int				rows;
	short*			text_p;
	int				row;
	int				lines;
	int				currentColor;

	lines = idMath::Ftoi( renderSystem->GetVirtualHeight() * frac );
	if( lines <= 0 )
	{
		return;
	}

	if( lines > renderSystem->GetVirtualHeight() )
	{
		lines = renderSystem->GetVirtualHeight();
	}

	// draw the background
	y = frac * renderSystem->GetVirtualHeight() - 2;
	if( y < 1.0f )
	{
		y = 0.0f;
	}
	else
	{
		renderSystem->DrawFilled( idVec4( 0.0f, 0.0f, 0.0f, 0.75f ), 0, 0, renderSystem->GetVirtualWidth(), y );
	}

	renderSystem->DrawFilled( colorGold, 0, y, renderSystem->GetVirtualWidth(), 2 );

	// draw the version number

	renderSystem->SetColor( colorGold );

	// RB begin
	//idStr version = va( "%s.%i.%i", ENGINE_VERSION, BUILD_NUMBER, BUILD_NUMBER_MINOR );
	idStr version = va( "%s %s", ENGINE_VERSION, BUILD_STRING );
	//idStr version = com_version.GetString();
	// RB end

	i = version.Length();

#define VERSION_LINE_SPACE (SMALLCHAR_HEIGHT + 4)

	for( x = 0; x < i; x++ )
	{
		renderSystem->DrawSmallChar( LOCALSAFE_WIDTH - ( i - x ) * SMALLCHAR_WIDTH,
									 ( lines - ( SMALLCHAR_HEIGHT + SMALLCHAR_HEIGHT / 4 ) ) - VERSION_LINE_SPACE - VERSION_LINE_SPACE, version[x] );

	}
// jmarshall
	idStr branchVersion = va( "Branch %s", ENGINE_BRANCH );
	i = branchVersion.Length();

	for( x = 0; x < i; x++ )
	{
		renderSystem->DrawSmallChar( LOCALSAFE_WIDTH - ( i - x ) * SMALLCHAR_WIDTH,
									 ( lines - ( SMALLCHAR_HEIGHT + SMALLCHAR_HEIGHT / 2 ) ) - ( VERSION_LINE_SPACE - 2 ), branchVersion[x] );

	}

	idStr builddate = va( "%s %s", __DATE__, __TIME__ );
	i = builddate.Length();

	for( x = 0; x < i; x++ )
	{
		renderSystem->DrawSmallChar( LOCALSAFE_WIDTH - ( i - x ) * SMALLCHAR_WIDTH,
									 ( lines - ( SMALLCHAR_HEIGHT + SMALLCHAR_HEIGHT / 2 ) ), builddate[x] );

	}
// jmarshall end

	// draw the text
	vislines = lines;
	rows = ( lines - SMALLCHAR_WIDTH ) / SMALLCHAR_WIDTH;		// rows of text to draw

	y = lines - ( SMALLCHAR_HEIGHT * 3 );

	// draw from the bottom up
	if( display != current )
	{
		// draw arrows to show the buffer is backscrolled
		renderSystem->SetColor( idStr::ColorForIndex( C_COLOR_WHITE ) );
		for( x = 0; x < LINE_WIDTH; x += 4 )
		{
			renderSystem->DrawSmallChar( LOCALSAFE_LEFT + ( x + 1 )*SMALLCHAR_WIDTH, idMath::Ftoi( y ), '^' );
		}
		y -= SMALLCHAR_HEIGHT;
		rows--;
	}

	row = display;

	if( x == 0 )
	{
		row--;
	}

	currentColor = idStr::ColorIndex( C_COLOR_WHITE );
	renderSystem->SetColor( idStr::ColorForIndex( currentColor ) );

	for( i = 0; i < rows; i++, y -= SMALLCHAR_HEIGHT, row-- )
	{
		if( row < 0 )
		{
			break;
		}
		if( current - row >= TOTAL_LINES )
		{
			// past scrollback wrap point
			continue;
		}

		text_p = text + ( row % TOTAL_LINES ) * LINE_WIDTH;

		for( x = 0; x < LINE_WIDTH; x++ )
		{
			if( ( text_p[x] & 0xff ) == ' ' )
			{
				continue;
			}

			if( idStr::ColorIndex( text_p[x] >> 8 ) != currentColor )
			{
				currentColor = idStr::ColorIndex( text_p[x] >> 8 );
				renderSystem->SetColor( idStr::ColorForIndex( currentColor ) );
			}
			renderSystem->DrawSmallChar( LOCALSAFE_LEFT + ( x + 1 )*SMALLCHAR_WIDTH, idMath::Ftoi( y ), text_p[x] & 0xff );
		}
	}

	// draw the input prompt, user text, and cursor if desired
	DrawInput();

	renderSystem->SetColor( colorWhite );
}


/*
==============
Draw

ForceFullScreen is used by the editor
==============
*/
void idConsoleLocal::Draw( bool forceFullScreen )
{
	Resize();

	if( forceFullScreen )
	{
		// if we are forced full screen because of a disconnect,
		// we want the console closed when we go back to a session state
		Close();
		// we are however catching keyboard input
		keyCatching = true;
	}

	Scroll();

	UpdateDisplayFraction();

	if( forceFullScreen )
	{
		DrawSolidConsole( 1.0f );
	}
	else if( displayFrac )
	{
		DrawSolidConsole( displayFrac );
	}
	else
	{
		// only draw the notify lines if the developer cvar is set,
		// or we are a debug build
		if( !con_noPrint.GetBool() )
		{
			DrawNotify();
		}
	}

	float lefty = LOCALSAFE_TOP;
	float righty = LOCALSAFE_TOP;
	float centery = LOCALSAFE_TOP;

	if( com_showFPS.GetBool() )
	{
		righty = DrawFPS( righty );
	}
	if( com_showMemoryUsage.GetBool() )
	{
		righty = DrawMemoryUsage( righty );
	}

	DrawOverlayText( lefty, righty, centery );
	DrawDebugGraphs();
}

/*
========================
idConsoleLocal::PrintOverlay
========================
*/
void idConsoleLocal::PrintOverlay( idOverlayHandle& handle, justify_t justify, const char* text, ... )
{
	if( handle.index >= 0 && handle.index < overlayText.Num() )
	{
		if( overlayText[handle.index].time == handle.time )
		{
			return;
		}
	}

	char string[MAX_PRINT_MSG];
	va_list argptr;
	va_start( argptr, text );
	idStr::vsnPrintf( string, sizeof( string ), text, argptr );
	va_end( argptr );

	overlayText_t& overlay = overlayText.Alloc();
	overlay.text = string;
	overlay.justify = justify;
	overlay.time = Sys_Milliseconds();

	handle.index = overlayText.Num() - 1;
	handle.time = overlay.time;
}

/*
========================
idConsoleLocal::DrawOverlayText
========================
*/
void idConsoleLocal::DrawOverlayText( float& leftY, float& rightY, float& centerY )
{
	for( int i = 0; i < overlayText.Num(); i++ )
	{
		const idStr& text = overlayText[i].text;

		int maxWidth = 0;
		int numLines = 0;
		for( int j = 0; j < text.Length(); j++ )
		{
			int width = 1;
			for( ; j < text.Length() && text[j] != '\n'; j++ )
			{
				width++;
			}
			numLines++;
			if( width > maxWidth )
			{
				maxWidth = width;
			}
		}

		idVec4 bgColor( 0.0f, 0.0f, 0.0f, 0.75f );

		const float width = maxWidth * SMALLCHAR_WIDTH;
		const float height = numLines * ( SMALLCHAR_HEIGHT + 4 );
		const float bgAdjust = - 0.5f * SMALLCHAR_WIDTH;
		if( overlayText[i].justify == JUSTIFY_LEFT )
		{
			renderSystem->DrawFilled( bgColor, LOCALSAFE_LEFT + bgAdjust, leftY, width, height );
		}
		else if( overlayText[i].justify == JUSTIFY_RIGHT )
		{
			renderSystem->DrawFilled( bgColor, LOCALSAFE_RIGHT - width + bgAdjust, rightY, width, height );
		}
		else if( overlayText[i].justify == JUSTIFY_CENTER_LEFT || overlayText[i].justify == JUSTIFY_CENTER_RIGHT )
		{
			renderSystem->DrawFilled( bgColor, LOCALSAFE_LEFT + ( LOCALSAFE_WIDTH - width + bgAdjust ) * 0.5f, centerY, width, height );
		}
		else
		{
			assert( false );
		}

		idStr singleLine;
		for( int j = 0; j < text.Length(); j += singleLine.Length() + 1 )
		{
			singleLine = "";
			for( int k = j; k < text.Length() && text[k] != '\n'; k++ )
			{
				singleLine.Append( text[k] );
			}
			if( overlayText[i].justify == JUSTIFY_LEFT )
			{
				DrawTextLeftAlign( LOCALSAFE_LEFT, leftY, "%s", singleLine.c_str() );
			}
			else if( overlayText[i].justify == JUSTIFY_RIGHT )
			{
				DrawTextRightAlign( LOCALSAFE_RIGHT, rightY, "%s", singleLine.c_str() );
			}
			else if( overlayText[i].justify == JUSTIFY_CENTER_LEFT )
			{
				DrawTextLeftAlign( LOCALSAFE_LEFT + ( LOCALSAFE_WIDTH - width ) * 0.5f, centerY, "%s", singleLine.c_str() );
			}
			else if( overlayText[i].justify == JUSTIFY_CENTER_RIGHT )
			{
				DrawTextRightAlign( LOCALSAFE_LEFT + ( LOCALSAFE_WIDTH + width ) * 0.5f, centerY, "%s", singleLine.c_str() );
			}
			else
			{
				assert( false );
			}
		}
	}
	overlayText.SetNum( 0 );
}

/*
========================
idConsoleLocal::CreateGraph
========================
*/
idDebugGraph* idConsoleLocal::CreateGraph( int numItems )
{
	idDebugGraph* graph = new( TAG_SYSTEM ) idDebugGraph( numItems );
	debugGraphs.Append( graph );
	return graph;
}

/*
========================
idConsoleLocal::DestroyGraph
========================
*/
void idConsoleLocal::DestroyGraph( idDebugGraph* graph )
{
	debugGraphs.Remove( graph );
	delete graph;
}

/*
========================
idConsoleLocal::DrawDebugGraphs
========================
*/
void idConsoleLocal::DrawDebugGraphs()
{
	for( int i = 0; i < debugGraphs.Num(); i++ )
	{
		debugGraphs[i]->Render( renderSystem );
	}
}
