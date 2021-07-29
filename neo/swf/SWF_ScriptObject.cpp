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

idCVar swf_debugShowAddress( "swf_debugShowAddress", "0", CVAR_BOOL, "shows addresses along with object types when they are serialized" );


/*
========================
idSWFScriptObject::swfNamedVar_t::~swfNamedVar_t
========================
*/
idSWFScriptObject::swfNamedVar_t::~swfNamedVar_t()
{
}

/*
========================
idSWFScriptObject::swfNamedVar_t::operator=
========================
*/
idSWFScriptObject::swfNamedVar_t& idSWFScriptObject::swfNamedVar_t::operator=( const swfNamedVar_t& other )
{
	if( &other != this )
	{
		index = other.index;
		name = other.name;
		hashNext = other.hashNext;
		value = other.value;
		native = other.native;
		flags = other.flags;
	}
	return *this;
}

/*
========================
idSWFScriptObject::idSWFScriptObject
========================
*/
idSWFScriptObject::idSWFScriptObject() : refCount( 1 ), noAutoDelete( false ), prototype( NULL ), objectType( SWF_OBJECT_OBJECT )
{
	data.sprite = NULL;
	data.text = NULL;
	Clear();
	refCount = 1;
}

/*
========================
idSWFScriptObject::~idSWFScriptObject
========================
*/
idSWFScriptObject::~idSWFScriptObject()
{
	if( prototype != NULL )
	{
		prototype->Release();
	}
}

/*
========================
idSWFScriptObject::Alloc
========================
*/
idSWFScriptObject* 	idSWFScriptObject::Alloc()
{
	return new( TAG_SWF ) idSWFScriptObject;
}

/*
========================
idSWFScriptObject::AddRef
========================
*/
void idSWFScriptObject::AddRef()
{
	refCount++;
}

/*
========================
idSWFScriptObject::Release
========================
*/
void idSWFScriptObject::Release()
{
	if( --refCount == 0 && !noAutoDelete )
	{
		delete this;
	}
}

/*
========================
idSWFScriptObject::Clear
========================
*/
void idSWFScriptObject::Clear()
{
	variables.Clear();
	for( int i = 0; i < VARIABLE_HASH_BUCKETS; i++ )
	{
		variablesHash[i] = -1;
	}
}

/*
========================
idSWFScriptObject::HasProperty
========================
*/
bool idSWFScriptObject::HasProperty( const char* name )
{
	return ( GetVariable( name, false ) != NULL );
}

/*
========================
idSWFScriptObject::HasValidProperty
========================
*/
bool idSWFScriptObject::HasValidProperty( const char* name )
{
	idSWFScriptObject::swfNamedVar_t* const variable = GetVariable( name, false );
	if( variable == NULL )
	{
		return false;
	}
	if( variable->native != NULL )
	{
		idSWFScriptVar nv = variable->native->Get( this );
		if( nv.IsNULL() || nv.IsUndefined() )
		{
			return false;
		}
	}
	else
	{
		if( variable->value.IsNULL() || variable->value.IsUndefined() )
		{
			return false;
		}
	}
	return true;
}

/*
========================
idSWFScriptObject::Get
========================
*/
idSWFScriptVar idSWFScriptObject::Get( const char* name )
{
	swfNamedVar_t* variable = GetVariable( name, false );
	if( variable == NULL )
	{
		return idSWFScriptVar();
	}
	else
	{
		if( variable->native )
		{
			return variable->native->Get( this );
		}
		else
		{
			return variable->value;
		}
	}
}

/*
========================
idSWFScriptObject::Get
========================
*/
idSWFScriptVar idSWFScriptObject::Get( int index )
{
	swfNamedVar_t* variable = GetVariable( index, false );
	if( variable == NULL )
	{
		return idSWFScriptVar();
	}
	else
	{
		if( variable->native )
		{
			return variable->native->Get( this );
		}
		else
		{
			return variable->value;
		}
	}
}

/*
========================
idSWFScriptObject::GetSprite
========================
*/
idSWFSpriteInstance* idSWFScriptObject::GetSprite( int index )
{
	idSWFScriptVar var = Get( index );
	return var.ToSprite();
}

/*
========================
idSWFScriptObject::GetSprite
========================
*/
idSWFSpriteInstance* idSWFScriptObject::GetSprite( const char* name )
{
	idSWFScriptVar var = Get( name );
	return var.ToSprite();
}

/*
========================
idSWFScriptObject::GetObject
========================
*/
idSWFScriptObject* idSWFScriptObject::GetObject( int index )
{
	idSWFScriptVar var = Get( index );
	if( var.IsObject() )
	{
		return var.GetObject();
	}
	return NULL;
}

/*
========================
idSWFScriptObject::GetObject
========================
*/
idSWFScriptObject* idSWFScriptObject::GetObject( const char* name )
{
	idSWFScriptVar var = Get( name );
	if( var.IsObject() )
	{
		return var.GetObject();
	}
	return NULL;
}

/*
========================
idSWFScriptObject::GetText
========================
*/
idSWFTextInstance* idSWFScriptObject::GetText( int index )
{
	idSWFScriptVar var = Get( index );
	if( var.IsObject() )
	{
		return var.GetObject()->GetText();
	}
	return NULL;
}

/*
========================
idSWFScriptObject::GetText
========================
*/
idSWFTextInstance* idSWFScriptObject::GetText( const char* name )
{
	idSWFScriptVar var = Get( name );
	if( var.IsObject() )
	{
		return var.GetObject()->GetText();
	}
	return NULL;
}

/*
========================
idSWFScriptObject::Set
========================
*/
void idSWFScriptObject::Set( const char* name, const idSWFScriptVar& value )
{
	if( objectType == SWF_OBJECT_ARRAY )
	{
		if( idStr::Cmp( name, "length" ) == 0 )
		{
			int newLength = value.ToInteger();
			for( int i = 0; i < variables.Num(); i++ )
			{
				if( variables[i].index >= newLength )
				{
					variables.RemoveIndexFast( i );
					i--;
				}
			}
			// rebuild the hash table
			for( int i = 0; i < VARIABLE_HASH_BUCKETS; i++ )
			{
				variablesHash[i] = -1;
			}
			for( int i = 0; i < variables.Num(); i++ )
			{
				int hash = idStr::Hash( variables[i].name.c_str() ) & ( VARIABLE_HASH_BUCKETS - 1 );
				variables[i].hashNext = variablesHash[hash];
				variablesHash[hash] = i;
			}
		}
		else
		{
			int iName = atoi( name );
			if( iName > 0 || ( iName == 0 && idStr::Cmp( name, "0" ) == 0 ) )
			{
				swfNamedVar_t* lengthVar = GetVariable( "length", true );
				if( lengthVar->value.ToInteger() <= iName )
				{
					lengthVar->value = idSWFScriptVar( iName + 1 );
				}
			}
		}
	}

	swfNamedVar_t* variable = GetVariable( name, true );
	if( variable->native )
	{
		variable->native->Set( this, value );
	}
	else if( ( variable->flags & SWF_VAR_FLAG_READONLY ) == 0 )
	{
		variable->value = value;
	}
}

/*
========================
idSWFScriptObject::Set
========================
*/
void idSWFScriptObject::Set( int index, const idSWFScriptVar& value )
{
	if( index < 0 )
	{
		extern idCVar swf_debug;
		if( swf_debug.GetBool() )
		{
			idLib::Printf( "SWF: Trying to set a negative array index.\n" );
		}
		return;
	}
	if( objectType == SWF_OBJECT_ARRAY )
	{
		swfNamedVar_t* lengthVar = GetVariable( "length", true );
		if( lengthVar->value.ToInteger() <= index )
		{
			lengthVar->value = idSWFScriptVar( index + 1 );
		}
	}

	swfNamedVar_t* variable = GetVariable( index, true );
	if( variable->native )
	{
		variable->native->Set( this, value );
	}
	else if( ( variable->flags & SWF_VAR_FLAG_READONLY ) == 0 )
	{
		variable->value = value;
	}
}

/*
========================
idSWFScriptObject::SetNative
========================
*/
void idSWFScriptObject::SetNative( const char* name, idSWFScriptNativeVariable* native )
{
	swfNamedVar_t* variable = GetVariable( name, true );
	variable->flags = SWF_VAR_FLAG_DONTENUM;
	variable->native = native;
	if( native->IsReadOnly() )
	{
		variable->flags |= SWF_VAR_FLAG_READONLY;
	}
}

/*
========================
idSWFScriptObject::DefaultValue
========================
*/
idSWFScriptVar idSWFScriptObject::DefaultValue( bool stringHint )
{
	const char* methods[2] = { "toString", "valueOf" };
	if( !stringHint )
	{
		SwapValues( methods[0], methods[1] );
	}
	for( int i = 0; i < 2; i++ )
	{
		idSWFScriptVar method = Get( methods[i] );
		if( method.IsFunction() )
		{
			idSWFScriptVar value = method.GetFunction()->Call( this, idSWFParmList() );
			if( !value.IsObject() && !value.IsFunction() )
			{
				return value;
			}
		}
	}
	switch( objectType )
	{
		case SWF_OBJECT_OBJECT:
			if( swf_debugShowAddress.GetBool() )
			{
				return idSWFScriptVar( va( "[object:%p]", this ) );
			}
			else
			{
				return idSWFScriptVar( "[object]" );
			}
		case SWF_OBJECT_ARRAY:
			if( swf_debugShowAddress.GetBool() )
			{
				return idSWFScriptVar( va( "[array:%p]", this ) );
			}
			else
			{
				return idSWFScriptVar( "[array]" );
			}
		case SWF_OBJECT_SPRITE:
			if( data.sprite != NULL )
			{
				if( data.sprite->parent == NULL )
				{
					return idSWFScriptVar( "[_root]" );
				}
				else
				{
					return idSWFScriptVar( va( "[%s]", data.sprite->GetName() ) );
				}
			}
			else
			{
				return idSWFScriptVar( "[NULL]" );
			}
		case SWF_OBJECT_TEXT:
			if( swf_debugShowAddress.GetBool() )
			{
				return idSWFScriptVar( va( "[edittext:%p]", this ) );
			}
			else
			{
				return idSWFScriptVar( "[edittext]" );
			}
	}
	return idSWFScriptVar( "[unknown]" );
}

/*
========================
idSWFScriptObject::GetVariable
========================
*/
idSWFScriptObject::swfNamedVar_t* idSWFScriptObject::GetVariable( int index, bool create )
{
	for( int i = 0; i < variables.Num(); i++ )
	{
		if( variables[i].index == index )
		{
			return &variables[i];
		}
	}
	if( create )
	{
		swfNamedVar_t* variable = &variables.Alloc();
		variable->flags = SWF_VAR_FLAG_NONE;
		variable->index = index;
		variable->name = va( "%d", index );
		variable->native = NULL;
		int hash = idStr::Hash( variable->name ) & ( VARIABLE_HASH_BUCKETS - 1 );
		variable->hashNext = variablesHash[hash];
		variablesHash[hash] = variables.Num() - 1;
		return variable;
	}
	return NULL;
}

/*
========================
idSWFScriptObject::GetVariable
========================
*/
idSWFScriptObject::swfNamedVar_t* idSWFScriptObject::GetVariable( const char* name, bool create )
{
	int hash = idStr::Hash( name ) & ( VARIABLE_HASH_BUCKETS - 1 );
	for( int i = variablesHash[hash]; i >= 0; i = variables[i].hashNext )
	{
		if( variables[i].name == name )
		{
			return &variables[i];
		}
	}

	if( prototype != NULL )
	{
		swfNamedVar_t* variable = prototype->GetVariable( name, false );
		if( ( variable != NULL ) && ( variable->native || !create ) )
		{
			// If the variable is native, we want to pull it from the prototype even if we're going to set it
			return variable;
		}
	}

	if( create )
	{
		swfNamedVar_t* variable = &variables.Alloc();
		variable->flags = SWF_VAR_FLAG_NONE;
		variable->index = atoi( name );
		if( variable->index == 0 && idStr::Cmp( name, "0" ) != 0 )
		{
			variable->index = -1;
		}
		variable->name = name;
		variable->native = NULL;
		variable->hashNext = variablesHash[hash];
		variablesHash[hash] = variables.Num() - 1;
		return variable;
	}
	return NULL;
}

/*
========================
idSWFScriptObject::MakeArray
========================
*/
void idSWFScriptObject::MakeArray()
{
	objectType = SWF_OBJECT_ARRAY;
	swfNamedVar_t* variable = GetVariable( "length", true );
	variable->value = idSWFScriptVar( 0 );
	variable->flags = SWF_VAR_FLAG_DONTENUM;
}

/*
========================
idSWFScriptObject::GetNestedVar
========================
*/
idSWFScriptVar idSWFScriptObject::GetNestedVar( const char* arg1, const char* arg2, const char* arg3, const char* arg4, const char* arg5, const char* arg6 )
{
	const char* const args[] = { arg1, arg2, arg3, arg4, arg5, arg6 };
	const int numArgs = sizeof( args ) / sizeof( const char* );

	idStaticList< const char*, numArgs > vars;
	for( int i = 0; i < numArgs && args[ i ] != NULL; ++i )
	{
		vars.Append( args[ i ] );
	}

	idSWFScriptObject* baseObject = this;
	idSWFScriptVar retVal;

	for( int i = 0; i < vars.Num(); ++i )
	{
		idSWFScriptVar var = baseObject->Get( vars[ i ] );

		// when at the end of object path just use the latest value as result
		if( i == vars.Num() - 1 )
		{
			retVal = var;
			break;
		}

		// encountered variable in path that wasn't an object
		if( !var.IsObject() )
		{
			retVal = idSWFScriptVar();
			break;
		}

		baseObject = var.GetObject();
	}

	return retVal;
}

/*
========================
idSWFScriptObject::GetNestedObj
========================
*/
idSWFScriptObject* idSWFScriptObject::GetNestedObj( const char* arg1, const char* arg2, const char* arg3, const char* arg4, const char* arg5, const char* arg6 )
{
	idSWFScriptVar var = GetNestedVar( arg1, arg2, arg3, arg4, arg5, arg6 );

	if( !var.IsObject() )
	{
		return NULL;
	}

	return var.GetObject();
}

/*
========================
idSWFScriptObject::GetNestedSprite
========================
*/
idSWFSpriteInstance* idSWFScriptObject::GetNestedSprite( const char* arg1, const char* arg2, const char* arg3, const char* arg4, const char* arg5, const char* arg6 )
{
	idSWFScriptVar var = GetNestedVar( arg1, arg2, arg3, arg4, arg5, arg6 );
	return var.ToSprite();

}

/*
========================
idSWFScriptObject::GetNestedText
========================
*/
idSWFTextInstance* idSWFScriptObject::GetNestedText( const char* arg1, const char* arg2, const char* arg3, const char* arg4, const char* arg5, const char* arg6 )
{
	idSWFScriptVar var = GetNestedVar( arg1, arg2, arg3, arg4, arg5, arg6 );
	return var.ToText();

}

/*
========================
idSWFScriptObject::PrintToConsole
========================
*/
void idSWFScriptObject::PrintToConsole() const
{
	if( variables.Num() > 0 )
	{
		idLib::Printf( "%d subelements:\n", variables.Num() );
		int maxVarLength = 0;

		for( int i = 0; i < variables.Num(); ++i )
		{
			const idSWFScriptObject::swfNamedVar_t& nv = variables[ i ];
			const int nameLength = idStr::Length( nv.name );
			if( maxVarLength < nameLength )
			{
				maxVarLength = nameLength;
			}
		}

		maxVarLength += 2;	// a little extra padding

		const char* const fmt = va( "%%-%ds %%-10s %%-s\n", maxVarLength );
		idLib::Printf( fmt, "Name", "Type", "Value" );
		idLib::Printf( "------------------------------------------------------------\n" );
		for( int i = 0; i < variables.Num(); ++i )
		{
			const idSWFScriptObject::swfNamedVar_t& nv = variables[ i ];
			idLib::Printf( fmt, nv.name.c_str(), nv.value.TypeOf(),
						   nv.value.ToString().c_str() );
		}
	}
	else
	{
		idLib::Printf( "No subelements\n" );
	}
}
