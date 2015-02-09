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

#ifndef __HASHTABLE_H__
#define __HASHTABLE_H__

/*
================================================================================================
idHashNodeT is a generic node for a HashTable. It is specialized by the
StringHashNode and CStringHashNode template classes.
================================================================================================
*/
template< typename _key_, class _value_ >
class idHashNodeT
{
public:
	idHashNodeT()
		:	next( NULL )
	{
	}
	
	idHashNodeT( const _key_ & key, const _value_ & value, idHashNodeT* next )
		:	key( key ),
		  value( value ),
		  next( next )
	{
	}
	
	static int	GetHash( const _key_ & key, const int tableMask )
	{
		return key & tableMask;
	}
	
	static int	Compare( const _key_ & key1, const _key_ & key2 )
	{
		if( key1 < key2 )
		{
			return -1;
		}
		else if( key1 > key2 )
		{
			return 1;
		}
		return 0;
	}
	
public:
	_key_							key;
	_value_							value;
	idHashNodeT< _key_, _value_ >* 	next;
};

/*
================================================
idHashNodeT is a HashNode that provides for partial
specialization for the HashTable, allowing the String class's Cmp function to be used
for inserting values in sorted order.
================================================
*/
template< class _value_ >
class idHashNodeT< idStr, _value_ >
{
public:
	idHashNodeT( const idStr& key, const _value_ & value, idHashNodeT* next )
		:	key( key ),
		  value( value ),
		  next( next )
	{
	}
	
	static int	GetHash( const idStr& key, const int tableMask )
	{
		return ( idStr::Hash( key ) & tableMask );
	}
	
	static int	Compare( const idStr& key1, const idStr& key2 )
	{
		return idStr::Icmp( key1, key2 );
	}
	
public:
	idStr							key;
	_value_							value;
	idHashNodeT< idStr, _value_ >* 	next;
};

/*
================================================
idHashNodeT is a HashNode that provides for a partial specialization
for the HashTable, allowing the String class's Cmp function to
be used for inserting values in sorted order. It also ensures that a copy of the the key is
stored in a String (to more closely model the original implementation of the HashTable).
================================================
*/
template< class _value_ >
class idHashNodeT< const char*, _value_ >
{
public:
	idHashNodeT( const char* const& key, const _value_ & value, idHashNodeT* next )
		:	key( key ),
		  value( value ),
		  next( next )
	{
	}
	
	static int	GetHash( const char* const& key, const int tableMask )
	{
		return ( idStr::Hash( key ) & tableMask );
	}
	
	static int	Compare( const char* const& key1, const char* const& key2 )
	{
		return idStr::Icmp( key1, key2 );
	}
	
public:
	idStr									key;	// char * keys must still get stored in an idStr
	_value_									value;
	idHashNodeT< const char*, _value_ >* 	next;
};

/*
================================================
idHashTableT is a general implementation of a hash table data type. It is
slower than the HashIndex, but it can also be used for LinkedLists and other data structures,
rather than just indexes and arrays.

It uses an arbitrary key type. For String keys, use the StringHashTable template
specialization.
================================================
*/
template< typename _key_, class _value_ >
class idHashTableT
{
public:
	idHashTableT( const int tableSize = 256 );
	idHashTableT( const idHashTableT& other );
	~idHashTableT();
	
	size_t			Allocated() const;
	size_t			Size() const;
	
	_value_& 		Set( const _key_ & key, const _value_ & value );
	
	bool			Get( const _key_ & key, _value_** value = NULL );
	bool			Get( const _key_ & key, const _value_** value = NULL ) const;
	
	bool			Remove( const _key_ & key );
	
	void			Clear();
	void			DeleteContents();
	
	int				Num() const;
	_value_* 		GetIndex( const int index ) const;
	bool			GetIndexKey( const int index, _key_ & key ) const;
	
	int				GetSpread() const;
	
	idHashTableT& 	operator=( const idHashTableT& other );
	
protected:
	void			Copy( const idHashTableT& other );
	
private:
	typedef idHashNodeT< _key_, _value_ > hashnode_t;
	
	hashnode_t**		heads;
	
	int				tableSize;
	int				numEntries;
	int				tableSizeMask;
};

/*
========================
idHashTableT<_key_,_value_>::idHashTableT
========================
*/
template< typename _key_, class _value_ >
ID_INLINE idHashTableT<_key_, _value_>::idHashTableT( const int tableSize )
{
	assert( idMath::IsPowerOfTwo( tableSize ) );
	
	this->tableSize = tableSize;
	
	heads = new( TAG_IDLIB_HASH ) hashnode_t* [ tableSize ];
	memset( heads, 0, sizeof( hashnode_t* ) * tableSize );
	
	numEntries = 0;
	tableSizeMask = tableSize - 1;
}

/*
========================
idHashTableT<_key_,_value_>::idHashTableT
========================
*/
template< typename _key_, class _value_ >
ID_INLINE idHashTableT<_key_, _value_>::idHashTableT( const idHashTableT& other )
{
	Copy( other );
}

/*
========================
idHashTableT<_key_,_value_>::~idHashTableT
========================
*/
template< typename _key_, class _value_ >
ID_INLINE idHashTableT<_key_, _value_>::~idHashTableT()
{
	Clear();
	delete [] heads;
	heads = NULL;
	tableSize = 0;
	tableSizeMask = 0;
	numEntries = 0;
}

/*
========================
idHashTableT<_key_,_value_>::Allocated
========================
*/
template< typename _key_, class _value_ >
ID_INLINE size_t idHashTableT<_key_, _value_>::Allocated() const
{
	return sizeof( heads ) * tableSize + sizeof( hashnode_t* ) * numEntries;
}

/*
========================
idHashTableT<_key_,_value_>::Size
========================
*/
template< typename _key_, class _value_ >
ID_INLINE size_t idHashTableT<_key_, _value_>::Size() const
{
	return sizeof( idHashTableT ) + sizeof( heads ) * tableSize + sizeof( hashnode_t* ) * numEntries;
}

/*
========================
idHashTableT<_key_,_value_>::Set
========================
*/
template< typename _key_, class _value_ >
ID_INLINE _value_& idHashTableT<_key_, _value_>::Set( const _key_ & key, const _value_ & value )
{
	// insert sorted
	int hash = hashnode_t::GetHash( key, tableSizeMask );
	hashnode_t** nextPtr = &( heads[ hash ] );
	hashnode_t* node = * nextPtr;
	for( ;
			node != NULL;
			nextPtr = &( node->next ), node = *nextPtr )
	{
		int s = node->Compare( node->key, key );
		if( s == 0 )
		{
			// return existing hashed item
			node->value = value;
			return node->value;
		}
		if( s > 0 )
		{
			break;
		}
	}
	
	numEntries++;
	
	*nextPtr = new( TAG_IDLIB_HASH ) hashnode_t( key, value, heads[ hash ] );
	( *nextPtr )->next = node;
	return ( *nextPtr )->value;
}

/*
========================
idHashTableT<_key_,_value_>::Get
========================
*/
template< typename _key_, class _value_ >
ID_INLINE bool idHashTableT<_key_, _value_>::Get( const _key_ & key, _value_** value )
{
	int hash = hashnode_t::GetHash( key, tableSizeMask );
	hashnode_t* node = heads[ hash ];
	for( ; node != NULL; node = node->next )
	{
		int s = node->Compare( node->key, key );
		if( s == 0 )
		{
			if( value )
			{
				*value = &node->value;
			}
			return true;
		}
		if( s > 0 )
		{
			break;
		}
	}
	if( value )
	{
		*value = NULL;
	}
	return false;
}

/*
========================
idHashTableT<_key_,_value_>::Get
========================
*/
template< typename _key_, class _value_ >
ID_INLINE bool idHashTableT<_key_, _value_>::Get( const _key_ & key, const _value_** value ) const
{
	int hash = hashnode_t::GetHash( key, tableSizeMask );
	hashnode_t* node = heads[ hash ];
	for( ; node != NULL; node = node->next )
	{
		int s = node->Compare( node->key, key );
		if( s == 0 )
		{
			if( value )
			{
				*value = &node->value;
			}
			return true;
		}
		if( s > 0 )
		{
			break;
		}
	}
	if( value )
	{
		*value = NULL;
	}
	return false;
}

/*
========================
idHashTableT<_key_,_value_>::GetIndex
========================
*/
template< typename _key_, class _value_ >
ID_INLINE _value_* idHashTableT<_key_, _value_>::GetIndex( const int index ) const
{
	if( index < 0 || index > numEntries )
	{
		assert( 0 );
		return NULL;
	}
	
	int count = 0;
	for( int i = 0; i < tableSize; i++ )
	{
		for( hashnode_t* node = heads[ i ]; node != NULL; node = node->next )
		{
			if( count == index )
			{
				return &node->value;
			}
			count++;
		}
	}
	return NULL;
}

/*
========================
idHashTableT<_key_,_value_>::GetIndexKey
========================
*/
template< typename _key_, class _value_ >
ID_INLINE bool idHashTableT<_key_, _value_>::GetIndexKey( const int index, _key_ & key ) const
{
	if( index < 0 || index > numEntries )
	{
		assert( 0 );
		return false;
	}
	
	int count = 0;
	for( int i = 0; i < tableSize; i++ )
	{
		for( hashnode_t* node = heads[ i ]; node != NULL; node = node->next )
		{
			if( count == index )
			{
				key = node->key;
				return true;
			}
			count++;
		}
	}
	return false;
}

/*
========================
idHashTableT<_key_,_value_>::Remove
========================
*/
template< typename _key_, class _value_ >
ID_INLINE bool idHashTableT<_key_, _value_>::Remove( const _key_ & key )
{
	int hash = hashnode_t::GetHash( key, tableSizeMask );
	hashnode_t** head = &heads[ hash ];
	if( *head )
	{
		hashnode_t* prev = NULL;
		hashnode_t* node = *head;
		for( ; node != NULL; prev = node, node = node->next )
		{
			if( node->key == key )
			{
				if( prev )
				{
					prev->next = node->next;
				}
				else
				{
					*head = node->next;
				}
				
				delete node;
				numEntries--;
				return true;
			}
		}
	}
	return false;
}

/*
========================
idHashTableT<_key_,_value_>::Clear
========================
*/
template< typename _key_, class _value_ >
ID_INLINE void idHashTableT<_key_, _value_>::Clear()
{
	for( int i = 0; i < tableSize; i++ )
	{
		hashnode_t* next = heads[ i ];
		while( next != NULL )
		{
			hashnode_t* node = next;
			next = next->next;
			delete node;
		}
		heads[ i ] = NULL;
	}
	numEntries = 0;
}

/*
========================
idHashTableT<_key_,_value_>::DeleteContents
========================
*/
template< typename _key_, class _value_ >
ID_INLINE void idHashTableT<_key_, _value_>::DeleteContents()
{
	for( int i = 0; i < tableSize; i++ )
	{
		hashnode_t* next = heads[ i ];
		while( next != NULL )
		{
			hashnode_t* node = next;
			next = next->next;
			delete node->value;
			delete node;
		}
		heads[ i ] = NULL;
	}
	numEntries = 0;
}

/*
========================
idHashTableT<_key_,_value_>::Num
========================
*/
template< typename _key_, class _value_ >
ID_INLINE int idHashTableT<_key_, _value_>::Num() const
{
	return numEntries;
}

/*
========================
idHashTableT<_key_,_value_>::GetSpread
========================
*/
template< typename _key_, class _value_ >
ID_INLINE int idHashTableT<_key_, _value_>::GetSpread() const
{
	if( !numEntries )
	{
		return 100;
	}
	
	int average = numEntries / tableSize;
	int error = 0;
	for( int i = 0; i < tableSize; i++ )
	{
		int numItems = 0;
		for( hashnode_t* node = heads[ i ]; node != NULL; node = node->next )
		{
			numItems++;
		}
		int e = abs( numItems - average );
		if( e > 1 )
		{
			error += e - 1;
		}
	}
	return 100 - ( error * 100 / numEntries );
}

/*
========================
idHashTableT<_key_,_value_>::operator=
========================
*/
template< typename _key_, class _value_ >
ID_INLINE idHashTableT< _key_, _value_ >& idHashTableT<_key_, _value_>::operator=( const idHashTableT& other )
{
	Copy( other );
	return *this;
}

/*
========================
idHashTableT<_key_,_value_>::Copy
========================
*/
template< typename _key_, class _value_ >
ID_INLINE void idHashTableT<_key_, _value_>::Copy( const idHashTableT& other )
{
	if( &other == this )
	{
		return;
	}
	assert( other.tableSize > 0 );
	
	tableSize		= other.tableSize;
	heads			= new( TAG_IDLIB_HASH ) hashnode_t* [ tableSize ];
	numEntries		= other.numEntries;
	tableSizeMask	= other.tableSizeMask;
	
	for( int i = 0; i < tableSize; i++ )
	{
		if( !other.heads[ i ] )
		{
			heads[ i ] = NULL;
			continue;
		}
		hashnode_t** prev = & heads[ i ];
		for( hashnode_t* node = other.heads[ i ]; node != NULL; node = node->next )
		{
			*prev = new( TAG_IDLIB_HASH ) hashnode_t( node->key, node->value, NULL );
			prev = &( *prev )->next;
		}
	}
}

/*
===============================================================================

	General hash table. Slower than idHashIndex but it can also be used for
	linked lists and other data structures than just indexes or arrays.

===============================================================================
*/

template< class Type >
class idHashTable
{
public:
	idHashTable( int newtablesize = 256 );
	idHashTable( const idHashTable<Type>& map );
	~idHashTable();
	
	// returns total size of allocated memory
	size_t			Allocated() const;
	// returns total size of allocated memory including size of hash table type
	size_t			Size() const;
	
	void			Set( const char* key, Type& value );
	bool			Get( const char* key, Type** value = NULL ) const;
	bool			Remove( const char* key );
	
	void			Clear();
	void			DeleteContents();
	
	// the entire contents can be itterated over, but note that the
	// exact index for a given element may change when new elements are added
	int				Num() const;
	Type* 			GetIndex( int index ) const;
	
	int				GetSpread() const;
	
private:
	struct hashnode_s
	{
		idStr		key;
		Type		value;
		hashnode_s* next;
		
		hashnode_s( const idStr& k, Type v, hashnode_s* n ) : key( k ), value( v ), next( n ) {};
		hashnode_s( const char* k, Type v, hashnode_s* n ) : key( k ), value( v ), next( n ) {};
	};
	
	hashnode_s** 	heads;
	
	int				tablesize;
	int				numentries;
	int				tablesizemask;
	
	int				GetHash( const char* key ) const;
};

/*
================
idHashTable<Type>::idHashTable
================
*/
template< class Type >
ID_INLINE idHashTable<Type>::idHashTable( int newtablesize )
{

	assert( idMath::IsPowerOfTwo( newtablesize ) );
	
	tablesize = newtablesize;
	assert( tablesize > 0 );
	
	heads = new( TAG_IDLIB_HASH ) hashnode_s *[ tablesize ];
	memset( heads, 0, sizeof( *heads ) * tablesize );
	
	numentries		= 0;
	
	tablesizemask = tablesize - 1;
}

/*
================
idHashTable<Type>::idHashTable
================
*/
template< class Type >
ID_INLINE idHashTable<Type>::idHashTable( const idHashTable<Type>& map )
{
	int			i;
	hashnode_s*	node;
	hashnode_s**	prev;
	
	assert( map.tablesize > 0 );
	
	tablesize		= map.tablesize;
	heads			= new( TAG_IDLIB_HASH ) hashnode_s *[ tablesize ];
	numentries		= map.numentries;
	tablesizemask	= map.tablesizemask;
	
	for( i = 0; i < tablesize; i++ )
	{
		if( !map.heads[ i ] )
		{
			heads[ i ] = NULL;
			continue;
		}
		
		prev = &heads[ i ];
		for( node = map.heads[ i ]; node != NULL; node = node->next )
		{
			*prev = new( TAG_IDLIB_HASH ) hashnode_s( node->key, node->value, NULL );
			prev = &( *prev )->next;
		}
	}
}

/*
================
idHashTable<Type>::~idHashTable<Type>
================
*/
template< class Type >
ID_INLINE idHashTable<Type>::~idHashTable()
{
	Clear();
	delete[] heads;
}

/*
================
idHashTable<Type>::Allocated
================
*/
template< class Type >
ID_INLINE size_t idHashTable<Type>::Allocated() const
{
	return sizeof( heads ) * tablesize + sizeof( *heads ) * numentries;
}

/*
================
idHashTable<Type>::Size
================
*/
template< class Type >
ID_INLINE size_t idHashTable<Type>::Size() const
{
	return sizeof( idHashTable<Type> ) + sizeof( heads ) * tablesize + sizeof( *heads ) * numentries;
}

/*
================
idHashTable<Type>::GetHash
================
*/
template< class Type >
ID_INLINE int idHashTable<Type>::GetHash( const char* key ) const
{
	return ( idStr::Hash( key ) & tablesizemask );
}

/*
================
idHashTable<Type>::Set
================
*/
template< class Type >
ID_INLINE void idHashTable<Type>::Set( const char* key, Type& value )
{
	hashnode_s* node, **nextPtr;
	int hash, s;
	
	hash = GetHash( key );
	for( nextPtr = &( heads[hash] ), node = *nextPtr; node != NULL; nextPtr = &( node->next ), node = *nextPtr )
	{
		s = node->key.Cmp( key );
		if( s == 0 )
		{
			node->value = value;
			return;
		}
		if( s > 0 )
		{
			break;
		}
	}
	
	numentries++;
	
	*nextPtr = new( TAG_IDLIB_HASH ) hashnode_s( key, value, heads[ hash ] );
	( *nextPtr )->next = node;
}

/*
================
idHashTable<Type>::Get
================
*/
template< class Type >
ID_INLINE bool idHashTable<Type>::Get( const char* key, Type** value ) const
{
	hashnode_s* node;
	int hash, s;
	
	hash = GetHash( key );
	for( node = heads[ hash ]; node != NULL; node = node->next )
	{
		s = node->key.Cmp( key );
		if( s == 0 )
		{
			if( value )
			{
				*value = &node->value;
			}
			return true;
		}
		if( s > 0 )
		{
			break;
		}
	}
	
	if( value )
	{
		*value = NULL;
	}
	
	return false;
}

/*
================
idHashTable<Type>::GetIndex

the entire contents can be itterated over, but note that the
exact index for a given element may change when new elements are added
================
*/
template< class Type >
ID_INLINE Type* idHashTable<Type>::GetIndex( int index ) const
{
	hashnode_s*	node;
	int			count;
	int			i;
	
	if( ( index < 0 ) || ( index > numentries ) )
	{
		assert( 0 );
		return NULL;
	}
	
	count = 0;
	for( i = 0; i < tablesize; i++ )
	{
		for( node = heads[ i ]; node != NULL; node = node->next )
		{
			if( count == index )
			{
				return &node->value;
			}
			count++;
		}
	}
	
	return NULL;
}

/*
================
idHashTable<Type>::Remove
================
*/
template< class Type >
ID_INLINE bool idHashTable<Type>::Remove( const char* key )
{
	hashnode_s**	head;
	hashnode_s*	node;
	hashnode_s*	prev;
	int			hash;
	
	hash = GetHash( key );
	head = &heads[ hash ];
	if( *head )
	{
		for( prev = NULL, node = *head; node != NULL; prev = node, node = node->next )
		{
			if( node->key == key )
			{
				if( prev )
				{
					prev->next = node->next;
				}
				else
				{
					*head = node->next;
				}
				
				delete node;
				numentries--;
				return true;
			}
		}
	}
	
	return false;
}

/*
================
idHashTable<Type>::Clear
================
*/
template< class Type >
ID_INLINE void idHashTable<Type>::Clear()
{
	int			i;
	hashnode_s*	node;
	hashnode_s*	next;
	
	for( i = 0; i < tablesize; i++ )
	{
		next = heads[ i ];
		while( next != NULL )
		{
			node = next;
			next = next->next;
			delete node;
		}
		
		heads[ i ] = NULL;
	}
	
	numentries = 0;
}

/*
================
idHashTable<Type>::DeleteContents
================
*/
template< class Type >
ID_INLINE void idHashTable<Type>::DeleteContents()
{
	int			i;
	hashnode_s*	node;
	hashnode_s*	next;
	
	for( i = 0; i < tablesize; i++ )
	{
		next = heads[ i ];
		while( next != NULL )
		{
			node = next;
			next = next->next;
			delete node->value;
			delete node;
		}
		
		heads[ i ] = NULL;
	}
	
	numentries = 0;
}

/*
================
idHashTable<Type>::Num
================
*/
template< class Type >
ID_INLINE int idHashTable<Type>::Num() const
{
	return numentries;
}

#if defined(ID_TYPEINFO)
#define __GNUC__ 99
#endif

#if !defined(__GNUC__) || __GNUC__ < 4
/*
================
idHashTable<Type>::GetSpread
================
*/
template< class Type >
int idHashTable<Type>::GetSpread() const
{
	int i, average, error, e;
	hashnode_s*	node;
	
	// if no items in hash
	if( !numentries )
	{
		return 100;
	}
	average = numentries / tablesize;
	error = 0;
	for( i = 0; i < tablesize; i++ )
	{
		numItems = 0;
		for( node = heads[ i ]; node != NULL; node = node->next )
		{
			numItems++;
		}
		e = abs( numItems - average );
		if( e > 1 )
		{
			error += e - 1;
		}
	}
	return 100 - ( error * 100 / numentries );
}
#endif

#if defined(ID_TYPEINFO)
#undef __GNUC__
#endif

#endif /* !__HASHTABLE_H__ */
