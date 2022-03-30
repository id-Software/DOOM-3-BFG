/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2016 Robert Beckebans

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

/*
================================================================================================
Contains the Image implementation for OpenGL.
================================================================================================
*/

#include "../RenderCommon.h"

/*
====================
idImage::idImage
====================
*/
idImage::idImage( const char* name ) : imgName( name )
{
	texnum = TEXTURE_NOT_LOADED;
	internalFormat = 0;
	dataFormat = 0;
	dataType = 0;
	generatorFunction = NULL;
	filter = TF_DEFAULT;
	repeat = TR_REPEAT;
	usage = TD_DEFAULT;
	cubeFiles = CF_2D;
	cubeMapSize = 0;

	referencedOutsideLevelLoad = false;
	levelLoadReferenced = false;
	defaulted = false;
	sourceFileTime = FILE_NOT_FOUND_TIMESTAMP;
	binaryFileTime = FILE_NOT_FOUND_TIMESTAMP;
	refCount = 0;
}

/*
====================
idImage::~idImage
====================
*/
idImage::~idImage()
{
	PurgeImage();
}

/*
====================
idImage::IsLoaded
====================
*/
bool idImage::IsLoaded() const
{
	return texnum != TEXTURE_NOT_LOADED;
}

/*
==============
Bind

Automatically enables 2D mapping or cube mapping if needed
==============
*/
void idImage::Bind()
{
	RENDERLOG_PRINTF( "idImage::Bind( %s )\n", GetName() );

	// load the image if necessary (FIXME: not SMP safe!)
	// RB: don't try again if last time failed
	if( !IsLoaded() && !defaulted )
	{
		// load the image on demand here, which isn't our normal game operating mode
		ActuallyLoadImage( true );
	}

	const int texUnit = tr.backend.GetCurrentTextureUnit();

	// RB: added support for more types
	tmu_t* tmu = &glcontext.tmu[texUnit];
	// bind the texture
	if( opts.textureType == TT_2D )
	{
		if( tmu->current2DMap != texnum )
		{
			tmu->current2DMap = texnum;

#if !defined(USE_GLES2) && !defined(USE_GLES3)
			if( glConfig.directStateAccess )
			{
				glBindMultiTextureEXT( GL_TEXTURE0 + texUnit, GL_TEXTURE_2D, texnum );
			}
			else
#endif
			{
				glActiveTexture( GL_TEXTURE0 + texUnit );
				glBindTexture( GL_TEXTURE_2D, texnum );
			}
		}
	}
	else if( opts.textureType == TT_CUBIC )
	{
		if( tmu->currentCubeMap != texnum )
		{
			tmu->currentCubeMap = texnum;

#if !defined(USE_GLES2) && !defined(USE_GLES3)
			if( glConfig.directStateAccess )
			{
				glBindMultiTextureEXT( GL_TEXTURE0 + texUnit, GL_TEXTURE_CUBE_MAP, texnum );
			}
			else
#endif
			{
				glActiveTexture( GL_TEXTURE0 + texUnit );
				glBindTexture( GL_TEXTURE_CUBE_MAP, texnum );
			}
		}
	}
	else if( opts.textureType == TT_2D_ARRAY )
	{
		if( tmu->current2DArray != texnum )
		{
			tmu->current2DArray = texnum;

#if !defined(USE_GLES2) && !defined(USE_GLES3)
			if( glConfig.directStateAccess )
			{
				glBindMultiTextureEXT( GL_TEXTURE0 + texUnit, GL_TEXTURE_2D_ARRAY, texnum );
			}
			else
#endif
			{
				glActiveTexture( GL_TEXTURE0 + texUnit );
				glBindTexture( GL_TEXTURE_2D_ARRAY, texnum );
			}
		}
	}
	else if( opts.textureType == TT_2D_MULTISAMPLE )
	{
		if( tmu->current2DMap != texnum )
		{
			tmu->current2DMap = texnum;

#if !defined(USE_GLES2) && !defined(USE_GLES3)
			if( glConfig.directStateAccess )
			{
				glBindMultiTextureEXT( GL_TEXTURE0 + texUnit, GL_TEXTURE_2D_MULTISAMPLE, texnum );
			}
			else
#endif
			{
				glActiveTexture( GL_TEXTURE0 + texUnit );
				glBindTexture( GL_TEXTURE_2D_MULTISAMPLE, texnum );
			}
		}
	}
	// RB end
}

/*
====================
CopyFramebuffer
====================
*/
void idImage::CopyFramebuffer( int x, int y, int imageWidth, int imageHeight )
{
	int target = GL_TEXTURE_2D;
	switch( opts.textureType )
	{
		case TT_2D:
			target = GL_TEXTURE_2D;
			break;
		case TT_CUBIC:
			target = GL_TEXTURE_CUBE_MAP;
			break;
		case TT_2D_ARRAY:
			target = GL_TEXTURE_2D_ARRAY;
			break;
		case TT_2D_MULTISAMPLE:
			target = GL_TEXTURE_2D_MULTISAMPLE;
			break;
		default:
			//idLib::FatalError( "%s: bad texture type %d", GetName(), opts.textureType );
			return;
	}

	glBindTexture( target, texnum );

#if !defined(USE_GLES2)
	if( Framebuffer::IsDefaultFramebufferActive() )
	{
		glReadBuffer( GL_BACK );
	}
#endif

	opts.width = imageWidth;
	opts.height = imageHeight;

#if defined(USE_GLES2)
	glCopyTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, x, y, imageWidth, imageHeight, 0 );
#else
	if( r_useHDR.GetBool() && globalFramebuffers.hdrFBO->IsBound() )
	{

		//if( backEnd.glState.currentFramebuffer != NULL && backEnd.glState.currentFramebuffer->IsMultiSampled() )

#if defined(USE_HDR_MSAA)
		if( globalFramebuffers.hdrFBO->IsMultiSampled() )
		{
			glBindFramebuffer( GL_READ_FRAMEBUFFER, globalFramebuffers.hdrFBO->GetFramebuffer() );
			glBindFramebuffer( GL_DRAW_FRAMEBUFFER, globalFramebuffers.hdrNonMSAAFBO->GetFramebuffer() );
			glBlitFramebuffer( 0, 0, glConfig.nativeScreenWidth, glConfig.nativeScreenHeight,
							   0, 0, glConfig.nativeScreenWidth, glConfig.nativeScreenHeight,
							   GL_COLOR_BUFFER_BIT,
							   GL_LINEAR );

			globalFramebuffers.hdrNonMSAAFBO->Bind();

			glCopyTexImage2D( target, 0, GL_RGBA16F, x, y, imageWidth, imageHeight, 0 );

			globalFramebuffers.hdrFBO->Bind();
		}
		else
#endif
		{
			glCopyTexImage2D( target, 0, GL_RGBA16F, x, y, imageWidth, imageHeight, 0 );
		}
	}
	else
	{
		glCopyTexImage2D( target, 0, GL_RGBA8, x, y, imageWidth, imageHeight, 0 );
	}
#endif

	// these shouldn't be necessary if the image was initialized properly
	glTexParameterf( target, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameterf( target, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

	glTexParameterf( target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameterf( target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

	tr.backend.pc.c_copyFrameBuffer++;
}

/*
====================
CopyDepthbuffer
====================
*/
void idImage::CopyDepthbuffer( int x, int y, int imageWidth, int imageHeight )
{
	glBindTexture( ( opts.textureType == TT_CUBIC ) ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D, texnum );

	opts.width = imageWidth;
	opts.height = imageHeight;
	glCopyTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, x, y, imageWidth, imageHeight, 0 );

	tr.backend.pc.c_copyFrameBuffer++;
}

/*
========================
idImage::SubImageUpload
========================
*/
void idImage::SubImageUpload( int mipLevel, int x, int y, int z, int width, int height, const void* pic, int pixelPitch )
{
	assert( x >= 0 && y >= 0 && mipLevel >= 0 && width >= 0 && height >= 0 && mipLevel < opts.numLevels );

	int compressedSize = 0;

	if( IsCompressed() )
	{
		assert( !( x & 3 ) && !( y & 3 ) );

		// compressed size may be larger than the dimensions due to padding to quads
		int quadW = ( width + 3 ) & ~3;
		int quadH = ( height + 3 ) & ~3;
		compressedSize = quadW * quadH * BitsForFormat( opts.format ) / 8;

		int padW = ( opts.width + 3 ) & ~3;
		int padH = ( opts.height + 3 ) & ~3;

		assert( x + width <= padW && y + height <= padH );
		// upload the non-aligned value, OpenGL understands that there
		// will be padding
		if( x + width > opts.width )
		{
			width = opts.width - x;
		}
		if( y + height > opts.height )
		{
			height = opts.height - x;
		}
	}
	else
	{
		assert( x + width <= opts.width && y + height <= opts.height );
	}

	int target;
	int uploadTarget;
	if( opts.textureType == TT_2D )
	{
		target = GL_TEXTURE_2D;
		uploadTarget = GL_TEXTURE_2D;
	}
	else if( opts.textureType == TT_CUBIC )
	{
		target = GL_TEXTURE_CUBE_MAP;
		uploadTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X + z;
	}
	else
	{
		assert( !"invalid opts.textureType" );
		target = GL_TEXTURE_2D;
		uploadTarget = GL_TEXTURE_2D;
	}

	glBindTexture( target, texnum );

	if( pixelPitch != 0 )
	{
		glPixelStorei( GL_UNPACK_ROW_LENGTH, pixelPitch );
	}

	if( opts.format == FMT_RGB565 )
	{
#if !defined(USE_GLES3)
		glPixelStorei( GL_UNPACK_SWAP_BYTES, GL_TRUE );
#endif
	}

#if defined(DEBUG) || defined(__ANDROID__)
	GL_CheckErrors();
#endif
	if( IsCompressed() )
	{
		glCompressedTexSubImage2D( uploadTarget, mipLevel, x, y, width, height, internalFormat, compressedSize, pic );
	}
	else
	{

		// make sure the pixel store alignment is correct so that lower mips get created
		// properly for odd shaped textures - this fixes the mip mapping issues with
		// fonts
		int unpackAlignment = width * BitsForFormat( ( textureFormat_t )opts.format ) / 8;
		if( ( unpackAlignment & 3 ) == 0 )
		{
			glPixelStorei( GL_UNPACK_ALIGNMENT, 4 );
		}
		else
		{
			glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
		}

		glTexSubImage2D( uploadTarget, mipLevel, x, y, width, height, dataFormat, dataType, pic );
	}

#if defined(DEBUG) || defined(__ANDROID__)
	GL_CheckErrors();
#endif

	if( opts.format == FMT_RGB565 )
	{
		glPixelStorei( GL_UNPACK_SWAP_BYTES, GL_FALSE );
	}
	if( pixelPitch != 0 )
	{
		glPixelStorei( GL_UNPACK_ROW_LENGTH, 0 );
	}
}

/*
========================
idImage::SetSamplerState
========================
*/
void idImage::SetSamplerState( textureFilter_t tf, textureRepeat_t tr )
{
	if( tf == filter && tr == repeat )
	{
		return;
	}
	filter = tf;
	repeat = tr;
	glBindTexture( ( opts.textureType == TT_CUBIC ) ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D, texnum );
	SetTexParameters();
}

/*
========================
idImage::SetTexParameters
========================
*/
void idImage::SetTexParameters()
{
	int target = GL_TEXTURE_2D;
	switch( opts.textureType )
	{
		case TT_2D:
			target = GL_TEXTURE_2D;
			break;
		case TT_CUBIC:
			target = GL_TEXTURE_CUBE_MAP;
			break;
		// RB begin
		case TT_2D_ARRAY:
			target = GL_TEXTURE_2D_ARRAY;
			break;
		case TT_2D_MULTISAMPLE:
			//target = GL_TEXTURE_2D_MULTISAMPLE;
			//break;
			// no texture parameters for MSAA FBO textures
			return;
		// RB end
		default:
			idLib::FatalError( "%s: bad texture type %d", GetName(), opts.textureType );
			return;
	}

	// ALPHA, LUMINANCE, LUMINANCE_ALPHA, and INTENSITY have been removed
	// in OpenGL 3.2. In order to mimic those modes, we use the swizzle operators
	if( opts.colorFormat == CFM_GREEN_ALPHA )
	{
		glTexParameteri( target, GL_TEXTURE_SWIZZLE_R, GL_ONE );
		glTexParameteri( target, GL_TEXTURE_SWIZZLE_G, GL_ONE );
		glTexParameteri( target, GL_TEXTURE_SWIZZLE_B, GL_ONE );
		glTexParameteri( target, GL_TEXTURE_SWIZZLE_A, GL_GREEN );
	}
	else if( opts.format == FMT_LUM8 )
	{
		glTexParameteri( target, GL_TEXTURE_SWIZZLE_R, GL_RED );
		glTexParameteri( target, GL_TEXTURE_SWIZZLE_G, GL_RED );
		glTexParameteri( target, GL_TEXTURE_SWIZZLE_B, GL_RED );
		glTexParameteri( target, GL_TEXTURE_SWIZZLE_A, GL_ONE );
	}
	else if( opts.format == FMT_L8A8 )//|| opts.format == FMT_RG16F )
	{
		glTexParameteri( target, GL_TEXTURE_SWIZZLE_R, GL_RED );
		glTexParameteri( target, GL_TEXTURE_SWIZZLE_G, GL_RED );
		glTexParameteri( target, GL_TEXTURE_SWIZZLE_B, GL_RED );
		glTexParameteri( target, GL_TEXTURE_SWIZZLE_A, GL_GREEN );
	}
	else if( opts.format == FMT_ALPHA )
	{
		glTexParameteri( target, GL_TEXTURE_SWIZZLE_R, GL_ONE );
		glTexParameteri( target, GL_TEXTURE_SWIZZLE_G, GL_ONE );
		glTexParameteri( target, GL_TEXTURE_SWIZZLE_B, GL_ONE );
		glTexParameteri( target, GL_TEXTURE_SWIZZLE_A, GL_RED );
	}
	else if( opts.format == FMT_INT8 )
	{
		glTexParameteri( target, GL_TEXTURE_SWIZZLE_R, GL_RED );
		glTexParameteri( target, GL_TEXTURE_SWIZZLE_G, GL_RED );
		glTexParameteri( target, GL_TEXTURE_SWIZZLE_B, GL_RED );
		glTexParameteri( target, GL_TEXTURE_SWIZZLE_A, GL_RED );
	}
	else
	{
		glTexParameteri( target, GL_TEXTURE_SWIZZLE_R, GL_RED );
		glTexParameteri( target, GL_TEXTURE_SWIZZLE_G, GL_GREEN );
		glTexParameteri( target, GL_TEXTURE_SWIZZLE_B, GL_BLUE );
		glTexParameteri( target, GL_TEXTURE_SWIZZLE_A, GL_ALPHA );
	}

	switch( filter )
	{
		case TF_DEFAULT:
			if( r_useTrilinearFiltering.GetBool() )
			{
				glTexParameterf( target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
			}
			else
			{
				glTexParameterf( target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST );
			}
			glTexParameterf( target, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
			break;
		case TF_LINEAR:
			glTexParameterf( target, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
			glTexParameterf( target, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
			break;
		case TF_NEAREST:
		case TF_NEAREST_MIPMAP:
			glTexParameterf( target, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
			glTexParameterf( target, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
			break;
		default:
			common->FatalError( "%s: bad texture filter %d", GetName(), filter );
	}

	if( glConfig.anisotropicFilterAvailable )
	{
		// only do aniso filtering on mip mapped images
		if( filter == TF_DEFAULT )
		{
			int aniso = r_maxAnisotropicFiltering.GetInteger();
			if( aniso > glConfig.maxTextureAnisotropy )
			{
				aniso = glConfig.maxTextureAnisotropy;
			}
			if( aniso < 0 )
			{
				aniso = 0;
			}
			glTexParameterf( target, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso );
		}
		else
		{
			glTexParameterf( target, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1 );
		}
	}

	// RB: disabled use of unreliable extension that can make the game look worse but doesn't save any VRAM
	/*
	if( glConfig.textureLODBiasAvailable && ( usage != TD_FONT ) )
	{
		// use a blurring LOD bias in combination with high anisotropy to fix our aliasing grate textures...
		glTexParameterf( target, GL_TEXTURE_LOD_BIAS_EXT, 0.5 ); //r_lodBias.GetFloat() );
	}
	*/
	// RB end

	// set the wrap/clamp modes
	switch( repeat )
	{
		case TR_REPEAT:
			glTexParameterf( target, GL_TEXTURE_WRAP_S, GL_REPEAT );
			glTexParameterf( target, GL_TEXTURE_WRAP_T, GL_REPEAT );
			break;
		case TR_CLAMP_TO_ZERO:
		{
			float color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
			glTexParameterfv( target, GL_TEXTURE_BORDER_COLOR, color );
			glTexParameterf( target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER );
			glTexParameterf( target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER );
		}
		break;
		case TR_CLAMP_TO_ZERO_ALPHA:
		{
			float color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
			glTexParameterfv( target, GL_TEXTURE_BORDER_COLOR, color );
			glTexParameterf( target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER );
			glTexParameterf( target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER );
		}
		break;
		case TR_CLAMP:
			glTexParameterf( target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
			glTexParameterf( target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
			break;
		default:
			common->FatalError( "%s: bad texture repeat %d", GetName(), repeat );
	}

	// RB: added shadow compare parameters for shadow map textures
	if( opts.format == FMT_SHADOW_ARRAY )
	{
		//glTexParameteri( target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
		glTexParameteri( target, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE );
		glTexParameteri( target, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL );
	}
}

/*
========================
idImage::AllocImage

Every image will pass through this function. Allocates all the necessary MipMap levels for the
Image, but doesn't put anything in them.

This should not be done during normal game-play, if you can avoid it.
========================
*/
void idImage::AllocImage()
{
	GL_CheckErrors();
	PurgeImage();

	switch( opts.format )
	{
		case FMT_RGBA8:
			internalFormat = GL_RGBA8;
			dataFormat = GL_RGBA;
			dataType = GL_UNSIGNED_BYTE;
			break;

		case FMT_XRGB8:
			internalFormat = GL_RGB;
			dataFormat = GL_RGBA;
			dataType = GL_UNSIGNED_BYTE;
			break;

		case FMT_RGB565:
			internalFormat = GL_RGB;
			dataFormat = GL_RGB;
			dataType = GL_UNSIGNED_SHORT_5_6_5;
			break;

		case FMT_ALPHA:
			internalFormat = GL_R8;
			dataFormat = GL_RED;
			dataType = GL_UNSIGNED_BYTE;
			break;

		case FMT_L8A8:
			internalFormat = GL_RG8;
			dataFormat = GL_RG;
			dataType = GL_UNSIGNED_BYTE;
			break;

		case FMT_LUM8:
			internalFormat = GL_R8;
			dataFormat = GL_RED;
			dataType = GL_UNSIGNED_BYTE;
			break;

		case FMT_INT8:
			internalFormat = GL_R8;
			dataFormat = GL_RED;
			dataType = GL_UNSIGNED_BYTE;
			break;

		case FMT_DXT1:
			internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
			dataFormat = GL_RGBA;
			dataType = GL_UNSIGNED_BYTE;
			break;

		case FMT_DXT5:
			internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
			dataFormat = GL_RGBA;
			dataType = GL_UNSIGNED_BYTE;
			break;

		case FMT_DEPTH:
			internalFormat = GL_DEPTH_COMPONENT;
			dataFormat = GL_DEPTH_COMPONENT;
			dataType = GL_UNSIGNED_BYTE;
			break;

		case FMT_DEPTH_STENCIL:
			internalFormat = GL_DEPTH24_STENCIL8;
			dataFormat = GL_DEPTH_STENCIL;
			dataType = GL_UNSIGNED_INT_24_8;
			break;

		case FMT_SHADOW_ARRAY:
			internalFormat = GL_DEPTH_COMPONENT;
			dataFormat = GL_DEPTH_COMPONENT;
			dataType = GL_UNSIGNED_BYTE;
			break;

		case FMT_RG16F:
			internalFormat = GL_RG16F;
			dataFormat = GL_RG;
			dataType = GL_HALF_FLOAT;
			break;

		case FMT_RGBA16F:
			internalFormat = GL_RGBA16F;
			dataFormat = GL_RGBA;
			dataType = GL_HALF_FLOAT;
			break;

		case FMT_RGBA32F:
			internalFormat = GL_RGBA32F;
			dataFormat = GL_RGBA;
			dataType = GL_UNSIGNED_BYTE;
			break;

		case FMT_R32F:
			internalFormat = GL_R32F;
			dataFormat = GL_RED;
			dataType = GL_UNSIGNED_BYTE;
			break;

		case FMT_X16:
			internalFormat = GL_INTENSITY16;
			dataFormat = GL_LUMINANCE;
			dataType = GL_UNSIGNED_SHORT;
			break;
		case FMT_Y16_X16:
			internalFormat = GL_LUMINANCE16_ALPHA16;
			dataFormat = GL_LUMINANCE_ALPHA;
			dataType = GL_UNSIGNED_SHORT;
			break;

		// see http://what-when-how.com/Tutorial/topic-615ll9ug/Praise-for-OpenGL-ES-30-Programming-Guide-291.html
		case FMT_R11G11B10F:
			internalFormat = GL_R11F_G11F_B10F;
			dataFormat = GL_RGB;
			dataType = GL_UNSIGNED_INT_10F_11F_11F_REV;
			break;

		default:
			idLib::Error( "Unhandled image format %d in %s\n", opts.format, GetName() );
	}

	// if we don't have a rendering context, just return after we
	// have filled in the parms.  We must have the values set, or
	// an image match from a shader before OpenGL starts would miss
	// the generated texture
	if( !tr.IsInitialized() )
	{
		return;
	}

	// generate the texture number
	glGenTextures( 1, ( GLuint* )&texnum );
	assert( texnum != TEXTURE_NOT_LOADED );

	//----------------------------------------------------
	// allocate all the mip levels with NULL data
	//----------------------------------------------------

	int numSides;
	int target;
	int uploadTarget;
	if( opts.textureType == TT_2D )
	{
		target = uploadTarget = GL_TEXTURE_2D;
		numSides = 1;
	}
	else if( opts.textureType == TT_CUBIC )
	{
		target = GL_TEXTURE_CUBE_MAP;
		uploadTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X;
		numSides = 6;
	}
	// RB begin
	else if( opts.textureType == TT_2D_ARRAY )
	{
		target = GL_TEXTURE_2D_ARRAY;
		uploadTarget = GL_TEXTURE_2D_ARRAY;
		numSides = 6;
	}
	else if( opts.textureType == TT_2D_MULTISAMPLE )
	{
		target = GL_TEXTURE_2D_MULTISAMPLE;
		uploadTarget = GL_TEXTURE_2D_MULTISAMPLE;
		numSides = 1;
	}
	// RB end
	else
	{
		assert( !"opts.textureType" );
		target = uploadTarget = GL_TEXTURE_2D;
		numSides = 1;
	}

	glBindTexture( target, texnum );

	if( opts.textureType == TT_2D_ARRAY )
	{
		glTexImage3D( uploadTarget, 0, internalFormat, opts.width, opts.height, numSides, 0, dataFormat, GL_UNSIGNED_BYTE, NULL );
	}
	else if( opts.textureType == TT_2D_MULTISAMPLE )
	{
		glTexImage2DMultisample( uploadTarget, opts.samples, internalFormat, opts.width, opts.height, GL_FALSE );
	}
	else
	{
		for( int side = 0; side < numSides; side++ )
		{
			int w = opts.width;
			int h = opts.height;
			if( opts.textureType == TT_CUBIC )
			{
				h = w;
			}
			for( int level = 0; level < opts.numLevels; level++ )
			{

				// clear out any previous error
				GL_CheckErrors();

				if( IsCompressed() )
				{
					int compressedSize = ( ( ( w + 3 ) / 4 ) * ( ( h + 3 ) / 4 ) * int64( 16 ) * BitsForFormat( opts.format ) ) / 8;

					// Even though the OpenGL specification allows the 'data' pointer to be NULL, for some
					// drivers we actually need to upload data to get it to allocate the texture.
					// However, on 32-bit systems we may fail to allocate a large block of memory for large
					// textures. We handle this case by using HeapAlloc directly and allowing the allocation
					// to fail in which case we simply pass down NULL to glCompressedTexImage2D and hope for the best.
					// As of 2011-10-6 using NVIDIA hardware and drivers we have to allocate the memory with HeapAlloc
					// with the exact size otherwise large image allocation (for instance for physical page textures)
					// may fail on Vista 32-bit.

					// RB begin
#if defined(_WIN32)
					void* data = HeapAlloc( GetProcessHeap(), 0, compressedSize );
					glCompressedTexImage2D( uploadTarget + side, level, internalFormat, w, h, 0, compressedSize, data );
					if( data != NULL )
					{
						HeapFree( GetProcessHeap(), 0, data );
					}
#else
					byte* data = ( byte* )Mem_Alloc( compressedSize, TAG_TEMP );
					glCompressedTexImage2D( uploadTarget + side, level, internalFormat, w, h, 0, compressedSize, data );
					if( data != NULL )
					{
						Mem_Free( data );
					}
#endif
					// RB end
				}
				else
				{
					glTexImage2D( uploadTarget + side, level, internalFormat, w, h, 0, dataFormat, dataType, NULL );
				}

				GL_CheckErrors();

				w = Max( 1, w >> 1 );
				h = Max( 1, h >> 1 );
			}
		}

		glTexParameteri( target, GL_TEXTURE_MAX_LEVEL, opts.numLevels - 1 );
	}

	// see if we messed anything up
	GL_CheckErrors();

	SetTexParameters();

	GL_CheckErrors();
}

/*
========================
idImage::PurgeImage
========================
*/
void idImage::PurgeImage()
{
	if( texnum != TEXTURE_NOT_LOADED )
	{
		glDeleteTextures( 1, ( GLuint* )&texnum );	// this should be the ONLY place it is ever called!
		texnum = TEXTURE_NOT_LOADED;
	}

	// clear all the current binding caches, so the next bind will do a real one
	for( int i = 0; i < MAX_MULTITEXTURE_UNITS; i++ )
	{
		glcontext.tmu[i].current2DMap = TEXTURE_NOT_LOADED;
		glcontext.tmu[i].current2DArray = TEXTURE_NOT_LOADED;
		glcontext.tmu[i].currentCubeMap = TEXTURE_NOT_LOADED;
	}

	// reset for reloading images
	defaulted = false;
}

/*
========================
idImage::Resize
========================
*/
void idImage::Resize( int width, int height )
{
	if( opts.width == width && opts.height == height )
	{
		return;
	}
	opts.width = width;
	opts.height = height;
	AllocImage();
}
