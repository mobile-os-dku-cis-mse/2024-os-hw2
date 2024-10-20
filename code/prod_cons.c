#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

typedef struct sharedobject {
    FILE *rfile;
    int linenum;
    char *line;
    pthread_mutex_t lock;
    pthread_cond_t full;  // 데이터가 사용할 수 있을 때 신호를 보내는 조건 변수
    pthread_cond_t empty; // 버퍼가 비어 있을 때 신호를 보내는 조건 변수
    int full_flag;        // 버퍼에 데이터가 있는지 여부를 나타내는 플래그
} so_t;

void *producer(void *arg) {
    so_t *so = arg;
    int *ret = malloc(sizeof(int));
    FILE *rfile = so->rfile;
    int i = 0;
    char *line = NULL;
    size_t len = 0;
    ssize_t read = 0;

    while (1) {
        pthread_mutex_lock(&so->lock);
        // 버퍼가 비어 있을 때까지 대기
        while (so->full_flag == 1) {
            pthread_cond_wait(&so->empty, &so->lock);
        }

        read = getdelim(&line, &len, '\n', rfile);
        if (read == -1) {
            so->line = NULL; // 생산 종료를 나타냄
            so->full_flag = 1; // 생산이 끝났음을 알림
            pthread_cond_signal(&so->full);
            pthread_mutex_unlock(&so->lock);
            break;
        }

        so->linenum = i;
        so->line = strdup(line); // 읽은 줄을 공유
        i++;
        so->full_flag = 1; // 버퍼가 꽉 찼음을 표시

        pthread_cond_signal(&so->full); // 새로운 데이터가 사용 가능함을 알림
        pthread_mutex_unlock(&so->lock);
    }
    
    free(line);
    printf("Prod_%x: %d lines\n", (unsigned int)pthread_self(), i);
    *ret = i;
    pthread_exit(ret);
}

void *consumer(void *arg) {
    so_t *so = arg;
    int *ret = malloc(sizeof(int));
    int i = 0;
    char *line;

    while (1) {
        pthread_mutex_lock(&so->lock);
        // 소비할 데이터가 있을 때까지 대기
        while (so->full_flag == 0) {
            pthread_cond_wait(&so->full, &so->lock);
        }

        line = so->line;
        if (line == NULL) {
            break; // 생산자가 종료되었으면 루프 탈출
        }

        printf("Cons_%x: [%02d:%02d] %s", (unsigned int)pthread_self(), i, so->linenum, line);
        free(so->line);
        so->full_flag = 0; // 버퍼가 비었음을 표시

        pthread_cond_signal(&so->empty); // 버퍼가 비었음을 알림
        pthread_mutex_unlock(&so->lock);
        i++;
    }

    printf("Cons: %d lines\n", i);
    *ret = i;
    pthread_exit(ret);
}

int main (int argc, char *argv[]) {
    pthread_t prod[100];
    pthread_t cons[100];
    int Nprod, Ncons;
    int rc;   long t;
    int *ret;
    int i;
    FILE *rfile;

    if (argc == 1 || argc > 2) {
        printf("usage: ./prod_cons <readfile>\n");
        exit(0);
    }

    so_t *share = malloc(sizeof(so_t));
    memset(share, 0, sizeof(so_t));
    rfile = fopen((char *) argv[1], "r");
    if (rfile == NULL) {
        perror("rfile");
        exit(0);
    }

    if (argv[2] != NULL) {
		Nprod = atoi(argv[2]);
		if (Nprod > 100) Nprod = 100;
		if (Nprod == 0) Nprod = 1;
	} else Nprod = 1;
	if (argv[3] != NULL) {
		Ncons = atoi(argv[3]);
		if (Ncons > 100) Ncons = 100;
		if (Ncons == 0) Ncons = 1;
	} else Ncons = 1;

    share->rfile = rfile;
    share->line = NULL;
    share->full_flag = 0; // 처음에는 버퍼가 비어 있음

    pthread_mutex_init(&share->lock, NULL);
    pthread_cond_init(&share->full, NULL);
    pthread_cond_init(&share->empty, NULL);

    for (i = 0; i < Nprod; i++)
        pthread_create(&prod[i], NULL, producer, share);
    for (i = 0; i < Ncons; i++)
        pthread_create(&cons[i], NULL, consumer, share);

    printf("main continuing\n");

    for (i = 0; i < Ncons; i++) {
        rc = pthread_join(cons[i], (void **) &ret);
        printf("main: consumer_%d joined with %d\n", i, *ret);
    }
    for (i = 0; i < Nprod; i++) {
        rc = pthread_join(prod[i], (void **) &ret);
        printf("main: producer_%d joined with %d\n", i, *ret);
    }

    pthread_mutex_destroy(&share->lock);
    pthread_cond_destroy(&share->full);
    pthread_cond_destroy(&share->empty);
    fclose(rfile);
    free(share);
    return 0;
}
