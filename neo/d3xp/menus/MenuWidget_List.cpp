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

/*
================================================================================================
idMenuWidget_List

Provides a paged view of this widgets children.  Each child is expected to take on the following
naming scheme.  Children outside of the given window size (NumVisibleOptions) are not rendered,
and will affect which type of arrow indicators are shown.

Future work:
- Make upIndicator another kind of widget (Image widget?)
================================================================================================
*/

/*
========================
idMenuWidget_List::Update
========================
*/
void idMenuWidget_List::Update()
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

	for( int optionIndex = 0; optionIndex < GetNumVisibleOptions(); ++optionIndex )
	{
		const int childIndex = GetViewOffset() + optionIndex;
		bool shown = false;

		if( optionIndex < GetChildren().Num() )
		{
			idMenuWidget& child = GetChildByIndex( optionIndex );
			const int controlIndex = GetNumVisibleOptions() - Min( GetNumVisibleOptions(), GetTotalNumberOfOptions() ) + optionIndex;
			child.SetSpritePath( GetSpritePath(), va( "item%d", controlIndex ) );
			if( child.BindSprite( root ) )
			{
				PrepareListElement( child, childIndex );
				child.Update();
				shown = true;
			}
		}

		if( !shown )
		{
			// hide the item
			idSWFSpriteInstance* const sprite = GetSprite()->GetScriptObject()->GetSprite( va( "item%d", optionIndex - GetTotalNumberOfOptions() ) );
			if( sprite != NULL )
			{
				sprite->SetVisible( false );
			}
		}
	}

	idSWFSpriteInstance* const upSprite = GetSprite()->GetScriptObject()->GetSprite( "upIndicator" );
	if( upSprite != NULL )
	{
		upSprite->SetVisible( GetViewOffset() > 0 );
	}

	idSWFSpriteInstance* const downSprite = GetSprite()->GetScriptObject()->GetSprite( "downIndicator" );
	if( downSprite != NULL )
	{
		downSprite->SetVisible( GetViewOffset() + GetNumVisibleOptions() < GetTotalNumberOfOptions() );
	}
}

/*
========================
idMenuWidget_List::HandleAction
========================
*/
bool idMenuWidget_List::HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled )
{

	const idSWFParmList& parms = action.GetParms();

	if( action.GetType() == WIDGET_ACTION_SCROLL_VERTICAL )
	{
		const scrollType_t scrollType = static_cast< scrollType_t >( event.arg );
		if( scrollType == SCROLL_SINGLE )
		{
			Scroll( parms[ 0 ].ToInteger() );
		}
		else if( scrollType == SCROLL_PAGE )
		{
			ScrollOffset( parms[ 0 ].ToInteger() * ( GetNumVisibleOptions() - 1 ) );
		}
		else if( scrollType == SCROLL_FULL )
		{
			ScrollOffset( parms[ 0 ].ToInteger() * 999 );
		}
		return true;
	}

	return idMenuWidget::HandleAction( action, event, widget, forceHandled );
}

/*
========================
idMenuWidget_List::ObserveEvent
========================
*/
void idMenuWidget_List::ObserveEvent( const idMenuWidget& widget, const idWidgetEvent& event )
{
	ExecuteEvent( event );
}

/*
========================
idMenuWidget_List::CalculatePositionFromIndexDelta

Pure functional encapsulation of how to calculate a new index and offset based on how the user
chose to move through the list.
========================
*/
void idMenuWidget_List::CalculatePositionFromIndexDelta( int& outIndex, int& outOffset, const int currentIndex, const int currentOffset, const int windowSize, const int maxSize, const int indexDelta, const bool allowWrapping, const bool wrapAround ) const
{
	assert( indexDelta != 0 );

	int newIndex = currentIndex + indexDelta;
	bool wrapped = false;

	if( indexDelta > 0 )
	{
		// moving down the list
		if( newIndex > maxSize - 1 )
		{
			if( allowWrapping )
			{
				if( wrapAround )
				{
					wrapped = true;
					newIndex = 0 + ( newIndex - maxSize );
				}
				else
				{
					newIndex = 0;
				}
			}
			else
			{
				newIndex = maxSize - 1;
			}
		}
	}
	else
	{
		// moving up the list
		if( newIndex < 0 )
		{
			if( allowWrapping )
			{
				if( wrapAround )
				{
					newIndex = maxSize + newIndex;
				}
				else
				{
					newIndex = maxSize - 1;
				}
			}
			else
			{
				newIndex = 0;
			}
		}
	}

	// calculate the offset
	if( newIndex - currentOffset >= windowSize )
	{
		outOffset = newIndex - windowSize + 1;
	}
	else if( currentOffset > newIndex )
	{
		if( wrapped )
		{
			outOffset = 0;
		}
		else
		{
			outOffset = newIndex;
		}
	}
	else
	{
		outOffset = currentOffset;
	}

	outIndex = newIndex;

	// the intended behavior is that outOffset and outIndex are always within maxSize of each
	// other, as they are meant to model a window of items that should be visible in the list.
	assert( outIndex - outOffset < windowSize );
	assert( outIndex >= outOffset && outIndex >= 0 && outOffset >= 0 );
}

/*
========================
idMenuWidget_List::CalculatePositionFromOffsetDelta
========================
*/
void idMenuWidget_List::CalculatePositionFromOffsetDelta( int& outIndex, int& outOffset, const int currentIndex, const int currentOffset, const int windowSize, const int maxSize, const int offsetDelta ) const
{
	// shouldn't be setting both indexDelta AND offsetDelta
	// FIXME: make this simpler code - just pass a boolean to control it?
	assert( offsetDelta != 0 );

	const int newOffset = Max( currentIndex + offsetDelta, 0 );

	if( newOffset >= maxSize )
	{
		// scrolling past the end - just scroll all the way to the end
		outIndex = maxSize - 1;
		outOffset = Max( maxSize - windowSize, 0 );
	}
	else if( newOffset >= maxSize - windowSize )
	{
		// scrolled to the last window
		outIndex = newOffset;
		outOffset = Max( maxSize - windowSize, 0 );
	}
	else
	{
		outIndex = outOffset = newOffset;
	}

	// the intended behavior is that outOffset and outIndex are always within maxSize of each
	// other, as they are meant to model a window of items that should be visible in the list.
	assert( outIndex - outOffset < windowSize );
	assert( outIndex >= outOffset && outIndex >= 0 && outOffset >= 0 );
}

/*
========================
idMenuWidget_List::Scroll
========================
*/
void idMenuWidget_List::Scroll( const int scrollAmount, const bool wrapAround )
{

	if( GetTotalNumberOfOptions() == 0 )
	{
		return;
	}

	int newIndex, newOffset;

	CalculatePositionFromIndexDelta( newIndex, newOffset, GetViewIndex(), GetViewOffset(), GetNumVisibleOptions(), GetTotalNumberOfOptions(), scrollAmount, IsWrappingAllowed(), wrapAround );
	if( newOffset != GetViewOffset() )
	{
		SetViewOffset( newOffset );
		if( menuData != NULL )
		{
			menuData->PlaySound( GUI_SOUND_FOCUS );
		}
		Update();
	}

	if( newIndex != GetViewIndex() )
	{
		SetViewIndex( newIndex );
		SetFocusIndex( newIndex - newOffset );
	}
}

/*
========================
idMenuWidget_List::ScrollOffset
========================
*/
void idMenuWidget_List::ScrollOffset( const int scrollAmount )
{

	if( GetTotalNumberOfOptions() == 0 )
	{
		return;
	}

	int newIndex, newOffset;

	CalculatePositionFromOffsetDelta( newIndex, newOffset, GetViewIndex(), GetViewOffset(), GetNumVisibleOptions(), GetTotalNumberOfOptions(), scrollAmount );
	if( newOffset != GetViewOffset() )
	{
		SetViewOffset( newOffset );
		Update();
	}

	if( newIndex != GetViewIndex() )
	{
		SetViewIndex( newIndex );
		SetFocusIndex( newIndex - newOffset );
	}
}

//*********************************************************************************
// GAME BROWSER LIST
//********************************************************************************

/*
========================
idMenuWidget_GameBrowserList::Update
========================
*/
void idMenuWidget_GameBrowserList::Update()
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

	for( int optionIndex = 0; optionIndex < GetNumVisibleOptions(); ++optionIndex )
	{
		const int childIndex = GetViewOffset() + optionIndex;
		bool shown = false;
		if( optionIndex < GetChildren().Num() )
		{
			idMenuWidget& child = GetChildByIndex( optionIndex );
			child.SetSpritePath( GetSpritePath(), va( "item%d", optionIndex ) );
			if( child.BindSprite( root ) )
			{
				shown = PrepareListElement( child, childIndex );
				if( shown )
				{
					child.SetState( WIDGET_STATE_NORMAL );
					child.GetSprite()->SetVisible( true );
					child.Update();
				}
				else
				{
					child.GetSprite()->SetVisible( false );
				}
			}
		}
	}

	idSWFSpriteInstance* const upSprite = GetSprite()->GetScriptObject()->GetSprite( "upIndicator" );
	if( upSprite != NULL )
	{
		upSprite->SetVisible( GetViewOffset() > 0 );
	}

	idSWFSpriteInstance* const downSprite = GetSprite()->GetScriptObject()->GetSprite( "downIndicator" );
	if( downSprite != NULL )
	{
		downSprite->SetVisible( GetViewOffset() + GetNumVisibleOptions() < GetTotalNumberOfOptions() );
	}
}

/*
========================
idMenuWidget_GameBrowserList::PrepareListElement
========================
*/
bool idMenuWidget_GameBrowserList::PrepareListElement( idMenuWidget& widget, const int childIndex )
{

	if( childIndex >= games.Num() )
	{
		return false;
	}

	idMenuWidget_ServerButton* const button = dynamic_cast< idMenuWidget_ServerButton* >( &widget );
	if( button == NULL )
	{
		return false;
	}

	if( games[childIndex].serverName.IsEmpty() )
	{
		return false;
	}

	const idBrowserEntry_t entry = games[childIndex];

	button->SetButtonInfo( entry.serverName, entry.mapName, entry.modeName, entry.index, entry.players, entry.maxPlayers, entry.joinable, entry.validMap );

	return true;

}

/*
========================
idMenuWidget_GameBrowserList::PrepareListElement
========================
*/
void idMenuWidget_GameBrowserList::ClearGames()
{
	games.Clear();
}

/*
========================
idMenuWidget_GameBrowserList::PrepareListElement
========================
*/
void idMenuWidget_GameBrowserList::AddGame( idStr name_, idStrId mapName_, idStr modeName_, int index_, int players_, int maxPlayers_, bool joinable_, bool validMap_ )
{

	idBrowserEntry_t entry;

	entry.serverName = name_;
	entry.index = index_;
	entry.players = players_;
	entry.maxPlayers = maxPlayers_;
	entry.joinable = joinable_;
	entry.validMap = validMap_;
	entry.mapName = mapName_;
	entry.modeName = modeName_;

	games.Append( entry );
}

/*
========================
idMenuWidget_GameBrowserList::GetTotalNumberOfOptions
========================
*/
int idMenuWidget_GameBrowserList::GetTotalNumberOfOptions() const
{
	return games.Num();
}

/*
========================
idMenuWidget_GameBrowserList::PrepareListElement
========================
*/
int idMenuWidget_GameBrowserList::GetServerIndex()
{

	if( GetViewIndex() < games.Num() )
	{
		return games[ GetViewIndex() ].index;
	}

	return -1;

}