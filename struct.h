#ifndef STRUCT_H
#define STRUCT_H

#include <stdio.h>
#include <pthread.h>

typedef struct sharedobject
{
	FILE *rfile; // * A file pointer for input file
	int linenum; // * Number of lines
	char *line; // * Current line string
	pthread_mutex_t lock; // * Shared object lock
	int full; // * Indicator that the shared buffer is full
	int end; // * Indicator that the EOF has been reached
} so_t;

#endif