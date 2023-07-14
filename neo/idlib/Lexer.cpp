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

#define PUNCTABLE

//longer punctuations first
punctuation_t default_punctuations[] =
{
	//binary operators
	{">>=", P_RSHIFT_ASSIGN},
	{"<<=", P_LSHIFT_ASSIGN},
	//
	{"...", P_PARMS},
	//define merge operator
	{"##", P_PRECOMPMERGE},				// pre-compiler
	//logic operators
	{"&&", P_LOGIC_AND},					// pre-compiler
	{"||", P_LOGIC_OR},					// pre-compiler
	{">=", P_LOGIC_GEQ},					// pre-compiler
	{"<=", P_LOGIC_LEQ},					// pre-compiler
	{"==", P_LOGIC_EQ},					// pre-compiler
	{"!=", P_LOGIC_UNEQ},				// pre-compiler
	//arithmatic operators
	{"*=", P_MUL_ASSIGN},
	{"/=", P_DIV_ASSIGN},
	{"%=", P_MOD_ASSIGN},
	{"+=", P_ADD_ASSIGN},
	{"-=", P_SUB_ASSIGN},
	{"++", P_INC},
	{"--", P_DEC},
	//binary operators
	{"&=", P_BIN_AND_ASSIGN},
	{"|=", P_BIN_OR_ASSIGN},
	{"^=", P_BIN_XOR_ASSIGN},
	{">>", P_RSHIFT},					// pre-compiler
	{"<<", P_LSHIFT},					// pre-compiler
	//reference operators
	{"->", P_POINTERREF},
	//C++
	{"::", P_CPP1},
	{".*", P_CPP2},
	//arithmatic operators
	{"*", P_MUL},						// pre-compiler
	{"/", P_DIV},						// pre-compiler
	{"%", P_MOD},						// pre-compiler
	{"+", P_ADD},						// pre-compiler
	{"-", P_SUB},						// pre-compiler
	{"=", P_ASSIGN},
	//binary operators
	{"&", P_BIN_AND},					// pre-compiler
	{"|", P_BIN_OR},						// pre-compiler
	{"^", P_BIN_XOR},					// pre-compiler
	{"~", P_BIN_NOT},					// pre-compiler
	//logic operators
	{"!", P_LOGIC_NOT},					// pre-compiler
	{">", P_LOGIC_GREATER},				// pre-compiler
	{"<", P_LOGIC_LESS},					// pre-compiler
	//reference operator
	{".", P_REF},
	//seperators
	{",", P_COMMA},						// pre-compiler
	{";", P_SEMICOLON},
	//label indication
	{":", P_COLON},						// pre-compiler
	//if statement
	{"?", P_QUESTIONMARK},				// pre-compiler
	//embracements
	{"(", P_PARENTHESESOPEN},			// pre-compiler
	{")", P_PARENTHESESCLOSE},			// pre-compiler
	{"{", P_BRACEOPEN},					// pre-compiler
	{"}", P_BRACECLOSE},					// pre-compiler
	{"[", P_SQBRACKETOPEN},
	{"]", P_SQBRACKETCLOSE},
	//
	{"\\", P_BACKSLASH},
	//precompiler operator
	{"#", P_PRECOMP},					// pre-compiler
	{"$", P_DOLLAR},
	{NULL, 0}
};

int default_punctuationtable[256];
int default_nextpunctuation[sizeof( default_punctuations ) / sizeof( punctuation_t )];
int default_setup;

char idLexer::baseFolder[ 256 ];

/*
================
idLexer::CreatePunctuationTable
================
*/
void idLexer::CreatePunctuationTable( const punctuation_t* punctuations )
{
	int i, n, lastp;
	const punctuation_t* p, *newp;

	//get memory for the table
	if( punctuations == default_punctuations )
	{
		idLexer::punctuationtable = default_punctuationtable;
		idLexer::nextpunctuation = default_nextpunctuation;
		if( default_setup )
		{
			return;
		}
		default_setup = true;
		i = sizeof( default_punctuations ) / sizeof( punctuation_t );
	}
	else
	{
		if( !idLexer::punctuationtable || idLexer::punctuationtable == default_punctuationtable )
		{
			idLexer::punctuationtable = ( int* ) Mem_Alloc( 256 * sizeof( int ), TAG_IDLIB_LEXER );
		}
		if( idLexer::nextpunctuation && idLexer::nextpunctuation != default_nextpunctuation )
		{
			Mem_Free( idLexer::nextpunctuation );
		}
		for( i = 0; punctuations[i].p; i++ )
		{
		}
		idLexer::nextpunctuation = ( int* ) Mem_Alloc( i * sizeof( int ), TAG_IDLIB_LEXER );
	}
	memset( idLexer::punctuationtable, 0xFF, 256 * sizeof( int ) );
	memset( idLexer::nextpunctuation, 0xFF, i * sizeof( int ) );
	//add the punctuations in the list to the punctuation table
	for( i = 0; punctuations[i].p; i++ )
	{
		newp = &punctuations[i];
		lastp = -1;
		//sort the punctuations in this table entry on length (longer punctuations first)
		for( n = idLexer::punctuationtable[( unsigned int ) newp->p[0]]; n >= 0; n = idLexer::nextpunctuation[n] )
		{
			p = &punctuations[n];
			if( strlen( p->p ) < strlen( newp->p ) )
			{
				idLexer::nextpunctuation[i] = n;
				if( lastp >= 0 )
				{
					idLexer::nextpunctuation[lastp] = i;
				}
				else
				{
					idLexer::punctuationtable[( unsigned int ) newp->p[0]] = i;
				}
				break;
			}
			lastp = n;
		}
		if( n < 0 )
		{
			idLexer::nextpunctuation[i] = -1;
			if( lastp >= 0 )
			{
				idLexer::nextpunctuation[lastp] = i;
			}
			else
			{
				idLexer::punctuationtable[( unsigned int ) newp->p[0]] = i;
			}
		}
	}
}

/*
================
idLexer::GetPunctuationFromId
================
*/
const char* idLexer::GetPunctuationFromId( int id )
{
	int i;

	for( i = 0; idLexer::punctuations[i].p; i++ )
	{
		if( idLexer::punctuations[i].n == id )
		{
			return idLexer::punctuations[i].p;
		}
	}
	return "unknown punctuation";
}

/*
================
idLexer::GetPunctuationId
================
*/
int idLexer::GetPunctuationId( const char* p )
{
	int i;

	for( i = 0; idLexer::punctuations[i].p; i++ )
	{
		if( !strcmp( idLexer::punctuations[i].p, p ) )
		{
			return idLexer::punctuations[i].n;
		}
	}
	return 0;
}

/*
================
idLexer::Error
================
*/
void idLexer::Error( const char* str, ... )
{
	char text[MAX_STRING_CHARS];
	va_list ap;

	hadError = true;

	if( idLexer::flags & LEXFL_NOERRORS )
	{
		return;
	}

	va_start( ap, str );
	idStr::vsnPrintf( text, sizeof( text ), str, ap );
	va_end( ap );

	if( idLexer::flags & LEXFL_NOFATALERRORS )
	{
		idLib::common->Warning( "file %s, line %d: %s", idLexer::filename.c_str(), idLexer::line, text );
	}
	else
	{
		idLib::common->Error( "file %s, line %d: %s", idLexer::filename.c_str(), idLexer::line, text );
	}
}

/*
================
idLexer::Warning
================
*/
void idLexer::Warning( const char* str, ... )
{
	char text[MAX_STRING_CHARS];
	va_list ap;

	if( idLexer::flags & LEXFL_NOWARNINGS )
	{
		return;
	}

	va_start( ap, str );
	idStr::vsnPrintf( text, sizeof( text ), str, ap );
	va_end( ap );
	idLib::common->Warning( "file %s, line %d: %s", idLexer::filename.c_str(), idLexer::line, text );
}

/*
================
idLexer::SetPunctuations
================
*/
void idLexer::SetPunctuations( const punctuation_t* p )
{
#ifdef PUNCTABLE
	if( p )
	{
		idLexer::CreatePunctuationTable( p );
	}
	else
	{
		idLexer::CreatePunctuationTable( default_punctuations );
	}
#endif //PUNCTABLE
	if( p )
	{
		idLexer::punctuations = p;
	}
	else
	{
		idLexer::punctuations = default_punctuations;
	}
}

/*
================
idLexer::ReadWhiteSpace

Reads spaces, tabs, C-like comments etc.
When a newline character is found the scripts line counter is increased.
================
*/
int idLexer::ReadWhiteSpace()
{
	while( 1 )
	{
		// skip white space
		while( *idLexer::script_p <= ' ' )
		{
			if( !*idLexer::script_p )
			{
				return 0;
			}
			if( *idLexer::script_p == '\n' )
			{
				idLexer::line++;
			}
			idLexer::script_p++;
		}
		// skip comments
		if( *idLexer::script_p == '/' )
		{
			// comments //
			if( *( idLexer::script_p + 1 ) == '/' )
			{
				idLexer::script_p++;
				do
				{
					idLexer::script_p++;
					if( !*idLexer::script_p )
					{
						return 0;
					}
				}
				while( *idLexer::script_p != '\n' );
				idLexer::line++;
				idLexer::script_p++;
				if( !*idLexer::script_p )
				{
					return 0;
				}
				continue;
			}
			// comments /* */
			else if( *( idLexer::script_p + 1 ) == '*' )
			{
				idLexer::script_p++;
				while( 1 )
				{
					idLexer::script_p++;
					if( !*idLexer::script_p )
					{
						return 0;
					}
					if( *idLexer::script_p == '\n' )
					{
						idLexer::line++;
					}
					else if( *idLexer::script_p == '/' )
					{
						if( *( idLexer::script_p - 1 ) == '*' )
						{
							break;
						}
						if( *( idLexer::script_p + 1 ) == '*' )
						{
							idLexer::Warning( "nested comment" );
						}
					}
				}
				idLexer::script_p++;
				if( !*idLexer::script_p )
				{
					return 0;
				}
				idLexer::script_p++;
				if( !*idLexer::script_p )
				{
					return 0;
				}
				continue;
			}
		}
		break;
	}
	return 1;
}

/*
========================
idLexer::SkipWhiteSpace

Reads spaces, tabs, C-like comments etc. When a newline character is found, the scripts line
counter is increased. Returns false if there is no token left to be read.
========================
*/
bool idLexer::SkipWhiteSpace( bool currentLine )
{
	while( 1 )
	{
		assert( script_p <= end_p );
		if( script_p == end_p )
		{
			return false;
		}
		// skip white space
		while( *script_p <= ' ' )
		{
			if( script_p == end_p )
			{
				return false;
			}
			if( !*script_p )
			{
				return false;
			}
			if( *script_p == '\n' )
			{
				line++;
				if( currentLine )
				{
					script_p++;
					return true;
				}
			}
			script_p++;
		}
		// skip comments
		if( *script_p == '/' )
		{
			// comments //
			if( *( script_p + 1 ) == '/' )
			{
				script_p++;
				do
				{
					script_p++;
					if( !*script_p )
					{
						return false;
					}
				}
				while( *script_p != '\n' );
				line++;
				script_p++;
				if( currentLine )
				{
					return true;
				}
				if( !*script_p )
				{
					return false;
				}
				continue;
			}
			// comments /* */
			else if( *( script_p + 1 ) == '*' )
			{
				script_p++;
				while( 1 )
				{
					script_p++;
					if( !*script_p )
					{
						return false;
					}
					if( *script_p == '\n' )
					{
						line++;
					}
					else if( *script_p == '/' )
					{
						if( *( script_p - 1 ) == '*' )
						{
							break;
						}
						if( *( script_p + 1 ) == '*' )
						{
							Warning( "nested comment" );
						}
					}
				}
				script_p++;
				if( !*script_p )
				{
					return false;
				}
				continue;
			}
		}
		break;
	}
	return true;
}

/*
================
idLexer::ReadEscapeCharacter
================
*/
int idLexer::ReadEscapeCharacter( char* ch )
{
	int c, val, i;

	// step over the leading '\\'
	idLexer::script_p++;
	// determine the escape character
	switch( *idLexer::script_p )
	{
		case '\\':
			c = '\\';
			break;
		case 'n':
			c = '\n';
			break;
		case 'r':
			c = '\r';
			break;
		case 't':
			c = '\t';
			break;
		case 'v':
			c = '\v';
			break;
		case 'b':
			c = '\b';
			break;
		case 'f':
			c = '\f';
			break;
		case 'a':
			c = '\a';
			break;
		case '\'':
			c = '\'';
			break;
		case '\"':
			c = '\"';
			break;
		case '\?':
			c = '\?';
			break;
		case 'x':
		{
			idLexer::script_p++;
			for( i = 0, val = 0; ; i++, idLexer::script_p++ )
			{
				c = *idLexer::script_p;
				if( c >= '0' && c <= '9' )
				{
					c = c - '0';
				}
				else if( c >= 'A' && c <= 'Z' )
				{
					c = c - 'A' + 10;
				}
				else if( c >= 'a' && c <= 'z' )
				{
					c = c - 'a' + 10;
				}
				else
				{
					break;
				}
				val = ( val << 4 ) + c;
			}
			idLexer::script_p--;
			if( val > 0xFF )
			{
				idLexer::Warning( "too large value in escape character" );
				val = 0xFF;
			}
			c = val;
			break;
		}
		default: //NOTE: decimal ASCII code, NOT octal
		{
			if( *idLexer::script_p < '0' || *idLexer::script_p > '9' )
			{
				idLexer::Error( "unknown escape char" );
			}
			for( i = 0, val = 0; ; i++, idLexer::script_p++ )
			{
				c = *idLexer::script_p;
				if( c >= '0' && c <= '9' )
				{
					c = c - '0';
				}
				else
				{
					break;
				}
				val = val * 10 + c;
			}
			idLexer::script_p--;
			if( val > 0xFF )
			{
				idLexer::Warning( "too large value in escape character" );
				val = 0xFF;
			}
			c = val;
			break;
		}
	}
	// step over the escape character or the last digit of the number
	idLexer::script_p++;
	// store the escape character
	*ch = c;
	// succesfully read escape character
	return 1;
}

/*
================
idLexer::ReadString

Escape characters are interpretted.
Reads two strings with only a white space between them as one string.
================
*/
int idLexer::ReadString( idToken* token, int quote )
{
	int tmpline;
	const char* tmpscript_p;
	char ch;

	if( quote == '\"' )
	{
		token->type = TT_STRING;
	}
	else
	{
		token->type = TT_LITERAL;
	}

	// leading quote
	idLexer::script_p++;

	while( 1 )
	{
		// if there is an escape character and escape characters are allowed
		if( *idLexer::script_p == '\\' && !( idLexer::flags & LEXFL_NOSTRINGESCAPECHARS ) )
		{
			if( !idLexer::ReadEscapeCharacter( &ch ) )
			{
				return 0;
			}
			token->AppendDirty( ch );
		}
		// if a trailing quote
		else if( *idLexer::script_p == quote )
		{
			// step over the quote
			idLexer::script_p++;
			// if consecutive strings should not be concatenated
			if( ( idLexer::flags & LEXFL_NOSTRINGCONCAT ) &&
					( !( idLexer::flags & LEXFL_ALLOWBACKSLASHSTRINGCONCAT ) || ( quote != '\"' ) ) )
			{
				break;
			}

			tmpscript_p = idLexer::script_p;
			tmpline = idLexer::line;
			// read white space between possible two consecutive strings
			if( !idLexer::ReadWhiteSpace() )
			{
				idLexer::script_p = tmpscript_p;
				idLexer::line = tmpline;
				break;
			}

			if( idLexer::flags & LEXFL_NOSTRINGCONCAT )
			{
				if( *idLexer::script_p != '\\' )
				{
					idLexer::script_p = tmpscript_p;
					idLexer::line = tmpline;
					break;
				}
				// step over the '\\'
				idLexer::script_p++;
				if( !idLexer::ReadWhiteSpace() || ( *idLexer::script_p != quote ) )
				{
					idLexer::Error( "expecting string after '\' terminated line" );
					return 0;
				}
			}

			// if there's no leading qoute
			if( *idLexer::script_p != quote )
			{
				idLexer::script_p = tmpscript_p;
				idLexer::line = tmpline;
				break;
			}
			// step over the new leading quote
			idLexer::script_p++;
		}
		else
		{
			if( *idLexer::script_p == '\0' )
			{
				idLexer::Error( "missing trailing quote" );
				return 0;
			}
			if( *idLexer::script_p == '\n' )
			{
				idLexer::Error( "newline inside string" );
				return 0;
			}
			token->AppendDirty( *idLexer::script_p++ );
		}
	}
	token->data[token->len] = '\0';

	if( token->type == TT_LITERAL )
	{
		if( !( idLexer::flags & LEXFL_ALLOWMULTICHARLITERALS ) )
		{
			if( token->Length() != 1 )
			{
				idLexer::Warning( "literal is not one character long" );
			}
		}
		token->subtype = ( *token )[0];
	}
	else
	{
		// the sub type is the length of the string
		token->subtype = token->Length();
	}
	return 1;
}

/*
================
idLexer::ReadName
================
*/
int idLexer::ReadName( idToken* token )
{
	char c;

	token->type = TT_NAME;
	do
	{
		token->AppendDirty( *idLexer::script_p++ );
		c = *idLexer::script_p;
	}
	while( ( c >= 'a' && c <= 'z' ) ||
			( c >= 'A' && c <= 'Z' ) ||
			( c >= '0' && c <= '9' ) ||
			c == '_' ||
			// if treating all tokens as strings, don't parse '-' as a separate token
			( ( idLexer::flags & LEXFL_ONLYSTRINGS ) && ( c == '-' ) ) ||
			// if special path name characters are allowed
			( ( idLexer::flags & LEXFL_ALLOWPATHNAMES ) && ( c == '/' || c == '\\' || c == ':' || c == '.' ) ) );
	token->data[token->len] = '\0';
	//the sub type is the length of the name
	token->subtype = token->Length();
	return 1;
}

/*
================
idLexer::CheckString
================
*/
ID_INLINE int idLexer::CheckString( const char* str ) const
{
	int i;

	for( i = 0; str[i]; i++ )
	{
		if( idLexer::script_p[i] != str[i] )
		{
			return false;
		}
	}
	return true;
}

/*
================
idLexer::ReadNumber
================
*/
int idLexer::ReadNumber( idToken* token )
{
	int i;
	int dot;
	char c, c2;

	token->type = TT_NUMBER;
	token->subtype = 0;
	token->intvalue = 0;
	token->floatvalue = 0;

	c = *idLexer::script_p;
	c2 = *( idLexer::script_p + 1 );

	if( c == '0' && c2 != '.' )
	{
		// check for a hexadecimal number
		if( c2 == 'x' || c2 == 'X' )
		{
			token->AppendDirty( *idLexer::script_p++ );
			token->AppendDirty( *idLexer::script_p++ );
			c = *idLexer::script_p;
			while( ( c >= '0' && c <= '9' ) ||
					( c >= 'a' && c <= 'f' ) ||
					( c >= 'A' && c <= 'F' ) )
			{
				token->AppendDirty( c );
				c = *( ++idLexer::script_p );
			}
			token->subtype = TT_HEX | TT_INTEGER;
		}
		// check for a binary number
		else if( c2 == 'b' || c2 == 'B' )
		{
			token->AppendDirty( *idLexer::script_p++ );
			token->AppendDirty( *idLexer::script_p++ );
			c = *idLexer::script_p;
			while( c == '0' || c == '1' )
			{
				token->AppendDirty( c );
				c = *( ++idLexer::script_p );
			}
			token->subtype = TT_BINARY | TT_INTEGER;
		}
		// its an octal number
		else
		{
			token->AppendDirty( *idLexer::script_p++ );
			c = *idLexer::script_p;
			while( c >= '0' && c <= '7' )
			{
				token->AppendDirty( c );
				c = *( ++idLexer::script_p );
			}
			token->subtype = TT_OCTAL | TT_INTEGER;
		}
	}
	else
	{
		// decimal integer or floating point number or ip address
		dot = 0;
		while( 1 )
		{
			if( c >= '0' && c <= '9' )
			{
			}
			else if( c == '.' )
			{
				dot++;
			}
			else
			{
				break;
			}
			token->AppendDirty( c );
			c = *( ++idLexer::script_p );
		}
		if( c == 'e' && dot == 0 )
		{
			//We have scientific notation without a decimal point
			dot++;
		}
		// if a floating point number
		if( dot == 1 )
		{
			token->subtype = TT_DECIMAL | TT_FLOAT;
			// check for floating point exponent
			if( c == 'e' )
			{
				//Append the e so that GetFloatValue code works
				token->AppendDirty( c );
				c = *( ++idLexer::script_p );
				if( c == '-' )
				{
					token->AppendDirty( c );
					c = *( ++idLexer::script_p );
				}
				else if( c == '+' )
				{
					token->AppendDirty( c );
					c = *( ++idLexer::script_p );
				}
				while( c >= '0' && c <= '9' )
				{
					token->AppendDirty( c );
					c = *( ++idLexer::script_p );
				}
			}
			// check for floating point exception infinite 1.#INF or indefinite 1.#IND or NaN
			else if( c == '#' )
			{
				c2 = 4;
				if( CheckString( "INF" ) )
				{
					token->subtype |= TT_INFINITE;
				}
				else if( CheckString( "IND" ) )
				{
					token->subtype |= TT_INDEFINITE;
				}
				else if( CheckString( "NAN" ) )
				{
					token->subtype |= TT_NAN;
				}
				else if( CheckString( "QNAN" ) )
				{
					token->subtype |= TT_NAN;
					c2++;
				}
				else if( CheckString( "SNAN" ) )
				{
					token->subtype |= TT_NAN;
					c2++;
				}
				for( i = 0; i < c2; i++ )
				{
					token->AppendDirty( c );
					c = *( ++idLexer::script_p );
				}
				while( c >= '0' && c <= '9' )
				{
					token->AppendDirty( c );
					c = *( ++idLexer::script_p );
				}
				if( !( idLexer::flags & LEXFL_ALLOWFLOATEXCEPTIONS ) )
				{
					token->AppendDirty( 0 );	// zero terminate for c_str
					idLexer::Error( "parsed %s", token->c_str() );
				}
			}
		}
		else if( dot > 1 )
		{
			if( !( idLexer::flags & LEXFL_ALLOWIPADDRESSES ) )
			{
				idLexer::Error( "more than one dot in number" );
				return 0;
			}
			if( dot != 3 )
			{
				idLexer::Error( "ip address should have three dots" );
				return 0;
			}
			token->subtype = TT_IPADDRESS;
		}
		else
		{
			token->subtype = TT_DECIMAL | TT_INTEGER;
		}
	}

	if( token->subtype & TT_FLOAT )
	{
		if( c > ' ' )
		{
			// single-precision: float
			if( c == 'f' || c == 'F' )
			{
				token->subtype |= TT_SINGLE_PRECISION;
				idLexer::script_p++;
			}
			// extended-precision: long double
			else if( c == 'l' || c == 'L' )
			{
				token->subtype |= TT_EXTENDED_PRECISION;
				idLexer::script_p++;
			}
			// default is double-precision: double
			else
			{
				token->subtype |= TT_DOUBLE_PRECISION;
			}
		}
		else
		{
			token->subtype |= TT_DOUBLE_PRECISION;
		}
	}
	else if( token->subtype & TT_INTEGER )
	{
		if( c > ' ' )
		{
			// default: signed long
			for( i = 0; i < 2; i++ )
			{
				// long integer
				if( c == 'l' || c == 'L' )
				{
					token->subtype |= TT_LONG;
				}
				// unsigned integer
				else if( c == 'u' || c == 'U' )
				{
					token->subtype |= TT_UNSIGNED;
				}
				else
				{
					break;
				}
				c = *( ++idLexer::script_p );
			}
		}
	}
	else if( token->subtype & TT_IPADDRESS )
	{
		if( c == ':' )
		{
			token->AppendDirty( c );
			c = *( ++idLexer::script_p );
			while( c >= '0' && c <= '9' )
			{
				token->AppendDirty( c );
				c = *( ++idLexer::script_p );
			}
			token->subtype |= TT_IPPORT;
		}
	}
	token->data[token->len] = '\0';
	return 1;
}

/*
================
idLexer::ReadPunctuation
================
*/
int idLexer::ReadPunctuation( idToken* token )
{
	int l, n, i;
	const char* p;
	const punctuation_t* punc;

#ifdef PUNCTABLE
	for( n = idLexer::punctuationtable[( unsigned int ) * ( idLexer::script_p )]; n >= 0; n = idLexer::nextpunctuation[n] )
	{
		punc = &( idLexer::punctuations[n] );
#else
	int i;

	for( i = 0; idLexer::punctuations[i].p; i++ )
	{
		punc = &idLexer::punctuations[i];
#endif
		p = punc->p;
		// check for this punctuation in the script
		for( l = 0; p[l] && idLexer::script_p[l]; l++ )
		{
			if( idLexer::script_p[l] != p[l] )
			{
				break;
			}
		}
		if( !p[l] )
		{
			//
			token->EnsureAlloced( l + 1, false );
			for( i = 0; i <= l; i++ )
			{
				token->data[i] = p[i];
			}
			token->len = l;
			//
			idLexer::script_p += l;
			token->type = TT_PUNCTUATION;
			// sub type is the punctuation id
			token->subtype = punc->n;
			return 1;
		}
	}
	return 0;
}

/*
================
idLexer::ReadToken
================
*/
int idLexer::ReadToken( idToken* token )
{
	int c;

	if( !loaded )
	{
		idLib::common->Error( "idLexer::ReadToken: no file loaded" );
		return 0;
	}

	if( script_p == NULL )
	{
		return 0;
	}

	// if there is a token available (from unreadToken)
	if( tokenavailable )
	{
		tokenavailable = 0;
		*token = idLexer::token;
		return 1;
	}
	// save script pointer
	lastScript_p = script_p;
	// save line counter
	lastline = line;
	// clear the token stuff
	token->data[0] = '\0';
	token->len = 0;
	// start of the white space
	whiteSpaceStart_p = script_p;
	token->whiteSpaceStart_p = script_p;
	// read white space before token
	if( !ReadWhiteSpace() )
	{
		return 0;
	}
	// end of the white space
	idLexer::whiteSpaceEnd_p = script_p;
	token->whiteSpaceEnd_p = script_p;
	// line the token is on
	token->line = line;
	// number of lines crossed before token
	token->linesCrossed = line - lastline;
	// clear token flags
	token->flags = 0;

	c = *idLexer::script_p;

	// if we're keeping everything as whitespace deliminated strings
	if( idLexer::flags & LEXFL_ONLYSTRINGS )
	{
		// if there is a leading quote
		if( c == '\"' || c == '\'' )
		{
			if( !idLexer::ReadString( token, c ) )
			{
				return 0;
			}
		}
		else if( !idLexer::ReadName( token ) )
		{
			return 0;
		}
	}
	// if there is a number
	else if( ( c >= '0' && c <= '9' ) ||
			 ( c == '.' && ( *( idLexer::script_p + 1 ) >= '0' && *( idLexer::script_p + 1 ) <= '9' ) ) )
	{
		if( !idLexer::ReadNumber( token ) )
		{
			return 0;
		}
		// if names are allowed to start with a number
		if( idLexer::flags & LEXFL_ALLOWNUMBERNAMES )
		{
			c = *idLexer::script_p;
			if( ( c >= 'a' && c <= 'z' ) ||	( c >= 'A' && c <= 'Z' ) || c == '_' )
			{
				if( !idLexer::ReadName( token ) )
				{
					return 0;
				}
			}
		}
	}
	// if there is a leading quote
	else if( c == '\"' || c == '\'' )
	{
		if( !idLexer::ReadString( token, c ) )
		{
			return 0;
		}
	}
	// if there is a name
	else if( ( c >= 'a' && c <= 'z' ) ||	( c >= 'A' && c <= 'Z' ) || c == '_' )
	{
		if( !idLexer::ReadName( token ) )
		{
			return 0;
		}
	}
	// names may also start with a slash when pathnames are allowed
	else if( ( idLexer::flags & LEXFL_ALLOWPATHNAMES ) && ( ( c == '/' || c == '\\' ) || c == '.' ) )
	{
		if( !idLexer::ReadName( token ) )
		{
			return 0;
		}
	}
	// check for punctuations
	else if( !idLexer::ReadPunctuation( token ) )
	{
		idLexer::Error( "unknown punctuation %c", c );
		return 0;
	}
	// succesfully read a token
	return 1;
}

/*
================
idLexer::ExpectTokenString
================
*/
int idLexer::ExpectTokenString( const char* string )
{
	idToken token;

	if( !idLexer::ReadToken( &token ) )
	{
		idLexer::Error( "couldn't find expected '%s'", string );
		return 0;
	}
	if( token != string )
	{
		idLexer::Error( "expected '%s' but found '%s'", string, token.c_str() );
		return 0;
	}
	return 1;
}

/*
================
idLexer::ExpectTokenType
================
*/
int idLexer::ExpectTokenType( int type, int subtype, idToken* token )
{
	idStr str;

	if( !idLexer::ReadToken( token ) )
	{
		idLexer::Error( "couldn't read expected token" );
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
		idLexer::Error( "expected a %s but found '%s'", str.c_str(), token->c_str() );
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
			idLexer::Error( "expected %s but found '%s'", str.c_str(), token->c_str() );
			return 0;
		}
	}
	else if( token->type == TT_PUNCTUATION )
	{
		if( subtype < 0 )
		{
			idLexer::Error( "BUG: wrong punctuation subtype" );
			return 0;
		}
		if( token->subtype != subtype )
		{
			idLexer::Error( "expected '%s' but found '%s'", GetPunctuationFromId( subtype ), token->c_str() );
			return 0;
		}
	}
	return 1;
}

/*
================
idLexer::ExpectAnyToken
================
*/
int idLexer::ExpectAnyToken( idToken* token )
{
	if( !idLexer::ReadToken( token ) )
	{
		idLexer::Error( "couldn't read expected token" );
		return 0;
	}
	else
	{
		return 1;
	}
}

/*
================
idLexer::CheckTokenString
================
*/
int idLexer::CheckTokenString( const char* string )
{
	idToken tok;

	if( !ReadToken( &tok ) )
	{
		return 0;
	}
	// if the given string is available
	if( tok == string )
	{
		return 1;
	}
	// unread token
	script_p = lastScript_p;
	line = lastline;
	return 0;
}

/*
================
idLexer::CheckTokenType
================
*/
int idLexer::CheckTokenType( int type, int subtype, idToken* token )
{
	idToken tok;

	if( !ReadToken( &tok ) )
	{
		return 0;
	}
	// if the type matches
	if( tok.type == type && ( tok.subtype & subtype ) == subtype )
	{
		*token = tok;
		return 1;
	}
	// unread token
	script_p = lastScript_p;
	line = lastline;
	return 0;
}

/*
================
idLexer::PeekTokenString
================
*/
int idLexer::PeekTokenString( const char* string )
{
	idToken tok;

	if( !ReadToken( &tok ) )
	{
		return 0;
	}

	// unread token
	script_p = lastScript_p;
	line = lastline;

	// if the given string is available
	if( tok == string )
	{
		return 1;
	}
	return 0;
}

/*
================
idLexer::PeekTokenType
================
*/
int idLexer::PeekTokenType( int type, int subtype, idToken* token )
{
	idToken tok;

	if( !ReadToken( &tok ) )
	{
		return 0;
	}

	// unread token
	script_p = lastScript_p;
	line = lastline;

	// if the type matches
	if( tok.type == type && ( tok.subtype & subtype ) == subtype )
	{
		*token = tok;
		return 1;
	}
	return 0;
}

/*
================
idLexer::SkipUntilString
================
*/
int idLexer::SkipUntilString( const char* string )
{
	idToken token;

	while( idLexer::ReadToken( &token ) )
	{
		if( token == string )
		{
			return 1;
		}
	}
	return 0;
}

/*
================
idLexer::SkipRestOfLine
================
*/
int idLexer::SkipRestOfLine()
{
	idToken token;

	while( idLexer::ReadToken( &token ) )
	{
		if( token.linesCrossed )
		{
			idLexer::script_p = lastScript_p;
			idLexer::line = lastline;
			return 1;
		}
	}
	return 0;
}

/*
=================
idLexer::SkipBracedSection

Skips until a matching close brace is found.
Internal brace depths are properly skipped.
=================
*/
int idLexer::SkipBracedSection( bool parseFirstBrace, braceSkipMode_t skipMode/* = BRSKIP_BRACE */, int* skipped /*= nullptr*/ )
{
	idToken token;
	int depth;
	idStr openTokens[2] = { "{" , "["   };
	idStr closeTokens[2] = { "}" , "]" };

	if( skipped != nullptr )
	{
		*skipped = 0;
	}

	int scopeCount = 0;
	depth = parseFirstBrace ? 0 : 1;
	do
	{
		if( !ReadToken( &token ) )
		{
			return false;
		}
		if( token.type == TT_PUNCTUATION )
		{
			if( token == openTokens[skipMode] )
			{
				depth++;
				if( skipped != nullptr )
				{
					( *skipped )++;
				}
			}
			else if( token == closeTokens[skipMode] )
			{
				depth--;
			}
		}
	}
	while( depth );
	return true;
}

/*
================
idLexer::UnreadToken
================
*/
void idLexer::UnreadToken( const idToken* token )
{
	if( idLexer::tokenavailable )
	{
		idLib::common->FatalError( "idLexer::unreadToken, unread token twice\n" );
	}
	idLexer::token = *token;
	idLexer::tokenavailable = 1;
}

/*
================
idLexer::ReadTokenOnLine
================
*/
int idLexer::ReadTokenOnLine( idToken* token )
{
	idToken tok;

	if( !idLexer::ReadToken( &tok ) )
	{
		idLexer::script_p = lastScript_p;
		idLexer::line = lastline;
		return false;
	}
	// if no lines were crossed before this token
	if( !tok.linesCrossed )
	{
		*token = tok;
		return true;
	}
	// restore our position
	idLexer::script_p = lastScript_p;
	idLexer::line = lastline;
	token->Clear();
	return false;
}

/*
================
idLexer::ReadRestOfLine
================
*/
const char*	idLexer::ReadRestOfLine( idStr& out )
{
	while( 1 )
	{

		if( *idLexer::script_p == '\n' )
		{
			idLexer::line++;
			break;
		}

		if( !*idLexer::script_p )
		{
			break;
		}

		if( *idLexer::script_p <= ' ' )
		{
			out += " ";
		}
		else
		{
			out += *idLexer::script_p;
		}
		idLexer::script_p++;

	}

	out.Strip( ' ' );
	return out.c_str();
}

/*
================
idLexer::ParseInt
================
*/
int idLexer::ParseInt()
{
	idToken token;

	if( !idLexer::ReadToken( &token ) )
	{
		idLexer::Error( "couldn't read expected integer" );
		return 0;
	}
	if( token.type == TT_PUNCTUATION && token == "-" )
	{
		idLexer::ExpectTokenType( TT_NUMBER, TT_INTEGER, &token );
		return -( ( signed int ) token.GetIntValue() );
	}
	else if( token.type != TT_NUMBER || token.subtype == TT_FLOAT )
	{
		idLexer::Error( "expected integer value, found '%s'", token.c_str() );
	}
	return token.GetIntValue();
}

/*
================
idLexer::ParseBool
================
*/
bool idLexer::ParseBool()
{
	idToken token;

	if( !idLexer::ExpectTokenType( TT_NUMBER, 0, &token ) )
	{
		idLexer::Error( "couldn't read expected boolean" );
		return false;
	}
	return ( token.GetIntValue() != 0 );
}

/*
================
idLexer::ParseFloat
================
*/
float idLexer::ParseFloat( bool* errorFlag )
{
	idToken token;

	if( errorFlag )
	{
		*errorFlag = false;
	}

	if( !idLexer::ReadToken( &token ) )
	{
		if( errorFlag )
		{
			idLexer::Warning( "couldn't read expected floating point number" );
			*errorFlag = true;
		}
		else
		{
			idLexer::Error( "couldn't read expected floating point number" );
		}
		return 0;
	}
	if( token.type == TT_PUNCTUATION && token == "-" )
	{
		idLexer::ExpectTokenType( TT_NUMBER, 0, &token );
		return -token.GetFloatValue();
	}
	else if( token.type != TT_NUMBER )
	{
		if( errorFlag )
		{
			idLexer::Warning( "expected float value, found '%s'", token.c_str() );
			*errorFlag = true;
		}
		else
		{
			idLexer::Error( "expected float value, found '%s'", token.c_str() );
		}
	}
	return token.GetFloatValue();
}

/*
================
idLexer::Parse1DMatrix
================
*/
int idLexer::Parse1DMatrix( int x, float* m )
{
	int i;

	if( !idLexer::ExpectTokenString( "(" ) )
	{
		return false;
	}

	for( i = 0; i < x; i++ )
	{
		m[i] = idLexer::ParseFloat();
	}

	if( !idLexer::ExpectTokenString( ")" ) )
	{
		return false;
	}
	return true;
}

// RB begin
int idLexer::Parse1DMatrixJSON( int x, float* m )
{
	int i;

	if( !idLexer::ExpectTokenString( "[" ) )
	{
		return false;
	}

	for( i = 0; i < x; i++ )
	{
		m[i] = idLexer::ParseFloat();

		if( i < ( x - 1 ) && !idLexer::ExpectTokenString( "," ) )
		{
			return false;
		}
	}

	if( !idLexer::ExpectTokenString( "]" ) )
	{
		return false;
	}
	return true;
}
// RB end

/*
================
idLexer::Parse2DMatrix
================
*/
int idLexer::Parse2DMatrix( int y, int x, float* m )
{
	int i;

	if( !idLexer::ExpectTokenString( "(" ) )
	{
		return false;
	}

	for( i = 0; i < y; i++ )
	{
		if( !idLexer::Parse1DMatrix( x, m + i * x ) )
		{
			return false;
		}
	}

	if( !idLexer::ExpectTokenString( ")" ) )
	{
		return false;
	}
	return true;
}

/*
================
idLexer::Parse3DMatrix
================
*/
int idLexer::Parse3DMatrix( int z, int y, int x, float* m )
{
	int i;

	if( !idLexer::ExpectTokenString( "(" ) )
	{
		return false;
	}

	for( i = 0 ; i < z; i++ )
	{
		if( !idLexer::Parse2DMatrix( y, x, m + i * x * y ) )
		{
			return false;
		}
	}

	if( !idLexer::ExpectTokenString( ")" ) )
	{
		return false;
	}
	return true;
}

/*
=================
idParser::ParseBracedSection

The next token should be an open brace.
Parses until a matching close brace is found.
Maintains exact characters between braces.

  FIXME: this should use ReadToken and replace the token white space with correct indents and newlines
=================
*/
const char* idLexer::ParseBracedSectionExact( idStr& out, int tabs )
{
	int		depth;
	bool	doTabs;
	bool	skipWhite;

	out.Empty();

	if( !idLexer::ExpectTokenString( "{" ) )
	{
		return out.c_str();
	}

	out = "{";
	depth = 1;
	skipWhite = false;
	doTabs = tabs >= 0;

	while( depth && *idLexer::script_p )
	{
		char c = *( idLexer::script_p++ );

		switch( c )
		{
			case '\t':
			case ' ':
			{
				if( skipWhite )
				{
					continue;
				}
				break;
			}
			case '\n':
			{
				if( doTabs )
				{
					skipWhite = true;
					out += c;
					continue;
				}
				break;
			}
			case '{':
			{
				depth++;
				tabs++;
				break;
			}
			case '}':
			{
				depth--;
				tabs--;
				break;
			}
		}

		if( skipWhite )
		{
			int i = tabs;
			if( c == '{' )
			{
				i--;
			}
			skipWhite = false;
			for( ; i > 0; i-- )
			{
				out += '\t';
			}
		}
		out += c;
	}
	return out.c_str();
}

/*
=================
idParser::ParseBracedSection

The next token should be an open brace.
Parses until a matching close brace is found.
Maintains exact characters between braces.

  FIXME: this should use ReadToken and replace the token white space with correct indents and newlines
=================
*/
const char* idLexer::ParseBracketSectionExact( idStr& out, int tabs )
{
	int		depth;
	bool	doTabs;
	bool	skipWhite;

	out.Empty();

	if( !idLexer::ExpectTokenString( "[" ) )
	{
		return out.c_str();
	}

	out = "[";
	depth = 1;
	skipWhite = false;
	doTabs = tabs >= 0;

	while( depth && *idLexer::script_p )
	{
		char c = *( idLexer::script_p++ );

		switch( c )
		{
			case '\t':
			case ' ':
			{
				if( skipWhite )
				{
					continue;
				}
				break;
			}
			case '\n':
			{
				if( doTabs )
				{
					skipWhite = true;
					out += c;
					continue;
				}
				break;
			}
			case '[':
			{
				depth++;
				tabs++;
				break;
			}
			case ']':
			{
				depth--;
				tabs--;
				break;
			}
		}

		if( skipWhite )
		{
			int i = tabs;
			if( c == '[' )
			{
				i--;
			}
			skipWhite = false;
			for( ; i > 0; i-- )
			{
				out += '\t';
			}
		}
		out += c;
	}
	return out.c_str();
}

/*
=================
idLexer::ParseBracedSection

The next token should be an open brace.
Parses until a matching close brace is found.
Internal brace depths are properly skipped.
=================
*/
const char* idLexer::ParseBracedSection( idStr& out )
{
	idToken token;
	int i, depth;

	out.Empty();
	if( !idLexer::ExpectTokenString( "{" ) )
	{
		return out.c_str();
	}
	out = "{";
	depth = 1;
	do
	{
		if( !idLexer::ReadToken( &token ) )
		{
			Error( "missing closing brace" );
			return out.c_str();
		}

		// if the token is on a new line
		for( i = 0; i < token.linesCrossed; i++ )
		{
			out += "\r\n";
		}

		if( token.type == TT_PUNCTUATION )
		{
			if( token[0] == '{' )
			{
				depth++;
			}
			else if( token[0] == '}' )
			{
				depth--;
			}
		}

		if( token.type == TT_STRING )
		{
			out += "\"" + token + "\"";
		}
		else
		{
			out += token;
		}
		out += " ";
	}
	while( depth );

	return out.c_str();
}

/*
=================
idLexer::ParseRestOfLine

  parse the rest of the line
=================
*/
const char* idLexer::ParseRestOfLine( idStr& out )
{
	idToken token;

	out.Empty();
	while( idLexer::ReadToken( &token ) )
	{
		if( token.linesCrossed )
		{
			idLexer::script_p = lastScript_p;
			idLexer::line = lastline;
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
========================
idLexer::ParseCompleteLine

Returns a string up to the \n, but doesn't eat any whitespace at the beginning of the next line.
========================
*/
const char* idLexer::ParseCompleteLine( idStr& out )
{
	idToken token;
	const char*	start;

	start = script_p;

	while( 1 )
	{
		// end of buffer
		if( *script_p == 0 )
		{
			break;
		}
		if( *script_p == '\n' )
		{
			line++;
			script_p++;
			break;
		}
		script_p++;
	}

	out.Empty();
	out.Append( start, script_p - start );

	return out.c_str();
}

/*
================
idLexer::GetLastWhiteSpace
================
*/
int idLexer::GetLastWhiteSpace( idStr& whiteSpace ) const
{
	whiteSpace.Clear();
	for( const char* p = whiteSpaceStart_p; p < whiteSpaceEnd_p; p++ )
	{
		whiteSpace.Append( *p );
	}
	return whiteSpace.Length();
}

/*
================
idLexer::GetLastWhiteSpaceStart
================
*/
int idLexer::GetLastWhiteSpaceStart() const
{
	return whiteSpaceStart_p - buffer;
}

/*
================
idLexer::GetLastWhiteSpaceEnd
================
*/
int idLexer::GetLastWhiteSpaceEnd() const
{
	return whiteSpaceEnd_p - buffer;
}

/*
================
idLexer::Reset
================
*/
void idLexer::Reset()
{
	// pointer in script buffer
	idLexer::script_p = idLexer::buffer;
	// pointer in script buffer before reading token
	idLexer::lastScript_p = idLexer::buffer;
	// begin of white space
	idLexer::whiteSpaceStart_p = NULL;
	// end of white space
	idLexer::whiteSpaceEnd_p = NULL;
	// set if there's a token available in idLexer::token
	idLexer::tokenavailable = 0;

	idLexer::line = intialLine;
	idLexer::lastline = intialLine;

	// clear the saved token
	idLexer::token = "";
}

/*
================
idLexer::EndOfFile
================
*/
bool idLexer::EndOfFile()
{
	return idLexer::script_p >= idLexer::end_p;
}

/*
================
idLexer::NumLinesCrossed
================
*/
int idLexer::NumLinesCrossed()
{
	return idLexer::line - idLexer::lastline;
}

/*
================
idLexer::LoadFile
================
*/
int idLexer::LoadFile( const char* filename, bool OSPath )
{
	idFile* fp;
	idStr pathname;
	int length;
	char* buf;

	if( idLexer::loaded )
	{
		idLib::common->Error( "idLexer::LoadFile: another script already loaded" );
		return false;
	}

	if( !OSPath && ( baseFolder[0] != '\0' ) )
	{
		pathname = va( "%s/%s", baseFolder, filename );
	}
	else
	{
		pathname = filename;
	}
	if( OSPath )
	{
		fp = idLib::fileSystem->OpenExplicitFileRead( pathname );
	}
	else
	{
		fp = idLib::fileSystem->OpenFileRead( pathname );
	}
	if( !fp )
	{
		return false;
	}
	length = fp->Length();
	buf = ( char* ) Mem_Alloc( length + 1, TAG_IDLIB_LEXER );
	buf[length] = '\0';
	fp->Read( buf, length );
	idLexer::fileTime = fp->Timestamp();
	idLexer::filename = fp->GetFullPath();
	idLib::fileSystem->CloseFile( fp );

	idLexer::buffer = buf;
	idLexer::length = length;
	// pointer in script buffer
	idLexer::script_p = idLexer::buffer;
	// pointer in script buffer before reading token
	idLexer::lastScript_p = idLexer::buffer;
	// pointer to end of script buffer
	idLexer::end_p = &( idLexer::buffer[length] );

	idLexer::tokenavailable = 0;
	idLexer::line = 1;
	idLexer::line = 1;
	idLexer::lastline = 1;
	idLexer::allocated = true;
	idLexer::loaded = true;

	return true;
}

/*
================
idLexer::LoadMemory
================
*/
int idLexer::LoadMemory( const char* ptr, int length, const char* name, int startLine )
{
	if( idLexer::loaded )
	{
		idLib::common->Error( "idLexer::LoadMemory: another script already loaded" );
		return false;
	}
	idLexer::filename = name;
	idLexer::buffer = ptr;
	idLexer::fileTime = 0;
	idLexer::length = length;
	// pointer in script buffer
	idLexer::script_p = idLexer::buffer;
	// pointer in script buffer before reading token
	idLexer::lastScript_p = idLexer::buffer;
	// pointer to end of script buffer
	idLexer::end_p = &( idLexer::buffer[length] );

	idLexer::tokenavailable = 0;
	idLexer::line = startLine;
	idLexer::lastline = startLine;
	idLexer::intialLine = startLine;
	idLexer::allocated = false;
	idLexer::loaded = true;

	return true;
}

/*
================
idLexer::FreeSource
================
*/
void idLexer::FreeSource()
{
#ifdef PUNCTABLE
	if( idLexer::punctuationtable && idLexer::punctuationtable != default_punctuationtable )
	{
		Mem_Free( ( void* ) idLexer::punctuationtable );
		idLexer::punctuationtable = NULL;
	}
	if( idLexer::nextpunctuation && idLexer::nextpunctuation != default_nextpunctuation )
	{
		Mem_Free( ( void* ) idLexer::nextpunctuation );
		idLexer::nextpunctuation = NULL;
	}
#endif //PUNCTABLE
	if( idLexer::allocated )
	{
		Mem_Free( ( void* ) idLexer::buffer );
		idLexer::buffer = NULL;
		idLexer::allocated = false;
	}
	idLexer::tokenavailable = 0;
	idLexer::token = "";
	idLexer::loaded = false;
}

/*
================
idLexer::idLexer
================
*/
idLexer::idLexer()
{
	idLexer::loaded = false;
	idLexer::filename = "";
	idLexer::flags = 0;
	idLexer::SetPunctuations( NULL );
	idLexer::allocated = false;
	idLexer::fileTime = 0;
	idLexer::length = 0;
	idLexer::line = 0;
	idLexer::lastline = 0;
	idLexer::tokenavailable = 0;
	idLexer::token = "";
	idLexer::next = NULL;
	idLexer::hadError = false;
}

/*
================
idLexer::idLexer
================
*/
idLexer::idLexer( int flags )
{
	idLexer::loaded = false;
	idLexer::filename = "";
	idLexer::flags = flags;
	idLexer::SetPunctuations( NULL );
	idLexer::allocated = false;
	idLexer::fileTime = 0;
	idLexer::length = 0;
	idLexer::line = 0;
	idLexer::lastline = 0;
	idLexer::tokenavailable = 0;
	idLexer::token = "";
	idLexer::next = NULL;
	idLexer::hadError = false;
}

/*
================
idLexer::idLexer
================
*/
idLexer::idLexer( const char* filename, int flags, bool OSPath )
{
	idLexer::loaded = false;
	idLexer::flags = flags;
	idLexer::SetPunctuations( NULL );
	idLexer::allocated = false;
	idLexer::token = "";
	idLexer::next = NULL;
	idLexer::hadError = false;
	idLexer::LoadFile( filename, OSPath );
}

/*
================
idLexer::idLexer
================
*/
idLexer::idLexer( const char* ptr, int length, const char* name, int flags )
{
	idLexer::loaded = false;
	idLexer::flags = flags;
	idLexer::SetPunctuations( NULL );
	idLexer::allocated = false;
	idLexer::token = "";
	idLexer::next = NULL;
	idLexer::hadError = false;
	idLexer::LoadMemory( ptr, length, name );
}

/*
================
idLexer::~idLexer
================
*/
idLexer::~idLexer()
{
	idLexer::FreeSource();
}

/*
================
idLexer::SetBaseFolder
================
*/
void idLexer::SetBaseFolder( const char* path )
{
	idStr::Copynz( baseFolder, path, sizeof( baseFolder ) );
}

/*
================
idLexer::HadError
================
*/
bool idLexer::HadError() const
{
	return hadError;
}

