#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#define N 10

static void ssu_alarm(int signo) {
	printf("Awake!\n");
}
int my_sleep(unsigned int seconds);

int main(void)
{
	// sleep() -> alarm() & pause()
	
	struct sigaction sig;
	unsigned int seconds;
	char time_str[N];

	sig.sa_handler = ssu_alarm;
	sig.sa_flags = 0;
	sigemptyset(&sig.sa_mask);

	if (sigaction(SIGALRM, &sig, NULL) != 0) {
		fprintf(stderr, "sigaction error\n");
		exit(1);
	}

	while (1) {
		printf("How many seconds should I sleep? (-1 to exit): ");
		scanf("%s", time_str);

		// input error
		if (strcmp(time_str, "0") == 0) {
			printf("Please enter a value greater than 0\n");
			continue;
		}
		else if (strcmp(time_str, "-1") == 0) {
			printf("The program will now exit\n");
			break;
		}
		for (int i = 0; time_str[i] != '\0'; i++) {
			if (!(time_str[i] >= '0' && time_str[i] <= '9')) {
				fprintf(stderr, "Input error. The program will now exit..\n");
				exit(1);
			}
		}
		if (strlen(time_str) == N && strcmp(time_str, "410065408") > 0) {
			fprintf(stderr, "Input is so big number\n");
			continue;
		}

		seconds = atoi(time_str);
		my_sleep(seconds);
	}
	printf("\nExiting the program.\n");
	exit(0);
}

int my_sleep(unsigned int seconds) {
	printf("Sleeping for %u seconds...\n", seconds);
	alarm(seconds);
	pause();
}
