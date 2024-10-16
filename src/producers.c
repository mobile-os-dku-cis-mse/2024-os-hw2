#include <pthread.h>
#include "producers.h"
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "words_array.h"

void *start_producer(void *producer_ptr)
{
    producer_t *producer = (producer_t *)producer_ptr;
    char *local_buffer = malloc(sizeof(char) * producer->char_to_read);
    size_t read_size = 0;

    // if (read(producer->fd, local_buffer, producer->char_to_read) == -1) {
    //     perror("Erreur lors de la lecture du fichier");
    //     pthread_exit(NULL);
    // }
    // pthread_mutex_lock(producer->mutex);
    // for (size_t i = 0; i < producer->char_to_read; i++) {
    //     producer->buffer[i + producer->index] = local_buffer[i];
    // }
    // pthread_mutex_unlock(producer->mutex);
    while (1) {
        pthread_mutex_lock(producer->mutex);
        if (getlinefd(&local_buffer, &read_size, producer->fd) == -1) {
            pthread_mutex_unlock(producer->mutex);
            break;
        }
        printf("Read: %s\n", local_buffer);
        strcat(producer->buffer, local_buffer);
        pthread_mutex_unlock(producer->mutex);
    }
    pthread_exit(NULL);
}

void init_producer(producer_t *producer, int fd, char *buffer,
    size_t char_to_read, pthread_mutex_t *mutex, off_t index)
{
    producer->fd = fd;
    producer->buffer = buffer;
    producer->char_to_read = char_to_read;
    producer->index = index;
    producer->thread = 0;
    producer->mutex = mutex;
}

int create_producers(size_t num_producers, int fd,
    size_t char_per_producers, char *buffer)
{
    producer_t *producers = malloc(sizeof(producer_t) * num_producers);
    pthread_mutex_t mutex;
    off_t index = 0;

    pthread_mutex_init(&mutex, NULL);
    for (size_t i = 0; i < num_producers; i++) {
        init_producer(&producers[i], fd, buffer, char_per_producers, &mutex, index);
        if (pthread_create(&producers[i].thread, NULL, start_producer,
        &producers[i]) == 1) {
            perror("Error while creating producer thread");
            return 84;
        }
        index += char_per_producers;
    }
    for (size_t i = 0; i < num_producers; i++) {
        pthread_join(producers[i].thread, NULL);
    }
    free(producers);
    return 0;
}

int process_producers(size_t num_producers, char *filename)
{
    int fd = open(filename, O_RDONLY);
    size_t char_per_producers = 0;
    char *buffer = NULL;
    struct stat statbuf;

    if (fd == -1) {
        perror("Error while opening file");
        return 1;
    }
    if (stat(filename, &(statbuf)) == -1) {
        perror("Error while getting file size");
        return 1;
    }
    buffer = malloc(sizeof(char) * statbuf.st_size);
    char_per_producers = statbuf.st_size / num_producers;
    create_producers(num_producers, fd, char_per_producers, buffer);
    close(fd);
    write(1, buffer, statbuf.st_size);
    return 0;
}
