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
#ifndef __CONSOLEHISTORY_H__
#define __CONSOLEHISTORY_H__

/*

This should behave like the windows command prompt, with the addition
of a persistant history file.

Note that commands bound to keys do not go through the console history.

*/

class idConsoleHistory
{
public:
	idConsoleHistory() :
		upPoint( 0 ),
		downPoint( 0 ),
		returnLine( 0 ),
		numHistory( 0 )
	{
		ClearHistory();
	}

	void	LoadHistoryFile();

	// the line should not have a \n
	// Empty lines are never added to the history.
	// If the command is the same as the last returned history line, nothing is changed.
	void	AddToHistory( const char* line, bool writeHistoryFile = true );

	// the string will not have a \n
	// Returns an empty string if there is nothing to retrieve in that
	// direction.
	idStr	RetrieveFromHistory( bool backward );

	// console commands
	void	PrintHistory();
	void	ClearHistory();

private:
	int		upPoint;	// command to be retrieved with next up-arrow
	int		downPoint;	// command to be retrieved with next down-arrow
	int		returnLine;	// last returned by RetrieveFromHistory()
	int		numHistory;

	static const int COMMAND_HISTORY = 64;
	idArray<idStr, COMMAND_HISTORY>	historyLines;

	compile_time_assert( CONST_ISPOWEROFTWO( COMMAND_HISTORY ) );	// we use the binary 'and' operator for wrapping
};

extern idConsoleHistory consoleHistory;

#endif // !__CONSOLEHISTORY_H__
