#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#define CREAT_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

/*
 S_IRWXU - 00700
 S_IRWXG - 00070
 S_IRWXO - 00007

 S_IRUSR - 00400
 S_IWUSR - 00200
 S_IXUSR - 00100

 S_IRGRP - 00040
 S_IWGRP - 00020
 S_IXGRP - 00010

 S_IROTH - 00004
 S_IWOTH - 00002
 S_IXOTH - 00001
 */

char buf1[] = "1234567890";
char buf2[] = "ABCDEFGHIJ";

int main(void)
{
	char *fname = "ssu_hole.txt";
	int fd;

	if ((fd = creat(fname, CREAT_MODE)) < 0) {
		fprintf(stderr, "creat error for %s\n", fname);
		exit(1);
	}

	//if (write(fd, buf1, 12) != 12) {
	if (write(fd, buf1, 12) != 12) {
		fprintf(stderr, "write error for buf1\n");
		exit(1);
	}

	if (lseek(fd, 15000, SEEK_SET) < 0) {
		fprintf(stderr, "lseek error\n");
		exit(1);
	}

	if (write(fd, buf2, 12) != 12) {
		fprintf(stderr, "write error for buf2\n");
		exit(1);
	}

	exit(0);
}
