# 컴파일러 설정
CC = gcc
CFLAGS = -pthread -Wall

# 실행 파일 이름
TARGETS = prod_cons_1_1 prod_cons_ prod_cons_bin

# 기본 타겟: 모든 실행 파일을 빌드
all: $(TARGETS)

# 각 파일에 대한 컴파일 규칙
prod_cons_1_1: prod_cons_1_1.c
	$(CC) $(CFLAGS) -o $@ prod_cons_1_1.c

prod_cons_: prod_cons_.c
	$(CC) $(CFLAGS) -o $@ prod_cons_.c

prod_cons_bin: prod_cons_bin.c
	$(CC) $(CFLAGS) -o $@ prod_cons_bin.c

# 실행 후 불필요한 파일을 정리하는 규칙
clean:
	rm -f $(TARGETS)
