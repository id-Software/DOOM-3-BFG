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
#ifndef __SWF_ENUMS_H__
#define __SWF_ENUMS_H__

enum swfDictType_t
{
	SWF_DICT_NULL,
	SWF_DICT_IMAGE,
	SWF_DICT_SHAPE,
	SWF_DICT_MORPH,
	SWF_DICT_SPRITE,
	SWF_DICT_FONT,
	SWF_DICT_TEXT,
	SWF_DICT_EDITTEXT
};
enum swfTag_t
{
	Tag_End = 0,
	Tag_ShowFrame = 1,
	Tag_DefineShape = 2,
	Tag_PlaceObject = 4,
	Tag_RemoveObject = 5,
	Tag_DefineBits = 6,
	Tag_DefineButton = 7,
	Tag_JPEGTables = 8,
	Tag_SetBackgroundColor = 9,
	Tag_DefineFont = 10,
	Tag_DefineText = 11,
	Tag_DoAction = 12,
	Tag_DefineFontInfo = 13,
	Tag_DefineSound = 14,
	Tag_StartSound = 15,
	Tag_DefineButtonSound = 17,
	Tag_SoundStreamHead = 18,
	Tag_SoundStreamBlock = 19,
	Tag_DefineBitsLossless = 20,
	Tag_DefineBitsJPEG2 = 21,
	Tag_DefineShape2 = 22,
	Tag_DefineButtonCxform = 23,
	Tag_Protect = 24,
	Tag_PlaceObject2 = 26,
	Tag_RemoveObject2 = 28,
	Tag_DefineShape3 = 32,
	Tag_DefineText2 = 33,
	Tag_DefineButton2 = 34,
	Tag_DefineBitsJPEG3 = 35,
	Tag_DefineBitsLossless2 = 36,
	Tag_DefineEditText = 37,
	Tag_DefineSprite = 39,
	Tag_FrameLabel = 43,
	Tag_SoundStreamHead2 = 45,
	Tag_DefineMorphShape = 46,
	Tag_DefineFont2 = 48,
	Tag_ExportAssets = 56,
	Tag_ImportAssets = 57,
	Tag_EnableDebugger = 58,
	Tag_DoInitAction = 59,
	Tag_DefineVideoStream = 60,
	Tag_VideoFrame = 61,
	Tag_DefineFontInfo2 = 62,
	Tag_EnableDebugger2 = 64,
	Tag_ScriptLimits = 65,
	Tag_SetTabIndex = 66,
	Tag_FileAttributes = 69,
	Tag_PlaceObject3 = 70,
	Tag_ImportAssets2 = 71,
	Tag_DefineFontAlignZones = 73,
	Tag_CSMTextSettings = 74,
	Tag_DefineFont3 = 75,
	Tag_SymbolClass = 76,
	Tag_Metadata = 77,
	Tag_DefineScalingGrid = 78,
	Tag_DoABC = 82,
	Tag_DefineShape4 = 83,
	Tag_DefineMorphShape2 = 84,
	Tag_DefineSceneAndFrameLabelData = 86,
	Tag_DefineBinaryData = 87,
	Tag_DefineFontName = 88,
	Tag_StartSound2 = 89
};
enum swfAction_t
{
	Action_End = 0,

	// swf 3
	Action_NextFrame = 0x04,
	Action_PrevFrame = 0x05,
	Action_Play = 0x06,
	Action_Stop = 0x07,
	Action_ToggleQuality = 0x08,
	Action_StopSounds = 0x09,

	Action_GotoFrame = 0x81,
	Action_GetURL = 0x83,
	Action_WaitForFrame = 0x8A,
	Action_SetTarget = 0x8B,
	Action_GoToLabel = 0x8C,

	// swf 4
	Action_Add = 0x0A,
	Action_Subtract = 0x0B,
	Action_Multiply = 0x0C,
	Action_Divide = 0x0D,
	Action_Equals = 0x0E,
	Action_Less = 0x0F,
	Action_And = 0x10,
	Action_Or = 0x11,
	Action_Not = 0x12,
	Action_StringEquals = 0x13,
	Action_StringLength = 0x14,
	Action_StringExtract = 0x15,
	Action_Pop = 0x17,
	Action_ToInteger = 0x18,
	Action_GetVariable = 0x1C,
	Action_SetVariable = 0x1D,
	Action_SetTarget2 = 0x20,
	Action_StringAdd = 0x21,
	Action_GetProperty = 0x22,
	Action_SetProperty = 0x23,
	Action_CloneSprite = 0x24,
	Action_RemoveSprite = 0x25,
	Action_Trace = 0x26,
	Action_StartDrag = 0x27,
	Action_EndDrag = 0x28,
	Action_StringLess = 0x29,
	Action_RandomNumber = 0x30,
	Action_MBStringLength = 0x31,
	Action_CharToAscii = 0x32,
	Action_AsciiToChar = 0x33,
	Action_GetTime = 0x34,
	Action_MBStringExtract = 0x35,
	Action_MBCharToAscii = 0x36,
	Action_MBAsciiToChar = 0x37,

	Action_WaitForFrame2 = 0x8D,
	Action_Push = 0x96,
	Action_Jump = 0x99,
	Action_GetURL2 = 0x9A,
	Action_If = 0x9D,
	Action_Call = 0x9E,
	Action_GotoFrame2 = 0x9F,

	// swf 5
	Action_Delete = 0x3A,
	Action_Delete2 = 0x3B,
	Action_DefineLocal = 0x3C,
	Action_CallFunction = 0x3D,
	Action_Return = 0x3E,
	Action_Modulo = 0x3F,
	Action_NewObject = 0x40,
	Action_DefineLocal2 = 0x41,
	Action_InitArray = 0x42,
	Action_InitObject = 0x43,
	Action_TypeOf = 0x44,
	Action_TargetPath = 0x45,
	Action_Enumerate = 0x46,
	Action_Add2 = 0x47,
	Action_Less2 = 0x48,
	Action_Equals2 = 0x49,
	Action_ToNumber = 0x4A,
	Action_ToString = 0x4B,
	Action_PushDuplicate = 0x4C,
	Action_StackSwap = 0x4D,
	Action_GetMember = 0x4E,
	Action_SetMember = 0x4F,
	Action_Increment = 0x50,
	Action_Decrement = 0x51,
	Action_CallMethod = 0x52,
	Action_NewMethod = 0x53,
	Action_BitAnd = 0x60,
	Action_BitOr = 0x61,
	Action_BitXor = 0x62,
	Action_BitLShift = 0x63,
	Action_BitRShift = 0x64,
	Action_BitURShift = 0x65,

	Action_StoreRegister = 0x87,
	Action_ConstantPool = 0x88,
	Action_With = 0x94,
	Action_DefineFunction = 0x9B,

	// swf 6
	Action_InstanceOf = 0x54,
	Action_Enumerate2 = 0x55,
	Action_StrictEquals = 0x66,
	Action_Greater = 0x67,
	Action_StringGreater = 0x68,

	// swf 7
	Action_Extends = 0x69,
	Action_CastOp = 0x2B,
	Action_ImplementsOp = 0x2C,
	Action_Throw = 0x2A,
	Action_Try = 0x8F,

	Action_DefineFunction2 = 0x8E,
};

#endif // !__SWF_ENUMS_H__
