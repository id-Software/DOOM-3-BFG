/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2015 Robert Beckebans

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

#ifndef __DICT_H__
#define __DICT_H__

class idSerializer;

/*
===============================================================================

Key/value dictionary

This is a dictionary class that tracks an arbitrary number of key / value
pair combinations. It is used for map entity spawning, GUI state management,
and other things.

Keys are compared case-insensitive.

Does not allocate memory until the first key/value pair is added.

===============================================================================
*/

class idKeyValue
{
	friend class idDict;

public:
	const idStr& 		GetKey() const
	{
		return *key;
	}
	const idStr& 		GetValue() const
	{
		return *value;
	}

	size_t				Allocated() const
	{
		return key->Allocated() + value->Allocated();
	}
	size_t				Size() const
	{
		return sizeof( *this ) + key->Size() + value->Size();
	}

	bool				operator==( const idKeyValue& kv ) const
	{
		return ( key == kv.key && value == kv.value );
	}

private:
	const idPoolStr* 	key;
	const idPoolStr* 	value;
};

/*
================================================
idSort_KeyValue
================================================
*/
class idSort_KeyValue : public idSort_Quick< idKeyValue, idSort_KeyValue >
{
public:
	int Compare( const idKeyValue& a, const idKeyValue& b ) const
	{
		return a.GetKey().Icmp( b.GetKey() );
	}
};

class idDict
{
public:
	idDict();
	idDict( const idDict& other );	// allow declaration with assignment
	~idDict();

	// set the granularity for the index
	void				SetGranularity( int granularity );

	// set hash size
	void				SetHashSize( int hashSize );

	// clear existing key/value pairs and copy all key/value pairs from other
	idDict& 			operator=( const idDict& other );

	// copy from other while leaving existing key/value pairs in place
	void				Copy( const idDict& other );

	// clear existing key/value pairs and transfer key/value pairs from other
	void				TransferKeyValues( idDict& other );

	// parse dict from parser
	bool				Parse( idParser& parser );

	// copy key/value pairs from other dict not present in this dict
	void				SetDefaults( const idDict* dict );

	// clear dict freeing up memory
	void				Clear();

	// print the dict
	void				Print() const;

	size_t				Allocated() const;
	size_t				Size() const
	{
		return sizeof( *this ) + Allocated();
	}

	void				Set( const char* key, const char* value );
	void				SetFloat( const char* key, float val );
	void				SetInt( const char* key, int val );
	void				SetBool( const char* key, bool val );
	void				SetVector( const char* key, const idVec3& val );
	void				SetVec2( const char* key, const idVec2& val );
	void				SetVec4( const char* key, const idVec4& val );
	void				SetAngles( const char* key, const idAngles& val );
	void				SetMatrix( const char* key, const idMat3& val );

	// these return default values of 0.0, 0 and false
	const char* 		GetString( const char* key, const char* defaultString = "" ) const;
	float				GetFloat( const char* key, const char* defaultString ) const;
	int					GetInt( const char* key, const char* defaultString ) const;
	bool				GetBool( const char* key, const char* defaultString ) const;
	float				GetFloat( const char* key, const float defaultFloat = 0.0f ) const;
	int					GetInt( const char* key, const int defaultInt = 0 ) const;
	bool				GetBool( const char* key, const bool defaultBool = false ) const;
	idVec3				GetVector( const char* key, const char* defaultString = NULL ) const;
	idVec2				GetVec2( const char* key, const char* defaultString = NULL ) const;
	idVec4				GetVec4( const char* key, const char* defaultString = NULL ) const;
	idAngles			GetAngles( const char* key, const char* defaultString = NULL ) const;
	idMat3				GetMatrix( const char* key, const char* defaultString = NULL ) const;

	bool				GetString( const char* key, const char* defaultString, const char** out ) const;
	bool				GetString( const char* key, const char* defaultString, idStr& out ) const;
	bool				GetFloat( const char* key, const char* defaultString, float& out ) const;
	bool				GetInt( const char* key, const char* defaultString, int& out ) const;
	bool				GetBool( const char* key, const char* defaultString, bool& out ) const;
	bool				GetFloat( const char* key, const float defaultFloat, float& out ) const;
	bool				GetInt( const char* key, const int defaultInt, int& out ) const;
	bool				GetBool( const char* key, const bool defaultBool, bool& out ) const;
	bool				GetVector( const char* key, const char* defaultString, idVec3& out ) const;
	bool				GetVec2( const char* key, const char* defaultString, idVec2& out ) const;
	bool				GetVec4( const char* key, const char* defaultString, idVec4& out ) const;
	bool				GetAngles( const char* key, const char* defaultString, idAngles& out ) const;
	bool				GetMatrix( const char* key, const char* defaultString, idMat3& out ) const;

	int					GetNumKeyVals() const;
	const idKeyValue* 	GetKeyVal( int index ) const;

	// returns the key/value pair with the given key
	// returns NULL if the key/value pair does not exist
	const idKeyValue* 	FindKey( const char* key ) const;

	// returns the index to the key/value pair with the given key
	// returns -1 if the key/value pair does not exist
	int					FindKeyIndex( const char* key ) const;

	// delete the key/value pair with the given key
	void				Delete( const char* key );

	// RB: delete all keys with a "" value
	void				DeleteEmptyKeys();

	// finds the next key/value pair with the given key prefix.
	// lastMatch can be used to do additional searches past the first match.
	const idKeyValue* 	MatchPrefix( const char* prefix, const idKeyValue* lastMatch = NULL ) const;

	// randomly chooses one of the key/value pairs with the given key prefix and returns it's value
	const char* 		RandomPrefix( const char* prefix, idRandom& random ) const;

	void				WriteToFileHandle( idFile* f ) const;
	void				ReadFromFileHandle( idFile* f );

	// RB begin
	void				WriteJSON( idFile* f, const char* prefix = "" ) const;
	// RB end

	void				WriteToIniFile( idFile* f ) const;
	bool				ReadFromIniFile( idFile* f );

	void				Serialize( idSerializer& ser );

	// returns a unique checksum for this dictionary's content
	int					Checksum() const;

	static void			Init();
	static void			Shutdown();

	static void			ShowMemoryUsage_f( const idCmdArgs& args );
	static void			ListKeys_f( const idCmdArgs& args );
	static void			ListValues_f( const idCmdArgs& args );

private:
	idList<idKeyValue>	args;
	idHashIndex			argHash;

	static idStrPool	globalKeys;
	static idStrPool	globalValues;
};


ID_INLINE idDict::idDict()
{
	args.SetGranularity( 16 );
	argHash.SetGranularity( 16 );
	argHash.Clear( 128, 16 );
}

ID_INLINE idDict::idDict( const idDict& other )
{
	*this = other;
}

ID_INLINE idDict::~idDict()
{
	Clear();
}

ID_INLINE void idDict::SetGranularity( int granularity )
{
	args.SetGranularity( granularity );
	argHash.SetGranularity( granularity );
}

ID_INLINE void idDict::SetHashSize( int hashSize )
{
	if( args.Num() == 0 )
	{
		argHash.Clear( hashSize, 16 );
	}
}

ID_INLINE void idDict::SetFloat( const char* key, float val )
{
	Set( key, va( "%f", val ) );
}

ID_INLINE void idDict::SetInt( const char* key, int val )
{
	Set( key, va( "%i", val ) );
}

ID_INLINE void idDict::SetBool( const char* key, bool val )
{
	Set( key, va( "%i", val ) );
}

ID_INLINE void idDict::SetVector( const char* key, const idVec3& val )
{
	Set( key, val.ToString() );
}

ID_INLINE void idDict::SetVec4( const char* key, const idVec4& val )
{
	Set( key, val.ToString() );
}

ID_INLINE void idDict::SetVec2( const char* key, const idVec2& val )
{
	Set( key, val.ToString() );
}

ID_INLINE void idDict::SetAngles( const char* key, const idAngles& val )
{
	Set( key, val.ToString() );
}

ID_INLINE void idDict::SetMatrix( const char* key, const idMat3& val )
{
	Set( key, val.ToString() );
}

ID_INLINE bool idDict::GetString( const char* key, const char* defaultString, const char** out ) const
{
	const idKeyValue* kv = FindKey( key );
	if( kv )
	{
		*out = kv->GetValue();
		return true;
	}
	*out = defaultString;
	return false;
}

ID_INLINE bool idDict::GetString( const char* key, const char* defaultString, idStr& out ) const
{
	const idKeyValue* kv = FindKey( key );
	if( kv )
	{
		out = kv->GetValue();
		return true;
	}
	out = defaultString;
	return false;
}

ID_INLINE const char* idDict::GetString( const char* key, const char* defaultString ) const
{
	const idKeyValue* kv = FindKey( key );
	if( kv )
	{
		return kv->GetValue();
	}
	return defaultString;
}

ID_INLINE float idDict::GetFloat( const char* key, const char* defaultString ) const
{
	return atof( GetString( key, defaultString ) );
}

ID_INLINE int idDict::GetInt( const char* key, const char* defaultString ) const
{
	return atoi( GetString( key, defaultString ) );
}

ID_INLINE bool idDict::GetBool( const char* key, const char* defaultString ) const
{
	return ( atoi( GetString( key, defaultString ) ) != 0 );
}

ID_INLINE float idDict::GetFloat( const char* key, const float defaultFloat ) const
{
	const idKeyValue* kv = FindKey( key );
	if( kv )
	{
		return atof( kv->GetValue() );
	}
	return defaultFloat;
}

ID_INLINE int idDict::GetInt( const char* key, int defaultInt ) const
{
	const idKeyValue* kv = FindKey( key );
	if( kv )
	{
		return atoi( kv->GetValue() );
	}
	return defaultInt;
}

ID_INLINE bool idDict::GetBool( const char* key, const bool defaultBool ) const
{
	const idKeyValue* kv = FindKey( key );
	if( kv )
	{
		return atoi( kv->GetValue() ) != 0;
	}
	return defaultBool;
}

ID_INLINE idVec3 idDict::GetVector( const char* key, const char* defaultString ) const
{
	idVec3 out;
	GetVector( key, defaultString, out );
	return out;
}

ID_INLINE idVec2 idDict::GetVec2( const char* key, const char* defaultString ) const
{
	idVec2 out;
	GetVec2( key, defaultString, out );
	return out;
}

ID_INLINE idVec4 idDict::GetVec4( const char* key, const char* defaultString ) const
{
	idVec4 out;
	GetVec4( key, defaultString, out );
	return out;
}

ID_INLINE idAngles idDict::GetAngles( const char* key, const char* defaultString ) const
{
	idAngles out;
	GetAngles( key, defaultString, out );
	return out;
}

ID_INLINE idMat3 idDict::GetMatrix( const char* key, const char* defaultString ) const
{
	idMat3 out;
	GetMatrix( key, defaultString, out );
	return out;
}

ID_INLINE int idDict::GetNumKeyVals() const
{
	return args.Num();
}

ID_INLINE const idKeyValue* idDict::GetKeyVal( int index ) const
{
	if( index >= 0 && index < args.Num() )
	{
		return &args[ index ];
	}
	return NULL;
}

#endif /* !__DICT_H__ */
