#include "../include/queue.h"

#include <stdlib.h>
#include <stdio.h>


Queue *queue_init() {
    Queue *queue = (Queue *)malloc(sizeof(Queue));
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

    queue->head = NULL;
    queue->tail = NULL;
    queue->node_num = 0;
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
    queue->node_num ++;

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
    queue->node_num --;

    pthread_mutex_unlock(&queue->queue_lock);

    return buffer;
}

void queue_destroy(Queue *queue) {
    pthread_mutex_lock(&queue->queue_lock);
    while (queue_pop(queue) != NULL);
    free(queue);
    pthread_mutex_unlock(&queue->queue_lock);
}

bool queue_if_empty(Queue *queue) {
    return (queue->tail == NULL);
}

void queue_push_q(Queue *de_queue, Queue *src_queue) {
    pthread_mutex_lock(&de_queue->queue_lock);

    if(src_queue->tail != NULL) {
        if(de_queue->tail != NULL) {
            de_queue->tail->next = src_queue->head;
        } else {
            de_queue->head = src_queue->head;
        }
        de_queue->tail = src_queue->tail;
    }
    de_queue->node_num += src_queue->node_num;
    
    pthread_mutex_unlock(&de_queue->queue_lock);
}

void queue_reset(Queue *queue) {
    pthread_mutex_lock(&queue->queue_lock);
    queue->head = NULL;
    queue->tail = NULL;
    queue->node_num = 0;
    pthread_mutex_unlock(&queue->queue_lock);
}

int num_of_queue(Queue *queue) {
    return queue->node_num;
}