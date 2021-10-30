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
#ifndef __MENU_WIDGET_H__
#define __MENU_WIDGET_H__

class idMenuHandler;
class idMenuWidget;

enum menuOption_t
{
	OPTION_INVALID = -1,
	OPTION_BUTTON_TEXT,
	OPTION_SLIDER_BAR,
	OPTION_SLIDER_TEXT,
	OPTION_SLIDER_TOGGLE,
	OPTION_BUTTON_INFO,
	OPTION_BUTTON_FULL_TEXT_SLIDER,
	MAX_MENU_OPTION_TYPES
};

enum widgetEvent_t
{
	WIDGET_EVENT_PRESS,
	WIDGET_EVENT_RELEASE,
	WIDGET_EVENT_ROLL_OVER,
	WIDGET_EVENT_ROLL_OUT,
	WIDGET_EVENT_FOCUS_ON,
	WIDGET_EVENT_FOCUS_OFF,

	WIDGET_EVENT_SCROLL_UP_LSTICK,
	WIDGET_EVENT_SCROLL_UP_LSTICK_RELEASE,
	WIDGET_EVENT_SCROLL_DOWN_LSTICK,
	WIDGET_EVENT_SCROLL_DOWN_LSTICK_RELEASE,
	WIDGET_EVENT_SCROLL_LEFT_LSTICK,
	WIDGET_EVENT_SCROLL_LEFT_LSTICK_RELEASE,
	WIDGET_EVENT_SCROLL_RIGHT_LSTICK,
	WIDGET_EVENT_SCROLL_RIGHT_LSTICK_RELEASE,

	WIDGET_EVENT_SCROLL_UP_RSTICK,
	WIDGET_EVENT_SCROLL_UP_RSTICK_RELEASE,
	WIDGET_EVENT_SCROLL_DOWN_RSTICK,
	WIDGET_EVENT_SCROLL_DOWN_RSTICK_RELEASE,
	WIDGET_EVENT_SCROLL_LEFT_RSTICK,
	WIDGET_EVENT_SCROLL_LEFT_RSTICK_RELEASE,
	WIDGET_EVENT_SCROLL_RIGHT_RSTICK,
	WIDGET_EVENT_SCROLL_RIGHT_RSTICK_RELEASE,

	WIDGET_EVENT_SCROLL_UP,
	WIDGET_EVENT_SCROLL_UP_RELEASE,
	WIDGET_EVENT_SCROLL_DOWN,
	WIDGET_EVENT_SCROLL_DOWN_RELEASE,
	WIDGET_EVENT_SCROLL_LEFT,
	WIDGET_EVENT_SCROLL_LEFT_RELEASE,
	WIDGET_EVENT_SCROLL_RIGHT,
	WIDGET_EVENT_SCROLL_RIGHT_RELEASE,

	WIDGET_EVENT_DRAG_START,
	WIDGET_EVENT_DRAG_STOP,

	WIDGET_EVENT_SCROLL_PAGEDWN,
	WIDGET_EVENT_SCROLL_PAGEDWN_RELEASE,
	WIDGET_EVENT_SCROLL_PAGEUP,
	WIDGET_EVENT_SCROLL_PAGEUP_RELEASE,

	WIDGET_EVENT_SCROLL,
	WIDGET_EVENT_SCROLL_RELEASE,
	WIDGET_EVENT_BACK,
	WIDGET_EVENT_COMMAND,
	WIDGET_EVENT_TAB_NEXT,
	WIDGET_EVENT_TAB_PREV,
	MAX_WIDGET_EVENT
};

enum scrollType_t
{
	SCROLL_SINGLE,		// scroll a single unit
	SCROLL_PAGE,		// scroll a page
	SCROLL_FULL,		// scroll all the way to the end
	SCROLL_TOP,			// scroll to the first selection
	SCROLL_END,			// scroll to the last selection
};

enum widgetAction_t
{
	WIDGET_ACTION_NONE,
	WIDGET_ACTION_COMMAND,
	WIDGET_ACTION_FUNCTION,					// call the SWF function
	WIDGET_ACTION_SCROLL_VERTICAL,			// scroll something. takes one param = amount to scroll (can be negative)
	WIDGET_ACTION_SCROLL_VERTICAL_VARIABLE,
	WIDGET_ACTION_SCROLL_PAGE,
	WIDGET_ACTION_SCROLL_HORIZONTAL,		// scroll something. takes one param = amount to scroll (can be negative)
	WIDGET_ACTION_SCROLL_TAB,
	WIDGET_ACTION_START_REPEATER,
	WIDGET_ACTION_STOP_REPEATER,
	WIDGET_ACTION_ADJUST_FIELD,
	WIDGET_ACTION_PRESS_FOCUSED,
	WIDGET_ACTION_JOY3_ON_PRESS,
	WIDGET_ACTION_JOY4_ON_PRESS,
	//
	WIDGET_ACTION_GOTO_MENU,
	WIDGET_ACTION_GO_BACK,
	WIDGET_ACTION_EXIT_GAME,
	WIDGET_ACTION_LAUNCH_MULTIPLAYER,
	WIDGET_ACTION_MENU_BAR_SELECT,
	WIDGET_ACTION_EMAIL_HOVER,
	// PDA USER DATA ACTIONS
	WIDGET_ACTION_PDA_SELECT_USER,
	WIDGET_ACTION_SELECT_GAMERTAG,
	WIDGET_ACTION_PDA_SELECT_NAV,
	WIDGET_ACTION_SELECT_PDA_AUDIO,
	WIDGET_ACTION_SELECT_PDA_VIDEO,
	WIDGET_ACTION_SELECT_PDA_ITEM,
	WIDGET_ACTION_SCROLL_DRAG,
	// PDA EMAIL ACTIONS
	WIDGET_ACTION_PDA_SELECT_EMAIL,
	WIDGET_ACTION_PDA_CLOSE,
	WIDGET_ACTION_REFRESH,
	WIDGET_ACTION_MUTE_PLAYER,
	MAX_WIDGET_ACTION
};

enum actionHandler_t
{
	WIDGET_ACTION_EVENT_SCROLL_UP_START_REPEATER,
	WIDGET_ACTION_EVENT_SCROLL_UP_START_REPEATER_VARIABLE,
	WIDGET_ACTION_EVENT_SCROLL_DOWN_START_REPEATER,
	WIDGET_ACTION_EVENT_SCROLL_DOWN_START_REPEATER_VARIABLE,
	WIDGET_ACTION_EVENT_SCROLL_LEFT_START_REPEATER,
	WIDGET_ACTION_EVENT_SCROLL_RIGHT_START_REPEATER,
	WIDGET_ACTION_EVENT_SCROLL_PAGE_DOWN_START_REPEATER,
	WIDGET_ACTION_EVENT_SCROLL_PAGE_UP_START_REPEATER,
	WIDGET_ACTION_EVENT_STOP_REPEATER,
	WIDGET_ACTION_EVENT_TAB_NEXT,
	WIDGET_ACTION_EVENT_TAB_PREV,
	WIDGET_ACTION_EVENT_DRAG_START,
	WIDGET_ACTION_EVENT_DRAG_STOP,
	WIDGET_ACTION_EVENT_JOY3_ON_PRESS,
};

struct widgetTransition_t
{
	widgetTransition_t() :
		animationName( NULL )
	{

	}

	const char* 						animationName;			// name of the animation to run
	idStaticList< const char*, 4 >		prefixes;				// prefixes to try to use for animation
};


/*
================================================
scoreboardInfo_t
================================================
*/
struct scoreboardInfo_t
{
	scoreboardInfo_t() :
		index( -1 ),
		voiceState( VOICECHAT_DISPLAY_NONE )
	{
	}

	idList< idStr, TAG_IDLIB_LIST_MENU> values;
	int						index;
	voiceStateDisplay_t		voiceState;
};

/*
================================================
idSort_SavesByDate
================================================
*/
class idSort_SavesByDate : public idSort_Quick< idSaveGameDetails, idSort_SavesByDate >
{
public:
	int Compare( const idSaveGameDetails& a, const idSaveGameDetails& b ) const
	{
		return b.date - a.date;
	}
};

/*
================================================
idMenuDataSource
================================================
*/
class idMenuDataSource
{
public:
	virtual ~idMenuDataSource() { }

	virtual void				LoadData()							= 0;
	virtual void				CommitData()						= 0;
	virtual bool				IsDataChanged() const				= 0;
	virtual idSWFScriptVar		GetField( const int fieldIndex ) const	= 0;
	virtual void				AdjustField( const int fieldIndex, const int adjustAmount ) = 0;
};

/*
================================================
idWidgetEvent
================================================
*/
class idWidgetEvent
{
public:
	idWidgetEvent() :
		type( WIDGET_EVENT_PRESS ),
		arg( 0 ),
		thisObject( NULL )
	{

	}

	idWidgetEvent( const widgetEvent_t type_, const int arg_, idSWFScriptObject* thisObject_, const idSWFParmList& parms_ ) :
		type( type_ ),
		arg( arg_ ),
		thisObject( thisObject_ ),
		parms( parms_ )
	{
	}

	widgetEvent_t			type;
	int						arg;
	idSWFScriptObject* 		thisObject;
	idSWFParmList 			parms;
};

/*
================================================
idWidgetAction
================================================
*/
class idWidgetAction
{
public:
	idWidgetAction() :
		action( WIDGET_ACTION_NONE ),
		scriptFunction( NULL )
	{
	}

	idWidgetAction( const idWidgetAction& src )
	{
		action = src.action;
		parms = src.parms;
		scriptFunction = src.scriptFunction;
		if( scriptFunction != NULL )
		{
			scriptFunction->AddRef();
		}
	}

	~idWidgetAction()
	{
		if( scriptFunction != NULL )
		{
			scriptFunction->Release();
		}
	}

	void operator=( const idWidgetAction& src )
	{
		action = src.action;
		parms = src.parms;
		scriptFunction = src.scriptFunction;
		if( scriptFunction != NULL )
		{
			scriptFunction->AddRef();
		}
	}

	bool operator==( const idWidgetAction& otherAction ) const
	{
		if( GetType() != otherAction.GetType()
				|| GetParms().Num() != otherAction.GetParms().Num() )
		{
			return false;
		}

		// everything else is equal, so check all parms. NOTE: this assumes we are only sending
		// integral types.
		for( int i = 0; i < GetParms().Num(); ++i )
		{
			if( GetParms()[ i ].GetType() != otherAction.GetParms()[ i ].GetType()
					|| GetParms()[ i ].ToInteger() != otherAction.GetParms()[ i ].ToInteger() )
			{
				return false;
			}
		}

		return true;
	}

	void Set( idSWFScriptFunction* function )
	{
		action = WIDGET_ACTION_FUNCTION;
		if( scriptFunction != NULL )
		{
			scriptFunction->Release();
		}
		scriptFunction = function;
		scriptFunction->AddRef();
	}

	void Set( widgetAction_t action_ )
	{
		action = action_;
		parms.Clear();
	}

	void Set( widgetAction_t action_, const idSWFScriptVar& var1 )
	{
		action = action_;
		parms.Clear();
		parms.Append( var1 );
	}

	void Set( widgetAction_t action_, const idSWFScriptVar& var1, const idSWFScriptVar& var2 )
	{
		action = action_;
		parms.Clear();
		parms.Append( var1 );
		parms.Append( var2 );
	}

	void Set( widgetAction_t action_, const idSWFScriptVar& var1, const idSWFScriptVar& var2, const idSWFScriptVar& var3 )
	{
		action = action_;
		parms.Clear();
		parms.Append( var1 );
		parms.Append( var2 );
		parms.Append( var3 );
	}

	void Set( widgetAction_t action_, const idSWFScriptVar& var1, const idSWFScriptVar& var2, const idSWFScriptVar& var3, const idSWFScriptVar& var4 )
	{
		action = action_;
		parms.Clear();
		parms.Append( var1 );
		parms.Append( var2 );
		parms.Append( var3 );
		parms.Append( var4 );
	}

	idSWFScriptFunction* 	GetScriptFunction()
	{
		return scriptFunction;
	}
	const widgetAction_t	GetType() const
	{
		return action;
	}
	const idSWFParmList& 	GetParms() const
	{
		return parms;
	}

private:
	widgetAction_t			action;
	idSWFParmList			parms;
	idSWFScriptFunction* 	scriptFunction;
};

typedef idList< idMenuWidget*, TAG_IDLIB_LIST_MENU > idMenuWidgetList;

/*
================================================
idMenuWidget

We're using a model/view architecture, so this is the combination of both model and view.  The
other part of the view is the SWF itself.
================================================
*/
class idMenuWidget
{
public:

	/*
	================================================
	WrapWidgetSWFEvent
	================================================
	*/
	class WrapWidgetSWFEvent : public idSWFScriptFunction_RefCounted
	{
	public:
		WrapWidgetSWFEvent( idMenuWidget* widget, const widgetEvent_t event, const int eventArg ) :
			targetWidget( widget ),
			targetEvent( event ),
			targetEventArg( eventArg )
		{
		}

		idSWFScriptVar Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
		{
			targetWidget->ReceiveEvent( idWidgetEvent( targetEvent, targetEventArg, thisObject, parms ) );
			return idSWFScriptVar();
		}

	private:
		idMenuWidget* 	targetWidget;
		widgetEvent_t	targetEvent;
		int				targetEventArg;
	};


	enum widgetState_t
	{
		WIDGET_STATE_HIDDEN,	// hidden
		WIDGET_STATE_NORMAL,	// normal
		WIDGET_STATE_SELECTING,	// going into the selected state
		WIDGET_STATE_SELECTED,	// fully selected
		WIDGET_STATE_DISABLED,	// disabled
		WIDGET_STATE_MAX
	};

	idMenuWidget();

	virtual								~idMenuWidget();

	void								Cleanup();
	// typically this is where the allocations for a widget will occur: sub widgets, etc.
	// Note that SWF sprite objects may not be accessible at this point.
	virtual void						Initialize( idMenuHandler* data )
	{
		menuData = data;
	}

	// takes the information described in this widget and applies it to a given script object.
	// the script object should point to the root that you want to run from. Running this will
	// also create the sprite binding, if any.
	virtual void						Update() {}
	virtual void						Show();
	virtual void						Hide();

	widgetState_t						GetState() const
	{
		return widgetState;
	}
	void								SetState( const widgetState_t state );

	// actually binds the sprite. this must be called after setting sprite path!
	idSWFSpriteInstance* 				GetSprite()
	{
		return boundSprite;
	}
	idSWF* 								GetSWFObject();
	idMenuHandler* 						GetMenuData();
	bool								BindSprite( idSWFScriptObject& root );
	void								ClearSprite();

	void								SetSpritePath( const char* arg1, const char* arg2 = NULL, const char* arg3 = NULL, const char* arg4 = NULL, const char* arg5 = NULL );
	void								SetSpritePath( const idList< idStr >& spritePath_, const char* arg1 = NULL, const char* arg2 = NULL, const char* arg3 = NULL, const char* arg4 = NULL, const char* arg5 = NULL );
	idList< idStr, TAG_IDLIB_LIST_MENU >& 					GetSpritePath()
	{
		return spritePath;
	}
	int									GetRefCount() const
	{
		return refCount;
	}
	void						AddRef()
	{
		refCount++;
	}
	void						Release()
	{
		assert( refCount > 0 );
		if( --refCount == 0 && !noAutoFree )
		{
			delete this;
		}
	}

	//------------------------
	// Event Handling
	//------------------------
	virtual bool						HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled = false );
	virtual void						ObserveEvent( const idMenuWidget& widget, const idWidgetEvent& event ) { }
	void								SendEventToObservers( const idWidgetEvent& event );
	void								RegisterEventObserver( idMenuWidget* observer );
	void								ReceiveEvent( const idWidgetEvent& event );

	// Executes an event in the context of this widget.  Only rarely should this be called
	// directly.  Instead calls should go through ReceiveEvent which will propagate the event
	// through the standard focus order.
	virtual bool						ExecuteEvent( const idWidgetEvent& event );

	// returns the list of actions for a given event, or NULL if no actions are registered for
	// that event.  Events should not be directly added to the returned list.  Instead use
	// AddEventAction for adding new events.
	idList< idWidgetAction, TAG_IDLIB_LIST_MENU >* 			GetEventActions( const widgetEvent_t eventType );

	// allocates an action for the given event
	idWidgetAction& 					AddEventAction( const widgetEvent_t eventType );
	void								ClearEventActions();

	//------------------------
	// Data modeling
	//------------------------
	void								SetDataSource( idMenuDataSource* dataSource, const int fieldIndex );
	idMenuDataSource* 					GetDataSource()
	{
		return dataSource;
	}
	void								SetDataSourceFieldIndex( const int dataSourceFieldIndex_ )
	{
		dataSourceFieldIndex = dataSourceFieldIndex_;
	}
	int									GetDataSourceFieldIndex() const
	{
		return dataSourceFieldIndex;
	}

	idMenuWidget* 						GetFocus()
	{
		return ( focusIndex >= 0 && focusIndex < children.Num() ) ? children[ focusIndex ] : NULL;
	}
	int									GetFocusIndex() const
	{
		return focusIndex;
	}
	void								SetFocusIndex( const int index, bool skipSound = false );

	//------------------------
	// Hierarchy
	//------------------------
	idMenuWidgetList& 					GetChildren()
	{
		return children;
	}
	const idMenuWidgetList& 			GetChildren() const
	{
		return children;
	}
	idMenuWidget& 						GetChildByIndex( const int index ) const
	{
		return *children[ index ];
	}

	void								AddChild( idMenuWidget* widget );
	void								RemoveChild( idMenuWidget* widget );
	bool								HasChild( idMenuWidget* widget );
	void								RemoveAllChildren();

	idMenuWidget* 						GetParent()
	{
		return parent;
	}
	const idMenuWidget* 				GetParent() const
	{
		return parent;
	}
	void								SetParent( idMenuWidget* parent_ )
	{
		parent = parent_;
	}
	void								SetSWFObj( idSWF* obj )
	{
		swfObj = obj;
	}
	bool								GetHandlerIsParent()
	{
		return handlerIsParent;
	}
	void								SetHandlerIsParent( bool val )
	{
		handlerIsParent = val;
	}
	void								SetNoAutoFree( bool b )
	{
		noAutoFree = b;
	}

protected:
	void								ForceFocusIndex( const int index )
	{
		focusIndex = index;
	}

protected:
	bool								handlerIsParent;
	idMenuHandler* 						menuData;
	idSWF* 								swfObj;
	idSWFSpriteInstance* 				boundSprite;
	idMenuWidget* 						parent;
	idList< idStr, TAG_IDLIB_LIST_MENU >						spritePath;
	idMenuWidgetList											children;
	idMenuWidgetList											observers;

	static const int INVALID_ACTION_INDEX = -1;
	idList< idList< idWidgetAction, TAG_IDLIB_LIST_MENU >, TAG_IDLIB_LIST_MENU >		eventActions;
	idStaticList< int, MAX_WIDGET_EVENT >	eventActionLookup;

	idMenuDataSource* 					dataSource;
	int									dataSourceFieldIndex;

	int									focusIndex;

	widgetState_t						widgetState;
	int									refCount;
	bool								noAutoFree;
};

/*
================================================
idMenuWidget_Button

This might be better named "ComboButton", as it contains behavior for several controls along
with standard button behavior.
================================================
*/
class idMenuWidget_Button : public idMenuWidget
{
public:

	enum animState_t
	{
		ANIM_STATE_UP,			// standard
		ANIM_STATE_DOWN,		// pressed down
		ANIM_STATE_OVER,		// hovered over this
		ANIM_STATE_MAX
	};

	idMenuWidget_Button() :
		animState( ANIM_STATE_UP ),
		img( NULL ),
		ignoreColor( false )
	{
	}

	virtual ~idMenuWidget_Button() {}

	virtual bool			ExecuteEvent( const idWidgetEvent& event );
	virtual void			Update();

	//---------------
	// Model
	//---------------
	void					SetLabel( const idStr& label )
	{
		btnLabel = label;
	}
	const idStr& 			GetLabel() const
	{
		return btnLabel;
	}
	void					SetValues( idList< idStr >& list );
	const idStr& 			GetValue( int index ) const;
	void					SetImg( const idMaterial* val )
	{
		img = val;
	}
	const idMaterial* 		GetImg()
	{
		return img;
	}
	void					SetDescription( const char* desc_ )
	{
		description = desc_;
	}
	const idStr& 			GetDescription() const
	{
		return description;
	}

	void					SetIgnoreColor( const bool b )
	{
		ignoreColor = b;
	}

	animState_t				GetAnimState() const
	{
		return animState;
	}
	void					SetAnimState( const animState_t state )
	{
		animState = state;
	}
	void					SetOnPressFunction( idSWFScriptFunction* func )
	{
		scriptFunction = func;
	}

protected:
	void					SetupTransitionInfo( widgetTransition_t& trans, const widgetState_t buttonState, const animState_t sourceAnimState, const animState_t destAnimState ) const;
	void					AnimateToState( const animState_t targetState, const bool force = false );

	idList< idStr, TAG_IDLIB_LIST_MENU >			values;
	idStr					btnLabel;
	idStr					description;
	animState_t				animState;
	const idMaterial* 		img;
	idSWFScriptFunction* 	scriptFunction;
	bool					ignoreColor;
};

/*
================================================
idMenuWidget_LobbyButton
================================================
*/
class idMenuWidget_LobbyButton : public idMenuWidget_Button
{
public:
	idMenuWidget_LobbyButton() :
		voiceState( VOICECHAT_DISPLAY_NONE )
	{
	}

	virtual void			Update();
	void					SetButtonInfo( idStr name_, voiceStateDisplay_t voiceState_ );
	bool					IsValid()
	{
		return !name.IsEmpty();
	}

protected:
	idStr					name;
	voiceStateDisplay_t		voiceState;
};

/*
================================================
idMenuWidget_ScoreboardButton
================================================
*/
class idMenuWidget_ScoreboardButton : public idMenuWidget_Button
{
public:
	idMenuWidget_ScoreboardButton() :
		voiceState( VOICECHAT_DISPLAY_NONE ),
		index( -1 )
	{
	}

	virtual void			Update();
	void					SetButtonInfo( int index_, idList< idStr >& list, voiceStateDisplay_t voiceState_ );

protected:
	voiceStateDisplay_t		voiceState;
	int						index;
};

/*
================================================
idMenuWidget_ControlButton
================================================
*/
class idMenuWidget_ControlButton : public idMenuWidget_Button
{
public:
	idMenuWidget_ControlButton() :
		optionType( OPTION_BUTTON_TEXT ),
		disabled( false )
	{
	}

	virtual void			Update();
	void					SetOptionType( const menuOption_t type )
	{
		optionType = type;
	}
	menuOption_t			GetOptionType() const
	{
		return optionType;
	}
	void					SetupEvents( int delay, int index );
	void					SetDisabled( bool disable )
	{
		disabled = disable;
	}

protected:
	menuOption_t			optionType;
	bool					disabled;
};

/*
================================================
idMenuWidget_ServerButton
================================================
*/
class idMenuWidget_ServerButton : public idMenuWidget_Button
{
public:

	idMenuWidget_ServerButton() :
		index( 0 ),
		players( 0 ),
		maxPlayers( 0 ),
		joinable( false ),
		validMap( false )
	{
	}

	virtual void			Update();
	void					SetButtonInfo( idStr name_, idStrId mapName_, idStr modeName_, int index_ = 0, int players_ = 0, int maxPlayers_ = 0, bool joinable_ = false, bool validMap_ = false );
	bool					IsValid()
	{
		return !serverName.IsEmpty();
	}
	bool					CanJoin()
	{
		return ( joinable && validMap );
	}

protected:
	idStr					serverName;
	int						index;
	int						players;
	int						maxPlayers;
	bool					joinable;
	bool					validMap;
	idStrId					mapName;
	idStr					modeName;
};

/*
================================================
idMenuWidget_NavButton
================================================
*/
class idMenuWidget_NavButton : public idMenuWidget_Button
{
public:

	enum navWidgetState_t
	{
		NAV_WIDGET_LEFT,		// option on left side
		NAV_WIDGET_RIGHT,		// option on right side
		NAV_WIDGET_SELECTED		// option is selected
	};

	idMenuWidget_NavButton() :
		navIndex( 0 ),
		xPos( 0 )
	{
	}

	virtual bool			ExecuteEvent( const idWidgetEvent& event );
	virtual void			Update();

	void					SetNavIndex( int i, const navWidgetState_t type )
	{
		navIndex = i;
		navState = type;
	}
	void					SetPosition( float pos )
	{
		xPos = pos;
	}

private:

	int						navIndex;
	float					xPos;
	navWidgetState_t		navState;

};

/*
================================================
idMenuWidget_NavButton
================================================
*/
class idMenuWidget_MenuButton : public idMenuWidget_Button
{
public:

	idMenuWidget_MenuButton() :
		xPos( 0 )
	{
	}

	virtual void			Update();
	void					SetPosition( float pos )
	{
		xPos = pos;
	}

private:

	float					xPos;

};

/*
================================================
idMenuWidget_List

Stores a list of widgets but displays only a segment of them at a time.
================================================
*/
class idMenuWidget_List : public idMenuWidget
{
public:
	idMenuWidget_List() :
		numVisibleOptions( 0 ),
		viewOffset( 0 ),
		viewIndex( 0 ),
		allowWrapping( false )
	{

	}

	virtual void				Update();
	virtual bool				HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled = false );
	virtual void				ObserveEvent( const idMenuWidget& widget, const idWidgetEvent& event );
	virtual void				Scroll( const int scrollIndexAmount, const bool wrapAround = false );
	virtual void				ScrollOffset( const int scrollIndexAmount );
	virtual int					GetTotalNumberOfOptions() const
	{
		return GetChildren().Num();
	}
	virtual bool				PrepareListElement( idMenuWidget& widget, const int childIndex )
	{
		return true;
	}

	bool						IsWrappingAllowed() const
	{
		return allowWrapping;
	}
	void						SetWrappingAllowed( const bool allow )
	{
		allowWrapping = allow;
	}

	void						SetNumVisibleOptions( const int numVisibleOptions_ )
	{
		numVisibleOptions = numVisibleOptions_;
	}
	int							GetNumVisibleOptions() const
	{
		return numVisibleOptions;
	}

	int							GetViewOffset() const
	{
		return viewOffset;
	}
	void						SetViewOffset( const int offset )
	{
		viewOffset = offset;
	}

	int							GetViewIndex() const
	{
		return viewIndex;
	}
	void						SetViewIndex( const int index )
	{
		viewIndex = index;
	}

	void						CalculatePositionFromIndexDelta( int& outIndex, int& outOffset, const int currentIndex, const int currentOffset, const int windowSize, const int maxSize, const int indexDelta, const bool allowWrapping, const bool wrapAround = false ) const;
	void						CalculatePositionFromOffsetDelta( int& outIndex, int& outOffset, const int currentIndex, const int currentOffset, const int windowSize, const int maxSize, const int offsetDelta ) const;

private:
	int							numVisibleOptions;
	int							viewOffset;
	int							viewIndex;
	bool						allowWrapping;
};

class idBrowserEntry_t
{
public:
	idStr serverName;
	int	index;
	int players;
	int maxPlayers;
	bool joinable;
	bool validMap;
	idStrId mapName;
	idStr modeName;
};

/*
================================================
idMenuWidget_GameBrowserList
================================================
*/
class idMenuWidget_GameBrowserList : public idMenuWidget_List
{
public:
	virtual void				Update();
	virtual bool				PrepareListElement( idMenuWidget& widget, const int childIndex );
	virtual int					GetTotalNumberOfOptions() const;
	void						ClearGames();
	void						AddGame( idStr name_, idStrId mapName_, idStr modeName_, int index_ = 0, int players_ = 0, int maxPlayers_ = 0, bool joinable_ = false, bool validMap_ = false );
	int							GetServerIndex();
private:
	idList< idBrowserEntry_t >	games;
};

/*
================================================
idMenuWidget_Carousel
Displays a list of items in a looping carousel pattern
================================================
*/
class idMenuWidget_Carousel : public idMenuWidget
{
public:

	idMenuWidget_Carousel() :
		numVisibleOptions( 0 ),
		viewIndex( 0 ),
		moveToIndex( 0 ),
		moveDiff( 0 ),
		fastScroll( false ),
		scrollLeft( false )
	{
	}

	virtual void				Initialize( idMenuHandler* data );
	virtual void				Update();
	virtual bool				HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled = false );
	virtual int					GetTotalNumberOfOptions() const
	{
		return imgList.Num();
	}
	virtual bool				PrepareListElement( idMenuWidget& widget, const int childIndex )
	{
		return true;
	}

	void						SetNumVisibleOptions( const int numVisibleOptions_ )
	{
		numVisibleOptions = numVisibleOptions_;
	}
	int							GetNumVisibleOptions() const
	{
		return numVisibleOptions;
	}

	void						MoveToIndex( int index, bool instant = false );
	void						MoveToFirstItem( bool instant = true );
	void						MoveToLastItem( bool instant = true );
	int							GetMoveToIndex()
	{
		return moveToIndex;
	}
	void						SetMoveToIndex( int index )
	{
		moveToIndex = index;
	}
	void						SetViewIndex( int index )
	{
		viewIndex = index;
	}
	int							GetViewIndex() const
	{
		return viewIndex;
	}
	void						SetListImages( idList<const idMaterial*>& list );
	void						SetMoveDiff( int val )
	{
		moveDiff = val;
	}
	int							GetMoveDiff()
	{
		return moveDiff;
	}
	bool						GetScrollLeft()
	{
		return scrollLeft;
	}

private:

	int							numVisibleOptions;
	int							viewIndex;
	int							moveToIndex;
	int							moveDiff;
	bool						fastScroll;
	bool						scrollLeft;
	idList<const idMaterial*>	imgList;

};

/*
================================================
idMenuWidget_Help

Knows how to display help tooltips by observing events from other widgets
================================================
*/
class idMenuWidget_Help : public idMenuWidget
{
public:
	virtual void Update();
	virtual void ObserveEvent( const idMenuWidget& widget, const idWidgetEvent& event );

private:
	idStr		lastFocusedMessage;		// message from last widget that had focus
	idStr		lastHoveredMessage;		// message from last widget that was hovered over
	bool		hideMessage;
};

/*
================================================
idMenuWidget_CommandBar
================================================
*/
class idMenuWidget_CommandBar : public idMenuWidget
{
public:
	enum button_t
	{
		BUTTON_JOY1,
		BUTTON_JOY2,
		BUTTON_JOY3,
		BUTTON_JOY4,
		BUTTON_JOY10,
		BUTTON_TAB,
		MAX_BUTTONS
	};

	enum alignment_t
	{
		LEFT,
		RIGHT
	};

	struct buttonInfo_t
	{
		idStr			label;			// empty labels are treated as hidden buttons
		idWidgetAction	action;
	};

	idMenuWidget_CommandBar() :
		alignment( LEFT )
	{

		buttons.SetNum( MAX_BUTTONS );
	}

	virtual void		Update();
	virtual bool		ExecuteEvent( const idWidgetEvent& event );

	buttonInfo_t* 		GetButton( const button_t button )
	{
		return &buttons[ button ];
	}
	void				ClearAllButtons();

	alignment_t			GetAlignment() const
	{
		return alignment;
	}
	void				SetAlignment( const alignment_t alignment_ )
	{
		alignment = alignment_;
	}

private:
	idStaticList< buttonInfo_t, MAX_BUTTONS >	buttons;
	alignment_t									alignment;
};

/*
================================================
	idMenuWidget_DynamicList
================================================
*/
class idMenuWidget_LobbyList : public idMenuWidget_List
{
public:
	idMenuWidget_LobbyList() :
		numEntries( 0 )
	{
	}

	virtual void				Update();
	virtual bool				PrepareListElement( idMenuWidget& widget, const int childIndex );
	virtual int					GetTotalNumberOfOptions() const
	{
		return numEntries;
	}
	void						SetEntryData( int index, idStr name, voiceStateDisplay_t voiceState );
	void						SetHeadingInfo( idList< idStr >& list );
	void						SetNumEntries( int num )
	{
		numEntries = num;
	}
	int							GetNumEntries()
	{
		return numEntries;
	}
	void						SetRefreshFunction( const char* func );
private:
	idList< idStr, TAG_IDLIB_LIST_MENU >	headings;
	int							numEntries;
};

/*
================================================
idMenuWidget_DynamicList
================================================
*/
class idMenuWidget_DynamicList : public idMenuWidget_List
{
public:

	idMenuWidget_DynamicList() :
		controlList( false ),
		ignoreColor( false )
	{
	}

	virtual void				Update();
	virtual void				Initialize( idMenuHandler* data );
	virtual int					GetTotalNumberOfOptions() const;
	virtual bool				PrepareListElement( idMenuWidget& widget, const int childIndex );

	virtual void				Recalculate();
	virtual void				SetListData( idList< idList< idStr, TAG_IDLIB_LIST_MENU >, TAG_IDLIB_LIST_MENU >& list );

	void						SetControlList( bool val )
	{
		controlList = val;
	}
	void						SetIgnoreColor( bool val )
	{
		ignoreColor = val;
	}

protected:
	idList< idList< idStr, TAG_IDLIB_LIST_MENU >, TAG_IDLIB_LIST_MENU >	listItemInfo;
	bool						controlList;
	bool						ignoreColor;
};

/*
================================================
idMenuWidget_ScoreboardList
================================================
*/
class idMenuWidget_ScoreboardList : public idMenuWidget_DynamicList
{
public:
	virtual void				Update();
	virtual int					GetTotalNumberOfOptions() const;
};

// RB begin
class idMenuWidget_SystemOptionsList : public idMenuWidget_DynamicList
{
public:
	virtual void				Update() override;
	virtual void				Scroll( const int scrollAmount, const bool wrapAround = false ) override;
};
// RB end


/*
================================================
idMenuWidget_NavBar

The nav bar is set up with the main option being at the safe frame line.
================================================
*/
class idMenuWidget_NavBar : public idMenuWidget_DynamicList
{
public:

	idMenuWidget_NavBar() :
		initialPos( 0.0f ),
		buttonPos( 0.0f ),
		leftSpacer( 0.0f ),
		rightSpacer( 0.0f ),
		selectedSpacer( 0.0f )
	{
	}

	virtual void				Update();
	virtual void				Initialize( idMenuHandler* data );
	virtual void				SetInitialXPos( float pos )
	{
		initialPos = pos;
	}
	virtual void				SetButtonSpacing( float lSpace, float rSpace, float sSpace )
	{
		leftSpacer = lSpace;
		rightSpacer = rSpace;
		selectedSpacer = sSpace;
	}
	virtual bool				PrepareListElement( idMenuWidget& widget, const int navIndex );
	virtual void				SetListHeadings( idList< idStr >& list );
	virtual int					GetTotalNumberOfOptions() const;

private:

	idList< idStr, TAG_IDLIB_LIST_MENU >		headings;
	float				initialPos;
	float				buttonPos;
	float				leftSpacer;
	float				rightSpacer;
	float				selectedSpacer;

};

/*
================================================
idMenuWidget_NavBar

The nav bar is set up with the main option being at the safe frame line.
================================================
*/
class idMenuWidget_MenuBar : public idMenuWidget_DynamicList
{
public:

	idMenuWidget_MenuBar() :
		totalWidth( 0.0f ),
		buttonPos( 0.0f ),
		rightSpacer( 0.0f )
	{
	}

	virtual void				Update();
	virtual void				Initialize( idMenuHandler* data );
	virtual void				SetButtonSpacing( float rSpace )
	{
		rightSpacer = rSpace;
	}
	virtual bool				PrepareListElement( idMenuWidget& widget, const int navIndex );
	virtual void				SetListHeadings( idList< idStr >& list );
	virtual int					GetTotalNumberOfOptions() const;

private:

	idList< idStr, TAG_IDLIB_LIST_MENU >				headings;
	float						totalWidth;
	float						buttonPos;
	float						rightSpacer;

};

/*
================================================
idMenuWidget_PDA_UserData
================================================
*/
class idMenuWidget_PDA_UserData : public idMenuWidget
{
public:
	idMenuWidget_PDA_UserData() :
		pdaIndex( 0 )
	{
	}
	virtual ~idMenuWidget_PDA_UserData() {}
	virtual void	Update();
	virtual void	ObserveEvent( const idMenuWidget& widget, const idWidgetEvent& event );

private:
	int		pdaIndex;
};

/*
================================================
idMenuWidget_DynamicList
================================================
*/
class idMenuWidget_ScrollBar : public idMenuWidget
{
public:
	idMenuWidget_ScrollBar() :
		yTop( 0.0f ),
		yBot( 0.0f ),
		dragging( false )
	{
	}

	virtual void	Initialize( idMenuHandler* data );
	virtual void	Update();
	virtual bool	HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled = false );
	virtual void	ObserveEvent( const idMenuWidget& widget, const idWidgetEvent& event );

	void			CalcTopAndBottom();
	void			CalculatePosition( float x, float y );

	float			yTop;
	float			yBot;
	bool			dragging;
};

/*
================================================
idMenuWidget_InfoBox
================================================
*/
class idMenuWidget_InfoBox: public idMenuWidget
{
public:
	idMenuWidget_InfoBox() :
		scrollbar( NULL )
	{
	}

	virtual void	Initialize( idMenuHandler* data );
	virtual void	Update();
	virtual bool	HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled = false );
	virtual void	ObserveEvent( const idMenuWidget& widget, const idWidgetEvent& event );
	void			SetHeading( idStr val )
	{
		heading = val;
	}
	void			SetBody( idStr val )
	{
		info = val;
	}
	void			ResetInfoScroll();
	void			Scroll( int d );
	int				GetScroll();
	int				GetMaxScroll();
	void			SetScroll( int scroll );
	void			SetScrollbar( idMenuWidget_ScrollBar* bar );
private:
	idMenuWidget_ScrollBar* 	scrollbar;
	idStr						heading;
	idStr						info;
};

/*
================================================
idMenuWidget_PDA_Objective
================================================
*/
class idMenuWidget_PDA_Objective: public idMenuWidget
{
public:
	idMenuWidget_PDA_Objective() :
		pdaIndex( 0 )
	{
	}
	virtual void	Update();
	virtual void	ObserveEvent( const idMenuWidget& widget, const idWidgetEvent& event );
private:
	int		pdaIndex;
};

/*
================================================
idMenuWidget_Shell_SaveInfo
================================================
*/
class idMenuWidget_Shell_SaveInfo: public idMenuWidget
{
public:
	idMenuWidget_Shell_SaveInfo() :
		loadIndex( 0 ),
		forSaveScreen( false )
	{
	}
	virtual void	Update();
	virtual void	ObserveEvent( const idMenuWidget& widget, const idWidgetEvent& event );

	void			SetForSaveScreen( bool val )
	{
		forSaveScreen = val;
	}
private:
	int		loadIndex;
	bool	forSaveScreen;
};

/*
================================================
idMenuWidget_PDA_AudioFiles
================================================
*/
class idMenuWidget_PDA_AudioFiles: public idMenuWidget
{
public:
	idMenuWidget_PDA_AudioFiles() :
		pdaIndex( 0 )
	{
	}
	virtual ~idMenuWidget_PDA_AudioFiles();
	virtual void	Update();
	virtual void	Initialize( idMenuHandler* data );
	virtual void	ObserveEvent( const idMenuWidget& widget, const idWidgetEvent& event );
private:
	int								pdaIndex;
	idList< idList< idStr, TAG_IDLIB_LIST_MENU >, TAG_IDLIB_LIST_MENU >		audioFileNames;
};

/*
================================================
idMenuWidget_PDA_AudioFiles
================================================
*/
class idMenuWidget_PDA_EmailInbox: public idMenuWidget
{
public:
	idMenuWidget_PDA_EmailInbox() :
		emailList( NULL ),
		scrollbar( NULL ),
		pdaIndex( 0 )
	{
	}
	virtual void	Update();
	virtual void	Initialize( idMenuHandler* data );
	virtual void	ObserveEvent( const idMenuWidget& widget, const idWidgetEvent& event );

	idMenuWidget_DynamicList* 	GetEmailList()
	{
		return emailList;
	}
	idMenuWidget_ScrollBar* 	GetScrollbar()
	{
		return scrollbar;
	}
private:
	idMenuWidget_DynamicList* 		emailList;
	idMenuWidget_ScrollBar* 		scrollbar;
	int								pdaIndex;
	idList< idList< idStr, TAG_IDLIB_LIST_MENU >, TAG_IDLIB_LIST_MENU >		emailInfo;
};

/*
================================================
	idMenuWidget_PDA_AudioFiles
================================================
*/
class idMenuWidget_ItemAssignment: public idMenuWidget
{
public:
	idMenuWidget_ItemAssignment() :
		slotIndex( 0 )
	{
	}

	virtual void	Update();
	void			SetIcon( int index, const idMaterial* icon );
	void			FindFreeSpot();
	int				GetSlotIndex()
	{
		return slotIndex;
	}
	void			SetSlotIndex( int num )
	{
		slotIndex = num;
	}
private:
	const idMaterial* images[ NUM_QUICK_SLOTS ];
	int				slotIndex;

};


/*
================================================
idMenuWidget_PDA_AudioFiles
================================================
*/
class idMenuWidget_PDA_VideoInfo: public idMenuWidget
{
public:
	virtual void	Update();
	virtual void	ObserveEvent( const idMenuWidget& widget, const idWidgetEvent& event );
private:
	int				videoIndex;
};


/*
================================================
idWidgetActionHandler
================================================
*/
class idWidgetActionHandler : public idSWFScriptFunction_RefCounted
{
public:
	idWidgetActionHandler( idMenuWidget* widget, actionHandler_t actionEventType, widgetEvent_t _event ) :
		targetWidget( widget ),
		type( actionEventType ),
		targetEvent( _event )
	{

	}

	idSWFScriptVar Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
	{

		idWidgetAction action;
		bool handled = false;
		switch( type )
		{
			case WIDGET_ACTION_EVENT_SCROLL_DOWN_START_REPEATER:
			{
				action.Set( ( widgetAction_t )WIDGET_ACTION_START_REPEATER, WIDGET_ACTION_SCROLL_VERTICAL, 1 );
				handled = true;
				break;
			}
			case WIDGET_ACTION_EVENT_SCROLL_UP_START_REPEATER:
			{
				action.Set( ( widgetAction_t )WIDGET_ACTION_START_REPEATER, WIDGET_ACTION_SCROLL_VERTICAL, -1 );
				handled = true;
				break;
			}
			case WIDGET_ACTION_EVENT_SCROLL_DOWN_START_REPEATER_VARIABLE:
			{
				action.Set( ( widgetAction_t )WIDGET_ACTION_START_REPEATER, WIDGET_ACTION_SCROLL_VERTICAL_VARIABLE, 1 );
				handled = true;
				break;
			}
			case WIDGET_ACTION_EVENT_SCROLL_UP_START_REPEATER_VARIABLE:
			{
				action.Set( ( widgetAction_t )WIDGET_ACTION_START_REPEATER, WIDGET_ACTION_SCROLL_VERTICAL_VARIABLE, -1 );
				handled = true;
				break;
			}
			case WIDGET_ACTION_EVENT_SCROLL_PAGE_DOWN_START_REPEATER:
			{
				action.Set( ( widgetAction_t )WIDGET_ACTION_START_REPEATER, WIDGET_ACTION_SCROLL_PAGE, 1 );
				handled = true;
				break;
			}
			case WIDGET_ACTION_EVENT_SCROLL_PAGE_UP_START_REPEATER:
			{
				action.Set( ( widgetAction_t )WIDGET_ACTION_START_REPEATER, WIDGET_ACTION_SCROLL_PAGE, -1 );
				handled = true;
				break;
			}
			case WIDGET_ACTION_EVENT_STOP_REPEATER:
			{
				action.Set( ( widgetAction_t )WIDGET_ACTION_STOP_REPEATER );
				handled = true;
				break;
			}
			case WIDGET_ACTION_EVENT_TAB_NEXT:
			{
				action.Set( ( widgetAction_t )WIDGET_ACTION_SCROLL_TAB, 1 );
				handled = true;
				break;
			}
			case WIDGET_ACTION_EVENT_TAB_PREV:
			{
				action.Set( ( widgetAction_t )WIDGET_ACTION_SCROLL_TAB, -1 );
				handled = true;
				break;
			}
			case WIDGET_ACTION_EVENT_JOY3_ON_PRESS:
			{
				action.Set( ( widgetAction_t )WIDGET_ACTION_JOY3_ON_PRESS );
				handled = true;
				break;
			}
			case WIDGET_ACTION_EVENT_SCROLL_LEFT_START_REPEATER:
			{
				action.Set( ( widgetAction_t )WIDGET_ACTION_START_REPEATER, WIDGET_ACTION_SCROLL_HORIZONTAL, -1 );
				handled = true;
				break;
			}
			case WIDGET_ACTION_EVENT_SCROLL_RIGHT_START_REPEATER:
			{
				action.Set( ( widgetAction_t )WIDGET_ACTION_START_REPEATER, WIDGET_ACTION_SCROLL_HORIZONTAL, 1 );
				handled = true;
				break;
			}
			case WIDGET_ACTION_EVENT_DRAG_START:
			{
				action.Set( ( widgetAction_t )WIDGET_ACTION_SCROLL_DRAG );
				handled = true;
				break;
			}
			case WIDGET_ACTION_EVENT_DRAG_STOP:
			{
				action.Set( ( widgetAction_t )WIDGET_ACTION_EVENT_DRAG_STOP );
				handled = true;
				break;
			}
		}

		if( handled )
		{
			targetWidget->HandleAction( action, idWidgetEvent( targetEvent, 0, thisObject, parms ), targetWidget );
		}

		return idSWFScriptVar();
	}

private:
	idMenuWidget* 	targetWidget;
	actionHandler_t type;
	widgetEvent_t targetEvent;
};

#endif
