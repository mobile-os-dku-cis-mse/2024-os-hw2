#define _POSIX_C_SOURCE 199309L // CLOCK_MONOTONIC 오류 해결용
#define ASCII_SIZE 256
#define MAX_STRING_LENGTH 30
#define BUFFER_SIZE 1024  // 한 번에 읽어올 바이너리 데이터 크기 (1KB)

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

typedef struct sharedobject 
{
    FILE *rfile;
    unsigned char buffer[BUFFER_SIZE];  // 바이너리 데이터를 저장할 버퍼
    size_t bytes_read;
    int full;
    int done;
    int stat[MAX_STRING_LENGTH];  // 문자열 길이 통계
    int stat2[ASCII_SIZE];        // 알파벳 통계
    pthread_mutex_t lock;
    pthread_cond_t cond;
} so_t;

// 프로듀서 스레드 함수: 파일에서 바이너리 데이터를 읽어 공유 버퍼에 저장
void *producer(void *arg) 
{
    so_t *so = (so_t *)arg;
    int i = 0;

    while (1) 
    {
        pthread_mutex_lock(&so->lock);
        while (so->full) 
        {
            pthread_cond_wait(&so->cond, &so->lock);
        }

        // 바이너리 파일에서 데이터를 읽음
        so->bytes_read = fread(so->buffer, 1, BUFFER_SIZE, so->rfile);
        if (so->bytes_read == 0) // 파일 끝에 도달한 경우
        {
            so->done = 1;
            pthread_cond_broadcast(&so->cond);
            pthread_mutex_unlock(&so->lock);
            break;
        }

        so->full = 1;
        pthread_cond_broadcast(&so->cond);
        pthread_mutex_unlock(&so->lock);
    }

    pthread_exit(NULL);
}

// 컨슈머 스레드 함수: 공유 버퍼에서 데이터를 읽어 알파벳 통계 수집
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

        // 바이너리 데이터를 처리
        for (size_t i = 0; i < so->bytes_read; i++) 
        {
            unsigned char byte = so->buffer[i];

            // 알파벳이면 통계에 반영
            if ((byte >= 'A' && byte <= 'Z') || (byte >= 'a' && byte <= 'z')) 
            {
                so->stat2[byte]++;
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

    // 바이너리 파일 열기
    shared.rfile = fopen(argv[1], "rb");  // "rb"로 바이너리 모드로 파일 열기
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
