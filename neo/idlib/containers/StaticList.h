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

#ifndef __STATICLIST_H__
#define __STATICLIST_H__

#include "List.h"

/*
===============================================================================

	Static list template
	A non-growing, memset-able list using no memory allocation.

===============================================================================
*/

template<class type, int size>
class idStaticList
{
public:

	idStaticList();
	idStaticList( const idStaticList<type, size>& other );
	~idStaticList<type, size>();

	void				Clear();										// marks the list as empty.  does not deallocate or intialize data.
	int					Num() const;									// returns number of elements in list
	int					Max() const;									// returns the maximum number of elements in the list
	void				SetNum( int newnum );								// set number of elements in list

	// sets the number of elements in list and initializes any newly allocated elements to the given value
	void				SetNum( int newNum, const type& initValue );

	size_t				Allocated() const;							// returns total size of allocated memory
	size_t				Size() const;									// returns total size of allocated memory including size of list type
	size_t				MemoryUsed() const;							// returns size of the used elements in the list

	const type& 		operator[]( int index ) const;
	type& 				operator[]( int index );

	type* 				Ptr();										// returns a pointer to the list
	const type* 		Ptr() const;									// returns a pointer to the list
	type* 				Alloc();										// returns reference to a new data element at the end of the list.  returns NULL when full.
	int					Append( const type& obj );							// append element
	int					Append( const idStaticList<type, size>& other );		// append list
	int					AddUnique( const type& obj );						// add unique element
	int					Insert( const type& obj, int index = 0 );				// insert the element at the given index
	int					FindIndex( const type& obj ) const;				// find the index for the given element
	type* 				Find( type const& obj ) const;						// find pointer to the given element
	int					FindNull() const;								// find the index for the first NULL pointer in the list
	int					IndexOf( const type* obj ) const;					// returns the index for the pointer to an element in the list
	bool				RemoveIndex( int index );							// remove the element at the given index
	bool				RemoveIndexFast( int index );							// remove the element at the given index
	bool				Remove( const type& obj );							// remove the element
	void				Swap( idStaticList<type, size>& other );				// swap the contents of the lists
	void				DeleteContents( bool clear );						// delete the contents of the list

	void				Sort( const idSort<type>& sort = idSort_QuickDefault<type>() );

private:
	int					num;
	type 				list[ size ];

private:
	// resizes list to the given number of elements
	void				Resize( int newsize );
};

/*
================
idStaticList<type,size>::idStaticList()
================
*/
template<class type, int size>
ID_INLINE idStaticList<type, size>::idStaticList()
{
	num = 0;
}

/*
================
idStaticList<type,size>::idStaticList( const idStaticList<type,size> &other )
================
*/
template<class type, int size>
ID_INLINE idStaticList<type, size>::idStaticList( const idStaticList<type, size>& other )
{
	*this = other;
}

/*
================
idStaticList<type,size>::~idStaticList<type,size>
================
*/
template<class type, int size>
ID_INLINE idStaticList<type, size>::~idStaticList()
{
}

/*
================
idStaticList<type,size>::Clear

Sets the number of elements in the list to 0.  Assumes that type automatically handles freeing up memory.
================
*/
template<class type, int size>
ID_INLINE void idStaticList<type, size>::Clear()
{
	num	= 0;
}

/*
========================
idList<_type_,_tag_>::Sort

Performs a QuickSort on the list using the supplied sort algorithm.

Note:	The data is merely moved around the list, so any pointers to data within the list may
		no longer be valid.
========================
*/
template< class type, int size >
ID_INLINE void idStaticList<type, size>::Sort( const idSort<type>& sort )
{
	/*  if( list == NULL )  */
	if( Num() <= 0 )                // SRS - Instead of checking this->list for NULL, check this->Num() for empty list
	{
		return;
	}
	sort.Sort( Ptr(), Num() );
}

/*
================
idStaticList<type,size>::DeleteContents

Calls the destructor of all elements in the list.  Conditionally frees up memory used by the list.
Note that this only works on lists containing pointers to objects and will cause a compiler error
if called with non-pointers.  Since the list was not responsible for allocating the object, it has
no information on whether the object still exists or not, so care must be taken to ensure that
the pointers are still valid when this function is called.  Function will set all pointers in the
list to NULL.
================
*/
template<class type, int size>
ID_INLINE void idStaticList<type, size>::DeleteContents( bool clear )
{
	int i;

	for( i = 0; i < num; i++ )
	{
		delete list[ i ];
		list[ i ] = NULL;
	}

	if( clear )
	{
		Clear();
	}
	else
	{
		memset( list, 0, sizeof( list ) );
	}
}

/*
================
idStaticList<type,size>::Num

Returns the number of elements currently contained in the list.
================
*/
template<class type, int size>
ID_INLINE int idStaticList<type, size>::Num() const
{
	return num;
}

/*
================
idStaticList<type,size>::Num

Returns the maximum number of elements in the list.
================
*/
template<class type, int size>
ID_INLINE int idStaticList<type, size>::Max() const
{
	return size;
}

/*
================
idStaticList<type>::Allocated
================
*/
template<class type, int size>
ID_INLINE size_t idStaticList<type, size>::Allocated() const
{
	return size * sizeof( type );
}

/*
================
idStaticList<type>::Size
================
*/
template<class type, int size>
ID_INLINE size_t idStaticList<type, size>::Size() const
{
	return sizeof( idStaticList<type, size> ) + Allocated();
}

/*
================
idStaticList<type,size>::Num
================
*/
template<class type, int size>
ID_INLINE size_t idStaticList<type, size>::MemoryUsed() const
{
	return num * sizeof( list[ 0 ] );
}

/*
================
idStaticList<type,size>::SetNum

Set number of elements in list.
================
*/
template<class type, int size>
ID_INLINE void idStaticList<type, size>::SetNum( int newnum )
{
	assert( newnum >= 0 );
	assert( newnum <= size );
	num = newnum;
}

/*
========================
idStaticList<_type_,_tag_>::SetNum
========================
*/
template< class type, int size >
ID_INLINE void idStaticList<type, size>::SetNum( int newNum, const type& initValue )
{
	assert( newNum >= 0 );
	newNum = Min( newNum, size );
	assert( newNum <= size );
	for( int i = num; i < newNum; i++ )
	{
		list[i] = initValue;
	}
	num = newNum;
}

/*
================
idStaticList<type,size>::operator[] const

Access operator.  Index must be within range or an assert will be issued in debug builds.
Release builds do no range checking.
================
*/
template<class type, int size>
ID_INLINE const type& idStaticList<type, size>::operator[]( int index ) const
{
	assert( index >= 0 );
	assert( index < num );

	return list[ index ];
}

/*
================
idStaticList<type,size>::operator[]

Access operator.  Index must be within range or an assert will be issued in debug builds.
Release builds do no range checking.
================
*/
template<class type, int size>
ID_INLINE type& idStaticList<type, size>::operator[]( int index )
{
	assert( index >= 0 );
	assert( index < num );

	return list[ index ];
}

/*
================
idStaticList<type,size>::Ptr

Returns a pointer to the begining of the array.  Useful for iterating through the list in loops.

Note: may return NULL if the list is empty.

FIXME: Create an iterator template for this kind of thing.
================
*/
template<class type, int size>
ID_INLINE type* idStaticList<type, size>::Ptr()
{
	return &list[ 0 ];
}

/*
================
idStaticList<type,size>::Ptr

Returns a pointer to the begining of the array.  Useful for iterating through the list in loops.

Note: may return NULL if the list is empty.

FIXME: Create an iterator template for this kind of thing.
================
*/
template<class type, int size>
ID_INLINE const type* idStaticList<type, size>::Ptr() const
{
	return &list[ 0 ];
}

/*
================
idStaticList<type,size>::Alloc

Returns a pointer to a new data element at the end of the list.
================
*/
template<class type, int size>
ID_INLINE type* idStaticList<type, size>::Alloc()
{
	if( num >= size )
	{
		return NULL;
	}

	return &list[ num++ ];
}

/*
================
idStaticList<type,size>::Append

Increases the size of the list by one element and copies the supplied data into it.

Returns the index of the new element, or -1 when list is full.
================
*/
template<class type, int size>
ID_INLINE int idStaticList<type, size>::Append( type const& obj )
{
	assert( num < size );
	if( num < size )
	{
		list[ num ] = obj;
		num++;
		return num - 1;
	}

	return -1;
}


/*
================
idStaticList<type,size>::Insert

Increases the size of the list by at leat one element if necessary
and inserts the supplied data into it.

Returns the index of the new element, or -1 when list is full.
================
*/
template<class type, int size>
ID_INLINE int idStaticList<type, size>::Insert( type const& obj, int index )
{
	assert( num < size );
	if( num >= size )
	{
		return -1;
	}

	assert( index >= 0 );
	if( index < 0 )
	{
		index = 0;
	}
	else if( index > num )
	{
		index = num;
	}

	for( int i = num; i > index; --i )
	{
		list[i] = list[i - 1];
	}

	num++;
	list[index] = obj;
	return index;
}

/*
================
idStaticList<type,size>::Append

adds the other list to this one

Returns the size of the new combined list
================
*/
template<class type, int size>
ID_INLINE int idStaticList<type, size>::Append( const idStaticList<type, size>& other )
{
	int n = other.Num();

	if( num + n > size )
	{
		n = size - num;
	}
	for( int i = 0; i < n; i++ )
	{
		list[i + num] = other.list[i];
	}
	num += n;
	return Num();
}

/*
================
idStaticList<type,size>::AddUnique

Adds the data to the list if it doesn't already exist.  Returns the index of the data in the list.
================
*/
template<class type, int size>
ID_INLINE int idStaticList<type, size>::AddUnique( type const& obj )
{
	int index = FindIndex( obj );
	if( index < 0 )
	{
		index = Append( obj );
	}

	return index;
}

/*
================
idStaticList<type,size>::FindIndex

Searches for the specified data in the list and returns it's index.  Returns -1 if the data is not found.
================
*/
template<class type, int size>
ID_INLINE int idStaticList<type, size>::FindIndex( type const& obj ) const
{
	for( int i = 0; i < num; i++ )
	{
		if( list[ i ] == obj )
		{
			return i;
		}
	}

	// Not found
	return -1;
}

/*
================
idStaticList<type,size>::Find

Searches for the specified data in the list and returns it's address. Returns NULL if the data is not found.
================
*/
template<class type, int size>
ID_INLINE type* idStaticList<type, size>::Find( type const& obj ) const
{
	int i = FindIndex( obj );
	if( i >= 0 )
	{
		return ( type* ) &list[ i ];
	}

	return NULL;
}

/*
================
idStaticList<type,size>::FindNull

Searches for a NULL pointer in the list.  Returns -1 if NULL is not found.

NOTE: This function can only be called on lists containing pointers. Calling it
on non-pointer lists will cause a compiler error.
================
*/
template<class type, int size>
ID_INLINE int idStaticList<type, size>::FindNull() const
{
	for( int i = 0; i < num; i++ )
	{
		if( list[ i ] == NULL )
		{
			return i;
		}
	}

	// Not found
	return -1;
}

/*
================
idStaticList<type,size>::IndexOf

Takes a pointer to an element in the list and returns the index of the element.
This is NOT a guarantee that the object is really in the list.
Function will assert in debug builds if pointer is outside the bounds of the list,
but remains silent in release builds.
================
*/
template<class type, int size>
ID_INLINE int idStaticList<type, size>::IndexOf( type const* objptr ) const
{
	int index;

	index = objptr - list;

	assert( index >= 0 );
	assert( index < num );

	return index;
}

/*
================
idStaticList<type,size>::RemoveIndex

Removes the element at the specified index and moves all data following the element down to fill in the gap.
The number of elements in the list is reduced by one.  Returns false if the index is outside the bounds of the list.
Note that the element is not destroyed, so any memory used by it may not be freed until the destruction of the list.
================
*/
template<class type, int size>
ID_INLINE bool idStaticList<type, size>::RemoveIndex( int index )
{
	int i;

	assert( index >= 0 );
	assert( index < num );

	if( ( index < 0 ) || ( index >= num ) )
	{
		return false;
	}

	num--;
	for( i = index; i < num; i++ )
	{
		list[ i ] = std::move( list[ i + 1 ] );
	}

	return true;
}

/*
========================
idStaticList<_type_,_tag_>::RemoveIndexFast

Removes the element at the specified index and moves the last element into its spot, rather
than moving the whole array down by one. Of course, this doesn't maintain the order of
elements! The number of elements in the list is reduced by one.

return:	bool	- false if the data is not found in the list.

NOTE:	The element is not destroyed, so any memory used by it may not be freed until the
		destruction of the list.
========================
*/
template< typename _type_, int size >
ID_INLINE bool idStaticList<_type_, size>::RemoveIndexFast( int index )
{

	if( ( index < 0 ) || ( index >= num ) )
	{
		return false;
	}

	num--;
	if( index != num )
	{
		list[ index ] = list[ num ];
	}

	return true;
}

/*
================
idStaticList<type,size>::Remove

Removes the element if it is found within the list and moves all data following the element down to fill in the gap.
The number of elements in the list is reduced by one.  Returns false if the data is not found in the list.  Note that
the element is not destroyed, so any memory used by it may not be freed until the destruction of the list.
================
*/
template<class type, int size>
ID_INLINE bool idStaticList<type, size>::Remove( type const& obj )
{
	int index;

	index = FindIndex( obj );
	if( index >= 0 )
	{
		return RemoveIndex( index );
	}

	return false;
}

/*
================
idStaticList<type,size>::Swap

Swaps the contents of two lists
================
*/
template<class type, int size>
ID_INLINE void idStaticList<type, size>::Swap( idStaticList<type, size>& other )
{
	idStaticList<type, size> temp = *this;
	*this = other;
	other = temp;
}

// debug tool to find uses of idlist that are dynamically growing
// Ideally, most lists on shipping titles will explicitly set their size correctly
// instead of relying on allocate-on-add
void BreakOnListGrowth();
void BreakOnListDefault();

/*
========================
idStaticList<_type_,_tag_>::Resize

Allocates memory for the amount of elements requested while keeping the contents intact.
Contents are copied using their = operator so that data is correctly instantiated.
========================
*/
template< class type, int size >
ID_INLINE void idStaticList<type, size>::Resize( int newsize )
{

	assert( newsize >= 0 );

	// free up the list if no data is being reserved
	if( newsize <= 0 )
	{
		Clear();
		return;
	}

	if( newsize == size )
	{
		// not changing the size, so just exit
		return;
	}

	assert( newsize < size );
	return;
}
#endif /* !__STATICLIST_H__ */
