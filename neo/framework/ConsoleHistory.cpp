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
#include "precompiled.h"
#include "ConsoleHistory.h"

idConsoleHistory consoleHistory;

const char* HISTORY_FILE_NAME = "consoleHistory.txt";

/*
========================
idConsoleHistory::AddToHistory
========================
*/
void idConsoleHistory::AddToHistory( const char* line, bool writeHistoryFile )
{
	// empty lines never modify history
	if( line == NULL )
	{
		return;
	}
	const char* s;
	for( s = line; *s != '\0'; s++ )
	{
		if( *s > ' ' )
		{
			break;
		}
	}
	if( *s == '\0' )
	{
		return;
	}

	// repeating the last command doesn't add to the list
	if( historyLines[( numHistory - 1 ) & ( COMMAND_HISTORY - 1 )].Icmp( line ) == 0 )
	{
		if( historyLines[returnLine & ( COMMAND_HISTORY - 1 )].Icmp( line ) == 0 )
		{
			// the command was retrieved from the history, so
			// move the up point down so that up arrow will retrieve the same
			// command again.
			upPoint = returnLine;
		}
		else
		{
			// the command was typed again, so leave the up/down points alone
		}
		return;
	}
	// If we had previously retrieved older history commands with
	// up arrow, the up/down point will be reset to the end where
	// this command is added.
	upPoint = numHistory;
	returnLine = numHistory;
	downPoint = numHistory + 1;
//	//mem.PushHeap();	// go to the system heap, not the current map heap
	historyLines[numHistory & ( COMMAND_HISTORY - 1 )] = line;
//	//mem.PopHeap();
	numHistory++;

	// write the history file to disk
	if( writeHistoryFile )
	{
		idFile* f = fileSystem->OpenFileWrite( HISTORY_FILE_NAME );
		if( f != NULL )
		{
			for( int i = numHistory - COMMAND_HISTORY; i < numHistory; i++ )
			{
				if( i < 0 )
				{
					continue;
				}
				f->Printf( "%s\n", historyLines[i & ( COMMAND_HISTORY - 1 )].c_str() );
			}
			delete f;
		}
	}
}

/*
========================
idConsoleHistory::RetrieveFromHistory
========================
*/
idStr idConsoleHistory::RetrieveFromHistory( bool backward )
{
	// if there are no commands in the history
	if( numHistory == 0 )
	{
		return idStr( "" );
	}
	// move the history point
	if( backward )
	{
		if( upPoint < numHistory - COMMAND_HISTORY || upPoint < 0 )
		{
			return idStr( "" );
		}
		returnLine = upPoint;
		downPoint = upPoint + 1;
		upPoint--;
	}
	else
	{
		if( downPoint >= numHistory )
		{
			// DG: without this you'll get the last-but-one command when pressing UP
			upPoint = downPoint - 1;
			// DG end
			return idStr( "" );
		}
		returnLine = downPoint;
		upPoint = downPoint - 1;
		downPoint++;
	}
	return historyLines[returnLine & ( COMMAND_HISTORY - 1 )];
}

/*
========================
idConsoleHistory::LoadHistoryFile
========================
*/
void idConsoleHistory::LoadHistoryFile()
{
	idLexer lex;
	if( lex.LoadFile( HISTORY_FILE_NAME, false ) )
	{
		while( 1 )
		{
			idStr	line;
			lex.ParseCompleteLine( line );
			if( line.IsEmpty() )
			{
				break;
			}
			line.StripTrailingWhitespace();	// remove the \n
			AddToHistory( line, false );
		}
	}
}

/*
========================
idConsoleHistory::PrintHistory
========================
*/
void idConsoleHistory::PrintHistory()
{
	for( int i = numHistory - COMMAND_HISTORY; i < numHistory; i++ )
	{
		if( i < 0 )
		{
			continue;
		}
		idLib::Printf( "%c%c%c%4i: %s\n",
					   i == upPoint ? 'U' : ' ',
					   i == downPoint ? 'D' : ' ',
					   i == returnLine ? 'R' : ' ',
					   i, historyLines[i & ( COMMAND_HISTORY - 1 )].c_str() );
	}
}

/*
========================
idConsoleHistory::ClearHistory
========================
*/
void idConsoleHistory::ClearHistory()
{
	upPoint = 0;
	downPoint = 0;
	returnLine = 0;
	numHistory = 0;
}

/*
========================
history
========================
*/
CONSOLE_COMMAND_SHIP( history, "Displays the console command history", 0 )
{
	consoleHistory.PrintHistory();
}

/*
========================
clearHistory
========================
*/
CONSOLE_COMMAND_SHIP( clearHistory, "Clears the console history", 0 )
{
	consoleHistory.ClearHistory();
}
