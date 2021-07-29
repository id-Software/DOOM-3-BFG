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
#include "../renderer/RenderCommon.h"

idCVar swf_timescale( "swf_timescale", "1", CVAR_FLOAT, "timescale for swf files" );
idCVar swf_stopat( "swf_stopat", "0", CVAR_FLOAT, "stop at a specific frame" );

idCVar swf_titleSafe( "swf_titleSafe", "0.005", CVAR_FLOAT, "space between UI elements and screen edge", 0.0f, 0.075f );

idCVar swf_forceAlpha( "swf_forceAlpha", "0", CVAR_FLOAT, "force an alpha value on all elements, useful to show invisible animating elements", 0.0f, 1.0f );

// RB begin
idCVar swf_skipSolids( "swf_skipSolids", "0", CVAR_BOOL, "" );
idCVar swf_skipGradients( "swf_skipGradients", "0", CVAR_BOOL, "" );
idCVar swf_skipLineDraws( "swf_skipLineDraws", "0", CVAR_BOOL, "" );
idCVar swf_skipBitmaps( "swf_skipBitmaps", "0", CVAR_BOOL, "" );

idCVar swf_show( "swf_show", "0", CVAR_INTEGER, "" );
// RB end

extern idCVar swf_textStrokeSize;
extern idCVar swf_textStrokeSizeGlyphSpacer;
extern idCVar in_useJoystick;

#define ALPHA_EPSILON	0.001f

#define STENCIL_DECR -1
#define STENCIL_INCR -2

/*
========================
idSWF::DrawStretchPic
========================
*/
void idSWF::DrawStretchPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, const idMaterial* material )
{
	renderSystem->DrawStretchPic( x * scaleToVirtual.x, y * scaleToVirtual.y, w * scaleToVirtual.x, h * scaleToVirtual.y, s1, t1, s2, t2, material );
}

/*
========================
idSWF::DrawStretchPic
========================
*/
void idSWF::DrawStretchPic( const idVec4& topLeft, const idVec4& topRight, const idVec4& bottomRight, const idVec4& bottomLeft, const idMaterial* material )
{
	renderSystem->DrawStretchPic(
		idVec4( topLeft.x * scaleToVirtual.x, topLeft.y * scaleToVirtual.y, topLeft.z, topLeft.w ),
		idVec4( topRight.x * scaleToVirtual.x, topRight.y * scaleToVirtual.y, topRight.z, topRight.w ),
		idVec4( bottomRight.x * scaleToVirtual.x, bottomRight.y * scaleToVirtual.y, bottomRight.z, bottomRight.w ),
		idVec4( bottomLeft.x * scaleToVirtual.x, bottomLeft.y * scaleToVirtual.y, bottomLeft.z, bottomLeft.w ),
		material );
}

/*
========================
idSWF::Render
========================
*/
void idSWF::Render( idRenderSystem* gui, int time, bool isSplitscreen )
{
	if( !IsLoaded() )
	{
		return;
	}
	if( !IsActive() )
	{
		return;
	}
	if( swf_stopat.GetInteger() > 0 )
	{
		if( mainspriteInstance->currentFrame == swf_stopat.GetInteger() )
		{
			swf_timescale.SetFloat( 0.0f );
		}
	}

	int currentTime = Sys_Milliseconds();
	int framesToRun = 0;

	if( paused )
	{
		lastRenderTime = currentTime;
	}

	if( swf_timescale.GetFloat() > 0.0f )
	{
		if( lastRenderTime == 0 )
		{
			lastRenderTime = currentTime;
			framesToRun = 1;
		}
		else
		{
			float deltaTime = ( currentTime - lastRenderTime );
			float fr = ( ( float )frameRate / 256.0f ) * swf_timescale.GetFloat();
			framesToRun = idMath::Ftoi( ( fr * deltaTime ) / 1000.0f );
			lastRenderTime += ( framesToRun * ( 1000.0f / fr ) );
			if( framesToRun > 10 )
			{
				framesToRun = 10;
			}
		}
		for( int i = 0; i < framesToRun; i++ )
		{
			mainspriteInstance->Run();
			mainspriteInstance->RunActions();
		}
	}

	const float pixelAspect = renderSystem->GetPixelAspect();
	const float sysWidth = renderSystem->GetWidth() * ( pixelAspect > 1.0f ? pixelAspect : 1.0f );
	const float sysHeight = renderSystem->GetHeight() / ( pixelAspect < 1.0f ? pixelAspect : 1.0f );
	float scale = swfScale * sysHeight / ( float )frameHeight;

	swfRenderState_t renderState;
	renderState.stereoDepth = ( stereoDepthType_t )mainspriteInstance->GetStereoDepth();
	renderState.matrix.xx = scale;
	renderState.matrix.yy = scale;
	renderState.matrix.tx = 0.5f * ( sysWidth - ( frameWidth * scale ) );
	renderState.matrix.ty = 0.5f * ( sysHeight - ( frameHeight * scale ) );

	renderBorder = renderState.matrix.tx / scale;

	scaleToVirtual.Set( ( float )renderSystem->GetVirtualWidth() / sysWidth, ( float )renderSystem->GetVirtualHeight() / sysHeight );

	RenderSprite( gui, mainspriteInstance, renderState, time, isSplitscreen );

	if( blackbars )
	{
		float barWidth = renderState.matrix.tx + 0.5f;
		float barHeight = renderState.matrix.ty + 0.5f;
		if( barWidth > 0.0f )
		{
			gui->SetColor( idVec4( 0.0f, 0.0f, 0.0f, 1.0f ) );
			DrawStretchPic( 0.0f, 0.0f, barWidth, sysHeight, 0, 0, 1, 1, white );
			DrawStretchPic( sysWidth - barWidth, 0.0f, barWidth, sysHeight, 0, 0, 1, 1, white );
		}
		if( barHeight > 0.0f )
		{
			gui->SetColor( idVec4( 0.0f, 0.0f, 0.0f, 1.0f ) );
			DrawStretchPic( 0.0f, 0.0f, sysWidth, barHeight, 0, 0, 1, 1, white );
			DrawStretchPic( 0.0f, sysHeight - barHeight, sysWidth, barHeight, 0, 0, 1, 1, white );
		}
	}

	if( isMouseInClientArea && ( mouseEnabled && useMouse ) && ( InhibitControl() || ( !InhibitControl() && !useInhibtControl ) ) )
	{
		gui->SetGLState( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
		gui->SetColor( idVec4( 1.0f, 1.0f, 1.0f, 1.0f ) );
		idVec2 mouse = renderState.matrix.Transform( idVec2( mouseX - 1, mouseY - 2 ) );
		//idSWFScriptObject * hitObject = HitTest( mainspriteInstance, swfRenderState_t(), mouseX, mouseY, NULL );
		if( !hasHitObject )    //hitObject == NULL ) {
		{
			DrawStretchPic( mouse.x, mouse.y, 32.0f, 32.0f, 0, 0, 1, 1, guiCursor_arrow );
		}
		else
		{
			DrawStretchPic( mouse.x, mouse.y, 32.0f, 32.0f, 0, 0, 1, 1, guiCursor_hand );
		}
	}

	// restore the GL State
	gui->SetGLState( 0 );
}

/*
========================
idSWF::RenderMask
========================
*/
void idSWF::RenderMask( idRenderSystem* gui, const swfDisplayEntry_t* mask, const swfRenderState_t& renderState, const int stencilMode )
{
	swfRenderState_t renderState2;
	renderState2.stereoDepth = renderState.stereoDepth;
	renderState2.matrix = mask->matrix.Multiply( renderState.matrix );
	renderState2.cxf = mask->cxf.Multiply( renderState.cxf );
	renderState2.ratio = mask->ratio;
	renderState2.material = guiSolid;
	renderState2.activeMasks = stencilMode;

	idSWFDictionaryEntry& entry = dictionary[ mask->characterID ];
	if( entry.type == SWF_DICT_SHAPE )
	{
		RenderShape( gui, entry.shape, renderState2 );
	}
	else if( entry.type == SWF_DICT_MORPH )
	{
		RenderMorphShape( gui, entry.shape, renderState2 );
	}
}

/*
========================
idSWF::RenderSprite
========================
*/
void idSWF::RenderSprite( idRenderSystem* gui, idSWFSpriteInstance* spriteInstance, const swfRenderState_t& renderState, int time, bool isSplitscreen )
{

	if( spriteInstance == NULL )
	{
		idLib::Warning( "%s: RenderSprite: spriteInstance == NULL", filename.c_str() );
		return;
	}
	if( !spriteInstance->isVisible )
	{
		return;
	}
	if( ( ( renderState.cxf.mul.w + renderState.cxf.add.w ) <= ALPHA_EPSILON ) && ( swf_forceAlpha.GetFloat() <= 0.0f ) )
	{
		return;
	}

	idStaticList<const swfDisplayEntry_t*, 256> activeMasks;

	for( int i = 0; i < spriteInstance->displayList.Num(); i++ )
	{
		const swfDisplayEntry_t& display = spriteInstance->displayList[i];

		for( int j = 0; j < activeMasks.Num(); j++ )
		{
			const swfDisplayEntry_t* mask = activeMasks[ j ];
			if( display.depth > mask->clipDepth )
			{
				RenderMask( gui, mask, renderState, STENCIL_DECR );
				activeMasks.RemoveIndexFast( j );
			}
		}
		if( display.clipDepth > 0 )
		{
			activeMasks.Append( &display );
			RenderMask( gui, &display, renderState, STENCIL_INCR );
			continue;
		}
		idSWFDictionaryEntry* entry = FindDictionaryEntry( display.characterID );
		if( entry == NULL )
		{
			continue;
		}

		swfRenderState_t renderState2;

		if( spriteInstance->stereoDepth != STEREO_DEPTH_TYPE_NONE )
		{
			renderState2.stereoDepth = ( stereoDepthType_t )spriteInstance->stereoDepth;
		}
		else if( renderState.stereoDepth != STEREO_DEPTH_TYPE_NONE )
		{
			renderState2.stereoDepth = renderState.stereoDepth;
		}

		renderState2.matrix = display.matrix.Multiply( renderState.matrix );
		renderState2.cxf = display.cxf.Multiply( renderState.cxf );
		renderState2.ratio = display.ratio;
		if( display.blendMode != 0 )
		{
			renderState2.blendMode = display.blendMode;
		}
		else
		{
			renderState2.blendMode = renderState.blendMode;
		}
		renderState2.activeMasks = renderState.activeMasks + activeMasks.Num();

		if( spriteInstance->materialOverride != NULL )
		{
			renderState2.material = spriteInstance->materialOverride;
			renderState2.materialWidth = spriteInstance->materialWidth;
			renderState2.materialHeight = spriteInstance->materialHeight;
		}
		else
		{
			renderState2.material = renderState.material;
			renderState2.materialWidth = renderState.materialWidth;
			renderState2.materialHeight = renderState.materialHeight;
		}

		float xOffset = 0.0f;
		float yOffset = 0.0f;

		if( entry->type == SWF_DICT_SPRITE )
		{
			display.spriteInstance->SetAlignment( spriteInstance->xOffset, spriteInstance->yOffset );

			if( display.spriteInstance->name[0] == '_' )
			{
				//if ( display.spriteInstance->name.Icmp( "_leftAlign" ) == 0 ) {
				//	float adj = (float)frameWidth  * 0.10;
				//	renderState2.matrix.tx = ( display.matrix.tx - adj ) * renderState.matrix.xx;
				//}
				//if ( display.spriteInstance->name.Icmp( "_rightAlign" ) == 0 ) {
				//	renderState2.matrix.tx = ( (float)renderSystem->GetWidth() - ( ( (float)frameWidth - display.matrix.tx - adj ) * renderState.matrix.xx ) );
				//}

				float widthAdj = swf_titleSafe.GetFloat() * frameWidth;
				float heightAdj = swf_titleSafe.GetFloat() * frameHeight;

				const float pixelAspect = renderSystem->GetPixelAspect();
				const float sysWidth = renderSystem->GetWidth() * ( pixelAspect > 1.0f ? pixelAspect : 1.0f );
				const float sysHeight = renderSystem->GetHeight() / ( pixelAspect < 1.0f ? pixelAspect : 1.0f );

				if( display.spriteInstance->name.Icmp( "_fullScreen" ) == 0 )
				{
					renderState2.matrix.tx = display.matrix.tx * renderState.matrix.xx;
					renderState2.matrix.ty = display.matrix.ty * renderState.matrix.yy;

					float xScale = sysWidth / ( float )frameWidth;
					float yScale = sysHeight / ( float )frameHeight;

					renderState2.matrix.xx = xScale;
					renderState2.matrix.yy = yScale;
				}

				if( display.spriteInstance->name.Icmp( "_absTop" ) == 0 )
				{
					renderState2.matrix.ty = display.matrix.ty * renderState.matrix.yy;
					display.spriteInstance->SetAlignment( spriteInstance->xOffset + xOffset, spriteInstance->yOffset + yOffset );
				}
				else if( display.spriteInstance->name.Icmp( "_top" ) == 0 )
				{
					renderState2.matrix.ty = ( display.matrix.ty + heightAdj ) * renderState.matrix.yy;
					display.spriteInstance->SetAlignment( spriteInstance->xOffset + xOffset, spriteInstance->yOffset + yOffset );
				}
				else if( display.spriteInstance->name.Icmp( "_topLeft" ) == 0 )
				{
					renderState2.matrix.tx = ( display.matrix.tx + widthAdj ) * renderState.matrix.xx;
					renderState2.matrix.ty = ( display.matrix.ty + heightAdj ) * renderState.matrix.yy;
					display.spriteInstance->SetAlignment( spriteInstance->xOffset + xOffset, spriteInstance->yOffset + yOffset );
				}
				else if( display.spriteInstance->name.Icmp( "_left" ) == 0 )
				{
					float prevX = renderState2.matrix.tx;
					renderState2.matrix.tx = ( display.matrix.tx + widthAdj ) * renderState.matrix.xx;
					xOffset = ( ( renderState2.matrix.tx - prevX ) / renderState.matrix.xx );
					display.spriteInstance->SetAlignment( spriteInstance->xOffset + xOffset, spriteInstance->yOffset + yOffset );
				}
				else if( idStr::FindText( display.spriteInstance->name, "_absLeft", false ) >= 0 )
				{
					float prevX = renderState2.matrix.tx;
					renderState2.matrix.tx = display.matrix.tx * renderState.matrix.xx;
					xOffset = ( ( renderState2.matrix.tx - prevX ) / renderState.matrix.xx );
					display.spriteInstance->SetAlignment( spriteInstance->xOffset + xOffset, spriteInstance->yOffset + yOffset );
				}
				else if( display.spriteInstance->name.Icmp( "_bottomLeft" ) == 0 )
				{
					float prevX = renderState2.matrix.tx;
					renderState2.matrix.tx = ( display.matrix.tx + widthAdj ) * renderState.matrix.xx;
					xOffset = ( ( renderState2.matrix.tx - prevX ) / renderState.matrix.xx );


					float prevY = renderState2.matrix.ty;
					renderState2.matrix.ty = ( ( float )sysHeight - ( ( ( float )frameHeight - display.matrix.ty + heightAdj ) * renderState.matrix.yy ) );
					yOffset = ( ( renderState2.matrix.ty - prevY ) / renderState.matrix.yy );

					display.spriteInstance->SetAlignment( spriteInstance->xOffset + xOffset, spriteInstance->yOffset + yOffset );
				}
				else if( display.spriteInstance->name.Icmp( "_absBottom" ) == 0 )
				{
					renderState2.matrix.ty = ( ( float )sysHeight - ( ( ( float )frameHeight - display.matrix.ty ) * renderState.matrix.yy ) );
					display.spriteInstance->SetAlignment( spriteInstance->xOffset + xOffset, spriteInstance->yOffset + yOffset );
				}
				else if( display.spriteInstance->name.Icmp( "_bottom" ) == 0 )
				{
					renderState2.matrix.ty = ( ( float )sysHeight - ( ( ( float )frameHeight - display.matrix.ty + heightAdj ) * renderState.matrix.yy ) );
					display.spriteInstance->SetAlignment( spriteInstance->xOffset + xOffset, spriteInstance->yOffset + yOffset );
				}
				else if( display.spriteInstance->name.Icmp( "_topRight" ) == 0 )
				{
					renderState2.matrix.tx = ( ( float )sysWidth - ( ( ( float )frameWidth - display.matrix.tx + widthAdj ) * renderState.matrix.xx ) );
					renderState2.matrix.ty = ( display.matrix.ty + heightAdj ) * renderState.matrix.yy;
					display.spriteInstance->SetAlignment( spriteInstance->xOffset + xOffset, spriteInstance->yOffset + yOffset );
				}
				else if( display.spriteInstance->name.Icmp( "_right" ) == 0 )
				{
					float prevX = renderState2.matrix.tx;
					renderState2.matrix.tx = ( ( float )sysWidth - ( ( ( float )frameWidth - display.matrix.tx + widthAdj ) * renderState.matrix.xx ) );
					xOffset = ( ( renderState2.matrix.tx - prevX ) / renderState.matrix.xx );
					display.spriteInstance->SetAlignment( spriteInstance->xOffset + xOffset, spriteInstance->yOffset + yOffset );
				}
				else if( idStr::FindText( display.spriteInstance->name, "_absRight", true ) >= 0 )
				{
					float prevX = renderState2.matrix.tx;
					renderState2.matrix.tx = ( ( float )sysWidth - ( ( ( float )frameWidth - display.matrix.tx ) * renderState.matrix.xx ) );
					xOffset = ( ( renderState2.matrix.tx - prevX ) / renderState.matrix.xx );
					display.spriteInstance->SetAlignment( spriteInstance->xOffset + xOffset, spriteInstance->yOffset + yOffset );
				}
				else if( display.spriteInstance->name.Icmp( "_bottomRight" ) == 0 )
				{
					renderState2.matrix.tx = ( ( float )sysWidth - ( ( ( float )frameWidth - display.matrix.tx + widthAdj ) * renderState.matrix.xx ) );
					renderState2.matrix.ty = ( ( float )sysHeight - ( ( ( float )frameHeight - display.matrix.ty + heightAdj ) * renderState.matrix.yy ) );
					display.spriteInstance->SetAlignment( spriteInstance->xOffset + xOffset, spriteInstance->yOffset + yOffset );
				}
				else if( display.spriteInstance->name.Icmp( "_absTopLeft" ) == 0 )  	// ABSOLUTE CORNERS OF SCREEN
				{
					renderState2.matrix.tx = display.matrix.tx * renderState.matrix.xx;
					renderState2.matrix.ty = display.matrix.ty * renderState.matrix.yy;
					display.spriteInstance->SetAlignment( spriteInstance->xOffset + xOffset, spriteInstance->yOffset + yOffset );
				}
				else if( display.spriteInstance->name.Icmp( "_absTopRight" ) == 0 )
				{
					renderState2.matrix.tx = ( ( float )sysWidth - ( ( ( float )frameWidth - display.matrix.tx ) * renderState.matrix.xx ) );
					renderState2.matrix.ty = display.matrix.ty * renderState.matrix.yy;
					display.spriteInstance->SetAlignment( spriteInstance->xOffset + xOffset, spriteInstance->yOffset + yOffset );
				}
				else if( display.spriteInstance->name.Icmp( "_absBottomLeft" ) == 0 )
				{
					renderState2.matrix.tx = display.matrix.tx * renderState.matrix.xx;
					renderState2.matrix.ty = ( ( float )sysHeight - ( ( ( float )frameHeight - display.matrix.ty ) * renderState.matrix.yy ) );
					display.spriteInstance->SetAlignment( spriteInstance->xOffset + xOffset, spriteInstance->yOffset + yOffset );
				}
				else if( display.spriteInstance->name.Icmp( "_absBottomRight" ) == 0 )
				{
					renderState2.matrix.tx = ( ( float )sysWidth - ( ( ( float )frameWidth - display.matrix.tx ) * renderState.matrix.xx ) );
					renderState2.matrix.ty = ( ( float )sysHeight - ( ( ( float )frameHeight - display.matrix.ty ) * renderState.matrix.yy ) );
					display.spriteInstance->SetAlignment( spriteInstance->xOffset + xOffset, spriteInstance->yOffset + yOffset );
				}
			}

			RenderSprite( gui, display.spriteInstance, renderState2, time, isSplitscreen );
		}
		else if( entry->type == SWF_DICT_SHAPE )
		{
			RenderShape( gui, entry->shape, renderState2 );
		}
		else if( entry->type == SWF_DICT_MORPH )
		{
			RenderMorphShape( gui, entry->shape, renderState2 );
		}
		else if( entry->type == SWF_DICT_EDITTEXT )
		{
			RenderEditText( gui, display.textInstance, renderState2, time, isSplitscreen );
		}
		else
		{
			//idLib::Warning( "%s: Tried to render an unrenderable character %d", filename.c_str(), entry->type );
		}
	}

	// RB begin
	if( swf_show.GetInteger() > 0 && !spriteInstance->name.IsEmpty() )//Icmp( "buttonBar" ) == 0 )
	{
		swfRect_t rect = CalcRect( spriteInstance, renderState );

		DrawRect( gui, rect, colorRed );

		if( swf_show.GetInteger() > 1 )
		{
			idVec4 color = colorWhite;

			if( spriteInstance->parent != NULL && spriteInstance->parent == mainspriteInstance )
			{
				color = colorCyan;
			}

			idStr str;
			//str = display.spriteInstance->name.c_str();
			sprintf( str, "%s\n%s", spriteInstance->name.c_str(), GetName() );

			DrawText( gui, str, 0.35f, 0, color, swfRect_t( rect.tl.x, rect.tl.y, 300, 40 ), false );
			//DrawText( gui, str, 0.25 * 2, 0, colorWhite, swfRect_t( rect.tl.x, rect.tl.y, 300, 40 ), false );
		}
	}
	// RB end

	for( int j = 0; j < activeMasks.Num(); j++ )
	{
		const swfDisplayEntry_t* mask = activeMasks[ j ];
		RenderMask( gui, mask, renderState, STENCIL_DECR );
	}
}

/*
========================
idSWF::GLStateForBlendMode
========================
*/
uint64 idSWF::GLStateForRenderState( const swfRenderState_t& renderState )
{
	uint64 extraGLState = GLS_OVERRIDE | GLS_DEPTHFUNC_LESS | GLS_DEPTHMASK; // SWF GL State always overrides what's set in the material

	if( renderState.activeMasks > 0 )
	{
		extraGLState |= GLS_STENCIL_FUNC_EQUAL | GLS_STENCIL_MAKE_REF( 128 + renderState.activeMasks ) | GLS_STENCIL_MAKE_MASK( 255 );
	}
	else if( renderState.activeMasks == STENCIL_INCR )
	{
		return GLS_COLORMASK | GLS_ALPHAMASK | GLS_STENCIL_OP_FAIL_KEEP | GLS_STENCIL_OP_ZFAIL_KEEP | GLS_STENCIL_OP_PASS_INCR;
	}
	else if( renderState.activeMasks == STENCIL_DECR )
	{
		return GLS_COLORMASK | GLS_ALPHAMASK | GLS_STENCIL_OP_FAIL_KEEP | GLS_STENCIL_OP_ZFAIL_KEEP | GLS_STENCIL_OP_PASS_DECR;
	}

	switch( renderState.blendMode )
	{
		case 7: // difference : dst = abs( dst - src )
		case 9: // subtract : dst = dst - src
			return extraGLState | ( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_BLENDOP_SUB );
		case 8: // add : dst = dst + src
			return extraGLState | ( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );
		case 6: // darken : dst = min( dst, src )
			return extraGLState | ( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_BLENDOP_MIN );
		case 5: // lighten : dst = max( dst, src )
			return extraGLState | ( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_BLENDOP_MAX );
		case 4: // screen : dst = dst + src - dst*src ( we only do dst - dst * src, we could do the extra + src with another pass if we need to)
			return extraGLState | ( GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ONE | GLS_BLENDOP_SUB );
		case 14: // hardlight : src < 0.5 ? multiply : screen
		case 13: // overlay : dst < 0.5 ? multiply  : screen
		case 3: // multiply : dst = ( dst * src ) + ( dst * (1-src.a) )
			return extraGLState | ( GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
		case 12: // erase
		case 11: // alpha
		case 10: // invert
		case 2: // layer
		case 1: // normal
		case 0: // normaler
		default:
			return extraGLState | ( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
	}
}

/*
========================
idSWF::RenderMorphShape
========================
*/
void idSWF::RenderMorphShape( idRenderSystem* gui, const idSWFShape* shape, const swfRenderState_t& renderState )
{
	if( shape == NULL )
	{
		idLib::Warning( "%s: RenderMorphShape: shape == NULL", filename.c_str() );
		return;
	}

	for( int i = 0; i < shape->fillDraws.Num(); i++ )
	{
		const idSWFShapeDrawFill& fill = shape->fillDraws[i];
		const idMaterial* material = NULL;
		swfColorXform_t color;

		if( renderState.material != NULL )
		{
			material = renderState.material;
		}
		else if( fill.style.type == 0 )
		{
			material = guiSolid;
			idVec4 startColor = fill.style.startColor.ToVec4();
			idVec4 endColor = fill.style.endColor.ToVec4();
			color.mul = Lerp( startColor, endColor, renderState.ratio );
		}
		else if( fill.style.type == 4 && fill.style.bitmapID != 65535 )
		{
			material = dictionary[ fill.style.bitmapID ].material;
		}
		else
		{
			material = guiSolid;
		}
		color = color.Multiply( renderState.cxf );
		if( swf_forceAlpha.GetFloat() > 0.0f )
		{
			color.mul.w = swf_forceAlpha.GetFloat();
			color.add.w = 0.0f;
		}
		if( ( color.mul.w + color.add.w ) <= ALPHA_EPSILON )
		{
			continue;
		}
		uint32 packedColorM = LittleLong( PackColor( color.mul ) );
		uint32 packedColorA = LittleLong( PackColor( ( color.add * 0.5f ) + idVec4( 0.5f ) ) ); // Compress from -1..1 to 0..1

		swfRect_t bounds;
		bounds.tl = Lerp( shape->startBounds.tl, shape->endBounds.tl, renderState.ratio );
		bounds.br = Lerp( shape->startBounds.br, shape->endBounds.br, renderState.ratio );
		idVec2 size( material->GetImageWidth(), material->GetImageHeight() );
		if( renderState.materialWidth > 0 )
		{
			size.x = renderState.materialWidth;
		}
		if( renderState.materialHeight > 0 )
		{
			size.y = renderState.materialHeight;
		}
		idVec2 oneOverSize( 1.0f / size.x, 1.0f / size.y );

		swfMatrix_t styleMatrix;
		styleMatrix.xx = Lerp( fill.style.startMatrix.xx, fill.style.endMatrix.xx, renderState.ratio );
		styleMatrix.yy = Lerp( fill.style.startMatrix.yy, fill.style.endMatrix.yy, renderState.ratio );
		styleMatrix.xy = Lerp( fill.style.startMatrix.xy, fill.style.endMatrix.xy, renderState.ratio );
		styleMatrix.yx = Lerp( fill.style.startMatrix.yx, fill.style.endMatrix.yx, renderState.ratio );
		styleMatrix.tx = Lerp( fill.style.startMatrix.tx, fill.style.endMatrix.tx, renderState.ratio );
		styleMatrix.ty = Lerp( fill.style.startMatrix.ty, fill.style.endMatrix.ty, renderState.ratio );

		swfMatrix_t invMatrix = styleMatrix.Inverse();

		gui->SetGLState( GLStateForRenderState( renderState ) );

		idDrawVert* verts = gui->AllocTris( fill.startVerts.Num(), fill.indices.Ptr(), fill.indices.Num(), material, renderState.stereoDepth );
		if( verts == NULL )
		{
			continue;
		}

		for( int j = 0; j < fill.startVerts.Num(); j++ )
		{
			idVec2 xy = Lerp( fill.startVerts[j], fill.endVerts[j], renderState.ratio );

			idVec2 st;
			st.x = ( ( xy.x - bounds.tl.x ) * oneOverSize.x ) * 20.0f;
			st.y = ( ( xy.y - bounds.tl.y ) * oneOverSize.y ) * 20.0f;
			idVec2 adjust( 0.5f * oneOverSize.x, 0.5f * oneOverSize.y );

			ALIGNTYPE16 idDrawVert tempVert;

			tempVert.Clear();
			tempVert.xyz.ToVec2() = renderState.matrix.Transform( xy ).Scale( scaleToVirtual );
			tempVert.xyz.z = 0.0f;
			tempVert.SetTexCoord( invMatrix.Transform( st ) + adjust );
			tempVert.SetNativeOrderColor( packedColorM );
			tempVert.SetNativeOrderColor2( packedColorA );

			WriteDrawVerts16( & verts[j], & tempVert, 1 );
		}
	}
}

/*
========================
idSWF::RenderShape
========================
*/
void idSWF::RenderShape( idRenderSystem* gui, const idSWFShape* shape, const swfRenderState_t& renderState )
{
	if( shape == NULL )
	{
		idLib::Warning( "%s: RenderShape: shape == NULL", filename.c_str() );
		return;
	}

	for( int i = 0; i < shape->fillDraws.Num(); i++ )
	{
		const idSWFShapeDrawFill& fill = shape->fillDraws[i];
		const idMaterial* material = NULL;
		swfColorXform_t color;

		swfMatrix_t invMatrix;
		idVec2	atlasScale( 0.0f, 0.0f );
		idVec2	atlasBias( 0.0f, 0.0f );
		bool	useAtlas = false;

		idVec2 size( 1.0f, 1.0f );

		if( renderState.material != NULL )
		{
			material = renderState.material;
			invMatrix.xx = invMatrix.yy = ( 1.0f / 20.0f );
		}
		else if( fill.style.type == 0 )
		{
			// RB begin
			if( swf_skipSolids.GetBool() )
			{
				continue;
			}
			// RB end

			material = guiSolid;
			color.mul = fill.style.startColor.ToVec4();
		}
		else if( fill.style.type == 4 && fill.style.bitmapID != 65535 )
		{
			// RB begin
			if( swf_skipBitmaps.GetBool() )
			{
				continue;
			}
			// RB end

			// everything in a single image atlas
			idSWFDictionaryEntry* entry = &dictionary[ fill.style.bitmapID ];
			material = atlasMaterial;
			idVec2i	atlasSize( material->GetImageWidth(), material->GetImageHeight() );
			for( int i = 0 ; i < 2 ; i++ )
			{
				size[i] = entry->imageSize[i];
				atlasScale[i] = ( float )size[i] / atlasSize[i];
				atlasBias[i] = ( float )entry->imageAtlasOffset[i] / atlasSize[i];
			}
			// de-normalize color channels after DXT decompression
			color.mul = entry->channelScale;
			useAtlas = true;

			const swfMatrix_t& styleMatrix = fill.style.startMatrix;
			invMatrix = styleMatrix.Inverse();
		}
		else
		{
			// RB begin
			if( fill.style.type == 1 && swf_skipGradients.GetBool() )
			{
				continue;
			}
			// RB end

			material = guiSolid;
		}
		color = color.Multiply( renderState.cxf );
		if( swf_forceAlpha.GetFloat() > 0.0f )
		{
			color.mul.w = swf_forceAlpha.GetFloat();
			color.add.w = 0.0f;
		}
		if( ( color.mul.w + color.add.w ) <= ALPHA_EPSILON )
		{
			continue;
		}

		uint32 packedColorM = LittleLong( PackColor( color.mul ) );
		uint32 packedColorA = LittleLong( PackColor( ( color.add * 0.5f ) + idVec4( 0.5f ) ) ); // Compress from -1..1 to 0..1

		const swfRect_t& bounds = shape->startBounds;
		if( renderState.materialWidth > 0 )
		{
			size.x = renderState.materialWidth;
		}
		if( renderState.materialHeight > 0 )
		{
			size.y = renderState.materialHeight;
		}
		idVec2 oneOverSize( 1.0f / size.x, 1.0f / size.y );

		gui->SetGLState( GLStateForRenderState( renderState ) );

		idDrawVert* verts = gui->AllocTris( fill.startVerts.Num(), fill.indices.Ptr(), fill.indices.Num(), material, renderState.stereoDepth );
		if( verts == NULL )
		{
			continue;
		}

		ALIGNTYPE16 idDrawVert tempVerts[4];
		for( int j = 0; j < fill.startVerts.Num(); j++ )
		{
			const idVec2& xy = fill.startVerts[j];

			idDrawVert& vert = tempVerts[j & 3];

			vert.Clear();
			vert.xyz.ToVec2() = renderState.matrix.Transform( xy ).Scale( scaleToVirtual );
			vert.xyz.z = 0.0f;
			vert.SetNativeOrderColor( packedColorM );
			vert.SetNativeOrderColor2( packedColorA );

			// For some reason I don't understand, having texcoords
			// in the range of 2000 or so causes what should be solid
			// fill areas to have horizontal bands on nvidia, but not 360.
			// Forcing the texcoords to zero fixes it.
			if( fill.style.type != 0 )
			{
				idVec2 st;
				// all the swf vertexes have an implicit scale of 1/20 for some reason...
				st.x = ( ( xy.x - bounds.tl.x ) * oneOverSize.x ) * 20.0f;
				st.y = ( ( xy.y - bounds.tl.y ) * oneOverSize.y ) * 20.0f;
				st = invMatrix.Transform( st );
				if( useAtlas )
				{
					st = st.Scale( atlasScale ) + atlasBias;
				}

				// inset the tc - the gui may use a vmtr and the tc might end up
				// crossing page boundaries if using [0.0,1.0]
				st.x = idMath::ClampFloat( 0.001f, 0.999f, st.x );
				st.y = idMath::ClampFloat( 0.001f, 0.999f, st.y );
				vert.SetTexCoord( st );
			}

			// write four verts at a time to video memory
			if( ( j & 3 ) == 3 )
			{
				WriteDrawVerts16( & verts[j & ~3], tempVerts, 4 );
			}
		}
		// write any remaining verts to video memory
		WriteDrawVerts16( & verts[fill.startVerts.Num() & ~3], tempVerts, fill.startVerts.Num() & 3 );
	}

	// RB begin
	if( !swf_skipLineDraws.GetBool() )
	{
		for( int i = 0; i < shape->lineDraws.Num(); i++ )
		{
			const idSWFShapeDrawLine& line = shape->lineDraws[i];
			swfColorXform_t color;
			color.mul = line.style.startColor.ToVec4();
			color = color.Multiply( renderState.cxf );
			if( swf_forceAlpha.GetFloat() > 0.0f )
			{
				color.mul.w = swf_forceAlpha.GetFloat();
				color.add.w = 0.0f;
			}
			if( ( color.mul.w + color.add.w ) <= ALPHA_EPSILON )
			{
				continue;
			}
			uint32 packedColorM = LittleLong( PackColor( color.mul ) );
			uint32 packedColorA = LittleLong( PackColor( ( color.add * 0.5f ) + idVec4( 0.5f ) ) ); // Compress from -1..1 to 0..1

			gui->SetGLState( GLStateForRenderState( renderState ) | GLS_POLYMODE_LINE );

			idDrawVert* verts = gui->AllocTris( line.startVerts.Num(), line.indices.Ptr(), line.indices.Num(), white, renderState.stereoDepth );
			if( verts == NULL )
			{
				continue;
			}

			for( int j = 0; j < line.startVerts.Num(); j++ )
			{
				const idVec2& xy = line.startVerts[j];

				ALIGNTYPE16 idDrawVert tempVert;

				tempVert.Clear();
				tempVert.xyz.ToVec2() = renderState.matrix.Transform( xy ).Scale( scaleToVirtual );
				tempVert.xyz.z = 0.0f;
				tempVert.SetTexCoord( 0.0f, 0.0f );
				tempVert.SetNativeOrderColor( packedColorM );
				tempVert.SetNativeOrderColor2( packedColorA );

				WriteDrawVerts16( & verts[j], & tempVert, 1 );
			}
		}
	}
	// RB end
}

/*
========================
idSWF::DrawEditCursor
========================
*/
void idSWF::DrawEditCursor( idRenderSystem* gui, float x, float y, float w, float h, const swfMatrix_t& matrix )
{
	idVec2 topl = matrix.Transform( idVec2( x, y ) );
	idVec2 topr = matrix.Transform( idVec2( x + w, y ) );
	idVec2 br = matrix.Transform( idVec2( x + w, y + h ) );
	idVec2 bl = matrix.Transform( idVec2( x, y + h ) );
	DrawStretchPic( idVec4( topl.x, topl.y, 0.0f, 0.0f ), idVec4( topr.x, topr.y, 1.0f, 0.0f ), idVec4( br.x, br.y, 1.0f, 1.0f ), idVec4( bl.x, bl.y, 0.0f, 1.0f ), white );
}

/*
========================
idSWF::RenderEditText
========================
*/
void idSWF::RenderEditText( idRenderSystem* gui, idSWFTextInstance* textInstance, const swfRenderState_t& renderState, int time, bool isSplitscreen )
{
	if( textInstance == NULL )
	{
		idLib::Warning( "%s: RenderEditText: textInstance == NULL", filename.c_str() );
		return;
	}

	if( !textInstance->visible )
	{
		return;
	}

	const idSWFEditText* shape = textInstance->editText;

	idStr text;

	if( textInstance->variable.IsEmpty() )
	{
		if( textInstance->renderMode == SWF_TEXT_RENDER_PARAGRAPH )
		{
			if( textInstance->NeedsGenerateRandomText() )
			{
				textInstance->StartParagraphText( Sys_Milliseconds() );
			}
			text = textInstance->GetParagraphText( Sys_Milliseconds() );
		}
		else if( textInstance->renderMode == SWF_TEXT_RENDER_RANDOM_APPEAR || textInstance->renderMode == SWF_TEXT_RENDER_RANDOM_APPEAR_CAPS )
		{
			if( textInstance->NeedsGenerateRandomText() )
			{
				textInstance->StartRandomText( Sys_Milliseconds() );
			}
			text = textInstance->GetRandomText( Sys_Milliseconds() );
		}
		else
		{
			text = idLocalization::GetString( textInstance->text );
		}
	}
	else
	{
		idSWFScriptVar var = globals->Get( textInstance->variable );
		if( var.IsUndefined() )
		{
			text = idLocalization::GetString( textInstance->text );
		}
		else
		{
			text = idLocalization::GetString( var.ToString() );
		}
	}

	if( text.Length() == 0 )
	{
		textInstance->selectionEnd = -1;
		textInstance->selectionStart = -1;
	}

	if( textInstance->NeedsSoundPlayed() )
	{
		PlaySound( textInstance->GetSoundClip() );
		textInstance->ClearPlaySound();
	}

	if( textInstance->tooltip )
	{
		FindTooltipIcons( &text );
	}
	else
	{
		tooltipIconList.Clear();
	}

	int selStart = textInstance->selectionStart;
	int selEnd = textInstance->selectionEnd;

	int cursorPos = selEnd;

	bool inputField = false;

	idSWFScriptVar focusWindow = globals->Get( "focusWindow" );
	if( focusWindow.IsObject() && focusWindow.GetObject() == &textInstance->scriptObject )
	{
		inputField = true;
	}

	bool drawCursor = false;

	if( inputField && ( ( idLib::frameNumber >> 4 ) & 1 ) == 0 )
	{
		cursorPos = selEnd;
		drawCursor = true;
	}
	if( selStart > selEnd )
	{
		SwapValues( selStart, selEnd );
	}

	idVec2 xScaleVec = renderState.matrix.Scale( idVec2( 1.0f, 0.0f ) );
	idVec2 yScaleVec = renderState.matrix.Scale( idVec2( 0.0f, 1.0f ) );

	float xScale = xScaleVec.Length();
	float yScale = yScaleVec.Length();

	if( isSplitscreen )
	{
		yScale *= 0.5f;
	}

	float invXScale = 1.0f / xScale;
	float invYScale = 1.0f / yScale;

	swfMatrix_t matrix = renderState.matrix;
	matrix.xx *= invXScale;
	matrix.xy *= invXScale;
	matrix.yy *= invYScale;
	matrix.yx *= invYScale;

	idSWFDictionaryEntry* fontEntry = FindDictionaryEntry( shape->fontID, SWF_DICT_FONT );
	if( fontEntry == NULL )
	{
		idLib::Warning( "idSWF::RenderEditText: NULL Font" );
		return;
	}
	idSWFFont* swfFont = fontEntry->font;

	float postTransformHeight = SWFTWIP( shape->fontHeight ) * yScale;

	const idFont* fontInfo = swfFont->fontID;

	float glyphScale = postTransformHeight / 48.0f;
	float imageScale = postTransformHeight / 24.0f;
	textInstance->glyphScale = glyphScale;

	idVec4 defaultColor = textInstance->color.ToVec4();
	defaultColor = defaultColor.Multiply( renderState.cxf.mul ) + renderState.cxf.add;
	if( swf_forceAlpha.GetFloat() > 0.0f )
	{
		defaultColor.w = swf_forceAlpha.GetFloat();
	}
	if( defaultColor.w <= ALPHA_EPSILON )
	{
		return;
	}

	idVec4 selColor( defaultColor );
	selColor.w *= 0.5f;

	gui->SetColor( defaultColor );
	gui->SetGLState( GLStateForRenderState( renderState ) );

	swfRect_t bounds;
	bounds.tl.x = xScale * ( shape->bounds.tl.x + SWFTWIP( shape->leftMargin ) );
	bounds.br.x = xScale * ( shape->bounds.br.x - SWFTWIP( shape->rightMargin ) );

	float linespacing = fontInfo->GetAscender( 1.15f * glyphScale );
	if( shape->leading != 0 )
	{
		linespacing += SWFTWIP( shape->leading );
	}

	bounds.tl.y = yScale * ( shape->bounds.tl.y + ( 1.15f * glyphScale ) );
	bounds.br.y = yScale * ( shape->bounds.br.y );

	textInstance->linespacing = linespacing;
	textInstance->bounds = bounds;

	if( shape->flags & SWF_ET_AUTOSIZE )
	{
		bounds.br.x = frameWidth;
		bounds.br.y = frameHeight;
	}

	if( drawCursor && cursorPos <= 0 )
	{
		float yPos = 0.0f;
		scaledGlyphInfo_t glyph;
		fontInfo->GetScaledGlyph( glyphScale, ' ', glyph );
		yPos = glyph.height / 2.0f;
		DrawEditCursor( gui, bounds.tl.x, yPos, 1.0f, linespacing, matrix );
	}

	if( textInstance->IsSubtitle() )
	{
		if( text.IsEmpty() && textInstance->subtitleText.IsEmpty() )
		{
			return;
		}
	}
	else if( text.IsEmpty() )
	{
		return;
	}

	float x = bounds.tl.x;
	float y = bounds.tl.y;

	int maxLines = idMath::Ftoi( ( bounds.br.y - bounds.tl.y ) / linespacing );
	if( maxLines == 0 )
	{
		maxLines = 1;
	}

	textInstance->maxLines = maxLines;

	idList< idStr > textLines;
	idStr* currentLine = &textLines.Alloc();

	// tracks the last breakable character we found
	int lastbreak = 0;
	float lastbreakX = 0;

	bool insertingImage = false;
	int iconIndex = 0;

	int charIndex = 0;

	if( textInstance->IsSubtitle() )
	{
		charIndex = textInstance->GetSubStartIndex();
	}

	while( charIndex < text.Length() )
	{
		if( text[ charIndex ] == '\n' )
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
		uint32 tc = text.UTF8Char( charIndex );
		scaledGlyphInfo_t glyph;
		fontInfo->GetScaledGlyph( glyphScale, tc, glyph );
		float glyphSkip = glyph.xSkip;
		if( textInstance->HasStroke() )
		{
			glyphSkip += ( swf_textStrokeSizeGlyphSpacer.GetFloat() * textInstance->GetStrokeWeight() * glyphScale );
		}

		tooltipIcon_t iconCheck;

		if( iconIndex < tooltipIconList.Num() )
		{
			iconCheck = tooltipIconList[iconIndex];
		}

		float imageSkip = 0.0f;

		if( charIndex - 1 == iconCheck.startIndex )
		{
			insertingImage = true;
			imageSkip = iconCheck.imageWidth * imageScale;
		}
		else if( charIndex - 1 == iconCheck.endIndex )
		{
			insertingImage = false;
			iconIndex++;
			glyphSkip = 0.0f;
		}

		if( insertingImage )
		{
			glyphSkip = 0.0f;
		}

		if( !inputField )    // only break lines of text when we are not inputting data
		{
			if( x + glyphSkip > bounds.br.x || x + imageSkip > bounds.br.x )
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
		}
		while( glyphStart < charIndex && glyphStart < text.Length() )
		{
			currentLine->Append( text[ glyphStart++ ] );
		}
		x += glyphSkip + imageSkip;
		if( tc == ' ' || tc == '-' )
		{
			lastbreak = currentLine->Length();
			lastbreakX = x;
		}
	}

	// Subtitle functionality
	if( textInstance->IsSubtitle() && textInstance->IsUpdatingSubtitle() )
	{
		if( textLines.Num() > 0 && textInstance->SubNeedsSwitch() )
		{

			int lastWordIndex = textInstance->GetApporoximateSubtitleBreak( time );
			int newEndChar = textInstance->GetSubStartIndex() + textLines[0].Length();

			int wordCount = 0;
			bool earlyOut = false;
			for( int index = 0; index < textLines[0].Length(); ++index )
			{
				if( textLines[0][index] == ' ' || textLines[0][index] == '-' )
				{
					if( index != 0 )
					{
						if( wordCount == lastWordIndex )
						{
							newEndChar = textInstance->GetSubStartIndex() + index;
							earlyOut = true;
							break;
						}

						// cover the double space at the beginning of sentences
						if( index > 0 && textLines[0][index - 1 ] != ' ' )
						{
							wordCount++;
						}
					}
				}
				else if( index == textLines[0].Length() )
				{
					if( wordCount == lastWordIndex )
					{
						newEndChar = textInstance->GetSubStartIndex() + index;
						earlyOut = true;
						break;
					}
					wordCount++;
				}
			}

			if( wordCount <= 0 && textLines[0].Length() > 0 )
			{
				wordCount = 1;
			}

			if( !earlyOut )
			{
				textInstance->LastWordChanged( wordCount, time );
			}

			textInstance->SetSubEndIndex( newEndChar, time );

			idStr subText = textLines[0].Left( newEndChar - textInstance->GetSubStartIndex() );
			idSWFParmList parms;
			parms.Append( subText );
			parms.Append( textInstance->GetSpeaker().c_str() );
			parms.Append( textInstance->GetSubAlignment() );
			Invoke( "subtitleChanged", parms );
			parms.Clear();

			textInstance->SetSubNextStartIndex( textInstance->GetSubEndIndex() );
			textInstance->SwitchSubtitleText( time );
		}

		if( !textInstance->UpdateSubtitle( time ) )
		{
			textInstance->SubtitleComplete();
			idSWFParmList parms;
			parms.Append( textInstance->GetSubAlignment() );
			Invoke( "subtitleComplete", parms );
			parms.Clear();
			textInstance->SubtitleCleanup();
		}
	}

	//*************************************************
	// CALCULATE THE NUMBER OF SCROLLS LINES LEFT
	//*************************************************

	textInstance->CalcMaxScroll( textLines.Num() - maxLines );

	int c = 1;
	int textLine = textInstance->scroll;

	if( textLine + maxLines > textLines.Num() && maxLines < textLines.Num() )
	{
		textLine = textLines.Num() - maxLines;
		textInstance->scroll = textLine;
	}
	else if( textLine < 0 || textLines.Num() <= maxLines )
	{
		textLine = 0;
		textInstance->scroll = textLine;
	}
	else if( textInstance->renderMode == SWF_TEXT_RENDER_AUTOSCROLL )
	{
		textLine = textLines.Num() - maxLines;
		textInstance->scroll = textInstance->maxscroll;
	}

	// END SCROLL CALCULATION
	//*************************************************

	int index = 0;

	int startCharacter = 0;
	int endCharacter = 0;
	int inputEndChar = 0;
	iconIndex = 0;
	int overallIndex = 0;
	int curIcon = 0;
	float yPrevBottomOffset = 0.0f;
	float yOffset = 0;

	int strokeXOffsets[] = { -1, 1, -1, 1 };
	int strokeYOffsets[] = { -1, -1, 1, 1 };

	idStr inputText;
	if( inputField )
	{
		if( textLines.Num() > 0 )
		{
			idStr& text = textLines[0];
			float left = bounds.tl.x;

			int startCheckIndex = textInstance->GetInputStartChar();

			if( startCheckIndex >= text.Length() )
			{
				startCheckIndex = 0;
			}

			if( cursorPos < startCheckIndex && cursorPos >= 0 )
			{
				startCheckIndex = cursorPos;
			}

			bool endFound = false;
			int c = startCheckIndex;
			while( c < text.Length() )
			{
				uint32 tc = text.UTF8Char( c );
				scaledGlyphInfo_t glyph;
				fontInfo->GetScaledGlyph( glyphScale, tc, glyph );
				float glyphSkip = glyph.xSkip;
				if( textInstance->HasStroke() )
				{
					glyphSkip += ( swf_textStrokeSizeGlyphSpacer.GetFloat() * textInstance->GetStrokeWeight() * glyphScale );
				}

				if( left + glyphSkip > bounds.br.x )
				{
					if( cursorPos > c && cursorPos != endCharacter )
					{

						float removeSize = 0.0f;

						while( removeSize < glyphSkip )
						{
							if( endCharacter == c )
							{
								break;
							}
							scaledGlyphInfo_t removeGlyph;
							fontInfo->GetScaledGlyph( glyphScale, inputText[ endCharacter++ ], removeGlyph );
							removeSize += removeGlyph.xSkip;
						}

						left -= removeSize;
					}
					else
					{
						inputEndChar = c;
						endFound = true;
						break;
					}
				}
				inputText.AppendUTF8Char( tc );
				left += glyphSkip;
			}

			if( !endFound )
			{
				inputEndChar = text.Length();
			}

			startCheckIndex += endCharacter;
			textInstance->SetInputStartCharacter( startCheckIndex );
			endCharacter = startCheckIndex;
		}
	}

	for( int t = 0; t < textLines.Num(); t++ )
	{

		if( textInstance->IsSubtitle() && t > 0 )
		{
			break;
		}

		if( t < textLine )
		{
			idStr& text = textLines[t];
			c += text.Length();
			startCharacter = endCharacter;
			endCharacter = startCharacter + text.Length();
			overallIndex += text.Length();

			// find the right icon index if we scrolled passed the previous ones
			for( int iconChar = curIcon; iconChar < tooltipIconList.Num(); ++iconChar )
			{
				if( endCharacter > tooltipIconList[iconChar].startIndex )
				{
					curIcon++;
				}
				else
				{
					break;
				}
			}

			continue;
		}

		if( index == maxLines )
		{
			break;
		}

		startCharacter = endCharacter;

		idStr& text = textLines[textLine];
		int lastChar = text.Length();
		if( textInstance->IsSubtitle() )
		{
			lastChar = textInstance->GetSubEndIndex();
		}

		textLine++;

		if( inputField )
		{
			if( inputEndChar == 0 )
			{
				inputEndChar += 1;
			}
			selStart -= startCharacter;
			selEnd -= startCharacter;
			cursorPos -= startCharacter;
			endCharacter = inputEndChar;
			lastChar = endCharacter;
			text = text.Mid( startCharacter, endCharacter - startCharacter );
		}
		else
		{

			if( lastChar == 0 )
			{
				// blank line so add space char
				endCharacter = startCharacter + 1;
			}
			else
			{
				endCharacter = startCharacter + lastChar;
			}
		}

		float width = 0.0f;
		insertingImage = false;
		int i = 0;
		while( i < lastChar )
		{
			if( curIcon < tooltipIconList.Num() && tooltipIconList[curIcon].startIndex == startCharacter + i )
			{
				width += tooltipIconList[curIcon].imageWidth * imageScale;
				i += tooltipIconList[curIcon].endIndex - tooltipIconList[curIcon].startIndex - 1;
				curIcon++;
			}
			else
			{
				if( i < text.Length() )
				{
					scaledGlyphInfo_t glyph;
					fontInfo->GetScaledGlyph( glyphScale, text.UTF8Char( i ), glyph );
					width += glyph.xSkip;
					if( textInstance->HasStroke() )
					{
						width += ( swf_textStrokeSizeGlyphSpacer.GetFloat() * textInstance->GetStrokeWeight() * glyphScale );
					}
				}
				else
				{
					i++;
				}
			}
		}

		y = bounds.tl.y + ( index * linespacing );

		float biggestGlyphHeight = 0.0f;
		/*for ( int image = 0; image < tooltipIconList.Num(); ++image ) {
			if ( tooltipIconList[image].startIndex >= startCharacter && tooltipIconList[image].endIndex < endCharacter ) {
				biggestGlyphHeight = tooltipIconList[image].imageHeight > biggestGlyphHeight ? tooltipIconList[image].imageHeight : biggestGlyphHeight;
			}
		}*/

		float yBottomOffset = 0.0f;
		float yTopOffset = 0.0f;

		if( biggestGlyphHeight > 0.0f )
		{

			float topSpace = 0.0f;
			float bottomSpace = 0.0f;

			int idx = 0;
			scaledGlyphInfo_t glyph;
			fontInfo->GetScaledGlyph( glyphScale, text.UTF8Char( idx ), glyph );

			topSpace = ( ( biggestGlyphHeight * imageScale ) - glyph.height ) / 2.0f;

			bottomSpace = topSpace;

			if( topSpace > 0.0f && t != 0 )
			{
				yTopOffset += topSpace;
			}

			if( bottomSpace > 0.0f )
			{
				yBottomOffset += bottomSpace;
			}
		}
		else
		{
			yBottomOffset = 0.0f;
		}

		if( t != 0 )
		{
			if( yPrevBottomOffset > 0 || yTopOffset > 0 )
			{
				yOffset += yTopOffset > yPrevBottomOffset ? yTopOffset : yPrevBottomOffset;
			}
		}

		y += yOffset;
		yPrevBottomOffset = yBottomOffset;

		float extraSpace = 0.0f;
		switch( shape->align )
		{
			case SWF_ET_ALIGN_LEFT:
				x = bounds.tl.x;
				break;
			case SWF_ET_ALIGN_RIGHT:
				x = bounds.br.x - width;
				break;
			case SWF_ET_ALIGN_CENTER:
				x = ( bounds.tl.x + bounds.br.x - width ) * 0.5f;
				break;
			case SWF_ET_ALIGN_JUSTIFY:
				x = bounds.tl.x;
				if( width > ( bounds.br.x - bounds.tl.x ) * 0.5f && index < textLines.Num() - 1 )
				{
					extraSpace = ( ( bounds.br.x - bounds.tl.x ) - width ) / ( ( float ) lastChar - 1.0f );
				}
				break;
		}

		tooltipIcon_t icon;
		insertingImage = false;

		// find the right icon index if we scrolled passed the previous ones
		for( int iconChar = iconIndex; iconChar < tooltipIconList.Num(); ++iconChar )
		{
			if( overallIndex > tooltipIconList[iconChar].startIndex )
			{
				iconIndex++;
			}
			else
			{
				break;
			}
		}

		float baseLine = y + ( fontInfo->GetAscender( glyphScale ) );

		i = 0;
		int overallLineIndex = 0;
		idVec4 textColor = defaultColor;
		while( i < lastChar )
		{

			if( i >= text.Length() )
			{
				break;
			}

			// Support colors
			if( !textInstance->ignoreColor )
			{
				if( text[ i ] == C_COLOR_ESCAPE )
				{
					if( idStr::IsColor( text.c_str() + i++ ) )
					{
						if( text[ i ] == C_COLOR_DEFAULT )
						{
							i++;
							textColor = defaultColor;
						}
						else
						{
							textColor = idStr::ColorForIndex( text[ i++ ] );
							textColor.w = defaultColor.w;
						}
						continue;
					}
				}
			}

			uint32 character = text.UTF8Char( i );

			if( character == '\n' )
			{
				c++;
				overallIndex += i - overallLineIndex;
				overallLineIndex = i;;
				continue;
			}

			// Skip a single leading space
			if( character == ' ' && i == 1 )
			{
				c++;
				overallIndex += i - overallLineIndex;
				overallLineIndex = i;
				continue;
			}

			if( iconIndex <  tooltipIconList.Num() )
			{
				icon = tooltipIconList[iconIndex];
			}

			if( overallIndex == icon.startIndex )
			{
				insertingImage = true;

				scaledGlyphInfo_t glyph;
				fontInfo->GetScaledGlyph( glyphScale, character, glyph );

				float imageHeight = icon.imageHeight * imageScale;
				float glyphHeight = glyph.height;

				float imageY = 0.0f;
				if( icon.baseline == 0 )
				{
					imageY = baseLine - glyph.top;
					imageY += ( glyphHeight - imageHeight ) * 0.5f;
					imageY += 2.0f;
				}
				else
				{
					imageY = ( y + glyphHeight ) - ( ( icon.imageHeight * imageScale ) - ( glyphHeight ) );
				}

				float imageX = x + glyph.left;
				float imageW = icon.imageWidth * imageScale;
				float imageH = icon.imageHeight * imageScale;

				idVec2 topl = matrix.Transform( idVec2( imageX, imageY ) );
				idVec2 topr = matrix.Transform( idVec2( imageX + imageW, imageY ) );
				idVec2 br = matrix.Transform( idVec2( imageX + imageW, imageY + imageH ) );
				idVec2 bl = matrix.Transform( idVec2( imageX, imageY + imageH ) );

				float s1 = 0.0f;
				float t1 = 0.0f;
				float s2 = 1.0f;
				float t2 = 1.0f;

				//uint32 color = gui->GetColor();
				idVec4 imgColor = colorWhite;
				imgColor.w = defaultColor.w;
				gui->SetColor( imgColor );
				DrawStretchPic( idVec4( topl.x, topl.y, s1, t1 ), idVec4( topr.x, topr.y, s2, t1 ), idVec4( br.x, br.y, s2, t2 ), idVec4( bl.x, bl.y, s1, t2 ), icon.material );
				gui->SetColor( defaultColor );

				x += icon.imageWidth * imageScale;
				x += extraSpace;

			}
			else if( overallIndex == icon.endIndex )
			{
				insertingImage = false;
				iconIndex++;
			}

			if( insertingImage )
			{
				overallIndex += i - overallLineIndex;
				overallLineIndex = i;
				continue;
			}

			// the glyphs texcoords assume nearest filtering, to get proper
			// bilinear support we need to go an extra half texel on each side
			scaledGlyphInfo_t glyph;
			fontInfo->GetScaledGlyph( glyphScale, character, glyph );

			float glyphSkip = glyph.xSkip;
			if( textInstance->HasStroke() )
			{
				glyphSkip += ( swf_textStrokeSizeGlyphSpacer.GetFloat() * textInstance->GetStrokeWeight() * glyphScale );
			}

			float glyphW = glyph.width + 1.0f;	// +1 for bilinear half texel on each side
			float glyphH = glyph.height + 1.0f;

			float glyphY = baseLine - glyph.top;
			float glyphX = x + glyph.left;

			idVec2 topl = matrix.Transform( idVec2( glyphX, glyphY ) );
			idVec2 topr = matrix.Transform( idVec2( glyphX + glyphW, glyphY ) );
			idVec2 br = matrix.Transform( idVec2( glyphX + glyphW, glyphY + glyphH ) );
			idVec2 bl = matrix.Transform( idVec2( glyphX, glyphY + glyphH ) );

			float s1 = glyph.s1;
			float t1 = glyph.t1;
			float s2 = glyph.s2;
			float t2 = glyph.t2;
			if( c > selStart && c <= selEnd )
			{
				idVec2 topl = matrix.Transform( idVec2( x, y ) );
				idVec2 topr = matrix.Transform( idVec2( x + glyphSkip, y ) );
				idVec2 br = matrix.Transform( idVec2( x + glyphSkip, y + linespacing ) );
				idVec2 bl = matrix.Transform( idVec2( x, y + linespacing ) );
				gui->SetColor( selColor );
				DrawStretchPic( idVec4( topl.x, topl.y, 0, 0 ), idVec4( topr.x, topr.y, 1, 0 ), idVec4( br.x, br.y, 1, 1 ), idVec4( bl.x, bl.y, 0, 1 ), white );
				gui->SetColor( textColor );
			}

			if( textInstance->GetHasDropShadow() )
			{

				float dsY = glyphY + glyphScale * 2.0f;
				float dsX = glyphX + glyphScale * 2.0f;

				idVec2 dstopl = matrix.Transform( idVec2( dsX, dsY ) );
				idVec2 dstopr = matrix.Transform( idVec2( dsX + glyphW, dsY ) );
				idVec2 dsbr = matrix.Transform( idVec2( dsX + glyphW, dsY + glyphH ) );
				idVec2 dsbl = matrix.Transform( idVec2( dsX, dsY + glyphH ) );

				idVec4 dsColor = colorBlack;
				dsColor.w = defaultColor.w;
				gui->SetColor( dsColor );
				DrawStretchPic( idVec4( dstopl.x, dstopl.y, s1, t1 ), idVec4( dstopr.x, dstopr.y, s2, t1 ), idVec4( dsbr.x, dsbr.y, s2, t2 ), idVec4( dsbl.x, dsbl.y, s1, t2 ), glyph.material );
				gui->SetColor( textColor );
			}
			else if( textInstance->HasStroke() )
			{

				idVec4 strokeColor = colorBlack;
				strokeColor.w = textInstance->GetStrokeStrength() * defaultColor.w;
				gui->SetColor( strokeColor );
				for( int index = 0; index < 4; ++index )
				{
					float xPos = glyphX + ( ( strokeXOffsets[ index ] * textInstance->GetStrokeWeight() ) * glyphScale );
					float yPos = glyphY + ( ( strokeYOffsets[ index ] * textInstance->GetStrokeWeight() ) * glyphScale );
					idVec2 topLeft = matrix.Transform( idVec2( xPos, yPos ) );
					idVec2 topRight = matrix.Transform( idVec2( xPos + glyphW, yPos ) );
					idVec2 botRight = matrix.Transform( idVec2( xPos + glyphW, yPos + glyphH ) );
					idVec2 botLeft = matrix.Transform( idVec2( xPos, yPos + glyphH ) );
					DrawStretchPic( idVec4( topLeft.x, topLeft.y, s1, t1 ), idVec4( topRight.x, topRight.y, s2, t1 ), idVec4( botRight.x, botRight.y, s2, t2 ), idVec4( botLeft.x, botLeft.y, s1, t2 ), glyph.material );
				}
				gui->SetColor( textColor );
			}

			DrawStretchPic( idVec4( topl.x, topl.y, s1, t1 ), idVec4( topr.x, topr.y, s2, t1 ), idVec4( br.x, br.y, s2, t2 ), idVec4( bl.x, bl.y, s1, t2 ), glyph.material );
			x += glyphSkip;
			x += extraSpace;
			if( cursorPos == c )
			{
				DrawEditCursor( gui, x - 1.0f, y, 1.0f, linespacing, matrix );
			}
			c++;
			overallIndex += i - overallLineIndex;
			overallLineIndex = i;
		}

		index++;
	}
}

/*
========================
idSWF::FindTooltipIcons

This replaces text like "_use" with platform specific text like "<JOY1>"
========================
*/
void idSWF::FindTooltipIcons( idStr* text )
{

	tooltipIconList.Clear();

	for( int i = UB_MAX_BUTTONS - 1; i >= 0; i-- )
	{
		//for ( userCmdString_t * ucs = userCmdStrings ; ucs->string ; ucs++ ) {
		userCmdString_t ucs = userCmdStrings[i];
		if( ucs.string && idStr::FindText( text->c_str(), ucs.string, false ) != idStr::INVALID_POSITION )
		{
			idStr replacement;

			keyBindings_t bind = idKeyInput::KeyBindingsFromBinding( ucs.string, true );
			idStr gamepad = "<";
			gamepad.Append( bind.gamepad );
			gamepad.Append( ">" );

			if( !in_useJoystick.GetBool() )
			{

				if( !bind.mouse.IsEmpty() )
				{
					replacement.Format( "<%s>", bind.mouse.c_str() );
				}
				else if( !bind.keyboard.IsEmpty() )
				{
					replacement = bind.keyboard;
				}
				if( replacement.IsEmpty() )
				{
					text->Replace( ucs.string, idStrId( "#str_swf_unbound" ).GetLocalizedString() );
				}
			}
			else
			{
				replacement = gamepad;
			}
			if( !replacement.IsEmpty() )
			{
				replacement.ToUpper();
				text->Replace( ucs.string, replacement.c_str() );
			}
		}
	}

	for( int count = 0; count < tooltipButtonImage.Num(); ++count )
	{
		int index = -1;
		while( ( index = idStr::FindText( text->c_str(), tooltipButtonImage[count].key, false, index + 1 ) ) != idStr::INVALID_POSITION )
		{
			tooltipIcon_t icon;
			icon.startIndex = index;
			icon.endIndex = index + idStr::Length( tooltipButtonImage[count].key );

			icon.material = declManager->FindMaterial( tooltipButtonImage[count].xbImage );

			if( icon.material )
			{
				icon.imageWidth = tooltipButtonImage[count].width;
				icon.imageHeight = tooltipButtonImage[count].height;
				icon.baseline = tooltipButtonImage[count].baseline;
			}
			else
			{
				icon.imageWidth = 0;
				icon.imageHeight = 0;
				icon.baseline = 0;
			}

			bool inserted = false;
			if( tooltipIconList.Num() > 0 )
			{
				for( int i = 0; i < tooltipIconList.Num(); ++i )
				{
					if( tooltipIconList[i].startIndex > icon.startIndex )
					{
						tooltipIconList.Insert( icon, i );
						inserted = true;
						break;
					}
				}
			}

			if( !inserted )
			{
				tooltipIconList.Append( icon );
			}
		}
	}
}

// RB begin
swfRect_t idSWF::CalcRect( const idSWFSpriteInstance* spriteInstance, const swfRenderState_t& renderState )
{
	swfRect_t bounds;
	bounds.tl.x = renderState.matrix.tx;
	bounds.tl.y = renderState.matrix.ty;
	bounds.br.x = renderState.matrix.tx;
	bounds.br.y = renderState.matrix.ty;

	if( spriteInstance == NULL )
	{
		idLib::Warning( "%s: CalcRect: spriteInstance == NULL", filename.c_str() );
		return bounds;
	}

#if 1
	for( int i = 0; i < spriteInstance->displayList.Num(); i++ )
	{
		const swfDisplayEntry_t& display = spriteInstance->displayList[i];

		idSWFDictionaryEntry* entry = FindDictionaryEntry( display.characterID );
		if( entry == NULL )
		{
			continue;
		}

		swfRenderState_t renderState2;
		renderState2.matrix = display.matrix.Multiply( renderState.matrix );

		if( entry->type == SWF_DICT_SPRITE )
		{
			swfRect_t spriteBounds = CalcRect( display.spriteInstance, renderState2 );

			if( spriteBounds.tl.x < bounds.tl.x )
			{
				bounds.tl.x = spriteBounds.tl.x;
			}
			else if( spriteBounds.br.x > bounds.br.x )
			{
				bounds.br.x = spriteBounds.br.x;
			}

			if( spriteBounds.tl.y < bounds.tl.y )
			{
				bounds.tl.y = spriteBounds.tl.y;
			}
			else if( spriteBounds.br.y > bounds.br.y )
			{
				bounds.br.y = spriteBounds.br.y;
			}
		}
		else if( entry->type == SWF_DICT_SHAPE )
		{
			const idSWFShape* shape = entry->shape;

			for( int i = 0; i < shape->fillDraws.Num(); i++ )
			{
				const idSWFShapeDrawFill& fill = shape->fillDraws[i];

				for( int j = 0; j < fill.startVerts.Num(); j++ )
				{
					const idVec2& xy = fill.startVerts[j];

					idVec2 p = renderState.matrix.Transform( xy );//.Scale( scaleToVirtual );

					if( p.x < bounds.tl.x )
					{
						bounds.tl.x = p.x;
					}
					else if( p.x > bounds.br.x )
					{
						bounds.br.x = p.x;
					}

					if( p.y < bounds.tl.y )
					{
						bounds.tl.y = p.y;
					}
					else if( p.y > bounds.br.y )
					{
						bounds.br.y = p.y;
					}
				}
			}
		}
		else if( entry->type == SWF_DICT_MORPH )
		{
			// TODO
		}
		else if( entry->type == SWF_DICT_EDITTEXT )
		{
			// TODO
		}
		else
		{
			//idLib::Warning( "%s: Tried to render an unrenderable character %d", filename.c_str(), entry->type );
		}
	}
#endif

	return bounds;
}

void idSWF::DrawRect( idRenderSystem* gui, const swfRect_t& rect, const idVec4& color )
{
	renderSystem->SetColor( color );

	float x = rect.tl.x;
	float y = rect.tl.y;
	float w = fabs( rect.br.x - rect.tl.x );
	float h = fabs( rect.br.y - rect.tl.y );

	float size = 1;

	DrawStretchPic( x, y, size, h, 0, 0, 0, 0, white );
	DrawStretchPic( x + w - size, y, size, h, 0, 0, 0, 0, white );
	DrawStretchPic( x, y, w, size, 0, 0, 0, 0, white );
	DrawStretchPic( x, y + h - size, w, size, 0, 0, 0, 0, white );
}

static triIndex_t quadPicIndexes[6] = { 3, 0, 2, 2, 0, 1 };
int idSWF::DrawText( idRenderSystem* gui, float x, float y, float scale, idVec4 color, const char* text, float adjust, int limit, int style )
{
	/*
	if( !matIsIdentity || cursor != -1 )
	{
		// fallback to old code
		return idDeviceContext::DrawText( x, y, scale, color, text, adjust, limit, style, cursor );
	}
	*/

	idStr drawText = text;

	if( drawText.Length() == 0 )
	{
		return 0;
	}
	if( color.w == 0.0f )
	{
		return 0;
	}

	const uint32 currentColor = PackColor( color );
	uint32 currentColorNativeByteOrder = LittleLong( currentColor );

	int len = drawText.Length();
	if( limit > 0 && len > limit )
	{
		len = limit;
	}

	int charIndex = 0;
	while( charIndex < drawText.Length() )
	{
		uint32 textChar = drawText.UTF8Char( charIndex );
		if( textChar == C_COLOR_ESCAPE )
		{
			// I'm not sure if inline text color codes are used anywhere in the game,
			// they may only be needed for multi-color user names
			idVec4		newColor;
			uint32 colorIndex = drawText.UTF8Char( charIndex );
			if( colorIndex == C_COLOR_DEFAULT )
			{
				newColor = color;
			}
			else
			{
				newColor = idStr::ColorForIndex( colorIndex );
				newColor[3] = color[3];
			}
			renderSystem->SetColor( newColor );
			currentColorNativeByteOrder = LittleLong( PackColor( newColor ) );
			continue;
		}

		scaledGlyphInfo_t glyphInfo;
		debugFont->GetScaledGlyph( scale, textChar, glyphInfo );

		// PaintChar( x, y, glyphInfo );
		float drawY = y - glyphInfo.top;
		float drawX = x + glyphInfo.left;
		float w = glyphInfo.width;
		float h = glyphInfo.height;
		float s = glyphInfo.s1;
		float t = glyphInfo.t1;
		float s2 = glyphInfo.s2;
		float t2 = glyphInfo.t2;

		float xOffset = 0;
		float yOffset = 0;

		//idDrawVert* verts = gui->AllocTris( fill.startVerts.Num(), fill.indices.Ptr(), fill.indices.Num(), material, renderState.stereoDepth );

		//if( !ClippedCoords( &drawX, &drawY, &w, &h, &s, &t, &s2, &t2 ) )
		{
			float x1 = xOffset + drawX * scaleToVirtual.x;
			float x2 = xOffset + ( drawX + w ) * scaleToVirtual.x;
			float y1 = yOffset + drawY * scaleToVirtual.y;
			float y2 = yOffset + ( drawY + h ) * scaleToVirtual.y;
			idDrawVert* verts = gui->AllocTris( 4, quadPicIndexes, 6, glyphInfo.material, STEREO_DEPTH_TYPE_NONE );
			if( verts != NULL )
			{
				verts[0].xyz[0] = x1;
				verts[0].xyz[1] = y1;
				verts[0].xyz[2] = 0.0f;
				verts[0].SetTexCoord( s, t );
				verts[0].SetNativeOrderColor( currentColorNativeByteOrder );
				verts[0].ClearColor2();

				verts[1].xyz[0] = x2;
				verts[1].xyz[1] = y1;
				verts[1].xyz[2] = 0.0f;
				verts[1].SetTexCoord( s2, t );
				verts[1].SetNativeOrderColor( currentColorNativeByteOrder );
				verts[1].ClearColor2();

				verts[2].xyz[0] = x2;
				verts[2].xyz[1] = y2;
				verts[2].xyz[2] = 0.0f;
				verts[2].SetTexCoord( s2, t2 );
				verts[2].SetNativeOrderColor( currentColorNativeByteOrder );
				verts[2].ClearColor2();

				verts[3].xyz[0] = x1;
				verts[3].xyz[1] = y2;
				verts[3].xyz[2] = 0.0f;
				verts[3].SetTexCoord( s, t2 );
				verts[3].SetNativeOrderColor( currentColorNativeByteOrder );
				verts[3].ClearColor2();
			}
		}

		x += glyphInfo.xSkip + adjust;
	}
	return drawText.Length();
}

int idSWF::DrawText( idRenderSystem* gui, const char* text, float textScale, int textAlign, idVec4 color, const swfRect_t& rectDraw, bool wrap, int cursor, bool calcOnly, idList<int>* breaks, int limit )
{
	int			count = 0;
	int			charIndex = 0;
	int			lastBreak = 0;
	float		y = 0.0f;
	float		textWidth = 0.0f;
	float		textWidthAtLastBreak = 0.0f;

	float		charSkip = idMath::Ftoi( debugFont->GetMaxCharWidth( textScale ) ) + 1;
	float		lineSkip = idMath::Ftoi( debugFont->GetMaxCharWidth( textScale ) );

	bool		lineBreak = false;
	bool		wordBreak = false;

	float		rectWidth = fabs( rectDraw.br.x - rectDraw.tl.x );
	float		rectHeight = fabs( rectDraw.br.y - rectDraw.tl.y );

	idStr drawText = text;
	idStr textBuffer;

	if( !calcOnly && !( text && *text ) )
	{
		//if( cursor == 0 )
		//{
		//	renderSystem->SetColor( color );
		//	DrawEditCursor( rectDraw.tl.x, lineSkip + rectDraw.y, textScale );
		//}
		return idMath::Ftoi( rectWidth / charSkip );
	}

	y = lineSkip + rectDraw.y();

	if( breaks )
	{
		breaks->Append( 0 );
	}

	while( charIndex < drawText.Length() )
	{
		uint32 textChar = drawText.UTF8Char( charIndex );

		// See if we need to start a new line.
		if( textChar == '\n' || textChar == '\r' || charIndex == drawText.Length() )
		{
			lineBreak = true;
			if( charIndex < drawText.Length() )
			{
				// New line character and we still have more text to read.
				char nextChar = drawText[ charIndex + 1 ];
				if( ( textChar == '\n' && nextChar == '\r' ) || ( textChar == '\r' && nextChar == '\n' ) )
				{
					// Just absorb extra newlines.
					textChar = drawText.UTF8Char( charIndex );
				}
			}
		}

		// Check for escape colors if not then simply get the glyph width.
		if( textChar == C_COLOR_ESCAPE && charIndex < drawText.Length() )
		{
			textBuffer.AppendUTF8Char( textChar );
			textChar = drawText.UTF8Char( charIndex );
		}

		// If the character isn't a new line then add it to the text buffer.
		if( textChar != '\n' && textChar != '\r' )
		{
			textWidth += debugFont->GetGlyphWidth( textScale, textChar );
			textBuffer.AppendUTF8Char( textChar );
		}

		if( !lineBreak && ( textWidth > rectWidth ) )
		{
			// The next character will cause us to overflow, if we haven't yet found a suitable
			// break spot, set it to be this character
			if( textBuffer.Length() > 0 && lastBreak == 0 )
			{
				lastBreak = textBuffer.Length();
				textWidthAtLastBreak = textWidth;
			}
			wordBreak = true;
		}
		else if( lineBreak || ( wrap && ( textChar == ' ' || textChar == '\t' ) ) )
		{
			// The next character is in view, so if we are a break character, store our position
			lastBreak = textBuffer.Length();
			textWidthAtLastBreak = textWidth;
		}

		// We need to go to a new line
		if( lineBreak || wordBreak )
		{
			float x = rectDraw.tl.x;

			if( textWidthAtLastBreak > 0 )
			{
				textWidth = textWidthAtLastBreak;
			}

			// Align text if needed
#if 0
			if( textAlign == ALIGN_RIGHT )
			{
				x = rectDraw.tl.x + rectWidth - textWidth;
			}
			else if( textAlign == ALIGN_CENTER )
			{
				x = rectDraw.tl.x + ( rectWidth - textWidth ) / 2;
			}
#endif

			if( wrap || lastBreak > 0 )
			{
				// This is a special case to handle breaking in the middle of a word.
				// if we didn't do this, the cursor would appear on the end of this line
				// and the beginning of the next.
				if( wordBreak && cursor >= lastBreak && lastBreak == textBuffer.Length() )
				{
					cursor++;
				}
			}

			// Draw what's in the current text buffer.
			if( !calcOnly )
			{
				if( lastBreak > 0 )
				{
					count += DrawText( gui, x, y, textScale, color, textBuffer.Left( lastBreak ).c_str(), 0, 0, 0 );
					textBuffer = textBuffer.Right( textBuffer.Length() - lastBreak );
				}
				else
				{
					count += DrawText( gui, x, y, textScale, color, textBuffer.c_str(), 0, 0, 0 );
					textBuffer.Clear();
				}
			}

			if( cursor < lastBreak )
			{
				cursor = -1;
			}
			else if( cursor >= 0 )
			{
				cursor -= ( lastBreak + 1 );
			}

			// If wrap is disabled return at this point.
			if( !wrap )
			{
				return lastBreak;
			}

			// If we've hit the allowed character limit then break.
			if( limit && count > limit )
			{
				break;
			}

			y += lineSkip + 5;

			if( !calcOnly && y > rectDraw.Bottom() )
			{
				break;
			}

			// If breaks were requested then make a note of this one.
			if( breaks )
			{
				breaks->Append( drawText.Length() - charIndex );
			}

			// Reset necessary parms for next line.
			lastBreak = 0;
			textWidth = 0;
			textWidthAtLastBreak = 0;
			lineBreak = false;
			wordBreak = false;

			// Reassess the remaining width
			for( int i = 0; i < textBuffer.Length(); )
			{
				if( textChar != C_COLOR_ESCAPE )
				{
					textWidth += debugFont->GetGlyphWidth( textScale, textBuffer.UTF8Char( i ) );
				}
			}

			continue;
		}
	}

	return idMath::Ftoi( rectWidth / charSkip );
}