// Rodrigo Valle
// 104 494 120

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#include <unistd.h>
#include <getopt.h>

#include <stdio.h>
#include <stdlib.h>

// function name inspired by Carey Nachenberg
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

// __attribute__((unused)) supresses -Wunused-parameter warning
void
sigseg_handler(__attribute__((unused)) int signum)
{
	// fprintf and perror are not async safe, see signal(7)
	write(STDERR_FILENO, "segfault caught, exiting", 24); // 24 byte string
	exit(3);
}

void
print_usage(char *argv0)
{
	printf("Usage: %s [--input=FILE] [--output=FILE] [--segfault] [--catch]",
			argv0);
}

int
main(int argc, char *argv[])
{
	int o, mode, new_fd;
	int oindex = 0;
	int segflag = 0;
	struct option long_opts[] =
	{
		{"input",    required_argument, 0,        'i'},
		{"output",   required_argument, 0,        'o'},
		{"segfault", no_argument,       &segflag,  1 },
		{"catch",    no_argument,       0,        'c'},
		{0, 0, 0, 0}
	};

	while ((o = getopt_long(argc, argv, "", long_opts, &oindex)) != -1)
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
				print_usage(argv[0]);
				exit(4);
				break;
		}
	}

	if (segflag) {
		blow_chunks();  // force a segfault
	}

	int stat;
	char buf[1000];  // do a buffered read because it's faster
	while ((stat = read(STDIN_FILENO, &buf, 1000)) > 0)
	{
		if(write(STDOUT_FILENO, &buf, stat) == -1) {
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
