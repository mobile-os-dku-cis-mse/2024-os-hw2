#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#define BUFFSIZE 4096

typedef struct sharedobject {
    int nextin;
    int nextout;
    FILE* rfile;
    char* line[BUFFSIZE];

    pthread_mutex_t lock;
    pthread_cond_t cond_producer; // 생산자 조건 변수
    pthread_cond_t cond_consumer; // 소비자 조건 변수
    int full; // 버퍼에 채워진 항목 수
    int done; // 생산자 종료 플래그
} so_t;

void *producer(void *arg) {
    so_t *so = arg;
    size_t len = 0;
    ssize_t read = 0;
    char *line = NULL;
    int *ret = malloc(sizeof(int));
    int count = 0;

    while (1) {
        pthread_mutex_lock(&so->lock);

        // 버퍼가 가득 차면 대기
        while (so->full >= BUFFSIZE) {
            pthread_cond_wait(&so->cond_producer, &so->lock);
        }

        read = getdelim(&line, &len, '\n', so->rfile);
        if (read == -1) {
            so->done = 1; // 생산자가 완료되었음을 표시
            pthread_cond_broadcast(&so->cond_consumer); // 소비자에게 신호를 보냄
            printf("Producer done.\n");
            pthread_mutex_unlock(&so->lock);
            free(line); // 완료 시 line 메모리 해제
            break;
        }

        // 읽은 줄에 대한 메모리 할당
        char *new_line = strdup(line);
        if (new_line == NULL) {
            perror("Failed to allocate memory");
            free(line); // strdup 실패 시 line 해제
            pthread_mutex_unlock(&so->lock);
            break;
        }

        so->line[so->nextin] = new_line; // 줄을 버퍼에 저장
        so->nextin = (so->nextin + 1) % BUFFSIZE;
        so->full++;
        count++;

        printf("Prod_%x: [%02d:%02d] %s", (unsigned int)pthread_self(), count, so->full, new_line);
        
        pthread_cond_broadcast(&so->cond_consumer); // 소비자에게 신호를 보냄
        pthread_mutex_unlock(&so->lock);
    }

    *ret = count;
    pthread_exit(ret);
}

void *consumer(void *arg) {
    so_t *so = arg;
    int count = 0;
    char *line;
    int *ret = malloc(sizeof(int));

    while (1) {
        pthread_mutex_lock(&so->lock);

        // 데이터가 없으면 대기
        while (so->full <= 0 && !so->done) {
            pthread_cond_wait(&so->cond_consumer, &so->lock);
        }

        if (so->full <= 0 && so->done) { // 생산자가 완료되었고 버퍼가 비어 있으면
            pthread_mutex_unlock(&so->lock);
            break; 
        }

        line = so->line[so->nextout]; // 소비할 줄 가져오기
        if (line == NULL) {
            pthread_mutex_unlock(&so->lock);
            break; 
        }

        // 소비한 줄 출력 및 메모리 해제
        printf("Cons_%x: [%02d:%02d] %s", (unsigned int)pthread_self(), count, so->nextout, line);
        free(line);
        so->line[so->nextout] = NULL;
        so->nextout = (so->nextout + 1) % BUFFSIZE;
        so->full--;
        count++;

        pthread_cond_broadcast(&so->cond_producer); // 생산자에게 신호를 보냄
        pthread_mutex_unlock(&so->lock);
    }

    *ret = count;
    pthread_exit(ret);
}

int main (int argc, char *argv[]) {
    int *ret;
    int i;
    FILE *rfile;
    int Nprod, Ncons;
    pthread_t prod[100];
    pthread_t cons[100];

    if (argc < 4) {
        printf("usage: ./multi_cons <readfile> #Producer #Consumer\n");
        exit(0);
    }

    so_t *share = malloc(sizeof(so_t));
    memset(share, 0, sizeof(so_t));
    rfile = fopen(argv[1], "r"); 

    if (rfile == NULL) {
        perror("rfile");
        exit(0);
    }

    Nprod = atoi(argv[2]);
    Ncons = atoi(argv[3]); 

    share->rfile = rfile;
    for (i = 0; i < BUFFSIZE; i++) share->line[i] = NULL;

    // 동기화 프리미티브 초기화
    pthread_mutex_init(&share->lock, NULL);
    pthread_cond_init(&share->cond_producer, NULL);
    pthread_cond_init(&share->cond_consumer, NULL);
    share->full = 0;
    share->done = 0; 
    share->nextin = 0;
    share->nextout = 0; 

    // 생산자 스레드 생성
    for (i = 0; i < Nprod; i++) {
        pthread_create(&prod[i], NULL, producer, share);
    }

    // 소비자 스레드 생성
    for (i = 0; i < Ncons; i++) {
        pthread_create(&cons[i], NULL, consumer, share);
    }

    // 스레드 조인 및 결과 출력
    printf("\n\n");
    for (i = 0; i < Ncons; i++) {
        pthread_join(cons[i], (void**)&ret);
        printf("main: consumer_%d joined with %d\n", i, *ret);
        free(ret); 
    }
    for (i = 0; i < Nprod; i++) {
        pthread_join(prod[i], (void**)&ret); 
        printf("main: producer_%d joined with %d\n", i, *ret);
        free(ret);
    }

    // 정리
    fclose(rfile); 
    free(share); 
    pthread_mutex_destroy(&share->lock); 
    pthread_cond_destroy(&share->cond_producer); 
    pthread_cond_destroy(&share->cond_consumer); 
    pthread_exit(NULL); 
    return 0;
}