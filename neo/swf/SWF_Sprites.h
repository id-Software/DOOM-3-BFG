/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2015 Robert Beckebans

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
#ifndef __SWF_SPRITES_H__
#define __SWF_SPRITES_H__

#undef Bool
#include "rapidjson/document.h"

/*
================================================
What the swf file format calls a "sprite" is known as a "movie clip" in Flash
There is one main sprite, and many many sub-sprites
Only the main sprite is allowed to add things to the dictionary
================================================
*/
class idSWFSprite
{
public:
	idSWFSprite( class idSWF* swf );
	~idSWFSprite();

	void	Load( idSWFBitStream& bitstream, bool parseDictionary );

	void	Read( idFile* f );
	void	Write( idFile* f );

	// RB begin
	void	ReadJSON( rapidjson::Value& entry );

	void	WriteJSON( idFile* f, int characterID );
	void	WriteJSON_PlaceObject2( idFile* f, idSWFBitStream& bitstream, int characterID, int commandID, const char* indentPrefix = "" );
	void	WriteJSON_PlaceObject3( idFile* f, idSWFBitStream& bitstream, int characterID, int commandID, const char* indentPrefix = "" );
	void	WriteJSON_RemoveObject2( idFile* f, idSWFBitStream& bitstream, int characterID, int commandID, const char* indentPrefix = "" );
	void	WriteJSON_DoAction( idFile* f, idSWFBitStream& bitstream, int characterID, int commandID, const char* indentPrefix = "" );

	void	WriteSWF( idFile_SWF& f, int characterID );

	uint16	GetFrameCount()
	{
		return frameCount;
	}
	// RB end

	class idSWF* GetSWF()
	{
		return swf;
	}

private:
	friend class idSWFSpriteInstance;
	friend class idSWFScriptFunction_Script;

	class idSWF* swf;	// this is required so things can access the dictionary, it would be kind of nice if we just had an idSWFDictionary pointer instead

	uint16	frameCount;

	// frameOffsets contains offsets into the commands list for each frame
	// the first command for frame 3 is frameOffsets[2] and the last command is frameOffsets[3]
	idList< uint32, TAG_SWF >	frameOffsets;

	struct swfFrameLabel_t
	{
		idStr frameLabel;
		uint32 frameNum;
	};
	idList< swfFrameLabel_t, TAG_SWF > frameLabels;

	struct swfSpriteCommand_t
	{
		swfTag_t		tag;
		idSWFBitStream	stream;
	};
	idList< swfSpriteCommand_t, TAG_SWF > commands;

	//// [ES-BrianBugh 1/16/10] - There can be multiple DoInitAction tags, and all need to be executed.
	idList<idSWFBitStream, TAG_SWF> doInitActions;

	byte* commandBuffer;

};

#endif // !__SWF_SPRITES_H__
