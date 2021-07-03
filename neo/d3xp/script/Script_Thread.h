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

#ifndef __SCRIPT_THREAD_H__
#define __SCRIPT_THREAD_H__

extern const idEventDef EV_Thread_Execute;
extern const idEventDef EV_Thread_SetCallback;
extern const idEventDef EV_Thread_TerminateThread;
extern const idEventDef EV_Thread_Pause;
extern const idEventDef EV_Thread_Wait;
extern const idEventDef EV_Thread_WaitFrame;
extern const idEventDef EV_Thread_WaitFor;
extern const idEventDef EV_Thread_WaitForThread;
extern const idEventDef EV_Thread_Print;
extern const idEventDef EV_Thread_PrintLn;
extern const idEventDef EV_Thread_Say;
extern const idEventDef EV_Thread_Assert;
extern const idEventDef EV_Thread_Trigger;
extern const idEventDef EV_Thread_SetCvar;
extern const idEventDef EV_Thread_GetCvar;
extern const idEventDef EV_Thread_Random;
extern const idEventDef EV_Thread_GetTime;
extern const idEventDef EV_Thread_KillThread;
extern const idEventDef EV_Thread_SetThreadName;
extern const idEventDef EV_Thread_GetEntity;
extern const idEventDef EV_Thread_Spawn;
extern const idEventDef EV_Thread_SetSpawnArg;
extern const idEventDef EV_Thread_SpawnString;
extern const idEventDef EV_Thread_SpawnFloat;
extern const idEventDef EV_Thread_SpawnVector;
extern const idEventDef EV_Thread_AngToForward;
extern const idEventDef EV_Thread_AngToRight;
extern const idEventDef EV_Thread_AngToUp;
extern const idEventDef EV_Thread_Sine;
extern const idEventDef EV_Thread_Cosine;
extern const idEventDef EV_Thread_Normalize;
extern const idEventDef EV_Thread_VecLength;
extern const idEventDef EV_Thread_VecDotProduct;
extern const idEventDef EV_Thread_VecCrossProduct;
extern const idEventDef EV_Thread_OnSignal;
extern const idEventDef EV_Thread_ClearSignal;
extern const idEventDef EV_Thread_SetCamera;
extern const idEventDef EV_Thread_FirstPerson;
extern const idEventDef EV_Thread_TraceFraction;
extern const idEventDef EV_Thread_TracePos;
extern const idEventDef EV_Thread_FadeIn;
extern const idEventDef EV_Thread_FadeOut;
extern const idEventDef EV_Thread_FadeTo;
extern const idEventDef EV_Thread_Restart;

class idThread : public idClass
{
private:
	static idThread*				currentThread;

	idThread*					waitingForThread;
	int							waitingFor;
	int							waitingUntil;
	idInterpreter				interpreter;

	idDict						spawnArgs;

	int 						threadNum;
	idStr 						threadName;

	int							lastExecuteTime;
	int							creationTime;

	bool						manualControl;

	static int					threadIndex;
	static idList<idThread*, TAG_THREAD>	threadList;

	static trace_t				trace;

	void						Init();
	void						Pause();

	void						Event_Execute();
	void						Event_SetThreadName( const char* name );

	//
	// script callable Events
	//
	void						Event_TerminateThread( int num );
	void						Event_Pause();
	void						Event_Wait( float time );
	void						Event_WaitFrame();
	void						Event_WaitFor( idEntity* ent );
	void						Event_WaitForThread( int num );
	void						Event_Print( const char* text );
	void						Event_PrintLn( const char* text );
	void						Event_Say( const char* text );
	void						Event_Assert( float value );
	void						Event_Trigger( idEntity* ent );
	void						Event_SetCvar( const char* name, const char* value ) const;
	void						Event_GetCvar( const char* name ) const;
	void						Event_Random( float range ) const;
	void						Event_RandomInt( int range ) const;
	void						Event_GetTime();
	void						Event_KillThread( const char* name );
	void						Event_GetEntity( const char* name );
	void						Event_Spawn( const char* classname );
	void						Event_CopySpawnArgs( idEntity* ent );
	void						Event_SetSpawnArg( const char* key, const char* value );
	void						Event_SpawnString( const char* key, const char* defaultvalue );
	void						Event_SpawnFloat( const char* key, float defaultvalue );
	void						Event_SpawnVector( const char* key, idVec3& defaultvalue );
	void						Event_ClearPersistantArgs();
	void 						Event_SetPersistantArg( const char* key, const char* value );
	void 						Event_GetPersistantString( const char* key );
	void 						Event_GetPersistantFloat( const char* key );
	void 						Event_GetPersistantVector( const char* key );
	void						Event_AngToForward( idAngles& ang );
	void						Event_AngToRight( idAngles& ang );
	void						Event_AngToUp( idAngles& ang );
	void						Event_GetSine( float angle );
	void						Event_GetCosine( float angle );
	void						Event_GetArcSine( float a );
	void						Event_GetArcCosine( float a );
	void						Event_GetSquareRoot( float theSquare );
	void						Event_VecNormalize( idVec3& vec );
	void						Event_VecLength( idVec3& vec );
	void						Event_VecDotProduct( idVec3& vec1, idVec3& vec2 );
	void						Event_VecCrossProduct( idVec3& vec1, idVec3& vec2 );
	void						Event_VecToAngles( idVec3& vec );
	void						Event_VecToOrthoBasisAngles( idVec3& vec );
	void						Event_RotateVector( idVec3& vec, idVec3& ang );
	void						Event_OnSignal( int signal, idEntity* ent, const char* func );
	void						Event_ClearSignalThread( int signal, idEntity* ent );
	void						Event_SetCamera( idEntity* ent );
	void						Event_FirstPerson();
	void						Event_Trace( const idVec3& start, const idVec3& end, const idVec3& mins, const idVec3& maxs, int contents_mask, idEntity* passEntity );
	void						Event_TracePoint( const idVec3& start, const idVec3& end, int contents_mask, idEntity* passEntity );
	void						Event_GetTraceFraction();
	void						Event_GetTraceEndPos();
	void						Event_GetTraceNormal();
	void						Event_GetTraceEntity();
	void						Event_GetTraceJoint();
	void						Event_GetTraceBody();
	void						Event_FadeIn( idVec3& color, float time );
	void						Event_FadeOut( idVec3& color, float time );
	void						Event_FadeTo( idVec3& color, float alpha, float time );
	void						Event_SetShaderParm( int parmnum, float value );
	void						Event_StartMusic( const char* name );
	void						Event_Warning( const char* text );
	void						Event_Error( const char* text );
	void 						Event_StrLen( const char* string );
	void 						Event_StrLeft( const char* string, int num );
	void 						Event_StrRight( const char* string, int num );
	void 						Event_StrSkip( const char* string, int num );
	void 						Event_StrMid( const char* string, int start, int num );
	void						Event_StrToFloat( const char* string );
	void						Event_RadiusDamage( const idVec3& origin, idEntity* inflictor, idEntity* attacker, idEntity* ignore, const char* damageDefName, float dmgPower );
	void						Event_IsClient();
	void 						Event_IsMultiplayer();
	void 						Event_GetFrameTime();
	void 						Event_GetTicsPerSecond();
	void						Event_CacheSoundShader( const char* soundName );
	void						Event_DebugLine( const idVec3& color, const idVec3& start, const idVec3& end, const float lifetime );
	void						Event_DebugArrow( const idVec3& color, const idVec3& start, const idVec3& end, const int size, const float lifetime );
	void						Event_DebugCircle( const idVec3& color, const idVec3& origin, const idVec3& dir, const float radius, const int numSteps, const float lifetime );
	void						Event_DebugBounds( const idVec3& color, const idVec3& mins, const idVec3& maxs, const float lifetime );
	void						Event_DrawText( const char* text, const idVec3& origin, float scale, const idVec3& color, const int align, const float lifetime );
	void						Event_InfluenceActive();

public:
	CLASS_PROTOTYPE( idThread );

	idThread();
	idThread( idEntity* self, const function_t* func );
	idThread( const function_t* func );
	idThread( idInterpreter* source, const function_t* func, int args );
	idThread( idInterpreter* source, idEntity* self, const function_t* func, int args );

	virtual						~idThread();

	// tells the thread manager not to delete this thread when it ends
	void						ManualDelete();

	// save games
	void						Save( idSaveGame* savefile ) const;				// archives object for save game file
	void						Restore( idRestoreGame* savefile );				// unarchives object from save game file

	void						EnableDebugInfo()
	{
		interpreter.debug = true;
	};
	void						DisableDebugInfo()
	{
		interpreter.debug = false;
	};

	void						WaitMS( int time );
	void						WaitSec( float time );
	void						WaitFrame();

	// NOTE: If this is called from within a event called by this thread, the function arguments will be invalid after calling this function.
	void						CallFunction( const function_t*	func, bool clearStack );

	// NOTE: If this is called from within a event called by this thread, the function arguments will be invalid after calling this function.
	void						CallFunction( idEntity* obj, const function_t* func, bool clearStack );

	void						DisplayInfo();
	static idThread*				GetThread( int num );
	static void					ListThreads_f( const idCmdArgs& args );
	static void					Restart();
	static void					ObjectMoveDone( int threadnum, idEntity* obj );

	static idList<idThread*>&	GetThreads();

	bool						IsDoneProcessing();
	bool						IsDying();

	void						End();
	static void					KillThread( const char* name );
	static void					KillThread( int num );
	bool						Execute();
	void						ManualControl()
	{
		manualControl = true;
		CancelEvents( &EV_Thread_Execute );
	};
	void						DoneProcessing()
	{
		interpreter.doneProcessing = true;
	};
	void						ContinueProcessing()
	{
		interpreter.doneProcessing = false;
	};
	bool						ThreadDying()
	{
		return interpreter.threadDying;
	};
	void						EndThread()
	{
		interpreter.threadDying = true;
	};
	bool						IsWaiting();
	void						ClearWaitFor();
	bool						IsWaitingFor( idEntity* obj );
	void						ObjectMoveDone( idEntity* obj );
	void						ThreadCallback( idThread* thread );
	void						DelayedStart( int delay );
	bool						Start();
	idThread*					WaitingOnThread();
	void						SetThreadNum( int num );
	int 						GetThreadNum();
	void						SetThreadName( const char* name );
	const char*					GetThreadName();

	void						Error( VERIFY_FORMAT_STRING const char* fmt, ... ) const;
	void						Warning( VERIFY_FORMAT_STRING const char* fmt, ... ) const;

	static idThread*				CurrentThread();
	static int					CurrentThreadNum();
	static bool					BeginMultiFrameEvent( idEntity* ent, const idEventDef* event );
	static void					EndMultiFrameEvent( idEntity* ent, const idEventDef* event );

	static void					ReturnString( const char* text );
	static void					ReturnFloat( float value );
	static void					ReturnInt( int value );
	static void					ReturnVector( idVec3 const& vec );
	static void					ReturnEntity( idEntity* ent );
};

/*
================
idThread::WaitingOnThread
================
*/
ID_INLINE idThread* idThread::WaitingOnThread()
{
	return waitingForThread;
}

/*
================
idThread::SetThreadNum
================
*/
ID_INLINE void idThread::SetThreadNum( int num )
{
	threadNum = num;
}

/*
================
idThread::GetThreadNum
================
*/
ID_INLINE int idThread::GetThreadNum()
{
	return threadNum;
}

/*
================
idThread::GetThreadName
================
*/
ID_INLINE const char* idThread::GetThreadName()
{
	return threadName.c_str();
}

/*
================
idThread::GetThreads
================
*/
ID_INLINE idList<idThread*>& idThread::GetThreads()
{
	return threadList;
}

/*
================
idThread::IsDoneProcessing
================
*/
ID_INLINE bool idThread::IsDoneProcessing()
{
	return interpreter.doneProcessing;
}

/*
================
idThread::IsDying
================
*/
ID_INLINE bool idThread::IsDying()
{
	return interpreter.threadDying;
}

#endif /* !__SCRIPT_THREAD_H__ */
