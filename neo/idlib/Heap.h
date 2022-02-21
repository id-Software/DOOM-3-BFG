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

#ifndef __HEAP_H__
#define __HEAP_H__

/*
===============================================================================

	Memory Management

===============================================================================
*/

// memory tag names are used to sort allocations for sys_dumpMemory and other reporting functions
enum memTag_t
{
#define MEM_TAG( x )	TAG_##x,
#include "sys/sys_alloc_tags.h"
	TAG_NUM_TAGS,
};

static const int MAX_TAGS = 256;



// RB: 64 bit fixes, changed int to size_t
void* 		Mem_Alloc16( const size_t size, const memTag_t tag );
void		Mem_Free16( void* ptr );

ID_INLINE void* 	Mem_Alloc( const size_t size, const memTag_t tag )
{
	return Mem_Alloc16( size, tag );
}

ID_INLINE void		Mem_Free( void* ptr )
{
	Mem_Free16( ptr );
}

void* 		Mem_ClearedAlloc( const size_t size, const memTag_t tag );
char* 		Mem_CopyString( const char* in );
// RB end

ID_INLINE void* operator new( size_t s )
{
	return Mem_Alloc( s, TAG_NEW );
}

// SRS - Added noexcept to silence build-time warning
ID_INLINE void operator delete( void* p ) noexcept
{
	Mem_Free( p );
}
ID_INLINE void* operator new[]( size_t s )
{
	return Mem_Alloc( s, TAG_NEW );
}

// SRS - Added noexcept to silence build-time warning
ID_INLINE void operator delete[]( void* p ) noexcept
{
	Mem_Free( p );
}

ID_INLINE void* operator new( size_t s, memTag_t tag )
{
	return Mem_Alloc( s, tag );
}

ID_INLINE void operator delete( void* p, memTag_t tag )
{
	Mem_Free( p );
}

ID_INLINE void* operator new[]( size_t s, memTag_t tag )
{
	return Mem_Alloc( s, tag );
}

ID_INLINE void operator delete[]( void* p, memTag_t tag )
{
	Mem_Free( p );
}

// Define replacements for the PS3 library's aligned new operator.
// Without these, allocations of objects with 32 byte or greater alignment
// may not go through our memory system.

/*
================================================
idTempArray is an array that is automatically free'd when it goes out of scope.
There is no "cast" operator because these are very unsafe.

The template parameter MUST BE POD!

Compile time asserting POD-ness of the template parameter is complicated due
to our vector classes that need a default constructor but are otherwise
considered POD.
================================================
*/
template < class T >
class idTempArray
{
public:
	idTempArray( idTempArray<T>& other );
	idTempArray( unsigned int num );

	~idTempArray();

	T& operator []( unsigned int i )
	{
		assert( i < num );
		return buffer[i];
	}
	const T& operator []( unsigned int i ) const
	{
		assert( i < num );
		return buffer[i];
	}

	T* Ptr()
	{
		return buffer;
	}
	const T* Ptr() const
	{
		return buffer;
	}

	size_t Size() const
	{
		return num * sizeof( T );
	}
	unsigned int Num() const
	{
		return num;
	}

	void Zero()
	{
		memset( Ptr(), 0, Size() );
	}

private:
	T* 				buffer;		// Ensure this buffer comes first, so this == &this->buffer
	unsigned int	num;
};

/*
========================
idTempArray::idTempArray
========================
*/
template < class T >
ID_INLINE idTempArray<T>::idTempArray( idTempArray<T>& other )
{
	this->num = other.num;
	this->buffer = other.buffer;
	other.num = 0;
	other.buffer = NULL;
}

/*
========================
idTempArray::idTempArray
========================
*/
template < class T >
ID_INLINE idTempArray<T>::idTempArray( unsigned int num )
{
	this->num = num;
	buffer = ( T* )Mem_Alloc( num * sizeof( T ), TAG_TEMP );
}

/*
========================
idTempArray::~idTempArray
========================
*/
template < class T >
ID_INLINE idTempArray<T>::~idTempArray()
{
	Mem_Free( buffer );
}

/*
===============================================================================

	Block based allocator for fixed size objects.

	All objects of the 'type' are properly constructed and destructed when reused.

===============================================================================
*/

#define BLOCK_ALLOC_ALIGNMENT 16

// Define this to force all block allocators to act like normal new/delete allocation
// for tool checking.
//#define	FORCE_DISCRETE_BLOCK_ALLOCS

/*
================================================
idBlockAlloc is a block-based allocator for fixed-size objects.

All objects are properly constructed and destructed.
================================================
*/
template<class _type_, int _blockSize_, memTag_t memTag = TAG_BLOCKALLOC>
class idBlockAlloc
{
public:
	ID_INLINE			idBlockAlloc( bool clear = false );
	ID_INLINE			~idBlockAlloc();

	// returns total size of allocated memory
	size_t				Allocated() const
	{
		return total * sizeof( _type_ );
	}

	// returns total size of allocated memory including size of (*this)
	size_t				Size() const
	{
		return sizeof( *this ) + Allocated();
	}

	ID_INLINE void		Shutdown();
	ID_INLINE void		SetFixedBlocks( int numBlocks );
	ID_INLINE void		FreeEmptyBlocks();

	ID_INLINE _type_* 	Alloc();
	ID_INLINE void		Free( _type_ *element );

	int					GetTotalCount() const
	{
		return total;
	}
	int					GetAllocCount() const
	{
		return active;
	}
	int					GetFreeCount() const
	{
		return total - active;
	}

private:
	union element_t
	{
		_type_* 		data;	// this is a hack to make sure the save game system marks _type_ as saveable
		element_t* 		next;
		byte			buffer[( CONST_MAX( sizeof( _type_ ), sizeof( element_t* ) ) + ( BLOCK_ALLOC_ALIGNMENT - 1 ) ) & ~( BLOCK_ALLOC_ALIGNMENT - 1 )];
	};

	class idBlock
	{
	public:
		element_t		elements[_blockSize_];
		idBlock* 		next;
		element_t* 		free;		// list with free elements in this block (temp used only by FreeEmptyBlocks)
		int				freeCount;	// number of free elements in this block (temp used only by FreeEmptyBlocks)
	};

	idBlock* 			blocks;
	element_t* 			free;
	int					total;
	int					active;
	bool				allowAllocs;
	bool				clearAllocs;

	ID_INLINE void		AllocNewBlock();
};

/*
========================
idBlockAlloc<_type_,_blockSize_,align_t>::idBlockAlloc
========================
*/
template<class _type_, int _blockSize_, memTag_t memTag>
ID_INLINE idBlockAlloc<_type_, _blockSize_, memTag>::idBlockAlloc( bool clear ) :
	blocks( NULL ),
	free( NULL ),
	total( 0 ),
	active( 0 ),
	allowAllocs( true ),
	clearAllocs( clear )
{
}

/*
========================
idBlockAlloc<_type_,_blockSize__,align_t>::~idBlockAlloc
========================
*/
template<class _type_, int _blockSize_, memTag_t memTag>
ID_INLINE idBlockAlloc<_type_, _blockSize_, memTag>::~idBlockAlloc()
{
	Shutdown();
}

/*
========================
idBlockAlloc<_type_,_blockSize_,align_t>::Alloc
========================
*/
template<class _type_, int _blockSize_, memTag_t memTag>
ID_INLINE _type_* idBlockAlloc<_type_, _blockSize_, memTag>::Alloc()
{
#ifdef FORCE_DISCRETE_BLOCK_ALLOCS
	// for debugging tools
	return new _type_;
#else
	if( free == NULL )
	{
		if( !allowAllocs )
		{
			return NULL;
		}
		AllocNewBlock();
	}

	active++;
	element_t* element = free;
	free = free->next;
	element->next = NULL;

	_type_ * t = ( _type_* ) element->buffer;
	if( clearAllocs )
	{
		memset( ( void* )t, 0, sizeof( _type_ ) );  // SRS - Added (void*) cast to silence build-time warning
	}
	new( t ) _type_;
	return t;
#endif
}

/*
========================
idBlockAlloc<_type_,_blockSize_,align_t>::Free
========================
*/
template<class _type_, int _blockSize_, memTag_t memTag>
ID_INLINE void idBlockAlloc<_type_, _blockSize_, memTag>::Free( _type_ * t )
{
#ifdef FORCE_DISCRETE_BLOCK_ALLOCS
	// for debugging tools
	delete t;
#else
	if( t == NULL )
	{
		return;
	}

	t->~_type_();

	element_t* element = ( element_t* )( t );
	element->next = free;
	free = element;
	active--;
#endif
}

/*
========================
idBlockAlloc<_type_,_blockSize_,align_t>::Shutdown
========================
*/
template<class _type_, int _blockSize_, memTag_t memTag>
ID_INLINE void idBlockAlloc<_type_, _blockSize_, memTag>::Shutdown()
{
	while( blocks != NULL )
	{
		idBlock* block = blocks;
		blocks = blocks->next;
		Mem_Free( block );
	}
	blocks = NULL;
	free = NULL;
	total = active = 0;
}

/*
========================
idBlockAlloc<_type_,_blockSize_,align_t>::SetFixedBlocks
========================
*/
template<class _type_, int _blockSize_, memTag_t memTag>
ID_INLINE void idBlockAlloc<_type_, _blockSize_, memTag>::SetFixedBlocks( int numBlocks )
{
	int currentNumBlocks = 0;
	for( idBlock* block = blocks; block != NULL; block = block->next )
	{
		currentNumBlocks++;
	}
	for( int i = currentNumBlocks; i < numBlocks; i++ )
	{
		AllocNewBlock();
	}
	allowAllocs = false;
}

/*
========================
idBlockAlloc<_type_,_blockSize_,align_t>::AllocNewBlock
========================
*/
template<class _type_, int _blockSize_, memTag_t memTag>
ID_INLINE void idBlockAlloc<_type_, _blockSize_, memTag>::AllocNewBlock()
{
	idBlock* block = ( idBlock* )Mem_Alloc( sizeof( idBlock ), memTag );
	block->next = blocks;
	blocks = block;
	for( int i = 0; i < _blockSize_; i++ )
	{
		block->elements[i].next = free;
		free = &block->elements[i];

		// RB: changed UINT_PTR to uintptr_t
		assert( ( ( ( uintptr_t )free ) & ( BLOCK_ALLOC_ALIGNMENT - 1 ) ) == 0 );
		// RB end
	}
	total += _blockSize_;
}

/*
========================
idBlockAlloc<_type_,_blockSize_,align_t>::FreeEmptyBlocks
========================
*/
template<class _type_, int _blockSize_, memTag_t memTag>
ID_INLINE void idBlockAlloc<_type_, _blockSize_, memTag>::FreeEmptyBlocks()
{
	// first count how many free elements are in each block
	// and build up a free chain per block
	for( idBlock* block = blocks; block != NULL; block = block->next )
	{
		block->free = NULL;
		block->freeCount = 0;
	}
	for( element_t* element = free; element != NULL; )
	{
		element_t* next = element->next;
		for( idBlock* block = blocks; block != NULL; block = block->next )
		{
			if( element >= block->elements && element < block->elements + _blockSize_ )
			{
				element->next = block->free;
				block->free = element;
				block->freeCount++;
				break;
			}
		}
		// if this assert fires, we couldn't find the element in any block
		assert( element->next != next );
		element = next;
	}
	// now free all blocks whose free count == _blockSize_
	idBlock* prevBlock = NULL;
	for( idBlock* block = blocks; block != NULL; )
	{
		idBlock* next = block->next;
		if( block->freeCount == _blockSize_ )
		{
			if( prevBlock == NULL )
			{
				assert( blocks == block );
				blocks = block->next;
			}
			else
			{
				assert( prevBlock->next == block );
				prevBlock->next = block->next;
			}
			Mem_Free( block );
			total -= _blockSize_;
		}
		else
		{
			prevBlock = block;
		}
		block = next;
	}
	// now rebuild the free chain
	free = NULL;
	for( idBlock* block = blocks; block != NULL; block = block->next )
	{
		for( element_t* element = block->free; element != NULL; )
		{
			element_t* next = element->next;
			element->next = free;
			free = element;
			element = next;
		}
	}
}

/*
==============================================================================

	Dynamic allocator, simple wrapper for normal allocations which can
	be interchanged with idDynamicBlockAlloc.

	No constructor is called for the 'type'.
	Allocated blocks are always 16 byte aligned.

==============================================================================
*/

template<class type, int baseBlockSize, int minBlockSize>
class idDynamicAlloc
{
public:
	idDynamicAlloc();
	~idDynamicAlloc();

	void							Init();
	void							Shutdown();
	void							SetFixedBlocks( int numBlocks ) {}
	void							SetLockMemory( bool lock ) {}
	void							FreeEmptyBaseBlocks() {}

	type* 							Alloc( const int num );
	type* 							Resize( type* ptr, const int num );
	void							Free( type* ptr );
	const char* 					CheckMemory( const type* ptr ) const;

	int								GetNumBaseBlocks() const
	{
		return 0;
	}
	int								GetBaseBlockMemory() const
	{
		return 0;
	}
	int								GetNumUsedBlocks() const
	{
		return numUsedBlocks;
	}
	int								GetUsedBlockMemory() const
	{
		return usedBlockMemory;
	}
	int								GetNumFreeBlocks() const
	{
		return 0;
	}
	int								GetFreeBlockMemory() const
	{
		return 0;
	}
	int								GetNumEmptyBaseBlocks() const
	{
		return 0;
	}

private:
	int								numUsedBlocks;			// number of used blocks
	int								usedBlockMemory;		// total memory in used blocks

	int								numAllocs;
	int								numResizes;
	int								numFrees;

	void							Clear();
};

template<class type, int baseBlockSize, int minBlockSize>
idDynamicAlloc<type, baseBlockSize, minBlockSize>::idDynamicAlloc()
{
	Clear();
}

template<class type, int baseBlockSize, int minBlockSize>
idDynamicAlloc<type, baseBlockSize, minBlockSize>::~idDynamicAlloc()
{
	Shutdown();
}

template<class type, int baseBlockSize, int minBlockSize>
void idDynamicAlloc<type, baseBlockSize, minBlockSize>::Init()
{
}

template<class type, int baseBlockSize, int minBlockSize>
void idDynamicAlloc<type, baseBlockSize, minBlockSize>::Shutdown()
{
	Clear();
}

template<class type, int baseBlockSize, int minBlockSize>
type* idDynamicAlloc<type, baseBlockSize, minBlockSize>::Alloc( const int num )
{
	numAllocs++;
	if( num <= 0 )
	{
		return NULL;
	}
	numUsedBlocks++;
	usedBlockMemory += num * sizeof( type );
	return Mem_Alloc16( num * sizeof( type ), TAG_BLOCKALLOC );
}

template<class type, int baseBlockSize, int minBlockSize>
type* idDynamicAlloc<type, baseBlockSize, minBlockSize>::Resize( type* ptr, const int num )
{

	numResizes++;

	if( ptr == NULL )
	{
		return Alloc( num );
	}

	if( num <= 0 )
	{
		Free( ptr );
		return NULL;
	}

	assert( 0 );
	return ptr;
}

template<class type, int baseBlockSize, int minBlockSize>
void idDynamicAlloc<type, baseBlockSize, minBlockSize>::Free( type* ptr )
{
	numFrees++;
	if( ptr == NULL )
	{
		return;
	}
	Mem_Free16( ptr );
}

template<class type, int baseBlockSize, int minBlockSize>
const char* idDynamicAlloc<type, baseBlockSize, minBlockSize>::CheckMemory( const type* ptr ) const
{
	return NULL;
}

template<class type, int baseBlockSize, int minBlockSize>
void idDynamicAlloc<type, baseBlockSize, minBlockSize>::Clear()
{
	numUsedBlocks = 0;
	usedBlockMemory = 0;
	numAllocs = 0;
	numResizes = 0;
	numFrees = 0;
}


/*
==============================================================================

	Fast dynamic block allocator.

	No constructor is called for the 'type'.
	Allocated blocks are always 16 byte aligned.

==============================================================================
*/

#include "containers/BTree.h"

//#define DYNAMIC_BLOCK_ALLOC_CHECK

template<class type>
class idDynamicBlock
{
public:
	type* 							GetMemory() const
	{
		return ( type* )( ( ( byte* ) this ) + sizeof( idDynamicBlock<type> ) );
	}
	int								GetSize() const
	{
		return abs( size );
	}
	void							SetSize( int s, bool isBaseBlock )
	{
		size = isBaseBlock ? -s : s;
	}
	bool							IsBaseBlock() const
	{
		return ( size < 0 );
	}

#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
	int								id[3];
	void* 							allocator;
#endif

	int								size;					// size in bytes of the block
	idDynamicBlock<type>* 			prev;					// previous memory block
	idDynamicBlock<type>* 			next;					// next memory block
	idBTreeNode<idDynamicBlock<type>, int>* node;			// node in the B-Tree with free blocks
};

template<class type, int baseBlockSize, int minBlockSize, memTag_t _tag_ = TAG_BLOCKALLOC>
class idDynamicBlockAlloc
{
public:
	idDynamicBlockAlloc();
	~idDynamicBlockAlloc();

	void							Init();
	void							Shutdown();
	void							SetFixedBlocks( int numBlocks );
	void							SetLockMemory( bool lock );
	void							FreeEmptyBaseBlocks();

	type* 							Alloc( const int num );
	type* 							Resize( type* ptr, const int num );
	void							Free( type* ptr );
	const char* 					CheckMemory( const type* ptr ) const;

	int								GetNumBaseBlocks() const
	{
		return numBaseBlocks;
	}
	int								GetBaseBlockMemory() const
	{
		return baseBlockMemory;
	}
	int								GetNumUsedBlocks() const
	{
		return numUsedBlocks;
	}
	int								GetUsedBlockMemory() const
	{
		return usedBlockMemory;
	}
	int								GetNumFreeBlocks() const
	{
		return numFreeBlocks;
	}
	int								GetFreeBlockMemory() const
	{
		return freeBlockMemory;
	}
	int								GetNumEmptyBaseBlocks() const;

private:
	idDynamicBlock<type>* 			firstBlock;				// first block in list in order of increasing address
	idDynamicBlock<type>* 			lastBlock;				// last block in list in order of increasing address
	idBTree<idDynamicBlock<type>, int, 4>freeTree;			// B-Tree with free memory blocks
	bool							allowAllocs;			// allow base block allocations
	bool							lockMemory;				// lock memory so it cannot get swapped out

#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
	int								blockId[3];
#endif

	int								numBaseBlocks;			// number of base blocks
	int								baseBlockMemory;		// total memory in base blocks
	int								numUsedBlocks;			// number of used blocks
	int								usedBlockMemory;		// total memory in used blocks
	int								numFreeBlocks;			// number of free blocks
	int								freeBlockMemory;		// total memory in free blocks

	int								numAllocs;
	int								numResizes;
	int								numFrees;

	memTag_t						tag;

	void							Clear();
	idDynamicBlock<type>* 			AllocInternal( const int num );
	idDynamicBlock<type>* 			ResizeInternal( idDynamicBlock<type>* block, const int num );
	void							FreeInternal( idDynamicBlock<type>* block );
	void							LinkFreeInternal( idDynamicBlock<type>* block );
	void							UnlinkFreeInternal( idDynamicBlock<type>* block );
	void							CheckMemory() const;
};

template<class type, int baseBlockSize, int minBlockSize, memTag_t _tag_>
idDynamicBlockAlloc<type, baseBlockSize, minBlockSize, _tag_>::idDynamicBlockAlloc()
{
	tag = _tag_;
	Clear();
}

template<class type, int baseBlockSize, int minBlockSize, memTag_t _tag_>
idDynamicBlockAlloc<type, baseBlockSize, minBlockSize, _tag_>::~idDynamicBlockAlloc()
{
	Shutdown();
}

template<class type, int baseBlockSize, int minBlockSize, memTag_t _tag_>
void idDynamicBlockAlloc<type, baseBlockSize, minBlockSize, _tag_>::Init()
{
	freeTree.Init();
}

template<class type, int baseBlockSize, int minBlockSize, memTag_t _tag_>
void idDynamicBlockAlloc<type, baseBlockSize, minBlockSize, _tag_>::Shutdown()
{
	idDynamicBlock<type>* block;

	for( block = firstBlock; block != NULL; block = block->next )
	{
		if( block->node == NULL )
		{
			FreeInternal( block );
		}
	}

	for( block = firstBlock; block != NULL; block = firstBlock )
	{
		firstBlock = block->next;
		assert( block->IsBaseBlock() );
		if( lockMemory )
		{
			//idLib::sys->UnlockMemory( block, block->GetSize() + (int)sizeof( idDynamicBlock<type> ) );
		}
		Mem_Free16( block );
	}

	freeTree.Shutdown();

	Clear();
}

template<class type, int baseBlockSize, int minBlockSize, memTag_t _tag_>
void idDynamicBlockAlloc<type, baseBlockSize, minBlockSize, _tag_>::SetFixedBlocks( int numBlocks )
{
	idDynamicBlock<type>* block;

	for( int i = numBaseBlocks; i < numBlocks; i++ )
	{
		block = ( idDynamicBlock<type>* ) Mem_Alloc16( baseBlockSize, _tag_ );
		if( lockMemory )
		{
			//idLib::sys->LockMemory( block, baseBlockSize );
		}
#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
		memcpy( block->id, blockId, sizeof( block->id ) );
		block->allocator = ( void* )this;
#endif
		block->SetSize( baseBlockSize - ( int )sizeof( idDynamicBlock<type> ), true );
		block->next = NULL;
		block->prev = lastBlock;
		if( lastBlock )
		{
			lastBlock->next = block;
		}
		else
		{
			firstBlock = block;
		}
		lastBlock = block;
		block->node = NULL;

		FreeInternal( block );

		numBaseBlocks++;
		baseBlockMemory += baseBlockSize;
	}

	allowAllocs = false;
}

template<class type, int baseBlockSize, int minBlockSize, memTag_t _tag_>
void idDynamicBlockAlloc<type, baseBlockSize, minBlockSize, _tag_>::SetLockMemory( bool lock )
{
	lockMemory = lock;
}

template<class type, int baseBlockSize, int minBlockSize, memTag_t _tag_>
void idDynamicBlockAlloc<type, baseBlockSize, minBlockSize, _tag_>::FreeEmptyBaseBlocks()
{
	idDynamicBlock<type>* block, *next;

	for( block = firstBlock; block != NULL; block = next )
	{
		next = block->next;

		if( block->IsBaseBlock() && block->node != NULL && ( next == NULL || next->IsBaseBlock() ) )
		{
			UnlinkFreeInternal( block );
			if( block->prev )
			{
				block->prev->next = block->next;
			}
			else
			{
				firstBlock = block->next;
			}
			if( block->next )
			{
				block->next->prev = block->prev;
			}
			else
			{
				lastBlock = block->prev;
			}
			if( lockMemory )
			{
				//idLib::sys->UnlockMemory( block, block->GetSize() + (int)sizeof( idDynamicBlock<type> ) );
			}
			numBaseBlocks--;
			baseBlockMemory -= block->GetSize() + ( int )sizeof( idDynamicBlock<type> );
			Mem_Free16( block );
		}
	}

#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
	CheckMemory();
#endif
}

template<class type, int baseBlockSize, int minBlockSize, memTag_t _tag_>
int idDynamicBlockAlloc<type, baseBlockSize, minBlockSize, _tag_>::GetNumEmptyBaseBlocks() const
{
	int numEmptyBaseBlocks;
	idDynamicBlock<type>* block;

	numEmptyBaseBlocks = 0;
	for( block = firstBlock; block != NULL; block = block->next )
	{
		if( block->IsBaseBlock() && block->node != NULL && ( block->next == NULL || block->next->IsBaseBlock() ) )
		{
			numEmptyBaseBlocks++;
		}
	}
	return numEmptyBaseBlocks;
}

template<class type, int baseBlockSize, int minBlockSize, memTag_t _tag_>
type* idDynamicBlockAlloc<type, baseBlockSize, minBlockSize, _tag_>::Alloc( const int num )
{
	idDynamicBlock<type>* block;

	numAllocs++;

	if( num <= 0 )
	{
		return NULL;
	}

	block = AllocInternal( num );
	if( block == NULL )
	{
		return NULL;
	}
	block = ResizeInternal( block, num );
	if( block == NULL )
	{
		return NULL;
	}

#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
	CheckMemory();
#endif

	numUsedBlocks++;
	usedBlockMemory += block->GetSize();

	return block->GetMemory();
}

template<class type, int baseBlockSize, int minBlockSize, memTag_t _tag_>
type* idDynamicBlockAlloc<type, baseBlockSize, minBlockSize, _tag_>::Resize( type* ptr, const int num )
{

	numResizes++;

	if( ptr == NULL )
	{
		return Alloc( num );
	}

	if( num <= 0 )
	{
		Free( ptr );
		return NULL;
	}

	idDynamicBlock<type>* block = ( idDynamicBlock<type>* )( ( ( byte* ) ptr ) - ( int )sizeof( idDynamicBlock<type> ) );

	usedBlockMemory -= block->GetSize();

	block = ResizeInternal( block, num );
	if( block == NULL )
	{
		return NULL;
	}

#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
	CheckMemory();
#endif

	usedBlockMemory += block->GetSize();

	return block->GetMemory();
}

template<class type, int baseBlockSize, int minBlockSize, memTag_t _tag_>
void idDynamicBlockAlloc<type, baseBlockSize, minBlockSize, _tag_>::Free( type* ptr )
{

	numFrees++;

	if( ptr == NULL )
	{
		return;
	}

	idDynamicBlock<type>* block = ( idDynamicBlock<type>* )( ( ( byte* ) ptr ) - ( int )sizeof( idDynamicBlock<type> ) );

	numUsedBlocks--;
	usedBlockMemory -= block->GetSize();

	FreeInternal( block );

#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
	CheckMemory();
#endif
}

template<class type, int baseBlockSize, int minBlockSize, memTag_t _tag_>
const char* idDynamicBlockAlloc<type, baseBlockSize, minBlockSize, _tag_>::CheckMemory( const type* ptr ) const
{
	idDynamicBlock<type>* block;

	if( ptr == NULL )
	{
		return NULL;
	}

	block = ( idDynamicBlock<type>* )( ( ( byte* ) ptr ) - ( int )sizeof( idDynamicBlock<type> ) );

	if( block->node != NULL )
	{
		return "memory has been freed";
	}

#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
	if( block->id[0] != 0x11111111 || block->id[1] != 0x22222222 || block->id[2] != 0x33333333 )
	{
		return "memory has invalid id";
	}
	if( block->allocator != ( void* )this )
	{
		return "memory was allocated with different allocator";
	}
#endif

	/* base blocks can be larger than baseBlockSize which can cause this code to fail
	idDynamicBlock<type> *base;
	for ( base = firstBlock; base != NULL; base = base->next ) {
		if ( base->IsBaseBlock() ) {
			if ( ((int)block) >= ((int)base) && ((int)block) < ((int)base) + baseBlockSize ) {
				break;
			}
		}
	}
	if ( base == NULL ) {
		return "no base block found for memory";
	}
	*/

	return NULL;
}

template<class type, int baseBlockSize, int minBlockSize, memTag_t _tag_>
void idDynamicBlockAlloc<type, baseBlockSize, minBlockSize, _tag_>::Clear()
{
	firstBlock = lastBlock = NULL;
	allowAllocs = true;
	lockMemory = false;
	numBaseBlocks = 0;
	baseBlockMemory = 0;
	numUsedBlocks = 0;
	usedBlockMemory = 0;
	numFreeBlocks = 0;
	freeBlockMemory = 0;
	numAllocs = 0;
	numResizes = 0;
	numFrees = 0;

#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
	blockId[0] = 0x11111111;
	blockId[1] = 0x22222222;
	blockId[2] = 0x33333333;
#endif
}

template<class type, int baseBlockSize, int minBlockSize, memTag_t _tag_>
idDynamicBlock<type>* idDynamicBlockAlloc<type, baseBlockSize, minBlockSize, _tag_>::AllocInternal( const int num )
{
	idDynamicBlock<type>* block;
	int alignedBytes = ( num * sizeof( type ) + 15 ) & ~15;

	block = freeTree.FindSmallestLargerEqual( alignedBytes );
	if( block != NULL )
	{
		UnlinkFreeInternal( block );
	}
	else if( allowAllocs )
	{
		int allocSize = Max( baseBlockSize, alignedBytes + ( int )sizeof( idDynamicBlock<type> ) );
		block = ( idDynamicBlock<type>* ) Mem_Alloc16( allocSize, _tag_ );
		if( lockMemory )
		{
			//idLib::sys->LockMemory( block, baseBlockSize );
		}
#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
		memcpy( block->id, blockId, sizeof( block->id ) );
		block->allocator = ( void* )this;
#endif
		block->SetSize( allocSize - ( int )sizeof( idDynamicBlock<type> ), true );
		block->next = NULL;
		block->prev = lastBlock;
		if( lastBlock )
		{
			lastBlock->next = block;
		}
		else
		{
			firstBlock = block;
		}
		lastBlock = block;
		block->node = NULL;

		numBaseBlocks++;
		baseBlockMemory += allocSize;
	}

	return block;
}

template<class type, int baseBlockSize, int minBlockSize, memTag_t _tag_>
idDynamicBlock<type>* idDynamicBlockAlloc<type, baseBlockSize, minBlockSize, _tag_>::ResizeInternal( idDynamicBlock<type>* block, const int num )
{
	int alignedBytes = ( num * sizeof( type ) + 15 ) & ~15;

#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
	assert( block->id[0] == 0x11111111 && block->id[1] == 0x22222222 && block->id[2] == 0x33333333 && block->allocator == ( void* )this );
#endif

	// if the new size is larger
	if( alignedBytes > block->GetSize() )
	{

		idDynamicBlock<type>* nextBlock = block->next;

		// try to annexate the next block if it's free
		if( nextBlock && !nextBlock->IsBaseBlock() && nextBlock->node != NULL &&
				block->GetSize() + ( int )sizeof( idDynamicBlock<type> ) + nextBlock->GetSize() >= alignedBytes )
		{

			UnlinkFreeInternal( nextBlock );
			block->SetSize( block->GetSize() + ( int )sizeof( idDynamicBlock<type> ) + nextBlock->GetSize(), block->IsBaseBlock() );
			block->next = nextBlock->next;
			if( nextBlock->next )
			{
				nextBlock->next->prev = block;
			}
			else
			{
				lastBlock = block;
			}
		}
		else
		{
			// allocate a new block and copy
			idDynamicBlock<type>* oldBlock = block;
			block = AllocInternal( num );
			if( block == NULL )
			{
				return NULL;
			}
			memcpy( block->GetMemory(), oldBlock->GetMemory(), oldBlock->GetSize() );
			FreeInternal( oldBlock );
		}
	}

	// if the unused space at the end of this block is large enough to hold a block with at least one element
	if( block->GetSize() - alignedBytes - ( int )sizeof( idDynamicBlock<type> ) < Max( minBlockSize, ( int )sizeof( type ) ) )
	{
		return block;
	}

	idDynamicBlock<type>* newBlock;

	newBlock = ( idDynamicBlock<type>* )( ( ( byte* ) block ) + ( int )sizeof( idDynamicBlock<type> ) + alignedBytes );
#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
	memcpy( newBlock->id, blockId, sizeof( newBlock->id ) );
	newBlock->allocator = ( void* )this;
#endif
	newBlock->SetSize( block->GetSize() - alignedBytes - ( int )sizeof( idDynamicBlock<type> ), false );
	newBlock->next = block->next;
	newBlock->prev = block;
	if( newBlock->next )
	{
		newBlock->next->prev = newBlock;
	}
	else
	{
		lastBlock = newBlock;
	}
	newBlock->node = NULL;
	block->next = newBlock;
	block->SetSize( alignedBytes, block->IsBaseBlock() );

	FreeInternal( newBlock );

	return block;
}

template<class type, int baseBlockSize, int minBlockSize, memTag_t _tag_>
void idDynamicBlockAlloc<type, baseBlockSize, minBlockSize, _tag_>::FreeInternal( idDynamicBlock<type>* block )
{

	assert( block->node == NULL );

#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
	assert( block->id[0] == 0x11111111 && block->id[1] == 0x22222222 && block->id[2] == 0x33333333 && block->allocator == ( void* )this );
#endif

	// try to merge with a next free block
	idDynamicBlock<type>* nextBlock = block->next;
	if( nextBlock && !nextBlock->IsBaseBlock() && nextBlock->node != NULL )
	{
		UnlinkFreeInternal( nextBlock );
		block->SetSize( block->GetSize() + ( int )sizeof( idDynamicBlock<type> ) + nextBlock->GetSize(), block->IsBaseBlock() );
		block->next = nextBlock->next;
		if( nextBlock->next )
		{
			nextBlock->next->prev = block;
		}
		else
		{
			lastBlock = block;
		}
	}

	// try to merge with a previous free block
	idDynamicBlock<type>* prevBlock = block->prev;
	if( prevBlock && !block->IsBaseBlock() && prevBlock->node != NULL )
	{
		UnlinkFreeInternal( prevBlock );
		prevBlock->SetSize( prevBlock->GetSize() + ( int )sizeof( idDynamicBlock<type> ) + block->GetSize(), prevBlock->IsBaseBlock() );
		prevBlock->next = block->next;
		if( block->next )
		{
			block->next->prev = prevBlock;
		}
		else
		{
			lastBlock = prevBlock;
		}
		LinkFreeInternal( prevBlock );
	}
	else
	{
		LinkFreeInternal( block );
	}
}

template<class type, int baseBlockSize, int minBlockSize, memTag_t _tag_>
ID_INLINE void idDynamicBlockAlloc<type, baseBlockSize, minBlockSize, _tag_>::LinkFreeInternal( idDynamicBlock<type>* block )
{
	block->node = freeTree.Add( block, block->GetSize() );
	numFreeBlocks++;
	freeBlockMemory += block->GetSize();
}

template<class type, int baseBlockSize, int minBlockSize, memTag_t _tag_>
ID_INLINE void idDynamicBlockAlloc<type, baseBlockSize, minBlockSize, _tag_>::UnlinkFreeInternal( idDynamicBlock<type>* block )
{
	freeTree.Remove( block->node );
	block->node = NULL;
	numFreeBlocks--;
	freeBlockMemory -= block->GetSize();
}

template<class type, int baseBlockSize, int minBlockSize, memTag_t _tag_>
void idDynamicBlockAlloc<type, baseBlockSize, minBlockSize, _tag_>::CheckMemory() const
{
	idDynamicBlock<type>* block;

	for( block = firstBlock; block != NULL; block = block->next )
	{
		// make sure the block is properly linked
		if( block->prev == NULL )
		{
			assert( firstBlock == block );
		}
		else
		{
			assert( block->prev->next == block );
		}
		if( block->next == NULL )
		{
			assert( lastBlock == block );
		}
		else
		{
			assert( block->next->prev == block );
		}
	}
}

#endif /* !__HEAP_H__ */
