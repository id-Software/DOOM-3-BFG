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

#ifndef __FILE_RESOURCE_H__
#define __FILE_RESOURCE_H__

/*
==============================================================

  Resource containers

==============================================================
*/

class idResourceCacheEntry {
public:
	idResourceCacheEntry() {
		Clear();
	}
	void Clear() {
		filename.Empty();
		//filename = NULL;
		offset = 0;
		length = 0;
		containerIndex = 0;
	}
	size_t Read( idFile *f ) {
		size_t sz = f->ReadString( filename );
		sz += f->ReadBig( offset );
		sz += f->ReadBig( length );
		return sz;
	}
	size_t Write( idFile *f ) {
		size_t sz = f->WriteString( filename );
		sz += f->WriteBig( offset );
		sz += f->WriteBig( length );
		return sz;
	}
	idStrStatic< 256 >	filename;
	int					offset;							// into the resource file
	int 				length;
	uint8				containerIndex;
};

static const uint32 RESOURCE_FILE_MAGIC = 0xD000000D;
class idResourceContainer {
	friend class	idFileSystemLocal;
	//friend class	idReadSpawnThread;
public:
	idResourceContainer() {
		resourceFile = NULL;
		tableOffset = 0;
		tableLength = 0;
		resourceMagic = 0;
		numFileResources = 0;
	}
	~idResourceContainer() {
		delete resourceFile;
		cacheTable.Clear();
	}
	bool Init( const char * fileName, uint8 containerIndex );
	static void WriteResourceFile( const char *fileName, const idStrList &manifest, const bool &_writeManifest );
	static void WriteManifestFile( const char *name, const idStrList &list );
	static int ReadManifestFile( const char *filename, idStrList &list );
	static void ExtractResourceFile ( const char * fileName, const char * outPath, bool copyWavs );
	static void UpdateResourceFile( const char *filename, const idStrList &filesToAdd );
	idFile *OpenFile( const char *fileName );
	const char * GetFileName() const { return fileName.c_str(); }
	void SetContainerIndex( const int & _idx );
	void ReOpen();
private:
	idStrStatic< 256 > fileName;
	idFile *	resourceFile;			// open file handle
	// offset should probably be a 64 bit value for development, but 4 gigs won't fit on
	// a DVD layer, so it isn't a retail limitation.
	int		tableOffset;			// table offset
	int		tableLength;			// table length
	int		resourceMagic;			// magic
	int		numFileResources;		// number of file resources in this container
	idList< idResourceCacheEntry, TAG_RESOURCE>	cacheTable;
	idHashIndex	cacheHash;
};


#endif /* !__FILE_RESOURCE_H__ */
