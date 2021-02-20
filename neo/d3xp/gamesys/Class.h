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

Base class for all game objects.  Provides fast run-time type checking and run-time
instancing of objects.

*/

#ifndef __SYS_CLASS_H__
#define __SYS_CLASS_H__

class idClass;
class idTypeInfo;

extern const idEventDef EV_Remove;
extern const idEventDef EV_SafeRemove;

typedef void ( idClass::*eventCallback_t )();

template< class Type >
struct idEventFunc
{
	const idEventDef*	event;
	eventCallback_t		function;
};

// added & so gcc could compile this
#define EVENT( event, function )	{ &( event ), ( void ( idClass::* )() )( &function ) },
#define END_CLASS					{ NULL, NULL } };


class idEventArg
{
public:
	int			type;
	// RB: 64 bit fix, changed int to intptr_t
	intptr_t	value;
	// RB end

	idEventArg()
	{
		type = D_EVENT_INTEGER;
		value = 0;
	};
	idEventArg( int data )
	{
		type = D_EVENT_INTEGER;
		value = data;
	};
	idEventArg( float data )
	{
		type = D_EVENT_FLOAT;
		value = *reinterpret_cast<int*>( &data );
	};
	// RB: 64 bit fixes, changed int to intptr_t
	idEventArg( idVec3& data )
	{
		type = D_EVENT_VECTOR;
		value = reinterpret_cast<intptr_t>( &data );
	};
	idEventArg( const idStr& data )
	{
		type = D_EVENT_STRING;
		value = reinterpret_cast<intptr_t>( data.c_str() );
	};
	idEventArg( const char* data )
	{
		type = D_EVENT_STRING;
		value = reinterpret_cast<intptr_t>( data );
	};
	idEventArg( const class idEntity* data )
	{
		type = D_EVENT_ENTITY;
		value = reinterpret_cast<intptr_t>( data );
	};
	idEventArg( const struct trace_s* data )
	{
		type = D_EVENT_TRACE;
		value = reinterpret_cast<intptr_t>( data );
	};
	// RB end
};

class idAllocError : public idException
{
public:
	idAllocError( const char* text = "" ) : idException( text ) {}
};

/***********************************************************************

  idClass

***********************************************************************/

/*
================
CLASS_PROTOTYPE

This macro must be included in the definition of any subclass of idClass.
It prototypes variables used in class instanciation and type checking.
Use this on single inheritance concrete classes only.
================
*/
#define CLASS_PROTOTYPE( nameofclass )									\
public:																	\
	static	idTypeInfo						Type;						\
	static	idClass							*CreateInstance();	\
	virtual	idTypeInfo						*GetType() const;		\
	static	idEventFunc<nameofclass>		eventCallbacks[]

/*
================
CLASS_DECLARATION

This macro must be included in the code to properly initialize variables
used in type checking and run-time instanciation.  It also defines the list
of events that the class responds to.  Take special care to ensure that the
proper superclass is indicated or the run-time type information will be
incorrect.  Use this on concrete classes only.
================
*/
// RB: made exceptions optional
#if defined(USE_EXCEPTIONS)
#define CLASS_DECLARATION( nameofsuperclass, nameofclass )											\
	idTypeInfo nameofclass::Type( #nameofclass, #nameofsuperclass,									\
		( idEventFunc<idClass> * )nameofclass::eventCallbacks,	nameofclass::CreateInstance, ( void ( idClass::* )() )&nameofclass::Spawn,	\
		( void ( idClass::* )( idSaveGame * ) const )&nameofclass::Save, ( void ( idClass::* )( idRestoreGame * ) )&nameofclass::Restore );	\
	idClass *nameofclass::CreateInstance() {														\
		try {																						\
			nameofclass *ptr = new nameofclass;														\
			ptr->FindUninitializedMemory();															\
			return ptr;																				\
		}																							\
		catch( idAllocError & ) {																	\
			return NULL;																			\
		}																							\
	}																								\
	idTypeInfo *nameofclass::GetType() const {														\
		return &( nameofclass::Type );																\
	}																								\
idEventFunc<nameofclass> nameofclass::eventCallbacks[] = {
#else
#define CLASS_DECLARATION( nameofsuperclass, nameofclass )											\
	idTypeInfo nameofclass::Type( #nameofclass, #nameofsuperclass,									\
		( idEventFunc<idClass> * )nameofclass::eventCallbacks,	nameofclass::CreateInstance, ( void ( idClass::* )() )&nameofclass::Spawn,	\
		( void ( idClass::* )( idSaveGame * ) const )&nameofclass::Save, ( void ( idClass::* )( idRestoreGame * ) )&nameofclass::Restore );	\
	idClass *nameofclass::CreateInstance() {													\
			nameofclass *ptr = new nameofclass;														\
			ptr->FindUninitializedMemory();															\
			return ptr;																				\
	}																								\
	idTypeInfo *nameofclass::GetType() const {												\
		return &( nameofclass::Type );																\
	}																								\
idEventFunc<nameofclass> nameofclass::eventCallbacks[] = {
#endif
// RB end

/*
================
ABSTRACT_PROTOTYPE

This macro must be included in the definition of any abstract subclass of idClass.
It prototypes variables used in class instanciation and type checking.
Use this on single inheritance abstract classes only.
================
*/
#define ABSTRACT_PROTOTYPE( nameofclass )								\
public:																	\
	static	idTypeInfo						Type;						\
	static	idClass							*CreateInstance();	\
	virtual	idTypeInfo						*GetType() const;		\
	static	idEventFunc<nameofclass>		eventCallbacks[]

/*
================
ABSTRACT_DECLARATION

This macro must be included in the code to properly initialize variables
used in type checking.  It also defines the list of events that the class
responds to.  Take special care to ensure that the proper superclass is
indicated or the run-time tyep information will be incorrect.  Use this
on abstract classes only.
================
*/
#define ABSTRACT_DECLARATION( nameofsuperclass, nameofclass )										\
	idTypeInfo nameofclass::Type( #nameofclass, #nameofsuperclass,									\
		( idEventFunc<idClass> * )nameofclass::eventCallbacks, nameofclass::CreateInstance, ( void ( idClass::* )() )&nameofclass::Spawn,	\
		( void ( idClass::* )( idSaveGame * ) const )&nameofclass::Save, ( void ( idClass::* )( idRestoreGame * ) )&nameofclass::Restore );	\
	idClass *nameofclass::CreateInstance() {													\
		gameLocal.Error( "Cannot instanciate abstract class %s.", #nameofclass );					\
		return NULL;																				\
	}																								\
	idTypeInfo *nameofclass::GetType() const {												\
		return &( nameofclass::Type );																\
	}																								\
	idEventFunc<nameofclass> nameofclass::eventCallbacks[] = {

typedef void ( idClass::*classSpawnFunc_t )();

class idSaveGame;
class idRestoreGame;

class idClass
{
public:
	ABSTRACT_PROTOTYPE( idClass );

	void* 						operator new( size_t );
	void						operator delete( void* );

	virtual						~idClass();

	void						Spawn();
	void						CallSpawn();
	bool						IsType( const idTypeInfo& c ) const;
	const char* 				GetClassname() const;
	const char* 				GetSuperclass() const;
	void						FindUninitializedMemory();

	virtual void				SharedThink() { }
	void						Save( idSaveGame* savefile ) const {};
	void						Restore( idRestoreGame* savefile ) {};

	bool						RespondsTo( const idEventDef& ev ) const;

	bool						PostEventMS( const idEventDef* ev, int time );
	bool						PostEventMS( const idEventDef* ev, int time, idEventArg arg1 );
	bool						PostEventMS( const idEventDef* ev, int time, idEventArg arg1, idEventArg arg2 );
	bool						PostEventMS( const idEventDef* ev, int time, idEventArg arg1, idEventArg arg2, idEventArg arg3 );
	bool						PostEventMS( const idEventDef* ev, int time, idEventArg arg1, idEventArg arg2, idEventArg arg3, idEventArg arg4 );
	bool						PostEventMS( const idEventDef* ev, int time, idEventArg arg1, idEventArg arg2, idEventArg arg3, idEventArg arg4, idEventArg arg5 );
	bool						PostEventMS( const idEventDef* ev, int time, idEventArg arg1, idEventArg arg2, idEventArg arg3, idEventArg arg4, idEventArg arg5, idEventArg arg6 );
	bool						PostEventMS( const idEventDef* ev, int time, idEventArg arg1, idEventArg arg2, idEventArg arg3, idEventArg arg4, idEventArg arg5, idEventArg arg6, idEventArg arg7 );
	bool						PostEventMS( const idEventDef* ev, int time, idEventArg arg1, idEventArg arg2, idEventArg arg3, idEventArg arg4, idEventArg arg5, idEventArg arg6, idEventArg arg7, idEventArg arg8 );

	bool						PostEventSec( const idEventDef* ev, float time );
	bool						PostEventSec( const idEventDef* ev, float time, idEventArg arg1 );
	bool						PostEventSec( const idEventDef* ev, float time, idEventArg arg1, idEventArg arg2 );
	bool						PostEventSec( const idEventDef* ev, float time, idEventArg arg1, idEventArg arg2, idEventArg arg3 );
	bool						PostEventSec( const idEventDef* ev, float time, idEventArg arg1, idEventArg arg2, idEventArg arg3, idEventArg arg4 );
	bool						PostEventSec( const idEventDef* ev, float time, idEventArg arg1, idEventArg arg2, idEventArg arg3, idEventArg arg4, idEventArg arg5 );
	bool						PostEventSec( const idEventDef* ev, float time, idEventArg arg1, idEventArg arg2, idEventArg arg3, idEventArg arg4, idEventArg arg5, idEventArg arg6 );
	bool						PostEventSec( const idEventDef* ev, float time, idEventArg arg1, idEventArg arg2, idEventArg arg3, idEventArg arg4, idEventArg arg5, idEventArg arg6, idEventArg arg7 );
	bool						PostEventSec( const idEventDef* ev, float time, idEventArg arg1, idEventArg arg2, idEventArg arg3, idEventArg arg4, idEventArg arg5, idEventArg arg6, idEventArg arg7, idEventArg arg8 );

	bool						ProcessEvent( const idEventDef* ev );
	bool						ProcessEvent( const idEventDef* ev, idEventArg arg1 );
	bool						ProcessEvent( const idEventDef* ev, idEventArg arg1, idEventArg arg2 );
	bool						ProcessEvent( const idEventDef* ev, idEventArg arg1, idEventArg arg2, idEventArg arg3 );
	bool						ProcessEvent( const idEventDef* ev, idEventArg arg1, idEventArg arg2, idEventArg arg3, idEventArg arg4 );
	bool						ProcessEvent( const idEventDef* ev, idEventArg arg1, idEventArg arg2, idEventArg arg3, idEventArg arg4, idEventArg arg5 );
	bool						ProcessEvent( const idEventDef* ev, idEventArg arg1, idEventArg arg2, idEventArg arg3, idEventArg arg4, idEventArg arg5, idEventArg arg6 );
	bool						ProcessEvent( const idEventDef* ev, idEventArg arg1, idEventArg arg2, idEventArg arg3, idEventArg arg4, idEventArg arg5, idEventArg arg6, idEventArg arg7 );
	bool						ProcessEvent( const idEventDef* ev, idEventArg arg1, idEventArg arg2, idEventArg arg3, idEventArg arg4, idEventArg arg5, idEventArg arg6, idEventArg arg7, idEventArg arg8 );

	// RB: 64 bit fix, changed int to intptr_t
	bool						ProcessEventArgPtr( const idEventDef* ev, intptr_t* data );
	// RB end
	void						CancelEvents( const idEventDef* ev );

	void						Event_Remove();

	// Static functions
	static void					Init();
	static void					Shutdown();
	static idTypeInfo* 			GetClass( const char* name );
	static void					DisplayInfo_f( const idCmdArgs& args );
	static void					ListClasses_f( const idCmdArgs& args );
	// RB begin
	static void					ExportScriptEvents_f( const idCmdArgs& args );
	// RB end
	static idClass* 			CreateInstance( const char* name );
	static int					GetNumTypes()
	{
		return types.Num();
	}
	static int					GetTypeNumBits()
	{
		return typeNumBits;
	}
	static idTypeInfo* 			GetType( int num );

private:
	classSpawnFunc_t			CallSpawnFunc( idTypeInfo* cls );

	bool						PostEventArgs( const idEventDef* ev, int time, int numargs, ... );
	bool						ProcessEventArgs( const idEventDef* ev, int numargs, ... );

	void						Event_SafeRemove();

	static bool					initialized;
	static idList<idTypeInfo*, TAG_IDCLASS>	types;
	static idList<idTypeInfo*, TAG_IDCLASS>	typenums;
	static int					typeNumBits;
	static int					memused;
	static int					numobjects;
};

/***********************************************************************

  idTypeInfo

***********************************************************************/

class idTypeInfo
{
public:
	const char* 				classname;
	const char* 				superclass;
	idClass* 					( *CreateInstance )();
	void	( idClass::*Spawn )();
	void	( idClass::*Save )( idSaveGame* savefile ) const;
	void	( idClass::*Restore )( idRestoreGame* savefile );

	idEventFunc<idClass>* 		eventCallbacks;
	eventCallback_t* 			eventMap;
	idTypeInfo* 				super;
	idTypeInfo* 				next;
	bool						freeEventMap;
	int							typeNum;
	int							lastChild;

	idHierarchy<idTypeInfo>		node;

	idTypeInfo( const char* classname, const char* superclass,
				idEventFunc<idClass>* eventCallbacks, idClass * ( *CreateInstance )(), void ( idClass::*Spawn )(),
				void ( idClass::*Save )( idSaveGame* savefile ) const, void	( idClass::*Restore )( idRestoreGame* savefile ) );
	~idTypeInfo();

	void						Init();
	void						Shutdown();

	bool						IsType( const idTypeInfo& superclass ) const;
	bool						RespondsTo( const idEventDef& ev ) const;
};

/*
================
idTypeInfo::IsType

Checks if the object's class is a subclass of the class defined by the
passed in idTypeInfo.
================
*/
ID_INLINE bool idTypeInfo::IsType( const idTypeInfo& type ) const
{
	return ( ( typeNum >= type.typeNum ) && ( typeNum <= type.lastChild ) );
}

/*
================
idTypeInfo::RespondsTo
================
*/
ID_INLINE bool idTypeInfo::RespondsTo( const idEventDef& ev ) const
{
	assert( idEvent::initialized );
	if( !eventMap[ ev.GetEventNum() ] )
	{
		// we don't respond to this event
		return false;
	}

	return true;
}

/*
================
idClass::IsType

Checks if the object's class is a subclass of the class defined by the
passed in idTypeInfo.
================
*/
ID_INLINE bool idClass::IsType( const idTypeInfo& superclass ) const
{
	idTypeInfo* subclass;

	subclass = GetType();
	return subclass->IsType( superclass );
}

/*
================
idClass::RespondsTo
================
*/
ID_INLINE bool idClass::RespondsTo( const idEventDef& ev ) const
{
	const idTypeInfo* c;

	assert( idEvent::initialized );
	c = GetType();
	return c->RespondsTo( ev );
}

#endif /* !__SYS_CLASS_H__ */
