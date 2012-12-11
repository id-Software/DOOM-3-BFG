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

#ifdef ID_MCHECK
#include <mcheck.h>
#endif

static idStr	basepath;
static idStr	savepath;



/*
 ==============
 Sys_DefaultSavePath
 ==============
 */
const char* Sys_DefaultSavePath()
{
#if defined( ID_DEMO_BUILD )
	sprintf( savepath, "%s/.rbdoom3-demo", getenv( "HOME" ) );
#else
	sprintf( savepath, "%s/.rbdoom3", getenv( "HOME" ) );
#endif
	return savepath.c_str();
}
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
================
Sys_DefaultBasePath

Get the default base path
- binary image path
- current directory
- hardcoded
Try to be intelligent: if there is no BASE_GAMEDIR, try the next path
================
*/
const char* Sys_DefaultBasePath()
{
	struct stat st;
	idStr testbase;
	basepath = Sys_EXEPath();
	if( basepath.Length() )
	{
		basepath.StripFilename();
		testbase = basepath;
		testbase += "/";
		testbase += BASE_GAMEDIR;
		if( stat( testbase.c_str(), &st ) != -1 && S_ISDIR( st.st_mode ) )
		{
			return basepath.c_str();
		}
		else
		{
			common->Printf( "no '%s' directory in exe path %s, skipping\n", BASE_GAMEDIR, basepath.c_str() );
		}
	}
	if( basepath != Posix_Cwd() )
	{
		basepath = Posix_Cwd();
		testbase = basepath;
		testbase += "/";
		testbase += BASE_GAMEDIR;
		if( stat( testbase.c_str(), &st ) != -1 && S_ISDIR( st.st_mode ) )
		{
			return basepath.c_str();
		}
		else
		{
			common->Printf( "no '%s' directory in cwd path %s, skipping\n", BASE_GAMEDIR, basepath.c_str() );
		}
	}
	common->Printf( "WARNING: using hardcoded default base path\n" );
	return LINUX_DEFAULT_PATH;
}



/*
===============
Sys_Shutdown
===============
*/
void Sys_Shutdown()
{
	basepath.Clear();
	savepath.Clear();
	Posix_Shutdown();
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
Sys_FPU_EnableExceptions
===============
*/
//void Sys_FPU_EnableExceptions( int exceptions )
//{
//}

/*
===============
Sys_FPE_handler
===============
*/
void Sys_FPE_handler( int signum, siginfo_t* info, void* context )
{
	assert( signum == SIGFPE );
	Sys_Printf( "FPE\n" );
}

/*
===============
Sys_GetClockticks
===============
*/
double Sys_GetClockTicks()
{
#if defined( __i386__ )
	unsigned long lo, hi;
	
	__asm__ __volatile__(
		"push %%ebx\n"			\
		"xor %%eax,%%eax\n"		\
		"cpuid\n"					\
		"rdtsc\n"					\
		"mov %%eax,%0\n"			\
		"mov %%edx,%1\n"			\
		"pop %%ebx\n"
		: "=r"( lo ), "=r"( hi ) );
	return ( double ) lo + ( double ) 0xFFFFFFFF * hi;
#else
//#error unsupported CPU
// RB begin
	struct timespec now;
	clock_gettime( CLOCK_MONOTONIC, &now );
	return now.tv_sec * 1000000000LL + now.tv_nsec;
// RB end
#endif
}

/*
===============
MeasureClockTicks
===============
*/
double MeasureClockTicks()
{
	double t0, t1;
	
	t0 = Sys_GetClockTicks( );
	Sys_Sleep( 1000 );
	t1 = Sys_GetClockTicks( );
	return t1 - t0;
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
	
	int		fd, len, pos, end;
	char	buf[ 4096 ];
	
	if( init )
	{
		return ret;
	}
	
	fd = open( "/proc/cpuinfo", O_RDONLY );
	if( fd == -1 )
	{
		common->Printf( "couldn't read /proc/cpuinfo\n" );
		ret = MeasureClockTicks();
		init = true;
		common->Printf( "measured CPU frequency: %g MHz\n", ret / 1000000.0 );
		return ret;
	}
	len = read( fd, buf, 4096 );
	close( fd );
	pos = 0;
	while( pos < len )
	{
		if( !idStr::Cmpn( buf + pos, "cpu MHz", 7 ) )
		{
			pos = strchr( buf + pos, ':' ) - buf + 2;
			end = strchr( buf + pos, '\n' ) - buf;
			if( pos < len && end < len )
			{
				buf[end] = '\0';
				ret = atof( buf + pos );
			}
			else
			{
				common->Printf( "failed parsing /proc/cpuinfo\n" );
				ret = MeasureClockTicks();
				init = true;
				common->Printf( "measured CPU frequency: %g MHz\n", ret / 1000000.0 );
				return ret;
			}
			common->Printf( "/proc/cpuinfo CPU frequency: %g MHz\n", ret );
			ret *= 1000000;
			init = true;
			return ret;
		}
		pos = strchr( buf + pos, '\n' ) - buf + 1;
	}
	common->Printf( "failed parsing /proc/cpuinfo\n" );
	ret = MeasureClockTicks();
	init = true;
	common->Printf( "measured CPU frequency: %g MHz\n", ret / 1000000.0 );
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
	static double	ret;
	
	static int		s_numLogicalCPUCores;
	static int		s_numPhysicalCPUCores;
	static int		s_numCPUPackages;
	
	int		fd, len, pos, end;
	char	buf[ 4096 ];
	
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
			if( !idStr::Cmpn( buf + pos, "processor", 9 ) )
			{
				pos = strchr( buf + pos, ':' ) - buf + 2;
				end = strchr( buf + pos, '\n' ) - buf;
				if( pos < len && end < len )
				{
					buf[end] = '\0';
					int processor = atoi( buf + pos );
					
					if( ( processor + 1 ) > s_numPhysicalCPUCores )
					{
						s_numPhysicalCPUCores = processor + 1;
					}
				}
				else
				{
					common->Printf( "failed parsing /proc/cpuinfo\n" );
					break;
				}
			}
			
			if( !idStr::Cmpn( buf + pos, "core id", 7 ) )
			{
				pos = strchr( buf + pos, ':' ) - buf + 2;
				end = strchr( buf + pos, '\n' ) - buf;
				if( pos < len && end < len )
				{
					buf[end] = '\0';
					int coreId = atoi( buf + pos );
					
					if( ( coreId + 1 ) > s_numLogicalCPUCores )
					{
						s_numLogicalCPUCores = coreId + 1;
					}
				}
				else
				{
					common->Printf( "failed parsing /proc/cpuinfo\n" );
					break;
				}
			}
			
			pos = strchr( buf + pos, '\n' ) - buf + 1;
		}
	}
	
	common->Printf( "/proc/cpuinfo CPU processors: %d\n", s_numPhysicalCPUCores );
	common->Printf( "/proc/cpuinfo CPU logical cores: %d\n", s_numLogicalCPUCores );
	
	numPhysicalCPUCores = s_numPhysicalCPUCores;
	numLogicalCPUCores = s_numLogicalCPUCores;
	numCPUPackages = s_numCPUPackages;
}
// RB end

/*
================
Sys_GetSystemRam
returns in megabytes
================
*/
int Sys_GetSystemRam()
{
	long	count, page_size;
	int		mb;
	
	count = sysconf( _SC_PHYS_PAGES );
	if( count == -1 )
	{
		common->Printf( "GetSystemRam: sysconf _SC_PHYS_PAGES failed\n" );
		return 512;
	}
	page_size = sysconf( _SC_PAGE_SIZE );
	if( page_size == -1 )
	{
		common->Printf( "GetSystemRam: sysconf _SC_PAGE_SIZE failed\n" );
		return 512;
	}
	mb = ( int )( ( double )count * ( double )page_size / ( 1024 * 1024 ) );
	// round to the nearest 16Mb
	mb = ( mb + 8 ) & ~15;
	return mb;
}



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
=================
Sys_OpenURL
=================
*/
void idSysLocal::OpenURL( const char* url, bool quit )
{
	const char*	script_path;
	idFile*		script_file;
	char		cmdline[ 1024 ];
	
	static bool	quit_spamguard = false;
	
	if( quit_spamguard )
	{
		common->DPrintf( "Sys_OpenURL: already in a doexit sequence, ignoring %s\n", url );
		return;
	}
	
	common->Printf( "Open URL: %s\n", url );
	// opening an URL on *nix can mean a lot of things ..
	// just spawn a script instead of deciding for the user :-)
	
	// look in the savepath first, then in the basepath
	script_path = fileSystem->BuildOSPath( cvarSystem->GetCVarString( "fs_savepath" ), "", "openurl.sh" );
	script_file = fileSystem->OpenExplicitFileRead( script_path );
	if( !script_file )
	{
		script_path = fileSystem->BuildOSPath( cvarSystem->GetCVarString( "fs_basepath" ), "", "openurl.sh" );
		script_file = fileSystem->OpenExplicitFileRead( script_path );
	}
	if( !script_file )
	{
		common->Printf( "Can't find URL script 'openurl.sh' in either savepath or basepath\n" );
		common->Printf( "OpenURL '%s' failed\n", url );
		return;
	}
	fileSystem->CloseFile( script_file );
	
	// if we are going to quit, only accept a single URL before quitting and spawning the script
	if( quit )
	{
		quit_spamguard = true;
	}
	
	common->Printf( "URL script: %s\n", script_path );
	
	// StartProcess is going to execute a system() call with that - hence the &
	idStr::snPrintf( cmdline, 1024, "%s '%s' &",  script_path, url );
	sys->StartProcess( cmdline, quit );
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
	return "TODO Sys_GetCmdLine";
}

/*
========================
Sys_ReLaunch
========================
*/
void Sys_ReLaunch( void* data, const unsigned int dataSize )
{
	idLib::Error( "Could not start process: TODO Sys_ReLaunch() " );
}

/*
===============
main
===============
*/
int main( int argc, const char** argv )
{
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
