#include <linux/types.h>
#include <stdint.h>

/*
 * User structure
 */
typedef struct iNode {
	__le32 inode;     // Inode 번호
	char *name;       // 파일 혹은 디렉토리 이름
	char *parent;     // 부모 디렉토리 이름
	int dir_depth;    // 깊이
	__le32 file_size;  // 크기
	__u8 file_type;   // 파일 타입
	char *file_authority; // 접근 권한

	struct iNode *next; // next node pointer
} iNode;

/*
 * User Functions
 */
struct iNode* createINode(__le32 inode, const char *name, const char *parent, int dir_depth, off_t file_size, __u8 file_type, const char *file_authority);
void appendINode(iNode **head, __le32 inode, const char *name, const char *parent, int dir_depth, off_t file_size, __u8 file_type, const char *file_authority);
void deleteINode(iNode **head, __le32 inode, const char *name, const char *parent, int dir_depth);
void printINodeList(iNode *head);
void freeINodeList(iNode **head);

int make_dir_structure_with_ext2(struct iNode **head, int fd, int inode, int block_size, int inode_size, off_t inode_table_offset, char *parent, int dir_depth);
void read_root_inode_data_block(struct iNode **newList, int fd, uint32_t block_num, int block_size, int inode_size, off_t inode_table_offset, char *parent, int dir_depth);

void ascending_order_iNodeList(iNode **head);
char *make_file_authority(unsigned short i_mode);
int print_file_type(__u8 file_type);


// 노드 생성
struct iNode* createINode(__le32 inode, const char *name, const char *parent,
                   int dir_depth, off_t file_size, __u8 file_type, const char *file_authority) {
    iNode *newNode = (iNode*)malloc(sizeof(iNode));
    if (!newNode) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }

    newNode->inode = inode;
    newNode->name = strdup(name);
    newNode->parent = strdup(parent);
    newNode->dir_depth = dir_depth;
    newNode->file_size = file_size;
    newNode->file_type = file_type;
    newNode->file_authority = strdup(file_authority);
    newNode->next = NULL;

    return newNode;
}

// 리스트에 노드 추가
void appendINode(iNode **head, __le32 inode, const char *name, const char *parent,
                 int dir_depth, off_t file_size, __u8 file_type, const char *file_authority) {
    iNode *newNode = createINode(inode, name, parent, dir_depth, file_size, file_type, file_authority);

    if (*head == NULL) {
        *head = newNode;
        return;
    }

    iNode *temp = *head;
    while (temp->next)
        temp = temp->next;

    temp->next = newNode;
}

// 노드 삭제
void deleteINode(iNode **head, __le32 inode, const char *name, const char *parent, int dir_depth) {
    iNode *temp = *head, *prev = NULL;

    while (temp != NULL) {
        if (temp->inode == inode &&
            strcmp(temp->name, name) == 0 &&
            strcmp(temp->parent, parent) == 0 &&
            temp->dir_depth == dir_depth) {

            if (prev == NULL)
                *head = temp->next;
            else
                prev->next = temp->next;

            free(temp->name);
            free(temp->parent);
            free(temp->file_authority);
            free(temp);
            return;
        }

        prev = temp;
        temp = temp->next;
    }
}

// 리스트 출력
void printINodeList(iNode *head) {
    while (head != NULL) {
        printf("Inode: %u | Name: %s | Parent: %s | Depth: %d | Type: %u |Size: %u | Auth: %s\n",
               head->inode, head->name, head->parent, head->dir_depth,
			   head->file_type, head->file_size, head->file_authority);
        head = head->next;
    }
}

// 메모리 해제
void freeINodeList(iNode **head) {
    iNode *temp;
    while (*head) {
        temp = *head;
        *head = (*head)->next;
        free(temp->name);
        free(temp->parent);
        free(temp->file_authority);
        free(temp);
    }
}

// 처음 입력받은 inode에 관하여 디렉토리 구조를 링크드 리스트로 표현하는 함수
// 단, head는 NULL이여야 한다.
// (head 앞 부분의 iNode의 name에 따라 재귀를 하다 논리구조가 꼬일 수 있음)
int make_dir_structure_with_ext2(struct iNode **head, int fd, int inode, int block_size,
				int inode_size, off_t inode_table_offset, char *parent, int dir_depth) {
	struct iNode *newList = NULL;
	struct ext2_inode ei;

	// inode_table에서 해당 inode의 정보를 읽는다
	off_t offset = inode_table_offset + (inode - 1) * inode_size;
	if (lseek(fd, offset, SEEK_SET) < 0) {
		fprintf(stderr, "lseek error for inode(%d)\n", inode);
		return -1;
	}
	if (read(fd, &ei, sizeof(ei)) != sizeof(ei)) {
		fprintf(stderr, "read error for ei\n");
		return -1;
	}

	// #1. inode에 해당하는 디렉토리가 사용하는 모든 data block을 읽는다
	// 1. Direct blocks (0~11)
	for (int i = 0; i < 12; i++) {
		if (ei.i_block[i] == 0)
			continue;
		read_root_inode_data_block(&newList, fd, ei.i_block[i], block_size, inode_size, inode_table_offset, parent, dir_depth);
	}
	// 2. Single Indirect block (12)
	if (ei.i_block[12] != 0) {
		uint32_t *indirect = malloc(block_size);
		if (lseek(fd, ei.i_block[12] * block_size, SEEK_SET) < 0) exit(12);
		if (read(fd, indirect, block_size) < 0) exit(12);

		for (int i = 0; i < block_size / sizeof(uint32_t); i++) {
			if (indirect[i] == 0)
				continue;
			read_root_inode_data_block(&newList, fd, indirect[i], block_size, inode_size, inode_table_offset, parent, dir_depth);
		}
		free(indirect);
	}
	// 3. Double Indirect block (13)
	if (ei.i_block[13] != 0) {
		uint32_t *dindirect = malloc(block_size);
		if (lseek(fd, ei.i_block[13] * block_size, SEEK_SET) < 0) exit(13);
		if (read(fd, dindirect, block_size) < 0) exit(13);

		for (int i = 0; i < block_size / sizeof(uint32_t); i++) {
			if (dindirect[i] == 0)
				continue;
			uint32_t *indirect = malloc(block_size);
			if (lseek(fd, dindirect[i] * block_size, SEEK_SET) < 0) exit(12);
			if (read(fd, indirect, block_size) < 0) exit(12);

			for (int j = 0; j < block_size / sizeof(uint32_t); i++) {
				if (indirect[j] == 0)
					continue;
				read_root_inode_data_block(&newList, fd, indirect[j], block_size, inode_size, inode_table_offset, parent, dir_depth);
			}
			free(indirect);
		}
		free(dindirect);
	}
	// 4. Triple Indirect block (14)
	if (ei.i_block[14] != 0) {
		uint32_t *tindirect = malloc(block_size);
		if (lseek(fd, ei.i_block[14] * block_size, SEEK_SET) < 0) exit(14);
		if (read(fd, tindirect, block_size) < 0) exit(14);

		for (int i = 0; i < block_size / sizeof(uint32_t); i++) {
			if (tindirect[i] == 0)
				continue;
			uint32_t *dindirect = malloc(block_size);
			if (lseek(fd, tindirect[i] * block_size, SEEK_SET) < 0) exit(13);
			if (read(fd, dindirect, block_size) < 0) exit(13);

			for (int j = 0; j < block_size / sizeof(uint32_t); i++) {
				if (dindirect[j] == 0)
					continue;
				uint32_t *indirect = malloc(block_size);
				if (lseek(fd, dindirect[j] * block_size, SEEK_SET) < 0) exit(12);
				if (read(fd, indirect, block_size) < 0) exit(12);

				for (int k = 0; k < block_size / sizeof(uint32_t); i++) {
					if (indirect[k] == 0)
						continue;
					read_root_inode_data_block(&newList, fd, indirect[k], block_size, inode_size, inode_table_offset, parent, dir_depth);
				}
				free(indirect);
			}
			free(dindirect);
		}
		free(tindirect);
	}

	// #2. newList 오름차순 정렬
	ascending_order_iNodeList(&newList);

	// #3. 새로 만들어진 newList를 기존 head가 가리키는 list에 연결한다.
	struct iNode *prev = NULL;
	struct iNode *concern = NULL;
	if (*head == NULL) {
		*head = newList;
	}
	else {
		struct iNode *temp1 = *head;
		struct iNode *temp2 = newList;

		while (temp1 != NULL) {
			if (temp1->dir_depth == dir_depth - 1 && strcmp(temp1->name, parent) == 0)
				break;
			temp1 = temp1->next;
		}
		prev = temp1->next; // # 복구 지점
		concern = temp1;
		//concern = createINode(temp1->inode, temp1->name, temp1->parent, temp1->dir_depth, temp1->file_size, temp1->file_type, temp1->file_authority);

		// 기존 head가 가리키는 linked-list에 새로 만들어진 newList를 삽입
		while (temp2 != NULL) {
			struct iNode *newNode = createINode(temp2->inode, temp2->name, temp2->parent, temp2->dir_depth, temp2->file_size, temp2->file_type, temp2->file_authority);
			temp1->next = newNode;

			temp1 = temp1->next;
			temp2 = temp2->next;
		}
		temp1->next = prev; // # 끊어진 연결 복구
		
		freeINodeList(&newList);
	}

	// 처음에만 모두 조사하고, 나머지의 경우는 새로 추가된 지점부터 조사한다.
	struct iNode *temp = NULL;
	if (concern == NULL)
		temp = *head;
	else
		temp = concern->next;
	//printf("depth %d - concern: %s\n", dir_depth, temp->name);
	
	// #4. 재귀적으로 make_dir_structure_with_ext2 호출
	int inspect_dir_depth = temp->dir_depth;

	// while문의 조건에 무엇이 들어가야하는가?
	// (temp != NULL && temp->dir_depth == inspect_dir_depth) 해버리면
	// temp = temp->next; 이렇게 돌다가 한 번 make_dir_structure_with_ext2 호출이 되어버린 후,
	// linked-list에 추가된 것이 없으면 상관없는데, 추가되면 문제가 있다. 왜?
	// 다음 iNode가 dir_depth+1로 증가되기 때문에 while문이 종료되어
	// 이후의 디렉토리에 대한 재귀호출이 이뤄지지 않는다.
	// temp != NULL하게 되면, 물론 concern을 활용해서 *head 앞부분의 내용을 건너뛰기 때문에
	// 정상동작을 한다.
	// 근데 동작횟수가 list의 길이가 늘어날 수록 불필요하게 node를 훑게 된다.

	// 따라서 새로 생긴 node에 대해서만 순회하도록 temp != prev를 채택했다.
	// 맨 처음 이 함수가 호출되더라도 head가 null이면 시작점(concern)은 첫 node가 되고, 마지막(prev)은 NULL이 되기에 성립한다.

	while (temp != prev) {
		if (temp->file_type == 2 && temp->dir_depth == inspect_dir_depth) {
			//printf("재귀 %s (depth: %d)\n", temp->name, temp->dir_depth);
			make_dir_structure_with_ext2(head, fd, temp->inode, block_size, inode_size, inode_table_offset, temp->name, dir_depth+1);
			//printf("%s 재귀 후:\n", temp->name);
			//printINodeList(*head);
		}
		temp = temp->next;
	}

	return 0;
}
// 특정 경로에 대한 root inode에 대한 data block영역을 읽는 함수
// (directory 이기에 ext2_dir_entry_2 구조체로 entry의 rec_len을 더하고 점프하면서 읽는다.)
void read_root_inode_data_block(struct iNode **newList, int fd, uint32_t block_num, int block_size,
				int inode_size, off_t inode_table_offset, char *parent, int dir_depth) {
	uint8_t *block = (uint8_t *)malloc(block_size);
	if (block == NULL) {
		fprintf(stderr, "malloc error for block\n");
		exit(2);
	}

	if (lseek(fd, block_num * block_size, SEEK_SET) < 0) {
		fprintf(stderr, "lseek error for data block\n");
		exit(2);
	}
	if (read(fd, block, block_size) < 0) {
		fprintf(stderr, "read error for data block\n");
		exit(2);
	}

	/* [ new linked-list 만들기 ] */
	uint32_t pos = 0;
	while (pos < block_size) {
		struct ext2_dir_entry_2 *entry = (void *)(block + pos);

		if (entry->inode == 0 || entry->rec_len == 0)
			break;

		// entry 구조체에서 name 구하기
		char name[entry->name_len+1];
		strcpy(name, entry->name);
		name[entry->name_len] = '\0';

		// "."와 "..", 그리고 "lost+found" directory는 제외시킨다.
		if (entry->file_type == 2 &&
				(strcmp(name, ".") == 0 || strcmp(name, "..") == 0 || strcmp(name, "lost+found") == 0)) {
			pos += entry->rec_len;
			continue;
		}
		// inode table에서 해당 inode값에 해당하는 위치를 계산하여, 읽어온다. (file_size, file_authority 구하기 위해)
		struct ext2_inode another_ei;

		off_t offset = inode_table_offset + (entry->inode - 1) * inode_size;
		if (lseek(fd, offset, SEEK_SET) < 0) {
			fprintf(stderr, "lseek error for inode(%u)\n", entry->inode);
			exit(2);
		}
		if (read(fd, &another_ei, sizeof(another_ei)) != sizeof(another_ei)) {
			fprintf(stderr, "read error for another_ei\n");
			exit(2);
		}
		char *permissions = make_file_authority(another_ei.i_mode);

		// 해당 inode의 data block영역에서 사용하는 block개수를 구할 수도 있다.
		// int real_use_block = (another_ei.i_blocks * 512) / block_size;
		appendINode(newList, entry->inode, name, parent, dir_depth, another_ei.i_size, entry->file_type, permissions);

		free(permissions);
		pos += entry->rec_len; // 다음 entry로 이동한다
	}
	free(block);
}
// 리스트 오름차순 정렬(같은 dir_depth에서만) : 삽입 정렬, O(n^2)
void ascending_order_iNodeList(iNode **head) {
	struct iNode *sorted_head = NULL; // 정렬된 새로운 linked-list
	struct iNode *temp = *head;
	struct iNode *min; // name 값이 최소가 되는 구조체를 가리키는 변수
	
	
	while (temp != NULL) {
		min = temp;

		// head를 순회하면서 name이 최소인 구조체를 찾는다
		while (temp != NULL) {
			if (strcmp(min->name, temp->name) > 0) {
				min = temp;
			}

			temp = temp->next;
		}
		appendINode(&sorted_head, min->inode, min->name, min->parent, min->dir_depth, min->file_size, min->file_type, min->file_authority);
		deleteINode(head, min->inode, min->name, min->parent, min->dir_depth);

		temp = *head;
	}

	*head = sorted_head;
}
// file authority 구하는 함수
char *make_file_authority(unsigned short i_mode) {
	char *permissions = (char *)malloc(10);
	if (permissions == NULL) {
		fprintf(stderr, "malloc error\n");
		return NULL;
	}

	// Owner
	permissions[0] = (i_mode & S_IRUSR) ? 'r' : '-';
	permissions[1] = (i_mode & S_IWUSR) ? 'w' : '-';
	permissions[2] = (i_mode & S_IXUSR) ? 'x' : '-';
	// Group
	permissions[3] = (i_mode & S_IRGRP) ? 'r' : '-';
	permissions[4] = (i_mode & S_IWGRP) ? 'w' : '-';
	permissions[5] = (i_mode & S_IXGRP) ? 'x' : '-';
	// Others
	permissions[6] = (i_mode & S_IROTH) ? 'r' : '-';
	permissions[7] = (i_mode & S_IWOTH) ? 'w' : '-';
	permissions[8] = (i_mode & S_IXOTH) ? 'x' : '-';
	permissions[9] = '\0';

	return permissions;
}
// file_type에 따라 상징 문자열을 출력하는 함수
int print_file_type(__u8 file_type) {
	switch (file_type) {
		case 1:  printf("-"); break; // regular file
		case 2:  printf("d"); break; // directory
		case 3:  printf("c"); break; // character device
		case 4:  printf("b"); break; // block device
		case 5:  printf("p"); break; // FIFO
		case 6:  printf("s"); break; // socket
		case 7:  printf("l"); break; // symbolic link
		default: printf("?"); break; // unknown
	}
	return 0;
}
