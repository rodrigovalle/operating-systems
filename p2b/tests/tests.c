#include "cutest.h"
#include "../SortedList.h"
#include <stdlib.h>

int opt_yield = 0;

/* HELPER FUNCTIONS */

/* takes a pointer to a preallocated SortedList_t struct
 * sets structure members for a circularly linked list */
void list_init(SortedList_t *head)
{
	head->next = head;
	head->prev = head;
	head->key = NULL;
}

/* takes a pointer to a preallocated SortedListElement
 * initializes its key member to the given C string */
void element_set_key(SortedList_t *element, const char* str)
{
	element->key = strdup(str);
}

/* takes a pointer to a preallocated SortedListElement
 * initializes its key member to a random string of length 5 */
void element_set_randkey(SortedList_t *element)
{
	char rand_str[6];

	for (int i = 0; i < 5; i++) {
		rand_str[i] = ((unsigned)rand()) % 26 + 97;
	}
	rand_str[5] = 0; // null terminate

	element_set_key(element, rand_str);
}

/* frees the element's key member */
void element_destroy_key(SortedList_t *element)
{
	free((char *)element->key);
}


/* DIAGNOSTIC FUNCTIONS */

void print_list(SortedList_t *list)
{
	int i = 0;
	SortedListElement_t *cur = list->next;
	printf("\nhead: key=%s\n", list->key);
	while(cur->key != NULL) {
		i++;
		printf("element %d: key=%s\n", i, cur->key);
		cur = cur->next;
	}
}


/* TEST FUNCTIONS */

void test_insert()
{
	SortedList_t list;
	list_init(&list);

	for (int i = 0; i < 100; i++) {
		TEST_CHECK(SortedList_length(&list) == i);

		SortedListElement_t *e = malloc(sizeof(SortedListElement_t));
		element_set_randkey(e);
		SortedList_insert(&list, e);
	}

	TEST_CHECK(SortedList_length(&list) == 100);

	// free list elements
	SortedListElement_t *freeme;
	SortedListElement_t *cur = list.next;
	while (cur->key != NULL) {
		element_destroy_key(cur);
		freeme = cur;
		cur = cur->next;
		free(freeme);
	}
}

void test_delete_lookup()
{
	SortedList_t list;
	list_init(&list);

	SortedListElement_t elements[26];

	// make a SortedList with keys ordered from lowercase a to lowercase z
	for (int i = 0; i < 26; i++) {
		char c[] = { (char)(i + 97), 0 };
		element_set_key(&elements[i], c);
		SortedList_insert(&list, &elements[i]);
	}

	TEST_CHECK(SortedList_lookup(&list, elements[20].key) == &elements[20]);
	SortedList_delete(&elements[20]);
	TEST_CHECK(SortedList_length(&list) == 25);
	TEST_CHECK(SortedList_lookup(&list, elements[20].key) == NULL);
	// SortedList_insert(&list, &elements[20]);
	// SortedList_insert(&list, &elements[20]); TODO: inserting the same element twice corrupts pointers

	SortedListElement_t *cur = list.next;
	while(cur->key != NULL) {
		TEST_CHECK(strcmp(cur->key, elements[20].key) != 0);
		cur = cur->next;
	}
}

/* TODO:
 * void test_delete()
 * void test_lookup()
 * void test_length()
 * int check_sorted(SortedList_t *list);
 */

TEST_LIST = {
	{ "insert 100 elements", test_insert },
	{ "delete an element with lookup", test_delete_lookup },
	{ 0 }
};
