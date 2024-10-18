#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "struct.h"
#define MAX_STRING_LENGTH 90
#define ASCII_SIZE	256

extern int stat [MAX_STRING_LENGTH];
extern int stat2 [ASCII_SIZE];

extern pthread_cond_t prod_cond, cons_cond;

void check_error(int test, unsigned char code)
{
    if(test != 0)
    {
        switch(code)
        {
            case 0:
                perror("pthread_join failed");
                break;

            case 1:
                perror("lock error");
                break;

            case 2:
                perror("cv wait error");
                break;

            case 3:
                perror("cv signal error");
                break;

            case 4:
                perror("cv error");
                break;
            
            case 5:
                perror("mutex error");
                break;

            case 6:
                perror("thread creation error");
                break;

            default:
                perror("unlisted error");
                break;
        }
        exit(EXIT_FAILURE);
    }
}

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
        if (*Nprod < 0)
        {
            printf("Invalid number of producers\n");
            exit(EXIT_FAILURE);
        }
	} else *Nprod = 1;
	if (argv[3] != NULL)
    {
		*Ncons = atoi(argv[3]);
		if (*Ncons > 100) *Ncons = 100;
		if (*Ncons == 0) *Ncons = 1;
        if (*Ncons < 0)
        {
            printf("Invalid number of consumers\n");
            exit(EXIT_FAILURE);
        }
	} else *Ncons = 1;
}

void read_line(ssize_t *read, char *line, size_t *len, FILE* rfile, so_t *so, int *i)
{
    int test;
    while (1)
    {
        // * Lock the shared object
        test = pthread_mutex_lock(&so->lock);
        check_error(test, 1);

        // * Wait while the buffer is full
        while (so->full && !so->end)
        {
            test = pthread_cond_wait(&prod_cond, &so->lock);
            check_error(test, 2);
        }

        // * Read a line (until '\n' is read) from the input file
        *read = getdelim(&line, len, '\n', rfile);

        // * If an error occurs or EOF is reached
        if (*read == -1)
        {
            so->full = 1;
            so->line = NULL;
            so->end = 1;
            test = pthread_mutex_unlock(&so->lock); // * Unlock the shared object
            check_error(test, 1);
            test = pthread_cond_broadcast(&cons_cond); // * Signal all consumers that EOF is reached
            check_error(test, 3);
            test = pthread_cond_broadcast(&prod_cond); // * Signal all producers that EOF is reached
            check_error(test, 3);
            break;
        }

        // * Upon successful read, copy the line to the shared buffer and mark the buffer as full
        so->linenum = *i;
        so->line = strdup(line);  // * Copy the line into shared memory
        *i += 1;
        so->full = 1; // * Indicate that the buffer is full

        // * Signal consumers that a new line is available
        test = pthread_cond_signal(&cons_cond);
        check_error(test, 3);

        // * Unlock the shared object and allow consumers to work
        test = pthread_mutex_unlock(&so->lock);
        check_error(test, 1);
    }
}

void process_line(char *line, int *len, so_t *so, int *i)
{
    char *line_copy = NULL;
    memset(stat2, 0, sizeof(stat));
    int test;
    while (1)
    {
        // * Lock the shared object
        test = pthread_mutex_lock(&so->lock);
        check_error(test, 1);
        
        // * Wait while the buffer is empty
        while (!so->full)
        {
            test = pthread_cond_wait(&cons_cond, &so->lock);
            check_error(test, 2);
        }

        // * Exit the loop if the producer has finished and there are no more lines
        if (so->line == NULL && so->end)
        {
            test = pthread_mutex_unlock(&so->lock);
            check_error(test, 1);
            break;
        }

        // * Process the line
        line = so->line;
        *len = strlen(line);  // * Measure the length of the line

        // * Calculate statistics
        line_copy = strdup(line);
        for(; *line_copy != '\0'; line_copy++)
        {
            stat2[*line_copy]++;
        }
        printf("Cons_%x: [%02d:%02d] %s", (unsigned int)pthread_self(), *i, so->linenum, line);

        // * Free the memory used for the line and mark the shared buffer as empty
        free(so->line);
        so->line = NULL;
        so->full = 0; // * Indicate that the shared buffer is empty

        // * Signal producers that the buffer is empty and they can produce
        test = pthread_cond_signal(&prod_cond);
        check_error(test, 3);

        // * Unlock the shared object
        test = pthread_mutex_unlock(&so->lock);
        check_error(test, 1);

        // * Increment the number of lines processed
        (*i)++; 
    }
}