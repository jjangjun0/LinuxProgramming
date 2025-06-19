#include <signal.h>
sighandler_t signal (int signum, sighandler_t handler);
// 성공 시 포인터, 에러 시 NULL 리턴

typedef void (*sighandler_t) (int);

#include <sys/types.h>
#include <signal.h>
int kill(pid_t pid, int sig);
// 성공 시 0, 에러 시 -1을 리턴하고 errno 설정

#include <signal.h>
int raise(int sig);
// 성공 시 0, 에러 시 0이 아닌 값을 리턴

#include <unistd.h>
unsigned int alarm(unsigned int seconds);
// 이전에 설정한 타이머가 없다면 0,
// 그렇지 않다면 이전에 설정된 타이머가 만료될 때까지의 시간(초)

#include <unistd.h>
int pause(void);
// 시그널이 캐치되거나 시그널 핸들러 함수가 리턴되었을 때,
// -1을 리턴하고 errno를 EINTR로 설정

#include <signal.h>
int sigemptyset(sigset_t *set);
int sigfillset(sigset_t *set);
int sigaddset(sigset_t *set, int signo);
int sigdelset(sigset_t *set, int signo);
// 성공 시 0, 에러 시 -1을 리턴하고 errno 설정

int sigismember(const sigset_t *set, int signo);
// signo가 set의 멤버이면 1, 그렇지 않다면 0, 에러 시 -1을 리턴하고 errno 설정

#include <signal.h>
int sigprocmask(int how, const sigset_t *set, sigset_t *oldset);
// 성공 시 0, 에러 시 -1을 리턴하고 errno 설정

#include <signal.h>
int sigpending(sigset_t *set);
// 성공 시 0, 에러 시 -1을 리턴하고 errno 설정

#include <signal.h>
int sigaction(int signo, const struct sigaction *act, struct sigaction *oldact);
// 성공 시 0, 에러 시 -1을 리턴하고 errno 설정

#include <setjmp.h>
int sigsetjmp(sigjmp_buf env, int savesigs);
// 직접 호출되었을 때에는 0, siglongjmp() 호출에서 리턴된 경우에는 0이 아닌 값

void siglongjmp(sigjmp_buf env, int val);

#include <signal.h>
int sigsuspend(const sigset_t *sigmask);
// 항상 -1, errno는 EINTR로 설정

#include <stdlib.h>
void abort(void);

#include <unistd.h>
unsigned int sleep(unsigned int seconds);
// 0 또는 덜 잠든 시간(초)
