#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "functions.h"
#include "struct.h"

void *producer(void *arg) {
	so_t *so = arg;
	int *ret = malloc(sizeof(int));
	FILE *rfile = so->rfile;
	int i = 0;
	char *line = NULL;
	size_t len = 0;
	ssize_t read = 0;

	pthread_mutex_lock(&so->lock);
	read_line(&read, line, &len, rfile, so, &i);
	pthread_mutex_unlock(&so->lock);

	// free(line);
	printf("\nProd_%x: %d lines\n", (unsigned int)pthread_self(), i); // * Print the number of lines read
	*ret = i;
	pthread_exit(ret); // * Terminate the thread and return the number of lines read
}

void *consumer(void *arg) {
	so_t *so = arg;
	int *ret = malloc(sizeof(int));
	int i = 0; // * Index of current line
	int len;
	char *line;

	pthread_mutex_lock(&so->lock);
	process_line(line, &len, so, &i);
	pthread_mutex_unlock(&so->lock);

	printf("Cons: %d lines\n\n", i);
	*ret = i;
	pthread_exit(ret);
}

int main (int argc, char *argv[])
{
	pthread_t prod[100];
	pthread_t cons[100];
	int Nprod, Ncons = 0; // * Number of producers and Number of consumers
	int rc;   long t;
	int *ret = 0;
	int i;

	FILE *rfile;
	if (argc == 1) {
		printf("usage: ./prod_cons <readfile> #Producer #Consumer\n");
		exit (0);
	}

	so_t *share = malloc(sizeof(so_t));
	memset(share, 0, sizeof(so_t)); // * Initialize shared memory value
	pthread_cond_init(&share->prod_cond, NULL);
	pthread_cond_init(&share->cons_cond, NULL);

	rfile = fopen((char *) argv[1], "r");
	
	check_syntax(rfile, &Nprod, &Ncons, argv);

	share->rfile = rfile;
	share->line = NULL;
	pthread_mutex_init(&share->lock, NULL);

	// * Create *Nprod* producers and *Ncons* consumers (as per defined amount)
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