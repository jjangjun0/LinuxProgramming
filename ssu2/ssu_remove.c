#include "ssu_cleanupd.h"

int main(int argc, char *argv[])
{
	char dir_realpath[PATH_MAX];
	int idx = 1;
	int isDir, isHome;
	int origin_fd, new_fd;

	if (argc != 2)
		exit(ERR_GRAMMER);

	// 1. <DIR_PATH>의 길이가 PATH_MAX를 넘기는가?
	if (strlen(argv[idx]) > PATH_MAX) {
		fprintf(stderr, "<DIR_PATH> length is less then 4096 Bytes\n");
		exit(1);
	}
	// 2. <DIR_PATH>에 접근할 수 있는가?
	if (!realpath(argv[idx], dir_realpath)) {
		fprintf(stderr, "realpath error for %s\n", argv[idx]);
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
	// 4. <DIR_PATH>가 사용자의 홈 디렉토리를 벗어나는가?
	if ((isHome = CheckPath_InsideHome(dir_realpath)) == -1) {
		exit(1);
	}
	else if (isHome == 0) {
		fprintf(stderr, "<%s> is outside the home directory\n", argv[idx]);
		exit(1);
	}

	//printf("<DIR_PATH>'s real path : %s\n", dir_realpath);

	// ~/.ssu_cleanupd/current_daemon_list 파일 읽기
	if (chdir(dname) < 0) {
		fprintf(stderr, "chdir error for %s\n", dname);
		exit(1);
	}
	if ((origin_fd = open(DMlist_Filename, O_RDONLY)) < 0) {
		fprintf(stderr, "open error for %s\n", DMlist_Filename);
		exit(1);
	}
	if ((new_fd = open("temp_list", O_WRONLY | O_CREAT | O_TRUNC, S_MODE)) < 0) {
		fprintf(stderr, "open error for temp_list\n");
		exit(1);
	}
	
	struct ssu_daemon record;
	int isDaemon = 0;
	pid_t pid; // kill할 pid
	while (read(origin_fd, &record, sizeof(record)) > 0) {
		if (strcmp(record.real_path, dir_realpath) != 0)
			write(new_fd, &record, sizeof(record));
		else {
			pid = record.pid;
			isDaemon = 1; // monitoring 경로가 존재한다면 Daemon process가 존재하는 것이다.
		}
	}
	close(origin_fd);
	close(new_fd);

	// <DIR_PATH> 경로를 monitoring 하는 daemon process가 없는 경우
	if (isDaemon == 0) {
		fprintf(stderr, "There is no daemon process to monitor <%s>\n", argv[idx]);

		remove("temp_list");
		chdir("..");
		exit(1);
	}

	// 원래 current_daemon_list 파일 삭제
	if (remove(DMlist_Filename) < 0) {
		fprintf(stderr, "remove error for %s\n", DMlist_Filename);
		exit(1);
	}
	// temp_list를 current_daemon_list 파일로 이름을 바꾼다.
	if(rename("temp_list", DMlist_Filename) < 0) {
		fprintf(stderr, "rename error\n");
		exit(1);
	}

	// kill daemon process
	kill(pid, SIGKILL);
	
	// monitoring 하는 경로로 이동
	if (chdir(dir_realpath) < 0) {
		fprintf(stderr, "chdir error for %s\n", dir_realpath);
		exit(1);
	}
	// ssu_cleanupd.config, ssu_cleanupd.log 파일 삭제
	if (remove(config_Filename) < 0) {
		fprintf(stderr, "remove error for %s\n", config_Filename);
		exit(1);
	}
	if (remove(log_Filename) < 0) {
		fprintf(stderr, "remove error for %s\n", log_Filename);
		exit(1);
	}

	exit(0);
}
