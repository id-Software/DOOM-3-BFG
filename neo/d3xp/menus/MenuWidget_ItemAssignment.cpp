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
#include "../Game_local.h"

void idMenuWidget_ItemAssignment::SetIcon( int index, const idMaterial* icon )
{

	if( index < 0 || index >= NUM_QUICK_SLOTS )
	{
		return;
	}

	images[ index ] = icon;
}

void idMenuWidget_ItemAssignment::FindFreeSpot()
{
	slotIndex = 0;
	for( int i = 0; i < NUM_QUICK_SLOTS; ++i )
	{
		if( images[ i ] == NULL )
		{
			slotIndex = i;
			break;
		}
	}
}

/*
========================
idMenuWidget_ItemAssignment::Update
========================
*/
void idMenuWidget_ItemAssignment::Update()
{

	if( GetSWFObject() == NULL )
	{
		return;
	}

	idSWFScriptObject& root = GetSWFObject()->GetRootObject();

	if( !BindSprite( root ) )
	{
		return;
	}

	idSWFSpriteInstance* dpad = GetSprite()->GetScriptObject()->GetNestedSprite( "dpad" );

	if( dpad != NULL )
	{
		dpad->StopFrame( slotIndex + 2 );
	}

	for( int i = 0; i < NUM_QUICK_SLOTS; ++i )
	{
		idSWFSpriteInstance* item = GetSprite()->GetScriptObject()->GetNestedSprite( va( "item%d", i ) );
		if( item != NULL )
		{
			if( i == slotIndex )
			{
				item->StopFrame( 2 );
			}
			else
			{
				item->StopFrame( 1 );
			}
		}

		idSWFSpriteInstance* itemIcon = GetSprite()->GetScriptObject()->GetNestedSprite( va( "item%d", i ), "img" );
		if( itemIcon != NULL )
		{
			if( images[ i ] != NULL )
			{
				itemIcon->SetVisible( true );
				itemIcon->SetMaterial( images[ i ] );
			}
			else
			{
				itemIcon->SetVisible( false );
			}
		}

		itemIcon = GetSprite()->GetScriptObject()->GetNestedSprite( va( "item%d", i ), "imgTop" );
		if( itemIcon != NULL )
		{
			if( images[ i ] != NULL )
			{
				itemIcon->SetVisible( true );
				itemIcon->SetMaterial( images[ i ] );
			}
			else
			{
				itemIcon->SetVisible( false );
			}
		}
	}
}
