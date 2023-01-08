/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2014 Robert Beckebans

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

//#define DEBUG_EVAL
#define MAX_DEFINEPARMS				128
#define DEFINEHASHSIZE				2048

#define TOKEN_FL_RECURSIVE_DEFINE	1

define_t* idParser::globaldefines;

/*
================
idParser::SetBaseFolder
================
*/
void idParser::SetBaseFolder( const char* path )
{
	idLexer::SetBaseFolder( path );
}

/*
================
idParser::AddGlobalDefine
================
*/
int idParser::AddGlobalDefine( const char* string )
{
	define_t* define;

	define = idParser::DefineFromString( string );
	if( !define )
	{
		return false;
	}
	define->next = globaldefines;
	globaldefines = define;
	return true;
}

/*
================
idParser::RemoveGlobalDefine
================
*/
int idParser::RemoveGlobalDefine( const char* name )
{
	define_t* d, *prev;

	for( prev = NULL, d = idParser::globaldefines; d; prev = d, d = d->next )
	{
		if( !strcmp( d->name, name ) )
		{
			break;
		}
	}
	if( d )
	{
		if( prev )
		{
			prev->next = d->next;
		}
		else
		{
			idParser::globaldefines = d->next;
		}
		idParser::FreeDefine( d );
		return true;
	}
	return false;
}

/*
================
idParser::RemoveAllGlobalDefines
================
*/
void idParser::RemoveAllGlobalDefines()
{
	define_t* define;

	for( define = globaldefines; define; define = globaldefines )
	{
		globaldefines = globaldefines->next;
		idParser::FreeDefine( define );
	}
}


/*
===============================================================================

idParser

===============================================================================
*/

/*
================
idParser::PrintDefine
================
*/
void idParser::PrintDefine( define_t* define )
{
	idLib::common->Printf( "define->name = %s\n", define->name );
	idLib::common->Printf( "define->flags = %d\n", define->flags );
	idLib::common->Printf( "define->builtin = %d\n", define->builtin );
	idLib::common->Printf( "define->numparms = %d\n", define->numparms );
}

/*
================
PC_PrintDefineHashTable
================
* /
static void PC_PrintDefineHashTable(define_t **definehash) {
	int i;
	define_t *d;

	for (i = 0; i < DEFINEHASHSIZE; i++) {
		Log_Write("%4d:", i);
		for (d = definehash[i]; d; d = d->hashnext) {
			Log_Write(" %s", d->name);
		}
		Log_Write("\n");
	}
}
*/

/*
================
PC_NameHash
================
*/
ID_INLINE int PC_NameHash( const char* name )
{
	int hash, i;

	hash = 0;
	for( i = 0; name[i] != '\0'; i++ )
	{
		hash += name[i] * ( 119 + i );
	}
	hash = ( hash ^ ( hash >> 10 ) ^ ( hash >> 20 ) ) & ( DEFINEHASHSIZE - 1 );
	return hash;
}

/*
================
idParser::AddDefineToHash
================
*/
void idParser::AddDefineToHash( define_t* define, define_t** definehash )
{
	int hash;

	hash = PC_NameHash( define->name );
	define->hashnext = definehash[hash];
	definehash[hash] = define;
}

/*
================
FindHashedDefine
================
*/
define_t* idParser::FindHashedDefine( define_t** definehash, const char* name )
{
	define_t* d;
	int hash;

	hash = PC_NameHash( name );
	for( d = definehash[hash]; d; d = d->hashnext )
	{
		if( !strcmp( d->name, name ) )
		{
			return d;
		}
	}
	return NULL;
}

/*
================
idParser::FindDefine
================
*/
define_t* idParser::FindDefine( define_t* defines, const char* name )
{
	define_t* d;

	for( d = defines; d; d = d->next )
	{
		if( !strcmp( d->name, name ) )
		{
			return d;
		}
	}
	return NULL;
}

/*
================
idParser::FindDefineParm
================
*/
int idParser::FindDefineParm( define_t* define, const char* name )
{
	idToken* p;
	int i;

	i = 0;
	for( p = define->parms; p; p = p->next )
	{
		if( ( *p ) == name )
		{
			return i;
		}
		i++;
	}
	return -1;
}

/*
================
idParser::CopyDefine
================
*/
define_t* idParser::CopyDefine( define_t* define )
{
	define_t* newdefine;
	idToken* token, *newtoken, *lasttoken;

	newdefine = ( define_t* ) Mem_Alloc( sizeof( define_t ) + strlen( define->name ) + 1, TAG_IDLIB_PARSER );
	//copy the define name
	newdefine->name = ( char* ) newdefine + sizeof( define_t );
	strcpy( newdefine->name, define->name );
	newdefine->flags = define->flags;
	newdefine->builtin = define->builtin;
	newdefine->numparms = define->numparms;
	//the define is not linked
	newdefine->next = NULL;
	newdefine->hashnext = NULL;
	//copy the define tokens
	newdefine->tokens = NULL;
	for( lasttoken = NULL, token = define->tokens; token; token = token->next )
	{
		newtoken = new( TAG_IDLIB_PARSER ) idToken( token );
		newtoken->next = NULL;
		if( lasttoken )
		{
			lasttoken->next = newtoken;
		}
		else
		{
			newdefine->tokens = newtoken;
		}
		lasttoken = newtoken;
	}
	//copy the define parameters
	newdefine->parms = NULL;
	for( lasttoken = NULL, token = define->parms; token; token = token->next )
	{
		newtoken = new( TAG_IDLIB_PARSER ) idToken( token );
		newtoken->next = NULL;
		if( lasttoken )
		{
			lasttoken->next = newtoken;
		}
		else
		{
			newdefine->parms = newtoken;
		}
		lasttoken = newtoken;
	}
	return newdefine;
}

/*
================
idParser::FreeDefine
================
*/
void idParser::FreeDefine( define_t* define )
{
	idToken* t, *next;

	//free the define parameters
	for( t = define->parms; t; t = next )
	{
		next = t->next;
		delete t;
	}
	//free the define tokens
	for( t = define->tokens; t; t = next )
	{
		next = t->next;
		delete t;
	}
	//free the define
	Mem_Free( define );
}

/*
================
idParser::DefineFromString
================
*/
define_t* idParser::DefineFromString( const char* string )
{
	idParser src;
	define_t* def;

	if( !src.LoadMemory( string, strlen( string ), "*defineString" ) )
	{
		return NULL;
	}
	// create a define from the source
	if( !src.Directive_define() )
	{
		src.FreeSource();
		return NULL;
	}
	def = src.CopyFirstDefine();
	src.FreeSource();
	//if the define was created succesfully
	return def;
}

/*
================
idParser::Error
================
*/
void idParser::Error( const char* str, ... ) const
{
	char text[MAX_STRING_CHARS];
	va_list ap;

	va_start( ap, str );
	vsprintf( text, str, ap );
	va_end( ap );
	if( idParser::scriptstack )
	{
		idParser::scriptstack->Error( text );
	}
}

/*
================
idParser::Warning
================
*/
void idParser::Warning( const char* str, ... ) const
{
	char text[MAX_STRING_CHARS];
	va_list ap;

	va_start( ap, str );
	vsprintf( text, str, ap );
	va_end( ap );
	if( idParser::scriptstack )
	{
		idParser::scriptstack->Warning( text );
	}
}

/*
================
idParser::PushIndent
================
*/
void idParser::PushIndent( int type, int skip )
{
	indent_t* indent;

	indent = ( indent_t* ) Mem_Alloc( sizeof( indent_t ), TAG_IDLIB_PARSER );
	indent->type = type;
	indent->script = idParser::scriptstack;
	indent->skip = ( skip != 0 );
	idParser::skip += indent->skip;
	indent->next = idParser::indentstack;
	idParser::indentstack = indent;
}

/*
================
idParser::PopIndent
================
*/
void idParser::PopIndent( int* type, int* skip )
{
	indent_t* indent;

	*type = 0;
	*skip = 0;

	indent = idParser::indentstack;
	if( !indent )
	{
		return;
	}

	// must be an indent from the current script
	if( idParser::indentstack->script != idParser::scriptstack )
	{
		return;
	}

	*type = indent->type;
	*skip = indent->skip;
	idParser::indentstack = idParser::indentstack->next;
	idParser::skip -= indent->skip;
	Mem_Free( indent );
}

/*
================
idParser::PushScript
================
*/
void idParser::PushScript( idLexer* script )
{
	idLexer* s;

	for( s = idParser::scriptstack; s; s = s->next )
	{
		if( !idStr::Icmp( s->GetFileName(), script->GetFileName() ) )
		{
			idParser::Warning( "'%s' recursively included", script->GetFileName() );
			return;
		}
	}
	//push the script on the script stack
	script->next = idParser::scriptstack;
	idParser::scriptstack = script;
}

/*
================
idParser::ReadSourceToken
================
*/
int idParser::ReadSourceToken( idToken* token )
{
	idToken* t;
	idLexer* script;
	int type, skip, changedScript;

	if( !idParser::scriptstack )
	{
		idLib::common->FatalError( "idParser::ReadSourceToken: not loaded" );
		return false;
	}
	changedScript = 0;
	// if there's no token already available
	while( !idParser::tokens )
	{
		// if there's a token to read from the script
		if( idParser::scriptstack->ReadToken( token ) )
		{
			token->linesCrossed += changedScript;

			// set the marker based on the start of the token read in
			if( !marker_p )
			{
				marker_p = token->whiteSpaceEnd_p;
			}
			return true;
		}
		// if at the end of the script
		if( idParser::scriptstack->EndOfFile() )
		{
			// remove all indents of the script
			while( idParser::indentstack && idParser::indentstack->script == idParser::scriptstack )
			{
				idParser::Warning( "missing #endif" );
				idParser::PopIndent( &type, &skip );
			}
			changedScript = 1;
		}
		// if this was the initial script
		if( !idParser::scriptstack->next )
		{
			return false;
		}
		// remove the script and return to the previous one
		script = idParser::scriptstack;
		idParser::scriptstack = idParser::scriptstack->next;
		delete script;
	}
	// copy the already available token
	*token = idParser::tokens;
	// remove the token from the source
	t = idParser::tokens;
	assert( idParser::tokens != NULL );
	idParser::tokens = idParser::tokens->next;
	delete t;
	return true;
}

/*
================
idParser::UnreadSourceToken
================
*/
int idParser::UnreadSourceToken( idToken* token )
{
	idToken* t;

	t = new( TAG_IDLIB_PARSER ) idToken( token );
	t->next = idParser::tokens;
	idParser::tokens = t;
	return true;
}

/*
================
idParser::ReadDefineParms
================
*/
int idParser::ReadDefineParms( define_t* define, idToken** parms, int maxparms )
{
	define_t* newdefine;
	idToken token, *t, *last;
	int i, done, lastcomma, numparms, indent;

	if( !idParser::ReadSourceToken( &token ) )
	{
		idParser::Error( "define '%s' missing parameters", define->name );
		return false;
	}

	if( define->numparms > maxparms )
	{
		idParser::Error( "define with more than %d parameters", maxparms );
		return false;
	}

	for( i = 0; i < define->numparms; i++ )
	{
		parms[i] = NULL;
	}
	// if no leading "("
	if( token != "(" )
	{
		idParser::UnreadSourceToken( &token );
		idParser::Error( "define '%s' missing parameters", define->name );
		return false;
	}
	// read the define parameters
	for( done = 0, numparms = 0, indent = 1; !done; )
	{
		if( numparms >= maxparms )
		{
			idParser::Error( "define '%s' with too many parameters", define->name );
			return false;
		}
		parms[numparms] = NULL;
		lastcomma = 1;
		last = NULL;
		while( !done )
		{

			if( !idParser::ReadSourceToken( &token ) )
			{
				idParser::Error( "define '%s' incomplete", define->name );
				return false;
			}

			if( token == "," )
			{
				if( indent <= 1 )
				{
					if( lastcomma )
					{
						idParser::Warning( "too many comma's" );
					}
					if( numparms >= define->numparms )
					{
						idParser::Warning( "too many define parameters" );
					}
					lastcomma = 1;
					break;
				}
			}
			else if( token == "(" )
			{
				indent++;
			}
			else if( token == ")" )
			{
				indent--;
				if( indent <= 0 )
				{
					if( !parms[define->numparms - 1] )
					{
						idParser::Warning( "too few define parameters" );
					}
					done = 1;
					break;
				}
			}
			else if( token.type == TT_NAME )
			{
				newdefine = FindHashedDefine( idParser::definehash, token.c_str() );
				if( newdefine )
				{
					if( !idParser::ExpandDefineIntoSource( &token, newdefine ) )
					{
						return false;
					}
					continue;
				}
			}

			lastcomma = 0;

			if( numparms < define->numparms )
			{

				t = new( TAG_IDLIB_PARSER ) idToken( token );
				t->next = NULL;
				if( last )
				{
					last->next = t;
				}
				else
				{
					parms[numparms] = t;
				}
				last = t;
			}
		}
		numparms++;
	}
	return true;
}

/*
================
idParser::StringizeTokens
================
*/
int idParser::StringizeTokens( idToken* tokens, idToken* token )
{
	idToken* t;

	token->type = TT_STRING;
	token->whiteSpaceStart_p = NULL;
	token->whiteSpaceEnd_p = NULL;
	( *token ) = "";
	for( t = tokens; t; t = t->next )
	{
		token->Append( t->c_str() );
	}
	return true;
}

/*
================
idParser::MergeTokens
================
*/
int idParser::MergeTokens( idToken* t1, idToken* t2 )
{
	// merging of a name with a name or number
	if( t1->type == TT_NAME && ( t2->type == TT_NAME || ( t2->type == TT_NUMBER && !( t2->subtype & TT_FLOAT ) ) ) )
	{
		t1->Append( t2->c_str() );
		return true;
	}
	// merging of two strings
	if( t1->type == TT_STRING && t2->type == TT_STRING )
	{
		t1->Append( t2->c_str() );
		return true;
	}
	// merging of two numbers
	if( t1->type == TT_NUMBER && t2->type == TT_NUMBER &&
			!( t1->subtype & ( TT_HEX | TT_BINARY ) ) && !( t2->subtype & ( TT_HEX | TT_BINARY ) ) &&
			( !( t1->subtype & TT_FLOAT ) || !( t2->subtype & TT_FLOAT ) ) )
	{
		t1->Append( t2->c_str() );
		return true;
	}

	return false;
}

/*
================
idParser::AddBuiltinDefines
================
*/
void idParser::AddBuiltinDefines()
{
	int i;
	define_t* define;
	struct builtin
	{
		const char* string;
		int id;
	} builtin[] =
	{
		{ "__LINE__",	BUILTIN_LINE },
		{ "__FILE__",	BUILTIN_FILE },
		{ "__DATE__",	BUILTIN_DATE },
		{ "__TIME__",	BUILTIN_TIME },
		{ "__STDC__", BUILTIN_STDC },
		{ NULL, 0 }
	};

	for( i = 0; builtin[i].string; i++ )
	{
		define = ( define_t* ) Mem_Alloc( sizeof( define_t ) + strlen( builtin[i].string ) + 1, TAG_IDLIB_PARSER );
		define->name = ( char* ) define + sizeof( define_t );
		strcpy( define->name, builtin[i].string );
		define->flags = DEFINE_FIXED;
		define->builtin = builtin[i].id;
		define->numparms = 0;
		define->parms = NULL;
		define->tokens = NULL;
		// add the define to the source
		AddDefineToHash( define, idParser::definehash );
	}
}

/*
================
idParser::CopyFirstDefine
================
*/
define_t* idParser::CopyFirstDefine()
{
	int i;

	for( i = 0; i < DEFINEHASHSIZE; i++ )
	{
		if( idParser::definehash[i] )
		{
			return CopyDefine( idParser::definehash[i] );
		}
	}
	return NULL;
}

static idStr PreProcessorDate()
{
	time_t t = time( NULL );
	char* curtime = ctime( &t );
	if( idStr::Length( curtime ) < 24 )
	{
		return idStr( "*** BAD CURTIME ***" );
	}
	idStr	str = "\"";
	// skip DAY, extract MMM DD
	for( int i = 4 ; i < 10 ; i++ )
	{
		str.Append( curtime[i] );
	}
	// skip time, extract space+YYYY
	for( int i = 19 ; i < 24 ; i++ )
	{
		str.Append( curtime[i] );
	}
	str.Append( "\"" );
	return str;
}

static idStr PreProcessorTime()
{
	time_t t = time( NULL );
	char* curtime = ctime( &t );
	if( idStr::Length( curtime ) < 24 )
	{
		return idStr( "*** BAD CURTIME ***" );
	}

	idStr	str = "\"";
	for( int i = 11 ; i < 19 ; i++ )
	{
		str.Append( curtime[i] );
	}
	str.Append( "\"" );
	return str;
}

CONSOLE_COMMAND( TestPreprocessorMacros, "check analyze warning", 0 )
{
	idLib::Printf( "%s : %s\n", __DATE__, PreProcessorDate().c_str() );
	idLib::Printf( "%s : %s\n", __TIME__, PreProcessorTime().c_str() );
}

/*
================
idParser::ExpandBuiltinDefine
================
*/
int idParser::ExpandBuiltinDefine( idToken* deftoken, define_t* define, idToken** firsttoken, idToken** lasttoken )
{
	idToken* token;
	char buf[MAX_STRING_CHARS];

	token = new( TAG_IDLIB_PARSER ) idToken( deftoken );
	switch( define->builtin )
	{
		case BUILTIN_LINE:
		{
			sprintf( buf, "%d", deftoken->line );
			( *token ) = buf;
			token->intvalue = deftoken->line;
			token->floatvalue = deftoken->line;
			token->type = TT_NUMBER;
			token->subtype = TT_DECIMAL | TT_INTEGER | TT_VALUESVALID;
			token->line = deftoken->line;
			token->linesCrossed = deftoken->linesCrossed;
			token->flags = 0;
			*firsttoken = token;
			*lasttoken = token;
			break;
		}
		case BUILTIN_FILE:
		{
			( *token ) = idParser::scriptstack->GetFileName();
			token->type = TT_NAME;
			token->subtype = token->Length();
			token->line = deftoken->line;
			token->linesCrossed = deftoken->linesCrossed;
			token->flags = 0;
			*firsttoken = token;
			*lasttoken = token;
			break;
		}
		case BUILTIN_DATE:
		{
			*token = PreProcessorDate();
			token->type = TT_STRING;
			token->subtype = token->Length();
			token->line = deftoken->line;
			token->linesCrossed = deftoken->linesCrossed;
			token->flags = 0;
			*firsttoken = token;
			*lasttoken = token;
			break;
		}
		case BUILTIN_TIME:
		{
			*token = PreProcessorTime();
			token->type = TT_STRING;
			token->subtype = token->Length();
			token->line = deftoken->line;
			token->linesCrossed = deftoken->linesCrossed;
			token->flags = 0;
			*firsttoken = token;
			*lasttoken = token;
			break;
		}
		case BUILTIN_STDC:
		{
			idParser::Warning( "__STDC__ not supported\n" );
			*firsttoken = NULL;
			*lasttoken = NULL;
			break;
		}
		default:
		{
			*firsttoken = NULL;
			*lasttoken = NULL;
			break;
		}
	}
	return true;
}

/*
================
idParser::ExpandDefine
================
*/
int idParser::ExpandDefine( idToken* deftoken, define_t* define, idToken** firsttoken, idToken** lasttoken )
{
	idToken* parms[MAX_DEFINEPARMS], *dt, *pt, *t;
	idToken* t1, *t2, *first, *last, *nextpt, token;
	int parmnum, i;

	// if it is a builtin define
	if( define->builtin )
	{
		return idParser::ExpandBuiltinDefine( deftoken, define, firsttoken, lasttoken );
	}
	// if the define has parameters
	if( define->numparms )
	{
		if( !idParser::ReadDefineParms( define, parms, MAX_DEFINEPARMS ) )
		{
			return false;
		}
#ifdef DEBUG_EVAL
		for( i = 0; i < define->numparms; i++ )
		{
			Log_Write( "define parms %d:", i );
			for( pt = parms[i]; pt; pt = pt->next )
			{
				Log_Write( "%s", pt->c_str() );
			}
		}
#endif //DEBUG_EVAL
	}
	// empty list at first
	first = NULL;
	last = NULL;
	// create a list with tokens of the expanded define
	for( dt = define->tokens; dt; dt = dt->next )
	{
		parmnum = -1;
		// if the token is a name, it could be a define parameter
		if( dt->type == TT_NAME )
		{
			parmnum = FindDefineParm( define, dt->c_str() );
		}
		// if it is a define parameter
		if( parmnum >= 0 )
		{
			for( pt = parms[parmnum]; pt; pt = pt->next )
			{
				t = new( TAG_IDLIB_PARSER ) idToken( pt );
				//add the token to the list
				t->next = NULL;
				if( last )
				{
					last->next = t;
				}
				else
				{
					first = t;
				}
				last = t;
			}
		}
		else
		{
			// if stringizing operator
			if( ( *dt ) == "#" )
			{
				// the stringizing operator must be followed by a define parameter
				if( dt->next )
				{
					parmnum = FindDefineParm( define, dt->next->c_str() );
				}
				else
				{
					parmnum = -1;
				}

				if( parmnum >= 0 )
				{
					// step over the stringizing operator
					dt = dt->next;
					// stringize the define parameter tokens
					if( !idParser::StringizeTokens( parms[parmnum], &token ) )
					{
						idParser::Error( "can't stringize tokens" );
						return false;
					}
					t = new( TAG_IDLIB_PARSER ) idToken( token );
					t->line = deftoken->line;
				}
				else
				{
					idParser::Warning( "stringizing operator without define parameter" );
					continue;
				}
			}
			else
			{
				t = new( TAG_IDLIB_PARSER ) idToken( dt );
				t->line = deftoken->line;
			}
			// add the token to the list
			t->next = NULL;
// the token being read from the define list should use the line number of
// the original file, not the header file
			t->line = deftoken->line;

			if( last )
			{
				last->next = t;
			}
			else
			{
				first = t;
			}
			last = t;
		}
	}
	// check for the merging operator
	for( t = first; t; )
	{
		if( t->next )
		{
			// if the merging operator
			if( ( *t->next ) == "##" )
			{
				t1 = t;
				t2 = t->next->next;
				if( t2 )
				{
					if( !idParser::MergeTokens( t1, t2 ) )
					{
						idParser::Error( "can't merge '%s' with '%s'", t1->c_str(), t2->c_str() );
						return false;
					}
					delete t1->next;
					t1->next = t2->next;
					if( t2 == last )
					{
						last = t1;
					}
					delete t2;
					continue;
				}
			}
		}
		t = t->next;
	}
	// store the first and last token of the list
	*firsttoken = first;
	*lasttoken = last;
	// free all the parameter tokens
	for( i = 0; i < define->numparms; i++ )
	{
		for( pt = parms[i]; pt; pt = nextpt )
		{
			nextpt = pt->next;
			delete pt;
		}
	}

	return true;
}

/*
================
idParser::ExpandDefineIntoSource
================
*/
int idParser::ExpandDefineIntoSource( idToken* deftoken, define_t* define )
{
	idToken* firsttoken, *lasttoken;

	if( !idParser::ExpandDefine( deftoken, define, &firsttoken, &lasttoken ) )
	{
		return false;
	}
	// if the define is not empty
	if( firsttoken && lasttoken )
	{
		firsttoken->linesCrossed += deftoken->linesCrossed;
		lasttoken->next = idParser::tokens;
		idParser::tokens = firsttoken;
	}
	return true;
}

/*
================
idParser::ReadLine

reads a token from the current line, continues reading on the next
line only if a backslash '\' is found
================
*/
int idParser::ReadLine( idToken* token )
{
	int crossline;

	crossline = 0;
	do
	{
		if( !idParser::ReadSourceToken( token ) )
		{
			return false;
		}

		if( token->linesCrossed > crossline )
		{
			idParser::UnreadSourceToken( token );
			return false;
		}
		crossline = 1;
	}
	while( ( *token ) == "\\" );
	return true;
}

/*
================
idParser::Directive_include
================
*/
// RB: added token as parameter
int idParser::Directive_include( idToken* token, bool supressWarning )
{
	idLexer* script;
	idStr path;

	if( !idParser::ReadSourceToken( token ) )
	{
		idParser::Error( "#include without file name" );
		return false;
	}
	if( token->linesCrossed > 0 )
	{
		idParser::Error( "#include without file name" );
		return false;
	}
	if( token->type == TT_STRING )
	{
		script = new( TAG_IDLIB_PARSER ) idLexer;
		// try relative to the current file
		path = scriptstack->GetFileName();
		path.StripFilename();
		// first remove any trailing path overlap with token
		idStr token_path = *token;
		if( !path.StripTrailingOnce( token_path.StripFilename() ) )
		{
			// if no path overlap add separator before token
			path += "/";
		}
		path += *token;
		// try assuming a full os path from GetFileName()
		if( !script->LoadFile( path, true ) )
		{
			// try from the token path
			path = *token;
			if( !script->LoadFile( path, OSPath ) )
			{
				// try from the include path
				path = includepath + *token;
				if( !script->LoadFile( path, OSPath ) )
				{
					delete script;
					script = NULL;
				}
			}
		}
	}
	else if( token->type == TT_PUNCTUATION && *token == "<" )
	{
		path = idParser::includepath;
		while( idParser::ReadSourceToken( token ) )
		{
			if( token->linesCrossed > 0 )
			{
				idParser::UnreadSourceToken( token );
				break;
			}
			if( token->type == TT_PUNCTUATION && *token == ">" )
			{
				break;
			}
			path += *token;
		}
		if( *token != ">" )
		{
			idParser::Warning( "#include missing trailing >" );
		}
		if( !path.Length() )
		{
			idParser::Error( "#include without file name between < >" );
			return false;
		}
		if( idParser::flags & LEXFL_NOBASEINCLUDES )
		{
			return true;
		}
		script = new( TAG_IDLIB_PARSER ) idLexer;
		if( !script->LoadFile( includepath + path, OSPath ) )
		{
			delete script;
			script = NULL;
		}
	}
	else
	{
		idParser::Error( "#include without file name" );
		return false;
	}

	if( !script )
	{
		if( !supressWarning )
		{
			idParser::Error( "file '%s' not found", path.c_str() );
		}
		return false;
	}
	script->SetFlags( idParser::flags );
	script->SetPunctuations( idParser::punctuations );
	idParser::PushScript( script );
	return true;
}
// RB end

/*
================
idParser::Directive_undef
================
*/
int idParser::Directive_undef()
{
	idToken token;
	define_t* define, *lastdefine;
	int hash;

	//
	if( !idParser::ReadLine( &token ) )
	{
		idParser::Error( "undef without name" );
		return false;
	}
	if( token.type != TT_NAME )
	{
		idParser::UnreadSourceToken( &token );
		idParser::Error( "expected name but found '%s'", token.c_str() );
		return false;
	}

	hash = PC_NameHash( token.c_str() );
	for( lastdefine = NULL, define = idParser::definehash[hash]; define; define = define->hashnext )
	{
		if( !strcmp( define->name, token.c_str() ) )
		{
			if( define->flags & DEFINE_FIXED )
			{
				idParser::Warning( "can't undef '%s'", token.c_str() );
			}
			else
			{
				if( lastdefine )
				{
					lastdefine->hashnext = define->hashnext;
				}
				else
				{
					idParser::definehash[hash] = define->hashnext;
				}
				FreeDefine( define );
			}
			break;
		}
		lastdefine = define;
	}
	return true;
}

/*
================
idParser::Directive_define
================
*/
int idParser::Directive_define()
{
	idToken token, *t, *last;
	define_t* define;

	if( !idParser::ReadLine( &token ) )
	{
		idParser::Error( "#define without name" );
		return false;
	}
	if( token.type != TT_NAME )
	{
		idParser::UnreadSourceToken( &token );
		idParser::Error( "expected name after #define, found '%s'", token.c_str() );
		return false;
	}
	// check if the define already exists
	define = FindHashedDefine( idParser::definehash, token.c_str() );
	if( define )
	{
		if( define->flags & DEFINE_FIXED )
		{
			idParser::Error( "can't redefine '%s'", token.c_str() );
			return false;
		}
		idParser::Warning( "redefinition of '%s'", token.c_str() );
		// unread the define name before executing the #undef directive
		idParser::UnreadSourceToken( &token );
		if( !idParser::Directive_undef() )
		{
			return false;
		}
		// if the define was not removed (define->flags & DEFINE_FIXED)
		define = FindHashedDefine( idParser::definehash, token.c_str() );
	}
	// allocate define
	define = ( define_t* ) Mem_ClearedAlloc( sizeof( define_t ) + token.Length() + 1, TAG_IDLIB_PARSER );
	define->name = ( char* ) define + sizeof( define_t );
	strcpy( define->name, token.c_str() );
	// add the define to the source
	AddDefineToHash( define, idParser::definehash );
	// if nothing is defined, just return
	if( !idParser::ReadLine( &token ) )
	{
		return true;
	}
	// if it is a define with parameters
	if( token.WhiteSpaceBeforeToken() == 0 && token == "(" )
	{
		// read the define parameters
		last = NULL;
		if( !idParser::CheckTokenString( ")" ) )
		{
			while( 1 )
			{
				if( !idParser::ReadLine( &token ) )
				{
					idParser::Error( "expected define parameter" );
					return false;
				}
				// if it isn't a name
				if( token.type != TT_NAME )
				{
					idParser::Error( "invalid define parameter" );
					return false;
				}

				if( FindDefineParm( define, token.c_str() ) >= 0 )
				{
					idParser::Error( "two the same define parameters" );
					return false;
				}
				// add the define parm
				t = new( TAG_IDLIB_PARSER ) idToken( token );
				t->ClearTokenWhiteSpace();
				t->next = NULL;
				if( last )
				{
					last->next = t;
				}
				else
				{
					define->parms = t;
				}
				last = t;
				define->numparms++;
				// read next token
				if( !idParser::ReadLine( &token ) )
				{
					idParser::Error( "define parameters not terminated" );
					return false;
				}

				if( token == ")" )
				{
					break;
				}
				// then it must be a comma
				if( token != "," )
				{
					idParser::Error( "define not terminated" );
					return false;
				}
			}
		}
		if( !idParser::ReadLine( &token ) )
		{
			return true;
		}
	}
	// read the defined stuff
	last = NULL;
	do
	{
		t = new( TAG_IDLIB_PARSER ) idToken( token );
		if( t->type == TT_NAME && !strcmp( t->c_str(), define->name ) )
		{
			t->flags |= TOKEN_FL_RECURSIVE_DEFINE;
			idParser::Warning( "recursive define (removed recursion)" );
		}
		t->ClearTokenWhiteSpace();
		t->next = NULL;
		if( last )
		{
			last->next = t;
		}
		else
		{
			define->tokens = t;
		}
		last = t;
	}
	while( idParser::ReadLine( &token ) );

	if( last )
	{
		// check for merge operators at the beginning or end
		if( ( *define->tokens ) == "##" || ( *last ) == "##" )
		{
			idParser::Error( "define with misplaced ##" );
			return false;
		}
	}
	return true;
}

/*
================
idParser::AddDefine
================
*/
int idParser::AddDefine( const char* string )
{
	define_t* define;

	define = DefineFromString( string );
	if( !define )
	{
		return false;
	}
	AddDefineToHash( define, idParser::definehash );
	return true;
}

/*
================
idParser::AddGlobalDefinesToSource
================
*/
void idParser::AddGlobalDefinesToSource()
{
	define_t* define, *newdefine;

	for( define = globaldefines; define; define = define->next )
	{
		newdefine = CopyDefine( define );
		AddDefineToHash( newdefine, idParser::definehash );
	}
}

/*
================
idParser::Directive_if_def
================
*/
int idParser::Directive_if_def( int type )
{
	idToken token;
	define_t* d;
	int skip;

	if( !idParser::ReadLine( &token ) )
	{
		idParser::Error( "#ifdef without name" );
		return false;
	}
	if( token.type != TT_NAME )
	{
		idParser::UnreadSourceToken( &token );
		idParser::Error( "expected name after #ifdef, found '%s'", token.c_str() );
		return false;
	}
	d = FindHashedDefine( idParser::definehash, token.c_str() );
	skip = ( type == INDENT_IFDEF ) == ( d == NULL );
	idParser::PushIndent( type, skip );
	return true;
}

/*
================
idParser::Directive_ifdef
================
*/
int idParser::Directive_ifdef()
{
	return idParser::Directive_if_def( INDENT_IFDEF );
}

/*
================
idParser::Directive_ifndef
================
*/
int idParser::Directive_ifndef()
{
	return idParser::Directive_if_def( INDENT_IFNDEF );
}

/*
================
idParser::Directive_else
================
*/
int idParser::Directive_else()
{
	int type, skip;

	idParser::PopIndent( &type, &skip );
	if( !type )
	{
		idParser::Error( "misplaced #else" );
		return false;
	}
	if( type == INDENT_ELSE )
	{
		idParser::Error( "#else after #else" );
		return false;
	}
	idParser::PushIndent( INDENT_ELSE, !skip );
	return true;
}

/*
================
idParser::Directive_endif
================
*/
int idParser::Directive_endif()
{
	int type, skip;

	idParser::PopIndent( &type, &skip );
	if( !type )
	{
		idParser::Error( "misplaced #endif" );
		return false;
	}
	return true;
}

/*
================
idParser::EvaluateTokens
================
*/
typedef struct operator_s
{
	int op;
	int priority;
	int parentheses;
	struct operator_s* prev, *next;
} operator_t;

typedef struct value_s
{
	signed int intvalue; // DG: use int instead of long for 64bit compatibility
	double floatvalue;
	int parentheses;
	struct value_s* prev, *next;
} value_t;

int PC_OperatorPriority( int op )
{
	switch( op )
	{
		case P_MUL:
			return 15;
		case P_DIV:
			return 15;
		case P_MOD:
			return 15;
		case P_ADD:
			return 14;
		case P_SUB:
			return 14;

		case P_LOGIC_AND:
			return 7;
		case P_LOGIC_OR:
			return 6;
		case P_LOGIC_GEQ:
			return 12;
		case P_LOGIC_LEQ:
			return 12;
		case P_LOGIC_EQ:
			return 11;
		case P_LOGIC_UNEQ:
			return 11;

		case P_LOGIC_NOT:
			return 16;
		case P_LOGIC_GREATER:
			return 12;
		case P_LOGIC_LESS:
			return 12;

		case P_RSHIFT:
			return 13;
		case P_LSHIFT:
			return 13;

		case P_BIN_AND:
			return 10;
		case P_BIN_OR:
			return 8;
		case P_BIN_XOR:
			return 9;
		case P_BIN_NOT:
			return 16;

		case P_COLON:
			return 5;
		case P_QUESTIONMARK:
			return 5;
	}
	return false;
}

//#define AllocValue()			GetClearedMemory(sizeof(value_t));
//#define FreeValue(val)		FreeMemory(val)
//#define AllocOperator(op)		op = (operator_t *) GetClearedMemory(sizeof(operator_t));
//#define FreeOperator(op)		FreeMemory(op);

#define MAX_VALUES		64
#define MAX_OPERATORS	64

#define AllocValue(val)									\
	if ( numvalues >= MAX_VALUES ) {					\
		idParser::Error( "out of value space\n" );		\
		error = 1;										\
		break;											\
	}													\
	else {												\
		val = &value_heap[numvalues++];					\
	}

#define FreeValue(val)

#define AllocOperator(op)								\
	if ( numoperators >= MAX_OPERATORS ) {				\
		idParser::Error( "out of operator space\n" );	\
		error = 1;										\
		break;											\
	}													\
	else {												\
		op = &operator_heap[numoperators++];			\
	}

#define FreeOperator(op)

int idParser::EvaluateTokens( idToken* tokens, signed int* intvalue, double* floatvalue, int integer )
{
	operator_t* o, *firstoperator, *lastoperator;
	value_t* v, *firstvalue, *lastvalue, *v1, *v2;
	idToken* t;
	int brace = 0;
	int parentheses = 0;
	int error = 0;
	int lastwasvalue = 0;
	int negativevalue = 0;
	int questmarkintvalue = 0;
	double questmarkfloatvalue = 0;
	int gotquestmarkvalue = false;
	int lastoperatortype = 0;
	//
	operator_t operator_heap[MAX_OPERATORS];
	int numoperators = 0;
	value_t value_heap[MAX_VALUES];
	int numvalues = 0;

	firstoperator = lastoperator = NULL;
	firstvalue = lastvalue = NULL;
	if( intvalue )
	{
		*intvalue = 0;
	}
	if( floatvalue )
	{
		*floatvalue = 0;
	}
	for( t = tokens; t; t = t->next )
	{
		switch( t->type )
		{
			case TT_NAME:
			{
				if( lastwasvalue || negativevalue )
				{
					idParser::Error( "syntax error in #if/#elif" );
					error = 1;
					break;
				}
				if( ( *t ) != "defined" )
				{
					idParser::Error( "undefined name '%s' in #if/#elif", t->c_str() );
					error = 1;
					break;
				}
				t = t->next;
				if( ( *t ) == "(" )
				{
					brace = true;
					t = t->next;
				}
				if( !t || t->type != TT_NAME )
				{
					idParser::Error( "defined() without name in #if/#elif" );
					error = 1;
					break;
				}
				//v = (value_t *) GetClearedMemory(sizeof(value_t));
				AllocValue( v );
				if( FindHashedDefine( idParser::definehash, t->c_str() ) )
				{
					v->intvalue = 1;
					v->floatvalue = 1;
				}
				else
				{
					v->intvalue = 0;
					v->floatvalue = 0;
				}
				v->parentheses = parentheses;
				v->next = NULL;
				v->prev = lastvalue;
				if( lastvalue )
				{
					lastvalue->next = v;
				}
				else
				{
					firstvalue = v;
				}
				lastvalue = v;
				if( brace )
				{
					t = t->next;
					if( !t || ( *t ) != ")" )
					{
						idParser::Error( "defined missing ) in #if/#elif" );
						error = 1;
						break;
					}
				}
				brace = false;
				// defined() creates a value
				lastwasvalue = 1;
				break;
			}
			case TT_NUMBER:
			{
				if( lastwasvalue )
				{
					idParser::Error( "syntax error in #if/#elif" );
					error = 1;
					break;
				}
				//v = (value_t *) GetClearedMemory(sizeof(value_t));
				AllocValue( v );
				if( negativevalue )
				{
					v->intvalue = - t->GetIntValue();
					v->floatvalue = - t->GetFloatValue();
				}
				else
				{
					v->intvalue = t->GetIntValue();
					v->floatvalue = t->GetFloatValue();
				}
				v->parentheses = parentheses;
				v->next = NULL;
				v->prev = lastvalue;
				if( lastvalue )
				{
					lastvalue->next = v;
				}
				else
				{
					firstvalue = v;
				}
				lastvalue = v;
				//last token was a value
				lastwasvalue = 1;
				//
				negativevalue = 0;
				break;
			}
			case TT_PUNCTUATION:
			{
				if( negativevalue )
				{
					idParser::Error( "misplaced minus sign in #if/#elif" );
					error = 1;
					break;
				}
				if( t->subtype == P_PARENTHESESOPEN )
				{
					parentheses++;
					break;
				}
				else if( t->subtype == P_PARENTHESESCLOSE )
				{
					parentheses--;
					if( parentheses < 0 )
					{
						idParser::Error( "too many ) in #if/#elsif" );
						error = 1;
					}
					break;
				}
				//check for invalid operators on floating point values
				if( !integer )
				{
					if( t->subtype == P_BIN_NOT || t->subtype == P_MOD ||
							t->subtype == P_RSHIFT || t->subtype == P_LSHIFT ||
							t->subtype == P_BIN_AND || t->subtype == P_BIN_OR ||
							t->subtype == P_BIN_XOR )
					{
						idParser::Error( "illigal operator '%s' on floating point operands\n", t->c_str() );
						error = 1;
						break;
					}
				}
				switch( t->subtype )
				{
					case P_LOGIC_NOT:
					case P_BIN_NOT:
					{
						if( lastwasvalue )
						{
							idParser::Error( "! or ~ after value in #if/#elif" );
							error = 1;
							break;
						}
						break;
					}
					case P_INC:
					case P_DEC:
					{
						idParser::Error( "++ or -- used in #if/#elif" );
						break;
					}
					case P_SUB:
					{
						if( !lastwasvalue )
						{
							negativevalue = 1;
							break;
						}
					}

					case P_MUL:
					case P_DIV:
					case P_MOD:
					case P_ADD:

					case P_LOGIC_AND:
					case P_LOGIC_OR:
					case P_LOGIC_GEQ:
					case P_LOGIC_LEQ:
					case P_LOGIC_EQ:
					case P_LOGIC_UNEQ:

					case P_LOGIC_GREATER:
					case P_LOGIC_LESS:

					case P_RSHIFT:
					case P_LSHIFT:

					case P_BIN_AND:
					case P_BIN_OR:
					case P_BIN_XOR:

					case P_COLON:
					case P_QUESTIONMARK:
					{
						if( !lastwasvalue )
						{
							idParser::Error( "operator '%s' after operator in #if/#elif", t->c_str() );
							error = 1;
							break;
						}
						break;
					}
					default:
					{
						idParser::Error( "invalid operator '%s' in #if/#elif", t->c_str() );
						error = 1;
						break;
					}
				}
				if( !error && !negativevalue )
				{
					//o = (operator_t *) GetClearedMemory(sizeof(operator_t));
					AllocOperator( o );
					o->op = t->subtype;
					o->priority = PC_OperatorPriority( t->subtype );
					o->parentheses = parentheses;
					o->next = NULL;
					o->prev = lastoperator;
					if( lastoperator )
					{
						lastoperator->next = o;
					}
					else
					{
						firstoperator = o;
					}
					lastoperator = o;
					lastwasvalue = 0;
				}
				break;
			}
			default:
			{
				idParser::Error( "unknown '%s' in #if/#elif", t->c_str() );
				error = 1;
				break;
			}
		}
		if( error )
		{
			break;
		}
	}
	if( !error )
	{
		if( !lastwasvalue )
		{
			idParser::Error( "trailing operator in #if/#elif" );
			error = 1;
		}
		else if( parentheses )
		{
			idParser::Error( "too many ( in #if/#elif" );
			error = 1;
		}
	}
	//
	gotquestmarkvalue = false;
	questmarkintvalue = 0;
	questmarkfloatvalue = 0;
	//while there are operators
	while( !error && firstoperator )
	{
		v = firstvalue;
		for( o = firstoperator; o->next; o = o->next )
		{
			//if the current operator is nested deeper in parentheses
			//than the next operator
			if( o->parentheses > o->next->parentheses )
			{
				break;
			}
			//if the current and next operator are nested equally deep in parentheses
			if( o->parentheses == o->next->parentheses )
			{
				//if the priority of the current operator is equal or higher
				//than the priority of the next operator
				if( o->priority >= o->next->priority )
				{
					break;
				}
			}
			//if the arity of the operator isn't equal to 1
			if( o->op != P_LOGIC_NOT && o->op != P_BIN_NOT )
			{
				v = v->next;
			}
			//if there's no value or no next value
			if( !v )
			{
				idParser::Error( "mising values in #if/#elif" );
				error = 1;
				break;
			}
		}
		if( error )
		{
			break;
		}
		v1 = v;
		v2 = v->next;
#ifdef DEBUG_EVAL
		if( integer )
		{
			Log_Write( "operator %s, value1 = %d", idParser::scriptstack->getPunctuationFromId( o->op ), v1->intvalue );
			if( v2 )
			{
				Log_Write( "value2 = %d", v2->intvalue );
			}
		}
		else
		{
			Log_Write( "operator %s, value1 = %f", idParser::scriptstack->getPunctuationFromId( o->op ), v1->floatvalue );
			if( v2 )
			{
				Log_Write( "value2 = %f", v2->floatvalue );
			}
		}
#endif //DEBUG_EVAL
		switch( o->op )
		{
			case P_LOGIC_NOT:
				v1->intvalue = !v1->intvalue;
				v1->floatvalue = !v1->floatvalue;
				break;
			case P_BIN_NOT:
				v1->intvalue = ~v1->intvalue;
				break;
			case P_MUL:
				v1->intvalue *= v2->intvalue;
				v1->floatvalue *= v2->floatvalue;
				break;
			case P_DIV:
				if( !v2->intvalue || !v2->floatvalue )
				{
					idParser::Error( "divide by zero in #if/#elif\n" );
					error = 1;
					break;
				}
				v1->intvalue /= v2->intvalue;
				v1->floatvalue /= v2->floatvalue;
				break;
			case P_MOD:
				if( !v2->intvalue )
				{
					idParser::Error( "divide by zero in #if/#elif\n" );
					error = 1;
					break;
				}
				v1->intvalue %= v2->intvalue;
				break;
			case P_ADD:
				v1->intvalue += v2->intvalue;
				v1->floatvalue += v2->floatvalue;
				break;
			case P_SUB:
				v1->intvalue -= v2->intvalue;
				v1->floatvalue -= v2->floatvalue;
				break;
			case P_LOGIC_AND:
				v1->intvalue = v1->intvalue && v2->intvalue;
				v1->floatvalue = v1->floatvalue && v2->floatvalue;
				break;
			case P_LOGIC_OR:
				v1->intvalue = v1->intvalue || v2->intvalue;
				v1->floatvalue = v1->floatvalue || v2->floatvalue;
				break;
			case P_LOGIC_GEQ:
				v1->intvalue = v1->intvalue >= v2->intvalue;
				v1->floatvalue = v1->floatvalue >= v2->floatvalue;
				break;
			case P_LOGIC_LEQ:
				v1->intvalue = v1->intvalue <= v2->intvalue;
				v1->floatvalue = v1->floatvalue <= v2->floatvalue;
				break;
			case P_LOGIC_EQ:
				v1->intvalue = v1->intvalue == v2->intvalue;
				v1->floatvalue = v1->floatvalue == v2->floatvalue;
				break;
			case P_LOGIC_UNEQ:
				v1->intvalue = v1->intvalue != v2->intvalue;
				v1->floatvalue = v1->floatvalue != v2->floatvalue;
				break;
			case P_LOGIC_GREATER:
				v1->intvalue = v1->intvalue > v2->intvalue;
				v1->floatvalue = v1->floatvalue > v2->floatvalue;
				break;
			case P_LOGIC_LESS:
				v1->intvalue = v1->intvalue < v2->intvalue;
				v1->floatvalue = v1->floatvalue < v2->floatvalue;
				break;
			case P_RSHIFT:
				v1->intvalue >>= v2->intvalue;
				break;
			case P_LSHIFT:
				v1->intvalue <<= v2->intvalue;
				break;
			case P_BIN_AND:
				v1->intvalue &= v2->intvalue;
				break;
			case P_BIN_OR:
				v1->intvalue |= v2->intvalue;
				break;
			case P_BIN_XOR:
				v1->intvalue ^= v2->intvalue;
				break;
			case P_COLON:
			{
				if( !gotquestmarkvalue )
				{
					idParser::Error( ": without ? in #if/#elif" );
					error = 1;
					break;
				}
				if( integer )
				{
					if( !questmarkintvalue )
					{
						v1->intvalue = v2->intvalue;
					}
				}
				else
				{
					if( !questmarkfloatvalue )
					{
						v1->floatvalue = v2->floatvalue;
					}
				}
				gotquestmarkvalue = false;
				break;
			}
			case P_QUESTIONMARK:
			{
				if( gotquestmarkvalue )
				{
					idParser::Error( "? after ? in #if/#elif" );
					error = 1;
					break;
				}
				questmarkintvalue = v1->intvalue;
				questmarkfloatvalue = v1->floatvalue;
				gotquestmarkvalue = true;
				break;
			}
		}
#ifdef DEBUG_EVAL
		if( integer )
		{
			Log_Write( "result value = %d", v1->intvalue );
		}
		else
		{
			Log_Write( "result value = %f", v1->floatvalue );
		}
#endif //DEBUG_EVAL
		if( error )
		{
			break;
		}
		lastoperatortype = o->op;
		//if not an operator with arity 1
		if( o->op != P_LOGIC_NOT && o->op != P_BIN_NOT )
		{
			//remove the second value if not question mark operator
			if( o->op != P_QUESTIONMARK )
			{
				v = v->next;
			}
			//
			if( v->prev )
			{
				v->prev->next = v->next;
			}
			else
			{
				firstvalue = v->next;
			}
			if( v->next )
			{
				v->next->prev = v->prev;
			}
			else
			{
				lastvalue = v->prev;
			}
			//FreeMemory(v);
			FreeValue( v );
		}
		//remove the operator
		if( o->prev )
		{
			o->prev->next = o->next;
		}
		else
		{
			firstoperator = o->next;
		}
		if( o->next )
		{
			o->next->prev = o->prev;
		}
		else
		{
			lastoperator = o->prev;
		}
		//FreeMemory(o);
		FreeOperator( o );
	}
	if( firstvalue )
	{
		if( intvalue )
		{
			*intvalue = firstvalue->intvalue;
		}
		if( floatvalue )
		{
			*floatvalue = firstvalue->floatvalue;
		}
	}
	for( o = firstoperator; o; o = lastoperator )
	{
		lastoperator = o->next;
		//FreeMemory(o);
		FreeOperator( o );
	}
	for( v = firstvalue; v; v = lastvalue )
	{
		lastvalue = v->next;
		//FreeMemory(v);
		FreeValue( v );
	}
	if( !error )
	{
		return true;
	}
	if( intvalue )
	{
		*intvalue = 0;
	}
	if( floatvalue )
	{
		*floatvalue = 0;
	}
	return false;
}

/*
================
idParser::Evaluate
================
*/
int idParser::Evaluate( signed int* intvalue, double* floatvalue, int integer )
{
	idToken token, *firsttoken, *lasttoken;
	idToken* t, *nexttoken;
	define_t* define;
	int defined = false;

	if( intvalue )
	{
		*intvalue = 0;
	}
	if( floatvalue )
	{
		*floatvalue = 0;
	}
	//
	if( !idParser::ReadLine( &token ) )
	{
		idParser::Error( "no value after #if/#elif" );
		return false;
	}
	firsttoken = NULL;
	lasttoken = NULL;
	do
	{
		//if the token is a name
		if( token.type == TT_NAME )
		{
			if( defined )
			{
				defined = false;
				t = new( TAG_IDLIB_PARSER ) idToken( token );
				t->next = NULL;
				if( lasttoken )
				{
					lasttoken->next = t;
				}
				else
				{
					firsttoken = t;
				}
				lasttoken = t;
			}
			else if( token == "defined" )
			{
				defined = true;
				t = new( TAG_IDLIB_PARSER ) idToken( token );
				t->next = NULL;
				if( lasttoken )
				{
					lasttoken->next = t;
				}
				else
				{
					firsttoken = t;
				}
				lasttoken = t;
			}
			else
			{
				//then it must be a define
				define = FindHashedDefine( idParser::definehash, token.c_str() );
				if( !define )
				{
					idParser::Error( "can't Evaluate '%s', not defined", token.c_str() );
					return false;
				}
				if( !idParser::ExpandDefineIntoSource( &token, define ) )
				{
					return false;
				}
			}
		}
		//if the token is a number or a punctuation
		else if( token.type == TT_NUMBER || token.type == TT_PUNCTUATION )
		{
			t = new( TAG_IDLIB_PARSER ) idToken( token );
			t->next = NULL;
			if( lasttoken )
			{
				lasttoken->next = t;
			}
			else
			{
				firsttoken = t;
			}
			lasttoken = t;
		}
		else
		{
			idParser::Error( "can't Evaluate '%s'", token.c_str() );
			return false;
		}
	}
	while( idParser::ReadLine( &token ) );
	//
	if( !idParser::EvaluateTokens( firsttoken, intvalue, floatvalue, integer ) )
	{
		return false;
	}
	//
#ifdef DEBUG_EVAL
	Log_Write( "eval:" );
#endif //DEBUG_EVAL
	for( t = firsttoken; t; t = nexttoken )
	{
#ifdef DEBUG_EVAL
		Log_Write( " %s", t->c_str() );
#endif //DEBUG_EVAL
		nexttoken = t->next;
		delete t;
	} //end for
#ifdef DEBUG_EVAL
	if( integer )
	{
		Log_Write( "eval result: %d", *intvalue );
	}
	else
	{
		Log_Write( "eval result: %f", *floatvalue );
	}
#endif //DEBUG_EVAL
	//
	return true;
}

/*
================
idParser::DollarEvaluate
================
*/
int idParser::DollarEvaluate( signed int* intvalue, double* floatvalue, int integer )
{
	int indent, defined = false;
	idToken token, *firsttoken, *lasttoken;
	idToken* t, *nexttoken;
	define_t* define;

	if( intvalue )
	{
		*intvalue = 0;
	}
	if( floatvalue )
	{
		*floatvalue = 0;
	}
	//
	if( !idParser::ReadSourceToken( &token ) )
	{
		idParser::Error( "no leading ( after $evalint/$evalfloat" );
		return false;
	}
	if( !idParser::ReadSourceToken( &token ) )
	{
		idParser::Error( "nothing to Evaluate" );
		return false;
	}
	indent = 1;
	firsttoken = NULL;
	lasttoken = NULL;
	do
	{
		//if the token is a name
		if( token.type == TT_NAME )
		{
			if( defined )
			{
				defined = false;
				t = new( TAG_IDLIB_PARSER ) idToken( token );
				t->next = NULL;
				if( lasttoken )
				{
					lasttoken->next = t;
				}
				else
				{
					firsttoken = t;
				}
				lasttoken = t;
			}
			else if( token == "defined" )
			{
				defined = true;
				t = new( TAG_IDLIB_PARSER ) idToken( token );
				t->next = NULL;
				if( lasttoken )
				{
					lasttoken->next = t;
				}
				else
				{
					firsttoken = t;
				}
				lasttoken = t;
			}
			else
			{
				//then it must be a define
				define = FindHashedDefine( idParser::definehash, token.c_str() );
				if( !define )
				{
					idParser::Warning( "can't Evaluate '%s', not defined", token.c_str() );
					return false;
				}
				if( !idParser::ExpandDefineIntoSource( &token, define ) )
				{
					return false;
				}
			}
		}
		//if the token is a number or a punctuation
		else if( token.type == TT_NUMBER || token.type == TT_PUNCTUATION )
		{
			if( token[0] == '(' )
			{
				indent++;
			}
			else if( token[0] == ')' )
			{
				indent--;
			}
			if( indent <= 0 )
			{
				break;
			}
			t = new( TAG_IDLIB_PARSER ) idToken( token );
			t->next = NULL;
			if( lasttoken )
			{
				lasttoken->next = t;
			}
			else
			{
				firsttoken = t;
			}
			lasttoken = t;
		}
		else
		{
			idParser::Error( "can't Evaluate '%s'", token.c_str() );
			return false;
		}
	}
	while( idParser::ReadSourceToken( &token ) );
	//
	if( !idParser::EvaluateTokens( firsttoken, intvalue, floatvalue, integer ) )
	{
		return false;
	}
	//
#ifdef DEBUG_EVAL
	Log_Write( "$eval:" );
#endif //DEBUG_EVAL
	for( t = firsttoken; t; t = nexttoken )
	{
#ifdef DEBUG_EVAL
		Log_Write( " %s", t->c_str() );
#endif //DEBUG_EVAL
		nexttoken = t->next;
		delete t;
	} //end for
#ifdef DEBUG_EVAL
	if( integer )
	{
		Log_Write( "$eval result: %d", *intvalue );
	}
	else
	{
		Log_Write( "$eval result: %f", *floatvalue );
	}
#endif //DEBUG_EVAL
	//
	return true;
}

/*
================
idParser::Directive_elif
================
*/
int idParser::Directive_elif()
{
	signed int value; // DG: use int instead of long for 64bit compatibility
	int type, skip;

	idParser::PopIndent( &type, &skip );
	if( !type || type == INDENT_ELSE )
	{
		idParser::Error( "misplaced #elif" );
		return false;
	}
	if( !idParser::Evaluate( &value, NULL, true ) )
	{
		return false;
	}
	skip = ( value == 0 );
	idParser::PushIndent( INDENT_ELIF, skip );
	return true;
}

/*
================
idParser::Directive_if
================
*/
int idParser::Directive_if()
{
	signed int value; // DG: use int instead of long for 64bit compatibility
	int skip;

	if( !idParser::Evaluate( &value, NULL, true ) )
	{
		return false;
	}
	skip = ( value == 0 );
	idParser::PushIndent( INDENT_IF, skip );
	return true;
}

/*
================
idParser::Directive_line
================
*/
int idParser::Directive_line()
{
	idToken token;

	idParser::Error( "#line directive not supported" );
	while( idParser::ReadLine( &token ) )
	{
	}
	return true;
}

/*
================
idParser::Directive_error
================
*/
int idParser::Directive_error()
{
	idToken token;

	if( !idParser::ReadLine( &token ) || token.type != TT_STRING )
	{
		idParser::Error( "#error without string" );
		return false;
	}
	idParser::Error( "#error: %s", token.c_str() );
	return true;
}

/*
================
idParser::Directive_warning
================
*/
int idParser::Directive_warning()
{
	idToken token;

	if( !idParser::ReadLine( &token ) || token.type != TT_STRING )
	{
		idParser::Warning( "#warning without string" );
		return false;
	}
	idParser::Warning( "#warning: %s", token.c_str() );
	return true;
}

/*
================
idParser::Directive_pragma
================
*/
int idParser::Directive_pragma()
{
	idToken token;

	idParser::Warning( "#pragma directive not supported" );
	while( idParser::ReadLine( &token ) )
	{
	}
	return true;
}

/*
================
idParser::UnreadSignToken
================
*/
void idParser::UnreadSignToken()
{
	idToken token;

	token.line = idParser::scriptstack->GetLineNum();
	token.whiteSpaceStart_p = NULL;
	token.whiteSpaceEnd_p = NULL;
	token.linesCrossed = 0;
	token.flags = 0;
	token = "-";
	token.type = TT_PUNCTUATION;
	token.subtype = P_SUB;
	idParser::UnreadSourceToken( &token );
}

/*
================
idParser::Directive_eval
================
*/
int idParser::Directive_eval()
{
	signed int value; // DG: use int instead of long for 64bit compatibility
	idToken token;
	char buf[128];

	if( !idParser::Evaluate( &value, NULL, true ) )
	{
		return false;
	}

	token.line = idParser::scriptstack->GetLineNum();
	token.whiteSpaceStart_p = NULL;
	token.whiteSpaceEnd_p = NULL;
	token.linesCrossed = 0;
	token.flags = 0;
	sprintf( buf, "%d", abs( value ) );
	token = buf;
	token.type = TT_NUMBER;
	token.subtype = TT_INTEGER | TT_LONG | TT_DECIMAL;
	idParser::UnreadSourceToken( &token );
	if( value < 0 )
	{
		idParser::UnreadSignToken();
	}
	return true;
}

/*
================
idParser::Directive_evalfloat
================
*/
int idParser::Directive_evalfloat()
{
	double value;
	idToken token;
	char buf[128];

	if( !idParser::Evaluate( NULL, &value, false ) )
	{
		return false;
	}

	token.line = idParser::scriptstack->GetLineNum();
	token.whiteSpaceStart_p = NULL;
	token.whiteSpaceEnd_p = NULL;
	token.linesCrossed = 0;
	token.flags = 0;
	sprintf( buf, "%1.2f", idMath::Fabs( value ) );
	token = buf;
	token.type = TT_NUMBER;
	token.subtype = TT_FLOAT | TT_LONG | TT_DECIMAL;
	idParser::UnreadSourceToken( &token );
	if( value < 0 )
	{
		idParser::UnreadSignToken();
	}
	return true;
}

/*
================
idParser::ReadDirective
================
*/
int idParser::ReadDirective()
{
	idToken token;

	//read the directive name
	if( !idParser::ReadSourceToken( &token ) )
	{
		idParser::Error( "found '#' without name" );
		return false;
	}
	//directive name must be on the same line
	if( token.linesCrossed > 0 )
	{
		idParser::UnreadSourceToken( &token );
		idParser::Error( "found '#' at end of line" );
		return false;
	}
	//if if is a name
	if( token.type == TT_NAME )
	{
		if( token == "if" )
		{
			return idParser::Directive_if();
		}
		else if( token == "ifdef" )
		{
			return idParser::Directive_ifdef();
		}
		else if( token == "ifndef" )
		{
			return idParser::Directive_ifndef();
		}
		else if( token == "elif" )
		{
			return idParser::Directive_elif();
		}
		else if( token == "else" )
		{
			return idParser::Directive_else();
		}
		else if( token == "endif" )
		{
			return idParser::Directive_endif();
		}
		else if( idParser::skip > 0 )
		{
			// skip the rest of the line
			while( idParser::ReadLine( &token ) )
			{
			}
			return true;
		}
		else
		{
			if( token == "include" )
			{
				// RB lets override for embedded shaders
				idToken filename;
				return Directive_include( &filename );
				// RB end
			}
			else if( token == "define" )
			{
				return idParser::Directive_define();
			}
			else if( token == "undef" )
			{
				return idParser::Directive_undef();
			}
			else if( token == "line" )
			{
				return idParser::Directive_line();
			}
			else if( token == "error" )
			{
				return idParser::Directive_error();
			}
			else if( token == "warning" )
			{
				return idParser::Directive_warning();
			}
			else if( token == "pragma" )
			{
				return idParser::Directive_pragma();
			}
			else if( token == "eval" )
			{
				return idParser::Directive_eval();
			}
			else if( token == "evalfloat" )
			{
				return idParser::Directive_evalfloat();
			}
		}
	}
	idParser::Error( "unknown precompiler directive '%s'", token.c_str() );
	return false;
}

/*
================
idParser::DollarDirective_evalint
================
*/
int idParser::DollarDirective_evalint()
{
	signed int value; // DG: use int instead of long for 64bit compatibility
	idToken token;
	char buf[128];

	if( !idParser::DollarEvaluate( &value, NULL, true ) )
	{
		return false;
	}

	token.line = idParser::scriptstack->GetLineNum();
	token.whiteSpaceStart_p = NULL;
	token.whiteSpaceEnd_p = NULL;
	token.linesCrossed = 0;
	token.flags = 0;
	sprintf( buf, "%d", abs( value ) );
	token = buf;
	token.type = TT_NUMBER;
	token.subtype = TT_INTEGER | TT_LONG | TT_DECIMAL | TT_VALUESVALID;
	token.intvalue = abs( value );
	token.floatvalue = abs( value );
	idParser::UnreadSourceToken( &token );
	if( value < 0 )
	{
		idParser::UnreadSignToken();
	}
	return true;
}

/*
================
idParser::DollarDirective_evalfloat
================
*/
int idParser::DollarDirective_evalfloat()
{
	double value;
	idToken token;
	char buf[128];

	if( !idParser::DollarEvaluate( NULL, &value, false ) )
	{
		return false;
	}

	token.line = idParser::scriptstack->GetLineNum();
	token.whiteSpaceStart_p = NULL;
	token.whiteSpaceEnd_p = NULL;
	token.linesCrossed = 0;
	token.flags = 0;
	sprintf( buf, "%1.2f", fabs( value ) );
	token = buf;
	token.type = TT_NUMBER;
	token.subtype = TT_FLOAT | TT_LONG | TT_DECIMAL | TT_VALUESVALID;
	token.intvalue = ( unsigned int ) fabs( value ); // DG: use int instead of long for 64bit compatibility
	token.floatvalue = fabs( value );
	idParser::UnreadSourceToken( &token );
	if( value < 0 )
	{
		idParser::UnreadSignToken();
	}
	return true;
}

/*
================
idParser::ReadDollarDirective
================
*/
int idParser::ReadDollarDirective()
{
	idToken token;

	// read the directive name
	if( !idParser::ReadSourceToken( &token ) )
	{
		idParser::Error( "found '$' without name" );
		return false;
	}
	// directive name must be on the same line
	if( token.linesCrossed > 0 )
	{
		idParser::UnreadSourceToken( &token );
		idParser::Error( "found '$' at end of line" );
		return false;
	}
	// if if is a name
	if( token.type == TT_NAME )
	{
		if( token == "evalint" )
		{
			return idParser::DollarDirective_evalint();
		}
		else if( token == "evalfloat" )
		{
			return idParser::DollarDirective_evalfloat();
		}
	}
	idParser::UnreadSourceToken( &token );
	return false;
}

/*
================
idParser::ReadToken
================
*/
int idParser::ReadToken( idToken* token )
{
	define_t* define;

	while( 1 )
	{
		if( !idParser::ReadSourceToken( token ) )
		{
			return false;
		}
		// check for precompiler directives
		if( token->type == TT_PUNCTUATION && ( *token )[0] == '#' && ( *token )[1] == '\0' )
		{
			// read the precompiler directive
			if( !idParser::ReadDirective() )
			{
				return false;
			}
			continue;
		}
		// if skipping source because of conditional compilation
		if( idParser::skip )
		{
			continue;
		}
		// recursively concatenate strings that are behind each other still resolving defines
		if( token->type == TT_STRING && !( idParser::scriptstack->GetFlags() & LEXFL_NOSTRINGCONCAT ) )
		{
			idToken newtoken;
			if( idParser::ReadToken( &newtoken ) )
			{
				if( newtoken.type == TT_STRING )
				{
					token->Append( newtoken.c_str() );
				}
				else
				{
					idParser::UnreadSourceToken( &newtoken );
				}
			}
		}
		//
		if( !( idParser::scriptstack->GetFlags() & LEXFL_NODOLLARPRECOMPILE ) )
		{
			// check for special precompiler directives
			if( token->type == TT_PUNCTUATION && ( *token )[0] == '$' && ( *token )[1] == '\0' )
			{
				// read the precompiler directive
				if( idParser::ReadDollarDirective() )
				{
					continue;
				}
			}
		}
		// if the token is a name
		if( token->type == TT_NAME && !( token->flags & TOKEN_FL_RECURSIVE_DEFINE ) )
		{
			// check if the name is a define macro
			define = FindHashedDefine( idParser::definehash, token->c_str() );
			// if it is a define macro
			if( define )
			{
				// expand the defined macro
				if( !idParser::ExpandDefineIntoSource( token, define ) )
				{
					return false;
				}
				continue;
			}
		}
		// found a token
		return true;
	}
}

/*
================
idParser::ExpectTokenString
================
*/
int idParser::ExpectTokenString( const char* string )
{
	idToken token;

	if( !idParser::ReadToken( &token ) )
	{
		idParser::Error( "couldn't find expected '%s'", string );
		return false;
	}

	if( token != string )
	{
		idParser::Error( "expected '%s' but found '%s'", string, token.c_str() );
		return false;
	}
	return true;
}

/*
================
idParser::ExpectTokenType
================
*/
int idParser::ExpectTokenType( int type, int subtype, idToken* token )
{
	idStr str;

	if( !idParser::ReadToken( token ) )
	{
		idParser::Error( "couldn't read expected token" );
		return 0;
	}

	if( token->type != type )
	{
		switch( type )
		{
			case TT_STRING:
				str = "string";
				break;
			case TT_LITERAL:
				str = "literal";
				break;
			case TT_NUMBER:
				str = "number";
				break;
			case TT_NAME:
				str = "name";
				break;
			case TT_PUNCTUATION:
				str = "punctuation";
				break;
			default:
				str = "unknown type";
				break;
		}
		idParser::Error( "expected a %s but found '%s'", str.c_str(), token->c_str() );
		return 0;
	}
	if( token->type == TT_NUMBER )
	{
		if( ( token->subtype & subtype ) != subtype )
		{
			str.Clear();
			if( subtype & TT_DECIMAL )
			{
				str = "decimal ";
			}
			if( subtype & TT_HEX )
			{
				str = "hex ";
			}
			if( subtype & TT_OCTAL )
			{
				str = "octal ";
			}
			if( subtype & TT_BINARY )
			{
				str = "binary ";
			}
			if( subtype & TT_UNSIGNED )
			{
				str += "unsigned ";
			}
			if( subtype & TT_LONG )
			{
				str += "long ";
			}
			if( subtype & TT_FLOAT )
			{
				str += "float ";
			}
			if( subtype & TT_INTEGER )
			{
				str += "integer ";
			}
			str.StripTrailing( ' ' );
			idParser::Error( "expected %s but found '%s'", str.c_str(), token->c_str() );
			return 0;
		}
	}
	else if( token->type == TT_PUNCTUATION )
	{
		if( subtype < 0 )
		{
			idParser::Error( "BUG: wrong punctuation subtype" );
			return 0;
		}
		if( token->subtype != subtype )
		{
			idParser::Error( "expected '%s' but found '%s'", scriptstack->GetPunctuationFromId( subtype ), token->c_str() );
			return 0;
		}
	}
	return 1;
}

/*
================
idParser::ExpectAnyToken
================
*/
int idParser::ExpectAnyToken( idToken* token )
{
	if( !idParser::ReadToken( token ) )
	{
		idParser::Error( "couldn't read expected token" );
		return false;
	}
	else
	{
		return true;
	}
}

/*
================
idParser::CheckTokenString
================
*/
int idParser::CheckTokenString( const char* string )
{
	idToken tok;

	if( !ReadToken( &tok ) )
	{
		return false;
	}
	//if the token is available
	if( tok == string )
	{
		return true;
	}

	UnreadSourceToken( &tok );
	return false;
}

/*
================
idParser::CheckTokenType
================
*/
int idParser::CheckTokenType( int type, int subtype, idToken* token )
{
	idToken tok;

	if( !ReadToken( &tok ) )
	{
		return false;
	}
	//if the type matches
	if( tok.type == type && ( tok.subtype & subtype ) == subtype )
	{
		*token = tok;
		return true;
	}

	UnreadSourceToken( &tok );
	return false;
}

/*
================
idParser::PeekTokenString
================
*/
int idParser::PeekTokenString( const char* string )
{
	idToken tok;

	if( !ReadToken( &tok ) )
	{
		return false;
	}

	UnreadSourceToken( &tok );

	// if the token is available
	if( tok == string )
	{
		return true;
	}
	return false;
}

/*
================
idParser::PeekTokenType
================
*/
int idParser::PeekTokenType( int type, int subtype, idToken* token )
{
	idToken tok;

	if( !ReadToken( &tok ) )
	{
		return false;
	}

	UnreadSourceToken( &tok );

	// if the type matches
	if( tok.type == type && ( tok.subtype & subtype ) == subtype )
	{
		*token = tok;
		return true;
	}
	return false;
}

/*
================
idParser::SkipUntilString
================
*/
int idParser::SkipUntilString( const char* string )
{
	idToken token;

	while( idParser::ReadToken( &token ) )
	{
		if( token == string )
		{
			return true;
		}
	}
	return false;
}

/*
================
idParser::SkipRestOfLine
================
*/
int idParser::SkipRestOfLine()
{
	idToken token;

	while( idParser::ReadToken( &token ) )
	{
		if( token.linesCrossed )
		{
			idParser::UnreadSourceToken( &token );
			return true;
		}
	}
	return false;
}

/*
=================
idParser::SkipBracedSection

Skips until a matching close brace is found.
Internal brace depths are properly skipped.
=================
*/
int idParser::SkipBracedSection( bool parseFirstBrace )
{
	idToken token;
	int depth;

	depth = parseFirstBrace ? 0 : 1;
	do
	{
		if( !ReadToken( &token ) )
		{
			return false;
		}
		if( token.type == TT_PUNCTUATION )
		{
			if( token == "{" )
			{
				depth++;
			}
			else if( token == "}" )
			{
				depth--;
			}
		}
	}
	while( depth );
	return true;
}

/*
=================
idParser::ParseBracedSectionExact

The next token should be an open brace.
Parses until a matching close brace is found.
Maintains the exact formating of the braced section

  FIXME: what about precompilation ?
=================
*/
const char* idParser::ParseBracedSectionExact( idStr& out, int tabs )
{
	return scriptstack->ParseBracedSectionExact( out, tabs );
}


/*
========================
idParser::ParseBracedSection

The next token should be an open brace. Parses until a matching close brace is found. Internal
brace depths are properly skipped.
========================
*/
const char* idParser::ParseBracedSection( idStr& out, int tabs, bool parseFirstBrace, char intro, char outro )
{
	idToken token;
	int i, depth;
	bool doTabs;

	char temp[ 2 ] = { 0, 0 };
	*temp = intro;

	out.Empty();
	if( parseFirstBrace )
	{
		if( !ExpectTokenString( temp ) )
		{
			return out.c_str();
		}
		out = temp;
	}
	depth = 1;
	doTabs = ( tabs >= 0 );
	do
	{
		if( !ReadToken( &token ) )
		{
			Error( "missing closing brace" );
			return out.c_str();
		}

		// if the token is on a new line
		for( i = 0; i < token.linesCrossed; i++ )
		{
			out += "\r\n";
		}

		if( doTabs && token.linesCrossed )
		{
			i = tabs;
			if( token[ 0 ] == outro && i > 0 )
			{
				i--;
			}
			while( i-- > 0 )
			{
				out += "\t";
			}
		}
		if( token.type == TT_STRING )
		{
			out += "\"" + token + "\"";
		}
		else if( token.type == TT_LITERAL )
		{
			out += "\'" + token + "\'";
		}
		else
		{
			if( token[ 0 ] == intro )
			{
				depth++;
				if( doTabs )
				{
					tabs++;
				}
			}
			else if( token[ 0 ] == outro )
			{
				depth--;
				if( doTabs )
				{
					tabs--;
				}
			}
			out += token;
		}
		out += " ";
	}
	while( depth );

	return out.c_str();
}

/*
=================
idParser::ParseRestOfLine

  parse the rest of the line
=================
*/
const char* idParser::ParseRestOfLine( idStr& out )
{
	idToken token;

	out.Empty();
	while( idParser::ReadToken( &token ) )
	{
		if( token.linesCrossed )
		{
			idParser::UnreadSourceToken( &token );
			break;
		}
		if( out.Length() )
		{
			out += " ";
		}
		out += token;
	}
	return out.c_str();
}

/*
================
idParser::UnreadToken
================
*/
void idParser::UnreadToken( idToken* token )
{
	idParser::UnreadSourceToken( token );
}

/*
================
idParser::ReadTokenOnLine
================
*/
int idParser::ReadTokenOnLine( idToken* token )
{
	idToken tok;

	if( !idParser::ReadToken( &tok ) )
	{
		return false;
	}
	// if no lines were crossed before this token
	if( !tok.linesCrossed )
	{
		*token = tok;
		return true;
	}
	//
	idParser::UnreadSourceToken( &tok );
	return false;
}

/*
================
idParser::ParseInt
================
*/
int idParser::ParseInt()
{
	idToken token;

	if( !idParser::ReadToken( &token ) )
	{
		idParser::Error( "couldn't read expected integer" );
		return 0;
	}
	if( token.type == TT_PUNCTUATION && token == "-" )
	{
		idParser::ExpectTokenType( TT_NUMBER, TT_INTEGER, &token );
		return -( ( signed int ) token.GetIntValue() );
	}
	else if( token.type != TT_NUMBER || token.subtype == TT_FLOAT )
	{
		idParser::Error( "expected integer value, found '%s'", token.c_str() );
	}
	return token.GetIntValue();
}

/*
================
idParser::ParseBool
================
*/
bool idParser::ParseBool()
{
	idToken token;

	if( !idParser::ExpectTokenType( TT_NUMBER, 0, &token ) )
	{
		idParser::Error( "couldn't read expected boolean" );
		return false;
	}
	return ( token.GetIntValue() != 0 );
}

/*
================
idParser::ParseFloat
================
*/
float idParser::ParseFloat()
{
	idToken token;

	if( !idParser::ReadToken( &token ) )
	{
		idParser::Error( "couldn't read expected floating point number" );
		return 0.0f;
	}
	if( token.type == TT_PUNCTUATION && token == "-" )
	{
		idParser::ExpectTokenType( TT_NUMBER, 0, &token );
		return -token.GetFloatValue();
	}
	else if( token.type != TT_NUMBER )
	{
		idParser::Error( "expected float value, found '%s'", token.c_str() );
	}
	return token.GetFloatValue();
}

/*
================
idParser::Parse1DMatrix
================
*/
int idParser::Parse1DMatrix( int x, float* m )
{
	int i;

	if( !idParser::ExpectTokenString( "(" ) )
	{
		return false;
	}

	for( i = 0; i < x; i++ )
	{
		m[i] = idParser::ParseFloat();
	}

	if( !idParser::ExpectTokenString( ")" ) )
	{
		return false;
	}
	return true;
}

/*
================
idParser::Parse2DMatrix
================
*/
int idParser::Parse2DMatrix( int y, int x, float* m )
{
	int i;

	if( !idParser::ExpectTokenString( "(" ) )
	{
		return false;
	}

	for( i = 0; i < y; i++ )
	{
		if( !idParser::Parse1DMatrix( x, m + i * x ) )
		{
			return false;
		}
	}

	if( !idParser::ExpectTokenString( ")" ) )
	{
		return false;
	}
	return true;
}

/*
================
idParser::Parse3DMatrix
================
*/
int idParser::Parse3DMatrix( int z, int y, int x, float* m )
{
	int i;

	if( !idParser::ExpectTokenString( "(" ) )
	{
		return false;
	}

	for( i = 0 ; i < z; i++ )
	{
		if( !idParser::Parse2DMatrix( y, x, m + i * x * y ) )
		{
			return false;
		}
	}

	if( !idParser::ExpectTokenString( ")" ) )
	{
		return false;
	}
	return true;
}

/*
================
idParser::GetLastWhiteSpace
================
*/
int idParser::GetLastWhiteSpace( idStr& whiteSpace ) const
{
	if( scriptstack )
	{
		scriptstack->GetLastWhiteSpace( whiteSpace );
	}
	else
	{
		whiteSpace.Clear();
	}
	return whiteSpace.Length();
}

/*
================
idParser::SetMarker
================
*/
void idParser::SetMarker()
{
	marker_p = NULL;
}

/*
================
idParser::GetStringFromMarker

  FIXME: this is very bad code, the script isn't even garrenteed to still be around
================
*/
void idParser::GetStringFromMarker( idStr& out, bool clean )
{
	char*	p;
	char	save;

	if( marker_p == NULL )
	{
		marker_p = scriptstack->buffer;
	}

	if( tokens )
	{
		p = ( char* )tokens->whiteSpaceStart_p;
	}
	else
	{
		p = ( char* )scriptstack->script_p;
	}

	// Set the end character to NULL to give us a complete string
	save = *p;
	*p = 0;

	// If cleaning then reparse
	if( clean )
	{
		idParser temp( marker_p, strlen( marker_p ), "temp", flags );
		idToken token;
		while( temp.ReadToken( &token ) )
		{
			out += token;
		}
	}
	else
	{
		out = marker_p;
	}

	// restore the character we set to NULL
	*p = save;
}

/*
================
idParser::SetIncludePath
================
*/
void idParser::SetIncludePath( const char* path )
{
	idParser::includepath = path;
	// add trailing path seperator
	if( idParser::includepath[idParser::includepath.Length() - 1] != '\\' &&
			idParser::includepath[idParser::includepath.Length() - 1] != '/' )
	{
		idParser::includepath += PATHSEPARATOR_STR;
	}
}

/*
================
idParser::SetPunctuations
================
*/
void idParser::SetPunctuations( const punctuation_t* p )
{
	idParser::punctuations = p;
}

/*
================
idParser::SetFlags
================
*/
void idParser::SetFlags( int flags )
{
	idLexer* s;

	idParser::flags = flags;
	for( s = idParser::scriptstack; s; s = s->next )
	{
		s->SetFlags( flags );
	}
}

/*
================
idParser::GetFlags
================
*/
int idParser::GetFlags() const
{
	return idParser::flags;
}

/*
================
idParser::LoadFile
================
*/
int idParser::LoadFile( const char* filename, bool OSPath )
{
	idLexer* script;

	if( idParser::loaded )
	{
		idLib::common->FatalError( "idParser::loadFile: another source already loaded" );
		return false;
	}
	script = new( TAG_IDLIB_PARSER ) idLexer( filename, 0, OSPath );
	if( !script->IsLoaded() )
	{
		delete script;
		return false;
	}
	script->SetFlags( idParser::flags );
	script->SetPunctuations( idParser::punctuations );
	script->next = NULL;
	idParser::OSPath = OSPath;
	idParser::filename = filename;
	idParser::scriptstack = script;
	idParser::tokens = NULL;
	idParser::indentstack = NULL;
	idParser::skip = 0;
	idParser::loaded = true;

	if( !idParser::definehash )
	{
		idParser::defines = NULL;
		idParser::definehash = ( define_t** ) Mem_ClearedAlloc( DEFINEHASHSIZE * sizeof( define_t* ), TAG_IDLIB_PARSER );
		idParser::AddGlobalDefinesToSource();
	}
	return true;
}

/*
================
idParser::LoadMemory
================
*/
int idParser::LoadMemory( const char* ptr, int length, const char* name )
{
	idLexer* script;

	if( idParser::loaded )
	{
		idLib::common->FatalError( "idParser::loadMemory: another source already loaded" );
		return false;
	}
	script = new( TAG_IDLIB_PARSER ) idLexer( ptr, length, name );
	if( !script->IsLoaded() )
	{
		delete script;
		return false;
	}
	script->SetFlags( idParser::flags );
	script->SetPunctuations( idParser::punctuations );
	script->next = NULL;
	idParser::filename = name;
	idParser::scriptstack = script;
	idParser::tokens = NULL;
	idParser::indentstack = NULL;
	idParser::skip = 0;
	idParser::loaded = true;

	if( !idParser::definehash )
	{
		idParser::defines = NULL;
		idParser::definehash = ( define_t** ) Mem_ClearedAlloc( DEFINEHASHSIZE * sizeof( define_t* ), TAG_IDLIB_PARSER );
		idParser::AddGlobalDefinesToSource();
	}
	return true;
}

/*
================
idParser::FreeSource
================
*/
void idParser::FreeSource( bool keepDefines )
{
	idLexer* script;
	idToken* token;
	define_t* define;
	indent_t* indent;
	int i;

	// free all the scripts
	while( scriptstack )
	{
		script = scriptstack;
		scriptstack = scriptstack->next;
		delete script;
	}
	// free all the tokens
	while( tokens )
	{
		token = tokens;
		tokens = tokens->next;
		delete token;
	}
	// free all indents
	while( indentstack )
	{
		indent = indentstack;
		indentstack = indentstack->next;
		Mem_Free( indent );
	}
	if( !keepDefines )
	{
		// free hash table
		if( definehash )
		{
			// free defines
			for( i = 0; i < DEFINEHASHSIZE; i++ )
			{
				while( definehash[i] )
				{
					define = definehash[i];
					definehash[i] = definehash[i]->hashnext;
					FreeDefine( define );
				}
			}
			defines = NULL;
			Mem_Free( idParser::definehash );
			definehash = NULL;
		}
	}
	loaded = false;
}

/*
================
idParser::GetPunctuationFromId
================
*/
const char* idParser::GetPunctuationFromId( int id )
{
	int i;

	if( !idParser::punctuations )
	{
		idLexer lex;
		return lex.GetPunctuationFromId( id );
	}

	for( i = 0; idParser::punctuations[i].p; i++ )
	{
		if( idParser::punctuations[i].n == id )
		{
			return idParser::punctuations[i].p;
		}
	}
	return "unknown punctuation";
}

/*
================
idParser::GetPunctuationId
================
*/
int idParser::GetPunctuationId( const char* p )
{
	int i;

	if( !idParser::punctuations )
	{
		idLexer lex;
		return lex.GetPunctuationId( p );
	}

	for( i = 0; idParser::punctuations[i].p; i++ )
	{
		if( !strcmp( idParser::punctuations[i].p, p ) )
		{
			return idParser::punctuations[i].n;
		}
	}
	return 0;
}

/*
================
idParser::idParser
================
*/
idParser::idParser()
{
	this->loaded = false;
	this->OSPath = false;
	this->punctuations = 0;
	this->flags = 0;
	this->scriptstack = NULL;
	this->indentstack = NULL;
	this->definehash = NULL;
	this->defines = NULL;
	this->tokens = NULL;
	this->marker_p = NULL;
}

/*
================
idParser::idParser
================
*/
idParser::idParser( int flags )
{
	this->loaded = false;
	this->OSPath = false;
	this->punctuations = 0;
	this->flags = flags;
	this->scriptstack = NULL;
	this->indentstack = NULL;
	this->definehash = NULL;
	this->defines = NULL;
	this->tokens = NULL;
	this->marker_p = NULL;
}

/*
================
idParser::idParser
================
*/
idParser::idParser( const char* filename, int flags, bool OSPath )
{
	this->loaded = false;
	this->OSPath = true;
	this->punctuations = 0;
	this->flags = flags;
	this->scriptstack = NULL;
	this->indentstack = NULL;
	this->definehash = NULL;
	this->defines = NULL;
	this->tokens = NULL;
	this->marker_p = NULL;
	LoadFile( filename, OSPath );
}

/*
================
idParser::idParser
================
*/
idParser::idParser( const char* ptr, int length, const char* name, int flags )
{
	this->loaded = false;
	this->OSPath = false;
	this->punctuations = 0;
	this->flags = flags;
	this->scriptstack = NULL;
	this->indentstack = NULL;
	this->definehash = NULL;
	this->defines = NULL;
	this->tokens = NULL;
	this->marker_p = NULL;
	LoadMemory( ptr, length, name );
}

/*
================
idParser::~idParser
================
*/
idParser::~idParser()
{
	idParser::FreeSource( false );
}

/*
========================
idParser::EndOfFile
========================
*/
bool idParser::EndOfFile()
{
	if( scriptstack != NULL )
	{
		return ( bool ) scriptstack->EndOfFile();
	}
	return true;
}

