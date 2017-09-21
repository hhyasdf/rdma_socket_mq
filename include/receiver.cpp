#include "receiver.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "rdma_socket.h"
#include "queue.h"



Receiver *receiver_build() {
    Receiver *n = (Receiver *)malloc(sizeof(Receiver));

    n->listener = nullptr;
    n->recv_queue = queue_init();
    n->socket_queue = queue_init();

    n->cond = PTHREAD_COND_INITIALIZER;
    return n;
}

static void *recv_process(void *listen) {
    Socket *l = (Socket *)listen;
    Queue *msg_queue = l->msg_queue;
    Message *msg; 
    while(1) {
        recv_(l, (void **)&msg);

        if(msg == NULL){
            queue_push_q(l->receiver->recv_queue, msg_queue);
            break;
        }

        if (msg->flag == SNDMORE_FLAG) {
            queue_push(l->msg_queue, (void *)msg);
        } else {
            if(!queue_if_empty(msg_queue)) {
                queue_push_q(l->receiver->recv_queue, msg_queue);
            }
            queue_push(l->receiver->recv_queue, (void *)msg);

            pthread_cond_signal(&l->receiver->cond);
        }
    }
}

static void *listen_process(void *re) {
    Socket *listen = nullptr;
    pthread_t p_id;

    while(1) {
        listen = accept_(((Receiver *)re)->listener, (Receiver *)re);
        
        queue_push(((Receiver *)re)->socket_queue, static_cast<void *>(listen));
        pthread_create(&p_id, NULL, recv_process, (void *)listen);
    }
}


void reciever_bind(Receiver* re, int port) {
    struct sockaddr_in addr;
    pthread_t p_id;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    memset(&addr, 0, sizeof(addr));
    addr.sin_port = htons(port);
    addr.sin_family = AF_INET;

    Socket *socket = nullptr;

    socket = socket_(RDMA_PS_TCP);
    bind_(socket, &addr, AF_INET);

    listen_(socket, 100);
    re->listener = socket;

    pthread_create(&static_cast<Receiver *>(re)->p_id, &attr, recv_process, (void *)re);
}

Message *receiver_recv(Receiver* re) {
    pthread_mutex_lock(&re->recv_queue->queue_lock);
    if(queue_if_empty(re->recv_queue)) {
        pthread_cond_wait(&re->cond, &re->recv_queue->queue_lock);
    }
    pthread_mutex_unlock(&re->recv_queue->queue_lock);

    return static_cast<Message *>(queue_pop(re->recv_queue));
}


void receiver_close(Receiver *re) {
    Socket *socket = nullptr;
    close_(re->listener);
    while((socket = static_cast<Socket *>(queue_pop(re->socket_queue))) != NULL) {
        close_(socket);
    }
    queue_destroy(re->socket_queue);
    queue_destroy(re->recv_queue);
}

