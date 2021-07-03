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
#ifndef __QUEUE_H__
#define __QUEUE_H__

/*
===============================================================================

	Queue template

===============================================================================
*/
template< class type, int nextOffset >
class idQueueTemplate
{
public:
	idQueueTemplate();

	void					Add( type* element );
	type* 					Get();

private:
	type* 					first;
	type* 					last;
};

#define QUEUE_NEXT_PTR( element )		(*((type**)(((byte*)element)+nextOffset)))

template< class type, int nextOffset >
idQueueTemplate<type, nextOffset>::idQueueTemplate()
{
	first = last = NULL;
}

template< class type, int nextOffset >
void idQueueTemplate<type, nextOffset>::Add( type* element )
{
	QUEUE_NEXT_PTR( element ) = NULL;
	if( last )
	{
		QUEUE_NEXT_PTR( last ) = element;
	}
	else
	{
		first = element;
	}
	last = element;
}

template< class type, int nextOffset >
type* idQueueTemplate<type, nextOffset>::Get()
{
	type* element;

	element = first;
	if( element )
	{
		first = QUEUE_NEXT_PTR( first );
		if( last == element )
		{
			last = NULL;
		}
		QUEUE_NEXT_PTR( element ) = NULL;
	}
	return element;
}

/*
================================================
A node of a Queue
================================================
*/
template< typename type >
class idQueueNode
{
public:
	idQueueNode()
	{
		next = NULL;
	}

	type* 		GetNext() const
	{
		return next;
	}
	void		SetNext( type* next )
	{
		this->next = next;
	}

private:
	type* 		next;
};

/*
================================================
A Queue, idQueue, is a template Container class implementing the Queue abstract data
type.
================================================
*/
template< typename type, idQueueNode<type> type::*nodePtr >
class idQueue
{
public:
	idQueue();

	void		Add( type* element );
	type* 		RemoveFirst();
	type* 		Peek() const;
	bool		IsEmpty();

	static void	Test();

private:
	type* 		first;
	type* 		last;
};

/*
========================
idQueue<type,nodePtr>::idQueue
========================
*/
template< typename type, idQueueNode<type> type::*nodePtr >
idQueue<type, nodePtr>::idQueue()
{
	first = last = NULL;
}

/*
========================
idQueue<type,nodePtr>::Add
========================
*/
template< typename type, idQueueNode<type> type::*nodePtr >
void idQueue<type, nodePtr>::Add( type* element )
{
	( element->*nodePtr ).SetNext( NULL );
	if( last )
	{
		( last->*nodePtr ).SetNext( element );
	}
	else
	{
		first = element;
	}
	last = element;
}

/*
========================
idQueue<type,nodePtr>::RemoveFirst
========================
*/
template< typename type, idQueueNode<type> type::*nodePtr >
type* idQueue<type, nodePtr>::RemoveFirst()
{
	type* element;

	element = first;
	if( element )
	{
		first = ( first->*nodePtr ).GetNext();
		if( last == element )
		{
			last = NULL;
		}
		( element->*nodePtr ).SetNext( NULL );
	}
	return element;
}

/*
========================
idQueue<type,nodePtr>::Peek
========================
*/
template< typename type, idQueueNode<type> type::*nodePtr >
type* idQueue<type, nodePtr>::Peek() const
{
	return first;
}

/*
========================
idQueue<type,nodePtr>::IsEmpty
========================
*/
template< typename type, idQueueNode<type> type::*nodePtr >
bool idQueue<type, nodePtr>::IsEmpty()
{
	return ( first == NULL );
}

/*
========================
idQueue<type,nodePtr>::Test
========================
*/
template< typename type, idQueueNode<type> type::*nodePtr >
void idQueue<type, nodePtr>::Test()
{

	class idMyType
	{
	public:
		idQueueNode<idMyType> queueNode;
	};

	idQueue<idMyType, &idMyType::queueNode> myQueue;

	idMyType* element = new( TAG_IDLIB ) idMyType;
	myQueue.Add( element );
	element = myQueue.RemoveFirst();
	delete element;
}

#endif // !__QUEUE_H__
