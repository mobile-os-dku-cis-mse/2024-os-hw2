#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define NUM_CONSUMERS 3  // 소비자 스레드의 개수를 설정

typedef struct sharedobject 
{
    FILE *rfile;
    int linenum;
    char *line;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    int full;  // 0: empty, 1: full
    int done;  // 파일 읽기가 완료되었는지 여부를 체크하는 플래그
} so_t;

void *producer(void *arg) 
{
    so_t *so = arg;
    int *ret = malloc(sizeof(int));
    FILE *rfile = so->rfile;
    int i = 0;
    char *line = NULL;
    size_t len = 0;
    ssize_t read = 0;

    while (1) 
    {
        read = getdelim(&line, &len, '\n', rfile);
        pthread_mutex_lock(&so->lock);
        while (so->full) 
        {
            // Buffer is full, wait for consumer to consume
            pthread_cond_wait(&so->cond, &so->lock);
        }
        if (read == -1) 
        {
            // 파일의 끝을 알림
            so->done = 1;
            so->full = 1;  // 마지막 신호를 보냄
            pthread_cond_broadcast(&so->cond);
            pthread_mutex_unlock(&so->lock);
            break;
        }
        so->linenum = i;
        so->line = strdup(line);
        so->full = 1;
        pthread_cond_broadcast(&so->cond);
        pthread_mutex_unlock(&so->lock);
        i++;
    }
    free(line);
    printf("Producer %x: %d lines\n", (unsigned int)pthread_self(), i);
    *ret = i;
    pthread_exit(ret);
}

void *consumer(void *arg) 
{
    so_t *so = arg;
    int *ret = malloc(sizeof(int));
    int i = 0;
    char *line;

    while (1) 
    {
        pthread_mutex_lock(&so->lock);
        while (!so->full && !so->done) 
        {
            // Buffer is empty, wait for producer to produce
            pthread_cond_wait(&so->cond, &so->lock);
        }
        if (so->done && !so->full) 
        {
            // 파일의 끝에 도달했을 때 종료
            pthread_mutex_unlock(&so->lock);
            break;
        }
        line = so->line;
        printf("Consumer %x: [%02d:%02d] %s", (unsigned int)pthread_self(), i, so->linenum, line);
        free(so->line);
        so->line = NULL;
        so->full = 0;
        pthread_cond_signal(&so->cond);
        pthread_mutex_unlock(&so->lock);
        i++;
    }
    printf("Consumer %x: %d lines processed\n", (unsigned int)pthread_self(), i);
    *ret = i;
    pthread_exit(ret);
}

int main(int argc, char *argv[]) 
{
    pthread_t prod;
    pthread_t cons[NUM_CONSUMERS];
    int rc, *ret;
    FILE *rfile;

    if (argc == 1) 
    {
        printf("usage: ./prod_cons <readfile>\n");
        exit(0);
    }

    so_t *share = malloc(sizeof(so_t));
    memset(share, 0, sizeof(so_t));
    rfile = fopen((char *)argv[1], "r");
    if (rfile == NULL) 
    {
        perror("rfile");
        exit(0);
    }

    share->rfile = rfile;
    pthread_mutex_init(&share->lock, NULL);
    pthread_cond_init(&share->cond, NULL);

    // 프로듀서 스레드 생성
    pthread_create(&prod, NULL, producer, share);

    // 여러 개의 컨슈머 스레드 생성
    for (int i = 0; i < NUM_CONSUMERS; i++) 
    {
        pthread_create(&cons[i], NULL, consumer, share);
    }

    // 모든 컨슈머 스레드 종료 대기
    for (int i = 0; i < NUM_CONSUMERS; i++) 
    {
        pthread_join(cons[i], (void **)&ret);
        printf("main: consumer_%d joined with %d\n", i, *ret);
        free(ret);
    }

    // 프로듀서 스레드 종료 대기
    pthread_join(prod, (void **)&ret);
    printf("main: producer joined with %d\n", *ret);
    free(ret);

    // 리소스 정리
    pthread_mutex_destroy(&share->lock);
    pthread_cond_destroy(&share->cond);
    fclose(rfile);
    free(share);

    return 0;
}
