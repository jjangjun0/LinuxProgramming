#include "ssu_ext2.h"
#include "ssu_iNode.h"
#include "ssu_relative_path_analysis.h"

int print_tree(struct iNode *head, int p_flag, int s_flag, long *d_count, long *f_count);

int main(int argc, char *argv[])
{
	int opt;
	int option_value = NON;
	int r_flag, s_flag, p_flag;

	char *relative_path;
	char *ext2_img_name = argv[argc - 1];

	// <PATH> 저장
	relative_path = (char *)malloc(strlen(argv[1]) + 1);
	strcpy(relative_path, argv[1]);

	// ext2_img_name 복원
	argv[argc - 1] = (char *)0;
	argc--;

	//printf("Success! => %s\n", ext2_img_name);
	//printf("relative_path : %s\n", relative_path);

	if (argc < 2)
		exit(ERR_GRAMMER);

	// optind = 1; // 처리되는 인자의 숫자 초기화
	// (getopt함수를 특정 process에서 여러 번 호출하게 되면 optind값은 초기화되지 않기에 예기치 않은 결과가 발생한다.
	opterr = 0; // getopt함수 자체 에러 메세지를 출력하지 않는다.
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
						//fprintf(stderr, "%c is not valid option!\n", optopt);
						exit(ERR_GRAMMER);
						continue;
					}
				default:
					continue;
			}
		}
	}
	
	if (option_value != NON &&
			option_value != O_R && option_value != O_S && option_value != O_P &&
			option_value != (O_R + O_S) && option_value != (O_R + O_P) && option_value != (O_S + O_P) &&
			option_value != (O_R + O_S + O_P)) {
		exit(ERR_GRAMMER); // option 에러 시 입력받은 명령을 수행하지 않는다.
	}
	r_flag = option_value % 10;
	option_value /= 10;
	s_flag = option_value % 10;
	option_value /= 10;
	p_flag = option_value % 10;
	//printf("r_flag, s_flag, p_flag: %d %d %d\n", r_flag, s_flag, p_flag);

	/* tree 문법에 맞게 명령줄을 작성했다면, 옵션 이후에 나오는 위치가 끝이어야 한다. */
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
	if (check_path_idx != argc - 1) {
		fprintf(stderr, "Error: Only one path should be inputed.\n");
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

	//printf("root excluded head list\n");
	//printINodeList(head);

	/* <PATH> 상대 경로 분석 & 추출 */
	char *interest_path = (char *)malloc(strlen(relative_path) + 1);  // 파일 이름
	int interest_path_depth = 0;
	struct iNode *interest_iNode = NULL;
	
	// Relative_analysis 호출 (2 => interest_path는 directory 파일을 기대한다.)
	interest_iNode = Relative_analysis(relative_path, 2, &interest_path, &interest_path_depth, head);
	
	
	// $ DEBUG
	//printf("interest_path: %s\n", interest_path);
	//printf("interest_path_depth: %d\n", interest_path_depth);
	
	/* <PATH>에 대해 디렉토리 구조화 (<PATH>와 연관 없다면 list에서 삭제한다) */
	struct iNode *temp = NULL;

	if (strcmp(interest_path, ".") != 0) {
		// #1. interest_path를 찾으면서 interest_path가 아니면 삭제한다.
		// <PATH> = "."이 아닌 경우 interest_iNode != NULL을 보장한다.
		temp = head;
		while (temp != NULL) {
			// interest_path를 찾은 경우 종료
			if (temp->inode == interest_iNode->inode &&
					temp->dir_depth == interest_iNode->dir_depth &&
					temp->file_size == interest_iNode->file_size &&
					temp->file_type == interest_iNode->file_type &&
					strcmp(temp->name, interest_iNode->name) == 0 &&
					strcmp(temp->parent, interest_iNode->parent) == 0) {
				break;
			}
			deleteINode(&head, temp->inode, temp->name, temp->parent, temp->dir_depth);
			temp = temp->next;
		}

		// #2. 그 지점부터 시작하는 것이기에, dir_depth가 같아지거나 작아지는 경우부터 모두 삭제한다.
		// make_dir_structure_with_ext2 함수를 호출하여 만든 linked-list 특징을 활용한다.
		int interest_dir_depth = temp->dir_depth;
		int is_delete_iNode = 0;

		temp = temp->next; // interest_path 다음의 iNode부터 검사한다.
		while (temp != NULL) {
			if (temp->dir_depth <= interest_dir_depth)
				is_delete_iNode = 1;

			// iNode 삭제
			if (is_delete_iNode)
				deleteINode(&head, temp->inode, temp->name, temp->parent, temp->dir_depth);
			temp = temp->next;
		}
	}
	// $ DEBUG
	//printf("root 디렉토리를 구조화한 최종:\n");
	//printINodeList(head);

	/* --- "tree <PATH>"를 입력받을 때, <PATH>에 대해 디렉토리 구조화 완료 --- */
	// 단 <PATH> == "."일 때, "."에 대해서는 linked-list에 포함 X => 따로 출력할 때 입력받는다.
	

	/* -r 옵션 처리 (-s, -p는 출력할 때 처리) */
	if (r_flag) { }
	else {
		int interest_dir_depth;

		temp = head;
		interest_dir_depth = temp->dir_depth;

		if (strcmp(interest_path, ".") == 0)
			interest_dir_depth = root_dir_depth;

		while (temp != NULL) {
			if (temp->dir_depth != interest_dir_depth &&
					temp->dir_depth != (interest_dir_depth + 1)) {
				deleteINode(&head, temp->inode, temp->name, temp->parent, temp->dir_depth);
			}
			temp = temp->next;
		}
		// $ DEBUG
		//printf("-r 옵션이 없는 경우:\n");
		//printINodeList(head);
	}


	/* tree 구조로 출력 */
	long int d_count = 1, f_count = 0;
	// #1. <PATH> 출력
	if (strcmp(interest_path, ".") == 0) {
		off_t root_offset = inode_table_offset + (root_inode - 1) * inode_size;
		struct ext2_inode root_ext2;
		if (lseek(fd, root_offset, SEEK_SET) < 0) {
			fprintf(stderr, "lseek error for root_inode\n");
			exit(1);
		}
		if (read(fd, &root_ext2, sizeof(root_ext2)) != sizeof(root_ext2)) {
			fprintf(stderr, "read error for root_ext2\n");
			exit(1);
		}

		// root_ext2.i_size // 파일 크기
		__u8 root_file_type; // root 파일 유형
		switch (root_ext2.i_mode & 0xF000) {
			case 0x8000: root_file_type = 1; break; // regular file
			case 0x4000: root_file_type = 2; break; // directory
			case 0x2000: root_file_type = 3; break; // character device
			case 0x6000: root_file_type = 4; break; // block device
			case 0x1000: root_file_type = 5; break; // FIFO
			case 0xC000: root_file_type = 6; break; // socket
			case 0xA000: root_file_type = 7; break; // symbolic link
			default: root_file_type = 0; break;     // unknown
		}
		char *root_permissions = make_file_authority(root_ext2.i_mode); // root 파일 접근 권한

		if (p_flag || s_flag)
			printf("[");

		if (p_flag) {
			print_file_type(root_file_type);
			printf("%s", root_permissions);
			free(root_permissions);
		}
		if (p_flag && s_flag)
			printf(" ");
		if (s_flag) {
			printf("%d", root_ext2.i_size);
		}

		if (p_flag || s_flag)
			printf("] ");
		printf("%s\n", relative_path);
	}
	else {
		temp = head;
		if (p_flag || s_flag)
			printf("[");

		if (p_flag) {
			print_file_type(temp->file_type);
			printf("%s", temp->file_authority);
		}
		if (p_flag && s_flag)
			printf(" ");
		if (s_flag) {
			printf("%d", temp->file_size);
		}

		if (p_flag || s_flag)
			printf("] ");
		printf("%s\n", relative_path);
	}

	// #2. interest_path를 제외한 나머지 디렉토리 구조를 출력한다.
	if (strcmp(interest_path, ".") == 0) {
		print_tree(head, p_flag, s_flag, &d_count, &f_count);
	}
	else {
		temp = head;
		// 빈 디렉토리의 경우, next iNode가 없다.
		if (temp->next != NULL)
			print_tree(temp->next, p_flag, s_flag, &d_count, &f_count);
	}
	// #3. directory 개수, regular file 개수 출력
	printf("\n%ld directories, %ld files\n", d_count, f_count);
	
	freeINodeList(&head);
	freeINodeList(&interest_iNode);
	free(relative_path);
	close(fd);
	exit(0);
}

int print_tree(struct iNode *head, int p_flag, int s_flag, long *d_count, long *f_count) {
	int num = 0;                     // node 개수
	int depth_min = head->dir_depth; // dir_depth 최소값
	int depth_max = 0;               // dir_depth 최대값
	struct iNode *temp = NULL;

	// 1. depth_max, depth_min 구하기 & node 개수도 구하기
	temp = head;
	while (temp != NULL) {
		if (temp->dir_depth < depth_min)
			depth_min = temp->dir_depth;

		if (temp->dir_depth > depth_max)
			depth_max = temp->dir_depth;
		num++;
		temp = temp->next;
	}

	// depth_min을 통해 각 node의 dir_depth를 재구성 해야한다.
	// <PATH>의 dir_depth = 0이니, 최소값을 1로 맞춘다.
	// 당연히 print_tree는 root 이후의 리스트가 오기 때문에 depth_min-1이 음수가 될 수 없다.
	// ex) depth_max = 3, depth_min = 2 => 해당 list의 depth_max=2, depth_min=1이 된다.
	depth_min -= 1;
	temp = head;
	while (temp != NULL) {
		temp->dir_depth -= depth_min;
		temp = temp->next;
	}
	depth_max -= depth_min;
	depth_min = 1;

	// 2. 특정 위치의 iNode 앞의 노드와 dir_depth를 비교하며 print_rule 배열을 초기화한다.
	// print_rule 배열이 최종적으로 다음의 값을 갖도록 한다. { 3 => ㅏ, 2 => ㄴ, 1 => ㅣ, 0 => "    " }

	int depth_flags[depth_max];
	int print_rule[num][depth_max];
	int idx = 0;
	int prev_depth = depth_min-1;

	for (int i = 0; i < depth_max; i++) depth_flags[i] = 0;

	temp = head;
	while (temp != NULL) {
		if (prev_depth < temp->dir_depth) {
			depth_flags[temp->dir_depth - 1] = 1; // ON
		}
		
		if (prev_depth > temp->dir_depth) {
			for (int i = depth_max - 1; i > temp->dir_depth - 1; i--) {
				depth_flags[i] = 0; // OFF
			}
		}
		// depth에 따른 print_rule 배열 만들기
		// "prev_depth가 작아지기 전까지는 depth_flags 배열이 바뀌지 않아 그대로 저장된다."
		for (int j = 0; j < depth_max; j++) {
			print_rule[idx][j] = depth_flags[j];
		}

		idx++;
		prev_depth = temp->dir_depth;
		temp = temp->next;
	}

	// $ DEBUG
	// 첫 번째 print_rule 에서 (0 => 출력X, 1 => 출력O) 의미한다.
	//printf("\nFirst print_rule:\n");
	//for (int i = 0; i < num; i++) {
	//	for (int j = 0; j < depth_max; j++) {
	//		printf("%d ", print_rule[i][j]);
	//	}
	//	printf("\n");
	//}

	// 3. dir_start_end 배열을 구한다. 이를 활용해서 print_rule 배열을 수정한다.
	int start = -1, end = -1;
	int d_idx;
	int search_depth = 1; // root가 없으니 dir_depth=1부터 검사한다.

	int dir_start_end[depth_max][num * 2]; // 각 depth마다 어디서 시작하고 끝나는지 쌍으로 순차적이게 저장하는 배열
	// worst_case: 1 1 2 2 3 3 ... num num 이더라도 num*2보다 클 수 없다.

	while (1) {
		idx = 0;
		d_idx = 0;
		temp = head; // temp 초기화
		while (temp != NULL) {
			// #1. start가 정해지지 않은 경우
			if (start == -1 && temp->dir_depth == search_depth) {
				start = idx;
			}
			// #2. start가 정해졌는데, 똑같은 depth가 나온 경우
			if (start != -1 && temp->dir_depth == search_depth) {
				end = idx;
			}
			// #3. start가 정해졌는데, node의 끝에 도달한 경우 OR start보다 depth가 작아진 경우
			if (start != -1 && (idx == (num - 1) || temp->dir_depth < search_depth)) {
				dir_start_end[search_depth - 1][d_idx++] = start;
				dir_start_end[search_depth - 1][d_idx++] = end;
				start = end = -1; // start, end 초기화
			}

			temp = temp->next;
			idx++;
			if (idx == num) {
				dir_start_end[search_depth - 1][d_idx] = -1; // '-1'은 null 역할을 한다.
				break;
			}
		}
		search_depth++;
		if (search_depth > depth_max)
			break;
	}

	// $ DEBUG
	//printf("\ndir_start_end:\n");
	//for (int i = 0; i < depth_max; i++) {
	//	printf("depth(%d): ", i+1);
	//	for (int j = 0; j < num; j++) {
	//		if (dir_start_end[i][j] == -1)
	//			break;
	//		printf("%d ", dir_start_end[i][j]);
	//	}
	//	printf("\n");
	//}

	// 4. dir_start_end 배열을 순회하며 시작과 끝이 아닌 지점은 0으로 바꾼다.
	start = end = 0;
	int newstart;
	for (int depth = 0; depth < depth_max; depth++) {
		for (int j = 0; j < num; j+= 2) {
			newstart = -1; // newstart 초기화

			if (dir_start_end[depth][j] == -1)
				break;

			start = dir_start_end[depth][j];
			end = dir_start_end[depth][j + 1];
			// 새로운 start가 있는지 확인
			if (dir_start_end[depth][j + 2] != -1) {
				newstart = dir_start_end[depth][j + 2];
			}

			// start와 end를 가지고 print_rule 수정
			for (int k = 0; k < num; k++) {
				if (k < start)
					continue;
				else if (k >= start && k <= end)
					print_rule[k][depth] = 1;

				if (k == end) {
					print_rule[k][depth] = 2;
				}
				else if (k > end) {
					if (newstart != -1 && k == newstart)
						break;
					print_rule[k][depth] = 0;
				}
			}
			// k = newstart여서 break; 되었다면 다음 start, end를 탐색하고 수행한다.
		}
	}

	// $ DEBUG
	//printf("\nSecond print_rule:\n");
	//for (int i = 0; i < num; i++) {
	//	for (int j = 0; j < depth_max; j++) {
	//		printf("%d ", print_rule[i][j]);
	//	}
	//	printf("\n");
	//}

	// 5. 위의 규칙에 해당하지 않는다면 'ㅏ' 가 출력되어야 한다. 
	int temp_n;
	for (int i = 0; i < num; i++) {
		temp_n = -1;
		for (int j = 0; j < depth_max; j++) {
			if (print_rule[i][j] == 1) {
				temp_n = j;
			}
			if (print_rule[i][j] == 2) {
				temp_n = -1;
				break;
			}
		}
		if (temp_n == -1)
			continue; // 1이 없는 경우
		else
			print_rule[i][temp_n] = 3; // 각 디렉토리에서 끝의 값이 아니면 3으로 변경
	}

	// $ DEBUG
	//printf("\nFinal print_rule:\n");
	//for (int i = 0; i < num; i++) {
	//	for (int j = 0; j < depth_max; j++) {
	//		printf("%d ", print_rule[i][j]);
	//	}
	//	printf("\n");
	//}

	// 6. print_rule 배열을 활용해서 head가 가리키는 list를 출력 
	idx = 0;
	temp = head;
	while (temp != NULL) {
		// directory, regular file 개수 세기
		if (temp->file_type == 1) (*f_count)++;
		else if (temp->file_type == 2) (*d_count)++;

		// #1. print_rule에 따라 앞부분의 출력을 결정한다.
		for (int j = 0; j < depth_max; j++) {
			if (j == temp->dir_depth)
				break;

			// \u251c: ㅏ, \u2514: ㄴ, \u2500: ㅡ, \u2502: ㅣ
			if (print_rule[idx][j] == 3)
				printf("\u251c\u2500  ");
			else if (print_rule[idx][j] == 2)
				printf("\u2514\u2500  ");
			else if (print_rule[idx][j] == 1)
				printf("\u2502   ");
			else if (print_rule[idx][j] == 0)
				printf("    ");
		}

		// #2. option에 따라 파일 권한 또는 파일 크기를 출력할 지 결정한다.
		if (p_flag || s_flag)
			printf("[");

		if (p_flag) {
			print_file_type(temp->file_type);
			printf("%s", temp->file_authority);
		}
		if (p_flag && s_flag)
			printf(" ");
		if (s_flag) {
			printf("%d", temp->file_size);
		}

		if (p_flag || s_flag)
			printf("] ");

		// #3. 파일 이름을 출력한다.
		printf("%s\n", temp->name);

		idx++;
		temp = temp->next;
	}
	return 0;
}
