#include <stdio.h>
#include <stdlib.h> // atoi()
#include <string.h> // memset(), strlen()

#define QUANTUM 10
#define BUFSIZE 1024*1024

// open()
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <unistd.h> // read(), write()

int main(int argc, char **argv)
{
	char buf[BUFSIZE];
	int fd, count, s_pos, f_pos, rest;

	// buf initialize
	memset(buf, 0, 1024*1024);

	fd = open("/dev/scull0", O_RDWR);
	if(fd == -1)
		printf("open error\n");

	if(!argv[1]) {
		printf("Nothing happened\n");
		goto out;
	}

	switch(argv[1][1]){
		case 'r':
			count = atoi(argv[2]);
			if(count <= 0) break;

			s_pos=0;
			f_pos=0;
			while(1) {
				rest = count - f_pos;
				if(rest > QUANTUM) {
					read(fd, buf + s_pos*QUANTUM, QUANTUM);
					s_pos++;
					f_pos += QUANTUM;
				} else {
					read(fd, buf + s_pos*QUANTUM, rest);
					break;
				}
			}

			memset(buf+count, 0, BUFSIZE-count);
			printf("%s\n", buf);

			break;
		case 'w':
			count = strlen(argv[2]);
			if(count <= 0) break;

			s_pos=0;
			f_pos=0;
			while(1) {
				rest = count - f_pos;
				if(rest > QUANTUM) {
					write(fd, argv[2] + s_pos*QUANTUM, QUANTUM);
					s_pos++;
					f_pos += QUANTUM;
				} else {
					write(fd, argv[2] + s_pos*QUANTUM, rest);
					break;
				}
			}
			break;
		default:
			printf("error args\n");
			break;
	}

out:
	close(fd);
	return 0;
}
