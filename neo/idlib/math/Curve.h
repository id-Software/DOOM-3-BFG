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

#ifndef __MATH_CURVE_H__
#define __MATH_CURVE_H__

/*
===============================================================================

	Curve base template.

===============================================================================
*/

template< class type >
class idCurve
{
public:
	idCurve();
	virtual				~idCurve();

	virtual int			AddValue( const float time, const type& value );
	virtual void		RemoveIndex( const int index )
	{
		values.RemoveIndex( index );
		times.RemoveIndex( index );
		changed = true;
	}
	virtual void		Clear()
	{
		values.Clear();
		times.Clear();
		currentIndex = -1;
		changed = true;
	}

	virtual type		GetCurrentValue( const float time ) const;
	virtual type		GetCurrentFirstDerivative( const float time ) const;
	virtual type		GetCurrentSecondDerivative( const float time ) const;

	virtual bool		IsDone( const float time ) const;

	int					GetNumValues() const
	{
		return values.Num();
	}
	void				SetValue( const int index, const type& value )
	{
		values[index] = value;
		changed = true;
	}
	type				GetValue( const int index ) const
	{
		return values[index];
	}
	type* 				GetValueAddress( const int index )
	{
		return &values[index];
	}
	float				GetTime( const int index ) const
	{
		return times[index];
	}

	float				GetLengthForTime( const float time ) const;
	float				GetTimeForLength( const float length, const float epsilon = 0.1f ) const;
	float				GetLengthBetweenKnots( const int i0, const int i1 ) const;

	void				MakeUniform( const float totalTime );
	void				SetConstantSpeed( const float totalTime );
	void				ShiftTime( const float deltaTime );
	void				Translate( const type& translation );

protected:

	idList<float>		times;			// knots
	idList<type>		values;			// knot values

	mutable int			currentIndex;	// cached index for fast lookup
	mutable bool		changed;		// set whenever the curve changes

	int					IndexForTime( const float time ) const;
	float				TimeForIndex( const int index ) const;
	type				ValueForIndex( const int index ) const;

	float				GetSpeed( const float time ) const;
	float				RombergIntegral( const float t0, const float t1, const int order ) const;
};

/*
====================
idCurve::idCurve
====================
*/
template< class type >
ID_INLINE idCurve<type>::idCurve()
{
	currentIndex = -1;
	changed = false;
}

/*
====================
idCurve::~idCurve
====================
*/
template< class type >
ID_INLINE idCurve<type>::~idCurve()
{
}

/*
====================
idCurve::AddValue

  add a timed/value pair to the spline
  returns the index to the inserted pair
====================
*/
template< class type >
ID_INLINE int idCurve<type>::AddValue( const float time, const type& value )
{
	int i;

	i = IndexForTime( time );
	times.Insert( time, i );
	values.Insert( value, i );
	changed = true;
	return i;
}

/*
====================
idCurve::GetCurrentValue

  get the value for the given time
====================
*/
template< class type >
ID_INLINE type idCurve<type>::GetCurrentValue( const float time ) const
{
	int i;

	i = IndexForTime( time );
	if( i >= values.Num() )
	{
		return values[values.Num() - 1];
	}
	else
	{
		return values[i];
	}
}

/*
====================
idCurve::GetCurrentFirstDerivative

  get the first derivative for the given time
====================
*/
template< class type >
ID_INLINE type idCurve<type>::GetCurrentFirstDerivative( const float time ) const
{
	return ( values[0] - values[0] ); //-V501
}

/*
====================
idCurve::GetCurrentSecondDerivative

  get the second derivative for the given time
====================
*/
template< class type >
ID_INLINE type idCurve<type>::GetCurrentSecondDerivative( const float time ) const
{
	return ( values[0] - values[0] ); //-V501
}

/*
====================
idCurve::IsDone
====================
*/
template< class type >
ID_INLINE bool idCurve<type>::IsDone( const float time ) const
{
	return ( time >= times[ times.Num() - 1 ] );
}

/*
====================
idCurve::GetSpeed
====================
*/
template< class type >
ID_INLINE float idCurve<type>::GetSpeed( const float time ) const
{
	int i;
	float speed;
	type value;

	value = GetCurrentFirstDerivative( time );
	for( speed = 0.0f, i = 0; i < value.GetDimension(); i++ )
	{
		speed += value[i] * value[i];
	}
	return idMath::Sqrt( speed );
}

/*
====================
idCurve::RombergIntegral
====================
*/
template< class type >
ID_INLINE float idCurve<type>::RombergIntegral( const float t0, const float t1, const int order ) const
{
	int i, j, k, m, n;
	float sum, delta;
	float* temp[2];

	temp[0] = ( float* ) _alloca16( order * sizeof( float ) );
	temp[1] = ( float* ) _alloca16( order * sizeof( float ) );

	delta = t1 - t0;
	temp[0][0] = 0.5f * delta * ( GetSpeed( t0 ) + GetSpeed( t1 ) );

	for( i = 2, m = 1; i <= order; i++, m *= 2, delta *= 0.5f )
	{

		// approximate using the trapezoid rule
		sum = 0.0f;
		for( j = 1; j <= m; j++ )
		{
			sum += GetSpeed( t0 + delta * ( j - 0.5f ) );
		}

		// Richardson extrapolation
		temp[1][0] = 0.5f * ( temp[0][0] + delta * sum );
		for( k = 1, n = 4; k < i; k++, n *= 4 )
		{
			temp[1][k] = ( n * temp[1][k - 1] - temp[0][k - 1] ) / ( n - 1 );
		}

		for( j = 0; j < i; j++ )
		{
			temp[0][j] = temp[1][j];
		}
	}
	return temp[0][order - 1];
}

/*
====================
idCurve::GetLengthBetweenKnots
====================
*/
template< class type >
ID_INLINE float idCurve<type>::GetLengthBetweenKnots( const int i0, const int i1 ) const
{
	float length = 0.0f;
	for( int i = i0; i < i1; i++ )
	{
		length += RombergIntegral( times[i], times[i + 1], 5 );
	}
	return length;
}

/*
====================
idCurve::GetLengthForTime
====================
*/
template< class type >
ID_INLINE float idCurve<type>::GetLengthForTime( const float time ) const
{
	float length = 0.0f;
	int index = IndexForTime( time );
	for( int i = 0; i < index; i++ )
	{
		length += RombergIntegral( times[i], times[i + 1], 5 );
	}
	length += RombergIntegral( times[index], time, 5 );
	return length;
}

/*
====================
idCurve::GetTimeForLength
====================
*/
template< class type >
ID_INLINE float idCurve<type>::GetTimeForLength( const float length, const float epsilon ) const
{
	int i, index;
	float* accumLength, totalLength, len0, len1, t, diff;

	if( length <= 0.0f )
	{
		return times[0];
	}

	accumLength = ( float* ) _alloca16( values.Num() * sizeof( float ) );
	totalLength = 0.0f;
	for( index = 0; index < values.Num() - 1; index++ )
	{
		totalLength += GetLengthBetweenKnots( index, index + 1 );
		accumLength[index] = totalLength;
		if( length < accumLength[index] )
		{
			break;
		}
	}

	if( index >= values.Num() - 1 )
	{
		return times[times.Num() - 1];
	}

	if( index == 0 )
	{
		len0 = length;
		len1 = accumLength[0];
	}
	else
	{
		len0 = length - accumLength[index - 1];
		len1 = accumLength[index] - accumLength[index - 1];
	}

	// invert the arc length integral using Newton's method
	t = ( times[index + 1] - times[index] ) * len0 / len1;
	for( i = 0; i < 32; i++ )
	{
		diff = RombergIntegral( times[index], times[index] + t, 5 ) - len0;
		if( idMath::Fabs( diff ) <= epsilon )
		{
			return times[index] + t;
		}
		t -= diff / GetSpeed( times[index] + t );
	}
	return times[index] + t;
}

/*
====================
idCurve::MakeUniform
====================
*/
template< class type >
ID_INLINE void idCurve<type>::MakeUniform( const float totalTime )
{
	int i, n;

	n = times.Num() - 1;
	for( i = 0; i <= n; i++ )
	{
		times[i] = i * totalTime / n;
	}
	changed = true;
}

/*
====================
idCurve::SetConstantSpeed
====================
*/
template< class type >
ID_INLINE void idCurve<type>::SetConstantSpeed( const float totalTime )
{
	int i, j;
	float* length, totalLength, scale, t;

	length = ( float* ) _alloca16( values.Num() * sizeof( float ) );
	totalLength = 0.0f;
	for( i = 0; i < values.Num() - 1; i++ )
	{
		length[i] = GetLengthBetweenKnots( i, i + 1 );
		totalLength += length[i];
	}
	scale = totalTime / totalLength;
	for( t = 0.0f, i = 0; i < times.Num() - 1; i++ )
	{
		times[i] = t;
		t += scale * length[i];
	}
	times[times.Num() - 1] = totalTime;
	changed = true;
}

/*
====================
idCurve::ShiftTime
====================
*/
template< class type >
ID_INLINE void idCurve<type>::ShiftTime( const float deltaTime )
{
	for( int i = 0; i < times.Num(); i++ )
	{
		times[i] += deltaTime;
	}
	changed = true;
}

/*
====================
idCurve::Translate
====================
*/
template< class type >
ID_INLINE void idCurve<type>::Translate( const type& translation )
{
	for( int i = 0; i < values.Num(); i++ )
	{
		values[i] += translation;
	}
	changed = true;
}

/*
====================
idCurve::IndexForTime

  find the index for the first time greater than or equal to the given time
====================
*/
template< class type >
ID_INLINE int idCurve<type>::IndexForTime( const float time ) const
{
	int len, mid, offset, res;

	if( currentIndex >= 0 && currentIndex <= times.Num() )
	{
		// use the cached index if it is still valid
		if( currentIndex == 0 )
		{
			if( time <= times[currentIndex] )
			{
				return currentIndex;
			}
		}
		else if( currentIndex == times.Num() )
		{
			if( time > times[currentIndex - 1] )
			{
				return currentIndex;
			}
		}
		else if( time > times[currentIndex - 1] && time <= times[currentIndex] )
		{
			return currentIndex;
		}
		else if( time > times[currentIndex] && ( currentIndex + 1 == times.Num() || time <= times[currentIndex + 1] ) )
		{
			// use the next index
			currentIndex++;
			return currentIndex;
		}
	}

	// use binary search to find the index for the given time
	len = times.Num();
	mid = len;
	offset = 0;
	res = 0;
	while( mid > 0 )
	{
		mid = len >> 1;
		if( time == times[offset + mid] )
		{
			return offset + mid;
		}
		else if( time > times[offset + mid] )
		{
			offset += mid;
			len -= mid;
			res = 1;
		}
		else
		{
			len -= mid;
			res = 0;
		}
	}
	currentIndex = offset + res;
	return currentIndex;
}

/*
====================
idCurve::ValueForIndex

  get the value for the given time
====================
*/
template< class type >
ID_INLINE type idCurve<type>::ValueForIndex( const int index ) const
{
	int n = values.Num() - 1;

	if( index < 0 )
	{
		return values[0] + index * ( values[1] - values[0] );
	}
	else if( index > n )
	{
		return values[n] + ( index - n ) * ( values[n] - values[n - 1] );
	}
	return values[index];
}

/*
====================
idCurve::TimeForIndex

  get the value for the given time
====================
*/
template< class type >
ID_INLINE float idCurve<type>::TimeForIndex( const int index ) const
{
	int n = times.Num() - 1;

	if( index < 0 )
	{
		return times[0] + index * ( times[1] - times[0] );
	}
	else if( index > n )
	{
		return times[n] + ( index - n ) * ( times[n] - times[n - 1] );
	}
	return times[index];
}


/*
===============================================================================

	Bezier Curve template.
	The degree of the polynomial equals the number of knots minus one.

===============================================================================
*/

template< class type >
class idCurve_Bezier : public idCurve<type>
{
public:
	idCurve_Bezier();

	virtual type		GetCurrentValue( const float time ) const;
	virtual type		GetCurrentFirstDerivative( const float time ) const;
	virtual type		GetCurrentSecondDerivative( const float time ) const;

protected:
	void				Basis( const int order, const float t, float* bvals ) const;
	void				BasisFirstDerivative( const int order, const float t, float* bvals ) const;
	void				BasisSecondDerivative( const int order, const float t, float* bvals ) const;
};

/*
====================
idCurve_Bezier::idCurve_Bezier
====================
*/
template< class type >
ID_INLINE idCurve_Bezier<type>::idCurve_Bezier()
{
}

/*
====================
idCurve_Bezier::GetCurrentValue

  get the value for the given time
====================
*/
template< class type >
ID_INLINE type idCurve_Bezier<type>::GetCurrentValue( const float time ) const
{
	int i;
	float* bvals;
	type v;

	bvals = ( float* ) _alloca16( this->values.Num() * sizeof( float ) );

	Basis( this->values.Num(), time, bvals );
	v = bvals[0] * this->values[0];
	for( i = 1; i < this->values.Num(); i++ )
	{
		v += bvals[i] * this->values[i];
	}
	return v;
}

/*
====================
idCurve_Bezier::GetCurrentFirstDerivative

  get the first derivative for the given time
====================
*/
template< class type >
ID_INLINE type idCurve_Bezier<type>::GetCurrentFirstDerivative( const float time ) const
{
	int i;
	float* bvals, d;
	type v;

	bvals = ( float* ) _alloca16( this->values.Num() * sizeof( float ) );

	BasisFirstDerivative( this->values.Num(), time, bvals );
	v = bvals[0] * this->values[0];
	for( i = 1; i < this->values.Num(); i++ )
	{
		v += bvals[i] * this->values[i];
	}
	d = ( this->times[this->times.Num() - 1] - this->times[0] );
	return ( ( float )( this->values.Num() - 1 ) / d ) * v;
}

/*
====================
idCurve_Bezier::GetCurrentSecondDerivative

  get the second derivative for the given time
====================
*/
template< class type >
ID_INLINE type idCurve_Bezier<type>::GetCurrentSecondDerivative( const float time ) const
{
	int i;
	float* bvals, d;
	type v;

	bvals = ( float* ) _alloca16( this->values.Num() * sizeof( float ) );

	BasisSecondDerivative( this->values.Num(), time, bvals );
	v = bvals[0] * this->values[0];
	for( i = 1; i < this->values.Num(); i++ )
	{
		v += bvals[i] * this->values[i];
	}
	d = ( this->times[this->times.Num() - 1] - this->times[0] );
	return ( ( float )( this->values.Num() - 2 ) * ( this->values.Num() - 1 ) / ( d * d ) ) * v;
}

/*
====================
idCurve_Bezier::Basis

  bezier basis functions
====================
*/
template< class type >
ID_INLINE void idCurve_Bezier<type>::Basis( const int order, const float t, float* bvals ) const
{
	int i, j, d;
	float* c, c1, c2, s, o, ps, po;

	bvals[0] = 1.0f;
	d = order - 1;
	if( d <= 0 )
	{
		return;
	}

	c = ( float* ) _alloca16( ( d + 1 ) * sizeof( float ) );
	s = ( float )( t - this->times[0] ) / ( this->times[this->times.Num() - 1] - this->times[0] );
	o = 1.0f - s;
	ps = s;
	po = o;

	for( i = 1; i < d; i++ )
	{
		c[i] = 1.0f;
	}
	for( i = 1; i < d; i++ )
	{
		c[i - 1] = 0.0f;
		c1 = c[i];
		c[i] = 1.0f;
		for( j = i + 1; j <= d; j++ )
		{
			c2 = c[j];
			c[j] = c1 + c[j - 1];
			c1 = c2;
		}
		bvals[i] = c[d] * ps;
		ps *= s;
	}
	for( i = d - 1; i >= 0; i-- )
	{
		bvals[i] *= po;
		po *= o;
	}
	bvals[d] = ps;
}

/*
====================
idCurve_Bezier::BasisFirstDerivative

  first derivative of bezier basis functions
====================
*/
template< class type >
ID_INLINE void idCurve_Bezier<type>::BasisFirstDerivative( const int order, const float t, float* bvals ) const
{
	int i;

	Basis( order - 1, t, bvals + 1 );
	bvals[0] = 0.0f;
	for( i = 0; i < order - 1; i++ )
	{
		bvals[i] -= bvals[i + 1];
	}
}

/*
====================
idCurve_Bezier::BasisSecondDerivative

  second derivative of bezier basis functions
====================
*/
template< class type >
ID_INLINE void idCurve_Bezier<type>::BasisSecondDerivative( const int order, const float t, float* bvals ) const
{
	int i;

	BasisFirstDerivative( order - 1, t, bvals + 1 );
	bvals[0] = 0.0f;
	for( i = 0; i < order - 1; i++ )
	{
		bvals[i] -= bvals[i + 1];
	}
}


/*
===============================================================================

	Quadratic Bezier Curve template.
	Should always have exactly three knots.

===============================================================================
*/

template< class type >
class idCurve_QuadraticBezier : public idCurve<type>
{

public:
	idCurve_QuadraticBezier();

	virtual type		GetCurrentValue( const float time ) const;
	virtual type		GetCurrentFirstDerivative( const float time ) const;
	virtual type		GetCurrentSecondDerivative( const float time ) const;

protected:
	void				Basis( const float t, float* bvals ) const;
	void				BasisFirstDerivative( const float t, float* bvals ) const;
	void				BasisSecondDerivative( const float t, float* bvals ) const;
};

/*
====================
idCurve_QuadraticBezier::idCurve_QuadraticBezier
====================
*/
template< class type >
ID_INLINE idCurve_QuadraticBezier<type>::idCurve_QuadraticBezier()
{
}


/*
====================
idCurve_QuadraticBezier::GetCurrentValue

  get the value for the given time
====================
*/
template< class type >
ID_INLINE type idCurve_QuadraticBezier<type>::GetCurrentValue( const float time ) const
{
	float bvals[3];
	assert( this->values.Num() == 3 );
	Basis( time, bvals );
	return ( bvals[0] * this->values[0] + bvals[1] * this->values[1] + bvals[2] * this->values[2] );
}

/*
====================
idCurve_QuadraticBezier::GetCurrentFirstDerivative

  get the first derivative for the given time
====================
*/
template< class type >
ID_INLINE type idCurve_QuadraticBezier<type>::GetCurrentFirstDerivative( const float time ) const
{
	float bvals[3], d;
	assert( this->values.Num() == 3 );
	BasisFirstDerivative( time, bvals );
	d = ( this->times[2] - this->times[0] );
	return ( bvals[0] * this->values[0] + bvals[1] * this->values[1] + bvals[2] * this->values[2] ) / d;
}

/*
====================
idCurve_QuadraticBezier::GetCurrentSecondDerivative

  get the second derivative for the given time
====================
*/
template< class type >
ID_INLINE type idCurve_QuadraticBezier<type>::GetCurrentSecondDerivative( const float time ) const
{
	float bvals[3], d;
	assert( this->values.Num() == 3 );
	BasisSecondDerivative( time, bvals );
	d = ( this->times[2] - this->times[0] );
	return ( bvals[0] * this->values[0] + bvals[1] * this->values[1] + bvals[2] * this->values[2] ) / ( d * d );
}

/*
====================
idCurve_QuadraticBezier::Basis

  quadratic bezier basis functions
====================
*/
template< class type >
ID_INLINE void idCurve_QuadraticBezier<type>::Basis( const float t, float* bvals ) const
{
	float s1 = ( float )( t - this->times[0] ) / ( this->times[2] - this->times[0] );
	float s2 = s1 * s1;
	bvals[0] = s2 - 2.0f * s1 + 1.0f;
	bvals[1] = -2.0f * s2 + 2.0f * s1;
	bvals[2] = s2;
}

/*
====================
idCurve_QuadraticBezier::BasisFirstDerivative

  first derivative of quadratic bezier basis functions
====================
*/
template< class type >
ID_INLINE void idCurve_QuadraticBezier<type>::BasisFirstDerivative( const float t, float* bvals ) const
{
	float s1 = ( float )( t - this->times[0] ) / ( this->times[2] - this->times[0] );
	bvals[0] = 2.0f * s1 - 2.0f;
	bvals[1] = -4.0f * s1 + 2.0f;
	bvals[2] = 2.0f * s1;
}

/*
====================
idCurve_QuadraticBezier::BasisSecondDerivative

  second derivative of quadratic bezier basis functions
====================
*/
template< class type >
ID_INLINE void idCurve_QuadraticBezier<type>::BasisSecondDerivative( const float t, float* bvals ) const
{
	float s1 = ( float )( t - this->times[0] ) / ( this->times[2] - this->times[0] );
	bvals[0] = 2.0f;
	bvals[1] = -4.0f;
	bvals[2] = 2.0f;
}


/*
===============================================================================

	Cubic Bezier Curve template.
	Should always have exactly four knots.

===============================================================================
*/

template< class type >
class idCurve_CubicBezier : public idCurve<type>
{

public:
	idCurve_CubicBezier();

	virtual type		GetCurrentValue( const float time ) const;
	virtual type		GetCurrentFirstDerivative( const float time ) const;
	virtual type		GetCurrentSecondDerivative( const float time ) const;

protected:
	void				Basis( const float t, float* bvals ) const;
	void				BasisFirstDerivative( const float t, float* bvals ) const;
	void				BasisSecondDerivative( const float t, float* bvals ) const;
};

/*
====================
idCurve_CubicBezier::idCurve_CubicBezier
====================
*/
template< class type >
ID_INLINE idCurve_CubicBezier<type>::idCurve_CubicBezier()
{
}


/*
====================
idCurve_CubicBezier::GetCurrentValue

  get the value for the given time
====================
*/
template< class type >
ID_INLINE type idCurve_CubicBezier<type>::GetCurrentValue( const float time ) const
{
	float bvals[4];
	assert( this->values.Num() == 4 );
	Basis( time, bvals );
	return ( bvals[0] * this->values[0] + bvals[1] * this->values[1] + bvals[2] * this->values[2] + bvals[3] * this->values[3] );
}

/*
====================
idCurve_CubicBezier::GetCurrentFirstDerivative

  get the first derivative for the given time
====================
*/
template< class type >
ID_INLINE type idCurve_CubicBezier<type>::GetCurrentFirstDerivative( const float time ) const
{
	float bvals[4], d;
	assert( this->values.Num() == 4 );
	BasisFirstDerivative( time, bvals );
	d = ( this->times[3] - this->times[0] );
	return ( bvals[0] * this->values[0] + bvals[1] * this->values[1] + bvals[2] * this->values[2] + bvals[3] * this->values[3] ) / d;
}

/*
====================
idCurve_CubicBezier::GetCurrentSecondDerivative

  get the second derivative for the given time
====================
*/
template< class type >
ID_INLINE type idCurve_CubicBezier<type>::GetCurrentSecondDerivative( const float time ) const
{
	float bvals[4], d;
	assert( this->values.Num() == 4 );
	BasisSecondDerivative( time, bvals );
	d = ( this->times[3] - this->times[0] );
	return ( bvals[0] * this->values[0] + bvals[1] * this->values[1] + bvals[2] * this->values[2] + bvals[3] * this->values[3] ) / ( d * d );
}

/*
====================
idCurve_CubicBezier::Basis

  cubic bezier basis functions
====================
*/
template< class type >
ID_INLINE void idCurve_CubicBezier<type>::Basis( const float t, float* bvals ) const
{
	float s1 = ( float )( t - this->times[0] ) / ( this->times[3] - this->times[0] );
	float s2 = s1 * s1;
	float s3 = s2 * s1;
	bvals[0] = -s3 + 3.0f * s2 - 3.0f * s1 + 1.0f;
	bvals[1] = 3.0f * s3 - 6.0f * s2 + 3.0f * s1;
	bvals[2] = -3.0f * s3 + 3.0f * s2;
	bvals[3] = s3;
}

/*
====================
idCurve_CubicBezier::BasisFirstDerivative

  first derivative of cubic bezier basis functions
====================
*/
template< class type >
ID_INLINE void idCurve_CubicBezier<type>::BasisFirstDerivative( const float t, float* bvals ) const
{
	float s1 = ( float )( t - this->times[0] ) / ( this->times[3] - this->times[0] );
	float s2 = s1 * s1;
	bvals[0] = -3.0f * s2 + 6.0f * s1 - 3.0f;
	bvals[1] = 9.0f * s2 - 12.0f * s1 + 3.0f;
	bvals[2] = -9.0f * s2 + 6.0f * s1;
	bvals[3] = 3.0f * s2;
}

/*
====================
idCurve_CubicBezier::BasisSecondDerivative

  second derivative of cubic bezier basis functions
====================
*/
template< class type >
ID_INLINE void idCurve_CubicBezier<type>::BasisSecondDerivative( const float t, float* bvals ) const
{
	float s1 = ( float )( t - this->times[0] ) / ( this->times[3] - this->times[0] );
	bvals[0] = -6.0f * s1 + 6.0f;
	bvals[1] = 18.0f * s1 - 12.0f;
	bvals[2] = -18.0f * s1 + 6.0f;
	bvals[3] = 6.0f * s1;
}


/*
===============================================================================

	Spline base template.

===============================================================================
*/

template< class type >
class idCurve_Spline : public idCurve<type>
{

public:
	enum				boundary_t { BT_FREE, BT_CLAMPED, BT_CLOSED };

	idCurve_Spline();

	virtual bool		IsDone( const float time ) const;

	virtual void		SetBoundaryType( const boundary_t bt )
	{
		boundaryType = bt;
		this->changed = true;
	}
	virtual boundary_t	GetBoundaryType() const
	{
		return boundaryType;
	}

	virtual void		SetCloseTime( const float t )
	{
		closeTime = t;
		this->changed = true;
	}
	virtual float		GetCloseTime()
	{
		return boundaryType == BT_CLOSED ? closeTime : 0.0f;
	}

protected:
	boundary_t			boundaryType;
	float				closeTime;

	type				ValueForIndex( const int index ) const;
	float				TimeForIndex( const int index ) const;
	float				ClampedTime( const float t ) const;
};

/*
====================
idCurve_Spline::idCurve_Spline
====================
*/
template< class type >
ID_INLINE idCurve_Spline<type>::idCurve_Spline()
{
	boundaryType = BT_FREE;
	closeTime = 0.0f;
}

/*
====================
idCurve_Spline::ValueForIndex

  get the value for the given time
====================
*/
template< class type >
ID_INLINE type idCurve_Spline<type>::ValueForIndex( const int index ) const
{
	int n = this->values.Num() - 1;

	if( index < 0 )
	{
		if( boundaryType == BT_CLOSED )
		{
			return this->values[ this->values.Num() + index % this->values.Num() ];
		}
		else
		{
			return this->values[0] + index * ( this->values[1] - this->values[0] );
		}
	}
	else if( index > n )
	{
		if( boundaryType == BT_CLOSED )
		{
			return this->values[ index % this->values.Num() ];
		}
		else
		{
			return this->values[n] + ( index - n ) * ( this->values[n] - this->values[n - 1] );
		}
	}
	return this->values[index];
}

/*
====================
idCurve_Spline::TimeForIndex

  get the value for the given time
====================
*/
template< class type >
ID_INLINE float idCurve_Spline<type>::TimeForIndex( const int index ) const
{
	int n = this->times.Num() - 1;

	if( index < 0 )
	{
		if( boundaryType == BT_CLOSED )
		{
			return ( index / this->times.Num() ) * ( this->times[n] + closeTime ) - ( this->times[n] + closeTime - this->times[this->times.Num() + index % this->times.Num()] );
		}
		else
		{
			return this->times[0] + index * ( this->times[1] - this->times[0] );
		}
	}
	else if( index > n )
	{
		if( boundaryType == BT_CLOSED )
		{
			return ( index / this->times.Num() ) * ( this->times[n] + closeTime ) + this->times[index % this->times.Num()];
		}
		else
		{
			return this->times[n] + ( index - n ) * ( this->times[n] - this->times[n - 1] );
		}
	}
	return this->times[index];
}

/*
====================
idCurve_Spline::ClampedTime

  return the clamped time based on the boundary type
====================
*/
template< class type >
ID_INLINE float idCurve_Spline<type>::ClampedTime( const float t ) const
{
	if( boundaryType == BT_CLAMPED )
	{
		if( t < this->times[0] )
		{
			return this->times[0];
		}
		else if( t >= this->times[this->times.Num() - 1] )
		{
			return this->times[this->times.Num() - 1];
		}
	}
	return t;
}

/*
====================
idCurve_Spline::IsDone
====================
*/
template< class type >
ID_INLINE bool idCurve_Spline<type>::IsDone( const float time ) const
{
	return ( boundaryType != BT_CLOSED && time >= this->times[ this->times.Num() - 1 ] );
}


/*
===============================================================================

	Cubic Interpolating Spline template.
	The curve goes through all the knots.

===============================================================================
*/

template< class type >
class idCurve_NaturalCubicSpline : public idCurve_Spline<type>
{
public:
	idCurve_NaturalCubicSpline();

	virtual void		Clear()
	{
		idCurve_Spline<type>::Clear();
		this->values.Clear();
		b.Clear();
		c.Clear();
		d.Clear();
	}

	virtual type		GetCurrentValue( const float time ) const;
	virtual type		GetCurrentFirstDerivative( const float time ) const;
	virtual type		GetCurrentSecondDerivative( const float time ) const;

protected:
	mutable idList<type>b;
	mutable idList<type>c;
	mutable idList<type>d;

	void				Setup() const;
	void				SetupFree() const;
	void				SetupClamped() const;
	void				SetupClosed() const;
};

/*
====================
idCurve_NaturalCubicSpline::idCurve_NaturalCubicSpline
====================
*/
template< class type >
ID_INLINE idCurve_NaturalCubicSpline<type>::idCurve_NaturalCubicSpline()
{
}

/*
====================
idCurve_NaturalCubicSpline::GetCurrentValue

  get the value for the given time
====================
*/
template< class type >
ID_INLINE type idCurve_NaturalCubicSpline<type>::GetCurrentValue( const float time ) const
{
	float clampedTime = this->ClampedTime( time );
	int i = this->IndexForTime( clampedTime );
	float s = time - this->TimeForIndex( i );
	Setup();
	return ( this->values[i] + s * ( b[i] + s * ( c[i] + s * d[i] ) ) );
}

/*
====================
idCurve_NaturalCubicSpline::GetCurrentFirstDerivative

  get the first derivative for the given time
====================
*/
template< class type >
ID_INLINE type idCurve_NaturalCubicSpline<type>::GetCurrentFirstDerivative( const float time ) const
{
	float clampedTime = this->ClampedTime( time );
	int i = this->IndexForTime( clampedTime );
	float s = time - this->TimeForIndex( i );
	Setup();
	return ( b[i] + s * ( 2.0f * c[i] + 3.0f * s * d[i] ) );
}

/*
====================
idCurve_NaturalCubicSpline::GetCurrentSecondDerivative

  get the second derivative for the given time
====================
*/
template< class type >
ID_INLINE type idCurve_NaturalCubicSpline<type>::GetCurrentSecondDerivative( const float time ) const
{
	float clampedTime = this->ClampedTime( time );
	int i = this->IndexForTime( clampedTime );
	float s = time - this->TimeForIndex( i );
	Setup();
	return ( 2.0f * c[i] + 6.0f * s * d[i] );
}

/*
====================
idCurve_NaturalCubicSpline::Setup
====================
*/
template< class type >
ID_INLINE void idCurve_NaturalCubicSpline<type>::Setup() const
{
	if( this->changed )
	{
		switch( this->boundaryType )
		{
			case idCurve_Spline<type>::BT_FREE:
				SetupFree();
				break;
			case idCurve_Spline<type>::BT_CLAMPED:
				SetupClamped();
				break;
			case idCurve_Spline<type>::BT_CLOSED:
				SetupClosed();
				break;
		}
		this->changed = false;
	}
}

/*
====================
idCurve_NaturalCubicSpline::SetupFree
====================
*/
template< class type >
ID_INLINE void idCurve_NaturalCubicSpline<type>::SetupFree() const
{
	int i;
	float inv;
	float* d0, *d1, *beta, *gamma;
	type* alpha, *delta;

	d0 = ( float* ) _alloca16( ( this->values.Num() - 1 ) * sizeof( float ) );
	d1 = ( float* ) _alloca16( ( this->values.Num() - 1 ) * sizeof( float ) );
	alpha = ( type* ) _alloca16( ( this->values.Num() - 1 ) * sizeof( type ) );
	beta = ( float* ) _alloca16( this->values.Num() * sizeof( float ) );
	gamma = ( float* ) _alloca16( ( this->values.Num() - 1 ) * sizeof( float ) );
	delta = ( type* ) _alloca16( this->values.Num() * sizeof( type ) );

	for( i = 0; i < this->values.Num() - 1; i++ )
	{
		d0[i] = this->times[i + 1] - this->times[i];
	}

	for( i = 1; i < this->values.Num() - 1; i++ )
	{
		d1[i] = this->times[i + 1] - this->times[i - 1];
	}

	for( i = 1; i < this->values.Num() - 1; i++ )
	{
		type sum = 3.0f * ( d0[i - 1] * this->values[i + 1] - d1[i] * this->values[i] + d0[i] * this->values[i - 1] );
		inv = 1.0f / ( d0[i - 1] * d0[i] );
		alpha[i] = inv * sum;
	}

	beta[0] = 1.0f;
	gamma[0] = 0.0f;
	delta[0] = this->values[0] - this->values[0]; //-V501

	for( i = 1; i < this->values.Num() - 1; i++ )
	{
		beta[i] = 2.0f * d1[i] - d0[i - 1] * gamma[i - 1];
		inv = 1.0f / beta[i];
		gamma[i] = inv * d0[i];
		delta[i] = inv * ( alpha[i] - d0[i - 1] * delta[i - 1] );
	}
	beta[this->values.Num() - 1] = 1.0f;
	delta[this->values.Num() - 1] = this->values[0] - this->values[0]; //-V501

	b.AssureSize( this->values.Num() );
	c.AssureSize( this->values.Num() );
	d.AssureSize( this->values.Num() );

	c[this->values.Num() - 1] = this->values[0] - this->values[0]; //-V501

	for( i = this->values.Num() - 2; i >= 0; i-- )
	{
		c[i] = delta[i] - gamma[i] * c[i + 1];
		inv = 1.0f / d0[i];
		b[i] = inv * ( this->values[i + 1] - this->values[i] ) - ( 1.0f / 3.0f ) * d0[i] * ( c[i + 1] + 2.0f * c[i] );
		d[i] = ( 1.0f / 3.0f ) * inv * ( c[i + 1] - c[i] );
	}
}

/*
====================
idCurve_NaturalCubicSpline::SetupClamped
====================
*/
template< class type >
ID_INLINE void idCurve_NaturalCubicSpline<type>::SetupClamped() const
{
	int i;
	float inv;
	float* d0, *d1, *beta, *gamma;
	type* alpha, *delta;

	d0 = ( float* ) _alloca16( ( this->values.Num() - 1 ) * sizeof( float ) );
	d1 = ( float* ) _alloca16( ( this->values.Num() - 1 ) * sizeof( float ) );
	alpha = ( type* ) _alloca16( ( this->values.Num() - 1 ) * sizeof( type ) );
	beta = ( float* ) _alloca16( this->values.Num() * sizeof( float ) );
	gamma = ( float* ) _alloca16( ( this->values.Num() - 1 ) * sizeof( float ) );
	delta = ( type* ) _alloca16( this->values.Num() * sizeof( type ) );

	for( i = 0; i < this->values.Num() - 1; i++ )
	{
		d0[i] = this->times[i + 1] - this->times[i];
	}

	for( i = 1; i < this->values.Num() - 1; i++ )
	{
		d1[i] = this->times[i + 1] - this->times[i - 1];
	}

	inv = 1.0f / d0[0];
	alpha[0] = 3.0f * ( inv - 1.0f ) * ( this->values[1] - this->values[0] );
	inv = 1.0f / d0[this->values.Num() - 2];
	alpha[this->values.Num() - 1] = 3.0f * ( 1.0f - inv ) * ( this->values[this->values.Num() - 1] - this->values[this->values.Num() - 2] );

	for( i = 1; i < this->values.Num() - 1; i++ )
	{
		type sum = 3.0f * ( d0[i - 1] * this->values[i + 1] - d1[i] * this->values[i] + d0[i] * this->values[i - 1] );
		inv = 1.0f / ( d0[i - 1] * d0[i] );
		alpha[i] = inv * sum;
	}

	beta[0] = 2.0f * d0[0];
	gamma[0] = 0.5f;
	inv = 1.0f / beta[0];
	delta[0] = inv * alpha[0];

	for( i = 1; i < this->values.Num() - 1; i++ )
	{
		beta[i] = 2.0f * d1[i] - d0[i - 1] * gamma[i - 1];
		inv = 1.0f / beta[i];
		gamma[i] = inv * d0[i];
		delta[i] = inv * ( alpha[i] - d0[i - 1] * delta[i - 1] );
	}

	beta[this->values.Num() - 1] = d0[this->values.Num() - 2] * ( 2.0f - gamma[this->values.Num() - 2] );
	inv = 1.0f / beta[this->values.Num() - 1];
	delta[this->values.Num() - 1] = inv * ( alpha[this->values.Num() - 1] - d0[this->values.Num() - 2] * delta[this->values.Num() - 2] );

	b.AssureSize( this->values.Num() );
	c.AssureSize( this->values.Num() );
	d.AssureSize( this->values.Num() );

	c[this->values.Num() - 1] = delta[this->values.Num() - 1];

	for( i = this->values.Num() - 2; i >= 0; i-- )
	{
		c[i] = delta[i] - gamma[i] * c[i + 1];
		inv = 1.0f / d0[i];
		b[i] = inv * ( this->values[i + 1] - this->values[i] ) - ( 1.0f / 3.0f ) * d0[i] * ( c[i + 1] + 2.0f * c[i] );
		d[i] = ( 1.0f / 3.0f ) * inv * ( c[i + 1] - c[i] );
	}
}

/*
====================
idCurve_NaturalCubicSpline::SetupClosed
====================
*/
template< class type >
ID_INLINE void idCurve_NaturalCubicSpline<type>::SetupClosed() const
{
	int i, j;
	float c0, c1;
	float* d0;
	idMatX mat;
	idVecX x;

	d0 = ( float* ) _alloca16( ( this->values.Num() - 1 ) * sizeof( float ) );
	x.SetData( this->values.Num(), VECX_ALLOCA( this->values.Num() ) );
	mat.SetData( this->values.Num(), this->values.Num(), MATX_ALLOCA( this->values.Num() * this->values.Num() ) );

	b.AssureSize( this->values.Num() );
	c.AssureSize( this->values.Num() );
	d.AssureSize( this->values.Num() );

	for( i = 0; i < this->values.Num() - 1; i++ )
	{
		d0[i] = this->times[i + 1] - this->times[i];
	}

	// matrix of system
	mat[0][0] = 1.0f;
	mat[0][this->values.Num() - 1] = -1.0f;
	for( i = 1; i <= this->values.Num() - 2; i++ )
	{
		mat[i][i - 1] = d0[i - 1];
		mat[i][i  ] = 2.0f * ( d0[i - 1] + d0[i] );
		mat[i][i + 1] = d0[i];
	}
	mat[this->values.Num() - 1][this->values.Num() - 2] = d0[this->values.Num() - 2];
	mat[this->values.Num() - 1][0] = 2.0f * ( d0[this->values.Num() - 2] + d0[0] );
	mat[this->values.Num() - 1][1] = d0[0];

	// right-hand side
	c[0].Zero();
	for( i = 1; i <= this->values.Num() - 2; i++ )
	{
		c0 = 1.0f / d0[i];
		c1 = 1.0f / d0[i - 1];
		c[i] = 3.0f * ( c0 * ( this->values[i + 1] - this->values[i] ) - c1 * ( this->values[i] - this->values[i - 1] ) );
	}
	c0 = 1.0f / d0[0];
	c1 = 1.0f / d0[this->values.Num() - 2];
	c[this->values.Num() - 1] = 3.0f * ( c0 * ( this->values[1] - this->values[0] ) - c1 * ( this->values[0] - this->values[this->values.Num() - 2] ) );

	// solve system for each dimension
	mat.LU_Factor( NULL );
	for( i = 0; i < this->values[0].GetDimension(); i++ )
	{
		for( j = 0; j < this->values.Num(); j++ )
		{
			x[j] = c[j][i];
		}
		mat.LU_Solve( x, x, NULL );
		for( j = 0; j < this->values.Num(); j++ )
		{
			c[j][i] = x[j];
		}
	}

	for( i = 0; i < this->values.Num() - 1; i++ )
	{
		c0 = 1.0f / d0[i];
		b[i] = c0 * ( this->values[i + 1] - this->values[i] ) - ( 1.0f / 3.0f ) * ( c[i + 1] + 2.0f * c[i] ) * d0[i];
		d[i] = ( 1.0f / 3.0f ) * c0 * ( c[i + 1] - c[i] );
	}
}


/*
===============================================================================

	Uniform Cubic Interpolating Spline template.
	The curve goes through all the knots.

===============================================================================
*/

template< class type >
class idCurve_CatmullRomSpline : public idCurve_Spline<type>
{

public:
	idCurve_CatmullRomSpline();

	virtual type		GetCurrentValue( const float time ) const;
	virtual type		GetCurrentFirstDerivative( const float time ) const;
	virtual type		GetCurrentSecondDerivative( const float time ) const;

protected:
	void				Basis( const int index, const float t, float* bvals ) const;
	void				BasisFirstDerivative( const int index, const float t, float* bvals ) const;
	void				BasisSecondDerivative( const int index, const float t, float* bvals ) const;
};

/*
====================
idCurve_CatmullRomSpline::idCurve_CatmullRomSpline
====================
*/
template< class type >
ID_INLINE idCurve_CatmullRomSpline<type>::idCurve_CatmullRomSpline()
{
}

/*
====================
idCurve_CatmullRomSpline::GetCurrentValue

  get the value for the given time
====================
*/
template< class type >
ID_INLINE type idCurve_CatmullRomSpline<type>::GetCurrentValue( const float time ) const
{
	int i, j, k;
	float bvals[4], clampedTime;
	type v;

	if( this->times.Num() == 1 )
	{
		return this->values[0];
	}

	clampedTime = this->ClampedTime( time );
	i = this->IndexForTime( clampedTime );
	Basis( i - 1, clampedTime, bvals );
	v = this->values[0] - this->values[0]; //-V501
	for( j = 0; j < 4; j++ )
	{
		k = i + j - 2;
		v += bvals[j] * this->ValueForIndex( k );
	}
	return v;
}

/*
====================
idCurve_CatmullRomSpline::GetCurrentFirstDerivative

  get the first derivative for the given time
====================
*/
template< class type >
ID_INLINE type idCurve_CatmullRomSpline<type>::GetCurrentFirstDerivative( const float time ) const
{
	int i, j, k;
	float bvals[4], d, clampedTime;
	type v;

	if( this->times.Num() == 1 )
	{
		return ( this->values[0] - this->values[0] ); //-V501
	}

	clampedTime = this->ClampedTime( time );
	i = this->IndexForTime( clampedTime );
	BasisFirstDerivative( i - 1, clampedTime, bvals );
	v = this->values[0] - this->values[0]; //-V501
	for( j = 0; j < 4; j++ )
	{
		k = i + j - 2;
		v += bvals[j] * this->ValueForIndex( k );
	}
	d = ( this->TimeForIndex( i ) - this->TimeForIndex( i - 1 ) );
	return v / d;
}

/*
====================
idCurve_CatmullRomSpline::GetCurrentSecondDerivative

  get the second derivative for the given time
====================
*/
template< class type >
ID_INLINE type idCurve_CatmullRomSpline<type>::GetCurrentSecondDerivative( const float time ) const
{
	int i, j, k;
	float bvals[4], d, clampedTime;
	type v;

	if( this->times.Num() == 1 )
	{
		return ( this->values[0] - this->values[0] ); //-V501
	}

	clampedTime = this->ClampedTime( time );
	i = this->IndexForTime( clampedTime );
	BasisSecondDerivative( i - 1, clampedTime, bvals );
	v = this->values[0] - this->values[0]; //-V501
	for( j = 0; j < 4; j++ )
	{
		k = i + j - 2;
		v += bvals[j] * this->ValueForIndex( k );
	}
	d = ( this->TimeForIndex( i ) - this->TimeForIndex( i - 1 ) );
	return v / ( d * d );
}

/*
====================
idCurve_CatmullRomSpline::Basis

  spline basis functions
====================
*/
template< class type >
ID_INLINE void idCurve_CatmullRomSpline<type>::Basis( const int index, const float t, float* bvals ) const
{
	float s = ( float )( t - this->TimeForIndex( index ) ) / ( this->TimeForIndex( index + 1 ) - this->TimeForIndex( index ) );
	bvals[0] = ( ( -s + 2.0f ) * s - 1.0f ) * s * 0.5f;				// -0.5f s * s * s + s * s - 0.5f * s
	bvals[1] = ( ( ( 3.0f * s - 5.0f ) * s ) * s + 2.0f ) * 0.5f;	// 1.5f * s * s * s - 2.5f * s * s + 1.0f
	bvals[2] = ( ( -3.0f * s + 4.0f ) * s + 1.0f ) * s * 0.5f;		// -1.5f * s * s * s - 2.0f * s * s + 0.5f s
	bvals[3] = ( ( s - 1.0f ) * s * s ) * 0.5f;						// 0.5f * s * s * s - 0.5f * s * s
}

/*
====================
idCurve_CatmullRomSpline::BasisFirstDerivative

  first derivative of spline basis functions
====================
*/
template< class type >
ID_INLINE void idCurve_CatmullRomSpline<type>::BasisFirstDerivative( const int index, const float t, float* bvals ) const
{
	float s = ( float )( t - this->TimeForIndex( index ) ) / ( this->TimeForIndex( index + 1 ) - this->TimeForIndex( index ) );
	bvals[0] = ( -1.5f * s + 2.0f ) * s - 0.5f;						// -1.5f * s * s + 2.0f * s - 0.5f
	bvals[1] = ( 4.5f * s - 5.0f ) * s;								// 4.5f * s * s - 5.0f * s
	bvals[2] = ( -4.5 * s + 4.0f ) * s + 0.5f;						// -4.5 * s * s + 4.0f * s + 0.5f
	bvals[3] = 1.5f * s * s - s;									// 1.5f * s * s - s
}

/*
====================
idCurve_CatmullRomSpline::BasisSecondDerivative

  second derivative of spline basis functions
====================
*/
template< class type >
ID_INLINE void idCurve_CatmullRomSpline<type>::BasisSecondDerivative( const int index, const float t, float* bvals ) const
{
	float s = ( float )( t - this->TimeForIndex( index ) ) / ( this->TimeForIndex( index + 1 ) - this->TimeForIndex( index ) );
	bvals[0] = -3.0f * s + 2.0f;
	bvals[1] = 9.0f * s - 5.0f;
	bvals[2] = -9.0f * s + 4.0f;
	bvals[3] = 3.0f * s - 1.0f;
}


/*
===============================================================================

	Cubic Interpolating Spline template.
	The curve goes through all the knots.
	The curve becomes the Catmull-Rom spline if the tension,
	continuity and bias are all set to zero.

===============================================================================
*/

template< class type >
class idCurve_KochanekBartelsSpline : public idCurve_Spline<type>
{

public:
	idCurve_KochanekBartelsSpline();

	virtual int			AddValue( const float time, const type& value );
	virtual int			AddValue( const float time, const type& value, const float tension, const float continuity, const float bias );
	virtual void		RemoveIndex( const int index )
	{
		this->values.RemoveIndex( index );
		this->times.RemoveIndex( index );
		tension.RemoveIndex( index );
		continuity.RemoveIndex( index );
		bias.RemoveIndex( index );
	}
	virtual void		Clear()
	{
		this->values.Clear();
		this->times.Clear();
		tension.Clear();
		continuity.Clear();
		bias.Clear();
		this->currentIndex = -1;
	}

	virtual type		GetCurrentValue( const float time ) const;
	virtual type		GetCurrentFirstDerivative( const float time ) const;
	virtual type		GetCurrentSecondDerivative( const float time ) const;

protected:
	idList<float>		tension;
	idList<float>		continuity;
	idList<float>		bias;

	void				TangentsForIndex( const int index, type& t0, type& t1 ) const;

	void				Basis( const int index, const float t, float* bvals ) const;
	void				BasisFirstDerivative( const int index, const float t, float* bvals ) const;
	void				BasisSecondDerivative( const int index, const float t, float* bvals ) const;
};

/*
====================
idCurve_KochanekBartelsSpline::idCurve_KochanekBartelsSpline
====================
*/
template< class type >
ID_INLINE idCurve_KochanekBartelsSpline<type>::idCurve_KochanekBartelsSpline()
{
}

/*
====================
idCurve_KochanekBartelsSpline::AddValue

  add a timed/value pair to the spline
  returns the index to the inserted pair
====================
*/
template< class type >
ID_INLINE int idCurve_KochanekBartelsSpline<type>::AddValue( const float time, const type& value )
{
	int i;

	i = this->IndexForTime( time );
	this->times.Insert( time, i );
	this->values.Insert( value, i );
	tension.Insert( 0.0f, i );
	continuity.Insert( 0.0f, i );
	bias.Insert( 0.0f, i );
	return i;
}

/*
====================
idCurve_KochanekBartelsSpline::AddValue

  add a timed/value pair to the spline
  returns the index to the inserted pair
====================
*/
template< class type >
ID_INLINE int idCurve_KochanekBartelsSpline<type>::AddValue( const float time, const type& value, const float tension, const float continuity, const float bias )
{
	int i;

	i = this->IndexForTime( time );
	this->times.Insert( time, i );
	this->values.Insert( value, i );
	this->tension.Insert( tension, i );
	this->continuity.Insert( continuity, i );
	this->bias.Insert( bias, i );
	return i;
}

/*
====================
idCurve_KochanekBartelsSpline::GetCurrentValue

  get the value for the given time
====================
*/
template< class type >
ID_INLINE type idCurve_KochanekBartelsSpline<type>::GetCurrentValue( const float time ) const
{
	int i;
	float bvals[4], clampedTime;
	type v, t0, t1;

	if( this->times.Num() == 1 )
	{
		return this->values[0];
	}

	clampedTime = this->ClampedTime( time );
	i = this->IndexForTime( clampedTime );
	TangentsForIndex( i - 1, t0, t1 );
	Basis( i - 1, clampedTime, bvals );
	v = bvals[0] * this->ValueForIndex( i - 1 );
	v += bvals[1] * this->ValueForIndex( i );
	v += bvals[2] * t0;
	v += bvals[3] * t1;
	return v;
}

/*
====================
idCurve_KochanekBartelsSpline::GetCurrentFirstDerivative

  get the first derivative for the given time
====================
*/
template< class type >
ID_INLINE type idCurve_KochanekBartelsSpline<type>::GetCurrentFirstDerivative( const float time ) const
{
	int i;
	float bvals[4], d, clampedTime;
	type v, t0, t1;

	if( this->times.Num() == 1 )
	{
		return ( this->values[0] - this->values[0] ); //-V501
	}

	clampedTime = this->ClampedTime( time );
	i = this->IndexForTime( clampedTime );
	TangentsForIndex( i - 1, t0, t1 );
	BasisFirstDerivative( i - 1, clampedTime, bvals );
	v = bvals[0] * this->ValueForIndex( i - 1 );
	v += bvals[1] * this->ValueForIndex( i );
	v += bvals[2] * t0;
	v += bvals[3] * t1;
	d = ( this->TimeForIndex( i ) - this->TimeForIndex( i - 1 ) );
	return v / d;
}

/*
====================
idCurve_KochanekBartelsSpline::GetCurrentSecondDerivative

  get the second derivative for the given time
====================
*/
template< class type >
ID_INLINE type idCurve_KochanekBartelsSpline<type>::GetCurrentSecondDerivative( const float time ) const
{
	int i;
	float bvals[4], d, clampedTime;
	type v, t0, t1;

	if( this->times.Num() == 1 )
	{
		return ( this->values[0] - this->values[0] ); //-V501
	}

	clampedTime = this->ClampedTime( time );
	i = this->IndexForTime( clampedTime );
	TangentsForIndex( i - 1, t0, t1 );
	BasisSecondDerivative( i - 1, clampedTime, bvals );
	v = bvals[0] * this->ValueForIndex( i - 1 );
	v += bvals[1] * this->ValueForIndex( i );
	v += bvals[2] * t0;
	v += bvals[3] * t1;
	d = ( this->TimeForIndex( i ) - this->TimeForIndex( i - 1 ) );
	return v / ( d * d );
}

/*
====================
idCurve_KochanekBartelsSpline::TangentsForIndex
====================
*/
template< class type >
ID_INLINE void idCurve_KochanekBartelsSpline<type>::TangentsForIndex( const int index, type& t0, type& t1 ) const
{
	float dt, omt, omc, opc, omb, opb, adj, s0, s1;
	type delta;

	delta = this->ValueForIndex( index + 1 ) - this->ValueForIndex( index );
	dt = this->TimeForIndex( index + 1 ) - this->TimeForIndex( index );

	omt = 1.0f - tension[index];
	omc = 1.0f - continuity[index];
	opc = 1.0f + continuity[index];
	omb = 1.0f - bias[index];
	opb = 1.0f + bias[index];
	adj = 2.0f * dt / ( this->TimeForIndex( index + 1 ) - this->TimeForIndex( index - 1 ) );
	s0 = 0.5f * adj * omt * opc * opb;
	s1 = 0.5f * adj * omt * omc * omb;

	// outgoing tangent at first point
	t0 = s1 * delta + s0 * ( this->ValueForIndex( index ) - this->ValueForIndex( index - 1 ) );

	omt = 1.0f - tension[index + 1];
	omc = 1.0f - continuity[index + 1];
	opc = 1.0f + continuity[index + 1];
	omb = 1.0f - bias[index + 1];
	opb = 1.0f + bias[index + 1];
	adj = 2.0f * dt / ( this->TimeForIndex( index + 2 ) - this->TimeForIndex( index ) );
	s0 = 0.5f * adj * omt * omc * opb;
	s1 = 0.5f * adj * omt * opc * omb;

	// incoming tangent at second point
	t1 = s1 * ( this->ValueForIndex( index + 2 ) - this->ValueForIndex( index + 1 ) ) + s0 * delta;
}

/*
====================
idCurve_KochanekBartelsSpline::Basis

  spline basis functions
====================
*/
template< class type >
ID_INLINE void idCurve_KochanekBartelsSpline<type>::Basis( const int index, const float t, float* bvals ) const
{
	float s = ( float )( t - this->TimeForIndex( index ) ) / ( this->TimeForIndex( index + 1 ) - this->TimeForIndex( index ) );
	bvals[0] = ( ( 2.0f * s - 3.0f ) * s ) * s + 1.0f;				// 2.0f * s * s * s - 3.0f * s * s + 1.0f
	bvals[1] = ( ( -2.0f * s + 3.0f ) * s ) * s;					// -2.0f * s * s * s + 3.0f * s * s
	bvals[2] = ( ( s - 2.0f ) * s ) * s + s;						// s * s * s - 2.0f * s * s + s
	bvals[3] = ( ( s - 1.0f ) * s ) * s;							// s * s * s - s * s
}

/*
====================
idCurve_KochanekBartelsSpline::BasisFirstDerivative

  first derivative of spline basis functions
====================
*/
template< class type >
ID_INLINE void idCurve_KochanekBartelsSpline<type>::BasisFirstDerivative( const int index, const float t, float* bvals ) const
{
	float s = ( float )( t - this->TimeForIndex( index ) ) / ( this->TimeForIndex( index + 1 ) - this->TimeForIndex( index ) );
	bvals[0] = ( 6.0f * s - 6.0f ) * s;								// 6.0f * s * s - 6.0f * s
	bvals[1] = ( -6.0f * s + 6.0f ) * s;							// -6.0f * s * s + 6.0f * s
	bvals[2] = ( 3.0f * s - 4.0f ) * s + 1.0f;						// 3.0f * s * s - 4.0f * s + 1.0f
	bvals[3] = ( 3.0f * s - 2.0f ) * s;								// 3.0f * s * s - 2.0f * s
}

/*
====================
idCurve_KochanekBartelsSpline::BasisSecondDerivative

  second derivative of spline basis functions
====================
*/
template< class type >
ID_INLINE void idCurve_KochanekBartelsSpline<type>::BasisSecondDerivative( const int index, const float t, float* bvals ) const
{
	float s = ( float )( t - this->TimeForIndex( index ) ) / ( this->TimeForIndex( index + 1 ) - this->TimeForIndex( index ) );
	bvals[0] = 12.0f * s - 6.0f;
	bvals[1] = -12.0f * s + 6.0f;
	bvals[2] = 6.0f * s - 4.0f;
	bvals[3] = 6.0f * s - 2.0f;
}


/*
===============================================================================

	B-Spline base template. Uses recursive definition and is slow.
	Use idCurve_UniformCubicBSpline or idCurve_NonUniformBSpline instead.

===============================================================================
*/

template< class type >
class idCurve_BSpline : public idCurve_Spline<type>
{

public:
	idCurve_BSpline();

	virtual int			GetOrder() const
	{
		return order;
	}
	virtual void		SetOrder( const int i )
	{
		assert( i > 0 && i < 10 );
		order = i;
	}

	virtual type		GetCurrentValue( const float time ) const;
	virtual type		GetCurrentFirstDerivative( const float time ) const;
	virtual type		GetCurrentSecondDerivative( const float time ) const;

protected:
	int					order;

	float				Basis( const int index, const int order, const float t ) const;
	float				BasisFirstDerivative( const int index, const int order, const float t ) const;
	float				BasisSecondDerivative( const int index, const int order, const float t ) const;
};

/*
====================
idCurve_BSpline::idCurve_NaturalCubicSpline
====================
*/
template< class type >
ID_INLINE idCurve_BSpline<type>::idCurve_BSpline()
{
	order = 4;	// default to cubic
}

/*
====================
idCurve_BSpline::GetCurrentValue

  get the value for the given time
====================
*/
template< class type >
ID_INLINE type idCurve_BSpline<type>::GetCurrentValue( const float time ) const
{
	int i, j, k;
	float clampedTime;
	type v;

	if( this->times.Num() == 1 )
	{
		return this->values[0];
	}

	clampedTime = this->ClampedTime( time );
	i = this->IndexForTime( clampedTime );
	v = this->values[0] - this->values[0]; //-V501
	for( j = 0; j < order; j++ )
	{
		k = i + j - ( order >> 1 );
		v += Basis( k - 2, order, clampedTime ) * this->ValueForIndex( k );
	}
	return v;
}

/*
====================
idCurve_BSpline::GetCurrentFirstDerivative

  get the first derivative for the given time
====================
*/
template< class type >
ID_INLINE type idCurve_BSpline<type>::GetCurrentFirstDerivative( const float time ) const
{
	int i, j, k;
	float clampedTime;
	type v;

	if( this->times.Num() == 1 )
	{
		return this->values[0];
	}

	clampedTime = this->ClampedTime( time );
	i = this->IndexForTime( clampedTime );
	v = this->values[0] - this->values[0]; //-V501
	for( j = 0; j < order; j++ )
	{
		k = i + j - ( order >> 1 );
		v += BasisFirstDerivative( k - 2, order, clampedTime ) * this->ValueForIndex( k );
	}
	return v;
}

/*
====================
idCurve_BSpline::GetCurrentSecondDerivative

  get the second derivative for the given time
====================
*/
template< class type >
ID_INLINE type idCurve_BSpline<type>::GetCurrentSecondDerivative( const float time ) const
{
	int i, j, k;
	float clampedTime;
	type v;

	if( this->times.Num() == 1 )
	{
		return this->values[0];
	}

	clampedTime = this->ClampedTime( time );
	i = this->IndexForTime( clampedTime );
	v = this->values[0] - this->values[0]; //-V501
	for( j = 0; j < order; j++ )
	{
		k = i + j - ( order >> 1 );
		v += BasisSecondDerivative( k - 2, order, clampedTime ) * this->ValueForIndex( k );
	}
	return v;
}

/*
====================
idCurve_BSpline::Basis

  spline basis function
====================
*/
template< class type >
ID_INLINE float idCurve_BSpline<type>::Basis( const int index, const int order, const float t ) const
{
	if( order <= 1 )
	{
		if( this->TimeForIndex( index ) < t && t <= this->TimeForIndex( index + 1 ) )
		{
			return 1.0f;
		}
		else
		{
			return 0.0f;
		}
	}
	else
	{
		float sum = 0.0f;
		float d1 = this->TimeForIndex( index + order - 1 ) - this->TimeForIndex( index );
		if( d1 != 0.0f )
		{
			sum += ( float )( t - this->TimeForIndex( index ) ) * Basis( index, order - 1, t ) / d1;
		}

		float d2 = this->TimeForIndex( index + order ) - this->TimeForIndex( index + 1 );
		if( d2 != 0.0f )
		{
			sum += ( float )( this->TimeForIndex( index + order ) - t ) * Basis( index + 1, order - 1, t ) / d2;
		}
		return sum;
	}
}

/*
====================
idCurve_BSpline::BasisFirstDerivative

  first derivative of spline basis function
====================
*/
template< class type >
ID_INLINE float idCurve_BSpline<type>::BasisFirstDerivative( const int index, const int order, const float t ) const
{
	return ( Basis( index, order - 1, t ) - Basis( index + 1, order - 1, t ) ) *
		   ( float )( order - 1 ) / ( this->TimeForIndex( index + ( order - 1 ) - 2 ) - this->TimeForIndex( index - 2 ) );
}

/*
====================
idCurve_BSpline::BasisSecondDerivative

  second derivative of spline basis function
====================
*/
template< class type >
ID_INLINE float idCurve_BSpline<type>::BasisSecondDerivative( const int index, const int order, const float t ) const
{
	return ( BasisFirstDerivative( index, order - 1, t ) - BasisFirstDerivative( index + 1, order - 1, t ) ) *
		   ( float )( order - 1 ) / ( this->TimeForIndex( index + ( order - 1 ) - 2 ) - this->TimeForIndex( index - 2 ) );
}


/*
===============================================================================

	Uniform Non-Rational Cubic B-Spline template.

===============================================================================
*/

template< class type >
class idCurve_UniformCubicBSpline : public idCurve_BSpline<type>
{

public:
	idCurve_UniformCubicBSpline();

	virtual type		GetCurrentValue( const float time ) const;
	virtual type		GetCurrentFirstDerivative( const float time ) const;
	virtual type		GetCurrentSecondDerivative( const float time ) const;

protected:
	void				Basis( const int index, const float t, float* bvals ) const;
	void				BasisFirstDerivative( const int index, const float t, float* bvals ) const;
	void				BasisSecondDerivative( const int index, const float t, float* bvals ) const;
};

/*
====================
idCurve_UniformCubicBSpline::idCurve_UniformCubicBSpline
====================
*/
template< class type >
ID_INLINE idCurve_UniformCubicBSpline<type>::idCurve_UniformCubicBSpline()
{
	this->order = 4;	// always cubic
}

/*
====================
idCurve_UniformCubicBSpline::GetCurrentValue

  get the value for the given time
====================
*/
template< class type >
ID_INLINE type idCurve_UniformCubicBSpline<type>::GetCurrentValue( const float time ) const
{
	int i, j, k;
	float bvals[4], clampedTime;
	type v;

	if( this->times.Num() == 1 )
	{
		return this->values[0];
	}

	clampedTime = this->ClampedTime( time );
	i = this->IndexForTime( clampedTime );
	Basis( i - 1, clampedTime, bvals );
	v = this->values[0] - this->values[0]; //-V501
	for( j = 0; j < 4; j++ )
	{
		k = i + j - 2;
		v += bvals[j] * this->ValueForIndex( k );
	}
	return v;
}

/*
====================
idCurve_UniformCubicBSpline::GetCurrentFirstDerivative

  get the first derivative for the given time
====================
*/
template< class type >
ID_INLINE type idCurve_UniformCubicBSpline<type>::GetCurrentFirstDerivative( const float time ) const
{
	int i, j, k;
	float bvals[4], d, clampedTime;
	type v;

	if( this->times.Num() == 1 )
	{
		return ( this->values[0] - this->values[0] ); //-V501
	}

	clampedTime = this->ClampedTime( time );
	i = this->IndexForTime( clampedTime );
	BasisFirstDerivative( i - 1, clampedTime, bvals );
	v = this->values[0] - this->values[0]; //-V501
	for( j = 0; j < 4; j++ )
	{
		k = i + j - 2;
		v += bvals[j] * this->ValueForIndex( k );
	}
	d = ( this->TimeForIndex( i ) - this->TimeForIndex( i - 1 ) );
	return v / d;
}

/*
====================
idCurve_UniformCubicBSpline::GetCurrentSecondDerivative

  get the second derivative for the given time
====================
*/
template< class type >
ID_INLINE type idCurve_UniformCubicBSpline<type>::GetCurrentSecondDerivative( const float time ) const
{
	int i, j, k;
	float bvals[4], d, clampedTime;
	type v;

	if( this->times.Num() == 1 )
	{
		return ( this->values[0] - this->values[0] ); //-V501
	}

	clampedTime = this->ClampedTime( time );
	i = this->IndexForTime( clampedTime );
	BasisSecondDerivative( i - 1, clampedTime, bvals );
	v = this->values[0] - this->values[0]; //-V501
	for( j = 0; j < 4; j++ )
	{
		k = i + j - 2;
		v += bvals[j] * this->ValueForIndex( k );
	}
	d = ( this->TimeForIndex( i ) - this->TimeForIndex( i - 1 ) );
	return v / ( d * d );
}

/*
====================
idCurve_UniformCubicBSpline::Basis

  spline basis functions
====================
*/
template< class type >
ID_INLINE void idCurve_UniformCubicBSpline<type>::Basis( const int index, const float t, float* bvals ) const
{
	float s = ( float )( t - this->TimeForIndex( index ) ) / ( this->TimeForIndex( index + 1 ) - this->TimeForIndex( index ) );
	bvals[0] = ( ( ( -s + 3.0f ) * s - 3.0f ) * s + 1.0f ) * ( 1.0f / 6.0f );
	bvals[1] = ( ( ( 3.0f * s - 6.0f ) * s ) * s + 4.0f ) * ( 1.0f / 6.0f );
	bvals[2] = ( ( ( -3.0f * s + 3.0f ) * s + 3.0f ) * s + 1.0f ) * ( 1.0f / 6.0f );
	bvals[3] = ( s * s * s ) * ( 1.0f / 6.0f );
}

/*
====================
idCurve_UniformCubicBSpline::BasisFirstDerivative

  first derivative of spline basis functions
====================
*/
template< class type >
ID_INLINE void idCurve_UniformCubicBSpline<type>::BasisFirstDerivative( const int index, const float t, float* bvals ) const
{
	float s = ( float )( t - this->TimeForIndex( index ) ) / ( this->TimeForIndex( index + 1 ) - this->TimeForIndex( index ) );
	bvals[0] = -0.5f * s * s + s - 0.5f;
	bvals[1] = 1.5f * s * s - 2.0f * s;
	bvals[2] = -1.5f * s * s + s + 0.5f;
	bvals[3] = 0.5f * s * s;
}

/*
====================
idCurve_UniformCubicBSpline::BasisSecondDerivative

  second derivative of spline basis functions
====================
*/
template< class type >
ID_INLINE void idCurve_UniformCubicBSpline<type>::BasisSecondDerivative( const int index, const float t, float* bvals ) const
{
	float s = ( float )( t - this->TimeForIndex( index ) ) / ( this->TimeForIndex( index + 1 ) - this->TimeForIndex( index ) );
	bvals[0] = -s + 1.0f;
	bvals[1] = 3.0f * s - 2.0f;
	bvals[2] = -3.0f * s + 1.0f;
	bvals[3] = s;
}


/*
===============================================================================

	Non-Uniform Non-Rational B-Spline (NUBS) template.

===============================================================================
*/

template< class type >
class idCurve_NonUniformBSpline : public idCurve_BSpline<type>
{

public:
	idCurve_NonUniformBSpline();

	virtual type		GetCurrentValue( const float time ) const;
	virtual type		GetCurrentFirstDerivative( const float time ) const;
	virtual type		GetCurrentSecondDerivative( const float time ) const;

protected:
	void				Basis( const int index, const int order, const float t, float* bvals ) const;
	void				BasisFirstDerivative( const int index, const int order, const float t, float* bvals ) const;
	void				BasisSecondDerivative( const int index, const int order, const float t, float* bvals ) const;
};

/*
====================
idCurve_NonUniformBSpline::idCurve_NonUniformBSpline
====================
*/
template< class type >
ID_INLINE idCurve_NonUniformBSpline<type>::idCurve_NonUniformBSpline()
{
}

/*
====================
idCurve_NonUniformBSpline::GetCurrentValue

  get the value for the given time
====================
*/
template< class type >
ID_INLINE type idCurve_NonUniformBSpline<type>::GetCurrentValue( const float time ) const
{
	int i, j, k;
	float clampedTime;
	type v;
	float* bvals = ( float* ) _alloca16( this->order * sizeof( float ) );

	if( this->times.Num() == 1 )
	{
		return this->values[0];
	}

	clampedTime = this->ClampedTime( time );
	i = this->IndexForTime( clampedTime );
	Basis( i - 1, this->order, clampedTime, bvals );
	v = this->values[0] - this->values[0]; //-V501
	for( j = 0; j < this->order; j++ )
	{
		k = i + j - ( this->order >> 1 );
		v += bvals[j] * this->ValueForIndex( k );
	}
	return v;
}

/*
====================
idCurve_NonUniformBSpline::GetCurrentFirstDerivative

  get the first derivative for the given time
====================
*/
template< class type >
ID_INLINE type idCurve_NonUniformBSpline<type>::GetCurrentFirstDerivative( const float time ) const
{
	int i, j, k;
	float clampedTime;
	type v;
	float* bvals = ( float* ) _alloca16( this->order * sizeof( float ) );

	if( this->times.Num() == 1 )
	{
		return ( this->values[0] - this->values[0] ); //-V501
	}

	clampedTime = this->ClampedTime( time );
	i = this->IndexForTime( clampedTime );
	BasisFirstDerivative( i - 1, this->order, clampedTime, bvals );
	v = this->values[0] - this->values[0]; //-V501
	for( j = 0; j < this->order; j++ )
	{
		k = i + j - ( this->order >> 1 );
		v += bvals[j] * this->ValueForIndex( k );
	}
	return v;
}

/*
====================
idCurve_NonUniformBSpline::GetCurrentSecondDerivative

  get the second derivative for the given time
====================
*/
template< class type >
ID_INLINE type idCurve_NonUniformBSpline<type>::GetCurrentSecondDerivative( const float time ) const
{
	int i, j, k;
	float clampedTime;
	type v;
	float* bvals = ( float* ) _alloca16( this->order * sizeof( float ) );

	if( this->times.Num() == 1 )
	{
		return ( this->values[0] - this->values[0] ); //-V501
	}

	clampedTime = this->ClampedTime( time );
	i = this->IndexForTime( clampedTime );
	BasisSecondDerivative( i - 1, this->order, clampedTime, bvals );
	v = this->values[0] - this->values[0]; //-V501
	for( j = 0; j < this->order; j++ )
	{
		k = i + j - ( this->order >> 1 );
		v += bvals[j] * this->ValueForIndex( k );
	}
	return v;
}

/*
====================
idCurve_NonUniformBSpline::Basis

  spline basis functions
====================
*/
template< class type >
ID_INLINE void idCurve_NonUniformBSpline<type>::Basis( const int index, const int order, const float t, float* bvals ) const
{
	int r, s, i;
	float omega;

	bvals[order - 1] = 1.0f;
	for( r = 2; r <= order; r++ )
	{
		i = index - r + 1;
		bvals[order - r] = 0.0f;
		for( s = order - r + 1; s < order; s++ )
		{
			i++;
			omega = ( float )( t - this->TimeForIndex( i ) ) / ( this->TimeForIndex( i + r - 1 ) - this->TimeForIndex( i ) );
			bvals[s - 1] += ( 1.0f - omega ) * bvals[s];
			bvals[s] *= omega;
		}
	}
}

/*
====================
idCurve_NonUniformBSpline::BasisFirstDerivative

  first derivative of spline basis functions
====================
*/
template< class type >
ID_INLINE void idCurve_NonUniformBSpline<type>::BasisFirstDerivative( const int index, const int order, const float t, float* bvals ) const
{
	int i;

	Basis( index, order - 1, t, bvals + 1 );
	bvals[0] = 0.0f;
	for( i = 0; i < order - 1; i++ )
	{
		bvals[i] -= bvals[i + 1];
		bvals[i] *= ( float )( order - 1 ) / ( this->TimeForIndex( index + i + ( order - 1 ) - 2 ) - this->TimeForIndex( index + i - 2 ) );
	}
	bvals[i] *= ( float )( order - 1 ) / ( this->TimeForIndex( index + i + ( order - 1 ) - 2 ) - this->TimeForIndex( index + i - 2 ) );
}

/*
====================
idCurve_NonUniformBSpline::BasisSecondDerivative

  second derivative of spline basis functions
====================
*/
template< class type >
ID_INLINE void idCurve_NonUniformBSpline<type>::BasisSecondDerivative( const int index, const int order, const float t, float* bvals ) const
{
	int i;

	BasisFirstDerivative( index, order - 1, t, bvals + 1 );
	bvals[0] = 0.0f;
	for( i = 0; i < order - 1; i++ )
	{
		bvals[i] -= bvals[i + 1];
		bvals[i] *= ( float )( order - 1 ) / ( this->TimeForIndex( index + i + ( order - 1 ) - 2 ) - this->TimeForIndex( index + i - 2 ) );
	}
	bvals[i] *= ( float )( order - 1 ) / ( this->TimeForIndex( index + i + ( order - 1 ) - 2 ) - this->TimeForIndex( index + i - 2 ) );
}


/*
===============================================================================

	Non-Uniform Rational B-Spline (NURBS) template.

===============================================================================
*/

template< class type >
class idCurve_NURBS : public idCurve_NonUniformBSpline<type>
{

public:
	idCurve_NURBS();

	virtual int			AddValue( const float time, const type& value );
	virtual int			AddValue( const float time, const type& value, const float weight );
	virtual void		RemoveIndex( const int index )
	{
		this->values.RemoveIndex( index );
		this->times.RemoveIndex( index );
		weights.RemoveIndex( index );
	}
	virtual void		Clear()
	{
		this->values.Clear();
		this->times.Clear();
		weights.Clear();
		this->currentIndex = -1;
	}

	virtual type		GetCurrentValue( const float time ) const;
	virtual type		GetCurrentFirstDerivative( const float time ) const;
	virtual type		GetCurrentSecondDerivative( const float time ) const;

protected:
	idList<float>		weights;

	float				WeightForIndex( const int index ) const;
};

/*
====================
idCurve_NURBS::idCurve_NURBS
====================
*/
template< class type >
ID_INLINE idCurve_NURBS<type>::idCurve_NURBS()
{
}

/*
====================
idCurve_NURBS::AddValue

  add a timed/value pair to the spline
  returns the index to the inserted pair
====================
*/
template< class type >
ID_INLINE int idCurve_NURBS<type>::AddValue( const float time, const type& value )
{
	int i;

	i = this->IndexForTime( time );
	this->times.Insert( time, i );
	this->values.Insert( value, i );
	weights.Insert( 1.0f, i );
	return i;
}

/*
====================
idCurve_NURBS::AddValue

  add a timed/value pair to the spline
  returns the index to the inserted pair
====================
*/
template< class type >
ID_INLINE int idCurve_NURBS<type>::AddValue( const float time, const type& value, const float weight )
{
	int i;

	i = this->IndexForTime( time );
	this->times.Insert( time, i );
	this->values.Insert( value, i );
	weights.Insert( weight, i );
	return i;
}

/*
====================
idCurve_NURBS::GetCurrentValue

  get the value for the given time
====================
*/
template< class type >
ID_INLINE type idCurve_NURBS<type>::GetCurrentValue( const float time ) const
{
	int i, j, k;
	float w, b, *bvals, clampedTime;
	type v;

	if( this->times.Num() == 1 )
	{
		return this->values[0];
	}

	bvals = ( float* ) _alloca16( this->order * sizeof( float ) );

	clampedTime = this->ClampedTime( time );
	i = this->IndexForTime( clampedTime );
	this->Basis( i - 1, this->order, clampedTime, bvals );
	v = this->values[0] - this->values[0]; //-V501
	w = 0.0f;
	for( j = 0; j < this->order; j++ )
	{
		k = i + j - ( this->order >> 1 );
		b = bvals[j] * WeightForIndex( k );
		w += b;
		v += b * this->ValueForIndex( k );
	}
	return v / w;
}

/*
====================
idCurve_NURBS::GetCurrentFirstDerivative

  get the first derivative for the given time
====================
*/
template< class type >
ID_INLINE type idCurve_NURBS<type>::GetCurrentFirstDerivative( const float time ) const
{
	int i, j, k;
	float w, wb, wd1, b, d1, *bvals, *d1vals, clampedTime;
	type v, vb, vd1;

	if( this->times.Num() == 1 )
	{
		return this->values[0];
	}

	bvals = ( float* ) _alloca16( this->order * sizeof( float ) );
	d1vals = ( float* ) _alloca16( this->order * sizeof( float ) );

	clampedTime = this->ClampedTime( time );
	i = this->IndexForTime( clampedTime );
	this->Basis( i - 1, this->order, clampedTime, bvals );
	this->BasisFirstDerivative( i - 1, this->order, clampedTime, d1vals );
	vb = vd1 = this->values[0] - this->values[0]; //-V501
	wb = wd1 = 0.0f;
	for( j = 0; j < this->order; j++ )
	{
		k = i + j - ( this->order >> 1 );
		w = WeightForIndex( k );
		b = bvals[j] * w;
		d1 = d1vals[j] * w;
		wb += b;
		wd1 += d1;
		v = this->ValueForIndex( k );
		vb += b * v;
		vd1 += d1 * v;
	}
	return ( wb * vd1 - vb * wd1 ) / ( wb * wb );
}

/*
====================
idCurve_NURBS::GetCurrentSecondDerivative

  get the second derivative for the given time
====================
*/
template< class type >
ID_INLINE type idCurve_NURBS<type>::GetCurrentSecondDerivative( const float time ) const
{
	int i, j, k;
	float w, wb, wd1, wd2, b, d1, d2, *bvals, *d1vals, *d2vals, clampedTime;
	type v, vb, vd1, vd2;

	if( this->times.Num() == 1 )
	{
		return this->values[0];
	}

	bvals = ( float* ) _alloca16( this->order * sizeof( float ) );
	d1vals = ( float* ) _alloca16( this->order * sizeof( float ) );
	d2vals = ( float* ) _alloca16( this->order * sizeof( float ) );

	clampedTime = this->ClampedTime( time );
	i = this->IndexForTime( clampedTime );
	this->Basis( i - 1, this->order, clampedTime, bvals );
	this->BasisFirstDerivative( i - 1, this->order, clampedTime, d1vals );
	this->BasisSecondDerivative( i - 1, this->order, clampedTime, d2vals );
	vb = vd1 = vd2 = this->values[0] - this->values[0]; //-V501
	wb = wd1 = wd2 = 0.0f;
	for( j = 0; j < this->order; j++ )
	{
		k = i + j - ( this->order >> 1 );
		w = WeightForIndex( k );
		b = bvals[j] * w;
		d1 = d1vals[j] * w;
		d2 = d2vals[j] * w;
		wb += b;
		wd1 += d1;
		wd2 += d2;
		v = this->ValueForIndex( k );
		vb += b * v;
		vd1 += d1 * v;
		vd2 += d2 * v;
	}
	return ( ( wb * wb ) * ( wb * vd2 - vb * wd2 ) - ( wb * vd1 - vb * wd1 ) * 2.0f * wb * wd1 ) / ( wb * wb * wb * wb );
}

/*
====================
idCurve_NURBS::WeightForIndex

  get the weight for the given index
====================
*/
template< class type >
ID_INLINE float idCurve_NURBS<type>::WeightForIndex( const int index ) const
{
	int n = weights.Num() - 1;

	if( index < 0 )
	{
		if( this->boundaryType == idCurve_Spline<type>::BT_CLOSED )
		{
			return weights[ weights.Num() + index % weights.Num() ];
		}
		else
		{
			return weights[0] + index * ( weights[1] - weights[0] );
		}
	}
	else if( index > n )
	{
		if( this->boundaryType == idCurve_Spline<type>::BT_CLOSED )
		{
			return weights[ index % weights.Num() ];
		}
		else
		{
			return weights[n] + ( index - n ) * ( weights[n] - weights[n - 1] );
		}
	}
	return weights[index];
}

#endif /* !__MATH_CURVE_H__ */
