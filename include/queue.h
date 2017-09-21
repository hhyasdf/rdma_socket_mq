#include <pthread.h>

#ifndef _RECVQUEUE_
#define _RECVQUEUE_

typedef struct Node_ {
    void *buffer;
    struct Node_ *next;
}Node;

typedef struct Queue_ {
    Node *head;
    Node *tail;
    pthread_mutex_t queue_lock;
}Queue;


Queue *queue_init();
void queue_push(Queue *queue, void *buffer);
void *queue_pop(Queue *queue);
void queue_destroy(Queue *queue);

#endif