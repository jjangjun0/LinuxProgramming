#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
int main(void)
{
	if (chdir("/etc") < 0) {
		fprintf(stderr, "chdir error\n");
		exit(1);
	}

	printf("chdir to /etc succeeded.\n");
	exit(0);
}
