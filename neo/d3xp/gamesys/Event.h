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
/*
sys_event.h

Event are used for scheduling tasks and for linking script commands.
*/
#ifndef __SYS_EVENT_H__
#define __SYS_EVENT_H__

#define D_EVENT_MAXARGS				8			// if changed, enable the CREATE_EVENT_CODE define in Event.cpp to generate switch statement for idClass::ProcessEventArgPtr.
// running the game will then generate c:\doom\base\events.txt, the contents of which should be copied into the switch statement.

// RB: from dhewm3
// stack size of idVec3, aligned to native pointer size
#define E_EVENT_SIZEOF_VEC			((sizeof(idVec3) + (sizeof(intptr_t) - 1)) & ~(sizeof(intptr_t) - 1))
// RB end

#define D_EVENT_VOID				( ( char )0 )
#define D_EVENT_INTEGER				'd'
#define D_EVENT_FLOAT				'f'
#define D_EVENT_VECTOR				'v'
#define D_EVENT_STRING				's'
#define D_EVENT_ENTITY				'e'
#define	D_EVENT_ENTITY_NULL			'E'			// event can handle NULL entity pointers
#define D_EVENT_TRACE				't'

#define MAX_EVENTS					4096

class idClass;
class idTypeInfo;

class idEventDef
{
private:
	const char*					name;
	const char*					formatspec;
	unsigned int				formatspecIndex;
	int							returnType;
	int							numargs;
	size_t						argsize;
	int							argOffset[ D_EVENT_MAXARGS ];
	int							eventnum;
	const idEventDef* 			next;

	static idEventDef* 			eventDefList[MAX_EVENTS];
	static int					numEventDefs;

public:
	idEventDef( const char* command, const char* formatspec = NULL, char returnType = 0 );

	const char*					GetName() const;
	const char*					GetArgFormat() const;
	unsigned int				GetFormatspecIndex() const;
	char						GetReturnType() const;
	int							GetEventNum() const;
	int							GetNumArgs() const;
	size_t						GetArgSize() const;
	int							GetArgOffset( int arg ) const;

	static int					NumEventCommands();
	static const idEventDef*		GetEventCommand( int eventnum );
	static const idEventDef*		FindEvent( const char* name );
};

class idSaveGame;
class idRestoreGame;

class idEvent
{
private:
	const idEventDef*			eventdef;
	byte*						data;
	int							time;
	idClass*						object;
	const idTypeInfo*			typeinfo;

	idLinkList<idEvent>			eventNode;

	static idDynamicBlockAlloc<byte, 16 * 1024, 256> eventDataAllocator;


public:
	static bool					initialized;

	~idEvent();

	static idEvent*				Alloc( const idEventDef* evdef, int numargs, va_list args );
	// RB: 64 bit fix, changed int to intptr_t
	static void					CopyArgs( const idEventDef* evdef, int numargs, va_list args, intptr_t data[ D_EVENT_MAXARGS ] );
	// RB end

	void						Free();
	void						Schedule( idClass* object, const idTypeInfo* cls, int time );
	byte*						GetData();

	static void					CancelEvents( const idClass* obj, const idEventDef* evdef = NULL );
	static void					ClearEventList();
	static void					ServiceEvents();
	static void					ServiceFastEvents();
	static void					Init();
	static void					Shutdown();

	// save games
	static void					Save( idSaveGame* savefile );					// archives object for save game file
	static void					Restore( idRestoreGame* savefile );				// unarchives object from save game file
	static void					SaveTrace( idSaveGame* savefile, const trace_t& trace );
	static void					RestoreTrace( idRestoreGame* savefile, trace_t& trace );

};

/*
================
idEvent::GetData
================
*/
ID_INLINE byte* idEvent::GetData()
{
	return data;
}

/*
================
idEventDef::GetName
================
*/
ID_INLINE const char* idEventDef::GetName() const
{
	return name;
}

/*
================
idEventDef::GetArgFormat
================
*/
ID_INLINE const char* idEventDef::GetArgFormat() const
{
	return formatspec;
}

/*
================
idEventDef::GetFormatspecIndex
================
*/
ID_INLINE unsigned int idEventDef::GetFormatspecIndex() const
{
	return formatspecIndex;
}

/*
================
idEventDef::GetReturnType
================
*/
ID_INLINE char idEventDef::GetReturnType() const
{
	return returnType;
}

/*
================
idEventDef::GetNumArgs
================
*/
ID_INLINE int idEventDef::GetNumArgs() const
{
	return numargs;
}

/*
================
idEventDef::GetArgSize
================
*/
ID_INLINE size_t idEventDef::GetArgSize() const
{
	return argsize;
}

/*
================
idEventDef::GetArgOffset
================
*/
ID_INLINE int idEventDef::GetArgOffset( int arg ) const
{
	assert( ( arg >= 0 ) && ( arg < D_EVENT_MAXARGS ) );
	return argOffset[ arg ];
}

/*
================
idEventDef::GetEventNum
================
*/
ID_INLINE int idEventDef::GetEventNum() const
{
	return eventnum;
}

#endif /* !__SYS_EVENT_H__ */
