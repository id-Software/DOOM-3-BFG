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

#ifndef __FILE_MANIFEST_H__
#define __FILE_MANIFEST_H__

/*
==============================================================

  File and preload manifests.

==============================================================
*/

class idFileManifest
{
public:
	idFileManifest()
	{
		cacheTable.SetGranularity( 4096 );
		cacheHash.SetGranularity( 4096 );
	}
	~idFileManifest() {}
	
	bool LoadManifest( const char* fileName );
	bool LoadManifestFromFile( idFile* file );
	void WriteManifestFile( const char* fileName );
	
	int NumFiles()
	{
		return cacheTable.Num();
	}
	
	int FindFile( const char* fileName );
	
	const idStr& GetFileNameByIndex( int idx ) const;
	
	
	const char* GetManifestName()
	{
		return filename;
	}
	
	void RemoveAll( const char* filename );
	void AddFile( const char* filename );
	
	void PopulateList( idStaticList< idStr, 16384 >& dest )
	{
		dest.Clear();
		for( int i = 0; i < cacheTable.Num(); i++ )
		{
			dest.Append( cacheTable[ i ] );
		}
	}
	
	void Print()
	{
		idLib::Printf( "dump for manifest %s\n", GetManifestName() );
		idLib::Printf( "---------------------------------------\n" );
		for( int i = 0; i < NumFiles(); i++ )
		{
			const idStr& name = GetFileNameByIndex( i );
			if( name.Find( ".idwav", false ) >= 0 )
			{
				idLib::Printf( "%s\n", GetFileNameByIndex( i ).c_str() );
			}
		}
	}
	
private:
	idStrList cacheTable;
	idHashIndex	cacheHash;
	idStr filename;
};

// image preload
struct imagePreload_s
{
	imagePreload_s()
	{
		filter = 0;
		repeat = 0;
		usage = 0;
		cubeMap = 0;
	}
	void Write( idFile* f )
	{
		f->WriteBig( filter );
		f->WriteBig( repeat );
		f->WriteBig( usage );
		f->WriteBig( cubeMap );
	}
	void Read( idFile* f )
	{
		f->ReadBig( filter );
		f->ReadBig( repeat );
		f->ReadBig( usage );
		f->ReadBig( cubeMap );
	}
	bool operator==( const imagePreload_s& b ) const
	{
		return ( filter == b.filter && repeat == b.repeat && usage == b.usage && cubeMap == b.cubeMap );
	}
	int filter;
	int repeat;
	int usage;
	int cubeMap;
};

enum preloadType_t
{
	PRELOAD_IMAGE,
	PRELOAD_MODEL,
	PRELOAD_SAMPLE,
	PRELOAD_ANIM,
	PRELOAD_COLLISION,
	PRELOAD_PARTICLE
};

// preload
struct preloadEntry_s
{
	preloadEntry_s()
	{
		resType = 0;
	}
	bool operator==( const preloadEntry_s& b ) const
	{
		bool ret = ( resourceName.Icmp( b.resourceName ) == 0 );
		if( ret && resType == PRELOAD_IMAGE )
		{
			// this should never matter but...
			ret &= ( imgData == b.imgData );
		}
		return ret;
	}
	void Write( idFile* outFile )
	{
		outFile->WriteBig( resType );
		outFile->WriteString( resourceName );
		imgData.Write( outFile );
	}
	
	void Read( idFile* inFile )
	{
		inFile->ReadBig( resType );
		inFile->ReadString( resourceName );
		imgData.Read( inFile );
	}
	
	int				resType;		// type
	idStr			resourceName;	// resource name
	imagePreload_s	imgData;		// image specific data
};

struct preloadSort_t
{
	int idx;
	int ofs;
};
class idSort_Preload : public idSort_Quick< preloadSort_t, idSort_Preload >
{
public:
	int Compare( const preloadSort_t& a, const preloadSort_t& b ) const
	{
		return a.ofs - b.ofs;
	}
};

class idPreloadManifest
{
public:
	idPreloadManifest()
	{
		entries.SetGranularity( 2048 );
	}
	~idPreloadManifest() {}
	
	bool LoadManifest( const char* fileName );
	bool LoadManifestFromFile( idFile* file );
	
	void WriteManifest( const char* fileName );
	void WriteManifestToFile( idFile* outFile )
	{
		if( outFile == NULL )
		{
			return;
		}
		filename = outFile->GetName();
		outFile->WriteBig( ( int )entries.Num() );
		outFile->WriteString( filename );
		for( int i = 0; i < entries.Num(); i++ )
		{
			entries[ i ].Write( outFile );
		}
	}
	
	int NumResources() const
	{
		return entries.Num();
	}
	
	const preloadEntry_s& GetPreloadByIndex( int idx ) const
	{
		return entries[ idx ];
	}
	
	const idStr& GetResourceNameByIndex( int idx ) const
	{
		return entries[ idx ].resourceName;
	}
	
	const char* GetManifestName() const
	{
		return filename;
	}
	
	void Add( const preloadEntry_s& p )
	{
		entries.AddUnique( p );
	}
	
	void AddSample( const char* _resourceName )
	{
		static preloadEntry_s pe;
		pe.resType = PRELOAD_SAMPLE;
		pe.resourceName = _resourceName;
		entries.Append( pe );
	}
	void AddModel( const char* _resourceName )
	{
		static preloadEntry_s pe;
		pe.resType = PRELOAD_MODEL;
		pe.resourceName = _resourceName;
		entries.Append( pe );
	}
	void AddParticle( const char* _resourceName )
	{
		static preloadEntry_s pe;
		pe.resType = PRELOAD_PARTICLE;
		pe.resourceName = _resourceName;
		entries.Append( pe );
	}
	void AddAnim( const char* _resourceName )
	{
		static preloadEntry_s pe;
		pe.resType = PRELOAD_ANIM;
		pe.resourceName = _resourceName;
		entries.Append( pe );
	}
	void AddCollisionModel( const char* _resourceName )
	{
		static preloadEntry_s pe;
		pe.resType = PRELOAD_COLLISION;
		pe.resourceName = _resourceName;
		entries.Append( pe );
	}
	void AddImage( const char* _resourceName, int _filter, int _repeat, int _usage, int _cube )
	{
		static preloadEntry_s pe;
		pe.resType = PRELOAD_IMAGE;
		pe.resourceName = _resourceName;
		pe.imgData.filter = _filter;
		pe.imgData.repeat = _repeat;
		pe.imgData.usage = _usage;
		pe.imgData.cubeMap = _cube;
		entries.Append( pe );
	}
	
	void Clear()
	{
		entries.Clear();
	}
	
	int FindResource( const char* name )
	{
		for( int i = 0; i < entries.Num(); i++ )
		{
			if( idStr::Icmp( name, entries[ i ].resourceName ) == 0 )
			{
				return i;
			}
		}
		return -1;
	}
	
	void Print()
	{
		idLib::Printf( "dump for preload manifest %s\n", GetManifestName() );
		idLib::Printf( "---------------------------------------\n" );
		for( int i = 0; i < NumResources(); i++ )
		{
			idLib::Printf( "%s\n", GetResourceNameByIndex( i ).c_str() );
		}
	}
private:
	idList< preloadEntry_s > entries;
	idStr filename;
};


#endif /* !__FILE_MANIFEST_H__ */
