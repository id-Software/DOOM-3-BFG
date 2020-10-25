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
#ifndef SYS_DEFINES_H
#define SYS_DEFINES_H

/*
================================================================================================

	Platform Specific ID_ Defines

	The ID_ defines are the only platform defines we should be using.

================================================================================================
*/

#undef ID_PC
#undef ID_PC_WIN
#undef ID_PC_WIN64
#undef ID_CONSOLE
#undef ID_WIN32
#undef ID_LITTLE_ENDIAN

#if defined(_WIN32)
	// _WIN32 always defined
	// _WIN64 also defined for x64 target
/*
	#if !defined( _MANAGED )
		#if !defined( _WIN64 )
			#define ID_WIN_X86_ASM
			#define ID_WIN_X86_MMX_ASM
			#define ID_WIN_X86_MMX_INTRIN
			#define ID_WIN_X86_SSE_ASM
			#define ID_WIN_X86_SSE_INTRIN
			#define ID_WIN_X86_SSE2_ASM
			#define ID_WIN_X86_SSE2_INTRIN
			// the 32 bit build is now as close to the console builds as possible
			#define ID_CONSOLE
		#else
			#define ID_PC_WIN64
			#define ID_WIN_X86_MMX_INTRIN
			#define ID_WIN_X86_SSE_INTRIN
			#define ID_WIN_X86_SSE2_INTRIN
			#define ID_WIN_X86_SSE3_INTRIN
		#endif
	#endif
*/

	#define ID_PC
	#define ID_PC_WIN
	#define ID_WIN32
	#define ID_LITTLE_ENDIAN
#else
#error Unknown Platform
#endif

#define ID_OPENGL

/*
================================================================================================

	PC Windows

================================================================================================
*/

#ifdef ID_PC_WIN

#define	CPUSTRING						"x86"

#define	BUILD_STRING					"win-" CPUSTRING
#define BUILD_OS_ID						0

#define ALIGN16( x )					__declspec(align(16)) x
#define ALIGNTYPE16						__declspec(align(16))
#define ALIGNTYPE128					__declspec(align(128))
#define FORMAT_PRINTF( x )

#define PATHSEPARATOR_STR				"\\"
#define PATHSEPARATOR_CHAR				'\\'
#define NEWLINE							"\r\n"

#define ID_INLINE						inline
#define ID_FORCE_INLINE					__forceinline

// lint complains that extern used with definition is a hazard, but it
// has the benefit (?) of making it illegal to take the address of the function
#ifdef _lint
#define ID_INLINE_EXTERN				inline
#define ID_FORCE_INLINE_EXTERN			__forceinline
#else
#define ID_INLINE_EXTERN				extern inline
#define ID_FORCE_INLINE_EXTERN			extern __forceinline
#endif

// we should never rely on this define in our code. this is here so dodgy external libraries don't get confused
#ifndef WIN32
	#define WIN32
#endif

#endif

/*
================================================================================================

Defines and macros usable in all code

================================================================================================
*/

#define ALIGN( x, a ) ( ( ( x ) + ((a)-1) ) & ~((a)-1) )

#define _alloca16( x )					((void *)ALIGN( (UINT_PTR)_alloca( ALIGN( x, 16 ) + 16 ), 16 ) )
#define _alloca128( x )					((void *)ALIGN( (UINT_PTR)_alloca( ALIGN( x, 128 ) + 128 ), 128 ) )

#define likely( x )	( x )
#define unlikely( x )	( x )

// A macro to disallow the copy constructor and operator= functions
// NOTE: The macro contains "private:" so all members defined after it will be private until
// public: or protected: is specified.
#define DISALLOW_COPY_AND_ASSIGN(TypeName)	\
private:									\
  TypeName(const TypeName&);				\
  void operator=(const TypeName&);


/*
================================================================================================
Setup for /analyze code analysis, which we currently only have on the 360, but
we may get later for win32 if we buy the higher end vc++ licenses.

Even with VS2010 ultmate, /analyze only works for x86, not x64

Also note the __analysis_assume macro in sys_assert.h relates to code analysis.

This header should be included even by job code that doesn't reference the
bulk of the codebase, so it is the best place for analyze pragmas.
================================================================================================
*/

#if defined( ID_WIN32 )

// disable some /analyze warnings here
#pragma warning( disable: 6255 )	// warning C6255: _alloca indicates failure by raising a stack overflow exception. Consider using _malloca instead. (Note: _malloca requires _freea.)
#pragma warning( disable: 6262 )	// warning C6262: Function uses '36924' bytes of stack: exceeds /analyze:stacksize'32768'. Consider moving some data to heap
#pragma warning( disable: 6326 )	// warning C6326: Potential comparison of a constant with another constant

#pragma warning( disable: 6031 )	//  warning C6031: Return value ignored
// this warning fires whenever you have two calls to new in a function, but we assume new never fails, so it is not relevant for us
#pragma warning( disable: 6211 )	// warning C6211: Leaking memory 'staticModel' due to an exception. Consider using a local catch block to clean up memory

// we want to fix all these at some point...
#pragma warning( disable: 6246 )	// warning C6246: Local declaration of 'es' hides declaration of the same name in outer scope. For additional information, see previous declaration at line '969' of 'w:\tech5\rage\game\ai\fsm\fsm_combat.cpp': Lines: 969
#pragma warning( disable: 6244 )	// warning C6244: Local declaration of 'viewList' hides previous declaration at line '67' of 'w:\tech5\engine\renderer\rendertools.cpp'

// win32 needs this, but 360 doesn't
#pragma warning( disable: 6540 )	// warning C6540: The use of attribute annotations on this function will invalidate all of its existing __declspec annotations [D:\tech5\engine\engine-10.vcxproj]

// checking format strings catches a LOT of errors
#include <CodeAnalysis\SourceAnnotations.h>

#define	VERIFY_FORMAT_STRING	[SA_FormatString(Style="printf")]


// We need to inform the compiler that Error() and FatalError() will
// never return, so any conditions that leeds to them being called are
// guaranteed to be false in the following code
#define NO_RETURN __declspec(noreturn)

#endif

// I don't want to disable "warning C6031: Return value ignored" from /analyze
// but there are several cases with sprintf where we pre-initialized the variables
// being scanned into, so we truly don't care if they weren't all scanned.
// Rather than littering #pragma statements around these cases, we can assign the
// return value to this, which means we have considered the issue and decided that
// it doesn't require action.
// The volatile qualifier is to prevent:PVS-Studio warnings like:
// False	2	4214	V519	The 'ignoredReturnValue' object is assigned values twice successively. Perhaps this is a mistake. Check lines: 545, 547.	Rage	collisionmodelmanager_debug.cpp	547	False
extern volatile int ignoredReturnValue;

#define MAX_TYPE( x )			( ( ( ( 1 << ( ( sizeof( x ) - 1 ) * 8 - 1 ) ) - 1 ) << 8 ) | 255 )
#define MIN_TYPE( x )			( - MAX_TYPE( x ) - 1 )
#define MAX_UNSIGNED_TYPE( x )	( ( ( ( 1U << ( ( sizeof( x ) - 1 ) * 8 ) ) - 1 ) << 8 ) | 255U )
#define MIN_UNSIGNED_TYPE( x )	0

#endif
