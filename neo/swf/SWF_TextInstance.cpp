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
#include "../renderer/Font.h"

idSWFScriptObject_TextInstancePrototype textInstanceScriptObjectPrototype;

idCVar swf_textScrollSpeed( "swf_textScrollSpeed", "80", CVAR_INTEGER, "scroll speed for text" );
idCVar swf_textRndLetterSpeed( "swf_textRndLetterSpeed", "8", CVAR_INTEGER, "scroll speed for text" );
idCVar swf_textRndLetterDelay( "swf_textRndLetterDelay", "100", CVAR_INTEGER, "scroll speed for text" );
idCVar swf_textParagraphSpeed( "swf_textParagraphSpeed", "15", CVAR_INTEGER, "scroll speed for text" );
idCVar swf_textParagraphInc( "swf_textParagraphInc", "1.3", CVAR_FLOAT, "scroll speed for text" );
idCVar swf_subtitleExtraTime( "swf_subtitleExtraTime", "3500", CVAR_INTEGER, "time after subtitles vo is complete" );
idCVar swf_subtitleEarlyTrans( "swf_subtitleEarlyTrans", "3500", CVAR_INTEGER, "early time out to switch the line" );
idCVar swf_subtitleLengthGuess( "swf_subtitleLengthGuess", "10000", CVAR_INTEGER, "early time out to switch the line" );
idCVar swf_textMaxInputLength( "swf_textMaxInputLength", "104", CVAR_INTEGER, "max number of characters that can go into the input line" );
idCVar swf_textStrokeSize( "swf_textStrokeSize", "1.65f", CVAR_FLOAT, "size of font glyph stroke", 0.0f, 2.0f );
idCVar swf_textStrokeSizeGlyphSpacer( "swf_textStrokeSizeGlyphSpacer", "1.5f", CVAR_FLOAT, "additional space for spacing glyphs using stroke" );


/*
========================
idSWFTextInstance::idSWFTextInstance
========================
*/
idSWFTextInstance::idSWFTextInstance()
{
	swf = NULL;
}

/*
========================
idSWFTextInstance::~idSWFTextInstance
================== ======
*/
idSWFTextInstance::~idSWFTextInstance()
{
	scriptObject.SetText( NULL );
	scriptObject.Clear();
	scriptObject.Release();

	subtitleTimingInfo.Clear();
}

/*
========================
idSWFTextInstance::Init
========================
*/
void idSWFTextInstance::Init( idSWFEditText* _editText, idSWF* _swf )
{
	editText = _editText;
	swf = _swf;

	text = idLocalization::GetString( editText->initialText );

	lengthCalculated = false;
	variable = editText->variable;
	color = editText->color;
	visible = true;

	selectionStart = -1;
	selectionEnd = -1;

	scroll = 0;
	scrollTime = 0;
	maxscroll = 0;
	maxLines = 0;
	linespacing = 0;
	glyphScale = 1.0f;

	shiftHeld = false;
	tooltip = false;
	renderMode = SWF_TEXT_RENDER_NORMAL;
	generatingText = false;
	triggerGenerate = false;
	rndSpotsVisible = 0;
	textSpotsVisible = 0;
	startRndTime = 0;
	charMultiplier = 0;
	prevReplaceIndex = 0;
	scrollUpdate = false;
	ignoreColor = false;

	isSubtitle = false;
	subLength = 0;
	subAlign = 0;
	subUpdating = false;
	subCharStartIndex = 0;
	subNextStartIndex = 0;
	subCharEndIndex = 0;
	subDisplayTime = 0;
	subStartTime = -1;
	subSourceID = -1;
	subNeedsSwitch = false;
	subForceKill = false;
	subKillTimeDelay = 0;
	subSwitchTime = 0;
	subLastWordIndex = 0;
	subPrevLastWordIndex = 0;
	subInitialLine = true;

	textLength = 0;

	inputTextStartChar = 0;

	renderDelay = swf_textRndLetterDelay.GetInteger();
	needsSoundUpdate = false;
	useDropShadow = false;
	useStroke = false;
	strokeStrength = 1.0f;
	strokeWeight = swf_textStrokeSize.GetFloat();

	scriptObject.SetPrototype( &textInstanceScriptObjectPrototype );
	scriptObject.SetText( this );
	scriptObject.SetNoAutoDelete( true );
}

/*
========================
idSWFTextInstance::GetTextLength
========================
*/
float idSWFTextInstance::GetTextLength()
{
	// CURRENTLY ONLY WORKS FOR SINGLE LINE TEXTFIELDS

	if( lengthCalculated && variable.IsEmpty() )
	{
		return textLength;
	}

	idStr txtLengthCheck = "";

	float len = 0.0f;
	if( verify( swf != NULL ) )
	{

		if( !variable.IsEmpty() )
		{
			idSWFScriptVar var = swf->GetGlobal( variable );
			if( var.IsUndefined() )
			{
				txtLengthCheck = text;
			}
			else
			{
				txtLengthCheck = var.ToString();
			}
			txtLengthCheck = idLocalization::GetString( txtLengthCheck );
		}
		else
		{
			txtLengthCheck = idLocalization::GetString( text );
		}

		const idSWFEditText* shape = editText;
		idSWFDictionaryEntry* fontEntry = swf->FindDictionaryEntry( shape->fontID, SWF_DICT_FONT );
		idSWFFont* swfFont = fontEntry->font;
		float width = fabs( shape->bounds.br.x - shape->bounds.tl.x );
		float postTrans = SWFTWIP( shape->fontHeight );
		const idFont* fontInfo = swfFont->fontID;
		float glyphScale = postTrans / 48.0f;

		int tlen = txtLengthCheck.Length();
		int index = 0;
		while( index < tlen )
		{
			scaledGlyphInfo_t glyph;
			fontInfo->GetScaledGlyph( glyphScale, txtLengthCheck.UTF8Char( index ), glyph );

			len += glyph.xSkip;
			if( useStroke )
			{
				len += ( swf_textStrokeSizeGlyphSpacer.GetFloat() * strokeWeight * glyphScale );
			}

			if( !( shape->flags & SWF_ET_AUTOSIZE ) && len >= width )
			{
				len = width;
				break;
			}
		}
	}

	lengthCalculated = true;
	textLength = len;
	return textLength;
}

/*
========================
idSWFTextInstance::StartParagraphText
========================
*/
void idSWFTextInstance::StartParagraphText( int time )
{
	generatingText = true;
	textSpotsVisible = 0;
	randomtext = "";
	triggerGenerate = false;
	startRndTime = time;
	rndTime = time;
	rnd.SetSeed( time );
	prevReplaceIndex = 0;
	rndSpotsVisible = text.Length();
	indexArray.Clear();
	charMultiplier = 0;

	text = idLocalization::GetString( text );
	lengthCalculated = false;

	for( int index = 0; index < text.Length(); ++index )
	{
		randomtext.Append( " " );
		indexArray.Append( index );
	}

	for( int index = 0; index < indexArray.Num(); ++index )
	{
		int swapIndex = rnd.RandomInt( indexArray.Num() );
		int val = indexArray[index];
		indexArray[index] = indexArray[swapIndex];
		indexArray[swapIndex] = val;
	}
}

/*
========================
idSWFTextInstance::GetParagraphText
========================
*/
idStr idSWFTextInstance::GetParagraphText( int time )
{
	if( triggerGenerate )
	{
		return " ";
	}
	else if( time - startRndTime < renderDelay )
	{
		return " ";
	}
	else if( generatingText )
	{
		if( time - rndTime >= renderDelay )
		{
			rndTime = time;
			needsSoundUpdate = true;

			if( prevReplaceIndex >= text.Length() )
			{
				generatingText = false;
				return text;
			}

			randomtext[prevReplaceIndex] = text[prevReplaceIndex];
			prevReplaceIndex++;
		}
	}
	else
	{
		scrollUpdate = false;
		return text;
	}

	return randomtext;
}

/*
========================
idSWFTextInstance::StartRandomText
========================
*/
bool idSWFTextInstance::NeedsSoundPlayed()
{
	if( soundClip.IsEmpty() )
	{
		return false;
	}

	return needsSoundUpdate;
}

/*
========================
idSWFTextInstance::StartRandomText
========================
*/
void idSWFTextInstance::StartRandomText( int time )
{
	generatingText = true;
	textSpotsVisible = 0;
	randomtext = "";
	triggerGenerate = false;
	startRndTime = time;
	rndTime = time;
	rnd.SetSeed( time );
	rndSpotsVisible = 0;

	text = idLocalization::GetString( text );
	lengthCalculated = false;

	for( int index = 0; index < text.Length(); ++index )
	{
		if( text[index] == ' ' )
		{
			randomtext.Append( " " );
		}
		else
		{
			randomtext.Append( "." );
			rndSpotsVisible++;
		}
	}
}

/*
========================
idSWFTextInstance::GetRandomText
========================
*/
idStr idSWFTextInstance::GetRandomText( int time )
{

	if( triggerGenerate )
	{
		return " ";
	}
	else if( time - startRndTime < renderDelay )
	{
		return " ";
	}
	else if( generatingText )
	{
		if( rndSpotsVisible > 0 )
		{

			int waitTime = swf_textRndLetterSpeed.GetInteger();

			if( randomtext.Length() >= 10 )
			{
				waitTime = waitTime / 3;
			}

			if( time - rndTime >= waitTime )
			{
				rndTime = time;

				int spotIndex = rnd.RandomInt( rndSpotsVisible );
				int cIndex = 0;
				for( int c = 0; c < randomtext.Length(); ++ c )
				{

					if( c >= text.Length() )
					{
						rndSpotsVisible = 0;
						break;
					}

					if( randomtext[c] == '.' )
					{
						cIndex++;
					}

					if( cIndex == spotIndex )
					{

						bool useCaps = false;

						if( c - 1 >= 0 && text[c - 1] == ' ' )
						{
							useCaps = true;
						}
						else if( c == 0 )
						{
							useCaps = true;
						}

						if( useCaps || renderMode == SWF_TEXT_RENDER_RANDOM_APPEAR_CAPS )
						{
							randomtext[c] = rnd.RandomInt( 'Z' - 'A' ) + 'A';
						}
						else
						{
							randomtext[c] = rnd.RandomInt( 'z' - 'a' ) + 'a';
						}

						rndSpotsVisible--;
						if( !soundClip.IsEmpty() )
						{
							needsSoundUpdate = true;
						}
						break;
					}
				}
			}
		}
		else if( rndSpotsVisible == 0 && textSpotsVisible < text.Length() )
		{
			if( textSpotsVisible >= randomtext.Length() )
			{
				textSpotsVisible++;
			}
			else
			{
				if( time - rndTime >= swf_textRndLetterSpeed.GetInteger() )
				{
					rndTime = time;
					randomtext[textSpotsVisible] = text[textSpotsVisible];
					textSpotsVisible++;
					if( !soundClip.IsEmpty() )
					{
						needsSoundUpdate = true;
					}
				}
			}
		}
		else
		{
			return " ";
		}
	}
	else
	{
		return text;
	}

	if( rndSpotsVisible == 0 && textSpotsVisible == text.Length() )
	{
		generatingText = false;
	}

	return randomtext;
}

/*
==============================================
SUBTITLE FUNCTIONALITY
==============================================
*/

/*
==============================================
idSWFTextInstance::SwitchSubtitleText
==============================================
*/
void idSWFTextInstance::SwitchSubtitleText( int time )
{
	subNeedsSwitch = false;
}

void idSWFTextInstance::SetSubNextStartIndex( int value )
{
	subNextStartIndex = value;
}

/*
==============================================
idSWFTextInstance::UpdateSubtitle
==============================================
*/
bool idSWFTextInstance::UpdateSubtitle( int time )
{

	if( subForceKillQueued )
	{
		subForceKillQueued = false;
		subForceKill = true;
		subKillTimeDelay = time + swf_subtitleExtraTime.GetInteger();
	}

	if( subUpdating && !subForceKill )
	{
		if( ( time >= subSwitchTime && !subNeedsSwitch ) || ( !subNeedsSwitch && subInitialLine ) )
		{
			//idLib::Printf( "SWITCH TIME %d / %d \n", time, subSwitchTime );

			if( subInitialLine && subtitleTimingInfo.Num() > 0 )
			{
				if( subStartTime == -1 )
				{
					subStartTime = time - 600;
				}
				if( time < subStartTime + subtitleTimingInfo[0].startTime )
				{
					return true;
				}
				else
				{
					text = subtitleText;//subtitleText = "";
					subInitialLine = false;
				}
			}

			if( subNextStartIndex + 1 >= text.Length( ) )
			{
				subForceKillQueued = true;
			}
			else
			{
				subCharStartIndex = subNextStartIndex;
				//subtitleText.CopyRange( text, subCharStartIndex, subCharEndIndex );
				subNeedsSwitch = true;
			}
		}
	}

	if( subForceKill )
	{
		if( time >= subKillTimeDelay )
		{
			subForceKill = false;
			return false;
		}
	}

	return true;
}

/*
==============================================
idSWFTextInstance::SubtitleComplete
==============================================
*/
void idSWFTextInstance::SetSubEndIndex( int endChar, int time )
{
	subCharEndIndex = endChar;
	if( subCharEndIndex + 1 >= text.Length() )
	{
		LastWordChanged( subtitleTimingInfo.Num(), time );
	}
}

/*
==============================================
idSWFTextInstance::SubtitleComplete
==============================================
*/
void idSWFTextInstance::SubtitleComplete()
{
	subInitialLine = true;
	subUpdating = false;
	isSubtitle = false;
	subNeedsSwitch = false;
	subCharDisplayTime = 0;
	subForceKillQueued = false;
	subForceKill = false;
	subKillTimeDelay = 0;
	subSwitchTime = 0;
	subLastWordIndex = 0;
	subPrevLastWordIndex = 0;
	subStartTime = -1;
	subSpeaker = "";
	subtitleText = "";
	text = "";
	subtitleTimingInfo.Clear();
}

/*
==============================================
idSWFTextInstance::LastWordChanged
==============================================
*/
void idSWFTextInstance::LastWordChanged( int wordCount, int time )
{
	if( subPrevLastWordIndex + wordCount >= subtitleTimingInfo.Num() )
	{
		subLastWordIndex = subtitleTimingInfo.Num() - 1;
	}
	else
	{
		subLastWordIndex = subPrevLastWordIndex + wordCount - 1;
	}

	if( subStartTime == -1 )
	{
		if( subtitleTimingInfo.Num() > 0 )
		{
			subStartTime = time + subtitleTimingInfo[0].startTime;
		}
		else
		{
			subStartTime = time;
		}
	}

	subSwitchTime = subStartTime + subtitleTimingInfo[subLastWordIndex].startTime;// - swf_subtitleEarlyTrans.GetInteger();
	//idLib::Printf( "switchtime set 1 %d last word %d\n", subSwitchTime, subLastWordIndex );
}

/*
==============================================
idSWFTextInstance::GetSubtitleBreak
==============================================
*/
int idSWFTextInstance::GetApporoximateSubtitleBreak( int time )
{

	int wordIndex = subLastWordIndex;
	bool setSwitchTime = false;

	if( subStartTime == -1 )
	{
		subStartTime = time;
	}

	if( time >= subSwitchTime )
	{
		subPrevLastWordIndex = subLastWordIndex;
		for( int i = wordIndex; i < subtitleTimingInfo.Num(); ++i )
		{
			if( subtitleTimingInfo[i].forceBreak )
			{
				if( i + 1 < subtitleTimingInfo.Num() )
				{
					subSwitchTime = subStartTime + subtitleTimingInfo[i + 1].startTime; // - swf_subtitleEarlyTrans.GetInteger();
					//idLib::Printf( "switchtime set 2 %d\n", subSwitchTime );
					subLastWordIndex = i;
					setSwitchTime = true;
					break;
				}
				else
				{
					subSwitchTime = subStartTime + subtitleTimingInfo[i].startTime;// - swf_subtitleEarlyTrans.GetInteger();
					//idLib::Printf( "switchtime set 3 %d\n", subSwitchTime );
					subLastWordIndex = i;
					setSwitchTime = true;
					break;
				}
			}
			else
			{
				int timeSpan = subtitleTimingInfo[i].startTime - subtitleTimingInfo[wordIndex].startTime;
				if( timeSpan > swf_subtitleLengthGuess.GetInteger() )
				{
					if( i - 1 >= 0 )
					{
						subSwitchTime = subStartTime + subtitleTimingInfo[i].startTime;// - swf_subtitleEarlyTrans.GetInteger();
						//idLib::Printf( "switchtime set 4 %d\n", subSwitchTime );
						subLastWordIndex = i - 1;
					}
					else
					{
						subSwitchTime = subStartTime + subtitleTimingInfo[i].startTime;// - swf_subtitleEarlyTrans.GetInteger();
						//idLib::Printf( "switchtime set 5 %d\n", subSwitchTime );
						subLastWordIndex = i;
					}
					setSwitchTime = true;
					break;
				}
			}
		}

		if( !setSwitchTime && subtitleTimingInfo.Num() > 0 )
		{
			subSwitchTime = subStartTime + subtitleTimingInfo[ subtitleTimingInfo.Num() - 1 ].startTime;// - swf_subtitleEarlyTrans.GetInteger();
			//idLib::Printf( "switchtime set 6 %d\n", subSwitchTime );
			subLastWordIndex = subtitleTimingInfo.Num();
		}
	}

	return subLastWordIndex;
}


/*
==============================================
idSWFTextInstance::SubtitleComplete
==============================================
*/
void idSWFTextInstance::SubtitleCleanup()
{
	subSourceID = -1;
	subAlign = -1;
	text = "";
}

/*
==============================================
idSWFTextInstance::SetStrokeInfo
==============================================
*/
void idSWFTextInstance::SetStrokeInfo( bool use, float strength, float weight )
{
	useStroke = use;
	if( use )
	{
		strokeWeight = weight;
		strokeStrength = strength;
	}
}

/*
==============================================
idSWFTextInstance::CalcMaxScroll
==============================================
*/
int idSWFTextInstance::CalcMaxScroll( int numLines )
{

	if( numLines != -1 )
	{
		if( numLines < 0 )
		{
			numLines = 0;
		}
		maxscroll = numLines;
		return maxscroll;
	}

	const idSWFEditText* shape = editText;
	if( !( shape->flags & SWF_ET_MULTILINE ) )
	{
		return 0;
	}

	if( swf == NULL )
	{
		return 0;
	}

	idSWFDictionaryEntry* fontEntry = swf->FindDictionaryEntry( shape->fontID, SWF_DICT_FONT );
	if( fontEntry == NULL )
	{
		return 0;
	}

	idSWFFont* swfFont = fontEntry->font;
	if( swfFont == NULL )
	{
		return 0;
	}

	const idFont* fontInfo = swfFont->fontID;
	if( fontInfo == NULL )
	{
		return 0;
	}

	idStr textCheck;
	if( variable.IsEmpty() )
	{
		textCheck = idLocalization::GetString( text );
	}
	else
	{
		textCheck = idLocalization::GetString( variable );
	}

	if( textCheck.IsEmpty() )
	{
		return 0;
	}

	float x = bounds.tl.x;
	float y = bounds.tl.y;

	idList< idStr > textLines;
	idStr* currentLine = &textLines.Alloc();

	// tracks the last breakable character we found
	int lastbreak = 0;
	float lastbreakX = 0;
	int charIndex = 0;

	if( IsSubtitle() )
	{
		charIndex = GetSubStartIndex();
	}

	while( charIndex < textCheck.Length() )
	{
		if( textCheck[ charIndex ] == '\n' )
		{
			if( shape->flags & SWF_ET_MULTILINE )
			{
				currentLine->Append( '\n' );
				x = bounds.tl.x;
				y += linespacing;
				currentLine = &textLines.Alloc();
				lastbreak = 0;
				charIndex++;
				continue;
			}
			else
			{
				break;
			}
		}
		int glyphStart = charIndex;
		uint32 tc = textCheck.UTF8Char( charIndex );
		scaledGlyphInfo_t glyph;
		fontInfo->GetScaledGlyph( glyphScale, tc, glyph );
		float glyphSkip = glyph.xSkip;
		if( HasStroke() )
		{
			glyphSkip += ( swf_textStrokeSizeGlyphSpacer.GetFloat() * GetStrokeWeight() * glyphScale );
		}

		if( x + glyphSkip > bounds.br.x )
		{
			if( shape->flags & ( SWF_ET_MULTILINE | SWF_ET_WORDWRAP ) )
			{
				if( lastbreak > 0 )
				{
					int curLineIndex = currentLine - &textLines[0];
					idStr* newline = &textLines.Alloc();
					currentLine = &textLines[ curLineIndex ];
					if( maxLines == 1 )
					{
						currentLine->CapLength( currentLine->Length() - 3 );
						currentLine->Append( "..." );
						break;
					}
					else
					{
						*newline = currentLine->c_str() + lastbreak;
						currentLine->CapLength( lastbreak );
						currentLine = newline;
						x -= lastbreakX;
					}
				}
				else
				{
					currentLine = &textLines.Alloc();
					x = bounds.tl.x;
				}
				lastbreak = 0;
			}
			else
			{
				break;
			}
		}
		while( glyphStart < charIndex && glyphStart < text.Length() )
		{
			currentLine->Append( text[ glyphStart++ ] );
		}
		x += glyphSkip;
		if( tc == ' ' || tc == '-' )
		{
			lastbreak = currentLine->Length();
			lastbreakX = x;
		}
	}

	maxscroll = textLines.Num() - maxLines;
	if( maxscroll < 0 )
	{
		maxscroll = 0;
	}
	return maxscroll;
}

int idSWFTextInstance::CalcNumLines()
{

	const idSWFEditText* shape = editText;
	if( !( shape->flags & SWF_ET_MULTILINE ) )
	{
		return 1;
	}

	idSWFDictionaryEntry* fontEntry = swf->FindDictionaryEntry( shape->fontID, SWF_DICT_FONT );
	if( fontEntry == NULL )
	{
		return 1;
	}

	idStr textCheck;

	if( variable.IsEmpty() )
	{
		textCheck = idLocalization::GetString( text );
	}
	else
	{
		textCheck = idLocalization::GetString( variable );
	}

	if( textCheck.IsEmpty() )
	{
		return 1;
	}

	if( swf == NULL )
	{
		return 1;
	}

	idSWFFont* swfFont = fontEntry->font;
	float postTransformHeight = SWFTWIP( shape->fontHeight );
	const idFont* fontInfo = swfFont->fontID;

	float glyphScale = postTransformHeight / 48.0f;

	swfRect_t bounds;
	bounds.tl.x = ( shape->bounds.tl.x + SWFTWIP( shape->leftMargin ) );
	bounds.br.x = ( shape->bounds.br.x - SWFTWIP( shape->rightMargin ) );
	bounds.tl.y = ( shape->bounds.tl.y + ( 1.15f * glyphScale ) );
	bounds.br.y = ( shape->bounds.br.y );

	float linespacing = fontInfo->GetAscender( 1.15f * glyphScale );
	if( shape->leading != 0 )
	{
		linespacing += ( glyphScale * SWFTWIP( shape->leading ) );
	}

	float x = bounds.tl.x;
	int maxLines = idMath::Ftoi( ( bounds.br.y - bounds.tl.y ) / linespacing );
	if( maxLines == 0 )
	{
		maxLines = 1;
	}

	// tracks the last breakable character we found
	int numLines = 1;
	int lastbreak = 0;
	int charIndex = 0;

	while( charIndex < textCheck.Length() )
	{
		if( textCheck[ charIndex ] == '\n' )
		{
			if( numLines == maxLines )
			{
				return maxLines;
			}
			numLines++;
			lastbreak = 0;
			x = bounds.tl.x;
			charIndex++;
		}
		else
		{
			uint32 tc = textCheck[ charIndex++ ];
			scaledGlyphInfo_t glyph;
			fontInfo->GetScaledGlyph( glyphScale, tc, glyph );

			float glyphSkip = glyph.xSkip;
			if( useStroke )
			{
				glyphSkip += ( swf_textStrokeSizeGlyphSpacer.GetFloat() * strokeWeight * glyphScale );
			}

			if( x + glyphSkip > bounds.br.x )
			{
				if( numLines == maxLines )
				{
					return maxLines;
				}
				else
				{
					numLines++;
					if( lastbreak != 0 )
					{
						charIndex = charIndex - ( charIndex - lastbreak );
					}
					x = bounds.tl.x;
					lastbreak = 0;
				}
			}
			else
			{
				x += glyphSkip;
				if( tc == ' ' || tc == '-' )
				{
					lastbreak = charIndex;
				}
			}
		}
	}

	return numLines;
}

/*
========================
idSWFScriptObject_TextInstancePrototype
========================
*/

#define SWF_TEXT_FUNCTION_DEFINE( x ) idSWFScriptVar idSWFScriptObject_TextInstancePrototype::idSWFScriptFunction_##x::Call( idSWFScriptObject * thisObject, const idSWFParmList & parms )
#define SWF_TEXT_NATIVE_VAR_DEFINE_GET( x ) idSWFScriptVar idSWFScriptObject_TextInstancePrototype::idSWFScriptNativeVar_##x::Get( class idSWFScriptObject * object )
#define SWF_TEXT_NATIVE_VAR_DEFINE_SET( x ) void  idSWFScriptObject_TextInstancePrototype::idSWFScriptNativeVar_##x::Set( class idSWFScriptObject * object, const idSWFScriptVar & value )

#define SWF_TEXT_PTHIS_FUNC( x ) idSWFTextInstance * pThis = thisObject ? thisObject->GetText() : NULL; if ( !verify( pThis != NULL ) ) { idLib::Warning( "SWF: tried to call " x " on NULL edittext" ); return idSWFScriptVar(); }
#define SWF_TEXT_PTHIS_GET( x ) idSWFTextInstance * pThis = object ? object->GetText() : NULL; if ( pThis == NULL ) { return idSWFScriptVar(); }
#define SWF_TEXT_PTHIS_SET( x ) idSWFTextInstance * pThis = object ? object->GetText() : NULL; if ( pThis == NULL ) { return; }

#define SWF_TEXT_FUNCTION_SET( x ) scriptFunction_##x.AddRef(); Set( #x, &scriptFunction_##x );
#define SWF_TEXT_NATIVE_VAR_SET( x ) SetNative( #x, &swfScriptVar_##x );

idSWFScriptObject_TextInstancePrototype::idSWFScriptObject_TextInstancePrototype()
{

	SWF_TEXT_FUNCTION_SET( onKey );
	SWF_TEXT_FUNCTION_SET( onChar );
	SWF_TEXT_FUNCTION_SET( generateRnd );
	SWF_TEXT_FUNCTION_SET( calcNumLines );
	SWF_TEXT_FUNCTION_SET( clearTimingInfo );

	SWF_TEXT_NATIVE_VAR_SET( text );
	SWF_TEXT_NATIVE_VAR_SET( _textLength );	// only works on single lines of text not multiline
	SWF_TEXT_NATIVE_VAR_SET( autoSize );
	SWF_TEXT_NATIVE_VAR_SET( dropShadow );
	SWF_TEXT_NATIVE_VAR_SET( _stroke );
	SWF_TEXT_NATIVE_VAR_SET( _strokeStrength );
	SWF_TEXT_NATIVE_VAR_SET( _strokeWeight );
	SWF_TEXT_NATIVE_VAR_SET( variable );
	SWF_TEXT_NATIVE_VAR_SET( _alpha );
	SWF_TEXT_NATIVE_VAR_SET( textColor );
	SWF_TEXT_NATIVE_VAR_SET( _visible );
	SWF_TEXT_NATIVE_VAR_SET( selectionStart );
	SWF_TEXT_NATIVE_VAR_SET( selectionEnd );
	SWF_TEXT_NATIVE_VAR_SET( scroll );
	SWF_TEXT_NATIVE_VAR_SET( maxscroll );
	SWF_TEXT_NATIVE_VAR_SET( isTooltip );
	SWF_TEXT_NATIVE_VAR_SET( mode );
	SWF_TEXT_NATIVE_VAR_SET( delay );
	SWF_TEXT_NATIVE_VAR_SET( renderSound );
	SWF_TEXT_NATIVE_VAR_SET( updateScroll );
	SWF_TEXT_NATIVE_VAR_SET( subtitle );
	SWF_TEXT_NATIVE_VAR_SET( subtitleAlign );
	SWF_TEXT_NATIVE_VAR_SET( subtitleSourceID );
	SWF_TEXT_NATIVE_VAR_SET( subtitleSpeaker );

	SWF_TEXT_FUNCTION_SET( subtitleSourceCheck );
	SWF_TEXT_FUNCTION_SET( subtitleStart );
	SWF_TEXT_FUNCTION_SET( subtitleLength );
	SWF_TEXT_FUNCTION_SET( killSubtitle );
	SWF_TEXT_FUNCTION_SET( forceKillSubtitle );
	SWF_TEXT_FUNCTION_SET( subLastLine );
	SWF_TEXT_FUNCTION_SET( addSubtitleInfo );
	SWF_TEXT_FUNCTION_SET( terminateSubtitle );
}

SWF_TEXT_NATIVE_VAR_DEFINE_GET( text )
{
	SWF_TEXT_PTHIS_GET( "text" );
	return pThis->text;
}

SWF_TEXT_NATIVE_VAR_DEFINE_SET( text )
{
	SWF_TEXT_PTHIS_SET( "text " );
	pThis->text = idLocalization::GetString( value.ToString() );
	if( pThis->text.IsEmpty() )
	{
		pThis->selectionEnd = -1;
		pThis->selectionStart = -1;
		pThis->inputTextStartChar = 0;
	}
	pThis->lengthCalculated = false;
}

SWF_TEXT_NATIVE_VAR_DEFINE_GET( autoSize )
{
	SWF_TEXT_PTHIS_GET( "autoSize" );
	return ( pThis->editText->flags & SWF_ET_AUTOSIZE ) != 0;
}

SWF_TEXT_NATIVE_VAR_DEFINE_SET( autoSize )
{
	SWF_TEXT_PTHIS_SET( "autoSize" );
	if( value.ToBool() )
	{
		pThis->editText->flags |= SWF_ET_AUTOSIZE;
	}
	else
	{
		pThis->editText->flags &= ~SWF_ET_AUTOSIZE;
	}
	pThis->lengthCalculated = false;
}

SWF_TEXT_NATIVE_VAR_DEFINE_GET( dropShadow )
{
	SWF_TEXT_PTHIS_GET( "dropShadow" );
	return pThis->useDropShadow;
}
SWF_TEXT_NATIVE_VAR_DEFINE_SET( dropShadow )
{
	SWF_TEXT_PTHIS_SET( "dropShadow" );
	pThis->useDropShadow = value.ToBool();
}
SWF_TEXT_NATIVE_VAR_DEFINE_GET( _stroke )
{
	SWF_TEXT_PTHIS_GET( "_stroke" );
	return pThis->useStroke;
}
SWF_TEXT_NATIVE_VAR_DEFINE_SET( _stroke )
{
	SWF_TEXT_PTHIS_SET( "_stroke" );
	pThis->useStroke = value.ToBool();
}
SWF_TEXT_NATIVE_VAR_DEFINE_GET( _strokeStrength )
{
	SWF_TEXT_PTHIS_GET( "_strokeStrength" );
	return pThis->strokeStrength;
}
SWF_TEXT_NATIVE_VAR_DEFINE_SET( _strokeStrength )
{
	SWF_TEXT_PTHIS_SET( "_strokeStrength" );
	pThis->strokeStrength = value.ToFloat();
}
SWF_TEXT_NATIVE_VAR_DEFINE_GET( _strokeWeight )
{
	SWF_TEXT_PTHIS_GET( "_strokeWeight" );
	return pThis->strokeWeight;
}
SWF_TEXT_NATIVE_VAR_DEFINE_SET( _strokeWeight )
{
	SWF_TEXT_PTHIS_SET( "_strokeWeight" );
	pThis->strokeWeight = value.ToFloat();
}
SWF_TEXT_NATIVE_VAR_DEFINE_GET( variable )
{
	SWF_TEXT_PTHIS_GET( "variable" );
	return pThis->variable;
}
SWF_TEXT_NATIVE_VAR_DEFINE_SET( variable )
{
	SWF_TEXT_PTHIS_SET( "variable" );
	pThis->variable = value.ToString();
}
SWF_TEXT_NATIVE_VAR_DEFINE_GET( _alpha )
{
	SWF_TEXT_PTHIS_GET( "_alpha" );
	return pThis->color.a / 255.0f;
}
SWF_TEXT_NATIVE_VAR_DEFINE_SET( _alpha )
{
	SWF_TEXT_PTHIS_SET( "_alpha" );
	pThis->color.a = idMath::Ftob( value.ToFloat() * 255.0f );
}
SWF_TEXT_NATIVE_VAR_DEFINE_GET( _visible )
{
	SWF_TEXT_PTHIS_GET( "_visible" );
	return pThis->visible;
}
SWF_TEXT_NATIVE_VAR_DEFINE_SET( _visible )
{
	SWF_TEXT_PTHIS_SET( "_visible" );
	pThis->visible = value.ToBool();
}
SWF_TEXT_NATIVE_VAR_DEFINE_GET( selectionStart )
{
	SWF_TEXT_PTHIS_GET( "selectionStart" );
	return pThis->selectionStart;
}
SWF_TEXT_NATIVE_VAR_DEFINE_SET( selectionStart )
{
	SWF_TEXT_PTHIS_SET( "selectionStart" );
	pThis->selectionStart = value.ToInteger();
}
SWF_TEXT_NATIVE_VAR_DEFINE_GET( selectionEnd )
{
	SWF_TEXT_PTHIS_GET( "selectionEnd" );
	return pThis->selectionEnd;
}
SWF_TEXT_NATIVE_VAR_DEFINE_SET( selectionEnd )
{
	SWF_TEXT_PTHIS_SET( "selectionEnd" );
	pThis->selectionEnd = value.ToInteger();
}
SWF_TEXT_NATIVE_VAR_DEFINE_SET( isTooltip )
{
	SWF_TEXT_PTHIS_SET( "isTooltip" );
	pThis->tooltip = value.ToBool();
}
SWF_TEXT_NATIVE_VAR_DEFINE_GET( isTooltip )
{
	SWF_TEXT_PTHIS_GET( "isTooltip" );
	return pThis->tooltip;
}
SWF_TEXT_NATIVE_VAR_DEFINE_SET( delay )
{
	SWF_TEXT_PTHIS_SET( "delay" );
	pThis->renderDelay = value.ToInteger();
}
SWF_TEXT_NATIVE_VAR_DEFINE_GET( delay )
{
	SWF_TEXT_PTHIS_GET( "delay" );
	return pThis->renderDelay;
}
SWF_TEXT_NATIVE_VAR_DEFINE_SET( renderSound )
{
	SWF_TEXT_PTHIS_SET( "renderSound" );
	pThis->soundClip = value.ToString();
}
SWF_TEXT_NATIVE_VAR_DEFINE_GET( renderSound )
{
	SWF_TEXT_PTHIS_GET( "renderSound" );
	return pThis->soundClip;
}
SWF_TEXT_NATIVE_VAR_DEFINE_SET( updateScroll )
{
	SWF_TEXT_PTHIS_SET( "updateScroll" );
	pThis->scrollUpdate = value.ToBool();
}
SWF_TEXT_NATIVE_VAR_DEFINE_GET( updateScroll )
{
	SWF_TEXT_PTHIS_GET( "updateScroll" );
	return pThis->scrollUpdate;
}

SWF_TEXT_NATIVE_VAR_DEFINE_GET( mode )
{
	SWF_TEXT_PTHIS_GET( "mode" );
	return pThis->renderMode;
}
SWF_TEXT_NATIVE_VAR_DEFINE_GET( scroll )
{
	SWF_TEXT_PTHIS_GET( "scroll" );
	return pThis->scroll;
}
SWF_TEXT_NATIVE_VAR_DEFINE_GET( maxscroll )
{
	SWF_TEXT_PTHIS_GET( "maxscroll" );
	return pThis->maxscroll;
}

SWF_TEXT_NATIVE_VAR_DEFINE_GET( _textLength )
{
	SWF_TEXT_PTHIS_GET( "_textLength" );
	return pThis->GetTextLength();
}

SWF_TEXT_NATIVE_VAR_DEFINE_SET( mode )
{
	SWF_TEXT_PTHIS_SET( "mode" );

	int mode = value.ToInteger();

	if( mode >= ( int )SWF_TEXT_RENDER_MODE_COUNT || mode < 0 )
	{
		mode = SWF_TEXT_RENDER_NORMAL;
	}

	pThis->renderMode = swfTextRenderMode_t( mode );
}

SWF_TEXT_NATIVE_VAR_DEFINE_SET( scroll )
{
	SWF_TEXT_PTHIS_SET( "scroll" );

	int time = Sys_Milliseconds();
	if( time >= pThis->scrollTime )
	{
		pThis->scrollTime = Sys_Milliseconds() + swf_textScrollSpeed.GetInteger();
		pThis->scroll = value.ToInteger();
	}
}

SWF_TEXT_NATIVE_VAR_DEFINE_SET( maxscroll )
{
	SWF_TEXT_PTHIS_SET( "maxscroll" );
	pThis->maxscroll = value.ToInteger();
}

SWF_TEXT_NATIVE_VAR_DEFINE_GET( textColor )
{
	SWF_TEXT_PTHIS_GET( "textColor" );

	int r = ( pThis->color.r << 16 );
	int g = ( pThis->color.g << 8 );
	int b = pThis->color.b;

	int textColor = r | g | b;

	return textColor;
}

SWF_TEXT_NATIVE_VAR_DEFINE_SET( textColor )
{
	SWF_TEXT_PTHIS_SET( "textColor" );

	int textColor = value.ToInteger();
	int r = ( textColor >> 16 ) & 0xFF;
	int g = ( textColor >> 8 ) & 0x00FF;
	int b = textColor & 0x0000FF;

	pThis->color.r = r;
	pThis->color.g = g;
	pThis->color.b = b;
}

SWF_TEXT_FUNCTION_DEFINE( clearTimingInfo )
{
	SWF_TEXT_PTHIS_FUNC( "clearTimingInfo" );
	pThis->subtitleTimingInfo.Clear();
	return idSWFScriptVar();
}


SWF_TEXT_FUNCTION_DEFINE( generateRnd )
{
	SWF_TEXT_PTHIS_FUNC( "generateRnd" );
	pThis->triggerGenerate = true;
	pThis->rndSpotsVisible = -1;
	pThis->generatingText = false;
	return idSWFScriptVar();
}

SWF_TEXT_FUNCTION_DEFINE( calcNumLines )
{
	SWF_TEXT_PTHIS_FUNC( "calcNumLines" );

	return pThis->CalcNumLines();
}

SWF_TEXT_FUNCTION_DEFINE( onKey )
{
	SWF_TEXT_PTHIS_FUNC( "onKey" );

	int keyCode = parms[0].ToInteger();
	bool keyDown = parms[1].ToBool();

	if( keyDown )
	{
		switch( keyCode )
		{
			case K_LSHIFT:
			case K_RSHIFT:
			{
				pThis->shiftHeld = true;
				break;
			}
			case K_BACKSPACE:
			case K_DEL:
			{
				if( pThis->selectionStart == pThis->selectionEnd )
				{
					if( keyCode == K_BACKSPACE )
					{
						pThis->selectionStart = pThis->selectionEnd - 1;
					}
					else
					{
						pThis->selectionEnd = pThis->selectionStart + 1;
					}
				}
				int start = Min( pThis->selectionStart, pThis->selectionEnd );
				int end = Max( pThis->selectionStart, pThis->selectionEnd );
				idStr left = pThis->text.Left( Max( start, 0 ) );
				idStr right = pThis->text.Right( Max( pThis->text.Length() - end, 0 ) );
				pThis->text = left + right;
				pThis->selectionStart = start;
				pThis->selectionEnd = start;
				break;
			}
			case K_LEFTARROW:
			{
				if( pThis->selectionEnd > 0 )
				{
					pThis->selectionEnd--;
					if( !pThis->shiftHeld )
					{
						pThis->selectionStart = pThis->selectionEnd;
					}
				}
				break;
			}
			case K_RIGHTARROW:
			{
				if( pThis->selectionEnd < pThis->text.Length() )
				{
					pThis->selectionEnd++;
					if( !pThis->shiftHeld )
					{
						pThis->selectionStart = pThis->selectionEnd;
					}
				}
				break;
			}
			case K_HOME:
			{
				pThis->selectionEnd = 0;
				if( !pThis->shiftHeld )
				{
					pThis->selectionStart = pThis->selectionEnd;
				}
				break;
			}
			case K_END:
			{
				pThis->selectionEnd = pThis->text.Length();
				if( !pThis->shiftHeld )
				{
					pThis->selectionStart = pThis->selectionEnd;
				}
				break;
			}
		}
	}
	else
	{
		if( keyCode == K_LSHIFT || keyCode == K_RSHIFT )
		{
			pThis->shiftHeld = false;
		}
	}
	return true;
}

SWF_TEXT_FUNCTION_DEFINE( onChar )
{
	SWF_TEXT_PTHIS_FUNC( "onChar" );

	int keyCode = parms[0].ToInteger();

	if( keyCode < 32 || keyCode == 127 )
	{
		return false;
	}

	char letter = ( char )keyCode;
	// assume ` is meant for the console
	if( letter == '`' )
	{
		return false;
	}
	if( pThis->selectionStart != pThis->selectionEnd )
	{
		int start = Min( pThis->selectionStart, pThis->selectionEnd );
		int end = Max( pThis->selectionStart, pThis->selectionEnd );
		idStr left = pThis->text.Left( Max( start, 0 ) );
		idStr right = pThis->text.Right( Max( pThis->text.Length() - end, 0 ) );
		pThis->text = left + right;
		pThis->selectionStart = start;

		pThis->text.Clear();
		pThis->text.Append( left );
		pThis->text.Append( letter );
		pThis->text.Append( right );
		pThis->selectionStart++;
	}
	else if( pThis->selectionStart < swf_textMaxInputLength.GetInteger() )
	{
		if( pThis->selectionStart < 0 )
		{
			pThis->selectionStart = 0;
		}
		pThis->text.Insert( letter, pThis->selectionStart++ );
	}
	pThis->selectionEnd = pThis->selectionStart;
	return true;
}

SWF_TEXT_NATIVE_VAR_DEFINE_GET( subtitle )
{
	SWF_TEXT_PTHIS_GET( "subtitle" );
	return pThis->isSubtitle;
}
SWF_TEXT_NATIVE_VAR_DEFINE_SET( subtitle )
{
	SWF_TEXT_PTHIS_SET( "subtitle" );
	pThis->isSubtitle = value.ToBool();
}
SWF_TEXT_NATIVE_VAR_DEFINE_GET( subtitleAlign )
{
	SWF_TEXT_PTHIS_GET( "subtitleAlign" );
	return pThis->subAlign;
}
SWF_TEXT_NATIVE_VAR_DEFINE_SET( subtitleAlign )
{
	SWF_TEXT_PTHIS_SET( "subtitleAlign" );
	pThis->subAlign = value.ToInteger();
}
SWF_TEXT_NATIVE_VAR_DEFINE_GET( subtitleSourceID )
{
	SWF_TEXT_PTHIS_GET( "subtitleSourceID" );
	return pThis->subSourceID;
}
SWF_TEXT_NATIVE_VAR_DEFINE_SET( subtitleSourceID )
{
	SWF_TEXT_PTHIS_SET( "subtitleSourceID" );
	pThis->subSourceID = value.ToInteger();
}
SWF_TEXT_NATIVE_VAR_DEFINE_GET( subtitleSpeaker )
{
	SWF_TEXT_PTHIS_GET( "subtitleSpeaker" );
	return pThis->subSpeaker.c_str();
}
SWF_TEXT_NATIVE_VAR_DEFINE_SET( subtitleSpeaker )
{
	SWF_TEXT_PTHIS_SET( "subtitleSpeaker" );
	pThis->subSpeaker = value.ToString();
}

SWF_TEXT_FUNCTION_DEFINE( subtitleLength )
{
	SWF_TEXT_PTHIS_FUNC( "subtitleLength" );
	pThis->subLength = parms[0].ToInteger();
	return idSWFScriptVar();
}

SWF_TEXT_FUNCTION_DEFINE( subtitleSourceCheck )
{
	SWF_TEXT_PTHIS_FUNC( "subtitleSourceCheck" );

	int idCheck = parms[0].ToInteger();

	if( pThis->subSourceID == -1 )
	{
		pThis->subSourceID = idCheck;
		return 1;
	}

	if( idCheck == pThis->subSourceID )    // || pThis->subForceKill ) {
	{
		pThis->SubtitleComplete();
		pThis->subSourceID = idCheck;
		return -1;
	}

	return 0;
}

SWF_TEXT_FUNCTION_DEFINE( subtitleStart )
{
	SWF_TEXT_PTHIS_FUNC( "subtitleStart" );
	pThis->subUpdating = true;
	pThis->subNeedsSwitch = false;
	pThis->subForceKillQueued = false;
	pThis->subForceKill = false;
	pThis->subKillTimeDelay = 0;
	// trickery to swap the text so subtitles don't show until they should
	pThis->subtitleText = pThis->text;
	pThis->text = "";
	pThis->subCharStartIndex = 0;
	pThis->subNextStartIndex = 0;
	pThis->subCharEndIndex = 0;
	pThis->subSwitchTime = 0;
	pThis->subLastWordIndex = 0;
	pThis->subPrevLastWordIndex = 0;
	pThis->subStartTime = -1;
	pThis->subInitialLine = true;
	return idSWFScriptVar();
}

SWF_TEXT_FUNCTION_DEFINE( forceKillSubtitle )
{
	SWF_TEXT_PTHIS_FUNC( "forceKillSubtitle" );
	pThis->subForceKill = true;
	pThis->subKillTimeDelay = 0;
	return idSWFScriptVar();
}

SWF_TEXT_FUNCTION_DEFINE( killSubtitle )
{
	SWF_TEXT_PTHIS_FUNC( "killSubtitle" );
	pThis->subForceKillQueued = true;
	//pThis->SubtitleComplete();
	return idSWFScriptVar();
}

SWF_TEXT_FUNCTION_DEFINE( terminateSubtitle )
{
	SWF_TEXT_PTHIS_FUNC( "terminateSubtitle" );
	pThis->SubtitleComplete();
	pThis->SubtitleCleanup();
	return idSWFScriptVar();
}

SWF_TEXT_FUNCTION_DEFINE( subLastLine )
{
	SWF_TEXT_PTHIS_FUNC( "subLastLine" );
	idStr lastLine;
	int len = pThis->subCharEndIndex - pThis->subCharStartIndex;
	pThis->text.Mid( pThis->subCharStartIndex, len, lastLine );
	return lastLine;
}

SWF_TEXT_FUNCTION_DEFINE( addSubtitleInfo )
{
	SWF_TEXT_PTHIS_FUNC( "addSubtitleInfo" );

	if( parms.Num() != 3 )
	{
		return idSWFScriptVar();
	}

	subTimingWordData_t info;
	info.phrase = parms[0].ToString();
	info.startTime = parms[1].ToInteger();
	info.forceBreak = parms[2].ToBool();

	pThis->subtitleTimingInfo.Append( info );
	return idSWFScriptVar();
}
