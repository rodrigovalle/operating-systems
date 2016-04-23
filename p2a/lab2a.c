#include <pthread.h>
#include <getopt.h>

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#define FATAL 1

static int t_count = 1;
static int iter_count = 1;
static long long count = 0;

void
add(long long *pointer, long long value)
{
	long long sum = *pointer + value;
	*pointer = sum;
}

void
usage(char* name)
{
	printf("Usage: %s [--thread=<thread_count>] [--iterations=<iter_count>]\n", name);
}

void
parse_opts(int argc, char* argv[])
{
	int o, o_index, d;
	struct option longopts[] = {
		{"threads",    required_argument, NULL, 't'},
		{"iterations", required_argument, NULL, 'i'},
		{0, 0, 0, 0}
	};

	while((o = getopt_long(argc, argv, "", longopts, &o_index)) != -1) {
		switch (o) {
			case 't':
				d = atoi(optarg);
				if (d > 0)  // these inputs better be valid (otherwise default to 1)
					t_count = d;
				break;
			case 'i':;
				d = atoi(optarg);
				if (d > 0)
					iter_count = d;
				break;
			case '?':
				usage(argv[0]);
				exit(FATAL);
		}
	}
}

void
terminate(char *msg)
{
	perror(msg);
	exit(FATAL);
}

void
*t_routine(__attribute__((unused)) void *arg)
{
	for (int i = 0; i < iter_count; i++) {
		add(&count, 1);
	}

	for (int i = 0; i < iter_count; i++) {
		add(&count, -1);
	}
	
	return NULL;
}

int
main(int argc, char *argv[])
{
	struct timespec tp;
	pthread_t tid[t_count]; // variable length array, dunno how safe this is

	parse_opts(argc, argv);
	if (clock_gettime(CLOCK_MONOTONIC, &tp) == -1)
		terminate("clock");

	// start threads
	for (int i = 0; i < t_count; i++)
		if ((errno = pthread_create(&tid[i], NULL, t_routine, NULL)) != 0)
			terminate("thread creation");

	// wait until threads are done
	for (int i = 0; i < t_count; i++)
		if ((errno = pthread_join(tid[i], NULL)) != 0) // segfault, but why?
			terminate("thread join");

	// print some statistics
	printf("%d threads x %d iterations x (add + subtract) = %d operations\n",
			t_count, iter_count, t_count * iter_count * 2);

	if (count != 0)
		fprintf(stderr, "ERROR: final count = %lld\n", count);

	// printf("elapsed time: %d\n"); TODO: calculate elapsed time, avg time
	// printf("per opt: %d\n");
}
