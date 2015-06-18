#ifndef _SETOS_H
#define _SETOS_H

/**
* File:			setos.h
* Author:		Moshe Sulamy
* Description:	Header file of concurrent set structure
*				for OS course 15b at Tel-Aviv University
**/

int setos_init(void);
int setos_free(void);

// add the specified value with the given key
// returns 0 if the key is already in the Set, and 1 otherwise
int setos_add(int key, void* value);

// remove specified item and set its value into "value"
// returns 0 if the key does not exist, and 1 otherwise
int setos_remove(int key, void** value);

// determine whether the given key is in the Set
// returns 0 if it isn't in the set, and 1 otherwise
int setos_contains(int key);

#endif
