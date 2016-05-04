#include "cutest.h"
#include "../SortedList.h"
#include <stdlib.h>
#include <string.h>

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
 * initializes its key member to the given C string */ void element_set_key(SortedList_t *element, const char* str)
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

/* returns 1 if true, 0 if false or corrupted pointers */
int is_sorted(SortedList_t *list) // TODO: segfaults
{
	SortedListElement_t *el = list->next;

	while (el != list) {
		if (el->prev->next != el || el->next->prev != el)
			return 0;
		if (strcmp(el->prev->key, el->key) > 0)
			return 0;
		el = el->next;
	}

	return 1;
}

/* TEST FUNCTIONS */

void test_insert()
{
	SortedList_t list;
	SortedListElement_t element[100];
	list_init(&list);

	for (int i = 0; i < 100; i++) {
		element_set_randkey(&element[i]);
		SortedList_insert(&list, &element[i]);
		//TEST_CHECK(is_sorted(&list));
	}
}

void test_length()
{
	SortedList_t list;
	SortedListElement_t element[100];
	list_init(&list);

	for (int i = 0;; i++) {
		TEST_CHECK(SortedList_length(&list) == i);
		if (i >= 100)
			break;

		element_set_randkey(&element[i]);
		SortedList_insert(&list, &element[i]);
	}
}

void test_corrupt_length()
{
	SortedList_t list;
	SortedListElement_t element[100];
	list_init(&list);

	for (int i = 0; i < 100; i++) {
		element_set_randkey(&element[i]);
		SortedList_insert(&list, &element[i]);
	}

	element[99].next = element[99].prev;
	// TODO: setting to NULL causes segfault

	TEST_CHECK(SortedList_length(&list) == -1);
}

void test_lookup()
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

	TEST_CHECK(SortedList_lookup(&list, "z") == &elements[25]);
	TEST_CHECK(SortedList_lookup(&list, "") == NULL);
}

void test_delete()
{
	SortedList_t list;
	list_init(&list);

	SortedListElement_t elements[26];
	for (int i = 0; i < 26; i++) {
		char c[] = { (char)(i+97), 0 };
		element_set_key(&elements[i], c);
		SortedList_insert(&list, &elements[i]);
	}

	TEST_CHECK(SortedList_delete(&elements[25]) == 0);
	TEST_CHECK(SortedList_length(&list) == 25);
	//TEST_CHECK(is_sorted(&list));

	SortedListElement_t *e = list.next;
	while (e != &list) {
		TEST_CHECK(strcmp(e->key, elements[25].key) != 0);
		e = e->next;
	}
}

void test_corrupt_delete() {
	SortedListElement_t e;
	// e.next = e.prev = NULL; TODO: causes segfault
	e.next = &e + 1;
	e.prev = &e + 2;
	e.key = NULL;
	TEST_CHECK(SortedList_delete(&e) == 1);

	e.next = &e;
	TEST_CHECK(SortedList_delete(&e) == 1);

	e.prev = &e;
	TEST_CHECK(SortedList_delete(&e) == 0);
}


TEST_LIST = {
	{ "insert 100 elements", test_insert },
	{ "length", test_length },
	{ "corrupt length", test_corrupt_length },
	{ "lookup", test_lookup },
	{ "delete", test_delete },
	{ "corrupt delete", test_corrupt_delete },
	{ 0 }
};
