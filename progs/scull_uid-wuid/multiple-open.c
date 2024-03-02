#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <pthread.h>

#include <unistd.h> // write(), close()

// open()
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MULOP_DEBUG

#ifdef MULOP_DEBUG
#	define DEBUG(fmt, args...) \
		printf("[ multiple-open ] %s:%d %s():" fmt, \
					__FILE__, __LINE__, __func__, ##args)
#else
#	define DEBUG
#endif

#define BUFSIZE 10

char device[] = "/dev/scull_uid";

static void *threadFunc(void *arg)
{
	char *buf;
	int retval = 0;
	int fd;

	// init buf
	buf = malloc(BUFSIZE);
	memset(buf, 'X', BUFSIZE);

	// open device
	fd = open(device, O_RDWR);
	if(fd == -1) {
		DEBUG("error\n");
		return (void *) -1;
	}
	DEBUG("%s opened, fd is %d\n", device, fd);

	// write to device
	write(fd, buf, BUFSIZE);
	write(fd, buf, BUFSIZE);
	DEBUG("writed X to %s\n", device);

	close(fd);
	DEBUG("%s closed\n", device);
	return (void *) 0;
}

int main(int argc, char *argv[])
{
	int fd, err;
	void *res;
	char *buf;
	pthread_t t1;

	// init buf
	buf = malloc(BUFSIZE);
	memset(buf, 'O', BUFSIZE);

	// open device
	fd = open(device, O_RDWR);
	if(fd == -1) {
		DEBUG("open error\n");
		return -1;
	}
	DEBUG("%s opened, fd is %d\n", device, fd);

	// create pthread
	err = pthread_create(&t1, NULL, threadFunc, &fd);
	if (err != 0) {
		DEBUG("open error\n");
		return -1;
	}
	sleep(1);

	// write to device
	write(fd, buf, BUFSIZE);
	DEBUG("writed O to %s\n", device);

	err = pthread_join(t1, &res);
	if (err != 0) {
		DEBUG("pthread_join error\n");
		return -1;
	}

	DEBUG("threadFunc returned %ld\n", (long)res);

	close(fd);

	return 0;
}
