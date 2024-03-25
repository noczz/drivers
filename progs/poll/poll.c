#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <poll.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define PROG_DEBUG
#include "debug.h"

#define ms 1000

int numPipes = 4;
int pfds[4];

int child(void)
{
	char bufp[5] = "hello";
	int ret, i, random;

	srand(time(0));
	random = rand() % numPipes;
	//DEBUG("random %d\n", random);

	usleep(random*500*ms);

	if (write(pfds[random], bufp, 5) == -1) {
		DEBUG("error\n");
		return -1;
	}
	DEBUG("write to pdfs[%d]\n", random);
	return 0;
}

int parent(void)
{
	struct pollfd *pollFds;
	char bufc[5];
	int i, ready, ret;

	pollFds = calloc(numPipes, sizeof(struct pollfd));

	// OUT

	for (i = 0; i < numPipes; i++) {
		pollFds[i].fd = pfds[i];
		pollFds[i].events = POLLOUT;
	}

	ready = poll(pollFds, numPipes, -1);
	if (ready == -1)
		return -1;

	DEBUG("ready %d\n", ready);

	for (i = 0; i < numPipes; i++)
		DEBUG("pllFds[%d].revents %04X\n", i, pollFds[i].revents);

	sleep(1);

	// IN

	for (i = 0; i < numPipes; i++) {
		pollFds[i].fd = pfds[i];
		pollFds[i].events = POLLIN;
	}

	ready = poll(pollFds, numPipes, -1);
	if (ready == -1)
		return -1;

	DEBUG("ready %d\n", ready);

	for (i = 0; i < numPipes; i++) {
		if (pollFds[i].revents && POLLIN) {
			DEBUG("pollFds[%d].revents %04X\n", i, pollFds[i].revents);
			ret = read(pollFds[i].fd, bufc, 5);
			DEBUG("%s\n", bufc);
		}
	}

	return 0;
}

int main(int argc, char *argv[])
{
	int i;

	pfds[0] = open("/dev/scull_pipe0", O_RDWR);
	pfds[1] = open("/dev/scull_pipe1", O_RDWR);
	pfds[2] = open("/dev/scull_pipe2", O_RDWR);
	pfds[3] = open("/dev/scull_pipe3", O_RDWR);

	switch(fork()) {
		case -1:
			DEBUG("error\n");
			break;
		case 0:
			return child();
			break;
		default:
			return parent();
			break;
	}
	return 0;
}
