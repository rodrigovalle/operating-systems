/* Rodrigo Valle
 * 104 494 120
 */

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <termios.h>
#include <getopt.h>
#include <pthread.h>
#include <errno.h>

#include <stdio.h>
#include <stdlib.h>

#define BUFSIZE 128

static struct termios termdef;
static int shflg = 0;

/* 
 * parses the --shell command line option
 * exits with code 2 if invalid flags are found
 */
void
parse_opts(int argc, char* argv[])
{
	char opt;
	int optindex;
	struct option longopts[] = {
		{"shell", no_argument, &shflg, 1},
		{0, 0, 0, 0}
	};

	while ((opt = getopt_long(argc, argv, "", longopts, &optindex)) != -1) {
		switch (opt) {
			case '?':
				exit(2);
		}
	}
}

/*
 * stores the default terminal settings and sets noncanonical no-echo mode
 */
void
setup_terminal()
{
	struct termios termopts;
	if (tcgetattr(STDERR_FILENO, &termdef) < 0)
		perror("tcgeattr");

	termopts = termdef;
	termopts.c_lflag &= ~(ICANON | ECHO | ISIG);
	termopts.c_cc[VTIME] = 0;
	termopts.c_cc[VMIN] = 1;

	if (tcsetattr(STDERR_FILENO, TCSAFLUSH, &termopts) < 0)
		perror("tcsetattr");
}

/*
 * restores the terminal to default settings -- gets called on exit
 * also handles reaping the terminated shell's exit status if shflg is set
 */
void
restore_terminal()
{
	if (tcsetattr(STDERR_FILENO, TCSAFLUSH, &termdef) < 0)
		fprintf(stderr, "unable to restore default terminal settings\n");

	if (shflg) {
		int wstat;

		if (waitpid(-1, &wstat, WNOHANG) < 0) {
			perror("waitpid");
			return;
		}

		if (WIFEXITED(wstat)) {
			printf("shell exited with status %d\n", WEXITSTATUS(wstat));
		} else {
			fprintf(stderr, "shell exited abnormally\n");
		}
	}
}

/* * Handles SIGPIPE by exiting with return code 1. 
 * remember, exiting always causes the restore_terminal() function to run
 */
void
sighandler(__attribute__((unused)) int sig)
{
	// recieved SIGPIPE
	exit(1);
}

/*
 * Creates a new shell process, setting up two pipes for communcation with the
 * shell process. Exits with code 2 if the fork fails.
 *
 * Stores
 * - keybd_out: a pipe from the keybd (parent) process to the shell process
 * - sh_out: a pipe from the shell process to the keybd (parent) process.
 * 
 * Returns: the process id of the shell
 * NOTE: this function only returns in the parent process.
 */
pid_t
spawn_shell(int keybd_out[2], int sh_out[2])
{
	// create the pipes
	if (pipe(keybd_out) < 0 || pipe(sh_out) < 0) {
		perror("pipe");
		exit(2);
	}

	pid_t pid = fork();
	if (pid == 0) { // child (shell) process
		// close the keybd process's pipe ends:
		close(sh_out[0]);    // the read end of the shell output pipe
		close(keybd_out[1]); // the write end of the keyboard output pipe
		
		/* do some plumbing, connect the
		 * - keyboard output to the shell's STDIN
		 * - shell's STDOUT to the shell output pipe
		 * - shell's STDERR to the shell output pipe */
		dup2(keybd_out[0], STDIN_FILENO);
		dup2(sh_out[1], STDOUT_FILENO);
		dup2(sh_out[1], STDERR_FILENO);

		// clean up the pipe file descriptors
		//close(keybd_out[0]);
		//close(sh_out[1]);

		if (execl("/bin/bash", "/bin/bash", NULL) < 0)
			perror("shell: exec");

	} else if (pid > 0) { // parent (keybd) process
		// close the shell process's pipe ends:
		close(sh_out[1]);    // the write end of the shell output pipe
		close(keybd_out[0]); // the read end of the keyboard output pipe

	} else {
		perror("fork");
		exit(2);
	}
	
	return pid; // this is only executed in the parent process
}

/*
 * A thread routine to process the output of the shell background process.
 * On receiving EOF from the shell output pipe, the entire program will exit
 * with code 1, restoring terminal settings in the process.
 * 
 * Parameters:
 * - void* arg: an int* that contains a file descriptor of the read end of the
 *   shell output pipe
 */
void
*sh_listener(void *arg)
{
	// get the shell pipe file descriptor from the argument
	int pfd = *(int*)arg;

	int r;
	char buf[BUFSIZE];
	while ((r = read(pfd, buf, BUFSIZE)) > 0) {
		write(STDOUT_FILENO, buf, r);
	}

	if (r == 0) { // got EOF from shell
		exit(1);
	} else {
		perror("keybd: couldn't read from shell process");
		exit(2);
	}

	return NULL;
}


int
main(int argc, char *argv[])
{
	int keybd_out[2];
	int sh_out[2];
	pthread_t tid;
	int shpid;

	setup_terminal();
	atexit(restore_terminal);

	parse_opts(argc, argv);

	if (shflg) {
		shpid = spawn_shell(keybd_out, sh_out);
		signal(SIGPIPE, sighandler);

		// spawn thread to handle shell proc I/O
		if ((errno = pthread_create(&tid, NULL, sh_listener, sh_out)) != 0) {
			perror("thread creation");
		}
	}

	ssize_t s;
	char buf[BUFSIZE];
	while ((s = read(STDIN_FILENO, buf, BUFSIZE)) > 0) {
		for (int i = 0; i < s; i++) {
			switch (buf[i]) {
				case '\r':
				case '\n':
					write(STDOUT_FILENO, "\r\n" , 2);
					if (shflg)
						write(keybd_out[1], "\n", 1);
					break;

				case '\004':  // ^D from terminal
					if (shflg) {
						pthread_cancel(tid); // avoid thread race condition
						close(keybd_out[1]); // shell sees EOF
						kill(shpid, SIGHUP); // shell recieves HUP
					}
					exit(0);
					break;

				case '\003':  // ^C from terminal
					if (shflg) {
						kill(shpid, SIGINT);
					} else {
						exit(0);
					}
					break;

				default:
					write(STDOUT_FILENO, buf + i, 1);
					if (shflg)
						write(keybd_out[1], buf + i, 1);
					break;
			}
		}
	}
	if (s < 0) {
		perror("read");
		exit(2);
	}
}
