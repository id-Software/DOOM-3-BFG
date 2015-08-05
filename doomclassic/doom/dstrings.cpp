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

#include "Precompiled.h"

#ifdef __GNUG__
#pragma implementation "dstrings.h"
#endif
#include "dstrings.h"

#include "globaldata.h"


const char* endmsg[NUM_QUITMESSAGES+1]=
{
  // DOOM1
  QUITMSG,
  "are you sure you want to quit?",
  "are you sure you want to quit?",
  "are you sure you want to quit?",
  "are you sure you want to quit?",
  "are you sure you want to quit?",
  "are you sure you want to quit?",
  "are you sure you want to quit?"

  // QuitDOOM II messages
  "are you sure you want to quit?",
  "are you sure you want to quit?",
  "are you sure you want to quit?",
  "are you sure you want to quit?",
  "are you sure you want to quit?",
  "are you sure you want to quit?",
  "are you sure you want to quit?"

  // FinalDOOM?
  "are you sure you want to quit?",
  "are you sure you want to quit?",
  "are you sure you want to quit?",
  "are you sure you want to quit?",
  "are you sure you want to quit?",
  "are you sure you want to quit?",
  "are you sure you want to quit?",

  // Internal debug. Different style, too.
  "are you sure you want to quit?"
};


  



