#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <getopt.h>
#include <pthread.h>
#include "SortedList.h"

#define FATAL 100

static int t_count = 1;
static int iter_count = 1;
static int estatus = 0;

static pthread_mutex_t m_lock = PTHREAD_MUTEX_INITIALIZER;
static volatile int s_lock = 0;

typedef void (*lock_fun_t)(); // typedef for lock functions
void no_lock() {;}  // default lock/unlock function is no-op
static lock_fun_t lock_fun = no_lock;
static lock_fun_t unlock_fun = no_lock;

static SortedList_t list;
int opt_yield = 0x00;


/* lock functions */
void
lock_mutex()
{
	pthread_mutex_lock(&m_lock);
}

void
unlock_mutex()
{
	pthread_mutex_unlock(&m_lock);
}

void
lock_spin()
{
	while(__sync_lock_test_and_set(&s_lock, 1))
		;
}

void
unlock_spin()
{
	__sync_lock_release(&s_lock);
}


/* option parser */
void
parse_opts(int argc, char *argv[]) {
	int opt, optindex, d;
	struct option longopts[] = {
		{"threads"   , required_argument, NULL, 't'},
		{"iterations", required_argument, NULL, 'i'},
		{"yield"     , required_argument, NULL, 'y'},
		{"sync"      , required_argument, NULL, 's'},
		{0, 0, 0, 0}
	};

	while((opt = getopt_long(argc, argv, "", longopts, &optindex)) != -1) {
		switch (opt) {
			case 't':
				d = atoi(optarg);
				if (d > 0)
					t_count = d;
				break;

			case 'i':
				d = atoi(optarg);
				if (d > 0)
					iter_count = d;
				break;

			case 'y':
				for (int k = 0; optarg[k] != 0; k++) {
					switch (optarg[k]) {
						case 'i':
							opt_yield |= INSERT_YIELD;
							break;
						case 'd':
							opt_yield |= DELETE_YIELD;
							break;
						case 's':
							opt_yield |= SEARCH_YIELD;
							break;
						default:
							fprintf(stderr, "yield option not recognized\n");
					}
				}
				break;

			case 's':
				switch (*optarg) {
					case 'm':
						lock_fun = lock_mutex;
						unlock_fun = unlock_mutex;
						break;
					case 's':
						lock_fun = lock_spin;
						unlock_fun = unlock_spin;
						break;
					default:
						fprintf(stderr, "sync option not recognized\n");
				}

			case '?':
				break;
		}
	}
}


/* print out the statistics that the spec calls for */
void
print_stats(struct timespec end, struct timespec start)
{
	long long diff = (end.tv_sec - start.tv_sec) * 1000000000;
	diff += end.tv_nsec;
	diff -= start.tv_nsec;

	int ops = t_count * iter_count * 2;
	printf("%d threads x %d iterations x (insert + lookup/delete) = %d\n",
			t_count, iter_count, ops);
	printf("elapsed time: %lldns\n", diff);
	printf("per operation: %lldns\n", diff/ops);
}


/* list helper functions */

void
list_init(SortedList_t *list)
{
	list->prev = list;
	list->next = list;
	list->key = NULL;
}

/* sets element->key to a random 5 character string composed of lowercase letters */
void
element_set_rand_key(SortedListElement_t *element)
{
	char randstr[6];
	
	for (int i = 0; i < 5; i++)
		randstr[i] = rand() % 26 + 97; //get a random lowercase letter
	randstr[5] = 0; //null terminate

	void *str = strdup(randstr);
	if (str == NULL) {
		perror("strdup/malloc");
		exit(FATAL);
	}

	element->key = str;
}

SortedListElement_t
**make_element_pile()
{
	// allocate an array of pointers to element arrays (1 element array per thread)
	SortedListElement_t **ptrs = malloc(t_count * sizeof(void *));
	if (ptrs == NULL) {
		perror("malloc");
		exit(FATAL);
	}

	// allocate an array of list elements for each thread
	for (int i = 0; i < t_count; i++) {
		ptrs[i] = malloc(iter_count * sizeof(SortedListElement_t));
		if (ptrs[i] == NULL) {
			perror("malloc");
			exit(FATAL);
		}

		// allocate a random 5 character key for each element
		for (int j = 0; j < iter_count; j++) {
			element_set_rand_key(&ptrs[i][j]);
		}
	}
	
	return ptrs;
}

void
destroy_element_pile(SortedListElement_t **ptrs)
{
	for (int i = 0; i < t_count; i++) {
		SortedListElement_t *elements = ptrs[i]; //get each thread's list of elements
		for (int j = 0; j < iter_count; j++) {
			free((void*)elements[j].key); //free each element's key
		}
		free(ptrs[i]); //free the thread's array of elements
	}
	free(ptrs); //free the array of pointers to arrays of elements
}


/* thread routine */
void
*t_routine(void *arg)
{
	SortedListElement_t *element = (SortedListElement_t *)arg;

	for (int i = 0; i < iter_count; i++) {
		lock_fun();  // lock_fun / unlock_fun are function pointers set by --sync=[ms]
		SortedList_insert(&list, &element[i]);
		unlock_fun();
	}

	lock_fun();
	SortedList_length(&list);
	unlock_fun();

	for (int i = 0; i < iter_count; i++) {
		lock_fun();
		SortedList_delete(SortedList_lookup(&list, element[i].key));
		unlock_fun();
	}

	return NULL;
}

int
main(int argc, char *argv[])
{
	parse_opts(argc, argv);

	// allocate a bunch of memory
	struct timespec start_time, end_time;
	pthread_t *tid = malloc(t_count * sizeof(pthread_t));
	SortedListElement_t **element_pile = make_element_pile();
	list_init(&list);  // list is a global variable

	clock_gettime(CLOCK_MONOTONIC, &start_time);
	for (int i = 0; i < t_count; i++)
		pthread_create(&tid[i], NULL, t_routine, (void*)(element_pile[i]));

	for (int i = 0; i < t_count; i++)
		pthread_join(tid[i], NULL);
	clock_gettime(CLOCK_MONOTONIC, &end_time);

	// print error if list size != 0
	if (SortedList_length(&list) != 0) {
		estatus = FATAL;
		fprintf(stderr, "ERROR: list length is nonzero\n");
	}
	print_stats(end_time, start_time);

	// free memory
	destroy_element_pile(element_pile);
	free(tid);
	return estatus;
}
