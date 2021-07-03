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

#ifndef __MATH_VECX_H__
#define __MATH_VECX_H__

/*
===============================================================================

idVecX - arbitrary sized vector

The vector lives on 16 byte aligned and 16 byte padded memory.

NOTE: due to the temporary memory pool idVecX cannot be used by multiple threads

===============================================================================
*/

#define VECX_MAX_TEMP		1024
#define VECX_QUAD( x )		( ( ( ( x ) + 3 ) & ~3 ) * sizeof( float ) )
#define VECX_CLEAREND()		int s = size; while( s < ( ( s + 3) & ~3 ) ) { p[s++] = 0.0f; }
#define VECX_ALLOCA( n )	( (float *) _alloca16( VECX_QUAD( n ) ) )

#if defined(USE_INTRINSICS_SSE)
	#define VECX_SIMD
#endif

class idVecX
{
	friend class idMatX;

public:
	ID_INLINE					idVecX();
	ID_INLINE					explicit idVecX( int length );
	ID_INLINE					explicit idVecX( int length, float* data );
	ID_INLINE					~idVecX();

	ID_INLINE	float			Get( int index ) const;
	ID_INLINE	float& 			Get( int index );

	ID_INLINE	float			operator[]( const int index ) const;
	ID_INLINE	float& 			operator[]( const int index );
	ID_INLINE	idVecX			operator-() const;
	ID_INLINE	idVecX& 		operator=( const idVecX& a );
	ID_INLINE	idVecX			operator*( const float a ) const;
	ID_INLINE	idVecX			operator/( const float a ) const;
	ID_INLINE	float			operator*( const idVecX& a ) const;
	ID_INLINE	idVecX			operator-( const idVecX& a ) const;
	ID_INLINE	idVecX			operator+( const idVecX& a ) const;
	ID_INLINE	idVecX& 		operator*=( const float a );
	ID_INLINE	idVecX& 		operator/=( const float a );
	ID_INLINE	idVecX& 		operator+=( const idVecX& a );
	ID_INLINE	idVecX& 		operator-=( const idVecX& a );

	friend ID_INLINE	idVecX	operator*( const float a, const idVecX& b );

	ID_INLINE	bool			Compare( const idVecX& a ) const;							// exact compare, no epsilon
	ID_INLINE	bool			Compare( const idVecX& a, const float epsilon ) const;		// compare with epsilon
	ID_INLINE	bool			operator==(	const idVecX& a ) const;						// exact compare, no epsilon
	ID_INLINE	bool			operator!=(	const idVecX& a ) const;						// exact compare, no epsilon

	ID_INLINE	void			SetSize( int size );
	ID_INLINE	void			ChangeSize( int size, bool makeZero = false );
	ID_INLINE	int				GetSize() const
	{
		return size;
	}
	ID_INLINE	void			SetData( int length, float* data );
	ID_INLINE	void			Zero();
	ID_INLINE	void			Zero( int length );
	ID_INLINE	void			Random( int seed, float l = 0.0f, float u = 1.0f );
	ID_INLINE	void			Random( int length, int seed, float l = 0.0f, float u = 1.0f );
	ID_INLINE	void			Negate();
	ID_INLINE	void			Clamp( float min, float max );
	ID_INLINE	idVecX& 		SwapElements( int e1, int e2 );

	ID_INLINE	float			Length() const;
	ID_INLINE	float			LengthSqr() const;
	ID_INLINE	idVecX			Normalize() const;
	ID_INLINE	float			NormalizeSelf();

	ID_INLINE	int				GetDimension() const;

	ID_INLINE	void			AddScaleAdd( const float scale, const idVecX& v0, const idVecX& v1 );

	ID_INLINE	const idVec3& 	SubVec3( int index ) const;
	ID_INLINE	idVec3& 		SubVec3( int index );
	ID_INLINE	const idVec6& 	SubVec6( int index = 0 ) const;
	ID_INLINE	idVec6& 		SubVec6( int index = 0 );
	ID_INLINE	const float* 	ToFloatPtr() const;
	ID_INLINE	float* 			ToFloatPtr();
	const char* 	ToString( int precision = 2 ) const;

private:
	int				size;					// size of the vector
	int				alloced;				// if -1 p points to data set with SetData
	float* 			p;						// memory the vector is stored

	static float	temp[VECX_MAX_TEMP + 4];	// used to store intermediate results
	static float* 	tempPtr;				// pointer to 16 byte aligned temporary memory
	static int		tempIndex;				// index into memory pool, wraps around

	ID_INLINE void	SetTempSize( int size );
};


/*
========================
idVecX::idVecX
========================
*/
ID_INLINE idVecX::idVecX()
{
	size = alloced = 0;
	p = NULL;
}

/*
========================
idVecX::idVecX
========================
*/
ID_INLINE idVecX::idVecX( int length )
{
	size = alloced = 0;
	p = NULL;
	SetSize( length );
}

/*
========================
idVecX::idVecX
========================
*/
ID_INLINE idVecX::idVecX( int length, float* data )
{
	size = alloced = 0;
	p = NULL;
	SetData( length, data );
}

/*
========================
idVecX::~idVecX
========================
*/
ID_INLINE idVecX::~idVecX()
{
	// if not temp memory
	if( p && ( p < idVecX::tempPtr || p >= idVecX::tempPtr + VECX_MAX_TEMP ) && alloced != -1 )
	{
		Mem_Free16( p );
	}
}

/*
========================
idVecX::Get
========================
*/
ID_INLINE float idVecX::Get( int index ) const
{
	assert( index >= 0 && index < size );
	return p[index];
}

/*
========================
idVecX::Get
========================
*/
ID_INLINE float& idVecX::Get( int index )
{
	assert( index >= 0 && index < size );
	return p[index];
}

/*
========================
idVecX::operator[]
========================
*/
ID_INLINE float idVecX::operator[]( int index ) const
{
	return Get( index );
}

/*
========================
idVecX::operator[]
========================
*/
ID_INLINE float& idVecX::operator[]( int index )
{
	return Get( index );
}

/*
========================
idVecX::operator-
========================
*/
ID_INLINE idVecX idVecX::operator-() const
{
	idVecX m;

	m.SetTempSize( size );
#ifdef VECX_SIMD
	ALIGN16( unsigned int signBit[4] ) = { IEEE_FLT_SIGN_MASK, IEEE_FLT_SIGN_MASK, IEEE_FLT_SIGN_MASK, IEEE_FLT_SIGN_MASK };
	for( int i = 0; i < size; i += 4 )
	{
		_mm_store_ps( m.p + i, _mm_xor_ps( _mm_load_ps( p + i ), ( __m128& ) signBit[0] ) );
	}
#else
	for( int i = 0; i < size; i++ )
	{
		m.p[i] = -p[i];
	}
#endif
	return m;
}

/*
========================
idVecX::operator=
========================
*/
ID_INLINE idVecX& idVecX::operator=( const idVecX& a )
{
	SetSize( a.size );
#ifdef VECX_SIMD
	for( int i = 0; i < a.size; i += 4 )
	{
		_mm_store_ps( p + i, _mm_load_ps( a.p + i ) );
	}
#else
	memcpy( p, a.p, a.size * sizeof( float ) );
#endif
	idVecX::tempIndex = 0;
	return *this;
}

/*
========================
idVecX::operator+
========================
*/
ID_INLINE idVecX idVecX::operator+( const idVecX& a ) const
{
	idVecX m;

	assert( size == a.size );
	m.SetTempSize( size );
#ifdef VECX_SIMD
	for( int i = 0; i < size; i += 4 )
	{
		_mm_store_ps( m.p + i, _mm_add_ps( _mm_load_ps( p + i ), _mm_load_ps( a.p + i ) ) );
	}
#else
	for( int i = 0; i < size; i++ )
	{
		m.p[i] = p[i] + a.p[i];
	}
#endif
	return m;
}

/*
========================
idVecX::operator-
========================
*/
ID_INLINE idVecX idVecX::operator-( const idVecX& a ) const
{
	idVecX m;

	assert( size == a.size );
	m.SetTempSize( size );
#ifdef VECX_SIMD
	for( int i = 0; i < size; i += 4 )
	{
		_mm_store_ps( m.p + i, _mm_sub_ps( _mm_load_ps( p + i ), _mm_load_ps( a.p + i ) ) );
	}
#else
	for( int i = 0; i < size; i++ )
	{
		m.p[i] = p[i] - a.p[i];
	}
#endif
	return m;
}

/*
========================
idVecX::operator+=
========================
*/
ID_INLINE idVecX& idVecX::operator+=( const idVecX& a )
{
	assert( size == a.size );
#ifdef VECX_SIMD
	for( int i = 0; i < size; i += 4 )
	{
		_mm_store_ps( p + i, _mm_add_ps( _mm_load_ps( p + i ), _mm_load_ps( a.p + i ) ) );
	}
#else
	for( int i = 0; i < size; i++ )
	{
		p[i] += a.p[i];
	}
#endif
	idVecX::tempIndex = 0;
	return *this;
}

/*
========================
idVecX::operator-=
========================
*/
ID_INLINE idVecX& idVecX::operator-=( const idVecX& a )
{
	assert( size == a.size );
#ifdef VECX_SIMD
	for( int i = 0; i < size; i += 4 )
	{
		_mm_store_ps( p + i, _mm_sub_ps( _mm_load_ps( p + i ), _mm_load_ps( a.p + i ) ) );
	}
#else
	for( int i = 0; i < size; i++ )
	{
		p[i] -= a.p[i];
	}
#endif
	idVecX::tempIndex = 0;
	return *this;
}

/*
========================
idVecX::operator*
========================
*/
ID_INLINE idVecX idVecX::operator*( const float a ) const
{
	idVecX m;

	m.SetTempSize( size );
#ifdef VECX_SIMD
	__m128 va = _mm_load1_ps( & a );
	for( int i = 0; i < size; i += 4 )
	{
		_mm_store_ps( m.p + i, _mm_mul_ps( _mm_load_ps( p + i ), va ) );
	}
#else
	for( int i = 0; i < size; i++ )
	{
		m.p[i] = p[i] * a;
	}
#endif
	return m;
}

/*
========================
idVecX::operator*=
========================
*/
ID_INLINE idVecX& idVecX::operator*=( const float a )
{
#ifdef VECX_SIMD
	__m128 va = _mm_load1_ps( & a );
	for( int i = 0; i < size; i += 4 )
	{
		_mm_store_ps( p + i, _mm_mul_ps( _mm_load_ps( p + i ), va ) );
	}
#else
	for( int i = 0; i < size; i++ )
	{
		p[i] *= a;
	}
#endif
	return *this;
}

/*
========================
idVecX::operator/
========================
*/
ID_INLINE idVecX idVecX::operator/( const float a ) const
{
	assert( fabs( a ) > idMath::FLT_SMALLEST_NON_DENORMAL );
	return ( *this ) * ( 1.0f / a );
}

/*
========================
idVecX::operator/=
========================
*/
ID_INLINE idVecX& idVecX::operator/=( const float a )
{
	assert( fabs( a ) > idMath::FLT_SMALLEST_NON_DENORMAL );
	( *this ) *= ( 1.0f / a );
	return *this;
}

/*
========================
operator*
========================
*/
ID_INLINE idVecX operator*( const float a, const idVecX& b )
{
	return b * a;
}

/*
========================
idVecX::operator*
========================
*/
ID_INLINE float idVecX::operator*( const idVecX& a ) const
{
	assert( size == a.size );
	float sum = 0.0f;
	for( int i = 0; i < size; i++ )
	{
		sum += p[i] * a.p[i];
	}
	return sum;
}

/*
========================
idVecX::Compare
========================
*/
ID_INLINE bool idVecX::Compare( const idVecX& a ) const
{
	assert( size == a.size );
	for( int i = 0; i < size; i++ )
	{
		if( p[i] != a.p[i] )
		{
			return false;
		}
	}
	return true;
}

/*
========================
idVecX::Compare
========================
*/
ID_INLINE bool idVecX::Compare( const idVecX& a, const float epsilon ) const
{
	assert( size == a.size );
	for( int i = 0; i < size; i++ )
	{
		if( idMath::Fabs( p[i] - a.p[i] ) > epsilon )
		{
			return false;
		}
	}
	return true;
}

/*
========================
idVecX::operator==
========================
*/
ID_INLINE bool idVecX::operator==( const idVecX& a ) const
{
	return Compare( a );
}

/*
========================
idVecX::operator!=
========================
*/
ID_INLINE bool idVecX::operator!=( const idVecX& a ) const
{
	return !Compare( a );
}

/*
========================
idVecX::SetSize
========================
*/
ID_INLINE void idVecX::SetSize( int newSize )
{
	//assert( p < idVecX::tempPtr || p > idVecX::tempPtr + VECX_MAX_TEMP );
	if( newSize != size || p == NULL )
	{
		int alloc = ( newSize + 3 ) & ~3;
		if( alloc > alloced && alloced != -1 )
		{
			if( p )
			{
				Mem_Free16( p );
			}
			p = ( float* ) Mem_Alloc16( alloc * sizeof( float ), TAG_MATH );
			alloced = alloc;
		}
		size = newSize;
		VECX_CLEAREND();
	}
}

/*
========================
idVecX::ChangeSize
========================
*/
ID_INLINE void idVecX::ChangeSize( int newSize, bool makeZero )
{
	if( newSize != size )
	{
		int alloc = ( newSize + 3 ) & ~3;
		if( alloc > alloced && alloced != -1 )
		{
			float* oldVec = p;
			p = ( float* ) Mem_Alloc16( alloc * sizeof( float ), TAG_MATH );
			alloced = alloc;
			if( oldVec )
			{
				for( int i = 0; i < size; i++ )
				{
					p[i] = oldVec[i];
				}
				Mem_Free16( oldVec );
			}
			if( makeZero )
			{
				// zero any new elements
				for( int i = size; i < newSize; i++ )
				{
					p[i] = 0.0f;
				}
			}
		}
		size = newSize;
		VECX_CLEAREND();
	}
}

/*
========================
idVecX::SetTempSize
========================
*/
ID_INLINE void idVecX::SetTempSize( int newSize )
{
	size = newSize;
	alloced = ( newSize + 3 ) & ~3;
	assert( alloced < VECX_MAX_TEMP );
	if( idVecX::tempIndex + alloced > VECX_MAX_TEMP )
	{
		idVecX::tempIndex = 0;
	}
	p = idVecX::tempPtr + idVecX::tempIndex;
	idVecX::tempIndex += alloced;
	VECX_CLEAREND();
}

/*
========================
idVecX::SetData
========================
*/
ID_INLINE void idVecX::SetData( int length, float* data )
{
	if( p != NULL && ( p < idVecX::tempPtr || p >= idVecX::tempPtr + VECX_MAX_TEMP ) && alloced != -1 )
	{
		Mem_Free16( p );
	}
	assert_16_byte_aligned( data ); // data must be 16 byte aligned
	p = data;
	size = length;
	alloced = -1;
	VECX_CLEAREND();
}

/*
========================
idVecX::Zero
========================
*/
ID_INLINE void idVecX::Zero()
{
#ifdef VECX_SIMD
	for( int i = 0; i < size; i += 4 )
	{
		_mm_store_ps( p + i, _mm_setzero_ps() );
	}
#else
	memset( p, 0, size * sizeof( float ) );
#endif
}

/*
========================
idVecX::Zero
========================
*/
ID_INLINE void idVecX::Zero( int length )
{
	SetSize( length );
#ifdef VECX_SIMD
	for( int i = 0; i < length; i += 4 )
	{
		_mm_store_ps( p + i, _mm_setzero_ps() );
	}
#else
	memset( p, 0, length * sizeof( float ) );
#endif
}

/*
========================
idVecX::Random
========================
*/
ID_INLINE void idVecX::Random( int seed, float l, float u )
{
	idRandom rnd( seed );

	float c = u - l;
	for( int i = 0; i < size; i++ )
	{
		p[i] = l + rnd.RandomFloat() * c;
	}
}

/*
========================
idVecX::Random
========================
*/
ID_INLINE void idVecX::Random( int length, int seed, float l, float u )
{
	idRandom rnd( seed );

	SetSize( length );
	float c = u - l;
	for( int i = 0; i < size; i++ )
	{
		p[i] = l + rnd.RandomFloat() * c;
	}
}

/*
========================
idVecX::Negate
========================
*/
ID_INLINE void idVecX::Negate()
{
#ifdef VECX_SIMD
	ALIGN16( const unsigned int signBit[4] ) = { IEEE_FLT_SIGN_MASK, IEEE_FLT_SIGN_MASK, IEEE_FLT_SIGN_MASK, IEEE_FLT_SIGN_MASK };
	for( int i = 0; i < size; i += 4 )
	{
		_mm_store_ps( p + i, _mm_xor_ps( _mm_load_ps( p + i ), ( __m128& ) signBit[0] ) );
	}
#else
	for( int i = 0; i < size; i++ )
	{
		p[i] = -p[i];
	}
#endif
}

/*
========================
idVecX::Clamp
========================
*/
ID_INLINE void idVecX::Clamp( float min, float max )
{
	for( int i = 0; i < size; i++ )
	{
		if( p[i] < min )
		{
			p[i] = min;
		}
		else if( p[i] > max )
		{
			p[i] = max;
		}
	}
}

/*
========================
idVecX::SwapElements
========================
*/
ID_INLINE idVecX& idVecX::SwapElements( int e1, int e2 )
{
	float tmp;
	tmp = p[e1];
	p[e1] = p[e2];
	p[e2] = tmp;
	return *this;
}

/*
========================
idVecX::Length
========================
*/
ID_INLINE float idVecX::Length() const
{
	float sum = 0.0f;
	for( int i = 0; i < size; i++ )
	{
		sum += p[i] * p[i];
	}
	return idMath::Sqrt( sum );
}

/*
========================
idVecX::LengthSqr
========================
*/
ID_INLINE float idVecX::LengthSqr() const
{
	float sum = 0.0f;
	for( int i = 0; i < size; i++ )
	{
		sum += p[i] * p[i];
	}
	return sum;
}

/*
========================
idVecX::Normalize
========================
*/
ID_INLINE idVecX idVecX::Normalize() const
{
	idVecX m;

	m.SetTempSize( size );
	float sum = 0.0f;
	for( int i = 0; i < size; i++ )
	{
		sum += p[i] * p[i];
	}
	float invSqrt = idMath::InvSqrt( sum );
	for( int i = 0; i < size; i++ )
	{
		m.p[i] = p[i] * invSqrt;
	}
	return m;
}

/*
========================
idVecX::NormalizeSelf
========================
*/
ID_INLINE float idVecX::NormalizeSelf()
{
	float sum = 0.0f;
	for( int i = 0; i < size; i++ )
	{
		sum += p[i] * p[i];
	}
	float invSqrt = idMath::InvSqrt( sum );
	for( int i = 0; i < size; i++ )
	{
		p[i] *= invSqrt;
	}
	return invSqrt * sum;
}

/*
========================
idVecX::GetDimension
========================
*/
ID_INLINE int idVecX::GetDimension() const
{
	return size;
}

/*
========================
idVecX::SubVec3
========================
*/
ID_INLINE idVec3& idVecX::SubVec3( int index )
{
	assert( index >= 0 && index * 3 + 3 <= size );
	return *reinterpret_cast<idVec3*>( p + index * 3 );
}

/*
========================
idVecX::SubVec3
========================
*/
ID_INLINE const idVec3& idVecX::SubVec3( int index ) const
{
	assert( index >= 0 && index * 3 + 3 <= size );
	return *reinterpret_cast<const idVec3*>( p + index * 3 );
}

/*
========================
idVecX::SubVec6
========================
*/
ID_INLINE idVec6& idVecX::SubVec6( int index )
{
	assert( index >= 0 && index * 6 + 6 <= size );
	return *reinterpret_cast<idVec6*>( p + index * 6 );
}

/*
========================
idVecX::SubVec6
========================
*/
ID_INLINE const idVec6& idVecX::SubVec6( int index ) const
{
	assert( index >= 0 && index * 6 + 6 <= size );
	return *reinterpret_cast<const idVec6*>( p + index * 6 );
}

/*
========================
idVecX::ToFloatPtr
========================
*/
ID_INLINE const float* idVecX::ToFloatPtr() const
{
	return p;
}

/*
========================
idVecX::ToFloatPtr
========================
*/
ID_INLINE float* idVecX::ToFloatPtr()
{
	return p;
}

/*
========================
idVecX::AddScaleAdd
========================
*/
ID_INLINE void idVecX::AddScaleAdd( const float scale, const idVecX& v0, const idVecX& v1 )
{
	assert( GetSize() == v0.GetSize() );
	assert( GetSize() == v1.GetSize() );

	const float* v0Ptr = v0.ToFloatPtr();
	const float* v1Ptr = v1.ToFloatPtr();
	float* dstPtr = ToFloatPtr();

	for( int i = 0; i < size; i++ )
	{
		dstPtr[i] += scale * ( v0Ptr[i] + v1Ptr[i] );
	}
}

#endif // !__MATH_VECTORX_H__
