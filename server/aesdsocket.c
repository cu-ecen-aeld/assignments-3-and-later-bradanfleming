#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PATH "/var/tmp/aesdsocketdata"
#define PORT "9000"
#define BACKLOG 2
#define BUFSIZE 512
#define MTUSIZE 512

int sfd, cfd, ofd;
bool running = true;

void handleSignal(int s)
{
	syslog(LOG_INFO, "%s\n", "Caught signal, exiting");
	running = false;
	close(sfd); /* break fd to stop accept from blocking */
}

void *get_in_addr(struct sockaddr *sa)
{
	/* Copied from Beej's Guide to Network Programming */
	if (sa->sa_family == AF_INET)
		return &(((struct sockaddr_in*)sa)->sin_addr);
	else
		return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void fappend(char *out, size_t out_len) {
	if (write(ofd, out, out_len) < 0) {
		syslog(LOG_ERR, "write: %s\n", strerror(errno));
	}
}

void fsend() {
	lseek(ofd, 0, SEEK_SET);
	char chunk[MTUSIZE];
	size_t pos = 0;
	ssize_t bytes_out;
	while (bytes_out = read(ofd, chunk, MTUSIZE)) {
		if (bytes_out == -1)
			syslog(LOG_ERR, "read: %s\n", strerror(errno));
		if (bytes_out <= 0)
			break;
		if (send(cfd, chunk, bytes_out, 0) == -1) {
			syslog(LOG_ERR, "send: %s\n", strerror(errno));
			return;
		}
	}
}

void bufexpand(char **buf, size_t *buf_size, size_t buf_content) {
	*buf_size *= 2;
	char *new = malloc(*buf_size);
	if (new == NULL) {
		syslog(LOG_ERR, "malloc: %s\n",	strerror(errno));
		exit(EXIT_FAILURE);
	}
	memcpy(new, *buf, buf_content);
	free(*buf);
	*buf = new;
}

int main(int argc, char *argv[])
{
	bool daemonize = false;
	for (int i = 1; i < argc; i++)
		if (strcmp(argv[i], "-d") == 0)
			daemonize = true;

	openlog(NULL, LOG_PERROR, LOG_USER);

	struct addrinfo hints, *res;
	hints.ai_family = AF_UNSPEC; /* allow IPv4 and IPv6 */
	hints.ai_flags = AI_PASSIVE; /* bind to host IP */
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_socktype = SOCK_STREAM;
	int r = getaddrinfo(NULL, PORT, &hints, &res);
	if (r != 0) {
		syslog(LOG_ERR, "getaddrinfo: %s\n", gai_strerror(r));
		exit(-1);
	}

	/* try the first result returned */
	sfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (sfd == -1) {
		syslog(LOG_ERR, "socket: %s\n", strerror(errno));
		exit(-1);
	}
	if (bind(sfd, res->ai_addr, res->ai_addrlen) == -1) {
		syslog(LOG_ERR, "bind: %s\n", strerror(errno));
		exit(-1);
	}
	freeaddrinfo(res);

	if (listen(sfd, BACKLOG) == -1) {
		syslog(LOG_ERR, "listen: %s\n", strerror(errno));
		exit(-1);
	}

	ofd = open(PATH, O_CREAT | O_RDWR | O_TRUNC | O_APPEND, 0644);
	if (ofd < 0) {
		syslog(LOG_ERR, "open %s: %s\n", PATH, strerror(errno));
		exit(EXIT_FAILURE);
	}

	signal(SIGINT, handleSignal);
	signal(SIGTERM, handleSignal);

	if (daemonize) {
		pid_t pid = fork();
		if (pid == -1) {
			syslog(LOG_ERR, "fork: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}
		if (pid != 0) {
			fprintf(stdout, "Kicked off daemon: %d\n", pid);
			exit(EXIT_SUCCESS);
		}
		if (setsid() == -1) {
			syslog(LOG_ERR, "setsid: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}
		if (chdir("/") == -1) {
			syslog(LOG_ERR, "chdir: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}
		close(fileno(stdin));
		close(fileno(stdout));
		close(fileno(stderr));
	}

	struct sockaddr_storage addr;
	socklen_t addrlen;
	char addr_str[INET6_ADDRSTRLEN];

	char *buf = malloc(BUFSIZE);
	if (buf == NULL) {
		syslog(LOG_ERR, "malloc: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	char *l; /* newline pointer */
	size_t buf_content; /* content size */
	size_t buf_size; /* buffer size */
	size_t bytes_in; /* bytes read */

	syslog(LOG_INFO, "%s\n", "Listening for connections");
	while (running) {
		addrlen = sizeof addr;
		cfd = accept(sfd, (struct sockaddr *)&addr, &addrlen);
		if (cfd == -1) {
			syslog(LOG_ERR, "accept: %s\n", strerror(errno));
			continue;
		}
		inet_ntop(addr.ss_family,
				get_in_addr((struct sockaddr *)&addr),
				addr_str, sizeof addr_str);
		syslog(LOG_INFO, "Accepted connection from %s\n", addr_str);

		l = NULL;
		buf_content = 0;
		buf_size = BUFSIZE;
		bytes_in = -1;
		while (bytes_in != 0) {
			if (buf_content == buf_size)
				bufexpand(&buf, &buf_size, buf_content);
			bytes_in = recv(cfd, buf+buf_content,
					buf_size - buf_content, 0);
			buf_content += bytes_in;
			l = memchr(buf, '\n', buf_content);
			if (l != NULL) {
				/* handle newline */
				l += 1;
				fappend(buf, l - buf);
				buf_content -= l - buf;
				memcpy(buf, l, buf_content);
				fsend();
			}
		}

		close(cfd);
		syslog(LOG_INFO, "Closed connection from %s\n", addr_str);
	}

	free(buf);
	close(sfd);
	close(ofd);
	if (remove(PATH) != 0)
		syslog(LOG_ERR, "remove %s: %s\n", PATH, strerror(errno));
	closelog();
	exit(EXIT_SUCCESS);
}
