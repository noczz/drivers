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

const char devfile[] = "/dev/scull_pipe0";

int main(int argc, char *argv[])
{
	int fd, err, count = 0;
	char buf[BUFSIZE];

	fd = open (devfile, O_RDWR);
	if (fd == -1) {
		DEBUG("open error\n");
		return -1;
	}

	if (argc != 2)
		printf("pipe-read count\n");
	else {
		count = atoi(argv[1]);
		if (count == 0)
			printf("%s: atoi error\n", argv[0]);
		count = read(fd, buf, count);
		if (count < 0) {
			DEBUG("read error\n");
			return -1;
		}
	}

	buf[count] = '\0';

	printf("%s: read %d bytes from %s\n", argv[0], count, devfile);
	printf("%s\n", buf);

	close(fd);
	return 0;
}
