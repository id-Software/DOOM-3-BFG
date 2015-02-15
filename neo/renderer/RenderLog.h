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
#ifndef __RENDERLOG_H__
#define __RENDERLOG_H__

/*
================================================================================================
Contains the RenderLog declaration.
================================================================================================
*/

#if defined(ID_RETAIL) && !defined(ID_RETAIL_INTERNAL)
#define STUB_RENDER_LOG
#endif

enum renderLogMainBlock_t
{
	MRB_NONE,
	MRB_BEGIN_DRAWING_VIEW,
	MRB_FILL_DEPTH_BUFFER,
	MRB_DRAW_INTERACTIONS,
	MRB_DRAW_SHADER_PASSES,
	MRB_FOG_ALL_LIGHTS,
	MRB_DRAW_SHADER_PASSES_POST,
	MRB_DRAW_DEBUG_TOOLS,
	MRB_CAPTURE_COLORBUFFER,
	MRB_POSTPROCESS,
	MRB_GPU_SYNC,
	MRB_END_FRAME,
	MRB_BINK_FRAME,
	MRB_BINK_NEXT_FRAME,
	MRB_TOTAL,
	MRB_MAX
};

// these are used to make sure each Indent() is properly paired with an Outdent()
enum renderLogIndentLabel_t
{
	RENDER_LOG_INDENT_DEFAULT,
	RENDER_LOG_INDENT_MAIN_BLOCK,
	RENDER_LOG_INDENT_BLOCK,
	RENDER_LOG_INDENT_TEST
};

// using this macro avoids printf parameter overhead if the renderlog isn't active
#define RENDERLOG_PRINTF( ... ) if ( renderLog.activeLevel ) renderLog.Printf( __VA_ARGS__ );

#if !defined( STUB_RENDER_LOG )

/*
================================================
idRenderLog contains block-based performance-tuning information. It combines
logfile, and msec accumulation code.
================================================
*/
class idRenderLog
{
public:
	idRenderLog();
	
	void		StartFrame();
	void		EndFrame();
	void		Close();
	int			Active()
	{
		return activeLevel;    // returns greater than 1 for more detailed logging
	}
	
	// The label must be a constant string literal and may not point to a temporary.
	void		OpenMainBlock( renderLogMainBlock_t block );
	void		CloseMainBlock();
	
	void		OpenBlock( const char* label );
	void		CloseBlock();
	
	void		Indent( renderLogIndentLabel_t label = RENDER_LOG_INDENT_DEFAULT );
	void		Outdent( renderLogIndentLabel_t label = RENDER_LOG_INDENT_DEFAULT );
	
	void		Printf( VERIFY_FORMAT_STRING const char* fmt, ... );
	
	static const int		MAX_LOG_LEVELS = 20;
	
	int						activeLevel;
	renderLogIndentLabel_t	indentLabel[MAX_LOG_LEVELS];
	char					indentString[MAX_LOG_LEVELS * 4];
	int						indentLevel;
	const char* 			lastLabel;
	renderLogMainBlock_t	lastMainBlock;
//	idFile*					logFile;

	struct logStats_t
	{
		uint64	startTiming;
		int		startDraws;
		int		startIndexes;
	};
	
	uint64					frameStartTime;
	uint64					closeBlockTime;
	logStats_t				logStats[MAX_LOG_LEVELS];
	int						logLevel;
	
	void					LogOpenBlock( renderLogIndentLabel_t label, const char* fmt, ... );
	void					LogCloseBlock( renderLogIndentLabel_t label );
};

/*
========================
idRenderLog::Indent
========================
*/
ID_INLINE void idRenderLog::Indent( renderLogIndentLabel_t label )
{
	//if( logFile != NULL )
	if( r_logFile.GetInteger() != 0 )
	{
		indentLabel[indentLevel] = label;
		indentLevel++;
		for( int i = 4; i > 0; i-- )
		{
			indentString[indentLevel * 4 - i] = ' ';
		}
		indentString[indentLevel * 4] = '\0';
	}
}

/*
========================
idRenderLog::Outdent
========================
*/
ID_INLINE void idRenderLog::Outdent( renderLogIndentLabel_t label )
{
	//if( logFile != NULL && indentLevel > 0 )
	if( r_logFile.GetInteger() != 0 && indentLevel > 0 )
	{
		indentLevel--;
		assert( indentLabel[indentLevel] == label );	// indent and outdent out of sync ?
		indentString[indentLevel * 4] = '\0';
	}
}

#else	// !STUB_RENDER_LOG

/*
================================================
idRenderLog stubbed version for the SPUs and high
performance rendering in retail builds.
================================================
*/
class idRenderLog
{
public:
	idRenderLog() {}
	
	void		StartFrame() {}
	void		EndFrame() {}
	void		Close() {}
	int			Active()
	{
		return 0;
	}
	
	void		OpenBlock( const char* label );
	void		CloseBlock();
	void		OpenMainBlock( renderLogMainBlock_t block ) {}
	void		CloseMainBlock() {}
	void		Indent( renderLogIndentLabel_t label = RENDER_LOG_INDENT_DEFAULT ) {}
	void		Outdent( renderLogIndentLabel_t label = RENDER_LOG_INDENT_DEFAULT ) {}
	
	void		Printf( VERIFY_FORMAT_STRING const char* fmt, ... ) {}
	
	int			activeLevel;
};

#endif	// !STUB_RENDER_LOG

extern idRenderLog renderLog;

#endif // !__RENDERLOG_H__
