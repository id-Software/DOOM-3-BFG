/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.
Copyright (C) 2012 Robert Beckebans

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
#include "../../idlib/precompiled.h"
#include "../posix/posix_public.h"
#include "../sys_local.h"
//#include "local.h"

#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

// DG: needed for Sys_ReLaunch()
#include <dirent.h>

static const char** cmdargv = NULL;
static int cmdargc = 0;
// DG end

// RB begin
#include <stdio.h> // needed for sysconf()
#include <cstring>
// RB end

#ifdef ID_MCHECK
	#include <mcheck.h>
#endif

/*
==============
Sys_EXEPath
==============
*/
const char* Sys_EXEPath()
{
	static char	buf[ 1024 ];
	idStr		linkpath;
	int			len;

	buf[ 0 ] = '\0';
	sprintf( linkpath, "/proc/%d/exe", getpid() );
	len = readlink( linkpath.c_str(), buf, sizeof( buf ) );
	if( len == -1 )
	{
		Sys_Printf( "couldn't stat exe path link %s\n", linkpath.c_str() );
		// RB: fixed array subscript is below array bounds
		buf[ 0 ] = '\0';
		// RB end
	}
	return buf;
}

/*
===============
Sys_GetProcessorId
===============
*/
cpuid_t Sys_GetProcessorId()
{
	return CPUID_GENERIC;
}

/*
===============
Sys_GetProcessorString
===============
*/
const char* Sys_GetProcessorString()
{
	return "generic";
}

/*
===============
Sys_ClockTicksPerSecond
===============
*/
double Sys_ClockTicksPerSecond()
{
	static bool		init = false;
	static double	ret;

	if( init )
	{
		return ret;
	}

	ret = MeasureClockTicks();
	init = true;
	common->Printf( "measured CPU frequency: %g MHz\n", ret / 1000000.0 );
	return ret;
}

/*
========================
Sys_CPUCount

numLogicalCPUCores	- the total number of logical CPU cores (equal to the total number of threads from all CPU)
numPhysicalCPUCores	- the total number of physical CPU cores
numCPUPackages		- the total number of packages (physical processors)
========================
*/
// RB begin
void Sys_CPUCount( int& numLogicalCPUCores, int& numPhysicalCPUCores, int& numCPUPackages )
{
	static bool		init = false;
	static bool		CPUCoresIsFound = false; // needed for sysconf()
	static bool		SiblingsIsFound = false; // needed for sysconf()
	static double	ret;

	static int		s_numLogicalCPUCores;
	static int		s_numPhysicalCPUCores;
	static int		s_numCPUPackages;

	int		fd, len, pos, end;
	char	buf[ 4096 ];
	char	number[100];

	if( init )
	{
		numPhysicalCPUCores = s_numPhysicalCPUCores;
		numLogicalCPUCores = s_numLogicalCPUCores;
		numCPUPackages = s_numCPUPackages;
	}

	s_numPhysicalCPUCores = 1;
	s_numLogicalCPUCores = 1;
	s_numCPUPackages = 1;

	fd = open( "/proc/cpuinfo", O_RDONLY );
	if( fd != -1 )
	{
		len = read( fd, buf, 4096 );
		close( fd );
		pos = 0;
		while( pos < len )
		{
			if( !idStr::Cmpn( buf + pos, "cpu cores", 9 ) )
			{
				pos = strchr( buf + pos, ':' ) - buf + 2;
				end = strchr( buf + pos, '\n' ) - buf;
				if( pos < len && end < len )
				{
					idStr::Copynz( number, buf + pos, sizeof( number ) );
					assert( ( end - pos ) > 0 && ( end - pos ) < sizeof( number ) );
					number[ end - pos ] = '\0';

					int processor = atoi( number );

					if( ( processor ) > s_numPhysicalCPUCores )
					{
						s_numPhysicalCPUCores = processor;
						CPUCoresIsFound = true;
					}
				}
				else
				{
					common->Printf( "failed parsing /proc/cpuinfo\n" );
					CPUCoresIsFound = false;
					break;
				}
			}
			else if( !idStr::Cmpn( buf + pos, "siblings", 8 ) )
			{
				pos = strchr( buf + pos, ':' ) - buf + 2;
				end = strchr( buf + pos, '\n' ) - buf;
				if( pos < len && end < len )
				{
					idStr::Copynz( number, buf + pos, sizeof( number ) );
					assert( ( end - pos ) > 0 && ( end - pos ) < sizeof( number ) );
					number[ end - pos ] = '\0';

					int coreId = atoi( number );

					if( ( coreId ) > s_numLogicalCPUCores )
					{
						s_numLogicalCPUCores = coreId;
						SiblingsIsFound = true;
					}
				}
				else
				{
					common->Printf( "failed parsing /proc/cpuinfo\n" );
					SiblingsIsFound = false;
					break;
				}
			}

			pos = strchr( buf + pos, '\n' ) - buf + 1;
		}
		if( CPUCoresIsFound == false && SiblingsIsFound == false )
		{
			common->Printf( "failed parsing /proc/cpuinfo\n" );
			common->Printf( "alternative method used\n" );
			s_numPhysicalCPUCores = sysconf( _SC_NPROCESSORS_CONF ); // _SC_NPROCESSORS_ONLN may not be reliable on Android
			s_numLogicalCPUCores = s_numPhysicalCPUCores; // hack for CPU without Hyper-Threading (HT) technology
		}
		else if( CPUCoresIsFound == true && SiblingsIsFound == false )
		{
			s_numLogicalCPUCores = s_numPhysicalCPUCores; // hack for CPU without Hyper-Threading (HT) technology
		}
	}
	else
	{
		common->Printf( "failed to read /proc/cpuinfo\n" );
		common->Printf( "alternative method used\n" );
		s_numPhysicalCPUCores = sysconf( _SC_NPROCESSORS_CONF ); // _SC_NPROCESSORS_ONLN may not be reliable on Android
		s_numLogicalCPUCores = s_numPhysicalCPUCores; // hack for CPU without Hyper-Threading (HT) technology
	}

	common->Printf( "/proc/cpuinfo CPU processors: %d\n", s_numPhysicalCPUCores );
	common->Printf( "/proc/cpuinfo CPU logical cores: %d\n", s_numLogicalCPUCores );

	numPhysicalCPUCores = s_numPhysicalCPUCores;
	numLogicalCPUCores = s_numLogicalCPUCores;
	numCPUPackages = s_numCPUPackages;
}
// RB end

/*
==================
Sys_DoStartProcess
if we don't fork, this function never returns
the no-fork lets you keep the terminal when you're about to spawn an installer

if the command contains spaces, system() is used. Otherwise the more straightforward execl ( system() blows though )
==================
*/
void Sys_DoStartProcess( const char* exeName, bool dofork )
{
	bool use_system = false;
	if( strchr( exeName, ' ' ) )
	{
		use_system = true;
	}
	else
	{
		// set exec rights when it's about a single file to execute
		struct stat buf;
		if( stat( exeName, &buf ) == -1 )
		{
			printf( "stat %s failed: %s\n", exeName, strerror( errno ) );
		}
		else
		{
			if( chmod( exeName, buf.st_mode | S_IXUSR ) == -1 )
			{
				printf( "cmod +x %s failed: %s\n", exeName, strerror( errno ) );
			}
		}
	}
	if( dofork )
	{
		switch( fork() )
		{
			case -1:
				// main thread
				break;
			case 0:
				if( use_system )
				{
					printf( "system %s\n", exeName );
					system( exeName );
					_exit( 0 );
				}
				else
				{
					printf( "execl %s\n", exeName );
					execl( exeName, exeName, NULL );
					printf( "execl failed: %s\n", strerror( errno ) );
					_exit( -1 );
				}
				break;
		}
	}
	else
	{
		if( use_system )
		{
			printf( "system %s\n", exeName );
			system( exeName );
			sleep( 1 );	// on some systems I've seen that starting the new process and exiting this one should not be too close
		}
		else
		{
			printf( "execl %s\n", exeName );
			execl( exeName, exeName, NULL );
			printf( "execl failed: %s\n", strerror( errno ) );
		}
		// terminate
		_exit( 0 );
	}
}

/*
 ==================
 Sys_DoPreferences
 ==================
 */
void Sys_DoPreferences() { }

#if 0
/*
================
Sys_FPU_SetDAZ
================
*/
void Sys_FPU_SetDAZ( bool enable )
{
	/*
	DWORD dwData;

	_asm {
		movzx	ecx, byte ptr enable
		and		ecx, 1
		shl		ecx, 6
		STMXCSR	dword ptr dwData
		mov		eax, dwData
		and		eax, ~(1<<6)	// clear DAX bit
		or		eax, ecx		// set the DAZ bit
		mov		dwData, eax
		LDMXCSR	dword ptr dwData
	}
	*/
}

/*
================
Sys_FPU_SetFTZ
================
*/
void Sys_FPU_SetFTZ( bool enable )
{
	/*
	DWORD dwData;

	_asm {
		movzx	ecx, byte ptr enable
		and		ecx, 1
		shl		ecx, 15
		STMXCSR	dword ptr dwData
		mov		eax, dwData
		and		eax, ~(1<<15)	// clear FTZ bit
		or		eax, ecx		// set the FTZ bit
		mov		dwData, eax
		LDMXCSR	dword ptr dwData
	}
	*/
}
#endif

/*
===============
mem consistency stuff
===============
*/

#ifdef ID_MCHECK

const char* mcheckstrings[] =
{
	"MCHECK_DISABLED",
	"MCHECK_OK",
	"MCHECK_FREE",	// block freed twice
	"MCHECK_HEAD",	// memory before the block was clobbered
	"MCHECK_TAIL"	// memory after the block was clobbered
};

void abrt_func( mcheck_status status )
{
	Sys_Printf( "memory consistency failure: %s\n", mcheckstrings[ status + 1 ] );
	Posix_SetExit( EXIT_FAILURE );
	common->Quit();
}

#endif


/*
========================
Sys_GetCmdLine
========================
*/
const char* Sys_GetCmdLine()
{
	// DG: don't use this, use cmdargv and cmdargc instead!
	return "TODO Sys_GetCmdLine";
}

/*
========================
Sys_ReLaunch
========================
*/
void Sys_ReLaunch()
{
	// DG: implementing this... basic old fork() exec() (+ setsid()) routine..
	// NOTE: this function used to have parameters: the commandline arguments, but as one string..
	//       for Linux/Unix we want one char* per argument so we'll just add the friggin'
	//       " +set com_skipIntroVideos 1" to the other commandline arguments in this function.

	int ret = fork();
	if( ret < 0 )
	{
		idLib::Error( "Sys_ReLaunch(): Couldn't fork(), reason: %s ", strerror( errno ) );
	}

	if( ret == 0 )
	{
		// child process

		// get our own session so we don't depend on the (soon to be killed)
		// parent process anymore - else we'll freeze
		pid_t sId = setsid();
		if( sId == ( pid_t ) - 1 )
		{
			idLib::Error( "Sys_ReLaunch(): setsid() failed! Reason: %s ", strerror( errno ) );
		}

		// close all FDs (except for stdin/out/err) so we don't leak FDs
		DIR* devfd = opendir( "/dev/fd" );
		if( devfd != NULL )
		{
			struct dirent entry;
			struct dirent* result;
			while( readdir_r( devfd, &entry, &result ) == 0 )
			{
				const char* filename = result->d_name;
				char* endptr = NULL;
				long int fd = strtol( filename, &endptr, 0 );
				if( endptr != filename && fd > STDERR_FILENO )
				{
					close( fd );
				}
			}
		}
		else
		{
			idLib::Warning( "Sys_ReLaunch(): Couldn't open /dev/fd/ - will leak file descriptors. Reason: %s", strerror( errno ) );
		}

		// + 3 because "+set" "com_skipIntroVideos" "1" - and note that while we'll skip
		// one (the first) cmdargv argument, we need one more pointer for NULL at the end.
		int argc = cmdargc + 3;
		const char** argv = ( const char** )calloc( argc, sizeof( char* ) );

		int i;
		for( i = 0; i < cmdargc - 1; ++i )
		{
			argv[i] = cmdargv[i + 1];    // ignore cmdargv[0] == executable name
		}

		// add +set com_skipIntroVideos 1
		argv[i++] = "+set";
		argv[i++] = "com_skipIntroVideos";
		argv[i++] = "1";
		// execv expects NULL terminated array
		argv[i] = NULL;

		const char* exepath = Sys_EXEPath();

		errno = 0;
		execv( exepath, ( char** )argv );
		// we only get here if execv() fails, else the executable is restarted
		idLib::Error( "Sys_ReLaunch(): WTF exec() failed! Reason: %s ", strerror( errno ) );

	}
	else
	{
		// original process
		// just do a clean shutdown
		cmdSystem->AppendCommandText( "quit\n" );
	}
	// DG end
}

/*
===============
main
===============
*/
int main( int argc, const char** argv )
{
	// DG: needed for Sys_ReLaunch()
	cmdargc = argc;
	cmdargv = argv;
	// DG end
#ifdef ID_MCHECK
	// must have -lmcheck linkage
	mcheck( abrt_func );
	Sys_Printf( "memory consistency checking enabled\n" );
#endif

	Posix_EarlyInit( );

	if( argc > 1 )
	{
		common->Init( argc - 1, &argv[1], NULL );
	}
	else
	{
		common->Init( 0, NULL, NULL );
	}

	Posix_LateInit( );


	while( 1 )
	{
		common->Frame();
	}
}
