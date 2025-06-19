#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
int open(const char *pathname, int oflag);
int open(const char *pathname, int oflag, mode_t mode);
// 성공 시 파일디스크립터, 에러 시 -1을 리턴하고 errno 설정 

#include <unistd.h>
int close(int filedes);
// 성공 시 0, 에러 시 -1을 리턴하고 errno 설정

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
int creat(const char *pathname, mode_t mode);
// 성공 시 쓰기 전용으로 열린 파일디스크립터, 에러 시 -1을 리턴하고 errno 설정

#include <sys/types.h>
#include <unistd.h>
off_t lseek(int filedes, off_t offset, int whence);
// 성공 시 새 파일 오프셋, 에러 시 -1을 리턴하고 errno 설정

#include <unistd.h>
ssize_t read(int filedes, void *buf, size_t nbytes);
// 읽은 바이트 수, 파일의 끝에 도달한 경우 0, 에러 시 -1을 리턴하고 errno 설정

#include <unistd.h>
ssize_t write(int filedes, void *buf, size_t nbytes);
// 성공 시 기록된 바이트 수, 에러 시 -1을 리턴하고 errno 설정

#include <unistd.h>
ssize_t pread(int filedes, void *buf, size_t nbytes, off_t offset);
// 성공 시 읽은 바이트 수, 파일의 끝에 도달하면 0, 요청한 바이트 수보다 적은 값이 리턴되더라도 에러가 이님, 에러 시 -1을 리턴하고 errno 설정

#include <unistd.h>
ssize_t pwrite(int filedes, void *buf, size_t nbytes, off_t offset);
// 성공 시 기록된 바이트 수, 쓰여야 할 바이트 수보다 적은 값이 리턴되더라도 에러가 아님, 에러 시 -1을 리턴하고 errno 설정

#include <unistd.h>
int dup(int filedes);
int dup2(int filedes, int filedes2);
// 성공 시 새로운 파일디스크립터, 에러 시 -1을 리턴하고 errno 설정

#include <unistd.h>
int fsync(int filedes);
int fdatasync(int filedes);
// 성공 시 0, 에러 시 -1을 리턴하고 errno 설정
void sync(void);
