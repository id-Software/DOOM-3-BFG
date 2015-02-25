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

#include "precompiled.h"
#pragma hdrstop

#include "Unzip.h"
#include "Zip.h"

#ifdef WIN32
#include <io.h>	// for _read
#else
#if !__MACH__ && __MWERKS__
#include <types.h>
#include <stat.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#endif
#include <unistd.h>
#endif


/*
=============================================================================

DOOM FILESYSTEM

All of Doom's data access is through a hierarchical file system, but the contents of
the file system can be transparently merged from several sources.

A "relativePath" is a reference to game file data, which must include a terminating zero.
"..", "\\", and ":" are explicitly illegal in qpaths to prevent any references
outside the Doom directory system.

The "base path" is the path to the directory holding all the game directories and
usually the executable. It defaults to the current directory, but can be overridden
with "+set fs_basepath c:\doom" on the command line. The base path cannot be modified
at all after startup.

The "save path" is the path to the directory where game files will be saved. It defaults
to the base path, but can be overridden with a "+set fs_savepath c:\doom" on the
command line. Any files that are created during the game (demos, screenshots, etc.) will
be created reletive to the save path.

If a user runs the game directly from a CD, the base path would be on the CD. This
should still function correctly, but all file writes will fail (harmlessly).

The "base game" is the directory under the paths where data comes from by default, and
can be either "base" or "demo".

The "current game" may be the same as the base game, or it may be the name of another
directory under the paths that should be searched for files before looking in the base
game. The game directory is set with "+set fs_game myaddon" on the command line. This is
the basis for addons.

No other directories outside of the base game and current game will ever be referenced by
filesystem functions.

Because we will have updated executables freely available online, there is no point to
trying to restrict demo / oem versions of the game with code changes. Demo / oem versions
should be exactly the same executables as release versions, but with different data that
automatically restricts where game media can come from to prevent add-ons from working.

If the "fs_copyfiles" cvar is set to 1, then every time a file is sourced from the base
path, it will be copied over to the save path. This is a development aid to help build
test releases and to copy working sets of files.

The relative path "sound/newstuff/test.wav" would be searched for in the following places:

for save path, base path:
	for current game, base game:
		search directory
		search zip files

downloaded files, to be written to save path + current game's directory

The filesystem can be safely shutdown and reinitialized with different
basedir / cddir / game combinations, but all other subsystems that rely on it
(sound, video) must also be forced to restart.

"additional mod path search":
fs_game_base can be used to set an additional search path
in search order, fs_game, fs_game_base, BASEGAME
for instance to base a mod of D3 + D3XP assets, fs_game mymod, fs_game_base d3xp

=============================================================================
*/

#define MAX_ZIPPED_FILE_NAME	2048
#define FILE_HASH_SIZE			1024

struct searchpath_t
{
	idStr	path;		// c:\doom
	idStr	gamedir;	// base
};

// search flags when opening a file
#define FSFLAG_SEARCH_DIRS		( 1 << 0 )
#define FSFLAG_RETURN_FILE_MEM	( 1 << 1 )

class idFileSystemLocal : public idFileSystem
{
public:
	idFileSystemLocal();
	
	virtual void			Init();
	virtual void			Restart();
	virtual void			Shutdown( bool reloading );
	virtual bool			IsInitialized() const;
	virtual idFileList* 	ListFiles( const char* relativePath, const char* extension, bool sort = false, bool fullRelativePath = false, const char* gamedir = NULL );
	virtual idFileList* 	ListFilesTree( const char* relativePath, const char* extension, bool sort = false, const char* gamedir = NULL );
	virtual void			FreeFileList( idFileList* fileList );
	virtual const char* 	OSPathToRelativePath( const char* OSPath );
	virtual const char* 	RelativePathToOSPath( const char* relativePath, const char* basePath );
	virtual const char* 	BuildOSPath( const char* base, const char* game, const char* relativePath );
	virtual const char* 	BuildOSPath( const char* base, const char* relativePath );
	virtual void			CreateOSPath( const char* OSPath );
	virtual int				ReadFile( const char* relativePath, void** buffer, ID_TIME_T* timestamp );
	virtual void			FreeFile( void* buffer );
	virtual int				WriteFile( const char* relativePath, const void* buffer, int size, const char* basePath = "fs_savepath" );
	virtual void			RemoveFile( const char* relativePath );
	virtual	bool			RemoveDir( const char* relativePath );
	virtual bool			RenameFile( const char* relativePath, const char* newName, const char* basePath = "fs_savepath" );
	virtual idFile* 		OpenFileReadFlags( const char* relativePath, int searchFlags, bool allowCopyFiles = true, const char* gamedir = NULL );
	virtual idFile* 		OpenFileRead( const char* relativePath, bool allowCopyFiles = true, const char* gamedir = NULL );
	virtual idFile* 		OpenFileReadMemory( const char* relativePath, bool allowCopyFiles = true, const char* gamedir = NULL );
	virtual idFile* 		OpenFileWrite( const char* relativePath, const char* basePath = "fs_savepath" );
	virtual idFile* 		OpenFileAppend( const char* relativePath, bool sync = false, const char* basePath = "fs_basepath" );
	virtual idFile* 		OpenFileByMode( const char* relativePath, fsMode_t mode );
	virtual idFile* 		OpenExplicitFileRead( const char* OSPath );
	virtual idFile* 		OpenExplicitFileWrite( const char* OSPath );
	virtual idFile_Cached* 		OpenExplicitPakFile( const char* OSPath );
	virtual void			CloseFile( idFile* f );
	virtual void			FindDLL( const char* basename, char dllPath[ MAX_OSPATH ] );
	virtual void			CopyFile( const char* fromOSPath, const char* toOSPath );
	virtual findFile_t		FindFile( const char* path );
	virtual bool			FilenameCompare( const char* s1, const char* s2 ) const;
	virtual int				GetFileLength( const char* relativePath );
	virtual sysFolder_t		IsFolder( const char* relativePath, const char* basePath = "fs_basepath" );
	// resource tracking
	virtual void			EnableBackgroundCache( bool enable );
	virtual void			BeginLevelLoad( const char* name, char* _blockBuffer, int _blockBufferSize );
	virtual void			EndLevelLoad();
	
	// RB begin
	virtual bool			InProductionMode();
	// RB end
	virtual bool			UsingResourceFiles()
	{
		return resourceFiles.Num() > 0;
	}
	virtual void			UnloadMapResources( const char* name );
	virtual void			UnloadResourceContainer( const char* name );
	idFile* 				GetResourceContainer( int idx )
	{
		if( idx >= 0 && idx < resourceFiles.Num() )
		{
			return resourceFiles[ idx ]->resourceFile;
		}
		return NULL;
	}
	
	virtual void			StartPreload( const idStrList& _preload );
	virtual void			StopPreload();
	idFile* 				GetResourceFile( const char* fileName, bool memFile );
	bool					GetResourceCacheEntry( const char* fileName, idResourceCacheEntry& rc );
	virtual int				ReadFromBGL( idFile* _resourceFile, void* _buffer, int _offset, int _len );
	virtual bool			IsBinaryModel( const idStr& resName ) const;
	virtual bool			IsSoundSample( const idStr& resName ) const;
	virtual void			FreeResourceBuffer()
	{
		resourceBufferAvailable = resourceBufferSize;
	}
	virtual void			AddImagePreload( const char* resName, int _filter, int _repeat, int _usage, int _cube )
	{
		preloadList.AddImage( resName, _filter, _repeat, _usage, _cube );
	}
	virtual void			AddSamplePreload( const char* resName )
	{
		preloadList.AddSample( resName );
	}
	virtual void			AddModelPreload( const char* resName )
	{
		preloadList.AddModel( resName );
	}
	virtual void			AddAnimPreload( const char* resName )
	{
		preloadList.AddAnim( resName );
	}
	virtual void			AddCollisionPreload( const char* resName )
	{
		preloadList.AddCollisionModel( resName );
	}
	virtual void			AddParticlePreload( const char* resName )
	{
		preloadList.AddParticle( resName );
	}
	
	static void				Dir_f( const idCmdArgs& args );
	static void				DirTree_f( const idCmdArgs& args );
	static void				Path_f( const idCmdArgs& args );
	static void				TouchFile_f( const idCmdArgs& args );
	static void				TouchFileList_f( const idCmdArgs& args );
	static void				BuildGame_f( const idCmdArgs& args );
	//static void				FileStats_f( const idCmdArgs &args );
	static void				WriteResourceFile_f( const idCmdArgs& args );
	static void				ExtractResourceFile_f( const idCmdArgs& args );
	static void				UpdateResourceFile_f( const idCmdArgs& args );
	static void				GenerateResourceCRCs_f( const idCmdArgs& args );
	static void				CreateCRCsForResourceFileList( const idFileList& list );
	
	void					BuildOrderedStartupContainer();
private:
	idList<searchpath_t>	searchPaths;
	int						loadCount;			// total files read
	int						loadStack;			// total files in memory
	idStr					gameFolder;			// this will be a single name without separators
	
	static idCVar			fs_debug;
	static idCVar			fs_debugResources;
	static idCVar			fs_copyfiles;
	static idCVar			fs_buildResources;
	static idCVar			fs_game;
	static idCVar			fs_game_base;
	static idCVar			fs_enableBGL;
	static idCVar			fs_debugBGL;
	
	idStr					manifestName;
	idStrList				fileManifest;
	idPreloadManifest		preloadList;
	
	idList< idResourceContainer* > resourceFiles;
	byte* 	resourceBufferPtr;
	int		resourceBufferSize;
	int		resourceBufferAvailable;
	int		numFilesOpenedAsCached;
	
private:

	// .resource file creation
	void					ClearResourcePacks();
	void					WriteResourcePacks();
	void					AddRenderProgs( idStrList& files );
	void					AddFonts( idStrList& files );
	
	void					ReplaceSeparators( idStr& path, char sep = PATHSEPARATOR_CHAR );
	int						ListOSFiles( const char* directory, const char* extension, idStrList& list );
	idFileHandle			OpenOSFile( const char* name, fsMode_t mode );
	void					CloseOSFile( idFileHandle o );
	int						DirectFileLength( idFileHandle o );
	void					CopyFile( idFile* src, const char* toOSPath );
	int						AddUnique( const char* name, idStrList& list, idHashIndex& hashIndex ) const;
	void					GetExtensionList( const char* extension, idStrList& extensionList ) const;
	int						GetFileList( const char* relativePath, const idStrList& extensions, idStrList& list, idHashIndex& hashIndex, bool fullRelativePath, const char* gamedir = NULL );
	
	int						GetFileListTree( const char* relativePath, const idStrList& extensions, idStrList& list, idHashIndex& hashIndex, const char* gamedir = NULL );
	void					AddGameDirectory( const char* path, const char* dir );
	
	int						AddResourceFile( const char* resourceFileName );
	void					RemoveMapResourceFile( const char* resourceFileName );
	void					RemoveResourceFileByIndex( const int& idx );
	void					RemoveResourceFile( const char* resourceFileName );
	int						FindResourceFile( const char* resourceFileName );
	
	void					SetupGameDirectories( const char* gameName );
	void					Startup();
	void					InitPrecache();
	void					ReOpenCacheFiles();
};

idCVar	idFileSystemLocal::fs_debug( "fs_debug", "0", CVAR_SYSTEM | CVAR_INTEGER, "", 0, 2, idCmdSystem::ArgCompletion_Integer<0, 2> );
idCVar	idFileSystemLocal::fs_debugResources( "fs_debugResources", "0", CVAR_SYSTEM | CVAR_BOOL, "" );
idCVar	idFileSystemLocal::fs_enableBGL( "fs_enableBGL", "0", CVAR_SYSTEM | CVAR_BOOL, "" );
idCVar	idFileSystemLocal::fs_debugBGL( "fs_debugBGL", "0", CVAR_SYSTEM | CVAR_BOOL, "" );
idCVar	idFileSystemLocal::fs_copyfiles( "fs_copyfiles", "0", CVAR_SYSTEM | CVAR_INIT | CVAR_BOOL, "Copy every file touched to fs_savepath" );
idCVar	idFileSystemLocal::fs_buildResources( "fs_buildresources", "0", CVAR_SYSTEM | CVAR_BOOL | CVAR_INIT, "Copy every file touched to a resource file" );
idCVar	idFileSystemLocal::fs_game( "fs_game", "", CVAR_SYSTEM | CVAR_INIT | CVAR_SERVERINFO, "mod path" );
idCVar  idFileSystemLocal::fs_game_base( "fs_game_base", "", CVAR_SYSTEM | CVAR_INIT | CVAR_SERVERINFO, "alternate mod path, searched after the main fs_game path, before the basedir" );

idCVar	fs_basepath( "fs_basepath", "", CVAR_SYSTEM | CVAR_INIT, "" );
idCVar	fs_savepath( "fs_savepath", "", CVAR_SYSTEM | CVAR_INIT, "" );
idCVar	fs_resourceLoadPriority( "fs_resourceLoadPriority", "1", CVAR_SYSTEM , "if 1, open requests will be honored from resource files first; if 0, the resource files are checked after normal search paths" );
idCVar	fs_enableBackgroundCaching( "fs_enableBackgroundCaching", "1", CVAR_SYSTEM , "if 1 allow the 360 to precache game files in the background" );

idFileSystemLocal	fileSystemLocal;
idFileSystem* 		fileSystem = &fileSystemLocal;

/*
================
idFileSystemLocal::ReadFromBGL
================
*/
int idFileSystemLocal::ReadFromBGL( idFile* _resourceFile, void* _buffer, int _offset, int _len )
{
	if( _resourceFile->Tell() != _offset )
	{
		_resourceFile->Seek( _offset, FS_SEEK_SET );
	}
	return _resourceFile->Read( _buffer, _len );
}

/*
================
idFileSystemLocal::StartPreload
================
*/
void idFileSystemLocal::StartPreload( const idStrList& _preload )
{
}

/*
================
idFileSystemLocal::StopPreload
================
*/
void idFileSystemLocal::StopPreload()
{
}

/*
================
idFileSystemLocal::idFileSystemLocal
================
*/
idFileSystemLocal::idFileSystemLocal()
{
	loadCount = 0;
	loadStack = 0;
	resourceBufferPtr = NULL;
	resourceBufferSize = 0;
	resourceBufferAvailable = 0;
	numFilesOpenedAsCached = 0;
}

/*
===========
idFileSystemLocal::FilenameCompare

Ignore case and separator char distinctions
===========
*/
bool idFileSystemLocal::FilenameCompare( const char* s1, const char* s2 ) const
{
	int		c1, c2;
	
	do
	{
		c1 = *s1++;
		c2 = *s2++;
		
		if( c1 >= 'a' && c1 <= 'z' )
		{
			c1 -= ( 'a' - 'A' );
		}
		if( c2 >= 'a' && c2 <= 'z' )
		{
			c2 -= ( 'a' - 'A' );
		}
		
		if( c1 == '\\' || c1 == ':' )
		{
			c1 = '/';
		}
		if( c2 == '\\' || c2 == ':' )
		{
			c2 = '/';
		}
		
		if( c1 != c2 )
		{
			return true;		// strings not equal
		}
	}
	while( c1 );
	
	return false;		// strings are equal
}

/*
========================
idFileSystemLocal::GetFileLength
========================
*/
int idFileSystemLocal::GetFileLength( const char* relativePath )
{
	idFile* 	f;
	int			len;
	
	if( !IsInitialized() )
	{
		idLib::FatalError( "Filesystem call made without initialization" );
	}
	
	if( !relativePath || !relativePath[0] )
	{
		idLib::Warning( "idFileSystemLocal::GetFileLength with empty name" );
		return -1;
	}
	
	if( resourceFiles.Num() > 0 )
	{
		idResourceCacheEntry rc;
		if( GetResourceCacheEntry( relativePath, rc ) )
		{
			return rc.length;
		}
	}
	
	// look for it in the filesystem or pack files
	f = OpenFileRead( relativePath, false );
	if( f == NULL )
	{
		return -1;
	}
	
	len = ( int )f->Length();
	
	delete f;
	return len;
}

/*
================
idFileSystemLocal::OpenOSFile
================
*/
idFileHandle idFileSystemLocal::OpenOSFile( const char* fileName, fsMode_t mode )
{
	idFileHandle fp;
	
	// RB begin
#if defined(_WIN32)
	DWORD dwAccess = 0;
	DWORD dwShare = 0;
	DWORD dwCreate = 0;
	DWORD dwFlags = 0;
	
	if( mode == FS_WRITE )
	{
		dwAccess = GENERIC_READ | GENERIC_WRITE;
		dwShare = FILE_SHARE_READ;
		dwCreate = CREATE_ALWAYS;
		dwFlags = FILE_ATTRIBUTE_NORMAL;
	}
	else if( mode == FS_READ )
	{
		dwAccess = GENERIC_READ;
		dwShare = FILE_SHARE_READ;
		dwCreate = OPEN_EXISTING;
		dwFlags = FILE_ATTRIBUTE_NORMAL;
	}
	else if( mode == FS_APPEND )
	{
		dwAccess = GENERIC_READ | GENERIC_WRITE;
		dwShare = FILE_SHARE_READ;
		dwCreate = OPEN_ALWAYS;
		dwFlags = FILE_ATTRIBUTE_NORMAL;
	}
	
	fp = CreateFile( fileName, dwAccess, dwShare, NULL, dwCreate, dwFlags, NULL );
	if( fp == INVALID_HANDLE_VALUE )
	{
		return NULL;
	}
#else
	
#ifndef __MWERKS__
#ifndef WIN32
	// some systems will let you fopen a directory
	struct stat buf;
	if( stat( fileName, &buf ) != -1 && !S_ISREG( buf.st_mode ) )
	{
		return NULL;
	}
#endif
#endif
	
	if( mode == FS_WRITE )
	{
		fp = fopen( fileName, "wb" );
	}
	else if( mode == FS_READ )
	{
		fp = fopen( fileName, "rb" );
	}
	else if( mode == FS_APPEND )
	{
		fp = fopen( fileName, "ab" );
	}
	
	if( !fp )//&& fs_caseSensitiveOS.GetBool() )
	{
		// RB: really any proper OS other than Windows should have a case sensitive filesystem
		idStr fpath, entry;
		idStrList list;
	
		fpath = fileName;
		fpath.StripFilename();
		fpath.StripTrailing( PATHSEPARATOR_CHAR );
		if( ListOSFiles( fpath, NULL, list ) == -1 )
		{
			return NULL;
		}
	
		for( int i = 0; i < list.Num(); i++ )
		{
			entry = fpath + PATHSEPARATOR_CHAR + list[i];
			if( !entry.Icmp( fileName ) )
			{
				if( mode == FS_WRITE )
				{
					fp = fopen( entry, "wb" );
				}
				else if( mode == FS_READ )
				{
					fp = fopen( entry, "rb" );
				}
				else if( mode == FS_APPEND )
				{
					fp = fopen( entry, "ab" );
				}
	
				if( fp )
				{
					if( fs_debug.GetInteger() )
					{
						common->Printf( "idFileSystemLocal::OpenFileRead: changed %s to %s\n", fileName, entry.c_str() );
					}
					break;
				}
				else
				{
					// not supposed to happen if ListOSFiles is doing it's job correctly
					common->Warning( "idFileSystemLocal::OpenFileRead: fs_caseSensitiveOS 1 could not open %s", entry.c_str() );
				}
			}
		}
	}
	
#endif
	// RB end
	
	return fp;
}

/*
================
idFileSystemLocal::CloseOSFile
================
*/
void idFileSystemLocal::CloseOSFile( idFileHandle o )
{
	// RB begin
#if defined(_WIN32)
	::CloseHandle( o );
#else
	fclose( o );
#endif
	// RB end
}

/*
================
idFileSystemLocal::DirectFileLength
================
*/
int idFileSystemLocal::DirectFileLength( idFileHandle o )
{
	// RB begin
#if defined(_WIN32)
	return GetFileSize( o, NULL );
#else
	int		pos;
	int		end;
	
	pos = ftell( o );
	fseek( o, 0, SEEK_END );
	end = ftell( o );
	fseek( o, pos, SEEK_SET );
	
	return end;
#endif
	// RB end
}

/*
============
idFileSystemLocal::CreateOSPath

Creates any directories needed to store the given filename
============
*/
void idFileSystemLocal::CreateOSPath( const char* OSPath )
{
	char*	ofs;
	
	// make absolutely sure that it can't back up the path
	// FIXME: what about c: ?
	if( strstr( OSPath, ".." ) || strstr( OSPath, "::" ) )
	{
#ifdef _DEBUG
		common->DPrintf( "refusing to create relative path \"%s\"\n", OSPath );
#endif
		return;
	}
	
	idStrStatic< MAX_OSPATH > path( OSPath );
	
	// RB begin
#if defined(_WIN32)
	path.SlashesToBackSlashes();
#endif
	// RB end
	
	for( ofs = &path[ 1 ]; *ofs ; ofs++ )
	{
		if( *ofs == PATHSEPARATOR_CHAR )
		{
			// create the directory
			*ofs = 0;
			Sys_Mkdir( path );
			*ofs = PATHSEPARATOR_CHAR;
		}
	}
}

/*
=================
idFileSystemLocal::EnableBackgroundCache
=================
*/
void idFileSystemLocal::EnableBackgroundCache( bool enable )
{
	if( !fs_enableBackgroundCaching.GetBool() )
	{
		return;
	}
}

/*
=================
idFileSystemLocal::BeginLevelLoad
=================
*/
void idFileSystemLocal::BeginLevelLoad( const char* name, char* _blockBuffer, int _blockBufferSize )
{

	if( name == NULL || *name == '\0' )
	{
		return;
	}
	
	resourceBufferPtr = ( byte* )_blockBuffer;
	resourceBufferAvailable = _blockBufferSize;
	resourceBufferSize = _blockBufferSize;
	
	manifestName = name;
	
	fileManifest.Clear();
	preloadList.Clear();
	
	EnableBackgroundCache( false );
	
	ReOpenCacheFiles();
	manifestName.StripPath();
	
	if( resourceFiles.Num() > 0 )
	{
		AddResourceFile( va( "%s.resources", manifestName.c_str() ) );
	}
	
}

/*
=================
idFileSystemLocal::UnloadResourceContainer

=================
*/
void idFileSystemLocal::UnloadResourceContainer( const char* name )
{
	if( name == NULL || *name == '\0' )
	{
		return;
	}
	RemoveResourceFile( va( "%s.resources", name ) );
}

/*
=================
idFileSystemLocal::UnloadMapResources
=================
*/
void idFileSystemLocal::UnloadMapResources( const char* name )
{
	if( name == NULL || *name == '\0' || idStr::Icmp( "_startup", name ) == 0 )
	{
		return;
	}
	
	// RB: don't
	/*
	if( resourceFiles.Num() > 0 )
	{
		RemoveMapResourceFile( va( "%s.resources", name ) );
	}
	*/
	// RB end
}

/*
=================
idFileSystemLocal::EndLevelLoad

=================
*/
void idFileSystemLocal::EndLevelLoad()
{
	if( fs_buildResources.GetBool() )
	{
		int saveCopyFiles = fs_copyfiles.GetInteger();
		fs_copyfiles.SetInteger( 0 );
		
		idStr manifestFileName = manifestName;
		manifestFileName.StripPath();
		manifestFileName.SetFileExtension( "manifest" );
		manifestFileName.Insert( "maps/", 0 );
		idFile* outFile = fileSystem->OpenFileWrite( manifestFileName );
		if( outFile != NULL )
		{
			int num = fileManifest.Num();
			outFile->WriteBig( num );
			for( int i = 0; i < num; i++ )
			{
				outFile->WriteString( fileManifest[ i ] );
			}
			delete outFile;
		}
		
		idStrStatic< MAX_OSPATH > preloadName = manifestName;
		preloadName.Insert( "maps/", 0 );
		preloadName += ".preload";
		idFile* fileOut = fileSystem->OpenFileWrite( preloadName, "fs_savepath" );
		preloadList.WriteManifestToFile( fileOut );
		delete fileOut;
		
		fs_copyfiles.SetInteger( saveCopyFiles );
	}
	
	EnableBackgroundCache( true );
	
	resourceBufferPtr = NULL;
	resourceBufferAvailable = 0;
	resourceBufferSize = 0;
	
}

// RB begin
bool idFileSystemLocal::InProductionMode()
{
	//return fs_resourceLoadPriority.GetBool() && ( resourceFiles.Num() > 0 ) || ( com_productionMode.GetInteger() != 0 );
	
	return ( com_productionMode.GetInteger() != 0 );
}
// RB end

bool FileExistsInAllManifests( const char* filename, idList< idFileManifest >& manifests )
{
	for( int i = 0; i < manifests.Num(); i++ )
	{
		if( strstr( manifests[ i ].GetManifestName(), "_startup" ) != NULL )
		{
			continue;
		}
		if( strstr( manifests[ i ].GetManifestName(), "_pc" ) != NULL )
		{
			continue;
		}
		if( manifests[ i ].FindFile( filename ) == -1 )
		{
			return false;
		}
	}
	return true;
}

bool FileExistsInAllPreloadManifests( const char* filename, idList< idPreloadManifest >& manifests )
{
	for( int i = 0; i < manifests.Num(); i++ )
	{
		if( strstr( manifests[ i ].GetManifestName(), "_startup" ) != NULL )
		{
			continue;
		}
		if( manifests[ i ].FindResource( filename ) == -1 )
		{
			return false;
		}
	}
	return true;
}

void RemoveFileFromAllManifests( const char* filename, idList< idFileManifest >& manifests )
{
	for( int i = 0; i < manifests.Num(); i++ )
	{
		if( strstr( manifests[ i ].GetManifestName(), "_startup" ) != NULL )
		{
			continue;
		}
		if( strstr( manifests[ i ].GetManifestName(), "_pc" ) != NULL )
		{
			continue;
		}
		manifests[ i ].RemoveAll( filename );
	}
}


/*
================
idFileSystemLocal::AddPerPlatformResources
================
*/
void idFileSystemLocal::AddRenderProgs( idStrList& files )
{
	idStrList work;
	
	// grab all the renderprogs
	idStr path = RelativePathToOSPath( "renderprogs/cgb", "fs_savepath" );
	ListOSFiles( path, "*.cgb", work );
	for( int i = 0; i < work.Num(); i++ )
	{
		files.Append( idStr( "renderprogs/cgb/" ) + work[i] );
	}
	
	path = RelativePathToOSPath( "renderprogs/hlsl", "fs_savepath" );
	ListOSFiles( path, "*.v360", work );
	for( int i = 0; i < work.Num(); i++ )
	{
		files.Append( idStr( "renderprogs/hlsl/" ) + work[i] );
	}
	ListOSFiles( path, "*.p360", work );
	for( int i = 0; i < work.Num(); i++ )
	{
		files.Append( idStr( "renderprogs/hlsl/" ) + work[i] );
	}
	
	path = RelativePathToOSPath( "renderprogs/gl", "fs_savepath" );
	ListOSFiles( path, "*.*", work );
	for( int i = 0; i < work.Num(); i++ )
	{
		files.Append( idStr( "renderprogs/gl/" ) + work[i] );
	}
	
}

/*
================
idFileSystemLocal::AddSoundResources
================
*/
void idFileSystemLocal::AddFonts( idStrList& files )
{
	// temp fix for getting idaudio files in
	idFileList* fl = ListFilesTree( "generated/images/newfonts", "*.bimage", false );
	for( int i = 0; i < fl->GetList().Num(); i++ )
	{
		files.AddUnique( fl->GetList()[i] );
	}
	FreeFileList( fl );
	
	fl = ListFilesTree( "newfonts", "*.dat", false );
	for( int i = 0; i < fl->GetList().Num(); i++ )
	{
		files.AddUnique( fl->GetList()[i] );
	}
	FreeFileList( fl );
}


const char* excludeExtensions[] =
{
	".idxma", ".idmsf", ".idwav", ".xma", ".msf", ".wav", ".resource"
};
const int numExcludeExtensions = sizeof( excludeExtensions ) / sizeof( excludeExtensions[ 0 ] );

bool IsExcludedFile( const idStr& resName )
{
	for( int k = 0; k < numExcludeExtensions; k++ )
	{
		if( resName.Find( excludeExtensions[ k ], false ) >= 0 )
		{
			return true;
		}
	}
	return false;
}

/*
================
idFileSystemLocal::IsBinaryModel
================
*/
bool idFileSystemLocal::IsBinaryModel( const idStr& resName ) const
{
	idStrStatic< 32 > ext;
	resName.ExtractFileExtension( ext );
	if( ( ext.Icmp( "base" ) == 0 ) || ( ext.Icmp( "blwo" ) == 0 ) || ( ext.Icmp( "bflt" ) == 0 ) || ( ext.Icmp( "bma" ) == 0 ) )
	{
		return true;
	}
	return false;
}

/*
================
idFileSystemLocal::IsSoundSample
================
*/
bool idFileSystemLocal::IsSoundSample( const idStr& resName ) const
{
	idStrStatic< 32 > ext;
	resName.ExtractFileExtension( ext );
	if( ( ext.Icmp( "idxma" ) == 0 ) || ( ext.Icmp( "idwav" ) == 0 ) || ( ext.Icmp( "idmsf" ) == 0 ) || ( ext.Icmp( "xma" ) == 0 ) || ( ext.Icmp( "wav" ) == 0 ) || ( ext.Icmp( "msf" ) == 0 ) || ( ext.Icmp( "msadpcm" ) == 0 ) )
	{
		return true;
	}
	return false;
}


void idFileSystemLocal::BuildOrderedStartupContainer()
{
	idStrList orderedFiles( 1024 );
	
	idFileList* fl = ListFilesTree( "materials", "*.mtr", true );
	for( int i = 0; i < fl->GetList().Num(); i++ )
	{
		orderedFiles.AddUnique( fl->GetList()[i] );
	}
	FreeFileList( fl );
	
	fl = ListFilesTree( "renderprogs", "*.v360", true );
	for( int i = 0; i < fl->GetList().Num(); i++ )
	{
		orderedFiles.AddUnique( fl->GetList()[i] );
	}
	FreeFileList( fl );
	
	fl = ListFilesTree( "renderprogs", "*.p360", true );
	for( int i = 0; i < fl->GetList().Num(); i++ )
	{
		orderedFiles.AddUnique( fl->GetList()[i] );
	}
	FreeFileList( fl );
	
	fl = ListFilesTree( "renderprogs", "*.cgb", true );
	for( int i = 0; i < fl->GetList().Num(); i++ )
	{
		orderedFiles.AddUnique( fl->GetList()[i] );
	}
	FreeFileList( fl );
	
	fl = ListFilesTree( "renderprogs/gl", "*.*", true );
	for( int i = 0; i < fl->GetList().Num(); i++ )
	{
		orderedFiles.AddUnique( fl->GetList()[i] );
	}
	FreeFileList( fl );
	
	fl = ListFilesTree( "skins", "*.skin", true );
	for( int i = 0; i < fl->GetList().Num(); i++ )
	{
		orderedFiles.AddUnique( fl->GetList()[i] );
	}
	FreeFileList( fl );
	
	fl = ListFilesTree( "sound", "*.sndshd", false );
	for( int i = 0; i < fl->GetList().Num(); i++ )
	{
		orderedFiles.AddUnique( fl->GetList()[i] );
	}
	FreeFileList( fl );
	
	fl = ListFilesTree( "def", "*.def", false );
	for( int i = 0; i < fl->GetList().Num(); i++ )
	{
		orderedFiles.AddUnique( fl->GetList()[i] );
	}
	FreeFileList( fl );
	
	fl = ListFilesTree( "fx", "*.fx", false );
	for( int i = 0; i < fl->GetList().Num(); i++ )
	{
		orderedFiles.AddUnique( fl->GetList()[i] );
	}
	FreeFileList( fl );
	
	fl = ListFilesTree( "particles", "*.prt", false );
	for( int i = 0; i < fl->GetList().Num(); i++ )
	{
		orderedFiles.AddUnique( fl->GetList()[i] );
	}
	FreeFileList( fl );
	
	fl = ListFilesTree( "af", "*.af", false );
	for( int i = 0; i < fl->GetList().Num(); i++ )
	{
		orderedFiles.AddUnique( fl->GetList()[i] );
	}
	FreeFileList( fl );
	fl = ListFilesTree( "newpdas", "*.pda", false );
	for( int i = 0; i < fl->GetList().Num(); i++ )
	{
		orderedFiles.AddUnique( fl->GetList()[i] );
	}
	FreeFileList( fl );
	
	orderedFiles.Append( "script/doom_main.script" );
	orderedFiles.Append( "script/doom_defs.script" );
	orderedFiles.Append( "script/doom_defs.script" );
	orderedFiles.Append( "script/doom_events.script" );
	orderedFiles.Append( "script/doom_util.script" );
	orderedFiles.Append( "script/weapon_base.script" );
	orderedFiles.Append( "script/ai_base.script" );
	orderedFiles.Append( "script/weapon_fists.script" );
	orderedFiles.Append( "script/weapon_pistol.script" );
	orderedFiles.Append( "script/weapon_shotgun.script" );
	orderedFiles.Append( "script/weapon_machinegun.script" );
	orderedFiles.Append( "script/weapon_chaingun.script" );
	orderedFiles.Append( "script/weapon_handgrenade.script" );
	orderedFiles.Append( "script/weapon_plasmagun.script" );
	orderedFiles.Append( "script/weapon_rocketlauncher.script" );
	orderedFiles.Append( "script/weapon_bfg.script" );
	orderedFiles.Append( "script/weapon_soulcube.script" );
	orderedFiles.Append( "script/weapon_chainsaw.script" );
	orderedFiles.Append( "script/weapon_flashlight.script" );
	orderedFiles.Append( "script/weapon_pda.script" );
	orderedFiles.Append( "script/ai_monster_base.script" );
	orderedFiles.Append( "script/ai_monster_zombie_base.script" );
	orderedFiles.Append( "script/ai_monster_demon_archvile.script" );
	orderedFiles.Append( "script/ai_monster_demon_cherub.script" );
	orderedFiles.Append( "script/ai_monster_demon_hellknight.script" );
	orderedFiles.Append( "script/ai_monster_demon_imp.script" );
	orderedFiles.Append( "script/ai_monster_demon_maggot.script" );
	orderedFiles.Append( "script/ai_monster_demon_mancubus.script" );
	orderedFiles.Append( "script/ai_monster_demon_pinky.script" );
	orderedFiles.Append( "script/ai_monster_demon_revenant.script" );
	orderedFiles.Append( "script/ai_monster_demon_trite.script" );
	orderedFiles.Append( "script/ai_monster_demon_wraith.script" );
	orderedFiles.Append( "script/ai_monster_flying_lostsoul.script" );
	orderedFiles.Append( "script/ai_monster_flying_cacodemon.script" );
	orderedFiles.Append( "script/ai_monster_zombie.script" );
	orderedFiles.Append( "script/ai_monster_zombie_morgue.script" );
	orderedFiles.Append( "script/ai_monster_zombie_sawyer.script" );
	orderedFiles.Append( "script/ai_monster_zombie_bernie.script" );
	orderedFiles.Append( "script/ai_monster_zombie_commando_cgun.script" );
	orderedFiles.Append( "script/ai_monster_zombie_commando_tentacle.script" );
	orderedFiles.Append( "script/ai_monster_zombie_security_pistol.script" );
	orderedFiles.Append( "script/ai_monster_turret.script" );
	orderedFiles.Append( "script/ai_monster_boss_vagary.script" );
	orderedFiles.Append( "script/ai_monster_boss_cyberdemon.script" );
	orderedFiles.Append( "script/ai_monster_boss_guardian.script" );
	orderedFiles.Append( "script/ai_monster_boss_guardian_seeker.script" );
	orderedFiles.Append( "script/ai_monster_boss_sabaoth.script" );
	orderedFiles.Append( "script/ai_character.script" );
	orderedFiles.Append( "script/ai_character_prone.script" );
	orderedFiles.Append( "script/ai_character_sentry.script" );
	orderedFiles.Append( "script/ai_player.script" );
	orderedFiles.Append( "script/ai_alphalabs2_scientist1.script" );
	orderedFiles.Append( "script/ai_cinematic_le.script" );
	orderedFiles.Append( "script/map_admin1.script" );
	orderedFiles.Append( "script/map_alphalabs1.script" );
	orderedFiles.Append( "script/map_alphalabs2.script" );
	orderedFiles.Append( "script/map_alphalabs3.script" );
	orderedFiles.Append( "script/map_alphalabs3_crane.script" );
	orderedFiles.Append( "script/map_alphalabs4.script" );
	orderedFiles.Append( "script/map_caves.script" );
	orderedFiles.Append( "script/map_caves2.script" );
	orderedFiles.Append( "script/map_comm1.script" );
	orderedFiles.Append( "script/map_commoutside_lift.script" );
	orderedFiles.Append( "script/map_commoutside.script" );
	orderedFiles.Append( "script/map_cpu.script" );
	orderedFiles.Append( "script/map_cpuboss.script" );
	orderedFiles.Append( "script/map_delta1.script" );
	orderedFiles.Append( "script/map_delta2a.script" );
	orderedFiles.Append( "script/map_delta2b.script" );
	orderedFiles.Append( "script/map_delta3.script" );
	orderedFiles.Append( "script/map_delta5.script" );
	orderedFiles.Append( "script/map_enpro.script" );
	orderedFiles.Append( "script/map_hell1.script" );
	orderedFiles.Append( "script/map_hellhole.script" );
	orderedFiles.Append( "script/map_recycling1.script" );
	orderedFiles.Append( "script/map_recycling2.script" );
	orderedFiles.Append( "script/map_site3.script" );
	orderedFiles.Append( "script/map_marscity1.script" );
	orderedFiles.Append( "script/map_marscity2.script" );
	orderedFiles.Append( "script/map_mc_underground.script" );
	orderedFiles.Append( "script/map_monorail.script" );
	orderedFiles.Append( "script/d3xp_events.script" );
	orderedFiles.Append( "script/weapon_bloodstone_passive.script" );
	orderedFiles.Append( "script/weapon_bloodstone_active1.script" );
	orderedFiles.Append( "script/weapon_bloodstone_active2.script" );
	orderedFiles.Append( "script/weapon_bloodstone_active3.script" );
	orderedFiles.Append( "script/weapon_shotgun_double.script" );
	orderedFiles.Append( "script/weapon_grabber.script" );
	orderedFiles.Append( "script/ai_monster_hunter_helltime.script" );
	orderedFiles.Append( "script/ai_monster_hunter_berserk.script" );
	orderedFiles.Append( "script/ai_monster_hunter_invul.script" );
	orderedFiles.Append( "script/ai_monster_boss_maledict.script" );
	orderedFiles.Append( "script/ai_monster_demon_vulgar.script" );
	orderedFiles.Append( "script/ai_monster_demon_d3xp_bruiser.script" );
	orderedFiles.Append( "script/ai_monster_dummy_target.script" );
	orderedFiles.Append( "script/ai_monster_dummy.script" );
	orderedFiles.Append( "script/ai_monster_demon_sentry.script" );
	orderedFiles.Append( "script/ai_monster_demon_trite_jump.script" );
	orderedFiles.Append( "script/ai_monster_turret_ancient.script" );
	orderedFiles.Append( "script/ai_monster_flying_forgotten.script" );
	orderedFiles.Append( "script/ai_character_erebus3.script" );
	orderedFiles.Append( "script/d3xp_airlock.script" );
	orderedFiles.Append( "script/d3xp_bloodstone.script" );
	orderedFiles.Append( "script/map_erebus1.script" );
	orderedFiles.Append( "script/map_erebus2_helltime.script" );
	orderedFiles.Append( "script/map_erebus2.script" );
	orderedFiles.Append( "script/map_erebus3.script" );
	orderedFiles.Append( "script/map_erebus4.script" );
	orderedFiles.Append( "script/map_erebus5.script" );
	orderedFiles.Append( "script/map_erebus5_cloud.script" );
	orderedFiles.Append( "script/map_erebus6.script" );
	orderedFiles.Append( "script/map_erebus6_berzerk.script" );
	orderedFiles.Append( "script/map_phobos1.script" );
	orderedFiles.Append( "script/map_phobos2.script" );
	orderedFiles.Append( "script/map_phobos2_invul.script" );
	orderedFiles.Append( "script/map_phobos3.script" );
	orderedFiles.Append( "script/map_phobos4.script" );
	orderedFiles.Append( "script/map_deltax.script" );
	orderedFiles.Append( "script/map_hell.script" );
	orderedFiles.Append( "script/map_maledict.script" );
	orderedFiles.Append( "script/d3le-ai_monster_boss_guardian2.script" );
	orderedFiles.Append( "script/ai_follower.script" );
	orderedFiles.Append( "generated/swf/shell.bswf" );
	fl = ListFilesTree( "newfonts", "*.dat", false );
	for( int i = 0; i < fl->GetList().Num(); i++ )
	{
		orderedFiles.AddUnique( fl->GetList()[i] );
	}
	FreeFileList( fl );
	
	idResourceContainer::WriteResourceFile( "_ordered.resources", orderedFiles, false );
}

/*
================
idFileSystemLocal::WriteResourcePacks
================
*/
void idFileSystemLocal::WriteResourcePacks()
{

	idStrList filesNotCommonToAllMaps( 16384 );		// files that are not shared by all maps, used to trim the common list
	idStrList filesCommonToAllMaps( 16384 );		// files that are shared by all maps, will include startup files, renderprogs etc..
	idPreloadManifest commonPreloads;				// preload entries that exist in all map preload files
	
	idStr path = RelativePathToOSPath( "maps/", "fs_savepath" );
	
	idStrList manifestFiles;
	ListOSFiles( path, ".manifest", manifestFiles );
	idStrList preloadFiles;
	ListOSFiles( path, ".preload", preloadFiles );
	
	idList< idFileManifest > manifests;				// list of all manifest files
	// load all file manifests
	for( int i = 0; i < manifestFiles.Num(); i++ )
	{
		idStr path = "maps/";
		path += manifestFiles[ i ];
		idFileManifest manifest;
		if( manifest.LoadManifest( path ) )
		{
			//manifest.Print();
			manifest.RemoveAll( va( "strings/%s", ID_LANG_ENGLISH ) );	// remove all .lang files
			manifest.RemoveAll( va( "strings/%s", ID_LANG_FRENCH ) );
			manifest.RemoveAll( va( "strings/%s", ID_LANG_ITALIAN ) );
			manifest.RemoveAll( va( "strings/%s", ID_LANG_GERMAN ) );
			manifest.RemoveAll( va( "strings/%s", ID_LANG_SPANISH ) );
			manifest.RemoveAll( va( "strings/%s", ID_LANG_JAPANESE ) );
			manifests.Append( manifest );
		}
	}
	
	idList< idPreloadManifest > preloadManifests;	// list of all preload manifest files
	// load all preload manifests
	for( int i = 0; i < preloadFiles.Num(); i++ )
	{
		idStr path = "maps/";
		path += preloadFiles[ i ];
		if( path.Find( "_startup", false ) >= 0 )
		{
			continue;
		}
		idPreloadManifest preload;
		if( preload.LoadManifest( path ) )
		{
			preloadManifests.Append( preload );
			//preload.Print();
		}
	}
	
	// build common list of files
	for( int i = 0; i < manifests.Num(); i++ )
	{
		idFileManifest& manifest = manifests[ i ];
		for( int j = 0; j < manifest.NumFiles(); j++ )
		{
			idStr name = manifest.GetFileNameByIndex( j );
			if( name.CheckExtension( ".cfg" ) || ( name.Find( ".lang", false ) >= 0 ) )
			{
				continue;
			}
			if( FileExistsInAllManifests( name, manifests ) )
			{
				filesCommonToAllMaps.AddUnique( name );
			}
			else
			{
				filesNotCommonToAllMaps.AddUnique( name );
			}
		}
	}
	// common list of preload reosurces, image, sample or models
	for( int i = 0; i < preloadManifests.Num(); i++ )
	{
		idPreloadManifest& preload = preloadManifests[ i ];
		for( int j = 0; j < preload.NumResources(); j++ )
		{
			idStr name = preload.GetResourceNameByIndex( j );
			if( FileExistsInAllPreloadManifests( name, preloadManifests ) )
			{
				commonPreloads.Add( preload.GetPreloadByIndex( j ) );
				idLib::Printf( "Common preload added %s\n", name.c_str() );
			}
			else
			{
				idLib::Printf( "preload missed %s\n", name.c_str() );
			}
		}
	}
	
	AddRenderProgs( filesCommonToAllMaps );
	AddFonts( filesCommonToAllMaps );
	
	idStrList work;
	
	// remove all common files from each map manifest
	for( int i = 0; i < manifests.Num(); i++ )
	{
		if( ( strstr( manifests[ i ].GetManifestName(), "_startup" ) != NULL ) || ( strstr( manifests[ i ].GetManifestName(), "_pc" ) != NULL ) )
		{
			continue;
		}
		//idLib::Printf( "%04d referenced files for %s\n", manifests[ i ].GetReferencedFileCount(), manifests[ i ].GetManifestName() );
		
		for( int j = 0; j < filesCommonToAllMaps.Num(); j++ )
		{
			manifests[ i ].RemoveAll( filesCommonToAllMaps[ j ] );
		}
		//idLib::Printf( "%04d referenced files for %s\n", manifests[ i ].GetReferencedFileCount(), manifests[ i ].GetManifestName() );
	}
	
	idStrList commonImages( 2048 );
	idStrList commonModels( 2048 );
	idStrList commonAnims( 2048 );
	idStrList commonCollision( 2048 );
	idStrList soundFiles( 2048 );		// don't write these per map so we fit on disc
	
	for( int i = 0; i < manifests.Num(); i++ )
	{
		idStr resourceFileName = manifests[ i ].GetManifestName();
		if( resourceFileName.Find( "_startup.manifest", false ) >= 0 )
		{
			// add all the startup manifest files to the common list
			for( int j = 0; j < manifests[ i ].NumFiles(); j++ )
			{
				idStr check = manifests[i].GetFileNameByIndex( j );
				if( check.CheckExtension( ".cfg" ) == false )
				{
					filesCommonToAllMaps.AddUnique( check.c_str() );
				}
			}
			continue;
		}
		
		idStaticList< idStr, 16384 > mapFiles;		// map files from the manifest, these are static for easy debugging
		idStaticList< idStr, 16384 > mapFilesTwo;	// accumulates non bimage, bmodel and sample files
		commonImages.Clear();	// collect images and models separately so they can be added in linear preload order
		commonModels.Clear();
		commonAnims.Clear();
		commonCollision.Clear();
		
		manifests[ i ].PopulateList( mapFiles );
		
		for( int j = 0; j < mapFiles.Num(); j++ )
		{
			idStr& resName = mapFiles[ j ];
			if( resName.Find( ".bimage", false ) >= 0 )
			{
				commonImages.AddUnique( resName );
				continue;
			}
			if( IsBinaryModel( resName ) )
			{
				commonModels.AddUnique( resName );
				continue;
			}
			if( IsSoundSample( resName ) )
			{
				soundFiles.AddUnique( resName );
				continue;
			}
			if( resName.Find( ".bik", false ) >= 0 )
			{
				// don't add bik files
				continue;
			}
			if( resName.Find( ".bmd5anim", false ) >= 0 )
			{
				commonAnims.AddUnique( resName );
				continue;
			}
			if( resName.Find( ".bcmodel", false ) >= 0 )
			{
				commonCollision.AddUnique( resName );
				continue;
			}
			if( resName.Find( ".lang", false ) >= 0 )
			{
				continue;
			}
			mapFilesTwo.AddUnique( resName );
		}
		
		for( int j = 0; j < commonImages.Num(); j++ )
		{
			mapFilesTwo.AddUnique( commonImages[ j ] );
		}
		for( int j = 0; j < commonModels.Num(); j++ )
		{
			mapFilesTwo.AddUnique( commonModels[ j ] );
		}
		for( int j = 0; j < commonAnims.Num(); j++ )
		{
			mapFilesTwo.AddUnique( commonAnims[ j ] );
		}
		for( int j = 0; j < commonCollision.Num(); j++ )
		{
			mapFilesTwo.AddUnique( commonCollision[ j ] );
		}
		// write map resources
		idStrList mapFilesToWrite;
		for( int j = 0; j < mapFilesTwo.Num(); j++ )
		{
			mapFilesToWrite.Append( mapFilesTwo[ j ] );
		}
		idResourceContainer::WriteResourceFile( resourceFileName, mapFilesToWrite, false );
	}
	
	// add  the new manifests just written
	path = RelativePathToOSPath( "maps", "fs_savepath" );
	ListOSFiles( path, "*.preload", work );
	for( int i = 0; i < work.Num(); i++ )
	{
		filesCommonToAllMaps.Append( idStr( "maps/" ) + work[ i ] );
	}
	
	filesCommonToAllMaps.Append( "_common.preload" );
	
	// write out common models, images and sounds to separate containers
	//idStrList commonSounds( 2048 );
	commonImages.Clear();
	commonModels.Clear();
	
	idStrList commonFiles;
	for( int i = 0; i < filesCommonToAllMaps.Num(); i++ )
	{
		idStr& resName = filesCommonToAllMaps[ i ];
		if( resName.Find( ".bimage", false ) >= 0 )
		{
			commonImages.AddUnique( resName );
			continue;
		}
		if( IsBinaryModel( resName ) )
		{
			commonModels.AddUnique( resName );
			continue;
		}
		if( IsSoundSample( resName ) )
		{
			soundFiles.AddUnique( resName );
			continue;
		}
		if( resName.Find( ".bik", false ) >= 0 )
		{
			// no bik files in the .resource
			continue;
		}
		if( resName.Find( ".lang", false ) >= 0 )
		{
			// no bik files in the .resource
			continue;
		}
		commonFiles.AddUnique( resName );
	}
	
	for( int j = 0; j < commonImages.Num(); j++ )
	{
		commonFiles.AddUnique( commonImages[ j ] );
	}
	for( int j = 0; j < commonModels.Num(); j++ )
	{
		commonFiles.AddUnique( commonModels[ j ] );
	}
	
	//idResourceContainer::WriteResourceFile( "_common_images", commonImages );
	//idResourceContainer::WriteResourceFile( "_common_models", commonModels );
	
	commonPreloads.WriteManifest( "_common.preload" );
	idResourceContainer::WriteResourceFile( "_common", commonFiles, false );
	
	
	idList< idStrList > soundOutputFiles;
	soundOutputFiles.SetNum( 16 );
	
	struct soundVOInfo_t
	{
		const char* filename;
		const char* voqualifier;
		idStrList* samples;
	};
	const soundVOInfo_t soundFileInfo[] =
	{
		{ "fr", "sound/vo/french/", &soundOutputFiles[ 0 ] },
		{ "it", "sound/vo/italian/", &soundOutputFiles[ 1 ] },
		{ "gr", "sound/vo/german/", &soundOutputFiles[ 2 ] },
		{ "sp", "sound/vo/spanish/", &soundOutputFiles[ 3 ] },
		{ "jp", "sound/vo/japanese/", &soundOutputFiles[ 4 ] },
		{ "en", "sound/vo/", &soundOutputFiles[ 5 ] }	// english last so the other langs are culled first
	};
	const int numSoundFiles = sizeof( soundFileInfo ) / sizeof( soundVOInfo_t );
	
	for( int k = soundFiles.Num() - 1; k > 0; k-- )
	{
		for( int l = 0; l < numSoundFiles; l++ )
		{
			if( soundFiles[ k ].Find( soundFileInfo[ l ].voqualifier, false ) >= 0 )
			{
				soundFileInfo[ l ].samples->AddUnique( soundFiles[ k ] );
				soundFiles.RemoveIndex( k );
			}
		}
	}
	
	for( int k = 0; k < numSoundFiles; k++ )
	{
		idStrList& sampleList = *soundFileInfo[ k ].samples;
		
		// write pc
		idResourceContainer::WriteResourceFile( va( "_sound_pc_%s", soundFileInfo[ k ].filename ), sampleList, false );
		for( int l = 0; l < sampleList.Num(); l++ )
		{
			sampleList[ l ].Replace( ".idwav", ".idxma" );
		}
	}
	
	idResourceContainer::WriteResourceFile( "_sound_pc", soundFiles, false );
	for( int k = 0; k < soundFiles.Num(); k++ )
	{
		soundFiles[ k ].Replace( ".idwav", ".idxma" );
	}
	
	for( int k = 0; k < soundFiles.Num(); k++ )
	{
		soundFiles[ k ].Replace( ".idxma", ".idmsf" );
	}
	
	BuildOrderedStartupContainer();
	
	ClearResourcePacks();
}


/*
=================
idFileSystemLocal::CopyFile

Copy a fully specified file from one place to another`
=================
*/
void idFileSystemLocal::CopyFile( const char* fromOSPath, const char* toOSPath )
{

	idFile* src = OpenExplicitFileRead( fromOSPath );
	if( src == NULL )
	{
		idLib::Warning( "Could not open %s for read", fromOSPath );
		return;
	}
	
	if( idStr::Icmp( fromOSPath, toOSPath ) == 0 )
	{
		// same file can happen during build games
		return;
	}
	
	CopyFile( src, toOSPath );
	delete src;
	
	if( strstr( fromOSPath, ".wav" ) != NULL )
	{
		idStrStatic< MAX_OSPATH > newFromPath = fromOSPath;
		idStrStatic< MAX_OSPATH > newToPath = toOSPath;
		
		idLib::Printf( "Copying console samples for %s\n", newFromPath.c_str() );
		newFromPath.SetFileExtension( "xma" );
		newToPath.SetFileExtension( "xma" );
		src = OpenExplicitFileRead( newFromPath );
		if( src == NULL )
		{
			idLib::Warning( "Could not open %s for read", newFromPath.c_str() );
		}
		else
		{
			CopyFile( src, newToPath );
			delete src;
			src = NULL;
		}
		
		newFromPath.SetFileExtension( "msf" );
		newToPath.SetFileExtension( "msf" );
		src = OpenExplicitFileRead( newFromPath );
		if( src == NULL )
		{
			idLib::Warning( "Could not open %s for read", newFromPath.c_str() );
		}
		else
		{
			CopyFile( src, newToPath );
			delete src;
		}
		
		newFromPath.BackSlashesToSlashes();
		newFromPath.ToLower();
		if( newFromPath.Find( "/vo/", false ) >= 0 )
		{
			for( int i = 0; i < Sys_NumLangs(); i++ )
			{
				const char* lang = Sys_Lang( i );
				if( idStr::Icmp( lang, ID_LANG_ENGLISH ) == 0 )
				{
					continue;
				}
				newFromPath = fromOSPath;
				newToPath = toOSPath;
				newFromPath.BackSlashesToSlashes();
				newFromPath.ToLower();
				newToPath.BackSlashesToSlashes();
				newToPath.ToLower();
				newFromPath.Replace( "/vo/", va( "/vo/%s/", lang ) );
				newToPath.Replace( "/vo/", va( "/vo/%s/", lang ) );
				
				src = OpenExplicitFileRead( newFromPath );
				if( src == NULL )
				{
					idLib::Warning( "LOCALIZATION PROBLEM: Could not open %s for read", newFromPath.c_str() );
				}
				else
				{
					CopyFile( src, newToPath );
					delete src;
					src = NULL;
				}
				
				newFromPath.SetFileExtension( "xma" );
				newToPath.SetFileExtension( "xma" );
				src = OpenExplicitFileRead( newFromPath );
				if( src == NULL )
				{
					idLib::Warning( "LOCALIZATION PROBLEM: Could not open %s for read", newFromPath.c_str() );
				}
				else
				{
					CopyFile( src, newToPath );
					delete src;
					src = NULL;
				}
				
				newFromPath.SetFileExtension( "msf" );
				newToPath.SetFileExtension( "msf" );
				src = OpenExplicitFileRead( newFromPath );
				if( src == NULL )
				{
					idLib::Warning( "LOCALIZATION PROBLEM: Could not open %s for read", newFromPath.c_str() );
				}
				else
				{
					CopyFile( src, newToPath );
					delete src;
				}
				
			}
		}
	}
}

/*
=================
idFileSystemLocal::CopyFile
=================
*/
void idFileSystemLocal::CopyFile( idFile* src, const char* toOSPath )
{
	idFile* dst = OpenExplicitFileWrite( toOSPath );
	if( dst == NULL )
	{
		idLib::Warning( "Could not open %s for write", toOSPath );
		return;
	}
	
	common->Printf( "copy %s to %s\n", src->GetName(), toOSPath );
	
	int len = src->Length();
	int copied = 0;
	while( copied < len )
	{
		byte buffer[4096];
		int read = src->Read( buffer, Min( 4096, len - copied ) );
		if( read <= 0 )
		{
			idLib::Warning( "Copy failed during read" );
			break;
		}
		int written = dst->Write( buffer, read );
		if( written < read )
		{
			idLib::Warning( "Copy failed during write" );
			break;
		}
		copied += written;
	}
	
	delete dst;
}

/*
====================
idFileSystemLocal::ReplaceSeparators

Fix things up differently for win/unix/mac
====================
*/
void idFileSystemLocal::ReplaceSeparators( idStr& path, char sep )
{
	char* s;
	
	for( s = &path[ 0 ]; *s ; s++ )
	{
		if( *s == '/' || *s == '\\' )
		{
			*s = sep;
		}
	}
}

/*
========================
IsOSPath
========================
*/
static bool IsOSPath( const char* path )
{
	assert( path );
	
	if( idStr::Icmpn( path, "mtp:", 4 ) == 0 )
	{
		return true;
	}
	
	
	if( idStr::Length( path ) >= 2 )
	{
		if( path[ 1 ] == ':' )
		{
			if( ( path[ 0 ] > 64 && path[ 0 ] < 91 ) || ( path[ 0 ] > 96 && path[ 0 ] < 123 ) )
			{
				// already an OS path starting with a drive.
				return true;
			}
		}
		if( path[ 0 ] == '\\' || path[ 0 ] == '/' )
		{
			// a root path
			return true;
		}
	}
	return false;
}

/*
========================
idFileSystemLocal::BuildOSPath
========================
*/
const char* idFileSystemLocal::BuildOSPath( const char* base, const char* relativePath )
{
	// handle case of this already being an OS path
	if( IsOSPath( relativePath ) )
	{
		return relativePath;
	}
	
	return BuildOSPath( base, gameFolder, relativePath );
}

/*
===================
idFileSystemLocal::BuildOSPath
===================
*/
const char* idFileSystemLocal::BuildOSPath( const char* base, const char* game, const char* relativePath )
{
	static char OSPath[MAX_STRING_CHARS];
	idStr newPath;
	
	// handle case of this already being an OS path
	if( IsOSPath( relativePath ) )
	{
		return relativePath;
	}
	
	idStr strBase = base;
	strBase.StripTrailing( '/' );
	strBase.StripTrailing( '\\' );
	sprintf( newPath, "%s/%s/%s", strBase.c_str(), game, relativePath );
	ReplaceSeparators( newPath );
	idStr::Copynz( OSPath, newPath, sizeof( OSPath ) );
	return OSPath;
}

/*
================
idFileSystemLocal::OSPathToRelativePath

takes a full OS path, as might be found in data from a media creation
program, and converts it to a relativePath by stripping off directories

Returns false if the osPath tree doesn't match any of the existing
search paths.

================
*/
const char* idFileSystemLocal::OSPathToRelativePath( const char* OSPath )
{
	if( ( OSPath[0] != '/' ) && ( OSPath[0] != '\\' ) && ( idStr::FindChar( OSPath, ':' ) < 0 ) )
	{
		// No colon and it doesn't start with a slash... it must already be a relative path
		return OSPath;
	}
	idStaticList< idStrStatic< 32 >, 5 > basePaths;
	basePaths.Append( "base" );
	basePaths.Append( "d3xp" );
	basePaths.Append( "d3le" );
	if( fs_game.GetString()[0] != 0 )
	{
		basePaths.Append( fs_game.GetString() );
	}
	if( fs_game_base.GetString()[0] != 0 )
	{
		basePaths.Append( fs_game_base.GetString() );
	}
	idStaticList<int, MAX_OSPATH> slashes;
	for( const char* s = OSPath; *s != 0; s++ )
	{
		if( *s == '/' || *s == '\\' )
		{
			slashes.Append( s - OSPath );
		}
	}
	for( int n = 0; n < slashes.Num() - 1; n++ )
	{
		const char* start = OSPath + slashes[n] + 1;
		const char* end = OSPath + slashes[n + 1];
		int componentLength = end - start;
		if( componentLength == 0 )
		{
			continue;
		}
		for( int i = 0; i < basePaths.Num(); i++ )
		{
			if( componentLength != basePaths[i].Length() )
			{
				continue;
			}
			if( basePaths[i].Icmpn( start, componentLength ) == 0 )
			{
				// There are some files like:
				// W:\d3xp\base\...
				// But we can't search backwards because there are others like:
				// W:\doom3\base\models\mapobjects\base\...
				// So instead we check for 2 base paths next to each other and take the 2nd in that case
				if( n < slashes.Num() - 2 )
				{
					const char* start2 = OSPath + slashes[n + 1] + 1;
					const char* end2 = OSPath + slashes[n + 2];
					int componentLength2 = end2 - start2;
					if( componentLength2 > 0 )
					{
						for( int j = 0; j < basePaths.Num(); j++ )
						{
							if( componentLength2 != basePaths[j].Length() )
							{
								continue;
							}
							if( basePaths[j].Icmpn( start2, basePaths[j].Length() ) == 0 )
							{
								return end2 + 1;
							}
						}
					}
				}
				return end + 1;
			}
		}
	}
	idLib::Warning( "OSPathToRelativePath failed on %s", OSPath );
	return OSPath;
}

/*
=====================
idFileSystemLocal::RelativePathToOSPath

Returns a fully qualified path that can be used with stdio libraries
=====================
*/
const char* idFileSystemLocal::RelativePathToOSPath( const char* relativePath, const char* basePath )
{
	const char* path = cvarSystem->GetCVarString( basePath );
	if( !path[0] )
	{
		path = fs_savepath.GetString();
	}
	return BuildOSPath( path, gameFolder, relativePath );
}

/*
=================
idFileSystemLocal::RemoveFile
=================
*/
void idFileSystemLocal::RemoveFile( const char* relativePath )
{
	idStr OSPath;
	
	if( fs_basepath.GetString()[0] )
	{
		OSPath = BuildOSPath( fs_basepath.GetString(), gameFolder, relativePath );
		
		// RB begin
#if defined(_WIN32)
		::DeleteFile( OSPath );
#else
		remove( OSPath );
#endif
		// RB end
	}
	
	OSPath = BuildOSPath( fs_savepath.GetString(), gameFolder, relativePath );
	
	// RB begin
#if defined(_WIN32)
	::DeleteFile( OSPath );
#else
	remove( OSPath );
#endif
	// RB end
}

/*
========================
idFileSystemLocal::RemoveDir
========================
*/
bool idFileSystemLocal::RemoveDir( const char* relativePath )
{
	bool success = true;
	if( fs_savepath.GetString()[0] )
	{
		success &= Sys_Rmdir( BuildOSPath( fs_savepath.GetString(), relativePath ) );
	}
	success &= Sys_Rmdir( BuildOSPath( fs_basepath.GetString(), relativePath ) );
	return success;
}

/*
============
idFileSystemLocal::ReadFile

Filename are relative to the search path
a null buffer will just return the file length and time without loading
timestamp can be NULL if not required
============
*/
int idFileSystemLocal::ReadFile( const char* relativePath, void** buffer, ID_TIME_T* timestamp )
{

	idFile* 	f;
	byte* 		buf;
	int			len;
	bool		isConfig;
	
	if( !IsInitialized() )
	{
		common->FatalError( "Filesystem call made without initialization\n" );
		return 0;
	}
	
	if( relativePath == NULL || !relativePath[0] )
	{
		common->FatalError( "idFileSystemLocal::ReadFile with empty name\n" );
		return 0;
	}
	
	if( timestamp )
	{
		*timestamp = FILE_NOT_FOUND_TIMESTAMP;
	}
	
	if( buffer )
	{
		*buffer = NULL;
	}
	
	buf = NULL;	// quiet compiler warning
	
	// if this is a .cfg file and we are playing back a journal, read
	// it from the journal file
	if( strstr( relativePath, ".cfg" ) == relativePath + strlen( relativePath ) - 4 )
	{
		isConfig = true;
		if( eventLoop && eventLoop->JournalLevel() == 2 )
		{
			int		r;
			
			loadCount++;
			loadStack++;
			
			common->DPrintf( "Loading %s from journal file.\n", relativePath );
			len = 0;
			r = eventLoop->com_journalDataFile->Read( &len, sizeof( len ) );
			if( r != sizeof( len ) )
			{
				*buffer = NULL;
				return -1;
			}
			buf = ( byte* )Mem_ClearedAlloc( len + 1, TAG_IDFILE );
			*buffer = buf;
			r = eventLoop->com_journalDataFile->Read( buf, len );
			if( r != len )
			{
				common->FatalError( "Read from journalDataFile failed" );
			}
			
			// guarantee that it will have a trailing 0 for string operations
			buf[len] = 0;
			
			return len;
		}
	}
	else
	{
		isConfig = false;
	}
	
	// look for it in the filesystem or pack files
	f = OpenFileRead( relativePath, ( buffer != NULL ) );
	if( f == NULL )
	{
		// RB: moved here
		if( buffer == NULL && timestamp != NULL && resourceFiles.Num() > 0 )
		{
			static idResourceCacheEntry rc;
			int size = 0;
			if( GetResourceCacheEntry( relativePath, rc ) )
			{
				*timestamp = 0;
				size = rc.length;
			}
			return size;
		}
		// RB end
		
		if( buffer )
		{
			*buffer = NULL;
		}
		
		return -1;
	}
	len = f->Length();
	
	if( timestamp )
	{
		*timestamp = f->Timestamp();
	}
	
	if( !buffer )
	{
		CloseFile( f );
		return len;
	}
	
	loadCount++;
	loadStack++;
	
	buf = ( byte* )Mem_ClearedAlloc( len + 1, TAG_IDFILE );
	*buffer = buf;
	
	f->Read( buf, len );
	
	// guarantee that it will have a trailing 0 for string operations
	buf[len] = 0;
	CloseFile( f );
	
	// if we are journalling and it is a config file, write it to the journal file
	if( isConfig && eventLoop && eventLoop->JournalLevel() == 1 )
	{
		common->DPrintf( "Writing %s to journal file.\n", relativePath );
		eventLoop->com_journalDataFile->Write( &len, sizeof( len ) );
		eventLoop->com_journalDataFile->Write( buf, len );
		eventLoop->com_journalDataFile->Flush();
	}
	
	return len;
}

/*
=============
idFileSystemLocal::FreeFile
=============
*/
void idFileSystemLocal::FreeFile( void* buffer )
{
	if( !IsInitialized() )
	{
		common->FatalError( "Filesystem call made without initialization\n" );
	}
	if( !buffer )
	{
		common->FatalError( "idFileSystemLocal::FreeFile( NULL )" );
	}
	loadStack--;
	
	Mem_Free( buffer );
}

/*
============
idFileSystemLocal::WriteFile

Filenames are relative to the search path
============
*/
int idFileSystemLocal::WriteFile( const char* relativePath, const void* buffer, int size, const char* basePath )
{
	idFile* f;
	
	if( !IsInitialized() )
	{
		common->FatalError( "Filesystem call made without initialization\n" );
	}
	
	if( !relativePath || !buffer )
	{
		common->FatalError( "idFileSystemLocal::WriteFile: NULL parameter" );
	}
	
	f = idFileSystemLocal::OpenFileWrite( relativePath, basePath );
	if( !f )
	{
		common->Printf( "Failed to open %s\n", relativePath );
		return -1;
	}
	
	size = f->Write( buffer, size );
	
	CloseFile( f );
	
	return size;
}

/*
========================
idFileSystemLocal::RenameFile
========================
*/
bool idFileSystemLocal::RenameFile( const char* relativePath, const char* newName, const char* basePath )
{
	const char* path = cvarSystem->GetCVarString( basePath );
	if( !path[0] )
	{
		path = fs_savepath.GetString();
	}
	
	idStr oldOSPath = BuildOSPath( path, gameFolder, relativePath );
	idStr newOSPath = BuildOSPath( path, gameFolder, newName );
	
	// RB begin
#if defined(_WIN32)
	// this gives atomic-delete-on-rename, like POSIX rename()
	// There is a MoveFileTransacted() on vista and above, not sure if that means there
	// is a race condition inside MoveFileEx...
	const bool success = ( MoveFileEx( oldOSPath.c_str(), newOSPath.c_str(), MOVEFILE_REPLACE_EXISTING ) != 0 );
	
	if( !success )
	{
		const int err = GetLastError();
		idLib::Warning( "RenameFile( %s, %s ) error %i", newOSPath.c_str(), oldOSPath.c_str(), err );
	}
#else
	const bool success = ( rename( oldOSPath.c_str(), newOSPath.c_str() ) == 0 );
	
	if( !success )
	{
		const int err = errno;
		idLib::Warning( "rename( %s, %s ) error %s", newOSPath.c_str(), oldOSPath.c_str(), strerror( errno ) );
	}
#endif
	// RB end
	
	return success;
}

/*
===============
idFileSystemLocal::AddUnique
===============
*/
int idFileSystemLocal::AddUnique( const char* name, idStrList& list, idHashIndex& hashIndex ) const
{
	int i, hashKey;
	
	hashKey = hashIndex.GenerateKey( name );
	for( i = hashIndex.First( hashKey ); i >= 0; i = hashIndex.Next( i ) )
	{
		if( list[i].Icmp( name ) == 0 )
		{
			return i;
		}
	}
	i = list.Append( name );
	hashIndex.Add( hashKey, i );
	return i;
}

/*
===============
idFileSystemLocal::GetExtensionList
===============
*/
void idFileSystemLocal::GetExtensionList( const char* extension, idStrList& extensionList ) const
{
	int s, e, l;
	
	l = idStr::Length( extension );
	s = 0;
	while( 1 )
	{
		e = idStr::FindChar( extension, '|', s, l );
		if( e != -1 )
		{
			extensionList.Append( idStr( extension, s, e ) );
			s = e + 1;
		}
		else
		{
			extensionList.Append( idStr( extension, s, l ) );
			break;
		}
	}
}

/*
===============
idFileSystemLocal::GetFileList

Does not clear the list first so this can be used to progressively build a file list.
When 'sort' is true only the new files added to the list are sorted.
===============
*/
int idFileSystemLocal::GetFileList( const char* relativePath, const idStrList& extensions, idStrList& list, idHashIndex& hashIndex, bool fullRelativePath, const char* gamedir )
{
	if( !IsInitialized() )
	{
		common->FatalError( "Filesystem call made without initialization\n" );
	}
	
	if( !extensions.Num() )
	{
		return 0;
	}
	
	if( !relativePath )
	{
		return 0;
	}
	
	int pathLength = strlen( relativePath );
	if( pathLength )
	{
		pathLength++;	// for the trailing '/'
	}
	
	idStrStatic< MAX_OSPATH > strippedName;
	if( resourceFiles.Num() > 0 )
	{
		int idx = resourceFiles.Num() - 1;
		while( idx >= 0 )
		{
			for( int i = 0; i < resourceFiles[ idx ]->cacheTable.Num(); i++ )
			{
				idResourceCacheEntry& rt = resourceFiles[ idx ]->cacheTable[ i ];
				// if the name is not long anough to at least contain the path
				
				if( rt.filename.Length() <= pathLength )
				{
					continue;
				}
				
				// check for a path match without the trailing '/'
				if( pathLength && idStr::Icmpn( rt.filename, relativePath, pathLength - 1 ) != 0 )
				{
					continue;
				}
				
				// ensure we have a path, and not just a filename containing the path
				if( rt.filename[ pathLength ] == '\0' || rt.filename[pathLength - 1] != '/' )
				{
					continue;
				}
				
				// make sure the file is not in a subdirectory
				int j = pathLength;
				for( ; rt.filename[j + 1] != '\0'; j++ )
				{
					if( rt.filename[ j ] == '/' )
					{
						break;
					}
				}
				if( rt.filename[ j + 1 ] )
				{
					continue;
				}
				
				// check for extension match
				for( j = 0; j < extensions.Num(); j++ )
				{
					if( rt.filename.Length() >= extensions[j].Length() && extensions[j].Icmp( rt.filename.c_str() +   rt.filename.Length() - extensions[j].Length() ) == 0 )
					{
						break;
					}
				}
				if( j >= extensions.Num() )
				{
					continue;
				}
				
				// unique the match
				if( fullRelativePath )
				{
					idStr work = relativePath;
					work += "/";
					work += rt.filename.c_str() + pathLength;
					work.StripTrailing( '/' );
					AddUnique( work, list, hashIndex );
				}
				else
				{
					idStr work = rt.filename.c_str() + pathLength;
					work.StripTrailing( '/' );
					AddUnique( work, list, hashIndex );
				}
			}
			idx--;
		}
	}
	
	// search through the path, one element at a time, adding to list
	for( int sp = searchPaths.Num() - 1; sp >= 0; sp-- )
	{
		if( gamedir != NULL && gamedir[0] != 0 )
		{
			if( searchPaths[sp].gamedir != gamedir )
			{
				continue;
			}
		}
		
		idStr netpath = BuildOSPath( searchPaths[sp].path, searchPaths[sp].gamedir, relativePath );
		
		for( int i = 0; i < extensions.Num(); i++ )
		{
		
			// scan for files in the filesystem
			idStrList sysFiles;
			ListOSFiles( netpath, extensions[i], sysFiles );
			
			// if we are searching for directories, remove . and ..
			if( extensions[i][0] == '/' && extensions[i][1] == 0 )
			{
				sysFiles.Remove( "." );
				sysFiles.Remove( ".." );
			}
			
			for( int j = 0; j < sysFiles.Num(); j++ )
			{
				// unique the match
				if( fullRelativePath )
				{
					idStr work = relativePath;
					work += "/";
					work += sysFiles[j];
					AddUnique( work, list, hashIndex );
				}
				else
				{
					AddUnique( sysFiles[j], list, hashIndex );
				}
			}
		}
	}
	
	return list.Num();
}

/*
===============
idFileSystemLocal::ListFiles
===============
*/
idFileList* idFileSystemLocal::ListFiles( const char* relativePath, const char* extension, bool sort, bool fullRelativePath, const char* gamedir )
{
	idHashIndex hashIndex( 4096, 4096 );
	idStrList extensionList;
	
	idFileList* fileList = new( TAG_IDFILE ) idFileList;
	fileList->basePath = relativePath;
	
	GetExtensionList( extension, extensionList );
	
	GetFileList( relativePath, extensionList, fileList->list, hashIndex, fullRelativePath, gamedir );
	
	if( sort )
	{
		fileList->list.SortWithTemplate( idSort_PathStr() );
	}
	
	return fileList;
}

/*
===============
idFileSystemLocal::GetFileListTree
===============
*/
int idFileSystemLocal::GetFileListTree( const char* relativePath, const idStrList& extensions, idStrList& list, idHashIndex& hashIndex, const char* gamedir )
{
	int i;
	idStrList slash, folders( 128 );
	idHashIndex folderHashIndex( 1024, 128 );
	
	// recurse through the subdirectories
	slash.Append( "/" );
	GetFileList( relativePath, slash, folders, folderHashIndex, true, gamedir );
	for( i = 0; i < folders.Num(); i++ )
	{
		if( folders[i][0] == '.' )
		{
			continue;
		}
		if( folders[i].Icmp( relativePath ) == 0 )
		{
			continue;
		}
		GetFileListTree( folders[i], extensions, list, hashIndex, gamedir );
	}
	
	// list files in the current directory
	GetFileList( relativePath, extensions, list, hashIndex, true, gamedir );
	
	return list.Num();
}

/*
===============
idFileSystemLocal::ListFilesTree
===============
*/
idFileList* idFileSystemLocal::ListFilesTree( const char* relativePath, const char* extension, bool sort, const char* gamedir )
{
	idHashIndex hashIndex( 4096, 4096 );
	idStrList extensionList;
	
	idFileList* fileList = new( TAG_IDFILE ) idFileList();
	fileList->basePath = relativePath;
	fileList->list.SetGranularity( 4096 );
	
	GetExtensionList( extension, extensionList );
	
	GetFileListTree( relativePath, extensionList, fileList->list, hashIndex, gamedir );
	
	if( sort )
	{
		fileList->list.SortWithTemplate( idSort_PathStr() );
	}
	
	return fileList;
}

/*
===============
idFileSystemLocal::FreeFileList
===============
*/
void idFileSystemLocal::FreeFileList( idFileList* fileList )
{
	delete fileList;
}

/*
===============
idFileSystemLocal::ListOSFiles

 call to the OS for a listing of files in an OS directory
===============
*/
int	idFileSystemLocal::ListOSFiles( const char* directory, const char* extension, idStrList& list )
{
	if( !extension )
	{
		extension = "";
	}
	
	return Sys_ListFiles( directory, extension, list );
}

/*
================
idFileSystemLocal::Dir_f
================
*/
void idFileSystemLocal::Dir_f( const idCmdArgs& args )
{
	idStr		relativePath;
	idStr		extension;
	idFileList* fileList;
	int			i;
	
	if( args.Argc() < 2 || args.Argc() > 3 )
	{
		common->Printf( "usage: dir <directory> [extension]\n" );
		return;
	}
	
	if( args.Argc() == 2 )
	{
		relativePath = args.Argv( 1 );
		extension = "";
	}
	else
	{
		relativePath = args.Argv( 1 );
		extension = args.Argv( 2 );
		if( extension[0] != '.' )
		{
			common->Warning( "extension should have a leading dot" );
		}
	}
	relativePath.BackSlashesToSlashes();
	relativePath.StripTrailing( '/' );
	
	common->Printf( "Listing of %s/*%s\n", relativePath.c_str(), extension.c_str() );
	common->Printf( "---------------\n" );
	
	fileList = fileSystemLocal.ListFiles( relativePath, extension );
	
	for( i = 0; i < fileList->GetNumFiles(); i++ )
	{
		common->Printf( "%s\n", fileList->GetFile( i ) );
	}
	common->Printf( "%d files\n", fileList->list.Num() );
	
	fileSystemLocal.FreeFileList( fileList );
}

/*
================
idFileSystemLocal::DirTree_f
================
*/
void idFileSystemLocal::DirTree_f( const idCmdArgs& args )
{
	idStr		relativePath;
	idStr		extension;
	idFileList* fileList;
	int			i;
	
	if( args.Argc() < 2 || args.Argc() > 3 )
	{
		common->Printf( "usage: dirtree <directory> [extension]\n" );
		return;
	}
	
	if( args.Argc() == 2 )
	{
		relativePath = args.Argv( 1 );
		extension = "";
	}
	else
	{
		relativePath = args.Argv( 1 );
		extension = args.Argv( 2 );
		if( extension[0] != '.' )
		{
			common->Warning( "extension should have a leading dot" );
		}
	}
	relativePath.BackSlashesToSlashes();
	relativePath.StripTrailing( '/' );
	
	common->Printf( "Listing of %s/*%s /s\n", relativePath.c_str(), extension.c_str() );
	common->Printf( "---------------\n" );
	
	fileList = fileSystemLocal.ListFilesTree( relativePath, extension );
	
	for( i = 0; i < fileList->GetNumFiles(); i++ )
	{
		common->Printf( "%s\n", fileList->GetFile( i ) );
	}
	common->Printf( "%d files\n", fileList->list.Num() );
	
	fileSystemLocal.FreeFileList( fileList );
}

/*
================
idFileSystemLocal::ClearResourcePacks
================
*/
void idFileSystemLocal::ClearResourcePacks()
{
}

/*
================
idFileSystemLocal::BuildGame_f
================
*/
void idFileSystemLocal::BuildGame_f( const idCmdArgs& args )
{
	fileSystemLocal.WriteResourcePacks();
}

/*
================
idFileSystemLocal::WriteResourceFile_f
================
*/
void idFileSystemLocal::WriteResourceFile_f( const idCmdArgs& args )
{
	if( args.Argc() != 2 )
	{
		common->Printf( "Usage: writeResourceFile <manifest file>\n" );
		return;
	}
	
	idStrList manifest;
	idResourceContainer::ReadManifestFile( args.Argv( 1 ), manifest );
	idResourceContainer::WriteResourceFile( args.Argv( 1 ), manifest, false );
}


/*
================
idFileSystemLocal::UpdateResourceFile_f
================
*/
void idFileSystemLocal::UpdateResourceFile_f( const idCmdArgs& args )
{
	if( args.Argc() < 3 )
	{
		common->Printf( "Usage: updateResourceFile <resource file> <files>\n" );
		return;
	}
	
	idStr filename =  args.Argv( 1 );
	idStrList filesToAdd;
	for( int i = 2; i < args.Argc(); i++ )
	{
		filesToAdd.Append( args.Argv( i ) );
	}
	idResourceContainer::UpdateResourceFile( filename, filesToAdd );
}

/*
================
idFileSystemLocal::ExtractResourceFile_f
================
*/
void idFileSystemLocal::ExtractResourceFile_f( const idCmdArgs& args )
{
	if( args.Argc() < 3 )
	{
		common->Printf( "Usage: extractResourceFile <resource file> <outpath> <copysound>\n" );
		return;
	}
	
	idStr filename =  args.Argv( 1 );
	idStr outPath = args.Argv( 2 );
	bool copyWaves = ( args.Argc() > 3 );
	idResourceContainer::ExtractResourceFile( filename, outPath, copyWaves );
}

/*
============
idFileSystemLocal::Path_f
============
*/
void idFileSystemLocal::Path_f( const idCmdArgs& args )
{
	common->Printf( "Current search path:\n" );
	for( int sp = fileSystemLocal.searchPaths.Num() - 1; sp >= 0; sp-- )
	{
		common->Printf( "%s/%s\n", fileSystemLocal.searchPaths[sp].path.c_str(), fileSystemLocal.searchPaths[sp].gamedir.c_str() );
	}
	
	for( int i = 0; i < fileSystemLocal.resourceFiles.Num(); i++ )
	{
		common->Printf( "%s\n", fileSystemLocal.resourceFiles[i]->GetFileName() );
	}
}

/*
============
idFileSystemLocal::TouchFile_f

The only purpose of this function is to allow game script files to copy
arbitrary files furing an "fs_copyfiles 1" run.
============
*/
void idFileSystemLocal::TouchFile_f( const idCmdArgs& args )
{
	idFile* f;
	
	if( args.Argc() != 2 )
	{
		common->Printf( "Usage: touchFile <file>\n" );
		return;
	}
	
	f = fileSystemLocal.OpenFileRead( args.Argv( 1 ) );
	if( f )
	{
		fileSystemLocal.CloseFile( f );
	}
}

/*
============
idFileSystemLocal::TouchFileList_f

Takes a text file and touches every file in it, use one file per line.
============
*/
void idFileSystemLocal::TouchFileList_f( const idCmdArgs& args )
{

	if( args.Argc() != 2 )
	{
		common->Printf( "Usage: touchFileList <filename>\n" );
		return;
	}
	
	const char* buffer = NULL;
	idParser src( LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_ALLOWBACKSLASHSTRINGCONCAT );
	if( fileSystem->ReadFile( args.Argv( 1 ), ( void** )&buffer, NULL ) && buffer )
	{
		src.LoadMemory( buffer, strlen( buffer ), args.Argv( 1 ) );
		if( src.IsLoaded() )
		{
			idToken token;
			while( src.ReadToken( &token ) )
			{
				common->Printf( "%s\n", token.c_str() );
				const bool captureToImage = false;
				common->UpdateScreen( captureToImage );
				idFile* f = fileSystemLocal.OpenFileRead( token );
				if( f )
				{
					fileSystemLocal.CloseFile( f );
				}
			}
		}
	}
	
}

/*
============
idFileSystemLocal::GenerateResourceCRCs_f

Generates a CRC checksum file for each .resources file.
============
*/
void idFileSystemLocal::GenerateResourceCRCs_f( const idCmdArgs& args )
{
	idLib::Printf( "Generating CRCs for resource files...\n" );
	
	std::auto_ptr<idFileList> baseResourceFileList( fileSystem->ListFiles( ".", ".resources" ) );
	if( baseResourceFileList.get() != NULL )
	{
		CreateCRCsForResourceFileList( *baseResourceFileList );
	}
	
	std::auto_ptr<idFileList> mapResourceFileList( fileSystem->ListFilesTree( "maps", ".resources" ) );
	if( mapResourceFileList.get() != NULL )
	{
		CreateCRCsForResourceFileList( *mapResourceFileList );
	}
	
	idLib::Printf( "Done generating CRCs for resource files.\n" );
}

/*
================
idFileSystemLocal::CreateCRCsForResourceFileList
================
*/
void idFileSystemLocal::CreateCRCsForResourceFileList( const idFileList& list )
{
	for( int fileIndex = 0; fileIndex < list.GetNumFiles(); ++fileIndex )
	{
		idLib::Printf( " Processing %s.\n", list.GetFile( fileIndex ) );
		
		std::auto_ptr<idFile_Memory> currentFile( static_cast<idFile_Memory*>( fileSystem->OpenFileReadMemory( list.GetFile( fileIndex ) ) ) );
		
		if( currentFile.get() == NULL )
		{
			idLib::Printf( " Error reading %s.\n", list.GetFile( fileIndex ) );
			continue;
		}
		
		uint32 resourceMagic;
		currentFile->ReadBig( resourceMagic );
		
		if( resourceMagic != RESOURCE_FILE_MAGIC )
		{
			idLib::Printf( "Resource file magic number doesn't match, skipping %s.\n", list.GetFile( fileIndex ) );
			continue;
		}
		
		int tableOffset;
		currentFile->ReadBig( tableOffset );
		
		int tableLength;
		currentFile->ReadBig( tableLength );
		
		// Read in the table
		currentFile->Seek( tableOffset, FS_SEEK_SET );
		
		int numFileResources;
		currentFile->ReadBig( numFileResources );
		
		idList< idResourceCacheEntry > cacheEntries;
		cacheEntries.SetNum( numFileResources );
		
		for( int innerFileIndex = 0; innerFileIndex < numFileResources; ++innerFileIndex )
		{
			cacheEntries[innerFileIndex].Read( currentFile.get() );
		}
		
		// All tables read, now seek to each one and calculate the CRC.
		idTempArray< unsigned int > innerFileCRCs( numFileResources ); // DG: use int instead of long for 64bit compatibility
		for( int innerFileIndex = 0; innerFileIndex < numFileResources; ++innerFileIndex )
		{
			const char* innerFileDataBegin = currentFile->GetDataPtr() + cacheEntries[innerFileIndex].offset;
			
			innerFileCRCs[innerFileIndex] = CRC32_BlockChecksum( innerFileDataBegin, cacheEntries[innerFileIndex].length );
		}
		
		// Get the CRC for all the CRCs.
		const unsigned int totalCRC = CRC32_BlockChecksum( innerFileCRCs.Ptr(), innerFileCRCs.Size() ); // DG: use int instead of long for 64bit compatibility
		
		// Write the .crc file corresponding to the .resources file.
		idStr crcFilename = list.GetFile( fileIndex );
		crcFilename.SetFileExtension( ".crc" );
		std::auto_ptr<idFile> crcOutputFile( fileSystem->OpenFileWrite( crcFilename, "fs_basepath" ) );
		if( crcOutputFile.get() == NULL )
		{
			// RB: fixed potential crash because of "cannot pass objects of non-trivially-copyable type 'class idStr' through '...'"
			idLib::Printf( "Error writing CRC file %s.\n", crcFilename.c_str() );
			// RB end
			continue;
		}
		
		const uint32 CRC_FILE_MAGIC = 0xCC00CC00; // I just made this up, it has no meaning.
		const uint32 CRC_FILE_VERSION = 1;
		crcOutputFile->WriteBig( CRC_FILE_MAGIC );
		crcOutputFile->WriteBig( CRC_FILE_VERSION );
		crcOutputFile->WriteBig( totalCRC );
		crcOutputFile->WriteBig( numFileResources );
		crcOutputFile->WriteBigArray( innerFileCRCs.Ptr(), numFileResources );
	}
}

/*
================
idFileSystemLocal::AddResourceFile
================
*/
int idFileSystemLocal::AddResourceFile( const char* resourceFileName )
{
	// RB: check if it was already added
	int idx = FindResourceFile( resourceFileName );
	if( idx != -1 )
	{
		return idx;
	}
	// RB end
	
	idStrStatic< MAX_OSPATH > resourceFile = va( "maps/%s", resourceFileName );
	idResourceContainer* rc = new idResourceContainer();
	if( rc->Init( resourceFile, resourceFiles.Num() ) )
	{
		resourceFiles.Append( rc );
		common->Printf( "Loaded resource file %s\n", resourceFile.c_str() );
		return resourceFiles.Num() - 1;
	}
	return -1;
}

/*
================
idFileSystemLocal::FindResourceFile
================
*/
int idFileSystemLocal::FindResourceFile( const char* resourceFileName )
{
	for( int i = 0; i < resourceFiles.Num(); i++ )
	{
		if( idStr::Icmp( resourceFileName, resourceFiles[ i ]->GetFileName() ) == 0 )
		{
			return i;
		}
	}
	return -1;
}
/*
================
idFileSystemLocal::RemoveResourceFileByIndex
================
*/
void idFileSystemLocal::RemoveResourceFileByIndex( const int& idx )
{
	if( idx >= 0 && idx < resourceFiles.Num() )
	{
		if( idx >= 0 && idx < resourceFiles.Num() )
		{
			delete resourceFiles[ idx ];
			resourceFiles.RemoveIndex( idx );
			for( int i = 0; i < resourceFiles.Num(); i++ )
			{
				// fixup any container indexes
				resourceFiles[ i ]->SetContainerIndex( i );
			}
		}
	}
}

/*
================
idFileSystemLocal::RemoveMapResourceFile
================
*/
void idFileSystemLocal::RemoveMapResourceFile( const char* resourceFileName )
{
	int idx = FindResourceFile( va( "maps/%s", resourceFileName ) );
	if( idx >= 0 )
	{
		RemoveResourceFileByIndex( idx );
	}
}

/*
================
idFileSystemLocal::RemoveResourceFile
================
*/
void idFileSystemLocal::RemoveResourceFile( const char* resourceFileName )
{
	int idx = FindResourceFile( resourceFileName );
	if( idx >= 0 )
	{
		RemoveResourceFileByIndex( idx );
	}
}

/*
================
idFileSystemLocal::AddGameDirectory

Sets gameFolder, adds the directory to the head of the search paths
================
*/
void idFileSystemLocal::AddGameDirectory( const char* path, const char* dir )
{
	// check if the search path already exists
	for( int i = 0; i < searchPaths.Num(); i++ )
	{
		if( searchPaths[i].path.Cmp( path ) == 0 && searchPaths[i].gamedir.Cmp( dir ) == 0 )
		{
			return;
		}
	}
	
	gameFolder = dir;
	
	//
	// add the directory to the search path
	//
	searchpath_t& search = searchPaths.Alloc();
	search.path = path;
	search.gamedir = dir;
	
	// RB: add all maps/*.resources
	idStr pakfile;
	for( int i = 0; i < 2; i++ )
	{
		if( i == 1 )
		{
			pakfile = BuildOSPath( path, dir, "maps" );
		}
		else
		{
			pakfile = BuildOSPath( path, dir, "" );
			pakfile[ pakfile.Length() - 1 ] = 0;	// strip the trailing slash
		}
		
		idStrList pakfiles;
		ListOSFiles( pakfile, ".resources", pakfiles );
		pakfiles.SortWithTemplate( idSort_PathStr() );
		if( pakfiles.Num() > 0 )
		{
			// resource files present, ignore pak files
			for( int j = 0; j < pakfiles.Num(); j++ )
			{
				pakfile = pakfiles[j]; //BuildOSPath( path, dir, pakfiles[i] );
				
				if( i == 1 )
				{
					pakfile.Insert( "maps/", 0 );
				}
				
				idResourceContainer* rc = new idResourceContainer();
				if( rc->Init( pakfile, resourceFiles.Num() ) )
				{
					resourceFiles.Append( rc );
					common->Printf( "Loaded resource file %s\n", pakfile.c_str() );
					//com_productionMode.SetInteger( 2 );
				}
			}
		}
	}
	// RB end
}

/*
================
idFileSystemLocal::SetupGameDirectories

  Takes care of the correct search order.
================
*/
void idFileSystemLocal::SetupGameDirectories( const char* gameName )
{
	// setup basepath
	if( fs_basepath.GetString()[0] )
	{
		AddGameDirectory( fs_basepath.GetString(), gameName );
	}
	// setup savepath
	if( fs_savepath.GetString()[0] )
	{
		AddGameDirectory( fs_savepath.GetString(), gameName );
	}
}

/*
================
idFileSystemLocal::InitPrecache
================
*/
void idFileSystemLocal::InitPrecache()
{
	if( !fs_enableBackgroundCaching.GetBool() )
	{
		return;
	}
	numFilesOpenedAsCached = 0;
}

/*
================
idFileSystemLocal::ReOpenCacheFiles
================
*/
void idFileSystemLocal::ReOpenCacheFiles()
{

	if( !fs_enableBackgroundCaching.GetBool() )
	{
		return;
	}
}


/*
================
idFileSystemLocal::Startup
================
*/
void idFileSystemLocal::Startup()
{
	common->Printf( "------ Initializing File System ------\n" );
	
	InitPrecache();
	
	SetupGameDirectories( BASE_GAMEDIR );
	
	// fs_game_base override
	if( fs_game_base.GetString()[0] &&
			idStr::Icmp( fs_game_base.GetString(), BASE_GAMEDIR ) )
	{
		SetupGameDirectories( fs_game_base.GetString() );
	}
	
	// fs_game override
	if( fs_game.GetString()[0] &&
			idStr::Icmp( fs_game.GetString(), BASE_GAMEDIR ) &&
			idStr::Icmp( fs_game.GetString(), fs_game_base.GetString() ) )
	{
		SetupGameDirectories( fs_game.GetString() );
	}
	
	// add our commands
	cmdSystem->AddCommand( "dir", Dir_f, CMD_FL_SYSTEM, "lists a folder", idCmdSystem::ArgCompletion_FileName );
	cmdSystem->AddCommand( "dirtree", DirTree_f, CMD_FL_SYSTEM, "lists a folder with subfolders" );
	cmdSystem->AddCommand( "path", Path_f, CMD_FL_SYSTEM, "lists search paths" );
	cmdSystem->AddCommand( "touchFile", TouchFile_f, CMD_FL_SYSTEM, "touches a file" );
	cmdSystem->AddCommand( "touchFileList", TouchFileList_f, CMD_FL_SYSTEM, "touches a list of files" );
	
	cmdSystem->AddCommand( "buildGame", BuildGame_f, CMD_FL_SYSTEM, "builds game pak files" );
	cmdSystem->AddCommand( "writeResourceFile", WriteResourceFile_f, CMD_FL_SYSTEM, "writes a .resources file from a supplied manifest" );
	cmdSystem->AddCommand( "extractResourceFile", ExtractResourceFile_f, CMD_FL_SYSTEM, "extracts to the supplied resource file to the supplied path" );
	cmdSystem->AddCommand( "updateResourceFile", UpdateResourceFile_f, CMD_FL_SYSTEM, "updates or appends the supplied files in the supplied resource file" );
	
	cmdSystem->AddCommand( "generateResourceCRCs", GenerateResourceCRCs_f, CMD_FL_SYSTEM, "Generates CRC checksums for all the resource files." );
	
	// print the current search paths
	Path_f( idCmdArgs() );
	
	common->Printf( "file system initialized.\n" );
	common->Printf( "--------------------------------------\n" );
}

/*
================
idFileSystemLocal::Init

Called only at inital startup, not when the filesystem
is resetting due to a game change
================
*/
void idFileSystemLocal::Init()
{
	// allow command line parms to override our defaults
	// we have to specially handle this, because normal command
	// line variable sets don't happen until after the filesystem
	// has already been initialized
	common->StartupVariable( "fs_basepath" );
	common->StartupVariable( "fs_savepath" );
	common->StartupVariable( "fs_game" );
	common->StartupVariable( "fs_game_base" );
	common->StartupVariable( "fs_copyfiles" );
	
	if( fs_basepath.GetString()[0] == '\0' )
	{
		fs_basepath.SetString( Sys_DefaultBasePath() );
	}
	if( fs_savepath.GetString()[0] == '\0' )
	{
		fs_savepath.SetString( Sys_DefaultSavePath() );
	}
	
	// try to start up normally
	Startup();
	
	// if we can't find default.cfg, assume that the paths are
	// busted and error out now, rather than getting an unreadable
	// graphics screen when the font fails to load
	// Dedicated servers can run with no outside files at all
	if( ReadFile( "default.cfg", NULL, NULL ) <= 0 )
	{
		common->FatalError( "Couldn't load default.cfg" );
	}
}

/*
================
idFileSystemLocal::Restart
================
*/
void idFileSystemLocal::Restart()
{
	// free anything we currently have loaded
	Shutdown( true );
	
	Startup();
	
	// if we can't find default.cfg, assume that the paths are
	// busted and error out now, rather than getting an unreadable
	// graphics screen when the font fails to load
	if( ReadFile( "default.cfg", NULL, NULL ) <= 0 )
	{
		common->FatalError( "Couldn't load default.cfg" );
	}
}

/*
================
idFileSystemLocal::Shutdown

Frees all resources and closes all files
================
*/
void idFileSystemLocal::Shutdown( bool reloading )
{
	gameFolder.Clear();
	searchPaths.Clear();
	
	resourceFiles.DeleteContents();
	
	
	cmdSystem->RemoveCommand( "path" );
	cmdSystem->RemoveCommand( "dir" );
	cmdSystem->RemoveCommand( "dirtree" );
	cmdSystem->RemoveCommand( "touchFile" );
}

/*
================
idFileSystemLocal::IsInitialized
================
*/
bool idFileSystemLocal::IsInitialized() const
{
	return ( searchPaths.Num() != 0 );
}


/*
=================================================================================

Opening files

=================================================================================
*/

/*
========================
idFileSystemLocal::GetResourceCacheEntry

Returns false if the entry isn't found
========================
*/
bool idFileSystemLocal::GetResourceCacheEntry( const char* fileName, idResourceCacheEntry& rc )
{
	idStrStatic< MAX_OSPATH > canonical;
	if( strstr( fileName, ":" ) != NULL )
	{
		// os path, convert to relative? scripts can pass in an OS path
		//idLib::Printf( "RESOURCE: os path passed %s\n", fileName );
		// DG: this should return a bool, i.e. false, not NULL
		return false;
		// DG end
	}
	else
	{
		canonical = fileName;
	}
	
	canonical.BackSlashesToSlashes();
	canonical.ToLower();
	int idx = resourceFiles.Num() - 1;
	while( idx >= 0 )
	{
		const int key = resourceFiles[ idx ]->cacheHash.GenerateKey( canonical, false );
		for( int index = resourceFiles[ idx ]->cacheHash.GetFirst( key ); index != idHashIndex::NULL_INDEX; index = resourceFiles[ idx ]->cacheHash.GetNext( index ) )
		{
			idResourceCacheEntry& rt = resourceFiles[ idx ]->cacheTable[ index ];
			if( idStr::Icmp( rt.filename, canonical ) == 0 )
			{
				rc.filename = rt.filename;
				rc.length = rt.length;
				rc.containerIndex = idx;
				rc.offset = rt.offset;
				return true;
			}
		}
		idx--;
	}
	return false;
}

/*
========================
idFileSystemLocal::GetResourceFile

Returns NULL
========================
*/

idFile* idFileSystemLocal::GetResourceFile( const char* fileName, bool memFile )
{

	if( resourceFiles.Num() == 0 )
	{
		return NULL;
	}
	
	static idResourceCacheEntry rc;
	if( GetResourceCacheEntry( fileName, rc ) )
	{
		if( fs_debugResources.GetBool() )
		{
			idLib::Printf( "RES: loading file %s\n", rc.filename.c_str() );
		}
		idFile_InnerResource* file = new idFile_InnerResource( rc.filename, resourceFiles[ rc.containerIndex ]->resourceFile, rc.offset, rc.length );
		// DG: add parenthesis to make sure this block is only entered when file != NULL - bug found by clang.
		if( file != NULL && ( ( memFile || rc.length <= resourceBufferAvailable ) || rc.length < 8 * 1024 * 1024 ) )
		{
			byte* buf = NULL;
			if( rc.length < resourceBufferAvailable )
			{
				buf = resourceBufferPtr;
				resourceBufferAvailable = 0;
			}
			else
			{
				if( fs_debugResources.GetBool() )
				{
					idLib::Printf( "MEM: Allocating %05d bytes for a resource load\n", rc.length );
				}
				buf = ( byte* )Mem_Alloc( rc.length, TAG_TEMP );
			}
			file->Read( ( void* )buf, rc.length );
			
			if( buf == resourceBufferPtr )
			{
				file->SetResourceBuffer( buf );
				return file;
			}
			else
			{
				idFile_Memory* mfile = new idFile_Memory( rc.filename, ( const char* )buf, rc.length );
				if( mfile != NULL )
				{
					mfile->TakeDataOwnership();
					delete file;
					return mfile;
				}
			}
		}
		return file;
	}
	
	return NULL;
}


/*
===========
idFileSystemLocal::OpenFileReadFlags

Finds the file in the search path, following search flag recommendations
Returns filesize and an open FILE pointer.
Used for streaming data out of either a
separate file or a ZIP file.
===========
*/
idFile* idFileSystemLocal::OpenFileReadFlags( const char* relativePath, int searchFlags, bool allowCopyFiles, const char* gamedir )
{

	if( !IsInitialized() )
	{
		common->FatalError( "Filesystem call made without initialization\n" );
		return NULL;
	}
	
	if( relativePath == NULL )
	{
		common->FatalError( "idFileSystemLocal::OpenFileRead: NULL 'relativePath' parameter passed\n" );
		return NULL;
	}
	
	// qpaths are not supposed to have a leading slash
	if( relativePath[0] == '/' || relativePath[0] == '\\' )
	{
		relativePath++;
	}
	
	// make absolutely sure that it can't back up the path.
	// The searchpaths do guarantee that something will always
	// be prepended, so we don't need to worry about "c:" or "//limbo"
	if( strstr( relativePath, ".." ) || strstr( relativePath, "::" ) )
	{
		return NULL;
	}
	
	// edge case
	if( relativePath[0] == '\0' )
	{
		return NULL;
	}
	
	if( fs_debug.GetBool() )
	{
		idLib::Printf( "FILE DEBUG: opening %s\n", relativePath );
	}
	
	if( resourceFiles.Num() > 0 && fs_resourceLoadPriority.GetInteger() ==  1 )
	{
		idFile* rf = GetResourceFile( relativePath, ( searchFlags & FSFLAG_RETURN_FILE_MEM ) != 0 );
		if( rf != NULL )
		{
			return rf;
		}
	}
	
	//
	// search through the path, one element at a time
	//
	if( searchFlags & FSFLAG_SEARCH_DIRS )
	{
		for( int sp = searchPaths.Num() - 1; sp >= 0; sp-- )
		{
			if( gamedir != NULL && gamedir[0] != 0 )
			{
				if( searchPaths[sp].gamedir != gamedir )
				{
					continue;
				}
			}
			
			idStr netpath = BuildOSPath( searchPaths[sp].path, searchPaths[sp].gamedir, relativePath );
			idFileHandle fp = OpenOSFile( netpath, FS_READ );
			if( !fp )
			{
				continue;
			}
			
			idFile_Permanent* file = new( TAG_IDFILE ) idFile_Permanent();
			file->o = fp;
			file->name = relativePath;
			file->fullPath = netpath;
			file->mode = ( 1 << FS_READ );
			file->fileSize = DirectFileLength( file->o );
			if( fs_debug.GetInteger() )
			{
				common->Printf( "idFileSystem::OpenFileRead: %s (found in '%s/%s')\n", relativePath, searchPaths[sp].path.c_str(), searchPaths[sp].gamedir.c_str() );
			}
			
			// if fs_copyfiles is set
			if( allowCopyFiles )
			{
			
				idStr copypath;
				idStr name;
				copypath = BuildOSPath( fs_savepath.GetString(), searchPaths[sp].gamedir, relativePath );
				netpath.ExtractFileName( name );
				copypath.StripFilename();
				copypath += PATHSEPARATOR_STR;
				copypath += name;
				
				if( fs_buildResources.GetBool() )
				{
					idStrStatic< MAX_OSPATH > relativePath = OSPathToRelativePath( copypath );
					relativePath.BackSlashesToSlashes();
					relativePath.ToLower();
					
					if( IsSoundSample( relativePath ) )
					{
						idStrStatic< MAX_OSPATH > samplePath = relativePath;
						samplePath.SetFileExtension( "idwav" );
						if( samplePath.Find( "generated/" ) == -1 )
						{
							samplePath.Insert( "generated/", 0 );
						}
						fileManifest.AddUnique( samplePath );
						if( relativePath.Find( "/vo/", false ) >= 0 )
						{
							// this is vo so add the language variants
							for( int i = 0; i < Sys_NumLangs(); i++ )
							{
								const char* lang = Sys_Lang( i );
								if( idStr::Icmp( lang, ID_LANG_ENGLISH ) == 0 )
								{
									continue;
								}
								samplePath = relativePath;
								samplePath.Replace( "/vo/", va( "/vo/%s/", lang ) );
								samplePath.SetFileExtension( "idwav" );
								if( samplePath.Find( "generated/" ) == -1 )
								{
									samplePath.Insert( "generated/", 0 );
								}
								fileManifest.AddUnique( samplePath );
								
							}
						}
					}
					else if( relativePath.Icmpn( "guis/", 5 ) == 0 )
					{
						// this is a gui so add the language variants
						for( int i = 0; i < Sys_NumLangs(); i++ )
						{
							const char* lang = Sys_Lang( i );
							if( idStr::Icmp( lang, ID_LANG_ENGLISH ) == 0 )
							{
								fileManifest.Append( relativePath );
								continue;
							}
							idStrStatic< MAX_OSPATH > guiPath = relativePath;
							guiPath.Replace( "guis/", va( "guis/%s/", lang ) );
							fileManifest.Append( guiPath );
						}
					}
					else
					{
						// never add .amp files
						if( strstr( relativePath, ".amp" ) == NULL )
						{
							fileManifest.Append( relativePath );
						}
					}
					
				}
				
				if( fs_copyfiles.GetBool() )
				{
					CopyFile( netpath, copypath );
				}
			}
			
			if( searchFlags & FSFLAG_RETURN_FILE_MEM )
			{
				idFile_Memory* memFile = new( TAG_IDFILE ) idFile_Memory( file->name );
				memFile->SetLength( file->fileSize );
				file->Read( ( void* )memFile->GetDataPtr(), file->fileSize );
				delete file;
				memFile->TakeDataOwnership();
				return memFile;
			}
			
			return file;
		}
	}
	
	if( resourceFiles.Num() > 0 && fs_resourceLoadPriority.GetInteger() ==  0 )
	{
		idFile* rf = GetResourceFile( relativePath, ( searchFlags & FSFLAG_RETURN_FILE_MEM ) != 0 );
		if( rf != NULL )
		{
			return rf;
		}
	}
	
	if( fs_debug.GetInteger( ) )
	{
		common->Printf( "Can't find %s\n", relativePath );
	}
	
	return NULL;
}

/*
===========
idFileSystemLocal::OpenFileRead
===========
*/
idFile* idFileSystemLocal::OpenFileRead( const char* relativePath, bool allowCopyFiles, const char* gamedir )
{
	return OpenFileReadFlags( relativePath, FSFLAG_SEARCH_DIRS, allowCopyFiles, gamedir );
}

/*
===========
idFileSystemLocal::OpenFileReadMemory
===========
*/
idFile* idFileSystemLocal::OpenFileReadMemory( const char* relativePath, bool allowCopyFiles, const char* gamedir )
{
	return OpenFileReadFlags( relativePath, FSFLAG_SEARCH_DIRS | FSFLAG_RETURN_FILE_MEM, allowCopyFiles, gamedir );
}

/*
===========
idFileSystemLocal::OpenFileWrite
===========
*/
idFile* idFileSystemLocal::OpenFileWrite( const char* relativePath, const char* basePath )
{

	const char* path;
	idStr OSpath;
	idFile_Permanent* f;
	
	if( !IsInitialized() )
	{
		common->FatalError( "Filesystem call made without initialization\n" );
	}
	
	path = cvarSystem->GetCVarString( basePath );
	if( !path[0] )
	{
		path = fs_savepath.GetString();
	}
	
	OSpath = BuildOSPath( path, gameFolder, relativePath );
	
	if( fs_debug.GetInteger() )
	{
		common->Printf( "idFileSystem::OpenFileWrite: %s\n", OSpath.c_str() );
	}
	
	common->DPrintf( "writing to: %s\n", OSpath.c_str() );
	CreateOSPath( OSpath );
	
	f = new( TAG_IDFILE ) idFile_Permanent();
	f->o = OpenOSFile( OSpath, FS_WRITE );
	if( !f->o )
	{
		delete f;
		return NULL;
	}
	f->name = relativePath;
	f->fullPath = OSpath;
	f->mode = ( 1 << FS_WRITE );
	f->handleSync = false;
	f->fileSize = 0;
	
	return f;
}

/*
===========
idFileSystemLocal::OpenExplicitFileRead
===========
*/
idFile* idFileSystemLocal::OpenExplicitFileRead( const char* OSPath )
{
	idFile_Permanent* f;
	
	if( !IsInitialized() )
	{
		common->FatalError( "Filesystem call made without initialization\n" );
	}
	
	if( fs_debug.GetInteger() )
	{
		common->Printf( "idFileSystem::OpenExplicitFileRead: %s\n", OSPath );
	}
	
	//common->DPrintf( "idFileSystem::OpenExplicitFileRead - reading from: %s\n", OSPath );
	
	f = new( TAG_IDFILE ) idFile_Permanent();
	f->o = OpenOSFile( OSPath, FS_READ );
	if( !f->o )
	{
		delete f;
		return NULL;
	}
	f->name = OSPath;
	f->fullPath = OSPath;
	f->mode = ( 1 << FS_READ );
	f->handleSync = false;
	f->fileSize = DirectFileLength( f->o );
	
	return f;
}

/*
===========
idFileSystemLocal::OpenExplicitPakFile
===========
*/
idFile_Cached* idFileSystemLocal::OpenExplicitPakFile( const char* OSPath )
{
	idFile_Cached* f;
	
	if( !IsInitialized() )
	{
		common->FatalError( "Filesystem call made without initialization\n" );
	}
	
	//if ( fs_debug.GetInteger() ) {
	//	common->Printf( "idFileSystem::OpenExplicitFileRead: %s\n", OSPath );
	//}
	
	//common->DPrintf( "idFileSystem::OpenExplicitFileRead - reading from: %s\n", OSPath );
	
	f = new( TAG_IDFILE ) idFile_Cached();
	f->o = OpenOSFile( OSPath, FS_READ );
	if( !f->o )
	{
		delete f;
		return NULL;
	}
	f->name = OSPath;
	f->fullPath = OSPath;
	f->mode = ( 1 << FS_READ );
	f->handleSync = false;
	f->fileSize = DirectFileLength( f->o );
	
	return f;
}

/*
===========
idFileSystemLocal::OpenExplicitFileWrite
===========
*/
idFile* idFileSystemLocal::OpenExplicitFileWrite( const char* OSPath )
{
	idFile_Permanent* f;
	
	if( !IsInitialized() )
	{
		common->FatalError( "Filesystem call made without initialization\n" );
	}
	
	if( fs_debug.GetInteger() )
	{
		common->Printf( "idFileSystem::OpenExplicitFileWrite: %s\n", OSPath );
	}
	
	common->DPrintf( "writing to: %s\n", OSPath );
	CreateOSPath( OSPath );
	
	f = new( TAG_IDFILE ) idFile_Permanent();
	f->o = OpenOSFile( OSPath, FS_WRITE );
	if( !f->o )
	{
		delete f;
		return NULL;
	}
	f->name = OSPath;
	f->fullPath = OSPath;
	f->mode = ( 1 << FS_WRITE );
	f->handleSync = false;
	f->fileSize = 0;
	
	return f;
}

/*
===========
idFileSystemLocal::OpenFileAppend
===========
*/
idFile* idFileSystemLocal::OpenFileAppend( const char* relativePath, bool sync, const char* basePath )
{

	const char* path;
	idStr OSpath;
	idFile_Permanent* f;
	
	if( !IsInitialized() )
	{
		common->FatalError( "Filesystem call made without initialization\n" );
	}
	
	path = cvarSystem->GetCVarString( basePath );
	if( !path[0] )
	{
		path = fs_savepath.GetString();
	}
	
	OSpath = BuildOSPath( path, gameFolder, relativePath );
	CreateOSPath( OSpath );
	
	if( fs_debug.GetInteger() )
	{
		common->Printf( "idFileSystem::OpenFileAppend: %s\n", OSpath.c_str() );
	}
	
	f = new( TAG_IDFILE ) idFile_Permanent();
	f->o = OpenOSFile( OSpath, FS_APPEND );
	if( !f->o )
	{
		delete f;
		return NULL;
	}
	f->name = relativePath;
	f->fullPath = OSpath;
	f->mode = ( 1 << FS_WRITE ) + ( 1 << FS_APPEND );
	f->handleSync = sync;
	f->fileSize = DirectFileLength( f->o );
	
	return f;
}

/*
================
idFileSystemLocal::OpenFileByMode
================
*/
idFile* idFileSystemLocal::OpenFileByMode( const char* relativePath, fsMode_t mode )
{
	if( mode == FS_READ )
	{
		return OpenFileRead( relativePath );
	}
	if( mode == FS_WRITE )
	{
		return OpenFileWrite( relativePath );
	}
	if( mode == FS_APPEND )
	{
		return OpenFileAppend( relativePath, true );
	}
	common->FatalError( "idFileSystemLocal::OpenFileByMode: bad mode" );
	return NULL;
}

/*
==============
idFileSystemLocal::CloseFile
==============
*/
void idFileSystemLocal::CloseFile( idFile* f )
{
	if( !IsInitialized() )
	{
		common->FatalError( "Filesystem call made without initialization\n" );
	}
	delete f;
}

/*
=================
idFileSystemLocal::FindDLL
=================
*/
void idFileSystemLocal::FindDLL( const char* name, char _dllPath[ MAX_OSPATH ] )
{
	char dllName[MAX_OSPATH];
	sys->DLL_GetFileName( name, dllName, MAX_OSPATH );
	
	// from executable directory first - this is handy for developement
	idStr dllPath = Sys_EXEPath( );
	dllPath.StripFilename( );
	dllPath.AppendPath( dllName );
	idFile* dllFile = OpenExplicitFileRead( dllPath );
	
	if( dllFile )
	{
		dllPath = dllFile->GetFullPath();
		CloseFile( dllFile );
		dllFile = NULL;
	}
	else
	{
		dllPath = "";
	}
	idStr::snPrintf( _dllPath, MAX_OSPATH, dllPath.c_str() );
}

/*
===============
idFileSystemLocal::FindFile
===============
*/
findFile_t idFileSystemLocal::FindFile( const char* path )
{
	idFile* f = OpenFileReadFlags( path, FSFLAG_SEARCH_DIRS );
	if( f == NULL )
	{
		return FIND_NO;
	}
	delete f;
	return FIND_YES;
}

/*
===============
idFileSystemLocal::IsFolder
===============
*/
sysFolder_t idFileSystemLocal::IsFolder( const char* relativePath, const char* basePath )
{
	return Sys_IsFolder( RelativePathToOSPath( relativePath, basePath ) );
}
