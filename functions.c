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
    if (rfile == NULL)
    {
		perror("rfile");
		exit(0);
	}
    // * Check the number of producers and consumers
	if (argv[2] != NULL)
    {
		*Nprod = atoi(argv[2]);
		if (*Nprod > 100) *Nprod = 100;
		if (*Nprod == 0) *Nprod = 1;
	} else *Nprod = 1;
	if (argv[3] != NULL)
    {
		*Ncons = atoi(argv[3]);
		if (*Ncons > 100) *Ncons = 100;
		if (*Ncons == 0) *Ncons = 1;
	} else *Ncons = 1;
}

void read_line(ssize_t *read, char *line, size_t *len, FILE* rfile, so_t *so, int *i)
{
    while (1)
    {
        // * Lock the shared object
        pthread_mutex_lock(&so->lock);

        // * Wait while the buffer is full
        while (so->full && !so->end) pthread_cond_wait(&prod_cond, &so->lock);

        // * Read a line (until '\n' is read) from the input file
        *read = getdelim(&line, len, '\n', rfile);

        // * If an error occurs or EOF is reached
        if (*read == -1)
        {
            so->full = 1;
            so->line = NULL;
            so->end = 1;
            pthread_mutex_unlock(&so->lock); // * Unlock the shared object
            pthread_cond_broadcast(&cons_cond); // * Signal all consumers that EOF is reached
            pthread_cond_broadcast(&prod_cond);
            break;
        }

        // * Upon successful read, copy the line to the shared buffer and mark the buffer as full
        so->linenum = *i;
        so->line = strdup(line);  // * Copy the line into shared memory
        *i += 1;
        so->full = 1; // * Indicate that the buffer is full

        // * Signal consumers that a new line is available
        pthread_cond_signal(&cons_cond);

        // * Unlock the shared object and allow consumers to work
        pthread_mutex_unlock(&so->lock);
    }
}

void process_line(char *line, int *len, so_t *so, int *i)
{
    int val = *i;
    while (1)
    {
        // * Lock the shared object
        pthread_mutex_lock(&so->lock);
        
        // * Wait while the buffer is empty
        while (!so->full) pthread_cond_wait(&cons_cond, &so->lock);

        // * Exit the loop if the producer has finished and there are no more lines
        if (so->line == NULL)
        {
            pthread_mutex_unlock(&so->lock);
            break;
        }

        // * Process the line
        line = so->line;
        *len = strlen(line);  // * Measure the length of the line
        printf("Cons_%x: [%02d:%02d] %s", (unsigned int)pthread_self(), val, so->linenum, line);
        
        // * Free the memory used for the line and mark the buffer as empty
        free(so->line);
        so->line = NULL;
        so->full = 0; // * Buffer is now empty

        // * Signal producers that the buffer is empty and they can produce
        pthread_cond_signal(&prod_cond);

        // * Unlock the shared object
        pthread_mutex_unlock(&so->lock);

        // * Increment the number of lines processed
        val++; 
    }
    // * Update the line count
    *i = val;
}