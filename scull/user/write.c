#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <stdlib.h>

// open()
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define BUFSIZE 1024

int main(int argc, char **argv)
{
	char ch = '1';
	int fd, i, count = 0;

	fd = open(argv[3], O_RDWR);
	if(fd == -1)
		printf("open error\n");

	ch = argv[1][0];
	count = atoi(argv[2]);
	printf("count %d\n", count);
	for(i = 0; i < count; i++) {
		write(fd, &ch, 1);
		printf("%c", ch);
	}

	return 0;
}
