#include "SortedList.h"
#include <string.h>

// alright, it's gonna be circular because that seems like the right idea
void SortedList_insert(SortedList_t *list , SortedListElement_t *element)
{
	// first node is the head so skip it
	SortedListElement_t *cur = list->next;

	// loop to find where to place the given element
	while ((cur->key != NULL) && (strcmp(element->key, cur->key) > 0))
		cur = cur->next;

	// the element should be placed before the current element
	element->prev = cur->prev;
	element->next = cur;

	cur->prev->next = element;
	cur->prev = element;
}

// assuming it's the caller's responsibility to free element and element->key
int SortedList_delete(SortedListElement_t *element)
{
	if ((element->prev)->next != element || (element->next)->prev != element)
		return 1;  // corrupted prev/next pointers

	(element->prev)->next = (element->next);
	(element->next)->prev = (element->prev);
	return 0;
}

SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key)
{
	SortedListElement_t *cur = list->next;

	while ((cur->key != NULL) && (strcmp(key, cur->key) != 0))
		cur = cur->next;

	if (cur->key == NULL) {
		return NULL;
	} else {
		return cur;
	}
}

// TODO: check prev/next pointers while traversing list.
int SortedList_length(SortedList_t *list)
{
	int count = 0;
	SortedListElement_t *cur = list->next;

	while (cur->key != NULL) {
		cur = cur->next;
		count++;
	}

	return count;
}
