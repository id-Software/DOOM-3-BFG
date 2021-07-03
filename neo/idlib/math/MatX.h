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

#ifndef __MATH_MATX_H__
#define __MATH_MATX_H__

/*
===============================================================================

idMatX - arbitrary sized dense real matrix

The matrix lives on 16 byte aligned and 16 byte padded memory.

NOTE: due to the temporary memory pool idMatX cannot be used by multiple threads.

===============================================================================
*/

#define MATX_MAX_TEMP		1024
#define MATX_QUAD( x )		( ( ( ( x ) + 3 ) & ~3 ) * sizeof( float ) )
#define MATX_CLEAREND()		int s = numRows * numColumns; while( s < ( ( s + 3 ) & ~3 ) ) { mat[s++] = 0.0f; }
#define MATX_ALLOCA( n )	( (float *) _alloca16( MATX_QUAD( n ) ) )
#define MATX_ALLOCA_CACHE_LINES( n )	( (float *) _alloca128( ( ( n ) * sizeof( float ) + CACHE_LINE_SIZE - 1 ) & ~ ( CACHE_LINE_SIZE - 1 ) ) )

#if defined(USE_INTRINSICS_SSE)
	#define MATX_SIMD
#endif

class idMatX
{
public:
	ID_INLINE					idMatX();
	ID_INLINE					idMatX( const idMatX& other );
	ID_INLINE					explicit idMatX( int rows, int columns );
	ID_INLINE					explicit idMatX( int rows, int columns, float* src );
	ID_INLINE					~idMatX();

	ID_INLINE	void			Set( int rows, int columns, const float* src );
	ID_INLINE	void			Set( const idMat3& m1, const idMat3& m2 );
	ID_INLINE	void			Set( const idMat3& m1, const idMat3& m2, const idMat3& m3, const idMat3& m4 );

	ID_INLINE	const float* 	operator[]( int index ) const;
	ID_INLINE	float* 			operator[]( int index );
	ID_INLINE	idMatX& 		operator=( const idMatX& a );
	ID_INLINE	idMatX			operator*( const float a ) const;
	ID_INLINE	idVecX			operator*( const idVecX& vec ) const;
	ID_INLINE	idMatX			operator*( const idMatX& a ) const;
	ID_INLINE	idMatX			operator+( const idMatX& a ) const;
	ID_INLINE	idMatX			operator-( const idMatX& a ) const;
	ID_INLINE	idMatX& 		operator*=( const float a );
	ID_INLINE	idMatX& 		operator*=( const idMatX& a );
	ID_INLINE	idMatX& 		operator+=( const idMatX& a );
	ID_INLINE	idMatX& 		operator-=( const idMatX& a );

	friend ID_INLINE	idMatX	operator*( const float a, const idMatX& m );
	friend ID_INLINE	idVecX	operator*( const idVecX& vec, const idMatX& m );
	friend ID_INLINE	idVecX& operator*=( idVecX& vec, const idMatX& m );

	ID_INLINE	bool			Compare( const idMatX& a ) const;									// exact compare, no epsilon
	ID_INLINE	bool			Compare( const idMatX& a, const float epsilon ) const;				// compare with epsilon
	ID_INLINE	bool			operator==( const idMatX& a ) const;								// exact compare, no epsilon
	ID_INLINE	bool			operator!=( const idMatX& a ) const;								// exact compare, no epsilon

	ID_INLINE	void			SetSize( int rows, int columns );									// set the number of rows/columns
	void			ChangeSize( int rows, int columns, bool makeZero = false );		// change the size keeping data intact where possible
	ID_INLINE	void			ChangeNumRows( int rows )
	{
		ChangeSize( rows, numColumns );	   // set the number of rows/columns
	}
	int				GetNumRows() const
	{
		return numRows;    // get the number of rows
	}
	int				GetNumColumns() const
	{
		return numColumns;    // get the number of columns
	}
	ID_INLINE	void			SetData( int rows, int columns, float* data );						// set float array pointer
	ID_INLINE	void			SetDataCacheLines( int rows, int columns, float* data, bool clear );// set float array pointer
	ID_INLINE	void			Zero();																// clear matrix
	ID_INLINE	void			Zero( int rows, int columns );										// set size and clear matrix
	ID_INLINE	void			Identity();															// clear to identity matrix
	ID_INLINE	void			Identity( int rows, int columns );									// set size and clear to identity matrix
	ID_INLINE	void			Diag( const idVecX& v );											// create diagonal matrix from vector
	ID_INLINE	void			Random( int seed, float l = 0.0f, float u = 1.0f );					// fill matrix with random values
	ID_INLINE	void			Random( int rows, int columns, int seed, float l = 0.0f, float u = 1.0f );
	ID_INLINE	void			Negate();															// (*this) = - (*this)
	ID_INLINE	void			Clamp( float min, float max );										// clamp all values
	ID_INLINE	idMatX& 		SwapRows( int r1, int r2 );											// swap rows
	ID_INLINE	idMatX& 		SwapColumns( int r1, int r2 );										// swap columns
	ID_INLINE	idMatX& 		SwapRowsColumns( int r1, int r2 );									// swap rows and columns
	idMatX& 		RemoveRow( int r );												// remove a row
	idMatX& 		RemoveColumn( int r );											// remove a column
	idMatX& 		RemoveRowColumn( int r );										// remove a row and column
	ID_INLINE	void			ClearUpperTriangle();												// clear the upper triangle
	ID_INLINE	void			ClearLowerTriangle();												// clear the lower triangle
	void			CopyLowerToUpperTriangle();											// copy the lower triangle to the upper triangle
	ID_INLINE	void			SquareSubMatrix( const idMatX& m, int size );						// get square sub-matrix from 0,0 to size,size
	ID_INLINE	float			MaxDifference( const idMatX& m ) const;								// return maximum element difference between this and m

	ID_INLINE	bool			IsSquare() const
	{
		return ( numRows == numColumns );
	}
	ID_INLINE	bool			IsZero( const float epsilon = MATRIX_EPSILON ) const;
	ID_INLINE	bool			IsIdentity( const float epsilon = MATRIX_EPSILON ) const;
	ID_INLINE	bool			IsDiagonal( const float epsilon = MATRIX_EPSILON ) const;
	ID_INLINE	bool			IsTriDiagonal( const float epsilon = MATRIX_EPSILON ) const;
	ID_INLINE	bool			IsSymmetric( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsOrthogonal( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsOrthonormal( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsPMatrix( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsZMatrix( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsPositiveDefinite( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsSymmetricPositiveDefinite( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsPositiveSemiDefinite( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsSymmetricPositiveSemiDefinite( const float epsilon = MATRIX_EPSILON ) const;

	ID_INLINE	float			Trace() const;													// returns product of diagonal elements
	ID_INLINE	float			Determinant() const;											// returns determinant of matrix
	ID_INLINE	idMatX			Transpose() const;												// returns transpose
	ID_INLINE	idMatX& 		TransposeSelf();												// transposes the matrix itself
	ID_INLINE	void			Transpose( idMatX& dst ) const;								// stores transpose in 'dst'
	ID_INLINE	idMatX			Inverse() const;												// returns the inverse ( m * m.Inverse() = identity )
	ID_INLINE	bool			InverseSelf();													// returns false if determinant is zero
	ID_INLINE	idMatX			InverseFast() const;											// returns the inverse ( m * m.Inverse() = identity )
	ID_INLINE	bool			InverseFastSelf();												// returns false if determinant is zero
	ID_INLINE	void			Inverse( idMatX& dst ) const;									// stores the inverse in 'dst' ( m * m.Inverse() = identity )

	bool			LowerTriangularInverse();									// in-place inversion, returns false if determinant is zero
	bool			UpperTriangularInverse();									// in-place inversion, returns false if determinant is zero

	ID_INLINE	void			Subtract( const idMatX& a );									// (*this) -= a;

	ID_INLINE	idVecX			Multiply( const idVecX& vec ) const;							// (*this) * vec
	ID_INLINE	idVecX			TransposeMultiply( const idVecX& vec ) const;					// this->Transpose() * vec

	ID_INLINE	idMatX			Multiply( const idMatX& a ) const;								// (*this) * a
	ID_INLINE	idMatX			TransposeMultiply( const idMatX& a ) const;						// this->Transpose() * a

	ID_INLINE	void			Multiply( idVecX& dst, const idVecX& vec ) const;				// dst = (*this) * vec
	ID_INLINE	void			MultiplyAdd( idVecX& dst, const idVecX& vec ) const;			// dst += (*this) * vec
	ID_INLINE	void			MultiplySub( idVecX& dst, const idVecX& vec ) const;			// dst -= (*this) * vec
	ID_INLINE	void			TransposeMultiply( idVecX& dst, const idVecX& vec ) const;		// dst = this->Transpose() * vec
	ID_INLINE	void			TransposeMultiplyAdd( idVecX& dst, const idVecX& vec ) const;	// dst += this->Transpose() * vec
	ID_INLINE	void			TransposeMultiplySub( idVecX& dst, const idVecX& vec ) const;	// dst -= this->Transpose() * vec

	ID_INLINE	void			Multiply( idMatX& dst, const idMatX& a ) const;					// dst = (*this) * a
	ID_INLINE	void			TransposeMultiply( idMatX& dst, const idMatX& a ) const;		// dst = this->Transpose() * a

	ID_INLINE	int				GetDimension() const;											// returns total number of values in matrix

	ID_INLINE	const idVec6& 	SubVec6( int row ) const;										// interpret beginning of row as a const idVec6
	ID_INLINE	idVec6& 		SubVec6( int row );												// interpret beginning of row as an idVec6
	ID_INLINE	const idVecX	SubVecX( int row ) const;										// interpret complete row as a const idVecX
	ID_INLINE	idVecX			SubVecX( int row );												// interpret complete row as an idVecX
	ID_INLINE	const float* 	ToFloatPtr() const;												// pointer to const matrix float array
	ID_INLINE	float* 			ToFloatPtr();													// pointer to matrix float array
	const char* 	ToString( int precision = 2 ) const;

	void			Update_RankOne( const idVecX& v, const idVecX& w, float alpha );
	void			Update_RankOneSymmetric( const idVecX& v, float alpha );
	void			Update_RowColumn( const idVecX& v, const idVecX& w, int r );
	void			Update_RowColumnSymmetric( const idVecX& v, int r );
	void			Update_Increment( const idVecX& v, const idVecX& w );
	void			Update_IncrementSymmetric( const idVecX& v );
	void			Update_Decrement( int r );

	bool			Inverse_GaussJordan();					// invert in-place with Gauss-Jordan elimination
	bool			Inverse_UpdateRankOne( const idVecX& v, const idVecX& w, float alpha );
	bool			Inverse_UpdateRowColumn( const idVecX& v, const idVecX& w, int r );
	bool			Inverse_UpdateIncrement( const idVecX& v, const idVecX& w );
	bool			Inverse_UpdateDecrement( const idVecX& v, const idVecX& w, int r );
	void			Inverse_Solve( idVecX& x, const idVecX& b ) const;

	bool			LU_Factor( int* index, float* det = NULL );		// factor in-place: L * U
	bool			LU_UpdateRankOne( const idVecX& v, const idVecX& w, float alpha, int* index );
	bool			LU_UpdateRowColumn( const idVecX& v, const idVecX& w, int r, int* index );
	bool			LU_UpdateIncrement( const idVecX& v, const idVecX& w, int* index );
	bool			LU_UpdateDecrement( const idVecX& v, const idVecX& w, const idVecX& u, int r, int* index );
	void			LU_Solve( idVecX& x, const idVecX& b, const int* index ) const;
	void			LU_Inverse( idMatX& inv, const int* index ) const;
	void			LU_UnpackFactors( idMatX& L, idMatX& U ) const;
	void			LU_MultiplyFactors( idMatX& m, const int* index ) const;

	bool			QR_Factor( idVecX& c, idVecX& d );				// factor in-place: Q * R
	bool			QR_UpdateRankOne( idMatX& R, const idVecX& v, const idVecX& w, float alpha );
	bool			QR_UpdateRowColumn( idMatX& R, const idVecX& v, const idVecX& w, int r );
	bool			QR_UpdateIncrement( idMatX& R, const idVecX& v, const idVecX& w );
	bool			QR_UpdateDecrement( idMatX& R, const idVecX& v, const idVecX& w, int r );
	void			QR_Solve( idVecX& x, const idVecX& b, const idVecX& c, const idVecX& d ) const;
	void			QR_Solve( idVecX& x, const idVecX& b, const idMatX& R ) const;
	void			QR_Inverse( idMatX& inv, const idVecX& c, const idVecX& d ) const;
	void			QR_UnpackFactors( idMatX& Q, idMatX& R, const idVecX& c, const idVecX& d ) const;
	void			QR_MultiplyFactors( idMatX& m, const idVecX& c, const idVecX& d ) const;

	bool			SVD_Factor( idVecX& w, idMatX& V );				// factor in-place: U * Diag(w) * V.Transpose()
	void			SVD_Solve( idVecX& x, const idVecX& b, const idVecX& w, const idMatX& V ) const;
	void			SVD_Inverse( idMatX& inv, const idVecX& w, const idMatX& V ) const;
	void			SVD_MultiplyFactors( idMatX& m, const idVecX& w, const idMatX& V ) const;

	bool			Cholesky_Factor();						// factor in-place: L * L.Transpose()
	bool			Cholesky_UpdateRankOne( const idVecX& v, float alpha, int offset = 0 );
	bool			Cholesky_UpdateRowColumn( const idVecX& v, int r );
	bool			Cholesky_UpdateIncrement( const idVecX& v );
	bool			Cholesky_UpdateDecrement( const idVecX& v, int r );
	void			Cholesky_Solve( idVecX& x, const idVecX& b ) const;
	void			Cholesky_Inverse( idMatX& inv ) const;
	void			Cholesky_MultiplyFactors( idMatX& m ) const;

	bool			LDLT_Factor();							// factor in-place: L * D * L.Transpose()
	bool			LDLT_UpdateRankOne( const idVecX& v, float alpha, int offset = 0 );
	bool			LDLT_UpdateRowColumn( const idVecX& v, int r );
	bool			LDLT_UpdateIncrement( const idVecX& v );
	bool			LDLT_UpdateDecrement( const idVecX& v, int r );
	void			LDLT_Solve( idVecX& x, const idVecX& b ) const;
	void			LDLT_Inverse( idMatX& inv ) const;
	void			LDLT_UnpackFactors( idMatX& L, idMatX& D ) const;
	void			LDLT_MultiplyFactors( idMatX& m ) const;

	void			TriDiagonal_ClearTriangles();
	bool			TriDiagonal_Solve( idVecX& x, const idVecX& b ) const;
	void			TriDiagonal_Inverse( idMatX& inv ) const;

	bool			Eigen_SolveSymmetricTriDiagonal( idVecX& eigenValues );
	bool			Eigen_SolveSymmetric( idVecX& eigenValues );
	bool			Eigen_Solve( idVecX& realEigenValues, idVecX& imaginaryEigenValues );
	void			Eigen_SortIncreasing( idVecX& eigenValues );
	void			Eigen_SortDecreasing( idVecX& eigenValues );

	static void		Test();

private:
	int				numRows;				// number of rows
	int				numColumns;				// number of columns
	int				alloced;				// floats allocated, if -1 then mat points to data set with SetData
	float* 			mat;					// memory the matrix is stored

	static float	temp[MATX_MAX_TEMP + 4];	// used to store intermediate results
	static float* 	tempPtr;				// pointer to 16 byte aligned temporary memory
	static int		tempIndex;				// index into memory pool, wraps around

private:
	void			SetTempSize( int rows, int columns );
	float			DeterminantGeneric() const;
	bool			InverseSelfGeneric();
	void			QR_Rotate( idMatX& R, int i, float a, float b );
	float			Pythag( float a, float b ) const;
	void			SVD_BiDiag( idVecX& w, idVecX& rv1, float& anorm );
	void			SVD_InitialWV( idVecX& w, idMatX& V, idVecX& rv1 );
	void			HouseholderReduction( idVecX& diag, idVecX& subd );
	bool			QL( idVecX& diag, idVecX& subd );
	void			HessenbergReduction( idMatX& H );
	void			ComplexDivision( float xr, float xi, float yr, float yi, float& cdivr, float& cdivi );
	bool			HessenbergToRealSchur( idMatX& H, idVecX& realEigenValues, idVecX& imaginaryEigenValues );
};

/*
========================
idMatX::idMatX
========================
*/
ID_INLINE idMatX::idMatX()
{
	numRows = numColumns = alloced = 0;
	mat = NULL;
}

/*
========================
idMatX::~idMatX
========================
*/
ID_INLINE idMatX::~idMatX()
{
	// if not temp memory
	if( mat != NULL && ( mat < idMatX::tempPtr || mat > idMatX::tempPtr + MATX_MAX_TEMP ) && alloced != -1 )
	{
		Mem_Free16( mat );
	}
}

/*
========================
idMatX::idMatX
========================
*/
ID_INLINE idMatX::idMatX( int rows, int columns )
{
	numRows = numColumns = alloced = 0;
	mat = NULL;
	SetSize( rows, columns );
}

/*
========================
idMatX::idMatX
========================
*/
ID_INLINE idMatX::idMatX( const idMatX& other )
{
	numRows = numColumns = alloced = 0;
	mat = NULL;
	Set( other.GetNumRows(), other.GetNumColumns(), other.ToFloatPtr() );
}

/*
========================
idMatX::idMatX
========================
*/
ID_INLINE idMatX::idMatX( int rows, int columns, float* src )
{
	numRows = numColumns = alloced = 0;
	mat = NULL;
	SetData( rows, columns, src );
}

/*
========================
idMatX::Set
========================
*/
ID_INLINE void idMatX::Set( int rows, int columns, const float* src )
{
	SetSize( rows, columns );
	memcpy( this->mat, src, rows * columns * sizeof( float ) );
}

/*
========================
idMatX::Set
========================
*/
ID_INLINE void idMatX::Set( const idMat3& m1, const idMat3& m2 )
{
	SetSize( 3, 6 );
	for( int i = 0; i < 3; i++ )
	{
		for( int j = 0; j < 3; j++ )
		{
			mat[( i + 0 ) * numColumns + ( j + 0 )] = m1[i][j];
			mat[( i + 0 ) * numColumns + ( j + 3 )] = m2[i][j];
		}
	}
}

/*
========================
idMatX::Set
========================
*/
ID_INLINE void idMatX::Set( const idMat3& m1, const idMat3& m2, const idMat3& m3, const idMat3& m4 )
{
	SetSize( 6, 6 );
	for( int i = 0; i < 3; i++ )
	{
		for( int j = 0; j < 3; j++ )
		{
			mat[( i + 0 ) * numColumns + ( j + 0 )] = m1[i][j];
			mat[( i + 0 ) * numColumns + ( j + 3 )] = m2[i][j];
			mat[( i + 3 ) * numColumns + ( j + 0 )] = m3[i][j];
			mat[( i + 3 ) * numColumns + ( j + 3 )] = m4[i][j];
		}
	}
}

/*
========================
idMatX::operator[]
========================
*/
ID_INLINE const float* idMatX::operator[]( int index ) const
{
	assert( ( index >= 0 ) && ( index < numRows ) );
	return mat + index * numColumns;
}

/*
========================
idMatX::operator[]
========================
*/
ID_INLINE float* idMatX::operator[]( int index )
{
	assert( ( index >= 0 ) && ( index < numRows ) );
	return mat + index * numColumns;
}

/*
========================
idMatX::operator=
========================
*/
ID_INLINE idMatX& idMatX::operator=( const idMatX& a )
{
	SetSize( a.numRows, a.numColumns );
	int s = a.numRows * a.numColumns;
#ifdef MATX_SIMD
	for( int i = 0; i < s; i += 4 )
	{
		_mm_store_ps( mat + i, _mm_load_ps( a.mat + i ) );
	}
#else
	memcpy( mat, a.mat, s * sizeof( float ) );
#endif
	idMatX::tempIndex = 0;
	return *this;
}

/*
========================
idMatX::operator*
========================
*/
ID_INLINE idMatX idMatX::operator*( const float a ) const
{
	idMatX m;

	m.SetTempSize( numRows, numColumns );
	int s = numRows * numColumns;
#ifdef MATX_SIMD
	__m128 va = _mm_load1_ps( & a );
	for( int i = 0; i < s; i += 4 )
	{
		_mm_store_ps( m.mat + i, _mm_mul_ps( _mm_load_ps( mat + i ), va ) );
	}
#else
	for( int i = 0; i < s; i++ )
	{
		m.mat[i] = mat[i] * a;
	}
#endif
	return m;
}

/*
========================
idMatX::operator*
========================
*/
ID_INLINE idVecX idMatX::operator*( const idVecX& vec ) const
{
	assert( numColumns == vec.GetSize() );

	idVecX dst;
	dst.SetTempSize( numRows );
	Multiply( dst, vec );
	return dst;
}

/*
========================
idMatX::operator*
========================
*/
ID_INLINE idMatX idMatX::operator*( const idMatX& a ) const
{
	assert( numColumns == a.numRows );

	idMatX dst;
	dst.SetTempSize( numRows, a.numColumns );
	Multiply( dst, a );
	return dst;
}

/*
========================
idMatX::operator+
========================
*/
ID_INLINE idMatX idMatX::operator+( const idMatX& a ) const
{
	idMatX m;

	assert( numRows == a.numRows && numColumns == a.numColumns );
	m.SetTempSize( numRows, numColumns );
	int s = numRows * numColumns;
#ifdef MATX_SIMD
	for( int i = 0; i < s; i += 4 )
	{
		_mm_store_ps( m.mat + i, _mm_add_ps( _mm_load_ps( mat + i ), _mm_load_ps( a.mat + i ) ) );
	}
#else
	for( int i = 0; i < s; i++ )
	{
		m.mat[i] = mat[i] + a.mat[i];
	}
#endif
	return m;
}

/*
========================
idMatX::operator-
========================
*/
ID_INLINE idMatX idMatX::operator-( const idMatX& a ) const
{
	idMatX m;

	assert( numRows == a.numRows && numColumns == a.numColumns );
	m.SetTempSize( numRows, numColumns );
	int s = numRows * numColumns;
#ifdef MATX_SIMD
	for( int i = 0; i < s; i += 4 )
	{
		_mm_store_ps( m.mat + i, _mm_sub_ps( _mm_load_ps( mat + i ), _mm_load_ps( a.mat + i ) ) );
	}
#else
	for( int i = 0; i < s; i++ )
	{
		m.mat[i] = mat[i] - a.mat[i];
	}
#endif
	return m;
}

/*
========================
idMatX::operator*=
========================
*/
ID_INLINE idMatX& idMatX::operator*=( const float a )
{
	int s = numRows * numColumns;
#ifdef MATX_SIMD
	__m128 va = _mm_load1_ps( & a );
	for( int i = 0; i < s; i += 4 )
	{
		_mm_store_ps( mat + i, _mm_mul_ps( _mm_load_ps( mat + i ), va ) );
	}
#else
	for( int i = 0; i < s; i++ )
	{
		mat[i] *= a;
	}
#endif
	idMatX::tempIndex = 0;
	return *this;
}

/*
========================
idMatX::operator*=
========================
*/
ID_INLINE idMatX& idMatX::operator*=( const idMatX& a )
{
	*this = *this * a;
	idMatX::tempIndex = 0;
	return *this;
}

/*
========================
idMatX::operator+=
========================
*/
ID_INLINE idMatX& idMatX::operator+=( const idMatX& a )
{
	assert( numRows == a.numRows && numColumns == a.numColumns );
	int s = numRows * numColumns;
#ifdef MATX_SIMD
	for( int i = 0; i < s; i += 4 )
	{
		_mm_store_ps( mat + i, _mm_add_ps( _mm_load_ps( mat + i ), _mm_load_ps( a.mat + i ) ) );
	}
#else
	for( int i = 0; i < s; i++ )
	{
		mat[i] += a.mat[i];
	}
#endif
	idMatX::tempIndex = 0;
	return *this;
}

/*
========================
idMatX::operator-=
========================
*/
ID_INLINE idMatX& idMatX::operator-=( const idMatX& a )
{
	assert( numRows == a.numRows && numColumns == a.numColumns );
	int s = numRows * numColumns;
#ifdef MATX_SIMD
	for( int i = 0; i < s; i += 4 )
	{
		_mm_store_ps( mat + i, _mm_sub_ps( _mm_load_ps( mat + i ), _mm_load_ps( a.mat + i ) ) );
	}
#else
	for( int i = 0; i < s; i++ )
	{
		mat[i] -= a.mat[i];
	}
#endif
	idMatX::tempIndex = 0;
	return *this;
}

/*
========================
operator*
========================
*/
ID_INLINE idMatX operator*( const float a, idMatX const& m )
{
	return m * a;
}

/*
========================
operator*
========================
*/
ID_INLINE idVecX operator*( const idVecX& vec, const idMatX& m )
{
	return m * vec;
}

/*
========================
operator*=
========================
*/
ID_INLINE idVecX& operator*=( idVecX& vec, const idMatX& m )
{
	vec = m * vec;
	return vec;
}

/*
========================
idMatX::Compare
========================
*/
ID_INLINE bool idMatX::Compare( const idMatX& a ) const
{
	assert( numRows == a.numRows && numColumns == a.numColumns );

	int s = numRows * numColumns;
	for( int i = 0; i < s; i++ )
	{
		if( mat[i] != a.mat[i] )
		{
			return false;
		}
	}
	return true;
}

/*
========================
idMatX::Compare
========================
*/
ID_INLINE bool idMatX::Compare( const idMatX& a, const float epsilon ) const
{
	assert( numRows == a.numRows && numColumns == a.numColumns );

	int s = numRows * numColumns;
	for( int i = 0; i < s; i++ )
	{
		if( idMath::Fabs( mat[i] - a.mat[i] ) > epsilon )
		{
			return false;
		}
	}
	return true;
}

/*
========================
idMatX::operator==
========================
*/
ID_INLINE bool idMatX::operator==( const idMatX& a ) const
{
	return Compare( a );
}

/*
========================
idMatX::operator!=
========================
*/
ID_INLINE bool idMatX::operator!=( const idMatX& a ) const
{
	return !Compare( a );
}

/*
========================
idMatX::SetSize
========================
*/
ID_INLINE void idMatX::SetSize( int rows, int columns )
{
	if( rows != numRows || columns != numColumns || mat == NULL )
	{
		assert( mat < idMatX::tempPtr || mat > idMatX::tempPtr + MATX_MAX_TEMP );
		int alloc = ( rows * columns + 3 ) & ~3;
		if( alloc > alloced && alloced != -1 )
		{
			if( mat != NULL )
			{
				Mem_Free16( mat );
			}
			mat = ( float* ) Mem_Alloc16( alloc * sizeof( float ), TAG_MATH );
			alloced = alloc;
		}
		numRows = rows;
		numColumns = columns;
		MATX_CLEAREND();
	}
}

/*
========================
idMatX::SetTempSize
========================
*/
ID_INLINE void idMatX::SetTempSize( int rows, int columns )
{
	int newSize;

	newSize = ( rows * columns + 3 ) & ~3;
	assert( newSize < MATX_MAX_TEMP );
	if( idMatX::tempIndex + newSize > MATX_MAX_TEMP )
	{
		idMatX::tempIndex = 0;
	}
	mat = idMatX::tempPtr + idMatX::tempIndex;
	idMatX::tempIndex += newSize;
	alloced = newSize;
	numRows = rows;
	numColumns = columns;
	MATX_CLEAREND();
}

/*
========================
idMatX::SetData
========================
*/
ID_INLINE void idMatX::SetData( int rows, int columns, float* data )
{
	assert( mat < idMatX::tempPtr || mat > idMatX::tempPtr + MATX_MAX_TEMP );
	if( mat != NULL && alloced != -1 )
	{
		Mem_Free16( mat );
	}
	// RB: changed UINT_PTR to uintptr_t
	assert( ( ( ( uintptr_t ) data ) & 15 ) == 0 ); // data must be 16 byte aligned
	// RB end
	mat = data;
	alloced = -1;
	numRows = rows;
	numColumns = columns;
	MATX_CLEAREND();
}

/*
========================
idMatX::SetDataCacheLines
========================
*/
ID_INLINE void idMatX::SetDataCacheLines( int rows, int columns, float* data, bool clear )
{
	if( mat != NULL && alloced != -1 )
	{
		Mem_Free( mat );
	}

	// RB: changed UINT_PTR to uintptr_t
	assert( ( ( ( uintptr_t ) data ) & 127 ) == 0 ); // data must be 128 byte aligned
	// RB end

	mat = data;
	alloced = -1;
	numRows = rows;
	numColumns = columns;

	if( clear )
	{
		int size = numRows * numColumns * sizeof( float );
		for( int i = 0; i < size; i += CACHE_LINE_SIZE )
		{
			ZeroCacheLine( mat, i );
		}
	}
	else
	{
		MATX_CLEAREND();
	}
}

/*
========================
idMatX::Zero
========================
*/
ID_INLINE void idMatX::Zero()
{
	int s = numRows * numColumns;
#ifdef MATX_SIMD
	for( int i = 0; i < s; i += 4 )
	{
		_mm_store_ps( mat + i, _mm_setzero_ps() );
	}
#else
	memset( mat, 0, numRows * numColumns * sizeof( float ) );
#endif
}

/*
========================
idMatX::Zero
========================
*/
ID_INLINE void idMatX::Zero( int rows, int columns )
{
	SetSize( rows, columns );
	Zero();
}

/*
========================
idMatX::Identity
========================
*/
ID_INLINE void idMatX::Identity()
{
	assert( numRows == numColumns );
	Zero();
	for( int i = 0; i < numRows; i++ )
	{
		mat[i * numColumns + i] = 1.0f;
	}
}

/*
========================
idMatX::Identity
========================
*/
ID_INLINE void idMatX::Identity( int rows, int columns )
{
	assert( rows == columns );
	SetSize( rows, columns );
	idMatX::Identity();
}

/*
========================
idMatX::Diag
========================
*/
ID_INLINE void idMatX::Diag( const idVecX& v )
{
	Zero( v.GetSize(), v.GetSize() );
	for( int i = 0; i < v.GetSize(); i++ )
	{
		mat[i * numColumns + i] = v[i];
	}
}

/*
========================
idMatX::Random
========================
*/
ID_INLINE void idMatX::Random( int seed, float l, float u )
{
	idRandom rnd( seed );

	float c = u - l;
	int s = numRows * numColumns;
	for( int i = 0; i < s; i++ )
	{
		mat[i] = l + rnd.RandomFloat() * c;
	}
}

/*
========================
idMatX::Random
========================
*/
ID_INLINE void idMatX::Random( int rows, int columns, int seed, float l, float u )
{
	idRandom rnd( seed );

	SetSize( rows, columns );
	float c = u - l;
	int s = numRows * numColumns;
	for( int i = 0; i < s; i++ )
	{
		mat[i] = l + rnd.RandomFloat() * c;
	}
}

/*
========================
idMatX::Negate
========================
*/
ID_INLINE void idMatX::Negate()
{
	int s = numRows * numColumns;
#ifdef MATX_SIMD
	ALIGN16( const unsigned int signBit[4] ) = { IEEE_FLT_SIGN_MASK, IEEE_FLT_SIGN_MASK, IEEE_FLT_SIGN_MASK, IEEE_FLT_SIGN_MASK };
	for( int i = 0; i < s; i += 4 )
	{
		_mm_store_ps( mat + i, _mm_xor_ps( _mm_load_ps( mat + i ), ( __m128& ) signBit[0] ) );
	}
#else
	for( int i = 0; i < s; i++ )
	{
		mat[i] = -mat[i];
	}
#endif
}

/*
========================
idMatX::Clamp
========================
*/
ID_INLINE void idMatX::Clamp( float min, float max )
{
	int s = numRows * numColumns;
	for( int i = 0; i < s; i++ )
	{
		if( mat[i] < min )
		{
			mat[i] = min;
		}
		else if( mat[i] > max )
		{
			mat[i] = max;
		}
	}
}

/*
========================
idMatX::SwapRows
========================
*/
ID_INLINE idMatX& idMatX::SwapRows( int r1, int r2 )
{
	float* ptr1 = mat + r1 * numColumns;
	float* ptr2 = mat + r2 * numColumns;
	for( int i = 0; i < numColumns; i++ )
	{
		SwapValues( ptr1[i], ptr2[i] );
	}
	return *this;
}

/*
========================
idMatX::SwapColumns
========================
*/
ID_INLINE idMatX& idMatX::SwapColumns( int r1, int r2 )
{
	float* ptr = mat;
	for( int i = 0; i < numRows; i++, ptr += numColumns )
	{
		SwapValues( ptr[r1], ptr[r2] );
	}
	return *this;
}

/*
========================
idMatX::SwapRowsColumns
========================
*/
ID_INLINE idMatX& idMatX::SwapRowsColumns( int r1, int r2 )
{
	SwapRows( r1, r2 );
	SwapColumns( r1, r2 );
	return *this;
}

/*
========================
idMatX::ClearUpperTriangle
========================
*/
ID_INLINE void idMatX::ClearUpperTriangle()
{
	assert( numRows == numColumns );
	for( int i = numRows - 2; i >= 0; i-- )
	{
		memset( mat + i * numColumns + i + 1, 0, ( numColumns - 1 - i ) * sizeof( float ) );
	}
}

/*
========================
idMatX::ClearLowerTriangle
========================
*/
ID_INLINE void idMatX::ClearLowerTriangle()
{
	assert( numRows == numColumns );
	for( int i = 1; i < numRows; i++ )
	{
		memset( mat + i * numColumns, 0, i * sizeof( float ) );
	}
}

/*
========================
idMatX::SquareSubMatrix
========================
*/
ID_INLINE void idMatX::SquareSubMatrix( const idMatX& m, int size )
{
	assert( size <= m.numRows && size <= m.numColumns );
	SetSize( size, size );
	for( int i = 0; i < size; i++ )
	{
		memcpy( mat + i * numColumns, m.mat + i * m.numColumns, size * sizeof( float ) );
	}
}

/*
========================
idMatX::MaxDifference
========================
*/
ID_INLINE float idMatX::MaxDifference( const idMatX& m ) const
{
	assert( numRows == m.numRows && numColumns == m.numColumns );

	float maxDiff = -1.0f;
	for( int i = 0; i < numRows; i++ )
	{
		for( int j = 0; j < numColumns; j++ )
		{
			float diff = idMath::Fabs( mat[ i * numColumns + j ] - m[i][j] );
			if( maxDiff < 0.0f || diff > maxDiff )
			{
				maxDiff = diff;
			}
		}
	}
	return maxDiff;
}

/*
========================
idMatX::IsZero
========================
*/
ID_INLINE bool idMatX::IsZero( const float epsilon ) const
{
	// returns true if (*this) == Zero
	for( int i = 0; i < numRows; i++ )
	{
		for( int j = 0; j < numColumns; j++ )
		{
			if( idMath::Fabs( mat[i * numColumns + j] ) > epsilon )
			{
				return false;
			}
		}
	}
	return true;
}

/*
========================
idMatX::IsIdentity
========================
*/
ID_INLINE bool idMatX::IsIdentity( const float epsilon ) const
{
	// returns true if (*this) == Identity
	assert( numRows == numColumns );
	for( int i = 0; i < numRows; i++ )
	{
		for( int j = 0; j < numColumns; j++ )
		{
			if( idMath::Fabs( mat[i * numColumns + j] - ( float )( i == j ) ) > epsilon )
			{
				return false;
			}
		}
	}
	return true;
}

/*
========================
idMatX::IsDiagonal
========================
*/
ID_INLINE bool idMatX::IsDiagonal( const float epsilon ) const
{
	// returns true if all elements are zero except for the elements on the diagonal
	assert( numRows == numColumns );
	for( int i = 0; i < numRows; i++ )
	{
		for( int j = 0; j < numColumns; j++ )
		{
			if( i != j && idMath::Fabs( mat[i * numColumns + j] ) > epsilon )
			{
				return false;
			}
		}
	}
	return true;
}

/*
========================
idMatX::IsTriDiagonal
========================
*/
ID_INLINE bool idMatX::IsTriDiagonal( const float epsilon ) const
{
	// returns true if all elements are zero except for the elements on the diagonal plus or minus one column

	if( numRows != numColumns )
	{
		return false;
	}
	for( int i = 0; i < numRows - 2; i++ )
	{
		for( int j = i + 2; j < numColumns; j++ )
		{
			if( idMath::Fabs( ( *this )[i][j] ) > epsilon )
			{
				return false;
			}
			if( idMath::Fabs( ( *this )[j][i] ) > epsilon )
			{
				return false;
			}
		}
	}
	return true;
}

/*
========================
idMatX::IsSymmetric
========================
*/
ID_INLINE bool idMatX::IsSymmetric( const float epsilon ) const
{
	// (*this)[i][j] == (*this)[j][i]
	if( numRows != numColumns )
	{
		return false;
	}
	for( int i = 0; i < numRows; i++ )
	{
		for( int j = 0; j < numColumns; j++ )
		{
			if( idMath::Fabs( mat[ i * numColumns + j ] - mat[ j * numColumns + i ] ) > epsilon )
			{
				return false;
			}
		}
	}
	return true;
}

/*
========================
idMatX::Trace
========================
*/
ID_INLINE float idMatX::Trace() const
{
	float trace = 0.0f;

	assert( numRows == numColumns );

	// sum of elements on the diagonal
	for( int i = 0; i < numRows; i++ )
	{
		trace += mat[i * numRows + i];
	}
	return trace;
}

/*
========================
idMatX::Determinant
========================
*/
ID_INLINE float idMatX::Determinant() const
{

	assert( numRows == numColumns );

	switch( numRows )
	{
		case 1:
			return mat[0];
		case 2:
			return reinterpret_cast<const idMat2*>( mat )->Determinant();
		case 3:
			return reinterpret_cast<const idMat3*>( mat )->Determinant();
		case 4:
			return reinterpret_cast<const idMat4*>( mat )->Determinant();
		case 5:
			return reinterpret_cast<const idMat5*>( mat )->Determinant();
		case 6:
			return reinterpret_cast<const idMat6*>( mat )->Determinant();
		default:
			return DeterminantGeneric();
	}
}

/*
========================
idMatX::Transpose
========================
*/
ID_INLINE idMatX idMatX::Transpose() const
{
	idMatX transpose;

	transpose.SetTempSize( numColumns, numRows );

	for( int i = 0; i < numRows; i++ )
	{
		for( int j = 0; j < numColumns; j++ )
		{
			transpose.mat[j * transpose.numColumns + i] = mat[i * numColumns + j];
		}
	}

	return transpose;
}

/*
========================
idMatX::TransposeSelf
========================
*/
ID_INLINE idMatX& idMatX::TransposeSelf()
{
	*this = Transpose();
	return *this;
}

/*
========================
idMatX::Transpose
========================
*/
ID_INLINE void idMatX::Transpose( idMatX& dst ) const
{
	dst = Transpose();
}

/*
========================
idMatX::Inverse
========================
*/
ID_INLINE idMatX idMatX::Inverse() const
{
	idMatX invMat;

	invMat.SetTempSize( numRows, numColumns );
	memcpy( invMat.mat, mat, numRows * numColumns * sizeof( float ) );
	verify( invMat.InverseSelf() );
	return invMat;
}

/*
========================
idMatX::InverseSelf
========================
*/
ID_INLINE bool idMatX::InverseSelf()
{

	assert( numRows == numColumns );

	switch( numRows )
	{
		case 1:
			if( idMath::Fabs( mat[0] ) < MATRIX_INVERSE_EPSILON )
			{
				return false;
			}
			mat[0] = 1.0f / mat[0];
			return true;
		case 2:
			return reinterpret_cast<idMat2*>( mat )->InverseSelf();
		case 3:
			return reinterpret_cast<idMat3*>( mat )->InverseSelf();
		case 4:
			return reinterpret_cast<idMat4*>( mat )->InverseSelf();
		case 5:
			return reinterpret_cast<idMat5*>( mat )->InverseSelf();
		case 6:
			return reinterpret_cast<idMat6*>( mat )->InverseSelf();
		default:
			return InverseSelfGeneric();
	}
}

/*
========================
idMatX::InverseFast
========================
*/
ID_INLINE idMatX idMatX::InverseFast() const
{
	idMatX invMat;

	invMat.SetTempSize( numRows, numColumns );
	memcpy( invMat.mat, mat, numRows * numColumns * sizeof( float ) );
	verify( invMat.InverseFastSelf() );
	return invMat;
}

/*
========================
idMatX::InverseFastSelf
========================
*/
ID_INLINE bool idMatX::InverseFastSelf()
{

	assert( numRows == numColumns );

	switch( numRows )
	{
		case 1:
			if( idMath::Fabs( mat[0] ) < MATRIX_INVERSE_EPSILON )
			{
				return false;
			}
			mat[0] = 1.0f / mat[0];
			return true;
		case 2:
			return reinterpret_cast<idMat2*>( mat )->InverseFastSelf();
		case 3:
			return reinterpret_cast<idMat3*>( mat )->InverseFastSelf();
		case 4:
			return reinterpret_cast<idMat4*>( mat )->InverseFastSelf();
		case 5:
			return reinterpret_cast<idMat5*>( mat )->InverseFastSelf();
		case 6:
			return reinterpret_cast<idMat6*>( mat )->InverseFastSelf();
		default:
			return InverseSelfGeneric();
	}
}

/*
========================
idMatX::Inverse
========================
*/
ID_INLINE void idMatX::Inverse( idMatX& dst ) const
{
	dst = InverseFast();
}

/*
========================
idMatX::Subtract
========================
*/
ID_INLINE void idMatX::Subtract( const idMatX& a )
{
	( *this ) -= a;
}

/*
========================
idMatX::Multiply
========================
*/
ID_INLINE idVecX idMatX::Multiply( const idVecX& vec ) const
{
	assert( numColumns == vec.GetSize() );

	idVecX dst;
	dst.SetTempSize( numRows );
	Multiply( dst, vec );
	return dst;
}

/*
========================
idMatX::Multiply
========================
*/
ID_INLINE idMatX idMatX::Multiply( const idMatX& a ) const
{
	assert( numColumns == a.numRows );

	idMatX dst;
	dst.SetTempSize( numRows, a.numColumns );
	Multiply( dst, a );
	return dst;
}

/*
========================
idMatX::TransposeMultiply
========================
*/
ID_INLINE idVecX idMatX::TransposeMultiply( const idVecX& vec ) const
{
	assert( numRows == vec.GetSize() );

	idVecX dst;
	dst.SetTempSize( numColumns );
	TransposeMultiply( dst, vec );
	return dst;
}

/*
========================
idMatX::TransposeMultiply
========================
*/
ID_INLINE idMatX idMatX::TransposeMultiply( const idMatX& a ) const
{
	assert( numRows == a.numRows );

	idMatX dst;
	dst.SetTempSize( numColumns, a.numColumns );
	TransposeMultiply( dst, a );
	return dst;
}

/*
========================
idMatX::Multiply
========================
*/
ID_INLINE void idMatX::Multiply( idVecX& dst, const idVecX& vec ) const
{
	dst.SetSize( numRows );
	const float* mPtr = mat;
	const float* vPtr = vec.ToFloatPtr();
	float* dstPtr = dst.ToFloatPtr();
	float* temp = ( float* )_alloca16( numRows * sizeof( float ) );
	for( int i = 0; i < numRows; i++ )
	{
		float sum = mPtr[0] * vPtr[0];
		for( int j = 1; j < numColumns; j++ )
		{
			sum += mPtr[j] * vPtr[j];
		}
		temp[i] = sum;
		mPtr += numColumns;
	}
	for( int i = 0; i < numRows; i++ )
	{
		dstPtr[i] = temp[i];
	}
}

/*
========================
idMatX::MultiplyAdd
========================
*/
ID_INLINE void idMatX::MultiplyAdd( idVecX& dst, const idVecX& vec ) const
{
	assert( dst.GetSize() == numRows );
	const float* mPtr = mat;
	const float* vPtr = vec.ToFloatPtr();
	float* dstPtr = dst.ToFloatPtr();
	float* temp = ( float* )_alloca16( numRows * sizeof( float ) );
	for( int i = 0; i < numRows; i++ )
	{
		float sum = mPtr[0] * vPtr[0];
		for( int j = 1; j < numColumns; j++ )
		{
			sum += mPtr[j] * vPtr[j];
		}
		temp[i] = dstPtr[i] + sum;
		mPtr += numColumns;
	}
	for( int i = 0; i < numRows; i++ )
	{
		dstPtr[i] = temp[i];
	}
}

/*
========================
idMatX::MultiplySub
========================
*/
ID_INLINE void idMatX::MultiplySub( idVecX& dst, const idVecX& vec ) const
{
	assert( dst.GetSize() == numRows );
	const float* mPtr = mat;
	const float* vPtr = vec.ToFloatPtr();
	float* dstPtr = dst.ToFloatPtr();
	float* temp = ( float* )_alloca16( numRows * sizeof( float ) );
	for( int i = 0; i < numRows; i++ )
	{
		float sum = mPtr[0] * vPtr[0];
		for( int j = 1; j < numColumns; j++ )
		{
			sum += mPtr[j] * vPtr[j];
		}
		temp[i] = dstPtr[i] - sum;
		mPtr += numColumns;
	}
	for( int i = 0; i < numRows; i++ )
	{
		dstPtr[i] = temp[i];
	}
}

/*
========================
idMatX::TransposeMultiply
========================
*/
ID_INLINE void idMatX::TransposeMultiply( idVecX& dst, const idVecX& vec ) const
{
	dst.SetSize( numColumns );
	const float* vPtr = vec.ToFloatPtr();
	float* dstPtr = dst.ToFloatPtr();
	float* temp = ( float* )_alloca16( numColumns * sizeof( float ) );
	for( int i = 0; i < numColumns; i++ )
	{
		const float* mPtr = mat + i;
		float sum = mPtr[0] * vPtr[0];
		for( int j = 1; j < numRows; j++ )
		{
			mPtr += numColumns;
			sum += mPtr[0] * vPtr[j];
		}
		temp[i] = sum;
	}
	for( int i = 0; i < numColumns; i++ )
	{
		dstPtr[i] = temp[i];
	}
}

/*
========================
idMatX::TransposeMultiplyAdd
========================
*/
ID_INLINE void idMatX::TransposeMultiplyAdd( idVecX& dst, const idVecX& vec ) const
{
	assert( dst.GetSize() == numColumns );
	const float* vPtr = vec.ToFloatPtr();
	float* dstPtr = dst.ToFloatPtr();
	float* temp = ( float* )_alloca16( numColumns * sizeof( float ) );
	for( int i = 0; i < numColumns; i++ )
	{
		const float* mPtr = mat + i;
		float sum = mPtr[0] * vPtr[0];
		for( int j = 1; j < numRows; j++ )
		{
			mPtr += numColumns;
			sum += mPtr[0] * vPtr[j];
		}
		temp[i] = dstPtr[i] + sum;
	}
	for( int i = 0; i < numColumns; i++ )
	{
		dstPtr[i] = temp[i];
	}
}

/*
========================
idMatX::TransposeMultiplySub
========================
*/
ID_INLINE void idMatX::TransposeMultiplySub( idVecX& dst, const idVecX& vec ) const
{
	assert( dst.GetSize() == numColumns );
	const float* vPtr = vec.ToFloatPtr();
	float* dstPtr = dst.ToFloatPtr();
	float* temp = ( float* )_alloca16( numColumns * sizeof( float ) );
	for( int i = 0; i < numColumns; i++ )
	{
		const float* mPtr = mat + i;
		float sum = mPtr[0] * vPtr[0];
		for( int j = 1; j < numRows; j++ )
		{
			mPtr += numColumns;
			sum += mPtr[0] * vPtr[j];
		}
		temp[i] = dstPtr[i] - sum;
	}
	for( int i = 0; i < numColumns; i++ )
	{
		dstPtr[i] = temp[i];
	}
}

/*
========================
idMatX::Multiply
========================
*/
ID_INLINE void idMatX::Multiply( idMatX& dst, const idMatX& a ) const
{
	assert( numColumns == a.numRows );
	assert( &dst != &a && &dst != this );

	dst.SetSize( numRows, a.numColumns );
	float* dstPtr = dst.ToFloatPtr();
	const float* m1Ptr = ToFloatPtr();
	int k = numRows;
	int l = a.GetNumColumns();
	for( int i = 0; i < k; i++ )
	{
		for( int j = 0; j < l; j++ )
		{
			const float* m2Ptr = a.ToFloatPtr() + j;
			float sum = m1Ptr[0] * m2Ptr[0];
			for( int n = 1; n < numColumns; n++ )
			{
				m2Ptr += l;
				sum += m1Ptr[n] * m2Ptr[0];
			}
			*dstPtr++ = sum;
		}
		m1Ptr += numColumns;
	}
}

/*
========================
idMatX::TransposeMultiply
========================
*/
ID_INLINE void idMatX::TransposeMultiply( idMatX& dst, const idMatX& a ) const
{
	assert( numRows == a.numRows );
	assert( &dst != &a && &dst != this );

	dst.SetSize( numColumns, a.numColumns );
	float* dstPtr = dst.ToFloatPtr();
	int k = numColumns;
	int l = a.numColumns;
	for( int i = 0; i < k; i++ )
	{
		for( int j = 0; j < l; j++ )
		{
			const float* m1Ptr = ToFloatPtr() + i;
			const float* m2Ptr = a.ToFloatPtr() + j;
			float sum = m1Ptr[0] * m2Ptr[0];
			for( int n = 1; n < numRows; n++ )
			{
				m1Ptr += numColumns;
				m2Ptr += a.numColumns;
				sum += m1Ptr[0] * m2Ptr[0];
			}
			*dstPtr++ = sum;
		}
	}
}

/*
========================
idMatX::GetDimension
========================
*/
ID_INLINE int idMatX::GetDimension() const
{
	return numRows * numColumns;
}

/*
========================
idMatX::SubVec6
========================
*/
ID_INLINE const idVec6& idMatX::SubVec6( int row ) const
{
	assert( numColumns >= 6 && row >= 0 && row < numRows );
	return *reinterpret_cast<const idVec6*>( mat + row * numColumns );
}

/*
========================
idMatX::SubVec6
========================
*/
ID_INLINE idVec6& idMatX::SubVec6( int row )
{
	assert( numColumns >= 6 && row >= 0 && row < numRows );
	return *reinterpret_cast<idVec6*>( mat + row * numColumns );
}

/*
========================
idMatX::SubVecX
========================
*/
ID_INLINE const idVecX idMatX::SubVecX( int row ) const
{
	idVecX v;
	assert( row >= 0 && row < numRows );
	v.SetData( numColumns, mat + row * numColumns );
	return v;
}

/*
========================
idMatX::SubVecX
========================
*/
ID_INLINE idVecX idMatX::SubVecX( int row )
{
	idVecX v;
	assert( row >= 0 && row < numRows );
	v.SetData( numColumns, mat + row * numColumns );
	return v;
}

/*
========================
idMatX::ToFloatPtr
========================
*/
ID_INLINE const float* idMatX::ToFloatPtr() const
{
	return mat;
}

/*
========================
idMatX::ToFloatPtr
========================
*/
ID_INLINE float* idMatX::ToFloatPtr()
{
	return mat;
}

#endif // !__MATH_MATRIXX_H__
