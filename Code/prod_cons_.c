#define _POSIX_C_SOURCE 199309L // CLOCK_MONOTONIC 오류 해결용
#define ASCII_SIZE 256
#define MAX_STRING_LENGTH 30

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

typedef struct sharedobject 
{
    FILE *rfile;
    char *line;
    int linenum;
    int full;
    int done;
    int stat[MAX_STRING_LENGTH];  // 문자열 길이 통계
    int stat2[ASCII_SIZE];        // 알파벳 통계
    pthread_mutex_t lock;
    pthread_cond_t cond;
} so_t;

// 프로듀서 스레드 함수: 파일에서 줄을 읽어 공유 버퍼에 저장
void *producer(void *arg) 
{
    so_t *so = (so_t *)arg;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    int i = 0;

    while (1) 
    {
        pthread_mutex_lock(&so->lock);
        while (so->full) 
        {
            pthread_cond_wait(&so->cond, &so->lock);
        }

        // 이전 줄 메모리 해제
        if (so->line) 
        {
            free(so->line);
            so->line = NULL;
        }

        read = getline(&line, &len, so->rfile);
        if (read == -1)                             // 파일 끝에 도달한 경우
        {
            so->done = 1;
            pthread_cond_broadcast(&so->cond);
            pthread_mutex_unlock(&so->lock);
            break;
        }

        so->line = strdup(line);            // 줄을 공유 버퍼에 저장
        so->linenum = i++;
        so->full = 1;
        pthread_cond_broadcast(&so->cond);
        pthread_mutex_unlock(&so->lock);
    }

    free(line);
    pthread_exit(NULL);
}

// 컨슈머 스레드 함수: 공유 버퍼에서 줄을 읽어 알파벳 통계 수집
void *consumer(void *arg) 
{
    so_t *so = (so_t *)arg;
    int *ret = malloc(sizeof(int));
    *ret = 0;

    while (1) 
    {
        pthread_mutex_lock(&so->lock);
        while (!so->full && !so->done) 
        {
            pthread_cond_wait(&so->cond, &so->lock);
        }

        if (so->done && !so->full) 
        {
            pthread_mutex_unlock(&so->lock);
            break;
        }

        char *line = so->line;
        printf("Consumer %lx: [line %d] %s", pthread_self(), so->linenum, line);

        // 문자열 길이 통계 수집
        size_t len = strlen(line);
        if (len >= MAX_STRING_LENGTH) len = MAX_STRING_LENGTH;
        so->stat[len - 1]++;

        // 알파벳 통계 수집
        for (int i = 0; line[i] != '\0'; i++) 
        {
            if ((line[i] >= 'A' && line[i] <= 'Z') || (line[i] >= 'a' && line[i] <= 'z')) 
            {
                so->stat2[(unsigned char)line[i]]++;
            }
        }

        so->full = 0;
        pthread_cond_signal(&so->cond);
        pthread_mutex_unlock(&so->lock);
        (*ret)++;
    }

    pthread_exit(ret);
}

int main(int argc, char *argv[]) 
{
    if (argc < 4) {  // 파일 이름, 프로듀서 수, 소비자 수를 인자로 받음
        printf("Usage: %s <filename> <num_producers> <num_consumers>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int num_producers = atoi(argv[2]);
    int num_consumers = atoi(argv[3]);

    if (num_producers <= 0 || num_consumers <= 0) 
    {
        printf("Error: The number of producers and consumers must be greater than 0\n");
        exit(EXIT_FAILURE);
    }

    so_t shared;
    memset(&shared, 0, sizeof(so_t));
    pthread_mutex_init(&shared.lock, NULL);
    pthread_cond_init(&shared.cond, NULL);

    // 파일 열기
    shared.rfile = fopen(argv[1], "r");
    if (!shared.rfile) 
    {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    // 구조체 설정 및 시작 시간 측정
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    // 프로듀서 스레드 생성
    pthread_t producers[num_producers];
    for (int i = 0; i < num_producers; i++) 
    {
        pthread_create(&producers[i], NULL, producer, &shared);
    }

    // 컨슈머 스레드 생성
    pthread_t consumers[num_consumers];
    for (int i = 0; i < num_consumers; i++) 
    {
        pthread_create(&consumers[i], NULL, consumer, &shared);
    }

    // 모든 프로듀서 스레드 종료 대기
    for (int i = 0; i < num_producers; i++) 
    {
        pthread_join(producers[i], NULL);
    }

    // 모든 컨슈머 스레드 종료 대기
    int total_processed = 0;
    for (int i = 0; i < num_consumers; i++) 
    {
        int *lines_processed;
        pthread_join(consumers[i], (void **)&lines_processed);
        total_processed += *lines_processed;
        free(lines_processed);
    }

    // 문자열 길이 통계 출력
    printf("*** String length distribution ***\n");
    for (int i = 0; i < MAX_STRING_LENGTH; i++) 
    {
        if (shared.stat[i] > 0) {
            printf("[%2d]: %4d\n", i + 1, shared.stat[i]);
        }
    }

    // 알파벳 통계 출력
    printf("\nAlphabet statistics:\n");
    for (char c = 'A'; c <= 'Z'; c++) 
    {
        printf("%c: %d\n", c, shared.stat2[(unsigned char)c] + shared.stat2[(unsigned char)(c + 32)]);
    }

    // 끝난 시간 측정 및 경과 시간 계산
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double elapsed_time = (end_time.tv_sec - start_time.tv_sec) + 
                          (end_time.tv_nsec - start_time.tv_nsec) / 1e9;
    printf("Elapsed time: %.6f seconds\n", elapsed_time);

    // 리소스 해제
    fclose(shared.rfile);
    pthread_mutex_destroy(&shared.lock);
    pthread_cond_destroy(&shared.cond);

    printf("Total lines processed: %d\n", total_processed);
    
    return 0;
}

