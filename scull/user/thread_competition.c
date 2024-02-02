#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <pthread.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

int fd;

static void *
thread_func(void * arg)
{
	char ch = 'X';
	int i;
	int count = *( (int *)arg );

	printf("thread count %d\n", count);
	for(i = 0; i < count; i++) {
		write(fd, &ch, 1);
		printf("%c", ch);
	}
	printf("\n");

	return arg;
}

int
main(int argc, char **argv)
{
	pthread_t thread;
	char ch = 'O';
	int i, count;
	void *retval;

	fd = open(argv[2], O_RDWR);
	if(fd == -1)
		printf("open error\n");

	count = atoi(argv[1]);
	printf("main count %d\n", count);

	if(pthread_create(&thread, NULL, thread_func, &count))
		printf("create thread error\n");

	for(i = 0; i < count; i++) {
		write(fd, &ch, 1);
		printf("%c", ch);
	}
	printf("\n");

	if(pthread_join(thread, &retval))
		printf("join thread error\n");
	printf("retval %d\n", *( (int *)retval ) );

	close(fd);

	return 0;
}
