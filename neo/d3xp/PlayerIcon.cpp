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

#include "Game_local.h"
#include "PlayerIcon.h"

static const char* iconKeys[ ICON_NONE ] =
{
	"mtr_icon_lag",
	"mtr_icon_chat"
	, "mtr_icon_redteam",
	"mtr_icon_blueteam"
};

/*
===============
idPlayerIcon::idPlayerIcon
===============
*/
idPlayerIcon::idPlayerIcon()
{
	iconHandle	= -1;
	iconType	= ICON_NONE;
}

/*
===============
idPlayerIcon::~idPlayerIcon
===============
*/
idPlayerIcon::~idPlayerIcon()
{
	FreeIcon();
}

/*
===============
idPlayerIcon::Draw
===============
*/
void idPlayerIcon::Draw( idPlayer* player, jointHandle_t joint )
{
	idVec3 origin;
	idMat3 axis;

	if( joint == INVALID_JOINT )
	{
		FreeIcon();
		return;
	}

	player->GetJointWorldTransform( joint, gameLocal.time, origin, axis );
	origin.z += 16.0f;

	Draw( player, origin );
}

/*
===============
idPlayerIcon::Draw
===============
*/
void idPlayerIcon::Draw( idPlayer* player, const idVec3& origin )
{
	idPlayer* localPlayer = gameLocal.GetLocalPlayer();
	if( !localPlayer || !localPlayer->GetRenderView() )
	{
		FreeIcon();
		return;
	}

	idMat3 axis = localPlayer->GetRenderView()->viewaxis;

	if( player->isLagged && !player->spectating )
	{
		// create the icon if necessary, or update if already created
		if( !CreateIcon( player, ICON_LAG, origin, axis ) )
		{
			UpdateIcon( player, origin, axis );
		}
	}
	else if( g_CTFArrows.GetBool() && gameLocal.mpGame.IsGametypeFlagBased() && gameLocal.GetLocalPlayer() && player->team == gameLocal.GetLocalPlayer()->team && !player->IsHidden() && !player->AI_DEAD )
	{
		int icon = ICON_TEAM_RED + player->team;

		if( icon != ICON_TEAM_RED && icon != ICON_TEAM_BLUE )
		{
			return;
		}

		if( !CreateIcon( player, ( playerIconType_t )icon, origin, axis ) )
		{
			UpdateIcon( player, origin, axis );
		}
	}
	else
	{
		FreeIcon();
	}
}

/*
===============
idPlayerIcon::FreeIcon
===============
*/
void idPlayerIcon::FreeIcon()
{
	if( iconHandle != - 1 )
	{
		gameRenderWorld->FreeEntityDef( iconHandle );
		iconHandle = -1;
	}
	iconType = ICON_NONE;
}

/*
===============
idPlayerIcon::CreateIcon
===============
*/
bool idPlayerIcon::CreateIcon( idPlayer* player, playerIconType_t type, const idVec3& origin, const idMat3& axis )
{
	assert( type < ICON_NONE );
	const char* mtr = player->spawnArgs.GetString( iconKeys[ type ], "_default" );
	return CreateIcon( player, type, mtr, origin, axis );
}

/*
===============
idPlayerIcon::CreateIcon
===============
*/
bool idPlayerIcon::CreateIcon( idPlayer* player, playerIconType_t type, const char* mtr, const idVec3& origin, const idMat3& axis )
{
	assert( type != ICON_NONE );

	if( type == iconType )
	{
		return false;
	}

	FreeIcon();

	memset( &renderEnt, 0, sizeof( renderEnt ) );
	renderEnt.origin	= origin;
	renderEnt.axis		= axis;
	renderEnt.shaderParms[ SHADERPARM_RED ]				= 1.0f;
	renderEnt.shaderParms[ SHADERPARM_GREEN ]			= 1.0f;
	renderEnt.shaderParms[ SHADERPARM_BLUE ]			= 1.0f;
	renderEnt.shaderParms[ SHADERPARM_ALPHA ]			= 1.0f;
	renderEnt.shaderParms[ SHADERPARM_SPRITE_WIDTH ]	= 16.0f;
	renderEnt.shaderParms[ SHADERPARM_SPRITE_HEIGHT ]	= 16.0f;
	renderEnt.hModel = renderModelManager->FindModel( "_sprite" );
	renderEnt.callback = NULL;
	renderEnt.numJoints = 0;
	renderEnt.joints = NULL;
	renderEnt.customSkin = 0;
	renderEnt.noShadow = true;
	renderEnt.noSelfShadow = true;
	renderEnt.customShader = declManager->FindMaterial( mtr );
	renderEnt.referenceShader = 0;
	renderEnt.bounds = renderEnt.hModel->Bounds( &renderEnt );

	iconHandle = gameRenderWorld->AddEntityDef( &renderEnt );
	iconType = type;

	return true;
}

/*
===============
idPlayerIcon::UpdateIcon
===============
*/
void idPlayerIcon::UpdateIcon( idPlayer* player, const idVec3& origin, const idMat3& axis )
{
	assert( iconHandle >= 0 );

	renderEnt.origin = origin;
	renderEnt.axis	= axis;
	gameRenderWorld->UpdateEntityDef( iconHandle, &renderEnt );
}

