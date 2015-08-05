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

GUIs and script remain separately parsed

Following a parse, all referenced media (and other decls) will have been touched.

sinTable and cosTable are required for the rotate material keyword to function

A new FindType on a purged decl will cause it to be reloaded, but a stale pointer to a purged
decl will look like a defaulted decl.

Moving a decl from one file to another will not be handled correctly by a reload, the material
will be defaulted.

NULL or empty decl names will always return NULL
	Should probably make a default decl for this

Decls are initially created without a textSource
A parse without textSource set should always just call MakeDefault()
A parse that has an error should internally call MakeDefault()
A purge does nothing to a defaulted decl

Should we have a "purged" media state separate from the "defaulted" media state?

reloading over a decl name that was defaulted

reloading over a decl name that was valid

missing reload over a previously explicit definition

*/

#define USE_COMPRESSED_DECLS
//#define GET_HUFFMAN_FREQUENCIES

class idDeclType
{
public:
	idStr						typeName;
	declType_t					type;
	idDecl* 					( *allocator )();
};

class idDeclFolder
{
public:
	idStr						folder;
	idStr						extension;
	declType_t					defaultType;
};

class idDeclFile;

class idDeclLocal : public idDeclBase
{
	friend class idDeclFile;
	friend class idDeclManagerLocal;
	
public:
	idDeclLocal();
	virtual 					~idDeclLocal() {};
	virtual const char* 		GetName() const;
	virtual declType_t			GetType() const;
	virtual declState_t			GetState() const;
	virtual bool				IsImplicit() const;
	virtual bool				IsValid() const;
	virtual void				Invalidate();
	virtual void				Reload();
	virtual void				EnsureNotPurged();
	virtual int					Index() const;
	virtual int					GetLineNum() const;
	virtual const char* 		GetFileName() const;
	virtual size_t				Size() const;
	virtual void				GetText( char* text ) const;
	virtual int					GetTextLength() const;
	virtual void				SetText( const char* text );
	virtual bool				ReplaceSourceFileText();
	virtual bool				SourceFileChanged() const;
	virtual void				MakeDefault();
	virtual bool				EverReferenced() const;
	
protected:
	virtual bool				SetDefaultText();
	virtual const char* 		DefaultDefinition() const;
	virtual bool				Parse( const char* text, const int textLength, bool allowBinaryVersion );
	virtual void				FreeData();
	virtual void				List() const;
	virtual void				Print() const;
	
protected:
	void						AllocateSelf();
	
	// Parses the decl definition.
	// After calling parse, a decl will be guaranteed usable.
	void						ParseLocal();
	
	// Does a MakeDefualt, but flags the decl so that it
	// will Parse() the next time the decl is found.
	void						Purge();
	
	// Set textSource possible with compression.
	void						SetTextLocal( const char* text, const int length );
	
private:
	idDecl* 					self;
	
	idStr						name;					// name of the decl
	char* 						textSource;				// decl text definition
	int							textLength;				// length of textSource
	int							compressedLength;		// compressed length
	idDeclFile* 				sourceFile;				// source file in which the decl was defined
	int							sourceTextOffset;		// offset in source file to decl text
	int							sourceTextLength;		// length of decl text in source file
	int							sourceLine;				// this is where the actual declaration token starts
	int							checksum;				// checksum of the decl text
	declType_t					type;					// decl type
	declState_t					declState;				// decl state
	int							index;					// index in the per-type list
	
	bool						parsedOutsideLevelLoad;	// these decls will never be purged
	bool						everReferenced;			// set to true if the decl was ever used
	bool						referencedThisLevel;	// set to true when the decl is used for the current level
	bool						redefinedInReload;		// used during file reloading to make sure a decl that has
	// its source removed will be defaulted
	idDeclLocal* 				nextInFile;				// next decl in the decl file
};

class idDeclFile
{
public:
	idDeclFile();
	idDeclFile( const char* fileName, declType_t defaultType );
	
	void						Reload( bool force );
	int							LoadAndParse();
	
public:
	idStr						fileName;
	declType_t					defaultType;
	
	ID_TIME_T						timestamp;
	int							checksum;
	int							fileSize;
	int							numLines;
	
	idDeclLocal* 				decls;
};

class idDeclManagerLocal : public idDeclManager
{
	friend class idDeclLocal;
	
public:
	virtual void				Init();
	virtual void				Init2();
	virtual void				Shutdown();
	virtual void				Reload( bool force );
	virtual void				BeginLevelLoad();
	virtual void				EndLevelLoad();
	virtual void				RegisterDeclType( const char* typeName, declType_t type, idDecl * ( *allocator )() );
	virtual void				RegisterDeclFolder( const char* folder, const char* extension, declType_t defaultType );
	virtual int					GetChecksum() const;
	virtual int					GetNumDeclTypes() const;
	virtual int					GetNumDecls( declType_t type );
	virtual const char* 		GetDeclNameFromType( declType_t type ) const;
	virtual declType_t			GetDeclTypeFromName( const char* typeName ) const;
	virtual const idDecl* 		FindType( declType_t type, const char* name, bool makeDefault = true );
	virtual const idDecl* 		DeclByIndex( declType_t type, int index, bool forceParse = true );
	
	virtual const idDecl*		FindDeclWithoutParsing( declType_t type, const char* name, bool makeDefault = true );
	virtual void				ReloadFile( const char* filename, bool force );
	
	virtual void				ListType( const idCmdArgs& args, declType_t type );
	virtual void				PrintType( const idCmdArgs& args, declType_t type );
	
	virtual idDecl* 			CreateNewDecl( declType_t type, const char* name, const char* fileName );
	
	//BSM Added for the material editors rename capabilities
	virtual bool				RenameDecl( declType_t type, const char* oldName, const char* newName );
	
	virtual void				MediaPrint( VERIFY_FORMAT_STRING const char* fmt, ... ) ID_INSTANCE_ATTRIBUTE_PRINTF( 1, 2 );
	virtual void				WritePrecacheCommands( idFile* f );
	
	virtual const idMaterial* 		FindMaterial( const char* name, bool makeDefault = true );
	virtual const idDeclSkin* 		FindSkin( const char* name, bool makeDefault = true );
	virtual const idSoundShader* 	FindSound( const char* name, bool makeDefault = true );
	
	virtual const idMaterial* 		MaterialByIndex( int index, bool forceParse = true );
	virtual const idDeclSkin* 		SkinByIndex( int index, bool forceParse = true );
	virtual const idSoundShader* 	SoundByIndex( int index, bool forceParse = true );
	
	virtual void					Touch( const idDecl* decl );
	
public:
	static void					MakeNameCanonical( const char* name, char* result, int maxLength );
	idDeclLocal* 				FindTypeWithoutParsing( declType_t type, const char* name, bool makeDefault = true );
	
	idDeclType* 				GetDeclType( int type ) const
	{
		return declTypes[type];
	}
	const idDeclFile* 			GetImplicitDeclFile() const
	{
		return &implicitDecls;
	}
	
	void						ConvertPDAsToStrings( const idCmdArgs& args );
	
private:
	idSysMutex					mutex;
	
	idList<idDeclType*, TAG_IDLIB_LIST_DECL>		declTypes;
	idList<idDeclFolder*, TAG_IDLIB_LIST_DECL>		declFolders;
	
	idList<idDeclFile*, TAG_IDLIB_LIST_DECL>		loadedFiles;
	idHashIndex					hashTables[DECL_MAX_TYPES];
	idList<idDeclLocal*, TAG_IDLIB_LIST_DECL>		linearLists[DECL_MAX_TYPES];
	idDeclFile					implicitDecls;	// this holds all the decls that were created because explicit
	// text definitions were not found. Decls that became default
	// because of a parse error are not in this list.
	int							checksum;		// checksum of all loaded decl text
	int							indent;			// for MediaPrint
	bool						insideLevelLoad;
	
	static idCVar				decl_show;
	
private:
	static void					ListDecls_f( const idCmdArgs& args );
	static void					ReloadDecls_f( const idCmdArgs& args );
	static void					TouchDecl_f( const idCmdArgs& args );
};

idCVar idDeclManagerLocal::decl_show( "decl_show", "0", CVAR_SYSTEM, "set to 1 to print parses, 2 to also print references", 0, 2, idCmdSystem::ArgCompletion_Integer<0, 2> );

idDeclManagerLocal	declManagerLocal;
idDeclManager* 		declManager = &declManagerLocal;

/*
====================================================================================

 decl text huffman compression

====================================================================================
*/

const int MAX_HUFFMAN_SYMBOLS	= 256;

typedef struct huffmanNode_s
{
	int						symbol;
	int						frequency;
	struct huffmanNode_s* 	next;
	struct huffmanNode_s* 	children[2];
} huffmanNode_t;

typedef struct huffmanCode_s
{
	unsigned int			bits[8]; // DG: use int instead of long for 64bit compatibility
	int						numBits;
} huffmanCode_t;

// compression ratio = 64%
static int huffmanFrequencies[] =
{
	0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
	0x00000001, 0x00078fb6, 0x000352a7, 0x00000002, 0x00000001, 0x0002795e, 0x00000001, 0x00000001,
	0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
	0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
	0x00049600, 0x000000dd, 0x00018732, 0x0000005a, 0x00000007, 0x00000092, 0x0000000a, 0x00000919,
	0x00002dcf, 0x00002dda, 0x00004dfc, 0x0000039a, 0x000058be, 0x00002d13, 0x00014d8c, 0x00023c60,
	0x0002ddb0, 0x0000d1fc, 0x000078c4, 0x00003ec7, 0x00003113, 0x00006b59, 0x00002499, 0x0000184a,
	0x0000250b, 0x00004e38, 0x000001ca, 0x00000011, 0x00000020, 0x000023da, 0x00000012, 0x00000091,
	0x0000000b, 0x00000b14, 0x0000035d, 0x0000137e, 0x000020c9, 0x00000e11, 0x000004b4, 0x00000737,
	0x000006b8, 0x00001110, 0x000006b3, 0x000000fe, 0x00000f02, 0x00000d73, 0x000005f6, 0x00000be4,
	0x00000d86, 0x0000014d, 0x00000d89, 0x0000129b, 0x00000db3, 0x0000015a, 0x00000167, 0x00000375,
	0x00000028, 0x00000112, 0x00000018, 0x00000678, 0x0000081a, 0x00000677, 0x00000003, 0x00018112,
	0x00000001, 0x000441ee, 0x000124b0, 0x0001fa3f, 0x00026125, 0x0005a411, 0x0000e50f, 0x00011820,
	0x00010f13, 0x0002e723, 0x00003518, 0x00005738, 0x0002cc26, 0x0002a9b7, 0x0002db81, 0x0003b5fa,
	0x000185d2, 0x00001299, 0x00030773, 0x0003920d, 0x000411cd, 0x00018751, 0x00005fbd, 0x000099b0,
	0x00009242, 0x00007cf2, 0x00002809, 0x00005a1d, 0x00000001, 0x00005a1d, 0x00000001, 0x00000001,
	
	0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
	0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
	0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
	0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
	0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
	0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
	0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
	0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
	0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
	0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
	0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
	0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
	0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
	0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
	0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
	0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
};

static huffmanCode_t huffmanCodes[MAX_HUFFMAN_SYMBOLS];
static huffmanNode_t* huffmanTree = NULL;
static int totalUncompressedLength = 0;
static int totalCompressedLength = 0;
static int maxHuffmanBits = 0;


/*
================
ClearHuffmanFrequencies
================
*/
void ClearHuffmanFrequencies()
{
	int i;
	
	for( i = 0; i < MAX_HUFFMAN_SYMBOLS; i++ )
	{
		huffmanFrequencies[i] = 1;
	}
}

/*
================
InsertHuffmanNode
================
*/
huffmanNode_t* InsertHuffmanNode( huffmanNode_t* firstNode, huffmanNode_t* node )
{
	huffmanNode_t* n, *lastNode;
	
	lastNode = NULL;
	for( n = firstNode; n; n = n->next )
	{
		if( node->frequency <= n->frequency )
		{
			break;
		}
		lastNode = n;
	}
	if( lastNode )
	{
		node->next = lastNode->next;
		lastNode->next = node;
	}
	else
	{
		node->next = firstNode;
		firstNode = node;
	}
	return firstNode;
}

/*
================
BuildHuffmanCode_r
================
*/
void BuildHuffmanCode_r( huffmanNode_t* node, huffmanCode_t code, huffmanCode_t codes[MAX_HUFFMAN_SYMBOLS] )
{
	if( node->symbol == -1 )
	{
		huffmanCode_t newCode = code;
		assert( code.numBits < sizeof( codes[0].bits ) * 8 );
		newCode.numBits++;
		if( code.numBits > maxHuffmanBits )
		{
			maxHuffmanBits = newCode.numBits;
		}
		BuildHuffmanCode_r( node->children[0], newCode, codes );
		newCode.bits[code.numBits >> 5] |= 1 << ( code.numBits & 31 );
		BuildHuffmanCode_r( node->children[1], newCode, codes );
	}
	else
	{
		assert( code.numBits <= sizeof( codes[0].bits ) * 8 );
		codes[node->symbol] = code;
	}
}

/*
================
FreeHuffmanTree_r
================
*/
void FreeHuffmanTree_r( huffmanNode_t* node )
{
	if( node->symbol == -1 )
	{
		FreeHuffmanTree_r( node->children[0] );
		FreeHuffmanTree_r( node->children[1] );
	}
	delete node;
}

/*
================
HuffmanHeight_r
================
*/
int HuffmanHeight_r( huffmanNode_t* node )
{
	if( node == NULL )
	{
		return -1;
	}
	int left = HuffmanHeight_r( node->children[0] );
	int right = HuffmanHeight_r( node->children[1] );
	if( left > right )
	{
		return left + 1;
	}
	return right + 1;
}

/*
================
SetupHuffman
================
*/
void SetupHuffman()
{
	int i, height;
	huffmanNode_t* firstNode, *node;
	huffmanCode_t code;
	
	firstNode = NULL;
	for( i = 0; i < MAX_HUFFMAN_SYMBOLS; i++ )
	{
		node = new( TAG_DECL ) huffmanNode_t;
		node->symbol = i;
		node->frequency = huffmanFrequencies[i];
		node->next = NULL;
		node->children[0] = NULL;
		node->children[1] = NULL;
		firstNode = InsertHuffmanNode( firstNode, node );
	}
	
	for( i = 1; i < MAX_HUFFMAN_SYMBOLS; i++ )
	{
		node = new( TAG_DECL ) huffmanNode_t;
		node->symbol = -1;
		node->frequency = firstNode->frequency + firstNode->next->frequency;
		node->next = NULL;
		node->children[0] = firstNode;
		node->children[1] = firstNode->next;
		firstNode = InsertHuffmanNode( firstNode->next->next, node );
	}
	
	maxHuffmanBits = 0;
	memset( &code, 0, sizeof( code ) );
	BuildHuffmanCode_r( firstNode, code, huffmanCodes );
	
	huffmanTree = firstNode;
	
	height = HuffmanHeight_r( firstNode );
	assert( maxHuffmanBits == height );
}

/*
================
ShutdownHuffman
================
*/
void ShutdownHuffman()
{
	if( huffmanTree )
	{
		FreeHuffmanTree_r( huffmanTree );
	}
}

/*
================
HuffmanCompressText
================
*/
int HuffmanCompressText( const char* text, int textLength, byte* compressed, int maxCompressedSize )
{
	int i, j;
	idBitMsg msg;
	
	totalUncompressedLength += textLength;
	
	msg.InitWrite( compressed, maxCompressedSize );
	msg.BeginWriting();
	for( i = 0; i < textLength; i++ )
	{
		const huffmanCode_t& code = huffmanCodes[( unsigned char )text[i]];
		for( j = 0; j < ( code.numBits >> 5 ); j++ )
		{
			msg.WriteBits( code.bits[j], 32 );
		}
		if( code.numBits & 31 )
		{
			msg.WriteBits( code.bits[j], code.numBits & 31 );
		}
	}
	
	totalCompressedLength += msg.GetSize();
	
	return msg.GetSize();
}

/*
================
HuffmanDecompressText
================
*/
int HuffmanDecompressText( char* text, int textLength, const byte* compressed, int compressedSize )
{
	int i, bit;
	idBitMsg msg;
	huffmanNode_t* node;
	
	msg.InitRead( compressed, compressedSize );
	msg.SetSize( compressedSize );
	msg.BeginReading();
	for( i = 0; i < textLength; i++ )
	{
		node = huffmanTree;
		do
		{
			bit = msg.ReadBits( 1 );
			node = node->children[bit];
		}
		while( node->symbol == -1 );
		text[i] = node->symbol;
	}
	text[i] = '\0';
	return msg.GetReadCount();
}

/*
================
ListHuffmanFrequencies_f
================
*/
void ListHuffmanFrequencies_f( const idCmdArgs& args )
{
	int		i;
	float compression;
	compression = !totalUncompressedLength ? 100 : 100 * totalCompressedLength / totalUncompressedLength;
	common->Printf( "// compression ratio = %d%%\n", ( int )compression );
	common->Printf( "static int huffmanFrequencies[] = {\n" );
	for( i = 0; i < MAX_HUFFMAN_SYMBOLS; i += 8 )
	{
		common->Printf( "\t0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x,\n",
						huffmanFrequencies[i + 0], huffmanFrequencies[i + 1],
						huffmanFrequencies[i + 2], huffmanFrequencies[i + 3],
						huffmanFrequencies[i + 4], huffmanFrequencies[i + 5],
						huffmanFrequencies[i + 6], huffmanFrequencies[i + 7] );
	}
	common->Printf( "}\n" );
}

void ConvertPDAsToStrings_f( const idCmdArgs& args )
{
	declManagerLocal.ConvertPDAsToStrings( args );
}

/*
====================================================================================

 idDeclFile

====================================================================================
*/

/*
================
idDeclFile::idDeclFile
================
*/
idDeclFile::idDeclFile( const char* fileName, declType_t defaultType )
{
	this->fileName = fileName;
	this->defaultType = defaultType;
	this->timestamp = 0;
	this->checksum = 0;
	this->fileSize = 0;
	this->numLines = 0;
	this->decls = NULL;
}

/*
================
idDeclFile::idDeclFile
================
*/
idDeclFile::idDeclFile()
{
	this->fileName = "<implicit file>";
	this->defaultType = DECL_MAX_TYPES;
	this->timestamp = 0;
	this->checksum = 0;
	this->fileSize = 0;
	this->numLines = 0;
	this->decls = NULL;
}

/*
================
idDeclFile::Reload

ForceReload will cause it to reload even if the timestamp hasn't changed
================
*/
void idDeclFile::Reload( bool force )
{
	// check for an unchanged timestamp
	if( !force && timestamp != 0 )
	{
		ID_TIME_T	testTimeStamp;
		fileSystem->ReadFile( fileName, NULL, &testTimeStamp );
		
		if( testTimeStamp == timestamp )
		{
			return;
		}
	}
	
	// parse the text
	LoadAndParse();
}

/*
================
idDeclFile::LoadAndParse

This is used during both the initial load, and any reloads
================
*/
int c_savedMemory = 0;

int idDeclFile::LoadAndParse()
{
	int			i, numTypes;
	idLexer		src;
	idToken		token;
	int			startMarker;
	char* 		buffer;
	int			length, size;
	int			sourceLine;
	idStr		name;
	idDeclLocal* newDecl;
	bool		reparse;
	
	// load the text
	common->DPrintf( "...loading '%s'\n", fileName.c_str() );
	length = fileSystem->ReadFile( fileName, ( void** )&buffer, &timestamp );
	if( length == -1 )
	{
		common->FatalError( "couldn't load %s", fileName.c_str() );
		return 0;
	}
	
	if( !src.LoadMemory( buffer, length, fileName ) )
	{
		common->Error( "Couldn't parse %s", fileName.c_str() );
		Mem_Free( buffer );
		return 0;
	}
	
	// mark all the defs that were from the last reload of this file
	for( idDeclLocal* decl = decls; decl; decl = decl->nextInFile )
	{
		decl->redefinedInReload = false;
	}
	
	src.SetFlags( DECL_LEXER_FLAGS );
	
	checksum = MD5_BlockChecksum( buffer, length );
	
	fileSize = length;
	
	// scan through, identifying each individual declaration
	while( 1 )
	{
	
		startMarker = src.GetFileOffset();
		sourceLine = src.GetLineNum();
		
		// parse the decl type name
		if( !src.ReadToken( &token ) )
		{
			break;
		}
		
		declType_t identifiedType = DECL_MAX_TYPES;
		
		// get the decl type from the type name
		numTypes = declManagerLocal.GetNumDeclTypes();
		for( i = 0; i < numTypes; i++ )
		{
			idDeclType* typeInfo = declManagerLocal.GetDeclType( i );
			if( typeInfo != NULL && typeInfo->typeName.Icmp( token ) == 0 )
			{
				identifiedType = ( declType_t ) typeInfo->type;
				break;
			}
		}
		
		if( i >= numTypes )
		{
		
			if( token.Icmp( "{" ) == 0 )
			{
			
				// if we ever see an open brace, we somehow missed the [type] <name> prefix
				src.Warning( "Missing decl name" );
				src.SkipBracedSection( false );
				continue;
				
			}
			else
			{
			
				if( defaultType == DECL_MAX_TYPES )
				{
					src.Warning( "No type" );
					continue;
				}
				src.UnreadToken( &token );
				// use the default type
				identifiedType = defaultType;
			}
		}
		
		// now parse the name
		if( !src.ReadToken( &token ) )
		{
			src.Warning( "Type without definition at end of file" );
			break;
		}
		
		if( !token.Icmp( "{" ) )
		{
			// if we ever see an open brace, we somehow missed the [type] <name> prefix
			src.Warning( "Missing decl name" );
			src.SkipBracedSection( false );
			continue;
		}
		
		// FIXME: export decls are only used by the model exporter, they are skipped here for now
		if( identifiedType == DECL_MODELEXPORT )
		{
			src.SkipBracedSection();
			continue;
		}
		
		name = token;
		
		// make sure there's a '{'
		if( !src.ReadToken( &token ) )
		{
			src.Warning( "Type without definition at end of file" );
			break;
		}
		if( token != "{" )
		{
			src.Warning( "Expecting '{' but found '%s'", token.c_str() );
			continue;
		}
		src.UnreadToken( &token );
		
		// now take everything until a matched closing brace
		src.SkipBracedSection();
		size = src.GetFileOffset() - startMarker;
		
		// look it up, possibly getting a newly created default decl
		reparse = false;
		newDecl = declManagerLocal.FindTypeWithoutParsing( identifiedType, name, false );
		if( newDecl )
		{
			// update the existing copy
			if( newDecl->sourceFile != this || newDecl->redefinedInReload )
			{
				src.Warning( "%s '%s' previously defined at %s:%i", declManagerLocal.GetDeclNameFromType( identifiedType ),
							 name.c_str(), newDecl->sourceFile->fileName.c_str(), newDecl->sourceLine );
				continue;
			}
			if( newDecl->declState != DS_UNPARSED )
			{
				reparse = true;
			}
		}
		else
		{
			// allow it to be created as a default, then add it to the per-file list
			newDecl = declManagerLocal.FindTypeWithoutParsing( identifiedType, name, true );
			newDecl->nextInFile = this->decls;
			this->decls = newDecl;
		}
		
		newDecl->redefinedInReload = true;
		
		if( newDecl->textSource )
		{
			Mem_Free( newDecl->textSource );
			newDecl->textSource = NULL;
		}
		
		newDecl->SetTextLocal( buffer + startMarker, size );
		newDecl->sourceFile = this;
		newDecl->sourceTextOffset = startMarker;
		newDecl->sourceTextLength = size;
		newDecl->sourceLine = sourceLine;
		newDecl->declState = DS_UNPARSED;
		
		// if it is currently in use, reparse it immedaitely
		if( reparse )
		{
			newDecl->ParseLocal();
		}
	}
	
	numLines = src.GetLineNum();
	
	Mem_Free( buffer );
	
	// any defs that weren't redefinedInReload should now be defaulted
	for( idDeclLocal* decl = decls ; decl ; decl = decl->nextInFile )
	{
		if( decl->redefinedInReload == false )
		{
			decl->MakeDefault();
			decl->sourceTextOffset = decl->sourceFile->fileSize;
			decl->sourceTextLength = 0;
			decl->sourceLine = decl->sourceFile->numLines;
		}
	}
	
	return checksum;
}

/*
====================================================================================

 idDeclManagerLocal

====================================================================================
*/

const char* listDeclStrings[] = { "current", "all", "ever", NULL };

/*
===================
idDeclManagerLocal::Init
===================
*/
void idDeclManagerLocal::Init()
{

	common->Printf( "----- Initializing Decls -----\n" );
	
	checksum = 0;
	
#ifdef USE_COMPRESSED_DECLS
	SetupHuffman();
#endif
	
#ifdef GET_HUFFMAN_FREQUENCIES
	ClearHuffmanFrequencies();
#endif
	
	// decls used throughout the engine
	RegisterDeclType( "table",				DECL_TABLE,			idDeclAllocator<idDeclTable> );
	RegisterDeclType( "material",			DECL_MATERIAL,		idDeclAllocator<idMaterial> );
	RegisterDeclType( "skin",				DECL_SKIN,			idDeclAllocator<idDeclSkin> );
	RegisterDeclType( "sound",				DECL_SOUND,			idDeclAllocator<idSoundShader> );
	
	RegisterDeclType( "entityDef",			DECL_ENTITYDEF,		idDeclAllocator<idDeclEntityDef> );
	RegisterDeclType( "mapDef",				DECL_MAPDEF,		idDeclAllocator<idDeclEntityDef> );
	RegisterDeclType( "fx",					DECL_FX,			idDeclAllocator<idDeclFX> );
	RegisterDeclType( "particle",			DECL_PARTICLE,		idDeclAllocator<idDeclParticle> );
	RegisterDeclType( "articulatedFigure",	DECL_AF,			idDeclAllocator<idDeclAF> );
	RegisterDeclType( "pda",				DECL_PDA,			idDeclAllocator<idDeclPDA> );
	RegisterDeclType( "email",				DECL_EMAIL,			idDeclAllocator<idDeclEmail> );
	RegisterDeclType( "video",				DECL_VIDEO,			idDeclAllocator<idDeclVideo> );
	RegisterDeclType( "audio",				DECL_AUDIO,			idDeclAllocator<idDeclAudio> );
	
	RegisterDeclFolder( "materials",		".mtr",				DECL_MATERIAL );
	
	// add console commands
	cmdSystem->AddCommand( "listDecls", ListDecls_f, CMD_FL_SYSTEM, "lists all decls" );
	
	cmdSystem->AddCommand( "reloadDecls", ReloadDecls_f, CMD_FL_SYSTEM, "reloads decls" );
	cmdSystem->AddCommand( "touch", TouchDecl_f, CMD_FL_SYSTEM, "touches a decl" );
	
	cmdSystem->AddCommand( "listTables", idListDecls_f<DECL_TABLE>, CMD_FL_SYSTEM, "lists tables", idCmdSystem::ArgCompletion_String<listDeclStrings> );
	cmdSystem->AddCommand( "listMaterials", idListDecls_f<DECL_MATERIAL>, CMD_FL_SYSTEM, "lists materials", idCmdSystem::ArgCompletion_String<listDeclStrings> );
	cmdSystem->AddCommand( "listSkins", idListDecls_f<DECL_SKIN>, CMD_FL_SYSTEM, "lists skins", idCmdSystem::ArgCompletion_String<listDeclStrings> );
	cmdSystem->AddCommand( "listSoundShaders", idListDecls_f<DECL_SOUND>, CMD_FL_SYSTEM, "lists sound shaders", idCmdSystem::ArgCompletion_String<listDeclStrings> );
	
	cmdSystem->AddCommand( "listEntityDefs", idListDecls_f<DECL_ENTITYDEF>, CMD_FL_SYSTEM, "lists entity defs", idCmdSystem::ArgCompletion_String<listDeclStrings> );
	cmdSystem->AddCommand( "listFX", idListDecls_f<DECL_FX>, CMD_FL_SYSTEM, "lists FX systems", idCmdSystem::ArgCompletion_String<listDeclStrings> );
	cmdSystem->AddCommand( "listParticles", idListDecls_f<DECL_PARTICLE>, CMD_FL_SYSTEM, "lists particle systems", idCmdSystem::ArgCompletion_String<listDeclStrings> );
	cmdSystem->AddCommand( "listAF", idListDecls_f<DECL_AF>, CMD_FL_SYSTEM, "lists articulated figures", idCmdSystem::ArgCompletion_String<listDeclStrings> );
	
	cmdSystem->AddCommand( "listPDAs", idListDecls_f<DECL_PDA>, CMD_FL_SYSTEM, "lists PDAs", idCmdSystem::ArgCompletion_String<listDeclStrings> );
	cmdSystem->AddCommand( "listEmails", idListDecls_f<DECL_EMAIL>, CMD_FL_SYSTEM, "lists Emails", idCmdSystem::ArgCompletion_String<listDeclStrings> );
	cmdSystem->AddCommand( "listVideos", idListDecls_f<DECL_VIDEO>, CMD_FL_SYSTEM, "lists Videos", idCmdSystem::ArgCompletion_String<listDeclStrings> );
	cmdSystem->AddCommand( "listAudios", idListDecls_f<DECL_AUDIO>, CMD_FL_SYSTEM, "lists Audios", idCmdSystem::ArgCompletion_String<listDeclStrings> );
	
	cmdSystem->AddCommand( "printTable", idPrintDecls_f<DECL_TABLE>, CMD_FL_SYSTEM, "prints a table", idCmdSystem::ArgCompletion_Decl<DECL_TABLE> );
	cmdSystem->AddCommand( "printMaterial", idPrintDecls_f<DECL_MATERIAL>, CMD_FL_SYSTEM, "prints a material", idCmdSystem::ArgCompletion_Decl<DECL_MATERIAL> );
	cmdSystem->AddCommand( "printSkin", idPrintDecls_f<DECL_SKIN>, CMD_FL_SYSTEM, "prints a skin", idCmdSystem::ArgCompletion_Decl<DECL_SKIN> );
	cmdSystem->AddCommand( "printSoundShader", idPrintDecls_f<DECL_SOUND>, CMD_FL_SYSTEM, "prints a sound shader", idCmdSystem::ArgCompletion_Decl<DECL_SOUND> );
	
	cmdSystem->AddCommand( "printEntityDef", idPrintDecls_f<DECL_ENTITYDEF>, CMD_FL_SYSTEM, "prints an entity def", idCmdSystem::ArgCompletion_Decl<DECL_ENTITYDEF> );
	cmdSystem->AddCommand( "printFX", idPrintDecls_f<DECL_FX>, CMD_FL_SYSTEM, "prints an FX system", idCmdSystem::ArgCompletion_Decl<DECL_FX> );
	cmdSystem->AddCommand( "printParticle", idPrintDecls_f<DECL_PARTICLE>, CMD_FL_SYSTEM, "prints a particle system", idCmdSystem::ArgCompletion_Decl<DECL_PARTICLE> );
	cmdSystem->AddCommand( "printAF", idPrintDecls_f<DECL_AF>, CMD_FL_SYSTEM, "prints an articulated figure", idCmdSystem::ArgCompletion_Decl<DECL_AF> );
	
	cmdSystem->AddCommand( "printPDA", idPrintDecls_f<DECL_PDA>, CMD_FL_SYSTEM, "prints an PDA", idCmdSystem::ArgCompletion_Decl<DECL_PDA> );
	cmdSystem->AddCommand( "printEmail", idPrintDecls_f<DECL_EMAIL>, CMD_FL_SYSTEM, "prints an Email", idCmdSystem::ArgCompletion_Decl<DECL_EMAIL> );
	cmdSystem->AddCommand( "printVideo", idPrintDecls_f<DECL_VIDEO>, CMD_FL_SYSTEM, "prints an Audio", idCmdSystem::ArgCompletion_Decl<DECL_VIDEO> );
	cmdSystem->AddCommand( "printAudio", idPrintDecls_f<DECL_AUDIO>, CMD_FL_SYSTEM, "prints a Video", idCmdSystem::ArgCompletion_Decl<DECL_AUDIO> );
	
	cmdSystem->AddCommand( "listHuffmanFrequencies", ListHuffmanFrequencies_f, CMD_FL_SYSTEM, "lists decl text character frequencies" );
	
	cmdSystem->AddCommand( "convertPDAsToStrings", ConvertPDAsToStrings_f, CMD_FL_SYSTEM, "Converts *.pda files to text which can be plugged into *.lang files." );
	
	common->Printf( "------------------------------\n" );
}

void idDeclManagerLocal::Init2()
{
	RegisterDeclFolder( "skins",			".skin",			DECL_SKIN );
	RegisterDeclFolder( "sound",			".sndshd",			DECL_SOUND );
}

/*
===================
idDeclManagerLocal::Shutdown
===================
*/
void idDeclManagerLocal::Shutdown()
{
	int			i, j;
	idDeclLocal* decl;
	
	// free decls
	for( i = 0; i < DECL_MAX_TYPES; i++ )
	{
		for( j = 0; j < linearLists[i].Num(); j++ )
		{
			decl = linearLists[i][j];
			if( decl->self != NULL )
			{
				decl->self->FreeData();
				delete decl->self;
			}
			if( decl->textSource )
			{
				Mem_Free( decl->textSource );
				decl->textSource = NULL;
			}
			delete decl;
		}
		linearLists[i].Clear();
		hashTables[i].Free();
	}
	
	// free decl files
	loadedFiles.DeleteContents( true );
	
	// free the decl types and folders
	declTypes.DeleteContents( true );
	declFolders.DeleteContents( true );
	
#ifdef USE_COMPRESSED_DECLS
	ShutdownHuffman();
#endif
}

/*
===================
idDeclManagerLocal::Reload
===================
*/
void idDeclManagerLocal::Reload( bool force )
{
	for( int i = 0; i < loadedFiles.Num(); i++ )
	{
		loadedFiles[i]->Reload( force );
	}
}

/*
===================
idDeclManagerLocal::BeginLevelLoad
===================
*/
void idDeclManagerLocal::BeginLevelLoad()
{
	insideLevelLoad = true;
	
	// clear all the referencedThisLevel flags and purge all the data
	// so the next reference will cause a reparse
	for( int i = 0; i < DECL_MAX_TYPES; i++ )
	{
		int	num = linearLists[i].Num();
		for( int j = 0 ; j < num ; j++ )
		{
			idDeclLocal* decl = linearLists[i][j];
			decl->Purge();
		}
	}
}

/*
===================
idDeclManagerLocal::EndLevelLoad
===================
*/
void idDeclManagerLocal::EndLevelLoad()
{
	insideLevelLoad = false;
	
	// we don't need to do anything here, but the image manager, model manager,
	// and sound sample manager will need to free media that was not referenced
}

/*
===================
idDeclManagerLocal::RegisterDeclType
===================
*/
void idDeclManagerLocal::RegisterDeclType( const char* typeName, declType_t type, idDecl * ( *allocator )() )
{
	idDeclType* declType;
	
	if( type < declTypes.Num() && declTypes[( int )type] )
	{
		common->Warning( "idDeclManager::RegisterDeclType: type '%s' already exists", typeName );
		return;
	}
	
	declType = new( TAG_DECL ) idDeclType;
	declType->typeName = typeName;
	declType->type = type;
	declType->allocator = allocator;
	
	if( ( int )type + 1 > declTypes.Num() )
	{
		declTypes.AssureSize( ( int )type + 1, NULL );
	}
	declTypes[type] = declType;
}

/*
===================
idDeclManagerLocal::RegisterDeclFolder
===================
*/
void idDeclManagerLocal::RegisterDeclFolder( const char* folder, const char* extension, declType_t defaultType )
{
	int i, j;
	idStr fileName;
	idDeclFolder* declFolder;
	idFileList* fileList;
	idDeclFile* df;
	
	// check whether this folder / extension combination already exists
	for( i = 0; i < declFolders.Num(); i++ )
	{
		if( declFolders[i]->folder.Icmp( folder ) == 0 && declFolders[i]->extension.Icmp( extension ) == 0 )
		{
			break;
		}
	}
	if( i < declFolders.Num() )
	{
		declFolder = declFolders[i];
	}
	else
	{
		declFolder = new( TAG_DECL ) idDeclFolder;
		declFolder->folder = folder;
		declFolder->extension = extension;
		declFolder->defaultType = defaultType;
		declFolders.Append( declFolder );
	}
	
	// scan for decl files
	fileList = fileSystem->ListFiles( declFolder->folder, declFolder->extension, true );
	
	// load and parse decl files
	for( i = 0; i < fileList->GetNumFiles(); i++ )
	{
		fileName = declFolder->folder + "/" + fileList->GetFile( i );
		
		// check whether this file has already been loaded
		for( j = 0; j < loadedFiles.Num(); j++ )
		{
			if( fileName.Icmp( loadedFiles[j]->fileName ) == 0 )
			{
				break;
			}
		}
		if( j < loadedFiles.Num() )
		{
			df = loadedFiles[j];
		}
		else
		{
			df = new( TAG_DECL ) idDeclFile( fileName, defaultType );
			loadedFiles.Append( df );
		}
		df->LoadAndParse();
	}
	
	fileSystem->FreeFileList( fileList );
}

/*
===================
idDeclManagerLocal::GetChecksum
===================
*/
int idDeclManagerLocal::GetChecksum() const
{
	int i, j, total, num;
	int* checksumData;
	
	// get the total number of decls
	total = 0;
	for( i = 0; i < DECL_MAX_TYPES; i++ )
	{
		total += linearLists[i].Num();
	}
	
	checksumData = ( int* ) _alloca16( total * 2 * sizeof( int ) );
	
	total = 0;
	for( i = 0; i < DECL_MAX_TYPES; i++ )
	{
		declType_t type = ( declType_t ) i;
		
		// FIXME: not particularly pretty but PDAs and associated decls are localized and should not be checksummed
		if( type == DECL_PDA || type == DECL_VIDEO || type == DECL_AUDIO || type == DECL_EMAIL )
		{
			continue;
		}
		
		num = linearLists[i].Num();
		for( j = 0; j < num; j++ )
		{
			idDeclLocal* decl = linearLists[i][j];
			
			if( decl->sourceFile == &implicitDecls )
			{
				continue;
			}
			
			checksumData[total * 2 + 0] = total;
			checksumData[total * 2 + 1] = decl->checksum;
			total++;
		}
	}
	
	LittleRevBytes( checksumData, sizeof( int ), total * 2 );
	return MD5_BlockChecksum( checksumData, total * 2 * sizeof( int ) );
}

/*
===================
idDeclManagerLocal::GetNumDeclTypes
===================
*/
int idDeclManagerLocal::GetNumDeclTypes() const
{
	return declTypes.Num();
}

/*
===================
idDeclManagerLocal::GetDeclNameFromType
===================
*/
const char* idDeclManagerLocal::GetDeclNameFromType( declType_t type ) const
{
	int typeIndex = ( int )type;
	
	if( typeIndex < 0 || typeIndex >= declTypes.Num() || declTypes[typeIndex] == NULL )
	{
		common->FatalError( "idDeclManager::GetDeclNameFromType: bad type: %i", typeIndex );
	}
	return declTypes[typeIndex]->typeName;
}

/*
===================
idDeclManagerLocal::GetDeclTypeFromName
===================
*/
declType_t idDeclManagerLocal::GetDeclTypeFromName( const char* typeName ) const
{
	int i;
	
	for( i = 0; i < declTypes.Num(); i++ )
	{
		if( declTypes[i] && declTypes[i]->typeName.Icmp( typeName ) == 0 )
		{
			return ( declType_t )declTypes[i]->type;
		}
	}
	return DECL_MAX_TYPES;
}

/*
=================
idDeclManagerLocal::FindType

External users will always cause the decl to be parsed before returning
=================
*/
const idDecl* idDeclManagerLocal::FindType( declType_t type, const char* name, bool makeDefault )
{
	idDeclLocal* decl;
	
	idScopedCriticalSection cs( mutex );
	
	if( !name || !name[0] )
	{
		name = "_emptyName";
		//common->Warning( "idDeclManager::FindType: empty %s name", GetDeclType( (int)type )->typeName.c_str() );
	}
	
	decl = FindTypeWithoutParsing( type, name, makeDefault );
	if( !decl )
	{
		return NULL;
	}
	
	decl->AllocateSelf();
	
	// if it hasn't been parsed yet, parse it now
	if( decl->declState == DS_UNPARSED )
	{
		if( !idLib::IsMainThread() )
		{
			// we can't load images from a background thread on OpenGL,
			// the renderer on the main thread should parse it if needed
			idLib::Error( "Attempted to load %s decl '%s' from game thread!", GetDeclNameFromType( type ), name );
		}
		decl->ParseLocal();
	}
	
	// mark it as referenced
	decl->referencedThisLevel = true;
	decl->everReferenced = true;
	if( insideLevelLoad )
	{
		decl->parsedOutsideLevelLoad = false;
	}
	
	return decl->self;
}

/*
===============
idDeclManagerLocal::FindDeclWithoutParsing
===============
*/
const idDecl* idDeclManagerLocal::FindDeclWithoutParsing( declType_t type, const char* name, bool makeDefault )
{
	idDeclLocal* decl;
	decl = FindTypeWithoutParsing( type, name, makeDefault );
	if( decl )
	{
		return decl->self;
	}
	return NULL;
}

/*
===============
idDeclManagerLocal::ReloadFile
===============
*/
void idDeclManagerLocal::ReloadFile( const char* filename, bool force )
{
	for( int i = 0; i < loadedFiles.Num(); i++ )
	{
		if( !loadedFiles[i]->fileName.Icmp( filename ) )
		{
			checksum ^= loadedFiles[i]->checksum;
			loadedFiles[i]->Reload( force );
			checksum ^= loadedFiles[i]->checksum;
		}
	}
}

/*
===================
idDeclManagerLocal::GetNumDecls
===================
*/
int idDeclManagerLocal::GetNumDecls( declType_t type )
{
	int typeIndex = ( int )type;
	
	if( typeIndex < 0 || typeIndex >= declTypes.Num() || declTypes[typeIndex] == NULL )
	{
		common->FatalError( "idDeclManager::GetNumDecls: bad type: %i", typeIndex );
		return 0;
	}
	return linearLists[ typeIndex ].Num();
}

/*
===================
idDeclManagerLocal::DeclByIndex
===================
*/
const idDecl* idDeclManagerLocal::DeclByIndex( declType_t type, int index, bool forceParse )
{
	int typeIndex = ( int )type;
	
	if( typeIndex < 0 || typeIndex >= declTypes.Num() || declTypes[typeIndex] == NULL )
	{
		common->FatalError( "idDeclManager::DeclByIndex: bad type: %i", typeIndex );
		return NULL;
	}
	if( index < 0 || index >= linearLists[ typeIndex ].Num() )
	{
		common->Error( "idDeclManager::DeclByIndex: out of range" );
	}
	idDeclLocal* decl = linearLists[ typeIndex ][ index ];
	
	decl->AllocateSelf();
	
	if( forceParse && decl->declState == DS_UNPARSED )
	{
		decl->ParseLocal();
	}
	
	return decl->self;
}

/*
===================
idDeclManagerLocal::ListType

list*
Lists decls currently referenced

list* ever
Lists decls that have been referenced at least once since app launched

list* all
Lists every decl declared, even if it hasn't been referenced or parsed

FIXME: alphabetized, wildcards?
===================
*/
void idDeclManagerLocal::ListType( const idCmdArgs& args, declType_t type )
{
	bool all, ever;
	
	if( !idStr::Icmp( args.Argv( 1 ), "all" ) )
	{
		all = true;
	}
	else
	{
		all = false;
	}
	if( !idStr::Icmp( args.Argv( 1 ), "ever" ) )
	{
		ever = true;
	}
	else
	{
		ever = false;
	}
	
	common->Printf( "--------------------\n" );
	int printed = 0;
	int	count = linearLists[( int )type ].Num();
	for( int i = 0 ; i < count ; i++ )
	{
		idDeclLocal* decl = linearLists[( int )type ][ i ];
		
		if( !all && decl->declState == DS_UNPARSED )
		{
			continue;
		}
		
		if( !all && !ever && !decl->referencedThisLevel )
		{
			continue;
		}
		
		if( decl->referencedThisLevel )
		{
			common->Printf( "*" );
		}
		else if( decl->everReferenced )
		{
			common->Printf( "." );
		}
		else
		{
			common->Printf( " " );
		}
		if( decl->declState == DS_DEFAULTED )
		{
			common->Printf( "D" );
		}
		else
		{
			common->Printf( " " );
		}
		common->Printf( "%4i: ", decl->index );
		printed++;
		if( decl->declState == DS_UNPARSED )
		{
			// doesn't have any type specific data yet
			common->Printf( "%s\n", decl->GetName() );
		}
		else
		{
			decl->self->List();
		}
	}
	
	common->Printf( "--------------------\n" );
	common->Printf( "%i of %i %s\n", printed, count, declTypes[type]->typeName.c_str() );
}

/*
===================
idDeclManagerLocal::PrintType
===================
*/
void idDeclManagerLocal::PrintType( const idCmdArgs& args, declType_t type )
{
	// individual decl types may use additional command parameters
	if( args.Argc() < 2 )
	{
		common->Printf( "USAGE: Print<decl type> <decl name> [type specific parms]\n" );
		return;
	}
	
	// look it up, skipping the public path so it won't parse or reference
	idDeclLocal* decl = FindTypeWithoutParsing( type, args.Argv( 1 ), false );
	if( !decl )
	{
		common->Printf( "%s '%s' not found.\n", declTypes[ type ]->typeName.c_str(), args.Argv( 1 ) );
		return;
	}
	
	// print information common to all decls
	common->Printf( "%s %s:\n", declTypes[ type ]->typeName.c_str(), decl->name.c_str() );
	common->Printf( "source: %s:%i\n", decl->sourceFile->fileName.c_str(), decl->sourceLine );
	common->Printf( "----------\n" );
	if( decl->textSource != NULL )
	{
		char* declText = ( char* )_alloca( decl->textLength + 1 );
		decl->GetText( declText );
		common->Printf( "%s\n", declText );
	}
	else
	{
		common->Printf( "NO SOURCE\n" );
	}
	common->Printf( "----------\n" );
	switch( decl->declState )
	{
		case DS_UNPARSED:
			common->Printf( "Unparsed.\n" );
			break;
		case DS_DEFAULTED:
			common->Printf( "<DEFAULTED>\n" );
			break;
		case DS_PARSED:
			common->Printf( "Parsed.\n" );
			break;
	}
	
	if( decl->referencedThisLevel )
	{
		common->Printf( "Currently referenced this level.\n" );
	}
	else if( decl->everReferenced )
	{
		common->Printf( "Referenced in a previous level.\n" );
	}
	else
	{
		common->Printf( "Never referenced.\n" );
	}
	
	// allow type-specific data to be printed
	if( decl->self != NULL )
	{
		decl->self->Print();
	}
}

/*
===================
idDeclManagerLocal::CreateNewDecl
===================
*/
idDecl* idDeclManagerLocal::CreateNewDecl( declType_t type, const char* name, const char* _fileName )
{
	int typeIndex = ( int )type;
	int i, hash;
	
	if( typeIndex < 0 || typeIndex >= declTypes.Num() || declTypes[typeIndex] == NULL || typeIndex >= DECL_MAX_TYPES )
	{
		common->FatalError( "idDeclManager::CreateNewDecl: bad type: %i", typeIndex );
		return NULL;
	}
	
	char  canonicalName[MAX_STRING_CHARS];
	
	MakeNameCanonical( name, canonicalName, sizeof( canonicalName ) );
	
	idStr fileName = _fileName;
	fileName.BackSlashesToSlashes();
	
	// see if it already exists
	hash = hashTables[typeIndex].GenerateKey( canonicalName, false );
	for( i = hashTables[typeIndex].First( hash ); i >= 0; i = hashTables[typeIndex].Next( i ) )
	{
		if( linearLists[typeIndex][i]->name.Icmp( canonicalName ) == 0 )
		{
			linearLists[typeIndex][i]->AllocateSelf();
			return linearLists[typeIndex][i]->self;
		}
	}
	
	idDeclFile* sourceFile;
	
	// find existing source file or create a new one
	for( i = 0; i < loadedFiles.Num(); i++ )
	{
		if( loadedFiles[i]->fileName.Icmp( fileName ) == 0 )
		{
			break;
		}
	}
	if( i < loadedFiles.Num() )
	{
		sourceFile = loadedFiles[i];
	}
	else
	{
		sourceFile = new( TAG_DECL ) idDeclFile( fileName, type );
		loadedFiles.Append( sourceFile );
	}
	
	idDeclLocal* decl = new( TAG_DECL ) idDeclLocal;
	decl->name = canonicalName;
	decl->type = type;
	decl->declState = DS_UNPARSED;
	decl->AllocateSelf();
	idStr header = declTypes[typeIndex]->typeName;
	idStr defaultText = decl->self->DefaultDefinition();
	
	
	int size = header.Length() + 1 + idStr::Length( canonicalName ) + 1 + defaultText.Length();
	char* declText = ( char* ) _alloca( size + 1 );
	
	memcpy( declText, header, header.Length() );
	declText[header.Length()] = ' ';
	memcpy( declText + header.Length() + 1, canonicalName, idStr::Length( canonicalName ) );
	declText[header.Length() + 1 + idStr::Length( canonicalName )] = ' ';
	memcpy( declText + header.Length() + 1 + idStr::Length( canonicalName ) + 1, defaultText, defaultText.Length() + 1 );
	
	decl->SetTextLocal( declText, size );
	decl->sourceFile = sourceFile;
	decl->sourceTextOffset = sourceFile->fileSize;
	decl->sourceTextLength = 0;
	decl->sourceLine = sourceFile->numLines;
	
	decl->ParseLocal();
	
	// add this decl to the source file list
	decl->nextInFile = sourceFile->decls;
	sourceFile->decls = decl;
	
	// add it to the hash table and linear list
	decl->index = linearLists[typeIndex].Num();
	hashTables[typeIndex].Add( hash, linearLists[typeIndex].Append( decl ) );
	
	return decl->self;
}

/*
===============
idDeclManagerLocal::RenameDecl
===============
*/
bool idDeclManagerLocal::RenameDecl( declType_t type, const char* oldName, const char* newName )
{

	char canonicalOldName[MAX_STRING_CHARS];
	MakeNameCanonical( oldName, canonicalOldName, sizeof( canonicalOldName ) );
	
	char canonicalNewName[MAX_STRING_CHARS];
	MakeNameCanonical( newName, canonicalNewName, sizeof( canonicalNewName ) );
	
	idDeclLocal*	decl = NULL;
	
	// make sure it already exists
	int typeIndex = ( int )type;
	int i, hash;
	hash = hashTables[typeIndex].GenerateKey( canonicalOldName, false );
	for( i = hashTables[typeIndex].First( hash ); i >= 0; i = hashTables[typeIndex].Next( i ) )
	{
		if( linearLists[typeIndex][i]->name.Icmp( canonicalOldName ) == 0 )
		{
			decl = linearLists[typeIndex][i];
			break;
		}
	}
	if( !decl )
		return false;
		
	//if ( !hashTables[(int)type].Get( canonicalOldName, &declPtr ) )
	//	return false;
	
	//decl = *declPtr;
	
	//Change the name
	decl->name = canonicalNewName;
	
	
	// add it to the hash table
	//hashTables[(int)decl->type].Set( decl->name, decl );
	int newhash = hashTables[typeIndex].GenerateKey( canonicalNewName, false );
	hashTables[typeIndex].Add( newhash, decl->index );
	
	//Remove the old hash item
	hashTables[typeIndex].Remove( hash, decl->index );
	
	return true;
}

/*
===================
idDeclManagerLocal::MediaPrint

This is just used to nicely indent media caching prints
===================
*/
void idDeclManagerLocal::MediaPrint( const char* fmt, ... )
{
	if( !decl_show.GetInteger() )
	{
		return;
	}
	for( int i = 0 ; i < indent ; i++ )
	{
		common->Printf( "    " );
	}
	va_list		argptr;
	char		buffer[1024];
	va_start( argptr, fmt );
	idStr::vsnPrintf( buffer, sizeof( buffer ), fmt, argptr );
	va_end( argptr );
	buffer[sizeof( buffer ) - 1] = '\0';
	
	common->Printf( "%s", buffer );
}

/*
===================
idDeclManagerLocal::WritePrecacheCommands
===================
*/
void idDeclManagerLocal::WritePrecacheCommands( idFile* f )
{
	for( int i = 0; i < declTypes.Num(); i++ )
	{
		int num;
		
		if( declTypes[i] == NULL )
		{
			continue;
		}
		
		num = linearLists[i].Num();
		
		for( int j = 0 ; j < num ; j++ )
		{
			idDeclLocal* decl = linearLists[i][j];
			
			if( !decl->referencedThisLevel )
			{
				continue;
			}
			
			char	str[1024];
			sprintf( str, "touch %s %s\n", declTypes[i]->typeName.c_str(), decl->GetName() );
			common->Printf( "%s", str );
			f->Printf( "%s", str );
		}
	}
}

/********************************************************************/

const idMaterial* idDeclManagerLocal::FindMaterial( const char* name, bool makeDefault )
{
	return static_cast<const idMaterial*>( FindType( DECL_MATERIAL, name, makeDefault ) );
}

const idMaterial* idDeclManagerLocal::MaterialByIndex( int index, bool forceParse )
{
	return static_cast<const idMaterial*>( DeclByIndex( DECL_MATERIAL, index, forceParse ) );
}

/********************************************************************/

const idDeclSkin* idDeclManagerLocal::FindSkin( const char* name, bool makeDefault )
{
	return static_cast<const idDeclSkin*>( FindType( DECL_SKIN, name, makeDefault ) );
}

const idDeclSkin* idDeclManagerLocal::SkinByIndex( int index, bool forceParse )
{
	return static_cast<const idDeclSkin*>( DeclByIndex( DECL_SKIN, index, forceParse ) );
}

/********************************************************************/

const idSoundShader* idDeclManagerLocal::FindSound( const char* name, bool makeDefault )
{
	return static_cast<const idSoundShader*>( FindType( DECL_SOUND, name, makeDefault ) );
}

const idSoundShader* idDeclManagerLocal::SoundByIndex( int index, bool forceParse )
{
	return static_cast<const idSoundShader*>( DeclByIndex( DECL_SOUND, index, forceParse ) );
}

/*
===================
idDeclManagerLocal::Touch
===================
*/
void idDeclManagerLocal::Touch( const idDecl* decl )
{

	if( decl->base->GetState() ==  DS_UNPARSED )
	{
		// This should parse the decl as well.
		FindType( decl->GetType(), decl->GetName() );
	}
}

/*
===================
idDeclManagerLocal::MakeNameCanonical
===================
*/
void idDeclManagerLocal::MakeNameCanonical( const char* name, char* result, int maxLength )
{
	int i, lastDot;
	
	lastDot = -1;
	for( i = 0; i < maxLength && name[i] != '\0'; i++ )
	{
		int c = name[i];
		if( c == '\\' )
		{
			result[i] = '/';
		}
		else if( c == '.' )
		{
			lastDot = i;
			result[i] = c;
		}
		else
		{
			result[i] = idStr::ToLower( c );
		}
	}
	if( lastDot != -1 )
	{
		result[lastDot] = '\0';
	}
	else
	{
		result[i] = '\0';
	}
}

/*
================
idDeclManagerLocal::ListDecls_f
================
*/
void idDeclManagerLocal::ListDecls_f( const idCmdArgs& args )
{
	int		i, j;
	int		totalDecls = 0;
	int		totalText = 0;
	int		totalStructs = 0;
	
	for( i = 0; i < declManagerLocal.declTypes.Num(); i++ )
	{
		int size, num;
		
		if( declManagerLocal.declTypes[i] == NULL )
		{
			continue;
		}
		
		num = declManagerLocal.linearLists[i].Num();
		totalDecls += num;
		
		size = 0;
		for( j = 0; j < num; j++ )
		{
			size += declManagerLocal.linearLists[i][j]->Size();
			if( declManagerLocal.linearLists[i][j]->self != NULL )
			{
				size += declManagerLocal.linearLists[i][j]->self->Size();
			}
		}
		totalStructs += size;
		
		common->Printf( "%4ik %4i %s\n", size >> 10, num, declManagerLocal.declTypes[i]->typeName.c_str() );
	}
	
	for( i = 0 ; i < declManagerLocal.loadedFiles.Num() ; i++ )
	{
		idDeclFile*	df = declManagerLocal.loadedFiles[i];
		totalText += df->fileSize;
	}
	
	common->Printf( "%i total decls is %i decl files\n", totalDecls, declManagerLocal.loadedFiles.Num() );
	common->Printf( "%iKB in text, %iKB in structures\n", totalText >> 10, totalStructs >> 10 );
}

/*
===================
idDeclManagerLocal::ReloadDecls_f

Reload will not find any new files created in the directories, it
will only reload existing files.

A reload will never cause anything to be purged.
===================
*/
void idDeclManagerLocal::ReloadDecls_f( const idCmdArgs& args )
{
	bool	force;
	
	if( !idStr::Icmp( args.Argv( 1 ), "all" ) )
	{
		force = true;
		common->Printf( "reloading all decl files:\n" );
	}
	else
	{
		force = false;
		common->Printf( "reloading changed decl files:\n" );
	}
	
	declManagerLocal.Reload( force );
}

/*
===================
idDeclManagerLocal::TouchDecl_f
===================
*/
void idDeclManagerLocal::TouchDecl_f( const idCmdArgs& args )
{
	int	i;
	
	if( args.Argc() != 3 )
	{
		common->Printf( "usage: touch <type> <name>\n" );
		common->Printf( "valid types: " );
		for( int i = 0 ; i < declManagerLocal.declTypes.Num() ; i++ )
		{
			if( declManagerLocal.declTypes[i] )
			{
				common->Printf( "%s ", declManagerLocal.declTypes[i]->typeName.c_str() );
			}
		}
		common->Printf( "\n" );
		return;
	}
	
	for( i = 0; i < declManagerLocal.declTypes.Num(); i++ )
	{
		if( declManagerLocal.declTypes[i] && declManagerLocal.declTypes[i]->typeName.Icmp( args.Argv( 1 ) ) == 0 )
		{
			break;
		}
	}
	if( i >= declManagerLocal.declTypes.Num() )
	{
		common->Printf( "unknown decl type '%s'\n", args.Argv( 1 ) );
		return;
	}
	
	const idDecl* decl = declManagerLocal.FindType( ( declType_t )i, args.Argv( 2 ), false );
	if( !decl )
	{
		common->Printf( "%s '%s' not found\n", declManagerLocal.declTypes[i]->typeName.c_str(), args.Argv( 2 ) );
	}
}

/*
===================
idDeclManagerLocal::FindTypeWithoutParsing

This finds or creats the decl, but does not cause a parse.  This is only used internally.
===================
*/
idDeclLocal* idDeclManagerLocal::FindTypeWithoutParsing( declType_t type, const char* name, bool makeDefault )
{
	int typeIndex = ( int )type;
	int i, hash;
	
	if( typeIndex < 0 || typeIndex >= declTypes.Num() || declTypes[typeIndex] == NULL || typeIndex >= DECL_MAX_TYPES )
	{
		common->FatalError( "idDeclManager::FindTypeWithoutParsing: bad type: %i", typeIndex );
		return NULL;
	}
	
	char canonicalName[MAX_STRING_CHARS];
	
	MakeNameCanonical( name, canonicalName, sizeof( canonicalName ) );
	
	// see if it already exists
	hash = hashTables[typeIndex].GenerateKey( canonicalName, false );
	for( i = hashTables[typeIndex].First( hash ); i >= 0; i = hashTables[typeIndex].Next( i ) )
	{
		if( linearLists[typeIndex][i]->name.Icmp( canonicalName ) == 0 )
		{
			// only print these when decl_show is set to 2, because it can be a lot of clutter
			if( decl_show.GetInteger() > 1 )
			{
				MediaPrint( "referencing %s %s\n", declTypes[ type ]->typeName.c_str(), name );
			}
			return linearLists[typeIndex][i];
		}
	}
	
	if( !makeDefault )
	{
		return NULL;
	}
	
	idDeclLocal* decl = new( TAG_DECL ) idDeclLocal;
	decl->self = NULL;
	decl->name = canonicalName;
	decl->type = type;
	decl->declState = DS_UNPARSED;
	decl->textSource = NULL;
	decl->textLength = 0;
	decl->sourceFile = &implicitDecls;
	decl->referencedThisLevel = false;
	decl->everReferenced = false;
	decl->parsedOutsideLevelLoad = !insideLevelLoad;
	
	// add it to the linear list and hash table
	decl->index = linearLists[typeIndex].Num();
	hashTables[typeIndex].Add( hash, linearLists[typeIndex].Append( decl ) );
	
	return decl;
}

/*
=================
idDeclManagerLocal::ConvertPDAsToStrings
=================
*/
void idDeclManagerLocal::ConvertPDAsToStrings( const idCmdArgs& args )
{

	idStr pdaStringsFileName = "temppdas/pdas.lang";
	idFileLocal file( fileSystem->OpenFileWrite( pdaStringsFileName ) );
	
	if( file == NULL )
	{
		idLib::Printf( "Failed to Convert PDA data to Strings.\n" );
	}
	
	int totalEmailCount = 0;
	int totalAudioCount = 0;
	int totalVideoCount = 0;
	idStr headEnd = "\t\"#str_%s_";
	idStr tailEnd = "\"\t\"%s\"\n";
	idStr temp;
	
	int count = linearLists[ DECL_PDA ].Num();
	for( int i = 0; i < count; i++ )
	{
		const idDeclPDA* decl = static_cast< const idDeclPDA* >( FindType( DECL_PDA, linearLists[ DECL_PDA ][ i ]->GetName(), false ) );
		
		idStr pdaBaseStrId = va( headEnd.c_str(), decl->GetName() );
		
		temp = va( "\n\n//////// %s PDA ////////////\n", decl->GetName() );
		file->Write( temp, temp.Length() );
		
		idStr pdaBase = pdaBaseStrId + "pda_%s" + tailEnd;
		// Pda Name
		temp = va( pdaBase.c_str(), "name", decl->GetPdaName() );
		file->Write( temp, temp.Length() );
		// Full Name
		temp = va( pdaBase.c_str(), "fullname", decl->GetFullName() );
		file->Write( temp, temp.Length() );
		// ID
		temp = va( pdaBase.c_str(), "id", decl->GetID() );
		file->Write( temp, temp.Length() );
		// Post
		temp = va( pdaBase.c_str(), "post", decl->GetPost() );
		file->Write( temp, temp.Length() );
		// Title
		temp = va( pdaBase.c_str(), "title", decl->GetTitle() );
		file->Write( temp, temp.Length() );
		// Security
		temp = va( pdaBase.c_str(), "security", decl->GetSecurity() );
		file->Write( temp, temp.Length() );
		
		int emailCount = decl->GetNumEmails();
		for( int emailIter = 0; emailIter < emailCount; emailIter++ )
		{
			const idDeclEmail* email = decl->GetEmailByIndex( emailIter );
			
			idStr emailBaseStrId = va( headEnd.c_str(), email->GetName() );
			idStr emailBase = emailBaseStrId + "email_%s" + tailEnd;
			
			file->Write( "\t//Email\n", 9 );
			// Date
			temp = va( emailBase, "date", email->GetDate() );
			file->Write( temp, temp.Length() );
			// To
			temp = va( emailBase, "to", email->GetTo() );
			file->Write( temp, temp.Length() );
			// From
			temp = va( emailBase, "from", email->GetFrom() );
			file->Write( temp, temp.Length() );
			// Subject
			temp = va( emailBase, "subject", email->GetSubject() );
			file->Write( temp, temp.Length() );
			// Body
			idStr body = email->GetBody();
			body.Replace( "\n", "\\n" );
			temp = va( emailBase, "text", body.c_str() );
			file->Write( temp, temp.Length() );
			
			totalEmailCount++;
		}
		
		int audioCount = decl->GetNumAudios();
		for( int audioIter = 0; audioIter < audioCount; audioIter++ )
		{
			const idDeclAudio* audio = decl->GetAudioByIndex( audioIter );
			
			idStr audioBaseStrId = va( headEnd.c_str(), audio->GetName() );
			idStr audioBase = audioBaseStrId + "audio_%s" + tailEnd;
			
			file->Write( "\t//Audio\n", 9 );
			// Name
			temp = va( audioBase, "name", audio->GetAudioName() );
			file->Write( temp, temp.Length() );
			// Info
			idStr info = audio->GetInfo();
			info.Replace( "\n", "\\n" );
			temp = va( audioBase, "info", info.c_str() );
			file->Write( temp, temp.Length() );
			
			totalAudioCount++;
		}
	}
	
	int infoEmailCount = linearLists[ DECL_EMAIL ].Num();
	if( infoEmailCount > 0 )
	{
		temp = "\n\n//////// PDA Info Emails ////////////\n";
		file->Write( temp, temp.Length() );
	}
	for( int i = 0; i < infoEmailCount; i++ )
	{
		const idDeclEmail* email = static_cast< const idDeclEmail* >( FindType( DECL_EMAIL, linearLists[ DECL_EMAIL ][ i ]->GetName(), false ) );
		
		idStr filename = email->base->GetFileName();
		if( filename.Icmp( "newpdas/info_emails.pda" ) != 0 )
		{
			continue;
		}
		
		idStr emailBaseStrId = va( "\t\"#str_%s_", email->GetName() );
		idStr emailBase = emailBaseStrId + "email_%s" + tailEnd;
		
		file->Write( "\t//Email\n", 9 );
		
		// Date
		temp = va( emailBase, "date", email->GetDate() );
		file->Write( temp, temp.Length() );
		// To
		temp = va( emailBase, "to", email->GetTo() );
		file->Write( temp, temp.Length() );
		// From
		temp = va( emailBase, "from", email->GetFrom() );
		file->Write( temp, temp.Length() );
		// Subject
		temp = va( emailBase, "subject", email->GetSubject() );
		file->Write( temp, temp.Length() );
		// Body
		idStr body = email->GetBody();
		body.Replace( "\n", "\\n" );
		temp = va( emailBase, "text", body.c_str() );
		file->Write( temp, temp.Length() );
		
		totalEmailCount++;
	}
	
	int videoCount = linearLists[ DECL_VIDEO ].Num();
	if( videoCount > 0 )
	{
		temp = "\n\n//////// PDA Videos ////////////\n";
		file->Write( temp, temp.Length() );
	}
	for( int i = 0; i < videoCount; i++ )
	{
		const idDeclVideo* video = static_cast< const idDeclVideo* >( FindType( DECL_VIDEO, linearLists[ DECL_VIDEO ][ i ]->GetName(), false ) );
		
		idStr videoBaseStrId = va( "\t\"#str_%s_", video->GetName() );
		idStr videoBase = videoBaseStrId + "video_%s" + tailEnd;
		
		file->Write( "\t//Video\n", 9 );
		
		// Name
		temp = va( videoBase, "name", video->GetVideoName() );
		file->Write( temp, temp.Length() );
		// Info
		idStr info = video->GetInfo();
		info.Replace( "\n", "\\n" );
		temp = va( videoBase, "info", info.c_str() );
		file->Write( temp, temp.Length() );
		
		totalVideoCount++;
	}
	
	file->Flush();
	
	idLib::Printf( "\nData written to %s\n", pdaStringsFileName.c_str() );
	idLib::Printf( "----------------------------\n" );
	idLib::Printf( "Wrote %d PDAs.\n", count );
	idLib::Printf( "Wrote %d Emails.\n", totalEmailCount );
	idLib::Printf( "Wrote %d Audio Records.\n", totalAudioCount );
	idLib::Printf( "Wrote %d Video Records.\n", totalVideoCount );
	idLib::Printf( "Please copy the results into the appropriate .lang file.\n" );
}

/*
====================================================================================

	idDeclLocal

====================================================================================
*/

/*
=================
idDeclLocal::idDeclLocal
=================
*/
idDeclLocal::idDeclLocal()
{
	name = "unnamed";
	textSource = NULL;
	textLength = 0;
	compressedLength = 0;
	sourceFile = NULL;
	sourceTextOffset = 0;
	sourceTextLength = 0;
	sourceLine = 0;
	checksum = 0;
	type = DECL_ENTITYDEF;
	index = 0;
	declState = DS_UNPARSED;
	parsedOutsideLevelLoad = false;
	referencedThisLevel = false;
	everReferenced = false;
	redefinedInReload = false;
	nextInFile = NULL;
}

/*
=================
idDeclLocal::GetName
=================
*/
const char* idDeclLocal::GetName() const
{
	return name.c_str();
}

/*
=================
idDeclLocal::GetType
=================
*/
declType_t idDeclLocal::GetType() const
{
	return type;
}

/*
=================
idDeclLocal::GetState
=================
*/
declState_t idDeclLocal::GetState() const
{
	return declState;
}

/*
=================
idDeclLocal::IsImplicit
=================
*/
bool idDeclLocal::IsImplicit() const
{
	return ( sourceFile == declManagerLocal.GetImplicitDeclFile() );
}

/*
=================
idDeclLocal::IsValid
=================
*/
bool idDeclLocal::IsValid() const
{
	return ( declState != DS_UNPARSED );
}

/*
=================
idDeclLocal::Invalidate
=================
*/
void idDeclLocal::Invalidate()
{
	declState = DS_UNPARSED;
}

/*
=================
idDeclLocal::EnsureNotPurged
=================
*/
void idDeclLocal::EnsureNotPurged()
{
	if( declState == DS_UNPARSED )
	{
		ParseLocal();
	}
}

/*
=================
idDeclLocal::Index
=================
*/
int idDeclLocal::Index() const
{
	return index;
}

/*
=================
idDeclLocal::GetLineNum
=================
*/
int idDeclLocal::GetLineNum() const
{
	return sourceLine;
}

/*
=================
idDeclLocal::GetFileName
=================
*/
const char* idDeclLocal::GetFileName() const
{
	return ( sourceFile ) ? sourceFile->fileName.c_str() : "*invalid*";
}

/*
=================
idDeclLocal::Size
=================
*/
size_t idDeclLocal::Size() const
{
	return sizeof( idDecl ) + name.Allocated();
}

/*
=================
idDeclLocal::GetText
=================
*/
void idDeclLocal::GetText( char* text ) const
{
#ifdef USE_COMPRESSED_DECLS
	HuffmanDecompressText( text, textLength, ( byte* )textSource, compressedLength );
#else
	memcpy( text, textSource, textLength + 1 );
#endif
}

/*
=================
idDeclLocal::GetTextLength
=================
*/
int idDeclLocal::GetTextLength() const
{
	return textLength;
}

/*
=================
idDeclLocal::SetText
=================
*/
void idDeclLocal::SetText( const char* text )
{
	SetTextLocal( text, idStr::Length( text ) );
}

/*
=================
idDeclLocal::SetTextLocal
=================
*/
void idDeclLocal::SetTextLocal( const char* text, const int length )
{

	Mem_Free( textSource );
	
	checksum = MD5_BlockChecksum( text, length );
	
#ifdef GET_HUFFMAN_FREQUENCIES
	for( int i = 0; i < length; i++ )
	{
		huffmanFrequencies[( ( const unsigned char* )text )[i]]++;
	}
#endif
	
#ifdef USE_COMPRESSED_DECLS
	int maxBytesPerCode = ( maxHuffmanBits + 7 ) >> 3;
	byte* compressed = ( byte* )_alloca( length * maxBytesPerCode );
	compressedLength = HuffmanCompressText( text, length, compressed, length * maxBytesPerCode );
	textSource = ( char* )Mem_Alloc( compressedLength, TAG_DECLTEXT );
	memcpy( textSource, compressed, compressedLength );
#else
	compressedLength = length;
	textSource = ( char* ) Mem_Alloc( length + 1, TAG_DECLTEXT );
	memcpy( textSource, text, length );
	textSource[length] = '\0';
#endif
	textLength = length;
}

/*
=================
idDeclLocal::ReplaceSourceFileText
=================
*/
bool idDeclLocal::ReplaceSourceFileText()
{
	int oldFileLength, newFileLength;
	idFile* file;
	
	common->Printf( "Writing \'%s\' to \'%s\'...\n", GetName(), GetFileName() );
	
	if( sourceFile == &declManagerLocal.implicitDecls )
	{
		common->Warning( "Can't save implicit declaration %s.", GetName() );
		return false;
	}
	
	// get length and allocate buffer to hold the file
	oldFileLength = sourceFile->fileSize;
	newFileLength = oldFileLength - sourceTextLength + textLength;
	idTempArray<char> buffer( Max( newFileLength, oldFileLength ) );
	memset( buffer.Ptr(), 0, buffer.Size() );
	
	// read original file
	if( sourceFile->fileSize )
	{
	
		file = fileSystem->OpenFileRead( GetFileName() );
		if( !file )
		{
			common->Warning( "Couldn't open %s for reading.", GetFileName() );
			return false;
		}
		
		if( file->Length() != sourceFile->fileSize || file->Timestamp() != sourceFile->timestamp )
		{
			common->Warning( "The file %s has been modified outside of the engine.", GetFileName() );
			return false;
		}
		
		file->Read( buffer.Ptr(), oldFileLength );
		fileSystem->CloseFile( file );
		
		if( MD5_BlockChecksum( buffer.Ptr(), oldFileLength ) != ( unsigned int )sourceFile->checksum )
		{
			common->Warning( "The file %s has been modified outside of the engine.", GetFileName() );
			return false;
		}
	}
	
	// insert new text
	char* declText = ( char* ) _alloca( textLength + 1 );
	GetText( declText );
	memmove( buffer.Ptr() + sourceTextOffset + textLength, buffer.Ptr() + sourceTextOffset + sourceTextLength, oldFileLength - sourceTextOffset - sourceTextLength );
	memcpy( buffer.Ptr() + sourceTextOffset, declText, textLength );
	
	// write out new file
	file = fileSystem->OpenFileWrite( GetFileName(), "fs_basepath" );
	if( !file )
	{
		common->Warning( "Couldn't open %s for writing.", GetFileName() );
		return false;
	}
	file->Write( buffer.Ptr(), newFileLength );
	fileSystem->CloseFile( file );
	
	// set new file size, checksum and timestamp
	sourceFile->fileSize = newFileLength;
	sourceFile->checksum = MD5_BlockChecksum( buffer.Ptr(), newFileLength );
	fileSystem->ReadFile( GetFileName(), NULL, &sourceFile->timestamp );
	
	// move all decls in the same file
	for( idDeclLocal* decl = sourceFile->decls; decl; decl = decl->nextInFile )
	{
		if( decl->sourceTextOffset > sourceTextOffset )
		{
			decl->sourceTextOffset += textLength - sourceTextLength;
		}
	}
	
	// set new size of text in source file
	sourceTextLength = textLength;
	
	return true;
}

/*
=================
idDeclLocal::SourceFileChanged
=================
*/
bool idDeclLocal::SourceFileChanged() const
{
	int newLength;
	ID_TIME_T newTimestamp;
	
	if( sourceFile->fileSize <= 0 )
	{
		return false;
	}
	
	newLength = fileSystem->ReadFile( GetFileName(), NULL, &newTimestamp );
	
	if( newLength != sourceFile->fileSize || newTimestamp != sourceFile->timestamp )
	{
		return true;
	}
	
	return false;
}

/*
=================
idDeclLocal::MakeDefault
=================
*/
void idDeclLocal::MakeDefault()
{
	static int recursionLevel;
	const char* defaultText;
	
	declManagerLocal.MediaPrint( "DEFAULTED\n" );
	declState = DS_DEFAULTED;
	
	AllocateSelf();
	
	defaultText = self->DefaultDefinition();
	
	// a parse error inside a DefaultDefinition() string could
	// cause an infinite loop, but normal default definitions could
	// still reference other default definitions, so we can't
	// just dump out on the first recursion
	if( ++recursionLevel > 100 )
	{
		common->FatalError( "idDecl::MakeDefault: bad DefaultDefinition(): %s", defaultText );
	}
	
	// always free data before parsing
	self->FreeData();
	
	// parse
	self->Parse( defaultText, strlen( defaultText ), false );
	
	// we could still eventually hit the recursion if we have enough Error() calls inside Parse...
	--recursionLevel;
}

/*
=================
idDeclLocal::SetDefaultText
=================
*/
bool idDeclLocal::SetDefaultText()
{
	return false;
}

/*
=================
idDeclLocal::DefaultDefinition
=================
*/
const char* idDeclLocal::DefaultDefinition() const
{
	return "{ }";
}

/*
=================
idDeclLocal::Parse
=================
*/
bool idDeclLocal::Parse( const char* text, const int textLength, bool allowBinaryVersion )
{
	idLexer src;
	
	src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	src.SetFlags( DECL_LEXER_FLAGS );
	src.SkipUntilString( "{" );
	src.SkipBracedSection( false );
	return true;
}

/*
=================
idDeclLocal::FreeData
=================
*/
void idDeclLocal::FreeData()
{
}

/*
=================
idDeclLocal::List
=================
*/
void idDeclLocal::List() const
{
	common->Printf( "%s\n", GetName() );
}

/*
=================
idDeclLocal::Print
=================
*/
void idDeclLocal::Print() const
{
}

/*
=================
idDeclLocal::Reload
=================
*/
void idDeclLocal::Reload()
{
	this->sourceFile->Reload( false );
}

/*
=================
idDeclLocal::AllocateSelf
=================
*/
void idDeclLocal::AllocateSelf()
{
	if( self == NULL )
	{
		self = declManagerLocal.GetDeclType( ( int )type )->allocator();
		self->base = this;
	}
}

/*
=================
idDeclLocal::ParseLocal
=================
*/
void idDeclLocal::ParseLocal()
{
	bool generatedDefaultText = false;
	
	AllocateSelf();
	
	// always free data before parsing
	self->FreeData();
	
	declManagerLocal.MediaPrint( "parsing %s %s\n", declManagerLocal.declTypes[type]->typeName.c_str(), name.c_str() );
	
	// if no text source try to generate default text
	if( textSource == NULL )
	{
		generatedDefaultText = self->SetDefaultText();
	}
	
	// indent for DEFAULTED or media file references
	declManagerLocal.indent++;
	
	// no text immediately causes a MakeDefault()
	if( textSource == NULL )
	{
		MakeDefault();
		declManagerLocal.indent--;
		return;
	}
	
	declState = DS_PARSED;
	
	// parse
	char* declText = ( char* ) _alloca( ( GetTextLength() + 1 ) * sizeof( char ) );
	GetText( declText );
	self->Parse( declText, GetTextLength(), true );
	
	// free generated text
	if( generatedDefaultText )
	{
		Mem_Free( textSource );
		textSource = NULL;
		textLength = 0;
	}
	
	declManagerLocal.indent--;
}

/*
=================
idDeclLocal::Purge
=================
*/
void idDeclLocal::Purge()
{
	// never purge things that were referenced outside level load,
	// like the console and menu graphics
	if( parsedOutsideLevelLoad )
	{
		return;
	}
	
	referencedThisLevel = false;
	MakeDefault();
	
	// the next Find() for this will re-parse the real data
	declState = DS_UNPARSED;
}

/*
=================
idDeclLocal::EverReferenced
=================
*/
bool idDeclLocal::EverReferenced() const
{
	return everReferenced;
}
