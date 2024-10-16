#include <stdio.h>
#include <stdlib.h>
#include "producers.h"
#include "queue.h"

int main(int argc, char **argv)
{
    size_t num_consumers = 0;
    size_t num_producers = 0;

    if (argc != 4) {
        fprintf(stderr, "Usage: %s <filename> <number of consummers> <number \
of producers>\n", argv[0]);
        return 1;
    }
    num_consumers = strtoul(argv[2], NULL, 0);
    num_producers = strtoul(argv[3], NULL, 0);
    if (num_consumers == 0 || num_producers == 0) {
        fprintf(stderr, "Number of consumers and producers must be greater than\
 0\n");
    }
    process_producers(num_producers, argv[1]);
    return 0;
}
