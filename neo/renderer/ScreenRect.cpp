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

#include "RenderCommon.h"

/*
==========================================================================================

idScreenRect

==========================================================================================
*/

/*
======================
idScreenRect::Clear
======================
*/
void idScreenRect::Clear()
{
	x1 = y1 = 32000;
	x2 = y2 = -32000;
	zmin = 0.0f;
	zmax = 1.0f;
}

/*
======================
idScreenRect::AddPoint
======================
*/
void idScreenRect::AddPoint( float x, float y )
{
	int	ix = idMath::Ftoi( x );
	int iy = idMath::Ftoi( y );

	if( ix < x1 )
	{
		x1 = ix;
	}
	if( ix > x2 )
	{
		x2 = ix;
	}
	if( iy < y1 )
	{
		y1 = iy;
	}
	if( iy > y2 )
	{
		y2 = iy;
	}
}

/*
======================
idScreenRect::Expand
======================
*/
void idScreenRect::Expand()
{
	x1--;
	y1--;
	x2++;
	y2++;
}

/*
======================
idScreenRect::Intersect
======================
*/
void idScreenRect::Intersect( const idScreenRect& rect )
{
	if( rect.x1 > x1 )
	{
		x1 = rect.x1;
	}
	if( rect.x2 < x2 )
	{
		x2 = rect.x2;
	}
	if( rect.y1 > y1 )
	{
		y1 = rect.y1;
	}
	if( rect.y2 < y2 )
	{
		y2 = rect.y2;
	}
}

/*
======================
idScreenRect::Union
======================
*/
void idScreenRect::Union( const idScreenRect& rect )
{
	if( rect.x1 < x1 )
	{
		x1 = rect.x1;
	}
	if( rect.x2 > x2 )
	{
		x2 = rect.x2;
	}
	if( rect.y1 < y1 )
	{
		y1 = rect.y1;
	}
	if( rect.y2 > y2 )
	{
		y2 = rect.y2;
	}
}

/*
======================
idScreenRect::Equals
======================
*/
bool idScreenRect::Equals( const idScreenRect& rect ) const
{
	return ( x1 == rect.x1 && x2 == rect.x2 && y1 == rect.y1 && y2 == rect.y2 );
}

/*
======================
idScreenRect::IsEmpty
======================
*/
bool idScreenRect::IsEmpty() const
{
	return ( x1 > x2 || y1 > y2 );
}

/*
======================
R_ShowColoredScreenRect
======================
*/
void R_ShowColoredScreenRect( const idScreenRect& rect, int colorIndex )
{
	if( !rect.IsEmpty() )
	{
		static idVec4 colors[] = { colorRed, colorGreen, colorBlue, colorYellow, colorMagenta, colorCyan, colorWhite, colorPurple };
		tr.viewDef->renderWorld->DebugScreenRect( colors[colorIndex & 7], rect, tr.viewDef );
	}
}
