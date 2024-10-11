#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "struct.h"

pthread_cond_t prod_cond, cons_cond;

void check_syntax(FILE *rfile, int *Nprod, int *Ncons, char *argv[])
{
    // * Check for file errors
    if (rfile == NULL) {
		perror("rfile");
		exit(0);
	}
    // * Check the number of producers and consumers
	if (argv[2] != NULL) {
		*Nprod = atoi(argv[2]);
		if (*Nprod > 100) *Nprod = 100;
		if (*Nprod == 0) *Nprod = 1;
	} else *Nprod = 1;
	if (argv[3] != NULL) {
		*Ncons = atoi(argv[3]);
		if (*Ncons > 100) *Ncons = 100;
		if (*Ncons == 0) *Ncons = 1;
	} else *Ncons = 1;
}

void read_line(ssize_t *read, char *line, size_t *len, FILE* rfile, so_t *so, int *i)
{
    while (1)
    {     
        while(so->full) pthread_cond_wait(&prod_cond, &so->lock); // * Make sure that the shared buffer is full
        pthread_mutex_lock(&so->lock); // * Lock the shared object

        // * Read a line (until '\n' is read) from the input file
        *read = getdelim(&line, len, '\n', rfile);

        // * If an error occurs, indicate that the buffer is full and break the loop
        if (*read == -1) {
            so->full = 1;
            so->line = NULL;
            pthread_mutex_unlock(&so->lock); // * Unlock the shared object
            pthread_cond_broadcast(&cons_cond); // * Signal all consumers that the EOF is reached (or an error occured)
            break;
        }

        // * Upon a successful return, share the line, move to the next line, and indicate that the buffer is full
        so->linenum = *i;
        so->line = strdup(line);
        *i += 1;
        so->full = 1;

        // * Unlock the shared object, send a signal to the consumer that a line is ready, and wait for a signal from consumer
        pthread_mutex_unlock(&so->lock);
        pthread_cond_signal(&cons_cond);
        pthread_cond_wait(&prod_cond, &so->lock);
    }
}

void process_line(char *line, int *len, so_t *so, int *i)
{
    int val = *i;
    while (1) {
        while(!so->full) pthread_cond_wait(&cons_cond, &so->lock); // * Make sure that the shared buffer is empty
        pthread_mutex_lock(&so->lock); // * Lock the shared object 
        line = so->line;

        // * Exit the loop if there is no more line to read (line is empty)
        if (line == NULL) break;

        *len = strlen(line); // * Measure the length of the line
        printf("Cons_%x: [%02d:%02d] %s",
            (unsigned int)pthread_self(), val, so->linenum, line);
            
        free(so->line);
        so->line = NULL;
        so->full = 0; // * Indicate that the buffer is empty

        // * Lock the lock and send a signal to the producer
        pthread_mutex_unlock(&so->lock);
        pthread_cond_signal(&prod_cond);
        val++; // * Increment the number of lines (return value)
    }
    *i = val;
}