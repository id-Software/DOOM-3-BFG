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

#include "DeviceContext.h"

#include "imgui.h"
#include "../renderer/RenderCommon.h"

extern idCVar in_useJoystick;

// bypass rendersystem to directly work on guiModel
extern idGuiModel* tr_guiModel;

idVec4 idDeviceContext::colorPurple;
idVec4 idDeviceContext::colorOrange;
idVec4 idDeviceContext::colorYellow;
idVec4 idDeviceContext::colorGreen;
idVec4 idDeviceContext::colorBlue;
idVec4 idDeviceContext::colorRed;
idVec4 idDeviceContext::colorBlack;
idVec4 idDeviceContext::colorWhite;
idVec4 idDeviceContext::colorNone;

void idDeviceContext::Init()
{
	xScale = 1.0f;
	yScale = 1.0f;
	xOffset = 0.0f;
	yOffset = 0.0f;
	whiteImage = declManager->FindMaterial( "guis/assets/white.tga" );
	whiteImage->SetSort( SS_GUI );
	activeFont = renderSystem->RegisterFont( "" );
	colorPurple = idVec4( 1, 0, 1, 1 );
	colorOrange = idVec4( 1, 1, 0, 1 );
	colorYellow = idVec4( 0, 1, 1, 1 );
	colorGreen = idVec4( 0, 1, 0, 1 );
	colorBlue = idVec4( 0, 0, 1, 1 );
	colorRed = idVec4( 1, 0, 0, 1 );
	colorWhite = idVec4( 1, 1, 1, 1 );
	colorBlack = idVec4( 0, 0, 0, 1 );
	colorNone = idVec4( 0, 0, 0, 0 );
	cursorImages[CURSOR_ARROW] = declManager->FindMaterial( "ui/assets/guicursor_arrow.tga" );
	cursorImages[CURSOR_HAND] = declManager->FindMaterial( "ui/assets/guicursor_hand.tga" );
	cursorImages[CURSOR_HAND_JOY1] = declManager->FindMaterial( "ui/assets/guicursor_hand_cross.tga" );
	cursorImages[CURSOR_HAND_JOY2] = declManager->FindMaterial( "ui/assets/guicursor_hand_circle.tga" );
	cursorImages[CURSOR_HAND_JOY3] = declManager->FindMaterial( "ui/assets/guicursor_hand_square.tga" );
	cursorImages[CURSOR_HAND_JOY4] = declManager->FindMaterial( "ui/assets/guicursor_hand_triangle.tga" );
	cursorImages[CURSOR_HAND_JOY1] = declManager->FindMaterial( "ui/assets/guicursor_hand_a.tga" );
	cursorImages[CURSOR_HAND_JOY2] = declManager->FindMaterial( "ui/assets/guicursor_hand_b.tga" );
	cursorImages[CURSOR_HAND_JOY3] = declManager->FindMaterial( "ui/assets/guicursor_hand_x.tga" );
	cursorImages[CURSOR_HAND_JOY4] = declManager->FindMaterial( "ui/assets/guicursor_hand_y.tga" );
	scrollBarImages[SCROLLBAR_HBACK] = declManager->FindMaterial( "ui/assets/scrollbarh.tga" );
	scrollBarImages[SCROLLBAR_VBACK] = declManager->FindMaterial( "ui/assets/scrollbarv.tga" );
	scrollBarImages[SCROLLBAR_THUMB] = declManager->FindMaterial( "ui/assets/scrollbar_thumb.tga" );
	scrollBarImages[SCROLLBAR_RIGHT] = declManager->FindMaterial( "ui/assets/scrollbar_right.tga" );
	scrollBarImages[SCROLLBAR_LEFT] = declManager->FindMaterial( "ui/assets/scrollbar_left.tga" );
	scrollBarImages[SCROLLBAR_UP] = declManager->FindMaterial( "ui/assets/scrollbar_up.tga" );
	scrollBarImages[SCROLLBAR_DOWN] = declManager->FindMaterial( "ui/assets/scrollbar_down.tga" );
	cursorImages[CURSOR_ARROW]->SetSort( SS_GUI );
	cursorImages[CURSOR_HAND]->SetSort( SS_GUI );
	scrollBarImages[SCROLLBAR_HBACK]->SetSort( SS_GUI );
	scrollBarImages[SCROLLBAR_VBACK]->SetSort( SS_GUI );
	scrollBarImages[SCROLLBAR_THUMB]->SetSort( SS_GUI );
	scrollBarImages[SCROLLBAR_RIGHT]->SetSort( SS_GUI );
	scrollBarImages[SCROLLBAR_LEFT]->SetSort( SS_GUI );
	scrollBarImages[SCROLLBAR_UP]->SetSort( SS_GUI );
	scrollBarImages[SCROLLBAR_DOWN]->SetSort( SS_GUI );
	cursor = CURSOR_ARROW;
	enableClipping = true;
	overStrikeMode = true;
	mat.Identity();
	matIsIdentity = true;
	origin.Zero();
	initialized = true;
}

void idDeviceContext::Shutdown()
{
	clipRects.Clear();
	Clear();
}

void idDeviceContext::Clear()
{
	initialized = false;
}

idDeviceContext::idDeviceContext()
{
	Clear();
}

void idDeviceContext::SetTransformInfo( const idVec3& org, const idMat3& m )
{
	origin = org;
	mat = m;
	matIsIdentity = mat.IsIdentity();
}

//
//  added method
void idDeviceContext::GetTransformInfo( idVec3& org, idMat3& m )
{
	m = mat;
	org = origin;
}
//

void idDeviceContext::EnableClipping( bool b )
{
	enableClipping = b;
};

void idDeviceContext::PopClipRect()
{
	if( clipRects.Num() )
	{
		clipRects.RemoveIndex( clipRects.Num() - 1 );
	}
}

void idDeviceContext::PushClipRect( idRectangle r )
{
	clipRects.Append( r );
}

bool idDeviceContext::ClippedCoords( float* x, float* y, float* w, float* h, float* s1, float* t1, float* s2, float* t2 )
{

	if( enableClipping == false || clipRects.Num() == 0 )
	{
		return false;
	}

	int c = clipRects.Num();
	while( --c > 0 )
	{
		idRectangle* clipRect = &clipRects[c];

		float ox = *x;
		float oy = *y;
		float ow = *w;
		float oh = *h;

		if( ow <= 0.0f || oh <= 0.0f )
		{
			break;
		}

		if( *x < clipRect->x )
		{
			*w -= clipRect->x - *x;
			*x = clipRect->x;
		}
		else if( *x > clipRect->x + clipRect->w )
		{
			*x = *w = *y = *h = 0;
		}
		if( *y < clipRect->y )
		{
			*h -= clipRect->y - *y;
			*y = clipRect->y;
		}
		else if( *y > clipRect->y + clipRect->h )
		{
			*x = *w = *y = *h = 0;
		}
		if( *w > clipRect->w )
		{
			*w = clipRect->w - *x + clipRect->x;
		}
		else if( *x + *w > clipRect->x + clipRect->w )
		{
			*w = clipRect->Right() - *x;
		}
		if( *h > clipRect->h )
		{
			*h = clipRect->h - *y + clipRect->y;
		}
		else if( *y + *h > clipRect->y + clipRect->h )
		{
			*h = clipRect->Bottom() - *y;
		}

		if( s1 && s2 && t1 && t2 && ow > 0.0f )
		{
			float ns1, ns2, nt1, nt2;
			// upper left
			float u = ( *x - ox ) / ow;
			ns1 = *s1 * ( 1.0f - u ) + *s2 * ( u );

			// upper right
			u = ( *x + *w - ox ) / ow;
			ns2 = *s1 * ( 1.0f - u ) + *s2 * ( u );

			// lower left
			u = ( *y - oy ) / oh;
			nt1 = *t1 * ( 1.0f - u ) + *t2 * ( u );

			// lower right
			u = ( *y + *h - oy ) / oh;
			nt2 = *t1 * ( 1.0f - u ) + *t2 * ( u );

			// set values
			*s1 = ns1;
			*s2 = ns2;
			*t1 = nt1;
			*t2 = nt2;
		}
	}

	return ( *w == 0 || *h == 0 ) ? true : false;
}

/*
=============
DrawStretchPic
=============
*/
void idDeviceContext::DrawWinding( idWinding& w, const idMaterial* mat )
{

	idPlane p;

	p.Normal().Set( 1.0f, 0.0f, 0.0f );
	p.SetDist( 0.0f );
	w.ClipInPlace( p );

	p.Normal().Set( -1.0f, 0.0f, 0.0f );
	p.SetDist( -SCREEN_WIDTH );
	w.ClipInPlace( p );

	p.Normal().Set( 0.0f, 1.0f, 0.0f );
	p.SetDist( 0.0f );
	w.ClipInPlace( p );

	p.Normal().Set( 0.0f, -1.0f, 0.0f );
	p.SetDist( -SCREEN_HEIGHT );
	w.ClipInPlace( p );

	if( w.GetNumPoints() < 3 )
	{
		return;
	}

	int numIndexes = 0;
	triIndex_t tempIndexes[( MAX_POINTS_ON_WINDING - 2 ) * 3];
	for( int j = 2; j < w.GetNumPoints(); j++ )
	{
		tempIndexes[numIndexes++] = 0;
		tempIndexes[numIndexes++] = j - 1;
		tempIndexes[numIndexes++] = j;
	}
	assert( numIndexes == ( w.GetNumPoints() - 2 ) * 3 );

	idDrawVert* verts = renderSystem->AllocTris( w.GetNumPoints(), tempIndexes, numIndexes, mat );
	if( verts == NULL )
	{
		return;
	}
	uint32 currentColor = renderSystem->GetColor();

	for( int j = 0 ; j < w.GetNumPoints() ; j++ )
	{
		verts[j].xyz.x = xOffset + w[j].x * xScale;
		verts[j].xyz.y = yOffset + w[j].y * yScale;
		verts[j].xyz.z = w[j].z;
		verts[j].SetTexCoord( w[j].s, w[j].t );
		verts[j].SetColor( currentColor );
		verts[j].ClearColor2();
		verts[j].SetNormal( 0.0f, 0.0f, 1.0f );
		verts[j].SetTangent( 1.0f, 0.0f, 0.0f );
		verts[j].SetBiTangent( 0.0f, 1.0f, 0.0f );
	}
}

void idDeviceContext::DrawStretchPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, const idMaterial* shader )
{
	if( matIsIdentity )
	{
		renderSystem->DrawStretchPic( xOffset + x * xScale, yOffset + y * yScale, w * xScale, h * yScale, s1, t1, s2, t2, shader );
		return;
	}

	idFixedWinding winding;
	winding.AddPoint( idVec5( x, y, 0.0f, s1, t1 ) );
	winding.AddPoint( idVec5( x + w, y, 0.0f, s2, t1 ) );
	winding.AddPoint( idVec5( x + w, y + h, 0.0f, s2, t2 ) );
	winding.AddPoint( idVec5( x, y + h, 0.0f, s1, t2 ) );

	for( int i = 0; i < winding.GetNumPoints(); i++ )
	{
		winding[i].ToVec3() -= origin;
		winding[i].ToVec3() *= mat;
		winding[i].ToVec3() += origin;
	}

	DrawWinding( winding, shader );
}


void idDeviceContext::DrawMaterial( float x, float y, float w, float h, const idMaterial* mat, const idVec4& color, float scalex, float scaley )
{

	renderSystem->SetColor( color );

	float	s0, s1, t0, t1;
//
//  handle negative scales as well
	if( scalex < 0 )
	{
		w *= -1;
		scalex *= -1;
	}
	if( scaley < 0 )
	{
		h *= -1;
		scaley *= -1;
	}
//
	if( w < 0 )  	// flip about vertical
	{
		w  = -w;
		s0 = 1 * scalex;
		s1 = 0;
	}
	else
	{
		s0 = 0;
		s1 = 1 * scalex;
	}

	if( h < 0 )  	// flip about horizontal
	{
		h  = -h;
		t0 = 1 * scaley;
		t1 = 0;
	}
	else
	{
		t0 = 0;
		t1 = 1 * scaley;
	}

	if( ClippedCoords( &x, &y, &w, &h, &s0, &t0, &s1, &t1 ) )
	{
		return;
	}

	DrawStretchPic( x, y, w, h, s0, t0, s1, t1, mat );
}

void idDeviceContext::DrawMaterialRotated( float x, float y, float w, float h, const idMaterial* mat, const idVec4& color, float scalex, float scaley, float angle )
{

	renderSystem->SetColor( color );

	float	s0, s1, t0, t1;
	//
	//  handle negative scales as well
	if( scalex < 0 )
	{
		w *= -1;
		scalex *= -1;
	}
	if( scaley < 0 )
	{
		h *= -1;
		scaley *= -1;
	}
	//
	if( w < 0 )  	// flip about vertical
	{
		w  = -w;
		s0 = 1 * scalex;
		s1 = 0;
	}
	else
	{
		s0 = 0;
		s1 = 1 * scalex;
	}

	if( h < 0 )  	// flip about horizontal
	{
		h  = -h;
		t0 = 1 * scaley;
		t1 = 0;
	}
	else
	{
		t0 = 0;
		t1 = 1 * scaley;
	}

	if( angle == 0.0f && ClippedCoords( &x, &y, &w, &h, &s0, &t0, &s1, &t1 ) )
	{
		return;
	}

	DrawStretchPicRotated( x, y, w, h, s0, t0, s1, t1, mat, angle );
}

void idDeviceContext::DrawStretchPicRotated( float x, float y, float w, float h, float s1, float t1, float s2, float t2, const idMaterial* shader, float angle )
{

	idFixedWinding winding;
	winding.AddPoint( idVec5( x, y, 0.0f, s1, t1 ) );
	winding.AddPoint( idVec5( x + w, y, 0.0f, s2, t1 ) );
	winding.AddPoint( idVec5( x + w, y + h, 0.0f, s2, t2 ) );
	winding.AddPoint( idVec5( x, y + h, 0.0f, s1, t2 ) );

	for( int i = 0; i < winding.GetNumPoints(); i++ )
	{
		winding[i].ToVec3() -= origin;
		winding[i].ToVec3() *= mat;
		winding[i].ToVec3() += origin;
	}

	//Generate a translation so we can translate to the center of the image rotate and draw
	idVec3 origTrans;
	origTrans.x = x + ( w / 2 );
	origTrans.y = y + ( h / 2 );
	origTrans.z = 0;


	//Rotate the verts about the z axis before drawing them
	idMat3 rotz;
	rotz.Identity();
	float sinAng, cosAng;
	idMath::SinCos( angle, sinAng, cosAng );
	rotz[0][0] = cosAng;
	rotz[0][1] = sinAng;
	rotz[1][0] = -sinAng;
	rotz[1][1] = cosAng;
	for( int i = 0; i < winding.GetNumPoints(); i++ )
	{
		winding[i].ToVec3() -= origTrans;
		winding[i].ToVec3() *= rotz;
		winding[i].ToVec3() += origTrans;
	}

	DrawWinding( winding, shader );
}

void idDeviceContext::DrawFilledRect( float x, float y, float w, float h, const idVec4& color )
{

	if( color.w == 0.0f )
	{
		return;
	}

	renderSystem->SetColor( color );

	if( ClippedCoords( &x, &y, &w, &h, NULL, NULL, NULL, NULL ) )
	{
		return;
	}

	DrawStretchPic( x, y, w, h, 0, 0, 0, 0, whiteImage );
}


void idDeviceContext::DrawRect( float x, float y, float w, float h, float size, const idVec4& color )
{

	if( color.w == 0.0f )
	{
		return;
	}

	renderSystem->SetColor( color );

	if( ClippedCoords( &x, &y, &w, &h, NULL, NULL, NULL, NULL ) )
	{
		return;
	}

	DrawStretchPic( x, y, size, h, 0, 0, 0, 0, whiteImage );
	DrawStretchPic( x + w - size, y, size, h, 0, 0, 0, 0, whiteImage );
	DrawStretchPic( x, y, w, size, 0, 0, 0, 0, whiteImage );
	DrawStretchPic( x, y + h - size, w, size, 0, 0, 0, 0, whiteImage );
}

void idDeviceContext::DrawMaterialRect( float x, float y, float w, float h, float size, const idMaterial* mat, const idVec4& color )
{

	if( color.w == 0.0f )
	{
		return;
	}

	renderSystem->SetColor( color );
	DrawMaterial( x, y, size, h, mat, color );
	DrawMaterial( x + w - size, y, size, h, mat, color );
	DrawMaterial( x, y, w, size, mat, color );
	DrawMaterial( x, y + h - size, w, size, mat, color );
}


void idDeviceContext::SetCursor( int n )
{

	if( n > CURSOR_ARROW && n < CURSOR_COUNT )
	{

		keyBindings_t binds = idKeyInput::KeyBindingsFromBinding( "_use", true );

		keyNum_t keyNum = K_NONE;
		if( in_useJoystick.GetBool() )
		{
			keyNum = idKeyInput::StringToKeyNum( binds.gamepad.c_str() );
		}

		if( keyNum != K_NONE )
		{

			if( keyNum == K_JOY1 )
			{
				cursor = CURSOR_HAND_JOY1;
			}
			else if( keyNum == K_JOY2 )
			{
				cursor = CURSOR_HAND_JOY2;
			}
			else if( keyNum == K_JOY3 )
			{
				cursor = CURSOR_HAND_JOY3;
			}
			else if( keyNum == K_JOY4 )
			{
				cursor = CURSOR_HAND_JOY4;
			}

		}
		else
		{
			cursor = CURSOR_HAND;
		}

	}
	else
	{
		cursor = CURSOR_ARROW;
	}
}

void idDeviceContext::DrawCursor( float* x, float* y, float size )
{
	if( *x < 0 )
	{
		*x = 0;
	}

	if( *x >= VIRTUAL_WIDTH )
	{
		*x = VIRTUAL_WIDTH;
	}

	if( *y < 0 )
	{
		*y = 0;
	}

	if( *y >= VIRTUAL_HEIGHT )
	{
		*y = VIRTUAL_HEIGHT;
	}

	renderSystem->SetColor( colorWhite );
	DrawStretchPic( *x, *y, size, size, 0, 0, 1, 1, cursorImages[cursor] );
}
/*
 =======================================================================================================================
 =======================================================================================================================
 */

void idDeviceContext::PaintChar( float x, float y, const scaledGlyphInfo_t& glyphInfo )
{
	y -= glyphInfo.top;
	x += glyphInfo.left;

	float w = glyphInfo.width;
	float h = glyphInfo.height;
	float s = glyphInfo.s1;
	float t = glyphInfo.t1;
	float s2 = glyphInfo.s2;
	float t2 = glyphInfo.t2;
	const idMaterial* hShader = glyphInfo.material;

	if( ClippedCoords( &x, &y, &w, &h, &s, &t, &s2, &t2 ) )
	{
		return;
	}

	DrawStretchPic( x, y, w, h, s, t, s2, t2, hShader );
}

int idDeviceContext::DrawText( float x, float y, float scale, idVec4 color, const char* text, float adjust, int limit, int style, int cursor )
{
	int			len;
	idVec4		newColor;

	idStr drawText = text;
	int charIndex = 0;

	if( text && color.w != 0.0f )
	{
		renderSystem->SetColor( color );
		memcpy( &newColor[0], &color[0], sizeof( idVec4 ) );
		len = drawText.Length();
		if( limit > 0 && len > limit )
		{
			len = limit;
		}

		float prevGlyphSkip = 0.0f;

		while( charIndex < len )
		{
			uint32 textChar = drawText.UTF8Char( charIndex );

			if( idStr::IsColor( drawText.c_str() + charIndex ) )
			{
				if( drawText[ charIndex++ ] == C_COLOR_DEFAULT )
				{
					newColor = color;
				}
				else
				{
					newColor = idStr::ColorForIndex( charIndex );
					newColor[3] = color[3];
				}
				if( cursor == charIndex - 1 || cursor == charIndex )
				{
					float backup = 0.0f;
					if( prevGlyphSkip > 0.0f )
					{
						backup = ( prevGlyphSkip + adjust ) / 5.0f;
					}
					if( cursor == charIndex - 1 )
					{
						backup *= 2.0f;
					}
					else
					{
						renderSystem->SetColor( newColor );
					}
					DrawEditCursor( x - backup, y, scale );
				}
				renderSystem->SetColor( newColor );
				continue;
			}
			else
			{
				scaledGlyphInfo_t glyphInfo;
				activeFont->GetScaledGlyph( scale, textChar, glyphInfo );
				prevGlyphSkip = glyphInfo.xSkip;

				PaintChar( x, y, glyphInfo );

				if( cursor == charIndex - 1 )
				{
					DrawEditCursor( x, y, scale );
				}
				x += glyphInfo.xSkip + adjust;
			}
		}
		if( cursor == len )
		{
			DrawEditCursor( x, y, scale );
		}
	}
	return drawText.Length();
}

void idDeviceContext::SetSize( float width, float height )
{
	xScale = VIRTUAL_WIDTH / width;
	yScale = VIRTUAL_HEIGHT / height;
}

void idDeviceContext::SetOffset( float x, float y )
{
	xOffset = x;
	yOffset = y;
}

int idDeviceContext::CharWidth( const char c, float scale )
{
	return idMath::Ftoi( activeFont->GetGlyphWidth( scale, c ) );
}

int idDeviceContext::TextWidth( const char* text, float scale, int limit )
{
	if( text == NULL )
	{
		return 0;
	}

	int i;
	float width = 0;
	if( limit > 0 )
	{
		for( i = 0; text[i] != '\0' && i < limit; i++ )
		{
			if( idStr::IsColor( text + i ) )
			{
				i++;
			}
			else
			{
				width += activeFont->GetGlyphWidth( scale, ( ( const unsigned char* )text )[i] );
			}
		}
	}
	else
	{
		for( i = 0; text[i] != '\0'; i++ )
		{
			if( idStr::IsColor( text + i ) )
			{
				i++;
			}
			else
			{
				width += activeFont->GetGlyphWidth( scale, ( ( const unsigned char* )text )[i] );
			}
		}
	}
	return idMath::Ftoi( width );
}

int idDeviceContext::TextHeight( const char* text, float scale, int limit )
{
	return idMath::Ftoi( activeFont->GetLineHeight( scale ) );
}

int idDeviceContext::MaxCharWidth( float scale )
{
	return idMath::Ftoi( activeFont->GetMaxCharWidth( scale ) );
}

int idDeviceContext::MaxCharHeight( float scale )
{
	return idMath::Ftoi( activeFont->GetLineHeight( scale ) );
}

const idMaterial* idDeviceContext::GetScrollBarImage( int index )
{
	if( index >= SCROLLBAR_HBACK && index < SCROLLBAR_COUNT )
	{
		return scrollBarImages[index];
	}
	return scrollBarImages[SCROLLBAR_HBACK];
}

// this only supports left aligned text
idRegion* idDeviceContext::GetTextRegion( const char* text, float textScale, idRectangle rectDraw, float xStart, float yStart )
{
	return NULL;
}

void idDeviceContext::DrawEditCursor( float x, float y, float scale )
{
	if( ( int )( idLib::frameNumber >> 4 ) & 1 )
	{
		return;
	}
	char cursorChar = ( overStrikeMode ) ? '_' : '|';
	scaledGlyphInfo_t glyphInfo;
	activeFont->GetScaledGlyph( scale, cursorChar, glyphInfo );
	PaintChar( x, y, glyphInfo );
}

int idDeviceContext::DrawText( const char* text, float textScale, int textAlign, idVec4 color, idRectangle rectDraw, bool wrap, int cursor, bool calcOnly, idList<int>* breaks, int limit )
{
	int			count = 0;
	int			charIndex = 0;
	int			lastBreak = 0;
	float		y = 0.0f;
	float		textWidth = 0.0f;
	float		textWidthAtLastBreak = 0.0f;

	float		charSkip = MaxCharWidth( textScale ) + 1;
	float		lineSkip = MaxCharHeight( textScale );

	bool		lineBreak = false;
	bool		wordBreak = false;

	idStr drawText = text;
	idStr textBuffer;

	if( !calcOnly && !( text && *text ) )
	{
		if( cursor == 0 )
		{
			renderSystem->SetColor( color );
			DrawEditCursor( rectDraw.x, lineSkip + rectDraw.y, textScale );
		}
		return idMath::Ftoi( rectDraw.w / charSkip );
	}

	y = lineSkip + rectDraw.y;

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
			textWidth += activeFont->GetGlyphWidth( textScale, textChar );
			textBuffer.AppendUTF8Char( textChar );
		}

		if( !lineBreak && ( textWidth > rectDraw.w ) )
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
			float x = rectDraw.x;

			if( textWidthAtLastBreak > 0 )
			{
				textWidth = textWidthAtLastBreak;
			}

			// Align text if needed
			if( textAlign == ALIGN_RIGHT )
			{
				x = rectDraw.x + rectDraw.w - textWidth;
			}
			else if( textAlign == ALIGN_CENTER )
			{
				x = rectDraw.x + ( rectDraw.w - textWidth ) / 2;
			}

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
					count += DrawText( x, y, textScale, color, textBuffer.Left( lastBreak ).c_str(), 0, 0, 0, cursor );
					textBuffer = textBuffer.Right( textBuffer.Length() - lastBreak );
				}
				else
				{
					count += DrawText( x, y, textScale, color, textBuffer.c_str(), 0, 0, 0, cursor );
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
					textWidth += activeFont->GetGlyphWidth( textScale, textBuffer.UTF8Char( i ) );
				}
			}

			continue;
		}
	}

	return idMath::Ftoi( rectDraw.w / charSkip );
}

/*
=============
idRectangle::String
=============
*/
char* idRectangle::String() const
{
	static	int		index = 0;
	static	char	str[ 8 ][ 48 ];
	char*	s;

	// use an array so that multiple toString's won't collide
	s = str[ index ];
	index = ( index + 1 ) & 7;

	sprintf( s, "%.2f %.2f %.2f %.2f", x, y, w, h );

	return s;
}


/*
================================================================================================

OPTIMIZED VERSIONS

================================================================================================
*/

// this is only called for the cursor and debug strings, and it should
// scope properly with push/pop clipRect
void idDeviceContextOptimized::EnableClipping( bool b )
{
	if( b == enableClipping )
	{
		return;
	}
	enableClipping = b;
	if( !enableClipping )
	{
		PopClipRect();
	}
	else
	{
		// the actual value of the rect is irrelvent
		PushClipRect( idRectangle( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT ) );

		// allow drawing beyond the normal bounds for debug text
		// this also allows the cursor to draw outside, so we might want
		// to make this exactly the screen bounds, since we aren't likely
		// to ever turn on the gui debug text again...
		clipX1 = -SCREEN_WIDTH;
		clipX2 = SCREEN_WIDTH * 2;
		clipY1 = -SCREEN_HEIGHT;
		clipY2 = SCREEN_HEIGHT * 2;
	}
};


void idDeviceContextOptimized::PopClipRect()
{
	if( clipRects.Num() )
	{
		clipRects.SetNum( clipRects.Num() - 1 );	// don't resize the list, just change num
	}
	if( clipRects.Num() > 0 )
	{
		const idRectangle& clipRect = clipRects[ clipRects.Num() - 1 ];
		clipX1 = clipRect.x;
		clipY1 = clipRect.y;
		clipX2 = clipRect.x + clipRect.w;
		clipY2 = clipRect.y + clipRect.h;
	}
	else
	{
		clipX1 = 0;
		clipY1 = 0;
		clipX2 = SCREEN_WIDTH;
		clipY2 = SCREEN_HEIGHT;
	}
}

static const idRectangle baseScreenRect( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT );

void idDeviceContextOptimized::PushClipRect( idRectangle r )
{
	const idRectangle& prev = ( clipRects.Num() == 0 ) ? baseScreenRect : clipRects[clipRects.Num() - 1];

	// instead of storing the rect, store the intersection of the rect
	// with the previous rect, so ClippedCoords() only has to test against one rect
	idRectangle intersection = prev;
	intersection.ClipAgainst( r, false );
	clipRects.Append( intersection );

	const idRectangle& clipRect = clipRects[ clipRects.Num() - 1 ];
	clipX1 = clipRect.x;
	clipY1 = clipRect.y;
	clipX2 = clipRect.x + clipRect.w;
	clipY2 = clipRect.y + clipRect.h;
}

bool idDeviceContextOptimized::ClippedCoords( float* x, float* y, float* w, float* h, float* s1, float* t1, float* s2, float* t2 )
{
	const float ox = *x;
	const float oy = *y;
	const float ow = *w;
	const float oh = *h;

	// presume visible first
	if( ox >= clipX1 && oy >= clipY1 && ox + ow <= clipX2 && oy + oh <= clipY2 )
	{
		return false;
	}

	// do clipping
	if( ox < clipX1 )
	{
		*w -= clipX1 - ox;
		*x = clipX1;
	}
	else if( ox > clipX2 )
	{
		return true;
	}
	if( oy < clipY1 )
	{
		*h -= clipY1 - oy;
		*y = clipY1;
	}
	else if( oy > clipY2 )
	{
		return true;
	}
	if( *x + *w > clipX2 )
	{
		*w = clipX2 - *x;
	}
	if( *y + *h > clipY2 )
	{
		*h = clipY2 - *y;
	}

	if( *w <= 0 || *h <= 0 )
	{
		return true;
	}

	// filled rects won't pass in texcoords
	if( s1 )
	{
		float ns1, ns2, nt1, nt2;
		// upper left
		float u = ( *x - ox ) / ow;
		ns1 = *s1 * ( 1.0f - u ) + *s2 * ( u );

		// upper right
		u = ( *x + *w - ox ) / ow;
		ns2 = *s1 * ( 1.0f - u ) + *s2 * ( u );

		// lower left
		u = ( *y - oy ) / oh;
		nt1 = *t1 * ( 1.0f - u ) + *t2 * ( u );

		// lower right
		u = ( *y + *h - oy ) / oh;
		nt2 = *t1 * ( 1.0f - u ) + *t2 * ( u );

		// set values
		*s1 = ns1;
		*s2 = ns2;
		*t1 = nt1;
		*t2 = nt2;
	}

	// still needs to be drawn
	return false;
}

/*
=============
idDeviceContextOptimized::DrawText
=============
*/
static triIndex_t quadPicIndexes[6] = { 3, 0, 2, 2, 0, 1 };
int idDeviceContextOptimized::DrawText( float x, float y, float scale, idVec4 color, const char* text, float adjust, int limit, int style, int cursor )
{
	if( !matIsIdentity || cursor != -1 )
	{
		// fallback to old code
		return idDeviceContext::DrawText( x, y, scale, color, text, adjust, limit, style, cursor );
	}

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
		activeFont->GetScaledGlyph( scale, textChar, glyphInfo );

		// PaintChar( x, y, glyphInfo );
		float drawY = y - glyphInfo.top;
		float drawX = x + glyphInfo.left;
		float w = glyphInfo.width;
		float h = glyphInfo.height;
		float s = glyphInfo.s1;
		float t = glyphInfo.t1;
		float s2 = glyphInfo.s2;
		float t2 = glyphInfo.t2;

		if( !ClippedCoords( &drawX, &drawY, &w, &h, &s, &t, &s2, &t2 ) )
		{
			float x1 = xOffset + drawX * xScale;
			float x2 = xOffset + ( drawX + w ) * xScale;
			float y1 = yOffset + drawY * yScale;
			float y2 = yOffset + ( drawY + h ) * yScale;
			idDrawVert* verts = tr_guiModel->AllocTris( 4, quadPicIndexes, 6, glyphInfo.material, 0, STEREO_DEPTH_TYPE_NONE );
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
