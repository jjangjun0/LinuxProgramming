#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

static void ssu_signal_handler(int signo);

int main(void)
{
	struct sigaction sig;
	sigset_t sig_set, sig_set2;

	sig.sa_handler = ssu_signal_handler;
	sig.sa_flags = 0;
	sigemptyset(&sig.sa_mask);

	if (sigaction(SIGUSR1, &sig, NULL) != 0) {
		fprintf(stderr, "sigaction error\n");
		exit(1);
	}

	sigemptyset(&sig_set);
	sigaddset(&sig_set, SIGUSR1);
	if (sigprocmask(SIG_BLOCK, &sig_set, NULL) != 0) {
		fprintf(stderr, "sigprocmask error\n");
		exit(1);
	}
	printf("SIGUSR1 signals are now blocked\n");
	kill(getpid(), SIGUSR1);
	printf("after kill()\n");

	if (sigpending(&sig_set2) == 0) {
		if (sigismember(&sig_set2, SIGUSR1) != 0)
			printf("a SIGUSR1 signal is pending\n");
		else
			printf("SIGUSR1 signals are not pending\n");
	}
	
	if (sigprocmask(SIG_UNBLOCK, &sig_set, NULL) != 0) {
		fprintf(stderr, "sigprocmask error\n");
		exit(1);
	}
	printf("SIGUSR1 signals are no longer blocked\n");

	if (sigpending(&sig_set2) == 0) {
		if (sigismember(&sig_set2, SIGUSR1) != 0)
			printf("a SIGUSR1 signal is pending\n");
		else
			printf("SIGUSR1 signals are not pending\n");
	}
	
	exit(0);
}

static void ssu_signal_handler(int signo) {
	printf("in ssu_signal_handler function\n");
}
