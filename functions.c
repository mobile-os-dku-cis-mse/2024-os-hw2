#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "struct.h"
#define MAX_STRING_LENGTH 90
#define ASCII_SIZE	256

int stat [MAX_STRING_LENGTH];
int stat2 [ASCII_SIZE];

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
    char *line_copy = NULL;
    memset(stat2, 0, sizeof(stat));
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

        // * Calculate statistics
        line_copy = strdup(line);
        for(; *line_copy != '\n'; line_copy++)
        {
            stat2[*line_copy]++;
        }
        printf("Cons_%x: [%02d:%02d] %s", (unsigned int)pthread_self(), val, so->linenum, line);
        printf("    A: %3d    B: %3d    C: %3d    D: %3d    E: %3d    F: %3d    G: %3d    H: %3d    I: %3d\n", stat2['A']+stat2['a'], stat2['B']+stat2['b'], stat2['C']+stat2['c'], stat2['D']+stat2['d'], stat2['E']+stat2['e'], stat2['F']+stat2['f'], stat2['G']+stat2['g'], stat2['H']+stat2['h'], stat2['I']+stat2['i']);
        printf("    J: %3d    K: %3d    L: %3d    M: %3d    N: %3d    O: %3d    P: %3d    Q: %3d    R: %3d\n", stat2['J']+stat2['j'], stat2['K']+stat2['k'], stat2['L']+stat2['l'], stat2['M']+stat2['m'], stat2['N']+stat2['n'], stat2['O']+stat2['o'], stat2['P']+stat2['p'], stat2['Q']+stat2['q'], stat2['R']+stat2['r']);
        printf("    S: %3d    T: %3d    U: %3d    V: %3d    W: %3d    X: %3d    Y: %3d    Z: %3d\n", stat2['S']+stat2['s'], stat2['T']+stat2['t'], stat2['U']+stat2['u'], stat2['V']+stat2['v'], stat2['W']+stat2['w'], stat2['X']+stat2['x'], stat2['Y']+stat2['y'], stat2['Z']+stat2['z']);

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