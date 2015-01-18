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
#pragma hdrstop
#include "precompiled.h"

idCVar swf_debug( "swf_debug", "0", CVAR_INTEGER | CVAR_ARCHIVE, "debug swf scripts.  1 shows traces/errors.  2 also shows warnings.  3 also shows disassembly.  4 shows parameters in the disassembly." );
idCVar swf_debugInvoke( "swf_debugInvoke", "0", CVAR_INTEGER, "debug swf functions being called from game." );

idSWFConstantPool::idSWFConstantPool()
{
}


/*
========================
idSWFConstantPool::Clear
========================
*/
void idSWFConstantPool::Clear()
{
	for( int i = 0; i < pool.Num(); i++ )
	{
		pool[i]->Release();
	}
	pool.Clear();
}

/*
========================
idSWFConstantPool::Copy
========================
*/
void idSWFConstantPool::Copy( const idSWFConstantPool& other )
{
	Clear();
	pool.SetNum( other.pool.Num() );
	for( int i = 0; i < pool.Num(); i++ )
	{
		pool[i] = other.pool[i];
		pool[i]->AddRef();
	}
}

/*
========================
idSWFScriptFunction_Script::~idSWFScriptFunction_Script
========================
*/
idSWFScriptFunction_Script::~idSWFScriptFunction_Script()
{
	for( int i = 0; i < scope.Num(); i++ )
	{
		if( verify( scope[i] ) )
		{
			scope[i]->Release();
		}
	}
	if( prototype )
	{
		prototype->Release();
	}
}

/*
========================
idSWFScriptFunction_Script::Call
========================
*/
void idSWFScriptFunction_Script::SetScope( idList<idSWFScriptObject*>& newScope )
{
	assert( scope.Num() == 0 );
	for( int i = 0; i < scope.Num(); i++ )
	{
		if( verify( scope[i] ) )
		{
			scope[i]->Release();
		}
	}
	scope.Clear();
	scope.Append( newScope );
	for( int i = 0; i < newScope.Num(); i++ )
	{
		if( verify( scope[i] ) )
		{
			scope[i]->AddRef();
		}
	}
}

/*
========================
idSWFScriptFunction_Script::Call
========================
*/
idSWFScriptVar idSWFScriptFunction_Script::Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
{
	idSWFBitStream bitstream( data, length, false );
	
	// We assume scope[0] is the global scope
	assert( scope.Num() > 0 );
	
	if( thisObject == NULL )
	{
		thisObject = scope[0];
	}
	
	idSWFScriptObject* locals = idSWFScriptObject::Alloc();
	
	idSWFStack stack;
	stack.SetNum( parms.Num() + 1 );
	for( int i = 0; i < parms.Num(); i++ )
	{
		stack[ parms.Num() - i - 1 ] = parms[i];
		
		// Unfortunately at this point we don't have the function name anymore, so our warning messages aren't very detailed
		if( i < parameters.Num() )
		{
			if( parameters[i].reg > 0 && parameters[i].reg < registers.Num() )
			{
				registers[ parameters[i].reg ] = parms[i];
			}
			locals->Set( parameters[i].name, parms[i] );
		}
	}
	// Set any additional parameters to undefined
	for( int i = parms.Num(); i < parameters.Num(); i++ )
	{
		if( parameters[i].reg > 0 && parameters[i].reg < registers.Num() )
		{
			registers[ parameters[i].reg ].SetUndefined();
		}
		locals->Set( parameters[i].name, idSWFScriptVar() );
	}
	stack.A().SetInteger( parms.Num() );
	
	int preloadReg = 1;
	if( flags & BIT( 0 ) )
	{
		// load "this" into a register
		registers[ preloadReg ].SetObject( thisObject );
		preloadReg++;
	}
	if( ( flags & BIT( 1 ) ) == 0 )
	{
		// create "this"
		locals->Set( "this", idSWFScriptVar( thisObject ) );
	}
	if( flags & BIT( 2 ) )
	{
		idSWFScriptObject* arguments = idSWFScriptObject::Alloc();
		// load "arguments" into a register
		arguments->MakeArray();
		
		int numElements = parms.Num();
		
		for( int i = 0; i < numElements; i++ )
		{
			arguments->Set( i, parms[i] );
		}
		
		registers[ preloadReg ].SetObject( arguments );
		preloadReg++;
		
		arguments->Release();
	}
	if( ( flags & BIT( 3 ) ) == 0 )
	{
		idSWFScriptObject* arguments = idSWFScriptObject::Alloc();
		
		// create "arguments"
		arguments->MakeArray();
		
		int numElements = parms.Num();
		
		for( int i = 0; i < numElements; i++ )
		{
			arguments->Set( i, parms[i] );
		}
		
		locals->Set( "arguments", idSWFScriptVar( arguments ) );
		
		arguments->Release();
	}
	if( flags & BIT( 4 ) )
	{
		// load "super" into a register
		registers[ preloadReg ].SetObject( thisObject->GetPrototype() );
		preloadReg++;
	}
	if( ( flags & BIT( 5 ) ) == 0 )
	{
		// create "super"
		locals->Set( "super", idSWFScriptVar( thisObject->GetPrototype() ) );
	}
	if( flags & BIT( 6 ) )
	{
		// preload _root
		registers[ preloadReg ] = scope[0]->Get( "_root" );
		preloadReg++;
	}
	if( flags & BIT( 7 ) )
	{
		// preload _parent
		if( thisObject->GetSprite() != NULL && thisObject->GetSprite()->parent != NULL )
		{
			registers[ preloadReg ].SetObject( thisObject->GetSprite()->parent->scriptObject );
		}
		else
		{
			registers[ preloadReg ].SetNULL();
		}
		preloadReg++;
	}
	if( flags & BIT( 8 ) )
	{
		// load "_global" into a register
		registers[ preloadReg ].SetObject( scope[0] );
		preloadReg++;
	}
	
	int scopeSize = scope.Num();
	scope.Append( locals );
	locals->AddRef();
	
	idSWFScriptVar retVal = Run( thisObject, stack, bitstream );
	
	assert( scope.Num() == scopeSize + 1 );
	for( int i = scopeSize; i < scope.Num(); i++ )
	{
		if( verify( scope[i] ) )
		{
			scope[i]->Release();
		}
	}
	scope.SetNum( scopeSize );
	
	locals->Release();
	locals = NULL;
	
	return retVal;
}

/*
========================
<anonymous>::Split
========================
*/
namespace
{
const char* GetPropertyName( int index )
{
	switch( index )
	{
		case 0:
			return "_x";
		case 1:
			return "_y";
		case 2:
			return "_xscale";
		case 3:
			return "_yscale";
		case 4:
			return "_currentframe";
		case 5:
			return "_totalframes";
		case 6:
			return "_alpha";
		case 7:
			return "_visible";
		case 8:
			return "_width";
		case 9:
			return "_height";
		case 10:
			return "_rotation";
		case 11:
			return "_target";
		case 12:
			return "_framesloaded";
		case 13:
			return "_name";
		case 14:
			return "_droptarget";
		case 15:
			return "_url";
		case 16:
			return "_highquality";
		case 17:
			return "_focusrect";
		case 18:
			return "_soundbuftime";
		case 19:
			return "_quality";
		case 20:
			return "_mousex";
		case 21:
			return "_mousey";
	}
	return "";
}

const char* GetSwfActionName( swfAction_t code )
{
	switch( code )
	{
		case Action_End:
			return "Action_End";
			
		// swf 3
		case Action_NextFrame:
			return "Action_NextFrame";
		case Action_PrevFrame:
			return "Action_PrevFrame";
		case Action_Play:
			return "Action_Play";
		case Action_Stop:
			return "Action_Stop";
		case Action_ToggleQuality:
			return "Action_ToggleQuality";
		case Action_StopSounds:
			return "Action_StopSounds";
			
		case Action_GotoFrame:
			return "Action_GotoFrame";
		case Action_GetURL:
			return "Action_GetURL";
		case Action_WaitForFrame:
			return "Action_WaitForFrame";
		case Action_SetTarget:
			return "Action_SetTarget";
		case Action_GoToLabel:
			return "Action_GoToLabel";
			
		// swf 4
		case Action_Add:
			return "Action_Add";
		case Action_Subtract:
			return "Action_Subtract";
		case Action_Multiply:
			return "Action_Multiply";
		case Action_Divide:
			return "Action_Divide";
		case Action_Equals:
			return "Action_Equals";
		case Action_Less:
			return "Action_Less";
		case Action_And:
			return "Action_And";
		case Action_Or:
			return "Action_Or";
		case Action_Not:
			return "Action_Not";
		case Action_StringEquals:
			return "Action_StringEquals";
		case Action_StringLength:
			return "Action_StringLength";
		case Action_StringExtract:
			return "Action_StringExtract";
		case Action_Pop:
			return "Action_Pop";
		case Action_ToInteger:
			return "Action_ToInteger";
		case Action_GetVariable:
			return "Action_GetVariable";
		case Action_SetVariable:
			return "Action_SetVariable";
		case Action_SetTarget2:
			return "Action_SetTarget2";
		case Action_StringAdd:
			return "Action_StringAdd";
		case Action_GetProperty:
			return "Action_GetProperty";
		case Action_SetProperty:
			return "Action_SetProperty";
		case Action_CloneSprite:
			return "Action_CloneSprite";
		case Action_RemoveSprite:
			return "Action_RemoveSprite";
		case Action_Trace:
			return "Action_Trace";
		case Action_StartDrag:
			return "Action_StartDrag";
		case Action_EndDrag:
			return "Action_EndDrag";
		case Action_StringLess:
			return "Action_StringLess";
		case Action_RandomNumber:
			return "Action_RandomNumber";
		case Action_MBStringLength:
			return "Action_MBStringLength";
		case Action_CharToAscii:
			return "Action_CharToAscii";
		case Action_AsciiToChar:
			return "Action_AsciiToChar";
		case Action_GetTime:
			return "Action_GetTime";
		case Action_MBStringExtract:
			return "Action_MBStringExtract";
		case Action_MBCharToAscii:
			return "Action_MBCharToAscii";
		case Action_MBAsciiToChar:
			return "Action_MBAsciiToChar";
			
		case Action_WaitForFrame2:
			return "Action_WaitForFrame2";
		case Action_Push:
			return "Action_Push";
		case Action_Jump:
			return "Action_Jump";
		case Action_GetURL2:
			return "Action_GetURL2";
		case Action_If:
			return "Action_If";
		case Action_Call:
			return "Action_Call";
		case Action_GotoFrame2:
			return "Action_GotoFrame2";
			
		// swf 5
		case Action_Delete:
			return "Action_Delete";
		case Action_Delete2:
			return "Action_Delete2";
		case Action_DefineLocal:
			return "Action_DefineLocal";
		case Action_CallFunction:
			return "Action_CallFunction";
		case Action_Return:
			return "Action_Return";
		case Action_Modulo:
			return "Action_Modulo";
		case Action_NewObject:
			return "Action_NewObject";
		case Action_DefineLocal2:
			return "Action_DefineLocal2";
		case Action_InitArray:
			return "Action_InitArray";
		case Action_InitObject:
			return "Action_InitObject";
		case Action_TypeOf:
			return "Action_TypeOf";
		case Action_TargetPath:
			return "Action_TargetPath";
		case Action_Enumerate:
			return "Action_Enumerate";
		case Action_Add2:
			return "Action_Add2";
		case Action_Less2:
			return "Action_Less2";
		case Action_Equals2:
			return "Action_Equals2";
		case Action_ToNumber:
			return "Action_ToNumber";
		case Action_ToString:
			return "Action_ToString";
		case Action_PushDuplicate:
			return "Action_PushDuplicate";
		case Action_StackSwap:
			return "Action_StackSwap";
		case Action_GetMember:
			return "Action_GetMember";
		case Action_SetMember:
			return "Action_SetMember";
		case Action_Increment:
			return "Action_Increment";
		case Action_Decrement:
			return "Action_Decrement";
		case Action_CallMethod:
			return "Action_CallMethod";
		case Action_NewMethod:
			return "Action_NewMethod";
		case Action_BitAnd:
			return "Action_BitAnd";
		case Action_BitOr:
			return "Action_BitOr";
		case Action_BitXor:
			return "Action_BitXor";
		case Action_BitLShift:
			return "Action_BitLShift";
		case Action_BitRShift:
			return "Action_BitRShift";
		case Action_BitURShift:
			return "Action_BitURShift";
			
		case Action_StoreRegister:
			return "Action_StoreRegister";
		case Action_ConstantPool:
			return "Action_ConstantPool";
		case Action_With:
			return "Action_With";
		case Action_DefineFunction:
			return "Action_DefineFunction";
			
		// swf 6
		case Action_InstanceOf:
			return "Action_InstanceOf";
		case Action_Enumerate2:
			return "Action_Enumerate2";
		case Action_StrictEquals:
			return "Action_StrictEquals";
		case Action_Greater:
			return "Action_Greater";
		case Action_StringGreater:
			return "Action_StringGreater";
			
		// swf 7
		case Action_Extends:
			return "Action_Extends";
		case Action_CastOp:
			return "Action_CastOp";
		case Action_ImplementsOp:
			return "Action_ImplementsOp";
		case Action_Throw:
			return "Action_Throw";
		case Action_Try:
			return "Action_Try";
			
		case Action_DefineFunction2:
			return "Action_DefineFunction2";
		default:
			return "UNKNOWN CODE";
	}
}
}

/*
========================
idSWFScriptFunction_Script::Run
========================
*/
idSWFScriptVar idSWFScriptFunction_Script::Run( idSWFScriptObject* thisObject, idSWFStack& stack, idSWFBitStream& bitstream )
{
	static int callstackLevel = -1;
	idSWFSpriteInstance* thisSprite = thisObject->GetSprite();
	idSWFSpriteInstance* currentTarget = thisSprite;
	
	if( currentTarget == NULL )
	{
		thisSprite = currentTarget = defaultSprite;
	}
	
	callstackLevel++;
	
	while( bitstream.Tell() < bitstream.Length() )
	{
		swfAction_t code = ( swfAction_t )bitstream.ReadU8();
		uint16 recordLength = 0;
		if( code >= 0x80 )
		{
			recordLength = bitstream.ReadU16();
		}
		
		if( swf_debug.GetInteger() >= 3 )
		{
			// stack[0] is always 0 so don't read it
			if( swf_debug.GetInteger() >= 4 )
			{
				for( int i = stack.Num() - 1; i >= 0 ; i-- )
				{
					idLib::Printf( "  %c: %s (%s)\n", ( char )( 64 + stack.Num() - i ), stack[i].ToString().c_str(), stack[i].TypeOf() );
				}
				
				for( int i = 0; i < registers.Num(); i++ )
				{
					if( !registers[i].IsUndefined() )
					{
						idLib::Printf( " R%d: %s (%s)\n", i, registers[i].ToString().c_str(), registers[i].TypeOf() );
					}
				}
			}
			
			idLib::Printf( "SWF%d: code %s\n", callstackLevel, GetSwfActionName( code ) );
		}
		
		switch( code )
		{
			case Action_Return:
				callstackLevel--;
				return stack.A();
			case Action_End:
				callstackLevel--;
				return idSWFScriptVar();
			case Action_NextFrame:
				if( verify( currentTarget != NULL ) )
				{
					currentTarget->NextFrame();
				}
				else if( swf_debug.GetInteger() > 0 )
				{
					idLib::Printf( "SWF: no target movie clip for nextFrame\n" );
				}
				break;
			case Action_PrevFrame:
				if( verify( currentTarget != NULL ) )
				{
					currentTarget->PrevFrame();
				}
				else if( swf_debug.GetInteger() > 0 )
				{
					idLib::Printf( "SWF: no target movie clip for prevFrame\n" );
				}
				break;
			case Action_Play:
				if( verify( currentTarget != NULL ) )
				{
					currentTarget->Play();
				}
				else if( swf_debug.GetInteger() > 0 )
				{
					idLib::Printf( "SWF: no target movie clip for play\n" );
				}
				break;
			case Action_Stop:
				if( verify( currentTarget != NULL ) )
				{
					currentTarget->Stop();
				}
				else if( swf_debug.GetInteger() > 0 )
				{
					idLib::Printf( "SWF: no target movie clip for stop\n" );
				}
				break;
			case Action_ToggleQuality:
				break;
			case Action_StopSounds:
				break;
			case Action_GotoFrame:
			{
				assert( recordLength == 2 );
				int frameNum = bitstream.ReadU16() + 1;
				if( verify( currentTarget != NULL ) )
				{
					currentTarget->RunTo( frameNum );
				}
				else if( swf_debug.GetInteger() > 0 )
				{
					idLib::Printf( "SWF: no target movie clip for runTo %d\n", frameNum );
				}
				break;
			}
			case Action_SetTarget:
			{
				const char* targetName = ( const char* )bitstream.ReadData( recordLength );
				if( verify( thisSprite != NULL ) )
				{
					currentTarget = thisSprite->ResolveTarget( targetName );
				}
				else if( swf_debug.GetInteger() > 0 )
				{
					idLib::Printf( "SWF: no target movie clip for setTarget %s\n", targetName );
				}
				break;
			}
			case Action_GoToLabel:
			{
				const char* targetName = ( const char* )bitstream.ReadData( recordLength );
				if( verify( currentTarget != NULL ) )
				{
					currentTarget->RunTo( currentTarget->FindFrame( targetName ) );
				}
				else if( swf_debug.GetInteger() > 0 )
				{
					idLib::Printf( "SWF: no target movie clip for runTo %s\n", targetName );
				}
				break;
			}
			case Action_Push:
			{
				idSWFBitStream pushstream( bitstream.ReadData( recordLength ), recordLength, false );
				while( pushstream.Tell() < pushstream.Length() )
				{
					uint8 type = pushstream.ReadU8();
					switch( type )
					{
						case 0:
							stack.Alloc().SetString( pushstream.ReadString() );
							break;
						case 1:
							stack.Alloc().SetFloat( pushstream.ReadFloat() );
							break;
						case 2:
							stack.Alloc().SetNULL();
							break;
						case 3:
							stack.Alloc().SetUndefined();
							break;
						case 4:
							stack.Alloc() = registers[ pushstream.ReadU8() ];
							break;
						case 5:
							stack.Alloc().SetBool( pushstream.ReadU8() != 0 );
							break;
						case 6:
							stack.Alloc().SetFloat( ( float )pushstream.ReadDouble() );
							break;
						case 7:
							stack.Alloc().SetInteger( pushstream.ReadS32() );
							break;
						case 8:
							stack.Alloc().SetString( constants.Get( pushstream.ReadU8() ) );
							break;
						case 9:
							stack.Alloc().SetString( constants.Get( pushstream.ReadU16() ) );
							break;
					}
				}
				break;
			}
			case Action_Pop:
				stack.Pop( 1 );
				break;
			case Action_Add:
				stack.B().SetFloat( stack.B().ToFloat() + stack.A().ToFloat() );
				stack.Pop( 1 );
				break;
			case Action_Subtract:
				stack.B().SetFloat( stack.B().ToFloat() - stack.A().ToFloat() );
				stack.Pop( 1 );
				break;
			case Action_Multiply:
				stack.B().SetFloat( stack.B().ToFloat() * stack.A().ToFloat() );
				stack.Pop( 1 );
				break;
			case Action_Divide:
				stack.B().SetFloat( stack.B().ToFloat() / stack.A().ToFloat() );
				stack.Pop( 1 );
				break;
			case Action_Equals:
				stack.B().SetBool( stack.B().ToFloat() == stack.A().ToFloat() );
				stack.Pop( 1 );
				break;
			case Action_Less:
				stack.B().SetBool( stack.B().ToFloat() < stack.A().ToFloat() );
				stack.Pop( 1 );
				break;
			case Action_And:
				stack.B().SetBool( stack.B().ToBool() && stack.A().ToBool() );
				stack.Pop( 1 );
				break;
			case Action_Or:
				stack.B().SetBool( stack.B().ToBool() || stack.A().ToBool() );
				stack.Pop( 1 );
				break;
			case Action_Not:
				stack.A().SetBool( !stack.A().ToBool() );
				break;
			case Action_StringEquals:
				stack.B().SetBool( stack.B().ToString() == stack.A().ToString() );
				stack.Pop( 1 );
				break;
			case Action_StringLength:
				stack.A().SetInteger( stack.A().ToString().Length() );
				break;
			case Action_StringAdd:
				stack.B().SetString( stack.B().ToString() + stack.A().ToString() );
				stack.Pop( 1 );
				break;
			case Action_StringExtract:
				stack.C().SetString( stack.C().ToString().Mid( stack.B().ToInteger(), stack.A().ToInteger() ) );
				stack.Pop( 2 );
				break;
			case Action_StringLess:
				stack.B().SetBool( stack.B().ToString() < stack.A().ToString() );
				stack.Pop( 1 );
				break;
			case Action_StringGreater:
				stack.B().SetBool( stack.B().ToString() > stack.A().ToString() );
				stack.Pop( 1 );
				break;
			case Action_ToInteger:
				stack.A().SetInteger( stack.A().ToInteger() );
				break;
			case Action_CharToAscii:
				stack.A().SetInteger( stack.A().ToString()[0] );
				break;
			case Action_AsciiToChar:
				stack.A().SetString( va( "%c", stack.A().ToInteger() ) );
				break;
			case Action_Jump:
				bitstream.Seek( bitstream.ReadS16() );
				break;
			case Action_If:
			{
				int16 offset = bitstream.ReadS16();
				if( stack.A().ToBool() )
				{
					bitstream.Seek( offset );
				}
				stack.Pop( 1 );
				break;
			}
			case Action_GetVariable:
			{
				idStr variableName = stack.A().ToString();
				for( int i = scope.Num() - 1; i >= 0; i-- )
				{
					stack.A() = scope[i]->Get( variableName );
					if( !stack.A().IsUndefined() )
					{
						break;
					}
				}
				if( stack.A().IsUndefined() && swf_debug.GetInteger() > 1 )
				{
					idLib::Printf( "SWF: unknown variable %s\n", variableName.c_str() );
				}
				break;
			}
			case Action_SetVariable:
			{
				idStr variableName = stack.B().ToString();
				bool found = false;
				for( int i = scope.Num() - 1; i >= 0; i-- )
				{
					if( scope[i]->HasProperty( variableName ) )
					{
						scope[i]->Set( variableName, stack.A() );
						found = true;
						break;
					}
				}
				if( !found )
				{
					thisObject->Set( variableName, stack.A() );
				}
				stack.Pop( 2 );
				break;
			}
			case Action_GotoFrame2:
			{
			
				uint32 frameNum = 0;
				uint8 flags = bitstream.ReadU8();
				if( flags & 2 )
				{
					frameNum += bitstream.ReadU16();
				}
				
				if( verify( thisSprite != NULL ) )
				{
					if( stack.A().IsString() )
					{
						frameNum += thisSprite->FindFrame( stack.A().ToString() );
					}
					else
					{
						frameNum += ( uint32 )stack.A().ToInteger();
					}
					if( ( flags & 1 ) != 0 )
					{
						thisSprite->Play();
					}
					else
					{
						thisSprite->Stop();
					}
					thisSprite->RunTo( frameNum );
				}
				else if( swf_debug.GetInteger() > 0 )
				{
					if( ( flags & 1 ) != 0 )
					{
						idLib::Printf( "SWF: no target movie clip for gotoAndPlay\n" );
					}
					else
					{
						idLib::Printf( "SWF: no target movie clip for gotoAndStop\n" );
					}
				}
				stack.Pop( 1 );
				break;
			}
			case Action_GetProperty:
			{
				if( verify( thisSprite != NULL ) )
				{
					idSWFSpriteInstance* target = thisSprite->ResolveTarget( stack.B().ToString() );
					stack.B() = target->scriptObject->Get( GetPropertyName( stack.A().ToInteger() ) );
				}
				else if( swf_debug.GetInteger() > 0 )
				{
					idLib::Printf( "SWF: no target movie clip for getProperty\n" );
				}
				stack.Pop( 1 );
				break;
			}
			case Action_SetProperty:
			{
				if( verify( thisSprite != NULL ) )
				{
					idSWFSpriteInstance* target = thisSprite->ResolveTarget( stack.C().ToString() );
					target->scriptObject->Set( GetPropertyName( stack.B().ToInteger() ), stack.A() );
				}
				else if( swf_debug.GetInteger() > 0 )
				{
					idLib::Printf( "SWF: no target movie clip for setProperty\n" );
				}
				stack.Pop( 3 );
				break;
			}
			case Action_Trace:
				idLib::PrintfIf( swf_debug.GetInteger() > 0, "SWF Trace: %s\n", stack.A().ToString().c_str() );
				stack.Pop( 1 );
				break;
			case Action_GetTime:
				stack.Alloc().SetInteger( Sys_Milliseconds() );
				break;
			case Action_RandomNumber:
				assert( thisSprite && thisSprite->sprite && thisSprite->sprite->GetSWF() );
				stack.A().SetInteger( thisSprite->sprite->GetSWF()->GetRandom().RandomInt( stack.A().ToInteger() ) );
				break;
			case Action_CallFunction:
			{
				idStr functionName = stack.A().ToString();
				idSWFScriptVar function;
				idSWFScriptObject* object = NULL;
				for( int i = scope.Num() - 1; i >= 0; i-- )
				{
					function = scope[i]->Get( functionName );
					if( !function.IsUndefined() )
					{
						object = scope[i];
						break;
					}
				}
				stack.Pop( 1 );
				
				idSWFParmList parms;
				parms.SetNum( stack.A().ToInteger() );
				stack.Pop( 1 );
				for( int i = 0; i < parms.Num(); i++ )
				{
					parms[i] = stack.A();
					stack.Pop( 1 );
				}
				
				if( function.IsFunction() && verify( object ) )
				{
					stack.Alloc() = function.GetFunction()->Call( object, parms );
				}
				else
				{
					idLib::PrintfIf( swf_debug.GetInteger() > 0, "SWF: unknown function %s\n", functionName.c_str() );
					stack.Alloc().SetUndefined();
				}
				
				break;
			}
			case Action_CallMethod:
			{
				idStr functionName = stack.A().ToString();
				// If the top stack is undefined but there is an object, it's calling the constructor
				if( functionName.IsEmpty() || stack.A().IsUndefined() || stack.A().IsNULL() )
				{
					functionName = "__constructor__";
				}
				idSWFScriptObject* object = NULL;
				idSWFScriptVar function;
				if( stack.B().IsObject() )
				{
					object = stack.B().GetObject();
					function = object->Get( functionName );
					if( !function.IsFunction() )
					{
						idLib::PrintfIf( swf_debug.GetInteger() > 1, "SWF: unknown method %s on %s\n", functionName.c_str(), object->DefaultValue( true ).ToString().c_str() );
					}
				}
				else
				{
					idLib::PrintfIf( swf_debug.GetInteger() > 1, "SWF: NULL object for method %s\n", functionName.c_str() );
				}
				
				stack.Pop( 2 );
				
				idSWFParmList parms;
				parms.SetNum( stack.A().ToInteger() );
				stack.Pop( 1 );
				for( int i = 0; i < parms.Num(); i++ )
				{
					parms[i] = stack.A();
					stack.Pop( 1 );
				}
				
				if( function.IsFunction() )
				{
					stack.Alloc() = function.GetFunction()->Call( object, parms );
				}
				else
				{
					stack.Alloc().SetUndefined();
				}
				break;
			}
			case Action_ConstantPool:
			{
				constants.Clear();
				uint16 numConstants = bitstream.ReadU16();
				for( int i = 0; i < numConstants; i++ )
				{
					constants.Append( idSWFScriptString::Alloc( bitstream.ReadString() ) );
				}
				break;
			}
			case Action_DefineFunction:
			{
				idStr functionName = bitstream.ReadString();
				
				idSWFScriptFunction_Script* newFunction = idSWFScriptFunction_Script::Alloc();
				newFunction->SetScope( scope );
				newFunction->SetConstants( constants );
				newFunction->SetDefaultSprite( defaultSprite );
				
				uint16 numParms = bitstream.ReadU16();
				newFunction->AllocParameters( numParms );
				for( int i = 0; i < numParms; i++ )
				{
					newFunction->SetParameter( i, 0, bitstream.ReadString() );
				}
				uint16 codeSize = bitstream.ReadU16();
				newFunction->SetData( bitstream.ReadData( codeSize ), codeSize );
				
				if( functionName.IsEmpty() )
				{
					stack.Alloc().SetFunction( newFunction );
				}
				else
				{
					thisObject->Set( functionName, idSWFScriptVar( newFunction ) );
				}
				newFunction->Release();
				break;
			}
			case Action_DefineFunction2:
			{
				idStr functionName = bitstream.ReadString();
				
				idSWFScriptFunction_Script* newFunction = idSWFScriptFunction_Script::Alloc();
				newFunction->SetScope( scope );
				newFunction->SetConstants( constants );
				newFunction->SetDefaultSprite( defaultSprite );
				
				uint16 numParms = bitstream.ReadU16();
				
				// The number of registers is from 0 to 255, although valid values are 1 to 256.
				// There must always be at least one register for DefineFunction2, to hold "this" or "super" when required.
				uint8 numRegs = bitstream.ReadU8() + 1;
				
				// Note that SWF byte-ordering causes the flag bits to be reversed per-byte
				// from how the swf_file_format_spec_v10.pdf document describes the ordering in ActionDefineFunction2.
				// PreloadThisFlag is byte 0, not 7, PreloadGlobalFlag is 8, not 15.
				uint16 flags = bitstream.ReadU16();
				
				newFunction->AllocParameters( numParms );
				newFunction->AllocRegisters( numRegs );
				newFunction->SetFlags( flags );
				
				for( int i = 0; i < numParms; i++ )
				{
					uint8 reg = bitstream.ReadU8();
					const char* name = bitstream.ReadString();
					if( reg >= numRegs )
					{
						idLib::Warning( "SWF: Parameter %s in function %s bound to out of range register %d", name, functionName.c_str(), reg );
						reg = 0;
					}
					newFunction->SetParameter( i, reg, name );
				}
				
				uint16 codeSize = bitstream.ReadU16();
				newFunction->SetData( bitstream.ReadData( codeSize ), codeSize );
				
				if( functionName.IsEmpty() )
				{
					stack.Alloc().SetFunction( newFunction );
				}
				else
				{
					thisObject->Set( functionName, idSWFScriptVar( newFunction ) );
				}
				newFunction->Release();
				break;
			}
			case Action_Enumerate:
			{
				idStr variableName = stack.A().ToString();
				for( int i = scope.Num() - 1; i >= 0; i-- )
				{
					stack.A() = scope[i]->Get( variableName );
					if( !stack.A().IsUndefined() )
					{
						break;
					}
				}
				if( !stack.A().IsObject() )
				{
					stack.A().SetNULL();
				}
				else
				{
					idSWFScriptObject* object = stack.A().GetObject();
					object->AddRef();
					stack.A().SetNULL();
					for( int i = 0; i < object->NumVariables(); i++ )
					{
						stack.Alloc().SetString( object->EnumVariable( i ) );
					}
					object->Release();
				}
				break;
			}
			case Action_Enumerate2:
			{
				if( !stack.A().IsObject() )
				{
					stack.A().SetNULL();
				}
				else
				{
					idSWFScriptObject* object = stack.A().GetObject();
					object->AddRef();
					stack.A().SetNULL();
					for( int i = 0; i < object->NumVariables(); i++ )
					{
						stack.Alloc().SetString( object->EnumVariable( i ) );
					}
					object->Release();
				}
				break;
			}
			case Action_Equals2:
			{
				stack.B().SetBool( stack.A().AbstractEquals( stack.B() ) );
				stack.Pop( 1 );
				break;
			}
			case Action_StrictEquals:
			{
				stack.B().SetBool( stack.A().StrictEquals( stack.B() ) );
				stack.Pop( 1 );
				break;
			}
			case Action_GetMember:
			{
				if( ( stack.B().IsUndefined() || stack.B().IsNULL() ) && swf_debug.GetInteger() > 1 )
				{
					idLib::Printf( "SWF: tried to get member %s on an invalid object in sprite '%s'\n", stack.A().ToString().c_str(), thisSprite != NULL ? thisSprite->GetName() : "" );
				}
				if( stack.B().IsObject() )
				{
					idSWFScriptObject* object = stack.B().GetObject();
					if( stack.A().IsNumeric() )
					{
						stack.B() = object->Get( stack.A().ToInteger() );
					}
					else
					{
						stack.B() = object->Get( stack.A().ToString() );
					}
					if( stack.B().IsUndefined() && swf_debug.GetInteger() > 1 )
					{
						idLib::Printf( "SWF: unknown member %s\n", stack.A().ToString().c_str() );
					}
				}
				else if( stack.B().IsString() )
				{
					idStr propertyName = stack.A().ToString();
					if( propertyName.Cmp( "length" ) == 0 )
					{
						stack.B().SetInteger( stack.B().ToString().Length() );
					}
					else if( propertyName.Cmp( "value" ) == 0 )
					{
						// Do nothing
					}
					else
					{
						stack.B().SetUndefined();
					}
				}
				else if( stack.B().IsFunction() )
				{
					idStr propertyName = stack.A().ToString();
					if( propertyName.Cmp( "prototype" ) == 0 )
					{
						// if this is a function, it's a class definition function, and it just wants the prototype object
						// create it if it hasn't been already, and return it
						idSWFScriptFunction* sfs = stack.B().GetFunction();
						idSWFScriptObject* object = sfs->GetPrototype();
						
						if( object == NULL )
						{
							object = idSWFScriptObject::Alloc();
							// Set the __proto__ to the main Object prototype
							idSWFScriptVar baseObjConstructor = scope[0]->Get( "Object" );
							idSWFScriptFunction* baseObj = baseObjConstructor.GetFunction();
							object->Set( "__proto__", baseObj->GetPrototype() );
							sfs->SetPrototype( object );
						}
						
						stack.B() = idSWFScriptVar( object );
					}
					else
					{
						stack.B().SetUndefined();
					}
				}
				else
				{
					stack.B().SetUndefined();
				}
				stack.Pop( 1 );
				break;
			}
			case Action_SetMember:
			{
				if( stack.C().IsObject() )
				{
					idSWFScriptObject* object = stack.C().GetObject();
					if( stack.B().IsNumeric() )
					{
						object->Set( stack.B().ToInteger(), stack.A() );
					}
					else
					{
						object->Set( stack.B().ToString(), stack.A() );
					}
				}
				stack.Pop( 3 );
				break;
			}
			case Action_InitArray:
			{
				idSWFScriptObject* object = idSWFScriptObject::Alloc();
				object->MakeArray();
				
				int numElements = stack.A().ToInteger();
				stack.Pop( 1 );
				
				for( int i = 0; i < numElements; i++ )
				{
					object->Set( i, stack.A() );
					stack.Pop( 1 );
				}
				
				stack.Alloc().SetObject( object );
				
				object->Release();
				break;
			}
			case Action_InitObject:
			{
				idSWFScriptObject* object = idSWFScriptObject::Alloc();
				
				int numElements = stack.A().ToInteger();
				stack.Pop( 1 );
				
				for( int i = 0; i < numElements; i++ )
				{
					object->Set( stack.B().ToString(), stack.A() );
					stack.Pop( 2 );
				}
				
				stack.Alloc().SetObject( object );
				
				object->Release();
				break;
			}
			case Action_NewObject:
			{
				idSWFScriptObject* object = idSWFScriptObject::Alloc();
				
				idStr functionName = stack.A().ToString();
				stack.Pop( 1 );
				
				if( functionName.Cmp( "Array" ) == 0 )
				{
					object->MakeArray();
					
					int numElements = stack.A().ToInteger();
					stack.Pop( 1 );
					
					for( int i = 0; i < numElements; i++ )
					{
						object->Set( i, stack.A() );
						stack.Pop( 1 );
					}
					
					idSWFScriptVar baseObjConstructor = scope[0]->Get( "Object" );
					idSWFScriptFunction* baseObj = baseObjConstructor.GetFunction();
					object->Set( "__proto__", baseObj->GetPrototype() );
					// object prototype is not set here because it will be auto created from Object later
				}
				else
				{
					idSWFParmList parms;
					parms.SetNum( stack.A().ToInteger() );
					stack.Pop( 1 );
					for( int i = 0; i < parms.Num(); i++ )
					{
						parms[i] = stack.A();
						stack.Pop( 1 );
					}
					
					idSWFScriptVar objdef = scope[0]->Get( functionName );
					if( objdef.IsFunction() )
					{
						idSWFScriptFunction* constructorFunction = objdef.GetFunction();
						object->Set( "__proto__", constructorFunction->GetPrototype() );
						object->SetPrototype( constructorFunction->GetPrototype() );
						constructorFunction->Call( object, parms );
					}
					else
					{
						idLib::Warning( "SWF: Unknown class definition %s", functionName.c_str() );
					}
				}
				
				stack.Alloc().SetObject( object );
				
				object->Release();
				break;
			}
			case Action_Extends:
			{
				idSWFScriptFunction* superclassConstructorFunction = stack.A().GetFunction();
				idSWFScriptFunction* subclassConstructorFunction = stack.B().GetFunction();
				stack.Pop( 2 );
				
				idSWFScriptObject* scriptObject = idSWFScriptObject::Alloc();
				scriptObject->SetPrototype( superclassConstructorFunction->GetPrototype() );
				scriptObject->Set( "__proto__", idSWFScriptVar( superclassConstructorFunction->GetPrototype() ) );
				scriptObject->Set( "__constructor__", idSWFScriptVar( superclassConstructorFunction ) );
				
				subclassConstructorFunction->SetPrototype( scriptObject );
				
				scriptObject->Release();
				break;
			}
			case Action_TargetPath:
			{
				if( !stack.A().IsObject() )
				{
					stack.A().SetUndefined();
				}
				else
				{
					idSWFScriptObject* object = stack.A().GetObject();
					if( object->GetSprite() == NULL )
					{
						stack.A().SetUndefined();
					}
					else
					{
						idStr dotName = object->GetSprite()->name.c_str();
						for( idSWFSpriteInstance* target = object->GetSprite()->parent; target != NULL; target = target->parent )
						{
							dotName = target->name + "." + dotName;
						}
						stack.A().SetString( dotName );
					}
				}
				break;
			}
			case Action_With:
			{
				int withSize = bitstream.ReadU16();
				idSWFBitStream bitstream2( bitstream.ReadData( withSize ), withSize, false );
				if( stack.A().IsObject() )
				{
					idSWFScriptObject* withObject = stack.A().GetObject();
					withObject->AddRef();
					stack.Pop( 1 );
					scope.Append( withObject );
					Run( thisObject, stack, bitstream2 );
					scope.SetNum( scope.Num() - 1 );
					withObject->Release();
				}
				else
				{
					if( swf_debug.GetInteger() > 0 )
					{
						idLib::Printf( "SWF: with() invalid object specified\n" );
					}
					stack.Pop( 1 );
				}
				break;
			}
			case Action_ToNumber:
				stack.A().SetFloat( stack.A().ToFloat() );
				break;
			case Action_ToString:
				stack.A().SetString( stack.A().ToString() );
				break;
			case Action_TypeOf:
				stack.A().SetString( stack.A().TypeOf() );
				break;
			case Action_Add2:
			{
				if( stack.A().IsString() || stack.B().IsString() )
				{
					stack.B().SetString( stack.B().ToString() + stack.A().ToString() );
				}
				else
				{
					stack.B().SetFloat( stack.B().ToFloat() + stack.A().ToFloat() );
				}
				stack.Pop( 1 );
				break;
			}
			case Action_Less2:
			{
				if( stack.A().IsString() && stack.B().IsString() )
				{
					stack.B().SetBool( stack.B().ToString() < stack.A().ToString() );
				}
				else
				{
					stack.B().SetBool( stack.B().ToFloat() < stack.A().ToFloat() );
				}
				stack.Pop( 1 );
				break;
			}
			case Action_Greater:
			{
				if( stack.A().IsString() && stack.B().IsString() )
				{
					stack.B().SetBool( stack.B().ToString() > stack.A().ToString() );
				}
				else
				{
					stack.B().SetBool( stack.B().ToFloat() > stack.A().ToFloat() );
				}
				stack.Pop( 1 );
				break;
			}
			case Action_Modulo:
			{
				int32 a = stack.A().ToInteger();
				int32 b = stack.B().ToInteger();
				if( a == 0 )
				{
					stack.B().SetUndefined();
				}
				else
				{
					stack.B().SetInteger( b % a );
				}
				stack.Pop( 1 );
				break;
			}
			case Action_BitAnd:
				stack.B().SetInteger( stack.B().ToInteger() & stack.A().ToInteger() );
				stack.Pop( 1 );
				break;
			case Action_BitLShift:
				stack.B().SetInteger( stack.B().ToInteger() << stack.A().ToInteger() );
				stack.Pop( 1 );
				break;
			case Action_BitOr:
				stack.B().SetInteger( stack.B().ToInteger() | stack.A().ToInteger() );
				stack.Pop( 1 );
				break;
			case Action_BitRShift:
				stack.B().SetInteger( stack.B().ToInteger() >> stack.A().ToInteger() );
				stack.Pop( 1 );
				break;
			case Action_BitURShift:
				stack.B().SetInteger( ( uint32 )stack.B().ToInteger() >> stack.A().ToInteger() );
				stack.Pop( 1 );
				break;
			case Action_BitXor:
				stack.B().SetInteger( stack.B().ToInteger() ^ stack.A().ToInteger() );
				stack.Pop( 1 );
				break;
			case Action_Decrement:
				stack.A().SetFloat( stack.A().ToFloat() - 1.0f );
				break;
			case Action_Increment:
				stack.A().SetFloat( stack.A().ToFloat() + 1.0f );
				break;
			case Action_PushDuplicate:
			{
				idSWFScriptVar dup = stack.A();
				stack.Alloc() = dup;
				break;
			}
			case Action_StackSwap:
			{
				idSWFScriptVar temp = stack.A();
				stack.A() = stack.B();
				stack.A() = temp;
				break;
			}
			case Action_StoreRegister:
			{
				uint8 registerNumber = bitstream.ReadU8();
				registers[ registerNumber ] = stack.A();
				break;
			}
			case Action_DefineLocal:
			{
				scope[scope.Num() - 1]->Set( stack.B().ToString(), stack.A() );
				stack.Pop( 2 );
				break;
			}
			case Action_DefineLocal2:
			{
				scope[scope.Num() - 1]->Set( stack.A().ToString(), idSWFScriptVar() );
				stack.Pop( 1 );
				break;
			}
			case Action_Delete:
			{
				if( swf_debug.GetInteger() > 0 )
				{
					idLib::Printf( "SWF: Delete ignored\n" );
				}
				// We no longer support deleting variables because the performance cost of updating the hash tables is not worth it
				stack.Pop( 2 );
				break;
			}
			case Action_Delete2:
			{
				if( swf_debug.GetInteger() > 0 )
				{
					idLib::Printf( "SWF: Delete2 ignored\n" );
				}
				// We no longer support deleting variables because the performance cost of updating the hash tables is not worth it
				stack.Pop( 1 );
				break;
			}
			// These are functions we just don't support because we never really needed to
			case Action_CloneSprite:
			case Action_RemoveSprite:
			case Action_Call:
			case Action_SetTarget2:
			case Action_NewMethod:
			default:
				idLib::Warning( "SWF: Unhandled Action %s", idSWF::GetActionName( code ) );
				// We have to abort here because the rest of the script is basically meaningless now
				assert( false );
				callstackLevel--;
				return idSWFScriptVar();
		}
	}
	callstackLevel--;
	return idSWFScriptVar();
}

/*
========================
idSWF::Invoke
========================
*/
void idSWF::Invoke( const char* functionName, const idSWFParmList& parms )
{
	idSWFScriptObject* obj = mainspriteInstance->GetScriptObject();
	idSWFScriptVar scriptVar = obj->Get( functionName );
	
	if( swf_debugInvoke.GetBool() )
	{
		idLib::Printf( "SWF: Invoke %s with %d parms (%s)\n", functionName, parms.Num(), GetName() );
	}
	
	if( scriptVar.IsFunction() )
	{
		scriptVar.GetFunction()->Call( NULL, parms );
	}
}

/*
========================
idSWF::Invoke
========================
*/
void idSWF::Invoke( const char* functionName, const idSWFParmList& parms, idSWFScriptVar& scriptVar )
{

	if( scriptVar.IsFunction() )
	{
		scriptVar.GetFunction()->Call( NULL, parms );
	}
	else
	{
		idSWFScriptObject* obj = mainspriteInstance->GetScriptObject();
		scriptVar = obj->Get( functionName );
		
		if( scriptVar.IsFunction() )
		{
			scriptVar.GetFunction()->Call( NULL, parms );
		}
	}
}

/*
========================
idSWF::Invoke
========================
*/
void idSWF::Invoke( const char*   functionName, const idSWFParmList& parms, bool& functionExists )
{
	idSWFScriptObject* obj = mainspriteInstance->GetScriptObject();
	idSWFScriptVar scriptVar = obj->Get( functionName );
	
	if( swf_debugInvoke.GetBool() )
	{
		idLib::Printf( "SWF: Invoke %s with %d parms (%s)\n", functionName, parms.Num(), GetName() );
	}
	
	if( scriptVar.IsFunction() )
	{
		scriptVar.GetFunction()->Call( NULL, parms );
		functionExists = true;
	}
	else
	{
		functionExists = false;
	}
}