#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

int main(void)
{
	char *fname = "ssu_dump.txt";

	if (open(fname, O_RDWR) < 0) {
		fprintf(stderr, "open erro for %s\n", fname);
		exit(1);
	}

	if (unlink(fname) < 0) {
		fprintf(stderr, "unlink error for %s\n", fname);
		exit(1);
	}
	printf("File unlinked\n");
	sleep(20);
	printf("Done\n");
	exit(0);
}
