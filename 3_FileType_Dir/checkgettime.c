#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void do_heavy_work() {
	for (volatile int i = 0; i < 100000000; i++); // 단순한 연산
}

int main(void)
{
	struct timespec start, end;
	long seconds, nanoseconds;
	double elapsed;

	clock_gettime(CLOCK_MONOTONIC, &start); // 시작 시간 측정

	do_heavy_work(); // 수행할 작업

	clock_gettime(CLOCK_MONOTONIC, &end);  // 종료 시간 측정

	// 시간 차이 계산
	seconds = end.tv_sec - start.tv_sec;
	nanoseconds = end.tv_nsec - start.tv_nsec;
	if (nanoseconds < 0) {
		seconds--;
		nanoseconds += 1000000000;
	}

	elapsed = seconds + nanoseconds*1e-9;
	printf("Elapsed time: %.9f seconds\n", elapsed);
	
	exit(0);
}
