/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2012 Robert Beckebans

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
#include "Window.h"
#include "UserInterfaceLocal.h"
#include "SimpleWindow.h"


idSimpleWindow::idSimpleWindow( idWindow* win )
{
	gui = win->GetGui();
	drawRect = win->drawRect;
	clientRect = win->clientRect;
	textRect = win->textRect;
	origin = win->origin;
	font = win->font;
	name = win->name;
	matScalex = win->matScalex;
	matScaley = win->matScaley;
	borderSize = win->borderSize;
	textAlign = win->textAlign;
	textAlignx = win->textAlignx;
	textAligny = win->textAligny;
	background = win->background;
	flags = win->flags;
	textShadow = win->textShadow;

	visible = win->visible;
	text = win->text;
	rect = win->rect;
	backColor = win->backColor;
	matColor = win->matColor;
	foreColor = win->foreColor;
	borderColor = win->borderColor;
	textScale = win->textScale;
	rotate = win->rotate;
	shear = win->shear;
	backGroundName = win->backGroundName;
	if( backGroundName.Length() )
	{
		background = declManager->FindMaterial( backGroundName );
		background->SetSort( SS_GUI );
	}
	backGroundName.SetMaterialPtr( &background );

//
//  added parent
	mParent = win->GetParent();
//

	hideCursor = win->hideCursor;

	idWindow* parent = win->GetParent();
	if( parent )
	{
		if( text.NeedsUpdate() )
		{
			parent->AddUpdateVar( &text );
		}
		if( visible.NeedsUpdate() )
		{
			parent->AddUpdateVar( &visible );
		}
		if( rect.NeedsUpdate() )
		{
			parent->AddUpdateVar( &rect );
		}
		if( backColor.NeedsUpdate() )
		{
			parent->AddUpdateVar( &backColor );
		}
		if( matColor.NeedsUpdate() )
		{
			parent->AddUpdateVar( &matColor );
		}
		if( foreColor.NeedsUpdate() )
		{
			parent->AddUpdateVar( &foreColor );
		}
		if( borderColor.NeedsUpdate() )
		{
			parent->AddUpdateVar( &borderColor );
		}
		if( textScale.NeedsUpdate() )
		{
			parent->AddUpdateVar( &textScale );
		}
		if( rotate.NeedsUpdate() )
		{
			parent->AddUpdateVar( &rotate );
		}
		if( shear.NeedsUpdate() )
		{
			parent->AddUpdateVar( &shear );
		}
		if( backGroundName.NeedsUpdate() )
		{
			parent->AddUpdateVar( &backGroundName );
		}
	}
}

idSimpleWindow::~idSimpleWindow()
{

}

void idSimpleWindow::StateChanged( bool redraw )
{
}

void idSimpleWindow::SetupTransforms( float x, float y )
{
	static idMat3 trans;
	static idVec3 org;

	trans.Identity();
	org.Set( origin.x + x, origin.y + y, 0 );
	if( rotate )
	{
		static idRotation rot;
		static idVec3 vec( 0, 0, 1 );
		rot.Set( org, vec, rotate );
		trans = rot.ToMat3();
	}

	static idMat3 smat;
	smat.Identity();
	if( shear.x() || shear.y() )
	{
		smat[0][1] = shear.x();
		smat[1][0] = shear.y();
		trans *= smat;
	}

	if( !trans.IsIdentity() )
	{
		dc->SetTransformInfo( org, trans );
	}
}

void idSimpleWindow::DrawBackground( const idRectangle& drawRect )
{
	if( backColor.w() > 0 )
	{
		dc->DrawFilledRect( drawRect.x, drawRect.y, drawRect.w, drawRect.h, backColor );
	}

	if( background )
	{
		if( matColor.w() > 0 )
		{
			float scalex, scaley;
			if( flags & WIN_NATURALMAT )
			{
				scalex = drawRect.w / background->GetImageWidth();
				scaley = drawRect.h / background->GetImageHeight();
			}
			else
			{
				scalex = matScalex;
				scaley = matScaley;
			}
			dc->DrawMaterial( drawRect.x, drawRect.y, drawRect.w, drawRect.h, background, matColor, scalex, scaley );
		}
	}
}

void idSimpleWindow::DrawBorderAndCaption( const idRectangle& drawRect )
{
	if( flags & WIN_BORDER )
	{
		if( borderSize )
		{
			dc->DrawRect( drawRect.x, drawRect.y, drawRect.w, drawRect.h, borderSize, borderColor );
		}
	}
}

void idSimpleWindow::CalcClientRect( float xofs, float yofs )
{

	drawRect = rect;

	if( flags & WIN_INVERTRECT )
	{
		drawRect.x = rect.x() - rect.w();
		drawRect.y = rect.y() - rect.h();
	}

	drawRect.x += xofs;
	drawRect.y += yofs;

	clientRect = drawRect;
	if( rect.h() > 0.0 && rect.w() > 0.0 )
	{

		if( flags & WIN_BORDER && borderSize != 0.0 )
		{
			clientRect.x += borderSize;
			clientRect.y += borderSize;
			clientRect.w -= borderSize;
			clientRect.h -= borderSize;
		}

		textRect = clientRect;
		textRect.x += 2.0;
		textRect.w -= 2.0;
		textRect.y += 2.0;
		textRect.h -= 2.0;
		textRect.x += textAlignx;
		textRect.y += textAligny;

	}
	origin.Set( rect.x() + ( rect.w() / 2 ), rect.y() + ( rect.h() / 2 ) );

}


void idSimpleWindow::Redraw( float x, float y )
{

	if( !visible )
	{
		return;
	}

	CalcClientRect( 0, 0 );
	dc->SetFont( font );
	drawRect.Offset( x, y );
	clientRect.Offset( x, y );
	textRect.Offset( x, y );
	SetupTransforms( x, y );
	if( flags & WIN_NOCLIP )
	{
		dc->EnableClipping( false );
	}
	DrawBackground( drawRect );
	DrawBorderAndCaption( drawRect );
	if( textShadow )
	{
		idStr shadowText = text;
		idRectangle shadowRect = textRect;

		shadowText.RemoveColors();
		shadowRect.x += textShadow;
		shadowRect.y += textShadow;

		dc->DrawText( shadowText, textScale, textAlign, colorBlack, shadowRect, !( flags & WIN_NOWRAP ), -1 );
	}
	dc->DrawText( text, textScale, textAlign, foreColor, textRect, !( flags & WIN_NOWRAP ), -1 );
	dc->SetTransformInfo( vec3_origin, mat3_identity );
	if( flags & WIN_NOCLIP )
	{
		dc->EnableClipping( true );
	}
	drawRect.Offset( -x, -y );
	clientRect.Offset( -x, -y );
	textRect.Offset( -x, -y );
}

int idSimpleWindow::GetWinVarOffset( idWinVar* wv, drawWin_t* owner )
{
	int ret = -1;

	// RB: 64 bit fixes, changed oldschool offsets using ptrdiff_t
	if( wv == &rect )
	{
		ret = ( ptrdiff_t )&rect - ( ptrdiff_t )this;
	}

	if( wv == &backColor )
	{
		ret = ( ptrdiff_t )&backColor - ( ptrdiff_t )this;
	}

	if( wv == &matColor )
	{
		ret = ( ptrdiff_t )&matColor - ( ptrdiff_t )this;
	}

	if( wv == &foreColor )
	{
		ret = ( ptrdiff_t )&foreColor - ( ptrdiff_t )this;
	}

	if( wv == &borderColor )
	{
		ret = ( ptrdiff_t )&borderColor - ( ptrdiff_t )this;
	}

	if( wv == &textScale )
	{
		ret = ( ptrdiff_t )&textScale - ( ptrdiff_t )this;
	}

	if( wv == &rotate )
	{
		ret = ( ptrdiff_t )&rotate - ( ptrdiff_t )this;
	}
	// RB end

	if( ret != -1 )
	{
		owner->simp = this;
	}
	return ret;
}

idWinVar* idSimpleWindow::GetWinVarByName( const char* _name )
{
	idWinVar* retVar = NULL;
	if( idStr::Icmp( _name, "background" ) == 0 )
	{
		retVar = &backGroundName;
	}
	if( idStr::Icmp( _name, "visible" ) == 0 )
	{
		retVar = &visible;
	}
	if( idStr::Icmp( _name, "rect" ) == 0 )
	{
		retVar = &rect;
	}
	if( idStr::Icmp( _name, "backColor" ) == 0 )
	{
		retVar = &backColor;
	}
	if( idStr::Icmp( _name, "matColor" ) == 0 )
	{
		retVar = &matColor;
	}
	if( idStr::Icmp( _name, "foreColor" ) == 0 )
	{
		retVar = &foreColor;
	}
	if( idStr::Icmp( _name, "borderColor" ) == 0 )
	{
		retVar = &borderColor;
	}
	if( idStr::Icmp( _name, "textScale" ) == 0 )
	{
		retVar = &textScale;
	}
	if( idStr::Icmp( _name, "rotate" ) == 0 )
	{
		retVar = &rotate;
	}
	if( idStr::Icmp( _name, "shear" ) == 0 )
	{
		retVar = &shear;
	}
	if( idStr::Icmp( _name, "text" ) == 0 )
	{
		retVar = &text;
	}
	return retVar;
}

/*
========================
idSimpleWindow::WriteToSaveGame
========================
*/
void idSimpleWindow::WriteToSaveGame( idFile* savefile )
{

	savefile->Write( &flags, sizeof( flags ) );
	savefile->Write( &drawRect, sizeof( drawRect ) );
	savefile->Write( &clientRect, sizeof( clientRect ) );
	savefile->Write( &textRect, sizeof( textRect ) );
	savefile->Write( &origin, sizeof( origin ) );
	savefile->Write( &matScalex, sizeof( matScalex ) );
	savefile->Write( &matScaley, sizeof( matScaley ) );
	savefile->Write( &borderSize, sizeof( borderSize ) );
	savefile->Write( &textAlign, sizeof( textAlign ) );
	savefile->Write( &textAlignx, sizeof( textAlignx ) );
	savefile->Write( &textAligny, sizeof( textAligny ) );
	savefile->Write( &textShadow, sizeof( textShadow ) );
	savefile->WriteString( font->GetName() );

	text.WriteToSaveGame( savefile );
	visible.WriteToSaveGame( savefile );
	rect.WriteToSaveGame( savefile );
	backColor.WriteToSaveGame( savefile );
	matColor.WriteToSaveGame( savefile );
	foreColor.WriteToSaveGame( savefile );
	borderColor.WriteToSaveGame( savefile );
	textScale.WriteToSaveGame( savefile );
	rotate.WriteToSaveGame( savefile );
	shear.WriteToSaveGame( savefile );
	backGroundName.WriteToSaveGame( savefile );

	int stringLen;

	if( background )
	{
		stringLen = strlen( background->GetName() );
		savefile->Write( &stringLen, sizeof( stringLen ) );
		savefile->Write( background->GetName(), stringLen );
	}
	else
	{
		stringLen = 0;
		savefile->Write( &stringLen, sizeof( stringLen ) );
	}

}

/*
========================
idSimpleWindow::ReadFromSaveGame
========================
*/
void idSimpleWindow::ReadFromSaveGame( idFile* savefile )
{

	savefile->Read( &flags, sizeof( flags ) );
	savefile->Read( &drawRect, sizeof( drawRect ) );
	savefile->Read( &clientRect, sizeof( clientRect ) );
	savefile->Read( &textRect, sizeof( textRect ) );
	savefile->Read( &origin, sizeof( origin ) );
	/*	if ( savefile->GetFileVersion() < BUILD_NUMBER_8TH_ANNIVERSARY_1 ) {
			int fontNum;
			savefile->Read( &fontNum, sizeof( fontNum ) );
			font = renderSystem->RegisterFont( "" );
		} */
	savefile->Read( &matScalex, sizeof( matScalex ) );
	savefile->Read( &matScaley, sizeof( matScaley ) );
	savefile->Read( &borderSize, sizeof( borderSize ) );
	savefile->Read( &textAlign, sizeof( textAlign ) );
	savefile->Read( &textAlignx, sizeof( textAlignx ) );
	savefile->Read( &textAligny, sizeof( textAligny ) );
	savefile->Read( &textShadow, sizeof( textShadow ) );
//	if ( savefile->GetFileVersion() >= BUILD_NUMBER_8TH_ANNIVERSARY_1 ) {
	idStr fontName;
	savefile->ReadString( fontName );
	font = renderSystem->RegisterFont( fontName );
//	}

	text.ReadFromSaveGame( savefile );
	visible.ReadFromSaveGame( savefile );
	rect.ReadFromSaveGame( savefile );
	backColor.ReadFromSaveGame( savefile );
	matColor.ReadFromSaveGame( savefile );
	foreColor.ReadFromSaveGame( savefile );
	borderColor.ReadFromSaveGame( savefile );
	textScale.ReadFromSaveGame( savefile );
	rotate.ReadFromSaveGame( savefile );
	shear.ReadFromSaveGame( savefile );
	backGroundName.ReadFromSaveGame( savefile );

	int stringLen;

	savefile->Read( &stringLen, sizeof( stringLen ) );
	if( stringLen > 0 )
	{
		idStr backName;

		backName.Fill( ' ', stringLen );
		savefile->Read( &( backName )[0], stringLen );

		background = declManager->FindMaterial( backName );
		background->SetSort( SS_GUI );
	}
	else
	{
		background = NULL;
	}

}


/*
===============================
*/

size_t idSimpleWindow::Size()
{
	size_t sz = sizeof( *this );
	sz += name.Size();
	sz += text.Size();
	sz += backGroundName.Size();
	return sz;
}
