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

#ifndef __SURFACE_POLYTOPE_H__
#define __SURFACE_POLYTOPE_H__

/*
===============================================================================

	Polytope surface.

	NOTE: vertexes are not duplicated for texture coordinates.

===============================================================================
*/

class idSurface_Polytope : public idSurface
{
public:
	idSurface_Polytope();
	explicit idSurface_Polytope( const idSurface& surface ) : idSurface( surface ) {}

	void				FromPlanes( const idPlane* planes, const int numPlanes );

	void				SetupTetrahedron( const idBounds& bounds );
	void				SetupHexahedron( const idBounds& bounds );
	void				SetupOctahedron( const idBounds& bounds );
	void				SetupDodecahedron( const idBounds& bounds );
	void				SetupIcosahedron( const idBounds& bounds );
	void				SetupCylinder( const idBounds& bounds, const int numSides );
	void				SetupCone( const idBounds& bounds, const int numSides );

	int					SplitPolytope( const idPlane& plane, const float epsilon, idSurface_Polytope** front, idSurface_Polytope** back ) const;

protected:

};

/*
====================
idSurface_Polytope::idSurface_Polytope
====================
*/
ID_INLINE idSurface_Polytope::idSurface_Polytope()
{
}

#endif /* !__SURFACE_POLYTOPE_H__ */
