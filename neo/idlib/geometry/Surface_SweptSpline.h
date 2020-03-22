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

#ifndef __SURFACE_SWEPTSPLINE_H__
#define __SURFACE_SWEPTSPLINE_H__

/*
===============================================================================

	Swept Spline surface.

===============================================================================
*/

class idSurface_SweptSpline : public idSurface
{
public:
	idSurface_SweptSpline();
	~idSurface_SweptSpline();

	void					SetSpline( idCurve_Spline<idVec4>* spline );
	void					SetSweptSpline( idCurve_Spline<idVec4>* sweptSpline );
	void					SetSweptCircle( const float radius );

	void					Tessellate( const int splineSubdivisions, const int sweptSplineSubdivisions );

	void					Clear();

protected:
	idCurve_Spline<idVec4>* spline;
	idCurve_Spline<idVec4>* sweptSpline;

	void					GetFrame( const idMat3& previousFrame, const idVec3 dir, idMat3& newFrame );
};

/*
====================
idSurface_SweptSpline::idSurface_SweptSpline
====================
*/
ID_INLINE idSurface_SweptSpline::idSurface_SweptSpline()
{
	spline = NULL;
	sweptSpline = NULL;
}

/*
====================
idSurface_SweptSpline::~idSurface_SweptSpline
====================
*/
ID_INLINE idSurface_SweptSpline::~idSurface_SweptSpline()
{
	delete spline;
	delete sweptSpline;
}

/*
====================
idSurface_SweptSpline::Clear
====================
*/
ID_INLINE void idSurface_SweptSpline::Clear()
{
	idSurface::Clear();
	delete spline;
	spline = NULL;
	delete sweptSpline;
	sweptSpline = NULL;
}

#endif /* !__SURFACE_SWEPTSPLINE_H__ */
