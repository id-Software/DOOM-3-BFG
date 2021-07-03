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

#ifndef __MATH_INTERPOLATE_H__
#define __MATH_INTERPOLATE_H__

/*
==============================================================================================

	Linear interpolation.

==============================================================================================
*/

template< class type >
class idInterpolate
{
public:
	idInterpolate();

	void				Init( const int startTime, const int duration, const type& startValue, const type& endValue );
	void				SetStartTime( int time )
	{
		this->startTime = time;
	}
	void				SetDuration( int duration )
	{
		this->duration = duration;
	}
	void				SetStartValue( const type& startValue )
	{
		this->startValue = startValue;
	}
	void				SetEndValue( const type& endValue )
	{
		this->endValue = endValue;
	}

	type				GetCurrentValue( int time ) const;
	bool				IsDone( int time ) const
	{
		return ( time >= startTime + duration );
	}

	int					GetStartTime() const
	{
		return startTime;
	}
	int					GetEndTime() const
	{
		return startTime + duration;
	}
	int					GetDuration() const
	{
		return duration;
	}
	const type& 		GetStartValue() const
	{
		return startValue;
	}
	const type& 		GetEndValue() const
	{
		return endValue;
	}

private:
	int					startTime;
	int					duration;
	type				startValue;
	type				endValue;
};

/*
====================
idInterpolate::idInterpolate
====================
*/
template< class type >
ID_INLINE idInterpolate<type>::idInterpolate()
{
	startTime = duration = 0;
	memset( &startValue, 0, sizeof( startValue ) );
	memset( &endValue, 0, sizeof( endValue ) );
}

/*
====================
idInterpolate::Init
====================
*/
template< class type >
ID_INLINE void idInterpolate<type>::Init( const int startTime, const int duration, const type& startValue, const type& endValue )
{
	this->startTime = startTime;
	this->duration = duration;
	this->startValue = startValue;
	this->endValue = endValue;
}

/*
====================
idInterpolate::GetCurrentValue
====================
*/
template< class type >
ID_INLINE type idInterpolate<type>::GetCurrentValue( int time ) const
{
	if( time <= startTime )
	{
		return startValue;
	}
	else if( time >= startTime + duration )
	{
		return endValue;
	}
	else
	{
		const float deltaTime = time - startTime;
		const float f = deltaTime / ( float )duration;
		const type range = ( endValue - startValue );
		return startValue + ( range * f );
	}
}

/*
==============================================================================================

	Continuous interpolation with linear acceleration and deceleration phase.
	The velocity is continuous but the acceleration is not.

==============================================================================================
*/

template< class type >
class idInterpolateAccelDecelLinear
{
public:
	idInterpolateAccelDecelLinear();

	void				Init( const int startTime, const int accelTime, const int decelTime, const int duration, const type& startValue, const type& endValue );
	void				SetStartTime( int time )
	{
		startTime = time;
		Invalidate();
	}
	void				SetStartValue( const type& startValue )
	{
		this->startValue = startValue;
		Invalidate();
	}
	void				SetEndValue( const type& endValue )
	{
		this->endValue = endValue;
		Invalidate();
	}

	type				GetCurrentValue( int time ) const;
	type				GetCurrentSpeed( int time ) const;
	bool				IsDone( int time ) const
	{
		return ( time >= startTime + accelTime + linearTime + decelTime );
	}

	int					GetStartTime() const
	{
		return startTime;
	}
	int					GetEndTime() const
	{
		return startTime + accelTime + linearTime + decelTime;
	}
	int					GetDuration() const
	{
		return accelTime + linearTime + decelTime;
	}
	int					GetAcceleration() const
	{
		return accelTime;
	}
	int					GetDeceleration() const
	{
		return decelTime;
	}
	const type& 		GetStartValue() const
	{
		return startValue;
	}
	const type& 		GetEndValue() const
	{
		return endValue;
	}

private:
	int					startTime;
	int					accelTime;
	int					linearTime;
	int					decelTime;
	type				startValue;
	type				endValue;
	mutable idExtrapolate<type> extrapolate;

	void				Invalidate();
	void				SetPhase( int time ) const;
};

/*
====================
idInterpolateAccelDecelLinear::idInterpolateAccelDecelLinear
====================
*/
template< class type >
ID_INLINE idInterpolateAccelDecelLinear<type>::idInterpolateAccelDecelLinear()
{
	startTime = accelTime = linearTime = decelTime = 0;
	memset( &startValue, 0, sizeof( startValue ) );
	endValue = startValue;
}

/*
====================
idInterpolateAccelDecelLinear::Init
====================
*/
template< class type >
ID_INLINE void idInterpolateAccelDecelLinear<type>::Init( const int startTime, const int accelTime, const int decelTime, const int duration, const type& startValue, const type& endValue )
{
	this->startTime = startTime;
	this->accelTime = accelTime;
	this->decelTime = decelTime;
	this->startValue = startValue;
	this->endValue = endValue;

	if( duration <= 0 )
	{
		return;
	}

	if( this->accelTime + this->decelTime > duration )
	{
		this->accelTime = this->accelTime * duration / ( this->accelTime + this->decelTime );
		this->decelTime = duration - this->accelTime;
	}
	this->linearTime = duration - this->accelTime - this->decelTime;
	const type speed = ( endValue - startValue ) * ( 1000.0f / ( ( float ) this->linearTime + ( this->accelTime + this->decelTime ) * 0.5f ) );

	if( this->accelTime )
	{
		extrapolate.Init( startTime, this->accelTime, startValue, ( startValue - startValue ), speed, EXTRAPOLATION_ACCELLINEAR ); //-V501
	}
	else if( this->linearTime )
	{
		extrapolate.Init( startTime, this->linearTime, startValue, ( startValue - startValue ), speed, EXTRAPOLATION_LINEAR ); //-V501
	}
	else
	{
		extrapolate.Init( startTime, this->decelTime, startValue, ( startValue - startValue ), speed, EXTRAPOLATION_DECELLINEAR ); //-V501
	}
}

/*
====================
idInterpolateAccelDecelLinear::Invalidate
====================
*/
template< class type >
ID_INLINE void idInterpolateAccelDecelLinear<type>::Invalidate()
{
	extrapolate.Init( 0, 0, extrapolate.GetStartValue(), extrapolate.GetBaseSpeed(), extrapolate.GetSpeed(), EXTRAPOLATION_NONE );
}

/*
====================
idInterpolateAccelDecelLinear::SetPhase
====================
*/
template< class type >
ID_INLINE void idInterpolateAccelDecelLinear<type>::SetPhase( int time ) const
{
	const float deltaTime = time - startTime;
	if( deltaTime < accelTime )
	{
		if( extrapolate.GetExtrapolationType() != EXTRAPOLATION_ACCELLINEAR )
		{
			extrapolate.Init( startTime, accelTime, startValue, extrapolate.GetBaseSpeed(), extrapolate.GetSpeed(), EXTRAPOLATION_ACCELLINEAR );
		}
	}
	else if( deltaTime < accelTime + linearTime )
	{
		if( extrapolate.GetExtrapolationType() != EXTRAPOLATION_LINEAR )
		{
			extrapolate.Init( startTime + accelTime, linearTime, startValue + extrapolate.GetSpeed() * ( accelTime * 0.001f * 0.5f ), extrapolate.GetBaseSpeed(), extrapolate.GetSpeed(), EXTRAPOLATION_LINEAR );
		}
	}
	else
	{
		if( extrapolate.GetExtrapolationType() != EXTRAPOLATION_DECELLINEAR )
		{
			extrapolate.Init( startTime + accelTime + linearTime, decelTime, endValue - ( extrapolate.GetSpeed() * ( decelTime * 0.001f * 0.5f ) ), extrapolate.GetBaseSpeed(), extrapolate.GetSpeed(), EXTRAPOLATION_DECELLINEAR );
		}
	}
}

/*
====================
idInterpolateAccelDecelLinear::GetCurrentValue
====================
*/
template< class type >
ID_INLINE type idInterpolateAccelDecelLinear<type>::GetCurrentValue( int time ) const
{
	SetPhase( time );
	return extrapolate.GetCurrentValue( time );
}

/*
====================
idInterpolateAccelDecelLinear::GetCurrentSpeed
====================
*/
template< class type >
ID_INLINE type idInterpolateAccelDecelLinear<type>::GetCurrentSpeed( int time ) const
{
	SetPhase( time );
	return extrapolate.GetCurrentSpeed( time );
}


/*
==============================================================================================

	Continuous interpolation with sinusoidal acceleration and deceleration phase.
	Both the velocity and acceleration are continuous.

==============================================================================================
*/

template< class type >
class idInterpolateAccelDecelSine
{
public:
	idInterpolateAccelDecelSine();

	void				Init( const int startTime, const int accelTime, const int decelTime, const int duration, const type& startValue, const type& endValue );
	void				SetStartTime( int time )
	{
		startTime = time;
		Invalidate();
	}
	void				SetStartValue( const type& startValue )
	{
		this->startValue = startValue;
		Invalidate();
	}
	void				SetEndValue( const type& endValue )
	{
		this->endValue = endValue;
		Invalidate();
	}

	type				GetCurrentValue( int time ) const;
	type				GetCurrentSpeed( int time ) const;
	bool				IsDone( int time ) const
	{
		return ( time >= startTime + accelTime + linearTime + decelTime );
	}

	int					GetStartTime() const
	{
		return startTime;
	}
	int					GetEndTime() const
	{
		return startTime + accelTime + linearTime + decelTime;
	}
	int					GetDuration() const
	{
		return accelTime + linearTime + decelTime;
	}
	int					GetAcceleration() const
	{
		return accelTime;
	}
	int					GetDeceleration() const
	{
		return decelTime;
	}
	const type& 		GetStartValue() const
	{
		return startValue;
	}
	const type& 		GetEndValue() const
	{
		return endValue;
	}

private:
	int					startTime;
	int					accelTime;
	int					linearTime;
	int					decelTime;
	type				startValue;
	type				endValue;
	mutable idExtrapolate<type> extrapolate;

	void				Invalidate();
	void				SetPhase( int time ) const;
};

/*
====================
idInterpolateAccelDecelSine::idInterpolateAccelDecelSine
====================
*/
template< class type >
ID_INLINE idInterpolateAccelDecelSine<type>::idInterpolateAccelDecelSine()
{
	startTime = accelTime = linearTime = decelTime = 0;
	memset( &startValue, 0, sizeof( startValue ) );
	memset( &endValue, 0, sizeof( endValue ) );
}

/*
====================
idInterpolateAccelDecelSine::Init
====================
*/
template< class type >
ID_INLINE void idInterpolateAccelDecelSine<type>::Init( const int startTime, const int accelTime, const int decelTime, const int duration, const type& startValue, const type& endValue )
{
	this->startTime = startTime;
	this->accelTime = accelTime;
	this->decelTime = decelTime;
	this->startValue = startValue;
	this->endValue = endValue;

	if( duration <= 0 )
	{
		return;
	}

	if( this->accelTime + this->decelTime > duration )
	{
		this->accelTime = this->accelTime * duration / ( this->accelTime + this->decelTime );
		this->decelTime = duration - this->accelTime;
	}
	this->linearTime = duration - this->accelTime - this->decelTime;
	const type speed = ( endValue - startValue ) * ( 1000.0f / ( ( float ) this->linearTime + ( this->accelTime + this->decelTime ) * idMath::SQRT_1OVER2 ) );

	if( this->accelTime )
	{
		extrapolate.Init( startTime, this->accelTime, startValue, ( startValue - startValue ), speed, EXTRAPOLATION_ACCELSINE ); //-V501
	}
	else if( this->linearTime )
	{
		extrapolate.Init( startTime, this->linearTime, startValue, ( startValue - startValue ), speed, EXTRAPOLATION_LINEAR ); //-V501
	}
	else
	{
		extrapolate.Init( startTime, this->decelTime, startValue, ( startValue - startValue ), speed, EXTRAPOLATION_DECELSINE ); //-V501
	}
}

/*
====================
idInterpolateAccelDecelSine::Invalidate
====================
*/
template< class type >
ID_INLINE void idInterpolateAccelDecelSine<type>::Invalidate()
{
	extrapolate.Init( 0, 0, extrapolate.GetStartValue(), extrapolate.GetBaseSpeed(), extrapolate.GetSpeed(), EXTRAPOLATION_NONE );
}

/*
====================
idInterpolateAccelDecelSine::SetPhase
====================
*/
template< class type >
ID_INLINE void idInterpolateAccelDecelSine<type>::SetPhase( int time ) const
{
	const float deltaTime = time - startTime;
	if( deltaTime < accelTime )
	{
		if( extrapolate.GetExtrapolationType() != EXTRAPOLATION_ACCELSINE )
		{
			extrapolate.Init( startTime, accelTime, startValue, extrapolate.GetBaseSpeed(), extrapolate.GetSpeed(), EXTRAPOLATION_ACCELSINE );
		}
	}
	else if( deltaTime < accelTime + linearTime )
	{
		if( extrapolate.GetExtrapolationType() != EXTRAPOLATION_LINEAR )
		{
			extrapolate.Init( startTime + accelTime, linearTime, startValue + extrapolate.GetSpeed() * ( accelTime * 0.001f * idMath::SQRT_1OVER2 ), extrapolate.GetBaseSpeed(), extrapolate.GetSpeed(), EXTRAPOLATION_LINEAR );
		}
	}
	else
	{
		if( extrapolate.GetExtrapolationType() != EXTRAPOLATION_DECELSINE )
		{
			extrapolate.Init( startTime + accelTime + linearTime, decelTime, endValue - ( extrapolate.GetSpeed() * ( decelTime * 0.001f * idMath::SQRT_1OVER2 ) ), extrapolate.GetBaseSpeed(), extrapolate.GetSpeed(), EXTRAPOLATION_DECELSINE );
		}
	}
}

/*
====================
idInterpolateAccelDecelSine::GetCurrentValue
====================
*/
template< class type >
ID_INLINE type idInterpolateAccelDecelSine<type>::GetCurrentValue( int time ) const
{
	SetPhase( time );
	return extrapolate.GetCurrentValue( time );
}

/*
====================
idInterpolateAccelDecelSine::GetCurrentSpeed
====================
*/
template< class type >
ID_INLINE type idInterpolateAccelDecelSine<type>::GetCurrentSpeed( int time ) const
{
	SetPhase( time );
	return extrapolate.GetCurrentSpeed( time );
}

#endif /* !__MATH_INTERPOLATE_H__ */
