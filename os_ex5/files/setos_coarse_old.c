/**
* File:			setos.c
* Author:		Moshe Sulamy
* Description:	Source file of simple set structure
*				for OS course 15b at Tel-Aviv University
**/

#include "setos.h"
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <assert.h>
#include <pthread.h>


//global lock object
pthread_mutex_t _lock;

// a node in the set - you may freely change this struct
typedef struct _setos_node {
	void*							value;
	int								key;
	volatile struct _setos_node*	next;
} setos_node;

// head node of the linked list
setos_node* head;

int setos_init(void)
{
	head = (setos_node*) malloc(sizeof(setos_node));
	head->next = NULL;
	head->key = INT_MIN;

	//initialize the lock
	if (pthread_mutex_init(&_lock, NULL) != 0)
	{
		printf("mutex init failed\n");
		return -1;
	}

	return 1;
}


int setos_free(void)
{
	free(head);

	//free the lock.
	pthread_mutex_destroy(&_lock);
}

int setos_add(int key, void* value)
{

	int rc = pthread_mutex_lock(&_lock);
	if (rc != 0)
	{
		perror("cannot acquire lock\n");
	}


	volatile setos_node* node = head;
	volatile setos_node* prev;

	// go through nodes until the key of "node" exceeds "key"
	// new node should then be between "prev" and "node"
	while ((node != NULL) && (node->key < key)) {
		prev = node;
		node = node->next;
	}

	// if node is same as our key - key already in list
	if ((node != NULL) && (node->key == key))
	{
		pthread_mutex_unlock(&_lock);
		return 0;
	}
	// allocate new node and initialize it
	setos_node* new_node = (setos_node*) malloc(sizeof(setos_node));
	new_node->key = key;
	new_node->value = value;

	// place node into list
	new_node->next = node;
	prev->next = new_node;

	pthread_mutex_unlock(&_lock);
	return 1;
}

int setos_remove(int key, void** value)
{

	int rc = pthread_mutex_lock(&_lock);
	if (rc != 0)
	{
		perror("cannot acquire lock\n");
	}

	volatile setos_node* node = head->next;
	volatile setos_node* prev = head;

	// go through all nodes
	while (node != NULL) {
		if (node->key == key) {			// found node
			if (value != NULL)			// set value if needed
				*value = node->value;

			prev->next = node->next;	// remove node from list
			pthread_mutex_unlock(&_lock);
			return 1;					// (we don't deal with memory leaks)
		}

		prev = node;					// proceed to next node
		node = node->next;
	}

	pthread_mutex_unlock(&_lock);
	return 0;
}

int setos_contains(int key)
{

	int rc = pthread_mutex_lock(&_lock);
	if (rc != 0)
	{
		perror("cannot acquire lock\n");
	}
	volatile setos_node* node = head->next;

	// go through all nodes
	while (node != NULL) {
		if (node->key == key)
		{		// found node
			pthread_mutex_unlock(&_lock);
			return 1;
		}
		node = node->next;
	}
	pthread_mutex_unlock(&_lock);
	return 0;
}


int main(void)
{
	printf ("coarse\n" );

	int x = 1;
	if (setos_init()==-1)
	{
		return -1;
	}

	setos_add(1, &x);
	setos_add(2, &x);
	setos_add(3, &x);

	assert(setos_contains(2));
	assert(!setos_contains(4));

	assert(!setos_remove(4,NULL));
	assert(setos_remove(2,NULL));

	assert(!setos_contains(2));

	setos_free();


	printf ("done\n" );

	return 0;
}
