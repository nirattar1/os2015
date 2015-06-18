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

} setos_node;

// head node of the linked list
setos_node*			head;


//global lock
pthread_mutex_t	_global_lock;


void DoLock()
{
	if (pthread_mutex_lock(&_global_lock) != 0)
	{
		exit(-1);
	}
}

void DoUnlock(){

	if (pthread_mutex_unlock(&_global_lock) != 0)
	{
		exit(-1);
	}
}

int setos_init(void)
{

	//allocate and init global lock
	head = (setos_node*)malloc(sizeof(setos_node));
	head->next = NULL;
	head->key = INT_MIN;

	if(pthread_mutex_init(&_global_lock, NULL) != 0)
	{
		setos_free();
		return -1;
	}
	return 1;
}


int setos_free(void)
{
	//clean memory, destroy lock.
	free(head);
	pthread_mutex_destroy(&_global_lock);
	return 0;
}



int setos_add(int key, void* value)
{
	DoLock();
	volatile setos_node* node = head;
	volatile setos_node* prev;

	// go through nodes until the key of "node" exceeds "key"
	// new node should then be between "prev" and "node"
	while ((node != NULL) && (node->key < key)) {
		prev = node;
		node = node->next;
	}

	// if node is same as our key - key already in list
	if ((node != NULL) && (node->key == key)){
		DoUnlock();
		return 0;
	}
	
	// allocate new node and initialize it
	setos_node* new_node = (setos_node*)malloc(sizeof(setos_node));
	new_node->key = key;
	new_node->value = value;

	// place node into list
	new_node->next = node;
	prev->next = new_node;

	DoUnlock();
	return 1;
}



int setos_remove(int key, void** value)
{

	DoLock();
	volatile setos_node* node = head->next;
	volatile setos_node* prev = head;

	// go through all nodes
	while (node != NULL) {
		if (node->key == key) {			// found node
			if (value != NULL)			// set value if needed
				*value = node->value;

			prev->next = node->next;	// remove node from list

			DoUnlock();
			return 1;					// (we don't deal with memory leaks)
		}

		prev = node;					// proceed to next node
		node = node->next;
	}

	DoUnlock();
	return 0;
}

int setos_contains(int key)
{
	DoLock();

	volatile setos_node* node = head->next;


	while (node != NULL) // go through all nodes
	{
		// if found node
		if (node->key == key)
		{
			DoUnlock(head);
			return 1;
		}
		node = node->next;
	}

	DoUnlock();
	return 0;
}


