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
#ifndef __FILE_SAVEGAME_H__
#define __FILE_SAVEGAME_H__

#include <zlib.h>

// Listing of the types of files within a savegame package
enum saveGameType_t
{
	SAVEGAMEFILE_NONE			= 0,
	SAVEGAMEFILE_TEXT			= BIT( 0 ),	// implies that no checksum will be used
	SAVEGAMEFILE_BINARY			= BIT( 1 ),	// implies that a checksum will also be used
	SAVEGAMEFILE_COMPRESSED		= BIT( 2 ),
	SAVEGAMEFILE_PIPELINED		= BIT( 3 ),
	SAVEGAMEFILE_THUMB			= BIT( 4 ),	// for special processing on certain platforms
	SAVEGAMEFILE_BKGRND_IMAGE	= BIT( 5 ),	// for special processing on certain platforms, large background used on PS3
	SAVEGAMEFILE_AUTO_DELETE	= BIT( 6 ),	// to be deleted automatically after completed
	SAVEGAMEFILE_OPTIONAL		= BIT( 7 )	// if this flag is not set and missing, there is an error
};

/*
================================================
idFile_SaveGame
================================================
*/
class idFile_SaveGame : public idFile_Memory
{
public:
	idFile_SaveGame() : type( SAVEGAMEFILE_NONE ), error( false ) {}
	idFile_SaveGame( const char* _name ) : idFile_Memory( _name ), type( SAVEGAMEFILE_NONE ), error( false ) {}
	idFile_SaveGame( const char* _name, int type_ ) : idFile_Memory( _name ), type( type_ ), error( false ) {}

	virtual ~idFile_SaveGame() { }

	bool operator==( const idFile_SaveGame& other ) const
	{
		return idStr::Icmp( GetName(), other.GetName() ) == 0;
	}
	bool operator==( const char* _name ) const
	{
		return idStr::Icmp( GetName(), _name ) == 0;
	}
	void SetNameAndType( const char* _name, int _type )
	{
		name = _name;
		type = _type;
	}
public: // TODO_KC_CR for now...

	int					type;			// helps platform determine what to do with the file (encrypt, checksum, etc.)
	bool				error;			// when loading, this is set if there is a problem
};

/*
================================================
idFile_SaveGamePipelined uses threads to pipeline overlap compression and IO
================================================
*/
class idSGFreadThread;
class idSGFwriteThread;
class idSGFdecompressThread;
class idSGFcompressThread;

struct blockForIO_t
{
	byte* 		data;
	size_t		bytes;
};

class idFile_SaveGamePipelined : public idFile
{
public:
	// The buffers each hold two blocks of data, so one block can be operated on by
	// the next part of the generate / compress / IO pipeline.  The factor of two
	// size difference between the uncompressed and compressed blocks is unrelated
	// to the fact that there are two blocks in each buffer.
	static const int COMPRESSED_BLOCK_SIZE		= 128 * 1024;
	static const int UNCOMPRESSED_BLOCK_SIZE	= 256 * 1024;


	idFile_SaveGamePipelined();
	virtual					~idFile_SaveGamePipelined();

	bool					OpenForReading( const char* const filename, bool useNativeFile );
	bool					OpenForWriting( const char* const filename, bool useNativeFile );

	bool					OpenForReading( idFile* file );
	bool					OpenForWriting( idFile* file );

	// Finish any reading or writing.
	void					Finish();

	// Abort any reading or writing.
	void					Abort();

	// Cancel any reading or writing for app termination
	static void				CancelToTerminate()
	{
		cancelToTerminate = true;
	}

	bool					ReadBuildVersion();
	const char* 			GetBuildVersion() const
	{
		return buildVersion;
	}

	bool					ReadSaveFormatVersion();
	int						GetSaveFormatVersion() const
	{
		return saveFormatVersion;
	}
	int						GetPointerSize() const;

	//------------------------
	// idFile Interface
	//------------------------

	virtual const char* 	GetName() const
	{
		return name.c_str();
	}
	virtual const char* 	GetFullPath() const
	{
		return name.c_str();
	}
	virtual int				Read( void* buffer, int len );
	virtual int				Write( const void* buffer, int len );

	// this file is strictly streaming, you can't seek at all
	virtual int				Length() const
	{
		// RB: 64 bit fix, we don't need support for files bigger than 2 GB
		return ( int ) compressedLength;
		// RB end
	}
	virtual void			SetLength( size_t len )
	{
		compressedLength = len;
	}
	virtual int				Tell() const
	{
		assert( 0 );
		return 0;
	}
	virtual int				Seek( long offset, fsOrigin_t origin )
	{
		assert( 0 );
		return 0;
	}

	virtual ID_TIME_T		Timestamp()	const
	{
		return 0;
	}

	//------------------------
	// These can be used by a background thread to read/write data
	// when the file was opened with 'useNativeFile' set to false.
	//------------------------

	enum mode_t
	{
		CLOSED,
		WRITE,
		READ
	};

	// Get the file mode: read/write.
	mode_t					GetMode() const
	{
		return mode;
	}

	// Called by a background thread to get the next block to be written out.
	// This may block until a block has been made available through the pipeline.
	// Pass in NULL to notify the last write failed.
	// Returns false if there are no more blocks.
	bool					NextWriteBlock( blockForIO_t* block );

	// Called by a background thread to get the next block to read data into and to
	// report the number of bytes written to the previous block.
	// This may block until space is available to place the next block.
	// Pass in NULL to notify the end of the file was reached.
	// Returns false if there are no more blocks.
	bool					NextReadBlock( blockForIO_t* block, size_t lastReadBytes );

private:
	friend class idSGFreadThread;
	friend class idSGFwriteThread;
	friend class idSGFdecompressThread;
	friend class idSGFcompressThread;

	idStr					name;		// Name of the file.
	idStr					osPath;		// OS path.
	mode_t					mode;		// Open mode.
	size_t					compressedLength;

	static const int COMPRESSED_BUFFER_SIZE		= COMPRESSED_BLOCK_SIZE * 2;
	static const int UNCOMPRESSED_BUFFER_SIZE	= UNCOMPRESSED_BLOCK_SIZE * 2;

	byte					uncompressed[UNCOMPRESSED_BUFFER_SIZE];
	size_t					uncompressedProducedBytes;	// not masked
	size_t					uncompressedConsumedBytes;	// not masked

	byte					compressed[COMPRESSED_BUFFER_SIZE];
	size_t					compressedProducedBytes;	// not masked
	size_t					compressedConsumedBytes;	// not masked

	//------------------------
	// These variables are used to pass data between threads in a thread-safe manner.
	//------------------------

	byte* 					dataZlib;
	size_t					bytesZlib;

	byte* 					dataIO;
	size_t					bytesIO;

	//------------------------
	// These variables are used by CompressBlock() and DecompressBlock().
	//------------------------

	z_stream				zStream;
	int						zLibFlushType;		// Z_NO_FLUSH or Z_FINISH
	bool					zStreamEndHit;
	int						numChecksums;

	//------------------------
	// These variables are used by WriteBlock() and ReadBlock().
	//------------------------

	idFile* 				nativeFile;
	bool					nativeFileEndHit;
	bool					finished;

	//------------------------
	// The background threads and signals for NextWriteBlock() and NextReadBlock().
	//------------------------

	idSGFreadThread* 		readThread;
	idSGFwriteThread* 		writeThread;

	idSGFdecompressThread* 	decompressThread;
	idSGFcompressThread* 	compressThread;

	idSysSignal				blockRequested;
	idSysSignal				blockAvailable;
	idSysSignal				blockFinished;

	idStrStatic< 32 >		buildVersion;		// build version this file was saved with
	int16					pointerSize;		// the number of bytes in a pointer, because different pointer sizes mean different offsets into objects a 64 bit build cannot load games saved from a 32 bit build or vice version (a value of 0 is interpreted as 4 bytes)
	int16					saveFormatVersion;	// version number specific to save games (for maintaining save compatibility across builds)

	//------------------------
	// These variables are used when we want to abort due to the termination of the application
	//------------------------
	static bool				cancelToTerminate;

	void					FlushUncompressedBlock();
	void					FlushCompressedBlock();
	void					CompressBlock();
	void					WriteBlock();

	void					PumpUncompressedBlock();
	void					PumpCompressedBlock();
	void					DecompressBlock();
	void					ReadBlock();
};

#endif // !__FILE_SAVEGAME_H__
