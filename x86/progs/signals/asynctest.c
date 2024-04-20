/*
 * asynctest.c: use async notification to read stdin
 *
 * Copyright (C) 2001 Alessandro Rubini and Jonathan Corbet
 * Copyright (C) 2001 O'Reilly & Associates
 *
 * The source code in this file can be freely used, adapted,
 * and redistributed in source or binary form, so long as an
 * acknowledgment appears in derived source files.  The citation
 * should list that the code comes from the book "Linux Device
 * Drivers" by Alessandro Rubini and Jonathan Corbet, published
 * by O'Reilly & Associates.   No warranty is attached;
 * we cannot take responsibility for errors or fitness for use.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>

const char dev[] = "/dev/scull_pipe0";

int gotdata=0;
void sighandler(int signo)
{
    if (signo==SIGIO)
        gotdata++;
    return;
}

char buffer[4096];

int main(int argc, char **argv)
{
    int count, devfd;
    struct sigaction action;

    devfd = open(dev, O_RDWR);
    if (devfd == -1) {
	    printf("open error\n");
	    return -1;
    }

    memset(&action, 0, sizeof(action));
    action.sa_handler = sighandler;
    action.sa_flags = 0;

    sigaction(SIGIO, &action, NULL);

    fcntl(devfd, F_SETOWN, getpid());
    fcntl(devfd, F_SETFL, fcntl(devfd, F_GETFL) | FASYNC);

    printf("%s: PID is %d\n", argv[0], getpid());

    while(1) {
        /* this only returns if a signal arrives */
        sleep(86400); /* one day */
        if (!gotdata)
            continue;
        count=read(devfd, buffer, 4096);
	buffer[count] = '\0';
        /* buggy: if avail data is more than 4kbytes... */

	printf("%s: Got %d bytes\n", argv[0], count);
	printf("%s\n", buffer);

        count=write(devfd, buffer, count);
        gotdata=0;
    }
}
