#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/resource.h>
#include <sys/wait.h>

double ssu_maketime(struct timeval *time);

void term_stat(int stat);

void ssu_print_child_info(int stat, struct rusage *rusage);

int main(void)
{
	struct rusage rusage;
	pid_t pid;
	int status;

	if ((pid = fork()) == 0) {
		char *args[] = {"find", "/", "-maxdepth", "4", "-name", "stdio.h", NULL};

		if (execv("/usr/bin/find", args) < 0) {
			fprintf(stderr, "execv error\n");
			exit(1);
		}
	}

	if (wait3(&status, 0, &rusage) == pid)
		ssu_print_child_info(status, &rusage);
	else {
		fprintf(stderr, "wait3 error\n");
		exit(1);
	}

	exit(0);
}

double ssu_maketime(struct timeval *time) {
	return ((double)time->tv_sec + (double)time->tv_usec/1000000.0);
}

void ssu_echo_exit(int status) {
	if (WIFEXITED(status))
		printf("normal termination, exit status = %d\n",
				WEXITSTATUS(status));
	else if (WIFSIGNALED(status))
		printf("abnormal termination, signal number = %d%s\n",
				WTERMSIG(status),
#ifdef WCOREDUMP
				WCOREDUMP(status) ? " (core file generated)" : "no core"
#else
				NULL
#endif
		      );
	else if (WIFSTOPPED(status))
		printf("child stopped, signal number = %d\n", WSTOPSIG(status));
}

void ssu_print_child_info(int stat, struct rusage *rusage) {
	printf("Termination info follows\n");
	term_stat(stat);
	printf("user CPU time : %.2f(sec)\n",
			ssu_maketime(&rusage->ru_utime));
	printf("system CPU time : %.2f(sec)\n",
			ssu_maketime(&rusage->ru_stime));
}
