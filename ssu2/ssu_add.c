#include "ssu_cleanupd.h"
#include "ssu_node.h"
#include "ssu_daemon_run.h"

int main(int argc, char *argv[])
{
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
	
	int fd;
	FILE *fp1, *fp2;

	struct ssu_daemon_config info_package; // daemon process에 보내고 받는 택배 구조체
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
	
	// ~/.ssu_cleanupd 이동
	if (chdir(dname) < 0) {
		fprintf(stderr, "[0] - chdir error for %s\n", dname);
		exit(1);
	}
	// ~/.ssu_cleanupd/current_daemon_list 파일 열어서 pid, real_path 쓰기
	if (CheckPathAccess(DMlist_Filename) < 0 || CheckPathType(DMlist_Filename) != 1) {
		fprintf(stderr, "%s is not exist\n", DMlist_Filename);
		exit(1);
	}
	else {
		if ((fd = open(DMlist_Filename, O_WRONLY | O_APPEND)) < 0) {
			fprintf(stderr, "open error for %s\n", DMlist_Filename);
			exit(1);
		}
		close(fd);
	}
	if (chdir("..") < 0) {
		fprintf(stderr, "[1] - chdir error for ..\n");
		exit(1);
	}
	
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
	
	//printf("ssu_add's option_factor_num: ");
	//for (int i = 0; i < 6; i++) printf("%d ", option_factor_num[i]);
	//printf("\n");

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
	//printf("flag : %d | %d | %d | %d | %d | %d\n", d_flag, i_flag, l_flag, x_flag, e_flag, m_flag);

	// 0. option이 없는데 인자가 2개 초과일 때 문법 에러 (나머지 문법 에러는 인자 분석하다가 걸린다.)
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

	// <DIR_PATH>의 부모 절대경로 구하기
	int temp_idx = 0;
	for (int i = 0; i < strlen(dir_realpath); i++) {
		if (dir_realpath[i] == delim)
			temp_idx = i;
	}
	strncpy(dir_parentPath, dir_realpath, temp_idx);
	dir_parentPath[temp_idx] = '\0';

	//printf("<DIR_PATH>'s real path : %s\n", dir_realpath);
	//printf("<DIR_PATH>'s parent path : %s\n", dir_parentPath);
	
	// <DIR_PATH>에 가서 ssu_cleanupd.config 와 ssu_cleanupd.log 파일 생성
	// 있으면 open만 한다.
	if (chdir(dir_realpath) < 0) {
		fprintf(stderr, "[2] - chdir error\n");
		exit(1);
	}
	// 파일 열기
	if (CheckPathAccess(config_Filename) < 0 ||
			CheckPathType(config_Filename) != 1) {
		if ((fp1 = fopen(config_Filename, "w")) == NULL) {
			fprintf(stderr, "<%s> file didn't created\n", config_Filename);
			exit(1);
		}
		fclose(fp1);
	}
	if (CheckPathAccess(log_Filename) < 0 ||
			CheckPathType(log_Filename) != 1) {
		if ((fp2 = fopen(log_Filename, "w")) == NULL) {
			fprintf(stderr, "<%s> file didn't created\n", log_Filename);
			exit(1);
		}
		fclose(fp2);
	}

	if (chdir("..") < 0) {
		fprintf(stderr, "[3] - chdir error\n");
		exit(1);
	}
	// "-d" 인자 실행 : <DIR_PATH>_arranged 만들기
	if (d_flag == 0) {
		char temp_dir[NAME_MAX]; // test/A 와 같이 경로의 형태라면 마지막 A -> 이름만 가져와야 한다.
		int start_idx = 0;
		for (int i = 0; i < strlen(dir_realpath); i++) {
			if (dir_realpath[i] == delim)
				start_idx = i;
		}
		strcpy(temp_dir, dir_realpath + start_idx+1);
		strcpy(dir_arrangedName, temp_dir);
		strcat(dir_arrangedName, "_arranged");
		// directory 이름이 255 bytes를 넘는 경우
		if (strlen(dir_arrangedName) > NAME_MAX) {
			fprintf(stderr, "<%s> is exceeded 255Bytes\n", dir_arrangedName);
			exit(1);
		}
		// 현재 작업 디렉토리에서 <DIR_PATH>_arranged 디렉토리가 없다면 생성
		if ((isAccess = CheckPathAccess(dir_arrangedName)) < 0 || (isDir = CheckPathType(dir_arrangedName)) != 2) {
			// d_flag == 0 인 경우
			if (mkdir(dir_arrangedName, D_MODE) < 0) {
				fprintf(stderr, "mkdir error for %s\n", dir_arrangedName);
				exit(1);
			}
		}
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
		int dir_relation;
		dir_relation = dir_relationship(temp_realpath, dir_realpath);
		// temp_realpath : <OUTPUT_PATH>
		// dir_realpath : <DIR_PATH>
		if (dir_relation == 1) {
			fprintf(stderr, "OUTPUT_PATH <%s>는 DIR_PATH <%s>의 상위 디렉토리이다.\n", temp_realpath, dir_realpath);
			exit(1);
		}
		else if (dir_relation == 0) {
			fprintf(stderr, "OUTPUT_PATH <%s>와 DIR_PATH <%s>는 동일한 디렉토리이다.\n", temp_realpath, dir_realpath);
			exit(1);
		}
		else if (dir_relation == -1) {
			fprintf(stderr, "OUTPUT_PATH <%s>는 DIR_PATH <%s>의 하위 디렉토리이다.\n", temp_realpath, dir_realpath);
			exit(1);
		}
	}
	if (!realpath(dir_arrangedName, output_realpath)) {
		fprintf(stderr, "realpath error for %s\n", dir_arrangedName);
		exit(1);
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
		int length;
		temp_idx = (idx + 1) + optFactor_jumpIdx(argc, argv, "-l", option_factor_num);

		if (argv[temp_idx] == NULL)
			exit(1);

		length = strlen(argv[temp_idx]);
		length++; // 끝에 null
		max_log_lines = (char *)malloc(length);
		if (max_log_lines == NULL) {
			fprintf(stderr, "malloc error for max_log_lines\n");
			exit(1);
		}
		max_log_lines[0] = '\0';
		strcpy(max_log_lines, argv[temp_idx]);
	}
	// ssu_cleanupd 프로그램을 실행시킨 작업 디렉토리 기준으로 "-x" 인자의 상대경로를 파악한다.
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

		int dir_relation;
		for (int i = 0; i < x_num; i++) {
			if (i == x_num-1) {
				strcat(exclude_path, x_path[i]);

				break; // j = i+1 => Segmentation Fault 방지
			}

			// -x 인자들끼리 (상위, 동일한, 하위 디렉토리) 경로인 경우 에러다.
			for (int j = 0; j < i; j++) {
				dir_relation = dir_relationship(x_path[i], x_path[j]);
				if (dir_relation == 1) {
					fprintf(stderr, "<%s>는 <%s>의 상위 디렉토리이다.\n", x_path[i], x_path[j]);
					exit(1);
				}
				else if (dir_relation == 0) {
					fprintf(stderr, "<%s>와 <%s>는 동일한 디렉토리이다.\n", x_path[i], x_path[j]);
					exit(1);
				}
				else if (dir_relation == -1) {
					fprintf(stderr, "<%s>는 <%s>의 하위 디렉토리이다.\n", x_path[i], x_path[j]);
					exit(1);
				}
			}

			strcat(exclude_path, x_path[i]); // 복사
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
	strcpy(info_package.monitoring_path, dir_realpath);
	strcpy(info_package.output_path, output_realpath);
	info_package.time_interval = time_interval;
	strcpy(info_package.max_log_lines, max_log_lines);
	strcpy(info_package.exclude_path, exclude_path);
	strcpy(info_package.extension, extension);
	info_package.mode = mode;

	// 동적할당 해제
	if (l_flag == 1)
		free(max_log_lines);
	if (x_flag == 1)
		free(exclude_path);
	if (e_flag == 1)
		free(extension);

	// 중간 프로세스 생성
	pid_t middle;
	if ((middle = fork()) < 0) {
		fprintf(stderr, "fork error\n");
		exit(1);
	}
	else if (middle == 0) {
		// 중간 프로세스
		if (chdir(origin_wd) < 0) {
			fprintf(stderr, "chdir error for %s\n", origin_wd);
			exit(1);
		}

		// daemon process 실행을 위해 ssu_daemon_init 함수 call
		if (ssu_daemon_init(&info_package) < 0) {
			fprintf(stderr, "ssu_daemon_init error\n");
			exit(1);
		}

		// 중간 프로세스 종료 => ssu_daemon_init에서 생성하는 프로세스가 데몬 프로세스가 된다.
		//exit(0);
	}
	else {
		// 여기서 middle process wait 해야할 거 같은데
		int status;
		if (wait(&status) != middle) {
			fprintf(stderr, "wait error\n");
			exit(1);
		}
	}

	exit(0);
}
