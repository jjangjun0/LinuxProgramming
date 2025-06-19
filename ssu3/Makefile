# 공통 설정
CC = gcc
CFLAGS = -Wall -g

# 각각의 실행파일과 대응하는 소스들
TARGETS = ssu_ext2 ssu_tree ssu_print ssu_help
SRCS_ssu_ext2     = ssu_ext2.c
SRCS_ssu_tree     = ssu_tree.c
SRCS_ssu_print    = ssu_print.c
SRCS_ssu_help     = ssu_help.c


# .o 파일
OBJS_ssu_ext2 = $(SRCS_ssu_ext2:.c=.o)
OBJS_ssu_tree     = $(SRCS_ssu_tree:.c=.o)
OBJS_ssu_print    = $(SRCS_ssu_print:.c=.o)
OBJS_ssu_help     = $(SRCS_ssu_help:.c=.o)

# 기본 타겟
all: $(TARGETS)

# 링크
ssu_ext2: $(OBJS_ssu_ext2)
	$(CC) $(CFLAGS) -o $@ $^

ssu_tree: $(OBJS_ssu_tree)
	$(CC) $(CFLAGS) -o $@ $^

ssu_print: $(OBJS_ssu_print)
	$(CC) $(CFLAGS) -o $@ $^

ssu_help: $(OBJS_ssu_help)
	$(CC) $(CFLAGS) -o $@ $^

# 개별 컴파일
%.o: %.c
	$(CC) $(CFLAGS) -c $<

# 모두 삭제
clean:
	rm -f *.o $(TARGETS)
