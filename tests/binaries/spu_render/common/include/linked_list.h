#ifndef COMMON_LINKED_LIST_H
#define COMMON_LINKED_LIST_H

#include <stdint.h>

// Forward declaration
template <class Node> class LinkedList;

template <typename Data>
class LinkedListNode : public Data
{
	friend class LinkedList< class LinkedListNode<Data> >;
public:
	LinkedListNode(LinkedListNode* node=0) : m_next(node) {};
	~LinkedListNode(){}
	
	LinkedListNode<Data>*		getNext() {	return	m_next;	}
	//template <class> class friend LinkedList;
	
protected:
	void		setNext( LinkedListNode* node )	{	m_next = node;	}
	
private:
	LinkedListNode*		m_next;
}__attribute__ ((aligned(16)));


template <class Node>
class LinkedList{
public:
	LinkedList(): m_num( 0 ), m_head( 0 ), m_tail( 0 ) {};
	~LinkedList(){};
	
	uint32_t		getNum() 	const	{	return	m_num;	}
	Node*			getHead()	const	{	return	m_head;	}
	Node*			getTail()	const	{	return	m_tail;	}
	
	void reset()
	{
		m_num	= 0;
		m_head	= 0;
		m_tail	= 0;
	}
	
	void add( Node* node )
	{
		if( m_tail ) { m_tail->setNext( node ); }
		else {m_head	=	node; }
		node->setNext( 0 );
		m_tail	= node;
		m_num++;
	}

	void connectList( LinkedList* list)
	{
		if(m_tail)
		{ m_tail->setNext(list->head); }
		else{ m_head = list->head; }
		m_num += list->m_num;
	}

	void addHead( Node* node )
	{
		if( !m_tail )
		{ m_tail	=	node; }
		node->setNext( m_head );
		m_head	= node;
		m_num++;
	}
	
	bool del( Node* node )
	{
		Node	*pnow	=	m_head;
		Node	*prev	=	0;
		while( pnow )
		{
			if( pnow == node )
			{
				if( prev )
				{ prev->setNext( pnow->getNext() ); }
				if( m_head == pnow )
				{ m_head = pnow->getNext(); }
				if( m_tail == pnow )
				{ m_tail = prev; }
				m_num--;
				return true;
			}
			prev	=	pnow;
			pnow	=	pnow->getNext();
		}
		return false;
	}
	
	bool del( Node* node, Node* prev )
	{
		if( prev )
		{ prev->setNext( node->getNext() ); }
		if( m_head == node )
		{ m_head = node->getNext(); }
		if( m_tail == node )
		{ m_tail = prev; }
		m_num--;
		return	true;
	}
	
	inline Node* delHead()
	{
		Node*	node	= m_head;
		if ( node )
		{
			if( m_tail == node ) { m_tail = 0; }
			m_head = m_head->getNext();
			m_num--;
		}
		return	node;
	}
	
private:
	uint32_t		m_num;
	Node*			m_head;
	Node*			m_tail;
};

#endif // COMMON_LINKED_LIST_H