/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#ifndef __SYS_POSIX__
#define __SYS_POSIX__

#include <signal.h>

extern glconfig_t glConfig;

void		Posix_QueEvent( sysEventType_t type, int value, int value2, int ptrLength, void* ptr );
const char*	Posix_Cwd();

// called first thing. does InitSigs and various things
void		Posix_EarlyInit( );
// called after common has been initialized
void		Posix_LateInit( );

void		Posix_InitPThreads( );
void		Posix_InitSigs( );
void		Posix_ClearSigs( );

void		Posix_Exit( int ret );
void		Posix_SetExit( int ret ); // override the exit code
void		Posix_SetExitSpawn( const char* exeName ); // set the process to be spawned when we quit

bool		Posix_AddKeyboardPollEvent( int key, bool state );
bool		Posix_AddMousePollEvent( int action, int value );

void		Posix_PollInput();
void		Posix_InitConsoleInput();
void		Posix_Shutdown();

void		Sys_FPE_handler( int signum, siginfo_t* info, void* context );
void		Sys_DoStartProcess( const char* exeName, bool dofork = true ); // if not forking, current process gets replaced

char*		Posix_ConsoleInput();

double 		MeasureClockTicks();

#ifdef __APPLE__
#if !defined(CLOCK_REALTIME)                    // SRS - define clockid_t enum for OSX 10.11 and earlier
enum /*clk_id_t*/ clockid_t { CLOCK_REALTIME, CLOCK_MONOTONIC, CLOCK_MONOTONIC_RAW };
#endif
int clock_gettime( /*clk_id_t*/ clockid_t clock, struct timespec* tp );     // SRS - use APPLE clockid_t
#endif

#endif

