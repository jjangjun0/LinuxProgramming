#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

int main(void)
{
	sigset_t sig_set;
	int count;

	sigemptyset(&sig_set);
	sigaddset(&sig_set, SIGINT);
	sigprocmask(SIG_BLOCK, &sig_set, NULL);

	for (count = 3; 0 < count ; count--) {
		printf("count %d\n", count);
		sleep(1);
	}

	printf("Ctrl-C -> block release\n");
	sigprocmask(SIG_UNBLOCK, &sig_set, NULL);
	printf("If counting & Ctrl-C input, it doesn't output\n");

	while (1);

	exit(0);
}
