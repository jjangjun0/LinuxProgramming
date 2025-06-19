#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
int stat(const char *restrict pathname, struct stat *restrict buf);
int fstat(int filedes, struct stat *buf);
int lstat(const char *restrict pathname, struct stat *restrict buf);
// 성공 시 0, 에러 시 -1을 리턴하고 errno 설정

#include <unistd.h>
int access(const char *pathname, int mode);
// 성공 시 0, 에러 시 -1을 리턴하고 errno 설정

#include <sys/types.h>
#include <sys/stat.h>
mode_t umask(mode_t cmask);
// 이전의 파일 모드 생성 마스크

#include <sys/stat.h>
int chmod(const char *pathname, mode_t mode);
int fchmod(int filedes, mode_t mode);
// 성공 시 0, 에러 시 -1을 리턴하고 errno 설정

#include <unistd.h>
int chown(const char *pathname, uid_t owner, gid_t group);
int fchown(int filedes, uid_t owner, gid_t group);
int lchwon(const char *pathname, uid_t owner, gid_t group);
// 성공 시 0, 에러 시 -1을 리턴하고 errno 설정

#include <unistd.h>
#include <sys/types.h>
int truncate(const char *pathname, off_t length);
int ftruncate(int filedes, off_t length);
// 성공 시 0, 에러 시 -1을 리턴하고 errno 설정

#include <unistd.h>
int link(const char *existingpath, const char *newpath);
int unlink(const char *pathname);
// 성공 시 0, 에러 시 -1을 리턴하고 errno 설정

#include <stdio.h>
int remove(const char *pathname);
int rename(const char *oldname, const char *newname);
// 성공 시 0, 에러 시 -1을 리턴하고 errno 설정


#include <unistd.h>
int symlink(const char *actualpath, const char *sympath);
// 성공 시 0, 에러 시 -1을 리턴하고 errno 설정

ssize_t readlink(const char *pathname, char *buf, size_t bufsize);
// 성공 시 읽은 바이트 수, 에러 시 -1을 리턴하고 errno 설정

#include <sys/types.h>
#include <utime.h>
int utime(const char *pathname, const struct utimbuf *times);
// 성공 시 0, 에러 시 -1을 리턴하고 errno 설정

#include <time.h>
int clock_gettime(clockid_t clk_id, struct timespec *tp);
// clk_id: 측정할 clock 종류 지정. (CLOCK_REALTIME, CLOCK_MONOTONIC 등)
// tp: 결과 저장할 timespec 구조체 포인터
// 경과 시간 측정용으로는 CLOCK_MONOTONIC 또는 CLOCK_MONOTONIC_RAW를 사용하는 것이 일반적이다.
// timespec.tv_nsec // 1초 = 1,000,000,000 나노초
// 성공 시 0, 에러 시 -1을 리턴하고 errno 설정

#include <sys/time.h>
int gettimeofday(struct timeval *tv, struct timezone *tz);
// *tv: 현재 시간 저장할 struct timeval 포인터
// tz: 일반적으로 NULL을 넣는다. 타임존 정보 저장용이다.
// timeval.tv_usec // 1초 = 1,000,000 마이크로초

#include <sys/stat.h>
#include <sys/types.h>
int mkdir(const char *pathname, mode_t mode);
// 성공 시 0, 에러 시 -1을 리턴하고 errno 설정

#include <unistd.h>
int rmdir(const char *pathname);
// 성공 시 0, 에러 시 -1을 리턴하고 errno 설정

#include <sys/types.h>
#include <dirent.h>
DIR *opendir(const char *name);
// 성공 시 포인터, 에러 시 NULL을 리턴하고 errno 설정

#include <dirent.h>
struct dirent *readdir(DIR *dp);
// 성공 시 포인터, 디렉토리의 끝일 경우 NULL, 에러 시 NULL을 리턴하고 errno 설정

#include <sys/types.h>
#include <dirent.h>
int rewinddir(DIR *dp);

#include <sys/types.h>
#include <dirent.h>
int closedir(DIR *dp);
// 성공 시 0, 에러 시 -1을 리턴하고 errno 설정

#include <dirent.h>
long telldir(DIR *dp);
// 성공 시 dp에 해당하는 디렉토리 안의 현재 위치, 에러 시 -1을 리턴하고 errno 설정

#include <dirent.h>
void seekdir(DIR *dp, long loc);

#include <dirent.h>
int scandir(const char *dirp, struct dirent ***namelist,
		int (*filter)(const struct dirent *), int (*compar)(const struct dirent **, const struct dirent **));
// 에러 시 -1을 리턴하고 errno 설정


#include <unistd.h>
int chdir(const char *pathname);
int fchdir(int filedes);
// 성공 시 0, 에러 시 -1을 리턴하고 errno 설정

#include <unistd.h>
char *getcwd(char *buf, size_t size);
char *get_current_dir_name(void);
// 성공 시 현재 작업 디렉토리의 pathname, 에러 시 NULL을 리턴하고 errno 설정
