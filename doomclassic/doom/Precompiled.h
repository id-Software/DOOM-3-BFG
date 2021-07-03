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

#pragma once

#include "idlib/precompiled.h"


#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <queue>

#define ID_INLINE inline

typedef unsigned char byte;
typedef unsigned int dword;


#include <math.h>
#include <assert.h>

#define ACTUALTEXTUREWIDTH	1024		// should always be equal to or larger
#define ACTUALTEXTUREHEIGHT	1024

#define GLOBAL_IMAGE_SCALER	3

#define ORIGINAL_WIDTH		320
#define ORIGINAL_HEIGHT		200

#define WIDTH				( ORIGINAL_WIDTH * GLOBAL_IMAGE_SCALER )
#define HEIGHT				( ORIGINAL_HEIGHT * GLOBAL_IMAGE_SCALER )

#define TEXTUREWIDTH		WIDTH
#define TEXTUREHEIGHT		HEIGHT

#define	BASE_WIDTH			WIDTH
#define SCREENWIDTH			WIDTH
#define SCREENHEIGHT		HEIGHT

#define MAXWIDTH			1120
#define MAXHEIGHT			832

// DG: provide MAXINT seems to be available on MSVC but not MinGW,
// so use the standard defines from limits.h
#ifndef MAXINT
#define MAXINT INT_MAX
#endif

#ifndef MININT
#define MININT INT_MIN
#endif
// DG end
