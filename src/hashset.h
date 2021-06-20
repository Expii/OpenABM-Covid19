/*
 * hashset.h
 *
 *  Created on: 15 Jun 2021
 *      Author: Howard Halim
 */

#ifndef HASHSET_H_
#define HASHSET_H_

#include <stdbool.h>

typedef struct elem elem;

struct elem {
	short shift;
	int key;
};

typedef struct hashset hashset;

struct hashset {
	short log_size;
	unsigned int bitmask;
	unsigned int size;
	unsigned int n_data;	// there are actually n_data+1 elements in data,
							// but the last one isn't used as data and can be ignored
	elem *data;
};

unsigned int hash(int n);
short log_2(unsigned int n);

hashset *init_set(void);
void destroy_set(hashset *hs);

bool set_empty(hashset *hs);
unsigned int set_size(hashset *hs);
void set_resize(hashset *hs, unsigned int size);

void set_insert(hashset *hs, int key);
//void set_remove(hashset *hs, int key);
bool set_contains(hashset *hs, int key);


#endif
