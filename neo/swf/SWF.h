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
#ifndef __SWF_H__
#define __SWF_H__

#include "SWF_Enums.h"
#include "SWF_Types.h"
#include "SWF_Bitstream.h"
// RB begin
#include "SWF_File.h"
// RB end
#include "SWF_ScriptVar.h"
#include "SWF_Sprites.h"
#include "SWF_ScriptObject.h"
#include "SWF_ParmList.h"
#include "SWF_ScriptFunction.h"
#include "SWF_SpriteInstance.h"
#include "SWF_ShapeParser.h"
#include "SWF_TextInstance.h"

class idSWFDictionaryEntry
{
public:
	idSWFDictionaryEntry();
	~idSWFDictionaryEntry();
	idSWFDictionaryEntry& operator=( idSWFDictionaryEntry& other );
	idSWFDictionaryEntry& operator=( idSWFDictionaryEntry&& other );

	swfDictType_t		type;
	const idMaterial* 	material;
	idSWFShape* 		shape;
	idSWFSprite* 		sprite;
	idSWFFont* 			font;
	idSWFText* 			text;
	idSWFEditText* 		edittext;

	idVec2i				imageSize;
	idVec2i				imageAtlasOffset;
	// the compressed images are normalize to reduce compression artifacts,
	// color must be scaled down by this
	idVec4				channelScale;
};

struct purgableSwfImage_t
{
	purgableSwfImage_t()
	{
		image = NULL;
		swfFrameNum = 0;
	}
	idImage* image;
	unsigned swfFrameNum;
};

/*
================================================
This class handles loading and rendering SWF files
================================================
*/
class idSWF
{
public:
	idSWF( const char* filename, idSoundWorld* soundWorld = NULL );
	~idSWF();

	bool	IsLoaded()
	{
		return ( frameRate > 0 );
	}
	bool	IsActive()
	{
		return isActive;
	}
	void	Activate( bool b );

	const char* GetName()
	{
		return filename;
	}

	void Pause()
	{
		mainspriteInstance->Stop();
		paused = true;
	}
	void Resume()
	{
		mainspriteInstance->Play();
		paused = false;
	}
	bool IsPaused()
	{
		return paused;
	}
	void SetPausedRender( bool valid )
	{
		pausedRender = valid;
	}
	bool GetPausedRender()
	{
		return pausedRender;
	}

	void Render( idRenderSystem* gui, int time = 0, bool isSplitscreen = false );
	bool HandleEvent( const sysEvent_t* event );
	bool InhibitControl();
	void ForceInhibitControl( bool val )
	{
		inhibitControl = val;
	}

	void SetGlobal( const char* name, const idSWFScriptVar& value )
	{
		globals->Set( name, value );
	}
	void SetGlobalNative( const char* name, idSWFScriptNativeVariable* native )
	{
		globals->SetNative( name, native );
	}
	idSWFScriptVar GetGlobal( const char* name )
	{
		return globals->Get( name );
	}
	idSWFScriptObject& GetRootObject()
	{
		assert( mainspriteInstance->GetScriptObject() != NULL );
		return *( mainspriteInstance->GetScriptObject() );
	}

	void Invoke( const char*   functionName, const idSWFParmList& parms );
	void Invoke( const char*   functionName, const idSWFParmList& parms, idSWFScriptVar& scriptVar );
	void Invoke( const char*   functionName, const idSWFParmList& parms, bool& functionExists );

	int PlaySound( const char* sound, int channel = SCHANNEL_ANY, bool blocking = false );
	void StopSound( int channel = SCHANNEL_ANY );

	float GetFrameWidth() const
	{
		return frameWidth;
	}
	float GetFrameHeight() const
	{
		return frameHeight;
	}

	int GetMouseX()
	{
		return mouseX;
	}
	int GetMouseY()
	{
		return mouseY;
	}

	bool UseCircleForAccept();

	void SetSWFScale( float scale )
	{
		swfScale = scale;
	}

	void SetForceNonPCGetPlatform()
	{
		forceNonPCPlatform = true;
	}

	idRandom2& GetRandom()
	{
		return random;
	}

	int	GetPlatform();

	//----------------------------------
	// SWF_Dictionary.cpp
	//----------------------------------
	idSWFDictionaryEntry* 	AddDictionaryEntry( int characterID, swfDictType_t type );
	idSWFDictionaryEntry* 	FindDictionaryEntry( int characterID, swfDictType_t type );
	idSWFDictionaryEntry* 	FindDictionaryEntry( int characterID );

	idSWFDictionaryEntry* 	GetDictionaryEntry( int index )
	{
		return &dictionary[ index ];
	}
	int	GetNumDictionaryEntry()
	{
		return dictionary.Num();
	}

	idSWFScriptObject* HitTest( idSWFSpriteInstance* spriteInstance, const swfRenderState_t& renderState, int x, int y, idSWFScriptObject* parentObject );

private:
	idStr			filename;
	ID_TIME_T		timestamp;

	float			frameWidth;
	float			frameHeight;
	uint16			frameRate;
	float			renderBorder;
	float			swfScale;

	idVec2			scaleToVirtual;

	int				lastRenderTime;

	bool			isActive;
	bool			inhibitControl;
	bool			useInhibtControl;

	// certain screens need to be rendered when the pause menu is up so if this flag is
	// set on the gui we will allow it to render at a paused state;
	bool			pausedRender;

	bool			mouseEnabled;
	bool			useMouse;

	bool			blackbars;
	bool			crop;
	bool			paused;
	bool			hasHitObject;

	bool			forceNonPCPlatform;

	idRandom2		random;

	static int		mouseX;		// mouse x coord for all flash files
	static int		mouseY;		// mouse y coord for all flash files
	static bool		isMouseInClientArea;

	idSWFScriptObject* 	mouseObject;
	idSWFScriptObject* hoverObject;

	idSWFSprite* 			mainsprite;
	idSWFSpriteInstance* 	mainspriteInstance;

	idSWFScriptObject* 		globals;
	idSWFScriptObject* 		shortcutKeys;

	idSoundWorld* 			soundWorld;

	const idMaterial* 		atlasMaterial;

	idBlockAlloc< idSWFSpriteInstance, 16 >	spriteInstanceAllocator;
	idBlockAlloc< idSWFTextInstance, 16 >	textInstanceAllocator;

#define SWF_NATIVE_FUNCTION_SWF_DECLARE( x ) \
	class idSWFScriptFunction_##x : public idSWFScriptFunction_Nested< idSWF > { \
	public: \
		idSWFScriptVar Call( idSWFScriptObject * thisObject, const idSWFParmList & parms ); \
	} scriptFunction_##x;

	SWF_NATIVE_FUNCTION_SWF_DECLARE( shortcutKeys_clear );
	SWF_NATIVE_FUNCTION_SWF_DECLARE( deactivate );
	SWF_NATIVE_FUNCTION_SWF_DECLARE( inhibitControl );
	SWF_NATIVE_FUNCTION_SWF_DECLARE( useInhibit );
	SWF_NATIVE_FUNCTION_SWF_DECLARE( precacheSound );
	SWF_NATIVE_FUNCTION_SWF_DECLARE( playSound );
	SWF_NATIVE_FUNCTION_SWF_DECLARE( stopSounds );
	SWF_NATIVE_FUNCTION_SWF_DECLARE( getPlatform );
	SWF_NATIVE_FUNCTION_SWF_DECLARE( getTruePlatform );
	SWF_NATIVE_FUNCTION_SWF_DECLARE( getLocalString );
	SWF_NATIVE_FUNCTION_SWF_DECLARE( swapPS3Buttons );
	SWF_NATIVE_FUNCTION_SWF_DECLARE( getCVarInteger );
	SWF_NATIVE_FUNCTION_SWF_DECLARE( setCVarInteger );
	SWF_NATIVE_FUNCTION_SWF_DECLARE( strReplace );

	SWF_NATIVE_FUNCTION_SWF_DECLARE( acos );
	SWF_NATIVE_FUNCTION_SWF_DECLARE( cos );
	SWF_NATIVE_FUNCTION_SWF_DECLARE( sin );
	SWF_NATIVE_FUNCTION_SWF_DECLARE( round );
	SWF_NATIVE_FUNCTION_SWF_DECLARE( pow );
	SWF_NATIVE_FUNCTION_SWF_DECLARE( sqrt );
	SWF_NATIVE_FUNCTION_SWF_DECLARE( abs );
	SWF_NATIVE_FUNCTION_SWF_DECLARE( rand );
	SWF_NATIVE_FUNCTION_SWF_DECLARE( floor );
	SWF_NATIVE_FUNCTION_SWF_DECLARE( ceil );

	SWF_NATIVE_FUNCTION_SWF_DECLARE( toUpper );

	SWF_NATIVE_VAR_DECLARE_NESTED_READONLY( platform, idSWFScriptFunction_getPlatform, Call( object, idSWFParmList() ) );
	SWF_NATIVE_VAR_DECLARE_NESTED( blackbars, idSWF );
	SWF_NATIVE_VAR_DECLARE_NESTED( crop, idSWF );

	class idSWFScriptFunction_Object : public idSWFScriptFunction
	{
	public:
		idSWFScriptVar	Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
		{
			return idSWFScriptVar();
		}
		void			AddRef() { }
		void			Release() { }
		idSWFScriptObject* GetPrototype()
		{
			return &object;
		}
		void			SetPrototype( idSWFScriptObject* _object )
		{
			assert( false );
		}
		idSWFScriptObject object;
	} scriptFunction_Object;

	idList< idSWFDictionaryEntry, TAG_SWF >	dictionary;

	struct keyButtonImages_t
	{

		keyButtonImages_t()
		{
			key = "";
			xbImage = "";
			psImage = "";
			width = 0;
			height = 0;
			baseline = 0;
		}

		keyButtonImages_t( const char* _key, const char* _xbImage, const char* _psImage, int w, int h, int _baseline )
		{
			key = _key;
			xbImage = _xbImage;
			psImage = _psImage;
			width = w;
			height = h;
			baseline = _baseline;
		}

		const char* key;
		const char* xbImage;
		const char* psImage;
		int width;
		int height;
		int baseline;
	};
	idList< keyButtonImages_t, TAG_SWF > tooltipButtonImage;

	struct tooltipIcon_t
	{
		tooltipIcon_t()
		{
			startIndex = -1;
			endIndex = -1;
			material = NULL;
			imageWidth = 0;
			imageHeight = 0;
			baseline = 0;
		};

		int					startIndex;
		int					endIndex;
		const idMaterial* 	material;
		short				imageWidth;
		short				imageHeight;
		int					baseline;
	};
	idList< tooltipIcon_t, TAG_SWF > tooltipIconList;

	const idMaterial* guiSolid;
	const idMaterial* guiCursor_arrow;
	const idMaterial* guiCursor_hand;
	const idMaterial* white;
	// RB begin
	const idFont*     debugFont;
	// RB end

private:
	friend class idSWFSprite;
	friend class idSWFSpriteInstance;

	// RB begin
	bool			LoadSWF( const char* fullpath );
	void			WriteSWF( const char* filename, const byte* atlasImageRGBA, int atlasImageWidth, int atlasImageHeight );

	bool			LoadBinary( const char* bfilename, ID_TIME_T sourceTime );
	void			WriteBinary( const char* bfilename );

	void			FileAttributes( idSWFBitStream& bitstream );
	void			Metadata( idSWFBitStream& bitstream );
	void			SetBackgroundColor( idSWFBitStream& bitstream );

	void			LoadXML( const char* filename );
	void			WriteXML( const char* filename );

	bool			LoadJSON( const char* filename );
	void			WriteJSON( const char* filename );
	// RB end

	//----------------------------------
	// SWF_Shapes.cpp
	//----------------------------------
	void			DefineShape( idSWFBitStream& bitstream );
	void			DefineShape2( idSWFBitStream& bitstream );
	void			DefineShape3( idSWFBitStream& bitstream );
	void			DefineShape4( idSWFBitStream& bitstream );
	void			DefineMorphShape( idSWFBitStream& bitstream );

	//----------------------------------
	// SWF_Sprites.cpp
	//----------------------------------
	void			DefineSprite( idSWFBitStream& bitstream );

	//----------------------------------
	// SWF_Sounds.cpp
	//----------------------------------
	void			DefineSound( idSWFBitStream& bitstream );

	//----------------------------------
	// SWF_Render.cpp
	//----------------------------------
	void			DrawStretchPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, const idMaterial* material );
	void			DrawStretchPic( const idVec4& topLeft, const idVec4& topRight, const idVec4& bottomRight, const idVec4& bottomLeft, const idMaterial* material );
	void			RenderSprite( idRenderSystem* gui, idSWFSpriteInstance* sprite, const swfRenderState_t& renderState, int time, bool isSplitscreen = false );
	void			RenderMask( idRenderSystem* gui, const swfDisplayEntry_t* mask, const swfRenderState_t& renderState, const int stencilMode );
	void			RenderShape( idRenderSystem* gui, const idSWFShape* shape, const swfRenderState_t& renderState );
	void			RenderMorphShape( idRenderSystem* gui, const idSWFShape* shape, const swfRenderState_t& renderState );
	void			DrawEditCursor( idRenderSystem* gui, float x, float y, float w, float h, const swfMatrix_t& matrix );
	void			DrawLine( idRenderSystem* gui, const idVec2& p1, const idVec2& p2, float width, const swfMatrix_t& matrix );
	void			RenderEditText( idRenderSystem* gui, idSWFTextInstance* textInstance, const swfRenderState_t& renderState, int time, bool isSplitscreen = false );
	uint64			GLStateForRenderState( const swfRenderState_t& renderState );
	void			FindTooltipIcons( idStr* text );

	// RB: debugging tools
	swfRect_t		CalcRect( const idSWFSpriteInstance* sprite, const swfRenderState_t& renderState );
	void			DrawRect( idRenderSystem* gui, const swfRect_t& rect, const idVec4& color );
	int				DrawText( idRenderSystem* gui, float x, float y, float scale, idVec4 color, const char* text, float adjust, int limit, int style );
	int				DrawText( idRenderSystem* gui, const char* text, float textScale, int textAlign, idVec4 color, const swfRect_t& rectDraw, bool wrap, int cursor = -1, bool calcOnly = false, idList<int>* breaks = NULL, int limit = 0 );
	// RB end

	//----------------------------------
	// SWF_Image.cpp
	//----------------------------------

	class idDecompressJPEG
	{
	public:
		idDecompressJPEG();
		~idDecompressJPEG();

		byte* Load( const byte* input, int inputSize, int& width, int& height );

	private:
		void* vinfo;
	};

	idDecompressJPEG	jpeg;

	void			LoadImage( int characterID, const byte* imageData, int width, int height );

	void			JPEGTables( idSWFBitStream& bitstream );
	void			DefineBits( idSWFBitStream& bitstream );
	void			DefineBitsJPEG2( idSWFBitStream& bitstream );
	void			DefineBitsJPEG3( idSWFBitStream& bitstream );
	void			DefineBitsLossless( idSWFBitStream& bitstream );
	void			DefineBitsLossless2( idSWFBitStream& bitstream );


	// per-swf image atlas
	struct imageToPack_t
	{
		int	characterID;
		idVec2i	trueSize;	// in texels
		byte* imageData;	// trueSize.x * trueSize.y * 4
		idVec2i	allocSize;	// in DXT tiles, includes a border texel and rounding up to DXT blocks
	};

	class idSortBlocks : public idSort_Quick< imageToPack_t, idSortBlocks >
	{
	public:
		int Compare( const imageToPack_t& a, const imageToPack_t& b ) const
		{
			return ( b.allocSize.x * b.allocSize.y ) - ( a.allocSize.x * a.allocSize.y );
		}
	};

	idList<imageToPack_t, TAG_SWF>	packImages;	// only used during creation
	void			WriteSwfImageAtlas( const char* filename );

	//----------------------------------
	// SWF_Text.cpp
	//----------------------------------
	void			DefineFont2( idSWFBitStream& bitstream );
	void			DefineFont3( idSWFBitStream& bitstream );
	void			DefineTextX( idSWFBitStream& bitstream, bool rgba );
	void			DefineText( idSWFBitStream& bitstream );
	void			DefineText2( idSWFBitStream& bitstream );
	void			DefineEditText( idSWFBitStream& bitstream );

	//----------------------------------
	// SWF_Zlib.cpp
	//----------------------------------
	bool			Inflate( const byte* input, int inputSize, byte* output, int outputSize );
	// RB begin
	bool			Deflate( const byte* input, int inputSize, byte* output, int& outputSize );
	// RB end

public:
	//----------------------------------
	// SWF_Names.cpp
	//----------------------------------
	// RB begin
	static const char* GetDictTypeName( swfDictType_t type );
	static const char* GetEditTextAlignName( swfEditTextAlign_t align );
	// RB end
	static const char* GetTagName( swfTag_t tag );
	static const char* GetActionName( swfAction_t action );

};

#endif // !__SWF_H__
