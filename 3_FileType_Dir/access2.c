#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define TABLE_SIZE (sizeof(table)/sizeof(*table))

int main(int argc, char *argv[])
{
	struct {
		char *text;
		int mode;
	} table[] = {
		{"exists", 0},
		{"execute", 1},
		{"write", 2},
		{"read", 4}
	};
	int i;

	if (argc < 2) {
		fprintf(stderr, "usage : %s <file>\n", argv[0]);
		exit(1);
	}

	for (i = 0; i < TABLE_SIZE; i++) {
		if (access(argv[1], table[i].mode) != -1)
			printf("%s -ok\n", table[i].text);
		else
			printf("%s\n", table[i].text);
	}
	exit(0);
}
