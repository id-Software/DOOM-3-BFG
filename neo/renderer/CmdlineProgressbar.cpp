/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 2018-2022 Robert Beckebans

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

#include "RenderCommon.h"
#include "CmdlineProgressbar.h"


void CommandlineProgressBar::Start()
{
	// restore the original resolution, same as "vid_restart"
	glConfig.nativeScreenWidth = sysWidth;
	glConfig.nativeScreenHeight = sysHeight;
	R_SetNewMode( false );

	common->Printf( "0%%  10   20   30   40   50   60   70   80   90   100%%\n" );
	common->Printf( "|----|----|----|----|----|----|----|----|----|----|\n" );

	common->UpdateScreen( false );
}

void CommandlineProgressBar::Increment( bool updateScreen )
{
	if( ( count + 1 ) >= nextTicCount )
	{
		if( updateScreen )
		{
			// restore the original resolution, same as "vid_restart"
			glConfig.nativeScreenWidth = sysWidth;
			glConfig.nativeScreenHeight = sysHeight;
			R_SetNewMode( false );

			// resize frame buffers (this triggers SwapBuffers)
			tr.SwapCommandBuffers( NULL, NULL, NULL, NULL, NULL, NULL );
		}

		size_t ticsNeeded = ( size_t )( ( ( double )( count + 1 ) / expectedCount ) * 50.0 );

		do
		{
			common->Printf( "*" );
		}
		while( ++tics < ticsNeeded );

		nextTicCount = ( size_t )( ( tics / 50.0 ) * expectedCount );
		if( count == ( expectedCount - 1 ) )
		{
			if( tics < 51 )
			{
				common->Printf( "*" );
			}
			common->Printf( "\n" );
		}

		if( updateScreen )
		{
			common->UpdateScreen( false );

			// swap front / back buffers
			tr.SwapCommandBuffers( NULL, NULL, NULL, NULL, NULL, NULL );
		}
	}

	count++;
}

void CommandlineProgressBar::Reset()
{
	count = 0;
	tics = 0;
	nextTicCount = 0;
}

void CommandlineProgressBar::Reset( int expected )
{
	expectedCount = expected;
	count = 0;
	tics = 0;
	nextTicCount = 0;
}
