#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

typedef struct sharedobject 
{
    FILE *rfile;
    int linenum;
    char *line;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    int full;  // 0: empty, 1: full
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
            so->full = 1;
            so->line = NULL;  // Signal end of file
            pthread_cond_signal(&so->cond);
            pthread_mutex_unlock(&so->lock);
            break;
        }
        so->linenum = i;
        so->line = strdup(line);  // Share the line
        so->full = 1;
        pthread_cond_signal(&so->cond);  // Wake up the consumer
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
        while (!so->full) 
        {
            // Buffer is empty, wait for producer to produce
            pthread_cond_wait(&so->cond, &so->lock);
        }
        if (so->line == NULL) 
        {
            // End of file signal
            pthread_mutex_unlock(&so->lock);
            break;
        }
        line = so->line;
        printf("Consumer %x: [%02d:%02d] %s", (unsigned int)pthread_self(), i, so->linenum, line);
        free(so->line);
        so->full = 0;
        pthread_cond_signal(&so->cond);  // Wake up the producer
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
    pthread_t cons;
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

    pthread_create(&prod, NULL, producer, share);
    pthread_create(&cons, NULL, consumer, share);

    pthread_join(cons, (void **)&ret);
    printf("main: consumer joined with %d\n", *ret);
    free(ret);

    pthread_join(prod, (void **)&ret);
    printf("main: producer joined with %d\n", *ret);
    free(ret);

    pthread_mutex_destroy(&share->lock);
    pthread_cond_destroy(&share->cond);
    fclose(rfile);
    free(share);
    return 0;
}
