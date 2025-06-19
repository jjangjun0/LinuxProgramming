#include <stdio.h>
FILE *fopen(const char *pathname, const char *mode);
FILE *fdopen(int filedes, const char *mode);
FILE *freopen(const char *pathname, const char *mode, FILE *fp);
// 성공 시 파일 포인터, 에러 시 NULL을 리턴하고 errno 설정

#include <stdio.h>
int fclose(FILE *fp);
// 성공 시 0, 에러 시 EOF를 리턴하고 errno 설정

#define _GNU_SOURCE
#include <stdio.h>
int fcloseall(void);
// 성공 시 0, 에러 시 EOF

#include <stdio.h>
void setbuf(FILE *fp, char *buf);
int setvbuf(FILE *fp, char *buf, int mode, size_t size);
// 성공 시 0, 에러 시 0이 아닌 값을 리턴하고 errno 설정

#include <stdio.h>
int fflush(FILE *fp);
// 성공 시 0, 에러 시 EOF 리턴하고 errno 설정

#include <stdio.h>
int getc(FILE *fp);
int fgetc(FILE *fp);
int getchar(void);
// 성공 시 다음 문자, 파일 끝이나 에러 시 EOF

#include <stdio.h>
int ferror(FILE *fp);
// 성공 시 0, 에러 시 0이 아닌 값
int feof(FILE *fp);
// 조건이 참이라면 0이 아닌 값, 참이 아니라면 0 리턴
void clearerr(FILE *fp);

#include <stdio.h>
int ungetc(int c, FILE *fp);
// 성공 시 c, 에러 시 EOF

#include <stdio.h>
int putc(int c, FILE *fp);
int fputc(int c, FILE *fp);
int putchar(int c);
// 성공 시 c, 에러 시 EOF

#include <stdio.h>
char *fgets(char *buf, int n, FILE *fp);
char *gets(char *buf);
// 성공 시 buf, 파일 끝이나 에러 시 NULL 리턴

#include <stdio.h>
int fputs(const char *str, FILE *fp);
int puts(const char *str);
// 성공 시 음이 아닌 값, 에러 시 EOF

#include <stdio.h>
size_t fread(void *ptr, size_t size, size_t nobj, FILE *fp);
size_t fwrite(const void *ptr, size_t size, size_t nobj, FILE *fp);
// 성공적으로 읽거나 쓴 객체들의 개수

#include <stdio.h>
long ftell(FILE *fp);
// 성공 시 파일의 현재 오프셋, 에러 시 -1L을 리턴하고 errno 설정
int fseek(FILE *fp, long offset, int whence);
// 성공 시 0, 에러 시 0이 아닌 값 리턴
void rewind(FILE *fp);

#include <stdio.h>
off_t ftello(FILE *fp);
// 성공 시 파일의 현재 오프셋, 에러 시 (off_t)(-1)을 리턴하고 errno 설정
int fseeko(FILE *fp, off_t offset, int whence);
// 성공 시 0, 에러 시 0이 아닌 값 리턴

#include <stdio.h>
int fgetpos(FILE *fp, fpos_t *pos);
// 성공 시 0, 에러 시 0이 아닌 값 리턴
int fsetpos(FILE *fp, fpos_t *pos);
// 성공 시 0, 에러 시 0이 아닌 값 리턴하고 errno 설정

#include <stdio.h>
int print(const char *format, ...);
int fprintf(FILE *fp, const char *format, ...);
// 성공 시 출력된 문자 개수, 에러 시 음의 값 리턴
int sprintf(char *buf, const char *format, ...);
int snprintf(char *buf, size_t n, const char *format, ...);
// 성공 시 배열에 저장된 문자 개수, 에러 시 음의 값 리턴

#include <stdio.h>
int scanf(const char *format, ...);
int fscanf(FILE *fp, const char *format, ...);
int sscanf(const char *buf, const char *format, ...);
// 성공 시 배정된 입력 항목들의 개수,
// 어떠한 변화도 일어나기 전에 파일 끝에 도달한 경우에 EOF,
// 읽기 에러 발생 시 EOF를 리턴하고 errno 설정

#include <stdio.h>
int fileno(FILE *fp);
// 스트림에 연관된 파일 디스크립터,
// 잘못된(invalid) 스트림일 경우 -1을 리턴하고 errno를 EBADF로 설정

#include <stdio.h>
char *tmpnam(char *ptr);
// 고유한 임시파일의 경로 이름을 가리키는 포인터,
// 고유한 이름을 더 만들 수 없는 경우는 NULL을 리턴
FILE *tmpfile(void);
// 성공 시 파일 포인터, 에러 시 NULL을 리턴하고 errno 설정

#include <stdio.h>
char *tempnam(const char *directory, const char *prefix);
// 고유한 경로 이름을 가리키는 포인터, 에러 시 NULL을 리턴하고 errno 설정
