const char delim = '/';

/* Function */
int ssu_daemon_init(struct ssu_daemon_config *info);
int monitoring_act(struct ssu_daemon_config *info, dNode** prev_head, FILE *fp);
int arrange_dirList(dNode* head, char* monitoring_path, char* output_path, FILE *fp, pid_t pid, char *max_log_lines);
int dir_relationship(char path1[PATH_MAX], char path2[PATH_MAX]);
mode_t str_to_mode(const char *perm_str);
int lockfile(int fd);
int unlockfile(int fd);
int parse_config_file(struct ssu_daemon_config *info, FILE *fp);
long count_lines(FILE *fp);
int floating_log(FILE *fp, pid_t pid, char *parent_realpath, char *output_path, char *filename, char *max_log_lines);



int ssu_daemon_init(struct ssu_daemon_config *info) {
	pid_t pid;
	int fd, temp_fd, maxfd;
	char time_buf[64];

	if ((pid = fork()) < 0) {
		fprintf(stderr, "fork error\n");
		return -1;
	}
	// 여기
	else if (pid != 0) {
		// 중간 프로세스의 종료(exit)
		exit(0);
		//return 0;
	}
	
	/* child 프로세스 : daemon process start! */
	//printf("daemon process start!\n");
	// ~/
	if (chdir(dname) < 0) {
		fprintf(stderr, "[4] - chdir error for %s\n", dname);
		exit(1);
	}
	// ~/.ssu_cleanupd/current_daemon_list 파일 open
	if ((fd = open(DMlist_Filename, O_WRONLY | O_APPEND)) < 0) { // MODE : write
		fprintf(stderr, "open error for %s\n", DMlist_Filename);
		exit(1);
	}
	if ((temp_fd = open(DMlist_Filename, O_RDONLY)) < 0) { // MODE : read
		fprintf(stderr, "open error for %s\n", DMlist_Filename);
		exit(1);
	}
	if (chdir("..") < 0) {
		fprintf(stderr, "[5] - chdir error for ..\n");
		exit(1);
	}

	info->start_time = time(NULL);
	strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", localtime(&info->start_time)); // start_time 저장

	pid = getpid();
	info->pid = pid; // pid 저장

	//printf("time now: %ld\n", info->start_time);
	//printf("process %d running as daemon\n", pid);
	
	struct ssu_daemon cur_record; // current_daemon_list 파일에 정보를 쓸 구조체
	struct ssu_daemon temp_cur_record; // current_daemon_list 에서 정보를 읽는 구조체

	cur_record.pid = info->pid;
	strcpy(cur_record.real_path, info->monitoring_path);

	// [0] - monitoring 하는 daemonprocess가 이미 등록되었는지 확인
	int length1, length2;
	length1 = strlen(cur_record.real_path); // 등록하고 싶은 경로의 길이

	while (read(temp_fd, &temp_cur_record, sizeof(struct ssu_daemon)) > 0) {
		length2 = strlen(temp_cur_record.real_path);
		//fprintf(stderr, "%s | %s\n", cur_record.real_path, temp_cur_record.real_path);
		//fprintf(stderr, "%d | %d\n", length1, length2);
		if (length2 > length1 &&
				strncmp(temp_cur_record.real_path, cur_record.real_path, length1) == 0 &&
				temp_cur_record.real_path[length1] == delim) {

			fprintf(stderr, "daemon process: <%s>의 하위 디렉토리 경로가 이미 등록되어 있습니다.\n", cur_record.real_path);
			kill(info->pid, SIGKILL); // 데몬 프로세스 종료
			close(temp_fd);
			exit(1);
		}
		else if (length2 == length1 &&
				strcmp(temp_cur_record.real_path, cur_record.real_path) == 0) {
			fprintf(stderr, "daemon process: <%s>는 이미 등록된 모니터링 경로입니다.\n", cur_record.real_path);
			kill(info->pid, SIGKILL); // 데몬 프로세스 종료
			close(temp_fd);
			exit(1);		
		}
		else if (length2 < length1 &&
				strncmp(temp_cur_record.real_path, cur_record.real_path, length2) == 0 &&
				cur_record.real_path[length2] == delim) {

			fprintf(stderr, "daemon process: <%s>의 상위 디렉토리 경로가 이미 등록되어 있습니다.\n", cur_record.real_path);
			kill(info->pid, SIGKILL); // 데몬 프로세스 종료
			close(temp_fd);
			exit(1);
		}
	}

	// [1] - current_daemon_list에 등록되지 않는 경우 추가
	if (write(fd, &cur_record, sizeof(struct ssu_daemon)) < 0) {
		fprintf(stderr, "write error for %s\n", DMlist_Filename);
		close(fd);
		close(temp_fd);
		exit(1);
	}

	// [2] - daemon에 해당하는 정보(초기값)를 ssu_cleanupd.config에 적기
	FILE *fp;
	if (chdir(info->monitoring_path) < 0) {
		fprintf(stderr, "chdir error for %s\n", info->monitoring_path);
		exit(1);
	}
	if ((fp = fopen(config_Filename, "a")) == NULL) {
		fprintf(stderr, "fopen error for %s\n", config_Filename);
		exit(1);
	}
	fprintf(fp, "monitoring_path : %s\n", info->monitoring_path);
	fprintf(fp, "pid : %d\n", info->pid);
	fprintf(fp, "start_time : %s\n", time_buf); // 파일에는 start_time을 문자열로 저장
	fprintf(fp, "output_path : %s\n", info->output_path);
	fprintf(fp, "time_interval : %d\n", info->time_interval);
	fprintf(fp, "max_log_lines : %s\n", info->max_log_lines);
	fprintf(fp, "exclude_path : %s\n", info->exclude_path);
	fprintf(fp, "extension : %s\n", info->extension);
	fprintf(fp, "mode : %d\n", info->mode);

	fclose(fp);

	// 본격 데몬 프로세스 생성
	setsid();
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);

	maxfd = getdtablesize();
	for (fd = 0; fd < maxfd; fd++)
		close(fd);

	umask(0);
	chdir("/");
	fd = open("/dev/null", O_RDWR);
	dup(0);
	dup(0);

	dNode** prev_head = malloc(sizeof(dNode *));
	*prev_head = NULL;

	while (1) {
		/* 실제 daemon process 작업 수행 */
		FILE *fp1, *fp2;

		// 잠시 작업 디렉토리 옮기기
		if(chdir(info->monitoring_path) < 0)
			continue;

		// ssu_cleanupd.config에서 daemon에 해당하는 정보를 가져오기
		if ((fp1 = fopen(config_Filename, "a+")) == NULL)
			continue;
		// ssu_cleanupd.log 파일 열기
		if ((fp2 = fopen(log_Filename, "a+")) == NULL)
			continue;


		int fd1 = fileno(fp1);
		int fd2 = fileno(fp2);
		int unexpected_flag;

		// ssu_cleanupd.config 파일 lock 걸기 => modify와 충돌 방지
		while (1) {
			unexpected_flag = 0;

			if (lockfile(fd1) == 0) {
				break; // lock 성공
			}

			// 다른 프로세스가 lock을 걸고 있는 상태
			if (errno == EACCES || errno == EAGAIN) {
				usleep(100000); // 0.1초 대기
				continue;
			}
			else {
				unexpected_flag = 1;
				break;
			}
		}

		// 이전의 info랑 달라진 점이 있는가 비교
		int config_changed = 0;
		struct ssu_daemon_config temp_info;
		if (prev_head != NULL) {
			// info 갱신하기 전에 복사해놓는다.
			strcpy(temp_info.monitoring_path, info->monitoring_path);
			temp_info.pid = info->pid;
			temp_info.start_time = info->start_time;
			strcpy(temp_info.output_path, info->output_path);
			temp_info.time_interval = info->time_interval;
			strcpy(temp_info.max_log_lines, info->max_log_lines);
			strcpy(temp_info.exclude_path, info->exclude_path);
			strcpy(temp_info.extension, info->extension);
			temp_info.mode = info->mode;
			// info 갱신하기
			parse_config_file(info, fp1);

			// monitoring_path, pid, start_time은 바뀌면 안된다. 나머지가 바뀌었는지 확인한다.
			if (strcmp(temp_info.output_path, info->output_path) != 0 ||
					strcmp(temp_info.max_log_lines, info->max_log_lines) != 0 ||
					strcmp(temp_info.exclude_path, info->exclude_path) != 0 ||
					strcmp(temp_info.extension, info->extension) != 0 ||
					temp_info.time_interval != info->time_interval ||
					temp_info.mode != info->mode)
			{
				config_changed = 1;
			}
			// 특이한 case: output_path가 변하지 않고, 영향을 주지 않는 max_log_lines, time_interval만 바뀐 경우
			if (strcmp(temp_info.output_path, info->output_path) == 0 &&
					strcmp(temp_info.exclude_path, info->exclude_path) == 0 &&
					strcmp(temp_info.extension, info->extension) == 0 &&
					temp_info.mode == info->mode)
			{
				config_changed = 0; // 새로 갱신하지 않아야 한다.
			}
			// 달라졌다면 prev_head = NULL로 설정한다.
			if (config_changed == 1) {
				freeList(prev_head);
				*prev_head = NULL;
			}
		}

		// ssu_cleanupd.log 파일 lock 걸기 => show와 충돌 방지
		while (1) {
			if (lockfile(fd2) == 0) {
				break;
			}

			// 다른 프로세스가 lock을 걸고 있는 상태
			if (errno == EACCES || errno == EAGAIN) {
				usleep(100000); // 0.1초 대기
				continue;
			}
			else {
				unexpected_flag = 1;
				break;
			}
		}
		if (unexpected_flag == 1)
			continue;

		// 정리되지 않은 파일들을 주기적으로 식별하여 정리한다.
		monitoring_act(info, prev_head, fp2);

		// lock 해제
		if (unlockfile(fd1) == -1)
			continue;
		if (unlockfile(fd2) == -1)
			continue;

		close(fd1);
		close(fd2);
		fclose(fp1);
		fclose(fp2);

		chdir("/");
		sleep(info->time_interval);
	}

	return 0;
}


int monitoring_act(struct ssu_daemon_config *info, dNode** prev_head, FILE *fp) {
	// 작업 디렉토리를 <monitoring_path> 경로로 옮기기
	if (chdir(info->monitoring_path) < 0) {
		return -1;
	}

	char myName[NAME_MAX];
	char parentName[NAME_MAX];
	char parent_realpath[PATH_MAX];
	dNode* head = NULL; /* 현재 monitoring 경로에 대한 디렉토리 구조를 가리키는 head 선언 */

	/* 사전 작업 */
	int start_idx = 0;
	for (int i = 0; i < strlen(info->monitoring_path); i++) {
		if (info->monitoring_path[i] == delim)
			start_idx = i;
	}
	// 내 이름 구하기
	strcpy(myName, info->monitoring_path + start_idx+1);
	// 부모 절대 경로 구하기
	strncpy(parent_realpath, info->monitoring_path, start_idx);
	parent_realpath[start_idx] = '\0';

	start_idx = 0;
	for (int i = 0; i < strlen(parent_realpath); i++) {
		if (parent_realpath[i] == delim)
			start_idx = i;
	}
	// 부모 이름 구하기
	strcpy(parentName, parent_realpath + start_idx+1);

	int root_type;
	int dir_depth;
	off_t file_size;
	char *file_authority;
	time_t last_mtime;
	time_t last_ctime;

	root_type = CheckPathType(info->monitoring_path);
	dir_depth = 0; // 디렉토리 구조의 첫 번째는 루트(depth=0)이 된다.
	file_size = make_file_size(info->monitoring_path);
	file_authority = make_file_authority(info->monitoring_path);
	last_mtime = make_file_mtime(info->monitoring_path);
	last_ctime = make_file_ctime(info->monitoring_path);

	// root 추가
	appendNode(&head, myName, parentName, parent_realpath, root_type, dir_depth, file_size, file_authority, last_mtime, last_ctime);

	/* root 밑 구조화 */
	structuralize_stop_flag = 0;
	if (directory_structuralize(info->monitoring_path, dir_depth + 1, &head) < 0)
		return -1;

	/* 전처리 */
	dNode* temp;
	char dot = '.';
	// [0] - ssu_cleanupd.log와 ssu_cleanupd.config 파일 제외
	temp = head;
	while (temp != NULL) {
		if (strcmp(temp->name, config_Filename) == 0 || strcmp(temp->name, log_Filename) == 0)
			deleteNode(&head, temp->name, temp->parentName, temp->parent_realpath, temp->current_type, temp->dir_depth);
		temp = temp->next;
	}

	// [1] - 숨김 파일 제거 ( .{} )
	temp = head;
	while (temp != NULL) {
		if (temp->name[0] == dot)
			deleteNode(&head, temp->name, temp->parentName, temp->parent_realpath, temp->current_type, temp->dir_depth);
		temp = temp->next;
	}
	// [2] - 실행 파일 제거
	int tmp_dot_idx = 0;
	temp = head;
	while (temp != NULL) {
		// 디렉토리 제외
		if (temp->current_type == 2) {
			deleteNode(&head, temp->name, temp->parentName, temp->parent_realpath, temp->current_type, temp->dir_depth);
			temp = temp->next;
			continue;
		}
		// 실행 파일 제외
		tmp_dot_idx = 0;
		for (int i = 0; i < strlen(temp->name); i++) {
			if (temp->name[i] == dot)
				tmp_dot_idx = i;
		}
		// 위의 코드에서 숨김 파일을 제거했기에, (tmp_dot_idx == 0)의 조건을 만족한다면, 이는 '.'이 없는 파일이다.
		if (tmp_dot_idx == 0)
			deleteNode(&head, temp->name, temp->parentName, temp->parent_realpath, temp->current_type, temp->dir_depth);
		temp = temp->next;
	}
	// [3] - "-x" 옵션 인자에 해당하는 디렉토리에 포함되는 경로를 모두 제외
	int dir_relation;
	if (strcmp(info->exclude_path, "none") != 0) {
		char *temp_exclude_path; // info->exclude_path 임시 저장
		int length;
		length = strlen(info->exclude_path);
		temp_exclude_path = (char *)malloc(length + 1);
		strcpy(temp_exclude_path, info->exclude_path);

		char *token = strtok(temp_exclude_path, ",");
		while (token != NULL) {
			temp = head;
			while (temp != NULL) {
				// 부모가 <exclude_path>인 경우 || 부모가 <exclude_path> 산하의 디렉토리인 경우
				dir_relation = dir_relationship(temp->parent_realpath, token);
				if (dir_relation == 0 || dir_relation == -1)
					deleteNode(&head, temp->name, temp->parentName, temp->parent_realpath, temp->current_type, temp->dir_depth);
				temp = temp->next;
			}

			token = strtok(NULL, ","); // NULL값을 전달하여 다음 토큰을 가져온다.
		}
		free(temp_exclude_path);
	}

	// [4] - "-e" 옵션 인자에 해당하지 않는 확장자에 해당하는 파일을 모두 제외
	if (strcmp(info->extension, "all") != 0) {
		int dot_end; // 마지막 '.'의 index를 구하기 위한 변수
		int del_flag; // 해당 파일을 제외할 지 여부를 판단하는 flag
		char f_extension[NAME_MAX];
		char *token;
		char *temp_extension; // info->extension 임시 저장
		int length;
		length = strlen(info->extension);
		temp_extension = (char *)malloc(length + 1);

		temp = head;
		while (temp != NULL) {
			// directory면 pass
			if (temp->current_type == 2) {
				temp = temp->next;
				continue;
			}
			// 기타 변수 초기화
			dot_end = 0;
			f_extension[0] = '\0';

			for (int i = 0; i < strlen(temp->name); i++) {
				if (temp->name[i] == dot)
					dot_end = i; // 마지막 dot 지점
			}
			// 파일이 확장자가 없다면 이를 제외한다. ex) current_daemon_list 파일
			if (temp->current_type == 1 && dot_end == 0) {
				deleteNode(&head, temp->name, temp->parentName, temp->parent_realpath, temp->current_type, temp->dir_depth);
				temp = temp->next;
				continue;
			}
			strcpy(f_extension, temp->name + dot_end+1);

			del_flag = 1; // flag 활성화

			strcpy(temp_extension, info->extension);
			token = strtok(temp_extension, ","); // token 구하기
			while (token != NULL) {
				if (strcmp(f_extension, token) == 0) {
					del_flag = 0; // 등록된 확장자 중에 포함된 경우 : flag 비활성화
					break;
				}

				token = strtok(NULL, ",");
			}

			if (del_flag == 1)
				deleteNode(&head, temp->name, temp->parentName, temp->parent_realpath, temp->current_type, temp->dir_depth);

			temp = temp->next;
		}
		free(temp_extension);
	}

	// [5] - "-m" 옵션값에 따라 중복된 파일에 대한 처리 : mtime을 기준으로

	// [5] Step.1 직접 list를 순회하면서 중복 파일의 종류를 파악한다. 덤으로 확장자의 종류도 파악한다.
	int num = 0; // 파일의 개수
	temp = head;
	while (temp != NULL) {
		if (temp->current_type == 1)
			num++;
		temp = temp->next;
	}

	int overlap_num = 0;   // 중복이 되는 파일의 종류의 개수
	int extension_num = 0; // 파일 확장자 개수
	int is_overlap;        // 파일이 겹치는 것이 있는가?
	int is_extension;      // 파일 확장자가 겹치는 것이 있는가?

	int dot_end;
	int count; // 특정 파일의 이름이 중복되는 횟수

	char fileName_overlapped[(int)num/2 + 1][NAME_MAX]; // 중복된 파일의 이름
	char cur_ext[NAME_MAX]; // 잠시 확장자를 저장할 배열
	char file_extension[num][NAME_MAX]; // 확장자 종류

	dNode* checker; // 특정 파일의 이름을 조사할 때 쓰이는 구조체 포인터
		
	temp = head;
	while (temp != NULL) {
		// !Regular file -> pass
		if (temp->current_type != 1) {
			temp = temp->next;
			continue;
		}
		/* 중복 파일 파악 */
		is_overlap = 0;
		// 1-1. 이미 중복 파일로 저장되어 있으면 배열에 저장하지 않는다.
		for (int i = 0; i < overlap_num; i++) {
			if (strcmp(temp->name, fileName_overlapped[i]) == 0) {
				is_overlap = 1;
				break;
			}
		}
		// 1-2. 중복 파일에 저장되어 있지 않다면, 이름이 단독으로 존재하는지 검색한다.
		count = 0;

		checker = head;
		while (checker != NULL) {
			if (checker->current_type == 1 && strcmp(temp->name, checker->name) == 0)
				count++;
			checker = checker->next;
		}
		// 1-3. fileName_overlapped에 등록되어 있지 않고, 2개 이상 있는 경우
		if (is_overlap == 0 && count >= 2)
			strcpy(fileName_overlapped[overlap_num++], temp->name);

		/* 확장자 종류 파악 */
		is_extension = 0;

		dot_end = 0;
		for (int i = 0; i < strlen(temp->name); i++) {
			if (temp->name[i] == dot)
				dot_end = i;
		}
		// 숨김 파일은 앞에서 걸러졌다. 마지막에 '.'이 나오는 index = 0이면 확장자가 없는 것
		if (dot_end == 0) {
			temp = temp->next;
			continue;
		}
		strcpy(cur_ext, temp->name + dot_end+1);
		// 확장자가 이미 등록되어 있는가? => 새로운 확장자만 배열에 추가한다.
		for (int i = 0; i < extension_num; i++) {
			if (strcmp(cur_ext, file_extension[i]) == 0) {
				is_extension = 1;
				break;
			}
		}
		// 새로운 확장자만 저장한다.
		if (is_extension == 0)
			strcpy(file_extension[extension_num++], cur_ext);

		/* 다음 node로 이동 */
		temp = temp->next;
	}

	// [5] Step.2 중복된 파일명을 가진 node로 이루어진 linked list를 만든다.
	dNode* problemList = NULL;

	for (int i = 0; i < overlap_num; i++) {
		temp = head;
		while (temp != NULL) {
			if (strcmp(fileName_overlapped[i], temp->name) == 0)
				appendNode(&problemList, temp->name, temp->parentName, temp->parent_realpath,
						temp->current_type, temp->dir_depth, temp->file_size, temp->file_authority, temp->last_mtime, temp->last_ctime);
			temp = temp -> next;
		}
	}

	// [5] Step.3 problemList를 mode에 따라 어떤 파일이 삭제해야할 problem node인지 판단한다.
	dNode* scan = problemList;
	dNode* prev_scan = NULL;

	dNode* delete_head;

	int survive; // survive번 째 오는 경로는 살린다.
	for (int i = 0; i < overlap_num; i++) {
		count = 0;
		
		// Segmentation Fault 방지
		if (scan == NULL)
			break;

		prev_scan = scan;
		while (scan != NULL && strcmp(fileName_overlapped[i], scan->name) == 0) {
			count++;
			scan = scan->next;
		}

		switch (info->mode) {
			case 1:
				// 가장 최신 파일을 선택, 나머지 제외
				survive = 0;
				delete_head = prev_scan;
				time_t max_mtime = 0;
				
				for (int i = 0; i < count; i++) {
					if (prev_scan->last_mtime > max_mtime) {
						max_mtime = prev_scan->last_mtime;
						survive = i;
					}
					prev_scan = prev_scan->next;
				}

				for (int i = 0; i < count; i++) {
					if (i != survive) {
						delete_head = delete_head->next;
						continue;
					}
					// survive번 째에 있는 node만 삭제한다.
					deleteNode(&problemList, delete_head->name, delete_head->parentName, delete_head->parent_realpath, delete_head->current_type, delete_head->dir_depth);
					delete_head = delete_head->next;
				}
				break;
			case 2:
				// 가장 오래된 파일을 선택, 나머지 제외
				survive = 0;
				delete_head = prev_scan;
				time_t min_mtime = 0;

				for (int i = 0; i < count; i++) {
					if (prev_scan->last_mtime < min_mtime) {
						min_mtime = prev_scan->last_mtime;
						survive = i;
					}
					prev_scan = prev_scan->next;
				}

				for (int i = 0; i < count; i++) {
					if (i != survive) {
						delete_head = delete_head->next;
						continue;
					}
					// survive번 째에 있는 node만 삭제한다.
					deleteNode(&problemList, delete_head->name, delete_head->parentName, delete_head->parent_realpath, delete_head->current_type, delete_head->dir_depth);

					delete_head = delete_head->next;
				}
				break;
			case 3:
				// 파일명이 중복되는 경우 모두 제외 -> 아무것도 하지 않는다.
				break;
			default:
				break;
		}
	}

	// [5] Step.4 problemList와 head를 비교하며 문제가 되는 node를 삭제한다.
	scan = problemList;
	temp = head;

	while (scan != NULL) {
		deleteNode(&head, scan->name, scan->parentName, scan->parent_realpath, scan->current_type, scan->dir_depth);

		scan = scan->next;
	}
	freeList(&problemList);


	/* Arrange */
	// 작업 디렉토리를 <output_path> 경로로 옮기기
	if (chdir(info->output_path) < 0) {
		return -1;
	}
	// [0] - <extension> 디렉토리 만들기
	if (strcmp(info->extension, "all") != 0) {
		// not all
		char *temp_extension;
		int length;
		length = strlen(info->extension);
		temp_extension = (char *)malloc(length + 1);
		strcpy(temp_extension, info->extension);

		char *token = strtok(temp_extension, ","); // token 구하기
		while (token != NULL) {
			// 현재 작업 디렉토리에 <extension> directory가 없으면 만든다.
			if (CheckPathAccess(token) == -1 || CheckPathType(token) != 2) {
				if (mkdir(token, D_MODE) < 0)
					return -1;
			}

			token = strtok(NULL, ",");
		}
		free(temp_extension);
	}
	else {
		// all
		for (int i = 0; i < extension_num; i++) {
			if (CheckPathAccess(file_extension[i]) == -1 || CheckPathType(file_extension[i]) != 2) {
				if (mkdir(file_extension[i], D_MODE) < 0)
					return -1;
			}
		}
	}

	// [1] - 첫 번째로 실행하는 경우 : prev_head = NULL
	if (*prev_head == NULL) {
		// (prev_head와 head를 비교하지 않는다)
		arrange_dirList(head, info->monitoring_path, info->output_path, fp, info->pid, info->max_log_lines); // Arrange

		*prev_head = head;
		return 0;
	}

	// [2] - prev_head와 head가 각각 가리키는 list를 분석하여, 변화한 파일만 정리한다.
	int file_exist; // 파일의 존재 여부를 알려주는 flag

	dNode* prev_temp; // prev_head 탐색할 구조체 포인터
	dNode* creat_head = NULL; // 파일을 생성할 파일들만 모아둔 list를 가리키는 구조체 포인터 (전달할 head가 가리키는 list가 손상되면 안된다.)

	temp = head;
	while (temp != NULL) {
		file_exist = 0;
		prev_temp = *prev_head;
		// 같은 파일 찾기
		while (prev_temp != NULL) {
			if (strcmp(temp->name, prev_temp->name) == 0) {
				file_exist = 1;
				break;
			}
			prev_temp = prev_temp->next;
		}
		// 없던 파일은 새로 생긴 파일이기에, 추가해야 한다.
		if (file_exist == 0) {
			appendNode(&creat_head, temp->name, temp->parentName, temp->parent_realpath,
					temp->current_type, temp->dir_depth, temp->file_size, temp->file_authority, temp->last_mtime, temp->last_ctime);
		}
		// 있던 파일이면 mtime, ctime에 변화가 있어야만 추가한다.
		else {
			if (temp->last_mtime != prev_temp->last_mtime || temp->last_ctime != prev_temp->last_ctime)
				appendNode(&creat_head, temp->name, temp->parentName, temp->parent_realpath,
						temp->current_type, temp->dir_depth, temp->file_size, temp->file_authority, temp->last_mtime, temp->last_ctime);
		}
		temp = temp->next;
	}

	arrange_dirList(creat_head, info->monitoring_path, info->output_path, fp, info->pid, info->max_log_lines); // Arrange

	// prev_head 해제하고, head가 가리키는 list를 prev_head가 가리키도록 한다.
	freeList(prev_head);
	*prev_head = head;

	return 0;
}
// <monitoring_path>에서 파일을 읽고 <output_path>에서 파일 정리
int arrange_dirList(dNode* head, char* monitoring_path, char* output_path, FILE *fp, pid_t pid, char *max_log_lines) {
	int dot_idx;
	char dot = '.';

	char buf[BUFFER_SIZE];
	dNode* temp;

	temp = head;
	while (temp != NULL) {
		// 일반 파일만 취급한다.
		if (temp->current_type != 1) {
			temp = temp->next;
			continue;
		}

		dot_idx = 0;
		for (int i = 0; i < strlen(temp->name); i++) {
			if (temp->name[i] == dot)
				dot_idx = i;
		}

		// [0] - monitoring_path 이동
		if (chdir(monitoring_path) < 0)
			return -1;
		// [1] - 읽을 파일이 있는 부모 디렉토리로 이동
		if (chdir(temp->parent_realpath) < 0)
			return -1;
		// [2] - temp->name 파일 읽기
		FILE *fp1;
		if ((fp1 = fopen(temp->name, "r")) == NULL)
			return -1;
		// [3] - output_path로 이동
		if (chdir(output_path) < 0)
			return -1;
		// [4] - 확장자 디렉토리로 이동
		if (chdir(temp->name + dot_idx+1) < 0)
			return -1;
		// [5] - file_authority 문자열을 해독해, 새로운 temp->name 파일 open
		FILE *fp2;
		mode_t mode;
		if ((fp2 = fopen(temp->name, "w")) == NULL)
			return -1;
		if ((mode = str_to_mode(temp->file_authority)) == 0)
			return -1;
		if (chmod(temp->name, mode) < 0)
			return -1;

		// [6] - fp1이 가리키는 파일의 내용을 읽어 fp2가 가리키는 파일에 쓴다.
		size_t n;
		while ((n = fread(buf, 1, sizeof(buf), fp1)) > 0) {
			fwrite(buf, 1, n, fp2);
		}

		// [7] - 파일 포인터(디스크립터) 닫기
		fclose(fp1);
		fclose(fp2);
		
		// [8] - log 찍기
		if (chdir(monitoring_path))
			return -1;
		// floating_log를 하게 되었을 때, 기존 line이 max_log_lines를 초과하게되는 경우 freopen이 일어난다.
		// 위의 경우, fp는 함수 내부 변수이기에, 반영을 시켜야 한다. 뒤에다가 추가하는 로직이니 "a+" mode
		if ((fp = freopen(log_Filename, "a+", fp)) == NULL)
			return -1;
		floating_log(fp, pid, temp->parent_realpath, output_path, temp->name, max_log_lines);

		temp = temp->next;
	}

	return 0;
}
// 두 절대경로의 상하관계
int dir_relationship(char path1[PATH_MAX], char path2[PATH_MAX]) {
	int length1, length2;
	char delim = '/';

	length1 = strlen(path1);
	length2 = strlen(path2);

	// path1이 path2에 대해서..
	// path1 == path2/~ -> return 1    {상위}
	// path1 == path2   -> return 0    {동일}
	// path1/~ == path2 -> return -1   {하위}
	// others           -> return 999  {독립}
	//printf("path1 : %s | path2 : %s\n", path1, path2);
	if (length2 > length1 &&
			strncmp(path2, path1, length1) == 0 &&
			path2[length1] == delim) {
		//printf("<%s>는 DIR_PATH <%s>의 상위 디렉토리이다.\n", path1, path2);
		return 1;
	}
	else if (length2 == length1 &&
			strcmp(path2, path1) == 0) {
		//printf("<%s>는 DIR_PATH <%s>와 동일한 디렉토리이다.\n", path1, path2);
		return 0;
	}
	else if (length2 < length1 &&
			strncmp(path2, path1, length2) == 0 &&
			path1[length2] == delim) {
		//printf("<%s>는 DIR_PATH <%s>의 하위 디렉토리이다.\n", path1, path2);
		return -1;
	}

	return 999; // 쓰레기값 반환
}
// "rwxrwxrwx" 문자열을 mode_t 타입으로 변환하는 함수
mode_t str_to_mode(const char *perm_str) {
	if (strlen(perm_str) != 9)
		return 0;

	mode_t mode = 0;
	// Owner
	if (perm_str[0] == 'r') mode |= S_IRUSR;
	if (perm_str[1] == 'w') mode |= S_IWUSR;
	if (perm_str[2] == 'x') mode |= S_IXUSR;
	// Group
	if (perm_str[3] == 'r') mode |= S_IRGRP;
	if (perm_str[4] == 'w') mode |= S_IWGRP;
	if (perm_str[5] == 'x') mode |= S_IXGRP;
	// Others
	if (perm_str[6] == 'r') mode |= S_IROTH;
	if (perm_str[7] == 'w') mode |= S_IWOTH;
	if (perm_str[8] == 'x') mode |= S_IXOTH;

	return mode;
}
// 파일 전체에 락을 설정하는 함수
int lockfile(int fd) {
	struct flock fl;

	fl.l_type = F_WRLCK;
	fl.l_start = 0;
	fl.l_whence = SEEK_SET;
	fl.l_len = 0;
	return (fcntl(fd, F_SETLK, &fl));
}
// 파일의 락을 해제하는 함수
int unlockfile(int fd) {
	struct flock fl;

	fl.l_type = F_UNLCK;
	fl.l_start = 0;
	fl.l_whence = SEEK_SET;
	fl.l_len = 0;
	return (fcntl(fd, F_SETLK, &fl));
}
// ssu_cleanupd.config 파일 읽고 info에 저장하는 함수
int parse_config_file(struct ssu_daemon_config *info, FILE *fp) {
	// <중요> start_time은 문자열로 저장되어 있기에, time_t 타입으로 읽어올 수 없다.
	// start_time은 오로지 데몬 프로세스의 시작 시간을 눈으로 확인하기 위한 시간 계산 변수다.
	// 문자열 time_buf[64] 선언 후, 따로 읽어와야 한다.

	char line[STRING_MAX * 4];
	char value[STRING_MAX * 4];

	pid_t pid;
	time_t start_time;
	unsigned int time_interval;
	int mode;

	rewind(fp); // 처음부터 읽기

	while (fgets(line, sizeof(line), fp) != NULL) {
		if (sscanf(line, "monitoring_path : %s", value) == 1)
			strcpy(info->monitoring_path, value);
		else if (sscanf(line, "pid : %d", &pid) == 1)
			continue; // pid는 변동없다.
		else if (sscanf(line, "start_time : %ld", &start_time) == 1)
			continue; // start_time도 변동없다.
		else if (sscanf(line, "output_path : %s", value) == 1)
			strcpy(info->output_path, value);
		else if (sscanf(line, "time_interval : %d", &time_interval) == 1)
			info->time_interval = time_interval;
		else if (sscanf(line, "max_log_lines : %s", value) == 1)
			strcpy(info->max_log_lines, value);
		else if (sscanf(line, "exclude_path : %[^\n]", value) == 1)
			strcpy(info->exclude_path, value);
		else if (sscanf(line, "extension : %[^\n]", value) == 1)
			strcpy(info->extension, value);
		else if (sscanf(line, "mode : %d", &mode) == 1)
			info->mode = mode;
	}

	return 0;
}
// 파일의 현재 라인 수를 확인하는 함수
long count_lines(FILE *fp) {
	if (fp == NULL)
		return -1;

	long cur_offset = ftell(fp);
	rewind(fp);

	int lines = 0;
	int c;

	while ((c = fgetc(fp)) != EOF) {
		if (c == '\n')
			lines++;
	}

	fseek(fp, cur_offset, SEEK_SET);
	return lines;
}

// 로그 찍는 함수
int floating_log(FILE *fp, pid_t pid, char *parent_realpath, char *output_path, char *filename, char *max_log_lines) {
	// 이 함수의 동작이 끝난 후, 바뀐 fp에 대해 다시 freopen를 해서 fp를 갱신해야 한다. => "a+" mode로 할 것
	char time_str[16];
	char before_path[PATH_MAX];
	char after_path[PATH_MAX];

	time_t now = time(NULL);
	if (strcmp(max_log_lines, "none") != 0) { // max_log_lines가 설정된 경우에만 다음의 로직을 수행한다.
		long lines = count_lines(fp);
		char *endptr;
		long max_lines = strtol(max_log_lines, &endptr, 10);
		if (*endptr != '\0')
			return -1;

		if (lines >= max_lines) {
			char buffer[BUFFER_SIZE];
			FILE *new_fp = fopen("temp.log", "w");
			int n = 0;
			int skip = lines - max_lines;

			rewind(fp);
			while (fgets(buffer, sizeof(buffer), fp) != NULL) {
				if (n++ <= skip)
					continue; // lines수가 max_lines를 넘지 않도록 방지한다.
				fputs(buffer, new_fp);
			}
			fclose(new_fp);

			fp = freopen(log_Filename, "w", fp); // 기존 로그 파일에 덮어쓴다.

			new_fp = fopen("temp.log", "r");
			while (fgets(buffer, sizeof(buffer), new_fp) != NULL) {
				fputs(buffer, fp);
			}
			fclose(new_fp);
			remove("temp.log"); // 덮어쓰기 위해 썼던 임시 log파일 삭제
		}
	}

	strcpy(before_path, parent_realpath);
	strcpy(after_path, output_path);

	strcat(before_path, "/");
	strcat(after_path, "/");

	if (filename == NULL)
		return -1;

	char dot = '.';
	int dot_end = 0;
	for (int i = 0; i < strlen(filename); i++) {
		if (filename[i] == dot)
			dot_end = i;
	}
	strcat(after_path, filename + dot_end+1);
	strcat(after_path, "/");
	
	strcat(before_path, filename);
	strcat(after_path, filename);
	before_path[strlen(before_path)] = '\0';
	after_path[strlen(after_path)] = '\0';
	
	strftime(time_str, sizeof(time_str), "%H:%M:%S", localtime(&now));
	fprintf(fp, "[%s] [%d] [%s] [%s]\n", time_str, pid, before_path, after_path);
	
	fflush(fp); // 버퍼의 내용물을 강제로 디스크에 기록한다.

	return 0;
}
