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


/*
================================================================================================

idPreloadManifest

================================================================================================
*/

/*
========================
idPreloadManifest::LoadManifest
========================
*/
bool idPreloadManifest::LoadManifest( const char* fileName )
{
	idFile* inFile = fileSystem->OpenFileReadMemory( fileName );
	if( inFile != NULL )
	{
		int numEntries;
		inFile->ReadBig( numEntries );
		inFile->ReadString( filename );
		entries.SetNum( numEntries );
		for( int i = 0; i < numEntries; i++ )
		{
			entries[ i ].Read( inFile );
		}
		delete inFile;
		return true;
	}
	return false;
}

/*
================================================================================================

idFileManifest

================================================================================================
*/
/*
========================
idFileManifest::LoadManifest
========================
*/
bool idFileManifest::LoadManifest( const char* _fileName )
{
	idFile* file = fileSystem->OpenFileRead( _fileName , false );
	if( file != NULL )
	{
		return LoadManifestFromFile( file );
	}
	return false;
}

/*
========================
idFileManifest::LoadManifestFromFile

// this will delete the file when finished
========================
*/
bool idFileManifest::LoadManifestFromFile( idFile* file )
{
	if( file == NULL )
	{
		return false;
	}
	filename = file->GetName();
	idStr str;
	int num;
	file->ReadBig( num );
	cacheTable.SetNum( num );
	for( int i = 0; i < num; i++ )
	{
		file->ReadString( cacheTable[ i ] );
		//if ( FindFile( cacheTable[ i ].filename ) == NULL ) {
		// we only care about the first usage
		const int key = cacheHash.GenerateKey( cacheTable[ i ], false );
		cacheHash.Add( key, i );
		//}
	}
	delete file;
	return true;
}

/*
========================
idFileManifest::WriteManifestFile
========================
*/
void idFileManifest::WriteManifestFile( const char* fileName )
{
	idFile* file = fileSystem->OpenFileWrite( fileName );
	if( file == NULL )
	{
		return;
	}
	idStr str;
	int num = cacheTable.Num();
	file->WriteBig( num );
	for( int i = 0; i < num; i++ )
	{
		file->WriteString( cacheTable[ i ] );
	}
	delete file;
}

/*
========================
idPreloadManifest::WriteManifestFile
========================
*/
void idPreloadManifest::WriteManifest( const char* fileName )
{
	idFile* file = fileSystem->OpenFileWrite( fileName, "fs_savepath" );
	if( file != NULL )
	{
		WriteManifestToFile( file );
		delete file;
	}
}

/*
========================
idFileManifest::FindFile
========================
*/
int idFileManifest::FindFile( const char* fileName )
{
	const int key = cacheHash.GenerateKey( fileName, false );
	for( int index = cacheHash.GetFirst( key ); index != idHashIndex::NULL_INDEX; index = cacheHash.GetNext( index ) )
	{
		if( idStr::Icmp( cacheTable[ index ], fileName ) == 0 )
		{
			return index;
		}
	}
	return -1;
}

/*
========================
idFileManifest::RemoveAll
========================
*/
void idFileManifest::RemoveAll( const char* _fileName )
{
	for( int i = 0; i < cacheTable.Num(); i++ )
	{
		if( cacheTable[ i ].Icmp( _fileName ) == 0 )
		{
			const int key = cacheHash.GenerateKey( cacheTable[ i ], false );
			cacheTable.RemoveIndex( i );
			cacheHash.RemoveIndex( key, i );
			i--;
		}
	}
}


/*
========================
idFileManifest::GetFileNameByIndex
========================
*/
const idStr& idFileManifest::GetFileNameByIndex( int idx ) const
{
	return cacheTable[ idx ];
}



/*
=========================
idFileManifest::AddFile
=========================
*/
void idFileManifest::AddFile( const char* fileName )
{
	//if ( FindFile( fileName ) == NULL ) {
	// we only care about the first usage
	const int key = cacheHash.GenerateKey( fileName, false );
	int idx = cacheTable.Append( fileName );
	cacheHash.Add( key, idx );
	//}
}



