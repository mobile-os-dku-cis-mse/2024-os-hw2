#include <stdatomic.h>
#include <stdbool.h>
#include <stdlib.h>

typedef struct Node {
    int data;
    _Atomic(struct Node*) next;
} Node;

typedef struct {
    _Atomic(Node*) head;
    _Atomic(Node*) tail;
} LockFreeQueue;

LockFreeQueue* createQueue() {
    LockFreeQueue* queue = (LockFreeQueue*)malloc(sizeof(LockFreeQueue));
    Node* dummy = (Node*)malloc(sizeof(Node));
    dummy->next = ATOMIC_VAR_INIT(NULL);
    queue->head = ATOMIC_VAR_INIT(dummy);
    queue->tail = ATOMIC_VAR_INIT(dummy);
    return queue;
}

void enqueue(LockFreeQueue* queue, int value) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    newNode->data = value;
    newNode->next = ATOMIC_VAR_INIT(NULL);

    while (true) {
        Node* tail = atomic_load(&queue->tail);
        Node* next = atomic_load(&tail->next);
        if (tail == atomic_load(&queue->tail)) {
            if (next == NULL) {
                if (atomic_compare_exchange_weak(&tail->next, &next, newNode)) {
                    atomic_compare_exchange_weak(&queue->tail, &tail, newNode);
                    return;
                }
            } else {
                atomic_compare_exchange_weak(&queue->tail, &tail, next);
            }
        }
    }
}

bool dequeue(LockFreeQueue* queue, int* result) {
    while (true) {
        Node* head = atomic_load(&queue->head);
        Node* tail = atomic_load(&queue->tail);
        Node* next = atomic_load(&head->next);
        if (head == atomic_load(&queue->head)) {
            if (next == NULL) {
                return false; // 큐가 비어있음
            }
            if (head == tail) {
                atomic_compare_exchange_weak(&queue->tail, &tail, next);
            } else {
                *result = next->data;
                if (atomic_compare_exchange_weak(&queue->head, &head, next)) {
                    free(head);
                    return true;
                }
            }
        }
    }
}

void destroyQueue(LockFreeQueue* queue) {
    int temp;
    while (dequeue(queue, &temp));
    free(atomic_load(&queue->head));
    free(queue);
}
