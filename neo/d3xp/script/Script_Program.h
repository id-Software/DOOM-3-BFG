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

#ifndef __SCRIPT_PROGRAM_H__
#define __SCRIPT_PROGRAM_H__

class idScriptObject;
class idEventDef;
class idVarDef;
class idTypeDef;
class idEntity;
class idThread;
class idSaveGame;
class idRestoreGame;

#define MAX_STRING_LEN		128
#define MAX_GLOBALS			296608			// in bytes
#define MAX_STRINGS			1024

#define MAX_FUNCS			3584

#define MAX_STATEMENTS		131072			// statement_t - 18 bytes last I checked

typedef enum
{
	ev_error = -1, ev_void, ev_scriptevent, ev_namespace, ev_string, ev_float, ev_vector, ev_entity, ev_field, ev_function, ev_virtualfunction, ev_pointer, ev_object, ev_jumpoffset, ev_argsize, ev_boolean
} etype_t;

class function_t
{
public:
	function_t();

	size_t				Allocated() const;
	void				SetName( const char* name );
	const char*			Name() const;
	void				Clear();

private:
	idStr 				name;
public:
	const idEventDef*	eventdef;
	idVarDef*			def;
	const idTypeDef*		type;
	int 				firstStatement;
	int 				numStatements;
	int 				parmTotal;
	int 				locals; 			// total ints of parms + locals
	int					filenum; 			// source file defined in
	idList<int, TAG_SCRIPT>			parmSize;
};

typedef union eval_s
{
	const char*			stringPtr;
	float				_float;
	float				vector[ 3 ];
	function_t*			function;
	int 				_int;
	int 				entity;
} eval_t;

/***********************************************************************

idTypeDef

Contains type information for variables and functions.

***********************************************************************/

class idTypeDef
{
private:
	etype_t						type;
	idStr 						name;
	int							size;

	// function types are more complex
	idTypeDef*					auxType;					// return type
	idList<idTypeDef*, TAG_SCRIPT>			parmTypes;
	idStrList					parmNames;
	idList<const function_t*, TAG_SCRIPT>	functions;

public:
	idVarDef*					def;						// a def that points to this type

	idTypeDef( const idTypeDef& other );
	idTypeDef( etype_t etype, idVarDef* edef, const char* ename, int esize, idTypeDef* aux );
	void				operator=( const idTypeDef& other );
	size_t				Allocated() const;

	bool				Inherits( const idTypeDef* basetype ) const;
	bool				MatchesType( const idTypeDef& matchtype ) const;
	bool				MatchesVirtualFunction( const idTypeDef& matchfunc ) const;
	void				AddFunctionParm( idTypeDef* parmtype, const char* name );
	void				AddField( idTypeDef* fieldtype, const char* name );

	void				SetName( const char* newname );
	const char*			Name() const;

	etype_t				Type() const;
	int					Size() const;

	idTypeDef*			SuperClass() const;

	idTypeDef*			ReturnType() const;
	void				SetReturnType( idTypeDef* type );

	idTypeDef*			FieldType() const;
	void				SetFieldType( idTypeDef* type );

	idTypeDef*			PointerType() const;
	void				SetPointerType( idTypeDef* type );

	int					NumParameters() const;
	idTypeDef*			GetParmType( int parmNumber ) const;
	const char*			GetParmName( int parmNumber ) const;

	int					NumFunctions() const;
	int					GetFunctionNumber( const function_t* func ) const;
	const function_t*	GetFunction( int funcNumber ) const;
	void				AddFunction( const function_t* func );
};

/***********************************************************************

idScriptObject

In-game representation of objects in scripts.  Use the idScriptVariable template
(below) to access variables.

***********************************************************************/

class idScriptObject
{
private:
	idTypeDef*					type;

public:
	byte*						data;

	idScriptObject();
	~idScriptObject();

	void						Save( idSaveGame* savefile ) const;			// archives object for save game file
	void						Restore( idRestoreGame* savefile );			// unarchives object from save game file

	void						Free();
	bool						SetType( const char* typeName );
	void						ClearObject();
	bool						HasObject() const;
	idTypeDef*					GetTypeDef() const;
	const char*					GetTypeName() const;
	const function_t*			GetConstructor() const;
	const function_t*			GetDestructor() const;
	const function_t*			GetFunction( const char* name ) const;

	byte*						GetVariable( const char* name, etype_t etype ) const;
};

/***********************************************************************

idScriptVariable

Helper template that handles looking up script variables stored in objects.
If the specified variable doesn't exist, or is the wrong data type, idScriptVariable
will cause an error.

***********************************************************************/

template<class type, etype_t etype, class returnType>
class idScriptVariable
{
private:
	type*				data;

public:
	idScriptVariable();
	bool				IsLinked() const;
	void				Unlink();
	void				LinkTo( idScriptObject& obj, const char* name );
	idScriptVariable&	operator=( const returnType& value );
	operator returnType() const;
};

template<class type, etype_t etype, class returnType>
ID_INLINE idScriptVariable<type, etype, returnType>::idScriptVariable()
{
	data = NULL;
}

template<class type, etype_t etype, class returnType>
ID_INLINE bool idScriptVariable<type, etype, returnType>::IsLinked() const
{
	return ( data != NULL );
}

template<class type, etype_t etype, class returnType>
ID_INLINE void idScriptVariable<type, etype, returnType>::Unlink()
{
	data = NULL;
}

template<class type, etype_t etype, class returnType>
ID_INLINE void idScriptVariable<type, etype, returnType>::LinkTo( idScriptObject& obj, const char* name )
{
	data = ( type* )obj.GetVariable( name, etype );
	if( !data )
	{
		gameError( "Missing '%s' field in script object '%s'", name, obj.GetTypeName() );
	}
}

template<class type, etype_t etype, class returnType>
ID_INLINE idScriptVariable<type, etype, returnType>& idScriptVariable<type, etype, returnType>::operator=( const returnType& value )
{
	// check if we attempt to access the object before it's been linked
	assert( data );

	// make sure we don't crash if we don't have a pointer
	if( data )
	{
		*data = ( type )value;
	}
	return *this;
}

template<class type, etype_t etype, class returnType>
ID_INLINE idScriptVariable<type, etype, returnType>::operator returnType() const
{
	// check if we attempt to access the object before it's been linked
	assert( data );

	// make sure we don't crash if we don't have a pointer
	if( data )
	{
		return ( const returnType ) * data;
	}
	else
	{
		// reasonably safe value
		return ( const returnType )0;
	}
}

/***********************************************************************

Script object variable access template instantiations

These objects will automatically handle looking up of the current value
of a variable in a script object.  They can be stored as part of a class
for up-to-date values of the variable, or can be used in functions to
sample the data for non-dynamic values.

***********************************************************************/

typedef idScriptVariable<int, ev_boolean, int>				idScriptBool;
typedef idScriptVariable<float, ev_float, float>			idScriptFloat;
typedef idScriptVariable<float, ev_float, int>				idScriptInt;
typedef idScriptVariable<idVec3, ev_vector, idVec3>			idScriptVector;
typedef idScriptVariable<idStr, ev_string, const char*>	idScriptString;

/***********************************************************************

idCompileError

Causes the compiler to exit out of compiling the current function and
display an error message with line and file info.

***********************************************************************/

class idCompileError : public idException
{
public:
	idCompileError( const char* text ) : idException( text ) {}
};

/***********************************************************************

idVarDef

Define the name, type, and location of variables, functions, and objects
defined in script.

***********************************************************************/

typedef union varEval_s
{
	idScriptObject**			objectPtrPtr;
	char*					stringPtr;
	float*					floatPtr;
	idVec3*					vectorPtr;
	function_t*				functionPtr;
	int*					 intPtr;
	byte*					bytePtr;
	int*					 entityNumberPtr;
	int						virtualFunction;
	int						jumpOffset;
	int						stackOffset;		// offset in stack for local variables
	int						argSize;
	varEval_s*				evalPtr;
	int						ptrOffset;
} varEval_t;

class idVarDefName;

class idVarDef
{
	friend class idVarDefName;

public:
	int						num;
	varEval_t				value;
	idVarDef* 				scope; 			// function, namespace, or object the var was defined in
	int						numUsers;		// number of users if this is a constant

	typedef enum
	{
		uninitialized, initializedVariable, initializedConstant, stackVariable
	} initialized_t;

	initialized_t			initialized;

public:
	idVarDef( idTypeDef* typeptr = NULL );
	~idVarDef();

	const char* 			Name() const;
	const char* 			GlobalName() const;

	void					SetTypeDef( idTypeDef* _type )
	{
		typeDef = _type;
	}
	idTypeDef* 				TypeDef() const
	{
		return typeDef;
	}
	etype_t					Type() const
	{
		return ( typeDef != NULL ) ? typeDef->Type() : ev_void;
	}

	int						DepthOfScope( const idVarDef* otherScope ) const;

	void					SetFunction( function_t* func );
	void					SetObject( idScriptObject* object );
	void					SetValue( const eval_t& value, bool constant );
	void					SetString( const char* string, bool constant );

	idVarDef* 				Next() const
	{
		return next;    // next var def with same name
	}

	void					PrintInfo( idFile* file, int instructionPointer ) const;

private:
	idTypeDef* 				typeDef;
	idVarDefName* 			name;		// name of this var
	idVarDef* 				next;		// next var with the same name
};

/***********************************************************************

  idVarDefName

***********************************************************************/

class idVarDefName
{
public:
	idVarDefName()
	{
		defs = NULL;
	}
	idVarDefName( const char* n )
	{
		name = n;
		defs = NULL;
	}

	const char* 			Name() const
	{
		return name;
	}
	idVarDef* 				GetDefs() const
	{
		return defs;
	}

	void					AddDef( idVarDef* def );
	void					RemoveDef( idVarDef* def );

private:
	idStr					name;
	idVarDef* 				defs;
};

/***********************************************************************

  Variable and type defintions

***********************************************************************/

extern	idTypeDef	type_void;
extern	idTypeDef	type_scriptevent;
extern	idTypeDef	type_namespace;
extern	idTypeDef	type_string;
extern	idTypeDef	type_float;
extern	idTypeDef	type_vector;
extern	idTypeDef	type_entity;
extern  idTypeDef	type_field;
extern	idTypeDef	type_function;
extern	idTypeDef	type_virtualfunction;
extern  idTypeDef	type_pointer;
extern	idTypeDef	type_object;
extern	idTypeDef	type_jumpoffset;	// only used for jump opcodes
extern	idTypeDef	type_argsize;		// only used for function call and thread opcodes
extern	idTypeDef	type_boolean;

extern	idVarDef	def_void;
extern	idVarDef	def_scriptevent;
extern	idVarDef	def_namespace;
extern	idVarDef	def_string;
extern	idVarDef	def_float;
extern	idVarDef	def_vector;
extern	idVarDef	def_entity;
extern	idVarDef	def_field;
extern	idVarDef	def_function;
extern	idVarDef	def_virtualfunction;
extern	idVarDef	def_pointer;
extern	idVarDef	def_object;
extern	idVarDef	def_jumpoffset;		// only used for jump opcodes
extern	idVarDef	def_argsize;		// only used for function call and thread opcodes
extern	idVarDef	def_boolean;

typedef struct statement_s
{
	unsigned short	op;
	idVarDef*		a;
	idVarDef*		b;
	idVarDef*		c;
	unsigned short	linenumber;
	unsigned short	file;
} statement_t;

/***********************************************************************

idProgram

Handles compiling and storage of script data.  Multiple idProgram objects
would represent separate programs with no knowledge of each other.  Scripts
meant to access shared data and functions should all be compiled by a
single idProgram.

***********************************************************************/

class idProgram
{
private:
	idStrList									fileList;
	idStr 										filename;
	int											filenum;

	int											numVariables;
	byte										variables[ MAX_GLOBALS ];
	idStaticList<byte, MAX_GLOBALS>				variableDefaults;
	idStaticList<function_t, MAX_FUNCS>			functions;
	idStaticList<statement_t, MAX_STATEMENTS>	statements;
	idList<idTypeDef*, TAG_SCRIPT>				types;
	idHashIndex									typesHash;
	idList<idVarDefName*, TAG_SCRIPT>			varDefNames;
	idHashIndex									varDefNameHash;
	idList<idVarDef*, TAG_SCRIPT>				varDefs;

	idVarDef*									sysDef;

	int											top_functions;
	int											top_statements;
	int											top_types;
	int											top_defs;
	int											top_files;

	void										CompileStats();

public:
	idVarDef*									returnDef;
	idVarDef*									returnStringDef;

	idProgram();
	~idProgram();

	// save games
	void										Save( idSaveGame* savefile ) const;
	bool										Restore( idRestoreGame* savefile );
	int											CalculateChecksum() const;		// Used to insure program code has not
	//    changed between savegames

	void										Startup( const char* defaultScript );
	void										Restart();
	bool										CompileText( const char* source, const char* text, bool console );
	const function_t*							CompileFunction( const char* functionName, const char* text );
	void										CompileFile( const char* filename );
	void										BeginCompilation();
	void										FinishCompilation();
	void										DisassembleStatement( idFile* file, int instructionPointer ) const;
	void										Disassemble() const;
	void										FreeData();

	const char*									GetFilename( int num );
	int											GetFilenum( const char* name );
	int											GetLineNumberForStatement( int index );
	const char*									GetFilenameForStatement( int index );

	idTypeDef*									AllocType( idTypeDef& type );
	idTypeDef*									AllocType( etype_t etype, idVarDef* edef, const char* ename, int esize, idTypeDef* aux );
	idTypeDef*									GetType( idTypeDef& type, bool allocate );
	idTypeDef*									FindType( const char* name );

	// RB begin
private:
	byte*										ReserveDefMemory( int size );
	idVarDef*									AllocVarDef( idTypeDef* type, const char* name, idVarDef* scope );
public:
	// RB end

	idVarDef*									AllocDef( idTypeDef* type, const char* name, idVarDef* scope, bool constant );
	idVarDef*									GetDef( const idTypeDef* type, const char* name, const idVarDef* scope ) const;
	void										FreeDef( idVarDef* d, const idVarDef* scope );
	idVarDef*									FindFreeResultDef( idTypeDef* type, const char* name, idVarDef* scope, const idVarDef* a, const idVarDef* b );
	idVarDef*									GetDefList( const char* name ) const;
	void										AddDefToNameList( idVarDef* def, const char* name );

	function_t*									FindFunction( const char* name ) const;						// returns NULL if function not found
	function_t*									FindFunction( const char* name, const idTypeDef* type ) const;	// returns NULL if function not found
	function_t&									AllocFunction( idVarDef* def );
	function_t*									GetFunction( int index );
	int											GetFunctionIndex( const function_t* func );

	void										SetEntity( const char* name, idEntity* ent );

	statement_t*									AllocStatement();
	statement_t&									GetStatement( int index );
	int											NumStatements()
	{
		return statements.Num();
	}

	int 										GetReturnedInteger();

	void										ReturnFloat( float value );
	void										ReturnInteger( int value );
	void										ReturnVector( idVec3 const& vec );
	void										ReturnString( const char* string );
	void										ReturnEntity( idEntity* ent );

	int											NumFilenames()
	{
		return fileList.Num();
	}
};

/*
================
idProgram::GetStatement
================
*/
ID_INLINE statement_t& idProgram::GetStatement( int index )
{
	return statements[ index ];
}

/*
================
idProgram::GetFunction
================
*/
ID_INLINE function_t* idProgram::GetFunction( int index )
{
	return &functions[ index ];
}

/*
================
idProgram::GetFunctionIndex
================
*/
ID_INLINE int idProgram::GetFunctionIndex( const function_t* func )
{
	return func - &functions[0];
}

/*
================
idProgram::GetReturnedInteger
================
*/
ID_INLINE int idProgram::GetReturnedInteger()
{
	return *returnDef->value.intPtr;
}

/*
================
idProgram::ReturnFloat
================
*/
ID_INLINE void idProgram::ReturnFloat( float value )
{
	*returnDef->value.floatPtr = value;
}

/*
================
idProgram::ReturnInteger
================
*/
ID_INLINE void idProgram::ReturnInteger( int value )
{
	*returnDef->value.intPtr = value;
}

/*
================
idProgram::ReturnVector
================
*/
ID_INLINE void idProgram::ReturnVector( idVec3 const& vec )
{
	*returnDef->value.vectorPtr = vec;
}

/*
================
idProgram::ReturnString
================
*/
ID_INLINE void idProgram::ReturnString( const char* string )
{
	idStr::Copynz( returnStringDef->value.stringPtr, string, MAX_STRING_LEN );
}

/*
================
idProgram::GetFilename
================
*/
ID_INLINE const char* idProgram::GetFilename( int num )
{
	return fileList[ num ];
}

/*
================
idProgram::GetLineNumberForStatement
================
*/
ID_INLINE int idProgram::GetLineNumberForStatement( int index )
{
	return statements[ index ].linenumber;
}

/*
================
idProgram::GetFilenameForStatement
================
*/
ID_INLINE const char* idProgram::GetFilenameForStatement( int index )
{
	return GetFilename( statements[ index ].file );
}

#endif /* !__SCRIPT_PROGRAM_H__ */
