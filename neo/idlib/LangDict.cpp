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

// This is the default language dict that the entire system uses, but you can instantiate your own idLangDict classes to manipulate a language dictionary in a tool
idLangDict	idLocalization::languageDict;

idCVar lang_maskLocalizedStrings( "lang_maskLocalizedStrings", "0", CVAR_BOOL, "Masks all localized strings to help debugging.  When set will replace strings with an equal length of W's and ending in an X.  Note: The masking occurs at string table load time." );

/*
========================
idLocalization::ClearDictionary
========================
*/
void idLocalization::ClearDictionary()
{
	languageDict.Clear();
}

/*
========================
idLocalization::LoadDictionary
========================
*/
bool idLocalization::LoadDictionary( const byte* data, int dataLen, const char* fileName )
{
	return languageDict.Load( data, dataLen, fileName );
}

/*
========================
idLocalization::GetString
========================
*/
const char* idLocalization::GetString( const char* inString )
{
	return languageDict.GetString( inString );
}

/*
========================
idLocalization::FindString
========================
*/
const char* idLocalization::FindString( const char* inString )
{
	return languageDict.FindString( inString );
}

/*
========================
idLocalization::VerifyUTF8
========================
*/
utf8Encoding_t idLocalization::VerifyUTF8( const uint8* buffer, const int bufferLen, const char* name )
{
	utf8Encoding_t encoding;
	idStr::IsValidUTF8( buffer, bufferLen, encoding );
	if( encoding == UTF8_INVALID )
	{
		idLib::FatalError( "Language file %s is not valid UTF-8 or plain ASCII.", name );
	}
	else if( encoding == UTF8_INVALID_BOM )
	{
		idLib::FatalError( "Language file %s is marked as UTF-8 but has invalid encoding.", name );
	}
	else if( encoding == UTF8_ENCODED_NO_BOM )
	{
		idLib::FatalError( "Language file %s has no byte order marker. Fix this or roll back to a version that has the marker.", name );
	}
	else if( encoding != UTF8_ENCODED_BOM && encoding != UTF8_PURE_ASCII )
	{
		idLib::FatalError( "Language file %s has unknown utf8Encoding_t.", name );
	}
	return encoding;
}

// string entries can refer to other string entries,
// recursing up to this many times before we decided someone did something stupid
const char* idLangDict::KEY_PREFIX = "#str_";	// all keys should be prefixed with this for redirection to work
const int idLangDict::KEY_PREFIX_LEN = idStr::Length( KEY_PREFIX );

/*
========================
idLangDict::idLangDict
========================
*/
idLangDict::idLangDict() : keyIndex( 4096, 4096 )
{
}

/*
========================
idLangDict::~idLangDict
========================
*/
idLangDict::~idLangDict()
{
	Clear();
}

/*
========================
idLangDict::Clear
========================
*/
void idLangDict::Clear()
{
	//mem.PushHeap();
	for( int i = 0; i < keyVals.Num(); i++ )
	{
		if( keyVals[i].value == NULL )
		{
			continue;
		}
		blockAlloc.Free( keyVals[i].value );
		keyVals[i].value = NULL;
	}
	//mem.PopHeap();
}

/*
========================
idLangDict::Load
========================
*/
bool idLangDict::Load( const byte* buffer, const int bufferLen, const char* name )
{

	if( buffer == NULL || bufferLen <= 0 )
	{
		// let whoever called us deal with the failure (so sys_lang can be reset)
		return false;
	}

	idLib::Printf( "Reading %s", name );

	bool utf8 = false;

	// in all but retail builds, ensure that the byte-order mark is NOT MISSING so that
	// we can avoid debugging UTF-8 code
#ifndef ID_RETAIL
	utf8Encoding_t encoding = idLocalization::VerifyUTF8( buffer, bufferLen, name );
	if( encoding == UTF8_ENCODED_BOM )
	{
		utf8 = true;
	}
	else if( encoding == UTF8_PURE_ASCII )
	{
		utf8 = false;
	}
	else
	{
		assert( false );	// this should have been handled in VerifyUTF8 with a FatalError
		return false;
	}
#else
	// in release we just check the BOM so we're not scanning the lang file twice on startup
	if( bufferLen > 3 && buffer[0] == 0xEF && buffer[1] == 0xBB && buffer[2] == 0xBF )
	{
		utf8 = true;
	}
#endif

	if( utf8 )
	{
		idLib::Printf( " as UTF-8\n" );
	}
	else
	{
		idLib::Printf( " as ASCII\n" );
	}

	idStr tempKey;
	idStr tempVal;

	int line = 0;
	int numStrings = 0;

	int i = 0;
	while( i < bufferLen )
	{
		uint32 c = buffer[i++];
		if( c == '/' )    // comment, read until new line
		{
			while( i < bufferLen )
			{
				c = buffer[i++];
				if( c == '\n' )
				{
					line++;
					break;
				}
			}
		}
		else if( c == '}' )
		{
			break;
		}
		else if( c == '\n' )
		{
			line++;
		}
		else if( c == '\"' )
		{
			int keyStart = i;
			int keyEnd = -1;
			while( i < bufferLen )
			{
				c = buffer[i++];
				if( c == '\"' )
				{
					keyEnd = i - 1;
					break;
				}
			}
			if( keyEnd < keyStart )
			{
				idLib::FatalError( "%s File ended while reading key at line %d", name, line );
			}
			tempKey.CopyRange( ( char* )buffer, keyStart, keyEnd );

			int valStart = -1;
			while( i < bufferLen )
			{
				c = buffer[i++];
				if( c == '\"' )
				{
					valStart = i;
					break;
				}
			}
			if( valStart < 0 )
			{
				idLib::FatalError( "%s File ended while reading value at line %d", name, line );
			}
			int valEnd = -1;
			tempVal.CapLength( 0 );
			while( i < bufferLen )
			{
				c = utf8 ? idStr::UTF8Char( buffer, i ) : buffer[i++];
				if( !utf8 && c >= 0x80 )
				{
					// this is a serious error and we must check this to avoid accidentally shipping a file where someone squased UTF-8 encodings
					idLib::FatalError( "Language file %s is supposed to be plain ASCII, but has byte values > 127!", name );
				}
				if( c == '\"' )
				{
					valEnd = i - 1;
					continue;
				}
				if( c == '\n' )
				{
					line++;
					break;
				}
				if( c == '\r' )
				{
					continue;
				}
				if( c == '\\' )
				{
					c = utf8 ? idStr::UTF8Char( buffer, i ) : buffer[i++];
					if( c == 'n' )
					{
						c = '\n';
					}
					else if( c == 't' )
					{
						c = '\t';
					}
					else if( c == '\"' )
					{
						c = '\"';
					}
					else if( c == '\\' )
					{
						c = '\\';
					}
					else
					{
						idLib::Warning( "Unknown escape sequence %x at line %d", c, line );
					}
				}
				tempVal.AppendUTF8Char( c );
			}
			if( valEnd < valStart )
			{
				idLib::FatalError( "%s File ended while reading value at line %d", name, line );
			}
			if( lang_maskLocalizedStrings.GetBool() && tempVal.Length() > 0 && tempKey.Find( "#font_" ) == -1 )
			{
				int len = tempVal.Length();
				if( len > 0 )
				{
					tempVal.Fill( 'W', len - 1 );
				}
				else
				{
					tempVal.Empty();
				}
				tempVal.Append( 'X' );
			}
			AddKeyVal( tempKey, tempVal );
			numStrings++;
		}
	}

	idLib::Printf( "%i strings read\n", numStrings );

	// get rid of any waste due to geometric list growth
	//mem.PushHeap();
	keyVals.Condense();
	//mem.PopHeap();

	return true;
}

/*
========================
idLangDict::Save
========================
*/
bool idLangDict::Save( const char* fileName )
{
	idFile* outFile = fileSystem->OpenFileWrite( fileName );
	if( outFile == NULL )
	{
		idLib::Warning( "Error saving: %s", fileName );
		return false;
	}
	byte bof[3] = { 0xEF, 0xBB, 0xBF };
	outFile->Write( bof, 3 );
	outFile->WriteFloatString( "// string table\n//\n\n{\n" );
	for( int j = 0; j < keyVals.Num(); j++ )
	{
		const idLangKeyValue& kvp = keyVals[j];
		if( kvp.value == NULL )
		{
			continue;
		}
		outFile->WriteFloatString( "\t\"%s\"\t\"", kvp.key );
		for( int k = 0; kvp.value[k] != 0; k++ )
		{
			char ch = kvp.value[k];
			if( ch == '\t' )
			{
				outFile->Write( "\\t", 2 );
			}
			else if( ch == '\n' || ch == '\r' )
			{
				outFile->Write( "\\n", 2 );
			}
			else if( ch == '"' )
			{
				outFile->Write( "\\\"", 2 );
			}
			else if( ch == '\\' )
			{
				outFile->Write( "\\\\", 2 );
			}
			else
			{
				outFile->Write( &ch, 1 );
			}
		}
		outFile->WriteFloatString( "\"\n" );
	}
	outFile->WriteFloatString( "\n}\n" );
	delete outFile;
	return true;
}

/*
========================
idLangDict::GetString
========================
*/
const char* idLangDict::GetString( const char* str ) const
{
	const char* localized = FindString( str );
	if( localized == NULL )
	{
		return str;
	}
	return localized;
}

/*
========================
idLangDict::FindStringIndex
========================
*/
int idLangDict::FindStringIndex( const char* str ) const
{
	if( str == NULL )
	{
		return -1;
	}
	int hash = idStr::IHash( str );
	for( int i = keyIndex.GetFirst( hash ); i >= 0; i = keyIndex.GetNext( i ) )
	{
		if( idStr::Icmp( str, keyVals[i].key ) == 0 )
		{
			return i;
		}
	}
	return -1;
}

/*
========================
idLangDict::FindString_r
========================
*/
const char* idLangDict::FindString_r( const char* str, int& depth ) const
{
	depth++;
	if( depth > MAX_REDIRECTION_DEPTH )
	{
		// This isn't an error because we assume the error will be obvious somewhere in a GUI or something,
		// and the whole point of tracking the depth is to avoid a crash.
		idLib::Warning( "String '%s', indirection depth > %d", str, MAX_REDIRECTION_DEPTH );
		return NULL;
	}

	if( str == NULL || str[0] == '\0' )
	{
		return NULL;
	}

	int index = FindStringIndex( str );
	if( index < 0 )
	{
		return NULL;
	}
	const char* value = keyVals[index].value;
	if( value == NULL )
	{
		return NULL;
	}
	if( IsStringId( value ) )
	{
		// this string is re-directed to another entry
		return FindString_r( value, depth );
	}
	return value;
}

/*
========================
idLangDict::FindString
========================
*/
const char* idLangDict::FindString( const char* str ) const
{
	int depth = 0;
	return FindString_r( str, depth );
}

/*
========================
idLangDict::DeleteString
========================
*/
bool idLangDict::DeleteString( const char* key )
{
	return DeleteString( FindStringIndex( key ) );
}

/*
========================
idLangDict::DeleteString
========================
*/
bool idLangDict::DeleteString( const int idx )
{
	if( idx < 0 || idx >= keyVals.Num() )
	{
		return false;
	}

	//mem.PushHeap();
	blockAlloc.Free( keyVals[idx].value );
	keyVals[idx].value = NULL;
	//mem.PopHeap();

	return true;
}

/*
========================
idLangDict::RenameStringKey
========================
*/
bool idLangDict::RenameStringKey( const char* oldKey, const char* newKey )
{
	int index = FindStringIndex( oldKey );
	if( index < 0 )
	{
		return false;
	}
	//mem.PushHeap();
	blockAlloc.Free( keyVals[index].key );
	int newKeyLen = idStr::Length( newKey );
	keyVals[index].key = blockAlloc.Alloc( newKeyLen + 1 );
	idStr::Copynz( keyVals[index].key, newKey, newKeyLen + 1 );
	int oldHash = idStr::IHash( oldKey );
	int newHash = idStr::IHash( newKey );
	if( oldHash != newHash )
	{
		keyIndex.Remove( oldHash, index );
		keyIndex.Add( newHash, index );
	}
	//mem.PopHeap();

	return true;
}

/*
========================
idLangDict::SetString
========================
*/
bool idLangDict::SetString( const char* key, const char* val )
{
	int index = FindStringIndex( key );
	if( index < 0 )
	{
		return false;
	}
	//mem.PushHeap();
	if( keyVals[index].value != NULL )
	{
		blockAlloc.Free( keyVals[index].value );
	}
	int valLen = idStr::Length( val );
	keyVals[index].value = blockAlloc.Alloc( valLen + 1 );
	idStr::Copynz( keyVals[index].value, val, valLen + 1 );
	//mem.PopHeap();
	return true;
}

/*
========================
idLangDict::AddKeyVal
========================
*/
void idLangDict::AddKeyVal( const char* key, const char* val )
{
	if( SetString( key, val ) )
	{
		return;
	}
	//mem.PushHeap();
	int keyLen = idStr::Length( key );
	char* k = blockAlloc.Alloc( keyLen + 1 );
	idStr::Copynz( k, key, keyLen + 1 );
	char* v = NULL;
	if( val != NULL )
	{
		int valLen = idStr::Length( val );
		v = blockAlloc.Alloc( valLen + 1 );
		idStr::Copynz( v, val, valLen + 1 );
	}
	int index = keyVals.Append( idLangKeyValue( k, v ) );
	int hash = idStr::IHash( key );
	keyIndex.Add( hash, index );
	//mem.PopHeap();
}

/*
========================
idLangDict::AddString
========================
*/
const char* idLangDict::AddString( const char* val )
{
	int i = Sys_Milliseconds();
	idStr key;
	sprintf( key, "#str_%06d", ( i++ % 1000000 ) );
	while( FindStringIndex( key ) > 0 )
	{
		sprintf( key, "#str_%06d", ( i++ % 1000000 ) );
	}
	AddKeyVal( key, val );
	int index = FindStringIndex( key );
	return keyVals[index].key;
}

/*
========================
idLangDict::GetNumKeyVals
========================
*/
int idLangDict::GetNumKeyVals() const
{
	return keyVals.Num();
}

/*
========================
idLangDict::GetKeyVal
========================
*/
const idLangKeyValue* idLangDict::GetKeyVal( int i ) const
{
	return &keyVals[i];
}

/*
========================
idLangDict::IsStringId
========================
*/
bool idLangDict::IsStringId( const char* str )
{
	return idStr::Icmpn( str, KEY_PREFIX, KEY_PREFIX_LEN ) == 0;
}

/*
========================
idLangDict::GetLocalizedString
========================
*/
const char* idLangDict::GetLocalizedString( const idStrId& strId ) const
{
	if( strId.GetIndex() >= 0 && strId.GetIndex() < keyVals.Num() )
	{
		if( keyVals[ strId.GetIndex() ].value == NULL )
		{
			return keyVals[ strId.GetIndex() ].key;
		}
		else
		{
			return keyVals[ strId.GetIndex() ].value;
		}
	}
	return "";
}

/*
================================================================================================
idStrId
================================================================================================
*/

/*
========================
idStrId::Set
========================
*/
void idStrId::Set( const char* key )
{
	if( key == NULL || key[0] == 0 )
	{
		index = -1;
	}
	else
	{
		index = idLocalization::languageDict.FindStringIndex( key );
		if( index < 0 )
		{
			// don't allow setting of string ID's to an unknown ID... this should only be allowed from
			// the string table tool because additions from anywhere else are not guaranteed to be
			// saved to the .lang file.
			idLib::Warning( "Attempted to set unknown string ID '%s'", key );
		}
	}
}

/*
========================
idStrId::GetKey
========================
*/
const char* idStrId::GetKey() const
{
	if( index >= 0 && index < idLocalization::languageDict.keyVals.Num() )
	{
		return idLocalization::languageDict.keyVals[index].key;
	}
	return "";
}

/*
========================
idStrId::GetLocalizedString
========================
*/
const char* idStrId::GetLocalizedString() const
{
	return idLocalization::languageDict.GetLocalizedString( *this );
}

