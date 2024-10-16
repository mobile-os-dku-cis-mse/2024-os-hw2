#pragma once
#include <stddef.h>
#include <pthread.h>
#include <stdio.h>

typedef struct producer {
    int fd;
    char *line;
    size_t char_to_read;
    off_t index;
    pthread_t thread;
    pthread_mutex_t *mutex;
} producer_t;

int process_producers(size_t num_producers, char *filename);
