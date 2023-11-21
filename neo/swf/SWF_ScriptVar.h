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
#ifndef __SWF_SCRIPTVAR_H__
#define __SWF_SCRIPTVAR_H__

class idSWFScriptObject;
class idSWFScriptFunction;

/*
========================
A reference counted string
========================
*/
class idSWFScriptString : public idStr
{
public:
	idSWFScriptString( const idStr& s ) : idStr( s ), refCount( 1 ) { }

	static idSWFScriptString* Alloc( const idStr& s )
	{
		return new( TAG_SWF ) idSWFScriptString( s );
	}
	ID_INLINE void	AddRef()
	{
		refCount++;
	}
	ID_INLINE void	Release()
	{
		if( --refCount == 0 )
		{
			delete this;
		}
	}

private:
	std::atomic<int> refCount;
};

/*
========================
A variable in an action script
these can be on the stack, in a script object, passed around as parameters, etc
they can contain raw data (int, float), strings, functions, or objects
========================
*/
class idSWFScriptVar
{
public:
	idSWFScriptVar() : type( SWF_VAR_UNDEF ) { }
	idSWFScriptVar( const idSWFScriptVar& other );
	idSWFScriptVar( idSWFScriptObject* o ) : type( SWF_VAR_UNDEF )
	{
		SetObject( o );
	}
	idSWFScriptVar( idStrId s ) : type( SWF_VAR_UNDEF )
	{
		SetString( s );
	}
	idSWFScriptVar( const idStr& s ) : type( SWF_VAR_UNDEF )
	{
		SetString( s );
	}
	idSWFScriptVar( const char* s ) : type( SWF_VAR_UNDEF )
	{
		SetString( idStr( s ) );
	}
	idSWFScriptVar( float f ) : type( SWF_VAR_UNDEF )
	{
		SetFloat( f );
	}
	idSWFScriptVar( bool b ) : type( SWF_VAR_UNDEF )
	{
		SetBool( b );
	}
	idSWFScriptVar( int32 i ) : type( SWF_VAR_UNDEF )
	{
		SetInteger( i );
	}
	idSWFScriptVar( idSWFScriptFunction* nf ) : type( SWF_VAR_UNDEF )
	{
		SetFunction( nf );
	}
	~idSWFScriptVar();

	idSWFScriptVar& operator=( const idSWFScriptVar& other );

	// implements ECMA 262 11.9.3
	bool AbstractEquals( const idSWFScriptVar& other );
	bool StrictEquals( const idSWFScriptVar& other );

	void SetString( idStrId s )
	{
		Free();
		type = SWF_VAR_STRINGID;
		value.i = s.GetIndex();
	}
	void SetString( const idStr& s )
	{
		Free();
		type = SWF_VAR_STRING;
		value.string = idSWFScriptString::Alloc( s );
	}
	void SetString( const char* s )
	{
		Free();
		type = SWF_VAR_STRING;
		value.string = idSWFScriptString::Alloc( s );
	}
	void SetString( idSWFScriptString* s )
	{
		Free();
		type = SWF_VAR_STRING;
		value.string = s;
		s->AddRef();
	}

	// RB begin
	void SetResult( const idStr& s )
	{
		Free();
		type = SWF_VAR_RESULT;
		value.string = idSWFScriptString::Alloc( s );
	}
	void SetResult( const char* s )
	{
		Free();
		type = SWF_VAR_RESULT;
		value.string = idSWFScriptString::Alloc( s );
	}
	// RB end

	void SetFloat( float f )
	{
		Free();
		type = SWF_VAR_FLOAT;
		value.f = f;
	}
	void SetNULL()
	{
		Free();
		type = SWF_VAR_NULL;
	}
	void SetUndefined()
	{
		Free();
		type = SWF_VAR_UNDEF;
	}
	void SetBool( bool b )
	{
		Free();
		type = SWF_VAR_BOOL;
		value.b = b;
	}
	void SetInteger( int32 i )
	{
		Free();
		type = SWF_VAR_INTEGER;
		value.i = i;
	}

	void SetObject( idSWFScriptObject* o );
	void SetFunction( idSWFScriptFunction* f );

	idStr	ToString() const;
	float	ToFloat() const;
	bool	ToBool() const;
	int32	ToInteger() const;

	idSWFScriptObject* 		GetObject()
	{
		assert( type == SWF_VAR_OBJECT );
		return value.object;
	}
	idSWFScriptObject* 		GetObject() const
	{
		assert( type == SWF_VAR_OBJECT );
		return value.object;
	}
	idSWFScriptFunction* 	GetFunction()
	{
		assert( type == SWF_VAR_FUNCTION );
		return value.function;
	}
	idSWFSpriteInstance* 	ToSprite();
	idSWFTextInstance* 		ToText();

	idSWFScriptVar			GetNestedVar( const char* arg1, const char* arg2 = NULL, const char* arg3 = NULL, const char* arg4 = NULL, const char* arg5 = NULL, const char* arg6 = NULL );
	idSWFScriptObject* 		GetNestedObj( const char* arg1, const char* arg2 = NULL, const char* arg3 = NULL, const char* arg4 = NULL, const char* arg5 = NULL, const char* arg6 = NULL );
	idSWFSpriteInstance* 	GetNestedSprite( const char* arg1, const char* arg2 = NULL, const char* arg3 = NULL, const char* arg4 = NULL, const char* arg5 = NULL, const char* arg6 = NULL );
	idSWFTextInstance* 		GetNestedText( const char* arg1, const char* arg2 = NULL, const char* arg3 = NULL, const char* arg4 = NULL, const char* arg5 = NULL, const char* arg6 = NULL );

	const char* 			TypeOf() const;

	// debug print of this variable to the console
	void					PrintToConsole() const;

	bool IsString()		const
	{
		return ( type == SWF_VAR_STRING ) || ( type == SWF_VAR_STRINGID );
	}
	bool IsNULL()		const
	{
		return ( type == SWF_VAR_NULL );
	}
	bool IsUndefined()	const
	{
		return ( type == SWF_VAR_UNDEF );
	}
	bool IsValid()		const
	{
		return ( type != SWF_VAR_UNDEF ) && ( type != SWF_VAR_NULL );
	}
	bool IsFunction()	const
	{
		return ( type == SWF_VAR_FUNCTION );
	}
	bool IsObject()		const
	{
		return ( type == SWF_VAR_OBJECT );
	}
	bool IsNumeric()	const
	{
		return ( type == SWF_VAR_FLOAT ) || ( type == SWF_VAR_INTEGER ) || ( type == SWF_VAR_BOOL );
	}

	bool IsResult()		const
	{
		return ( type == SWF_VAR_RESULT );
	}

	enum swfScriptVarType
	{
		SWF_VAR_STRINGID,
		SWF_VAR_STRING,
		SWF_VAR_FLOAT,
		SWF_VAR_NULL,
		SWF_VAR_UNDEF,
		SWF_VAR_BOOL,
		SWF_VAR_INTEGER,
		SWF_VAR_FUNCTION,
		SWF_VAR_OBJECT,
		SWF_VAR_RESULT	// RB: for P-Code to Lua
	};

	swfScriptVarType	GetType() const
	{
		return type;
	}

private:
	void Free();
	swfScriptVarType type;

	union swfScriptVarValue_t
	{
		float	f;
		int32	i;
		bool	b;
		idSWFScriptObject* object;
		idSWFScriptString* string;
		idSWFScriptFunction* function;
	} value;
};

#endif // !__SWF_SCRIPTVAR_H__
