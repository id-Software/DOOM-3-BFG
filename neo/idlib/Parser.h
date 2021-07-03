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

#ifndef __PARSER_H__
#define __PARSER_H__

/*
===============================================================================

	C/C++ compatible pre-compiler

===============================================================================
*/

#define DEFINE_FIXED			0x0001

#define BUILTIN_LINE			1
#define BUILTIN_FILE			2
#define BUILTIN_DATE			3
#define BUILTIN_TIME			4
#define BUILTIN_STDC			5

#define INDENT_IF				0x0001
#define INDENT_ELSE				0x0002
#define INDENT_ELIF				0x0004
#define INDENT_IFDEF			0x0008
#define INDENT_IFNDEF			0x0010

// macro definitions
typedef struct define_s
{
	char* 			name;						// define name
	int				flags;						// define flags
	int				builtin;					// > 0 if builtin define
	int				numparms;					// number of define parameters
	idToken* 		parms;						// define parameters
	idToken* 		tokens;						// macro tokens (possibly containing parm tokens)
	struct define_s*	next;						// next defined macro in a list
	struct define_s*	hashnext;					// next define in the hash chain
} define_t;

// indents used for conditional compilation directives:
// #if, #else, #elif, #ifdef, #ifndef
typedef struct indent_s
{
	int				type;						// indent type
	int				skip;						// true if skipping current indent
	idLexer* 		script;						// script the indent was in
	struct indent_s*	next;						// next indent on the indent stack
} indent_t;


class idParser
{

public:
	// constructor
	idParser();
	idParser( int flags );
	idParser( const char* filename, int flags = 0, bool OSPath = false );
	idParser( const char* ptr, int length, const char* name, int flags = 0 );
	// destructor
	~idParser();
	// load a source file
	int				LoadFile( const char* filename, bool OSPath = false );
	// load a source from the given memory with the given length
	// NOTE: the ptr is expected to point at a valid C string: ptr[length] == '\0'
	int				LoadMemory( const char* ptr, int length, const char* name );
	// free the current source
	void			FreeSource( bool keepDefines = false );
	// returns true if a source is loaded
	int				IsLoaded() const
	{
		return idParser::loaded;
	}
	// read a token from the source
	int				ReadToken( idToken* token );
	// expect a certain token, reads the token when available
	int				ExpectTokenString( const char* string );
	// expect a certain token type
	int				ExpectTokenType( int type, int subtype, idToken* token );
	// expect a token
	int				ExpectAnyToken( idToken* token );
	// returns true if the next token equals the given string and removes the token from the source
	int				CheckTokenString( const char* string );
	// returns true if the next token equals the given type and removes the token from the source
	int				CheckTokenType( int type, int subtype, idToken* token );
	// returns true if the next token equals the given string but does not remove the token from the source
	int				PeekTokenString( const char* string );
	// returns true if the next token equals the given type but does not remove the token from the source
	int				PeekTokenType( int type, int subtype, idToken* token );
	// skip tokens until the given token string is read
	int				SkipUntilString( const char* string );
	// skip the rest of the current line
	int				SkipRestOfLine();
	// skip the braced section
	int				SkipBracedSection( bool parseFirstBrace = true );
	// parse a braced section into a string
	const char*		ParseBracedSection( idStr& out, int tabs, bool parseFirstBrace, char intro, char outro );
	// parse a braced section into a string, maintaining indents and newlines
	const char* 	ParseBracedSectionExact( idStr& out, int tabs = -1 );
	// parse the rest of the line
	const char* 	ParseRestOfLine( idStr& out );
	// unread the given token
	void			UnreadToken( idToken* token );
	// read a token only if on the current line
	int				ReadTokenOnLine( idToken* token );
	// read a signed integer
	int				ParseInt();
	// read a boolean
	bool			ParseBool();
	// read a floating point number
	float			ParseFloat();
	// parse matrices with floats
	int				Parse1DMatrix( int x, float* m );
	int				Parse2DMatrix( int y, int x, float* m );
	int				Parse3DMatrix( int z, int y, int x, float* m );
	// get the white space before the last read token
	int				GetLastWhiteSpace( idStr& whiteSpace ) const;
	// Set a marker in the source file (there is only one marker)
	void			SetMarker();
	// Get the string from the marker to the current position
	void			GetStringFromMarker( idStr& out, bool clean = false );
	// add a define to the source
	int				AddDefine( const char* string );
	// add builtin defines
	void			AddBuiltinDefines();
	// set the source include path
	void			SetIncludePath( const char* path );
	// set the punctuation set
	void			SetPunctuations( const punctuation_t* p );
	// returns a pointer to the punctuation with the given id
	const char* 	GetPunctuationFromId( int id );
	// get the id for the given punctuation
	int				GetPunctuationId( const char* p );
	// set lexer flags
	void			SetFlags( int flags );
	// get lexer flags
	int				GetFlags() const;
	// returns the current filename
	const char* 	GetFileName() const;
	// get current offset in current script
	const int		GetFileOffset() const;
	// get file time for current script
	const ID_TIME_T	GetFileTime() const;
	// returns the current line number
	const int		GetLineNum() const;
	// print an error message
	void			Error( VERIFY_FORMAT_STRING const char* str, ... ) const;
	// print a warning message
	void			Warning( VERIFY_FORMAT_STRING const char* str, ... ) const;
	// returns true if at the end of the file
	bool			EndOfFile();
	// add a global define that will be added to all opened sources
	static int		AddGlobalDefine( const char* string );
	// remove the given global define
	static int		RemoveGlobalDefine( const char* name );
	// remove all global defines
	static void		RemoveAllGlobalDefines();
	// set the base folder to load files from
	static void		SetBaseFolder( const char* path );

// RB: made protected to have custom #include behaviours for embedded resources
protected:
// RB end
	int				loaded;						// set when a source file is loaded from file or memory
	idStr			filename;					// file name of the script
	idStr			includepath;				// path to include files
	bool			OSPath;						// true if the file was loaded from an OS path
	const punctuation_t* punctuations;			// punctuations to use
	int				flags;						// flags used for script parsing
	idLexer* 		scriptstack;				// stack with scripts of the source
	idToken* 		tokens;						// tokens to read first
	define_t* 		defines;					// list with macro definitions
	define_t** 		definehash;					// hash chain with defines
	indent_t* 		indentstack;				// stack with indents
	int				skip;						// > 0 if skipping conditional code
	const char*		marker_p;

	static define_t* globaldefines;				// list with global defines added to every source loaded

	void			PushIndent( int type, int skip );
	void			PopIndent( int* type, int* skip );
	void			PushScript( idLexer* script );
	int				ReadSourceToken( idToken* token );
	int				ReadLine( idToken* token );
	int				UnreadSourceToken( idToken* token );
	int				ReadDefineParms( define_t* define, idToken** parms, int maxparms );
	int				StringizeTokens( idToken* tokens, idToken* token );
	int				MergeTokens( idToken* t1, idToken* t2 );
	int				ExpandBuiltinDefine( idToken* deftoken, define_t* define, idToken** firsttoken, idToken** lasttoken );
	int				ExpandDefine( idToken* deftoken, define_t* define, idToken** firsttoken, idToken** lasttoken );
	int				ExpandDefineIntoSource( idToken* deftoken, define_t* define );
	void			AddGlobalDefinesToSource();
	define_t* 		CopyDefine( define_t* define );
	define_t* 		FindHashedDefine( define_t** definehash, const char* name );
	int				FindDefineParm( define_t* define, const char* name );
	void			AddDefineToHash( define_t* define, define_t** definehash );
	static void		PrintDefine( define_t* define );
	static void		FreeDefine( define_t* define );
	static define_t* FindDefine( define_t* defines, const char* name );
	static define_t* DefineFromString( const char* string );
	define_t* 		CopyFirstDefine();
	// RB: allow override
	virtual int		Directive_include( idToken* token, bool supressWarning = false );
	// RB end
	int				Directive_undef();
	int				Directive_if_def( int type );
	int				Directive_ifdef();
	int				Directive_ifndef();
	int				Directive_else();
	int				Directive_endif();
	// DG: use int instead of long for 64bit compatibility
	int				EvaluateTokens( idToken* tokens, signed int* intvalue, double* floatvalue, int integer );
	int				Evaluate( signed int* intvalue, double* floatvalue, int integer );
	int				DollarEvaluate( signed int* intvalue, double* floatvalue, int integer );
	// DG end
	int				Directive_define();
	int				Directive_elif();
	int				Directive_if();
	int				Directive_line();
	int				Directive_error();
	int				Directive_warning();
	int				Directive_pragma();
	void			UnreadSignToken();
	int				Directive_eval();
	int				Directive_evalfloat();
	int				ReadDirective();
	int				DollarDirective_evalint();
	int				DollarDirective_evalfloat();
	int				ReadDollarDirective();
};

ID_INLINE const char* idParser::GetFileName() const
{
	if( idParser::scriptstack )
	{
		return idParser::scriptstack->GetFileName();
	}
	else
	{
		return "";
	}
}

ID_INLINE const int idParser::GetFileOffset() const
{
	if( idParser::scriptstack )
	{
		return idParser::scriptstack->GetFileOffset();
	}
	else
	{
		return 0;
	}
}

ID_INLINE const ID_TIME_T idParser::GetFileTime() const
{
	if( idParser::scriptstack )
	{
		return idParser::scriptstack->GetFileTime();
	}
	else
	{
		return 0;
	}
}

ID_INLINE const int idParser::GetLineNum() const
{
	if( idParser::scriptstack )
	{
		return idParser::scriptstack->GetLineNum();
	}
	else
	{
		return 0;
	}
}

#endif /* !__PARSER_H__ */
