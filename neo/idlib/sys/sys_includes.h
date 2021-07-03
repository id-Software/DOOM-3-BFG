/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2012 Daniel Gibson
Copyright (C) 2012 Robert Beckebans

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
#ifndef SYS_INCLUDES_H
#define SYS_INCLUDES_H

// Include the various platform specific header files (windows.h, etc)

/*
================================================================================================

	Windows

================================================================================================
*/

// RB: windows specific stuff should only be set on Windows
#if defined(_WIN32)

	#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// prevent auto literal to string conversion

	#ifndef _D3SDK
		#ifndef GAME_DLL

			#define WINVER				0x501

			#include <winsock2.h>
			#include <mmsystem.h>
			#include <mmreg.h>

			#define DIRECTINPUT_VERSION  0x0800			// was 0x0700 with the old mssdk
			#define DIRECTSOUND_VERSION  0x0800

			#ifdef _MSC_VER
				#include <dsound.h>
			#else
				// DG: MinGW is incompatible with the original dsound.h because it contains MSVC specific annotations
				#include <wine-dsound.h>

				// RB: was missing in MinGW/include/winuser.h
				#ifndef MAPVK_VSC_TO_VK_EX
					#define MAPVK_VSC_TO_VK_EX 3
				#endif

				// RB begin
				#if defined(__MINGW32__)
					//#include <sal.h> 	// RB: missing __analysis_assume
					// including <sal.h> breaks some STL crap ...

					#ifndef __analysis_assume
						#define __analysis_assume( x )
					#endif

				#endif
				// RB end

			#endif



			#include <dinput.h>

		#endif /* !GAME_DLL */
	#endif /* !_D3SDK */

	// DG: intrinsics for GCC
	#if defined(__GNUC__) && defined(__SSE2__)
		#include <emmintrin.h>

		// TODO: else: alternative implementations?
	#endif
	// DG end

	#ifdef _MSC_VER
		#include <intrin.h>			// needed for intrinsics like _mm_setzero_si28

		#pragma warning(disable : 4100)				// unreferenced formal parameter
		#pragma warning(disable : 4127)				// conditional expression is constant
		#pragma warning(disable : 4244)				// conversion to smaller type, possible loss of data
		#pragma warning(disable : 4267)				// RB 'initializing': conversion from 'size_t' to 'int', possible loss of data
		#pragma warning(disable : 4714)				// function marked as __forceinline not inlined
		#pragma warning(disable : 4996)				// unsafe string operations
	#endif // _MSC_VER

	#include <windows.h>						// for gl.h

#elif defined(__linux__) || defined(__FreeBSD__)

	#include <signal.h>
	#include <pthread.h>

#endif // #if defined(_WIN32)
// RB end

#include <stdlib.h>							// no malloc.h on mac or unix
#undef FindText								// fix namespace pollution


/*
================================================================================================

	Common Include Files

================================================================================================
*/

#if !defined( _DEBUG ) && !defined( NDEBUG )
	// don't generate asserts
	#define NDEBUG
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <ctype.h>
#include <typeinfo>
#include <errno.h>
#include <math.h>
#include <limits.h>
#include <memory>
// RB: added <stdint.h> for missing uintptr_t with MinGW
#include <stdint.h>
// RB end
// Yamagi: <stddef.h> for ptrdiff_t on FreeBSD
#include <stddef.h>
// Yamagi end

//-----------------------------------------------------

// Hacked stuff we may want to consider implementing later
class idScopedGlobalHeap
{
};

#endif // SYS_INCLUDES_H
