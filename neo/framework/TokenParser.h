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

#ifndef __TOKENPARSER_H__
#define __TOKENPARSER_H__
class idBinaryToken
{
public:
	idBinaryToken()
	{
		tokenType = 0;
		tokenSubType = 0;
	}
	idBinaryToken( const idToken& tok )
	{
		token = tok.c_str();
		tokenType = tok.type;
		tokenSubType = tok.subtype;
	}
	bool operator==( const idBinaryToken& b ) const
	{
		return ( tokenType == b.tokenType && tokenSubType == b.tokenSubType && token.Cmp( b.token ) == 0 );
	}
	void Read( idFile* inFile )
	{
		inFile->ReadString( token );
		inFile->ReadBig( tokenType );
		inFile->ReadBig( tokenSubType );
	}
	void Write( idFile* inFile )
	{
		inFile->WriteString( token );
		inFile->WriteBig( tokenType );
		inFile->WriteBig( tokenSubType );
	}
	idStr token;
	int8  tokenType;
	short tokenSubType;
};

class idTokenIndexes
{
public:
	idTokenIndexes() {}
	void Clear()
	{
		tokenIndexes.Clear();
	}
	int Append( short sdx )
	{
		return tokenIndexes.Append( sdx );
	}
	int Num()
	{
		return tokenIndexes.Num();
	}
	void SetNum( int num )
	{
		tokenIndexes.SetNum( num );
	}
	short& 	operator[]( const int index )
	{
		return tokenIndexes[ index ];
	}
	void SetName( const char* name )
	{
		fileName = name;
	}
	const char* GetName()
	{
		return fileName.c_str();
	}
	void Write( idFile* outFile )
	{
		outFile->WriteString( fileName );
		outFile->WriteBig( ( int )tokenIndexes.Num() );
		outFile->WriteBigArray( tokenIndexes.Ptr(), tokenIndexes.Num() );
	}
	void Read( idFile* inFile )
	{
		inFile->ReadString( fileName );
		int num;
		inFile->ReadBig( num );
		tokenIndexes.SetNum( num );
		inFile->ReadBigArray( tokenIndexes.Ptr(), num );
	}
private:
	idList< short > tokenIndexes;
	idStr fileName;
};

class idTokenParser
{
public:
	idTokenParser()
	{
		timeStamp = FILE_NOT_FOUND_TIMESTAMP;
		preloaded = false;
		currentToken = 0;
		currentTokenList = 0;
	}
	~idTokenParser()
	{
		Clear();
	}
	void Clear()
	{
		tokens.Clear();
		guiTokenIndexes.Clear();
		currentToken = 0;
		currentTokenList = -1;
		preloaded = false;
	}
	void LoadFromFile( const char* filename );
	void WriteToFile( const char* filename );
	void LoadFromParser( idParser& parser, const char* guiName );

	bool StartParsing( const char* fileName );
	void DoneParsing()
	{
		currentTokenList = -1;
	}

	bool IsLoaded()
	{
		return tokens.Num() > 0;
	}
	bool ReadToken( idToken* tok );
	int	ExpectTokenString( const char* string );
	int	ExpectTokenType( int type, int subtype, idToken* token );
	int ExpectAnyToken( idToken* token );
	void SetMarker() {}
	void UnreadToken( const idToken* token );
	void Error( VERIFY_FORMAT_STRING const char* str, ... ) ID_INSTANCE_ATTRIBUTE_PRINTF( 1, 2 );
	void Warning( VERIFY_FORMAT_STRING const char* str, ... ) ID_INSTANCE_ATTRIBUTE_PRINTF( 1, 2 );
	int ParseInt();
	bool ParseBool();
	float ParseFloat( bool* errorFlag = NULL );
	void UpdateTimeStamp( ID_TIME_T& t )
	{
		if( t > timeStamp )
		{
			timeStamp = t;
		}
	}
private:
	idList< idBinaryToken > tokens;
	idList< idTokenIndexes > guiTokenIndexes;
	int currentToken;
	int currentTokenList;
	ID_TIME_T timeStamp;
	bool preloaded;
};

#endif /* !__TOKENPARSER_H__ */
