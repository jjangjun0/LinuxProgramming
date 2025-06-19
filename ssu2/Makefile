# CC     : 사용할 컴파일러
# CFLAGS : 컴파일 옵션
#     -Wall : 모든 경고 출력
#     -g    : 디버깅 정보 포함
# TARGET : 최종 실행 파일 이름
# SRCS   : 소스 파일 목록 (.c) // .h는 컴파일 대상이 아니다.
# OBJS   : 목적 파일 목록 (.o 확장자)

# $@ : 현재 타겟 이름
# $^ : 모든 의존 파일
# $< : 첫 번째 의존 파일
# $? : 타겟보다 더 최근에 수정된 의존 파일들
# $* : 확장자 없는 타겟 이름

# 공통 설정
CC = gcc
CFLAGS = -Wall -g

# 각각의 실행파일과 대응하는 소스들
TARGETS = ssu_cleanupd ssu_help ssu_show ssu_add ssu_modify ssu_remove
SRCS_ssu_cleanupd = ssu_cleanupd.c
SRCS_ssu_help     = ssu_help.c
SRCS_ssu_show     = ssu_show.c
SRCS_ssu_add      = ssu_add.c
SRCS_ssu_modify   = ssu_modify.c
SRCS_ssu_remove   = ssu_remove.c

# .o 파일
OBJS_ssu_cleanupd = $(SRCS_ssu_cleanupd:.c=.o)
OBJS_ssu_help     = $(SRCS_ssu_help:.c=.o)
OBJS_ssu_show     = $(SRCS_ssu_show:.c=.o)
OBJS_ssu_add      = $(SRCS_ssu_add:.c=.o)
OBJS_ssu_modify   = $(SRCS_ssu_modify:.c=.o)
OBJS_ssu_remove   = $(SRCS_ssu_remove:.c=.o)

# 기본 타겟
all: $(TARGETS)

# 링크
ssu_cleanupd: $(OBJS_ssu_cleanupd)
	$(CC) $(CFLAGS) -o $@ $^

ssu_help: $(OBJS_ssu_help)
	$(CC) $(CFLAGS) -o $@ $^
ssu_show: $(OBJS_ssu_show)
	$(CC) $(CFLAGS) -o $@ $^
ssu_add:  $(OBJS_ssu_add)
	$(CC) $(CFLAGS) -o $@ $^
ssu_modify: $(OBJS_ssu_modify)
	$(CC) $(CFLAGS) -o $@ $^
ssu_remove: $(OBJS_ssu_remove)
	$(CC) $(CFLAGS) -o $@ $^

# 개별 컴파일
%.o: %.c
	$(CC) $(CFLAGS) -c $<

# 모두 삭제
clean:
	rm -f *.o $(TARGETS)
