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

#ifndef __CMDLINE_PROGRESSBAR_H__
#define __CMDLINE_PROGRESSBAR_H__

// CommandlineProgressBar draws a nice progressbar in the console like you would get with boost
class CommandlineProgressBar
{
private:
	size_t tics = 0;
	size_t nextTicCount = 0;
	int	count = 0;
	int expectedCount = 0;

	int sysWidth = 1280;
	int sysHeight = 720;

public:
	CommandlineProgressBar( int _expectedCount, int width, int height )
	{
		expectedCount = _expectedCount;
		sysWidth = width;
		sysHeight = height;
	}

	void Start();
	void Increment( bool updateScreen );

	void Reset();
	void Reset( int expected );
};

#endif /* !__CMDLINE_PROGRESSBAR_H__ */
