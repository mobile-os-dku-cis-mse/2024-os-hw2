//
// Created by beafowl on 9/10/24.
//

#include <stdio.h>
#include <stdlib.h>
#include "consumer.h"
#include "buffer.h"

void* consumer(void* arg) {
    consumer_args_t* args = (consumer_args_t*) arg;
    buffer_t* buffer = args->buffer;

    while (1) {
        char* line = buffer_get(buffer);

        if (line == NULL) {
            break;
        }

        printf("Consumed: %s", line);
        free(line);
    }

    free(args);
    return NULL;
}
