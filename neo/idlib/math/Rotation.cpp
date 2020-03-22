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

#pragma hdrstop
#include "precompiled.h"

/*
============
idRotation::ToAngles
============
*/
idAngles idRotation::ToAngles() const
{
	return ToMat3().ToAngles();
}

/*
============
idRotation::ToQuat
============
*/
idQuat idRotation::ToQuat() const
{
	float a, s, c;

	a = angle * ( idMath::M_DEG2RAD * 0.5f );
	idMath::SinCos( a, s, c );
	return idQuat( vec.x * s, vec.y * s, vec.z * s, c );
}

/*
============
idRotation::toMat3
============
*/
const idMat3& idRotation::ToMat3() const
{
	float wx, wy, wz;
	float xx, yy, yz;
	float xy, xz, zz;
	float x2, y2, z2;
	float a, c, s, x, y, z;

	if( axisValid )
	{
		return axis;
	}

	a = angle * ( idMath::M_DEG2RAD * 0.5f );
	idMath::SinCos( a, s, c );

	x = vec[0] * s;
	y = vec[1] * s;
	z = vec[2] * s;

	x2 = x + x;
	y2 = y + y;
	z2 = z + z;

	xx = x * x2;
	xy = x * y2;
	xz = x * z2;

	yy = y * y2;
	yz = y * z2;
	zz = z * z2;

	wx = c * x2;
	wy = c * y2;
	wz = c * z2;

	axis[ 0 ][ 0 ] = 1.0f - ( yy + zz );
	axis[ 0 ][ 1 ] = xy - wz;
	axis[ 0 ][ 2 ] = xz + wy;

	axis[ 1 ][ 0 ] = xy + wz;
	axis[ 1 ][ 1 ] = 1.0f - ( xx + zz );
	axis[ 1 ][ 2 ] = yz - wx;

	axis[ 2 ][ 0 ] = xz - wy;
	axis[ 2 ][ 1 ] = yz + wx;
	axis[ 2 ][ 2 ] = 1.0f - ( xx + yy );

	axisValid = true;

	return axis;
}

/*
============
idRotation::ToMat4
============
*/
idMat4 idRotation::ToMat4() const
{
	return ToMat3().ToMat4();
}

/*
============
idRotation::ToAngularVelocity
============
*/
idVec3 idRotation::ToAngularVelocity() const
{
	return vec * DEG2RAD( angle );
}

/*
============
idRotation::Normalize180
============
*/
void idRotation::Normalize180()
{
	angle -= floor( angle / 360.0f ) * 360.0f;
	if( angle > 180.0f )
	{
		angle -= 360.0f;
	}
	else if( angle < -180.0f )
	{
		angle += 360.0f;
	}
}

/*
============
idRotation::Normalize360
============
*/
void idRotation::Normalize360()
{
	angle -= floor( angle / 360.0f ) * 360.0f;
	if( angle > 360.0f )
	{
		angle -= 360.0f;
	}
	else if( angle < 0.0f )
	{
		angle += 360.0f;
	}
}
