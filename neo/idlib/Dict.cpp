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

idStrPool		idDict::globalKeys;
idStrPool		idDict::globalValues;

/*
================
idDict::operator=

  clear existing key/value pairs and copy all key/value pairs from other
================
*/
idDict& idDict::operator=( const idDict& other )
{
	int i;

	// check for assignment to self
	if( this == &other )
	{
		return *this;
	}

	Clear();

	args = other.args;
	argHash = other.argHash;

	for( i = 0; i < args.Num(); i++ )
	{
		args[i].key = globalKeys.CopyString( args[i].key );
		args[i].value = globalValues.CopyString( args[i].value );
	}

	return *this;
}

/*
================
idDict::Copy

  copy all key value pairs without removing existing key/value pairs not present in the other dict
================
*/
void idDict::Copy( const idDict& other )
{
	int i, n, *found;
	idKeyValue kv;

	// check for assignment to self
	if( this == &other )
	{
		return;
	}

	n = other.args.Num();

	if( args.Num() )
	{
		found = ( int* ) _alloca16( other.args.Num() * sizeof( int ) );
		for( i = 0; i < n; i++ )
		{
			found[i] = FindKeyIndex( other.args[i].GetKey() );
		}
	}
	else
	{
		found = NULL;
	}

	for( i = 0; i < n; i++ )
	{
		if( found && found[i] != -1 )
		{
			// first set the new value and then free the old value to allow proper self copying
			const idPoolStr* oldValue = args[found[i]].value;
			args[found[i]].value = globalValues.CopyString( other.args[i].value );
			globalValues.FreeString( oldValue );
		}
		else
		{
			kv.key = globalKeys.CopyString( other.args[i].key );
			kv.value = globalValues.CopyString( other.args[i].value );
			argHash.Add( argHash.GenerateKey( kv.GetKey(), false ), args.Append( kv ) );
		}
	}
}

/*
================
idDict::TransferKeyValues

  clear existing key/value pairs and transfer key/value pairs from other
================
*/
void idDict::TransferKeyValues( idDict& other )
{
	int i, n;

	if( this == &other )
	{
		return;
	}

	if( other.args.Num() && other.args[0].key->GetPool() != &globalKeys )
	{
		common->FatalError( "idDict::TransferKeyValues: can't transfer values across a DLL boundary" );
		return;
	}

	Clear();

	n = other.args.Num();
	args.SetNum( n );
	for( i = 0; i < n; i++ )
	{
		args[i].key = other.args[i].key;
		args[i].value = other.args[i].value;
	}
	argHash = other.argHash;

	other.args.Clear();
	other.argHash.Free();
}

/*
================
idDict::Parse
================
*/
bool idDict::Parse( idParser& parser )
{
	idToken	token;
	idToken	token2;
	bool	errors;

	errors = false;

	parser.ExpectTokenString( "{" );
	parser.ReadToken( &token );
	while( ( token.type != TT_PUNCTUATION ) || ( token != "}" ) )
	{
		if( token.type != TT_STRING )
		{
			parser.Error( "Expected quoted string, but found '%s'", token.c_str() );
		}

		if( !parser.ReadToken( &token2 ) )
		{
			parser.Error( "Unexpected end of file" );
		}

		if( FindKey( token ) )
		{
			parser.Warning( "'%s' already defined", token.c_str() );
			errors = true;
		}
		Set( token, token2 );

		if( !parser.ReadToken( &token ) )
		{
			parser.Error( "Unexpected end of file" );
		}
	}

	return !errors;
}

/*
================
idDict::SetDefaults
================
*/
void idDict::SetDefaults( const idDict* dict )
{
	int i, n;
	const idKeyValue* kv, *def;
	idKeyValue newkv;

	n = dict->args.Num();
	for( i = 0; i < n; i++ )
	{
		def = &dict->args[i];
		kv = FindKey( def->GetKey() );
		if( !kv )
		{
			newkv.key = globalKeys.CopyString( def->key );
			newkv.value = globalValues.CopyString( def->value );
			argHash.Add( argHash.GenerateKey( newkv.GetKey(), false ), args.Append( newkv ) );
		}
	}
}

/*
================
idDict::Clear
================
*/
void idDict::Clear()
{
	int i;

	for( i = 0; i < args.Num(); i++ )
	{
		globalKeys.FreeString( args[i].key );
		globalValues.FreeString( args[i].value );
	}

	args.Clear();
	argHash.Free();
}

/*
================
idDict::Print
================
*/
void idDict::Print() const
{
	int i;
	int n;

	n = args.Num();
	for( i = 0; i < n; i++ )
	{
		idLib::common->Printf( "%s = %s\n", args[i].GetKey().c_str(), args[i].GetValue().c_str() );
	}
}

int KeyCompare( const idKeyValue* a, const idKeyValue* b )
{
	return idStr::Cmp( a->GetKey(), b->GetKey() );
}

/*
================
idDict::Checksum
================
*/
int	idDict::Checksum() const
{
	// RB: 64 bit fixes, changed long to int
	unsigned int ret;
	// RB end
	int i, n;

	idList<idKeyValue> sorted = args;
	sorted.SortWithTemplate( idSort_KeyValue() );
	n = sorted.Num();
	CRC32_InitChecksum( ret );
	for( i = 0; i < n; i++ )
	{
		CRC32_UpdateChecksum( ret, sorted[i].GetKey().c_str(), sorted[i].GetKey().Length() );
		CRC32_UpdateChecksum( ret, sorted[i].GetValue().c_str(), sorted[i].GetValue().Length() );
	}
	CRC32_FinishChecksum( ret );
	return ret;
}

/*
================
idDict::Allocated
================
*/
size_t idDict::Allocated() const
{
	int		i;
	size_t	size;

	size = args.Allocated() + argHash.Allocated();
	for( i = 0; i < args.Num(); i++ )
	{
		size += args[i].Size();
	}

	return size;
}

/*
================
idDict::Set
================
*/
void idDict::Set( const char* key, const char* value )
{
	int i;
	idKeyValue kv;

	if( key == NULL || key[0] == '\0' )
	{
		return;
	}

	i = FindKeyIndex( key );
	if( i != -1 )
	{
		// first set the new value and then free the old value to allow proper self copying
		const idPoolStr* oldValue = args[i].value;
		args[i].value = globalValues.AllocString( value );
		globalValues.FreeString( oldValue );
	}
	else
	{
		kv.key = globalKeys.AllocString( key );
		kv.value = globalValues.AllocString( value );
		argHash.Add( argHash.GenerateKey( kv.GetKey(), false ), args.Append( kv ) );
	}
}

/*
================
idDict::GetFloat
================
*/
bool idDict::GetFloat( const char* key, const char* defaultString, float& out ) const
{
	const char*	s;
	bool		found;

	found = GetString( key, defaultString, &s );
	out = atof( s );
	return found;
}

/*
================
idDict::GetInt
================
*/
bool idDict::GetInt( const char* key, const char* defaultString, int& out ) const
{
	const char*	s;
	bool		found;

	found = GetString( key, defaultString, &s );
	out = atoi( s );
	return found;
}

/*
================
idDict::GetBool
================
*/
bool idDict::GetBool( const char* key, const char* defaultString, bool& out ) const
{
	const char*	s;
	bool		found;

	found = GetString( key, defaultString, &s );
	out = ( atoi( s ) != 0 );
	return found;
}

/*
================
idDict::GetFloat
================
*/
bool idDict::GetFloat( const char* key, const float defaultFloat, float& out ) const
{
	const idKeyValue* kv = FindKey( key );
	if( kv )
	{
		out = atof( kv->GetValue() );
		return true;
	}
	else
	{
		out = defaultFloat;
		return false;
	}
}

/*
================
idDict::GetInt
================
*/
bool idDict::GetInt( const char* key, const int defaultInt, int& out ) const
{
	const idKeyValue* kv = FindKey( key );
	if( kv )
	{
		out = atoi( kv->GetValue() );
		return true;
	}
	else
	{
		out = defaultInt;
		return false;
	}
}

/*
================
idDict::GetBool
================
*/
bool idDict::GetBool( const char* key, const bool defaultBool, bool& out ) const
{
	const idKeyValue* kv = FindKey( key );
	if( kv )
	{
		out = ( atoi( kv->GetValue() ) != 0 );
		return true;
	}
	else
	{
		out = defaultBool;
		return false;
	}
}

/*
================
idDict::GetAngles
================
*/
bool idDict::GetAngles( const char* key, const char* defaultString, idAngles& out ) const
{
	bool		found;
	const char*	s;

	if( !defaultString )
	{
		defaultString = "0 0 0";
	}

	found = GetString( key, defaultString, &s );
	out.Zero();
	sscanf( s, "%f %f %f", &out.pitch, &out.yaw, &out.roll );
	return found;
}

/*
================
idDict::GetVector
================
*/
bool idDict::GetVector( const char* key, const char* defaultString, idVec3& out ) const
{
	bool		found;
	const char*	s;

	if( !defaultString )
	{
		defaultString = "0 0 0";
	}

	found = GetString( key, defaultString, &s );
	out.Zero();
	sscanf( s, "%f %f %f", &out.x, &out.y, &out.z );
	return found;
}

/*
================
idDict::GetVec2
================
*/
bool idDict::GetVec2( const char* key, const char* defaultString, idVec2& out ) const
{
	bool		found;
	const char*	s;

	if( !defaultString )
	{
		defaultString = "0 0";
	}

	found = GetString( key, defaultString, &s );
	out.Zero();
	sscanf( s, "%f %f", &out.x, &out.y );
	return found;
}

/*
================
idDict::GetVec4
================
*/
bool idDict::GetVec4( const char* key, const char* defaultString, idVec4& out ) const
{
	bool		found;
	const char*	s;

	if( !defaultString )
	{
		defaultString = "0 0 0 0";
	}

	found = GetString( key, defaultString, &s );
	out.Zero();
	sscanf( s, "%f %f %f %f", &out.x, &out.y, &out.z, &out.w );
	return found;
}

/*
================
idDict::GetMatrix
================
*/
bool idDict::GetMatrix( const char* key, const char* defaultString, idMat3& out ) const
{
	const char*	s;
	bool		found;

	if( !defaultString )
	{
		defaultString = "1 0 0 0 1 0 0 0 1";
	}

	found = GetString( key, defaultString, &s );
	out.Identity();		// sccanf has a bug in it on Mac OS 9.  Sigh.
	sscanf( s, "%f %f %f %f %f %f %f %f %f", &out[0].x, &out[0].y, &out[0].z, &out[1].x, &out[1].y, &out[1].z, &out[2].x, &out[2].y, &out[2].z );
	return found;
}

/*
================
WriteString
================
*/
static void WriteString( const char* s, idFile* f )
{
	int	len = strlen( s );
	if( len >= MAX_STRING_CHARS - 1 )
	{
		idLib::common->Error( "idDict::WriteToFileHandle: bad string" );
	}
	f->Write( s, strlen( s ) + 1 );
}

/*
================
idDict::FindKey
================
*/
const idKeyValue* idDict::FindKey( const char* key ) const
{
	int i, hash;

	if( key == NULL || key[0] == '\0' )
	{
		idLib::common->DWarning( "idDict::FindKey: empty key" );
		return NULL;
	}

	hash = argHash.GenerateKey( key, false );
	for( i = argHash.First( hash ); i != -1; i = argHash.Next( i ) )
	{
		if( args[i].GetKey().Icmp( key ) == 0 )
		{
			return &args[i];
		}
	}

	return NULL;
}

/*
================
idDict::FindKeyIndex
================
*/
int idDict::FindKeyIndex( const char* key ) const
{

	if( key == NULL || key[0] == '\0' )
	{
		idLib::common->DWarning( "idDict::FindKeyIndex: empty key" );
		return 0;
	}

	int hash = argHash.GenerateKey( key, false );
	for( int i = argHash.First( hash ); i != -1; i = argHash.Next( i ) )
	{
		if( args[i].GetKey().Icmp( key ) == 0 )
		{
			return i;
		}
	}

	return -1;
}

/*
================
idDict::Delete
================
*/
void idDict::Delete( const char* key )
{
	int hash, i;

	hash = argHash.GenerateKey( key, false );
	for( i = argHash.First( hash ); i != -1; i = argHash.Next( i ) )
	{
		if( args[i].GetKey().Icmp( key ) == 0 )
		{
			globalKeys.FreeString( args[i].key );
			globalValues.FreeString( args[i].value );
			args.RemoveIndex( i );
			argHash.RemoveIndex( hash, i );
			break;
		}
	}

#if 0
	// make sure all keys can still be found in the hash index
	for( i = 0; i < args.Num(); i++ )
	{
		assert( FindKey( args[i].GetKey() ) != NULL );
	}
#endif
}

// RB
void idDict::DeleteEmptyKeys()
{
	idList<idKeyValue> orig = args;

	for( int i = 0; i < orig.Num(); i++ )
	{
		const idKeyValue& kv = orig[ i ];

		if( kv.GetValue().Length() == 0 )
		{
			Delete( kv.GetKey() );
		}
	}
}

/*
================
idDict::MatchPrefix
================
*/
const idKeyValue* idDict::MatchPrefix( const char* prefix, const idKeyValue* lastMatch ) const
{
	int	i;
	int len;
	int start;

	assert( prefix );
	len = strlen( prefix );

	start = -1;
	if( lastMatch )
	{
		start = args.FindIndex( *lastMatch );
		assert( start >= 0 );
		if( start < 1 )
		{
			start = 0;
		}
	}

	for( i = start + 1; i < args.Num(); i++ )
	{
		if( !args[i].GetKey().Icmpn( prefix, len ) )
		{
			return &args[i];
		}
	}
	return NULL;
}

/*
================
idDict::RandomPrefix
================
*/
const char* idDict::RandomPrefix( const char* prefix, idRandom& random ) const
{
	int count;
	const int MAX_RANDOM_KEYS = 2048;
	const char* list[MAX_RANDOM_KEYS];
	const idKeyValue* kv;

	list[0] = "";
	for( count = 0, kv = MatchPrefix( prefix ); kv != NULL && count < MAX_RANDOM_KEYS; kv = MatchPrefix( prefix, kv ) )
	{
		list[count++] = kv->GetValue().c_str();
	}
	return list[random.RandomInt( count )];
}

/*
================
idDict::WriteToFileHandle
================
*/
void idDict::WriteToFileHandle( idFile* f ) const
{
	int c = LittleLong( args.Num() );
	f->Write( &c, sizeof( c ) );
	for( int i = 0; i < args.Num(); i++ )  	// don't loop on the swapped count use the original
	{
		WriteString( args[i].GetKey().c_str(), f );
		WriteString( args[i].GetValue().c_str(), f );
	}
}



// RB begin
void idDict::WriteJSON( idFile* f, const char* prefix ) const
{
	//f->Printf( "%s[\n", prefix );

	//f->Printf( "%s\"dict\": {\n", prefix );

	idStr key;

	for( int i = 0; i < args.Num(); i++ )  	// don't loop on the swapped count use the original
	{
		//f->Printf( "%s{\"key\": \"%s\", \"value\": \"%s\"},\n", prefix, args[i].GetKey().c_str(), args[i].GetValue().c_str() );

		key = args[i].GetKey();

		//for( int j = 0; j < key.Length(); j++ )
		key.ReplaceChar( '\t', ' ' );

		f->Printf( "%s\t\"%s\": \"%s\"%s\n", prefix, key.c_str(), args[i].GetValue().c_str(), ( i == ( args.Num() - 1 ) ) ? "" : "," );
	}

	//f->Printf( "%s}\n", prefix );
}
// RB end

/*
================
ReadString
================
*/
static idStr ReadString( idFile* f )
{
	char	str[MAX_STRING_CHARS];
	int		len;

	for( len = 0; len < MAX_STRING_CHARS; len++ )
	{
		f->Read( ( void* )&str[len], 1 );
		if( str[len] == 0 )
		{
			break;
		}
	}
	if( len == MAX_STRING_CHARS )
	{
		idLib::common->Error( "idDict::ReadFromFileHandle: bad string" );
	}

	return idStr( str );
}

/*
================
idDict::ReadFromFileHandle
================
*/
void idDict::ReadFromFileHandle( idFile* f )
{
	int c;
	idStr key, val;

	Clear();

	f->Read( &c, sizeof( c ) );
	c = LittleLong( c );
	for( int i = 0; i < c; i++ )
	{
		key = ReadString( f );
		val = ReadString( f );
		Set( key, val );
	}
}

/*
========================
idDict::Serialize
========================
*/
void idDict::Serialize( idSerializer& ser )
{
	if( ser.IsReading() )
	{
		Clear();
	}

	int num = args.Num();
	ser.SerializePacked( num );
	for( int i = 0; i < num; i++ )
	{
		idStr key;
		idStr val;

		if( ser.IsWriting() )
		{
			key = args[i].GetKey();
			val = args[i].GetValue();
		}

		ser.SerializeString( key );
		ser.SerializeString( val );

		if( ser.IsReading() )
		{
			Set( key.c_str(), val.c_str() );
		}
	}
}

/*
================
idDict::WriteToIniFile
================
*/
void idDict::WriteToIniFile( idFile* f ) const
{
	// make a copy so we don't affect the checksum of the original dict
	idList< idKeyValue > sortedArgs( args );
	sortedArgs.SortWithTemplate( idSort_KeyValue() );

	idList< idStr > prefixList;
	idTempArray< int > prefixIndex( sortedArgs.Num() );	// for each keyValue in the args, this is an index into which prefix it uses.
	// 0 means no prefix, otherwise, it's an index + (-1) into prefixList
	// we do this so we can print all the non-prefix based pairs first
	idStr prevPrefix = "";
	idStr skipFirstLine = "";

	// Scan for all the prefixes
	for( int i = 0; i < sortedArgs.Num(); i++ )
	{
		const idKeyValue* kv = &sortedArgs[i];
		int slashPosition = kv->GetKey().Last( '/' );
		if( slashPosition != idStr::INVALID_POSITION )
		{
			idStr prefix = kv->GetKey().Mid( 0, slashPosition );
			if( prefix != prevPrefix )
			{
				prevPrefix = prefix;
				prefixList.Append( prefix );
			}
			prefixIndex[i] = prefixList.Num();
		}
		else
		{
			prefixIndex[i] = 0;

			// output all the prefix-less first
			idStr str = va( "%s=%s\n", kv->GetKey().c_str(), idStr::CStyleQuote( kv->GetValue() ) );
			f->Write( ( void* )str.c_str(), str.Length() );

			skipFirstLine = "\n";
		}
	}

	int prevPrefixIndex = 0;
	int prefixLength = 0;

	// output all the rest without their prefix
	for( int i = 0; i < sortedArgs.Num(); i++ )
	{
		if( prefixIndex[i] == 0 )
		{
			continue;
		}

		if( prefixIndex[i] != prevPrefixIndex )
		{
			prevPrefixIndex = prefixIndex[i];
			prefixLength = prefixList[prevPrefixIndex - 1].Length() + 1; // to skip past the '/' too

			// output prefix
			idStr str = va( "%s[%s]\n", skipFirstLine.c_str(), prefixList[prevPrefixIndex - 1].c_str() );
			f->Write( ( void* )str.c_str(), str.Length() );
		}

		const idKeyValue* kv = &sortedArgs[i];
		idStr str = va( "%s=%s\n", kv->GetKey().c_str() + prefixLength, idStr::CStyleQuote( kv->GetValue() ) );
		f->Write( ( void* )str.c_str(), str.Length() );
	}
}

/*
================
idDict::ReadFromIniFile
================
*/
bool idDict::ReadFromIniFile( idFile* f )
{
	int length = f->Length();
	idTempArray< char > buffer( length );
	if( ( int )f->Read( buffer.Ptr(), length ) != length )
	{
		return false;
	}
	buffer[length - 1] = '\0';	// Since the .ini files are not null terminated, make sure we mark where the end of the .ini file is in our read buffer

	idLexer parser( LEXFL_NOFATALERRORS | LEXFL_ALLOWPATHNAMES /*| LEXFL_ONLYSTRINGS */ );
	idStr name = f->GetName();
	name.Append( " dictionary INI reader" );
	if( !parser.LoadMemory( ( const char* )buffer.Ptr(), length, name.c_str() ) )
	{
		return false;
	}

	idToken	token;
	idToken	token2;
	idStr prefix = "";
	idStr valueStr;
	bool success = true;

	Clear();

	const punctuation_t ini_punctuations[] =
	{
		{ "[", P_SQBRACKETOPEN },
		{ "]", P_SQBRACKETCLOSE },
		{ "=", P_ASSIGN },
		{ NULL, 0 }
	};
	parser.SetPunctuations( ini_punctuations );

	while( success && !parser.EndOfFile() )
	{
		if( parser.PeekTokenType( TT_PUNCTUATION, P_SQBRACKETOPEN, &token ) )
		{
			success = success && parser.ExpectTokenType( TT_PUNCTUATION, P_SQBRACKETOPEN, &token );
			success = success && parser.ReadToken( &token );
			prefix = token.c_str();
			prefix.Append( '/' );
			success = success && parser.ExpectTokenType( TT_PUNCTUATION, P_SQBRACKETCLOSE, &token );
		}

		if( !parser.PeekTokenType( TT_NAME, 0, &token ) )
		{
			// end of file most likely
			break;
		}

		success = success && parser.ExpectTokenType( TT_NAME, 0, &token );
		success = success && parser.ExpectTokenType( TT_PUNCTUATION, P_ASSIGN, &token2 );
		success = success && ( parser.ParseRestOfLine( valueStr ) != NULL );

		valueStr = idStr::CStyleUnQuote( valueStr );

		idStr key = va( "%s%s", prefix.c_str(), token.c_str() );
		if( FindKey( key.c_str() ) )
		{
			parser.Warning( "'%s' already defined", key.c_str() );
		}

		Set( key.c_str(), valueStr.c_str() );
	}

	return success;
}

CONSOLE_COMMAND( TestDictIniFile, "Tests the writing/reading of various items in a dict to/from an ini file", 0 )
{
	// Write to the file
	idFile* file = fileSystem->OpenFileWrite( "idDict_ini_test.ini" );
	if( file == NULL )
	{
		idLib::Printf( "[^1FAILED^0] Couldn't open file for writing.\n" );
		return;
	}

	idDict vars;
	vars.SetInt( "section1/section3/a", -1 );
	vars.SetInt( "section1/section3/b", 0 );
	vars.SetInt( "section1/section3/c", 3 );
	vars.SetFloat( "section2/d", 4.0f );
	vars.SetFloat( "section2/e", -5.0f );
	vars.SetBool( "section2/f", true );
	vars.SetBool( "section1/g", false );
	vars.Set( "section1/h", "test1" );
	vars.Set( "i", "1234" );
	vars.SetInt( "j", 9 );
	vars.WriteToIniFile( file );
	delete file;

	// Read from the file
	file = fileSystem->OpenFileRead( "idDict_ini_test.ini" );
	if( file == NULL )
	{
		idLib::Printf( "[^1FAILED^0] Couldn't open file for reading.\n" );
	}

	idDict readVars;
	readVars.ReadFromIniFile( file );
	delete file;

	if( vars.Checksum() != readVars.Checksum() )
	{
		idLib::Printf( "[^1FAILED^0] Dictionaries do not match.\n" );
	}
	else
	{
		idLib::Printf( "[^2PASSED^0] Dictionaries match.\n" );
	}

	// Output results
	for( int i = 0; i < readVars.GetNumKeyVals(); i++ )
	{
		const idKeyValue* kv = readVars.GetKeyVal( i );
		idLib::Printf( "%s=%s\n", kv->GetKey().c_str(), kv->GetValue().c_str() );
	}
}

/*
================
idDict::Init
================
*/
void idDict::Init()
{
	globalKeys.SetCaseSensitive( false );
	globalValues.SetCaseSensitive( true );
}

/*
================
idDict::Shutdown
================
*/
void idDict::Shutdown()
{
	globalKeys.Clear();
	globalValues.Clear();
}

/*
================
idDict::ShowMemoryUsage_f
================
*/
void idDict::ShowMemoryUsage_f( const idCmdArgs& args )
{
	idLib::common->Printf( "%5d KB in %d keys\n", globalKeys.Size() >> 10, globalKeys.Num() );
	idLib::common->Printf( "%5d KB in %d values\n", globalValues.Size() >> 10, globalValues.Num() );
}

/*
================
idDict::ListKeys_f
================
*/
void idDict::ListKeys_f( const idCmdArgs& args )
{
	idLib::Printf( "Not implemented due to sort impl issues.\n" );
	//int i;
	//idList<const idPoolStr *> keyStrings;

	//for ( i = 0; i < globalKeys.Num(); i++ ) {
	//	keyStrings.Append( globalKeys[i] );
	//}
	//keyStrings.SortWithTemplate( idSort_PoolStrPtr() );
	//for ( i = 0; i < keyStrings.Num(); i++ ) {
	//	idLib::common->Printf( "%s\n", keyStrings[i]->c_str() );
	//}
	//idLib::common->Printf( "%5d keys\n", keyStrings.Num() );
}

/*
================
idDict::ListValues_f
================
*/
void idDict::ListValues_f( const idCmdArgs& args )
{
	idLib::Printf( "Not implemented due to sort impl issues.\n" );
	//int i;
	//idList<const idPoolStr *> valueStrings;

	//for ( i = 0; i < globalValues.Num(); i++ ) {
	//	valueStrings.Append( globalValues[i] );
	//}
	//valueStrings.SortWithTemplate( idSort_PoolStrPtr() );
	//for ( i = 0; i < valueStrings.Num(); i++ ) {
	//	idLib::common->Printf( "%s\n", valueStrings[i]->c_str() );
	//}
	//idLib::common->Printf( "%5d values\n", valueStrings.Num() );
}
