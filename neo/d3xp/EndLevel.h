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

class idTarget_EndLevel : public idEntity
{
public:
	CLASS_PROTOTYPE( idTarget_EndLevel );

	void	Spawn( void );
	~idTarget_EndLevel();

	void	Draw();
	// the endLevel will be responsible for drawing the entire screen
	// when it is active

	void	PlayerCommand( int buttons );
	// when an endlevel is active, plauer buttons get sent here instead
	// of doing anything to the player, which will allow moving to
	// the next level

	const char* ExitCommand();
	// the game will check this each frame, and return it to the
	// session when there is something to give

private:
	idStr	exitCommand;

	idVec3	initialViewOrg;
	idVec3	initialViewAngles;
	// set when the player triggers the exit

	idUserInterface*	gui;

	bool	buttonsReleased;
	// don't skip out until buttons are released, then pressed

	bool	readyToExit;
	bool	noGui;

	void	Event_Trigger( idEntity* activator );
};

