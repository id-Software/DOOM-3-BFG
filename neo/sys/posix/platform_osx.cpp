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
//#include "../sys_local.h"

#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include <sys/sysctl.h>
#include <mach/clock.h>
#include <mach/clock_types.h>
#include <mach/mach.h>
#include <mach-o/dyld.h>

// DG: needed for Sys_ReLaunch()
#include <dirent.h>

static const char** cmdargv = NULL;
static int cmdargc = 0;
// DG end

/*
==============
Sys_EXEPath
==============
*/
const char* Sys_EXEPath()
{
	static char path[1024];
	uint32_t size = sizeof( path );

	if( _NSGetExecutablePath( path, &size ) != 0 )
	{
		Sys_Printf( "buffer too small to store exe path, need size %u\n", size );
		path[0] = '\0';
	}
	return path;
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
	int64_t temp;
	size_t len = sizeof( temp );
	int status;

	if( init )
	{
		return ret;
	}

	status = sysctlbyname( "hw.cpufrequency", &temp, &len, NULL, 0 );
	ret = double( temp );

	if( status == -1 )
	{
		common->Printf( "couldn't read systclbyname\n" );
		ret = MeasureClockTicks();
		init = true;
		common->Printf( "measured CPU frequency: %g MHz\n", ret / 1000000.0 );
		return ret;
	}

	common->Printf( "CPU frequency: %g MHz\n", ret / 1000000.0 );
	init = true;

	return ret;
}

/*
========================
Sys_CPUCount

numLogicalCPUCores	- the number of logical CPU per core
numPhysicalCPUCores	- the total number of cores per package
numCPUPackages		- the total number of packages (physical processors)
========================
*/
// RB begin
void Sys_CPUCount( int& numLogicalCPUCores, int& numPhysicalCPUCores, int& numCPUPackages )
{
	static bool		init = false;

	static int		s_numLogicalCPUCores;
	static int		s_numPhysicalCPUCores;
	static int		s_numCPUPackages;

	size_t len = sizeof( s_numPhysicalCPUCores );

	if( init )
	{
		numPhysicalCPUCores = s_numPhysicalCPUCores;
		numLogicalCPUCores = s_numLogicalCPUCores;
		numCPUPackages = s_numCPUPackages;
	}

	s_numPhysicalCPUCores = 1;
	s_numLogicalCPUCores = 1;
	s_numCPUPackages = 1;


	sysctlbyname( "hw.physicalcpu", &s_numPhysicalCPUCores, &len, NULL, 0 );
	sysctlbyname( "hw.logicalcpu", &s_numLogicalCPUCores, &len, NULL, 0 );

	common->Printf( "CPU processors: %d\n", s_numPhysicalCPUCores );
	common->Printf( "CPU logical cores: %d\n", s_numLogicalCPUCores );

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
			//struct dirent entry;
			struct dirent* result;
			//while( readdir_r( devfd, &entry, &result ) == 0 )
			// SRS - readdir_r() is deprecated on linux, readdir() is thread safe with different dir streams
			while( ( result = readdir( devfd ) ) != NULL )
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

// OS X 10.11 or earlier doesn't have native clock_gettime()
int clock_gettime( /*clk_id_t*/ clockid_t clock, struct timespec* tp )   // SRS - use APPLE clockid_t
{
	switch( clock )
	{
		case CLOCK_MONOTONIC_RAW:
		case CLOCK_MONOTONIC:
		{
			clock_serv_t clock_ref;
			mach_timespec_t tm;
			host_name_port_t self = mach_host_self();
			memset( &tm, 0, sizeof( tm ) );
			if( KERN_SUCCESS != host_get_clock_service( self, SYSTEM_CLOCK, &clock_ref ) )
			{
				mach_port_deallocate( mach_task_self(), self );
				return -1;
			}
			if( KERN_SUCCESS != clock_get_time( clock_ref, &tm ) )
			{
				mach_port_deallocate( mach_task_self(), self );
				return -1;
			}
			mach_port_deallocate( mach_task_self(), self );
			mach_port_deallocate( mach_task_self(), clock_ref );
			tp->tv_sec = tm.tv_sec;
			tp->tv_nsec = tm.tv_nsec;
			break;
		}

		case CLOCK_REALTIME:
		default:
		{
			struct timeval now;
			if( KERN_SUCCESS != gettimeofday( &now, NULL ) )
			{
				return -1;
			}
			tp->tv_sec  = now.tv_sec;
			tp->tv_nsec = now.tv_usec * 1000;
			break;
		}
	}
	return 0;
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

	// Setting memory allocators
	OPTICK_SET_MEMORY_ALLOCATOR(
		[]( size_t size ) -> void* { return operator new( size ); },
		[]( void* p )
	{
		operator delete( p );
	},
	[]()
	{
		/* Do some TLS initialization here if needed */
	}
	);

	Posix_EarlyInit();

	if( argc > 1 )
	{
		common->Init( argc - 1, &argv[1], NULL );
	}
	else
	{
		common->Init( 0, NULL, NULL );
	}

	Posix_LateInit();

	while( 1 )
	{
		OPTICK_FRAME( "MainThread" );

		common->Frame();
	}
}
