#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

void ssu_signal_handler(int signo);

void (*ssu_func)(int);

int main(void)
{
	ssu_func = signal(SIGINT, ssu_signal_handler);

	while (1) {
		printf("process running...\n");
		sleep(1);
	}

	exit(0);
}

void ssu_signal_handler(int signo) {
	printf("SIGINT signal generated.\n");
	printf("SIGINT -> SIG_DFL reset.\n");
	signal(SIGINT, ssu_func);
}
