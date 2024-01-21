#include <stdio.h>
#include <stdlib.h> // atoi()
#include <string.h> // memset(), strlen()

// open()
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <unistd.h>

int main(int argc, char **argv)
{
	char buf[1024];
	int fd;

	// buf initialize
	memset(buf, '_', 1023);
	buf[1023] = '\0';

	fd = open("/dev/scull0", O_RDWR);
	if(fd == -1)
		printf("open error\n");

	if(!argv[1]) {
		printf("Nothing happened\n");
		goto out;
	}

	switch(argv[1][1]){
		case 'r':
			read(fd, buf, atoi(argv[2]));
			printf("%s\n", buf);

			break;
		case 'w':
			if(!argv[2]) return 0;
			write(fd, argv[2], strlen(argv[2]));

			break;
		case 'h':
			printf(	"NAME\n"
				"\t test /dev/scull\n"

				"SYNOPSIS\n"
				"\t test -r COUNT\n"
				"\t test -w STRING COUNT\n"

				"DESCRIPTION\n"
				"\t-r COUNT \n"
				"\t\tCOUNT bytes will be read.\n"
				"\t-w STRING COUNT\n"
				"\t\tThe STRING will be write into scull"
			);
		default:
			printf("error args\n");
			break;
	}

out:
	close(fd);
	return 0;
}
