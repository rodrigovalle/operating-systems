#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#include <unistd.h>
#include <getopt.h>

#include <stdio.h>

void blow_chunks()
{
	char* wrong = NULL;
	*wrong = 0;
}

void replace_fd(int old_fd, int new_fd)
{
	if(new_fd >= 0) {
		close(old_fd);
		dup(new_fd);
		close(new_fd);
	}
}

int main(int argc, char *argv[])
{
	int o;
	int new_fd;
	int mode;
	int opt_index = 0;
	static struct option long_opts[] =
	{
		{"input",    required_argument, 0, 'i'},
		{"output",   required_argument, 0, 'o'},
		{"segfault", no_argument,       0, 's'},
		{"catch",    no_argument,       0, 'c'},
		{0, 0, 0, 0}
	};

	while ((o = getopt_long(argc, argv, "i:o", long_opts, &opt_index)) != -1)
	{
		switch (o) {
			case 'i':
				new_fd = open(optarg, O_RDONLY);
				replace_fd(STDIN_FILENO, new_fd);
				break;
			case 'o':
				mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
				new_fd = creat(optarg, mode);
				replace_fd(STDOUT_FILENO, new_fd);
				break;
			case 's':
				// force a segfault
				blow_chunks();
				break;
			case 'c':
				break;
			case '?':
				break;
			default:
				break;
		}
	}

	char* c;
	while (read(STDIN_FILENO, c, 1) > 0)
	{
		write(STDOUT_FILENO, c, 1);
	}

	close(STDIN_FILENO);
	close(STDOUT_FILENO);

	return 0;
}
