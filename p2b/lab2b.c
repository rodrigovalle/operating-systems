#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <pthread.h>
#include "SortedList.h"

static int t_count = 1;
static int iter_count = 1;
int opt_yield = 0x00;

void
parse_opts(int argc, char *argv[]) {
	int opt, optindex, d;
	struct option longopts[] = {
		{"threads"   , required_argument, NULL, 't'},
		{"iterations", required_argument, NULL, 'i'},
		{"yield"     , optional_argument, NULL, 'y'},
		{0, 0, 0, 0}
	};

	while((opt = getopt_long(argc, argv, "", longopts, &optindex)) != -1) { switch (opt) {
			case 't':
				d = atoi(optarg);
				if (d > 0)
					t_count = d;
				break;

			case 'i':
				d = atoi(optarg);
				if (d > 0)
					t_count = d;
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
					}
				}
				break;

			case '?':
				break;
		}
	}
}

/* DEBUG */
// TODO: void print_state() ?
void
print_args()
{
	printf("Arguments:\n");
	printf("t_count: %d\n", t_count);
	printf("iter_count: %d\n", iter_count);
	printf("opt_yield: %x\n", opt_yield);
}

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

int
main(int argc, char *argv[])
{
	parse_opts(argc, argv);

	// initialize empty list
	SortedList_t list;
	list.key = NULL;
}
