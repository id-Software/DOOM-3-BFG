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

#ifndef __TIMER_H__
#define __TIMER_H__

/*
===============================================================================

	Clock tick counter. Should only be used for profiling.

===============================================================================
*/

class idTimer
{
public:
	idTimer();
	idTimer( double clockTicks );
	~idTimer();

	idTimer			operator+( const idTimer& t ) const;
	idTimer			operator-( const idTimer& t ) const;
	idTimer& 		operator+=( const idTimer& t );
	idTimer& 		operator-=( const idTimer& t );

	void			Start();
	void			Stop();
	void			Clear();
	double			ClockTicks() const;
	double			Milliseconds() const;

private:
	static double	base;
	enum
	{
		TS_STARTED,
		TS_STOPPED
	} state;
	double			start;
	double			clockTicks;

	void			InitBaseClockTicks() const;
};

/*
=================
idTimer::idTimer
=================
*/
ID_INLINE idTimer::idTimer()
{
	state = TS_STOPPED;
	clockTicks = 0.0;
}

/*
=================
idTimer::idTimer
=================
*/
ID_INLINE idTimer::idTimer( double _clockTicks )
{
	state = TS_STOPPED;
	clockTicks = _clockTicks;
}

/*
=================
idTimer::~idTimer
=================
*/
ID_INLINE idTimer::~idTimer()
{
}

/*
=================
idTimer::operator+
=================
*/
ID_INLINE idTimer idTimer::operator+( const idTimer& t ) const
{
	assert( state == TS_STOPPED && t.state == TS_STOPPED );
	return idTimer( clockTicks + t.clockTicks );
}

/*
=================
idTimer::operator-
=================
*/
ID_INLINE idTimer idTimer::operator-( const idTimer& t ) const
{
	assert( state == TS_STOPPED && t.state == TS_STOPPED );
	return idTimer( clockTicks - t.clockTicks );
}

/*
=================
idTimer::operator+=
=================
*/
ID_INLINE idTimer& idTimer::operator+=( const idTimer& t )
{
	assert( state == TS_STOPPED && t.state == TS_STOPPED );
	clockTicks += t.clockTicks;
	return *this;
}

/*
=================
idTimer::operator-=
=================
*/
ID_INLINE idTimer& idTimer::operator-=( const idTimer& t )
{
	assert( state == TS_STOPPED && t.state == TS_STOPPED );
	clockTicks -= t.clockTicks;
	return *this;
}

/*
=================
idTimer::Start
=================
*/
ID_INLINE void idTimer::Start()
{
	assert( state == TS_STOPPED );
	state = TS_STARTED;
	start = idLib::sys->GetClockTicks();
}

/*
=================
idTimer::Stop
=================
*/
ID_INLINE void idTimer::Stop()
{
	assert( state == TS_STARTED );
	clockTicks += idLib::sys->GetClockTicks() - start;
	if( base < 0.0 )
	{
		InitBaseClockTicks();
	}
	if( clockTicks > base )
	{
		clockTicks -= base;
	}
	state = TS_STOPPED;
}

/*
=================
idTimer::Clear
=================
*/
ID_INLINE void idTimer::Clear()
{
	clockTicks = 0.0;
}

/*
=================
idTimer::ClockTicks
=================
*/
ID_INLINE double idTimer::ClockTicks() const
{
	assert( state == TS_STOPPED );
	return clockTicks;
}

/*
=================
idTimer::Milliseconds
=================
*/
ID_INLINE double idTimer::Milliseconds() const
{
	assert( state == TS_STOPPED );
	return clockTicks / ( idLib::sys->ClockTicksPerSecond() * 0.001 );
}


/*
===============================================================================

	Report of multiple named timers.

===============================================================================
*/

class idTimerReport
{
public:
	idTimerReport();
	~idTimerReport();

	void			SetReportName( const char* name );
	int				AddReport( const char* name );
	void			Clear();
	void			Reset();
	void			PrintReport();
	void			AddTime( const char* name, idTimer* time );

private:
	idList<idTimer*>timers;
	idStrList		names;
	idStr			reportName;
};

#endif /* !__TIMER_H__ */
