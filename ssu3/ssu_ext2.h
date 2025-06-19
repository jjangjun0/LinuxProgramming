#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <ctype.h>

#include "ssu_ext2_struct.h"

// System rule
#define STRING_MAX 4096
#define FACTOR_MAX 4096
#define PATH_MAX 4096
#define NAME_MAX 255
#define BUFFER_SIZE 1024

// USER ERROR
#define ERR_GRAMMER 10

// Option type value
#define NON      0
#define O_R      1
#define O_S     10
#define O_P    100
#define O_N   1000

// Command type
char *commandSet[10] = {
	"tree",
	"print",
	"help",
	"exit"
};
// Option type
char *option_type[] = {
	"-r", "-s", "-p", "-n"
};

// string
const char *prompt = "20220000> ";

/*
 * ssu_ext2 program 종료하는 함수
 */
int Exit() {
	printf("\n");
	exit(0);
}
/*
 * parsing function
 */
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
/*
 * 자식 프로세스에게 ext2 filesystem image를 전송하기 위해 argv 끝에 삽입하여 전달하는 함수
 */
int input_img_insert_argv(int *argc, char *argv[], char *ext2_img_name) {
	argv[*argc] = (char *)malloc(strlen(ext2_img_name) + 1);
	strcpy(argv[*argc], ext2_img_name);
	(*argc)++;
	argv[*argc] = (char *)0;

	return 0;
}
/*
 * fork() & execv()
 */
int fork_execv(int argc, char *argv[], char *executableFile) {
	pid_t pid;
	int status;

	if ((pid = fork()) < 0) {
		fprintf(stderr, "fork error\n");
		return -1;
	}
	else if (pid == 0) {
		//printf("자식 프로세스 실행\n");
		if (execv(executableFile, argv) < 0) {
			fprintf(stderr, "execv error\n");
			exit(1);
		}
	}
	//printf("wait 실행 전\n");
	if (wait(&status) != pid) {
		fprintf(stderr, "wait error\n");
		return -1;
	}
	//printf("wait 실행 후\n");
	//ssu_echo_exit(status);

	return WEXITSTATUS(status);
}
/*
 * process exit status checking function
 */
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
