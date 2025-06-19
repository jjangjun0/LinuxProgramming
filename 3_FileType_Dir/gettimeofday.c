#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

void do_heavy_work() {
	for (volatile int i = 0; i < 100000000; i++); // 단순한 연산
}

int main(void)
{
	struct timeval start, end;
	long seconds, microseconds;
	double elapsed;

	gettimeofday(&start, NULL); // 시작 시간 측정

	do_heavy_work(); // 수행할 작업

	gettimeofday(&end, NULL);  // 종료 시간 측정

	// 시간 차이 계산
	seconds = end.tv_sec - start.tv_sec;
	microseconds = end.tv_usec - start.tv_usec;
	if (microseconds < 0) {
		seconds--;
		microseconds += 1000000;
	}

	elapsed = seconds + microseconds * 1e-6;
	printf("Elapsed time: %.6f seconds\n", elapsed); // 마이크로초 단위로 출력
	exit(0);
}
