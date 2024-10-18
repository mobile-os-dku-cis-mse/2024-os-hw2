#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "functions.h"
#include "struct.h"
#define MAX_STRING_LENGTH 90
#define ASCII_SIZE	256

pthread_cond_t prod_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t cons_cond = PTHREAD_COND_INITIALIZER;

int stat [MAX_STRING_LENGTH];
int stat2 [ASCII_SIZE];

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

	// * Terminate the thread and return the number of lines read
	pthread_exit(ret);
}

void *consumer(void *arg)
{
	so_t *so = arg;
	int *ret = malloc(sizeof(int));
	int i = 0;
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
    if (argc == 1)
	{
        printf("usage: ./prod_cons <readfile> #Producer #Consumer\n");
        exit(0);
    }

    // * Shared object and synchronization setup
    so_t *share = malloc(sizeof(so_t));
    memset(share, 0, sizeof(so_t));
    share->rfile = fopen(argv[1], "r");
    rc = pthread_cond_init(&prod_cond, NULL);
	check_error(rc, 4);
    rc = pthread_cond_init(&cons_cond, NULL);
	check_error(rc, 4);
    rc = pthread_mutex_init(&share->lock, NULL);
	check_error(rc, 5);

    // * Determine the number of producers and consumers
    check_syntax(share->rfile, &Nprod, &Ncons, argv);

    // * Create Nprod producers and Ncons consumers
    for (i = 0; i < Nprod; i++)
	{
		rc = pthread_create(&prod[i], NULL, producer, share);
		check_error(rc, 6);
	}
    for (i = 0; i < Ncons; i++) 
	{
		rc = pthread_create(&cons[i], NULL, consumer, share);
		check_error(rc, 6);
	}

    // * Join consumer threads
    for (i = 0; i < Ncons; i++)
	{
        rc = pthread_join(cons[i], (void **) &ret);
		check_error(rc, 0);
        printf("main: consumer_%d joined with %d\n", i, *ret);
    }

    // * Join producer threads
    for (i = 0; i < Nprod; i++)
	{
        rc = pthread_join(prod[i], (void **) &ret);
		check_error(rc, 0);
        printf("main: producer_%d joined with %d\n", i, *ret);
    }

	// * Display statistics
	printf("\nCharacter Frequency Distribution:\n");
	printf("    A: %3d    B: %3d    C: %3d    D: %3d    E: %3d    F: %3d    G: %3d    H: %3d    I: %3d\n", stat2['A']+stat2['a'], stat2['B']+stat2['b'], stat2['C']+stat2['c'], stat2['D']+stat2['d'], stat2['E']+stat2['e'], stat2['F']+stat2['f'], stat2['G']+stat2['g'], stat2['H']+stat2['h'], stat2['I']+stat2['i']);
	printf("    J: %3d    K: %3d    L: %3d    M: %3d    N: %3d    O: %3d    P: %3d    Q: %3d    R: %3d\n", stat2['J']+stat2['j'], stat2['K']+stat2['k'], stat2['L']+stat2['l'], stat2['M']+stat2['m'], stat2['N']+stat2['n'], stat2['O']+stat2['o'], stat2['P']+stat2['p'], stat2['Q']+stat2['q'], stat2['R']+stat2['r']);
	printf("    S: %3d    T: %3d    U: %3d    V: %3d    W: %3d    X: %3d    Y: %3d    Z: %3d\n", stat2['S']+stat2['s'], stat2['T']+stat2['t'], stat2['U']+stat2['u'], stat2['V']+stat2['v'], stat2['W']+stat2['w'], stat2['X']+stat2['x'], stat2['Y']+stat2['y'], stat2['Z']+stat2['z']);

    // * Clean up
    rc = pthread_mutex_destroy(&share->lock);
	check_error(rc, 5);
    rc = pthread_cond_destroy(&prod_cond);
	check_error(rc, 4);
    rc = pthread_cond_destroy(&cons_cond);
	check_error(rc, 4);
    pthread_exit(NULL);
}