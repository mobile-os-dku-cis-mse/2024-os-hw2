#include <pthread.h>
#include <stddef.h>
#include "shared_queue.h"

void shared_queue_init(shared_queue_t *queue) {
    queue->head = NULL;
    queue->tail = NULL;
    queue->count = 0;
    queue->eof = 0;
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
        // 큐가 비어있는 경우
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
        // 모든 생산자가 EOF에 도달했고, 큐가 비어있음
        pthread_mutex_unlock(&queue->lock);
        return NULL;
    }

    node_t *temp = queue->head;
    char *data = temp->line;

    queue->head = queue->head->next;
    if (queue->head == NULL) {
        // 큐가 비어있게 되었으므로 tail도 NULL로 설정
        queue->tail = NULL;
    }
    queue->count--;

    pthread_mutex_unlock(&queue->lock);
    free(temp);

    return data;
}