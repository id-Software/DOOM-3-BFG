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

#ifndef __MATH_MATRIX_H__
#define __MATH_MATRIX_H__

/*
===============================================================================

  Matrix classes, all matrices are row-major except idMat3

===============================================================================
*/

#define MATRIX_INVERSE_EPSILON		1e-14
#define MATRIX_EPSILON				1e-6

class idAngles;
class idQuat;
class idCQuat;
class idRotation;
class idMat4;

//===============================================================
//
//	idMat2 - 2x2 matrix
//
//===============================================================

class idMat2
{
public:
	idMat2();
	explicit idMat2( const idVec2& x, const idVec2& y );
	explicit idMat2( const float xx, const float xy, const float yx, const float yy );
	explicit idMat2( const float src[ 2 ][ 2 ] );

	const idVec2& 	operator[]( int index ) const;
	idVec2& 		operator[]( int index );
	idMat2			operator-() const;
	idMat2			operator*( const float a ) const;
	idVec2			operator*( const idVec2& vec ) const;
	idMat2			operator*( const idMat2& a ) const;
	idMat2			operator+( const idMat2& a ) const;
	idMat2			operator-( const idMat2& a ) const;
	idMat2& 		operator*=( const float a );
	idMat2& 		operator*=( const idMat2& a );
	idMat2& 		operator+=( const idMat2& a );
	idMat2& 		operator-=( const idMat2& a );

	friend idMat2	operator*( const float a, const idMat2& mat );
	friend idVec2	operator*( const idVec2& vec, const idMat2& mat );
	friend idVec2& 	operator*=( idVec2& vec, const idMat2& mat );

	bool			Compare( const idMat2& a ) const;						// exact compare, no epsilon
	bool			Compare( const idMat2& a, const float epsilon ) const;	// compare with epsilon
	bool			operator==( const idMat2& a ) const;					// exact compare, no epsilon
	bool			operator!=( const idMat2& a ) const;					// exact compare, no epsilon

	void			Zero();
	void			Identity();
	bool			IsIdentity( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsSymmetric( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsDiagonal( const float epsilon = MATRIX_EPSILON ) const;

	float			Trace() const;
	float			Determinant() const;
	idMat2			Transpose() const;	// returns transpose
	idMat2& 		TransposeSelf();
	idMat2			Inverse() const;		// returns the inverse ( m * m.Inverse() = identity )
	bool			InverseSelf();		// returns false if determinant is zero
	idMat2			InverseFast() const;	// returns the inverse ( m * m.Inverse() = identity )
	bool			InverseFastSelf();	// returns false if determinant is zero

	int				GetDimension() const;

	const float* 	ToFloatPtr() const;
	float* 			ToFloatPtr();
	const char* 	ToString( int precision = 2 ) const;

private:
	idVec2			mat[ 2 ];
};

extern idMat2 mat2_zero;
extern idMat2 mat2_identity;
#define mat2_default	mat2_identity

ID_INLINE idMat2::idMat2()
{
}

ID_INLINE idMat2::idMat2( const idVec2& x, const idVec2& y )
{
	mat[ 0 ].x = x.x;
	mat[ 0 ].y = x.y;
	mat[ 1 ].x = y.x;
	mat[ 1 ].y = y.y;
}

ID_INLINE idMat2::idMat2( const float xx, const float xy, const float yx, const float yy )
{
	mat[ 0 ].x = xx;
	mat[ 0 ].y = xy;
	mat[ 1 ].x = yx;
	mat[ 1 ].y = yy;
}

ID_INLINE idMat2::idMat2( const float src[ 2 ][ 2 ] )
{
	memcpy( mat, src, 2 * 2 * sizeof( float ) );
}

ID_INLINE const idVec2& idMat2::operator[]( int index ) const
{
	//assert( ( index >= 0 ) && ( index < 2 ) );
	return mat[ index ];
}

ID_INLINE idVec2& idMat2::operator[]( int index )
{
	//assert( ( index >= 0 ) && ( index < 2 ) );
	return mat[ index ];
}

ID_INLINE idMat2 idMat2::operator-() const
{
	return idMat2(	-mat[0][0], -mat[0][1],
					-mat[1][0], -mat[1][1] );
}

ID_INLINE idVec2 idMat2::operator*( const idVec2& vec ) const
{
	return idVec2(
			   mat[ 0 ].x * vec.x + mat[ 0 ].y * vec.y,
			   mat[ 1 ].x * vec.x + mat[ 1 ].y * vec.y );
}

ID_INLINE idMat2 idMat2::operator*( const idMat2& a ) const
{
	return idMat2(
			   mat[0].x * a[0].x + mat[0].y * a[1].x,
			   mat[0].x * a[0].y + mat[0].y * a[1].y,
			   mat[1].x * a[0].x + mat[1].y * a[1].x,
			   mat[1].x * a[0].y + mat[1].y * a[1].y );
}

ID_INLINE idMat2 idMat2::operator*( const float a ) const
{
	return idMat2(
			   mat[0].x * a, mat[0].y * a,
			   mat[1].x * a, mat[1].y * a );
}

ID_INLINE idMat2 idMat2::operator+( const idMat2& a ) const
{
	return idMat2(
			   mat[0].x + a[0].x, mat[0].y + a[0].y,
			   mat[1].x + a[1].x, mat[1].y + a[1].y );
}

ID_INLINE idMat2 idMat2::operator-( const idMat2& a ) const
{
	return idMat2(
			   mat[0].x - a[0].x, mat[0].y - a[0].y,
			   mat[1].x - a[1].x, mat[1].y - a[1].y );
}

ID_INLINE idMat2& idMat2::operator*=( const float a )
{
	mat[0].x *= a;
	mat[0].y *= a;
	mat[1].x *= a;
	mat[1].y *= a;

	return *this;
}

ID_INLINE idMat2& idMat2::operator*=( const idMat2& a )
{
	float x, y;
	x = mat[0].x;
	y = mat[0].y;
	mat[0].x = x * a[0].x + y * a[1].x;
	mat[0].y = x * a[0].y + y * a[1].y;
	x = mat[1].x;
	y = mat[1].y;
	mat[1].x = x * a[0].x + y * a[1].x;
	mat[1].y = x * a[0].y + y * a[1].y;
	return *this;
}

ID_INLINE idMat2& idMat2::operator+=( const idMat2& a )
{
	mat[0].x += a[0].x;
	mat[0].y += a[0].y;
	mat[1].x += a[1].x;
	mat[1].y += a[1].y;

	return *this;
}

ID_INLINE idMat2& idMat2::operator-=( const idMat2& a )
{
	mat[0].x -= a[0].x;
	mat[0].y -= a[0].y;
	mat[1].x -= a[1].x;
	mat[1].y -= a[1].y;

	return *this;
}

ID_INLINE idVec2 operator*( const idVec2& vec, const idMat2& mat )
{
	return mat * vec;
}

ID_INLINE idMat2 operator*( const float a, idMat2 const& mat )
{
	return mat * a;
}

ID_INLINE idVec2& operator*=( idVec2& vec, const idMat2& mat )
{
	vec = mat * vec;
	return vec;
}

ID_INLINE bool idMat2::Compare( const idMat2& a ) const
{
	if( mat[0].Compare( a[0] ) &&
			mat[1].Compare( a[1] ) )
	{
		return true;
	}
	return false;
}

ID_INLINE bool idMat2::Compare( const idMat2& a, const float epsilon ) const
{
	if( mat[0].Compare( a[0], epsilon ) &&
			mat[1].Compare( a[1], epsilon ) )
	{
		return true;
	}
	return false;
}

ID_INLINE bool idMat2::operator==( const idMat2& a ) const
{
	return Compare( a );
}

ID_INLINE bool idMat2::operator!=( const idMat2& a ) const
{
	return !Compare( a );
}

ID_INLINE void idMat2::Zero()
{
	mat[0].Zero();
	mat[1].Zero();
}

ID_INLINE void idMat2::Identity()
{
	*this = mat2_identity;
}

ID_INLINE bool idMat2::IsIdentity( const float epsilon ) const
{
	return Compare( mat2_identity, epsilon );
}

ID_INLINE bool idMat2::IsSymmetric( const float epsilon ) const
{
	return ( idMath::Fabs( mat[0][1] - mat[1][0] ) < epsilon );
}

ID_INLINE bool idMat2::IsDiagonal( const float epsilon ) const
{
	if( idMath::Fabs( mat[0][1] ) > epsilon ||
			idMath::Fabs( mat[1][0] ) > epsilon )
	{
		return false;
	}
	return true;
}

ID_INLINE float idMat2::Trace() const
{
	return ( mat[0][0] + mat[1][1] );
}

ID_INLINE float idMat2::Determinant() const
{
	return mat[0][0] * mat[1][1] - mat[0][1] * mat[1][0];
}

ID_INLINE idMat2 idMat2::Transpose() const
{
	return idMat2(	mat[0][0], mat[1][0],
					mat[0][1], mat[1][1] );
}

ID_INLINE idMat2& idMat2::TransposeSelf()
{
	float tmp;

	tmp = mat[0][1];
	mat[0][1] = mat[1][0];
	mat[1][0] = tmp;

	return *this;
}

ID_INLINE idMat2 idMat2::Inverse() const
{
	idMat2 invMat;

	invMat = *this;
	verify( invMat.InverseSelf() );
	return invMat;
}

ID_INLINE idMat2 idMat2::InverseFast() const
{
	idMat2 invMat;

	invMat = *this;
	verify( invMat.InverseFastSelf() );
	return invMat;
}

ID_INLINE int idMat2::GetDimension() const
{
	return 4;
}

ID_INLINE const float* idMat2::ToFloatPtr() const
{
	return mat[0].ToFloatPtr();
}

ID_INLINE float* idMat2::ToFloatPtr()
{
	return mat[0].ToFloatPtr();
}


//===============================================================
//
//	idMat3 - 3x3 matrix
//
//	NOTE:	matrix is column-major
//
//===============================================================

class idMat3
{
public:
	idMat3();
	explicit idMat3( const idVec3& x, const idVec3& y, const idVec3& z );
	explicit idMat3( const float xx, const float xy, const float xz, const float yx, const float yy, const float yz, const float zx, const float zy, const float zz );
	explicit idMat3( const float src[ 3 ][ 3 ] );

	const idVec3& 	operator[]( int index ) const;
	idVec3& 		operator[]( int index );
	idMat3			operator-() const;
	idMat3			operator*( const float a ) const;
	idVec3			operator*( const idVec3& vec ) const;
	idMat3			operator*( const idMat3& a ) const;
	idMat3			operator+( const idMat3& a ) const;
	idMat3			operator-( const idMat3& a ) const;
	idMat3& 		operator*=( const float a );
	idMat3& 		operator*=( const idMat3& a );
	idMat3& 		operator+=( const idMat3& a );
	idMat3& 		operator-=( const idMat3& a );

	friend idMat3	operator*( const float a, const idMat3& mat );
	friend idVec3	operator*( const idVec3& vec, const idMat3& mat );
	friend idVec3& 	operator*=( idVec3& vec, const idMat3& mat );

	bool			Compare( const idMat3& a ) const;						// exact compare, no epsilon
	bool			Compare( const idMat3& a, const float epsilon ) const;	// compare with epsilon
	bool			operator==( const idMat3& a ) const;					// exact compare, no epsilon
	bool			operator!=( const idMat3& a ) const;					// exact compare, no epsilon

	void			Zero();
	void			Identity();
	bool			IsIdentity( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsSymmetric( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsDiagonal( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsRotated() const;

	void			ProjectVector( const idVec3& src, idVec3& dst ) const;
	void			UnprojectVector( const idVec3& src, idVec3& dst ) const;

	bool			FixDegeneracies();	// fix degenerate axial cases
	bool			FixDenormals();		// change tiny numbers to zero

	float			Trace() const;
	float			Determinant() const;
	idMat3			OrthoNormalize() const;
	idMat3& 		OrthoNormalizeSelf();
	idMat3			Transpose() const;	// returns transpose
	idMat3& 		TransposeSelf();
	idMat3			Inverse() const;		// returns the inverse ( m * m.Inverse() = identity )
	bool			InverseSelf();		// returns false if determinant is zero
	idMat3			InverseFast() const;	// returns the inverse ( m * m.Inverse() = identity )
	bool			InverseFastSelf();	// returns false if determinant is zero
	idMat3			TransposeMultiply( const idMat3& b ) const;

	idMat3			InertiaTranslate( const float mass, const idVec3& centerOfMass, const idVec3& translation ) const;
	idMat3& 		InertiaTranslateSelf( const float mass, const idVec3& centerOfMass, const idVec3& translation );
	idMat3			InertiaRotate( const idMat3& rotation ) const;
	idMat3& 		InertiaRotateSelf( const idMat3& rotation );

	int				GetDimension() const;

	idAngles		ToAngles() const;
	idQuat			ToQuat() const;
	idCQuat			ToCQuat() const;
	idRotation		ToRotation() const;
	idMat4			ToMat4() const;
	idVec3			ToAngularVelocity() const;
	const float* 	ToFloatPtr() const;
	float* 			ToFloatPtr();
	const char* 	ToString( int precision = 2 ) const;

	friend void		TransposeMultiply( const idMat3& inv, const idMat3& b, idMat3& dst );
	friend idMat3	SkewSymmetric( idVec3 const& src );

private:
	idVec3			mat[ 3 ];
};

extern idMat3 mat3_zero;
extern idMat3 mat3_identity;
#define mat3_default	mat3_identity

ID_INLINE idMat3::idMat3()
{
}

ID_INLINE idMat3::idMat3( const idVec3& x, const idVec3& y, const idVec3& z )
{
	mat[ 0 ].x = x.x;
	mat[ 0 ].y = x.y;
	mat[ 0 ].z = x.z;
	mat[ 1 ].x = y.x;
	mat[ 1 ].y = y.y;
	mat[ 1 ].z = y.z;
	mat[ 2 ].x = z.x;
	mat[ 2 ].y = z.y;
	mat[ 2 ].z = z.z;
}

ID_INLINE idMat3::idMat3( const float xx, const float xy, const float xz, const float yx, const float yy, const float yz, const float zx, const float zy, const float zz )
{
	mat[ 0 ].x = xx;
	mat[ 0 ].y = xy;
	mat[ 0 ].z = xz;
	mat[ 1 ].x = yx;
	mat[ 1 ].y = yy;
	mat[ 1 ].z = yz;
	mat[ 2 ].x = zx;
	mat[ 2 ].y = zy;
	mat[ 2 ].z = zz;
}

ID_INLINE idMat3::idMat3( const float src[ 3 ][ 3 ] )
{
	memcpy( mat, src, 3 * 3 * sizeof( float ) );
}

ID_INLINE const idVec3& idMat3::operator[]( int index ) const
{
	//assert( ( index >= 0 ) && ( index < 3 ) );
	return mat[ index ];
}

ID_INLINE idVec3& idMat3::operator[]( int index )
{
	//assert( ( index >= 0 ) && ( index < 3 ) );
	return mat[ index ];
}

ID_INLINE idMat3 idMat3::operator-() const
{
	return idMat3(	-mat[0][0], -mat[0][1], -mat[0][2],
					-mat[1][0], -mat[1][1], -mat[1][2],
					-mat[2][0], -mat[2][1], -mat[2][2] );
}

ID_INLINE idVec3 idMat3::operator*( const idVec3& vec ) const
{
	return idVec3(
			   mat[ 0 ].x * vec.x + mat[ 1 ].x * vec.y + mat[ 2 ].x * vec.z,
			   mat[ 0 ].y * vec.x + mat[ 1 ].y * vec.y + mat[ 2 ].y * vec.z,
			   mat[ 0 ].z * vec.x + mat[ 1 ].z * vec.y + mat[ 2 ].z * vec.z );
}

ID_INLINE idMat3 idMat3::operator*( const idMat3& a ) const
{
	int i, j;
	const float* m1Ptr, *m2Ptr;
	float* dstPtr;
	idMat3 dst;

	m1Ptr = reinterpret_cast<const float*>( this );
	m2Ptr = reinterpret_cast<const float*>( &a );
	dstPtr = reinterpret_cast<float*>( &dst );

	for( i = 0; i < 3; i++ )
	{
		for( j = 0; j < 3; j++ )
		{
			*dstPtr = m1Ptr[0] * m2Ptr[ 0 * 3 + j ]
					  + m1Ptr[1] * m2Ptr[ 1 * 3 + j ]
					  + m1Ptr[2] * m2Ptr[ 2 * 3 + j ];
			dstPtr++;
		}
		m1Ptr += 3;
	}
	return dst;
}

ID_INLINE idMat3 idMat3::operator*( const float a ) const
{
	return idMat3(
			   mat[0].x * a, mat[0].y * a, mat[0].z * a,
			   mat[1].x * a, mat[1].y * a, mat[1].z * a,
			   mat[2].x * a, mat[2].y * a, mat[2].z * a );
}

ID_INLINE idMat3 idMat3::operator+( const idMat3& a ) const
{
	return idMat3(
			   mat[0].x + a[0].x, mat[0].y + a[0].y, mat[0].z + a[0].z,
			   mat[1].x + a[1].x, mat[1].y + a[1].y, mat[1].z + a[1].z,
			   mat[2].x + a[2].x, mat[2].y + a[2].y, mat[2].z + a[2].z );
}

ID_INLINE idMat3 idMat3::operator-( const idMat3& a ) const
{
	return idMat3(
			   mat[0].x - a[0].x, mat[0].y - a[0].y, mat[0].z - a[0].z,
			   mat[1].x - a[1].x, mat[1].y - a[1].y, mat[1].z - a[1].z,
			   mat[2].x - a[2].x, mat[2].y - a[2].y, mat[2].z - a[2].z );
}

ID_INLINE idMat3& idMat3::operator*=( const float a )
{
	mat[0].x *= a;
	mat[0].y *= a;
	mat[0].z *= a;
	mat[1].x *= a;
	mat[1].y *= a;
	mat[1].z *= a;
	mat[2].x *= a;
	mat[2].y *= a;
	mat[2].z *= a;

	return *this;
}

ID_INLINE idMat3& idMat3::operator*=( const idMat3& a )
{
	int i, j;
	const float* m2Ptr;
	float* m1Ptr, dst[3];

	m1Ptr = reinterpret_cast<float*>( this );
	m2Ptr = reinterpret_cast<const float*>( &a );

	for( i = 0; i < 3; i++ )
	{
		for( j = 0; j < 3; j++ )
		{
			dst[j]  = m1Ptr[0] * m2Ptr[ 0 * 3 + j ]
					  + m1Ptr[1] * m2Ptr[ 1 * 3 + j ]
					  + m1Ptr[2] * m2Ptr[ 2 * 3 + j ];
		}
		m1Ptr[0] = dst[0];
		m1Ptr[1] = dst[1];
		m1Ptr[2] = dst[2];
		m1Ptr += 3;
	}
	return *this;
}

ID_INLINE idMat3& idMat3::operator+=( const idMat3& a )
{
	mat[0].x += a[0].x;
	mat[0].y += a[0].y;
	mat[0].z += a[0].z;
	mat[1].x += a[1].x;
	mat[1].y += a[1].y;
	mat[1].z += a[1].z;
	mat[2].x += a[2].x;
	mat[2].y += a[2].y;
	mat[2].z += a[2].z;

	return *this;
}

ID_INLINE idMat3& idMat3::operator-=( const idMat3& a )
{
	mat[0].x -= a[0].x;
	mat[0].y -= a[0].y;
	mat[0].z -= a[0].z;
	mat[1].x -= a[1].x;
	mat[1].y -= a[1].y;
	mat[1].z -= a[1].z;
	mat[2].x -= a[2].x;
	mat[2].y -= a[2].y;
	mat[2].z -= a[2].z;

	return *this;
}

ID_INLINE idVec3 operator*( const idVec3& vec, const idMat3& mat )
{
	return mat * vec;
}

ID_INLINE idMat3 operator*( const float a, const idMat3& mat )
{
	return mat * a;
}

ID_INLINE idVec3& operator*=( idVec3& vec, const idMat3& mat )
{
	float x = mat[ 0 ].x * vec.x + mat[ 1 ].x * vec.y + mat[ 2 ].x * vec.z;
	float y = mat[ 0 ].y * vec.x + mat[ 1 ].y * vec.y + mat[ 2 ].y * vec.z;
	vec.z = mat[ 0 ].z * vec.x + mat[ 1 ].z * vec.y + mat[ 2 ].z * vec.z;
	vec.x = x;
	vec.y = y;
	return vec;
}

ID_INLINE bool idMat3::Compare( const idMat3& a ) const
{
	if( mat[0].Compare( a[0] ) &&
			mat[1].Compare( a[1] ) &&
			mat[2].Compare( a[2] ) )
	{
		return true;
	}
	return false;
}

ID_INLINE bool idMat3::Compare( const idMat3& a, const float epsilon ) const
{
	if( mat[0].Compare( a[0], epsilon ) &&
			mat[1].Compare( a[1], epsilon ) &&
			mat[2].Compare( a[2], epsilon ) )
	{
		return true;
	}
	return false;
}

ID_INLINE bool idMat3::operator==( const idMat3& a ) const
{
	return Compare( a );
}

ID_INLINE bool idMat3::operator!=( const idMat3& a ) const
{
	return !Compare( a );
}

ID_INLINE void idMat3::Zero()
{
	memset( mat, 0, sizeof( idMat3 ) );
}

ID_INLINE void idMat3::Identity()
{
	*this = mat3_identity;
}

ID_INLINE bool idMat3::IsIdentity( const float epsilon ) const
{
	return Compare( mat3_identity, epsilon );
}

ID_INLINE bool idMat3::IsSymmetric( const float epsilon ) const
{
	if( idMath::Fabs( mat[0][1] - mat[1][0] ) > epsilon )
	{
		return false;
	}
	if( idMath::Fabs( mat[0][2] - mat[2][0] ) > epsilon )
	{
		return false;
	}
	if( idMath::Fabs( mat[1][2] - mat[2][1] ) > epsilon )
	{
		return false;
	}
	return true;
}

ID_INLINE bool idMat3::IsDiagonal( const float epsilon ) const
{
	if( idMath::Fabs( mat[0][1] ) > epsilon ||
			idMath::Fabs( mat[0][2] ) > epsilon ||
			idMath::Fabs( mat[1][0] ) > epsilon ||
			idMath::Fabs( mat[1][2] ) > epsilon ||
			idMath::Fabs( mat[2][0] ) > epsilon ||
			idMath::Fabs( mat[2][1] ) > epsilon )
	{
		return false;
	}
	return true;
}

ID_INLINE bool idMat3::IsRotated() const
{
	return !Compare( mat3_identity );
}

ID_INLINE void idMat3::ProjectVector( const idVec3& src, idVec3& dst ) const
{
	dst.x = src * mat[ 0 ];
	dst.y = src * mat[ 1 ];
	dst.z = src * mat[ 2 ];
}

ID_INLINE void idMat3::UnprojectVector( const idVec3& src, idVec3& dst ) const
{
	dst = mat[ 0 ] * src.x + mat[ 1 ] * src.y + mat[ 2 ] * src.z;
}

ID_INLINE bool idMat3::FixDegeneracies()
{
	bool r = mat[0].FixDegenerateNormal();
	r |= mat[1].FixDegenerateNormal();
	r |= mat[2].FixDegenerateNormal();
	return r;
}

ID_INLINE bool idMat3::FixDenormals()
{
	bool r = mat[0].FixDenormals();
	r |= mat[1].FixDenormals();
	r |= mat[2].FixDenormals();
	return r;
}

ID_INLINE float idMat3::Trace() const
{
	return ( mat[0][0] + mat[1][1] + mat[2][2] );
}

ID_INLINE idMat3 idMat3::OrthoNormalize() const
{
	idMat3 ortho;

	ortho = *this;
	ortho[ 0 ].Normalize();
	ortho[ 2 ].Cross( mat[ 0 ], mat[ 1 ] );
	ortho[ 2 ].Normalize();
	ortho[ 1 ].Cross( mat[ 2 ], mat[ 0 ] );
	ortho[ 1 ].Normalize();
	return ortho;
}

ID_INLINE idMat3& idMat3::OrthoNormalizeSelf()
{
	mat[ 0 ].Normalize();
	mat[ 2 ].Cross( mat[ 0 ], mat[ 1 ] );
	mat[ 2 ].Normalize();
	mat[ 1 ].Cross( mat[ 2 ], mat[ 0 ] );
	mat[ 1 ].Normalize();
	return *this;
}

ID_INLINE idMat3 idMat3::Transpose() const
{
	return idMat3(	mat[0][0], mat[1][0], mat[2][0],
					mat[0][1], mat[1][1], mat[2][1],
					mat[0][2], mat[1][2], mat[2][2] );
}

ID_INLINE idMat3& idMat3::TransposeSelf()
{
	float tmp0, tmp1, tmp2;

	tmp0 = mat[0][1];
	mat[0][1] = mat[1][0];
	mat[1][0] = tmp0;
	tmp1 = mat[0][2];
	mat[0][2] = mat[2][0];
	mat[2][0] = tmp1;
	tmp2 = mat[1][2];
	mat[1][2] = mat[2][1];
	mat[2][1] = tmp2;

	return *this;
}

ID_INLINE idMat3 idMat3::Inverse() const
{
	idMat3 invMat;

	invMat = *this;
	verify( invMat.InverseSelf() );
	return invMat;
}

ID_INLINE idMat3 idMat3::InverseFast() const
{
	idMat3 invMat;

	invMat = *this;
	verify( invMat.InverseFastSelf() );
	return invMat;
}

ID_INLINE idMat3 idMat3::TransposeMultiply( const idMat3& b ) const
{
	return idMat3(	mat[0].x * b[0].x + mat[1].x * b[1].x + mat[2].x * b[2].x,
					mat[0].x * b[0].y + mat[1].x * b[1].y + mat[2].x * b[2].y,
					mat[0].x * b[0].z + mat[1].x * b[1].z + mat[2].x * b[2].z,
					mat[0].y * b[0].x + mat[1].y * b[1].x + mat[2].y * b[2].x,
					mat[0].y * b[0].y + mat[1].y * b[1].y + mat[2].y * b[2].y,
					mat[0].y * b[0].z + mat[1].y * b[1].z + mat[2].y * b[2].z,
					mat[0].z * b[0].x + mat[1].z * b[1].x + mat[2].z * b[2].x,
					mat[0].z * b[0].y + mat[1].z * b[1].y + mat[2].z * b[2].y,
					mat[0].z * b[0].z + mat[1].z * b[1].z + mat[2].z * b[2].z );
}

ID_INLINE void TransposeMultiply( const idMat3& transpose, const idMat3& b, idMat3& dst )
{
	dst[0].x = transpose[0].x * b[0].x + transpose[1].x * b[1].x + transpose[2].x * b[2].x;
	dst[0].y = transpose[0].x * b[0].y + transpose[1].x * b[1].y + transpose[2].x * b[2].y;
	dst[0].z = transpose[0].x * b[0].z + transpose[1].x * b[1].z + transpose[2].x * b[2].z;
	dst[1].x = transpose[0].y * b[0].x + transpose[1].y * b[1].x + transpose[2].y * b[2].x;
	dst[1].y = transpose[0].y * b[0].y + transpose[1].y * b[1].y + transpose[2].y * b[2].y;
	dst[1].z = transpose[0].y * b[0].z + transpose[1].y * b[1].z + transpose[2].y * b[2].z;
	dst[2].x = transpose[0].z * b[0].x + transpose[1].z * b[1].x + transpose[2].z * b[2].x;
	dst[2].y = transpose[0].z * b[0].y + transpose[1].z * b[1].y + transpose[2].z * b[2].y;
	dst[2].z = transpose[0].z * b[0].z + transpose[1].z * b[1].z + transpose[2].z * b[2].z;
}

ID_INLINE idMat3 SkewSymmetric( idVec3 const& src )
{
	return idMat3( 0.0f, -src.z,  src.y, src.z,   0.0f, -src.x, -src.y,  src.x,   0.0f );
}

ID_INLINE int idMat3::GetDimension() const
{
	return 9;
}

ID_INLINE const float* idMat3::ToFloatPtr() const
{
	return mat[0].ToFloatPtr();
}

ID_INLINE float* idMat3::ToFloatPtr()
{
	return mat[0].ToFloatPtr();
}


//===============================================================
//
//	idMat4 - 4x4 matrix
//
//===============================================================

class idMat4
{
public:
	idMat4();
	explicit idMat4( const idVec4& x, const idVec4& y, const idVec4& z, const idVec4& w );
	explicit idMat4( const float xx, const float xy, const float xz, const float xw,
					 const float yx, const float yy, const float yz, const float yw,
					 const float zx, const float zy, const float zz, const float zw,
					 const float wx, const float wy, const float wz, const float ww );
	explicit idMat4( const idMat3& rotation, const idVec3& translation );
	explicit idMat4( const float src[ 4 ][ 4 ] );

	const idVec4& 	operator[]( int index ) const;
	idVec4& 		operator[]( int index );
	idMat4			operator*( const float a ) const;
	idVec4			operator*( const idVec4& vec ) const;
	idVec3			operator*( const idVec3& vec ) const;
	idMat4			operator*( const idMat4& a ) const;
	idMat4			operator+( const idMat4& a ) const;
	idMat4			operator-( const idMat4& a ) const;
	idMat4& 		operator*=( const float a );
	idMat4& 		operator*=( const idMat4& a );
	idMat4& 		operator+=( const idMat4& a );
	idMat4& 		operator-=( const idMat4& a );

	friend idMat4	operator*( const float a, const idMat4& mat );
	friend idVec4	operator*( const idVec4& vec, const idMat4& mat );
	friend idVec3	operator*( const idVec3& vec, const idMat4& mat );
	friend idVec4& 	operator*=( idVec4& vec, const idMat4& mat );
	friend idVec3& 	operator*=( idVec3& vec, const idMat4& mat );

	bool			Compare( const idMat4& a ) const;						// exact compare, no epsilon
	bool			Compare( const idMat4& a, const float epsilon ) const;	// compare with epsilon
	bool			operator==( const idMat4& a ) const;					// exact compare, no epsilon
	bool			operator!=( const idMat4& a ) const;					// exact compare, no epsilon

	void			Zero();
	void			Identity();
	bool			IsIdentity( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsSymmetric( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsDiagonal( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsRotated() const;

	void			ProjectVector( const idVec4& src, idVec4& dst ) const;
	void			UnprojectVector( const idVec4& src, idVec4& dst ) const;

	float			Trace() const;
	float			Determinant() const;
	idMat4			Transpose() const;	// returns transpose
	idMat4& 		TransposeSelf();
	idMat4			Inverse() const;		// returns the inverse ( m * m.Inverse() = identity )
	bool			InverseSelf();		// returns false if determinant is zero
	idMat4			InverseFast() const;	// returns the inverse ( m * m.Inverse() = identity )
	bool			InverseFastSelf();	// returns false if determinant is zero
	idMat4			TransposeMultiply( const idMat4& b ) const;

	int				GetDimension() const;

	const float* 	ToFloatPtr() const;
	float* 			ToFloatPtr();
	const char* 	ToString( int precision = 2 ) const;
// jmarshall
	idMat3			ToMat3() const;
// jmarshall end
private:
	idVec4			mat[ 4 ];
};

extern idMat4 mat4_zero;
extern idMat4 mat4_identity;
#define mat4_default	mat4_identity

// jmarshall
ID_INLINE idMat3 idMat4::ToMat3() const
{
	idMat3 m;

	m[0][0] = mat[0][0];
	m[0][1] = mat[0][1];
	m[0][2] = mat[0][2];
	m[1][0] = mat[1][0];
	m[1][1] = mat[1][1];
	m[1][2] = mat[1][2];
	m[2][0] = mat[2][0];
	m[2][1] = mat[2][1];
	m[2][2] = mat[2][2];

	return m;
}
// jmarshall end

ID_INLINE idMat4::idMat4()
{
}

ID_INLINE idMat4::idMat4( const idVec4& x, const idVec4& y, const idVec4& z, const idVec4& w )
{
	mat[ 0 ] = x;
	mat[ 1 ] = y;
	mat[ 2 ] = z;
	mat[ 3 ] = w;
}

ID_INLINE idMat4::idMat4( const float xx, const float xy, const float xz, const float xw,
						  const float yx, const float yy, const float yz, const float yw,
						  const float zx, const float zy, const float zz, const float zw,
						  const float wx, const float wy, const float wz, const float ww )
{
	mat[0][0] = xx;
	mat[0][1] = xy;
	mat[0][2] = xz;
	mat[0][3] = xw;
	mat[1][0] = yx;
	mat[1][1] = yy;
	mat[1][2] = yz;
	mat[1][3] = yw;
	mat[2][0] = zx;
	mat[2][1] = zy;
	mat[2][2] = zz;
	mat[2][3] = zw;
	mat[3][0] = wx;
	mat[3][1] = wy;
	mat[3][2] = wz;
	mat[3][3] = ww;
}

ID_INLINE idMat4::idMat4( const idMat3& rotation, const idVec3& translation )
{
	// NOTE: idMat3 is transposed because it is column-major
	mat[ 0 ][ 0 ] = rotation[0][0];
	mat[ 0 ][ 1 ] = rotation[1][0];
	mat[ 0 ][ 2 ] = rotation[2][0];
	mat[ 0 ][ 3 ] = translation[0];
	mat[ 1 ][ 0 ] = rotation[0][1];
	mat[ 1 ][ 1 ] = rotation[1][1];
	mat[ 1 ][ 2 ] = rotation[2][1];
	mat[ 1 ][ 3 ] = translation[1];
	mat[ 2 ][ 0 ] = rotation[0][2];
	mat[ 2 ][ 1 ] = rotation[1][2];
	mat[ 2 ][ 2 ] = rotation[2][2];
	mat[ 2 ][ 3 ] = translation[2];
	mat[ 3 ][ 0 ] = 0.0f;
	mat[ 3 ][ 1 ] = 0.0f;
	mat[ 3 ][ 2 ] = 0.0f;
	mat[ 3 ][ 3 ] = 1.0f;
}

ID_INLINE idMat4::idMat4( const float src[ 4 ][ 4 ] )
{
	memcpy( mat, src, 4 * 4 * sizeof( float ) );
}

ID_INLINE const idVec4& idMat4::operator[]( int index ) const
{
	//assert( ( index >= 0 ) && ( index < 4 ) );
	return mat[ index ];
}

ID_INLINE idVec4& idMat4::operator[]( int index )
{
	//assert( ( index >= 0 ) && ( index < 4 ) );
	return mat[ index ];
}

ID_INLINE idMat4 idMat4::operator*( const float a ) const
{
	return idMat4(
			   mat[0].x * a, mat[0].y * a, mat[0].z * a, mat[0].w * a,
			   mat[1].x * a, mat[1].y * a, mat[1].z * a, mat[1].w * a,
			   mat[2].x * a, mat[2].y * a, mat[2].z * a, mat[2].w * a,
			   mat[3].x * a, mat[3].y * a, mat[3].z * a, mat[3].w * a );
}

ID_INLINE idVec4 idMat4::operator*( const idVec4& vec ) const
{
	return idVec4(
			   mat[ 0 ].x * vec.x + mat[ 0 ].y * vec.y + mat[ 0 ].z * vec.z + mat[ 0 ].w * vec.w,
			   mat[ 1 ].x * vec.x + mat[ 1 ].y * vec.y + mat[ 1 ].z * vec.z + mat[ 1 ].w * vec.w,
			   mat[ 2 ].x * vec.x + mat[ 2 ].y * vec.y + mat[ 2 ].z * vec.z + mat[ 2 ].w * vec.w,
			   mat[ 3 ].x * vec.x + mat[ 3 ].y * vec.y + mat[ 3 ].z * vec.z + mat[ 3 ].w * vec.w );
}

ID_INLINE idVec3 idMat4::operator*( const idVec3& vec ) const
{
	float s = mat[ 3 ].x * vec.x + mat[ 3 ].y * vec.y + mat[ 3 ].z * vec.z + mat[ 3 ].w;
	if( s == 0.0f )
	{
		return idVec3( 0.0f, 0.0f, 0.0f );
	}
	if( s == 1.0f )
	{
		return idVec3(
				   mat[ 0 ].x * vec.x + mat[ 0 ].y * vec.y + mat[ 0 ].z * vec.z + mat[ 0 ].w,
				   mat[ 1 ].x * vec.x + mat[ 1 ].y * vec.y + mat[ 1 ].z * vec.z + mat[ 1 ].w,
				   mat[ 2 ].x * vec.x + mat[ 2 ].y * vec.y + mat[ 2 ].z * vec.z + mat[ 2 ].w );
	}
	else
	{
		float invS = 1.0f / s;
		return idVec3(
				   ( mat[ 0 ].x * vec.x + mat[ 0 ].y * vec.y + mat[ 0 ].z * vec.z + mat[ 0 ].w ) * invS,
				   ( mat[ 1 ].x * vec.x + mat[ 1 ].y * vec.y + mat[ 1 ].z * vec.z + mat[ 1 ].w ) * invS,
				   ( mat[ 2 ].x * vec.x + mat[ 2 ].y * vec.y + mat[ 2 ].z * vec.z + mat[ 2 ].w ) * invS );
	}
}

ID_INLINE idMat4 idMat4::operator*( const idMat4& a ) const
{
	int i, j;
	const float* m1Ptr, *m2Ptr;
	float* dstPtr;
	idMat4 dst;

	m1Ptr = reinterpret_cast<const float*>( this );
	m2Ptr = reinterpret_cast<const float*>( &a );
	dstPtr = reinterpret_cast<float*>( &dst );

	for( i = 0; i < 4; i++ )
	{
		for( j = 0; j < 4; j++ )
		{
			*dstPtr = m1Ptr[0] * m2Ptr[ 0 * 4 + j ]
					  + m1Ptr[1] * m2Ptr[ 1 * 4 + j ]
					  + m1Ptr[2] * m2Ptr[ 2 * 4 + j ]
					  + m1Ptr[3] * m2Ptr[ 3 * 4 + j ];
			dstPtr++;
		}
		m1Ptr += 4;
	}
	return dst;
}

ID_INLINE idMat4 idMat4::operator+( const idMat4& a ) const
{
	return idMat4(
			   mat[0].x + a[0].x, mat[0].y + a[0].y, mat[0].z + a[0].z, mat[0].w + a[0].w,
			   mat[1].x + a[1].x, mat[1].y + a[1].y, mat[1].z + a[1].z, mat[1].w + a[1].w,
			   mat[2].x + a[2].x, mat[2].y + a[2].y, mat[2].z + a[2].z, mat[2].w + a[2].w,
			   mat[3].x + a[3].x, mat[3].y + a[3].y, mat[3].z + a[3].z, mat[3].w + a[3].w );
}

ID_INLINE idMat4 idMat4::operator-( const idMat4& a ) const
{
	return idMat4(
			   mat[0].x - a[0].x, mat[0].y - a[0].y, mat[0].z - a[0].z, mat[0].w - a[0].w,
			   mat[1].x - a[1].x, mat[1].y - a[1].y, mat[1].z - a[1].z, mat[1].w - a[1].w,
			   mat[2].x - a[2].x, mat[2].y - a[2].y, mat[2].z - a[2].z, mat[2].w - a[2].w,
			   mat[3].x - a[3].x, mat[3].y - a[3].y, mat[3].z - a[3].z, mat[3].w - a[3].w );
}

ID_INLINE idMat4& idMat4::operator*=( const float a )
{
	mat[0].x *= a;
	mat[0].y *= a;
	mat[0].z *= a;
	mat[0].w *= a;
	mat[1].x *= a;
	mat[1].y *= a;
	mat[1].z *= a;
	mat[1].w *= a;
	mat[2].x *= a;
	mat[2].y *= a;
	mat[2].z *= a;
	mat[2].w *= a;
	mat[3].x *= a;
	mat[3].y *= a;
	mat[3].z *= a;
	mat[3].w *= a;
	return *this;
}

ID_INLINE idMat4& idMat4::operator*=( const idMat4& a )
{
	*this = ( *this ) * a;
	return *this;
}

ID_INLINE idMat4& idMat4::operator+=( const idMat4& a )
{
	mat[0].x += a[0].x;
	mat[0].y += a[0].y;
	mat[0].z += a[0].z;
	mat[0].w += a[0].w;
	mat[1].x += a[1].x;
	mat[1].y += a[1].y;
	mat[1].z += a[1].z;
	mat[1].w += a[1].w;
	mat[2].x += a[2].x;
	mat[2].y += a[2].y;
	mat[2].z += a[2].z;
	mat[2].w += a[2].w;
	mat[3].x += a[3].x;
	mat[3].y += a[3].y;
	mat[3].z += a[3].z;
	mat[3].w += a[3].w;
	return *this;
}

ID_INLINE idMat4& idMat4::operator-=( const idMat4& a )
{
	mat[0].x -= a[0].x;
	mat[0].y -= a[0].y;
	mat[0].z -= a[0].z;
	mat[0].w -= a[0].w;
	mat[1].x -= a[1].x;
	mat[1].y -= a[1].y;
	mat[1].z -= a[1].z;
	mat[1].w -= a[1].w;
	mat[2].x -= a[2].x;
	mat[2].y -= a[2].y;
	mat[2].z -= a[2].z;
	mat[2].w -= a[2].w;
	mat[3].x -= a[3].x;
	mat[3].y -= a[3].y;
	mat[3].z -= a[3].z;
	mat[3].w -= a[3].w;
	return *this;
}

ID_INLINE idMat4 operator*( const float a, const idMat4& mat )
{
	return mat * a;
}

ID_INLINE idVec4 operator*( const idVec4& vec, const idMat4& mat )
{
	return mat * vec;
}

ID_INLINE idVec3 operator*( const idVec3& vec, const idMat4& mat )
{
	return mat * vec;
}

ID_INLINE idVec4& operator*=( idVec4& vec, const idMat4& mat )
{
	vec = mat * vec;
	return vec;
}

ID_INLINE idVec3& operator*=( idVec3& vec, const idMat4& mat )
{
	vec = mat * vec;
	return vec;
}

ID_INLINE bool idMat4::Compare( const idMat4& a ) const
{
	dword i;
	const float* ptr1, *ptr2;

	ptr1 = reinterpret_cast<const float*>( mat );
	ptr2 = reinterpret_cast<const float*>( a.mat );
	for( i = 0; i < 4 * 4; i++ )
	{
		if( ptr1[i] != ptr2[i] )
		{
			return false;
		}
	}
	return true;
}

ID_INLINE bool idMat4::Compare( const idMat4& a, const float epsilon ) const
{
	dword i;
	const float* ptr1, *ptr2;

	ptr1 = reinterpret_cast<const float*>( mat );
	ptr2 = reinterpret_cast<const float*>( a.mat );
	for( i = 0; i < 4 * 4; i++ )
	{
		if( idMath::Fabs( ptr1[i] - ptr2[i] ) > epsilon )
		{
			return false;
		}
	}
	return true;
}

ID_INLINE bool idMat4::operator==( const idMat4& a ) const
{
	return Compare( a );
}

ID_INLINE bool idMat4::operator!=( const idMat4& a ) const
{
	return !Compare( a );
}

ID_INLINE void idMat4::Zero()
{
	memset( mat, 0, sizeof( idMat4 ) );
}

ID_INLINE void idMat4::Identity()
{
	*this = mat4_identity;
}

ID_INLINE bool idMat4::IsIdentity( const float epsilon ) const
{
	return Compare( mat4_identity, epsilon );
}

ID_INLINE bool idMat4::IsSymmetric( const float epsilon ) const
{
	for( int i = 1; i < 4; i++ )
	{
		for( int j = 0; j < i; j++ )
		{
			if( idMath::Fabs( mat[i][j] - mat[j][i] ) > epsilon )
			{
				return false;
			}
		}
	}
	return true;
}

ID_INLINE bool idMat4::IsDiagonal( const float epsilon ) const
{
	for( int i = 0; i < 4; i++ )
	{
		for( int j = 0; j < 4; j++ )
		{
			if( i != j && idMath::Fabs( mat[i][j] ) > epsilon )
			{
				return false;
			}
		}
	}
	return true;
}

ID_INLINE bool idMat4::IsRotated() const
{
	if( !mat[ 0 ][ 1 ] && !mat[ 0 ][ 2 ] &&
			!mat[ 1 ][ 0 ] && !mat[ 1 ][ 2 ] &&
			!mat[ 2 ][ 0 ] && !mat[ 2 ][ 1 ] )
	{
		return false;
	}
	return true;
}

ID_INLINE void idMat4::ProjectVector( const idVec4& src, idVec4& dst ) const
{
	dst.x = src * mat[ 0 ];
	dst.y = src * mat[ 1 ];
	dst.z = src * mat[ 2 ];
	dst.w = src * mat[ 3 ];
}

ID_INLINE void idMat4::UnprojectVector( const idVec4& src, idVec4& dst ) const
{
	dst = mat[ 0 ] * src.x + mat[ 1 ] * src.y + mat[ 2 ] * src.z + mat[ 3 ] * src.w;
}

ID_INLINE float idMat4::Trace() const
{
	return ( mat[0][0] + mat[1][1] + mat[2][2] + mat[3][3] );
}

ID_INLINE idMat4 idMat4::Inverse() const
{
	idMat4 invMat;

	invMat = *this;
	verify( invMat.InverseSelf() );
	return invMat;
}

ID_INLINE idMat4 idMat4::InverseFast() const
{
	idMat4 invMat;

	invMat = *this;
	verify( invMat.InverseFastSelf() );
	return invMat;
}

ID_INLINE idMat4 idMat3::ToMat4() const
{
	// NOTE: idMat3 is transposed because it is column-major
	return idMat4(	mat[0][0],	mat[1][0],	mat[2][0],	0.0f,
					mat[0][1],	mat[1][1],	mat[2][1],	0.0f,
					mat[0][2],	mat[1][2],	mat[2][2],	0.0f,
					0.0f,		0.0f,		0.0f,		1.0f );
}

ID_INLINE int idMat4::GetDimension() const
{
	return 16;
}

ID_INLINE const float* idMat4::ToFloatPtr() const
{
	return mat[0].ToFloatPtr();
}

ID_INLINE float* idMat4::ToFloatPtr()
{
	return mat[0].ToFloatPtr();
}


//===============================================================
//
//	idMat5 - 5x5 matrix
//
//===============================================================

class idMat5
{
public:
	idMat5();
	explicit idMat5( const idVec5& v0, const idVec5& v1, const idVec5& v2, const idVec5& v3, const idVec5& v4 );
	explicit idMat5( const float src[ 5 ][ 5 ] );

	const idVec5& 	operator[]( int index ) const;
	idVec5& 		operator[]( int index );
	idMat5			operator*( const float a ) const;
	idVec5			operator*( const idVec5& vec ) const;
	idMat5			operator*( const idMat5& a ) const;
	idMat5			operator+( const idMat5& a ) const;
	idMat5			operator-( const idMat5& a ) const;
	idMat5& 		operator*=( const float a );
	idMat5& 		operator*=( const idMat5& a );
	idMat5& 		operator+=( const idMat5& a );
	idMat5& 		operator-=( const idMat5& a );

	friend idMat5	operator*( const float a, const idMat5& mat );
	friend idVec5	operator*( const idVec5& vec, const idMat5& mat );
	friend idVec5& 	operator*=( idVec5& vec, const idMat5& mat );

	bool			Compare( const idMat5& a ) const;						// exact compare, no epsilon
	bool			Compare( const idMat5& a, const float epsilon ) const;	// compare with epsilon
	bool			operator==( const idMat5& a ) const;					// exact compare, no epsilon
	bool			operator!=( const idMat5& a ) const;					// exact compare, no epsilon

	void			Zero();
	void			Identity();
	bool			IsIdentity( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsSymmetric( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsDiagonal( const float epsilon = MATRIX_EPSILON ) const;

	float			Trace() const;
	float			Determinant() const;
	idMat5			Transpose() const;	// returns transpose
	idMat5& 		TransposeSelf();
	idMat5			Inverse() const;		// returns the inverse ( m * m.Inverse() = identity )
	bool			InverseSelf();		// returns false if determinant is zero
	idMat5			InverseFast() const;	// returns the inverse ( m * m.Inverse() = identity )
	bool			InverseFastSelf();	// returns false if determinant is zero

	int				GetDimension() const;

	const float* 	ToFloatPtr() const;
	float* 			ToFloatPtr();
	const char* 	ToString( int precision = 2 ) const;

private:
	idVec5			mat[ 5 ];
};

extern idMat5 mat5_zero;
extern idMat5 mat5_identity;
#define mat5_default	mat5_identity

ID_INLINE idMat5::idMat5()
{
}

ID_INLINE idMat5::idMat5( const float src[ 5 ][ 5 ] )
{
	memcpy( mat, src, 5 * 5 * sizeof( float ) );
}

ID_INLINE idMat5::idMat5( const idVec5& v0, const idVec5& v1, const idVec5& v2, const idVec5& v3, const idVec5& v4 )
{
	mat[0] = v0;
	mat[1] = v1;
	mat[2] = v2;
	mat[3] = v3;
	mat[4] = v4;
}

ID_INLINE const idVec5& idMat5::operator[]( int index ) const
{
	//assert( ( index >= 0 ) && ( index < 5 ) );
	return mat[ index ];
}

ID_INLINE idVec5& idMat5::operator[]( int index )
{
	//assert( ( index >= 0 ) && ( index < 5 ) );
	return mat[ index ];
}

ID_INLINE idMat5 idMat5::operator*( const idMat5& a ) const
{
	int i, j;
	const float* m1Ptr, *m2Ptr;
	float* dstPtr;
	idMat5 dst;

	m1Ptr = reinterpret_cast<const float*>( this );
	m2Ptr = reinterpret_cast<const float*>( &a );
	dstPtr = reinterpret_cast<float*>( &dst );

	for( i = 0; i < 5; i++ )
	{
		for( j = 0; j < 5; j++ )
		{
			*dstPtr = m1Ptr[0] * m2Ptr[ 0 * 5 + j ]
					  + m1Ptr[1] * m2Ptr[ 1 * 5 + j ]
					  + m1Ptr[2] * m2Ptr[ 2 * 5 + j ]
					  + m1Ptr[3] * m2Ptr[ 3 * 5 + j ]
					  + m1Ptr[4] * m2Ptr[ 4 * 5 + j ];
			dstPtr++;
		}
		m1Ptr += 5;
	}
	return dst;
}

ID_INLINE idMat5 idMat5::operator*( const float a ) const
{
	return idMat5(
			   idVec5( mat[0][0] * a, mat[0][1] * a, mat[0][2] * a, mat[0][3] * a, mat[0][4] * a ),
			   idVec5( mat[1][0] * a, mat[1][1] * a, mat[1][2] * a, mat[1][3] * a, mat[1][4] * a ),
			   idVec5( mat[2][0] * a, mat[2][1] * a, mat[2][2] * a, mat[2][3] * a, mat[2][4] * a ),
			   idVec5( mat[3][0] * a, mat[3][1] * a, mat[3][2] * a, mat[3][3] * a, mat[3][4] * a ),
			   idVec5( mat[4][0] * a, mat[4][1] * a, mat[4][2] * a, mat[4][3] * a, mat[4][4] * a ) );
}

ID_INLINE idVec5 idMat5::operator*( const idVec5& vec ) const
{
	return idVec5(
			   mat[0][0] * vec[0] + mat[0][1] * vec[1] + mat[0][2] * vec[2] + mat[0][3] * vec[3] + mat[0][4] * vec[4],
			   mat[1][0] * vec[0] + mat[1][1] * vec[1] + mat[1][2] * vec[2] + mat[1][3] * vec[3] + mat[1][4] * vec[4],
			   mat[2][0] * vec[0] + mat[2][1] * vec[1] + mat[2][2] * vec[2] + mat[2][3] * vec[3] + mat[2][4] * vec[4],
			   mat[3][0] * vec[0] + mat[3][1] * vec[1] + mat[3][2] * vec[2] + mat[3][3] * vec[3] + mat[3][4] * vec[4],
			   mat[4][0] * vec[0] + mat[4][1] * vec[1] + mat[4][2] * vec[2] + mat[4][3] * vec[3] + mat[4][4] * vec[4] );
}

ID_INLINE idMat5 idMat5::operator+( const idMat5& a ) const
{
	return idMat5(
			   idVec5( mat[0][0] + a[0][0], mat[0][1] + a[0][1], mat[0][2] + a[0][2], mat[0][3] + a[0][3], mat[0][4] + a[0][4] ),
			   idVec5( mat[1][0] + a[1][0], mat[1][1] + a[1][1], mat[1][2] + a[1][2], mat[1][3] + a[1][3], mat[1][4] + a[1][4] ),
			   idVec5( mat[2][0] + a[2][0], mat[2][1] + a[2][1], mat[2][2] + a[2][2], mat[2][3] + a[2][3], mat[2][4] + a[2][4] ),
			   idVec5( mat[3][0] + a[3][0], mat[3][1] + a[3][1], mat[3][2] + a[3][2], mat[3][3] + a[3][3], mat[3][4] + a[3][4] ),
			   idVec5( mat[4][0] + a[4][0], mat[4][1] + a[4][1], mat[4][2] + a[4][2], mat[4][3] + a[4][3], mat[4][4] + a[4][4] ) );
}

ID_INLINE idMat5 idMat5::operator-( const idMat5& a ) const
{
	return idMat5(
			   idVec5( mat[0][0] - a[0][0], mat[0][1] - a[0][1], mat[0][2] - a[0][2], mat[0][3] - a[0][3], mat[0][4] - a[0][4] ),
			   idVec5( mat[1][0] - a[1][0], mat[1][1] - a[1][1], mat[1][2] - a[1][2], mat[1][3] - a[1][3], mat[1][4] - a[1][4] ),
			   idVec5( mat[2][0] - a[2][0], mat[2][1] - a[2][1], mat[2][2] - a[2][2], mat[2][3] - a[2][3], mat[2][4] - a[2][4] ),
			   idVec5( mat[3][0] - a[3][0], mat[3][1] - a[3][1], mat[3][2] - a[3][2], mat[3][3] - a[3][3], mat[3][4] - a[3][4] ),
			   idVec5( mat[4][0] - a[4][0], mat[4][1] - a[4][1], mat[4][2] - a[4][2], mat[4][3] - a[4][3], mat[4][4] - a[4][4] ) );
}

ID_INLINE idMat5& idMat5::operator*=( const float a )
{
	mat[0][0] *= a;
	mat[0][1] *= a;
	mat[0][2] *= a;
	mat[0][3] *= a;
	mat[0][4] *= a;
	mat[1][0] *= a;
	mat[1][1] *= a;
	mat[1][2] *= a;
	mat[1][3] *= a;
	mat[1][4] *= a;
	mat[2][0] *= a;
	mat[2][1] *= a;
	mat[2][2] *= a;
	mat[2][3] *= a;
	mat[2][4] *= a;
	mat[3][0] *= a;
	mat[3][1] *= a;
	mat[3][2] *= a;
	mat[3][3] *= a;
	mat[3][4] *= a;
	mat[4][0] *= a;
	mat[4][1] *= a;
	mat[4][2] *= a;
	mat[4][3] *= a;
	mat[4][4] *= a;
	return *this;
}

ID_INLINE idMat5& idMat5::operator*=( const idMat5& a )
{
	*this = *this * a;
	return *this;
}

ID_INLINE idMat5& idMat5::operator+=( const idMat5& a )
{
	mat[0][0] += a[0][0];
	mat[0][1] += a[0][1];
	mat[0][2] += a[0][2];
	mat[0][3] += a[0][3];
	mat[0][4] += a[0][4];
	mat[1][0] += a[1][0];
	mat[1][1] += a[1][1];
	mat[1][2] += a[1][2];
	mat[1][3] += a[1][3];
	mat[1][4] += a[1][4];
	mat[2][0] += a[2][0];
	mat[2][1] += a[2][1];
	mat[2][2] += a[2][2];
	mat[2][3] += a[2][3];
	mat[2][4] += a[2][4];
	mat[3][0] += a[3][0];
	mat[3][1] += a[3][1];
	mat[3][2] += a[3][2];
	mat[3][3] += a[3][3];
	mat[3][4] += a[3][4];
	mat[4][0] += a[4][0];
	mat[4][1] += a[4][1];
	mat[4][2] += a[4][2];
	mat[4][3] += a[4][3];
	mat[4][4] += a[4][4];
	return *this;
}

ID_INLINE idMat5& idMat5::operator-=( const idMat5& a )
{
	mat[0][0] -= a[0][0];
	mat[0][1] -= a[0][1];
	mat[0][2] -= a[0][2];
	mat[0][3] -= a[0][3];
	mat[0][4] -= a[0][4];
	mat[1][0] -= a[1][0];
	mat[1][1] -= a[1][1];
	mat[1][2] -= a[1][2];
	mat[1][3] -= a[1][3];
	mat[1][4] -= a[1][4];
	mat[2][0] -= a[2][0];
	mat[2][1] -= a[2][1];
	mat[2][2] -= a[2][2];
	mat[2][3] -= a[2][3];
	mat[2][4] -= a[2][4];
	mat[3][0] -= a[3][0];
	mat[3][1] -= a[3][1];
	mat[3][2] -= a[3][2];
	mat[3][3] -= a[3][3];
	mat[3][4] -= a[3][4];
	mat[4][0] -= a[4][0];
	mat[4][1] -= a[4][1];
	mat[4][2] -= a[4][2];
	mat[4][3] -= a[4][3];
	mat[4][4] -= a[4][4];
	return *this;
}

ID_INLINE idVec5 operator*( const idVec5& vec, const idMat5& mat )
{
	return mat * vec;
}

ID_INLINE idMat5 operator*( const float a, idMat5 const& mat )
{
	return mat * a;
}

ID_INLINE idVec5& operator*=( idVec5& vec, const idMat5& mat )
{
	vec = mat * vec;
	return vec;
}

ID_INLINE bool idMat5::Compare( const idMat5& a ) const
{
	dword i;
	const float* ptr1, *ptr2;

	ptr1 = reinterpret_cast<const float*>( mat );
	ptr2 = reinterpret_cast<const float*>( a.mat );
	for( i = 0; i < 5 * 5; i++ )
	{
		if( ptr1[i] != ptr2[i] )
		{
			return false;
		}
	}
	return true;
}

ID_INLINE bool idMat5::Compare( const idMat5& a, const float epsilon ) const
{
	dword i;
	const float* ptr1, *ptr2;

	ptr1 = reinterpret_cast<const float*>( mat );
	ptr2 = reinterpret_cast<const float*>( a.mat );
	for( i = 0; i < 5 * 5; i++ )
	{
		if( idMath::Fabs( ptr1[i] - ptr2[i] ) > epsilon )
		{
			return false;
		}
	}
	return true;
}

ID_INLINE bool idMat5::operator==( const idMat5& a ) const
{
	return Compare( a );
}

ID_INLINE bool idMat5::operator!=( const idMat5& a ) const
{
	return !Compare( a );
}

ID_INLINE void idMat5::Zero()
{
	memset( mat, 0, sizeof( idMat5 ) );
}

ID_INLINE void idMat5::Identity()
{
	*this = mat5_identity;
}

ID_INLINE bool idMat5::IsIdentity( const float epsilon ) const
{
	return Compare( mat5_identity, epsilon );
}

ID_INLINE bool idMat5::IsSymmetric( const float epsilon ) const
{
	for( int i = 1; i < 5; i++ )
	{
		for( int j = 0; j < i; j++ )
		{
			if( idMath::Fabs( mat[i][j] - mat[j][i] ) > epsilon )
			{
				return false;
			}
		}
	}
	return true;
}

ID_INLINE bool idMat5::IsDiagonal( const float epsilon ) const
{
	for( int i = 0; i < 5; i++ )
	{
		for( int j = 0; j < 5; j++ )
		{
			if( i != j && idMath::Fabs( mat[i][j] ) > epsilon )
			{
				return false;
			}
		}
	}
	return true;
}

ID_INLINE float idMat5::Trace() const
{
	return ( mat[0][0] + mat[1][1] + mat[2][2] + mat[3][3] + mat[4][4] );
}

ID_INLINE idMat5 idMat5::Inverse() const
{
	idMat5 invMat;

	invMat = *this;
	verify( invMat.InverseSelf() );
	return invMat;
}

ID_INLINE idMat5 idMat5::InverseFast() const
{
	idMat5 invMat;

	invMat = *this;
	verify( invMat.InverseFastSelf() );
	return invMat;
}

ID_INLINE int idMat5::GetDimension() const
{
	return 25;
}

ID_INLINE const float* idMat5::ToFloatPtr() const
{
	return mat[0].ToFloatPtr();
}

ID_INLINE float* idMat5::ToFloatPtr()
{
	return mat[0].ToFloatPtr();
}


//===============================================================
//
//	idMat6 - 6x6 matrix
//
//===============================================================

class idMat6
{
public:
	idMat6();
	explicit idMat6( const idVec6& v0, const idVec6& v1, const idVec6& v2, const idVec6& v3, const idVec6& v4, const idVec6& v5 );
	explicit idMat6( const idMat3& m0, const idMat3& m1, const idMat3& m2, const idMat3& m3 );
	explicit idMat6( const float src[ 6 ][ 6 ] );

	const idVec6& 	operator[]( int index ) const;
	idVec6& 		operator[]( int index );
	idMat6			operator*( const float a ) const;
	idVec6			operator*( const idVec6& vec ) const;
	idMat6			operator*( const idMat6& a ) const;
	idMat6			operator+( const idMat6& a ) const;
	idMat6			operator-( const idMat6& a ) const;
	idMat6& 		operator*=( const float a );
	idMat6& 		operator*=( const idMat6& a );
	idMat6& 		operator+=( const idMat6& a );
	idMat6& 		operator-=( const idMat6& a );

	friend idMat6	operator*( const float a, const idMat6& mat );
	friend idVec6	operator*( const idVec6& vec, const idMat6& mat );
	friend idVec6& 	operator*=( idVec6& vec, const idMat6& mat );

	bool			Compare( const idMat6& a ) const;						// exact compare, no epsilon
	bool			Compare( const idMat6& a, const float epsilon ) const;	// compare with epsilon
	bool			operator==( const idMat6& a ) const;					// exact compare, no epsilon
	bool			operator!=( const idMat6& a ) const;					// exact compare, no epsilon

	void			Zero();
	void			Identity();
	bool			IsIdentity( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsSymmetric( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsDiagonal( const float epsilon = MATRIX_EPSILON ) const;

	idMat3			SubMat3( int n ) const;
	float			Trace() const;
	float			Determinant() const;
	idMat6			Transpose() const;	// returns transpose
	idMat6& 		TransposeSelf();
	idMat6			Inverse() const;		// returns the inverse ( m * m.Inverse() = identity )
	bool			InverseSelf();		// returns false if determinant is zero
	idMat6			InverseFast() const;	// returns the inverse ( m * m.Inverse() = identity )
	bool			InverseFastSelf();	// returns false if determinant is zero

	int				GetDimension() const;

	const float* 	ToFloatPtr() const;
	float* 			ToFloatPtr();
	const char* 	ToString( int precision = 2 ) const;

private:
	idVec6			mat[ 6 ];
};

extern idMat6 mat6_zero;
extern idMat6 mat6_identity;
#define mat6_default	mat6_identity

ID_INLINE idMat6::idMat6()
{
}

ID_INLINE idMat6::idMat6( const idMat3& m0, const idMat3& m1, const idMat3& m2, const idMat3& m3 )
{
	mat[0] = idVec6( m0[0][0], m0[0][1], m0[0][2], m1[0][0], m1[0][1], m1[0][2] );
	mat[1] = idVec6( m0[1][0], m0[1][1], m0[1][2], m1[1][0], m1[1][1], m1[1][2] );
	mat[2] = idVec6( m0[2][0], m0[2][1], m0[2][2], m1[2][0], m1[2][1], m1[2][2] );
	mat[3] = idVec6( m2[0][0], m2[0][1], m2[0][2], m3[0][0], m3[0][1], m3[0][2] );
	mat[4] = idVec6( m2[1][0], m2[1][1], m2[1][2], m3[1][0], m3[1][1], m3[1][2] );
	mat[5] = idVec6( m2[2][0], m2[2][1], m2[2][2], m3[2][0], m3[2][1], m3[2][2] );
}

ID_INLINE idMat6::idMat6( const idVec6& v0, const idVec6& v1, const idVec6& v2, const idVec6& v3, const idVec6& v4, const idVec6& v5 )
{
	mat[0] = v0;
	mat[1] = v1;
	mat[2] = v2;
	mat[3] = v3;
	mat[4] = v4;
	mat[5] = v5;
}

ID_INLINE idMat6::idMat6( const float src[ 6 ][ 6 ] )
{
	memcpy( mat, src, 6 * 6 * sizeof( float ) );
}

ID_INLINE const idVec6& idMat6::operator[]( int index ) const
{
	//assert( ( index >= 0 ) && ( index < 6 ) );
	return mat[ index ];
}

ID_INLINE idVec6& idMat6::operator[]( int index )
{
	//assert( ( index >= 0 ) && ( index < 6 ) );
	return mat[ index ];
}

ID_INLINE idMat6 idMat6::operator*( const idMat6& a ) const
{
	int i, j;
	const float* m1Ptr, *m2Ptr;
	float* dstPtr;
	idMat6 dst;

	m1Ptr = reinterpret_cast<const float*>( this );
	m2Ptr = reinterpret_cast<const float*>( &a );
	dstPtr = reinterpret_cast<float*>( &dst );

	for( i = 0; i < 6; i++ )
	{
		for( j = 0; j < 6; j++ )
		{
			*dstPtr = m1Ptr[0] * m2Ptr[ 0 * 6 + j ]
					  + m1Ptr[1] * m2Ptr[ 1 * 6 + j ]
					  + m1Ptr[2] * m2Ptr[ 2 * 6 + j ]
					  + m1Ptr[3] * m2Ptr[ 3 * 6 + j ]
					  + m1Ptr[4] * m2Ptr[ 4 * 6 + j ]
					  + m1Ptr[5] * m2Ptr[ 5 * 6 + j ];
			dstPtr++;
		}
		m1Ptr += 6;
	}
	return dst;
}

ID_INLINE idMat6 idMat6::operator*( const float a ) const
{
	return idMat6(
			   idVec6( mat[0][0] * a, mat[0][1] * a, mat[0][2] * a, mat[0][3] * a, mat[0][4] * a, mat[0][5] * a ),
			   idVec6( mat[1][0] * a, mat[1][1] * a, mat[1][2] * a, mat[1][3] * a, mat[1][4] * a, mat[1][5] * a ),
			   idVec6( mat[2][0] * a, mat[2][1] * a, mat[2][2] * a, mat[2][3] * a, mat[2][4] * a, mat[2][5] * a ),
			   idVec6( mat[3][0] * a, mat[3][1] * a, mat[3][2] * a, mat[3][3] * a, mat[3][4] * a, mat[3][5] * a ),
			   idVec6( mat[4][0] * a, mat[4][1] * a, mat[4][2] * a, mat[4][3] * a, mat[4][4] * a, mat[4][5] * a ),
			   idVec6( mat[5][0] * a, mat[5][1] * a, mat[5][2] * a, mat[5][3] * a, mat[5][4] * a, mat[5][5] * a ) );
}

ID_INLINE idVec6 idMat6::operator*( const idVec6& vec ) const
{
	return idVec6(
			   mat[0][0] * vec[0] + mat[0][1] * vec[1] + mat[0][2] * vec[2] + mat[0][3] * vec[3] + mat[0][4] * vec[4] + mat[0][5] * vec[5],
			   mat[1][0] * vec[0] + mat[1][1] * vec[1] + mat[1][2] * vec[2] + mat[1][3] * vec[3] + mat[1][4] * vec[4] + mat[1][5] * vec[5],
			   mat[2][0] * vec[0] + mat[2][1] * vec[1] + mat[2][2] * vec[2] + mat[2][3] * vec[3] + mat[2][4] * vec[4] + mat[2][5] * vec[5],
			   mat[3][0] * vec[0] + mat[3][1] * vec[1] + mat[3][2] * vec[2] + mat[3][3] * vec[3] + mat[3][4] * vec[4] + mat[3][5] * vec[5],
			   mat[4][0] * vec[0] + mat[4][1] * vec[1] + mat[4][2] * vec[2] + mat[4][3] * vec[3] + mat[4][4] * vec[4] + mat[4][5] * vec[5],
			   mat[5][0] * vec[0] + mat[5][1] * vec[1] + mat[5][2] * vec[2] + mat[5][3] * vec[3] + mat[5][4] * vec[4] + mat[5][5] * vec[5] );
}

ID_INLINE idMat6 idMat6::operator+( const idMat6& a ) const
{
	return idMat6(
			   idVec6( mat[0][0] + a[0][0], mat[0][1] + a[0][1], mat[0][2] + a[0][2], mat[0][3] + a[0][3], mat[0][4] + a[0][4], mat[0][5] + a[0][5] ),
			   idVec6( mat[1][0] + a[1][0], mat[1][1] + a[1][1], mat[1][2] + a[1][2], mat[1][3] + a[1][3], mat[1][4] + a[1][4], mat[1][5] + a[1][5] ),
			   idVec6( mat[2][0] + a[2][0], mat[2][1] + a[2][1], mat[2][2] + a[2][2], mat[2][3] + a[2][3], mat[2][4] + a[2][4], mat[2][5] + a[2][5] ),
			   idVec6( mat[3][0] + a[3][0], mat[3][1] + a[3][1], mat[3][2] + a[3][2], mat[3][3] + a[3][3], mat[3][4] + a[3][4], mat[3][5] + a[3][5] ),
			   idVec6( mat[4][0] + a[4][0], mat[4][1] + a[4][1], mat[4][2] + a[4][2], mat[4][3] + a[4][3], mat[4][4] + a[4][4], mat[4][5] + a[4][5] ),
			   idVec6( mat[5][0] + a[5][0], mat[5][1] + a[5][1], mat[5][2] + a[5][2], mat[5][3] + a[5][3], mat[5][4] + a[5][4], mat[5][5] + a[5][5] ) );
}

ID_INLINE idMat6 idMat6::operator-( const idMat6& a ) const
{
	return idMat6(
			   idVec6( mat[0][0] - a[0][0], mat[0][1] - a[0][1], mat[0][2] - a[0][2], mat[0][3] - a[0][3], mat[0][4] - a[0][4], mat[0][5] - a[0][5] ),
			   idVec6( mat[1][0] - a[1][0], mat[1][1] - a[1][1], mat[1][2] - a[1][2], mat[1][3] - a[1][3], mat[1][4] - a[1][4], mat[1][5] - a[1][5] ),
			   idVec6( mat[2][0] - a[2][0], mat[2][1] - a[2][1], mat[2][2] - a[2][2], mat[2][3] - a[2][3], mat[2][4] - a[2][4], mat[2][5] - a[2][5] ),
			   idVec6( mat[3][0] - a[3][0], mat[3][1] - a[3][1], mat[3][2] - a[3][2], mat[3][3] - a[3][3], mat[3][4] - a[3][4], mat[3][5] - a[3][5] ),
			   idVec6( mat[4][0] - a[4][0], mat[4][1] - a[4][1], mat[4][2] - a[4][2], mat[4][3] - a[4][3], mat[4][4] - a[4][4], mat[4][5] - a[4][5] ),
			   idVec6( mat[5][0] - a[5][0], mat[5][1] - a[5][1], mat[5][2] - a[5][2], mat[5][3] - a[5][3], mat[5][4] - a[5][4], mat[5][5] - a[5][5] ) );
}

ID_INLINE idMat6& idMat6::operator*=( const float a )
{
	mat[0][0] *= a;
	mat[0][1] *= a;
	mat[0][2] *= a;
	mat[0][3] *= a;
	mat[0][4] *= a;
	mat[0][5] *= a;
	mat[1][0] *= a;
	mat[1][1] *= a;
	mat[1][2] *= a;
	mat[1][3] *= a;
	mat[1][4] *= a;
	mat[1][5] *= a;
	mat[2][0] *= a;
	mat[2][1] *= a;
	mat[2][2] *= a;
	mat[2][3] *= a;
	mat[2][4] *= a;
	mat[2][5] *= a;
	mat[3][0] *= a;
	mat[3][1] *= a;
	mat[3][2] *= a;
	mat[3][3] *= a;
	mat[3][4] *= a;
	mat[3][5] *= a;
	mat[4][0] *= a;
	mat[4][1] *= a;
	mat[4][2] *= a;
	mat[4][3] *= a;
	mat[4][4] *= a;
	mat[4][5] *= a;
	mat[5][0] *= a;
	mat[5][1] *= a;
	mat[5][2] *= a;
	mat[5][3] *= a;
	mat[5][4] *= a;
	mat[5][5] *= a;
	return *this;
}

ID_INLINE idMat6& idMat6::operator*=( const idMat6& a )
{
	*this = *this * a;
	return *this;
}

ID_INLINE idMat6& idMat6::operator+=( const idMat6& a )
{
	mat[0][0] += a[0][0];
	mat[0][1] += a[0][1];
	mat[0][2] += a[0][2];
	mat[0][3] += a[0][3];
	mat[0][4] += a[0][4];
	mat[0][5] += a[0][5];
	mat[1][0] += a[1][0];
	mat[1][1] += a[1][1];
	mat[1][2] += a[1][2];
	mat[1][3] += a[1][3];
	mat[1][4] += a[1][4];
	mat[1][5] += a[1][5];
	mat[2][0] += a[2][0];
	mat[2][1] += a[2][1];
	mat[2][2] += a[2][2];
	mat[2][3] += a[2][3];
	mat[2][4] += a[2][4];
	mat[2][5] += a[2][5];
	mat[3][0] += a[3][0];
	mat[3][1] += a[3][1];
	mat[3][2] += a[3][2];
	mat[3][3] += a[3][3];
	mat[3][4] += a[3][4];
	mat[3][5] += a[3][5];
	mat[4][0] += a[4][0];
	mat[4][1] += a[4][1];
	mat[4][2] += a[4][2];
	mat[4][3] += a[4][3];
	mat[4][4] += a[4][4];
	mat[4][5] += a[4][5];
	mat[5][0] += a[5][0];
	mat[5][1] += a[5][1];
	mat[5][2] += a[5][2];
	mat[5][3] += a[5][3];
	mat[5][4] += a[5][4];
	mat[5][5] += a[5][5];
	return *this;
}

ID_INLINE idMat6& idMat6::operator-=( const idMat6& a )
{
	mat[0][0] -= a[0][0];
	mat[0][1] -= a[0][1];
	mat[0][2] -= a[0][2];
	mat[0][3] -= a[0][3];
	mat[0][4] -= a[0][4];
	mat[0][5] -= a[0][5];
	mat[1][0] -= a[1][0];
	mat[1][1] -= a[1][1];
	mat[1][2] -= a[1][2];
	mat[1][3] -= a[1][3];
	mat[1][4] -= a[1][4];
	mat[1][5] -= a[1][5];
	mat[2][0] -= a[2][0];
	mat[2][1] -= a[2][1];
	mat[2][2] -= a[2][2];
	mat[2][3] -= a[2][3];
	mat[2][4] -= a[2][4];
	mat[2][5] -= a[2][5];
	mat[3][0] -= a[3][0];
	mat[3][1] -= a[3][1];
	mat[3][2] -= a[3][2];
	mat[3][3] -= a[3][3];
	mat[3][4] -= a[3][4];
	mat[3][5] -= a[3][5];
	mat[4][0] -= a[4][0];
	mat[4][1] -= a[4][1];
	mat[4][2] -= a[4][2];
	mat[4][3] -= a[4][3];
	mat[4][4] -= a[4][4];
	mat[4][5] -= a[4][5];
	mat[5][0] -= a[5][0];
	mat[5][1] -= a[5][1];
	mat[5][2] -= a[5][2];
	mat[5][3] -= a[5][3];
	mat[5][4] -= a[5][4];
	mat[5][5] -= a[5][5];
	return *this;
}

ID_INLINE idVec6 operator*( const idVec6& vec, const idMat6& mat )
{
	return mat * vec;
}

ID_INLINE idMat6 operator*( const float a, idMat6 const& mat )
{
	return mat * a;
}

ID_INLINE idVec6& operator*=( idVec6& vec, const idMat6& mat )
{
	vec = mat * vec;
	return vec;
}

ID_INLINE bool idMat6::Compare( const idMat6& a ) const
{
	dword i;
	const float* ptr1, *ptr2;

	ptr1 = reinterpret_cast<const float*>( mat );
	ptr2 = reinterpret_cast<const float*>( a.mat );
	for( i = 0; i < 6 * 6; i++ )
	{
		if( ptr1[i] != ptr2[i] )
		{
			return false;
		}
	}
	return true;
}

ID_INLINE bool idMat6::Compare( const idMat6& a, const float epsilon ) const
{
	dword i;
	const float* ptr1, *ptr2;

	ptr1 = reinterpret_cast<const float*>( mat );
	ptr2 = reinterpret_cast<const float*>( a.mat );
	for( i = 0; i < 6 * 6; i++ )
	{
		if( idMath::Fabs( ptr1[i] - ptr2[i] ) > epsilon )
		{
			return false;
		}
	}
	return true;
}

ID_INLINE bool idMat6::operator==( const idMat6& a ) const
{
	return Compare( a );
}

ID_INLINE bool idMat6::operator!=( const idMat6& a ) const
{
	return !Compare( a );
}

ID_INLINE void idMat6::Zero()
{
	memset( mat, 0, sizeof( idMat6 ) );
}

ID_INLINE void idMat6::Identity()
{
	*this = mat6_identity;
}

ID_INLINE bool idMat6::IsIdentity( const float epsilon ) const
{
	return Compare( mat6_identity, epsilon );
}

ID_INLINE bool idMat6::IsSymmetric( const float epsilon ) const
{
	for( int i = 1; i < 6; i++ )
	{
		for( int j = 0; j < i; j++ )
		{
			if( idMath::Fabs( mat[i][j] - mat[j][i] ) > epsilon )
			{
				return false;
			}
		}
	}
	return true;
}

ID_INLINE bool idMat6::IsDiagonal( const float epsilon ) const
{
	for( int i = 0; i < 6; i++ )
	{
		for( int j = 0; j < 6; j++ )
		{
			if( i != j && idMath::Fabs( mat[i][j] ) > epsilon )
			{
				return false;
			}
		}
	}
	return true;
}

ID_INLINE idMat3 idMat6::SubMat3( int n ) const
{
	assert( n >= 0 && n < 4 );
	int b0 = ( ( n & 2 ) >> 1 ) * 3;
	int b1 = ( n & 1 ) * 3;
	return idMat3(
			   mat[b0 + 0][b1 + 0], mat[b0 + 0][b1 + 1], mat[b0 + 0][b1 + 2],
			   mat[b0 + 1][b1 + 0], mat[b0 + 1][b1 + 1], mat[b0 + 1][b1 + 2],
			   mat[b0 + 2][b1 + 0], mat[b0 + 2][b1 + 1], mat[b0 + 2][b1 + 2] );
}

ID_INLINE float idMat6::Trace() const
{
	return ( mat[0][0] + mat[1][1] + mat[2][2] + mat[3][3] + mat[4][4] + mat[5][5] );
}

ID_INLINE idMat6 idMat6::Inverse() const
{
	idMat6 invMat;

	invMat = *this;
	verify( invMat.InverseSelf() );
	return invMat;
}

ID_INLINE idMat6 idMat6::InverseFast() const
{
	idMat6 invMat;

	invMat = *this;
	verify( invMat.InverseFastSelf() );
	return invMat;
}

ID_INLINE int idMat6::GetDimension() const
{
	return 36;
}

ID_INLINE const float* idMat6::ToFloatPtr() const
{
	return mat[0].ToFloatPtr();
}

ID_INLINE float* idMat6::ToFloatPtr()
{
	return mat[0].ToFloatPtr();
}

#endif /* !__MATH_MATRIX_H__ */
