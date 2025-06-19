#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <syslog.h>

#define NAME_SIZE 50

struct employee {
	char name[NAME_SIZE];
	int salary;
	int pid;
};

int main(int argc, char *argv[])
{
	struct employee record;
	struct flock lock;
	int fd, recnum, position;
	char ans[5];

	if (argc < 2) {
		fprintf(stderr, "usage: %s file\n", argv[0]);
		exit(1);
	}

	if ((fd = open(argv[1], O_RDWR)) < 0) {
		fprintf(stderr, "open error for %s\n", argv[1]);
		exit(1);
	}

	for (;;) {
		printf("\nEnter record number: ");
		scanf("%d", &recnum);
		if (recnum < 0)
			break;

		position = recnum * sizeof(record);
		lock.l_type = F_RDLCK;
		lock.l_whence = 0;
		lock.l_start = position;
		lock.l_len = sizeof(record);
		if (fcntl(fd, F_SETLKW, &lock) == -1) {
			fprintf(stderr, "fcntl F_RDLCK error\n");
			lock.l_type = F_UNLCK;
			fcntl(fd, F_SETLK, &lock);
			continue;
		}
		lseek(fd, position, 0);
		if (read(fd, (char *)&record, sizeof(record)) == 0) {
			printf("%d record not found\n", recnum);
			lock.l_type = F_UNLCK;
			fcntl(fd, F_SETLK, &lock);
			continue;
		}
		printf("Employee: %s, salary: %d\n", record.name, record.salary);
		printf("Do you want to update salary (y or n)? ");
		scanf("%s", ans);

		if (ans[0] != 'y') {
			lock.l_type = F_UNLCK;
			fcntl(fd, F_SETLK, &lock);
			continue;
		}
		printf("Enter new salary: ");
		scanf("%d", &record.salary);

		lock.l_type = F_WRLCK;
		if (fcntl(fd, F_SETLKW, &lock) == -1) {
			fprintf(stderr, "fcntl F_WRLCK error\n");
			lock.l_type = F_UNLCK;
			fcntl(fd, F_SETLK, &lock);
			exit(2);
		}
		
		record.pid = getpid();
		lseek(fd, position, 0);
		if (write(fd, &record, sizeof(record)) != sizeof(record)) {
			fprintf(stderr, "write error\n");
			lock.l_type = F_UNLCK;
			fcntl(fd, F_SETLK, &lock);
			exit(3);
		}
	}
	printf("Exiting...\n");
	close(fd);
	exit(0);
}
