/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013 Robert Beckebans

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

// RB begin
const char* idSWF::GetDictTypeName( swfDictType_t type )
{
#define SWF_DICT_NAME( x ) case SWF_DICT_##x: return #x;
	switch( type )
	{
			SWF_DICT_NAME( NULL );
			SWF_DICT_NAME( IMAGE );
			SWF_DICT_NAME( SHAPE );
			SWF_DICT_NAME( MORPH );
			SWF_DICT_NAME( SPRITE );
			SWF_DICT_NAME( FONT );
			SWF_DICT_NAME( TEXT );
			SWF_DICT_NAME( EDITTEXT );
		default:
			return "????";
	}
}

const char* idSWF::GetEditTextAlignName( swfEditTextAlign_t align )
{
#define SWF_ET_ALIGN_NAME( x ) case SWF_ET_ALIGN_##x: return #x;
	switch( align )
	{
			SWF_ET_ALIGN_NAME( LEFT );
			SWF_ET_ALIGN_NAME( RIGHT );
			SWF_ET_ALIGN_NAME( CENTER );
			SWF_ET_ALIGN_NAME( JUSTIFY );
		default:
			return "????";
	}
}
// RB end

/*
========================
idSWF::GetTagName
========================
*/
const char* idSWF::GetTagName( swfTag_t tag )
{
#define SWF_TAG_NAME( x ) case Tag_##x: return #x;
	switch( tag )
	{
			SWF_TAG_NAME( End );
			SWF_TAG_NAME( ShowFrame );
			SWF_TAG_NAME( DefineShape );
			SWF_TAG_NAME( PlaceObject );
			SWF_TAG_NAME( RemoveObject );
			SWF_TAG_NAME( DefineBits );
			SWF_TAG_NAME( DefineButton );
			SWF_TAG_NAME( JPEGTables );
			SWF_TAG_NAME( SetBackgroundColor );
			SWF_TAG_NAME( DefineFont );
			SWF_TAG_NAME( DefineText );
			SWF_TAG_NAME( DoAction );
			SWF_TAG_NAME( DefineFontInfo );
			SWF_TAG_NAME( DefineSound );
			SWF_TAG_NAME( StartSound );
			SWF_TAG_NAME( DefineButtonSound );
			SWF_TAG_NAME( SoundStreamHead );
			SWF_TAG_NAME( SoundStreamBlock );
			SWF_TAG_NAME( DefineBitsLossless );
			SWF_TAG_NAME( DefineBitsJPEG2 );
			SWF_TAG_NAME( DefineShape2 );
			SWF_TAG_NAME( DefineButtonCxform );
			SWF_TAG_NAME( Protect );
			SWF_TAG_NAME( PlaceObject2 );
			SWF_TAG_NAME( RemoveObject2 );
			SWF_TAG_NAME( DefineShape3 );
			SWF_TAG_NAME( DefineText2 );
			SWF_TAG_NAME( DefineButton2 );
			SWF_TAG_NAME( DefineBitsJPEG3 );
			SWF_TAG_NAME( DefineBitsLossless2 );
			SWF_TAG_NAME( DefineEditText );
			SWF_TAG_NAME( DefineSprite );
			SWF_TAG_NAME( FrameLabel );
			SWF_TAG_NAME( SoundStreamHead2 );
			SWF_TAG_NAME( DefineMorphShape );
			SWF_TAG_NAME( DefineFont2 );
			SWF_TAG_NAME( ExportAssets );
			SWF_TAG_NAME( ImportAssets );
			SWF_TAG_NAME( EnableDebugger );
			SWF_TAG_NAME( DoInitAction );
			SWF_TAG_NAME( DefineVideoStream );
			SWF_TAG_NAME( VideoFrame );
			SWF_TAG_NAME( DefineFontInfo2 );
			SWF_TAG_NAME( EnableDebugger2 );
			SWF_TAG_NAME( ScriptLimits );
			SWF_TAG_NAME( SetTabIndex );
			SWF_TAG_NAME( FileAttributes );
			SWF_TAG_NAME( PlaceObject3 );
			SWF_TAG_NAME( ImportAssets2 );
			SWF_TAG_NAME( DefineFontAlignZones );
			SWF_TAG_NAME( CSMTextSettings );
			SWF_TAG_NAME( DefineFont3 );
			SWF_TAG_NAME( SymbolClass );
			SWF_TAG_NAME( Metadata );
			SWF_TAG_NAME( DefineScalingGrid );
			SWF_TAG_NAME( DoABC );
			SWF_TAG_NAME( DefineShape4 );
			SWF_TAG_NAME( DefineMorphShape2 );
			SWF_TAG_NAME( DefineSceneAndFrameLabelData );
			SWF_TAG_NAME( DefineBinaryData );
			SWF_TAG_NAME( DefineFontName );
			SWF_TAG_NAME( StartSound2 );
		default:
			return "????";
	}
}

/*
========================
idSWF::GetActionName
========================
*/
const char* idSWF::GetActionName( swfAction_t action )
{
#define SWF_ACTION_NAME( x ) case Action_##x: return #x;
	switch( action )
	{
			SWF_ACTION_NAME( NextFrame );
			SWF_ACTION_NAME( PrevFrame );
			SWF_ACTION_NAME( Play );
			SWF_ACTION_NAME( Stop );
			SWF_ACTION_NAME( ToggleQuality );
			SWF_ACTION_NAME( StopSounds );
			SWF_ACTION_NAME( GotoFrame );
			SWF_ACTION_NAME( GetURL );
			SWF_ACTION_NAME( WaitForFrame );
			SWF_ACTION_NAME( SetTarget );
			SWF_ACTION_NAME( GoToLabel );
			SWF_ACTION_NAME( Add );
			SWF_ACTION_NAME( Subtract );
			SWF_ACTION_NAME( Multiply );
			SWF_ACTION_NAME( Divide );
			SWF_ACTION_NAME( Equals );
			SWF_ACTION_NAME( Less );
			SWF_ACTION_NAME( And );
			SWF_ACTION_NAME( Or );
			SWF_ACTION_NAME( Not );
			SWF_ACTION_NAME( StringEquals );
			SWF_ACTION_NAME( StringLength );
			SWF_ACTION_NAME( StringExtract );
			SWF_ACTION_NAME( Pop );
			SWF_ACTION_NAME( ToInteger );
			SWF_ACTION_NAME( GetVariable );
			SWF_ACTION_NAME( SetVariable );
			SWF_ACTION_NAME( SetTarget2 );
			SWF_ACTION_NAME( StringAdd );
			SWF_ACTION_NAME( GetProperty );
			SWF_ACTION_NAME( SetProperty );
			SWF_ACTION_NAME( CloneSprite );
			SWF_ACTION_NAME( RemoveSprite );
			SWF_ACTION_NAME( Trace );
			SWF_ACTION_NAME( StartDrag );
			SWF_ACTION_NAME( EndDrag );
			SWF_ACTION_NAME( StringLess );
			SWF_ACTION_NAME( RandomNumber );
			SWF_ACTION_NAME( MBStringLength );
			SWF_ACTION_NAME( CharToAscii );
			SWF_ACTION_NAME( AsciiToChar );
			SWF_ACTION_NAME( GetTime );
			SWF_ACTION_NAME( MBStringExtract );
			SWF_ACTION_NAME( MBCharToAscii );
			SWF_ACTION_NAME( MBAsciiToChar );
			SWF_ACTION_NAME( WaitForFrame2 );
			SWF_ACTION_NAME( Push );
			SWF_ACTION_NAME( Jump );
			SWF_ACTION_NAME( GetURL2 );
			SWF_ACTION_NAME( If );
			SWF_ACTION_NAME( Call );
			SWF_ACTION_NAME( GotoFrame2 );
			SWF_ACTION_NAME( Delete );
			SWF_ACTION_NAME( Delete2 );
			SWF_ACTION_NAME( DefineLocal );
			SWF_ACTION_NAME( CallFunction );
			SWF_ACTION_NAME( Return );
			SWF_ACTION_NAME( Modulo );
			SWF_ACTION_NAME( NewObject );
			SWF_ACTION_NAME( DefineLocal2 );
			SWF_ACTION_NAME( InitArray );
			SWF_ACTION_NAME( InitObject );
			SWF_ACTION_NAME( TypeOf );
			SWF_ACTION_NAME( TargetPath );
			SWF_ACTION_NAME( Enumerate );
			SWF_ACTION_NAME( Add2 );
			SWF_ACTION_NAME( Less2 );
			SWF_ACTION_NAME( Equals2 );
			SWF_ACTION_NAME( ToNumber );
			SWF_ACTION_NAME( ToString );
			SWF_ACTION_NAME( PushDuplicate );
			SWF_ACTION_NAME( StackSwap );
			SWF_ACTION_NAME( GetMember );
			SWF_ACTION_NAME( SetMember );
			SWF_ACTION_NAME( Increment );
			SWF_ACTION_NAME( Decrement );
			SWF_ACTION_NAME( CallMethod );
			SWF_ACTION_NAME( NewMethod );
			SWF_ACTION_NAME( BitAnd );
			SWF_ACTION_NAME( BitOr );
			SWF_ACTION_NAME( BitXor );
			SWF_ACTION_NAME( BitLShift );
			SWF_ACTION_NAME( BitRShift );
			SWF_ACTION_NAME( BitURShift );
			SWF_ACTION_NAME( StoreRegister );
			SWF_ACTION_NAME( ConstantPool );
			SWF_ACTION_NAME( With );
			SWF_ACTION_NAME( DefineFunction );
			SWF_ACTION_NAME( InstanceOf );
			SWF_ACTION_NAME( Enumerate2 );
			SWF_ACTION_NAME( StrictEquals );
			SWF_ACTION_NAME( Greater );
			SWF_ACTION_NAME( StringGreater );
			SWF_ACTION_NAME( Extends );
			SWF_ACTION_NAME( CastOp );
			SWF_ACTION_NAME( ImplementsOp );
			SWF_ACTION_NAME( Throw );
			SWF_ACTION_NAME( Try );
			SWF_ACTION_NAME( DefineFunction2 );
		default:
			return "???";
	}
}

