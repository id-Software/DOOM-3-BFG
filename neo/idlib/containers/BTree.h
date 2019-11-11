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

#ifndef __BTREE_H__
#define __BTREE_H__

/*
===============================================================================

	Balanced Search Tree

===============================================================================
*/

//#define BTREE_CHECK

template< class objType, class keyType >
class idBTreeNode
{
public:
	keyType							key;			// key used for sorting
	objType* 						object;			// if != NULL pointer to object stored in leaf node
	idBTreeNode* 					parent;			// parent node
	idBTreeNode* 					next;			// next sibling
	idBTreeNode* 					prev;			// prev sibling
	int								numChildren;	// number of children
	idBTreeNode* 					firstChild;		// first child
	idBTreeNode* 					lastChild;		// last child
};


template< class objType, class keyType, int maxChildrenPerNode >
class idBTree
{
public:
	idBTree();
	~idBTree();

	void							Init();
	void							Shutdown();

	idBTreeNode<objType, keyType>* 	Add( objType* object, keyType key );						// add an object to the tree
	void							Remove( idBTreeNode<objType, keyType>* node );				// remove an object node from the tree

	idBTreeNode<objType, keyType>* 	NodeFind( keyType key ) const;								// find an object using the given key
	idBTreeNode<objType, keyType>* 	NodeFindSmallestLargerEqual( keyType key ) const;			// find an object with the smallest key larger equal the given key
	idBTreeNode<objType, keyType>* 	NodeFindLargestSmallerEqual( keyType key ) const;			// find an object with the largest key smaller equal the given key

	objType* 						Find( keyType key ) const;									// find an object using the given key
	objType* 						FindSmallestLargerEqual( keyType key ) const;				// find an object with the smallest key larger equal the given key
	objType* 						FindLargestSmallerEqual( keyType key ) const;				// find an object with the largest key smaller equal the given key

	idBTreeNode<objType, keyType>* 	GetRoot() const;											// returns the root node of the tree
	int								GetNodeCount() const;										// returns the total number of nodes in the tree
	idBTreeNode<objType, keyType>* 	GetNext( idBTreeNode<objType, keyType>* node ) const;		// goes through all nodes of the tree
	idBTreeNode<objType, keyType>* 	GetNextLeaf( idBTreeNode<objType, keyType>* node ) const;	// goes through all leaf nodes of the tree

private:
	idBTreeNode<objType, keyType>* 	root;
	idBlockAlloc<idBTreeNode<objType, keyType>, 128>	nodeAllocator;

	idBTreeNode<objType, keyType>* 	AllocNode();
	void							FreeNode( idBTreeNode<objType, keyType>* node );
	void							SplitNode( idBTreeNode<objType, keyType>* node );
	idBTreeNode<objType, keyType>* 	MergeNodes( idBTreeNode<objType, keyType>* node1, idBTreeNode<objType, keyType>* node2 );

	void							CheckTree_r( idBTreeNode<objType, keyType>* node, int& numNodes ) const;
	void							CheckTree() const;
};


template< class objType, class keyType, int maxChildrenPerNode >
ID_INLINE idBTree<objType, keyType, maxChildrenPerNode>::idBTree()
{
	assert( maxChildrenPerNode >= 4 );
	root = NULL;
}

template< class objType, class keyType, int maxChildrenPerNode >
ID_INLINE idBTree<objType, keyType, maxChildrenPerNode>::~idBTree()
{
	Shutdown();
}

template< class objType, class keyType, int maxChildrenPerNode >
ID_INLINE void idBTree<objType, keyType, maxChildrenPerNode>::Init()
{
	root = AllocNode();
}

template< class objType, class keyType, int maxChildrenPerNode >
ID_INLINE void idBTree<objType, keyType, maxChildrenPerNode>::Shutdown()
{
	nodeAllocator.Shutdown();
	root = NULL;
}

template< class objType, class keyType, int maxChildrenPerNode >
ID_INLINE idBTreeNode<objType, keyType>* idBTree<objType, keyType, maxChildrenPerNode>::Add( objType* object, keyType key )
{
	idBTreeNode<objType, keyType>* node, *child, *newNode;

	if( root == NULL )
	{
		root = AllocNode();
	}

	if( root->numChildren >= maxChildrenPerNode )
	{
		newNode = AllocNode();
		newNode->key = root->key;
		newNode->firstChild = root;
		newNode->lastChild = root;
		newNode->numChildren = 1;
		root->parent = newNode;
		SplitNode( root );
		root = newNode;
	}

	newNode = AllocNode();
	newNode->key = key;
	newNode->object = object;

	for( node = root; node->firstChild != NULL; node = child )
	{

		if( key > node->key )
		{
			node->key = key;
		}

		// find the first child with a key larger equal to the key of the new node
		for( child = node->firstChild; child->next; child = child->next )
		{
			if( key <= child->key )
			{
				break;
			}
		}

		if( child->object )
		{

			if( key <= child->key )
			{
				// insert new node before child
				if( child->prev )
				{
					child->prev->next = newNode;
				}
				else
				{
					node->firstChild = newNode;
				}
				newNode->prev = child->prev;
				newNode->next = child;
				child->prev = newNode;
			}
			else
			{
				// insert new node after child
				if( child->next )
				{
					child->next->prev = newNode;
				}
				else
				{
					node->lastChild = newNode;
				}
				newNode->prev = child;
				newNode->next = child->next;
				child->next = newNode;
			}

			newNode->parent = node;
			node->numChildren++;

#ifdef BTREE_CHECK
			CheckTree();
#endif

			return newNode;
		}

		// make sure the child has room to store another node
		if( child->numChildren >= maxChildrenPerNode )
		{
			SplitNode( child );
			if( key <= child->prev->key )
			{
				child = child->prev;
			}
		}
	}

	// we only end up here if the root node is empty
	newNode->parent = root;
	root->key = key;
	root->firstChild = newNode;
	root->lastChild = newNode;
	root->numChildren++;

#ifdef BTREE_CHECK
	CheckTree();
#endif

	return newNode;
}

template< class objType, class keyType, int maxChildrenPerNode >
ID_INLINE void idBTree<objType, keyType, maxChildrenPerNode>::Remove( idBTreeNode<objType, keyType>* node )
{
	idBTreeNode<objType, keyType>* parent;

	assert( node->object != NULL );

	// unlink the node from it's parent
	if( node->prev )
	{
		node->prev->next = node->next;
	}
	else
	{
		node->parent->firstChild = node->next;
	}
	if( node->next )
	{
		node->next->prev = node->prev;
	}
	else
	{
		node->parent->lastChild = node->prev;
	}
	node->parent->numChildren--;

	// make sure there are no parent nodes with a single child
	for( parent = node->parent; parent != root && parent->numChildren <= 1; parent = parent->parent )
	{

		if( parent->next )
		{
			parent = MergeNodes( parent, parent->next );
		}
		else if( parent->prev )
		{
			parent = MergeNodes( parent->prev, parent );
		}

		// a parent may not use a key higher than the key of it's last child
		if( parent->key > parent->lastChild->key )
		{
			parent->key = parent->lastChild->key;
		}

		if( parent->numChildren > maxChildrenPerNode )
		{
			SplitNode( parent );
			break;
		}
	}
	for( ; parent != NULL && parent->lastChild != NULL; parent = parent->parent )
	{
		// a parent may not use a key higher than the key of it's last child
		if( parent->key > parent->lastChild->key )
		{
			parent->key = parent->lastChild->key;
		}
	}

	// free the node
	FreeNode( node );

	// remove the root node if it has a single internal node as child
	if( root->numChildren == 1 && root->firstChild->object == NULL )
	{
		idBTreeNode<objType, keyType>* oldRoot = root;
		root->firstChild->parent = NULL;
		root = root->firstChild;
		FreeNode( oldRoot );
	}

#ifdef BTREE_CHECK
	CheckTree();
#endif
}

template< class objType, class keyType, int maxChildrenPerNode >
ID_INLINE idBTreeNode<objType, keyType>* idBTree<objType, keyType, maxChildrenPerNode>::NodeFind( keyType key ) const
{
	idBTreeNode<objType, keyType>* node;

	for( node = root->firstChild; node != NULL; node = node->firstChild )
	{
		while( node->next )
		{
			if( node->key >= key )
			{
				break;
			}
			node = node->next;
		}
		if( node->object )
		{
			if( node->key == key )
			{
				return node;
			}
			else
			{
				return NULL;
			}
		}
	}
	return NULL;
}

template< class objType, class keyType, int maxChildrenPerNode >
ID_INLINE idBTreeNode<objType, keyType>* idBTree<objType, keyType, maxChildrenPerNode>::NodeFindSmallestLargerEqual( keyType key ) const
{
	idBTreeNode<objType, keyType>* node;

	if( root == NULL )
	{
		return NULL;
	}

	for( node = root->firstChild; node != NULL; node = node->firstChild )
	{
		while( node->next )
		{
			if( node->key >= key )
			{
				break;
			}
			node = node->next;
		}
		if( node->object )
		{
			if( node->key >= key )
			{
				return node;
			}
			else
			{
				return NULL;
			}
		}
	}
	return NULL;
}

template< class objType, class keyType, int maxChildrenPerNode >
ID_INLINE idBTreeNode<objType, keyType>* idBTree<objType, keyType, maxChildrenPerNode>::NodeFindLargestSmallerEqual( keyType key ) const
{
	idBTreeNode<objType, keyType>* node;

	if( root == NULL )
	{
		return NULL;
	}

	idBTreeNode<objType, keyType>* smaller = NULL;
	for( node = root->firstChild; node != NULL; node = node->firstChild )
	{
		while( node->next )
		{
			if( node->key >= key )
			{
				break;
			}
			smaller = node;
			node = node->next;
		}
		if( node->object )
		{
			if( node->key <= key )
			{
				return node;
			}
			else if( smaller == NULL )
			{
				return NULL;
			}
			else
			{
				node = smaller;
				if( node->object )
				{
					return node;
				}
			}
		}
	}
	return NULL;
}

template< class objType, class keyType, int maxChildrenPerNode >
ID_INLINE objType* idBTree<objType, keyType, maxChildrenPerNode>::Find( keyType key ) const
{
	idBTreeNode<objType, keyType>* node = NodeFind( key );
	if( node == NULL )
	{
		return NULL;
	}
	else
	{
		return node->object;
	}
}

template< class objType, class keyType, int maxChildrenPerNode >
ID_INLINE objType* idBTree<objType, keyType, maxChildrenPerNode>::FindSmallestLargerEqual( keyType key ) const
{
	idBTreeNode<objType, keyType>* node = NodeFindSmallestLargerEqual( key );
	if( node == NULL )
	{
		return NULL;
	}
	else
	{
		return node->object;
	}
}

template< class objType, class keyType, int maxChildrenPerNode >
ID_INLINE objType* idBTree<objType, keyType, maxChildrenPerNode>::FindLargestSmallerEqual( keyType key ) const
{
	idBTreeNode<objType, keyType>* node = NodeFindLargestSmallerEqual( key );
	if( node == NULL )
	{
		return NULL;
	}
	else
	{
		return node->object;
	}
}

template< class objType, class keyType, int maxChildrenPerNode >
ID_INLINE idBTreeNode<objType, keyType>* idBTree<objType, keyType, maxChildrenPerNode>::GetRoot() const
{
	return root;
}

template< class objType, class keyType, int maxChildrenPerNode >
ID_INLINE int idBTree<objType, keyType, maxChildrenPerNode>::GetNodeCount() const
{
	return nodeAllocator.GetAllocCount();
}

template< class objType, class keyType, int maxChildrenPerNode >
ID_INLINE idBTreeNode<objType, keyType>* idBTree<objType, keyType, maxChildrenPerNode>::GetNext( idBTreeNode<objType, keyType>* node ) const
{
	if( node->firstChild )
	{
		return node->firstChild;
	}
	else
	{
		while( node && node->next == NULL )
		{
			node = node->parent;
		}
		return node;
	}
}

template< class objType, class keyType, int maxChildrenPerNode >
ID_INLINE idBTreeNode<objType, keyType>* idBTree<objType, keyType, maxChildrenPerNode>::GetNextLeaf( idBTreeNode<objType, keyType>* node ) const
{
	if( node->firstChild )
	{
		while( node->firstChild )
		{
			node = node->firstChild;
		}
		return node;
	}
	else
	{
		while( node && node->next == NULL )
		{
			node = node->parent;
		}
		if( node )
		{
			node = node->next;
			while( node->firstChild )
			{
				node = node->firstChild;
			}
			return node;
		}
		else
		{
			return NULL;
		}
	}
}

template< class objType, class keyType, int maxChildrenPerNode >
ID_INLINE idBTreeNode<objType, keyType>* idBTree<objType, keyType, maxChildrenPerNode>::AllocNode()
{
	idBTreeNode<objType, keyType>* node = nodeAllocator.Alloc();
	node->key = 0;
	node->parent = NULL;
	node->next = NULL;
	node->prev = NULL;
	node->numChildren = 0;
	node->firstChild = NULL;
	node->lastChild = NULL;
	node->object = NULL;
	return node;
}

template< class objType, class keyType, int maxChildrenPerNode >
ID_INLINE void idBTree<objType, keyType, maxChildrenPerNode>::FreeNode( idBTreeNode<objType, keyType>* node )
{
	nodeAllocator.Free( node );
}

template< class objType, class keyType, int maxChildrenPerNode >
ID_INLINE void idBTree<objType, keyType, maxChildrenPerNode>::SplitNode( idBTreeNode<objType, keyType>* node )
{
	int i;
	idBTreeNode<objType, keyType>* child, *newNode;

	// allocate a new node
	newNode = AllocNode();
	newNode->parent = node->parent;

	// divide the children over the two nodes
	child = node->firstChild;
	child->parent = newNode;
	for( i = 3; i < node->numChildren; i += 2 )
	{
		child = child->next;
		child->parent = newNode;
	}

	newNode->key = child->key;
	newNode->numChildren = node->numChildren / 2;
	newNode->firstChild = node->firstChild;
	newNode->lastChild = child;

	node->numChildren -= newNode->numChildren;
	node->firstChild = child->next;

	child->next->prev = NULL;
	child->next = NULL;

	// add the new child to the parent before the split node
	assert( node->parent->numChildren < maxChildrenPerNode );

	if( node->prev )
	{
		node->prev->next = newNode;
	}
	else
	{
		node->parent->firstChild = newNode;
	}
	newNode->prev = node->prev;
	newNode->next = node;
	node->prev = newNode;

	node->parent->numChildren++;
}

template< class objType, class keyType, int maxChildrenPerNode >
ID_INLINE idBTreeNode<objType, keyType>* idBTree<objType, keyType, maxChildrenPerNode>::MergeNodes( idBTreeNode<objType, keyType>* node1, idBTreeNode<objType, keyType>* node2 )
{
	idBTreeNode<objType, keyType>* child;

	assert( node1->parent == node2->parent );
	assert( node1->next == node2 && node2->prev == node1 );
	assert( node1->object == NULL && node2->object == NULL );
	assert( node1->numChildren >= 1 && node2->numChildren >= 1 );

	for( child = node1->firstChild; child->next; child = child->next )
	{
		child->parent = node2;
	}
	child->parent = node2;
	child->next = node2->firstChild;
	node2->firstChild->prev = child;
	node2->firstChild = node1->firstChild;
	node2->numChildren += node1->numChildren;

	// unlink the first node from the parent
	if( node1->prev )
	{
		node1->prev->next = node2;
	}
	else
	{
		node1->parent->firstChild = node2;
	}
	node2->prev = node1->prev;
	node2->parent->numChildren--;

	FreeNode( node1 );

	return node2;
}

template< class objType, class keyType, int maxChildrenPerNode >
ID_INLINE void idBTree<objType, keyType, maxChildrenPerNode>::CheckTree_r( idBTreeNode<objType, keyType>* node, int& numNodes ) const
{
	int numChildren;
	idBTreeNode<objType, keyType>* child;

	numNodes++;

	// the root node may have zero children and leaf nodes always have zero children, all other nodes should have at least 2 and at most maxChildrenPerNode children
	assert( ( node == root ) || ( node->object != NULL && node->numChildren == 0 ) || ( node->numChildren >= 2 && node->numChildren <= maxChildrenPerNode ) );
	// the key of a node may never be larger than the key of it's last child
	assert( ( node->lastChild == NULL ) || ( node->key <= node->lastChild->key ) );

	numChildren = 0;
	for( child = node->firstChild; child; child = child->next )
	{
		numChildren++;
		// make sure the children are properly linked
		if( child->prev == NULL )
		{
			assert( node->firstChild == child );
		}
		else
		{
			assert( child->prev->next == child );
		}
		if( child->next == NULL )
		{
			assert( node->lastChild == child );
		}
		else
		{
			assert( child->next->prev == child );
		}
		// recurse down the tree
		CheckTree_r( child, numNodes );
	}
	// the number of children should equal the number of linked children
	assert( numChildren == node->numChildren );
}

template< class objType, class keyType, int maxChildrenPerNode >
ID_INLINE void idBTree<objType, keyType, maxChildrenPerNode>::CheckTree() const
{
	int numNodes = 0;
	idBTreeNode<objType, keyType>* node, *lastNode;

	CheckTree_r( root, numNodes );

	// the number of nodes in the tree should equal the number of allocated nodes
	assert( numNodes == nodeAllocator.GetAllocCount() );

	// all the leaf nodes should be ordered
	lastNode = GetNextLeaf( GetRoot() );
	if( lastNode )
	{
		for( node = GetNextLeaf( lastNode ); node; lastNode = node, node = GetNextLeaf( node ) )
		{
			assert( lastNode->key <= node->key );
		}
	}
}

#endif /* !__BTREE_H__ */
