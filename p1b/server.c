#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/stat.h>
#include <fcntl.h>

#include <getopt.h>
#include <pthread.h>
#include <errno.h>
#include <pthread.h>
#include <mcrypt.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFSIZE		128
#define FATAL		100
#define RANDSEED	42

// TODO:
// - catch SIGPIPE with sigaction
// - 
static int eflg = 0;
static uint16_t port = 0;
static MCRYPT td;
static char keyfile[] = "my.key";
static char *IV = NULL;

void
encrypt_init()
{
	int keyfd, i;
	int keysize = 16;
	char key[keysize];

	eflg = 1;

	// open keyfile
	keyfd = open(keyfile, O_RDONLY);
	if (keyfd < 0) {
		perror("couldn't open key file");
		exit(FATAL);
	}

	// read the key from the keyfile
	i = read(keyfd, key, keysize);
	if (i < 0) {
		perror("couldn't read from key file");
		exit(FATAL);
	}

	td = mcrypt_module_open("twofish", NULL, "cfb", NULL);
	if (td == MCRYPT_FAILED) {
		exit(FATAL);
	}
	IV = malloc(mcrypt_enc_get_iv_size(td));

	srand(RANDSEED);
	for (i = 0; i < mcrypt_enc_get_iv_size(td); i++) {
		IV[i] = rand();
	}

	i = mcrypt_generic_init(td, key, keysize, IV);
	if (i < 0) {
		mcrypt_perror(i);
		exit(FATAL);
	}
}

void
encrypt(char *c)
{
	mcrypt_generic(td, c, 1);
}

void
decrypt(char *c)
{
	mdecrypt_generic(td, c, 1);
}

void
exit_routine()
{
	if (eflg) {
		mcrypt_generic_end(td);
	}

	if (IV != NULL) {
		free(IV);
		IV = NULL;
	}
}

void
parse_opts(int argc, char *argv[])
{
	int opt;
	int optindex;
	struct option longopts[] = {
		{"port", required_argument, NULL, 'p'},
		{"encrypt", no_argument, NULL, 'e'},
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

			case 'e':
				encrypt_init();
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

void
write_to_client(char* buf, ssize_t s)
{
	if (eflg) {
		for (int i = 0; i < s; i++) {
			encrypt(buf+i);
		}
	}
	write(STDOUT_FILENO, buf, s);
}

void*
sh_listen(void *arg)
{
	int pfd;
	ssize_t r;
	char buf[BUFSIZE];

	// get the shell pipe file descriptor from arg
	pfd = *(int *)arg;

	// read from the shell and write to client socket
	while ((r = read(pfd, buf, BUFSIZE)) > 0) {
		write_to_client(buf, r);
	}

	if (r == 0) {  // EOF from shell
		exit(2);
	} else {
		perror("server: failed to read from shell process");
		exit(FATAL);
	}
}

/* returns a file descriptor that's connected to the client */
int
get_connection()
{
	int sockfd, clientfd, r;
	struct sockaddr_in addr;

	eflg = 1;

	sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sockfd < 0) {
		perror("srv: couldn't open socket");
		exit(FATAL);
	}

	memset(&addr, 0, sizeof addr);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	r = inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
	if (r != 1) {
		perror("address couldn't be converted");
		exit(FATAL);
	}

	r = bind(sockfd, (struct sockaddr *)&addr, sizeof addr);
	if (r < 0) {
		perror("couldn't bind to port");
		exit(FATAL);
	}

	r = listen(sockfd, 1); // server only handles one connection; backlog=1
	if (r < 0) {
		perror("listen");
		exit(FATAL);
	}

	clientfd = accept(sockfd, NULL, NULL);
	if (clientfd < 0) {
		perror("accept");
		exit(FATAL);
	}

	close(sockfd); // won't be needing this fd anymore
	return clientfd;
}

int
main(int argc, char *argv[])
{
	pid_t shpid;
	pthread_t tid;
	ssize_t s;
	int sh_rd, sh_wr, clientfd;
	char c;

	atexit(exit_routine);
	parse_opts(argc, argv);

	clientfd = get_connection();

	shpid = spawn_shell(&sh_rd, &sh_wr);
	// sigaction(SIGPIPE, sighandler); TODO: FIX THIS
	
	// redirect the server's standard streams to the socket
	dup2(clientfd, STDIN_FILENO);
	dup2(clientfd, STDOUT_FILENO);
	dup2(clientfd, STDERR_FILENO);
	close(clientfd); // don't need this fd anymore either

	// spawn thread to handle shell proc reading
	if ((errno = pthread_create(&tid, NULL, sh_listen, &sh_rd)) != 0) {
		perror("srv: sh_listener thread creation");
	}

	// read input from client and write to shell
	while ((s = read(STDIN_FILENO, &c, 1)) > 0) {
		if (eflg) {
			decrypt(&c);
		}
		write (sh_wr, &c, 1);
	}
	if (s == 0) {  // recieved EOF from client
		exit(1);
	} else {
		perror("srv: reading from client");
	}
}
