
#include "ssu_cleanupd.h"
#include <ctype.h>

int fork_execv(int argc, char *argv[], char *executableFile);
int option_factor_NumToStr_storing(int *argc, char *argv[], int *option_factor_num);
int Exit(int argc);

int main(void)
{
	int fd;
	char root_path[PATH_MAX];
	char root_realPath[PATH_MAX]; // root 작업 디렉토리의 절대 경로 (복구를 위해)
	if (getcwd(root_path, sizeof(root_path)) == NULL) {
		fprintf(stderr, "getcwd error\n");
		exit(1);
	}
	if (!realpath(root_path, root_realPath)) {
		fprintf(stderr, "realpath error\n");
		exit(1);
	}
	//printf("root_realPath: %s\n", root_realPath);

	if (CheckPathAccess(dname) < 0 || CheckPathType(dname) != 2) {
		if (mkdir(dname, D_MODE) < 0) {
			perror("mkdir");
			fprintf(stderr, "<%s> directory didn't created\n", dname);
			exit(1);
		}
	}
	if (chdir(dname) < 0) {
		fprintf(stderr, "[0] - chdir error\n");
		exit(1);
	}
	if (CheckPathAccess(DMlist_Filename) < 0 || CheckPathType(DMlist_Filename) != 1) {
		if ((fd = open(DMlist_Filename, O_WRONLY | O_CREAT | O_TRUNC, S_MODE)) < 0) {
			fprintf(stderr, "<%s> file didn't created\n", DMlist_Filename);
			exit(1);
		}
		close(fd);
	}
	if (chdir("..") < 0) {
		fprintf(stderr, "[1] - chdir error\n");
		exit(1);
	}
	
	//int n = 1;
	while (1) {
		//printf("===== %d번 =====\n", n++);
		int argc = 0;
		char *argv[FACTOR_MAX] = { NULL };
		int opt;
		int option_value = NON;

		// 명령어 입력 받기
		char input[STRING_MAX];
		fputs(prompt, stdout);
		if (fgets(input, sizeof(input), stdin) == NULL) {
			fprintf(stderr, "fgets() error\n");
			exit(1);
		}

		if (input[0] == '\n')
			continue;
		if (input[strlen(input) - 1] != '\n')
			continue;
		// input 배열 끝에 있는 '\n' 제거
		input[strlen(input) - 1] = '\0';
		
		// 토큰화
		get_factor(&argc, argv, input);

		/* -i, -l, -m 인자에 음수가 오는 경우를 방지한다.
		   또한 getopt 함수의 에러를 예방하기도 한다. */
		int isFactor_err = 0;
		for (int i = 0; i < argc; i++) {
			if ((strcmp(argv[i], option_type[1]) == 0 && i <= argc-2) || 
				(strcmp(argv[i], option_type[2]) == 0 && i <= argc-2) ||
			 	(strcmp(argv[i], option_type[5]) == 0 && i <= argc-2)) {
				char *endptr;
				long n = strtol(argv[i+1], &endptr, 10);
				if (strcmp(argv[i], option_type[5]) == 0) {
					if (*endptr != '\0') {
						fprintf(stderr, "Invalid factor [%s] entered -m option\n", endptr);
						isFactor_err = 1; // Flag on
						continue;
					}
					else
						if (n < 0) {
							fprintf(stderr, "The value is out of range[1 ~ 3]\n");
							isFactor_err = 1; // Flag on
							break;
						}
				}
				else {
					if (*endptr != '\0') {
						fprintf(stderr, "Invalid factor [%s] entered with -i or -l option\n", endptr);
						isFactor_err = 1; // Flag on
						break;
					}
					else
						if (n < 0) {
							fprintf(stderr, "The -i or -l option is limited to natural numbers\n");
							isFactor_err = 1; // Flag on
							break;
						}
				}
			}
		}
		if (isFactor_err == 1)
			continue;


		// { -d, -i, -l, -x, -e, -m } 각 옵션에 해당하는 인자의 개수 구하기
		int option_factor_num[6] = { 0 };
		int flag = -1;
		for (int i = 0; i < argc; i++) {
			if (strcmp(argv[i], option_type[0]) == 0) {
				flag = 0;
				continue;
			}
			if (strcmp(argv[i], option_type[1]) == 0) {
				flag = 1;
				continue;
			}
			if (strcmp(argv[i], option_type[2]) == 0) {
				flag = 2;
				continue;
			}
			if (strcmp(argv[i], option_type[3]) == 0) {
				flag = 3;
				continue;
			}
			if (strcmp(argv[i], option_type[4]) == 0) {
				flag = 4;
				continue;
			}
			if (strcmp(argv[i], option_type[5]) == 0) {
				flag = 5;
				continue;
			}

			if (flag >= 0 && flag < 6)
				option_factor_num[flag]++;
		}
		
		// 처리되는 인자의 숫자 초기화
		optind = 1;
		// getopt함수 자체 에러 메세지를 출력하지 않는다.
		opterr = 0;
		// 입력한 인자 처리 (getopt함수는 argc >= 2일 때 동작한다.)
		if (argc > 1) {
			while ((opt = getopt(argc, argv, "dilxem")) != -1) {
				switch (opt) {
					case 'd':
						option_value += O_D; break;
					case 'i':
						option_value += O_I; break;
					case 'l':
						option_value += O_L; break;
					case 'x':
						option_value += O_X; break;
					case 'e':
						option_value += O_E; break;
					case 'm':
						option_value += O_M; break;
					case '?':
						if (isdigit(optopt))
							continue;
						else {
							fprintf(stderr, "%c is not valid option!\n", optopt);
							continue;
						}
					default:
						continue;
				}
			}
		}

		// 모든 작업은 처음의 작업 디렉토리에서 시작한다.
		if (chdir(root_realPath) < 0) {
			fprintf(stderr, "[2] - chdir error\n");
			continue;
		}

		/* 각 command에 해당하는 것을 자식 process에서 실행한다.*/
		// 올바르지 않은 인자의 처리도 수반한다.
		char *executableFile;
		int option_error = 0;
		if (strcmp(argv[0], commandSet[0]) == 0) {
			if (option_value != NON) {
				fprintf(stderr, "%s didn't require an option\n", argv[0]);
				continue;
			}

			executableFile = "./ssu_show";
		}
		else if (strcmp(argv[0], commandSet[1]) == 0) {
			// 같은 옵션이 두 번 입력된 경우 에러 처리
			int position1;
			while (option_value != 0) {
				position1 = option_value % 10;
				if (!(position1 == 1 || position1 == 0)) {
					fprintf(stderr, "Duplicate option entered\n");
					option_error = 1;
				}
				option_value /= 10;
			}

			if (option_error == 1)
				continue;
			// option 구분 필요
			option_factor_NumToStr_storing(&argc, argv, option_factor_num);
			executableFile = "./ssu_add";
		}
		else if (strcmp(argv[0], commandSet[2]) == 0) {
			// 같은 옵션이 두 번 입력된 경우 에러 처리
			int position1;
			while (option_value != 0) {
				position1 = option_value % 10;
				if (!(position1 == 1 || position1 == 0)) {
					fprintf(stderr, "Duplicate option entered\n");
					option_error = 1;
				}
				option_value /= 10;
			}

			if (option_error == 1)
				continue;
			// option 구분 필요
			option_factor_NumToStr_storing(&argc, argv, option_factor_num);
			executableFile = "./ssu_modify";
		}
		else if (strcmp(argv[0], commandSet[3]) == 0) {
			if (option_value != NON) {
				fprintf(stderr, "%s didn't require an option\n", argv[0]);
				continue;
			}
			executableFile = "./ssu_remove";
		}
		else if (strcmp(argv[0], commandSet[4]) == 0) {
			if (option_value != NON) {
				fprintf(stderr, "%s didn't require an option\n", argv[0]);
				continue;
			}

			executableFile = "./ssu_help";
		}
		else if (strcmp(argv[0], commandSet[5]) == 0) {
			if (option_value != NON) {
				fprintf(stderr, "%s didn't require an option\n", argv[0]);
				continue;
			}

			Exit(argc);
		}
		else {
			printf("%s는 지정된 command가 아닙니다.\n", argv[0]);
			char *args[] = { "help", NULL };
			fork_execv(1, args, "./ssu_help");
			printf("\n");
			continue;
		}

		/*
		printf("\nexecv 하기 전 argv 확인\n");
		for (int i = 0; i < argc; i++) printf("argv[%d]: %s \n", i, argv[i]);
		printf("\n");
		*/
		int status;
		if ((status = fork_execv(argc, argv, executableFile)) < 0) {
			fprintf(stderr, "fork_execv error\n");
			continue;
		}
		else if (status == ERR_GRAMMER) {
			// 사용자 지정 문법 에러 문구 처리
			if (strcmp(argv[0], commandSet[0]) == 0) {
				char *args[] = { "help", "show", NULL };
				if (fork_execv(2, args, "./ssu_help") < 0) {
					fprintf(stderr, "fork_execv error\n");
					continue;
				}
			}
			else if (strcmp(argv[0], commandSet[1]) == 0) {
				char *args[] = { "help", "add", NULL };
				if (fork_execv(2, args, "./ssu_help") < 0) {
					fprintf(stderr, "fork_execv error\n");
					continue;
				}
			}
			else if (strcmp(argv[0], commandSet[2]) == 0) {
				char *args[] = { "help", "modify", NULL };
				if (fork_execv(2, args, "./ssu_help") < 0) {
					fprintf(stderr, "fork_execv error\n");
					continue;
				}
			}
			else if (strcmp(argv[0], commandSet[3]) == 0) {
				char *args[] = { "help", "remove", NULL };
				if (fork_execv(2, args, "./ssu_help") < 0) {
					fprintf(stderr, "fork_execv error\n");
					continue;
				}
			}
			else if (strcmp(argv[0], commandSet[4]) == 0) {
				char *args[] = { "help", "help", NULL };
				if (fork_execv(2, args, "./ssu_help") < 0) {
					fprintf(stderr, "fork_execv error\n");
					continue;
				}
			}
		}
	}

	exit(0);
}

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
int option_factor_NumToStr_storing(int *argc, char *argv[], int *option_factor_num) {
	char result[STRING_MAX] = "";
	char factor_num[12];

	for (int i = 0; i < 6; i++) {
		sprintf(factor_num, "%d", option_factor_num[i]);
		strcat(result, factor_num);
		strcat(result, " ");
	}

	argv[*argc] = (char *)malloc(strlen(result) + 1);
	strcpy(argv[*argc], result);
	(*argc)++;
	argv[*argc] = (char *)0;

	return 0;
}
int Exit(int argc) {
	if (argc != 1) {
		fprintf(stderr, "input only \"exit\"\n");
		return -1;
	}
	exit(0);
}
