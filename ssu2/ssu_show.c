#include "ssu_cleanupd.h"

int main(int argc, char *argv[])
{
	int developer_mode = 0; // [DEV_MODE]
	if (argc == 2) {
		if (strcmp(argv[1], "all") == 0)
			developer_mode = 1;
		else {
			fprintf(stderr, "[%s] must exist alone\n", argv[0]);
			exit(ERR_GRAMMER);
		}
	}
	if (argc > 2) {
		fprintf(stderr, "[%s] must exist alone\n", argv[0]);
		exit(ERR_GRAMMER);
	}

	struct ssu_daemon record;
	char buf[BUFFER_SIZE];
	char original_wd[PATH_MAX];
	int fd;
	int line;
	int skip;
	int line_max = 10; // 최대 출력 로그 줄 수
	int registed_num; // 등록된 데몬 프로세스의 개수
	ssize_t read_bytes;

	// 현재 경로의 절대경로 구하기
	if (getcwd(original_wd, sizeof(original_wd)) == NULL) {
		fprintf(stderr, "getcwd error\n");
		exit(1);
	}

	// ~/.ssu_cleanupd/current_daemon_list 파일 읽기
	if (chdir(dname) < 0) {
		fprintf(stderr, "[%s] - chdir error\n", dname);
		exit(1);
	}
	if ((fd = open(DMlist_Filename, O_RDONLY)) < 0) {
		fprintf(stderr, "open error for %s\n", DMlist_Filename);
		exit(1);
	}
	// 현재 등록된 데몬 프로세스의 개수 구하기
	registed_num = 0;
	while (read(fd, (char *)&record, sizeof(record)) > 0) {
		registed_num++;
	}

	// Daemon process list 출력
	printf("Current working daemon process list\n");
	while (1) {
		line = 0; // line 초기화
		// 1. 등록된 데몬 프로세스의 개수가 10개 초과면 최근 10개만 보여준다.
		skip = registed_num - line_max;
		if (skip < 0)
			skip = 0;
		// 2. [DEV_MODE]라면 모두 보여준다.
		if (developer_mode)
			skip = 0;

		printf("\n\n");
		printf("0. exit\n");
		while (1) {
			if (lseek(fd, (long)(skip+line) * sizeof(record), 0) < 0) {
				fprintf(stderr, "lseek error\n");
				exit(1);
			}

			if ((read_bytes = read(fd, (char *)&record, sizeof(record))) > 0)
				printf("%d. %s\n", ++line, record.real_path);
			else if (read_bytes == 0) {
				break;
			}
			else {
				fprintf(stderr, "read error\n");
				continue;
			}
		}

		if (chdir("..") < 0) {
			fprintf(stderr, "[..] - chdir error\n");
			exit(1);
		}
		printf("\n");

		// Select to see 구현
		int option;
		printf("Select one to see process info : ");
		if (scanf("%d", &option) < 1) {
			printf("Please check your input is valid\n");
			printf("\n");
			while (getchar() != '\n');
			continue;
		}

		if (option < 0 || option > line) {
			printf("Please check your input is valid\n");
			continue;
		}
		else if (option == 0) {
			break;
		}

		// option에 해당하는 줄의 pid와 real_path를 record 구조체에 저장한다.
		if (lseek(fd, (long)(skip+ option-1) * sizeof(record), SEEK_SET) < 0) {
			fprintf(stderr, "lseek error\n");
			continue;
		}
		if (read(fd, (char *)&record, sizeof(record)) > 0) {}

		if (chdir(record.real_path) < 0) {
			fprintf(stderr, "[%s] - chdir error\n", record.real_path);
			exit(1);
		}
		// 1. ssu_cleanupd.config 파일을 읽어온다.
		// 2. ssu_cleanupd.log 파일을 읽어온다.
		FILE *fp1 = fopen(config_Filename, "r");
		FILE *fp2 = fopen(log_Filename, "r");

		if (chdir(original_wd) < 0) {
			fprintf(stderr, "[%s] - chdir error\n", original_wd);
			exit(1);
		}

		if (!fp1 || !fp2) {
			fprintf(stderr, "fopen error for %s OR %s\n", config_Filename, log_Filename);
			if (fp1)
				fclose(fp1);
			if (fp2)
				fclose(fp2);
			continue;
		}

		// 파일 출력
		printf("1. config detail\n");
		printf("\n");
		while (fgets(buf, sizeof(buf), fp1)) {
			printf("%s", buf);
		}
		printf("\n");
		printf("2. log detail\n");
		printf("\n");
		while (fgets(buf, sizeof(buf), fp2)) {
			printf("%s", buf);
		}
		printf("\n");
		fclose(fp1);
		fclose(fp2);
		break;
	}

	close(fd);
	exit(0);
}
