#ifndef STRUCT_H
#define STRUCT_H

#include <stdio.h>
#include <pthread.h>

typedef struct sharedobject
{
	FILE *rfile; // * A file pointer for input file
	int linenum; // * Number of lines
	int current_index; // * Current line index
	char *line; // * Current line string
	pthread_mutex_t lock;
	pthread_mutex_t reader_lock;
	pthread_mutex_t writer_lock;
	unsigned char reader_count;
	unsigned char writer_count;
	int full;
	int end; // * Indicator that the EOF has been reached
} so_t;

#endif