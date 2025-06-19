#include "ssu_cleanupd.h"
#include "ssu_node.h"
#include "ssu_daemon_run.h"

int main(int argc, char *argv[])
{

	/* ssu_add.c와 동작이 유사하나, daemon process를 실행시키지 않고, ssu_cleanupd.config만 바꾼다. */

	if (argc < 3) {
		// argv[0] = "add", argv[2] = "0 0 0 0 0 0"
		exit(ERR_GRAMMER);
	}
	char origin_wd[PATH_MAX];
	if (getcwd(origin_wd, sizeof(origin_wd)) == NULL) {
		fprintf(stderr, "getcwd error\n");
		exit(1);
	}

	int option_factor_num[6] = { 0 };
	char *option_str = argv[argc - 1]; // argv의 마지막에 있는 option_factor_num_string 을 받아온다.
	char *token;
	int option_value; // option의 유무를 파악할 때 쓰인다.
	int idx = 0; // option_factor_num 복원할 때 쓰인 후, <DIR_PATH>의 index 값이 된다.
	char dir_realpath[PATH_MAX]; // <DIR_PATH> 의 절대경로
	char dir_parentPath[PATH_MAX]; // <DIR_PATH>의 부모 절대경로
	char output_realpath[PATH_MAX]; // output의 절대 경로
	int isAccess; // CheckPathAccess 리턴값을 저장하는 변수
	int isDir; // CheckPathType 리턴값을 저장하는 변수
	int isHome; // CheckPath_InsideHome 리턴값을 저장하는 변수
	

	struct ssu_daemon_config info; // daemon process에 보내고 받는 택배 구조체
	/* ssu_cleanupd.config에 적힐 변수들 */
	// argv[idx] => monitoring_path
	// pid
	// start_time 구하기
	char dir_arrangedName[PATH_MAX]; // <DIR_PATH>_arranged 이름 => output_path
	unsigned int time_interval; // => time_interval
	char *max_log_lines = NULL; // => max_log_lines
	char *exclude_path = NULL; // => exclude_path
	char *extension = NULL; // => extension
	int mode; // => mode
	

	int d_flag, i_flag, l_flag, x_flag, e_flag, m_flag; // 인자들의 flag
	int position1; // 1의 자리
	int count = 0;
	d_flag = i_flag = l_flag = x_flag = e_flag = m_flag = 0;
	
	// option_factor_num을 복원한다.
	token = strtok(option_str, " ");
	while (token != NULL && idx < 6) {
		option_factor_num[idx++] = atoi(token);
		token = strtok(NULL, " ");
	}
	argv[argc - 1] = (char *)0;
	argc--;

	// option factor의 개수 조건에 대한 예외 처리 : -d, -i, -l 은 옵션의 인자가 2개 이상 올 수 없다.
	if (option_factor_num[0] > 1 || option_factor_num[1] > 1 || option_factor_num[2] > 1)
		exit(ERR_GRAMMER);

	// option_value 만들기
	option_value = make_optionValue(argc, argv);
	// flag 활성화
	while (option_value != 0) {
		position1 = option_value % 10;
		if (position1 == 1) {
			switch (count) {
				case 0:
					d_flag = 1;
					break;
				case 1:
					i_flag = 1;
					break;
				case 2:
					l_flag = 1;
					break;
				case 3:
					x_flag = 1;
					break;
				case 4:
					e_flag = 1;
					break;
				case 5:
					m_flag = 1;
					break;
				default:
					break;
			}
		}
		option_value /= 10;
		count++;
	}

	// 0. option이 없는데 인자가 2개 초과일 때 문법 에러
	if (argc > 2 &&
			d_flag == 0 &&
			i_flag == 0 &&
			l_flag == 0 &&
			x_flag == 0 &&
			e_flag == 0 &&
			m_flag == 0)
	{
		exit(ERR_GRAMMER);
	}

	// <DIR_PATH>의 index 찾기
	idx = CheckDirIndex(argc, argv);

	// 1. <DIR_PATH> 길이가 PATH_MAX를 넘겼는가?
	if (strlen(argv[idx]) > PATH_MAX) {
		fprintf(stderr, "<DIR_PATH> length is less then 4096 Bytes\n");
		exit(1);
	}
	// 2. <DIR_PATH>에 접근할 수 있는가?
	if (!realpath(argv[idx], dir_realpath)) {
		fprintf(stderr, "realpath error for %s\n", argv[idx]);
		exit(1);
	}
	if (CheckPathAccess(dir_realpath) < 0) {
		fprintf(stderr, "access error for %s\n", argv[idx]);
		exit(1);
	}
	// 3. <DIR_PATH>는 directory인가?
	if ((isDir = CheckPathType(dir_realpath)) < 0) {
		exit(1);
	}
	else if (isDir != 2) {
		fprintf(stderr, "%s is not directory\n", argv[idx]);
		exit(ERR_GRAMMER);
	}
	// 4. <DIR_PATH>가 사용자의 홈 디렉토리($HOME, ~)를 벗어나는가?
	if ((isHome = CheckPath_InsideHome(dir_realpath)) == -1) {
		exit(1);
	}
	else if (isHome == 0) {
		fprintf(stderr, "<%s> is outside the home directory\n", argv[idx]);
		exit(1);
	}

	// ~/.ssu_cleanupd/current_daemon_list 파일에서 <DIR_PATH>가 등록되어 있는지 확인
	int fd1;
	int isRegistered = 0;
	struct ssu_daemon record;

	if (chdir(dname) < 0) {
		fprintf(stderr, "chdir error for %s\n", dname);
		exit(1);
	}
	if ((fd1 = open(DMlist_Filename, O_RDONLY)) < 0) {
		fprintf(stderr, "open error for %s\n", DMlist_Filename);
		exit(1);
	}
	if (chdir("..") < 0) {
		fprintf(stderr, "chdir error for ..\n");
		exit(1);
	}

	while (read(fd1, &record, sizeof(record)) > 0) {
		if (strcmp(record.real_path, dir_realpath) == 0) {
			isRegistered = 1;
			break;
		}
	}
	if (isRegistered == 0) {
		fprintf(stderr, "<%s>는 등록되지 않은 모니터링 경로입니다.\n", dir_realpath);
		exit(1);
	}
	close(fd1);

	// <DIR_PATH>의 부모 절대경로 구하기
	int temp_idx = 0;
	for (int i = 0; i < strlen(dir_realpath); i++) {
		if (dir_realpath[i] == delim)
			temp_idx = i;
	}
	strncpy(dir_parentPath, dir_realpath, temp_idx);
	dir_parentPath[temp_idx] = '\0';
	
	// "-d" 인자 실행 : 임시파일 만들기
	if (d_flag == 0) {
		strcpy(dir_arrangedName, "noName");
	}
	else if (d_flag == 1) {
		int temp_idx = 0;
		// <DIR_PATH> index 그 다음 부터 jump한다.
		temp_idx = (idx + 1) + optFactor_jumpIdx(argc, argv, "-d", option_factor_num);
		
		if (argv[temp_idx] == NULL)
			exit(1);

		//printf("idx: %d | temp_idx : %d\n", idx, temp_idx);
		//printf("argv[temp_idx] : %s\n", argv[temp_idx]);

		strcpy(dir_arrangedName, argv[temp_idx]);
		
		// -d 옵션으로 <OUTPUT_PATH>를 지정하는 경우
		// 현재 작업 디렉토리를 기준으로 경로를 받았기에, 복구시켜야 한다.
		if (chdir(origin_wd) < 0) {
			fprintf(stderr, "chdir error for %s\n", origin_wd);
			exit(1);
		}
		isAccess = CheckPathAccess(dir_arrangedName);
		isDir = CheckPathType(dir_arrangedName);

		// 1. -d 옵션의 인자에 대한 접근 권한 판단
		if (isAccess < 0) {
			fprintf(stderr, "<%s>는 존재하지 않습니다.\n", dir_arrangedName);
			exit(1);
		}
		if (isDir != 2) {
			fprintf(stderr, "<%s>는 디렉토리 파일이 아닙니다.\n", dir_arrangedName);
			exit(1);
		}
		if (isAccess != 1 &&
				isAccess != 3 &&
				isAccess != 5 &&
				isAccess != 7) {
			// X_OK가 보장되어야 한다.
			fprintf(stderr, "<%s>에 대해 접근권한이 없습니다.\n", dir_arrangedName);
			exit(1);
		}
		// 2. -d 옵션 인자가 홈 디렉토리에 속하는가?
		char temp_realpath[PATH_MAX];
		int d_isHome;
		if (!realpath(dir_arrangedName, temp_realpath)) {
			fprintf(stderr, "realpath error for %s\n", dir_arrangedName);
			exit(1);
		}
		//printf("dir_arrangedName의 절대 경로 : %s\n", temp_realpath);
		if ((d_isHome = CheckPath_InsideHome(temp_realpath)) == -1)
			exit(1);
		else if (d_isHome == 0) {
			fprintf(stderr, "<%s> is outside the home directory\n", dir_arrangedName);
			exit(1);
		}
		// 3. -d 옵션 인자가 <DIR_PATH>에 대해 상위/하위 디렉토리인가?
		// dir_realpath : <DIR_PATH>
		// temp_realpath : <OUTPUT_PATH>
		int length1, length2;
		length1 = strlen(dir_realpath);
		length2 = strlen(temp_realpath);

		//printf("%d | %d\n", length1, length2);
		//printf("%s | %s\n", dir_realpath, temp_realpath);
		if (length2 > length1 &&
				strncmp(dir_realpath, temp_realpath, length1) == 0 &&
				temp_realpath[length1] == delim) {
			fprintf(stderr, "<%s>는 DIR_PATH <%s>의 하위 디렉토리이다.\n", temp_realpath, dir_realpath);
			exit(1);
		}
		else if (length2 == length1 &&
				strcmp(dir_realpath, temp_realpath) == 0) {
			fprintf(stderr, "<%s>는 DIR_PATH <%s>와 동일한 디렉토리이다.\n", temp_realpath, dir_realpath);
			exit(1);
		}
		else if (length2 < length1 &&
				strncmp(dir_realpath, temp_realpath, length2) == 0 &&
				dir_realpath[length2] == delim) {
			fprintf(stderr, "<%s>는 DIR_PATH <%s>의 상위 디렉토리이다.\n", temp_realpath, dir_realpath);
			exit(1);
		}
		// modify는 -d 인자가 있는 경우에만 절대 경로를 구한다.
		if (!realpath(dir_arrangedName, output_realpath)) {
			fprintf(stderr, "realpath error for %s\n", dir_arrangedName);
			exit(1);
		}
	}
	// "-i" 인자 실행 : time_interval 설정
	if (i_flag == 0) {
		time_interval = 10;
	}
	else if (i_flag == 1) {
		int temp_idx = 0;	
		temp_idx = (idx + 1) + optFactor_jumpIdx(argc, argv, "-i", option_factor_num);

		if (argv[temp_idx] == NULL)
			exit(1);

		time_interval = atoi(argv[temp_idx]);
	}
	// "-l" 인자 실행 : max_log_lines 설정
	if (l_flag == 0) {
		max_log_lines = "none";
	}
	else if (l_flag == 1) {
		int temp_idx = 0;
		temp_idx = (idx + 1) + optFactor_jumpIdx(argc, argv, "-l", option_factor_num);

		if (argv[temp_idx] == NULL)
			exit(1);

		max_log_lines = (char *)malloc(strlen(argv[temp_idx]) + 1);
		strcpy(max_log_lines, argv[temp_idx]);
	}
	// ssu_cleanupd 프로그램을 실행시킨 작업 디렉토리 기준으로 -x인자의 상대경로를 파악한다.
	if (chdir(origin_wd) < 0) {
		fprintf(stderr, "chdir error for %s\n", origin_wd);
		exit(1);
	}
	// "-x" 인자 실행 :
	if (x_flag == 0) {
		exclude_path = "none";
	}
	else if (x_flag == 1) {
		int temp_idx = 0;
		int length = 0;
		int iteration_flag;
		int x_num = option_factor_num[3];
		char x_path[x_num][PATH_MAX];
		temp_idx = (idx + 1) + optFactor_jumpIdx(argc, argv, "-x", option_factor_num);

		if (argv[temp_idx] == NULL)
			exit(1);

		// 절대 경로 만들기
		for (int i = 0; i < x_num; i++) {
			if (!realpath(argv[temp_idx + i], x_path[i])) {
				fprintf(stderr, "realpath error for %s\n", argv[temp_idx + i]);
				exit(1);
			}
		}
		// A B C => A의 길이, B의 길이, ...
		for (int i = 0; i < x_num; i++) {
			length += strlen(x_path[i]);
		}
		// A,B,C => ','의 갯수
		length = length + (x_num - 1);
		// 끝에 null
		length++;

		exclude_path = (char *)malloc(length);
		if (exclude_path == NULL) {
			fprintf(stderr, "malloc error for exclude_path\n");
			exit(1);
		}
		exclude_path[0] = '\0';

		for (int i = 0; i < x_num; i++) {
			iteration_flag = 0;
			// 반복된 input이 있으면 pass
			for (int j = 0; j < i; j++) {
				if (strcmp(x_path[i], x_path[j]) == 0) {
					iteration_flag = 1;
					break;
				}
			}
			if (iteration_flag == 1)
				continue;

			strcat(exclude_path, x_path[i]); // 복사

			if (i != x_num - 1)
				strcat(exclude_path, ","); // 복사되었으면 ','으로 구분한다.
		}
		if (exclude_path[strlen(exclude_path)-1] == ',')
			exclude_path[strlen(exclude_path)-1] = '\0';
	}
	// "-e" 인자 실행 :
	if (e_flag == 0) {
		extension = "all";
	}
	else if (e_flag == 1) {
		int temp_idx = 0;
		int length = 0;
		int isDuplicated = 0;
		temp_idx = (idx + 1) + optFactor_jumpIdx(argc, argv, "-e", option_factor_num);

		if (argv[temp_idx] == NULL)
			exit(1);

		for (int i = 0; i < option_factor_num[4]; i++)
			length += strlen(argv[temp_idx + i]);
		length = length + (option_factor_num[4] - 1);
		length++;

		extension = (char *)malloc(length);
		if (extension == NULL) {
			fprintf(stderr, "malloc error for log\n");
			exit(1);
		}
		extension[0] = '\0';

		for (int i = 0; i < option_factor_num[4]; i++) {
			isDuplicated = 0;
			for (int j = 0; j < i; j++) {
				if (strcmp(argv[temp_idx + i], argv[temp_idx + j]) == 0) {
					isDuplicated = 1;
					break;
				}
			}
			// 중복된 확장자가 아닌 경우
			if (isDuplicated == 0) {
				strcat(extension, argv[temp_idx + i]);

				if (i != option_factor_num[4] - 1)
					strcat(extension, ",");
			}
		}
	}

	// "-m" 인자 실행 :
	if (m_flag == 0) {
		mode = 1;
	}
	else if (m_flag == 1) {
		int temp_idx = 0;

		temp_idx = (idx + 1) + optFactor_jumpIdx(argc, argv, "-m", option_factor_num);

		if (argv[temp_idx] == NULL)
			exit(1);
		mode = atoi(argv[temp_idx]);
		if (mode > 3 || mode <= 0) {
			fprintf(stderr, "The value is out of range[1 ~ 3]\n");
			exit(1);
		}
	}
	/*
	printf("\n");
	printf("monitoring_path : %s\n", dir_realpath);
	printf("output_path : %s\n", output_realpath);
	printf("time_interval: %d\n", time_interval);
	printf("max_log_lines: %s\n", max_log_lines);
	printf("exclude_path: %s\n", exclude_path);
	printf("extension : %s\n", extension);
	printf("mode : %d\n", mode);
	printf("\n");
	*/
	// pid, start_time을 제외한 정보를 택배에 담는다. (pid랑 start_time은 daemon process 생성 후에 저장)
	if (d_flag == 1)
		strcpy(info.monitoring_path, dir_realpath);
	strcpy(info.output_path, output_realpath);
	info.time_interval = time_interval;
	strcpy(info.max_log_lines, max_log_lines);
	strcpy(info.exclude_path, exclude_path);
	strcpy(info.extension, extension);
	info.mode = mode;

	// <DIR_PATH>에 가서 ssu_cleanupd.config 파일 open한다.
	if (chdir(dir_realpath) < 0) {
		fprintf(stderr, "chdir error for %s\n", dir_realpath);
		exit(1);
	}

	int fd;
	FILE *fp;
	if ((fd = open(config_Filename, O_RDWR | O_CREAT, S_MODE)) < 0) {
		fprintf(stderr, "open error for %s\n", config_Filename);
		exit(1);
	}

	// lock 시도
	while (1) {
		if (lockfile(fd) == 0)
			break;
		if (errno == EACCES || errno == EAGAIN)
			usleep(100000);
		else {
			fprintf(stderr, "lock error\n");
			close(fd);
			exit(1);
		}
	}

	// 기존 pid, start_time을 가져오기 위해 읽는다.
	fp = fdopen(fd, "r+");
	if (fp == NULL) {
		fprintf(stderr, "fdopen error\n");
		close(fd);
		exit(1);
	}

	char line[STRING_MAX * 4];
	char value[STRING_MAX * 4];
	char time_buf[64];

	rewind(fp);
	// monitoring_path는 무결성, pid는 info->pid에 저장, start_time은 time_buf에 저장
	while (fgets(line, sizeof(line), fp) != NULL) {
		if (sscanf(line, "pid : %d", &info.pid) == 1)
			continue;
		else if (sscanf(line, "start_time : %[^\n]", time_buf) == 1)
			continue;

		// modify 명령어 수행에서 입력되지 않은 옵션은 수정하지 않는다.
		// 기존의 config 파일의 내용으로 바꾼다.
		if (d_flag == 0 && sscanf(line, "output_path : %s", value) == 1)
			strcpy(info.output_path, value);
		else if (i_flag == 0 && sscanf(line, "time_interval : %d", &info.time_interval) == 1)
			continue;
		else if (l_flag == 0 && sscanf(line, "max_log_lines : %s", value) == 1)
			strcpy(info.max_log_lines, value);
		else if (x_flag == 0 && sscanf(line, "exclude_path : %s", value) == 1)
			strcpy(info.exclude_path, value);
		else if (e_flag == 0 && sscanf(line, "extension : %s", value) == 1)
			strcpy(info.extension, value);
		else if (m_flag == 0 && sscanf(line, "mode : %d", &info.mode) == 1)
			continue;
	}

	rewind(fp);
	ftruncate(fd, 0); // 기존 내용 삭제

	// 파일에다가 쓰기
	fprintf(fp, "monitoring_path : %s\n", info.monitoring_path);
	fprintf(fp, "pid : %d\n", info.pid);
	fprintf(fp, "start_time : %s\n", time_buf); // 파일에는 start_time을 문자열로 저장
	fprintf(fp, "output_path : %s\n", info.output_path);
	fprintf(fp, "time_interval : %d\n", info.time_interval);
	fprintf(fp, "max_log_lines : %s\n", info.max_log_lines);
	fprintf(fp, "exclude_path : %s\n", info.exclude_path);
	fprintf(fp, "extension : %s\n", info.extension);
	fprintf(fp, "mode : %d\n", info.mode);

	fflush(fp);
	// lock 해제
	unlockfile(fd);

	// close
	close(fd);
	fclose(fp);

	exit(0);
}
