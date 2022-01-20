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
#include "precompiled.h"
#pragma hdrstop
#include "../renderer/Image.h"
#include "../renderer/DXT/DXTCodec.h"

#pragma warning(disable: 4355) // 'this' : used in base member initializer list

idCVar swf_loadBinary( "swf_loadBinary", "1", CVAR_INTEGER, "used to set whether to load binary swf from generated" );
// RB begin
idCVar postLoadExportFlashAtlas( "postLoadExportFlashAtlas", "0", CVAR_INTEGER, "" );
idCVar postLoadExportFlashToSWF( "postLoadExportFlashToSWF", "0", CVAR_INTEGER, "" );
idCVar postLoadExportFlashToJSON( "postLoadExportFlashToJSON", "0", CVAR_INTEGER, "" );
// RB end

int idSWF::mouseX = -1;
int idSWF::mouseY = -1;
bool idSWF::isMouseInClientArea = false;

extern idCVar in_useJoystick;



/*
===================
idSWF::idSWF
===================
*/
idSWF::idSWF( const char* filename_, idSoundWorld* soundWorld_ )
{

	atlasMaterial = NULL;

	swfScale = 1.0f;
	scaleToVirtual.Set( 1.0f, 1.0f );

	random.SetSeed( Sys_Milliseconds() );

	guiSolid = declManager->FindMaterial( "guiSolid" );
	guiCursor_arrow = declManager->FindMaterial( "ui/assets/guicursor_arrow" );
	guiCursor_hand = declManager->FindMaterial( "ui/assets/guicursor_hand" );
	white = declManager->FindMaterial( "_white" );

	// RB:
	debugFont = renderSystem->RegisterFont( "Arial Narrow" );

	tooltipButtonImage.Append( keyButtonImages_t( "<JOY1>", "guis/assets/hud/controller/xb360/a", "guis/assets/hud/controller/ps3/cross", 37, 37, 0 ) );
	tooltipButtonImage.Append( keyButtonImages_t( "<JOY2>", "guis/assets/hud/controller/xb360/b", "guis/assets/hud/controller/ps3/circle", 37, 37, 0 ) );
	tooltipButtonImage.Append( keyButtonImages_t( "<JOY3>", "guis/assets/hud/controller/xb360/x", "guis/assets/hud/controller/ps3/square", 37, 37, 0 ) );
	tooltipButtonImage.Append( keyButtonImages_t( "<JOY4>", "guis/assets/hud/controller/xb360/y", "guis/assets/hud/controller/ps3/triangle", 37, 37, 0 ) );
	tooltipButtonImage.Append( keyButtonImages_t( "<JOY_TRIGGER2>", "guis/assets/hud/controller/xb360/rt", "guis/assets/hud/controller/ps3/r2", 64, 52, 0 ) );
	tooltipButtonImage.Append( keyButtonImages_t( "<JOY_TRIGGER1>", "guis/assets/hud/controller/xb360/lt", "guis/assets/hud/controller/ps3/l2", 64, 52, 0 ) );
	tooltipButtonImage.Append( keyButtonImages_t( "<JOY5>", "guis/assets/hud/controller/xb360/lb", "guis/assets/hud/controller/ps3/l1", 52, 32, 0 ) );
	tooltipButtonImage.Append( keyButtonImages_t( "<JOY6>", "guis/assets/hud/controller/xb360/rb", "guis/assets/hud/controller/ps3/r1", 52, 32, 0 ) );
	tooltipButtonImage.Append( keyButtonImages_t( "<MOUSE1>", "guis/assets/hud/controller/mouse1", "", 64, 52, 0 ) );
	tooltipButtonImage.Append( keyButtonImages_t( "<MOUSE2>", "guis/assets/hud/controller/mouse2", "", 64, 52, 0 ) );
	tooltipButtonImage.Append( keyButtonImages_t( "<MOUSE3>", "guis/assets/hud/controller/mouse3", "", 64, 52, 0 ) );

	for( int index = 0; index < tooltipButtonImage.Num(); index++ )
	{
		if( ( tooltipButtonImage[index].xbImage != NULL ) && ( tooltipButtonImage[index].xbImage[0] != '\0' ) )
		{
			declManager->FindMaterial( tooltipButtonImage[index].xbImage );
		}
		if( ( tooltipButtonImage[index].psImage != NULL ) && ( tooltipButtonImage[index].psImage[0] != '\0' ) )
		{
			declManager->FindMaterial( tooltipButtonImage[index].psImage );
		}
	}

	frameWidth = 0;
	frameHeight = 0;
	frameRate = 0;
	lastRenderTime = 0;

	isActive = false;
	inhibitControl = false;
	useInhibtControl = true;

	crop = false;
	blackbars = false;
	paused = false;
	hasHitObject = false;

	useMouse = true;
	mouseEnabled = false;
	renderBorder = 0;
	mouseObject = NULL;
	hoverObject = NULL;
	soundWorld = NULL;
	forceNonPCPlatform = false;

	if( idStr::Cmpn( filename_, "swf/", 4 ) != 0 )
	{
		// if it doesn't already have swf/ in front of it, add it
		filename = "swf/";
		filename += filename_;
	}
	else
	{
		filename = filename_;
	}
	filename.ToLower();
	filename.BackSlashesToSlashes();
	filename.SetFileExtension( ".swf" );

	timestamp = fileSystem->GetTimestamp( filename );

	mainsprite = new( TAG_SWF ) idSWFSprite( this );
	mainspriteInstance = NULL;

	idStr binaryFileName = "generated/";
	binaryFileName += filename;
	binaryFileName.SetFileExtension( ".bswf" );

	// RB: add JSON alternative
	idStr jsonFileName = filename;
	jsonFileName.SetFileExtension( ".json" );
	ID_TIME_T jsonSourceTime = fileSystem->GetTimestamp( jsonFileName );

	bool loadedFromJSON = false;
	if( swf_loadBinary.GetBool() )
	{
		if( timestamp == FILE_NOT_FOUND_TIMESTAMP )
		{
			timestamp = jsonSourceTime;
		}

		if( !LoadBinary( binaryFileName, timestamp ) )
		{
			if( LoadJSON( jsonFileName ) )
			{
				loadedFromJSON = true;

				WriteBinary( binaryFileName );
			}
			else if( LoadSWF( filename ) )
			{
				WriteBinary( binaryFileName );
			}
		}
	}
	else
	{
		if( LoadJSON( jsonFileName ) )
		{
			loadedFromJSON = true;
		}
		else
		{
			LoadSWF( filename );
		}
	}

	if( postLoadExportFlashToJSON.GetBool() )
	{
		idStr jsonFileName = "exported/";
		jsonFileName += filename;
		jsonFileName.SetFileExtension( ".json" );

		WriteJSON( jsonFileName );
	}

	idStr atlasFileName = binaryFileName;
	atlasFileName.SetFileExtension( ".png" );
	atlasMaterial = declManager->FindMaterial( atlasFileName );

	byte* atlasExportImageRGBA = NULL;
	int atlasExportImageWidth = 0;
	int atlasExportImageHeight = 0;

	if( /*!loadedFromJSON &&*/ ( postLoadExportFlashToJSON.GetBool() || postLoadExportFlashAtlas.GetBool() || postLoadExportFlashToSWF.GetBool() ) )
	{
		// try loading the TGA first
		ID_TIME_T timestamp;
		//LoadTGA( atlasFileName.c_str(), &atlasExportImageRGBA, &atlasExportImageWidth, &atlasExportImageHeight, &timestamp );
		LoadPNG( atlasFileName.c_str(), &atlasExportImageRGBA, &atlasExportImageWidth, &atlasExportImageHeight, &timestamp );

		if( ( atlasExportImageRGBA == NULL ) || ( timestamp == FILE_NOT_FOUND_TIMESTAMP ) )
		{
			idLib::Warning( "failed to load atlas '%s'", atlasFileName.c_str() );

			idStrStatic< MAX_OSPATH > generatedName = atlasFileName;
			generatedName.StripFileExtension();
			idImage::GetGeneratedName( generatedName, TD_DEFAULT, CF_2D );

			idBinaryImage im( generatedName );
			ID_TIME_T binaryFileTime = im.LoadFromGeneratedFile( FILE_NOT_FOUND_TIMESTAMP );

			if( binaryFileTime != FILE_NOT_FOUND_TIMESTAMP )
			{
				const bimageFile_t& imgHeader = im.GetFileHeader();
				const bimageImage_t& img = im.GetImageHeader( 0 );

				const byte* data = im.GetImageData( 0 );

				//( img.level, 0, 0, img.destZ, img.width, img.height, data );

				idTempArray<byte> rgba( img.width * img.height * 4 );
				memset( rgba.Ptr(), 255, rgba.Size() );

				if( imgHeader.format == FMT_DXT1 )
				{
					idDxtDecoder dxt;
					dxt.DecompressImageDXT1( data, rgba.Ptr(), img.width, img.height );
				}
				else if( imgHeader.format == FMT_DXT5 )
				{
					idDxtDecoder dxt;

					if( imgHeader.colorFormat == CFM_NORMAL_DXT5 )
					{
						dxt.DecompressNormalMapDXT5( data, rgba.Ptr(), img.width, img.height );
					}
					else if( imgHeader.colorFormat == CFM_YCOCG_DXT5 )
					{
						dxt.DecompressYCoCgDXT5( data, rgba.Ptr(), img.width, img.height );
					}
					else
					{

						dxt.DecompressImageDXT5( data, rgba.Ptr(), img.width, img.height );
					}
				}
				else if( imgHeader.format == FMT_LUM8 || imgHeader.format == FMT_INT8 )
				{
					// LUM8 and INT8 just read the red channel
					byte* pic = rgba.Ptr();
					for( int i = 0; i < img.dataSize; i++ )
					{
						pic[ i * 4 ] = data[ i ];
					}
				}
				else if( imgHeader.format == FMT_ALPHA )
				{
					// ALPHA reads the alpha channel
					byte* pic = rgba.Ptr();
					for( int i = 0; i < img.dataSize; i++ )
					{
						pic[ i * 4 + 3 ] = data[ i ];
					}
				}
				else if( imgHeader.format == FMT_L8A8 )
				{
					// L8A8 reads the alpha and red channels
					byte* pic = rgba.Ptr();
					for( int i = 0; i < img.dataSize / 2; i++ )
					{
						pic[ i * 4 + 0 ] = data[ i * 2 + 0 ];
						pic[ i * 4 + 3 ] = data[ i * 2 + 1 ];
					}
				}
				else if( imgHeader.format == FMT_RGB565 )
				{
					// FIXME
					/*
					byte* pic = rgba.Ptr();
					for( int i = 0; i < img.dataSize / 2; i++ )
					{
						unsigned short color = ( ( pic[ i * 4 + 0 ] >> 3 ) << 11 ) | ( ( pic[ i * 4 + 1 ] >> 2 ) << 5 ) | ( pic[ i * 4 + 2 ] >> 3 );
						img.data[ i * 2 + 0 ] = ( color >> 8 ) & 0xFF;
						img.data[ i * 2 + 1 ] = color & 0xFF;
					}
					*/
				}
				else
				{
					byte* pic = rgba.Ptr();
					for( int i = 0; i < img.dataSize; i++ )
					{
						pic[ i ] = data[ i ];
					}
				}

				idStr atlasFileNameExport = atlasFileName;
				atlasFileNameExport.Replace( "generated/", "exported/" );
				atlasFileNameExport.SetFileExtension( ".png" );

				R_WritePNG( atlasFileNameExport, rgba.Ptr(), 4, img.width, img.height, true, "fs_basepath" );

				if( postLoadExportFlashToSWF.GetBool() )
				{
					atlasExportImageWidth = img.width;
					atlasExportImageHeight = img.height;
					atlasExportImageRGBA = ( byte* ) Mem_Alloc( rgba.Size(), TAG_TEMP );
					memcpy( atlasExportImageRGBA, rgba.Ptr(), rgba.Size() );
				}
			}
		}

	}

	if( postLoadExportFlashToSWF.GetBool() )
	{
		idStr swfFileName = "exported/";
		swfFileName += filename;
		swfFileName.SetFileExtension( ".swf" );

		WriteSWF( swfFileName, atlasExportImageRGBA, atlasExportImageWidth, atlasExportImageHeight );
	}

	if( atlasExportImageRGBA != NULL )
	{
		Mem_Free( atlasExportImageRGBA );
		atlasExportImageRGBA = NULL;
	}
	// RB end

	globals = idSWFScriptObject::Alloc();
	globals->Set( "_global", globals );

	globals->Set( "Object", &scriptFunction_Object );

	mainspriteInstance = spriteInstanceAllocator.Alloc();
	mainspriteInstance->Init( mainsprite, NULL, 0 );

	shortcutKeys = idSWFScriptObject::Alloc();
	scriptFunction_shortcutKeys_clear.Bind( this );
	scriptFunction_shortcutKeys_clear.Call( shortcutKeys, idSWFParmList() );
	globals->Set( "shortcutKeys", shortcutKeys );

	globals->Set( "deactivate", scriptFunction_deactivate.Bind( this ) );
	globals->Set( "inhibitControl", scriptFunction_inhibitControl.Bind( this ) );
	globals->Set( "useInhibit", scriptFunction_useInhibit.Bind( this ) );
	globals->Set( "precacheSound", scriptFunction_precacheSound.Bind( this ) );
	globals->Set( "playSound", scriptFunction_playSound.Bind( this ) );
	globals->Set( "stopSounds", scriptFunction_stopSounds.Bind( this ) );
	globals->Set( "getPlatform", scriptFunction_getPlatform.Bind( this ) );
	globals->Set( "getTruePlatform", scriptFunction_getTruePlatform.Bind( this ) );
	globals->Set( "getLocalString", scriptFunction_getLocalString.Bind( this ) );
	globals->Set( "swapPS3Buttons", scriptFunction_swapPS3Buttons.Bind( this ) );
	globals->Set( "_root", mainspriteInstance->scriptObject );
	globals->Set( "strReplace", scriptFunction_strReplace.Bind( this ) );
	globals->Set( "getCVarInteger", scriptFunction_getCVarInteger.Bind( this ) );
	globals->Set( "setCVarInteger", scriptFunction_setCVarInteger.Bind( this ) );

	globals->Set( "acos", scriptFunction_acos.Bind( this ) );
	globals->Set( "cos", scriptFunction_cos.Bind( this ) );
	globals->Set( "sin", scriptFunction_sin.Bind( this ) );
	globals->Set( "round", scriptFunction_round.Bind( this ) );
	globals->Set( "pow", scriptFunction_pow.Bind( this ) );
	globals->Set( "sqrt", scriptFunction_sqrt.Bind( this ) );
	globals->Set( "abs", scriptFunction_abs.Bind( this ) );
	globals->Set( "rand", scriptFunction_rand.Bind( this ) );
	globals->Set( "floor", scriptFunction_floor.Bind( this ) );
	globals->Set( "ceil", scriptFunction_ceil.Bind( this ) );
	globals->Set( "toUpper", scriptFunction_toUpper.Bind( this ) );

	globals->SetNative( "platform", swfScriptVar_platform.Bind( &scriptFunction_getPlatform ) );
	globals->SetNative( "blackbars", swfScriptVar_blackbars.Bind( this ) );
	globals->SetNative( "cropToHeight", swfScriptVar_crop.Bind( this ) );
	globals->SetNative( "cropToFit", swfScriptVar_crop.Bind( this ) );
	globals->SetNative( "crop", swfScriptVar_crop.Bind( this ) );

	// Do this to touch any external references (like sounds)
	// But disable script warnings because many globals won't have been created yet
	extern idCVar swf_debug;
	int debug = swf_debug.GetInteger();
	swf_debug.SetInteger( 0 );

	mainspriteInstance->Run();
	mainspriteInstance->RunActions();
	mainspriteInstance->RunTo( 0 );

	swf_debug.SetInteger( debug );

	if( mouseX == -1 )
	{
		mouseX = ( frameWidth / 2 );
	}

	if( mouseY == -1 )
	{
		mouseY = ( frameHeight / 2 );
	}

	soundWorld = soundWorld_;
}

/*
===================
idSWF::~idSWF
===================
*/
idSWF::~idSWF()
{
	spriteInstanceAllocator.Free( mainspriteInstance );
	delete mainsprite;

	for( int i = 0 ; i < dictionary.Num() ; i++ )
	{
		if( dictionary[i].sprite )
		{
			delete dictionary[i].sprite;
			dictionary[i].sprite = NULL;
		}
		if( dictionary[i].shape )
		{
			delete dictionary[i].shape;
			dictionary[i].shape = NULL;
		}
		if( dictionary[i].font )
		{
			delete dictionary[i].font;
			dictionary[i].font = NULL;
		}
		if( dictionary[i].text )
		{
			delete dictionary[i].text;
			dictionary[i].text = NULL;
		}
		if( dictionary[i].edittext )
		{
			delete dictionary[i].edittext;
			dictionary[i].edittext = NULL;
		}
	}

	globals->Clear();
	tooltipButtonImage.Clear();
	globals->Release();

	shortcutKeys->Clear();
	shortcutKeys->Release();
}

/*
===================
idSWF::Activate
when a SWF is deactivated, it rewinds the timeline back to the start
===================
*/
void idSWF::Activate( bool b )
{
	if( !isActive && b )
	{
		inhibitControl = false;
		lastRenderTime = Sys_Milliseconds();

		mainspriteInstance->FreeDisplayList();
		mainspriteInstance->Play();
		mainspriteInstance->Run();
		mainspriteInstance->RunActions();
	}
	isActive = b;
}

/*
===================
idSWF::InhibitControl
===================
*/
bool idSWF::InhibitControl()
{
	if( !IsLoaded() || !IsActive() )
	{
		return false;
	}
	return ( inhibitControl && useInhibtControl );
}

/*
===================
idSWF::PlaySound
===================
*/
int idSWF::PlaySound( const char* sound, int channel, bool blocking )
{
	if( !IsActive() )
	{
		return -1;
	}
	if( soundWorld != NULL )
	{
		return soundWorld->PlayShaderDirectly( sound, channel );
	}
	else
	{
		idLib::Warning( "No playing sound world on soundSystem in swf play sound!" );
		return -1;
	}
}

/*
===================
idSWF::PlaySound
===================
*/
void idSWF::StopSound( int channel )
{
	if( soundWorld != NULL )
	{
		soundWorld->PlayShaderDirectly( NULL, channel );
	}
	else
	{
		idLib::Warning( "No playing sound world on soundSystem in swf play sound!" );
	}
}

/*
===================
idSWF::idSWFScriptFunction_inhibitControl::Call
===================
*/
idSWFScriptVar idSWF::idSWFScriptFunction_inhibitControl::Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
{
	pThis->inhibitControl = parms[0].ToBool();
	return idSWFScriptVar();
}

/*
===================
idSWF::idSWFScriptFunction_inhibitControl::Call
===================
*/
idSWFScriptVar idSWF::idSWFScriptFunction_useInhibit::Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
{
	pThis->useInhibtControl = parms[0].ToBool();
	return idSWFScriptVar();
}

/*
===================
idSWF::idSWFScriptFunction_deactivate::Call
===================
*/
idSWFScriptVar idSWF::idSWFScriptFunction_deactivate::Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
{
	pThis->Activate( false );
	return idSWFScriptVar();
}

/*
===================
idSWF::idSWFScriptFunction_precacheSound::Call
===================
*/
idSWFScriptVar idSWF::idSWFScriptFunction_precacheSound::Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
{
	const idSoundShader* soundShader = declManager->FindSound( parms[0].ToString(), true );
	return soundShader->GetName();
}

/*
===================
idSWF::idSWFScriptFunction_playSound::Call
===================
*/
idSWFScriptVar idSWF::idSWFScriptFunction_playSound::Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
{
	int channel = SCHANNEL_ANY;
	// specific channel passed in
	if( parms.Num() > 1 )
	{
		channel = parms[1].ToInteger();
	}

	pThis->PlaySound( parms[0].ToString(), channel );

	return idSWFScriptVar();
}

/*
===================
idSWF::idSWFScriptFunction_stopSounds::Call
===================
*/
idSWFScriptVar idSWF::idSWFScriptFunction_stopSounds::Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
{

	int channel = SCHANNEL_ANY;
	if( parms.Num() == 1 )
	{
		channel = parms[0].ToInteger();
	}

	pThis->StopSound( channel );

	return idSWFScriptVar();
}

/*
========================
idSWFScriptFunction_GetPlatform::Call
========================
*/
idSWFScriptVar idSWF::idSWFScriptFunction_getPlatform::Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
{
	return pThis->GetPlatform();
}

/*
========================
idSWFScriptFunction_GetPlatform::Call
========================
*/
idSWFScriptVar idSWF::idSWFScriptFunction_getTruePlatform::Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
{

	return 2;
}


/*
========================
idSWFScriptFunction_GetPlatform::Call
========================
*/
idSWFScriptVar idSWF::idSWFScriptFunction_strReplace::Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
{

	if( parms.Num() != 3 )
	{
		return "";
	}

	idStr str = parms[0].ToString();
	idStr repString = parms[1].ToString();
	idStr val = parms[2].ToString();
	str.Replace( repString, val );

	return str;
}

/*
========================
idSWFScriptFunction_GetPlatform::Call
========================
*/
idSWFScriptVar idSWF::idSWFScriptFunction_getLocalString::Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
{

	if( parms.Num() == 0 )
	{
		return idSWFScriptVar();
	}

	idStr val = idLocalization::GetString( parms[0].ToString() );
	return val;
}

/*
========================
idSWF::UseCircleForAccept
========================
*/
bool idSWF::UseCircleForAccept()
{
	return false;
}

/*
========================
idSWF::GetPlatform
========================
*/
int	idSWF::GetPlatform()
{


	if( in_useJoystick.GetBool() || forceNonPCPlatform )
	{
		forceNonPCPlatform = false;
		return 0;
	}

	return 2;
}

/*
========================
idSWFScriptFunction_swapPS3Buttons::Call
========================
*/
idSWFScriptVar idSWF::idSWFScriptFunction_swapPS3Buttons::Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
{
	return pThis->UseCircleForAccept();
}

/*
========================
idSWFScriptFunction_getCVarInteger::Call
========================
*/
idSWFScriptVar idSWF::idSWFScriptFunction_getCVarInteger::Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
{
	return cvarSystem->GetCVarInteger( parms[0].ToString() );
}

/*
========================
idSWFScriptFunction_setCVarInteger::Call
========================
*/
idSWFScriptVar idSWF::idSWFScriptFunction_setCVarInteger::Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
{
	cvarSystem->SetCVarInteger( parms[0].ToString(), parms[1].ToInteger() );
	return idSWFScriptVar();
}

/*
===================
idSWF::idSWFScriptFunction_acos::Call
===================
*/
idSWFScriptVar idSWF::idSWFScriptFunction_acos::Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
{
	if( parms.Num() != 1 )
	{
		return idSWFScriptVar();
	}
	return idMath::ACos( parms[0].ToFloat() );
}

/*
===================
idSWF::idSWFScriptFunction_cos::Call
===================
*/
idSWFScriptVar idSWF::idSWFScriptFunction_cos::Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
{
	if( parms.Num() != 1 )
	{
		return idSWFScriptVar();
	}
	return idMath::Cos( parms[0].ToFloat() );
}

/*
===================
idSWF::idSWFScriptFunction_sin::Call
===================
*/
idSWFScriptVar idSWF::idSWFScriptFunction_sin::Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
{
	if( parms.Num() != 1 )
	{
		return idSWFScriptVar();
	}
	return ( idMath::Sin( parms[0].ToFloat() ) );
}

/*
===================
idSWF::idSWFScriptFunction_round::Call
===================
*/
idSWFScriptVar idSWF::idSWFScriptFunction_round::Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
{
	if( parms.Num() != 1 )
	{
		return idSWFScriptVar();
	}
	int value = idMath::Ftoi( parms[0].ToFloat() + 0.5f );
	return value;
}

/*
===================
idSWF::idSWFScriptFunction_pow::Call
===================
*/
idSWFScriptVar idSWF::idSWFScriptFunction_pow::Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
{
	if( parms.Num() != 2 )
	{
		return idSWFScriptVar();
	}

	float value = parms[0].ToFloat();
	float power = parms[1].ToFloat();
	return ( idMath::Pow( value, power ) );
}

/*
===================
idSWF::idSWFScriptFunction_pow::Call
===================
*/
idSWFScriptVar idSWF::idSWFScriptFunction_sqrt::Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
{
	if( parms.Num() != 1 )
	{
		return idSWFScriptVar();
	}

	float value = parms[0].ToFloat();
	return ( idMath::Sqrt( value ) );
}

/*
===================
idSWF::idSWFScriptFunction_abs::Call
===================
*/
idSWFScriptVar idSWF::idSWFScriptFunction_abs::Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
{
	if( parms.Num() != 1 )
	{
		return idSWFScriptVar();
	}

	float value = idMath::Fabs( parms[0].ToFloat() );
	return value;
}

/*
===================
idSWF::idSWFScriptFunction_rand::Call
===================
*/
idSWFScriptVar idSWF::idSWFScriptFunction_rand::Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
{
	float min = 0.0f;
	float max = 1.0f;
	switch( parms.Num() )
	{
		case 0:
			break;
		case 1:
			max = parms[0].ToFloat();
			break;
		default:
			min = parms[0].ToFloat();
			max = parms[1].ToFloat();
			break;
	}
	return min + pThis->GetRandom().RandomFloat() * ( max - min );
}

/*
========================
idSWFScriptFunction_floor::Call
========================
*/
idSWFScriptVar idSWF::idSWFScriptFunction_floor::Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
{
	if( parms.Num() != 1 || !parms[0].IsNumeric() )
	{
		idLib::Warning( "Invalid parameters specified for floor" );
		return idSWFScriptVar();
	}

	float num = parms[0].ToFloat();

	return idSWFScriptVar( idMath::Floor( num ) );
}

/*
========================
idSWFScriptFunction_ceil::Call
========================
*/
idSWFScriptVar idSWF::idSWFScriptFunction_ceil::Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
{
	if( parms.Num() != 1 || !parms[0].IsNumeric() )
	{
		idLib::Warning( "Invalid parameters specified for ceil" );
		return idSWFScriptVar();
	}

	float num = parms[0].ToFloat();

	return idSWFScriptVar( idMath::Ceil( num ) );
}

/*
========================
idSWFScriptFunction_toUpper::Call
========================
*/
idSWFScriptVar idSWF::idSWFScriptFunction_toUpper::Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
{
	if( parms.Num() != 1 || !parms[0].IsString() )
	{
		idLib::Warning( "Invalid parameters specified for toUpper" );
		return idSWFScriptVar();
	}

	idStr val = idLocalization::GetString( parms[0].ToString() );
	val.ToUpper();
	return val;
}

/*
===================
idSWF::idSWFScriptFunction_shortcutKeys_clear::Call
===================
*/
idSWFScriptVar idSWF::idSWFScriptFunction_shortcutKeys_clear::Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
{
	idSWFScriptObject* object = pThis->shortcutKeys;
	object->Clear();
	object->Set( "clear", this );
	object->Set( "JOY1", "ENTER" );
	object->Set( "JOY2", "BACKSPACE" );
	object->Set( "JOY3", "START" );
	object->Set( "JOY5", "LB" );
	object->Set( "JOY6", "RB" );
	object->Set( "JOY9", "START" );
	object->Set( "JOY10", "BACKSPACE" );
	object->Set( "JOY_DPAD_UP", "UP" );
	object->Set( "JOY_DPAD_DOWN", "DOWN" );
	object->Set( "JOY_DPAD_LEFT", "LEFT" );
	object->Set( "JOY_DPAD_RIGHT", "RIGHT" );
	object->Set( "JOY_STICK1_UP", "STICK1_UP" );
	object->Set( "JOY_STICK1_DOWN", "STICK1_DOWN" );
	object->Set( "JOY_STICK1_LEFT", "STICK1_LEFT" );
	object->Set( "JOY_STICK1_RIGHT", "STICK1_RIGHT" );
	object->Set( "JOY_STICK2_UP", "STICK2_UP" );
	object->Set( "JOY_STICK2_DOWN", "STICK2_DOWN" );
	object->Set( "JOY_STICK2_LEFT", "STICK2_LEFT" );
	object->Set( "JOY_STICK2_RIGHT", "STICK2_RIGHT" );
	object->Set( "KP_ENTER", "ENTER" );
	object->Set( "MWHEELDOWN", "MWHEEL_DOWN" );
	object->Set( "MWHEELUP", "MWHEEL_UP" );
	object->Set( "K_TAB", "TAB" );


	// FIXME: I'm an RTARD and didn't realize the keys all have "ARROW" after them
	object->Set( "LEFTARROW", "LEFT" );
	object->Set( "RIGHTARROW", "RIGHT" );
	object->Set( "UPARROW", "UP" );
	object->Set( "DOWNARROW", "DOWN" );


	return idSWFScriptVar();
}

idSWFScriptVar idSWF::idSWFScriptNativeVar_blackbars::Get( idSWFScriptObject* object )
{
	return pThis->blackbars;
}

void idSWF::idSWFScriptNativeVar_blackbars::Set( idSWFScriptObject* object, const idSWFScriptVar& value )
{
	pThis->blackbars = value.ToBool();
}

idSWFScriptVar idSWF::idSWFScriptNativeVar_crop::Get( idSWFScriptObject* object )
{
	return pThis->crop;
}

void idSWF::idSWFScriptNativeVar_crop::Set( idSWFScriptObject* object, const idSWFScriptVar& value )
{
	pThis->crop = value.ToBool();
}
