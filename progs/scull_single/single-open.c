#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

static char dev[] = "/dev/scull_single";

void son(int fd)
{
	printf("son: open\n");
	fd = open(dev, O_RDWR);
	if (fd < 0) {
		printf("son: error open\n");
		return;
	}
	printf("son: opened (%d)\n", fd);
	printf("son: sleep ...\n");

	close(fd);
	return;
}

void parent(int fd)
{
	int fd_;

	printf("parent: open\n");
	fd = open(dev, O_RDWR);
	if (fd < 0) {
		printf("parenr: error open 0\n");
		return;
	}
	printf("parent: opened (%d)\n", fd);

	printf("parent: open again\n");
	fd_ = open(dev, O_RDWR);
	if (fd_ < 0) {
		printf("parent: error open 1\n");
	}
	else
		printf("parent: opened (%d)\n", fd_);

	printf("parent: sleep ...\n");
	sleep(2);

	close(fd);
	if(fd_ > 0)
		close(fd_);
	return;
}

int main(int argc, char *argv[])
{
	int fd, err;
	pid_t childPid;


	switch (childPid = fork()) {
		case -1:
			printf("error fork");
			return EXIT_FAILURE;
		case 0:
			sleep(1);
			son(fd);
			break;
		default:
			parent(fd);
			break;
	}

	return EXIT_SUCCESS;
}
