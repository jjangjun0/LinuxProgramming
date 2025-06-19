#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	int i;
	if (argc < 2) {
		fprintf(stderr, "usage: %s <file1> <file2> .. <fileN>\n", argv[0]);
		exit(1);
	}

	for (i = 1; i < argc; i++) {
		if (access(argv[i], F_OK) < 0) {
			fprintf(stderr, "%s doesn't exits.\n", argv[i]);
			continue;
		}

		if (access(argv[i], R_OK) == 0)
			printf("User can read %s\n", argv[i]);
		if (access(argv[i], W_OK) == 0)
			printf("User can write %s\n", argv[i]);
		if (access(argv[i], X_OK) == 0)
			printf("User can execute %s\n", argv[i]);
	}
	exit(0);
}
