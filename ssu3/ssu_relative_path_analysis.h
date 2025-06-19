struct iNode *Verify_and_get_iNode(char *formal_path, char *relative_path, __u8 file_type, char **interest_path, int *interest_path_depth, struct iNode *head);
struct iNode *Relative_analysis(char *relative_path, __u8 file_type, char **interest_path, int *interest_path_depth, struct iNode *head);


struct iNode *Verify_and_get_iNode(char *formal_path, char *relative_path, __u8 file_type,
	char **interest_path, int *interest_path_depth, struct iNode *head) {
	// $ DEBUG
	//printf("verify's formal_path: %s\n", formal_path);

	struct iNode *interest_iNode = NULL;
	// 정규화된 경로를 가지고 '/'을 구분자로 활용하여 parsing한다.
	char *temp_formal_path = (char *)malloc(strlen(formal_path) + 1);
	strcpy(temp_formal_path, formal_path);

	char *formal_args[FACTOR_MAX];
	int count = 0;

	char *token = strtok(temp_formal_path, "/");
	while (token != NULL) {
		formal_args[count++] = strdup(token);
		token = strtok(NULL, "/");
	}

	// 정규화된 경로 하나하나마다 특정 dir_depth에서 디렉토리가 존재하는지 검사
	struct iNode *temp = NULL;

	// formal_path = "."인 경우는 조사하지 않는다.
	if (strcmp(formal_path, ".") != 0) {
		int is_in_list = 0;
		int is_unknown = 0;

		// formal_args[0] == "."은 조사를 하나마나이다.
		(*interest_path_depth)++;

		for (int i = 1; i < count-1; i++) {
			temp = head;
			is_in_list = 0;
			while (temp != NULL) {
				//printf("%s | %s | %d | %u\n", temp->name, temp->parent, temp->dir_depth, temp->file_type);
				if (temp->dir_depth == (*interest_path_depth) &&
						temp->file_type == 2 &&
						strcmp(temp->parent, formal_args[i-1]) == 0 &&
						strcmp(temp->name, formal_args[i]) == 0) {
					is_in_list = 1;
					break;
				}
				temp = temp->next;
			}
			// 못 찾은 경우
			if (is_in_list == 0) {
				fprintf(stderr, "Error: '%s'(depth: %d) is not found in <%s> directory(depth: %d)\n",
						formal_args[i], (*interest_path_depth), formal_args[i-1], (*interest_path_depth)-1);
				is_unknown = 1;
				break;
			}
			(*interest_path_depth)++;
		}

		//printf("interest_path 있는지 조사\n");
		int is_interest_path = 0;
		int is_find = 0;
		temp = head;
		while (temp != NULL) {
			if (temp->dir_depth == (*interest_path_depth) &&
					strcmp(temp->parent, formal_args[count-2]) == 0 &&
					strcmp(temp->name, formal_args[count-1]) == 0) {
				if (temp->file_type == file_type) {
					is_interest_path = 1;
					is_unknown = 0;
					is_find = 1;
					// 앞의 과정에서 경로를 따라 이동했기에
					// 발견한 iNode가 우리가 입력한 interest_path이다.
					interest_iNode = createINode(temp->inode, temp->name, temp->parent,
							temp->dir_depth, temp->file_size, temp->file_type, temp->file_authority);
					break;
				}
				else {
					is_interest_path = 1;
				}
			}
			temp = temp->next;
		}

		//printf("is_interest_path(%d) | is_unknown(%d) | is_directory(%d)\n",
		//		is_interest_path, is_unknown, is_directory);

		if (is_interest_path == 0)
			is_unknown = 1;
		if (is_unknown) {
			fprintf(stderr, "Error: '%s' couldn't be verified\n", relative_path);
			exit(ERR_GRAMMER);
		}
		// 같은 이름의 파일은 있는데, 찾고자 한 파일의 유형과 다를 때
		if (!is_find) {
			switch (file_type) {
				case 1:
					fprintf(stderr, "Error: '%s' is not regular file\n", formal_args[count-1]);
					exit(1);
					break;
				case 2:
					fprintf(stderr, "Error: '%s' is not directory\n", formal_args[count-1]);
					exit(1);
					break;
				case 3:
					fprintf(stderr, "Error: '%s' is not character device\n", formal_args[count-1]);
					exit(1);
					break;
				case 4:
					fprintf(stderr, "Error: '%s' is not block device\n", formal_args[count-1]);
					exit(1);
					break;
				case 5:
					fprintf(stderr, "Error: '%s' is not FIFO\n", formal_args[count-1]);
					exit(1);
					break;
				case 6:
					fprintf(stderr, "Error: '%s' is not socket\n", formal_args[count-1]);
					exit(1);
					break;
				case 7:
					fprintf(stderr, "Error: '%s' is not symbolic link\n", formal_args[count-1]);
					exit(1);
					break;
				default:
					fprintf(stderr, "Error: '%s' is unknown\n", formal_args[count-1]);
					exit(1);
					break;
			}
		}
	}

	// args 중에서 interest_path 특정 짓기
	(*interest_path) = strdup(formal_args[(*interest_path_depth)]);
	// $ DEBUG
	//printf("verify's interest_path: %s\n", (*interest_path));

	for (int i = 0; i < count; i++)
		free(formal_args[i]);

	free(temp_formal_path);
	return interest_iNode;
}
struct iNode *Relative_analysis(char *relative_path, __u8 file_type, char **interest_path, int *interest_path_depth, struct iNode *head) {
	char formal_path[PATH_MAX];
	char delimit = '/';
	// #0. <PATH>가 상대경로가 아닌 경우
	if (relative_path[0] == delimit) {
		fprintf(stderr, "Error: '%s' is not relative path\n", relative_path);
		exit(1);
	}

	// '/'을 구분자로 활용하여 사용자 입력을 받은 relative_path를 parse한다. //
	char *args[FACTOR_MAX];
	int count = 0;

	if (relative_path[0] != '.')
		args[count++] = strdup(".");

	char *temp_path = (char *)malloc(strlen(relative_path) + 1);
	strcpy(temp_path, relative_path);
	char *token = strtok(temp_path, "/");
	while (token != NULL) {
		args[count++] = strdup(token);
		token = strtok(NULL, "/");
	}

	free(temp_path);
	// 중간에 ".", ".."을 계산하며 최종적인 "."기준 상대경로를 정규화시킨다. //

	formal_path[0] = '.';
	formal_path[1] = '\0';
	for (int i = 0; i < count; i++) {
		if (strcmp(args[i], ".") == 0) { }
		else if (strcmp(args[i], "..") == 0) {
			// "." 밖의 영역으로 나가는 경우를 예외 처리
			if (strcmp(formal_path, ".") == 0) {
				fprintf(stderr, "<PATH> is beyond the permitted range\n");
				exit(1);
			}
			// 여기서 ".."기준으로 그 앞에 있는 경로가 디렉토리인지 검사
			// ex1) ./A/hello.c/.. => 에러 처리
			// $ DEBUG
			//printf(".. find\n");
			//printf("formal_path: %s\n", formal_path);
			(*interest_path_depth) = 0;
			Verify_and_get_iNode(formal_path, relative_path, 2,
					interest_path, interest_path_depth, head);
			
			// ex2) relative_path = A/AA/..일 경우 formal_path = ./A/AA, ".."을 만나면 formal_path = ./A
			for (int j = strlen(formal_path) - 1; j > 0; j--) {
				if (formal_path[j] == delimit) {
					formal_path[j] = '\0';
						break;
				}
				formal_path[j] = '\0';
			}
		}
		else {
			strcat(formal_path, "/");
			strcat(formal_path, args[i]);
		}
	}
	// $ DEBUG
	//printf("\nnow interest_path check\n");
	//printf("Relative_analysis's formal_path: %s\n", formal_path);
	for (int i = 0; i < count; i++)
		free(args[i]);

	(*interest_path_depth) = 0;
	struct iNode *interest_iNode = Verify_and_get_iNode(formal_path, relative_path, file_type,
			interest_path, interest_path_depth, head);
	return interest_iNode;
}
