# 변수 정의
CC = gcc
CFLAGS = -g -Wall
TARGET = ssu_cleanup
SRCS = ssu_cleanup.c
OBJS = $(SRCS:.c=.o)

# 기본 타겟
all: $(TARGET)

# 실행 파일 생성
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# 오브젝트 파일 생성 규칙
%.o: %.c
	$(CC) $(CFLAGS) -c $<

# 정리
clean:
	rm -f $(OBJS) $(TARGET)
