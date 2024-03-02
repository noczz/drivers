#include <stdio.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define PROG_NAME "keep-open"
#define PROG_DEBUG
#include "debug.h"

char device[] = "/dev/scull_wuid";

int main(int argc, char *argv[])
{
	int fd;

	/**
	 * NOTICE!
	 *
	 * 	if the others who is not root
	 * 	trying to open the scull
	 * 	devices. Please change the O_RDWR
	 * 	to O_RDONLY, becase they are set
	 * 	up to only read for others.
	 */
	fd = open(device, O_RDWR);
	if (fd == -1) {
		DEBUG("open error\n");
		return -1;
	}

	DEBUG("sleep ...\n");
//	fflush(stdout);
	sleep(10);
	DEBUG("wake up!\n");

	close(fd);
	return 0;
}
