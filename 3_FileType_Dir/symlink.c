#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	// 파일의 경로명을 부여받는다. => symlink로 생성된 file2는 <file1>에 종속적이다.
	// link(하드 링크)와 다르게 symlink(심볼릭 링크)는 디렉토리 링크, 다른 파일시스템 간 링크가 가능하다.
	if (argc != 3) {
		fprintf(stderr, "usage: %s <actualname> <symname>\n", argv[0]);
		exit(1);
	}

	if (symlink(argv[1], argv[2]) < 0) {
		fprintf(stderr, "symlink error\n");
		exit(1);
	}
	else
		printf("simlink: %s -> %s\n", argv[2], argv[1]);

	exit(0);
}
