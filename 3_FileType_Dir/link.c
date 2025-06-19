#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	// 독립된 inode를 부여받는다. => link로 생성된 file2는 <file1>과 독립적이다.
	if (argc < 3) {
		fprintf(stderr, "usage: %s <file1> <file2>\n", argv[0]);
		exit(1);
	}

	if (link(argv[1], argv[2]) == -1) {
		fprintf(stderr, "link error for %s\n", argv[1]);
		exit(1);
	}

	exit(0);
}
