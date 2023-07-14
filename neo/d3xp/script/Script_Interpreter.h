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

#ifndef __SCRIPT_INTERPRETER_H__
#define __SCRIPT_INTERPRETER_H__

#define MAX_STACK_DEPTH 	64

// RB: doubled local stack size
#define LOCALSTACK_SIZE 	(6144 * 2)
// RB end

typedef struct prstack_s
{
	int 				s;
	const function_t*	f;
	int 				stackbase;
} prstack_t;

class idInterpreter
{
private:
	prstack_t			callStack[ MAX_STACK_DEPTH ];
	int 				callStackDepth;
	int 				maxStackDepth;

	byte				localstack[ LOCALSTACK_SIZE ];
	int 				localstackUsed;
	int 				localstackBase;
	int 				maxLocalstackUsed;

	const function_t*	currentFunction;
	int 				instructionPointer;

	int					popParms;
	const idEventDef*	multiFrameEvent;
	idEntity*			eventEntity;

	idThread*			thread;

	void				PopParms( int numParms );

	// RB begin
	// RB: 64 bit fix, changed int to intptr_t
public:
	void				PushString( const char* string );
	void				Push( intptr_t value );
private:
	// RB: added PushVector for new E_EVENT_SIZEOF_VEC
	void				PushVector( const idVec3& vector );
	// RB end
	const char*			FloatToString( float value );
	void				AppendString( idVarDef* def, const char* from );
	void				SetString( idVarDef* def, const char* from );
	const char*			GetString( idVarDef* def );
	varEval_t			GetVariable( idVarDef* def );
	idEntity*			GetEntity( int entnum ) const;
	idScriptObject*		GetScriptObject( int entnum ) const;
	void				NextInstruction( int position );

	void				LeaveFunction( idVarDef* returnDef );
	void				CallEvent( const function_t* func, int argsize );
	void				CallSysEvent( const function_t* func, int argsize );

public:
	bool				doneProcessing;
	bool				threadDying;
	bool				terminateOnExit;
	bool				debug;

	idInterpreter();

	// save games
	void				Save( idSaveGame* savefile ) const;				// archives object for save game file
	void				Restore( idRestoreGame* savefile );				// unarchives object from save game file

	void				SetThread( idThread* pThread );

	void				StackTrace() const;

	int					CurrentLine() const;
	const char*			CurrentFile() const;

	void				Error( VERIFY_FORMAT_STRING const char* fmt, ... ) const;
	void				Warning( VERIFY_FORMAT_STRING const char* fmt, ... ) const;
	void				DisplayInfo() const;

	bool				BeginMultiFrameEvent( idEntity* ent, const idEventDef* event );
	void				EndMultiFrameEvent( idEntity* ent, const idEventDef* event );
	bool				MultiFrameEventInProgress() const;

	void				ThreadCall( idInterpreter* source, const function_t* func, int args );
	void				EnterFunction( const function_t* func, bool clearStack );
	void				EnterObjectFunction( idEntity* self, const function_t* func, bool clearStack );

	bool				Execute();
	void				Reset();

	bool				GetRegisterValue( const char* name, idStr& out, int scopeDepth );
	int					GetCallstackDepth() const;
	const prstack_t*		GetCallstack() const;
	const function_t*	GetCurrentFunction() const;
	idThread*			GetThread() const;

};

/*
====================
idInterpreter::PopParms
====================
*/
ID_INLINE void idInterpreter::PopParms( int numParms )
{
	// pop our parms off the stack
	if( localstackUsed < numParms )
	{
		Error( "locals stack underflow\n" );
	}

	localstackUsed -= numParms;
}

/*
====================
idInterpreter::Push
====================
*/
// RB: 64 bit fix, changed int to intptr_t
ID_INLINE void idInterpreter::Push( intptr_t value )
{
	if( localstackUsed + sizeof( intptr_t ) > LOCALSTACK_SIZE )
	{
		Error( "Push: locals stack overflow\n" );
	}
	*( intptr_t* )&localstack[ localstackUsed ] = value;
	localstackUsed += sizeof( intptr_t );
}
// RB end

// RB begin
/*
====================
idInterpreter::PushVector
====================
*/
ID_INLINE void idInterpreter::PushVector( const idVec3& vector )
{
	if( localstackUsed + E_EVENT_SIZEOF_VEC > LOCALSTACK_SIZE )
	{
		Error( "Push: locals stack overflow\n" );
	}
	*( idVec3* )&localstack[ localstackUsed ] = vector;
	localstackUsed += E_EVENT_SIZEOF_VEC;
}
// RB end


/*
====================
idInterpreter::PushString
====================
*/
ID_INLINE void idInterpreter::PushString( const char* string )
{
	if( localstackUsed + MAX_STRING_LEN > LOCALSTACK_SIZE )
	{
		Error( "PushString: locals stack overflow\n" );
	}
	idStr::Copynz( ( char* )&localstack[ localstackUsed ], string, MAX_STRING_LEN );
	localstackUsed += MAX_STRING_LEN;
}

/*
====================
idInterpreter::FloatToString
====================
*/
ID_INLINE const char* idInterpreter::FloatToString( float value )
{
	static char	text[ 32 ];

	if( value == ( float )( int )value )
	{
		idStr::snPrintf( text, sizeof( text ), "%d", ( int )value );
	}
	else
	{
		idStr::snPrintf( text, sizeof( text ), "%f", value );
	}
	return text;
}

/*
====================
idInterpreter::AppendString
====================
*/
ID_INLINE void idInterpreter::AppendString( idVarDef* def, const char* from )
{
	if( def->initialized == idVarDef::stackVariable )
	{
		idStr::Append( ( char* )&localstack[ localstackBase + def->value.stackOffset ], MAX_STRING_LEN, from );
	}
	else
	{
		idStr::Append( def->value.stringPtr, MAX_STRING_LEN, from );
	}
}

/*
====================
idInterpreter::SetString
====================
*/
ID_INLINE void idInterpreter::SetString( idVarDef* def, const char* from )
{
	if( def->initialized == idVarDef::stackVariable )
	{
		idStr::Copynz( ( char* )&localstack[ localstackBase + def->value.stackOffset ], from, MAX_STRING_LEN );
	}
	else
	{
		idStr::Copynz( def->value.stringPtr, from, MAX_STRING_LEN );
	}
}

/*
====================
idInterpreter::GetString
====================
*/
ID_INLINE const char* idInterpreter::GetString( idVarDef* def )
{
	if( def->initialized == idVarDef::stackVariable )
	{
		return ( char* )&localstack[ localstackBase + def->value.stackOffset ];
	}
	else
	{
		return def->value.stringPtr;
	}
}

/*
====================
idInterpreter::GetVariable
====================
*/
ID_INLINE varEval_t idInterpreter::GetVariable( idVarDef* def )
{
	if( def->initialized == idVarDef::stackVariable )
	{
		varEval_t val;
		val.intPtr = ( int* )&localstack[ localstackBase + def->value.stackOffset ];
		return val;
	}
	else
	{
		return def->value;
	}
}

/*
====================
idInterpreter::NextInstruction
====================
*/
ID_INLINE void idInterpreter::NextInstruction( int position )
{
	// Before we execute an instruction, we increment instructionPointer,
	// therefore we need to compensate for that here.
	instructionPointer = position - 1;
}

#endif /* !__SCRIPT_INTERPRETER_H__ */
