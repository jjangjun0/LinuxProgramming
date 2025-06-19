int structuralize_stop_flag = 0;

int filter(const struct dirent *entry);
int check_file_type(unsigned char d_type);
off_t make_file_size(char *path);
char* make_file_authority(char *path);
time_t make_file_mtime(char *path);
time_t make_file_ctime(char *path);

// File Data Node Struct
typedef struct dNode {
	/* data */
	char* name; // 파일 혹은 디렉토리 이름
	char* parentName; // 부모 디렉토리 이름
	char* parent_realpath; // 부모 디렉토리의 절대 경로

	int current_type; // regular file: 1, directory: 2
	int dir_depth;  // 파일 혹은 디렉토리의 깊이
	off_t file_size; // 파일 혹은 디렉토리 크기
	char *file_authority; // 접근 권한
	time_t last_mtime; // 마지막 내용 수정 시간
	time_t last_ctime; // 마지막 권한 수정 시간

	/* next node pointer */
	struct dNode* next;
} dNode;

/* About Data Node Struct Operating */
dNode* createNode(const char* name, const char* parentName, const char* parent_realpath, int current_type, int dir_depth, off_t file_size, char* file_authority, time_t last_mtime, time_t last_ctime);
void appendNode(dNode** head, char* name, char* parentName, char* parent_realpath, int current_type, int dir_depth, off_t file_size, char* file_authority, time_t last_mtime, time_t last_ctime);
void deleteNode(dNode** head, char* name, char* parentName, char* parent_realpath,int current_type, int dir_depth);
void printList(dNode* head);
void freeList(dNode** head);

// 새로운 노드 생성 함수
dNode* createNode(const char* name, const char* parentName, const char* parent_realpath, int current_type, int dir_depth, off_t file_size, char* file_authority, time_t last_mtime, time_t last_ctime) {
	dNode* new_dNode = (dNode*)malloc(sizeof(dNode));
	if (new_dNode == NULL) {
		fprintf(stderr, "Memory allocation failure\n");
	exit(1);
	}

	/* data 복사 */
	new_dNode->name = strdup(name);
	new_dNode->parentName = strdup(parentName);
	new_dNode->parent_realpath = strdup(parent_realpath);
	new_dNode->current_type = current_type;
	new_dNode->dir_depth = dir_depth;
	new_dNode->file_size = file_size;
	new_dNode->file_authority = strdup(file_authority);
	new_dNode->last_mtime = last_mtime;
	new_dNode->last_ctime = last_ctime;

	/* next pointer 지정 */
	new_dNode->next = NULL;

	return new_dNode;
}

// list에 node 추가
void appendNode(dNode** head, char* name, char* parentName, char* parent_realpath, int current_type, int dir_depth, off_t file_size, char* file_authority, time_t last_mtime, time_t last_ctime) {
	dNode* new_dNode = createNode(name, parentName, parent_realpath, current_type, dir_depth, file_size, file_authority, last_mtime, last_ctime);
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
void deleteNode(dNode** head, char* name, char* parentName, char* parent_realpath,int current_type, int dir_depth) {
	dNode* temp = *head;
	dNode* prev = NULL;

	// 삭제할 노드가 첫 번째 노드일 경우
	if (temp != NULL &&
		strcmp(temp->name, name) == 0 &&
		strcmp(temp->parentName, parentName) == 0 &&
		strcmp(temp->parent_realpath, parent_realpath) == 0 &&
		temp->current_type == current_type &&
		temp->dir_depth == dir_depth) {
		*head = temp->next;
		free(temp->name);
		free(temp->parentName);
		free(temp->file_authority);
		free(temp);
		return;
	}

	while (temp != NULL) {
		if (strcmp(temp->name, name) == 0 &&
			strcmp(temp->parentName, parentName) == 0 &&
			strcmp(temp->parent_realpath, parent_realpath) == 0 &&
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
	free(temp->file_authority);
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
		
		printf("parentName -> %s", temp->parentName);
		printf(" | type: (%d), depth: (%d)", temp->current_type, temp->dir_depth);
		printf(" | size: (%ld), authority: %s", temp->file_size, temp->file_authority);
		printf(" | mtime: (%ld), ctime: (%ld)\n", temp->last_mtime, temp->last_ctime);

		temp = temp->next;
	}
}
// 구조체 linked list 메모리 전체 해제
void freeList(dNode** head) {
	dNode* temp;
	while (*head != NULL) {
		temp = *head;
		*head = (*head)->next;
		free(temp->name);
		free(temp->parentName);
		free(temp->file_authority);
		free(temp);
	}
}

/* 구조체 구조화 */
int directory_structuralize(char *path, int dir_depth, dNode** head) {
	if (structuralize_stop_flag == 1)
		return -1; // 재귀적으로 처리하다가 에러가 난 경우


	struct dirent **namelist;
	int count = 0;
	int current_path_type;
	off_t file_size;
	char *file_authority = NULL;
	time_t last_mtime;
	time_t last_ctime;
	
	if ((count = scandir(path, &namelist, filter, alphasort)) == -1) {
		fprintf(stderr, "%s Directory Scan error : %s\n", path, strerror(errno));
		structuralize_stop_flag = 1; // => directory_structuralize 순회 stop
		return -1;
	}

	// path주소 밑에서 순회한다.
	for (int idx = 0; idx < count; idx++) {
		//printf("%s : ", namelist[idx]->d_name);
		current_path_type = check_file_type(namelist[idx]->d_type);
		//printf("\n");

		// 현재 작업 디렉토리
		char temp_wd[PATH_MAX];
		if (getcwd(temp_wd, sizeof(temp_wd)) == NULL) {
			fprintf(stderr, "getcwd error\n");
			return -1;
		}
		// 절대경로를 통해서 부모 디렉토리 구하기
		char delim_c = '/'; // 구분자
		int start_idx = 0;
		char parentName[NAME_MAX]; // 부모 디렉토리 이름
		for (int i = 0; i < strlen(temp_wd); i++) {
			if (temp_wd[i] == delim_c)
				start_idx = i;
		}
		strcpy(parentName, temp_wd+start_idx+1);

		// 파일 크기, 접근 권한 구하기
		file_size = make_file_size(namelist[idx]->d_name);
		file_authority = make_file_authority(namelist[idx]->d_name);

		// 파일 내용 수정 시간, 권한 수정 시간 구하기
		last_mtime = make_file_mtime(namelist[idx]->d_name);
		last_ctime = make_file_ctime(namelist[idx]->d_name);


		// namelist에 있는 것들을 dNode에 복사
		appendNode(head, namelist[idx]->d_name, parentName, temp_wd, current_path_type, dir_depth, file_size, file_authority, last_mtime, last_ctime); /////////

		// 해당 경로가 일반 파일 또는 디렉토리가 맞는가?
		if (!(current_path_type == 1 || current_path_type == 2)) {
			fprintf(stderr, "<%s> is not Regular file OR Derectory\n", namelist[idx]->d_name);
			structuralize_stop_flag = 1; // => directory_structuralize 순회 stop
			return -1;
		}
		// 해당 경로로 접근할 수 있는가?
		if (CheckPathAccess(namelist[idx]->d_name) == -1) {
			fprintf(stderr, "Directory path <%s> is not exist\n", namelist[idx]->d_name);
			structuralize_stop_flag = 1; // => directory_structuralize 순회 stop
			return -1;
		} else if (CheckPathAccess(namelist[idx]->d_name) == 0) {
			fprintf(stderr, "It couldn't access to %s\n", namelist[idx]->d_name);
			structuralize_stop_flag = 1; // => directory_structuralize 순회 stop
			return -1;
		}

		// 만약에 지금의 경로가 디렉토리라면 directory_structuralize 재귀 호출
		if (current_path_type == 2) {
		//	printf("current <%s> path : Directory.\n", namelist[idx]->d_name);
			char original_wd[PATH_MAX], real_path[PATH_MAX];
			if (!realpath(namelist[idx]->d_name, real_path)) {
				fprintf(stderr, "realpath error for %s\n", namelist[idx]->d_name);
				structuralize_stop_flag = 1; // => directory_structuralize 순회 stop
				return -1;
			}
			if (getcwd(original_wd, sizeof(original_wd)) == NULL) {
				fprintf(stderr, "getcwd error\n");
				structuralize_stop_flag = 1; // => directory_structuralize 순회 stop
				return -1;
			}
			//printf("print_print_n() => original_wd : %s  real_path : %s\n", original_wd, real_path);
			if (chdir(real_path) != 0) {
				fprintf(stderr, "chdir error for %s\n", real_path);
				structuralize_stop_flag = 1; // => directory_structuralize 순회 stop
				return -1;
			}
			directory_structuralize(real_path, dir_depth+1, head); // directory 안에 있는 것들은 depth가 1 증가한다.
			if (chdir(original_wd) != 0) {
				fprintf(stderr, "chdir error for %s\n", original_wd);
				structuralize_stop_flag = 1; // => directory_structuralize 순회 stop
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

/* "node" Creation & ManageMent */
int filter(const struct dirent *entry) {
	if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
		return 0;
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
off_t make_file_size(char *path) {
	struct stat statbuf;

	if ((stat(path, &statbuf)) < 0) {
		fprintf(stderr, "stat error in file_size\n");
		return -1;
	}

	return statbuf.st_size;
}
char *make_file_authority(char *path) {
	struct stat statbuf;

	if ((stat(path, &statbuf)) < 0) {
		fprintf(stderr, "stat error in file_authority\n");
		return NULL;
	}

	char *permissions = (char *)malloc(10);
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

	return permissions;
}
time_t make_file_mtime(char *path) {
	struct stat statbuf;

	if ((stat(path, &statbuf)) < 0) {
		fprintf(stderr, "stat error in file_mtime\n");
		return -1;
	}

	return statbuf.st_mtime;
}
time_t make_file_ctime(char *path) {
	struct stat statbuf;

	if ((stat(path, &statbuf)) < 0) {
		fprintf(stderr, "stat error in file_ctime\n");
		return -1;
	}

	return statbuf.st_ctime;
}
