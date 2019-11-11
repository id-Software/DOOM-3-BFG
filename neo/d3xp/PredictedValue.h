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
#ifndef PREDICTED_VALUE_H_
#define PREDICTED_VALUE_H_

#include "Game_local.h"

/*
================================================
A simple class to handle simple predictable values
on multiplayer clients.

The class encapsulates the actual value to be stored
as well as the client frame number on which it is set.

When reading predicted values from a snapshot, the actual
value is only updated if the server has processed the client's
usercmd for the frame in which the client predicted the value.
Got that?
================================================
*/
template< class type_ >
class idPredictedValue
{
public:
	explicit	idPredictedValue();
	explicit	idPredictedValue( const type_ & value_ );

	void		Set( const type_ & newValue );

	idPredictedValue< type_ >& operator=( const type_ & value );

	idPredictedValue< type_ >& operator+=( const type_ & toAdd );
	idPredictedValue< type_ >& operator-=( const type_ & toSubtract );

	bool		UpdateFromSnapshot( const type_ & valueFromSnapshot, int clientNumber );

	type_		Get() const
	{
		return value;
	}

private:
	// Noncopyable
	idPredictedValue( const idPredictedValue< type_ >& other );
	idPredictedValue< type_ >& operator=( const idPredictedValue< type_ >& other );

	type_		value;
	int			clientPredictedMilliseconds;	// The time in which the client predicted the value.

	void		UpdatePredictionTime();
};




#endif
