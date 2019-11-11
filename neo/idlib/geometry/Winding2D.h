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

#ifndef __WINDING2D_H__
#define __WINDING2D_H__

/*
===============================================================================

	A 2D winding is an arbitrary convex 2D polygon defined by an array of points.

===============================================================================
*/

#define	MAX_POINTS_ON_WINDING_2D		16


class idWinding2D
{
public:
	idWinding2D();

	idWinding2D& 	operator=( const idWinding2D& winding );
	const idVec2& 	operator[]( const int index ) const;
	idVec2& 		operator[]( const int index );

	void			Clear();
	void			AddPoint( const idVec2& point );
	int				GetNumPoints() const;

	void			Expand( const float d );
	void			ExpandForAxialBox( const idVec2 bounds[2] );

	// splits the winding into a front and back winding, the winding itself stays unchanged
	// returns a SIDE_?
	int				Split( const idVec3& plane, const float epsilon, idWinding2D** front, idWinding2D** back ) const;
	// cuts off the part at the back side of the plane, returns true if some part was at the front
	// if there is nothing at the front the number of points is set to zero
	bool			ClipInPlace( const idVec3& plane, const float epsilon = ON_EPSILON, const bool keepOn = false );

	idWinding2D* 	Copy() const;
	idWinding2D* 	Reverse() const;

	float			GetArea() const;
	idVec2			GetCenter() const;
	float			GetRadius( const idVec2& center ) const;
	void			GetBounds( idVec2 bounds[2] ) const;

	bool			IsTiny() const;
	bool			IsHuge() const;	// base winding for a plane is typically huge
	void			Print() const;

	float			PlaneDistance( const idVec3& plane ) const;
	int				PlaneSide( const idVec3& plane, const float epsilon = ON_EPSILON ) const;

	bool			PointInside( const idVec2& point, const float epsilon ) const;
	bool			LineIntersection( const idVec2& start, const idVec2& end ) const;
	bool			RayIntersection( const idVec2& start, const idVec2& dir, float& scale1, float& scale2, int* edgeNums = NULL ) const;

	static idVec3	Plane2DFromPoints( const idVec2& start, const idVec2& end, const bool normalize = false );
	static idVec3	Plane2DFromVecs( const idVec2& start, const idVec2& dir, const bool normalize = false );
	static bool		Plane2DIntersection( const idVec3& plane1, const idVec3& plane2, idVec2& point );

private:
	int				numPoints;
	idVec2			p[MAX_POINTS_ON_WINDING_2D];
};

ID_INLINE idWinding2D::idWinding2D()
{
	numPoints = 0;
}

ID_INLINE idWinding2D& idWinding2D::operator=( const idWinding2D& winding )
{
	int i;

	for( i = 0; i < winding.numPoints; i++ )
	{
		p[i] = winding.p[i];
	}
	numPoints = winding.numPoints;
	return *this;
}

ID_INLINE const idVec2& idWinding2D::operator[]( const int index ) const
{
	return p[ index ];
}

ID_INLINE idVec2& idWinding2D::operator[]( const int index )
{
	return p[ index ];
}

ID_INLINE void idWinding2D::Clear()
{
	numPoints = 0;
}

ID_INLINE void idWinding2D::AddPoint( const idVec2& point )
{
	p[numPoints++] = point;
}

ID_INLINE int idWinding2D::GetNumPoints() const
{
	return numPoints;
}

ID_INLINE idVec3 idWinding2D::Plane2DFromPoints( const idVec2& start, const idVec2& end, const bool normalize )
{
	idVec3 plane;
	plane.x = start.y - end.y;
	plane.y = end.x - start.x;
	if( normalize )
	{
		plane.ToVec2().Normalize();
	}
	plane.z = - ( start.x * plane.x + start.y * plane.y );
	return plane;
}

ID_INLINE idVec3 idWinding2D::Plane2DFromVecs( const idVec2& start, const idVec2& dir, const bool normalize )
{
	idVec3 plane;
	plane.x = -dir.y;
	plane.y = dir.x;
	if( normalize )
	{
		plane.ToVec2().Normalize();
	}
	plane.z = - ( start.x * plane.x + start.y * plane.y );
	return plane;
}

ID_INLINE bool idWinding2D::Plane2DIntersection( const idVec3& plane1, const idVec3& plane2, idVec2& point )
{
	float n00, n01, n11, det, invDet, f0, f1;

	n00 = plane1.x * plane1.x + plane1.y * plane1.y;
	n01 = plane1.x * plane2.x + plane1.y * plane2.y;
	n11 = plane2.x * plane2.x + plane2.y * plane2.y;
	det = n00 * n11 - n01 * n01;

	if( idMath::Fabs( det ) < 1e-6f )
	{
		return false;
	}

	invDet = 1.0f / det;
	f0 = ( n01 * plane2.z - n11 * plane1.z ) * invDet;
	f1 = ( n01 * plane1.z - n00 * plane2.z ) * invDet;
	point.x = f0 * plane1.x + f1 * plane2.x;
	point.y = f0 * plane1.y + f1 * plane2.y;
	return true;
}

#endif /* !__WINDING2D_H__ */
