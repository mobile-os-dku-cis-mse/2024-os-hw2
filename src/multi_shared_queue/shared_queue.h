//
// Created by hochacha on 24. 10. 15.
//

#ifndef SHARED_QUEUE_H
#define SHARED_QUEUE_H

#include <pthread.h>
#include <stdlib.h>

typedef struct _node{
  char *line;
  struct node *next;
}node_t;

typedef struct _shared_queue {
  node_t *head;
  node_t *tail;
  pthread_mutex_t lock;
  pthread_cond_t not_empty;
  int count;
  int eof;
}shared_queue_t;


void shared_queue_init(shared_queue_t *queue);
void shared_queue_destroy(shared_queue_t *queue);
void enqueue(shared_queue_t *queue, char *data);
char *dequeue(shared_queue_t *queue);
#endif //SHARED_QUEUE_H
