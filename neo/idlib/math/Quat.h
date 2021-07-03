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

#ifndef __MATH_QUAT_H__
#define __MATH_QUAT_H__

/*
===============================================================================

	Quaternion

===============================================================================
*/

#include "../containers/Array.h"	// for idTupleSize

class idVec3;
class idAngles;
class idRotation;
class idMat3;
class idMat4;
class idCQuat;

class idQuat
{
public:
	float			x;
	float			y;
	float			z;
	float			w;

	idQuat();
	idQuat( float x, float y, float z, float w );

	void 			Set( float x, float y, float z, float w );

	float			operator[]( int index ) const;
	float& 			operator[]( int index );
	idQuat			operator-() const;
	idQuat& 		operator=( const idQuat& a );
	idQuat			operator+( const idQuat& a ) const;
	idQuat& 		operator+=( const idQuat& a );
	idQuat			operator-( const idQuat& a ) const;
	idQuat& 		operator-=( const idQuat& a );
	idQuat			operator*( const idQuat& a ) const;
	idVec3			operator*( const idVec3& a ) const;
	idQuat			operator*( float a ) const;
	idQuat& 		operator*=( const idQuat& a );
	idQuat& 		operator*=( float a );

	friend idQuat	operator*( const float a, const idQuat& b );
	friend idVec3	operator*( const idVec3& a, const idQuat& b );

	bool			Compare( const idQuat& a ) const;						// exact compare, no epsilon
	bool			Compare( const idQuat& a, const float epsilon ) const;	// compare with epsilon
	bool			operator==(	const idQuat& a ) const;					// exact compare, no epsilon
	bool			operator!=(	const idQuat& a ) const;					// exact compare, no epsilon

	idQuat			Inverse() const;
	float			Length() const;
	idQuat& 		Normalize();

	float			CalcW() const;
	int				GetDimension() const;

	idAngles		ToAngles() const;
	idRotation		ToRotation() const;
	idMat3			ToMat3() const;
	idMat4			ToMat4() const;
	idCQuat			ToCQuat() const;
	idVec3			ToAngularVelocity() const;
	const float* 	ToFloatPtr() const;
	float* 			ToFloatPtr();
	const char* 	ToString( int precision = 2 ) const;

	idQuat& 		Slerp( const idQuat& from, const idQuat& to, float t );
	idQuat& 		Lerp( const idQuat& from, const idQuat& to, const float t );
};

// A non-member slerp function allows constructing a const idQuat object with the result of a slerp,
// but without having to explicity create a temporary idQuat object.
idQuat Slerp( const idQuat& from, const idQuat& to, const float t );

ID_INLINE idQuat::idQuat()
{
}

ID_INLINE idQuat::idQuat( float x, float y, float z, float w )
{
	this->x = x;
	this->y = y;
	this->z = z;
	this->w = w;
}

ID_INLINE float idQuat::operator[]( int index ) const
{
	assert( ( index >= 0 ) && ( index < 4 ) );
	return ( &x )[ index ];
}

ID_INLINE float& idQuat::operator[]( int index )
{
	assert( ( index >= 0 ) && ( index < 4 ) );
	return ( &x )[ index ];
}

ID_INLINE idQuat idQuat::operator-() const
{
	return idQuat( -x, -y, -z, -w );
}

ID_INLINE idQuat& idQuat::operator=( const idQuat& a )
{
	x = a.x;
	y = a.y;
	z = a.z;
	w = a.w;

	return *this;
}

ID_INLINE idQuat idQuat::operator+( const idQuat& a ) const
{
	return idQuat( x + a.x, y + a.y, z + a.z, w + a.w );
}

ID_INLINE idQuat& idQuat::operator+=( const idQuat& a )
{
	x += a.x;
	y += a.y;
	z += a.z;
	w += a.w;

	return *this;
}

ID_INLINE idQuat idQuat::operator-( const idQuat& a ) const
{
	return idQuat( x - a.x, y - a.y, z - a.z, w - a.w );
}

ID_INLINE idQuat& idQuat::operator-=( const idQuat& a )
{
	x -= a.x;
	y -= a.y;
	z -= a.z;
	w -= a.w;

	return *this;
}

ID_INLINE idQuat idQuat::operator*( const idQuat& a ) const
{
	return idQuat(	w * a.x + x * a.w + y * a.z - z * a.y,
					w * a.y + y * a.w + z * a.x - x * a.z,
					w * a.z + z * a.w + x * a.y - y * a.x,
					w * a.w - x * a.x - y * a.y - z * a.z );
}

ID_INLINE idVec3 idQuat::operator*( const idVec3& a ) const
{
#if 0
	// it's faster to do the conversion to a 3x3 matrix and multiply the vector by this 3x3 matrix
	return ( ToMat3() * a );
#else
	// result = this->Inverse() * idQuat( a.x, a.y, a.z, 0.0f ) * (*this)
	float xxzz = x * x - z * z;
	float wwyy = w * w - y * y;

	float xw2 = x * w * 2.0f;
	float xy2 = x * y * 2.0f;
	float xz2 = x * z * 2.0f;
	float yw2 = y * w * 2.0f;
	float yz2 = y * z * 2.0f;
	float zw2 = z * w * 2.0f;

	return idVec3(
			   ( xxzz + wwyy ) * a.x		+ ( xy2 + zw2 ) * a.y		+ ( xz2 - yw2 ) * a.z,
			   ( xy2 - zw2 ) * a.x			+ ( y * y + w * w - x * x - z * z ) * a.y	+ ( yz2 + xw2 ) * a.z,
			   ( xz2 + yw2 ) * a.x			+ ( yz2 - xw2 ) * a.y		+ ( wwyy - xxzz ) * a.z
		   );
#endif
}

ID_INLINE idQuat idQuat::operator*( float a ) const
{
	return idQuat( x * a, y * a, z * a, w * a );
}

ID_INLINE idQuat operator*( const float a, const idQuat& b )
{
	return b * a;
}

ID_INLINE idVec3 operator*( const idVec3& a, const idQuat& b )
{
	return b * a;
}

ID_INLINE idQuat& idQuat::operator*=( const idQuat& a )
{
	*this = *this * a;

	return *this;
}

ID_INLINE idQuat& idQuat::operator*=( float a )
{
	x *= a;
	y *= a;
	z *= a;
	w *= a;

	return *this;
}

ID_INLINE bool idQuat::Compare( const idQuat& a ) const
{
	return ( ( x == a.x ) && ( y == a.y ) && ( z == a.z ) && ( w == a.w ) );
}

ID_INLINE bool idQuat::Compare( const idQuat& a, const float epsilon ) const
{
	if( idMath::Fabs( x - a.x ) > epsilon )
	{
		return false;
	}
	if( idMath::Fabs( y - a.y ) > epsilon )
	{
		return false;
	}
	if( idMath::Fabs( z - a.z ) > epsilon )
	{
		return false;
	}
	if( idMath::Fabs( w - a.w ) > epsilon )
	{
		return false;
	}
	return true;
}

ID_INLINE bool idQuat::operator==( const idQuat& a ) const
{
	return Compare( a );
}

ID_INLINE bool idQuat::operator!=( const idQuat& a ) const
{
	return !Compare( a );
}

ID_INLINE void idQuat::Set( float x, float y, float z, float w )
{
	this->x = x;
	this->y = y;
	this->z = z;
	this->w = w;
}

ID_INLINE idQuat idQuat::Inverse() const
{
	return idQuat( -x, -y, -z, w );
}

ID_INLINE float idQuat::Length() const
{
	float len;

	len = x * x + y * y + z * z + w * w;
	return idMath::Sqrt( len );
}

ID_INLINE idQuat& idQuat::Normalize()
{
	float len;
	float ilength;

	len = this->Length();
	if( len )
	{
		ilength = 1 / len;
		x *= ilength;
		y *= ilength;
		z *= ilength;
		w *= ilength;
	}
	return *this;
}

ID_INLINE float idQuat::CalcW() const
{
	// take the absolute value because floating point rounding may cause the dot of x,y,z to be larger than 1
	return sqrt( fabs( 1.0f - ( x * x + y * y + z * z ) ) );
}

ID_INLINE int idQuat::GetDimension() const
{
	return 4;
}

ID_INLINE const float* idQuat::ToFloatPtr() const
{
	return &x;
}

ID_INLINE float* idQuat::ToFloatPtr()
{
	return &x;
}

/*
===============================================================================

	Specialization to get size of an idQuat generically.

===============================================================================
*/
template<>
struct idTupleSize< idQuat >
{
	enum { value = 4 };
};

/*
===============================================================================

	Compressed quaternion

===============================================================================
*/

class idCQuat
{
public:
	float			x;
	float			y;
	float			z;

	idCQuat();
	idCQuat( float x, float y, float z );

	void 			Set( float x, float y, float z );

	float			operator[]( int index ) const;
	float& 			operator[]( int index );

	bool			Compare( const idCQuat& a ) const;						// exact compare, no epsilon
	bool			Compare( const idCQuat& a, const float epsilon ) const;	// compare with epsilon
	bool			operator==(	const idCQuat& a ) const;					// exact compare, no epsilon
	bool			operator!=(	const idCQuat& a ) const;					// exact compare, no epsilon

	int				GetDimension() const;

	idAngles		ToAngles() const;
	idRotation		ToRotation() const;
	idMat3			ToMat3() const;
	idMat4			ToMat4() const;
	idQuat			ToQuat() const;
	const float* 	ToFloatPtr() const;
	float* 			ToFloatPtr();
	const char* 	ToString( int precision = 2 ) const;
};

ID_INLINE idCQuat::idCQuat()
{
}

ID_INLINE idCQuat::idCQuat( float x, float y, float z )
{
	this->x = x;
	this->y = y;
	this->z = z;
}

ID_INLINE void idCQuat::Set( float x, float y, float z )
{
	this->x = x;
	this->y = y;
	this->z = z;
}

ID_INLINE float idCQuat::operator[]( int index ) const
{
	assert( ( index >= 0 ) && ( index < 3 ) );
	return ( &x )[ index ];
}

ID_INLINE float& idCQuat::operator[]( int index )
{
	assert( ( index >= 0 ) && ( index < 3 ) );
	return ( &x )[ index ];
}

ID_INLINE bool idCQuat::Compare( const idCQuat& a ) const
{
	return ( ( x == a.x ) && ( y == a.y ) && ( z == a.z ) );
}

ID_INLINE bool idCQuat::Compare( const idCQuat& a, const float epsilon ) const
{
	if( idMath::Fabs( x - a.x ) > epsilon )
	{
		return false;
	}
	if( idMath::Fabs( y - a.y ) > epsilon )
	{
		return false;
	}
	if( idMath::Fabs( z - a.z ) > epsilon )
	{
		return false;
	}
	return true;
}

ID_INLINE bool idCQuat::operator==( const idCQuat& a ) const
{
	return Compare( a );
}

ID_INLINE bool idCQuat::operator!=( const idCQuat& a ) const
{
	return !Compare( a );
}

ID_INLINE int idCQuat::GetDimension() const
{
	return 3;
}

ID_INLINE idQuat idCQuat::ToQuat() const
{
	// take the absolute value because floating point rounding may cause the dot of x,y,z to be larger than 1
	return idQuat( x, y, z, sqrt( fabs( 1.0f - ( x * x + y * y + z * z ) ) ) );
}

ID_INLINE const float* idCQuat::ToFloatPtr() const
{
	return &x;
}

ID_INLINE float* idCQuat::ToFloatPtr()
{
	return &x;
}

/*
===============================================================================

	Specialization to get size of an idCQuat generically.

===============================================================================
*/
template<>
struct idTupleSize< idCQuat >
{
	enum { value = 3 };
};

#endif /* !__MATH_QUAT_H__ */
