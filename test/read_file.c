#include <stdio.h>
#include <stdlib.h>

#define BUFFER_SIZE 1024

int main(void)
{
	char buf[BUFFER_SIZE];
	ssize_t num_read;

	if ((num_read = read(0, &buf, sizeof(buf))) < 0) {
		fprintf(stderr, "read error\n");
		exit(1);
	}
	buf[num_read] = '\0';

	printf("%s", buf);
	exit(0);
}
