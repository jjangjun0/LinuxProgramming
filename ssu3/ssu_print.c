#include "ssu_ext2.h"
#include "ssu_iNode.h"
#include "ssu_relative_path_analysis.h"

// mode = 0 (READ file), mode = 1 (READ file & WRITE to [STDOUT_FILENO])
void read_data_block(int fd, uint32_t block_num, int block_size, unsigned int *line, long limit_n);
void read_inode_blocks(int fd, struct ext2_inode *inode, int block_size, unsigned int *line, long limit_n);

int main(int argc, char *argv[])
{
	// <PATH> 저장
	char *relative_path = (char *)malloc(strlen(argv[1]) + 1);
	strcpy(relative_path, argv[1]);

	// ext2_img_name 복원
	char *ext2_img_name = argv[argc - 1];
	argv[argc - 1] = (char *)0;
	argc--;

	if (argc < 2)
		exit(ERR_GRAMMER);


	// 끝에 -n만 오고 "-n"의 인자가 오지 않으면 에러 처리
	for (int i = 0; i < argc; i++) {
		if (strcmp(argv[i], "-n") == 0) {
			if (i <= argc - 2) {
				break;
			}
			else {
				fprintf(stderr, "print: option requires an argument -- 'n'\n");
				exit(1);
			}
		}
	}

	/* -n 인자에 음수가 오는 경우를 방지한다.
		   또한 getopt 함수의 에러를 예방하기도 한다. */
	int isFactor_err = 0;
	long n = 0; // default : 0
	for (int i = 0; i < argc; i++) {
		if (strcmp(argv[i], "-n") == 0 && i <= argc-2) {
			char *endptr;
			n = strtol(argv[i+1], &endptr, 10);
				if (*endptr != '\0') {
					fprintf(stderr, "Error: Invalid factor [%s] entered with -n option\n", endptr);
					isFactor_err = 1; // Flag on
					break;
				}
				else {
					if (n < 0) {
						fprintf(stderr, "Error: The -n option[%ld] is limited to natural numbers\n", n);
						isFactor_err = 1; // Flag on
						break;
					}
				}
		}
	}
	if (isFactor_err == 1)
		exit(ERR_GRAMMER);

	int opt;
	int option_value = NON;
	int n_flag;
	
	// optind = 1;
	// getopt함수 자체 에러 메세지를 출력하지 않는다.
	opterr = 0;
	// 입력한 인자 처리 (getopt함수는 argc >= 2일 때 동작한다.)
	if (argc > 1) {
		while ((opt = getopt(argc, argv, "rspn")) != -1) {
			switch (opt) {
				case 'r':
					option_value += O_R; break;
				case 's':
					option_value += O_S; break;
				case 'p':
					option_value += O_P; break;
				case 'n':
					option_value += O_N; break;
				case '?':
					if (isdigit(optopt))
						continue;
					else {
						fprintf(stderr, "'%c' is not valid option!\n", optopt);
						exit(ERR_GRAMMER);
						continue;
					}
				default:
					continue;
			}
		}
	}
	
	if (option_value != NON && option_value != O_N)
		exit(ERR_GRAMMER);
	option_value /= 1000;
	n_flag = option_value % 10;

	// $DEBUG
	//for (int i = 0; i < argc; i++) {
	//	printf("argv[%i]: %s\n", i, argv[i]);
	//}


	/* print 문법에 맞게 명령줄을 작성했다면, 옵션 이후에 나오는 위치가 <PATH>, 다음 위치가 -n 옵션의 인자이다. */
	int check_path_idx = 0;
	for (int i = 1; i < argc; i++) {
		if (argv[i][0] != '-') {
			check_path_idx = i;
			break;
		}
	}
	if (check_path_idx == 0) {
		fprintf(stderr, "Error: Input <PATH>.\n");
		exit(ERR_GRAMMER);
	}
	if (n_flag == 0 && argc != 2)
		exit(ERR_GRAMMER);
	if (n_flag == 0 && check_path_idx != argc - 1) {
		fprintf(stderr, "Error: Only one path should be inputed.\n");
		exit(ERR_GRAMMER);
	}
	if (n_flag == 1 && check_path_idx != argc - 2) {
		fprintf(stderr, "Error: Only one path and one factor of \"n\" should be inputed.\n");
		exit(ERR_GRAMMER);
	}
	/* parsing 끝, ext2이미지 읽어와서 root inode(2)를 기준으로 루트 디렉토리 구조화 */
	
	int fd;
	int block_size;
	struct ext2_super_block sb;
	struct ext2_group_desc gd;

	if ((fd = open(ext2_img_name, O_RDONLY)) < 0) {
		fprintf(stderr, "open error for %s\n", ext2_img_name);
		exit(1);
	}

	// Super block 읽기 => block #0 + EXT2_SUPERBLOCK_OFFSET(1024)
	if (lseek(fd, EXT2_SUPERBLOCK_OFFSET, SEEK_SET) < 0) {
		fprintf(stderr, "lseek error for EXT2_SUPERBLOCK_OFFSET\n");
		exit(1);
	}
	if (read(fd, &sb, sizeof(sb)) != sizeof(sb)) {
		fprintf(stderr, "read error for super block\n");
		exit(1);
	}

	// block size 구하기
	block_size = 1024 << sb.s_log_block_size; // 1024 * 2^(sb.s_log_block_size)

	// Group Descriptor table 읽기 => block #1
	if (lseek(fd, block_size, SEEK_SET) < 0) {
		fprintf(stderr, "lseek error for block_size\n");
		exit(1);
	}
	if (read(fd, &gd, sizeof(gd)) != sizeof(gd)) {
		fprintf(stderr, "read error for group desc\n");
		exit(1);
	}


	/* Root Directory's structure를 Linked-List로 표현 */
	int root_inode = 2;
	int inode_size = sb.s_inode_size;
	off_t inode_table_offset = gd.bg_inode_table * block_size;
	int root_dir_depth = 0; // 루트는 depth=0
	char *parent = "."; // 루트 상대경로

	struct iNode *head = NULL; // make_dir_structure_with_ext2를 처음으로 호출하려면 head=NULL이 보장되어야 한다.
	make_dir_structure_with_ext2(&head, fd, root_inode, block_size, inode_size, inode_table_offset, parent, root_dir_depth+1);

	/* <PATH> 상대 경로 분석 & 추출 */
	char *interest_path = (char *)malloc(strlen(relative_path) + 1);  // 파일 이름
	int interest_path_depth = 0;
	struct iNode *interest_iNode = NULL;

	// Relative_analysis 호출 (1 => interest_path는 regular file을 기대한다.
	interest_iNode = Relative_analysis(relative_path, 1, &interest_path, &interest_path_depth, head);

	/* --- root(".")에 대해 디렉토리 구조화 완료 --- */
	// 단 <PATH> == "."일 때, "."에 대해서는 linked-list에 포함 X => 따로 생각한다.
	if (strcmp(interest_path, ".") == 0) {
		fprintf(stderr, "Error: '%s' is not file\n", interest_path);
		exit(1);
	}

	// <PATH>에 해당하는 경로가 일반 파일이 아닐 경우 에러
	if (interest_iNode->file_type != 1) {
		fprintf(stderr, "Error: '%s' is not file\n", interest_iNode->name);
		exit(1);
	}

	/* <PATH>에 해당하는 데이터를 읽는다! */
	struct ext2_inode ei;

	// #1. inode_table에서 <PATH>에 해당하는 inode의 정보를 읽는다.
	off_t offset = inode_table_offset + (interest_iNode->inode - 1) * inode_size;
	if (lseek(fd, offset, SEEK_SET) < 0) {
		fprintf(stderr, "lseek error for inode(%d)\n", interest_iNode->inode);
		exit(1);
	}
	if (read(fd, &ei, sizeof(ei)) < 0) {
		fprintf(stderr, "read error for ei\n");
		exit(1);
	}

	// #2. 해당 inode가 사용하는 모든 data block을 순회하며 읽는다.
	unsigned int line = 0;
	if (n_flag) {
		read_inode_blocks(fd, &ei, block_size, &line, n);
	}
	else {
		// n < 0인 경우는 위의 과정에서 이미 처리되었기에
		// 여기까지 도달했다면 n이 음수가 될 수 없다.

		// n_flag가 활성화되지 않은 경우는 모두 출력한다.
		n = -1; // magic number: -1
		read_inode_blocks(fd, &ei, block_size, &line, n);
	}
	// $ DEBUG
	//printf("line: %u\n", line);

	exit(0);
}

void read_data_block(int fd, uint32_t block_num, int block_size, unsigned int *line, long limit_n) {
	unsigned char *buffer = (unsigned char *)malloc(block_size + 1);
	ssize_t length;

	if (buffer == NULL) {
		fprintf(stderr, "malloc error for buffer\n");
		exit(1);
	}

	if (lseek(fd, block_num * block_size, SEEK_SET) < 0) {
		fprintf(stderr, "lseek error for data block\n");
		exit(1);
	}
	if ((length = read(fd, buffer, block_size)) < 0) {
		fprintf(stderr, "read error for data block\n");
		free(buffer);
		exit(1);
	}
	buffer[length] = '\0';

	for (int i = 0; i < length; i++) {
		// magic number: -1
		if (limit_n == -1) {
			putchar(buffer[i]);
			continue;
		}

		if ((*line) <= limit_n - 1)
			putchar(buffer[i]);
		else
			break;

		if (buffer[i] == '\n')
			(*line)++;
	}

	free(buffer);
}
void read_inode_blocks(int fd, struct ext2_inode *inode, int block_size, unsigned int *line, long limit_n) {
	uint32_t *indirect, *dindirect, *tindirect;

	// 1. Direct blocks (0~11)
	for (int i = 0; i < 12; i++) {
		if (inode->i_block[i] == 0)
			break;
		read_data_block(fd, inode->i_block[i], block_size, line, limit_n);
	}

	// 2. Single Indirect block (12)
	if (inode->i_block[12]) {
		indirect = (uint32_t *)malloc(block_size);
		if (lseek(fd, inode->i_block[12] * block_size, SEEK_SET) < 0) exit(12);
		if (read(fd, indirect, block_size) < 0) exit(12);

		for (int i = 0; i < block_size / 4; i++) {
			if (indirect[i] == 0)
				break;

			read_data_block(fd, indirect[i], block_size, line, limit_n);
		}
		free(indirect);
	}

	// 3. Double Indirect block (13)
	if (inode->i_block[13]) {
		dindirect = (uint32_t *)malloc(block_size);
		if (lseek(fd, inode->i_block[13] * block_size, SEEK_SET) < 0) exit(13);
		if (read(fd, dindirect, block_size) < 0) exit(13);

		for (int i = 0; i < block_size / 4; i++) {
			if (dindirect[i] == 0)
				break;
			indirect = (uint32_t *)malloc(block_size);
			if (lseek(fd, dindirect[i] * block_size, SEEK_SET) < 0) exit(12);
			if (read(fd, indirect, block_size) < 0) exit(12);

			for (int j = 0; i < block_size / 4; j++) {
				if (indirect[j] == 0)
					break;

				read_data_block(fd, indirect[j], block_size, line, limit_n);
			}
			free(indirect);
		}
		free(dindirect);
	}

	// 4. Triple Indirect block (14)
	if (inode->i_block[14]) {
		tindirect = (uint32_t *)malloc(block_size);
		if (lseek(fd, inode->i_block[14] * block_size, SEEK_SET) < 0) exit(14);
		if (read(fd, tindirect, block_size) < 0) exit(14);

		for (int i = 0; i < block_size / 4; i++) {
			if (tindirect[i] == 0)
				break;

			dindirect = (uint32_t *)malloc(block_size);
			if (lseek(fd, tindirect[i] * block_size, SEEK_SET) < 0) exit(13);
			if (read(fd, dindirect, block_size) < 0) exit(13);

			for (int j = 0; j < block_size / 4; j++) {
				if (dindirect[j] == 0)
					break;
				indirect = (uint32_t *)malloc(block_size);
				if (lseek(fd, dindirect[j] * block_size, SEEK_SET) < 0) exit(12);
				if (read(fd, indirect, block_size) < 0) exit(12);

				for (int k = 0; k < block_size / 4; k++) {
					if (indirect[k] == 0)
						break;

					read_data_block(fd, indirect[k], block_size, line, limit_n);
				}
				free(indirect);
			}
			free(dindirect);
		}
		free(tindirect);
	}
}
