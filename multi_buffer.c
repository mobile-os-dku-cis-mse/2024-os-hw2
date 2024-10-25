#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_STRING_LENGTH 30
#define ASCII_SIZE 256
#define BUFFER_SIZE 10

int done = 0;
FILE *write_fp;

// 공유 데이터 구조체
typedef struct sharedobject
{
    char *lines[BUFFER_SIZE];  // 버퍼에 여러 라인 저장
    int head;                  // 시작 인덱스
    int tail;                  // 끝 인덱스
    int count;                 // 현재 버퍼의 데이터 수
    pthread_mutex_t lock;      // 뮤텍스
    pthread_cond_t full_cond;  // 버퍼가 가득 찼을 때
    pthread_cond_t empty_cond; // 버퍼가 비었을 때
    int prod_count;            // 남은 프로듀서의 수
    int prod_done_count;       // 종료된 프로듀서 수
    int fd;                    // tar 파일 디스크립터
} so_t;

// 스레드 인자 구조체
typedef struct thread_arg
{
    int id;
    so_t *shared;
} thread_arg_t;

int stat1[MAX_STRING_LENGTH] = {0}; // 단어 길이 통계
int stat2[ASCII_SIZE] = {0};        // 문자 빈도 통계
pthread_mutex_t stats_lock;         // 통계를 보호할 뮤텍스

// 통계 업데이트 함수
void update_stats(const char *line)
{
    char *substr, *brka;
    char *cptr = strdup(line);         // 원본을 손상시키지 않기 위해 복사
    char *sep = " {}()[],;\" \n\t^.-"; // 구분자

    pthread_mutex_lock(&stats_lock); // 통계 보호를 위한 락

    for (substr = strtok_r(cptr, sep, &brka); substr; substr = strtok_r(NULL, sep, &brka))
    {
        size_t length = strlen(substr);
        if (length >= MAX_STRING_LENGTH)
            length = MAX_STRING_LENGTH;
        stat1[length - 1]++;

        // 문자 빈도 통계 업데이트 루프
        for (size_t i = 0; i < length; i++)
        {
            // 알파벳/숫자 골라내기
            if (isalnum((unsigned char)substr[i]))
            {
                stat2[(unsigned char)substr[i]]++;
            }
        }
    }

    pthread_mutex_unlock(&stats_lock); // 락 해제
    free(cptr);                        // 메모리 해제
}

// 수집된 통계 출력 함수
void print_stats()
{
    int sum = 0;
    for (int i = 0; i < MAX_STRING_LENGTH; i++)
    {
        sum += stat1[i];
    }

    // 길이 분포 출력
    printf("*** 길이 분포 *** \n");
    printf("  #ch  빈도 \n");
    for (int i = 0; i < MAX_STRING_LENGTH; i++)
    {
        int num_star = stat1[i] * 80 / sum;
        printf("[%3d]: %4d \t", i + 1, stat1[i]);
        for (int j = 0; j < num_star; j++)
            printf("*");
        printf("\n");
    }

    // A-Z 및 a-z 문자의 빈도 출력

    printf("       A        B        C        D        E        F        G        H        I        J        K        L        M        N        O        P        Q        R        S        T        U        V        W        X        Y        Z\n");
    printf("%8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d\n",
           stat2['A'] + stat2['a'], stat2['B'] + stat2['b'], stat2['C'] + stat2['c'], stat2['D'] + stat2['d'], stat2['E'] + stat2['e'],
           stat2['F'] + stat2['f'], stat2['G'] + stat2['g'], stat2['H'] + stat2['h'], stat2['I'] + stat2['i'], stat2['J'] + stat2['j'],
           stat2['K'] + stat2['k'], stat2['L'] + stat2['l'], stat2['M'] + stat2['m'], stat2['N'] + stat2['n'], stat2['O'] + stat2['o'],
           stat2['P'] + stat2['p'], stat2['Q'] + stat2['q'], stat2['R'] + stat2['r'], stat2['S'] + stat2['s'], stat2['T'] + stat2['t'],
           stat2['U'] + stat2['u'], stat2['V'] + stat2['v'], stat2['W'] + stat2['w'], stat2['X'] + stat2['x'], stat2['Y'] + stat2['y'],
           stat2['Z'] + stat2['z']);

}

// 프로듀서 함수
void *producer(void *arg)
{
    thread_arg_t *targ = (thread_arg_t *)arg;
    so_t *so = targ->shared;
    int id = targ->id;
    char buffer[1024];

    while (1)
    {
        ssize_t bytes_read = read(so->fd, buffer, sizeof(buffer) - 1);
        if (bytes_read == -1)
        {
            perror("Tar 파일 읽기 실패");
            pthread_exit(NULL);
        }
        if (bytes_read == 0)
        {
            break; // 파일 끝
        }

        buffer[bytes_read] = '\0';

        pthread_mutex_lock(&so->lock);
        while (so->count == BUFFER_SIZE)
        {
            pthread_cond_wait(&so->empty_cond, &so->lock);
        }

        so->lines[so->tail] = strdup(buffer);
        so->tail = (so->tail + 1) % BUFFER_SIZE;
        so->count++;

        pthread_cond_signal(&so->full_cond); // 소비자에게 알림
        pthread_mutex_unlock(&so->lock);
    }

    // 마지막 부분
    pthread_mutex_lock(&so->lock);
    so->prod_done_count++;
    if (so->prod_done_count == so->prod_count)
    {
        done = 1;
        pthread_cond_broadcast(&so->full_cond);
    }
    pthread_mutex_unlock(&so->lock);

    pthread_exit(NULL);
}


// 컨슈머 함수
void *consumer(void *arg)
{
    thread_arg_t *targ = (thread_arg_t *)arg;
    so_t *so = targ->shared;
    int id = targ->id;

    while (1)
    {
        pthread_mutex_lock(&so->lock);
        while (so->count == 0 && !done)
        {
            pthread_cond_wait(&so->full_cond, &so->lock);
        }

        if (done && so->count == 0)
        {
            pthread_mutex_unlock(&so->lock);
            break;
        }

        char *line = so->lines[so->head];
        so->head = (so->head + 1) % BUFFER_SIZE;
        so->count--;
        //update_stats(line);

        pthread_cond_signal(&so->empty_cond); 
        pthread_mutex_unlock(&so->lock);

        update_stats(line);
        free(line);
    }

    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    if (argc < 4)
    {
        printf("usage: ./prod_cons <tar_file> #Producer #Consumer\n");
        exit(0);
    }

    pthread_t prod_threads[100];
    pthread_t cons_threads[100];
    thread_arg_t prod_args[100];
    thread_arg_t cons_args[100];
    int Nprod, Ncons;
    struct timeval start, end;

    Nprod = atoi(argv[2]);
    Ncons = atoi(argv[3]);

    write_fp = fopen("result_mutex_mult.txt", "w");

    // 공유자원 초기화
    so_t share;
    memset(&share, 0, sizeof(so_t));
    done = 0;

    // Tar 파일 열기
    share.fd = open(argv[1], O_RDONLY);
    if (share.fd == -1)
    {
        perror("Tar 파일 열기 실패");
        exit(1);
    }

    gettimeofday(&start, NULL);

    pthread_mutex_init(&share.lock, NULL);
    pthread_cond_init(&share.full_cond, NULL);
    pthread_cond_init(&share.empty_cond, NULL);
    share.prod_count = Nprod;

    // 프로듀서 스레드 생성
    for (int i = 0; i < Nprod; i++)
    {
        prod_args[i].id = i;
        prod_args[i].shared = &share;
        pthread_create(&prod_threads[i], NULL, producer, &prod_args[i]);
    }

    // 컨슈머 스레드 생성
    for (int i = 0; i < Ncons; i++)
    {
        cons_args[i].id = i;
        cons_args[i].shared = &share;
        pthread_create(&cons_threads[i], NULL, consumer, &cons_args[i]);
    }

    // 프로듀서 스레드 종료 대기
    for (int i = 0; i < Nprod; i++)
    {
        pthread_join(prod_threads[i], NULL);
    }

    // 컨슈머 스레드 종료 대기
    for (int i = 0; i < Ncons; i++)
    {
        pthread_join(cons_threads[i], NULL);
    }

    pthread_mutex_destroy(&share.lock);
    pthread_cond_destroy(&share.full_cond);
    pthread_cond_destroy(&share.empty_cond);

    // 끝나는 시간 기록
    gettimeofday(&end, NULL);

    // 처리 시간 계산 (초와 마이크로초로)
    long seconds = end.tv_sec - start.tv_sec;
    long micros = ((seconds * 1000000) + end.tv_usec) - start.tv_usec;
    double sec = (double)micros / 1000000;

    // Tar 파일 닫기
    close(share.fd);

    print_stats();
    fclose(write_fp);

    // 처리 시간 출력
    printf("파일 처리 시간: %ld microseconds ( %lf sec )\n", micros, sec);
    pthread_mutex_destroy(&stats_lock);

    return 0;
}
