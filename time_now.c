#include <stdio.h>
#include <time.h>		// 1970년 1월 1일 0시부터 현재까지 몇 초 지났는지 알려줌

#define N 47
int main(void)
{
	time_t seconds = time(NULL);			// 초로 입력 받는다.
	struct tm *now = localtime(&seconds);	// tm 구조체를 이용한다.

	/*
	struct tm {
		int tm_sec;		// 초
		int tm_min;		// 분
		int tm_hour; 	// 시
		int tm_mday;	// 일
		int tm_mon; 	// 월 [0~11]
		int tm_year;	// 년 [현재시간 표시시 +1900 필요]
		int tm_wday;	// 요일 [0-일요일, 1-월요일, ... , 6-토요일]
		int tm_yday;	// 이번 해의 몇 번째 일인지
		int tm_isdst;
	};
	*/

	/* 1번째 줄 */
	printf("      \U0001F506      \U0001F505       \U0001F31E       \U0001F505      \U0001F506"); // 해
	printf("\n");

	/* 2번째 줄 */
	printf("\n");

	/* 3번째 줄 */
	printf("\u2554");
	for (int i = 0; i < N; i++) printf("\u2550");
	printf("\u2557");
	printf("\n");

	/* 4번째 줄 */
	printf("\u2551\t     ");
	printf("\U0001F4C5 "); // 달력
	printf("%d년 %d월 %d일 ", now->tm_year + 1900, now->tm_mon + 1, now->tm_mday);
	switch(now->tm_wday)
	{
		case 0: printf("일요일"); break;
		case 1: printf("월요일"); break;
		case 2: printf("화요일"); break;
		case 3: printf("수요일"); break;
		case 4: printf("목요일"); break;
		case 5: printf("금요일"); break;
		case 6: printf("토요일"); break;
		default: break;
	}
	printf("\t\t\u2551");
	printf("\n");
	/* 5번째 줄 */
	printf("\u2551\t");
	printf("\u2728      \U0001F31F      \u2728      \U0001F31F      ");
	printf("\u2728");

	printf("\t\u2551");
	printf("\n");
	
	/* 6번째 줄 */
	printf("\u2551\t      ");
	printf("\u23F0 "); // 시계
	if (now->tm_hour > 12) {
		printf("오후 ");
		printf("%d시 ", now->tm_hour-12);
	} else {
		printf("오전 ");
		printf("%d시 ", now->tm_hour);
	}
	printf("%d분 %d초", now->tm_min, now->tm_sec);
	printf("\t\t\u2551");
	printf("\n");
	
	/* 7번째 줄 */
	printf("\u255A");
	for (int i = 0; i < N; i++) printf("\u2550");
	printf("\u255D");
	printf("\n");
	/* 8번째 줄 */
	
	printf("\n");
	/* 9번째 줄 */
	printf("      \U0001F311      \U0001F312       \U0001F313       \U0001F314      \U0001F315"); // 달
	
	printf("\n");
	return 0;
}
