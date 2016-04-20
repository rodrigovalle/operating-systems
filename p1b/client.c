#include <getopt.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <mcrypt.h>

#define FATAL		100
#define BUFSIZE		128 
#define RANDSEED	42   // this should match with server.c

static const char keyfile[] = "my.key";

static int eflg = 0;
static int logfd = -1;
static int sockfd = -1;
static uint16_t port = 0;
static struct termios termdef;
static MCRYPT td;
static char *IV = NULL;
static pthread_mutex_t wr_lock = PTHREAD_MUTEX_INITIALIZER;

void exit_routine(int ret_code);
void encrypt_init();

/* If the --encrypt flag is found, set the global eflg.
 * If the --log option is found, return the logfile's name
 * Otherwise, return NULL
 */
void
parse_opts(int argc, char *argv[])
{
	int opt;
	int optindex;
	struct option longopts[] = {
		{"port", required_argument, NULL, 'p'},
		{"encrypt", no_argument, NULL, 'e'},
		{"log", required_argument, NULL, 'l'},
		{0,0,0,0}
	};

	while ((opt = getopt_long(argc, argv, "", longopts, &optindex)) != -1) {
		switch (opt) {
			case 'p':; // need this semicolon here to appease the gods of C programming
				// do some input sanitation
				long int p = strtol(optarg, NULL, 10);
				if (p > UINT16_MAX || p < 0) {
					fprintf(stderr, "invalid port specified\n");
					exit_routine(FATAL);
				}
				port = (uint16_t)p;
				break;

			case 'l':
				// optarg is a pointer to an argv element;
				// don't need to deal with malloc
				logfd = open(optarg, O_WRONLY|O_CREAT|O_TRUNC,
						     S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
				break;

			case 'e':
				encrypt_init();
				break;

			case '?':
				exit_routine(FATAL);
		}
	}
}

void
encrypt_init()
{
	int keyfd, i;
	int keysize = 16; // key is 16 bytes (128 bits)
	char key[keysize];

	eflg = 1;

	// open file containing key
	keyfd = open(keyfile, O_RDONLY);
	if (keyfd < 0) {
		perror("couldn't open key file");
		exit_routine(FATAL);
	}

	// read the first 16 bytes from the keyfile
	// note: these should be the only 16 bytes in the file
	i = read(keyfd, key, keysize);
	if (i < 0) {
		perror("couldn't read from key file");
		exit_routine(FATAL);
	}

	td = mcrypt_module_open("twofish", NULL, "cfb", NULL);
	if (td == MCRYPT_FAILED) {
		exit_routine(FATAL);
	}
	IV = malloc(mcrypt_enc_get_iv_size(td));

	srand(RANDSEED);
	for (i = 0; i < mcrypt_enc_get_iv_size(td); i++) {
		IV[i] = rand();
	}

	i = mcrypt_generic_init(td, key, keysize, IV);
	if (i < 0) {
		mcrypt_perror(i);
		exit_routine(FATAL);
	}
}

/* this should only be called by the send_to_srv routine */
void
encrypt(char *c)
{
	mcrypt_generic(td, c, 1);
}

/* this should only be called by the socket_listen routine */
void
decrypt(char *c)
{
	mdecrypt_generic(td, c, 1);
}

void
encrypt_deinit()
{
	mcrypt_generic_end(td);
	if (IV != NULL) {
		free(IV);
		IV = NULL;
	}
}

/* Stores the default terminal settings and sets noncanonical no-echo mode. */
void
setup_term()
{
	struct termios termopts;
	if (tcgetattr(STDERR_FILENO, &termdef) < 0)
		perror("tcgetattr");

	termopts = termdef;
	termopts.c_lflag &= ~(ICANON | ECHO);
	termopts.c_cc[VTIME] = 0;
	termopts.c_cc[VMIN] = 1;

	if (tcsetattr(STDERR_FILENO, TCSAFLUSH, &termopts) < 0)
		perror("tcsetattr");
}

/* Restores the default terminal settings */
// TODO: make sure that no two threads can call exit at the same time
void
restore_term()
{
	// restore default terminal settings
	if (tcsetattr(STDERR_FILENO, TCSAFLUSH, &termdef) < 0)
		fprintf(stderr, "Unable to restore default terminal settings.\n");
}

/* Creates a tcp socket, but exits on error. */
int
mk_socket()
{
	int sfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // see ip(7), tcp(7)
	if (sfd < 0) {
		perror("couldn't open socket");
		exit_routine(FATAL);
	}
	return sfd;
}

/* Connects to the server process, but exits on error. */
void
connect_to_server(int sfd)
{
	struct sockaddr_in addr;

	memset(&addr, 0, sizeof addr);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	inet_pton(AF_INET, "127.0.0.1", &(addr.sin_addr));

	if (connect(sfd, (struct sockaddr *)&addr, sizeof addr) == -1) {
		perror("couldn't connect to server");
		close(sfd);
		exit_routine(FATAL);
	}
}

/* The background thread routine recieving from the server process via
 * the socket and printing the server's messages to stdout. */
void
*socket_listen(void *arg)
{
	int sfd;
	ssize_t r;
	char buf[BUFSIZE];
	char str[BUFSIZE];

	sfd = *(int *)arg;

	while ((r = recv(sfd, buf, BUFSIZE, 0)) > 0) {
		if (logfd != -1) {
			pthread_mutex_lock(&wr_lock);
			sprintf(str, "RECEIVED %d bytes: ", (int)r);
			write(logfd, str, strlen(str)); // print message
			write(logfd, buf, r); // print bytes
			write(logfd, "\n", 1); // print newline
			pthread_mutex_unlock(&wr_lock);
		}
		
		if (eflg) {
			for (int i = 0; i < r; i++) {
				decrypt(buf+i);
			}
		}

		write(STDOUT_FILENO, buf, r);
	}
	// read error or EOF from connection
	exit_routine(1);
	return NULL;
}

/* handles sending, logging sends, and encryption */
void
send_to_srv(int sockfd, char c)
{
	if (eflg) {
		encrypt(&c);
	}

	send(sockfd, &c, 1, 0);

	if (logfd != -1) {
		pthread_mutex_lock(&wr_lock);
		write(logfd, "SENT 1 bytes: ", 14);
		write(logfd, &c, 1);
		write(logfd, "\n", 1);
		pthread_mutex_unlock(&wr_lock);
	}
}

// one exit routine to rule them all
/* close the network connection/log file and restore the terminal */
// TODO: make sure the two threads can't call this routine at the same time
void
exit_routine(int ret_code)
{
	if (sockfd != -1)
		close(sockfd);
	if (logfd != -1)
		close(logfd);
	if (eflg == 1)
		encrypt_deinit();

	exit(ret_code);
}

/* Read input from the user and send it to the server.
 * if --encrypt is specified, input is encrypted before it's sent
 * if --log is specified, input is recorded to a log file before it's sent */
int
main(int argc, char *argv[])
{
	ssize_t s;
	pthread_t tid;

	parse_opts(argc, argv);

	sockfd = mk_socket();
	connect_to_server(sockfd);

	// spawn a thread to handle reading from the socket
	if ((errno = pthread_create(&tid, NULL, socket_listen, &sockfd)) != 0) {
		perror("client: socket_listener thread creation");
	}

	setup_term();
	atexit(restore_term);

	// read one character at a time from the user
	char c;
	while ((s = read(STDIN_FILENO, &c, 1)) > 0) {
		switch (c) {
			case '\r':
			case '\n':
				write(STDOUT_FILENO, "\r\n", 2);
				send_to_srv(sockfd, '\n');
				break;

			case '\004': // ^D from terminal
				exit_routine(0);
				break;

			default:
				write(STDOUT_FILENO, &c, 1);  // echo to term
				send_to_srv(sockfd, c);
				break;
		}
	}
	// EOF or read error
	exit_routine(1);
}
