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

#ifndef __CMDARGS_H__
#define __CMDARGS_H__

/*
===============================================================================

	Command arguments.

===============================================================================
*/

class idCmdArgs
{
public:
	idCmdArgs()
	{
		argc = 0;
	}
	idCmdArgs( const char* text, bool keepAsStrings )
	{
		TokenizeString( text, keepAsStrings );
	}

	void					operator=( const idCmdArgs& args );

	// The functions that execute commands get their parameters with these functions.
	int						Argc() const
	{
		return argc;
	}
	// Argv() will return an empty string, not NULL if arg >= argc.
	const char* 			Argv( int arg ) const
	{
		return ( arg >= 0 && arg < argc ) ? argv[arg] : "";
	}
	// Returns a single string containing argv(start) to argv(end)
	// escapeArgs is a fugly way to put the string back into a state ready to tokenize again
	const char* 			Args( int start = 1, int end = -1, bool escapeArgs = false ) const;

	// Takes a null terminated string and breaks the string up into arg tokens.
	// Does not need to be /n terminated.
	// Set keepAsStrings to true to only separate tokens from whitespace and comments, ignoring punctuation
	void					TokenizeString( const char* text, bool keepAsStrings );

	void					AppendArg( const char* text );
	void					Clear()
	{
		argc = 0;
	}
	const char* const* 	GetArgs( int* argc );

private:
	static const int		MAX_COMMAND_ARGS = 64;
	static const int		MAX_COMMAND_STRING = 2 * MAX_STRING_CHARS;

	int						argc;								// number of arguments
	char* 					argv[MAX_COMMAND_ARGS];				// points into tokenized
	char					tokenized[MAX_COMMAND_STRING];		// will have 0 bytes inserted
};

#endif /* !__CMDARGS_H__ */
