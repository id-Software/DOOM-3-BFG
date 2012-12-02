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
#ifndef __SOFTWARECACHE_H__
#define __SOFTWARECACHE_H__

#ifdef _MSC_VER // DG: #pragma warning is MSVC specific
#pragma warning( disable : 4324 )	// structure was padded due to __declspec(align())
#endif
/*
================================================================================================

On-Demand Streamed Objects and Arrays

idODSObject					// DMA in a single object
idODSCachedObject			// DMA in a single object through a software cache
idODSArray					// DMA in an array with objects
idODSIndexedArray			// DMA gather from an array with objects
idODSStreamedArray			// overlapped DMA streaming of an array with objects
idODSStreamedIndexedArray	// overlapped DMA gather from an array with objects

On the SPU the 'idODSObject' streams the data into temporary memory using the DMA controller
and the object constructor immediately waits for the DMA transfer to complete. In other words
there is no caching and every random memory access incurs full memory latency. This should be
used to stream in objects that are only used once at unpredictable times.

The 'idODSCachedObject' uses an object based software cache on the SPU which is useful for
streaming in objects that may be used repeatedly or which usage can be predicted allowing
the objects to be prefetched.

	class idMyType {};
	class idMyCache : public idSoftwareCache< idMyType, 8, 4 > {};
	idMyCache myCache;
	idMyType * myPtr;
	idODSCachedObject< idMyType, idMyCache > myODS( myPtr, myCache );

The 'idSoftwareCache' implements a Prefetch() function that can be used to prefetch whole
objects into the cache well before they are needed. However, any idODSObject, idODSArray,
idODSIndexedArray etc. after calling the Prefetch() function will have to wait for the
prefetch to complete. In other words, make sure there is enough "work" done in between
a Prefetch() call and the first next idODS* object.

The 'idODSArray' streams in a block of objects that are tightly packed in memory.

The 'idODSIndexedArray' is used to gather a number of objects that are not necessarily
contiguous in memory. On the SPU a DMA-list is used in the 'idODSIndexedArray' constructor
to efficiently gather all the objects.

The 'idODSStreamedArray' is used for sequentially reading a large input array. Overlapped
streaming is used where one batch of array elements can be accessed while the next batch
is being streamed in.

The 'idODSStreamedIndexedArray' is used for gathering elements from an array using a
sequentially read index. Overlapped streaming is used for both the index and the array
elements where one batch of array elements can be accessed while the next batch of
indices/array elements is being streamed in.

Outside the SPU, data is never copied to temporary memory because this would cause
significant load-hit-store penalties. Instead, the object constructor issues prefetch
instructions where appropriate and only maintains pointers to the actual data. In the
case of 'idODSObject' or 'idODSCachedObject' the class is no more than a simple wrapper
of a pointer and the class should completely compile away with zero overhead.

COMMON MISTAKES:

1. When using ODS objects do not forget to set the "globalDmaTag" that is used to issue
   and wait for DMAs.

   void cellSpursJobMain2( CellSpursJobContext2 * stInfo, CellSpursJob256 * job ) {
      globalDmaTag = stInfo->dmaTag;	// for ODS objects
   }

2. ODS objects can consume quite a bit of stack space. You may have to increase the SPU job
   stack size. For instance:

   job->header.sizeStack = SPURS_QUADWORDS( 16 * 1024 );   // the ODS objects get pretty large

   Make sure you measure the size of each ODS object and if there are recursive functions
   using ODS objects make sure the recursion is bounded. When the stack overflows the scratch
   and output memory may get overwritten and the results will be undefined. Finding stack
   overflows is painful.

3. While you can setup a regular DMA list entry to use a NULL pointer with zero size, do not use
   a NULL pointer for a cache DMA list entry. This confuses SPURS and can cause your SPU binary
   to get corrupted.

================================================================================================
*/

extern uint32 globalDmaTag;

#define MAX_DMA_SIZE					( 1 << 14 )
#define ODS_ROUND16( x )				( ( x + 15 ) & ~15 )

enum streamBufferType_t
{
	SBT_DOUBLE		= 2,
	SBT_QUAD		= 4
};


/*
================================================================================================

	non-SPU code

================================================================================================
*/

/*
================================================
idSoftwareCache
================================================
*/
template< typename _type_, int _entries_ = 8, int _associativity_ = 4, bool aligned = false >
class ALIGNTYPE128 idSoftwareCache
{
public:
	void Prefetch( const _type_ * obj )
	{
		::Prefetch( obj, 0 );
	}
};

/*
================================================
idODSObject
================================================
*/
template< typename _type_ >
class idODSObject
{
public:
	idODSObject( const _type_ * obj ) : objectPtr( obj ) {}
	operator const _type_& () const
	{
		return *objectPtr;
	}
	const _type_* operator->() const
	{
		return objectPtr;
	}
	const _type_& Get() const
	{
		return *objectPtr;
	}
	const _type_* Ptr() const
	{
		return objectPtr;
	}
	const _type_* OriginalPtr() const
	{
		return objectPtr;
	}
	
private:
	const _type_* objectPtr;
};

/*
================================================
idODSCachedObject
================================================
*/
template< typename _type_, typename _cache_ >
class idODSCachedObject
{
public:
	idODSCachedObject( const _type_ * obj, _cache_ & cache ) : objectPtr( obj ) {}
	operator const _type_& () const
	{
		return *objectPtr;
	}
	const _type_* operator->() const
	{
		return objectPtr;
	}
	const _type_& Get() const
	{
		return *objectPtr;
	}
	const _type_* Ptr() const
	{
		return objectPtr;
	}
	const _type_* OriginalPtr() const
	{
		return objectPtr;
	}
	
private:
	const _type_* objectPtr;
};

/*
================================================
idODSArray
================================================
*/
template< typename _type_, int max >
class idODSArray
{
public:
	idODSArray( const _type_ * array, int num ) : arrayPtr( array ), arrayNum( num )
	{
		assert( num <= max );
		Prefetch( array, 0 );
	}
	const _type_& operator[]( int index ) const
	{
		assert( index >= 0 && index < arrayNum );
		return arrayPtr[index];
	}
	const _type_* Ptr() const
	{
		return arrayPtr;
	}
	const int Num() const
	{
		return arrayNum;
	}
	
private:
	const _type_* arrayPtr;
	int arrayNum;
};

/*
================================================
idODSIndexedArray
================================================
*/
template< typename _elemType_, typename _indexType_, int max >
class idODSIndexedArray
{
public:
	idODSIndexedArray( const _elemType_ * array, const _indexType_ * index, int num ) : arrayNum( num )
	{
		assert( num <= max );
		for( int i = 0; i < num; i++ )
		{
			Prefetch( arrayPtr, abs( index[i] ) * sizeof( _elemType_ ) );
			arrayPtr[i] = array + abs( index[i] );
		}
	}
	const _elemType_& operator[]( int index ) const
	{
		assert( index >= 0 && index < arrayNum );
		return * arrayPtr[index];
	}
	void ReplicateUpToMultipleOfFour()
	{
		assert( ( max & 3 ) == 0 );
		while( ( arrayNum & 3 ) != 0 )
		{
			arrayPtr[arrayNum++] = arrayPtr[0];
		}
	}
	
private:
	const _elemType_* arrayPtr[max];
	int arrayNum;
};

/*
================================================
idODSStreamedOutputArray
================================================
*/
template< typename _type_, int _bufferSize_ >
class ALIGNTYPE16 idODSStreamedOutputArray
{
public:
	idODSStreamedOutputArray( _type_ * array, int* numElements, int maxElements ) :
		localNum( 0 ),
		outArray( array ),
		outNum( numElements ),
		outMax( maxElements )
	{
		compile_time_assert( CONST_ISPOWEROFTWO( _bufferSize_ ) );
		compile_time_assert( ( ( _bufferSize_ * sizeof( _type_ ) ) & 15 ) == 0 );
		compile_time_assert( _bufferSize_ * sizeof( _type_ ) < MAX_DMA_SIZE );
		assert_16_byte_aligned( array );
	}
	~idODSStreamedOutputArray()
	{
		*outNum = localNum;
	}
	
	int			Num() const
	{
		return localNum;
	}
	void		Append( _type_ element )
	{
		assert( localNum < outMax );
		outArray[localNum++] = element;
	}
	_type_& 	Alloc()
	{
		assert( localNum < outMax );
		return outArray[localNum++];
	}
	
private:
	int			localNum;
	_type_* 	outArray;
	int* 		outNum;
	int			outMax;
};

/*
================================================
idODSStreamedArray
================================================
*/
template< typename _type_, int _bufferSize_, streamBufferType_t _sbt_ = SBT_DOUBLE, int _roundUpToMultiple_ = 1 >
class ALIGNTYPE16 idODSStreamedArray
{
public:
	idODSStreamedArray( const _type_ * array, const int numElements ) :
		cachedArrayStart( 0 ),
		cachedArrayEnd( 0 ),
		streamArrayEnd( 0 ),
		inArray( array ),
		inArrayNum( numElements ),
		inArrayNumRoundedUp( numElements )
	{
		compile_time_assert( CONST_ISPOWEROFTWO( _bufferSize_ ) );
		compile_time_assert( ( ( _bufferSize_ * sizeof( _type_ ) ) & 15 ) == 0 );
		compile_time_assert( _bufferSize_ * sizeof( _type_ ) < MAX_DMA_SIZE );
		compile_time_assert( _roundUpToMultiple_ >= 1 );
		assert_16_byte_aligned( array );
		assert( ( uintptr_t )array > _bufferSize_ * sizeof( _type_ ) );
		// Fetch the first batch of elements.
		FetchNextBatch();
		// Calculate the rounded up size here making the mod effectively for free because we have to wait
		// for memory access anyway while the above FetchNextBatch() does not need the rounded up size yet.
		inArrayNumRoundedUp += _roundUpToMultiple_ - 1;
		inArrayNumRoundedUp -= inArrayNumRoundedUp % ( ( _roundUpToMultiple_ > 1 ) ? _roundUpToMultiple_ : 1 );
	}
	~idODSStreamedArray()
	{
		// Flush the accessible part of the array.
		FlushArray( inArray, cachedArrayStart * sizeof( _type_ ), cachedArrayEnd * sizeof( _type_ ) );
	}
	
	// Fetches a new batch of array elements and returns the first index after this new batch.
	// After calling this, the elements starting at the index returned by the previous call to
	// FetchNextBach() (or zero if not yet called) up to (excluding) the index returned by
	// this call to FetchNextBatch() can be accessed through the [] operator. When quad-buffering,
	// the elements starting at the index returned by the second-from-last call to FetchNextBatch()
	// can still be accessed. This is useful when the algorithm needs to successively access
	// an odd number of elements at the same time that may cross a single buffer boundary.
	int				FetchNextBatch()
	{
		// If not everything has been streamed already.
		if( cachedArrayEnd < inArrayNum )
		{
			cachedArrayEnd = streamArrayEnd;
			cachedArrayStart = Max( cachedArrayEnd - _bufferSize_ * ( _sbt_ - 1 ), 0 );
			
			// Flush the last batch of elements that is no longer accessible.
			FlushArray( inArray, ( cachedArrayStart - _bufferSize_ ) * sizeof( _type_ ), cachedArrayStart * sizeof( _type_ ) );
			
			// Prefetch the next batch of elements.
			if( streamArrayEnd < inArrayNum )
			{
				streamArrayEnd = Min( streamArrayEnd + _bufferSize_, inArrayNum );
				for( unsigned int offset = cachedArrayEnd * sizeof( _type_ ); offset < streamArrayEnd * sizeof( _type_ ); offset += CACHE_LINE_SIZE )
				{
					Prefetch( inArray, offset );
				}
			}
		}
		return ( cachedArrayEnd == inArrayNum ) ? inArrayNumRoundedUp : cachedArrayEnd;
	}
	
	// Provides access to the elements starting at the index returned by the next-to-last call
	// to FetchNextBach() (or zero if only called once so far) up to (excluding) the index
	// returned by the last call to FetchNextBatch(). When quad-buffering, the elements starting
	// at the index returned by the second-from-last call to FetchNextBatch() can still be accessed.
	// This is useful when the algorithm needs to successively access an odd number of elements
	// at the same time that may cross a single buffer boundary.
	const _type_& 	operator[]( int index ) const
	{
		assert( ( index >= cachedArrayStart && index < cachedArrayEnd ) || ( cachedArrayEnd == inArrayNum && index >= inArrayNum && index < inArrayNumRoundedUp ) );
		if( _roundUpToMultiple_ > 1 )
		{
			index &= ( index - inArrayNum ) >> 31;
		}
		return inArray[index];
	}
	
private:
	int				cachedArrayStart;
	int				cachedArrayEnd;
	int				streamArrayEnd;
	const _type_* 	inArray;
	int				inArrayNum;
	int				inArrayNumRoundedUp;
	
	static void FlushArray( const void* flushArray, int flushStart, int flushEnd )
	{
#if 0
		// arrayFlushBase is rounded up so we do not flush anything before the array.
		// arrayFlushStart is rounded down so we start right after the last cache line that was previously flushed.
		// arrayFlushEnd is rounded down so we do not flush a cache line that holds data that may still be partially
		// accessible or a cache line that stretches beyond the end of the array.
		const uintptr_t arrayAddress = ( uintptr_t )flushArray;
		const uintptr_t arrayFlushBase = ( arrayAddress + CACHE_LINE_SIZE - 1 ) & ~( CACHE_LINE_SIZE - 1 );
		const uintptr_t arrayFlushStart = ( arrayAddress + flushStart ) & ~( CACHE_LINE_SIZE - 1 );
		const uintptr_t arrayFlushEnd = ( arrayAddress + flushEnd ) & ~( CACHE_LINE_SIZE - 1 );
		for( uintptr_t offset = Max( arrayFlushBase, arrayFlushStart ); offset < arrayFlushEnd; offset += CACHE_LINE_SIZE )
		{
			FlushCacheLine( flushArray, offset - arrayAddress );
		}
#endif
	}
};

/*
================================================
idODSStreamedIndexedArray

For gathering elements from an array using a sequentially read index.
This uses overlapped streaming for both the index and the array elements
where one batch of indices and/or array elements can be accessed while
the next batch is being streamed in.

NOTE: currently the size of array elements must be a multiple of 16 bytes.
An index with offsets and more complex logic is needed to support other sizes.
================================================
*/
template< typename _elemType_, typename _indexType_, int _bufferSize_, streamBufferType_t _sbt_ = SBT_DOUBLE, int _roundUpToMultiple_ = 1 >
class ALIGNTYPE16 idODSStreamedIndexedArray
{
public:
	idODSStreamedIndexedArray( const _elemType_ * array, const int numElements, const _indexType_ * index, const int numIndices ) :
		cachedArrayStart( 0 ),
		cachedArrayEnd( 0 ),
		streamArrayEnd( 0 ),
		cachedIndexStart( 0 ),
		cachedIndexEnd( 0 ),
		streamIndexEnd( 0 ),
		inArray( array ),
		inArrayNum( numElements ),
		inIndex( index ),
		inIndexNum( numIndices ),
		inIndexNumRoundedUp( numIndices )
	{
		compile_time_assert( CONST_ISPOWEROFTWO( _bufferSize_ ) );
		compile_time_assert( ( ( _bufferSize_ * sizeof( _indexType_ ) ) & 15 ) == 0 );
		compile_time_assert( _bufferSize_ * sizeof( _indexType_ ) < MAX_DMA_SIZE );
		compile_time_assert( _bufferSize_ * sizeof( _elemType_ ) < MAX_DMA_SIZE );
		compile_time_assert( ( sizeof( _elemType_ ) & 15 ) == 0 );	// to avoid complexity due to cellDmaListGet
		compile_time_assert( _roundUpToMultiple_ >= 1 );
		assert_16_byte_aligned( index );
		assert_16_byte_aligned( array );
		assert( ( uintptr_t )index > _bufferSize_ * sizeof( _indexType_ ) );
		assert( ( uintptr_t )array > _bufferSize_ * sizeof( _elemType_ ) );
		// Fetch the first batch of indices.
		FetchNextBatch();
		// Fetch the first batch of elements and the next batch of indices.
		FetchNextBatch();
		// Calculate the rounded up size here making the mod effectively for free because we have to wait
		// for memory access anyway while the above FetchNextBatch() do not need the rounded up size yet.
		inIndexNumRoundedUp += _roundUpToMultiple_ - 1;
		inIndexNumRoundedUp -= inIndexNumRoundedUp % ( ( _roundUpToMultiple_ > 1 ) ? _roundUpToMultiple_ : 1 );
	}
	~idODSStreamedIndexedArray()
	{
		// Flush the accessible part of the index.
		FlushArray( inIndex, cachedIndexStart * sizeof( _indexType_ ), cachedIndexEnd * sizeof( _indexType_ ) );
		// Flush the accessible part of the array.
		FlushArray( inArray, cachedArrayStart * sizeof( _elemType_ ), cachedArrayEnd * sizeof( _elemType_ ) );
	}
	
	// Fetches a new batch of array elements and returns the first index after this new batch.
	// After calling this, the elements starting at the index returned by the previous call to
	// FetchNextBach() (or zero if not yet called) up to (excluding) the index returned by
	// this call to FetchNextBatch() can be accessed through the [] operator. When quad-buffering,
	// the elements starting at the index returned by the second-from-last call to FetchNextBatch()
	// can still be accessed. This is useful when the algorithm needs to successively access
	// an odd number of elements at the same time that may cross a single buffer boundary.
	int				FetchNextBatch()
	{
		// If not everything has been streamed already.
		if( cachedArrayEnd < inIndexNum )
		{
			if( streamIndexEnd > 0 )
			{
				cachedArrayEnd = streamArrayEnd;
				cachedArrayStart = Max( cachedArrayEnd - _bufferSize_ * ( _sbt_ - 1 ), 0 );
				cachedIndexEnd = streamIndexEnd;
				cachedIndexStart = Max( cachedIndexEnd - _bufferSize_ * ( _sbt_ - 1 ), 0 );
				
				// Flush the last batch of indices that are no longer accessible.
				FlushArray( inIndex, ( cachedIndexStart - _bufferSize_ ) * sizeof( _indexType_ ), cachedIndexStart * sizeof( _indexType_ ) );
				// Flush the last batch of elements that is no longer accessible.
				FlushArray( inArray, ( cachedArrayStart - _bufferSize_ ) * sizeof( _elemType_ ), cachedArrayStart * sizeof( _elemType_ ) );
				
				// Prefetch the next batch of elements.
				if( streamArrayEnd < inIndexNum )
				{
					streamArrayEnd = cachedIndexEnd;
					for( int i = cachedArrayEnd; i < streamArrayEnd; i++ )
					{
						assert( i >= cachedIndexStart && i < cachedIndexEnd );
						assert( inIndex[i] >= 0 && inIndex[i] < inArrayNum );
						
						Prefetch( inArray, inIndex[i] * sizeof( _elemType_ ) );
					}
				}
			}
			
			// Prefetch the next batch of indices.
			if( streamIndexEnd < inIndexNum )
			{
				streamIndexEnd = Min( streamIndexEnd + _bufferSize_, inIndexNum );
				for( unsigned int offset = cachedIndexEnd * sizeof( _indexType_ ); offset < streamIndexEnd * sizeof( _indexType_ ); offset += CACHE_LINE_SIZE )
				{
					Prefetch( inIndex, offset );
				}
			}
		}
		return ( cachedArrayEnd == inIndexNum ) ? inIndexNumRoundedUp : cachedArrayEnd;
	}
	
	// Provides access to the elements starting at the index returned by the next-to-last call
	// to FetchNextBach() (or zero if only called once so far) up to (excluding) the index
	// returned by the last call to FetchNextBatch(). When quad-buffering, the elements starting
	// at the index returned by the second-from-last call to FetchNextBatch() can still be accessed.
	// This is useful when the algorithm needs to successively access an odd number of elements
	// at the same time that may cross a single buffer boundary.
	const _elemType_& operator[]( int index ) const
	{
		assert( ( index >= cachedArrayStart && index < cachedArrayEnd ) || ( cachedArrayEnd == inIndexNum && index >= inIndexNum && index < inIndexNumRoundedUp ) );
		if( _roundUpToMultiple_ > 1 )
		{
			index &= ( index - inIndexNum ) >> 31;
		}
		return inArray[inIndex[index]];
	}
	
private:
	int					cachedArrayStart;
	int					cachedArrayEnd;
	int					streamArrayEnd;
	int					cachedIndexStart;
	int					cachedIndexEnd;
	int					streamIndexEnd;
	const _elemType_* 	inArray;
	int					inArrayNum;
	const _indexType_* 	inIndex;
	int					inIndexNum;
	int					inIndexNumRoundedUp;
	
	static void FlushArray( const void* flushArray, int flushStart, int flushEnd )
	{
#if 0
		// arrayFlushBase is rounded up so we do not flush anything before the array.
		// arrayFlushStart is rounded down so we start right after the last cache line that was previously flushed.
		// arrayFlushEnd is rounded down so we do not flush a cache line that holds data that may still be partially
		// accessible or a cache line that stretches beyond the end of the array.
		const uintptr_t arrayAddress = ( uintptr_t )flushArray;
		const uintptr_t arrayFlushBase = ( arrayAddress + CACHE_LINE_SIZE - 1 ) & ~( CACHE_LINE_SIZE - 1 );
		const uintptr_t arrayFlushStart = ( arrayAddress + flushStart ) & ~( CACHE_LINE_SIZE - 1 );
		const uintptr_t arrayFlushEnd = ( arrayAddress + flushEnd ) & ~( CACHE_LINE_SIZE - 1 );
		for( uintptr_t offset = Max( arrayFlushBase, arrayFlushStart ); offset < arrayFlushEnd; offset += CACHE_LINE_SIZE )
		{
			FlushCacheLine( flushArray, offset - arrayAddress );
		}
#endif
	}
};


#endif // !__SOFTWARECACHE_H__
