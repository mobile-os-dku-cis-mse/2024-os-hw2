#ifndef STRUCT_H
#define STRUCT_H

#include <stdio.h>
#include <pthread.h>

typedef struct sharedobject {
	FILE *rfile; // * A file pointer for input file
	int linenum; // * Number of lines
	char *line; // * Current line
	pthread_mutex_t lock;
	int full;

} so_t;

#endif