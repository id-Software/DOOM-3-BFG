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

#ifndef __SCREENRECT_H__
#define __SCREENRECT_H__

/*
================================================================================================

idScreenRect

idScreenRect gets carried around with each drawSurf, so it makes sense
to keep it compact, instead of just using the idBounds class
================================================================================================
*/

class idScreenRect
{
public:
	// Inclusive pixel bounds inside viewport
	short		x1;
	short		y1;
	short		x2;
	short		y2;

	// for depth bounds test
	float       zmin;
	float		zmax;

	bool		operator==( idScreenRect& other ) const;
	bool		operator!=( idScreenRect& other ) const;

	// clear to backwards values
	void		Clear();
	bool		IsEmpty() const;
	short		GetWidth() const
	{
		return x2 - x1 + 1;
	}
	short		GetHeight() const
	{
		return y2 - y1 + 1;
	}
	int			GetArea() const
	{
		return ( x2 - x1 + 1 ) * ( y2 - y1 + 1 );
	}

	// expand by one pixel each way to fix roundoffs
	void		Expand();

	// adds a point
	void		AddPoint( float x, float y );

	void		Intersect( const idScreenRect& rect );
	void		Union( const idScreenRect& rect );
	bool		Equals( const idScreenRect& rect ) const;
};

void R_ShowColoredScreenRect( const idScreenRect& rect, int colorIndex );

#endif /* !__SCREENRECT_H__ */
