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

#ifndef __FILESYSTEM_H__
#define __FILESYSTEM_H__

/*
===============================================================================

	File System

	No stdio calls should be used by any part of the game, because of all sorts
	of directory and separator char issues. Throughout the game a forward slash
	should be used as a separator. The file system takes care of the conversion
	to an OS specific separator. The file system treats all file and directory
	names as case insensitive.

	The following cvars store paths used by the file system:

	"fs_basepath"		path to local install
	"fs_savepath"		path to config, save game, etc. files, read & write

	The base path for file saving can be set to "fs_savepath" or "fs_basepath".

===============================================================================
*/

static const ID_TIME_T	FILE_NOT_FOUND_TIMESTAMP	= ( ID_TIME_T ) - 1;
static const int		MAX_OSPATH					= 256;

// modes for OpenFileByMode
typedef enum
{
	FS_READ			= 0,
	FS_WRITE		= 1,
	FS_APPEND	= 2
} fsMode_t;

typedef enum
{
	FIND_NO,
	FIND_YES
} findFile_t;

// file list for directory listings
class idFileList
{
	friend class idFileSystemLocal;
public:
	const char* 			GetBasePath() const
	{
		return basePath;
	}
	int						GetNumFiles() const
	{
		return list.Num();
	}
	const char* 			GetFile( int index ) const
	{
		return list[index];
	}
	const idStrList& 		GetList() const
	{
		return list;
	}

private:
	idStr					basePath;
	idStrList				list;
};

class idFileSystem
{
public:
	virtual					~idFileSystem() {}

	// Initializes the file system.
	virtual void			Init() = 0;

	// Restarts the file system.
	virtual void			Restart() = 0;

	// Shutdown the file system.
	virtual void			Shutdown( bool reloading ) = 0;

	// Returns true if the file system is initialized.
	virtual bool			IsInitialized() const = 0;

	// Lists files with the given extension in the given directory.
	// Directory should not have either a leading or trailing '/'
	// The returned files will not include any directories or '/' unless fullRelativePath is set.
	// The extension must include a leading dot and may not contain wildcards.
	// If extension is "/", only subdirectories will be returned.
	virtual idFileList* 	ListFiles( const char* relativePath, const char* extension, bool sort = false, bool fullRelativePath = false, const char* gamedir = NULL ) = 0;

	// Lists files in the given directory and all subdirectories with the given extension.
	// Directory should not have either a leading or trailing '/'
	// The returned files include a full relative path.
	// The extension must include a leading dot and may not contain wildcards.
	virtual idFileList* 	ListFilesTree( const char* relativePath, const char* extension, bool sort = false, bool allowSubdirsForResourcePaks = false, const char* gamedir = NULL ) = 0;

	// Frees the given file list.
	virtual void			FreeFileList( idFileList* fileList ) = 0;

	// Converts a relative path to a full OS path.
	virtual const char* 	OSPathToRelativePath( const char* OSPath ) = 0;

	// Converts a full OS path to a relative path.
	virtual const char* 	RelativePathToOSPath( const char* relativePath, const char* basePath = "fs_basepath" ) = 0;

	// Builds a full OS path from the given components.
	virtual const char* 	BuildOSPath( const char* base, const char* game, const char* relativePath ) = 0;
	virtual const char* 	BuildOSPath( const char* base, const char* relativePath ) = 0;

	// Creates the given OS path for as far as it doesn't exist already.
	virtual void			CreateOSPath( const char* OSPath ) = 0;

	// Reads a complete file.
	// Returns the length of the file, or -1 on failure.
	// A null buffer will just return the file length without loading.
	// A null timestamp will be ignored.
	// As a quick check for existance. -1 length == not present.
	// A 0 byte will always be appended at the end, so string ops are safe.
	// The buffer should be considered read-only, because it may be cached for other uses.
	virtual int				ReadFile( const char* relativePath, void** buffer, ID_TIME_T* timestamp = NULL ) = 0;

	// Frees the memory allocated by ReadFile.
	virtual void			FreeFile( void* buffer ) = 0;

	// Writes a complete file, will create any needed subdirectories.
	// Returns the length of the file, or -1 on failure.
	virtual int				WriteFile( const char* relativePath, const void* buffer, int size, const char* basePath = "fs_savepath" ) = 0;

	// Removes the given file.
	virtual void			RemoveFile( const char* relativePath ) = 0;

	// Removes the specified directory.
	virtual	bool			RemoveDir( const char* relativePath ) = 0;

	// Renames a file, taken from idTech5 (minus the fsPath_t)
	virtual bool			RenameFile( const char* relativePath, const char* newName, const char* basePath = "fs_savepath" ) = 0;

	// Opens a file for reading.
	virtual idFile* 		OpenFileRead( const char* relativePath, bool allowCopyFiles = true, const char* gamedir = NULL ) = 0;

	// Opens a file for reading, reads the file completely in memory and returns an idFile_Memory obj.
	virtual idFile* 		OpenFileReadMemory( const char* relativePath, bool allowCopyFiles = true, const char* gamedir = NULL ) = 0;

	// Opens a file for writing, will create any needed subdirectories.
	virtual idFile* 		OpenFileWrite( const char* relativePath, const char* basePath = "fs_savepath" ) = 0;

	// Opens a file for writing at the end.
	virtual idFile* 		OpenFileAppend( const char* filename, bool sync = false, const char* basePath = "fs_basepath" ) = 0;

	// Opens a file for reading, writing, or appending depending on the value of mode.
	virtual idFile* 		OpenFileByMode( const char* relativePath, fsMode_t mode ) = 0;

	// Opens a file for reading from a full OS path.
	virtual idFile* 		OpenExplicitFileRead( const char* OSPath ) = 0;

	// Opens a file for writing to a full OS path.
	virtual idFile* 		OpenExplicitFileWrite( const char* OSPath ) = 0;

	// opens a zip container
	virtual idFile_Cached* 		OpenExplicitPakFile( const char* OSPath ) = 0;

	// Closes a file.
	virtual void			CloseFile( idFile* f ) = 0;

	// look for a dynamic module
	virtual void			FindDLL( const char* basename, char dllPath[ MAX_OSPATH ] ) = 0;

	// don't use for large copies - allocates a single memory block for the copy
	virtual void			CopyFile( const char* fromOSPath, const char* toOSPath ) = 0;

	// look for a file in the loaded paks or the addon paks
	// if the file is found in addons, FS's internal structures are ready for a reloadEngine
	virtual findFile_t		FindFile( const char* path ) = 0;

	// ignore case and seperator char distinctions
	virtual bool			FilenameCompare( const char* s1, const char* s2 ) const = 0;

	// This is just handy
	ID_TIME_T				GetTimestamp( const char* relativePath )
	{
		ID_TIME_T timestamp = FILE_NOT_FOUND_TIMESTAMP;
		if( relativePath == NULL || relativePath[ 0 ] == '\0' )
		{
			return timestamp;
		}
		ReadFile( relativePath, NULL, &timestamp );
		return timestamp;
	}

	// Returns length of file, -1 if no file exists
	virtual int				GetFileLength( const char* relativePath ) = 0;

	virtual sysFolder_t		IsFolder( const char* relativePath, const char* basePath = "fs_basepath" ) = 0;

	// resource tracking and related things
	virtual void			EnableBackgroundCache( bool enable ) = 0;
	virtual void			BeginLevelLoad( const char* name, char* _blockBuffer, int _blockBufferSize ) = 0;
	virtual void			EndLevelLoad() = 0;
	virtual bool			InProductionMode() = 0;
	virtual bool			UsingResourceFiles() = 0;
	virtual void			UnloadMapResources( const char* name ) = 0;
	virtual void			UnloadResourceContainer( const char* name ) = 0;
	virtual void			StartPreload( const idStrList& _preload ) = 0;
	virtual void			StopPreload() = 0;
	virtual int				ReadFromBGL( idFile* _resourceFile, void* _buffer, int _offset, int _len ) = 0;
	virtual bool			IsBinaryModel( const idStr& resName ) const = 0;
	virtual bool			IsSoundSample( const idStr& resName ) const = 0;
	virtual bool			GetResourceCacheEntry( const char* fileName, idResourceCacheEntry& rc ) = 0;
	virtual void			FreeResourceBuffer() = 0;
	virtual void			AddImagePreload( const char* resName, int filter, int repeat, int usage, int cube ) = 0;
	virtual void			AddSamplePreload( const char* resName ) = 0;
	virtual void			AddModelPreload( const char* resName ) = 0;
	virtual void			AddAnimPreload( const char* resName ) = 0;
	virtual void			AddParticlePreload( const char* resName ) = 0;
	virtual void			AddCollisionPreload( const char* resName ) = 0;

};

extern idFileSystem* 		fileSystem;

#endif /* !__FILESYSTEM_H__ */
