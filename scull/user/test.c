#include <stdio.h>
#include <stdlib.h> // atoi()

// open()
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <unistd.h>

int main(int argc, char **argv)
{
	char buf[65536]={'^'};

	int fd = open("/dev/scull0", O_RDWR);
	if(fd == -1)
		printf("open error\n");

	if(argv[1]) {
		switch(argv[1][1]){
			case 'r':
				read(fd, buf, atoi(argv[2]));
				printf("%s\n", buf);

				break;
			case 'w':
				if(!argv[2]) return 0;
				write(fd, argv[2], atoi(argv[3]));

				break;
			case 'h':
				printf(	"NAME\n"
					"\t test /dev/scull\n"

					"SYNOPSIS\n"
					"\t test -r COUNTS\n"
					"\t test -w STRING COUNTS\n"

					"DESCRIPTION\n"
					"\t-r COUNTS \n"
					"\t\t COUNTS bytes will be read.\n"
					"\t-w STRING COUNTS\n"
					"\t\tCOUNTS bytes of the STRING will be write into scull"
				);
			default:
				printf("error args\n");
				break;
		}
	}

	close(fd);
	
	return 0;
}
