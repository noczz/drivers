#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define BUFSIZE 100
#define PROG_DEBUG
#define PROG_NAME "pipe_read"
#include "debug.h"

int main(int argc, char *argv[])
{
	int fd, err;
	char buf[BUFSIZE];
	char devfile[] = "/dev/scull_pipe0";

	fd = open (devfile, O_RDWR);
	if (fd == -1) {
		DEBUG("open error\n");
		return -1;
	}

	if (!read(fd, buf, 3)) {
		DEBUG("read error\n");
		return -1;
	}

	sleep(5);

	if (!read(fd, buf+3, 3)) {
		DEBUG("read error\n");
		return -1;
	}
	buf[6] = '\0';
	DEBUG("buf %s\n", buf);

	close(fd);
	return 0;
}
