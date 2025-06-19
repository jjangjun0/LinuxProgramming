#include "ssu_ext2.h"

int main(int argc, char *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "Usage Error : ./ssu_ext2 <EXT2_IMAGE>\n\n");
		exit(1);
	}

	int ext2_img_length = strlen(argv[1]);
	char *ext2_img_name = (char *)malloc(ext2_img_length + 1);
	for (int i = 0; i < ext2_img_length; i++) {
		ext2_img_name[i] = argv[1][i];
	}
	ext2_img_name[ext2_img_length] = '\0';

	int fd;
	struct ext2_super_block sb;
	if ((fd = open(ext2_img_name, O_RDONLY)) < 0) {
		fprintf(stderr, "open error for %s\n", ext2_img_name);
		exit(1);
	}

	// SuperBlock은 정적인 위치에 존재한다. (block #0에서 1024 지점)
	if (lseek(fd, EXT2_SUPERBLOCK_OFFSET, SEEK_SET) < 0) {
		fprintf(stderr, "lseek error for EXT2_SUPERBLOCK_OFFSET\n");
		exit(1);
	}
	if (read(fd, &sb, sizeof(sb)) != sizeof(sb)) {
		fprintf(stderr, "read error for super block\n");
		exit(1);
	}

	// EXT2 magic number 확인
	if (sb.s_magic != EXT2_SUPER_MAGIC) {
		fprintf(stderr, "<%s> is not a valid ext2 file system (magic=0X%x)\n", ext2_img_name, sb.s_magic);
		exit(1);
	}
	close(fd);

	while (1) {
		int argc = 0;
		char *argv[FACTOR_MAX] = { NULL };

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

		
		/* 각 command에 해당하는 것을 자식 process에서 실행한다.*/
		char *executableFile;

		if (strcmp(argv[0], commandSet[0]) == 0) {
			input_img_insert_argv(&argc, argv, ext2_img_name);
			executableFile = "./ssu_tree";
		}
		else if (strcmp(argv[0], commandSet[1]) == 0) {
			input_img_insert_argv(&argc, argv, ext2_img_name);
			executableFile = "./ssu_print";
		}
		else if (strcmp(argv[0], commandSet[2]) == 0) {
			executableFile = "./ssu_help";
		}
		else if (strcmp(argv[0], commandSet[3]) == 0) {
			if (argc > 1) {
				fprintf(stderr, "Error: Only input \"exit\"\n");
				char *args[] = { "help", "exit", NULL };
				if (fork_execv(2, args, "./ssu_help") < 0) {
					fprintf(stderr, "fork_execv error\n");
					continue;
				}
				continue; // "exit" 단독 입력하지 않은 경우 프로그램은 종료되지 않는다.
			}
			else {
				Exit();
			}
		}
		else {
			fprintf(stderr, "Error: \"%s\" is not a specified command.\n", argv[0]);
			char *args[] = { "help", NULL };
			fork_execv(1, args, "./ssu_help");
			printf("\n");
			continue;
		}

		int status;
		if ((status = fork_execv(argc, argv, executableFile)) < 0) {
			fprintf(stderr, "fork_execv error\n");
			continue;
		}
		else if (status == ERR_GRAMMER) {
			// 사용자 지정 문법 에러 문구 처리
			if (strcmp(argv[0], commandSet[0]) == 0) {
				char *args[] = { "help", "tree", NULL };
				if (fork_execv(2, args, "./ssu_help") < 0) {
					fprintf(stderr, "fork_execv error\n");
					continue;
				}
			}
			else if (strcmp(argv[0], commandSet[1]) == 0) {
				char *args[] = { "help", "print", NULL };
				if (fork_execv(2, args, "./ssu_help") < 0) {
					fprintf(stderr, "fork_execv error\n");
					continue;
				}
			}
			else if (strcmp(argv[0], commandSet[2]) == 0) {
				char *args[] = { "help", "help", NULL };
				if (fork_execv(2, args, "./ssu_help") < 0) {
					fprintf(stderr, "fork_execv error\n");
					continue;
				}
			}
			else if (strcmp(argv[0], commandSet[3]) == 0) {
				char *args[] = { "help", "exit", NULL };
				if (fork_execv(2, args, "./ssu_help") < 0) {
					fprintf(stderr, "fork_execv error\n");
					continue;
				}
			}
		}
		printf("\n");
	}

	exit(0);
}
