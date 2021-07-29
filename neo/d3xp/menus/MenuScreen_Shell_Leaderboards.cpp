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

const static int NUM_LEADERBOARD_ITEMS = 16;
const int MAX_STAT_LISTINGS = 16;
static const int MAX_ROWS_PER_BLOCK = 50;

idMenuScreen_Shell_Leaderboards::~idMenuScreen_Shell_Leaderboards()
{
	if( lbCache != NULL )
	{
		delete lbCache;
		lbCache = NULL;
	}
}

// Helper functions for formatting leaderboard columns
static idStr FormatTime( int64 time )
{
	int minutes = time / ( 1000 * 60 );
	int seconds = ( time - ( minutes * 1000 * 60 ) ) / 1000;
	int mseconds = time - ( ( minutes * 1000 * 60 ) + ( seconds * 1000 ) );
	return idStr( va( "%02d:%02d.%03d", minutes, seconds, mseconds ) );
}

static idStr FormatCash( int64 cash )
{
	return idStr::FormatCash( static_cast<int32>( cash ) );
}
static int32 FormatNumber( int64 number )
{
	return static_cast<int32>( number );
}

static idSWFScriptVar FormatColumn( const columnDef_t* columnDef, int64 score )
{
	switch( columnDef->displayType )
	{
		case STATS_COLUMN_DISPLAY_TIME_MILLISECONDS:
			return idSWFScriptVar( FormatTime( score ) );
		case STATS_COLUMN_DISPLAY_CASH:
			return idSWFScriptVar( FormatCash( score ) );
		default:
			return idSWFScriptVar( FormatNumber( score ) );
	}
}

/*
========================
idMenuScreen_Shell_Leaderboards::Initialize
========================
*/
void idMenuScreen_Shell_Leaderboards::Initialize( idMenuHandler* data )
{
	idMenuScreen::Initialize( data );

	if( data != NULL )
	{
		menuGUI = data->GetGUI();
	}

	lbCache = new idLBCache();
	lbCache->Reset();

	SetSpritePath( "menuLeaderboards" );

	options = new( TAG_SWF ) idMenuWidget_DynamicList();
	options->SetNumVisibleOptions( NUM_LEADERBOARD_ITEMS );
	options->SetSpritePath( GetSpritePath(), "info", "options" );
	options->SetWrappingAllowed( true );

	while( options->GetChildren().Num() < NUM_LEADERBOARD_ITEMS )
	{
		idMenuWidget_Button* const buttonWidget = new( TAG_SWF ) idMenuWidget_Button();
		buttonWidget->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_PRESS_FOCUSED, options->GetChildren().Num() );
		buttonWidget->Initialize( data );
		options->AddChild( buttonWidget );
	}
	options->Initialize( data );

	AddChild( options );

	btnBack = new( TAG_SWF ) idMenuWidget_Button();
	btnBack->Initialize( data );
	btnBack->SetLabel( "#str_swf_party_lobby" );
	btnBack->SetSpritePath( GetSpritePath(), "info", "btnBack" );
	btnBack->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_GO_BACK );
	AddChild( btnBack );

	btnNext = new( TAG_SWF ) idMenuWidget_Button();
	btnNext->Initialize( data );
	btnNext->SetLabel( "#str_swf_next" );
	btnNext->SetSpritePath( GetSpritePath(), "info", "btnNext" );
	btnNext->AddEventAction( WIDGET_EVENT_PRESS ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_TAB_NEXT, WIDGET_EVENT_TAB_NEXT ) );
	AddChild( btnNext );

	btnPrev = new( TAG_SWF ) idMenuWidget_Button();
	btnPrev->Initialize( data );
	btnPrev->SetLabel( "#str_swf_prev" );
	btnPrev->SetSpritePath( GetSpritePath(), "info", "btnPrevious" );
	btnPrev->AddEventAction( WIDGET_EVENT_PRESS ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_TAB_PREV, WIDGET_EVENT_TAB_PREV ) );
	AddChild( btnPrev );

	btnPageDwn = new( TAG_SWF ) idMenuWidget_Button();
	btnPageDwn->Initialize( data );
	btnPageDwn->SetLabel( "#str_swf_next_page" );
	btnPageDwn->SetSpritePath( GetSpritePath(), "info", "btnPageDwn" );
	idSWFParmList parms;
	parms.Append( MAX_STAT_LISTINGS - 1 );
	btnPageDwn->AddEventAction( WIDGET_EVENT_PRESS ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_SCROLL_PAGE_DOWN_START_REPEATER, WIDGET_EVENT_SCROLL_PAGEDWN ) );
	btnPageDwn->AddEventAction( WIDGET_EVENT_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_PAGEDWN_RELEASE ) );
	AddChild( btnPageDwn );

	btnPageUp = new( TAG_SWF ) idMenuWidget_Button();
	btnPageUp->Initialize( data );
	btnPageUp->SetLabel( "#str_swf_prev_page" );
	btnPageUp->SetSpritePath( GetSpritePath(), "info", "btnPageUp" );
	parms.Clear();
	parms.Append( MAX_STAT_LISTINGS - 1 );
	btnPageUp->AddEventAction( WIDGET_EVENT_PRESS ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_SCROLL_PAGE_UP_START_REPEATER, WIDGET_EVENT_SCROLL_PAGEUP ) );
	btnPageUp->AddEventAction( WIDGET_EVENT_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_PAGEUP_RELEASE ) );
	AddChild( btnPageUp );

	AddEventAction( WIDGET_EVENT_SCROLL_DOWN_LSTICK ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_SCROLL_DOWN_START_REPEATER_VARIABLE, WIDGET_EVENT_SCROLL_DOWN_LSTICK ) );
	AddEventAction( WIDGET_EVENT_SCROLL_UP_LSTICK ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_SCROLL_UP_START_REPEATER_VARIABLE, WIDGET_EVENT_SCROLL_UP_LSTICK ) );
	AddEventAction( WIDGET_EVENT_SCROLL_DOWN_LSTICK_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_DOWN_LSTICK_RELEASE ) );
	AddEventAction( WIDGET_EVENT_SCROLL_UP_LSTICK_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_UP_LSTICK_RELEASE ) );
	AddEventAction( WIDGET_EVENT_SCROLL_DOWN ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_SCROLL_DOWN_START_REPEATER_VARIABLE, WIDGET_EVENT_SCROLL_DOWN ) );
	AddEventAction( WIDGET_EVENT_SCROLL_UP ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_SCROLL_UP_START_REPEATER_VARIABLE, WIDGET_EVENT_SCROLL_UP ) );
	AddEventAction( WIDGET_EVENT_SCROLL_DOWN_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_DOWN_RELEASE ) );
	AddEventAction( WIDGET_EVENT_SCROLL_UP_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_UP_RELEASE ) );
	AddEventAction( WIDGET_EVENT_SCROLL_PAGEDWN ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_SCROLL_PAGE_DOWN_START_REPEATER, WIDGET_EVENT_SCROLL_PAGEDWN ) );
	AddEventAction( WIDGET_EVENT_SCROLL_PAGEUP ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_SCROLL_PAGE_UP_START_REPEATER, WIDGET_EVENT_SCROLL_PAGEUP ) );
	AddEventAction( WIDGET_EVENT_SCROLL_PAGEDWN_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_PAGEDWN_RELEASE ) );
	AddEventAction( WIDGET_EVENT_SCROLL_PAGEUP_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_PAGEUP_RELEASE ) );
	AddEventAction( WIDGET_EVENT_TAB_NEXT ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_TAB_NEXT, WIDGET_EVENT_TAB_NEXT ) );
	AddEventAction( WIDGET_EVENT_TAB_PREV ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_TAB_PREV, WIDGET_EVENT_TAB_PREV ) );

	leaderboards.Clear();

	const idList< mpMap_t > maps = common->GetMapList();
	const char** gameModes = NULL;
	const char** gameModesDisplay = NULL;
	int numModes = game->GetMPGameModes( &gameModes, &gameModesDisplay );

	for( int mapIndex = 0; mapIndex < maps.Num(); ++mapIndex )
	{
		for( int modeIndex = 0; modeIndex < numModes; ++modeIndex )
		{
			// Check the supported modes on the map.
			if( maps[ mapIndex ].supportedModes & BIT( modeIndex ) )
			{
				int boardID = LeaderboardLocal_GetID( mapIndex, modeIndex );
				const leaderboardDefinition_t* lbDef = Sys_FindLeaderboardDef( boardID );
				if( lbDef != NULL )
				{
					doomLeaderboard_t lb = doomLeaderboard_t( lbDef, lbDef->boardName );
					leaderboards.Append( lb );
				}
			}
		}
	}

}

/*
========================
idMenuScreen_Shell_Leaderboards::PumpLBCache
========================
*/
void idMenuScreen_Shell_Leaderboards::PumpLBCache()
{

	if( lbCache == NULL )
	{
		return;
	}

	lbCache->Pump();

}

/*
========================
idMenuScreen_Shell_Leaderboards::ClearLeaderboard
========================
*/
void idMenuScreen_Shell_Leaderboards::ClearLeaderboard()
{

	if( lbCache == NULL )
	{
		return;
	}

	lbCache->Reset();

}

/*
========================
idMenuScreen_Shell_Leaderboards::Update
========================
*/
void idMenuScreen_Shell_Leaderboards::Update()
{

	if( menuData != NULL )
	{
		idMenuWidget_CommandBar* cmdBar = menuData->GetCmdBar();
		if( cmdBar != NULL )
		{
			cmdBar->ClearAllButtons();
			idMenuWidget_CommandBar::buttonInfo_t* buttonInfo;
			buttonInfo = cmdBar->GetButton( idMenuWidget_CommandBar::BUTTON_JOY2 );
			if( menuData->GetPlatform() != 2 )
			{
				buttonInfo->label = "#str_00395";
			}
			buttonInfo->action.Set( WIDGET_ACTION_GO_BACK );

			buttonInfo = cmdBar->GetButton( idMenuWidget_CommandBar::BUTTON_JOY3 );
			buttonInfo->label = "#str_online_leaderboards_toggle_filter";
			buttonInfo->action.Set( WIDGET_ACTION_JOY3_ON_PRESS );

			if( !lbCache->IsLoadingNewLeaderboard() && !lbCache->IsRequestingRows() && options != NULL && options->GetTotalNumberOfOptions() > 0 )
			{
				buttonInfo = cmdBar->GetButton( idMenuWidget_CommandBar::BUTTON_JOY1 );
				if( menuData->GetPlatform() != 2 )
				{
					buttonInfo->label = "#str_swf_view_profile";
				}
				buttonInfo->action.Set( WIDGET_ACTION_PRESS_FOCUSED );
			}
		}
	}

	idSWFScriptObject& root = GetSWFObject()->GetRootObject();
	if( BindSprite( root ) )
	{
		idSWFTextInstance* heading = GetSprite()->GetScriptObject()->GetNestedText( "info", "txtHeading" );
		if( heading != NULL )
		{
			heading->SetText( lbCache->GetFilterStrType() );
			heading->SetStrokeInfo( true, 0.75f, 1.75f );
		}

		idSWFSpriteInstance* gradient = GetSprite()->GetScriptObject()->GetNestedSprite( "info", "gradient" );
		if( gradient != NULL && heading != NULL )
		{
			gradient->SetXPos( heading->GetTextLength() );
		}
	}

	if( btnBack != NULL )
	{
		btnBack->BindSprite( root );
	}

	idMenuScreen::Update();
}

/*
========================
idMenuScreen_Shell_Leaderboards::ShowScreen
========================
*/
void idMenuScreen_Shell_Leaderboards::ShowScreen( const mainMenuTransition_t transitionType )
{
	idMenuScreen::ShowScreen( transitionType );

	if( GetSprite() != NULL )
	{
		lbHeading = GetSprite()->GetScriptObject()->GetNestedText( "info", "txtLbType" );
		if( menuData != NULL && menuData->GetGUI() != NULL )
		{
			idSWFScriptObject* const shortcutKeys = menuData->GetGUI()->GetGlobal( "shortcutKeys" ).GetObject();
			if( verify( shortcutKeys != NULL ) )
			{

				// TAB NEXT
				idSWFScriptObject* const btnTabNext = GetSprite()->GetScriptObject()->GetNestedObj( "info", "btnNext" );
				if( btnTabNext != NULL )
				{
					btnTabNext->Set( "onPress", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_TAB_NEXT, 0 ) );
					shortcutKeys->Set( "JOY6", btnTabNext );

					if( btnTabNext->GetSprite() != NULL && menuData != NULL )
					{
						btnTabNext->GetSprite()->StopFrame( menuData->GetPlatform() + 1 );
					}

				}

				// TAB PREV
				idSWFScriptObject* const btnTabPrev = GetSprite()->GetScriptObject()->GetNestedObj( "info", "btnPrevious" );
				if( btnTabPrev != NULL )
				{
					btnTabPrev->Set( "onPress", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_TAB_PREV, 0 ) );
					shortcutKeys->Set( "JOY5", btnTabPrev );

					if( btnTabPrev->GetSprite() != NULL && menuData != NULL )
					{
						btnTabPrev->GetSprite()->StopFrame( menuData->GetPlatform() + 1 );
					}
				}

				// TAB NEXT
				idSWFScriptObject* const btnDwn = GetSprite()->GetScriptObject()->GetNestedObj( "info", "btnPageDwn" );
				if( btnDwn != NULL )
				{
					btnDwn->Set( "onPress", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_SCROLL_PAGEDWN, 0 ) );
					shortcutKeys->Set( "JOY_TRIGGER2", btnDwn );

					if( btnDwn->GetSprite() != NULL && menuData != NULL )
					{
						btnDwn->GetSprite()->StopFrame( menuData->GetPlatform() + 1 );
					}

				}

				// TAB PREV
				idSWFScriptObject* const btnUp = GetSprite()->GetScriptObject()->GetNestedObj( "info", "btnPageUp" );
				if( btnUp != NULL )
				{
					btnUp->Set( "onPress", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_SCROLL_PAGEUP, 0 ) );
					shortcutKeys->Set( "JOY_TRIGGER1", btnUp );

					if( btnUp->GetSprite() != NULL && menuData != NULL )
					{
						btnUp->GetSprite()->StopFrame( menuData->GetPlatform() + 1 );
					}
				}

			}
		}
	}

	SetLeaderboardIndex();

	if( menuData == NULL )
	{
		return;
	}

	int platform = menuData->GetPlatform();
	if( btnNext != NULL && btnNext->GetSprite() != NULL )
	{
		idSWFSpriteInstance* btnImg = btnNext->GetSprite()->GetScriptObject()->GetNestedSprite( "btnImg" );

		if( btnImg != NULL )
		{
			if( platform == 2 )
			{
				btnImg->SetVisible( false );
			}
			else
			{
				btnImg->SetVisible( true );
				btnImg->StopFrame( platform + 1 );
			}
		}
	}

	if( btnPrev != NULL && btnPrev->GetSprite() != NULL )
	{
		idSWFSpriteInstance* btnImg = btnPrev->GetSprite()->GetScriptObject()->GetNestedSprite( "btnImg" );

		if( btnImg != NULL )
		{
			if( platform == 2 )
			{
				btnImg->SetVisible( false );
			}
			else
			{
				btnImg->SetVisible( true );
				btnImg->StopFrame( platform + 1 );
			}
		}
	}

	if( btnPageDwn != NULL && btnPageDwn->GetSprite() != NULL )
	{
		idSWFSpriteInstance* btnImg = btnPageDwn->GetSprite()->GetScriptObject()->GetNestedSprite( "btnImg" );

		if( btnImg != NULL )
		{
			if( platform == 2 )
			{
				btnImg->SetVisible( false );
			}
			else
			{
				btnImg->SetVisible( true );
				btnImg->StopFrame( platform + 1 );
			}
		}
	}

	if( btnPageUp != NULL && btnPageUp->GetSprite() != NULL )
	{
		idSWFSpriteInstance* btnImg = btnPageUp->GetSprite()->GetScriptObject()->GetNestedSprite( "btnImg" );

		if( btnImg != NULL )
		{
			if( platform == 2 )
			{
				btnImg->SetVisible( false );
			}
			else
			{
				btnImg->SetVisible( true );
				btnImg->StopFrame( platform + 1 );
			}
		}
	}


}

/*
========================
idMenuScreen_Shell_Leaderboards::HideScreen
========================
*/
void idMenuScreen_Shell_Leaderboards::HideScreen( const mainMenuTransition_t transitionType )
{
	idMenuScreen::HideScreen( transitionType );
}

/*
========================
idMenuScreen_Shell_Leaderboards::HandleAction
========================
*/
bool idMenuScreen_Shell_Leaderboards::HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled )
{

	if( menuData == NULL )
	{
		return true;
	}

	if( menuData->ActiveScreen() != SHELL_AREA_LEADERBOARDS )
	{
		return false;
	}

	widgetAction_t actionType = action.GetType();
	const idSWFParmList& parms = action.GetParms();

	switch( actionType )
	{
		case WIDGET_ACTION_GO_BACK:
		{
			menuData->SetNextScreen( SHELL_AREA_PARTY_LOBBY, MENU_TRANSITION_SIMPLE );
			return true;
		}
		case WIDGET_ACTION_JOY3_ON_PRESS:
		{
			lbCache->CycleFilter();
			refreshLeaderboard = true;
			return true;
		}
		case WIDGET_ACTION_PRESS_FOCUSED:
		{

			if( options == NULL )
			{
				return true;
			}

			int index = options->GetFocusIndex();
			if( parms.Num() != 0 )
			{
				index = parms[0].ToInteger();
			}

			if( lbCache->GetEntryIndex() != index )
			{
				lbCache->SetEntryIndex( index );
				refreshLeaderboard = true;
				return true;
			}

			const idLeaderboardCallback::row_t* row = lbCache->GetLeaderboardRow( lbCache->GetRowOffset() + lbCache->GetEntryIndex() );
			if( row != NULL )
			{
				lbCache->DisplayGamerCardUI( row );
			}

			return true;
		}
		case WIDGET_ACTION_SCROLL_TAB:
		{
			int delta = parms[0].ToInteger();
			lbIndex += delta;
			SetLeaderboardIndex();
			return true;
		}
		case WIDGET_ACTION_SCROLL_VERTICAL_VARIABLE:
		{
			if( parms.Num() == 0 )
			{
				return true;
			}

			if( options == NULL )
			{
				return true;
			}

			int dir = parms[ 0 ].ToInteger();
			if( lbCache->Scroll( dir ) )
			{
				// TODO_SPARTY: play scroll sound
				refreshLeaderboard = true;
			}

			return true;
		}
		case WIDGET_ACTION_SCROLL_PAGE:
		{
			if( parms.Num() == 0 )
			{
				return true;
			}

			if( options == NULL )
			{
				return true;
			}

			int dir = parms[ 0 ].ToInteger();
			if( lbCache->ScrollOffset( dir ) )
			{
				refreshLeaderboard = true;
			}

			return true;
		}
	}

	return idMenuWidget::HandleAction( action, event, widget, forceHandled );
}

/*
========================
idMenuScreen_Shell_Leaderboards::UpdateLeaderboard
========================
*/
void idMenuScreen_Shell_Leaderboards::UpdateLeaderboard( const idLeaderboardCallback* callback )
{

	lbCache->Update( callback );

	if( callback->GetErrorCode() != LEADERBOARD_ERROR_NONE )
	{
		if( !session->GetSignInManager().IsMasterLocalUserOnline() )
		{
			refreshWhenMasterIsOnline = true;
		}
	}

	refreshLeaderboard = true;
}

/*
========================
idMenuScreen_Shell_Leaderboards::SetLeaderboardIndex
========================
*/
void idMenuScreen_Shell_Leaderboards::SetLeaderboardIndex()
{

	if( lbIndex >= leaderboards.Num() )
	{
		lbIndex = 0;
	}
	else if( lbIndex < 0 )
	{
		lbIndex = leaderboards.Num() - 1;
	}

	const leaderboardDefinition_t* leaderboardDef = leaderboards[ lbIndex ].lb;
	for( int i = 0; i < leaderboardDef->numColumns; i++ )
	{
		/*if ( leaderboardDef->columnDefs[i].displayType != STATS_COLUMN_NEVER_DISPLAY ) {
			gameLocal->GetMainMenu()->mainMenu->SetGlobal( va("columnname%d",i), leaderboardDef->columnDefs[i].locDisplayName );
		}*/
	}

	lbCache->SetLeaderboard( leaderboardDef, lbCache->GetFilter() );

	refreshLeaderboard = true;

}

/*
========================
idMenuScreen_Shell_Leaderboards::RefreshLeaderboard
========================
*/
void idMenuScreen_Shell_Leaderboards::RefreshLeaderboard()
{

	if( refreshWhenMasterIsOnline )
	{
		SetLeaderboardIndex();
		refreshWhenMasterIsOnline = false;
	}

	if( !refreshLeaderboard )
	{
		return;
	}

	refreshLeaderboard = false;
	bool upArrow = false;
	bool downArrow = false;

	int focusIndex = -1;
	idList< idList< idStr, TAG_IDLIB_LIST_MENU >, TAG_IDLIB_LIST_MENU > lbListings;

	if( !lbCache->IsLoadingNewLeaderboard() && lbCache->GetErrorCode() == LEADERBOARD_DISPLAY_ERROR_NONE )
	{
		for( int addIndex = 0; addIndex < MAX_STAT_LISTINGS; ++addIndex )
		{

			idList< idStr > values;

			int index = lbCache->GetRowOffset() + addIndex;

			const idLeaderboardCallback::row_t* row = lbCache->GetLeaderboardRow( index );		// If this row is not in the cache, this will kick off a request
			if( row != NULL )
			{
				values.Append( va( "%i", ( int )row->rank ) );
				values.Append( row->name );
				values.Append( FormatColumn( &lbCache->GetLeaderboard()->columnDefs[0], row->columns[0] ).ToString() );
			}

			if( lbCache->GetEntryIndex() == addIndex )
			{
				focusIndex = addIndex;
			}

			lbListings.Append( values );
		}

		if( lbCache->GetRowOffset() != 0 )
		{
			upArrow = true;
		}

		if( ( lbCache->GetRowOffset() + MAX_STAT_LISTINGS ) < lbCache->GetNumRowsInLeaderboard() )
		{
			downArrow = true;
		}
	}

	if( lbHeading != NULL )
	{
		lbHeading->SetText( leaderboards[ lbIndex ].name );
		lbHeading->SetStrokeInfo( true, 0.75f, 1.75f );
	}

	if( focusIndex >= 0 )
	{
		options->SetFocusIndex( focusIndex );
	}

	if( btnPageDwn != NULL && btnPageDwn->GetSprite() != NULL )
	{
		btnPageDwn->GetSprite()->SetVisible( downArrow );
	}

	if( btnPageUp != NULL && btnPageUp->GetSprite() != NULL )
	{
		btnPageUp->GetSprite()->SetVisible( upArrow );
	}

	options->SetListData( lbListings );
	Update();

	const char* leaderboardErrorStrings[] =
	{
		"",
		"#str_online_leaderboards_error_failed",			// failed
		"",													// not online - players are just taken back to multiplayer menu
		"#str_online_leaderboards_error_not_ranked",		// not ranked
	};

	compile_time_assert( sizeof( leaderboardErrorStrings ) / sizeof( leaderboardErrorStrings[0] ) == LEADERBOARD_DISPLAY_ERROR_MAX );

	bool isLoading = lbCache->IsLoadingNewLeaderboard();
	idStr error = leaderboardErrorStrings[ lbCache->GetErrorCode() ];

	if( isLoading )
	{
		ShowMessage( true, "#str_online_loading", true );
	}
	else
	{
		if( !error.IsEmpty() )
		{
			ShowMessage( true, error, false );
		}
		else
		{
			if( lbCache->GetNumRowsInLeaderboard() > 0 )
			{
				ShowMessage( false, "", false );
			}
			else
			{
				ShowMessage( true, "#str_online_leaderboards_no_data", false );
			}
		}
	}
}

/*
========================
idMenuScreen_Shell_Leaderboards::ShowMessage
========================
*/
void idMenuScreen_Shell_Leaderboards::ShowMessage( bool show, idStr message, bool spinner )
{

	if( !menuData || !menuData->GetGUI() )
	{
		return;
	}

	idSWFSpriteInstance* pacifier = menuData->GetGUI()->GetRootObject().GetNestedSprite( "menuLeaderboards", "info", "pacifier" );

	if( !pacifier )
	{
		return;
	}

	if( show )
	{

		if( spinner && options != NULL && options->GetSprite() != NULL )
		{
			options->GetSprite()->SetAlpha( 0.35f );
		}
		else if( options != NULL && options->GetSprite() != NULL )
		{
			options->GetSprite()->SetVisible( false );
		}

		pacifier->SetVisible( true );
		idSWFTextInstance* txtMsg = pacifier->GetScriptObject()->GetNestedText( "message" );
		if( txtMsg != NULL )
		{
			txtMsg->SetText( message );
			txtMsg->SetStrokeInfo( true, 0.75f, 1.75f );
		}

		idSWFSpriteInstance* spriteSpinner = pacifier->GetScriptObject()->GetNestedSprite( "graphic" );
		if( spriteSpinner != NULL )
		{
			spriteSpinner->StopFrame( spinner ? 1 : 2 );
		}

	}
	else
	{

		if( options != NULL && options->GetSprite() != NULL )
		{
			options->GetSprite()->SetVisible( true );
			options->GetSprite()->SetAlpha( 1.0f );
		}

		pacifier->SetVisible( false );
	}

}


//*************************************************************************************************************************
// LBCACHE
//*************************************************************************************************************************

class LBCallback : public idLeaderboardCallback
{
public:
	LBCallback() {}

	void Call()
	{
		gameLocal.Shell_UpdateLeaderboard( this );
	}

	LBCallback* Clone() const
	{
		return new LBCallback( *this );
	}
};

/*
========================
idLBCache::Pump
========================
*/
void idLBCache::Pump()
{
	if( loadingNewLeaderboard || requestingRows )
	{
		return;
	}

	if( pendingDef != NULL )
	{
		SetLeaderboard( pendingDef, pendingFilter );
	}
}

/*
========================
idLBCache::Reset
========================
*/
void idLBCache::Reset()
{
	for( int i = 0; i < NUM_ROW_BLOCKS; i++ )
	{
		rowBlocks[i].startIndex = 0;
		rowBlocks[i].rows.Clear();
	}

	def						= NULL;
	filter					= DEFAULT_LEADERBOARD_FILTER;
	pendingDef				= NULL;
	pendingFilter			= DEFAULT_LEADERBOARD_FILTER;
	rowOffset				= 0;
	requestingRows			= false;
	numRowsInLeaderboard	= 0;
	entryIndex				= 0;
	loadingNewLeaderboard	= false;
}

/*
========================
idLBCache::SetLeaderboard
========================
*/
void idLBCache::SetLeaderboard( const leaderboardDefinition_t* def_, leaderboardFilterMode_t filter_ )
{

	// If we are busy waiting on results from a previous request, queue up this request
	if( loadingNewLeaderboard || requestingRows )
	{
		pendingDef		= def_;
		pendingFilter	= filter_;
		return;
	}

	//idLib::Printf( "SetLeaderboard 0x%p.\n", def_ );

	// Reset all
	Reset();

	// Set leaderboard and filter
	def		= def_;
	filter	= filter_;

	loadingNewLeaderboard = true;		// This means we are waiting on the first set of results for this new leaderboard

	localIndex = -1;	// don't know where the user is in the rows yet

	// Kick off initial stats request (which is initially based on the filter type)
	if( filter == LEADERBOARD_FILTER_MYSCORE )
	{
		LBCallback cb;
		session->LeaderboardDownload( 0, def, 0, MAX_ROWS_PER_BLOCK, cb );
	}
	else if( filter == LEADERBOARD_FILTER_FRIENDS )
	{
		LBCallback cb;
		session->LeaderboardDownload( 0, def, -1, 100, cb );		// Request up to 100 friends
	}
	else
	{
		LBCallback cb;
		session->LeaderboardDownload( 0, def, rowOffset + 1, MAX_ROWS_PER_BLOCK, cb );
		//session->LeaderboardDownload( 0, def, rowOffset + 1, 10, cb );		// For testing
	}
}

/*
========================
idLBCache::CycleFilter
========================
*/
void idLBCache::CycleFilter()
{
	// Set the proper filter
	if( filter == LEADERBOARD_FILTER_OVERALL )
	{
		filter = LEADERBOARD_FILTER_MYSCORE;
	}
	else if( filter == LEADERBOARD_FILTER_MYSCORE )
	{
		filter = LEADERBOARD_FILTER_FRIENDS;
	}
	else
	{
		filter = LEADERBOARD_FILTER_OVERALL;
	}

	// Reset the leaderboard with the new filter
	SetLeaderboard( def, filter );
}

/*
========================
idLBCache::GetFilterStrType
========================
*/
idStr idLBCache::GetFilterStrType()
{
	if( filter == LEADERBOARD_FILTER_FRIENDS )
	{
		return idLocalization::GetString( "#str_swf_leaderboards_friends_heading" );
	}
	else if( filter == LEADERBOARD_FILTER_MYSCORE )
	{
		return idLocalization::GetString( "#str_swf_leaderboards_global_self_heading" );
	}

	return idLocalization::GetString( "#str_swf_leaderboards_global_heading" );
}

/*
========================
idLBCache::Scroll
========================
*/
bool idLBCache::Scroll( int amount )
{
	if( GetErrorCode() != LEADERBOARD_DISPLAY_ERROR_NONE )
	{
		return false;	// don't allow scrolling on errors
	}

	// Remember old offsets so we know if anything moved
	int oldEntryIndex = entryIndex;
	int oldRowOffset = rowOffset;

	// Move cursor index by scroll amount
	entryIndex += amount;

	// Clamp cursor index (scrolling row offset if we can)
	if( entryIndex < 0 )
	{
		rowOffset += entryIndex;
		entryIndex = 0;
	}
	else if( entryIndex >= numRowsInLeaderboard )
	{
		entryIndex = numRowsInLeaderboard - 1;
		rowOffset = entryIndex - ( MAX_STAT_LISTINGS - 1 );
	}
	else if( entryIndex >= MAX_STAT_LISTINGS )
	{
		rowOffset += entryIndex - ( MAX_STAT_LISTINGS - 1 );
		entryIndex = MAX_STAT_LISTINGS - 1;
	}

	// Clamp row offset
	rowOffset = idMath::ClampInt( 0, Max( numRowsInLeaderboard - MAX_STAT_LISTINGS, 0 ), rowOffset );

	// Let caller know if anything actually changed
	return ( oldEntryIndex != entryIndex || oldRowOffset != rowOffset );
}

/*
========================
idLBCache::ScrollOffset
========================
*/
bool idLBCache::ScrollOffset( int amount )
{
	if( GetErrorCode() != LEADERBOARD_DISPLAY_ERROR_NONE )
	{
		return false;	// don't allow scrolling on errors
	}

	// Remember old offsets so we know if anything moved
	int oldEntryIndex = entryIndex;
	int oldRowOffset = rowOffset;

	rowOffset += amount;

	// Clamp row offset
	rowOffset = idMath::ClampInt( 0, Max( numRowsInLeaderboard - MAX_STAT_LISTINGS, 0 ), rowOffset );

	if( rowOffset != oldRowOffset )
	{
		entryIndex -= amount;	// adjust in opposite direction so same item stays selected
		entryIndex = idMath::ClampInt( 0, rowOffset + ( MAX_STAT_LISTINGS - 1 ), entryIndex );
	}
	else
	{
		entryIndex += amount;
		entryIndex = idMath::ClampInt( 0, numRowsInLeaderboard - 1, entryIndex );
	}

	// Let caller know if anything actually changed
	return ( oldEntryIndex != entryIndex || oldRowOffset != rowOffset );
}

/*
========================
idLBCache::FindFreeRowBlock
========================
*/
idLBRowBlock* idLBCache::FindFreeRowBlock()
{
	int bestTime		= 0;
	int bestBlockIndex	= 0;

	for( int i = 0; i < NUM_ROW_BLOCKS; i++ )
	{
		if( rowBlocks[i].rows.Num() == 0 )
		{
			return &rowBlocks[i];		// Prefer completely empty blocks
		}

		// Search for oldest block in the mean time
		if( i == 0 || rowBlocks[i].lastTime < bestTime )
		{
			bestBlockIndex = i;
			bestTime = rowBlocks[i].lastTime;
		}
	}

	return &rowBlocks[bestBlockIndex];
}

/*
========================
idLBCache::CallbackErrorToDisplayError
========================
*/
leaderboardDisplayError_t idLBCache::CallbackErrorToDisplayError( leaderboardError_t errorCode )
{
	switch( errorCode )
	{
		case LEADERBOARD_ERROR_NONE:
			return LEADERBOARD_DISPLAY_ERROR_NONE;
		default:
			return LEADERBOARD_DISPLAY_ERROR_FAILED;
	}
}

/*
========================
idLBCache::Update
========================
*/
void idLBCache::Update( const idLeaderboardCallback* callback )
{
	requestingRows = false;

	//idLib::Printf( "Stats returned.\n" );

	errorCode = CallbackErrorToDisplayError( callback->GetErrorCode() );

	// check if trying to view "My Score" leaderboard when you aren't ranked yet
	if( loadingNewLeaderboard && filter == LEADERBOARD_FILTER_MYSCORE && callback->GetLocalIndex() == -1 && errorCode == LEADERBOARD_DISPLAY_ERROR_NONE )
	{
		errorCode = LEADERBOARD_DISPLAY_ERROR_NOT_RANKED;
	}

	if( errorCode != LEADERBOARD_DISPLAY_ERROR_NONE )
	{
		numRowsInLeaderboard = 0;
		loadingNewLeaderboard = false;

		switch( errorCode )
		{
			case LEADERBOARD_DISPLAY_ERROR_NOT_ONLINE:
				/*idMenuHandler_Shell * shell = gameLocal.Shell_GetHandler();
				if ( shell != NULL ) {
					shell->SetNextScreen( SHELL_AREA_ROOT, MENU_TRANSITION_SIMPLE );
				}*/
				common->Dialog().AddDialog( GDM_CONNECTION_LOST, DIALOG_ACCEPT, NULL, NULL, true, "", 0, true );
				break;
			default:
				break;
		}

		return;
	}

	if( callback->GetDef() != def )
	{
		// Not the leaderboard we are looking for (This should no longer be possible)
		idLib::Printf( "Wrong leaderboard.\n" );
		numRowsInLeaderboard = 0;
		loadingNewLeaderboard = false;
		return;
	}

	// Get total rows in this leaderboard
	numRowsInLeaderboard = callback->GetNumRowsInLeaderboard();

	// Store the index that the master user is in, if we haven't already found the index
	if( callback->GetLocalIndex() != -1 )
	{
		localIndex = callback->GetStartIndex() + callback->GetLocalIndex();
	}

	if( loadingNewLeaderboard == true )
	{
		// Default to viewing the local user (if the right filter mode is set)
		if( callback->GetLocalIndex() != -1 && ( filter == LEADERBOARD_FILTER_MYSCORE || filter == LEADERBOARD_FILTER_FRIENDS ) )
		{
			// Put their name and cursor at the top
			rowOffset = callback->GetLocalIndex() + callback->GetStartIndex();
			entryIndex = 0;

			// Scroll the cursor up to center their name as much as possible
			Scroll( -MAX_STAT_LISTINGS / 2 );

			// Set the cursor to their name
			entryIndex = ( callback->GetLocalIndex() + callback->GetStartIndex() ) - rowOffset;
		}
		loadingNewLeaderboard = false;
	}

	// Find a a row block to store these new rows
	idLBRowBlock* rowBlock	= FindFreeRowBlock();

	rowBlock->lastTime		= Sys_Milliseconds();			// Freshen row
	rowBlock->startIndex	= callback->GetStartIndex();
	rowBlock->rows			= callback->GetRows();
}

/*
========================
idLBCache::GetLeaderboardRow
========================
*/
const idLeaderboardCallback::row_t* idLBCache::GetLeaderboardRow( int row )
{
	if( loadingNewLeaderboard )
	{
		return NULL;		// If we are refreshing (seeing this leaderboard for the first time), force NULL till we get first set of results
	}

	if( row >= numRowsInLeaderboard )
	{
		return NULL;
	}

	// Find it in the cache
	for( int i = 0; i < NUM_ROW_BLOCKS; i++ )
	{
		int startIndex = rowBlocks[i].startIndex;
		int lastIndex = startIndex + rowBlocks[i].rows.Num() - 1;
		if( row >= startIndex && row <= lastIndex )
		{
			rowBlocks[i].lastTime = Sys_Milliseconds();		// Freshen row
			return &rowBlocks[i].rows[row - startIndex];
		}
	}

	// Not found, kick off a request to download it
	// (this will not allow more than one request at a time)
	if( !requestingRows )
	{
		// If we don't have this row, kick off a request to get it
		LBCallback cb;
		requestingRows = true;
		session->LeaderboardDownload( 0, def, row + 1, MAX_ROWS_PER_BLOCK, cb );
		//session->LeaderboardDownload( 0, def, row + 1, 10, cb );
		//idLib::Printf( "Stat request\n" );
	}

	return NULL;
}


/*
========================
idMainMenu::SetSPLeaderboardFromMenuSettings
========================
*/
void idLBCache::DisplayGamerCardUI( const idLeaderboardCallback::row_t* row )
{
}