#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "producer.h"
#include "buffer.h"

void* producer(void* arg) {
    producer_args_t* args = (producer_args_t*) arg;
    buffer_t* buffer = args->buffer;
    char* filename = args->filename;

    FILE* file = fopen(filename, "r");

    if (file == NULL) {
        perror("Failed to open file");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    int num_producers = args->num_producers;
    int producer_index = args->producer_index;

    long start_pos = (file_size / num_producers) * producer_index;
    long end_pos = (producer_index == num_producers - 1) ? file_size : (file_size / num_producers) * (producer_index + 1);

    fseek(file, start_pos, SEEK_SET);

    if (producer_index != 0) {
        int c;
        while ((c = fgetc(file)) != '\n' && c != EOF);
    }

    char* line = NULL;
    size_t len = 0;
    long current_pos = ftell(file);

    while (current_pos < end_pos && getline(&line, &len, file) != -1) {
        char* line_copy = strdup(line);
        if (line_copy == NULL) {
            perror("Failed to allocate memory for line");
            break;
        }
        buffer_put(buffer, line_copy);
        current_pos = ftell(file);
    }

    free(line);
    fclose(file);
    free(args);

    return NULL;
}