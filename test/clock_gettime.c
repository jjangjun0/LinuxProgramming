/* 
#include <time.h>

int clock_gettime(clockid_t clk_id, struct timespec *tp);

clk_id : 측정할 clock 종류 지정. (CLOCK_REALTIME, CLOCK_MONOTONIC 등)
tp : 결과 저장할 timespec 구조체 포인터

리턴값 : 성공시 0, 에러 시 –1 과 errno 설정

struct timespec {

    time_t tv_sec;   // 초 단위

    long   tv_nsec;  // 나노초 단위 (0 ~ 999,999,999)

}; //시간 값을 초(tv_sec) + 나노초(tv_nsec) 단위로 표현. 1초 = 1,000,000,000 나노초

 * 참고로 경과 시간 측정용으로는 CLOCK_MONOTONIC 또는 CLOCK_MONOTONIC_RAW를 사용하는 것이 일반적이다.
 * 시스템 호출 함수이기 때문에 gettimeofday()보다 더 정밀하다고 알려져 있다
*/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void do_heavy_work() {
    for (volatile int i = 0; i < 100000000; i++); // 단순한 연산
}

int main() {
    struct timespec start, end;
    long seconds, nanoseconds;
    double elapsed;

    clock_gettime(CLOCK_MONOTONIC, &start);  // 시작 시간 측정

    do_heavy_work();                         // 수행할 작업

    clock_gettime(CLOCK_MONOTONIC, &end);    // 종료 시간 측정

    // 시간 차이 계산
    seconds = end.tv_sec - start.tv_sec;
    nanoseconds = end.tv_nsec - start.tv_nsec;
    if (nanoseconds < 0) {
        seconds--;
        nanoseconds += 1000000000;
    }

    elapsed = seconds + nanoseconds*1e-9;
    printf("Elapsed time: %.9f seconds\n", elapsed);
    return 0;
}
