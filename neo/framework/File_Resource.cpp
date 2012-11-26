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

#include "../idlib/precompiled.h"
#pragma hdrstop

/*
================================================================================================

idResourceContainer

================================================================================================
*/

/*
========================
idResourceContainer::ReOpen 
========================
*/ 
void idResourceContainer::ReOpen() {
	delete resourceFile;
	resourceFile = fileSystem->OpenFileRead( fileName );
}

/*
========================
idResourceContainer::Init 
========================
*/ 
bool idResourceContainer::Init( const char *_fileName, uint8 containerIndex ) {

	if ( idStr::Icmp( _fileName, "_ordered.resources" ) == 0 ) {
		resourceFile = fileSystem->OpenFileReadMemory( _fileName );
	} else {
		resourceFile = fileSystem->OpenFileRead( _fileName );
	}

	if ( resourceFile == NULL ) {
		idLib::Warning( "Unable to open resource file %s", _fileName );
		return false;
	}

	resourceFile->ReadBig( resourceMagic );
	if ( resourceMagic != RESOURCE_FILE_MAGIC ) {
		idLib::FatalError( "resourceFileMagic != RESOURCE_FILE_MAGIC" );
	}

	fileName = _fileName;

	resourceFile->ReadBig( tableOffset );
	resourceFile->ReadBig( tableLength );
	// read this into a memory buffer with a single read
	char * const buf = (char *)Mem_Alloc( tableLength, TAG_RESOURCE );
	resourceFile->Seek( tableOffset, FS_SEEK_SET );
	resourceFile->Read( buf, tableLength );
	idFile_Memory memFile( "resourceHeader", (const char *)buf, tableLength );

	// Parse the resourceFile header, which includes every resource used
	// by the game.
	memFile.ReadBig( numFileResources );

	cacheTable.SetNum( numFileResources );

	for ( int i = 0; i < numFileResources; i++ ) {
		idResourceCacheEntry &rt = cacheTable[ i ];
		rt.Read( &memFile );
		rt.filename.BackSlashesToSlashes();
		rt.filename.ToLower();
		rt.containerIndex = containerIndex;

		const int key = cacheHash.GenerateKey( rt.filename, false );
		bool found = false;
		//for ( int index = cacheHash.GetFirst( key ); index != idHashIndex::NULL_INDEX; index = cacheHash.GetNext( index ) ) {
		//	idResourceCacheEntry & rtc = cacheTable[ index ];
		//	if ( idStr::Icmp( rtc.filename, rt.filename ) == 0 ) {
		//		found = true;
		//		break;
		//	}
		//}
		if ( !found ) {
			//idLib::Printf( "rez file name: %s\n", rt.filename.c_str() );
			cacheHash.Add( key, i );
		}
	}
	Mem_Free( buf );

	return true;
}


/*
========================
idResourceContainer::WriteManifestFile 
========================
*/ 
void idResourceContainer::WriteManifestFile( const char *name, const idStrList &list ) {
	idStr filename( name );
	filename.SetFileExtension( "manifest" );
	filename.Insert( "maps/", 0 );
	idFile *outFile = fileSystem->OpenFileWrite( filename );
	if ( outFile != NULL ) {
		int num = list.Num();
		outFile->WriteBig( num );
		for ( int i = 0; i < num; i++ ) {
			outFile->WriteString( list[ i ] );
		}
		delete outFile;
	}
}

/*
========================
idResourceContainer::ReadManifestFile 
========================
*/ 
int idResourceContainer::ReadManifestFile( const char *name, idStrList &list ) {
	idFile *inFile = fileSystem->OpenFileRead( name );
	if ( inFile != NULL ) {
		list.SetGranularity( 16384 );
		idStr str;
		int num;
		list.Clear();
		inFile->ReadBig( num );
		for ( int i = 0; i < num; i++ ) {
			inFile->ReadString( str );
			list.Append( str );
		}
		delete inFile;
	}
	return list.Num();
}


/*
========================
idResourceContainer::UpdateResourceFile 
========================
*/ 
void idResourceContainer::UpdateResourceFile( const char *_filename, const idStrList &_filesToUpdate ) {
	idFile *outFile = fileSystem->OpenFileWrite( va( "%s.new", _filename ) );
	if ( outFile == NULL ) {
		idLib::Warning( "Unable to open resource file %s or new output file", _filename );
		return;
	}

	uint32 magic = 0;
	int _tableOffset = 0;
	int _tableLength = 0;
	idList< idResourceCacheEntry > entries;
	idStrList filesToUpdate = _filesToUpdate;

	idFile *inFile = fileSystem->OpenFileRead( _filename );
	if ( inFile == NULL ) {
		magic = RESOURCE_FILE_MAGIC;

		outFile->WriteBig( magic );
		outFile->WriteBig( _tableOffset );
		outFile->WriteBig( _tableLength );

	} else {
		inFile->ReadBig( magic );
		if ( magic != RESOURCE_FILE_MAGIC ) {
			delete inFile;
			return;
		}

		inFile->ReadBig( _tableOffset );
		inFile->ReadBig( _tableLength );
		// read this into a memory buffer with a single read
		char * const buf = (char *)Mem_Alloc( _tableLength, TAG_RESOURCE );
		inFile->Seek( _tableOffset, FS_SEEK_SET );
		inFile->Read( buf, _tableLength );
		idFile_Memory memFile( "resourceHeader", (const char *)buf, _tableLength );

		int _numFileResources = 0;
		memFile.ReadBig( _numFileResources );

		outFile->WriteBig( magic );
		outFile->WriteBig( _tableOffset );
		outFile->WriteBig( _tableLength );

		entries.SetNum( _numFileResources );

		for ( int i = 0; i < _numFileResources; i++ ) {
			entries[ i ].Read( &memFile );


			idLib::Printf( "examining %s\n", entries[ i ].filename.c_str() );
			byte * fileData = NULL;

			for ( int j = filesToUpdate.Num() - 1; j >= 0; j-- ) {
				if ( filesToUpdate[ j ].Icmp( entries[ i ].filename ) == 0 ) {
					idFile *newFile = fileSystem->OpenFileReadMemory( filesToUpdate[ j ] );
					if ( newFile != NULL ) {
						idLib::Printf( "Updating %s\n", filesToUpdate[ j ].c_str() );
						entries[ i ].length = newFile->Length();
						fileData = (byte *)Mem_Alloc( entries[ i ].length, TAG_TEMP );
						newFile->Read( fileData, newFile->Length() );
						delete newFile;
					}
					filesToUpdate.RemoveIndex( j );
				}
			}

			if ( fileData == NULL ) {
				inFile->Seek( entries[ i ].offset, FS_SEEK_SET );
				fileData = (byte *)Mem_Alloc( entries[ i ].length, TAG_TEMP );
				inFile->Read( fileData, entries[ i ].length );
			}

			entries[ i ].offset = outFile->Tell();
			outFile->Write( ( void* )fileData, entries[ i ].length );

			Mem_Free( fileData );
		}

		Mem_Free( buf );
	}

	while ( filesToUpdate.Num() > 0 ) {
		idFile *newFile = fileSystem->OpenFileReadMemory( filesToUpdate[ 0 ] );
		if ( newFile != NULL ) {
			idLib::Printf( "Appending %s\n", filesToUpdate[ 0 ].c_str() );
			idResourceCacheEntry rt;
			rt.filename = filesToUpdate[ 0 ];
			rt.length = newFile->Length();
			byte * fileData = (byte *)Mem_Alloc( rt.length, TAG_TEMP );
			newFile->Read( fileData, rt.length );
			int idx = entries.Append( rt );
			if ( idx >= 0 ) {
				entries[ idx ].offset = outFile->Tell();
				outFile->Write( ( void* )fileData, entries[ idx ].length );
			}
			delete newFile;
			Mem_Free( fileData );
		}
		filesToUpdate.RemoveIndex( 0 );
	}

	_tableOffset = outFile->Tell();
	outFile->WriteBig( entries.Num() );

	// write the individual resource entries
	for ( int i = 0; i < entries.Num(); i++ ) {
		entries[ i ].Write( outFile );
	}

	// go back and write the header offsets again, now that we have file offsets and lengths
	_tableLength = outFile->Tell() - _tableOffset;
	outFile->Seek( 0, FS_SEEK_SET );
	outFile->WriteBig( magic );
	outFile->WriteBig( _tableOffset );
	outFile->WriteBig( _tableLength );

	delete outFile;
	delete inFile;
}


/*
========================
idResourceContainer::ExtractResourceFile 
========================
*/ 
void idResourceContainer::SetContainerIndex( const int & _idx ) {
	for ( int i = 0; i < cacheTable.Num(); i++ ) {
		cacheTable[ i ].containerIndex = _idx;
	}
}

/*
========================
idResourceContainer::ExtractResourceFile 
========================
*/ 
void idResourceContainer::ExtractResourceFile ( const char * _fileName, const char * _outPath, bool _copyWavs ) {
	idFile *inFile = fileSystem->OpenFileRead( _fileName );

	if ( inFile == NULL ) {
		idLib::Warning( "Unable to open resource file %s", _fileName );
		return;
	}

	uint32 magic;
	inFile->ReadBig( magic );
	if ( magic != RESOURCE_FILE_MAGIC ) {
		delete inFile;
		return;
	}

	int _tableOffset;
	int _tableLength;
	inFile->ReadBig( _tableOffset );
	inFile->ReadBig( _tableLength );
	// read this into a memory buffer with a single read
	char * const buf = (char *)Mem_Alloc( _tableLength, TAG_RESOURCE );
	inFile->Seek( _tableOffset, FS_SEEK_SET );
	inFile->Read( buf, _tableLength );
	idFile_Memory memFile( "resourceHeader", (const char *)buf, _tableLength );

	int _numFileResources;
	memFile.ReadBig( _numFileResources );

	for ( int i = 0; i < _numFileResources; i++ ) {
		idResourceCacheEntry rt;
		rt.Read( &memFile );
		rt.filename.BackSlashesToSlashes();
		rt.filename.ToLower();
		byte *fbuf = NULL;
		if ( _copyWavs && ( rt.filename.Find( ".idwav" ) >= 0 ||  rt.filename.Find( ".idxma" ) >= 0 ||  rt.filename.Find( ".idmsf" ) >= 0 ) ) {
			rt.filename.SetFileExtension( "wav" );
			rt.filename.Replace( "generated/", "" );
			int len = fileSystem->GetFileLength( rt.filename );
			fbuf =  (byte *)Mem_Alloc( len, TAG_RESOURCE );
			fileSystem->ReadFile( rt.filename, (void**)&fbuf, NULL );
		} else {
			inFile->Seek( rt.offset, FS_SEEK_SET );
			fbuf =  (byte *)Mem_Alloc( rt.length, TAG_RESOURCE );
			inFile->Read( fbuf, rt.length );
		}
		idStr outName = _outPath;
		outName.AppendPath( rt.filename );
		idFile *outFile = fileSystem->OpenExplicitFileWrite( outName );
		if ( outFile != NULL ) {
			outFile->Write( ( byte* )fbuf, rt.length );
			delete outFile;
		}
		Mem_Free( fbuf );
	}
	delete inFile;
	Mem_Free( buf );
}



/*
========================
idResourceContainer::Open 
========================
*/ 
void idResourceContainer::WriteResourceFile( const char *manifestName, const idStrList &manifest, const bool &_writeManifest ) {

	if ( manifest.Num() == 0 ) {
		return;
	}

	idLib::Printf( "Writing resource file %s\n", manifestName );

	// build multiple output files at 1GB each
	idList < idStrList > outPutFiles;

	idFileManifest outManifest;
	int64 size = 0;
	idStrList flist;
	flist.SetGranularity( 16384 );
	for ( int i = 0; i < manifest.Num(); i++ ) {
		flist.Append( manifest[ i ] );
		size += fileSystem->GetFileLength( manifest[ i ] );
		if ( size > 1024 * 1024 * 1024 ) {
			outPutFiles.Append( flist );
			size = 0;
			flist.Clear();
		}
		outManifest.AddFile( manifest[ i ] );
	}

	outPutFiles.Append( flist );

	if ( _writeManifest ) {
		idStr temp = manifestName;
		temp.Replace( "maps/", "manifests/" );
		temp.StripFileExtension();
		temp.SetFileExtension( "manifest" );
		outManifest.WriteManifestFile( temp );
	}

	for ( int idx = 0; idx < outPutFiles.Num(); idx++ ) {

		idStrList &fileList = outPutFiles[ idx ];
		if ( fileList.Num() == 0 ) {
			continue;
		}

		idStr fileName = manifestName;
		if ( idx > 0 ) {
			fileName = va( "%s_%02d", manifestName, idx );
		} 
		fileName.SetFileExtension( "resources" );

		idFile *resFile = fileSystem->OpenFileWrite( fileName );

		if ( resFile == NULL ) {
			idLib::Warning( "Cannot open %s for writing.\n", fileName.c_str() );
			return;
		}

		idLib::Printf( "Writing resource file %s\n", fileName.c_str() );

		int	tableOffset = 0;
		int	tableLength = 0;
		int	tableNewLength = 0;
		uint32	resourceFileMagic = RESOURCE_FILE_MAGIC;

		resFile->WriteBig( resourceFileMagic );
		resFile->WriteBig( tableOffset );
		resFile->WriteBig( tableLength );

		idList< idResourceCacheEntry > entries;

		entries.Resize( fileList.Num() );

		for ( int i = 0; i < fileList.Num(); i++ ) {
			idResourceCacheEntry ent;

			ent.filename = fileList[ i ];
			ent.length = 0;
			ent.offset = 0;

			idFile *file = fileSystem->OpenFileReadMemory( ent.filename, false );
			idFile_Memory *fm = dynamic_cast< idFile_Memory* >( file );
			if ( fm == NULL ) {
				continue;
			}
			// if the entry is uncompressed, align the file pointer to a 16 byte boundary
			// so it will be usable if memory mapped
			ent.length = fm->Length();

			// always get the offset, even if the file will have zero length
			ent.offset = resFile->Tell();

			entries.Append( ent );

			if ( ent.length == 0 ) {
				ent.filename = "";
				delete fm;
				continue;
			}

			resFile->Write( fm->GetDataPtr(), ent.length );

			delete fm;

			// pacifier every ten megs
			if ( ( ent.offset + ent.length ) / 10000000 != ent.offset / 10000000 ) {
				idLib::Printf( "." );
			}
		}

		idLib::Printf( "\n" );

		// write the table out now that we have all the files
		tableOffset = resFile->Tell();

		// count how many we are going to write for this platform
		int	numFileResources = entries.Num();

		resFile->WriteBig( numFileResources );

		// write the individual resource entries
		for ( int i = 0; i < entries.Num(); i++ ) {
			entries[ i ].Write( resFile );
			if ( i + 1 == numFileResources ) {
				// we just wrote out the last new entry
				tableNewLength = resFile->Tell() - tableOffset;
			}
		}

		// go back and write the header offsets again, now that we have file offsets and lengths
		tableLength = resFile->Tell() - tableOffset;
		resFile->Seek( 0, FS_SEEK_SET );
		resFile->WriteBig( resourceFileMagic );
		resFile->WriteBig( tableOffset );
		resFile->WriteBig( tableLength );
		delete resFile;
	}
}
