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

#ifndef	__PLAYERICON_H__
#define	__PLAYERICON_H__

typedef enum
{
	ICON_LAG,
	ICON_CHAT,
	ICON_TEAM_RED,
	ICON_TEAM_BLUE,
	ICON_NONE
} playerIconType_t;

class idPlayerIcon
{
public:

public:
	idPlayerIcon();
	~idPlayerIcon();

	void	Draw( idPlayer* player, jointHandle_t joint );
	void	Draw( idPlayer* player, const idVec3& origin );

public:
	playerIconType_t	iconType;
	renderEntity_t		renderEnt;
	qhandle_t			iconHandle;

public:
	void	FreeIcon();
	bool	CreateIcon( idPlayer* player, playerIconType_t type, const char* mtr, const idVec3& origin, const idMat3& axis );
	bool	CreateIcon( idPlayer* player, playerIconType_t type, const idVec3& origin, const idMat3& axis );
	void	UpdateIcon( idPlayer* player, const idVec3& origin, const idMat3& axis );

};


#endif	/* !_PLAYERICON_H_ */

