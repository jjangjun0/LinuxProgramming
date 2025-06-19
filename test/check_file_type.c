#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>

int main(int argc, char *argv[])
{
	struct stat file_info;
	char *str;
	int i;
	
	struct timeval start, end;
	long seconds, nanoseconds;
	double elapsed;

	gettimeofday(&start, NULL);

	for (i = 1; i < argc; i++) {
		if (lstat(argv[i], &file_info) < 0) {
			fprintf(stderr, "lstat error for %s\n", argv[i]);
			continue;
		}
		printf("name = %s, ", argv[i]);

		printf("type = ");
		if (S_ISREG(file_info.st_mode))
			printf("regular file");
		else if (S_ISDIR(file_info.st_mode))
			printf("directory");
		else if (S_ISCHR(file_info.st_mode))
			printf("character special file");
		else if (S_ISBLK(file_info.st_mode))
			printf("block special file");
		else if (S_ISFIFO(file_info.st_mode))
			printf("FIFO");
		else if (S_ISLNK(file_info.st_mode))
			printf("symbolic link");
		else if (S_ISSOCK(file_info.st_mode))
			printf("socket");


		printf("\n");

	}

	gettimeofday(&end, NULL);

	seconds = end.tv_sec - start.tv_sec;
	nanoseconds = end.tv_usec - start.tv_usec;

	if (nanoseconds < 0) {
		seconds--;
		nanoseconds += 10000000;
	}

	elapsed = seconds + nanoseconds * 1e-9;
	printf("Elapsed time: %.9f seconds\n", elapsed);

	exit(0);
}
