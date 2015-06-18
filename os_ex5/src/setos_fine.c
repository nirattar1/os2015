#include "setos.h"
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <assert.h>
#include <pthread.h>


//node struct
typedef struct _setos_node
{
	void*							value;
	int								key;
	volatile struct _setos_node*	next;

	//lock for each node
	pthread_mutex_t					_lock;

} setos_node;



// head node of the linked list
setos_node*		head;


void DoLock(volatile setos_node* node)
{
	if (node) //lock only if node exists
	{
		if( pthread_mutex_lock((void*)&node->_lock)!= 0 )
		{
			exit(-1);
		}
	}
}

void DoUnlock(volatile setos_node* node)
{
	if (node) //only if node exists
	{
		if (pthread_mutex_unlock((void*)&node->_lock) != 0 )
		{
			exit(-1);
		}
	}
}

int setos_init(void)
{

	//allocate and init lock (of head)
	head = (setos_node*)malloc(sizeof(setos_node));
	head->next = NULL;
	head->key = INT_MIN;
	if (pthread_mutex_init(&head->_lock, NULL)!=0)
	{
		setos_free();
		return -1;
	}
	return 1;
}


int setos_free(void)
{

	// go through all nodes and destroy their locks
	volatile setos_node* node = head;
	while (node != NULL)
	{
		pthread_mutex_destroy((void *) &(node->_lock));
		node = node->next;
	}

	free(head);
}



int setos_add(int key, void* value)
{
	
	volatile setos_node* node = head;
	volatile setos_node* prev = NULL;

	//lock head
	DoLock(node);

	// go through nodes until the key of "node" exceeds "key"
	// new node should then be between "prev" and "node"
	while ((node != NULL) && (node->key < key)) {

		DoUnlock(prev);

		prev = node;
		node = node->next;
		DoLock(node);
	}

	// if node is same as our key - key already in list
	if ((node != NULL) && (node->key == key))
	{
		DoUnlock(node);
		DoUnlock(prev);
		return 0;
	}

	// allocate new node and initialize it
	setos_node* new_node = (setos_node*)malloc(sizeof(setos_node));
	new_node->key = key;
	new_node->value = value;
	if (pthread_mutex_init(&new_node->_lock, NULL) != 0)
	{
		return 1;
	}

	//insert link
	new_node->next = node;
	prev->next = new_node;
	DoUnlock(node);
	DoUnlock(prev);
	return 1;
}



int setos_remove(int key, void** value)
{

	volatile setos_node* node = head->next;
	volatile setos_node* prev = head;

	DoLock(prev);
	DoLock(node);

	// go through all nodes
	while (node != NULL) {
		if (node->key == key) {			// found node
			if (value != NULL)			// set value if needed
				*value = node->value;

			prev->next = node->next;	// remove node from list
			DoUnlock(prev);
			DoUnlock(node);
			return 1;					// (we don't deal with memory leaks)
		}
		DoUnlock(prev);
		prev = node;					// proceed to next node
		node = node->next;
		DoLock(node);
	}

	DoUnlock(prev);
	DoUnlock(node);
	return 0;
}

int setos_contains(int key)
{

	volatile setos_node* node = head->next;
	DoLock(node);

	// go through all nodes
	while (node != NULL) {
		if (node->key == key)	{	// found node
			DoUnlock(node);
			return 1;
		}
		DoUnlock(node);
		node = node->next;
		DoLock(node);
	}

	DoUnlock(node);
	return 0;
}


//
//
//int main(void)
//{
//	printf ("fine\n" );
//
//	int x = 1;
//	if (setos_init()==-1)
//	{
//		return -1;
//	}
//
//	setos_add(1, &x);
//	setos_add(2, &x);
//	setos_add(3, &x);
//
//	assert(setos_contains(2));
//	assert(!setos_contains(4));
//
//	assert(!setos_remove(4,NULL));
//	assert(setos_remove(2,NULL));
//
//	assert(!setos_contains(2));
//
//	setos_free();
//
//
//	printf ("done\n" );
//
//	return 0;
//}
