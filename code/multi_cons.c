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
    pthread_cond_t cond_producer; // ������ ���� ����
    pthread_cond_t cond_consumer; // �Һ��� ���� ����
    int full; // ���ۿ� ä���� �׸� ��
    int done; // ������ ���� �÷���
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

        // ���۰� ���� ���� ���
        while (so->full >= BUFFSIZE) {
            pthread_cond_wait(&so->cond_producer, &so->lock);
        }

        read = getdelim(&line, &len, '\n', so->rfile);
        if (read == -1) {
            so->done = 1; // �����ڰ� �Ϸ�Ǿ����� ǥ��
            pthread_cond_broadcast(&so->cond_consumer); // �Һ��ڿ��� ��ȣ�� ����
            printf("Producer done.\n");
            pthread_mutex_unlock(&so->lock);
            free(line); // �Ϸ� �� line �޸� ����
            break;
        }

        // ���� �ٿ� ���� �޸� �Ҵ�
        char *new_line = strdup(line);
        if (new_line == NULL) {
            perror("Failed to allocate memory");
            free(line); // strdup ���� �� line ����
            pthread_mutex_unlock(&so->lock);
            break;
        }

        so->line[so->nextin] = new_line; // ���� ���ۿ� ����
        so->nextin = (so->nextin + 1) % BUFFSIZE;
        so->full++;
        count++;

        printf("Prod_%x: [%02d:%02d] %s", (unsigned int)pthread_self(), count, so->full, new_line);
        
        pthread_cond_broadcast(&so->cond_consumer); // �Һ��ڿ��� ��ȣ�� ����
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

        // �����Ͱ� ������ ���
        while (so->full <= 0 && !so->done) {
            pthread_cond_wait(&so->cond_consumer, &so->lock);
        }

        if (so->full <= 0 && so->done) { // �����ڰ� �Ϸ�Ǿ��� ���۰� ��� ������
            pthread_mutex_unlock(&so->lock);
            break; 
        }

        line = so->line[so->nextout]; // �Һ��� �� ��������
        if (line == NULL) {
            pthread_mutex_unlock(&so->lock);
            break; 
        }

        // �Һ��� �� ��� �� �޸� ����
        printf("Cons_%x: [%02d:%02d] %s", (unsigned int)pthread_self(), count, so->nextout, line);
        free(line);
        so->line[so->nextout] = NULL;
        so->nextout = (so->nextout + 1) % BUFFSIZE;
        so->full--;
        count++;

        pthread_cond_broadcast(&so->cond_producer); // �����ڿ��� ��ȣ�� ����
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

    // ����ȭ ������Ƽ�� �ʱ�ȭ
    pthread_mutex_init(&share->lock, NULL);
    pthread_cond_init(&share->cond_producer, NULL);
    pthread_cond_init(&share->cond_consumer, NULL);
    share->full = 0;
    share->done = 0; 
    share->nextin = 0;
    share->nextout = 0; 

    // ������ ������ ����
    for (i = 0; i < Nprod; i++) {
        pthread_create(&prod[i], NULL, producer, share);
    }

    // �Һ��� ������ ����
    for (i = 0; i < Ncons; i++) {
        pthread_create(&cons[i], NULL, consumer, share);
    }

    // ������ ���� �� ��� ���
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

    // ����
    fclose(rfile); 
    free(share); 
    pthread_mutex_destroy(&share->lock); 
    pthread_cond_destroy(&share->cond_producer); 
    pthread_cond_destroy(&share->cond_consumer); 
    pthread_exit(NULL); 
    return 0;
}