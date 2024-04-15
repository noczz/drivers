#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <unistd.h>


int main(int argc, char **argv)
{
	int err;

	int fd = open(argv[1], O_RDWR);
	if (fd == -1) {
		printf("open error\n");
		return 1;
	}

	err = write(fd, argv[2], sizeof(argv[2]));
	if (fd == -1) {
		printf("write error\n");
		return 1;
	}

	close(fd);
	return 0;
}
