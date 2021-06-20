/*
 * hashset.c
 *
 *  Created on: 15 Jun 2021
 *      Author: Howard Halim
 */

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include "hashset.h"

/*
 * Uses the FNV-1a hash algorithm
 */
unsigned int hash(int n)
{
	unsigned char *c = (void *)&n;
	unsigned char *end = (void *)(&n+1);
	unsigned int h = 2166136261U;
	while (c != end) {
		h ^= *c++;
		h += (h<<24) + (h<<8) + (h<<7) + (h<<7) + (h<<4) + (h<<1);
	}
    return h;
}

/*
 * floor of log_2(n)
 * n must be positive
 */
short log_2(unsigned int n) {
	return __builtin_clz(n) ^ 31;
}

hashset *init_set(void) {
	hashset *hs = malloc(sizeof(hashset));
	if (hs == NULL)
		return NULL;

	hs->log_size = -1;
	hs->n_data = 0;
	return hs;
}

void destroy_set(hashset *hs) {
	if (hs) {
		free(hs->data);
		free(hs);
	}
}

bool set_empty(hashset *hs) {
	return hs == NULL || hs->log_size == -1;
}

unsigned int set_size(hashset *hs) {
	return set_empty(hs) ? 0 : hs->size;
}

void set_resize(hashset *hs, unsigned int size) {
	if (hs == NULL)
		return;

	if (size < 8)
		size = 8;
	short log_size = log_2(size-1) + 1;

	if (log_size > hs->log_size) {
		unsigned int old_n_data = hs->n_data;
		elem *old_data = hs->data;

		hs->log_size = log_size;
		hs->size = 0; // will be incremented back to original value after reinserting
		hs->bitmask = (1 << hs->log_size) - 1;
		hs->n_data = hs->bitmask + hs->log_size;
		hs->data = calloc(hs->n_data + 1, sizeof(elem));

		for (unsigned int i = 0; i < hs->n_data; i++)
			hs->data[i].shift = -1; // empty
		hs->data[hs->n_data].shift = 0; // mark as non-empty

		for (unsigned int i = 0; i < old_n_data; i++) {
			if (old_data[i].shift >= 0)
				set_insert(hs, old_data[i].key);
		}

		free(old_data);
	}
}

void insert_at(hashset *hs, int key, elem *p, short shift) {
	if (p->shift == -1) {
		p->shift = shift;
		p->key = key;
		hs->size++;
	}
	else {
		short tmp1 = p->shift;
		p->shift = shift;
		shift = tmp1;
		int tmp2 = p->key;
		p->key = key;
		key = tmp2;
		do {
			p++, shift++; // current position doens't work, try next one
			if (shift == hs->log_size) {
				set_resize(hs, hs->bitmask<<1);
				set_insert(hs, key);
				return;
			}
		} while (p->shift >= shift);
		insert_at(hs, key, p, shift);
	}
}

void set_insert(hashset *hs, int key) {
	if (hs == NULL)
		return;

	if (set_empty(hs))
		set_resize(hs, 1);

	unsigned int idx = hash(key) & hs->bitmask;
	elem *p = &(hs->data[idx]);
	short shift = 0;

	for (; p->shift >= shift; p++, shift++) {
		if (p->key == key)
			return; // key already exists in set
	}
	if (shift == hs->log_size || hs->size > 0.9 * hs->bitmask) {
		set_resize(hs, hs->bitmask<<1);
		set_insert(hs, key);
		return;
	}
	insert_at(hs, key, p, shift);
}

bool set_contains(hashset *hs, int key) {
	unsigned int idx = hash(key) & hs->bitmask;
	elem *p = &(hs->data[idx]);
	for (short shift = 0; p->shift >= shift; p++, shift++) {
		if (p->key == key)
			return true;
	}
	return false;
}
