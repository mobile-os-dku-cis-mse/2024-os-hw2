//
// Created by beafowl on 9/10/24.
//

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "buffer.h"

void buffer_init(buffer_t* buffer) {
    buffer->head = 0;
    buffer->tail = 0;
    buffer->count = 0;
    pthread_mutex_init(&buffer->mutex, NULL);
    pthread_cond_init(&buffer->not_full, NULL);
    pthread_cond_init(&buffer->not_empty, NULL);
}

void buffer_put(buffer_t* buffer, char* line) {
    pthread_mutex_lock(&buffer->mutex);

    while (buffer->count == BUFFER_SIZE) {
        pthread_cond_wait(&buffer->not_full, &buffer->mutex);
    }

    if (line != NULL) {
        buffer->lines[buffer->head] = strdup(line);
        if (buffer->lines[buffer->head] == NULL) {
            perror("Failed to duplicate line");
            pthread_mutex_unlock(&buffer->mutex);
            return;
        }
    } else {
        buffer->lines[buffer->head] = NULL;
    }

    buffer->head = (buffer->head + 1) % BUFFER_SIZE;
    buffer->count++;

    pthread_cond_signal(&buffer->not_empty);
    pthread_mutex_unlock(&buffer->mutex);
}

char* buffer_get(buffer_t* buffer) {
    pthread_mutex_lock(&buffer->mutex);

    while (buffer->count == 0) {
        pthread_cond_wait(&buffer->not_empty, &buffer->mutex);
    }

    char* line = buffer->lines[buffer->tail];
    buffer->tail = (buffer->tail + 1) % BUFFER_SIZE;
    buffer->count--;

    pthread_cond_signal(&buffer->not_full);
    pthread_mutex_unlock(&buffer->mutex);

    return line;
}