#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "functions.h"
#include "struct.h"

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

	printf("\nProd_%x: %d lines\n", (unsigned int)pthread_self(), i); // * Print the number of lines read
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

	printf("Cons: %d lines\n\n", i);
	*ret = i;
	pthread_exit(ret);
}

int main (int argc, char *argv[])
{
	pthread_t prod[100];
	pthread_t cons[100];
	int Nprod, Ncons = 0;
	int rc;   long t;
	int *ret = 0;
	int i;

	// * Handle execution syntax error
	FILE *rfile;
	if (argc == 1) {
		printf("usage: ./prod_cons <readfile> #Producer #Consumer\n");
		exit (0);
	}

	so_t *share = malloc(sizeof(so_t)); // * Allocate space for the shared object
	memset(share, 0, sizeof(so_t)); // * Initialize shared memory value

	// * Open the file
	rfile = fopen((char *) argv[1], "r");
	
	// * Check the syntax to determine the number of producers and consumers
	check_syntax(rfile, &Nprod, &Ncons, argv);

	// * Update the shared object
	share->rfile = rfile;
	share->line = NULL;
	pthread_mutex_init(&share->lock, NULL); // * Initialize the lock

	// * Create *Nprod* producers and *Ncons* consumers (as per defined amount)
	for (i = 0 ; i < Nprod ; i++)
	{
		pthread_create(&prod[i], NULL, producer, share);
	}
	for (i = 0 ; i < Ncons ; i++)
	{
		pthread_create(&cons[i], NULL, consumer, share);
	}
	printf("main continuing\n");

	// * Collect the return value from the producer and consumer threads
	for (i = 0 ; i < Ncons ; i++)
	{
		rc = pthread_join(cons[i], (void **) &ret);
		printf("main: consumer_%d joined with %d\n", i, *ret);
	}
	for (i = 0 ; i < Nprod ; i++)
	{
		rc = pthread_join(prod[i], (void **) &ret);
		printf("main: producer_%d joined with %d\n", i, *ret);
	}
	pthread_exit(NULL);
	exit(0);
}