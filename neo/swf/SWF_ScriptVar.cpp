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

extern idCVar swf_debugShowAddress;

/*
========================
idSWFScriptVar::idSWFScriptVar
========================
*/
idSWFScriptVar::idSWFScriptVar( const idSWFScriptVar& other )
{
	type = other.type;
	value = other.value;
	if( other.type == SWF_VAR_STRING )
	{
		other.value.string->AddRef();
	}
	else if( other.type == SWF_VAR_OBJECT )
	{
		other.value.object->AddRef();
	}
	else if( other.type == SWF_VAR_FUNCTION )
	{
		other.value.function->AddRef();
	}
}

/*
========================
idSWFScriptVar::operator=
========================
*/
idSWFScriptVar& idSWFScriptVar::operator=( const idSWFScriptVar& other )
{
	if( this != &other )
	{
		Free();
		type = other.type;
		value = other.value;
		if( other.type == SWF_VAR_STRING )
		{
			other.value.string->AddRef();
		}
		else if( other.type == SWF_VAR_OBJECT )
		{
			other.value.object->AddRef();
		}
		else if( other.type == SWF_VAR_FUNCTION )
		{
			other.value.function->AddRef();
		}
	}
	return *this;
}

/*
========================
idSWFScriptVar::~idSWFScriptVar
========================
*/
idSWFScriptVar::~idSWFScriptVar()
{
	Free();
}

/*
========================
idSWFScriptVar::Free
========================
*/
void idSWFScriptVar::Free()
{
	if( type == SWF_VAR_STRING )
	{
		value.string->Release();
	}
	else if( type == SWF_VAR_OBJECT )
	{
		value.object->Release();
	}
	else if( type == SWF_VAR_FUNCTION )
	{
		value.function->Release();
	}
	value.string = NULL;
	value.function = NULL;
	value.object = NULL;
	type = SWF_VAR_UNDEF;
}

/*
========================
idSWFScriptVar::SetObject
========================
*/
void idSWFScriptVar::SetObject( idSWFScriptObject* o )
{
	Free();
	if( o == NULL )
	{
		type = SWF_VAR_NULL;
	}
	else
	{
		type = SWF_VAR_OBJECT;
		value.object = o;
		o->AddRef();
	}
}

/*
========================
idSWFScriptVar::SetFunction
========================
*/
void idSWFScriptVar::SetFunction( idSWFScriptFunction* f )
{
	Free();
	if( f == NULL )
	{
		type = SWF_VAR_NULL;
	}
	else
	{
		type = SWF_VAR_FUNCTION;
		value.function = f;
		f->AddRef();
	}
}

/*
========================
idSWFScriptVar::StrictEquals
========================
*/
bool idSWFScriptVar::StrictEquals( const idSWFScriptVar& other )
{
	if( type != other.type )
	{
		return false;
	}
	switch( type )
	{
		case SWF_VAR_STRINGID:
			return ( value.i == other.value.i );
		case SWF_VAR_STRING:
			return ( *value.string == *other.value.string );
		case SWF_VAR_FLOAT:
			return ( value.f == other.value.f );
		case SWF_VAR_BOOL:
			return ( value.b == other.value.b );
		case SWF_VAR_INTEGER:
			return ( value.i == other.value.i );
		case SWF_VAR_NULL:
			return true;
		case SWF_VAR_UNDEF:
			return true;
		case SWF_VAR_OBJECT:
			return ( value.object == other.value.object );
		case SWF_VAR_FUNCTION:
			return ( value.function == other.value.function );
		default:
			assert( false );
			return false;
	}
}

/*
========================
idSWFScriptVar::AbstractEquals
========================
*/
bool idSWFScriptVar::AbstractEquals( const idSWFScriptVar& other )
{
	if( type == other.type )
	{
		switch( type )
		{
			case SWF_VAR_STRINGID:
				return ( value.i == other.value.i );
			case SWF_VAR_STRING:
				return ( *value.string == *other.value.string );
			case SWF_VAR_FLOAT:
				return ( value.f == other.value.f );
			case SWF_VAR_BOOL:
				return ( value.b == other.value.b );
			case SWF_VAR_INTEGER:
				return ( value.i == other.value.i );
			case SWF_VAR_NULL:
				return true;
			case SWF_VAR_UNDEF:
				return true;
			case SWF_VAR_OBJECT:
				return ( value.object == other.value.object );
			case SWF_VAR_FUNCTION:
				return ( value.function == other.value.function );
			default:
				assert( false );
				return false;
		}
	}
	switch( type )
	{
		case SWF_VAR_STRINGID:
			return ToString() == other.ToString();
		case SWF_VAR_STRING:
			switch( other.type )
			{
				case SWF_VAR_STRINGID:
					return *value.string == other.ToString();
				case SWF_VAR_FLOAT:
					return ToFloat() == other.value.f;
				case SWF_VAR_BOOL:
					return ToBool() == other.value.b;
				case SWF_VAR_INTEGER:
					return ToInteger() == other.value.i;
				case SWF_VAR_OBJECT:
					return *value.string == other.ToString();
				default:
					return false;
			}
		case SWF_VAR_FLOAT:
			return ( other.ToFloat() == value.f );
		case SWF_VAR_BOOL:
			return ( other.ToBool() == value.b );
		case SWF_VAR_INTEGER:
			return ( other.ToInteger() == value.i );
		case SWF_VAR_NULL:
			return ( other.type == SWF_VAR_UNDEF );
		case SWF_VAR_UNDEF:
			return ( other.type == SWF_VAR_NULL );
		case SWF_VAR_OBJECT:
			switch( other.type )
			{
				case SWF_VAR_STRING:
					return ToString() == *other.value.string;
				case SWF_VAR_FLOAT:
					return ToFloat() == other.value.f;
				case SWF_VAR_BOOL:
					return ToBool() == other.value.b;
				case SWF_VAR_INTEGER:
					return ToInteger() == other.value.i;
				default:
					return false;
			}
		case SWF_VAR_FUNCTION:
			return false;
		default:
			assert( false );
			return false;
	}
}

/*
========================
idSWFScriptVar::ToString
========================
*/
idStr idSWFScriptVar::ToString() const
{
	switch( type )
	{
		case SWF_VAR_STRINGID:
			return idStrId( value.i ).GetLocalizedString();
		case SWF_VAR_STRING:
			return *value.string;
		// RB begin
		case SWF_VAR_RESULT:
			return *value.string;
		// RB end

		case SWF_VAR_FLOAT:
			return va( "%g", value.f );
		case SWF_VAR_BOOL:
			return value.b ? "true" : "false";
		case SWF_VAR_INTEGER:
			return va( "%i", value.i );

		case SWF_VAR_NULL:
			return "[null]";
		case SWF_VAR_UNDEF:
			return "[undefined]";
		case SWF_VAR_OBJECT:
			return value.object->DefaultValue( true ).ToString();
		case SWF_VAR_FUNCTION:
			if( swf_debugShowAddress.GetBool() )
			{
				return va( "[function:%p]", value.function );
			}
			else
			{
				return "[function]";
			}
		default:
			assert( false );
			return "";
	}
}

/*
========================
idSWFScriptVar::ToFloat
========================
*/
float idSWFScriptVar::ToFloat() const
{
	switch( type )
	{
		case SWF_VAR_STRING:
			return atof( *value.string );

		case SWF_VAR_FLOAT:
			return value.f;
		case SWF_VAR_BOOL:
			return ( float )value.b;
		case SWF_VAR_INTEGER:
			return ( float )value.i;

		case SWF_VAR_OBJECT:
			return value.object->DefaultValue( false ).ToFloat();

		case SWF_VAR_FUNCTION:
		case SWF_VAR_NULL:
		case SWF_VAR_UNDEF:
			return 0.0f;
		default:
			assert( false );
			return 0.0f;
	}
}

/*
========================
idSWFScriptVar::ToBool
========================
*/
bool idSWFScriptVar::ToBool() const
{
	switch( type )
	{
		case SWF_VAR_STRING:
			return ( value.string->Icmp( "true" ) == 0 || value.string->Icmp( "1" ) == 0 );

		case SWF_VAR_FLOAT:
			return ( value.f != 0.0f );
		case SWF_VAR_BOOL:
			return value.b;
		case SWF_VAR_INTEGER:
			return value.i != 0;

		case SWF_VAR_OBJECT:
			return value.object->DefaultValue( false ).ToBool();

		case SWF_VAR_FUNCTION:
		case SWF_VAR_NULL:
		case SWF_VAR_UNDEF:
			return false;
		default:
			assert( false );
			return false;
	}
}

/*
========================
idSWFScriptVar::ToInteger
========================
*/
int32 idSWFScriptVar::ToInteger() const
{
	switch( type )
	{
		case SWF_VAR_STRING:
			return atoi( *value.string );

		case SWF_VAR_FLOAT:
			return idMath::Ftoi( value.f );

		case SWF_VAR_BOOL:
			return value.b ? 1 : 0;
		case SWF_VAR_INTEGER:
			return value.i;

		case SWF_VAR_OBJECT:
			return value.object->DefaultValue( false ).ToInteger();

		case SWF_VAR_FUNCTION:
		case SWF_VAR_NULL:
		case SWF_VAR_UNDEF:
			return 0;
		default:
			assert( false );
			return 0;
	}
}

/*
========================
idSWFScriptVar::ToSprite
========================
*/
idSWFSpriteInstance* idSWFScriptVar::ToSprite()
{
	if( IsObject() && value.object != NULL )
	{
		return value.object->GetSprite();
	}

	return NULL;
}

/*
========================
idSWFScriptVar::ToText
========================
*/
idSWFTextInstance* idSWFScriptVar::ToText()
{
	if( IsObject() && value.object != NULL )
	{
		return value.object->GetText();
	}

	return NULL;
}

/*
========================
idSWFScriptVar::GetNestedVar
========================
*/
idSWFScriptVar idSWFScriptVar::GetNestedVar( const char* arg1, const char* arg2, const char* arg3, const char* arg4, const char* arg5, const char* arg6 )
{
	if( !IsObject() )
	{
		return idSWFScriptVar();
	}

	return GetObject()->GetNestedVar( arg1, arg2, arg3, arg4, arg5, arg6 );
}

/*
========================
idSWFScriptVar::GetNestedObj
========================
*/
idSWFScriptObject* idSWFScriptVar::GetNestedObj( const char* arg1, const char* arg2, const char* arg3, const char* arg4, const char* arg5, const char* arg6 )
{
	if( !IsObject() )
	{
		return NULL;
	}

	return GetObject()->GetNestedObj( arg1, arg2, arg3, arg4, arg5, arg6 );
}

/*
========================
idSWFScriptVar::GetNestedSprite
========================
*/
idSWFSpriteInstance* idSWFScriptVar::GetNestedSprite( const char* arg1, const char* arg2, const char* arg3, const char* arg4, const char* arg5, const char* arg6 )
{
	if( !IsObject() )
	{
		return NULL;
	}

	return GetObject()->GetNestedSprite( arg1, arg2, arg3, arg4, arg5, arg6 );
}

/*
========================
idSWFScriptVar::GetNestedSprite
========================
*/
idSWFTextInstance* idSWFScriptVar::GetNestedText( const char* arg1, const char* arg2, const char* arg3, const char* arg4, const char* arg5, const char* arg6 )
{
	if( !IsObject() )
	{
		return NULL;
	}

	return GetObject()->GetNestedText( arg1, arg2, arg3, arg4, arg5, arg6 );
}

/*
========================
idSWFScriptVar::TypeOf
========================
*/
const char* idSWFScriptVar::TypeOf() const
{
	switch( type )
	{
		case SWF_VAR_STRINGID:
			return "stringid";
		case SWF_VAR_STRING:
			return "string";
		// RB begin
		case SWF_VAR_RESULT:
			return "result";
		// RB end

		case SWF_VAR_FLOAT:
			return "number";
		case SWF_VAR_BOOL:
			return "boolean";
		case SWF_VAR_INTEGER:
			return "number";

		case SWF_VAR_OBJECT:
			if( value.object->GetSprite() != NULL )
			{
				return "movieclip";
			}
			else if( value.object->GetText() != NULL )
			{
				return "text";
			}
			else
			{
				return "object";
			}

		case SWF_VAR_FUNCTION:
			return "function";
		case SWF_VAR_NULL:
			return "null";
		case SWF_VAR_UNDEF:
			return "undefined";
		default:
			assert( false );
			return "";
	}
}

/*
========================
idSWFScriptVar::PrintToConsole
========================
*/
void idSWFScriptVar::PrintToConsole() const
{
	idLib::Printf( "Object type: %s\n", TypeOf() );

	if( IsObject() )
	{
		GetObject()->PrintToConsole();
	}
	else if( IsNumeric() )
	{
		idLib::Printf( "%d\n", ToInteger() );
	}
	else if( IsString() )
	{
		idLib::Printf( "%s\n", ToString().c_str() );
	}
	else
	{
		idLib::Printf( "unknown\n" );
	}
}
