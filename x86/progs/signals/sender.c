#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>

#define PROG_DEBUG
#include "debug.h"

int main(int argc, char *argv[])
{
	int numSigs, sig, j, ret;
	char *endptr;
	pid_t pid;

	if (argc < 4 || strcmp(argv[1], "--help") == 0) {
		printf("%s pid num-sigs sig-num [sig-num-2]\n", argv[0]);
		return -1;
	}

	pid = (int) strtol(argv[1], &endptr, 0);
	numSigs = (int) strtol(argv[2], &endptr, 0);
	sig = (int) strtol(argv[3], &endptr, 0);
//	DEBUG("The offset of sig end char is %d\n", (int) (endptr - argv[3]));

	DEBUG("%s: sending signal %d to process %d %d times\n",
			argv[0], sig, pid, numSigs);

	for (j = 0; j < numSigs; j++) {
		ret = kill(pid, sig);
		if (ret == -1) {
			printf("kill error\n");
			switch (errno) {
				case ESRCH:
					printf("target is not exist.\n");
					return -ESRCH;
				case EPERM:
					printf("target not permit.");
					return -EPERM;
			}
			return -1;
		}
	}

	if (argc > 4)
		if (kill(pid, strtol(argv[4], &endptr, 0)) == -1) {
			printf("kill error\n");
			return -1;
		}

	return 0;
}
