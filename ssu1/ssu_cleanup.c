#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>

// 시스템 규정
#define STRING_MAX 4096
#define PATH_MAX 4096
#define FACTOR_MAX 4096
#define NAME_MAX 255
#define BUFFER_SIZE 1024
#define S_MODE 0644


// Command type
#define TREE     10000000
#define ARRANGE  20000000
#define HELP     30000000
#define EXIT     40000000
// Option type
#define NON      0
#define O_S      1
#define O_P     10
#define O_SP    11
#define O_D    100
#define O_T   1000
#define O_X  10000
#define O_E 100000
// Help type
#define H_TREE    1
#define H_ARRANGE 2
#define H_HELP    3
#define H_EXIT    4
#define H_ALL  1234
// Flag
#define F_ON  1
#define F_OFF 0

// 명령어 타입
char *commandSet[5] = {
	"tree",
	"arrange",
	"help",
	"exit"
};
// 전역변수 flag
int tree_stop_flag;

int Prompt();
void get_factor(int *argc, char *argv[], char *str);
char *make_str(char *str, int start, int end);
int parsing(int argc, char *argv[]);

int Tree(int argc, char *argv[], int tree_option);
int Arrange(int argc, char *argv[], int arrange_option, int *option_factor_num);
void Help(int print_type, int repeat_call);
void Exit();

int CheckOnlyPath(int argc, char *argv[]);
int CheckPathAccess_fixWD(char *path, char *absolute_root);
int CheckPathType_fixWD(char *path, char *absolute_root);
int CheckPath_InsideHome(const char *path);

int CheckPathAccess(char *path);
int CheckPathType(char *path);


// node struct define
typedef struct dNode {
	/* data */
	char* name; // 파일 혹은 디렉토리 이름
	char* parentName; // 부모 디렉토리 이름
	char* parent_real_path; // 부모 디렉토리의 절대 경로

	int current_type; // regular file: 1, directory: 2
	int dir_depth;  // 파일 혹은 디렉토리의 깊이

	/* next node pointer */
	struct dNode* next;
} dNode;
/* linked list 구조를 위한 Node 관련 함수 */
dNode* createNode(const char* name, const char* parentName, const char* parent_real_path, int current_type, int dir_depth);
void appendNode(dNode** head, char* name, char* parentName, char* parent_real_path, int current_type, int dir_depth);
void deleteNode(dNode** head, char* name, char* parentName, int current_type, int dir_depth);
void printList(dNode* head);
void freeList(dNode* head);

// 새로운 노드 생성 함수
dNode* createNode(const char* name, const char* parentName, const char* parent_real_path, int current_type, int dir_depth) {
	dNode* new_dNode = (dNode*)malloc(sizeof(dNode));
	if (new_dNode == NULL) {
		fprintf(stderr, "Memory allocation failure\n");
	exit(1);
	}

	/* data 복사 */
	new_dNode->name = strdup(name);
	new_dNode->parentName = strdup(parentName);
	new_dNode->parent_real_path = strdup(parent_real_path);
	new_dNode->current_type = current_type;
	new_dNode->dir_depth = dir_depth;

	/* next pointer 지정 */
	new_dNode->next = NULL;

	return new_dNode;
}

// list에 node 추가
void appendNode(dNode** head, char* name, char* parentName, char* parent_real_path, int current_type, int dir_depth) {
	dNode* new_dNode = createNode(name, parentName, parent_real_path, current_type, dir_depth);
	if (*head == NULL) {
		*head = new_dNode;
	return;
	}
	dNode* temp = *head;
	while (temp->next != NULL) {
		temp = temp->next;
	}
	temp->next = new_dNode;
}

// list에서 node 삭제
void deleteNode(dNode** head, char* name, char* parentName, int current_type, int dir_depth) {
	dNode* temp = *head;
	dNode* prev = NULL;

	// 삭제할 노드가 첫 번째 노드일 경우
	if (temp != NULL &&
		strcmp(temp->name, name) == 0 &&
		strcmp(temp->parentName, parentName) == 0 &&
		temp->current_type == current_type &&
		temp->dir_depth == dir_depth) {
		*head = temp->next;
		free(temp->name);
		free(temp->parentName);
		free(temp);
		return;
	}

	while (temp != NULL) {
		if (strcmp(temp->name, name) == 0 &&
			strcmp(temp->parentName, parentName) == 0 &&
			temp->current_type == current_type &&
			temp->dir_depth == dir_depth) {
			break;
		}
		prev = temp;
		temp = temp->next;
	}

	if (temp == NULL) return;

	prev->next = temp->next;
	free(temp->name);
	free(temp->parentName);
	free(temp);
}

// list 출력 (디버깅 용도)
void printList(dNode* head) {
	dNode* temp = head;
	while (temp != NULL) {
		if (temp->dir_depth != 0 && temp->current_type == 1) printf("    ");
		printf("%s", temp->name);
		if (temp->dir_depth != 0 && temp->current_type == 2) printf("/");
		printf("  : ");
		//printf("parentName: %s | dir_depth(%d)\n", temp->parentName, temp->dir_depth);
		
		printf("parentName -> %s | parent_real_path -> %s", temp->parentName, temp->parent_real_path);
		printf("\n");
		//printf(" | type: (%d), depth: (%d)\n", temp->current_type, temp->dir_depth);
		temp = temp->next;
	}
}

// 구조체 linked list 메모리 전체 해제
void freeList(dNode* head) {
	dNode* temp;
	while (head != NULL) {
		temp = head;
		head = head->next;
		free(temp->name);
		free(temp->parentName);
		free(temp);
	}
}

/* 프로그램 동작 함수 */
int filter(const struct dirent *entry);

char *make_str(char *str, int start, int end) {
	//printf("    *make_str() start!\n");

	char *res = (char*)malloc(sizeof(char) * (end-start + 2));

	for (int i = start; i <= end; i++) {
		res[i-start] = str[i];
	}
	res[end-start+1] = '\0';

		//printf("res 출력");
		//for (int i = 0; i <= end-start; i++) printf("|%c| ", res[i]);
		//printf("\n");

	return res;
}

void get_factor(int *argc, char *argv[], char *str) {
	//printf("    get_factor() start!\n");

	/* argc에 인자 개수 저장
	   argv[]에는 인자 하나하나에 대응하는 포인터 저장
	   str는 명령줄이 저장되어 있는 문자열 */

	int start_idx = -1;
	int end_idx = -1;

	for (int i = 0; i <= strlen(str); i++) {
		if (str[i] != ' ' && str[i] != '\0') {
			if (start_idx == -1) {
				start_idx = i; // 인자의 시작 위치 저장
			}
		}
		else if (start_idx != -1) { // 인자의 시작이 지정된 경우
			// 인자의 끝 지점 찾기
			for (int j = start_idx; j <= strlen(str); j++) {
				if (str[j] == ' ' || str[j] == '\0') { // tab은 '\t'로 문자로 취급한다.
					end_idx = j-1;
					
					//printf("start(%d) : %c\t end(%d) : %c\n", start_idx, str[start_idx], end_idx, str[end_idx]);
					argv[*argc] = make_str(str, start_idx, end_idx);
					(*argc)++; // 인자의 개수 증가
					
					i = j; // i 동기화
					start_idx = -1; // start_idx 초기화
					break;
				}
			}
		}

	}
	argv[*argc] = (char*)0;
}
int parsing(int argc, char *argv[]) {
	// 명령어 타입과 옵션을 반환해주는 함수
	// printf("    parsing() start!\n");
	// argc : 인자 개수, argv[] : 인자
	int command_exist = 0;
	int command_type = 0;
	int command_option = 0;
	int opt;

	opterr = 0; // error 메세지는 출력하지 않는다.

	// command 처리
	if ((strcmp(argv[0], commandSet[0])) == 0) command_type = TREE;
	if ((strcmp(argv[0], commandSet[1])) == 0) command_type = ARRANGE;
	if ((strcmp(argv[0], commandSet[2])) == 0) command_type = HELP;
	if ((strcmp(argv[0], commandSet[3])) == 0) command_type = EXIT;


	if (command_type != 0) command_exist = 1;
	if (!command_exist) return -1; // 첫 번째 인자가 등록된 command가 아니라면
	
	//printf("getopt()전 argv[] : ");
	//for (int i = 0; i < argc; i++) printf("%s ", argv[i]);
	//printf("\n");
	if (argc > 1) { 			// option 처리 : getopt는 최소 argc = 2일 때 동작한다.
		while ((opt = getopt(argc, argv, "spdtxe")) != -1) {
			switch (opt) {
				case 's': command_option += O_S; break;
				case 'p': command_option += O_P; break;
				case 'd': command_option += O_D; break;
				case 't': command_option += O_T; break;
				case 'x': command_option += O_X; break;
				case 'e': command_option += O_E; break;
				case '?':
					fprintf(stderr, "ERROR : parsing() %c is not appointed option\n", optopt);
					return command_type * (-1); // 등록된 option이 아니라면 command type만 리턴
				default: break;
			}
		}
	}
	//printf("command_type : %d | command_option : %d\n", command_type, command_option);

	return command_type + command_option;
}
int CheckOnlyPath(int argc, char *argv[]) {
	int notOptions = 0;
	int idx = 0;
	for (int i = 1; i < argc; i++) {
		// argv[0]은 command
		if (strncmp(argv[i], "-", 1) == 0) {
			continue; // option
		} else {
			idx = i; // path가 들어가 있는 index 저장
			notOptions++;
		}
	}
	if (notOptions > 1) return -1;
	return idx; // 오직 하나의 path라면, path의 index 반환
}
int CheckPathAccess(char *path) {
	int type = 0;
	if (access(path, F_OK) < 0) return -1; // 상대경로로 path의 접근성을 판단한다.

	if (access(path, R_OK) == 0) type += 4;
	if (access(path, W_OK) == 0) type += 2;
	if (access(path, X_OK) == 0) type += 1;
	return type;
}

int CheckPathAccess_fixWD(char *path, char *absolute_root) {
	/*  R_OK : 읽기 권한 판정
        W_OK : 쓰기 권한 판정
        X_OK : 실행 권한 판정
        F_OK : 파일 존재 여부 판정  */
	int type = 0;
	char real_path[PATH_MAX];

	// 입력받은 path를 절대경로로 전환
	if (!realpath(path, real_path)) {
		perror("realpath");
		return -1;
	}
	//printf("real_path : %s\nabsolute_root : %s\n", real_path, absolute_root);
	// 사용자에게 입력받은 절대경로에서 접근할 수 있는지, root에서 path가 파생되었는지 확인해야한다.	
	if (strncmp(real_path, absolute_root, strlen(absolute_root)) != 0) {
		fprintf(stderr, "GRAMMER ERROR : 해당 파일은 <%s>경로 밑에 존재하지 않는다.\n", absolute_root);
		fprintf(stderr, "  > Check what yourself inputed in root\n");
		return -1;
	}
	if (access(real_path, F_OK) < 0) // 절대경로로 path의 접근성을 판단한다.
		return -1;

	if (access(path, R_OK) == 0) type += 4;
	if (access(path, W_OK) == 0) type += 2;
	if (access(path, X_OK) == 0) type += 1;

	return type;
}
int CheckPathType(char *path) {
	struct stat statbuf;
	int p_type;
	
	if (lstat(path, &statbuf) < 0) { // 상대경로로 path의 유형을 파악한다.
		fprintf(stderr, "lstat error\n");
		return -127;
	}

	if      (S_ISREG(statbuf.st_mode))  p_type = 1;
	else if (S_ISDIR(statbuf.st_mode))  p_type = 2;
	else if (S_ISCHR(statbuf.st_mode))  p_type = 3;
	else if (S_ISBLK(statbuf.st_mode))  p_type = 4;
	else if (S_ISFIFO(statbuf.st_mode)) p_type = 5;
	else if (S_ISSOCK(statbuf.st_mode)) p_type = 6;
	else if (S_ISLNK(statbuf.st_mode))  p_type = 7;
	else p_type = -1; // unknown

	return p_type;
}
int CheckPathType_fixWD(char *path, char *absolute_root) {
	/*  File_type | Description
		0         : Unknown
		1         : Regular file
		2         : Directory
		3         : Character device
		4         : Block device
		5         : Named pipe
		6         : Socket
		7         : Symbolic link  */
	struct stat statbuf;
	int p_type;
	char real_path[PATH_MAX];
	
	// 입력받은 path를 절대경로로 전환
	if (!realpath(path, real_path)) {
		perror("realpath");
		return -1;
	}
	// 사용자에게 입력받은 절대경로에서 접근할 수 있는지, root에서 path가 파생되었는지 확인해야한다.	
	if (strncmp(real_path, absolute_root, strlen(absolute_root)) != 0) {
		fprintf(stderr, "GRAMMER ERROR : 해당 파일은 <%s>경로 밑에 존재하지 않는다.\n", absolute_root);
		fprintf(stderr, "  > Check what yourself inputed in root\n");
		return -1;
	}
	if (lstat(real_path, &statbuf) < 0) { // 절대경로로 path의 유형을 파악한다.
		fprintf(stderr, "lstat error\n");
		return -127;
	}

	if      (S_ISREG(statbuf.st_mode))  p_type = 1;
	else if (S_ISDIR(statbuf.st_mode))  p_type = 2;
	else if (S_ISCHR(statbuf.st_mode))  p_type = 3;
	else if (S_ISBLK(statbuf.st_mode))  p_type = 4;
	else if (S_ISFIFO(statbuf.st_mode)) p_type = 5;
	else if (S_ISSOCK(statbuf.st_mode)) p_type = 6;
	else if (S_ISLNK(statbuf.st_mode))  p_type = 7;
	else p_type = -1; // unknown

	return p_type;
}
int CheckPath_InsideHome(const char *path) {
	char *home = getenv("HOME");
	char real_path[PATH_MAX];
	
	if (!home) {
		fprintf(stderr, "HOME's environmental variable couldn't be found.\n");
		return -1;
	}
	if (!realpath(path, real_path)) {
		perror("realpath");
		return -1;
	}

	if (strncmp(real_path, home, strlen(home)) == 0) return 1;
	return 0;
}

int filter(const struct dirent *entry) {
	if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
		return 0; // 자신과, 부모는 포함하지 않도록 필터를 설정
	return 1;
}
int check_file_type(unsigned char d_type) {
	int type = 0;
	switch(d_type) {
		case DT_REG:
		//	printf("Regular file");
			type = 1;  break;   // Regular file : 1
		case DT_DIR:
		//	printf("Directory");
			type = 2;  break;   // Directory : 2
		case DT_CHR:
		//	printf("Character device");
			type = 3;  break;   // Character device : 3
		case DT_BLK:
		//	printf("Block device");
			type = 4;  break;   // Block device : 4
		case DT_FIFO:
		//	printf("FIFO");
			type = 5;  break;   // Named pipe : 5
		case DT_SOCK:
		//	printf("Socket");
			type = 6;  break;   // Socket : 6
		case DT_LNK:
		//	printf("Symbolic link");
			type = 7;  break;   // Symbolic link : 7
		default:
		//	printf("Unknown file");
			type = -1; break;   // Unknown : -1
	}
	return type;
}

int print_size(char *path) {
	struct stat statbuf;

	if ((stat(path, &statbuf)) < 0) {
		fprintf(stderr, "stat error\n");
		return -1;
	}
	printf("%ld", statbuf.st_size);
	return 0;
}
int print_authority(char *path) {
	struct stat statbuf;

	if ((stat(path, &statbuf)) < 0) {
		fprintf(stderr, "stat error\n");
		return -1;
	}

	char permissions[10];
	// Owner
	permissions[0] = (statbuf.st_mode & S_IRUSR) ? 'r' : '-';
	permissions[1] = (statbuf.st_mode & S_IWUSR) ? 'w' : '-';
	permissions[2] = (statbuf.st_mode & S_IXUSR) ? 'x' : '-';
	// Group
	permissions[3] = (statbuf.st_mode & S_IRGRP) ? 'r' : '-';
	permissions[4] = (statbuf.st_mode & S_IWGRP) ? 'w' : '-';
	permissions[5] = (statbuf.st_mode & S_IXGRP) ? 'x' : '-';
	// Others
	permissions[6] = (statbuf.st_mode & S_IROTH) ? 'r' : '-';
	permissions[7] = (statbuf.st_mode & S_IWOTH) ? 'w' : '-';
	permissions[8] = (statbuf.st_mode & S_IXOTH) ? 'x' : '-';
	// 끝에 null
	permissions[9] = '\0';
	
	switch (CheckPathType(path)) {
		case 1: printf("-"); break;
		case 2: printf("d"); break;
		case 3: printf("c"); break;
		case 4: printf("b"); break;
		case 5: printf("p"); break;
		case 6: printf("s"); break;
		case 7: printf("l"); break;
		default: printf("?"); break;
	}
	
	printf("%s", permissions);
	return 0;
}
int print_treeList(dNode* head, int tree_option, int *dir_count, int *f_count) {
	int prev_depth = 0;
	int depth_max = 0;

	// depth_max 구하기
	dNode* scan = head;
	int num = 1; // 실제 node 개수
	while (scan != NULL) {
		if (scan->dir_depth > depth_max) {
			depth_max = scan->dir_depth;
		}
		scan = scan->next;
		num++;
	}

	num--; // 실제 node 개수보다 값이 +1 크게 저장되어 있음
	//printf("num: %d, depth_max: %d\n", num, depth_max);

	int depth_flags[depth_max]; // flag 저장
	for (int i = 0; i < depth_max; i++) depth_flags[i] = 0;
	int print_rule[num][depth_max]; // 각 노드마다 어떻게 출력해야 하는지 문자로 저장

	dNode* temp = head;
	int idx = 0;
	while (temp != NULL) {
		if (prev_depth < temp->dir_depth) {
			depth_flags[temp->dir_depth-1] = 1; // ON
		}
		if (prev_depth > temp->dir_depth) {
			for (int i = depth_max-1; i > temp->dir_depth-1; i--) {
				depth_flags[i] = 0; // OFF
			}
		}
		// 1. depth에 따른 print_rule 배열 만들기
		for (int j = 0; j < depth_max; j++) {
			print_rule[idx][j] = depth_flags[j]; // depth_flags 복사
		}

		idx++;
		prev_depth = temp->dir_depth;
		temp = temp->next;
	}
	/*
	printf("First print_rule: \n");
	for (int i = 0; i < num; i++) {
		for (int j = 0; j < depth_max; j++) {
			printf("%d ", print_rule[i][j]);
		}
		printf("\n");
	}
	printf("\n");
	*/

	// 2. 각 디렉토리의 끝 지점 반영해서 => dir_start_end 배열을 통해 print_rule 갱신
	// 2-1. dir_start_end 배열 구하기
	// num: node 개수, depth_max: 최대 깊이 
	int start, end, d_idx;
	int depth = 1;

	int dir_start_end[depth_max][num*2]; // 각 depth마다 어디서 시작하고 끝나는지 저장하는 배열
	// worst_case: 1 1 2 2 3 3 4 4 ... num num 이더라도 num*2보다 클 수 없다.

	temp = head; // temp 초기화
	start = end = -1; // start, end 초기화
	while (1) {
		idx = 0; // idx 초기화
		d_idx = 0;
		temp = head; // temp 초기화
		while (temp != NULL) {
			// start가 정해지지 않은 경우
			if (start == -1 && temp->dir_depth == depth) {
				start = idx; // start 설정
	//			printf("depth: %d, start: %d\n", depth, start+1);
			}
			// start가 정해졌는데, 똑같은 depth가 나온 경우
			if (start != -1 && temp->dir_depth == depth) {
				// start가 설정된 경우 end = start로 설정된다.
				end = idx; // end 설정
	//			printf("depth: %d, end: %d\n", depth, end+1);
			}
			// start가 정해졌는데, node에 끝에 도달한 경우 OR depth가 더 작아진 경우
			if (start != -1 && (idx == (num-1) || temp->dir_depth < depth)) {
	//			printf("Depth %d=> %d번 째 start: %d | end: %d\n", depth, d_idx+1, start+1, end+1);
				// start = end 여도 변동 없이 2개씩 쌍을 이뤄 저장
				dir_start_end[depth-1][d_idx] = start;
				d_idx++;
				dir_start_end[depth-1][d_idx] = end;
				d_idx++;
				start = end = -1; // start, end 초기화
			}

			temp = temp->next; // 다음 node로 이동
			idx++;
			if (idx == num) {
				dir_start_end[depth-1][d_idx] = -1; // -1은 null 역할
				break;
			}
		}
		depth++;
		if (depth > depth_max)
			break;
	}

	/*
	printf("dir_start_end: \n");
	for (int i = 0; i < depth_max; i++) {
		for (int j = 0; j < num; j++) {
			if (dir_start_end[i][j] == -1)
				break;
			printf("%d ", dir_start_end[i][j]);
		}
		printf("\n");
	}
	printf("\n");
	*/

	// 2-2. dir_start_end 배열을 돌면서 시작과 끝이 아닌 지점은 0으로 바꿔준다.

	
	//int print_rule[num][depth_max]; // 각 노드마다 어떻게 출력해야 하는지 문자로 저장
	//int dir_start_end[depth_max][num]; // 각 depth마다 어디서 시작하고 끝나는지 저장하는 배열
	start = end = 0;
	int newstart;
	for (int depth = 0; depth < depth_max; depth++) {
		for (int j = 0; j < num; j+=2) {
			newstart = -1; // newstart 초기화
			if (dir_start_end[depth][j] == -1) break;

			start = dir_start_end[depth][j];
			end = dir_start_end[depth][j+1];
			// 새로운 start가 있는 경우
			if (dir_start_end[depth][j+2] != -1) {
				newstart = dir_start_end[depth][j+2];
			}
			// start와 end를 가지고 print_rule 수정
			for (int k = 0; k < num; k++) {
				if (k < start) continue;
				else if (k >= start && k <= end) print_rule[k][depth] = 1;

				if (k == end) print_rule[k][depth] = 2;
				if (k > end) {
					if (newstart != -1 && k == newstart) break;
					print_rule[k][depth] = 0;
				}
			}
			// k = newstart여서 break; 되었다면 다음 start, end를 탐색하고 수행
		}
	}
	/*
	printf("Middle print_rule: \n");
	for (int i = 0; i < num; i++) {
		for (int j = 0; j < depth_max; j++) {
			printf("%d ", print_rule[i][j]);
		}
		printf("\n");
	}
	printf("\n");
	*/
	/* 
	   3 => |-
	   2 => ㄴ
	   1 => |
	   0 => " "
	   */
	// 3. 2가지의 규칙에도 포괄되지 않는다면 이는 특징이 없는 디렉토리의 요소이다.
	// 따라서 디렉토리의 마지막이 아닌 요소라면 |- 가 출력되야 한다.
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
		if (temp_n == -1) continue; // 1이 없는 경우
		else
			print_rule[i][temp_n] = 3; // 제일 끝에 |-가 출력되도록 설정
	}

	/*
	// 최종 print_rule 출력
	printf("print_rule: \n");
	for (int i = 0; i < num; i++) {
		for (int j = 0; j < depth_max; j++) {
			printf("%d ", print_rule[i][j]);
		}
		printf("\n");
	}
	printf("\n");
	*/

	// 실제 tree 출력
	temp = head; // temp 초기화

	char original_wd[PATH_MAX], now_path[PATH_MAX];
	if (getcwd(original_wd, sizeof(original_wd)) == NULL) {
		perror("getcwd() error");
		return -1;
	}

	int root = 0; // root 선언. => root 경로의 출력을 따로 관리하기 위한 변수

	idx = 0; // idx 초기화
	while (temp != NULL) {
		// directory, file 개수 세기
		if (temp->current_type == 2) (*dir_count)++;
		else if (temp->current_type == 1) (*f_count)++;

		// print_rule에 따라서 앞에 출력을 결정한다.
		for (int j = 0; j < depth_max; j++) {
			if (j == temp->dir_depth) break;
			/* \u251c : ㅏ, \u2514 : ㄴ, \u2500 : ㅡ, \u2502 : ㅣ*/

			if (print_rule[idx][j] == 3) {
				printf("\u251c\u2500  ");
			} else if (print_rule[idx][j] == 2) {
				printf("\u2514\u2500  ");
			} else if (print_rule[idx][j] == 1) {
				printf("\u2502   ");
			} else if (print_rule[idx][j] == 0) {
				printf("    ");
			}
		}
		// temp->name의 절대경로
		strcpy(now_path, temp->parent_real_path);
		strcat(now_path, "/");
		strcat(now_path, temp->name);

		// option에 따라 node 출력
		switch (tree_option) {
			case NON:
				printf("%s", temp->name);
				if (root != 0 && temp->current_type == 2) printf("/");
				printf("\n");
				root = -1; // root 비활성화
				break;
			case O_S:
				// option
				printf("[");
				if (chdir(temp->parent_real_path) != 0) {
					perror("chdir() error");
					return -1;
				}
				if (print_size(temp->name) < 0) return -1;
				if (chdir(original_wd) != 0) {
					perror("chdir() error");
					return -1;
				}
				printf("]");
				printf(" ");
				// 출력
				printf("%s", temp->name);
				if (root != 0 && temp->current_type == 2) printf("/");
				printf("\n");
				root = -1; // root 비활성화
				break;
			case O_P:
				// option
				printf("[");
				if (chdir(temp->parent_real_path) != 0) {
					perror("chdir() error");
					return -1;
				}
				if (print_authority(temp->name) < 0) return -1;
				if (chdir(original_wd) != 0) {
					perror("chdir() error");
					return -1;
				}
				printf("]");
				printf(" ");
				// 출력
				printf("%s", temp->name);
				if (root != 0 && temp->current_type == 2) printf("/");
				printf("\n");
				root = -1; // root 비활성화
				break;
			case O_SP:
				// option
				printf("[");
				if (chdir(temp->parent_real_path) != 0) {
					perror("chdir() error");
					return -1;
				}
				if (print_authority(temp->name) < 0) return -1;
				printf(" ");
				if (print_size(temp->name) < 0) return -1;
				if (chdir(original_wd) != 0) {
					perror("chdir() error");
					return -1;
				}
				printf("]");
				printf(" ");
				// 출력
				printf("%s", temp->name);
				if (root != 0 && temp->current_type == 2) printf("/");
				printf("\n");
				root = -1; // root 비활성화
				break;
			default:
			    break;
		}

		idx++;
		prev_depth = temp->dir_depth;
		temp = temp->next;
	}
	printf("\n");
	return 0;
}

int directory_structuralize(char *path, int dir_depth, dNode** head) {
//	printf("    directory_structuralize() start!\n");
	if (tree_stop_flag == 1)
		return -1; // 재귀적으로 처리하다가 에러가 난 경우


	struct dirent **namelist;
	int count = 0;
	int current_path_type;
	
	if ((count = scandir(path, &namelist, filter, alphasort)) == -1) {
		fprintf(stderr, "%s Directory Scan error : %s\n", path, strerror(errno));
		tree_stop_flag = 1; // => directory_structuralize 순회 stop
		return -1;
	}

	// path주소 밑에서 순회한다.
	for (int idx = 0; idx < count; idx++) {
		//printf("%s : ", namelist[idx]->d_name);
		current_path_type = check_file_type(namelist[idx]->d_type);
		//printf("\n");

		// 현재 작업 디렉토리 출력
		char temp_wd[PATH_MAX];
		if (getcwd(temp_wd, sizeof(temp_wd)) == NULL) {
			perror("getcwd() error");
			return -1;
		}
		//printf("> > > > 현재 작업 디렉토리 : %s\n", temp_wd);
		// 절대경로를 통해서 부모 디렉토리 구하기
		char delim = '/'; // 구분자
		int start_idx = 0;
		char parentName[NAME_MAX]; // 부모 디렉토리 이름
		for (int i = 0; i < strlen(temp_wd); i++) {
			if (temp_wd[i] == delim)
				start_idx = i;
		}
		//printf("start_idx : %d\n", start_idx);
		strcpy(parentName, temp_wd+start_idx+1);
		//printf("parent directory : %s\n", parentName);
		

		// namelist에 있는 것들을 dNode에 복사
		appendNode(head, namelist[idx]->d_name, parentName, temp_wd, current_path_type, dir_depth); /////////

		// 6. 해당 경로가 일반 파일 또는 디렉토리가 맞는가?
		if (!(current_path_type == 1 || current_path_type == 2)) {
			fprintf(stderr, "directory_structuralize() ERROR : It is not Regular file OR Derectory\n");
			tree_stop_flag = 1; // => directory_structuralize 순회 stop
			return -1;
		}
		// 7. 해당 경로로 접근할 수 있는가?
		if (CheckPathAccess(namelist[idx]->d_name) == -1) {
			fprintf(stderr, "ERROR : directory_structuralize() Directory path is not exist\n");
			tree_stop_flag = 1; // => directory_structuralize 순회 stop
			return -1;
		} else if (CheckPathAccess(namelist[idx]->d_name) == 0) {
			fprintf(stderr, "ERROR : directory_structuralize() It couldn't access\n");
			tree_stop_flag = 1; // => directory_structuralize 순회 stop
			return -1;
		}

		// 만약에 지금의 경로가 디렉토리라면 directory_structuralize 재귀 호출
		if (current_path_type == 2) {
		//	printf("current <%s> path : Directory.\n", namelist[idx]->d_name);
			char original_wd[PATH_MAX], real_path[PATH_MAX];
			if (!realpath(namelist[idx]->d_name, real_path)) {
				perror("realpath() error");
				tree_stop_flag = 1; // => directory_structuralize 순회 stop
				return -1;
			}
			if (getcwd(original_wd, sizeof(original_wd)) == NULL) {
				perror("getcwd() error");
				tree_stop_flag = 1; // => directory_structuralize 순회 stop
				return -1;
			}
			//printf("print_print_n() => original_wd : %s  real_path : %s\n", original_wd, real_path);
			if (chdir(real_path) != 0) {
				perror("chdir() error");
				tree_stop_flag = 1; // => directory_structuralize 순회 stop
				return -1;
			}
			directory_structuralize(real_path, dir_depth+1, head); // directory 안에 있는 것들은 depth가 1 증가한다.
			if (chdir(original_wd) != 0) {
				perror("chdir() error");
				tree_stop_flag = 1; // => directory_structuralize 순회 stop
				return -1;
			}
		}
	}

	// 메모리 해제
	for (int i = 0; i < count; i++) {
		free(namelist[i]);
	}
	free(namelist);
	return 0;
}

int Tree(int argc, char *argv[], int tree_option) {
	int idx = 0;
	int root_type;
	int ishome;

	//printf("    tree() start!\n");
	// 1. 첫 번째 인자 입력이 있는가?
	if ((idx = CheckOnlyPath(argc, argv)) == 0) {
		Help(H_TREE, 0);
		return -1;
	}
	// 그리고 <DIR_PATH>가 단독인가? => 문법에 알맞다면 idx는 경로를 가리키는 인덱스로 설정된다.
	if ((idx = CheckOnlyPath(argc, argv)) == -1) {
		fprintf(stderr, "ERROR : Tree() Need only one Directory path\n");
		Help(H_TREE, 0);
		return -1;
	}
	// 2. <DIR_PATH>가 길이 제한(255 Byte)를 넘어가는가?
	if (strlen(argv[idx]) > NAME_MAX) {
		fprintf(stderr, "ERROR : Tree()'s <DIR_PATH> exceeds 255 Bytes.\n");
		Help(H_TREE, 0);
		return -1;
	}
	// 3. <DIR_PATH>가 존재하는가? & 접근할 수 있는가?
	if (CheckPathAccess_fixWD(argv[idx], "/") == -1) {
		fprintf(stderr, "ERROR : Tree() Directory path is not exist OR is not access\n");
		Help(H_TREE, 0);
		return -1;
	}
	// 4. <DIR_PATH>가 존재한다면 그것이 Directory인가?
	if ((root_type = CheckPathType_fixWD(argv[idx], "/")) == -127) return -1; // lstat()함수가 제대로 안 된 경우
	if (root_type != 2) {
	   // fprintf(stderr, "ERROR : Tree() <%s> It is not directory\n", argv[idx]);
	   Help(H_TREE, 0);
	   return -1;
	}
	// 5. <DIR_PATH>가 사용자의 홈 디렉토리($HOME, ~)를 벗어나는가?
	if ((ishome = CheckPath_InsideHome(argv[idx])) == -1) return -1; // 내부 함수의 문자열 할당이 제대로 안 된 경우
	if (ishome == 0) {
		fprintf(stdout, "<%s> is outside the home directory\n", argv[idx]);
		return -1;
	}

	// 0. 두 번째 인자로 올바르지 않은 옵션이 들어왔을 경우
	if (tree_option != NON &&
			tree_option != O_S && tree_option != O_P &&
			tree_option != O_SP) {
		// fprintf(stderr, "ERROR : Tree() only allow <none>, '-s' or '-p' option\n");
		Help(H_TREE, 0);
		return -1;
	} else {
		char original_wd[PATH_MAX];
		char real_path[PATH_MAX];
		if (!realpath(argv[idx], real_path)) {
			perror("realpath() error");
			return -1;
		}
		if (getcwd(original_wd, sizeof(original_wd)) == NULL) {
			perror("getcwd() error");
			return -1;
		}
		if (chdir(real_path) != 0) { // 현재 작업 디렉토리를 real_path로 전환
			perror("chdir() error");
			return -1;
		}
		//printf("Tree() => original_wd: %s \t real_path: %s\n", original_wd, real_path);
		
		// 디렉토리 구조를 가리키는 head 선언
		int dir_depth = 0; // 디렉토리 구조의 첫 번째는 루트(depth=0)가 된다. 그 이후로는 depth = 1부터 시작이다.
		dNode* head = NULL;

		// root 추가
		char delim = '/';
		int start_idx = 0;
		char parentName[NAME_MAX];
		char parent_real_path[PATH_MAX];
		for (int i = 0; i < strlen(real_path); i++) {
			if (real_path[i] == delim)
				start_idx = i;
		}
		strncpy(parent_real_path, real_path, start_idx); // 부모 절대경로 구하기
		parent_real_path[start_idx] = '\0';
		start_idx = 0; // start_idx 초기화
		for (int i = 0; i < strlen(parent_real_path); i++) {
			if (parent_real_path[i] == delim)
				start_idx = i;
		}
		strcpy(parentName, parent_real_path+start_idx+1); // 부모 디렉토리 이름 구하기
		//printf("parentName: %s | head parent_real_path: %s\n", parentName, parent_real_path);

		appendNode(&head, argv[idx], parentName, parent_real_path, root_type, dir_depth);
		

		// root 이후는 directory_structuralize 함수가 처리	
		if (directory_structuralize(real_path, dir_depth+1, &head) == -1) { // 중간에 에러나면 함수 call들이 종료된다.
			Help(H_TREE, 0);
		}
		// tree 구조화 end
		//printf("Later list: \n");
		//printList(head);
		//printf("\n");
		//printf("head->parentName: %s | head->parent_real_path: %s\n", head->parentName, head->parent_real_path);

		// tree 출력
		int dir_count = 0, f_count = 0;
		print_treeList(head, tree_option, &dir_count, &f_count);
		if (dir_count <= 1)
			printf("%d directory, ", dir_count);
		else
			printf("%d directories, ", dir_count);
		if (f_count <= 1)
			printf("%d file", f_count);
		else
			printf("%d files", f_count);
		printf("\n");

		freeList(head);

		// Tree() 동작을 모두 수행했으니, 현재 작업 디렉토리를 복구한다.
		if (chdir(original_wd) != 0) {
			perror("chdir() error");
			return -1;
		}
	}
	return 0;
}
double how_long_modified(dNode *scan) {
	struct stat statbuf;
	char original_wd[PATH_MAX];

	if (getcwd(original_wd, sizeof(original_wd)) == NULL) {
		perror("getcwd()");
		return -1;
	}
	if (chdir(scan->parent_real_path) != 0) { // 실행 시킬 파일의 부모 디렉토리로 작업 디렉토리를 이동
		perror("chdir()");
		return -1;
	}

	if (scan->current_type != 1) {
		//printf("Regular file이 아닙니다\n");
		chdir(original_wd);
		return -1;
	}

	// stat 구조체에서 time_t st_mtime /* 최종 수정 시간 */ 을 가져옴
	if (stat(scan->name, &statbuf) < 0) {
		perror("stat()");
		chdir(original_wd);
		return -1;
	}
	
	time_t now = time(NULL); // 현재 시간을 반환: NULL로 설정했기에 1970-01-01, 0시부터 현재까지 몇 초 지났는지 알려준다.
	double diff = difftime(now, statbuf.st_mtime); // 초 단위 시간 차이

	if (chdir(original_wd) != 0) {
		perror("chdir()");
		return -1;
	}

	return diff;
}
int overlapFile_handling(dNode* scan) {
    int number = 0;
    dNode* temp = scan;

    // 중복된 파일 목록 출력
    while (temp != NULL) {
        printf("%d. %s/%s\n", ++number, temp->parent_real_path, temp->name);
        temp = temp->next;
    }

    char input[PATH_MAX * 3];
    int argc;
    char* argv[FACTOR_MAX];

    while (1) {
        printf("choose an option:\n");
        printf("0. select [num]\n");
        printf("1. diff [num1] [num2]\n");
        printf("2. vi [num]\n");
        printf("3. do not select\n");
		printf("\n");
        printf("20220000> ");

        if (fgets(input, sizeof(input), stdin) == NULL)
            continue;
        input[strcspn(input, "\n")] = '\0';

        argc = 0;
        char* token = strtok(input, " ");
        while (token != NULL && argc < FACTOR_MAX) {
            argv[argc++] = token;
            token = strtok(NULL, " ");
        }

        // 잘못된 입력
        if (argc < 1) {
			printf("\n");
            continue;
		}

        // 0. select
        if (strcmp(argv[0], "select") == 0 && argc == 2) {
            int idx = atoi(argv[1]);
            if (idx <= 0 || idx > number) {
                printf("invalid number\n");
                continue;
            }

            dNode* selected = scan;
            for (int i = 1; i < idx; i++)
                selected = selected->next;

            printf("Selected: %s/%s\n", selected->parent_real_path, selected->name);
			// 파일 작성
			char temp_wd[PATH_MAX];
			if (getcwd(temp_wd, sizeof(temp_wd)) == NULL) {
				perror("getcwd()");
				return -1;
			}
			//printf("now working directory: %s\n", temp_wd);
			char origin_filepath[PATH_MAX];
			snprintf(origin_filepath, sizeof(origin_filepath), "%s/%s", selected->parent_real_path, selected->name);
			//printf("origin: %s\n", origin_filepath);
			int fd1, fd2;
			if ((fd1 = open(origin_filepath, O_RDONLY)) < 0) {
				fprintf(stderr, "open error for %s\n", origin_filepath);
				return -1;
			}
			// 확장자 디렉토리로 접근
			int dot_idx = 0;
			for (int i = 0; i < strlen(selected->name); i++) {
				if (selected->name[i] == '.')
					dot_idx = i;
			}
			if (chdir(selected->name + dot_idx+1) != 0) {
				perror("chdir()");
				return -1;
			}
			//printf("Later working directory: %s\n", temp_wd);
			if ((fd2 = open(selected->name, O_WRONLY | O_CREAT | O_TRUNC, S_MODE)) < 0) {
				fprintf(stderr, "open error for %s\n", selected->name);
				return -1;
			}
			// buf로 fd1의 내용을 읽어 fd2에 쓰기
			char buf[BUFFER_SIZE];
			ssize_t n;
			while ((n = read(fd1, buf, BUFFER_SIZE)) > 0) {
				//printf("ssize_t : %ld\n", n);
				buf[n] = '\0';

				if (write(fd2, buf, n) != n) {
					perror("write()");
					return -1;
				}
			}
			if (n < 0) {
				perror("read()");
				return -1;
			}
			close(fd1);
			close(fd2);
			// 작업 디렉토리 복구
			if (chdir(temp_wd) != 0) {
				perror("chdir()");
				return -1;
			}
			printf("\n");
			break;
        }

        // 1. diff
        else if (strcmp(argv[0], "diff") == 0 && argc == 3) {
            int idx1 = atoi(argv[1]);
            int idx2 = atoi(argv[2]);
            if (idx1 <= 0 || idx2 <= 0 || idx1 > number || idx2 > number) {
                printf("invalid number\n");
                continue;
            }

            dNode *f1 = scan, *f2 = scan;
            for (int i = 1; i < idx1; i++) f1 = f1->next;
            for (int i = 1; i < idx2; i++) f2 = f2->next;

            char path1[PATH_MAX], path2[PATH_MAX];
            snprintf(path1, sizeof(path1), "%s/%s", f1->parent_real_path, f1->name);
            snprintf(path2, sizeof(path2), "%s/%s", f2->parent_real_path, f2->name);

            FILE *fp1 = fopen(path1, "r");
            FILE *fp2 = fopen(path2, "r");

            if (!fp1 || !fp2) {
                perror("fopen");
                if (fp1) fclose(fp1);
                if (fp2) fclose(fp2);
                continue;
            }

            char line1[BUFFER_SIZE], line2[BUFFER_SIZE];
            int line_num = 1;

            while (fgets(line1, sizeof(line1), fp1) && fgets(line2, sizeof(line2), fp2)) {
                if (strcmp(line1, line2) != 0) {
                    printf("%dc%d\n", line_num, line_num);
                    printf("< %s", line1);
                    printf("---- ---- ---- ---- ----\n");
                    printf("> %s", line2);
                }
                line_num++;
            }

            // 둘 중 하나가 끝났는지 체크
            if (feof(fp1) && !feof(fp2))
                printf("file1 ends first\n");
            else if (!feof(fp1) && feof(fp2))
                printf("file2 ends first\n");

            fclose(fp1);
            fclose(fp2);
			printf("\n");
        }

        // 2. vi
        else if (strcmp(argv[0], "vi") == 0 && argc == 2) {
            int idx = atoi(argv[1]);
            if (idx <= 0 || idx > number) {
                printf("invalid number\n");
                continue;
            }

            dNode* target = scan;
            for (int i = 1; i < idx; i++) target = target->next;

            char filepath[PATH_MAX];
            snprintf(filepath, sizeof(filepath), "%s/%s", target->parent_real_path, target->name);

            FILE *fp = fopen(filepath, "r");
            if (!fp) {
                perror("fopen");
                continue;
            }

            char line[BUFFER_SIZE];
            while (fgets(line, sizeof(line), fp)) {
                fputs(line, stdout);
            }

            fclose(fp);
			printf("\n");
        }

        // 3. do not select
        else if (strcmp(argv[0], "do") == 0 && argc >= 3 && strcmp(argv[1], "not") == 0 && strcmp(argv[2], "select") == 0)
            break;
		printf("\n");
    }

    return 0;
}
int arrange_dirList(dNode* head, char fileName_overlapped[][NAME_MAX], int overlap) {
				// head 기반으로 Arrange()에서 디렉토리가 만들어졌기에, 만들어지지 않은 확장자 디렉토리는 없다.
	char original_wd[PATH_MAX], temp_wd[PATH_MAX];
	if (getcwd(original_wd, sizeof(original_wd)) == NULL) {
		perror("getcwd()");
		return -1;
	}
	//printf("\narrange_dirList's pwd: %s\n", original_wd);
	char delim_dot = '.';
	int dot_idx = 0;
	dNode* temp;
	char buf[BUFFER_SIZE + 1];
	int src_fd, dst_fd;
	ssize_t n; // read()의 return값을 조사하기 위한 변수

	//printf("arrange_dirList에 들어온 head: ");
	//printList(head);
	//printf("\n");

	// overlap : 중복된 파일명의 개수
	//for (int i = 0; i < overlap; i++) {
	//	printf("%s ", fileName_overlapped[i]);
	//}
	//printf("\n");

	/* 중복된 파일이 없는 경우 */
	if (overlap == 0) {
		//printf("중복된 파일이 없습니다! ^~^\n");
		temp = head;
		while (temp != NULL) { 
			// directory-> pass
			if (temp->current_type == 2) {
				temp = temp->next;
				continue;
			}
			// root가 되어 그 밑에서 확장자 디렉토리로 전환하여 파일이 저장되는 형태다.
			if (chdir(original_wd) != 0) {
				perror("chdir()");
				return -1;
			}

			// 일반 파일의 확장자 조사
			dot_idx = 0;
			for (int i = 0; i < strlen(temp->name); i++) {
				if (temp->name[i] == delim_dot)
					dot_idx = i;
			}
			// 작업 디렉토리를 확장자 디렉토리로 전환
			if (chdir(temp->name + dot_idx+1) != 0) {
				perror("chdir()");
				return -1;
			}

			/* 파일 불러와서, 확장자 디렉토리에 파일 복사 */
			// 1. 현재 작업 디렉토리 (확장자 디렉토리) 임시 저장
			if (getcwd(temp_wd, sizeof(temp_wd)) == NULL) {
				perror("getcwd()");
				return -1;
			}
			// 2. temp->parent_real_path로 전환
			if (chdir(temp->parent_real_path) != 0) {
				perror("chdir()");
				return -1;
			}
			// 3. temp->name 파일 open (read mode)
			if ((src_fd = open(temp->name, O_RDONLY)) < 0) {
				fprintf(stderr, "open error for %s\n", temp->name);
				return -1;
			}

			// 4. 확장자 디렉토리로 복구
			if (chdir(temp_wd) != 0) {
				perror("chdir()");
				return -1;
			}
			// 5. temp->name 파일 open (read & write mode)
			if ((dst_fd = open(temp->name, O_WRONLY | O_CREAT | O_TRUNC, S_MODE)) < 0) {
				fprintf(stderr, "open error for %s\n", temp->name);
				return -1;
			}
			// 6. buf로 src_fd의 내용을 읽어 dst_fd에 모두 쓰기
			while((n = read(src_fd, buf, BUFFER_SIZE)) > 0) {
				buf[n] = '\0';

				if (write(dst_fd, buf, n) != n) {
					perror("write()");
					return -1;
				}
			}
			if (n < 0) {
				// 파일의 끝에 도달 한 경우: n = 0, 에러인 경우: n = -1
				perror("read()");
				return -1;
			}
			// 7. 사용한 파일 닫기
			close(src_fd);
			close(dst_fd);

			temp = temp->next;
		}

		if (chdir(original_wd) != 0) {
			perror("chdir()");
			return -1;
		}
		return 0;
	}
	/* 중복된 파일이 있는 경우 */
	//printf("중복된 파일이 있습니다.\n");
	int is_overlap = 0;
	// Step.1 중복된 파일이 아닌 파일들은 먼저 arrange 한다.
	temp = head;
	//int jang = 1;
	while (temp != NULL) { 
		//printf("중복되지 않은 파일 : %d번째 while문 동작\n", jang++);
		// directory-> pass
		if (temp->current_type == 2) {
			temp = temp->next;
			continue;
		}

		for (int i = 0; i < overlap; i++) {
			if (strcmp(temp->name, fileName_overlapped[i]) == 0) {
				is_overlap = 1;    // is_overlap 초기화
			//	break;
			}
		}
		// temp->name = fileName_overlapped[] ? -> pass
		if (is_overlap == 1) {
			is_overlap = 0; // 여기서 초기화
			temp = temp->next;
			continue;
		}

		// root가 되어 그 밑에서 확장자 디렉토리로 전환하여 파일이 저장되는 형태다.
		if (chdir(original_wd) != 0) {
			perror("chdir()");
			return -1;
		}
		//printf("%s 실행\n", temp->name);

		// 일반 파일의 확장자 조사
		dot_idx = 0;
		for (int i = 0; i < strlen(temp->name); i++) {
			if (temp->name[i] == delim_dot)
				dot_idx = i;
		}
		// 작업 디렉토리를 확장자 디렉토리로 전환
		if (chdir(temp->name + dot_idx+1) != 0) {
			perror("chdir()");
			return -1;
		}

		/* 파일 불러와서, 확장자 디렉토리에 파일 복사 */
		// 1. 현재 작업 디렉토리 (확장자 디렉토리) 임시 저장
		if (getcwd(temp_wd, sizeof(temp_wd)) == NULL) {
			perror("getcwd()");
			return -1;
		}
		// 2. temp->parent_real_path로 전환
		if (chdir(temp->parent_real_path) != 0) {
			perror("chdir()");
			return -1;
		}
		// 3. temp->name 파일 open (read mode)
		if ((src_fd = open(temp->name, O_RDONLY)) < 0) {
			fprintf(stderr, "open error for %s\n", temp->name);
			return -1;
		}

		// 4. 확장자 디렉토리로 복구
		if (chdir(temp_wd) != 0) {
			perror("chdir()");
			return -1;
		}
		// 5. temp->name 파일 open (read & write mode)
		if ((dst_fd = open(temp->name, O_WRONLY | O_CREAT | O_TRUNC, S_MODE)) < 0) {
			fprintf(stderr, "open error for %s\n", temp->name);
			return -1;
		}
		// 6. buf로 src_fd의 내용을 읽어 dst_fd에 모두 쓰기
		while((n = read(src_fd, buf, BUFFER_SIZE)) > 0) {
			buf[n] = '\0';

			if (write(dst_fd, buf, n) != n) {
				perror("write()");
				return -1;
			}
		}
		if (n < 0) {
			// 파일의 끝에 도달 한 경우: n = 0, 에러인 경우: n = -1
			perror("read()");
			return -1;
		}
		// 7. 사용한 파일 닫기
		close(src_fd);
		close(dst_fd);

		temp = temp->next;
	}

	if (chdir(original_wd) != 0) {
		perror("chdir()");
		return -1;
	}

	// Step.2 중복된 파일명을 가진 것들을 어떻게 정리할 지 판단한다.
	dNode* problemList; // 중복된 파일명을 가진 node를 추가하여 linked list로 만든다.
	for (int i = 0; i < overlap; i++) {
		problemList = NULL;
		temp = head; // temp 초기화
		while (temp != NULL) {
			if (strcmp(fileName_overlapped[i], temp->name) == 0)
				appendNode(&problemList, temp->name, temp->parentName, temp->parent_real_path, temp->current_type, temp->dir_depth);

			temp = temp->next;
		}
		
		if (overlapFile_handling(problemList) != 0) return -1;
		freeList(problemList);
		// 중복되는 것들 중 하나를 선택하면 arrange를 하기에 작업 디렉토리를 복구해줘야 한다.
		if (chdir(original_wd) != 0) {
			perror("chdir()");
			return -1;
		}
	}

	if (chdir(original_wd) != 0) {
		perror("chdir()");
		return -1;
	}
	return 0;
}
int Arrange(int argc, char *argv[], int arrange_option, int *option_factor_num) {
	int idx = 0;
	int root_type;
	int ishome;
	char first_wd[PATH_MAX];
	if (getcwd(first_wd, sizeof(first_wd)) == NULL) {
		perror("getcwd()");
		return -1;
	}

	//printf("    arrange() start!\n");
	// 1. 첫 번째 인자 입력이 있는가?
	if (CheckOnlyPath(argc, argv) == 0) {
		//Help(H_ARRANGE, 0);
		fprintf(stdout, "Usage : arrange <DIR_PATH> [OPTION]\n");
		return -1;
	}
	// arrange는 option 이후 나오는 인자가 DIR_PATH다. argv[0] = command
	for (int i = 1; i < argc; i++) {
		if (argv[i][0] != '-') {
			idx = i;
			break;
		}
	}
	//printf("<DIR_PATH> : %s\n", argv[idx]);

	// 2. <DIR_PATH>가 길이 제한(255 Byte)를 넘어가는가?
	if (strlen(argv[idx]) > NAME_MAX) {
		fprintf(stderr, "ERROR : Arrange()'s <DIR_PATH> exceeds 255 Bytes.\n");
		return -1;
	}
	// 3. <DIR_PATH>가 존재하는가? & 접근할 수 있는가?
	if (CheckPathAccess_fixWD(argv[idx], "/") == -1) {
		fprintf(stderr, "%s does not exist\n", argv[idx]);
		return -1;
	}
	// 4. <DIR_PATH>가 존재한다면 그것이 Directory인가?
	if ((root_type = CheckPathType_fixWD(argv[idx], "/")) == -127) return -1;
	if (root_type != 2) {
		fprintf(stderr, "%s is not a directory\n", argv[idx]);
		Help(H_ARRANGE, 0);
		return -1;
	}
	// 5. <DIR_PATH>가 사용자의 홈 디렉토리($HOME, ~)를 벗어나는가?
	if ((ishome = CheckPath_InsideHome(argv[idx])) == -1) return -1;
	if (ishome == 0) {
		fprintf(stdout, "<DIR_PATH> is outside the home directory\n");
		return -1;
	}

	// 0. 두 번째 인자로 올바르지 않은 옵션이 들어왔을 경우 : 옵션은 하나씩 입력할 수 있다.
	// 옵션의 경우의 수가 2^4이기에 들어온 옵션 별로 하나씩 처리하고, arrange() 옵션이 아닌 것만 예외 처리를 한다. (여사건)
	if (arrange_option == O_S ||
			arrange_option == O_P || arrange_option == O_SP) {
		//fprintf(stderr, "not allow option\n");
		Help(H_ARRANGE, 0);
		return -1;
	}
	// arrange의 경우 "-d hello -t 10 -x f"와 같이 option을 띄어 입력 받아야 한다.
	//for (int i = 0; i < 4; i++) {
	//	printf("%d ", option_factor_num[i]);
	//}
	//printf("\n");
	/* option_facotr_num[4]
	   [0] : -d's num		=> -d <output_path>
	   [1] : -t's num		=> -t <seconds>
	   [2] : -x's num		=> -x <exclude_p1>, ...
	   [3] : -e's num		=> -e <extension1>, ...
	 */
	 // -d와 -t는 옵션의 인자가 최대 1개다.
	 // 옵션이 입력되었지만 옵션의 인자가 입력되지 않는 경우는 인자를 처리하면서 예외처리 한다.
	if (option_factor_num[0] > 1 || option_factor_num[1] > 1) {
		Help(H_ARRANGE, 0);
		return -1;
	}
	
	/* <DIR_PATH>_arranged 디렉토리가 존재하지 않는 경우 option과는 무관하게 디렉토리 생성 */
	char original_wd[PATH_MAX];
	char real_path[PATH_MAX]; // 현재 작업 디렉토리의 절대 경로
	char real_dirName[NAME_MAX]; // 상대 경로일 경우를 대비해 실제 이름을 저장한다.
	char arranged_dirName[NAME_MAX]; // 정리한 파일을 위치시킬 디렉토리 이름
	char DirPathArranged_real_path[PATH_MAX]; // <DIR_PATH>_arranged의 절대 경로
	char *addString = "_arranged";
	
	if (!realpath(argv[idx], real_path)) {
		perror("realpath() error");
		return -1;
	}
	if (getcwd(original_wd, sizeof(original_wd)) == NULL) {
		perror("getcwd() error");
		return -1;
	}
	// real_dName 구하기
	char delim = '/';
	int start_idx = 0;
	for (int i = 0; i < strlen(real_path); i++) {
		if (real_path[i] == delim)
			start_idx = i;
	}
	strcpy(real_dirName, real_path + start_idx+1);

	strcpy(arranged_dirName, real_dirName);
	strcat(arranged_dirName, addString);
	//printf("Arrange's Working Directory: %s, what directory <%s>?\n", original_wd, real_dirName);
	//printf("arranged_dirName: %s\n", arranged_dirName);

	// 지금 작업 디렉토리에서 <DIR_PATH>_arranged 디렉토리가 존재하지 않는다면 만들어준다.
	if (CheckPathAccess(arranged_dirName) == -1) {
		if (mkdir(arranged_dirName, 0775) != 0) {
			perror("arranged_directory creat failure!");
			return -1;
		}
		printf("<%s> directory make!\n", arranged_dirName);
	}
	// 이제 <DIR_PATH>_arranged 디렉토리가 보장되니 이 디렉토리의 절대 경로를 구한다.
	if (!realpath(arranged_dirName, DirPathArranged_real_path)) {
		perror("realpath()");
		return -1;
	}
	//printf("dir_path_arranged => %s\n", DirPathArranged_real_path);

	// 현재 작업 디렉토리 기준으로 linked list를 만들기 위해 작업 디렉토리를 real_path로 전환 (예외 방지)
	if (chdir(real_path) != 0) {
		perror("chdir() error");
		return -1;
	}
	// 정리할 디렉토리 내부의 구조를 가리키는 head 선언
	int dir_depth = 0;
	dNode* head = NULL;
	char parentName[NAME_MAX];
	char parent_real_path[PATH_MAX]; // <DIR_PATH>로 간 다음 그 바로 위의 디렉토리를 조사
	//printf(">>>> real_path: %s\n", real_path);
	start_idx = 0; // start_idx 초기화
	for (int i = 0; i < strlen(real_path); i++) {
		if (real_path[i] == delim)
			start_idx = i;
	}
	strncpy(parent_real_path, real_path, start_idx);
	parent_real_path[start_idx] = '\0';
	start_idx = 0; // start_idx 초기화
	for (int i = 0; i < strlen(parent_real_path); i++) {
		if (parent_real_path[i] == delim)
			start_idx = i;
	}
	strcpy(parentName, parent_real_path + start_idx+1);
	//printf("head parent: %s | head parent_real_path: %s\n", parentName, parent_real_path);

	// arrange의 linked list는 root가 상대경로라면 절대경로(real_dirName)로 전환한다.
	appendNode(&head, real_dirName, parentName, parent_real_path, root_type, dir_depth);
	//printf("First list: \n");
	//printList(head);
	//printf("\n");

	if (directory_structuralize(real_path, dir_depth+1, &head) == -1) {
		Help(H_ARRANGE, 0);
	}
	// 현재 작업 디렉토리에 생성된 <DIR_PATH>_arranged 디렉토리를 제외시킨다.
	deleteNode(&head, arranged_dirName, real_dirName, 2, dir_depth+1);


	// directory_structuralize 하면서 작업 디렉토리가 변환되었으니, 다시 복구
	char temp_pp[PATH_MAX];
	getcwd(temp_pp, sizeof(temp_pp));
	//printf("<<<<<<    temp: %s\n", temp_pp);
	if (chdir(first_wd) != 0) {
		perror("chdir()");
		return -1;
	}
	//getcwd(temp_pp, sizeof(temp_pp));
	//printf(">>>>>>    temp: %s\n", temp_pp);
	

	/* 숨김 파일 제거 ( .{} )*/
	dNode* temp = head;
	while (temp != NULL) {
		//printf("%c\t", temp->name[0]);
		if (temp->name[0] == '.') {
			deleteNode(&head, temp->name, temp->parentName, temp->current_type, temp->dir_depth);
		}

		temp = temp->next;
	}
	/* 실행 파일 제거 */
	int tmp_dot_idx = 0;
	temp = head;
	while (temp != NULL) {
		tmp_dot_idx = 0;
		for (int i = 0; i < strlen(temp->name); i++) {
			if (temp->name[i] == '.')
				tmp_dot_idx = i;
		}
		// 위의 코드에서 숨김 파일을 제거했기 때문에 그 후에 나오는 파일은 확장자가 없는 파일
		if (tmp_dot_idx == 0)
			deleteNode(&head, temp->name, temp->parentName, temp->current_type, temp->dir_depth);

		temp = temp->next;
	}
	// Node 출력
	//printf("숨김파일, 실행파일 제거\n");
	//printList(head);
	//printf("\n");
	//printf("idx : %d\n", idx);

	/* 처리할 option에 해당하는 인자의 start_idx 찾기 */
	//printf("arrange_option : %d\n", arrange_option);
	int temp_idx = 0;
	int temp_isoption;

	temp_isoption = arrange_option;
	temp_isoption %= 100000;
	temp_isoption -= O_X;
	//printf(">>> temp_isoption: %d\n", temp_isoption);
	// 1. -x 있다면 실행
	if (temp_isoption >= 0) {
		temp_idx = idx; // idx는 현재 command <DIR_PATH>의 <DIR_PATH>를 가리키고 있다.
		for (int i = 1; i < argc; i++) {
			if (strcmp(argv[i], "-d") == 0) {
				temp_idx += option_factor_num[0];
				//printf("    temp_idx : %d\n", temp_idx);
			}
			else if (strcmp(argv[i], "-t") == 0) {
				temp_idx += option_factor_num[1];
				//printf("    temp_idx : %d\n", temp_idx);
			}
			else if (strcmp(argv[i], "-x") == 0) {
				break; // -x
			}
			else if (strcmp(argv[i], "-e") == 0) {
				temp_idx += option_factor_num[3];
				//printf("    temp_idx : %d\n", temp_idx);
			}
			if (temp_idx > argc) break; // argv[i]'s Segmentation Fault prevention
		}
		temp_idx++;
		//printf("-x's agent : %d -> %s\n", temp_idx, argv[temp_idx]);
		if (argv[temp_idx] == NULL) {
			printf("option's factor is not exist!\n");
			chdir(first_wd); // 작업 디렉토리 복구
			Help(H_ARRANGE, 0);
			return -1;
		}
		//printf("option_factor_num: %d\n", option_factor_num[2]);
		// <exclude_path1>, <exclude_path2>, ... 에 해당하는 디렉토리와 그 하위 파일들을 제외한다.
		for (int i = 0; i < option_factor_num[2]; i++) {
			temp = head; // temp 초기화
			while (temp != NULL) {
				// 부모가 <exclude_path>인 경우
				if (temp->parentName && strcmp(temp->parentName, argv[temp_idx]) == 0) {
					deleteNode(&head, temp->name, temp->parentName, temp->current_type, temp->dir_depth);
				}
				// 제외할 <exclude_path>인 경우
				if (temp->name && strcmp(temp->name, argv[temp_idx]) == 0) {
					deleteNode(&head, temp->name, temp->parentName, temp->current_type, temp->dir_depth);
				}
				temp = temp->next;
			}
			temp_idx++;
		}
	}
	temp_isoption = arrange_option;
	temp_isoption %= 10000;
	temp_isoption -= O_T;
	//printf(">>> temp_isoption: %d\n", temp_isoption);
	// 2. -t 있다면 실행
	if (temp_isoption >= 0) {
		temp_idx = idx;
		for (int i = 1; i < argc; i++) {
			if (strcmp(argv[i], "-d") == 0) {
				temp_idx += option_factor_num[0];
				//printf("    temp_idx : %d\n", temp_idx);
			}
			else if (strcmp(argv[i], "-t") == 0) {
				break; // -t
			}
			else if (strcmp(argv[i], "-x") == 0) {
				temp_idx += option_factor_num[2];
				//printf("    temp_idx : %d\n", temp_idx);
			}
			else if (strcmp(argv[i], "-e") == 0) {
				temp_idx += option_factor_num[3];
				//printf("    temp_idx : %d\n", temp_idx);
			}
			if (temp_idx >= argc) break; // argv[i]'s Segmentation Fault prevention
		}
		temp_idx++;
		//printf("-t's agent : %d -> %s\n", temp_idx, argv[temp_idx]);
		if (argv[temp_idx] == NULL) {
			printf("option's factor is not exist!\n");
			chdir(first_wd); // 작업 디렉토리 복구
			Help(H_ARRANGE, 0);
			return -1;
		}
		// <seconds> 문자열을 정수형으로 변환
		char *endptr;
		long int value = strtol(argv[temp_idx], &endptr, 10); // 10진수로
		double diff;
		dNode* scan;
		char original_wd[PATH_MAX];
		long int sub_res = 0;
		if (getcwd(original_wd, sizeof(original_wd)) == NULL) {
			perror("getcwd()");
			return -1;
		}
		if (*endptr != '\0') {
			return -1;
		}
		//printf("value : %ld\n", value);
		temp = head; // temp 초기화
		while (temp != NULL) {
			scan = temp;
			if ((diff = how_long_modified(scan)) == -1) {
				// regular file이 아닌 경우 & 내부 함수 호출이 제대로 이루어지지 않은 경우
				temp = temp->next;
				continue;
			}
			//printf("<%s> 파일 수정한 지 %0.f초 지났습니다.\n", temp->name, diff);
			// diff = value 부터 <seconds> 초가 지난 파일이라고 생각한다.
			sub_res = diff - value;
			//printf("sub_res: %ld\n", sub_res);
			if (sub_res < 0) {
				printf("%s 삭제\n", temp->name);
				deleteNode(&head, temp->name, temp->parentName, temp->current_type, temp->dir_depth);
			}
			temp = temp->next;
		}
		// how_long_modified함수에서 현재 작업 디렉토리를 변환하는 코드가 있으니, 다시 원상태로 복구한다.
		if (chdir(original_wd) != 0) {
			perror("chdir()");
			return -1;
		}
	}
	temp_isoption = arrange_option;
	temp_isoption %= 1000000;
	temp_isoption -= O_E;
	//printf(">>> temp_isoption: %d\n", temp_isoption);
	// 3. -e 있다면 실행
	if (temp_isoption >= 0) {
		temp_idx = idx;
		for (int i = 1; i < argc; i++) {
			if (strcmp(argv[i], "-d") == 0) {
				temp_idx += option_factor_num[0];
				//printf("    temp_idx : %d\n", temp_idx);
			}
			else if (strcmp(argv[i], "-t") == 0) {
				temp_idx += option_factor_num[1];
				//printf("    temp_idx : %d\n", temp_idx);
			}
			else if (strcmp(argv[i], "-x") == 0) {
				temp_idx += option_factor_num[2];
				//printf("    temp_idx : %d\n", temp_idx);
			}
			else if (strcmp(argv[i], "-e") == 0) {
				break; // -e
			}
			if (temp_idx >= argc) break; // argv[i]'s Segmentation Fault prevention
		}
		temp_idx++;
		//printf("-e's agent : %d -> %s\n", temp_idx, argv[temp_idx]);
		if (argv[temp_idx] == NULL) {
			printf("option's factor is not exist!\n");
			chdir(first_wd); // 작업 디렉토리 복구
			Help(H_ARRANGE, 0);
			return -1;
		}
		char f_extension[NAME_MAX];
		int tmp_start = 0; // 마지막 '.'의 index을 구하기 위한 변수
		char ext_delim = '.';
		int del_flag = 1;

		temp = head; // temp 초기화
		while (temp != NULL) {
			// directory면 pass
			if (temp->current_type == 2) {
				temp = temp->next;
				continue;
			}
			tmp_start = 0; // tmp_start 초기화
			f_extension[0] = '\0'; // 확장자 초기화

			for (int i = 0; i < strlen(temp->name); i++) {
				if (temp->name[i] == ext_delim)
					tmp_start = i;
			}
			// tmp_start = 0이라면 숨김파일 OR 확장자가 없는 것이다. -> 이를 제외한다.
			if (tmp_start == 0) {
				deleteNode(&head, temp->name, temp->parentName, temp->current_type, temp->dir_depth);
				temp = temp->next;
				continue;
			}
			// 현재 node(일반 파일) 확장자 구하기
			//printf("tmp_start : %d\n", tmp_start);
			strcpy(f_extension, temp->name + tmp_start+1);
			//printf("arrange -a : %s\n", f_extension);
			
			// extension1, extension2, ... 내에 있는 확장자가 맞는지 확인
			del_flag = 1; // 해당 파일을 제외할 지 여부를 판단하는 flag
			for (int i = 0; i < option_factor_num[3]; i++) {
				if (argc < temp_idx + i) break;

				if (strcmp(f_extension, argv[temp_idx + i]) == 0) {
					del_flag = 0;
					break; // 등록된 확장자 중에 포함된 경우
				}
			}
			if (del_flag == 1) {
				deleteNode(&head, temp->name, temp->parentName, temp->current_type, temp->dir_depth);
			}
			temp = temp->next;
		}
	}
	temp_isoption = arrange_option;
	temp_isoption %= 1000;
	temp_isoption -= O_D;
	//printf(">>> temp_isoption: %d\n", temp_isoption);
	// 4. -d 있다면 실행
	if (temp_isoption >= 0) {
		temp_idx = idx;
		for (int i = 1; i < argc; i++) {
			if (strcmp(argv[i], "-d") == 0) {
				break; // -d
			}
			else if (strcmp(argv[i], "-t") == 0) {
				temp_idx += option_factor_num[1];
			}
			else if (strcmp(argv[i], "-x") == 0) {
				temp_idx += option_factor_num[2];
			}
			else if (strcmp(argv[i], "-e") == 0) {
				temp_idx += option_factor_num[3];
			}
			if (temp_idx >= argc) break; // argv[i]'s Segmentation Fault prevention
		}
		temp_idx++;
		//printf("-d's agent : %d -> %s\n", temp_idx, argv[temp_idx]);
		if (argv[temp_idx] == NULL) {
			printf("option's factor is not exist!\n");
			chdir(first_wd); // 작업 디렉토리 복구
			Help(H_ARRANGE, 0);
			return -1;
		}
		// 지금 작업 디렉토리에서 <output_path>가 없다면 만들어야 한다.
		char real_path[PATH_MAX];

		// 부모 디렉토리 구하기 => 부모 디렉토리에서 <output_path>가 있는지 확인하기 위함이다.
		// 만약 없으면 realpath() 당연히 에러
		char delim = '/';
		int start_idx = 0;
		int tmp_idx = 0; // ~home{ '/' } 와 같다면 사실 ~home이다. '/'으로만 구분하게 되면 한 번 더 parent_path 구해야 한다.
		int is_delim_flag = 0;

		char parent_path[PATH_MAX], parent_real_path[PATH_MAX];
		char output_dir[NAME_MAX];
		for (int i = 0; i < strlen(argv[temp_idx]); i++) {
			if (argv[temp_idx][i] == delim) {
				tmp_idx = i;
				is_delim_flag = 1; // delim이 있다!
				continue;
			}
			else {
				start_idx = tmp_idx;
			}
		}
		// <output_path>가 없을 수 있으니 함부로 realpath 하면 안된다.
		if (is_delim_flag == 0)
			strcpy(output_dir, argv[temp_idx] + start_idx); // ex) home
		else
			strcpy(output_dir, argv[temp_idx] + start_idx+1); // ex) /home
		//printf("arrange -d => output_dir: %s\n", output_dir);
		if (start_idx == 0) {
			if (argv[temp_idx][0] == '.' && argv[temp_idx][1] == '.') {
				parent_path[0] = '.';
				parent_path[1] = '.';
				parent_path[2] = '\0';
			}
			else if (argv[temp_idx][0] == '.' && argv[temp_idx][1] != '.') {
				parent_path[0] = '.';
				parent_path[1] = '\0';
			}
			else if (argv[temp_idx][0] != '.') {
				parent_path[0] = '.';
				parent_path[1] = '\0';
			}
		}
		else {
			strncpy(parent_path, argv[temp_idx], start_idx);
			parent_path[start_idx] = '\0';
		}
		//printf("arrange -d first=> parentPath: %s\n", parent_path);
		// 상대경로일 때 /home, home 과 같은 경우가 생길 수 있다.
		if (parent_path[0] == '\0') {
			parent_path[0] = '.';
			parent_path[1] = '\0';
		}
		//printf("arrange -d second=> parentPath: %s\n", parent_path);
		if (!realpath(parent_path, parent_real_path)) {
			perror("realpath()");
			return -1;
		}
		//printf("arrange -d => parent_real_path: %s\n", parent_real_path);
		// <output_path>를 상대 경로로 잡았으면 현재 작업 디렉토리에 맞춰 계산되고,
		// 절대경로라면 바뀌지는 않도록 한다. 만약, 절대 경로가 없다면 밑의 에러 처리에서 걸러진다.
		if (chdir(parent_real_path) != 0) {
			perror("chdir()");
			return -1;
		}
		// <output_path>의 부모 디렉토리에서 <output_path>가 없으면 만든다.
		if (CheckPathAccess(output_dir) == -1) {
			if (mkdir(output_dir, 0775) != 0) {
				perror("<output_path> directory not creat");
				return -1;
			}
			printf("<%s> directory creat!\n", output_dir);
		}
		// <output_path>의 존재가 보장되니 realpath를 구한다.
		if (!realpath(output_dir, real_path)) {
			perror("realpath()");
			return -1;
		}
		//printf("arrange -d => real_path: %s\n", real_path);
		// => 생성되었으니 <output_path>의 real_path로 전환 : <output_path> 하위에서 파일들을 정리하기 위함이다.
		if (chdir(real_path) != 0) {
			perror("chdir()");
			return -1;
		}
	}
	else {
		// -d 옵션이 없으면 생성된 <output_path>_arranged로 작업 디렉토리를 전환해야 한다.
		//printf("-d옵션이 없다! => %s\n", DirPathArranged_real_path);
		if (chdir(DirPathArranged_real_path) != 0) {
			perror("chdir()");
			return -1;
		}
	}

	/* -d 옵션이 있었으면 현재 작업 디렉토리: output_path, 없으면 탐색하느라고 작업디렉토리 변환이 있었으니 다시 복구 */
	char temp_wd[PATH_MAX];
	if (getcwd(temp_wd, sizeof(temp_wd)) == NULL) {
		perror("getcwd()");
		return -1;
	}
	//printf("option END, now Working Directory: %s\n", temp_wd);
	// option 처리한 최종 arrange할 리스트
	//printf("Final list: \n");
	//printList(head);
	//printf("\n");

	temp = head; // temp 초기화
	// arrange할 Directory linked list의 node 개수 구하기
	int num = 0;
	while (temp != NULL) {
		num++;
		temp = temp->next;
	}
	// 시작하기 전에 중복되는 파일이 있나 확인
	//printf("num : %d\n", num);
	dNode* checker;
	char fileName_overlapped[(int)num/2 + 1][NAME_MAX];
	char file_extension[num][NAME_MAX];
	int overlap = 0;
	int extension = 0;
	char delim_dot = '.';
	int is_overlap;
	int is_extension;
	char cur_ext[NAME_MAX]; // 잠시 확장자를 저장할 배열
	int count;
	int dot_idx;

	temp = head; // temp 초기화
	while (temp != NULL) {
		// Directory -> pass
		if (temp->current_type == 2) {
			temp = temp->next;
			continue;
		}
		// 일반 파일에 해당하는 경우
		/* 1. 중복 파일 있는가? */
		is_overlap = 0;
		// 이미 중복 파일로 저장되어 있으면 fileName_overlapped에 저장하지 않는다.
		for (int i = 0; i < overlap; i++) {
			if (strcmp(temp->name, fileName_overlapped[i]) == 0) {
				is_overlap = 1;
				break;
			}
		}
		// 중복 파일에 저장되어 있지 않다면 이름이 단독으로 존재하는지 검색한다.
		checker = head;
		count = 0;
		while (checker != NULL) {
			// checker도 Regular file만 조사한다
			if (checker->current_type == 1 && strcmp(temp->name, checker->name) == 0)
				count++;
			checker = checker->next;
		}
		// fileName_overlapped에 등록되어 있지 않고, 2개 이상 있는 경우
		if (is_overlap == 0 && count >= 2)
			strcpy(fileName_overlapped[overlap++], temp->name);

		/* 2. 확장자 종류 파악 */
		// 어차피 숨김 파일은 앞에서 걸러진다. 따라서 마지막에 '.'이후에 나오는 것이 확장자다.
		dot_idx = 0;
		for (int i = 0; i < strlen(temp->name); i++) {
			if (temp->name[i] == delim_dot)
				dot_idx = i;
		}
		if (dot_idx == 0) {
			// 확장자가 없거나 숨김 파일인 경우
			temp = temp->next;
			continue;
		}
		strcpy(cur_ext, temp->name + dot_idx + 1); // 현재 파일의 확장자
		// 확장자가 이미 등록되어 있는가? => 새로운 확장자만 배열에 추가한다.
		is_extension = 0;
		for (int i = 0; i < extension; i++) {
			if (strcmp(cur_ext, file_extension[i])  == 0) {
				is_extension = 1;
				break;
			}
		}
		if (is_extension == 0)
			strcpy(file_extension[extension++], cur_ext);
		temp = temp->next;
	}

	/*
	printf("overlap: %d | extension: %d\n", overlap, extension);
	
	// fileName_overlapped 배열, file_extension 배열 출력
	printf("fileName_overlapped: ");
	for (int i = 0; i < overlap; i++) {
		printf("%s ", fileName_overlapped[i]);
	}
	printf("\n");
	printf("file_extension: ");
	for (int i = 0; i < extension; i++) {
		printf("%s ", file_extension[i]);
	}
	printf("\n");
	*/
	//printList(head);

	/* arrange 수행 */
	if (extension != 0) {
		// 1. 확장자 이름의 디렉토리 만들기
		for (int i = 0; i < extension; i++) {
			// 현재 작업 디렉토리에 <extension> directory가 없으면 만든다.
			if (CheckPathAccess(file_extension[i]) == -1) {
				if (mkdir(file_extension[i], 0775) != 0) {
					perror("<%s> directory not creat");
					return -1;
				}
				printf("<%s> directory creat!\n", file_extension[i]);
			}
		}
		// 2. 실제로 linked list를 arrange
		if (arrange_dirList(head, fileName_overlapped, overlap) == 0)
			printf("%s arranged\n", real_dirName);
		else
			printf("%s isn't arranged\n", real_dirName);
	}
	else {
		// 정리할 확장자가 없는 경우도 결과적으로는 arrange 된 상태로 볼 수 있다.
		printf("%s arranged\n", real_dirName);
	}

	// 메모리 해제
	freeList(head);
	// Arrange() 동작을 모두 수행했으니, 현재 작업 디렉토리를 복구한다.
	//printf("first_wd: %s\n", first_wd);
	if (chdir(first_wd) != 0) {
		perror("chdir() error");
		return -1;
	}
	return 0;
}

void Help(int print_type, int repeat_call) {
	char* cmd1[] = {
    	"  > tree <DIR_PATH> [OPTION]...",
    	"\t<none> : Display the directory structure recursively if <DIR_PATH> is a director",
    	"\t-s : Display the directory structure recursively if <DIR_PATH> is a directory,",
		"\t      including the size of each file",
    	"\t-p : Display the directory structure recursively if <DIR_PATH> is a directory,",
		"\t      including the permissions of each directory and file",
    	NULL
	};
	char* cmd2[] = {
	    "  > arrange <DIR_PATH> [OPTION]...",
    	"\t<none> : Arrange the directory if <DIR_PATH> is a directory",
	    "\t-d <output_path> : Specify the output directory <output_path>",
		"\t      where <DIR_PATH> will be arranged if <DIR_PATH> is a directory",
    	"\t-t <seconds> : Only arrange files that were modified more than <seconds> seconds ago",
    	"\t-x <exclude_path1, exclude_path2, ...> : Arrange the directory",
		"\t      if <DIR_PATH> is a directory except for the files inside <exclude_path> directory",
    	"\t-e <extension1, extension2, ...> : Arrange the directory with",
		"\t      the specified extension <extension1, extension2, ...>",
   		 NULL
	};
	char* cmd3[] = { "  > help [COMMAND]", NULL };
	char* cmd4[] = { "  > exit", NULL };

	char** commandDescription[5] = { NULL };
	commandDescription[0] = cmd1; // tree
	commandDescription[1] = cmd2; // arrange
	commandDescription[2] = cmd3; // help
	commandDescription[3] = cmd4; // exit

	if (repeat_call == 0) printf("Usage:\n"); // 반복호출 하지 않을 경우

	switch (print_type) {
		case H_TREE:
			for (int j = 0; commandDescription[0][j] != NULL; j++)
				printf("%s\n", commandDescription[0][j]);
			break;
		case H_ARRANGE:
			for (int j = 0; commandDescription[1][j] != NULL; j++)
				printf("%s\n", commandDescription[1][j]);
			break;
		case H_HELP:
			for (int j = 0; commandDescription[2][j] != NULL; j++)
				printf("%s\n", commandDescription[2][j]);
			break;
		case H_EXIT:
			for (int j = 0; commandDescription[3][j] != NULL; j++)
				printf("%s\n", commandDescription[3][j]);
			break;
		case H_ALL:
			for (int i = 0; i < 5; i++) {
				if (commandDescription[i] == NULL) break;
				for (int j = 0; commandDescription[i][j] != NULL; j++)
					printf("%s\n", commandDescription[i][j]);
			}
			break;
		default: break;
	}
}
void Exit() {
	//fprintf(stdout, "exit~!\n");
	exit(0);
}



int Prompt() {
	char *prompt = "20220000> "; // 프롬프트 출력문
	
	char input[STRING_MAX]; // 입력 명령줄

	int argc = 0;			// 인자의 개수
	char *argv[FACTOR_MAX] = { NULL }; // 인자들의 집합(순서대로)

	int command = -1; // 명령어

	fputs(prompt, stdout); 	// 프롬프트 출력
		
	if (fgets(input, sizeof(input), stdin) == NULL) { // 파일 끝이나 에러인 경우
		fprintf(stderr, "fgets() error!\n");
		return -1;
	}
		/*
		   1. 아무것도 입력하지 않았을 때
		   2. fgets 함수는 sizeof(input) 만큼 읽어오기에, \n가 필연적으로 저장되어야 한다.
		    그런데 사용자 입력이 STRING_MAX를 초과한다면 fgets함수는 STRING_MAX까지만 읽어온다.
		    즉, 초과하면 '\n'을 받아오지 못한다.
		 */
	if (input[0] == '\n') return 0; // 개행 문자만 받은 경우
	if (input[strlen(input) -1] != '\n') {
		fprintf(stderr, "ERROR : Prompt() It's so long~\n");
		return 0; // 받을 수 있는 STRING_MAX를 넘어선 경우
	}
	input[strlen(input) -1] = '\0'; // input에는 '\n'까지 저장되기에 개행 문자를 제거해야 한다.

	get_factor(&argc, argv, input); // 토큰화
	//printf("parsing 전\n");
	//for (int i = 0; i < argc; i++) printf("|%s| ", argv[i]);
	
	// arrange의 -d, -t, -x, -e의 경우 각각 옵션 뒤에 인자가 몇 개 오는 지 체크를 해야 한다.
	int option_factor_num[4] = { 0, 0, 0, 0 }; // [0]: -d의 인자의 개수, [1]: -t의 인자의 개수, [2]: -x의 인자의 개수, [3]: -e의 인자의 개수

	if (strcmp(argv[0], commandSet[1]) == 0) {
		int flag = -1;
		for (int i = 1; i < argc; i++) {
			if (strcmp(argv[i], "-d") == 0) {
				flag = 0;
				continue;
			}
			if (strcmp(argv[i], "-t") == 0) {
				flag = 1;
				continue;
			}
			if (strcmp(argv[i], "-x") == 0) {
				flag = 2;
				continue;
			}
			if (strcmp(argv[i], "-e") == 0) {
				flag = 3;
				continue;
			}
			option_factor_num[flag]++;
			
		}
	}
	//printf("option_factor_num : ");
	//for (int i = 0; i < 4; i++) printf("%d ", option_factor_num[i]);
	//printf("\n");

	optind = 1; // 처리되는 인자의 숫자 초기화 (getopt함수를 여러 번 호출하게 되면 optind 값은 누적된다)

	command = parsing(argc, argv); // 구문 분석
	//printf("parsing 후\n");
	//for (int i = 0; i < argc; i++) printf("|%s| ", argv[i]);
	//printf("\n");

	// 명령어 처리
	if (command == -1) {
		// 첫 번째 argv[]가 등록된 명령어가 아니다.
		fprintf(stderr, "ERROR : Prompt() \"%s\" is not command\n", argv[0]);
		return 0;
	} else if (command < -1) {
		// 등록되지 않은 option인 경우
		command *= -1; // 복구
		switch (command / 10000000) {
			case 1: Help(H_TREE, 0);    return 0;
			case 2: Help(H_ARRANGE, 0); return 0;
			case 3: Help(H_HELP, 0);    return 0;
			case 4: Help(H_EXIT, 0);    return 0;
		}
	}
	switch (command / 10000000) {
		case 1:
			//printf("command is tree\n");
			tree_stop_flag = 0; // tree()를 수행하기 전에 flag 초기화

			/* tree <DIR_PATH> [OPTION]... */
			// option : none, -s, -p
			Tree(argc, argv, command % 1000000);
			break;
		case 2:
			//printf("command is arrange\n");
			/* arrange <DIR_PATH> [OPTION]... */
			// option : none, -d, -t, -x, -e
			Arrange(argc, argv, command % 1000000, option_factor_num);
			break;
		case 3:
			/* help [COMMAND] */
			// option : none
			//printf("command is help\n");
			if ((command % 1000000) != 0) {
				fprintf(stderr, "ERROR : help() don\'t need option\n"); // help명령어는 허용되는 option이 없다.
				Help(H_HELP, 0);
				return 0; // 사용자 입력 명령어 강제 종료
			}
			if (argc == 1) {
				Help(H_ALL, 0);
				break;
			}
			int command_exist = 0, Usage_flag = 0;
			for (int i = 1; i < argc; i++) {
				command_exist = 0; // command_exist 초기화
				if ((strcmp(argv[i], commandSet[0])) == 0) {
					if (Usage_flag == 0) { printf("Usage:\n"); Usage_flag = 1; }
					command_exist = 1;
					Help(H_TREE, 1);
				}
				if ((strcmp(argv[i], commandSet[1])) == 0) {
					if (Usage_flag == 0) { printf("Usage:\n"); Usage_flag = 1; }
					command_exist = 1;
					Help(H_ARRANGE, 1);
				}
				if ((strcmp(argv[i], commandSet[2])) == 0) {
					if (Usage_flag == 0) { printf("Usage:\n"); Usage_flag = 1; }
					command_exist = 1;
					Help(H_HELP, 1);
				}
				if ((strcmp(argv[i], commandSet[3])) == 0) {
					if (Usage_flag == 0) { printf("Usage:\n"); Usage_flag = 1; }
					command_exist = 1;
					Help(H_EXIT, 1);
				}

				// help 뒤에 온 인자가 [COMMAND]가 아니였던 경우
				if (command_exist == 0) {
					fprintf(stderr, "ERROR : help() \"%s\" is not command\n", argv[i]);
					Help(H_HELP, 0);
					return 0; // 사용자 입력 명령어 강제 종료
				}
			}
			break;
		case 4:
			/* exit */
			// option : none
			//printf("command is exit\n");
			if ((command % 1000000) != 0) {
				fprintf(stderr, "ERROR : exit() don\'t need option\n"); // exit명령어는 허용되는 옵션이 없다.
				Help(H_EXIT, 0);
				return 0; // 사용자 입력 명령어 강제 종료
			}
			if (argc > 1) {
				printf("exit() ERROR : ");
				for (int i = 1; i < argc; i++) {
					printf("\"%s\"", argv[i]);
					if (i == argc-1) printf(" : ??\n");
					else printf(", ");
				}
				Help(H_EXIT, 0);
				return 0; // 사용자 입력 명령어 강제 종료
			}
			if (argc == 1) Exit();
			break;
		default:
			break;
	}

	// for (int i = 0; i < argc; i++) free(argv[i]);

	return 0;
}

int main(int argc, char *argv[])
{
	if (argc != 1) {
		fprintf(stderr, "Error!, Enter only executable file without a factor\n");
		exit(1);
	}

	//int n = 1;
	
	//printf("=====%d번=====\n", n++);	
	while (Prompt() != -1) {
		//printf("=====%d번=====\n", n++);	
		printf("\n");
	};

	exit(0);
}
