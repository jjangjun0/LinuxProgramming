#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <syslog.h>

int daemon_init(void);

int main(void)
{
	if (daemon_init() < 0) {
		fprintf(stderr, "daemon error\n");
		exit(1);
	}

	for (int i = 0; i < 3; i++) {
		openlog("mydaemon", LOG_PID | LOG_CONS, LOG_DAEMON);
		syslog(LOG_INFO, "Daemon (PID %d) is alive\n", getpid());
		closelog();

		sleep(5);
	}
	exit(0);
}

int daemon_init(void) {
	pid_t pid;
	int fd, maxfd;

	if ((pid = fork()) < 0) {
		fprintf(stderr, "fork error\n");
		exit(1);
	}
	else if (pid != 0)
		exit(0);

	pid = getpid();

	setsid();
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	maxfd = getdtablesize();

	for (fd = 0; fd < maxfd; fd++)
		close(fd);
	umask(0);
	chdir("/");
	fd = open("/dev/null", O_RDWR);
	dup(0);
	dup(0);
	return 0;
}
