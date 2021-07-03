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
#ifndef __SCRIPT_COMPILER_H__
#define __SCRIPT_COMPILER_H__

const char* const RESULT_STRING = "<RESULT>";

typedef struct opcode_s
{
	// RB begin
	const char*	name;
	const char*	opname;
	// RB end
	int			priority;
	bool		rightAssociative;
	idVarDef*	type_a;
	idVarDef*	type_b;
	idVarDef*	type_c;
} opcode_t;

// These opcodes are no longer necessary:
// OP_PUSH_OBJ:
// OP_PUSH_OBJENT:

enum
{
	OP_RETURN,

	OP_UINC_F,
	OP_UINCP_F,
	OP_UDEC_F,
	OP_UDECP_F,
	OP_COMP_F,

	OP_MUL_F,
	OP_MUL_V,
	OP_MUL_FV,
	OP_MUL_VF,
	OP_DIV_F,
	OP_MOD_F,
	OP_ADD_F,
	OP_ADD_V,
	OP_ADD_S,
	OP_ADD_FS,
	OP_ADD_SF,
	OP_ADD_VS,
	OP_ADD_SV,
	OP_SUB_F,
	OP_SUB_V,

	OP_EQ_F,
	OP_EQ_V,
	OP_EQ_S,
	OP_EQ_E,
	OP_EQ_EO,
	OP_EQ_OE,
	OP_EQ_OO,

	OP_NE_F,
	OP_NE_V,
	OP_NE_S,
	OP_NE_E,
	OP_NE_EO,
	OP_NE_OE,
	OP_NE_OO,

	OP_LE,
	OP_GE,
	OP_LT,
	OP_GT,

	OP_INDIRECT_F,
	OP_INDIRECT_V,
	OP_INDIRECT_S,
	OP_INDIRECT_ENT,
	OP_INDIRECT_BOOL,
	OP_INDIRECT_OBJ,

	OP_ADDRESS,

	OP_EVENTCALL,
	OP_OBJECTCALL,
	OP_SYSCALL,

	OP_STORE_F,
	OP_STORE_V,
	OP_STORE_S,
	OP_STORE_ENT,
	OP_STORE_BOOL,
	OP_STORE_OBJENT,
	OP_STORE_OBJ,
	OP_STORE_ENTOBJ,

	OP_STORE_FTOS,
	OP_STORE_BTOS,
	OP_STORE_VTOS,
	OP_STORE_FTOBOOL,
	OP_STORE_BOOLTOF,

	OP_STOREP_F,
	OP_STOREP_V,
	OP_STOREP_S,
	OP_STOREP_ENT,
	OP_STOREP_FLD,
	OP_STOREP_BOOL,
	OP_STOREP_OBJ,
	OP_STOREP_OBJENT,

	OP_STOREP_FTOS,
	OP_STOREP_BTOS,
	OP_STOREP_VTOS,
	OP_STOREP_FTOBOOL,
	OP_STOREP_BOOLTOF,

	OP_UMUL_F,
	OP_UMUL_V,
	OP_UDIV_F,
	OP_UDIV_V,
	OP_UMOD_F,
	OP_UADD_F,
	OP_UADD_V,
	OP_USUB_F,
	OP_USUB_V,
	OP_UAND_F,
	OP_UOR_F,

	OP_NOT_BOOL,
	OP_NOT_F,
	OP_NOT_V,
	OP_NOT_S,
	OP_NOT_ENT,

	OP_NEG_F,
	OP_NEG_V,

	OP_INT_F,
	OP_IF,
	OP_IFNOT,

	OP_CALL,
	OP_THREAD,
	OP_OBJTHREAD,

	OP_PUSH_F,
	OP_PUSH_V,
	OP_PUSH_S,
	OP_PUSH_ENT,
	OP_PUSH_OBJ,
	OP_PUSH_OBJENT,
	OP_PUSH_FTOS,
	OP_PUSH_BTOF,
	OP_PUSH_FTOB,
	OP_PUSH_VTOS,
	OP_PUSH_BTOS,

	OP_GOTO,

	OP_AND,
	OP_AND_BOOLF,
	OP_AND_FBOOL,
	OP_AND_BOOLBOOL,
	OP_OR,
	OP_OR_BOOLF,
	OP_OR_FBOOL,
	OP_OR_BOOLBOOL,

	OP_BITAND,
	OP_BITOR,

	OP_BREAK,			// placeholder op.  not used in final code
	OP_CONTINUE,		// placeholder op.  not used in final code

	NUM_OPCODES
};

class idCompiler
{
private:
	static bool		punctuationValid[ 256 ];
	// RB begin
	static const char*		punctuation[];
	// RB end

	idParser		parser;
	idParser*		parserPtr;
	idToken			token;

	idTypeDef*		immediateType;
	eval_t			immediate;

	bool			eof;
	bool			console;
	bool			callthread;
	int				braceDepth;
	int				loopDepth;
	int				currentLineNumber;
	int				currentFileNumber;
	int				errorCount;

	idVarDef*		scope;				// the function being parsed, or NULL
	const idVarDef*	basetype;			// for accessing fields

	float			Divide( float numerator, float denominator );
	void			Error( VERIFY_FORMAT_STRING const char* error, ... ) const;
	void			Warning( VERIFY_FORMAT_STRING const char* message, ... ) const;
	idVarDef*		OptimizeOpcode( const opcode_t* op, idVarDef* var_a, idVarDef* var_b );
	idVarDef*		EmitOpcode( const opcode_t* op, idVarDef* var_a, idVarDef* var_b );
	idVarDef*		EmitOpcode( int op, idVarDef* var_a, idVarDef* var_b );
	bool			EmitPush( idVarDef* expression, const idTypeDef* funcArg );
	void			NextToken();
	void			ExpectToken( const char* string );
	bool			CheckToken( const char* string );
	void			ParseName( idStr& name );
	void			SkipOutOfFunction();
	void			SkipToSemicolon();
	idTypeDef*		CheckType();
	idTypeDef*		ParseType();
	idVarDef*		FindImmediate( const idTypeDef* type, const eval_t* eval, const char* string ) const;
	idVarDef*		GetImmediate( idTypeDef* type, const eval_t* eval, const char* string );
	idVarDef*		VirtualFunctionConstant( idVarDef* func );
	idVarDef*		SizeConstant( int size );
	idVarDef*		JumpConstant( int value );
	idVarDef*		JumpDef( int jumpfrom, int jumpto );
	idVarDef*		JumpTo( int jumpto );
	idVarDef*		JumpFrom( int jumpfrom );
	idVarDef*		ParseImmediate();
	idVarDef*		EmitFunctionParms( int op, idVarDef* func, int startarg, int startsize, idVarDef* object );
	idVarDef*		ParseFunctionCall( idVarDef* func );
	idVarDef*		ParseObjectCall( idVarDef* object, idVarDef* func );
	idVarDef*		ParseEventCall( idVarDef* object, idVarDef* func );
	idVarDef*		ParseSysObjectCall( idVarDef* func );
	idVarDef*		LookupDef( const char* name, const idVarDef* baseobj );
	idVarDef*		ParseValue();
	idVarDef*		GetTerm();
	bool			TypeMatches( etype_t type1, etype_t type2 ) const;
	idVarDef*		GetExpression( int priority );
	idTypeDef*		GetTypeForEventArg( char argType );
	void			PatchLoop( int start, int continuePos );
	void			ParseReturnStatement();
	void			ParseWhileStatement();
	void			ParseForStatement();
	void			ParseDoWhileStatement();
	void			ParseIfStatement();
	void			ParseStatement();
	void			ParseObjectDef( const char* objname );
	idTypeDef*		ParseFunction( idTypeDef* returnType, const char* name );
	void			ParseFunctionDef( idTypeDef* returnType, const char* name );
	void			ParseVariableDef( idTypeDef* type, const char* name );
	void			ParseEventDef( idTypeDef* type, const char* name );
	void			ParseDefs();
	void			ParseNamespace( idVarDef* newScope );

public :
	// RB: added const
	static const opcode_t	opcodes[];
	// RB end

	idCompiler();
	void			CompileFile( const char* text, const char* filename, bool console );
};

#endif /* !__SCRIPT_COMPILER_H__ */
