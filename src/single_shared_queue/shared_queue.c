#include <pthread.h>
#include <stddef.h>
#include "shared_queue.h"

void shared_queue_init(shared_queue_t *queue, int producer_count) {
    queue->head = NULL;
    queue->tail = NULL;
    queue->count = 0;
    queue->eof = 0;
    queue->producer_count = producer_count;
    pthread_mutex_init(&queue->lock, NULL);
    pthread_cond_init(&queue->not_empty, NULL);

}

void shared_queue_destroy(shared_queue_t *queue) {
    pthread_mutex_destroy(&queue->lock);
    pthread_cond_destroy(&queue->not_empty);
}

void enqueue(shared_queue_t *queue, char *data) {
    node_t *new_node = malloc(sizeof(node_t));
    new_node->line = data;
    new_node->next = NULL;

    pthread_mutex_lock(&queue->lock);

    if (queue->tail == NULL) {
        queue->head = new_node;
        queue->tail = new_node;
    } else {
        queue->tail->next = new_node;
        queue->tail = new_node;
    }
    queue->count++;

    pthread_cond_signal(&queue->not_empty);
    pthread_mutex_unlock(&queue->lock);
}

char *dequeue(shared_queue_t *queue) {
    pthread_mutex_lock(&queue->lock);

    while (queue->head == NULL && queue->eof == 0) {
        pthread_cond_wait(&queue->not_empty, &queue->lock);
    }

    if (queue->head == NULL && queue->eof > 0) {
        pthread_mutex_unlock(&queue->lock);
        return NULL;
    }

    node_t *temp = queue->head;
    char *data = temp->line;
    queue->head = queue->head->next;

    if (queue->head == NULL) {
        queue->tail = NULL;
    }
    queue->count--;

    pthread_mutex_unlock(&queue->lock);
    free(temp);

    return data;
}