/**
* File:			setos-coarse.c
* Author:		Uriya B
* Description:	Source file of simple set structure
*				for OS course 15b at Tel-Aviv University
**/

#include "setos.h"
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <assert.h>
#define NUMTHREADS 4

// mutex and cond variable
pthread_mutex_t sharedElementMutex; // mutex for access to shared set
int num=0;

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
	int rc=0;	
	rc = pthread_mutex_init(&sharedElementMutex, NULL); assert(rc==0);
	head = (setos_node*) malloc(sizeof(setos_node));
	head->next = NULL;
	head->key = INT_MIN;
	
	
}
int setos_free(void)
{
	pthread_mutex_destroy(&sharedElementMutex);
	free(head);
}

int setos_add(int key, void* value)
{
	printf ("%u wants to add key:  %d\n ", pthread_self(), key);
	int rc = pthread_mutex_lock(&sharedElementMutex); assert(!rc);
	volatile setos_node* node = head;
	volatile setos_node* prev;

	// go through nodes until the key of "node" exceeds "key"
	// new node should then be between "prev" and "node"
	while ((node != NULL) && (node->key < key)) {
		printf ("curr key: %d\n ", node->key);
		prev = node;
		node = node->next;
		
	}

	// if node is same as our key - key already in list
	if ((node != NULL) && (node->key == key)){	
		printf(" item already inside\n");
		rc = pthread_mutex_unlock(&sharedElementMutex); assert(!rc);
		printf ("unlock: %d\n ",rc);
		return 0;
	}
	

	//puts ("adding item\n");
	// allocate new node and initialize it
	setos_node* new_node = (setos_node*) malloc(sizeof(setos_node));
	new_node->key = key;
	new_node->value = value;
	
	// place node into list
	new_node->next = node;
	prev->next = new_node;

	rc = pthread_mutex_unlock(&sharedElementMutex); assert(!rc);
	printf ("%d IS ADDED, unlock: %d\n ", new_node->key, rc);
	return 1;
}

int setos_remove(int key, void** value)
{
	int rc;
	volatile setos_node* node = head->next;
	volatile setos_node* prev = head;
	rc = pthread_mutex_lock(&sharedElementMutex); assert(!rc);
	// go through all nodes
	while (node != NULL) {
		if (node->key == key) {			// found node
			
			if (value != NULL)			// set value if needed
				*value = node->value;

			prev->next = node->next;	// remove node from list
			printf ("%u is REMOVING node %d\n",pthread_self(), node->key);
			rc = pthread_mutex_unlock(&sharedElementMutex); assert(!rc);
			return 1;					// (we don't deal with memory leaks)
		}
		
		prev = node;					// proceed to next node
		node = node->next;
	}
	printf ("%u has nothing to remove\n",pthread_self());
	rc = pthread_mutex_unlock(&sharedElementMutex); assert(!rc);
	return 0;
}

//DOESN'T CHANGE THE SET SO NO NEED FOR LOCK
int setos_contains(int key)
{
	printf ("%u is searching for %d\n",pthread_self(), key);
	int rc = pthread_mutex_lock(&sharedElementMutex); assert(!rc);
	volatile setos_node* node = head->next;

	// go through all nodes
	while (node != NULL) {
		if (node->key == key)	{// found node
			rc = pthread_mutex_unlock(&sharedElementMutex); assert(!rc);
			printf ("%d IS FOUND\n ", key);
			return 1;
		}
		node = node->next;
	}
	rc = pthread_mutex_unlock(&sharedElementMutex); assert(!rc);
	printf ("%d IS not FOUND \n ", key);	
	return 0;
}

void *theThread(void *parm){
   int rc, x=1;
 
   printf("Thread %u: Entered, with param: %d \n", pthread_self(), *((int*)parm));
	x=num;
   setos_add(num++, &x);
   setos_contains (x);
   setos_remove(x,NULL);
   printf("Thread %u: Ended, with param: %d \n", pthread_self(), *((int*)parm));
   return NULL; // exiting gracefuly
}
int main(void)
{
	int rc,x = 1;
	puts ("init\n");	
	setos_init();

	pthread_t thread[NUMTHREADS];
 	 
  	 
  	 int i;
 
  	 /* init mutex and cond variable */
  	 rc = pthread_mutex_init(&sharedElementMutex, NULL); assert(rc==0);
  	 //rc = pthread_cond_init(&canConsumeCondition, NULL); assert(rc==0);

  	 printf("Create/start threads\n");
  	 for (i=0; i <NUMTHREADS; i++) {
	printf("%d\n", i);
   	   rc = pthread_create(&thread[i], NULL, theThread, &i); assert(!rc);
  	 }
   	sleep(3); // give time for all consumers to start

	/*
	puts ("add 1\n");
	rc= setos_add(1, &x);
	printf ("addition result:  %d\n ", rc);	

	//printf ("%d", pthread_mutex_trylock (&sharedElementMutex));
	puts ("add 2");
	setos_add(2, &x);
	puts ("add 3");
	setos_add(3, &x);
*/
	puts("");
	puts ("contains");
	assert(setos_contains(1));

	assert(!setos_contains(4));
	/*puts ("remove");
	assert(!setos_remove(4,NULL));
	assert(setos_remove(1,NULL));
	puts ("contains 2");
	assert(!setos_contains(2));
*/
	puts ("free");
	setos_free();
	printf ("end");
	return;
}


 
