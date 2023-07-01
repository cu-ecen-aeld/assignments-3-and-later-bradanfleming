#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	openlog(NULL, 0, LOG_USER);
	if (argc < 3) {
		fprintf(stderr, "Not enough arguments!\nUsage: %s <path> <string>\n", argv[0]);
		syslog(LOG_ERR, "Not enough arguments!\nUsage: %s <path> <string>\n", argv[0]);
		closelog();
		return 1;
	}
	char *path = argv[1];
	char *string = argv[2];
	// Creating the directory is not required this time
	int fd = open(path, O_CREAT | O_WRONLY | O_TRUNC, 0644);
	if (fd < 0) {
		fprintf(stderr, "Failed to open %s: %s\n", path, strerror(errno));
		syslog(LOG_ERR, "Failed to open %s: %s\n", path, strerror(errno));
		closelog();
		return 1;
	}
	syslog(LOG_DEBUG, "Writing %s to %s\n", string, path);
	if (write(fd, string, strlen(string)) < 0) {
		fprintf(stderr, "Failed to write to %s: %s\n", path, strerror(errno));
		syslog(LOG_ERR, "Failed to write to %s: %s\n", path, strerror(errno));
		close(fd);
		closelog();
		return 1;
	}
	close(fd);
	closelog();
	return 0;
}
