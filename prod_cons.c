#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "functions.h"
#include "struct.h"

pthread_cond_t prod_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t cons_cond = PTHREAD_COND_INITIALIZER;

void *producer(void *arg)
{
	so_t *so = arg;
	int *ret = malloc(sizeof(int));
	FILE *rfile = so->rfile;
	int i = 0;
	char *line = NULL;
	size_t len = 0;
	ssize_t read = 0;

	// * Read the line from the input file and move it to the shared buffer
	read_line(&read, line, &len, rfile, so, &i);

	// * Print the number of lines read
	printf("\nProd_%x: %d lines\n", (unsigned int)pthread_self(), i);
	*ret = i;
	pthread_exit(ret); // * Terminate the thread and return the number of lines read
}

void *consumer(void *arg)
{
	so_t *so = arg;
	int *ret = malloc(sizeof(int));
	int i = 0; // * Index of current line
	int len;
	char *line = NULL;

	// * Read the line from the shared buffer and print it out
	process_line(line, &len, so, &i);

	printf("\nCons_%x: %d lines\n", (unsigned int)pthread_self(), i);
	*ret = i;
	pthread_exit(ret);
}

int main (int argc, char *argv[])
{
    pthread_t prod[100];
    pthread_t cons[100];
    int Nprod, Ncons = 0;
    int rc, i, *ret = 0;

    // * Handle execution syntax error
    if (argc == 1) {
        printf("usage: ./prod_cons <readfile> #Producer #Consumer\n");
        exit(0);
    }

    // * Shared object and synchronization setup
    so_t *share = malloc(sizeof(so_t));
    memset(share, 0, sizeof(so_t));
    share->rfile = fopen(argv[1], "r");
    pthread_cond_init(&prod_cond, NULL);
    pthread_cond_init(&cons_cond, NULL);
    pthread_mutex_init(&share->lock, NULL);

    // * Determine the number of producers and consumers
    check_syntax(share->rfile, &Nprod, &Ncons, argv);

    // * Create Nprod producers and Ncons consumers
    for (i = 0; i < Nprod; i++) pthread_create(&prod[i], NULL, producer, share);
    for (i = 0; i < Ncons; i++) pthread_create(&cons[i], NULL, consumer, share);

    // * Join consumer threads
    for (i = 0; i < Ncons; i++)
	{
        rc = pthread_join(cons[i], (void **) &ret);
        printf("main: consumer_%d joined with %d\n", i, *ret);
    }

    // * Join producer threads
    for (i = 0; i < Nprod; i++)
	{
        rc = pthread_join(prod[i], (void **) &ret);
        printf("main: producer_%d joined with %d\n", i, *ret);
    }

    // * Clean up
    pthread_mutex_destroy(&share->lock);
    pthread_cond_destroy(&prod_cond);
    pthread_cond_destroy(&cons_cond);
    pthread_exit(NULL);
}
