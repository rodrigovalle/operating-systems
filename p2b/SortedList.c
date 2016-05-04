#define _GNU_SOURCE
#include "SortedList.h"
#include <string.h>
#include <pthread.h>


void SortedList_insert(SortedList_t *list, SortedListElement_t *element)
{
	SortedListElement_t *prev_el = list;
	SortedListElement_t *next_el = list->next;

	while (next_el != list) {
		if (strcmp(element->key, next_el->key) <= 0)
			break;
		prev_el = next_el;
		next_el = next_el->next;
	}

	if (opt_yield & INSERT_YIELD)
		pthread_yield();

	element->prev = prev_el;
	element->next = next_el;
	prev_el->next = element;
	next_el->prev = element;
}


/* it's the caller's responsibility to free any associated memory allocations */
int SortedList_delete(SortedListElement_t *element)
{
	SortedListElement_t *prev_el = element->prev;
	SortedListElement_t *next_el = element->next;

	if (prev_el->next != element || next_el->prev != element)
		return 1;

	if (opt_yield & DELETE_YIELD)
		pthread_yield();

	prev_el->next = next_el;
	next_el->prev = prev_el;

	element->prev = NULL;
	element->next = NULL;

	return 0;
}


SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key)
{
	SortedListElement_t *el = list->next;

	while (el != list) {
		if (strcmp(key, el->key) == 0)
			return el;

		if (opt_yield & SEARCH_YIELD)
			pthread_yield();

		el = el->next;
	}

	return NULL;
}


int SortedList_length(SortedList_t *list)
{
	int count = 0;
	SortedListElement_t *el = list->next;

	while (el != list) {
		if (el->prev->next != el || el->next->prev != el)
			return -1;

		if (opt_yield & SEARCH_YIELD)
			pthread_yield();

		el = el->next;
		count++;
	}

	return count;
}
