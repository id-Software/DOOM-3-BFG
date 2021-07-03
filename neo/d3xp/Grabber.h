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

/*
===============================================================================

	Grabber Object - Class to extend idWeapon to include functionality for
						manipulating physics objects.

===============================================================================
*/

class idBeam;

class idGrabber : public idEntity
{
public:
	CLASS_PROTOTYPE( idGrabber );

	idGrabber();
	~idGrabber();

	void					Save( idSaveGame* savefile ) const;
	void					Restore( idRestoreGame* savefile );

	void					Initialize();
	void					SetDragDistance( float dist );
	int						Update( idPlayer* player, bool hide );

private:
	idEntityPtr<idEntity>	dragEnt;			// entity being dragged
	idForce_Grab			drag;
	idVec3					saveGravity;

	int						id;					// id of body being dragged
	idVec3					localPlayerPoint;	// dragged point in player space
	idEntityPtr<idPlayer>	owner;
	int						oldImpulseSequence;
	bool					holdingAF;
	bool					shakeForceFlip;
	int						endTime;
	int						lastFiredTime;
	int						dragFailTime;
	int						startDragTime;
	float					dragTraceDist;
	int						savedContents;
	int						savedClipmask;

	idBeam*					beam;
	idBeam*					beamTarget;

	int						warpId;

	bool					grabbableAI( const char* aiName );
	void					StartDrag( idEntity* grabEnt, int id );
	void					StopDrag( bool dropOnly );
	void					UpdateBeams();
	void					ApplyShake();
};

