#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <pthread.h>

#include <unistd.h> // write(), close()

// open()
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define BUFSIZE 512000

int fd;

static void *
threadFunc(void *arg)
{
	char *buf = malloc(BUFSIZE);
	int i;

	memset(buf, 'X', BUFSIZE);
	buf[BUFSIZE-1] = '\0';

	// do something
	for (i = 0; i < 100; i++)
		write(fd, "O", 1);

	return 0;
}

int
main(int argc, char *argv[])
{
	pthread_t t1;
	void *res;
	int s, i;
	char *buf = malloc(BUFSIZE);

	memset(buf, 'X', BUFSIZE);
	buf[BUFSIZE-1] = '\0';

	fd = open("/dev/scull0", O_RDWR);
	if(fd == -1) {
		perror("[ error ] open");
	}

	s = pthread_create(&t1, NULL, threadFunc, NULL);
	if (s != 0)
		perror("[ error ] pthread_create");

	// do something
	for (i = 0; i < 100; i++)
		write(fd, "X", 1);

	s = pthread_join(t1, &res);
	if (s != 0)
		perror("[ error ] pthread_join");

	printf("\n");

	close(fd);

	return EXIT_SUCCESS;
}
