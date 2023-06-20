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

#include "precompiled.h"
#pragma hdrstop


#include "../Game_local.h"

const idEventDef EV_Thread_Execute( "<execute>", NULL );
const idEventDef EV_Thread_SetCallback( "<script_setcallback>", NULL );

// script callable events
const idEventDef EV_Thread_TerminateThread( "terminate", "d" );
const idEventDef EV_Thread_Pause( "pause", NULL );
const idEventDef EV_Thread_Wait( "wait", "f" );
const idEventDef EV_Thread_WaitFrame( "waitFrame" );
const idEventDef EV_Thread_WaitFor( "waitFor", "e" );
const idEventDef EV_Thread_WaitForThread( "waitForThread", "d" );
const idEventDef EV_Thread_Print( "print", "s" );
const idEventDef EV_Thread_PrintLn( "println", "s" );
const idEventDef EV_Thread_Say( "say", "s" );
const idEventDef EV_Thread_Assert( "assert", "f" );
const idEventDef EV_Thread_Trigger( "trigger", "e" );
const idEventDef EV_Thread_SetCvar( "setcvar", "ss" );
const idEventDef EV_Thread_GetCvar( "getcvar", "s", 's' );
const idEventDef EV_Thread_Random( "random", "f", 'f' );
const idEventDef EV_Thread_RandomInt( "randomInt", "d", 'd' );
const idEventDef EV_Thread_GetTime( "getTime", NULL, 'f' );
const idEventDef EV_Thread_KillThread( "killthread", "s" );
const idEventDef EV_Thread_SetThreadName( "threadname", "s" );
const idEventDef EV_Thread_GetEntity( "getEntity", "s", 'e' );
const idEventDef EV_Thread_Spawn( "spawn", "s", 'e' );
const idEventDef EV_Thread_CopySpawnArgs( "copySpawnArgs", "e" );
const idEventDef EV_Thread_SetSpawnArg( "setSpawnArg", "ss" );
const idEventDef EV_Thread_SpawnString( "SpawnString", "ss", 's' );
const idEventDef EV_Thread_SpawnFloat( "SpawnFloat", "sf", 'f' );
const idEventDef EV_Thread_SpawnVector( "SpawnVector", "sv", 'v' );
const idEventDef EV_Thread_ClearPersistantArgs( "clearPersistantArgs" );
const idEventDef EV_Thread_SetPersistantArg( "setPersistantArg", "ss" );
const idEventDef EV_Thread_GetPersistantString( "getPersistantString", "s", 's' );
const idEventDef EV_Thread_GetPersistantFloat( "getPersistantFloat", "s", 'f' );
const idEventDef EV_Thread_GetPersistantVector( "getPersistantVector", "s", 'v' );
const idEventDef EV_Thread_AngToForward( "angToForward", "v", 'v' );
const idEventDef EV_Thread_AngToRight( "angToRight", "v", 'v' );
const idEventDef EV_Thread_AngToUp( "angToUp", "v", 'v' );
const idEventDef EV_Thread_Sine( "sin", "f", 'f' );
const idEventDef EV_Thread_Cosine( "cos", "f", 'f' );
const idEventDef EV_Thread_ArcSine( "asin", "f", 'f' );
const idEventDef EV_Thread_ArcCosine( "acos", "f", 'f' );
const idEventDef EV_Thread_SquareRoot( "sqrt", "f", 'f' );
const idEventDef EV_Thread_Normalize( "vecNormalize", "v", 'v' );
const idEventDef EV_Thread_VecLength( "vecLength", "v", 'f' );
const idEventDef EV_Thread_VecDotProduct( "DotProduct", "vv", 'f' );
const idEventDef EV_Thread_VecCrossProduct( "CrossProduct", "vv", 'v' );
const idEventDef EV_Thread_VecToAngles( "VecToAngles", "v", 'v' );
const idEventDef EV_Thread_VecToOrthoBasisAngles( "VecToOrthoBasisAngles", "v", 'v' );
const idEventDef EV_Thread_RotateVector( "rotateVector", "vv", 'v' );
const idEventDef EV_Thread_OnSignal( "onSignal", "des" );
const idEventDef EV_Thread_ClearSignal( "clearSignalThread", "de" );
const idEventDef EV_Thread_SetCamera( "setCamera", "e" );
const idEventDef EV_Thread_FirstPerson( "firstPerson", NULL );
const idEventDef EV_Thread_Trace( "trace", "vvvvde", 'f' );
const idEventDef EV_Thread_TracePoint( "tracePoint", "vvde", 'f' );
const idEventDef EV_Thread_GetTraceFraction( "getTraceFraction", NULL, 'f' );
const idEventDef EV_Thread_GetTraceEndPos( "getTraceEndPos", NULL, 'v' );
const idEventDef EV_Thread_GetTraceNormal( "getTraceNormal", NULL, 'v' );
const idEventDef EV_Thread_GetTraceEntity( "getTraceEntity", NULL, 'e' );
const idEventDef EV_Thread_GetTraceJoint( "getTraceJoint", NULL, 's' );
const idEventDef EV_Thread_GetTraceBody( "getTraceBody", NULL, 's' );
const idEventDef EV_Thread_FadeIn( "fadeIn", "vf" );
const idEventDef EV_Thread_FadeOut( "fadeOut", "vf" );
const idEventDef EV_Thread_FadeTo( "fadeTo", "vff" );
const idEventDef EV_Thread_StartMusic( "music", "s" );
const idEventDef EV_Thread_Error( "error", "s" );
const idEventDef EV_Thread_Warning( "warning", "s" );
const idEventDef EV_Thread_StrLen( "strLength", "s", 'd' );
const idEventDef EV_Thread_StrLeft( "strLeft", "sd", 's' );
const idEventDef EV_Thread_StrRight( "strRight", "sd", 's' );
const idEventDef EV_Thread_StrSkip( "strSkip", "sd", 's' );
const idEventDef EV_Thread_StrMid( "strMid", "sdd", 's' );
const idEventDef EV_Thread_StrToFloat( "strToFloat", "s", 'f' );
const idEventDef EV_Thread_RadiusDamage( "radiusDamage", "vEEEsf" );
const idEventDef EV_Thread_IsClient( "isClient", NULL, 'f' );
const idEventDef EV_Thread_IsMultiplayer( "isMultiplayer", NULL, 'f' );
const idEventDef EV_Thread_GetFrameTime( "getFrameTime", NULL, 'f' );
const idEventDef EV_Thread_GetTicsPerSecond( "getTicsPerSecond", NULL, 'f' );
const idEventDef EV_Thread_DebugLine( "debugLine", "vvvf" );
const idEventDef EV_Thread_DebugArrow( "debugArrow", "vvvdf" );
const idEventDef EV_Thread_DebugCircle( "debugCircle", "vvvfdf" );
const idEventDef EV_Thread_DebugBounds( "debugBounds", "vvvf" );
const idEventDef EV_Thread_DrawText( "drawText", "svfvdf" );
const idEventDef EV_Thread_InfluenceActive( "influenceActive", NULL, 'd' );

CLASS_DECLARATION( idClass, idThread )
EVENT( EV_Thread_Execute,				idThread::Event_Execute )
EVENT( EV_Thread_TerminateThread,		idThread::Event_TerminateThread )
EVENT( EV_Thread_Pause,					idThread::Event_Pause )
EVENT( EV_Thread_Wait,					idThread::Event_Wait )
EVENT( EV_Thread_WaitFrame,				idThread::Event_WaitFrame )
EVENT( EV_Thread_WaitFor,				idThread::Event_WaitFor )
EVENT( EV_Thread_WaitForThread,			idThread::Event_WaitForThread )
EVENT( EV_Thread_Print,					idThread::Event_Print )
EVENT( EV_Thread_PrintLn,				idThread::Event_PrintLn )
EVENT( EV_Thread_Say,					idThread::Event_Say )
EVENT( EV_Thread_Assert,				idThread::Event_Assert )
EVENT( EV_Thread_Trigger,				idThread::Event_Trigger )
EVENT( EV_Thread_SetCvar,				idThread::Event_SetCvar )
EVENT( EV_Thread_GetCvar,				idThread::Event_GetCvar )
EVENT( EV_Thread_Random,				idThread::Event_Random )
EVENT( EV_Thread_RandomInt,				idThread::Event_RandomInt )
EVENT( EV_Thread_GetTime,				idThread::Event_GetTime )
EVENT( EV_Thread_KillThread,			idThread::Event_KillThread )
EVENT( EV_Thread_SetThreadName,			idThread::Event_SetThreadName )
EVENT( EV_Thread_GetEntity,				idThread::Event_GetEntity )
EVENT( EV_Thread_Spawn,					idThread::Event_Spawn )
EVENT( EV_Thread_CopySpawnArgs,			idThread::Event_CopySpawnArgs )
EVENT( EV_Thread_SetSpawnArg,			idThread::Event_SetSpawnArg )
EVENT( EV_Thread_SpawnString,			idThread::Event_SpawnString )
EVENT( EV_Thread_SpawnFloat,			idThread::Event_SpawnFloat )
EVENT( EV_Thread_SpawnVector,			idThread::Event_SpawnVector )
EVENT( EV_Thread_ClearPersistantArgs,	idThread::Event_ClearPersistantArgs )
EVENT( EV_Thread_SetPersistantArg,		idThread::Event_SetPersistantArg )
EVENT( EV_Thread_GetPersistantString,	idThread::Event_GetPersistantString )
EVENT( EV_Thread_GetPersistantFloat,	idThread::Event_GetPersistantFloat )
EVENT( EV_Thread_GetPersistantVector,	idThread::Event_GetPersistantVector )
EVENT( EV_Thread_AngToForward,			idThread::Event_AngToForward )
EVENT( EV_Thread_AngToRight,			idThread::Event_AngToRight )
EVENT( EV_Thread_AngToUp,				idThread::Event_AngToUp )
EVENT( EV_Thread_Sine,					idThread::Event_GetSine )
EVENT( EV_Thread_Cosine,				idThread::Event_GetCosine )
EVENT( EV_Thread_ArcSine,				idThread::Event_GetArcSine )
EVENT( EV_Thread_ArcCosine,				idThread::Event_GetArcCosine )
EVENT( EV_Thread_SquareRoot,			idThread::Event_GetSquareRoot )
EVENT( EV_Thread_Normalize,				idThread::Event_VecNormalize )
EVENT( EV_Thread_VecLength,				idThread::Event_VecLength )
EVENT( EV_Thread_VecDotProduct,			idThread::Event_VecDotProduct )
EVENT( EV_Thread_VecCrossProduct,		idThread::Event_VecCrossProduct )
EVENT( EV_Thread_VecToAngles,			idThread::Event_VecToAngles )
EVENT( EV_Thread_VecToOrthoBasisAngles, idThread::Event_VecToOrthoBasisAngles )
EVENT( EV_Thread_RotateVector,			idThread::Event_RotateVector )
EVENT( EV_Thread_OnSignal,				idThread::Event_OnSignal )
EVENT( EV_Thread_ClearSignal,			idThread::Event_ClearSignalThread )
EVENT( EV_Thread_SetCamera,				idThread::Event_SetCamera )
EVENT( EV_Thread_FirstPerson,			idThread::Event_FirstPerson )
EVENT( EV_Thread_Trace,					idThread::Event_Trace )
EVENT( EV_Thread_TracePoint,			idThread::Event_TracePoint )
EVENT( EV_Thread_GetTraceFraction,		idThread::Event_GetTraceFraction )
EVENT( EV_Thread_GetTraceEndPos,		idThread::Event_GetTraceEndPos )
EVENT( EV_Thread_GetTraceNormal,		idThread::Event_GetTraceNormal )
EVENT( EV_Thread_GetTraceEntity,		idThread::Event_GetTraceEntity )
EVENT( EV_Thread_GetTraceJoint,			idThread::Event_GetTraceJoint )
EVENT( EV_Thread_GetTraceBody,			idThread::Event_GetTraceBody )
EVENT( EV_Thread_FadeIn,				idThread::Event_FadeIn )
EVENT( EV_Thread_FadeOut,				idThread::Event_FadeOut )
EVENT( EV_Thread_FadeTo,				idThread::Event_FadeTo )
EVENT( EV_SetShaderParm,				idThread::Event_SetShaderParm )
EVENT( EV_Thread_StartMusic,			idThread::Event_StartMusic )
EVENT( EV_Thread_Warning,				idThread::Event_Warning )
EVENT( EV_Thread_Error,					idThread::Event_Error )
EVENT( EV_Thread_StrLen,				idThread::Event_StrLen )
EVENT( EV_Thread_StrLeft,				idThread::Event_StrLeft )
EVENT( EV_Thread_StrRight,				idThread::Event_StrRight )
EVENT( EV_Thread_StrSkip,				idThread::Event_StrSkip )
EVENT( EV_Thread_StrMid,				idThread::Event_StrMid )
EVENT( EV_Thread_StrToFloat,			idThread::Event_StrToFloat )
EVENT( EV_Thread_RadiusDamage,			idThread::Event_RadiusDamage )
EVENT( EV_Thread_IsClient,				idThread::Event_IsClient )
EVENT( EV_Thread_IsMultiplayer,			idThread::Event_IsMultiplayer )
EVENT( EV_Thread_GetFrameTime,			idThread::Event_GetFrameTime )
EVENT( EV_Thread_GetTicsPerSecond,		idThread::Event_GetTicsPerSecond )
EVENT( EV_CacheSoundShader,				idThread::Event_CacheSoundShader )
EVENT( EV_Thread_DebugLine,				idThread::Event_DebugLine )
EVENT( EV_Thread_DebugArrow,			idThread::Event_DebugArrow )
EVENT( EV_Thread_DebugCircle,			idThread::Event_DebugCircle )
EVENT( EV_Thread_DebugBounds,			idThread::Event_DebugBounds )
EVENT( EV_Thread_DrawText,				idThread::Event_DrawText )
EVENT( EV_Thread_InfluenceActive,		idThread::Event_InfluenceActive )
END_CLASS

idThread*			idThread::currentThread = NULL;
int					idThread::threadIndex = 0;
idList<idThread*, TAG_THREAD>	idThread::threadList;
trace_t				idThread::trace;

/*
================
idThread::CurrentThread
================
*/
idThread* idThread::CurrentThread()
{
	return currentThread;
}

/*
================
idThread::CurrentThreadNum
================
*/
int idThread::CurrentThreadNum()
{
	if( currentThread )
	{
		return currentThread->GetThreadNum();
	}
	else
	{
		return 0;
	}
}

/*
================
idThread::BeginMultiFrameEvent
================
*/
bool idThread::BeginMultiFrameEvent( idEntity* ent, const idEventDef* event )
{
	if( currentThread == NULL )
	{
		gameLocal.Error( "idThread::BeginMultiFrameEvent called without a current thread" );
		return false;
	}
	return currentThread->interpreter.BeginMultiFrameEvent( ent, event );
}

/*
================
idThread::EndMultiFrameEvent
================
*/
void idThread::EndMultiFrameEvent( idEntity* ent, const idEventDef* event )
{
	if( currentThread == NULL )
	{
		gameLocal.Error( "idThread::EndMultiFrameEvent called without a current thread" );
		return;
	}
	currentThread->interpreter.EndMultiFrameEvent( ent, event );
}

/*
================
idThread::idThread
================
*/
idThread::idThread()
{
	Init();
	SetThreadName( va( "thread_%d", threadIndex ) );
	if( g_debugScript.GetBool() )
	{
		gameLocal.Printf( "%d: create thread (%d) '%s'\n", gameLocal.time, threadNum, threadName.c_str() );
	}
}

/*
================
idThread::idThread
================
*/
idThread::idThread( idEntity* self, const function_t* func )
{
	assert( self );

	Init();
	SetThreadName( self->name );
	interpreter.EnterObjectFunction( self, func, false );
	if( g_debugScript.GetBool() )
	{
		gameLocal.Printf( "%d: create thread (%d) '%s'\n", gameLocal.time, threadNum, threadName.c_str() );
	}
}

/*
================
idThread::idThread
================
*/
idThread::idThread( const function_t* func )
{
	assert( func );

	Init();
	SetThreadName( func->Name() );
	interpreter.EnterFunction( func, false );
	if( g_debugScript.GetBool() )
	{
		gameLocal.Printf( "%d: create thread (%d) '%s'\n", gameLocal.time, threadNum, threadName.c_str() );
	}
}

/*
================
idThread::idThread
================
*/
idThread::idThread( idInterpreter* source, const function_t* func, int args )
{
	Init();
	interpreter.ThreadCall( source, func, args );
	if( g_debugScript.GetBool() )
	{
		gameLocal.Printf( "%d: create thread (%d) '%s'\n", gameLocal.time, threadNum, threadName.c_str() );
	}
}

/*
================
idThread::idThread
================
*/
idThread::idThread( idInterpreter* source, idEntity* self, const function_t* func, int args )
{
	assert( self );

	Init();
	SetThreadName( self->name );
	interpreter.ThreadCall( source, func, args );
	if( g_debugScript.GetBool() )
	{
		gameLocal.Printf( "%d: create thread (%d) '%s'\n", gameLocal.time, threadNum, threadName.c_str() );
	}
}

/*
================
idThread::~idThread
================
*/
idThread::~idThread()
{
	idThread*	thread;
	int			i;
	int			n;

	if( g_debugScript.GetBool() )
	{
		gameLocal.Printf( "%d: end thread (%d) '%s'\n", gameLocal.time, threadNum, threadName.c_str() );
	}
	threadList.Remove( this );
	n = threadList.Num();
	for( i = 0; i < n; i++ )
	{
		thread = threadList[ i ];
		if( thread->WaitingOnThread() == this )
		{
			thread->ThreadCallback( this );
		}
	}

	if( currentThread == this )
	{
		currentThread = NULL;
	}
}

/*
================
idThread::ManualDelete
================
*/
void idThread::ManualDelete()
{
	interpreter.terminateOnExit = false;
}

/*
================
idThread::Save
================
*/
void idThread::Save( idSaveGame* savefile ) const
{

	// We will check on restore that threadNum is still the same,
	//  threads should have been restored in the same order.
	savefile->WriteInt( threadNum );

	savefile->WriteObject( waitingForThread );
	savefile->WriteInt( waitingFor );
	savefile->WriteInt( waitingUntil );

	interpreter.Save( savefile );

	savefile->WriteDict( &spawnArgs );
	savefile->WriteString( threadName );

	savefile->WriteInt( lastExecuteTime );
	savefile->WriteInt( creationTime );

	savefile->WriteBool( manualControl );
}

/*
================
idThread::Restore
================
*/
void idThread::Restore( idRestoreGame* savefile )
{
	savefile->ReadInt( threadNum );

	savefile->ReadObject( reinterpret_cast<idClass*&>( waitingForThread ) );
	savefile->ReadInt( waitingFor );
	savefile->ReadInt( waitingUntil );

	interpreter.Restore( savefile );

	savefile->ReadDict( &spawnArgs );
	savefile->ReadString( threadName );

	savefile->ReadInt( lastExecuteTime );
	savefile->ReadInt( creationTime );

	savefile->ReadBool( manualControl );
}

/*
================
idThread::Init
================
*/
void idThread::Init()
{
	// create a unique threadNum
	do
	{
		threadIndex++;
		if( threadIndex == 0 )
		{
			threadIndex = 1;
		}
	}
	while( GetThread( threadIndex ) );

	threadNum = threadIndex;
	threadList.Append( this );

	creationTime = gameLocal.time;
	lastExecuteTime = 0;
	manualControl = false;

	ClearWaitFor();

	interpreter.SetThread( this );
}

/*
================
idThread::GetThread
================
*/
idThread* idThread::GetThread( int num )
{
	int			i;
	int			n;
	idThread*	thread;

	n = threadList.Num();
	for( i = 0; i < n; i++ )
	{
		thread = threadList[ i ];
		if( thread->GetThreadNum() == num )
		{
			return thread;
		}
	}

	return NULL;
}

/*
================
idThread::DisplayInfo
================
*/
void idThread::DisplayInfo()
{
	gameLocal.Printf(
		"%12i: '%s'\n"
		"        File: %s(%d)\n"
		"     Created: %d (%d ms ago)\n"
		"      Status: ",
		threadNum, threadName.c_str(),
		interpreter.CurrentFile(), interpreter.CurrentLine(),
		creationTime, gameLocal.time - creationTime );

	if( interpreter.threadDying )
	{
		gameLocal.Printf( "Dying\n" );
	}
	else if( interpreter.doneProcessing )
	{
		gameLocal.Printf(
			"Paused since %d (%d ms)\n"
			"      Reason: ",  lastExecuteTime, gameLocal.time - lastExecuteTime );
		if( waitingForThread )
		{
			gameLocal.Printf( "Waiting for thread #%3i '%s'\n", waitingForThread->GetThreadNum(), waitingForThread->GetThreadName() );
		}
		else if( ( waitingFor != ENTITYNUM_NONE ) && ( waitingFor < MAX_GENTITIES ) && ( gameLocal.entities[ waitingFor ] ) )
		{
			gameLocal.Printf( "Waiting for entity #%3i '%s'\n", waitingFor, gameLocal.entities[ waitingFor ]->name.c_str() );
		}
		else if( waitingUntil )
		{
			gameLocal.Printf( "Waiting until %d (%d ms total wait time)\n", waitingUntil, waitingUntil - lastExecuteTime );
		}
		else
		{
			gameLocal.Printf( "None\n" );
		}
	}
	else
	{
		gameLocal.Printf( "Processing\n" );
	}

	interpreter.DisplayInfo();

	gameLocal.Printf( "\n" );
}

/*
================
idThread::ListThreads_f
================
*/
void idThread::ListThreads_f( const idCmdArgs& args )
{
	int	i;
	int	n;

	n = threadList.Num();
	for( i = 0; i < n; i++ )
	{
		//threadList[ i ]->DisplayInfo();
		gameLocal.Printf( "%3i: %-20s : %s(%d)\n", threadList[ i ]->threadNum, threadList[ i ]->threadName.c_str(), threadList[ i ]->interpreter.CurrentFile(), threadList[ i ]->interpreter.CurrentLine() );
	}
	gameLocal.Printf( "%d active threads\n\n", n );
}

/*
================
idThread::Restart
================
*/
void idThread::Restart()
{
	int	i;
	int	n;

	// reset the threadIndex
	threadIndex = 0;

	currentThread = NULL;
	n = threadList.Num();
	for( i = n - 1; i >= 0; i-- )
	{
		delete threadList[ i ];
	}
	threadList.Clear();

	memset( &trace, 0, sizeof( trace ) );
	trace.c.entityNum = ENTITYNUM_NONE;
}

/*
================
idThread::DelayedStart
================
*/
void idThread::DelayedStart( int delay )
{
	CancelEvents( &EV_Thread_Execute );
	if( gameLocal.time <= 0 )
	{
		delay++;
	}
	PostEventMS( &EV_Thread_Execute, delay );
}

/*
================
idThread::Start
================
*/
bool idThread::Start()
{
	bool result;

	CancelEvents( &EV_Thread_Execute );
	result = Execute();

	return result;
}

/*
================
idThread::SetThreadName
================
*/
void idThread::SetThreadName( const char* name )
{
	threadName = name;
}

/*
================
idThread::ObjectMoveDone
================
*/
void idThread::ObjectMoveDone( int threadnum, idEntity* obj )
{
	idThread* thread;

	if( !threadnum )
	{
		return;
	}

	thread = GetThread( threadnum );
	if( thread )
	{
		thread->ObjectMoveDone( obj );
	}
}

/*
================
idThread::End
================
*/
void idThread::End()
{
	// Tell thread to die.  It will exit on its own.
	Pause();
	interpreter.threadDying	= true;
}

/*
================
idThread::KillThread
================
*/
void idThread::KillThread( const char* name )
{
	int			i;
	int			num;
	int			len;
	const char*	ptr;
	idThread*	thread;

	// see if the name uses a wild card
	ptr = strchr( name, '*' );
	if( ptr )
	{
		len = ptr - name;
	}
	else
	{
		len = strlen( name );
	}

	// kill only those threads whose name matches name
	num = threadList.Num();
	for( i = 0; i < num; i++ )
	{
		thread = threadList[ i ];
		if( !idStr::Cmpn( thread->GetThreadName(), name, len ) )
		{
			thread->End();
		}
	}
}

/*
================
idThread::KillThread
================
*/
void idThread::KillThread( int num )
{
	idThread* thread;

	thread = GetThread( num );
	if( thread )
	{
		// Tell thread to die.  It will delete itself on it's own.
		thread->End();
	}
}

/*
================
idThread::Execute
================
*/
bool idThread::Execute()
{
	idThread*	oldThread;
	bool		done;

	if( manualControl && ( waitingUntil > gameLocal.time ) )
	{
		return false;
	}

	oldThread = currentThread;
	currentThread = this;

	lastExecuteTime = gameLocal.time;
	ClearWaitFor();
	done = interpreter.Execute();
	if( done )
	{
		End();
		if( interpreter.terminateOnExit )
		{
			PostEventMS( &EV_Remove, 0 );
		}
	}
	else if( !manualControl )
	{
		if( waitingUntil > lastExecuteTime )
		{
			PostEventMS( &EV_Thread_Execute, waitingUntil - lastExecuteTime );
		}
		else if( interpreter.MultiFrameEventInProgress() )
		{
			PostEventMS( &EV_Thread_Execute, 1 );
		}
	}

	currentThread = oldThread;

	return done;
}

/*
================
idThread::IsWaiting

Checks if thread is still waiting for some event to occur.
================
*/
bool idThread::IsWaiting()
{
	if( waitingForThread || ( waitingFor != ENTITYNUM_NONE ) )
	{
		return true;
	}

	if( waitingUntil && ( waitingUntil > gameLocal.time ) )
	{
		return true;
	}

	return false;
}

/*
================
idThread::CallFunction

NOTE: If this is called from within a event called by this thread, the function arguments will be invalid after calling this function.
================
*/
void idThread::CallFunction( const function_t* func, bool clearStack )
{
	ClearWaitFor();
	interpreter.EnterFunction( func, clearStack );
}

/*
================
idThread::CallFunction

NOTE: If this is called from within a event called by this thread, the function arguments will be invalid after calling this function.
================
*/
void idThread::CallFunction( idEntity* self, const function_t* func, bool clearStack )
{
	assert( self );
	ClearWaitFor();
	interpreter.EnterObjectFunction( self, func, clearStack );
}

/*
================
idThread::ClearWaitFor
================
*/
void idThread::ClearWaitFor()
{
	waitingFor			= ENTITYNUM_NONE;
	waitingForThread	= NULL;
	waitingUntil		= 0;
}

/*
================
idThread::IsWaitingFor
================
*/
bool idThread::IsWaitingFor( idEntity* obj )
{
	assert( obj );
	return waitingFor == obj->entityNumber;
}

/*
================
idThread::ObjectMoveDone
================
*/
void idThread::ObjectMoveDone( idEntity* obj )
{
	assert( obj );

	if( IsWaitingFor( obj ) )
	{
		ClearWaitFor();
		DelayedStart( 0 );
	}
}

/*
================
idThread::ThreadCallback
================
*/
void idThread::ThreadCallback( idThread* thread )
{
	if( interpreter.threadDying )
	{
		return;
	}

	if( thread == waitingForThread )
	{
		ClearWaitFor();
		DelayedStart( 0 );
	}
}

/*
================
idThread::Event_SetThreadName
================
*/
void idThread::Event_SetThreadName( const char* name )
{
	SetThreadName( name );
}

/*
================
idThread::Error
================
*/
void idThread::Error( const char* fmt, ... ) const
{
	va_list	argptr;
	char	text[ 1024 ];

	va_start( argptr, fmt );
	idStr::vsnPrintf( text, sizeof( text ), fmt, argptr );
	va_end( argptr );

	interpreter.Error( text );
}

/*
================
idThread::Warning
================
*/
void idThread::Warning( const char* fmt, ... ) const
{
	va_list	argptr;
	char	text[ 1024 ];

	va_start( argptr, fmt );
	idStr::vsnPrintf( text, sizeof( text ), fmt, argptr );
	va_end( argptr );

	interpreter.Warning( text );
}

/*
================
idThread::ReturnString
================
*/
void idThread::ReturnString( const char* text )
{
	gameLocal.program.ReturnString( text );
}

/*
================
idThread::ReturnFloat
================
*/
void idThread::ReturnFloat( float value )
{
	gameLocal.program.ReturnFloat( value );
}

/*
================
idThread::ReturnInt
================
*/
void idThread::ReturnInt( int value )
{
	// true integers aren't supported in the compiler,
	// so int values are stored as floats
	gameLocal.program.ReturnFloat( value );
}

/*
================
idThread::ReturnVector
================
*/
void idThread::ReturnVector( idVec3 const& vec )
{
	gameLocal.program.ReturnVector( vec );
}

/*
================
idThread::ReturnEntity
================
*/
void idThread::ReturnEntity( idEntity* ent )
{
	gameLocal.program.ReturnEntity( ent );
}

/*
================
idThread::Event_Execute
================
*/
void idThread::Event_Execute()
{
	Execute();
}

/*
================
idThread::Pause
================
*/
void idThread::Pause()
{
	ClearWaitFor();
	interpreter.doneProcessing = true;
}

/*
================
idThread::WaitMS
================
*/
void idThread::WaitMS( int time )
{
	Pause();
	waitingUntil = gameLocal.time + time;
}

/*
================
idThread::WaitSec
================
*/
void idThread::WaitSec( float time )
{
	WaitMS( SEC2MS( time ) );
}

/*
================
idThread::WaitFrame
================
*/
void idThread::WaitFrame()
{
	Pause();

	// manual control threads don't set waitingUntil so that they can be run again
	// that frame if necessary.
	if( !manualControl )
	{
		waitingUntil = gameLocal.time + 1;
	}
}

/***********************************************************************

  Script callable events

***********************************************************************/

/*
================
idThread::Event_TerminateThread
================
*/
void idThread::Event_TerminateThread( int num )
{
	idThread* thread;

	thread = GetThread( num );
	KillThread( num );
}

/*
================
idThread::Event_Pause
================
*/
void idThread::Event_Pause()
{
	Pause();
}

/*
================
idThread::Event_Wait
================
*/
void idThread::Event_Wait( float time )
{
	WaitSec( time );
}

/*
================
idThread::Event_WaitFrame
================
*/
void idThread::Event_WaitFrame()
{
	WaitFrame();
}

/*
================
idThread::Event_WaitFor
================
*/
void idThread::Event_WaitFor( idEntity* ent )
{
	if( ent && ent->RespondsTo( EV_Thread_SetCallback ) )
	{
		ent->ProcessEvent( &EV_Thread_SetCallback );
		if( gameLocal.program.GetReturnedInteger() )
		{
			Pause();
			waitingFor = ent->entityNumber;
		}
	}
}

/*
================
idThread::Event_WaitForThread
================
*/
void idThread::Event_WaitForThread( int num )
{
	idThread* thread;

	thread = GetThread( num );
	if( !thread )
	{
		if( g_debugScript.GetBool() )
		{
			// just print a warning and continue executing
			Warning( "Thread %d not running", num );
		}
	}
	else
	{
		Pause();
		waitingForThread = thread;
	}
}

/*
================
idThread::Event_Print
================
*/
void idThread::Event_Print( const char* text )
{
	gameLocal.Printf( "%s", text );
}

/*
================
idThread::Event_PrintLn
================
*/
void idThread::Event_PrintLn( const char* text )
{
	gameLocal.Printf( "%s\n", text );
}

/*
================
idThread::Event_Say
================
*/
void idThread::Event_Say( const char* text )
{
	cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "say \"%s\"", text ) );
}

/*
================
idThread::Event_Assert
================
*/
void idThread::Event_Assert( float value )
{
	assert( value );
}

/*
================
idThread::Event_Trigger
================
*/
void idThread::Event_Trigger( idEntity* ent )
{
	if( ent )
	{
		ent->Signal( SIG_TRIGGER );
		ent->ProcessEvent( &EV_Activate, gameLocal.GetLocalPlayer() );
		ent->TriggerGuis();
	}
}

/*
================
idThread::Event_SetCvar
================
*/
void idThread::Event_SetCvar( const char* name, const char* value ) const
{
	cvarSystem->SetCVarString( name, value );
}

/*
================
idThread::Event_GetCvar
================
*/
void idThread::Event_GetCvar( const char* name ) const
{
	ReturnString( cvarSystem->GetCVarString( name ) );
}

/*
================
idThread::Event_Random
================
*/
void idThread::Event_Random( float range ) const
{
	float result;

	result = gameLocal.random.RandomFloat();
	ReturnFloat( range * result );
}


void idThread::Event_RandomInt( int range ) const
{
	int result;
	result = gameLocal.random.RandomInt( range );
	ReturnFloat( result );
}


/*
================
idThread::Event_GetTime
================
*/
void idThread::Event_GetTime()
{

	ReturnFloat( MS2SEC( gameLocal.realClientTime ) );

	/*  Script always uses realClient time to determine scripty stuff. ( This Fixes Weapon Animation timing bugs )
	if ( common->IsMultiplayer() ) {
		ReturnFloat( MS2SEC( gameLocal.GetServerGameTimeMs() ) );
	} else {
		ReturnFloat( MS2SEC( gameLocal.realClientTime ) );
	}
	*/
}

/*
================
idThread::Event_KillThread
================
*/
void idThread::Event_KillThread( const char* name )
{
	KillThread( name );
}

/*
================
idThread::Event_GetEntity
================
*/
void idThread::Event_GetEntity( const char* name )
{
	int			entnum;
	idEntity*	ent;

	assert( name );

	if( name[ 0 ] == '*' )
	{
		entnum = atoi( &name[ 1 ] );
		if( ( entnum < 0 ) || ( entnum >= MAX_GENTITIES ) )
		{
			Error( "Entity number in string out of range." );
			return;
		}
		ReturnEntity( gameLocal.entities[ entnum ] );
	}
	else
	{
		ent = gameLocal.FindEntity( name );
		ReturnEntity( ent );
	}
}

/*
================
idThread::Event_Spawn
================
*/
void idThread::Event_Spawn( const char* classname )
{
	idEntity* ent;

	spawnArgs.Set( "classname", classname );
	gameLocal.SpawnEntityDef( spawnArgs, &ent );
	ReturnEntity( ent );
	spawnArgs.Clear();
}

/*
================
idThread::Event_CopySpawnArgs
================
*/
void idThread::Event_CopySpawnArgs( idEntity* ent )
{
	spawnArgs.Copy( ent->spawnArgs );
}

/*
================
idThread::Event_SetSpawnArg
================
*/
void idThread::Event_SetSpawnArg( const char* key, const char* value )
{
	spawnArgs.Set( key, value );
}

/*
================
idThread::Event_SpawnString
================
*/
void idThread::Event_SpawnString( const char* key, const char* defaultvalue )
{
	const char* result;

	spawnArgs.GetString( key, defaultvalue, &result );
	ReturnString( result );
}

/*
================
idThread::Event_SpawnFloat
================
*/
void idThread::Event_SpawnFloat( const char* key, float defaultvalue )
{
	float result;

	spawnArgs.GetFloat( key, va( "%f", defaultvalue ), result );
	ReturnFloat( result );
}

/*
================
idThread::Event_SpawnVector
================
*/
void idThread::Event_SpawnVector( const char* key, idVec3& defaultvalue )
{
	idVec3 result;

	spawnArgs.GetVector( key, va( "%f %f %f", defaultvalue.x, defaultvalue.y, defaultvalue.z ), result );
	ReturnVector( result );
}

/*
================
idThread::Event_ClearPersistantArgs
================
*/
void idThread::Event_ClearPersistantArgs()
{
	gameLocal.persistentLevelInfo.Clear();
}


/*
================
idThread::Event_SetPersistantArg
================
*/
void idThread::Event_SetPersistantArg( const char* key, const char* value )
{
	gameLocal.persistentLevelInfo.Set( key, value );
}

/*
================
idThread::Event_GetPersistantString
================
*/
void idThread::Event_GetPersistantString( const char* key )
{
	const char* result;

	gameLocal.persistentLevelInfo.GetString( key, "", &result );
	ReturnString( result );
}

/*
================
idThread::Event_GetPersistantFloat
================
*/
void idThread::Event_GetPersistantFloat( const char* key )
{
	float result;

	gameLocal.persistentLevelInfo.GetFloat( key, "0", result );
	ReturnFloat( result );
}

/*
================
idThread::Event_GetPersistantVector
================
*/
void idThread::Event_GetPersistantVector( const char* key )
{
	idVec3 result;

	gameLocal.persistentLevelInfo.GetVector( key, "0 0 0", result );
	ReturnVector( result );
}

/*
================
idThread::Event_AngToForward
================
*/
void idThread::Event_AngToForward( idAngles& ang )
{
	ReturnVector( ang.ToForward() );
}

/*
================
idThread::Event_AngToRight
================
*/
void idThread::Event_AngToRight( idAngles& ang )
{
	idVec3 vec;

	ang.ToVectors( NULL, &vec );
	ReturnVector( vec );
}

/*
================
idThread::Event_AngToUp
================
*/
void idThread::Event_AngToUp( idAngles& ang )
{
	idVec3 vec;

	ang.ToVectors( NULL, NULL, &vec );
	ReturnVector( vec );
}

/*
================
idThread::Event_GetSine
================
*/
void idThread::Event_GetSine( float angle )
{
	ReturnFloat( idMath::Sin( DEG2RAD( angle ) ) );
}

/*
================
idThread::Event_GetCosine
================
*/
void idThread::Event_GetCosine( float angle )
{
	ReturnFloat( idMath::Cos( DEG2RAD( angle ) ) );
}

/*
================
idThread::Event_GetArcSine
================
*/
void idThread::Event_GetArcSine( float a )
{
	ReturnFloat( RAD2DEG( idMath::ASin( a ) ) );
}

/*
================
idThread::Event_GetArcCosine
================
*/
void idThread::Event_GetArcCosine( float a )
{
	ReturnFloat( RAD2DEG( idMath::ACos( a ) ) );
}

/*
================
idThread::Event_GetSquareRoot
================
*/
void idThread::Event_GetSquareRoot( float theSquare )
{
	ReturnFloat( idMath::Sqrt( theSquare ) );
}

/*
================
idThread::Event_VecNormalize
================
*/
void idThread::Event_VecNormalize( idVec3& vec )
{
	idVec3 n;

	n = vec;
	n.Normalize();
	ReturnVector( n );
}

/*
================
idThread::Event_VecLength
================
*/
void idThread::Event_VecLength( idVec3& vec )
{
	ReturnFloat( vec.Length() );
}

/*
================
idThread::Event_VecDotProduct
================
*/
void idThread::Event_VecDotProduct( idVec3& vec1, idVec3& vec2 )
{
	ReturnFloat( vec1 * vec2 );
}

/*
================
idThread::Event_VecCrossProduct
================
*/
void idThread::Event_VecCrossProduct( idVec3& vec1, idVec3& vec2 )
{
	ReturnVector( vec1.Cross( vec2 ) );
}

/*
================
idThread::Event_VecToAngles
================
*/
void idThread::Event_VecToAngles( idVec3& vec )
{
	idAngles ang = vec.ToAngles();
	ReturnVector( idVec3( ang[0], ang[1], ang[2] ) );
}

/*
================
idThread::Event_VecToOrthoBasisAngles
================
*/
void idThread::Event_VecToOrthoBasisAngles( idVec3& vec )
{
	idVec3 left, up;
	idAngles ang;

	vec.OrthogonalBasis( left, up );
	idMat3 axis( left, up, vec );

	ang = axis.ToAngles();

	ReturnVector( idVec3( ang[0], ang[1], ang[2] ) );
}

void idThread::Event_RotateVector( idVec3& vec, idVec3& ang )
{

	idAngles tempAng( ang );
	idMat3 axis = tempAng.ToMat3();
	idVec3 ret = vec * axis;
	ReturnVector( ret );

}

/*
================
idThread::Event_OnSignal
================
*/
void idThread::Event_OnSignal( int signal, idEntity* ent, const char* func )
{
	const function_t* function;

	assert( func );

	if( ent == NULL )
	{
		Error( "Entity not found" );
		return;
	}

	if( ( signal < 0 ) || ( signal >= NUM_SIGNALS ) )
	{
		Error( "Signal out of range" );
	}

	function = gameLocal.program.FindFunction( func );
	if( !function )
	{
		Error( "Function '%s' not found", func );
	}

	ent->SetSignal( ( signalNum_t )signal, this, function );
}

/*
================
idThread::Event_ClearSignalThread
================
*/
void idThread::Event_ClearSignalThread( int signal, idEntity* ent )
{
	if( ent == NULL )
	{
		Error( "Entity not found" );
		return;
	}

	if( ( signal < 0 ) || ( signal >= NUM_SIGNALS ) )
	{
		Error( "Signal out of range" );
	}

	ent->ClearSignalThread( ( signalNum_t )signal, this );
}

/*
================
idThread::Event_SetCamera
================
*/
void idThread::Event_SetCamera( idEntity* ent )
{
	if( !ent )
	{
		Error( "Entity not found" );
		return;
	}

	if( !ent->IsType( idCamera::Type ) )
	{
		Error( "Entity is not a camera" );
		return;
	}

	gameLocal.SetCamera( ( idCamera* )ent );
}

/*
================
idThread::Event_FirstPerson
================
*/
void idThread::Event_FirstPerson()
{
	gameLocal.SetCamera( NULL );
}

/*
================
idThread::Event_Trace
================
*/
void idThread::Event_Trace( const idVec3& start, const idVec3& end, const idVec3& mins, const idVec3& maxs, int contents_mask, idEntity* passEntity )
{
	if( mins == vec3_origin && maxs == vec3_origin )
	{
		gameLocal.clip.TracePoint( trace, start, end, contents_mask, passEntity );
	}
	else
	{
		gameLocal.clip.TraceBounds( trace, start, end, idBounds( mins, maxs ), contents_mask, passEntity );
	}
	ReturnFloat( trace.fraction );
}

/*
================
idThread::Event_TracePoint
================
*/
void idThread::Event_TracePoint( const idVec3& start, const idVec3& end, int contents_mask, idEntity* passEntity )
{
	gameLocal.clip.TracePoint( trace, start, end, contents_mask, passEntity );
	ReturnFloat( trace.fraction );
}

/*
================
idThread::Event_GetTraceFraction
================
*/
void idThread::Event_GetTraceFraction()
{
	ReturnFloat( trace.fraction );
}

/*
================
idThread::Event_GetTraceEndPos
================
*/
void idThread::Event_GetTraceEndPos()
{
	ReturnVector( trace.endpos );
}

/*
================
idThread::Event_GetTraceNormal
================
*/
void idThread::Event_GetTraceNormal()
{
	if( trace.fraction < 1.0f )
	{
		ReturnVector( trace.c.normal );
	}
	else
	{
		ReturnVector( vec3_origin );
	}
}

/*
================
idThread::Event_GetTraceEntity
================
*/
void idThread::Event_GetTraceEntity()
{
	if( trace.fraction < 1.0f )
	{
		ReturnEntity( gameLocal.entities[ trace.c.entityNum ] );
	}
	else
	{
		ReturnEntity( ( idEntity* )NULL );
	}
}

/*
================
idThread::Event_GetTraceJoint
================
*/
void idThread::Event_GetTraceJoint()
{
	if( trace.fraction < 1.0f && trace.c.id < 0 )
	{
		idAFEntity_Base* af = static_cast<idAFEntity_Base*>( gameLocal.entities[ trace.c.entityNum ] );
		if( af && af->IsType( idAFEntity_Base::Type ) && af->IsActiveAF() )
		{
			ReturnString( af->GetAnimator()->GetJointName( CLIPMODEL_ID_TO_JOINT_HANDLE( trace.c.id ) ) );
			return;
		}
	}
	ReturnString( "" );
}

/*
================
idThread::Event_GetTraceBody
================
*/
void idThread::Event_GetTraceBody()
{
	if( trace.fraction < 1.0f && trace.c.id < 0 )
	{
		idAFEntity_Base* af = static_cast<idAFEntity_Base*>( gameLocal.entities[ trace.c.entityNum ] );
		if( af && af->IsType( idAFEntity_Base::Type ) && af->IsActiveAF() )
		{
			int bodyId = af->BodyForClipModelId( trace.c.id );
			idAFBody* body = af->GetAFPhysics()->GetBody( bodyId );
			if( body )
			{
				ReturnString( body->GetName() );
				return;
			}
		}
	}
	ReturnString( "" );
}

/*
================
idThread::Event_FadeIn
================
*/
void idThread::Event_FadeIn( idVec3& color, float time )
{
	idVec4		fadeColor;
	idPlayer*	player;

	player = gameLocal.GetLocalPlayer();
	if( player )
	{
		fadeColor.Set( color[ 0 ], color[ 1 ], color[ 2 ], 0.0f );
		player->playerView.Fade( fadeColor, SEC2MS( time ) );
	}
}

/*
================
idThread::Event_FadeOut
================
*/
void idThread::Event_FadeOut( idVec3& color, float time )
{
	idVec4		fadeColor;
	idPlayer*	player;

	player = gameLocal.GetLocalPlayer();
	if( player )
	{
		fadeColor.Set( color[ 0 ], color[ 1 ], color[ 2 ], 1.0f );
		player->playerView.Fade( fadeColor, SEC2MS( time ) );
	}
}

/*
================
idThread::Event_FadeTo
================
*/
void idThread::Event_FadeTo( idVec3& color, float alpha, float time )
{
	idVec4		fadeColor;
	idPlayer*	player;

	player = gameLocal.GetLocalPlayer();
	if( player )
	{
		fadeColor.Set( color[ 0 ], color[ 1 ], color[ 2 ], alpha );
		player->playerView.Fade( fadeColor, SEC2MS( time ) );
	}
}

/*
================
idThread::Event_SetShaderParm
================
*/
void idThread::Event_SetShaderParm( int parmnum, float value )
{
	if( ( parmnum < 0 ) || ( parmnum >= MAX_GLOBAL_SHADER_PARMS ) )
	{
		Error( "shader parm index (%d) out of range", parmnum );
		return;
	}

	gameLocal.globalShaderParms[ parmnum ] = value;
}

/*
================
idThread::Event_StartMusic
================
*/
void idThread::Event_StartMusic( const char* text )
{
	// RB: this should go into SND_CHANNEL_MUSIC and might conflict with the logic from worldspawn
	// it would be better to have a music manager instance that fades into this track until done and then resumes the old track
	gameSoundWorld->PlayShaderDirectly( text );//, SND_CHANNEL_MUSIC );
}

/*
================
idThread::Event_Warning
================
*/
void idThread::Event_Warning( const char* text )
{
	Warning( "%s", text );
}

/*
================
idThread::Event_Error
================
*/
void idThread::Event_Error( const char* text )
{
	Error( "%s", text );
}

/*
================
idThread::Event_StrLen
================
*/
void idThread::Event_StrLen( const char* string )
{
	int len;

	len = strlen( string );
	idThread::ReturnInt( len );
}

/*
================
idThread::Event_StrLeft
================
*/
void idThread::Event_StrLeft( const char* string, int num )
{
	int len;

	if( num < 0 )
	{
		idThread::ReturnString( "" );
		return;
	}

	len = strlen( string );
	if( len < num )
	{
		idThread::ReturnString( string );
		return;
	}

	idStr result( string, 0, num );
	idThread::ReturnString( result );
}

/*
================
idThread::Event_StrRight
================
*/
void idThread::Event_StrRight( const char* string, int num )
{
	int len;

	if( num < 0 )
	{
		idThread::ReturnString( "" );
		return;
	}

	len = strlen( string );
	if( len < num )
	{
		idThread::ReturnString( string );
		return;
	}

	idThread::ReturnString( string + len - num );
}

/*
================
idThread::Event_StrSkip
================
*/
void idThread::Event_StrSkip( const char* string, int num )
{
	int len;

	if( num < 0 )
	{
		idThread::ReturnString( string );
		return;
	}

	len = strlen( string );
	if( len < num )
	{
		idThread::ReturnString( "" );
		return;
	}

	idThread::ReturnString( string + num );
}

/*
================
idThread::Event_StrMid
================
*/
void idThread::Event_StrMid( const char* string, int start, int num )
{
	int len;

	if( num < 0 )
	{
		idThread::ReturnString( "" );
		return;
	}

	if( start < 0 )
	{
		start = 0;
	}
	len = strlen( string );
	if( start > len )
	{
		start = len;
	}

	if( start + num > len )
	{
		num = len - start;
	}

	idStr result( string, start, start + num );
	idThread::ReturnString( result );
}

/*
================
idThread::Event_StrToFloat( const char *string )
================
*/
void idThread::Event_StrToFloat( const char* string )
{
	float result;

	result = atof( string );
	idThread::ReturnFloat( result );
}

/*
================
idThread::Event_RadiusDamage
================
*/
void idThread::Event_RadiusDamage( const idVec3& origin, idEntity* inflictor, idEntity* attacker, idEntity* ignore, const char* damageDefName, float dmgPower )
{
	gameLocal.RadiusDamage( origin, inflictor, attacker, ignore, ignore, damageDefName, dmgPower );
}

/*
================
idThread::Event_IsClient
================
*/
void idThread::Event_IsClient()
{
	idThread::ReturnFloat( common->IsClient() );
}

/*
================
idThread::Event_IsMultiplayer
================
*/
void idThread::Event_IsMultiplayer()
{
	idThread::ReturnFloat( common->IsMultiplayer() );
}

/*
================
idThread::Event_GetFrameTime
================
*/
void idThread::Event_GetFrameTime()
{
	idThread::ReturnFloat( MS2SEC( gameLocal.time - gameLocal.previousTime ) );
}

/*
================
idThread::Event_GetTicsPerSecond
================
*/
void idThread::Event_GetTicsPerSecond()
{
	idThread::ReturnFloat( com_engineHz_latched );
}

/*
================
idThread::Event_CacheSoundShader
================
*/
void idThread::Event_CacheSoundShader( const char* soundName )
{
	declManager->FindSound( soundName );
}

/*
================
idThread::Event_DebugLine
================
*/
void idThread::Event_DebugLine( const idVec3& color, const idVec3& start, const idVec3& end, const float lifetime )
{
	gameRenderWorld->DebugLine( idVec4( color.x, color.y, color.z, 0.0f ), start, end, SEC2MS( lifetime ) );
}

/*
================
idThread::Event_DebugArrow
================
*/
void idThread::Event_DebugArrow( const idVec3& color, const idVec3& start, const idVec3& end, const int size, const float lifetime )
{
	gameRenderWorld->DebugArrow( idVec4( color.x, color.y, color.z, 0.0f ), start, end, size, SEC2MS( lifetime ) );
}

/*
================
idThread::Event_DebugCircle
================
*/
void idThread::Event_DebugCircle( const idVec3& color, const idVec3& origin, const idVec3& dir, const float radius, const int numSteps, const float lifetime )
{
	gameRenderWorld->DebugCircle( idVec4( color.x, color.y, color.z, 0.0f ), origin, dir, radius, numSteps, SEC2MS( lifetime ) );
}

/*
================
idThread::Event_DebugBounds
================
*/
void idThread::Event_DebugBounds( const idVec3& color, const idVec3& mins, const idVec3& maxs, const float lifetime )
{
	gameRenderWorld->DebugBounds( idVec4( color.x, color.y, color.z, 0.0f ), idBounds( mins, maxs ), vec3_origin, SEC2MS( lifetime ) );
}

/*
================
idThread::Event_DrawText
================
*/
void idThread::Event_DrawText( const char* text, const idVec3& origin, float scale, const idVec3& color, const int align, const float lifetime )
{
	gameRenderWorld->DrawText( text, origin, scale, idVec4( color.x, color.y, color.z, 0.0f ), gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), align, SEC2MS( lifetime ) );
}

/*
================
idThread::Event_InfluenceActive
================
*/
void idThread::Event_InfluenceActive()
{
	idPlayer* player;

	player = gameLocal.GetLocalPlayer();
	if( player != NULL && player->GetInfluenceLevel() )
	{
		idThread::ReturnInt( true );
	}
	else
	{
		idThread::ReturnInt( false );
	}
}
