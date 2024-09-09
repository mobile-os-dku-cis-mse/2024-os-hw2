//
// Created by beafowl on 9/10/24.
//

#ifndef BUFFER_H
#define BUFFER_H

#include <pthread.h>

#define BUFFER_SIZE 10

typedef struct {
    char* lines[BUFFER_SIZE];
    int head;
    int tail;
    int count;
    pthread_mutex_t mutex;
    pthread_cond_t not_full;
    pthread_cond_t not_empty;
} buffer_t;

typedef struct {
    buffer_t* buffer;
    char* filename;
    int num_producers;
    int producer_index;
} producer_args_t;

typedef struct {
    buffer_t* buffer;
} consumer_args_t;

void buffer_init(buffer_t* buffer);
void buffer_put(buffer_t* buffer, char* line);
char* buffer_get(buffer_t* buffer);

#endif //BUFFER_H
