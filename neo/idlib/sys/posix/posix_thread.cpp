/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2012-2013 Robert Beckebans
Copyright (C) 2013 Daniel Gibson

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
#pragma hdrstop
#include "../../precompiled.h"

#ifndef _WIN32
	#include <sched.h>
#endif

#ifdef __APPLE__
	#include "../../../sys/posix/posix_public.h"
#endif

#ifdef __FreeBSD__
	#include <pthread_np.h> // for pthread_set_name_np
#endif

// DG: Note: On Linux you need at least (e)glibc 2.12 to be able to set the threadname
//#define DEBUG_THREADS

typedef void* ( *pthread_function_t )( void* );

/*
========================
Sys_SetThreadName

caedes: This should be seen as a helper-function for Sys_CreateThread() only.
        (re)setting the name of a running thread seems like a bad idea and
        currently (fresh d3 bfg source) isn't done anyway.
        Furthermore SDL doesn't support it

========================
*/
#ifdef DEBUG_THREADS
static int Sys_SetThreadName( pthread_t handle, const char* name )
{
	int ret = 0;
#ifdef __linux__
	// NOTE: linux only supports threadnames up to 16chars *including* terminating NULL
	// http://man7.org/linux/man-pages/man3/pthread_setname_np.3.html
	// on my machine a longer name (eg "JobListProcessor_0") caused an ENOENT error (instead of ERANGE)
	assert( strlen( name ) < 16 );

	ret = pthread_setname_np( handle, name );
	if( ret != 0 )
	{
		idLib::common->Printf( "Setting threadname \"%s\" failed, reason: %s (%i)\n", name, strerror( errno ), errno );
	}
#elif defined(__FreeBSD__)
	// according to http://www.freebsd.org/cgi/man.cgi?query=pthread_set_name_np&sektion=3
	// the interface is void pthread_set_name_np(pthread_t tid, const char *name);
	pthread_set_name_np( handle, name ); // doesn't return anything
#endif
	/* TODO: OSX:
		// according to http://stackoverflow.com/a/7989973
		// this needs to be called in the thread to be named!
		ret = pthread_setname_np(name);

		// so we'd have to wrap the xthread_t function in Sys_CreateThread and set the name in the wrapping function...
	*/

	return ret;
}

static int Sys_GetThreadName( pthread_t handle, char* namebuf, size_t buflen )
{
	int ret = 0;
#ifdef __linux__
	ret = pthread_getname_np( handle, namebuf, buflen );
	if( ret != 0 )
	{
		idLib::common->Printf( "Getting threadname failed, reason: %s (%i)\n", strerror( errno ), errno );
	}
#elif defined(__FreeBSD__)
	// seems like there is no pthread_getname_np equivalent on FreeBSD
	idStr::snPrintf( namebuf, buflen, "Can't read threadname on this platform!" );
#endif
	/* TODO: OSX:
		// int pthread_getname_np(pthread_t, char*, size_t);
	*/

	return ret;
}

#endif // DEBUG_THREADS



/*
========================
Sys_Createthread
========================
*/
uintptr_t Sys_CreateThread( xthread_t function, void* parms, xthreadPriority priority, const char* name, core_t core, int stackSize, bool suspended )
{
	pthread_attr_t attr;
	pthread_attr_init( &attr );

	if( pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_JOINABLE ) != 0 )
	{
		idLib::common->FatalError( "ERROR: pthread_attr_setdetachstate %s failed\n", name );
		return ( uintptr_t )0;
	}

	pthread_t handle;
	if( pthread_create( ( pthread_t* )&handle, &attr, ( pthread_function_t )function, parms ) != 0 )
	{
		idLib::common->FatalError( "ERROR: pthread_create %s failed\n", name );
		return ( uintptr_t )0;
	}

#if defined(DEBUG_THREADS)
	if( Sys_SetThreadName( handle, name ) != 0 )
	{
		idLib::common->Warning( "Warning: pthread_setname_np %s failed\n", name );
		return ( uintptr_t )0;
	}
#endif

	pthread_attr_destroy( &attr );


#if 0
	// RB: realtime policies require root privileges

	// all Linux threads have one of the following scheduling policies:

	// SCHED_OTHER or SCHED_NORMAL: the default policy,  priority: [-20..0..19], default 0

	// SCHED_FIFO: first in/first out realtime policy

	// SCHED_RR: round-robin realtime policy

	// SCHED_BATCH: similar to SCHED_OTHER, but with a throughput orientation

	// SCHED_IDLE: lower priority than SCHED_OTHER

	int schedulePolicy = SCHED_OTHER;
	struct sched_param scheduleParam;

	int error = pthread_getschedparam( handle, &schedulePolicy, &scheduleParam );
	if( error != 0 )
	{
		idLib::common->FatalError( "ERROR: pthread_getschedparam %s failed: %s\n", name, strerror( error ) );
		return ( uintptr_t )0;
	}

	schedulePolicy = SCHED_FIFO;

	int minPriority = sched_get_priority_min( schedulePolicy );
	int maxPriority = sched_get_priority_max( schedulePolicy );

	if( priority == THREAD_HIGHEST )
	{
		//  we better sleep enough to do this
		scheduleParam.__sched_priority = maxPriority;
	}
	else if( priority == THREAD_ABOVE_NORMAL )
	{
		scheduleParam.__sched_priority = Lerp( minPriority, maxPriority, 0.75f );
	}
	else if( priority == THREAD_NORMAL )
	{
		scheduleParam.__sched_priority = Lerp( minPriority, maxPriority, 0.5f );
	}
	else if( priority == THREAD_BELOW_NORMAL )
	{
		scheduleParam.__sched_priority = Lerp( minPriority, maxPriority, 0.25f );
	}
	else if( priority == THREAD_LOWEST )
	{
		scheduleParam.__sched_priority = minPriority;
	}

	// set new priority
	error = pthread_setschedparam( handle, schedulePolicy, &scheduleParam );
	if( error != 0 )
	{
		idLib::common->FatalError( "ERROR: pthread_setschedparam( name = %s, policy = %i, priority = %i ) failed: %s\n", name, schedulePolicy, scheduleParam.__sched_priority, strerror( error ) );
		return ( uintptr_t )0;
	}

	pthread_getschedparam( handle, &schedulePolicy, &scheduleParam );
	if( error != 0 )
	{
		idLib::common->FatalError( "ERROR: pthread_getschedparam %s failed: %s\n", name, strerror( error ) );
		return ( uintptr_t )0;
	}
#endif

	// Under Linux, we don't set the thread affinity and let the OS deal with scheduling

	return ( uintptr_t )handle;
}


/*
========================
Sys_GetCurrentThreadID
========================
*/
uintptr_t Sys_GetCurrentThreadID()
{
	/*
	 * This cast is safe because pthread_self()
	 * returns a pointer and uintptr_t is
	 * designed to hold a pointer. The compiler
	 * is just too stupid to know. :)
	 *  -- Yamagi
	 */
	return ( uintptr_t )pthread_self();
}

/*
========================
Sys_DestroyThread
========================
*/
void Sys_DestroyThread( uintptr_t threadHandle )
{
	if( threadHandle == 0 )
	{
		return;
	}

	char	name[128];
	name[0] = '\0';

#if defined(DEBUG_THREADS)
	Sys_GetThreadName( ( pthread_t )threadHandle, name, sizeof( name ) );
#endif

#if 0 //!defined(__ANDROID__)
	if( pthread_cancel( ( pthread_t )threadHandle ) != 0 )
	{
		idLib::common->FatalError( "ERROR: pthread_cancel %s failed\n", name );
	}
#endif

	if( pthread_join( ( pthread_t )threadHandle, NULL ) != 0 )
	{
		idLib::common->FatalError( "ERROR: pthread_join %s failed\n", name );
	}
}

/*
========================
Sys_Yield
========================
*/
void Sys_Yield()
{
// SRS - pthread_yield() is deprecated on linux
//#if defined(__ANDROID__) || defined(__APPLE__)
	sched_yield();
//#else
//	pthread_yield();
//#endif
}

/*
================================================================================================

	Signal

================================================================================================
*/

/*
========================
Sys_SignalCreate
========================
*/
void Sys_SignalCreate( signalHandle_t& handle, bool manualReset )
{
	// handle = CreateEvent( NULL, manualReset, FALSE, NULL );

	handle.manualReset = manualReset;
	// if this is true, the signal is only set to nonsignaled when Clear() is called,
	// else it's "auto-reset" and the state is set to !signaled after a single waiting
	// thread has been released

	// the inital state is always "not signaled"
	handle.signaled = false;
	handle.waiting = 0;
#if 0
	pthread_mutexattr_t attr;

	pthread_mutexattr_init( &attr );
	pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_ERRORCHECK );
	//pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_DEFAULT );
	pthread_mutex_init( &mutex, &attr );
	pthread_mutexattr_destroy( &attr );
#else
	pthread_mutex_init( &handle.mutex, NULL );
#endif

	pthread_cond_init( &handle.cond, NULL );

}

/*
========================
Sys_SignalDestroy
========================
*/
void Sys_SignalDestroy( signalHandle_t& handle )
{
	// CloseHandle( handle );
	handle.signaled = false;
	handle.waiting = 0;
	pthread_mutex_destroy( &handle.mutex );
	pthread_cond_destroy( &handle.cond );
}

/*
========================
Sys_SignalRaise
========================
*/
void Sys_SignalRaise( signalHandle_t& handle )
{
	// SetEvent( handle );
	pthread_mutex_lock( &handle.mutex );

	if( handle.manualReset )
	{
		// signaled until reset
		handle.signaled = true;
		// wake *all* threads waiting on this cond
		pthread_cond_broadcast( &handle.cond );
	}
	else
	{
		// automode: signaled until first thread is released
		if( handle.waiting > 0 )
		{
			// there are waiting threads => release one
			pthread_cond_signal( &handle.cond );
		}
		else
		{
			// no waiting threads, save signal
			handle.signaled = true;
			// while the MSDN documentation is a bit unspecific about what happens
			// when SetEvent() is called n times without a wait inbetween
			// (will only one wait be successful afterwards or n waits?)
			// it seems like the signaled state is a flag, not a counter.
			// http://stackoverflow.com/a/13703585 claims the same.
		}
	}

	pthread_mutex_unlock( &handle.mutex );
}

/*
========================
Sys_SignalClear
========================
*/
void Sys_SignalClear( signalHandle_t& handle )
{
	// ResetEvent( handle );
	pthread_mutex_lock( &handle.mutex );

	// TODO: probably signaled could be atomically changed?
	handle.signaled = false;

	pthread_mutex_unlock( &handle.mutex );
}


/*
========================
Sys_SignalWait
========================
*/
bool Sys_SignalWait( signalHandle_t& handle, int timeout )
{
	//DWORD result = WaitForSingleObject( handle, timeout == idSysSignal::WAIT_INFINITE ? INFINITE : timeout );
	//assert( result == WAIT_OBJECT_0 || ( timeout != idSysSignal::WAIT_INFINITE && result == WAIT_TIMEOUT ) );
	//return ( result == WAIT_OBJECT_0 );

	int status;
	pthread_mutex_lock( &handle.mutex );

	if( handle.signaled ) // there is a signal that hasn't been used yet
	{
		if( ! handle.manualReset ) // for auto-mode only one thread may be released - this one.
		{
			handle.signaled = false;
		}

		status = 0; // success!
	}
	else // we'll have to wait for a signal
	{
		++handle.waiting;
		if( timeout == idSysSignal::WAIT_INFINITE )
		{
			status = pthread_cond_wait( &handle.cond, &handle.mutex );
		}
		else
		{
			timespec ts;

			clock_gettime( CLOCK_REALTIME, &ts );

			// DG: handle timeouts > 1s better
			ts.tv_nsec += ( timeout % 1000 ) * 1000000; // millisec to nanosec
			ts.tv_sec  += timeout / 1000;
			if( ts.tv_nsec >= 1000000000 ) // nanoseconds are more than one second
			{
				ts.tv_nsec -= 1000000000; // remove one second in nanoseconds
				ts.tv_sec += 1; // add one second to seconds
			}
			// DG end
			status = pthread_cond_timedwait( &handle.cond, &handle.mutex, &ts );
		}
		--handle.waiting;
	}

	pthread_mutex_unlock( &handle.mutex );

	assert( status == 0 || ( timeout != idSysSignal::WAIT_INFINITE && status == ETIMEDOUT ) );

	return ( status == 0 );

}

/*
================================================================================================

	Mutex

================================================================================================
*/

/*
========================
Sys_MutexCreate
========================
*/
void Sys_MutexCreate( mutexHandle_t& handle )
{
	pthread_mutexattr_t attr;

	pthread_mutexattr_init( &attr );
	pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_ERRORCHECK );
	pthread_mutex_init( &handle, &attr );

	pthread_mutexattr_destroy( &attr );
}

/*
========================
Sys_MutexDestroy
========================
*/
void Sys_MutexDestroy( mutexHandle_t& handle )
{
	pthread_mutex_destroy( &handle );
}

/*
========================
Sys_MutexLock
========================
*/
bool Sys_MutexLock( mutexHandle_t& handle, bool blocking )
{
	if( pthread_mutex_trylock( &handle ) != 0 )
	{
		if( !blocking )
		{
			return false;
		}
		pthread_mutex_lock( &handle );
	}
	return true;
}

/*
========================
Sys_MutexUnlock
========================
*/
void Sys_MutexUnlock( mutexHandle_t& handle )
{
	pthread_mutex_unlock( & handle );
}

/*
================================================================================================

	Interlocked Integer

================================================================================================
*/

/*
========================
Sys_InterlockedIncrement
========================
*/
interlockedInt_t Sys_InterlockedIncrement( interlockedInt_t& value )
{
	// return InterlockedIncrementAcquire( & value );
	return __sync_add_and_fetch( &value, 1 );
}

/*
========================
Sys_InterlockedDecrement
========================
*/
interlockedInt_t Sys_InterlockedDecrement( interlockedInt_t& value )
{
	// return InterlockedDecrementRelease( & value );
	return __sync_sub_and_fetch( &value, 1 );
}

/*
========================
Sys_InterlockedAdd
========================
*/
interlockedInt_t Sys_InterlockedAdd( interlockedInt_t& value, interlockedInt_t i )
{
	//return InterlockedExchangeAdd( & value, i ) + i;
	return __sync_add_and_fetch( &value, i );
}

/*
========================
Sys_InterlockedSub
========================
*/
interlockedInt_t Sys_InterlockedSub( interlockedInt_t& value, interlockedInt_t i )
{
	//return InterlockedExchangeAdd( & value, - i ) - i;
	return __sync_sub_and_fetch( &value, i );
}

/*
========================
Sys_InterlockedExchange
========================
*/
interlockedInt_t Sys_InterlockedExchange( interlockedInt_t& value, interlockedInt_t exchange )
{
	//return InterlockedExchange( & value, exchange );

	// source: http://gcc.gnu.org/onlinedocs/gcc-4.1.1/gcc/Atomic-Builtins.html
	// These builtins perform an atomic compare and swap. That is, if the current value of *ptr is oldval, then write newval into *ptr.
	return __sync_val_compare_and_swap( &value, value, exchange );
}

/*
========================
Sys_InterlockedCompareExchange
========================
*/
interlockedInt_t Sys_InterlockedCompareExchange( interlockedInt_t& value, interlockedInt_t comparand, interlockedInt_t exchange )
{
	//return InterlockedCompareExchange( & value, exchange, comparand );
	return __sync_val_compare_and_swap( &value, comparand, exchange );
}

/*
================================================================================================

	Interlocked Pointer

================================================================================================
*/

/*
========================
Sys_InterlockedExchangePointer
========================
*/
void* Sys_InterlockedExchangePointer( void*& ptr, void* exchange )
{
	//return InterlockedExchangePointer( & ptr, exchange );
	return __sync_val_compare_and_swap( &ptr, ptr, exchange );
}

/*
========================
Sys_InterlockedCompareExchangePointer
========================
*/
void* Sys_InterlockedCompareExchangePointer( void*& ptr, void* comparand, void* exchange )
{
	//return InterlockedCompareExchangePointer( & ptr, exchange, comparand );
	return __sync_val_compare_and_swap( &ptr, comparand, exchange );
}
