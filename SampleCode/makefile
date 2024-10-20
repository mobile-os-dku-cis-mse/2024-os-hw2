# Makefile for char_stat, prod_cons, and pthread programs

CC = gcc
CFLAGS = -pthread -Wall -g

# 실행 파일 이름들
CHAR_STAT_EXEC = char_stat
PROD_CONS_EXEC = prod_cons
PTHREAD_EXEC = pthread

# 소스 파일들
CHAR_STAT_SRC = char_stat.c
PROD_CONS_SRC = prod_cons.c
PTHREAD_SRC = pthread.c

# 모든 실행 파일 빌드
all: $(CHAR_STAT_EXEC) $(PROD_CONS_EXEC) $(PTHREAD_EXEC)

# char_stat 실행 파일 빌드
$(CHAR_STAT_EXEC): $(CHAR_STAT_SRC)
	$(CC) $(CFLAGS) -o $(CHAR_STAT_EXEC) $(CHAR_STAT_SRC)

# prod_cons 실행 파일 빌드
$(PROD_CONS_EXEC): $(PROD_CONS_SRC)
	$(CC) $(CFLAGS) -o $(PROD_CONS_EXEC) $(PROD_CONS_SRC)

# pthread 실행 파일 빌드
$(PTHREAD_EXEC): $(PTHREAD_SRC)
	$(CC) $(CFLAGS) -o $(PTHREAD_EXEC) $(PTHREAD_SRC)

# 생성된 실행 파일과 오브젝트 파일 정리
clean:
	rm -f $(CHAR_STAT_EXEC) $(PROD_CONS_EXEC) $(PTHREAD_EXEC)

# 파일 이름과 충돌하지 않도록 하는 가짜 목표 설정
.PHONY: all clean
