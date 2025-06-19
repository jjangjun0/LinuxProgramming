#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>

// System rule
#define STRING_MAX 4096
#define FACTOR_MAX 4096
#define PATH_MAX 4096
#define NAME_MAX 255
#define BUFFER_SIZE 1024
#define S_MODE 0644
#define D_MODE 0755

// Option type value
#define NON      0
#define O_D      1
#define O_I     10
#define O_L    100
#define O_X   1000
#define O_E  10000
#define O_M 100000

// Help type value
#define H_SHOW   1
#define H_ADD    2
#define H_MODIFY 3
#define H_REMOVE 4
#define H_HELP   5
#define H_EXIT   6

// Customization exit error
#define ERR_GRAMMER 10

// Command type
char *commandSet[10] = {
	"show",
	"add",
	"modify",
	"remove",
	"help",
	"exit"
};
// Option type
char *option_type[] = {
	"-d", "-i", "-l", "-x", "-e", "-m"
};
// string
const char *prompt = "20220000> ";
char *dname = ".ssu_cleanupd";
char *DMlist_Filename = "current_daemon_list";
char *config_Filename = "ssu_cleanupd.config";
char *log_Filename = "ssu_cleanupd.log";

// Daemon Data Strict
// -> current_daemon_list
struct ssu_daemon {
	pid_t pid;
	char real_path[PATH_MAX];
};

// -> ssu_cleanupd.config
struct ssu_daemon_config {
	char monitoring_path[PATH_MAX];
	pid_t pid;
	time_t start_time;
	char output_path[PATH_MAX];
	unsigned int time_interval;
	char max_log_lines[16];
	char exclude_path[PATH_MAX];
	char extension[NAME_MAX];
	int mode;
};
// File Type Value //
/*
   Unknown file     : 0
   Regular file     : 1
   Directory        : 2
   Character device : 3
   Block device     : 4
   FIFO             : 5
   Socket           : 6
   Symbolic link    : 7
 */


/* process exit status checking function */
void ssu_echo_exit(int status) {
	if (WIFEXITED(status))
		printf("normal termination, exit status = %d\n", WEXITSTATUS(status));
	else if (WIFSIGNALED(status))
		printf("abnormal termination, signal number = %d%s\n", WTERMSIG(status),
#ifdef WCOREDUMP
			WCOREDUMP(status) ? " (core file generated)" : "");
#else
			"");
#endif
	else if (WIFSTOPPED(status))
		printf("child stopped, signal number = %d\n", WSTOPSIG(status));
}

/* parsing function */
// 시작과 끝의 index를 가지고 문자열을 생성하는 함수
char *make_str(int start, int end, char *str) {
	char *res = (char*)malloc(sizeof(char) * (end-start + 2));

	for (int i = start; i <= end; i++) {
		// 동적 할당된 배열에는 index = 0부터 저장한다.
		res[i-start] = str[i];
	}
	res[end-start + 1] = '\0';

	return res;
}
// 토큰화 함수
void get_factor(int *argc, char *argv[], char *str) {
	int start_idx = -1;
	int end_idx = -1;
	int length = strlen(str);

	for (int i = 0; i <= length; i++) {
		if (str[i] != ' ' && str[i] != '\0') {
			// start_idx가 지정되지 않았다면 지정한다.
			if (start_idx == -1)
				start_idx = i;
		}
		else if (start_idx != -1) {
			// start_idx가 지정된 경우에 해당 인자의 끝 지점을 찾는다.
			for (int j = start_idx; j <= length; j++) {
				if (str[j] == ' ' || str[j] == '\0') {
					end_idx = j-1;

					argv[*argc] = make_str(start_idx, end_idx, str);
					(*argc)++;

					// i 동기화
					i = j;
					start_idx = -1;
					break;
				}
			}
		}
	}
	argv[*argc] = (char *)0;
}

/* Check functions */
int CheckDirIndex(int argc, char *argv[]) {
	int idx = 0;
	
	// argv[0] = command
	for (int i = 1; i < argc; i++) {
		// option이 나오면 pass
		if (strcmp(argv[i], option_type[0]) == 0 ||
				strcmp(argv[i], option_type[1]) == 0 ||
				strcmp(argv[i], option_type[2]) == 0 ||
				strcmp(argv[i], option_type[3]) == 0 ||
				strcmp(argv[i], option_type[4]) == 0 ||
				strcmp(argv[i], option_type[5]) == 0)
			continue;
		else {
			idx = i;
			break; // 첫 빠따가 <DIR_PATH>
		}
	}
	return idx;	
}

int CheckPathAccess(char *path) {
	int type = 0;
	if (access(path, F_OK) < 0)
		return -1;

	if (access(path, R_OK) == 0)
		type += 4;
	if (access(path, W_OK) == 0)
		type += 2;
	if (access(path, X_OK) == 0)
		type += 1;
	return type;
}

int CheckPathType(char *path) {
	struct stat statbuf;
	int type;

	if (lstat(path, &statbuf) < 0) {
		fprintf(stderr, "lstat error\n");
		return -1;
	}

	if (S_ISREG(statbuf.st_mode))
		type = 1; // Regular file
	else if (S_ISDIR(statbuf.st_mode))
		type = 2; // Directory
	else if (S_ISCHR(statbuf.st_mode))
		type = 3; // Character device
	else if (S_ISBLK(statbuf.st_mode))
		type = 4; // Block device
	else if (S_ISFIFO(statbuf.st_mode))
		type = 5; // Named pipe
	else if (S_ISSOCK(statbuf.st_mode))
		type = 6; // Socket
	else if (S_ISLNK(statbuf.st_mode))
		type = 7; // Symbolic link
	else
		type = 0; // Unknown

	return type;
}

int CheckPath_InsideHome(char *real_path) {
	char *home = getenv("HOME");
	if (!home) {
		fprintf(stderr, "HOME's environmental variable couldn't be found.\n");
		return -1;
	}

	if (strncmp(real_path, home, strlen(home)) == 0)
		return 1;
	return 0;
}

/* remnant */
int make_optionValue(int argc, char *argv[]) {
	int option_value = NON;
	
	for (int i = 0; i < argc; i++) {
		if (strcmp(argv[i], option_type[0]) == 0)
			option_value += O_D;
		else if (strcmp(argv[i], option_type[1]) == 0)
			option_value += O_I;
		else if (strcmp(argv[i], option_type[2]) == 0)
			option_value += O_L;
		else if (strcmp(argv[i], option_type[3]) == 0)
			option_value += O_X;
		else if (strcmp(argv[i], option_type[4]) == 0)
			option_value += O_E;
		else if (strcmp(argv[i], option_type[5]) == 0)
			option_value += O_M;
	}
	return option_value;
}
int optFactor_jumpIdx(int argc, char *argv[], char *stop_option, int option_factor_num[6]) {
	int idx = 0;
	for (int i = 1; i < argc; i++) {
		// stop_option을 만나면 종료
		if (strcmp(argv[i], stop_option) == 0)
			break;
					
		if (strcmp(argv[i], option_type[0]) == 0)
			idx += option_factor_num[0];
		else if (strcmp(argv[i], option_type[1]) == 0)
			idx += option_factor_num[1];
		else if (strcmp(argv[i], option_type[2]) == 0)
			idx += option_factor_num[2];
		else if (strcmp(argv[i], option_type[3]) == 0)
			idx += option_factor_num[3];
		else if (strcmp(argv[i], option_type[4]) == 0)
			idx += option_factor_num[4];
		else if (strcmp(argv[i], option_type[5]) == 0)
			idx += option_factor_num[5];

		// Segmentation Fault 방지
		if (idx > argc)
			break;
	}

	return idx;
}
