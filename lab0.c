#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#include <unistd.h>
#include <getopt.h>

#include <stdio.h>
#include <stdlib.h>

// BUG(?): when --output and --input refer to the same file, it deletes all
// contents of that file
// Should the program segfault before or after writing to files?
// What should open/creat errors look like with printf and perror?

void
blow_chunks()
{
	char* wrong = NULL;
	*wrong = 0;
}

void
replace_fd(int old_fd, int new_fd)
{
	if(new_fd >= 0) {
		close(old_fd);
		dup(new_fd);
		close(new_fd);
	}
}

void
sigseg_handler(int signum)
{
	fprintf(stderr, "error: segfault caught, exiting");
	exit(3);
}

int
main(int argc, char *argv[])
{
	int o;
	int new_fd;
	int mode;
	int opt_index = 0;
	int segflag = 0;
	struct option long_opts[] =
	{
		{"input",    required_argument, 0,        'i'},
		{"output",   required_argument, 0,        'o'},
		{"segfault", no_argument,       &segflag,  1 },
		{"catch",    no_argument,       0,        'c'},
		{0, 0, 0, 0}
	};

	while ((o = getopt_long(argc, argv, "i:o", long_opts, &opt_index)) != -1)
	{
		switch (o) {
			case 'i':
				if ((new_fd = open(optarg, O_RDONLY)) == -1) {
					perror("open");
					exit(1);
				}
				replace_fd(STDIN_FILENO, new_fd);
				break;
			case 'o':
				mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
				if ((new_fd = creat(optarg, mode)) == -1) {
					perror("creat");
					exit(2);
				}
				replace_fd(STDOUT_FILENO, new_fd);
				break;
			case 'c':
				signal(SIGSEGV, sigseg_handler);
				break;
			case '?':
				break;
			default:
				break;
		}
	}

	if (segflag) {
		blow_chunks(); // force a segfault
	}

	int stat;
	char c = 0;
	while ((stat = read(STDIN_FILENO, &c, 1)) > 0)
	{
		if(write(STDOUT_FILENO, &c, 1) == -1) {
			perror("write");
		}
	}
	if (stat < 0) {
		perror("read");
	}

	close(STDIN_FILENO);
	close(STDOUT_FILENO);

	return 0;
}
