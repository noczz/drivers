#include <stdio.h>
#include <stdlib.h>	// EXIT_FAILURE
#include <string.h>

// open()
#include <sys/stat.h>
#include <fcntl.h>

// fork()
#include <unistd.h>	// sleep(), close()
#include <sys/types.h>

#define BUFSIZE 10

static int idata = 111;

void son(void) {
	int fd, i;
	char *buf;

	buf = malloc(BUFSIZE);
	memset(buf, 'O', BUFSIZE);
	buf[BUFSIZE-1] = '\0';

	fd = open("/dev/scull0", O_RDWR);
	if(fd == -1)
		perror("(son) error open\n");
	printf("(son) opened\n");


	printf("(son) start writting\n");
	for(i = 0; i < 100; i++) {
		usleep(200000);
		write(fd, buf, BUFSIZE);
	}
	printf("(son) written done\n");

	close(fd);
	printf("(son) closed\n");

	free(buf);
	buf = NULL;

	return;
}

void parent(void) {
	int fd, i;
	char *buf = NULL;

	buf = malloc(BUFSIZE);
	memset(buf, 'X', BUFSIZE);
	buf[BUFSIZE-1] = '\0';

	usleep(100);

	fd = open("/dev/scull0", O_RDWR);
	if(fd == -1)
		perror("(son) error open\n");
	printf("(parent) opened\n");

	printf("(parent) start writting\n");
	for(i = 0; i < 100; i++) {
		usleep(100000);
		write(fd, buf, BUFSIZE);
	}
	printf("(parent) written done\n");

	close(fd);
	printf("(parent) closed\n");

	free(buf);
	buf = NULL;

	return;
}

int
main(int argc, char *argv[])
{
	int istack = 222;
	pid_t childPid;

	switch (childPid = fork()) {
		case -1:
			perror("error fork");
			return EXIT_FAILURE;

		case 0: // son
			son();
			break;

		default: // parent
			parent();
			break;
	}

//	printf("PID=%ld %s idata=%d istack=%d\n", (long) getpid(),
//			(childPid == 0) ? "(child) ": "(parent)", idata, istack);

	return EXIT_SUCCESS;
}
