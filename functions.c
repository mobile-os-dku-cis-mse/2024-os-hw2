#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "struct.h"

void check_syntax(FILE *rfile, int *Nprod, int *Ncons, char *argv[])
{
    if (rfile == NULL) {
		perror("rfile");
		exit(0);
	}
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
        // * Read a line (until '\n' is read) from the input file
        *read = getdelim(&line, len, '\n', rfile);

        // * If an error occurs, indicate that the buffer is full and break the loop
        if (*read == -1) {
            so->full = 1;
            so->line = NULL;
            pthread_cond_broadcast(&so->cons_cond); // * Signal all consumers that the EOF is reached (or an error occured)
            break;
        }

        // * Upon a successful return, share the line, move to the next line, and indicate that the buffer is full
        so->linenum = *i;
        so->line = strdup(line);      /* share the line */
        // /**/printf("Line is shared: %s\n", so->line);
        *i += 1;
        so->full = 1;
        pthread_cond_signal(&so->cons_cond);
        pthread_cond_wait(&so->prod_cond, &so->lock);
    }
}

void process_line(char *line, size_t *len, so_t *so, int *i)
{
    int val = *i;
    while (1) {
        if(so->line == NULL) pthread_cond_wait(&so->cons_cond, &so->lock);
        line = so->line;

        // * Exit the loop if there is no more line to read (line is empty)
        if (line == NULL) {
            break;
        }

        *len = strlen(line); // * Measure the length of the line
        printf("Cons_%x: [%02d:%02d] %s",
            (unsigned int)pthread_self(), val, so->linenum, line);
        free(so->line);
        so->line = NULL;
        so->full = 0; // * Indicate that the buffer is empty
        pthread_cond_signal(&so->prod_cond);
        val++;
    }
    *i = val;
}