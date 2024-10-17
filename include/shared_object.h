#pragma once
#define MAX_THREADS 100
#define BUFFER_SIZE 10

#include <pthread.h>
#include <stdio.h>

typedef struct sharedobject {
	FILE *rfile;
	char *buffer[BUFFER_SIZE];
	int linenum[BUFFER_SIZE];
	int in;
	int out;
	int count;
	pthread_mutex_t lock;
	pthread_cond_t not_full;
	pthread_cond_t not_empty;
	int done;
    size_t alpha_char_number;
} so_t;

void parse_args(int argc, char *argv[], size_t *Nprod, size_t *Ncons);
so_t* init_shared_object(FILE *rfile);
