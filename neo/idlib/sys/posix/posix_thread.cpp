/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
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
#pragma hdrstop
#include "../../precompiled.h"


//#define DEBUG_THREADS

typedef void* ( *pthread_function_t )( void* );

/*
========================
Sys_Createthread
========================
*/
uintptr_t Sys_CreateThread( xthread_t function, void* parms, xthreadPriority priority, const char* name, core_t core, int stackSize, bool suspended )
{
	//Sys_EnterCriticalSection();
	
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
	pthread_attr_destroy( &attr );
	
	// RB: TODO pthread_setname_np is different on Linux, MacOSX and other systems
#if defined(DEBUG_THREADS)
	if( pthread_setname_np( handle, name ) != 0 )
	{
		idLib::common->FatalError( "ERROR: pthread_setname_np %s failed\n", name );
		return ( uintptr_t )0;
	}
#endif
	
	/*
	TODO RB: support thread priorities?
	
	if( priority == THREAD_HIGHEST )
	{
		SetThreadPriority( ( HANDLE )handle, THREAD_PRIORITY_HIGHEST );		//  we better sleep enough to do this
	}
	else if( priority == THREAD_ABOVE_NORMAL )
	{
		SetThreadPriority( ( HANDLE )handle, THREAD_PRIORITY_ABOVE_NORMAL );
	}
	else if( priority == THREAD_BELOW_NORMAL )
	{
		SetThreadPriority( ( HANDLE )handle, THREAD_PRIORITY_BELOW_NORMAL );
	}
	else if( priority == THREAD_LOWEST )
	{
		SetThreadPriority( ( HANDLE )handle, THREAD_PRIORITY_LOWEST );
	}
	*/
	
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
	 * This cast is save because pthread_self()
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
	pthread_getname_np( threadHandle, name, sizeof( name ) );
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
	pthread_yield();
}

/*
================================================================================================

	Signal

================================================================================================
*/

idSysSignal::idSysSignal( bool manualReset )
{
#if 0
	pthread_mutexattr_t attr;
	
	pthread_mutexattr_init( &attr );
	pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_ERRORCHECK );
	//pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_DEFAULT );
	pthread_mutex_init( &mutex, &attr );
	pthread_mutexattr_destroy( &attr );
#else
	pthread_mutex_init( &mutex, NULL );
#endif
	
	pthread_cond_init( &cond, NULL );
	
	signaled = false;
	signalCounter = 0;
	waiting = false;
	this->manualReset = manualReset;
}

idSysSignal::~idSysSignal()
{
	pthread_cond_destroy( &cond );
	pthread_mutex_destroy( &mutex );
}

void idSysSignal::Raise()
{
	pthread_mutex_lock( &mutex );
	
	//if( waiting )
	{
		//pthread_cond_signal( &cond );
		//pthread_cond_broadcast( &cond );
	}
	//else
	if( !signaled )
	{
		// emulate Windows behaviour: if no thread is waiting, leave the signal on so next wait keeps going
		signaled = true;
		signalCounter++;
		
		pthread_cond_signal( &cond );
		//pthread_cond_broadcast( &cond );
	}
	
	pthread_mutex_unlock( &mutex );
}

void idSysSignal::Clear()
{
	pthread_mutex_lock( &mutex );
	
	signaled = false;
	
	pthread_mutex_unlock( &mutex );
}

// Wait returns true if the object is in a signalled state and
// returns false if the wait timed out. Wait also clears the signalled
// state when the signalled state is reached within the time out period.
bool idSysSignal::Wait( int timeout )
{
	pthread_mutex_lock( &mutex );
	
	int result = 0;
	
#if 1
	assert( !waiting );	// WaitForEvent from multiple threads? that wouldn't be good
	
	if( signaled )
	{
		if( !manualReset )
		{
			// emulate Windows behaviour: signal has been raised already. clear and keep going
			signaled = false;
			result = 0;
		}
	}
	else
#endif
	{
	
#if 0
	
		int signalValue = signalCounter;
		//while( !signaled && signalValue == signalCounter )
#endif
		{
			waiting = true;
			
			if( timeout == WAIT_INFINITE )
			{
				result = pthread_cond_wait( &cond, &mutex );
				
				assert( result == 0 );
			}
			else
			{
				timespec ts;
				clock_gettime( CLOCK_REALTIME, &ts );
				
				ts.tv_nsec += ( timeout * 1000000 );
				
				result = pthread_cond_timedwait( &cond, &mutex, &ts );
				
				assert( result == 0 || ( timeout != idSysSignal::WAIT_INFINITE && result == ETIMEDOUT ) );
			}
			
			waiting = false;
		}
		
		if( !manualReset )
		{
			signaled = false;
		}
	}
	
	pthread_mutex_unlock( &mutex );
	
	return ( result == 0 );
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
