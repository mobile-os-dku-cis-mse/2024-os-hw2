//
// Created by beafowl on 9/10/24.
//

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "producer.h"
#include "consumer.h"
#include "buffer.h"

void* producer(void* arg);
void* consumer(void* arg);

int main(int argc, char *argv[]) {
    int producer_nbr = 0;
    int consumer_nbr = 0;

    if (argc < 2 || argc > 4) {
        fprintf(stderr, "usage: ./prod_cons <readfile> #Producer #Consumer\n");
        return 1;
    }

    if (argv[2] != NULL) {
        producer_nbr = atoi(argv[2]);
        if (producer_nbr == 0)
            fprintf(stderr, "Producer must be greater than 0\n");
    } else {
        producer_nbr = 1;
    }

    if (argv[3] != NULL) {
        consumer_nbr = atoi(argv[3]);
        if (consumer_nbr == 0)
            fprintf(stderr, "Consumer must be greater than 0\n");
    } else {
        consumer_nbr = 1;
    }

    char* filename = argv[1];
    pthread_t prod[producer_nbr];
    pthread_t cons[consumer_nbr];
    buffer_t buffer;
    buffer_init(&buffer);

    for (int i = 0; i < producer_nbr; i++) {
        producer_args_t* p_args = (producer_args_t*)malloc(sizeof(producer_args_t));
        p_args->buffer = &buffer;
        p_args->filename = filename;
        p_args->num_producers = producer_nbr;
        p_args->producer_index = i;

        if (pthread_create(&prod[i], NULL, producer, (void*)p_args) != 0) {
            perror("Failed to create producer thread");
            return 1;
        }
    }

    for (int i = 0; i < consumer_nbr; i++) {
        consumer_args_t* c_args = (consumer_args_t*)malloc(sizeof(consumer_args_t));
        c_args->buffer = &buffer;

        if (pthread_create(&cons[i], NULL, consumer, (void*)c_args) != 0) {
            perror("Failed to create consumer thread");
            return 1;
        }
    }

    for (int i = 0; i < producer_nbr; i++) {
        pthread_join(prod[i], NULL);
    }

    for (int i = 0; i < consumer_nbr; i++) {
        buffer_put(&buffer, NULL);
    }

    for (int i = 0; i < consumer_nbr; i++) {
        pthread_join(cons[i], NULL);
    }

    return 0;
}
