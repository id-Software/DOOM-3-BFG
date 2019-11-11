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
#ifndef __SWF_SPRITEINSTANCE_H__
#define __SWF_SPRITEINSTANCE_H__

// RB: moved here
#define PlaceFlagHasClipActions		BIT( 7 )
#define PlaceFlagHasClipDepth		BIT( 6 )
#define PlaceFlagHasName			BIT( 5 )
#define PlaceFlagHasRatio			BIT( 4 )
#define PlaceFlagHasColorTransform	BIT( 3 )
#define PlaceFlagHasMatrix			BIT( 2 )
#define PlaceFlagHasCharacter		BIT( 1 )
#define PlaceFlagMove				BIT( 0 )

#define PlaceFlagPad0				BIT( 7 )
#define PlaceFlagPad1				BIT( 6 )
#define PlaceFlagPad2				BIT( 5 )
#define PlaceFlagHasImage			BIT( 4 )
#define PlaceFlagHasClassName		BIT( 3 )
#define PlaceFlagCacheAsBitmap		BIT( 2 )
#define PlaceFlagHasBlendMode		BIT( 1 )
#define PlaceFlagHasFilterList		BIT( 0 )
// RB end

/*
================================================
There can be multiple instances of a single sprite running
================================================
*/
class idSWFSpriteInstance
{
public:
	idSWFSpriteInstance();
	~idSWFSpriteInstance();

	void	Init( idSWFSprite* sprite, idSWFSpriteInstance* parent, int depth );

	bool	Run();
	bool	RunActions();

	const char* GetName() const
	{
		return name.c_str();
	}

	idSWFScriptObject* GetScriptObject()
	{
		return scriptObject;
	}
	void SetAlignment( float x, float y )
	{
		xOffset = x;
		yOffset = y;
	}

	void SetMaterial( const idMaterial* material, int width = -1, int height = -1 );
	void SetVisible( bool visible );
	bool IsVisible()
	{
		return isVisible;
	}
	void PlayFrame( const idSWFParmList& parms );
	void PlayFrame( const char* frameName )
	{
		idSWFParmList parms;
		parms.Append( frameName );
		PlayFrame( parms );
	}
	void PlayFrame( const int frameNum )
	{
		idSWFParmList parms;
		parms.Append( frameNum );
		PlayFrame( parms );
	}
	void StopFrame( const idSWFParmList& parms );
	void StopFrame( const char* frameName )
	{
		idSWFParmList parms;
		parms.Append( frameName );
		StopFrame( parms );
	}
	void StopFrame( const int frameNum )
	{
		idSWFParmList parms;
		parms.Append( frameNum );
		StopFrame( parms );
	}
	// FIXME: Why do all the Set functions have defaults of -1.0f?  This seems arbitrar.
	// Probably better to not have a default at all, so any non-parametized calls throw a
	// compilation error.
	float GetXPos() const;
	float GetYPos( bool overallPos = false ) const;
	void SetXPos( float xPos = -1.0f );
	void SetYPos( float yPos = -1.0f );
	void SetPos( float xPos = -1.0f, float yPos = -1.0f );
	void SetAlpha( float val );
	void SetScale( float x = -1.0f, float y = -1.0f );
	void SetMoveToScale( float x = -1.0f, float y = -1.0f );
	bool UpdateMoveToScale( float speed );	// returns true if the update was successful
	void SetRotation( float rot );
	uint16 GetCurrentFrame()
	{
		return currentFrame;
	}
	bool IsPlaying() const
	{
		return isPlaying;
	}
	int GetStereoDepth()
	{
		return stereoDepth;
	}

	// Removing the private access control statement due to cl 214702
	// Apparently MS's C++ compiler supports the newer C++ standard, and GCC supports C++03
	// In the new C++ standard, nested members of a friend class have access to private/protected members of the class granting friendship
	// In C++03, nested members defined in a friend class do NOT have access to private/protected members of the class granting friendship	friend class idSWF;

	bool isPlaying;
	bool isVisible;
	bool childrenRunning;
	bool firstRun;

	// currentFrame is the frame number currently in the displayList
	// we use 1 based frame numbers because currentFrame = 0 means nothing is in the display list
	// it's also convenient because Flash also uses 1 based frame numbers
	uint16	currentFrame;
	uint16	frameCount;

	// the sprite this is an instance of
	idSWFSprite* sprite;

	// sprite instances can be nested
	idSWFSpriteInstance* parent;

	// depth of this sprite instance in the parent's display list
	int depth;

	// if this is set, apply this material when rendering any child shapes
	int itemIndex;

	const idMaterial* materialOverride;
	uint16 materialWidth;
	uint16 materialHeight;

	float xOffset;
	float yOffset;

	float moveToXScale;
	float moveToYScale;
	float moveToSpeed;

	int stereoDepth;

	idSWFScriptObject* scriptObject;

	// children display entries
	idList< swfDisplayEntry_t, TAG_SWF > displayList;
	swfDisplayEntry_t* FindDisplayEntry( int depth );

	// name of this sprite instance
	idStr name;

	struct swfAction_t
	{
		const byte* data;
		uint32 dataLength;
	};
	idList< swfAction_t, TAG_SWF > actions;

	idSWFScriptFunction_Script* actionScript;

	idSWFScriptVar onEnterFrame;
	//idSWFScriptVar onLoad;

	// Removing the private access control statement due to cl 214702
	// Apparently MS's C++ compiler supports the newer C++ standard, and GCC supports C++03
	// In the new C++ standard, nested members of a friend class have access to private/protected members of the class granting friendship
	// In C++03, nested members defined in a friend class do NOT have access to private/protected members of the class granting friendship

	//----------------------------------
	// SWF_PlaceObject.cpp
	//----------------------------------
	void			PlaceObject2( idSWFBitStream& bitstream );
	void			PlaceObject3( idSWFBitStream& bitstream );
	void			RemoveObject2( idSWFBitStream& bitstream );

	//----------------------------------
	// SWF_Sounds.cpp
	//----------------------------------
	void			StartSound( idSWFBitStream& bitstream );

	//----------------------------------
	// SWF_SpriteInstance.cpp
	//----------------------------------
	void			NextFrame();
	void			PrevFrame();
	void			RunTo( int frameNum );

	void			Play();
	void			Stop();

	void					FreeDisplayList();
	swfDisplayEntry_t* 		AddDisplayEntry( int depth, int characterID );
	void					RemoveDisplayEntry( int depth );
	void					SwapDepths( int depth1, int depth2 );

	void					DoAction( idSWFBitStream& bitstream );

	idSWFSpriteInstance* 	FindChildSprite( const char* childName );
	idSWFSpriteInstance* 	ResolveTarget( const char* targetName );
	uint32					FindFrame( const char* frameLabel ) const;
	bool					FrameExists( const char* frameLabel ) const;
	bool					IsBetweenFrames( const char* frameLabel1, const char* frameLabel2 ) const;
};

/*
================================================
This is the prototype object that all the sprite instance script objects reference
================================================
*/
class idSWFScriptObject_SpriteInstancePrototype : public idSWFScriptObject
{
public:
	idSWFScriptObject_SpriteInstancePrototype();

#define SWF_SPRITE_FUNCTION_DECLARE( x ) \
	class idSWFScriptFunction_##x : public idSWFScriptFunction { \
	public: \
		void			AddRef() {} \
		void			Release() {} \
		idSWFScriptVar Call( idSWFScriptObject * thisObject, const idSWFParmList & parms ); \
	} scriptFunction_##x

	SWF_SPRITE_FUNCTION_DECLARE( duplicateMovieClip );
	SWF_SPRITE_FUNCTION_DECLARE( gotoAndPlay );
	SWF_SPRITE_FUNCTION_DECLARE( gotoAndStop );
	SWF_SPRITE_FUNCTION_DECLARE( swapDepths );
	SWF_SPRITE_FUNCTION_DECLARE( nextFrame );
	SWF_SPRITE_FUNCTION_DECLARE( prevFrame );
	SWF_SPRITE_FUNCTION_DECLARE( play );
	SWF_SPRITE_FUNCTION_DECLARE( stop );

	SWF_NATIVE_VAR_DECLARE( _x );
	SWF_NATIVE_VAR_DECLARE( _y );
	SWF_NATIVE_VAR_DECLARE( _xscale );
	SWF_NATIVE_VAR_DECLARE( _yscale );
	SWF_NATIVE_VAR_DECLARE( _alpha );
	SWF_NATIVE_VAR_DECLARE( _brightness );
	SWF_NATIVE_VAR_DECLARE( _visible );
	SWF_NATIVE_VAR_DECLARE( _width );
	SWF_NATIVE_VAR_DECLARE( _height );
	SWF_NATIVE_VAR_DECLARE( _rotation );

	SWF_NATIVE_VAR_DECLARE_READONLY( _name );
	SWF_NATIVE_VAR_DECLARE_READONLY( _currentframe );
	SWF_NATIVE_VAR_DECLARE_READONLY( _totalframes );
	SWF_NATIVE_VAR_DECLARE_READONLY( _target );
	SWF_NATIVE_VAR_DECLARE_READONLY( _framesloaded );
	SWF_NATIVE_VAR_DECLARE_READONLY( _droptarget );
	SWF_NATIVE_VAR_DECLARE_READONLY( _url );
	SWF_NATIVE_VAR_DECLARE_READONLY( _highquality );
	SWF_NATIVE_VAR_DECLARE_READONLY( _focusrect );
	SWF_NATIVE_VAR_DECLARE_READONLY( _soundbuftime );
	SWF_NATIVE_VAR_DECLARE_READONLY( _quality );
	SWF_NATIVE_VAR_DECLARE_READONLY( _mousex );
	SWF_NATIVE_VAR_DECLARE_READONLY( _mousey );

	SWF_NATIVE_VAR_DECLARE( _stereoDepth );
	SWF_NATIVE_VAR_DECLARE( _itemindex );

	SWF_NATIVE_VAR_DECLARE( material );
	SWF_NATIVE_VAR_DECLARE( materialWidth );
	SWF_NATIVE_VAR_DECLARE( materialHeight );

	SWF_NATIVE_VAR_DECLARE( xOffset );
	SWF_NATIVE_VAR_DECLARE( onEnterFrame );
	//SWF_NATIVE_VAR_DECLARE( onLoad );
};

extern idSWFScriptObject_SpriteInstancePrototype spriteInstanceScriptObjectPrototype;

#endif
