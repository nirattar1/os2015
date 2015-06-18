#include "setos.h"
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <assert.h>
#include <pthread.h>

//node struct
typedef struct _setos_node {
	void*							value;
	int								key;
	struct _setos_node*				next;
	//lock for each node
	pthread_mutex_t					_lock;

} setos_node;


// head node of the linked list
setos_node* head;




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
	head = (setos_node*)malloc(sizeof(setos_node));
	head->next = NULL;
	head->key = INT_MIN;
	if ( pthread_mutex_init(&head->_lock, NULL) !=  0)
	{
		setos_free();
		return 1;
	}
	return 0;
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

	setos_node* node = head;
	setos_node* prev = NULL;

	// go through nodes until the key of "node" exceeds "key"
	// new node should then be between "prev" and "node"
	while ((node != NULL) && (node->key < key))
	{
		prev = node;
		node = node->next;
	}

	// if node is same as our key - key already in list
	if ((node != NULL) && (node->key == key)){
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

	// 1. lock
	// 2. validate that node is still accessible from head
	// 3. validate that prev points to node
	DoLock(node);
	DoLock(prev);


	int CheckNext = 0;
	int CheckPrev = 0;

	if (node != NULL)
	{
		CheckNext = setos_contains(node->key);
	}
	else
	{
		CheckNext = 1;
	}

	if (prev!=head)
	{
		CheckPrev = setos_contains(prev->key);
	}
	else
	{
		CheckPrev = 1;
	}



	if (CheckPrev && CheckNext)
	{
		//validations succeeded, perform add
		new_node->next = node;
		prev->next = new_node;
		DoUnlock(prev);
		DoUnlock(node);
	}
	else
	{
		//one of conditions doesn't hold, unlock and try again
		DoUnlock(prev);
		DoUnlock(node);
		return setos_add(key, value);
	}

	return 1;
}



int setos_remove(int key, void** value)
{
	setos_node* node = head->next;
	setos_node* prev = head;


	// go through all nodes
	while (node != NULL) {
		if (node->key == key) {			// found node

			//first, lock
			DoLock(prev);
			DoLock(node);

			//validate node still accessible from head
			if (setos_contains(key))
			{
				if (value != NULL)
				{			// set value if needed
					*value = node->value;
				}

				//do the removal
				prev->next = node->next;

				//unlock
				DoUnlock(node);
				DoUnlock(prev);
				return 1;
			}
			else
			{
				//validation failed, try again
				DoUnlock(node);
				DoUnlock(prev);
				return setos_remove(key, value);
			}
		}
		prev = node;					// proceed to next node
		node = node->next;
	}

	return 0;
}


//no locking
int setos_contains(int key)
{
	setos_node* node = head->next;
	// go through all nodes
	while (node != NULL) {
		if (node->key == key)	{	// found node
			return 1;
		}
		node = node->next;
	}

	return 0;
}




//
//int main(void)
//{
//	printf ("optimistic\n" );
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
