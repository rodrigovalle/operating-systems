#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <getopt.h>
#include <pthread.h>
#include <errno.h>
#include <pthread.h>
#include <mcrypt.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFSIZE 128
#define FATAL 100

static int eflg = 0;
static uint16_t port = 0;

void
parse_opts(int argc, char *argv[])
{
	int opt;
	int optindex;
	struct option longopts[] = {
		{"port", required_argument, NULL, 'p'},
		{"encrypt", no_argument, &eflg, 1},
		{0,0,0,0}
	};

	while ((opt = getopt_long(argc, argv, "", longopts, &optindex)) != -1) {
		switch (opt) {
			case 'p':; // need this semicolon here to appease the gods of C programming
				long int p = strtol(optarg, NULL, 10);
				if (p > UINT16_MAX || p < 0) {
					fprintf(stderr, "invalid port specified\n");
					exit(FATAL);
				}
				port = (uint16_t)p;
				break;

			case '?':
				exit(FATAL);
		}
	}
}

/* Reaps the shell's exit status and prints it to the screen. */
void
reap_sh()
{
	int wstat;

	if (waitpid(-1, &wstat, WNOHANG) < 0) {
		perror("waitpid");
		return;
	}
}

/* Handles SIGPIPE by exiting with return code 1. 
 * remember, exiting always causes the restore_terminal() function to run
 */
void
sighandler(__attribute__((unused)) int sig)
{
	// recieved SIGPIPE
	// - close the network connection
	// - kill the shell
	exit(2);
}

/*
 * Creates a new shell process, setting up two pipes for communcation with the
 * shell process. Exits with code 2 if the fork fails.
 *
 * Stores
 * - int *sh_rd: a file descriptor to write to the shell process
 * - int *sh_wr: a file descriptor to read from the shell process
 * 
 * Returns: the process id of the shell
 * NOTE: this function only returns in the parent process.
 */
pid_t
spawn_shell(int *sh_rd, int *sh_wr)
{
	int srv_out[2], sh_out[2];

	// create the pipes
	if (pipe(srv_out) < 0 || pipe(sh_out) < 0) {
		perror("pipe");
		exit(FATAL);
	}

	pid_t pid = fork();
	if (pid == 0) { // child (shell) process
		// close the server process's pipe ends:
		close(sh_out[0]);    // the read end of the shell output pipe
		close(srv_out[1]); // the write end of the keyboard output pipe
		
		/* do some plumbing, connect the
		 * - keyboard output to the shell's STDIN
		 * - shell's STDOUT to the shell output pipe
		 * - shell's STDERR to the shell output pipe */
		dup2(srv_out[0], STDIN_FILENO);
		dup2(sh_out[1], STDOUT_FILENO);
		dup2(sh_out[1], STDERR_FILENO);

		// clean up the pipe file descriptors
		close(srv_out[0]);
		close(sh_out[1]);

		if (execl("/bin/bash", "/bin/bash", NULL) < 0)
			perror("shell: exec");

	} else if (pid > 0) { // parent (server) process
		// close the shell process's pipe ends:
		close(sh_out[1]);   // the write end of the shell output pipe
		close(srv_out[0]);  // the read end of the server output pipe
		if (sh_rd != NULL && sh_wr != NULL) {
			// do these get pushed off the stack after the function exits?
			*sh_rd = sh_out[0];
			*sh_wr = srv_out[1];
		}

	} else {
		perror("fork");
		exit(FATAL);
	}
	
	return pid;
}

void*
sh_listen(void *arg)
{
	int pfd;
	ssize_t r;
	char buf[BUFSIZE];

	// get the shell pipe file descriptor from arg
	pfd = *(int *)arg;

	while ((r = read(pfd, buf, BUFSIZE)) > 0) {
		write(STDOUT_FILENO, buf, r);
	}

	if (r == 0) {  // EOF from shell
		exit(2);
	} else {
		perror("server: failed to read from shell process");
		exit(FATAL);
	}
}

int
main(int argc, char *argv[])
{
	pid_t shpid;
	pthread_t tid;
	ssize_t s;
	int sh_rd, sh_wr;
	int sockfd, clientfd;
	struct sockaddr_in addr;
	char buf[BUFSIZE];

	parse_opts(argc, argv);

	sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sockfd < 0) {
		perror("srv: couldn't open socket");
		exit(FATAL);
	}

	memset(&addr, 0, sizeof addr);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

	// TODO: error checks
	bind(sockfd, (struct sockaddr *)&addr, sizeof addr);
	listen(sockfd, 1); // server only handles one connection; backlog=1
	clientfd = accept(sockfd, NULL, NULL);

	shpid = spawn_shell(&sh_rd, &sh_wr);
	// sigaction(SIGPIPE, sighandler); TODO: FIX THIS
	
	// redirect the server's standard streams to the socket
	dup2(clientfd, STDIN_FILENO);
	dup2(clientfd, STDOUT_FILENO);
	dup2(clientfd, STDERR_FILENO);

	// spawn thread to handle shell proc reading
	if ((errno = pthread_create(&tid, NULL, sh_listen, &sh_rd)) != 0) {
		perror("srv: sh_listener thread creation");
	}

	// read input from client and write to shell
	while ((s = read(STDIN_FILENO, buf, BUFSIZE)) > 0) {
		write (sh_wr, buf, s);
	}
	if (s == 0) {  // recieved EOF from client
		close(clientfd);
		close(sockfd);
		exit(1);
	} else {
		perror("srv: reading from client");
	}
}
