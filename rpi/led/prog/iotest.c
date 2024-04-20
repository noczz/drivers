#include <stdio.h>
#include <sys/ioctl.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <unistd.h>

#include <stdlib.h>

#define IOC_MAGIC 'k'
#define LED_IOCRESET	_IO(IOC_MAGIC, 0)
#define GPIO_ON		_IO(IOC_MAGIC, 1)
#define GPIO_OFF	_IO(IOC_MAGIC, 2)

int main(int argc, char *argv[])
{
	char cmd = argv[1][1];


	int fd = open("/dev/led", O_RDWR);

	switch (cmd) {
		case 'v':
			ioctl(fd, GPIO_ON, (long) atoi(argv[2]));
			break;
		case 'x':
			ioctl(fd, GPIO_OFF, (long) atoi(argv[2]));
			break;
		default:
			printf("./iotest [-vx] <PINS>\n");
	}

	close(fd);
	return 0;
}
