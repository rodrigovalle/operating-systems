#define _GNU_SOURCE
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>

#define FATAL 1

static int t_count = 1;
static int iter_count = 1;
static int opt_yield = 0;
static long long count = 0;

void
add(long long *pointer, long long value)
{
	long long sum = *pointer + value;
	if (opt_yield)
		pthread_yield();
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
		{"threads"   , required_argument, NULL, 't'},
		{"iterations", required_argument, NULL, 'i'},
		{"yield"     , no_argument, &opt_yield,  1 },
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

/* calculates the time difference in nano seconds */
long long
time_diff(struct timespec end, struct timespec start)
{
	long long diff = (end.tv_sec - start.tv_sec) * 1000000000;
	diff += end.tv_nsec;
	diff -= start.tv_nsec;
	return diff;
}

int
main(int argc, char *argv[])
{
	struct timespec start_time, end_time;
	long long elapsed_time, ops;
	pthread_t *tid;

	parse_opts(argc, argv); // this should always be first (updates t_count);
	tid = malloc(t_count * sizeof(pthread_t));

	// ladies and gentlemen, start your engines (it's a race! get it?)
	if (clock_gettime(CLOCK_MONOTONIC, &start_time) == -1)
		terminate("clock");

	// start threads -- and they're off
	for (int i = 0; i < t_count; i++)
		if ((errno = pthread_create(&tid[i], NULL, t_routine, NULL)) != 0)
			terminate("thread creation");

	// wait until threads are done
	for (int i = 0; i < t_count; i++)
		if ((errno = pthread_join(tid[i], NULL)) != 0)
			terminate("thread join");

	// clocking in at the finish line
	if (clock_gettime(CLOCK_MONOTONIC, &end_time) == -1)
		terminate("clock");

	// print some statistics
	ops = t_count * iter_count * 2;
	printf("%d threads x %d iterations x (add + subtract) = %lld operations\n",
			t_count, iter_count, ops);

	// check for errors
	if (count != 0)
		fprintf(stderr, "ERROR: final count = %lld\n", count);

	// calculate the elapsed time
	elapsed_time = time_diff(end_time, start_time);
	printf("elapsed time: %lld\n", elapsed_time);
	printf("per opt: %lld\n", elapsed_time/ops);
}
