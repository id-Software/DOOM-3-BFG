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

#ifndef __MATH_ODE_H__
#define __MATH_ODE_H__

/*
===============================================================================

	Numerical solvers for ordinary differential equations.

===============================================================================
*/


//===============================================================
//
//	idODE
//
//===============================================================

typedef void ( *deriveFunction_t )( const float t, const void* userData, const float* state, float* derivatives );

class idODE
{

public:
	virtual				~idODE() {}

	virtual float		Evaluate( const float* state, float* newState, float t0, float t1 ) = 0;

protected:
	int					dimension;		// dimension in floats allocated for
	deriveFunction_t	derive;			// derive function
	const void* 		userData;		// client data
};

//===============================================================
//
//	idODE_Euler
//
//===============================================================

class idODE_Euler : public idODE
{

public:
	idODE_Euler( const int dim, const deriveFunction_t dr, const void* ud );
	virtual				~idODE_Euler();

	virtual float		Evaluate( const float* state, float* newState, float t0, float t1 );

protected:
	float* 				derivatives;	// space to store derivatives
};

//===============================================================
//
//	idODE_Midpoint
//
//===============================================================

class idODE_Midpoint : public idODE
{

public:
	idODE_Midpoint( const int dim, const deriveFunction_t dr, const void* ud );
	virtual				~idODE_Midpoint();

	virtual float		Evaluate( const float* state, float* newState, float t0, float t1 );

protected:
	float* 				tmpState;
	float* 				derivatives;	// space to store derivatives
};

//===============================================================
//
//	idODE_RK4
//
//===============================================================

class idODE_RK4 : public idODE
{

public:
	idODE_RK4( const int dim, const deriveFunction_t dr, const void* ud );
	virtual				~idODE_RK4();

	virtual float		Evaluate( const float* state, float* newState, float t0, float t1 );

protected:
	float* 				tmpState;
	float* 				d1;				// derivatives
	float* 				d2;
	float* 				d3;
	float* 				d4;
};

//===============================================================
//
//	idODE_RK4Adaptive
//
//===============================================================

class idODE_RK4Adaptive : public idODE
{

public:
	idODE_RK4Adaptive( const int dim, const deriveFunction_t dr, const void* ud );
	virtual				~idODE_RK4Adaptive();

	virtual float		Evaluate( const float* state, float* newState, float t0, float t1 );
	void				SetMaxError( const float err );

protected:
	float				maxError;		// maximum allowed error
	float* 				tmpState;
	float* 				d1;				// derivatives
	float* 				d1half;
	float* 				d2;
	float* 				d3;
	float* 				d4;
};

#endif /* !__MATH_ODE_H__ */
