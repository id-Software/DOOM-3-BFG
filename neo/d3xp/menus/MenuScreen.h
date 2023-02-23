/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2014 Robert Beckebans

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
#ifndef __MENUSCREEN_H__
#define __MENUSCREEN_H__

#ifndef __TYPEINFOGEN__
	#include "../../renderer/RenderCommon.h"
#endif

enum mainMenuTransition_t
{
	MENU_TRANSITION_INVALID = -1,
	MENU_TRANSITION_SIMPLE,
	MENU_TRANSITION_ADVANCE,
	MENU_TRANSITION_BACK,
	MENU_TRANSITION_FORCE
};

enum cursorState_t
{
	CURSOR_NONE,
	CURSOR_IN_COMBAT,
	CURSOR_TALK,
	CURSOR_GRABBER,
	CURSOR_ITEM,
};

/*
================================================
idLBRowBlock
================================================
*/
class idLBRowBlock
{
public:
	idLBRowBlock() : lastTime( 0 ), startIndex( 0 ) {}

	int										lastTime;
	int										startIndex;
	idList< idLeaderboardCallback::row_t >	rows;
};

enum leaderboardFilterMode_t
{
	LEADERBOARD_FILTER_OVERALL	= 0,
	LEADERBOARD_FILTER_MYSCORE	= 1,
	LEADERBOARD_FILTER_FRIENDS	= 2
};

/*
================================================
idLBCache
================================================
*/
class idLBCache
{
public:
	static const int NUM_ROW_BLOCKS		= 5;
	static const leaderboardFilterMode_t DEFAULT_LEADERBOARD_FILTER = LEADERBOARD_FILTER_OVERALL;

	idLBCache() :
		def( NULL ),
		filter( DEFAULT_LEADERBOARD_FILTER ),
		pendingDef( NULL ),
		pendingFilter( DEFAULT_LEADERBOARD_FILTER ),
		requestingRows( false ),
		loadingNewLeaderboard( false ),
		numRowsInLeaderboard( 0 ),
		entryIndex( 0 ),
		rowOffset( 0 ),
		localIndex( -1 ),
		errorCode( LEADERBOARD_DISPLAY_ERROR_NONE ) {}

	void									Pump();
	void									Reset();
	void									SetLeaderboard( const leaderboardDefinition_t* 	def_, leaderboardFilterMode_t filter_ = DEFAULT_LEADERBOARD_FILTER );
	void									CycleFilter();
	leaderboardFilterMode_t					GetFilter() const
	{
		return filter;
	}
	idStr									GetFilterStrType();
	bool									Scroll( int amount );
	bool									ScrollOffset( int amount );
	idLBRowBlock* 							FindFreeRowBlock();
	void									Update( const idLeaderboardCallback* callback );
	const idLeaderboardCallback::row_t* 	GetLeaderboardRow( int row );

	const leaderboardDefinition_t* 			GetLeaderboard() const
	{
		return def;
	}
	int										GetNumRowsInLeaderboard() const
	{
		return numRowsInLeaderboard;
	}

	int										GetEntryIndex() const
	{
		return entryIndex;
	}
	int										GetRowOffset() const
	{
		return rowOffset;
	}
	int										GetLocalIndex() const
	{
		return localIndex;
	}
	leaderboardDisplayError_t				GetErrorCode() const
	{
		return errorCode;
	}

	bool									IsRequestingRows() const
	{
		return requestingRows;
	}
	bool									IsLoadingNewLeaderboard() const
	{
		return loadingNewLeaderboard;
	}

	void									SetEntryIndex( int value )
	{
		entryIndex = value;
	}
	void									SetRowOffset( int value )
	{
		rowOffset = value;
	}

	void									DisplayGamerCardUI( const idLeaderboardCallback::row_t* row );

private:
	leaderboardDisplayError_t				CallbackErrorToDisplayError( leaderboardError_t errorCode );

	idLBRowBlock					rowBlocks[NUM_ROW_BLOCKS];

	const leaderboardDefinition_t* 	def;
	leaderboardFilterMode_t			filter;

	// Pending def and filter are simply used to queue up SetLeaderboard calls when the system is currently
	// busy waiting on results from a previous SetLeaderboard/GetLeaderboardRow call.
	// This is so we only have ONE request in-flight at any given time.
	const leaderboardDefinition_t* 	pendingDef;
	leaderboardFilterMode_t			pendingFilter;

	bool							requestingRows;				// True while requested rows are in flight
	bool							loadingNewLeaderboard;		// True when changing to a new leaderboard (or filter type)

	int								numRowsInLeaderboard;		// Total rows in this leaderboard (they won't all be cached though)
	int								entryIndex;					// Relative row offset (from top of viewing window)
	int								rowOffset;					// Absolute row offset
	int								localIndex;					// Row for the master local user (-1 means not on leaderboard)

	leaderboardDisplayError_t		errorCode;					// Error state of the leaderboard
};

/*
================================================
idMenuScreen
================================================
*/
class idMenuScreen : public idMenuWidget
{
public:

	idMenuScreen();
	virtual ~idMenuScreen();

	virtual void				Update();
	virtual void				UpdateCmds();
	virtual void				HandleMenu( const mainMenuTransition_t type );

	virtual void				ShowScreen( const mainMenuTransition_t transitionType );
	virtual void				HideScreen( const mainMenuTransition_t transitionType );

	virtual void				ObserveEvent( const idMenuWidget& widget, const idWidgetEvent& event );
	virtual void				SetScreenGui( idSWF* gui )
	{
		menuGUI = gui;
	}

protected:

	idSWF* 	menuGUI;
	mainMenuTransition_t	transition;

};

/*
================================================
idMenuScreen_PDA_UserData
================================================
*/
class idMenuScreen_PDA_UserData : public idMenuScreen
{
public:

	idMenuScreen_PDA_UserData() {}

	virtual ~idMenuScreen_PDA_UserData() {}
	virtual void					Initialize( idMenuHandler* data );
	virtual void					Update();
	virtual void					ShowScreen( const mainMenuTransition_t transitionType );
	virtual void					HideScreen( const mainMenuTransition_t transitionType );
	virtual bool					HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled = false );
	idMenuWidget_PDA_UserData* 		GetUserData()
	{
		return &pdaUserData;
	}
	idMenuWidget_PDA_Objective* 	GetObjective()
	{
		return &pdaObjectiveSimple;
	}
	idMenuWidget_PDA_AudioFiles* 	GetAudioFiles()
	{
		return &pdaAudioFiles;
	}

private:
	idMenuWidget_PDA_UserData 			pdaUserData;
	idMenuWidget_PDA_Objective 		pdaObjectiveSimple;
	idMenuWidget_PDA_AudioFiles 		pdaAudioFiles;
};

/*
================================================
idMenuScreen_PDA_UserEmails
================================================
*/
class idMenuScreen_PDA_UserEmails : public idMenuScreen
{
public:
	idMenuScreen_PDA_UserEmails() :
		readingEmails( false ),
		scrollEmailInfo( false )
	{
	}

	virtual ~idMenuScreen_PDA_UserEmails()
	{
	}

	virtual void					Update();
	virtual void					Initialize( idMenuHandler* data );
	virtual void					ShowScreen( const mainMenuTransition_t transitionType );
	virtual void					HideScreen( const mainMenuTransition_t transitionType );
	virtual bool					HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled = false );
	virtual void					ObserveEvent( const idMenuWidget& widget, const idWidgetEvent& event );
	idMenuWidget_PDA_EmailInbox& 	GetInbox()
	{
		return pdaInbox;
	}

	bool							ScrollCorrectList( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget );
	void							ShowEmail( bool show );
	void							UpdateEmail();
private:
	idMenuWidget_PDA_EmailInbox 	pdaInbox;
	idMenuWidget_InfoBox 			emailInfo;
	idMenuWidget_ScrollBar 			emailScrollbar;
	bool							readingEmails;
	bool							scrollEmailInfo;
};

/*
================================================
idMenuScreen_PDA_UserData
================================================
*/
class idMenuScreen_PDA_VideoDisks : public idMenuScreen
{
public:
	idMenuScreen_PDA_VideoDisks() :
		activeVideo( NULL )
	{
	}

	virtual ~idMenuScreen_PDA_VideoDisks()
	{
	}

	virtual void				Initialize( idMenuHandler* data );
	virtual void				Update();
	virtual void				ShowScreen( const mainMenuTransition_t transitionType );
	virtual void				HideScreen( const mainMenuTransition_t transitionType );
	virtual bool				HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled = false );

	void						ToggleVideoDiskPlay();
	void						UpdateVideoDetails();
	void						SelectedVideoToPlay( int index );
	void						ClearActiveVideo()
	{
		activeVideo = NULL;
	}
	const idDeclVideo* 			GetActiveVideo()
	{
		return activeVideo;
	}
private:
	idMenuWidget_ScrollBar		scrollbar;
	idMenuWidget_DynamicList 		pdaVideoList;
	idMenuWidget_PDA_VideoInfo 	videoDetails;
	idList< idList< idStr, TAG_IDLIB_LIST_MENU>, TAG_IDLIB_LIST_MENU >		videoItems;
	const idDeclVideo* 				activeVideo;
};

/*
================================================
idMenuScreen_PDA_UserData
================================================
*/
class idMenuScreen_PDA_Inventory : public idMenuScreen
{
public:
	idMenuScreen_PDA_Inventory()
	{
	}
	virtual void				Initialize( idMenuHandler* data );
	virtual void				Update();
	virtual void				ShowScreen( const mainMenuTransition_t transitionType );
	virtual void				HideScreen( const mainMenuTransition_t transitionType );
	virtual bool				HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled = false );

	void						EquipWeapon();
	const char* 				GetWeaponName( int index );
	bool						IsVisibleWeapon( int index );

private:
	idMenuWidget_Carousel 			itemList;
	idMenuWidget_InfoBox 			infoBox;
};

//*
//================================================
//idMenuScreen_Shell_Root
//================================================
//*/
class idMenuScreen_Shell_Root : public idMenuScreen
{
public:
	idMenuScreen_Shell_Root() :
		options( NULL ),
		helpWidget( NULL )
	{
	}
	virtual void				Initialize( idMenuHandler* data );
	virtual void				Update();
	virtual void				ShowScreen( const mainMenuTransition_t transitionType );
	virtual void				HideScreen( const mainMenuTransition_t transitionType );
	virtual bool				HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled = false );

	void						HandleExitGameBtn();
	int							GetRootIndex();
	void						SetRootIndex( int index );
	idMenuWidget_Help* 			GetHelpWidget()
	{
		return helpWidget;
	}

private:
	idMenuWidget_DynamicList* 	options;
	idMenuWidget_Help* 			helpWidget;
};

//*
//================================================
//idMenuScreen_Shell_Pause
//================================================
//*/
class idMenuScreen_Shell_Pause : public idMenuScreen
{
public:
	idMenuScreen_Shell_Pause() :
		options( NULL ),
		isMpPause( false )
	{
	}
	virtual void				Initialize( idMenuHandler* data );
	virtual void				Update();
	virtual void				ShowScreen( const mainMenuTransition_t transitionType );
	virtual void				HideScreen( const mainMenuTransition_t transitionType );
	virtual bool				HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled = false );

	void						HandleExitGameBtn();
	void						HandleRestartBtn();

private:
	idMenuWidget_DynamicList* 	options;
	bool						isMpPause;
};

//*
//================================================
//idMenuScreen_Shell_PressStart
//================================================
//*/
class idMenuScreen_Shell_PressStart : public idMenuScreen
{
public:
	idMenuScreen_Shell_PressStart() :
		startButton( NULL ),
		options( NULL ),
		itemList( NULL ),
		doomCover( NULL ),
		doom2Cover( NULL ),
		doom3Cover( NULL )
	{
	}
	virtual void				Initialize( idMenuHandler* data );
	virtual void				Update();
	virtual void				ShowScreen( const mainMenuTransition_t transitionType );
	virtual void				HideScreen( const mainMenuTransition_t transitionType );
	virtual bool				HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled = false );
private:
	idMenuWidget_Button* 		startButton;
	idMenuWidget_DynamicList* 	options;
	idMenuWidget_Carousel* 		itemList;
	const idMaterial* 			doomCover;
	const idMaterial* 			doom2Cover;
	const idMaterial* 			doom3Cover;
};

//*
//================================================
//idMenuScreen_Shell_PressStart
//================================================
//*/
class idMenuScreen_Shell_GameSelect : public idMenuScreen
{
public:
	idMenuScreen_Shell_GameSelect() :
		startButton( NULL ),
		options( NULL ),
		itemList( NULL ),
		doomCover( NULL ),
		doom2Cover( NULL ),
		doom3Cover( NULL )
	{
	}
	virtual void				Initialize( idMenuHandler* data );
	virtual void				Update();
	virtual void				ShowScreen( const mainMenuTransition_t transitionType );
	virtual void				HideScreen( const mainMenuTransition_t transitionType );
	virtual bool				HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled = false );
private:
	idMenuWidget_Button* 		startButton;
	idMenuWidget_DynamicList* 	options;
	idMenuWidget_Carousel* 		itemList;
	const idMaterial* 			doomCover;
	const idMaterial* 			doom2Cover;
	const idMaterial* 			doom3Cover;
};

//*
//================================================
//idMenuScreen_Shell_Singleplayer
//================================================
//*/
class idMenuScreen_Shell_Singleplayer : public idMenuScreen
{
public:
	idMenuScreen_Shell_Singleplayer() :
		canContinue( false ),
		options( NULL ),
		btnBack( NULL )
	{
	}
	virtual void				Initialize( idMenuHandler* data );
	virtual void				Update();
	virtual void				ShowScreen( const mainMenuTransition_t transitionType );
	virtual void				HideScreen( const mainMenuTransition_t transitionType );
	virtual bool				HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled = false );

	void						SetCanContinue( bool valid )
	{
		canContinue = valid;
	}
	void						ContinueGame();
private:
	bool						canContinue;
	idMenuWidget_DynamicList* 	options;
	idMenuWidget_Button*			btnBack;
};

//*
//================================================
//idMenuScreen_Shell_Settings
//================================================
//*/
class idMenuScreen_Shell_Settings : public idMenuScreen
{
public:
	idMenuScreen_Shell_Settings() :
		options( NULL ),
		btnBack( NULL )
	{
	}
	virtual void				Initialize( idMenuHandler* data );
	virtual void				Update();
	virtual void				ShowScreen( const mainMenuTransition_t transitionType );
	virtual void				HideScreen( const mainMenuTransition_t transitionType );
	virtual bool				HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled = false );
private:
	idMenuWidget_DynamicList* 	options;
	idMenuWidget_Button*			btnBack;
};

struct creditInfo_t
{

	creditInfo_t()
	{
		type = -1;
		entry = "";
	}

	creditInfo_t( int t, const char* val )
	{
		type = t;
		entry = val;
	}

	int type;
	idStr entry;
};

//*
//================================================
//idMenuScreen_Shell_Credits
//================================================
//*/
class idMenuScreen_Shell_Credits : public idMenuScreen
{
public:
	idMenuScreen_Shell_Credits() :
		btnBack( NULL ),
		creditIndex( 0 )
	{
	}
	virtual void				Initialize( idMenuHandler* data );
	virtual void				Update();
	virtual void				ShowScreen( const mainMenuTransition_t transitionType );
	virtual void				HideScreen( const mainMenuTransition_t transitionType );
	virtual bool				HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled = false );

	void						SetupCreditList();
	void						UpdateCredits();

private:
	idMenuWidget_Button*			btnBack;
	idList< creditInfo_t >		creditList;
	int							creditIndex;
};

//*
//================================================
//idMenuScreen_Shell_Resolution
//================================================
//*/
class idMenuScreen_Shell_Resolution : public idMenuScreen
{
public:
	idMenuScreen_Shell_Resolution() :
		options( NULL ),
		btnBack( NULL )
	{
	}
	virtual void				Initialize( idMenuHandler* data );
	virtual void				Update();
	virtual void				ShowScreen( const mainMenuTransition_t transitionType );
	virtual void				HideScreen( const mainMenuTransition_t transitionType );
	virtual bool				HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled = false );

private:
	struct optionData_t
	{
		optionData_t()
		{
			fullscreen = -1;
			vidmode = -1;
		}
		optionData_t( int f, int v )
		{
			fullscreen = f;
			vidmode = v;
		}
		optionData_t( const optionData_t& other )
		{
			fullscreen = other.fullscreen;
			vidmode = other.vidmode;
		}
		const optionData_t& operator=( const optionData_t& other )
		{
			fullscreen = other.fullscreen;
			vidmode = other.vidmode;
			return *this;
		}
		bool operator==( const optionData_t& other ) const
		{
			return ( fullscreen == other.fullscreen ) && ( ( vidmode == other.vidmode ) || ( fullscreen == 0 ) );
		}
		int fullscreen;
		int vidmode;
	};
	idList<optionData_t>		optionData;

	optionData_t				originalOption;

	idMenuWidget_DynamicList* 	options;
	idMenuWidget_Button*			btnBack;
};

//*
//================================================
//idMenuScreen_Shell_Difficulty
//================================================
//*/
class idMenuScreen_Shell_Difficulty : public idMenuScreen
{
public:
	idMenuScreen_Shell_Difficulty() :
		nightmareUnlocked( false ),
		options( NULL ),
		btnBack( NULL )
	{
	}
	virtual void				Initialize( idMenuHandler* data );
	virtual void				Update();
	virtual void				ShowScreen( const mainMenuTransition_t transitionType );
	virtual void				HideScreen( const mainMenuTransition_t transitionType );
	virtual bool				HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled = false );
private:
	bool						nightmareUnlocked;
	idMenuWidget_DynamicList* 	options;
	idMenuWidget_Button*		btnBack;
};

//*
//================================================
//idMenuScreen_Shell_Playstation
//================================================
//*/
class idMenuScreen_Shell_Playstation : public idMenuScreen
{
public:
	idMenuScreen_Shell_Playstation() :
		options( NULL ),
		btnBack( NULL )
	{
	}
	virtual void				Initialize( idMenuHandler* data );
	virtual void				Update();
	virtual void				ShowScreen( const mainMenuTransition_t transitionType );
	virtual void				HideScreen( const mainMenuTransition_t transitionType );
	virtual bool				HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled = false );
private:
	idMenuWidget_DynamicList* 	options;
	idMenuWidget_Button*			btnBack;
};

//*
//================================================
//idMenuScreen_Shell_ModeSelect
//================================================
//*/
class idMenuScreen_Shell_ModeSelect : public idMenuScreen
{
public:
	idMenuScreen_Shell_ModeSelect() :
		options( NULL ),
		btnBack( NULL )
	{
	}
	virtual void				Initialize( idMenuHandler* data );
	virtual void				Update();
	virtual void				ShowScreen( const mainMenuTransition_t transitionType );
	virtual void				HideScreen( const mainMenuTransition_t transitionType );
	virtual bool				HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled = false );
private:
	idMenuWidget_DynamicList* 	options;
	idMenuWidget_Button*			btnBack;
};

//*
//================================================
//idMenuScreen_GameBrowser
//================================================
//*/
class idMenuScreen_Shell_GameBrowser : public idMenuScreen
{
public:
	idMenuScreen_Shell_GameBrowser() :
		listWidget( NULL ),
		btnBack( NULL )
	{
	}

	virtual void				Initialize( idMenuHandler* data );
	virtual void				ShowScreen( const mainMenuTransition_t transitionType );
	virtual void				HideScreen( const mainMenuTransition_t transitionType );
	virtual bool				HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandle = false );

	void						UpdateServerList();
	void						OnServerListReady();
	void						DescribeServer( const serverInfo_t& server, const int index );

private:
	idMenuWidget_GameBrowserList* listWidget;
	idMenuWidget_Button*				btnBack;
};

//*
//================================================
//idMenuScreen_Shell_Leaderboards
//================================================
//*/
class idMenuScreen_Shell_Leaderboards : public idMenuScreen
{
public:
	idMenuScreen_Shell_Leaderboards() :
		options( NULL ),
		btnBack( NULL ),
		btnPrev( NULL ),
		btnNext( NULL ),
		btnPageDwn( NULL ),
		btnPageUp( NULL ),
		lbCache( NULL ),
		lbHeading( NULL ),
		lbIndex( 0 ),
		refreshLeaderboard( false ),
		refreshWhenMasterIsOnline( false )
	{
	}

	virtual ~idMenuScreen_Shell_Leaderboards();

	virtual void				Initialize( idMenuHandler* data );
	virtual void				Update();
	virtual void				ShowScreen( const mainMenuTransition_t transitionType );
	virtual void				HideScreen( const mainMenuTransition_t transitionType );
	virtual bool				HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled = false );

	void						UpdateLeaderboard( const idLeaderboardCallback* callback );
	void						PumpLBCache();
	void						RefreshLeaderboard();
	void						ShowMessage( bool show, idStr message, bool spinner );
	void						ClearLeaderboard();
	void 						SetLeaderboardIndex();

protected:

	struct	doomLeaderboard_t
	{
		doomLeaderboard_t() : lb( NULL ) { }
		doomLeaderboard_t( const leaderboardDefinition_t* _lb, idStr _name )
		{
			lb = _lb;
			name = _name;
		}
		const leaderboardDefinition_t* 	lb;
		idStr					name;
	};

	idList< doomLeaderboard_t >	leaderboards;

	idMenuWidget_DynamicList* 	options;
	idMenuWidget_Button*			btnBack;
	idMenuWidget_Button*			btnPrev;
	idMenuWidget_Button*			btnNext;
	idMenuWidget_Button*			btnPageDwn;
	idMenuWidget_Button*			btnPageUp;
	idLBCache* 					lbCache;
	idSWFTextInstance* 			lbHeading;
	int							lbIndex;
	bool						refreshLeaderboard;
	bool						refreshWhenMasterIsOnline;
};

//*
//================================================
//idMenuScreen_Shell_Bindings
//================================================
//*/
class idMenuScreen_Shell_Bindings : public idMenuScreen
{
public:
	idMenuScreen_Shell_Bindings() :
		options( NULL ),
		restoreDefault( NULL ),
		blinder( NULL ),
		txtBlinder( NULL ),
		btnBack( NULL ),
		bindingsChanged( false )
	{
	}
	virtual void				Initialize( idMenuHandler* data );
	virtual void				Update();
	virtual void				ShowScreen( const mainMenuTransition_t transitionType );
	virtual void				HideScreen( const mainMenuTransition_t transitionType );
	virtual bool				HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled = false );

	void						SetBinding( int keyNum );
	void						UpdateBindingDisplay();
	void						ToggleWait( bool wait );
	void						SetBindingChanged( bool changed )
	{
		bindingsChanged = changed;
	}

protected:
	void						HandleRestoreDefaults();

	idMenuWidget_DynamicList* 	options;
	idMenuWidget_Button* 		restoreDefault;
	idSWFSpriteInstance* 		blinder;
	idSWFSpriteInstance* 		txtBlinder;
	idMenuWidget_Button*		btnBack;
	bool						bindingsChanged;
};

//*
//================================================
//idMenuScreen_Shell_Dev
//================================================
//*/
class idMenuScreen_Shell_Dev : public idMenuScreen
{
public:

	struct devOption_t
	{
		devOption_t()
		{
			map = "";
			name = "";
		};

		devOption_t( const char* m, const char* n )
		{
			map = m;
			name = n;
		}

		const char* 	map;
		const char* 	name;
	};

	idMenuScreen_Shell_Dev() :
		options( NULL ),
		btnBack( NULL )
	{
	}
	virtual void				Initialize( idMenuHandler* data );
	virtual void				Update();
	virtual void				ShowScreen( const mainMenuTransition_t transitionType );
	virtual void				HideScreen( const mainMenuTransition_t transitionType );
	virtual bool				HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled = false );

	void						SetupDevOptions();

private:
	idMenuWidget_DynamicList* 	options;
	idMenuWidget_Button*			btnBack;
	idList< devOption_t, TAG_IDLIB_LIST_MENU >		devOptions;
};

//*
//================================================
//idMenuScreen_Shell_NewGame
//================================================
//*/
class idMenuScreen_Shell_NewGame : public idMenuScreen
{
public:
	idMenuScreen_Shell_NewGame() :
		options( NULL ),
		btnBack( NULL )
	{
	}
	virtual void				Initialize( idMenuHandler* data );
	virtual void				Update();
	virtual void				ShowScreen( const mainMenuTransition_t transitionType );
	virtual void				HideScreen( const mainMenuTransition_t transitionType );
	virtual bool				HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled = false );
private:
	idMenuWidget_DynamicList* 	options;
	idMenuWidget_Button*			btnBack;
};

//*
//================================================
//idMenuScreen_Shell_Load
//================================================
//*/
class idMenuScreen_Shell_Load : public idMenuScreen
{
public:
	idMenuScreen_Shell_Load() :
		options( NULL ),
		saveInfo( NULL ),
		btnBack( NULL ),
		btnDelete( NULL )
	{
	}
	virtual void				Initialize( idMenuHandler* data );
	virtual void				Update();
	virtual void				ShowScreen( const mainMenuTransition_t transitionType );
	virtual void				HideScreen( const mainMenuTransition_t transitionType );
	virtual bool				HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled = false );

	void						UpdateSaveEnumerations();
	void						LoadDamagedGame( int index );
	void						LoadGame( int index );
	void						DeleteGame( int index );
	saveGameDetailsList_t		GetSortedSaves() const
	{
		return sortedSaves;
	}

private:
	idMenuWidget_DynamicList* 	options;
	idMenuWidget_Shell_SaveInfo* saveInfo;
	idMenuWidget_Button*			btnBack;
	idMenuWidget_Button* 		btnDelete;
	saveGameDetailsList_t		sortedSaves;
};

//*
//================================================
//idMenuScreen_Shell_Save
//================================================
//*/
class idMenuScreen_Shell_Save : public idMenuScreen
{
public:
	idMenuScreen_Shell_Save() :
		btnBack( NULL ),
		options( NULL ),
		saveInfo( NULL ),
		btnDelete( NULL )
	{
	}
	virtual void				Initialize( idMenuHandler* data );
	virtual void				Update();
	virtual void				ShowScreen( const mainMenuTransition_t transitionType );
	virtual void				HideScreen( const mainMenuTransition_t transitionType );
	virtual bool				HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled = false );
	saveGameDetailsList_t		GetSortedSaves() const
	{
		return sortedSaves;
	}

	void						UpdateSaveEnumerations();
	void						SaveGame( int index );
	void						DeleteGame( int index );

private:
	idMenuWidget_Button*			btnBack;
	idMenuWidget_DynamicList* 		options;
	idMenuWidget_Shell_SaveInfo* 	saveInfo;
	idMenuWidget_Button* 			btnDelete;
	saveGameDetailsList_t			sortedSaves;
};

//*
//================================================
//idMenuScreen_Shell_GameOptions
//================================================
//*/
class idMenuScreen_Shell_GameOptions : public idMenuScreen
{
public:

	/*
	================================================
	idMenuDataSource_GameSettings
	================================================
	*/
	class idMenuDataSource_GameSettings : public idMenuDataSource
	{
	public:
		enum gameSettingFields_t
		{
			GAME_FIELD_FOV,
			GAME_FIELD_CHECKPOINTS,
			GAME_FIELD_AUTO_SWITCH,
			GAME_FIELD_AUTO_RELOAD,
			GAME_FIELD_AIM_ASSIST,
			GAME_FIELD_ALWAYS_SPRINT,
			GAME_FIELD_FLASHLIGHT_SHADOWS,
			GAME_FIELD_MUZZLE_FLASHES,
			MAX_GAME_FIELDS
		};

		idMenuDataSource_GameSettings();

		// loads data
		virtual void				LoadData();

		// submits data
		virtual void				CommitData();

		// says whether something changed with the data
		virtual bool				IsDataChanged() const;

		// retrieves a particular field for reading or updating
		virtual idSWFScriptVar		GetField( const int fieldIndex ) const
		{
			return fields[ fieldIndex ];
		}

		virtual void				AdjustField( const int fieldIndex, const int adjustAmount );

	private:
		idStaticList< idSWFScriptVar, MAX_GAME_FIELDS >	fields;
		idStaticList< idSWFScriptVar, MAX_GAME_FIELDS >	originalFields;
	};

	idMenuScreen_Shell_GameOptions() :
		options( NULL ),
		btnBack( NULL )
	{
	}
	virtual void				Initialize( idMenuHandler* data );
	virtual void				Update();
	virtual void				ShowScreen( const mainMenuTransition_t transitionType );
	virtual void				HideScreen( const mainMenuTransition_t transitionType );
	virtual bool				HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled = false );
private:
	idMenuWidget_DynamicList* 	options;
	idMenuDataSource_GameSettings	systemData;
	idMenuWidget_Button*			btnBack;
};

//*
//================================================
//idMenuScreen_Shell_GameOptions
//================================================
//*/
class idMenuScreen_Shell_MatchSettings : public idMenuScreen
{
public:

	/*
	================================================
	idMenuDataSource_MatchSettings
	================================================
	*/
	class idMenuDataSource_MatchSettings : public idMenuDataSource
	{
	public:
		enum matchSettingFields_t
		{
			MATCH_FIELD_MODE,
			MATCH_FIELD_MAP,
			MATCH_FIELD_TIME,
			MATCH_FIELD_SCORE,
			MAX_MATCH_FIELDS
		};

		idMenuDataSource_MatchSettings();

		// loads data
		virtual void				LoadData();

		// submits data
		virtual void				CommitData();

		// says whether something changed with the data
		virtual bool				IsDataChanged() const;

		// retrieves a particular field for reading or updating
		virtual idSWFScriptVar		GetField( const int fieldIndex ) const
		{
			return fields[ fieldIndex ];
		}

		virtual void				AdjustField( const int fieldIndex, const int adjustAmount );

		bool						MapChanged()
		{
			return updateMap;
		}
		void						ClearMapChanged()
		{
			updateMap = false;
		}

	private:

		void						GetModeName( int index, idStr& name );
		void						GetMapName( int index, idStr& name );

		idStaticList< idSWFScriptVar, MAX_MATCH_FIELDS >	fields;
		idStaticList< idSWFScriptVar, MAX_MATCH_FIELDS >	originalFields;
		bool						updateMap;
	};

	idMenuScreen_Shell_MatchSettings() :
		options( NULL ),
		btnBack( NULL )
	{
	}
	virtual void				Initialize( idMenuHandler* data );
	virtual void				Update();
	virtual void				ShowScreen( const mainMenuTransition_t transitionType );
	virtual void				HideScreen( const mainMenuTransition_t transitionType );
	virtual bool				HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled = false );
private:
	idMenuWidget_DynamicList* 		options;
	idMenuDataSource_MatchSettings	matchData;
	idMenuWidget_Button*				btnBack;
};

//*
//================================================
//idMenuScreen_Shell_Controls
//================================================
//*/
class idMenuScreen_Shell_Controls : public idMenuScreen
{
public:

	/*
	================================================
	idMenuDataSource_ControlSettings
	================================================
	*/
	class idMenuDataSource_ControlSettings : public idMenuDataSource
	{
	public:
		enum controlSettingFields_t
		{
			CONTROLS_FIELD_INVERT_MOUSE,
			CONTROLS_FIELD_GAMEPAD_ENABLED,
			CONTROLS_FIELD_MOUSE_SENS,
			MAX_CONTROL_FIELDS
		};

		idMenuDataSource_ControlSettings();

		// loads data
		virtual void				LoadData();

		// submits data
		virtual void				CommitData();

		// says whether something changed with the data
		virtual bool				IsDataChanged() const;

		// retrieves a particular field for reading or updating
		virtual idSWFScriptVar		GetField( const int fieldIndex ) const
		{
			return fields[ fieldIndex ];
		}

		virtual void				AdjustField( const int fieldIndex, const int adjustAmount );

	private:
		idStaticList< idSWFScriptVar, MAX_CONTROL_FIELDS >	fields;
		idStaticList< idSWFScriptVar, MAX_CONTROL_FIELDS >	originalFields;
	};

	idMenuScreen_Shell_Controls() :
		options( NULL ),
		btnBack( NULL )
	{
	}
	virtual void				Initialize( idMenuHandler* data );
	virtual void				Update();
	virtual void				ShowScreen( const mainMenuTransition_t transitionType );
	virtual void				HideScreen( const mainMenuTransition_t transitionType );
	virtual bool				HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled = false );
private:
	idMenuWidget_DynamicList* 			options;
	idMenuDataSource_ControlSettings	controlData;
	idMenuWidget_Button*					btnBack;
};

//*
//================================================
//idMenuScreen_Shell_Controls
//================================================
//*/
class idMenuScreen_Shell_Gamepad : public idMenuScreen
{
public:

	/*
	================================================
	idMenuDataSource_ControlSettings
	================================================
	*/
	class idMenuDataSource_GamepadSettings : public idMenuDataSource
	{
	public:
		enum controlSettingFields_t
		{
			GAMEPAD_FIELD_LEFTY,
			GAMEPAD_FIELD_INVERT,
			GAMEPAD_FIELD_VIBRATE,
			GAMEPAD_FIELD_HOR_SENS,
			GAMEPAD_FIELD_VERT_SENS,
			GAMEPAD_FIELD_ACCELERATION,
			GAMEPAD_FIELD_THRESHOLD,
			MAX_GAMEPAD_FIELDS
		};

		idMenuDataSource_GamepadSettings();

		// loads data
		virtual void				LoadData();

		// submits data
		virtual void				CommitData();

		// says whether something changed with the data
		virtual bool				IsDataChanged() const;

		// retrieves a particular field for reading or updating
		virtual idSWFScriptVar		GetField( const int fieldIndex ) const
		{
			return fields[ fieldIndex ];
		}

		virtual void				AdjustField( const int fieldIndex, const int adjustAmount );

	private:
		idStaticList< idSWFScriptVar, MAX_GAMEPAD_FIELDS >	fields;
		idStaticList< idSWFScriptVar, MAX_GAMEPAD_FIELDS >	originalFields;
	};

	idMenuScreen_Shell_Gamepad() :
		options( NULL ),
		btnBack( NULL )
	{
	}
	virtual void				Initialize( idMenuHandler* data );
	virtual void				Update();
	virtual void				ShowScreen( const mainMenuTransition_t transitionType );
	virtual void				HideScreen( const mainMenuTransition_t transitionType );
	virtual bool				HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled = false );
private:
	idMenuWidget_DynamicList* 			options;
	idMenuDataSource_GamepadSettings	gamepadData;
	idMenuWidget_Button*					btnBack;
};

//*
//================================================
//idMenuScreen_Shell_ControllerLayout
//================================================
//*/
class idMenuScreen_Shell_ControllerLayout : public idMenuScreen
{
public:

	/*
	================================================
	idMenuDataSource_ControlSettings
	================================================
	*/
	class idMenuDataSource_LayoutSettings : public idMenuDataSource
	{
	public:
		enum controlSettingFields_t
		{
			LAYOUT_FIELD_LAYOUT,
			MAX_LAYOUT_FIELDS,
		};

		idMenuDataSource_LayoutSettings();

		// loads data
		virtual void				LoadData();

		// submits data
		virtual void				CommitData();

		// says whether something changed with the data
		virtual bool				IsDataChanged() const;

		// retrieves a particular field for reading or updating
		virtual idSWFScriptVar		GetField( const int fieldIndex ) const
		{
			return fields[ fieldIndex ];
		}

		virtual void				AdjustField( const int fieldIndex, const int adjustAmount );

	private:
		idStaticList< idSWFScriptVar, MAX_LAYOUT_FIELDS >	fields;
		idStaticList< idSWFScriptVar, MAX_LAYOUT_FIELDS >	originalFields;
	};

	idMenuScreen_Shell_ControllerLayout() :
		options( NULL ),
		btnBack( NULL )
	{
	}
	virtual void				Initialize( idMenuHandler* data );
	virtual void				Update();
	virtual void				ShowScreen( const mainMenuTransition_t transitionType );
	virtual void				HideScreen( const mainMenuTransition_t transitionType );
	virtual bool				HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled = false );

	void						UpdateBindingInfo();
private:

	idMenuDataSource_LayoutSettings		layoutData;
	idMenuWidget_DynamicList* 			options;
	idMenuWidget_Button*					btnBack;
};

//*
//================================================
//idMenuScreen_Shell_SystemOptions
//================================================
//*/
class idMenuScreen_Shell_SystemOptions : public idMenuScreen
{
public:

	/*
	================================================
	idMenuDataSource_SystemSettings
	================================================
	*/
	class idMenuDataSource_SystemSettings : public idMenuDataSource
	{
	public:
		enum systemSettingFields_t
		{
#ifdef _WIN32
			SYSTEM_FIELD_RENDERAPI, // RB: choose between DX12 and Vulkan on Windows
#endif
			SYSTEM_FIELD_FULLSCREEN,
			SYSTEM_FIELD_FRAMERATE,
			SYSTEM_FIELD_VSYNC,
			SYSTEM_FIELD_ANTIALIASING,
			// RB begin
			SYSTEM_FIELD_POSTFX,
			SYSTEM_FIELD_SSAO,
			SYSTEM_FIELD_AMBIENT_BRIGHTNESS,
			// RB end
			SYSTEM_FIELD_BRIGHTNESS,
			SYSTEM_FIELD_VOLUME,
			MAX_SYSTEM_FIELDS
		};

		idMenuDataSource_SystemSettings();

		// loads data
		virtual void				LoadData();

		// submits data
		virtual void				CommitData();

		// says whether something changed with the data
		virtual bool				IsDataChanged() const;

		// retrieves a particular field for reading
		virtual idSWFScriptVar		GetField( const int fieldIndex ) const;

		// updates a particular field value
		virtual void				AdjustField( const int fieldIndex, const int adjustAmount );

		bool						IsRestartRequired() const;

	private:
		idStr originalRenderAPI;
		int originalFramerate;
		int originalAntialias;
		int originalVsync;
		float originalBrightness;
		float originalVolume;
		// RB begin
		int originalShadowMapping;
		int originalSSAO;
		int originalPostProcessing;
		float originalAmbientBrightness;
		// RB end

		idList<vidMode_t>			modeList;
	};

	idMenuScreen_Shell_SystemOptions() :
		options( NULL ),
		btnBack( NULL )
	{
	}
	virtual void				Initialize( idMenuHandler* data );
	virtual void				Update();
	virtual void				ShowScreen( const mainMenuTransition_t transitionType );
	virtual void				HideScreen( const mainMenuTransition_t transitionType );
	virtual bool				HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled = false );

private:
	idMenuWidget_DynamicList* 	options;
	idMenuDataSource_SystemSettings	systemData;
	idMenuWidget_Button*			btnBack;

};

//*
//================================================
//idMenuScreen_Shell_Stereoscopics
//================================================
//*/
class idMenuScreen_Shell_Stereoscopics : public idMenuScreen
{
public:

	/*
	================================================
	idMenuDataSource_StereoSettings
	================================================
	*/
	class idMenuDataSource_StereoSettings : public idMenuDataSource
	{
	public:
		enum stereoSettingFields_t
		{
			STEREO_FIELD_ENABLE,
			STEREO_FIELD_SEPERATION,
			STEREO_FIELD_SWAP_EYES,
			MAX_STEREO_FIELDS
		};

		idMenuDataSource_StereoSettings();

		// loads data
		virtual void				LoadData();

		// submits data
		virtual void				CommitData();

		// says whether something changed with the data
		virtual bool				IsDataChanged() const;

		// retrieves a particular field for reading or updating
		virtual idSWFScriptVar		GetField( const int fieldIndex ) const;

		virtual void				AdjustField( const int fieldIndex, const int adjustAmount );

		bool						IsRestartRequired() const;

	private:
		idStaticList< idSWFScriptVar, MAX_STEREO_FIELDS >	fields;
		idStaticList< idSWFScriptVar, MAX_STEREO_FIELDS >	originalFields;
	};

	idMenuScreen_Shell_Stereoscopics() :
		options( NULL ),
		btnBack( NULL ),
		leftEyeMat( NULL ),
		rightEyeMat( NULL )
	{
	}
	virtual void				Initialize( idMenuHandler* data );
	virtual void				Update();
	virtual void				ShowScreen( const mainMenuTransition_t transitionType );
	virtual void				HideScreen( const mainMenuTransition_t transitionType );
	virtual bool				HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled = false );
private:
	idMenuWidget_DynamicList* 	options;
	idMenuDataSource_StereoSettings	stereoData;
	idMenuWidget_Button*			btnBack;
	const idMaterial* 			leftEyeMat;
	const idMaterial* 			rightEyeMat;
};

//*
//================================================
//idMenuScreen_Shell_PartyLobby
//================================================
//*/
class idMenuScreen_Shell_PartyLobby : public idMenuScreen
{
public:
	idMenuScreen_Shell_PartyLobby() :
		isHost( false ),
		isPeer( false ),
		inParty( false ),
		options( NULL ),
		lobby( NULL ),
		btnBack( NULL )
	{
	}

	virtual void				Initialize( idMenuHandler* data );
	virtual void				Update();
	virtual void				ShowScreen( const mainMenuTransition_t transitionType );
	virtual void				HideScreen( const mainMenuTransition_t transitionType );
	virtual bool				HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled = false );

	void						UpdateOptions();
	void						UpdateLobby();
	bool						CanKickSelectedPlayer( lobbyUserID_t& luid );
	void						ShowLeaderboards();

private:

	bool							isHost;
	bool							isPeer;
	bool							inParty;
	idMenuWidget_DynamicList* 		options;
	idMenuWidget_LobbyList* 		lobby;
	idMenuWidget_Button*				btnBack;
	idList< idList< idStr, TAG_IDLIB_LIST_MENU >, TAG_IDLIB_LIST_MENU > menuOptions;
};

//*
//================================================
//idMenuScreen_Shell_GameLobby
//================================================
//*/
class idMenuScreen_Shell_GameLobby : public idMenuScreen
{
public:
	idMenuScreen_Shell_GameLobby() :
		longCountdown( 0 ),
		longCountRemaining( 0 ),
		shortCountdown( 0 ),
		isHost( false ),
		isPeer( false ),
		privateGameLobby( true ),
		options( NULL ),
		lobby( NULL ),
		btnBack( NULL )
	{
	}

	virtual void				Initialize( idMenuHandler* data );
	virtual void				Update();
	virtual void				ShowScreen( const mainMenuTransition_t transitionType );
	virtual void				HideScreen( const mainMenuTransition_t transitionType );
	virtual bool				HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled = false );
	void						UpdateLobby();
	bool						CanKickSelectedPlayer( lobbyUserID_t& luid );

private:

	int								longCountdown;
	int								longCountRemaining;
	int								shortCountdown;

	bool							isHost;
	bool							isPeer;
	bool							privateGameLobby;

	idMenuWidget_DynamicList* 		options;
	idMenuWidget_LobbyList* 		lobby;
	idMenuWidget_Button*				btnBack;
	idList< idList< idStr, TAG_IDLIB_LIST_MENU >, TAG_IDLIB_LIST_MENU > menuOptions;
};

//*
//================================================
//idMenuScreen_HUD
//================================================
//*/
class idMenuScreen_HUD : public idMenuScreen
{
public:

	idMenuScreen_HUD() :
		weaponInfo( NULL ),
		playerInfo( NULL ),
		stamina( NULL ),
		weaponName( NULL ),
		weaponPills( NULL ),
		downloadPda( NULL ),
		downloadVideo( NULL ),
		tipInfo( NULL ),
		mpChat( NULL ),
		mpWeapons( NULL ),
		healthBorder( NULL ),
		healthPulse( NULL ),
		armorFrame( NULL ),
		security( NULL ),
		newPDADownload( NULL ),
		newVideoDownload( NULL ),
		newPDA( NULL ),
		newVideo( NULL ),
		audioLog( NULL ),
		communication( NULL ),
		oxygen( NULL ),
		objective( NULL ),
		objectiveComplete( NULL ),
		ammoInfo( NULL ),
		weaponImg( NULL ),
		newWeapon( NULL ),
		pickupInfo( NULL ),
		talkCursor( NULL ),
		combatCursor( NULL ),
		grabberCursor( NULL ),
		bsInfo( NULL ),
		soulcubeInfo( NULL ),
		newItem( NULL ),
		respawnMessage( NULL ),
		flashlight( NULL ),
		mpChatObject( NULL ),
		mpConnection( NULL ),
		mpInfo( NULL ),
		mpHitInfo( NULL ),
		locationName( NULL ),
		securityText( NULL ),
		newPDAName( NULL ),
		newPDAHeading( NULL ),
		newVideoHeading( NULL ),
		mpMessage( NULL ),
		mpTime( NULL ),
		audioLogPrevTime( 0 ),
		commPrevTime( 0 ),
		oxygenComm( false ),
		inVaccuum( false ),
		objScreenshot( NULL ),
		cursorState( CURSOR_NONE ),
		cursorInCombat( 0 ),
		cursorTalking( 0 ),
		cursorItem( 0 ),
		cursorGrabber( 0 ),
		cursorNone( 0 ),
		showSoulCubeInfoOnLoad( false )
	{
	}

	virtual void			Initialize( idMenuHandler* data );
	virtual void			Update();
	virtual void			ShowScreen( const mainMenuTransition_t transitionType );
	virtual void			HideScreen( const mainMenuTransition_t transitionType );

	void					UpdateHealthArmor( idPlayer* player );
	void					UpdateStamina( idPlayer* player );
	void					UpdateLocation( idPlayer* player );
	void					UpdateWeaponInfo( idPlayer* player );
	void					UpdateWeaponStates( idPlayer* player, bool weaponChanged );
	void					ShowTip( const char* title, const char* tip );
	void					HideTip();
	void					DownloadVideo();
	void					DownloadPDA( const idDeclPDA* pda, bool newSecurity );
	void					UpdatedSecurity();
	void					ToggleNewVideo( bool show );
	void					ClearNewPDAInfo();
	void					ToggleNewPDA( bool show );
	void					UpdateAudioLog( bool show );
	void					UpdateCommunication( bool show, idPlayer* player );
	void					UpdateOxygen( bool show, int val = 0 );
	void					SetupObjective( const idStr& title, const idStr& desc, const idMaterial* screenshot );
	void					SetupObjectiveComplete( const idStr& title );
	void					ShowObjective( bool complete );
	void					HideObjective( bool complete );
	void					GiveWeapon( idPlayer* player, int weaponIndex );
	void					UpdatePickupInfo( int index, const idStr& name );
	bool					IsPickupListReady();
	void					ShowPickups();
	void					SetCursorState( idPlayer* player, cursorState_t state, int set );
	void					SetCursorText( const idStr& action, const idStr& focus );
	void					UpdateCursorState();
	void					CombatCursorFlash();
	void					UpdateSoulCube( bool ready );
	void					ShowRespawnMessage( bool show );
	void					SetShowSoulCubeOnLoad( bool show )
	{
		showSoulCubeInfoOnLoad = show;
	}

	// MULTIPLAYER

	void					ToggleMPInfo( bool show, bool showTeams, bool isCTF = false );
	void					SetFlagState( int team, int state );
	void					SetTeamScore( int team, int score );
	void					SetTeam( int team );
	void					TriggerHitTarget( bool show, const idStr& target, int color = 0 );
	void					ToggleLagged( bool show );
	void					UpdateGameTime( const char* time );
	void					UpdateMessage( bool show, const idStr& message );
	void					ShowNewItem( const char* name, const char* icon );
	void					UpdateFlashlight( idPlayer* player );
	void					UpdateChattingHud( idPlayer* player );

private:

	idSWFScriptObject* 		weaponInfo;
	idSWFScriptObject* 		playerInfo;
	idSWFScriptObject* 		stamina;
	idSWFScriptObject* 		weaponName;
	idSWFScriptObject* 		weaponPills;
	idSWFScriptObject* 		downloadPda;
	idSWFScriptObject* 		downloadVideo;
	idSWFScriptObject* 		tipInfo;
	idSWFScriptObject* 		mpChat;
	idSWFScriptObject* 		mpWeapons;

	idSWFSpriteInstance* 	healthBorder;
	idSWFSpriteInstance* 	healthPulse;
	idSWFSpriteInstance* 	armorFrame;
	idSWFSpriteInstance* 	security;
	idSWFSpriteInstance* 	newPDADownload;
	idSWFSpriteInstance* 	newVideoDownload;
	idSWFSpriteInstance* 	newPDA;
	idSWFSpriteInstance* 	newVideo;
	idSWFSpriteInstance* 	audioLog;
	idSWFSpriteInstance* 	communication;
	idSWFSpriteInstance* 	oxygen;
	idSWFSpriteInstance* 	objective;
	idSWFSpriteInstance* 	objectiveComplete;
	idSWFSpriteInstance* 	ammoInfo;
	idSWFSpriteInstance* 	weaponImg;
	idSWFSpriteInstance* 	newWeapon;
	idSWFSpriteInstance* 	pickupInfo;
	idSWFSpriteInstance* 	talkCursor;
	idSWFSpriteInstance* 	combatCursor;
	idSWFSpriteInstance* 	grabberCursor;
	idSWFSpriteInstance* 	bsInfo;
	idSWFSpriteInstance* 	soulcubeInfo;
	idSWFSpriteInstance* 	newItem;
	idSWFSpriteInstance*		respawnMessage;
	idSWFSpriteInstance* 	flashlight;
	idSWFSpriteInstance* 	mpChatObject;
	idSWFSpriteInstance* 	mpConnection;

	idSWFSpriteInstance* 	mpInfo;
	idSWFSpriteInstance* 	mpHitInfo;

	idSWFTextInstance* 		locationName;
	idSWFTextInstance* 		securityText;
	idSWFTextInstance* 		newPDAName;
	idSWFTextInstance* 		newPDAHeading;
	idSWFTextInstance* 		newVideoHeading;

	idSWFTextInstance* 		mpMessage;
	idSWFTextInstance* 		mpTime;

	int						audioLogPrevTime;
	int						commPrevTime;

	bool					oxygenComm;
	bool					inVaccuum;

	idStr					objTitle;
	idStr					objDesc;
	const idMaterial* 		objScreenshot;
	idStr					objCompleteTitle;

	cursorState_t			cursorState;
	int						cursorInCombat;
	int						cursorTalking;
	int						cursorItem;
	int						cursorGrabber;
	int						cursorNone;
	idStr					cursorAction;
	idStr					cursorFocus;

	bool					showSoulCubeInfoOnLoad;
};

//*
//================================================
//idMenuScreen_Scoreboard
//================================================
//*/
class idMenuScreen_Scoreboard : public idMenuScreen
{
public:

	idMenuScreen_Scoreboard() :
		playerList( NULL )
	{

	}

	virtual void				Initialize( idMenuHandler* data );
	virtual void				Update();
	virtual void				ShowScreen( const mainMenuTransition_t transitionType );
	virtual bool				HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled = false );

	virtual void				SetPlayerData( idList< scoreboardInfo_t, TAG_IDLIB_LIST_MENU > data );
	virtual void				UpdateTeamScores( int r, int b );
	virtual void				UpdateGameInfo( idStr gameInfo );
	virtual void				UpdateSpectating( idStr spectating, idStr follow );
	virtual void				UpdateHighlight();

protected:
	idMenuWidget_ScoreboardList* 		playerList;

};

//*
//================================================
//idMenuScreen_Scoreboard_CTF
//================================================
//*/
class idMenuScreen_Scoreboard_CTF : public idMenuScreen_Scoreboard
{
public:
	virtual void				Initialize( idMenuHandler* data );
};

//*
//================================================
//idMenuScreen_Scoreboard_Team
//================================================
//*/
class idMenuScreen_Scoreboard_Team : public idMenuScreen_Scoreboard
{
public:
	virtual void				Initialize( idMenuHandler* data );
};


/*
========================
InvitePartyOrFriends

Invites the master local user's party, if he's in one and the party isn't in the lobby already.
Otherwise brings up the invite friends system menu.
========================
*/
inline void InvitePartyOrFriends()
{
	const idLocalUser* const user = session->GetSignInManager().GetMasterLocalUser();
	if( user != NULL && user->IsInParty() && user->GetPartyCount() > 1 && !session->IsPlatformPartyInLobby() )
	{
		session->InviteParty();
	}
	else
	{
		session->InviteFriends();
	}
}

#endif
