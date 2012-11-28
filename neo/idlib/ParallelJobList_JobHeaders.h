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
#ifndef __PARALLELJOBLIST_JOBHEADERS_H__
#define __PARALLELJOBLIST_JOBHEADERS_H__

/*
================================================================================================

	Minimum set of headers needed to compile the code for a job.

================================================================================================
*/

#include "sys/sys_defines.h"

#include <stddef.h>					// for offsetof
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <basetsd.h>				// for UINT_PTR
#include <intrin.h>
#pragma warning( disable : 4100 )	// unreferenced formal parameter
#pragma warning( disable : 4127 )	// conditional expression is constant





#include "sys/sys_assert.h"
#include "sys/sys_types.h"
#include "sys/sys_intrinsics.h"
#include "math/Math.h"
#include "ParallelJobList.h"

#if _MSC_VER >= 1600
#undef NULL
#define NULL 0
#endif

#endif // !__PARALLELJOBLIST_JOBHEADERS_H__
