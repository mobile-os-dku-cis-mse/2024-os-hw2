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
    pthread_cond_t cond;
    int full;
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
        read = getdelim(&line, &len, '\n', rfile);
        pthread_mutex_lock(&so->lock);
        while (so->full) {
            pthread_cond_wait(&so->cond, &so->lock);
        }
        if (read == -1) {
            so->line = NULL;
            so->full = 1;
            pthread_cond_broadcast(&so->cond);
            pthread_mutex_unlock(&so->lock);
	    printf("생산자 마지막 라인 찍음");
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
        
        // 데이터가 없으면 대기
        while (so->full == 0) {
            pthread_cond_wait(&so->cond, &so->lock);
        }
        
        line = so->line; // 데이터를 가져옴

        // 데이터가 NULL이면 루프 종료
        if (line == NULL) {
		printf("NULL을 찍음");
            pthread_mutex_unlock(&so->lock);
            break;
        }

        printf("Cons_%x: [%02d:%02d] %s",
               (unsigned int)pthread_self(), i, so->linenum, line);
        free(so->line); // 메모리 해제
        so->full = 0;   // 데이터 소비 후 상태 업데이트

        pthread_cond_broadcast(&so->cond); // 생산자 및 다른 소비자에게 신호
        pthread_mutex_unlock(&so->lock);
        i++;
    }

    printf("Cons: %d lines\n", i);
    *ret = i;
    pthread_exit(ret);
}

int main (int argc, char *argv[])
{
	pthread_t prod[100];
	pthread_t cons[100];
	int Nprod, Ncons;
	int rc;   long t;
	int *ret;
	int i;
	FILE *rfile;
	if (argc == 1) {
		printf("usage: ./prod_cons <readfile> #Producer #Consumer\n");
		exit (0);
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
	pthread_mutex_init(&share->lock, NULL);
	for (i = 0 ; i < Nprod ; i++)
		pthread_create(&prod[i], NULL, producer, share);
	for (i = 0 ; i < Ncons ; i++)
		pthread_create(&cons[i], NULL, consumer, share);
	printf("main continuing\n");

	for (i = 0 ; i < Ncons ; i++) {
		rc = pthread_join(cons[i], (void **) &ret);
		printf("main: consumer_%d joined with %d\n", i, *ret);
	}
	for (i = 0 ; i < Nprod ; i++) {
		rc = pthread_join(prod[i], (void **) &ret);
		printf("main: producer_%d joined with %d\n", i, *ret);
	}
	pthread_exit(NULL);
	exit(0);
}


