#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

int main(void)
{
	int fd;
	int val;

	if ((fd = open("exec_copy.txt", O_CREAT)) < 0) {
		fprintf(stderr, "open error for %s\n", "exec_copy.txt");
		exit(1);
	}
	val = fcntl(fd, F_GETFD, 0);

	if (val & FD_CLOEXEC)
		printf("close-on-exec bit on\n");
	else
		printf("close-on-exec bit off\n");

	val |= FD_CLOEXEC;

	if (val & FD_CLOEXEC)
		printf("close-on-exec bit on\n");
	else
		printf("close-on-exec bit off\n");

	fcntl(fd, F_SETFD, val);
	execl("/home/oslab/loop", "./loop", NULL);
}
