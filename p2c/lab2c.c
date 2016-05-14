#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <getopt.h>
#include <pthread.h>
#include <stdint.h>
#include "SortedList.h"

#define FATAL 100

union lock_var {
    volatile int s_lock;
    pthread_mutex_t m_lock;
};
typedef union lock_var lock_var_t;
// the correct lock_var type is selected by the
// list_lock() list_unlock() functions

static enum sync_type {
    NONE,
    SPIN,
    MUTEX
} sync = NONE;  //global sync type

static int t_count = 1;
static int iter_count = 1;
static int list_count = 1;
static int estatus = 0;

static SortedList_t *sublist;  //global sublist array
static lock_var_t *locklist;   //global lock array
int opt_yield = 0x00;


/* lock functions */

/* lock the sublist at the specified index */
void lock_sublist(unsigned int l_index)
{
    switch (sync) {
        case SPIN:
            while(__sync_lock_test_and_set(&locklist[l_index].s_lock, 1))
                ;
            break;
        case MUTEX:
            pthread_mutex_lock(&locklist[l_index].m_lock);
            break;
        case NONE:
        default:
            break;
    }
}

/* unlock the sublist at the specified index */
void unlock_sublist(unsigned int l_index)
{
    switch (sync) {
        case SPIN:
            __sync_lock_release(&locklist[l_index].s_lock);
            break;
        case MUTEX:
            pthread_mutex_unlock(&locklist[l_index].m_lock);
            break;
        case NONE:
        default:
            break;
    }
}


/* option parser */
void parse_opts(int argc, char *argv[]) {
    int opt, optindex, d;
    struct option longopts[] = {
        {"threads"   , required_argument, NULL, 't'},
        {"iterations", required_argument, NULL, 'i'},
        {"yield"     , required_argument, NULL, 'y'},
        {"sync"      , required_argument, NULL, 's'},
        {"lists"     , required_argument, NULL, 'l'},
        {0, 0, 0, 0}
    };

    while((opt = getopt_long(argc, argv, "", longopts, &optindex)) != -1) {
        switch (opt) {
            case 't':  //--threads=#
                d = atoi(optarg);
                if (d > 0)
                    t_count = d;
                break;

            case 'i':  //--iterations=#
                d = atoi(optarg);
                if (d > 0)
                    iter_count = d;
                break;

            case 'y':  //--yield=[ids]
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
                            fprintf(stderr, "yield option -%c not recognized\n", *optarg);
                    }
                }
                break;

            case 's':  //--sync=[ms]
                switch (*optarg) {
                    case 'm':
                        sync = MUTEX;
                        break;
                    case 's':
                        sync = SPIN;
                        break;
                    default:
                        fprintf(stderr, "sync option -%c not recognized\n", *optarg);
                }
                break;

            case 'l':  //--lists=#
                d = atoi(optarg);
                if (d > 0)
                list_count = d;

            case '?':
                break;
        }
    }
}


/* print out the statistics that the spec calls for */
void print_stats(struct timespec end, struct timespec start)
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

/* initialize a circular linked list */
void list_init(SortedList_t *list)
{
    list->prev = list;
    list->next = list;
    list->key = NULL;
}

/* sets the given element's key to a random 5 character string composed of
 * lowercase letters */
void element_set_rand_key(SortedListElement_t *element)
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

/* make a 2d array -- an array of pointers to initialized element arrays */
SortedListElement_t **make_element_pile()
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

/* free the memory allocated by make_element_pile() */
void destroy_element_pile(SortedListElement_t **ptrs)
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

/* implemented using the FNV-1a 32 bit hash algorithm, found here:
 * www.isthe.com/chongo/tech/comp/fnv/#FNV-1a
 *
 * NOTE: currently expects an element with a non null key */
uint32_t hash_element(SortedListElement_t *e)
{
    const uint8_t *str = (uint8_t *)e->key;
    uint8_t c;
    uint32_t hash = 2166136261;

    while ((c = *str++)) {
        hash ^= c;
        hash *= 16777619;
    }

    return hash;
}


/* thread routine */
void *t_routine(void *arg)
{
    int li, sum;
    SortedListElement_t *elements = (SortedListElement_t *)arg;

    // hash and insert elements into a sublist
    for (int k = 0; k < iter_count; k++) {
        li = hash_element(&elements[k]) % list_count; //get list index

        lock_sublist(li);
        SortedList_insert(&sublist[li], &elements[k]);
        unlock_sublist(li);
    }

    // lock and enumerate all lists
    sum = 0;
    for (int k = 0; k < list_count; k++) {
        lock_sublist(k);
        sum += SortedList_length(&sublist[k]);
        unlock_sublist(k);
    }

    // hash and remove deleted 
    for (int k = 0; k < iter_count; k++) {
        li = hash_element(&elements[k]) % list_count; //get list index

        lock_sublist(li);
        SortedList_delete(SortedList_lookup(&sublist[li], elements[k].key));
        unlock_sublist(li);
    }

    return NULL;
}

void init_sublists()
{
    sublist = malloc(list_count * sizeof(SortedList_t));
    locklist = malloc(list_count * sizeof(lock_var_t));

    for (int i = 0; i < list_count; i++) {
        // initialize the sublists
        list_init(&sublist[i]);

        // initialize the lock list according to the lock type
        switch (sync) {
            case SPIN:
                locklist[i].s_lock = 0;
                break;
            case MUTEX:
                pthread_mutex_init(&locklist[i].m_lock, NULL);
                break;
            case NONE:
            default:
                break;
        }
    }
}

/* it's good practice right? */
void free_sublists()
{
    free(sublist);
    free(locklist);
}

int main(int argc, char *argv[])
{
    parse_opts(argc, argv);

    // do some memory allocations
    struct timespec start_time, end_time;
    pthread_t *tid = malloc(t_count * sizeof(pthread_t));
    SortedListElement_t **element_pile = make_element_pile();
    init_sublists();

    // run + time the threads
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    for (int i = 0; i < t_count; i++)
        pthread_create(&tid[i], NULL, t_routine, (void*)(element_pile[i]));

    for (int i = 0; i < t_count; i++)
        pthread_join(tid[i], NULL);
    clock_gettime(CLOCK_MONOTONIC, &end_time);

    print_stats(end_time, start_time);

    // free memory
    destroy_element_pile(element_pile);
    free_sublists();
    free(tid);
    return estatus;
}
