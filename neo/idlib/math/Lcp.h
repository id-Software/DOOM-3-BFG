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
#ifndef __MATH_LCP_H__
#define __MATH_LCP_H__

/*
================================================
The *LCP* class, idLCP, is a Box-Constrained Mixed Linear Complementarity Problem solver.

'A' is a matrix of dimension n*n and 'x', 'b', 'lo', 'hi' are vectors of dimension n.

Solve: Ax = b + t, where t is a vector of dimension n, with complementarity condition:

	(x[i] - lo[i]) * (x[i] - hi[i]) * t[i] = 0

such that for each 0 <= i < n one of the following holds:

	lo[i] < x[i] < hi[i], t[i] == 0
	x[i] == lo[i], t[i] >= 0
	x[i] == hi[i], t[i] <= 0

Partly-bounded or unbounded variables can have lo[i] and/or hi[i] set to negative/positive
idMath::INFITITY, respectively.

If boxIndex != NULL and boxIndex[i] != -1, then

	lo[i] = - fabs( lo[i] * x[boxIndex[i]] )
	hi[i] = fabs( hi[i] * x[boxIndex[i]] )
	boxIndex[boxIndex[i]] must be -1

Before calculating any of the bounded x[i] with boxIndex[i] != -1, the solver calculates all
unbounded x[i] and all x[i] with boxIndex[i] == -1.
================================================
*/
class idLCP
{
public:
	static idLCP* 	AllocSquare();		// 'A' must be a square matrix
	static idLCP* 	AllocSymmetric();	// 'A' must be a symmetric matrix

	virtual			~idLCP();

	virtual bool	Solve( const idMatX& A, idVecX& x, const idVecX& b, const idVecX& lo,
						   const idVecX& hi, const int* boxIndex = NULL ) = 0;

	virtual void	SetMaxIterations( int max );
	virtual int		GetMaxIterations();

	static void		Test_f( const idCmdArgs& args );

protected:
	int				maxIterations;
};

#endif // !__MATH_LCP_H__
