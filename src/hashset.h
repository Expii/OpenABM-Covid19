/*
 * hashset.h
 *
 *  Created on: 15 Jun 2021
 *      Author: Howard Halim
 */

#ifndef HASHSET_H_
#define HASHSET_H_

#include <stdbool.h>
#include <stdint.h>

typedef long key_type; // the type of values being inserted into the set

typedef struct elem elem;

struct elem {
	short shift;
	key_type key;
};

typedef struct hashset hashset;

struct hashset {
	short log_size;
	uint32_t bitmask;
	uint32_t size;
	uint32_t n_data;	// there are actually n_data+1 elements in data,
							// but the last one isn't used as data and can be ignored
	elem *data;
};

uint32_t hash(key_type key);
short log_2(uint32_t n);

hashset *init_set(void);
void destroy_set(hashset *hs);

bool set_empty(hashset *hs);
uint32_t set_size(hashset *hs);
void set_resize(hashset *hs, uint32_t size);

void set_insert(hashset *hs, key_type key);
//void set_remove(hashset *hs, key_type key);
bool set_contains(hashset *hs, key_type key);

key_type *set_to_list(hashset *hs);

#endif
