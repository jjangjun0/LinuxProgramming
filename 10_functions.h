#include <unistd.h>
#include <fcntl.h>
int fcntl(int fd, int cmd, ... /*arg*/);

int fcntl(int fd, int cmd);
int fcntl(int fd, int cmd, long arg);
int fcntl(int fd, int cmd, struct flock *lock);
// 성공 시 cmd에 따라 다르다. 에러시 -1을 리턴한다.


    /* cmd flag */
{
	F_DUPFD            : // fd로 지정된 파일 디스크립터를 복사 (락, 파일 위치 포인터, 파일 플래그 공유 O | close-on-exec() 공유 X)
	F_DUPFD_CLOEXEC    : // F_DUPFD와 같은 기능인데, close-on-exec()을 설정한다.

                    // close-on-exec : 아버지와는 완전히 독립된

	F_GETFD            : // fd로 지정된 파일 디스크립터의 플래그를 리턴하는데, arg 인자는 무시
	F_SETFD            : // fd로 지정된 파일 디스크립터의 플래그를 arg 인자에서 지정한 플래그 값으로 재설정
	F_GETFL            : // fd로 지정된 파일 디스크립터가 open할 때 지정한 파일의 접근 권한과 상태 플래그를 리턴하고, arg 인자는 무시
	F_SETFL            : // fd로 지정된 파일 디스크립터의 파일 상태 플래그를 arg 인자에 지정한 플래그 값으로 재설정한다.
}

/*
    {   파일 상태 플래그 -> O_APPEND, O_NONBLOCK, O_ASYNC, O_DIRECT   }

	{   arg 접근 권한 플래그 -> O_RDONLY, O_WRONLY, O_RDWR   }

	{   파일 생성 플래그 -> O_CREAT, O_EXCL, O_NOCTTY, O_TRUNC   }
*/

    // lock을 설정하기 위한 cmd 인자
{
	F_GETLK    : // 지정된 락을 설정할 수 없게 만드는 다른 어떤 락이 존재하는지 파악 
	F_SETLK    : // 락을 설정할 때 사용하며 다른 프로세스가 이미 락을 설정 했을 경우 -1을 리턴, flockptr로 지정된 락을 설정할 수 있다.
                 // Read or Write lock 설정하는데 호환성 규칙 때문에 시스템이 잠금을 거부한 경우, fcntl은 errno를 EACCESS나 EAGAIN 설정

	F_SETLKW   : // 다른 프로세스가 이미 락을 설정 했을 경우 락을 해제할 때까지 기다림 (F_SETLK의 차단 버전)
}

// flock 구조체의 멤버 변수
struct flock {
	short l_type; // F_RDLCK, F_WRLCK, 또는 F_UNLCK
	short l_whence; // SEEK_SET, SEEK_CUR, 또는 SEEK_END
	__off_t l_start; // 바이트 단위 오프셋, l_whence와 관련됨
	__off_t l_len; // length, in bytes; 0 means lock to EOF
	__pid_t l_pid; // returned with F_GETLK
};

// flock 구조체의 l_type
{
	F_RDLCK : // 다른 프로세스가 Read 락만 가능 / Write 락은 불가능
	F_WRLCK : // 다른 프로세스는 Read 락과 Write 락 모두 불가능
	F_UNLCK : // 락 해제
}


    /* File 부분적으로 lock 설정하는 코드 */
int lock_reg(int fd, int cmd, int type, off_t offset, int whence, off_t len)
{
	struct flock lock;

	lock.l_type = type;
	lock.l_start = offset;
	lock.l_whence = whence;
	lock.l_len = len;

	return(fcntl(fd, cmd, &lock));
}

    /* File 부분적으로 lock 설정하는 코드 */
int lockfile(int fd)
{
	struct flock fl;

	fl.l_type = F_WRLCK;
	fl.l_start = 0;
	fl.l_whence = SEEK_SET;
	fl.l_len = 0;
	return(fcntl(fd, F_SETLK, &fl));
}



    /* 비트 연산자를 이용한 플래그 처리 */
{
	flag |= mask    => 플래그의 특정 비트를 켠다
	flag &= mask    => 플래그의 특정 비트를 끈다
	flag ^= mask    => 플래그의 특정 비트를 토글시킨다 (*토글 : 비트 반전) ex. 0001 -> 1110
	flag &mask      => 플래그의 특정 비트가 켜져 있는지 검사한다
}
