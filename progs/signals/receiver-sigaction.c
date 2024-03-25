#include <sys/types.h>
#include <unistd.h>

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

void printSigset(FILE *of, const char *prefix, const sigset_t *sigset)
{
    int sig, cnt;

    cnt = 0;
    for (sig = 1; sig < NSIG; sig++) {
        if (sigismember(sigset, sig)) {
            cnt++;
            fprintf(of, "%s%d (%s)\n", prefix, sig, strsignal(sig));
        }
    }

    if (cnt == 0)
        fprintf(of, "%s<empty signal set>\n", prefix);
}

static void handler1(int sig)
{
	printf("handler1 sig %d\n", sig);
	sleep(10);
	printf("handler1 quit\n");
}

static void handler2(int sig)
{
	printf("handler2 sig %d\n", sig);
	printf("block SIGUSR1\n");
	sleep(10);
	printf("unblock SIGUSR1\n");
	printf("handler2 quit\n");
}

int main(int argc, char *argv[])
{
	int i;
	sigset_t mask;
	struct sigaction sa, sa_;

	printf("%s: PID is %d\n", argv[0], getpid());

	if (sigemptyset(&mask) == -1) {
		printf("empty error\n");
		return -1;
	}

	sa.sa_handler = handler1;
	sa.sa_mask = mask;
	sa.sa_flags = 0;

	/* no block SIGUSR1 */
	if (sigaction(SIGINT, &sa, NULL) == -1) {
		printf("sigaction error\n");
		return -1;
	}

	if (sigaddset(&mask, SIGUSR1) == -1) {
		printf("add error\n");
		return -1;
	}

	sa.sa_handler = handler2;
	sa.sa_mask = mask;
	sa.sa_flags = 0;

	for (i = 0; 1; i++) {
		if (i % 2 == 0) {
			if (sigaction(SIGINT, &sa, &sa_) == -1) {
				printf("sigaction error\n");
				return -1;
			}
		} else {
			if (sigaction(SIGINT, &sa_, &sa) == -1) {
				printf("sigaction error\n");
				return -1;
			}
		}
	}

	printf("success\n");
	return 0;
}
