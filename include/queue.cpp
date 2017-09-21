#include "queue.h"
#include <stdlib.h>


Queue *queue_init() {
    Queue *queue = (Queue *)malloc(sizeof(Queue));
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

    queue->head = NULL;
    queue->tail = NULL;
    pthread_mutex_init(&queue->queue_lock, &attr);

    return queue;
}


void queue_push(Queue *queue, void *buffer) {
    pthread_mutex_lock(&queue->queue_lock);

    Node *new_node = (Node *)malloc(sizeof(Node));

    new_node->buffer = buffer;

    if (queue->tail == NULL) {
        queue->head = new_node;
    } else {
        queue->tail->next = new_node;
    }

    queue->tail = new_node;
    new_node->next = NULL;

    pthread_mutex_unlock(&queue->queue_lock);
}

void *queue_pop(Queue *queue) {
    pthread_mutex_lock(&queue->queue_lock);

    Node *old_head = queue->head;
    if(old_head == NULL) {
        pthread_mutex_unlock(&queue->queue_lock);
        return NULL;
    }
    
    void *buffer;
    
    buffer = old_head->buffer;
    queue->head = queue->head->next;

    if(queue->head == NULL) {
        queue->tail = NULL;
    }

    free(old_head);

    pthread_mutex_unlock(&queue->queue_lock);
    return buffer;
}

void queue_destroy(Queue *queue) {
    pthread_mutex_lock(&queue->queue_lock);
    while (queue_pop(queue) != NULL);
    free(queue);
    pthread_mutex_unlock(&queue->queue_lock);
}