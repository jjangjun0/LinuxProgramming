#include <stdlib.h>
void exit(int status);
void _Exit(int status);

#include <unistd.h>
void _exit(int status);

#include <stdlib.h>
int atexit(void (*__func__) (void));
int on_exit(void (*function)(int, void *), void *arg);
// 성공 시 0, 실패 시 0 이외의 값 리턴

int main(int argc, char *argv[]);

#include <stdlib.h>
void *malloc(size_t size);
void *calloc(size_t nobj, size_t size);
void *realloc(void *ptr, size_t newsize);
// 성공 시 할당된 메모리를 가리키는 포인터, 에러 시 NULL 리턴
void free(void *ptr);

#include <stdlib.h>
char *getenv(const char *name);
// 성공 시 name으로 주어진 이름에 해당하는 환경 변수 값을 가리키는 포인터,
// 그런 이름이 없는 경우 NULL 리턴
int putenv(char *str);
// 성공 시 0, 에러 시 0이 아닌 값을 리턴하고 errno 설정
int setenv(const char *name, const char *value, int rewrite);
int unsetenv(const char *name);
// 성공 시 0, 에러 시 -1을 리턴하고 errno 설정

#include <setjmp.h>
int setjmp(jmp_buf env);
// 직접 호출된 경우에는 0, longjmp를 통해서 호출된 경우에는 0이 아닌 값 리턴
void longjmp(jmp_buf env, int val);

#include <sys/resource.h>
#include <sys/time.h>
int getrlimit(int resource, struct rlimit *rlptr);
int setrlimit(int resource, const struct rlimit *rlptr);
// 성공 시 0, 에러 시 -1을 리턴하고 errno 설정

#include <unistd.h>
#include <sys/types.h>
pid_t getpid(void);
// 리턴 값: 호출한 프로세스의 프로세스 ID
pid_t getppid(void);
// 리턴 값: 호출한 프로세스의 부모 프로세스 ID
uid_t getuid(void);
// 리턴 값: 호출한 프로세스의 실제 사용자 ID
uid_t geteuid(void);
// 리턴 값: 호출한 프로세스의 유효 사용자 ID
gid_t getgid(void);
// 리턴 값: 호출한 프로세스의 실제 그룹 ID
gid_t getegid(void);
// 리턴 값: 호출한 프로세스의 유효 그룹 ID


#include <unistd.h>
pid_t fork(void);
// 자식 프로세스의 경우 0,
// 부모 프로세스의 경우 자식의 pid,
// 에러 시 -1을 리턴하고 errno 설정

#include <unistd.h>
#include <sys/types.h>
pid_d vfork(void);
// 자식 프로세스의 경우 0,
// 부모 프로세스의 경우 자식의 프로세스 ID
// 에러 시 -1을 리턴하고 errno 설정

#include <sys/types.h>
#include <sys/wait.h>
pid_t wait(int *statloc);
pid_t waitpid(pid_t pid, int *statloc, int options);
// 성공 시 프로세스 ID, 에러 시 -1
// (waitpid()의 경우, WNOHANG 옵션으로 실행되었고,
//  자식 프로세스가 종료되지 않았을 때 0을 리턴한다.)

#include <sys/types.h>
#include <sys/wait.h>
int waitid(idtype_t idtype, id_t id, siginfo_t *infop, int options);
// 성공 시 0, WNOHANG 옵션이 있는 상태에서
// 자식 프로세스가 종료되지 않았을 경우 0, 에러 시 -1을 리턴

#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
pid_t wait3(int *statloc, int options, struct rusage *rusage);
pid_t wait4(pid_t pid, int *statloc, int options, struct rusage *rusage);
// 성공 시 프로세스 ID, 에러 시 -1,
// WNOHANG 옵션으로 실행되었고, 자식 프로세스가 종료되지 않았을 때 0 리턴

#include <unistd.h>
int execl(const char *pathname, const char *arg0, ... /* (char *)0 */ );
int execv(const char *pathname, char *const argv []);
int execle(const char *pathname, const char *arg0, ... /* (char *)0, char *const envp[] */);
int execve(const char *pathname, char *const argv [], char *const envp[]);
int execlp(const char *filename, const char *arg0, ... /* (char *)0 */ );
int execvp(const char *filename, char *const argv[]);
// 성공 시 리턴하지 않고, 에러 시 -1을 리턴하고 errno 설정

#include <unistd.h>
#include <sys/types.h>
int setuid(uid_t uid);
int setgid(gid_t gid);
// 성공 시 0, 에러 시 -1을 리턴하고 errno 설정

#include <unistd.h>
#include <sys/types.h>
int setreuid(uid_t ruid, uid_t euid);
int setregid(gid_t rgid, gid_t egid);
// 성공 시 0, 에러 시 -1을 리턴하고 errno 설정

#include <unistd.h>
#include <sys/types.h>
int seteuid(uid_t uid);
int setegid(gid_t gid);
// 성공 시 0, 에러 시 -1을 리턴하고 errno 설정

#include <stdlib.h>
int system(const char *cmdstring);

#include <unistd.h>
char *getlogin(void);
// 성공 시 로그인 이름을 담은 문자열을 가리키는 포인터,
// 에러 시 NULL을 리턴하고 errno 설정

#include <sys/times.h>
clock_t times(struct tms *buf);
// 성공 시 소비된 클록 시간 값(클록 틱 수),
// 에러 시 -1을 리턴하고 errno 설정

#include <unistd.h>
pid_t getpgrp(void);
pid_t getpgid(pid_t pid);
// 성공 시 프로세스 그룹 ID, 에러 시 -1을 리턴하고 errno 설정

int setpgid(pid_t pid, pid_t pgid);
// 성공 시 0, 에러 시 -1을 리턴하고 errno 설정

#include <unistd.h>
pid_t setsid(void);
// 성공 시 프로세스 그룹 ID, 에러 시 -1을 리턴하고 errno 설정
pid_t getsid(pid_t pid);
// 성공 시 pid의 세션 리더의 ID, 에러 시 -1을 리턴하고 errno 설정

#include <unistd.h>
pid_t tcgetpgrp(int filedes);
// 성공 시 전경 프로세스 그룹의 프로세스 그룹 ID,
// 에러 시 -1을 리턴하고 errno 설정
int tcsetpgrp(int filedes, pid_t pgrpid);
// 성공 시 0, 에러 시 -1을 리턴하고 errno 설정
