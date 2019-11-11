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

#ifndef __MATH_PLANE_H__
#define __MATH_PLANE_H__

/*
===============================================================================

	3D plane with equation: a * x + b * y + c * z + d = 0

===============================================================================
*/


class idVec3;
class idMat3;

#define	ON_EPSILON					0.1f
#define DEGENERATE_DIST_EPSILON		1e-4f

#define	SIDE_FRONT					0
#define	SIDE_BACK					1
#define	SIDE_ON						2
#define	SIDE_CROSS					3

// plane sides
#define PLANESIDE_FRONT				0
#define PLANESIDE_BACK				1
#define PLANESIDE_ON				2
#define PLANESIDE_CROSS				3

// plane types
#define PLANETYPE_X					0
#define PLANETYPE_Y					1
#define PLANETYPE_Z					2
#define PLANETYPE_NEGX				3
#define PLANETYPE_NEGY				4
#define PLANETYPE_NEGZ				5
#define PLANETYPE_TRUEAXIAL			6	// all types < 6 are true axial planes
#define PLANETYPE_ZEROX				6
#define PLANETYPE_ZEROY				7
#define PLANETYPE_ZEROZ				8
#define PLANETYPE_NONAXIAL			9

class idPlane
{
public:
	idPlane();
	explicit idPlane( float a, float b, float c, float d );
	explicit idPlane( const idVec3& normal, const float dist );
	explicit idPlane( const idVec3& v0, const idVec3& v1, const idVec3& v2, bool fixDegenerate = false );

	float			operator[]( int index ) const;
	float& 			operator[]( int index );
	idPlane			operator-() const;						// flips plane
	idPlane& 		operator=( const idVec3& v );			// sets normal and sets idPlane::d to zero
	idPlane			operator+( const idPlane& p ) const;	// add plane equations
	idPlane			operator-( const idPlane& p ) const;	// subtract plane equations
	idPlane			operator*( const float s ) const;		// scale plane
	idPlane& 		operator*=( const idMat3& m );			// Normal() *= m

	bool			Compare( const idPlane& p ) const;						// exact compare, no epsilon
	bool			Compare( const idPlane& p, const float epsilon ) const;	// compare with epsilon
	bool			Compare( const idPlane& p, const float normalEps, const float distEps ) const;	// compare with epsilon
	bool			operator==(	const idPlane& p ) const;					// exact compare, no epsilon
	bool			operator!=(	const idPlane& p ) const;					// exact compare, no epsilon

	void			Zero();							// zero plane
	void			SetNormal( const idVec3& normal );		// sets the normal
	const idVec3& 	Normal() const;					// reference to const normal
	idVec3& 		Normal();							// reference to normal
	float			Normalize( bool fixDegenerate = true );	// only normalizes the plane normal, does not adjust d
	bool			FixDegenerateNormal();			// fix degenerate normal
	bool			FixDegeneracies( float distEpsilon );	// fix degenerate normal and dist
	float			Dist() const;						// returns: -d
	void			SetDist( const float dist );			// sets: d = -dist
	int				Type() const;						// returns plane type

	bool			FromPoints( const idVec3& p1, const idVec3& p2, const idVec3& p3, bool fixDegenerate = true );
	bool			FromVecs( const idVec3& dir1, const idVec3& dir2, const idVec3& p, bool fixDegenerate = true );
	void			FitThroughPoint( const idVec3& p );	// assumes normal is valid
	bool			HeightFit( const idVec3* points, const int numPoints );
	idPlane			Translate( const idVec3& translation ) const;
	idPlane& 		TranslateSelf( const idVec3& translation );
	idPlane			Rotate( const idVec3& origin, const idMat3& axis ) const;
	idPlane& 		RotateSelf( const idVec3& origin, const idMat3& axis );

	float			Distance( const idVec3& v ) const;
	int				Side( const idVec3& v, const float epsilon = 0.0f ) const;

	bool			LineIntersection( const idVec3& start, const idVec3& end ) const;
	// intersection point is start + dir * scale
	bool			RayIntersection( const idVec3& start, const idVec3& dir, float& scale ) const;
	bool			PlaneIntersection( const idPlane& plane, idVec3& start, idVec3& dir ) const;

	int				GetDimension() const;

	const idVec4& 	ToVec4() const;
	idVec4& 		ToVec4();
	const float* 	ToFloatPtr() const;
	float* 			ToFloatPtr();
	const char* 	ToString( int precision = 2 ) const;

private:
	float			a;
	float			b;
	float			c;
	float			d;
};

extern idPlane plane_origin;
#define plane_zero plane_origin

ID_INLINE idPlane::idPlane()
{
}

ID_INLINE idPlane::idPlane( float a, float b, float c, float d )
{
	this->a = a;
	this->b = b;
	this->c = c;
	this->d = d;
}

ID_INLINE idPlane::idPlane( const idVec3& normal, const float dist )
{
	this->a = normal.x;
	this->b = normal.y;
	this->c = normal.z;
	this->d = -dist;
}

ID_INLINE idPlane::idPlane( const idVec3& v0, const idVec3& v1, const idVec3& v2, bool fixDegenerate )
{
	FromPoints( v0, v1, v2, fixDegenerate );
}

ID_INLINE float idPlane::operator[]( int index ) const
{
	return ( &a )[ index ];
}

ID_INLINE float& idPlane::operator[]( int index )
{
	return ( &a )[ index ];
}

ID_INLINE idPlane idPlane::operator-() const
{
	return idPlane( -a, -b, -c, -d );
}

ID_INLINE idPlane& idPlane::operator=( const idVec3& v )
{
	a = v.x;
	b = v.y;
	c = v.z;
	d = 0;
	return *this;
}

ID_INLINE idPlane idPlane::operator+( const idPlane& p ) const
{
	return idPlane( a + p.a, b + p.b, c + p.c, d + p.d );
}

ID_INLINE idPlane idPlane::operator-( const idPlane& p ) const
{
	return idPlane( a - p.a, b - p.b, c - p.c, d - p.d );
}

ID_INLINE idPlane idPlane::operator*( const float s ) const
{
	return idPlane( a * s, b * s, c * s, d * s );
}

ID_INLINE idPlane& idPlane::operator*=( const idMat3& m )
{
	Normal() *= m;
	return *this;
}

ID_INLINE bool idPlane::Compare( const idPlane& p ) const
{
	return ( a == p.a && b == p.b && c == p.c && d == p.d );
}

ID_INLINE bool idPlane::Compare( const idPlane& p, const float epsilon ) const
{
	if( idMath::Fabs( a - p.a ) > epsilon )
	{
		return false;
	}

	if( idMath::Fabs( b - p.b ) > epsilon )
	{
		return false;
	}

	if( idMath::Fabs( c - p.c ) > epsilon )
	{
		return false;
	}

	if( idMath::Fabs( d - p.d ) > epsilon )
	{
		return false;
	}

	return true;
}

ID_INLINE bool idPlane::Compare( const idPlane& p, const float normalEps, const float distEps ) const
{
	if( idMath::Fabs( d - p.d ) > distEps )
	{
		return false;
	}
	if( !Normal().Compare( p.Normal(), normalEps ) )
	{
		return false;
	}
	return true;
}

ID_INLINE bool idPlane::operator==( const idPlane& p ) const
{
	return Compare( p );
}

ID_INLINE bool idPlane::operator!=( const idPlane& p ) const
{
	return !Compare( p );
}

ID_INLINE void idPlane::Zero()
{
	a = b = c = d = 0.0f;
}

ID_INLINE void idPlane::SetNormal( const idVec3& normal )
{
	a = normal.x;
	b = normal.y;
	c = normal.z;
}

ID_INLINE const idVec3& idPlane::Normal() const
{
	return *reinterpret_cast<const idVec3*>( &a );
}

ID_INLINE idVec3& idPlane::Normal()
{
	return *reinterpret_cast<idVec3*>( &a );
}

ID_INLINE float idPlane::Normalize( bool fixDegenerate )
{
	float length = reinterpret_cast<idVec3*>( &a )->Normalize();

	if( fixDegenerate )
	{
		FixDegenerateNormal();
	}
	return length;
}

ID_INLINE bool idPlane::FixDegenerateNormal()
{
	return Normal().FixDegenerateNormal();
}

ID_INLINE bool idPlane::FixDegeneracies( float distEpsilon )
{
	bool fixedNormal = FixDegenerateNormal();
	// only fix dist if the normal was degenerate
	if( fixedNormal )
	{
		if( idMath::Fabs( d - idMath::Rint( d ) ) < distEpsilon )
		{
			d = idMath::Rint( d );
		}
	}
	return fixedNormal;
}

ID_INLINE float idPlane::Dist() const
{
	return -d;
}

ID_INLINE void idPlane::SetDist( const float dist )
{
	d = -dist;
}

ID_INLINE bool idPlane::FromPoints( const idVec3& p1, const idVec3& p2, const idVec3& p3, bool fixDegenerate )
{
	Normal() = ( p1 - p2 ).Cross( p3 - p2 );
	if( Normalize( fixDegenerate ) == 0.0f )
	{
		return false;
	}
	d = -( Normal() * p2 );
	return true;
}

ID_INLINE bool idPlane::FromVecs( const idVec3& dir1, const idVec3& dir2, const idVec3& p, bool fixDegenerate )
{
	Normal() = dir1.Cross( dir2 );
	if( Normalize( fixDegenerate ) == 0.0f )
	{
		return false;
	}
	d = -( Normal() * p );
	return true;
}

ID_INLINE void idPlane::FitThroughPoint( const idVec3& p )
{
	d = -( Normal() * p );
}

ID_INLINE idPlane idPlane::Translate( const idVec3& translation ) const
{
	return idPlane( a, b, c, d - translation * Normal() );
}

ID_INLINE idPlane& idPlane::TranslateSelf( const idVec3& translation )
{
	d -= translation * Normal();
	return *this;
}

ID_INLINE idPlane idPlane::Rotate( const idVec3& origin, const idMat3& axis ) const
{
	idPlane p;
	p.Normal() = Normal() * axis;
	p.d = d + origin * Normal() - origin * p.Normal();
	return p;
}

ID_INLINE idPlane& idPlane::RotateSelf( const idVec3& origin, const idMat3& axis )
{
	d += origin * Normal();
	Normal() *= axis;
	d -= origin * Normal();
	return *this;
}

ID_INLINE float idPlane::Distance( const idVec3& v ) const
{
	return a * v.x + b * v.y + c * v.z + d;
}

ID_INLINE int idPlane::Side( const idVec3& v, const float epsilon ) const
{
	float dist = Distance( v );
	if( dist > epsilon )
	{
		return PLANESIDE_FRONT;
	}
	else if( dist < -epsilon )
	{
		return PLANESIDE_BACK;
	}
	else
	{
		return PLANESIDE_ON;
	}
}

ID_INLINE bool idPlane::LineIntersection( const idVec3& start, const idVec3& end ) const
{
	float d1, d2, fraction;

	d1 = Normal() * start + d;
	d2 = Normal() * end + d;
	if( d1 == d2 )
	{
		return false;
	}
	if( d1 > 0.0f && d2 > 0.0f )
	{
		return false;
	}
	if( d1 < 0.0f && d2 < 0.0f )
	{
		return false;
	}
	fraction = ( d1 / ( d1 - d2 ) );
	return ( fraction >= 0.0f && fraction <= 1.0f );
}

ID_INLINE bool idPlane::RayIntersection( const idVec3& start, const idVec3& dir, float& scale ) const
{
	float d1, d2;

	d1 = Normal() * start + d;
	d2 = Normal() * dir;
	if( d2 == 0.0f )
	{
		return false;
	}
	scale = -( d1 / d2 );
	return true;
}

ID_INLINE int idPlane::GetDimension() const
{
	return 4;
}

ID_INLINE const idVec4& idPlane::ToVec4() const
{
	return *reinterpret_cast<const idVec4*>( &a );
}

ID_INLINE idVec4& idPlane::ToVec4()
{
	return *reinterpret_cast<idVec4*>( &a );
}

ID_INLINE const float* idPlane::ToFloatPtr() const
{
	return reinterpret_cast<const float*>( &a );
}

ID_INLINE float* idPlane::ToFloatPtr()
{
	return reinterpret_cast<float*>( &a );
}

#endif /* !__MATH_PLANE_H__ */
