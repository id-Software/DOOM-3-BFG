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

#ifndef _WIN32
	#include <signal.h> // for raise()
#endif

/*
================================================================================================
Contains the AssertMacro implementation.
================================================================================================
*/

idCVar com_assertOutOfDebugger( "com_assertOutOfDebugger", "0", CVAR_BOOL, "by default, do not assert while not running under the debugger" );

struct skippedAssertion_t
{
	skippedAssertion_t() :
		file( NULL ),
		line( -1 )
	{
	}
	const char* 	file;
	int				line;
};
static idStaticList< skippedAssertion_t, 20 > skippedAssertions;

/*
========================
AssertFailed
========================
*/
bool AssertFailed( const char* file, int line, const char* expression )
{
	// Set this to true to skip ALL assertions, including ones YOU CAUSE!
	static volatile bool skipAllAssertions = false;
	if( skipAllAssertions )
	{
		return false;
	}

	// Set this to true to skip ONLY this assertion
	static volatile bool skipThisAssertion = false;
	skipThisAssertion = false;

	for( int i = 0; i < skippedAssertions.Num(); i++ )
	{
		if( skippedAssertions[i].file == file && skippedAssertions[i].line == line )
		{
			skipThisAssertion = true;
			// Set breakpoint here to re-enable
			if( !skipThisAssertion )
			{
				skippedAssertions.RemoveIndexFast( i );
			}
			return false;
		}
	}

	idLib::Warning( "ASSERTION FAILED! %s(%d): '%s'", file, line, expression );

// RB begin
#ifdef _WIN32
	if( IsDebuggerPresent() || com_assertOutOfDebugger.GetBool() )
#else
	//if( com_assertOutOfDebugger.GetBool() )
#endif
// RB end
	{
#ifdef _WIN32
#ifdef _MSC_VER
		__debugbreak();
#else
		// DG: mingw support
		DebugBreak();
#endif
#else // not _WIN32
		// DG: POSIX support
		raise( SIGTRAP );
		// DG: end
#endif // _WIN32
	}

	if( skipThisAssertion )
	{
		skippedAssertion_t* skipped = skippedAssertions.Alloc();
		skipped->file = file;
		skipped->line = line;
	}

	return true;
}

