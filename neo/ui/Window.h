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

#ifndef __WINDOW_H__
#define __WINDOW_H__

#include "Rectangle.h"
#include "DeviceContext.h"
#include "RegExp.h"
#include "Winvar.h"
#include "GuiScript.h"
#include "SimpleWindow.h"

const int WIN_CHILD			= 0x00000001;
const int WIN_CAPTION		= 0x00000002;
const int WIN_BORDER		= 0x00000004;
const int WIN_SIZABLE		= 0x00000008;
const int WIN_MOVABLE		= 0x00000010;
const int WIN_FOCUS			= 0x00000020;
const int WIN_CAPTURE		= 0x00000040;
const int WIN_HCENTER		= 0x00000080;
const int WIN_VCENTER		= 0x00000100;
const int WIN_MODAL			= 0x00000200;
const int WIN_INTRANSITION	= 0x00000400;
const int WIN_CANFOCUS		= 0x00000800;
const int WIN_SELECTED		= 0x00001000;
const int WIN_TRANSFORM		= 0x00002000;
const int WIN_HOLDCAPTURE	= 0x00004000;
const int WIN_NOWRAP		= 0x00008000;
const int WIN_NOCLIP		= 0x00010000;
const int WIN_INVERTRECT	= 0x00020000;
const int WIN_NATURALMAT	= 0x00040000;
const int WIN_NOCURSOR		= 0x00080000;
const int WIN_MENUGUI		= 0x00100000;
const int WIN_ACTIVE		= 0x00200000;
const int WIN_SHOWCOORDS	= 0x00400000;
const int WIN_SHOWTIME		= 0x00800000;
const int WIN_WANTENTER		= 0x01000000;

const int WIN_DESKTOP		= 0x10000000;

const char CAPTION_HEIGHT[] = "16.0";
const char SCROLLER_SIZE[] = "16.0";
const int SCROLLBAR_SIZE = 16;

const int MAX_WINDOW_NAME = 32;
const int MAX_LIST_ITEMS = 1024;

const char DEFAULT_BACKCOLOR[] = "1 1 1 1";
const char DEFAULT_FORECOLOR[] = "0 0 0 1";
const char DEFAULT_BORDERCOLOR[] = "0 0 0 1";
const char DEFAULT_TEXTSCALE[] = "0.4";

typedef enum
{
	WOP_TYPE_ADD,
	WOP_TYPE_SUBTRACT,
	WOP_TYPE_MULTIPLY,
	WOP_TYPE_DIVIDE,
	WOP_TYPE_MOD,
	WOP_TYPE_TABLE,
	WOP_TYPE_GT,
	WOP_TYPE_GE,
	WOP_TYPE_LT,
	WOP_TYPE_LE,
	WOP_TYPE_EQ,
	WOP_TYPE_NE,
	WOP_TYPE_AND,
	WOP_TYPE_OR,
	WOP_TYPE_VAR,
	WOP_TYPE_VARS,
	WOP_TYPE_VARF,
	WOP_TYPE_VARI,
	WOP_TYPE_VARB,
	WOP_TYPE_COND
} wexpOpType_t;

typedef enum
{
	WEXP_REG_TIME,
	WEXP_REG_NUM_PREDEFINED
} wexpRegister_t;

typedef struct
{
	wexpOpType_t opType;
	// RB: 64 bit fixes, changed int to intptr_t
	intptr_t	a, b, c, d;
	// RB end
} wexpOp_t;

struct idRegEntry
{
	const char* name;
	idRegister::REGTYPE type;
	int index;
};


class rvGEWindowWrapper;
class idWindow;

struct idTimeLineEvent
{
	idTimeLineEvent()
	{
		event = new( TAG_OLD_UI ) idGuiScriptList;
	}
	~idTimeLineEvent()
	{
		delete event;
	}
	int time;
	idGuiScriptList* event;
	bool pending;
	size_t Size()
	{
		return sizeof( *this ) + event->Size();
	}
};

class rvNamedEvent
{
public:

	rvNamedEvent( const char* name )
	{
		mEvent = new( TAG_OLD_UI ) idGuiScriptList;
		mName  = name;
	}
	~rvNamedEvent()
	{
		delete mEvent;
	}
	size_t Size()
	{
		return sizeof( *this ) + mEvent->Size();
	}

	idStr				mName;
	idGuiScriptList*	mEvent;
};

struct idTransitionData
{
	idWinVar* data;
	int	offset;
	idInterpolateAccelDecelLinear<idVec4> interp;
};


class idUserInterfaceLocal;
class idWindow
{
public:
	idWindow( idUserInterfaceLocal* gui );
	virtual ~idWindow();

	enum
	{
		ON_MOUSEENTER = 0,
		ON_MOUSEEXIT,
		ON_ACTION,
		ON_ACTIVATE,
		ON_DEACTIVATE,
		ON_ESC,
		ON_FRAME,
		ON_TRIGGER,
		ON_ACTIONRELEASE,
		ON_ENTER,
		ON_ENTERRELEASE,
		SCRIPT_COUNT
	};

	enum
	{
		ADJUST_MOVE = 0,
		ADJUST_TOP,
		ADJUST_RIGHT,
		ADJUST_BOTTOM,
		ADJUST_LEFT,
		ADJUST_TOPLEFT,
		ADJUST_BOTTOMRIGHT,
		ADJUST_TOPRIGHT,
		ADJUST_BOTTOMLEFT
	};

	static const char* ScriptNames[SCRIPT_COUNT];

	static const idRegEntry RegisterVars[];
	static const int		NumRegisterVars;

	idWindow* SetFocus( idWindow* w, bool scripts = true );

	idWindow* SetCapture( idWindow* w );
	void SetParent( idWindow* w );
	void SetFlag( unsigned int f );
	void ClearFlag( unsigned int f );
	unsigned GetFlags()
	{
		return flags;
	};
	void Move( float x, float y );
	void BringToTop( idWindow* w );
	void Adjust( float xd, float yd );
	void SetAdjustMode( idWindow* child );
	void Size( float x, float y, float w, float h );
	void SetupFromState();
	void SetupBackground();
	drawWin_t* FindChildByName( const char* name );
	idSimpleWindow* FindSimpleWinByName( const char* _name );
	idWindow* GetParent()
	{
		return parent;
	}
	idUserInterfaceLocal* GetGui()
	{
		return gui;
	};
	bool Contains( float x, float y );
	size_t Size();
	virtual size_t Allocated();
	idStr* GetStrPtrByName( const char* _name );

	virtual idWinVar* GetWinVarByName( const char* _name, bool winLookup = false, drawWin_t** owner = NULL );
	// DG: the return value is a pointer, so use intptr_t
	intptr_t GetWinVarOffset( idWinVar* wv, drawWin_t* dw );
	// DG end
	float GetMaxCharHeight();
	float GetMaxCharWidth();
	void SetFont();
	void SetInitialState( const char* _name );
	void AddChild( idWindow* win );
	void DebugDraw( int time, float x, float y );
	void CalcClientRect( float xofs, float yofs );
	void CommonInit();
	void CleanUp();
	void DrawBorderAndCaption( const idRectangle& drawRect );
	void DrawCaption( int time, float x, float y );
	void SetupTransforms( float x, float y );
	bool Contains( const idRectangle& sr, float x, float y );
	const char* GetName()
	{
		return name;
	};

	virtual bool Parse( idTokenParser* src, bool rebuild = true );
	virtual const char* HandleEvent( const sysEvent_t* event, bool* updateVisuals );
	void	CalcRects( float x, float y );
	virtual void Redraw( float x, float y, bool hud );

	virtual void ArchiveToDictionary( idDict* dict, bool useNames = true );
	virtual void InitFromDictionary( idDict* dict, bool byName = true );
	virtual void PostParse();
	virtual void Activate( bool activate, idStr& act );
	virtual void Trigger();
	virtual void GainFocus();
	virtual void LoseFocus();
	virtual void GainCapture();
	virtual void LoseCapture();
	virtual void Sized();
	virtual void Moved();
	virtual void Draw( int time, float x, float y );
	virtual void MouseExit();
	virtual void MouseEnter();
	virtual void DrawBackground( const idRectangle& drawRect );
	virtual idWindow* GetChildWithOnAction( float xd, float yd );
	virtual const char* RouteMouseCoords( float xd, float yd );
	virtual void SetBuddy( idWindow* buddy ) {};
	virtual void HandleBuddyUpdate( idWindow* buddy ) {};
	virtual void StateChanged( bool redraw );

	// SaveGame support
	void			WriteSaveGameString( const char* string, idFile* savefile );
	void			WriteSaveGameTransition( idTransitionData& trans, idFile* savefile );
	virtual void	WriteToSaveGame( idFile* savefile );
	void			ReadSaveGameString( idStr& string, idFile* savefile );
	void			ReadSaveGameTransition( idTransitionData& trans, idFile* savefile );
	virtual void	ReadFromSaveGame( idFile* savefile );
	void			FixupTransitions();
	virtual void HasAction() {};
	virtual void HasScripts() {};

	void FixupParms();
	void GetScriptString( const char* name, idStr& out );
	void SetScriptParams();
	bool HasOps()
	{
		return ( ops.Num() > 0 );
	};
	float EvalRegs( int test = -1, bool force = false );
	void StartTransition();
	void AddTransition( idWinVar* dest, idVec4 from, idVec4 to, int time, float accelTime, float decelTime );
	void ResetTime( int time );
	void ResetCinematics();

	int NumTransitions();

	bool ParseScript( idTokenParser* src, idGuiScriptList& list, int* timeParm = NULL, bool allowIf = false );
	bool RunScript( int n );
	bool RunScriptList( idGuiScriptList* src );
	void SetRegs( const char* key, const char* val );
	// DG: component and the return value are really pointers, so use intptr_t
	intptr_t ParseExpression( idTokenParser* src, idWinVar* var = NULL, intptr_t component = 0 );
	// DG end
	int ExpressionConstant( float f );
	idRegisterList* RegList()
	{
		return &regList;
	}
	void AddCommand( const char* cmd );
	void AddUpdateVar( idWinVar* var );
	bool Interactive();
	bool ContainsStateVars();
	void SetChildWinVarVal( const char* name, const char* var, const char* val );
	idWindow* GetFocusedChild();
	idWindow* GetCaptureChild();
	const char* GetComment()
	{
		return comment;
	}
	void SetComment( const char* p )
	{
		comment = p;
	}

	idStr cmd;

	virtual void RunNamedEvent( const char* eventName );

	void		AddDefinedVar( idWinVar* var );

	idWindow*	FindChildByPoint( float x, float y, idWindow* below = NULL );
	int			GetChildIndex( idWindow* window );
	int			GetChildCount();
	idWindow*	GetChild( int index );
	void		RemoveChild( idWindow* win );
	bool		InsertChild( idWindow* win, idWindow* before );

	void		ScreenToClient( idRectangle* rect );
	void		ClientToScreen( idRectangle* rect );

	bool		UpdateFromDictionary( idDict& dict );

protected:

	friend		class rvGEWindowWrapper;

	idWindow*	FindChildByPoint( float x, float y, idWindow** below );
	void		SetDefaults();

	friend class idSimpleWindow;
	friend class idUserInterfaceLocal;
	bool IsSimple();
	void UpdateWinVars();
	void DisableRegister( const char* _name );
	void Transition();
	void Time();
	bool RunTimeEvents( int time );
	void Dump();

	int ExpressionTemporary();
	wexpOp_t* ExpressionOp();
	// DG: a, b, component and the return values are really pointers, so use intptr_t
	intptr_t EmitOp( intptr_t a, intptr_t b, wexpOpType_t opType, wexpOp_t** opp = NULL );
	intptr_t ParseEmitOp( idTokenParser* src, intptr_t a, wexpOpType_t opType, int priority, wexpOp_t** opp = NULL );
	intptr_t ParseTerm( idTokenParser* src, idWinVar* var = NULL, intptr_t component = 0 );
	intptr_t ParseExpressionPriority( idTokenParser* src, int priority, idWinVar* var = NULL, intptr_t component = 0 );
	// DG end
	void EvaluateRegisters( float* registers );
	void SaveExpressionParseState();
	void RestoreExpressionParseState();
	void ParseBracedExpression( idTokenParser* src );
	bool ParseScriptEntry( const char* name, idTokenParser* src );
	bool ParseRegEntry( const char* name, idTokenParser* src );
	virtual bool ParseInternalVar( const char* name, idTokenParser* src );
	void ParseString( idTokenParser* src, idStr& out );
	void ParseVec4( idTokenParser* src, idVec4& out );
	void ConvertRegEntry( const char* name, idTokenParser* src, idStr& out, int tabs );

	float actualX;					// physical coords
	float actualY;					// ''
	int	  childID;					// this childs id
	unsigned int flags;             // visible, focus, mouseover, cursor, border, etc..
	int lastTimeRun;				//
	idRectangle drawRect;			// overall rect
	idRectangle clientRect;			// client area
	idVec2	origin;

	int timeLine;					// time stamp used for various fx
	float xOffset;
	float yOffset;
	float forceAspectWidth;
	float forceAspectHeight;
	float matScalex;
	float matScaley;
	float borderSize;
	float textAlignx;
	float textAligny;
	idStr	name;
	idStr	comment;
	idVec2	shear;

	class idFont* font;
	signed char	textShadow;
	unsigned char cursor;					//
	signed char	textAlign;

	idWinBool	noTime;					//
	idWinBool	visible;				//
	idWinBool	noEvents;
	idWinRectangle rect;				// overall rect
	idWinVec4	backColor;
	idWinVec4	matColor;
	idWinVec4	foreColor;
	idWinVec4	hoverColor;
	idWinVec4	borderColor;
	idWinFloat	textScale;
	idWinFloat	rotate;
	idWinStr	text;
	idWinBackground	backGroundName;			//

	idList<idWinVar*, TAG_OLD_UI> definedVars;
	idList<idWinVar*, TAG_OLD_UI> updateVars;

	idRectangle textRect;			// text extented rect
	const idMaterial* background;         // background asset

	idWindow* parent;				// parent window
	idList<idWindow*, TAG_OLD_UI> children;		// child windows
	idList<drawWin_t, TAG_OLD_UI> drawWindows;

	idWindow* focusedChild;			// if a child window has the focus
	idWindow* captureChild;			// if a child window has mouse capture
	idWindow* overChild;			// if a child window has mouse capture
	bool hover;

	idUserInterfaceLocal* gui;

	static idCVar gui_debug;
	static idCVar gui_edit;

	idGuiScriptList* scripts[SCRIPT_COUNT];
	bool* saveTemps;

	idList<idTimeLineEvent*, TAG_OLD_UI> timeLineEvents;
	idList<idTransitionData, TAG_OLD_UI> transitions;

	static bool registerIsTemporary[MAX_EXPRESSION_REGISTERS]; // statics to assist during parsing

	idList<wexpOp_t, TAG_OLD_UI> ops;			   	// evaluate to make expressionRegisters
	idList<float, TAG_OLD_UI> expressionRegisters;
	idList<wexpOp_t, TAG_OLD_UI>* saveOps;			   	// evaluate to make expressionRegisters
	idList<rvNamedEvent*, TAG_OLD_UI>		namedEvents;		//  added named events
	idList<float, TAG_OLD_UI>* saveRegs;

	idRegisterList regList;

	idWinBool	hideCursor;
};

ID_INLINE void idWindow::AddDefinedVar( idWinVar* var )
{
	definedVars.AddUnique( var );
}

#endif /* !__WINDOW_H__ */
