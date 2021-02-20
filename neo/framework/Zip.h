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
#ifndef	__ZIP_H__
#define	__ZIP_H__

#include <zlib.h>

// DG: all the zip access stuff from minizip is now in minizip/zip.h
#ifndef TYPEINFOPROJECT
	#include "libs/zlib/minizip/zip.h"
#endif


/*
================================================
idZipBuilder

simple interface for zipping up a folder of files
by default, the source folder files are added recursively
================================================
*/
class idZipBuilder
{
public:
	idZipBuilder() {}
	~idZipBuilder() {}

public:
	// adds a list of file extensions ( e.g. "bcm|bmodel|" ) to be compressed in the zip file
	void				AddFileFilters( const char* filters );
	// adds a list of file extensions ( e.g. "genmodel|" ) to be added to the zip file, in an uncompressed form
	void				AddUncompressedFileFilters( const char* filters );
	// builds a zip file of all the files in the specified folder, overwriting if necessary
	bool				Build( const char* zipPath, const char* folder, bool cleanFolder );
	// updates a zip file with the files in the specified folder
	bool				Update( const char* zipPath, const char* folder, bool cleanFolder );

	// helper function to zip up all the files and put in a new zip file
	static bool			BuildMapFolderZip( const char* mapFileName );
	// helper function to update a map folder zip for newer files
	static bool			UpdateMapFolderZip( const char* mapFileName );

	// combines multiple in-memory files into a single memory file
	static idFile_Memory* CombineFiles( const idList< idFile_Memory* >& srcFiles );
	// extracts multiple in-memory files from a single memory file
	static bool			ExtractFiles( idFile_Memory*& srcFile, idList< idFile_Memory* >& destFiles );

	void				CleanSourceFolder();

	bool				CreateZipFileFromFileList( const char* name, const idList< idFile_Memory* >& srcFiles );
#ifndef TYPEINFOPROJECT
	zipFile				CreateZipFile( const char* name );
	bool				AddFile( zipFile zf, idFile_Memory* fm, bool deleteFile );
	void				CloseZipFile( zipFile zf );
#endif
private:
	bool				CreateZipFile( bool appendFiles );
	bool				CreateZipFileFromFiles( const idList< idFile_Memory* >& srcFiles );
	bool				GetFileTime( const idStr& filename, unsigned long* dostime ) const;
	bool				IsFiltered( const idStr& filename ) const;
	bool				IsUncompressed( const idStr& filename ) const;

private:
	idStr				zipFileName;				// os path to the zip file
	idStr				sourceFolderName;			// source folder of files to zip or add
	idStrList			filterExts;					// file extensions we want to compressed
	idStrList			uncompressedFilterExts;		// file extensions we don't want to compress
};

#endif	/* __ZIP_H__ */
