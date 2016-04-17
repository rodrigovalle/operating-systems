#include <getopt.h>
#include <termios.h>
#include <unistd.h>

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

#define FATAL 100
#define BUFSIZE 128 
static const char keyfile[] = "my.key";

static int eflg = 0;
static int logfd = -1;
static uint16_t port = 0;
static struct termios termdef;


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
		{"encrypt", no_argument, &eflg, 1},
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
					exit(FATAL);
				}
				port = (uint16_t)p;
				break;

			case 'l':
				// optarg is a pointer to an argv element;
				// don't need to deal with malloc
				logfd = open(optarg, O_WRONLY|O_CREAT|O_TRUNC,
						     S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
				break;

			case '?':
				exit(FATAL);
		}
	}
}

/* Stores the default terminal settings and sets noncanonical no-echo mode. */
void
setup_terminal()
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

/* Closes the network connection and restores the default terminal settings */
// TODO: implement closing the network connection
// TODO: make sure that no two threads can call exit at the same time
void
cleanup()
{
	// close network connection
	// restore default terminal settings
	if (tcsetattr(STDERR_FILENO, TCSAFLUSH, &termdef) < 0)
		fprintf(stderr, "Unable to restore default terminal settings.\n");
}

/* Read input from the user and send it to the server.
 * if --encrypt is specified, input is encrypted before it's sent
 * if --log is specified, input is recorded to a log file before it's sent */
int
main(int argc, char *argv[])
{
	ssize_t s;
	int sockfd;
	struct sockaddr_in addr;
	char buf[BUFSIZE];

	parse_opts(argc, argv);
	fprintf(stderr, "eflg: %d\n", eflg);
	fprintf(stderr, "port: %d\n", port);
	fprintf(stderr, "logfd: %d\n", logfd);

	// make a TCP socket
	sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // see ip(7), tcp(7)
	if (sockfd < 0) {
		perror("couldn't open socket");
		exit(FATAL);
	}

	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	inet_pton(AF_INET, "127.0.0.1", &(addr.sin_addr));

	if (connect(sockfd, (struct sockaddr *)&addr, sizeof addr) == -1) {
		perror("couldn't connect to server");
		close(sockfd);
		exit(FATAL);
	}

	setup_terminal();
	atexit(cleanup);

	// read input from the user
	while ((s = read(STDIN_FILENO, buf, BUFSIZE)) > 0) {
		for (ssize_t i = 0; i < s; i++) {
			switch (buf[i]) {
				case '\004': // ^D from terminal
					// - close network connection
					// - restore terminal
					exit(0);
					break;

				default:
					// TODO: encryption
					write(STDOUT_FILENO, buf+i, 1);
					send(sockfd, buf+i, 1, 0);
					if (logfd != -1) {
						// TODO: careful, races can occur with the background
						// thread that's also trying to write to the log
						write(logfd, "SENT: ", 6);
						write(logfd, buf+i, 1);
						write(logfd, "\n", 1);
					}
					break;
			}
		}
	}
	// EOF or read error
	// - close the network connection
	// - restore terminal
	exit(1);
}
