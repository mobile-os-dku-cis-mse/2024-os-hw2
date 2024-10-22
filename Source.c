#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <pthread.h>

#include <stdlib.h>
#define MAX_LINE_LENGTH 1024
#define ALPHABET_SIZE 26

typedef struct shrdobj {
    FILE* rfile;
    char line[MAX_LINE_LENGTH];
    pthread_mutex_t lock;
    pthread_cond_t cond_prod;
    pthread_cond_t cond_cons;
    int full; // If consumable data exists
    int done; // If producer is done
} so_t;
void* prod(void* arg) {
    so_t* so = arg;
    int i = 0;
    while (fgets(so->line, MAX_LINE_LENGTH, so->rfile) != NULL) {
        pthread_mutex_lock(&so->lock);
        so->full = 1; // Full marking
        pthread_cond_signal(&so->cond_cons); // Signal consumers
        pthread_mutex_unlock(&so->lock);
        i++;
    }
    pthread_mutex_lock(&so->lock);
    so->done = 1; // If producer is done
    pthread_cond_broadcast(&so->cond_cons); // Return to waiting consumers
    pthread_mutex_unlock(&so->lock);

    printf("Producer: produced %d lines\n", i);
    pthread_exit(NULL);
}

void* cons(void* arg) {
    so_t* so = arg;
    int count[ALPHABET_SIZE] = { 0 };
    int i = 0;
    while (1) {
        pthread_mutex_lock(&so->lock);
        while (!so->full && !so->done) { // Wait for data or consumer to finish
            pthread_cond_wait(&so->cond_cons, &so->lock);
        }
        if (so->done && !so->full) {
            pthread_mutex_unlock(&so->lock);
            break; // If no data and consumer finish --> Exit
        }
        // Process line
        char* line = so->line;
        so->full = 0; // Mark empty
        pthread_mutex_unlock(&so->lock);
        // Count
        for (int j = 0; line[j] != '\0'; j++) {
            if (isalpha(line[j])) {
                count[tolower(line[j]) - 'a']++;
            }
        }
        i++;
    }
    // Display statistics
    printf("Consumer %lx: processed %d lines\n", pthread_self(), i);
    printf("Character counts:\n");
    for (int k = 0; k < ALPHABET_SIZE; k++) {
        if (count[k] > 0) {
            printf("%c: %d\n", 'a' + k, count[k]);
        }
    }
    pthread_exit(NULL);
}
int main(int argc, char* argv[]) {
    pthread_t prod;
    pthread_t cons[100];
    int Ncons;
    FILE* rfile;
    if (argc != 3) {
        printf("usage: ./prod_cons <readfile> #Consumer\n");
        exit(1);
    }
    Ncons = atoi(argv[2]);
    if (Ncons > 100) Ncons = 100; // Max 100 consumers
    so_t* share = malloc(sizeof(so_t));
    memset(share, 0, sizeof(so_t));
    rfile = fopen(argv[1], "r");
    if (rfile == NULL) {
        perror("rfile");
        exit(1);
    }
    share->rfile = rfile;
    pthread_mutex_init(&share->lock, NULL);
    pthread_cond_init(&share->cond_prod, NULL);
    pthread_cond_init(&share->cond_cons, NULL);
    share->full = 0;
    share->done = 0;
    pthread_create(&prod, NULL, prod, share);
    for (int i = 0; i < Ncons; i++) {
        pthread_create(&cons[i], NULL, cons, share);
    }
    pthread_join(prod, NULL);
    for (int i = 0; i < Ncons; i++) {
        pthread_join(cons[i], NULL);
    }
    fclose(rfile);
    pthread_mutex_destroy(&share->lock);
    pthread_cond_destroy(&share->cond_prod);
    pthread_cond_destroy(&share->cond_cons);
    free(share);
    return 0;
}
