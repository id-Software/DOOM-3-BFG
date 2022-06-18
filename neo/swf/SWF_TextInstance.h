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
#ifndef __SWF_TEXTINSTANCE_H__
#define __SWF_TEXTINSTANCE_H__

struct subTimingWordData_t
{
	subTimingWordData_t()
	{
		startTime = 0;
		forceBreak = false;
	}

	idStr phrase;
	int startTime;
	bool forceBreak;
};

class idSWFTextInstance
{
public:
	idSWFTextInstance();
	~idSWFTextInstance();

	void Init( idSWFEditText* editText, idSWF* _swf );

	idSWFScriptObject* GetScriptObject()
	{
		return &scriptObject;
	}

	bool	GetHasDropShadow()
	{
		return useDropShadow;
	}
	bool	HasStroke()
	{
		return useStroke;
	}
	float	GetStrokeStrength()
	{
		return strokeStrength;
	}
	float	GetStrokeWeight()
	{
		return strokeWeight;
	}

	// used for when text has random render mode set
	bool	IsGeneratingRandomText()
	{
		return generatingText;
	}
	void	StartRandomText( int time );
	idStr	GetRandomText( int time );
	void	StartParagraphText( int time );
	idStr	GetParagraphText( int time );
	bool	NeedsGenerateRandomText()
	{
		return triggerGenerate;
	}
	bool	NeedsSoundPlayed();
	void	ClearPlaySound()
	{
		needsSoundUpdate = false;
	}
	idStr	GetSoundClip()
	{
		return soundClip;
	}
	void	SetIgnoreColor( bool ignore )
	{
		ignoreColor = ignore;
	}

	void	SetStrokeInfo( bool use, float strength = 0.75f, float weight = 1.75f );
	int		CalcMaxScroll( int numLines = -1 );
	int		CalcNumLines();

	// subtitle functions
	void	SwitchSubtitleText( int time );
	bool	UpdateSubtitle( int time );
	bool	IsSubtitle()
	{
		return isSubtitle;
	}
	bool	IsUpdatingSubtitle()
	{
		return subUpdating;
	}
	void	SetSubEndIndex( int endChar, int time );
	int		GetLastWordIndex()
	{
		return subLastWordIndex;
	}
	int		GetPrevLastWordIndex()
	{
		return subPrevLastWordIndex;
	}
	void	LastWordChanged( int wordCount, int time );
	void	SetSubStartIndex( int value )
	{
		subCharStartIndex = value;
	}
	int		GetSubEndIndex()
	{
		return subCharEndIndex;
	}
	int		GetSubStartIndex()
	{
		return subCharStartIndex;
	}
	void	SetSubNextStartIndex( int value );
	int		GetApporoximateSubtitleBreak( int time );
	bool	SubNeedsSwitch()
	{
		return subNeedsSwitch;
	}
	idStr	GetPreviousText()
	{
		return subtitleText.c_str();
	}
	void	SubtitleComplete();
	int		GetSubAlignment()
	{
		return subAlign;
	}
	idStr	GetSpeaker()
	{
		return subSpeaker.c_str();
	}
	void	SubtitleCleanup();
	float	GetTextLength();
	int		GetInputStartChar()
	{
		return inputTextStartChar;
	}
	void	SetInputStartCharacter( int c )
	{
		inputTextStartChar = c;
	}

	const idSWFEditText* GetEditText() const
	{
		return editText;
	}
	void	SetText( idStr val )
	{
		text = val;
		lengthCalculated = false;
	}

	// Removing the private access control statement due to cl 214702
	// Apparently MS's C++ compiler supports the newer C++ standard, and GCC supports C++03
	// In the new C++ standard, nested members of a friend class have access to private/protected members of the class granting friendship
	// In C++03, nested members defined in a friend class do NOT have access to private/protected members of the class granting friendship

	idSWFEditText* editText;
	idSWF* 	swf;

	// this text instance's script object
	idSWFScriptObject  scriptObject;

	idStr text;
	idStr randomtext;
	idStr variable;
	swfColorRGBA_t color;

	bool visible;
	bool tooltip;

	int selectionStart;
	int selectionEnd;
	bool ignoreColor;

	int scroll;
	int scrollTime;
	int maxscroll;
	int maxLines;
	float glyphScale;
	swfRect_t bounds;
	float linespacing;

	bool shiftHeld;
	int lastInputTime;

	bool useDropShadow;
	bool useStroke;

	float strokeStrength;
	float strokeWeight;

	int		textLength;
	bool	lengthCalculated;

	swfTextRenderMode_t renderMode;
	bool		generatingText;
	int			rndSpotsVisible;
	int			rndSpacesVisible;
	int			charMultiplier;
	int			textSpotsVisible;
	int			rndTime;
	int			startRndTime;
	int			prevReplaceIndex;
	bool		triggerGenerate;
	int			renderDelay;
	bool		scrollUpdate;
	idStr		soundClip;
	bool		needsSoundUpdate;
	idList<int, TAG_SWF>	indexArray;
	idRandom2	rnd;

	// used for subtitles
	bool		isSubtitle;
	int			subLength;
	int			subCharDisplayTime;
	int			subAlign;
	bool		subUpdating;
	int			subCharStartIndex;
	int			subNextStartIndex;
	int			subCharEndIndex;
	int			subDisplayTime;
	int			subStartTime;
	int			subSourceID;
	idStr		subtitleText;
	bool		subNeedsSwitch;
	bool		subForceKillQueued;
	bool		subForceKill;
	int			subKillTimeDelay;
	int			subSwitchTime;
	int			subLastWordIndex;
	int			subPrevLastWordIndex;
	idStr		subSpeaker;
	bool		subWaitClear;
	bool		subInitialLine;

	// input text
	int			inputTextStartChar;

	idList< subTimingWordData_t, TAG_SWF > subtitleTimingInfo;
};

/*
================================================
This is the prototype object that all the text instance script objects reference
================================================
*/
class idSWFScriptObject_TextInstancePrototype : public idSWFScriptObject
{
public:
	idSWFScriptObject_TextInstancePrototype();

	//----------------------------------
	// Native Script Functions
	//----------------------------------
#define SWF_TEXT_FUNCTION_DECLARE( x ) \
	class idSWFScriptFunction_##x : public idSWFScriptFunction_RefCounted { \
	public: \
		void			AddRef() {} \
		void			Release() {} \
		idSWFScriptVar Call( idSWFScriptObject * thisObject, const idSWFParmList & parms ); \
	} scriptFunction_##x;

	SWF_TEXT_FUNCTION_DECLARE( onKey );
	SWF_TEXT_FUNCTION_DECLARE( onChar );
	SWF_TEXT_FUNCTION_DECLARE( generateRnd );
	SWF_TEXT_FUNCTION_DECLARE( calcNumLines );

	SWF_NATIVE_VAR_DECLARE( text );
	SWF_NATIVE_VAR_DECLARE( autoSize );
	SWF_NATIVE_VAR_DECLARE( dropShadow );
	SWF_NATIVE_VAR_DECLARE( _stroke );
	SWF_NATIVE_VAR_DECLARE( _strokeStrength );
	SWF_NATIVE_VAR_DECLARE( _strokeWeight );
	SWF_NATIVE_VAR_DECLARE( variable );
	SWF_NATIVE_VAR_DECLARE( _alpha );
	SWF_NATIVE_VAR_DECLARE( textColor );
	SWF_NATIVE_VAR_DECLARE( _visible );
	SWF_NATIVE_VAR_DECLARE( scroll );
	SWF_NATIVE_VAR_DECLARE( maxscroll );
	SWF_NATIVE_VAR_DECLARE( selectionStart );
	SWF_NATIVE_VAR_DECLARE( selectionEnd );
	SWF_NATIVE_VAR_DECLARE( isTooltip );
	SWF_NATIVE_VAR_DECLARE( mode );
	SWF_NATIVE_VAR_DECLARE( delay );
	SWF_NATIVE_VAR_DECLARE( renderSound );
	SWF_NATIVE_VAR_DECLARE( updateScroll );
	SWF_NATIVE_VAR_DECLARE( subtitle );
	SWF_NATIVE_VAR_DECLARE( subtitleAlign );
	SWF_NATIVE_VAR_DECLARE( subtitleSourceID );
	SWF_NATIVE_VAR_DECLARE( subtitleSpeaker );

	SWF_NATIVE_VAR_DECLARE_READONLY( _textLength );

	SWF_TEXT_FUNCTION_DECLARE( subtitleSourceCheck );
	SWF_TEXT_FUNCTION_DECLARE( subtitleStart );
	SWF_TEXT_FUNCTION_DECLARE( subtitleLength );
	SWF_TEXT_FUNCTION_DECLARE( killSubtitle );
	SWF_TEXT_FUNCTION_DECLARE( forceKillSubtitle );
	SWF_TEXT_FUNCTION_DECLARE( subLastLine );
	SWF_TEXT_FUNCTION_DECLARE( addSubtitleInfo );
	SWF_TEXT_FUNCTION_DECLARE( terminateSubtitle );
	SWF_TEXT_FUNCTION_DECLARE( clearTimingInfo );
};

#endif // !__SWF_TEXTINSTANCE_H__
