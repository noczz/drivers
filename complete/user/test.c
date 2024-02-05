#include <stdio.h>

#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define BUFSIZE 10

int main(int argc, char **argv)
{
	char buf[BUFSIZE];

	int fd = open("/dev/complete", O_RDWR);

	read(fd, buf, BUFSIZE);

	printf("%s\n", buf);

	close(fd);

	return 0;
}
