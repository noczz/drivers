#include <stdio.h>

// open()
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <unistd.h>

int main(int argc, char **argv)
{
	char buf[5]="xxxxx";

	int fd = open("/dev/scull0", O_RDWR);
	if(fd == -1)
		printf("open error\n");

	if(argv[1]) {
		switch(argv[1][1]){
			case 'r':
				read(fd, buf, 5);
				printf("%s\n", buf);

				break;
			case 'w':
				if(!argv[2]) return 0;
				write(fd, argv[2], 5);

				break;
			default:
				printf("error args\n");
				break;
		}
	}

	close(fd);
	
	return 0;
}
